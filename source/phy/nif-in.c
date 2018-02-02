/*
 * E/STACK - Network interface input
 *
 * Author: Michel Megens
 * Date:   02/02/2018
 * Email:  dev@bietje.net
 */

#include <stdlib.h>
#include <stdint.h>

#include <estack/estack.h>
#include <estack/netdev.h>
#include <estack/netbuf.h>

void netif_input(struct netdev *dev, const void *data, size_t length, int protocol)
{
	struct netbuf *nb;

	assert(data);
	assert(length);
	assert(protocol);

	nb = netbuf_alloc(NBAF_DATALINK, length);
	assert(nb);

	nb->protocol = protocol;
	netbuf_cpy_data(nb, data, length, NBAF_DATALINK);
	netbuf_set_flag(nb, NBUF_RX);
	netdev_add_backlog(dev, nb);
	netdev_wakeup();
}
