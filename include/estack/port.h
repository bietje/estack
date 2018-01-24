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
#include <arch.h>

#include <estack/estack.h>

#ifndef HAVE_MUTEX
#error "Missing mutex definition!"
#endif
#ifndef HAVE_THREAD
#error "Missing thread definition!"
#endif

#ifndef HAVE_EVENT
#error "Missing event definition!"
#endif

#ifndef HAVE_TIMER
#error "Missing timer definition!"
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
extern DLL_EXPORT void estack_sleep(int ms);

extern DLL_EXPORT void estack_event_create(estack_event_t *event, int length);
extern DLL_EXPORT void estack_event_destroy(estack_event_t *e);
extern DLL_EXPORT void estack_event_signal(estack_event_t *event);
extern DLL_EXPORT int estack_event_wait(estack_event_t *event, int tmo);
extern DLL_EXPORT void estack_event_signal_irq(estack_event_t *event);

#define TIMER_ONSHOT_FLAG 0x1
extern DLL_EXPORT void estack_timer_create(estack_timer_t *timer, const char *name, int ms,
	uint32_t flags, void *arg, void (*cb)(estack_timer_t *timer, void *arg));
extern DLL_EXPORT int estack_timer_start(estack_timer_t *timer);
extern DLL_EXPORT int estack_timer_destroy(estack_timer_t *timer);
extern DLL_EXPORT int estack_timer_stop(estack_timer_t *timer);
extern DLL_EXPORT int estack_timer_destroy(estack_timer_t *timer);
extern DLL_EXPORT void estack_timers_init(void);
extern DLL_EXPORT void estack_timers_destroy(void);
extern DLL_EXPORT bool estack_timer_is_running(estack_timer_t *timer);
extern DLL_EXPORT int estack_timer_set_period(estack_timer_t *timer, int ms);

#define FOREVER 0
CDECL_END

#endif
