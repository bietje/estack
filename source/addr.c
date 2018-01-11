/*
 * E/STACK sockets
 *
 * Author: Michel Megens
 * Date:   22/12/2017
 * Email:  dev@bietje.net
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <estack/estack.h>
#include <estack/addr.h>

#include <estack/in.h>
#include <estack/in6.h>

bool ip_addr_cmp(ip_addr_t *a, ip_addr_t *b)
{
	bool rv;

	if(a->type != b->type && a->type != IPADDR_TYPE_ANY && b->type != IPADDR_TYPE_ANY)
		return false;

	switch(a->type) {
	case 4:
		rv = a->addr.in4_addr.s_addr == b->addr.in4_addr.s_addr;
		break;

	case 6:
		rv = (bool)memcmp(a->addr.in6_addr.s6_addr, b->addr.in6_addr.s6_addr, 16);
		break;

	default:
		rv = false;
		break;
	}

	return rv;
}

bool ip_addr_any(const ip_addr_t *addr)
{
	const uint32_t *ip6;

	if(addr->type == 4)
		return addr->addr.in4_addr.s_addr == INADDR_ANY;

	if(unlikely(addr->type != 6 && addr->type != IPADDR_TYPE_ANY))
		return false;

	/* IPv6 */
	for(int idx = 0; idx < IP6_ADDR_LENGTH / sizeof(uint32_t); idx++) {
		ip6 = (const uint32_t *)addr->addr.in6_addr.s6_addr;
		if(ip6[idx] != 0x0)
			return false;
	}

	return true;
}
