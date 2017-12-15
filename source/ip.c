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

#include "config.h"

uint16_t ip_checksum_partial(uint16_t start, const void *buf, int len)
{
	register uint32_t sum;
	register uint8_t *buffer = (uint8_t *)buf;

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

	while(sum >> 16) {
		sum = (uint16_t)sum + (sum >> 16);
	}

	return (uint16_t)sum & 0xFFFF;
}

uint16_t ip_checksum(uint16_t start, const void *buf, int len)
{
	return ~ip_checksum_partial(start, buf, len);
}

void ip_input(struct netbuf *nb)
{
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
