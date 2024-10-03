/*
 * Copyright (c) 2009-2010, Juniper Networks, Inc.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */


int tbbcpld_valid_read_reg(int reg);
int tbbcpld_valid_write_reg(int reg);
extern int tbbcpld_reg_read(int reg, uint8_t *val);
extern int tbbcpld_reg_write(int reg, uint8_t val);
extern int tbbcpld_version(void);
int tbbcpld_powercycle_reset(void);
int tbbcpld_system_reset(void);
void tbbcpld_eeprom_enable(void);
void tbbcpld_eeprom_disable(void);
void tbbcpld_watchdog_disable(void);
void tbbcpld_watchdog_enable(void);
void tbbcpld_watchdog_tickle(void);
void tbbcpld_watchdog_init(void);
void tbbcpld_swizzle_enable(void);
void tbbcpld_swizzle_disable(void);
void tbbcpld_reg_dump_all(void);
void tbbcpld_scratchpad_write(u_int8_t scratch1, u_int8_t scratch2);
void tbbcpld_scratchpad_read(u_int8_t *scratch1, u_int8_t *scratch2);
void tbbcpld_init(void);
void set_noboot_flag(void);
