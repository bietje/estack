/*
 * E/STACK netbuf implementation
 *
 * Author: Michel Megens
 * Date: 03/12/2017
 * Email: dev@bietje.net
 */

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include <estack/estack.h>
#include <estack/list.h>
#include <estack/netbuf.h>

struct netbuf *netbuf_realloc(struct netbuf *nb, netbuf_type_t type, size_t size)
{
	struct nbdata *nbd;
	bool prealloc;

	assert(nb);
	assert(size > 0);

	prealloc = false;

	switch(type) {
	case NBAF_DATALINK:
		nbd = &nb->datalink;
		if(nbd->size >= size) {
			nbd->size = size;
			return nb;
		}

		if(netbuf_test_flag(nb, NBUF_DATALINK_ALLOC))
			prealloc = true;

		netbuf_set_flag(nb, NBUF_DATALINK_ALLOC);
		break;

	case NBAF_NETWORK:
		nbd = &nb->network;
		if(nbd->size >= size) {
			nbd->size = size;
			return nb;
		}

		if(netbuf_test_flag(nb, NBUF_NETWORK_ALLOC))
			prealloc = true;
		netbuf_set_flag(nb, NBUF_NETWORK_ALLOC);
		break;

	case NBAF_TRANSPORT:
		nbd = &nb->transport;
		if(nbd->size >= size) {
			nbd->size = size;
			return nb;
		}

		if(netbuf_test_flag(nb, NBUF_TRANSPORT_ALLOC))
			prealloc = true;

		netbuf_set_flag(nb, NBUF_TRANSPORT_ALLOC);
		break;

	case NBAF_APPLICTION:
		nbd = &nb->application;
		if(nbd->size >= size) {
			nbd->size = size;
			return nb;
		}

		if(netbuf_test_flag(nb, NBUF_APPLICATION_ALLOC))
			prealloc = true;
		netbuf_set_flag(nb, NBUF_APPLICATION_ALLOC);
		break;

	default:
		return NULL;
	}

	if(prealloc)
		nbd->data = realloc(nbd->data, size);
	else
		nbd->data = z_alloc(size);

	nbd->size = size;
	return nb;
}

struct netbuf *netbuf_alloc(netbuf_type_t type, size_t size)
{
	struct netbuf *nb;

	assert(size > 0);
	nb = z_alloc(sizeof(*nb));

	list_head_init(&nb->bl_entry);
	list_head_init(&nb->entry);

	if(netbuf_realloc(nb, type, size) == NULL) {
		free(nb);
		return NULL;
	}

	return nb;
}

void netbuf_free_partial(struct netbuf *nb, netbuf_type_t type)
{
	struct nbdata *nbd;

	assert(nb);
	nbd = NULL;

	switch(type) {
	case NBAF_DATALINK:
		if(!(nb->flags & NBAF_DATALINK_MASK))
			return;
		nbd = &nb->datalink;
		break;

	case NBAF_NETWORK:
		if(!(nb->flags & NBAF_NETWORK_MASK))
			return;
		nbd = &nb->network;
		break;

	case NBAF_TRANSPORT:
		if(!(nb->flags & NBAF_TRANSPORT_MASK))
			return;
		
		nbd = &nb->transport;
		break;

	case NBAF_APPLICTION:
		if(!(nb->flags & NBAF_APPLICTION_MASK))
			return;
		
		nbd = &nb->application;
		break;

	default:
		return;
	}

	if(nbd && nbd->data) {
		free(nbd->data);
		nbd->size = 0;
	}
}

void netbuf_free(struct netbuf *nb)
{
	assert(nb);

	if(netbuf_test_flag(nb, NBUF_DATALINK_ALLOC))
		free(nb->datalink.data);

	if(netbuf_test_flag(nb, NBUF_NETWORK_ALLOC))
		free(nb->network.data);

	if(netbuf_test_flag(nb, NBUF_TRANSPORT_ALLOC))
		free(nb->transport.data);

	if(netbuf_test_flag(nb, NBUF_APPLICATION_ALLOC))
		free(nb->application.data);

	free(nb);
}

void netbuf_cpy_data(struct netbuf *nb, const void *src, size_t length, netbuf_type_t type)
{
	struct nbdata *nbd;

	assert(nb);
	assert(src);
	assert(length > 0);

	switch(type) {
	case NBAF_DATALINK:
		nbd = &nb->datalink;
		break;

	case NBAF_NETWORK:
		nbd = &nb->network;
		break;

	case NBAF_TRANSPORT:
		nbd = &nb->transport;
		break;

	case NBAF_APPLICTION:
		nbd = &nb->application;
		break;

	default:
		return;
	}

	memcpy(nbd->data, src, length);
}

void netbuf_cpy_data_offset(struct netbuf *nb, size_t ofs, const void *src,
							size_t length, netbuf_type_t type)
{
	struct nbdata *nbd;

	assert(nb);

	if(!src || !length)
		return;

	switch(type) {
	case NBAF_DATALINK:
		nbd = &nb->datalink;
		break;

	case NBAF_NETWORK:
		nbd = &nb->network;
		break;

	case NBAF_TRANSPORT:
		nbd = &nb->transport;
		break;

	case NBAF_APPLICTION:
		nbd = &nb->application;
		break;

	default:
		return;
	}

	memcpy((uint8_t*)nbd->data+ofs, src, length);
}

struct netbuf *netbuf_clone(struct netbuf *nb, uint32_t layers)
{
	struct netbuf *copy;

	copy = z_alloc(sizeof(*nb));

	list_head_init(&copy->bl_entry);
	list_head_init(&copy->entry);

	for(int i = 1; i < (1 << NBAF_APPLICTION); i <<= 1) {
		switch(i) {
		case NBAF_DATALINK:
			copy = netbuf_realloc(copy, NBAF_DATALINK, nb->datalink.size);
			netbuf_cpy_data(copy, nb->datalink.data, nb->datalink.size, NBAF_DATALINK);
			break;

		case NBAF_NETWORK:
			copy = netbuf_realloc(copy, NBAF_NETWORK, nb->network.size);
			netbuf_cpy_data(copy, nb->network.data, nb->network.size, NBAF_NETWORK);
			break;

		case NBAF_TRANSPORT:
			copy = netbuf_realloc(copy, NBAF_TRANSPORT, nb->transport.size);
			netbuf_cpy_data(copy, nb->transport.data, nb->transport.size, NBAF_TRANSPORT);
			break;

		case NBAF_APPLICTION:
			copy = netbuf_realloc(copy, NBAF_APPLICTION, nb->application.size);
			netbuf_cpy_data(copy, nb->application.data, nb->application.size, NBAF_APPLICTION);
			break;
		
		default:
			break;
		}
	}

	copy->size = nb->size;
	copy->protocol = nb->protocol;
	copy->dev = nb->dev;

	return copy;
}

static size_t __netbuf_pkt_size(struct netbuf *nb)
{
	size_t bytes;

	bytes = nb->application.size;
	bytes += nb->transport.size;
	bytes += nb->network.size;
	bytes += nb->datalink.size;
	return bytes;
}

size_t netbuf_get_size(struct netbuf *nb)
{
	struct list_head *entry;
	struct netbuf *_nb;
	size_t totals;

	totals = 0UL;
	list_for_each(entry, &nb->entry) {
		_nb = list_entry(entry, struct netbuf, entry);
		totals += __netbuf_pkt_size(_nb);
	}

	totals += __netbuf_pkt_size(nb);
	return totals;
}

size_t netbuf_calc_size(struct netbuf *nb)
{
	size_t totals;

	totals = nb->datalink.size;
	totals += nb->network.size;
	totals += nb->transport.size;
	totals += nb->application.size;

	return totals;
}
