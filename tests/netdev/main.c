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

#include <estack/estack.h>
#include <estack/netdev.h>
#include <estack/netbuf.h>
#include <estack/ethernet.h>

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

static void setup_dst_cache(void)
{
	netdev_add_destination(hw1, ETHERNET_MAC_LENGTH, (void*)&addr1, 4);
	netdev_add_destination(hw2, ETHERNET_MAC_LENGTH, (void*)&addr2, 4);
	netdev_add_destination(hw3, ETHERNET_MAC_LENGTH, (void*)&addr3, 4);
}

static void test_dst_cache(void)
{
	struct dst_cache_entry *entry;

	entry = netdev_find_destination((void*)&addr2, 4);
	assert(!memcmp(entry->hwaddr, hw2, ETHERNET_MAC_LENGTH));

	entry = netdev_find_destination((void*)&addr1, 4);
	assert(!memcmp(entry->hwaddr, hw1, ETHERNET_MAC_LENGTH));

	entry = netdev_find_destination((void*)&addr3, 4);
	assert(!memcmp(entry->hwaddr, hw3, ETHERNET_MAC_LENGTH));

	entry = netdev_find_destination((void*)&addr3, 4);
	assert(memcmp(entry->hwaddr, hw1, ETHERNET_MAC_LENGTH));

	entry = netdev_find_destination((void*)&addr4, 4);
	assert(!entry);
}

int main(int argc, char **argv)
{
	UNUSED(argc);
	UNUSED(argv);

	setup_dst_cache();
	test_dst_cache();

	printf("Netdev test succesful!\n");
	getchar();

	return -EXIT_SUCCESS;
}
