/*
 * E/STACK - IP routing
 *
 * Author: Michel Megens
 * Date: 13/12/2017
 * Email: dev@bietje.net
 */

#include <stdlib.h>
#include <stdint.h>

#include <estack/estack.h>
#include <estack/list.h>
#include <estack/ip.h>
#include <estack/netdev.h>
#include <estack/netbuf.h>
#include <estack/route.h>

static struct iproute_head ip4_head = {
	.head = STATIC_INIT_LIST_HEAD(ip4_head.head),
	.version = 4,
};

static struct iproute4_entry *__route4_search(uint32_t addr)
{
	struct list_head *entry;
	struct iproute4_entry *e;

	list_for_each(entry, &ip4_head.head) {
		e = container_of(entry, struct iproute4_entry, entry);

		if ((addr & e->mask) == e->ip)
			return e;
	}

	return NULL;
}

/* IPv4 routing */
bool route4_add(uint32_t addr, uint32_t mask, uint32_t gw, struct netdev *dev)
{
	struct iproute4_entry *entry;

	assert(dev);

	if (__route4_search(addr))
		return false;

	entry = malloc(sizeof(*entry));
	assert(entry);

	entry->dev = dev;
	entry->gateway = gw;
	entry->ip = addr;
	entry->mask = mask;

	list_add(&entry->entry, &ip4_head.head);
	return false;
}

#define LOOKUP_MAX_LEVEL 4
static struct iproute4_entry *__route4_lookup(uint32_t ip, uint32_t *gw, uint8_t level)
{
	return NULL;
}

struct netdev *route4_lookup(uint32_t ip, uint32_t *gw)
{
	struct iproute4_entry *entry;

	if (gw)
		*gw = 0;

	if ((ip == INADDR_BCAST) || IS_MULTICAST(ip)) {
		if (list_empty(&ip4_head.head))
			return NULL;

		entry = list_first_entry(&ip4_head.head, struct iproute4_entry, entry);		
	} else {
		entry = __route4_lookup(ip, gw, 0);
	}

	return entry ? entry->dev : NULL;
}
