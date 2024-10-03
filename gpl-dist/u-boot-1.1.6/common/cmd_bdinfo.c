/*
 * Copyright (c) 2006-2012, Juniper Networks, Inc.
 * All rights reserved.
 *
 * (C) Copyright 2003
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

/*
 * Boot support
 */
#include <common.h>
#include <command.h>
#include <net.h>		/* for print_IPaddr */
#if defined(CONFIG_EX3242) || defined(CONFIG_EX82XX) || defined(CONFIG_EX45XX)
#include <asm/processor.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

#if (CONFIG_COMMANDS & CFG_CMD_BDI)
static void print_num(const char *, ulong);

#ifndef CONFIG_ARM	/* PowerPC and other */

#ifdef CONFIG_PPC
static void print_str(const char *, const char *);

int do_bdinfo ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i;
	bd_t *bd = gd->bd;
	char buf[32];

#if defined(CONFIG_EX3242) || defined(CONFIG_EX82XX) || defined(CONFIG_EX45XX)
	sys_info_t sysinfo;
	volatile immap_t *immap = (immap_t *)CFG_IMMR;
	volatile ccsr_gur_t *gur = &immap->im_gur;
	uint clkdiv;
	uint pvr, svr;
	uint fam;
	uint svr_ver, rev, ver;
	uint major, minor;
       int dcache=0, icache=0, l2cache=0;
	uint ddr_ratio;
	int ddr_clk_freq = 66666666;

	svr = get_svr();
	ver = SVR_VER(svr);
	svr_ver = ver;
	rev = SVR_REV(svr);
	major = SVR_MAJ(svr);
	minor = SVR_MIN(svr);
	printf ("processor   = ");
       dcache = 32;
       icache = 32;
       l2cache = 256;
       clkdiv = CFG_LBC_LCRR & 0x0f;
	switch (svr_ver) {
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
              l2cache = 512;
              clkdiv = clkdiv * 2;
		break;
	case SVR_8548_E:
		puts("8548_E");
              l2cache = 512;
              clkdiv = clkdiv * 2;
		break;
	case SVR_8533:
              if ((rev & 0xFF00) == 0x0100)
		    puts("8544");
              else
		    puts("8533");
              clkdiv = clkdiv * 2;
		break;
	case SVR_8533_E:
              if ((rev & 0xFF00) == 0x0100)
		    puts("8544_E");
              else
		    puts("8533_E");
              clkdiv = clkdiv * 2;
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
	printf ("core        = ");
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

	if (svr_ver == SVR_P2010 || svr_ver == SVR_P2010_E ||
	    svr_ver == SVR_P2020 || svr_ver == SVR_P2020_E) {
		ddr_ratio = (gur->porpllsr) & MPC85xx_PORPLLSR_DDR_RATIO;
		ddr_ratio >>= 9;
		if (ddr_ratio == 0x7) { /* Synchronous */ 
			sysinfo.freqDDRBus = sysinfo.freqSystemBus;
		} else {
			switch(ddr_ratio) {
			case 0x3:
			case 0x4:
			case 0x6:
			case 0x8:
			case 0xa:
			case 0xc:
			case 0xe:
				sysinfo.freqDDRBus = ddr_ratio * ddr_clk_freq;
				break;
			default:
				sysinfo.freqDDRBus = sysinfo.freqSystemBus;
				break;
			}
		}
	}

	printf ("CPU clock   = %4lu MHz\n", sysinfo.freqProcessor / 1000000);
	printf ("CCB clock   = %4lu MHz\n", sysinfo.freqSystemBus / 1000000);
	if (svr_ver == SVR_P2010 || svr_ver == SVR_P2010_E ||
	    svr_ver == SVR_P2020 || svr_ver == SVR_P2020_E) {
		printf ("DDR clock   = %4lu MHz\n", sysinfo.freqDDRBus / 2000000);
	} else {
		printf ("DDR clock   = %4lu MHz\n", sysinfo.freqSystemBus / 2000000);
	}
	printf ("LBC clock   = %4lu MHz\n", sysinfo.freqSystemBus / 1000000 / clkdiv);
	printf ("D-cache     = %4lu KB\n", dcache);
	printf ("I-cache     = %4lu KB\n", icache);
	printf ("L2-cache    = %4lu KB\n", l2cache);
	print_num ("memstart",	    bd->bi_memstart	);
	print_num ("memsize",	    bd->bi_memsize	);
	print_num ("flashstart",    bd->bi_flashstart	);
	print_num ("flashsize",	    bd->bi_flashsize	);
	puts ("ethaddr     =");
	for (i=0; i<6; ++i) {
		printf ("%c%02X", i ? ':' : ' ', bd->bi_enetaddr[i]);
	}
#if defined(CONFIG_EX45XX)
	puts ("\neth1addr    =");
	for (i=0; i<6; ++i) {
		printf ("%c%02X", i ? ':' : ' ', bd->bi_enet1addr[i]);
	}
       puts ("\neth2addr    =");
       for (i=0; i<6; ++i) {
		printf ("%c%02X", i ? ':' : ' ', bd->bi_enet2addr[i]);
	}
       puts ("\neth3addr    =");
       for (i=0; i<6; ++i) {
		printf ("%c%02X", i ? ':' : ' ', bd->bi_enet3addr[i]);
	}
#endif
	puts ("\nIP addr     = ");	print_IPaddr (bd->bi_ip_addr);
	printf ("\nbaudrate    = %6ld bps\n", bd->bi_baudrate   );
       return 0;
#endif

#if defined(CONFIG_MX80) || defined(CONFIG_ACX)
	sys_info_t sysinfo;

	get_sys_info(&sysinfo);
	printf ("CPU clock   = %4lu MHz\n", sysinfo.freqProcessor / 1000000);
	printf ("CCB clock   = %4lu MHz\n", sysinfo.freqSystemBus / 1000000);
	printf ("DDR clock   = %4lu MHz\n", sysinfo.freqSystemBus / 2000000);
#endif

#ifdef DEBUG
	print_num ("bd address",    (ulong)bd		);
#endif
	print_num ("memstart",	    bd->bi_memstart	);
	print_num ("memsize",	    bd->bi_memsize	);
	print_num ("flashstart",    bd->bi_flashstart	);
	print_num ("flashsize",	    bd->bi_flashsize	);
	print_num ("flashoffset",   bd->bi_flashoffset	);
	print_num ("sramstart",	    bd->bi_sramstart	);
	print_num ("sramsize",	    bd->bi_sramsize	);
#if defined(CONFIG_5xx)  || defined(CONFIG_8xx) || \
    defined(CONFIG_8260) || defined(CONFIG_E500)
	print_num ("immr_base",	    bd->bi_immr_base	);
#endif
	print_num ("bootflags",	    bd->bi_bootflags	);
#if defined(CONFIG_405GP) || defined(CONFIG_405CR) || \
    defined(CONFIG_405EP) || defined(CONFIG_XILINX_ML300) || \
    defined(CONFIG_440EP) || defined(CONFIG_440GR) || \
    defined(CONFIG_440EPX) || defined(CONFIG_440GRX) ||	\
    defined(CONFIG_440SP) || defined(CONFIG_440SPE)
	print_str ("procfreq",	    strmhz(buf, bd->bi_procfreq));
	print_str ("plb_busfreq",   strmhz(buf, bd->bi_plb_busfreq));
#if defined(CONFIG_405GP) || defined(CONFIG_405EP) || defined(CONFIG_XILINX_ML300) || \
    defined(CONFIG_440EP) || defined(CONFIG_440GR) || defined(CONFIG_440SPE) || \
    defined(CONFIG_440EPX) || defined(CONFIG_440GRX)
	print_str ("pci_busfreq",   strmhz(buf, bd->bi_pci_busfreq));
#endif
#else	/* ! CONFIG_405GP, CONFIG_405CR, CONFIG_405EP, CONFIG_XILINX_ML300, CONFIG_440EP CONFIG_440GR */
#if defined(CONFIG_CPM2)
	print_str ("vco",	    strmhz(buf, bd->bi_vco));
	print_str ("sccfreq",	    strmhz(buf, bd->bi_sccfreq));
	print_str ("brgfreq",	    strmhz(buf, bd->bi_brgfreq));
#endif
	print_str ("intfreq",	    strmhz(buf, bd->bi_intfreq));
#if defined(CONFIG_CPM2)
	print_str ("cpmfreq",	    strmhz(buf, bd->bi_cpmfreq));
#endif
	print_str ("busfreq",	    strmhz(buf, bd->bi_busfreq));
#endif /* CONFIG_405GP, CONFIG_405CR, CONFIG_405EP, CONFIG_XILINX_ML300, CONFIG_440EP CONFIG_440GR */
#if defined(CONFIG_MPC8220)
	print_str ("inpfreq",	    strmhz(buf, bd->bi_inpfreq));
	print_str ("flbfreq",	    strmhz(buf, bd->bi_flbfreq));
	print_str ("pcifreq",	    strmhz(buf, bd->bi_pcifreq));
	print_str ("vcofreq",	    strmhz(buf, bd->bi_vcofreq));
	print_str ("pevfreq",	    strmhz(buf, bd->bi_pevfreq));
#endif

	puts ("ethaddr     =");
	for (i=0; i<6; ++i) {
		printf ("%c%02X", i ? ':' : ' ', bd->bi_enetaddr[i]);
	}

#if defined(CONFIG_HAS_ETH1)
	puts ("\neth1addr    =");
	for (i=0; i<6; ++i) {
		printf ("%c%02X", i ? ':' : ' ', bd->bi_enet1addr[i]);
	}
#endif

#if defined(CONFIG_HAS_ETH2)
       puts ("\neth2addr    =");
       for (i=0; i<6; ++i) {
		printf ("%c%02X", i ? ':' : ' ', bd->bi_enet2addr[i]);
	}
#endif

#if defined(CONFIG_HAS_ETH3)
       puts ("\neth3addr    =");
       for (i=0; i<6; ++i) {
		printf ("%c%02X", i ? ':' : ' ', bd->bi_enet3addr[i]);
	}
#endif

#ifdef CONFIG_HERMES
	print_str ("ethspeed",	    strmhz(buf, bd->bi_ethspeed));
#endif
	puts ("\nIP addr     = ");	print_IPaddr (bd->bi_ip_addr);
	printf ("\nbaudrate    = %6ld bps\n", bd->bi_baudrate   );
	return 0;
}

#elif defined(CONFIG_NIOS) /* NIOS*/

int do_bdinfo ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i;
	bd_t *bd = gd->bd;

	print_num ("memstart",		(ulong)bd->bi_memstart);
	print_num ("memsize",		(ulong)bd->bi_memsize);
	print_num ("flashstart",	(ulong)bd->bi_flashstart);
	print_num ("flashsize",		(ulong)bd->bi_flashsize);
	print_num ("flashoffset",	(ulong)bd->bi_flashoffset);

	puts ("ethaddr     =");
	for (i=0; i<6; ++i) {
		printf ("%c%02X", i ? ':' : ' ', bd->bi_enetaddr[i]);
	}
	puts ("\nip_addr     = ");
	print_IPaddr (bd->bi_ip_addr);
	printf ("\nbaudrate    = %ld bps\n", bd->bi_baudrate);

	return 0;
}

#elif defined(CONFIG_NIOS2) /* Nios-II */

int do_bdinfo ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i;
	bd_t *bd = gd->bd;

	print_num ("mem start",		(ulong)bd->bi_memstart);
	print_num ("mem size",		(ulong)bd->bi_memsize);
	print_num ("flash start",	(ulong)bd->bi_flashstart);
	print_num ("flash size",	(ulong)bd->bi_flashsize);
	print_num ("flash offset",	(ulong)bd->bi_flashoffset);

#if defined(CFG_SRAM_BASE)
	print_num ("sram start",	(ulong)bd->bi_sramstart);
	print_num ("sram size",		(ulong)bd->bi_sramsize);
#endif

#if defined(CFG_CMD_NET)
	puts ("ethaddr     =");
	for (i=0; i<6; ++i) {
		printf ("%c%02X", i ? ':' : ' ', bd->bi_enetaddr[i]);
	}
	puts ("\nip_addr     = ");
	print_IPaddr (bd->bi_ip_addr);
#endif

	printf ("\nbaudrate    = %ld bps\n", bd->bi_baudrate);

	return 0;
}

#else /* ! PPC, which leaves MIPS */

int do_bdinfo ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i;
	bd_t *bd = gd->bd;

	print_num ("boot_params",	(ulong)bd->bi_boot_params);
	print_num ("memstart",		(ulong)bd->bi_memstart);
	print_num ("memsize",		(ulong)bd->bi_memsize);
	print_num ("flashstart",	(ulong)bd->bi_flashstart);
	print_num ("flashsize",		(ulong)bd->bi_flashsize);
	print_num ("flashoffset",	(ulong)bd->bi_flashoffset);

	puts ("ethaddr     =");
	for (i=0; i<6; ++i) {
		printf ("%c%02X", i ? ':' : ' ', bd->bi_enetaddr[i]);
	}
	puts ("\nip_addr     = ");
	print_IPaddr (bd->bi_ip_addr);
	printf ("\nbaudrate    = %d bps\n", bd->bi_baudrate);

	return 0;
}
#endif  /* MIPS */

#else	/* ARM */

int do_bdinfo ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i;
	bd_t *bd = gd->bd;

#if defined(CONFIG_EXTENDED_BDINFO)
	extern void ext_bdinfo(void);

	ext_bdinfo();
#else
	print_num ("arch_number",	bd->bi_arch_number);
	print_num ("env_t",		(ulong)bd->bi_env);
	print_num ("boot_params",	(ulong)bd->bi_boot_params);

	for (i=0; i<CONFIG_NR_DRAM_BANKS; ++i) {
		print_num("DRAM bank",	i);
		print_num("-> start",	bd->bi_dram[i].start);
		print_num("-> size",	bd->bi_dram[i].size);
	}
#endif

	puts ("ethaddr     =");
	for (i=0; i<6; ++i) {
		printf ("%c%02X", i ? ':' : ' ', bd->bi_enetaddr[i]);
	}
	puts  ( "\n"
		"ip_addr     = ");
	print_IPaddr (bd->bi_ip_addr);
	printf ("\n"
		"baudrate    = %d bps\n", bd->bi_baudrate);

	return 0;
}

#endif /* CONFIG_ARM XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */

static void print_num(const char *name, ulong value)
{
	printf ("%-12s= 0x%08lX\n", name, value);
}

#ifdef CONFIG_PPC
static void print_str(const char *name, const char *str)
{
	printf ("%-12s= %6s MHz\n", name, str);
}
#endif	/* CONFIG_PPC */


/* -------------------------------------------------------------------- */

U_BOOT_CMD(
	bdinfo,	1,	1,	do_bdinfo,
	"bdinfo  - print Board Info structure\n",
	NULL
);
#endif	/* CFG_CMD_BDI */
