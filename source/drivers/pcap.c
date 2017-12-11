/*
 * PCap as a network device
 *
 * Author: Michel Megens
 * Date: 28/11/2017
 * Email: dev@bietje.net
 */

#define _CRT_SECURE_NO_WARNINGS 1

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <pcap.h>
#include <stdarg.h>

#ifndef WIN32
#include <limits.h>
#endif

#include <estack/estack.h>
#include <estack/netbuf.h>
#include <estack/netdev.h>
#include <estack/ethernet.h>
#include <estack/error.h>

struct pcapdev_private {
	pcap_t *cap;
	char *src;
	FILE *dst;
	struct netdev dev;
	int available;
};

static pcap_t *pcapdev_open_file(const char *src)
{
	char errb[PCAP_ERRBUF_SIZE];
	pcap_t *cap;

	memset(errb, 0, PCAP_ERRBUF_SIZE);
	cap = pcap_open_offline(src, errb);

	if (!cap) {
		fprintf(stderr, "[PCAP DEV]: %s\n", errb);
		return NULL;
	}

	return cap;
}

static int pcapdev_available(struct netdev *dev)
{
	struct pcapdev_private *priv;
	pcap_t *cap;
	struct pcap_pkthdr *hdr;
	int rv, count;
	const u_char *data;

	priv = container_of(dev, struct pcapdev_private, dev);
	if (priv->available >= 0)
		return priv->available;

	cap = pcapdev_open_file(priv->src);

	if (!cap)
		return -1;

	count = 0;
	while ((rv = pcap_next_ex(cap, &hdr, &data)) >= 0)
		count += 1;

	priv->available = count;
	pcap_close(cap);

	return count;
}

#define PCAP_MAGIC 0xa1b2c3d4

static int pcapdev_write(struct netdev *dev, struct netbuf *nb)
{
	FILE *fp;
	struct pcap_pkthdr hdr;
	struct pcapdev_private *priv;
	time_t tstamp;

	priv = container_of(dev, struct pcapdev_private, dev);
	fp = priv->dst;
	assert(fp);

	hdr.caplen = hdr.len = nb->size;
	tstamp = estack_utime();
	hdr.ts.tv_sec = (long)(tstamp / 1e6L);
	hdr.ts.tv_usec = tstamp % 1000000ULL;

	fwrite(&hdr, sizeof(hdr), 1, fp);

	if (nb->datalink.size > 0)
		fwrite(nb->datalink.data, nb->datalink.size, 1, fp);
	if (nb->network.size > 0)
		fwrite(nb->network.data, nb->network.size, 1, fp);
	if (nb->transport.size > 0)
		fwrite(nb->transport.data, nb->transport.size, 1, fp);
	if (nb->application.size > 0)
		fwrite(nb->application.data, nb->application.size, 1, fp);

	fflush(fp);

	return -EOK;
}

static int pcapdev_read(struct netdev *dev, int num)
{
	struct pcap_pkthdr *hdr;
	const u_char *data;
	struct netbuf *nb;
	struct pcapdev_private *priv;
	int rv, tmp;
	size_t length;

	assert(dev != NULL);
	priv = container_of(dev, struct pcapdev_private, dev);
	tmp = 0;

	if (num < 0)
		num = INT_MAX;

	while ((rv = pcap_next_ex(priv->cap, &hdr, &data)) >= 0 && num > 0) {
		length = hdr->len;
		nb = netbuf_alloc(NBAF_DATALINK, length);
		netbuf_cpy_data(nb, data, length, NBAF_DATALINK);
		netbuf_set_flag(nb, NBUF_RX);
		nb->protocol = ethernet_get_type(nb);
		netdev_add_backlog(dev, nb);

		num -= 1;
		tmp += 1;
	}

	return tmp;
}

#define HWADDR_LENGTH 6
static void pcapdev_init(struct netdev *dev, const char *name, const uint8_t *hw, uint16_t mtu)
{
	int len;

	assert(dev != NULL);
	assert(name != NULL);

	netdev_init(dev);
	dev->mtu = mtu;

	memcpy(dev->hwaddr, hw, HWADDR_LENGTH);
	dev->addrlen = HWADDR_LENGTH;

	len = strlen(name);
	dev->name = z_alloc(len + 1);
	memcpy((char*)dev->name, name, len);
}

static void pcapdev_setup_output(const char *dst, struct pcapdev_private *priv)
{
	struct pcap_file_header fh;

	priv->dst = fopen(dst, "w");
	assert(priv->dst);

	fh.magic = PCAP_MAGIC;
	fh.sigfigs = 0;
	fh.version_major = 2;
	fh.version_minor = 8;
	fh.snaplen = USHRT_MAX;
	fh.thiszone = 0;
	fh.linktype = DLT_EN10MB;
	
	fwrite(&fh, sizeof(fh), 1, priv->dst);
}

void pcapdev_create_link_ip4(struct netdev *dev, uint32_t local, uint32_t remote, uint32_t mask)
{
	uint8_t *_local, *_remote, *_mask;

	_local = (void*)&local;
	_remote = (void*)&remote;
	_mask = (void*)&mask;
	ifconfig(dev, _local, _remote, _mask, 4, NIF_TYPE_IP4);
}

struct netdev *pcapdev_create(const char *srcfile, const char *dstfile,
	const uint8_t *hwaddr, uint16_t mtu)
{
	struct pcapdev_private *priv;
	struct netdev *dev;
	int len;

	priv = z_alloc(sizeof(*priv));
	assert(srcfile != NULL);
	assert(priv != NULL);

	len = strlen(srcfile);
	priv->src = z_alloc(len + 1);
	memcpy(priv->src, srcfile, len);

	pcapdev_setup_output(dstfile, priv);
	priv->cap = pcapdev_open_file(srcfile);
	dev = &priv->dev;
	pcapdev_init(dev, "dbg0", hwaddr, mtu);

	if (!priv->cap) {
		return NULL;
	}

	priv->available = -1;
	dev->read = pcapdev_read;
	dev->write = pcapdev_write;
	dev->available = pcapdev_available;

	dev->rx = ethernet_input;
	dev->tx = ethernet_output;

	return dev;
}
