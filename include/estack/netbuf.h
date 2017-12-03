/*
 * E/STACK netbuf header
 *
 * Author: Michel Megens
 * Date: 01/12/2017
 * Email: dev@bietje.net
 */

#ifndef __NETBUF_H__
#define __NETBUF_H__

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <estack/estack.h>
#include <estack/list.h>
#include <estack/netdev.h>

struct DLL_EXPORT nbdata {
	void *data;
	size_t size;
};

#define NBUF_ARRIVED 0
#define NBUF_DROPPED 1
#define NBUF_AGAIN   2
#define NBUF_RX      3

struct DLL_EXPORT netbuf {
	struct list_head entry;

	struct nbdata datalink,
		network,
		transport,
		application;

	struct netdev *dev;
	uint16_t protocol;
	uint32_t flags;
};

CDECL
static inline int netbuf_test_flag(struct netbuf *nb, unsigned int num)
{
	register uint32_t mask;

	mask = nb->flags >> num;
	mask &= 1UL;
	return mask == 1UL;
}

static inline void netbuf_set_flag(struct netbuf *nb, unsigned int num)
{
	register uint32_t mask;

	mask = 1UL << num;
	nb->flags |= mask;
}
CDECL_END

#endif __NETBUF_H__
