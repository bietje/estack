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

#ifndef __UNIX_PORT_H__
#define __UNIX_PORT_H__

#include <pthread.h>

#include <estack/list.h>

typedef DLL_EXPORT struct mutex {
	pthread_mutex_t mtx;
#define HAVE_MUTEX
} estack_mutex_t;

typedef DLL_EXPORT struct thread {
	const char *name;
	pthread_t tid;
	void(*handle)(void *arg);
	void  *arg;
#define HAVE_THREAD
} estack_thread_t;

typedef DLL_EXPORT struct event {
	pthread_mutex_t mtx;
	pthread_cond_t cond;
	bool signalled;
	int length;
	int size;
#define HAVE_EVENT
} estack_event_t;

typedef DLL_EXPORT struct timer {
	struct list_head entry;
	void (*handle)(struct timer *timer, void *arg);
	bool oneshot;
	time_t expiry;
	int tmo;
	void *arg;
#define HAVE_TIMER
} estack_timer_t;

#endif
