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
#include <estack/in.h>
#include <estack/in6.h>

typedef struct ip_addr {
	uint8_t version;
	union {
		struct in_addr in4_addr;
		struct in6_addr in6_addr;
	} addr;
} ip_addr_t;

#define add4 addr.in4_addr
#define addr6 addr.in6_addr

CDECL
extern DLL_EXPORT bool ip_addr_cmp(ip_addr_t *a, ip_addr_t *b);
CDECL_END

#endif
