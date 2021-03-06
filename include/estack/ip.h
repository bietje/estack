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

#define IPV4_ADDR_SIZE 4
#define IPV6_ADDR_SIZE 16

#define IP4_DONT_FRAGMENT_FLAG  1
#define IP4_MORE_FRAGMENTS_FLAG 0

#pragma pack(push, 1)
struct ipv4_header {
	uint8_t ihl_version;
	uint8_t tos;
	uint16_t length;
	uint16_t id;
	uint16_t offset;
	uint8_t ttl;
	uint8_t protocol;
	uint16_t chksum;
	uint32_t saddr;
	uint32_t daddr;
};
#pragma pack(pop)

#define IS_MULTICAST(x) false
#define IPV4_TTL 0x40

typedef enum {
	IP_PROTO_ICMP = 1,
	IP_PROTO_IGMP,
	IP_PROTO_TCP = 6,
	IP_PROTO_UDP = 17,
	IP_PROTO_RAW = 255
} ipproto_type_t;

CDECL
extern DLL_EXPORT void ip_input(struct netbuf *nb);
extern DLL_EXPORT void ipv4_input(struct netbuf *nb);
extern DLL_EXPORT void ipv4_output(struct netbuf *nb, uint32_t dst);
extern DLL_EXPORT void __ipv4_output(struct netbuf *nb, uint32_t dst);

extern DLL_EXPORT uint16_t ip_checksum_partial(uint16_t start, const void *buf, int len);
extern uint16_t DLL_EXPORT ip_checksum(uint16_t start, const void *buf, int len);
extern DLL_EXPORT uint16_t ipv4_inet_csum(const void *start, uint16_t length,
											uint32_t saddr, uint32_t daddr, uint8_t proto);

extern DLL_EXPORT void ipfrag4_add_packet(struct netbuf *nb);
extern DLL_EXPORT void ipv4_input_postfrag(struct netbuf *nb);
extern DLL_EXPORT void ipfrag4_tmo(void);
extern DLL_EXPORT void ipfrag4_fragment(struct netbuf *nb, uint32_t dst);
extern DLL_EXPORT void ip_htons(struct netbuf *nb);
extern DLL_EXPORT uint32_t ipv4_pseudo_partial_csum(uint32_t saddr, uint32_t daddr, uint8_t proto, uint16_t length);

static inline bool ip_is_ipv4(struct netbuf *nb)
{
	struct ipv4_header *hdr;
	uint8_t version;

	hdr = nb->network.data;
#ifdef HAVE_BIG_ENDIAN
	version = hdr->version & 0xF;
#else
	version = (hdr->ihl_version >> 4) & 0xF;
#endif

	return version == 4;
}

static inline uint32_t ipv4_get_saddr(struct ipv4_header *hdr)
{
	return hdr->saddr;
}

static inline uint32_t ipv4_get_remote_address(struct netbuf *nb)
{
	return ipv4_get_saddr(nb->network.data);
}

static inline uint32_t ipv4_get_daddr(struct ipv4_header *hdr)
{
	return hdr->daddr;
}

static inline uint8_t ipv4_get_flags(struct ipv4_header *hdr)
{
	uint16_t offset = hdr->offset;

	offset &= 0xE000;
	offset >>= 13;
	return (uint8_t)offset;
}

static inline uint16_t ipv4_get_offset(struct ipv4_header *hdr)
{
	uint16_t offset = hdr->offset;

	offset &= ~0xE000;
	return offset * 8; /* Offset i stored as 8-byte blocks */
}
CDECL_END

#endif /* !__IP_H__ */