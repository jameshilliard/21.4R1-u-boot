/***********************************************************************
Copyright 2003-2006 Raza Microelectronics, Inc.(RMI). All rights
reserved.
Use of this software shall be governed in all respects by the terms and
conditions of the RMI software license agreement ("SLA") that was
accepted by the user as a condition to opening the attached files.
Without limiting the foregoing, use of this software in source and
binary code forms, with or without modification, and subject to all
other SLA terms and conditions, is permitted.
Any transfer or redistribution of the source code, with or without
modification, IS PROHIBITED,unless specifically allowed by the SLA.
Any transfer or redistribution of the binary code, with or without
modification, is permitted, provided that the following condition is
met:
Redistributions in binary form must reproduce the above copyright
notice, the SLA, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution:
THIS SOFTWARE IS PROVIDED BY Raza Microelectronics, Inc. `AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL RMI OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*****************************#RMI_3#***********************************/
#ifndef _PACKET_H
#define _PACKET_H

#include "rmi/types.h"
#include "linux/stddef.h"

#define PKT_DATA_LEN 1592
#define PKT_SEC_AUTH_LEN 32
#define PKT_SEC_CTRL_DESC_LEN 128
#define PKT_SEC_PKT_DESC_LEN 32

//#define MAX_PACKETS 2048
/* DLINK Yukon II driver needs 256 pkts minimum */
#define MAX_PACKETS 256
#define DEFAULT_SPILL_ENTRIES (MAX_PACKETS>>1)

#define MAX_FRIN_SPILL_ENTRIES DEFAULT_SPILL_ENTRIES
#define MAX_FROUT_SPILL_ENTRIES DEFAULT_SPILL_ENTRIES
#define MAX_RX_SPILL_ENTRIES DEFAULT_SPILL_ENTRIES

struct packet {
	/* New cacheline */
	uint8_t data[PKT_DATA_LEN];
	uint32_t len;
	uint32_t seq_num;
	/* New cacheline */
	uint8_t sec_ctrl_desc[PKT_SEC_CTRL_DESC_LEN];
	/* New cacheline */
	uint8_t sec_pkt_desc[PKT_SEC_PKT_DESC_LEN];
	/* New cacheline */
	uint8_t sec_auth[PKT_SEC_AUTH_LEN];
	/* New cacheline */
	uint16_t sec_cksum;
	uint8_t sec_cksum_padding[30];
	/* New cacheline */
	uint32_t sec_error;
	uint32_t sec_op_timestamp;
	uint16_t sec_ctrl_desc_size;
	uint8_t padding[14];
	/* New cacheline */
} __attribute__ ((aligned(32)));

struct size_1_desc {
	uint64_t entry0;
};

struct size_2_desc {
	uint64_t entry0;
	uint64_t entry1;
};

struct size_3_desc {
	uint64_t entry0;
	uint64_t entry1;
	uint64_t entry2;
};

struct size_4_desc {
	uint64_t entry0;
	uint64_t entry1;
	uint64_t entry2;
	uint64_t entry3;
};

struct fr_desc {
	struct size_1_desc d1;
};

union rx_tx_desc {
	struct size_2_desc d2;
	struct size_3_desc d3;
	struct size_4_desc d4;
};

#define RX_DESC_ENTRIES 2
#define TX_DESC_ENTRIES 2

extern struct packet packets[];
extern struct fr_desc frin_spill_packets[];
extern struct fr_desc frout_spill_packets[];
extern union rx_tx_desc rx_spill_packets[];

extern struct packet *alloc_pbuf(void);
extern void free_pbuf(struct packet *pbuf);
extern void packet_init(void);
void print_packet(const char *pkt_buf, int len);
void enable_pbuf_dbg(void);
void disable_pbuf_dbg(void);
unsigned int get_pbuf_dbg(void);

static inline int pkt_valid(struct packet *pbuf)
{
	if (pbuf < &packets[0] || pbuf >= &packets[MAX_PACKETS])
		return 0;
	return 1;
}

static __inline__ void *pkt_get_data_ptr(struct packet *pbuf)
{
	return pbuf->data;
}

static __inline__ uint16_t pkt_get_len(struct packet *pbuf)
{
	return pbuf->len;
}

static __inline__ void *pkt_get_sec_ctrl_desc_ptr(struct packet *pbuf)
{
	return pbuf->sec_ctrl_desc;
}

static __inline__ void *pkt_get_sec_pkt_desc_ptr(struct packet *pbuf)
{
	return pbuf->sec_pkt_desc;
}

static __inline__ void *pkt_get_sec_auth_ptr(struct packet *pbuf)
{
	return pbuf->sec_auth;
}

static __inline__ void *pkt_get_sec_cksum_ptr(struct packet *pbuf)
{
	return &pbuf->sec_cksum;
}

static __inline__ struct packet *pkt_get_sec_ctrl_desc_pbuf(void *ctrl_desc)
{
	return (struct packet *)((uint8_t *) ctrl_desc -
				 offsetof(struct packet, sec_ctrl_desc));
}

#endif
