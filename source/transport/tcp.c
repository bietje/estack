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

static void tcp_release_queue(struct list_head *lh)
{
	struct netbuf *nb;
	struct list_head *entry, *tmp;
	struct netdev *dev;

	list_for_each_safe(entry, tmp, lh) {
		nb = list_entry(entry, struct netbuf, entry);
		dev = nb->dev;
		estack_mutex_lock(&dev->mtx, 0);
		list_del(entry);
		netbuf_free(nb);
		estack_mutex_unlock(&dev->mtx);
	}
}

void tcp_socket_free(struct socket *sock)
{
	struct tcp_pcb *tcb;

	tcb = container_of(sock, struct tcp_pcb, sock);
	tcp_pcb_lock(tcb);
	estack_timer_destroy(&tcb->rtx);
	estack_timer_destroy(&tcb->keepalive);
	tcp_release_queue(&tcb->unack_q);
	tcp_release_queue(&tcb->snd_q);
	tcp_release_queue(&tcb->oos_q);
	tcp_pcb_unlock(tcb);
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

	if(list_empty(&pcb->unack_q)) {
		estack_timer_start(&pcb->rtx);
		estack_timer_set_period(&pcb->rtx, pcb->rto);
	}

	hdr = nb->transport.data;
	if(pcb->inflight == 0) {
		list_add_tail(&nb->entry, &pcb->unack_q);
		rc = tcp_output(nb, pcb, pcb->snd_next);
		pcb->inflight++;
		pcb->snd_next += tcp_datalength(hdr, (uint16_t)nb->transport.size);
		nb->sequence_end = pcb->snd_next;

		if(tcp_hdr_get_flags(hdr) & TCP_FIN)
			pcb->snd_next++;
		netbuf_set_flag(nb, NBUF_TX_KEEP);
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
	uint8_t *data;

	data = tcp_hdr_get_options(hdr);
	mss_options = (struct tcp_options_mss*)data;
	mss_options->kind = TCP_OPT_MSS;
	mss_options->length = TCP_OPTLEN_MSS;
	mss_options->mss = htons(opts->mss);

	idx = sizeof(*mss_options);

	if(opts->sack) {
		data[idx++] = TCP_OPT_NOOP;
		data[idx++] = TCP_OPT_NOOP;
		data[idx++] = TCP_OPT_SACK_OK;
		data[idx++] = TCP_OPTLEN_SACK;
	}

	hlen = TCP_HDR_LENGTH / sizeof(uint32_t);
	hlen += (uint16_t)(optlen / sizeof(uint32_t));
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
	struct tcp_pcb *pcb;
	struct netbuf *nb;

	pcb = (struct tcp_pcb*)arg;
	switch(pcb->state) {
	case TCP_SYN_SENT:
		tcp_pcb_lock(pcb);
		if(pcb->backoff >= TCP_CONN_TMO) {
			pcb->sock.err = -ETIMEOUT;
			estack_timer_stop(timer);
			estack_event_signal(&pcb->sock.read_event);
			return;
		}

		nb = list_peak(&pcb->unack_q, struct netbuf, entry);
		nb->protocol = IP_PROTO_TCP;
		tcp_output(nb, pcb, pcb->snd_unack);
		pcb->backoff += 1;
		estack_timer_set_period(&pcb->rtx, pcb->rto << pcb->backoff);
		tcp_pcb_unlock(pcb);
		break;

	default:
		break;
	}

	print_dbg("TCP RTO timer fired! (%lu)\n", estack_utime());
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
		return -EINVALID;
	}

	if(!pcb->dev) {
		tcp_pcb_unlock(pcb);
		return -EINVALID;
	}

	sock->dev = dev;

	pcb->mss = TCP_MSS;
	pcb->rcv_window = TCP_WINSIZE;
	pcb->rcv_window_announce = TCP_WINSIZE;

	pcb->snd_next = 0;
	pcb->snd_window = 0;
	pcb->snd_unack = 0;
	pcb->snd_wu_ack = pcb->snd_wu_seq = 0;
	pcb->iss = 0;

	pcb->rto = TCP_SYN_BACKOFF;
	estack_timer_create(&pcb->rtx, "rto", TCP_SYN_BACKOFF << pcb->backoff, 0, pcb, tcp_rto_timer);
	pcb->rto = TCP_SYN_BACKOFF;
	rc = tcp_send_syn(pcb, dev);
	pcb->snd_next++;
	tcp_pcb_unlock(pcb);
	estack_event_wait(&sock->read_event, INFINITE);
	return sock->err;
}

static void tcp_parse_options(struct tcp_pcb *pcb, struct tcp_hdr *hdr)
{
	struct tcp_options_mss *opt_mss;
	uint16_t mss;
	uint8_t *data;
	uint8_t optlen;

	optlen = (uint8_t)(tcp_hdr_get_hlen(hdr) - TCP_HDR_LENGTH);
	data = (uint8_t*)hdr;
	data += TCP_HDR_LENGTH;

	while(optlen > 0 && optlen < 20) {
		switch(*data) {
		case TCP_OPT_MSS:
			opt_mss = (struct tcp_options_mss*)data;
			mss = ntohs(opt_mss->mss);

			if(mss > TCP_MSS && mss < TCP_MAX_MSS)
				pcb->smss = mss;

			data += sizeof(struct tcp_options_mss);
			optlen -= sizeof(struct tcp_options_mss);
			break;

		case TCP_OPT_NOOP:
			data++;
			optlen--;
			break;

		case TCP_OPT_SACK_OK:
			optlen--;
			break;

		case TCP_OPT_TS:
			optlen--;
			break;

		default:
			optlen--;
			break;
		}
	}
}

static void tcp_clear_rto(struct tcp_pcb *pcb)
{
	struct list_head *entry, *tmp;
	struct netbuf *nb;

	list_for_each_safe(entry, tmp, &pcb->unack_q) {
		nb = list_entry(entry, struct netbuf, entry);
		if(nb->sequence_end <= pcb->snd_unack) {
			list_del(entry);
			if(pcb->inflight > 0)
				pcb->inflight--;
			netbuf_free(nb);
		}
	}

	if(list_empty(&pcb->unack_q) || !pcb->inflight) {
		estack_timer_stop(&pcb->rtx);
	}
}

static void tcp_send_reset(struct tcp_pcb *pcb)
{

}

static void tcp_reset(struct tcp_pcb *pcb)
{

}

static int tcp_options_length(struct tcp_pcb *pcb)
{
	int optlen = 0;

	if(pcb->sackok && pcb->sacklength > 0) {
		/* TODO: implement SACKS */

		optlen += 2;
	}

	while(optlen % 4 > 0) optlen++;

	return optlen;
}

static void tcp_send_ack(struct tcp_pcb *pcb)
{
	struct netbuf *nb;
	struct tcp_hdr *hdr;
	int optlen;

	if(pcb->state == TCP_CLOSED)
		return;

	optlen = tcp_options_length(pcb);
	nb = netbuf_alloc(NBAF_TRANSPORT, TCP_HDR_LENGTH + optlen);
	hdr = nb->transport.data;
	tcp_hdr_set_flags(hdr, TCP_ACK);
	tcp_hdr_set_hlen(hdr, (uint8_t)(TCP_HDR_LENGTH + optlen) / sizeof(uint32_t));
	tcp_output(nb, pcb, pcb->snd_next);
}

static void tcp_syn_sent(struct tcp_pcb *pcb, struct netbuf *nb)
{
	struct tcp_hdr *hdr;
	uint16_t flags;

	hdr = nb->transport.data;
	flags = tcp_hdr_get_flags(hdr);

	if(flags & TCP_ACK) {
		/* Validate packet */
		if(hdr->ack_no <= pcb->iss || hdr->ack_no > pcb->snd_next) {
			if(!(flags & TCP_RST))
				tcp_send_reset(pcb);

			netbuf_set_flag(nb, NBUF_DROPPED);
			return;
		}

		if(hdr->ack_no < pcb->snd_unack || hdr->ack_no > pcb->snd_next) {
			tcp_send_reset(pcb);
			netbuf_set_flag(nb, NBUF_DROPPED);
			return;
		}
	}

	if(!(flags & TCP_SYN)) {
		tcp_send_reset(pcb);
		netbuf_set_flag(nb, NBUF_DROPPED);
		return;
	}

	pcb->rcv_next = hdr->seq_no + 1;
	if(flags & TCP_ACK) {
		pcb->snd_unack = hdr->ack_no;
		tcp_clear_rto(pcb);
	}

	if(pcb->snd_unack > pcb->iss) {
		pcb->state = TCP_ESTABLISHED;
		pcb->snd_unack = pcb->snd_next;
		pcb->backoff = 0;
		pcb->rto = TCP_RTO;
		tcp_send_ack(pcb);
		tcp_parse_options(pcb, hdr);
		estack_event_signal(&pcb->sock.read_event);
	}

	netbuf_set_flag(nb, NBUF_ARRIVED);
}

void tcp_process(struct socket *sock , struct netbuf *nb)
{
	struct tcp_pcb *pcb;

	pcb = container_of(sock, struct tcp_pcb, sock);

	switch(pcb->state) {
	case TCP_CLOSED:
		break;

	case TCP_SYN_SENT:
		tcp_pcb_lock(pcb);
		tcp_syn_sent(pcb, nb);
		tcp_pcb_unlock(pcb);
		return;

	default:
		break;
	}
}
