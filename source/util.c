/*
 * E/STACK utilities
 *
 * Author: Michel Megens
 * Date: 03/12/2017
 * Email: dev@bietje.net
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "config.h"

void *z_alloc(size_t size)
{
	void *vp;

	assert(size > 0);
	vp = malloc(size);

	assert(vp);
	memset(vp, 0, size);
	return vp;
}

#ifdef WIN32
#define WIN32_EPOCH_ADJUSTMENT 11644473600000000ULL
#include <Windows.h>
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
#else
#include <time.h>
time_t estack_utime(void)
{
	time_t rv;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	rv = tv.tv_sec * 1000000ULL;
	rv += tv.tv_usec;
	return rv;
}
#endif // WIN32

