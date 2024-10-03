/*
 * Copyright (c) 2009-2012, Juniper Networks, Inc.
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

#include "rmi/gpio.h"
#include "rmi/pcmcia.h"
#include "rmi/on_chip.h"
#include "rmi/board.h"
#include "../common/fx_cpld.h"
#include "common.h"
#include "../common/fx_common.h"

#ifndef _SOHO_CPLD_MAP_H_
#define _SOHO_CPLD_MAP_H_

#define SOHO_SCPLD_0_INTR_SRC_TOP       0x0
#define SOHO_CPLD_REG_FIRST             SOHO_SCPLD_0_INTR_SRC_TOP
#define SOHO_SCPLD_1_INTR_SRC_TOP       0x1
#define SOHO_SCPLD_2_INTR_SRC_TOP       0x2
#define SOHO_SCPLD_3_INTR_SRC_TOP       0x3
#define SOHO_SCPLD_4_INTR_SRC_TOP       0x4
#define SOHO_SCPLD_5_INTR_SRC_TOP       0x5
#define SOHO_SCPLD_6_INTR_SRC_TOP       0x6
#define SOHO_SCPLD_7_INTR_SRC_TOP       0x7

#define SOHO_MCPLD_INTR_SRC_TOP_0       0x10
#define SOHO_MCPLD_REG_FIRST            SOHO_MCPLD_INTR_SRC_TOP_0
#define SOHO_MCPLD_INTR_SRC_TOP_1       0x11

#define SOHO_MCPLD_INTR_FAN_0           0x20
#define SOHO_MCPLD_INTR_FAN_1           0x21
#define SOHO_MCPLD_INTR_TEMP            0x22
#define SOHO_MCPLD_INTR_TEMP_CRIT       0x23
#define SOHO_MCPLD_INTR_MGT_BRD         0x24
#define SOHO_MCPLD_INTR_ASIC            0x25

#define SOHO_MCPLD_INTR_FAN_0_EN        0x30
#define SOHO_MCPLD_INTR_FAN_1_EN        0x31
#define SOHO_MCPLD_INTR_TEMP_EN         0x32

#define SOHO_MCPLD_INTR_MGT_BRD_EN      0x34
#define SOHO_MCPLD_INTR_ASIC_EN         0x35

#define SOHO_MCPLD_FAN_PWR_PRE          0x40

#define SOHO_MCPLD_FAN_PWR_PRE_CHN      0x44

#define SOHO_MCPLD_FAN_PWR_PRE_CHN_EN   0x48

#define SOHO_MCPLD_EXT_RST_0            0x50
#define SOHO_MCPLD_EXT_RST_1            0x51

#define SOHO_MCPLD_FAN_SPEED            0x58
#define SOHO_MCPLD_FAN_LED              0x59

#define SOHO_MCPLD_MDIO_OP              0x5A
#define SOHO_MCPLD_MDIO_PHY_ADDR        0x5B
#define SOHO_MCPLD_MDIO_DEV_ADDR        0x5C
#define SOHO_MCPLD_MDIO_HI_ADDR         0x5D
#define SOHO_MCPLD_MDIO_LOW_ADDR        0x5E
#define SOHO_MCPLD_MDIO_CTRL_ADDR       0x5F

#define SOHO_MCPLD_UPL_OPT_0_INTR       0x60
#define SOHO_MCPLD_UPL_OPT_1_INTR       0x61
#define SOHO_MCPLD_UPL_OPT_2_INTR       0x62
#define SOHO_MCPLD_UPL_OPT_3_INTR       0x63
#define SOHO_MCPLD_UPL_OPT_0_STAT       0x64
#define SOHO_MCPLD_UPL_OPT_1_STAT       0x65
#define SOHO_MCPLD_UPL_OPT_2_STAT       0x66
#define SOHO_MCPLD_UPL_OPT_3_STAT       0x67

#define TOR_MCPLD_I2C_CMD_ADDR_0        0x68
#define TOR_MCPLD_I2C_CMD_ADDR_1        0x69
#define TOR_MCPLD_I2C_DATA_0            0x6A
#define TOR_MCPLD_I2C_DATA_1            0x6B
#define TOR_MCPLD_I2C_DATA_2            0x6C
#define TOR_MCPLD_I2C_DATA_3            0x6D

#define SOHO_MCPLD_BOOT_KEY             0x70

#define SOHO_MCPLD_CPU_RST              0x74
#define SOHO_MCPLD_CPU_RST_STATUS       0x75
#define SOHO_MCPLD_WD                   0x76
#define SOHO_MCPLD_WD_TIME              0x77
#define SOHO_MCPLD_MISC_GPIO            0x78
#define SOHO_MCPLD_MISC                 0x79

#define SOHO_RCPLD_SCRATCH              0x71
#define SOHO_RCPLD_CPU_RST              SOHO_MCPLD_CPU_RST
#define SOHO_RCPLD_CPU_RST_STATUS       SOHO_MCPLD_CPU_RST_STATUS
#define SOHO_RCPLD_WD                   SOHO_MCPLD_WD
#define SOHO_RCPLD_WD_TIME              SOHO_MCPLD_WD_TIME
#define SOHO_RCPLD_MISC_GPIO            SOHO_MCPLD_MISC_GPIO
#define SOHO_RCPLD_MISC                 SOHO_MCPLD_MISC

#define SOHO_MCPLD_VERSION              0x7C
#define SOHO_MCPLD_SCRATCH              0x7D
#define SOHO_MCPLD_REG_LAST             SOHO_MCPLD_SCRATCH

#define SOHO_SCPLD_SFP_0_INTR           0x80

#define QFX3500_RST_REG_0               0x90
#define QFX3500_RST_REG_1               0x91
#define QFX3500_RST_REG_2               0x92
#define QFX3500_RST_REG_3               0x93

#define SOHO_SCPLD_SFP_RST_0            0xE0

#define SOHO_SCPLD_AEL_0_PHY_RST        0xE0
#define SOHO_SCPLD_AEL_1_PHY_RST        0xE1
#define SOHO_SCPLD_AEL_2_PHY_RST        0xE2

#define SOHO_SCPLD_10G_S1_PHY_RST       0xE2
#define SOHO_SCPLD_10G_S7_PHY_RST       0xE3
#define SOHO_SCPLD_ML_PHY_RST           0xE4

#define SOHO_SCPLD_AEL_3_PHY_RST        0xE5

#define SOHO_SCPLD_MGT_BRD_INTR_EN      0xF0
#define SOHO_SCPLD_MGT_BRD_PHY_RST      0xF1
#define SOHO_SCPLD_LCD_LED              0xF2
#define SOHO_SCPLD_LCD_TUNE             0xF3
#define SOHO_SCPLD_LCD_ACCESS           0xF4
#define SOHO_SCPLD_LCD_RS               0xF5
#define SOHO_SCPLD_LCD_DATA             0xF6
#define SOHO_SCPLD_PHY_LED              0xF7

#define SOHO_SCPLD_SFP_C0_PRE           0xF8
#define SOHO_SCPLD_SFP_C1_PRE           0xF9

#define QFX5500_SCPLD_SFP_I2C_ADDR      0xFA
#define QFX5500_SCPLD_SFP_I2C_DATA_LSB  0xFB
#define QFX5500_SCPLD_SFP_I2C_DATA_MSB  0xFC
#define QFX5500_SCPLD_SFP_I2C_CNTL      0xFD
#define QFX5500_SCPLD_INTR_SRC_TOP      0xFF
#define SOHO_CPLD_REG_LAST              0xFF 

#endif
