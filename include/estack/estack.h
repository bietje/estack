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

#ifdef WIN32

#define container_of(addr, type, field) \
((type *)((char*)(addr) - offsetof(type, field)))

#pragma warning(disable : 4251)
#define DLL_EXPORT __declspec(dllexport)
#define __always_inline __forceinline
#pragma comment(lib, "Ws2_32.lib")

#else
#define DLL_EXPORT

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define __always_inline __attribute__((__always_inline__))

typedef unsigned char u_char;
#endif

#define container_of(ptr, type, member) ({		\
		const typeof( ((type *)0)->member) *__mptr = (ptr); \
		(type *)( ( char *)__mptr - offsetof(type,member) );})
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

CDECL
extern DLL_EXPORT void *z_alloc(size_t size);
CDECL_END

#endif // !__ESTACK_H__
