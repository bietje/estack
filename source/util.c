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

#include <estack/estack.h>

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
