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
 * ex43xx_local
 *
 */

#ifndef __EX43XX_LOCAL__
#define __EX43XX_LOCAL__

typedef enum flash_banks {
    FLASH_BANK0 = 0,
    FLASH_BANK1
} flash_banks_t;

typedef enum upgrade_state {
    INITIAL_STATE_B0 = 0,           /* Set by Bank 0 u-boot */
    INITIAL_STATE_B1,               /* Set by Bank 1 u-boot */
    UPGRADE_START_B0,               /* Set by JunOS */
    UPGRADE_START_B1,               /* Set by JunOS */
    UPGRADE_CHECK,
    UPGRADE_TRY_B0,                 /* Set by Bank 1 u-boot */
    UPGRADE_TRY_B1,                 /* Set by Bank 0 u-boot */
    UPGRADE_OK_B0,                  /* Set by Bank 0 u-boot */
    UPGRADE_OK_B1,                  /* Set by Bank 1 u-boot */
    UPGRADE_STATE_END
} upgrade_state_t;

extern char version_string[];
extern int secure_eeprom_read(unsigned dev_addr, unsigned offset, 
			      uchar *buffer, unsigned cnt);

/* misc */
extern int valid_elf_image(unsigned long addr);
extern void i2c_sfp_temp(void);
extern int cmd_get_data_size(char* arg, int default_size);

int set_upgrade_state(upgrade_state_t state);
uint32_t get_upgrade_state(void);
int valid_env(unsigned long addr);
int valid_uboot_image(unsigned long addr);
flash_banks_t get_booted_bank(void);
int set_next_bootbank(flash_banks_t bank);
void boot_next_bank(void);
int set_curr_bank0(void);
int set_curr_bank1(void);
int enable_both_banks(void);
int enable_single_bank_access(void);
#endif /*__EX43XX_LOCAL__*/
