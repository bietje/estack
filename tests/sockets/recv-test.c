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

#include <estack/test.h>
#include <estack/ethernet.h>
#include <estack/pcapdev.h>
#include <estack/socket.h>
#include <estack/route.h>

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

static void socket_task(void *arg)
{
	uint8_t buf[210];
	int fd = *(int*)arg;
	int i = 0;

	estack_recv(fd, buf, sizeof(buf), 0);
	for(; i < 150; i++)
		assert(buf[i] == 0x99);

	estack_sleep(500);
	vTaskEndScheduler();
}

static void write_task(void *arg)
{
	uint8_t *data;
	int fd = *(int*)arg;

	data = malloc(150);
	memset(data, 0x99, 150);
	socket_trigger_receive(fd, data, 150);
	free(data);
}

int main(int argc, char **argv)
{
	estack_thread_t tp, tp2;
	int fd;

	estack_init(stdout);
	tp.name = "sock-tsk";
	tp2.name = "wr-tsk";

	fd = estack_socket(PF_INET, SOCK_DGRAM, 0);
	estack_thread_create(&tp, socket_task, &fd);
	estack_thread_create(&tp2, write_task, &fd);

#ifdef HAVE_RTOS
	vTaskStartScheduler();
#else
	estack_sleep(1000);
#endif

	estack_close(fd);
	estack_thread_destroy(&tp);
	estack_thread_destroy(&tp2);
	estack_destroy();
	wait_close();
	return -EXIT_SUCCESS;
}
