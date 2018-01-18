/*
 * E/STACK - Generic IP functions
 *
 * Author: Michel Megens
 * Date: 12/12/2017
 * Email: dev@bietje.net
 */

#include <stdlib.h>
#include <stdint.h>

#include <estack/estack.h>
#include <estack/prototype.h>
#include <estack/netbuf.h>
#include <estack/netdev.h>
#include <estack/log.h>
#include <estack/ip.h>
#include <estack/inet.h>

#include "config.h"

#define FOLD_U32(u) (((u) >> 16) + ((u) & 0x0000ffffUL))

uint16_t ip_checksum_partial(uint16_t start, const void *buf, int len)
{
	register uint32_t sum;
	register uint8_t *buffer = (uint8_t *)buf;

	if(!buf || !len)
		return start;

	sum = start;
	while(len > 1) {
#ifdef HAVE_BIG_ENDIAN
		sum += ((uint8_t)*buffer << 8) | *(buffer + 1);
#else
		sum += ((uint8_t) *(buffer + 1) << 8) | *buffer;
#endif

		buffer += 2;
		len -= 2;
	}

	if(len) {
#ifdef HAVE_BIG_ENDIAN
		sum += (uint16_t) *buffer << 8;
#else
		sum += *buffer;
#endif
	}

	sum = FOLD_U32(sum);
	sum = FOLD_U32(sum);

	return (uint16_t)sum & 0xFFFF;
}

#pragma pack(push, 1)
struct pseudo_hdr {
	uint32_t src;
	uint32_t dst;
	uint8_t zero;
	uint8_t proto;
	uint16_t length;
};
#pragma pack(pop)

uint32_t ipv4_pseudo_partial_csum(uint32_t saddr, uint32_t daddr, uint8_t proto, uint16_t length)
{
	struct pseudo_hdr hdr;

	hdr.dst = daddr;
	hdr.src = saddr;
	hdr.zero = 0;
	hdr.proto = proto;
	hdr.length = length;

	return ip_checksum_partial(0, &hdr, sizeof(hdr));
}

uint16_t ipv4_inet_csum(const void *start, uint16_t length, uint32_t saddr,
							uint32_t daddr, uint8_t proto)
{
	uint32_t csum;

	csum = ip_checksum_partial(0, start, length);

	/* Add the pseudo header to the mix */
	saddr = htonl(saddr);
	daddr = htonl(daddr);
	csum += saddr & 0xFFFF;
	csum += (saddr >> 16) & 0xFFFF;
	csum += daddr & 0xFFFF;
	csum += (daddr >> 16) & 0xFFFF;

	csum += htons(proto);
	csum += htons(length);

	csum = FOLD_U32(csum);
	csum = FOLD_U32(csum);
	
	return (uint16_t)~(csum & 0xFFFF);
}

uint16_t ip_checksum(uint16_t start, const void *buf, int len)
{
	return ~ip_checksum_partial(start, buf, len);
}

void ip_input(struct netbuf *nb)
{
	netdev_demux_handle(nb);

	switch(nb->protocol) {
	case PROTO_IPV4:
		ipv4_input(nb);
		break;
	case PROTO_IPV6:
	default:
		print_dbg("Dropped supposed IP packet\n");
		netbuf_set_flag(nb, NBUF_DROPPED);
	}
}

void ip_htons(struct netbuf *nb)
{
	struct ipv4_header *hdr;

	if(ip_is_ipv4(nb)) {
		hdr = nb->network.data;
		hdr->daddr = htonl(hdr->daddr);
		hdr->saddr = htonl(hdr->saddr);
		hdr->length = htons(hdr->length);
		hdr->offset = htons(hdr->offset);
		hdr->id = htons(hdr->id);
	}
}
