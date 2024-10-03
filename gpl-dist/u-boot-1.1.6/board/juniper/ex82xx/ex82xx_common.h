/*
 * Copyright (c) 2008-2011, Juniper Networks, Inc.
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

#ifndef __EX82XX_COMMON_H__
#define __EX82XX_COMMON_H__


typedef int                      status_t;
typedef unsigned char            boolean;

#define TRUE                      1
#define FALSE                     0
#define EOK                       0
#define EINVAL                    (-1)
#define EFAIL                     (-1)

/* This group id indicates direct i2c access to the devices*/
#define I2C_DIR_ACCESS_GROUP  0xFF 

/* externs */
extern int is_2xs_44ge_48p_board(void);
extern int disable_p2020e_core1(void);

#endif /* __EX82XX_COMMON_H__ */
