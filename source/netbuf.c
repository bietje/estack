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

	assert(nb);
	assert(size > 0);

	switch (type) {
	case NBAF_DATALINK:
		nbd = &nb->datalink;
		netbuf_set_flag(nb, NBUF_DATALINK_ALLOC);
		break;

	case NBAF_NETWORK:
		nbd = &nb->network;
		netbuf_set_flag(nb, NBUF_NETWORK_ALLOC);
		break;

	case NBAF_TRANSPORT:
		nbd = &nb->transport;
		netbuf_set_flag(nb, NBUF_TRANSPORT_ALLOC);
		break;

	case NBAF_APPLICTION:
		nbd = &nb->application;
		netbuf_set_flag(nb, NBUF_APPLICATION_ALLOC);
		break;

	default:
		return NULL;
	}

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

	if (netbuf_realloc(nb, type, size) == NULL) {
		free(nb);
		return NULL;
	}

	return nb;
}

void netbuf_free(struct netbuf *nb)
{
	assert(nb);

	if (netbuf_test_flag(nb, NBUF_DATALINK_ALLOC))
		free(nb->datalink.data);

	if (netbuf_test_flag(nb, NBUF_NETWORK_ALLOC))
		free(nb->network.data);

	if (netbuf_test_flag(nb, NBUF_TRANSPORT_ALLOC))
		free(nb->transport.data);

	if (netbuf_test_flag(nb, NBUF_APPLICATION_ALLOC))
		free(nb->application.data);

	free(nb);
}

void netbuf_cpy_data(struct netbuf *nb, const void *src, size_t length, netbuf_type_t type)
{
	struct nbdata *nbd;

	assert(nb);
	assert(src);
	assert(length > 0);

	switch (type) {
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
