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

/*
 * syspld
 */

#ifndef __EX2200_SYSPLD__
#define __EX2200_SYSPLD__

enum {
    SYSPLD_REVISION = 0x00,
    SYSPLD_SCRATCH = 0x01,
    SYSPLD_CREATE_DAY = 0x02,
    SYSPLD_CREATE_MONTH = 0x03,
    SYSPLD_CREATE_YEAR_1 = 0x04,
    SYSPLD_CREATE_YEAR_2 = 0x05,
    SYSPLD_RESET_BLOCK_UNLOCK_1 = 0x08,
    SYSPLD_RESET_BLOCK_UNLOCK_2 = 0x09,
    SYSPLD_RESET_REG_1 = 0x0A,
    SYSPLD_RESET_REG_2 = 0x0B,
    SYSPLD_RESET_REG_3 = 0x0C,
    SYSPLD_RESET_REG_4 = 0x0D,
    SYSPLD_WATCHDOG_ENABLE = 0x10,
    SYSPLD_WATCHDOG_L1_TIMER = 0x11,
    SYSPLD_WATCHDOG_L2_TIMER = 0x12,
    SYSPLD_WATCHDOG_L3_TIMER = 0x13,
    SYSPLD_CHJ2_INIT = 0x14,
    SYSPLD_INT_STATUS = 0x15,
    SYSPLD_INT_ENABLE = 0x16,
    SYSPLD_UPLINK_1 = 0x17,
    SYSPLD_UPLINK_2 = 0x18,
    SYSPLD_UPLINK_LED = 0x19,
    SYSPLD_SYS_LED = 0x1A,
    SYSPLD_MISC = 0x1B,
    SYSPLD_LAST_RESET = 0x1C,
    SYSPLD_I2C_STATUS = 0x1D,
    SYSPLD_ST_ALARM_BLINK = 0x1F,
    SYSPLD_LED_PORT_0_1 = 0x20,
    SYSPLD_LED_PORT_2_3 = 0x21,
    SYSPLD_LED_PORT_4_5 = 0x22,
    SYSPLD_LED_PORT_6_7 = 0x23,
    SYSPLD_LED_PORT_8_9 = 0x24,
    SYSPLD_LED_PORT_10_11 = 0x25,
    SYSPLD_LED_PORT_12_13 = 0x26,
    SYSPLD_LED_PORT_14_15 = 0x27,
    SYSPLD_LED_PORT_16_17 = 0x28,
    SYSPLD_LED_PORT_18_19 = 0x29,
    SYSPLD_LED_PORT_20_21 = 0x2A,
    SYSPLD_LED_PORT_22_23 = 0x2B,
    SYSPLD_LED_PORT_24_25 = 0x2C,
    SYSPLD_LED_PORT_26_27 = 0x2D,
    SYSPLD_LED_PORT_28_29 = 0x2E,
    SYSPLD_LED_PORT_30_31 = 0x2F,
    SYSPLD_LED_PORT_32_33 = 0x30,
    SYSPLD_LED_PORT_34_35 = 0x31,
    SYSPLD_LED_PORT_36_37 = 0x32,
    SYSPLD_LED_PORT_38_39 = 0x33,
    SYSPLD_LED_PORT_40_41 = 0x34,
    SYSPLD_LED_PORT_42_44 = 0x35,
    SYSPLD_LED_PORT_44_45 = 0x36,
    SYSPLD_LED_PORT_46_47 = 0x37,
    SYSPLD_ST_ALARM_BLINK_C = 0x3c,
};

#define SYSPLD_FIRST_RESET_BLOCK_REG   (SYSPLD_RESET_REG_1)
#define SYSPLD_LAST_RESET_BLOCK_REG   (SYSPLD_WATCHDOG_L3_TIMER)
#define SYSPLD_FIRST_PORT_LED_REG       (SYSPLD_LED_PORT_0_1)
#define SYSPLD_LAST_PORT_LED_REG       (SYSPLD_LED_PORT_46_47)
#define SYSPLD_LAST_REG                (SYSPLD_ST_ALARM_BLINK_C)

/* EPLD_RESET_BLOCK_UNLOCK */
#define SYSPLD_MAGIC_0 0x1A
#define SYSPLD_MAGIC_1 0x2B

/* SYSPLD_RESET_REG_1 */
#define SYSPLD_CHJ2_2_CPU_RESET (1<<7)
#define SYSPLD_CHJ2_2_SW_RESET (1<<6)
#define SYSPLD_CHJ2_1_CPU_RESET (1<<5)
#define SYSPLD_CHJ2_1_SW_RESET (1<<4)
#define SYSPLD_RSTBLK_UNLOCK (1<<3)
#define SYSPLD_PLD_OT (1<<2)
#define SYSPLD_SYS_RESET_HOLD (1<<1)
#define SYSPLD_SYS_RESET (1<<0)

/* SYSPLD_RESET_REG_2 */
#define SYSPLD_POECTRL_ENABLE (1<<5)
#define SYSPLD_MGMT_PHY_ENABLE (1<<4)
#define SYSPLD_USB_HUB_ENABLE (1<<3)
#define SYSPLD_NAND_FLASH_ENABLE (1<<2)
#define SYSPLD_USB_HUB_VCC_ENABLE (1<<1)
#define SYSPLD_MAC_EEPROM_VCC_ENABLE (1<<0)

/* SYSPLD_RESET_REG_3 */
#define SYSPLD_QUAD_PHY_20_23_ENABLE (1<<5)
#define SYSPLD_QUAD_PHY_16_19_ENABLE (1<<4)
#define SYSPLD_QUAD_PHY_12_15_ENABLE (1<<3)
#define SYSPLD_QUAD_PHY_8_11_ENABLE (1<<2)
#define SYSPLD_QUAD_PHY_4_7_ENABLE (1<<1)
#define SYSPLD_QUAD_PHY_0_3_ENABLE (1<<0)

/* SYSPLD_RESET_REG_4 */
#define SYSPLD_QUAD_PHY_44_47_ENABLE (1<<5)
#define SYSPLD_QUAD_PHY_40_43_ENABLE (1<<4)
#define SYSPLD_QUAD_PHY_36_39_ENABLE (1<<3)
#define SYSPLD_QUAD_PHY_32_35_ENABLE (1<<2)
#define SYSPLD_QUAD_PHY_28_31_ENABLE (1<<1)
#define SYSPLD_QUAD_PHY_24_27_ENABLE (1<<0)


/* SYSPLD_WATCHDOG_ENABLE */
#define SYSPLD_WDOG_ENABLE (1<<7)
#define SYSPLD_WDOG_LEVEL (3<<0)

/* SYSPLD_CHJ2_INIT */
#define SYSPLD_CHJ2_2_INIT_DONE (1<<6)
#define SYSPLD_CHJ2_2_ENABLE (1<<5)
#define SYSPLD_CHJ2_2_SKIP_INIT (1<<4)
#define SYSPLD_CHJ2_1_INIT_DONE (1<<2)
#define SYSPLD_CHJ2_1_ENABLE (1<<1)
#define SYSPLD_CHJ2_1_SKIP_INIT (1<<0)

/* SYSPLD_INT_STATUS */

/* SYSPLD_INT_ENABLE */

/* SYSPLD_UPLINK_1 */
#define SYSPLD_SFP_1_TX_ENABLE (1<<7)
#define SYSPLD_SFP_1_TX_FAULT (1<<6)
#define SYSPLD_SFP_1_LOS (1<<5)
#define SYSPLD_SFP_1_PRESENT (1<<4)
#define SYSPLD_SFP_0_TX_ENABLE (1<<3)
#define SYSPLD_SFP_0_TX_FAULT (1<<2)
#define SYSPLD_SFP_0_LOS (1<<1)
#define SYSPLD_SFP_0_PRESENT (1<<0)

/* SYSPLD_UPLINK_2 */
#define SYSPLD_SFP_3_TX_ENABLE (1<<7)
#define SYSPLD_SFP_3_TX_FAULT (1<<6)
#define SYSPLD_SFP_3_LOS (1<<5)
#define SYSPLD_SFP_3_PRESENT (1<<4)
#define SYSPLD_SFP_2_TX_ENABLE (1<<3)
#define SYSPLD_SFP_2_TX_FAULT (1<<2)
#define SYSPLD_SFP_2_LOS (1<<1)
#define SYSPLD_SFP_2_PRESENT (1<<0)

/* SYSPLD_UPLINK_LED */
#define SYSPLD_MODE_3_LED_ON (1<<7)
#define SYSPLD_MODE_2_LED_ON (1<<6)
#define SYSPLD_MODE_1_LED_ON (1<<5)
#define SYSPLD_MODE_0_LED_ON (1<<4)
#define SYSPLD_SPEED_3_LED_ON (1<<3)
#define SYSPLD_SPEED_2_LED_ON (1<<2)
#define SYSPLD_SPEED_1_LED_ON (1<<1)
#define SYSPLD_SPEED_0_LED_ON (1<<0)

/* SYSPLD_SYS_LED */
#define SYSPLD_MGMT_LINK (1 << 7)
#define SYSPLD_MGMT_10MB (0 << 5)
#define SYSPLD_MGMT_100MB (2 << 5)
#define SYSPLD_MGMT_FULL_DUPLEX (1 << 4)
#define SYSPLD_SYS_STATUS_RED_ON (1 << 3)
#define SYSPLD_SYS_STATUS_GREEN_ON (1 << 2)
#define SYSPLD_SYS_ALARM_RED_ON (1 << 1)
#define SYSPLD_SYS_ALARM_GREEN_ON (1 << 0)

/* SYSPLD_MISC */
#define SYSPLD_2_5V_NORMAL (0<<6)
#define SYSPLD_2_5V_NEG_5 (1<<6)
#define SYSPLD_2_5V_POS_5 (2<<6)
#define SYSPLD_1V_NORMAL (0<<4)
#define SYSPLD_1V_NEG_5 (1<<4)
#define SYSPLD_1V_POS_5 (2<<4)
#define SYSPLD_BUTTON_PRESS (1<<3)
#define SYSPLD_RPS_PRESENT (1<<2)
#define SYSPLD_POE_ENABLE (1<<1)
#define SYSPLD_POE_PRESENT (1<<0)

/* SYSPLD_LAST_RESET */
#define SYSPLD_POWER_ON_RESET (1<<4)
#define SYSPLD_WDOG_CPU_RESET (1<<3)
#define SYSPLD_WDOG_GLOBAL_SYSTEM_RESET (1<<2)
#define SYSPLD_HARD_RESET (1<<1)
#define SYSPLD_SOFT_RESET (1<<0)

/* SYSPLD_I2C_STATUS */
#define SYSPLD_I2C_PROTOCOL_ERR (1<<7)
#define SYSPLD_I2C_ADDR_ERR (1<<6)
#define SYSPLD_I2C_TIMEOUT (1<<0)

/* SYSPLD_ST_ALARM_BLINK */
#define SYSPLD_STS_LED_BLINK (1<<1)
#define SYSPLD_STS_LED_BLINK_C (1<<2)

/* Port LED */
#define SYSPLD_PORT_LINK_UP (1<<3)
#define SYSPLD_PORT_NO_LINK (0<<3)
#define SYSPLD_PORT_SPEED_10M (0<<1)
#define SYSPLD_PORT_SPEED_100M (1<<1)
#define SYSPLD_PORT_SPEED_1G (2<<1)
#define SYSPLD_PORT_HALF_DUPLEX (0<<0)
#define SYSPLD_PORT_FULL_DUPLEX (1<<0)

extern int syspld_reg_read(int reg, uint8_t *val);
extern int syspld_reg_write(int reg, uint8_t val);
extern int syspld_read(int32_t offset, uint8_t *data, int32_t length);

#endif /*__EX2200_SYSPLD__*/
