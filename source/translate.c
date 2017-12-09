/*
 * E/STACK address translation
 *
 * Author: Michel Megens
 * Date: 07/12/2017
 * Email: dev@bietje.net
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <estack/estack.h>
#include <estack/netbuf.h>
#include <estack/arp.h>

void translate_ipv4_to_mac(struct netdev *dev, uint8_t *src)
{
	uint32_t *ip;

	assert(dev);
	assert(src);
	ip = (void*)src;
	arp_ipv4_request(dev, *ip);
}
