/*
 * E/STACK - TCP
 * 
 * Author: Michel Megens
 * Date:   19/01/2018
 * Email:  dev@bietje.net
 */

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#include <estack/estack.h>
#include <estack/list.h>
#include <estack/socket.h>
#include <estack/error.h>
#include <estack/tcp.h>
#include <estack/route.h>

static inline struct tcp_pcb *tcp_sock_to_pcb(struct socket *sock)
{
	return container_of(sock, struct tcp_pcb, sock);
}

static inline void tcp_pcb_lock(struct tcp_pcb *pcb)
{
	estack_mutex_lock(&pcb->sock.mtx, 0);
}

static inline void tcp_pcb_unlock(struct tcp_pcb *pcb)
{
	estack_mutex_unlock(&pcb->sock.mtx);
}

struct socket *tcp_socket_alloc(void)
{
	struct tcp_pcb *pcb;

	pcb = z_alloc(sizeof(*pcb));
	assert(pcb);

	socket_init(&pcb->sock);
	pcb->state = TCP_CLOSED;

	list_head_init(&pcb->oos_q);
	list_head_init(&pcb->snd_q);
	list_head_init(&pcb->unack_q);

	return &pcb->sock;
}

void tcp_socket_free(struct socket *sock)
{
	struct tcp_pcb *tcb;

	tcb = container_of(sock, struct tcp_pcb, sock);
	socket_destroy(sock);
	free(tcb);
}

static inline uint16_t tcp_datalength(struct tcp_hdr *hdr, uint16_t buffersize)
{
	uint16_t hdrlen;

	hdrlen = tcp_hdr_get_hlen(hdr);
	hdrlen *= sizeof(uint32_t);
	return buffersize - hdrlen;
}

static int tcp_queue_transmit_nb(struct tcp_pcb *pcb, struct netbuf *nb)
{
	struct tcp_hdr *hdr;
	int rc;

	hdr = nb->transport.data;
	if(pcb->inflight == 0) {
		list_add_tail(&nb->entry, &pcb->unack_q);
		rc = tcp_output(nb, pcb);
		pcb->inflight++;
		pcb->snd_next += tcp_datalength(hdr, (uint16_t)nb->transport.size);
		if(tcp_hdr_get_flags(hdr) & TCP_FIN)
			pcb->snd_next++;
	} else {
		list_add_tail(&nb->entry, &pcb->snd_q);
		rc = -EOK;
	}

	return rc;
}

static void tcp_write_syn_options(struct tcp_hdr *hdr, struct tcp_options *opts, int optlen)
{
	struct tcp_options_mss *mss_options;
	size_t idx;
	uint16_t hlen;

	mss_options = (struct tcp_options_mss*) hdr->data;
	mss_options->kind = TCP_OPT_MSS;
	mss_options->length = TCP_OPTLEN_MSS;
	mss_options->mss = htons(opts->mss);

	idx = sizeof(*mss_options);

	if(opts->sack) {
		hdr->data[idx++] = TCP_OPT_NOOP;
		hdr->data[idx++] = TCP_OPT_NOOP;
		hdr->data[idx++] = TCP_OPT_SACK_OK;
		hdr->data[idx++] = TCP_OPTLEN_SACK;
	}

	hlen = TCP_HDR_LENGTH / sizeof(uint32_t);
	hlen += optlen / sizeof(uint32_t);
	tcp_hdr_set_hlen(hdr,(uint8_t) hlen & 0xFF);
}

static int tcp_syn_options(struct tcp_pcb *pcb, struct tcp_options *opts)
{
	int optlen;

	opts->mss = pcb->mss;
	optlen = TCP_OPTLEN_MSS;

	if(pcb->sackok) {
		opts->sack = 1;
		optlen += TCP_OPT_NOOP * 2;
		optlen += TCP_OPTLEN_SACK;
	} else {
		opts->sack = 0;
	}

	return optlen;
}

static int tcp_send_syn(struct tcp_pcb *pcb, struct netdev *dev)
{
	struct netbuf *nb;
	struct tcp_hdr *hdr;
	struct tcp_options opts = {0};
	int optlen;

	optlen = tcp_syn_options(pcb, &opts);
	nb = netbuf_alloc(NBAF_TRANSPORT, TCP_HDR_LENGTH + optlen);
	nb->dev = dev;
	nb->protocol = IP_PROTO_TCP;
	hdr = (struct tcp_hdr*) nb->transport.data;
	tcp_write_syn_options(hdr, &opts, optlen);
	pcb->state = TCP_SYN_SENT;
	tcp_hdr_set_flags(hdr, TCP_SYN);

	return tcp_queue_transmit_nb(pcb, nb);
}

static void tcp_rto_timer(estack_timer_t *timer, void *arg)
{
	print_dbg("TCP RTO timeout triggered!\n");
}

int tcp_connect(struct socket *sock)
{
	struct tcp_pcb *pcb;
	int rc;
	struct netdev *dev;

	pcb = tcp_sock_to_pcb(sock);

	tcp_pcb_lock(pcb);

	/*
	 * Start by checking for a valid state. Setting up a TCB
	 * for which there is no route is also bollucks, so the route
	 * is checked after.
	 */
	if(pcb->state == TCP_LISTEN) {
		tcp_pcb_unlock(pcb);
		return -ENOTSUPPORTED;
	} else if(pcb->state != TCP_CLOSED) {
		tcp_pcb_unlock(pcb);
		return -EISCONNECTED;
	}

	if(sock->addr.type == IPADDR_TYPE_V4) {
		dev = route4_lookup(sock->addr.addr.in4_addr.s_addr, NULL);
		pcb->dev = dev;
	} else {
		print_dbg("TCP/IPv6 not yet supported!\n");
	}

	if(!pcb->dev) {
		tcp_pcb_unlock(pcb);
		return -EINVALID;
	}

	pcb->mss = TCP_MSS;
	pcb->rcv_window = TCP_WINSIZE;
	pcb->rcv_window_announce = TCP_WINSIZE;

	pcb->snd_next = 0;
	pcb->snd_window = 0;
	pcb->snd_unack = 0;
	pcb->snd_wu_ack = pcb->snd_wu_seq = 0;
	pcb->iss = 0;

	estack_timer_create(&pcb->rtx, "rto", TCP_SYN_BACKOFF << pcb->backoff, 0, pcb, tcp_rto_timer);
	estack_timer_start(&pcb->rtx, 40);
	rc = tcp_send_syn(pcb, dev);
	pcb->snd_next++;
	tcp_pcb_unlock(pcb);

	return rc;
}
