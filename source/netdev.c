/*
 * E/STACK network device's
 *
 * Author: Michel Megens
 * Date: 03/12/2017
 * Email: dev@bietje.net
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

static struct list_head dst_cache = STATIC_INIT_LIST_HEAD(dst_cache);
static struct list_head devices = STATIC_INIT_LIST_HEAD(devices);

static uint32_t dst_resolve_tmo = 4500000;
static uint32_t dst_retry_tmo = 1000000;
static int dst_retries = 4;

/**
 * @brief Search the global device list.
 * @param name Device name to search for.
 * @return A device with name \p name or \p NULL.
 */
struct netdev *netdev_find(const char *name)
{
	struct list_head *entry;
	struct netdev *dev;

	list_for_each(entry, &devices) {
		dev = list_entry(entry, struct netdev, entry);
		if(!strcmp(dev->name, name))
			return dev;
	}

	return NULL;
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

	list_del(&dev->entry);
	return dev;
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

	list_add_tail(&nb->bl_entry, &dev->backlog.head);
	dev->backlog.size += 1;
}

/**
 * @brief Remove a backlog entry.
 * @param dev Device to remove \p nb from.
 * @param nb Packet buffer to remove.
 */
static inline void netdev_remove_backlog_entry(struct netdev *dev, struct netbuf *nb)
{
	list_del(&nb->bl_entry);
	dev->backlog.size -= 1;
}

/**
 * @brief Getter for the backlog length.
 * @param dev Device to get the backlog length for.
 * @return The length of the backlog for \p dev.
 */
int netdev_backlog_length(struct netdev *dev)
{
	assert(dev);
	return dev->backlog.size;
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

	list_for_each(entry, &dev->protocols) {
		p = list_entry(entry, struct protocol, entry);
		if(p->protocol == proto)
			return p;
	}

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
	list_add_tail(&p->entry, &dev->protocols);

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
		list_del(&p->entry);
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
	list_add(&centry->entry, &dev->destinations);
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
	list_add(&centry->entry, &dev->destinations);

	return centry;
}

/**
 * @brief Add a packet buffer to a destination cache entry.
 * @param e Destination cache entry to add \p nb to.
 * @param nb Packet buff to add to \p e.
 * @return True or false based on whether the packet buffer was added or not.
 */
bool netdev_dstcache_add_packet(struct dst_cache_entry *e, struct netbuf *nb)
{
	assert(e);
	assert(nb);

	if(e->state != DST_UNFINISHED)
		return false;

	assert(nb->dev);
	e->timeout = estack_utime() + dst_resolve_tmo;
	list_add(&nb->bl_entry, &e->packets);

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

	list_for_each(entry, &dev->destinations) {
		centry = list_entry(entry, struct dst_cache_entry, entry);
		if(centry->saddr_length != slength)
			continue;

		if(!memcmp(centry->saddr, src, slength)) {
			if(dlength != centry->hwaddr_length)
				centry->hwaddr = realloc(centry->hwaddr, dlength);

			memcpy(centry->hwaddr, dst, dlength);
			centry->state = DST_RESOLVED;
			return true;
		}
	}

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

	list_for_each(entry, &dev->destinations) {
		centry = list_entry(entry, struct dst_cache_entry, entry);
		if(centry->saddr_length != length)
			continue;

		if(!memcmp(centry->saddr, src, length)) {
			list_del(entry);
			netdev_free_dst_entry(centry);
			return true;
		}
	}

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

	list_for_each(entry, &dev->destinations) {
		centry = list_entry(entry, struct dst_cache_entry, entry);
		if(centry->saddr_length != length)
			continue;

		if(!memcmp(centry->saddr, src, length))
			return centry;
	}

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
			if(netdev_dst_timeout(e) || !e->translate ||
				e->retry <= 0 || list_empty(&e->packets)) {
				netdev_drop_dst(e);
				list_del(dst);
				netdev_free_dst_entry(e);
				continue;
			}

			if(e->last_attempt + dst_retry_tmo < estack_utime()) {
				e->translate(dev, e->saddr);
				e->retry--;
				e->last_attempt = estack_utime();
			}
			continue;
		}

		list_for_each_safe(entry, tmp, &e->packets) {
			nb = list_entry(entry, struct netbuf, bl_entry);
			list_del(entry);
			netbuf_set_dev(nb, dev);
			dev->tx(nb, e->hwaddr);
		}
	}
}

static int netdev_process_backlog(struct netdev *dev, int weight)
{
	struct netbuf *nb;
	struct list_head *entry,
		*tmp;
	int rc, arrived;

	if(list_empty(&dev->backlog.head))
		return -EOK;

	backlog_for_each_safe(&dev->backlog, entry, tmp) {
		nb = list_entry(entry, struct netbuf, bl_entry);
		netdev_remove_backlog_entry(dev, nb);

		if(weight < 0)
			break;

		if(likely(netbuf_test_flag(nb, NBUF_RX))) {
			/*
			 * Push arriving packets into the network stack through the
			 * receive handle: struct netdev:rx().
			 */
			netbuf_set_dev(nb, dev);
			netbuf_set_timestamp(nb);
			nb->protocol = ntohs(nb->protocol);
			nb->size = netbuf_get_size(nb);

			dev->rx(nb);
			if((arrived = netbuf_test_flag(nb, NBUF_ARRIVED)) != 0)
				netdev_rx_stats_inc(dev, nb);
		} else {
			nb->size = netbuf_calc_size(nb);
			rc = dev->write(dev, nb);
			arrived = !rc;

			if(!rc)
				netdev_tx_stats_inc(dev, nb);
		}

		if(netbuf_test_flag(nb, NBUF_AGAIN)) {
			netdev_add_backlog(dev, nb);
		} else if(netbuf_test_flag(nb, NBUF_DROPPED)) {
			netdev_dropped_stats_inc(dev);
			netbuf_free(nb);
			continue;
		}

		if(arrived) {
			weight -= nb->size;
			netbuf_free(nb);
		}
	}

	/* Attempt to resolve any unresolved destination cache entries. */
	netdev_try_translate_cache(dev);
	return weight;
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
	netdev_try_translate_cache(dev);
	while(weight > 0 && netdev_backlog_length(dev) != 0) {
		weight = netdev_process_backlog(dev, weight);
	}

	return netdev_backlog_length(dev);
}

static void __netdev_demux_handle(struct netbuf *nb)
{
	struct netdev *dev;
	struct list_head *entry;
	struct protocol *proto;

	dev = nb->dev;
	list_for_each(entry, &dev->protocols) {
		proto = list_entry(entry, struct protocol, entry);
		if(proto->protocol == nb->protocol)
			proto->rx(nb);
	}
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
void netdev_demux_handle(struct netbuf *nb)
{
	assert(nb);

	if(nb->protocol != 0)
		__netdev_demux_handle(nb);
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

	assert(dev);
	stats = netdev_get_stats(dev);
	return stats->dropped;
}

/**
 * @brief Get the number of received packets.
 * @param dev Device to get stats for.
 * @return Number of received bytes.
 */
uint32_t netdev_get_rx_bytes(struct netdev *dev)
{
	struct netdev_stats *stats;

	assert(dev);
	stats = netdev_get_stats(dev);
	return stats->rx_bytes;
}

/**
 * @brief Get the number of transmitted bytes.
 * @param dev Device to get stats for.
 * @return Number of transmitted bytes.
 */
uint32_t netdev_get_tx_bytes(struct netdev *dev)
{
	struct netdev_stats *stats;

	assert(dev);
	stats = netdev_get_stats(dev);
	return stats->tx_bytes;
}

/**
 * @brief Get the number of received packets.
 * @param dev Device to get stats for.
 * @return The number of received packets.
 */
uint32_t netdev_get_rx_packets(struct netdev *dev)
{
	struct netdev_stats *stats;

	assert(dev);
	stats = netdev_get_stats(dev);
	return stats->rx_packets;
}

/**
 * @brief Get the number of transmitted packets.
 * @param dev Device to get stats for.
 * @return The number of transmitted packets.
 */
uint32_t netdev_get_tx_packets(struct netdev *dev)
{
	struct netdev_stats *stats;

	assert(dev);
	stats = netdev_get_stats(dev);
	return stats->tx_packets;
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
	stats = netdev_get_stats(dev);

	fprintf(file, "Stats for: %s\n", dev->name);
	fprintf(file, "\tReceived: %lu bytes in %lu packets\n", (unsigned long)stats->rx_bytes, (unsigned long)stats->rx_packets);
	fprintf(file, "\tTransmit: %lu bytes in %lu packets\n", (unsigned long)stats->tx_bytes, (unsigned long)stats->tx_packets);
	fprintf(file, "\t%lu packets have been dropped\n", (unsigned long)stats->dropped);
	fprintf(file, "\tBacklog size %u\n", (unsigned int)dev->backlog.size);
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

	fprintf(file, "Info for %s:\n", dev->name);
	ethernet_mac_ntoa(dev->hwaddr, hwbuf, 18);
	fprintf(file, "\tHardware address %s\n", hwbuf);
	ipv4_ntoa(ipv4_ptoi(nif->local_ip), ipbuf, 16);
	fprintf(file, "\tLocal IP: %s\n", ipbuf);
	ipv4_ntoa(ipv4_ptoi(nif->ip_mask), ipbuf, 16);
	fprintf(file, "\tIP mask: %s\n", ipbuf);

	netdev_write_stats(dev, file);
}
#else
void netdev_print(struct netdev *dev, FILE *file)
{
}
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

	dev->backlog.size = 0;
	dev->processing_weight = 15000;
	dev->rx_max = 10;
	list_add(&dev->entry, &devices);
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
	list_for_each_safe(entry, tmp, &dev->destinations) {
		e = list_entry(entry, struct dst_cache_entry, entry);
		netdev_drop_dst(e);
		netdev_free_dst_entry(e);
	}

	backlog_for_each_safe(&dev->backlog, entry, tmp) {
		nb = list_entry(entry, struct netbuf, bl_entry);
		netbuf_free(nb);
	}

	list_for_each_safe(entry, tmp, &dev->protocols) {
		proto = list_entry(entry, struct protocol, entry);
		free(proto);
	}
}

/** @} */
