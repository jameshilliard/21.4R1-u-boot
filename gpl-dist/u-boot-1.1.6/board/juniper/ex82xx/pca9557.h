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

#ifndef __PCA9557_H__
#define __PCA9557_H__

#include <linux/types.h>

#define PCA9557_INPUT_PORT_REG      0x00
#define PCA9557_OUTPUT_PORT_REG     0x01
#define PCA9557_POL_INV_REG         0x02
#define PCA9557_CONFIG_REG          0x03
#define PCA9557_CONTROL_REG_MASK    0x03


int pca9557_set_output_port_map (u_int8_t group, u_int8_t device, u_int8_t port_map);
int pca9557_set_pol_inv_map (u_int8_t group, u_int8_t device, 
                                                        u_int8_t pol_map);
int pca9557_set_config_reg_map (u_int8_t group, u_int8_t device, 
                                                        u_int8_t config_map);
int pca9557_get_input_port_map (u_int8_t group, u_int8_t device, 
                                                        u_int8_t* port_map);
int pca9557_get_output_port_map (u_int8_t group, u_int8_t device, 
                                                        u_int8_t* port_map);
int pca9557_get_config_reg (u_int8_t group, u_int8_t device, 
                                                        u_int8_t* config_reg);
int pca9557_set_output_port (u_int8_t group, u_int8_t device, 
                                                u_int8_t port, unsigned char onoff);
int pca9557_get_input_port (u_int8_t group, u_int8_t device, 
                                            u_int8_t port, u_int8_t* port_val);
int pca9557_get_pol_inv_reg (u_int8_t group, u_int8_t device, 
                                                        u_int8_t* pol_inv_reg);
int pca9557_init (u_int8_t group, u_int8_t device, u_int8_t config_reg,
                              u_int8_t io_data, u_int8_t pol_inv);
#endif /* __PCA9557_H_ */
