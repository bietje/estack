/*
 * E/STACK - TCP definitions
 *
 * Author: Michel Megens
 * Date: 18/01/2017
 * Email: dev@bietje.net
 */

#ifndef __TCP_H__
#define __TCP_H__

#include <stdlib.h>
#include <stdint.h>
#include <estack.h>

#include <estack/socket.h>
#include <estack/list.h>

#pragma pack(push, 1)
struct tcp_hdr {
	uint16_t sport,
			 dport;
	uint32_t seqno;
	uint32_t ackno;
	uint16_t ofs_flags;
	uint16_t window;
	uint16_t chksum;
	uint16_t urgptr;
};
#pragma pack(pop)

typedef enum {
	TCP_CLOSED,
	TCP_LISTEN,
	TCP_SYN_SENT,
	TCP_SYN_RECEIVED,
	TCP_ESTABLISHED,
	TCP_FIN_WAIT_1,
	TCP_FIN_WAIT_2,
	TCP_CLOSE_WAIT,
	TCP_CLOSING,
	TCP_LAST_ACK,
	TCP_TIME_WAIT,
} tcp_state_t;

struct tcp_pcb {
	struct socket sock;
	tcp_state_t state;
	uint16_t flags;

	estack_timer_t *keepalive;
	estack_timer_t *rtx;

	uint32_t rcv_next;
	uint16_t mss;

	uint32_t rttestimate;
	uint32_t rttseq;

	int16_t rto;
	uint32_t lastack; //!< Highest acknowledged sequence number

	/* Sender data */
	uint32_t snd_next;
	uint32_t snd_window;
	uint32_t snd_wu_seq, //!< Sequence number of last window update.
	         snd_wu_ack; //!< Acknowledgement number of last window update.
	
	struct list_head snd_q;
	struct list_head unack_q;
	struct list_head oos_q;
};

#endif
