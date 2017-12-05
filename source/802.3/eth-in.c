/*
 * E/Stack ethernet input
 *
 * Author: Michel Megens
 * Date: 03/12/2017
 * Email: dev@bietje.net
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <estack/estack.h>
#include <estack/netdev.h>
#include <estack/netbuf.h>
#include <estack/ethernet.h>
#include <estack/inet.h>
#include <estack/log.h>

void ethernet_input(struct netbuf *nb)
{
	struct ethernet_header *hdr;

	hdr = nb->datalink.data;
	nb->network.data = hdr + 1;
	nb->network.size = nb->datalink.size - sizeof(struct ethernet_header);
	nb->datalink.size = sizeof(struct ethernet_header);

	switch (nb->protocol) {
	case ETH_TYPE_ARP:
		print_dbg("Received ARP packet in an ethernet (802.3) frame\n");
		break;

	case ETH_TYPE_IP:
	case ETH_TYPE_IP6:
	default:
		print_dbg("Unkown ethernet type detected (packet dropped) (type number: %u)\n", nb->protocol);
		netbuf_set_flag(nb, NBUF_DROPPED);
		break;
	}
}