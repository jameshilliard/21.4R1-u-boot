/*
 * $Id$
 *
 * acx.c -- Platform specific code for the Juniper ACX Product Family
 *
 * Samuel Jacob, Sep 2011
 *
 * Copyright (c) 2011-2012, Juniper Networks, Inc.
 * All rights reserved.
 *
 * Copyright 2008-2009 Freescale Semiconductor, Inc.
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
#include <asm/fsl_law.h>
#include <asm/mmu.h>

struct law_entry law_table[] = {
	SET_LAW(CONFIG_SYS_SDRAM_BASE, LAW_SIZE_2G, LAW_TRGT_IF_DDR),
	SET_LAW(CONFIG_SYS_FLASH_BASE, LAW_SIZE_4M, LAW_TRGT_IF_LBC),
	SET_LAW(CONFIG_SYS_COP_FPGA_BASE, LAW_SIZE_64K, LAW_TRGT_IF_LBC),
	SET_LAW(CONFIG_SYS_NVRAM_BASE, LAW_SIZE_128K, LAW_TRGT_IF_LBC),
	SET_LAW(CONFIG_SYS_SYSPLD_BASE, LAW_SIZE_32K, LAW_TRGT_IF_LBC),
	SET_LAW(CONFIG_SYS_PCIE1_MEM_PHYS, LAW_SIZE_512M, LAW_TRGT_IF_PCIE_1),
};

int num_law_entries = ARRAY_SIZE(law_table);
