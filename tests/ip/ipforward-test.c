/*
 * IP fragmentation unit test
 *
 * Author: Michel Megens
 * Date:   21/12/2017
 * Email:  dev@bietje.net
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <pcap.h>
#include <estack.h>
#include <string.h>
#include <assert.h>

#include <estack/netbuf.h>
#include <estack/ethernet.h>
#include <estack/netdev.h>
#include <estack/pcapdev.h>
#include <estack/error.h>
#include <estack/list.h>
#include <estack/arp.h>
#include <estack/ip.h>
#include <estack/test.h>
#include <estack/route.h>
#include <estack/inet.h>

#ifdef WIN32
#include <Windows.h>
#endif

#ifndef WIN32
typedef unsigned char u_char;
#endif

static int err_exit(int code, const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);

	wait_close();
	exit(code);
}

#define HW_ADDR1 {0x00, 0x00, 0x5e, 0x00, 0x01, 0x31}
static const uint8_t hw1[] = HW_ADDR1;
#define HW_ADDR2 {0x00, 0x00, 0x6e, 0xB0, 0x01, 0x31}
static const uint8_t hw2[] = HW_ADDR2;

#define HW_ADDR_LOCAL1 {0x48, 0x5D, 0x60, 0xBF, 0x51, 0xA9}
#define HW_ADDR_LOCAL2 {0x00, 0x5D, 0x61, 0xBF, 0x51, 0xB0}

static void test_setup_routes(struct netdev *dev1, struct netdev *dev2)
{
	uint32_t addr, mask, gw;

	addr = ipv4_atoi("80.114.190.241");
	mask = ipv4_atoi("255.255.255.0");
	gw = ipv4_atoi("80.114.190.254");
	route4_add(addr & mask, mask, gw, dev2);

	addr = ipv4_atoi("80.114.180.241");
	mask = ipv4_atoi("255.255.255.0");
	gw = ipv4_atoi("80.114.180.254");
	route4_add(addr & mask, mask, 0, dev1);
	route4_add(0, 0, gw, dev1);

}

static struct netdev *setup_dev(const char *input, const char *output, const uint8_t *hwaddr, uint32_t local, uint32_t mask)
{
	struct netdev *dev;

	dev = pcapdev_create(input, output, hwaddr, 1500);
	netdev_config_params(dev, 30, 15000);
	pcapdev_create_link_ip4(dev, local, 0, mask);
	return dev;
}

int main(int argc, char **argv)
{
	char *input;
	struct netdev *dev1, *dev2;
	const uint8_t hwaddr1[] = HW_ADDR_LOCAL1;
	const uint8_t hwaddr2[] = HW_ADDR_LOCAL2;
	uint32_t addr;

	if (argc < 2) {
		err_exit(-EXIT_FAILURE, "Usage: %s <input-file>\n", argv[0]);
	} else {
		input = argv[1];
	}

	estack_init(NULL);

	dev1 = setup_dev(input, "ipforward-input.pcap", hwaddr1, ipv4_atoi("80.114.180.241"), ipv4_atoi("255.255.255.0"));
	dev2 = setup_dev(NULL, "ipforward-output.pcap", hwaddr2, ipv4_atoi("80.114.190.241"), ipv4_atoi("255.255.255.0"));
	pcapdev_set_name(dev1, "dbg0");
	pcapdev_set_name(dev2, "dbg2");

	addr = ipv4_atoi("80.114.180.254");
	netdev_add_destination(dev1, hw1, ETHERNET_MAC_LENGTH, (void*)&addr, 4);
	addr = ipv4_atoi("80.114.190.254");
	netdev_add_destination(dev2, hw2, ETHERNET_MAC_LENGTH, (void*)&addr, 4);

	test_setup_routes(dev1, dev2);

	netdev_poll(dev1);
	netdev_poll(dev2);
	netdev_print(dev1, stdout);
	putchar('\n');
	netdev_print(dev2, stdout);

	assert(netdev_get_tx_bytes(dev1) == 0);
	assert(netdev_get_rx_bytes(dev1) == 3410);

	assert(netdev_get_tx_bytes(dev2) == 3410);
	assert(netdev_get_tx_bytes(dev1) == 0);

	route4_clear();
	pcapdev_destroy(dev1);
	pcapdev_destroy(dev2);

	wait_close();
	return 0;
}