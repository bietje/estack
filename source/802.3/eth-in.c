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

void ethernet_input(struct netbuf *nb)
{
	struct ethernet_header *hdr;

	hdr = nb->datalink.data;
	nb->network.data = hdr + 1;
	nb->network.size = nb->datalink.size - sizeof(struct ethernet_header);
	nb->datalink.size = sizeof(struct ethernet_header);

	nb->protocol = ntohs(hdr->type);

	switch (nb->protocol) {
	case ETH_TYPE_ARP:
		printf("Arp packet received!\n");
		break;

	case ETH_TYPE_IP:
	case ETH_TYPE_IP6:
	default:
		printf("Received protocol with unkown type field!\n");
		netbuf_set_flag(nb, NBUF_DROPPED);
		break;
	}
}
