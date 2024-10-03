/*
 * Copyright (c) 2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * (C) Copyright 2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>

#ifdef CONFIG_POST

/*
 * I2C test
 *
 * For verifying the I2C bus, a full I2C bus scanning is performed.
 *
 * #ifdef I2C_ADDR_LIST
 *   The test is considered as passed if all the devices and
 *   only the devices in the list are found.
 * #else [ ! I2C_ADDR_LIST ]
 *   The test is considered as passed if any I2C device is found.
 * I2C_ADDR_LIST - The i2c device list.
 * #endif
 */

#include <post.h>
#include "i2c.h"
#include <fsl_i2c.h>
#include <watchdog.h>
#include <ex82xx_devices.h>

#if CONFIG_POST & CFG_POST_I2C

DECLARE_GLOBAL_DATA_PTR;

void i2c_controller(int controller);

/* I2C device struct */
typedef struct i2c_probe_dev {
	unsigned char dev_addr;
	unsigned char switch_addr;
	int channel_index;
	int i2c_index;
	char name[50];
	struct i2c_probe_dev *sub_dev_list;
} i2c_addr;

/* Details of devices*/
i2c_addr i2c_dev_list_scb[] = {
	{
		/*
		 * Dev name: Real time clock DS1672
		 */
		SCB_RTC_I2C_ADDR,
		0x0,
		0,
		CFG_I2C_CTRL_1,
		"Real Time Clock",
	},
	{
		/*
		 * Dev name: ID Serial Eeprom AT24C02BN
		 */
		SCB_ID_EEPROM_I2C_ADDR,
		0x0,
		0,
		CFG_I2C_CTRL_1,
		"SMB IDEEPROM",
	},
	{
		/*
		 * Dev name: Voltage Monitor MAX1139
		 */
		SCB_VOLT_MON_I2C_ADDR,
		0x0,
		0,
		CFG_I2C_CTRL_1,
		"SCB Voltmeter",
	},
	{
		/*
		 * Dev name: IO Expander #1 PCF8574
		 */
		SCB_IO_EXP_1_I2C_ADDR,
		0x0,
		0,
		CFG_I2C_CTRL_1,
		"I2C IO Expander 1",
	},
	{
		/*
		 * Dev name: IO Expander #2 PCF8574
		 */
		SCB_IO_EXP_2_I2C_ADDR,
		0x0,
		0,
		CFG_I2C_CTRL_1,
		"I2C IO Expander 2",
	},
	{
		/*
		 * Dev name: DDR-II DIMM #1
		 */
		SCB_DDR_I2C_ADDR,
		0x0,
		0,
		CFG_I2C_CTRL_1,
		"DDR2 MiniDIMM SPD",
	}
};

i2c_addr i2c_dev_list_lcpu[] = {
	{
		/*
		 * Dev name: ID Serial Eeprom AT24C02BN
		 */
		LCPU_ID_EEPROM_I2C_ADDR,
		0x0,
		0,
		CFG_I2C_CTRL_1,
		"LCPU SMB ID EEPROM",
	},
	{
		/*
		 * Dev name: LC ID Serial Eeprom
		 */
		LC_ID_SLAVE_EEPROM_I2C_ADDR,
		0x0,
		0,
		CFG_I2C_CTRL_1,
		"LC ID EEPROM",
	},
};

i2c_addr *i2c_dev_list;
static int debug_flag = 0;

int i2c_post_test(int flags)
{
	unsigned int i2c_dev = 0;
	unsigned int pca9506_IO_0_addr = 0, pca9506_IO_1_addr = 0;
	unsigned int pca9506_IO_2_addr = 0, pca9506_IO_3_addr = 0;
	unsigned int pca9506_IO_4_addr = 0;
	unsigned int pass = 0;
	int ret = 0;
	unsigned int i2c_dev_list_size = 0;

	debug_flag = 0;
	post_log("\rI2C Test......Probing for few expected i2c devices.\n\n");
	if (flags & POST_DEBUG) {
		debug_flag = 1;
	}

	/* Assign the sub-system specific list to i2c_dev_list */
	if (EX82XX_RECPU) {
		i2c_dev_list = i2c_dev_list_scb;
		i2c_dev_list_size = sizeof(i2c_dev_list_scb) / sizeof(i2c_dev_list[0]);
	} else if (EX82XX_LCPU) {
		i2c_dev_list = i2c_dev_list_lcpu;
		i2c_dev_list_size = sizeof(i2c_dev_list_lcpu) / sizeof(i2c_dev_list[0]);
	} else {
		/* Error condition: Not supported ccpu_type */
		post_log("I2C: Unknown cpu type encountered: %x\n", gd->ccpu_type);
		return (-1);
	}

	for (i2c_dev = 0; i2c_dev < i2c_dev_list_size; i2c_dev++) {
		ret = i2c_probe_caffeine(i2c_dev_list[i2c_dev].i2c_index,
								 i2c_dev_list[i2c_dev].switch_addr,
								 i2c_dev_list[i2c_dev].channel_index,
								 i2c_dev_list[i2c_dev].dev_addr);

		if (ret == 0) {
			post_log("Device 0x%02X Found - %s [ i2c-bus = %02X "
					 "switch = 0x%02X "
					 "channel = %d ] \n",
					 i2c_dev_list[i2c_dev].dev_addr,
					 i2c_dev_list[i2c_dev].name,
					 i2c_dev_list[i2c_dev].i2c_index,
					 i2c_dev_list[i2c_dev].switch_addr,
					 i2c_dev_list[i2c_dev].channel_index);
			pass++;
		} else   {
			post_log("Device 0x%02X NOT Found - %s [ i2c-bus = %02X "
					 "switch = 0x%02X "
					 "channel = %d ]\n",
					 i2c_dev_list[i2c_dev].dev_addr,
					 i2c_dev_list[i2c_dev].name,
					 i2c_dev_list[i2c_dev].i2c_index,
					 i2c_dev_list[i2c_dev].switch_addr,
					 i2c_dev_list[i2c_dev].channel_index);
		}

		WATCHDOG_RESET();
#ifdef CONFIG_SHOW_ACTIVITY
		extern void show_activity(int arg);
		show_activity(1);
#endif
	}

	printf("\n");
	i2c_controller(CFG_I2C_CTRL_1);

	if (pass == i2c_dev_list_size) {
		return (0);
	} else   {
		return (-1);
	}
}


#endif /* CONFIG_POST & CFG_POST_I2C */
#endif /* CONFIG_POST */
