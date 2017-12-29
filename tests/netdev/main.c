/*
 * Network device layer unit test
 *
 * Author: Michel Megens
 * Date:   05/12/2017
 * Email:  dev@bietje.net
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <estack.h>

#include <estack/netdev.h>
#include <estack/netbuf.h>
#include <estack/ethernet.h>
#include <estack/pcapdev.h>
#include <estack/inet.h>
#include <estack/translate.h>
#include <estack/neighbour.h>
#include <estack/prototype.h>
#include <estack/test.h>

static int err_exit(int code, const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);

	wait_close();
	exit(code);
}

#define IPV4_ADDR1 0x9130CD1C
#define HW_ADDR1 {0xF4, 0x6D, 0x04, 0x18, 0xD6, 0x5B}
#define IPV4_ADDR2 0x9130CD1F
#define HW_ADDR2 {0xF4, 0x6D, 0x04, 0x22, 0xD6, 0x5B}
#define IPV4_ADDR3 0x9130AD1C
#define HW_ADDR3 {0xF4, 0x6D, 0x14, 0x18, 0xD6, 0x5B}

static const uint8_t hw1[] = HW_ADDR1;
static const uint8_t hw2[] = HW_ADDR3;
static const uint8_t hw3[] = HW_ADDR2;

static uint32_t addr1 = IPV4_ADDR1,
				addr2 = IPV4_ADDR2,
				addr3 = IPV4_ADDR3,
				addr4 = IPV4_ADDR2 - 20;

/* Local address definitions */
#define IPV4_ADDR 0x9130CD1B
#define HW_ADDR {0xF4, 0x6D, 0x04, 0x18, 0xD6, 0x5D}
static struct netdev *dev;

/* Remote address definitions */

/* 145.49.6.13 will eventually resolv to this MAC address */
#define DESTINATION_MAC {0xF4, 0x6D, 0x4, 0x18, 0xD1, 0x10}

#pragma pack(push, 1)
struct iphdr {
#ifdef HAVE_BIGENDIAN
	uint8_t version : 4;
	uint8_t ihl : 4;
#else
	uint8_t ihl_version;
#endif

	uint8_t tos;
	uint16_t len;
	uint16_t id;
	uint16_t frag_offset;
	uint8_t ttl;
	uint8_t proto;
	uint16_t csum;
	uint32_t saddr;
	uint32_t daddr;
};
#pragma pack(pop)

static void generate_ip_datagram(uint32_t ip)
{
	struct netif *nif;
	struct netbuf *nb;
	struct iphdr *hdr;

	nif = &dev->nif;
	nb = netbuf_alloc(NBAF_NETWORK, sizeof(struct iphdr));

	hdr = nb->network.data;

	hdr->ihl_version = (4 << 4) | 5;
	hdr->tos = 0;
	hdr->len = 20;
	hdr->id = 0;
	hdr->proto = 0xFE;
	hdr->saddr = ipv4_ptoi(nif->local_ip);
	hdr->daddr = ip;
	hdr->csum = 0xb71;
	hdr->frag_offset = 0x4000;
	hdr->ttl = 64;

	hdr->len = htons(hdr->len);
	hdr->id = htons(hdr->id);
	hdr->daddr = htonl(hdr->daddr);
	hdr->saddr = htonl(hdr->saddr);
	hdr->csum = htons(hdr->csum);
	hdr->frag_offset = htons(hdr->frag_offset);

	nb->protocol = PROTO_IPV4;
	neighbour_output(dev, nb, &ip, 4 /* address length */, translate_ipv4_to_mac);
}

static void test_cache_timeout(uint32_t ip)
{
	generate_ip_datagram(ip);
	estack_sleep(3700);
}

static void complete_dst_entry(uint32_t ip)
{
	const uint8_t hwaddr[] = DESTINATION_MAC;

	netdev_update_destination(dev, hwaddr, 6, (void*)&ip, 4);
	estack_sleep(250);
}

static void test_cache_notimeout(uint32_t ip)
{
	generate_ip_datagram(ip);
	estack_sleep(1500);

	complete_dst_entry(ip);
	estack_sleep(250);
	generate_ip_datagram(ip);
	estack_sleep(250);
}

static void setup_dst_cache(void)
{
	netdev_add_destination(dev, hw1, ETHERNET_MAC_LENGTH, (void*)&addr1, 4);
	netdev_add_destination(dev, hw2, ETHERNET_MAC_LENGTH, (void*)&addr2, 4);
	netdev_add_destination(dev, hw3, ETHERNET_MAC_LENGTH, (void*)&addr3, 4);
}

static void test_dst_cache(void)
{
	struct dst_cache_entry *entry;

	entry = netdev_find_destination(dev, (void*)&addr2, 4);
	assert(!memcmp(entry->hwaddr, hw2, ETHERNET_MAC_LENGTH));

	entry = netdev_find_destination(dev, (void*)&addr1, 4);
	assert(!memcmp(entry->hwaddr, hw1, ETHERNET_MAC_LENGTH));

	entry = netdev_find_destination(dev, (void*)&addr3, 4);
	assert(!memcmp(entry->hwaddr, hw3, ETHERNET_MAC_LENGTH));

	entry = netdev_find_destination(dev, (void*)&addr3, 4);
	assert(memcmp(entry->hwaddr, hw1, ETHERNET_MAC_LENGTH));

	entry = netdev_find_destination(dev, (void*)&addr4, 4);
	assert(!entry);
}

int main(int argc, char **argv)
{
	const uint8_t hwaddr[] = HW_ADDR;
	const char *input;
	uint32_t ip1, ip2;

	if (argc < 2) {
		err_exit(-EXIT_FAILURE, "Usage: %s <input-file>\n", argv[0]);
	} else {
		input = argv[1];
	}

	estack_init(stdout);
	dev = pcapdev_create(input, "netdev-output.pcap", hwaddr, 1500);
	netdev_config_params(dev, 30, 15000);
	pcapdev_create_link_ip4(dev, 0x9131060C, 0, 0xFFFFC000);

	setup_dst_cache();
	test_dst_cache();

	ip1 = ipv4_atoi("145.49.6.14");
	ip2 = ipv4_atoi("145.49.6.13");
	test_cache_timeout(ip1);
	test_cache_notimeout(ip2);

	putchar('\n');
	estack_sleep(500);
	netdev_print(dev, stdout);

	assert(netdev_get_rx_packets(dev) == 1);
	assert(netdev_get_tx_packets(dev) == 9);
	assert(netdev_get_dropped(dev) == 1);
	pcapdev_destroy(dev);
	estack_destroy();
	wait_close();

	return -EXIT_SUCCESS;
}
