/*
 * Copyright (c) 2010-2011, Juniper Networks, Inc.
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

#define I2CS_PARITY_MASK	0x80
#define NBBY				8

int ccb_i2c_init(void);
void ccb_iic_ctlr_reset(void);
void ccb_iic_ctlr_group_select(uint8_t group);
int ccb_iic_write(uint8_t group, uint8_t device, size_t bytes, uint8_t* buf);
int ccb_iic_read(uint8_t group, uint8_t device, size_t bytes, uint8_t* buf);
int ccb_iic_probe(uint8_t group, uint8_t device);
uint8_t ccb_i2cs_set_odd_parity(uint8_t data);
uint8_t enable_mastership(void);
uint32_t ore_present(uint8_t slot_id);
uint8_t scb_slot_id(void);

#endif /*__CCB_IIC_H__*/
