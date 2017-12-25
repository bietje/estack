/*
 * E/STACK - WIN32 port
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
#include <pthread.h>

#include <estack/error.h>

/*
 * THREAD FUNCTIONS
 */

static void *unix_thread_starter(void *arg)
{
	struct thread *tp;

	tp = (struct thread *)arg;
	tp->handle(tp->arg);

	return NULL;
}

int estack_thread_create(estack_thread_t *tp, thread_handle_t handle, void *arg)
{
	int rv;

	assert(tp);
	assert(handle);

	tp->arg = arg;
	tp->handle = handle;
	rv = pthread_create(&tp->tid, NULL, unix_thread_starter, tp);

	return rv;
}

int estack_thread_destroy(estack_thread_t *tp)
{
	assert(tp);

	pthread_join(tp->tid, NULL);
	tp->handle = NULL;
	tp->arg = NULL;

	return -EOK;
}

/*
 * MUTEX FUNCTIONS
 */

int estack_mutex_create(estack_mutex_t *mtx, const uint32_t flags)
{
	pthread_mutexattr_t attr;

	assert(mtx);

	pthread_mutexattr_init(&attr);
	if(flags & MTX_RECURSIVE) {
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	} else {
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
	}

	pthread_mutex_init(&mtx->mtx, &attr);
	return -EOK;
}

int estack_mutex_destroy(estack_mutex_t *mtx)
{
	assert(mtx);

	pthread_mutex_destroy(&mtx->mtx);
	return -EOK;
}

int estack_mutex_lock(estack_mutex_t *mtx, int tmo)
{
	assert(mtx);

	pthread_mutex_lock(&mtx->mtx);
	return -EOK;
}

void estack_mutex_unlock(estack_mutex_t *mtx)
{
	assert(mtx);
	pthread_mutex_unlock(&mtx->mtx);
}
