/*
 * Copyright (c) 2011, Juniper Networks, Inc.
 * All rights reserved.
 *
 * This program is free software; you can redissohoute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is dissohouted in the hope that it will be useful,
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

#include <rmi/types.h>
#include "common/post.h"
#include "common/i2c.h"
#include "common/fx_gpio.h"
#include "soho_board.h"
#include "soho_cpld_map.h"
#include "soho_post_i2c.h"
#include "tor_cpld_i2c.h"

static const soho_i2c_dev soho_i2c_device[] =
{
    {
        0,
        0,
        0,
        SOHO_NO_CHECK,
        0x51,
        0x1,
        "ID EEPROM",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        0,
        0,
        0,
        SOHO_PS0_PRESENT,
        0x50,
        0,
        "PS0",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        0,
        1,
        0,
        SOHO_PS1_PRESENT,
        0x50,
        0,
        "PS1",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        0,
        2,
        0,
        SOHO_DDR2A_PRESENT,
        0x50,
        0,
        "DDR2 A",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        0,
        2,
        0,
        SOHO_DDR2B_PRESENT,
        0x52,
        0,
        "DDR2 B",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        0,
        2,
        0,
        SOHO_DDR2C_PRESENT,
        0x54,
        0,
        "DDR2 C",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        0,
        2,
        0,
        SOHO_DDR2D_PRESENT,
        0x56,
        0,
        "DDR2 D",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        0,
        3,
        0,
        SOHO_NO_CHECK,
        0x51,
        0x1,
        "Mgt ID EEPROM",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        0,
        4,
        0,
        SOHO_NO_CHECK,
        0x1C,
        0,
        "MAX6697 die temp",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        0,
        4,
        0,
        SOHO_NO_CHECK,
        0x48,
        0,
        "TMP435 exhaust LT",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        0,
        4,
        0,
        SOHO_NO_CHECK,
        0x49,
        0,
        "TMP435 exhaust RT",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        0,
        4,
        0,
        SOHO_NO_CHECK,
        0x4A,
        0,
        "TMP435 intake LT",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        0,
        4,
        0,
        SOHO_NO_CHECK,
        0x4B,
        0,
        "TMP435 intake RT",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        0,
        5,
        0,
        SOHO_NO_CHECK,
        0x48,
        0,
        "TMP435 exhaust LB",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        0,
        5,
        0,
        SOHO_NO_CHECK,
        0x49,
        0,
        "TMP435 exhaust RB",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        0,
        5,
        0,
        SOHO_NO_CHECK,
        0x4A,
        0,
        "TMP435 intake LB",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        0,
        5,
        0,
        SOHO_NO_CHECK,
        0x4B,
        0,
        "TMP435 intake RB",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        0,
        5,
        0,
        SOHO_NO_CHECK,
        0x35,
        0,
        "POWR1220 power mon",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        0,
        6,
        0,
        SOHO_SEEPROM_PRESENT,
        0x42,
        0,
        "Secure EEPROM",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        0,
        6,
        0,
        SOHO_NO_CHECK,
        0x68,
        0,
        "RTC",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        0,
        7,
        0,
        SOHO_NO_CHECK,
        0x5C,
        0,
        "LTC2978 power mon",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        1,
        0,
        0,
        SOHO_NO_CHECK,
        0x57,
        0x1,
        "LOG EEPROM",
        TOR_CPLD_I2C_16_OFF,
    },
    {
        1,
        0x3F,  /* bit map */
        0xFF,  /* bit map */
        SOHO_SFP_PRESENT,
        0x50,
        0x1,
        "SFP+",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        1,
        6,
        4,
        SOHO_CXP0_PRESENT,
        0x50,
        0,
        "CXP0",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        1,
        6,
        5,
        9,
        0x50,
        0,
        "CXP1",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        1,
        7,
        0,
        SOHO_TL0_PRESENT,
        0x78,
        0x80,
        "TL0",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        1,
        7,
        1,
        SOHO_TL1_PRESENT,
        0x78,
        0x80,
        "TL1",
        TOR_CPLD_I2C_8_OFF,
    },
    {
        1,
        7,
        2,
        SOHO_TQ_PRESENT,
        0x7C,
        0x80,
        "TQ",
        TOR_CPLD_I2C_8_OFF,
    },
};

int
soho_i2c_diag (void)
{
    int i, j, k, count = 0;
    uint8_t reg_val, addr, offset;
    soho_i2c_dev_detect i2c_dev_test[SOHO_MAX_I2C_DEVICES];
    uint32_t gpio_cfg = GPIO_CFG_IO_VAL;
    uint32_t gpio_data  = GPIO_WRITE_DATA_VAL;
    unsigned char desc[20], dev[14], found[12];
    unsigned char buf[3];
    int fail = 0;
    uint32_t mux;

    memset(i2c_dev_test, 0, 
           SOHO_MAX_I2C_DEVICES * sizeof(soho_i2c_dev_detect));

    for (i = 0; i < sizeof(soho_i2c_device)/sizeof(soho_i2c_device[0]); i++) {
        i2c_dev_test[count].bus = soho_i2c_device[i].bus;
        i2c_dev_test[count].pseg = soho_i2c_device[i].pseg;
        i2c_dev_test[count].sseg = soho_i2c_device[i].sseg;
        i2c_dev_test[count].index = 0xff;
        i2c_dev_test[count].dev_index = i;
        i2c_dev_test[count].i2c_mode= soho_i2c_device[i].i2c_mode;

        switch(soho_i2c_device[i].present) {
            case SOHO_PS0_PRESENT:
                soho_cpld_read(SOHO_MCPLD_FAN_PWR_PRE, &reg_val);
                i2c_dev_test[count].present = reg_val & 0x1 ? 1 : 0;
                break;
                
            case SOHO_PS1_PRESENT:
                soho_cpld_read(SOHO_MCPLD_FAN_PWR_PRE, &reg_val);
                i2c_dev_test[count].present = reg_val & 0x2 ? 1 : 0;
                break;
                
            case SOHO_DDR2A_PRESENT:
            case SOHO_DDR2B_PRESENT:
            case SOHO_DDR2C_PRESENT:
            case SOHO_DDR2D_PRESENT:
                i2c_dev_test[count].present = SOHO_DONT_CARE;
                break;
                
            case SOHO_SEEPROM_PRESENT:
                soho_cpld_read(MCPLD_RST_REG_OFFSET, &reg_val);
                soho_cpld_write(MCPLD_RST_REG_OFFSET, reg_val | 0x40);
                udelay(100);
                soho_cpld_write(MCPLD_RST_REG_OFFSET, reg_val & ~0x40);
                udelay(1000);
                i2c_dev_test[count].present = 1;
                break;
                
            case SOHO_SFP_PRESENT:
                /* expanding SFP list */
                for (j = 0; soho_i2c_device[i].pseg & (1 << j); j++) {
                    for (k = 0; soho_i2c_device[i].sseg & (1 << k); k++) {
                        i2c_dev_test[count].bus = 1;
                        i2c_dev_test[count].pseg = j;
                        i2c_dev_test[count].sseg = k;
                        soho_cpld_read(SOHO_SCPLD_SFP_0_INTR + (0x10 * j + k),
                                       &reg_val);
                        i2c_dev_test[count].present = reg_val & 0x10 ? 1 : 0;
                        i2c_dev_test[count].index = j * 8 + k;
                        i2c_dev_test[count].dev_index = i;
                        count++;
                    }
                }
                count--;
                
                break;
                
            case SOHO_CXP0_PRESENT:
                soho_cpld_read(SOHO_MCPLD_UPL_OPT_0_INTR, &reg_val);
                i2c_dev_test[count].present = reg_val & 0x20 ? 1 : 0;
                break;
                
            case SOHO_CXP1_PRESENT:
                soho_cpld_read(SOHO_MCPLD_UPL_OPT_2_INTR, &reg_val);
                i2c_dev_test[count].present = reg_val & 0x20 ? 1 : 0;
                break;
                
            case SOHO_TL0_PRESENT:
                soho_cpld_read(MCPLD_RST_REG_OFFSET, &reg_val);
                soho_cpld_write(MCPLD_RST_REG_OFFSET, reg_val & ~0x1);
                i2c_dev_test[count].present = 1;
                break;
                
            case SOHO_TL1_PRESENT:
                soho_cpld_read(MCPLD_RST_REG_OFFSET, &reg_val);
                soho_cpld_write(MCPLD_RST_REG_OFFSET, reg_val & ~0x2);
                i2c_dev_test[count].present = 1;
                break;
                
            case SOHO_TQ_PRESENT:
                soho_cpld_read(MCPLD_RST_REG_OFFSET, &reg_val);
                soho_cpld_write(MCPLD_RST_REG_OFFSET, reg_val & ~0x4);
                i2c_dev_test[count].present = 1;
                break;
                
            case SOHO_NO_CHECK:
            default:
                i2c_dev_test[count].present = 1;
                break;
        }
        count++;
    }

    /* check i2c device */
    for (i = 0; i < count; i++) {
        addr = soho_i2c_device[i2c_dev_test[i].dev_index].addr;
        offset = soho_i2c_device[i2c_dev_test[i].dev_index].reg;
        if (i2c_dev_test[i].present) {
            if (fx_use_i2c_cpld(addr)) {
                mux = TOR_CPLD_I2C_CREATE_MUX(i2c_dev_test[i].bus, i2c_dev_test[i].sseg, 
                                              i2c_dev_test[i].pseg);
            } else {
                mux = i2c_dev_test[i].bus; 
                gpio_cfg_io_set((gpio_cfg | 0x3F) | (1 << 24));
                if (i2c_dev_test[i].bus == 0) {
                    gpio_write_data_set((gpio_data & CHAN_MASK) | 
                                    i2c_dev_test[i].pseg | (1 << 24));

                } else {
                    gpio_write_data_set((gpio_data & CHAN_MASK)     | 
                                    (i2c_dev_test[i].sseg << 3) | 
                                    (i2c_dev_test[i].pseg)      | 
                                    (1 << 24));
                }
                udelay(10000);
            }
            if (offset == 0) {
                i2c_dev_test[i].found = i2c_probe(mux, addr) ? 
                                        0 : 1;
            } else {
                if (fx_use_i2c_cpld(addr)) {
                    i2c_dev_test[i].found = 
                        tor_cpld_i2c_read(mux, addr, offset, i2c_dev_test[i].i2c_mode, buf, 2) ? 0 : 1;
                } else {
                    i2c_dev_test[i].found = 
                        i2c_read_seq(mux, addr, offset, buf, 2) ? 0 : 1;
                }
            }
            if ((i2c_dev_test[i].found == 0) && 
                (i2c_dev_test[i].present != SOHO_DONT_CARE)) {
                fail = 1;
            }
        }
    }

    /* print final result */
    if (fail) {
        post_printf(POST_VERB_STD,
                    "-- I2C test                             FAILED @\n");
    } else {
        post_printf(POST_VERB_STD,
                    "-- I2C test                             PASSED\n");
    }

    /* print individual device result */
    for (i = 0; i < count; i++) {
        desc[0] = dev[0] = found[0] = 0;

        /* name */
        if (i2c_dev_test[i].index == 0xFF) {
            sprintf(desc, "%s", 
                    soho_i2c_device[i2c_dev_test[i].dev_index].name);
        } else {
            sprintf(desc, "%s %2d", 
                    soho_i2c_device[i2c_dev_test[i].dev_index].name,
                    i2c_dev_test[i].index);
        }

        /* mux */
        addr = soho_i2c_device[i2c_dev_test[i].dev_index].addr;
        if (i2c_dev_test[i].bus == 0) {
            if ((i2c_dev_test[i].pseg == 0) && 
                (addr == 0x51)) {
                sprintf(dev, "b%1d m*,- %02x ", 
                        i2c_dev_test[i].bus, addr);
            } else {
                sprintf(dev, "b%1d m%1d,- %02x ", 
                        i2c_dev_test[i].bus, 
                        i2c_dev_test[i].pseg, 
                        addr);
            }
        } else {
            if ((i2c_dev_test[i].pseg == 0) && 
                (addr == 0x57)) {
                sprintf(dev, "b%1d m*,* %02x", 
                        i2c_dev_test[i].bus, addr);
            } else {
                sprintf(dev, "b%1d m%1d,%1d %02x ", 
                        i2c_dev_test[i].bus, i2c_dev_test[i].pseg, 
                        i2c_dev_test[i].sseg, addr);
            }
        }
        if ((i2c_dev_test[i].found == 0) && (i2c_dev_test[i].present == 1)) {
            strcat(dev, "@");
        }

        /* found */
        sprintf(found, "%s", 
            i2c_dev_test[i].found ? "Found" :
            i2c_dev_test[i].present ? "Not Found" : "Not Present");

        post_printf(POST_VERB_STD,"%-20s %-12s %-14s\n", desc, found, dev);
    }
    
    return fail;
}

int
soho_diag_post_i2c_init (void)
{
    return 0;
}


int
soho_diag_post_i2c_run (void)
{
    return soho_i2c_diag();
}


int
soho_diag_post_i2c_clean (void)
{
    return 0;
}


