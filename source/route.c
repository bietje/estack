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

static inline struct iproute_head *route4_get_head(void)
{
	return &ip4_head;
}

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

	list_add_tail(&entry->entry, &ip4_head.head);
	return false;
}

void route4_clear(void)
{
	struct list_head *entry, *tmp;
	struct iproute4_entry *e;
	struct iproute_head *head;

	head = route4_get_head();
	list_for_each_safe(entry, tmp, &head->head) {
		e = list_entry(entry, struct iproute4_entry, entry);
		list_del(entry);
		free(e);
	}
}

bool route4_delete(uint32_t ip, uint32_t mask, uint32_t gate, struct netdev *dev)
{
	struct iproute4_entry *entry;
	struct list_head *e, *tmp;
	struct iproute_head *head;
	bool rv = false;

	head = route4_get_head();
	list_for_each_safe(e, tmp, &head->head) {
		entry = list_entry(e, struct iproute4_entry, entry);

		if(entry->ip == ip && entry->mask == mask &&
				entry->gateway == gate && entry->dev == dev) {
			list_del(e);
			free(entry);
			rv = true;
		}
	}

	return rv;
}

#define LOOKUP_MAX_LEVEL 4
static struct iproute4_entry *__route4_lookup(uint32_t ip, uint32_t *gw, uint8_t level)
{
	struct iproute4_entry *entry;
	struct list_head *e;
	struct iproute_head *head;

	entry = NULL;

	if(level >= LOOKUP_MAX_LEVEL)
		return entry;

	head = route4_get_head();
	list_for_each(e, &head->head) {
		entry = list_entry(e, struct iproute4_entry, entry);

		if((ip & entry->mask) == entry->ip) {
			if(gw && entry->gateway)
				*gw = entry->gateway;

			if(!entry->dev)
				entry = __route4_lookup(ip, gw, level + 1);

			break;
		}
	}

	return entry;
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