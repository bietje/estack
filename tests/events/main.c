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

void __attribute__((weak)) vTaskStartScheduler(void)
{

}

void __attribute__((weak)) vTaskEndScheduler(void)
{

}

static void event_task(void *arg)
{
	estack_event_t *e;

	printf("Waiting for event..\n");
	e = (estack_event_t*)arg;
	estack_event_wait(e);
	printf("Event received!\n");

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

	estack_event_create(&event, 10);
	estack_thread_create(&tp2, signal_task, &event);
	estack_thread_create(&tp1, event_task, &event);
	vTaskStartScheduler();

	wait_close();

	estack_thread_destroy(&tp1);
	estack_thread_destroy(&tp2);
	estack_event_destroy(&event);
	return -EXIT_SUCCESS;
}

void vApplicationMallocFailedHook(void)
{
	fprintf(stderr, "Failed to allocate memory!\n");
}

void vApplicationStackOverflowHook(TaskHandle_t handle, char *name)
{
	fprintf(stderr, "Stack overflow on %s\n", name);
}
