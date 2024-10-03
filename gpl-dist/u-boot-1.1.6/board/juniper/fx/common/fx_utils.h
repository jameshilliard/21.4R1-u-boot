/*
 * Copyright (c) 2011, Juniper Networks, Inc.
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
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/gpl-2.0.html>.
 */

#ifndef __FX_UTILS__
#define __FX_UTILS__

#define SOHO_SCPLD_JTAG     0xE7

/* JTAG output pins */
#define TCK         0
#define TMS         1
#define TDI         2

/* TOR CPLD GPIO register bit definition */
#define TOR_JTAG_TDI        (unsigned char)0x1  /* output */
#define TOR_JTAG_TMS        (unsigned char)0x2  /* output */
#define TOR_JTAG_TCK        (unsigned char)0x4  /* output */
#define TOR_JTAG_TDO        (unsigned char)0x8  /* input */
#define TOR_FPGA_PROG       (unsigned char)0x10 /* output */
#define TOR_FPGA_DONE       (unsigned char)0x20 /* input */
#define TOR_FPGA_INIT_N     (unsigned char)0x40 /* output */
#define TOR_FPGA_JTAG_EN    (unsigned char)0x80 /* output */

/* TAP states */
#define TAP_RESET           0x00
#define TAP_RUNTEST         0x01    /* IDLE */
#define TAP_SELECTDR        0x02
#define TAP_CAPTUREDR       0x03
#define TAP_SHIFTDR         0x04
#define TAP_EXIT1DR         0x05
#define TAP_PAUSEDR         0x06
#define TAP_EXIT2DR         0x07
#define TAP_UPDATEDR        0x08
#define TAP_IRSTATES        0x09    /* All IR states begin here */
#define TAP_SELECTIR        0x09
#define TAP_CAPTUREIR       0x0A
#define TAP_SHIFTIR         0x0B
#define TAP_EXIT1IR         0x0C
#define TAP_PAUSEIR         0x0D
#define TAP_EXIT2IR         0x0E
#define TAP_UPDATEIR        0x0F

/* TAP error */
#define TAP_ERROR_NONE          0
#define TAP_ERROR_UNKNOWN       1
#define TAP_ERROR_TDOMISMATCH   2
#define TAP_ERROR_MAXRETRIES    3   /* TDO mismatch after max retries */
#define TAP_ERROR_ILLEGALCMD    4
#define TAP_ERROR_ILLEGALSTATE  5
#define TAP_ERROR_DATAOVERFLOW  6

/* misc */
#define JEDEC_ID_ILLEGIAL_1S    8
#define MAX_JTAG_DEVICES        25

#endif /*__FX_UTILS__*/
