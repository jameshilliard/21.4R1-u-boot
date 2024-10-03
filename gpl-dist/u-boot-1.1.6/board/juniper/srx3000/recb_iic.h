/*
 * Copyright (c) 2007-2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * Copyright 2004 Freescale Semiconductor.
 *
 * (C) Copyright 2002 Scott McNutt <smcnutt@artesyncp.com>
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
 
#ifndef __recb_IIC_H__
#define __recb_IIC_H__
#include <linux/types.h>

extern int  recb_iic_init(void);
extern void recb_iic_ctlr_reset (void);
extern void recb_iic_ctlr_group_select (u_char group);
extern int  recb_iic_write (u_int8_t group, u_int8_t device, size_t bytes, u_int8_t *buf);
extern int  recb_iic_read (u_int8_t group, u_int8_t device, size_t bytes, u_int8_t *buf);
extern u_int8_t recb_i2cs_set_odd_parity(u_int8_t data);
extern int recb_iic_probe (u_int8_t group, u_int8_t device);


#endif /*__recb_IIC_H__*/
