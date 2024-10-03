/*
 * $Id$
 *
 * tor_mgmt_board.c
 *
 * TOR Mgmt Board related commands at the uboot level.
 *
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

#include <command.h>
#include <common/bcm54xxx.h>
#include <common/fx_common.h>
#include <rmi/rmigmac.h>
#include "soho_cpld.h"
#include "soho_cpld_map.h"
#include "tor_mgmt_board.h"

extern gmac_eth_info_t *eth_info_array[8];

/* 
 * Write to the external SFP through the i2c controller.
 * 
 * It is NOT RECOMMENDED to call this function directly, but instead you should
 * call tor_mcpld_i2c_controller_write, which will call the following function
 * several times since sometime i2c transaction fails and we need to retry.
 */
static uint32_t    
tor_mcpld_i2c_controller_write_proc (const uint32_t addr, const uint16_t data,
                                     const uint16_t port, const uint32_t mempage)
{
    uint8_t res = 0;
    uint32_t wait_cnt = 0;
    uint8_t regval = 0;

    /* little endian */
    soho_cpld_write(QFX5500_SCPLD_SFP_I2C_DATA_LSB, (data >> 8) & 0xFF);

    soho_cpld_write(QFX5500_SCPLD_SFP_I2C_DATA_MSB, data & 0xFF);

    soho_cpld_write(QFX5500_SCPLD_SFP_I2C_ADDR, addr);

    /* start I2C transaction */
    regval |= (0x1 << SFP_I2C_CNTL_START_TRANS_OFF);    

    if (mempage == 0xA2) {
        regval |= (0x1 << SFP_I2C_CNTL_ADDR_0_OFF);
    } else if (mempage == 0x56) {
        regval |= (0x1 << SFP_I2C_CNTL_ADDR_1_OFF);
    }

    /* gmac port 0x6 is C1 */
    if (port == 0x6) {
        regval |= 0x1;
    } 

    soho_cpld_write(QFX5500_SCPLD_SFP_I2C_CNTL, regval);

    do {
        soho_cpld_read(QFX5500_SCPLD_SFP_I2C_CNTL, &res);
        udelay(1000*10); /* 10 ms */
        if (wait_cnt++ > 0xFF) {
            printf("CPLD TIMEOUT write control reg result = 0x%x \n", res);
            return TOR_MGMT_CPLD_ERR;
        }
    } while (!(res & (1 << SFP_I2C_CNTL_DONE_OFF)));

    /* Check for bus error or slave timeout */
    if (res & (0x1 << SFP_I2C_CNTL_SLAVETIMEOUT_OFF)) {
        printf("CPLD SLAVE TIMEOUT writing to addr 0x%x\n", addr);
        return TOR_MGMT_CPLD_I2C_SLAVETIMEOUT;
    } else if (res & (0x1 << SFP_I2C_CNTL_BUSLOSS_OFF)) {
        printf("CPLD BUS LOSS writing to addr 0x%x\n", addr);
        return TOR_MGMT_CPLD_I2C_BUSLOSS;
    }
    return TOR_MGMT_CPLD_OK;
}

/*
 * This is the function to call if you want to write to external SFP through 
 * I2C.
 * This I2C transaction should retry several times. The gut of this function
 * is actually within tor_mcpld_i2c_controller_write_proc.
 */
uint32_t
tor_mcpld_i2c_controller_write (const uint32_t addr, uint16_t data, 
                                const uint16_t port, const uint32_t mempage)
{
    uint32_t ret = 0, cnt = 0;
    const uint32_t max_try = 10;

    ret = tor_mcpld_i2c_controller_write_proc(addr, data, port, mempage);
    while (ret != TOR_MGMT_CPLD_OK) {
	if (++cnt == max_try) {
	    break;
	}
        ret = tor_mcpld_i2c_controller_write_proc(addr, data, port, mempage);
	udelay(100);
    }
    return ret;
}

/*  
 * UBOOT Command to write 
 */
void
tor_mcpld_i2c_controller_write_cmd (cmd_tbl_t *cmdtp, int flag, int argc, 
                                    char *argv[]) 
{
    uint32_t addr; 
    uint16_t data;
    uint16_t port;
    uint32_t mempage;
    uint32_t ret;

    if (argc != 5) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    port = simple_strtoul(argv[1], NULL, 16);
    addr = simple_strtoul(argv[2], NULL, 16);
    data = simple_strtoul(argv[3], NULL, 16);
    mempage = simple_strtoul(argv[4], NULL, 16);

    if (!GMAC_PORT_IS_TOR_MGMT_BRD_SFP_PORT(port)) {
        printf("ERROR - portNum %s unknown! use 0x5 or 0x6\n", port);
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    if ((mempage != 0xA0) && (mempage != 0xA2) && (mempage != 0x56)) {
        printf("ERROR - mempage %s unknown!\n", mempage);
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    ret = tor_mcpld_i2c_controller_write(addr, data, port, mempage);
    if (ret == TOR_MGMT_CPLD_OK) {
        printf("Written 0x%x: 0x%x\n", addr, data); 
    } else {
        switch (ret) {
            case TOR_MGMT_CPLD_I2C_BUSLOSS:
                printf("Write failed, buss loss.\n");
                break; 
            case TOR_MGMT_CPLD_I2C_SLAVETIMEOUT:
                printf("Write failed, slave timeout.\n");
                break; 
            case SOHO_CPLD_INVALID_REG:
                printf("Write failed, invalid register.\n");
                break; 
            case TOR_MGMT_CPLD_ERR:
            default:
                printf("Write failed, error.\n");
                break; 
        }
    }
}

/* 
 * Read one byte from the external SFP through the i2c controller.
 * 
 * It is NOT RECOMMENDED to call this function directly, but instead you should
 * call tor_mcpld_i2c_controller_read_8, which will call the following function
 * several times since sometime i2c transaction fails and we need to retry.
 */
static uint32_t
tor_mcpld_i2c_controller_read_8_proc (const uint32_t addr, uint8_t *data, 
                                      const uint16_t port, const uint32_t mempage)
{
    uint8_t res = 0;
    uint32_t wait_cnt;
    uint8_t regval = 0;

    *data = 0;

    soho_cpld_write (QFX5500_SCPLD_SFP_I2C_ADDR, addr);

    regval |= (0x1 << SFP_I2C_CNTL_START_TRANS_OFF);
    regval |= (0x1 << SFP_I2C_CNTL_RW_OFF); /* read request */

    if (mempage == 0xA2) {
        regval |= (0x1 << SFP_I2C_CNTL_ADDR_0_OFF);
    } else if (mempage == 0x56) {
        regval |= (0x1 << SFP_I2C_CNTL_ADDR_1_OFF);
    }

    /* gmac port 0x6 is port C1 */
    if (port == 0x6) {
        regval |= 0x1;
    } 

    /* issue command to initiate read */
    soho_cpld_write(QFX5500_SCPLD_SFP_I2C_CNTL, regval);
    
    wait_cnt = 0;
    do {
        soho_cpld_read(QFX5500_SCPLD_SFP_I2C_CNTL, &res);

        udelay(1000*10); /* 10 ms */
        if (wait_cnt++ > 0xFF) {
            printf("CPLD TIMEOUT read byte control reg result = 0x%x \n", res);
            return TOR_MGMT_CPLD_ERR;
        }
    } while (!(res & (1 << SFP_I2C_CNTL_DONE_OFF)));

    if (wait_cnt == 0xFF) {
        return TOR_MGMT_CPLD_ERR;
    }

    /* Check for bus error or slave timeout */
    if (res & (0x1 << SFP_I2C_CNTL_SLAVETIMEOUT_OFF)) {
        printf("CPLD SLAVE TIMEOUT reading byte from addr 0x%x\n", addr);
        return TOR_MGMT_CPLD_I2C_SLAVETIMEOUT;
    } else if (res & (0x1 << SFP_I2C_CNTL_BUSLOSS_OFF)) {
        printf("CPLD BUS LOSS reading byte from addr 0x%x\n", addr);
        return TOR_MGMT_CPLD_I2C_BUSLOSS;
    }

    soho_cpld_read(QFX5500_SCPLD_SFP_I2C_DATA_LSB, data);

    return TOR_MGMT_CPLD_OK;
}

/*
 * This is the function to call if you want to read one byte from external SFP
 * through I2C.
 * This I2C transaction should retry several times. The gut of this function
 * is actually within tor_mcpld_i2c_controller_read_8_proc.
 */
uint32_t
tor_mcpld_i2c_controller_read_8 (const uint32_t addr, uint8_t *data, 
                                 const uint16_t port, const uint32_t mempage)
{
    uint32_t ret = 0, cnt = 0;
    const uint32_t max_try = 10;

    ret = tor_mcpld_i2c_controller_read_8_proc(addr, data, port, mempage);
    while (ret != TOR_MGMT_CPLD_OK) {
	if (++cnt == max_try) {
	    break;
	}
        ret = tor_mcpld_i2c_controller_read_8_proc(addr, data, port, mempage);
	udelay(100);
    }
    return ret;
}

/* 
 * Read 2 bytes from the external SFP through the i2c controller.
 *
 * It is NOT RECOMMENDED to call this function directly, but instead you should
 * call tor_mcpld_i2c_controller_read, which will call the following function
 * several times since sometimes i2c transaction fails and we need to retry.
 */
static uint32_t
tor_mcpld_i2c_controller_read_proc (const uint32_t addr, uint16_t *data, 
                                    const uint16_t port, const uint32_t mempage)
{
    uint8_t res = 0;
    uint32_t wait_cnt = 0;
    uint8_t temp_data = 0;
    uint8_t regval = 0;
    *data = 0;

    soho_cpld_write(QFX5500_SCPLD_SFP_I2C_ADDR, addr);

    regval |= (0x1 << SFP_I2C_CNTL_START_TRANS_OFF);
    regval |= (0x1 << SFP_I2C_CNTL_RW_OFF); /* read request */

    if (mempage == 0xA2) {
        regval |= (0x1 << SFP_I2C_CNTL_ADDR_0_OFF);
    } else if (mempage == 0x56) {
        regval |= (0x1 << SFP_I2C_CNTL_ADDR_1_OFF);
    }

    /* gmac port 0x6 is port C1 */
    if (port == 0x6) {
        regval |= 0x1;
    } 

    /* issue command to initiate read */
    soho_cpld_write(QFX5500_SCPLD_SFP_I2C_CNTL, regval);
    
    do {
        soho_cpld_read(QFX5500_SCPLD_SFP_I2C_CNTL, &res);

        udelay(1000*10); /* 10 ms */
        if (wait_cnt++ > 0xFF) {
            printf("CPLD TIMEOUT read control reg result = 0x%x \n", res);
            return TOR_MGMT_CPLD_ERR;
        }
    } while (!(res & (1 << SFP_I2C_CNTL_DONE_OFF)));

    /* Check for bus error or slave timeout */
    if (res & (0x1 << SFP_I2C_CNTL_SLAVETIMEOUT_OFF)) {
        printf("CPLD SLAVE TIMEOUT reading from addr 0x%x\n", addr);
        return TOR_MGMT_CPLD_I2C_SLAVETIMEOUT;
    } else if (res & (0x1 << SFP_I2C_CNTL_BUSLOSS_OFF)) {
        printf("CPLD BUS LOSS reading from addr 0x%x\n", addr);
        return TOR_MGMT_CPLD_I2C_BUSLOSS;
    }

    /* Remove endianness */
    soho_cpld_read(QFX5500_SCPLD_SFP_I2C_DATA_LSB, &temp_data);

    *data = (temp_data & 0xFF) << 8; 
    soho_cpld_read(QFX5500_SCPLD_SFP_I2C_DATA_MSB, &temp_data);

    *data |= (temp_data & 0xFF);

    return TOR_MGMT_CPLD_OK;
}

/*
 * This is the function to call if you want to read 2 bytes from external SFP
 * through I2C.
 *
 * At the end of the read, the MSB of data will contain
 * data at the requested address.  The LSB will contain
 * data from the requested address + 1 (Endianness removed). 
 * 
 * This I2C transaction should retry several times. The gut of this function
 * is actually within tor_mcpld_i2c_controller_read_proc.
 */
uint32_t
tor_mcpld_i2c_controller_read (const uint32_t addr, uint16_t *data, 
                               const uint16_t port, const uint32_t mempage)
{
    uint32_t ret = 0, cnt = 0;
    const uint32_t max_try = 10;

    ret = tor_mcpld_i2c_controller_read_proc(addr, data, port, mempage);
    while (ret != TOR_MGMT_CPLD_OK) { 
	if (++cnt == max_try) {
	    break;
	}
    	ret = tor_mcpld_i2c_controller_read_proc(addr, data, port, mempage);
	udelay(100);
    }

    return ret;
}

/* 
 * UBOOT Command to read from the i2c controller.  
 */
void
tor_mcpld_i2c_controller_read_cmd (cmd_tbl_t *cmdtp, int flag, int argc, 
                                   char *argv[])
{
    uint32_t addr; 
    uint16_t data;
    uint32_t mempage;
    uint16_t port;
    uint32_t ret;

    if (argc != 4) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    port = simple_strtoul(argv[1], NULL, 16);
    addr = simple_strtoul(argv[2], NULL, 16);
    mempage = simple_strtoul(argv[3], NULL, 16);

    if (!GMAC_PORT_IS_TOR_MGMT_BRD_SFP_PORT(port)) {
        printf("ERROR - port %s unknown! use 0x5 or 0x6\n", argv[2]);
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    if ((mempage != 0xA0) && (mempage != 0xA2) && (mempage != 0x56)) {
        printf("ERROR - mempage %s unknown!\n", argv[4]);
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    ret = tor_mcpld_i2c_controller_read(addr, &data, port, mempage);
    if (ret == TOR_MGMT_CPLD_OK) {
        printf("0x%x: 0x%x\n", addr, data); 
    } else {
        switch (ret) {
            case TOR_MGMT_CPLD_I2C_BUSLOSS:
                printf("Read failed, buss loss.\n");
                break; 
            case TOR_MGMT_CPLD_I2C_SLAVETIMEOUT:
                printf("Read failed, slave timeout.\n");
                break; 
            case SOHO_CPLD_INVALID_REG:
                printf("Read failed, invalid register.\n");
                break; 
            case TOR_MGMT_CPLD_ERR:
            default:
                printf("Read failed, error.\n");
                break; 
        }
    }
}

uint32_t tor_mcpld_config_1000x (uint16_t port) 
{
    gmac_eth_info_t *eth_info;
    uint32_t data;
    uint32_t wait_cnt = 0;

    if (!GMAC_PORT_IS_TOR_MGMT_BRD_SFP_PORT(port)) {
        return TOR_MGMT_CPLD_ERR;
    }

    eth_info = eth_info_array[port];

    /* enable Secondary Serdes 1000BASE-X */
    BCM54XXX_PHY_WRITE(eth_info, 0x1c, 0xD001);

    BCM54XXX_PHY_WRITE(eth_info, 0x17, 0x0E00);

    /* reset secondary SerDes */
    BCM54XXX_PHY_WRITE(eth_info, 0x15, 0x9140);

    /* power up secondary SerDes */
    BCM54XXX_PHY_WRITE(eth_info, 0x15, 0x1140);

    /* wait until secondary Serdes is back up */
    udelay(1000*10); /* 10 ms */
    while (wait_cnt++ <= 0xFF) {
        BCM54XXX_PHY_READ(eth_info, 0x15, &data);

        if (!(data & (0x1 << 11))) {
            break;    
        }
        udelay(1000*10); /* 10 ms */
    }

    if (wait_cnt > 0xFF) {
        printf("Secondary Serdes failed to come up.\n");
        return TOR_MGMT_CPLD_ERR;
    }

    /* Select signal detect, and sync state to secondary SerDes for autodetect 
     * option. Make secondary SerDes drive LED Mode. */
    BCM54XXX_PHY_WRITE(eth_info, 0x1C, 0xD18F);

    return TOR_MGMT_CPLD_OK;
}

/* 
 * UBOOT Command to configure 1000Base-X 
 */
void
tor_mcpld_config_1000x_cmd (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uint16_t port;
    uint32_t ret;

    if (argc != 2) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    port = simple_strtoul(argv[1], NULL, 16);

    if (!GMAC_PORT_IS_TOR_MGMT_BRD_SFP_PORT(port)) {
        printf("ERROR - Command is suitable for gmac port "
                "0x5 and 0x6 only!\n");
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    ret = tor_mcpld_config_1000x(port);
    if (ret == TOR_MGMT_CPLD_OK) {
        printf("Successfully configured gmac port 0x%x to 1000Base-X\n",
               port);
    } else {
        printf("Configuration failed.\n");
    }
}

uint32_t tor_mcpld_config_copper (uint16_t port) 
{
    gmac_eth_info_t *eth_info;

    if (!GMAC_PORT_IS_TOR_MGMT_BRD_SFP_PORT(port)) {
        return TOR_MGMT_CPLD_ERR;
    }

    eth_info = eth_info_array[port];

    /* enable writing to copper register */
    BCM54XXX_PHY_WRITE(eth_info, 0x1C, 0xFC04);

    /* Power down the copper interface */
    BCM54XXX_PHY_WRITE(eth_info, 0x00, 0x1940);

    /* Enable secondary SerDes,
     * Select Signal Detect for secondary SerDes,
     * select sync function from secondary SerDes,
     * use Secondary Serdes to drive LED.
     */
    /* write command */
    BCM54XXX_PHY_WRITE(eth_info, 0x1C, 0xD08F);

    /* Disable power down mode */
    BCM54XXX_PHY_WRITE(eth_info, 0x17, 0xE00);
    BCM54XXX_PHY_WRITE(eth_info, 0x15, 0x1140);

    /* Enable SGMII-slave mode */
    BCM54XXX_PHY_WRITE(eth_info, 0x17, 0xE15);
    BCM54XXX_PHY_WRITE(eth_info, 0x15, 0x02);

    return TOR_MGMT_CPLD_OK;
}

/* 
 * UBOOT Command to configure copper PHY 
 */
void
tor_mcpld_config_copper_cmd (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uint16_t port;
    uint32_t ret;

    if (argc != 2) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    port = simple_strtoul(argv[1], NULL, 16);
    if (!GMAC_PORT_IS_TOR_MGMT_BRD_SFP_PORT(port)) {
        printf("ERROR - Command is suitable for gmac port "
                "0x5 and 0x6 only!\n");
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    ret = tor_mcpld_config_copper(port);
    if (ret == TOR_MGMT_CPLD_OK) {
        printf("Successfully configured gmac port 0x%x to Copper PHY\n",
               port);
    } else {
        printf("Configuration failed.\n");
    }
}

uint32_t tor_mcpld_config_copper_sfp (const uint16_t port) 
{
    const uint32_t mempage = 0x56; /* I2C address to reach SFP regs */
    uint16_t data = 0x0;
    uint16_t wait_cnt = 0;
    const uint16_t max_tries = 0x64;
    uint32_t ret;
 
    /* Enable SGMII mode w/o clock Auto-Negotiation to copper 8 */
    ret = tor_mcpld_i2c_controller_read (SFP_EXT_PHY_SPEC_STATUS_REG, &data,
	port, mempage);
    if (ret != TOR_MGMT_CPLD_OK) {
	return ret;
    }

    data &= 0xFFF0;
    data |= 0x4;

    ret = tor_mcpld_i2c_controller_write(SFP_EXT_PHY_SPEC_STATUS_REG, data,
	port, mempage);
    if (ret != TOR_MGMT_CPLD_OK) {
	return ret;
    }

    /* Enable the copper bank registers */    
    ret = tor_mcpld_i2c_controller_write(SFP_EXT_ADDR_REG, 0x0, port, mempage);
    if (ret != TOR_MGMT_CPLD_OK) {
	return ret;
    }

    /* Apply a soft reset */
    ret = tor_mcpld_i2c_controller_read(SFP_CNTL_REG, &data, port, mempage);
    if (ret != TOR_MGMT_CPLD_OK) {
	return ret;
    }

    data |= (1 << 15);

    ret = tor_mcpld_i2c_controller_write(SFP_CNTL_REG, data, port, mempage);
    if (ret != TOR_MGMT_CPLD_OK) {
	return ret;
    }

    /* Have to do it twice to make sure it's restarted */
    ret = tor_mcpld_i2c_controller_write(SFP_CNTL_REG, data, port, mempage);
    if (ret != TOR_MGMT_CPLD_OK) {
	return ret;
    }

    /* Make sure reset bit is 0 before move on */
    while (wait_cnt++ <= max_tries) {
        ret = tor_mcpld_i2c_controller_read(SFP_CNTL_REG, &data, port, mempage);
	if (ret != TOR_MGMT_CPLD_OK) {
	    return ret;
	}

        /* Reset bit has been verified to reset to 0. */
        if (!(data & (0x1 << 15))) {
            break;    
        }
        udelay(1000*10); /* 10 ms */
    }

    if (wait_cnt > max_tries) {
        printf("ERROR: SFP Reset fails!\n");
        return TOR_MGMT_CPLD_ERR;
    }

    ret = tor_mcpld_i2c_controller_read(SFP_AN_ADV_REG, &data, port, mempage);
    if (ret != TOR_MGMT_CPLD_OK) {
	return ret;
    }
        
    /* Broadcast all ability for 10/100, and by default set Pause */
    data |= ((1 << 10) | (1 << 8) | (1 << 7) | (1 << 6) | (1 << 5));

    /* By default, unset Asymmetric Pause */
    data &= ~(1 << 11);

    ret = tor_mcpld_i2c_controller_write(SFP_AN_ADV_REG, data, port, mempage);
    if (ret != TOR_MGMT_CPLD_OK) {
	return ret;
    }
    
    /* Broadcast ability for 1000Base-T Full duplex. 
     * 1000Base-T Half Duplex is not configured. */    
    ret = tor_mcpld_i2c_controller_read(SFP_1000BASE_T_CNTL_REG, &data, port,
	mempage);
    if (ret != TOR_MGMT_CPLD_OK) {
	return ret;
    }
        
    data |= (1 << 9);

    ret = tor_mcpld_i2c_controller_write(SFP_1000BASE_T_CNTL_REG, data, port,
	mempage);
    if (ret != TOR_MGMT_CPLD_OK) {
	return ret;
    }

    /* Set auto neg and apply a soft reset */
    ret = tor_mcpld_i2c_controller_read(SFP_CNTL_REG, &data, port, mempage);
    if (ret != TOR_MGMT_CPLD_OK) {
	return ret;
    }

    data |= ((1 << 12) | (1 << 15));

    ret = tor_mcpld_i2c_controller_write(SFP_CNTL_REG, data, port, mempage);
    if (ret != TOR_MGMT_CPLD_OK) {
	return ret;
    }

    /* Make sure reset bit is 0 before move on */
    while (wait_cnt++ <= max_tries) {
        ret = tor_mcpld_i2c_controller_read(SFP_CNTL_REG, &data, port, mempage);
	if (ret != TOR_MGMT_CPLD_OK) {
	    return ret;
	}

        /* Reset bit has been verified to reset to 0. */
        if (!(data & (0x1 << 15))) {
            break;    
        }
        udelay(1000*10); /* 10 ms */
    }

    if (wait_cnt > max_tries) {
        printf("ERROR: SFP 2nd Reset fails!\n");
        return TOR_MGMT_CPLD_ERR;
    }

    return TOR_MGMT_CPLD_OK;
}

/* 
 * UBOOT Command to configure copper SFP.
 * For Marvel 88E1111, we need to additionally configure the SFP
 * for it to work for 10/100/1000Base-T.
 */
void
tor_mcpld_config_copper_sfp_cmd (cmd_tbl_t *cmdtp, int flag, int argc, 
                                 char *argv[]) 
{
    uint16_t port;
    uint32_t ret;

    if (argc != 2) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    port = simple_strtoul(argv[1], NULL, 16);
    if (!GMAC_PORT_IS_TOR_MGMT_BRD_SFP_PORT(port)) {
        printf("ERROR - Command is suitable for gmac port "
                "0x5 and 0x6 only!\n");
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    ret = tor_mcpld_config_copper_sfp (port);
    if (ret == TOR_MGMT_CPLD_OK) {
        printf("Successfully configured gmac port 0x%x to Copper PHY\n",
               port);
    } else {
        printf("Configuration failed. Err code 0x%x\n", ret);
    }
}

/* 
 * The following will check SFP link status,
 * specifically for Marvell 88E1111 
 */
void tor_mcpld_check_copper_link_status (const uint16_t port) 
{
    uint16_t data = 0;
    uint32_t data32 = 0, ret = 0;
    gmac_eth_info_t *eth_info;

    if (!GMAC_PORT_IS_TOR_MGMT_BRD_SFP_PORT(port)) {
        return;
    }

    eth_info = eth_info_array[port];

    /* Check from SFP/Transceiver side */
    printf("SFP Link status info for gmac port 0x%x\n", port);

    /* Read Status Register - Copper */
    ret = tor_mcpld_i2c_controller_read (SFP_COPPER_STATUS_REG, &data, port,
	SFP_PHY_I2C_ADDR);
    if (ret != TOR_MGMT_CPLD_OK) {
	printf("WARNING: I2C access error 0x%x\n", ret);
    }

    printf("Copper Link Status (whether link was lost since last read): ");
    if (data & (0x1 << 2)) {
        printf(" Up\n");
    } else {
        printf(" Down\n");
    }

    printf("Copper Link (real time): ");    
    ret = tor_mcpld_i2c_controller_read (SFP_COPPER_PHY_SPEC_STATUS_REG,
	&data, port, SFP_PHY_I2C_ADDR);
    if (ret != TOR_MGMT_CPLD_OK) {
	printf("WARNING: I2C access error 0x%x\n", ret);
    }

    if (data & (0x1 << 10)) {
        printf(" Up\n");
    } else {
        printf(" Down\n");
    }

    printf("Copper Link Speed: ");
    if ((data & (0x1 << 15)) && ~(data & (0x1 << 14))) {
        printf("1000 Mbps ");
    } else if (~(data & (0x1 << 15)) && (data & (0x1 << 14))) {
        printf("100 Mbps ");
    } else if (~(data & (0x1 << 15)) && ~(data & (0x1 << 14))) {
        printf("10 Mbps ");
    } else {
        printf("Unknown ");
    }

    if (data & (0x1 << 13)) {
        printf("Full-duplex\n");
    } else {
        printf("Half-duplex\n");
    }

    /* Checking from PHY side */
    /* reading link status */
    BCM54XXX_PHY_WRITE(eth_info, 0x1c, 0x5000);
    BCM54XXX_PHY_READ(eth_info, 0x1c, &data32);
    
    printf("Secondary Serdes Link: ");
    if (data32 & (0x1 << 8)) {
        printf("Up\n");
    } else {
        printf("Down\n");
    }

    /* Full or half duplex, speed */
    BCM54XXX_PHY_WRITE(eth_info, 0x1c, 0x5400);
    BCM54XXX_PHY_READ(eth_info, 0x1c, &data32);

    printf("SGMII Slave Link Speed: ");
    if (((data32 >> 0x6) & 0x3) == 0x00) {
        printf("10 Mbps ");    
    } else if (((data32 >> 0x6) & 0x3) == 0x1) {
        printf("100 Mbps ");    
    } else if (((data32 >> 0x6) & 0x3) == 0x2) {
        printf("1000 Mbps ");    
    } else {
        printf("Unknown ");
    }

    if (data32 & (0x1 << 8)) {
        printf("Full-duplex\n");
    } else {
        printf("Half-duplex\n");
    }
}

/* 
 * UBOOT Command to check link status 
 */
void
tor_mcpld_check_copper_link_status_cmd (cmd_tbl_t *cmdtp, int flag, int argc, 
                                        char *argv[]) 
{
    uint16_t port;
    uint32_t ret;

    if (argc != 2) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    port = simple_strtoul(argv[1], NULL, 16);
    if (!GMAC_PORT_IS_TOR_MGMT_BRD_SFP_PORT(port)) {
        printf("ERROR - Command is suitable for gmac port"
                " 0x5 and 0x6 only!\n");
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    tor_mcpld_check_copper_link_status (port);
}

/* 
 * Configure for self-loopback.
 */
uint32_t tor_mcpld_config_self_loopback (const uint16_t port) 
{
    const uint32_t mempage = 0x56; /* I2C address to reach SFP regs */
    uint32_t ret;

    if (!GMAC_PORT_IS_TOR_MGMT_BRD_SFP_PORT(port)) {
        return TOR_MGMT_CPLD_ERR;
    }
   
    /* Disable all interrupts */ 
    ret = tor_mcpld_i2c_controller_write(0x12, 0x0, port, mempage);
    if (ret != TOR_MGMT_CPLD_OK) {
	return ret;		
    }

    /* Force master */
    ret = tor_mcpld_i2c_controller_write(0x9, 0x1E00, port, mempage);
    if (ret != TOR_MGMT_CPLD_OK) {
	return ret;		
    }

    /* Soft reset */
    ret = tor_mcpld_i2c_controller_write(0x0, 0x9140, port, mempage);
    if (ret != TOR_MGMT_CPLD_OK) {
	return ret;		
    }

    /* Force Gigabit Mode */ 
    ret = tor_mcpld_i2c_controller_write(0x1D, 0x7, port, mempage);
    if (ret != TOR_MGMT_CPLD_OK) {
	return ret;		
    }

    ret = tor_mcpld_i2c_controller_write(0x1E, 0x808, port, mempage);
    if (ret != TOR_MGMT_CPLD_OK) {
	return ret;		
    }

    /* Enable Gigabit Stub Loopback and CRC checker */
    ret = tor_mcpld_i2c_controller_write(0x1D, 0x10, port, mempage);
    if (ret != TOR_MGMT_CPLD_OK) {
	return ret;		
    }

    ret = tor_mcpld_i2c_controller_write(0x1E, 0x13, port, mempage);
    if (ret != TOR_MGMT_CPLD_OK) {
	return ret;		
    }

    /* Turn off NEXT Canceller */
    ret = tor_mcpld_i2c_controller_write(0x1D, 0x12, port, mempage);
    if (ret != TOR_MGMT_CPLD_OK) {
	return ret;		
    }

    ret = tor_mcpld_i2c_controller_write(0x1E, 0x8901, port, mempage);
    if (ret != TOR_MGMT_CPLD_OK) {
	return ret;		
    }

    return TOR_MGMT_CPLD_OK;
}

/* 
 * UBOOT Command to config self loopback
 */
void
tor_mcpld_config_self_loopback_cmd (cmd_tbl_t *cmdtp, int flag, int argc, 
                                    char *argv[]) 
{
    uint16_t port;
    uint32_t ret;

    if (argc != 2) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    port = simple_strtoul(argv[1], NULL, 16);
    if (!GMAC_PORT_IS_TOR_MGMT_BRD_SFP_PORT(port)) {
        printf("ERROR - Command is suitable for gmac port"
                " 0x5 and 0x6 only!\n");
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    if (ret = tor_mcpld_config_self_loopback (port)) {
	printf("ERROR %d\n", ret);
    }
}

/* 
 * Only for tor board which has SFP mgmt board, 
 * and for gmac port 5 and 6 
 */
void fx_init_gmac_port (const int gmac_id) 
{
    uint8_t data_8 = 0;
    uint8_t     res = 0;
    uint16_t cntl_reg;
    uint32_t ret = 0;

    if (gmac_id == 5) {
        cntl_reg = SOHO_SCPLD_SFP_C0_PRE;
    } else if (gmac_id == 6) {
        cntl_reg = SOHO_SCPLD_SFP_C1_PRE;
    } else {
        return;
    }

    /* Enable transmit */
    soho_cpld_write(cntl_reg, 0x20);

    /* Disable all interrupts at uBoot level */
    soho_cpld_write(SOHO_SCPLD_MGT_BRD_INTR_EN, 0x0);

    /* Check presence of SFP and configure SFP */
    soho_cpld_read(cntl_reg, &res);
    if (res & (0x1 << 4)) {
        b2_debug("gmac%d: SFP present at gmac port %d!\n", gmac_id, gmac_id);
    
        /* Check type of SFP at register 6 */

	ret = tor_mcpld_i2c_controller_read_8 (SFP_EEPROM_ETHER_TYPE_REG,
				     &data_8, gmac_id, SFP_EEPROM_I2C_ADDR);
	if (ret != TOR_MGMT_CPLD_OK) {
	    printf("WARNING: I2C access error 0x%x\n", ret);
	}

	/* Need more delay for external SFP PHY initialization */
	udelay(2000000);

        if ((data_8 & (0x1 << SFP_EEPROM_1000BASE_SX_OFFSET)) || 
            (data_8 & (0x1 << SFP_EEPROM_1000BASE_LX_OFFSET))) {

            b2_debug("gmac%d: Configuring SFP for 1000Base-X\n", gmac_id);
            /* SGMII-to-1000Base-X */
            tor_mcpld_config_1000x(gmac_id);

        } else if (data_8 & (0x1 << SFP_EEPROM_1000BASE_T_OFFSET)) {
            b2_debug("gmac%d: Configuring SFP for 10/100/1000Base-T\n",
		     gmac_id);

            /* Copper 10/100/1000Base-T requires additional configuration
	       for external PHY inside the SFP-T module. */
	    ret = tor_mcpld_config_copper(gmac_id);
	    if (ret != TOR_MGMT_CPLD_OK) {
		/* Wrong port */
		printf("gmac%d: Wrong SFP-T port\n", gmac_id);
		return;
	    }
            ret = tor_mcpld_config_copper_sfp(gmac_id);
            if (ret != TOR_MGMT_CPLD_OK) {
		printf("gmac%d: Configuration of SFP-T failed.  Err: 0x%x\n",
		       gmac_id, ret);
	    }
        } else {
            b2_debug("gmac%d: SFP type unknown! Defaulted to 1000Base-X\n",
		     gmac_id);
            /* SGMII-to-1000Base-X */
            tor_mcpld_config_1000x(gmac_id);
        }
    }
}

U_BOOT_CMD(
    tor_mcpld_write,  
    5,   
    1,   
    tor_mcpld_i2c_controller_write_cmd,
    "tor_mcpld_write <gmac port> <address> <data> <0xA0|0xA2|0x56>\n"
    "          -- write to SFP MCPLD\n",
    "write to CPLD"
);

U_BOOT_CMD(
    tor_mcpld_read,  
    4,   
    1,   
    tor_mcpld_i2c_controller_read_cmd,
    "tor_mcpld_read <gmac port> <address> <0xA0|0xA2|0x56>\n",
    "read CPLD"
);

U_BOOT_CMD(
    tor_mcpld_config_1000x,  
    2,   
    1,   
    tor_mcpld_config_1000x_cmd,
    "tor_mcpld_config_1000x <gmac port>\n",
    "configure 1000Base-X"
);

U_BOOT_CMD(
    tor_mcpld_config_copper,  
    2,   
    1,   
    tor_mcpld_config_copper_cmd,
    "tor_mcpld_config_copper <gmac port>\n",
    "configure copper PHY"
);

U_BOOT_CMD(
    tor_mcpld_config_copper_sfp,  
    2,   
    1,   
    tor_mcpld_config_copper_sfp_cmd,
    "tor_mcpld_config_copper_sfp <gmac port>\n",
    "configure copper SFP"
);

U_BOOT_CMD(
    tor_mcpld_check_copper_link_status,  
    2,   
    1,   
    tor_mcpld_check_copper_link_status_cmd,
    "tor_mcpld_check_copper_link_status <gmac port>\n",
    "check link status on CPLD and SFP"
);

U_BOOT_CMD(
    tor_mcpld_config_self_loopback,  
    2,   
    1,   
    tor_mcpld_config_self_loopback_cmd,
    "tor_mcpld_config_self_loopback <gmac port>\n",
    "configure self-loopback"
);
