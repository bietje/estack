/*
 * Network pcap device header
 *
 * Author: Michel Megens
 * Date: 29/11/2017
 * Email: dev@bietje.net
 */

#ifndef __PCAP_DEV_H__
#define __PCAP_DEV_H__

#include <stdlib.h>
#include <stdint.h>

#include <estack/estack.h>
#include <estack/netdev.h>

CDECL
extern DLL_EXPORT struct netdev *pcapdev_create(const char *srcfile, const char *dstfile,
	const uint8_t *hwaddr, uint16_t mtu);
extern DLL_EXPORT void pcapdev_create_link_ip4(struct netdev *dev, uint32_t local,
	uint32_t remote, uint32_t mask);
extern DLL_EXPORT void pcapdev_destroy(struct netdev *dev);
extern DLL_EXPORT void pcapdev_set_name(struct netdev *dev, const char *name);
CDECL_END

#endif