/*
 * E/STACK utilities
 *
 * Author: Michel Megens
 * Date: 03/12/2017
 * Email: dev@bietje.net
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <estack/estack.h>
#include <estack/types.h>
#include <estack/inet.h>

#ifdef HAVE_BIG_ENDIAN
uint16_t htons(uint16_t s)
{
	return s;
}

uint32_t htonl(uint32_t l)
{
	return l;
}

uint16_t ntohs(uint16_t s)
{
	return s;
}

uint32_t ntohl(uint32_t l)
{
	return l;
}
#else
uint16_t htons(uint16_t s)
{
	return ((s & 0xFF) << 8) | ((s & 0xFF00) >> 8);
}

uint32_t htonl(uint32_t l)
{
	uint8_t data[sizeof(l)] = {};

	memcpy(&data[0], &l, sizeof(l));
	return ((uint32_t)data[3] << 0)  |
	       ((uint32_t)data[2] << 8)  |
	       ((uint32_t)data[1] << 16) |
	       ((uint32_t)data[0] << 24);
}

uint16_t ntohs(uint16_t s)
{
	return htons(s);
}

uint32_t ntohl(uint32_t l)
{
	return htonl(l);
}
#endif
