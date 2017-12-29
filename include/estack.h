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
extern DLL_EXPORT void devcore_init(void);
extern DLL_EXPORT void route4_init(void);
extern DLL_EXPORT void route4_destroy(void);
extern DLL_EXPORT void devcore_destroy(void);

static inline void estack_init(FILE *log)
{
	log_init(log);
	route4_init();
	devcore_init();
}

static inline void estack_destroy(void)
{
	devcore_destroy();
	route4_destroy();
}
CDECL_END

#endif // !__ESTACK_BASE_HDR__

