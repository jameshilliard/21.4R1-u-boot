/*
 * Copyright (c) 2010, Juniper Networks, Inc.
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

#include <common.h>

#include <i2c.h>
#include "syspld.h"

#undef	EX3300_DEBUG
#ifdef	EX3300_DEBUG
#define debug(fmt,args...)	printf (fmt ,##args)
#else
#define debug(fmt,args...)
#endif

#define SYSPLD_ADDR_SHIFT 6
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
    volatile uint8_t *syspld = (uint8_t *)CFG_SYSPLD_ADDRESS;
    
    if (syspld_valid_read_reg(reg)) {
        *val = syspld[reg <<SYSPLD_ADDR_SHIFT];
        return (0);
    }
    return (-1);
}

int 
syspld_reg_write (int reg, uint8_t val)
{
    volatile uint8_t *syspld = (uint8_t *)CFG_SYSPLD_ADDRESS;
    
    if (syspld_valid_write_reg(reg)) {
        if ((reg >= SYSPLD_FIRST_RESET_BLOCK_REG) && 
            (reg <= SYSPLD_LAST_RESET_BLOCK_REG)) {
            syspld[SYSPLD_RESET_BLOCK_UNLOCK_1 << SYSPLD_ADDR_SHIFT] = 
                SYSPLD_MAGIC_0;
            syspld[SYSPLD_RESET_BLOCK_UNLOCK_2 << SYSPLD_ADDR_SHIFT] = 
                SYSPLD_MAGIC_1;
            syspld[reg << SYSPLD_ADDR_SHIFT] = val;
        }
        else {
            syspld[reg << SYSPLD_ADDR_SHIFT] = val;
        }
        return (0);
    }
    else
        return (-1);
}

