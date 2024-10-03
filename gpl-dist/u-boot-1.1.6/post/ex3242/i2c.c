/*
 * Copyright (c) 2007-2011, Juniper Networks, Inc.
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

/******** JAVA **********/

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
#include <configs/ex3242.h>

#if CONFIG_POST & CFG_POST_I2C

/*
 * Probe the given I2C chip address of java board.Returns 0 if a chip responded,
 * not 0 on failure.  */
extern int i2c_probe_java(uint i2c_ctrl, uchar sw_addr, uchar ch_index, uchar chip);

/* I2C device struct */
typedef struct i2c_probe_dev {
	unsigned char dev_addr;
	unsigned char switch_addr;
	int channel_index;
	int i2c_index;
	char name[50];
	struct i2c_probe_dev *sub_dev_list;
} i2c_addr;

/* i2c dev list  */
i2c_addr i2c_dev_detect[]={
	{
		/*
		   Dev name: PWR_Suppy1
		 */
		0x51,
		0x70,
		1,
		CFG_I2C_CTRL_1,
		"Pwr_Supply_1",
		NULL,
	},
	{
		/*
		   Dev name: PWR_Suppy2
		 */
		0x51,
		0x70,
		2,
		CFG_I2C_CTRL_1,
		"Pwr_Supply_2",
		NULL,

	},
	{
		/*
		   Dev name: External RPS
		 */
		0x51,
		0x70,
		0,
		CFG_I2C_CTRL_1,
		"External RPS",
		NULL,
	},
	{
		/* 
		   Dev name: Uplink I2C
		 */
		0x70,
		0x71,
		0,
		CFG_I2C_CTRL_2,
		"Up_Link_I2c",
		NULL,
	},
	{
		/* 
		   Dev name: POE Module EEPROM PCF8582C
		 */
		0x53,
		0x70,
		5,
		CFG_I2C_CTRL_1,
		"Poe_Module_Eeprom",
		NULL,
	}
};

/* i2c dev list 1 */
i2c_addr i2c_dev_list_sumatra[]={
#if 0
	{
		/* 
		   Dev name: Security IC 
		 */
		0x51,
		0x70,
		4,
		CFG_I2C_CTRL_1,
		"Security_Ic_Eeprom",
		NULL,
	},
#endif
	{
		/* 
		   Dev name: PD63000 POE Controller
		 */
		0x2c,
		0x70,
		5,
		CFG_I2C_CTRL_1,
		"Poe_Ctrl",
		NULL,
	},

	{
		/* 
		   Dev name: HW Monitor W83782G
		 */
		0x29,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Hw_Monitor",
		NULL,
	},

	{
		/* 
		   Dev name: NE1617A Temp Sensor #1
		 */
		0x18,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Temp_Sensor_1",
		NULL,
	},

	{
		/* 
		   Dev name: NE1617A Temp Sensor #2
		 */
		0x19,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Temp_Sensor_2",
		NULL,
	},

	{
		/* 
		   Dev name: MK1493-03B PCI Clock Generator
		   if this device is not to be probed; remove it from here
		 */
		0x69,
		0x70,
		7,
		CFG_I2C_CTRL_1,
		"Pci_Clock",
		NULL,
	},
	{
		/* 
		   Dev name: I/O expander
		 */
		0,  /* no device address */
		0x22,
		0,  /* no channel index */
		CFG_I2C_CTRL_2,
		"I/O-Expander-Dev",
		NULL,
	},	
	{
		/* 
		   Dev name:  Null Device
		 */
		0,
		0,
		0,
		0,
		"End_Of_I2c",
		NULL,
	},
};
/* i2c dev list 2 */
i2c_addr i2c_dev_list_expresso24[]={
#if 0
	{
		/* 
		   Dev name: secure cookie.
		 */
		0x51,
		0x70,
		4,
		CFG_I2C_CTRL_1,
		"Secure_cookie_Eeprom",
		NULL,
	},
#endif
	{
		/* 
		   Dev name: PD63000 POE Controller
		 */
		0x2c,
		0x70,
		5,
		CFG_I2C_CTRL_1,
		"Poe_Ctrl",
		NULL,
	},
	{
		/* 
		   Dev name: HW Monitor W83782G
		 */
		0x29,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Hw_Monitor",
		NULL,
	},
	{
		/* 
		   Dev name: NE1617A Temp Sensor #1
		 */
		0x19,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Temp_Sensor_1",
		NULL,
	},
	{
		/* 
		   Dev name: NE1617A Temp Sensor #2
		 */
		0x1a,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Temp_Sensor_2",
		NULL,
	},
	{
		/* 
		   Dev name: NE1617A Temp Sensor #3
		 */
		0x2b,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Temp_Sensor_3",
		NULL,
	},
#if 0
	{
		/* 
		 * IDT5V9885 on port 7 of Mux PCA9548 clock generator.
		 */
		0x6a,
		0x70,
		7,
		CFG_I2C_CTRL_1,
		"Clock generator",
		NULL,
	},
#endif	
	{
		/* 
		 * ICS9DB801 on port 7 of Mux PCA9548 pcie clock generator. 
		 */
		0x6e,
		0x70,
		7,
		CFG_I2C_CTRL_1,
		"Pcie Clock generator",
		NULL,
	},
	{
		/* 
		 *VT1165M on port 7 of Mux PCA9548 1.0v PS 
		 */
		0x71,
		0x70,
		7,
		CFG_I2C_CTRL_1,
		"1.0V PS",
		NULL,
	},
	{
		/* 
		   Dev name: I/O expander
		 */
		0,  /* no device address */
		0x22,
		0,  /* no channel index */
		CFG_I2C_CTRL_2,
		"I/O-Expander-Dev",
		NULL,
	},	
	{
		/* 
		   Dev name:  Null Device
		 */
		0,
		0,
		0,
		0,
		"End_Of_I2c",
		NULL,
	},
};

/* i2c dev list  3 */
i2c_addr i2c_dev_list_expresso48[]={
#if 0
	{
		/* 
		   Dev name:Secure cookie
		 */
		0x51,
		0x70,
		4,
		CFG_I2C_CTRL_1,
		"Secure_cookie_Eeprom",
		NULL,
	},
#endif
	{
		/* 
		   Dev name: PD63000 POE Controller
		 */
		0x2c,
		0x70,
		5,
		CFG_I2C_CTRL_1,
		"Poe_Ctrl",
		NULL,
	},
	{
		/* 
		   Dev name: HW Monitor W83782G
		 */
		0x29,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Hw_Monitor",
		NULL,
	},
	{
		/* 
		   Dev name: NE1617A Temp Sensor #1
		 */
		0x19,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Temp_Sensor_1",
		NULL,
	},
	{
		/* 
		   Dev name: NE1617A Temp Sensor #2
		 */
		0x1a,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Temp_Sensor_2",
		NULL,
	},
	{
		/* 
		   Dev name: NE1617A Temp Sensor #3
		 */
		0x2b,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Temp_Sensor_3",
		NULL,
	},
#if 0
	{
		/* 
		 * IDT5V9885 on port 7 of Mux PCA9548 clock generator.
		 */
		0x6a,
		0x70,
		7,
		CFG_I2C_CTRL_1,
		"Clock generator",
		NULL,
	},
#endif	
	{
		/* 
		 * ICS9DB801 on port 7 of Mux PCA9548 pcie clock generator. 
		 */
		0x6e,
		0x70,
		7,
		CFG_I2C_CTRL_1,
		"Pcie Clock generator",
		NULL,
	},
	{
		/* 
		 *VT1165M on port 7 of Mux PCA9548 1.0v PS 
		 */
		0x71,
		0x70,
		7,
		CFG_I2C_CTRL_1,
		"1.0V PS",
		NULL,
	},
	{
		/* 
		   Dev name: I/O expander
		 */
		0,  /* no device address */
		0x22,
		0,  /* no channel index */
		CFG_I2C_CTRL_2,
		"I/O-Expander-Dev",
		NULL,
	},	
	{
		/* 
		   Dev name:  Null Device
		 */
		0,
		0,
		0,
		0,
		"End_Of_I2c",
		NULL,
	},
};
/* I2c_dev list 4a sfp list */
i2c_addr i2c_dev_list_sfp[]={
	{
		0x50,
		0x74,
		0,
		2,
		"sfp_0",
		&i2c_dev_list_sfp[1],
	},
	{
		0x50,
		0x74,
		1,
		2,
		"sfp_1",
		&i2c_dev_list_sfp[2],
	},
	{
		0x50,
		0x74,
		2,
		2,
		"sfp_2",
		&i2c_dev_list_sfp[3],
	},
	{
		0x50,
		0x74,
		3,
		2,
		"sfp_3",
		&i2c_dev_list_sfp[4],
	},
	{
		0x50,
		0x74,
		4,
		2,
		"sfp_4",
		&i2c_dev_list_sfp[5],
	},
	{
		0x50,
		0x74,
		5,
		2,
		"sfp_5",
		&i2c_dev_list_sfp[6],
	},
	{
		0x50,
		0x74,
		6,
		2,
		"sfp_6",
		&i2c_dev_list_sfp[7],
	},
	{
		0x50,
		0x74,
		7,
		2,
		"sfp_7",
		&i2c_dev_list_sfp[8],
	},
	{
		/* 
		   Dev name:  Null Device
		 */
		0,
		0,
		0,
		0,
		"End_Of_I2c",
		NULL,
	},
};
/* i2c dev list 4 */
i2c_addr i2c_dev_list_latte24_fibre[]={
#if 0
	{
		/* 
		   Dev name:Secure cookie 
		 */
		0x51,
		0x70,
		4,
		CFG_I2C_CTRL_1,
		"Secure_cookie_Eeprom",
		NULL,
	},
#endif
	{
		/* 
		   Dev name: HW Monitor W83782G
		 */
		0x29,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Hw_Monitor",
		NULL,
	},
	{
		/* 
		   Dev name: NE1617A Temp Sensor #1
		 */
		0x19,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Temp_Sensor_1",
		NULL,
	},
	{
		/* 
		   Dev name: NE1617A Temp Sensor #2
		 */
		0x1a,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Temp_Sensor_2",
		NULL,
	},
	{
		/* 
		   Dev name: NE1617A Temp Sensor #3
		 */
		0x2b,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Temp_Sensor_3",
		NULL,
	},
#if 0
	{
		/* 
		 * IDT5V9885 on port 7 of Mux PCA9548 clock generator.
		 */
		0x6a,
		0x70,
		7,
		CFG_I2C_CTRL_1,
		"Clock generator",
		NULL,
	},
#endif	
	{
		/* 
		 * ICS9DB801 on port 7 of Mux PCA9548 pcie clock generator. 
		 */
		0x6e,
		0x70,
		7,
		CFG_I2C_CTRL_1,
		"Pcie Clock generator",
		NULL,
	},
	{
		/* 
		 *VT1165M on port 7 of Mux PCA9548 1.0v PS 
		 */
		0x71,
		0x70,
		7,
		CFG_I2C_CTRL_1,
		"1.0V PS",
		NULL,
	},
	{
		/* 
		   Dev name: I/O expander
		 */
		0,  /* no device address */
		0x22,
		0,  /* no channel index */
		CFG_I2C_CTRL_2,
		"I/O-Expander-Dev",
		NULL,
	},	
	{
		/* 
		   Dev name:  Null Device
		 */
		0,
		0,
		0,
		0,
		"End_Of_I2c",
		NULL,
	},
};
/* i2c dev list 5 */
i2c_addr i2c_dev_list_latte24[]={
#if 0
	{
		/* 
		   Dev name: secure cookie.
		 */
		0x51,
		0x70,
		4,
		CFG_I2C_CTRL_1,
		"Secure_cookie_Eeprom",
		NULL,
	},
#endif
	{
		/* 
		   Dev name: PD63000 POE Controller
		 */
		0x2c,
		0x70,
		5,
		CFG_I2C_CTRL_1,
		"Poe_Ctrl",
		NULL,
	},
	{
		/* 
		   Dev name: HW Monitor W83782G
		 */
		0x29,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Hw_Monitor",
		NULL,
	},
	{
		/* 
		   Dev name: NE1617A Temp Sensor #1
		 */
		0x19,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Temp_Sensor_1",
		NULL,
	},
	{
		/* 
		   Dev name: NE1617A Temp Sensor #2
		 */
		0x1a,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Temp_Sensor_2",
		NULL,
	},
	{
		/* 
		   Dev name: NE1617A Temp Sensor #3
		 */
		0x2b,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Temp_Sensor_3",
		NULL,
	},
#if 0
	{
		/* 
		 * IDT5V9885 on port 7 of Mux PCA9548 clock generator.
		 */
		0x6a,
		0x70,
		7,
		CFG_I2C_CTRL_1,
		"Clock generator",
		NULL,
	},
#endif
	{
		/* 
		 * ICS9DB801 on port 7 of Mux PCA9548 pcie clock generator. 
		 */
		0x6e,
		0x70,
		7,
		CFG_I2C_CTRL_1,
		"Pcie Clock generator",
		NULL,
	},
	{
		/* 
		 *VT1165M on port 7 of Mux PCA9548 1.0v PS 
		 */
		0x71,
		0x70,
		7,
		CFG_I2C_CTRL_1,
		"1.0V PS",
		NULL,
	},
	{
		/* 
		   Dev name: I/O expander
		 */
		0,  /* no device address */
		0x22,
		0,  /* no channel index */
		CFG_I2C_CTRL_2,
		"I/O-Expander-Dev",
		NULL,
	},	
	{
		/* 
		   Dev name:  Null Device
		 */
		0,
		0,
		0,
		0,
		"End_Of_I2c",
		NULL,
	},
};
/* i2c dev list  6 */
i2c_addr i2c_dev_list_latte48[]={
#if 0
	{
		/* 
		   Dev name: secure cookie.
		 */
		0x51,
		0x70,
		4,
		CFG_I2C_CTRL_1,
		"Secure_cookie_Eeprom",
		NULL,
	},
#endif
	{
		/* 
		   Dev name: PD63000 POE Controller
		 */
		0x2c,
		0x70,
		5,
		CFG_I2C_CTRL_1,
		"Poe_Ctrl",
		NULL,
	},
	{
		/* 
		   Dev name: HW Monitor W83782G
		 */
		0x29,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Hw_Monitor",
		NULL,
	},
	{
		/* 
		   Dev name: NE1617A Temp Sensor #1
		 */
		0x19,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Temp_Sensor_1",
		NULL,
	},
	{
		/* 
		   Dev name: NE1617A Temp Sensor #2
		 */
		0x1a,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Temp_Sensor_2",
		NULL,
	},
	{
		/* 
		   Dev name: NE1617A Temp Sensor #3
		 */
		0x2b,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Temp_Sensor_3",
		NULL,
	},
	{
		/* 
		   Dev name: NE1617A Temp Sensor #4
		 */
		0x18,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Temp_Sensor_4",
		NULL,
	},
#if 0	
	{
		/* 
		 * IDT5V9885 on port 7 of Mux PCA9548 clock generator.
		 */
		0x6a,
		0x70,
		7,
		CFG_I2C_CTRL_1,
		"Clock generator",
		NULL,
	},
#endif	
	{
		/* 
		 * ICS9DB801 on port 7 of Mux PCA9548 pcie clock generator. 
		 */
		0x6e,
		0x70,
		7,
		CFG_I2C_CTRL_1,
		"Pcie Clock generator",
		NULL,
	},
	{
		/* 
		 *VT1165M on port 7 of Mux PCA9548 1.0v PS 
		 */
		0x71,
		0x70,
		7,
		CFG_I2C_CTRL_1,
		"1.0V PS",
		NULL,
	},
	{
		/* 
		   Dev name: I/O expander
		 */
		0,  /* no device address */
		0x22,
		0,  /* no channel index */
		CFG_I2C_CTRL_2,
		"I/O-Expander-Dev",
		NULL,
	},	
	{
		/* 
		   Dev name:  Null Device
		 */
		0,
		0,
		0,
		0,
		"End_Of_I2c",
		NULL,
	},
};

/* i2c dev list 7 */
i2c_addr i2c_dev_list_amaretto24[]={
	{
		/* 
		   Dev name: PD69000 POE Controller
		 */
		0x2c,
		0x70,
		5,
		CFG_I2C_CTRL_1,
		"Poe_Ctrl",
		NULL,
	},
	{
		/* 
		   Dev name: HW Monitor W83782G
		 */
		0x29,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Hw_Monitor",
		NULL,
	},
	{
		/* 
		   Dev name: NE1617A Temp Sensor #1
		 */
		0x19,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Temp_Sensor_1",
		NULL,
	},
	{
		/* 
		   Dev name: NE1617A Temp Sensor #2
		 */
		0x1a,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Temp_Sensor_2",
		NULL,
	},
	{
		/* 
		   Dev name: NE1617A Temp Sensor #3
		 */
		0x2b,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Temp_Sensor_3",
		NULL,
	},
	{
		/* 
		 * ICS9DB801 on port 7 of Mux PCA9548 pcie clock generator. 
		 */
		0x6e,
		0x70,
		7,
		CFG_I2C_CTRL_1,
		"Pcie Clock generator",
		NULL,
	},
	{
		/* 
		 *VT1165M on port 7 of Mux PCA9548 1.0v PS 
		 */
		0x71,
		0x70,
		7,
		CFG_I2C_CTRL_1,
		"1.0V PS",
		NULL,
	},
	{
		/* 
		   Dev name: I/O expander
		 */
		0,  /* no device address */
		0x22,
		0,  /* no channel index */
		CFG_I2C_CTRL_2,
		"I/O-Expander-Dev",
		NULL,
	},	
	{
		/* 
		   Dev name:  Null Device
		 */
		0,
		0,
		0,
		0,
		"End_Of_I2c",
		NULL,
	},
};

/* i2c dev list  8 */
i2c_addr i2c_dev_list_amaretto48[]={
	{
		/* 
		   Dev name: PD69000 POE Controller
		 */
		0x2c,
		0x70,
		5,
		CFG_I2C_CTRL_1,
		"Poe_Ctrl",
		NULL,
	},
	{
		/* 
		   Dev name: HW Monitor W83782G
		 */
		0x29,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Hw_Monitor",
		NULL,
	},
	{
		/* 
		   Dev name: NE1617A Temp Sensor #1
		 */
		0x19,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Temp_Sensor_1",
		NULL,
	},
	{
		/* 
		   Dev name: NE1617A Temp Sensor #2
		 */
		0x1a,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Temp_Sensor_2",
		NULL,
	},
	{
		/* 
		   Dev name: NE1617A Temp Sensor #3
		 */
		0x2b,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Temp_Sensor_3",
		NULL,
	},
	{
		/* 
		   Dev name: NE1617A Temp Sensor #4
		 */
		0x18,
		0x70,
		6,
		CFG_I2C_CTRL_1,
		"Temp_Sensor_4",
		NULL,
	},
	{
		/* 
		 * ICS9DB801 on port 7 of Mux PCA9548 pcie clock generator. 
		 */
		0x6e,
		0x70,
		7,
		CFG_I2C_CTRL_1,
		"Pcie Clock generator",
		NULL,
	},
	{
		/* 
		 *VT1165M on port 7 of Mux PCA9548 1.0v PS 
		 */
		0x71,
		0x70,
		7,
		CFG_I2C_CTRL_1,
		"1.0V PS",
		NULL,
	},
	{
		/* 
		   Dev name: I/O expander
		 */
		0,  /* no device address */
		0x22,
		0,  /* no channel index */
		CFG_I2C_CTRL_2,
		"I/O-Expander-Dev",
		NULL,
	},	
	{
		/* 
		   Dev name:  Null Device
		 */
		0,
		0,
		0,
		0,
		"End_Of_I2c",
		NULL,
	},
};

i2c_addr i2c_dev_list_uplink[]={
	{
		/* 
		   Dev name: UPlink moudle eeprom 
		 */
		0x57,
		0x71,
		0,
		CFG_I2C_CTRL_2,
		"Uplink Eeprom",
		NULL,
	},
};


#define MAX_JAVA_BOARD 8
#define CH_INDEX_2      2
#define CH_INDEX_3      3
#define UPLINK_I2C_LOC  3

extern uint bd_id;
i2c_addr *i2c_java_board_list[MAX_JAVA_BOARD];
static	int i2c_flag = 0;
void i2c_controller(int controller);
unsigned int i2c_dev_detect_list_size = sizeof (i2c_dev_detect) / 
sizeof (i2c_dev_detect[0]);
static int count ;
extern int board_id(void);
int i2c_detect_list[20]={0};
int i2c_dev_count_list[50] ={0};

int i2c_post_test (int flags)
{
	unsigned int i2c_dev = 0; 
	unsigned int pca9506_IO_0_addr = 0,pca9506_IO_1_addr = 0, pca9506_IO_2_addr = 0,
				 pca9506_IO_3_addr = 0, pca9506_IO_4_addr = 0; 
	unsigned int pca9506_IO_0_addr_p2 = 0,pca9506_IO_1_addr_p2 = 0,
				 pca9506_IO_2_addr_p2 = 0, pca9506_IO_3_addr_p2 = 0,
				 pca9506_IO_4_addr_p2 = 0; 
	unsigned int pca9506_IO_0_addr_p3 = 0,pca9506_IO_1_addr_p3= 0,
				 pca9506_IO_2_addr_p3 = 0, pca9506_IO_3_addr_p3 = 0,
				 pca9506_IO_4_addr_p3 = 0; 
	unsigned int pass = 0;
	volatile unsigned int dev_res = 0;
	unsigned char ch = 0;
	int ret = 0;
	int board_id_java = 0;
	int i = 0;
	int fail = 0;
	uint java_bd_id = 0;
	count = 0;
	i2c_flag = 0;
    i2c_addr *i2c_board_list;
	int uplink_flag = 0;
	
	i2c_java_board_list[0] = i2c_dev_list_sumatra;
	i2c_java_board_list[1] = i2c_dev_list_expresso24;
    i2c_java_board_list[2] = i2c_dev_list_expresso48;	
	i2c_java_board_list[3] = i2c_dev_list_latte24_fibre;
    i2c_java_board_list[4] = i2c_dev_list_latte24;	
    i2c_java_board_list[5] = i2c_dev_list_latte48;	
	i2c_java_board_list[6] = i2c_dev_list_amaretto24;	
	i2c_java_board_list[7] = i2c_dev_list_amaretto48;	

	if (flags & POST_DEBUG) {
		i2c_flag =1;
	}
	if (bd_id == 0) {
		java_bd_id = board_id();
	}
	else {
		java_bd_id = bd_id;
	}
    
    switch (java_bd_id) {
	  case I2C_ID_JUNIPER_EX3200_24_T:
          board_id_java = 1;
		  break;
	  case I2C_ID_JUNIPER_EX3200_24_P:
          board_id_java = 1;
		  break;
	  case I2C_ID_JUNIPER_EX3200_48_T:
          board_id_java = 2;
		  break;
	  case I2C_ID_JUNIPER_EX3200_48_P:
          board_id_java = 2;
		  break;
	  case I2C_ID_JUNIPER_EX4200_24_F:
          board_id_java = 3;
		  break;
	  case I2C_ID_JUNIPER_EX4200_24_T:
          board_id_java = 4;
		  break;
	  case I2C_ID_JUNIPER_EX4200_24_P:
          board_id_java = 4;
		  break;
	  case I2C_ID_JUNIPER_EX4200_48_T:
          board_id_java = 5;
		  break;
	  case I2C_ID_JUNIPER_EX4200_48_P:
		  board_id_java = 5;
		  break;
	  case I2C_ID_JUNIPER_EX4210_24_P:
		  board_id_java = 6;
		  break;
	  case I2C_ID_JUNIPER_EX4210_48_P:
		  board_id_java = 7;
		  break;
	  default:
		  board_id_java = 0;
		  break;
	};

	i2c_board_list = i2c_java_board_list[board_id_java];

	/* Reading the pin status to know the i2cdevice present or not */
	volatile immap_t *immap = (immap_t *) CFG_CCSRBAR;
	volatile ccsr_gur_t *gur = &immap->im_gur;
	dev_res = gur->gpindr >> 24;

       i2c_detect_list[0] = 0;
	if (!(dev_res & 0x80)) {
		i2c_detect_list[0]=1; /* PS1 detected */
	}	   
       i2c_detect_list[1] = 0;
	if (!(dev_res & 0x40)) {
		i2c_detect_list[1]=1; /* PS2 detected */
	}	   
       i2c_detect_list[2] = 0;
	if (!(dev_res & 0x20)) {
		i2c_detect_list[2]=1; /* RPS detected */
	}	   
       i2c_detect_list[3] = 0;
	if (!(dev_res & 0x10)) {
		i2c_detect_list[3]=1; /* Uplink module detected */
	}	   
       i2c_detect_list[4] = 0;
	if (!(dev_res & 0x08)) {
		i2c_detect_list[4]=1; /* POE module detected */
	}	   

	/* probes the i2c devices based on the detection through gpindr. */
	for (i2c_dev = 0; i2c_dev<i2c_dev_detect_list_size; i2c_dev++) {
		if (i2c_detect_list[i2c_dev] == 1) {
			ret = i2c_probe_java(i2c_dev_detect[i2c_dev].i2c_index,
								 i2c_dev_detect[i2c_dev].switch_addr,
								 i2c_dev_detect[i2c_dev].channel_index,
								 i2c_dev_detect[i2c_dev].dev_addr);
			if ( ret == 0) {			
				i2c_dev_count_list[count] = 1;
			}
			else {
				i2c_dev_count_list[count] = -1;
				fail = -1;
			}
		} 
		count++;
	}
	/*
	 * If uplink moudle is detected check for the uplink eeprom otherwise do not
	 * check for the uplink eeprom.
	 */
	if (i2c_detect_list[UPLINK_I2C_LOC] == 1) {
		ret = i2c_probe_java(i2c_dev_list_uplink[0].i2c_index,
							 i2c_dev_list_uplink[0].switch_addr,
							 i2c_dev_list_uplink[0].channel_index,
							 i2c_dev_list_uplink[0].dev_addr);
		if ( ret == 0) {			
			uplink_flag  = 1;
		}
		else {
			uplink_flag  = 2;
		}
	}


	/* Expected i2c devices on the board  */
	for (i2c_dev = 0; strcmp((i2c_board_list)[i2c_dev].name, "End_Of_I2c"); i2c_dev++) {
		if (((i2c_board_list))[i2c_dev].i2c_index == CFG_I2C_CTRL_2) {
			pca9506_IO_0_addr = i2c_reg_read(0x22, 0x0); /* IO0 */
			pca9506_IO_1_addr = i2c_reg_read(0x22, 0x1); /* IO1 */
			pca9506_IO_2_addr = i2c_reg_read(0x22, 0x2); /* IO2 */
			pca9506_IO_3_addr = i2c_reg_read(0x22, 0x3); /* IO3 */
			pca9506_IO_4_addr = i2c_reg_read(0x22, 0x4); /* IO4 */
			if (java_bd_id != I2C_ID_JUNIPER_EX4200_24_F) {
				i2c_dev_count_list[count]=1;
				pass =1;
				break;
			}
			else {
				i2c_controller(i2c_board_list[i2c_dev].i2c_index);
				if (((i2c_board_list))[i2c_dev].channel_index == 1) {	
					ch = 1 <<  i2c_board_list[i2c_dev].channel_index;
					/* enable the corresponding channel on the swtich */
					ret = i2c_write(i2c_board_list[i2c_dev].switch_addr, 0, 0, &ch, 1);
					if (ret == 0) {
						i2c_addr *temp = (i2c_board_list)[i2c_dev].sub_dev_list;
						for (; temp &&  (strcmp(temp->name,"End_Of_I2c")); temp = temp->sub_dev_list) {
							ret = i2c_probe_java(temp->i2c_index,
												 temp->switch_addr,
												 temp->channel_index,
												 temp->dev_addr);

							if (ret == 0) {
								i2c_dev_count_list[count] = 1;
								count++;
								pass =1;
							}
							else {
								i2c_dev_count_list[count] = -1;
								count++;
								fail = -1;
							}

							/* disable the channel on the switch */
							ch = 0x0;
							i2c_write(i2c_board_list[i2c_dev].switch_addr, 0, 0, &ch, 1);
						}
					}
					else {
						i2c_dev_count_list[count] = -2;
						count++;
						fail = -1;
					}
				}
				if (((i2c_board_list))[i2c_dev].channel_index == CH_INDEX_2) {	
					ch = 1 <<  i2c_board_list[i2c_dev].channel_index;
					/* enable the corresponding channel on the swtich */
					ret = i2c_write(i2c_board_list[i2c_dev].switch_addr, 0, 0, &ch, 1);
					if (ret == 0) {
						pca9506_IO_0_addr_p2 = i2c_reg_read(0x22, 0x0); /* IO0 */
						pca9506_IO_1_addr_p2 = i2c_reg_read(0x22, 0x1); /* IO1 */
						pca9506_IO_2_addr_p2 = i2c_reg_read(0x22, 0x2); /* IO2 */
						pca9506_IO_3_addr_p2 = i2c_reg_read(0x22, 0x3); /* IO3 */
						pca9506_IO_4_addr_p2 = i2c_reg_read(0x22, 0x4); /* IO4 */
						i2c_addr *temp = (i2c_board_list)[i2c_dev].sub_dev_list;
						for (; temp &&  (strcmp(temp->name,"End_Of_I2c")); temp = temp->sub_dev_list) {
							ret = i2c_probe_java(temp->i2c_index,
												 temp->switch_addr,
												 temp->channel_index,
												 temp->dev_addr);

							if (ret == 0) {
								i2c_dev_count_list[count] = 1;
								count++;
								pass =1;
							}
							else {
								i2c_dev_count_list[count] = -1;
								count++;
								fail = -1;
							}
							/* disable the channel on the switch */
							ch = 0x0;
							i2c_write(((i2c_board_list))[i2c_dev].switch_addr, 0, 0, &ch, 1);
						}
					}
					else {
						i2c_dev_count_list[count]=-2;
						count++;
						fail = -1;
					}
				}
				if (((i2c_board_list))[i2c_dev].channel_index == CH_INDEX_3){	
					ch = 1 <<  i2c_board_list[i2c_dev].channel_index;
					/* enable the corresponding channel on the swtich */
					ret = i2c_write(i2c_board_list[i2c_dev].switch_addr, 0, 0, &ch, 1);
					if (ret == 0) {
						pca9506_IO_0_addr_p3 = i2c_reg_read(0x22, 0x0); /* IO0 */
						pca9506_IO_1_addr_p3 = i2c_reg_read(0x22, 0x1); /* IO1 */
						pca9506_IO_2_addr_p3 = i2c_reg_read(0x22, 0x2); /* IO2 */
						pca9506_IO_3_addr_p3 = i2c_reg_read(0x22, 0x3); /* IO3 */
						pca9506_IO_4_addr_p3 = i2c_reg_read(0x22, 0x4); /* IO4 */
						i2c_addr *temp = (i2c_board_list)[i2c_dev].sub_dev_list;
						for (; temp &&  (strcmp(temp->name,"End_Of_I2c")); temp = temp->sub_dev_list) {
							ret = i2c_probe_java(temp->i2c_index,
												 temp->switch_addr,
												 temp->channel_index,
												 temp->dev_addr);

							if (ret == 0) {
								i2c_dev_count_list[count] = 1;
								count++;
								pass =1;
							}
							else {
								i2c_dev_count_list[count] = -1;
								count++;
								fail = -1;
							}
							/* disable the channel on the switch */
							ch = 0x0;
							i2c_write(i2c_board_list[i2c_dev].switch_addr, 0, 0, &ch, 1);
						}
					}
					else {
						i2c_dev_count_list[count]=-2;
						count++;
						fail = -1;
					}
				}
			}
		}
		else {
			ret = i2c_probe_java(i2c_board_list[i2c_dev].i2c_index,
								 i2c_board_list[i2c_dev].switch_addr,
								 i2c_board_list[i2c_dev].channel_index,
								 i2c_board_list[i2c_dev].dev_addr);

			if (ret == 0) {			
				i2c_dev_count_list[count]=1;
				count++;
				pass =1;
			}
			else {
				i2c_dev_count_list[count]= -1;
				count++;
				fail = -1;
			}
		}
	}

	i2c_controller(CFG_I2C_CTRL_1);

	/* i2c probe result display */
	if (fail == -1  || pass == 0) {
		post_log("-- I2C test                             FAILED @\n");
		for (i = 0; i < i2c_dev_detect_list_size; i++) {
			if (i2c_dev_count_list[i] == -1) {
				post_log (" > I2C: Dev Not Found %-22s C%1d"
                                         " M%02xP%1d"
                                         " %02x @\n",
						  i2c_dev_detect[i].name,
						  i2c_dev_detect[i].i2c_index,
						  i2c_dev_detect[i].switch_addr,
						  i2c_dev_detect[i].channel_index,
						  i2c_dev_detect[i].dev_addr);
			}
			else {
				if (i2c_flag) {
					if (i2c_dev_count_list[i] == 1) {
						post_log (" > I2C: Dev Found"
                                                         "     %-22s C%1d"
                                                         " M%02xP%1d"
                                                         " %02x\n",
								  i2c_dev_detect[i].name,
								  i2c_dev_detect[i].i2c_index,
								  i2c_dev_detect[i].switch_addr,
								  i2c_dev_detect[i].channel_index,
								  i2c_dev_detect[i].dev_addr);
					}
				}
			}
		}
		if (uplink_flag){
			if (uplink_flag == 1) {
				if (i2c_flag) {
					post_log (" > I2C: Dev Found"
                                                 "     %-22s C%1d M%02xP%1d"
                                                 " %02x\n",
							  i2c_dev_list_uplink[0].name,
							  i2c_dev_list_uplink[0].i2c_index,
							  i2c_dev_list_uplink[0].switch_addr,
							  i2c_dev_list_uplink[0].channel_index,
							  i2c_dev_list_uplink[0].dev_addr);
				}
			}
			else {
				post_log (" > I2C: Dev Not Found %-22s C%1d"
                                         " M%02xP%1d"
                                         " %02x @\n",
							  i2c_dev_list_uplink[0].name,
							  i2c_dev_list_uplink[0].i2c_index,
							  i2c_dev_list_uplink[0].switch_addr,
							  i2c_dev_list_uplink[0].channel_index,
							  i2c_dev_list_uplink[0].dev_addr);
			}
		}
		count =5;	
		for (i = 0; strcmp(i2c_board_list[i].name, "End_Of_I2c"); i++) {
			if (i2c_dev_count_list[count] == -1) {
				post_log (" > I2C: Dev Not Found %-22s C%1d"
                                         " M%02xP%1d"
                                         " %02x @\n",
						  i2c_board_list[i].name,
						  i2c_board_list[i].i2c_index,
						  i2c_board_list[i].switch_addr,
						  i2c_board_list[i].channel_index,
						  i2c_board_list[i].dev_addr);
			}
			else {
				if (i2c_flag){	
					if (i2c_dev_count_list[count] == 1) {
						post_log (" > I2C: Dev Found"
                                                         "     %-22s  C%1d"
                                                         "  M%02xP%1d"
                                                         "  %02x\n",
								  i2c_board_list[i].name,
								  i2c_board_list[i].i2c_index,
								  i2c_board_list[i].switch_addr,
								  i2c_board_list[i].channel_index,
								  i2c_board_list[i].dev_addr);
					}
				}
			}
			count++;
		}

		if ( pass == 0 || i2c_flag == 1) {
			post_log (" > IO_0   : read  %02X status \n",
					  pca9506_IO_0_addr);
			post_log (" > IO_1   : read  %02X status \n",
					  pca9506_IO_1_addr);
			post_log (" > IO_2   : read  %02X status \n",
					  pca9506_IO_2_addr);
			post_log (" > IO_3   : read  %02X status \n",
					  pca9506_IO_3_addr);
			post_log (" > I0_4   : read  %02X status \n",
					  pca9506_IO_4_addr);
		}
		if (java_bd_id != I2C_ID_JUNIPER_EX4200_24_F) {
			goto Done_end;
		}
		if (i2c_dev_count_list[count] == -2) {
			post_log(" Write to the switch pca9546a failed for port 1 \n");
			count++;
		}
		else {
			for (i = 0; i < 8; i++) {
				if (i2c_dev_count_list[count] == 1 ) {
					if (i2c_flag) {
						post_log(" Read SFP%d serail ID for the Port Number: 1 "
								 "of the switch pca9546a \n", i); 
					}
				}
				else {
					post_log(" Read SFP%d serail ID for the Port Number: 1 "
							 "of the switch pca9546a failed \n", i); 
				}
				count++;
			}
		}

		if (i2c_dev_count_list[count] == -2) {
			post_log(" Write to the switch pca9546a failed for port 2\n");
			count++;
		}
		else {
			for (i = 8; i < 16; i++) {
				if (i2c_dev_count_list[count] == 1 ) {
					if (i2c_flag) {
						post_log(" Read SFP%d serail ID for the Port Number: 2 "
								 "of the switch pca9546a \n", i); 
					}
				}
				else {
					post_log(" Read SFP%d serail ID for the Port Number: 2 "
							 "of the switch pca9546a failed \n", i); 
				}
				count++;
			}
			if ( pass == 0 || i2c_flag == 1) {
				post_log (" > IO_0 SFP(0:11)   : read  %02X "
                                         "status \n",
						  pca9506_IO_0_addr_p2);
				post_log (" > IO_1 SFP(0:11)   : read  %02X "
                                         "status \n",
						  pca9506_IO_1_addr_p2);
				post_log (" > IO_2 SFP(0:11)   : read  %02X "
                                         "status \n",
						  pca9506_IO_2_addr_p2);
				post_log (" > IO_3 SFP(0:11)   : read  %02X "
                                         "status \n",
						  pca9506_IO_3_addr_p2);
				post_log (" > I0_4 SFP(0:11)   : read  %02X "
                                         "status \n",
						  pca9506_IO_4_addr_p2);
			}
		}
		if (i2c_dev_count_list[count] == -2) {
			post_log(" Write to the switch pca9546a failed for port 3\n");
			count++;
		}
		else {
			for (i = 16; i < 24; i++) {
				if (i2c_dev_count_list[count] == 1 ) {
					if (i2c_flag) {
						post_log(" Read SFP%d serail ID for the Port Number: 3 "
								 "of the switch pca9546a \n", i); 
					}
				}
				else {
					post_log(" Read SFP%d serail ID for the Port Number: 3 "
							 "of the switch pca9546a failed \n", i); 
				}
				count++;
			}
			if ( pass == 0 || i2c_flag == 1) {
				post_log (" > IO_0 SFP(12:23)   : read  %02X "
                                         "status \n",
						  pca9506_IO_0_addr_p3);
				post_log (" > IO_1 SFP(12:23)   : read  %02X "
                                         "status \n",
						  pca9506_IO_1_addr_p3);
				post_log (" > IO_2 SFP(12:23)   : read  %02X "
                                         "status \n",
						  pca9506_IO_2_addr_p3);
				post_log (" > IO_3 SFP(12:23)   : read  %02X "
                                         "status \n",
						  pca9506_IO_3_addr_p3);
				post_log (" > IO_4 SFP(12:23)   : read  %02X "
                                         "status \n",
						  pca9506_IO_4_addr_p3);
			}
		}
	}
	else {
		post_log("-- I2C test                             PASSED\n");
		if (i2c_flag) {
			for (i = 0; i < i2c_dev_detect_list_size; i++) {
				if (i2c_dev_count_list[i] == 1) {
					post_log (" > I2C: Dev Found     "
                                                 "%-22s  C%1d  M%02xP%1d"
                                                 "  %02x\n",
							  i2c_dev_detect[i].name,
							  i2c_dev_detect[i].i2c_index,
							  i2c_dev_detect[i].switch_addr,
							  i2c_dev_detect[i].channel_index,
							  i2c_dev_detect[i].dev_addr);
				}
			}
			if (uplink_flag) {
				if (uplink_flag == 1) {
					post_log (" > I2C: Dev Found     "
                                                 "%-22s  C%1d  M%02xP%1d"
                                                 "  %02x\n",
							  i2c_dev_list_uplink[0].name,
							  i2c_dev_list_uplink[0].i2c_index,
							  i2c_dev_list_uplink[0].switch_addr,
							  i2c_dev_list_uplink[0].channel_index,
							  i2c_dev_list_uplink[0].dev_addr);
				}
			}
			count =5;	
			for (i = 0;  strcmp(i2c_board_list[i].name, "End_Of_I2c"); i++) {
				if (i2c_dev_count_list[count] == 1) {
					post_log (" > I2C: Dev Found     "
                                                 "%-22s  C%1d  M%02xP%1d"
                                                 "  %02x\n",
							  i2c_board_list[i].name,
							  i2c_board_list[i].i2c_index,
							  i2c_board_list[i].switch_addr,
							  i2c_board_list[i].channel_index,
							  i2c_board_list[i].dev_addr);
				}
				count++;
			}
			post_log (" > IO_0   : read  %02X status \n",
					  pca9506_IO_0_addr);
			post_log (" > IO_1   : read  %02X status \n",
					  pca9506_IO_1_addr);
			post_log (" > IO_2   : read  %02X status \n",
					  pca9506_IO_2_addr);
			post_log (" > IO_3   : read  %02X status \n",
					  pca9506_IO_3_addr);
			post_log (" > I0_4   : read  %02X status \n",
					  pca9506_IO_4_addr);
			if (board_id() != I2C_ID_JUNIPER_EX4200_24_F) {
				goto Done_end;
			}
			for (i = 0; i < 8; i++) {
				post_log(" Read SFP%d serail ID for the Port Number: 1 "
						 "of the switch pca9546a \n",
						 i); 
			}
			for (i = 8; i < 16; i++) {
				post_log(" Read SFP%d serail ID for the Port Number: 2 "
						 "of the switch pca9546a \n",
						 i); 
			}
			post_log (" > IO_0 SFP(0:11)   : read  %02X status \n",
					  pca9506_IO_0_addr_p2);
			post_log (" > IO_1 SFP(0:11)   : read  %02X status \n",
					  pca9506_IO_1_addr_p2);
			post_log (" > IO_2 SFP(0:11)   : read  %02X status \n",
					  pca9506_IO_2_addr_p2);
			post_log (" IO_3 SFP(0:11)   : read  %02X status \n",
					  pca9506_IO_3_addr_p2);
			post_log (" > I0_4 SFP(0:11)   : read  %02X status \n",
					  pca9506_IO_4_addr_p2);
			for (i = 16; i < 24; i++) {
				post_log(" Read SFP%d serail ID for the Port Number: 3 "
						 "of the switch pca9546a \n",
						 i); 
			}
			post_log (" > IO_0 SFP(12:23)   : read  %02X status \n",
					  pca9506_IO_0_addr_p3);
			post_log (" > IO_1 SFP(12:23)   : read  %02X status \n",
					  pca9506_IO_1_addr_p3);
			post_log (" > IO_2 SFP(12:23)   : read  %02X status \n",
					  pca9506_IO_2_addr_p3);
			post_log (" > IO_3 SFP(12:23)   : read  %02X status \n",
					  pca9506_IO_3_addr_p3);
			post_log (" > I0_4 SFP(12:23)   : read  %02X status \n",
					  pca9506_IO_4_addr_p3);
		}
	}
Done_end:		
	if (pass == 1 &&  fail == 0 ) {
		return 0;
	}
	else {
		return -1;
	}
}
#endif /* CONFIG_POST & CFG_POST_I2C */
#endif /* CONFIG_POST */
