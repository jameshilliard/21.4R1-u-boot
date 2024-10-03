/*
 * Copyright (c) 2010, Juniper Networks, Inc.
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
 * EPLD
 *
 */

#ifndef __EPLD__
#define __EPLD__

enum {
    EPLD_RESET_BLOCK_UNLOCK = 0x00,         /* 0x0 */
    EPLD_CPU_RESET = 0x01,                  /* 0x2 */
    EPLD_SYSTEM_RESET = 0x02,               /* 0x4 */
    EPLD_WATCHDOG_ENABLE = 0x03,            /* 0x6 */
    EPLD_WATCHDOG_SW_UPDATE = 0x04,         /* 0x8 */
    EPLD_WATCHDOG_L1_TIMER = 0x05,          /* 0xa */
    EPLD_WATCHDOG_L2_TIMER = 0x06,          /* 0xc */
    EPLD_WATCHDOG_L3_TIMER = 0x07,          /* 0xe */
    EPLD_WATCHDOG_L4_TIMER = 0x08,          /* 0x10 */
    EPLD_WATCHDOG_LEVEL_DEBUG = 0x09,       /* 0x12 */
    EPLD_INT_STATUS1 = 0x0A,                /* 0x14 */
    EPLD_INT_STATUS2 = 0x0B,                /* 0x16 */
    EPLD_INT_MASK1 = 0x0C,                  /* 0x18 */
    EPLD_INT_MASK2 = 0x0D,                  /* 0x1a */
    EPLD_LCD_DATA = 0x0E,                   /* 0x1c */
    EPLD_RSVD_1 = 0x0F,                     /* 0x1e */
    EPLD_MISC_CONTROL_0 = 0x10,             /* 0x20 */
    EPLD_MISC_CONTROL = 0x11,               /* 0x22 */
    EPLD_LAST_RESET = 0x12,                 /* 0x24 */
    EPLD_PUSH_BUTTON = 0x13,                /* 0x26 */
    EPLD_LED_AB_PORT_1_4 = 0x14,            /* 0x28 */
    EPLD_LED_AB_PORT_5_8 = 0x15,            /* 0x2a */
    EPLD_LED_AB_PORT_9_12 = 0x16,           /* 0x2c */
    EPLD_LED_AB_PORT_13_16 = 0x17,          /* 0x2e */
    EPLD_LED_AB_PORT_17_20 = 0x18,          /* 0x30 */
    EPLD_LED_AB_PORT_21_24 = 0x19,          /* 0x32 */
    EPLD_LED_AB_PORT_25_28 = 0x1A,          /* 0x34 */
    EPLD_LED_AB_PORT_29_32 = 0x1B,          /* 0x36 */
    EPLD_LED_AB_PORT_33_36 = 0x1C,          /* 0x38 */
    EPLD_LED_AB_PORT_37_40 = 0x1D,          /* 0x3a */
    EPLD_LED_AB_PORT_41_44 = 0x1E,          /* 0x3c */
    EPLD_LED_AB_PORT_45_48 = 0x1F,          /* 0x3e */
    EPLD_LED_AB_PORT_49_52 = 0x20,          /* 0x40 */
    EPLD_PHY_INT1 = 0x21,                   /* 0x42 */
    EPLD_PHY_INT2 = 0x22,                   /* 0x44 */
    EPLD_PHY_INT3 = 0x23,                   /* 0x46 */
    EPLD_PHY_INT4 = 0x24,                   /* 0x48 */
    EPLD_PHY_INT_MASK_1 = 0x25,             /* 0x4A */
    EPLD_PHY_INT_MASK_2 = 0x26,             /* 0x4C */
    EPLD_PHY_INT_MASK_3 = 0x27,             /* 0x4E */
    EPLD_PHY_INT_MASK_4 = 0x28,             /* 0x50 */
    EPLD_LED_MISC_CTRL = 0x29,              /* 0x52 */
    EPLD_DUMMY_2 = 0x2A,                    /* 0x54 */
    EPLD_DUMMY_3 = 0x2B,                    /* 0x56 */
    EPLD_DUMMY_4 = 0x2C,                    /* 0x58 */
    EPLD_DUMMY_5 = 0x2D,                    /* 0x5A */
    EPLD_DUMMY_6 = 0x2E,                    /* 0x5C */
    EPLD_LAST = 0x2F                        /* 0x5E */
};

#define EPLD_LAST_RESET_BLOCK_REG   (EPLD_WATCHDOG_L4_TIMER)
#define EPLD_LAST_REG               (EPLD_LAST)
#define EPLD_FIRST_PORT_LED_REG     (EPLD_LED_AB_PORT_1_4)

/* EPLD_RESET_BLOCK_UNLOCK */
#define EPLD_MAGIC 0x1A2B

/* EPLD_CPU_RESET = 0x01 */
#define EPLD_CPU_SOFT_RESET (1<<1)
#define EPLD_CPU_HARD_RESET (1<<0)

/* EPLD_SYSTEM_RESET */
#define EPLD_PLD_OT (1<<2)
#define EPLD_SYS_RESET_HOLD (1<<1)
#define EPLD_SYS_RESET (1<<0)

/* EPLD_WATCHDOG_ENABLE */
#define EPLD_WDOG_ENABLE (1<<0)

/* EPLD_WDOG_SW_UPDATE */
#define EPLD_WDOG_KICK (1<<0)

/* EPLD_WDOG_LEVEL_DEBUG */
#define EPLD_WDOG_LEVEL_IDLE (1<<0)
#define EPLD_WDOG_LEVEL_ONE (1<<1)
#define EPLD_WDOG_LEVEL_TWO (1<<2)
#define EPLD_WDOG_LEVEL_THREE (1<<3)
#define EPLD_WDOG_LEVEL_FOUR (1<<4)
#define EPLD_I2C_RST_L1 (1<<5)
#define EPLD_I2C_RST_L2 (1<<6)
#define EPLD_I2C_RST_L3 (1<<7)

/* EPLD_INT_STATUS1 (active low) */
#define EPLD_PHY_MGMT (1<<0)
#define EPLD_PHY_88E1145 (1<<1)
#define EPLD_PHY_SFPABS (1<<2)
#define EPLD_PHY_SFPASS (1<<3)
#define EPLD_RX_LOS (1<<4)
#define EPLD_CUB (1<<5)
#define EPLD_SW_PEX (1<<6)
#define EPLD_M1_80 (1<<7)
#define EPLD_M1_40 (1<<8)
#define EPLD_M2 (1<<9)
#define EPLD_M1_80_SWITCH (1<<10)
#define EPLD_M1_40_SWITCH (1<<11)
#define EPLD_M2_SWITCH (1<<12)
#define EPLD_M1_80_SPARE (1<<13)
#define EPLD_M1_40_SPARE (1<<14)
#define EPLD_M2_SPARE (1<<15)

/* EPLD_INT_STATUS2 */
#define EPLD_USB_INT (1<<0)
#define EPLD_HW_INT_0 (1<<1)
#define EPLD_HW_INT_1 (1<<2)
#define EPLD_FAN_SWITCH (1<<3)
#define EPLD_PUSH_BUTTON_FRONT_BOTTOM (1<<4)
#define EPLD_PUSH_BUTTON_FRONT_TOP (1<<5)
#define EPLD_PUSH_BUTTON_BACK_BOTTOM (1<<6)
#define EPLD_PUSH_BUTTON_BACK_TOP (1<<7)

/* EPLD_INT_MASK1 (active high) */
/* same bit pattern as EPLD_INT_STATUS1 */

/* EPLD_INT_MASK2 (active high) */
/* same bit pattern as EPLD_INT_STATUS2 */

/* EPLD_MISC_CONTROL_0 */
#define EPLD_LCD_BACKLIGHT_BACK_ON (1 << 0)
#define EPLD_LCD_BACKLIGHT_FRONT_ON (1 << 1)
#define EPLD_M2_HOTSWAP_LED (1 << 3)
#define EPLD_M1_40_HOTSWAP_LED (1 << 4)
#define EPLD_M1_80_HOTSWAP_LED (1 << 5)
#define EPLD_MAC_DEV_EN_1 (1 << 6)
#define EPLD_MAC_DEV_EN_0 (1 << 7)
#define EPLD_RRSH_RESET (1 << 8)
#define EPLD_DTR (1 << 9)
#define EPLD_MAC_DEV_EN_2 (1 << 10)
#define EPLD_USB_POWER_EN (1 << 11)
#define EPLD_FAN_POWER (1 << 12)
#define EPLD_USB_POWER_CTRL (1 << 13)
#define EPLD_I2C_RST_L4 (1<<14)
#define EPLD_I2C_RST_L5 (1<<15)

/* EPLD_MISC_CONTROL */
#define EPLD_1_2V_NORMAL (0<<2)
#define EPLD_1_2V_NEG_5 (0x1<<2)
#define EPLD_1_2V_POS_5 (0x2<<2)
#define EPLD_3_3V_NORMAL (0<<4)
#define EPLD_3_3V_NEG_5 (0x1<<4)
#define EPLD_3_3V_POS_5 (0x2<<4)
#define EPLD_LCD_BACK_EN (1<<6)
#define EPLD_LCD_FRONT_EN (1<<7)
#define EPLD_LCD_RS_INSTRUCTION (0<<8)
#define EPLD_LCD_RS_DATA (1<<8)
#define EPLD_LCD_READ (1<<9)
#define EPLD_LCD_WRITE (0<<9)
#define MGMT_PORT_HALF_DUPLEX (0<<12)
#define MGMT_PORT_FULL_DUPLEX (1<<12)
#define MGMT_PORT_SPEED_1G (0x1<<13)
#define MGMT_PORT_SPEED_10M (0x0<<13)
#define MGMT_PORT_SPEED_100M (0x2<<13)
#define MGMT_PORT_LINK_UP (1<<15)
#define MGMT_PORT_NO_LINK (0<<15)

/* EPLD_LAST_RESET */
#define EPLD_GLOBAL_RESET (1<<0)
#define EPLD_SOFT_RESET (1<<1)
#define EPLD_HARD_RESET (1<<2)
#define EPLD_WDOG_GLOBAL_RESET (1<<3)
#define EPLD_WDOG_HARD_RESET (1<<4)
#define EPLD_WDOG_SOFT_RESET (1<<5)
#define EPLD_WDOG_NMI (1<<6)
#define EPLD_POWER_ON_RESET (1<<7)
#define EPLD_DCD (1<<8)
#define EPLD_MAC_INIT_DONE_0 (1<<9)
#define EPLD_MAC_INIT_DONE_1 (1<<10)
#define EPLD_MAC_INIT_DONE_2 (1<<11)
#define EPLD_PS1_INT_0 (1<<12)
#define EPLD_PS1_INT_1 (1<<13)
#define EPLD_FAN_PRESENT (1<<14)
#define EPLD_FAN_DIRECTION (1<<15)

/* EPLD_PUSH_BUTTON */
#define EPLD_BUTTON_0_DOWN (1<<0)
#define EPLD_BUTTON_1_DOWN (1<<1)
#define EPLD_BUTTON_2_DOWN (1<<2)
#define EPLD_BUTTON_3_DOWN (1<<3)
#define EPLD_BUTTON_4_DOWN (1<<4)
#define EPLD_BUTTON_5_DOWN (1<<5)
#define EPLD_BUTTON_6_DOWN (1<<6)
#define EPLD_BUTTON_7_DOWN (1<<7)
#define EPLD_REAR_LCD_POWER_OFF (1<<10)
#define EPLD_PCI_USB_POWER_OFF (1<<11)
#define EPLD_1_1V_NORMAL (0<<12)
#define EPLD_1_1V_NEG_5 (0x1<<12)
#define EPLD_1_1V_POS_5 (0x2<<12)
#define EPLD_2_5V_NORMAL (0<<14)
#define EPLD_2_5V_NEG_5 (0x1<<14)
#define EPLD_2_5V_POS_5 (0x2<<14)

/* Port LED */
#define EPLD_PORT_HALF_DUPLEX (0<<3)
#define EPLD_PORT_FULL_DUPLEX (1<<3)
#define EPLD_PORT_SPEED_10M (0x1<<0)
#define EPLD_PORT_SPEED_100M (0x2<<0)
#define EPLD_PORT_SPEED_1G (0x3<<0)
#define EPLD_PORT_SPEED_10G (0x4<<0)
#define EPLD_PORT_NO_LINK (0<<0)

/* EPLD_LED_MISC_CTRL */
#define EPLD_LED_M1_80_PMODE (1<<0)

extern int epld_reg_read(int reg, uint16_t *val);
extern int epld_reg_write(int reg, uint16_t val);

#endif /*__EPLD__*/
