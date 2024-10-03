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
#include <i2c.h>
#include "fsl_i2c.h"
#include "ccb_iic.h"
#include "ex82xx_i2c.h"
#include <ex82xx_devices.h>


void
caffeine_i2c_sel_ctrlr(uint8_t group)
{
	switch (CAFF_I2C_GET_IF(group)) {
	case CAFF_I2C_IF_INT_I2C0:
		i2c_controller(CFG_I2C_CTRL_1);
		break;
	case CAFF_I2C_IF_INT_I2C1:
		i2c_controller(CFG_I2C_CTRL_2);
		break;
	case CAFF_I2C_IF_EXT_SCB:
		i2c_controller(CFG_I2C_CTRL_2);
		break;
	default:
		printf(
			"caffeine_sel_ctrlr : operation not supported for this group %d \n",
			group);
		break;
	}
}


void 
caffeine_i2c_reset_ctrlr(uint8_t group)
{
	switch (CAFF_I2C_GET_IF(group)) {
	case CAFF_I2C_IF_INT_I2C0:
		i2c_controller(CFG_I2C_CTRL_1);
		i2c_reset();
		break;
	case CAFF_I2C_IF_INT_I2C1:
		i2c_controller(CFG_I2C_CTRL_2);
		i2c_reset();
		break;
	case CAFF_I2C_IF_EXT_SCB:
		i2c_controller(CFG_I2C_CTRL_2);
		i2c_reset();
		break;
	case CAFF_I2C_IF_EXT_CCBC:
		ccb_iic_init();
		break;
	default:
		printf(
			"caffeine_reset_ctrlr : operation not supported for this group %d \n",
			group);
		break;
	}
}


int
caffeine_i2c_probe(uint8_t group, uint8_t device)
{
	int ret  = (-1);

	switch (CAFF_I2C_GET_IF(group)) {
	case CAFF_I2C_IF_INT_I2C0:
		i2c_controller(CFG_I2C_CTRL_1);
		ret = i2c_probe(device);
		break;
	case CAFF_I2C_IF_INT_I2C1:
		i2c_controller(CFG_I2C_CTRL_2);
		ret = i2c_probe(device);
		break;
	case CAFF_I2C_IF_EXT_SCB:
		i2c_controller(CFG_I2C_CTRL_2);
		ccb_iic_ctlr_group_select(CAFF_I2C_GET_GRP(group));
		ret = i2c_probe(device);
		break;
	case CAFF_I2C_IF_EXT_CCBC:
		ret = ccb_iic_probe(CAFF_I2C_GET_GRP(group), device);
		break;
	default:
		printf(
			"caffeine_i2c_probe : operation not supported for this group %d \n",
			group);
		break;
	}
	return ret;
}


int
caffeine_i2c_write(uint8_t group,
						uint8_t device,
						size_t bytes,
						uint8_t* buf)
{
	int ret  = (-1);

	switch (CAFF_I2C_GET_IF(group)) {
	case CAFF_I2C_IF_INT_I2C0:
		i2c_controller(CFG_I2C_CTRL_1);
		ret = i2c_raw_write(device, buf, (int)bytes);
		break;
	case CAFF_I2C_IF_INT_I2C1:
		i2c_controller(CFG_I2C_CTRL_2);
		ret = i2c_raw_write(device, buf, (int)bytes);
		break;
	case CAFF_I2C_IF_EXT_SCB:
		i2c_controller(CFG_I2C_CTRL_2);
		ccb_iic_ctlr_group_select(CAFF_I2C_GET_GRP(group));
		ret = i2c_raw_write(device, buf, (int)bytes);
		break;
	case CAFF_I2C_IF_EXT_CCBC:
		ret = ccb_iic_write(CAFF_I2C_GET_GRP(group), device, bytes, buf);
		break;
	default:
		printf("ccb_i2c_write : operation not supported for this group %d \n",
			   group);
		break;
	}
	return ret;
}


int
caffeine_i2c_read(uint8_t group,
					   uint8_t device,
					   size_t bytes,
					   uint8_t* buf)
{
	int ret  = (-1);

	switch (CAFF_I2C_GET_IF(group)) {
	case CAFF_I2C_IF_INT_I2C0:
		i2c_controller(CFG_I2C_CTRL_1);
		ret = i2c_raw_read(device, buf, (int)bytes);
		break;
	case CAFF_I2C_IF_INT_I2C1:
		i2c_controller(CFG_I2C_CTRL_2);
		ret = i2c_raw_read(device, buf, (int)bytes);
		break;
	case CAFF_I2C_IF_EXT_SCB:
		i2c_controller(CFG_I2C_CTRL_2);
		ccb_iic_ctlr_group_select(CAFF_I2C_GET_GRP(group));
		ret = i2c_raw_read(device, buf, (int)bytes);
		break;
	case CAFF_I2C_IF_EXT_CCBC:
		ret = ccb_iic_read(CAFF_I2C_GET_GRP(group), device, bytes, buf);
		break;
	default:
		printf("ccb_i2c_read : operation not supported for this group %d \n",
			   group);
		break;
	}
	return ret;
}

uint8_t 
my_i2c_groupid(void)
{
	uint16_t assembly_id;
	uint8_t my_group, slot;

	slot = get_slot_id();

	i2c_controller(CFG_I2C_CTRL_1);
	assembly_id = get_assembly_id(EX82XX_LOCAL_ID_EEPROM_ADDR);
	switch (assembly_id) {
	case EX82XX_GRANDE_CHASSIS:
		my_group = CFG_EX8208_SCB_SLOT_BASE + slot;
		break;
	case EX82XX_VENTI_CHASSIS:
		my_group = CFG_EX8216_CB_SLOT_BASE + slot;
		break;
	deafult:
		my_group = 0x00;
	}
	return (my_group);
}

int 
read_i2cs_cpld(uint8_t offset, uint8_t* val)
{
	uint8_t group, device, tdata[1];
	int ret;

	group = my_i2c_groupid();
	device = SCB_SLAVE_CPLD_ADDR;

	tdata[0] = ccb_i2cs_set_odd_parity(offset & 0xFF);
	if ((ret = ccb_iic_write(group, device, 1, tdata)) != 0) {
		printf ("I2CS write to device 0x%x failed.\n", device);
		goto ret_err;
	} else {
		if ((ret = ccb_iic_read(group, device, 1, tdata)) != 0) {
			printf ("I2C read from device 0x%x failed.\n", device);
			goto ret_err;
		} else {
			*val = tdata[0];
		}
	}

	return 0;

ret_err:
	return -1;
}

int 
write_i2cs_cpld(uint8_t offset, uint8_t val)
{
	uint8_t group, device, tdata[2];
	int ret;

	group = my_i2c_groupid();
	device = SCB_SLAVE_CPLD_ADDR;

	tdata[0] = ccb_i2cs_set_odd_parity(offset & 0xFF);
	tdata[1] = ccb_i2cs_set_odd_parity(val);
	if ((ret = ccb_iic_write (group, device, 2, tdata)) != 0) {
		printf ("I2CS write @ 0x%x to scratch pad failed.\n", device);
		goto ret_err;
	}
	return 0;

ret_err:
	return -1;
}

