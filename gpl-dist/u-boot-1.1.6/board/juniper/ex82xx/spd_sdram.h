/*
 * Copyright (c) 2010-2012, Juniper Networks, Inc.
 * All rights reserved.
 *
 * DDR controller register setting definitions for different speeds
 *
 * This software may be used and distributed according to the
 * terms of the GNU Public License, Version 2, incorporated
 * herein by reference.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef _DDR_MEMCTL_H_
#define _DDR_MEMCTL_H_


#define CONFIG_CHIP_SELECTS_PER_CTRL 1

#define DATARATE_400MHZ 400000000
#define DATARATE_533MHZ 533333333
#define DATARATE_667MHZ 666666666
#define DATARATE_800MHZ 800000000
/*
 * Manually set up DDR parameters for LC
 */

#define CFG_SDRAM_SIZE_LCPU		1024		/* DDR is 1024MB */
#define CFG_DDR_CS0_BNDS_LCPU		0x0000003F	/* 0-1024MB */
#define CFG_DDR_CS0_CONFIG_LCPU		0x80014202	/* offset 0x2080 */
#define CFG_DDR_REFREC_LCPU		0x00010000	/* offset 0x2100 */
#define CFG_DDR_TIMING_0_LCPU		0x55220802	/*0x00260802 = old -> offset 0x2104*/
#define CFG_DDR_TIMING_1_LCPU		0x38372322	/*0x39397322 = old -> offset 0x2108*/
#define CFG_DDR_CFG_LCPU		0xC3000000	
#define CFG_DDR_CFG_2_LCPU		0x04401000
#define CFG_DDR_MODE_LCPU		0x00040442  /*0x00400452 = old -> offset 0x2118*/
#define CFG_DDR_MODE_2_LCPU		0x0
#define CFG_DDRCDR_LCPU			0x0
/*400Mhz*/
#define CFG_DDR_TIMING_2_LCPU		0x031848C8 /*0x04A04CC8 = old -> offset 0x210c*/
#define CFG_DDR_INTERVAL_LCPU		0x06090100
#define CFG_DDR_CLK_CTRL_LCPU		0x02000000
#define CFG_DDR_MODE_CONTROL		0x00000000
#define CONFIG_MEM_INIT_VALUE		0x00000000
#define CFG_SYS_DDR_CLK_CTRL		0x00000000
#define CFG_SYS_DDR_ERR_DISABLE		0x0000000d
#define CFG_SYS_DDR_ERR_SBE		0x00ff0000

/* 533Mhz DDR2 data rate settings for extra scale CPU */
/* offset 0x2080 */
#define CFG_DDR_533_CS0_CONFIG_LCPU	    0x80014202
/* offset 0x2100 */
#define CFG_DDR_533_REFREC_LCPU	    0x00010000
/* offset 0x2104 */
#define CFG_DDR_533_TIMING_0_LCPU	    0x55220802
/* 0x38372322 = (CL=4); offset 0x2108 */
#define CFG_DDR_533_TIMING_1_LCPU	    0x38372322
/* offset 0x2110; 0xC3000000 = old (1T); new = 2T */
#define CFG_DDR_533_CFG_LCPU		    0xC3008000
#define CFG_DDR_533_CFG_2_LCPU		    0x04401000
/* 0x00400442(CL=4); offset 0x2118 */
#define CFG_DDR_533_MODE_LCPU		    0x00040442
#define CFG_DDR_533_MODE_2_LCPU	    0x0
#define CFG_DDR_533_DDRCDR_LCPU	    0x0
/* offset 0x210c */
#define CFG_DDR_533_TIMING_2_LCPU	    0x031848C8
#define CFG_DDR_533_INTERVAL_LCPU	    0x06090100
/* offset 0x2130 */
#define CFG_DDR_533_CLK_CTRL_LCPU	    0x02000000
#define CFG_DDR_533_MODE_CONTROL	    0x00000000

typedef struct ddr_cfg_regs_s {
	struct {
	    unsigned int bnds;
	    unsigned int config;
	} cs[CONFIG_CHIP_SELECTS_PER_CTRL];
	unsigned int ext_refrec;
	unsigned int timing_cfg_0;
	unsigned int timing_cfg_1;
	unsigned int timing_cfg_2;
	unsigned int ddr_sdram_cfg;
	unsigned int ddr_sdram_cfg_2;
	unsigned int ddr_sdram_mode;
	unsigned int ddr_sdram_mode_2;
	unsigned int ddr_sdram_md_cntl;
	unsigned int ddr_sdram_interval;
	unsigned int ddr_data_init;
	unsigned int ddr_sdram_clk_cntl;
	unsigned int ddr_err_disable;
	unsigned int ddr_err_sbe;
} ddr_cfg_regs_t;


ddr_cfg_regs_t ddr_cfg_regs_400 = {
	.cs[0].bnds = CFG_DDR_CS0_BNDS_LCPU, 
	.cs[0].config = CFG_DDR_CS0_CONFIG_LCPU,
	.ext_refrec = CFG_DDR_REFREC_LCPU,
	.timing_cfg_0 = CFG_DDR_TIMING_0_LCPU,
	.timing_cfg_1 = CFG_DDR_TIMING_1_LCPU,
	.timing_cfg_2 = CFG_DDR_TIMING_2_LCPU,
	.ddr_sdram_cfg = CFG_DDR_CFG_LCPU, 
	.ddr_sdram_cfg_2 = CFG_DDR_CFG_2_LCPU, 
	.ddr_sdram_mode = CFG_DDR_MODE_LCPU,
	.ddr_sdram_mode_2 = CFG_DDR_MODE_2_LCPU,
	.ddr_sdram_md_cntl = CFG_DDR_MODE_CONTROL, 
	.ddr_sdram_interval = CFG_DDR_INTERVAL_LCPU, 
	.ddr_data_init = CONFIG_MEM_INIT_VALUE,
	.ddr_sdram_clk_cntl = CFG_DDR_CLK_CTRL_LCPU, 
	.ddr_err_disable = CFG_SYS_DDR_ERR_DISABLE,
	.ddr_err_sbe = CFG_SYS_DDR_ERR_SBE
};

ddr_cfg_regs_t ddr_cfg_regs_533 = {
	.cs[0].bnds = CFG_DDR_CS0_BNDS_LCPU,
	.cs[0].config = CFG_DDR_533_CS0_CONFIG_LCPU,
	.ext_refrec = CFG_DDR_533_REFREC_LCPU,
	.timing_cfg_0 = CFG_DDR_533_TIMING_0_LCPU,
	.timing_cfg_1 = CFG_DDR_533_TIMING_1_LCPU,
	.timing_cfg_2 = CFG_DDR_533_TIMING_2_LCPU,
	.ddr_sdram_cfg = CFG_DDR_533_CFG_LCPU,
	.ddr_sdram_cfg_2 = CFG_DDR_533_CFG_2_LCPU,
	.ddr_sdram_mode = CFG_DDR_533_MODE_LCPU,
	.ddr_sdram_mode_2 = CFG_DDR_533_MODE_2_LCPU,
	.ddr_sdram_md_cntl = CFG_DDR_533_MODE_CONTROL,
	.ddr_sdram_interval = CFG_DDR_533_INTERVAL_LCPU,
	.ddr_data_init = CONFIG_MEM_INIT_VALUE,
	.ddr_sdram_clk_cntl = CFG_DDR_533_CLK_CTRL_LCPU,
	.ddr_err_disable = CFG_SYS_DDR_ERR_DISABLE,
	.ddr_err_sbe = CFG_SYS_DDR_ERR_SBE
};
#endif
