/*
 * Copyright (c) 2011-2012, Juniper Networks, Inc.
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
 * cmd_jmem
 *
 */

#ifndef __CMD_JMEM__
#define __CMD_JMEM__

#define LOW_TO_HIGH 1
#define HIGH_TO_LOW 0
#define MAX_ALGORITHM 10
#define MAX_STAGE 9
#define ERR_PATTERN 0x12345678

#ifdef RAMTEST_64BIT
#define UT_SIZE uint64_t
#define UT_BITS 64
#else
#define UT_SIZE uint32_t
#define UT_BITS 32
#endif

void inject_error (void);
int w0 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int r0 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int r0 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int r0_w1 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int r1_w0 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int r0_w0_w1 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int r0_w1_r1 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int r1_w0_w1 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int r1_w0_r0 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int r0_w1_w0_w1 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int r1_w0_w1_w0 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int r0_w1_r1_w0_r0_w1 (int direction, UT_SIZE saddr, UT_SIZE eaddr,
                       UT_SIZE pattern);

int u_w0 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int r0 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int r1 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int r0_w1 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int r1_w0 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int r0_w1_w0 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int r0_w1_r1 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int r1_w0_w1 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int r1_w0_r0 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int r0_w1_w0_w1 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int r1_w0_w1_w0 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int r0_w1_r1_w0_r0_w1 (int direction, UT_SIZE saddr, UT_SIZE eaddr, 
                       UT_SIZE pattern);


int u_w0 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int u_r0 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int u_r1 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int u_r0_w1 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int d_r0_w1 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int u_r1_w0 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int d_r1_w0 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int d_r0_w1_w0 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int u_r0_w1_r1 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int d_r0_w1_r1 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int u_r1_w0_w1 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int u_r1_w0_r0 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int d_r1_w0_r0 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int u_r0_w1_w0_w1 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int d_r1_w0_w1_w0 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int u_r0_w1_r1_w0_r0_w1 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int p_delay (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);
int p_null (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern);


extern int memtest (ulong saddr, ulong eaddr, int mloop, int display);
extern uint32_t single_bit_ecc_errors(void);
extern uint32_t double_bits_ecc_errors(void);

#endif /*__CMD_JMEM__*/
