/*
 * E/STACK network to datalink layer translation
 *
 * Author: Michel Megens
 * Date: 11/12/2017
 * Email: dev@bietje.net
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <estack/estack.h>
#include <estack/netdev.h>
#include <estack/netbuf.h>
#include <estack/ethernet.h>
#include <estack/inet.h>

bool neighbour_output(struct netdev *dev, struct netbuf *nb, void *addr, uint8_t length, resolve_handle handle)
{
	struct dst_cache_entry *e;

	assert(dev);
	assert(nb);
	assert(addr);
	assert(length);

	netbuf_set_dev(nb, dev);
	e = netdev_find_destination(dev, addr, length);

	if(likely(e)) {
		if(e->state == DST_RESOLVED) {
			dev->tx(nb, e->hwaddr);
			return true;
		}

		/* Entry isn't resolved yet, enqueue it */
	} else {
		e = netdev_add_destination_unresolved(dev, addr, length, handle);
	}

	netdev_dstcache_add_packet(e, nb);
	return false;
}
