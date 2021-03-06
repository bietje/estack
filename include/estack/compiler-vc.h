/*
 * E/STACK - Visual C/C++ compiler definitions
 *
 * Author: Michel Megens
 * Date: 12/12/2017
 * Email: dev@bietje.net
 */

#ifndef __COMPILER_H__
#error "Don't include compiler-vc.h directly, include compiler.h instead"
#endif

#define __compiler_likely(x) (x)
#define __compiler_unlikely(x) (x)

#define __compiler_co(addr, type, field) ((type *)((char*)(addr) - offsetof(type, field)))

#define __maybe

#pragma warning(disable : 4251)
#pragma warning (disable : 4820)
#pragma warning (disable : 4100)
#pragma warning (disable : 4201)

#define DLL_EXPORT __declspec(dllexport)
#define __always_inline __forceinline


#pragma comment(lib, "Ws2_32.lib")