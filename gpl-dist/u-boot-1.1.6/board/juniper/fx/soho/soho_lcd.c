/*
 * Copyright (c) 2009-2013, Juniper Networks, Inc.
 * All rights reserved.
 *
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

/* Soho 4 Bit LCD Driver 
 *
 * Chun Ng (cng@juniper.net)
 *
 * */


#include "soho_cpld.h"
#include "soho_lcd.h"
#include "soho_cpld_map.h"
#include <command.h>

boolean soho_lcd_dbg_print = FALSE;

static uint8_t soho_lcd_data[2][SOHO_LCD_LINE_LEN] =
                        {"FX SOHO        ", 
                         "Loading U-BOOT "};

static uint8_t pa_lcd_data[2][SOHO_LCD_LINE_LEN] =
                        {"QFX3500        ", 
                         "Loading U-BOOT "};

static uint8_t wa_lcd_data[2][SOHO_LCD_LINE_LEN] =
                        {"QFX3600        ", 
                         "Loading U-BOOT "};

static uint8_t lcd_data[2][SOHO_LCD_LINE_LEN];

static soho_cpld_status_t soho_lcd_write (soho_lcd_rs_t rs, uint8_t data);

static soho_cpld_status_t soho_lcd_read (soho_lcd_rs_t rs, uint8_t *data);

static soho_cpld_status_t soho_lcd_set_ddram_addr(uint8_t addr);

static soho_cpld_status_t soho_lcd_write_data_to_ram (uint8_t data);

static soho_cpld_status_t soho_lcd_read_data_from_ram(uint8_t *data);

static soho_cpld_status_t soho_lcd_return_home(void);

static soho_cpld_status_t soho_lcd_clear_display(void);

static soho_cpld_status_t soho_lcd_display_ctrl(boolean display_on, 
                                                 boolean cursor_on, 
                                                 boolean font_tbl_page_one);
static soho_cpld_status_t soho_lcd_set_cgram_addr(uint8_t addr);

soho_cpld_status_t
soho_lcd_wait (void)
{
    uint8_t reg_val;
    uint8_t i;
    soho_cpld_status_t status;

    for (i = 0; i < SOHO_LCD_MAX_WAIT_LOOP_CNT; i++) {
        if ((status = soho_lcd_read(SOHO_LCD_INSTR, &reg_val)) 
                                                    != SOHO_CPLD_OK) {
            return status;
        }

        if ((reg_val & SOHO_LCD_BF_MASK) != SOHO_LCD_BF_MASK) {
            return SOHO_CPLD_OK;
        }
        udelay(10);
    }

    if (soho_lcd_dbg_print) {
        printf("%s: LCD Too Busy\n", __func__);
    }
    return SOHO_CPLD_BUSY;
}

soho_cpld_status_t
soho_lcd_init (void)
{
    soho_cpld_status_t status;
    boolean display_on = TRUE;
    boolean cursor_on = FALSE;
    boolean page_one = TRUE;
    uint8_t i, j;

    soho_cpld_write(SOHO_SCPLD_LCD_TUNE, 0x7);
    /* default function set */
    soho_lcd_write(SOHO_LCD_INSTR, (SOHO_LCD_FUNC_SET | SOHO_LCD_8_BIT_MODE 
                                    | SOHO_LCD_2_LINE));

    udelay(2000);

    /* default function set */
    soho_lcd_write(SOHO_LCD_INSTR, (SOHO_LCD_FUNC_SET | SOHO_LCD_8_BIT_MODE 
                                    | SOHO_LCD_2_LINE));
    udelay(40);

    /* default function set */
    soho_lcd_write(SOHO_LCD_INSTR, (SOHO_LCD_FUNC_SET | SOHO_LCD_8_BIT_MODE 
                                    | SOHO_LCD_2_LINE));
    udelay(40);

    /* function set, 4 bit */
    soho_lcd_write(SOHO_LCD_INSTR, (SOHO_LCD_FUNC_SET | SOHO_LCD_4_BIT_MODE 
                                    | SOHO_LCD_2_LINE));
    udelay(40);

    /* function set, 4 bit */
    soho_lcd_write(SOHO_LCD_INSTR, (SOHO_LCD_FUNC_SET | SOHO_LCD_4_BIT_MODE 
                                    | SOHO_LCD_2_LINE));
    udelay(40);

    /* function set, 4 bit  2 line mode*/
    soho_lcd_write(SOHO_LCD_INSTR, (SOHO_LCD_FUNC_SET | SOHO_LCD_4_BIT_MODE 
                                    | SOHO_LCD_2_LINE));
    udelay(40);

    /* display on/off */
    if ((status = soho_lcd_display_ctrl(display_on, cursor_on, page_one)) 
                                                            != SOHO_CPLD_OK) {
        return status;
    }

    /* clear display */
    if ((status = soho_lcd_clear_display()) != SOHO_CPLD_OK) {
        return status;
    }

    /* entry mode set */
    if ((status = soho_lcd_wait()) != SOHO_CPLD_OK) {
        return status;
    }
    status = soho_lcd_write(SOHO_LCD_INSTR, (SOHO_LCD_ENTRY_MODE_SET | SOHO_LCD_INCRE_MODE));

    udelay(1000);

    for (i = 0 ; i < 2; i++) {
        for (j = 0; j < SOHO_LCD_LINE_LEN; j++) {
            if (fx_is_soho()) {
                lcd_data[i][j] = soho_lcd_data[i][j];
            } else if (fx_is_wa()) {
                lcd_data[i][j] = wa_lcd_data[i][j];
            } else {
                if (fx_is_wa())
                     lcd_data[i][j] = wa_lcd_data[i][j];
                else
                     lcd_data[i][j] = pa_lcd_data[i][j];
            }
        }
    }

    soho_lcd_write_str((char *)lcd_data[0], 0);
    soho_lcd_write_str((char *)lcd_data[1], 1);

    if (soho_lcd_dbg_print == TRUE) {
        printf("%s[%d]\n", __func__, __LINE__);
    }
    return status;
}

static soho_cpld_status_t
soho_lcd_write (soho_lcd_rs_t rs, uint8_t data)
{
    soho_cpld_status_t status;

    if ((status = soho_cpld_write(SOHO_SCPLD_LCD_RS, rs)) != SOHO_CPLD_OK) {
        return status;
    }

    if ((status = soho_cpld_write(SOHO_SCPLD_LCD_ACCESS, 
                               data & DATA_UPPER_4_BIT_MASK)) != SOHO_CPLD_OK) {
        return status;
    }

    return (soho_cpld_write(SOHO_SCPLD_LCD_ACCESS, ((data & DATA_LOWER_4_BIT_MASK) << 4)));
}


static soho_cpld_status_t
soho_lcd_read (soho_lcd_rs_t rs, uint8_t *data)
{
    soho_cpld_status_t status;
    uint8_t reg_val;

    if (data == NULL) {
        return SOHO_CPLD_NULL;
    }

    if ((status = soho_cpld_write(SOHO_SCPLD_LCD_RS, rs)) != SOHO_CPLD_OK) {
        return status;
    }

    if ((status = soho_cpld_read(SOHO_SCPLD_LCD_ACCESS, &reg_val)) != SOHO_CPLD_OK) {
        return status;
    }

    if ((status = soho_cpld_read(SOHO_SCPLD_LCD_DATA, &reg_val)) != SOHO_CPLD_OK) {
        return status;
    }

    *data = (reg_val & DATA_UPPER_4_BIT_MASK);

    if ((status = soho_cpld_read(SOHO_SCPLD_LCD_ACCESS, &reg_val)) != SOHO_CPLD_OK) {
        return status;
    }

    if ((status = soho_cpld_read(SOHO_SCPLD_LCD_DATA, &reg_val)) != SOHO_CPLD_OK) {
        return status;
    }

    *data |= ((reg_val & DATA_UPPER_4_BIT_MASK) >> 4);

    return status;
}

soho_cpld_status_t
soho_lcd_write_str (char *str, uint8_t line_no)
{
    char *tmp_str = str;
    uint8_t line_char_cnt = 0;
    uint8_t addr;
    uint8_t i;
    soho_cpld_status_t status;
#ifdef SOHO_LCD_DEBUG
    uint8_t data;
#endif

    if (tmp_str == NULL) {
        return SOHO_CPLD_NULL;
    }

    while (*tmp_str != '\0' && line_char_cnt < SOHO_LCD_LINE_LEN) {
        lcd_data[line_no][line_char_cnt] = *tmp_str++;
        line_char_cnt++;
    }

    addr =  (line_no == 0) ? SOHO_LCD_LINE_1_ADDR_START 
                           : SOHO_LCD_LINE_2_ADDR_START;

    if ((status = soho_lcd_set_cgram_addr(addr)) != SOHO_CPLD_OK) {
        return status;
    }

    if ((status = soho_lcd_set_ddram_addr(addr)) != SOHO_CPLD_OK) {
        return status;
    }

    for (i = 0; i < line_char_cnt; i++) {
        if ((status = soho_lcd_write_data_to_ram(lcd_data[line_no][i])) 
                                                            != SOHO_CPLD_OK) {
            printf("%s: write data to ram failed: %d\n", __func__, status);
            return status;
        }
    }

#ifdef SOHO_LCD_DEBUG
    for (i = 0; i < SOHO_LCD_LINE_LEN; i++) {
        soho_lcd_set_ddram_addr(addr + i); 
        soho_lcd_read_data_from_ram(&data);
        printf("read back addr=0x%x data=0x%x\n", addr + i, data);
    }
#endif
    return SOHO_CPLD_OK;
}

soho_cpld_status_t
fx_lcd_write (char *str, uint8_t line_no) 
{
    if (fx_is_pa() || fx_is_wa()) {
        return (soho_lcd_write_str(str, line_no));
    }

    return SOHO_CPLD_OK;
}


static soho_cpld_status_t
soho_lcd_clear_display (void)
{
    soho_cpld_status_t status;

    if ((status = soho_lcd_wait()) != SOHO_CPLD_OK) {
        return status;
    }

    status = soho_lcd_write(SOHO_LCD_INSTR, SOHO_LCD_DISP_CLR);
    udelay(1600);

    return status;
}

static soho_cpld_status_t
soho_lcd_display_ctrl (boolean display_on, boolean cursor_on, 
                       boolean font_tbl_page_one) 
{
    uint8_t reg_val = SOHO_LCD_DISP_CTRL;
    soho_cpld_status_t status;

    if (display_on == TRUE) {
        reg_val |= 1 << 2;
    }

    if (cursor_on == TRUE) {
        reg_val |= 1 << 1;
    }

    if (font_tbl_page_one == FALSE) {
        reg_val |= 1 << 0;
    }

    if ((status = soho_lcd_wait()) != SOHO_CPLD_OK) {
        return status;
    }

    status = soho_lcd_write(SOHO_LCD_INSTR, reg_val); 
    udelay(40);

    return status;
}


static soho_cpld_status_t
soho_lcd_set_cgram_addr (uint8_t addr)
{
    soho_cpld_status_t status;
    
    if ((status = soho_lcd_wait()) != SOHO_CPLD_OK) {
        return status;
    }

    status = soho_lcd_write(SOHO_LCD_INSTR, (addr | SOHO_LCD_SET_CGRAM_ADDR_MASK));
    udelay(50);

    return status;
}

static soho_cpld_status_t
soho_lcd_set_ddram_addr (uint8_t addr)
{
    soho_cpld_status_t status;
    
    if ((status = soho_lcd_wait()) != SOHO_CPLD_OK) {
        return status;
    }

    status = soho_lcd_write(SOHO_LCD_INSTR, (addr | SOHO_LCD_SET_DDRAM_ADDR));
    udelay(50);

    return status;
}

static soho_cpld_status_t
soho_lcd_write_data_to_ram (uint8_t data)
{
    soho_cpld_status_t status;

    if ((status = soho_lcd_wait()) != SOHO_CPLD_OK) {
        return status;
    }

    status = soho_lcd_write(SOHO_LCD_DATA, data);
    udelay(50);

    return status;
}

static soho_cpld_status_t
soho_lcd_read_data_from_ram (uint8_t *data)
{
    soho_cpld_status_t status;

    if ((status = soho_lcd_wait()) != SOHO_CPLD_OK) {
        return status;
    }

    status = soho_lcd_read(SOHO_LCD_DATA, data);
    udelay(40);

    return status;
}

static soho_cpld_status_t
soho_lcd_return_home (void)
{
    soho_cpld_status_t status;

    if ((status = soho_lcd_wait()) != SOHO_CPLD_OK) {
        return status;
    }

    status = soho_lcd_write(SOHO_LCD_INSTR, SOHO_LCD_RET_HOME);
    udelay(1600);

    return status;
}

static soho_cpld_status_t
soho_lcd_function_set (boolean bit_8_mode, boolean line_2_mode, boolean ext_mode)
{
    soho_cpld_status_t status;
    uint8_t reg_val = SOHO_LCD_FUNC_SET;

    if ((status = soho_lcd_wait()) != SOHO_CPLD_OK) {
        return status;
    }

    if (bit_8_mode) {
        reg_val |= 1 << 4;
    }

    status = soho_lcd_write(SOHO_LCD_INSTR, SOHO_LCD_RET_HOME);

    udelay(40);
}

#ifdef __RMI_BOOT2__
int soho_do_lcd(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uint8_t lcmd;
    uint8_t line_no;
    uint8_t data;
    char cmd1;
    int i, len;
    soho_cpld_status_t status;
    uint8_t addr;

    if (argc < 2)
        goto usage;
    
    cmd1 = argv[1][0];
    
    switch (cmd1) {
        case 'i':   /* init */
        case 'I':
            soho_lcd_init();
            break;
            
        case 'b':   /* busy */
        case 'B':
            if ((status = soho_lcd_read(SOHO_LCD_INSTR, &data)) != 
                SOHO_CPLD_OK) {
                printf("lcd read failed (%d).", status);
                return 0;
            }
            printf("%02x\n", data);
            break;
            
        case 'c':   /* command */
        case 'C':
            if (argc < 3)
                goto usage;

            lcmd = simple_strtoul(argv[2], NULL, 16);
            if ((status = soho_lcd_write(SOHO_LCD_INSTR, lcmd)) != 
                SOHO_CPLD_OK) {
                printf("lcd write failed (%d).", status);
                return 0;
            }
            break;
            
        case 'p':   /* print */
        case 'P':
            if (argc < 4)
                goto usage;

            line_no = simple_strtoul(argv[2], NULL, 16);
            addr =  (line_no == 0) ? SOHO_LCD_LINE_1_ADDR_START :
                                     SOHO_LCD_LINE_2_ADDR_START;
            len = strlen(argv[3]);
            if (len > SOHO_LCD_DDRAM_LINE_LEN) {
                len = SOHO_LCD_DDRAM_LINE_LEN;
            }

            if ((status = soho_lcd_set_cgram_addr(addr)) != SOHO_CPLD_OK) {
                return 0;
            }
            if ((status = soho_lcd_set_ddram_addr(addr)) != SOHO_CPLD_OK) {
                return 0;
            }
            
            for (i = 0; i < len; i++) {
                if ((status = soho_lcd_write_data_to_ram(argv[3][i])) != 
                    SOHO_CPLD_OK) {
                    return 0;
                }
            }
            break;
            
        case 's':   /* shift */
        case 'S':
            if (argc < 3)
                goto usage;

            if (!strncmp(argv[2],"left", 4)) {
                soho_lcd_write(SOHO_LCD_INSTR, 
                               SOHO_LCD_DISP_SHIFT_CTRL |
                               SOHO_LCD_DISP_SHIFT_LEFT);
            } else if (!strncmp(argv[2],"right", 5)) {
                soho_lcd_write(SOHO_LCD_INSTR, 
                               SOHO_LCD_DISP_SHIFT_CTRL |
                               SOHO_LCD_DISP_SHIFT_RIGHT);
            } else {
                goto usage;
            }
            
            break;
            
        case 'd':   /* dump */
        case 'D':
            printf("LCD DDRAM:\n");
            for (i = 0; i < SOHO_LCD_LINE_LEN; i++) {
                soho_lcd_set_ddram_addr(SOHO_LCD_LINE_1_ADDR_START + i); 
                soho_lcd_read_data_from_ram(&data);
                printf("%02x ", data);
            }
            printf("\n");
            for (i = 0; i < SOHO_LCD_LINE_LEN; i++) {
                soho_lcd_set_ddram_addr(SOHO_LCD_LINE_2_ADDR_START + i); 
                soho_lcd_read_data_from_ram(&data);
                printf("%02x ", data);
            }
            printf("\n");
            break;
                        
        default:
            printf("???");
            break;
    }

    return 1;
 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return 1;
}

U_BOOT_CMD(
    soho_lcd,   4,  1,  soho_do_lcd,
    "soho_lcd - lcd test commands\n",
    "\n"
    "soho_lcd init\n"
    "    - LCD init\n"
    "soho_lcd busy\n"
    "    - LCD read busy and address\n"
    "soho_lcd command <cmd>\n"
    "    - LCD control command\n"
    "soho_lcd print <line> <string>\n"
    "    - LCD print\n"
    "soho_lcd shift [left | right]\n"
    "    - LCD display shift left/right\n"
    "soho_lcd dump\n"
    "    - dump LCD DDRAM\n"
);
#endif
