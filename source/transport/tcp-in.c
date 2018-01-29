/*
 * E/STACK - TCP
 *
 * Author: Michel Megens
 * Date:   19/01/2018
 * Email:  dev@bietje.net
 */

#include <estack/estack.h>
#include <estack/list.h>
#include <estack/error.h>
#include <estack/socket.h>
#include <estack/tcp.h>
#include <estack/ip.h>
#include <estack/inet.h>

static int tcp_input_verify(struct netbuf *nb, struct tcp_hdr *hdr)
{
	struct ipv4_header *ip4hdr;
	uint16_t csum;

	if(ip_is_ipv4(nb)) {
		ip4hdr = nb->network.data;
		csum = ipv4_inet_csum(nb->transport.data, (uint16_t) nb->transport.size,
			ipv4_get_saddr(ip4hdr), ipv4_get_daddr(ip4hdr), IP_PROTO_TCP);

		if(csum) {
			print_dbg("Dropping TCP segment with bogus checksum: %x\n", hdr->checksum);
			return -EINVALID;
		}
	} else {
		print_dbg("IPv6/TCP not yet supported!\n");
		return -EINVALID;
	}

	return -EOK;
}

void tcp_input(struct netbuf *nb)
{
	struct tcp_hdr *hdr;
	uint16_t hdrlen;
	ip_addr_t addr;
	struct socket *sock;

	hdr = (struct tcp_hdr *)nb->transport.data;
	hdrlen = tcp_hdr_get_hlen(hdr) * sizeof(uint32_t);

	if(nb->transport.size > hdrlen) {
		nb->application.data = ((uint8_t*)nb->transport.data) + hdrlen;
		nb->application.size = nb->transport.size - hdrlen;
		nb->transport.size = hdrlen;
	}

	if(tcp_input_verify(nb, hdr)) {
		netbuf_set_dropped(nb);
		return;
	}

	hdr->sport = ntohs(hdr->sport);
	sock = NULL;
	if(ip_is_ipv4(nb)) {
		addr.type = IPADDR_TYPE_V4;
		addr.addr.in4_addr.s_addr = htonl(ipv4_get_daddr(nb->network.data));
		sock = socket_find(&addr, hdr->dport);
	}

	hdr->dport = ntohs(hdr->dport);
	hdr->seq_no = ntohl(hdr->seq_no);
	hdr->window = ntohs(hdr->window);
	hdr->ack_no = ntohl(hdr->ack_no);

	if(sock) {
		print_dbg("TCP segment arrived!\n");
		tcp_process(sock, nb);
	} else {
		print_dbg("TCP segment dropped: NO SOCKET!\n");
		netbuf_set_dropped(nb);
	}
}
