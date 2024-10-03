/*
 * Copyright (c) 2011-2013, Juniper Networks, Inc.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

#ifndef __ETHER_H
#define __ETHER_H

#define MIN_PACKET_LENGTH	64
#define MAX_PACKET_LENGTH	1518
#define POST_MAX_PACKET_LENGTH	256

/* capability */
#define LB_MAC		0
#define LB_PHY1145	1
#define LB_PHY1112	2
#define LB_EXT_10	3
#define LB_EXT_100	4
#define LB_EXT_1000	5
#define MAX_LB_TYPES	6

#ifdef CONFIG_EX4500
#define NUM_TSEC_DEVICE 4
#endif

#ifdef CONFIG_EX4510
#define NUM_TSEC_DEVICE 3
#endif

extern int ether_loopback (struct eth_device *dev, int loopback, 
                           int max_pkt_len, int debug_flag, int reg_dump); 

extern int tsec_cont_send_pkts(char *dev);
extern void tsec_phy_dump(char *dev);
extern void tsec_reg_dump(char *dev);
extern void mac_reg_dump(char *dev);
extern void init_registers_private(volatile tsec_t * regs);
#endif /* __ETHER_H */
