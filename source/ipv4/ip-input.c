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
#include <estack/icmp.h>
#include <estack/route.h>
#include <estack/udp.h>
#include <estack/in.h>

static inline struct ipv4_header *ipv4_nbuf_to_iphdr(struct netbuf *nb)
{
	assert(nb->network.data);
	assert(nb->network.size);

	return nb->network.data;
}

static inline int ipv4_is_fragmented(struct ipv4_header *hdr)
{
	uint16_t offset;
	uint8_t flags;

	flags = ipv4_get_flags(hdr);
	offset = ipv4_get_offset(hdr);

	return (flags & (1UL << IP4_MORE_FRAGMENTS_FLAG)) != 0 || offset;
}

static bool ipv4_forward(struct netbuf *nb, struct ipv4_header *hdr)
{
	struct netdev *dev;

	dev = route4_lookup(hdr->daddr, 0);
	if(nb->dev == dev)
		return false;

	netbuf_set_dev(nb, dev);
	netbuf_set_flag(nb, NBUF_REUSE);
	netbuf_set_flag(nb, NBUF_WAS_RX);
	hdr->offset = htons(hdr->offset);
	hdr->saddr = htonl(hdr->saddr);
	__ipv4_output(nb, hdr->daddr);
	return true;
}

void ipv4_input(struct netbuf *nb)
{
	struct ipv4_header *hdr;
	uint8_t hdrlen, version;
	struct netif *nif;
	uint32_t localmask;
	uint32_t localip;
	uint16_t csum;

	hdr = ipv4_nbuf_to_iphdr(nb);
#ifdef HAVE_BIG_ENDIAN
	hdrlen = (hdr->ihl_version >> 4) & 0xF;
	version = hdr->version & 0xF;
#else
	hdrlen = hdr->ihl_version & 0xF;
	version = (hdr->ihl_version >> 4) & 0xF;
#endif

	if(version != 4) {
		print_dbg("Dropping IPv4 packet with bogus version field (%u)!\n", version);
		netbuf_set_flag(nb, NBUF_DROPPED);
		return;
	}

	if(!netbuf_test_and_clear_flag(nb, NBUF_NOCSUM)) {
		csum = ip_checksum(0, nb->network.data, sizeof(*hdr));
		if(csum) {
			print_dbg("Dropping IPv4 packet with bogus checksum (is %x, should be %x)\n",
						hdr->chksum, csum);
			netbuf_set_flag(nb, NBUF_DROPPED);
			return;
		}
	}

	hdrlen = hdrlen * sizeof(uint32_t);
	nb->network.size = hdrlen;

	if(hdrlen < sizeof(*hdr) || hdrlen > nb->network.size) {
		print_dbg("Dropping IPv4 packet with bogus header length (%u)!\n", hdrlen);
		print_dbg("\tHeader size: %u\n", hdrlen);
		print_dbg("\tsizeof(ipv4_header): %u :: Buffer size: %u\n", sizeof(*hdr), nb->network.size);
		netbuf_set_flag(nb, NBUF_DROPPED);
		return;
	}

	hdr->offset = ntohs(hdr->offset);
	hdr->length = ntohs(hdr->length);

	hdr->saddr = ntohl(hdr->saddr);
	hdr->daddr = ntohl(hdr->daddr);
	nif = &nb->dev->nif;

	localip = ipv4_ptoi(nif->local_ip);
	localmask = ipv4_ptoi(nif->ip_mask);
	nb->protocol = hdr->protocol;

	if(unlikely(hdr->daddr == INADDR_BCAST ||
		(localip && localmask != INADDR_BCAST && (hdr->daddr | localmask) == INADDR_BCAST))) {
		/* Datagram is a broadcast */
		netbuf_set_flag(nb, NBUF_BCAST);
	} else if(unlikely(IS_MULTICAST(hdr->daddr))) {
		/* TODO: implement multicast */
		print_dbg("Multicast not supported, dropping IP datagram.\n");
		netbuf_set_flag(nb, NBUF_MULTICAST);
		netbuf_set_flag(nb, NBUF_DROPPED);
		return;
	} else {
		netbuf_set_flag(nb, NBUF_UNICAST);
	}

	nb->transport.size = hdr->length - hdrlen;
	if(nb->transport.size < hdrlen) {
		netbuf_set_flag(nb, NBUF_DROPPED);
		return;
	}

	if(nb->transport.size)
		nb->transport.data = ((uint8_t*)hdr) + hdrlen;

	if(localip && (hdr->daddr == 0 || hdr->daddr != localip)) {
		if(ipv4_forward(nb, hdr))
			return;
		print_dbg("Dropping IP packet that isn't ment for us..\n");
		netbuf_set_flag(nb, NBUF_DROPPED);
		return;
	}

	if(ipv4_is_fragmented(hdr)) {
		ipfrag4_add_packet(nb);
		return;
	} else {
		ipfrag4_tmo();
	}

	ipv4_input_postfrag(nb);
}

void ipv4_input_postfrag(struct netbuf *nb)
{
	struct ipv4_header *hdr;
	bool demux;

	hdr = nb->network.data;
	demux = netdev_demux_handle(nb);

	switch(hdr->protocol) {
	case IP_PROTO_ICMP:
		print_dbg("Received an IPv4 ICMP packet!\n");
		icmp_input(nb);
		break;

	case IP_PROTO_UDP:
		udp_input(nb);
		break;

	case IP_PROTO_IGMP:
	default:
		if(!demux) {
			netbuf_set_flag(nb, NBUF_REUSE);
			netbuf_set_flag(nb, NBUF_WAS_RX);
			icmp_response(nb, ICMP_UNREACH, ICMP_UNREACH_PROTO, 0);
		}
		break;
	}
}
