/*
 * Copyright (c) 2008-2010, Juniper Networks, Inc.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>

#include <i2c.h>
#include "syspld.h"

#undef	EX3242_DEBUG
#ifdef	EX2200_DEBUG
#define debug(fmt,args...)	printf (fmt ,##args)
#else
#define debug(fmt,args...)
#endif

int 
syspld_valid_read_reg (int reg)
{
    if (reg <= SYSPLD_LAST_REG)
        return (1);
    else 
        return (0);
}

int 
syspld_valid_write_reg (int reg)
{
    if ((reg <= SYSPLD_LAST_REG) &&
        (reg != SYSPLD_REVISION) &&
        (reg != SYSPLD_CREATE_DAY) &&
        (reg != SYSPLD_CREATE_MONTH) &&
        (reg != SYSPLD_CREATE_YEAR_1) &&
        (reg != SYSPLD_CREATE_YEAR_2))
        return (1);
    else 
        return (0);
}


int 
syspld_reg_read (int reg, uint8_t *val)
{
    uint8_t temp;
    
    if (syspld_valid_read_reg(reg)) {
        if (i2c_read(0, I2C_SYSPLD_ADDR, reg, 1, &temp, 1))
            return (-1);
        *val = temp;
    }
    return (0);
}

int 
syspld_reg_write (int reg, uint8_t val)
{
    uint8_t temp, magic[2] = {SYSPLD_MAGIC_0, SYSPLD_MAGIC_1};
    
    if (syspld_valid_write_reg(reg)) {
        if ((reg >= SYSPLD_FIRST_RESET_BLOCK_REG) && 
            (reg <= SYSPLD_LAST_RESET_BLOCK_REG)) {
            temp = SYSPLD_MAGIC_0;
            if (i2c_write(0, I2C_SYSPLD_ADDR, SYSPLD_RESET_BLOCK_UNLOCK_1, 1, magic, 2))
                return (-1);
            temp = val;
            if (i2c_write(0, I2C_SYSPLD_ADDR, reg, 1, &temp, 1))
                return (-1);
        }
        else {
            temp = val;
            if (i2c_write(0, I2C_SYSPLD_ADDR, reg, 1, &temp, 1))
                return (-1);
        }
    }
    return (0);
}

int 
syspld_read (int32_t offset, uint8_t *data, int32_t length)
{
    
    if (i2c_read(0, I2C_SYSPLD_ADDR, offset, 1, data, length)) {
        return (-1);
    }

    return (0);
}
