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
#include <estack/inet.h>

int tcp_output(struct netbuf *nb, struct tcp_pcb *pcb)
{
	struct tcp_hdr *hdr;
	struct socket *sock;

	hdr = nb->transport.data;
	hdr->sport = pcb->sock.lport;
	hdr->dport = pcb->sock.rport;
	hdr->seq_no = htonl(pcb->snd_next);
	hdr->ack_no = htonl(pcb->rcv_next);
	hdr->window = htons(pcb->rcv_window);
	hdr->checksum = 0;
	hdr->urg_ptr = 0;

	sock = &pcb->sock;
	if(sock->addr.type == IPADDR_TYPE_V4) {
		ipv4_output(nb, ntohl(sock->addr.addr.in4_addr.s_addr));
	} else {
		print_dbg("TCP over IPv6 is not yet supported!\n");
		return -EINVALID;
	}

	return -EOK;
}