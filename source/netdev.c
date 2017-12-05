/*
 * E/STACK network device's
 *
 * Author: Michel Megens
 * Date: 03/12/2017
 * Email: dev@bietje.net
 */

#include <stdlib.h>
#include <stdint.h>

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

void netdev_init(struct netdev *dev)
{
	list_head_init(&dev->entry);
	list_head_init(&dev->backlog.head);
	list_head_init(&dev->protocols);
	dev->backlog.size = 0;
}

void netdev_add_backlog(struct netdev *dev, struct netbuf *nb)
{
	assert(dev);
	assert(nb);

	list_add_tail(&nb->bl_entry, &dev->backlog.head);
	dev->backlog.size += 1;
}

static inline void netdev_remove_backlog_entry(struct netdev *dev, struct netbuf *nb)
{
	list_del(&nb->bl_entry);
	dev->backlog.size -= 1;
}

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

void netdev_add_protocol(struct netdev *dev, struct protocol *proto)
{
	list_head_init(&proto->entry);
	list_add_tail(&proto->entry, &dev->protocols);
}

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

void netdev_add_destination(const uint8_t *dst, uint8_t daddrlen , const uint8_t *src, uint8_t saddrlen)
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
	list_add(&centry->entry, &dst_cache);
}

bool netdev_update_destination(const uint8_t *dst, uint8_t dlength, const uint8_t *src, uint8_t slength)
{
	struct list_head *entry;
	struct dst_cache_entry *centry;

	list_for_each(entry, &dst_cache) {
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

bool netdev_remove_destination(const uint8_t *src, uint8_t length)
{
	struct list_head *entry;
	struct dst_cache_entry *centry;

	list_for_each(entry, &dst_cache)
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

struct dst_cache_entry *netdev_find_destination(const uint8_t *src, uint8_t length)
{
	struct list_head *entry;
	struct dst_cache_entry *centry;

	list_for_each(entry, &dst_cache)
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

void netdev_demux_handle(struct netbuf *nb)
{
	assert(nb);
	
	if (nb->protocol != 0)
		__netdev_demux_handle(nb);
}
