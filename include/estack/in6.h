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

#define IP6_ADDR_LENGTH 16
#ifdef CONFIG_NO_SYS
struct in6_addr {
	uint8_t s6_addr[16];
};

struct sockaddr_in6 {
	unsigned short  sin6_family;
	uint16_t         sin6_port;
	uint32_t        sin6_flowinfo;
	struct in6_addr sin6_addr;
	uint32_t        sin6_scope_id;
};
#else
#include <estack/inet.h>
#ifdef WIN32
#include <ws2ipdef.h>
#endif
#endif

#endif