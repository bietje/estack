/*
 * Events unit test
 *
 * Author: Michel Megens
 * Date:   07/01/2018
 * Email:  dev@bietje.net
 */

#include <stdlib.h>
#include <stdio.h>
#include <estack.h>

#include <estack/test.h>

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

static void event_task(void *arg)
{
	estack_event_t *e;

	print_dbg("Waiting for event..\n");
	e = (estack_event_t*)arg;
	estack_event_wait(e, FOREVER);
	print_dbg("Event received!\n");
	print_dbg("Timeout received! (%i)\n", estack_event_wait(e, 1000));

	vTaskEndScheduler();
}

static void signal_task(void *arg)
{
	estack_event_t *e;

	estack_sleep(1500);
	e = (estack_event_t*)arg;
	estack_event_signal(e);
}

int main(int argc, char **argv)
{
	estack_thread_t tp1, tp2;
	estack_event_t event;

	tp1.name = "wait-tsk";
	tp2.name = "signal-tsk";

	estack_init(stderr);

	estack_event_create(&event, 10);
	estack_thread_create(&tp2, signal_task, &event);
	estack_thread_create(&tp1, event_task, &event);
	vTaskStartScheduler();

#ifndef HAVE_RTOS
	estack_sleep(3000);
#endif
	wait_close();

	estack_thread_destroy(&tp1);
	estack_thread_destroy(&tp2);
	estack_event_destroy(&event);
	estack_destroy();
	return -EXIT_SUCCESS;
}

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
