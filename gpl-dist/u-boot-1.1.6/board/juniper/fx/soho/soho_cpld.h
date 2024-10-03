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

#include <rmi/gpio.h>
#include <rmi/pcmcia.h>
#include <rmi/on_chip.h>
#include <rmi/board.h>
#include "../common/fx_cpld.h"
#include "common.h"
#include "../common/fx_common.h"

#ifndef _SOHO_CPLD_H_
#define _SOHO_CPLD_H_

typedef enum 
{
    SOHO_CPLD_OK = 0,
    TOR_CPLD_OK = SOHO_CPLD_OK,
    TOR_MGMT_CPLD_OK = SOHO_CPLD_OK,
    SOHO_CPLD_ERR,
    TOR_CPLD_ERR = SOHO_CPLD_ERR,
    TOR_MGMT_CPLD_ERR = SOHO_CPLD_ERR,
    SOHO_CPLD_NULL,
    SOHO_CPLD_INVALID_REG,
    SOHO_CPLD_BUSY,
    SOHO_CPLD_MDIO_OP_BUSY = 5,
    SOHO_CPLD_MDIO_NO_ACK,
    TOR_CPLD_I2C_BUSY,
    TOR_CPLD_I2C_NOT_DONE,
    TOR_CPLD_I2C_RBERR,
    TOR_CPLD_I2C_NACK = 10,
    TOR_MGMT_CPLD_I2C_BUSLOSS,
    TOR_MGMT_CPLD_I2C_SLAVETIMEOUT,
} soho_cpld_status_t;

typedef struct soho_fan_config_
{
    boolean fan_front;
    boolean fan_rear;
    uint8_t fan_speed;
} soho_fan_config_t;

typedef enum
{
    SOHO_PHY_LED_OFF,
    SOHO_PHY_LED_GREEN,
    SOHO_PHY_LED_AMBER
} soho_phy_led_color;

#define FAN_SPEED_F_MASK            0xF
#define FAN_SPEED_F_SHIFT           0
#define FAN_SPEED_R_MASK            0xF0
#define FAN_SPEED_R_SHIFT           0x4

#define MDIO_C45_ADDR               0x0
#define MDIO_C45_WRITE              0x1
#define MDIO_C45_RINCR              0x2
#define MDIO_C45_READ               0x3

#define MDIO_C22_WRITE              0x1
#define MDIO_C22_READ               0x2

#define SOHO_CPLD_MDIO_22_TIME_PARA 0xC0

#define MGMT_SFP_PRESENCE_CHANGE    0x04
#define MGMT_SFP_PRESENCE           0x10

void soho_cpld_init (struct pio_dev *mcpld, struct pio_dev *scpld, struct pio_dev *acc_fpga,
                struct pio_dev *reset_cpld);
soho_cpld_status_t soho_cpld_read(uint8_t reg_id, uint8_t *reg_val);
soho_cpld_status_t soho_cpld_write(uint8_t reg_id, uint8_t reg_val);
uint8_t soho_cpld_get_version(void);
soho_cpld_status_t soho_cpld_qsfp_enable(void);
soho_cpld_status_t soho_cpld_mdio_c45_write(uint32_t dev_type, uint32_t phy_addr, 
                                            uint32_t reg_id, uint16_t data,
                                            uint32_t time_para);
soho_cpld_status_t soho_cpld_mdio_c45_read (uint32_t dev_type, uint32_t phy_addr, 
                                            uint32_t reg_id, uint16_t *data, 
                                            uint32_t time_para);
soho_cpld_status_t soho_cpld_mdio_c22_read(uint32_t phy_addr, uint32_t reg_id, 
                                           uint32_t *data, uint8_t time_para);
soho_cpld_status_t soho_cpld_mdio_c22_write(uint32_t phy_addr, uint32_t reg_id, 
                                            uint16_t data, uint8_t time_para);
void soho_cpld_set_phy_led(uint32_t port_id, uint32_t green);
void soho_cpld_select_phy_led (uint32_t port_id, int select);

uint32_t soho_cpld_mdio_set_mux(const uint32_t mux);

extern struct pio_dev fx_mcpld_dev;
extern struct pio_dev tor_scpld_dev;
extern struct pio_dev pa_acc_fpga_dev;
extern struct pio_dev soho_reset_cpld_dev;
extern struct pio_dev cfi_flash_dev;
#endif

