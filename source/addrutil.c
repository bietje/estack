/*
 * E/STACK utilities
 *
 * Author: Michel Megens
 * Date: 03/12/2017
 * Email: dev@bietje.net
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <estack/estack.h>
#include <estack/ethernet.h>

char *ethernet_mac_ntoa(uint8_t *addr, char *buf, size_t length)
{
	assert(addr);
	assert(buf);

	if (length < 18)
		return NULL;

	snprintf(buf, length, "%02x:%02x:%02x:%02x:%02x:%02x",
		addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

	buf[17] = '\0';

	return buf;
}

char *ipv4_ntoa(uint32_t addr, char *buf, size_t length)
{
	char inv[3];
	char *rp;
	uint8_t *ap;
	uint8_t rem; 
	uint8_t n;
	uint8_t i;
	size_t len = 0;

	rp = buf;
	ap = (uint8_t *)&addr;
	for (n = 0; n < 4; n++) {
		i = 0;
		do {
			rem = *ap % (uint8_t)10;
			*ap /= (uint8_t)10;
			inv[i++] = (char)('0' + rem);
		} while (*ap);
		while (i--) {
			if (len++ >= length) {
				return NULL;
			}
			*rp++ = inv[i];
		}
		if (len++ >= length) {
			return NULL;
		}
		*rp++ = '.';
		ap++;
	}
	*--rp = 0;
	return buf;
}
