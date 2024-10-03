/*
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
 * You should have received a copy of the GNU General Public
 * License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __BCM5482S_H__
#define __BCM5482S_H__

#include <rmi/rmigmac.h>
#include <rmi/gmac.h>
#include <rmi/on_chip.h>
#include <rmi/xlr_cpu.h>
#include <rmi/shared_structs.h>
#include <rmi/cpu_ipc.h>
#include <soho/soho_cpld.h>
#include <soho/soho_cpld_map.h>

#define IS_SEREDES_LINK_GOOD 0x40
#define PHY_IS_IN_DUPLEX_MODE 0x08
#define MAC_SPEED_1000_HALF_DUPLEX 0x06
#define MAC_SPEED_1000_FULL_DUPLEX 0x07
#define AUTONEG_ADV_REG_VALUE_RJ45 0x1E1
#define ADVERTISE_1000BASET_CAP 0x300
#define BCM54XXX_CTRL_REG_VALUE 0x1340

BOOLEAN bcm54xxx_is_type_5482S (void);
void bcm5482S_sfp_reset (gmac_eth_info_t *eth_info);
void bcm5482S_init_as_sfp (gmac_eth_info_t *eth_info);
BOOLEAN bcm5482S_sfp_linkup (gmac_eth_info_t *eth_info);
BOOLEAN bcm5482S_sfp_getduplexmode (gmac_eth_info_t *eth_info);
#endif
