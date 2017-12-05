/*
 * E/Stack ethernet input
 *
 * Author: Michel Megens
 * Date: 03/12/2017
 * Email: dev@bietje.net
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <estack/estack.h>
#include <estack/netdev.h>
#include <estack/netbuf.h>
#include <estack/ethernet.h>
#include <estack/inet.h>

void ethernet_output(struct netbuf *nb, uint8_t *hw)
{
	struct ethernet_header *hdr;
	struct netdev *dev;

	nb = netbuf_realloc(nb, NBAF_DATALINK, sizeof(*hdr));
	hdr = nb->datalink.data;

	dev = nb->dev;

	memcpy(hdr->src_mac, dev->hwaddr, ETHERNET_MAC_LENGTH);
	if (hw)
		memcpy(hdr->dest_mac, hw, ETHERNET_MAC_LENGTH);
	else
		memset(hdr->dest_mac, 0xFF, ETHERNET_MAC_LENGTH);

	hdr->type = htons(nb->protocol);
	netdev_add_backlog(dev, nb);
}
