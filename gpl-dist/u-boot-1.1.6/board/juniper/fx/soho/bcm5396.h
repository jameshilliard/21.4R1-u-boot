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

#include "rmi/rmigmac.h"
#include "rmi/gmac.h"
#include "rmi/on_chip.h"
#include "rmi/xlr_cpu.h"
#include "rmi/shared_structs.h"
#include "rmi/cpu_ipc.h"


#ifndef _BCM5396_H_
#define _BCM5396_H_

typedef enum {
    BCM5396_OK = 0,
    BCM5396_INV_PORT = 1,
    BCM5396_NULL = 2,
    BCM5396_TIMEOUT = 3,
} bcm5396_status_t;

#define BCM5396_MAX_RETRY                       0x1F
#define BCM5396_WRITE_OP                        0x1
#define BCM5396_READ_OP                         0x2
#define BCM5396_OP_MASK                         0x3

#define BCM5396_MAX_PORT                        0xF
#define BCM5396_PAGE_BASE                       0x10 

#define BCM5396_MII_CTRL_REG                    0x0
#define BCM5396_MII_STATUS_REG                  0x1

#define BCM5396_ACC_CTRL_REG                    0x11
#define BCM5396_DATA_0_REG                      0x18
#define BCM5396_DATA_1_REG                      0x19
#define BCM5396_DATA_2_REG                      0x1A
#define BCM5396_DATA_3_REG                      0x1B

#define BCM5396_SERDE_SGMII_REG                 0x20

#define BCM5396_PAGE_REG                        0x10

#define BCM5396_MII_ADDR                        0x1E

u_int32_t bcm5396_lpb_ctrl (gmac_eth_info_t *eth_info, u_int32_t lpb_en);
bcm5396_status_t bcm5396_read(gmac_eth_info_t *eth_info, uint32_t reg_id, 
                               u_int32_t *data);
bcm5396_status_t bcm5396_write(gmac_eth_info_t *eth_info, u_int32_t reg_id, 
                               u_int16_t data);
void bcm5396_reset(u_int32_t gmac_port);
BOOLEAN bcm5396_sgmii_linkup(u_int32_t gmac_id);




extern int gmac_phy_read(gmac_eth_info_t *this_phy, int reg, uint32_t *data);
extern int gmac_phy_write(gmac_eth_info_t *this_phy, int reg, uint32_t data);


#endif
