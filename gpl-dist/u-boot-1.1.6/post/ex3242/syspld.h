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

#ifndef __SYSPLDLED__
#define __SYSPLDLED__

typedef enum ledColor 
{
    LED_OFF,
    LED_AMBER, 
    LED_GREEN,
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
	DUMMY_LED,
	FAN_LED,
	UPLINK_LED,
	POE_LED,
	PWR_LED,
	RPS_LED,
	STACK_LED,
	SYS_LED,
	LAST_LED
}SysLed;

typedef enum networkMode{
	MODE_1G,
	MODE_10G
}NetworkMode;

typedef enum lcdBacklight 
{
	LCD_BACKLIGHT_OFF,
	LCD_BACKLIGHT_ON
} LcdBacklight;

char network[][10]={{"MODE_1G"},
	{"MODE_10G"}};

struct sys_led_info
{
	SysLed led;
	LedColor color;
	LedState state;
};

char  link_status[][10]={{"LINK_DOWN"},
	{"LINK_UP"}};

char  duplex_mode[][20]={{"HALF_DUPLEX"},
	{"FULL_DUPLEX"}};

char  port_speed[][20]={{"SPEED_10M"},
	{"SPEED_100M"},
	{"SPEED_1G"},
	{"SPEED_10G"}};


extern int set_port_led(int port, PortLink lnk, PortDuplex dup, PortSpeed spd);
extern void led_init (void);
extern int set_led(SysLed led, LedColor color, LedState state);
extern void update_led(void);
extern int set_network_mode(NetworkMode mode);


extern void lcd_init (void);
extern void lcd_clear(void);
extern int lcd_heartbeat(void);
extern int lcd_heartbeat_syspld(void);
extern int lcd_backlight_control(LcdBacklight state);
extern void lcd_cmd(uint8_t cmd);
extern int heart_beat(void);

#endif /*__SYSPLDLED__*/
