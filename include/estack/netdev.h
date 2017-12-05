/*
 * E/STACK netdev header
 *
 * Author: Michel Megens
 * Date: 03/12/2017
 * Email: dev@bietje.net
 */

#ifndef __NETDEV_H__
#define __NETDEV_H__

#include <stdlib.h>
#include <stdint.h>

#include <estack/estack.h>
#include <estack/list.h>

struct DLL_EXPORT netdev_stats {
	uint32_t rx_bytes, rx_packets;
	uint32_t tx_bytes, tx_packets;
	uint32_t dropped;
};

struct DLL_EXPORT netdev_backlog {
	struct list_head head;
	int size;
};

#define MAX_ADDR_LEN 8
#define MAX_LOCAL_ADDRESS_LENGTH 16

struct netif {
	uint8_t dummy;
};

struct netbuf;
typedef void(*xmit_handle)(struct netbuf *nb);

struct protocol {
	struct list_head entry;
	uint16_t protocol;
	xmit_handle rx;
};

struct DLL_EXPORT netdev {	
	const char *name;
	struct list_head entry;
	struct list_head protocols;
	struct list_head destinations;

	uint16_t mtu;
	struct netdev_backlog backlog;
	struct netdev_stats stats;

	struct netif netif;
	uint8_t hwaddr[MAX_ADDR_LEN];
	uint8_t addrlen;

	xmit_handle rx;
	int(*write)(struct netdev *dev, struct netbuf *nb);
	int (*read)(struct netdev *dev, int num);
	int(*available)(struct netdev *dev);
};

struct DLL_EXPORT dst_cache_entry {
	struct list_head entry;

	uint8_t *saddr;
	uint8_t saddr_length;
	uint8_t *hwaddr;
	uint8_t hwaddr_length;
};

CDECL
extern DLL_EXPORT void netdev_add_backlog(struct netdev *dev, struct netbuf *nb);
extern DLL_EXPORT void netdev_init(struct netdev *dev);
extern DLL_EXPORT int netdev_poll(struct netdev *dev);
extern DLL_EXPORT void netdev_demux_handle(struct netbuf *nb);
extern DLL_EXPORT bool netdev_remove_protocol(struct netdev *dev, struct protocol *proto);
extern DLL_EXPORT void netdev_add_destination(struct netdev *dev, const uint8_t *dst,
	uint8_t daddrlen, const uint8_t *src, uint8_t saddrlen);
extern DLL_EXPORT struct dst_cache_entry *netdev_find_destination(struct netdev *dev,
	const uint8_t *src, uint8_t length);
extern DLL_EXPORT bool netdev_remove_destination(struct netdev *dev, const uint8_t *src,
	uint8_t length);
extern DLL_EXPORT bool netdev_update_destination(struct netdev *dev, const uint8_t *dst,
	uint8_t dlength, const uint8_t *src, uint8_t slength);
CDECL_END

#define backlog_for_each_safe(bl, e, p) \
			list_for_each_safe(e, p, &((bl)->head))
#endif // !__NETDEV_H__
