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

#include <config.h>

#define IPV4_VERSION 0x4

void ipv4_output(struct netbuf *nb, uint32_t dst)
{
	struct ipv4_header *header;
	uint8_t proto;

	proto = nb->protocol & 0xFF;
	nb = netbuf_realloc(nb, NBAF_NETWORK, sizeof(*header));
	header = nb->network.data;
#ifdef HAVE_BIG_ENDIAN
	header->ihl_version = IPV4_VERSION | ((sizeof(*header) / sizeof(uint32_t)) << 4);
#else
	header->ihl_version = (IPV4_VERSION << 4) | (sizeof(*header) / sizeof(uint32_t));
#endif

	header->tos = 0;
	header->length = htons(nb->network.size + nb->transport.size + nb->application.size);
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
	/* TODO: lookup the route */
	netbuf_free(nb);
	return;
}
