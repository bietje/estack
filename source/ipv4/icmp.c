/*
 * E/STACK - ICMP implementation
 *
 * Author: Michel Megens
 * Date:   18/12/2017
 * Email:  dev@bietje.net
 */

#include <stdlib.h>
#include <stdint.h>

#include <estack/estack.h>
#include <estack/log.h>
#include <estack/netbuf.h>
#include <estack/netdev.h>
#include <estack/icmp.h>
#include <estack/ip.h>
#include <estack/inet.h>

void icmp_output(uint8_t type, uint32_t dst, struct netbuf *nb)
{
	struct icmp_header *hdr;
	uint16_t csum;

	hdr = nb->transport.data;
	hdr->type = type;
	hdr->csum = 0;

	csum = ip_checksum_partial(0, nb->transport.data, nb->transport.size);
	hdr->csum = ip_checksum(csum, nb->application.data, nb->application.size);

	nb->protocol = IP_PROTO_ICMP;
	ipv4_output(nb, dst);
}

static void icmp_reflect(struct netbuf *nb, uint8_t type)
{
	struct ipv4_header *iphdr;
	uint32_t dst;
	struct netif *nif;

	nif = &nb->dev->nif;
	iphdr = nb->network.data;
	dst = iphdr->saddr;
	iphdr->saddr = htons(nif->local_ip);

	print_dbg("\tICMP ECHOREPLY sent\n");
	icmp_output(type, dst, nb);
}

void icmp_input(struct netbuf *nb)
{
	struct icmp_header *header;

	header = nb->transport.data;

	if(!header || nb->transport.size < sizeof(*header)) {
		netbuf_set_flag(nb, NBUF_DROPPED);
		return;
	}

	nb->application.size = nb->transport.size - sizeof(*header);
	if(nb->application.size) {
		nb->application.data = header + 1;
		nb->transport.size = sizeof(*header);
	}

	switch(header->type) {
	case ICMP_ECHO:
		print_dbg("\tICMP ECHOREQUEST received!\n");
		icmp_reflect(nb, ICMP_REPLY);
		netbuf_set_flag(nb, NBUF_REUSE);
		break;

	case ICMP_REPLY:
		netbuf_set_flag(nb, NBUF_ARRIVED);
		print_dbg("\tICMP REPLY received!\n");
		break;

	default:
		netbuf_set_flag(nb, NBUF_DROPPED);
		break;
	}
}
