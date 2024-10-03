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

#ifndef __JCB_H__
#define __JCB_H__

#include <linux/types.h>

typedef struct jcb_fpga_regs_ {
	volatile u32 jcb_int_src;		/* 0x00, JSCB_CTL_INT_SRC1/SRC0 */
	volatile u32 jcb_int_en;		/* 0x04, JSCB_CTL_INT_SRC1_EN/SRC0_EN */
	volatile u32 jcb_i2cs_mux_ctrl;	/* 0x08, JSCB_CTL_I2C_MSTR_CTRL */
	volatile u32 jcb_timer_cnt;		/* 0x0c, JSCB_CTL_TIMER_CTRL */
	volatile u32 jcb_slot_presence;	/* 0x10, JSCB_CTL_BP_INFO */
	volatile u32 jcb_ch_presence;	/* 0x14, JSCB_CTL_FRU_PRSNT1/PRSNT0 */
	volatile u32 jcb_soft_reset;	/* 0x18, JSCB_CTL_SOFT_RST */
	volatile u32 reserved_1;		/* 0x1c, UNUSED */
	volatile u32 jcb_i2cs_int;		/* 0x20, JSCB_CTL_I2CS_INT1/INT0 */
	volatile u32 jcb_i2cs_int_en;	/* 0x24, JSCB_CTL_I2CS_INT1_EN/INT0_EN */
	volatile u32 reserved_2[2];		/* 0x28-0x2c, UNUSED */
	volatile u32 jcb_fan_ctrl_st;	/* 0x30, JSCB_CTL_FAN_CTRL_ST1/ST0 */
	volatile u32 reserved_3[2];		/* 0x34-0x38, UNUSED */
	volatile u32 jcb_mstr_cnfg;		/* 0x3c, JSCB_CTL_MSTR_CONFIG */
	volatile u32 jcb_mstr_alive;	/* 0x40, JSCB_CTL_MSTRL_ALIVE */
	volatile u32 jcb_mstr_alive_cnt;/* 0x44, JSCB_CTL_MSTR_ALIVE_CNT */
	volatile u32 reserved_6[6];		/* 0x48-0x5c, UNUSED */
	volatile u32 jcb_fpga_rev;		/* 0x60, JSCB_CTL_FPGA_REV */
	volatile u32 jcb_fpga_date;		/* 0x64, JSCB_CTL_FPGA_DATE1/DATE0 */
	volatile u32 reserved_7[11];	/* 0x68-0x90, UNUSED */
	volatile u32 jcb_device_ctrl;	/* 0x94, JSCB_CTL_RST_CNTL */
	volatile u32 jcb_scratch_reg;	/* 0x98, JSCB_CTL_SCRATCH */
	volatile u32 jcb_lcd_cntl;		/* 0x9c, JSCB_CTL_LCD_CNTL */
	volatile u32 jcb_phy_cntl;		/* 0xa0, JSCB_CTL_PHY_CNTL/LMAC_LED */
} jcb_fpga_regs_t;


/* JSCB_CTL_I2C_MSTR_CTRL */
#define JCB_I2C_MUX_SELECT_MASK		(0xe0ff)
#define JCB_I2C_MUX_GRP_MASK		(0x1f)	/* 5'b[12:8] */
#define JCB_I2C_DESELECT_I2C_MUX	(0x1f)

/*
 * definitions for mastership config
 */
#define JCB_MCONF_BOOTED			(1 << 0)
#define JCB_MCONF_RELINQUISH		(1 << 1)
#define JCB_MCONF_IM_RDY			(1 << 3)
#define JCB_MCONF_IM_MASTER			(1 << 4)
#define JCB_MCONF_HE_RDY			(1 << 5)
#define JCB_MCONF_HE_MASTER			(1 << 6)

#define JCB_MSTR_ALIVE_CNT_MSK		(0x7F)
#define JCB_MSTR_ALIVE_CNT_OPER_SIG	(1 << 8)

#define JCB_MSTR_ALIVE_CNT			0x105

/*
 * definitions for reset cntrl 0x94
 */
#define JCB_RST_CNTL_PLL			(1 << 0)
#define JCB_RST_CNTL_USB_HUB		(1 << 1)
#define JCB_RST_CNTL_USB_PHY		(1 << 2)
#define JCB_RST_CNTL_USB_NAND		(1 << 3)
#define JCB_RST_CNTL_DUART			(1 << 4)
#define JCB_RST_CNTL_MGMT_PHY		(1 << 5)
#define JCB_RST_CNTL_ETH_SW			(1 << 10)
#define JCB_RST_CNTL_10G_PHY		(1 << 11)
#define JCB_RST_CNTL_SECURITY		(1 << 12)
#define JCB_RST_CNTL_LMAC			(1 << 13)

#endif /* __JCB_H_ */
