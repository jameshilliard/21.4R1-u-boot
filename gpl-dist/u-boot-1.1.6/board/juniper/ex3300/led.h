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

#ifndef __LED__
#define __LED__

#define NUM_EXT_PORTS           48
#define NUM_UPLINK_PORTS        4
#define NUM_INTERVALS           10  /* 1 sec / 100ms */

/* port LEDs */
#define NUM_BITS_PER_PORT_LED               4
#define NUM_BITS_PER_PORT_LED_REG           8
#define PORT_LED_BIT_MASK                   0xF
#define LCD_LED_BIT_MASK                    0xFC
#define NUM_BITS_PER_UPLINK_PORT_LED        2
#define UPLINK_LED_BIT_MASK                 0x3

typedef enum ledColor 
{
    LED_OFF,
    LED_RED,
    LED_GREEN, 
    LED_AMBER, 
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
    ALM_LED,
    SYS_LED,
    MSTR_LED,
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

typedef enum uplinkLink 
{
    UPLINK_NORMAL,
    UPLINK_FORCE_OFF,
    UPLINK_FORCE_ON,
    UPLINK_FORCE_BLINK,
} UplinkLink;

typedef enum uplinkSpeed 
{
    UPLINK_SPEED_1G,
    UPLINK_SPEED_10G
} UplinkSpeed;

extern int set_port_led(int port, PortLink lnk, PortDuplex dup, PortSpeed spd);
extern int set_uplink_led(int port, UplinkLink lnk, UplinkSpeed spd);
extern int set_mgmt_led(PortLink lnk, PortDuplex dup, PortSpeed spd);
extern void led_init (void);
extern int set_led(SysLed led, LedColor color, LedState state);
extern void update_led(void);

#endif /*__LED__*/
