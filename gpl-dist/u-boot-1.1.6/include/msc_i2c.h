/*
 * Copyright (c) 2011, Juniper Networks, Inc.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. if not ,see
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0,html>  
 */


#ifndef _msc_i2c_h_
#define _msc_i2c_h_


/* Master CPLD I2C buses */
/* 
 * these defines are located here and not in the msc_cpld.h as they are
 * I2C specific
 */ 

/* I2C BUS #  */
/* bus addressed by base + card # */ 
#define I2C_BUS_CARD_BASE   0
/* 
 * The following definitions for device addresses on BUS N (0-11)
 * i.e. temp sensor of Card 11 11,0x4C to address the device 
*/
#define TEMP_SENS           0x4c
#define ID_EEPROM           0x51
#define SCARPINE_CPLD       0x52
#define MEZZ_ID_EEPROM      0x57
#define SECURITY_DEV        0x62


#define I2C_BUS_CARD_0      0
#define I2C_BUS_CARD_1      1
#define I2C_BUS_CARD_2      2
#define I2C_BUS_CARD_3      3
#define I2C_BUS_CARD_4      4
#define I2C_BUS_CARD_5      5
#define I2C_BUS_CARD_6      6
#define I2C_BUS_CARD_7      7
#define I2C_BUS_CARD_8      8
#define I2C_BUS_CARD_9      9
#define I2C_BUS_CARD_10     10
#define I2C_BUS_CARD_11     11

#define MSC_SLAVE_BUS       12

#define MSC_SLAVE_EEPROM    0x51
#define MSC_SLAVE_CPLD      0x54
#define MSC_SLAVE_TEMP_SENS 0x4C

/* 
 * On Bus 12 device address 0x19
 * We have a ispPAC-PWR1014 device
 * See Data Sheet to further define registers
 * content 
 */ 
 
#define MSC_SLAVE_PWR_SEQ   0x19




#define MSC_FAN_CTRL_BUS    13
#define MSC_FAN_CTRL        0x58

#define MIDPLAN_PWR_SUP_BUS 14
/* addressed base + sup # */
#define MIDPLAN_PWR_SUP_BASE 0x78
#define MIDPLAN_PWR_SUP_1   0x78
#define MIDPLAB_PWR_SUP_2   0x7A
#define MIDPLAN_PWR_SUP_3   0x7C
#define MIDPLAB_PWR_SUP_4   0x7E

#define MIDPLAN_EEPROM_BUS  15
#define MIDPLAN_EEPROM      0x51


#define FSC_BUS             16
#define FSC_EEPROM          0x51
#define MSC_ETH_PHY         0x5F

#define CHEETAH_BUS         17
#define CHEETAH_SWITCH      0xC0
#define CHEETAH_CFG_EEPROM  0x5E

#define LION_TWSI_0_BUS     18
#define LION_TWSI_0_SWITCH  0xC0
#define LION_TWSI_0_CFG_EEPROM 0x50

#define LION_TWSI_1_BUS     19
#define LION_TWSI_1_SWITCH  0xC0
#define LION_TWSI_1_CFG_EEPROM 0x51

#define LION_TWSI_2_BUS     20
#define LION_TWSI_2_SWITCH  0xC0
#define LION_TWSI_2_CFG_EEPROM 0x52

#define LION_TWSI_3_BUS     21
#define LION_TWSI_3_SWITCH  0xC0
#define LION_TWSI_3_CFG_EEPROM 0x53




/* Function prototypes */
uint8_t msc_get_clk(void);

uint8_t msc_i2c_read_8(uint8_t bus, 
		       uint8_t dev, 
		       uint8_t addr,
		       uint8_t *data, 
		       uint8_t clk);

uint8_t msc_i2c_read_16(uint8_t bus, 
			uint8_t dev, 
			uint8_t addr, 
			uint16_t *data, 
			uint8_t clk);

uint8_t msc_i2c_write_8(uint8_t bus, 
			uint8_t dev, 
			uint8_t addr, 
			uint8_t data, 
			uint8_t clk);

uint8_t msc_i2c_write_16(uint8_t bus, 
			 uint8_t dev, 
			 uint8_t addr, 
			 uint16_t data,  
			 uint8_t clk);
#endif /* msc_i2c_h */
