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

#define SOCK_EVENT_LENGTH MAX_SOCKETS

extern DLL_EXPORT int socket_stream_receive_event(struct socket *sock, struct netbuf *nb);
extern DLL_EXPORT int socket_datagram_receive_event(struct socket *sock, struct netbuf *nb);

static struct socket *socket_alloc(void)
{
	struct socket *sock;

	sock = z_alloc(sizeof(*sock));
	assert(sock);

	estack_mutex_create(&sock->mtx, 0);
	estack_event_create(&sock->read_event, SOCK_EVENT_LENGTH);
	list_head_init(&sock->lh);
	return sock;
}

static void socket_free(struct socket *sock)
{
	assert(sock);

	estack_mutex_lock(&sock->mtx, 0);
	estack_event_destroy(&sock->read_event);
	estack_mutex_unlock(&sock->mtx);
	estack_mutex_destroy(&sock->mtx);

	if(sock->rcv_buffer)
		free(sock->rcv_buffer);
	free(sock);
}

int estack_socket(int domain, int type, int protocol)
{
	struct socket *sock;

	sock = socket_alloc();
	if(domain == AF_INET)
		domain = PF_INET;

	if(domain == AF_INET6)
		domain = PF_INET6;

	if(domain != PF_INET && domain != PF_INET6)
		return -EINVALID;
	
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

	sock = socket_remove(fd);
	if(!sock)
		return -EINVALID;

	socket_free(sock);
	return -EOK;
}
