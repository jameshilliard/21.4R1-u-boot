/***********************license start***************
 * Copyright (c) 2003-2008  Cavium Networks (support@cavium.com). All rights
 * reserved.
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Cavium Networks nor the names of
 *       its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 *
 * This Software, including technical data, may be subject to U.S.  export
 * control laws, including the U.S.  Export Administration Act and its
 * associated regulations, and may be subject to export or import regulations
 * in other countries.  You warrant that You will comply strictly in all
 * respects with all such regulations and acknowledge that you have the
 * responsibility to obtain licenses to export, re-export or import the
 * Software.
 *
 * TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
 * AND WITH ALL FAULTS AND CAVIUM NETWORKS MAKES NO PROMISES, REPRESENTATIONS
 * OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 * RESPECT TO THE SOFTWARE, INCLUDING ITS CONDITION, ITS CONFORMITY TO ANY
 * REPRESENTATION OR DESCRIPTION, OR THE EXISTENCE OF ANY LATENT OR PATENT
 * DEFECTS, AND CAVIUM SPECIFICALLY DISCLAIMS ALL IMPLIED (IF ANY) WARRANTIES
 * OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A PARTICULAR
 * PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET
 * POSSESSION OR CORRESPONDENCE TO DESCRIPTION.  THE ENTIRE RISK ARISING OUT
 * OF USE OR PERFORMANCE OF THE SOFTWARE LIES WITH YOU.
 *
 *
 * For any questions regarding licensing please contact marketing@caviumnetworks.com
 *
 ***********************license end**************************************/






/**
 * @file
 *
 * Helper functions to abstract board specific data about
 * network ports from the rest of the cvmx-helper files.
 *
 * <hr>$Revision: 1.4 $<hr>
 */
#include "cvmx.h"
#include "cvmx-app-init.h"
#include "cvmx-mdio.h"
#include "cvmx-sysinfo.h"
#include "cvmx-helper.h"
#include "cvmx-helper-util.h"
#include "cvmx-helper-board.h"
#ifdef CONFIG_JSRXNLE
#include "configs/jsrxnle.h"
#endif
#ifdef CONFIG_MAG8600
#include "configs/mag8600.h"
#include "msc_i2c.h"
#endif

/**
 * cvmx_override_board_link_get(int ipd_port) is a function
 * pointer. It is meant to allow customization of the process of
 * talking to a PHY to determine link speed. It is called every
 * time a PHY must be polled for link status. Users should set
 * this pointer to a function before calling any cvmx-helper
 * operations.
 */
CVMX_SHARED cvmx_helper_link_info_t (*cvmx_override_board_link_get)(int ipd_port) = NULL;

/**
 * Return the MII PHY address associated with the given IPD
 * port. A result of -1 means there isn't a MII capable PHY
 * connected to this port. On chips supporting multiple MII
 * busses the bus number is encoded in bits <15:8>.
 *
 * This function must be modifed for every new Octeon board.
 * Internally it uses switch statements based on the cvmx_sysinfo
 * data to determine board types and revisions. It relys on the
 * fact that every Octeon board receives a unique board type
 * enumeration from the bootloader.
 *
 * @param ipd_port Octeon IPD port to get the MII address for.
 *
 * @return MII PHY address and bus number or -1.
 */
int cvmx_helper_board_get_mii_address(int ipd_port)
{
#ifdef CONFIG_JSRXNLE
    DECLARE_GLOBAL_DATA_PTR;
    switch (gd->board_desc.board_type)
    {
        CASE_ALL_SRX240_MODELS
            return -1;
    }
#endif
#ifdef CONFIG_MAG8600
    /* no direct mii */
    return -1;
#endif
#ifdef CONFIG_ACX500_SVCS
    /* no direct mii */
    return -1;
#endif

    switch (cvmx_sysinfo_get()->board_type)
    {
        case CVMX_BOARD_TYPE_SIM:
            /* Simulator doesn't have MII */
            return -1;
        case CVMX_BOARD_TYPE_EBT3000:
        case CVMX_BOARD_TYPE_EBT5800:
        case CVMX_BOARD_TYPE_THUNDER:
        case CVMX_BOARD_TYPE_NICPRO2:
            /* Interface 0 is SPI4, interface 1 is RGMII */
            if ((ipd_port >= 16) && (ipd_port < 20))
                return ipd_port - 16;
            else
                return -1;
        case CVMX_BOARD_TYPE_KODAMA:
        case CVMX_BOARD_TYPE_EBH3100:
        case CVMX_BOARD_TYPE_HIKARI:
        case CVMX_BOARD_TYPE_CN3010_EVB_HS5:
        case CVMX_BOARD_TYPE_CN3005_EVB_HS5:
        case CVMX_BOARD_TYPE_CN3020_EVB_HS5:
#ifdef CONFIG_JSRXNLE
        CASE_ALL_SRX210_MODELS
        CASE_ALL_SRX220_MODELS
            /* Port 0 is WAN connected to a PHY, Port 1 is GMII connected to a
                switch */
            if (ipd_port == 0)
                return 4;
            else if (ipd_port == 1)
                return 9;
            else
                return -1;
#endif
        case CVMX_BOARD_TYPE_NAC38:
            /* Board has 8 RGMII ports PHYs are 0-7 */
            if ((ipd_port >= 0) && (ipd_port < 4))
                return ipd_port;
            else if ((ipd_port >= 16) && (ipd_port < 20))
                return ipd_port - 16 + 4;
            else
                return -1;
        case CVMX_BOARD_TYPE_EBH3000:
            /* Board has dual SPI4 and no PHYs */
            return -1;
        case CVMX_BOARD_TYPE_EBH5200:
            /* Board has 4 SGMII ports. The PHYs start right after the MII
                ports MII0 = 0, MII1 = 1, SGMII = 2-5 */
            if ((ipd_port >= 0) && (ipd_port < 4))
                return ipd_port+2;
            else
                return -1;
        case CVMX_BOARD_TYPE_EBH5600:
        case CVMX_BOARD_TYPE_EBH5601:
            /* Board has 8 SGMII ports. 4 connect out, two connect to a switch,
                and 2 loop to each other */
            if ((ipd_port >= 0) && (ipd_port < 4))
                return ipd_port+1;
            else if ((ipd_port >= 16) && (ipd_port < 20))
                return ipd_port - 16 + 5;
            else
                return -1;
        case CVMX_BOARD_TYPE_CUST_NB5:
            if (ipd_port == 2)
                return 4;
            else
                return -1;
        case CVMX_BOARD_TYPE_BBGW_REF:
                return -1;  /* No PHYs are connected to Octeon, everything is through switch */
    }

    /* Some unknown board. Sombody forgot to update this function... */
    cvmx_dprintf("cvmx_helper_board_get_mii_address: Unknown board type %d\n",
                 cvmx_sysinfo_get()->board_type);
    return -1;
}


/**
 * @INTERNAL
 * This function is the board specific method of determining an
 * ethernet ports link speed. Most Octeon boards have Marvell PHYs
 * and are handled by the fall through case. This function must be
 * updated for boards that don't have the normal Marvell PHYs.
 *
 * This function must be modifed for every new Octeon board.
 * Internally it uses switch statements based on the cvmx_sysinfo
 * data to determine board types and revisions. It relys on the
 * fact that every Octeon board receives a unique board type
 * enumeration from the bootloader.
 *
 * @param ipd_port IPD input port associated with the port we want to get link
 *                 status for.
 *
 * @return The ports link status. If the link isn't fully resolved, this must
 *         return zero.
 */
cvmx_helper_link_info_t __cvmx_helper_board_link_get(int ipd_port)
{
    cvmx_helper_link_info_t result;
    int phy_addr;
    int is_broadcom_phy = 0;

#if !defined(CONFIG_ACX500_SVCS)
    /* Give the user a chance to override the processing of this function */
    if (cvmx_override_board_link_get)
        return cvmx_override_board_link_get(ipd_port);
#endif

    /* Unless we fix it later, all links are defaulted to down */
    result.u64 = 0;

    /* This switch statement should handle all ports that either don't use
        Marvell PHYS, or don't support inband status */
    switch (cvmx_sysinfo_get()->board_type)
    {
        case CVMX_BOARD_TYPE_SIM:
            /* The simulator gives you a simulated 1Gbps full duplex link */
            result.s.link_up = 1;
            result.s.full_duplex = 1;
            result.s.speed = 1000;
            return result;
        case CVMX_BOARD_TYPE_EBH3100:
        case CVMX_BOARD_TYPE_CN3010_EVB_HS5:
        case CVMX_BOARD_TYPE_CN3005_EVB_HS5:
        case CVMX_BOARD_TYPE_CN3020_EVB_HS5:
            /* Port 1 on these boards is always Gigabit */
            if (ipd_port == 1)
            {
                result.s.link_up = 1;
                result.s.full_duplex = 1;
                result.s.speed = 1000;
                return result;
            }
            /* Fall through to the generic code below */
            break;
        case CVMX_BOARD_TYPE_SRX_100_LOWMEM:
        case CVMX_BOARD_TYPE_SRX_100_HIGHMEM:
        case CVMX_BOARD_TYPE_SRX_100H2:
        case CVMX_BOARD_TYPE_SRX_110B_VA:
        case CVMX_BOARD_TYPE_SRX_110H_VA:
        case CVMX_BOARD_TYPE_SRX_110B_VB:
        case CVMX_BOARD_TYPE_SRX_110H_VB:
        case CVMX_BOARD_TYPE_SRX_110B_WL:
        case CVMX_BOARD_TYPE_SRX_110H_WL:
        case CVMX_BOARD_TYPE_SRX_110H_VA_WL:
        case CVMX_BOARD_TYPE_SRX_110H_VB_WL:
        case CVMX_BOARD_TYPE_SRX_110H2_VA:
        case CVMX_BOARD_TYPE_SRX_110H2_VB:
        case CVMX_BOARD_TYPE_SRX_210_LOWMEM:
        case CVMX_BOARD_TYPE_SRX_210_HIGHMEM:
        case CVMX_BOARD_TYPE_SRX_210_POE:
        case CVMX_BOARD_TYPE_SRX_210_VOICE:
        case CVMX_BOARD_TYPE_SRX_210BE:
        case CVMX_BOARD_TYPE_SRX_210HE:
        case CVMX_BOARD_TYPE_SRX_210HE_POE:
        case CVMX_BOARD_TYPE_SRX_210HE_VOICE:
        case CVMX_BOARD_TYPE_SRX_210HE2:
        case CVMX_BOARD_TYPE_SRX_210HE2_POE:
        case CVMX_BOARD_TYPE_SRX_220_HIGHMEM:
        case CVMX_BOARD_TYPE_SRX_220_POE:
        case CVMX_BOARD_TYPE_SRX_220_VOICE:
        case CVMX_BOARD_TYPE_SRX_220H2:
        case CVMX_BOARD_TYPE_SRX_220H2_POE:

            /* Port 0 on these boards is always Gigabit */
            if (ipd_port == 0)
            {
                result.s.link_up = 1;
                result.s.full_duplex = 1;
                result.s.speed = 1000;
                return result;
            }
            /* Fall through to the generic code below */
            break;
        case CVMX_BOARD_TYPE_CUST_NB5:
            /* Port 1 on these boards is always Gigabit */
            if (ipd_port == 1)
            {
                result.s.link_up = 1;
                result.s.full_duplex = 1;
                result.s.speed = 1000;
                return result;
            }
            else /* The other port uses a broadcom PHY */
                is_broadcom_phy = 1;
            break;
        case CVMX_BOARD_TYPE_BBGW_REF:
            /* Port 1 on these boards is always Gigabit */
            if (ipd_port == 2)
            {   
                /* Port 2 is not hooked up */
                result.u64 = 0;
                return result;
            }
            else
            {
                /* Ports 0 and 1 connect to the switch */
                result.s.link_up = 1;
                result.s.full_duplex = 1;
                result.s.speed = 1000;
                return result;
            }
            break;
    }

    phy_addr = cvmx_helper_board_get_mii_address(ipd_port);
    if (phy_addr != -1)
    {
        if (is_broadcom_phy)
        {
            /* Below we are going to read SMI/MDIO register 0x19 which works
                on Broadcom parts */
            int phy_status = cvmx_mdio_read(phy_addr >> 8, phy_addr & 0xff, 0x19);
            switch ((phy_status>>8) & 0x7)
            {
                case 0:
                    result.u64 = 0;
                    break;
                case 1:
                    result.s.link_up = 1;
                    result.s.full_duplex = 0;
                    result.s.speed = 10;
                    break;
                case 2:
                    result.s.link_up = 1;
                    result.s.full_duplex = 1;
                    result.s.speed = 10;
                    break;
                case 3:
                    result.s.link_up = 1;
                    result.s.full_duplex = 0;
                    result.s.speed = 100;
                    break;
                case 4:
                    result.s.link_up = 1;
                    result.s.full_duplex = 1;
                    result.s.speed = 100;
                    break;
                case 5:
                    result.s.link_up = 1;
                    result.s.full_duplex = 1;
                    result.s.speed = 100;
                    break;
                case 6:
                    result.s.link_up = 1;
                    result.s.full_duplex = 0;
                    result.s.speed = 1000;
                    break;
                case 7:
                    result.s.link_up = 1;
                    result.s.full_duplex = 1;
                    result.s.speed = 1000;
                    break;
            }
        }
        else
        {
            /* This code assumes we are using a Marvell Gigabit PHY. All the
                speed information can be read from register 17 in one go. Somebody
                using a different PHY will need to handle it above in the board
                specific area */
            int phy_status = cvmx_mdio_read(phy_addr >> 8, phy_addr & 0xff, 17);

            /* Only return a link if the PHY has finished auto negotiation
                and set the resolved bit (bit 11) */
            if (phy_status & (1<<11))
            {
                result.s.link_up = 1;
                result.s.full_duplex = ((phy_status>>13)&1);
                switch ((phy_status>>14)&3)
                {
                    case 0: /* 10 Mbps */
                        result.s.speed = 10;
                        break;
                    case 1: /* 100 Mbps */
                        result.s.speed = 100;
                        break;
                    case 2: /* 1 Gbps */
                        result.s.speed = 1000;
                        break;
                    case 3: /* Illegal */
                        result.u64 = 0;
                        break;
                }
            }
        }
    }
    else if (OCTEON_IS_MODEL(OCTEON_CN3XXX) || OCTEON_IS_MODEL(OCTEON_CN58XX) || OCTEON_IS_MODEL(OCTEON_CN50XX))
    {
        /* We don't have a PHY address, so attempt to use inband status. It is
            really important that boards not supporting inband status never get
            here. Reading broken inband status tends to do bad things */
        cvmx_gmxx_rxx_rx_inbnd_t inband_status;
        int interface = cvmx_helper_get_interface_num(ipd_port);
        int index = cvmx_helper_get_interface_index_num(ipd_port);
        inband_status.u64 = cvmx_read_csr(CVMX_GMXX_RXX_RX_INBND(index, interface));

        result.s.link_up = inband_status.s.status;
        result.s.full_duplex = inband_status.s.duplex;
        switch (inband_status.s.speed)
        {
            case 0: /* 10 Mbps */
                result.s.speed = 10;
                break;
            case 1: /* 100 Mbps */
                result.s.speed = 100;
                break;
            case 2: /* 1 Gbps */
                result.s.speed = 1000;
                break;
            case 3: /* Illegal */
                result.u64 = 0;
                break;
        }
    }
    else
    {
        /* We don't have a PHY address and we don't have inband status. There
            is no way to determine the link speed. Return down assuming this
            port isn't wired */
#ifdef CONFIG_JSRXNLE
        DECLARE_GLOBAL_DATA_PTR;
        switch (gd->board_desc.board_type)
        {
            CASE_ALL_SRX240_MODELS
                result.s.link_up = 1;
                result.s.speed = 1000;
                result.s.full_duplex = 1;
                return result;
        }
#endif
#ifdef CONFIG_MAG8600
        DECLARE_GLOBAL_DATA_PTR;
        /* This code is only valid for MSC boards */
        if ( gd->board_desc.board_type == MAG8600_MSC) 
        {
            /* Marvel PHY accessed thru IC2 via Master CPLD */
            /* All the speed information can be read from Register 17 in one go.*/
            /* We pass here: bus, dev, address, clk. See the cmd_mem.c file */
            uint16_t phy_status;
            uint8_t op_status;
            op_status = msc_i2c_read_16(FSC_BUS, MSC_ETH_PHY, 17, &phy_status, 0);
            /* If the resolve bit 11 isn't set, see if autoneg is turned off
               (bit 12, reg 0). The resolve bit doesn't get set properly when
               autoneg is off, so force it */
            if ((phy_status & (1<<11)) == 0)
            {
                /* Get Register 0 */
                uint16_t auto_status;
                op_status = msc_i2c_read_16(FSC_BUS, MSC_ETH_PHY, 0, &auto_status, 0);
                cvmx_dprintf("auto_status: %x\n", auto_status);
                if ((auto_status & (1<<12)) == 0)
                    phy_status |= 1<<11;
            }

            /* Only return a link if the PHY has finished auto negotiation
               and set the resolved bit (bit 11) */
            if (phy_status & (1<<11))
            {
                result.s.link_up = 1;
                result.s.full_duplex = ((phy_status>>13)&1);
                switch ((phy_status>>14)&3)
                {
                case 0: /* 10 Mbps */
                    result.s.speed = 10;
                    break;
                case 1: /* 100 Mbps */
                    result.s.speed = 100;
                    break;
                case 2: /* 1 Gbps */
                    result.s.speed = 1000;
                    break;
                case 3: /* Illegal */
                    result.u64 = 0;
                    break;
                }
            }
        }
        else 
        {
            /* We don't have a PHY address and we don't have in-band status. There
               is no way to determine the link speed. Return down assuming this
               port isn't wired */
            result.u64 = 0;
        }
#else
	/* We don't have a PHY address and we don't have in-band status. There
           is no way to determine the link speed. Return down assuming this
           port isn't wired */
           result.u64 = 0; 
#endif /* CONFIG_MAG8600 */

#if defined(CONFIG_ACX500_SVCS)
           result.s.link_up = 1;
           result.s.speed = 1000;
           result.s.full_duplex = 1;
           return result;
#endif
    }

    /* If link is down, return all fields as zero. */
    if (!result.s.link_up)
        result.u64 = 0;

    return result;
}


/**
 * @INTERNAL
 * This function is called by cvmx_helper_interface_probe() after it
 * determines the number of ports Octeon can support on a specific
 * interface. This function is the per board location to override
 * this value. It is called with the number of ports Octeon might
 * support and should return the number of actual ports on the
 * board.
 *
 * This function must be modifed for every new Octeon board.
 * Internally it uses switch statements based on the cvmx_sysinfo
 * data to determine board types and revisions. It relys on the
 * fact that every Octeon board receives a unique board type
 * enumeration from the bootloader.
 *
 * @param interface Interface to probe
 * @param supported_ports
 *                  Number of ports Octeon supports.
 *
 * @return Number of ports the actual board supports. Many times this will
 *         simple be "support_ports".
 */
int __cvmx_helper_board_interface_probe(int interface, int supported_ports)
{
#ifdef CONFIG_JSRXNLE
    DECLARE_GLOBAL_DATA_PTR;
    switch (gd->board_desc.board_type)
    {
        CASE_ALL_SRX240_MODELS
            return 2;
        default:
            break;
    }
    switch (cvmx_sysinfo_get()->board_type)
    {
        case CVMX_BOARD_TYPE_CN3005_EVB_HS5:
            if (interface == 0)
                return 2;
        case CVMX_BOARD_TYPE_BBGW_REF:
            if (interface == 0)
                return 2;
    }
#endif
#ifdef CONFIG_MAG8600
    if ( interface == 1)
       return 1;

#endif
    return supported_ports;
}


/**
 * @INTERNAL
 * Enable packet input/output from the hardware. This function is
 * called after by cvmx_helper_packet_hardware_enable() to
 * perform board specific initialization. For most boards
 * nothing is needed.
 *
 * @param interface Interface to enable
 *
 * @return Zero on success, negative on failure
 */
int __cvmx_helper_board_hardware_enable(int interface)
{
    if (cvmx_sysinfo_get()->board_type == CVMX_BOARD_TYPE_TRANTOR)
    {
        if (interface < 2)
        {
            int index;
            for (index = 0; index<4; index++)
            {
                cvmx_write_csr(CVMX_ASXX_TX_CLK_SETX(index, interface), 0);
                cvmx_write_csr(CVMX_ASXX_RX_CLK_SETX(index, interface), 16);
            }
        }
    }
    else if (cvmx_sysinfo_get()->board_type == CVMX_BOARD_TYPE_CN3005_EVB_HS5)
    {
        if (interface == 0)
        {
            /* Different config for switch port */
            cvmx_write_csr(CVMX_ASXX_TX_CLK_SETX(1, interface), 0);
            cvmx_write_csr(CVMX_ASXX_RX_CLK_SETX(1, interface), 0);
            /* Boards with gigabit WAN ports need a different setting that is
                compatible with 100 Mbit settings */
            cvmx_write_csr(CVMX_ASXX_TX_CLK_SETX(0, interface), 0xc);
            cvmx_write_csr(CVMX_ASXX_RX_CLK_SETX(0, interface), 0xc);
        }
    }
    else if (cvmx_sysinfo_get()->board_type == CVMX_BOARD_TYPE_CN3010_EVB_HS5)
    {
        /* Broadcom PHYs require differnet ASX clocks. Unfortunately
            many customer don't define a new board Id and simply
            mangle the CN3010_EVB_HS5 */
        if (interface == 0)
        {
            /* Some customers boards use a hacked up bootloader that identifies them as
            ** CN3010_EVB_HS5 evaluation boards.  This leads to all kinds of configuration
            ** problems.  Detect one case, and print warning, while trying to do the right thing.
            */
            int phy_addr = cvmx_helper_board_get_mii_address(0);
            if (phy_addr != -1)
            {
                int phy_identifier = cvmx_mdio_read(phy_addr >> 8, phy_addr & 0xff, 0x2);
                /* Is it a Broadcom PHY? */
                if (phy_identifier == 0x0143)
                {
                    cvmx_dprintf("\n");
                    cvmx_dprintf("ERROR:\n");
                    cvmx_dprintf("ERROR: Board type is CVMX_BOARD_TYPE_CN3010_EVB_HS5, but Broadcom PHY found.\n");
                    cvmx_dprintf("ERROR: The board type is mis-configured, and software malfunctions are likely.\n");
                    cvmx_dprintf("ERROR: All boards require a unique board type to identify them.\n");
                    cvmx_dprintf("ERROR:\n");
                    cvmx_dprintf("\n");
                    cvmx_wait(1000000000);
                    cvmx_write_csr(CVMX_ASXX_RX_CLK_SETX(0, interface), 5);
                    cvmx_write_csr(CVMX_ASXX_TX_CLK_SETX(0, interface), 5);
                }
            }
        }
    }
    return 0;
}

