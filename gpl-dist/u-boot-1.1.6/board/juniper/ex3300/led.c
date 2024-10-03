/*
 * Copyright (c) 2010-2011, Juniper Networks, Inc.
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
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/gpl-2.0.html>.
 */

#include <common.h>
#include "syspld.h"
#include "led.h"

#undef debug
#ifdef	EX3300_DEBUG
#define debug(fmt,args...)	printf (fmt ,##args)
#else
#define debug(fmt,args...)
#endif

/* hard coded for sys/alm LEDs */
static const uint8_t led_bit_mask[LAST_LED] = 
    {0xc0, 0x30, 0x0c};

static const uint8_t led_bit_pattern[LAST_COLOR][LAST_LED] = 
    {{0x00, 0x00, 0x00},
      {0x80, 0x20, 0x08},
      {0x40, 0x10, 0x04},
      {0xc0, 0x30, 0x0c},
    };
      
uint8_t ledStore[NUM_INTERVALS];
uint8_t lastLedData;
int ledLastInterval;

/* port number are 0 based.  0-47. */
int 
set_port_led (int port, PortLink lnk, PortDuplex dup, PortSpeed spd)
{
    int reg, bitShift;
    uint8_t tmp, val = 0;
    
    if (port > NUM_EXT_PORTS) {
        debug ("Port number %d is out of range 0-%d.\n", port, NUM_EXT_PORTS);
        return (0);
    }

    reg = SYSPLD_FIRST_PORT_LED_REG + 
             (port  / (NUM_BITS_PER_PORT_LED_REG / NUM_BITS_PER_PORT_LED));

    switch (lnk) {
        case LINK_UP:
            val += SYSPLD_PORT_LINK_UP;
            break;
        case LINK_DOWN:
            val += SYSPLD_PORT_NO_LINK;
            break;
        default:
            debug ("Link state %d is not valid. 0-link down, "
                   "1-link up.\n", lnk);
            return (0);
    }
    
    switch (dup) {
        case HALF_DUPLEX:
            val += SYSPLD_PORT_HALF_DUPLEX;
            break;
        case FULL_DUPLEX:
            val += SYSPLD_PORT_FULL_DUPLEX;
            break;
        default:
            debug ("Duplex %d is not valid. 0-half, 1-full .\n", dup);
            return (0);
    }

    switch (spd) {
        case SPEED_10M:
            val += SYSPLD_PORT_SPEED_10M;
            break;
        case SPEED_100M:
            val += SYSPLD_PORT_SPEED_100M;
            break;
        case SPEED_1G:
            val += SYSPLD_PORT_SPEED_1G;
            break;
        default:
            debug ("Speed %d is not valid.   0-10M, 1-100M, 2-1G.\n", spd);
            return (0);
    }

    if (syspld_reg_read(reg, &tmp)) {
	debug ("Unable to read SYSPLD register address 0x%x.\n", reg);
        return (0);
    }
    
    /* adjust for port number */
    bitShift = NUM_BITS_PER_PORT_LED *
            (port % (NUM_BITS_PER_PORT_LED_REG / NUM_BITS_PER_PORT_LED));

    tmp &= (~(PORT_LED_BIT_MASK << bitShift));
    tmp |= (val << bitShift);
    
    return (syspld_reg_write(reg, tmp));
}

int 
set_uplink_led (int port, UplinkLink lnk, UplinkSpeed spd)
{
    int reg_link, reg_speed, bitShift;
    uint8_t tmp_link, tmp_speed, val_link = 0, val_speed= 0;
    
    reg_link = SYSPLD_UPLINK_LINKACT_LED;
    reg_speed = SYSPLD_UPLINK_SPEED_LED;

    if (port > NUM_UPLINK_PORTS) {
        debug ("Uplink port number %d is out of range 0-%d.\n", 
               port, NUM_UPLINK_PORTS);
        return (0);
    }

    switch (lnk) {
        case UPLINK_NORMAL:
            val_link = SYSPLD_PORT_0_CHJ2;
            break;
        case UPLINK_FORCE_OFF:
            val_link = SYSPLD_PORT_0_FORCE_OFF;
            break;
        case UPLINK_FORCE_ON:
            val_link = SYSPLD_PORT_0_FORCE_ON;
            break;
        case UPLINK_FORCE_BLINK:
            val_link = SYSPLD_PORT_0_FORCE_BLINK;
            break;
        default:
            debug ("Link state %d is not valid.   "
                   "0-normal, 1-off, 2-on, 3-blink.\n", lnk);
            return (0);
    }
    
    switch (spd) {
        case UPLINK_SPEED_1G:
            val_speed = SYSPLD_PORT_SPEED_10M;
            break;
        case UPLINK_SPEED_10G:
            val_speed = SYSPLD_PORT_SPEED_100M;
            break;
        default:
            debug ("Speed %d is not valid.   0-1G, 1-10G.\n", spd);
            return (0);
    }

    if (syspld_reg_read(reg_speed, &tmp_speed)) {
            debug ("Unable to read SYSPLD register address 0x%x.\n", 
                   reg_speed);
        return (0);
    }
    
    if (syspld_reg_read(reg_link, &tmp_link)) {
            debug ("Unable to read SYSPLD register address 0x%x.\n", 
                   reg_link);
        return (0);
    }
    
    /* adjust for port number */
    bitShift = port * NUM_BITS_PER_UPLINK_PORT_LED;

    tmp_speed &= (~(UPLINK_LED_BIT_MASK << bitShift));
    tmp_speed |= (tmp_speed << bitShift);
    
    tmp_link &= (~(UPLINK_LED_BIT_MASK << bitShift));
    tmp_link |= (tmp_link << bitShift);
    
    if (syspld_reg_write(reg_speed, tmp_speed)) {
        debug ("Unable to write SYSPLD register address 0x%x.\n", 
               reg_speed);
        return (0);
    }
    if (syspld_reg_write(reg_speed, tmp_link)) {
        debug ("Unable to write SYSPLD register address 0x%x.\n", 
               tmp_link);
        return (0);
    }
    return (1);
}

int 
set_mgmt_led (PortLink lnk, PortDuplex dup, PortSpeed spd)
{
    int reg, bitShift;
    uint8_t tmp, val = 0;
    
    reg = SYSPLD_SYS_LED;

    switch (lnk) {
        case LINK_UP:
            val += SYSPLD_PORT_LINK_UP;
            break;
        case LINK_DOWN:
            val += SYSPLD_PORT_NO_LINK;
            break;
        default:
            debug("Link state %d is not valid. 0-link down, "
                  "1-link up.\n", lnk);
            return (0);
    }
    
    switch (dup) {
        case HALF_DUPLEX:
            val += SYSPLD_PORT_HALF_DUPLEX;
            break;
        case FULL_DUPLEX:
            val += SYSPLD_PORT_FULL_DUPLEX;
            break;
        default:
            debug ("Duplex %d is not valid. 0-half, 1-full.\n", dup);
            return (0);
    }

    switch (spd) {
        case SPEED_10M:
            val += SYSPLD_PORT_SPEED_10M;
            break;
        case SPEED_100M:
            val += SYSPLD_PORT_SPEED_100M;
            break;
        case SPEED_1G:
            val += SYSPLD_PORT_SPEED_1G;
            break;
        default:
            debug ("Speed %d is not valid.   0-10M, 1-100M, 2-1G.\n", 
                   spd);
            return (0);
    }

    if (syspld_reg_read(reg, &tmp)) {
        debug ("Unable to read SYSPLD register address 0x%x.\n", reg);
        return (0);
    }
    
    /* adjust for port number */
    bitShift = 4;

    tmp &= (~(PORT_LED_BIT_MASK << bitShift));
    tmp |= (val << bitShift);
    
    return syspld_reg_write(reg, tmp);
}

void 
led_init (void)
{
    memset (&ledStore, 0, NUM_INTERVALS * sizeof (uint8_t));
    lastLedData = 0xc0;
    ledLastInterval = 0;
}

void
setup_sys_blink(HwBlinkState state)
{
    uint8_t tmp;
    int reg = SYSPLD_SYS_LED;

    if (syspld_reg_read(reg, &tmp)) {
        debug ("Unable to read SYSPLD register address 0x%x.\n", reg);
    }
    if (state == HW_BLINK_ON)
        tmp |= (SYSPLD_SYS_GREEN_BLINK);
    else if (state == HW_BLINK_OFF)
        tmp &= ~(SYSPLD_SYS_GREEN_BLINK);
    syspld_reg_write(reg, tmp);
}

int 
set_led (SysLed led, LedColor color, LedState state)
{
    int i = 0;

    if (led >= LAST_LED) {
        
        debug ("LED %d is not valid.   0-Alm, 1-Sys, 2-Mstr.\n", led);
        return (0);
    } 
    
    if ((color  < LED_OFF) || (color > LED_AMBER)) {
        debug("LED color %d is not valid. 0-Off, 1-Red, 2-Green, 3-Amber.\n",
              color);
        return (0);
    }

    if ((state  < LED_STEADY) || (state  > LED_HW_BLINKING)) {
        debug ("LED state %d is not valid. 0-Steady, 1-Slow blinking, 2-Fast"
               " blinking, 3-Hardware blinking.\n", state);
        return (0);
    }

    switch (state) {
        case LED_STEADY:
            if (led == SYS_LED)
                setup_sys_blink(HW_BLINK_OFF);
            for (i = 0; i < NUM_INTERVALS; i++)
                ledStore[i] = (ledStore[i] & ~led_bit_mask[led]) | 
                        led_bit_pattern[color][led];
            break;
        case LED_SLOW_BLINKING:
            if (led == SYS_LED)
                setup_sys_blink(HW_BLINK_OFF);
            for (i = 0; i < NUM_INTERVALS; i++) {
                if (i < (NUM_INTERVALS / 2))
                    ledStore[i] = (ledStore[i] & ~led_bit_mask[led]) | 
                            led_bit_pattern[color][led];
                else
                    ledStore[i] = ledStore[i] & ~led_bit_mask[led];
            }
            break;
        case LED_FAST_BLINKING:
            if (led == SYS_LED)
                setup_sys_blink(HW_BLINK_OFF);
            for (i = 0; i < NUM_INTERVALS; i++) {
                if (i  % 2)
                    ledStore[i] = (ledStore[i] & ~led_bit_mask[led]) | 
                            led_bit_pattern[color][led];
                else
                    ledStore[i] = ledStore[i] & ~led_bit_mask[led];
            }
            break;
        case LED_HW_BLINKING:
            if (led == SYS_LED)
                setup_sys_blink(HW_BLINK_ON);
            for (i = 0; i < NUM_INTERVALS; i++)
                ledStore[i] = ledStore[i] & ~led_bit_mask[led];
            break;
    }
    return (1);
}

void 
update_led (void)
{
    int currInt;
    
    int reg;
    uint8_t tmp, val = 0;

    reg = SYSPLD_LCD_MOD_LED;

    currInt = ledLastInterval + 1;
    if (currInt >= NUM_INTERVALS)
        currInt = 0;

    if (lastLedData != ledStore[currInt]) {
        if (syspld_reg_read(reg, &tmp)) {
            debug ("Unable to read SYSPLD register address 0x%x.\n", reg);
            return (0);
        }

        tmp &= ~LCD_LED_BIT_MASK;
        tmp |= ledStore[currInt];

        syspld_reg_write(reg, tmp);
        lastLedData = ledStore[currInt];
    }

    ledLastInterval = currInt;
    return;
}

