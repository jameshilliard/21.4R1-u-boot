/*
 * Copyright 2004 Freescale Semiconductor.
 * (C) Copyright 2003 Motorola Inc.
 * Xianghua Xiao, (X.Xiao@motorola.com)
 *
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <ppc_asm.tmpl>
#include <asm/processor.h>

DECLARE_GLOBAL_DATA_PTR;

/* --------------------------------------------------------------- */

void get_sys_info (sys_info_t * sysInfo)
{
	volatile immap_t    *immap = (immap_t *)CFG_IMMR;
	volatile ccsr_gur_t *gur = &immap->im_gur;
	uint plat_ratio,e500_ratio;
#if defined(CONFIG_P2020) || defined(CONFIG_MPC8572)
	uint ddr_ratio;
#endif

	plat_ratio = (gur->porpllsr) & 0x0000003e;
	plat_ratio >>= 1;
	switch(plat_ratio) {
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x08:
	case 0x09:
	case 0x0a:
	case 0x0c:
	case 0x10:
		sysInfo->freqSystemBus = plat_ratio * CONFIG_SYS_CLK_FREQ;
		break;
	default:
		sysInfo->freqSystemBus = 0;
		break;
	}

	e500_ratio = (gur->porpllsr) & 0x003f0000;
	e500_ratio >>= 16;
	switch(e500_ratio) {
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
	case 0x08:
	case 0x09:
		sysInfo->freqProcessor = e500_ratio * sysInfo->freqSystemBus/2;
		break;
	default:
		sysInfo->freqProcessor = 0;
		break;
	}

#if defined(CONFIG_MULTICORE)
	e500_ratio = ((gur->porpllsr) >> 24)  & 0x3f;
	switch(e500_ratio) {
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
	case 0x08:
	case 0x09:
		sysInfo->freqProcessor1 = e500_ratio * (sysInfo->freqSystemBus/2);
		break;
	default:
		sysInfo->freqProcessor1 = 0;
		break;
	}
#endif

	sysInfo->freqDDRBus = sysInfo->freqSystemBus;

#ifdef CONFIG_DDR_CLK_FREQ
	ddr_ratio = (gur->porpllsr) & MPC85xx_PORPLLSR_DDR_RATIO;
	ddr_ratio >>= 9;
	/* Check if synchronous and if not, obtain DDR frequency */
	if (ddr_ratio != 0x7) {
	   switch(ddr_ratio) {
	      case 0x3:
	      case 0x4:
	      case 0x6:
	      case 0x8:
	      case 0xa:
	      case 0xc:
	      case 0xe:
		 sysInfo->freqDDRBus = ddr_ratio * CONFIG_DDR_CLK_FREQ;
		 break;
	      default:
		 /* Default to CCB frequency */
		 sysInfo->freqDDRBus = sysInfo->freqSystemBus;
		 break;
	   }
	}
#endif

}

int get_clocks (void)
{
	sys_info_t sys_info;
#if defined(CONFIG_CPM2)
	volatile immap_t *immap = (immap_t *) CFG_IMMR;
	uint sccr, dfbrg;

	/* set VCO = 4 * BRG */
	immap->im_cpm.im_cpm_intctl.sccr &= 0xfffffffc;
	sccr = immap->im_cpm.im_cpm_intctl.sccr;
	dfbrg = (sccr & SCCR_DFBRG_MSK) >> SCCR_DFBRG_SHIFT;
#endif
	get_sys_info (&sys_info);
	gd->cpu_clk = sys_info.freqProcessor;
	gd->bus_clk = sys_info.freqSystemBus;
#if defined(CONFIG_MPC8572) || defined(CONFIG_P2020)
	gd->ddr_clk = sys_info.freqDDRBus;
#endif
#if defined(CONFIG_CPM2)
	gd->vco_out = 2*sys_info.freqSystemBus;
	gd->cpm_clk = gd->vco_out / 2;
	gd->scc_clk = gd->vco_out / 4;
	gd->brg_clk = gd->vco_out / (1 << (2 * (dfbrg + 1)));
#endif

	if(gd->cpu_clk != 0) return (0);
	else return (1);
}


/********************************************
 * get_bus_freq
 * return system bus freq in Hz
 *********************************************/
ulong get_bus_freq (ulong dummy)
{
	ulong val;

	sys_info_t sys_info;

	get_sys_info (&sys_info);
	val = sys_info.freqSystemBus;

	return val;
}

#if defined(CONFIG_MPC8572) || defined(CONFIG_P2020)
/********************************************
 * get_ddr_freq
 * return ddr bus freq in Hz
 *********************************************/
ulong get_ddr_freq (ulong dummy)
{
	return gd->ddr_clk;
}
#endif
