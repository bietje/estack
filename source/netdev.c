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

static struct list_head devices = STATIC_INIT_LIST_HEAD(devices);

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
