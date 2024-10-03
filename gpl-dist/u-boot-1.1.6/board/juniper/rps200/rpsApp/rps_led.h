/*
 * Copyright (c) 2008-2010, Juniper Networks, Inc.
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

#ifndef __RPS_LED__
#define __RPS_LED__

typedef enum ledColor 
{
    LED_OFF = 0x0,
    LED_AMBER_BLINKING = 0x1,
    LED_AMBER = 0x2,
    LED_GREEN_BLINKING = 0x4,
    LED_GREEN = 0x8, 
} LedColor;

extern int32_t set_rps_led(LedColor color);
extern int32_t set_fanfail_led(LedColor color);
extern int32_t set_port_led(int port, LedColor color);
extern LedColor get_rps_led(void);
extern char * get_rps_led_state(void);
extern LedColor get_fanfail_led(void);
extern char * get_fanfail_led_state(void);
extern LedColor get_port_led(int32_t port);
extern char * get_port_led_state(int32_t port);

#endif /*__RPS_LED__*/
