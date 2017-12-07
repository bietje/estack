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
		if (!strcmp(dev->name, name))
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
	if (!dev)
		return NULL;

	list_del(&dev->entry);
	return dev;
}

/**
 * @brief Add a packet buffer to the backlog of \p dev.
 * @param dev Device to add \p nb to.
 * @param nb Packet buffer to add.
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

/**
 * @brief Add a protocol handler.
 * @param dev Device to add \p proto to.
 * @param proto Protocol handler to add to \p dev.
 */
void netdev_add_protocol(struct netdev *dev, struct protocol *proto)
{
	list_head_init(&proto->entry);
	list_add_tail(&proto->entry, &dev->protocols);
}

/**
 * @brief Remove a protocol from \p dev.
 * @param dev Device to remove \p proto from.
 * @param proto Protocol to remove.
 * @return True of false based on whether the protocol was removed or not.
 */
bool netdev_remove_protocol(struct netdev *dev, struct protocol *proto)
{
	struct list_head *entry;
	struct protocol *pentry;

	list_for_each(entry, &dev->protocols) {
		pentry = list_entry(entry, struct protocol, entry);

		if (proto == pentry) {
			list_del(entry);
			return true;
		}
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
void netdev_add_destination(struct netdev *dev, const uint8_t *dst, uint8_t daddrlen ,
	                        const uint8_t *src, uint8_t saddrlen)
{
	struct dst_cache_entry *centry;

	centry = z_alloc(sizeof(*centry));
	assert(centry);

	centry->saddr_length = saddrlen;
	centry->hwaddr_length = daddrlen;
	
	centry->saddr = malloc(saddrlen);
	memcpy(centry->saddr, src, saddrlen);

	centry->hwaddr = malloc(daddrlen);
	memcpy(centry->hwaddr, dst, daddrlen);

	list_head_init(&centry->entry);
	list_add(&centry->entry, &dev->destinations);
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
		if (!memcmp(centry->saddr, src, slength)) {
			if (dlength != centry->hwaddr_length)
				centry->hwaddr = realloc(centry->hwaddr, dlength);

			memcpy(centry->hwaddr, dst, dlength);
			return true;
		}
	}

	return false;
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

	list_for_each(entry, &dev->destinations)
	{
		centry = list_entry(entry, struct dst_cache_entry, entry);
		if (!memcmp(centry->saddr, src, length)) {
			list_del(entry);
			free(centry->hwaddr);
			free(centry->saddr);
			free(centry);
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

	list_for_each(entry, &dev->destinations)
	{
		centry = list_entry(entry, struct dst_cache_entry, entry);
		if (!memcmp(centry->saddr, src, length))
			return centry;
	}

	return NULL;
}

static int netdev_process_backlog(struct netdev *dev, int weight)
{
	struct netbuf *nb;
	struct list_head *entry,
		             *tmp;
	int rc, arrived;

	if (list_empty(&dev->backlog.head))
		return -EOK;

	backlog_for_each_safe(&dev->backlog, entry, tmp) {
		nb = list_entry(entry, struct netbuf, bl_entry);
		netdev_remove_backlog_entry(dev, nb);

		if (weight < 0)
			break;

		if (likely(netbuf_test_flag(nb, NBUF_RX))) {
			netbuf_set_dev(nb, dev);
			netbuf_set_timestamp(nb);
			nb->protocol = ntohs(nb->protocol);
			nb->size = netbuf_get_size(nb);

			dev->rx(nb);
			if ((arrived = netbuf_test_flag(nb, NBUF_ARRIVED)) != 0)
				netdev_rx_stats_inc(dev, nb);
		} else {
			if (!nb->size)
				nb->size = netbuf_get_size(nb);

			rc = dev->write(dev, nb);
			arrived = !rc;

			if (!rc)
				netdev_tx_stats_inc(dev, nb);
		}

		if (netbuf_test_flag(nb, NBUF_AGAIN)) {
			netdev_add_backlog(dev, nb);
		} else if (netbuf_test_flag(nb, NBUF_DROPPED)) {
			netdev_dropped_stats_inc(dev);
			netbuf_free(nb);
			continue;
		}

		if (arrived) {
			weight -= nb->size;
			netbuf_free(nb);
		}
	}

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
	if (available > 0)
		dev->read(dev, available);

	weight = dev->processing_weight;
	while (weight > 0 && netdev_backlog_length(dev) != 0) {
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
		if (proto->protocol == nb->protocol)
			proto->rx(nb);
	}
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
	
	if (nb->protocol != 0)
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
	fprintf(file, "\tReceived: %u bytes in %u packets\n", stats->rx_bytes, stats->rx_packets);
	fprintf(file, "\tTransmit: %u bytes in %u packets\n", stats->tx_bytes, stats->tx_packets);
	fprintf(file, "\t%u packets have been dropped\n", stats->dropped);
	fprintf(file, "\tBacklog size %u\n", dev->backlog.size);
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
	ipv4_ntoa(ipv4atoi(nif->local_ip), ipbuf, 16);
	fprintf(file, "\tLocal IP: %s\n", ipbuf);
	ipv4_ntoa(ipv4atoi(nif->ip_mask), ipbuf, 16);
	fprintf(file, "\tLocal IP: %s\n", ipbuf);

	netdev_write_stats(dev, file);
}
#else
void netdev_print(FILE *file)
{
}
#endif

/** @} */
