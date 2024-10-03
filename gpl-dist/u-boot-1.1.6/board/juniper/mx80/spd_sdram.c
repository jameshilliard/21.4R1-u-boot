/*
 * Copyright (c) 2009-2013, Juniper Networks, Inc.
 * All rights reserved.
 *
 * Copyright (C) 2008 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Author: James Yang (James.Yang@freescale.com) Apr 13 2008
 *
 *   - Enable runtime configuration of memory interleaving modes through
 *   u-boot environment variables.  See comments for configuration
 *   instructions.  Extensive validation of memory interleaving
 *   configuration with failsafe to non-interleaved mode.
 *
 *   - Set ODT for writes properly in single-DIMM and dual-DIMM
 *   configurations.
 *
 *   - Call tabulated memory timing parameters (cpo, wr_data_delay,
 *   clk_adjust) indexed by the detected memory frequency and configuration
 *   instead of hard coding a single config.  Parameters need to be more
 *   extensively verified, but this patch adds the structure and
 *   functionality.
 *
 *   - Improved 2-DIMM support.  Mismatched DIMMs (size, timing, etc.) are
 *   now allowed.  Changed LAW table to enable this.
 *
 *
 *
 *
 * Copyright 2004, 2007 Freescale Semiconductor.
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

#include <common.h>
#include <asm/processor.h>
#include <i2c.h>
#include <spd.h>
#include <asm/mmu.h>


#if defined(CONFIG_DDR_ECC) && !defined(CONFIG_ECC_INIT_VIA_DDRCONTROLLER)
extern void dma_init(void);
extern uint dma_check(void);
extern int dma_xfer(void *dest, uint count, void *src);
#endif


#ifdef CONFIG_SPD_EEPROM

#ifndef	CFG_READ_SPD
#define CFG_READ_SPD	i2c_read
#endif

static void setup_tlbs(void);

#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

typedef struct {
	unsigned char present;
	unsigned char nranks;
	unsigned char rowbits;
	unsigned char colbits;
	unsigned char bankbits;
	unsigned char regdimm;
} dimmparams_t;

#ifdef DEBUG
static void print_dimmparams(unsigned int memctl, unsigned int dimm,
	const dimmparams_t *p)
{
	printf("DDR: memctl=%u, dimm=%u: present = %u, nranks = %u, "
		"rowbits = %u, colbits = %u, bankbits = %u, regdimm = %u\n",
		memctl, dimm,
		p->present,
		p->nranks,
		p->rowbits,
		p->colbits,
		p->bankbits + 2,
		p->regdimm);
}
#endif



typedef struct {
	unsigned short memclk_mhz_low;
	unsigned short memclk_mhz_high;
	unsigned char nranks;
	unsigned char nchips_per_rank;
	unsigned char clk_adjust;
	unsigned char cpo;
	unsigned char write_data_delay;
} board_specific_parameters_t;

unsigned char compute_clk_adjust(
	unsigned int memctl,
	unsigned int nranks,
	unsigned int nchips_per_rank,
	unsigned int memclk_mhz);

unsigned char compute_write_data_delay(
	unsigned int memctl,
	unsigned int nranks,
	unsigned int nchips_per_rank,
	unsigned int memclk_mhz);

unsigned char compute_cpo(
	unsigned int memctl,
	unsigned int nranks,
	unsigned int nchips_per_rank,
	unsigned int memclk_mhz);

unsigned char compute_2T(
	unsigned int memctl,
	unsigned int nranks,
	unsigned int nchips_per_rank,
	unsigned int memclk_mhz);

#ifdef SPD_DEBUG
static void spd_debug(spd_eeprom_t *spd)
{
	printf ("\nDIMM type:       %-18.18s\n", spd->mpart);
	printf ("SPD size:        %d\n", spd->info_size);
	printf ("EEPROM size:     %d\n", 1 << spd->chip_size);
	printf ("Memory type:     %d\n", spd->mem_type);
	printf ("Row addr:        %d\n", spd->nrow_addr);
	printf ("Column addr:     %d\n", spd->ncol_addr);
	printf ("# of rows:       %d\n", spd->nrows);
	printf ("Row density:     %d\n", spd->row_dens);
	printf ("# of banks:      %d\n", spd->nbanks);
	printf ("Data width:      %d\n",
			256 * spd->dataw_msb + spd->dataw_lsb);
	printf ("Chip width:      %d\n", spd->primw);
	printf ("Refresh rate:    %02X\n", spd->refresh);
	printf ("CAS latencies:   %02X\n", spd->cas_lat);
	printf ("Write latencies: %02X\n", spd->write_lat);
	printf ("tRP:             %d\n", spd->trp);
	printf ("tRCD:            %d\n", spd->trcd);
	printf ("\n");
}
#endif

/*
 * Convert picoseconds into clock cycles (rounding up if needed).
 */

static unsigned int picos_to_clk(unsigned int picos)
{
	/* use unsigned long long to avoid rounding errors */
	const unsigned long long ULL_2e12 = 2000000000000ULL;
	unsigned long long clks;
	unsigned long long clks_temp;

	if (! picos)
	    return 0;

	clks = get_ddr_freq(0) * (unsigned long long) picos;
	clks_temp = clks;
	clks = clks / ULL_2e12;
	if (clks_temp % ULL_2e12) {
		clks++;
	}

	if (clks > 0xFFFFFFFFULL) {
		clks = 0xFFFFFFFFULL;
	}

	return (unsigned int) clks;
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
	unsigned int bsize = 0;

	switch (mem_type) {
	case SPD_MEMTYPE_DDR:
		/* Bottom 2 bits up to the top. */
		bsize = ((row_dens >> 2) | ((row_dens & 3) << 6)) << 24;
		break;

	case SPD_MEMTYPE_DDR2:
		/* Bottom 5 bits up to the top. */
		bsize = ((row_dens >> 5) | ((row_dens & 31) << 3)) << 27;
		break;

	default:
		break;
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
		0,	/* undefined */
		0	/* undefined */
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
	unsigned int refresh_time_ps[8] = {
		15625000,	/* 0 Normal    1.00x */
		3900000,	/* 1 Reduced    .25x */
		7800000,	/* 2 Extended   .50x */
		31300000,	/* 3 Extended  2.00x */
		62500000,	/* 4 Extended  4.00x */
		125000000,	/* 5 Extended  8.00x */
		15625000,	/* 6 Normal    1.00x  filler */
		15625000,	/* 7 Normal    1.00x  filler */
	};

	debug ("determine_refresh_rate: spd_refresh = %u\n", spd_refresh);
	return picos_to_clk(refresh_time_ps[spd_refresh & 0x7]);
}


static unsigned int compute_cs_config(unsigned int memctl_intlv_en,
	unsigned int memctl_intlv_ctl, const dimmparams_t *p)
{
	/* computes the values except for the ODT_RD_CFG and ODT_WR_CFG */

	unsigned int config = (1 << 31
			    | (memctl_intlv_en << 29)
			    | (memctl_intlv_ctl << 24)
			    | (p->bankbits << 14)
			    | (p->rowbits - 12) << 8
			    | (p->colbits - 8));
	return config;
}

static unsigned int compute_cs_bnds(unsigned long start, unsigned int end)
{


	start &= ~0xFFFFFF;
	end   &= ~0xFFFFFF;

	return (start >> 8) | ((end - 1) >> 24);

}

/* spd_cs_init()
 *
 * Set up CS_CONFIG and CS_BNDS registers, except
 * the ODT_RD_CFG and ODT_WR_CFG fields.
 * Returns total memory found on this controller. */
unsigned long spd_cs_init(
	volatile ccsr_ddr_t *ddr,
	const spd_eeprom_t *spd,
	const dimmparams_t *dimmparams,
	unsigned int memctl,
	unsigned int memctl_intlv_en,
	unsigned int memctl_intlv_ctl,
	unsigned int cs_intlv_en,
	unsigned int cs_intlv_ctl,
	const unsigned long memctl_start_addr,
	const unsigned int nranks)
{

	unsigned int i;
	unsigned long rank_size[CONFIG_NUM_DIMMS_PER_DDR * 2];

	unsigned long size;
	unsigned long total_mem_this_controller = 0;
	unsigned long mem_addr = memctl_start_addr;

	/* compute the size of each rank */

	if (nranks == 0)
		return total_mem_this_controller;

	debug("DDR: memctl=%u, memctl_start_addr = 0x%08X\n", memctl,
		memctl_start_addr);

	for (i = 0; i < CONFIG_NUM_DIMMS_PER_DDR; i++) {
		unsigned char present_memtype = dimmparams[i].present;
		size = compute_banksize(present_memtype, spd[i].row_dens);
		debug("DDR: memctl=%u, dimm=%u, CS%u rank size = 0x%08X\n",
			memctl, i, 2*i, size);
		rank_size[2*i] = size;
		if (dimmparams[i].nranks == 2) {
			debug("DDR: memctl=%u, dimm=%u, CS%u rank size = "
				"0x%08X\n", memctl, i, 2*i+1, size);
			rank_size[2*i+1] = size;
		} else {
			rank_size[2*i+1] = 0;
		}
	}

	debug("DDR: memctl=%u total nranks = %u\n", memctl, nranks);


	/* compute and program chip select address boundaries depending upon
	 * the combinations of interleaving mode */

#if CONFIG_NUM_DDR_CONTROLLERS  > 1
	if (cs_intlv_en && memctl_intlv_en) {
		/* MODE:  chip-select interleaving enabled with memory
		 * controller interleaving. */

		/* Boundary addresses for entire system memory need to be
		 * programmed identically into the CS0_BNDS register on each
		 * controller. */

		/* CS0 must be present, and this is checked for outside of
		 * this function. */
		/* XXX: rank_size is assumed to be the same for all ranks. */
		/* XXX: assumes earlier checks for this support have been
		 * 	done. */
		/* XXX: "2x2" chip-select interleaving not supported. */
		/* XXX: Assume there are 2 controllers in system if being
		 * called with this mode. */

		size = nranks * rank_size[0];

		ddr->cs0_bnds = compute_cs_bnds(mem_addr, mem_addr + 2 * size);
		ddr->cs1_bnds = 0;
		ddr->cs2_bnds = 0;
		ddr->cs3_bnds = 0;
		mem_addr += 2 * size;
		total_mem_this_controller = size;  /* doesn't really matter */

		ddr->cs0_config = compute_cs_config(memctl_intlv_en,
			memctl_intlv_ctl, &(dimmparams[0]));
		ddr->cs1_config = 0;
		ddr->cs2_config = 0;
		ddr->cs3_config = 0;
	}
#endif

	if (cs_intlv_en && !memctl_intlv_en) {
		/* MODE:  chip select interleaving without memory controller
		 * interleaving. */

		/* Bounds for the range of each rank-tuple (2 or 4 ranks)
		 * needs to be programmed into the lowest numbered rank of
		 * that tuple.
		 * DIMM0: CS0+CS1 => CS0_BNDS		"2x1 lower"
		 * DIMM1: CS2+CS3 => CS2_BNDS		"2x1 upper"
		 * CS0+CS1 => CS0_BNDS, CS2+CS3=> CS2_BNDS	"2x2"
		 * CS0+CS1+CS2+CS3 => CS0_BNDS	"4x1"
		 */

		/* XXX: rank_size assumed to be the same for ranks
		 * interleaved together, specifically, that #rows, #banks,
		 * #cols is the same on all ranks */
		/* XXX: assumes checks for this have been done earlier */

#if CONFIG_NUM_DIMMS_PER_DDR == 2
		if (cs_intlv_ctl == 0x04) { /* DIMM0+DIMM1: CS0+CS1+CS2+CS3 */
			size = 4 * rank_size[0];
			debug("DDR: 0x04: mem_addr=0x%08X -> "
			  "mem_addr+size=0x%08X\n", mem_addr, mem_addr + size);
			ddr->cs0_bnds =
				compute_cs_bnds(mem_addr, mem_addr + size);
			ddr->cs1_bnds = 0;
			ddr->cs2_bnds = 0;
			ddr->cs3_bnds = 0;
			mem_addr += size;
			total_mem_this_controller += size;

			ddr->cs0_config = compute_cs_config(memctl_intlv_en,
				memctl_intlv_ctl, &(dimmparams[0]));
			ddr->cs1_config = 0;
			ddr->cs2_config = 0;
			ddr->cs3_config = 0;
		} else {
#endif
		if (cs_intlv_ctl & 0x40) { /* DIMM0: CS0+CS1 */
			size = 2 * rank_size[0];

			ddr->cs0_bnds = compute_cs_bnds(mem_addr, mem_addr +
				size);
			ddr->cs1_bnds = 0;

			ddr->cs0_config = compute_cs_config(memctl_intlv_en,
				memctl_intlv_ctl, &(dimmparams[0]));
			ddr->cs1_config = 0;
		} else {
			/* If there's memory in DIMM0 but it was not requested
			 * to be chip-select interleaved, these registers still
			 * need to be set up.
			 */
			size = rank_size[0];
			if (size) {
				ddr->cs0_bnds = compute_cs_bnds(mem_addr,
					mem_addr + size);
				udelay(2000);  /* 2ms */
				ddr->cs0_config = compute_cs_config(
					memctl_intlv_en, memctl_intlv_ctl,
					&(dimmparams[0]));
				udelay(2000);  /* 2ms */
				mem_addr += size;
				total_mem_this_controller += size;
			}
			size = rank_size[1];
			if (size) {
				ddr->cs1_bnds = compute_cs_bnds(mem_addr,
					mem_addr + size);
				ddr->cs1_config = compute_cs_config(
					memctl_intlv_en, memctl_intlv_ctl,
					&(dimmparams[0]));
			}
		}
		mem_addr += size;
		total_mem_this_controller += size;

#if CONFIG_NUM_DIMMS_PER_DDR == 2
		if (cs_intlv_ctl & 0x20) {	/* DIMM1: CS2+CS3 */
			size = 2 * rank_size[2];
			ddr->cs2_bnds = compute_cs_bnds(mem_addr,
				mem_addr + size);
			ddr->cs3_bnds = 0;

			ddr->cs2_config = compute_cs_config(0, 0,
				&(dimmparams[1]));
			ddr->cs3_config = 0;
		} else {
			/* If there's memory in DIMM0 but it was not requested
			 * to be chip-select interleaved, these registers still
			 * need to be set up.
			 */
			size = rank_size[2];
			if (size) {
				ddr->cs2_bnds = compute_cs_bnds(mem_addr,
					mem_addr + size);
				ddr->cs2_config = compute_cs_config(
					memctl_intlv_en, memctl_intlv_ctl,
					&(dimmparams[1]));
				mem_addr += size;
				total_mem_this_controller += size;
			}
			size = rank_size[3];
			if (size) {
				ddr->cs3_bnds = compute_cs_bnds(mem_addr,
					mem_addr + size);
				ddr->cs3_config = compute_cs_config(
					memctl_intlv_en, memctl_intlv_ctl,
					&(dimmparams[1]));
			}
		}
		mem_addr += size;
		total_mem_this_controller += size;

		}
#endif
	}

#if CONFIG_NUM_DDR_CONTROLLERS > 1
	if (!cs_intlv_en && memctl_intlv_en) {
		/* MODE:  memory controller interleaving without chip select
		 * interleaving */

		/* Only the rank on CS0 is used on both controllers; all
		 * other ranks are not used.  */

		/* Boundary addresses for entire system memory need to be
		 * programmed identically into the CS0_BNDS register on each
		 * controller. */

		size = rank_size[0];
		ddr->cs0_bnds = compute_cs_bnds(mem_addr, mem_addr + 2 * size);
		ddr->cs1_bnds = 0;
		ddr->cs2_bnds = 0;
		ddr->cs3_bnds = 0;

		mem_addr += 2 * size;
		total_mem_this_controller += size;

		ddr->cs0_config = compute_cs_config(memctl_intlv_en,
			memctl_intlv_ctl, &(dimmparams[0]));
		ddr->cs1_config = 0;
		ddr->cs2_config = 0;
		ddr->cs3_config = 0;
	}
#endif

	if (!cs_intlv_en && !memctl_intlv_en) {
		/* MODE:  no interleaving enabled */

		if (rank_size[0]) {
			size = rank_size[0];
			ddr->cs0_bnds = compute_cs_bnds(mem_addr,
				mem_addr + size);

			mem_addr += size;
			total_mem_this_controller += size;

			ddr->cs0_config = compute_cs_config(memctl_intlv_en,
				memctl_intlv_ctl, &(dimmparams[0]));
		} else {
			ddr->cs0_bnds = 0;
			ddr->cs0_config = 0;
		}

		if (rank_size[1]) {
			size = rank_size[1];
			ddr->cs1_bnds = compute_cs_bnds(mem_addr,
				mem_addr + size);

			mem_addr += size;
			total_mem_this_controller += size;

			ddr->cs1_config = compute_cs_config(0, 0,
				&(dimmparams[0]));
		} else {
			ddr->cs1_bnds = 0;
			ddr->cs1_config = 0;
		}

#if CONFIG_NUM_DIMMS_PER_DDR > 1
		if (rank_size[2]) {
			size = rank_size[2];
			ddr->cs2_bnds = compute_cs_bnds(mem_addr,
				mem_addr + size);

			mem_addr += size;
			total_mem_this_controller += size;

			ddr->cs2_config = compute_cs_config(0, 0,
				&(dimmparams[1]));
		} else {
			ddr->cs2_bnds = 0;
			ddr->cs2_config = 0;
		}

		if (rank_size[3]) {
			size = rank_size[3];
			ddr->cs3_bnds = compute_cs_bnds(mem_addr,
				mem_addr + size);

			mem_addr += size;
			total_mem_this_controller += size;

			/* XXX: could just copy ddr->cs2_config ... */
			ddr->cs3_config = compute_cs_config(0, 0,
				&(dimmparams[1]));
		} else {
			ddr->cs3_bnds = 0;
			ddr->cs3_config = 0;
		}
#endif
	}

	return total_mem_this_controller;
}


/* program timing and configuration parameters */
/* return 1 if there is an error */

unsigned int spd_timing_init(
	volatile ccsr_ddr_t *ddr,
	unsigned int mem_type,
	const spd_eeprom_t *spd,
	const dimmparams_t *dimmparams,
	unsigned int memctl,
	const unsigned int nranks,
	const unsigned int ndimms)
{
	unsigned int i;
	unsigned int odt_rd_cfg = 0;
	unsigned int odt_wr_cfg = 0;
	unsigned int odt_cfg, mode_odt_enable = 0;
	unsigned int refresh_clk;
#ifdef MPC85xx_DDR_SDRAM_CLK_CNTL
	unsigned char clk_adjust;
#endif
	unsigned int dqs_cfg;
	unsigned char twr_clk, twtr_clk, twr_auto_clk;
	unsigned int busfreq = (get_ddr_freq(0) + 500000) / 1000000; /* MHz */
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
	unsigned char d_init;
	unsigned int tCK_ps = (unsigned int)(2000000000000ULL/get_ddr_freq(0));
	unsigned int ranks_per_dimm[CONFIG_NUM_DIMMS_PER_DDR];
	unsigned int num_pr = 1;	/* XXX: how do you calculate this? */

	if (!nranks) {
		debug("DDR: no DIMMs on controller %u\n", memctl);
		return 1;
	}

	/* round tCK_ps to nearest 10ps to work around precision problems. */

	tCK_ps = 10 * ((tCK_ps + 5) / 10);

	debug("DDR: tCK_ps = %u\n", tCK_ps);

	for (i = 0; i < CONFIG_NUM_DIMMS_PER_DDR; i++)
		ranks_per_dimm[i] =
			dimmparams[i].present ? dimmparams[i].nranks : 0;

	/*
	 * Find the largest CL of all DIMMs by locating the
	 * highest 1 bit in each of the spd.cas_lat fields.
	 * Translate it to a DDR controller timing_cfg_1 value:
	 *
	 *	CAS Lat	DDR I	DDR II	TIMING_CFG_1[CAS_LAT]
	 *	Clocks	SPD Bit	SPD Bit	Value
	 *	-------	-------	-------	-----
	 *	1.0	0		0001
	 *	1.5	1		0010
	 *	2.0	2	2	0011
	 *	2.5	3		0100
	 *	3.0	4	3	0101
	 *	3.5	5		0110
	 *	4.0	6	4	0111
	 *	4.5			1000
	 *	5.0		5	1001
	 *	5.5                     1010
	 *	6.0             6       1011
	 *	6.5                     1100
	 *	7.0             7       1101
	 *	7.5                     1110
	 *	8.0             8       1111
	 */

	{
		unsigned int caslat_X_minus[CONFIG_NUM_DIMMS_PER_DDR][3];
		unsigned int tCKmin_ps_X_minus[CONFIG_NUM_DIMMS_PER_DDR][3];
		unsigned int lowest_good_caslat = 0xFF;
		unsigned int temp1 = 0xFF;
		unsigned int temp2 = 0;
		unsigned int not_ok;
		unsigned int j;

		/* Find lowest CAS latency supported by all present DIMMs */

		/* Step 1: find CAS latency common to all DIMMs using bitwise
		 * operation */
		for (i = 0; i < CONFIG_NUM_DIMMS_PER_DDR; i++) {
			/* bitwise mask cas_lat to find a latency
			 * supported by all of the present DIMMs */
			if (dimmparams[i].present) {
				debug("DDR: memctl=%u, dimm=%u caslat "
					"bitmask=0x%02X\n", memctl, i,
					spd[i].cas_lat);
				temp1 &= spd[i].cas_lat;

				/* Note: __ilog2() returns 0xFF if its input
				 * is 0 */

				/* longest latency */
				caslat_X_minus[i][0] = __ilog2(spd[i].cas_lat);
				caslat_X_minus[i][1] = __ilog2(spd[i].cas_lat &
					~(1 << caslat_X_minus[i][0]));

				/* shortest latency */
				caslat_X_minus[i][2] = __ilog2(spd[i].cas_lat &
					~(1 << caslat_X_minus[i][0]) &
					~(1 << caslat_X_minus[i][1]));



				/* calculate minimum clock cycles */

				/* fastest clock */
				tCKmin_ps_X_minus[i][0] =
					convert_bcd_tenths_to_cycle_time_ps(
					spd[i].clk_cycle);

				tCKmin_ps_X_minus[i][1] =
					convert_bcd_tenths_to_cycle_time_ps(
					spd[i].clk_cycle2);

				/* slowest clock */
				tCKmin_ps_X_minus[i][2] =
					convert_bcd_tenths_to_cycle_time_ps(
					spd[i].clk_cycle3);
			}
		}

		debug("DDR: memctl=%u, common caslat bitmask=0x%02X\n",
			memctl, temp1);

		/* Step 2: check each common CAS latency from highest
		 * to lowest against tCKmin of each DIMM */

		while (temp1) {
			not_ok = 0;

			/* pick highest current common latency */
			temp2 =  __ilog2(temp1);
			debug("DDR: testing common caslat = %u\n", temp2);

			/* Check if this CAS latency will work on all DIMMs
			 * at tCK.  Basic rule: for any tCK_ps >=
			 * tCKmin_ps_X_minus[i], caslat_X_minus[i] is OK */

			for (i = 0; i < CONFIG_NUM_DIMMS_PER_DDR; i++) {
				if (!dimmparams[i].present)
					continue;

				for (j = 0; j < 3; j++) {
					if (caslat_X_minus[i][j] == temp2) {

	if (tCK_ps >= tCKmin_ps_X_minus[i][j]) {
		debug("DDR: CL = %u ok on DIMM %u at tCK=%u ps with its "
		"tCKmin_X-%u_ps of %u ps\n", temp2, i, tCK_ps, j,
		tCKmin_ps_X_minus[i][j]);
	} else {
		debug("DDR: CL = %u NOT ok on DIMM %u at tCK=%u ps with "
		"its tCKmin_X-%u_ps of %u ps\n", temp2, i, tCK_ps, j,
		tCKmin_ps_X_minus[i][j]);
		not_ok = 1;
	}

					}
				}
			}

			/* this latency was good on something */
			if (!not_ok)
				lowest_good_caslat = temp2;

			/* try next lowest latency on next iteration */
			temp1 &= ~(1 << temp2);

		}

		if (lowest_good_caslat == 0xFF) {
			printf("DDR: On memory controller %u, there were "
				"no supported latencies common to all of "
				"the DIMMs found.\n", memctl);
			return 1;
		}


		debug("DDR: controller %u: lowest common SPD-defined CAS "
			"latency = %u\n", memctl, lowest_good_caslat);
		debug("C%u:CL%u ", memctl, lowest_good_caslat);
		caslat = lowest_good_caslat;
	}

	/*
	 * Empirically set ~MCAS-to-preamble override for DDR 2.
	 * Your milage will vary.  This has to change for each board
	 * design, and it depends on not just the frequency.  See tables in
	 * the board-specific file board/freescale/<boardname>/<boardname>.c
	 */
	cpo = 0;
	if (mem_type == SPD_MEMTYPE_DDR2)
		cpo = compute_cpo(memctl, nranks, 0, busfreq);

	/*
	 * Convert caslat clocks to DDR controller value.
	 * Force caslat_ctrl to be DDR Controller field-sized.
	 */
	if (mem_type == SPD_MEMTYPE_DDR) {
		caslat_ctrl = (caslat + 1) & 0x07;
	} else {
		caslat_ctrl = (2 * caslat - 1) & 0x0f;
	}

	debug("DDR: caslat SPD MR bitvalue is %d, controller field is 0x%x\n",
	      caslat, caslat_ctrl);

	/*
	 * Timing Config 0.
	 * Avoid writing for DDR I.  The new PQ38 DDR controller
	 * dreams up non-zero default values to be backwards compatible.
	 */
	if (mem_type == SPD_MEMTYPE_DDR2) {
		unsigned char taxpd_clk = 8;		/* By the book. */
		unsigned char tmrd_clk = 2;		/* By the book. */
		unsigned char act_pd_exit = 2;		/* Empirical? */
		unsigned char pre_pd_exit = 6;		/* Empirical? */

		ddr->timing_cfg_0 = (0
			| ((act_pd_exit & 0x7) << 20)	/* ACT_PD_EXIT */
			| ((pre_pd_exit & 0x7) << 16)	/* PRE_PD_EXIT */
			| ((taxpd_clk & 0xf) << 8)	/* ODT_PD_EXIT */
			| ((tmrd_clk & 0xf) << 0)	/* MRS_CYC */
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

	if (mem_type == SPD_MEMTYPE_DDR) {
		twr_clk = 3;	/* Clocks */
		twtr_clk = 1;	/* Clocks */
	} else {
		unsigned int worst_twr = 0;
		unsigned int worst_twtr = 0;

		for (i = 0; i < CONFIG_NUM_DIMMS_PER_DDR; i++) {
			if (dimmparams[i].present) {
				twr_clk = picos_to_clk(spd[i].twr * 250);
				worst_twr = MAX(twr_clk, worst_twr);

				twtr_clk = picos_to_clk(spd[i].twtr * 250);
				worst_twtr = MAX(twtr_clk, worst_twtr);
			}
		}

		twr_clk = worst_twr;
		twtr_clk = worst_twtr;
		twtr_clk = MAX(2, twtr_clk);	/* tWTR minimum must be 2 cycles */
	}

	/*
	 * Calculate Trfc, in picos.
	 * DDR I:  Byte 42 straight up in ns.
	 * DDR II: Byte 40 and 42 swizzled some, in ns.
	 */
	if (mem_type == SPD_MEMTYPE_DDR) {
		unsigned int spd_worst_trfc = 0;
		for (i = 0; i < CONFIG_NUM_DIMMS_PER_DDR; i++) {
			if (dimmparams[i].present)
				spd_worst_trfc =
					MAX(spd[i].trfc, spd_worst_trfc);
		}

		trfc = spd_worst_trfc * 1000;		/* up to ps */
	} else {
		unsigned int worst_trfc = 0;
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

		for (i = 0; i < CONFIG_NUM_DIMMS_PER_DDR; i++) {
			if (dimmparams[i].present) {
				trfc = (((spd[i].trctrfc_ext & 0x1) * 256) +
				spd[i].trfc) * 1000 +
				byte40_table_ps[(spd[i].trctrfc_ext>>1) & 0x7];
				worst_trfc = MAX(trfc, worst_trfc);
			}
		}
		trfc = worst_trfc;
	}
	trfc_clk = picos_to_clk(trfc);

	/*
	 * Trcd, Byte 29, from quarter nanos to ps and clocks.
	 */
	{
		unsigned int worst_trcd_clk = 0;
		for (i = 0; i < CONFIG_NUM_DIMMS_PER_DDR; i++) {
			if (dimmparams[i].present) {
				trcd_clk = picos_to_clk(spd[i].trcd*250) & 0x7;
				worst_trcd_clk = MAX(worst_trcd_clk, trcd_clk);
			}
		}
		trcd_clk = worst_trcd_clk;
	}

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

	{
		unsigned int worst_trp_ps = 0;
		unsigned int worst_tras_ps = 0;
		unsigned int worst_trrd_ps = 0;
		unsigned int trp_ps = 0;
		unsigned int tras_ps = 0;
		unsigned int trrd_ps = 0;
		unsigned int acttoact;

		for (i = 0; i < CONFIG_NUM_DIMMS_PER_DDR; i++) {
			if (dimmparams[i].present) {
				trp_ps  = spd[i].trp * 250;
				tras_ps = spd[i].tras * 1000;
				trrd_ps = spd[i].trrd * 250;

				worst_trp_ps = MAX(trp_ps, worst_trp_ps);
				worst_tras_ps = MAX(tras_ps, worst_tras_ps);
				worst_trrd_ps = MAX(trrd_ps, worst_trrd_ps);
			}
		}

		debug("DDR: worst_trp_ps = %u ps\n", worst_trp_ps);
		debug("DDR: worst_tras_ps = %u ps\n", worst_tras_ps);
		debug("DDR: worst_trrd_ps = %u ps\n", worst_trrd_ps);

		acttoact = MAX(picos_to_clk(worst_trrd_ps) & 0x07, 2);

	    ddr->timing_cfg_1 =
	    (0
	     | ((picos_to_clk(worst_trp_ps) & 0x07) << 28)	/* PRETOACT */
	     | ((picos_to_clk(worst_tras_ps) & 0x0f) << 24)	/* ACTTOPRE */
	     | (trcd_clk << 20)					/* ACTTORW */
	     | (caslat_ctrl << 16)				/* CASLAT */
	     | (trfc_low << 12)					/* REFEC */
	     | ((twr_clk & 0x07) << 8)				/* WRRREC */
	     | (acttoact << 4)					/* ACTTOACT */
	     | ((twtr_clk & 0x07) << 0)				/* WRTORD */
	     );
	}

	debug("DDR: timing_cfg_1  = 0x%08x\n", ddr->timing_cfg_1);


	/*
	 * Timing_Config_2
	 * Was: 0x00000800;
	 */

	/*
	 * ODT configuration for unbuffered DDR2
	 *
	 * Micron Technology technotes TN-47-01 and TN4707, and
	 * Samsung appnote an_ddr2_odt_control_20041228.pdf
	 * contain tables describing when ODT should be asserted
	 * and to which ranks it should be asserted for given
	 * reads or writes to certain DIMM slots.  The contents of
	 * those tables can be distilled down to the following
	 * two tables:
	 *
	 *
	 * 1 slot populated:
	 *               |          Termination Resistance
	 * Operation     | Controller | DIMM 1 rank 1 | DIMM 2 rank 1
	 * --------------+------------+---------------+---------------
	 * Read slot 1   | 75 ohm     | infinite      | -
	 * Read slot 2   | 75 ohm     | -             | infinite
	 * Write slot 1  | infinite   | 150 ohm       | -
	 * Write slot 2  | infinite   | -             | 150 ohm
	 *
	 * 2 slots populated:
	 *
	 *               |          Termination Resistance
	 * Operation     | Controller | DIMM 1 rank 1 | DIMM 2 rank 1
	 * --------------+------------+---------------+---------------
	 * Read slot 1   | 150 ohm    | infinite      | 75 ohm
	 * Read slot 2   | 150 ohm    | 75 ohm        | infinite
	 * Write slot 1  | infinite   | infinite      | 75 ohm
	 * Write slot 2  | infinite   | 75 ohm        | infinite
	 *
	 * Note: the DIMM vendor's docs use ordinal 1 for numbering DIMM
	 * slots and ranks, so CS0=>DIMM1.rank1, CS1=>DIMM1.rank2, etc.
	 *
	 * Note: ODT off == ODT not asserted == infinite resistance
	 *
	 * Note: All DIMM's rank 2s should be infinite resistance (i.e.,
	 * CSn_CONFIG[ODT_RD_CFG] = 0 and CSn_CONFIG[ODT_WR_CFG] = 0
	 * for n = 1 and n = 3.)  See code below on actual settings
	 * to get the behavior described above.
	 *
	 * The MPC8572 memory controller's ODT termination resistance
	 * is configured using the concatenation of DDRCDR_1[ODT] and
	 * DDRCDR_2[ODT].  The two releavant values are 75 ohm and 150 ohm.
	 *
	 * ODT termination | DDRCDR_1[ODT] | DDRCDR_2[ODT]
	 * ----------------+---------------+---------------
	 * 75 ohm          | 0             | 0
	 * 150 ohm         | 2             | 0
	 * XXX: is this correct?  or is it:
	 * 150 ohm         | 0             | 1
	 *
	 * For reference, these are the DDR2 Mode Register values for
	 * controlling ODT at the SDRAM:
	 *
	 * SPD EMRS ODT control bits:
	 * E6 E2 | R_TT (Nominal) | mode_odt_enable | speed bin notes (tCK)
	 * ------+----------------+------++++++++++++-----------------------
	 *  0  0 | R_TT disabled  | 0x00 |
	 *  0  1 |    75 ohm      | 0x04 | 2-slot DDR2-400/533 (5 ns, 3.75 ns)
	 *  1  0 |   150 ohm      | 0x40 | one-slot, any
	 *  1  1 |    50 ohm      | 0x44 | 2-slot DDR2-667 (3 ns)
	 */

	if (mem_type == SPD_MEMTYPE_DDR2) {
		if (ndimms == 1) {
			/* If only 1 DIMM is inserted... */

#if 1
			odt_rd_cfg = 0;	/* Never assert ODT for reads */
			odt_wr_cfg = 4;	/* Assert ODT for writes to anything */
			mode_odt_enable = 0x40;	/* 150 Ohm */
#else
			odt_rd_cfg = 0;	/* Never assert ODT for reads */
			odt_wr_cfg = 1;	/* Assert ODT to active rank */
			mode_odt_enable = 0x40;	/* 150 Ohm */
#endif

			/* DIMM vendors expect 75 ohm for the
			 * controller resistance for 1 DIMM.   */
			ddr->ddrcdr_1 &= ~0xC0000;
			ddr->ddrcdr_2 &= ~1;
		} else {
			/* If 2 DIMMs are inserted... */

			odt_rd_cfg = 3;	/* Assert ODT to this CS
				if a read goes to the other slot. */
			odt_wr_cfg = 3;	/* Assert ODT to this CS
				if a write goes to the other slot. */

			/* The Samsung appnote suggests 50 ohm for 667 MHz
			 * (3000 ps) and 75 ohm for 533 MHz (3750 ps).
			 * Unfortunately, 600 MHz (3333 ps) falls exactly
			 * in between 533 and 667.  Assume that 75 ohm is
			 * OK for 600 MHz, switch to 50 ohm for tCK less
			 * than 3100ps, i.e. ~645 MHz data rate or higher
			 */
			if (tCK_ps < 3100) {
				mode_odt_enable = 0x44;	/* 50 Ohm */
			} else {
				mode_odt_enable = 0x04;	/* 75 Ohm */
			}

			/* DIMM vendors expect 150 ohm for the
			 * controller resistance for 2 DIMMs.   */
			ddr->ddrcdr_1 = (ddr->ddrcdr_1 & ~0xC0000) | 0x80000;
			ddr->ddrcdr_2 &= ~1;	/* XXX: is this correct? */
		}

		/* Fixup CS regs with ODT config values.  Write them all;
		 * if the config reg is not enabled it doesn't matter.  This
		 * pattern will cause ODT to only assert on the "front side"
		 * of each DIMM, if it's present.
		 */
#if 1
		ddr->cs0_config |= ((odt_rd_cfg << 20) | (odt_wr_cfg << 16));
		ddr->cs1_config |= ((0          << 20) | (0          << 16));
		ddr->cs2_config |= ((odt_rd_cfg << 20) | (odt_wr_cfg << 16));
		ddr->cs3_config |= ((0          << 20) | (0          << 16));
#else
		ddr->cs0_config |= ((odt_rd_cfg << 20) | (odt_wr_cfg << 16));
		ddr->cs1_config |= ((odt_rd_cfg << 20) | (odt_wr_cfg << 16));
		ddr->cs2_config |= ((odt_rd_cfg << 20) | (odt_wr_cfg << 16));
		ddr->cs3_config |= ((odt_rd_cfg << 20) | (odt_wr_cfg << 16));
#endif
	}

	debug("DDR: cs0_bnds   = 0x%08x\n", ddr->cs0_bnds);
	debug("DDR: cs0_config = 0x%08x\n", ddr->cs0_config);
	debug("DDR: cs1_bnds   = 0x%08x\n", ddr->cs1_bnds);
	debug("DDR: cs1_config = 0x%08x\n", ddr->cs1_config);
	debug("DDR: cs2_bnds   = 0x%08x\n", ddr->cs2_bnds);
	debug("DDR: cs2_config = 0x%08x\n", ddr->cs2_config);
	debug("DDR: cs3_bnds   = 0x%08x\n", ddr->cs3_bnds);
	debug("DDR: cs3_config = 0x%08x\n", ddr->cs3_config);

	/*
	 * Additive Latency
	 * For DDR I, 0.
	 * For DDR II, with ODT enabled, use "a value" less than ACTTORW,
	 * which comes from Trcd, and also note that:
	 *	add_lat + caslat must be >= 4
	 */
	add_lat = 0;
	if (mem_type == SPD_MEMTYPE_DDR2
	    && (odt_wr_cfg || odt_rd_cfg)
	    && (caslat < 4)) {
		add_lat = 4 - caslat;
		if (add_lat >= trcd_clk)
			add_lat = trcd_clk - 1;
	}

	/*
	 * Write Data Delay
	 * Historically 0x2 == 4/8 clock delay.
	 * Empirically, 0x3 == 6/8 clock delay is suggested for DDR I 266.
	 * This is a parameter that must be changed based on the memory
	 * organization and frequency.  It should not be hard coded.
	 */

	wr_data_delay = compute_write_data_delay(memctl, nranks, 0, busfreq);
	debug("DDR: wr_data_delay = %u\n", wr_data_delay);

	/*
	 * Write Latency
	 * Read to Precharge
	 * Minimum CKE Pulse Width.
	 * Four Activate Window
	 */
	if (mem_type == SPD_MEMTYPE_DDR) {
		/*
		 * This is a lie.  It should really be 1, but if it is
		 * set to 1, bits overlap into the old controller's
		 * otherwise unused ACSM field.  If we leave it 0, then
		 * the HW will magically treat it as 1 for DDR 1.  Oh Yea.
		 */
		wr_lat = 0;

		trtp_clk = 2;		/* By the book. */
		cke_min_clk = 1;	/* By the book. */
		four_act = 1;		/* By the book. */

	} else {
		unsigned int worst_trtp = 0;
		wr_lat = caslat - 1;

		for (i = 0; i < CONFIG_NUM_DIMMS_PER_DDR; i++) {
		/* Convert SPD value from quarter nanos to picos. */
			trtp_clk = picos_to_clk(spd[i].trtp * 250);
			worst_trtp = MAX(worst_trtp, trtp_clk);
		}
		trtp_clk = MAX(worst_trtp, 2);

		cke_min_clk = 3;	/* By the book. */
		four_act = picos_to_clk(37500);	/* By the book. 1k pages? */
	}

	debug("DDR: cpo = %u\n", cpo);
	ddr->timing_cfg_2 = (0
		| ((add_lat & 0x7) << 28)		/* ADD_LAT */
		| ((cpo & 0x1f) << 23)			/* CPO */
		| ((wr_lat & 0x7) << 19)		/* WR_LAT */
		| ((trtp_clk & 0x7) << 13)		/* RD_TO_PRE */
		| ((wr_data_delay & 0x7) << 10)		/* WR_DATA_DELAY */
		| ((cke_min_clk & 0x7) << 6)		/* CKE_PLS */
		| ((four_act & 0x1f) << 0)		/* FOUR_ACT */
		);

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
	if (mem_type == SPD_MEMTYPE_DDR) {
		if (1 <= caslat && caslat <= 4) {
			unsigned char mode_caslat_table[4] = {
				0x5,	/* 1.5 clocks */
				0x2,	/* 2.0 clocks */
				0x6,	/* 2.5 clocks */
				0x3	/* 3.0 clocks */
			};
			mode_caslat = mode_caslat_table[caslat - 1];
		} else {
			puts("DDR I: Only CAS Latencies of 1.5, 2.0, "
			     "2.5 and 3.0 clocks are supported.\n");
			return 1;
		}

	} else {
		if (2 <= caslat && caslat <= 6) {
			mode_caslat = caslat;
		} else {
			puts("DDR II: Only CAS Latencies of 2-6 "
			     "clocks are supported.\n");
			return 1;
		}
	}

	/*
	 * Encoded Burst Length of 4.
	 */
	burst_len = 2;			/* Fiat if using 64-bit memory. */

	if (mem_type == SPD_MEMTYPE_DDR) {
		twr_auto_clk = 0;	/* Historical */
	} else {
		/*
		 * Determine tCK max in picos.  Grab tWR and convert to picos.
		 * Auto-precharge write recovery is:
		 *	WR = roundup(tWR_ns/tCK_ns).
		 *
		 * Ponder: Is twr_auto_clk different than twr_clk?
		 */
		unsigned int worst_tWR_ps = 0;
		unsigned int tWR_ps;

		for (i = 0; i < CONFIG_NUM_DIMMS_PER_DDR; i++) {
			if (dimmparams[i].present) {
				tWR_ps = (spd[i].twr * 250);
				worst_tWR_ps = MAX(worst_tWR_ps, tWR_ps);
			}
		}

		tWR_ps = worst_tWR_ps;

		debug("tWR_ps = %u, tCK_ps = %u\n", tWR_ps , tCK_ps);

		/* Compute auto-precharge write recovery in cycles and
		 * round up to next integer if necessary. */
		twr_auto_clk = tWR_ps / tCK_ps;
		if (tWR_ps % tCK_ps)
			twr_auto_clk++;
		debug("DDR: wr recovery = %u\n", twr_auto_clk);

		/* MR encodes the auto-precharge write-recovery as 1
		 * less than the requested number of cycles. */
		twr_auto_clk--;
		debug("DDR: twr_auto_clk = %u + 1\n", twr_auto_clk);
	}

	/*
	 * Mode Reg in bits 16 ~ 31,
	 * Extended Mode Reg 1 in bits 0 ~ 15.
	 */

	/* mode_odt_enable is set above in ODT configuration for DDR2 . */

	ddr->sdram_mode =
		(0
		 | (add_lat << (16 + 3))	/* Additive Latency in EMRS1 */
		 | (mode_odt_enable << 16)	/* ODT Enable in EMRS1 */
		 | (twr_auto_clk << 9)		/* Write Recovery Autopre */
		 | (mode_caslat << 4)		/* caslat */
		 | (burst_len << 0)		/* Burst length */
		 );

	debug("DDR: ddr_sdram_mode = 0x%08x\n", ddr->sdram_mode);
	udelay(2000);  /* 2ms */

	/*
	 * Clear EMRS2 and EMRS3.
	 */
	ddr->sdram_mode_2 = 0;
	debug("DDR: sdram_mode_2 = 0x%08x\n", ddr->sdram_mode_2);

	/*
	 * Determine Refresh Rate.
	 */

	{
		unsigned int worst_refresh_clk = 0;
		for (i = 0; i < CONFIG_NUM_DIMMS_PER_DDR; i++) {
			if (dimmparams[i].present) {
				refresh_clk = determine_refresh_rate(
					spd[i].refresh & 0x7);
				worst_refresh_clk =
					MAX(worst_refresh_clk, refresh_clk);
			}
		}
		refresh_clk = worst_refresh_clk;
	}

	/*
	 * Set BSTOPRE to 0x100 for page mode
	 * If auto-charge is used, set BSTOPRE = 0
	 */
	ddr->sdram_interval =
		(0
		 | (refresh_clk & 0x3fff) << 16
		 | 0x100);
	debug("DDR: sdram_interval = 0x%08x\n", ddr->sdram_interval);


	/*
	 * Is this an ECC DDR DIMM according to byte 11?
	 * But don't mess with it if the DDR controller will init mem.
	 */
#if defined(CONFIG_DDR_ECC) && !defined(CONFIG_ECC_INIT_VIA_DDRCONTROLLER)
	{
		unsigned int all_ecc = 1;
		for (i = 0; i < CONFIG_NUM_DIMMS_PER_DDR; i++) {
			/* "Data ECC" */
			if (dimmparams[i].present && spd[i].config != 0x02)
				all_ecc = 0;
		}

		if (all_ecc) {
			ddr->err_disable = 0x0000000d;
			ddr->err_sbe = 0x00ff0000;
		}
	}

	debug("DDR: err_disable = 0x%08x\n", ddr->err_disable);
	debug("DDR: err_sbe = 0x%08x\n", ddr->err_sbe);
#endif

	/*
	 * SDRAM Cfg 2
	 */

	/*
	 * When ODT is enabled, Chap 9 suggests asserting ODT to
	 * the memory controller's internal IOs only during reads.
	 */
	odt_cfg = 0;
	if (odt_rd_cfg || odt_wr_cfg) {
		odt_cfg = 0x2;		/* ODT to IOs during reads */
	}

	/*
	 * Try to use differential DQS with DDR II.
	 */
	if (mem_type == SPD_MEMTYPE_DDR) {
		dqs_cfg = 0;		/* No Differential DQS for DDR I */
	} else {
		dqs_cfg = 0x1;		/* Differential DQS for DDR II */
	}

#if defined(CONFIG_ECC_INIT_VIA_DDRCONTROLLER)
	/*
	 * Use the DDR controller to auto initialize memory.
	 */
	d_init = 1;
	ddr->sdram_data_init = memctl ?
		CONFIG_MEM_INIT_VALUE : ~CONFIG_MEM_INIT_VALUE;
	debug("DDR: ddr_data_init = 0x%08x\n", ddr->sdram_data_init);
#else
	/*
	 * Memory will be initialized via DMA, or not at all.
	 */
	d_init = 0;
#endif

	ddr->sdram_cfg_2 = (0
			    | (dqs_cfg << 26)	/* Differential DQS */
			    | (odt_cfg << 21)	/* Controller's ODT config */
			    | (num_pr << 12)	/* Number of posted refresh */
			    | (d_init << 4)	/* D_INIT auto init DDR */
			    );

	debug("DDR: sdram_cfg_2  = 0x%08x\n", ddr->sdram_cfg_2);


#ifdef MPC85xx_DDR_SDRAM_CLK_CNTL
	/*
	 * Setup the clock control.
	 *
	 * SDRAM_CLK_CNTL[5-7] = Clock Adjust
	 *	0110	3/4 cycle late
	 *	0111	7/8 cycle late
	 *
	 * These parameters should not be hard coded here.
	 */
	if (mem_type == SPD_MEMTYPE_DDR)
		clk_adjust = 0x6;
	else {
		clk_adjust = compute_clk_adjust(memctl, nranks, 0, busfreq);
		debug("DDR: clk_adjust = %u\n", clk_adjust);
	}

	ddr->sdram_clk_cntl = clk_adjust << 23;
	debug("DDR: sdram_clk_cntl = 0x%08x\n", ddr->sdram_clk_cntl);
	udelay(2000);  /* 2ms */
#endif

	return 0;
}




/*
 * enable_ddr
 * Program sdram_cfg_1, err_disable, err_sbe, err_int_en (ECC),
 *
 * return non-zero on error
 */
unsigned int enable_ddr(
	volatile ccsr_ddr_t *ddr,
	unsigned int mem_type,
	unsigned int rd_en,
	unsigned int ecc_en,
	unsigned int cs_intlv_en,
	unsigned int cs_intlv_ctl,
	const dimmparams_t *dimmparams,
	unsigned int ndimms,
	unsigned int twoT_en)
{
	unsigned sdram_cfg_1;
	unsigned char sdram_type;

	debug("DDR: cs_intlv_en = %u, cs_intlv_ctl = 0x%X\n",
		cs_intlv_en, cs_intlv_ctl);

	/* XXX: should really only be checked once outside of this function */
	if (ndimms == 0)
		return 0;

#ifdef CONFIG_MX80
	/*
	 * ODT setting should be done before DDR_SDRAM_CFG[MEM_EN] is set.
	 */
	debug("DDR: setting ODT parameters\n");
	ddr->ddrcdr_1 = (  0 << 31 	/* DDR Driver Hardware compensation disable */
		         | 2 << 18	/* ODT = b'10 150 Ohm */
			);
	ddr->ddrcdr_2 = 0x00000000; 	/* ODT = 100 150 Ohm */
#endif
	/*
	 * Figure out the settings for the sdram_cfg register.
	 * Build up the entire register in 'sdram_cfg' before
	 * writing since the write into the register will
	 * actually enable the memory controller; all settings
	 * must be done before enabling.
	 *
	 * sdram_cfg[0]   = 1 (ddr sdram logic enable)
	 * sdram_cfg[1]   = 1 (self-refresh-enable)
	 * sdram_cfg[5:7] = (SDRAM type = DDR SDRAM)
	 *			010 DDR 1 SDRAM
	 *			011 DDR 2 SDRAM
	 */
	sdram_type = (mem_type == SPD_MEMTYPE_DDR) ? 2 : 3;
	sdram_cfg_1 = (0
			| (1 << 31)		/* Enable */
			| (1 << 30)		/* Self refresh */
			| ((cs_intlv_en ? cs_intlv_ctl : 0) << 8)
			| (sdram_type << 24));	/* SDRAM type */

	/*
	 * sdram_cfg[3] = RD_EN - registered DIMM enable
	 */
	if (rd_en)
		sdram_cfg_1 |= 0x10000000;		/* RD_EN */

#if defined(CONFIG_DDR_ECC)
	/*
	 * If the user wanted ECC (enabled via sdram_cfg[2])
	 */
	if (ecc_en) {
		ddr->err_disable = 0x00000000;
		asm volatile("sync;isync;");
		ddr->err_sbe = 0x00ff0000;
		/* XXX: wait 255 single bit errors before an error condition? */
		ddr->err_int_en = 0x0000000d;
		sdram_cfg_1 |= 0x20000000;		/* ECC_EN */
	}
#endif

	/*
	 * Set 1T or 2T timing based on 1 or 2 modules
	 */
	if (twoT_en) {
		debug("setting 2T_EN=1\n");
		/*
		 * 2T timing,because both DIMMS are present.
		 * Enable 2T timing by setting sdram_cfg[16].
		 */
		sdram_cfg_1 |= 0x8000;		/* 2T_EN */
	}

	/*
	 * 200 micro-seconds must elapse between when
	 * DDR_SDRAM_CLK_CNTL[CLK_ADJUST] is set and
	 * any chip select is enabled until
	 * DDR_SDRAM_CFG[MEM_EN] is set.
	 */
	udelay(2000);  /* 2ms */

	/*
	 * Go!
	 */

	asm volatile("sync;isync");
	ddr->sdram_cfg = sdram_cfg_1;


	debug("DDR: sdram_cfg   = 0x%08x\n", ddr->sdram_cfg);

#if defined(CONFIG_ECC_INIT_VIA_DDRCONTROLLER)
	debug("DDR: memory initializing\n");

	/*
	 * Poll until memory is initialized.
	 * 512 Meg at 400 might hit this 200 times or so.
	 */
	while ((ddr->sdram_cfg_2 & (1 << 4)) != 0)
		udelay(1000);	/* 1 ms */
	debug("DDR: memory initialized\n\n");
#endif

	return 0;
}

static struct {
	unsigned char addr;
	unsigned char memctl;
	unsigned char dimm;
} dimm_spd_addr_map[] = {
	{SPD_EEPROM_ADDRESS1, 0, 0},
	{SPD_EEPROM_ADDRESS2, 1, 0},
};

static unsigned char get_spd_eeprom_addr(unsigned int memctl,
		unsigned int dimm)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(dimm_spd_addr_map); i++) {
		if (dimm_spd_addr_map[i].memctl == memctl &&
			dimm_spd_addr_map[i].dimm == dimm)
			return dimm_spd_addr_map[i].addr;
	}

	return 0;
}

static unsigned long compute_dimmsize(const dimmparams_t *p)
{
	unsigned long dimmsize;
	/*
	 * bankbits is actually the value programmed into the CSn_CONFIG;
	 *   bankbits = 0 means 4 banks, or 2 bits
	 *   bankbits = 1 means 8 banks, or 3 bits
	 * 3 bits for 64-bit memory data bus
	 */
	dimmsize = p->nranks << (p->rowbits + p->colbits +
		(p->bankbits ? 3 : 2) + 3);
	return dimmsize;

}

void find_and_parse_dimm(
	unsigned int memctl,
	unsigned int dimm,
	spd_eeprom_t *spd,
	unsigned char spd_eeprom_addr,
	dimmparams_t *dimmparams,
	unsigned int *nranks,
	unsigned int *all_eccdimm,
	unsigned int *all_regdimm,
	unsigned int *ndimms)
{
	unsigned char rowbits;
	unsigned char colbits;
	unsigned int mem_type;
	unsigned char buf;

	memset(spd, 0, sizeof(spd_eeprom_t));
	/* Open i2c mux gate */
	buf = (1 << (4));  /* DIMM_0 or 1 connected to CTRL_0 or CTRL_1 */
	i2c_write(0x70, 0, 1, &buf, 1);
	spd_eeprom_addr = 0x50;

	CFG_READ_SPD(spd_eeprom_addr, 0, 1, (uchar *)spd, sizeof(spd_eeprom_t));

	/* close i2c mux gate */
	buf = 0x00;
	i2c_write(0x70, 0, 0, &buf, 1);
#ifdef SPD_DEBUG
	spd_debug(spd);
#endif
	/*
	 * Determine memory type from byte 2 of the SPD data.
	 * Use the reserved value of 0 to indicate an invalid
	 * or empty DIMM slot.  Pre-init this variable to 0 in
	 * case we exit this loop iteration early.
	 */

	memset(dimmparams, 0, sizeof(dimmparams_t));
	dimmparams->present = 0;

	mem_type = spd->mem_type;
	if (mem_type != SPD_MEMTYPE_DDR && mem_type != SPD_MEMTYPE_DDR2)
		return;

	/*
	 * Determine number of row address bits and column address bits.
	 * These tests gloss over DDR I and II differences in interpretation
	 * of bytes 3 and 4, but irrelevantly.  DDR I asymmetric physical banks
	 * (ranks) are not supported, and they are not encoded for DDR II.
	 */

	rowbits = spd->nrow_addr & 0xF;
	colbits = spd->ncol_addr & 0xF;

	if (rowbits < 12 || rowbits > 16) {
		printf("DDR: Unsupported number of Row Addr lines on "
		    "controller %u, dimm %u: %u.\n", memctl, dimm, rowbits);
		return;
	}
	if (colbits < 8 || colbits > 11) {
		printf("DDR: Unsupported number of Column Addr lines on "
		    "controller %u, dimm %u: %u.\n", memctl, dimm, rowbits);
		return;
	}

	dimmparams->rowbits = rowbits;
	dimmparams->colbits = colbits;

	/*
	 * Determine number of logical bank bits to use from byte 17.
	 *
	 * In Freescale memory controller documentation, logical banks are
	 * referred to as "sub-banks".
	 *
	 * Since nothing really depends upon the exact number of banks,
	 * just record the value to plug in to CSn_CONFIG[BA_BITS_CSn].
	 */


	switch (spd->nbanks) {
	case 4:
		dimmparams->bankbits = 0;  /* 4 banks */
		break;
	case 8:
		dimmparams->bankbits = 1;  /* 8 banks */
		break;
	default:
		printf("DDR: Unsupported number of logical banks "
			"on controller %u, dimm %u: %u\n",
				memctl, dimm, spd->nbanks);
		return;
		break;
	}

	/*
	 * Determine the number of physical banks (ranks) controlled by
	 * different Chip Select signals as encoded in byte 5.
	 *
	 * In Freescale memory controller documentation, physical banks (ranks)
	 * are referred to as "banks", usually without any qualifiers.
	 */

	if (mem_type == SPD_MEMTYPE_DDR)
		dimmparams->nranks = spd->nrows;
	else
		dimmparams->nranks = (spd->nrows & 0x7) + 1;

	*nranks += dimmparams->nranks;


#if defined(CONFIG_DDR_ECC)
	/* Check for ECC DIMMs */

	/*
	 * Byte 11 = "DIMM Configuration Type"
	 *   bit 1 = Data ECC
	 */

	if (!(spd->config & 0x02))
		*all_eccdimm = 0;
#endif

	/* Check for registered DIMMs */


	/*
	 *   A value of 0x26 indicates micron registered
	 *   DDR DIMMS (micron.com)
	 *
	 *   XXX: That statement in the original code is not quite right.
	 *   Only bit 1 of byte 21 needs to be 1 to indicate registered
	 *   address and control inputs.
	 */
	if (mem_type == SPD_MEMTYPE_DDR && spd->mod_attr != 0x26)
		*all_regdimm = 0;


	/*
	 * Note the inproper use of spd.h for DDR2: SPD byte 20 has
	 * changed meanining in DDR2 SPD to be DIMM type information.
	 * It is NOT write latency or write recovery as stated in spd.h.
	 *
	 * If byte 20 indicates an RDIMM or Mini-RDIMM, then we have
	 * a registered DIMM.
	 */
	if (mem_type == SPD_MEMTYPE_DDR2 &&
		spd->write_lat != 0x01 && /* RDIMM */
		spd->write_lat != 0x10) /* Mini-RDIMM */
		*all_regdimm = 0;

	debug("mem_type = %02X\n", mem_type);

	/* Finally, store the mem_type into .present last to indicate that this
	 * entry is valid. */
	dimmparams->present = mem_type;

	(*ndimms)++;
}


#if CONFIG_NUM_DDR_CONTROLLERS > 1
static const char *get_cs_intlv_mode_string(unsigned int cs_intlv_ctl)
{
	const char *cs_intlv_mode_str = NULL;
	switch (cs_intlv_ctl) {
	case 0x40:	/* CS0+CS1 */
		cs_intlv_mode_str = "CS0+CS1";
		break;
	case 0x04:	/* CS0+CS1+CS2+CS3 */
		cs_intlv_mode_str = "CS0+CS1+CS2+CS3";
		break;
	case 0x20:	/* CS2+CS3 */
		cs_intlv_mode_str = "CS2+CS3";
		break;
	case 0x60:	/* CS0+CS1, CS2+CS3 */
		cs_intlv_mode_str = "CS0+CS1,CS2+CS3";
		break;
	default:
		cs_intlv_mode_str = "unknown";
		break;
	}

	return cs_intlv_mode_str;
}

static const char *get_memctl_intlv_mode_string(unsigned int memctl_intlv_ctl)
{
	const char *memctl_intlv_str = NULL;
	switch (memctl_intlv_ctl) {
	case 0:
		memctl_intlv_str = "cache-line";
		break;
	case 1:
		memctl_intlv_str = "page";
		break;
	case 2:
		memctl_intlv_str = "bank";
		break;
	case 3:
		memctl_intlv_str = "superbank";
		break;
	default:
		memctl_intlv_str = "unknown";
		break;
	}
	return memctl_intlv_str;
}
#endif


/*
 * Setup TLB1 mappings for the requested amount of memory.  Just use a
 * single mapping of 2GB to cover all of DDR SDRAM.
 */

static void setup_tlbs(void)
{
	unsigned int tlb_size;
	unsigned int tlb_size_in_bytes;
	unsigned int ram_tlb_index;
	unsigned int ram_tlb_address;
	unsigned int memsize = 0x80000000; /* XXX: max supported by u-boot */

	if (PVR_VER(get_pvr()) > PVR_VER(PVR_85xx)) {
		tlb_size = BOOKE_PAGESZ_1GB;
		tlb_size_in_bytes = 1024*1024*1024;
	} else {
		tlb_size = BOOKE_PAGESZ_256M;
		tlb_size_in_bytes = 256*1024*1024;
	}

	/*
	 * Configure DDR TLB1 entries.
	 * Starting at TLB1 8, use no more than 8 TLB1 entries.
	 */
	ram_tlb_index = 8;
	ram_tlb_address = (unsigned int)CFG_DDR_SDRAM_BASE;
	while (ram_tlb_address < memsize && ram_tlb_index < 16) {
		mtspr(MAS0, TLB1_MAS0(1, ram_tlb_index, 0));
		mtspr(MAS1, TLB1_MAS1(1, 1, 0, 0, tlb_size));
		mtspr(MAS2, TLB1_MAS2(E500_TLB_EPN(ram_tlb_address),
				      0, 0, 0, 0, 0, 0, 0, 0));
		mtspr(MAS3, TLB1_MAS3(E500_TLB_RPN(ram_tlb_address),
				      0, 0, 0, 0, 0, 1, 0, 1, 0, 1));
		asm volatile("isync;msync;tlbwe;isync");

		debug("DDR: MAS0=0x%08x\n", TLB1_MAS0(1, ram_tlb_index, 0));
		debug("DDR: MAS1=0x%08x\n", TLB1_MAS1(1, 1, 0, 0, tlb_size));
		debug("DDR: MAS2=0x%08x\n",
		      TLB1_MAS2(E500_TLB_EPN(ram_tlb_address),
				0, 0, 0, 0, 0, 0, 0, 0));
		debug("DDR: MAS3=0x%08x\n",
		      TLB1_MAS3(E500_TLB_RPN(ram_tlb_address),
				0, 0, 0, 0, 0, 1, 0, 1, 0, 1));

		ram_tlb_address += tlb_size_in_bytes;
		ram_tlb_index++;
	}
}

unsigned long check_existing_config(volatile ccsr_ddr_t *ddr)
{
	unsigned int bnds;
	unsigned long memsize = 0;

	/*
	 * Skip configuration if already configured.
	 * memsize is determined from last configured chip select.
	 */
	if (ddr->cs0_config & 0x80000000) {
		debug(" cs0 already configured, bnds=%x\n", ddr->cs0_bnds);
		bnds = 0xfff & ddr->cs0_bnds;
		if (bnds < 0xff) { /* do not add if at top of 4G */
			memsize = (bnds + 1) << 4;
		}
	}
	if (ddr->cs1_config & 0x80000000) {
		debug(" cs1 already configured, bnds=%x\n", ddr->cs1_bnds);
		bnds = 0xfff & ddr->cs1_bnds;
		if (bnds < 0xff) { /* do not add if at top of 4G */
			memsize = (bnds + 1) << 4; /* assume ordered bnds */
		}
	}
	if (ddr->cs2_config & 0x80000000) {
		debug(" cs2 already configured, bnds=%x\n", ddr->cs2_bnds);
		bnds = 0xfff & ddr->cs2_bnds;
		if (bnds < 0xff) { /* do not add if at top of 4G */
			memsize = (bnds + 1) << 4;
		}
	}
	if (ddr->cs3_config & 0x80000000) {
		debug(" cs3 already configured, bnds=%x\n", ddr->cs3_bnds);
		bnds = 0xfff & ddr->cs3_bnds;
		if (bnds < 0xff) { /* do not add if at top of 4G */
			memsize = (bnds + 1) << 4;
		}
	}

	if (memsize) {
		printf("       Reusing current %dMB configuration "
			"(assuming not interleaved)\n", memsize);
		return memsize << 20;
	}

	return 0;
}


unsigned long spd_sdram(void)
{
	unsigned long memsize_memctl[CONFIG_NUM_DDR_CONTROLLERS];
	unsigned long memsize_total = 0;
	unsigned int memaddr;

	volatile immap_t *immap = (immap_t *)CFG_IMMR;
	volatile ccsr_local_ecm_t *ecm = &immap->im_local_ecm;
	unsigned int mem_type;


	/* dimmparams and spd are indexed using [controller][dimm] */
	dimmparams_t
	    dimmparams[CONFIG_NUM_DDR_CONTROLLERS][CONFIG_NUM_DIMMS_PER_DDR];
	spd_eeprom_t spd[CONFIG_NUM_DDR_CONTROLLERS][CONFIG_NUM_DIMMS_PER_DDR];

	unsigned int cs_intlv_en[CONFIG_NUM_DDR_CONTROLLERS];
	unsigned int cs_intlv_ctl[CONFIG_NUM_DDR_CONTROLLERS];
	unsigned int cs_intlv_en_general = 0;
	unsigned int cs_intlv_ctl_general = 0;

	unsigned int memctl_intlv_en = 0;
	unsigned int memctl_intlv_ctl = 0;

	unsigned int all_regdimm[CONFIG_NUM_DDR_CONTROLLERS];
	unsigned int all_eccdimm[CONFIG_NUM_DDR_CONTROLLERS];
	unsigned int nranks[CONFIG_NUM_DDR_CONTROLLERS];
	unsigned int ndimms[CONFIG_NUM_DDR_CONTROLLERS];

	unsigned char twoT_en[CONFIG_NUM_DDR_CONTROLLERS];

	unsigned int law_size;
	const char *p;
	unsigned int i, j;


#define MEMCTL0	(&immap->im_ddr1)
#define MEMCTL1	(&immap->im_ddr2)
	memsize_total += check_existing_config(MEMCTL0);
	memsize_total += check_existing_config(MEMCTL1);

	if (memsize_total) {
		setup_tlbs();
		return memsize_total;
	}
	debug("\n");

	/* Gather general DIMM info from SPD */

	for (i = 0; i < CONFIG_NUM_DDR_CONTROLLERS; i++) {
		memsize_memctl[i] = 0;
		all_regdimm[i] = 1;
		all_eccdimm[i] = 1;
		nranks[i] = 0;
		ndimms[i] = 0;
		for (j = 0; j < CONFIG_NUM_DIMMS_PER_DDR; j++)
			find_and_parse_dimm(
				i, j,
				&(spd[i][j]),
				get_spd_eeprom_addr(i, j),
				&(dimmparams[i][j]),
				&(nranks[i]),
				&(all_eccdimm[i]),
				&(all_regdimm[i]),
				&(ndimms[i]));
		debug("DDR: memctl=%u, ndimms=%u\n", i, ndimms[i]);
	}

	/* Check if there were any DIMMs found. */
	/* XXX: Assume both controllers use the same type of memory */

	mem_type = 0;
	for (i = 0; i < CONFIG_NUM_DDR_CONTROLLERS; i++) {
		for (j = 0; j < CONFIG_NUM_DIMMS_PER_DDR; j++)
			mem_type |= dimmparams[i][j].present;
	}

	if (!mem_type) {
		printf("DDR: No DIMMs found.\n");
		return 0;
	}

#ifdef DEBUG
	for (i = 0; i < CONFIG_NUM_DDR_CONTROLLERS; i++) {
		for (j = 0; j < CONFIG_NUM_DIMMS_PER_DDR; j++)
			print_dimmparams(i, j, &(dimmparams[i][j]));
	}
#endif

	debug("DDR: getting environment config regs\n");

	/* get interleaving configuration from environment */

/* Run these u-boot commands to configure the memory interleaving mode.
 *
 * # disable memory controller interleaving
 * setenv memctl_intlv_ctl
 *
 *
 * # cacheline interleaving
 * setenv memctl_intlv_ctl 0
 *
 * # page interleaving
 * setenv memctl_intlv_ctl 1
 *
 * # bank interleaving
 * setenv memctl_intlv_ctl 2
 *
 * # superbank
 * setenv memctl_intlv_ctl 3
 *
 *
 *
 * # disable chip select interleaving
 * setenv cs_intlv_ctl
 *
 *
 * # chip-select ineterleaving cs0, cs1
 * setenv cs_intlv_ctl 0x40
 *
 * # chip-select ineterleaving cs2, cs3
 * setenv cs_intlv_ctl 0x20
 *
 * # chip-select ineterleaving (cs0, cs1) and (cs2, cs3)  (2x2)
 * setenv cs_intlv_ctl 0x60
 *
 * # chip-select ineterleaving (cs0, cs1, cs2, cs3) (4x1)
 * setenv cs_intlv_ctl 0x04
 *
 *
 *
 * Table of interleaving modes supported by this code:
 *
 *  +-------------+---------------------------------------------------------+
 *  |             |                   Rank Interleaving                     |
 *  |             +--------+-----------+-----------+------------+-----------+
 *  |Memory       |        |           |           |    2x2     |    4x1    |
 *  |Controller   |  None  | 2x1 lower | 2x1 upper | {CS0+CS1}, | {CS0+CS1+ |
 *  |Interleaving |        | {CS0+CS1} | {CS2+CS3} | {CS2+CS3}  |  CS2+CS3} |
 *  +-------------+--------+-----------+-----------+------------+-----------+
 *  |None         |  Yes   | Yes       | Yes       | Yes        | Yes       |
 *  +-------------+--------+-----------+-----------+------------+-----------+
 *  |Cacheline    |  Yes   | Yes       | No        | No, Only(*)| Yes       |
 *  |             |CS0 Only|           |           | {CS0+CS1}  |           |
 *  +-------------+--------+-----------+-----------+------------+-----------+
 *  |Page         |  Yes   | Yes       | No        | No, Only(*)| Yes       |
 *  |             |CS0 Only|           |           | {CS0+CS1}  |           |
 *  +-------------+--------+-----------+-----------+------------+-----------+
 *  |Bank         |  Yes   | Yes       | No        | No, Only(*)| Yes       |
 *  |             |CS0 Only|           |           | {CS0+CS1}  |           |
 *  +-------------+--------+-----------+-----------+------------+-----------+
 *  |Superbank    |  No    | Yes       | No        | No, Only(*)| Yes       |
 *  |             |        |           |           | {CS0+CS1}  |           |
 *  +-------------+--------+-----------+-----------+------------+-----------+
 *
 * (*) Although the hardware can be configured with memory controller
 * interleaving using "2x2" rank interleaving, it only interleaves {CS0+CS1}
 * from each controller. {CS2+CS3} on each controller are only rank
 * interleaved on that controller.
 */


	/* XXX: Attempt to set both controllers to the same chip select
	 * interleaving mode. It will do a best effort to get the
	 * requested ranks interleaved together such that the result
	 * should be a subset of the requested configuration. */

	p = getenv("cs_intlv_ctl");
	if (p != NULL) {
		cs_intlv_ctl_general = simple_strtoul(p, NULL, 0);
		cs_intlv_en_general = 1;
		for (i = 0; i < CONFIG_NUM_DDR_CONTROLLERS; i++) {
			cs_intlv_en[i] = 1;
			cs_intlv_ctl[i] = cs_intlv_ctl_general;
		}
	} else {
		for (i = 0; i < CONFIG_NUM_DDR_CONTROLLERS; i++) {
			cs_intlv_en[i] = 0;
			cs_intlv_ctl[i] = 0;
		}
	}

	p = getenv("memctl_intlv_ctl");
	if (p != NULL) {
		memctl_intlv_en = 1;
		memctl_intlv_ctl = simple_strtoul(p, NULL, 0);
	} else {
		memctl_intlv_en = 0;
	}

	debug("DDR: Requested interleaving configuration: cs_intlv_en=%u, "
	  "cs_intlv_ctl=0x%02X, memctl_intlv_en=%u, memctl_intlv_ctl=0x%02X\n",
	  cs_intlv_en_general, cs_intlv_ctl_general, memctl_intlv_en,
	  memctl_intlv_ctl);

	/* Validate requirements for the requested interleaving mode.
	 * Fall back to non-interleaving if ANY requirement is not met.
	 */


#if CONFIG_NUM_DDR_CONTROLLERS != 2
	if (memctl_intlv_en) {
		printf("DDR: only 1 memory controller enabled in U-boot "
		"configuration.  Memory controller interleaving is not "
		"possible.\n");
		memctl_intlv_en = 0;
	}
#endif

	/* XXX: make this more compact and parameterized? */

#if CONFIG_NUM_DDR_CONTROLLERS > 1
	if (cs_intlv_en_general && memctl_intlv_en) {
		const char *cs_intlv_mode_str =
			get_cs_intlv_mode_string(cs_intlv_ctl_general);
		const char *memctl_intlv_str =
			get_memctl_intlv_mode_string(memctl_intlv_ctl);

		/* Requirements for memory controller interleaving with
		 * chip select interleaving:
		 *
		 * 0. memctl_intlv_ctl must be "3" for superbank interleaving
		 * 1. all ranks interleaved between controllers must be the
		 *    same #row, #col, #banks
		 * 2. cs_intlv_ctl = 0x40 (CS0+CS1) or 0x04 (CS0+CS1+CS2+CS3),
		 *    respectively, for chip-select interleaving on both
		 *    controllers. This implies that only dual-rank DIMMs may
		 *    be interleaved.
		 *
		 * Boundary address for the total memory across all controllers
		 * needs to be programmed identically into CS0_BNDS on each
		 * controller.  (All CS0_BNDS on all controllers will have the
		 * same value.)
		 */

		debug("DDR: %s chip-select interleaving mode with %s memory "
			"controller interleaving requested.  Checking...",
			cs_intlv_mode_str, memctl_intlv_str);

			/* Check for a valid chip select interleaving
			 * configuration and also if all DIMMs are the same */

/* -------------- Breaking indentation to reduce wrappiness --------------- */

	switch (cs_intlv_ctl_general) {
	case 0x40:	/* CS0+CS1 */

	/* The DIMMs being interleaved must be in slot 0 of both controllers. */
	if (!(dimmparams[0][0].present && dimmparams[1][0].present)) {
		printf("DIMMs not present in both controller's slot 0.  ");
		cs_intlv_en_general = 0;
		memctl_intlv_en = 0;
		break;
	}

#if CONFIG_NUM_DIMMS_PER_DDR > 1
	/* We can't interleave DIMMs in slot 1 of either controller. */
	if (dimmparams[0][1].present || dimmparams[1][1].present) {
		printf("There is a DIMM in slot 1 on at least one "
			"controller.  ");
		cs_intlv_en_general = 0;
		memctl_intlv_en = 0;
		break;
	}
#endif

	/* The interleaved DIMMs must have 2 ranks. */
	if ((dimmparams[0][0].nranks == 1) ||
	    (dimmparams[1][0].nranks == 1)) {
		printf("Some DIMMs only have 1 rank.  ");
		cs_intlv_en_general = 0;
		memctl_intlv_en = 0;
		break;
	}

	/* The DIMMs must have the same addressing bits. */
	if ((dimmparams[0][0].nranks != dimmparams[1][0].nranks) ||
	    (dimmparams[0][0].rowbits != dimmparams[1][0].rowbits) ||
	    (dimmparams[0][0].colbits != dimmparams[1][0].colbits) ||
	    (dimmparams[0][0].bankbits != dimmparams[1][0].bankbits)) {
		printf("DIMM's rank sizes do not match.  ");
		cs_intlv_en_general = 0;
		memctl_intlv_en = 0;
		break;
	}
	break;

#if CONFIG_NUM_DIMMS_PER_DDR > 1
	case 0x04:	/* CS0+CS1+CS2+CS3 */
		/* The DIMMs being interleaved must be in slot 0 AND 1 of both
		 * controllers. */
		if (!(dimmparams[0][0].present && dimmparams[1][0].present &&
		      dimmparams[0][1].present && dimmparams[1][1].present)) {
			printf("DIMMs not present in both controller's "
				"slot 0 and slot 1.  ");
			cs_intlv_en_general = 0;
			memctl_intlv_en = 0;
			break;
		}

		/* All DIMMs must have 2 ranks. */
		if ((dimmparams[0][0].nranks == 1) ||
		    (dimmparams[0][1].nranks == 1) ||
		    (dimmparams[1][0].nranks == 1) ||
		    (dimmparams[1][1].nranks == 1)) {
			printf("Some DIMMs have only 1 rank.  ");
			cs_intlv_en_general = 0;
			memctl_intlv_en = 0;
			break;
		}

		/* All DIMMs must have the same addressing bits. */
		if ((dimmparams[0][0].nranks != dimmparams[1][0].nranks) ||
		    (dimmparams[0][0].nranks != dimmparams[0][1].nranks) ||
		    (dimmparams[0][0].nranks != dimmparams[1][1].nranks) ||

		    (dimmparams[0][0].rowbits != dimmparams[1][0].rowbits) ||
		    (dimmparams[0][0].rowbits != dimmparams[0][1].rowbits) ||
		    (dimmparams[0][0].rowbits != dimmparams[1][1].rowbits) ||

		    (dimmparams[0][0].colbits != dimmparams[1][0].colbits) ||
		    (dimmparams[0][0].colbits != dimmparams[0][1].colbits) ||
		    (dimmparams[0][0].colbits != dimmparams[1][1].colbits) ||

		    (dimmparams[0][0].bankbits != dimmparams[1][0].bankbits) ||
		    (dimmparams[0][0].bankbits != dimmparams[0][1].bankbits) ||
		    (dimmparams[0][0].bankbits != dimmparams[1][1].bankbits)) {
			printf("DIMM sizes or addresses mismatch.  ");
			cs_intlv_en_general = 0;
			memctl_intlv_en = 0;
			break;
		}
		break;
#endif

	/* XXX: cs_intlv_ctl = 0x60 ((CS0+CS1) (CS2+CS3)) case is
	 * NOT supported by this code */

	default:
		printf("An unsupported chip-select interleaving mode "
			"0x%02X was selected.  ", cs_intlv_ctl_general);
		cs_intlv_en_general = 0;
		memctl_intlv_en = 0;
		break;
	}

/* --------------------- End indentation breakage ----------------------- */

	}

	if (memctl_intlv_en)
		debug("\nDDR: ... seems OK.\n");
	else
		printf("\nDDR: A configuration problem was found.  "
			"Forcing non-interleaved mode.\n");
#endif

	if (cs_intlv_en_general && !memctl_intlv_en) {
		/* Requirements for chip select interleaving without
		 * memory controller interleaving:
		 *
		 * 0. interleaved ranks must be the same size (#rows #cols
		 * #banks)
		 * 1. either a combination of CS0+CS1 or CS2+CS3 or
		 * 	CS0+CS1+CS2+CS3 or CS0+CS1 and CS2+CS3. (In practice,
		 * 	this means you can only interleave dual-rank DIMMs.)
		 *
		 * The boundary addresses for the range of each rank-tuple
		 * (2 or 4 ranks) need to be programmed into the lowest
		 * numbered rank of that tuple. e.g. CS0+CS1 => CS0_BNDS .
		 */

		debug("DDR: chip-select interleaving without memory "
			"controller interleving requested.  Checking "
			"feasibility...\n");

		/* XXX: Although the memory controllers support setting this
		 * parameter individually, this is not supported by this code.
		 * However, this code will attempt to interleave adjacent
		 * ranks if for the ranks that are requested.  If it can not
		 * (e.g., a single-rank DIMM is in that slot), it will disable
		 * chip-select interleaving on that DIMM slot.
		 * */

		for (i = 0; i < CONFIG_NUM_DDR_CONTROLLERS; i++) {
			unsigned int cs_intlv_ctl_temp = cs_intlv_ctl_general;
			printf("DDR: controller %u: ", i);

			/* cs_intlv_ctl is actually a bitmask.  Assume that if
			 * the number of ranks is 2 on a given DIMM for 0x40 or
			 * 0x20, then each rank is identical. */

			if (!ndimms[i]) {
				cs_intlv_en[i] = 0;
				printf("no DIMMs.\n");
				continue;
			}

			/* first DIMM */
			if (cs_intlv_ctl_temp & 0x40) {	/* CS0+CS1 */
				printf("CS0+CS1 ");
				if (!dimmparams[i][0].present ||
				     dimmparams[i][0].nranks != 2) {
					cs_intlv_ctl[i] &= ~0x40;
					printf("is not possible on memctl=%u "
					"because DIMM 0 is not present or "
					"dual-rank.  ", i);
				} else
					printf("is possible.  ");
				cs_intlv_ctl_temp &= ~0x40;
			}

#if CONFIG_NUM_DIMMS_PER_DDR > 1
			/* second DIMM */
			if (cs_intlv_ctl_temp & 0x20) {	/* CS2+CS3 */
				printf("CS2+CS3 ");
				if (!dimmparams[i][1].present ||
				     dimmparams[i][1].nranks != 2) {
					cs_intlv_ctl[i] &= ~0x20;
					printf("is not possible on memctl=%u "
						"because DIMM 1 is not present"
						" or dual-rank.\n", i);
				} else
					printf("is possible.  ");
				cs_intlv_ctl_temp &= ~0x20;
			}

			/* two DIMMs together */
			if (cs_intlv_ctl_temp == 0x04) { /* CS0+CS1+CS2+CS3 */
				printf("CS0+CS1+CS2+CS3 ");
				if (!dimmparams[i][0].present ||
				    !dimmparams[i][1].present ||
				     dimmparams[i][0].nranks != 2 ||
				     dimmparams[i][1].nranks != 2) {
					printf("is not possible because at "
						"least one of the DIMMs is "
						"not dual-rank.\n");
					cs_intlv_en[i] = 0;
					continue;
				}

				if (
		(dimmparams[i][0].rowbits != dimmparams[i][1].rowbits) ||
		(dimmparams[i][0].colbits != dimmparams[i][1].colbits) ||
		(dimmparams[i][0].bankbits != dimmparams[i][1].bankbits)) {
					printf("is not possible because "
						"the DIMMs are not all the "
						"same size.\n");
					cs_intlv_en[i] = 0;
					continue;
				}
				cs_intlv_ctl_temp &= ~0x04;
				printf("is possible.  ");
			}
#endif
			/* final check for any remaining config bits in the
			 * chip-select interleaving control value */
			if (cs_intlv_ctl_temp || !cs_intlv_ctl[i]) {
				printf("No valid bit(s) left in chip select "
					"interleaving control value for "
					"memctl=%u:  0x%X",
					i, cs_intlv_ctl_temp);
				cs_intlv_en[i] = 0;
			}
			printf("\n");
		}

		cs_intlv_en_general = 0;
		for (i = 0; i < CONFIG_NUM_DDR_CONTROLLERS; i++)
			cs_intlv_en_general |= cs_intlv_en[i];

		if (cs_intlv_en_general)
			debug("DDR: ... seems OK.\n");
		else
			printf("DDR: ... a configuration problem was found."
				"  Forcing non-interleaved mode.\n");
	}

#if CONFIG_NUM_DDR_CONTROLLERS > 1
	if (!cs_intlv_en_general && memctl_intlv_en) {
		/* Requirements for memory controller interleaving without
		 * chip select interleaving:
		 *
		 * 0. only CS0 can be interleaved between controllers.  CS1
		 * and higher ranks are not used.  (If dual-rank DIMMs are
		 * in slot 0, only half of each * DIMM will be available.)
		 * 1. CS0 rank size must be same on all controllers
		 *
		 * Bounds for total memory across all controllers CS0 needs
		 * to be programmed identically into CS0_BNDS on each
		 * controller.
		 */

		debug("DDR: memory controller interleaving without "
			"chip-select interleaving requested.\n"
		       "DDR: Only CS0 of each controller will be used.  "
		       "Checking feasibility...\n");

/* -------------- Breaking indentation to reduce wrappiness --------------- */

	switch (memctl_intlv_ctl) {
	case 3:
		printf("DDR: superbank interleaving "
		"requested without chip-select "
		"interleaving.  This doesn't make sense.\n");
		memctl_intlv_en = 0;
		break;
	case 0:	/* cache-line interleaved */
	case 1:	/* page interleaved */
	case 2: /* bank interleaved */
		/* check if there is memory on CS0 of
		 * both controllers.  */
		if (!(dimmparams[0][0].present && dimmparams[1][0].present)) {
			printf("DDR: no memory found on CS0 of one of the "
				"memory controllers.\n");
			memctl_intlv_en = 0;
			break;
		}

		/* check if both CS0 ranks have the same size */

		if ((dimmparams[0][0].rowbits != dimmparams[1][0].rowbits) ||
		    (dimmparams[0][0].colbits != dimmparams[1][0].colbits) ||
		    (dimmparams[0][0].bankbits != dimmparams[1][0].bankbits)) {
			printf("DDR: DIMM CS0 ranks not the same size.\n");
			memctl_intlv_en = 0;
			break;
		}

		/* Things seem OK if we get to here, but print a notice
		 * anyway if there are ranks not being used. */
#if CONFIG_NUM_DIMMS_PER_DDR > 1
		if (dimmparams[0][0].nranks > 1 || dimmparams[1][0].nranks > 1
		    || dimmparams[0][1].present || dimmparams[1][1].present)
#else
		if (dimmparams[0][0].nranks > 1 || dimmparams[1][0].nranks > 1)
#endif
			printf("DDR: NOTE: ranks OTHER than CS0 were found "
				"and they will NOT be used in this memory "
				"controller interleaving mode ...\n");
		break;

	default:
		printf("DDR: Invalid memctl_intlv_ctl mode value %u.\n",
			memctl_intlv_ctl);
		memctl_intlv_en = 0;
		break;
	}

/*-------------------  End indenation breaking ---------------------------- */

		if (memctl_intlv_en) {
			debug("DDR: Configuring for ");
			switch (memctl_intlv_ctl) {
			case 0:  debug("cache-line"); break;
			case 1:  debug("page"); break;
			case 2:  debug("bank"); break;
			}
			debug(" interleaving ... seems OK.\n");
		} else
			printf("DDR: A configuration problem was found.  "
				"Forcing non-interleaved mode.\n");
	}
#endif
	if (!cs_intlv_en_general && !memctl_intlv_en)
		printf("DDR: non-interleaved ");

	if (!cs_intlv_en_general) {
		cs_intlv_en[0] = 0;
#if CONFIG_NUM_DDR_CONTROLLERS > 1
		cs_intlv_en[1] = 0;
#endif
	}

	switch (mem_type) {
	case SPD_MEMTYPE_DDR2:
		debug("DDR2 SDRAM");
		break;
	case SPD_MEMTYPE_DDR:
		debug("DDR1 SDRAM");
		break;
	default:
		printf("unknown memory type");
		break;
	}

	puts(" - ");


	/* ******************************************** */
	/* Initialize CSn_BNDS and CSn_CONFIG registers */
	/* ******************************************** */

	memaddr = CFG_SDRAM_BASE;

	memsize_memctl[0] = spd_cs_init(MEMCTL0, spd[0], dimmparams[0], 0,
		memctl_intlv_en, memctl_intlv_ctl, cs_intlv_en[0],
		cs_intlv_ctl[0], memaddr, nranks[0]);
	debug("DDR: after spd_cs_init for memctl=0, memsize_memctl[0] = %u\n",
		memsize_memctl[0]);

	/* ***************************************** */
	/* initialize timing configuration registers */
	/* ***************************************** */

	debug("DDR: spd_timing_init memory controller 0\n");
	if (spd_timing_init(MEMCTL0, mem_type, spd[0], dimmparams[0], 0,
		nranks[0], ndimms[0])) {
		debug("DDR: oops, spd_timing_init error, so setting "
			"memsize_memctl[0] = 0\n");
		memsize_memctl[0] = 0;
		ndimms[0] = 0;
	}

	if (!memctl_intlv_en)
		memaddr += memsize_memctl[0];

#if CONFIG_NUM_DDR_CONTROLLERS > 1
	memsize_memctl[1] = spd_cs_init(MEMCTL1, spd[1], dimmparams[1], 1,
		memctl_intlv_en, memctl_intlv_ctl, cs_intlv_en[1],
		cs_intlv_ctl[1], memaddr, nranks[1]);
	debug("DDR: after spd_cs_init for memctl=1, memsize_memctl[1] = %u\n",
		memsize_memctl[1]);

	debug("DDR: spd_timing_init memory controller 1\n");
	if (spd_timing_init(MEMCTL1, mem_type, spd[1], dimmparams[1], 1,
		nranks[1], ndimms[1])) {
		debug("DDR: oops, spd_timing_init error, so setting "
			"memsize_memctl[1] = 0\n");
		memsize_memctl[1] = 0;
		ndimms[1] = 0;
	}
#endif

	twoT_en[0] = compute_2T(0, nranks[0], 0, get_ddr_freq(0)/1000000);
#if CONFIG_NUM_DDR_CONTROLLERS > 1
	twoT_en[1] = compute_2T(1, nranks[1], 0, get_ddr_freq(0)/1000000);
#endif

	/* ***************************************************** */
	/* Program final DDR_SDRAM_CFG on each controller and go */
	/* ***************************************************** */

	/* XXX: check return value */
	debug("DDR: spd_enable ddr controller 0\n");
	if (enable_ddr(MEMCTL0, mem_type, all_regdimm[0], all_eccdimm[0],
		cs_intlv_en[0], cs_intlv_ctl[0], dimmparams[0], ndimms[0],
		twoT_en[0]))
		memsize_memctl[0] = 0;

#if CONFIG_NUM_DDR_CONTROLLERS > 1
	debug("DDR: spd_enable ddr controller 1\n");
	if (enable_ddr(MEMCTL1, mem_type, all_regdimm[1], all_eccdimm[1],
		cs_intlv_en[1], cs_intlv_ctl[1], dimmparams[1], ndimms[1],
		twoT_en[1]))
		memsize_memctl[1] = 0;

	memsize_total = memsize_memctl[0] + memsize_memctl[1];
#else
	memsize_total = memsize_memctl[0];
#endif

	debug("DDR: memsize_total = %u before cap test\n", memsize_total);

	if (memsize_total > 0x80000000) {
		/* Current software can not support more
		 * than 2GB of memory. */
		memsize_total = 0x80000000;
		printf("DDR: capping memory size to 2 GB, but you may "
			"still have to remove some memory from the machine "
			"for it to work.\n");
	}

	debug("DDR: memsize_total = %u after cap test\n", memsize_total);

	/****************/
	/* Program LAWs */
	/****************/

	/*
	 * We have to do things differently depending upon the
	 * interleaving mode.  Allocate a LAW entry for each DIMM
	 * so that we can handle non-power-of-2 total memory sizes
	 * (e.g. 512MB + 1GB).
	 *
	 * XXX: this ought to be in or driven by a board-specific file
	 *
	 * XXX: Except for 8540ADS, all boards have only had 1 DIMM slot, but
	 * there may or may not be any free LAW entries to map each DIMM
	 * slot, and they all used LAW0 for DDR.  ONLY MPC8572DS has 2 memory
	 * controllers and it uses LAW0 and LAW1.  Actual mapping for
	 * a board with 2 DIMMs will need customization of the LAW mapping.
	 *
	 * XXX: constraints on LAW mapping are:
	 * - LAWs must begin on an address that is aligned to the window
	 *   size, i.e., a 512MB-sized LAW map must begin at an address that
	 *   is a multiple of 512MB.
	 * - Although LAWs may overlap, it is easier not to allow them to
	 *   overlap.
	 */

	/*
	 * LAW to DDR controller/DIMM mapping:
	 *
	 * LAW# Controller DIMM#
	 * 0    0          0
	 * 1    1          0
	 * 10   0          1
	 * 11   1          1
	 *
	 * 0    -          interleaved memory controllers
	 *
	 * 0    0          chip-select interleaved
	 * 1    1          chip-select interleaved
	 */

	ecm->lawar0 = 0;
#if CONFIG_NUM_DIMMS_PER_DDR > 1
	ecm->lawar10 = 0;
#endif
#if CONFIG_NUM_DDR_CONTROLLERS > 1
	ecm->lawar1 = 0;
#if CONFIG_NUM_DIMMS_PER_DDR > 1
	ecm->lawar11 = 0;
#endif
#endif

	if (memctl_intlv_en) {
		law_size = __ilog2(memsize_total) - 1;

		/* Double the law mask if memsize_total is not a perfect
		 * power of 2. However, this shouldn't ever happen. */
		if ((1 << (law_size + 1)) != memsize_total) {
			/* XXX: shouldn't get here */
			debug("DDR: WHOA. non-power-of-2 total memory %u "
				"with memory controller interleaving.\n",
				memsize_total);
			law_size++;
		}

		/*
		 * Set up LAW 0 for interleaved memory controllers.
		 */
		ecm->lawbar0 = ((CFG_DDR_SDRAM_BASE >> 12) & 0xfffff);
		ecm->lawar0 = (LAWAR_EN
			       | LAWAR_TRGT_IF_DDR_INTERLEAVED
			       | (LAWAR_SIZE & law_size));
	} else {
		unsigned long membase = CFG_DDR_SDRAM_BASE;
		unsigned long dimmsize;

		for (i = 0; i < CONFIG_NUM_DDR_CONTROLLERS; i++) {

/* -------------- Breaking indentation to reduce wrappiness --------------- */

if (memsize_memctl[i]) {
	if (cs_intlv_en[i] && (cs_intlv_ctl[i] == 0x04)) { /* CS0+CS1+CS2+CS3 */
		debug("DDR: setting up LAWs for 4-rank interleaving.\n\n");
		law_size = __ilog2(memsize_memctl[i]) - 1;
		/*
		 * Set up LAWBAR for DDR 1 space.
		 */
		if (i == 0) {
			/* controller 0 */
			ecm->lawbar0 = (membase >> 12) & 0xfffff;
			ecm->lawar0 = LAWAR_EN
					| LAWAR_TRGT_IF_DDR1
					| (LAWAR_SIZE & law_size);
		} else {
			/* controller 1 */
			ecm->lawbar1 = (membase >> 12) & 0xfffff;
			ecm->lawar1 = LAWAR_EN
					| LAWAR_TRGT_IF_DDR2
					| (LAWAR_SIZE & law_size);

		}
		membase += memsize_memctl[i];
	} else {
		debug("DDR: setting up LAWs per DIMM\n");
		for (j = 0; j < CONFIG_NUM_DIMMS_PER_DDR; j++) {
		    /* dimmsizes are going to be power of 2 */
		    if (dimmparams[i][j].present) {
			dimmsize = compute_dimmsize(&(dimmparams[i][j]));
			debug("DDR: memctl=%u dimm%u dimmsize = %08X\n",
				i, j, dimmsize);

			/* check that DIMM base address begins at multiple of
			 * its size
			 */
			if (membase & (dimmsize - 1)) {
				printf("DDR:  Cannot map a LAW for DIMM %u "
				"on controller %u because its base address "
				"is not aligned with its size.  Aborting!\n",
				j, i);
				return 0;
			}

			law_size = __ilog2(dimmsize) - 1;

			/*
			 * Set up LAWBAR for DDR 1 space.
			 */
			switch (j) {
			case 0:
				if (i == 0) {
				    ecm->lawbar0 = (membase >> 12) & 0xfffff;
				    ecm->lawar0 = LAWAR_EN
						| LAWAR_TRGT_IF_DDR1
						| (LAWAR_SIZE & law_size);
				    debug("LAWBAR0: %08X\n", ecm->lawbar0);
				    debug("LAWAR0:  %08X\n", ecm->lawar0);
				} else {
				    ecm->lawbar1 = (membase >> 12) & 0xfffff;
				    ecm->lawar1 = LAWAR_EN
						| LAWAR_TRGT_IF_DDR2
						| (LAWAR_SIZE & law_size);
				    debug("LAWBAR1: %08X\n", ecm->lawbar1);
				    debug("LAWAR1:  %08X\n", ecm->lawar1);
				}
				break;
#if CONFIG_NUM_DIMMS_PER_DDR > 1
			case 1:
				if (i == 0) {
				    ecm->lawbar10 = (membase >> 12) & 0xfffff;
				    ecm->lawar10 = LAWAR_EN
						    | LAWAR_TRGT_IF_DDR1
						    | (LAWAR_SIZE & law_size);
				} else {
				    ecm->lawbar11 = (membase >> 12) & 0xfffff;
				    ecm->lawar11 = LAWAR_EN
						    | LAWAR_TRGT_IF_DDR2
						    | (LAWAR_SIZE & law_size);
				}
				break;
#endif
			default:
				/* shouldn't get here */
				printf("DDR: memctl=%u, unexepcted DIMM "
					"number %u\n", i, j);
				break;
			}
			membase += dimmsize;
		    }
		}
	}
}

/*-------------------  End indenation breaking ---------------------------- */

			if (i == 0) {
				debug("DDR: LAWBAR0=0x%08x\n", ecm->lawbar0);
				debug("DDR: LAWAR0=0x%08x\n", ecm->lawar0);
#if CONFIG_NUM_DIMMS_PER_DDR > 1
				debug("DDR: LAWBAR10=0x%08x\n", ecm->lawbar10);
				debug("DDR: LAWAR10=0x%08x\n", ecm->lawar10);
#endif
			} else {
				debug("DDR: LAWBAR1=0x%08x\n", ecm->lawbar1);
				debug("DDR: LAWAR1=0x%08x\n", ecm->lawar1);
#if CONFIG_NUM_DIMMS_PER_DDR > 1
				debug("DDR: LAWBAR11=0x%08x\n", ecm->lawbar11);
				debug("DDR: LAWAR11=0x%08x\n", ecm->lawar11);
#endif
			}
		}
	}

	setup_tlbs();

	debug("DDR: memsize_total = %u\n", memsize_total);

	return memsize_total;
}
#endif /* CONFIG_SPD_EEPROM */

#if defined(CONFIG_DDR_ECC) && !defined(CONFIG_ECC_INIT_VIA_DDRCONTROLLER)

/*
 * Initialize all of memory for ECC, then enable errors.
 */

void ddr_enable_ecc(unsigned int dram_size)
{
	uint *p = 0;
	uint i = 0;
	volatile immap_t *immap = (immap_t *)CFG_IMMR;
	volatile ccsr_ddr_t *ddr1= &immap->im_ddr1;

	dma_init();

	for (*p = 0; p < (uint *)(8 * 1024); p++) {
		if (((unsigned int)p & 0x1f) == 0) {
			ppcDcbz((unsigned long) p);
		}
		*p = (unsigned int)CONFIG_MEM_INIT_VALUE;
		if (((unsigned int)p & 0x1c) == 0x1c) {
			ppcDcbf((unsigned long) p);
		}
	}

	dma_xfer((uint *)0x002000, 0x002000, (uint *)0); /* 8K */
	dma_xfer((uint *)0x004000, 0x004000, (uint *)0); /* 16K */
	dma_xfer((uint *)0x008000, 0x008000, (uint *)0); /* 32K */
	dma_xfer((uint *)0x010000, 0x010000, (uint *)0); /* 64K */
	dma_xfer((uint *)0x020000, 0x020000, (uint *)0); /* 128k */
	dma_xfer((uint *)0x040000, 0x040000, (uint *)0); /* 256k */
	dma_xfer((uint *)0x080000, 0x080000, (uint *)0); /* 512k */
	dma_xfer((uint *)0x100000, 0x100000, (uint *)0); /* 1M */
	dma_xfer((uint *)0x200000, 0x200000, (uint *)0); /* 2M */
	dma_xfer((uint *)0x400000, 0x400000, (uint *)0); /* 4M */

	for (i = 1; i < dram_size / 0x800000; i++) {
		dma_xfer((uint *)(0x800000*i), 0x800000, (uint *)0);
	}

	/*
	 * Enable errors for ECC.
	 */
	debug("DMA DDR: err_disable = 0x%08x\n", ddr->err_disable);
	ddr1->err_disable = 0x00000000;
	asm("sync;isync;msync");
	debug("DMA DDR: err_disable = 0x%08x\n", ddr->err_disable);
}


#endif	/* CONFIG_DDR_ECC  && ! CONFIG_ECC_INIT_VIA_DDRCONTROLLER */
