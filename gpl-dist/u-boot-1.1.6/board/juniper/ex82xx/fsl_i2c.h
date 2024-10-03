/* 
 * Copyright (c) 2008-2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * Freescale I2C Controller
 *
 * Copyright 2006 Freescale Semiconductor, Inc.
 *
 * Based on earlier versions by Gleb Natapov <gnatapov@mrv.com>,
 * Xianghua Xiao <x.xiao@motorola.com>, Eran Liberty (liberty@freescale.com),
 * and Jeff Brown.
 * Some bits are taken from linux driver writen by adrian@humboldt.co.uk.
 *
 * This software may be used and distributed according to the
 * terms of the GNU Public License, Version 2, incorporated
 * herein by reference.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 as published by the Free Software Foundation.
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
 */

#ifndef _FSL_I2C_H_
#define _FSL_I2C_H_

#include <asm/types.h>
#include <asm/fsl_i2c.h>

#define I2C ((fsl_i2c_t *)gd->mpc_i2c)

/*
 * MPC8548 has two i2c bus
 */
extern void i2c_controller(int controller);
extern int  i2c_probe_caffeine(uint i2c_ctrl, uchar sw_addr, uchar ch_index, uchar chip);
extern void i2c_reset(void);
extern void i2c_fdr(uint8_t clock);
extern int  i2c_raw_read(u8 dev, u8 *data, int length);
extern int  i2c_raw_write(u8 dev,u8 *data, int length);

#endif	/* _FSL_I2C_H_ */
