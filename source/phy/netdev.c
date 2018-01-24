/*
 * E/STACK - Network device core
 *
 * Author: Michel Megens
 * Date: 03/12/2017
 * Email: dev@bietje.net
 * 
 * This is an additional 'virtual' layer in the OSI model, that
 * sits between the datalink layer protocol and the PHY-layer (i.e.
 * the actual device drivers). This layer is purely an administrative
 * layer to ensure thread and memory safety.
 */

/**
 * @addtogroup netdev
 * @{
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <estack/estack.h>
#include <estack/netbuf.h>
#include <estack/ethernet.h>
#include <estack/list.h>
#include <estack/netdev.h>
#include <estack/error.h>
#include <estack/inet.h>
#include <estack/log.h>

/**
 * @brief Network device core data.
 */
struct dev_core {
	struct list_head devices; //!< Global device list.
	struct list_head dst_cache; //!< Global destination / ARP cache.
	estack_mutex_t mtx; //!< Global device lock.
	estack_thread_t runner; //!< Processing runner.
	estack_event_t event; //!< Data event.
	volatile bool running; //!< Core initialisation indicator.
};

static struct dev_core devcore = {
	.devices = STATIC_INIT_LIST_HEAD(devcore.devices),
	.dst_cache = STATIC_INIT_LIST_HEAD(devcore.dst_cache),
};

static uint32_t dst_resolve_tmo = 4500000;
static uint32_t dst_retry_tmo = 1000000;
static int dst_retries = 4;

/**
 * @brief Lock the networking core.
 * @note This function will acquire struct dev_core::mtx.
 */
static inline void netdev_lock_core(void)
{
	estack_mutex_lock(&devcore.mtx, 0);
}

/**
 * @brief Unlock the networking core.
 * @note This function will release struct dev_core::mtx.
 */
static inline void netdev_unlock_core(void)
{
	estack_mutex_unlock(&devcore.mtx);
}

/**
 * @brief Search the global device list.
 * @param name Device name to search for.
 * @return A device with name \p name or \p NULL.
 */
struct netdev *netdev_find(const char *name)
{
	struct list_head *entry;
	struct netdev *dev;

	netdev_lock_core();
	list_for_each(entry, &devcore.devices) {
		dev = list_entry(entry, struct netdev, entry);
		if(!strcmp(dev->name, name)) {
			netdev_unlock_core();
			return dev;
		}
	}
	netdev_unlock_core();

	return NULL;
}

static inline int netdev_backlog_empty(struct netdev *dev)
{
	return list_empty(&dev->backlog.head);
}

/**
 * @brief Lock a network device.
 * @param dev Network device to lock.
 * @note This function will acquire struct netdev::mtx.
 */
static inline void netdev_lock(struct netdev *dev)
{
	assert(dev);
	estack_mutex_lock(&dev->mtx, 0);
}

/**
 * @brief Unlock a network device.
 * @param dev Network device to unlock.
 * @note This function will release struct netdev::mtx.
 */
static inline void netdev_unlock(struct netdev *dev)
{
	assert(dev);
	estack_mutex_unlock(&dev->mtx);
}

/**
 * @brief Get the global list of network devices.
 * @return The list head to all registered network devices.
 * @see netdev_find
 */
struct list_head *netdev_get_devices(void)
{
	return &devcore.devices;
}

/**
 * @brief Find and remove a given device.
 * @param name Name to search for.
 * @return The device that was removed or \p NULL.
 */
struct netdev *netdev_remove(const char *name)
{
	struct netdev *dev;

	dev = netdev_find(name);
	if(!dev)
		return NULL;

	netdev_lock(dev);
	list_del(&dev->entry);
	netdev_unlock(dev);
	return dev;
}

static inline void __netdev_add_backlog(struct netdev *dev, struct netbuf *nb)
{
	list_add_tail(&nb->bl_entry, &dev->backlog.head);
	dev->backlog.size += 1;
}

/**
 * @brief Add a packet buffer to the backlog of \p dev.
 * @param dev Device to add \p nb to.
 * @param nb Packet buffer to add.
 *
 * Adds a netbuf to the the backlog and increments the backlog size by one. All
 * packets on the backlog are expected to have a valid output device set.
 */
void netdev_add_backlog(struct netdev *dev, struct netbuf *nb)
{
	assert(dev);
	assert(nb);

	netdev_lock(dev);
	__netdev_add_backlog(dev, nb);
	netdev_unlock(dev);
}

static inline void netdev_remove_backlog_entry(struct netdev *dev, struct netbuf *nb)
{
	list_del(&nb->bl_entry);
	dev->backlog.size -= 1;
}

static int netdev_backlog_length(struct netdev *dev)
{
	int length;

	assert(dev);
	netdev_lock(dev);
	length = dev->backlog.size;
	netdev_unlock(dev);

	return length;
}

static void netdev_rx_stats_inc(struct netdev *dev, struct netbuf *nb)
{
	struct netdev_stats *stats;

	stats = &dev->stats;
	stats->rx_packets++;
	stats->rx_bytes += nb->size;
}

static void netdev_tx_stats_inc(struct netdev *dev, struct netbuf *nb)
{
	struct netdev_stats *stats;

	stats = &dev->stats;
	stats->tx_packets++;
	stats->tx_bytes += nb->size;
}

static inline void netdev_dropped_stats_inc(struct netdev *dev)
{
	struct netdev_stats *stats;

	stats = &dev->stats;
	stats->dropped++;
}

static struct protocol *netdev_find_protocol(struct netdev *dev, uint16_t proto)
{
	struct list_head *entry;
	struct protocol *p;

	netdev_lock(dev);
	list_for_each(entry, &dev->protocols) {
		p = list_entry(entry, struct protocol, entry);
		if(p->protocol == proto) {
			netdev_unlock(dev);
			return p;
		}
	}

	netdev_unlock(dev);
	return NULL;
}

/**
 * @brief Add a protocol handler a to a netork device.
 * @param dev Network device to add the protocol handler to.
 * @param proto Protocol identifier.
 * @param handle Handler function.
 * @return True or false depending on whether or not the handler was added.
 */
bool netdev_add_protocol(struct netdev *dev, uint16_t proto, rx_handle handle)
{
	struct protocol *p;

	assert(dev);
	p = netdev_find_protocol(dev, proto);

	if(p)
		return false;

	p = malloc(sizeof(*p));
	assert(p);

	list_head_init(&p->entry);
	p->protocol = proto;
	p->rx = handle;
	netdev_lock(dev);
	list_add_tail(&p->entry, &dev->protocols);
	netdev_unlock(dev);

	return true;
}

/**
 * @brief Remove a protocol handler from a network device.
 * @param dev Network device to remove \p proto from.
 * @param proto Protocol identifier.
 * @return True or false depending on whether or not the handler was removed.
 */
bool netdev_remove_protocol(struct netdev *dev, uint16_t proto)
{
	struct protocol *p;

	assert(dev);
	p = netdev_find_protocol(dev, proto);

	if(p) {
		netdev_lock(dev);
		list_del(&p->entry);
		netdev_unlock(dev);
		free(p);
		return true;
	}

	return false;
}

/**
 * @brief Add a destination cache entry to \p dev.
 * @param dev Device to add the destination cache entry to.
 * @param dst Datalink layer address.
 * @param daddrlen Length of \p dst.
 * @param src Network layer address.
 * @param saddrlen Length of \p saddrlen.
 */
void netdev_add_destination(struct netdev *dev, const uint8_t *dst, uint8_t daddrlen,
	const uint8_t *src, uint8_t saddrlen)
{
	struct dst_cache_entry *centry;

	centry = z_alloc(sizeof(*centry));
	assert(centry);

	centry->state = DST_RESOLVED;
	centry->saddr_length = saddrlen;
	centry->hwaddr_length = daddrlen;

	centry->saddr = malloc(saddrlen);
	memcpy(centry->saddr, src, saddrlen);

	centry->hwaddr = malloc(daddrlen);
	memcpy(centry->hwaddr, dst, daddrlen);

	list_head_init(&centry->entry);
	list_head_init(&centry->packets);
	netdev_lock(dev);
	list_add(&centry->entry, &dev->destinations);
	netdev_unlock(dev);
}

/**
 * @brief Add a partially comlete cache entry.
 * @param dev Device to add the cache entry to.
 * @param src Destination network layer IP address.
 * @param length Length of \p src in bytes.
 * @param handler Handler to retry resolving the entry.
 * @return The created destination cache entry.
 */
struct dst_cache_entry *netdev_add_destination_unresolved(struct netdev *dev,
	const uint8_t *src, uint8_t length, resolve_handle handler)
{
	struct dst_cache_entry *centry;

	centry = z_alloc(sizeof(*centry));
	assert(centry);

	centry->state = DST_UNFINISHED;
	centry->saddr_length = length;

	centry->saddr = malloc(length);
	memcpy(centry->saddr, src, length);

	list_head_init(&centry->entry);
	list_head_init(&centry->packets);

	centry->timeout = estack_utime() + dst_resolve_tmo;
	centry->retry = dst_retries;
	centry->translate = handler;

	netdev_lock(dev);
	list_add(&centry->entry, &dev->destinations);
	netdev_unlock(dev);

	return centry;
}

/**
 * @brief Add a packet buffer to a destination cache entry.
 * @param dev Device to which \p e belongs.
 * @param e Destination cache entry to add \p nb to.
 * @param nb Packet buff to add to \p e.
 * @return True or false based on whether the packet buffer was added or not.
 */
bool netdev_dstcache_add_packet(struct netdev *dev, struct dst_cache_entry *e, struct netbuf *nb)
{
	assert(e);
	assert(nb);

	if(e->state != DST_UNFINISHED)
		return false;

	assert(nb->dev);
	e->timeout = estack_utime() + dst_resolve_tmo;
	netdev_lock(dev);
	list_add(&nb->bl_entry, &e->packets);
	netdev_unlock(dev);

	return true;
}

/**
 * @brief Update a destination cache entry.
 * @param dev Device to update a destination cache entry for.
 * @param dst Updated datalink layer address.
 * @param dlength Length of \p dst.
 * @param src Network layer address.
 * @param slength Length of \p saddrlen.
 * @return True of false based on whether the entry was updated or not.
 */
bool netdev_update_destination(struct netdev *dev, const uint8_t *dst, uint8_t dlength,
	const uint8_t *src, uint8_t slength)
{
	struct list_head *entry;
	struct dst_cache_entry *centry;

	netdev_lock(dev);
	list_for_each(entry, &dev->destinations) {
		centry = list_entry(entry, struct dst_cache_entry, entry);
		if(centry->saddr_length != slength)
			continue;

		if(!memcmp(centry->saddr, src, slength)) {
			if(dlength != centry->hwaddr_length)
				centry->hwaddr = realloc(centry->hwaddr, dlength);

			memcpy(centry->hwaddr, dst, dlength);
			centry->state = DST_RESOLVED;
			netdev_unlock(dev);
			return true;
		}
	}
	netdev_unlock(dev);

	return false;
}

static inline void netdev_free_dst_entry(struct dst_cache_entry *e)
{
	if(e->saddr)
		free(e->saddr);

	if(e->hwaddr)
		free(e->hwaddr);

	free(e);
}

/**
 * @brief Remove a destination cache entry.
 * @param dev Device to remove the cache entry from.
 * @param src Network layer address.
 * @param length Length of \p saddrlen.
 * @return True or false based on whether the entry was removed or not.
 */
bool netdev_remove_destination(struct netdev *dev, const uint8_t *src, uint8_t length)
{
	struct list_head *entry;
	struct dst_cache_entry *centry;

	netdev_lock(dev);
	list_for_each(entry, &dev->destinations) {
		centry = list_entry(entry, struct dst_cache_entry, entry);
		if(centry->saddr_length != length)
			continue;

		if(!memcmp(centry->saddr, src, length)) {
			list_del(entry);
			netdev_free_dst_entry(centry);
			netdev_unlock(dev);
			return true;
		}
	}

	netdev_unlock(dev);
	return false;
}

/**
 * @brief Perform a lookup on the destination cache of \p dev.
 * @param dev Device to perform a dst cache lookup on.
 * @param src Network layer address.
 * @param length Length of \p saddrlen.
 * @return The destination cache entry or \p NULL.
 */
struct dst_cache_entry *netdev_find_destination(struct netdev *dev, const uint8_t *src, uint8_t length)
{
	struct list_head *entry;
	struct dst_cache_entry *centry;

	netdev_lock(dev);
	list_for_each(entry, &dev->destinations) {
		centry = list_entry(entry, struct dst_cache_entry, entry);
		if(centry->saddr_length != length)
			continue;

		if(!memcmp(centry->saddr, src, length)) {
			netdev_unlock(dev);
			return centry;
		}
	}

	netdev_unlock(dev);
	return NULL;
}

static bool netdev_dst_timeout(struct dst_cache_entry *e)
{
	time_t time;

	time = estack_utime();
	return time > e->timeout;
}

static void netdev_drop_dst(struct dst_cache_entry *e)
{
	struct list_head *entry, *tmp;
	struct netbuf *nb;

	list_for_each_safe(entry, tmp, &e->packets) {
		nb = list_entry(entry, struct netbuf, bl_entry);
		list_del(entry);
		netdev_dropped_stats_inc(nb->dev);
		netbuf_free(nb);
	}
}

static void netdev_try_translate_cache(struct netdev *dev)
{
	struct list_head *dst, *dsttmp, *entry, *tmp;
	struct dst_cache_entry *e;
	struct netbuf *nb;

	list_for_each_safe(dst, dsttmp, &dev->destinations) {
		e = list_entry(dst, struct dst_cache_entry, entry);

		if(e->state == DST_UNFINISHED) {
			/*
			 * Attempt to clean up any unfinished destination
			 * cache entries. Unfinished entries are entries that
			 * have been requested but have not received an ARP / ICMP6
			 * reply as of yet. Attempt to send out another completion request
			 * or drop the entry if a timeout has ocurred
			 */
			if(netdev_dst_timeout(e) || !e->translate ||
				e->retry <= 0 || list_empty(&e->packets)) {
				netdev_drop_dst(e);
				list_del(dst);
				netdev_free_dst_entry(e);
				continue;
			}

			if(e->last_attempt + dst_retry_tmo < estack_utime()) {
				netdev_unlock(dev);
				e->translate(dev, e->saddr);
				netdev_lock(dev);
				e->retry--;
				e->last_attempt = estack_utime();
			}
			continue;
		}

		/*
		 * Push all packets that are waiting on a (recently)
		 * completed DST cache entry into the datalink layer. They
		 * will be processed by netdev_process_backlog in a later call
		 */
		list_for_each_safe(entry, tmp, &e->packets) {
			nb = list_entry(entry, struct netbuf, bl_entry);
			list_del(entry);
			netbuf_set_dev(nb, dev);

			netdev_unlock(dev);
			dev->tx(nb, e->hwaddr);
			netdev_lock(dev);
		}
	}
}

static void netdev_prepare_xmit(struct netdev *dev, struct netbuf *nb)
{
	size_t offset, tmp;
	size_t netw, transp, app;

	if(unlikely(netbuf_test_and_set_flag(nb, NBUF_IS_LINEAR)))
		return;

	tmp = offset = nb->datalink.size;
	netbuf_realloc(nb, NBAF_DATALINK, nb->size);
	if(nb->network.data) {
		netbuf_cpy_data_offset(nb, offset, nb->network.data, nb->network.size, NBAF_DATALINK);
		offset += nb->network.size;
	}

	if(nb->transport.data) {
		netbuf_cpy_data_offset(nb, offset, nb->transport.data, nb->transport.size, NBAF_DATALINK);
		offset += nb->transport.size;
	}

	if(nb->application.data) {
		netbuf_cpy_data_offset(nb, offset, nb->application.data,
			nb->application.size, NBAF_DATALINK);
	}

	netw = nb->network.size;
	transp = nb->transport.size;
	app = nb->application.size;

	netbuf_free_partial(nb, NBAF_NETWORK);
	netbuf_free_partial(nb, NBAF_TRANSPORT);
	netbuf_free_partial(nb, NBAF_APPLICTION);

	nb->network.data     = (uint8_t*)nb->datalink.data + tmp;
	nb->network.size = netw;

	nb->transport.data   = (uint8_t*)nb->network.data + nb->network.size;
	nb->transport.size = transp;

	nb->application.data = (uint8_t*)nb->transport.data + nb->transport.size;
	nb->application.size = app;

	nb->datalink.size = tmp;

	nb->flags &= ~(NBAF_APPLICTION_MASK | NBAF_TRANSPORT_MASK | NBAF_NETWORK_MASK);
}

static inline void netdev_deliver(struct netdev *dev, struct netbuf *nb)
{
	netdev_unlock(dev);
	dev->rx(nb);
	netdev_lock(dev);
}

static inline int netbuf_done(struct netbuf *nb)
{
	int rc;

	rc = !(nb->flags & ((1 << NBUF_TX_KEEP) | (1 << NBUF_REUSE) | (1 << NBUF_AGAIN)));
	return netbuf_test_flag(nb, NBUF_ARRIVED) && rc;
}

static inline int netbuf_test_and_clear_rx(struct netbuf *nb)
{
	register uint32_t old;

	old = netbuf_test_and_clear_flag(nb, NBUF_RX);
	nb->flags |= old << NBUF_WAS_RX;

	return (int)old;
}

static int netdev_process_backlog(struct netdev *dev, int weight)
{
	struct netbuf *nb;
	struct list_head *entry, *tmp;

	netdev_lock(dev);
	netdev_try_translate_cache(dev);

	if(unlikely(netdev_backlog_empty(dev))) {
		netdev_unlock(dev);
		return -EOK;
	}

	backlog_for_each_safe(&dev->backlog, entry, tmp) {
		if(weight <= 0)
			break;

		nb = list_entry(entry, struct netbuf, bl_entry);
		netdev_remove_backlog_entry(dev, nb);

		if(likely(netbuf_test_and_clear_rx(nb))) {
			netbuf_set_flag(nb, NBUF_IS_LINEAR);
			netbuf_set_dev(nb, dev);
			nb->size = netbuf_calc_size(nb);
			netdev_deliver(dev, nb);

			if(netbuf_dropped(nb))
				netdev_dropped_stats_inc(dev);
			
			if(netbuf_arrived(nb))
				netdev_rx_stats_inc(dev, nb);
		} else {
			nb->size = netbuf_calc_size(nb);
			netdev_prepare_xmit(dev, nb);

			if(likely(dev->write(dev, nb) == -EOK)) {
				netdev_tx_stats_inc(dev, nb);
			} else if(unlikely(netbuf_test_and_clear_flag(nb, NBUF_AGAIN))) {
				__netdev_add_backlog(dev, nb);
				netbuf_clear_flag(nb, NBUF_ARRIVED);
				continue;
			}

			if(netbuf_test_flag(nb, NBUF_TX_KEEP))
				continue;
		}

		weight -= nb->size;
		if(netbuf_done(nb))
			netbuf_free(nb);
		else
			netbuf_clear_flag(nb, NBUF_REUSE);
	}

	netdev_unlock(dev);
	return weight;
}

/**
 * @brief Wake up the core processor thread.
 */
void netdev_wakeup(void)
{
	estack_event_signal(&devcore.event);
}

/**
 * @brief Wake up the core processor thread from an ISR.
 */
void netdev_wakeup_irq(void)
{
	estack_event_signal_irq(&devcore.event);
}

/**
 * @brief Poll a network device.
 * @param dev Network device to poll.
 * @return Number of entries remaining on the backlog of \p dev or an error code.
 *
 * This function will first poll the PHY-layer. If data is available, it will perform
 * a read pushing new packets onto the backlog. Finally the backlog will be processed.
 */
int netdev_poll(struct netdev *dev)
{
	int available, weight;

	assert(dev);
	available = dev->available(dev);

	available = available > dev->rx_max ? dev->rx_max : available;
	if(available > 0)
		dev->read(dev, available);

	weight = dev->processing_weight;
	netdev_lock(dev);
	netdev_try_translate_cache(dev);
	netdev_unlock(dev);

	while(weight > 0 && netdev_backlog_length(dev) != 0) {
		weight = netdev_process_backlog(dev, weight);
	}

	return netdev_backlog_length(dev);
}

/**
 * @brief Poll all available network devices.
 * @return The total number of buffers remaining on the backlog.
 * 
 * Loop through the list of available devices, and keep track of how many entries
 * there are left on the backlog.
 */
int netdev_poll_all(void)
{
	struct list_head *entry;
	int num;
	struct netdev *dev;

	num = 0;
	netdev_lock_core();
	list_for_each(entry, &devcore.devices) {
		dev = list_entry(entry, struct netdev, entry);
		num += netdev_poll(dev);
	}
	netdev_unlock_core();

	return num;
}

/**
 * @brief Poll the networking core asynchronously.
 *
 * Poll all available network devices asynchronously by waking up
 * the core processing thread.
 */
void netdev_poll_async(void)
{
	struct list_head *entry;
	struct netdev *dev;

	netdev_lock_core();
	list_for_each(entry, &devcore.devices) {
		dev = list_entry(entry, struct netdev, entry);
		if(dev->available(dev)) {
			netdev_wakeup();
			break;
		}
	}

	netdev_unlock_core();
}

#ifndef CONFIG_POLL_TMO
#define CONFIG_POLL_TMO 100
#endif

static void netdev_poll_task(void *arg)
{
	int remaining;

	UNUSED(arg);
	/* start in a sleeping state */
	remaining = 0;

	while(true) {
		/*
		 * Roll through all devices as long as
		 * the device core is running. If there are no
		 * packets remaining on the backlog the processor
		 * will be suspended on the device core event queue.
		 */
		if(!remaining)
			estack_event_wait(&devcore.event, CONFIG_POLL_TMO);
		else
			estack_sleep(CONFIG_POLL_TMO);

		netdev_lock_core();
		if(unlikely(!devcore.running)) {
			netdev_unlock_core();
			break;
		}
		netdev_unlock_core();

		remaining = netdev_poll_all();
	}
}

/**
 * @brief Configure various network device parameters.
 * @param dev Network device to configure.
 * @param maxrx Maximum number of packets to receive at once.
 * @param maxweight Maximum number of bytes to process in a single pass.
 */
void netdev_config_params(struct netdev *dev, int maxrx, int maxweight)
{
	netdev_lock_core();
	netdev_lock(dev);
	dev->rx_max = maxrx;
	dev->processing_weight = maxweight;
	netdev_unlock(dev);
	netdev_unlock_core();
}

static bool __netdev_demux_handle(struct netbuf *nb)
{
	struct netdev *dev;
	struct list_head *entry;
	struct protocol *proto;
	bool rv;

	dev = nb->dev;
	rv = false;

	netdev_lock(dev);
	list_for_each(entry, &dev->protocols) {
		proto = list_entry(entry, struct protocol, entry);
		if(unlikely(proto->protocol == nb->protocol)) {
			netdev_unlock(dev);
			proto->rx(nb);
			netdev_lock(dev);
			rv = true;
		}
	}

	netdev_unlock(dev);

	return rv;
}

/**
 * @brief Configure various core parameters.
 * @param retry_tmo Time out (in us) between attempts to resolve a cache entry.
 * @param resolv_tmo Time out for packets on an unresolved cache.
 * @param retries Number of resolv attempts before the cache entry is deemed unresolvable.
 * @note If resolve timeout expires, the cache is deemed unrsolvable and all packets assosiated with
 *       said packets will be dropped.
 */
void netdev_config_core_params(uint32_t retry_tmo, uint32_t resolv_tmo, int retries)
{
	dst_retries = retries;
	dst_resolve_tmo = resolv_tmo;
	dst_retry_tmo = retry_tmo;
}

/**
 * @brief Call external handler for the current protocol of \p nb.
 * @param nb Packet buffer to call external handlers for.
 *
 * The external handlers will be selected based on the value of `struct netbuf::protocol`.
 */
bool netdev_demux_handle(struct netbuf *nb)
{
	assert(nb);
	assert(nb->dev);

	if(likely(nb->protocol != 0))
		return __netdev_demux_handle(nb);
	
	return false;
}

static inline struct netdev_stats *netdev_get_stats(struct netdev *dev)
{
	return &dev->stats;
}

/**
 * @brief Get the number of packets dropped by \p dev.
 * @param dev Device to get stats for.
 * @return The number of dropped packets.
 */
uint32_t netdev_get_dropped(struct netdev *dev)
{
	struct netdev_stats *stats;
	uint32_t dropped;

	assert(dev);
	netdev_lock(dev);
	stats = netdev_get_stats(dev);
	dropped = stats->dropped;
	netdev_unlock(dev);

	return dropped;
}

/**
 * @brief Get the number of received packets.
 * @param dev Device to get stats for.
 * @return Number of received bytes.
 */
uint32_t netdev_get_rx_bytes(struct netdev *dev)
{
	struct netdev_stats *stats;
	uint32_t bytes;

	assert(dev);

	netdev_lock(dev);
	stats = netdev_get_stats(dev);
	bytes = stats->rx_bytes;
	netdev_unlock(dev);

	return bytes;
}

/**
 * @brief Get the number of transmitted bytes.
 * @param dev Device to get stats for.
 * @return Number of transmitted bytes.
 */
uint32_t netdev_get_tx_bytes(struct netdev *dev)
{
	struct netdev_stats *stats;
	uint32_t bytes;

	assert(dev);

	netdev_lock(dev);
	stats = netdev_get_stats(dev);
	bytes = stats->tx_bytes;
	netdev_unlock(dev);

	return bytes;
}

/**
 * @brief Get the number of received packets.
 * @param dev Device to get stats for.
 * @return The number of received packets.
 */
uint32_t netdev_get_rx_packets(struct netdev *dev)
{
	struct netdev_stats *stats;
	uint32_t packets;

	assert(dev);

	netdev_lock(dev);
	stats = netdev_get_stats(dev);
	packets = stats->rx_packets;
	netdev_unlock(dev);

	return packets;
}

/**
 * @brief Get the number of transmitted packets.
 * @param dev Device to get stats for.
 * @return The number of transmitted packets.
 */
uint32_t netdev_get_tx_packets(struct netdev *dev)
{
	struct netdev_stats *stats;
	uint32_t packets;

	assert(dev);

	netdev_lock(dev);
	stats = netdev_get_stats(dev);
	packets = stats->tx_packets;
	netdev_unlock(dev);

	return packets;
}

/**
 * @brief Write device statistics to a file.
 * @param dev Device to get stats from.
 * @param file File handle to write statistics to.
 */
void netdev_write_stats(struct netdev *dev, FILE *file)
{
	struct netdev_stats *stats;

	assert(file);
	assert(dev);

	netdev_lock(dev);
	stats = netdev_get_stats(dev);

	fprintf(file, "Stats for: %s\n", dev->name);
	fprintf(file, "\tReceived: %lu bytes in %lu packets\n", (unsigned long)stats->rx_bytes,
			(unsigned long)stats->rx_packets);
	fprintf(file, "\tTransmit: %lu bytes in %lu packets\n", (unsigned long)stats->tx_bytes,
			(unsigned long)stats->tx_packets);
	fprintf(file, "\t%lu packets have been dropped\n", (unsigned long)stats->dropped);
	fprintf(file, "\tBacklog size %u\n", (unsigned int)dev->backlog.size);
	netdev_unlock(dev);
}

#ifdef HAVE_DEBUG
/**
 * @brief Print informatation known about a network device.
 * @param dev Network device to print.
 * @param file File to write to.
 */
void netdev_print(struct netdev *dev, FILE *file)
{
	char hwbuf[18];
	char ipbuf[16];
	struct netif *nif = &dev->nif;

	netdev_lock(dev);
	fprintf(file, "Info for %s:\n", dev->name);
	ethernet_mac_ntoa(dev->hwaddr, hwbuf, 18);
	fprintf(file, "\tHardware address %s\n", hwbuf);
	ipv4_ntoa(ipv4_ptoi(nif->local_ip), ipbuf, 16);
	fprintf(file, "\tLocal IP: %s\n", ipbuf);
	ipv4_ntoa(ipv4_ptoi(nif->ip_mask), ipbuf, 16);
	fprintf(file, "\tIP mask: %s\n", ipbuf);
	netdev_unlock(dev);

	netdev_write_stats(dev, file);
}
#else
void netdev_print(struct netdev *dev, FILE *file)
{
}
#endif

#ifndef CONFIG_CORE_EVENT_LENGTH
#define CONFIG_CORE_EVENT_LENGTH 4
#endif

/**
 * @brief	Initialize a network device.
 * @param	dev	Device to initialise.
 */
void netdev_init(struct netdev *dev)
{
	list_head_init(&dev->entry);
	list_head_init(&dev->backlog.head);
	list_head_init(&dev->protocols);
	list_head_init(&dev->destinations);
	estack_mutex_create(&dev->mtx, 0);

	dev->backlog.size = 0;
	dev->processing_weight = 15000;
	dev->rx_max = 10;

	netdev_lock_core();
	netdev_lock(dev);
	list_add(&dev->entry, &devcore.devices);
	netdev_unlock(dev);
	netdev_unlock_core();
}

/**
 * @brief Network device destructor.
 * @param dev Network device to destroy.
 * @note All memory associated with \p dev will be destroyed.
 */
void netdev_destroy(struct netdev *dev)
{
	struct list_head *entry, *tmp;
	struct dst_cache_entry *e;
	struct netbuf *nb;
	struct protocol *proto;

	assert(dev);

	netdev_lock_core();
	netdev_lock(dev);

	list_for_each_safe(entry, tmp, &dev->destinations) {
		e = list_entry(entry, struct dst_cache_entry, entry);
		list_del(entry);
		netdev_drop_dst(e);
		netdev_free_dst_entry(e);
	}

	backlog_for_each_safe(&dev->backlog, entry, tmp) {
		nb = list_entry(entry, struct netbuf, bl_entry);
		list_del(entry);
		netbuf_free(nb);
	}

	list_for_each_safe(entry, tmp, &dev->protocols) {
		proto = list_entry(entry, struct protocol, entry);
		list_del(entry);
		free(proto);
	}

	list_del(&dev->entry);
	netdev_unlock(dev);
	netdev_unlock_core();

	estack_mutex_destroy(&dev->mtx);
}

/**
 * @brief Initialise the network device core.
 *
 * Initialise the core parameters for the network device core / handler.
 */
void devcore_init(void)
{
	devcore.runner.name = "polltsk";
	devcore.running = true;

	estack_mutex_create(&devcore.mtx, 0);
	estack_event_create(&devcore.event, CONFIG_CORE_EVENT_LENGTH);
	estack_thread_create(&devcore.runner, netdev_poll_task, NULL);
}

/**
 * @brief Destroy the network core.
 *
 * The device core runner task will be terminated by this function.
 */
void devcore_destroy(void)
{
	netdev_lock_core();
	devcore.running = false;
	netdev_unlock_core();

	estack_event_signal(&devcore.event);
	estack_thread_destroy(&devcore.runner);
	estack_mutex_destroy(&devcore.mtx);
	estack_event_destroy(&devcore.event);
}

/** @} */
