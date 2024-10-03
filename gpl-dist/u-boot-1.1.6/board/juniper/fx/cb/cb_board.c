/*
 * Copyright (c) 2010, Juniper Networks, Inc.
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


/*************************************************
  CB board specific stuff
  Author : Bilahari Akkiraju
**************************************************/


#include <common/fx_common.h>
#include <common/i2c.h>

uchar cb_num_i2c_devices[ ]  = { 6, 6 };

uchar cb_i2c_bus1_list[ ]   = { 
    0x4c, 0x50, 0x51, 0x52, 0x57, 0x68 
};

/* 
 *  CAT24C208 on midplane
 *  0x50 - segment 0 i2c addr
 *  0x30 - segment register i2c addr
 *  0x31 - configuration register i2c addr
 */ 
uchar cb_i2c_bus2_list[ ]   = {
    0x30, 0x31, 0x38, 0x3e, 0x3f, 0x50
};

/* Board list of i2c devices on each i2c bus */
uchar *cb_board_i2c_devs[MAX_I2C_BUSES] = {
    cb_i2c_bus1_list,
    cb_i2c_bus2_list
};

/* CB board specific functions should be here */

