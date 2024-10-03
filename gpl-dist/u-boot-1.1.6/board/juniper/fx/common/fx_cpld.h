
/*
 * Copyright (c) 2009-2012, Juniper Networks, Inc.
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
 * You should have received a copy of the GNU General Public
 * License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __FX_CPLD_H__
#define __FX_CPLD_H__

#include <rmi/board.h>

#define DEFAULT_RMI_PROCESSOR_IO_BASE	0xbef00000

typedef volatile u_int32_t reg_t;
typedef u_int8_t boolean;

#define FX_FLASH_BAR			0xbef00068
#define XLS_IO_DEV_FLASH_BAR		0xbef19000
#define FX_QSFP_RESET_DELAY		5000

#define CS_BASE_ADDRESS(n)              (XLS_IO_DEV_FLASH_BAR + ((n) * 4))
#define CS_ADDRESS_MASK(n)              (XLS_IO_DEV_FLASH_BAR + (((n) + 0x10) * 4))
#define CS_DEV_PARAM_ADDRESS(n)         (XLS_IO_DEV_FLASH_BAR + (((n) + 0x20) * 4))
#define CS_DEV_OE_ADDRESS(n)            (XLS_IO_DEV_FLASH_BAR + (((n) + 0x30) * 4))
#define CS_DEV_TIME_ADDRESS(n)          (XLS_IO_DEV_FLASH_BAR + (((n) + 0x40) * 4))

/*Reset Types */
#define COLD_BOOT_RESET			0x01
#define PUSH_BUTTON_RESET		0x02
#define SOFTWARE_RESET			0x04
#define WATCHDOG_TIMER_RESET		0x08
#define CPLD_RESET		 	0x10

/*CPLD Register Offsets*/
#define BOOT_KEY_REG_OFFSET		0x70
#define FULL_FLASH_MODE_REG_OFFSET	0x71
#define CPU_RESET_EN_REG_OFFSET		0x73
#define CPU_RESET_REG_OFFSET		0x74
#define CPU_RESET_STATUS_REG_OFFSET	0x75
#define WATCHDOG_REG_OFFSET		0x76
#define CPLD_VERSION_REG_OFFSET		0x7C

/*CPLD QSFP Registers */
#define QSFP_PHY_RST_REG0_OFFSET	0xE0
#define QSFP_PHY_RST_REG1_OFFSET	0xE1
#define QSFP0_SLAVE_REG_OFFSET		0x80
#define QSFP8_SLAVE_REG_OFFSET		0x90
#define QSFP7_SLAVE_REG_OFFSET		0x88
#define QSFP15_SLAVE_REG_OFFSET		0x98

/*Upgrade status values */
#define UPGRADE_START			0x01
#define UPGRADE_ONE_SUCCESS		0x02
#define UPGRADE_TWO_START		0x03
#define UPGRADE_COMPLETE		0x04
#define UPGRADE_ONE_FAIL		0x05
#define UPGRADE_TWO_FAIL		0x06


#define NUM_TRIES_THRESHOLD		0x03
#define BOOT_KEY_VALUE			0x5A
#define EEPROM_VALID_KEY		0xA5

#define FX_BOOT_FLASH_CS        0
#define FX_MCPLD_CS             1
#define PA_SCPLD_CS             2
#define PA_ACC_FPGA_CS          3
#define SOHO_RESET_CPLD_CS      3
#define PA_RESET_CPLD_CS        4 

extern uint8_t g_log_eprom_bus;
extern struct pio_dev fx_mcpld_dev;
extern struct pio_dev tor_scpld_dev;
extern struct pio_dev pa_acc_fpga_dev;
extern struct pio_dev soho_reset_cpld_dev;
extern struct pio_dev cfi_flash_dev;


void fx_cpld_init (struct pio_dev *mcpld, struct pio_dev *scpld, struct pio_dev *acc_fpga,
              struct pio_dev *reset_cpld);
void fx_cpld_early_init (void);
void fx_cpld_set_bootkey (void);
uint8_t fx_cpld_get_reset_type (void);
uint8_t fx_cpld_get_current_partition (void);
void fx_cpld_set_next_boot_partition (uint8_t partition);
void fx_cpld_issue_sw_reset (void);
void fx_cpld_enable_watchdog (uint8_t enable, uint8_t value);
uint8_t fx_cpld_get_full_flash_mode (void);
void fx_cpld_enable_full_flash_mode (void);
void fx_cpld_disable_full_flash_mode (void);
uint8_t fx_cpld_get_version (void);
void fx_set_boot_status (uint32_t status);
uint32_t fx_get_boot_status (void);
void fx_set_valid_key (void);
int fx_cpld_write (u_int8_t *cpld_base, u_int8_t offset, u_int8_t value);
u_int8_t fx_cpld_read (u_int8_t *cpld_base, u_int8_t offset);
void reload_watchdog (int);
void fx_cpld_dump (void);
void fx_cpld_reset_to_next_partition (void);
void fx_cpld_qsfp_enable(void);
#endif //__FX_CPLD_H__
