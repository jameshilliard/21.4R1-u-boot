/*
 * Copyright (c) 2011, Juniper Networks, Inc.
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
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/gpl-2.0.html>.
 */

#ifndef __SOHO_POST_I2C__
#define __SOHO_POST_I2C__

#define SOHO_NO_CHECK           (uint8_t)0
#define SOHO_PS0_PRESENT        (uint8_t)1
#define SOHO_PS1_PRESENT        (uint8_t)2
#define SOHO_DDR2A_PRESENT      (uint8_t)3
#define SOHO_DDR2B_PRESENT      (uint8_t)4
#define SOHO_DDR2C_PRESENT      (uint8_t)5
#define SOHO_DDR2D_PRESENT      (uint8_t)6
#define SOHO_SEEPROM_PRESENT    (uint8_t)7
#define SOHO_SFP_PRESENT        (uint8_t)8
#define SOHO_CXP0_PRESENT       (uint8_t)9
#define SOHO_CXP1_PRESENT       (uint8_t)10
#define SOHO_TL0_PRESENT        (uint8_t)11
#define SOHO_TL1_PRESENT        (uint8_t)12
#define SOHO_TQ_PRESENT         (uint8_t)13
#define SOHO_DONT_CARE          (uint8_t)0xFF

#define SOHO_MAX_I2C_DEVICES    80
#define SOHO_MAX_I2C_NAME_LEN   20

typedef struct soho_i2c_dev_struct {
    uint8_t bus;
    uint8_t pseg;
    uint8_t sseg;
    uint8_t present;
    uint8_t addr;
    uint8_t reg;
    char *name;
    uint32_t i2c_mode;
} soho_i2c_dev;

typedef struct soho_i2c_dev_detect_struct {
    uint8_t bus;
    uint8_t pseg;
    uint8_t sseg;
    uint8_t present;
    uint8_t found;
    uint8_t index;
    uint8_t dev_index;
    uint32_t i2c_mode;
} soho_i2c_dev_detect;

#endif /*__SOHO_POST_I2C__*/
