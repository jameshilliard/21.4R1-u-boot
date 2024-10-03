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
 * PCI Configuration space access support for MPC85xx PCI Bridge
 */

#include <common.h>
#include <asm/immap_85xx.h>
#include <pcie.h>

extern void pcie_setup_indirect(struct pcie_controller* hose, unsigned int cfg_addr, unsigned int cfg_data);

void pcie_mpc85xx_init(struct pcie_controller *hose, PARAM_PCIE *pcie_info, uint pcie_offset)
{
    volatile immap_t    *immap = (immap_t *)CFG_CCSRBAR;
    volatile ccsr_pcie_t *pcie = NULL;
    volatile unsigned int temp;        
//    int i;

    pcie = (ccsr_pcie_t*)(CFG_CCSRBAR + pcie_offset);
    hose->first_busno = 0;
    hose->last_busno = 0xff;

    pcie_set_region(hose->regions + 0,
		       pcie_info->pcie_mem_base,
		       pcie_info->pcie_mem_base,
		       pcie_info->pcie_mem_size,
		       PCIE_REGION_MEM);

    hose->region_count = 1;
    pcie_setup_indirect(hose,
			   (CFG_CCSRBAR+pcie_offset),
			   (CFG_CCSRBAR+pcie_offset+4));

#ifdef PCIE_DEBUG       
    printf("registering hose...\r\n");
#endif
    pcie_register_hose(hose);


    if( ((immap->im_gur.porbmsr&0x00070000)>>16) !=7 ) /* TRUE : PCIE is configured in End Point mode */
    {
#define PCIE_TRANSLATION_ADDR_BASE 0x04000000 /* TARGET's DDR MEMORY BASE for EP PCIE memory space */ 

        /* init inbound window 1MB DDR memory */
        pcie->itar1  = (PCIE_TRANSLATION_ADDR_BASE>>12) & 0x000fffff;
        pcie->iwbar1 = 0x00000000;
        pcie->iwar1  = 0x80f55013;      /* Enable, Non Prefetch, Local Mem,
                                         * Snoop R/W, 1MB */
        /* init inbound window 1MB DDR memory */
        pcie->itar2  = ((PCIE_TRANSLATION_ADDR_BASE+0x100000)>>12) & 0x000fffff;
        pcie->iwbar2 = 0x00000000;
        pcie->iwar2  = 0x80f55013;       /* Enable,  Non Prefetch, Local Mem,
                                         * Snoop R/W, 1MB */
        /* init inbound window 1MB DDR memory */
        pcie->itar3  = ((PCIE_TRANSLATION_ADDR_BASE+0x200000)>>12) & 0x000fffff;
        pcie->iwbar3 = 0x00000000;
        pcie->iwar3  = 0x80f55013;       /* Enable,  Non Prefetch, Local Mem,
                                         * Snoop R/W, 1MB */
#if 0
        /* BUG FIX for PCIE EP mode */
        pcie_dev_t dev;
        dev = PCIE_BDF(0, 0, 0);
        pcie_read_config_dword(dev,0x4b0, &temp);
        temp |= 0x01;
        pcie_write_config_dword(dev,0x4b0,temp);
#endif
    }
    else{

	/* need to program the ATMU here */

	pcie->otar1   = (unsigned int)((pcie_info->pcie_mem_base >> 12) & 0x000fffff);
	temp = (pcie_info->pcie_mem_base >>32);
       temp &= 0xfffff; 
	pcie->otear1  = temp;
	pcie->owbar1  = (unsigned int)((pcie_info->pcie_mem_base >> 12) & 0x000fffff);
       pcie->owar1 = (POWAR_EN | POWAR_MEM_READ | POWAR_MEM_WRITE | POWAR_MEM_256M);

       pcie->iwar1 = (PIWAR_EN | PIWAR_PF | PIWAR_LOCAL |
				  PIWAR_READ_SNOOP | PIWAR_WRITE_SNOOP | POWAR_MEM_256M);
       pcie->itar1 = 0x00000000;
       pcie->iwbar1 = 0x00000000;
       pcie->iwar1 = (PIWAR_EN | PIWAR_PF | PIWAR_LOCAL |
			PIWAR_READ_SNOOP | PIWAR_WRITE_SNOOP | POWAR_MEM_256M);

#ifdef PCIE_DEBUG	
	printf("pci express outbount win tar %x: %x, bar %x, attr %x\r\n",pcie->otar1, pcie->otear1,pcie->owbar1, pcie->owar1);  
#endif
/*
#ifdef PCIE_DEBUG	
        printf("registering hose...\r\n");
#endif       
	pcie_register_hose(hose);
*/
#ifdef PCIE_DEBUG	

	        for(i=0x00; i<0x1000; i=i+4)
	{
//		pcie_hose_read_config_byte(hose, PCIE_BDF(0,0,0), i, &temp);
		pcie_hose_read_config_dword(hose, PCIE_BDF(0,0,0), i, &temp);
 		printf("PCI Express host : offset %x: data %x\r\n",i,temp);
	}

#endif	

	hose->last_busno = pcie_hose_scan(hose);


    }	/* endif if(PCIE EP) */	
}
