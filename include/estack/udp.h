/*
 * E/STACK - UDP implementation
 *
 * Author: Michel Megens
 * Date:   19/12/2017
 * Email:  dev@bietje.net
 */

#ifndef __ESTACK_UDP_H__
#define __ESTACK_UDP_H__

#include <stdint.h>

#include <estack/estack.h>
#include <estack/netdev.h>
#include <estack/netbuf.h>
#include <estack/addr.h>

#pragma pack(push, 1)
struct DLL_EXPORT udp_header {
	uint16_t sport, dport;
	uint16_t length, csum;
};
#pragma pack(pop)

CDECL
extern DLL_EXPORT void udp_input(struct netbuf *nb);
extern DLL_EXPORT uint16_t udp_get_remote_port(struct netbuf *nb);
extern DLL_EXPORT void udp_output(struct netbuf *nb, ip_addr_t *daddr, uint16_t rport, uint16_t lport);
CDECL_END

#endif
