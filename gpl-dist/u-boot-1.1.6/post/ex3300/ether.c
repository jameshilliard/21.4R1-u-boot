/*
 * Copyright (c) 2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * (C) Copyright 2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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

#include <common.h>

/************  etsec for EX3300 ***************/

/*
 * Ethernet test
 *
 * The eTSEC controller  listed in ctlr_list array below
 * are tested in the internal and external loopback  mode.
 * The controllers are configured accordingly and several packets
 * are transmitted and recevied back both in internal and external loopback.
 * The configurable test parameters are:
 * MIN_PACKET_LENGTH - minimum size of packet to transmit
 * MAX_PACKET_LENGTH - maximum size of packet to transmit
 * TEST_NUM - number of tests
 */
#ifdef  CONFIG_POST
#include <net.h>
#include <post.h>

#if CONFIG_POST & CFG_POST_ETHER
DECLARE_GLOBAL_DATA_PTR;

extern uint8_t diag_eth0;
extern uint8_t tx_err;
extern uint8_t rx_err;
extern char tx_err_msg[];
extern char rx_err_msg[];

extern void eth0_phy_loopback(int enable);
extern void eth0_ext_loopback(int enable, int sp100);
extern void eth0_mac_loopback(int enable);
extern int eth0_mac_link(void);
extern int eth0_ext_link(void);
extern int eth_receive(volatile void *packet, int length);

#define MIN_PACKET_LENGTH	64
#define MAX_PACKET_LENGTH	1516
#define POST_MAX_PACKET_LENGTH	256
#define TEST_NUM		1

/* capability */
#define LB_MAC           0
#define LB_PHY           1
#define LB_EXT_10        2
#define LB_EXT_100       3
#define LB_EXT_1000       4
#define MAX_LB_TYPES     5

extern char packet_send[];
extern char packet_recv[];

struct ether_device {
    char *name;
    u32 capability;
};

#define NUM_ETHER_DEVICE 1

struct ether_device ether_dev[] =
//    {{ENV_ETH_PRIME, 1<<LB_MAC | 1<<LB_PHY |
    {{ENV_ETH_PRIME, 1<<LB_PHY |
        1<<LB_EXT_10 |1<<LB_EXT_100 | 1<<LB_EXT_1000},
     {NULL , 0}
    };

#define NUM_POST_ETHER_DEVICE 1

struct ether_device post_ether_dev[] =
//    {{ENV_ETH_PRIME, 1<<LB_MAC | 1<<LB_PHY},
    {{ENV_ETH_PRIME, 1<<LB_PHY},
     {NULL , 0}
    };

typedef struct _loopback_status_field {
	uint32_t loopType :4; /* loopback type */
	uint32_t pktSize :12;	/* packet size */
	uint32_t TxErr :8;	/* Tx Error */
	uint32_t RxErr :8;	/* Rx Error */
} loopback_status_field;

typedef union _loopback_status {
    loopback_status_field field;
    uint32_t status;
} loopback_status;

static int ether_debug_flag;
static int boot_flag_post;
int post_result_ether;
loopback_status lb_status;

char mode_test_string[MAX_LB_TYPES][30] = 
     {{"Mac Loopback"},
	{"Phy Loopback"},
       {"Ext Loopback 10 Mbps"},
	{"Ext Loopback 100 Mbps"},
	{"Ext Loopback 1000 Mbps"},
     };

/*
 * Test routines
 */
static void 
packet_fill (char *packet, int length)
{
    char c = (char) length;
    int i;

    packet[0] = 0xFF;
    packet[1] = 0xFF;
    packet[2] = 0xFF;
    packet[3] = 0xFF;
    packet[4] = 0xFF;
    packet[5] = 0xFF;

    for (i = 6; i < length; i++) {
        packet[i] = c++;
    }
}

static int 
packet_check (char *packet, int length)
{
    char c = (char) length;
    int i;

    for (i = 6; i < length; i++) {
        if (packet[i] != c++) {
            if (ether_debug_flag) {
                post_log("\nPacket size %d,"
                    " data received/expected at location %d = %02x/%02x\n",
                    length, i, packet[i], c-1);
                post_log("Packet check contents are...\n");
                for (i = 0 ; i < length; i++) {
                    if ((i != 0) && ((i % 16) == 0)) 
                        post_log("\n");
                    post_log("%02x ", packet[i]);
                }
                post_log("\n");
            }
            return -1;
        }
    }
    return 0;
}

/*
 *This routine checks for mac,phy and external loop back mode
 */
int 
ether_loopback(char *dev_name, int loop_type, int max_pkt_len)
{
    int i, res = -1, len_rx, init_done = 0, ret = 0;
    bd_t *bd = gd->bd;

    if (max_pkt_len > MAX_PACKET_LENGTH)
        max_pkt_len = MAX_PACKET_LENGTH;

    lb_status.status = 0;
    lb_status.field.loopType = loop_type;

    if (loop_type == LB_MAC) { eth0_mac_loopback(1);}
    else if (loop_type == LB_PHY) { eth0_phy_loopback(1); }
    else if (loop_type == LB_EXT_10) { eth0_ext_loopback(1, 0); }
    else if (loop_type == LB_EXT_100) { eth0_ext_loopback(1, 1); }
    else if (loop_type == LB_EXT_1000) { eth0_ext_loopback(1, 2); }
    else
        return (-1);

    eth_halt();
    init_done = eth_init(bd);
    
    if (init_done) {
        if (loop_type == LB_MAC) { res = 0; }
        else if (loop_type == LB_PHY) {  res = eth0_mac_link(); }
        else if ((loop_type == LB_EXT_10) ||
            (loop_type == LB_EXT_100) ||
            (loop_type == LB_EXT_1000)) { 
            res = eth0_ext_link(); 
        }
    }

    if ((init_done) && (res == 0)) {
        for (i = MIN_PACKET_LENGTH; i <= max_pkt_len; i++) {
            lb_status.field.pktSize = i;
            packet_fill (packet_send, i);
            eth_send(packet_send, i);
            if (tx_err) {
                lb_status.field.TxErr = tx_err;
                post_log("%s: tx (%s)\n", dev_name, tx_err_msg);
                ret = -1;
                break;
            }
            udelay(5000);
            len_rx = eth_receive(packet_recv, i+2);  /* +checksum */
            if (rx_err) {
                lb_status.field.RxErr = rx_err;
                post_log("%s: tx (%s)\n", dev_name, rx_err_msg);
                ret = -1;
                break;
            }
            if (len_rx != i+2 || (res = packet_check (packet_recv, i) < 0)) {
                ret = -1;
                break;
            }
        }
    }
    else
        ret = -1;
    eth_halt();
    if (loop_type == LB_MAC) { eth0_mac_loopback(0); }
    else if (loop_type == LB_PHY) { eth0_phy_loopback(0); }
    else if (loop_type == LB_EXT_10) { eth0_ext_loopback(0, 0); }
    else if (loop_type == LB_EXT_100) { eth0_ext_loopback(0, 1); }
    else if (loop_type == LB_EXT_1000) { eth0_ext_loopback(0, 2); }
    
    return ret;
}

/*
 * Test ctlr routine for egiga
 */
static int 
test_loopback (struct ether_device* eth_dev)
{
    int ether_result[NUM_ETHER_DEVICE][MAX_LB_TYPES];
    int i = 0, j = 0, fail = 0;

    memset(ether_result, 0, NUM_ETHER_DEVICE * MAX_LB_TYPES * sizeof(int));

    for (i = 0; eth_dev[i].name; i++) {
        for (j = 0; j <MAX_LB_TYPES; j++) {
            if (eth_dev[i].capability & (1<<j)) {
                if ((ether_result[i][j] = ether_loopback(eth_dev[i].name, j, POST_MAX_PACKET_LENGTH)))
                    fail = 1;
            }
        }
    }

    if (boot_flag_post) {
        if (!fail)
            return 0;
        return -1;
    }
    
    /* egiga diag result printing */
    if (!fail) {
        post_log("-- Egiga test                           PASSED\n");
        if (ether_debug_flag) {
            for (i = 0; eth_dev[i].name; i++) {
                for (j = 0; j < MAX_LB_TYPES; j++) {
                    if (eth_dev[i].capability & (1<<j))
                        post_log(" > %-6s : %-25s   PASSED\n",eth_dev[i].name, mode_test_string[j]);
                }
            }
        }
    }
    else {
        post_log("-- Egiga test                           FAILED @\n");
        if (ether_debug_flag) {
            for (i = 0; eth_dev[i].name; i++) {
                for (j = 0; j<MAX_LB_TYPES; j++) {
                    if (eth_dev[i].capability & (1<<j)) {
                        if (ether_result[i][j] == -1) {
                            post_log(" > %-6s : %-25s   FAILED @\n",eth_dev[i].name, mode_test_string[j]);
                        }
                        else {
                            post_log(" > %-6s : %-25s   PASSED\n",eth_dev[i].name, mode_test_string[j]);
                        }
                    }
                }
            }
        }
    }

    if (!fail)
        return 0;
    return -1;
}	

int 
ether_post_test (int flags)
{
    int res = 0;
    boot_flag_post = 0;
    ether_debug_flag =0;
    post_result_ether = 0;

    if (flags & POST_DEBUG) {
        ether_debug_flag = 1;
    }
    if (flags & BOOT_POST_FLAG) {
        boot_flag_post = 1;
        ether_debug_flag = 0;
    }
    
    eth_set_current();
    if (ether_debug_flag) {
        post_log("This tests the functionality of egiga in MAC,PHY\n");
        post_log(" and external loopBack. \n");
        post_log(" Please insert a loopBack cable in the mgmt port. \n\n");
    }
    
    if (boot_flag_post) {
        res = test_loopback(post_ether_dev);
    }
    else {
        res = test_loopback(ether_dev);
    }

    return res;
}

uint32_t 
diag_ether_loopback(int loopback)
{
    int i, result = 0;
    int loop_ctrl = LB_PHY + 1;
    
    if (loopback)
        loop_ctrl = MAX_LB_TYPES;

    eth_set_current();

    diag_eth0 = 1;
    /* skip MAC loopback for Dragon */
    for (i = LB_PHY; i < loop_ctrl; i++) {
        if (ether_loopback(post_ether_dev[0].name, i, MAX_PACKET_LENGTH)) {
            result = (lb_status.field.loopType << 28) + (lb_status.field.pktSize << 16) +
                (lb_status.field.TxErr << 8) + lb_status.field.RxErr;
//            result = lb_status.status;
            break;
        }
    }
    diag_eth0 = 0;

    return result;
}

#endif /* CONFIG_POST & CFG_POST_ETHER */
#endif /* CONFIG_POST */





