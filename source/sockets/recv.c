/*
 * E/STACK socket recv
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
		estack_event_wait(&sock->read_event, FOREVER);
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

ssize_t estack_recv(int fd, void *buf, size_t length, int flags)
{
	UNUSED(flags);
	return estack_recvfrom(fd, buf, length, flags, NULL, 0);
}

