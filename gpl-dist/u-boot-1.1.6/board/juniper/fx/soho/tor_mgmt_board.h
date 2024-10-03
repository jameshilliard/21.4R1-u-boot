/*
 * $Id$
 *
 * tor_mgmt_board.h
 *
 * TOR Mgmt Board related APIs, its registers and offsets.
 *
 * Copyright (c) 2011-2012, Juniper Networks, Inc.
 * All rights reserved.
 */

#ifndef _TOR_MGMT_BOARD_H_
#define _TOR_MGMT_BOARD_H_

uint32_t tor_mcpld_i2c_controller_write(
const uint32_t, const uint16_t, const uint16_t, const uint32_t);

uint32_t tor_mcpld_i2c_controller_read(
const uint32_t, uint16_t*, const uint16_t, const uint32_t);

uint32_t tor_mcpld_config_1000x(uint16_t);
uint32_t tor_mcpld_config_copper(uint16_t);
void fx_init_gmac_port(const int);

#define GMAC_PORT_IS_TOR_MGMT_BRD_SFP_PORT(gmac) \
((gmac == 5) || (gmac == 6))

/* offset of SFP_I2C_CNTL register */
#define SFP_I2C_CNTL_BUSLOSS_OFF            7
#define SFP_I2C_CNTL_SLAVETIMEOUT_OFF       6
#define SFP_I2C_CNTL_DONE_OFF               5
#define SFP_I2C_CNTL_START_TRANS_OFF        4
#define SFP_I2C_CNTL_RW_OFF                 3
#define SFP_I2C_CNTL_ADDR_1_OFF             2
#define SFP_I2C_CNTL_ADDR_0_OFF             1

/* SFP Registers for MRVL88E1111 */
#define SFP_CNTL_REG                        0x0
#define SFP_COPPER_STATUS_REG               0x1 
#define SFP_AN_ADV_REG                      0x4
#define SFP_1000BASE_T_CNTL_REG             0x9
#define SFP_COPPER_PHY_SPEC_STATUS_REG      0x11
#define SFP_EXT_ADDR_REG                    0x16
#define SFP_EXT_PHY_SPEC_STATUS_REG         0x1b

/* SFP EEPROM Content Registers */
#define SFP_EEPROM_ETHER_TYPE_REG           0x6
#define SFP_EEPROM_1000BASE_T_OFFSET        0x3
#define SFP_EEPROM_1000BASE_LX_OFFSET       0x1
#define SFP_EEPROM_1000BASE_SX_OFFSET       0x0

#define SFP_EEPROM_I2C_ADDR                 0xA0
#define SFP_PHY_I2C_ADDR                    0x56

#endif /* _TOR_MGMT_BOARD_H_ */
