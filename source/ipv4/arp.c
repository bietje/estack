/*
 * E/STACK ethernet header
 *
 * Author: Michel Megens
 * Date: 06/12/2017
 * Email: dev@bietje.net
 */

#include <stdlib.h>
#include <stdio.h>

#include <estack/estack.h>
#include <estack/netbuf.h>
#include <estack/netdev.h>
#include <estack/arp.h>
#include <estack/inet.h>
#include <estack/log.h>

#define ARP_TYPE_ETHERNET 1
#define ARP_TYPE_IP 0x800

#ifdef HAVE_DEBUG
static void arp_print_info(struct arp_header *hdr, struct arp_ipv4_header *ip4hdr)
{
	char buf[16];
	char hwbuf[18];

	UNUSED(hdr);

	print_dbg("Received an ARP packet:\n");

	ipv4_ntoa(ip4hdr->ip_src_addr, buf, 16);
	ethernet_mac_ntoa(ip4hdr->hw_src_addr, hwbuf, 18);
	print_dbg("\tARP source IP: %s\n", buf);
	print_dbg("\tARP source MAC: %s\n", hwbuf);

	ipv4_ntoa(ip4hdr->ip_target_addr, buf, 16);
	ethernet_mac_ntoa(ip4hdr->hw_target_addr, hwbuf, 18);
	print_dbg("\tARP destination IP: %s\n", buf);
	print_dbg("\tARP destination MAC: %s\n", hwbuf);
}
#else
static inline void arp_print_info(struct arp_header *hdr, struct arp_ipv4_header *ip4hdr)
{
}
#endif

static void arp_input_ipv4(struct netbuf *nb, struct arp_header *hdr)
{
	struct arp_ipv4_header *ip4hdr;

	ip4hdr = (void*)(hdr + 1);
	arp_print_info(hdr, ip4hdr);
}

void arp_input(struct netbuf *nb)
{
	struct arp_header *hdr;

	if (nb->network.size < sizeof(*hdr)) {
		netbuf_set_flag(nb, NBUF_DROPPED);
		return;
	}

	hdr = nb->network.data;
	hdr->hwtype = ntohs(hdr->hwtype);
	hdr->protocol = ntohs(hdr->protocol);

	switch (hdr->protocol) {
	case ARP_TYPE_IP:
		arp_input_ipv4(nb, hdr);
		break;

	default:
		netbuf_set_flag(nb, NBUF_DROPPED);
		break;
	}
}
