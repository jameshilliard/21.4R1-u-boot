/*
 * $Id$
 *
 * i2c_usb2513.h -- register definitions for usb hub - USB2513i
 *
 * Rajat Jain, Feb 2011
 *
 * Copyright (c) 2011-2014, Juniper Networks, Inc.
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

#ifndef _I2C_USB2513_H_
#define _I2C_USB2513_H_

#define USB2513_VENDORID_LSB    0x00
#define USB2513_VENDORID_MSB    0x01
#define USB2513_PRODID_LSB      0x02
#define USB2513_PRODID_MSB      0x03
#define USB2513_DEVID_LSB       0x04
#define USB2513_DEVID_MSB       0x05
#define USB2513_CONF1           0x06
#define USB2513_CONF2           0x07
#define USB2513_CONF3           0x08
#define USB2513_NONREM_DEV      0x09
#define USB2513_PORTDIS_SELF    0x0A
#define USB2513_PORTDIS_BUS     0x0B
#define USB2513_MAXPOWER_SELF   0x0C
#define USB2513_MAXPOWER_BUS    0x0D
#define USB2513_HUBC_MAX_I_SELF 0x0E
#define USB2513_HUBC_MAX_I_BUS  0x0F
#define USB2513_POWERON_TIME    0x10
#define USB2513_LANGID_HIGH     0x11
#define USB2513_LANGID_LOW      0x12
#define USB2513_MFG_STR_LEN     0x13
#define USB2513_PROD_STR_LEN    0x14
#define USB2513_SER_STR_LEN     0x15
#define USB2513_MFG_STR_BASE    0x16
#define USB2513_PROD_STR_BASE   0x54
#define USB2513_SER_STR_BASE    0x92
#define USB2513_BAT_CHARG_EN    0xD0
#define USB2513_BOOSTUP         0xF6
#define USB2513_BOOST_75        0xF7
#define USB2513_BOOST_40        0xF8
#define USB2513_PORT_SWAP       0xFA
#define USB2513_PORT_MAP12      0xFB
#define USB2513_PORT_MAP34      0xFC
#define USB2513_PORT_MAP56      0xFD
#define USB2513_PORT_MAP7       0xFE
#define USB2513_STATCMD         0xFF

#define USB2513_MAX_STRLEN      62
#define USB2513_BUFSIZE         3


/*
 * Definitions for Vendor Id LSB register
 */
#define USB2513_VENDOR_ID_SMSC_LSB      0x24

/*
 * Definitions for Vendor Id MSB register
 */
#define USB2513_VENDOR_ID_SMSC_MSB      0x04

/*
 * Definitions for Product Id LSB register
 */
#define USB2513_PRODID_USB2513_LSB      0x13

/*
 * Definitions for Product Id MSB register
 */
#define USB2513_PRODID_USB2513_MSB      0x25

/*
 * Definitions for Device Id LSB register
 */
#define USB2513_DEVID_DEFAULT_LSB       0xA0

/*
 * Definitions for Device Id LSB register
 */
#define USB2513_DEVID_DEFAULT_MSB       0x0B

/*
 * Definitions for Configuration Data Byte 1 register
 */
#define USB2513_CONF1_SELF_PWR          0x80
#define USB2513_CONF1_HS_DIS            0x20
#define USB2513_CONF1_MTT_EN            0x10
#define USB2513_CONF1_EOP_DIS           0x08
#define USB2513_CONF1_CURR_SNS_DIS      0x04
#define USB2513_CONF1_CURR_SNS_PERPORT  0x02
#define USB2513_CONF1_PERPORT_POWER_SW  0x01

/*
 * Definitions for Configuration Data Byte 2 register
 */
#define USB2513_CONF2_DYNAMIC_AUTOSW_EN 0x80
#define USB2513_CONF2_OC_TIMER_16MS     0x30
#define USB2513_CONF2_OC_TIMER_8MS      0x20
#define USB2513_CONF2_OC_TIMER_4MS      0x10
#define USB2513_CONF2_COMPOUND_DEV      0x08

#define USB2513_CONF2_OC_TIMER_MASK     0x30


/*
 * Definitions for Configuration Data Byte 3 register
 */
#define USB2513_CONF3_PORTMAP_EN        0x08
#define USB2513_CONF3_LED_MODE_SPEED    0x02
#define USB2513_CONF3_STRING_SUPPORT_EN 0x01

#define USB2513_CONF3_LED_MODE_MASK     0x06

/*
 * Definitions for Non-Removable Devices register
 */
#define USB2513_NONREM_DEV_PORT3        0x08
#define USB2513_NONREM_DEV_PORT2        0x04
#define USB2513_NONREM_DEV_PORT1        0x02

/*
 * Definitions for Port Disable - Self Power register
 */
#define USB2513_PORTDIS_SELF_PORT3      0x08
#define USB2513_PORTDIS_SELF_PORT2      0x04
#define USB2513_PORTDIS_SELF_PORT1      0x02

/*
 * Definitions for Port Disable - Bus Power register
 */
#define USB2513_PORTDIS_BUS_PORT3       0x08
#define USB2513_PORTDIS_BUS_PORT2       0x04
#define USB2513_PORTDIS_BUS_PORT1       0x02

/*
 * Definition for Max Power - Self register
 */
#define USB2513_MAXPOWER_SELF_2MA      0x01  /* 2mA (in 2mA increments) */

/*
 * Definition for Max Power - Bus register
 */
#define USB2513_MAXPOWER_BUS_100MA     0x32  /* 100mA (in 2mA increments) */

/*
 * Definition for Hub Controller Max Current(Self) register
 */
#define USB2513_HUBC_MAX_I_SELF_2MA    0x01  /* 2mA (in 2mA increments) */

/*
 * Definition for Hub Controller Max Current(Bus) register
 */
#define USB2513_HUBC_MAX_I_BUS_100MA  0x32  /* 100mA (in 2mA increments) */

/*
 * Definition for Power-On Time register
 */
#define USB2513_POWERON_TIME_100MS     0x32  /* 100ms (in 2ms intervals) */

/*
 * Language ID for English(US) as defined by USB-IF
 */
#define USB2513_LANGID_ENGLISH_US_MSB  0x04
#define USB2513_LANGID_ENGLISH_US_LSB  0x09

/*
 * Definition for Power-On Time register
 */
#define USB2513_POWERON_TIME_100MS     0x32  /* 100ms (in 2ms intervals) */

/*
 * Definition for Battery Charging Enable Register
 */
#define USB2513_BAT_CHARG_EN_PORT3       0x08
#define USB2513_BAT_CHARG_EN_PORT2       0x04
#define USB2513_BAT_CHARG_EN_PORT1       0x02

/*
 * Definition for Boost_Up register
 */
#define USB2513_BOOSTUP_HIGH             0x03  /* 4%  boost */
#define USB2513_BOOSTUP_MED              0x02  /* 8%  boost */
#define USB2513_BOOSTUP_LOW              0x01  /* 12% boost */ 

#define USB2513_BOOSTUP_MASK             0x03

/*
 * Definition for Boost_4:0 register
 */
#define USB2513_BOOST_40_PORT3_HIGH      0x30  /* 4%  boost */
#define USB2513_BOOST_40_PORT3_MED       0x20  /* 8%  boost */
#define USB2513_BOOST_40_PORT3_LOW       0x10  /* 8%  boost */
#define USB2513_BOOST_40_PORT2_HIGH      0x0c  /* 4%  boost */
#define USB2513_BOOST_40_PORT2_MED       0x08  /* 8%  boost */
#define USB2513_BOOST_40_PORT2_LOW       0x04  /* 8%  boost */
#define USB2513_BOOST_40_PORT1_HIGH      0x03  /* 4%  boost */
#define USB2513_BOOST_40_PORT1_MED       0x02  /* 8%  boost */
#define USB2513_BOOST_40_PORT1_LOW       0x01  /* 8%  boost */

/*
 * Definitions for Port Swap register
 */
#define USB2513_PORT_SWAP_PORT3          0x08
#define USB2513_PORT_SWAP_PORT2          0x04
#define USB2513_PORT_SWAP_PORT1          0x02

/*
 * Definitions for Port Map12 register
 */
#define USB2513_PORT_MAP12_DEFAULT       0x21

/*
 * Definitions for Port Map34 register
 */
#define USB2513_PORT_MAP34_DEFAULT       0x03

/*
 * Definitions for Status/Command register
 */
#define USB2513_STATCMD_INTF_PW_DN       0x04
#define USB2513_STATCMD_RESET            0x02
#define USB2513_STATCMD_USB_ATTACH       0x01

#define USB2513_MFR_STRING               "SMSC"
#define USB2513_PROD_STRING              "USB2513Bi"
#define USB2513_SERNO_STRING             "0000"

/*
 * Definitions for Kotinos port mapping
 */
#define USB2513_PORTMAP_P1_TO_P3        0x03
#define USB2513_PORTMAP_P2_TO_P1        0x10
#define USB2513_PORTMAP_P3_TO_P2        0x02

#endif /* _I2C_USB2513_H_ */
