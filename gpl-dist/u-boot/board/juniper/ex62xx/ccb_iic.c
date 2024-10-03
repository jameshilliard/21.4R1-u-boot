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

#include <common.h>
#include <jcb.h>
#include <ccb_iic.h>
#include <ex62xx_common.h>
#include <asm/fsl_i2c.h>

static jcb_fpga_regs_t *global_ccb_p;
static uint8_t g_enable_scb_keep_master_alive;

/*
 * Return pointer to where the CCB FPGA registers are mapped
 */
jcb_fpga_regs_t *ccb_get_regp(void)
{
	return (jcb_fpga_regs_t *)CFG_JCBC_FPGA_BASE;
}

void ccb_iic_ctlr_group_select(uint8_t group)
{
	volatile uint32_t junk;
	jcb_fpga_regs_t *ccb_p = (jcb_fpga_regs_t *)global_ccb_p;

	group &= JCB_I2C_MUX_GRP_MASK;
	ccb_p->jcb_i2cs_mux_ctrl = ((ccb_p->jcb_i2cs_mux_ctrl &
	    JCB_I2C_MUX_SELECT_MASK) | (group << 8));
	udelay(10);
	junk = ccb_p->jcb_fpga_rev;	/* dummy read to push the write */
	udelay(10);
}

int ccb_i2c_init(void)
{
	i2c_set_bus_num(CONFIG_I2C_BUS_MASTER);

	/* Get the base address for jcbc fpga registers*/
	global_ccb_p = ccb_get_regp();
	if (enable_mastership()) {
		request_mastership_status();
	}

	/* deselect mux */
	ccb_iic_ctlr_group_select(JCB_I2C_DESELECT_I2C_MUX);
	return 0;
}

int ccb_iic_read(uint8_t group, uint8_t device,
	size_t bytes, uint8_t* buf)
{
	udelay(100);
	return i2c_read(device, 0, 0, buf, bytes);
}

int ccb_iic_write(uint8_t group, uint8_t device,
	size_t bytes, uint8_t* buf)
{
	i2c_set_bus_num(CONFIG_I2C_BUS_MASTER);
	ccb_iic_ctlr_group_select(group);
	/* delay after selecting the mux */
	udelay(100);
	return i2c_write(device, 0, 0, buf, bytes);
}

int ccb_iic_probe(uint8_t group, uint8_t device)
{
	i2c_set_bus_num(CONFIG_I2C_BUS_MASTER);
	ccb_iic_ctlr_group_select(group);
	return i2c_probe(device);
}

uint8_t ccb_i2cs_set_odd_parity(uint8_t data)
{
	int i;
	uint8_t parity;

	for (i = 0, parity = I2CS_PARITY_MASK; i < NBBY; i++) {
		if (data & (1 << i)) {
			parity ^=  I2CS_PARITY_MASK;
		}
	}
	data ^= parity;
	return data;
}

uint32_t ore_present(uint8_t slot_id)
{
	uint32_t re0_p, re1_p, ore_p = 0;
	jcb_fpga_regs_t *ccb_p = (jcb_fpga_regs_t *)global_ccb_p;

	re0_p = (ccb_p->jcb_ch_presence & (1 << 17));
	re1_p = (ccb_p->jcb_ch_presence & (1 << 18));

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

uint8_t enable_mastership(void)
{
	uint8_t slot_id, en_mstr = 1;
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

void get_jcbc_rev(uint8_t *maj, uint8_t *min)
{
	jcb_fpga_regs_t *p_ccb_regs;
	uint32_t revision;

	p_ccb_regs = (jcb_fpga_regs_t *)global_ccb_p;
	revision = p_ccb_regs->jcb_fpga_rev;
	*min = (revision & 0xff);
	*maj = (revision >> 8) & 0xff;
}

/*
 * Reset backplane renesas chip
 */
void reset_bp_renesas(void)
{
	uint32_t val;
	jcb_fpga_regs_t *ccb_p = (jcb_fpga_regs_t *)global_ccb_p;

	/* keep the device in reset */
	ccb_p->jcb_device_ctrl &= ~(JCB_RST_CNTL_SECURITY);
	udelay(500);
	/* Bring the security chip out of reset */
	ccb_p->jcb_device_ctrl |= JCB_RST_CNTL_SECURITY;
	/* 250ms delay after renesas reset */
	udelay(250000);
}

void keep_alive_master(void)
{
	jcb_fpga_regs_t* p_ccb_regs;

	/* if this board is already a master, keep it alive */
	if (g_enable_scb_keep_master_alive) {
		p_ccb_regs = (jcb_fpga_regs_t *)ccb_get_regp();
		if (p_ccb_regs == (jcb_fpga_regs_t *)0) {
			printf("JCBC fpga not found, disabling keep alive timer \n");
			g_enable_scb_keep_master_alive = 0;
			return;
		}
		if (p_ccb_regs->jcb_mstr_cnfg & JCB_MCONF_IM_MASTER) {
			/* do this only if this board is already a master */
			p_ccb_regs->jcb_mstr_alive = 1;
			/* 1us delay reqd. after fpga write */
			udelay(1);
		}
	}
}

void show_mastership_status(void)
{
	static jcb_fpga_regs_t* p_ccb_regs;
	uint32_t slot_id;

	p_ccb_regs = (jcb_fpga_regs_t*)ccb_get_regp();

	slot_id = scb_slot_id(); 

	printf("SCB-%d Mastership status: \n", slot_id);

	if (p_ccb_regs->jcb_mstr_cnfg & JCB_MCONF_IM_MASTER) {
		printf("  This SCB[%d] is master \n", slot_id);
	}
	if (p_ccb_regs->jcb_mstr_cnfg & JCB_MCONF_HE_MASTER) {
		printf("  Other SCB is master \n");
	}
	if (p_ccb_regs->jcb_mstr_cnfg & JCB_MCONF_IM_RDY) {
		printf("  This SCB[%d] is master ready\n", slot_id);
	}
	if (p_ccb_regs->jcb_mstr_cnfg & JCB_MCONF_HE_RDY) {
		printf("  Other SCB is master ready \n");
	}
	if (p_ccb_regs->jcb_mstr_cnfg & JCB_MCONF_BOOTED) {
		printf("  SCB[%d] Booted\n", slot_id);
	}
	if (p_ccb_regs->jcb_mstr_cnfg & JCB_MCONF_RELINQUISH) {
		printf("  SCB[%d] Mastership Relinquished\n", slot_id);
	}
}

void request_mastership_status(void)
{
	jcb_fpga_regs_t* p_ccb_regs;

	p_ccb_regs = (jcb_fpga_regs_t *)ccb_get_regp();

	p_ccb_regs->jcb_mstr_alive_cnt = JCB_MSTR_ALIVE_CNT;
	p_ccb_regs->jcb_mstr_alive = 1;
	p_ccb_regs->jcb_mstr_cnfg = JCB_MCONF_BOOTED;
	g_enable_scb_keep_master_alive = 1;
}

void release_mastership_status(void)
{
	jcb_fpga_regs_t* p_ccb_regs;
	uint32_t value;

	p_ccb_regs = (jcb_fpga_regs_t*)ccb_get_regp();

	value = p_ccb_regs->jcb_mstr_cnfg;
	value = (value & ~JCB_MCONF_RELINQUISH) | JCB_MCONF_RELINQUISH;
	p_ccb_regs->jcb_mstr_cnfg = value;

	g_enable_scb_keep_master_alive = 0;
}

uint8_t scb_slot_id(void)
{
	jcb_fpga_regs_t* p_ccb_regs;;
	uint8_t slot_id;

	p_ccb_regs = (jcb_fpga_regs_t*)ccb_get_regp();
	slot_id = (p_ccb_regs->jcb_slot_presence & 0x1) ? 1 : 0;

	return slot_id;
}

int i2cs_write(uint8_t group, uint8_t device, uint addr, int alen,
	       uint8_t *data, int dlen)
{
	i2c_set_bus_num(CONFIG_I2C_BUS_MASTER);
	/* select group */
	ccb_iic_ctlr_group_select(group);
	/* delay after selecting the mux */
	udelay(50);
	return i2c_write(device, addr, alen, data, dlen);
}

int i2cs_read(uint8_t group, uint8_t device, uint addr, int alen,
	       uint8_t *data, int dlen)
{
	i2c_set_bus_num(CONFIG_I2C_BUS_MASTER);
	/* select group */
	ccb_iic_ctlr_group_select(group);
	/* delay after selecting the mux */
	udelay(50);
	return i2c_read(device, addr, alen, data, dlen);
}

/* end of module */
