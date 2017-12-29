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
