/*
 * E/Stack ethernet header
 *
 * Author: Michel Megens
 * Date: 28/11/2017
 * Email: dev@bietje.net
 */

#ifndef __ETHERNET_H__
#define __ETHERNET_H__

#include <estack/estack.h>
#include <estack/netbuf.h>

typedef enum {
	ETH_TYPE_ARP = 0x806,
	ETH_TYPE_IP  = 0x800,
	ETH_TYPE_IP6 = 0x86DD,
} ethernet_type_t;

#pragma pack(push, 1)
struct DLL_EXPORT ethernet_header {
	uint8_t dest_mac[6];
	uint8_t src_mac[6];
	uint16_t type;
};
#pragma pack(pop)

#define ETHERNET_MAC_LENGTH 6

CDECL
extern DLL_EXPORT void ethernet_input(struct netbuf *nb);
extern DLL_EXPORT void ethernet_output(struct netbuf *nb, uint8_t *hw);
extern DLL_EXPORT bool ethernet_addr_is_broadcast(const uint8_t *addr);

static inline struct ethernet_header *ethernet_nb_to_hdr(struct netbuf *nb)
{
	return nb->datalink.data;
}

static inline uint16_t ethernet_get_type(struct netbuf *nb)
{
	struct ethernet_header *hdr = ethernet_nb_to_hdr(nb);
	return hdr->type;
}
CDECL_END

#endif // !__ETHERNET_H__
