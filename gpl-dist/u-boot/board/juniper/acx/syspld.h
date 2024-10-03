/*
 * $Id$
 *
 * acx_syspld.h -- ACX syspld routines declarations
 *
 * Rajat Jain, Feb 2011
 * Samuel Jacob, Sep 2011
 *
 * Copyright (c) 2011-2012, Juniper Networks, Inc.
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

#ifndef _ACX_SYSPLD_H_
#define _ACX_SYSPLD_H_

void syspld_reg_dump_all(void);
uint8_t syspld_reg_read(unsigned int reg);
void syspld_reg_write(unsigned int reg, uint8_t val);

uint8_t syspld_scratch1_read(void);
void syspld_scratch1_write(uint8_t val);
void syspld_scratch2_write(uint8_t val);

int syspld_version(void);
int syspld_revision(void);

int syspld_get_logical_bank(void);
void syspld_set_logical_bank(uint8_t bank);

void syspld_cpu_reset(void);

void syspld_watchdog_disable(void);
void syspld_watchdog_enable(void);
void syspld_watchdog_tickle(void);
void syspld_set_wdt_timeout(uint8_t);
uint8_t syspld_get_wdt_timeout(void);
uint8_t syspld_was_watchdog_reset(void);

void syspld_swizzle_enable(void);
void syspld_swizzle_disable(void);

void syspld_init(void);
void syspld_reset_usb_phy(void);

#define RESET_REASON_POWERCYCLE     0
#define RESET_REASON_WATCHDOG       1
#define RESET_REASON_HW_INITIATED   2
#define RESET_REASON_SW_INITIATED   3
#define RESET_REASON_SWIZZLE        4

uint8_t syspld_get_reboot_reason(void);

#endif

