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

#define MAX_SOCKETS 16

typedef unsigned short sa_family_t;

struct DLL_EXPORT socket {
	int fd;
	ip_addr_t addr;
	uint16_t port;
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
#endif

#endif
