/*
 * Copyright (c) 2011-2013, Juniper Networks, Inc.
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
#ifdef	EX43XX_DEBUG
#define debug(fmt,args...)	printf (fmt ,##args)
#else
#define debug(fmt,args...)
#endif

extern int board_id(void);

typedef enum lcdDataType
{
    LCD_COMMAND,
    LCD_DATA
} LcdDataType;

static char board_name[7][LCD_LINE_LENGTH+1] =
{
    "EX4300-24T      ",
    "EX4300-48T      ",
    "EX4300-24P      ",
    "EX4300-48P      ",
    "EX4300-48T-BF   ",
    "EX4300-48T-DC   ",
    "EX4300-32F      ",
};

static int board_index = 0;

static char lcd_data[2][LCD_LINE_LENGTH+1] =
    {"Juniper EX4300  ",
    "U-boot...       "};

static int curUnit;
static int curline[CFG_LCD_NUM];
static int curpos[CFG_LCD_NUM];
static int lcdReady[CFG_LCD_NUM] = {0};
int d_count, o_count = 0;
int lcd_gone[CFG_LCD_NUM] = {0};

static inline int 
lcd_wait_till_not_busy (uint16_t retMode)
{
    uint16_t temp, tmp;
    int loop_count = 0;
#if (EPLD_LCD_4BIT_MODE)
    uint16_t dummy;
#endif

    if (!lcdReady[curUnit])
	return 0;

    if (!epld_reg_read(EPLD_LCD_CONTROL_REG, &temp)) {
	debug ("Unable to read EPLD register address 0x%x.\n",
		EPLD_LCD_CONTROL_REG*2);
	return 0;
    }

    udelay(1);
    if (EPLD_LCD_INST_READ != (temp & ~EPLD_LCD_RW_RS_BIT_MASK)) {
	temp = (temp & EPLD_LCD_RW_RS_BIT_MASK) | EPLD_LCD_INST_READ;
	if (!epld_reg_write(EPLD_LCD_CONTROL_REG, temp)) {
	    debug ("Unable to write EPLD register address 0x%x[0x%x].\n",
		    EPLD_LCD_CONTROL_REG*2, temp);
	    return 0;
	}
    }

    udelay(1);
    while (epld_reg_read(EPLD_LCD_DATA_REG, &tmp)) {
#if (EPLD_LCD_4BIT_MODE)
        udelay(1);
	if (!epld_reg_write(EPLD_LCD_CONTROL_REG, temp)) {
	    debug ("Unable to write EPLD register address 0x%x[0x%x].\n",
		    EPLD_LCD_CONTROL_REG*2, temp);
	    return 0;
	}
        udelay(1);
	epld_reg_read(EPLD_LCD_DATA_REG, &dummy);
#endif
	loop_count++;
	if (loop_count > LCD_BUSY_MAX) {
	    lcd_gone[curUnit] = 1;
	    return 0;
	}

	if (0 == (tmp & LCD_BUSY)) {
	    temp = (temp & EPLD_LCD_RW_RS_BIT_MASK) | retMode;
            udelay(1);
	    if (!epld_reg_write(EPLD_LCD_CONTROL_REG, temp)) {
		debug ("Unable to write EPLD register address 0x%x[0x%x].\n",
			EPLD_LCD_CONTROL_REG*2, temp);
		return 0;
	    }
	    return 1;
	}
        udelay(1);
	if (!epld_reg_write(EPLD_LCD_CONTROL_REG, temp)) {
	    debug ("Unable to write EPLD register address 0x%x[0x%x].\n",
		    EPLD_LCD_CONTROL_REG*2, temp);
	    return 0;
	}
    }
    return 0;
}

static int
lcd_write (LcdDataType dtype, uint8_t data)
{
    if (lcd_wait_till_not_busy(dtype ? EPLD_LCD_DATA_WRITE : EPLD_LCD_INST_WRITE)) {
#if (EPLD_LCD_4BIT_MODE)
	if (!epld_reg_write(EPLD_LCD_DATA_REG, data & 0xf0)) {
	    debug ("Unable to write EPLD register address 0x%x[0x%x].\n",
		    EPLD_LCD_CONTROL_REG*2, data);
	    return 0;
	}

	/* Delay of 40 micro seconds to resolve LCD char corruption */
	udelay(40);
	if (!epld_reg_write(EPLD_LCD_DATA_REG, (data & 0x0f) << 4)) {
	    debug ("Unable to write EPLD register address 0x%x[0x%x].\n",
		    EPLD_LCD_CONTROL_REG*2, data);
	    return 0;
	}
	/* Delay of 40 micro seconds to resolve LCD char corruption */
	udelay(40);
#else
	if (!epld_reg_write(EPLD_LCD_DATA_REG, data)) {
	    debug ("Unable to write EPLD register address 0x%x[0x%x].\n",
		    EPLD_LCD_CONTROL_REG*2, data);
	    return 0;
	}
#endif
	return 1;
    }
    return 0;
}

static int 
lcd_write_nochk (LcdDataType dtype, uint8_t data)
{
#if (EPLD_LCD_4BIT_MODE)
    if (!epld_reg_write(EPLD_LCD_DATA_REG, data & 0xf0)) {
	debug ("Unable to write EPLD register address 0x%x[0x%x].\n",
		EPLD_LCD_CONTROL_REG*2, data);
	return 0;
    }
    udelay(40);
    if (!epld_reg_write(EPLD_LCD_DATA_REG, (data & 0x0f) << 4)) {
	debug ("Unable to write EPLD register address 0x%x[0x%x].\n",
		EPLD_LCD_CONTROL_REG*2, data);
	return 0;
    }
    udelay(40);
#else
    if (!epld_reg_write(EPLD_LCD_DATA_REG, data)) {
	debug ("Unable to write EPLD register address 0x%x[0x%x].\n",
		EPLD_LCD_CONTROL_REG*2, data);
	return 0;
    }
#endif
    return 1;
}

#if (EPLD_LCD_4BIT_MODE)
static int 
lcd_write_once (LcdDataType dtype, uint8_t data)
{
    if (!epld_reg_write(EPLD_LCD_DATA_REG, data & 0xf0)) {
	debug ("Unable to write EPLD register address 0x%x[0x%x].\n",
	EPLD_LCD_CONTROL_REG*2, data);
	return 0;
    }
    return 1;
}
#endif

static int 
lcd_display (int addr, char *data)
{
    int i, len = LCD_LINE_LENGTH;

    if (addr == LCD_LINE0_ADDR)
	len--;

    /* needs 2ms delay between two writes. */
    udelay(2000);
    if (!lcd_write(LCD_COMMAND, LCD_DDRAM_ADDR + addr))
	return 0;

    for (i = 0; (i < len) && (*data != '\0'); i++) {
	/* needs 2ms delay between two writes. */
	udelay(2000);
	if (!lcd_write(LCD_DATA, *data++))
	    return 0;
    }
    return 1;
}

static void 
lcd_unit (int unit)
{
    uint16_t tmp = 0;
    if (!epld_reg_read(EPLD_LCD_CONTROL_REG, &tmp)) {
	debug ("Unable to read EPLD register address 0x%x.\n",
		EPLD_LCD_CONTROL_REG*2);
	return;
    }

    if (unit == LCD_BACK) {
	tmp &= ~EPLD_LCD_FRONT_EN;
    } else {
	tmp |= EPLD_LCD_FRONT_EN;
    }

    if (!epld_reg_write(EPLD_LCD_CONTROL_REG, tmp)) {
	debug ("Unable to read EPLD register address 0x%x.\n",
	EPLD_LCD_CONTROL_REG*2);
	return;
    }
    curUnit = unit;
}

void 
lcd_init (int unit)
{
    uint16_t tmp = 0;

    if ((unit < 0) || (unit >= CFG_LCD_NUM))
	return;

    lcd_unit(unit);

    if (!epld_reg_read(EPLD_LCD_CONTROL_REG, &tmp)) {
	debug ("Unable to read EPLD register address 0x%x.\n",
	EPLD_LCD_CONTROL_REG*2);
	return;
    }

    tmp &= EPLD_LCD_RW_RS_BIT_MASK;
    tmp |= EPLD_LCD_INST_WRITE;
    if (!epld_reg_write(EPLD_LCD_CONTROL_REG, tmp)) {
	debug ("Unable to write EPLD register address 0x%x[0x%x].\n",
	EPLD_LCD_CONTROL_REG*2, tmp);
	return;
    }

#if (EPLD_LCD_4BIT_MODE)
    /* ST7070 4 bit interface */
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
    lcd_write_nochk(LCD_COMMAND, LCD_DISPLAY_ON);
    udelay(40);

    /*
     * Because LCD characters on the LCD display sometimes
     * were seen invereted, explictly set the direction bit.
     */ 
    /* function set, 4-bit, 2-line display,
     * enable Extension Inst.
     */
    lcd_write_nochk(LCD_COMMAND, LCD_MODE_4B_EXT);
    udelay(40);

    /* COM SEG direction select . no reverse */ 
    lcd_write_nochk(LCD_COMMAND, LCD_COMSEG_NOREVERSE);
    udelay(40);

    lcd_write_nochk(LCD_COMMAND, LCD_MODE_4BITS);
    udelay(40);

    /*
     * Need this setting, as we shifted to
     * ext mode, the normal writes are with
     * ext mode set to 0
     */
    lcd_write_nochk(LCD_COMMAND, LCD_DISPLAY_ON);
    udelay(40);
    
    lcd_write_nochk(LCD_COMMAND, LCD_CLEAR_DISPLAY);
    udelay(1600);

    lcd_write_nochk(LCD_COMMAND, 0x6);
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

    if (lcd_gone[unit] == 0)  /* kludge to detect LCD functional */
        lcdReady[unit] = 1;  /* use for manual control of LCD enable */

    switch (board_id()) {
    case I2C_ID_JUNIPER_EX4300_24T:
	board_index = 0;
	break;
    case I2C_ID_JUNIPER_EX4300_48T:
	board_index = 1;
	break;
    case I2C_ID_JUNIPER_EX4300_24_P:
	board_index = 2;
	break;
    case I2C_ID_JUNIPER_EX4300_48_P:
	board_index = 3;
	break;
    case I2C_ID_JUNIPER_EX4300_48_T_BF:
	board_index = 4;
	break;
    case I2C_ID_JUNIPER_EX4300_48_T_DC:
	board_index = 5;
	break;
    case I2C_ID_JUNIPER_EX4300_32F:
        board_index = 6;
        break;
    default:
	puts("unknown board type, lcd display will be incorrect\n");
	break;
    }
    memcpy( lcd_data[0], board_name[board_index], LCD_LINE_LENGTH-1);


    lcd_data[0][LCD_LINE_LENGTH] = 0;
    lcd_data[1][LCD_LINE_LENGTH] = 0;

    curline[unit] = curpos[unit] = 0;

    lcd_display(LCD_LINE0_ADDR, lcd_data[0]);
    lcd_display(LCD_LINE1_ADDR, lcd_data[1]);
}


void 
lcd_off (int unit)
{
    if ((unit < 0) || (unit >= CFG_LCD_NUM))
	return;

    lcd_clear(unit);
    lcd_backlight_control(unit, LCD_BACKLIGHT_OFF);
    curline[unit] = curpos[unit] = 0;
    lcdReady[unit] = 0;  /* use for manual control of LCD disable */
}

void 
lcd_clear (int unit)
{
    if (!lcdReady[unit])
	return;

    lcd_unit(unit);
    lcd_write(LCD_COMMAND, LCD_CLEAR_DISPLAY);
}


static void 
lcd_write_char (const char c)
{
    int i, len;

    /* ignore CR */
    if (c == '\r')
	return;

    len = LCD_LINE_LENGTH;
    if (curline[curUnit] == 0)
	len--;

    if (c == '\n') {
	lcd_display(LCD_LINE0_ADDR, &lcd_data[curline[curUnit]^1][0]);
	lcd_display(LCD_LINE1_ADDR, &lcd_data[curline[curUnit]][0]);

	/* Do a line feed */
	curline[curUnit] ^= 1;
	len = LCD_LINE_LENGTH;
	if (curline[curUnit] == 0)
	    len--;
	curpos[curUnit] = 0;

	for (i = 0; i < len; i++)
	    lcd_data[curline[curUnit]][i] = ' ';

	return;
    }

    /* Only allow to be output if there is room on the LCD line */
    if (curpos[curUnit] < len)
	lcd_data[curline[curUnit]][curpos[curUnit]++] = c;
}

static void 
lcd_write_string (const char *s)
{
    char *p;

    for (p = (char *)s; *p != '\0'; p++)
	lcd_write_char(*p);
}

void 
lcd_printf (int unit, const char *fmt, ...)
{
    va_list args;
    char buf[CONFIG_SYS_PBSIZE];

    if ((unit < 0) || (unit >= CFG_LCD_NUM))
	return;

    if (!lcdReady[unit])
	return;

    lcd_unit(unit);

    va_start(args, fmt);
    (void)vsprintf(buf, fmt, args);
    va_end(args);

    lcd_write_string(buf);
}

void 
lcd_printf_2 (const char *fmt, ...)
{
    va_list args;
    char buf[CONFIG_SYS_PBSIZE];
    int i;

    for (i = 0; i < CFG_LCD_NUM; i++) {
	if (!lcdReady[i])
	    continue;

	memset(buf, 0, LCD_LINE_LENGTH);

	lcd_unit(i);

	va_start(args, fmt);
	(void)vsprintf(buf, fmt, args);
	va_end(args);

	buf[LCD_LINE_LENGTH+1] = '\0';

	/* display to second line */
	curline[curUnit] = 1;
	curpos[curUnit] = 0;
	lcd_write_string(buf);
	lcd_display(LCD_LINE0_ADDR, &lcd_data[0][0]);
	lcd_display(LCD_LINE1_ADDR, &lcd_data[1][0]);
    }
}


int
lcd_heartbeat (int unit)
{
    static char rotate[] = {'|', '/', '-', '\315'};
    static int index[CFG_LCD_NUM] = {0};

    if ((unit < 0) || (unit >= CFG_LCD_NUM))
	return 0;

    if (!lcdReady[unit])
	return 0;

    lcd_unit(unit);

    lcd_write(LCD_COMMAND, LCD_DDRAM_ADDR +
	      LCD_LINE0_ADDR + LCD_LINE_LENGTH - 1);
    lcd_write(LCD_DATA, rotate[index[unit]]);

    if (++index[unit] >= (sizeof rotate / sizeof rotate[0]))
	index[unit] = 0;

    if (lcd_gone[unit])  /* kludge to detect LCD functional */
	lcdReady[unit] = 0;

    return 1;
}

/* FIXME - have a stub for now */
int
lcd_backlight_control (int unit, LcdBacklight state)
{
    return 0;
}

void
lcd_cmd (int unit, uint8_t cmd)
{
    if ((unit < 0) || (unit >= CFG_LCD_NUM))
	return;

    lcd_unit(unit);

    lcd_write(LCD_COMMAND, cmd);
}

static int 
lcd_read (LcdDataType dtype, uint8_t* data)
{
    uint16_t temp;

    if (lcd_wait_till_not_busy(dtype ? EPLD_LCD_DATA_READ :
				       EPLD_LCD_INST_READ)) {
	if (!epld_reg_read(EPLD_LCD_DATA_REG, &temp)) {
	    debug ("Unable to read EPLD register address 0x%x[0x%x].\n",
		    EPLD_LCD_CONTROL_REG*2, *data);
	    return 0;
	}
#if (EPLD_LCD_4BIT_MODE)
	uint16_t temp1;

	if (!epld_reg_read(EPLD_LCD_DATA_REG, &temp1)) {
	    debug ("Unable to read EPLD register address 0x%x[0x%x].\n",
		    EPLD_LCD_CONTROL_REG*2, *data);
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

void 
lcd_dump (int unit)
{
    uint8_t data;
    int i;

    if ((unit < 0) || (unit >= CFG_LCD_NUM))
	return;

    if (!lcdReady[unit])
	return;

    lcd_unit(unit);

    printf("LCD[%s] DDRAM:\n", unit ? "back" : "front");
    for (i = 0; i<LCD_LINE_LENGTH; i++) {
	lcd_write(LCD_COMMAND, LCD_DDRAM_ADDR + LCD_LINE0_ADDR + i);
	lcd_read(LCD_DATA, &data);
	printf("%02x ", data);
    }
    printf("\n");
    for (i = 0; i<LCD_LINE_LENGTH; i++) {
	lcd_write(LCD_COMMAND, LCD_DDRAM_ADDR + LCD_LINE1_ADDR + i);
	lcd_read(LCD_DATA, &data);
	printf("%02x ", data);
    }
    printf("\n");
}

