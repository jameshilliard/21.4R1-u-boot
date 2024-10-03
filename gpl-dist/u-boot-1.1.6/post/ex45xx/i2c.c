/*
 * Copyright (c) 2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * (C) Copyright 2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>

/******** EX45xx **********/

#ifdef CONFIG_POST

/*
 * I2C test
 *
 * For verifying the I2C bus, a full I2C bus scanning is performed.
 *
 * #ifdef I2C_ADDR_LIST
 *   The test is considered as passed if all the devices and
 *   only the devices in the list are found.
 * #else [ ! I2C_ADDR_LIST ]
 *   The test is considered as passed if any I2C device is found.
 * I2C_ADDR_LIST - The i2c device list.
 * #endif
 */

#include <post.h>
#include <i2c.h>
#include <fsl_i2c.h>
#include <epld.h>
#if !defined(CONFIG_PRODUCTION)
#include <cmd_ex45xx.h>
#endif

#if CONFIG_POST & CFG_POST_I2C

DECLARE_GLOBAL_DATA_PTR;

int i2c_post = 0;

#define MAX_MUX                 2
#define MAX_I2C_TEST_ENTRY      80

typedef int (*p2f) (uint8_t, uint16_t, uint16_t);
typedef int (*p2p) (void);
typedef struct i2c_dev_struct {
    int dev_ctrl;   /* 0 - controller 1, 1 - controller 2 */
    uint8_t dev_muxaddr[MAX_MUX];
    uint8_t dev_muxchan[MAX_MUX];
    uint8_t dev_addr;
    uint16_t dev_offset;
    uint16_t dev_len;
    uint8_t dev_speed;   /* 0 - 100KHz, 1 - 200KHz, 2 - 300KHz, 3 - 400KHz */
    int chk_present;   /* 0 - no check, 1 - check present */
    int present_ctrl;   /* 0 - controller 1, 1 - controller 2 */
    uint8_t present_muxaddr[MAX_MUX];
    uint8_t present_muxchan[MAX_MUX];
    uint8_t io_addr;
    uint8_t io_reg;
    uint8_t io_bit;
    uint8_t io_data; /* 0 - normal, 1 - inverted */
    p2f dev_op;  /* device function ptr */
    p2p present_op;  /* present function ptr */
    char name[50];
} i2c_dev;

typedef struct i2c_test_struct {
    struct i2c_dev_struct *i2c_list;
    int size;
    int result[MAX_I2C_TEST_ENTRY];
    int present;
    char name[16];
} i2c_test;

#define DEV_NOT_PRESENT 0
#define DEV_FAIL (-1)
#define DEV_PASS 1

int generic_read (uint8_t addr, uint16_t offset, uint16_t len);
int seeprom_read (uint8_t addr, uint16_t offset, uint16_t len);
int fan_present (void);

static uint8_t i2c_dev_fdr[4] = {0xd, 0x9, 0x6, 0x4};
static char i2c_speed_name[4][7] = 
 { "98KHz",
    "196KHz",
    "285KHz",
    "391KHz",
};

/* i2c dev list  T40 rev 1/2 */
i2c_dev i2c_t40_dev_list[] =
{
    {
        /* PCA9548A#1 (U17) */
        0,  /* device i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        CFG_I2C_C1_9548SW1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA9548A#1 (U17)",
    },
    {
        /* PS 1 */
        0,  /* device i2c controller */
        {CFG_I2C_C1_9548SW1, 0},  /* device mux */
        {1, 0},  /* device mux channel */
        CFG_I2C_C1_9548SW1_P1_PS,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {0x71, 0},  /* device mux */
        {1, 0},  /* device mux channel */
        0x24,  /* io expander addr */
        0x1,  /* read register */
        0x4,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "PS 1",
    },
    {
        /* PS 2 */
        0,  /* device i2c controller */
        {CFG_I2C_C1_9548SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C1_9548SW1_P2_PS,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {0x71, 0},  /* device mux */
        {1, 0},  /* device mux channel */
        0x24,  /* io expander addr */
        0x1,  /* read register */
        0x5,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "PS 2",
    },
    {
        /* LCD POT AD5245 front */
        0,  /* device i2c controller */
        {CFG_I2C_C1_9548SW1, 0},  /* device mux */
        {3, 0},  /* device mux channel */
        CFG_I2C_C1_9548SW1_P3_AD5245_1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "LCD POT AD5245 front",
    },
    {
        /* LCD POT AD5245 rear */
        0,  /* device i2c controller */
        {CFG_I2C_C1_9548SW1, 0},  /* device mux */
        {3, 0},  /* device mux channel */
        CFG_I2C_C1_9548SW1_P3_AD5245_2,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "LCD POT AD5245 rear",
    },
    {
        /* R5H30211 SEEPROM */
        0,  /* device i2c controller */
        {CFG_I2C_C1_9548SW1, 0},  /* device mux */
        {4, 0},  /* device mux channel */
        CFG_I2C_C1_9548SW1_P4_SEEPROM,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &seeprom_read,
        NULL,
        "R5H30211 SEEPROM",
    },
    {
        /* RTC-8564-JE */
        0,  /* device i2c controller */
        {CFG_I2C_C1_9548SW1, 0},  /* device mux */
        {5, 0},  /* device mux channel */
        CFG_I2C_C1_9548SW1_P5_RTC,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "RTC-8564-JE",
    },
    {
        /* Fan connector */
        0,  /* device i2c controller */
        {CFG_I2C_C1_9548SW1, 0},  /* device mux */
        {5, 0},  /* device mux channel */
        CFG_I2C_C1_9548SW1_P5_FAN,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        &fan_present,
        "Fan connector",
    },
    {
        /* PWR1220 */
        0,  /* device i2c controller */
        {CFG_I2C_C1_9548SW1, 0},  /* device mux */
        {6, 0},  /* device mux channel */
        CFG_I2C_C1_9548SW1_P6_PWR1220,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PWR1220",
    },
    {
        /* NE1617A Temp#0 */
        0,  /* device i2c controller */
        {CFG_I2C_C1_9548SW1, 0},  /* device mux */
        {6, 0},  /* device mux channel */
        CFG_I2C_C1_9548SW1_P6_TEMP_0,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "NE1617A Temp#0",
    },
    {
        /* NE1617A Temp#1 */
        0,  /* device i2c controller */
        {CFG_I2C_C1_9548SW1, 0},  /* device mux */
        {6, 0},  /* device mux channel */
        CFG_I2C_C1_9548SW1_P6_TEMP_1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "NE1617A Temp#1",
    },
    {
        /* W83782#0 */
        0,  /* device i2c controller */
        {CFG_I2C_C1_9548SW1, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        CFG_I2C_C1_9548SW1_ENV_0,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "W83782#0",
    },
    {
        /* W83782#1 */
        0,  /* device i2c controller */
        {CFG_I2C_C1_9548SW1, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        CFG_I2C_C1_9548SW1_ENV_1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "W83782#1",
    },
    {
        /* ISC9DB803 PCIe Clk#0 */
        0,  /* device i2c controller */
        {CFG_I2C_C1_9548SW1, 0},  /* device mux */
        {6, 0},  /* device mux channel */
        CFG_I2C_C1_9548SW1_P6_ICS_0,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "ISC9DB803 PCIe Clk#0",
    },
    {
        /* ISC9DB803 PCIe Clk#1 */
        0,  /* device i2c controller */
        {CFG_I2C_C1_9548SW1, 0},  /* device mux */
        {7, 0},  /* device mux channel */
        CFG_I2C_C1_9548SW1_P7_ICS_1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "ISC9DB803 PCIe Clk#1",
    },
    {
        /* ADI MUX#0 */
        0,  /* device i2c controller */
        {CFG_I2C_C1_9548SW1, 0},  /* device mux */
        {7, 0},  /* device mux channel */
        CFG_I2C_C1_9548SW1_P7_MUX_0,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "ADI MUX#0",
    },
    {
        /* ADI MUX#1 */
        0,  /* device i2c controller */
        {CFG_I2C_C1_9548SW1, 0},  /* device mux */
        {7, 0},  /* device mux channel */
        CFG_I2C_C1_9548SW1_P7_MUX_1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "ADI MUX#1",
    },
    {
        /* ADI MUX#2 */
        0,  /* device i2c controller */
        {CFG_I2C_C1_9548SW1, 0},  /* device mux */
        {7, 0},  /* device mux channel */
        CFG_I2C_C1_9548SW1_P7_MUX_2,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "ADI MUX#2",
    },
    {
        /* ADI MUX#3 */
        0,  /* device i2c controller */
        {CFG_I2C_C1_9548SW1, 0},  /* device mux */
        {7, 0},  /* device mux channel */
        CFG_I2C_C1_9548SW1_P7_MUX_3,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "ADI MUX#3",
    },
    {
        /* PEX8618 PCIe switch */
        0,  /* device i2c controller */
        {CFG_I2C_C1_9548SW1, 0},  /* device mux */
        {7, 0},  /* device mux channel */
        CFG_I2C_C1_9548SW1_P7_PEX8618,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PEX8618 PCIe switch",
    },
    {
        /* MK1493-03B PCI clock */
        0,  /* device i2c controller */
        {CFG_I2C_C1_9548SW1, 0},  /* device mux */
        {7, 0},  /* device mux channel */
        CFG_I2C_C1_9548SW1_P7_MK1493,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "MK1493-03B PCI clock",
    },
    {
        /* PCA9506A#1 (U16) */
        1,  /* device i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        CFG_I2C_C2_9506EXP1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA9506A#1 (U16)",
    },
    {
        /* PCA9546A#1 (U19) */
        1,  /* device i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA9546A#1 (U19)",
    },
    {
        /* PCA9548A#2 (U51) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P0_9548SW2,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA9548A#2 (U51)",
    },
    {
        /* SFP+_0 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P0_9548SW2},  /* device mux */
        {0, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P0_9548SW2_P0_SFPP_0,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x2,  /* read register */
        4,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_0",
    },
    {
        /* SFP+_1 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P0_9548SW2},  /* device mux */
        {0, 1},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P0_9548SW2_P1_SFPP_1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x2,  /* read register */
        5,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_1",
    },
    {
        /* SFP+_2 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P0_9548SW2},  /* device mux */
        {0, 2},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P0_9548SW2_P2_SFPP_2,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x2,  /* read register */
        6,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_2",
    },
    {
        /* SFP+_3 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P0_9548SW2},  /* device mux */
        {0, 3},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P0_9548SW2_P3_SFPP_3,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x2,  /* read register */
        7,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_3",
    },
    {
        /* SFP+_4 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P0_9548SW2},  /* device mux */
        {0, 4},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P0_9548SW2_P4_SFPP_4,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x3,  /* read register */
        0,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_4",
    },
    {
        /* SFP+_5 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P0_9548SW2},  /* device mux */
        {0, 5},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P0_9548SW2_P5_SFPP_5,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x3,  /* read register */
        1,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_5",
    },
    {
        /* SFP+_6 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1,
          CFG_I2C_C2_9546SW1_P0_9548SW2},  /* device mux */
        {0, 6},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P0_9548SW2_P6_SFPP_6,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x3,  /* read register */
        2,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_6",
    },
    {
        /* SFP+_7 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P0_9548SW2},  /* device mux */
        {0, 7},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P0_9548SW2_P7_SFPP_7,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x3,  /* read register */
        3,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_7",
    },
    {
        /* PCA9506A#2 (U55) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P0_9506EXP2,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA9506A#2 (U55)",
    },
    {
        /* PCA9548A#3 (U53) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {1, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P1_9548SW3,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA9548A#3 (U53)",
    },
    {
        /* SFP+_8 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P1_9548SW3},  /* device mux */
        {1, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P1_9548SW3_P0_SFPP_8,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x3,  /* read register */
        4,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_8",
    },
    {
        /* SFP+_9 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P1_9548SW3},  /* device mux */
        {1, 1},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P1_9548SW3_P1_SFPP_9,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x3,  /* read register */
        5,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_9",
    },
    {
        /* SFP+_10 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P1_9548SW3},  /* device mux */
        {1, 2},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P1_9548SW3_P2_SFPP_10,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x3,  /* read register */
        6,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_10",
    },
    {
        /* SFP+_11 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P1_9548SW3},  /* device mux */
        {1, 3},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P1_9548SW3_P3_SFPP_11,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x3,  /* read register */
        7,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_11",
    },
    {
        /* SFP+_12 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P1_9548SW3},  /* device mux */
        {1, 4},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P1_9548SW3_P4_SFPP_12,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x4,  /* read register */
        0,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_12",
    },
    {
        /* SFP+_13 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P1_9548SW3},  /* device mux */
        {1, 5},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P1_9548SW3_P5_SFPP_13,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x4,  /* read register */
        1,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_13",
    },
    {
        /* SFP+_14 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P1_9548SW3},  /* device mux */
        {1, 6},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P1_9548SW3_P6_SFPP_14,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x4,  /* read register */
        2,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_14",
    },
    {
        /* SFP+_15 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1,
          CFG_I2C_C2_9546SW1_P1_9548SW3},  /* device mux */
        {1, 7},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P1_9548SW3_P7_SFPP_15,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x4,  /* read register */
        3,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_15",
    },
    {
        /* PCA9506A#3 (U65) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {1, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P1_9506EXP3,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA9506A#3 (U65)",
    },
    {
        /* PCA9506A#4 (U66) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {1, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P1_9506EXP4,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA9506A#4 (U66)",
    },
    {
        /* PCA9506A#5 (U64) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {1, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P1_9506EXP5,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA9506A#5 (U64)",
    },
    {
        /* PCA9548A#4 (U52) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9548SW4,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA9548A#4 (U52)",
    },
    {
        /* SFP+_16 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1,
          CFG_I2C_C2_9546SW1_P2_9548SW4},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9548SW4_P0_SFPP_16,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x4,  /* read register */
        4,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_16",
    },
    {
        /* SFP+_17 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1,
          CFG_I2C_C2_9546SW1_P2_9548SW4},  /* device mux */
        {2, 1},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9548SW4_P1_SFPP_17,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x4,  /* read register */
        5,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_17",
    },
    {
        /* SFP+_18 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P2_9548SW4},  /* device mux */
        {2, 2},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9548SW4_P2_SFPP_18,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x4,  /* read register */
        6,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_18",
    },
    {
        /* SFP+_19 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P2_9548SW4},  /* device mux */
        {2, 3},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9548SW4_P3_SFPP_19,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x4,  /* read register */
        7,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_19",
    },
    {
        /* SFP+_20 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P2_9548SW4},  /* device mux */
        {2, 4},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9548SW4_P4_SFPP_20,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x2,  /* read register */
        4,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_20",
    },
    {
        /* SFP+_21 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P2_9548SW4},  /* device mux */
        {2, 5},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9548SW4_P5_SFPP_21,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x2,  /* read register */
        5,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_21",
    },
    {
        /* SFP+_22 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P2_9548SW4},  /* device mux */
        {2, 6},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9548SW4_P6_SFPP_22,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x2,  /* read register */
        6,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_22",
    },
    {
        /* SFP+_23 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P2_9548SW4},  /* device mux */
        {2, 7},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9548SW4_P7_SFPP_23,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x2,  /* read register */
        7,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_23",
    },
    {
        /* PCA9506A#6 (U207) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA9506A#6 (U207)",
    },
    {
        /* PCA9506A#7 (U210) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA9506A#7 (U210)",
    },
    {
        /* PCA9506A#8 (U208) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP8,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA9506A#8 (U208)",
    },
    {
        /* PCA9548A#5 (U67) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {3, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P3_9548SW5,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA9548A#5 (U67)",
    },
    {
        /* SFP+_24 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P3_9548SW5},  /* device mux */
        {3, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P3_9548SW5_P0_SFPP_24,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x3,  /* read register */
        0,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_24",
    },
    {
        /* SFP+_25 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P3_9548SW5},  /* device mux */
        {3, 1},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P3_9548SW5_P1_SFPP_25,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x3,  /* read register */
        1,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_25",
    },
    {
        /* SFP+_26 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P3_9548SW5},  /* device mux */
        {3, 2},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P3_9548SW5_P2_SFPP_26,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x3,  /* read register */
        2,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_26",
    },
    {
        /* SFP+_27 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P3_9548SW5},  /* device mux */
        {3, 3},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P3_9548SW5_P3_SFPP_27,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x3,  /* read register */
        3,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_27",
    },
    {
        /* SFP+_28 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P3_9548SW5},  /* device mux */
        {3, 4},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P3_9548SW5_P4_SFPP_28,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x3,  /* read register */
        4,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_28",
    },
    {
        /* SFP+_29 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P3_9548SW5},  /* device mux */
        {3, 5},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P3_9548SW5_P5_SFPP_29,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x3,  /* read register */
        5,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_29",
    },
    {
        /* SFP+_30 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P3_9548SW5},  /* device mux */
        {3, 6},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P3_9548SW5_P6_SFPP_30,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x3,  /* read register */
        6,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_30",
    },
    {
        /* SFP+_31 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P3_9548SW5},  /* device mux */
        {3, 7},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P3_9548SW5_P7_SFPP_31,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x3,  /* read register */
        7,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_31",
    },
    {
        /* PCA9546A#2 (U20) */
        1,  /* device i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW2,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA9546A#2 (U20)",
    },
    {
        /* PCA9548A#6 (U68) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW2_P0_9548SW6,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA9548A#6 (U68)",
    },
    {
        /* SFP+_32 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 
          CFG_I2C_C2_9546SW2_P0_9548SW6},  /* device mux */
        {0, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW2_P0_9548SW6_P0_SFPP_32,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x4,  /* read register */
        0,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_32",
    },
    {
        /* SFP+_33 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 
          CFG_I2C_C2_9546SW2_P0_9548SW6},  /* device mux */
        {0, 1},  /* device mux channel */
        CFG_I2C_C2_9546SW2_P0_9548SW6_P1_SFPP_33,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x4,  /* read register */
        1,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_33",
    },
    {
        /* SFP+_34 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 
          CFG_I2C_C2_9546SW2_P0_9548SW6},  /* device mux */
        {0, 2},  /* device mux channel */
        CFG_I2C_C2_9546SW2_P0_9548SW6_P2_SFPP_34,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x4,  /* read register */
        2,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_34",
    },
    {
        /* SFP+_35 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 
          CFG_I2C_C2_9546SW2_P0_9548SW6},  /* device mux */
        {0, 3},  /* device mux channel */
        CFG_I2C_C2_9546SW2_P0_9548SW6_P3_SFPP_35,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x4,  /* read register */
        3,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_35",
    },
    {
        /* SFP+_36 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 
          CFG_I2C_C2_9546SW2_P0_9548SW6},  /* device mux */
        {0, 4},  /* device mux channel */
        CFG_I2C_C2_9546SW2_P0_9548SW6_P4_SFPP_36,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x4,  /* read register */
        4,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_36",
    },
    {
        /* SFP+_37 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 
          CFG_I2C_C2_9546SW2_P0_9548SW6},  /* device mux */
        {0, 5},  /* device mux channel */
        CFG_I2C_C2_9546SW2_P0_9548SW6_P5_SFPP_37,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x4,  /* read register */
        5,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_37",
    },
    {
        /* SFP+_38 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 
          CFG_I2C_C2_9546SW2_P0_9548SW6},  /* device mux */
        {0, 6},  /* device mux channel */
        CFG_I2C_C2_9546SW2_P0_9548SW6_P6_SFPP_38,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x4,  /* read register */
        6,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_38",
    },
    {
        /* SFP+_39 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 
          CFG_I2C_C2_9546SW2_P0_9548SW6},  /* device mux */
        {0, 7},  /* device mux channel */
        CFG_I2C_C2_9546SW2_P0_9548SW6_P7_SFPP_39,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x4,  /* read register */
        7,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_39",
    },
};

#if !defined(CONFIG_PRODUCTION)
/* i2c dev list M1-SFP+ Rev 2 on M1-40 slot */
i2c_dev i2c_m1_sfpp_m1_40_slot_dev_list[] =
{
    {
        /* PCA948A (U8) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 0},  /* device mux */
        {3, 0},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA948A (U8)",
    },
    {
        /* PCA9506A (U7) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA9506A (U7)",
    },
    {
        /* R5H30211 SEEPROM */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 6},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P6_SEEPROM,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &seeprom_read,
        NULL,
        "R5H30211 SEEPROM",
    },
    {
        /* LM75BIM-3 (U6) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 5},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P5_LM75,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "LM75BIM-3 (U6)",
    },
    {
        /* SFP+_0 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 0},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P0_SFPP_1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* io expander addr */
        0x2,  /* read register */
        0,  /* bit offset */  
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_0",
    },
    {
        /* SFP+_1 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 1},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P1_SFPP_0,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* io expander addr */
        0x2,  /* read register */
        1,  /* bit offset */  
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_1",
    },
    {
        /* SFP+_2 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 2},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P2_SFPP_3,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* io expander addr */
        0x2,  /* read register */
        2,  /* bit offset */ 
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_2",
    },
    {
        /* SFP+_3 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 3},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P3_SFPP_2,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* io expander addr */
        0x2,  /* read register */
        3,  /* bit offset */ 
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_3",
    },
};

/* i2c dev list M1-SFP+ Rev 2 on M1-80 slot */
i2c_dev i2c_m1_sfpp_m1_80_slot_dev_list[] =
{
    {
        /* PCA948A (U8) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA948A (U8)",
    },
    {
        /* PCA9506A (U7) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA9506A (U7)",
    },
    {
        /* R5H30211 SEEPROM */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 6},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P6_SEEPROM,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &seeprom_read,
        NULL,
        "R5H30211 SEEPROM",
    },
    {
        /* LM75BIM-3 (U6) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 5},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P5_LM75,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "LM75BIM-3 (U6)",
    },
    {
        /* SFP+_0 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P0_SFPP_1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* io expander addr */
        0x2,  /* read register */
        0,  /* bit offset */ 
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_0",
    },
    {
        /* SFP+_1 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 1},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P1_SFPP_0,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* io expander addr */
        0x2,  /* read register */
        1,  /* bit offset */ 
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_1",
    },
    {
        /* SFP+_2 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 2},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P2_SFPP_3,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* io expander addr */
        0x2,  /* read register */
        2,  /* bit offset */ 
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_2",
    },
    {
        /* SFP+_3 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 3},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P3_SFPP_2,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* io expander addr */
        0x2,  /* read register */
        3,  /* bit offset */ 
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_3",
    },
};

/* i2c dev list M1-SFP+ Rev 1 on M1-40 slot */
i2c_dev i2c_m1_sfpp_m1_40_slot_dev_list_rev1[] =
{
    {
        /* R5H30211 SEEPROM */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 0},  /* device mux */
        {3, 0},  /* device mux channel */
        CFG_I2C_M1_SFPP_SEEPROM_REV1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &seeprom_read,
        NULL,
        "R5H30211 SEEPROM",
    },
    {
        /* PCA948A (U8) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 0},  /* device mux */
        {3, 0},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA948A (U8)",
    },
    {
        /* PCA9506A (U7) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA9506A (U7)",
    },
    {
        /* ADT7410Z (U13) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 5},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P5_ADT7410_REV1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "ADT7410Z (U13)",
    },
    {
        /* SFP+_0 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 0},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P0_SFPP_1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* io expander addr */
        0x2,  /* read register */
        0,  /* bit offset */ 
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_0",
    },
    {
        /* SFP+_1 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 1},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P1_SFPP_0,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* io expander addr */
        0x2,  /* read register */
        1,  /* bit offset */ 
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_1",
    },
    {
        /* SFP+_2 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 2},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P2_SFPP_3,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* io expander addr */
        0x2,  /* read register */
        2,  /* bit offset */ 
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_2",
    },
    {
        /* SFP+_3 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 3},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P3_SFPP_2,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* io expander addr */
        0x2,  /* read register */
        3,  /* bit offset */ 
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_3",
    },
};

/* i2c dev list M1-SFP+ Rev 1 on M1-80 slot */
i2c_dev i2c_m1_sfpp_m1_80_slot_dev_list_rev1[] =
{
    {
        /* R5H30211 SEEPROM */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_M1_SFPP_SEEPROM_REV1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &seeprom_read,
        NULL,
        "R5H30211 SEEPROM",
    },
    {
        /* PCA948A (U8) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA948A (U8)",
    },
    {
        /* PCA9506A (U7) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA9506A (U7)",
    },
    {
        /* ADT7410Z (U13) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 5},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P5_ADT7410_REV1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "ADT7410Z (U13)",
    },
    {
        /* SFP+_0 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P0_SFPP_1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* io expander addr */
        0x2,  /* read register */
        0,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_0",
    },
    {
        /* SFP+_1 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 1},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P1_SFPP_0,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* io expander addr */
        0x2,  /* read register */
        1,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_1",
    },
    {
        /* SFP+_2 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 2},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P2_SFPP_3,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* io expander addr */
        0x2,  /* read register */
        2,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_2",
    },
    {
        /* SFP+_3 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 3},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P3_SFPP_2,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* io expander addr */
        0x2,  /* read register */
        3,  /* bit offset */
        1,  /* normal/inverted */
        &generic_read,
        NULL,
        "SFP+_3",
    },
};

/* i2c dev list M2-LBK Rev 2 */
i2c_dev i2c_m2_lbk_dev_list[] =
{
    {
        /* PCA948A (U3) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 0},  /* device mux */
        {1, 0},  /* device mux channel */
        CFG_I2C_M2_LBK_9548SW1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA948A (U3)",
    },
    {
        /* PCA9506A (U5) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M2_LBK_9548SW1},  /* device mux */
        {1, 0},  /* device mux channel */
        CFG_I2C_M2_LBK_9548SW1_P0_9506EXP1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA9506A (U5)",
    },
    {
        /* R5H30211 SEEPROM */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M2_LBK_9548SW1},  /* device mux */
        {1, 1},  /* device mux channel */
        CFG_I2C_M2_LBK_9548SW1_P1_SEEPROM,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &seeprom_read,
        NULL,
        "R5H30211 SEEPROM",
    },
    {
        /* LM75BIM-3 (U6) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M2_LBK_9548SW1},  /* device mux */
        {1, 2},  /* device mux channel */
        CFG_I2C_M2_LBK_9548SW1_P2_LM75,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "LM75BIM-3 (U6)",
    },
};

/* i2c dev list M2-LBK Rev 1 */
i2c_dev i2c_m2_lbk_dev_list_rev1[] =
{
    {
        /* R5H30211 SEEPROM */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 0},  /* device mux */
        {1, 0},  /* device mux channel */
        CFG_I2C_M2_LBK_SEEPROM_REV1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &seeprom_read,
        NULL,
        "R5H30211 SEEPROM",
    },
    {
        /* PCA943A (U3) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 0},  /* device mux */
        {1, 0},  /* device mux channel */
        CFG_I2C_M2_LBK_9543SW1_REV1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA943A (U3)",
    },
    {
        /* PCA9506A (U5) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M2_LBK_9543SW1_REV1},  /* device mux */
        {1, 0},  /* device mux channel */
        CFG_I2C_M2_LBK_9543SW1_P0_9506EXP1_REV1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "PCA9506A (U5)",
    },
    {
        /* ADT7410Z (U6) */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M2_LBK_9543SW1_REV1},  /* device mux */
        {1, 0},  /* device mux channel */
        CFG_I2C_M2_LBK_9548SW1_P0_ADT7410_REV1,  /* device i2c addr */
        0,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        0,  /* present check */
        0,  /* present check i2c controller */
        {0, 0},  /* device mux */
        {0, 0},  /* device mux channel */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &generic_read,
        NULL,
        "ADT7410Z (U6)",
    },
};


int temp_read (uint8_t addr, uint16_t offset, uint16_t len);

/* i2c dev list  T40 SFP+ temperature */
i2c_dev i2c_t40_temp_list[] =
{
    {
        /* SFP+_0 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P0_9548SW2},  /* device mux */
        {0, 0},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x2,  /* read register */
        4,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_0",
    },
    {
        /* SFP+_1 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P0_9548SW2},  /* device mux */
        {0, 1},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x2,  /* read register */
        5,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_1",
    },
    {
        /* SFP+_2 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P0_9548SW2},  /* device mux */
        {0, 2},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x2,  /* read register */
        6,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_2",
    },
    {
        /* SFP+_3 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P0_9548SW2},  /* device mux */
        {0, 3},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x2,  /* read register */
        7,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_3",
    },
    {
        /* SFP+_4 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P0_9548SW2},  /* device mux */
        {0, 4},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x3,  /* read register */
        0,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_4",
    },
    {
        /* SFP+_5 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P0_9548SW2},  /* device mux */
        {0, 5},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x3,  /* read register */
        1,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_5",
    },
    {
        /* SFP+_6 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1,
          CFG_I2C_C2_9546SW1_P0_9548SW2},  /* device mux */
        {0, 6},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x3,  /* read register */
        2,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_6",
    },
    {
        /* SFP+_7 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P0_9548SW2},  /* device mux */
        {0, 7},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x3,  /* read register */
        3,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_7",
    },
    {
        /* SFP+_8 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P1_9548SW3},  /* device mux */
        {1, 0},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x3,  /* read register */
        4,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_8",
    },
    {
        /* SFP+_9 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P1_9548SW3},  /* device mux */
        {1, 1},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x3,  /* read register */
        5,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_9",
    },
    {
        /* SFP+_10 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P1_9548SW3},  /* device mux */
        {1, 2},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x3,  /* read register */
        6,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_10",
    },
    {
        /* SFP+_11 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P1_9548SW3},  /* device mux */
        {1, 3},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x3,  /* read register */
        7,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_11",
    },
    {
        /* SFP+_12 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P1_9548SW3},  /* device mux */
        {1, 4},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x4,  /* read register */
        0,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_12",
    },
    {
        /* SFP+_13 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P1_9548SW3},  /* device mux */
        {1, 5},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x4,  /* read register */
        1,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_13",
    },
    {
        /* SFP+_14 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P1_9548SW3},  /* device mux */
        {1, 6},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x4,  /* read register */
        2,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_14",
    },
    {
        /* SFP+_15 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1,
          CFG_I2C_C2_9546SW1_P1_9548SW3},  /* device mux */
        {1, 7},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x4,  /* read register */
        3,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_15",
    },
    {
        /* SFP+_16 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1,
          CFG_I2C_C2_9546SW1_P2_9548SW4},  /* device mux */
        {2, 0},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x4,  /* read register */
        4,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_16",
    },
    {
        /* SFP+_17 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1,
          CFG_I2C_C2_9546SW1_P2_9548SW4},  /* device mux */
        {2, 1},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x4,  /* read register */
        5,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_17",
    },
    {
        /* SFP+_18 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P2_9548SW4},  /* device mux */
        {2, 2},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x4,  /* read register */
        6,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_18",
    },
    {
        /* SFP+_19 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P2_9548SW4},  /* device mux */
        {2, 3},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP6,  /* io expander addr */
        0x4,  /* read register */
        7,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_19",
    },
    {
        /* SFP+_20 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P2_9548SW4},  /* device mux */
        {2, 4},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x2,  /* read register */
        4,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_20",
    },
    {
        /* SFP+_21 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P2_9548SW4},  /* device mux */
        {2, 5},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x2,  /* read register */
        5,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_21",
    },
    {
        /* SFP+_22 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P2_9548SW4},  /* device mux */
        {2, 6},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x2,  /* read register */
        6,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_22",
    },
    {
        /* SFP+_23 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P2_9548SW4},  /* device mux */
        {2, 7},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x2,  /* read register */
        7,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_23",
    },
    {
        /* SFP+_24 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P3_9548SW5},  /* device mux */
        {3, 0},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x3,  /* read register */
        0,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_24",
    },
    {
        /* SFP+_25 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P3_9548SW5},  /* device mux */
        {3, 1},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x3,  /* read register */
        1,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_25",
    },
    {
        /* SFP+_26 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P3_9548SW5},  /* device mux */
        {3, 2},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x3,  /* read register */
        2,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_26",
    },
    {
        /* SFP+_27 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P3_9548SW5},  /* device mux */
        {3, 3},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x3,  /* read register */
        3,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_27",
    },
    {
        /* SFP+_28 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P3_9548SW5},  /* device mux */
        {3, 4},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x3,  /* read register */
        4,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_28",
    },
    {
        /* SFP+_29 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P3_9548SW5},  /* device mux */
        {3, 5},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x3,  /* read register */
        5,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_29",
    },
    {
        /* SFP+_30 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P3_9548SW5},  /* device mux */
        {3, 6},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x3,  /* read register */
        6,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_30",
    },
    {
        /* SFP+_31 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW1, 
          CFG_I2C_C2_9546SW1_P3_9548SW5},  /* device mux */
        {3, 7},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x3,  /* read register */
        7,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_31",
    },
    {
        /* SFP+_32 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 
          CFG_I2C_C2_9546SW2_P0_9548SW6},  /* device mux */
        {0, 0},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x4,  /* read register */
        0,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_32",
    },
    {
        /* SFP+_33 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 
          CFG_I2C_C2_9546SW2_P0_9548SW6},  /* device mux */
        {0, 1},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x4,  /* read register */
        1,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_33",
    },
    {
        /* SFP+_34 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 
          CFG_I2C_C2_9546SW2_P0_9548SW6},  /* device mux */
        {0, 2},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x4,  /* read register */
        2,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_34",
    },
    {
        /* SFP+_35 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 
          CFG_I2C_C2_9546SW2_P0_9548SW6},  /* device mux */
        {0, 3},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x4,  /* read register */
        3,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_35",
    },
    {
        /* SFP+_36 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 
          CFG_I2C_C2_9546SW2_P0_9548SW6},  /* device mux */
        {0, 4},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x4,  /* read register */
        4,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_36",
    },
    {
        /* SFP+_37 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 
          CFG_I2C_C2_9546SW2_P0_9548SW6},  /* device mux */
        {0, 5},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x4,  /* read register */
        5,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_37",
    },
    {
        /* SFP+_38 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 
          CFG_I2C_C2_9546SW2_P0_9548SW6},  /* device mux */
        {0, 6},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x4,  /* read register */
        6,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_38",
    },
    {
        /* SFP+_39 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, 
          CFG_I2C_C2_9546SW2_P0_9548SW6},  /* device mux */
        {0, 7},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW1, 0},  /* device mux */
        {2, 0},  /* device mux channel */
        CFG_I2C_C2_9546SW1_P2_9506EXP7,  /* io expander addr */
        0x4,  /* read register */
        7,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_39",
    },
};

/* i2c dev list M1-SFP+ Rev 2 on M1-40 slot temperature */
i2c_dev i2c_m1_sfpp_m1_40_slot_temp_list[] =
{
    {
        /* SFP+_0 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 0},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* io expander addr */
        0x2,  /* read register */
        0,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_0",
    },
    {
        /* SFP+_1 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 1},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* io expander addr */
        0x2,  /* read register */
        1,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_1",
    },
    {
        /* SFP+_2 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 2},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* io expander addr */
        0x2,  /* read register */
        2,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_2",
    },
    {
        /* SFP+_3 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 3},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {3, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* io expander addr */
        0x2,  /* read register */
        3,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_3",
    },
};


/* i2c dev list M1-SFP+ Rev 2 on M1-80 slot temperature list */
i2c_dev i2c_m1_sfpp_m1_80_slot_temp_list[] =
{
    {
        /* SFP+_0 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 0},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* io expander addr */
        0x2,  /* read register */
        0,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_0",
    },
    {
        /* SFP+_1 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 1},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* io expander addr */
        0x2,  /* read register */
        1,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_1",
    },
    {
        /* SFP+_2 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 2},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* io expander addr */
        0x2,  /* read register */
        2,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_2",
    },
    {
        /* SFP+_3 */
        1,  /* device i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 3},  /* device mux channel */
        0x51,  /* device i2c addr */
        0x60,  /* device read offset */
        1,  /* device read length */
        0,  /* i2c speed */
        1,  /* present check */
        1,  /* present check i2c controller */
        {CFG_I2C_C2_9546SW2, CFG_I2C_M1_SFPP_9548SW1},  /* device mux */
        {2, 4},  /* device mux channel */
        CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1,  /* io expander addr */
        0x2,  /* read register */
        3,  /* bit offset */
        1,  /* normal/inverted */
        &temp_read,
        NULL,
        "SFP+_3",
    },
};

int
temp_read (uint8_t addr, uint16_t offset, uint16_t len)
{
    uint8_t temp;

    i2c_read(addr, offset, 1, &temp, 1);

    return ((int8_t)temp);
}

#endif /* CONFIG_PRODUCTION */

int
generic_read (uint8_t addr, uint16_t offset, uint16_t len)
{
    uint8_t temp[10];
    int alen = 1, dlen = 1;
    int res = DEV_PASS;

    if (len > 10)
        dlen = 10;
    if (offset == 0xFFFF) {
        if (i2c_read_noaddr(addr, temp, dlen)) 
            return (DEV_FAIL);
    }
    else {
        if (offset > 255)
            alen = 2;
        if (i2c_read(addr, offset, alen, temp, dlen))
            res = DEV_FAIL;
    }

    return (res);
}

int
seeprom_read (uint8_t addr, uint16_t offset, uint16_t len)
{
    uint8_t temp[10];
    int alen = 1, dlen = 1;
    int res = DEV_PASS;
    
    if (len > 10)
        dlen = 10;

    if (offset == 0xFFFF) {
        if (i2c_read_norsta(addr, 0, 1, temp, dlen)) 
            return (DEV_FAIL);
    }
    else {
        if (offset > 255)
            alen = 2;
        if (i2c_read_norsta(addr, offset, alen, temp, dlen))
            res = DEV_FAIL;
    }

    udelay(1000);

    return (res);
}

int 
fan_present (void)
{
    uint16_t epldreg = 0;

    epld_reg_read(EPLD_LAST_RESET, &epldreg);
    if (EPLD_FAN_PRESENT & epldreg) 
        return (1);
    else
        return (0);
}

static int 
i2c_post_test_one (struct i2c_test_struct* i2c_dev_detect)
{
    int present = 0, fail = 0, cur_ctrl, i, j, k;
    uint8_t ch, temp = 0;

    /* relocate function pointers from flash to ram */
    for (i = 0; i < i2c_dev_detect->size; i++) {
        if (i2c_dev_detect->i2c_list[i].dev_op)
            if (i2c_dev_detect->i2c_list[i].dev_op >= CFG_FLASH_BASE)
                i2c_dev_detect->i2c_list[i].dev_op = 
                i2c_dev_detect->i2c_list[i].dev_op + gd->reloc_off;
        if (i2c_dev_detect->i2c_list[i].present_op)
            if (i2c_dev_detect->i2c_list[i].dev_op >= CFG_FLASH_BASE)
                i2c_dev_detect->i2c_list[i].present_op = 
                i2c_dev_detect->i2c_list[i].present_op + gd->reloc_off;
    }
  
    i2c_post = 1;
    cur_ctrl = current_i2c_controller();

    for (i = 0; i < i2c_dev_detect->size; i++) {
        if (i2c_dev_detect->i2c_list[i].chk_present) {
            if (i2c_dev_detect->i2c_list[i].present_op) {
                present = i2c_dev_detect->i2c_list[i].present_op();
            }
            else {
                present = 0;
                i2c_controller(i2c_dev_detect->i2c_list[i].present_ctrl);
                k = 0;
                for (j = 0; j < MAX_MUX; j++) {
                    if (i2c_dev_detect->i2c_list[i].present_muxaddr[j] == 0)
                        break;
                    ch = 1 << i2c_dev_detect->i2c_list[i].present_muxchan[j];
                    i2c_write(i2c_dev_detect->i2c_list[i].present_muxaddr[j], 
                        0, 0, &ch, 1); 
                    k++;
                }
                i2c_read(i2c_dev_detect->i2c_list[i].io_addr, 
                    i2c_dev_detect->i2c_list[i].io_reg, 1, &temp, 1);
                if (i2c_dev_detect->i2c_list[i].io_data) {
                    temp = ~temp;
                }
                present = temp & (1 << i2c_dev_detect->i2c_list[i].io_bit);
                if (k) {
                    ch = 0;
                    for (j = k-1; j >= 0; j--) {
                        i2c_write(i2c_dev_detect->i2c_list[i].present_muxaddr[j],
                            0, 0, &ch, 1);
                    }
                }
            }
        }
        else
            present = 1;

        if (present) {
            i2c_controller(i2c_dev_detect->i2c_list[i].dev_ctrl);
            k = 0;
            for (j = 0; j < MAX_MUX; j++) {
                if (i2c_dev_detect->i2c_list[i].dev_muxaddr[j] == 0)
                    break;
                ch = 1 << i2c_dev_detect->i2c_list[i].dev_muxchan[j];
                i2c_write(i2c_dev_detect->i2c_list[i].dev_muxaddr[j], 
                    0, 0, &ch, 1);                 
                k++;
            }

            i2c_dev_detect->result[i] = i2c_dev_detect->i2c_list[i].dev_op(
                i2c_dev_detect->i2c_list[i].dev_addr, 
                i2c_dev_detect->i2c_list[i].dev_offset, 
                i2c_dev_detect->i2c_list[i].dev_len);
            if (k) {
                ch = 0;
                for (j = k-1; j >= 0; j--) {
                    i2c_write(i2c_dev_detect->i2c_list[i].dev_muxaddr[j],
                        0, 0, &ch, 1);
                }
            }
            if (i2c_dev_detect->result[i] == DEV_FAIL)
                fail = 1;
        }
        else
            i2c_dev_detect->result[i] = 0;
    }
    i2c_controller(cur_ctrl);
    i2c_post = 0;
   
    if (fail)
        return (-1);
    return (0);

}
 
int 
i2c_post_test (int flags)
{
    struct i2c_test_struct i2c_dev_test[CFG_EX4500_LAST_CARD];
    struct i2c_dev_struct *i2c_dev_detect;
    uint16_t mid = 0;
#if !defined(CONFIG_PRODUCTION)
    int rev;
    uint8_t addr;
#endif  /* CONFIG_PRODUCTION */
    int fail = 0, result[CFG_EX4500_LAST_CARD];
    int i, j, k;
    int i2c_verbose = flags & POST_DEBUG;
    unsigned char tmp[30], tmp1[10];

    memset(i2c_dev_test, 0, 
        CFG_EX4500_LAST_CARD * sizeof(struct i2c_test_struct));

    /* T40.  T20 ? */
    i2c_dev_test[CFG_EX4500_T40_INDEX].i2c_list = i2c_t40_dev_list;
    i2c_dev_test[CFG_EX4500_T40_INDEX].size = 
        sizeof(i2c_t40_dev_list) / sizeof(struct i2c_dev_struct);
    i2c_dev_test[CFG_EX4500_T40_INDEX].present = 1;
    strcpy(i2c_dev_test[CFG_EX4500_T40_INDEX].name, 
        id_to_name(I2C_ID_JUNIPER_EX4500_40F));

#if !defined(CONFIG_PRODUCTION)
    if (m2_present()) {
        if (m2_id(&rev, &mid, &addr) == 0) {
            if (rev == 1) {
                switch (mid) {
                    case I2C_ID_TSUNAMI_M2_LOOPBACK_PIC:
                        i2c_dev_test[CFG_EX4500_M2_INDEX].i2c_list = 
                            i2c_m2_lbk_dev_list_rev1;
                        i2c_dev_test[CFG_EX4500_M2_INDEX].size = 
                            sizeof(i2c_m2_lbk_dev_list_rev1) / sizeof(struct i2c_dev_struct);
                        i2c_dev_test[CFG_EX4500_M2_INDEX].present = 1;
                        strcpy(i2c_dev_test[CFG_EX4500_M2_INDEX].name, 
                            id_to_name(I2C_ID_TSUNAMI_M2_LOOPBACK_PIC));
                        break;
                        
                    default:
                        break;
                }
            }
            else if (rev == 2) {
                switch (mid) {
                    case I2C_ID_TSUNAMI_M2_LOOPBACK_PIC:
                        i2c_dev_test[CFG_EX4500_M2_INDEX].i2c_list = 
                            i2c_m2_lbk_dev_list;
                        i2c_dev_test[CFG_EX4500_M2_INDEX].size = 
                            sizeof(i2c_m2_lbk_dev_list) / sizeof(struct i2c_dev_struct);
                        i2c_dev_test[CFG_EX4500_M2_INDEX].present = 1;
                        strcpy(i2c_dev_test[CFG_EX4500_M2_INDEX].name, 
                            id_to_name(I2C_ID_TSUNAMI_M2_LOOPBACK_PIC));
                        break;
                        
                    default:
                        break;
                }
            }
        }
    }

    if (m1_40_present()) {
        if (m1_40_id(&rev, &mid, &addr) == 0) {
            if (rev == 1) {
                switch (mid) {
                    case I2C_ID_TSUNAMI_UPLINK_SFPPLUS_4PORT_PIC:
                        i2c_dev_test[CFG_EX4500_M1_40_INDEX].i2c_list = 
                            i2c_m1_sfpp_m1_40_slot_dev_list_rev1;
                        i2c_dev_test[CFG_EX4500_M1_40_INDEX].size = 
                            sizeof(i2c_m1_sfpp_m1_40_slot_dev_list_rev1) / 
                            sizeof(struct i2c_dev_struct);
                        i2c_dev_test[CFG_EX4500_M1_40_INDEX].present = 1;
                        strcpy(i2c_dev_test[CFG_EX4500_M1_40_INDEX].name, 
                          id_to_name(I2C_ID_TSUNAMI_UPLINK_SFPPLUS_4PORT_PIC));
                        break;
                        
                    default:
                        break;
                }
            }
            else if (rev == 2) {
                switch (mid) {
                    case I2C_ID_TSUNAMI_UPLINK_SFPPLUS_4PORT_PIC:
                        i2c_dev_test[CFG_EX4500_M1_40_INDEX].i2c_list = 
                            i2c_m1_sfpp_m1_40_slot_dev_list;
                        i2c_dev_test[CFG_EX4500_M1_40_INDEX].size = 
                            sizeof(i2c_m1_sfpp_m1_40_slot_dev_list) / 
                            sizeof(struct i2c_dev_struct);
                        i2c_dev_test[CFG_EX4500_M1_40_INDEX].present = 1;
                        strcpy(i2c_dev_test[CFG_EX4500_M1_40_INDEX].name, 
                          id_to_name(I2C_ID_TSUNAMI_UPLINK_SFPPLUS_4PORT_PIC));
                        break;
                        
                    default:
                        break;
                }
            }
        }
    }

    if (m1_80_present()) {
        if (m1_80_id(&rev, &mid, &addr) == 0) {
            if (rev == 1) {
                switch (mid) {
                    case I2C_ID_TSUNAMI_UPLINK_SFPPLUS_4PORT_PIC:
                        i2c_dev_test[CFG_EX4500_M1_80_INDEX].i2c_list = 
                            i2c_m1_sfpp_m1_80_slot_dev_list_rev1;
                        i2c_dev_test[CFG_EX4500_M1_80_INDEX].size = 
                            sizeof(i2c_m1_sfpp_m1_80_slot_dev_list_rev1) / 
                            sizeof(struct i2c_dev_struct);
                        i2c_dev_test[CFG_EX4500_M1_80_INDEX].present = 1;
                        strcpy(i2c_dev_test[CFG_EX4500_M1_80_INDEX].name, 
                          id_to_name(I2C_ID_TSUNAMI_UPLINK_SFPPLUS_4PORT_PIC));
                        break;
                        
                    default:
                        break;
                }
            }
            else if (rev == 2) {
                switch (mid) {
                    case I2C_ID_TSUNAMI_UPLINK_SFPPLUS_4PORT_PIC:
                        i2c_dev_test[CFG_EX4500_M1_80_INDEX].i2c_list = 
                            i2c_m1_sfpp_m1_80_slot_dev_list;
                        i2c_dev_test[CFG_EX4500_M1_80_INDEX].size = 
                            sizeof(i2c_m1_sfpp_m1_80_slot_dev_list) / 
                            sizeof(struct i2c_dev_struct);
                        i2c_dev_test[CFG_EX4500_M1_80_INDEX].present = 1;
                        strcpy(i2c_dev_test[CFG_EX4500_M1_80_INDEX].name, 
                          id_to_name(I2C_ID_TSUNAMI_UPLINK_SFPPLUS_4PORT_PIC));
                        break;
                        
                    default:
                        break;
                }
            }
        }
    }
#endif  /* CONFIG_PRODUCTION */

    for (i = 0; i < CFG_EX4500_LAST_CARD; i++) {
        if (i2c_dev_test[i].present) {
            if (result[i] = i2c_post_test_one(&i2c_dev_test[i]))
                fail++;
        }
    }

    if (fail) {
        post_log("-- I2C test                             FAILED @\n");
        for (k = 0; k < CFG_EX4500_LAST_CARD; k++) {
            if ((i2c_dev_test[k].present)) {
                post_log(" - %s (%s)\n", slot_to_name(mid, k), i2c_dev_test[k].name);
                i2c_dev_detect = i2c_dev_test[k].i2c_list;

                for (i = 0; i < i2c_dev_test[k].size; i++) {
                    sprintf(tmp, "C%1d ", i2c_dev_detect[i].dev_ctrl);
                    tmp1[0] = 0;
                    for (j = 0; j < MAX_MUX; j++) {
                        if (i2c_dev_detect[i].dev_muxaddr[j] == 0) {
                            break;
                        }
                        sprintf(tmp1, "M%02xP%1d/", i2c_dev_detect[i].dev_muxaddr[j],
                            i2c_dev_detect[i].dev_muxchan[j]);
                        strcat(tmp, tmp1);
                    }

                    tmp[strlen(tmp) - 1] = 0;
                    sprintf(tmp1, " %02x", i2c_dev_detect[i].dev_addr);
                    strcat(tmp, tmp1);

                    if (i2c_dev_test[k].result[i] == DEV_FAIL) {
                        post_log (" > I2C: %-11s %-20s "
						  "%-24s %6s @\n",
						  "Not Found",
						  i2c_dev_detect[i].name,
						  tmp,
						  i2c_speed_name[i2c_dev_detect[i].dev_speed]);
                    }
                    else {
                        if (i2c_verbose) {
                            post_log (" > I2C: %-11s %-20s "
						      "%-24s %6s\n",
						      (i2c_dev_test[k].result[i] == DEV_PASS) ? 
						      "Found" : "Not Present",
						      i2c_dev_detect[i].name,
						      tmp,
						      i2c_speed_name[i2c_dev_detect[i].dev_speed]);
                        }
                    }
                }
            }
        }
    }
    else {
        post_log("-- I2C test                             PASSED\n");
        if (i2c_verbose) {
            for (k = 0; k < CFG_EX4500_LAST_CARD; k++) {
                if ((i2c_dev_test[k].present)) {
                    post_log(" - %s (%s)\n", slot_to_name(mid, k), i2c_dev_test[k].name);
                    i2c_dev_detect = i2c_dev_test[k].i2c_list;

                    for (i = 0; i < i2c_dev_test[k].size; i++) {
                        sprintf(tmp, "C%1d ", i2c_dev_detect[i].dev_ctrl);
                        tmp1[0] = 0;
                        for (j = 0; j < MAX_MUX; j++) {
                            if (i2c_dev_detect[i].dev_muxaddr[j] == 0) {
                                break;
                            }
                            sprintf(tmp1, "M%02xP%1d/", i2c_dev_detect[i].dev_muxaddr[j],
                                i2c_dev_detect[i].dev_muxchan[j]);
                            strcat(tmp, tmp1);
                        }

                        tmp[strlen(tmp) - 1] = 0;
                        sprintf(tmp1, " %02x", i2c_dev_detect[i].dev_addr);
                        strcat(tmp, tmp1);

                        post_log (" > I2C: %-11s %-20s "
						  "%-24s %6s\n",
						  (i2c_dev_test[k].result[i] == DEV_PASS) ? 
						  "Found" : "Not Present",
						  i2c_dev_detect[i].name,
						  tmp,
						  i2c_speed_name[i2c_dev_detect[i].dev_speed]);
                    }                    
                }
            }
        }
    }
    
    if (fail)
        return (-1);
    return (0);

}

#if !defined(CONFIG_PRODUCTION)
 
static void
i2c_sfp_temp_one (struct i2c_test_struct* i2c_dev_detect)
{
    int present = 0, cur_ctrl, i, j, k;
    uint8_t ch, temp = 0;    

    /* relocate function pointers from flash to ram */
    for (i = 0; i < i2c_dev_detect->size; i++) {
        if (i2c_dev_detect->i2c_list[i].dev_op)
            if (i2c_dev_detect->i2c_list[i].dev_op >= CFG_FLASH_BASE)
                i2c_dev_detect->i2c_list[i].dev_op = 
                i2c_dev_detect->i2c_list[i].dev_op + gd->reloc_off;
        if (i2c_dev_detect->i2c_list[i].present_op)
            if (i2c_dev_detect->i2c_list[i].dev_op >= CFG_FLASH_BASE)
                i2c_dev_detect->i2c_list[i].present_op = 
                i2c_dev_detect->i2c_list[i].present_op + gd->reloc_off;
    }
  
    i2c_post = 1;
    cur_ctrl = current_i2c_controller();

    for (i = 0; i < i2c_dev_detect->size; i++) {
        if (i2c_dev_detect->i2c_list[i].chk_present) {
            if (i2c_dev_detect->i2c_list[i].present_op) {
                present = i2c_dev_detect->i2c_list[i].present_op();
            }
            else {
                present = 0;
                i2c_controller(i2c_dev_detect->i2c_list[i].present_ctrl);
                k = 0;
                for (j = 0; j < MAX_MUX; j++) {
                    if (i2c_dev_detect->i2c_list[i].present_muxaddr[j] == 0)
                        break;
                    ch = 1 << i2c_dev_detect->i2c_list[i].present_muxchan[j];
                    i2c_write(i2c_dev_detect->i2c_list[i].present_muxaddr[j], 
                        0, 0, &ch, 1); 
                    k++;
                }
                i2c_read(i2c_dev_detect->i2c_list[i].io_addr, 
                    i2c_dev_detect->i2c_list[i].io_reg, 1, &temp, 1);
                if (i2c_dev_detect->i2c_list[i].io_data) {
                    temp = ~temp;
                }
                present = temp & (1 << i2c_dev_detect->i2c_list[i].io_bit);
                if (k) {
                    ch = 0;
                    for (j = k-1; j >= 0; j--) {
                        i2c_write(i2c_dev_detect->i2c_list[i].present_muxaddr[j],
                            0, 0, &ch, 1);
                    }
                }
            }
        }
        else
            present = 1;

        if (present) {
            i2c_controller(i2c_dev_detect->i2c_list[i].dev_ctrl);
            k = 0;
            for (j = 0; j < MAX_MUX; j++) {
                if (i2c_dev_detect->i2c_list[i].dev_muxaddr[j] == 0)
                    break;
                ch = 1 << i2c_dev_detect->i2c_list[i].dev_muxchan[j];
                i2c_write(i2c_dev_detect->i2c_list[i].dev_muxaddr[j], 
                    0, 0, &ch, 1);                 
                k++;
            }
            i2c_fdr(i2c_dev_fdr[i2c_dev_detect->i2c_list[i].dev_speed], 
                CFG_I2C_SLAVE);
            i2c_dev_detect->result[i] = i2c_dev_detect->i2c_list[i].dev_op(
                i2c_dev_detect->i2c_list[i].dev_addr, 
                i2c_dev_detect->i2c_list[i].dev_offset, 
                i2c_dev_detect->i2c_list[i].dev_len);
            i2c_fdr(i2c_dev_fdr[0], CFG_I2C_SLAVE);
            if (k) {
                ch = 0;
                for (j = k-1; j >= 0; j--) {
                    i2c_write(i2c_dev_detect->i2c_list[i].dev_muxaddr[j],
                        0, 0, &ch, 1);
                }
            }
        }
        else
            i2c_dev_detect->result[i] = -1;
    }
    i2c_controller(cur_ctrl);
    i2c_post = 0;
   
}

void 
i2c_sfp_temp (void)
{
    struct i2c_test_struct i2c_dev_test[CFG_EX4500_LAST_CARD];
    struct i2c_dev_struct *i2c_dev_detect;
    uint16_t mid = 0;
    int rev;
    uint8_t addr;
    int i, k;

    memset(i2c_dev_test, 0, 
        CFG_EX4500_LAST_CARD * sizeof(struct i2c_test_struct));

    /* T40.  T20 ? */
    i2c_dev_test[CFG_EX4500_T40_INDEX].i2c_list = i2c_t40_temp_list;
    i2c_dev_test[CFG_EX4500_T40_INDEX].size = 
        sizeof(i2c_t40_temp_list) / sizeof(struct i2c_dev_struct);
    i2c_dev_test[CFG_EX4500_T40_INDEX].present = 1;
    strcpy(i2c_dev_test[CFG_EX4500_T40_INDEX].name, 
        id_to_name(I2C_ID_JUNIPER_EX4500_40F));

    if (m1_40_present()) {
        if (m1_40_id(&rev, &mid, &addr) == 0) {
            if (rev == 2) {
                switch (mid) {
                    case I2C_ID_TSUNAMI_UPLINK_SFPPLUS_4PORT_PIC:
                        i2c_dev_test[CFG_EX4500_M1_40_INDEX].i2c_list = 
                            i2c_m1_sfpp_m1_40_slot_temp_list;
                        i2c_dev_test[CFG_EX4500_M1_40_INDEX].size = 
                            sizeof(i2c_m1_sfpp_m1_40_slot_temp_list) / 
                            sizeof(struct i2c_dev_struct);
                        i2c_dev_test[CFG_EX4500_M1_40_INDEX].present = 1;
                        strcpy(i2c_dev_test[CFG_EX4500_M1_40_INDEX].name, 
                          id_to_name(I2C_ID_TSUNAMI_UPLINK_SFPPLUS_4PORT_PIC));
                        break;
                        
                    default:
                        break;
                }
            }
        }
    }

    if (m1_80_present()) {
        if (m1_80_id(&rev, &mid, &addr) == 0) {
            if (rev == 2) {
                switch (mid) {
                    case I2C_ID_TSUNAMI_UPLINK_SFPPLUS_4PORT_PIC:
                        i2c_dev_test[CFG_EX4500_M1_80_INDEX].i2c_list = 
                            i2c_m1_sfpp_m1_80_slot_temp_list;
                        i2c_dev_test[CFG_EX4500_M1_80_INDEX].size = 
                            sizeof(i2c_m1_sfpp_m1_80_slot_temp_list) / 
                            sizeof(struct i2c_dev_struct);
                        i2c_dev_test[CFG_EX4500_M1_80_INDEX].present = 1;
                        strcpy(i2c_dev_test[CFG_EX4500_M1_80_INDEX].name, 
                          id_to_name(I2C_ID_TSUNAMI_UPLINK_SFPPLUS_4PORT_PIC));
                        break;
                        
                    default:
                        break;
                }
            }
        }
    }

    for (i = 0; i < CFG_EX4500_LAST_CARD; i++) {
        if (i2c_dev_test[i].present) {
            i2c_sfp_temp_one(&i2c_dev_test[i]);
        }
    }

    for (k = 0; k < CFG_EX4500_LAST_CARD; k++) {
        if ((i2c_dev_test[k].present)) {
            printf ("%s (%s) transceiver temperature:\n", slot_to_name(0, k), 
                    i2c_dev_test[k].name);
            i2c_dev_detect = i2c_dev_test[k].i2c_list;

            for (i = 0; i < i2c_dev_test[k].size; i++) {
                if (i2c_dev_test[k].result[i] != -1) {
                    printf ("  %-8s = %4d\n", 
                        i2c_dev_detect[i].name, i2c_dev_test[k].result[i]);
                }
            }
            printf("\n");
        }
    }
}
#endif /* CONFIG_PRODUCTION */

#endif /* CONFIG_POST & CFG_POST_I2C */
#endif /* CONFIG_POST */
