/*
 * E/STACK network interface
 *
 * Author: Michel Megens
 * Date: 06/12/2017
 * Email: dev@bietje.net
 */

#include <stdlib.h>
#include <stdint.h>

#include <estack/estack.h>
#include <estack/arp.h>
#include <estack/netdev.h>
#include <estack/translate.h>

#define IP4_ADDR_LENGTH 4
struct dst_cache_entry *arp_resolve_ipv4(struct netdev *dev, uint32_t ip)
{
	void *addr;
	struct dst_cache_entry *e;

	assert(dev);
	assert(ip);

	addr = (void*)&ip;
	e = netdev_find_destination(dev, addr, IP4_ADDR_LENGTH);

	if (e)
		return e;

	e = netdev_add_destination_unresolved(dev, addr, IP4_ADDR_LENGTH, translate_ipv4_to_mac);
	return e;
}

