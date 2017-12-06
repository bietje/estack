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

static struct list_head dst_cache = STATIC_INIT_LIST_HEAD(dst_cache);
static int netdev_processing_weight = 15000;
static int netdev_rx_max = 10;

 /**
  * @brief	Initialize a network device.
  *
  * @param	dev	Device to initialise.
  */

void netdev_init(struct netdev *dev)
{
	list_head_init(&dev->entry);
	list_head_init(&dev->backlog.head);
	list_head_init(&dev->protocols);
	list_head_init(&dev->destinations);
	dev->backlog.size = 0;
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
	int rc;

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
			if(netbuf_test_flag(nb, NBUF_ARRIVED))
				netdev_rx_stats_inc(dev, nb);
		} else {
			if (!nb->size)
				nb->size = netbuf_get_size(nb);

			rc = dev->write(dev, nb);

			if (!rc) {
				netdev_tx_stats_inc(dev, nb);
				weight -= nb->size;
				netbuf_free(nb);
				continue;
			}
		}

		if (netbuf_test_flag(nb, NBUF_AGAIN)) {
			netdev_add_backlog(dev, nb);
		} else if (netbuf_test_flag(nb, NBUF_DROPPED)) {
			netdev_dropped_stats_inc(dev);
			netbuf_free(nb);
			continue;
		}

		weight -= nb->size;
	}

	return netdev_backlog_length(dev);
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
	int available;

	assert(dev);
	available = dev->available(dev);

	if (available > 0)
		dev->read(dev, netdev_rx_max);

	return netdev_process_backlog(dev, netdev_processing_weight);
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

/** @} */
