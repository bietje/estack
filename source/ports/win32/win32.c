/*
 * E/STACK - WIN32 port
 *
 * Author: Michel Megens
 * Date:   18/12/2017
 * Email:  dev@bietje.net
 *
 * This file contains a windows port of the E/STACK system
 * handles. These handles are wrappers around WINAPI. If you dislike
 * WINAPI as much as I do, get yourself a bucket now, puking is inevitable.
 * YOU HAVE BEEN WARNED.
 */

#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>
#include <estack.h>

#include <estack/error.h>

#include <WinSock2.h>
#include <Windows.h>

#define WIN32_EPOCH_ADJUSTMENT 11644473600000000ULL
time_t estack_utime(void)
{
	time_t rv;
	FILETIME ft;

	GetSystemTimeAsFileTime(&ft);
	rv = ft.dwHighDateTime;
	rv <<= 32;
	rv |= ft.dwLowDateTime;
	rv /= 10;

	return rv - WIN32_EPOCH_ADJUSTMENT;
}

/*
 * THREAD FUNCTIONS
 */

static DWORD WINAPI EStackThreadStarter(LPVOID lpParam)
{
	estack_thread_t *tp;

	tp = (estack_thread_t*)lpParam;
	tp->handle(tp->arg);
	return TRUE;
}

int estack_thread_create(estack_thread_t *tp, thread_handle_t handle, void *arg)
{
	assert(tp);
	assert(handle);

	tp->handle = handle;
	tp->arg = arg;
	tp->tp = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)EStackThreadStarter,
		tp,
		0,
		&tp->tid
	);

	if(!tp->tp) {
		print_dbg("Couldn't create thread!\n");
		return -EINVALID;
	}

	return -EOK;
}

int estack_thread_destroy(estack_thread_t *tp)
{
	assert(tp);

	WaitForSingleObject(tp->tp, INFINITE);
	CloseHandle(tp->tp);
	tp->tp = NULL;
	tp->arg = NULL;
	tp->tid = 0;

	return -EOK;
}

/*
 * MUTEX FUNCTIONS
 */

int estack_mutex_create(estack_mutex_t *mtx, const uint32_t flags)
{
	assert(mtx);

	if(flags & MTX_RECURSIVE) {
		print_dbg("Recursive mutexes not supported on this platform!\n");
	}

	mtx->mtx = CreateMutex(NULL, FALSE, NULL);
	if(!mtx->mtx) {
		print_dbg("Couldn't create mutex object\n");
		return -EINVALID;
	}

	return -EOK;
}

int estack_mutex_destroy(estack_mutex_t *mtx)
{
	assert(mtx);

	CloseHandle(mtx->mtx);
	return -EOK;
}

int estack_mutex_lock(estack_mutex_t *mtx, int tmo)
{
	DWORD result;
	int rv;

	if(tmo)
		result = WaitForSingleObject(mtx->mtx, tmo);
	else
		result = WaitForSingleObject(mtx->mtx, INFINITE);

	switch(result) {
	case WAIT_OBJECT_0:
		rv = -EOK;
		break;

	case WAIT_TIMEOUT:
		print_dbg("Mutex unlock timeout!\n");
		rv = -EINVALID;
		break;

	case WAIT_ABANDONED:
	default:
		print_dbg("Unable to lock mutex! (Error code: %u)\n", result);
		rv = -EINVALID;
		break;
	}

	return rv;
}

void estack_mutex_unlock(estack_mutex_t *mtx)
{
	if(!ReleaseMutex(mtx->mtx))
		print_dbg("Unable to release mutex!\n");
}

void estack_sleep(int ms)
{
	Sleep(ms);
}
