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
#include "lc_cpld.h"
#include <pca9557.h>
#include "ex82xx_common.h"

#undef debug
#define EX82XX_DEBUG
#ifdef  EX82XX_DEBUG
#define debug(fmt, args ...)      printf(fmt, ## args)
#else
#define debug(fmt, args ...)
#endif

/* This function checks whether the register read falls within the range.
 * It returns 0 on success, 1 on failure
 */

int lc_cpld_valid_reg_read (int reg)
{
	if (reg >= LC_CPLD_FIRST_REG && reg <= LC_CPLD_LAST_REG) {
		return 0;
	} else{
		return 1;
	}
}

/* This function checks whether the LC CPLD register written is a valid one.
 * It returns 0 on success, 1 on failure
 */

int lc_cpld_valid_reg_write (int reg)
{
	if ((reg >= LC_CPLD_FIRST_REG) &&
		(reg <= LC_CPLD_LAST_REG) &&
		(reg != LC_CPLD_REVISION) &&
		(reg != LC_CPLD_STATUS)) {
		return 0;
	} else{
		return 1;
	}
}

/* This function reads the specified LC CPLD register and stored it in a
 * variable (second function parameter). It returns 0 on success, 1 on failure.
 */

int lc_cpld_reg_read (int reg, uint8_t* val)
{
	volatile uint8_t* lcpld = (uint8_t*)CFG_CPLD_BASE;

	if (!lc_cpld_valid_reg_read(reg)) {
		*val = lcpld[reg];
		return 0;
	} else {
		return 1;
	}
}

/* This function writes the value passed as second parameter to the LC CPLD
 * register. It returns 0 on success, 1 on failure.
 */

int lc_cpld_reg_write (int reg, uint8_t val)
{
	volatile uint8_t* lcpld = (uint8_t*)CFG_CPLD_BASE;

	if (!lc_cpld_valid_reg_write(reg)) {
		lcpld[reg] = val;
		return 0;
	} else {
		return 1;
	}
}

/* This function can be used to hard reset VCPU */

int lc_cpld_vcpu_hard_reset (void)
{
	return lc_cpld_reg_write(LC_CPLD_VCPU_RESET_CTRL, LC_CPLD_VCPU_HRST);
}

/* This function can be used to soft reset VCPU */

int lc_cpld_vcpu_soft_reset (void)
{
	return lc_cpld_reg_write(LC_CPLD_VCPU_RESET_CTRL, LC_CPLD_VCPU_SRST);
}

static int
poll_db_cpld_access (void)
{
	int timeout = 3;
	volatile unsigned char *db_cpld_ctrl;

	db_cpld_ctrl = (volatile unsigned char *)(CFG_CTRL_CPLD_BASE +
		LC_CTRL_DB_CPLD_CTRL_REG);
	while ((*db_cpld_ctrl & DB_CPLD_RW_BUSY)  && timeout--){
		udelay(1000);
	}
	if (!timeout) {
		debug ("DB CPLD error\n");
		return EFAIL;
	}

	return EOK;
}

static void
lc_ctrl_cpld_reset (void)
{
	u_int8_t group, device, port, config_map, data;

	group = 0x00;

	caffeine_i2c_reset_ctrlr(group);
	udelay(100);
	data = 0x08; /*I2C SWitch channel 3*/
	device = CFG_I2C_CTRL_2_SW_ADDR;
	caffeine_i2c_write(group, device, 1, &data);

	device = 0x18;
	pca9557_get_config_reg(group, device, &config_map);
	udelay(100);
	config_map &= 0xFE;
	pca9557_set_config_reg_map(group, device, config_map);
	udelay(100);

	port = 0;
	pca9557_set_output_port(group, device, port, 0);
	udelay(1000 * 500);
	pca9557_set_output_port(group, device, port, 1);
	udelay(10000);

	*(unsigned char*)(CFG_CPLD_BASE + 0x01) = 0x80;
	udelay(100);
	*(unsigned char*)(CFG_CPLD_BASE + 0x01) = 0x00;
	udelay(2000);
}

/*
 * When the LC-8X10 is powered ON, the LCPU boots up and bring up all the devices
 * on the board out of reset in the sequence described below.
 *
 * Initiate Sentinels Power Up sequence by setting CTRL CPLD Reg 0x2F bit 0.
 * This bit will be self cleared after the reset sequence.
 * De-assert Resets to PHY[3-0] by writing CTRL CPLD Reg 0x34 = 0x00
 * De-assert TCAM[3:0] System Resets by writing CTRL CPLD Reg 0x37 = 0x00
 * De-assert Resets to PEXSW[1:0], ETHSW[1:0] by writing CTRL CPLD Reg 0x3D=0x00
 * De-assert PFE[3-0] SRAM DLL OFF signals by writing CTRL CPLD Reg 0x38 = 0x00
 * Wait 100 micro-seconds
 * De-assert PFE[3-0] Resets by writing CTRL CPLD Reg 0x35 = 0x00
 * Wait 3 milli-seconds
 * De-assert TCAM[3 :0] Core Resets by writing CTRL CPLD Reg 0x36 = 0x00
 */
static void 
lc_cpld_reset_sequence (unsigned short board_id)
{
	/* handle ctrl cpld reset via expander cpld. */	
	lc_ctrl_cpld_reset();

	debug("LC Ctrl CPLD init:\n");
	if ((board_id == EX82XX_LC_48F_BRD_ID) || 
	    (board_id == EX82XX_LC_48F_NM_BRD_ID) || 
	     board_id == EX82XX_LC_48F_ES_BRD_ID) {
		debug("LC_CTRL_EXP_CPLD_RESET = 0x00\n");
		*(unsigned char*)(CFG_CTRL_CPLD_BASE +
			LC_CTRL_EXP_CPLD_RESET) = 0x00;

		/* Switch OFF Link/Activity LEDs */
		*(uint8_t*)(CFG_CTRL_CPLD_BASE + LC_CTRL_SFP_LINK_ACT_LED_GLOBAL) =
			CTRL_CPLD_LINK_ACT_LED_SW_OVRRIDE;
	}

	if (board_id == EX82XX_LC_36XS_BRD_ID) {
		debug("Lion reset...\n");
		*(unsigned char*)(CFG_CTRL_CPLD_BASE +
			LC_CTRL_LION_DEV_RESET) = 0x00;
	}

	debug("LC_CTRL_TCAM_SRST = 0x00\n");
	*(unsigned char*)(CFG_CTRL_CPLD_BASE + LC_CTRL_TCAM_SRST) = 0x00;

	/* Bring PEX switches out of reset. Keep ETH swithces in reset */
	*(uint8_t*)(CFG_CTRL_CPLD_BASE + LC_CTRL_SWITCH_SRST) =
		(ETH_SWITCH_1_RESET | ETH_SWITCH_0_RESET);

	debug("LC_CTRL_PFE_GLBL_DLL_OFF = 0x00\n");
	*(unsigned char*)(CFG_CTRL_CPLD_BASE +
		LC_CTRL_PFE_GLBL_DLL_OFF) = 0x00;

	debug("LC_CTRL_PFE_GLBL_DLL_OFF+1 = 0x00\n");
	*(unsigned char*)(CFG_CTRL_CPLD_BASE +
		LC_CTRL_PFE_GLBL_DLL_OFF + 1) = 0x00;

	debug("LC_CTRL_PFE_GLBL_DLL_OFF+2 = 0x00\n");
	*(unsigned char*)(CFG_CTRL_CPLD_BASE +
		LC_CTRL_PFE_GLBL_DLL_OFF + 2) = 0x00;
	udelay(100);

	debug("LC_CTRL_PFE_RST = 0x00\n");
	*(unsigned char*)(CFG_CTRL_CPLD_BASE + LC_CTRL_PFE_RST) = 0x00;
	udelay(1000 * 3);

	debug("LC_CTRL_TCAM_CRST = 0x00\n");
	*(unsigned char*)(CFG_CTRL_CPLD_BASE + LC_CTRL_TCAM_CRST) = 0x00;
	debug("Done.\n");
}

static void 
lc_cpld_40xs_reset_sequence (unsigned short board_id)
{
	/* handle ctrl cpld reset via expander cpld. */
	lc_ctrl_cpld_reset();

	debug("LC_CTRL_TCAM_SRST = 0x00\n");
	*(unsigned char*)(CFG_CTRL_CPLD_BASE + LC_CTRL_TCAM_SRST) = 0x00;
	if (!poll_db_cpld_access()) {
		*(unsigned char*)(CFG_CTRL_CPLD_BASE +
			LC_CTRL_DB_40XS_TCAM_SRST) = 0x00;
	}

	/* Bring PEX switches out of reset. Keep ETH swithces in reset */
	*(uint8_t*)(CFG_CTRL_CPLD_BASE + LC_CTRL_SWITCH_SRST) =
		(ETH_SWITCH_1_RESET | ETH_SWITCH_0_RESET);
	udelay(100 * 1000);

	debug("LC_CTRL_PFE_GLBL_DLL_OFF = 0x00\n");
	*(unsigned char*)(CFG_CTRL_CPLD_BASE +
		LC_CTRL_PFE_GLBL_DLL_OFF) = 0x00;
	if (!poll_db_cpld_access()) {
		*(unsigned char*)(CFG_CTRL_CPLD_BASE +
			LC_CTRL_DB_40XS_PFE_DLL_OFF) = 0x00;
	}

	debug("LC_CTRL_PFE_RST = 0x00\n");
	*(unsigned char*)(CFG_CTRL_CPLD_BASE + LC_CTRL_PFE_RST) = 0x00;

	debug("Lion reset...\n");
	if (!poll_db_cpld_access()) {
		*(unsigned char*)(CFG_CTRL_CPLD_BASE +
			LC_CTRL_DB_40XS_PFE_LION_RESET) = 0x00;
	}
	udelay(1000 * 3);

	debug("LC_CTRL_TCAM_CRST = 0x00\n");
	*(unsigned char*)(CFG_CTRL_CPLD_BASE + LC_CTRL_TCAM_CRST) = 0x00;
	if (!poll_db_cpld_access()) {
		*(unsigned char*)(CFG_CTRL_CPLD_BASE +
			LC_CTRL_DB_40XS_TCAM_CRST) = 0x00;
	}

	debug("Done.\n");
}

/*
 * When the 48p and xs_44ge card is powered on, the LCPU
 * boots up and brings the devices out of reset (by writing
 * into corresponding registers of DCAP FPGA)  in the
 * following sequence.
 * PCI Express switch is brought out of reset.
 * Sentinel Reset Sequence.
 * De-assert TCAM_SRST Resets. Wait for 100 ms.
 * De-assert PFE_DLL_OFF signals.
 * De-assert PFE (PUMA-J) and CHEETAH-J resets. Wait for 3ms.
 * De-assert TCAM_CRST Resets. Follow the following steps to
 * bring TCAM to normal operation:
 *  - after TCAM_CRST is de-asserted, wait for at least 3 ms.
 *  - send exactly 100 consecutive NOP instrns to TCAM.
 *  - send at least 100 interface IDLE clock cycles to TCAM.
 * The below three steps are not done at u-boot, but done at a
 * later stage.
 * De-assert BCM8727 PHY Resets. 
 * De-assert quad PHY resets.
 * De-assert POE controller asserts.
 */
static void
lc_cpld_2xs_44ge_48p_reset_sequence(unsigned short board_id)
{
	unsigned char btcpld_reg;

	debug("PEX Switch reset...\n");
	btcpld_reg = *(unsigned char*)(CFG_CPLD_BASE + 0x01);
	btcpld_reg |= 0x80;
	udelay(1000);
	btcpld_reg |= ~0x80;

	debug("LC_CTRL_TCAM_SRST (0x37) = 0x00\n");
	*(unsigned char*)(CFG_CTRL_CPLD_BASE + LC_CTRL_TCAM_SRST) = 0x00;
	udelay(100*1000);

	debug("LC_CTRL_DLL_OFF (0x39) = 0x00\n");
	*(unsigned char*)(CFG_CTRL_CPLD_BASE +
		LC_CTRL_2XS_44GE_48P_DLL_OFF) = 0x00;

	debug("Puma reset...\n");
	debug("LC_CTRL_PFE_RST (0x35) = 0x00\n");
	*(unsigned char*)(CFG_CTRL_CPLD_BASE + LC_CTRL_PFE_RST) = 0x00;

	debug("Cheetah reset...\n");
	debug("LC_CTRL_PFE_CJ_RST (0x3C) = 0x00\n");
	*(unsigned char*)(CFG_CTRL_CPLD_BASE +
		LC_CTRL_2XS_44GE_48P_PFE_CJ_RST) = 0x00;
	udelay(3*1000);

	debug("LC_CTRL_TCAM_CRST (0x36) = 0x00\n");
	*(unsigned char*)(CFG_CTRL_CPLD_BASE + LC_CTRL_TCAM_CRST) = 0x00;
	udelay(100*1000);

	debug("LC_CTRL_TCAM_HSIM (0x3B) = 0x00\n");
	*(unsigned char*)(CFG_CTRL_CPLD_BASE + LC_CTRL_TCAM_HSIM) = 0x00;
	udelay(100*1000);

	debug("Done.\n");
}

int do_lcpu_ctrl_cpld_reset_sequence (unsigned short board_id)
{
    switch (board_id) {
    case EX82XX_LC_48F_BRD_ID:
    case EX82XX_LC_48F_NM_BRD_ID:
    case EX82XX_LC_48F_ES_BRD_ID:
    case EX82XX_LC_48T_BRD_ID:
    case EX82XX_LC_48T_NM_BRD_ID:
    case EX82XX_LC_48T_ES_BRD_ID:
    case EX82XX_LC_8XS_BRD_ID:       
    case EX82XX_LC_8XS_NM_BRD_ID:       
    case EX82XX_LC_8XS_ES_BRD_ID:
    case EX82XX_LC_36XS_BRD_ID:            
        lc_cpld_reset_sequence(board_id);
        break;

    case EX82XX_LC_40XS_BRD_ID:
    case EX82XX_LC_40XS_NM_BRD_ID:
    case EX82XX_LC_40XS_ES_BRD_ID:
        lc_cpld_40xs_reset_sequence(board_id);
        break;            

    case EX82XX_LC_48P_BRD_ID:
    case EX82XX_LC_48TE_BRD_ID:    
    case EX82XX_LC_2XS_44GE_BRD_ID:
    case EX82XX_LC_2X4F40TE_BRD_ID:	
        lc_cpld_2xs_44ge_48p_reset_sequence(board_id);
        break;
    default:
        break;
    }
    return 0;
}

