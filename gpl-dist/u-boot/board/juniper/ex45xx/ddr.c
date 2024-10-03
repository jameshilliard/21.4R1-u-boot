/*
 * Copyright (c) 2011-2013, Juniper Networks, Inc.
 * All rights reserved.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/mmu.h>
#include <asm/immap_85xx.h>
#include <asm/processor.h>
#include <asm/fsl_ddr_sdram.h>
#include <asm/io.h>
#include <asm/fsl_law.h>

DECLARE_GLOBAL_DATA_PTR;
extern void dma_init(void);
/*
 * Fixed sdram init -- doesn't use serial presence detect.
 */

phys_size_t fixed_sdram (void)
{
	volatile ccsr_ddr_t *ddr= (void *)CONFIG_SYS_MPC85xx_DDR_ADDR;
	uint32_t sdram_cfg = 0, msize = CONFIG_SYS_SDRAM_SIZE;
	sys_info_t sysinfo;
	phys_size_t ddr_size;
	char buf[32];

	get_sys_info(&sysinfo);
	printf("Configuring DDR for %s MT/s data rate\n",
				strmhz(buf, sysinfo.freqDDRBus));


	/* EX4500 has DDR2 */
#ifdef CONFIG_EX4500
	ddr->sdram_cfg = 0x42000000;
	ddr->cs0_bnds = (msize * 1024 * 1024 -1) >> 24;
	ddr->cs0_config = (msize == 1024) ?
			    (CFG_DDR_CS0_CONFIG | CFG_DDR_3BIT_BANKS_CONFIG) :
			    CFG_DDR_CS0_CONFIG;
	ddr->timing_cfg_3 = CFG_DDR_REFREC;
	ddr->timing_cfg_0 = CFG_DDR_TIMING_0;
	ddr->timing_cfg_1 = CFG_DDR_TIMING_1;
	ddr->timing_cfg_2 = CFG_DDR_TIMING_2;
	ddr->sdram_mode = CFG_DDR_MODE;
	sdram_cfg = CFG_DDR_CFG;
	ddr->sdram_cfg_2 = CFG_DDR_CFG_2;
	ddr->sdram_mode_2 = CFG_DDR_MODE_2;
	ddr->sdram_interval = CFG_DDR_INTERVAL;
	ddr->sdram_clk_cntl = CFG_DDR_CLK_CTRL;
	ddr->sdram_data_init = CONFIG_MEM_INIT_VALUE;

	asm("sync;isync");
	udelay(500);
#if defined (CONFIG_DDR_ECC)
	/* Enable ECC checking */
	ddr->sdram_cfg = (sdram_cfg | 0x20000000);
#else
	ddr->sdram_cfg = sdram_cfg;
#endif
	asm("sync; isync");
	udelay(500);

	while ((ddr->sdram_cfg_2 & (0x00000010)) != 0) {
		udelay(1000);
	}
#endif
	/* EX4510 has DDR3 */
#ifdef CONFIG_EX4510 
	ddr->sdram_cfg = CONFIG_SYS_SDRAM_CFG_INITIAL;

	ddr->cs0_bnds = CONFIG_SYS_DDR3_CS0_BNDS;
	ddr->cs0_config = CONFIG_SYS_DDR3_CS0_CONFIG;
	ddr->cs0_config_2 = CONFIG_SYS_DDR3_CS0_CONFIG_2;
	/* CONFIG_SYS_DDR3_TIMING_3 sets EXT_REFREC(trfc) to 32 clks. */
	ddr->timing_cfg_3 = CONFIG_SYS_DDR3_TIMING_3;
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
	ddr->ddr_cdr1 = CONFIG_SYS_DDR3_CDR1; 

#if defined (CONFIG_DDR_ECC)
	ddr->err_disable = CONFIG_SYS_DDR3_ERR_DISABLE;
	ddr->err_sbe = CONFIG_SYS_DDR3_ERR_SBE;
#endif

	/* Set D_INIT bit in S/W and poll on this bit till HW clears it */
	ddr->sdram_cfg_2 |= CONFIG_SYS_DDR3_DINIT; /* DINIT = 1 */
	asm ("sync;isync");
	udelay(500);

	/* Set, but do not enable the memory yet. */
	sdram_cfg = ddr->sdram_cfg;
	sdram_cfg &= ~(SDRAM_CFG_MEM_EN);
	ddr->sdram_cfg = sdram_cfg;

	/*
	 * At least 500 micro-seconds (in case of DDR3) must
	 * elapse between the DDR clock setup and the DDR config
	 * enable. For now to be on the safer side, wait for 1000
	 * micro-seconds and optimize later after the bringup.
	 */
	asm ("sync;isync");
	udelay(1000);

	/* Let the controller go. Enable. */
	ddr->sdram_cfg = sdram_cfg | SDRAM_CFG_MEM_EN;
#if defined (CONFIG_DDR_ECC)
	/* Enable ECC checking. */
	ddr->sdram_cfg = CONFIG_SYS_SDRAM_CFG | 0x20000000;
#else
	ddr->sdram_cfg = CONFIG_SYS_SDRAM_CFG_NO_ECC;
#endif
	asm ("sync; isync");
	udelay(500);
	while((ddr->sdram_cfg_2 & (CONFIG_SYS_DDR3_DINIT)) != 0) {
		udelay(1000);
	}
#endif
	ddr_size = (phys_size_t) (CONFIG_SYS_SDRAM_SIZE * 1024 * 1024);
	
	return ddr_size;
}

void ddr_enable_ecc(unsigned int dram_size)
{
	volatile ccsr_ddr_t *ddr= (void *)CONFIG_SYS_MPC85xx_DDR_ADDR;
       
	dma_meminit(CONFIG_MEM_INIT_VALUE, CONFIG_SYS_SDRAM_SIZE);
	/*
	 * Enable errors for ECC.
	 */
	debug("DMA DDR: err_disable = 0x%08x\n", ddr->err_disable);

	ddr->err_disable = 0x00000000;
	asm("sync;isync;msync");
	debug("DMA DDR: err_disable = 0x%08x\n", ddr->err_disable);
    
}
