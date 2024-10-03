/*
 * Copyright (c) 2011-2012, Juniper Networks, Inc.
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
#include <asm/fsl_i2c.h>

#undef debug
#ifdef	EX45XX_DEBUG
#define debug(fmt,args...)	printf (fmt ,##args)
#else
#define debug(fmt,args...)
#endif

/* hard coded for LEDs */
static const uint8_t led_bit_mask[LAST_LED] = 
    {0x00, 0x0c, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x00};
    
static const uint8_t led_bit_mask_back[LAST_LED_BACK] = 
    {0x03, 0x0c, 0x30, 0x00};
    
static const uint8_t led_bit_pattern[LAST_COLOR][LAST_LED] = 
    {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
      {0x00, 0x08, 0x00, 0x80, 0x02, 0x00, 0x00, 0x00},
      {0x00, 0x04, 0x00, 0x40, 0x01, 0x00, 0x00, 0x00},
      {0x00, 0x0c, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x00}};

static const uint8_t led_bit_pattern_back[LAST_COLOR][LAST_LED_BACK] = 
    {{0x00, 0x00, 0x00, 0x00},
      {0x02, 0x08, 0x20, 0x00},
      {0x01, 0x04, 0x10, 0x00},
      {0x03, 0x0c, 0x30, 0x00}};

static const uint8_t swap4[] = 
      {0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
        0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf};

uint8_t ledStore[NUM_INTERVALS][LAST_LED /NUM_LEDS_PER_BYTE];
uint8_t ledStoreBack[NUM_INTERVALS][LAST_LED_BACK /NUM_LEDS_PER_BYTE];
uint8_t lastLedData[LAST_LED /NUM_LEDS_PER_BYTE];
uint8_t lastLedDataBack[LAST_LED_BACK /NUM_LEDS_PER_BYTE];
int ledLastInterval, ledLastIntervalBack;

void port_led_init (void)
{
    /* Reset value shall be OK */
}

/* port number are 1 based.  1-24/50.  Port 0 for mgmt port */
int set_port_led (int port, PortLink lnk, PortDuplex dup, PortSpeed spd)
{
	int reg, bitShift;
	uint16_t tmp, val = 0;
	
	if (port > NUM_EXT_PORTS) {
	    debug ("Port number %d is out of range 0-%d\n",
			port, NUM_EXT_PORTS);
	    return 0;
	}

	if (port == 0)
	    reg = EPLD_MISC_CONTROL;
	else    
	    reg = EPLD_FIRST_PORT_LED_REG +
		 ((port - 1) /
		 (NUM_BITS_PER_PORT_LED_REG / NUM_BITS_PER_PORT_LED));

	switch (dup) {
	case HALF_DUPLEX:
	    val += EPLD_PORT_HALF_DUPLEX;
	    break;
	case FULL_DUPLEX:
	    val += EPLD_PORT_FULL_DUPLEX;
	    break;
	default:
	    debug ("Duplex %d is not valid.   0-half duplex, 1-full duplex.\n",
			dup);
	    return 0;
	}

	if (lnk == LINK_UP) {
	    if (port == 0) {
		/* EX4500 leave duplex as 0 */
		switch (spd) {
		case SPEED_10M:
		    val = 0x8;
		    break;
		case SPEED_100M:
		    val = 0xc;
		    break;
		case SPEED_1G:
		    val = 0xa;
		    break;
		default:
		    debug ("Speed %d is not valid.   0-10M, 1-100M, 2-1G.\n",
			   spd);
		    return 0;
		}
	    } else {
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
		    debug ("Speed %d is not valid.   "
			"0-10M, 1-100M, 2-1G, 3-10G.\n", spd);
		    return 0;
		}
	    }
	}

	if (!epld_reg_read(reg, &tmp)) {
	     debug ("Unable to read EPLD register address 0x%x.\n", reg*2);
	    return 0;
	}
	
	/* adjust for port number */
	if (port == 0) {
	    bitShift = MGMT_PORT_SHIFT;
	    /* Kludge.  Java SKU EPLD has mgmt port bits reversed vs front panel 
	     * ports.  Swap 4 bits. 
	     */
	    val = swap4[0xF & val];
	}  else {
	    bitShift = (NUM_BITS_PER_PORT_LED * 
		       ((NUM_BITS_PER_PORT_LED_REG / NUM_BITS_PER_PORT_LED) -
		       (port % 
		       (NUM_BITS_PER_PORT_LED_REG / NUM_BITS_PER_PORT_LED)))) % 
		       NUM_BITS_PER_PORT_LED_REG;
	}

	tmp &= (~(PORT_LED_BIT_MASK << bitShift));
	tmp |= (val << bitShift);
	
	return epld_reg_write(reg, tmp);
}

void 
led_init (void)
{
	memset (&ledStore, 0, NUM_INTERVALS * (LAST_LED /NUM_LEDS_PER_BYTE) *
		sizeof (uint8_t));
	memset (&lastLedData, 0, 
		(LAST_LED /NUM_LEDS_PER_BYTE) * sizeof (uint8_t));
	ledLastInterval = 0;
}

void led_init_back (void)
{
	memset (&ledStoreBack, 0, NUM_INTERVALS * 
		(LAST_LED_BACK /NUM_LEDS_PER_BYTE) * sizeof (uint8_t));
	memset (&lastLedDataBack, 0, 
		(LAST_LED_BACK /NUM_LEDS_PER_BYTE) * sizeof (uint8_t));
	ledLastIntervalBack = 0;
}

int set_led (SysLed led, LedColor color, LedState state)
{
	int i, index = 0;

	if ((led != BOTTOM_LED) && (led != MIDDLE_LED) &&
			(led != TOP_LED)) {
	    debug ("LED %d is not valid.   1-bottom, 3-middle, 4-top.\n", led);
	    return 0;
	}

	if ((color  < LED_OFF) || (color > LED_AMBER)) {
	    debug ("LED color %d is not valid.   0-Off, 1-Amber, 2-Green.\n",
		    color);
	    return 0;
	}

	if ((state  < LED_STEADY) || (state  > LED_FAST_BLINKING)) {
	    debug ("LED state %d is not valid.   0-Steady, 1-Slow blinking, "
		    "2-Fast blinking.\n", state);
	    return 0;
	}

	index = (led > MIDDLE_LED) ? 1 : 0;
	
	switch (state) {
	case LED_STEADY:
	    for (i = 0; i < NUM_INTERVALS; i++)
		ledStore[i][index] = (ledStore[i][index] &
					~led_bit_mask[led]) | 
					led_bit_pattern[color][led];
	    break;
	case LED_SLOW_BLINKING:
	    for (i = 0; i < NUM_INTERVALS; i++) {
		if (i < (NUM_INTERVALS / 2))
		    ledStore[i][index] = (ledStore[i][index] &
					    ~led_bit_mask[led]) | 
					    led_bit_pattern[color][led];
		else
		    ledStore[i][index] = ledStore[i][index] &
					    ~led_bit_mask[led];
	    }
	    break;
	case LED_FAST_BLINKING:
	    for (i = 0; i < NUM_INTERVALS; i++) {
		if (i % 2)
		    ledStore[i][index] = (ledStore[i][index] &
					    ~led_bit_mask[led]) | 
					    led_bit_pattern[color][led];
		else
		    ledStore[i][index] = ledStore[i][index] &
					    ~led_bit_mask[led];
	    }
	    break;
	}
	return 1;
}

int set_led_back (SysLedBack led, LedColor color, LedState state)
{
	int i, index = 0;

	if ((led != BOTTOM_LED_BACK) && (led != MIDDLE_LED_BACK) &&
					(led != TOP_LED_BACK)) {
	    debug ("LED %d is not valid.   1-bottom, 2-middle, 3-top.\n", led);
	    return 0;
	} 
	
	if ((color  < LED_OFF) || (color > LED_AMBER)) {
	    debug ("LED color %d is not valid.   0-Off, 1-Amber, 2-Green.\n",
		    color);
	    return 0;
	}

	if ((state  < LED_STEADY) || (state  > LED_FAST_BLINKING)) {
	    debug ("LED state %d is not valid.   0-Steady, 1-Slow blinking, "
			"2-Fast blinking.\n", state);
	    return 0;
	}

	switch (state) {
	case LED_STEADY:
	    for (i = 0; i < NUM_INTERVALS; i++)
		ledStoreBack[i][index] = (ledStoreBack[i][index] &
					 ~led_bit_mask_back[led]) | 
					 led_bit_pattern_back[color][led];
	    break;
	case LED_SLOW_BLINKING:
	    for (i = 0; i < NUM_INTERVALS; i++) {
		if (i < (NUM_INTERVALS / 2))
		    ledStoreBack[i][index] = (ledStoreBack[i][index] &
					     ~led_bit_mask_back[led]) | 
					     led_bit_pattern_back[color][led];
		else
		    ledStoreBack[i][index] = ledStoreBack[i][index] &
					     ~led_bit_mask_back[led];
	    }
	    break;
	case LED_FAST_BLINKING:
	    for (i = 0; i < NUM_INTERVALS; i++) {
		if (i  % 2)
		    ledStoreBack[i][index] = (ledStoreBack[i][index] &
					     ~led_bit_mask_back[led]) | 
					     led_bit_pattern_back[color][led];
		else
		    ledStoreBack[i][index] = ledStoreBack[i][index] &
					     ~led_bit_mask_back[led];
	    }
	    break;
	}
	return 1;
}

void update_led (void)
{
	uint8_t i2c_addr[] = LED_I2C_REG;
	uint8_t i2c_mask[] = LED_I2C_REG_MASK;
	int i, currInt, currCtrl;
	uint8_t temp;

	currInt = ledLastInterval + 1;
	if (currInt >= NUM_INTERVALS)
	    currInt = 0;

	for (i = 0; i < LAST_LED /NUM_LEDS_PER_BYTE; i++) {
	    if (lastLedData[i] != ledStore[currInt][i]) {
		currCtrl = i2c_get_bus_num();
		i2c_set_bus_num(CFG_I2C_CTRL_2);
		if (i2c_mask[i]) {
		    temp = i2c_reg_read(CFG_I2C_C2_9506EXP1, i2c_addr[i]);
		    i2c_reg_write(CFG_I2C_C2_9506EXP1, i2c_addr[i], 
				  ledStore[currInt][i] | (temp & i2c_mask[i]));
		} else {
		    i2c_reg_write(CFG_I2C_C2_9506EXP1, i2c_addr[i],
				  ledStore[currInt][i]);
		}
		i2c_set_bus_num(currCtrl);
		lastLedData[i] = ledStore[currInt][i];
	    }
	}

	ledLastInterval = currInt;
}

void update_led_back (void)
{
	uint8_t i2c_addr[] = LED_I2C_REG_BACK;
	int i, currInt, currCtrl;
	uint8_t ch = 1 << 1;

	currInt = ledLastIntervalBack + 1;
	if (currInt >= NUM_INTERVALS)
	    currInt = 0;

	for (i = 0; i < LAST_LED_BACK /NUM_LEDS_PER_BYTE; i++) {
	    if (lastLedDataBack[i] != ledStoreBack[currInt][i]) {
		currCtrl = i2c_get_bus_num();
		i2c_set_bus_num(CFG_I2C_CTRL_2);
		i2c_write(CFG_I2C_C2_9546SW1, 0, 0, &ch, 1);
		i2c_reg_write(CFG_I2C_C2_9546SW1_P1_9506EXP5, i2c_addr[i],
			      ledStoreBack[currInt][i]);
		ch = 0;
		i2c_write(CFG_I2C_C2_9546SW1, 0, 0, &ch, 1);
		i2c_set_bus_num(currCtrl);
		lastLedDataBack[i] = ledStoreBack[currInt][i];
	    }
	}

	ledLastIntervalBack = currInt;
}

