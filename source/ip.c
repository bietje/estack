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

void ip_input(struct netbuf *nb)
{
	switch (nb->protocol) {
	case PROTO_IPV4:
		ipv4_input(nb);
		break;
	case PROTO_IPV6:
	default:
		print_dbg("Dropped supposed IP packet\n");
		netbuf_set_flag(nb, NBUF_DROPPED);
	}
}
