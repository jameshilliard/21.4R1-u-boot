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


#include <common.h>
#include <command.h>

#ifdef CONFIG_MAG8600
#include "msc_i2c.h"
#include "msc_fsc_phy.h"


#define FSC_PHY_DEBUG
#define FSC_PHY_TRACE
#undef FSC_PHY_TRACE
#undef FSC_PHY_DEBUG

#ifdef FSC_PHY_DEBUG
#define dprintf printf
#ifdef FSC_PHY_TRACE
#define dtrace dprintf
#else
#define dtrace(...)
#endif
#else
#define dprintf(...)
#define dtrace(...)
#endif
#if (CONFIG_COMMANDS & CFG_CMD_MII)
#include <miiphy.h>
#endif
#define CONFIG_PHY_GIGE

#if defined(CONFIG_MII) || (CONFIG_COMMANDS & CFG_CMD_MII)
/*****************************************************************************
 *
 * Read the OUI, manufacture's model number, and revision number.
 * bus:  MSC I2C Bus to access PHY 
 * dev:  MSC I2C device address
 * OUI:     22 bits (unsigned int)
 * Model:    6 bits (unsigned char)
 * Revision: 4 bits (unsigned char)
 *
 * Returns:
 *   0 on success
 */
int  
fsc_phy_info (unsigned char bus,unsigned char dev, 
              unsigned int  *oui,unsigned char *model, 
              unsigned char *rev)
{

    unsigned int   reg = 0;
    unsigned short tmp;
    unsigned char  clk = msc_get_clk();

    dtrace(">> %s: \n", __FUNCTION__); 

    if (msc_i2c_read_16 (bus, dev, PHY_X_PHYIDR2, &tmp, clk ) != 0) {
#ifdef FSC_PHY_DEBUG 
        puts ("PHY ID register 2 read failed\n");
#endif
        return (-1);
    }
    reg = tmp;

#ifdef FSC_PHY_DEBUG 
    printf ("PHY_PHYIDR2 @ 0x%x,0x%x = 0x%04x\n", bus, dev , reg);
#endif
    if (reg == 0xFFFF) {
        /* No physical device present at this address */
        return (-1);
    }

    if (msc_i2c_read_16(bus, dev, PHY_X_PHYIDR1, &tmp, clk) != 0) {
#ifdef FSC_PHY_DEBUG 
        puts ("PHY ID register 1 read failed\n");
#endif
        return (-1);
    }
    reg |= tmp << 16;
#ifdef FSC_PHY_DEBUG 
    printf ("PHY_PHYIDR[1,2] @ 0x%x,0x%x = 0x%08x\n", bus, dev, reg);
#endif
    *oui   =                 ( reg >> 10);
    *model = (unsigned char) ((reg >>  4) & 0x0000003F);
    *rev   = (unsigned char) ( reg        & 0x0000000F);
    return (0);
}


/*****************************************************************************
 *
 * Reset the PHY.
 * Returns:
 *   0 on success
 */
int 
fsc_phy_reset (unsigned char bus, unsigned char dev)
{
    unsigned short reg;
    int            loop_cnt;
    unsigned char  clk = msc_get_clk();

    if (msc_i2c_write_16 (bus, dev, PHY_X_BMCR, 0x8000, clk) != 0) {
#ifdef FSC_PHY_DEBUG
        puts ("PHY reset failed\n");
#endif
        return (-1);
    }
#ifdef CONFIG_PHY_RESET_DELAY
    udelay (CONFIG_PHY_RESET_DELAY);	/* Intel LXT971A needs this */
#endif
    /*
     * Poll the control register for the reset bit to go to 0 (it is
     * auto-clearing).  This should happen within 0.5 seconds per the
     * IEEE spec.
     */
    loop_cnt = 0;
    reg = 0x8000;
    while (((reg & 0x8000) != 0) && (loop_cnt++ < 1000000)) {
        if (msc_i2c_read_16 (bus, dev, PHY_X_BMCR, &reg, clk) != 0) {
#ifdef FSC_PHY_DEBUG
            puts ("PHY status read failed\n");
#endif
            return (-1);
        }
    }
    if ((reg & 0x8000) == 0) {
        return (0);
    } else {
        puts ("PHY reset timed out\n");
        return (-1);
    }
    return (0);
}


/*****************************************************************************
 *
 * Determine the ethernet speed (10/100).
 */
int 
fsc_phy_speed (unsigned char bus, unsigned char dev)
{
    unsigned short reg;
    unsigned char  clk= msc_get_clk();

#if defined(CONFIG_PHY_GIGE)
    if (msc_i2c_read_16 (bus, dev, PHY_X_1000BTSR, &reg, clk)) {
        printf ("PHY 1000BT Status read failed\n");
    } else {
        if (reg != 0xFFFF) {
            if ((reg & (PHY_X_1000BTSR_1000FD | PHY_X_1000BTSR_1000HD)) !=0) {
                return (_1000BASET);
            }
        }
    }
#endif /* CONFIG_PHY_GIGE */

    if (msc_i2c_read_16(bus, dev, PHY_X_ANLPAR, &reg, clk)) {
        puts ("PHY speed1 read failed, assuming 10bT\n");
        return (_10BASET);
    }
    if ((reg & PHY_X_ANLPAR_100) != 0) {
        return (_100BASET);
    } else {
        return (_10BASET);
    }
}


/*****************************************************************************
 *
 * Determine full/half duplex.
 */
int 
fsc_phy_duplex (unsigned char bus, unsigned char dev)
{
    unsigned short reg;
    unsigned char  clk = msc_get_clk();

#if defined(CONFIG_PHY_GIGE)
    if (msc_i2c_read_16 (bus, dev, PHY_X_1000BTSR, &reg, clk)) {
        printf ("PHY 1000BT Status read failed\n");
    } else {
        if ( (reg != 0xFFFF) &&
             (reg & (PHY_X_1000BTSR_1000FD | PHY_X_1000BTSR_1000HD)) ) {
            if ((reg & PHY_X_1000BTSR_1000FD) !=0) {
                return (FULL);
            } else {
                return (HALF);
            }
        }
    }
#endif /* CONFIG_PHY_GIGE */

    if (msc_i2c_read_16 (bus, dev, PHY_X_ANLPAR, &reg, clk)) {
        puts ("PHY duplex read failed, assuming half duplex\n");
        return (HALF);
    }

    if ((reg & (PHY_X_ANLPAR_10FD | PHY_X_ANLPAR_TXFD)) != 0) {
        return (FULL);
    } else {
        return (HALF);
    }
}


#ifdef CFG_FAULT_ECHO_LINK_DOWN
/*****************************************************************************
 *
 * Determine link status
 */
int 
fsc_phy_link (unsigned char bus, unsigned char dev)
{
    unsigned short reg;
    unsigned char  clk = msc_get_clk();

    /* dummy read; needed to latch some phys */
    (void)msc_i2c_read_16(bus, dev, PHY_X_BMSR, &reg, clk);
    if (msc_i2c_read_16 (bus, dev, PHY_X_BMSR, &reg, clk)) {
        puts ("PHY_BMSR read failed, assuming no link\n");
        return (0);
    }

    /* Determine if a link is active */
    if ((reg & PHY_X_BMSR_LS) != 0) {
        return (1);
    } else {
        return (0);
    }
}
#endif

static uint last_op;
static uint last_bus;
static uint last_dev;
static uint last_reg;
static uint last_data;

int 
do_fsc_phy (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{

    char           op;
    unsigned char  bus;
    unsigned char  dev;
    unsigned short reg;
    unsigned short data; 
    int            rcode = 0;
    unsigned char  clk = msc_get_clk();

    dtrace(">> %s: \n", __FUNCTION__); 

    /*
     * We use the last specified parameters, unless new ones are
     * entered.
     */
    op   = last_op;
    bus  =  last_bus;
    dev  =  last_dev;
    reg  = last_reg;
    data = last_data;



    if ((flag & CMD_FLAG_REPEAT) == 0) {
        op = argv[1][0];
        if (argc >= 3) {
            bus = simple_strtoul (argv[2], NULL, 16);
        }
        if (argc >= 4) {
            dev  = simple_strtoul (argv[3], NULL, 16);
        }
        if (argc >= 5) {
            reg  = simple_strtoul (argv[4], NULL, 16);
        }
        if (argc >= 6) {
            data = simple_strtoul (argv[5], NULL, 16);
        }
    }


    if (op == 'i') {
        /* 
         *  retreive MARVEL PHY 
         *  88E1111
         *  See Doc. No. MV-S100649-00, Rev. H October 11, 2006 
         */
        unsigned int  oui;
        unsigned char model;
        unsigned char rev;

        if (fsc_phy_info (bus, dev , &oui, &model, &rev) == 0) {
            printf ("PHY @0x%02X,0x%02X: "
                    "OUI = 0x%04X, "
                    "Model = 0x%02X, "
                    "Rev = 0x%02X, "
                    "%3dbaseT, %s\n",
                    bus,dev, oui, model, rev,
                    fsc_phy_speed (bus, dev),
                    fsc_phy_duplex (bus, dev) == FULL ? "FDX" : "HDX");
        }

    } else if (op == 'r') {	
        data = 0xffff;
        if (msc_i2c_read_16(bus, dev, reg, &data, clk) != 0) {
            printf("Error reading from the PHY bus=%02x device=%02x reg=%02x\n",
                   bus, dev, reg);
            rcode = 1;
        } else {
            printf("bus=%02x, device=%02x reg=%02x data=%04X\n",
                   bus, dev, reg, data & 0x0000FFFF);
        }

    } else if (op == 'w') {
        if (msc_i2c_write_16 (bus, dev, reg, data, clk) != 0) {
            printf("Error writing to the PHY bus=%02x device=%02x reg=%02x\n",
                   bus, dev, reg);
            rcode = 1;
        }	

    } else if (op == 'd') {

        ushort reg_data[6];
        int    ok = 1;
        int    ireg;

        reg = 0; /* we take all 6 of them each time for dumping */

        for (ireg = 0; ireg < 6; ireg++) {
            if (msc_i2c_read_16(bus, dev, reg, &reg_data[ireg], clk) != 0) {
                ok = 0;
                printf("Error reading from the PHY bus=%02x addr=%02x reg=%02x\n",
                       bus, dev, reg);
                rcode = 1;
            }
            printf("reg[%d]=0x%X\n", ireg, reg_data[ireg]);
            reg++;
        }
        if (ok) {
            MII_dump_0_to_5(reg_data, 0, 5);
        }
        printf("\n");

    } else {
        printf("Usage:\n%s\n", cmdtp->usage);
        return 1;
    }

    /*
     * Save the parameters for repeats.
     */
    last_op   = op;
    last_bus  = bus;
    last_dev  = dev;
    last_reg  = reg;
    last_data = data;

    return rcode;
}

U_BOOT_CMD(
           fsc_phy,	6,	1,	do_fsc_phy,
           "fsc_phy                                   - MII  MSC utility commands\n",
           "info  <bus> <dev_addr>                    - display FSC PHY info\n"
           "fsc_phy read  <bus> <dev_addr> <reg_addr> - read  FSC PHY register \n"
           "fsc_phy write <bus> <dev_addr> <reg_addr> <value> - write FSC PHY register\n"
           "fsc_phy dump  <bus> <dev_addr>            - dump FSC PHY registers 0-5\n"
          );

#endif /* CONFIG_MII || (CONFIG_COMMANDS & CFG_CMD_MII) */

#endif /* CONFIG_OCTEON_MSC */
