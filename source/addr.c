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

	if(a->version != b->version)
		return false;

	switch(a->version) {
	case 4:
		rv = a->addr.in4_addr.s_addr == b->addr.in4_addr.s_addr;
		break;

	case 6:
		rv = (bool)memcmp(a->addr.in6_addr.in6_u.u6_addr8, b->addr.in6_addr.in6_u.u6_addr8, 16);
		break;

	default:
		rv = false;
		break;
	}

	return rv;
}
