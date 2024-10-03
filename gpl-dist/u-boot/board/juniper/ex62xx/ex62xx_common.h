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

#ifndef __EX62XX_COMMON_H__
#define __EX62XX_COMMON_H__

typedef enum flash_banks {
	FLASH_BANK0 = 0,
	FLASH_BANK1
} flash_banks_t;

typedef enum upgrade_state {
	INITIAL_STATE_B0 = 0,	/* Set by Bank 0 u-boot */
	INITIAL_STATE_B1,		/* Set by Bank 1 u-boot */
	UPGRADE_START_B0,		/* Set by JunOS */
	UPGRADE_START_B1,		/* Set by JunOS */
	UPGRADE_CHECK,
	UPGRADE_TRY_B0,			/* Set by Bank 1 u-boot */
	UPGRADE_TRY_B1,			/* Set by Bank 0 u-boot */
	UPGRADE_OK_B0,			/* Set by Bank 0 u-boot */
	UPGRADE_OK_B1,			/* Set by Bank 1 u-boot */
	UPGRADE_STATE_END
} upgrade_state_t;

#define TRUE	1
#define FALSE	0

struct mfgtest_list_t {
    char *name;
    char *test_desc;
    int (*test_func)(void);
};

/* PHY access functions */
#define write_phy_reg(priv, regnum, value) \
	tsec_local_mdio_write(priv->regs, priv->phyaddr, 0, regnum, value)
#define read_phy_reg(priv,regnum) \
	tsec_local_mdio_read(priv->regs, priv->phyaddr, 0, regnum)

void set_current_addr(void);
uint8_t get_master_re(void);
void set_caffeine_default_environment(void);
void ctlpld_reset_sequence(void);
void btcpld_reset_sequence(void);
int get_assembly_id_as_string(uint8_t ideprom_addr,	uint8_t* str_asmid);
uint16_t get_assembly_id(uint8_t eeprom_addr);
uint8_t get_slot_id(void);
void keep_alive_master(void);
void ex62xx_btcpld_cpu_reset(void);
int32_t set_re_state(uint8_t);
int32_t get_bp_eeprom_access(void);
int32_t get_ore_state(uint8_t*);
uint32_t get_upgrade_state(void);
int check_upgrade(void);
int set_upgrade_state(upgrade_state_t state);
int valid_env(unsigned long addr);
int valid_uboot_image(unsigned long addr);
flash_banks_t get_booted_bank(void);
int set_next_bootbank(flash_banks_t bank);
void boot_next_bank(void);
int enable_bank0(void);
int disable_bank0(void);
int set_upgrade_ok(void);
void btcpld_swizzle_disable(void);
void disable_bank0_atprompt(void);
void request_mastership_status(void);
int board_start_usb(void);
int board_init_nand(void);
void get_boot_string(void);
int mfgtest_memory(void);
int mfgtest_usb(void);
int mfgtest_wdog(void);
int do_mfgtest(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
int do_i2cs(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);

#endif /* __EX62XX_COMMON_H__ */
