/*
 * E/STACK - Socket send
 *
 * Author: Michel Megens
 * Date:   12/01/2018
 * Email:  dev@bietje.net
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <estack.h>

#include <estack/error.h>
#include <estack/inet.h>
#include <estack/in6.h>
#include <estack/socket.h>
#include <estack/udp.h>

ssize_t estack_sendto(int fd, const void *msg, size_t length, int flags,
	const struct sockaddr *daddr, socklen_t len)
{
	struct socket *sock;
	struct netbuf *nb;
	ip_addr_t remote;
	struct sockaddr_in *sin;
	struct sockaddr_in6 *sin6;
	uint16_t rport;
	ssize_t rv;

	sock = socket_get(fd);
	rv = -EINVALID;

	if(!sock)
		return -EINVALID;

	estack_mutex_lock(&sock->mtx, 0);

	if(!(sock->flags & SO_CONNECTED) && !len) {
		estack_mutex_unlock(&sock->mtx);
		return -EINVALID;
	}

	nb = netbuf_alloc(NBAF_APPLICTION, length);
	netbuf_cpy_data(nb, msg, length, NBAF_APPLICTION);

	if(sock->flags & SO_CONNECTED) {
		memcpy(&remote, &sock->addr, sizeof(remote));
		rport = sock->rport;
	} else {
		if(daddr->sa_family == AF_INET) {
			sin = (void*)daddr;
			remote.addr.in4_addr.s_addr = sin->sin_addr.s_addr;
			remote.type = IPADDR_TYPE_V4;
			rport = sin->sin_port;
		} else {
			sin6 = (void*)daddr;
			memcpy(remote.addr.in6_addr.s6_addr, sin6->sin6_addr.s6_addr, IP6_ADDR_LENGTH);
			remote.type = IPADDR_TYPE_V6;

			rport = sin6->sin6_port;
			if(!sock->lport)
				sock->lport = eph_port_alloc();
		}
	}

	if(sock->flags & SO_DGRAM || sock->flags & SO_UDP) {
		udp_output(nb, &remote, rport, sock->lport);
		rv = length;
	}

	estack_mutex_unlock(&sock->mtx);
	return rv;
}

ssize_t estack_send(int fd, const void *buffer, size_t length, int flags)
{
	return estack_sendto(fd, buffer, length, flags, NULL, 0);
}
