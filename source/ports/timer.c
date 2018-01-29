/*
 * E/STACK timer implementation
 * 
 * Author: Michel Megens
 * Date:   16/01/2018
 * Email:  dev@bietje.net
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <assert.h>
#include <estack.h>

#include <estack/error.h>
#include <estack/list.h>

static struct list_head timers = STATIC_INIT_LIST_HEAD(timers);
static estack_mutex_t timer_lock;
static estack_thread_t timer_thread;
static volatile bool running;

static inline void timers_lock(void)
{
	estack_mutex_lock(&timer_lock, 0);
}

static inline void timers_unlock(void)
{
	estack_mutex_unlock(&timer_lock);
}

static void timer_thread_handle(void *arg)
{
	struct list_head *entry, *tmp;
	estack_timer_t *timer;
	time_t now;

	UNUSED(arg);

	while(true) {
		timers_lock();

		if(unlikely(!running)) {
			timers_unlock();
			break;
		}

		now = estack_utime();
		list_for_each_safe(entry, tmp, &timers) {
			timer = list_entry(entry, struct timer, entry);
			if(now >= timer->expiry) {
				timers_unlock();
				timer->handle(timer, timer->arg);
				timers_lock();

				if(timer->state == TIMER_RUNNING && timer->oneshot) {
					list_del(entry);
					timer->state = TIMER_STOPPED;
				} else {
					timer->expiry = now + timer->tmo;
				}
			}
		}
		timers_unlock();

		estack_sleep(1);
	}
}

void estack_timers_init(void)
{
	estack_mutex_create(&timer_lock, 0);

	timer_thread.name = "timer-thread";
	timers_lock();
	running = true;
	timers_unlock();
	estack_thread_create(&timer_thread, timer_thread_handle, NULL);
}

void estack_timers_destroy(void)
{
	timers_lock();
	running = false;
	timers_unlock();
	estack_thread_destroy(&timer_thread);
	estack_mutex_destroy(&timer_lock);
}

void estack_timer_create(estack_timer_t *timer, const char *name, int ms,
	uint32_t flags, void *arg, void (*cb)(estack_timer_t *timer, void *arg))
{
	timer->handle = cb;
	timer->tmo = ms * 1000U;
	timer->arg = arg;

	if(flags & TIMER_ONSHOT_FLAG)
		timer->oneshot = true;
	else
		timer->oneshot = false;

	list_head_init(&timer->entry);
	timer->state = TIMER_CREATED;
}

int estack_timer_start(estack_timer_t *timer)
{
	if(timer->state == TIMER_RUNNING)
		return -EINVALID;

	timers_lock();
	timer->expiry = estack_utime() + timer->tmo;
	timer->state = TIMER_RUNNING;
	list_add(&timer->entry, &timers);
	timers_unlock();

	return -EOK;
}

int estack_timer_stop(estack_timer_t *timer)
{
	timers_lock();
	if(timer->state != TIMER_RUNNING) {
		timers_unlock();
		return -EINVALID;
	}

	timer->state = TIMER_STOPPED;
	list_del(&timer->entry);
	timers_unlock();

	return -EOK;
}

int estack_timer_destroy(estack_timer_t *timer)
{
	estack_timer_stop(timer);
	return -EOK;
}

bool estack_timer_is_running(estack_timer_t *timer)
{
	assert(timer);
	return timer->state == TIMER_RUNNING;
}

int estack_timer_set_period(estack_timer_t *timer, int ms)
{
	assert(timer);
	assert(ms);

	timers_lock();
	if(timer->state != TIMER_CREATED && timer->state != TIMER_RUNNING) {
		timers_unlock();
		return -EINVALID;
	}

	timer->tmo = ms * 1000U;
	timers_unlock();

	return -EOK;
}
