/*
 * Copyright (c) 2007-2010, Juniper Networks, Inc.
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
#include "epld.h"

#define	EX3242_DEBUG
#undef debug
#ifdef	EX3242_DEBUG
#define debug(fmt,args...)	printf (fmt ,##args)
#else
#define debug(fmt,args...)
#endif

int epld_valid_read_reg(int reg)
{
    if (reg <= EPLD_LAST_REG)
        return 1;
    else 
        return 0;
}

int epld_valid_write_reg(int reg)
{
    if ((reg <= EPLD_LAST_REG) &&
        (reg != EPLD_WATCHDOG_LEVEL_DEBUG) &&
        (reg != EPLD_INT_STATUS1) &&
        (reg != EPLD_INT_STATUS2))
        return 1;
    else 
        return 0;
}


int epld_reg_read(int reg, uint16_t *val)
{
    volatile uint16_t *epld = (uint16_t *)CFG_EPLD_BASE;
    
    if (epld_valid_read_reg(reg)) {
        if ((reg <= EPLD_LAST_RESET_BLOCK_REG) && 
            (reg > EPLD_RESET_BLOCK_UNLOCK)) {
            epld[0] = EPLD_MAGIC;
            *val = epld[reg];
        }
        else {
            *val = epld[reg];
        }
        return 1;
    }
    return 0;
}

int epld_reg_write(int reg, uint16_t val)
{
    volatile uint16_t *epld = (uint16_t *)CFG_EPLD_BASE;
    
    if (epld_valid_write_reg(reg)) {
        if ((reg <= EPLD_LAST_RESET_BLOCK_REG) && 
            (reg > EPLD_RESET_BLOCK_UNLOCK)) {
            epld[0] = EPLD_MAGIC;
            epld[reg] = val;
        }
        else {
            epld[reg] = val;
        }
        return 1;
    }
    return 0;
}
