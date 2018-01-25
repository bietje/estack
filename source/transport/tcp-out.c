/*
 * E/STACK - TCP
 * 
 * Author: Michel Megens
 * Date:   19/01/2018
 * Email:  dev@bietje.net
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <estack/estack.h>
#include <estack/error.h>
#include <estack/list.h>
#include <estack/socket.h>
#include <estack/tcp.h>
#include <estack/ip.h>
#include <estack/route.h>
#include <estack/inet.h>

static void tcp_write_options(struct tcp_pcb *pcb, struct tcp_hdr *hdr)
{

}

int tcp_output(struct netbuf *nb, struct tcp_pcb *pcb, uint32_t seq)
{
	struct tcp_hdr *hdr;
	struct socket *sock;
	uint16_t csum;
	struct netdev *dev;
	struct netif *nif;
	uint32_t saddr, dst;

	hdr = nb->transport.data;

	if(tcp_hdr_get_hlen(hdr) > 5)
		tcp_write_options(pcb, hdr);

	hdr->sport = pcb->sock.lport;
	hdr->dport = pcb->sock.rport;
	hdr->seq_no = htonl(seq);
	hdr->ack_no = htonl(pcb->rcv_next);
	hdr->window = htons(pcb->rcv_window);
	hdr->checksum = 0;
	hdr->urg_ptr = 0;
	nb->protocol = IP_PROTO_TCP;

	sock = &pcb->sock;
	if(sock->addr.type == IPADDR_TYPE_V4) {
		dst = sock->addr.addr.in4_addr.s_addr;
		dev = route4_lookup(dst, &saddr);
		if(dev) {
			nif = &dev->nif;
			saddr = ipv4_ptoi(nif->local_ip);
			nb->dev = dev;
		} else {
			saddr = 0;
			return -EINVALID;
		}
		csum = (uint16_t)ipv4_pseudo_partial_csum(htonl(saddr), dst, IP_PROTO_TCP,
			htons((uint16_t)(nb->transport.size + nb->application.size)));
		csum = ip_checksum_partial(csum, hdr, nb->transport.size);
		hdr->checksum = ip_checksum(csum, nb->application.data, nb->application.size);
		ipv4_output(nb, ntohl(sock->addr.addr.in4_addr.s_addr));
	} else {
		print_dbg("TCP over IPv6 is not yet supported!\n");
		return -EINVALID;
	}

	return -EOK;
}
