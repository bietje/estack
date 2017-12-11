/*
 * E/STACK networking neighbours
 *
 * Author: Michel Megens
 * Date: 11/12/2017
 * Email: dev@bietje.net
 */

#ifndef __NEIGHBOUR_H__
#define __NEIGHBOUR_H__

#include <stdlib.h>

#include <estack/estack.h>
#include <estack/netdev.h>
#include <estack/netbuf.h>

extern DLL_EXPORT bool neighbour_output(struct netdev *dev, struct netbuf *nb, void *addr, uint8_t length, resolve_handle handle);

#endif // !__NEIGHBOUR_H__
