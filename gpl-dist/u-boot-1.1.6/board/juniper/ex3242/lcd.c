/*
 * Copyright (c) 2007-2013, Juniper Networks, Inc.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include "epld.h"
#include "lcd.h"
#include <stdarg.h>

#undef debug
#ifdef	EX3242_DEBUG
#define debug(fmt,args...)	printf (fmt ,##args)
#else
#define debug(fmt,args...)
#endif

extern int board_id(void);

#define EPLD_LCD_4BIT_MODE         1

typedef enum lcdDataType 
{
    LCD_COMMAND,
    LCD_DATA
} LcdDataType;

#define LCD_BUSY_MAX             100 

#define EPLD_LCD_DATA_REG              EPLD_LCD_DATA
#define EPLD_LCD_CONTROL_REG        EPLD_MISC_CONTROL
#define EPLD_LCD_BACKLIGHT_MASK  0xFFF7

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
#define LCD_MODE_4BITS              0x28  /* 4 bits interface, 2 lines, 5x7 font */
#define LCD_MODE_4B_EXT             0x2c  /* 4bits, 2 line and extension bit */
#define LCD_MODE_DEFAULT              0x38  /* 8 bits interface, 2 lines, 5x7 font */
#define LCD_COMSEG_NOREVERSE        0x40  /* Set no reverse bit */
#define LCD_DDRAM_ADDR                  0x80

#define LCD_BUSY    0x80

#define LCD_LINE0_ADDR 0x00
#define LCD_LINE1_ADDR 0x40

static char board_name [][LCD_LINE_LENGTH+1] = 
 { "EX3200-24T      ",
    "EX3200-24POE    ",
    "EX3200-48T      ",
    "EX3200-48POE",
    "EX4200-24F      ",
    "EX4200-24T      ",
    "EX4200-24POE    ",
    "EX4200-48T      ",
    "EX4200-48POE    ",
    "EX4200-24PX     ",
    "EX4200-48PX     "
};

static int board_index = 1;

static char lcd_data[2][LCD_LINE_LENGTH+1] =
               {"Juniper JAVA    ",
                "U-boot...       "};

#define MAX_STATE 3
static int curline;
static int curpos;
static int lcdReady = 0;
int d_count, o_count = 0;
int lcd_gone = 0;

static inline int lcd_wait_till_not_busy(uint16_t retMode)
{
    uint16_t temp, tmp;
    int loop_count = 0;
#if (EPLD_LCD_4BIT_MODE)
    uint16_t dummy;
#endif

    if (!lcdReady)
        return 0;

    if (!epld_reg_read(EPLD_LCD_CONTROL_REG, &temp)) {
	 debug ("Unable to read EPLD register address 0x%x.\n", EPLD_LCD_CONTROL_REG*2);
        return 0;
    }

    if (EPLD_LCD_INST_READ != (temp & ~EPLD_LCD_RW_RS_BIT_MASK)) {
        temp = (temp & EPLD_LCD_RW_RS_BIT_MASK) | EPLD_LCD_INST_READ;
        if (!epld_reg_write(EPLD_LCD_CONTROL_REG, temp)) {
	     debug ("Unable to write EPLD register address 0x%x[0x%x].\n", EPLD_LCD_CONTROL_REG*2, temp);
            return 0;
        }
    }

    while (epld_reg_read(EPLD_LCD_DATA_REG, &tmp)) {
#if (EPLD_LCD_4BIT_MODE)
        if (!epld_reg_write(EPLD_LCD_CONTROL_REG, temp)) {
	     debug ("Unable to write EPLD register address 0x%x[0x%x].\n", EPLD_LCD_CONTROL_REG*2, temp);
            return 0;
        }
	 epld_reg_read(EPLD_LCD_DATA_REG, &dummy);
#endif
        loop_count++;
        if (loop_count > LCD_BUSY_MAX) {
	     lcd_gone = 1;
            return 0;
        }

        if (0 == (tmp & LCD_BUSY)) {
            temp = (temp & EPLD_LCD_RW_RS_BIT_MASK) | retMode;
            if (!epld_reg_write(EPLD_LCD_CONTROL_REG, temp)) {
	         debug ("Unable to write EPLD register address 0x%x[0x%x].\n", EPLD_LCD_CONTROL_REG*2, temp);
                return 0;
            }
            return 1;
        }
        if (!epld_reg_write(EPLD_LCD_CONTROL_REG, temp)) {
	     debug ("Unable to write EPLD register address 0x%x[0x%x].\n", EPLD_LCD_CONTROL_REG*2, temp);
            return 0;
        }
    }
    
    return 0;
}

static int lcd_write(LcdDataType dtype, uint8_t data)
{
    
    if (lcd_wait_till_not_busy(dtype ? EPLD_LCD_DATA_WRITE : EPLD_LCD_INST_WRITE)) {
#if (EPLD_LCD_4BIT_MODE)
        if (!epld_reg_write(EPLD_LCD_DATA_REG, data & 0xf0)) {
	     debug ("Unable to write EPLD register address 0x%x[0x%x].\n", EPLD_LCD_CONTROL_REG*2, data);
            return 0;
        }
        if (!epld_reg_write(EPLD_LCD_DATA_REG, (data & 0x0f) << 4)) {
	     debug ("Unable to write EPLD register address 0x%x[0x%x].\n", EPLD_LCD_CONTROL_REG*2, data);
            return 0;
        }
#else
        if (!epld_reg_write(EPLD_LCD_DATA_REG, data)) {
	     debug ("Unable to write EPLD register address 0x%x[0x%x].\n", EPLD_LCD_CONTROL_REG*2, data);
            return 0;
        }
#endif
        return 1;
    }

    return 0;
}

static int lcd_write_nochk(LcdDataType dtype, uint8_t data)
{
    
#if (EPLD_LCD_4BIT_MODE)
    if (!epld_reg_write(EPLD_LCD_DATA_REG, data & 0xf0)) {
	 debug ("Unable to write EPLD register address 0x%x[0x%x].\n", EPLD_LCD_CONTROL_REG*2, data);
            return 0;
    }
    if (!epld_reg_write(EPLD_LCD_DATA_REG, (data & 0x0f) << 4)) {
	 debug ("Unable to write EPLD register address 0x%x[0x%x].\n", EPLD_LCD_CONTROL_REG*2, data);
        return 0;
    }
#else
    if (!epld_reg_write(EPLD_LCD_DATA_REG, data)) {
	 debug ("Unable to write EPLD register address 0x%x[0x%x].\n", EPLD_LCD_CONTROL_REG*2, data);
        return 0;
    }
#endif
    return 1;
}

#if (EPLD_LCD_4BIT_MODE)
static int lcd_write_once(LcdDataType dtype, uint8_t data)
{
    
    if (!epld_reg_write(EPLD_LCD_DATA_REG, data & 0xf0)) {
	 debug ("Unable to write EPLD register address 0x%x[0x%x].\n", EPLD_LCD_CONTROL_REG*2, data);
        return 0;
    }
    return 1;
}
#endif

static int lcd_display(int addr, char *data)
{
    int i, len = LCD_LINE_LENGTH;

    if (addr == LCD_LINE0_ADDR)
        len--;
    
    if (!lcd_write(LCD_COMMAND, LCD_DDRAM_ADDR + addr))
        return 0;
    
    for (i = 0; (i < len) && (*data != '\0'); i++) {
        if (!lcd_write(LCD_DATA, *data++))
            return 0;
    }
    return 1;
}

void lcd_init (void)
{
    uint16_t tmp = 0;
    int bid = board_id();

    if (!epld_reg_read(EPLD_LCD_CONTROL_REG, &tmp)) {
	 debug ("Unable to read EPLD register address 0x%x.\n", EPLD_LCD_CONTROL_REG*2);
        return;
    }
    
    tmp &= EPLD_LCD_RW_RS_BIT_MASK;
    tmp |= (EPLD_LCD_INST_WRITE | EPLD_LCD_BACKLIGHT_ON);
    if (!epld_reg_write(EPLD_LCD_CONTROL_REG, tmp)) {
	 debug ("Unable to write EPLD register address 0x%x[0x%x].\n", EPLD_LCD_CONTROL_REG*2, tmp);
        return;
    }

#if (EPLD_LCD_4BIT_MODE)
#if 0
    /* OPTREX 4 bit interface */
    lcd_write_once(LCD_COMMAND, LCD_MODE_DEFAULT);
    udelay(4500);
    lcd_write_once(LCD_COMMAND, LCD_MODE_DEFAULT);
    udelay(100);
    lcd_write_once(LCD_COMMAND, LCD_MODE_DEFAULT);
    udelay(100);
    lcd_write_once(LCD_COMMAND, LCD_MODE_4BITS);
    lcd_write(LCD_COMMAND, LCD_MODE_4BITS);
    lcd_write(LCD_COMMAND, LCD_CLEAR_DISPLAY);
    lcd_write(LCD_COMMAND, 0x6);
    
    lcd_write(LCD_COMMAND, LCD_DISPLAY_ON);
#endif

    /* ST7070 4 bit interface */
    lcd_write_once(LCD_COMMAND, LCD_MODE_DEFAULT);
    udelay(2000);
    lcd_write_once(LCD_COMMAND, LCD_MODE_DEFAULT);
    udelay(40);
    lcd_write_once(LCD_COMMAND, LCD_MODE_DEFAULT);
    udelay(40);
    lcd_write_nochk(LCD_COMMAND, LCD_MODE_4BITS);
    udelay(40);
    lcd_write_nochk(LCD_COMMAND, LCD_MODE_4BITS);
    udelay(40);
    lcd_write(LCD_COMMAND, LCD_DISPLAY_ON);
    udelay(40);

    /*
     * Because LCD characters on the LCD display sometimes
     * were seen invereted, explictly set the direction bit.
     */ 

    /* 
     * function set, 4-bit,  2-line display,
     * enable extension inst
     */
    lcd_write(LCD_COMMAND, LCD_MODE_4B_EXT);
    udelay(40);

    /* COM SEG direction select . no reverse */
    lcd_write(LCD_COMMAND, LCD_COMSEG_NOREVERSE);
    udelay(40);

    lcd_write_nochk(LCD_COMMAND, LCD_MODE_4BITS);
    udelay(40);

    /*
     * Need this setting, as we shifted to
     * ext mode, the normal writes are with
     * ext mode set to 0
     */
    lcd_write(LCD_COMMAND, LCD_DISPLAY_ON);
    udelay(40);


    lcd_write(LCD_COMMAND, LCD_CLEAR_DISPLAY);
    udelay(1600);

    lcd_write(LCD_COMMAND, 0x6);
    udelay(40);
#else
    /* OPTREX 8 bit interface */
    lcd_write(LCD_COMMAND, LCD_MODE_DEFAULT);
    udelay(5000);
    lcd_write(LCD_COMMAND, LCD_MODE_DEFAULT);
    udelay(200);
    lcd_write(LCD_COMMAND, LCD_MODE_DEFAULT);
    lcd_write(LCD_COMMAND, LCD_MODE_DEFAULT);
    lcd_write(LCD_COMMAND, LCD_DISPLAY_OFF);
    lcd_write(LCD_COMMAND, LCD_CLEAR_DISPLAY);
    lcd_write(LCD_COMMAND, 0x6);

    lcd_write(LCD_COMMAND, LCD_DISPLAY_ON);
#endif

    if (lcd_gone == 0)  /* kludge to detect LCD functional */
        lcdReady = 1;  /* use for manual control of LCD enable */

    if ((bid > I2C_ID_JUNIPER_EX4200_48_P) || (bid < I2C_ID_JUNIPER_EX3200_24_T))
        board_index = 1;
    else
        board_index = bid -I2C_ID_JUNIPER_EX3200_24_T;
    
    if (bid == I2C_ID_JUNIPER_EX4210_24_P)
        board_index = 9;
    if (bid == I2C_ID_JUNIPER_EX4210_48_P)
        board_index = 10;

    memcpy( lcd_data[0], board_name[board_index], LCD_LINE_LENGTH-1);

    lcd_data[0][LCD_LINE_LENGTH] = 0;
    lcd_data[1][LCD_LINE_LENGTH] = 0;

    curline = curpos = 0;

    lcd_display(LCD_LINE0_ADDR, lcd_data[0]);
    lcd_display(LCD_LINE1_ADDR, lcd_data[1]);
}


void lcd_off (void)
{
    lcd_clear();
    lcd_backlight_control(LCD_BACKLIGHT_OFF);
    curline = curpos = 0;
    lcdReady = 0;  /* use for manual control of LCD disable */
}

void lcd_clear(void)
{
    if (!lcdReady)
        return;
    
    lcd_write(LCD_COMMAND, LCD_CLEAR_DISPLAY);
}


void lcd_write_char(const char c)
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
    if (curpos < len)
	lcd_data[curline][curpos++] = c;
}

void lcd_write_string(const char *s)
{
    char *p;

    for (p = (char *)s; *p != '\0'; p++)
	lcd_write_char(*p);
}

void lcd_printf(const char *fmt, ...)
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

void lcd_printf_2(const char *fmt, ...)
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


int  lcd_heartbeat(void)
{
    static char rotate[] = {'|','/','-','\315'};
    static int index = 0;

    if (!lcdReady)
        return 0;
    
    lcd_write(LCD_COMMAND, LCD_DDRAM_ADDR + LCD_LINE0_ADDR + LCD_LINE_LENGTH - 1);
    lcd_write(LCD_DATA, rotate[index]);

    if (++index >= (sizeof rotate / sizeof rotate[0]))
        index = 0;
    
    if (lcd_gone)  /* kludge to detect LCD functional */
        lcdReady = 0;
    
    return 1;
}

int lcd_backlight_control(LcdBacklight state)
{
    uint16_t tmp = 0;

    if (!lcdReady)
        return 0;
    
    if ((state < LCD_BACKLIGHT_OFF) || (state > LCD_BACKLIGHT_ON)) {
        debug ("LCD back light state %d is out of range.  0-off, 1-on.\n");
        return 0;
    }

    if (!epld_reg_read(EPLD_LCD_CONTROL_REG, &tmp)) {
	 debug ("Unable to read EPLD register address 0x%x.\n", EPLD_LCD_CONTROL_REG*2);
        return 0;
    }

    tmp &= EPLD_LCD_BACKLIGHT_MASK;
    tmp |= (state ? EPLD_LCD_BACKLIGHT_ON : 0);
    if (!epld_reg_write(EPLD_LCD_CONTROL_REG, tmp)) {
	 debug ("Unable to write EPLD register address 0x%x[0x%x].\n", EPLD_LCD_CONTROL_REG*2, tmp);
        return 0;
    }
    
    return 1;
}

void lcd_cmd (uint8_t cmd)
{
    lcd_write(LCD_COMMAND, cmd);
}

static int lcd_read(LcdDataType dtype, uint8_t* data)
{
    uint16_t temp;

    if (lcd_wait_till_not_busy(dtype ? EPLD_LCD_DATA_READ : EPLD_LCD_INST_READ)) {
        if (!epld_reg_read(EPLD_LCD_DATA_REG, &temp)) {
	     debug ("Unable to read EPLD register address 0x%x[0x%x].\n", EPLD_LCD_CONTROL_REG*2, *data);
            return 0;
        }
#if (EPLD_LCD_4BIT_MODE)
        uint16_t temp1;

        if (!epld_reg_read(EPLD_LCD_DATA_REG, &temp1)) {
	     debug ("Unable to read EPLD register address 0x%x[0x%x].\n", EPLD_LCD_CONTROL_REG*2, *data);
            return 0;
        }
	 *data = ((temp & 0xf0) | ((temp1 & 0xf0) >> 4)) & EPLD_LCD_DATA_MASK;
#else
        *data = temp & EPLD_LCD_DATA_MASK;
#endif
        return 1;
    }

    return 0;
}

void lcd_dump(void)
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

