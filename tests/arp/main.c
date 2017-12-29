/*
 * ARP unit test
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

#include <estack/netbuf.h>
#include <estack/ethernet.h>
#include <estack/netdev.h>
#include <estack/pcapdev.h>
#include <estack/error.h>
#include <estack/list.h>
#include <estack/arp.h>
#include <estack/test.h>

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

static void print_dst_cache(struct netdev *dev)
{
	struct dst_cache_entry *e;
	struct list_head *entry;
	char ipbuf[16];
	char hwbuf[18];

	printf("Destination cache entries:\n");
	estack_mutex_lock(&dev->mtx, 0);
	list_for_each(entry, &dev->destinations) {
		e = list_entry(entry, struct dst_cache_entry, entry);

		printf("\tEntry:\n");
		printf("\t\tSource IP: %s\n", ipv4_ntoa(ipv4_ptoi(e->saddr), ipbuf, 16));
		printf("\t\tHardware address: %s\n", ethernet_mac_ntoa(e->hwaddr, hwbuf, 18));
	}
	estack_mutex_unlock(&dev->mtx);
}

#define IPV4_ADDR 0xC0A83268
#define HW_ADDR {0x48, 0x5D, 0x60, 0xBF, 0x51, 0xA1}

int main(int argc, char **argv)
{
	char *input;
	struct netdev *dev;
	const uint8_t hwaddr[] = HW_ADDR;

	if (argc < 2) {
		err_exit(-EXIT_FAILURE, "Usage: %s <input-file>\n", argv[0]);
	} else {
		input = argv[1];
	}

	estack_init(NULL);

	dev = pcapdev_create(input, "arp-output.pcap", hwaddr, 1500);
	netdev_config_params(dev, 30, 15000);
	pcapdev_create_link_ip4(dev, 0x9131060C, 0, 0xFFFFC000);

	/* Lets ask where 145.49.6.13 is */
	arp_ipv4_request(dev, 0x9131060D);

	estack_sleep(500);
	putchar('\n');
	netdev_print(dev, stdout);
	putchar('\n');
	print_dst_cache(dev);

	pcapdev_destroy(dev);
	estack_destroy();

	wait_close();
	return 0;
}

