/*
 * E/STACK ARP header
 *
 * Author: Michel Megens
 * Date: 06/12/2017
 * Email: dev@bietje.net
 */

#ifndef __ARP_H__
#define __ARP_H__

#include <stdlib.h>
#include <stdint.h>

#include <estack/estack.h>
#include <estack/netbuf.h>
#include <estack/netdev.h>
#include <estack/ethernet.h>

#pragma pack(push, 1)
struct DLL_EXPORT arp_header {
	uint16_t hwtype;
	uint16_t protocol;
	uint8_t hwsize;
	uint8_t protosize;
	uint16_t opcode;
};

struct DLL_EXPORT arp_ipv4_header {
	uint8_t hw_src_addr[ETHERNET_MAC_LENGTH];
	uint32_t ip_src_addr;
	uint8_t hw_target_addr[ETHERNET_MAC_LENGTH];
	uint32_t ip_target_addr;
};
#pragma pack(pop)

CDECL
extern DLL_EXPORT void arp_input(struct netbuf *nb);
CDECL_END

#endif // !__ARP_H__
