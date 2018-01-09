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
	estack_mutex_lock(&sock->mtx, 0);
	sock->rcv_buffer = realloc(sock->rcv_buffer, sock->rcv_length + length);

	if(!sock->rcv_buffer) {
		estack_mutex_unlock(&sock->mtx);
		return -ENOMEM;
	}

	ptr = (uint8_t*)sock->rcv_buffer;

	memcpy(&ptr[sock->rcv_length], nb->application.data, length);
	sock->rcv_length += length;
	netbuf_set_flag(nb, NBUF_ARRIVED);

	if(sock->readsize != 0) {
		estack_event_signal(&sock->read_event);
	}

	estack_mutex_unlock(&sock->mtx);
	return length;
}

#ifdef HAVE_DEBUG
int socket_trigger_receive(int fd, void *data, size_t length)
{
	struct netbuf *nb;
	struct socket *s;

	s = socket_get(fd);
	if(!s)
		return -EINVALID;

	nb = netbuf_alloc(NBAF_APPLICTION, length);
	netbuf_cpy_data(nb, data, length, NBAF_APPLICTION);
	socket_receive_event(s, nb);
	netbuf_free(nb);
	return -EOK;
}
#endif

static struct socket *socket_alloc(void)
{
	struct socket *sock;

	sock = z_alloc(sizeof(*sock));
	assert(sock);

	estack_mutex_create(&sock->mtx, 0);
	estack_event_create(&sock->read_event, SOCK_EVENT_LENGTH);
	sock->rcv_event = socket_receive_event;
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

int estack_recv(int fd, void *buf, size_t length, int flags)
{
	struct socket *sock;
	const uint8_t *dataptr;
	size_t num;

	UNUSED(flags);
	sock = socket_get(fd);

	if(!length || !buf)
		return 0;

	if(!sock)
		return -EINVALID;

	estack_mutex_lock(&sock->mtx, 0);
	sock->readsize = length;
	if(likely(sock->rcv_index >= sock->rcv_length)) {
		estack_mutex_unlock(&sock->mtx);
		estack_event_wait(&sock->read_event);
		estack_mutex_lock(&sock->mtx, 0);
	}

	num = 0;
	if(likely(sock->rcv_index < sock->rcv_length)) {
		dataptr = sock->rcv_buffer;
		if(sock->rcv_index + length < sock->rcv_length)
			num = length;
		else
			num = sock->rcv_length - sock->rcv_index;

		memcpy(buf, &dataptr[sock->rcv_index], num);
		sock->rcv_index += num;

		if(sock->rcv_index >= sock->rcv_length) {
			sock->rcv_index = sock->rcv_length = 0;
			free(sock->rcv_buffer);
		}
	}

	sock->readsize = 0;
	estack_mutex_unlock(&sock->mtx);
	return (int)num;
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
