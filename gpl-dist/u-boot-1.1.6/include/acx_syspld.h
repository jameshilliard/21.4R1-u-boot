/*
 * $Id$
 *
 * acx_syspld.h -- ACX syspld routines declarations
 *
 * Rajat Jain, Feb 2011
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

extern void syspld_reg_dump_all(void);
extern uint8_t syspld_reg_read(unsigned int reg);
extern void syspld_reg_write(unsigned int reg, uint8_t val);

extern uint8_t syspld_scratch1_read(void);
extern void syspld_scratch1_write(uint8_t val);
extern void syspld_scratch2_write(uint8_t val);

extern int syspld_version(void);
extern int syspld_revision(void);

extern int syspld_get_logical_bank(void);
extern void syspld_set_logical_bank(uint8_t bank);


extern void syspld_self_reset(void);
extern void syspld_cpu_reset(void);

extern void syspld_watchdog_disable(void);
extern void syspld_watchdog_enable(void);
extern void syspld_watchdog_tickle(void);
extern void syspld_set_wdt_timeout(uint8_t);
extern uint8_t syspld_get_wdt_timeout(void);
extern uint8_t syspld_was_watchdog_reset(void);

extern void syspld_swizzle_enable(void);
extern void syspld_swizzle_disable(void);

extern void syspld_init(void);
extern void syspld_reset_usb_phy(void);

#define RESET_REASON_POWERCYCLE     0
#define RESET_REASON_WATCHDOG       1
#define RESET_REASON_HW_INITIATED   2
#define RESET_REASON_SW_INITIATED   3
#define RESET_REASON_SWIZZLE        4

extern uint8_t syspld_get_reboot_reason(void);

extern uint8_t syspld_get_gcr(void);
extern uint8_t syspld_set_gcr(uint8_t);
extern int syspld_is_glass_safe_open(void);
extern int syspld_close_glass_safe(void);
extern int syspld_nor_flash_wp_set(uint8_t);
extern int syspld_nvram_wp(int);
extern int syspld_tpm_pp_control(int);
extern int syspld_cpld_update(int);
extern int syspld_push_button_detect(void);
extern int syspld_jumper_detect(void);
extern void syspld_assert_sreset(void);
extern int syspld_cpu_only_reset(void);
extern int syspld_uart_rx_control(int);
extern int syspld_uart_tx_control(int);
extern int syspld_uart_get_rx_status(void);
extern int syspld_uart_get_tx_status(void);

extern int syspld_chk_tpm_busy(void);
extern void syspld_set_lpc_addr_byte1(uint8_t addr_val);
extern void syspld_set_lpc_addr_byte2(uint8_t addr_val);
extern void syspld_start_lpc_mem_read_trans(void);
extern void syspld_start_lpc_mem_write_trans(void);
extern int syspld_get_trans_result(void);
extern uint8_t syspld_get_lpc_din(void);
extern void syspld_set_lpc_dout(uint8_t data_out);
