/*
 * E/Stack network device's
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

static struct list_head devices = STATIC_INIT_LIST_HEAD(devices);
static int netdev_processing_weight = 15000;
static int netdev_rx_max = 10;

void netdev_init(struct netdev *dev)
{
	list_head_init(&dev->entry);
	list_head_init(&dev->backlog.head);
	dev->backlog.size = 0;

	list_add(&dev->entry, &devices);
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

static inline void netdev_dropped_stats_inc(struct netdev *dev, struct netbuf *nb)
{
	struct netdev_stats *stats;

	stats = &dev->stats;
	stats->dropped++;
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
			if(!netbuf_test_flag(nb, NBUF_ARRIVED))
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
			netdev_dropped_stats_inc(dev, nb);
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
