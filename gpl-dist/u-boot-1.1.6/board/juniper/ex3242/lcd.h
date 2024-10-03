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

#ifndef __LCD__
#define __LCD__

typedef enum lcdBacklight 
{
    LCD_BACKLIGHT_OFF,
    LCD_BACKLIGHT_ON
} LcdBacklight;

extern void lcd_init (void);
extern void lcd_off(void);
extern void lcd_clear(void);
extern int lcd_heartbeat(void);
extern int lcd_heartbeat_syspld(void);
extern void lcd_printf(const char *fmt, ...);
extern void lcd_printf_2(const char *fmt, ...);
extern int lcd_backlight_control(LcdBacklight state);
extern void lcd_cmd(uint8_t cmd);
extern void lcd_dump(void);

#endif /*__LCD__*/
