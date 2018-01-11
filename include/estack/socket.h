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

#include <estack/inet.h>
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
#define SO_CONNECTED 0x40

struct DLL_EXPORT socket {
	int fd;
	ip_addr_t local; //!< Local address
	ip_addr_t addr; //!< Remote address
	uint16_t rport;  //!< Remote port
	uint16_t lport; //!< Local port
	uint32_t flags;

	estack_mutex_t mtx;
	estack_event_t read_event;

	void *rcv_buffer;
	size_t rcv_length,
		rcv_index;
	size_t readsize;

	int(*rcv_event)(struct socket *sock, struct netbuf *nb);
};

#ifdef CONFIG_NO_SYS
struct sockaddr {
	sa_family_t sa_family;
	char        sa_data[14];
};

typedef size_t socklen_t;

typedef enum {
	AF_INET,
	AF_INET6,
} socket_domain_t;

#define PF_INET AF_INET
#define PF_INET6 AF_INET6

typedef enum {
	SOCK_STREAM,
	SOCK_DGRAM,
	SOCK_RAW,
} socket_protocol_t;
#endif

CDECL
extern DLL_EXPORT int socket_add(struct socket *socket);
extern DLL_EXPORT struct socket *socket_remove(int fd);
extern DLL_EXPORT struct socket *socket_find(ip_addr_t *addr, uint16_t port);
extern DLL_EXPORT struct socket *socket_get(int fd);
extern DLL_EXPORT uint16_t eph_port_alloc(void);

extern DLL_EXPORT int socket_trigger_receive(int fd, void *data, size_t length);
extern DLL_EXPORT int estack_recv(int fd, void *buf, size_t length, int flags);
extern DLL_EXPORT int estack_socket(int domain, int type, int protocol);
extern DLL_EXPORT int estack_close(int fd);
extern DLL_EXPORT int estack_connect(int fd, const struct sockaddr *addr, socklen_t len);
CDECL_END

#endif
