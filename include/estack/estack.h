/*
 * E/Stack header
 *
 * Author: Michel Megens
 * Date: 28/11/2017
 * Email: dev@bietje.net
 */

#ifndef __ESTACK_H__
#define __ESTACK_H__

#include <stdint.h>

#include <estack/types.h>
#include <estack/compiler.h>

#ifndef UNUSED
#define UNUSED(x) (void)x
#endif

/* x86 / x64 definitions */
#ifdef _WIN32
#define ENV32
#endif

#ifdef _WIN64
#define ENV64
#endif

#ifdef __GNUC__
#if defined(__x86_64__) || defined(__ppc64__)
#define ENV64
#else
#define ENV32
#endif
#endif

CDECL
extern DLL_EXPORT void *z_alloc(size_t size);
extern DLL_EXPORT time_t estack_utime(void);

extern DLL_EXPORT char *ipv4_ntoa(uint32_t addr, char *buf, size_t length);
extern DLL_EXPORT uint32_t ipv4_ptoi(const uint8_t *ary);
extern DLL_EXPORT char *ethernet_mac_ntoa(uint8_t *addr, char *buf, size_t length);
extern DLL_EXPORT uint32_t ipv4_atoi(char *addr);
CDECL_END

#endif // !__ESTACK_H__
