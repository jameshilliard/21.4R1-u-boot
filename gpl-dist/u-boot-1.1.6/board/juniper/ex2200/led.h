/*
 * Copyright (c) 2008-2011, Juniper Networks, Inc.
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

#ifndef __LED__
#define __LED__

#define NUM_EXT_PORTS        48

typedef enum ledColor 
{
    LED_OFF,
    LED_RED,
    LED_GREEN, 
    LAST_COLOR
} LedColor;

typedef enum ledState 
{
    LED_STEADY,
    LED_SLOW_BLINKING,
    LED_FAST_BLINKING,
    LED_HW_BLINKING
} LedState;

typedef enum sysLed{
    SYS_LED,
    ALM_LED,
    LAST_LED
}SysLed;

typedef enum hwBlinkState
{
   HW_BLINK_ON,
   HW_BLINK_OFF
} HwBlinkState;        

typedef enum portLink 
{
    LINK_DOWN,
    LINK_UP
} PortLink;

typedef enum portDuplex 
{
    HALF_DUPLEX,
    FULL_DUPLEX
} PortDuplex;

typedef enum portSpeed 
{
    SPEED_10M,
    SPEED_100M,
    SPEED_1G,
    SPEED_10G
} PortSpeed;

extern int set_port_led(int port, PortLink lnk, PortDuplex dup, PortSpeed spd);
extern int set_mgmt_led(PortLink lnk, PortDuplex dup, PortSpeed spd);
extern void led_init (void);
extern int set_led(SysLed led, LedColor color, LedState state);
extern void update_led(void);

#endif /*__LED__*/
