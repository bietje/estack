/*
 * E/STACK - IP header
 *
 * Author: Michel Megens
 * Date: 28/11/2017
 * Email: dev@bietje.net
 */

#ifndef __IP_H__
#define __IP_H__

#include <stdlib.h>
#include <stdint.h>

#include <estack/estack.h>
#include <estack/netbuf.h>

#pragma pack(push, 1)
struct ipv4_header {
	uint8_t ihl_version;
	uint8_t tos;
	short length;
	uint16_t id;
	short offset;
	uint8_t ttl;
	uint8_t protocol;
	uint16_t chksum;
	uint32_t saddr;
	uint32_t daddr;
};
#pragma pack(pop)

#define IS_MULTICAST(x) false

#define INADDR_BCAST 0xFFFFFFFF

CDECL
extern DLL_EXPORT void ip_input(struct netbuf *nb);
extern DLL_EXPORT void ipv4_input(struct netbuf *nb);
extern DLL_EXPORT void ipv4_output(struct netbuf *nb, uint32_t dst);
CDECL_END

#endif /* !__IP_H__ */