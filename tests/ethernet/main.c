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

#include <estack/estack.h>
#include <estack/netbuf.h>
#include <estack/ethernet.h>
#include <estack/netdev.h>
#include <estack/pcapdev.h>

#include <Windows.h>

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
	return code; /* Shouldn't be reached */
}

#define IPV4_ADDR 0x9130CD1B
#define HW_ADDR {0xF4, 0x6D, 0x04, 0x18, 0xD6, 0x5D}

int main(int argc, char **argv)
{
	char *input;
	struct netdev *dev;
	const char hwaddr[] = HW_ADDR;

	if (argc < 2) {
		err_exit(-EXIT_FAILURE, "Usage: %s <input-file>\n", argv[0]);
	} else {
		input = argv[1];
	}

	dev = pcapdev_create(input, IPV4_ADDR, hwaddr, 1500);
	dev->read(dev, -1);
	printf("Backlog size: %i\n", dev->backlog.size);

	getchar();
	return 0;
}