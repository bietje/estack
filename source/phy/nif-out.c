/*
 * E/STACK - Network interface output
 *
 * Author: Michel Megens
 * Date:   02/02/2018
 * Email:  dev@bietje.net
 */

#include <stdlib.h>
#include <estack.h>

#include <estack/error.h>
#include <estack/netdev.h>
#include <estack/netbuf.h>

int netif_output(struct netdev *dev, struct netbuf *nb)
{
	int rc;

	assert(nb);

	if(unlikely(!netbuf_test_flag(nb, NBUF_IS_LINEAR))) {
		netbuf_set_flag(nb, NBUF_AGAIN);
		return -EINVALID;
	}

	rc = dev->write(dev, nb->datalink.data, nb->size);
	if(likely(rc == -EOK)) {
		netbuf_set_flag(nb, NBUF_ARRIVED);
	} else {
		netbuf_clear_flag(nb, NBUF_ARRIVED);
		netbuf_set_flag(nb, NBUF_AGAIN);
		print_dbg("Couldn't write packet to link (%s:%i)\n", __FILE__, __LINE__);
	}

	return rc;
}
