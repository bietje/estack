/*
 * E/STACK - Socket call
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
#include <estack/tcp.h>

#define SOCK_EVENT_LENGTH MAX_SOCKETS

extern DLL_EXPORT int socket_stream_receive_event(struct socket *sock, struct netbuf *nb);
extern DLL_EXPORT int socket_datagram_receive_event(struct socket *sock, struct netbuf *nb);

void socket_init(struct socket *sock)
{
	estack_mutex_create(&sock->mtx, 0);
	estack_event_create(&sock->read_event, SOCK_EVENT_LENGTH);
	list_head_init(&sock->lh);
	sock->err = -EOK;
}

static struct socket *socket_alloc(void)
{
	struct socket *sock;

	sock = z_alloc(sizeof(*sock));
	assert(sock);

	socket_init(sock);
	return sock;
}

void socket_destroy(struct socket *sock)
{
	estack_mutex_lock(&sock->mtx, 0);
	estack_event_destroy(&sock->read_event);
	estack_mutex_unlock(&sock->mtx);
	estack_mutex_destroy(&sock->mtx);
}

static void socket_free(struct socket *sock)
{
	assert(sock);

	if(sock->flags & (SO_STREAM | SO_TCP)) {
		tcp_socket_free(sock);
		return;
	}

	socket_destroy(sock);
	free(sock);
}

int estack_socket(int domain, int type, int protocol)
{
	struct socket *sock;

	if(domain == AF_INET)
		domain = PF_INET;

	if(domain == AF_INET6)
		domain = PF_INET6;

	if(domain != PF_INET && domain != PF_INET6)
		return -EINVALID;

	if(type == SOCK_STREAM)
		sock = tcp_socket_alloc();
	else
		sock = socket_alloc();
	
	if(domain == PF_INET)
		sock->local.type = IPADDR_TYPE_V4;
	else
		sock->local.type = IPADDR_TYPE_V6;

	switch(type) {
	case SOCK_STREAM:
		sock->flags = SO_TCP | SO_STREAM;
		sock->rcv_event = socket_stream_receive_event;
		break;

	case SOCK_DGRAM:
		sock->flags = SO_UDP | SO_DGRAM;
		sock->rcv_event = socket_datagram_receive_event;
		break;

	case SOCK_RAW:
		print_dbg("Raw socket not yet supported!\n");
	default:
		socket_free(sock);
		return -EINVALID;
	}

	socket_add(sock);

	if(sock->fd < 0) {
		socket_free(sock);
		return -EINVALID;
	}

	return sock->fd;
}

int estack_close(int fd)
{
	struct socket *sock;

	sock = socket_get(fd);
	if(!sock)
		return -EINVALID;

	if(sock->flags & SO_TCP)
		tcp_close(sock);
	socket_remove(fd);
	socket_free(sock);
	return -EOK;
}
