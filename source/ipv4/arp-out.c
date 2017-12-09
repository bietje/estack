/*
 * E/STACK ARP output handling
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
#define IP_ADDR_BYTE_LENGTH 4
#define IP4_ARP_SIZE sizeof(struct arp_header) + sizeof(struct arp_ipv4_header)

struct netbuf *arp_alloc_nb_ipv4(uint16_t type, uint32_t ip, uint8_t *mac)
{
	struct netbuf *nb;
	struct arp_ipv4_header *ip4hdr;
	struct arp_header *hdr;

	nb = netbuf_alloc(NBAF_NETWORK, IP4_ARP_SIZE);
	assert(nb);

	hdr = nb->network.data;
	ip4hdr = (void*)(hdr + 1);

	hdr->hwtype = htons(ARP_TYPE_ETHERNET);
	hdr->protocol = htons(ARP_TYPE_IP);
	hdr->protosize = IP_ADDR_BYTE_LENGTH;
	hdr->hwsize = ETHERNET_MAC_LENGTH;
	hdr->opcode = htons(type);

	/* Load addresses. If no address is given, set to broadcast */
	if (mac)
		memcpy(ip4hdr->hw_target_addr, mac, ETHERNET_MAC_LENGTH);
	else
		memset(ip4hdr->hw_target_addr, 0x0, ETHERNET_MAC_LENGTH);
	ip4hdr->ip_target_addr = htonl(ip);

	return nb;
}

#ifdef HAVE_DEBUG
static void arp_print_info_nwo(struct arp_header *hdr, struct arp_ipv4_header *ip4hdr)
{
	char buf[16];
	char hwbuf[18];

	UNUSED(hdr);

	print_dbg("ARP TX packet data:\n");

	ipv4_ntoa(ntohl(ip4hdr->ip_src_addr), buf, 16);
	ethernet_mac_ntoa(ip4hdr->hw_src_addr, hwbuf, 18);
	print_dbg("\tARP source IP: %s\n", buf);
	print_dbg("\tARP source MAC: %s\n", hwbuf);

	ipv4_ntoa(ntohl(ip4hdr->ip_target_addr), buf, 16);
	ethernet_mac_ntoa(ip4hdr->hw_target_addr, hwbuf, 18);
	print_dbg("\tARP destination IP: %s\n", buf);
	print_dbg("\tARP destination MAC: %s\n", hwbuf);
}
#else
static inline void arp_print_info_nwo(struct arp_header *hdr, struct arp_ipv4_header *ip4hdr)
{
}
#endif

void arp_output(struct netdev *dev, struct netbuf *nb, uint8_t *addr)
{
	struct arp_ipv4_header *ip4hdr;
	struct arp_header *hdr;
	struct netif *nif;

	assert(nb);
	assert(dev);

	hdr = nb->network.data;
	ip4hdr = (void*)(hdr + 1);
	nif = &dev->nif;

	ip4hdr->ip_src_addr = htonl(ipv4atoi(nif->local_ip));
	memcpy(ip4hdr->hw_src_addr, dev->hwaddr, dev->addrlen);
	arp_print_info_nwo(hdr, ip4hdr);

	nb->protocol = ETH_TYPE_ARP;
	netbuf_set_dev(nb, dev);

	dev->tx(nb, addr);
}

void arp_ipv4_request(struct netdev *dev, uint32_t addr)
{
	struct netbuf *nb;

	nb = arp_alloc_nb_ipv4(ARP_OP_REQUEST, addr, NULL);
	arp_output(dev, nb, NULL);
}
