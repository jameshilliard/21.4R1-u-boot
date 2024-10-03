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

#ifndef __POST_MAC_H__
#define __POST_MAC_H__

#define MAX_PACKETS                256
#define DEFAULT_SPILL_ENTRIES      (MAX_PACKETS >> 1)
#define MAX_FRIN_SPILL_ENTRIES     DEFAULT_SPILL_ENTRIES
#define MAX_FROUT_SPILL_ENTRIES    DEFAULT_SPILL_ENTRIES
#define MAX_RX_SPILL_ENTRIES       DEFAULT_SPILL_ENTRIES
#define CACHELINE_SIZE             32
#define AUTONEG

extern void gmac_phy_init (gmac_eth_info_t *this_phy);
extern int gmac_phy_read (gmac_eth_info_t *this_phy, int reg, uint32_t *data);
extern int gmac_phy_write (gmac_eth_info_t *this_phy, int reg, uint32_t data);
extern void gmac_clear_phy_intr (gmac_eth_info_t *this_phy);
extern uint32_t fx_get_gmac_phy_id (unsigned int mac, int type);

extern unsigned long fx_get_gmac_mmio_addr (unsigned int gmac);
extern unsigned long fx_get_gmac_mii_addr (unsigned int mac);
extern unsigned long fx_get_gmac_pcs_addr (unsigned int mac);
extern unsigned long fx_get_gmac_serdes_addr (unsigned int mac);


#endif /*__POST_MAC_H__*/
