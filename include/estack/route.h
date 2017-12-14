/*
 * E/STACK - IP routing
 *
 * Author: Michel Megens
 * Date: 13/12/2017
 * Email: dev@bietje.net
 */

#ifndef __IP_ROUTE_H__
#define __IP_ROUTE_H__

#include <stdlib.h>
#include <stdint.h>

#include <estack/estack.h>
#include <estack/list.h>
#include <estack/ip.h>
#include <estack/netdev.h>
#include <estack/netbuf.h>

struct iproute_head {
	struct list_head head;
	uint8_t version;
};

struct iproute4_entry {
	struct list_head entry;

	uint32_t ip;
	uint32_t mask;
	uint32_t gateway;
	struct netdev *dev;
};

struct iproute6_entry {
	struct list_head entry;

	uint8_t ip[IPV6_ADDR_SIZE];
	uint8_t mask[IPV6_ADDR_SIZE];
	uint8_t gateway[IPV6_ADDR_SIZE];
	struct netdev *dev;
};

#endif
