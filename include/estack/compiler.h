/*
 * E/STACK - Compiler header
 *
 * Author: Michel Megens
 * Date: 12/12/2017
 * Email: dev@bietje.net
 */

#ifndef __COMPILER_H__
#define __COMPILER_H__

#ifdef WIN32
#include <estack/compiler-vc.h>
#else
#include <estack/compiler-gcc.h>
#endif

#define likely(x) __compiler_likely(x)
#define unlikely(x) __compiler_unlikely(x)

#define container_of(ptr, type, entry) __compiler_co(ptr, type, entry)

#ifndef __cplusplus
typedef unsigned char bool;

#define false 0
#define true !false
#endif

#ifdef __cplusplus
#define CDECL extern "C" {
#define CDECL_END }
#else
#define CDECL
#define CDECL_END
#endif

#endif
