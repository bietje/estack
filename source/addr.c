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

bool ip_addr_cmp(ip_addr_t *a, ip_addr_t *b)
{
	bool rv;

	if(a->type != b->type)
		return false;

	switch(a->type) {
	case IP4_ADDR:
		rv = a->ip4.addr == b->ip4.addr;
		break;

	case IP6_ADDR:
		rv = memcmp(a->ip6.addr, b->ip6.addr, sizeof(b->ip6.addr));
		break;

	default:
		rv = false;
		break;
	}

	return rv;
}
