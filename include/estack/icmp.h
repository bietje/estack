/*
 * E/STACK - ICMP header
 *
 * Author: Michel Megens
 * Date:   18/12/2017
 * Email:  dev@bietje.net
 */

#ifndef __ICMP_H__
#define __ICMP_H__

#include <stdlib.h>
#include <stdint.h>

#include <estack/estack.h>
#include <estack/netbuf.h>

#pragma pack(push, 1)
struct icmp_header {
	uint8_t type;
	uint8_t code;
	uint16_t csum;
	uint32_t spec;
};
#pragma pack(pop)

typedef enum {
	ICMP_REPLY = 0,
	ICMP_UNREACH = 3,
	ICMP_SOURCEQUENCH,
	ICMP_REDIRECT,
	ICMP_ECHO = 8,
} icmp_type_t;

typedef enum {
	ICMP_UNREACH_NET = 0,
	ICMP_UNREACH_HOST,
	ICMP_UNREACH_PROTO,
	ICMP_UNREACH_PORT,
	ICMP_UNREACH_NEEDFRAG,
	ICMP_UNREACH_SRCFAIL,
	ICMP_UNREACH_NETUNKOWN,
	ICMP_UNREACH_HOSTUNKOWN,
} icmp_code_t;

CDECL
extern DLL_EXPORT void icmp_input(struct netbuf *nb);
extern DLL_EXPORT void icmp_output(uint8_t type, uint32_t dst, struct netbuf *nb);
extern DLL_EXPORT int icmp_reply(uint8_t type, uint8_t code, uint32_t spec, uint32_t dst, struct netbuf *nb);
extern DLL_EXPORT int icmp_response(uint8_t type, uint8_t code, uint32_t spec, struct netbuf *nb);
CDECL_END

#endif
