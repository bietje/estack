/*
 * E/STACK logging
 *
 * Author: Michel Megens
 * Date: 04/12/2017
 * Email: dev@bietje.net
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <estack/estack.h>
#include <estack/log.h>

static FILE *dbg_file;

static void vfprint_dbg(const char *prefix, const char *fmt, va_list va)
{
	fprintf(dbg_file, "%s", prefix);
	vfprintf(dbg_file, fmt, va);
}

#ifdef HAVE_DEBUG
void print_dbg(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vfprint_dbg("[E/STACK]: ", fmt, va);
	va_end(va);
}
#endif

void panic(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vfprint_dbg("[E/STACK PANIC]: ", fmt, va);
	va_end(va);
}

void log_init(FILE *file)
{
	if(file)
		dbg_file = file;
	else
		dbg_file = stdout;
}
