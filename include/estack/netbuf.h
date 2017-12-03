/*
 * E/STACK netbuf header
 *
 * Author: Michel Megens
 * Date: 01/12/2017
 * Email: dev@bietje.net
 */

#ifndef __NETBUF_H__
#define __NETBUF_H__

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <estack/estack.h>
#include <estack/list.h>

struct DLL_EXPORT nbdata {
	void *data;
	size_t size;
};

struct DLL_EXPORT netbuf {
	struct list_head entry;

	struct nbdata datalink,
		network,
		transport,
		application;

	uint16_t protocol;
	unsigned int flags;
};



#endif __NETBUF_H__
