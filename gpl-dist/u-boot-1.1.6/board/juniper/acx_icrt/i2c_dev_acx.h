/*
 * $Id$
 *
 * i2c_dev_acx.h  -- structure defenitions to traverse through i2c tree
 *
 *
 * Copyright (c) 2012-2014, Juniper Networks, Inc.
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
 *
 */
#include <linux/types.h>

/* I2C hardware details */
#define I2C_CONTROLLER_0                0
#define I2C_CONTROLLER_1                1

/* flags to indicate i2c device specifics */
#define MULTIPLEXER_NOT_PRESENT         0
#define MULTIPLEXER_PRESENT             1
#define EXPANDER_PRESENT                2
#define I2C_DEFAULT_TIMO_ACX       400
/* Maximum number of muxex between
 * I2C master & a slave.
 */
#define I2C_MULTIPLEXER_MAX_DEPTH       3
#define I2C_DEV_INVALID 0xFFFF


/*
 * All common devices across the platform will be aded in common
 * poll of device number. If you want to add any common device
 * Please add at the last and adjust "I2C_COMMON_END_DEV_ID"
 * accordingly. Please give the logical number incremented by 1.
 * Example
 *
 * #define I2C_NEW_COMMON_DEV_ID  I2C_ADD_IN_COMMON_DEV(last_number + 1)
 * #define I2C_COMMON_END_DEV_ID  I2C_NEW_COMMON_DEV_ID
 */
#define I2C_COMMON_BASE_DEV_ID          0
#define I2C_ADD_IN_COMMON_DEV(n)        (I2C_COMMON_BASE_DEV_ID + n)

#define EEPROM_MAINBOARD                I2C_ADD_IN_COMMON_DEV(0)
#define SECURITY_IC                     I2C_ADD_IN_COMMON_DEV(1)
#define RTC_8564                        I2C_ADD_IN_COMMON_DEV(2)
#define RTC_DS1672                      I2C_ADD_IN_COMMON_DEV(3)
#define HUB_USB2513                     I2C_ADD_IN_COMMON_DEV(4)
#define I2C_COMMON_END_DEV_ID           HUB_USB2513

#define I2C_ACX_DEV_ID(n)              (I2C_PLATFORM_BASE_DEV_ID + n)

#define ACX_RE_PCA9548_1               I2C_ACX_DEV_ID(0)
#define ACX_RE_MAX6581_1               I2C_ACX_DEV_ID(1)
#define ACX_RE_POWR1014                I2C_ACX_DEV_ID(2)
#define ACX_RE_AT24C02                 I2C_ACX_DEV_ID(3)

#define ACX_RE_PCA9546_1               I2C_ACX_DEV_ID(4)
#define ACX_RE_W83782G                 I2C_ACX_DEV_ID(5)
#define ACX_RE_PSU0                    I2C_ACX_DEV_ID(6)
#define ACX_RE_PSU0_MCU                I2C_ACX_DEV_ID(7)
#define ACX_RE_PSU0_DS1271             I2C_ACX_DEV_ID(8)
#define ACX_RE_PSU1                    I2C_ACX_DEV_ID(9)
#define ACX_RE_PSU1_MCU                I2C_ACX_DEV_ID(10)
#define ACX_RE_PSU1_DS1271             I2C_ACX_DEV_ID(11)
#define ACX_RE_FANTRAY                 I2C_ACX_DEV_ID(12)
/* Additions for ACX500 */
#define ACX_RE_MAX6581_2               I2C_ACX_DEV_ID(13)
#define ACX_PFE_BOARD_EEPROM           I2C_ACX_DEV_ID(14)

#define I2C_PLATFORM_BASE_DEV_ID        (I2C_COMMON_END_DEV_ID + 1)

#define I2C_DEV_LIST_COUNT              0xFFFF

/*
 * Physical addr and multiplexer details of I2C slaves:
 * logical_addr         - Address visible to application
 * physical_addr        - Physical address of I2C slave
 * has_multiplexer      - Is device connected thru multiplexer or expander
 * num_multiplexer      - Is the device sitting under hierarchy of muxes
 * multiplexer_addr[]   - Physical address of the multiplexer
 * channel_index[]      - Mux channel index or expander's IO bank index
 * i2c_controller_index - I2C controller device for connection
 * timo                 - Device specific timeout
 */
typedef struct i2c_device {
    uint32_t logical_addr;
    uint32_t physical_addr;
    int      has_multiplexer;
    uint8_t  num_multiplexer;
    uint8_t  multiplexer_addr[I2C_MULTIPLEXER_MAX_DEPTH];
    int      channel_index[I2C_MULTIPLEXER_MAX_DEPTH];
    int      i2c_controller_index;
    int      timo;
} i2c_device_t;

i2c_device_t i2c_acx_device_list[] = {
    {
	EEPROM_MAINBOARD,
	0x51,
	MULTIPLEXER_PRESENT,
	1,
	{0x70, 0, 0},
	{5, 0, 0},
	I2C_CONTROLLER_0,
	I2C_DEFAULT_TIMO_ACX
    },
    {
	ACX_RE_PCA9548_1,
	0x70,
	MULTIPLEXER_NOT_PRESENT,
	0,
	{0, 0, 0},
	{0, 0, 0},
	I2C_CONTROLLER_0,
	I2C_DEFAULT_TIMO_ACX
    },
    /* devices behind PCA9548 */
    {
	RTC_8564,
	0x51,
	MULTIPLEXER_PRESENT,
	1,
	{0x70, 0, 0},
	{0, 0, 0},
	I2C_CONTROLLER_0,
	I2C_DEFAULT_TIMO_ACX
    },

    {
	ACX_RE_POWR1014,
	0x34,
	MULTIPLEXER_PRESENT,
	1,
	{0x70, 0, 0},
	{1, 0, 0},
	I2C_CONTROLLER_0,
	I2C_DEFAULT_TIMO_ACX
    },

    {
	ACX_RE_MAX6581_1,
	0x4d,
	MULTIPLEXER_PRESENT,
	1,
	{0x70, 0, 0},
	{2, 0, 0},
	I2C_CONTROLLER_0,
	I2C_DEFAULT_TIMO_ACX
    },

    {
	HUB_USB2513,
	0x2c,
	MULTIPLEXER_PRESENT,
	1,
	{0x70, 0, 0},
	{3, 0, 0},
	I2C_CONTROLLER_0,
	I2C_DEFAULT_TIMO_ACX
    },

    {
	SECURITY_IC,
	0x72,
	MULTIPLEXER_PRESENT,
	1,
	{0x70, 0, 0},
	{4, 0, 0},
	I2C_CONTROLLER_0,
	I2C_DEFAULT_TIMO_ACX
    },

    {
	ACX_RE_AT24C02,
	0x51,
	MULTIPLEXER_PRESENT,
	1,
	{0x70, 0, 0},
	{5, 0, 0},
	I2C_CONTROLLER_0,
	I2C_DEFAULT_TIMO_ACX
    },

    /* Added for ACX500 */
    {
        ACX_RE_MAX6581_2,
        0x4d,
        MULTIPLEXER_PRESENT,
        1,
        {0x70, 0, 0},
        {7, 0, 0},
        I2C_CONTROLLER_0,
        I2C_DEFAULT_TIMO_ACX
    },

    {
        ACX_PFE_BOARD_EEPROM,
        0x50, 
        MULTIPLEXER_PRESENT,
        1,
        {0x70, 0, 0},
        {6, 0, 0},
        I2C_CONTROLLER_0,
        I2C_DEFAULT_TIMO_ACX
    },

    /* The following are for fortius-m */
    {
	ACX_RE_PCA9546_1,
	0x77,
	MULTIPLEXER_PRESENT,
	1,
	{0x70, 0, 0},
	{6, 0, 0},
	I2C_CONTROLLER_0,
	I2C_DEFAULT_TIMO_ACX
    },

    {
	ACX_RE_W83782G,
	0x29,
	MULTIPLEXER_PRESENT,
	1,
	{0x70, 0, 0},
	{7, 0, 0},
	I2C_CONTROLLER_0,
	I2C_DEFAULT_TIMO_ACX
    },

    /* devices beyond the PCA9546 switch on the RE on fortius-m */
    {
	ACX_RE_PSU0,
	0x50,
	MULTIPLEXER_PRESENT,
	2,
	{0x70, 0x77, 0},
	{6, 0, 0},
	I2C_CONTROLLER_0,
	I2C_DEFAULT_TIMO_ACX
    },

    {
	ACX_RE_PSU0_MCU,
	0x28,
	MULTIPLEXER_PRESENT,
	2,
	{0x70, 0x77, 0},
	{6, 0, 0},
	I2C_CONTROLLER_0,
	I2C_DEFAULT_TIMO_ACX
    },

    {
	ACX_RE_PSU0_DS1271,
	0x48,
	MULTIPLEXER_PRESENT,
	2,
	{0x70, 0x77, 0},
	{6, 0, 0},
	I2C_CONTROLLER_0,
	I2C_DEFAULT_TIMO_ACX
    },

    {
	ACX_RE_PSU1,
	0x50,
	MULTIPLEXER_PRESENT,
	2,
	{0x70, 0x77, 0},
	{6, 1, 0},
	I2C_CONTROLLER_0,
	I2C_DEFAULT_TIMO_ACX
    },

    {
	ACX_RE_PSU1_MCU,
	0x28,
	MULTIPLEXER_PRESENT,
	2,
	{0x70, 0x77, 0},
	{6, 1, 0},
	I2C_CONTROLLER_0,
	I2C_DEFAULT_TIMO_ACX
    },

    {
	ACX_RE_PSU1_DS1271,
	0x48,
	MULTIPLEXER_PRESENT,
	2,
	{0x70, 0x77, 0},
	{6, 1, 0},
	I2C_CONTROLLER_0,
	I2C_DEFAULT_TIMO_ACX
    },

    {
	ACX_RE_FANTRAY,
	0x53,
	MULTIPLEXER_PRESENT,
	2,
	{0x70, 0x77, 0},
	{6, 2, 0},
	I2C_CONTROLLER_0,
	I2C_DEFAULT_TIMO_ACX
    },
    {
	I2C_DEV_INVALID,
	0x0,
	MULTIPLEXER_NOT_PRESENT,
	0,
	{0x00, 0x00, 0},
	{0, 0, 0},
	I2C_CONTROLLER_0,
	I2C_DEFAULT_TIMO_ACX
    },
};

