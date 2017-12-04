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

extern DLL_EXPORT struct netdev *pcapdev_create(const char *srcfile, uint32_t ip,
	const uint8_t *hwaddr, uint16_t mtu);

#endif