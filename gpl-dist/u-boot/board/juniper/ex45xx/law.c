/*
 * Copyright (c) 2011-2013, Juniper Networks, Inc.
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

#ifdef CONFIG_EX4500
struct law_entry law_table[] = {
	SET_LAW(CONFIG_SYS_LBC_BASE, LAW_SIZE_16M, LAW_TRGT_IF_LBC),
#ifdef CONFIG_SYS_PCI1_MEM_PHYS
	SET_LAW(CONFIG_SYS_PCI1_MEM_PHYS, LAW_SIZE_256M, LAW_TRGT_IF_PCI),
	SET_LAW(CONFIG_SYS_PCI1_IO_PHYS, LAW_SIZE_1M, LAW_TRGT_IF_PCI),
#endif
#ifdef CONFIG_SYS_PCIE1_MEM_PHYS
	SET_LAW(CONFIG_SYS_PCIE1_MEM_PHYS, LAW_SIZE_1G, LAW_TRGT_IF_PCIE_1),
	SET_LAW(CONFIG_SYS_PCIE1_IO_PHYS, LAW_SIZE_1M, LAW_TRGT_IF_PCIE_1),
#endif
};
#endif

#ifdef CONFIG_EX4510
struct law_entry law_table[] = {
	SET_LAW(CONFIG_SYS_LBC_BASE, LAW_SIZE_16M, LAW_TRGT_IF_LBC),
#ifdef CONFIG_SYS_NAND_BASE_PHYS
	SET_LAW(CONFIG_SYS_NAND_BASE_PHYS, LAW_SIZE_1M, LAW_TRGT_IF_LBC),
#endif
#ifdef CONFIG_SYS_PCIE1_MEM_PHYS
	SET_LAW(CONFIG_SYS_PCIE1_MEM_PHYS, LAW_SIZE_1G, LAW_TRGT_IF_PCIE_1),
	SET_LAW(CONFIG_SYS_PCIE1_IO_PHYS, LAW_SIZE_1M, LAW_TRGT_IF_PCIE_1),
#endif
};
#endif

int num_law_entries = ARRAY_SIZE(law_table);
