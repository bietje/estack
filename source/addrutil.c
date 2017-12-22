/*
 * E/STACK utilities
 *
 * Author: Michel Megens
 * Date: 03/12/2017
 * Email: dev@bietje.net
 */

#define _CRT_SECURE_NO_WARNINGS 1

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <estack/estack.h>
#include <estack/ethernet.h>
#include <estack/inet.h>

char *ethernet_mac_ntoa(uint8_t *addr, char *buf, size_t length)
{
	assert(addr);
	assert(buf);

	if(length < 18)
		return NULL;

	snprintf(buf, length, "%02x:%02x:%02x:%02x:%02x:%02x",
		addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

	buf[17] = '\0';

	return buf;
}

char *ipv4_ntoa(uint32_t ip, char *buf, size_t length)
{
	uint8_t bytes[4];

	bytes[0] = ip & 0xFF;
	bytes[1] = (ip >> 8) & 0xFF;
	bytes[2] = (ip >> 16) & 0xFF;
	bytes[3] = (ip >> 24) & 0xFF;

	snprintf(buf, length, "%d.%d.%d.%d", bytes[3], bytes[2], bytes[1], bytes[0]);
	return buf;
}

uint32_t ipv4_atoi(char *addr)
{
	uint8_t bytes[4];
	uint32_t ip;

	sscanf(addr, "%hhu.%hhu.%hhu.%hhu", &bytes[3], &bytes[2], &bytes[1], &bytes[0]);
	ip = ((uint32_t)bytes[3] << 24) | ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[1] << 8) | bytes[0];
	return ip;
}

uint32_t ipv4_ptoi(const uint8_t *ary)
{
	uint32_t *num;

	num = (void*)ary;
	return *num;
}
