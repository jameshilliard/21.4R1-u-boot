/*
 * Copyright (c) 2009-2013, Juniper Networks, Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include "rmi/gpio.h"
#include "rmi/pcmcia.h"
#include "rmi/on_chip.h"
#include "rmi/board.h"
#include <common/fx_cpld.h>
#include "soho_cpld.h"
#include "common.h"
#include <common/fx_common.h>

#ifndef _SOHO_LCD_H_
#define _SOHO_LCD_H_

typedef enum 
{
    SOHO_LCD_INSTR = 0,
    SOHO_LCD_DATA = 1
} soho_lcd_rs_t;

#define SOHO_LCD_LINE_LEN                       16
#define SOHO_LCD_DDRAM_LINE_LEN                 40
#define SOHO_LCD_MAX_NUM_CHAR                   (SOHO_LCD_LINE_LEN * 2)                      

#define SOHO_LCD_MAX_WAIT_LOOP_CNT              (0x40)

#define SOHO_LCD_2_LINE                         (0x1 << 3)

#define SOHO_LCD_LINE_1_ADDR_START              0x0
#define SOHO_LCD_LINE_2_ADDR_START              0x40 
#define SOHO_LCD_LINE_2_ADDR_END                SOHO_LCD_MAX_NUM_CHAR

#define DATA_LOWER_4_BIT_MASK                   (0xF)
#define DATA_UPPER_4_BIT_MASK                   (0xF0)

#define SOHO_LCD_DISP_CLR                       (0x1)

#define SOHO_LCD_RET_HOME                       (0x1 << 1)
#define SOHO_LCD_RET_HOME_MASK                  (0x3)

#define SOHO_LCD_ENTRY_MODE_SET                 (0x1 << 2)
#define SOHO_LCD_DECRE_MODE                     (0x0 << 1)
#define SOHO_LCD_INCRE_MODE                     (0x1 << 1)
#define SOHO_LCD_SHIFT_ON                       (0x1)

#define SOHO_LCD_DISP_CTRL                      (0x1 << 3)
#define SOHO_LCD_DISP_ON                        (0x1 << 2)
#define SOHO_LCD_DISP_CUR_ON                    (0x1 << 1)
#define SOHO_LCD_FONT_TBL_PAGE_2                (0x1)

#define SOHO_LCD_DISP_SHIFT_CTRL                (0x1 << 4)
#define SOHO_LCD_DISP_SHIFT_LEFT                (0x2 << 2)
#define SOHO_LCD_DISP_SHIFT_RIGHT               (0x3 << 2)

#define SOHO_LCD_FUNC_SET                       (0x1 << 5)
#define SOHO_LCD_FUNC_MASK                      (0x3F)

#define SOHO_LCD_SET_CGRAM_ADDR_MASK            (0x1 << 6)
#define SOHO_LCD_SET_CGRAM_ADDR                 ~(0x1 << 7)

#define SOHO_LCD_SET_DDRAM_ADDR                 (0x1 << 7)

#define SOHO_LCD_BF_MASK                        (0x1 << 7)

#define SOHO_LCD_8_BIT_MODE                     (1 << 4)
#define SOHO_LCD_4_BIT_MODE                     (0 << 4)

soho_cpld_status_t soho_lcd_write_str (char *str, u_int8_t line_no);
soho_cpld_status_t fx_lcd_write (char *str, u_int8_t line_no);
soho_cpld_status_t soho_lcd_init(void);

#endif

