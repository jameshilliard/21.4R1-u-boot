/*
 * Copyright (c) 2007-2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include "epld.h"
#include "led.h"
#include <i2c.h>
#include "fsl_i2c.h"

#undef debug
#ifdef	EX3242_DEBUG
#define debug(fmt,args...)	printf (fmt ,##args)
#else
#define debug(fmt,args...)
#endif

extern int board_id(void);

/* Network mode */
#define NETWORK_MODE_SHIFT 6
#define NETWORK_MODE_INDEX 0
#define NETWORK_MODE_BIT_MASK 0xC0

/* port LEDs */
#define NUM_BITS_PER_PORT_LED         4
#define NUM_BITS_PER_PORT_LED_REG 16
#define PORT_LED_BIT_MASK          0xF

/* mgmt port LEDs */
#define MGMT_PORT_SHIFT 12

/* Other LEDs */
#define NUM_BITS_PER_REG         8
#define NUM_LEDS_PER_BYTE       4
#define NUM_INTERVALS       10  /* 1 sec / 100ms */
#define LED_I2C_REG      {0x08, 0x09}
#define LED_I2C_REG_MASK      {0xcc, 0x03} /* bits used by LED */

/* hard coded for LEDs */
static const uint8_t led_bit_mask[LAST_LED] = 
    {0x00, 0x0c, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x00};
    
static const uint8_t led_bit_pattern[LAST_COLOR][LAST_LED] = 
    {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
      {0x00, 0x08, 0x00, 0x80, 0x02, 0x00, 0x00, 0x00},
      {0x00, 0x04, 0x00, 0x40, 0x01, 0x00, 0x00, 0x00},
      {0x00, 0x0c, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x00}};

static const uint8_t swap4[] = 
      {0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
        0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf};

uint8_t ledStore[NUM_INTERVALS][LAST_LED /NUM_LEDS_PER_BYTE];
uint8_t lastLedData[LAST_LED /NUM_LEDS_PER_BYTE];
int ledLastInterval;

void port_led_init (void)
{
    /* Reset value shall be OK */
}

/* port number are 1 based.  1-24/50.  Port 0 for mgmt port */
int set_port_led(int port, PortLink lnk, PortDuplex dup, PortSpeed spd)
{
    int reg, bitShift;
    uint16_t tmp, val = 0;
    int bid = board_id();
    
    if (port > NUM_EXT_PORTS) {
        debug ("Port number %d is out of range 0-%d.\n", port, NUM_EXT_PORTS);
        return 0;
    }

    /* kluge to work around for port 21-24 port copper speed/duplex LEDs. */
    if (((bid == I2C_ID_JUNIPER_EX3200_24_T) ||
         (bid == I2C_ID_JUNIPER_EX3200_24_P)) && 
         (port >= 21) &&
         (port <= 24))
        port += 24;
	
    if (port == 0)
        reg = EPLD_MISC_CONTROL;
    else if ((port == 49) || (port == 50))
        reg = EPLD_WATCHDOG_LEVEL_DEBUG;
    else    
        reg = EPLD_FIRST_PORT_LED_REG + 
             ((port - 1) / (NUM_BITS_PER_PORT_LED_REG / NUM_BITS_PER_PORT_LED));

    switch (lnk) {
        case LINK_UP:
            val += EPLD_PORT_LINK_UP;
            break;
        case LINK_DOWN:
            val += EPLD_PORT_NO_LINK;
            break;
        default:
	     debug ("Link state %d is not valid.   0-link down, 1-link up.\n", lnk);
            return 0;
    }
    
    switch (dup) {
        case HALF_DUPLEX:
            val += EPLD_PORT_HALF_DUPLEX;
            break;
        case FULL_DUPLEX:
            val += EPLD_PORT_FULL_DUPLEX;
            break;
        default:
	     debug ("Duplex %d is not valid.   0-half duplex, 1-full duplex.\n", dup);
            return 0;
    }

    switch (spd) {
        case SPEED_10M:
            val += EPLD_PORT_SPEED_10M;
            break;
        case SPEED_100M:
            val += EPLD_PORT_SPEED_100M;
            break;
        case SPEED_1G:
            val += EPLD_PORT_SPEED_1G;
            break;
        case SPEED_10G:
            val += EPLD_PORT_SPEED_10G;
            break;
        default:
	     debug ("Speed %d is not valid.   0-10M, 1-100M, 2-1G, 3-10G.\n", spd);
            return 0;
    }

    if (!epld_reg_read(reg, &tmp)) {
	 debug ("Unable to read EPLD register address 0x%x.\n", reg*2);
        return 0;
    }
    
    /* adjust for port number */
    if (port == 0) {
        bitShift = MGMT_PORT_SHIFT;
        /* Kludge.  Java SKU EPLD has mgmt port bits reversed vs front panel ports.  
            Swap 4 bits. */
        val = swap4[0xF & val];
    }
    else    
        bitShift = (NUM_BITS_PER_PORT_LED * 
                  ((NUM_BITS_PER_PORT_LED_REG / NUM_BITS_PER_PORT_LED) -
                   (port % (NUM_BITS_PER_PORT_LED_REG / NUM_BITS_PER_PORT_LED)))) % 
                   NUM_BITS_PER_PORT_LED_REG;

    tmp &= (~(PORT_LED_BIT_MASK << bitShift));
    tmp |= (val << bitShift);
    
    return epld_reg_write(reg, tmp);
}

void led_init (void)
{
    memset (&ledStore, 0, NUM_INTERVALS * (LAST_LED /NUM_LEDS_PER_BYTE) * sizeof (uint8_t));
    memset (&lastLedData, 0, (LAST_LED /NUM_LEDS_PER_BYTE) * sizeof (uint8_t));
    ledLastInterval = 0;
}

int set_led(SysLed led, LedColor color, LedState state)
{
    int i, index = 0;

    if ((led != BOTTOM_LED) &&
        (led != MIDDLE_LED) &&
        (led != TOP_LED)) {
	     debug ("LED %d is not valid.   1-bottom, 3-middle, 4-top.\n", led);
            return 0;
    } 
    
    if ((color  < LED_OFF) || (color > LED_AMBER)) {
	     debug ("LED color %d is not valid.   0-Off, 1-Amber, 2-Green.\n", color);
            return 0;
    }

    if ((state  < LED_STEADY) || (state  > LED_FAST_BLINKING)) {
	     debug ("LED state %d is not valid.   0-Steady, 1-Slow blinking, 2-Fast blinking.\n", state);
            return 0;
    }

    index = (led > MIDDLE_LED) ? 1 : 0;
    
    switch (state) {
        case LED_STEADY:
            for (i = 0; i < NUM_INTERVALS; i++)
                ledStore[i][index] = (ledStore[i][index] & ~led_bit_mask[led]) | led_bit_pattern[color][led];
            break;
        case LED_SLOW_BLINKING:
            for (i = 0; i < NUM_INTERVALS; i++) {
                if (i < (NUM_INTERVALS / 2))
                    ledStore[i][index] = (ledStore[i][index] & ~led_bit_mask[led]) | led_bit_pattern[color][led];
                else
                    ledStore[i][index] = ledStore[i][index] & ~led_bit_mask[led];
            }
            break;
        case LED_FAST_BLINKING:
            for (i = 0; i < NUM_INTERVALS; i++) {
                if (i  % 2)
                    ledStore[i][index] = (ledStore[i][index] & ~led_bit_mask[led]) | led_bit_pattern[color][led];
                else
                    ledStore[i][index] = ledStore[i][index] & ~led_bit_mask[led];
            }
            break;
    }
    return 1;
}

void update_led(void)
{
    uint8_t i2c_addr[] = LED_I2C_REG;
    uint8_t reg_mask[] = LED_I2C_REG_MASK;
    int i, currInt, currCtrl;
    uint8_t data;

    currInt = ledLastInterval + 1;
    if (currInt >= NUM_INTERVALS)
        currInt = 0;

    for (i = 0; i < LAST_LED /NUM_LEDS_PER_BYTE; i++) {
        if (lastLedData[i] != ledStore[currInt][i]) {
            currCtrl = current_i2c_controller();
            i2c_controller(CFG_I2C_CTRL_2);
            data = i2c_reg_read(CFG_I2C_CTRL_2_IO_ADDR, i2c_addr[i]);
            /* preserve other bits not used by LED */
            data &= ~reg_mask[i];
            data |= ledStore[currInt][i];
            i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, i2c_addr[i], data);
            i2c_controller(currCtrl);
            lastLedData[i] = ledStore[currInt][i];
        }
    }

    ledLastInterval = currInt;
}

int set_network_mode(NetworkMode mode)
{
    uint8_t val = 0;
    int i;
    uint8_t i2c_addr[] = LED_I2C_REG;
    
    if ((mode < MODE_1G) || (mode > MODE_10G)) {
	 debug ("Network mode %d is not valid.   1-1G, 2-10G.\n", mode);
        return 0;
    }
    
    val = ((uint8_t )mode) << NETWORK_MODE_SHIFT;
    for (i = 0; i < NUM_INTERVALS; i++)
        ledStore[i][NETWORK_MODE_INDEX] = (ledStore[i][NETWORK_MODE_INDEX] & NETWORK_MODE_BIT_MASK) | val;

    /* update now */
    i2c_controller(CFG_I2C_CTRL_2);
    i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, i2c_addr[NETWORK_MODE_INDEX], ledStore[ledLastInterval][NETWORK_MODE_INDEX]);
    i2c_controller(CFG_I2C_CTRL_1);

    return 1;
}

