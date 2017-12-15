/*
 * E/STACK network interface
 *
 * Author: Michel Megens
 * Date: 06/12/2017
 * Email: dev@bietje.net
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <estack/estack.h>
#include <estack/netbuf.h>
#include <estack/ethernet.h>
#include <estack/netdev.h>
#include <estack/error.h>
#include <estack/inet.h>
#include <estack/log.h>

void ifconfig(struct netdev *dev, uint8_t *local, uint8_t *remote,
	uint8_t *mask, uint8_t length, nif_type_t type)
{
	struct netif *nif;

	assert(dev);
	nif = &dev->nif;

	memcpy(nif->local_ip, local, length);
	memcpy(nif->remote_ip, remote, length);
	memcpy(nif->ip_mask, mask, length);
	nif->iftype = type;
}

uint16_t netif_get_id(struct netif *nif)
{
	uint16_t rv;

	rv = nif->pkt_id;
	nif->pkt_id++;
	return rv;
}

#ifdef HAVE_DEBUG
void netdev_print_nif(struct netdev *dev)
{
	struct netif *nif;
	char buf[16];

	assert(dev);
	nif = &dev->nif;

	if(nif->iftype == NIF_TYPE_ETHER) {
		print_dbg("Network interface: %s\n", dev->name);
		ipv4_ntoa(ipv4_ptoi(nif->local_ip), buf, 16);
		print_dbg("\tSource IP %s\n", buf);
		ipv4_ntoa(ipv4_ptoi(nif->ip_mask), buf, 16);
		print_dbg("\tIP mask %s\n", buf);
		return;
	}
}
#else
void netdev_print_nif(struct netdev *dev)
{
}
#endif
