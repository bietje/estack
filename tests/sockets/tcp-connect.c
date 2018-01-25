/**
 * E/STACK - TCP socket test
 *
 * Author: Michel Megens
 * Email:  dev@bietje.net
 * Date:   08/01/2018
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <estack.h>

#include <estack/error.h>
#include <estack/inet.h>
#include <estack/test.h>
#include <estack/ethernet.h>
#include <estack/pcapdev.h>
#include <estack/socket.h>
#include <estack/route.h>
#include <estack/in.h>

#define HW_ADDR1 {0xf0, 0xf7, 0x55, 0xbd, 0xbe, 0x40}
static const uint8_t hw1[] = HW_ADDR1;

#define HW_ADDR {0x48, 0x5D, 0x60, 0xBF, 0x51, 0xA9}

#ifdef WIN32
static void vTaskStartScheduler(void)
{

}

static void  vTaskEndScheduler(void)
{

}
#else
void __attribute__((weak)) vTaskStartScheduler(void)
{

}

void __attribute__((weak)) vTaskEndScheduler(void)
{

}
#endif

#ifdef HAVE_RTOS
void vApplicationMallocFailedHook(void)
{
	fprintf(stderr, "Failed to allocate memory!\n");
}

void vApplicationStackOverflowHook(TaskHandle_t handle, char *name)
{
	fprintf(stderr, "Stack overflow on %s\n", name);
}
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

static void test_setup_routes(struct netdev *dev)
{
	uint32_t addr, mask, gw;

	addr = ipv4_atoi("145.49.61.83");
	mask = ipv4_atoi("255.255.192.0");
	gw = ipv4_atoi("145.49.63.254");
	route4_add(addr & mask, mask, 0, dev);
	route4_add(0, 0, gw, dev);
}

static void socket_task(void *arg)
{
	int fd;
	struct netdev *dev;
	const uint8_t hwaddr[] = HW_ADDR;
	uint32_t ipaddr;
	struct sockaddr_in in;
	const char **argv = arg;

	estack_init(stdout);
	dev = pcapdev_create((const char**) argv+1, 1, "tcptest-output.pcap", hwaddr, 1500);
	netdev_config_params(dev, 30, 15000);
	pcapdev_create_link_ip4(dev, ipv4_atoi("145.49.61.83"), 0, 0xFFFFC000);

	ipaddr = ipv4_atoi("145.49.63.254");
	netdev_add_destination(dev, hw1, ETHERNET_MAC_LENGTH, (void*)&ipaddr, 4);
	test_setup_routes(dev);

	fd = estack_socket(PF_INET, SOCK_STREAM, 0);
	assert(fd >= 0);
	in.sin_addr.s_addr = htonl(ipv4_atoi("80.114.190.241"));
	in.sin_port = htons(80);
	in.sin_family = AF_INET;
	assert(estack_connect(fd, (struct sockaddr*)&in, sizeof(in)) == 0);
	estack_sleep(600);
	pcapdev_next_src(dev);

	estack_sleep(1600);
	estack_close(fd);

	netdev_print(dev, stdout);
	route4_clear();
	pcapdev_destroy(dev);
	estack_destroy();

	vTaskEndScheduler();
}

int main(int argc, char **argv)
{
	estack_thread_t tp;

	if (argc < 2) {
		err_exit(-EXIT_FAILURE, "Usage: %s <input-file>\n", argv[0]);
	}

	tp.name = "sock-tsk";

	estack_thread_create(&tp, socket_task, argv);

#ifdef HAVE_RTOS
	vTaskStartScheduler();
#else
	estack_sleep(2000);
#endif

	estack_thread_destroy(&tp);
	wait_close();
	return -EXIT_SUCCESS;
}
