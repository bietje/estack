/*
 * E/STACK - IPv6 definitions
 *
 * Author: Michel Megens
 * Date:   23/12/2017
 * Email:  dev@bietje.net
 */

#ifndef __IN6_ADDR_H__
#define __IN6_ADDR_H__

#include <stdlib.h>
#include <stdint.h>

#include <estack/estack.h>

struct in6_addr {
#define IP6_ADDR_LENGTH 16
	union {
		uint8_t u6_addr8[16];
		uint16_t u6_addr16[8];
		uint32_t u6_addr[4];
	} in6_u;
};

#define s6_addr in6_u.u6_addr8

struct sockaddr_in6 {
	unsigned short  sin6_family;
	uint16_t         sin6_port;
	uint32_t        sin6_flowinfo;
	struct in6_addr sin6_addr;
	uint32_t        sin6_scope_id;
};

#endif