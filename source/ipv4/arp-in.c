/*
 * E/STACK ARP input handling
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
void arp_print_info(struct arp_header *hdr, struct arp_ipv4_header *ip4hdr)
{
	char buf[16];
	char hwbuf[18];

	UNUSED(hdr);

	print_dbg("ARP packet data:\n");

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
void arp_print_info(struct arp_header *hdr, struct arp_ipv4_header *ip4hdr)
{
}
#endif

#define IP_ADDR_BYTE_LENGTH 4

static void arp_handle_request_ipv4(struct netbuf *nb, struct arp_header *hdr)
{
	struct arp_ipv4_header *ip4hdr;
	struct netbuf *nbr;

	ip4hdr = (void*)(hdr + 1);
	nbr = arp_alloc_nb_ipv4(ARP_OP_REPLY, ip4hdr->ip_src_addr, ip4hdr->hw_src_addr);

	assert(nbr);
	arp_output(nb->dev, nbr, ip4hdr->hw_src_addr);
}

static void arp_input_ipv4(struct netbuf *nb, struct arp_header *hdr)
{
	struct arp_ipv4_header *ip4hdr;
	struct netif *nif;
	struct dst_cache_entry *dst;

	ip4hdr = (void*)(hdr + 1);
	ip4hdr->ip_src_addr = ntohl(ip4hdr->ip_src_addr);
	ip4hdr->ip_target_addr = ntohl(ip4hdr->ip_target_addr);
	nif = &nb->dev->nif;

	/*
	 * Discard packets that aren't ment for us
	 */
	if(ipv4_ptoi(nif->local_ip) != ip4hdr->ip_target_addr) {
		netbuf_set_flag(nb, NBUF_DROPPED);
		return;
	}

	/* Discard packets with our own source address */
	if(ipv4_ptoi(nif->local_ip) == ip4hdr->ip_src_addr) {
		netbuf_set_flag(nb, NBUF_DROPPED);
		return;
	}

	/* Discard all packets that have have an ETHERNET broadcast address */
	if(hdr->hwtype == ARP_TYPE_ETHERNET && ethernet_addr_is_broadcast(ip4hdr->hw_src_addr)) {
		netbuf_set_flag(nb, NBUF_DROPPED);
		return;
	}

	netbuf_set_flag(nb, NBUF_ARRIVED);

	/*
	 * Normally only ARP-reply packets are stored in the ARP cache. However, we are also
	 * storing ARP-request packet addresses. This might save us a second or so in the
	 * future. The reasoning is that if somebody wants our hardware address, its probably
	 * because they're going to want to talk to us. Proactive baby.
	 */
	dst = netdev_find_destination(nb->dev, (uint8_t*)&ip4hdr->ip_src_addr, IP_ADDR_BYTE_LENGTH);
	if(!dst)
		netdev_add_destination(nb->dev, ip4hdr->hw_src_addr, ETHERNET_MAC_LENGTH,
		(uint8_t*)&ip4hdr->ip_src_addr, IP_ADDR_BYTE_LENGTH);
	else
		netdev_update_destination(nb->dev, ip4hdr->hw_src_addr, ETHERNET_MAC_LENGTH,
		(uint8_t*)&ip4hdr->ip_src_addr, IP_ADDR_BYTE_LENGTH);

	if(hdr->opcode == ARP_OP_REQUEST) {
		arp_handle_request_ipv4(nb, hdr);
	}

	arp_print_info(hdr, ip4hdr);
}

void arp_input(struct netbuf *nb)
{
	struct arp_header *hdr;

	if(nb->network.size < sizeof(*hdr)) {
		netbuf_set_flag(nb, NBUF_DROPPED);
		return;
	}

	hdr = nb->network.data;
	hdr->hwtype = ntohs(hdr->hwtype);
	hdr->protocol = ntohs(hdr->protocol);
	hdr->opcode = ntohs(hdr->opcode);

	switch(hdr->protocol) {
	case ARP_TYPE_IP:
		arp_input_ipv4(nb, hdr);
		break;

	default:
		netbuf_set_flag(nb, NBUF_DROPPED);
		break;
	}
}
