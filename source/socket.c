/*
 * E/STACK - Socket API
 *
 * Author: Michel Megens
 * Date:   22/12/2017
 * Email:  dev@bietje.net
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <estack/estack.h>
#include <estack/socket.h>
#include <estack/addr.h>
#include <estack/udp.h>
#include <estack/ip.h>
#include <estack/netbuf.h>
#include <estack/error.h>

#define SOCK_EVENT_LENGTH MAX_SOCKETS

static int socket_receive_event(struct socket *sock, struct netbuf *nb)
{
	size_t length;
	uint8_t *ptr;

	length = nb->application.size;
	sock->rcv_buffer = realloc(sock->rcv_buffer, sock->rcv_length + length);

	assert(sock->rcv_buffer);
	if(sock->rcv_buffer)
		return -ENOMEM;

	ptr = (uint8_t*)sock->rcv_buffer;
	memcpy(&ptr[sock->rcv_length], nb->application.data, length);
	sock->rcv_length += length;

	netbuf_set_flag(nb, NBUF_ARRIVED);
	if(sock->readsize != 0 && sock->readsize <= sock->rcv_length)
		estack_event_signal(&sock->read_event);

	return length;
}

static struct socket *socket_alloc(void)
{
	struct socket *sock;

	sock = z_alloc(sizeof(*sock));
	assert(sock);
	estack_event_create(&sock->read_event, SOCK_EVENT_LENGTH);
	sock->rcv_event = socket_receive_event;
	return sock;
}

static void socket_free(struct socket *sock)
{
	assert(sock);

	estack_event_destroy(&sock->read_event);
	free(sock);
}

int estack_socket(int domain, int type, int protocol)
{
	struct socket *sock;

	sock = socket_alloc();
	if(domain == AF_INET)
		domain = PF_INET;

	if(domain != PF_INET)
		return -EINVALID;

	switch(type) {
	case SOCK_STREAM:
		sock->flags = SO_TCP | SO_STREAM;
		break;

	case SOCK_DGRAM:
		sock->flags = SO_UDP | SO_DGRAM;
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
