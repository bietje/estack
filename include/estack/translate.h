/*
 * E/STACK address translation header
 *
 * Author: Michel Megens
 * Date: 28/11/2017
 * Email: dev@bietje.net
 */

#ifndef __ADDR_TRANSLATION_H__
#define __ADDR_TRANSLATION_H__

#include <estack/estack.h>

CDECL
extern DLL_EXPORT void translate_ipv4_to_mac(struct netdev *dev, uint8_t *src);
CDECL_END

#endif
