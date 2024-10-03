/*
 * Copyright (c) 2011-2013, Juniper Networks, Inc.
 * All rights reserved.
 *
 * Copyright 2008-2009 Freescale Semiconductor, Inc.
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

#include <hwconfig.h>
#include <asm/mmu.h>
#include <asm/fsl_ddr_sdram.h>
#include <asm/fsl_ddr_dimm_params.h>
#include <asm/fsl_law.h>

struct board_specific_parameters {
    u32 n_ranks;
    u32 datarate_mhz_high;
    u32 clk_adjust;
    u32 wrlvl_start;
    u32 cpo;
    u32 write_data_delay;
    u32 force_2T;
};

/*
 * This table contains all valid speeds we want to override with board
 * specific parameters. datarate_mhz_high values need to be in ascending order
 * for each n_ranks group.
 *
 * ranges for parameters:
 *  wr_data_delay = 0-6
 *  clk adjust = 0-8
 *  cpo 2-0x1E (30)
 */
static const struct board_specific_parameters dimm0[] = {
    /*
     * memory controller 0
     *   num|  hi|  clk| wrlvl | cpo  |wrdata|2T
     * ranks| mhz|adjst| start | delay|
     */
    {1,  1200,    3,     6,   0xff,    2,  0},
    {2,  1200,    4,     6,   0xff,    2,  0},
    {}
};

void
fsl_ddr_board_options (memctl_options_t *popts,
		       dimm_params_t *pdimm,
		       unsigned int ctrl_num)
{
    const struct board_specific_parameters *pbsp, *pbsp_highest = NULL;
    ulong ddr_freq;

    if (ctrl_num) {
	printf("Wrong parameter for controller number %d", ctrl_num);
	return;
    }

    if (!pdimm->n_ranks)
	return;

    pbsp = dimm0;

    /*
     * Get clk_adjust, cpo, write_data_delay,2T, according to the board ddr
     * freqency and n_banks specified in board_specific_parameters table.
     */
    ddr_freq = get_ddr_freq(0) / 1000000;
    while (pbsp->datarate_mhz_high) {
	if (pbsp->n_ranks == pdimm->n_ranks) {
	    if (ddr_freq <= pbsp->datarate_mhz_high) {
		popts->cpo_override = pbsp->cpo;
		popts->write_data_delay =
		  pbsp->write_data_delay;
		popts->clk_adjust = pbsp->clk_adjust;
		popts->wrlvl_start = pbsp->wrlvl_start;
		popts->twoT_en = pbsp->force_2T;
		goto found;
	    }
	    pbsp_highest = pbsp;
	}
	pbsp++;
    }

    if (pbsp_highest) {
	printf("Error: board specific timing not found "
	    "for data rate %lu MT/s!\n"
	    "Trying to use the highest speed (%u) parameters\n",
	    ddr_freq, pbsp_highest->datarate_mhz_high);
	popts->cpo_override = pbsp_highest->cpo;
	popts->write_data_delay = pbsp_highest->write_data_delay;
	popts->clk_adjust = pbsp_highest->clk_adjust;
	popts->wrlvl_start = pbsp_highest->wrlvl_start;
	popts->twoT_en = pbsp_highest->force_2T;
    } else {
	panic("DIMM is not supported by this board");
    }

found:
    /*
     * Factors to consider for half-strength driver enable:
     * - number of DIMMs installed
     */
    popts->half_strength_driver_enable = 0;
    /* Write leveling override */
    popts->wrlvl_override = 1;
    popts->wrlvl_sample = 0xf;
    /* Rtt and Rtt_WR override */
    popts->rtt_override = 0;

    /* Enable ZQ calibration */
    popts->zq_en = 1;

    /* DHC_EN =1, ODT = 60 Ohm */
    popts->ddr_cdr1 = DDR_CDR1_DHC_EN;
}

