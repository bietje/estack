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

#include <limits.h>

#include <estack/estack.h>
#include <estack/netbuf.h>
#include <estack/netdev.h>
#include <estack/ethernet.h>
#include <estack/error.h>
#include <estack/prototype.h>
#include <estack/interface.h>

struct pcapdev_private {
	pcap_t **pio;
	char **srcs;
	int idx;
	int length;

	pcap_t *dst;
	pcap_dumper_t *dumper;
	struct netdev dev;
	int available, nread;
	estack_thread_t thread;
	bool running;
};

static inline void pcapdev_lock(struct netdev *dev)
{
	estack_mutex_lock(&dev->mtx, 0);
}

static inline void pcapdev_unlock(struct netdev *dev)
{
	estack_mutex_unlock(&dev->mtx);
}

static pcap_t *pcapdev_open_file(const char *src)
{
	char errb[PCAP_ERRBUF_SIZE];
	pcap_t *cap;

	if(!src)
		return NULL;

	memset(errb, 0, PCAP_ERRBUF_SIZE);
	cap = pcap_open_offline(src, errb);

	if(!cap) {
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
	int rv, count, num;
	const u_char *data;

	priv = container_of(dev, struct pcapdev_private, dev);
	pcapdev_lock(dev);

	if(priv->available >= 0) {
		rv = priv->available;
		pcapdev_unlock(dev);
		return rv;
	}

	if(priv->idx >= priv->length) {
		pcapdev_unlock(dev);
		return 0;
	}

	cap = pcapdev_open_file(priv->srcs[priv->idx]);

	if(!cap) {
		pcapdev_unlock(dev);
		return -1;
	}

	count = num = 0;
	while((rv = pcap_next_ex(cap, &hdr, &data)) >= 0) {
		count++;
		num += hdr->len;
	}

	priv->nread = count;
	priv->available = num;
	pcap_close(cap);

	pcapdev_unlock(dev);
	return num;
}

#define PCAP_MAGIC 0xa1b2c3d4

/*static int pcapdev_write(struct netdev *dev, struct netbuf *nb)
{
	struct pcap_pkthdr hdr;
	struct pcapdev_private *priv;
	uint8_t *data;
	time_t timestamp;

	assert(dev);
	assert(nb);

	priv = container_of(dev, struct pcapdev_private, dev);

	memset(&hdr, 0, sizeof(hdr));
	hdr.caplen = hdr.len = nb->size;
	timestamp = estack_utime();
	hdr.ts.tv_sec = (long)(timestamp / 1e6L);
	hdr.ts.tv_usec = timestamp % (long)1e6L;
	data = malloc(hdr.len);
	memset(data, 0, hdr.len);

	memcpy(data, nb->datalink.data, nb->size);

	pcap_dump((u_char*)priv->dumper, &hdr, data);
	pcap_dump_flush(priv->dumper);
	free(data);

	netbuf_set_flag(nb, NBUF_ARRIVED);

	return -EOK;
}*/

static int pcapdev_write(struct netdev *dev, const void *data, size_t length)
{
	struct pcap_pkthdr hdr;
	struct pcapdev_private *priv;
	time_t timestamp;

	assert(dev);
	assert(data);
	assert(length);

	priv = container_of(dev, struct pcapdev_private, dev);

	memset(&hdr, 0, sizeof(hdr));
	hdr.caplen = hdr.len = length;
	timestamp = estack_utime();
	hdr.ts.tv_sec = (long)(timestamp / 1e6L);
	hdr.ts.tv_usec = timestamp % (long)1e6L;

	pcap_dump((u_char*)priv->dumper, &hdr, data);
	pcap_dump_flush(priv->dumper);
	return -EOK;
}

static int pcapdev_read(struct netdev *dev, int num)
{
	struct pcap_pkthdr *hdr;
	const u_char *data;
	struct pcapdev_private *priv;
	int rv, tmp;
	size_t length;
	time_t timestamp;
	pcap_t *cap;

	assert(dev);
	priv = container_of(dev, struct pcapdev_private, dev);
	tmp = 0;

	pcapdev_lock(dev);

	if(num < 0) 
		num = INT_MAX;

	if(priv->idx >= priv->length) {
		pcapdev_unlock(dev);
		return 0;
	}

	cap = priv->pio[priv->idx];
	if(!cap) {
		pcapdev_unlock(dev);
		return 0;
	}

	while(priv->nread > 0 && (rv = pcap_next_ex(cap, &hdr, &data)) >= 0 && num > 0) {
		length = hdr->len;
		pcapdev_unlock(dev);
		netif_input(dev, data, length, PROTO_ETHERNET);
		pcapdev_lock(dev);

		num -= 1;
		tmp += 1;

		timestamp = estack_utime();
		hdr->ts.tv_sec = (long)(timestamp / 1e6L);
		hdr->ts.tv_usec = timestamp % (long)1e6L;
		pcap_dump((u_char*)priv->dumper, hdr, data);

		priv->nread--;
		priv->available -= length;
	}

	pcapdev_unlock(dev);
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

void pcapdev_set_name(struct netdev *dev, const char *name)
{
	int len;

	len = strlen(name) + 1;
	dev->name = realloc((void*)dev->name, len);
	memcpy((char*)dev->name, name, len);
}

static void pcapdev_setup_output(const char *dst, struct pcapdev_private *priv)
{
	priv->dst = pcap_open_dead(DLT_EN10MB, (1 << 16) - 1);
	priv->dumper = pcap_dump_open(priv->dst, dst);
}

void pcapdev_create_link_ip4(struct netdev *dev, uint32_t local, uint32_t remote, uint32_t mask)
{
	uint8_t *_local, *_remote, *_mask;

	_local = (void*)&local;
	_remote = (void*)&remote;
	_mask = (void*)&mask;
	ifconfig(dev, _local, _remote, _mask, 4, NIF_TYPE_ETHER);
}

void pcapdev_next_src(struct netdev *dev)
{
	struct pcapdev_private *priv;

	assert(dev);
	priv = container_of(dev, struct pcapdev_private, dev);

	pcapdev_lock(dev);
	priv->idx++;
	priv->available = -1;
	priv->nread = 0;
	pcapdev_unlock(dev);
}

static inline bool pcapdev_is_running(struct netdev *dev)
{
	struct pcapdev_private *priv;
	bool running;

	priv = container_of(dev, struct pcapdev_private, dev);
	pcapdev_lock(dev);
	running = priv->running;
	pcapdev_unlock(dev);

	return running;
}

static void pcap_task(void *arg)
{
	struct netdev *dev;
	int available;

	dev = arg;

	while(pcapdev_is_running(dev)) {
		available = dev->available(dev);
		if(available)
			netdev_wakeup();
		
		estack_sleep(100);
	}
}

void pcapdev_start(struct netdev *dev)
{
	pcapdev_next_src(dev);
}

struct netdev *pcapdev_create(const char **srcs, int length, const char *dstfile,
	const uint8_t *hwaddr, uint16_t mtu)
{
	struct pcapdev_private *priv;
	struct netdev *dev;
	int len;

	priv = z_alloc(sizeof(*priv));
	assert(priv != NULL);
	assert(dstfile);

	if(srcs && length) {
		priv->srcs = calloc(length + 1, sizeof(void*));
		priv->pio = calloc(length + 1, sizeof(void*));
		priv->idx = 0;
		priv->length = length + 1;
		priv->srcs[0] = NULL;
		priv->pio[0] = NULL;

		for(int i = 0; i < length; i++) {
			len = strlen(srcs[i]);
			priv->srcs[i+1] = z_alloc(len + 1);
			strcpy(priv->srcs[i+1], srcs[i]);
			priv->pio[i+1] = pcapdev_open_file(srcs[i]);

			assert(priv->pio[i+1]);
		}
	}

	pcapdev_setup_output(dstfile, priv);

	dev = &priv->dev;

	priv->available = -1;
	dev->read = pcapdev_read;
	dev->write = pcapdev_write;
	dev->available = pcapdev_available;

	dev->rx = ethernet_input;
	dev->tx = ethernet_output;
	pcapdev_init(dev, "dbg0", hwaddr, mtu);

	pcapdev_lock(dev);
	priv->running = true;
	pcapdev_unlock(dev);

	priv->thread.name = "pcap-tsk";
	estack_thread_create(&priv->thread, pcap_task, dev);

	return dev;
}

void pcapdev_destroy(struct netdev *dev)
{
	struct pcapdev_private *priv;

	priv = container_of(dev, struct pcapdev_private, dev);

	pcapdev_lock(dev);
	priv->running = false;
	pcapdev_unlock(dev);
	estack_thread_destroy(&priv->thread);

	if(priv->srcs && priv->pio) {
		for(int i = 1; i < priv->length; i++) {
			pcap_close(priv->pio[i]);
			free(priv->srcs[i]);
		}

		free(priv->pio);
		free(priv->srcs);
	}

	netdev_destroy(dev);

	if(priv->dumper) {
		pcap_dump_close(priv->dumper);
		pcap_close(priv->dst);
	}

	free((void*)dev->name);
	free(priv);
}
