/*
 * Copyright (c) 2008-2011, Juniper Networks, Inc.
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

#ifndef __EX82XX_DEVICES_H__
#define __EX82XX_DEVICES_H__

/* SCB I2C controller #1 */
#define SCB_RTC_I2C_ADDR         0x68
#define SCB_ID_EEPROM_I2C_ADDR   0x57
#define SCB_VOLT_MON_I2C_ADDR    0x35
#define SCB_IO_EXP_1_I2C_ADDR    0x26
#define SCB_IO_EXP_2_I2C_ADDR    0x27

/* SCB I2C controller #2 */
#define SCB_DDR_I2C_ADDR         0x50


/* SCB I2C through scbc fpga */
#define SCB_TEMP_SENSOR_EXHAUST_A_ADDR      0x48
#define SCB_TEMP_SENSOR_EXHAUST_B_ADDR      0x49
#define SCB_TEMP_SENSOR_INTAKE_A_ADDR       0x4A
#define SCB_TEMP_SENSOR_INTAKE_B_ADDR       0x4B
#define SCB_SLAVE_ID_SERIAL_EEPROM_ADDR     0x51
#define SCB_SLAVE_CPLD_ADDR                 0x54
#define SCB_SLAVE_I2C_BUS_SWITCH_ADDR       0x76


/* LCPU I2C controller #1 */
#define LCPU_ID_EEPROM_I2C_ADDR  0x57


/* Linecard i2c devices connected to i2c controller 1 of LCPU*/
#define LC_ID_SLAVE_EEPROM_I2C_ADDR    0x51


#define LC_I2C_SW_1_I2C_ADDR     0x70

/* Linecard i2c devices connected to i2c controller 1 , Switch1 of LCPU*/
#define LC_SW1_CH0_PFE0_CTRL_I2C_ADDR     0xA0
#define LC_SW1_CH0_IO_EXP_I2C_ADDR        0x18
#define LC_SW1_CH0_SF0_TEMP_I2C_ADDR      0x4C

#define LC_SW1_CH1_PFE1_CTRL_I2C_ADDR     0xA0
#define LC_SW1_CH1_IO_EXP_I2C_ADDR        0x18
#define LC_SW1_CH1_SF1_TEMP_I2C_ADDR      0x4C

#define LC_SW1_CH2_PEX8508_I2C_ADDR       0x75



/* Linecard i2c devices connected to i2c controller 2 of LCPU*/
#define LC_I2C_SW_2_I2C_ADDR             0x70
#define LC_SW2_CH0_SFP_LED_I2C_ADDR      0x18
#define LC_SW2_CH0_SW3_SFP_I2C_ADDR      0x71


#define LC_SW2_CH1_IO_SFP_TX_DIS_I2C_ADDR          0x40
#define LC_SW2_CH1_IO_SFP_P0_P11_LED_I2C_ADDR      0x41
#define LC_SW2_CH1_IO_SFP_P12_P23_LED_I2C_ADDR     0x42

#define LC_SW2_CH1_SW4_SFP_I2C_ADDR   0x71
#define LC_SW2_CH1_SW5_SFP_I2C_ADDR   0x72
#define LC_SW2_CH1_SW6_SFP_I2C_ADDR   0x73

#define LC_SFP_I2C_ADDR   0x50



/* PCI /PCIE Devices */
#define PCIE_FREESCALE_VID 0x1957
#define PCIE_MPC8548_DID   0x0012
#define PCIE_P2020E_DID	   0x0070
#define PCIE_P2020_DID     0x0071

#define PCIE_PLXTECH_VID  0x10B5
#define PCIE_PEX8111_DID  0x8111
#define PCIE_PEX8508_DID  0x8508
#define PCIE_PEX8509_DID  0x8509

#define PCIE_BROADCOM_VID 0x14E4
#define PCIE_BCM56307_DID 0xB307
#define PCIE_BCM56305_DID 0xB305


#define PCIE_MARVELL_VID   0x11ab   
#define PCIE_PUMAJ_DID     0xc340
#define PCIE_PUMAJ_c341_DID     0xc341
#define PCIE_CHEETAH_DID	0xdb00

#define PCIE_JUNIPER_VID    0x1855
#define PCIE_SNTL_DID	    0xabcd

#define PCIE_NXP_VID          0x1131
#define PCIE_NXP1564_OHCI_DID 0x1561
#define PCIE_NXP1564_EHCI_DID 0x1562

#define PCIE_JNPRNET_VID      0x1172
#define PCIE_SCBC_DID         0x001E
#define PCIE_FSENT_DID        0xBABE

#endif /* __EX82XX_DEVICES_H__ */
