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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. if not ,see
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0,html>  
 */


#ifndef _msc_fsc_phy_h_
#define _msc_fsc_phy_h_

/* phy register offsets */
#define PHY_X_BMCR              0x00 /* Control Register */
#define PHY_X_BMSR              0x01 /* Status Register */
#define PHY_X_PHYIDR1           0x02 /* PHY Identifier  Register */
#define PHY_X_PHYIDR2           0x03 /* PHY Identifier  Register */
#define PHY_X_ANAR              0x04 /* Auto Negotiation Advertisement Register */
#define PHY_X_ANLPAR            0x05 /* Link Partner Advertisement Register */


/* phy speed setup */
#define AUTO                    99
#define _1000BASET              1000
#define _100BASET               100
#define _10BASET                10
#define HALF                    22
#define FULL                    44


/* phy BMSR */
#define PHY_X_BMSR_100T4        0x8000
#define PHY_X_BMSR_100TXF       0x4000
#define PHY_X_BMSR_100TXH       0x2000
#define PHY_X_BMSR_10TF         0x1000
#define PHY_X_BMSR_10TH         0x0800
#define PHY_X_BMSR_PRE_SUP      0x0040
#define PHY_X_BMSR_AUTN_COMP    0x0020
#define PHY_X_BMSR_RF           0x0010
#define PHY_X_BMSR_AUTN_ABLE    0x0008
#define PHY_X_BMSR_LS           0x0004
#define PHY_X_BMSR_JD           0x0002
#define PHY_X_BMSR_EXT          0x0001

/*phy ANLPAR */
#define PHY_X_ANLPAR_NP         0x8000
#define PHY_X_ANLPAR_ACK        0x4000
#define PHY_X_ANLPAR_RF         0x2000
#define PHY_X_ANLPAR_T4         0x0200
#define PHY_X_ANLPAR_TXFD       0x0100
#define PHY_X_ANLPAR_TX         0x0080
#define PHY_X_ANLPAR_10FD       0x0040
#define PHY_X_ANLPAR_10         0x0020
#define PHY_X_ANLPAR_100        0x0380	    /* we can run at 100 */

#define PHY_X_ANLPAR_PSB_MASK   0x001f
#define PHY_X_ANLPAR_PSB_802_3  0x0001
#define PHY_X_ANLPAR_PSB_802_9  0x0002

/* PHY_1000BTSR */
#define PHY_X_1000BTSR_MSCF     0x8000
#define PHY_X_1000BTSR_MSCR     0x4000
#define PHY_X_1000BTSR_LRS      0x2000
#define PHY_X_1000BTSR_RRS      0x1000
#define PHY_X_1000BTSR_1000FD   0x0800
#define PHY_X_1000BTSR_1000HD   0x0400


/* PHY_1000BTSR */
#define PHY_1000BTSR_MSCF       0x8000
#define PHY_1000BTSR_MSCR       0x4000
#define PHY_1000BTSR_LRS        0x2000
#define PHY_1000BTSR_RRS        0x1000
#define PHY_1000BTSR_1000FD     0x0800
#define PHY_1000BTSR_1000HD     0x0400



#define PHY_X_ANXR              0x06 /* Auto Negotiation Extention Register */


#define PHY_X_1000BTCR          0x09
#define PHY_X_1000BTSR          0x0A

#define PHY_X_EXPSSR            0x1B /* Extend PHY Specific Status Register */

#define PHY_X_EXPSSR_HWCFG_MODE_MASK        0x0F    /* bit 3:0 - Hardware Configuration Mode*/
#define PHY_X_HWCFG_MODE_SGMII_CLK_AN_CP    0x00 /* SGMII with clock + SGMII AN Copper */
#define PHY_X_HWCFG_MODE_SGMII_NCLK_AN_CP   0x04 /* SGMII without clock + SGMII AN Copper */
#define PHY_X_HWCFG_MODE_1000BX_NCLK_AN_CP  0x08 /* 1000BASE_X without clock + 1000BASE-X AN Copper */


int fsc_phy_read (unsigned char bus, unsigned char dev, unsigned short reg, unsigned short *value);
int fsc_phy_write(unsigned char bus, unsigned char dev, unsigned short reg, unsigned short value);
int fsc_phy_info(unsigned char bus, unsigned char dev, unsigned int  *oui, unsigned char *model,
		 unsigned char *rev);
int fsc_phy_speed (unsigned char bus, unsigned char dev);
int fsc_phy_duplex (unsigned char bus, unsigned char dev);
int fsc_phy_link (unsigned char bus, unsigned char dev);

#endif /*_msc_fsc_phy_h_ */
