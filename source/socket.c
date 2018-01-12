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

int socket_datagram_receive_event(struct socket *sock, struct netbuf *nb)
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

int socket_stream_receive_event(struct socket *sock, struct netbuf *nb)
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
