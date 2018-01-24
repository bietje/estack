/*
 * E/STACK - RTOS unix port
 *
 * Author: Michel Megens
 * Date:   18/12/2017
 * Email:  dev@bietje.net
 */

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <estack.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

#include <estack/error.h>

static void vPortTaskStarter(void *arg)
{
	struct thread *tp;

	tp = (struct thread *)arg;
	tp->handle(tp->arg);
	tp->task = NULL;
	vTaskDelete(NULL);
}

#define STACK_DEPTH 1024
#define TASK_PRIO 8
int estack_thread_create(estack_thread_t *tp, thread_handle_t handle, void *arg)
{
	BaseType_t bt;

	assert(tp);
	assert(handle);

	tp->handle = handle;
	tp->arg = arg;
	bt = xTaskCreate(vPortTaskStarter, tp->name, STACK_DEPTH, tp, TASK_PRIO, &tp->task);

	if(bt == pdPASS)
		return -EOK;
	
	print_dbg("Could not create task!\n");
	return -EINVALID;
}

int estack_thread_destroy(estack_thread_t *tp)
{
	assert(tp);

	if(tp->task) {
		vTaskDelete(tp->task);
		tp->task = NULL;
	}

	return -EOK;
}

int estack_mutex_create(estack_mutex_t *mtx, const uint32_t flags)
{
	assert(mtx);
	mtx->sem = xSemaphoreCreateMutex();

	if(!mtx->sem)
		return -EINVALID;

	mtx->recursive = !!(flags & MTX_RECURSIVE);
	return -EOK;
}

int estack_mutex_destroy(estack_mutex_t *mtx)
{
	assert(mtx);
	assert(mtx->sem);

	vQueueDelete(mtx->sem);
	return -EOK;
}

int estack_mutex_lock(estack_mutex_t *mtx, int tmo)
{
	bool rv;
	TickType_t ms;

	assert(mtx);
	assert(mtx->sem);

	if(tmo) {
		ms = tmo / portTICK_RATE_MS;

		if(mtx->recursive)
			rv = xSemaphoreTakeRecursive(mtx->sem, ms) == pdTRUE;
		else
			rv = xSemaphoreTake(mtx->sem, ms) == pdTRUE;
	} else {
		if(mtx->recursive)
			while(xSemaphoreTakeRecursive(mtx->sem, portMAX_DELAY) != pdTRUE);
		else
			while(xSemaphoreTake(mtx->sem, portMAX_DELAY) != pdTRUE);

		rv = true;
	}

	return rv ? -EOK : -EINVALID;
}

void estack_mutex_unlock(estack_mutex_t *mtx)
{
	assert(mtx);
	assert(mtx->sem);

	if(mtx->recursive)
		xSemaphoreGiveRecursive(mtx->sem);
	else
		xSemaphoreGive(mtx->sem);
}

void estack_sleep(int ms)
{
	vTaskDelay(ms / portTICK_PERIOD_MS);
}

void estack_event_create(estack_event_t *event, int length)
{
	event->evq = xQueueCreate(length, sizeof(void*));
}

int estack_event_wait(estack_event_t *event, int tmo)
{
	estack_event_t *ev;
	BaseType_t bt;
	int rv;

	if(tmo == FOREVER) {
		while(xQueueReceive(event->evq, (void*)&ev, portMAX_DELAY) != pdTRUE);
		rv = -EOK;
	} else {
		bt = xQueueReceive(event->evq, (void*)&ev, tmo / portTICK_PERIOD_MS);
		rv = bt == pdTRUE ? -EOK : -ETMO;
	}

#ifndef NDEBUG
	if(rv == -EOK)
		assert(ev == event);
#endif

	return rv;
}

void estack_event_signal_irq(estack_event_t *event)
{
	xQueueSendFromISR(&event->evq, &event, (portTickType) 0);
}

void estack_event_signal(estack_event_t *event)
{
	xQueueSend(event->evq, &event, (portTickType) 0);
}

void estack_event_destroy(estack_event_t *e)
{
	vQueueDelete(e->evq);
}

/*
 * TIMER API WRAPPERS
 */

void estack_timers_init(void)
{
}

void estack_timers_destroy(void)
{
}

static void vTimerCallbackHook(TimerHandle_t xTimer)
{
	estack_timer_t *tmr;

	tmr = pvTimerGetTimerID(xTimer);
	tmr->handle(tmr, tmr->arg);

	if(tmr->oneshot)
		tmr->state = TIMER_STOPPED;
}

void estack_timer_create(estack_timer_t *timer, const char *name, int ms,
	uint32_t flags, void *arg, void (*cb)(estack_timer_t *timer, void *arg))
{
	timer->handle = cb;
	timer->period = ms / portTICK_PERIOD_MS;
	timer->arg = arg;

	if(flags & TIMER_ONSHOT_FLAG)
		timer->oneshot = true;
	else
		timer->oneshot = false;

	timer->timer = xTimerCreate(name, ms / portTICK_PERIOD_MS, !timer->oneshot, timer, vTimerCallbackHook);
	timer->created = true;
	timer->state = TIMER_CREATED;
}

int estack_timer_start(estack_timer_t *timer)
{
	BaseType_t bt;

	if(timer->state == TIMER_RUNNING || !timer->created)
		return -EINVALID;

	bt = pdFAIL;
	timer->state = TIMER_RUNNING;

	while(bt != pdPASS)
		bt = xTimerStart(timer->timer, 0);

	return (bt == pdPASS) ? -EOK : -ETMO;
}

int estack_timer_stop(estack_timer_t *timer)
{
	BaseType_t bt;

	if(timer->state != TIMER_RUNNING || !timer->created)
		return -EINVALID;

	bt = pdFAIL;
	while(bt != pdPASS)
		bt = xTimerStop(timer->timer, 0);

	timer->state = TIMER_STOPPED;
	return (bt == pdPASS) ? -EOK : -ETMO;
}

int estack_timer_destroy(estack_timer_t *timer)
{
	BaseType_t bt = pdFAIL;

	if(!timer->created)
		return -EINVALID;

	estack_timer_stop(timer);

	while(bt != pdPASS)
		bt = xTimerDelete(timer->timer, 0);

	timer->state = TIMER_STOPPED;
	timer->created = false;
	return (bt == pdPASS) ? -EOK : -ETMO;
}

bool estack_timer_is_running(estack_timer_t *timer)
{
	return timer->state == TIMER_RUNNING;
}

int estack_timer_set_period(estack_timer_t *timer, int ms)
{
	BaseType_t bt = pdFAIL;

	if(!timer->created)
		return -EINVALID;

	while(bt != pdPASS)
		bt = xTimerChangePeriod(timer->timer, ms / portTICK_PERIOD_MS, 0);

	return -EOK;
}
