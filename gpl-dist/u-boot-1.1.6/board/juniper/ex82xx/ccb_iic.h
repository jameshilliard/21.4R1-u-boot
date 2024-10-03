/*
 * Copyright (c) 2008-2010, Juniper Networks, Inc.
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

#ifndef __CCB_IIC_H__
#define __CCB_IIC_H__
#include <linux/types.h>

extern int  ccb_iic_init(void);
extern void ccb_iic_ctlr_reset (void);
extern void ccb_iic_ctlr_group_select (u_char group);
extern int  ccb_iic_write (u_int8_t group, u_int8_t device, size_t bytes, u_int8_t* buf);
extern int  ccb_iic_read (u_int8_t group, u_int8_t device, size_t bytes, u_int8_t* buf);
extern u_int8_t ccb_i2cs_set_odd_parity(u_int8_t data);
extern int ccb_iic_probe (u_int8_t group, u_int8_t device);

#endif /*__CCB_IIC_H__*/
