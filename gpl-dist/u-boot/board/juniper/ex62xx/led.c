/*
 * Copyright (c) 2011, Juniper Networks, Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/gpl-2.0.html>.
 */

#include <common.h>
#include <ex62xx_common.h>
#include <led.h>
#include <tsec.h>

extern struct tsec_private *mgmt_priv;
DECLARE_GLOBAL_DATA_PTR;

void set_port_led(uint32_t link_stat_spd)
{
	uint16_t expansion_reg_04h = 0, tmp;
	uint link, link_speed;
	uint32_t mgmt_phy_is_copper = TRUE;	/* select copper by default */
	uint32_t val, val_t;

	write_phy_reg(mgmt_priv, MIIM_BCM54XX_SHD, MIIM_BCM54XX_MODE_CTRL);
	val = read_phy_reg(mgmt_priv, MIIM_BCM54XX_SHD);
	if (val & BRGPHY_1C_MCR_INTFSEL_RGMII_FIBER) {
	    mgmt_phy_is_copper = 0;
	    val |= BRGPHY_1C_MCR_1000BASEX_REG_EN;
	    link_stat_spd &= ~(0x0700);
	    link_stat_spd |= (MIIM_BCM5466R_PHY_AUX_STATUS_FULLDUPLEX_1000 << 8);
	    val_t = read_phy_reg(mgmt_priv, MIIM_STATUS); 
	    link_stat_spd |= MIIM_STATUS_LINK;
	} else {
	    mgmt_phy_is_copper = 1;
	    val &= ~(BRGPHY_1C_MCR_1000BASEX_REG_EN);
	}

	/* select copper/fiber registers */
	write_phy_reg(mgmt_priv, MIIM_BCM54XX_SHD, val | MIIM_BCM54XX_SHD_WRITE);

	link = link_stat_spd & 0x4;
	link_speed = ((link_stat_spd & 0x0700) >> 8);

	if (!link) { /* LINK DOWN */
		link_speed = SPEED_LED_OFF;		
	}

	switch (link_speed) {
	case MIIM_BCM5466R_PHY_AUX_STATUS_FULLDUPLEX_1000: 	/* LED ON */
	case MIIM_BCM5466R_PHY_AUX_STATUS_HALFDUPLEX_1000:
		expansion_reg_04h = MULTICOLOR_LED_FORCED_ON;
		break;
	case MIIM_BCM5466R_PHY_AUX_STATUS_FULLDUPLEX_100:	/* LED Blink */
	case MIIM_BCM5466R_PHY_AUX_STATUS_HALFDUPLEX_100:
		expansion_reg_04h = MULTICOLOR_LED_ALTERNATING;
		break;
	case MIIM_BCM5466R_PHY_AUX_STATUS_FULLDUPLEX_10:	/* LED OFF */
	case MIIM_BCM5466R_PHY_AUX_STATUS_HALFDUPLEX_10:
	case SPEED_LED_OFF:
	default:
		expansion_reg_04h = MULTICOLOR_LED_FORCED_OFF;
		break;
	}
	/* 0x17 = 0x0F04 - select exp reg 0x4 */
	write_phy_reg(mgmt_priv, MIIM_BCM54XX_EXP_SEL, 
	    (MIIM_BCM54XX_EXP_SEL_ER + BCM_EXP_REG_MULTICOLOR_SEL));

	/* 0x15(exp reg 0x4) = Link speed */
	write_phy_reg(mgmt_priv, MIIM_BCM54XX_EXP_DATA, expansion_reg_04h);

	/* 0x17 = 0x0 */
	write_phy_reg(mgmt_priv, MIIM_BCM54XX_EXP_SEL, (0x00));


	/* select the mgmt port LED */
	ex62xxcb_phy_led_select(mgmt_phy_is_copper);
}
