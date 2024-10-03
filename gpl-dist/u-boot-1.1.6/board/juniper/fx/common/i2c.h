/*
 * Copyright (c) 2009-2010, Juniper Networks, Inc.
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

#ifndef __I2C_FX_H__
#define __I2C_FX_H__
#include <rmi/types.h>

#define MAX_I2C_ADDR           128

#define MAX_I2C_BUSES          2
#define MAX_EEPROM_DEVICES     1

#define EEPROM_ADDR            0x57
#define ERR_LOG_EEPROM         0x00

#define I2C_BUS0               0
#define I2C_BUS1               1

#define MFG_EEPROM_HEAD_OFFSET 0x00
#define MFG_EEPROM_REV_OFFSET  12
#define MFG_EEPROM_NUM_OFFSET  16
#define MFG_EEPROM_SER_OFFSET  32
#define MFG_EEPROM_TAIL_OFSET  42


extern int i2c_read(uint i2c_bus,
        uchar chip,
        uint addr,
        uchar *buffer,
        int len);
extern int i2c_write(uint i2c_bus,
        uchar chip,
        uint addr,
        uchar *buffer,
        int len);
extern int i2c_write_fill(uint i2c_bus,
        uchar chip,
        uint addr,
        uchar *ch,
        int count);
extern int i2c_probe(unsigned char bus, unsigned int slave_addr);
#endif

