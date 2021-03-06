/*
 * IP fragmentation unit test
 *
 * Author: Michel Megens
 * Date:   19/12/2017
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
#include <estack/addr.h>
#include <estack/udp.h>

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

#define HW_ADDR {0x48, 0x5D, 0x60, 0xBF, 0x51, 0xA9}

struct dummy_pkt {
	uint16_t src, dst;
	uint16_t length, csum;
	uint8_t data[3400];
};

static void test_ipout(struct netdev *ndev, uint32_t addr)
{
	struct netbuf *nb;
	uint8_t *data;
	ip_addr_t dst;

	nb = netbuf_alloc(NBAF_APPLICTION, 3400);
	memset(nb->application.data, 0xAD, 3400);
	data = nb->application.data;
	data[3399] = 0xBF;

	dst.type = IPADDR_TYPE_V4;
	dst.addr.in4_addr.s_addr = addr;

	udp_output(nb, &dst, htons(2100), htons(48720));
}

static void test_setup_routes(struct netdev *dev)
{
	uint32_t addr, mask, gw;

	addr = ipv4_atoi("80.114.190.241");
	mask = ipv4_atoi("255.255.255.0");
	gw = ipv4_atoi("80.114.190.254");
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

	dev = pcapdev_create((const char**)&input, 1, "ipfrag-output.pcap", hwaddr, 1500);
	netdev_config_params(dev, 30, 15000);
	pcapdev_create_link_ip4(dev, ipv4_atoi("80.114.190.241"), 0, ipv4_atoi("255.255.255.0"));

	addr = ipv4_atoi("80.114.190.254");
	netdev_add_destination(dev, hw1, ETHERNET_MAC_LENGTH, (void*)&addr, 4);
	test_setup_routes(dev);
	pcapdev_start(dev);
	test_ipout(dev, addr);

	estack_sleep(1000);
	netdev_print(dev, stdout);

	assert(netdev_get_rx_bytes(dev) == 3410);
	assert(netdev_get_tx_bytes(dev) == 3580);

	route4_clear();
	pcapdev_destroy(dev);
	estack_destroy();

	wait_close();
	return 0;
}

