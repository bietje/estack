/*
 * E/STACK - IP input handler
 *
 * Author: Michel Megens
 * Date: 12/12/2017
 * Email: dev@bietje.net
 */

#include <stdlib.h>
#include <stdio.h>

#include <estack/estack.h>
#include <estack/netbuf.h>
#include <estack/netdev.h>
#include <estack/ip.h>
#include <estack/log.h>
#include <estack/inet.h>
#include <estack/route.h>
#include <estack/neighbour.h>
#include <estack/translate.h>
#include <estack/ethernet.h>

#include <config.h>

#define IPV4_VERSION 0x4

void ipv4_output(struct netbuf *nb, uint32_t dst)
{
	struct ipv4_header *header;
	uint8_t proto;
	struct netdev *dev;
	struct netif *nif;
	uint32_t gw, saddr;

	proto = nb->protocol & 0xFF;
	nb = netbuf_realloc(nb, NBAF_NETWORK, sizeof(*header));
	header = nb->network.data;
#ifdef HAVE_BIG_ENDIAN
	header->ihl_version = IPV4_VERSION | ((sizeof(*header) / sizeof(uint32_t)) << 4);
#else
	header->ihl_version = (IPV4_VERSION << 4) | (sizeof(*header) / sizeof(uint32_t));
#endif

	header->tos = 0;
	header->length = htons((uint16_t)(nb->network.size +
		nb->transport.size + nb->application.size));
	header->offset = 0;

	if(proto == IP_PROTO_IGMP)
		header->ttl = 1;
	else
		header->ttl = IPV4_TTL;

	header->protocol = proto;
	header->daddr = htonl(dst);

	if(dst == INADDR_BCAST || IS_MULTICAST(dst)) {
		/* broadcast */
		netbuf_free(nb);
		return;
	}

	/* Unicast */
	dev = route4_lookup(dst, &gw);
	if(!dev) {
		netbuf_free(nb);
		return;
	}

	nif = &dev->nif;
	header->id = netif_get_id(nif);
	saddr = ipv4_ptoi(nif->local_ip);
	header->saddr = htonl(saddr);
	header->chksum = ip_checksum(0, nb->network.data, nb->network.size);

	switch(nif->iftype) {
	case NIF_TYPE_ETHER:
		nb->protocol = ETH_TYPE_IP;
		neighbour_output(dev, nb, &dst, IPV4_ADDR_SIZE, translate_ipv4_to_mac);
		break;

	default:
		netbuf_free(nb);
		return;
	}

	return;
}
