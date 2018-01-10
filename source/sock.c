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

struct socket_pool {
	estack_mutex_t mtx;
	struct socket *sockets[MAX_SOCKETS];
};

static struct socket_pool sockets;

static inline void socket_pool_lock(void)
{
	estack_mutex_lock(&sockets.mtx, 0);
}

static inline void socket_pool_unlock(void)
{
	estack_mutex_unlock(&sockets.mtx);
}

static inline bool socket_cmp(struct socket *x, struct socket *y)
{
	if(x->lport != y->lport)
		return false;

	if(ip_addr_any(&x->local))
		return true;
	return ip_addr_cmp(&x->local, &y->local);
}

struct socket *socket_find(ip_addr_t *addr, uint16_t port)
{
	struct socket *socket;
	struct socket tmp;

	tmp.local = *addr;
	tmp.lport = port;

	socket_pool_lock();
	for(int i = 0; i < MAX_SOCKETS; i++) {
		socket = sockets.sockets[i];

		if(!socket)
			continue;

		if(socket_cmp(socket, &tmp)) {
			socket_pool_unlock();
			return socket;
		}
	}
	socket_pool_unlock();

	return NULL;
}

struct socket *socket_get(int fd)
{
	struct socket *sock;

	socket_pool_lock();
	sock = sockets.sockets[fd];
	socket_pool_unlock();

	return sock;
}

struct socket *socket_remove(int fd)
{
	struct socket *sock;

	socket_pool_lock();
	sock = sockets.sockets[fd];
	
	if(!sock) {
		socket_pool_unlock();
		return NULL;
	}

	sock->fd = -1;
	sockets.sockets[fd] = NULL;
	socket_pool_unlock();

	return sock;
}

int socket_add(struct socket *socket)
{
	int fd;
	struct socket *sock;

	fd = -1;
	socket_pool_lock();
	for(int i = 0; i < MAX_SOCKETS; i++) {
		sock = sockets.sockets[i];

		if(sock)
			continue;

		sockets.sockets[i] = socket;
		fd = i;
		break;
	}
	socket_pool_unlock();

	if(fd < 0)
		return fd;

	socket->fd = fd;
	return fd;
}

#define EPHEMERAL_PORT_START 0xC000
#define EPHEMERAL_PORT_END   0xFFFF

static inline bool eph_port_inuse(uint16_t port)
{
	struct socket *s;

	for(int i = 0; i < MAX_SOCKETS; i++) {
		s = sockets.sockets[i];
		if(likely(!s))
			continue;
		if(unlikely(s->lport == port))
			return true;
	}

	return false;
}

uint16_t eph_port_alloc(void)
{
	uint16_t indx;

	socket_pool_lock();
	for(indx = EPHEMERAL_PORT_START;
			indx < EPHEMERAL_PORT_END; indx++) {
		if(unlikely(!eph_port_inuse(indx)))
			break;
	}
	socket_pool_unlock();

	return indx;
}

void socket_api_init(void)
{
	estack_mutex_create(&sockets.mtx, 0);
}

void socket_api_destroy(void)
{
	estack_mutex_destroy(&sockets.mtx);
}
