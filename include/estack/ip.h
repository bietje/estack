/*
 * E/STACK - IP header
 *
 * Author: Michel Megens
 * Date: 28/11/2017
 * Email: dev@bietje.net
 */

#ifndef __IP_H__
#define __IP_H__

#include <stdlib.h>
#include <stdint.h>

#include <estack/estack.h>
#include <estack/netbuf.h>

CDECL
extern DLL_EXPORT void ip_input(struct netbuf *nb);
extern DLL_EXPORT void ipv4_input(struct netbuf *nb);
extern DLL_EXPORT void ipv4_output(struct netbuf *nb, uint32_t dst);
CDECL_END

#endif /* !__IP_H__ */