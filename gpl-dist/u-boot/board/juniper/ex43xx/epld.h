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
 *  EX43XX EPLD
 *
 */

#ifndef __EPLD__
#define __EPLD__

enum {
    EPLD_RESET_BLOCK_UNLOCK = 0x00,		/* 0x0 */
    EPLD_CPU_RESET = 0x01,			/* 0x2 */
    EPLD_SYSTEM_RESET = 0x02,			/* 0x4 */
    EPLD_WATCHDOG_ENABLE = 0x03,		/* 0x6 */
    EPLD_WATCHDOG_SW_UPDATE = 0x04,		/* 0x8 */
    EPLD_WATCHDOG_L1_TIMER = 0x05,		/* 0xa */
    EPLD_WATCHDOG_L2_TIMER = 0x06,		/* 0xc */
    EPLD_WATCHDOG_L3_TIMER = 0x07,		/* 0xe */
    EPLD_WATCHDOG_L4_TIMER = 0x08,		/* 0x10 */ /* NOT SUPPORTED */
    EPLD_WATCHDOG_LEVEL_DEBUG = 0x09,		/* 0x12 */
    EPLD_INT_STATUS1 = 0x0A,			/* 0x14 */ /* NOT SUPPORTED */
    EPLD_INT_STATUS2 = 0x0B,			/* 0x16 */
    EPLD_INT_MASK1 = 0x0C,			/* 0x18 */ /* NOT SUPPORTED */
    EPLD_INT_MASK2 = 0x0D,			/* 0x1a */ /* NOT SUPPORTED */
    EPLD_LCD_DATA = 0x0E,			/* 0x1c */
    EPLD_OUT_INT_MASK = 0x0F,			/* 0x1e */ /* NOT USED */
    EPLD_SYS_LED = 0x10,			/* 0x20 */
    EPLD_MISC_CONTROL = 0x11,			/* 0x22 */
    EPLD_LAST_RESET = 0x12,			/* 0x24 */
    EPLD_PUSH_BUTTON = 0x13,			/* 0x26 */
    EPLD_LED_AB_PORT_1_4 = 0x14,		/* 0x28 */
    EPLD_LED_AB_PORT_5_8 = 0x15,		/* 0x2a */
    EPLD_LED_AB_PORT_9_12 = 0x16,		/* 0x2c */
    EPLD_LED_AB_PORT_13_16 = 0x17,		/* 0x2e */
    EPLD_LED_AB_PORT_17_20 = 0x18,		/* 0x30 */
    EPLD_LED_AB_PORT_21_24 = 0x19,		/* 0x32 */
    EPLD_LED_AB_PORT_25_28 = 0x1A,		/* 0x34 */
    EPLD_LED_AB_PORT_29_32 = 0x1B,		/* 0x36 */
    EPLD_LED_AB_PORT_33_36 = 0x1C,		/* 0x38 */
    EPLD_LED_AB_PORT_37_40 = 0x1D,		/* 0x3a */
    EPLD_LED_AB_PORT_41_44 = 0x1E,		/* 0x3c */
    EPLD_LED_AB_PORT_45_48 = 0x1F,		/* 0x3e */
    EPLD_LED_AB_PORT_49_52 = 0x20,		/* 0x40 */
    EPLD_LED_REAR_PORT_0_3 = 0x22,		/* 0x44 */
    EPLD_PHY_INT4 = 0x24,			/* 0x48 */ /* NOT SUPPORTED */
    EPLD_INTR_CAUSE = 0x2D,			/* 0x5A */
    EPLD_PHY_PORTS_INTR_CAUSE = 0x2E,		/* 0x5C */
    EPLD_PHY_MAIN_INTR_CAUSE = 0x2F,		/* 0x5E */
    EPLD_MOD_INTR_CAUSE = 0x30,			/* 0x60 */
    EPLD_PHY_INT_DETAIL_1 = 0x31,		/* 0x62 */
    EPLD_PHY_INT_DETAIL_2 = 0x32,		/* 0x64 */
    EPLD_PHY_INT_DETAIL_3 = 0x33,		/* 0x66 */
    EPLD_INT_MASK = 0x34,			/* 0x68 */
    EPLD_PHY_PORTS_INT_MASK = 0x35,		/* 0x6A */
    EPLD_PHY_MAIN_INT_MASK = 0x36,		/* 0x6C */
    EPLD_MOD_INT_MASK = 0x37,			/* 0x6E */
    EPLD_VOLT_MARGIN = 0x38,			/* 0x70 */
    EPLD_FRONT_LED_CTRL = 0x39,			/* 0x72 */
    EPLD_MOD_PRES_PWR_EN_CTRL = 0x3A,		/* 0x74 */
    EPLD_MOD_CTRL_2_STATUS = 0x3B,		/* 0x76 */
    EPLD_FAN_CTRL_0_1 = 0x3C,			/* 0x78 */
    EPLD_FAN_SPD_0 = 0x3E,			/* 0x7C */
    EPLD_FAN_SPD_1 = 0x3F,			/* 0x7E */
    EPLD_BOOT_CTL = 0x42,			/* 0x84 */
    EPLD_LAST = 0x43				/* 0x86 */
};

#define EPLD_LAST_RESET_BLOCK_REG	(EPLD_WATCHDOG_L4_TIMER)
#define EPLD_LAST_REG			(EPLD_LAST)
#define EPLD_FIRST_PORT_LED_REG		(EPLD_LED_AB_PORT_1_4)

/* EPLD_RESET_BLOCK_UNLOCK */
#define EPLD_MAGIC 0x1A2B

/* EPLD_CPU_RESET = 0x01 */
#define EPLD_CPU_HARD_RESET		(1<<1)
#define EPLD_CPU_POWER_ON_HARD_RESET	(1<<0)

/* EPLD_SYSTEM_RESET */
#define EPLD_SYS_RESET			(1<<0)
#define EPLD_SYS_RESET_HOLD		(1<<1)
#define EPLD_NOR_FLASH_RST		(1<<3)
#define EPLD_QSFP2_RST			(1<<4)
#define EPLD_QSFP1_RST			(1<<5)
#define	EPLD_USB_CONS_RST		(1<<6)
#define EPLD_POE_RST			(1<<7)
#define EPLD_FMOD_I2C_RST		(1<<8)
#define EPLD_DDR_RST			(1<<9)
#define EPLD_PHY54380_0_RST		(1<10)
#define EPLD_PHY54380_1_RST		(1<11)
#define EPLD_PHY54380_2_RST		(1<12)
#define EPLD_PHY54380_3_RST		(1<13)
#define EPLD_PHY54380_4_RST		(1<14)
#define EPLD_PHY54380_5_RST		(1<15)

/* EPLD_WATCHDOG_ENABLE */
#define EPLD_WDOG_ENABLE		(1<<0)

/* EPLD_WATCHDOG_SW_UPDATE */
#define EPLD_WDOG_KICK			(1<<0)

/* EPLD_WATCHDOG_LEVEL_DEBUG */
#define EPLD_WDOG_LEVEL_IDLE		(1<<0)
#define EPLD_WDOG_LEVEL_ONE		(1<<1)
#define EPLD_WDOG_LEVEL_TWO		(1<<2)
#define EPLD_WDOG_LEVEL_THREE		(1<<3)
#define EPLD_WDOG_LEVEL_FOUR		(1<<4)
#define EPLD_I2C_RST_L1			(1<<5)
#define EPLD_I2C_RST_L2			(1<<6)
#define EPLD_I2C_RST_L3			(1<<7)

/* EPLD_INT_STATUS2 */
#define EPLD_PUSH_BUTTON_FRONT_BOTTOM	(1<<4)
#define EPLD_PUSH_BUTTON_FRONT_TOP	(1<<5)

/* EPLD_OUT_INT_MASK */
#define EPLD_MASK_CPLD_INT		(1<<0)
#define EPLD_MASK_PHY_PORT_INT		(1<<1)
#define EPLD_MASK_MOD_INT		(1<<2)
#define EPLD_MASK_PHY_MAIN_INT		(1<<3)

/* EPLD_SYS_LED */
#define EPLD_LCD_BACKLIGHT_EN_FRONT	(1<<0)
#define EPLD_R5H30211_RST		(1<<2)
#define EPLD_DTR			(1<<3)

/* EPLD_MISC_CONTROL */
#define EPLD_LCD_FRONT_EN		(1<<7)
#define EPLD_LCD_RS_INSTRUCTION		(0<<8)
#define EPLD_LCD_RS_DATA		(1<<8)
#define EPLD_LCD_READ			(1<<9)
#define EPLD_LCD_WRITE			(0<<9)
#define MGMT_PORT_HALF_DUPLEX		(0<<12)
#define MGMT_PORT_FULL_DUPLEX		(1<<12)
#define MGMT_PORT_SPEED_1G		(0x1<<13)
#define MGMT_PORT_SPEED_10M		(0x0<<13)
#define MGMT_PORT_SPEED_100M		(0x2<<13)

/* EPLD_LAST_RESET */
#define EPLD_GLOBAL_RESET		(1<<0)
#define EPLD_SOFT_RESET			(1<<1)
#define EPLD_HARD_RESET			(1<<2)
#define EPLD_WDOG_GLOBAL_RESET		(1<<3)
#define EPLD_WDOG_HARD_RESET		(1<<4)
#define EPLD_WDOG_SOFT_RESET		(1<<5)
#define EPLD_WDOG_NMI			(1<<6)
#define EPLD_POWER_ON_RESET		(1<<7)
#define EPLD_DCD			(1<<8)

/* EPLD_PUSH_BUTTON */
#define EPLD_BUTTON_0_DOWN		(1<<0)
#define EPLD_BUTTON_1_DOWN		(1<<1)
#define EPLD_BUTTON_2_DOWN		(1<<2)
#define EPLD_BUTTON_3_DOWN		(1<<3)
#define EPLD_BUTTON_4_DOWN		(1<<4)
#define EPLD_BUTTON_5_DOWN		(1<<5)
#define EPLD_BUTTON_6_DOWN		(1<<6)
#define EPLD_BUTTON_7_DOWN		(1<<7)
#define EPLD_REAR_LCD_POWER_OFF		(1<<10)
#define EPLD_PCI_USB_POWER_OFF		(1<<11)
#define EPLD_1_1V_NORMAL		(0<<12)
#define EPLD_1_1V_NEG_5			(0x1<<12)
#define EPLD_1_1V_POS_5			(0x2<<12)
#define EPLD_2_5V_NORMAL		(0<<14)
#define EPLD_2_5V_NEG_5			(0x1<<14)
#define EPLD_2_5V_POS_5			(0x2<<14)

/* Port LED */
#define EPLD_PORT_HALF_DUPLEX		(0<<3)
#define EPLD_PORT_FULL_DUPLEX		(1<<3)
#define EPLD_PORT_SPEED_10M		(0x1<<0)
#define EPLD_PORT_SPEED_100M		(0x2<<0)
#define EPLD_PORT_SPEED_1G		(0x3<<0)
#define EPLD_PORT_SPEED_10G		(0x4<<0)
#define EPLD_PORT_NO_LINK		(0<<0)

/* EPLD_LED_MISC_CTRL */
#define EPLD_LED_M1_80_PMODE		(1<<0)

/* SYSTEM LED GREEN BITS */
#define SYS_LED_GREEN_ON		(0x4)
#define SYS_LED_GREEN_OFF		(0x0)

/* EPLD_FAN_CTRL_0_1 */
#define FAN_0_PRNT			(1<<0)
#define FAN_0_DIR			(1<<1)
#define FAN_0_LED_ON			(1<<2)
#define FAN_0_DEFAULT_SPEED		(0xA<<4)
#define FAN_0_MASK_BITS  		(0xFF0F)
#define FAN_1_PRNT			(1<<8)
#define FAN_1_DIR			(1<<9)
#define FAN_1_LED_ON			(1<<10)
#define FAN_1_DEFAULT_SPEED		(0xA<<12)
#define FAN_1_MASK_BITS 		(0x0FFF)

/* EPLD_BOOT_CTL */
#define EPLD_SWIZZLE_EN			(1 << 5)
#define EPLD_ENTIRE_FLASH_ACCESS	(1 << 3)
#define EPLD_BOOT_FAILURE		(1 << 2)
#define EPLD_CURR_PARTITION		(1 << 1)
#define EPLD_BOOT_OK			(1 << 0)

extern int epld_reg_read(int reg, uint16_t *val);
extern int epld_reg_write(int reg, uint16_t val);

#endif /*__EPLD__*/
