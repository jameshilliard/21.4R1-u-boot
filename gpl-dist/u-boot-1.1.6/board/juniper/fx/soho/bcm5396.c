/*
 * Copyright (c) 2010-2011, Juniper Networks, Inc.
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

/* Chun Ng (cng@juniper.net) */

#include "rmi/rmigmac.h"
#include "rmi/gmac.h"
#include "rmi/on_chip.h"
#include "rmi/xlr_cpu.h"
#include "rmi/shared_structs.h"
#include "rmi/cpu_ipc.h"
#include "bcm5396.h"
#include "soho_cpld.h"
#include "soho_misc.h"
#include <command.h>

static BOOLEAN use_cpld_mdio = TRUE;
uint8_t bcm5396_dbg_flag = 1;
extern gmac_eth_info_t *eth_info_array[];
uint8_t soho_5396_page_map(uint32_t gmac_id);

#define MII_BUS_0                   0
#define MII_BUS_1                   1
#define SEG_0                       0
#define SEG_1                       1

#define SOHO_MDIO_22_WRITE(eth_info, reg_id, data)                          \
    do {                                                                    \
        if (use_cpld_mdio == TRUE) {                                        \
            soho_cpld_mdio_c22_write(eth_info->phy_id, reg_id,              \
                                     data & 0xFFFF,                         \
                                     SOHO_CPLD_MDIO_22_TIME_PARA);          \
        } else {                                                            \
            gmac_phy_write(eth_info, reg_id, (data & 0xFFFF));              \
        }                                                                   \
    } while (0);

#define SOHO_MDIO_22_READ(eth_info, reg_id, data)                           \
    do {                                                                    \
        if (use_cpld_mdio == TRUE) {                                        \
            soho_cpld_mdio_c22_read(eth_info->phy_id, reg_id,             \
                                    data, SOHO_CPLD_MDIO_22_TIME_PARA);     \
        } else {                                                            \
            gmac_phy_read(eth_info, BCM5396_DATA_0_REG, data);              \
        }                                                                   \
    } while (0);

static bcm5396_status_t
bcm5396_poll_busy (gmac_eth_info_t *eth_info)
{
    uint32_t loop_cnt = 0;
    uint32_t reg_val;

    while (loop_cnt++ < BCM5396_MAX_RETRY) {
        SOHO_MDIO_22_READ(eth_info, BCM5396_ACC_CTRL_REG, &reg_val); 

        if ((reg_val & BCM5396_OP_MASK) == 0) {
            return BCM5396_OK;
        }
    }

    return BCM5396_TIMEOUT;
}

bcm5396_status_t
bcm5396_write (gmac_eth_info_t *eth_info, uint32_t reg_id, uint16_t data)
{
    bcm5396_status_t status;
    uint32_t page_no;
    uint32_t port_no;

    if (eth_info == NULL) {
        return BCM5396_NULL;
    }
    port_no = eth_info->port_id;
    page_no = soho_5396_page_map(eth_info->port_id);

    soho_mdio_set_bcm5396();

    SOHO_MDIO_22_WRITE(eth_info, BCM5396_PAGE_REG, (page_no << 8) | 0x1);

    SOHO_MDIO_22_WRITE(eth_info, BCM5396_DATA_0_REG, data);

    SOHO_MDIO_22_WRITE(eth_info, BCM5396_DATA_1_REG, 0);

    SOHO_MDIO_22_WRITE(eth_info, BCM5396_DATA_2_REG, 0);

    SOHO_MDIO_22_WRITE(eth_info, BCM5396_DATA_3_REG, 0);

    SOHO_MDIO_22_WRITE(eth_info, BCM5396_ACC_CTRL_REG, ((reg_id & 0xFF) << 8));

    SOHO_MDIO_22_WRITE(eth_info, BCM5396_ACC_CTRL_REG, 
                       (((reg_id & 0xFF) << 8) | BCM5396_WRITE_OP));
    
    if ((status = bcm5396_poll_busy(eth_info)) != BCM5396_OK) {
        if (bcm5396_dbg_flag) {
            printf("%s: Write OP for Reg 0x%x failed\n", __func__, reg_id);
        }
        return (status);
    }
    return BCM5396_OK;
}

bcm5396_status_t
bcm5396_read (gmac_eth_info_t *eth_info, uint32_t reg_id, uint32_t *data)
{
    uint8_t page_no;
    bcm5396_status_t status;

    if (eth_info == NULL || data == NULL) {
        return BCM5396_NULL;
    }

    page_no = soho_5396_page_map(eth_info->port_id);
    *data = 0;

    if ((status = bcm5396_poll_busy(eth_info)) != BCM5396_OK) {
        printf("%s: Previous OP not cleared\n", __func__);
    }

    SOHO_MDIO_22_WRITE(eth_info, BCM5396_PAGE_REG, (((page_no & 0xFF) << 8) | 0x1));

    SOHO_MDIO_22_WRITE(eth_info, BCM5396_ACC_CTRL_REG, ((reg_id & 0xFF) << 8));

    SOHO_MDIO_22_WRITE(eth_info, BCM5396_ACC_CTRL_REG, 
                                    ((reg_id & 0xFF) << 8 | BCM5396_READ_OP));

    if ((status = bcm5396_poll_busy(eth_info)) != BCM5396_OK) {
        if (bcm5396_dbg_flag) {
            printf("%s: Read OP for Reg 0x%x failed\n", __func__, reg_id);
        }
        return status;
    }

    SOHO_MDIO_22_READ(eth_info, BCM5396_DATA_0_REG, data);

    return BCM5396_OK;
}

uint32_t
bcm5396_lpb_ctrl (gmac_eth_info_t *eth_info, uint32_t lpb_en)
{
    uint32_t reg_val;
    uint32_t port_no = eth_info->port_id;
    bcm5396_status_t status;

    if (port_no < 0 || port_no > BCM5396_MAX_PORT) {
        return BCM5396_INV_PORT;
    }

    status = bcm5396_read(eth_info, BCM5396_MII_CTRL_REG, &reg_val);
    if (status != BCM5396_OK) {
        return status;
    }

    if (bcm5396_dbg_flag) {
        printf("%s[%d]: MII CTRL Reg = 0x%x\n", __func__, __LINE__, reg_val);
    }

    if (lpb_en) {
        reg_val |= 1 << 14;
    } else {
        reg_val &= ~(1 << 14);
    }

    status = bcm5396_write(eth_info, BCM5396_MII_CTRL_REG, reg_val);
    if (status != BCM5396_OK) {
        return status;
    }

    if (bcm5396_dbg_flag) {
        bcm5396_read(eth_info, BCM5396_MII_CTRL_REG, &reg_val);
        printf("%s[%d]: MII CTRL Reg = 0x%x\n", __func__, __LINE__, reg_val);
    }

    status = bcm5396_read(eth_info, BCM5396_SERDE_SGMII_REG, &reg_val);
    if (status != BCM5396_OK) {
        return status;
    }

    if (lpb_en) {
        reg_val |= 1 << 10;
    } else {
        reg_val &= ~(1 << 10);
    }

    status = bcm5396_write(eth_info, BCM5396_SERDE_SGMII_REG, reg_val);
    if (status != BCM5396_OK) {
        return status;
    }

    return BCM5396_OK;
}

void 
bcm5396_autoneg (uint32_t gmac_id)
{
    uint32_t data;
    gmac_eth_info_t *eth_info;

    eth_info = eth_info_array[gmac_id];

    SOHO_MDIO_22_READ(eth_info, 0x0, &data);
    data |= (1 << 12 | 1 << 9);
    SOHO_MDIO_22_WRITE(eth_info, 0, data);
}

BOOLEAN 
bcm5396_sgmii_linkup (uint32_t gmac_id) 
{
    uint32_t data;
    gmac_eth_info_t *eth_info;

    eth_info = eth_info_array[gmac_id];

    SOHO_MDIO_22_READ(eth_info, 0x28, &data);
    if (data & 0x2) {
        return TRUE;
    } else {
        return FALSE;
    }
}

static void
fx_read_bcm5396_reg (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    gmac_eth_info_t *eth_info;
    uint32_t gmac_port, reg_id;
    uint16_t data;

    if (argc < 3) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    if (argc == 3) {
        gmac_port = simple_strtoul(argv[1], NULL, 16);
        reg_id = simple_strtoul(argv[2], NULL, 16);
    }

    soho_mdio_set_bcm5396();

    eth_info = eth_info_array[gmac_port];

    bcm5396_read(eth_info, reg_id, &data);

    printf("%s: Reg id: 0x%x value: 0x%x\n", __func__, reg_id, 
           (data & 0xFFFFFFFF));
}

static void
fx_write_bcm5396_reg (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    gmac_eth_info_t *eth_info;
    uint32_t data, gmac_port, reg_id;

    if (argc < 3) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    if (argc == 4) {
        gmac_port = simple_strtoul(argv[1], NULL, 16);
        reg_id = simple_strtoul(argv[2], NULL, 16);
        data = simple_strtoul(argv[3], NULL, 16);
    }

    if (gmac_port > 7) {
        printf("gmac_port %d is invalid\n", gmac_port);
        return;
    }

    soho_mdio_set_bcm5396();

    eth_info = eth_info_array[gmac_port];
    bcm5396_write(eth_info, reg_id, data & 0xFFFF);
    printf("write value 0x%x to reg_id 0x%x\n", data & 0xFFFF, reg_id);
}

static void 
bcm5396_cpld_page_read (uint8_t page_no, uint32_t reg_id, uint32_t* data)
{
    uint32_t loop_cnt = 0;
    uint32_t reg_val;
    
    soho_cpld_mdio_c22_write(BCM5396_MII_ADDR, BCM5396_PAGE_REG, 
                             (((page_no & 0xFF) << 8) | 0x1), 
                             SOHO_CPLD_MDIO_22_TIME_PARA);

    soho_cpld_mdio_c22_write(BCM5396_MII_ADDR, BCM5396_ACC_CTRL_REG, 
                             ((reg_id & 0xFF) << 8), SOHO_CPLD_MDIO_22_TIME_PARA);

    soho_cpld_mdio_c22_write(BCM5396_MII_ADDR, BCM5396_ACC_CTRL_REG, 
                                    ((reg_id & 0xFF) << 8 | BCM5396_READ_OP),
                                    SOHO_CPLD_MDIO_22_TIME_PARA);
    
    while (loop_cnt++ < BCM5396_MAX_RETRY) {
        soho_cpld_mdio_c22_read(BCM5396_MII_ADDR, BCM5396_ACC_CTRL_REG, 
                                &reg_val, SOHO_CPLD_MDIO_22_TIME_PARA);
        if ((reg_val & BCM5396_OP_MASK) == 0) {
            break;
        }
    }

    if (loop_cnt >= BCM5396_MAX_RETRY) {
        printf("%s: BCM5396 busy flag not cleared\n", __func__);
        return;
    }

    soho_cpld_mdio_c22_read(BCM5396_MII_ADDR, BCM5396_DATA_0_REG, data, 
                            SOHO_CPLD_MDIO_22_TIME_PARA);
}

static bcm5396_status_t 
bcm5396_cpld_page_write (uint8_t page_no, uint32_t reg_id, uint16_t data)
{
    uint32_t loop_cnt = 0;
    uint32_t reg_val;

    soho_cpld_mdio_c22_write(BCM5396_MII_ADDR, BCM5396_PAGE_REG, page_no, 
                             SOHO_CPLD_MDIO_22_TIME_PARA);

    soho_cpld_mdio_c22_write(BCM5396_MII_ADDR, BCM5396_DATA_0_REG, 
                             data & 0xFF, SOHO_CPLD_MDIO_22_TIME_PARA);

    soho_cpld_mdio_c22_write(BCM5396_MII_ADDR, BCM5396_ACC_CTRL_REG, 
                             ((reg_id & 0xFF) << 8), 
                             SOHO_CPLD_MDIO_22_TIME_PARA);

    soho_cpld_mdio_c22_write(BCM5396_MII_ADDR, BCM5396_ACC_CTRL_REG, 
                            (((reg_id & 0xFF) << 8) | BCM5396_WRITE_OP), 
                            SOHO_CPLD_MDIO_22_TIME_PARA);
    
    while (loop_cnt++ < BCM5396_MAX_RETRY) {
        soho_cpld_mdio_c22_read(BCM5396_MII_ADDR, BCM5396_ACC_CTRL_REG, 
                                &reg_val, SOHO_CPLD_MDIO_22_TIME_PARA);
        if ((reg_val & BCM5396_OP_MASK) == 0) {
            return BCM5396_OK;
        }
    }
    return BCM5396_TIMEOUT;
}

U_BOOT_CMD(
	fx_read_bcm5396_reg,  
    3,
    1,
    fx_read_bcm5396_reg,
	"fx_read_bcm5396_reg - read a bcm5396 register\n",
	"read a phy register <port> <phy reg id>\n"
);

U_BOOT_CMD(
	fx_write_bcm5396_reg,  
    4,
    1,
    fx_write_bcm5396_reg,
	"fx_write_bcm5396_reg - write a bcm5396 register\n",
	"write a bcm5396 register <port> <phy reg id> <value>\n"
);

uint8_t
soho_5396_page_map (uint32_t gmac_id)
{
    switch (gmac_id) {
    case 0:
        return 0x16;

    case 1:
        return 0x18;

    case 2:
        return 0x19;

    case 3:
        return 0x17;

    case 5:
        return 0x1A;

    case 6:
        return 0x1B;

    default:
        printf("ERROR: wrong gmac: %d\n", gmac_id);
        return 0;
    }
}

void
bcm5396_reset (uint32_t gmac_port)
{
    gmac_eth_info_t *eth_info;
    eth_info = eth_info_array[gmac_port];
    uint32_t page=soho_5396_page_map(gmac_port);
    soho_mdio_set_bcm5396();
    bcm5396_write(eth_info, 0x0, 0x8000);
    udelay(1000);
}

static void
bcm5396_page_write_reg (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uint32_t data, page_no, reg_id;

    if (argc < 3) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    if (argc == 4) {
        page_no = simple_strtoul(argv[1], NULL, 16);
        reg_id = simple_strtoul(argv[2], NULL, 16);
        data = simple_strtoul(argv[3], NULL, 16);
    }

    printf("write value 0x%x to page_no 0x%x reg_id 0x%x\n", 
           data & 0xFFFF, page_no, reg_id);
    bcm5396_cpld_page_write(page_no, reg_id, data & 0xFFFF);
}

U_BOOT_CMD(
	bcm5396_page_write_reg,  
    4,
    1,
    bcm5396_page_write_reg,
	"bcm5396_page_write_reg - write a bcm5396 register\n",
	"write a bcm5396 register <page> <phy reg id> <value>\n"
);

void 
bcm5396_page_read_reg (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uint32_t page_no, reg_id;
    uint32_t data = 0;

    if (argc < 3) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    if (argc == 3) {
        page_no = simple_strtoul(argv[1], NULL, 16);
        reg_id = simple_strtoul(argv[2], NULL, 16);
    }

    bcm5396_cpld_page_read(page_no, reg_id, &data);

    printf("%s: page_no : 0x%x Reg id: 0x%x value: 0x%x\n", __func__, page_no, reg_id, 
           (data & 0xFFFFFFFF));
}

U_BOOT_CMD(
	bcm5396_page_read_reg,  
    3,
    1,
    bcm5396_page_read_reg,
	"bcm5396_page_read_reg - read a bcm5396 register\n",
	"read a phy register <page> <phy reg id>\n"
);
