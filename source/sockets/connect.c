/*
 * E/STACK socket connect
 *
 * Author: Michel Megens
 * Date:   12/01/2018
 * Email:  dev@bietje.net
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <estack.h>

#include <estack/error.h>
#include <estack/socket.h>

static int datagram_connect_ipv4(struct socket *sock, const struct sockaddr *addr, socklen_t len)
{
	struct sockaddr_in *in;

	in = (struct sockaddr_in*)addr;
	print_dbg("Datagram connect: 0x%x\n", ntohl(in->sin_addr.s_addr));
	sock->rport = in->sin_port;
	sock->addr.addr.in4_addr.s_addr = in->sin_addr.s_addr;
	sock->addr.type = IPADDR_TYPE_V4;
	sock->lport = htons(eph_port_alloc());
	sock->local.addr.in4_addr.s_addr = INADDR_ANY;

	sock->flags |= SO_CONNECTED;

	return 0;
}

int estack_connect(int fd, const struct sockaddr *addr, socklen_t len)
{
	struct socket *sock;

	sock = socket_get(fd);
	if(!sock) {
		assert(sock);
		return -EINVALID;
	}

	switch(addr->sa_family) {
	case AF_INET:
		if(sock->flags & SO_DGRAM)
			return datagram_connect_ipv4(sock, addr, len);
		break;
	case AF_INET6:
		return -EINVALID;
	}

	return -EINVALID;
}
