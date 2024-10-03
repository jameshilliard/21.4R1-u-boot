/*
 * Copyright (c) 2006-2012, Juniper Networks, Inc.
 * All rights reserved.
 * 
 * Adapt mpc8533/8544
 *
 * Copyright 2004 Freescale Semiconductor.
 * (C) Copyright 2002, 2003 Motorola Inc.
 * Xianghua Xiao (X.Xiao@motorola.com)
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
#include <watchdog.h>
#include <command.h>
#include <asm/cache.h>

#if defined(CONFIG_OF_FLAT_TREE)
#include <ft_build.h>
#endif

#if defined(CONFIG_EX3242) || defined(CONFIG_EX45XX)
extern int epld_system_reset (void);
#elif defined(CONFIG_EX82XX)
extern int btcpld_cpu_hard_reset(void);
#elif defined(CONFIG_MX80)
extern int tbbcpld_system_reset(void);
#elif defined(CONFIG_ACX)
extern int syspld_cpu_reset(void);
extern void syspld_self_reset(void);
extern int is_acx500_board(void);
#if !defined(CONFIG_ACX_ICRT)
extern void reset_nand_next_slice(void);
#endif
#endif

int checkcpu (void)
{
	sys_info_t sysinfo;
	uint lcrr;		/* local bus clock ratio register */
	uint clkdiv;		/* clock divider portion of lcrr */
	uint pvr, svr;
	uint fam;
	uint ver, rev;
	uint major, minor;
#if defined (CONFIG_MULTICORE)
	uint pir;
#endif
	volatile immap_t    *immap = (immap_t *)CFG_IMMR;
	volatile ccsr_gur_t *gur = &immap->im_gur;

	svr = get_svr();
	ver = SVR_VER(svr);
	rev = SVR_REV(svr);
	major = SVR_MAJ(svr);
	minor = SVR_MIN(svr);

	puts("CPU:   ");
	switch (ver) {
	case SVR_8540:
		puts("8540");
		break;
	case SVR_8541:
		puts("8541");
		break;
	case SVR_8555:
		puts("8555");
		break;
	case SVR_8560:
		puts("8560");
		break;
	case SVR_8548:
		puts("8548");
		break;
	case SVR_8548_E:
		puts("8548_E");
		break;
	case SVR_8533:
              if ((rev & 0xFF00) == 0x0100)
		    puts("8544");
              else
		    puts("8533");
		break;
	case SVR_8533_E:
              if ((rev & 0xFF00) == 0x0100)
		    puts("8544_E");
              else
		    puts("8533_E");
		break;
	case SVR_8572_E:
		puts("8572_E");
		break;
	case SVR_8572:
		puts("8572");
		break;
	case SVR_P2010:
		puts("P2010");
		break;
	case SVR_P2010_E:
		puts("P2010_E");
		break;
	case SVR_P2020:
		puts("P2020");
		break;
	case SVR_P2020_E:
		puts("P2020_E");
		break;		
	default:
		puts("Unknown");
		break;
	}
	printf(", Version: %d.%d, (0x%08x)\n", major, minor, svr);

	pvr = get_pvr();
	fam = PVR_FAM(pvr);
	ver = PVR_VER(pvr);
	major = PVR_MAJ(pvr);
	minor = PVR_MIN(pvr);

#if defined (CONFIG_MULTICORE)
	pir = get_pir();
	printf("Core%d:  ", pir);
#else
	printf("Core:  ");
#endif
	switch (fam) {
	case PVR_FAM(PVR_85xx):
	    puts("E500");
	    break;
	default:
	    puts("Unknown");
	    break;
	}
	printf(", Version: %d.%d, (0x%08x)\n", major, minor, pvr);

	get_sys_info(&sysinfo);

	puts("Clock Configuration:\n");
#define ROUNDED_MHZ(x)   (((x) + 500000) / 1000000)
#ifdef CONFIG_MULTICORE
	printf("       CPU0:%4lu MHz, ", ROUNDED_MHZ(sysinfo.freqProcessor));
	printf("       CPU1:%4lu MHz, ", ROUNDED_MHZ(sysinfo.freqProcessor1));
#else
	printf("       CPU:%4lu MHz, ", sysinfo.freqProcessor / 1000000);
#endif
	printf("CCB:%4lu MHz,\n", ROUNDED_MHZ(sysinfo.freqSystemBus));
	printf("       DDR:%4lu MHz (%lu MT/s data rate) ",
	ROUNDED_MHZ(sysinfo.freqDDRBus/2),
	ROUNDED_MHZ(sysinfo.freqDDRBus));
	if ((((gur->porpllsr) & MPC85xx_PORPLLSR_DDR_RATIO) >> 9) != 0x7)
	    puts("(Asynchronous), ");
	else
	     puts("(Synchronous), ");

#if defined(CFG_LBC_LCRR)
	lcrr = CFG_LBC_LCRR;
#else
	{
	    volatile immap_t *immap = (immap_t *)CFG_IMMR;
	    volatile ccsr_lbc_t *lbc= &immap->im_lbc;

	    lcrr = lbc->lcrr;
	}
#endif
	clkdiv = lcrr & 0x0f;
	if (clkdiv == 2 || clkdiv == 4 || clkdiv == 8) {
#if defined(CONFIG_MPC8548) || defined(CONFIG_MPC8533) || defined(CONFIG_MPC8544) \
    || defined(CONFIG_MPC8572) || defined(CONFIG_P2020)
		/*
		 * Yes, the entire PQ38 family use the same
		 * bit-representation for twice the clock divider values.
		 */
		 clkdiv *= 2;
#endif
		printf("LBC:%4lu MHz\n",
		       sysinfo.freqSystemBus / 1000000 / clkdiv);
	} else {
		printf("LBC: unknown (lcrr: 0x%08x)\n", lcrr);
	}

	if (ver == SVR_8560) {
		printf("CPM:  %lu Mhz\n",
		       sysinfo.freqSystemBus / 1000000);
	}

	puts("L1:    D-cache 32 kB enabled\n       I-cache 32 kB enabled\n");

	return 0;
}


/* ------------------------------------------------------------------------- */

int do_reset (cmd_tbl_t *cmdtp, bd_t *bd, int flag, int argc, char *argv[])
{
	uint pvr;
	uint ver;
	pvr = get_pvr();
	ver = PVR_VER(pvr);
	if (ver & 1){
#if defined(CONFIG_EX3242) || defined(CONFIG_EX45XX)
              epld_system_reset ();
              return 1;
#elif defined(CONFIG_EX82XX)
			  btcpld_cpu_hard_reset();
			  return 1;
#elif defined(CONFIG_ACX)
#if !defined(CONFIG_ACX_ICRT)
		reset_nand_next_slice();
#endif
		/*
		 * If it is ACX-500 reset syspld, transfer control
		 * back to iCRT and also keep the slice changed to try
		 * boot from alternate slice
		 */
		if (is_acx500_board()) {
		    syspld_self_reset();
		}		
		else {
		    syspld_cpu_reset(); 
		}
		return 1;
#elif defined(CONFIG_MX80)
		tbbcpld_system_reset();
		return 1;
#endif
	/* e500 v2 core has reset control register */ 
        	volatile unsigned int * rstcr;
        	rstcr = (volatile unsigned int *)(CFG_IMMR + 0xE00B0);
       	*rstcr = 0x2;           /* HRESET_REQ */
	}else{
 	/*
 	 * Initiate hard reset in debug control register DBCR0
 	 * Make sure MSR[DE] = 1
 	 */

		unsigned long val;
		val = mfspr(DBCR0);
		val |= 0x70000000;
		mtspr(DBCR0,val);
	}
	return 1;
}

DECLARE_GLOBAL_DATA_PTR;

/*
 * Get timebase clock frequency
 */
unsigned long get_tbclk (void)
{

#if defined (CONFIG_MPC8572)
	return (gd->bus_clk + 4UL) / 8UL;
#else
	sys_info_t  sys_info;

	get_sys_info(&sys_info);
	return ((sys_info.freqSystemBus + 7L) / 8L);
#endif
}


#if defined(CONFIG_WATCHDOG)
void
watchdog_reset(void)
{
	int re_enable = disable_interrupts();
	reset_85xx_watchdog();
	if (re_enable) enable_interrupts();
}

void
reset_85xx_watchdog(void)
{
	/*
	 * Clear TSR(WIS) bit by writing 1
	 */
	unsigned long val;
	val = mfspr(tsr);
	val |= 0x40000000;
	mtspr(tsr, val);
}
#endif	/* CONFIG_WATCHDOG */

#if defined(CONFIG_DDR_ECC)
void dma_init(void) {
	volatile immap_t *immap = (immap_t *)CFG_IMMR;
	volatile ccsr_dma_t *dma = &immap->im_dma;

	dma->satr0 = 0x02c40000;
	dma->datr0 = 0x02c40000;
	asm("sync; isync; msync");
	return;
}

uint dma_check(void) {
	volatile immap_t *immap = (immap_t *)CFG_IMMR;
	volatile ccsr_dma_t *dma = &immap->im_dma;
	volatile uint status = dma->sr0;

	/* While the channel is busy, spin */
	while((status & 4) == 4) {
		status = dma->sr0;
	}

	if (status != 0) {
		printf ("DMA Error: status = %x\n", status);
	}
	return status;
}

int dma_xfer(void *dest, uint count, void *src) {
	volatile immap_t *immap = (immap_t *)CFG_IMMR;
	volatile ccsr_dma_t *dma = &immap->im_dma;

	dma->dar0 = (uint) dest;
	dma->sar0 = (uint) src;
	dma->bcr0 = count;
	dma->mr0 = 0xf000004;
	asm("sync;isync;msync");
	dma->mr0 = 0xf000005;
	asm("sync;isync;msync");
	return dma_check();
}
#endif


#ifdef CONFIG_OF_FLAT_TREE
void
ft_cpu_setup(void *blob, bd_t *bd)
{
	u32 *p;
	ulong clock;
	int len;

	clock = bd->bi_busfreq;
	p = ft_get_prop(blob, "/cpus/" OF_CPU "/bus-frequency", &len);
	if (p != NULL)
		*p = cpu_to_be32(clock);

	p = ft_get_prop(blob, "/" OF_SOC "/serial@4500/clock-frequency", &len);
	if (p != NULL)
		*p = cpu_to_be32(clock);

	p = ft_get_prop(blob, "/" OF_SOC "/serial@4600/clock-frequency", &len);
	if (p != NULL)
		*p = cpu_to_be32(clock);

#if defined(CONFIG_MPC85XX_TSEC1)
	p = ft_get_prop(blob, "/" OF_SOC "/ethernet@24000/mac-address", &len);
		memcpy(p, bd->bi_enetaddr, 6);
#endif

#if defined(CONFIG_HAS_ETH1)
	p = ft_get_prop(blob, "/" OF_SOC "/ethernet@25000/mac-address", &len);
		memcpy(p, bd->bi_enet1addr, 6);
#endif

#if defined(CONFIG_HAS_ETH2)
	p = ft_get_prop(blob, "/" OF_SOC "/ethernet@26000/mac-address", &len);
		memcpy(p, bd->bi_enet2addr, 6);
#endif

#if defined(CONFIG_HAS_ETH3)
	p = ft_get_prop(blob, "/" OF_SOC "/ethernet@27000/mac-address", &len);
		memcpy(p, bd->bi_enet3addr, 6);
#endif

}
#endif
