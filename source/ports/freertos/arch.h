/*
 * E/STACK - WIN32 port
 *
 * Author: Michel Megens
 * Date:   18/12/2017
 * Email:  dev@bietje.net
 */

#ifndef __PORT_H__
#error "Do not include this file directly, use \"ports.h\" instead!"
#endif

#ifndef __RTOS_PORT_H__
#define __RTOS_PORT_H__

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

typedef struct thread {
	const char *name;
	void *arg;
	void (*handle)(void *arg);
	TaskHandle_t task;
#define HAVE_THREAD
} estack_thread_t;

typedef struct mutex {
	SemaphoreHandle_t sem;
	bool recursive;
#define HAVE_MUTEX
} estack_mutex_t;

#endif
