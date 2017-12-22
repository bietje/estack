/*
 * IP test unit test
 *
 * Author: Michel Megens
 * Date:   07/12/2017
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

#define HW_ADDR1 {0xf0, 0xf7, 0x55, 0xbd, 0xbe, 0x40}
static const uint8_t hw1[] = HW_ADDR1;

#define HW_ADDR {0x48, 0x5D, 0x60, 0xBF, 0x51, 0xA9}

static void test_setup_routes(struct netdev *dev)
{
	uint32_t addr, mask, gw;

	addr = ipv4_atoi("145.49.33.186");
	mask = ipv4_atoi("255.255.192.0");
	gw = ipv4_atoi("145.49.63.254");
	route4_add(addr & mask, mask, 0, dev);
	route4_add(0, 0, gw, dev);
}

int main(int argc, char **argv)
{
	char *input;
	struct netdev *dev;
	const uint8_t hwaddr[] = HW_ADDR;
	uint32_t addr;

	if (argc < 2) {
		err_exit(-EXIT_FAILURE, "Usage: %s <input-file>\n", argv[0]);
	} else {
		input = argv[1];
	}

	estack_init(NULL);

	dev = pcapdev_create(input, "icmp-output.pcap", hwaddr, 1500);
	netdev_config_params(dev, 30, 15000);
	pcapdev_create_link_ip4(dev, ipv4_atoi("145.49.33.186"), 0, 0xFFFFC000);

	addr = ipv4_atoi("145.49.63.254");
	netdev_add_destination(dev, hw1, ETHERNET_MAC_LENGTH, (void*)&addr, 4);
	test_setup_routes(dev);

	netdev_poll(dev);

	putchar('\n');
	netdev_print(dev, stdout);
	wait_close();
	route4_clear();
	pcapdev_destroy(dev);
	return 0;
}