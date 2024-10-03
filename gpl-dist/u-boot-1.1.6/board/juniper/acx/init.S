/*
 * $Id$
 *
 * init.S -- ACX TLB / LAW settings & initialization
 *
 * Rajat Jain, Feb 2011
 *
 * Copyright (c) 2011-2012, Juniper Networks, Inc.
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

#include <ppc_asm.tmpl>
#include <ppc_defs.h>
#include <asm/cache.h>
#include <asm/mmu.h>
#include <config.h>
#include <mpc85xx.h>

#define LAWAR_TRGT_PCIE1	0x00200000
#define LAWAR_TRGT_PCIE2	0x00100000
#define LAWAR_TRGT_PCIE3	0x00000000
#define LAWAR_TRGT_LBC		0x00400000
#define LAWAR_TRGT_DDR		0x00f00000

#define SIZE_8MB (8 * 1024 * 1024)

/*
 * TLB0 and TLB1 Entries
 *
 * Out of reset, TLB1's Entry 0 maps the last 4K of memory,
 * and CCSRBAR is located at its default address.  TLB
 * entries are needed for temporary scratch space in the L1, and
 * for the default CCSRBAR setting.  CCSRBAR is then relocated
 * to CFG_CCSRBAR right after these TLB entries are established
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
	.long (tlb_end-tlb_start)/16
tlb_start:

	/* TLB 0 */

	/* Non-Cacheable - Initial CCSRBAR mapping */
	.long TLB1_MAS0(0, 1, 0)
	.long TLB1_MAS1(1, 0, 0, 0, BOOKE_PAGESZ_4K)
	.long TLB1_MAS2(E500_TLB_EPN(CFG_CCSRBAR_DEFAULT), 0,0,0,0,1,0,1,0)
	.long TLB1_MAS3(E500_TLB_RPN(CFG_CCSRBAR_DEFAULT), 0,0,0,0,0,1,0,1,0,1)

	/*
	 * TLB0		16K	Cacheable, guarded
	 * Temporary Global data for initialization
	 *
	 * Use four 4K TLB0 entries.  These entries must be cacheable
	 * as they provide the bootstrap memory before the memory
	 * controler and real memory have been configured.
	 *
	 * These entries end up at TLB0 Indicies 0x10, 0x14, 0x18 and 0x1c,
	 * and must not collide with other TLB0 entries.
	 */
	.long TLB1_MAS0(0, 0, 0)
	.long TLB1_MAS1(1, 0, 0, 0, BOOKE_PAGESZ_4K)
	.long TLB1_MAS2(E500_TLB_EPN(CFG_INIT_RAM_ADDR), 0,0,0,0,0,0,1,0)
	.long TLB1_MAS3(E500_TLB_RPN(CFG_INIT_RAM_ADDR), 0,0,0,0,0,1,0,1,0,1)

	.long TLB1_MAS0(0, 0, 0)
	.long TLB1_MAS1(1, 0, 0, 0, BOOKE_PAGESZ_4K)
	.long TLB1_MAS2(E500_TLB_EPN(CFG_INIT_RAM_ADDR + 4 * 1024),
			0,0,0,0,0,0,1,0)
	.long TLB1_MAS3(E500_TLB_RPN(CFG_INIT_RAM_ADDR + 4 * 1024),
			0,0,0,0,0,1,0,1,0,1)

	.long TLB1_MAS0(0, 0, 0)
	.long TLB1_MAS1(1, 0, 0, 0, BOOKE_PAGESZ_4K)
	.long TLB1_MAS2(E500_TLB_EPN(CFG_INIT_RAM_ADDR + 8 * 1024),
			0,0,0,0,0,0,1,0)
	.long TLB1_MAS3(E500_TLB_RPN(CFG_INIT_RAM_ADDR + 8 * 1024),
			0,0,0,0,0,1,0,1,0,1)

	.long TLB1_MAS0(0, 0, 0)
	.long TLB1_MAS1(1, 0, 0, 0, BOOKE_PAGESZ_4K)
	.long TLB1_MAS2(E500_TLB_EPN(CFG_INIT_RAM_ADDR + 12 * 1024),
			0,0,0,0,0,0,1,0)
	.long TLB1_MAS3(E500_TLB_RPN(CFG_INIT_RAM_ADDR + 12 * 1024),
			0,0,0,0,0,1,0,1,0,1)

	/* TLB 1 */
	/* Non-cacheable, guarded - Covers boot page */
	.long TLB1_MAS0(1, 0, 0)
	.long TLB1_MAS1(1, 1, 0, 0, BOOKE_PAGESZ_4K)
	.long TLB1_MAS2(E500_TLB_EPN(0xfffff000), 0,0,0,0,1,0,1,0)
	.long TLB1_MAS3(E500_TLB_RPN(0xfffff000), 0,0,0,0,0,1,0,1,0,1)

	/* Non-cacheable, guarded - CCSRBAR */
	.long TLB1_MAS0(1, 1, 0)
	.long TLB1_MAS1(1, 1, 0, 0, BOOKE_PAGESZ_1M)
	.long TLB1_MAS2(E500_TLB_EPN(CFG_CCSRBAR), 0,0,0,0,1,0,1,0)
	.long TLB1_MAS3(E500_TLB_RPN(CFG_CCSRBAR), 0,0,0,0,0,1,0,1,0,1)

	/* Cacheable - Flash - remapped Non-cacheable in board_early_init_r() */
	.long TLB1_MAS0(1, 2, 0)
	.long TLB1_MAS1(1, 1, 0, 0, BOOKE_PAGESZ_4M)
	.long TLB1_MAS2(E500_TLB_EPN(CFG_FLASH_BASE), 0,0,0,1,0,0,1,0)
	.long TLB1_MAS3(E500_TLB_RPN(CFG_FLASH_BASE), 0,0,0,0,0,1,0,1,0,1)

	/* Non-cacheable - SYSPLD */
	.long TLB1_MAS0(1, 3, 0)
	.long TLB1_MAS1(1, 1, 0, 0, BOOKE_PAGESZ_4K)
	.long TLB1_MAS2(E500_TLB_EPN(CFG_SYSPLD_BASE), 0,0,0,0,1,0,1,0)
	.long TLB1_MAS3(E500_TLB_RPN(CFG_SYSPLD_BASE), 0,0,0,0,0,1,0,1,0,1)

	/* Non-cacheable - NVRAM - last 4K (RE Part) */
	.long TLB1_MAS0(1, 4, 0)
	.long TLB1_MAS1(1, 1, 0, 0, BOOKE_PAGESZ_4K)
	.long TLB1_MAS2(E500_TLB_EPN(CFG_NVRAM_BASE + CFG_NVRAM_SIZE - 4096), 0,0,0,0,1,0,1,0)
	.long TLB1_MAS3(E500_TLB_RPN(CFG_NVRAM_BASE + CFG_NVRAM_SIZE - 4096), 0,0,0,0,0,1,0,1,0,1)
tlb_end:
	entry_end

/*
 * LAW(Local Access Window) configuration:
 *
 * Notes:
 *    CCSRBAR and L2-as-SRAM don't need a configured Local Access Window.
 *
 * LAW 0 is reserved for boot mapping
 */

	.section .bootpg, "ax"
	.globl	law_entry
law_entry:
	entry_start

	.long (law_end-law_start)/8
law_start:

	/* Reserve LAW1 for DDR but don't enable it. Defer that untill RAM size is known */
	.long	0
	.long	~LAWAR_EN | LAWAR_TRGT_DDR | (LAWAR_SIZE & LAWAR_SIZE_2G)

	.long	(CFG_FLASH_BASE>>12) & 0xfffff
	.long	LAWAR_EN | LAWAR_TRGT_LBC | (LAWAR_SIZE & LAWAR_SIZE_4M)

	.long	(CFG_COP_FPGA_BASE >> 12) & 0xfffff
	.long	LAWAR_EN | LAWAR_TRGT_LBC | (LAWAR_SIZE & LAWAR_SIZE_64K)

	.long	(CFG_NVRAM_BASE >> 12) & 0xfffff
	.long	LAWAR_EN | LAWAR_TRGT_LBC | (LAWAR_SIZE & LAWAR_SIZE_128K)

	.long	(CFG_SYSPLD_BASE >> 12) & 0xfffff
	.long	LAWAR_EN | LAWAR_TRGT_LBC | (LAWAR_SIZE & LAWAR_SIZE_32K)

	.long	(CFG_PCIE1_MEM_PHYS>>12) & 0xfffff
	.long	LAWAR_EN | LAWAR_TRGT_PCIE1 | (LAWAR_SIZE & LAWAR_SIZE_512M)

law_end:
	entry_end
