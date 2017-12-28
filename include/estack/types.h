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

#ifdef HAVE_TIME_H
#include <time.h>
#else
typedef unsigned long long time_t;
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#endif
