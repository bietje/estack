/*
 * E/Stack header
 *
 * Author: Michel Megens
 * Date: 28/11/2017
 * Email: dev@bietje.net
 */

#ifndef __ESTACK_H__
#define __ESTACK_H__

#include <time.h>
#include <stdint.h>

#ifndef UNUSED
#define UNUSED(x) (void)x
#endif

#ifdef WIN32

#define container_of(addr, type, field) \
((type *)((char*)(addr) - offsetof(type, field)))

#pragma warning(disable : 4251)
#pragma warning (disable : 4820)
#define DLL_EXPORT __declspec(dllexport)
#define __always_inline __forceinline
#pragma comment(lib, "Ws2_32.lib")

#define likely(x) (x)
#define unlikely(x) (x)

#else

#ifndef likely
#define likely(x) __builtin_expect((x), 1)
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect((x), 0)
#endif

#define DLL_EXPORT

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#define container_of(ptr, type, member) ({		\
		const typeof( ((type *)0)->member) *__mptr = (ptr); \
		(type *)( ( char *)__mptr - offsetof(type,member) );})

#ifndef __always_inline
#define __always_inline __attribute__((__always_inline__))
#endif
typedef unsigned char u_char;
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

#ifdef __cplusplus
#define CDECL extern "C" {
#define CDECL_END }
#else
#define CDECL
#define CDECL_END
#endif

#ifndef __cplusplus
typedef unsigned char bool;

#define false 0
#define true !false
#endif

CDECL
extern DLL_EXPORT void *z_alloc(size_t size);
extern DLL_EXPORT time_t estack_utime(void);
CDECL_END

#endif // !__ESTACK_H__
