/*
 * Copyright (c) 2006-2011, Juniper Networks, Inc.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <ppc_asm.tmpl>
#include <ppc_defs.h>
#include <asm/cache.h>
#include <asm/mmu.h>
#include <config.h>
#include <mpc85xx.h>


/*
 * TLB0 and TLB1 Entries
 *
 * Out of reset, TLB1's Entry 0 maps the highest 4K for CCSRBAR.
 * However, CCSRBAR is then relocated to CFG_CCSRBAR right after
 * these TLB entries are established.
 *
 * The TLB entries for DDR are dynamically setup in spd_sdram()
 * and use TLB1 Entries 8 through 15 as needed according to the
 * size of DDR memory.
 *
 * MAS0: tlbsel, esel, nv
 * MAS1: valid, iprot, tid, ts, tsize
 * MAS2: epn, sharen, x0, x1, w, i, m, g, e
 * MAS3: rpn, u0-u3, ux, sx, uw, sw, ur, sr
 */

#define	entry_start \
	mflr	r1 	;	\
	bl	0f 	;

#define	entry_end \
0:	mflr	r0	;	\
	mtlr	r1	;	\
	blr		;


	.section	.bootpg, "ax"
	.globl	tlb1_entry
tlb1_entry:
	entry_start

	/*
	 * Number of TLB0 and TLB1 entries in the following table
	 */
#if !defined(CONFIG_SPD_EEPROM)
	.long 14
#else
	.long 13
#endif

#if (CFG_CCSRBAR_DEFAULT != CFG_CCSRBAR)
	/*
	 * TLB0		4K	Non-cacheable, guarded
	 * 0xff700000	4K	Initial CCSRBAR mapping
	 *
	 * This ends up at a TLB0 Index==0 entry, and must not collide
	 * with other TLB0 Entries.
	 */
	.long TLB1_MAS0(0, 0, 0)
	.long TLB1_MAS1(1, 0, 0, 0, 0)
	.long TLB1_MAS2(E500_TLB_EPN(CFG_CCSRBAR_DEFAULT), 0,0,0,0,1,0,1,0)
	.long TLB1_MAS3(E500_TLB_RPN(CFG_CCSRBAR_DEFAULT), 0,0,0,0,0,1,0,1,0,1)
#else
#error("Update the number of table entries in tlb1_entry")
#endif

	/*
	 * TLB0		16K	Cacheable, non-guarded
	 * 0xd001_0000	16K	Temporary Global data for initialization
	 *
	 * Use four 4K TLB0 entries.  These entries must be cacheable
	 * as they provide the bootstrap memory before the memory
	 * controler and real memory have been configured.
	 *
	 * These entries end up at TLB0 Indicies 0x10, 0x14, 0x18 and 0x1c,
	 * and must not collide with other TLB0 entries.
	 */
	.long TLB1_MAS0(0, 0, 0)
	.long TLB1_MAS1(1, 0, 0, 0, 0)
	.long TLB1_MAS2(E500_TLB_EPN(CFG_INIT_RAM_ADDR),
			0,0,0,0,0,0,0,0)
	.long TLB1_MAS3(E500_TLB_RPN(CFG_INIT_RAM_ADDR),
			0,0,0,0,0,1,0,1,0,1)

	.long TLB1_MAS0(0, 0, 0)
	.long TLB1_MAS1(1, 0, 0, 0, 0)
	.long TLB1_MAS2(E500_TLB_EPN(CFG_INIT_RAM_ADDR + 4 * 1024),
			0,0,0,0,0,0,0,0)
	.long TLB1_MAS3(E500_TLB_RPN(CFG_INIT_RAM_ADDR + 4 * 1024),
			0,0,0,0,0,1,0,1,0,1)

	.long TLB1_MAS0(0, 0, 0)
	.long TLB1_MAS1(1, 0, 0, 0, 0)
	.long TLB1_MAS2(E500_TLB_EPN(CFG_INIT_RAM_ADDR + 8 * 1024),
			0,0,0,0,0,0,0,0)
	.long TLB1_MAS3(E500_TLB_RPN(CFG_INIT_RAM_ADDR + 8 * 1024),
			0,0,0,0,0,1,0,1,0,1)

	.long TLB1_MAS0(0, 0, 0)
	.long TLB1_MAS1(1, 0, 0, 0, 0)
	.long TLB1_MAS2(E500_TLB_EPN(CFG_INIT_RAM_ADDR + 12 * 1024),
			0,0,0,0,0,0,0,0)
	.long TLB1_MAS3(E500_TLB_RPN(CFG_INIT_RAM_ADDR + 12 * 1024),
			0,0,0,0,0,1,0,1,0,1)



	/*
	 * TLB 0:	16M	Non-cacheable, guarded
	 * 0xff000000	64K	SYSPLD
	 * 0xff800000	8M	FLASH
	 * Out of reset this entry is only 4K.
	 */
	.long TLB1_MAS0(1, 0, 0)
	.long TLB1_MAS1(1, 1, 0, 0, BOOKE_PAGESZ_16M)
	.long TLB1_MAS2(E500_TLB_EPN(CFG_EPLD_BASE), 0,0,0,0,1,0,1,0)
	.long TLB1_MAS3(E500_TLB_RPN(CFG_EPLD_BASE), 0,0,0,0,0,1,0,1,0,1)

	/*
	 * TLB 1:	256M	Non-cacheable, guarded
	 * 0x80000000	256M	PCI MEM USB controller
	 */
	.long TLB1_MAS0(1, 1, 0)
	.long TLB1_MAS1(1, 1, 0, 0, BOOKE_PAGESZ_256M)
	.long TLB1_MAS2(E500_TLB_EPN(CFG_PCI1_MEM_BASE), 0,0,0,0,1,0,1,0)
	.long TLB1_MAS3(E500_TLB_RPN(CFG_PCI1_MEM_BASE), 0,0,0,0,0,1,0,1,0,1)

	/*
	 * TLB 2:	256M	Non-cacheable, guarded
	 * 0xa0000000	256M	PCI MEM MAC#1
	 */
	.long TLB1_MAS0(1, 2, 0)
	.long TLB1_MAS1(1, 1, 0, 0, BOOKE_PAGESZ_256M)
	.long TLB1_MAS2(E500_TLB_EPN(CFG_PCI1_MEM_BASE + 0x20000000),
			0,0,0,0,1,0,1,0)
	.long TLB1_MAS3(E500_TLB_RPN(CFG_PCI1_MEM_BASE + 0x20000000),
			0,0,0,0,0,1,0,1,0,1)

	/*
	 * TLB 3:	256M	Non-cacheable, guarded
	 * 0xb0000000	256M	PCI MEM MAC#2
	 */
	.long TLB1_MAS0(1, 3, 0)
	.long TLB1_MAS1(1, 1, 0, 0, BOOKE_PAGESZ_256M)
	.long TLB1_MAS2(E500_TLB_EPN(CFG_PCI1_MEM_BASE + 0x30000000),
			0,0,0,0,1,0,1,0)
	.long TLB1_MAS3(E500_TLB_RPN(CFG_PCI1_MEM_BASE + 0x30000000),
			0,0,0,0,0,1,0,1,0,1)

	/*
	 * TLB 4:	64M	Non-cacheable, guarded
	 * 0xfef0_0000	1M	CCSRBAR
	 */
	.long TLB1_MAS0(1, 4, 0)
	.long TLB1_MAS1(1, 1, 0, 0, BOOKE_PAGESZ_1M)
	.long TLB1_MAS2(E500_TLB_EPN(CFG_CCSRBAR), 0,0,0,0,1,0,1,0)
	.long TLB1_MAS3(E500_TLB_RPN(CFG_CCSRBAR), 0,0,0,0,0,1,0,1,0,1)

	/*
	 * TLB 5:	256M	Non-cacheable, guarded
	 * 0xc0000000	256M	PCIe 1
	 */
	.long TLB1_MAS0(1, 5, 0)
	.long TLB1_MAS1(1, 1, 0, 0, BOOKE_PAGESZ_256M)
	.long TLB1_MAS2(E500_TLB_EPN(CFG_PCIE1_MEM_BASE),
			0,0,0,0,1,0,1,0)
	.long TLB1_MAS3(E500_TLB_RPN(CFG_PCIE1_MEM_BASE),
			0,0,0,0,0,1,0,1,0,1)

	/*
	 * TLB 6:	256M	Non-cacheable, guarded
	 * 0xd0000000	256M	PCIe 2
	 */
	.long TLB1_MAS0(1, 6, 0)
	.long TLB1_MAS1(1, 1, 0, 0, BOOKE_PAGESZ_256M)
	.long TLB1_MAS2(E500_TLB_EPN(CFG_PCIE2_MEM_BASE),
			0,0,0,0,1,0,1,0)
	.long TLB1_MAS3(E500_TLB_RPN(CFG_PCIE2_MEM_BASE),
			0,0,0,0,0,1,0,1,0,1)

	/*
	 * TLB 7:	256M	Non-cacheable, guarded
	 * 0xc0000000	256M	PCIe 3
	 */
	.long TLB1_MAS0(1, 7, 0)
	.long TLB1_MAS1(1, 1, 0, 0, BOOKE_PAGESZ_256M)
	.long TLB1_MAS2(E500_TLB_EPN(CFG_PCIE3_MEM_BASE),
			0,0,0,0,1,0,1,0)
	.long TLB1_MAS3(E500_TLB_RPN(CFG_PCIE3_MEM_BASE),
			0,0,0,0,0,1,0,1,0,1)

#if !defined(CONFIG_SPD_EEPROM)
	/*
	 * TLB 8:	1G	Cacheable, non-guarded
	 * 0x0000_0000	1G SDRAM
	 */
	.long TLB1_MAS0(1, 8, 0)
	.long TLB1_MAS1(1, 1, 0, 0, BOOKE_PAGESZ_1GB)
	.long TLB1_MAS2(E500_TLB_EPN(CFG_DDR_SDRAM_BASE), 0,0,0,0,0,0,0,0)
	.long TLB1_MAS3(E500_TLB_RPN(CFG_DDR_SDRAM_BASE), 0,0,0,0,0,1,0,1,0,1)
#endif

	entry_end

/*
 * LAW(Local Access Window) configuration:
 *
 * 0x0000_0000     0x7fff_ffff     DDR                     512M
 * 0x8000_0000     0xbfff_ffff     PCI MEM                  1G 
 * 0xc000_0000     0xcfff_ffff     PCIe 1                  256M
 * 0xd000_0000     0xdfff_ffff     PCIe 2                  256M
 * 0xe000_0000     0xefff_ffff     PCIe 3                  256M
 * 0xfef0_0000     0xfeff_ffff     CCSR                    1M
 * 0xff00_0000     0xff00_ffff   EPLD (LCD)               64K
 * 0xff01_0000     0xff01_7fff   NAND               32K
 * 0xff80_0000      0xffff_ffff     FLASH (boot bank)       8M
 *
 * Notes:
 *    CCSRBAR and L2-as-SRAM don't need a configured Local Access Window.
 *    If flash is 8M at default position (last 8M), no LAW needed.
 *
 * The defines below are 1-off of the actual LAWAR0 usage.
 * So LAWAR3 define uses the LAWAR4 register in the ECM.
 */

#define LAWBAR0 0
#if defined(CONFIG_SPD_EEPROM)
#define LAWAR0  (LAWAR_TRGT_IF_DDR | (LAWAR_SIZE & LAWAR_SIZE_1G) & ~LAWAR_EN)
#else
#define LAWAR0  (LAWAR_TRGT_IF_DDR | (LAWAR_SIZE & LAWAR_SIZE_1G) | LAWAR_EN)
#endif

/* PCI window */
#define LAWBAR1 ((CFG_PCI1_MEM_BASE>>12) & 0xfffff)
#define LAWAR1 	(LAWAR_EN | LAWAR_TRGT_IF_PCI | (LAWAR_SIZE & LAWAR_SIZE_1G))

/* LBC window */
#define LAWBAR2 ((CFG_EPLD_BASE>>12) & 0xfffff)
#define LAWAR2 	(LAWAR_EN | LAWAR_TRGT_IF_LBC | (LAWAR_SIZE & LAWAR_SIZE_16M))

/* PCIe 1 window */
#define LAWBAR3 ((CFG_PCIE1_MEM_BASE>>12) & 0xfffff)
#define LAWAR3 	(LAWAR_EN | LAWAR_TRGT_IF_PCIE1 | (LAWAR_SIZE & LAWAR_SIZE_256M))

/* PCIe 2 window */
#define LAWBAR4 ((CFG_PCIE2_MEM_BASE>>12) & 0xfffff)
#define LAWAR4 	(LAWAR_EN | LAWAR_TRGT_IF_PCIE2 | (LAWAR_SIZE & LAWAR_SIZE_256M))

/* PCIe 3 window */
#define LAWBAR5 ((CFG_PCIE3_MEM_BASE>>12) & 0xfffff)
#define LAWAR5 	(LAWAR_EN | LAWAR_TRGT_IF_PCIE3 | (LAWAR_SIZE & LAWAR_SIZE_256M))

	.section .bootpg, "ax"
	.globl	law_entry

law_entry:
	entry_start
	.long 6
	.long LAWBAR0,LAWAR0,LAWBAR1,LAWAR1,LAWBAR2,LAWAR2
	.long LAWBAR3,LAWAR3,LAWBAR4,LAWAR4,LAWBAR5,LAWAR5
	entry_end
