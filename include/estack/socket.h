/*
 * E/STACK sockets
 *
 * Author: Michel Megens
 * Date:   22/12/2017
 * Email:  dev@bietje.net
 */

#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <stdlib.h>
#include <stdint.h>
#include <estack.h>

#include <estack/addr.h>
#include <estack/netbuf.h>

#define MAX_SOCKETS 16

typedef unsigned short sa_family_t;

#define SO_LISTEN 0x1
#define SO_STREAM 0x2
#define SO_DGRAM  0x4
#define SO_INET   0x8

#define SO_UDP    0x10
#define SO_TCP    0x20

struct DLL_EXPORT socket {
	int fd;
	ip_addr_t addr;
	uint16_t port;
	uint32_t flags;

	estack_event_t read_event;

	void *rcv_buffer;
	size_t rcv_length,
		rcv_index;

	int(*rcv_event)(struct netbuf *nb);
};

#ifndef _WINSOCK2API_
struct sockaddr {
	sa_family_t sa_family;
	char        sa_data[14];
};

typedef enum {
	AF_INET,
} socket_domain_t;

#define PF_INET AF_INET

typedef enum {
	SOCK_STREAM,
	SOCK_DGRAM,
	SOCK_RAW,
} socket_protocol_t;
#endif

CDECL
extern DLL_EXPORT int socket_add(struct socket *socket);
extern DLL_EXPORT int socket_remove(int fd);
extern DLL_EXPORT struct socket *socket_find(ip_addr_t *addr, uint16_t port);
CDECL_END

#endif
