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

#endif // !__ETHERNET_H__
