/*
 * E/Stack header
 *
 * Author: Michel Megens
 * Date: 03/12/2017
 * Email: dev@bietje.net
 */

#ifndef __ESTACK_BASE_HDR__
#define __ESTACK_BASE_HDR__

#include <estack/estack.h>
#include <estack/log.h>


CDECL
extern DLL_EXPORT void devcore_init(void);
extern DLL_EXPORT void route4_init(void);
extern DLL_EXPORT void route4_destroy(void);
extern DLL_EXPORT void devcore_destroy(void);
extern DLL_EXPORT void socket_api_init(void);
extern DLL_EXPORT void socket_api_destroy(void);
extern DLL_EXPORT void estack_init(const FILE *output);
extern DLL_EXPORT void estack_destroy(void);

CDECL_END

#endif // !__ESTACK_BASE_HDR__

