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

#include "common.h"
#include "gvcb.h"
#include "ccb_iic.h"
#include <pcie.h>

#define CCB_IIC_LE    1
#ifdef CCB_IIC_LE
#define swap_ulong(x)						\
	({ unsigned long x_ = (unsigned long)x;	\
	   (unsigned long)(						\
		   ((x_ & 0x000000FFUL) << 24) |	\
		   ((x_ & 0x0000FF00UL) <<  8) |	\
		   ((x_ & 0x00FF0000UL) >>  8) |	\
		   ((x_ & 0xFF000000UL) >> 24) );	\
	 })
#else
#define swap_ulong(x)    (x)
#endif


gvcb_fpga_regs_t *ccb_get_regp(void);
extern unsigned char get_slot_id(void);
extern void request_mastership_status(void);
u_int8_t enable_mastership(void);
static gvcb_fpga_regs_t *global_ccb_p = 0;
#define I2CS_PARITY_MASK    0x80
#define NBBY                8

/*
 * Return pointer to where the CCB FPGA registers are mapped
 */
gvcb_fpga_regs_t *ccb_get_regp(void)
{
	int busdevfunc, bdf0, bdf1;
	unsigned int scbc_base_addr;       /* base address */

	bdf0 = pcie_find_device(PCI_JNX_VENDORID, PCI_JNX_DEVICEID_CCB, 0);
	bdf1 = pcie_find_device(PCI_JNX_VENDORID, PCI_JNX_DEVICEID_VCBC, 0);
	if ((bdf0 == -1) && (bdf1 == -1)) {
		printf("Error PCI- SCBC FPGA  (%04X,%04X,%04X) not found\n",
			   PCI_JNX_VENDORID, PCI_JNX_DEVICEID_CCB, PCI_JNX_DEVICEID_VCBC);
		return (gvcb_fpga_regs_t *)0;
	}

	if (bdf0 != -1) {
		busdevfunc = bdf0;
	} else   {
		busdevfunc = bdf1;
	}
	pcie_read_config_dword(busdevfunc, PCIE_BASE_ADDRESS_0, &scbc_base_addr);

	return (gvcb_fpga_regs_t *)scbc_base_addr;
}

void ccb_iic_ctlr_group_select(u_char group)
{
	volatile uint32_t junk;
	gvcb_fpga_regs_t *ccb_p = (gvcb_fpga_regs_t*)global_ccb_p;

	ccb_p->gvcb_i2cs_mux_ctrl =
		swap_ulong((((swap_ulong(ccb_p->gvcb_i2cs_mux_ctrl)) &
					 GVCB_I2C_MUX_SELECT_MASK) | (group << 7)));
	udelay(10);

	junk = swap_ulong(ccb_p->gvcb_fpga_rev);         /* dummy read to push the write */
	udelay(10);
}


int ccb_iic_init(void)
{
	i2c_controller(CFG_I2C_CTRL_2);
	i2c_reset();

	/* Get the base address for scbc fpga registers*/
	global_ccb_p = ccb_get_regp();
	if (enable_mastership()) {
		request_mastership_status();
	}

	return 0;
}


int ccb_iic_read(u_int8_t group, u_int8_t device,
				 size_t bytes, u_int8_t* buf)
{
	i2c_controller(CFG_I2C_CTRL_2);
	ccb_iic_ctlr_group_select(group);
	return i2c_raw_read(device, buf, bytes);
}


int ccb_iic_write(u_int8_t group, u_int8_t device,
				  size_t bytes, u_int8_t* buf)
{
	i2c_controller(CFG_I2C_CTRL_2);
	ccb_iic_ctlr_group_select(group);
	return i2c_raw_write(device, buf, bytes);
}


int ccb_iic_probe(u_int8_t group, u_int8_t device)
{
	i2c_controller(CFG_I2C_CTRL_2);
	ccb_iic_ctlr_group_select(group);
	return i2c_probe(device);
}


u_int8_t ccb_i2cs_set_odd_parity(u_int8_t data)
{
	int i;
	u_int8_t parity;

	for (i = 0, parity = I2CS_PARITY_MASK; i < NBBY; i++) {
		if (data & (1 << i)) {
			parity ^=  I2CS_PARITY_MASK;
		}
	}
	data ^= parity;
	return data;
}

uint32_t ore_present(u_int8_t slot_id)
{
	uint32_t re0_p, re1_p, ore_p = 0;
	gvcb_fpga_regs_t *ccb_p = (gvcb_fpga_regs_t*)global_ccb_p;

	re0_p = (swap_ulong(ccb_p->gvcb_ch_presence) & (1 << 17));
	re1_p = (swap_ulong(ccb_p->gvcb_ch_presence) & (1 << 18));

	if (slot_id) { /*RE1*/
		if (re0_p) {
			ore_p = 1;
		}
	} else {	/*RE0*/
		if (re1_p) {
			ore_p = 1;
		}
	}
	return ore_p;
}
u_int8_t enable_mastership(void)
{
	u_int8_t slot_id, en_mstr = 1;
	char* env;

	env = getenv("get_mastership");
	/* Dont acquire mastership by default */
	if (env == NULL) {
		return 0;
	}

	slot_id = get_slot_id();
	if (slot_id) {	/*RE1*/
		if (ore_present(slot_id)) {
			en_mstr = 0;
		}
	}
	return en_mstr;
}
/* end of module */
