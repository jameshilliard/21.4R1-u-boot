/*
 * Copyright (c) 2010-2013, Juniper Networks, Inc.
 * All rights reserved.
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
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/gpl-2.0.html>.
 */

#ifndef __CPLD_H__
#define __CPLD_H__

/* boot cpld register definitions */
#define BOOT_FPGA_VER			0x00
#define BOOT_RST_CTL			0x01
#define BOOT_RST_REASON			0x02
#define BOOT_CONTROL			0x03
#define BOOT_WD_INTR_CNT		0x04
#define BOOT_WD_TIMER_HI		0x05
#define BOOT_WD_TIMER_LOW		0x06
#define BOOT_LC_STATUS			0x08
#define BOOT_FLASH_WP			0x09
#define BOOT_BOOT_CTLSTAT		0x0a
#define BOOT_BOOT_FAIL			0x0b
#define BOOT_MSTR_CHG_STATUS	0x0e
#define BOOT_INT_CTL			0x0f
#define BOOT_WD_STATUS			0x12
#define BOOT_WD_ENABLE			0x13

/* JSCB_BOOT_RST_CTL - 0x01 */
#define BOOT_CPU_RESET			(1 << 0)
#define BOOT_DRAM_RESET_L		(1 << 4)
#define BOOT_FLASH_RESET		(1 << 6)
#define BOOT_PEX_RESET			(1 << 7)

/* JSCB_BOOT_RST_REASON - 0x02 */
#define BOOT_CPLD_WD_RST		(1 << 3)
#define BOOT_FLASH_SWZL_RST		(1 << 4)
#define BOOT_POR				(1 << 5)
#define BOOT_CPU_WD_RST			(1 << 6)
#define BOOT_SW_RST				(1 << 7)

/* JSCB_BOOT_CONTROL - 0x03 */
#define BOOT_CPU_UP_LED			(1 << 0)
#define BOOT_WDT_EN			(1 << 6)

/* JSCB_BOOT_LC_STATUS - 0x08 */
#define BOOT_SLOT_ID_MASK		(0x1f)
#define BOOT_STANDALONE_JMPR	(1 << 6)
#define BOOT_RE_MASTER			(1 << 7)

/* BOOT_BOOT_CTLSTAT - 0x0a */
#define BOOT_OK					(1 << 0)
#define BOOT_PARTITION			(1 << 1)
#define BOOT_FLASH_UPGRADE		(1 << 2)
#define BOOT_SET_NEXT_PARTITION	(1 << 6)
#define BOOT_NEXT_PARTITION		(1 << 7)

/* BOOT_BOOT_FAIL - 0x0b */
#define BOOT_FAILED				(1 << 0)


/* LC control cpld register definitions */
#define JLC_CTL_SCRATCH		0x00
#define JLC_CTL_CJ_RESET	0x3c
#define JLC_CTL_POE_CNTL	0x62

/* LC Control cpld POE Reset bit */
#define POE_RESET			0x1

/* 48F LC XCPLD register definitions */
#define XCPLD_MAJOR_VER		0x02
#define XCPLD_MINOR_VER		0x44

#endif /* __CPLD_H__ */
