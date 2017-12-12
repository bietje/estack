/*
 * E/STACK - IP input handler
 *
 * Author: Michel Megens
 * Date: 12/12/2017
 * Email: dev@bietje.net
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <estack/estack.h>
#include <estack/netbuf.h>
#include <estack/netdev.h>
#include <estack/ip.h>
#include <estack/log.h>
#include <estack/inet.h>

static inline struct ipv4_header *ipv4_nbuf_to_iphdr(struct netbuf *nb)
{
	assert(nb->network.data);
	assert(nb->network.size);

	return nb->network.data;
}

void ipv4_input(struct netbuf *nb)
{
	struct ipv4_header *hdr;
	uint8_t hdrlen, version;
	struct netif *nif;
	int bcast;
	uint32_t localmask;
	uint32_t localip;

	hdr = ipv4_nbuf_to_iphdr(nb);
#ifdef HAVE_BIG_ENDIAN
	hdrlen = (hdr->ihl_version >> 4) & 0xF;
	version = hdr->version & 0xF;
#else
	hdrlen = hdr->ihl_version & 0xF;
	version = (hdr->ihl_version >> 4) & 0xF;
#endif

	if (version != 4) {
		print_dbg("Dropping IPv4 packet with bogus version field (%u)!\n", version);
		netbuf_set_flag(nb, NBUF_DROPPED);
		return;
	}

	/* TODO: fragmentation */
	hdrlen = hdrlen * sizeof(uint32_t);
	if (hdrlen < sizeof(*hdr) || hdrlen > nb->network.size) {
		print_dbg("Dropping IPv4 packet with bogus header length (%u)!\n", hdrlen);
		print_dbg("\tHeader size: %u", hdrlen);
		print_dbg("\tsizeof(ipv4_header): %u :: Buffer size: %u\n", sizeof(*hdr), nb->network.size);
		netbuf_set_flag(nb, NBUF_DROPPED);
		return;
	}

	hdr->saddr = ntohl(hdr->saddr);
	hdr->daddr = ntohl(hdr->daddr);
	nif = &nb->dev->nif;

	localip = ipv4_ptoi(nif->local_ip);
	localmask = ipv4_ptoi(nif->ip_mask);

	if (unlikely(hdr->daddr == INADDR_BCAST ||
		(localip && localmask != INADDR_BCAST && (hdr->daddr | localmask) == INADDR_BCAST))) {
		/* Datagram is a broadcast */
		bcast = 1;
	} else if (unlikely(IS_MULTICAST(hdr->daddr))) {
		/* TODO: implement multicast */
		print_dbg("Multicast not supported, dropping IP datagram.\n");
		netbuf_set_flag(nb, NBUF_DROPPED);
		return;
	} else {
		bcast = 0;

		if (localip && (hdr->daddr == 0 || hdr->daddr != localip)) {
			print_dbg("Dropping IP packet that isn't ment for us..\n");
			netbuf_set_flag(nb, NBUF_DROPPED);
		}
		netbuf_set_flag(nb, NBUF_UNICAST);
	}

	/* TODO: handle IP packets */

	print_dbg("Received an IPv4 packet!\n");
	print_dbg("\tIP version: %u :: Header length: %u\n", version, hdrlen);
	netbuf_set_flag(nb, NBUF_ARRIVED);
}
