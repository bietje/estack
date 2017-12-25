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

struct DLL_EXPORT socket {
	int fd;
	ip_addr_t addr;
	uint16_t port;
};

#endif
