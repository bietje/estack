/*
 * E/STACK sockets
 *
 * Author: Michel Megens
 * Date:   22/12/2017
 * Email:  dev@bietje.net
 */

#include <stdlib.h>
#include <stdint.h>

#include <estack/estack.h>
#include <estack/socket.h>
#include <estack/addr.h>

static struct socket *sockets[MAX_SOCKETS];

static inline bool socket_cmp(struct socket *x, struct socket *y)
{
	if(x->port != y->port)
		return false;

	return ip_addr_cmp(&x->addr, &y->addr);
}

struct socket *socket_find(ip_addr_t *addr, uint16_t port)
{
	struct socket *socket;
	struct socket tmp;

	tmp.addr = *addr;
	tmp.port = port;

	for(int i = 0; i < MAX_SOCKETS; i++) {
		socket = sockets[i];

		if(!socket)
			continue;

		if(socket_cmp(socket, &tmp))
			return socket;
	}

	return NULL;
}

int socket_remove(int fd)
{
	struct socket *sock;

	sock = sockets[fd];
	
	if(!sock)
		return -1;

	sock->fd = -1;
	sockets[fd] = NULL;
	return 0;
}

int socket_add(struct socket *socket)
{
	int fd;
	struct socket *sock;

	fd = -1;
	for(int i = 0; i < MAX_SOCKETS; i++) {
		sock = sockets[i];

		if(sock)
			continue;

		sockets[i] = socket;
		fd = i;
		break;
	}

	if(fd < 0)
		return fd;

	socket->fd = fd;
	return fd;
}
