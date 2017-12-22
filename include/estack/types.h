/*
 * E/STACK - Type wrapper header
 *
 * Author: Michel Megens
 * Date: 15/12/2017
 * Email: dev@bietje.net
 */

#ifndef __ESTACK_TYPES_H__
#define __ESTACK_TYPES_H__

#include "config.h"

#ifdef HAVE_GENERIC_SYS
typedef unsigned long long time_t;
#else
#include <stdint.h>
#include <time.h>
#endif

#endif
