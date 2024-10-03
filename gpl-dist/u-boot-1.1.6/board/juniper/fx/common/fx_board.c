/*
 * $Id$
 *
 * fx_board.c
 *
 * Copyright (c) 2012-2013, Juniper Networks, Inc.
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

#include <configs/fx.h>
#include <rmi/pioctrl.h>
#include <common/eeprom.h>
#include <soho/tor_mgmt_board.h>
#include <soho/soho_cpld.h>
#include <soho/soho_cpld_map.h>
#include "fx_common.h"
#include "rmi/on_chip.h"
#include "rmi/xlr_cpu.h"

#define READ_FPGA_MAX_RETRY 20

u_int8_t  g_log_eprom_bus;
u_int16_t g_board_type = 0;
uint16_t g_mgmt_board_type = 0;
uint8_t g_board_ver;
uint16_t fx_get_mgmt_board_type(void);

fx_get_board_type (void)
{
    u_int16_t assm_id = 0;
    u_int8_t  get_buffer[2];

    if (i2c_read(FX_EEPROM_BUS, FX_BOARD_EEPROM_ADDR, FX_BOARD_ID_OFFSET, 
                get_buffer, 2) != 0) {
        printf("\nERROR: Could not read the Board ID EEPROM!!!!\n");
/* 
 * The EEPROM and the i2c driver are not very stable. 
 * Need to bootup as CB if the board ID cannot be read for
 * any reason. This should be removed when both 
 * the components become stable enough.
 */ 
#ifndef DELETE_LATER	    
    if (1) {
        printf("\nForcing it to be a CONTROL BOARD!!!!\n");
        assm_id = 0x09a1;
    }
#endif	    
    } else {
        assm_id = (get_buffer[0] & 0xff) << 8 | (get_buffer[1] & 0xff);
    }
#ifndef DELETE_LATER
    if (assm_id == 0xffff)
    {   
        assm_id = 0x09a5;
        printf("\nGot the Board ID as 0xffff... so forcing it to be 0x%x!!\n",
               assm_id);
    }	
#endif    
    g_board_type = assm_id;
    
    if (i2c_read(FX_EEPROM_BUS, FX_BOARD_EEPROM_ADDR, FX_BOARD_VERSION_OFFSET,
                 &g_board_ver, 1) != 0) {
        debug("%s: get board version failed\n", __func__);
    }

    if (!fx_is_cb()) {
       g_log_eprom_bus = 1; /* log eprom is on bus 1 for TOR */
       debug("============= Log eeprom on bus %d ============\n",g_log_eprom_bus);
    } else {
       g_log_eprom_bus = 0; 
       debug("============= Log eeprom on bus %d ============\n",g_log_eprom_bus);
    }

    /* Get mgmt board id */
    if (fx_is_pa()) {
        g_mgmt_board_type = fx_get_mgmt_board_type();
    } 

    return assm_id;
}

/* 
 * Return Management Board ID, only available for QFX3500 board. 
 */
uint16_t 
fx_get_mgmt_board_type (void)
{
    uint16_t mgmt_id = 0xffff;
    uint8_t  get_buffer[2];
    uint32_t orig_mux = 0;

    if (!fx_is_pa()) {
        return mgmt_id;
    } else {
        /* Need to set mux to be able to read Board ID. */
        orig_mux = soho_cpld_mdio_set_mux(0x3);

        if (i2c_read(FX_EEPROM_BUS, FX_MGMT_BOARD_EEPROM_I2C_ADDR, 
                     FX_BOARD_ID_OFFSET, get_buffer, 2) != 0) {
            printf("\nERROR: Could not read the Mgmt Board ID EEPROM!!!!\n");

        } else {
            mgmt_id = (get_buffer[0] & 0xff) << 8 | (get_buffer[1] & 0xff);
        }

        /* Write back original mux value. */
        soho_cpld_mdio_set_mux(orig_mux);
    } 

    return mgmt_id;
}

/* 
 * FPGA Reset is needed for the PCI to function properly as
 * per the hardware team recommendation.
 * This is the requirement only on the Control Board for 
 * now but this might be needed on other boards too.
 *
 * The FPGA is behind an IO Expander which is on i2c bus 1. 
 * The first bit of offset 0x1, of the IO expander should be 
 * set to 0 to assert and the first bit of offset 0x3 should be 
 * toggled to deassert.
 */
void fx_reset_fpga (void)
{
    uint8_t buf = 0;
    uint32_t retry = 0;
    uint32_t *mmio = (uint32_t *)(0xbef00000 + PHOENIX_IO_GPIO_OFFSET);

    switch(g_board_type) {

    case QFX_CONTROL_BOARD_ID:
        do {
            if (i2c_read(FX_IO_EXPANDER_BUS, FX_IO_EXPANDER_ADDR,
                        FX_IO_EXPANDER_FPGA_ASSERT_RST_OFFSET,
                        &buf, 1) == 0) {
                break;
            }
            udelay(phoenix_read_reg(mmio, 0x2b) % 100);
        } while (++retry < READ_FPGA_MAX_RETRY);

        if (retry) {
            if (retry == READ_FPGA_MAX_RETRY) {
                printf("\nERROR: Could not read FPGA Assert offset!!\n");
            } else {
                printf("Read FPGA Asser offset success after %d retry\n", retry);
            }
        }

        buf &= 0x7F;

        if (i2c_write(FX_IO_EXPANDER_BUS, FX_IO_EXPANDER_ADDR,
                     FX_IO_EXPANDER_FPGA_ASSERT_RST_OFFSET,
                     &buf, 0x1) != 0) {
            printf("\nERROR: Could not write Assert to FPGA!!\n");
        }

        if (i2c_read(FX_IO_EXPANDER_BUS, FX_IO_EXPANDER_ADDR,
                    FX_IO_EXPANDER_FPGA_DEASSERT_RST_OFFSET,
                    &buf, 1) != 0) {
            printf("\nERROR: Could not read FPGA DeAssert offset!!\n");
        }

        buf &= 0x7F;

        if (i2c_write(FX_IO_EXPANDER_BUS, FX_IO_EXPANDER_ADDR,
                     FX_IO_EXPANDER_FPGA_DEASSERT_RST_OFFSET,
                     &buf, 0x1) != 0) {
            printf("\nERROR: Could not write DeAssert to FPGA!!\n");
        }

        buf |= 0x80;

        udelay(100000);

        if (i2c_write(FX_IO_EXPANDER_BUS, FX_IO_EXPANDER_ADDR,
                     FX_IO_EXPANDER_FPGA_DEASSERT_RST_OFFSET,
                     &buf, 0x1) != 0) {
            printf("\nERROR: Could not write DeAssert toggle to FPGA!!\n");
        }
        break;

    default:
        break;
    }	
}

uint8_t fx_is_cb (void)
{
    return (g_board_type  == QFX_CONTROL_BOARD_ID);
}

uint8_t fx_is_soho (void)
{
    return (g_board_type == QFX5500_BOARD_ID);
}

uint8_t fx_is_pa (void)
{
    return (g_board_type == QFX3500_BOARD_ID
            || g_board_type == QFX3500_48S4Q_AFO_BOARD_ID);
}

uint8_t fx_is_qfx3500 (void)
{
    return (g_board_type == QFX3500_BOARD_ID);
}

uint8_t fx_is_wa (void)
{
    return (PRODUCT_IS_OF_UFABRIC_QFX3600_SERIES(g_board_type) ||
            PRODUCT_IS_OF_SE_QFX3600_SERIES(g_board_type));
}

uint8_t fx_is_tor (void) 
{
    return (fx_is_soho() || fx_is_pa() || fx_is_wa());
}

uint8_t wa_is_rev1 (void)
{
    u_int8_t  majorRev;

    if (i2c_read(FX_EEPROM_BUS, FX_BOARD_EEPROM_ADDR,
                 FX_MAJOR_REV_OFFSET, &majorRev, 0x01)
                 == 0x00) {
           if (majorRev == 0x01){
                 return 0x01;
           }
    }

    return 0x00;
}

uint8_t 
fx_get_board_ver (void)
{
    return g_board_ver;
}

uint8_t 
fx_has_reset_cpld (void)
{
    if (fx_is_soho() && fx_get_board_ver() >= 2) {
        return 1;
    }

    if (fx_is_pa() && fx_get_board_ver() >= 4) {
        return 1;
    }

    return 0;
}

uint8_t 
fx_use_i2c_cpld (uint8_t i2c_addr)
{
    return (fx_is_soho() && fx_get_board_ver() >= 2 && 
            i2c_addr != FX_BOARD_EEPROM_ADDR);
}

uint8_t 
fx_has_i2c_cpld (void)
{
    return (fx_is_soho() && fx_get_board_ver() >= 2);
}

uint8_t
fx_has_rj45_mgmt_board (void)
{
    return (g_mgmt_board_type == QFX3500_RJ45_MGMT_BOARD_ID);
}

uint8_t
fx_is_qfx3500_sfp_mgmt_board (void)
{
    return ((g_mgmt_board_type == QFX3500_SFP_MGMT_BOARD_ID) ||
            (g_mgmt_board_type == QFX3500_SFP_PTF_MGMT_BOARD_ID));
}

uint8_t 
fx_is_sfp_port (uint32_t gmac_id) 
{
    return ((fx_is_pa() && fx_is_qfx3500_sfp_mgmt_board()) &&
            GMAC_PORT_IS_TOR_MGMT_BRD_SFP_PORT(gmac_id));
}

uint8_t 
wa_is_rev3_or_above (void)
{
    if (fx_get_board_ver() >= 0x03) {
        return 0x01;
    }
    return 0x00;
}

