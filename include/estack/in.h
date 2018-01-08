/*
 * E/STACK - IPv6 definitions
 *
 * Author: Michel Megens
 * Date:   23/12/2017
 * Email:  dev@bietje.net
 */

#ifndef __IN_ADDR_H__
#define __IN_ADDR_H__

#include <stdlib.h>
#include <stdint.h>

#include <estack/estack.h>

#ifndef _WINSOCK2API_

struct in_addr {
	uint32_t s_addr;
};

struct sockaddr_in {
#define SOCK_SIZE 16
	unsigned short sin_family;
	uint16_t       sin_port;
	struct in_addr sin_addr;
	uint8_t __pad[SOCK_SIZE - sizeof(short) -
					sizeof(short) - sizeof(struct in_addr)];
};

#endif /* WINSOCK */

#endif
