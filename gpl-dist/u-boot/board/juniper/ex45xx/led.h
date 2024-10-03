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

#ifndef __LED__
#define __LED__

/* port LEDs */
#define NUM_BITS_PER_PORT_LED	    4
#define NUM_BITS_PER_PORT_LED_REG   16
#define PORT_LED_BIT_MASK	    0xF

/* mgmt port LEDs */
#define MGMT_PORT_SHIFT	    12

/* Other LEDs */
#define NUM_BITS_PER_REG	8
#define NUM_LEDS_PER_BYTE	4
#define NUM_INTERVALS		10  /* 1 sec / 100ms */
#define LED_I2C_REG		{0x08, 0x09}
#define LED_I2C_REG_MASK	{0x03, 0x0}
#define LED_I2C_REG_BACK	{0x0b}

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
    LED_FAST_BLINKING
} LedState;

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

typedef enum sysLed{
    DUMMY_LED_0,
    BOTTOM_LED,
    UNUSED_LED,
    MIDDLE_LED,
    TOP_LED,
    DUMMY_LED_1,
    DUMMY_LED_2,
    DUMMY_LED_3,
    LAST_LED
}SysLed;

typedef enum sysLedBack{
    BOTTOM_LED_BACK,
    MIDDLE_LED_BACK,
    TOP_LED_BACK,
    DUMMY_LED_BACK,
    LAST_LED_BACK
}SysLedBack;

extern void port_led_init (void);
extern int set_port_led(int port, PortLink lnk, PortDuplex dup, PortSpeed spd);
extern void led_init (void);
extern void led_init_back (void);
extern int set_led(SysLed led, LedColor color, LedState state);
extern int set_led_back(SysLedBack led, LedColor color, LedState state);
extern void update_led(void);
extern void update_led_back(void);

#endif /*__LED__*/
