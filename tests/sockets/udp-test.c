/**
 * E/STACK - UDP socket test
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

#include <estack/inet.h>
#include <estack/test.h>
#include <estack/ethernet.h>
#include <estack/pcapdev.h>
#include <estack/socket.h>
#include <estack/route.h>

#define HW_ADDR1 {0x00, 0x00, 0x5e, 0x00, 0x01, 0x31}
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

	addr = ipv4_atoi("145.49.6.12");
	mask = ipv4_atoi("255.255.192.0");
	gw = ipv4_atoi("145.49.63.254");
	route4_add(addr & mask, mask, 0, dev);
	route4_add(0, 0, gw, dev);
}

static void socket_task(void *arg)
{
	uint8_t buf[3400];
	int fd;
#ifdef HAVE_RTOS
	struct netdev *dev = arg;
#endif
	struct sockaddr_in addr;

	fd = estack_socket(PF_INET, SOCK_DGRAM, 0);
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(53);
	addr.sin_addr.s_addr = htonl(ipv4_atoi("145.48.16.26"));
	assert(estack_connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0);
	estack_recv(fd, buf, sizeof(buf), 0);
	estack_close(fd);

	assert(buf[3399] == 0xBF);

#ifdef HAVE_RTOS
	pcapdev_next_src(dev);
	estack_sleep(300);
#endif
	vTaskEndScheduler();
}

int main(int argc, char **argv)
{
	struct netdev *dev;
	const uint8_t hwaddr[] = HW_ADDR;
	uint32_t addr;
	estack_thread_t tp;

	if (argc < 2) {
		err_exit(-EXIT_FAILURE, "Usage: %s <input-file>\n", argv[0]);
	}

	estack_init(stdout);
	dev = pcapdev_create((const char**) argv+1, 2, "udptest-output.pcap", hwaddr, 1500);
	netdev_config_params(dev, 30, 15000);
	pcapdev_create_link_ip4(dev, 0x9131060C, 0, 0xFFFFC000);

	addr = ipv4_atoi("145.49.63.254");
	netdev_add_destination(dev, hw1, ETHERNET_MAC_LENGTH, (void*)&addr, 4);
	test_setup_routes(dev);

	tp.name = "sock-tsk";

	estack_thread_create(&tp, socket_task, dev);

#ifdef HAVE_RTOS
	vTaskStartScheduler();
#else
	estack_sleep(500);
	pcapdev_next_src(dev);
	estack_sleep(1000);
#endif

	estack_thread_destroy(&tp);
	netdev_print(dev, stdout);
	route4_clear();
	pcapdev_destroy(dev);
	estack_destroy();
	wait_close();
	return -EXIT_SUCCESS;
}
