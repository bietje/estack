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

#define NBUF_DATALINK_ALLOC    0
#define NBUF_NETWORK_ALLOC     1
#define NBUF_TRANSPORT_ALLOC   2
#define NBUF_APPLICATION_ALLOC 3

#define NBUF_ARRIVED           4
#define NBUF_DROPPED           5
#define NBUF_AGAIN             6
#define NBUF_RX                7
#define NBUF_WAS_RX            8
#define NBUF_REUSE             9
#define NBUF_IS_LINEAR        10
 
#define NBUF_UNICAST          11
#define NBUF_MULTICAST        12
#define NBUF_BCAST            13

#define NBUF_NOCSUM           14
#define NBUF_TX_KEEP          15

#define NBUF_BL_QUEUED        16

typedef enum {
	NBAF_DATALINK = 0,
	NBAF_NETWORK,
	NBAF_TRANSPORT,
	NBAF_APPLICTION,
} netbuf_type_t;

#define NBAF_DATALINK_MASK (1 << NBUF_DATALINK_ALLOC)
#define NBAF_NETWORK_MASK (1 << NBUF_NETWORK_ALLOC)
#define NBAF_TRANSPORT_MASK (1 << NBUF_TRANSPORT_ALLOC)
#define NBAF_APPLICTION_MASK (1 << NBUF_APPLICATION_ALLOC)

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
	uint32_t flags;
	uint32_t sequence_end;
};

CDECL
static inline int netbuf_test_flag(struct netbuf *nb, unsigned int num)
{
	register uint32_t mask;

	mask = nb->flags >> num;
	mask &= 1UL;
	return mask == 1UL;
}

static inline void netbuf_clear_flag(struct netbuf *nb, unsigned int num)
{
	register uint32_t mask;

	mask = 1UL << num;
	nb->flags &= ~mask;
}

static inline int netbuf_test_and_clear_flag(struct netbuf *nb, unsigned int num)
{
	register uint32_t mask;
	register uint32_t old;

	mask = 1UL << num;
	old = nb->flags & mask;
	nb->flags &= ~mask;

	return old >> num;
}

static inline int netbuf_test_and_set_flag(struct netbuf *nb, unsigned num)
{
	register uint32_t mask;
	register uint32_t old;

	mask = 1UL << num;
	old = nb->flags & mask;
	nb->flags |= mask;

	return old >> num;
}

static inline void netbuf_set_flag(struct netbuf *nb, unsigned int num)
{
	register uint32_t mask;

	mask = 1UL << num;
	nb->flags |= mask;
}

static inline void netbuf_set_dev(struct netbuf *nb, struct netdev *dev)
{
	nb->dev = dev;
}

static inline int netbuf_dropped(struct netbuf *nb)
{
	return netbuf_test_flag(nb, NBUF_DROPPED);
}

static inline void netbuf_set_dropped(struct netbuf *nb)
{
	nb->flags |= 1 << NBUF_DROPPED;
	nb->flags |= 1 << NBUF_ARRIVED;
}

static inline int netbuf_arrived(struct netbuf *nb)
{
	return netbuf_test_flag(nb, NBUF_ARRIVED) && !netbuf_test_flag(nb, NBUF_DROPPED);
}

extern DLL_EXPORT struct netbuf *netbuf_realloc(struct netbuf *nb, netbuf_type_t type, size_t size);
extern DLL_EXPORT struct netbuf *netbuf_alloc(netbuf_type_t type, size_t size);
extern DLL_EXPORT void netbuf_free(struct netbuf *nb);
extern DLL_EXPORT void netbuf_cpy_data(struct netbuf *nb, const void *src,
	size_t length, netbuf_type_t type);
extern DLL_EXPORT size_t netbuf_get_size(struct netbuf *nb);
extern DLL_EXPORT size_t netbuf_calc_size(struct netbuf *nb);
extern DLL_EXPORT struct netbuf *netbuf_clone(struct netbuf *nb, uint32_t layers);
extern DLL_EXPORT void netbuf_cpy_data_offset(struct netbuf *nb, size_t ofs, const void *src,
												size_t length, netbuf_type_t type);
extern DLL_EXPORT void netbuf_free_partial(struct netbuf *nb, netbuf_type_t type);
CDECL_END

#endif //!__NETBUF_H__
