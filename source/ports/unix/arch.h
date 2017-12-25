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

typedef DLL_EXPORT struct mutex {
	pthread_mutex_t mtx;
#define HAVE_MUTEX
} estack_mutex_t;

typedef DLL_EXPORT struct thread {
	pthread_t tid;
	void(*handle)(void *arg);
	void  *arg;
#define HAVE_THREAD
} estack_thread_t;

#endif
