/*
 * Copyright (c) 2008-2011, Juniper Networks, Inc.
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
 */

#ifndef __RPS__
#define __RPS__

#include <linux/types.h>

#undef CONFIG_PRODUCTION

/* tick */
#define MAX_TICKS 0xffffffff
extern uint32_t gTicksPerMS;
extern uint32_t gMaxMS;

/* PLD */
#define PLD_CACHE_ENABLE

#define RPS_MAIN_MUX 0x77
#define CPLD_CHANNEL 6
#define CPLD_I2C_ADDRESS 0x31

/* CPU slave */
#define CFG_I2C_CPU_SLAVE_ADDRESS   0x60

/* WinBond */
#define ENV_CHANNEL 6
#define CFG_I2C_HW_MON_ADDR          0x29 /* channel 6 */

/* EEPROM */
#define EEPROM_CHANNEL 6
#define CFG_EEPROM_ADDR          0x52 /* channel 6 */

/* power supply */
#define PS_I2C_ADDR 0x51
#define PS_ID_OFFSET 4
#define PS_ID_SIZE 2
#define PS_EEPROM_CHANNEL 7
#define PS_EEPROM_MUX 0x70

/* priority */
#define RPS_MAX_PRIORITY 6

#define MAX_SN_SZ 20 /* Switch serial number */

#define swap_ushort(x) \
	({ unsigned short x_ = (unsigned short)x; \
	 (unsigned short)( \
		((x_ & 0x00FFU) << 8) | ((x_ & 0xFF00U) >> 8) ); \
	})
#define swap_ulong(x) \
	({ unsigned long x_ = (unsigned long)x; \
	 (unsigned long)( \
		((x_ & 0x000000FFUL) << 24) | \
		((x_ & 0x0000FF00UL) <<	 8) | \
		((x_ & 0x00FF0000UL) >>	 8) | \
		((x_ & 0xFF000000UL) >> 24) ); \
	})

#endif /*__RPS__*/
