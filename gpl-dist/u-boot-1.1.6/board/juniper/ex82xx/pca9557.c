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

#include <common.h>

#ifdef CONFIG_PCA9557
#include "pca9557.h"
#ifdef CONFIG_EX82XX
#include "ex82xx_i2c.h"
#include "ex82xx_common.h"
#endif

int pca9557_set_output_port_map(u_int8_t group,
								u_int8_t device,
								u_int8_t port_map)
{
	u_int8_t i2c_bytes[2];
	int ret = 0;

	i2c_bytes[0] = PCA9557_OUTPUT_PORT_REG;
	i2c_bytes[1] = port_map;

	ret = caffeine_i2c_write(group, device, 2, i2c_bytes);

	if (ret != 0) {
		printf("Failed to set PCA9557[0x%x]->op_port_map(%d)\n",
			   device, port_map);
	}

	return ret;
}


int pca9557_set_pol_inv_map(u_int8_t group, u_int8_t device, u_int8_t pol_map)
{
	int ret = 0;
	u_int8_t i2c_bytes[2];

	i2c_bytes[0] = PCA9557_POL_INV_REG;
	i2c_bytes[1] = pol_map;

	ret = caffeine_i2c_write(group, device, 2, i2c_bytes);

	if (ret != 0) {
		printf("Failed to set PCA9557[0x%x]->pol_inv_map(%d)\n",
			   device, pol_map);
	}

	return ret;
}


int pca9557_set_config_reg_map(u_int8_t group,
							   u_int8_t device,
							   u_int8_t config_map)
{
	int ret = 0;
	u_int8_t i2c_bytes[2];

	i2c_bytes[0] = PCA9557_CONFIG_REG;
	i2c_bytes[1] = config_map;

	ret = caffeine_i2c_write(group, device, 2, i2c_bytes);
	if (ret != 0) {
		printf("Failed to set PCA9557[0x%x]->io_config_reg(%d)\n",
			   device, config_map);
	}

	return ret;
}


int pca9557_get_input_port_map(u_int8_t group,
							   u_int8_t device,
							   u_int8_t *port_map)
{
	int ret = 0;
	u_int8_t i2c_bytes[2];

	i2c_bytes[0] = PCA9557_INPUT_PORT_REG;

	ret = caffeine_i2c_write(group, device, 1, i2c_bytes);
	if (ret != 0) {
		return ret;
	}

	ret = caffeine_i2c_read(group, device, 1, port_map);
	if (ret != 0) {
		printf("Failed to get PCA9557[0x%x]->ip_port_reg\n",
			   device);
	}

	return ret;
}


int pca9557_get_output_port_map(u_int8_t group,
								u_int8_t device,
								u_int8_t *port_map)
{
	int ret = 0;
	u_int8_t i2c_bytes[2];

	i2c_bytes[0] = PCA9557_OUTPUT_PORT_REG;

	ret = caffeine_i2c_write(group, device, 1, i2c_bytes);
	if (ret != 0) {
		return ret;
	}

	ret = caffeine_i2c_read(group, device, 1, port_map);

	if (ret != 0) {
		printf("Failed to get PCA9557[0x%x]->op_port_reg\n",
			   device);
	}

	return ret;
}


int pca9557_get_config_reg(u_int8_t group,
						   u_int8_t device,
						   u_int8_t *config_reg)
{
	int ret = 0;
	u_int8_t i2c_bytes[2];

	i2c_bytes[0] = PCA9557_CONFIG_REG;

	ret = caffeine_i2c_write(group, device, 1, i2c_bytes);
	if (ret != 0) {
		return ret;
	}

	ret = caffeine_i2c_read(group, device, 1, config_reg);

	if (ret != 0) {
		printf("Failed to get PCA9557[0x%x]->config_reg \n",
			   device);
	}

	return ret;
}


int pca9557_get_pol_inv_reg(u_int8_t group,
							u_int8_t device,
							u_int8_t *pol_inv_reg)
{
	int ret = 0;
	u_int8_t i2c_bytes[2];

	i2c_bytes[0] = PCA9557_POL_INV_REG;

	ret = caffeine_i2c_write(group, device, 1, i2c_bytes);
	if (ret != 0) {
		return ret;
	}


	ret = caffeine_i2c_read(group, device, 1, pol_inv_reg);

	if (ret != 0) {
		printf("Failed to get PCA9557[0x%x]->pol_inv_reg \n",
			   device);
	}

	return ret;
}


int pca9557_set_output_port(u_int8_t group,
							u_int8_t device,
							u_int8_t port,
							boolean onoff)
{
	int ret = 0;
	u_int8_t io_data;

	ret = pca9557_get_output_port_map(group, device, &io_data);

	if (onoff) {
		io_data |= (1 << port);
	} else {
		io_data &= ~(1 << port);
	}

	ret = pca9557_set_output_port_map(group, device, io_data);

	return ret;
}


int pca9557_get_input_port(u_int8_t group,
						   u_int8_t device,
						   u_int8_t port,
						   u_int8_t *port_val)
{
	int ret = 0;
	u_int8_t port_map;
	u_int8_t i2c_bytes[2];

	i2c_bytes[0] = PCA9557_INPUT_PORT_REG;

	ret = caffeine_i2c_write(group, device, 1, i2c_bytes);
	if (ret != 0) {
		return ret;
	}

	ret = caffeine_i2c_read(group, device, 1, &port_map);

	if (ret != 0) {
		printf("Failed to get PCA9557[0x%x]->ip_port_reg\n",
			   device);
		return ret;
	}

	*port_val = ((port_map >> port) & 0x01);

	return 0;
}


int pca9557_init(u_int8_t group, u_int8_t device, u_int8_t config_reg,
				 u_int8_t io_data, u_int8_t pol_inv)
{
	int ret = 0;

	/*
	 * set polarity inversion
	 */
	ret = pca9557_set_pol_inv_map(group, device, pol_inv);

	/*
	 * set default output port value
	 */
	ret = pca9557_set_output_port_map(group, device, io_data);

	/*
	 * configure port as input or output
	 */
	ret = pca9557_set_config_reg_map(group, device, config_reg);

	return ret;
}

#endif /*CONFIG_PCA9557*/
