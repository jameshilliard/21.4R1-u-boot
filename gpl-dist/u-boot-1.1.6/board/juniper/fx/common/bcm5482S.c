/*
 * Copyright (c) 2011-2013, Juniper Networks, Inc.
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

#include <rmi/rmigmac.h>
#include <rmi/gmac.h>
#include <rmi/on_chip.h>
#include <rmi/xlr_cpu.h>
#include <rmi/shared_structs.h>
#include <rmi/cpu_ipc.h>
#include "bcm54xxx.h"
#include "bcm5482S.h"
#include "fx_common.h"
#include <soho/soho_cpld.h>
#include <soho/soho_cpld_map.h>

extern char bcm_chip_string[32];
#define BCM54XXX_SEC_SERDES_RESET     (1 << 15)

BOOLEAN bcm54xxx_is_type_5482S (void)
{
    uint8_t data1 = 0;
    uint8_t data2 = 0;

    if (!fx_is_tor()) {
        return FALSE;
    }

    soho_cpld_read(SOHO_SCPLD_SFP_C0_PRE, &data1);
    soho_cpld_read(SOHO_SCPLD_SFP_C1_PRE, &data2);
    if ((data1 == data2) && (data2 == 0xff)) {
        b2_debug("CPLD Regsiters 0xf8 and 0xf9 are set to 0xFF\n");
        return FALSE;
    }
    return TRUE;
}

void bcm5482S_sfp_reset (gmac_eth_info_t *eth_info)
{
    if (bcm54xxx_access_is_valid(eth_info) != TRUE) {
        b2_debug("%s: %s access is not valid\n", __func__, bcm_chip_string);
        return;
    }

    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_EXP_REG, 
    BCM54XXX_SEC_SERDES_SELECT << BCM54XXX_SEC_SERDES_SELECT_SHIFT);
    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_EXP_WRITE_REG, 
                       BCM54XXX_SEC_SERDES_RESET);
    udelay(900000); /* 900ms */
}

void bcm5482S_sfp_powerup (gmac_eth_info_t *eth_info)
{
    uint32_t data;

    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_EXP_REG, 
    BCM54XXX_SEC_SERDES_SELECT << BCM54XXX_SEC_SERDES_SELECT_SHIFT);
    BCM54XXX_PHY_READ(eth_info, BCM54XXX_EXP_WRITE_REG, &data);
 
    data = data & ~0x0800;
    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_EXP_REG, 
    BCM54XXX_SEC_SERDES_SELECT << BCM54XXX_SEC_SERDES_SELECT_SHIFT);
    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_EXP_WRITE_REG, data);
    udelay(900000); /* 900ms */
}

void bcm5482S_sfp_sigdetect (gmac_eth_info_t *eth_info)
{
    uint32_t data;

    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, 
                       (0x14 << BCM54XXX_SHADOW_SHIFT_BIT));
    BCM54XXX_PHY_READ(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL,&data);
    data |= 0x800E;
    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, 
                       data);
}

void bcm5482S_sfp_enable_led_indicator (gmac_eth_info_t *eth_info)
{
    uint32_t data;

    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, 
                       (0x0D << BCM54XXX_SHADOW_SHIFT_BIT));
    BCM54XXX_PHY_READ(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL,&data);

    data |= 0x8033;
    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, 
                       data);
}

BOOLEAN bcm5482S_sfp_getduplexmode (gmac_eth_info_t *eth_info)
{
    uint32_t value = 0;

    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, 
                       (0x1C << BCM54XXX_SHADOW_SHIFT_BIT));
    BCM54XXX_PHY_READ(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL,&value);
    if (value & PHY_IS_IN_DUPLEX_MODE)
        return TRUE;

    return FALSE;
}

void bcm5482S_init_as_sfp (gmac_eth_info_t *eth_info)
{
    uint32_t data;

    /* Enable Sec Serdes 1000BASE-X */
    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, 
                       (0x14 << BCM54XXX_SHADOW_SHIFT_BIT));
    BCM54XXX_PHY_READ(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, 
                      &data);
    data |= 0x8001;
    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_SHADOW_ENABLE_CTRL, 
                       data);

    /* Reset Sec Serdes*/
    bcm5482S_sfp_reset(eth_info);
    /* Power Up Sec Serdes */
    bcm5482S_sfp_powerup(eth_info);
    /* Select Signal Detect and Sync State */
    bcm5482S_sfp_sigdetect(eth_info);
    /* Enable LED indicator */
    bcm5482S_sfp_enable_led_indicator(eth_info);

}

BOOLEAN bcm5482S_sfp_linkup (gmac_eth_info_t *eth_info)
{
    uint32_t data;

    BCM54XXX_PHY_WRITE(eth_info, BCM54XXX_EXP_REG, 
                       (BCM54XXX_SEC_SERDES_SELECT << 
                       BCM54XXX_SEC_SERDES_SELECT_SHIFT) | 
                       BCM54XXX_SHADOW_ENABLE_CTRL);
    BCM54XXX_PHY_READ(eth_info, BCM54XXX_EXP_WRITE_REG, &data);
    b2_debug("%s: data=0x%x\n", __func__, data);

    if (data & 0x04) {
        return TRUE;
    } 
    return FALSE;
}
