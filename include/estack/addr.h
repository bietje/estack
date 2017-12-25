/*
 * E/STACK - IP address header
 *
 * Author: Michel Megens
 * Date:   23/12/2017
 * Email:  dev@bietje.net
 */

#ifndef __ADDR_H__
#define __ADDR_H__

#include <stdlib.h>
#include <stdio.h>

#include <estack/estack.h>
#include <estack/ip.h>

typedef struct {
	uint32_t addr[4];
} ip6_addr_t;

typedef struct {
	uint32_t addr;
} ip4_addr_t;

typedef enum {
	IP4_ADDR = 4,
	IP6_ADDR = 6,
	IP46_ADDR = 46,
} ip_addr_type_t;

typedef struct {
	union {
		ip6_addr_t ip6;
		ip4_addr_t ip4;
	};

	ip_addr_type_t type;
} ip_addr_t;

CDECL
extern DLL_EXPORT bool ip_addr_cmp(ip_addr_t *a, ip_addr_t *b);
CDECL_END

#endif
