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

/* Chun Ng cng@juniper.net */

#include <rmi/rmigmac.h>
#include <rmi/gmac.h>
#include <rmi/on_chip.h>
#include <rmi/xlr_cpu.h>
#include <rmi/shared_structs.h>
#include <rmi/cpu_ipc.h>
#include "bcm54xxx.h"
#include "fx_common.h"

char *bcm_chip_string[] = {
    "unknown",
    "BCM54980",
    "BCM54640",
    "BCM5482",
    "BCM5482S",
};

BOOLEAN
bcm54xxx_access_is_valid (gmac_eth_info_t *eth_info)
{
    uint32_t data;

    data = bcm54xxx_get_identifier(eth_info);

    b2_debug("%s: %s identifier is 0x%x\n", __func__, 
             bcm_chip_string[eth_info->phy_type], data);
    switch (eth_info->phy_type) {
    case BCM54980:
        if (((data & 0x3F0) >> 4) == BCM54980_MODEL &&
            (((data & 0xFC00) >> 10) == BCM54980_OUI)) {
            return TRUE;
        } else {
            return FALSE;
        }
        break;

    case BCM54640:
       if (((data & 0xFFFF) == 0x5DB1) ||
           (data & 0xFFF0) == 0x5E70) {
            return TRUE;
        } else {
            return FALSE;
       }
       break;
            
    case BCM5482:
    case BCM5482S:
        if ((data & 0xFFF0) == 0xBCB0) {
            return TRUE;
        } else {
            return FALSE;
        }
        break;


    default:
        return FALSE;
    }
}

void
bcm54xxx_reset (gmac_eth_info_t *eth_info)
{
    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_CTRL, 0x8000);
    udelay(1000);
}

uint32_t
bcm54xxx_get_oui (gmac_eth_info_t *eth_info)
{
    uint32_t data;

    BCM54XXX_PHY_READ(eth_info, BCM54XXX_PHYID_MSB, &data);

    return data;
}

uint32_t
bcm54xxx_get_identifier (gmac_eth_info_t *eth_info)
{
    uint32_t data;

    BCM54XXX_PHY_READ(eth_info, BCM54XXX_PHYID_LSB, &data);

    return data;
}

void
bcm54xxx_sgmii_access (gmac_eth_info_t *eth_info, BOOLEAN enable,
                       BOOLEAN write_enable)
{
    uint32_t data;

    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL,
                   (0x1F << BCM54XXX_SHADOW_SHIFT_BIT));
    udelay(1000 * 900);
    BCM54XXX_PHY_READ(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, &data);
    b2_debug("%s[%d]: data=0x%x\n", __func__, __LINE__, data);

    if (enable == TRUE) {
        data |= 0x1;
    } else {
        data &= ~0x1;
    }

    if (write_enable == TRUE) {
        data |= 0x8000;
    } else {
        data &= ~0x8000;
    }
    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, data);
}


uint32_t
bcm54xxx_copper_autoneg (gmac_eth_info_t *eth_info)
{
    uint32_t data;

    if (bcm54xxx_access_is_valid(eth_info) != TRUE) {
        b2_debug("%s: %s access is not valid\n", __func__, 
                 bcm_chip_string[eth_info->phy_type]);
        return 0;
    }

    BCM54XXX_PHY_READ(eth_info, BCM54XXX_CTRL, &data);
    data |= (1 << 12) | (1 << 9);
    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_CTRL, data);
    udelay(3000);
    return 1;
}

uint32_t
bcm54xxx_sgmii_autoneg_ctrl (gmac_eth_info_t *eth_info, BOOLEAN autoneg)
{
    uint32_t data;

    if (bcm54xxx_access_is_valid(eth_info) != TRUE) {
        b2_debug("%s: %s access is not valid\n", __func__, 
                 bcm_chip_string[eth_info->phy_type]);
        return 0;
    }

    bcm54xxx_sgmii_access(eth_info, TRUE, TRUE);

    BCM54XXX_PHY_READ(eth_info, BCM54XXX_CTRL, &data);
    if (autoneg == TRUE) {
        data |= (1 << 12) | (1 << 9);
        b2_debug("%s: AutoNeg is turned on\n", __func__);
    } else {
        data &= ~(1 << 12);
        b2_debug("%s: AutoNeg is disabled\n", __func__);
    }

    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_CTRL, data);
    udelay(3000);

    bcm54xxx_sgmii_access(eth_info, FALSE, TRUE);
    return 1;
}


BOOLEAN
bcm54xxx_sgmii_linkup (gmac_eth_info_t *eth_info)
{
    uint32_t data;

    if (bcm54xxx_access_is_valid(eth_info) != TRUE) {
        b2_debug("%s: %s access is not valid\n", __func__, 
                 bcm_chip_string[eth_info->phy_type]);
        return FALSE;
    }

    bcm54xxx_sgmii_access(eth_info, TRUE, TRUE);
    BCM54XXX_PHY_READ(eth_info, BCM54XXX_STATUS, &data);

    b2_debug("%s: data=0x%x\n", __func__, data);
    if ( (data >> 2) & 0x1) {
        bcm54xxx_sgmii_access(eth_info, FALSE, TRUE);
        return TRUE;
    } else {
        bcm54xxx_sgmii_access(eth_info, FALSE, TRUE);
        return FALSE;
    }
}


/*
 * bcm54xxx_sgmii_status: return sgmii status
 */
uint32_t
bcm54xxx_sgmii_status (gmac_eth_info_t *eth_info)
{
    uint32_t data;

    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL,
                   (0x15 << BCM54XXX_SHADOW_SHIFT_BIT ));

    BCM54XXX_PHY_READ(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, &data);

    return data;
}


BOOLEAN
bcm54xxx_lineside_linkup(gmac_eth_info_t *eth_info)
{
    uint32_t data;
    BOOLEAN ret;

    if (bcm54xxx_access_is_valid(eth_info) != TRUE) {
        b2_debug("%s: %s access is not valid\n", __func__, 
                 bcm_chip_string[eth_info->phy_type]);
        return FALSE;
    }

    bcm54xxx_sgmii_access(eth_info, FALSE, TRUE);

    if (fx_is_sfp_port(eth_info->port_id)) {
        BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, 
                           0x14 << BCM54XXX_SHADOW_SHIFT_BIT);
        BCM54XXX_PHY_READ(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, &data);
        if (data & (0x1 << 8)) {
            ret = TRUE;
        } else {
            ret = FALSE;
        }
    } else {
        BCM54XXX_PHY_READ(eth_info, BCM54XXX_STATUS, &data);
     
        b2_debug("%s: data=0x%x\n", __func__, data);
        if ((data >> 2) & 0x1) {
            ret = TRUE;
        } else {
            ret = FALSE;
        }
    }

    b2_debug("%s: data=0x%x\n", __func__, data);
    return ret; 
}


uint32_t
bcm54xxx_copper_recv_err_cnt (gmac_eth_info_t *eth_info)
{
    uint32_t data;

    if (bcm54xxx_access_is_valid(eth_info) != TRUE) {
        b2_debug("%s: %s access is not valid\n", __func__, 
                 bcm_chip_string[eth_info->phy_type]);
        return 0;
    }

    bcm54xxx_sgmii_access(eth_info, FALSE, FALSE);

    BCM54XXX_PHY_READ(eth_info, BCM54XXX_COPPER_RCV_ERR_CNT, &data);
    return data;
}


uint32_t
bcm54xxx_copper_pkt_cnt (gmac_eth_info_t *eth_info)
{
    uint32_t data;

    if (bcm54xxx_access_is_valid(eth_info) != TRUE) {
        printf("%s: %s access is not valid\n", __func__, 
               bcm_chip_string[eth_info->phy_type]);
        return 0;
    }

    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_EXP_REG, 0xF00);

    BCM54XXX_PHY_READ(eth_info, 0x15, &data);

    return data;
}

uint32_t
bcm54xxx_set_led (gmac_eth_info_t *eth_info, bcm54xxx_selector_t *selector)
{
    uint16_t data; 

    if (!eth_info || !selector) {
        return 0;
    }

    switch (eth_info->phy_type) {
    case BCM54640:
    case BCM54980:
        data = (1 << 15) | (0xD << BCM54XXX_SHADOW_SHIFT_BIT) | (selector->led_0 & 0xF) | 
                ((selector->led_1 & 0xF) << 4);

        BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, data);

        data = (1 << 15) | (0xE << BCM54XXX_SHADOW_SHIFT_BIT) | (selector->led_2 & 0xF) | 
                ((selector->led_3 & 0xF) << 4);

        BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, data);
        break;

    case BCM5482:
    case BCM5482S:
        data = (1 << 15) | (0xD << BCM54XXX_SHADOW_SHIFT_BIT) | (selector->led_0 & 0xF) | 
                ((selector->led_2 & 0xF) << 4);

        BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, data);

        data = (1 << 15) | (0xE << BCM54XXX_SHADOW_SHIFT_BIT) | (selector->led_3 & 0xF) | 
                ((selector->led_1 & 0xF) << 4);

        BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, data);
        break;

    default:
        printf("%s: unknown chip type %d\n", __func__, eth_info->phy_type);
        return 0;
    }

    return 1;
}

uint32_t
bcm54xxx_all_spd_enable (gmac_eth_info_t *eth_info)
{
    uint16_t data; 

    if (!eth_info) {
        return 0;
    }

    data = 0x1e1;
    BCM54XXX_PHY_WRITE(eth_info, 0x4, data);

    BCM54XXX_PHY_READ(eth_info, 0x9, &data);
    data |= 0x3 << 8;
    BCM54XXX_PHY_WRITE(eth_info, 0x9, data);

    return 1;
}

void
bcm54640_goto_serdes_mode (gmac_eth_info_t *eth_info)
{
    uint32_t data;
    
    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, 
                                            (0x1F << BCM54XXX_SHADOW_SHIFT_BIT));
    BCM54XXX_PHY_READ(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, &data);

    data |= 0x1 | (1 << 15);

    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, data);
}

void
bcm54640_goto_pri_serdes_mode (gmac_eth_info_t *eth_info)
{
    uint32_t data;

    bcm54640_goto_serdes_mode(eth_info);

    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, 0xd040);

    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, 0xd020); 

    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, 
                                            (0x1F << BCM54XXX_SHADOW_SHIFT_BIT));

    BCM54XXX_PHY_READ(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, &data);

    udelay(1000 * 1000);
}

void
bcm54640_goto_sec_serdes_mode (gmac_eth_info_t *eth_info)
{
    bcm54640_goto_serdes_mode(eth_info);

    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, 0xd041); 

    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, 0xd001); 

    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, 0xd000); 

    udelay(1000 * 1000);
}

