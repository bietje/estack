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

#include <estack/inet.h>
#include <estack/estack.h>
#include <estack/socket.h>
#include <estack/addr.h>
#include <estack/udp.h>
#include <estack/ip.h>
#include <estack/netbuf.h>
#include <estack/error.h>

#define SOCK_EVENT_LENGTH MAX_SOCKETS

struct sock_rcv_buffer {
	struct list_head entry;
	ip_addr_t addr;
	uint16_t port;
	void *data;
	size_t index;
	size_t length;
};

static int socket_datagram_receive_event(struct socket *sock, struct netbuf *nb)
{
	size_t length;
	struct sock_rcv_buffer *buf;

	buf = malloc(sizeof(*buf));
	buf->index = 0;
	list_head_init(&buf->entry);
	length = nb->application.size;
	buf->data = malloc(length);

	if(!buf->data) {
		free(buf);
		return -ENOMEMORY;
	}

	buf->length = length;

	estack_mutex_lock(&sock->mtx, 0);
	memcpy(buf->data, nb->application.data, length);
	buf->port = udp_get_remote_port(nb);

	if(ip_is_ipv4(nb)) {
		buf->addr.addr.in4_addr.s_addr = ipv4_get_remote_address(nb);
		buf->addr.type = IPADDR_TYPE_V4;
	} else {
		print_dbg("IPv6 isn't supported yet!\n");
		free(buf->data);
		free(buf);
		return -EINVALID;
	}

	list_add(&buf->entry, &sock->lh);
	netbuf_set_flag(nb, NBUF_ARRIVED);

	if(sock->readsize != 0)
		estack_event_signal(&sock->read_event);

	estack_mutex_unlock(&sock->mtx);
	return length;
}

static int socket_stream_receive_event(struct socket *sock, struct netbuf *nb)
{
	return -1;
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
	socket_datagram_receive_event(s, nb);
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

static ssize_t datagram_recvfrom(struct socket *sock, void *buf, size_t length,
                                   int flags, struct sockaddr *addr, socklen_t len)
{
	size_t num;
	struct sock_rcv_buffer *buffer;
	struct sockaddr_in *sin;

	if(!length || !buf)
		return 0;

	estack_mutex_lock(&sock->mtx, 0);
	if(!len && !(sock->flags & SO_CONNECTED)) {
		estack_mutex_unlock(&sock->mtx);
		return -EINVALID;
	} 
 
	sock->readsize = length;
	if(likely(list_empty(&sock->lh))) {
		estack_mutex_unlock(&sock->mtx);
		estack_event_wait(&sock->read_event);
		estack_mutex_lock(&sock->mtx, 0);
	}

	buffer = list_first_entry(&sock->lh, struct sock_rcv_buffer, entry);
	num = buffer->length - buffer->index;
	if(num > length)
		num = length;

	memcpy(buf, ((uint8_t*)buffer->data) + buffer->index, num);
	buffer->index += num;

	if(len) {
		/* Store the remote address and port */
		if(buffer->addr.type == IPADDR_TYPE_V4) {
			sin = (struct sockaddr_in *)addr;
			sin->sin_family = AF_INET;
			sin->sin_port = htons(buffer->port);
			sin->sin_addr.s_addr = htonl(buffer->addr.addr.in4_addr.s_addr);
		} else {
			print_dbg("IPv6 not yet supported!\n");
		}
	}

	/* Release the buffer if its fully used up */
	if(buffer->index >= buffer->length) {
		free(buffer->data);
		list_del(&buffer->entry);
		free(buffer);
	}

	sock->readsize = 0;
	estack_mutex_unlock(&sock->mtx);
	return (ssize_t)num;
}

ssize_t estack_recvfrom(int fd, void *buf, size_t length,
                          int flags, struct sockaddr *addr, socklen_t len)
{
	struct socket *sock;
	
	sock = socket_get(fd);
	if(!sock)
		return -EINVALID;

	if(sock->flags & SO_DGRAM)
		return datagram_recvfrom(sock, buf, length, flags, addr, len);
	
	return -EINVALID;
}

int estack_recv(int fd, void *buf, size_t length, int flags)
{
	UNUSED(flags);
	return estack_recvfrom(fd, buf, length, flags, NULL, 0);
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

static int datagram_connect_ipv4(struct socket *sock, const struct sockaddr *addr, socklen_t len)
{
	struct sockaddr_in *in;

	in = (struct sockaddr_in*)addr;
	print_dbg("Datagram connect: 0x%x\n", ntohl(in->sin_addr.s_addr));
	sock->rport = ntohs(in->sin_port);
	sock->addr.addr.in4_addr.s_addr = ntohl(in->sin_addr.s_addr);
	sock->lport = eph_port_alloc();
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
