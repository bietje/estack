/*
 * E/STACK - Network interface abstraction header
 *
 * Author: Michel Megens
 * Date:   02/02/2018
 * Email:  dev@bietje.net
 */

#ifndef __NIF_H__
#define __NIF_H__

#include <stdlib.h>
#include <estack.h>

#include <estack/netdev.h>
#include <estack/netbuf.h>

CDECL
extern DLL_EXPORT int netif_output(struct netdev *dev, struct netbuf *nb);
extern DLL_EXPORT void netif_input(struct netdev *dev, const void *data, size_t length, int protocol);
CDECL_END

#endif /* __NIF_H__ */
