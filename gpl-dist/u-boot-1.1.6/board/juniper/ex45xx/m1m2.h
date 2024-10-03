/*
 * Copyright (c) 2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __M1M2__
#define __M1M2__

#define I2C_DATA_START 8

/* msg-type */
#define MSG_COMMAND     1
#define MSG_REPORT      2  /* report to command */
#define MSG_REQUEST     3
#define MSG_REPLY       4  /* reply to request */

/* command/report */
#define CMD_SYNC        0x00
#define CMD_BAUD        0x01

/* request/reply */
#define REQ_ECHO        0x80
#define REQ_UART        0x81
#define REQ_ETH        0x82
#define REQ_SET_GPIO        0x83
#define REQ_GET_GPIO        0x84

#define REQ_RUN      0xa0
#define REQ_UPTIME      0xa1
#define REQ_VERSION     0xa2

#define MAX_PACKET_LENGTH	1518
#define DOWNLOAD_DATA_SIZE	1024

#define ETH_HDR_SIZE	14		/* Ethernet header size		*/

typedef struct {
	uchar		et_dest[6];	/* Destination node		*/
	uchar		et_src[6];	/* Source node			*/
	ushort		et_protlen;	/* Protocol or length		*/
} Eth_t;

#define M1M2_VERBOSE_ON 1
#define M1M2_DEBUG_ON 2

#define M1_40_SLOT 0
#define M1_80_SLOT 1
#define M2_SLOT 2
#define MAX_CARD_SLOTS 3

#define MAX_M1M2_TEST 20

struct m1m2_test {
    char *name;
    char *cmd;
    char *desc;
    char *limit;
    int (*test) (int, int);
};

struct m1m2_setup {
    int enable;
    int loop;
    int length;
    int address;
};

#endif /* __M1M2__ */
