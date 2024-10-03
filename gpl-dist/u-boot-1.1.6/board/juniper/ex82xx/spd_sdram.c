/*
 * Copyright (c) 2008-2011, Juniper Networks, Inc.
 * All rights reserved.
 *
 * Copyright 2004 Freescale Semiconductor.
 * (C) Copyright 2003 Motorola Inc.
 * Xianghua Xiao (X.Xiao@motorola.com)
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#undef  DEBUG
#include <common.h>
#include <asm/processor.h>
#include <i2c.h>
#include "fsl_i2c.h"
#include <spd.h>
#include <asm/mmu.h>
#include "ex82xx_common.h"
#include "lc_cpld.h"
#include <configs/ex82xx.h>
#include "spd_sdram.h"

#ifdef CONFIG_EX82XX
DECLARE_GLOBAL_DATA_PTR;

#if defined (CONFIG_DDR_ECC) && !defined (CONFIG_ECC_INIT_VIA_DDRCONTROLLER)
extern void dma_init(void);
extern uint dma_check(void);
extern int dma_xfer(void *dest, uint count, void *src);
#endif
extern int lc_cpld_reg_write(int reg, uint8_t val);

void init_ddr_reg_settings(const ddr_cfg_regs_t *regs, unsigned int ctrl_num)
{
	volatile immap_t *immap = (immap_t *)CFG_CCSRBAR;
	volatile ccsr_ddr_t *ddr = &immap->im_ddr;
	unsigned int memsize, i;

	if (ctrl_num) {
	    printf("%s unexpected ctrl_num = %u\n", __FUNCTION__, ctrl_num);
	    return 0;
	}

	ddr->sdram_cfg = 0x42000000;

	for (i = 0; i < CONFIG_CHIP_SELECTS_PER_CTRL; i++) {
	    if (i == 0) {
		ddr->cs0_bnds = regs->cs[i].bnds;
		ddr->cs0_config = regs->cs[i].config;
	    } else if (i == 1) {
		ddr->cs1_bnds = regs->cs[i].bnds;
		ddr->cs1_config = regs->cs[i].config;
	    } else if (i == 2) {
		ddr->cs2_bnds = regs->cs[i].bnds;
		ddr->cs2_config = regs->cs[i].config;
	    } else if (i == 3) {
		ddr->cs3_bnds = regs->cs[i].bnds;
		ddr->cs3_config = regs->cs[i].config;
	    }
	}

	ddr->ext_refrec = regs->ext_refrec;
	ddr->timing_cfg_0 = regs->timing_cfg_0;
	ddr->timing_cfg_1 = regs->timing_cfg_1;
	ddr->timing_cfg_2 = regs->timing_cfg_2;
	ddr->sdram_mode = regs->ddr_sdram_mode;
	ddr->sdram_mode_2 = regs->ddr_sdram_mode_2;
	ddr->sdram_interval = regs->ddr_sdram_interval;
	ddr->sdram_clk_cntl = regs->ddr_sdram_clk_cntl;
	ddr->sdram_data_init = regs->ddr_data_init;
	ddr->sdram_cfg_2 = regs->ddr_sdram_cfg_2;

#ifdef CONFIG_DDR_ECC
	ddr->err_disable = regs->ddr_err_disable; 
	ddr->err_sbe =  regs->ddr_err_sbe;
#endif
#if defined (CONFIG_DDR_ECC) && defined (CONFIG_ECC_INIT_VIA_DDRCONTROLLER)
	ddr->sdram_cfg_2 |= 0x00000010; /* Enable D-INIT bit */ 
#endif

	asm ("sync;isync");
	udelay(500);

#if defined (CONFIG_DDR_ECC)
	/* Enable ECC checking */
	ddr->sdram_cfg = (regs->ddr_sdram_cfg | 0x20000000);
#else
	ddr->sdram_cfg = regs->ddr_sdram_cfg;
#endif

	asm ("sync; isync");
	udelay(500);

#if defined (CONFIG_DDR_ECC) && defined (CONFIG_ECC_INIT_VIA_DDRCONTROLLER)
	while ((ddr->sdram_cfg_2 & (0x00000010)) != 0) {
	    udelay(1000);
	}
#endif

}

#ifdef CONFIG_SPD_EEPROM

#ifndef CFG_READ_SPD
#define CFG_READ_SPD    i2c_read
#endif

static unsigned int setup_laws_and_tlbs(unsigned int memsize);


/*
 * Convert picoseconds into clock cycles (rounding up if needed).
 */

int picos_to_clk(int picos)
{
	int clks;

	clks = picos / (2000000000 / (get_bus_freq(0) / 1000));
	if (picos % (2000000000 / (get_bus_freq(0) / 1000)) != 0) {
		clks++;
	}

	return clks;
}


/*
 * Calculate the Density of each Physical Rank.
 * Returned size is in bytes.
 *
 * Study these table from Byte 31 of JEDEC SPD Spec.
 *
 *		DDR I	DDR II
 *	Bit	Size	Size
 *	---	-----	------
 *	7 high	512MB	512MB
 *	6	256MB	256MB
 *	5	128MB	128MB
 *	4	 64MB	 16GB
 *	3	 32MB	  8GB
 *	2	 16MB	  4GB
 *	1	  2GB	  2GB
 *	0 low	  1GB	  1GB
 *
 * Reorder Table to be linear by stripping the bottom
 * 2 or 5 bits off and shifting them up to the top.
 */

unsigned int compute_banksize(unsigned int mem_type, unsigned char row_dens)
{
	unsigned int bsize;

	if (mem_type == SPD_MEMTYPE_DDR) {
		/* Bottom 2 bits up to the top. */
		bsize = ((row_dens >> 2) | ((row_dens & 3) << 6)) << 24;
		debug("DDR: DDR I rank density = 0x%08x\n", bsize);
	} else {
		/* Bottom 5 bits up to the top. */
		bsize = ((row_dens >> 5) | ((row_dens & 31) << 3)) << 27;
		debug("DDR: DDR II rank density = 0x%08x\n", bsize);
	}
	return bsize;
}


/*
 * Convert a two-nibble BCD value into a cycle time.
 * While the spec calls for nano-seconds, picos are returned.
 *
 * This implements the tables for bytes 9, 23 and 25 for both
 * DDR I and II.  No allowance for distinguishing the invalid
 * fields absent for DDR I yet present in DDR II is made.
 * (That is, cycle times of .25, .33, .66 and .75 ns are
 * allowed for both DDR II and I.)
 */

unsigned int convert_bcd_tenths_to_cycle_time_ps(unsigned int spd_val)
{
	/*
	 * Table look up the lower nibble, allow DDR I & II.
	 */
	unsigned int tenths_ps[16] = {
		0,
		100,
		200,
		300,
		400,
		500,
		600,
		700,
		800,
		900,
		250,
		330,
		660,
		750,
		0,      /* undefined */
		0       /* undefined */
	};

	unsigned int whole_ns = (spd_val & 0xF0) >> 4;
	unsigned int tenth_ns = spd_val & 0x0F;
	unsigned int ps = whole_ns * 1000 + tenths_ps[tenth_ns];

	return ps;
}


/*
 * Determine Refresh Rate.  Ignore self refresh bit on DDR I.
 * Table from SPD Spec, Byte 12, converted to picoseconds and
 * filled in with "default" normal values.
 */
unsigned int determine_refresh_rate(unsigned int spd_refresh)
{
	unsigned int refresh_time_ns[8] = {
		15625000,   /* 0 Normal    1.00x */
		3900000,    /* 1 Reduced    .25x */
		7800000,    /* 2 Extended   .50x */
		31300000,   /* 3 Extended  2.00x */
		62500000,   /* 4 Extended  4.00x */
		125000000,  /* 5 Extended  8.00x */
		15625000,   /* 6 Normal    1.00x  filler */
		15625000,   /* 7 Normal    1.00x  filler */
	};

	return picos_to_clk(refresh_time_ns[spd_refresh & 0x7]);
}


long int spd_sdram(void)
{
	ddr_cfg_regs_t ddr_cfg_regs;
	sys_info_t sysinfo;
	volatile immap_t *immap = (immap_t *)CFG_CCSRBAR;
	volatile ccsr_ddr_t *ddr = &immap->im_ddr;
	unsigned int memsize;
	unsigned int temp_sdram_cfg;	
	unsigned char btcpld_reg;

	get_sys_info(&sysinfo);

#ifdef CONFIG_EX82XX
	if (EX82XX_LCPU) {
		/* DDR3 configuration and initialization sequence for
		 * 2XS_44GE and 48P boards:
		 * At system reset, initialization software (boot code)
		 * must set up the programmable parameters in the memory
		 * interface configuration registers.
		 * Once configuration of all parameters is complete, system
		 * software must set DDR_SDRAM_CFG [MEM_EN] to enable the
		 * memory interface.  Also, a 200us(500us for DDR3) must elapse
		 * after DRAM clocks are stable before MEM_EN can be set, so a
		 * delay loop in the initialization code may be needed if software
		 * is enabling the memory controller. If DDR_SDRAM_CFG [BI] is not
		 * set, the DDR memory controller conducts an automatic initialization
		 * sequence to the memory, which follows the memory specifications.		 
		 */
		if (is_2xs_44ge_48p_board()) {
			/* Don't enable the memory controller now. */
			ddr->sdram_cfg = CONFIG_SYS_SDRAM_CFG_INITIAL;

			ddr->cs0_bnds = CONFIG_SYS_DDR3_CS0_BNDS;
			ddr->cs0_config = CONFIG_SYS_DDR3_CS0_CONFIG;
			ddr->cs0_config_2 = CONFIG_SYS_DDR3_CS0_CONFIG_2;
			/* CONFIG_SYS_DDR3_TIMING_3 sets EXT_REFREC(trfc) to 32 clks. */
			ddr->ext_refrec = CONFIG_SYS_DDR3_TIMING_3;
			ddr->timing_cfg_0 = CONFIG_SYS_DDR3_TIMING_0;
			ddr->timing_cfg_1 = CONFIG_SYS_DDR3_TIMING_1;
			ddr->timing_cfg_2 = CONFIG_SYS_DDR3_TIMING_2;
			ddr->sdram_cfg_2 = CONFIG_SYS_SDRAM_CFG_2;
			ddr->sdram_mode = CONFIG_SYS_DDR3_MODE;
			ddr->sdram_mode_2 = CONFIG_SYS_DDR3_MODE_2;
			ddr->sdram_md_cntl = CONFIG_SYS_DDR3_MD_CNTL;
			ddr->sdram_interval = CONFIG_SYS_DDR3_INTERVAL;
			ddr->sdram_data_init = CONFIG_SYS_DDR3_DATA_INIT;
			ddr->sdram_clk_cntl = CONFIG_SYS_DDR3_CLK_CNTL;
			ddr->init_addr = CONFIG_SYS_DDR3_INIT_ADDR;
			ddr->init_ext_addr = CONFIG_SYS_DDR3_INIT_EXT_ADDR;
			ddr->timing_cfg_4 = CONFIG_SYS_DDR3_TIMING_4;
			ddr->timing_cfg_5 = CONFIG_SYS_DDR3_TIMING_5;
			ddr->ddr_zq_cntl = CONFIG_SYS_DDR3_ZQ_CNTL;
			ddr->ddr_wrlvl_cntl = CONFIG_SYS_DDR3_WRLVL_CNTL;
			ddr->ddr_sr_cntr = CONFIG_SYS_DDR3_SR_CNTR;
			ddr->ddr_sdram_rcw_1 = CONFIG_SYS_DDR3_RCW_1;
			ddr->ddr_sdram_rcw_2 = CONFIG_SYS_DDR3_RCW_2;

#if defined (CONFIG_DDR_ECC)
			ddr->err_disable = CONFIG_SYS_DDR3_ERR_DISABLE;
			ddr->err_sbe = CONFIG_SYS_DDR3_ERR_SBE;
#endif

#if defined (CONFIG_DDR_ECC) && defined (CONFIG_ECC_INIT_VIA_DDRCONTROLLER)
			ddr->sdram_cfg_2 |= CONFIG_SYS_DDR3_DINIT; /* DINIT = 1 */
#endif
			asm ("sync;isync");
			udelay(500);

			btcpld_reg = *(unsigned char*)(CFG_CPLD_BASE + 0x01);
			btcpld_reg &= (~(0x10));
			lc_cpld_reg_write(LC_CPLD_RESET_CTRL, btcpld_reg);

			/* Set, but do not enable the memory yet. */
			temp_sdram_cfg = ddr->sdram_cfg;
			temp_sdram_cfg &= ~(SDRAM_CFG_MEM_EN);
			ddr->sdram_cfg = temp_sdram_cfg;

			/*
			 * At least 500 micro-seconds (in case of DDR3) must
			 * elapse between the DDR clock setup and the DDR config
			 * enable. For now to be on the safer side, wait for 1000
			 * micro-seconds and optimize later after the bringup.
			 */
			asm ("sync;isync");
			udelay(1000);

			/* Let the controller go. Enable. */
			ddr->sdram_cfg = temp_sdram_cfg | SDRAM_CFG_MEM_EN;
#if defined (CONFIG_DDR_ECC)
			/* Enable ECC checking. */
			ddr->sdram_cfg = CONFIG_SYS_SDRAM_CFG | 0x20000000;
#else
			ddr->sdram_cfg = CONFIG_SYS_SDRAM_CFG_NO_ECC;
#endif
			asm ("sync; isync");
			udelay(500);

#if defined (CONFIG_DDR_ECC) && defined (CONFIG_ECC_INIT_VIA_CONTROLLER)
			while((ddr->sdram_cfg_2 & (CONFIG_SYS_DDR3_DINIT)) != 0) {
				udelay(1000);
			}
#endif
		} else {
		    if(sysinfo.freqDDRBus <= DATARATE_400MHZ)
			memcpy(&ddr_cfg_regs, &ddr_cfg_regs_400, sizeof(ddr_cfg_regs));
		    else if(sysinfo.freqDDRBus <= DATARATE_533MHZ)
			memcpy(&ddr_cfg_regs, &ddr_cfg_regs_533, sizeof(ddr_cfg_regs));
		    else {
			puts("mismatch in ddr freq\n ");
			return 0;
		    }

		    init_ddr_reg_settings(&ddr_cfg_regs, 0);
		}
	    
		memsize = CFG_SDRAM_SIZE_LCPU;
		memsize = setup_laws_and_tlbs(memsize);

		if (memsize == 0) {
		    return 0;
		}
	
		return memsize * 1024 * 1024;
	} else if (!EX82XX_RECPU) /* wrong CPU type */ {
	    printf("spd_sdram : Wrong board type  %d\n", gd->ccpu_type);
	    return 0;
	}
#endif

	volatile ccsr_gur_t *gur = &immap->im_gur;
	unsigned int odt_rd_cfg = 0, odt_wr_cfg = 0;
	unsigned int odt_cfg, mode_odt_enable;
	unsigned int refresh_clk;
#ifdef MPC85xx_DDR_SDRAM_CLK_CNTL
	unsigned char clk_adjust;
#endif
	unsigned int dqs_cfg;
	unsigned char twr_clk, twtr_clk, twr_auto_clk;
	unsigned int tCKmin_ps, tCKmax_ps;
	unsigned int max_data_rate, effective_data_rate;
	unsigned int busfreq;
	unsigned sdram_cfg;
	unsigned char caslat, caslat_ctrl;
	unsigned int trfc, trfc_clk, trfc_low, trfc_high;
	unsigned int trcd_clk;
	unsigned int trtp_clk;
	unsigned char cke_min_clk;
	unsigned char add_lat;
	unsigned char wr_lat;
	unsigned char wr_data_delay;
	unsigned char four_act;
	unsigned char cpo;
	unsigned char burst_len;
	unsigned int mode_caslat;
	unsigned char sdram_type;
	unsigned char d_init;
	volatile unsigned int *cs_bnds = &immap->im_ddr.cs0_bnds;
	volatile unsigned int *cs_config = &immap->im_ddr.cs0_config;
	unsigned char i2c_dimm_addr[] = SPD_EEPROM_ADDRESS;
	int dimm_populated[sizeof(i2c_dimm_addr)];
	spd_eeprom_t spd[sizeof(i2c_dimm_addr)];
	unsigned int n_ranks[sizeof(i2c_dimm_addr)];
	unsigned int rank_density[sizeof(i2c_dimm_addr)];
	unsigned int accu_density = 0;
	int num_dimm_banks;                  /* on board dimm banks */
	int dimm_num, rank_num, num_dimm = 0, first_dimm = 0, cs_index = 0;
	uint32_t svr, ver, major_rev, minor_rev;

	num_dimm_banks = sizeof(i2c_dimm_addr);

	/*
	 * Select I2C controller
	 */
#if defined (CONFIG_EX82XX)
	i2c_controller(CFG_I2C_CTRL_1); /*SPD EEPROM sitting on I2C controller 1 bus*/
#endif

	for (dimm_num = 0; dimm_num < num_dimm_banks; dimm_num++) {
		/*
		 * Read SPD information.
		 */
		dimm_populated[dimm_num] = FALSE;

		if (i2c_read(i2c_dimm_addr[dimm_num], 0, 1, (uchar *)&spd[dimm_num],
					 sizeof(spd))) {
			printf("DDR: DIMM[%d] is not populated\n", dimm_num);
			continue;
		}

		/*
		 * Check for supported memory module types.
		 */
		if (spd[dimm_num].mem_type != SPD_MEMTYPE_DDR &&
			spd[dimm_num].mem_type != SPD_MEMTYPE_DDR2) {
			printf("Unable to locate DDR I or DDR II at DIMM[%d] module.\n"
				   "    Fundamental memory type is 0x%0x\n",
				   dimm_num, spd[dimm_num].mem_type);
			continue;
		}

		/*
		 * These test gloss over DDR I and II differences in interpretation
		 * of bytes 3 and 4, but irrelevantly.  Multiple asymmetric banks
		 * are not supported on DDR I; and not encoded on DDR II.
		 *
		 *    12 <= nrow <= 16
		 *     8 <= ncol <= 11 (still, for DDR)
		 */
		if (spd[dimm_num].nrow_addr < 12 || spd[dimm_num].nrow_addr > 16) {
			printf("DDR: DIMM[%d] unsupported number of Row Addr lines: %d.\n",
				   dimm_num, spd[dimm_num].nrow_addr);
			continue;
		}
		if (spd[dimm_num].ncol_addr < 8 || spd[dimm_num].ncol_addr > 11) {
			printf(
				"DDR: DIMM[%d] unsupported number of Column Addr lines: %d.\n",
				dimm_num,
				spd[dimm_num].ncol_addr);
			continue;
		}

		dimm_populated[dimm_num] = TRUE;
		num_dimm++;
		if (1 == num_dimm) {
			first_dimm = dimm_num;     /* first propulated dimm index */
		}
		/*
		 * Determine the number of physical banks controlled by
		 * different Chip Select signals.  This is not quite the
		 * same as the number of DIMM modules on the board.  Feh.
		 */
		if (spd[dimm_num].mem_type == SPD_MEMTYPE_DDR) {
			n_ranks[dimm_num] = spd[dimm_num].nrows;
		} else {
			n_ranks[dimm_num] = (spd[dimm_num].nrows & 0x7) + 1;
		}

		debug("DDR: DIMM[%d] number of ranks = %d\n", dimm_num,
			  n_ranks[dimm_num]);

		/*
		 * Determine the size of each Rank in bytes.
		 */
		rank_density[dimm_num] =
			compute_banksize(spd[dimm_num].mem_type, spd[dimm_num].row_dens);
	}

	if (0 == num_dimm) {
		printf("ERROR - No memory installed. Install a DDR-SDRAM DIMM.\n\n");
		return 0;
	}

	svr = get_svr();
	ver = SVR_VER(svr);
	major_rev = SVR_MAJ(svr);
	minor_rev = SVR_MIN(svr);
	/* Apply DDR 19 errata fix only for MPC8548 Rev2.0 CPU */
	if (((ver == SVR_8548) || (ver == SVR_8548_E)) &&
	    ((major_rev == 2) && (minor_rev == 0))) {
		/*
		 * Adjust DDR II IO voltage biasing.  It just makes it work.
		 * Errata DDR 19: 
		 *   DDR IOs default receiver biasing may not work across
		 *   voltage and temperature.
		 *   - Workaround:
		 *        Write CCSRBAR offset 0xE_0F24 with a value of 0x9000_0000
		 *        for DDR2 and a value of 0xA800_0000 for DDR1.
		 */
		if (spd[first_dimm].mem_type == SPD_MEMTYPE_DDR2) {
			gur->ddrioovcr =
			    (0 |
			    0x80000000 |  /* Enable */
			    0x10000000    /* VSEL to 1.8V */
			    );
		}
	}

	for (dimm_num = 0; dimm_num < num_dimm_banks; dimm_num++) {
		if (dimm_populated[dimm_num] &&
			(cs_index + n_ranks[dimm_num] > CFG_DDR_CS)) {
			printf(
				"DDR: DIMM[%d] needs additional %d chip selects which \
				is %d over maximum (%d) supported.\n",
				dimm_num,
				n_ranks[first_dimm],
				cs_index + n_ranks[dimm_num] - CFG_DDR_CS,
				CFG_DDR_CS);
			break;
		}

		if (dimm_populated[dimm_num]) {
			/*
			 * ODT configuration recommendation from DDR Controller Chapter.
			 */
			odt_rd_cfg = 0;             /* Never assert ODT */
			odt_wr_cfg = 0;             /* Never assert ODT */
			if (spd[dimm_num].mem_type == SPD_MEMTYPE_DDR2) {
				odt_wr_cfg = 1;         /* Assert ODT on writes to CS0 */
#if 0	
				/* FIXME: How to determine the number of dimm modules? */
				if (n_dimm_modules == 2) {
					odt_rd_cfg = 1; /* Assert ODT on reads to CS0 */
				}
#endif
			}

			for (rank_num = 0; rank_num < n_ranks[dimm_num]; rank_num++) {
				*cs_bnds = ( (accu_density >> 8)
							 | (((accu_density +
								  rank_density[dimm_num]) >> 24)  - 1) );
				*cs_config = ( 1 << 31
						   | (odt_rd_cfg << 20)
						   | (odt_wr_cfg << 16)
						   | (spd[dimm_num].nrow_addr - 12) << 8
#if defined (CONFIG_EX3242)
						   | (spd[dimm_num].ncol_addr - 8));
#elif defined (CONFIG_EX82XX)
						   | (spd[dimm_num].ncol_addr - 8)
						   | (1 << 14)); 
						   /* FIXME - HARD CODED FIX- 3 bank bits (RECPU -DDR DIMM)*/
#endif

				debug("\n");
				debug("DDR: cs%d_bnds   = 0x%08x\n", cs_index, *cs_bnds);
				debug("DDR: cs%d_config = 0x%08x\n", cs_index, *cs_config);
				accu_density += rank_density[dimm_num];
				cs_index++;
				cs_bnds = cs_bnds + 2;
				cs_config = cs_config + 1;
			}
		}
	}


	/*
	 * Find the largest CAS by locating the highest 1 bit
	 * in the spd.cas_lat field.  Translate it to a DDR
	 * controller field value:
	 *
	 *	CAS Lat	DDR I	DDR II	Ctrl
	 *	Clocks	SPD Bit	SPD Bit	Value
	 *	-------	-------	-------	-----
	 *	1.0	0		0001
	 *	1.5	1		0010
	 *	2.0	2	2	0011
	 *	2.5	3		0100
	 *	3.0	4	3	0101
	 *	3.5	5		0110
	 *	4.0		4	0111
	 *	4.5			1000
	 *	5.0		5	1001
	 */
	caslat = __ilog2(spd[first_dimm].cas_lat);
	if ((spd[first_dimm].mem_type == SPD_MEMTYPE_DDR)
		&& (caslat > 5)) {
		printf("DDR I: Invalid SPD CAS Latency: 0x%x.\n",
			   spd[first_dimm].cas_lat);
		return 0;
	} else if (spd[first_dimm].mem_type == SPD_MEMTYPE_DDR2
			   && (caslat < 2 || caslat > 5)) {
		printf("DDR II: Invalid SPD CAS Latency: 0x%x.\n",
			   spd[first_dimm].cas_lat);
		return 0;
	}
	debug("DDR: caslat SPD bit is %d\n", caslat);

	/*
	 * Calculate the Maximum Data Rate based on the Minimum Cycle time.
	 * The SPD clk_cycle field (tCKmin) is measured in tenths of
	 * nanoseconds and represented as BCD.
	 */
	tCKmin_ps = convert_bcd_tenths_to_cycle_time_ps(spd[first_dimm].clk_cycle);
	debug("DDR: tCKmin = %d ps\n", tCKmin_ps);

	/*
	 * Double-data rate, scaled 1000 to picoseconds, and back down to MHz.
	 */
	max_data_rate = 2 * 1000 * 1000 / tCKmin_ps;
	debug("DDR: Module max data rate = %d Mhz\n", max_data_rate);


	/*
	 * Adjust the CAS Latency to allow for bus speeds that
	 * are slower than the DDR module.
	 */
	busfreq = get_bus_freq(0) / 1000000;    /* MHz */

	effective_data_rate = max_data_rate;
	if (busfreq < 90) {
		/* DDR rate out-of-range */
		puts("DDR: platform frequency is not fit for DDR rate\n");
		return 0;
	} else if (90 <= busfreq && busfreq < 230 && max_data_rate >= 230) {
		/*
		 * busfreq 90~230 range, treated as DDR 200.
		 */
		effective_data_rate = 200;
		if (spd[first_dimm].clk_cycle3 == 0xa0) { /* 10 ns */
			caslat -= 2;
		} else if (spd[first_dimm].clk_cycle2 == 0xa0) {
			caslat--;
		}
	} else if (230 <= busfreq && busfreq < 280 && max_data_rate >= 280) {
		/*
		 * busfreq 230~280 range, treated as DDR 266.
		 */
		effective_data_rate = 266;
		if (spd[first_dimm].clk_cycle3 == 0x75) { /* 7.5 ns */
			caslat -= 2;
		} else if (spd[first_dimm].clk_cycle2 == 0x75) {
			caslat--;
		}
	} else if (280 <= busfreq && busfreq < 350 && max_data_rate >= 350) {
		/*
		 * busfreq 280~350 range, treated as DDR 333.
		 */
		effective_data_rate = 333;
		if (spd[first_dimm].clk_cycle3 == 0x60) { /* 6.0 ns */
			caslat -= 2;
		} else if (spd[first_dimm].clk_cycle2 == 0x60) {
			caslat--;
		}
	} else if (350 <= busfreq && busfreq < 460 && max_data_rate >= 460) {
		/*
		 * busfreq 350~460 range, treated as DDR 400.
		 */
		effective_data_rate = 400;
		if (spd[first_dimm].clk_cycle3 == 0x50) { /* 5.0 ns */
			caslat -= 2;
		} else if (spd[first_dimm].clk_cycle2 == 0x50) {
			caslat--;
		}
	} else if (460 <= busfreq && busfreq < 560 && max_data_rate >= 560) {
		/*
		 * busfreq 460~560 range, treated as DDR 533.
		 */
		effective_data_rate = 533;
		if (spd[first_dimm].clk_cycle3 == 0x3D) { /* 3.75 ns */
			caslat -= 2;
		} else if (spd[first_dimm].clk_cycle2 == 0x3D) {
			caslat--;
		}
	} else if (560 <= busfreq && busfreq < 700 && max_data_rate >= 700) {
		/*
		 * busfreq 560~700 range, treated as DDR 667.
		 */
		effective_data_rate = 667;
		if (spd[first_dimm].clk_cycle3 == 0x30) { /* 3.0 ns */
			caslat -= 2;
		} else if (spd[first_dimm].clk_cycle2 == 0x30) {
			caslat--;
		}
	} else if (700 <= busfreq) {
		/*
		 * DDR rate out-of-range
		 */
		printf("DDR: Bus freq %d MHz is not fit for DDR rate %d MHz\n",
			   busfreq, max_data_rate);
		return 0;
	}


	/*
	 * Convert caslat clocks to DDR controller value.
	 * Force caslat_ctrl to be DDR Controller field-sized.
	 */
	if (spd[first_dimm].mem_type == SPD_MEMTYPE_DDR) {
		caslat_ctrl = (caslat + 1) & 0x07;
	} else {
		caslat_ctrl =  (2 * caslat - 1) & 0x0f;
	}

	debug("DDR: effective data rate is %d MHz\n", effective_data_rate);
	debug("DDR: caslat SPD bit is %d, controller field is 0x%x\n",
		  caslat, caslat_ctrl);

	/*
	 * Timing Config 0.
	 * Avoid writing for DDR I.  The new PQ38 DDR controller
	 * dreams up non-zero default values to be backwards compatible.
	 */
	if (spd[first_dimm].mem_type == SPD_MEMTYPE_DDR2) {
		unsigned char taxpd_clk = 8;        /* By the book. */
		unsigned char tmrd_clk = 2;         /* By the book. */
		unsigned char act_pd_exit = 2;      /* Empirical? */
		unsigned char pre_pd_exit = 6;      /* Empirical? */

		ddr->timing_cfg_0 = (0
							 | ((act_pd_exit & 0x7) << 20)  /* ACT_PD_EXIT */
							 | ((pre_pd_exit & 0x7) << 16)  /* PRE_PD_EXIT */
							 | ((taxpd_clk & 0xf) << 8)     /* ODT_PD_EXIT */
							 | ((tmrd_clk & 0xf) << 0)      /* MRS_CYC */
							 );
		debug("DDR: timing_cfg_0 = 0x%08x\n", ddr->timing_cfg_0);
	}


	/*
	 * Some Timing Config 1 values now.
	 * Sneak Extended Refresh Recovery in here too.
	 */

	/*
	 * For DDR I, WRREC(Twr) and WRTORD(Twtr) are not in SPD,
	 * use conservative value.
	 * For DDR II, they are bytes 36 and 37, in quarter nanos.
	 */

	if (spd[first_dimm].mem_type == SPD_MEMTYPE_DDR) {
		twr_clk = 3;    /* Clocks */
		twtr_clk = 1;   /* Clocks */
	} else {
		twr_clk = picos_to_clk(spd[first_dimm].twr * 250);
		twtr_clk = picos_to_clk(spd[first_dimm].twtr * 250);
	}

	/*
	 * Calculate Trfc, in picos.
	 * DDR I:  Byte 42 straight up in ns.
	 * DDR II: Byte 40 and 42 swizzled some, in ns.
	 */
	if (spd[first_dimm].mem_type == SPD_MEMTYPE_DDR) {
		trfc = spd[first_dimm].trfc * 1000;     /* up to ps */
	} else {
		unsigned int byte40_table_ps[8] = {
			0,
			250,
			330,
			500,
			660,
			750,
			0,
			0
		};

		trfc =
			(((spd[first_dimm].trctrfc_ext &
			   0x1) * 256) + spd[first_dimm].trfc) * 1000
			+ byte40_table_ps[(spd[first_dimm].trctrfc_ext >> 1) & 0x7];
	}
	trfc_clk = picos_to_clk(trfc);

	/*
	 * Trcd, Byte 29, from quarter nanos to ps and clocks.
	 */
	trcd_clk = picos_to_clk(spd[first_dimm].trcd * 250) & 0x7;

	/*
	 * Convert trfc_clk to DDR controller fields.  DDR I should
	 * fit in the REFREC field (16-19) of TIMING_CFG_1, but the
	 * 8548 controller has an extended REFREC field of three bits.
	 * The controller automatically adds 8 clocks to this value,
	 * so preadjust it down 8 first before splitting it up.
	 */
	trfc_low = (trfc_clk - 8) & 0xf;
	trfc_high = ((trfc_clk - 8) >> 4) & 0x3;

	/*
	 * Sneak in some Extended Refresh Recovery.
	 */
	ddr->ext_refrec = (trfc_high << 16);
	debug("DDR: ext_refrec = 0x%08x\n", ddr->ext_refrec);

	ddr->timing_cfg_1 =
        (0
		 | ((picos_to_clk(spd[first_dimm].trp * 250) & 0x07) << 28)     /* PRETOACT */
		 | ((picos_to_clk(spd[first_dimm].tras * 1000) & 0x0f ) << 24)  /* ACTTOPRE */
		 | (trcd_clk << 20)                                             /* ACTTORW */
		 | (caslat_ctrl << 16)                                          /* CASLAT */
		 | (trfc_low << 12)                                             /* REFEC */
		 | ((twr_clk & 0x07) << 8)                                      /* WRRREC */
		 | ((picos_to_clk(spd[first_dimm].trrd * 250) & 0x07) << 4)     /* ACTTOACT */
		 | ((twtr_clk & 0x07) << 0)                                     /* WRTORD */
		);

	debug("DDR: timing_cfg_1  = 0x%08x\n", ddr->timing_cfg_1);


	/*
	 * Timing_Config_2
	 * Was: 0x00000800;
	 */

	/*
	 * Additive Latency
	 * For DDR I, 0.
	 * For DDR II, with ODT enabled, use "a value" less than ACTTORW,
	 * which comes from Trcd, and also note that:
	 *	add_lat + caslat must be >= 4
	 */
	add_lat = 0;
	if (spd[first_dimm].mem_type == SPD_MEMTYPE_DDR2
		&& (odt_wr_cfg || odt_rd_cfg)
		&& (caslat < 4)) {
		add_lat = 4 - caslat;
		if (add_lat > trcd_clk) {
			add_lat = trcd_clk - 1;
		}
	}

	/*
	 * Write Data Delay
	 * Historically 0x2 == 4/8 clock delay.
	 * Empirically, 0x3 == 6/8 clock delay is suggested for DDR I 266.
	 */
	wr_data_delay = 2; /* 1/2 clock delay. SI recommendation */

	/*
	 * Write Latency
	 * Read to Precharge
	 * Minimum CKE Pulse Width.
	 * Four Activate Window
	 */
	if (spd[first_dimm].mem_type == SPD_MEMTYPE_DDR) {
		/*
		 * This is a lie.  It should really be 1, but if it is
		 * set to 1, bits overlap into the old controller's
		 * otherwise unused ACSM field.  If we leave it 0, then
		 * the HW will magically treat it as 1 for DDR 1.  Oh Yea.
		 */
		wr_lat = 0;

		trtp_clk = 2;       /* By the book. */
		cke_min_clk = 1;    /* By the book. */
		four_act = 1;       /* By the book. */
	} else {
		wr_lat = caslat - 1;

		/* Convert SPD value from quarter nanos to picos. */
		trtp_clk = picos_to_clk(spd[first_dimm].trtp * 250);

		cke_min_clk = 3;                /* By the book. */
		four_act = picos_to_clk(37500); /* By the book. 1k pages? */
	}

	/*
	 * Empirically set ~MCAS-to-preamble override for DDR 2.
	 * Your milage will vary.
	 */
	cpo = 0;
	if (spd[first_dimm].mem_type == SPD_MEMTYPE_DDR2) {
		if (effective_data_rate == 266 || effective_data_rate == 333) {
			cpo = 0x7;      /* READ_LAT + 5/4 */
		} else if (effective_data_rate == 400) {
			cpo = 0x6;   /* READ_LAT + 1 - changed as per SI recommendation */
		} else {
			/* Pure speculation */
			cpo = 0xb;
		}
	}

	ddr->timing_cfg_2 = (0
   	        | ((add_lat & 0x7) << 28)          /* ADD_LAT */
            | ((cpo & 0x1f) << 23)             /* CPO */
            | ((wr_lat & 0x7) << 19)           /* WR_LAT */
            | ((trtp_clk & 0x7) << 13)         /* RD_TO_PRE */
            | ((wr_data_delay & 0x7) << 10)    /* WR_DATA_DELAY */
            | ((cke_min_clk & 0x7) << 6)       /* CKE_PLS */
            | ((four_act & 0x1f) << 0));       /* FOUR_ACT */

	debug("DDR: timing_cfg_2 = 0x%08x\n", ddr->timing_cfg_2);


	/*
	 * Determine the Mode Register Set.
	 *
	 * This is nominally part specific, but it appears to be
	 * consistent for all DDR I devices, and for all DDR II devices.
	 *
	 *     caslat must be programmed
	 *     burst length is always 4
	 *     burst type is sequential
	 *
	 * For DDR I:
	 *     operating mode is "normal"
	 *
	 * For DDR II:
	 *     other stuff
	 */

	mode_caslat = 0;

	/*
	 * Table lookup from DDR I or II Device Operation Specs.
	 */
	if (spd[first_dimm].mem_type == SPD_MEMTYPE_DDR) {
		if (1 <= caslat && caslat <= 4) {
			unsigned char mode_caslat_table[4] = {
				0x5,    /* 1.5 clocks */
				0x2,    /* 2.0 clocks */
				0x6,    /* 2.5 clocks */
				0x3     /* 3.0 clocks */
			};
			mode_caslat = mode_caslat_table[caslat - 1];
		} else {
			puts("DDR I: Only CAS Latencies of 1.5, 2.0, "
				 "2.5 and 3.0 clocks are supported.\n");
			return 0;
		}
	} else {
		if (2 <= caslat && caslat <= 5) {
			mode_caslat = caslat;
		} else {
			puts("DDR II: Only CAS Latencies of 2.0, 3.0, "
				 "4.0 and 5.0 clocks are supported.\n");
			return 0;
		}
	}

	/*
	 * Encoded Burst Lenght of 4.
	 */
	burst_len = 2;          /* Fiat. */

	if (spd[first_dimm].mem_type == SPD_MEMTYPE_DDR) {
		twr_auto_clk = 0;   /* Historical */
	} else {
		/*
		 * Determine tCK max in picos.  Grab tWR and convert to picos.
		 * Auto-precharge write recovery is:
		 *	WR = roundup(tWR_ns/tCKmax_ns).
		 *
		 * Ponder: Is twr_auto_clk different than twr_clk?
		 */
		tCKmax_ps = convert_bcd_tenths_to_cycle_time_ps(spd[first_dimm].tckmax);
		twr_auto_clk = (spd[first_dimm].twr * 250 + tCKmax_ps - 1) / tCKmax_ps;
	}


	/*
	 * Mode Reg in bits 16 ~ 31,
	 * Extended Mode Reg 1 in bits 0 ~ 15.
	 */
	mode_odt_enable = 0x0;          /* Default disabled */
	if (odt_wr_cfg || odt_rd_cfg) {
		/*
		 * Bits 6 and 2 in Extended MRS(1)
		 * Bit 2 == 0x04 == 75 Ohm, with 2 DIMM modules.
		 * Bit 6 == 0x40 == 150 Ohm, with 1 DIMM module.
		 */
		mode_odt_enable = 0x40;     /* 150 Ohm */
	}

	ddr->sdram_mode =
		(0
		 | (add_lat << (16 + 3))    /* Additive Latency in EMRS1 */
		 | (mode_odt_enable << 16)  /* ODT Enable in EMRS1 */
		 | (twr_auto_clk << 9)      /* Write Recovery Autopre */
		 | (mode_caslat << 4)       /* caslat */
		 | (burst_len << 0)         /* Burst length */
		);

	debug("DDR: sdram_mode   = 0x%08x\n", ddr->sdram_mode);


	/*
	 * Clear EMRS2 and EMRS3.
	 */
	ddr->sdram_mode_2 = 0;
	debug("DDR: sdram_mode_2 = 0x%08x\n", ddr->sdram_mode_2);

	/*
	 * Determine Refresh Rate.
	 */
	refresh_clk = determine_refresh_rate(spd[first_dimm].refresh & 0x7);

	/*
	 * Set BSTOPRE to 0x100 for page mode
	 * If auto-charge is used, set BSTOPRE = 0
	 */
	ddr->sdram_interval =
		(0
		 | (refresh_clk & 0x3fff) << 16
		 | 0x100
		);
	debug("DDR: sdram_interval = 0x%08x\n", ddr->sdram_interval);

	/*
	 * Is this an ECC DDR chip?
	 * But don't mess with it if the DDR controller will init mem.
	 */
#if defined (CONFIG_DDR_ECC) && !defined (CONFIG_ECC_INIT_VIA_DDRCONTROLLER)
	if (spd[first_dimm].config == 0x02) {
		ddr->err_disable = 0x0000000d;
		ddr->err_sbe = 0x00ff0000;
	}
	debug("DDR: err_disable = 0x%08x\n", ddr->err_disable);
	debug("DDR: err_sbe = 0x%08x\n", ddr->err_sbe);
#endif

	asm ("sync;isync;msync");
	udelay(500);

	/*
	 * SDRAM Cfg 2
	 */

	/*
	 * When ODT is enabled, Chap 9 suggests asserting ODT to
	 * internal IOs only during reads.
	 */
	odt_cfg = 0;
	if (odt_rd_cfg | odt_wr_cfg) {
		odt_cfg = 0x2;      /* ODT to IOs during reads */
	}

	/*
	 * Try to use differential DQS with DDR II.
	 */
	if (spd[first_dimm].mem_type == SPD_MEMTYPE_DDR) {
		dqs_cfg = 0;        /* No Differential DQS for DDR I */
	} else {
		dqs_cfg = 0x1;      /* Differential DQS for DDR II */
	}

#if defined (CONFIG_ECC_INIT_VIA_DDRCONTROLLER)
	/*
	 * Use the DDR controller to auto initialize memory.
	 */
	d_init = 1;
	ddr->sdram_data_init = CONFIG_MEM_INIT_VALUE;
	debug("DDR: ddr_data_init = 0x%08x\n", ddr->sdram_data_init);
#else
	/*
	 * Memory will be initialized via DMA, or not at all.
	 */
	d_init = 0;
#endif

	ddr->sdram_cfg_2 = (0
						| (dqs_cfg << 26)   /* Differential DQS */
						| (odt_cfg << 21)   /* ODT */
						| (d_init << 4)     /* D_INIT auto init DDR */
						);

	debug("DDR: sdram_cfg_2  = 0x%08x\n", ddr->sdram_cfg_2);


#ifdef MPC85xx_DDR_SDRAM_CLK_CNTL
	/*
	 * Setup the clock control.
	 * SDRAM_CLK_CNTL[0] = Source synchronous enable == 1
	 * SDRAM_CLK_CNTL[5-7] = Clock Adjust
	 *  0100    1/2 cycle late
	 *	0110	3/4 cycle late
	 *	0111	7/8 cycle late
	 */
	if (spd[first_dimm].mem_type == SPD_MEMTYPE_DDR) {
		clk_adjust = 0x6;
	} else {
		/* change the value to 0x4 as suggested by the SI team */
		clk_adjust = 0x4;

	}

	ddr->sdram_clk_cntl = (0
						   | 0x80000000
						   | (clk_adjust << 23)
						   );
	debug("DDR: sdram_clk_cntl = 0x%08x\n", ddr->sdram_clk_cntl);
#endif

	/*
	 * Figure out the settings for the sdram_cfg register.
	 * Build up the entire register in 'sdram_cfg' before writing
	 * since the write into the register will actually enable the
	 * memory controller; all settings must be done before enabling.
	 *
	 * sdram_cfg[0]   = 1 (ddr sdram logic enable)
	 * sdram_cfg[1]   = 1 (self-refresh-enable)
	 * sdram_cfg[5:7] = (SDRAM type = DDR SDRAM)
	 *			010 DDR 1 SDRAM
	 *			011 DDR 2 SDRAM
	 */
	sdram_type = (spd[first_dimm].mem_type == SPD_MEMTYPE_DDR) ? 2 : 3;
	sdram_cfg = (0
				 | (1 << 31)            /* Enable */
				 | (1 << 30)            /* Self refresh */
				 | (1 << 28)            /* REGISTERED dimm */
				 | (sdram_type << 24)   /* SDRAM type */
				 );

	/*
	 * sdram_cfg[3] = RD_EN - registered DIMM enable
	 *   A value of 0x26 indicates micron registered DIMMS (micron.com)
	 */
	if (spd[first_dimm].mem_type == SPD_MEMTYPE_DDR &&
		spd[first_dimm].mod_attr == 0x26) {
		sdram_cfg |= 0x10000000;        /* RD_EN */
	}

#if defined (CONFIG_DDR_ECC)
	/*
	 * If the user wanted ECC (enabled via sdram_cfg[2])
	 */
	if (spd[first_dimm].config == 0x02) {
		sdram_cfg |= 0x20000000;        /* ECC_EN */
	}
#endif

	/*
	 * REV1 uses 1T timing.
	 * REV2 may use 1T or 2T as configured by the user.
	 */
	{
		uint pvr = get_pvr();

		if (pvr != PVR_85xx_REV1) {
#if defined (CONFIG_DDR_2T_TIMING)
			/*
			 * Enable 2T timing by setting sdram_cfg[16].
			 */
			sdram_cfg |= 0x8000;        /* 2T_EN */
#endif
		}
	}

	/*
	 * 200 painful micro-seconds must elapse between
	 * the DDR clock setup and the DDR config enable.
	 */
	udelay(200);

	/*
	 * Go!
	 */
	ddr->sdram_cfg = sdram_cfg;

	asm ("sync;isync;msync");
	udelay(500);

	debug("DDR: sdram_cfg   = 0x%08x\n", ddr->sdram_cfg);


#if defined (CONFIG_ECC_INIT_VIA_DDRCONTROLLER)
	/*
	 * Poll until memory is initialized.
	 * 512 Meg at 400 might hit this 200 times or so.
	 */
	while ((ddr->sdram_cfg_2 & (d_init << 4)) != 0) {
		udelay(1000);
	}
#endif


	/*
	 * Figure out memory size in Megabytes.
	 */
	memsize = accu_density / 0x100000;

	/*
	 * Establish Local Access Window and TLB mappings for DDR memory.
	 */
	memsize = setup_laws_and_tlbs(memsize);
	if (memsize == 0) {
		return 0;
	}

	return memsize * 1024 * 1024;
}


/*
 * Setup Local Access Window and TLB1 mappings for the requested
 * amount of memory.  Returns the amount of memory actually mapped
 * (usually the original request size), or 0 on error.
 */

static unsigned int setup_laws_and_tlbs(unsigned int memsize)
{
	volatile immap_t *immap = (immap_t *)CFG_CCSRBAR;
	volatile ccsr_local_ecm_t *ecm = &immap->im_local_ecm;
	unsigned int tlb_size;
	unsigned int law_size;
	unsigned int ram_tlb_index;
	unsigned int ram_tlb_address;

	/*
	 * Determine size of each TLB1 entry.
	 */
	switch (memsize) {
	case 16:
	case 32:
		tlb_size = BOOKE_PAGESZ_16M;
		break;
	case 64:
	case 128:
		tlb_size = BOOKE_PAGESZ_64M;
		break;
	case 256:
	case 512:
	case 1024:
	case 2048:
		tlb_size = BOOKE_PAGESZ_256M;
		break;
	default:
		puts("DDR: only 16M,32M,64M,128M,256M,512M,1G and 2G are supported.\n");

		/*
		 * The memory was not able to be mapped.
		 */
		return 0;
		break;
	}

	/*
	 * Configure DDR TLB1 entries.
	 * Starting at TLB1 8, use no more than 8 TLB1 entries.
	 */
	ram_tlb_index = 8;
	ram_tlb_address = (unsigned int)CFG_DDR_SDRAM_BASE;
	while (ram_tlb_address < (memsize * 1024 * 1024)
		   && ram_tlb_index < 16) {
		mtspr(MAS0, TLB1_MAS0(1, ram_tlb_index, 0));
		mtspr(MAS1, TLB1_MAS1(1, 1, 0, 0, tlb_size));
		mtspr(MAS2, TLB1_MAS2(E500_TLB_EPN(ram_tlb_address),
							  0, 0, 0, 0, 0, 0, 0, 0));
		mtspr(MAS3, TLB1_MAS3(E500_TLB_RPN(ram_tlb_address),
							  0, 0, 0, 0, 0, 1, 0, 1, 0, 1));
		asm volatile ("isync;msync;tlbwe;isync");

		debug("DDR: MAS0=0x%08x\n", TLB1_MAS0(1, ram_tlb_index, 0));
		debug("DDR: MAS1=0x%08x\n", TLB1_MAS1(1, 1, 0, 0, tlb_size));
		debug("DDR: MAS2=0x%08x\n",
			  TLB1_MAS2(E500_TLB_EPN(ram_tlb_address),
						0, 0, 0, 0, 0, 0, 0, 0));
		debug("DDR: MAS3=0x%08x\n",
			  TLB1_MAS3(E500_TLB_RPN(ram_tlb_address),
						0, 0, 0, 0, 0, 1, 0, 1, 0, 1));

		ram_tlb_address += (0x1000 << ((tlb_size - 1) * 2));
		ram_tlb_index++;
	}

	/*
	 * First supported LAW size is 16M, at LAWAR_SIZE_16M == 23.  Fnord.
	 */
	law_size = 19 + __ilog2(memsize);

	/*
	 * Set up LAWBAR for all of DDR.
	 */
	ecm->lawbar1 = ((CFG_DDR_SDRAM_BASE >> 12) & 0xfffff);
	ecm->lawar1 = (LAWAR_EN
				   | LAWAR_TRGT_IF_DDR
				   | (LAWAR_SIZE & law_size));
	debug("DDR: LAWBAR1=0x%08x\n", ecm->lawbar1);
	debug("DDR: LARAR1=0x%08x\n", ecm->lawar1);

	/*
	 * Confirm that the requested amount of memory was mapped.
	 */
	return memsize;
}


#else
long int spd_sdram(void)
{
	ddr_cfg_regs_t ddr_cfg_regs;
	sys_info_t sysinfo;
	volatile immap_t *immap = (immap_t *)CFG_IMMR;
	volatile ccsr_ddr_t *ddr = &immap->im_ddr;
	unsigned int memsize;

	get_sys_info(&sysinfo);

#ifdef CONFIG_EX82XX
	if (EX82XX_LCPU) {
	    if(sysinfo.freqDDRBus <= DATARATE_400MHZ)
		memcpy(&ddr_cfg_regs, &ddr_cfg_regs_400, sizeof(ddr_cfg_regs));
	    else if(sysinfo.freqDDRBus <= DATARATE_533MHZ)
		memcpy(&ddr_cfg_regs, &ddr_cfg_regs_533, sizeof(ddr_cfg_regs));
	    else {
		puts("option mismatch in ddr speeds \n");
		return 0;
	    }

	    init_ddr_reg_settings(&ddr_cfg_regs, 0);
	    memsize = CFG_SDRAM_SIZE_LCPU;
	    
	    if (memsize == 0) {
		return 0;
	    }

	    return memsize * 1024 * 1024;
	} else if (!EX82XX_RECPU) /* wrong CPU type */ {
	    printf("spd_sdram : Wrong board type %d \n", gd->ccpu_type);
	    return 0;
	}
#endif
}
#endif /* CONFIG_SPD_EEPROM */


#if defined (CONFIG_DDR_ECC) && !defined (CONFIG_ECC_INIT_VIA_DDRCONTROLLER)

/*
 * Initialize all of memory for ECC, then enable errors.
 */

void ddr_enable_ecc(unsigned int dram_size)
{
	uint *p = 0;
	uint i = 0;
	volatile immap_t *immap = (immap_t *)CFG_CCSRBAR;
	volatile ccsr_ddr_t *ddr = &immap->im_ddr;

	dma_init();

	for (*p = 0; p < (uint *)(8 * 1024); p++) {
		if (((unsigned int)p & 0x1f) == 0) {
			ppcDcbz((unsigned long)p);
		}
		*p = (unsigned int)CONFIG_MEM_INIT_VALUE;
		if (((unsigned int)p & 0x1c) == 0x1c) {
			ppcDcbf((unsigned long)p);
		}
	}

	dma_xfer((uint *)0x002000, 0x002000, (uint *)0);    /* 8K */
	dma_xfer((uint *)0x004000, 0x004000, (uint *)0);    /* 16K */
	dma_xfer((uint *)0x008000, 0x008000, (uint *)0);    /* 32K */
	dma_xfer((uint *)0x010000, 0x010000, (uint *)0);    /* 64K */
	dma_xfer((uint *)0x020000, 0x020000, (uint *)0);    /* 128k */
	dma_xfer((uint *)0x040000, 0x040000, (uint *)0);    /* 256k */
	dma_xfer((uint *)0x080000, 0x080000, (uint *)0);    /* 512k */
	dma_xfer((uint *)0x100000, 0x100000, (uint *)0);    /* 1M */
	dma_xfer((uint *)0x200000, 0x200000, (uint *)0);    /* 2M */
	dma_xfer((uint *)0x400000, 0x400000, (uint *)0);    /* 4M */

	for (i = 1; i < dram_size / 0x800000; i++) {
		dma_xfer((uint *)(0x800000 * i), 0x800000, (uint *)0);
	}

	/*
	 * Enable errors for ECC.
	 */
	debug("DMA DDR: err_disable = 0x%08x\n", ddr->err_disable);
	ddr->err_disable = 0x00000000;
	asm ("sync;isync;msync");
	debug("DMA DDR: err_disable = 0x%08x\n", ddr->err_disable);
}

#endif  /* CONFIG_DDR_ECC  && ! CONFIG_ECC_INIT_VIA_DDRCONTROLLER */
#endif   /* CONFIG_EX82XX */
