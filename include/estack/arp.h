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

typedef enum {
	ARP_OP_RESERVED = 0,
	ARP_OP_REQUEST,
	ARP_OP_REPLY,
} arp_opcode_t;

CDECL
extern DLL_EXPORT void arp_input(struct netbuf *nb);
extern DLL_EXPORT void arp_output(struct netdev *dev, struct netbuf *nb, uint8_t *addr);
extern DLL_EXPORT struct netbuf *arp_alloc_nb_ipv4(uint16_t type, uint32_t ip, uint8_t *mac);
extern DLL_EXPORT void arp_print_info(struct arp_header *hdr, struct arp_ipv4_header *ip4hdr);
extern DLL_EXPORT void arp_ipv4_request(struct netdev *dev, uint32_t addr);
extern DLL_EXPORT struct dst_cache_entry *arp_resolve_ipv4(struct netdev *dev, uint32_t ip);
CDECL_END

#endif // !__ARP_H__
