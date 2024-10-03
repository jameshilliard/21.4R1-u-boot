/*
 * $Id$
 *
 * fixed_sdram.c -- ACX DDR controller initialization settings & code
 *
 * Rajat Jain, Feb 2011
 *
 * Copyright (c) 2011-2013, Juniper Networks, Inc.
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
#include <command.h>
#include <pci.h>
#include <asm/processor.h>
#include <asm/mmu.h>
#include <asm/cache.h>
#include <asm/immap_85xx.h>

DECLARE_GLOBAL_DATA_PTR;

typedef struct ddr_settings {
	unsigned int	cs0_bnds;		/* 0x2000 - DDR Chip Select 0 Memory Bounds */
	unsigned int	cs1_bnds;		/* 0x2008 - DDR Chip Select 1 Memory Bounds */
	unsigned int	cs0_config;		/* 0x2080 - DDR Chip Select Configuration */
	unsigned int	cs1_config;		/* 0x2084 - DDR Chip Select Configuration */
	unsigned int	timing_cfg_3;	/* 0x2100 - DDR SDRAM Extended Refresh Recovery */
	unsigned int	timing_cfg_0;   /* 0x2104 - DDR SDRAM Timing Configuration Register 0 */
	unsigned int	timing_cfg_1;	/* 0x2108 - DDR SDRAM Timing Configuration Register 1 */
	unsigned int	timing_cfg_2;	/* 0x210c - DDR SDRAM Timing Configuration Register 2 */
	unsigned int	sdram_cfg;	/* 0x2110 - DDR SDRAM Control Configuration */
	unsigned int	sdram_cfg_2;	/* 0x2114 - DDR SDRAM Control Configuration 2 */
	unsigned int	sdram_mode;	/* 0x2118 - DDR SDRAM Mode Configuration */
	unsigned int	sdram_mode_2;	/* 0x211c - DDR SDRAM Mode Configuration 2*/
	unsigned int	sdram_interval;	/* 0x2124 - DDR SDRAM Interval Configuration */
	unsigned int	sdram_data_init;/* 0x2128 - DDR SDRAM Data initialization */
	unsigned int	sdram_clk_cntl;	/* 0x2130 - DDR SDRAM Clock Control */
	unsigned int    timing_cfg_4;      /* 0x2160 - DDR SDRAM Timing Configuration Register 4 */
	unsigned int    timing_cfg_5;      /* 0x2164 - DDR SDRAM Timing Configuration Register 5 */
	unsigned int    ddr_zq_cntl;       /* 0x2170 - DDR ZQ calibration control*/
	unsigned int    ddr_wrlvl_cntl;    /* 0x2174 - DDR write leveling control*/
	unsigned int    ddr_pd_cntl;       /* 0x2178 - DDR pre-drive conditioning control*/
	unsigned int    ddrdsr_1;              /* 0x2b20 - DDR Debug Status Register 1 */
	unsigned int    ddrdsr_2;              /* 0x2b24 - DDR Debug Status Register 2 */
	unsigned int    ddrcdr_1;              /* 0x2b28 - DDR Control Driver Register 1 */
	unsigned int    ddrcdr_2;              /* 0x2b2c - DDR Control Driver Register 2 */
} ddr_settings;

static ddr_settings acx2000_proto1a_ddr_settings = {
	.cs0_bnds          = 0x0000007F,     /* CS0 = 0->2G */
	.cs1_bnds          = 0x00000000,
	.cs0_config        = 0x80044302,     /* Enable, ODT_WR=001, 8 banks, 15 row bits, 10 col bits */
	.cs1_config        = 0x00000000,
	.timing_cfg_3      = 0x00030000,     /* CL = 6, tRFC = 64, tRAS = 15 */
	.timing_cfg_0      = 0x003A0104,     /* No R-R/R-W/W-W/W-R trurnaround time */
	.timing_cfg_1      = 0x6F6B0644,
	.timing_cfg_2      = 0x0FA888D0,
	.sdram_cfg         = 0xE7000000,     /* Enable SDRAM, Self Refresh Enable, type=DDR3, No CS interleaving */
	.sdram_cfg_2       = 0x24401011,
	.sdram_mode        = 0x00421222,
	.sdram_mode_2      = 0x04000000,
	.sdram_interval    = 0x0A280100,
	.sdram_data_init   = CONFIG_MEM_INIT_VALUE,
	.sdram_clk_cntl    = 0x02000000,
	.timing_cfg_4      = 0x00220001,
	.timing_cfg_5      = 0x03402400,
	.ddr_zq_cntl       = 0x00000000,
	.ddr_wrlvl_cntl    = 0x8645F607,
	.ddrcdr_1          = 0x00000000,
	.ddrcdr_2          = 0x00000000,
};

static ddr_settings acx1000_ddr_settings = {
	.cs0_bnds          = 0x0000003F,     /* CS0 = 0->1G */
	.cs1_bnds          = 0x00000000,
	.cs0_config        = 0x80044302,     /* Enable, ODT_WR=001, 8 banks, 15 row bits, 10 col bits */
	.cs1_config        = 0x00000000,
	.timing_cfg_3      = 0x00030000,     /* CL = 6, tRFC = 64, tRAS = 15 */
	.timing_cfg_0      = 0x003A0104,     /* No R-R/R-W/W-W/W-R trurnaround time */
	.timing_cfg_1      = 0x6F6B0644,
	.timing_cfg_2      = 0x0FA888D0,
	.sdram_cfg         = 0xE7000000,     /* Enable SDRAM, Self Refresh Enable, type=DDR3, No CS interleaving */
	.sdram_cfg_2       = 0x24401011,
	.sdram_mode        = 0x00421222,
	.sdram_mode_2      = 0x04000000,
	.sdram_interval    = 0x0A280100,
	.sdram_data_init   = CONFIG_MEM_INIT_VALUE,
	.sdram_clk_cntl    = 0x02000000,
	.timing_cfg_4      = 0x00220001,
	.timing_cfg_5      = 0x03402400,
	.ddr_zq_cntl       = 0x00000000,
	.ddr_wrlvl_cntl    = 0x8645F607,
	.ddrcdr_1          = 0x00000000,
	.ddrcdr_2          = 0x00000000,
};

/*
 * Since we receive these settings from Acton as-is, we really
 * don't understand what they mean :-(. The below comments
 * are based on "best-of-understanding"
 */
static ddr_settings acx4000_ddr_settings = {
	.cs0_bnds          = 0x0000007F,     /* CS0 = 0->2G */
	.cs1_bnds          = 0x00000000,
	.cs0_config        = 0x80044302,     /* Enable, ODT_WR=001, 8 banks, 
						15 row bits, 10 col bits */
	.cs1_config        = 0x00000000,
	.timing_cfg_3      = 0x00030000,     /* CL = 6, tRFC = 64, tRAS = 15 */
	.timing_cfg_0      = 0x003A0104,     /* No R-R/R-W/W-W/W-R turnaround*/
	.timing_cfg_1      = 0x6F6B0644,
	.timing_cfg_2      = 0x0FA888D0,
	.sdram_cfg         = 0xE7000000,     /* Enable SDRAM, 
						Self Refresh Enable, type=DDR3,
						No CS interleaving */
	.sdram_cfg_2       = 0x24401011,
	.sdram_mode        = 0x00421222,
	.sdram_mode_2      = 0x04000000,
	.sdram_interval    = 0x0A280100,
	.sdram_data_init   = CONFIG_MEM_INIT_VALUE,
	.sdram_clk_cntl    = 0x02000000,
	.timing_cfg_4      = 0x00220001,
	.timing_cfg_5      = 0x03402400,
	.ddr_zq_cntl       = 0x00000000,
	.ddr_wrlvl_cntl    = 0x8645F607,
	.ddrcdr_1          = 0x00000000,
	.ddrcdr_2          = 0x00000000,
};

unsigned long fixed_sdram(int board_type)
{
	volatile ccsr_ddr_t *ddr_ctlr = (ccsr_ddr_t *)CONFIG_SYS_MPC85xx_DDR_ADDR;
	ddr_settings *ddr_settings;
	unsigned long memsize = 0x80000000; /* 2GB by default */

	switch (board_type) {
	case I2C_ID_ACX1000:
	case I2C_ID_ACX1100:
    		ddr_settings = &acx1000_ddr_settings;
		memsize = 0x40000000;    /* 1GB */
	        break;
	case I2C_ID_ACX2000:
	case I2C_ID_ACX2100:
	case I2C_ID_ACX2200:
    		ddr_settings = &acx2000_proto1a_ddr_settings;
		memsize = 0x80000000;    /* 2GB */
	case I2C_ID_ACX4000:
    		ddr_settings = &acx4000_ddr_settings;
		memsize = 0x80000000;    /* 2GB */
	default:
    		ddr_settings = &acx2000_proto1a_ddr_settings;
		memsize = 0x80000000;    /* 2GB */
	        break;
	}

	ddr_ctlr->cs0_bnds      = ddr_settings->cs0_bnds;
	ddr_ctlr->cs0_config    = ddr_settings->cs0_config;

	ddr_ctlr->cs1_bnds      = ddr_settings->cs1_bnds;
	ddr_ctlr->cs1_config    = ddr_settings->cs1_config;

	ddr_ctlr->timing_cfg_3	= ddr_settings->timing_cfg_3;
	ddr_ctlr->timing_cfg_0  = ddr_settings->timing_cfg_0;
	ddr_ctlr->timing_cfg_1  = ddr_settings->timing_cfg_1;
	ddr_ctlr->timing_cfg_2  = ddr_settings->timing_cfg_2;

	ddr_ctlr->sdram_cfg_2   = ddr_settings->sdram_cfg_2;
	ddr_ctlr->sdram_mode    = ddr_settings->sdram_mode;
	ddr_ctlr->sdram_mode_2  = ddr_settings->sdram_mode_2;

	ddr_ctlr->sdram_interval   = ddr_settings->sdram_interval;
	ddr_ctlr->sdram_data_init  = ddr_settings->sdram_data_init;
	ddr_ctlr->sdram_clk_cntl   = ddr_settings->sdram_clk_cntl;
	ddr_ctlr->timing_cfg_4  = ddr_settings->timing_cfg_4;
	ddr_ctlr->timing_cfg_5  = ddr_settings->timing_cfg_5;

	ddr_ctlr->ddr_zq_cntl   = ddr_settings->ddr_zq_cntl;
	ddr_ctlr->ddr_wrlvl_cntl= ddr_settings->ddr_wrlvl_cntl;

	ddr_ctlr->data_err_inject_hi = 0;

	ddr_ctlr->ddr_dsr1	= ddr_settings->ddrcdr_1;
	ddr_ctlr->ddr_dsr2      = ddr_settings->ddrcdr_2;

	ddr_ctlr->sdram_cfg     = ddr_settings->sdram_cfg & ~(1 << 31); /* Don't enable right now */
	udelay(1000);
	ddr_ctlr->sdram_cfg     = ddr_settings->sdram_cfg | (1 << 31); /* Enable */

	/* Wait until DRAM initialization completes */
	while (ddr_ctlr->sdram_cfg_2 & 0x10) {
	    udelay(1000);
	}

	return memsize;
}

