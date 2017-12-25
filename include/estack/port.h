/*
 * E/STACK - System portability helper
 *
 * Author: Michel Megens
 * Date:   18/12/2017
 * Email:  dev@bietje.net
 */

#ifndef __PORT_H__
#define __PORT_H__

#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <arch.h>

#include <estack/estack.h>

#ifndef HAVE_MUTEX
#error "Missing mutex definition!"
#endif
#ifndef HAVE_THREAD
#error "Missing thread definition!"
#endif

#define MTX_RECURSIVE 1

typedef void (*thread_handle_t)(void *arg);
CDECL
extern DLL_EXPORT time_t estack_utime(void);

extern DLL_EXPORT int estack_thread_create(estack_thread_t *tp, thread_handle_t handle, void *arg);
extern DLL_EXPORT int estack_thread_destroy(estack_thread_t *tp);

extern DLL_EXPORT int estack_mutex_create(estack_mutex_t *mtx, const uint32_t flags);
extern DLL_EXPORT int estack_mutex_destroy(estack_mutex_t *mtx);
extern DLL_EXPORT int estack_mutex_lock(estack_mutex_t *mtx, int tmo);
extern DLL_EXPORT void estack_mutex_unlock(estack_mutex_t *mtx);
CDECL_END

#endif
