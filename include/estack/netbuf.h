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
#include <time.h>

#include <estack/estack.h>
#include <estack/list.h>
#include <estack/netdev.h>

struct DLL_EXPORT nbdata {
	void *data;
	size_t size;
};

#define NBUF_DATALINK_ALLOC    0
#define NBUF_NETWORK_ALLOC     1
#define NBUF_TRANSPORT_ALLOC   2
#define NBUF_APPLICATION_ALLOC 3

#define NBUF_ARRIVED           4
#define NBUF_DROPPED           5
#define NBUF_AGAIN             6
#define NBUF_RX                7

typedef enum {
	NBAF_DATALINK,
	NBAF_NETWORK,
	NBAF_TRANSPORT,
	NBAF_APPLICTION,
} netbuf_type_t;

struct DLL_EXPORT netbuf {
	struct list_head entry;
	struct list_head bl_entry;

	struct nbdata datalink,
		network,
		transport,
		application;
	size_t size;

	struct netdev *dev;
	uint16_t protocol;
	time_t timestamp;
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

static inline void netbuf_set_timestamp(struct netbuf *nb)
{
	nb->timestamp = estack_utime();
}

static inline void netbuf_set_dev(struct netbuf *nb, struct netdev *dev)
{
	nb->dev = dev;
}

extern DLL_EXPORT struct netbuf *netbuf_realloc(struct netbuf *nb, netbuf_type_t type, size_t size);
extern DLL_EXPORT struct netbuf *netbuf_alloc(netbuf_type_t type, size_t size);
extern DLL_EXPORT void netbuf_free(struct netbuf *nb);
extern DLL_EXPORT void netbuf_cpy_data(struct netbuf *nb, const void *src,
	size_t length, netbuf_type_t type);
extern DLL_EXPORT size_t netbuf_set_size(struct netbuf *nb);
CDECL_END

#endif //!__NETBUF_H__
