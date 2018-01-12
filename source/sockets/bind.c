/*
 * E/STACK - Socket bind
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
#include <estack/socket.h>

int bind(int fd, const struct sockaddr *addr, socklen_t length)
{
	struct socket *sock;
	const struct sockaddr_in6 *sin6;
	const struct sockaddr_in *sin;

	sock = socket_get(fd);
	if(!sock)
		return -ENOSOCK;

	if(socket_find_by_addr(addr, length))
		return -EINUSE;

	if(addr->sa_family == AF_INET) {
		sin = (struct sockaddr_in*)addr;
		sock->lport = sin->sin_port;
		sock->local.addr.in4_addr.s_addr = sin->sin_addr.s_addr;
		sock->local.type = IPADDR_TYPE_V4;
	} else {
		/* AF_INET6 */
		sin6 = (struct sockaddr_in6*)addr;
		sock->lport = sin6->sin6_port;
		memcpy(sock->addr.addr.in6_addr.s6_addr, sin6->sin6_addr.s6_addr, IP6_ADDR_LENGTH);
		sock->addr.type = IPADDR_TYPE_V6;
	}

	return -EOK;
}
