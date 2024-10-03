/*
 * Copyright (c) 2010-2012, Juniper Networks, Inc.
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

/* Author : Bilahari Akkriaju */
/* SOHO Board specific definitions and values  */

#ifndef __SOHO_BOARD_H__
#define __SOHO_BOARD_H__


#define MAX_CHANNEL    8
#define WA_MAX_CHANNEL 4
#define SOHO_DELAY     5000
#define CHAN_MASK      0xffc0
#define WA_CHAN_MASK   0xff80
#define WA_CHAN_MASK   0xff80
#define GPIO_I2C_SEL_MASK    0x3F

/* power supply i2c addr*/
#define PS_I2C_ADDR          0x58
#define I2C_MAX_ADDR   127

/* Temp sensor defines*/
#define TEMP_MSB_MASK      0x8000
#define TEMP_MNTSA_MASK    0x1FF
#define TEMP_EXP_MASK      0x003F
#define TEMP_9_BIT_RES     511
#define TEMP_16_BIT_RES    8192

/* PSMI define */
#define PSMI_DIS_STR_OFFSET 0x3E
#define PSMI_RPM_REG_OFFSET 0x20
#define PSMI_DEFAULT_FAN_RPM 4000
#define PSMI_FAN_CFG_REG_OFFSET 0x63

#define SOHO_TEMP_MAX6697 0x1C
#define SOHO_TEMP_SENSOR1 0x48
#define SOHO_TEMP_SENSOR2 0x49
#define SOHO_TEMP_SENSOR3 0x4A
#define SOHO_TEMP_SENSOR4 0x4B

#define PWR_CPLD_I2C_ADDR  0x35
#define PWR_CPLD_ATTN_ON   0xF0
#define PWR_CPLD_ATTN_OFF  0xE0
#define PWR_CPLD_ATTN_REG  0x09
#define PWR_CPLD_VOL_HI    0x08
#define PWR_CPLD_VOL_LOW   0x07

#define PWR_LTC2978_I2C_ADDR  0x5C

#define MCPLD_RST_REG_OFFSET 0x50

#define SOHO_MAX6697_SENSORS 4
#define SOHO_PWR1220_SENSORS 12
#define SOHO_LTC2978_SENSORS 8

#define SOHO_MARGIN_NORMAL  0
#define SOHO_MARGIN_LOW     1
#define SOHO_MARGIN_HIGH    2
#define SOHO_MARGIN_LEVELS  3

#define SOHO_PWR1220_MARGIN_DEVICES 8
#define SOHO_LTC2978_MARGIN_DEVICES 8

#endif /* __SOHO_BOARD_H__ */



