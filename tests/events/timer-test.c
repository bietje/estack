/*
 * Timer unit test
 *
 * Author: Michel Megens
 * Date:   16/01/2018
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

static void timer_cb(estack_timer_t *timer, void *arg)
{
	static int tmr = 0;

	tmr++;
	print_dbg("Timer triggered!\n");

	if(tmr == 4)
		estack_timer_set_period(timer, 1000);

	if(tmr == 8)
		estack_timer_stop(timer);
}

static void timer_task(void *arg)
{
	estack_timer_t timer1;

	estack_init(NULL);
	estack_timer_create(&timer1, "timer1", 500, 0, NULL, timer_cb);
	estack_timer_start(&timer1);

	estack_sleep(8500);
	print_dbg("Deleting timer: %i\n", estack_timer_destroy(&timer1));
	estack_destroy();

	print_dbg("Timer test finished\n");
	vTaskEndScheduler();
}

int main(int argc, char **argv)
{
	estack_thread_t tp1;

	tp1.name = "wait-tsk";

	printf("Timer test started\n");
	estack_thread_create(&tp1, timer_task, NULL);
	vTaskStartScheduler();

#ifndef HAVE_RTOS
	estack_sleep(9000);
#endif

	wait_close();
	estack_thread_destroy(&tp1);
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
