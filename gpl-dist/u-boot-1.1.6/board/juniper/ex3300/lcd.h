/*
 * Copyright (c) 2010, Juniper Networks, Inc.
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

#ifndef __LCD__
#define __LCD__

#define LCD_FRONT   0
#define LCD_BACK    1

#define LCD_BUSY_MAX                    100

#define SYSPLD_LCD_BACKLIGHT_MASK       0xFB

#define SYSPLD_LCD_RW_RS_BIT_MASK       0xFC
#define SYSPLD_LCD_INST_WRITE           0x00
#define SYSPLD_LCD_INST_READ            0x01
#define SYSPLD_LCD_DATA_WRITE           0x02
#define SYSPLD_LCD_DATA_READ            0x03
#define SYSPLD_LCD_DATA_MASK            0xFF

/* LCD HW */
#define LCD_LINE_0          0
#define LCD_LINE_1          1
#define LCD_LINE_LENGTH     16

/* LCD commands */
#define LCD_CLEAR_DISPLAY           0x01
#define LCD_RETURN_HOME             0x02
#define LCD_MOVE_CURSOR_LEFT_RW     0x04  /* during data read/write */
#define LCD_MOVE_CURSOR_RIGHT_RW    0x06  /* during data read/write */
#define LCD_SHIFT_CURSOR_LEFT_RW    0x05  /* during data read/write */
#define LCD_SHIFT_CURSOR_RIGHT_RW   0x07  /* during data read/write */
#define LCD_DISPLAY_OFF             0x08
#define LCD_DISPLAY_ON              0x0C
#define LCD_BLINKING_CURSOR         0x09
#define LCD_CURSON_ON               0x0A
#define LCD_MOVE_CURSOR_LEFT        0x10
#define LCD_MOVE_CURSOR_RIGHT       0x18
#define LCD_SHIFT_DISPLAY_LEFT      0x14
#define LCD_SHIFT_DISPLAY_RIGHT     0x1C
#define LCD_MODE_8BITS              0x30  /* 8 bits interface */
#define LCD_MODE_4BITS              0x28  /* 4 bits intf, 2 lines, 5x7 font */
#define LCD_MODE_DEFAULT            0x38  /* 8 bits intf, 2 lines, 5x7 font */
#define LCD_DDRAM_ADDR              0x80

#define LCD_BUSY        0x80

#define LCD_LINE0_ADDR  0x00
#define LCD_LINE1_ADDR  0x40

#define MAX_STATE       3

typedef enum lcdBacklight 
{
    LCD_BACKLIGHT_OFF,
    LCD_BACKLIGHT_ON
} LcdBacklight;

typedef enum lcdDataType 
{
    LCD_COMMAND,
    LCD_DATA
} LcdDataType;

extern void lcd_init (void);
extern void lcd_off(void);
extern void lcd_clear(void);
extern int lcd_heartbeat(void);
extern void lcd_printf(const char *fmt, ...);
extern void lcd_printf_2(const char *fmt, ...);
extern int lcd_backlight_control(LcdBacklight state);
extern void lcd_cmd (uint8_t cmd);
extern void lcd_dump(void);
extern void lcd_position (int line, int pos);
extern void lcd_write_char_pos (int line, int pos, uint8_t data);
extern uint8_t lcd_read_char_pos (int line, int pos);
extern void lcd_default_display(void);

extern int board_idtype(void);

#endif /*__LCD__*/
