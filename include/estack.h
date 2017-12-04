/*
 * E/Stack header
 *
 * Author: Michel Megens
 * Date: 03/12/2017
 * Email: dev@bietje.net
 */

#ifndef __ESTACK_BASE_HDR__
#define __ESTACK_BASE_HDR__

#include <stdio.h>

#include <estack/estack.h>
#include <estack/log.h>

CDECL
static inline void estack_init(FILE *log)
{
	log_init(log);
}
CDECL_END

#endif // !__ESTACK_BASE_HDR__

