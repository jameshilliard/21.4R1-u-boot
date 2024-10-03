/*
 * Copyright (c) 2010-2013, Juniper Networks, Inc.
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

#define LCD_FRONT 0
#define LCD_BACK 1

#define EPLD_LCD_4BIT_MODE         1

#define LCD_BUSY_MAX             100

#define EPLD_LCD_DATA_REG              EPLD_LCD_DATA
#define EPLD_LCD_CONTROL_REG        EPLD_MISC_CONTROL
#define EPLD_LCD_BACKLIGHT_REG    EPLD_MISC_CONTROL_0
#define EPLD_LCD_BACKLIGHT_MASK  0xFFFC

#define EPLD_LCD_RW_RS_BIT_MASK  0xFCFF
#define EPLD_LCD_INST_WRITE           0x0000
#define EPLD_LCD_INST_READ             0x0200
#define EPLD_LCD_DATA_WRITE          0x0100
#define EPLD_LCD_DATA_READ            0x0300
#define EPLD_LCD_DATA_MASK            0x00FF

/* LCD HW */
#define LCD_LINE_0 0
#define LCD_LINE_1 1
#define LCD_LINE_LENGTH 16

/* LCD commands */
#define LCD_CLEAR_DISPLAY            0x01
#define LCD_RETURN_HOME               0x02
#define LCD_MOVE_CURSOR_LEFT_RW     0x04  /* during data read/write */
#define LCD_MOVE_CURSOR_RIGHT_RW   0x06  /* during data read/write */
#define LCD_SHIFT_CURSOR_LEFT_RW    0x05  /* during data read/write */
#define LCD_SHIFT_CURSOR_RIGHT_RW  0x07  /* during data read/write */
#define LCD_DISPLAY_OFF                0x08
#define LCD_DISPLAY_ON                  0x0C
#define LCD_BLINKING_CURSOR        0x09
#define LCD_CURSON_ON                   0x0A
#define LCD_MOVE_CURSOR_LEFT     0x10
#define LCD_MOVE_CURSOR_RIGHT   0x18
#define LCD_SHIFT_DISPLAY_LEFT    0x14
#define LCD_SHIFT_DISPLAY_RIGHT  0x1C
#define LCD_MODE_8BITS              0x30  /* 8 bits interface */
#define LCD_MODE_4BITS              0x28  /* 4 bits intf, 2 lines, 5x7 font */
#define LCD_MODE_4B_EXT             0x2c  /* 4bits, 2 line and extension bit */
#define LCD_MODE_DEFAULT            0x38  /* 8 bits intf, 2 lines, 5x7 font */
#define LCD_COMSEG_NOREVERSE        0x40  /* Set no reverse bit */
#define LCD_DDRAM_ADDR              0x80

#define LCD_BUSY    0x80

#define LCD_LINE0_ADDR 0x00
#define LCD_LINE1_ADDR 0x40

#define MAX_STATE 3

typedef enum lcdBacklight 
{
    LCD_BACKLIGHT_OFF,
    LCD_BACKLIGHT_ON
} LcdBacklight;

extern void lcd_init (int unit);
extern void lcd_off(int unit);
extern void lcd_clear(int unit);
extern int lcd_heartbeat(int unit);
extern void lcd_printf(int unit, const char *fmt, ...);
extern void lcd_printf_2(const char *fmt, ...);
extern int lcd_backlight_control(int unit, LcdBacklight state);
extern void lcd_cmd (int unit, uint8_t cmd);
extern void lcd_dump(int unit);

#endif /*__LCD__*/
