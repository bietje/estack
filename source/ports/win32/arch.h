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

#pragma once

#include <WinSock2.h>
#include <Windows.h>

#include <estack/list.h>

typedef struct DLL_EXPORT mutex {
	HANDLE mtx;
#define HAVE_MUTEX
} estack_mutex_t;

typedef struct DLL_EXPORT thread {
	const char *name;
	HANDLE tp;
	DWORD tid;
	void *arg;
	void(*handle)(void *param);
#define HAVE_THREAD
} estack_thread_t;

typedef struct DLL_EXPORT event {
	CRITICAL_SECTION cs;
	CONDITION_VARIABLE cond;
	bool signalled;
	int size,
		length;
#define HAVE_EVENT
} estack_event_t;

typedef DLL_EXPORT struct timer {
	struct list_head entry;
	void (*handle)(struct timer *timer, void *arg);
	bool oneshot;
	time_t expiry;
	int tmo;
	void *arg;
	timer_state_t state;
#define HAVE_TIMER
} estack_timer_t;
