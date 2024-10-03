/*
 * Copyright (c) 2009-2012, Juniper Networks, Inc.
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

#ifndef __BCM54XXX__
#define __BCM54XXX__

#include <rmi/rmigmac.h>
#include <rmi/gmac.h>
#include <rmi/on_chip.h>
#include <rmi/xlr_cpu.h>
#include <rmi/shared_structs.h>
#include <rmi/cpu_ipc.h>


#define BCM54XXX_LED_LINK_SPD_1 0x0
#define BCM54XXX_LED_LINK_SPD_2 0x1
#define BCM54XXX_LED_ACTIVITY   0x3
#define BCM54XXX_LED_FORCE_OFF  0xE
#define BCM54XXX_LED_FORCE_ON   0xF

typedef enum {
    BCM54980 = 1,
    BCM54640,
    BCM5482,
    BCM5482S,
} bcm_chip_type_t;

typedef struct bcm54xxx_selector_
{
    uint16_t    led_0:      4;
    uint16_t    led_1:      4;
    uint16_t    led_2:      4;
    uint16_t    led_3:      4;
} bcm54xxx_selector_t;

#define BCM54980_MODEL                 0x2B
#define BCM54980_OUI                   0x2F

#define BCM54640_MODEL                 0x2A
#define BCM54640_OUI                   0x2F

#define BCM54XXX_CTRL                  0x0
#define BCM54XXX_STATUS                0x1
#define BCM54XXX_PHYID_MSB             0x2
#define BCM54XXX_PHYID_LSB             0x3
#define BCM54XXX_AUTONEG_ADV           0x4
#define BCM54XXX_1000BASET_CTRL        0x9
#define BCM54XXX_COPPER_RCV_ERR_CNT    0x12
#define BCM54XXX_EXP_WRITE_REG         0x15
#define BCM54XXX_EXP_REG               0x17
#define BCM54XXX_IRQ_STATUS            0x1A
#define BCM54XXX_IRQ_SEC_SERD_STATUS   0x15
#define BCM54XXX_SGMII_SLAVE_REG       0x14
#define BCM54XXX_SHADOW_ENABLE_CTRL    0x1C

#define BCM54XXX_SHADOW_SHIFT_BIT      10
#define BCM54XXX_SEC_SERDES_SELECT     0xE
#define BCM54XXX_SEC_SERDES_SELECT_SHIFT      0x08
#define BCM54XXX_SEC_SERDES_RESET      15

#define BCM54XXX_PHY_READ(eth_info, reg, data)                  \
    do {                                                        \
        gmac_phy_read(eth_info, reg, data);                     \
        if ((reg !=  BCM54XXX_IRQ_STATUS) &&                    \
            (reg !=  BCM54XXX_IRQ_SEC_SERD_STATUS)) {           \
            gmac_phy_read(eth_info, reg, data);                 \
        }                                                       \
    } while (0)

#define BCM54XXX_PHY_WRITE(gmac, reg, data)                     \
    gmac_phy_write(eth_info, reg, data);

extern uint8_t fx_gmac_verbose;

void bcm54xxx_reset(gmac_eth_info_t *eth_info);
u_int32_t bcm54xxx_get_identifier(gmac_eth_info_t *eth_info);
u_int32_t bcm54xxx_get_oui(gmac_eth_info_t *eth_info);
void bcm54xxx_sgmii_access(gmac_eth_info_t *eth_info, BOOLEAN enable,
                           BOOLEAN write_enable);
u_int32_t bcm54xxx_copper_autoneg(gmac_eth_info_t *eth_info);
u_int32_t bcm54xxx_copper_force_1g(gmac_eth_info_t *eth_info);
u_int32_t bcm54xxx_sgmii_autoneg_ctrl(gmac_eth_info_t *eth_info, BOOLEAN autoneg);
BOOLEAN bcm54xxx_sgmii_linkup(gmac_eth_info_t *eth_info);
u_int32_t bcm54xxx_sgmii_status(gmac_eth_info_t *eth_info);
BOOLEAN bcm54xxx_lineside_linkup(gmac_eth_info_t *eth_info);
u_int32_t bcm54xxx_copper_recv_err_cnt(gmac_eth_info_t *eth_info);
u_int32_t bcm54xxx_copper_pkt_cnt(gmac_eth_info_t *eth_info);
BOOLEAN bcm54xxx_access_is_valid(gmac_eth_info_t *eth_info);
uint32_t bcm54xxx_set_led(gmac_eth_info_t *eth_info, 
                          bcm54xxx_selector_t *selector);
uint32_t bcm54xxx_all_spd_enable(gmac_eth_info_t *eth_info);

extern int gmac_phy_read(gmac_eth_info_t *this_phy, int reg, uint32_t *data);
extern int gmac_phy_write(gmac_eth_info_t *this_phy, int reg, uint32_t data);
void bcm54640_goto_pri_serdes_mode(gmac_eth_info_t *eth_info);
void bcm54640_goto_sec_serdes_mode(gmac_eth_info_t *eth_info);

#endif
