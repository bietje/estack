/*
 * Ethernet unit test
 *
 * Author: Michel Megens
 * Date:   29/11/2017
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

	getchar();
	exit(code);
}

#define IPV4_ADDR 0xC0A83268
#define HW_ADDR {0x48, 0x5D, 0x60, 0xBF, 0x51, 0xA1}

static struct netbuf *build_dummy_frame(const char *data, struct netdev *dev)
{
	struct netbuf *nb;
	int len;

	len = strlen(data);

	nb = netbuf_alloc(NBAF_NETWORK, strlen(data));
	netbuf_set_dev(nb, dev);

	memcpy(nb->network.data, data, len);
	nb->network.size = len;
	nb->protocol = ETH_TYPE_IP;

	return nb;
}

#define SAMPLE_DATA "This is some sample data. This data is used in a dummy packet " \
                    "for testing purposes."

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

	dev = pcapdev_create(input, "ethernet-output.pcap", hwaddr, 1500);
	netdev_config_params(dev, 30, 15000);
	pcapdev_create_link_ip4(dev, 0x9131060C, 0, 0xFFFFC000);

	ethernet_output(build_dummy_frame(SAMPLE_DATA, dev), dev->hwaddr);
	netdev_poll(dev);
	printf("Backlog size: %i\n", dev->backlog.size);
	putchar('\n');
	netdev_print(dev, stdout);

	getchar();
	return 0;
}
