/*
 * E/STACK - TCP definitions
 * 
 * Author: Michel Megens
 * Email:  dev@bietje.net
 * Date:   19/01/2018
 */

#ifndef __TCP_H__
#define __TCP_H__

#include <stdlib.h>
#include <stdint.h>

#include <estack/estack.h>
#include <estack/socket.h>
#include <estack/list.h>

#define TCP_OPT_NOOP 1
#define TCP_OPTLEN_MSS 4
#define TCP_OPT_MSS 2
#define TCP_OPT_SACK_OK 4
#define TCP_OPT_SACK 5
#define TCP_OPTLEN_SACK 2
#define TCP_OPT_TS 8

#define TCP_HDR_LENGTH 20

#define TCP_SYN_BACKOFF 500

/* TCP flags */
#define TCP_FIN 0x01U
#define TCP_SYN 0x02U
#define TCP_RST 0x04U
#define TCP_PSH 0x08U
#define TCP_ACK 0x10U
#define TCP_URG 0x20U
#define TCP_ECE 0x40U
#define TCP_CWR 0x80U

#pragma pack(push, 1)
struct tcp_hdr {
	uint16_t sport;
	uint16_t dport;
	uint32_t seq_no;
	uint32_t ack_no;
	uint16_t hlen_flags;
	uint16_t window;
	uint16_t checksum;
	uint16_t urg_ptr;
};

struct tcp_options {
	uint16_t options;
	uint16_t mss;
	uint8_t sack;
};

struct tcp_options_mss {
	uint8_t kind;
	uint8_t length;
	uint16_t mss;
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
	struct netdev *dev;

	estack_timer_t keepalive;
	estack_timer_t rtx;

	bool sackok;
	uint32_t iss;
	uint16_t mss;

	uint8_t backoff;
	uint16_t inflight;

	uint32_t rcv_next;
	uint16_t rcv_window;
	uint32_t rcv_window_announce;

	uint32_t snd_unack; /* Oldest unacknowledged sequence number */
	uint32_t snd_next;
	uint32_t snd_window;
	uint32_t snd_wu_seq; //!< Sequence number of last window update
	uint32_t snd_wu_ack; //!< Ack number of last window update

	int16_t rto; //!< RTT timeout.
	int32_t rttvar; //!< RTT variance.
	int32_t srtt; //!< Smoothed round trip time.

	struct list_head snd_q;
	struct list_head unack_q;
	struct list_head oos_q;
};

#define TCP_MSS 536
#define TCP_MAX_MSS 1440
#define TCP_WINSIZE 3216
#define TCP_MAX_WINDOW_SIZE 0xFFFF
#define TCP_CLIENT_SEND_WINDOW 4096
#define TCP_MAX_WINDOW_SHIFT 14

#define TCP_TIMER_INTERVAL 250
#define TCP_SLOW_TIMER_INTERVAL (2 * TCP_TIMER_INTERVAL)

CDECL
static inline uint16_t tcp_hdr_get_hlen(struct tcp_hdr *hdr)
{
	return ntohs(hdr->hlen_flags) >> 12;
}

#define TCP_FLAGS_MASK 0x3F

static inline uint16_t tcp_hdr_get_flags(struct tcp_hdr *hdr)
{
	return hdr->hlen_flags & TCP_FLAGS_MASK;
}

static inline void tcp_hdr_set_hlen(struct tcp_hdr *hdr, uint8_t len)
{
	uint16_t flags;

	flags = ntohs(hdr->hlen_flags) & TCP_FLAGS_MASK;
	hdr->hlen_flags = htons((len << 12) | flags);
}

static inline void tcp_hdr_set_flags(struct tcp_hdr *hdr, uint16_t flags)
{
	hdr->hlen_flags = (hdr->hlen_flags & htons((uint16_t)~TCP_FLAGS_MASK)) | htons(flags);
}

static inline void *tcp_hdr_get_options(struct tcp_hdr *hdr)
{
	return (void*)(hdr + 1);
}

extern DLL_EXPORT struct socket *tcp_socket_alloc(void);
extern DLL_EXPORT void tcp_socket_free(struct socket *sock);
extern DLL_EXPORT int tcp_connect(struct socket *sock);
extern DLL_EXPORT int tcp_output(struct netbuf *nb, struct tcp_pcb *pcb, uint32_t seq);

extern DLL_EXPORT void tcp_input(struct netbuf *nb);
CDECL_END

#endif
