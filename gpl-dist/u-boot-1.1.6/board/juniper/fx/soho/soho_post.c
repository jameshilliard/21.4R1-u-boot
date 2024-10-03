/*
 * Copyright (c) 2010, Juniper Networks, Inc.
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
 * You should have received a copy of the GNU General Public
 * License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */


/*************************************************
  Soho specific post
  Author : Bilahari Akkiraju
**************************************************/

#include <rmi/types.h>
#include <common/post.h>
#include <soho/soho_post.h>

#include "soho_cpld.h"

/* Phy address on segment 1 & seg 7 */
static uint8_t *phyaddr;
static uint8_t *seg;

static uint8_t soho_segment[ ]  = {1, 7};
static uint8_t soho_phy_addr[ ] = {0, 1, 2, 3, 4, 5, 6, 7};

/* SHOHO specific POST list */
diag_t soho_post_tests[ ] =
{
    SOHO_POST_I2C_PROBE_TEST,
#ifdef SOHO_EMI    
    SOHO_POST_NETLOGIC_TEST,
#endif    
    POST_GMAC_LPBK_TEST,
    POST_USB_PROBE_TEST,
    POST_DMA_TXR_TEST,
    POST_MULT_CORE_MEM_TEST,
    POST_USB_STORAGE_TEST,
    POST_TEST_LIST_TERMINATOR
};

/* PA POST test list */
diag_t pa_post_tests[ ] =
{
    POST_GMAC_LPBK_TEST,
    POST_USB_PROBE_TEST,
    POST_DMA_TXR_TEST,
    POST_MULT_CORE_MEM_TEST,
    POST_USB_STORAGE_TEST,
    POST_TEST_LIST_TERMINATOR
};


int
soho_diag_post_nlogic_init (void)
{
   seg = soho_segment;
   phyaddr = soho_phy_addr;
   return 0;
}


int
soho_diag_post_nlogic_run (void)
{
    int i, j;

    printf("In net logic post run\n");
    printf("Initing and Enabling PRBS on Net logic phys\n");
  
    for (j = 0; j < NUM_SOHO_NLOGIC_SEGS; j++) {
       printf("Setting Segment %d\n",seg[j]);
       soho_mdio_set_10G_phy(seg[j]);
       for (i=0; i < PER_SEG_SOHO_NLOGICS; i++) {
          soho_cpld_mdio_c45_write(0x1, phyaddr[i], 0xCCC3, 0x3, 0x0); 
          soho_cpld_mdio_c45_write(0x1, phyaddr[i], 0xCC83, 0x3, 0x0); 
       }
    
       phyaddr += PER_SEG_SOHO_NLOGICS;
    }
    
    return 0;
}


int
soho_diag_post_nlogic_clean (void)
{
    int i, j;
/* Clean up if not running EMI */    
#ifndef SOHO_EMI
    printf("\nIn net logic post clean\n");
    
    for (j = 0; j < NUM_SOHO_NLOGIC_SEGS; j++) {
       soho_mdio_set_10G_phy(seg[j]);
       for (i=0; i < PER_SEG_SOHO_NLOGICS; i++) {
          soho_cpld_mdio_c45_write(0x1, phyaddr[i], 0xCCC3, 0x0, 0x0); 
          soho_cpld_mdio_c45_write(0x1, phyaddr[i], 0xCC83, 0x0, 0x0); 
       }
    
       phyaddr += PER_SEG_SOHO_NLOGICS;
    }
#endif    
    return 0;
}

