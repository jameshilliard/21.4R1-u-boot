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

#ifndef __LED_H__
#define __LED_H__

/* LED states */
#define LED_GREEN_ON        1
#define LED_YELLOW_ON       2
#define LED_GREEN_BLINK     3
#define LED_YELLOW_BLINK    4
#define LED_OFF             5

/* Reg definitions on RE */
#define I2CS_LED_DEFAULT_REG_07       0x07
#define I2CS_GREEN_LED_REG_45         0x45
#define I2CS_YELLOW_LED_REG_46        0x46
#define I2CS_LED_BLINK_REG_5D         0x5D
#define I2CS_SW_LED_CONTROL_REG_5E    0x5E

/* I2CS CPLD 0x07 reg bits */
#define RESET_LED_STATE     (1 << 2)

/* I2CS CPLD 0x5D reg bits */
#define STATUS_LED_BLINK    (1 << 4)
#define MASTER_LED_BLINK    (1 << 5)
/* I2CS Reg 0x45 */
#define STATUS_LED_GREEN    (1 << 4)
#define MASTER_LED_GREEN    (1 << 5)
#define POWER_ON_LED_GREEN  (1 << 6)
/* I2CS Reg 0x46 */
#define STATUS_LED_YELLOW   (1 << 4)
#define MASTER_LED_YELLOW   (1 << 5)
#define POWER_ON_LED_YELLOW (1 << 6)
/* I2CS Reg 0x5e */
#define EN_LED_SW_CONTROL        (1 << 0)
#define EN_MASTER_LED_SW_CONTROL    (1 << 1)

/* Reg definitions on LC */
/* CTRL CPLD Regs */
#define STATUS_LED_CONTROL_REG    0x61
/* CTRL CPLD Reg 0x61 bits */
#define STS_LED_YELLOW_ON       (1 << 0)
#define STS_LED_GREEN_ON        (1 << 1)
#define STS_LED_YELLOW_BLINK    (1 << 2)
#define STS_LED_GREEN_BLINK     (1 << 3)

/* ex62xxcb fpga LED select offset - 0xa0 */
#define JCB_PHY_CTL_COPPER	(1 << 21)

void set_port_led(u_int32_t link_stat_spd);
void update_led_state(u_int8_t led_state);

extern uint8_t global_status_led_st;

#endif /* __LED_H__ */
