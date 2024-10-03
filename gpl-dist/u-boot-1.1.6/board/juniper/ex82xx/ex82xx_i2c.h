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

#ifndef __EX82XX_I2C_H__
#define __EX82XX_I2C_H__

/* Interface identifier is used to dispatch to appropriate interfaces */
#define CAFF_I2C_IF_INT_I2C0 0x0    /* Internal freescale i2c controller 0*/
#define CAFF_I2C_IF_INT_I2C1 0x1    /* Internal freescale i2c controller 1*/
#define CAFF_I2C_IF_EXT_CCBC 0x2    /* External i2c controller through ccbc fpga on PGSCB*/
#define CAFF_I2C_IF_EXT_SCB  0x3    /* External i2c controller through SCB fpga on GSCB */

/* I2CS CPLD registers */
#define I2CS_CPLD_I2C_RST_REG    0x07
/* powercycle bit - reg 0x07 */
#define BIT_POWERCYCLE_REG07    (1 << 3)

#define CAFF_I2C_IF_POS    0x5
#define CAFF_I2C_IF_MASK   0x7
#define CAFF_I2C_GRP_MASK  0x1F

#define CAFF_I2C_SET_IF(i, g) (((g) & \
							 CAFF_I2C_GRP_MASK)   |\
							 (((i) & CAFF_I2C_IF_MASK) << CAFF_I2C_IF_POS))
#define CAFF_I2C_GET_IF(g)   ((CAFF_I2C_IF_MASK)&((g) >> CAFF_I2C_IF_POS))
#define CAFF_I2C_GET_GRP(g)  ((CAFF_I2C_GRP_MASK)&(g))

void caffeine_i2c_sel_ctrlr   (u_int8_t group);
void caffeine_i2c_reset_ctrlr (u_int8_t group);
int  caffeine_i2c_probe       (u_int8_t group, u_int8_t device);
int  caffeine_i2c_write (u_int8_t group,
						 u_int8_t device,
						 size_t bytes,
						 u_int8_t* buf);
int  caffeine_i2c_read  (u_int8_t group,
						 u_int8_t device,
						 size_t bytes,
						 u_int8_t* buf);

uint8_t my_i2c_groupid(void);
int read_i2cs_cpld(uint8_t offset, uint8_t* val);
int write_i2cs_cpld(uint8_t offset, uint8_t val);

#endif /*__EX82XX_I2C_H__*/
