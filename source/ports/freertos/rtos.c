/*
 * E/STACK - RTOS unix port
 *
 * Author: Michel Megens
 * Date:   18/12/2017
 * Email:  dev@bietje.net
 */

#include <stdlib.h>
#include <stdint.h>
#include <time.h>
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
	vTaskDelete(NULL);
}

#define STACK_DEPTH 1024
#define TASK_PRIO 8
int estack_thread_create(estack_thread_t *tp, thread_handle_t handle, void *arg)
{
	BaseType_t bt;
	TaskHandle_t tsk;

	assert(tp);
	assert(handle);

	tp->handle = handle;
	tp->arg = arg;
	bt = xTaskCreate(vPortTaskStarter, tp->name, STACK_DEPTH, tp, TASK_PRIO, &tsk);
	tp->task = tsk;

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
		return -EOK;
	}

	return -EINVALID;
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