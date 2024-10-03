/*
 * Copyright 2004 Freescale Semiconductor.
 * Copyright (C) 2003 Motorola Inc.
 * Xianghua Xiao (x.xiao@motorola.com)
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
 * PCI Configuration space access support for ARM PCI Bridge
 */

#include <common.h>
#include "pex/mvPexRegs.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include <pcie.h>

extern void pcie_setup_indirect(struct pcie_controller* hose, unsigned int cfg_addr, unsigned int cfg_data);

void pcie_arm_init(struct pcie_controller *hose, PARAM_PCIE *pcie_info, uint pex)
{

    hose->first_busno = 0;
    hose->last_busno = 0x7f;

    pcie_set_region(hose->regions + 0,
		       pcie_info->pcie_mem_base,
		       pcie_info->pcie_mem_base,
		       pcie_info->pcie_mem_size,
		       PCIE_REGION_MEM);

    hose->region_count = 1;
    pcie_setup_indirect(hose,
			   INTER_REGS_BASE | PEX_CFG_ADDR_REG(pex),
			   INTER_REGS_BASE | PEX_CFG_DATA_REG(pex));


#ifdef PCIE_DEBUG       
    printf("registering hose...\r\n");
#endif
    pcie_register_hose(hose);

    hose->last_busno = pcie_hose_scan(hose);

}
