/*
 * E/STACK logging header
 *
 * Author: Michel Megens
 * Date: 04/12/2017
 * Email: dev@bietje.net
 */

#ifndef __LOG_H__
#define __LOG_H__

#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#include <estack/estack.h>

#ifdef HAVE_DEBUG
CDECL
extern DLL_EXPORT void log_init(FILE *file);
extern DLL_EXPORT void print_dbg(const char *fmt, ...);
CDECL_END
#else
#define log_init(x)
#endif

CDECL
extern DLL_EXPORT void panic(const char *fmt, ...);
CDECL_END

#endif // !__LOG_H__
