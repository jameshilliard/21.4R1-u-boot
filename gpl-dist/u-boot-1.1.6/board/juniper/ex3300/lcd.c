/*
 * Copyright (c) 2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * most of this is porting from the file board\cogent\lcd.c
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
#include "lcd.h"
#include <stdarg.h>

#undef debug
#ifdef	EX3300_DEBUG
#define debug(fmt,args...)	printf (fmt ,##args)
#else
#define debug(fmt,args...)
#endif

int lcd_debug = 0;

static char board_name [10][LCD_LINE_LENGTH+1] = 
{
    "EX3300-48P      ",
    "EX3300-48T      ",
    "EX3300-24P      ",
    "EX3300-24T      ",
    "EX3300-48T-BF   ",
    "EX3300-24T-DC   ",
};

static int board_index = 0;

static char lcd_data[2][LCD_LINE_LENGTH+1] =
               {"Juniper JAVA    ",
                "U-boot...       "};

static int curline;
static int curpos;
static int lcdReady = 0;
static uint8_t lcd_backlight = 0;
int d_count, o_count = 0;
int lcd_gone = 0;

int
lcd_reg_read (int reg, uint8_t *data)
{
    int res = 0;
    uint8_t temp;

    if ((res = syspld_reg_read(reg, &temp)) == 0) {
        *data = temp;
        if (lcd_debug)
            printf("LCD read reg = %02x, data = %02x\n", reg, temp);
    }
    udelay(2);
    
    return res;
}

int
lcd_reg_write (int reg, uint8_t data)
{
    int res = 0;

    if (lcd_debug)
        printf("LCD write reg = %02x, data = %02x\n", reg, data);
    res = syspld_reg_write(reg, data);
    udelay(40);
    return res;
}

static inline 
int 
lcd_wait_till_not_busy (uint8_t retMode)
{
    uint8_t temp;
    uint8_t dummy;
    int i = 0;

    if (!lcdReady)
        return 0;
    
    if (lcd_debug)
        printf("LCD check.\n");
    
    lcd_reg_write(SYSPLD_LCD_CONTROL, SYSPLD_LCD_INST_READ | lcd_backlight);

    for (i = 0; i < LCD_BUSY_MAX; i++) {
        lcd_reg_read(SYSPLD_LCD_DATA, &temp);
        /* high nibble read */
        lcd_reg_read(SYSPLD_LCD_CONTROL, &temp);
        lcd_reg_read(SYSPLD_LCD_DATA, &dummy);
        /* low nibble read */
        lcd_reg_read(SYSPLD_LCD_CONTROL, &dummy);
        
        if (0 == (temp & LCD_BUSY)) {
            lcd_reg_write(SYSPLD_LCD_CONTROL, retMode | lcd_backlight);
            if (lcd_debug)
                printf("LCD ready.\n");
            return 1;
        }
    }
    lcd_gone = 1;
    
    return 0;
}

static int 
lcd_write (LcdDataType dtype, uint8_t data)
{
    
    if (lcd_wait_till_not_busy(dtype ? SYSPLD_LCD_DATA_WRITE : 
        SYSPLD_LCD_INST_WRITE)) {
        if (lcd_debug)
            printf("lcd_write data = %02x\n", data); 
        lcd_reg_write(SYSPLD_LCD_DATA, data & 0xf0);
        lcd_reg_write(SYSPLD_LCD_DATA, (data & 0x0f) << 4);
        return 1;
    }

    return 0;
}

static void 
lcd_write_nochk (LcdDataType dtype, uint8_t data)
{
    lcd_reg_write(SYSPLD_LCD_DATA, data & 0xf0);
    lcd_reg_write(SYSPLD_LCD_DATA, (data & 0x0f) << 4);
}

static void 
lcd_write_once (LcdDataType dtype, uint8_t data)
{
    lcd_reg_write(SYSPLD_LCD_DATA, data & 0xf0);
}

static int 
lcd_display (int addr, char *data)
{
    int i, len = LCD_LINE_LENGTH;

    if (addr == LCD_LINE0_ADDR)
        len--;
    
    if (lcd_write(LCD_COMMAND, LCD_DDRAM_ADDR + addr) == 0)
        return 0;
    
    for (i = 0; (i < len) && (*data != '\0'); i++) {
        if (lcd_write(LCD_DATA, *data++) == 0)
            return 0;
    }
    return 1;
}

void 
lcd_init (void)
{
    int bid = board_idtype();

    if (lcd_gone == 0)  /* kludge to detect LCD functional */
        lcdReady = 1;  /* use for manual control of LCD enable */

    lcd_backlight = SYSPLD_LCD_BACKLIGHT_ON;
    lcd_reg_write(SYSPLD_LCD_CONTROL, SYSPLD_LCD_INST_WRITE |lcd_backlight);

    /* ST7070 4 bit interface */
    lcd_write_once(LCD_COMMAND, LCD_MODE_DEFAULT);
    udelay(2000);
    lcd_write_once(LCD_COMMAND, LCD_MODE_DEFAULT);
    udelay(40);

    lcd_write_once(LCD_COMMAND, LCD_MODE_4BITS);
    udelay(40);
    lcd_write_nochk(LCD_COMMAND, LCD_MODE_4BITS);
    udelay(40);
    lcd_write(LCD_COMMAND, LCD_DISPLAY_ON);
    udelay(40);
    lcd_write(LCD_COMMAND, LCD_CLEAR_DISPLAY);
    udelay(1600);

    lcd_write(LCD_COMMAND, 0x6);
    udelay(40);

    if ((bid >= I2C_ID_JUNIPER_EX3300_48_P) || 
        (bid <= I2C_ID_JUNIPER_EX3300_24_T_DC))
        board_index = bid -I2C_ID_JUNIPER_EX3300_48_P;

    memcpy( lcd_data[0], board_name[board_index], LCD_LINE_LENGTH-1);

    lcd_data[0][LCD_LINE_LENGTH] = 0;
    lcd_data[1][LCD_LINE_LENGTH] = 0;

    curline = curpos = 0;

    lcd_display(LCD_LINE0_ADDR, lcd_data[0]);
    lcd_display(LCD_LINE1_ADDR, lcd_data[1]);
}


void 
lcd_off (void)
{
    lcd_clear();
    lcd_backlight_control(LCD_BACKLIGHT_OFF);
    curline = curpos = 0;
    lcdReady = 0;  /* use for manual control of LCD disable */
}

void 
lcd_clear (void)
{
    if (lcdReady)
        lcd_write(LCD_COMMAND, LCD_CLEAR_DISPLAY);
    else {
        lcd_write_once(LCD_COMMAND, LCD_MODE_DEFAULT);
        udelay(2000);
        lcd_write_once(LCD_COMMAND, LCD_MODE_DEFAULT);
        udelay(40);
        lcd_write_once(LCD_COMMAND, LCD_MODE_DEFAULT);
        udelay(40);
        lcd_write_once(LCD_COMMAND, LCD_MODE_4BITS);
        udelay(40);
        lcd_write_nochk(LCD_COMMAND, LCD_MODE_4BITS);
        udelay(40);
        lcd_write(LCD_COMMAND, LCD_DISPLAY_ON);
        udelay(40);
        lcd_write(LCD_COMMAND, LCD_CLEAR_DISPLAY);
        udelay(1600);
    }
}


void 
lcd_write_char (const char c)
{
    int i, len;

    /* ignore CR */
    if (c == '\r')
	return;

    len = LCD_LINE_LENGTH;
    if (curline == 0)
	len--;

    if (c == '\n') {
	lcd_display(LCD_LINE0_ADDR, &lcd_data[curline^1][0]);
	lcd_display(LCD_LINE1_ADDR, &lcd_data[curline][0]);

	/* Do a line feed */
	curline ^= 1;
	len = LCD_LINE_LENGTH;
	if (curline == 0)
	    len--;
	curpos = 0;

	for (i = 0; i < len; i++)
	    lcd_data[curline][i] = ' ';

	return;
    }

    /* Only allow to be output if there is room on the LCD line */
    if (curpos < len) {
        lcd_data[curline][curpos++] = c;
    }
}

void 
lcd_write_string (const char *s)
{
    char *p;

    for (p = (char *)s; *p != '\0'; p++)
	lcd_write_char(*p);
}

void 
lcd_printf (const char *fmt, ...)
{
    va_list args;
    char buf[CFG_PBSIZE];

    if (!lcdReady)
        return;
    
    va_start(args, fmt);
    (void)vsprintf(buf, fmt, args);
    va_end(args);

    lcd_write_string(buf);
}

void 
lcd_printf_2 (const char *fmt, ...)
{
    va_list args;
    char buf[CFG_PBSIZE];

    if (!lcdReady)
        return;

    memset(buf, 0, LCD_LINE_LENGTH);
    
    va_start(args, fmt);
    (void)vsprintf(buf, fmt, args);
    va_end(args);

    buf[LCD_LINE_LENGTH+1] = '\0',

    /* display to second line */
    curline = 1;
    curpos = 0;
    lcd_write_string(buf);
    lcd_display(LCD_LINE0_ADDR, &lcd_data[0][0]);
    lcd_display(LCD_LINE1_ADDR, &lcd_data[1][0]);
}


int  
lcd_heartbeat (void)
{
    static char rotate[] = {'|','/','-','\315'};
    static int index = 0;

    if (!lcdReady)
        return 0;
    
    lcd_write(LCD_COMMAND, 
              LCD_DDRAM_ADDR + LCD_LINE0_ADDR + LCD_LINE_LENGTH - 1);
    lcd_write(LCD_DATA, rotate[index]);

    if (++index >= (sizeof rotate / sizeof rotate[0]))
        index = 0;
    
    if (lcd_gone)  /* kludge to detect LCD functional */
        lcdReady = 0;
    
    return 1;
}

int 
lcd_backlight_control (LcdBacklight state)
{
    uint8_t tmp = 0;

    if ((state < LCD_BACKLIGHT_OFF) || (state > LCD_BACKLIGHT_ON)) {
        debug ("LCD back light state %d is out of range.  0-off, 1-on.\n");
        return 0;
    }

    lcd_reg_read(SYSPLD_LCD_CONTROL, &tmp);

    tmp &= SYSPLD_LCD_BACKLIGHT_MASK;
    if (state) {
        tmp |= SYSPLD_LCD_BACKLIGHT_ON;
        lcd_backlight = SYSPLD_LCD_BACKLIGHT_ON;
    }
    else {
        tmp &= ~SYSPLD_LCD_BACKLIGHT_ON;
        lcd_backlight = 0;
    }

    lcd_reg_write(SYSPLD_LCD_CONTROL, tmp);
    
    return 1;
}

void 
lcd_cmd (uint8_t cmd)
{
    lcd_write(LCD_COMMAND, cmd);
}

static int 
lcd_read (LcdDataType dtype, uint8_t* data)
{
    uint8_t temp, temp1;

    if (lcd_wait_till_not_busy(dtype ? SYSPLD_LCD_DATA_READ : 
        SYSPLD_LCD_INST_READ)) {
        /* high nibble read */
        lcd_reg_read(SYSPLD_LCD_CONTROL, &temp);
        lcd_reg_read(SYSPLD_LCD_DATA, &temp);
        /* low nibble read */
        lcd_reg_read(SYSPLD_LCD_CONTROL, &temp1);
        lcd_reg_read(SYSPLD_LCD_DATA, &temp1);

	 *data = ((temp & 0xf0) | ((temp1 & 0xf0) >> 4));

        return 1;
    }

    return 0;
}

void 
lcd_dump (void)
{
    uint8_t data;
    int i;

    if (!lcdReady)
        return;
    
    printf("LCD DDRAM:\n");
    for (i= 0; i<LCD_LINE_LENGTH; i++) {
        lcd_write(LCD_COMMAND, LCD_DDRAM_ADDR + LCD_LINE0_ADDR + i);
        lcd_read(LCD_DATA, &data);
        printf("%02x ", data);
    }
    printf("\n");
    for (i= 0; i<LCD_LINE_LENGTH; i++) {
        lcd_write(LCD_COMMAND, LCD_DDRAM_ADDR + LCD_LINE1_ADDR + i);
        lcd_read(LCD_DATA, &data);
       printf("%02x ", data);
    }
    printf("\n");
}

void 
lcd_position (int line, int pos)
{
    if (line == 0)
        lcd_write(LCD_COMMAND, LCD_DDRAM_ADDR + LCD_LINE0_ADDR + pos);
    else if (line == 1)
        lcd_write(LCD_COMMAND, LCD_DDRAM_ADDR + LCD_LINE1_ADDR + pos);
}

void 
lcd_write_char_pos (int line, int pos, uint8_t data)
{
    lcd_position(line, pos);
    lcd_write(LCD_DATA, data);
}

uint8_t 
lcd_read_char_pos (int line, int pos)
{
    uint8_t data;
    
    lcd_position(line, pos);
    lcd_read(LCD_DATA, &data);
    return data;
}
