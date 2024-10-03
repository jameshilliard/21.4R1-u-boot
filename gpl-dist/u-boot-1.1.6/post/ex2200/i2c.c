/*
 * Copyright (c) 2009-2011, Juniper Networks, Inc.
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

/******** EX2200 **********/

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
#include "../board/juniper/ex2200/syspld.h"
#include "i2c.h"

typedef int (*p2f) (uint8_t, uint8_t, int);
typedef struct i2c_dev_struct {
    uint8_t device;
    uint8_t mux;
    uint8_t channel;
    char name[50];
    p2f op;
} i2c_dev;

#if CONFIG_POST & CFG_POST_I2C

#define DEV_NOT_PRESENT 0
#define DEV_FAIL (-1)
#define DEV_PASS 1

extern int i2c_read_norsta(uint8_t chanNum, uint8_t dev_addr, unsigned int offset, int alen, uint8_t* data, int len);
extern int i2c_mux(uint8_t mux, uint8_t chan, int ena);

int is_rps_present(void);
int is_poe_present(void);
int is_poe_enabled(void);
int poe_enable(int enable);
int is_sfp0_present(void);
int is_sfp1_present(void);
int is_sfp2_present(void);
int is_sfp3_present(void);

int generic_read(uint8_t addr, uint8_t mux, int chan);
int seeprom_read(uint8_t addr, uint8_t mux, int chan);
int rps_read(uint8_t addr, uint8_t mux, int chan);
int poe_controller_read(uint8_t addr, uint8_t mux, int chan);
int poe_eeprom_read(uint8_t addr, uint8_t mux, int chan);
int sfp0_read(uint8_t addr, uint8_t mux, int chan);
int sfp1_read(uint8_t addr, uint8_t mux, int chan);
int sfp2_read(uint8_t addr, uint8_t mux, int chan);
int sfp3_read(uint8_t addr, uint8_t mux, int chan);

/* i2c dev list for ex2200 */
i2c_dev i2c_dev_detect_ex2200[]=
{
    {
        /* syspld */
        I2C_SYSPLD_ADDR,
        0xFF,
        0,
        "SYSPLD",
        &generic_read,
    },
    {
        /* PCA9548A switch */
        I2C_MAIN_MUX_ADDR,
        0xFF,
        0,
        "PCA9548A Switch",
        &generic_read,
    },
    {
        /* P82B715 bus extender */
        I2C_RPS_ADDR,
        0x70,
        0,
        "P82B715 Bus Extender",
        &rps_read,
    },
    {
        /* PoE controller */
        I2C_POE_ADDR,
        I2C_MAIN_MUX_ADDR,
        1,
        "POE Controller",
        &poe_controller_read,
    },
    {
        /* PoE EEPROM */
        I2C_POE_EEPROM_ADDR,
        I2C_MAIN_MUX_ADDR,
        1,
        "POE EEPROM",
        &poe_eeprom_read,
    },
    {
        /* RTC */
        I2C_RTC_ADDR,
        I2C_MAIN_MUX_ADDR,
        2,
        "RTC",
        &generic_read,
    },
    {
        /* NE1617A temp sensor */
        I2C_TEMP_1_ADDR,
        I2C_MAIN_MUX_ADDR,
        2,
        "Temp Sensor NE1617A ",
        &generic_read,
    },
#if 0
    {
        /* ISP1520 USB hub */
        I2C_USB_1520_ADDR,
        I2C_MAIN_MUX_ADDR,
        2,
        "USB Hub ISP1520 ",
        &generic_read,
    },
#endif
    {
        /* HW Monitor W83782G */
        I2C_WINBOND_ADDR,
        I2C_MAIN_MUX_ADDR,
        2,
        "HW Monitor W83782G",
        &generic_read,
    },
    {
        /* Secure EEPROM */
        I2C_MAIN_EEPROM_ADDR,
        I2C_MAIN_MUX_ADDR,
        3,
        "Secure EEPROM",
        &seeprom_read,
    },
    {
        /* SFP 0 */
        0x50,
        I2C_MAIN_MUX_ADDR,
        4,
        "SFP 0",
        &sfp0_read,
    },
    {
        /* SFP 1 */
        0x50,
        I2C_MAIN_MUX_ADDR,
        5,
        "SFP 1",
        &sfp1_read,
    },
    {
        /* SFP 2 */
        0x50,
        I2C_MAIN_MUX_ADDR,
        6,
        "SFP 2",
        &sfp2_read,
    },
    {
        /* SFP 3 */
        0x50,
        I2C_MAIN_MUX_ADDR,
        7,
        "SFP 3",
        &sfp3_read,
    },
};

/* i2c dev list for ex2200_12x  */
i2c_dev i2c_dev_detect_ex2200_12x[]=
{
    {
        /* syspld */
        I2C_SYSPLD_ADDR,
        0xFF,
        0,
        "SYSPLD",
        &generic_read,
    },
    {
        /* PCA9548A switch */
        I2C_MAIN_MUX_ADDR,
        0xFF,
        0,
        "PCA9548A Switch",
        &generic_read,
    },
    {
        /* PoE controller */
        I2C_POE_ADDR,
        I2C_MAIN_MUX_ADDR,
        2,
        "POE Controller",
        &poe_controller_read,
    },
    {
        /* PoE EEPROM */
        I2C_POE_EEPROM_ADDR,
        I2C_MAIN_MUX_ADDR,
        2,
        "POE EEPROM",
        &poe_eeprom_read,
    },
    {
        /* Secure EEPROM */
        I2C_MAIN_EEPROM_ADDR,
        I2C_MAIN_MUX_ADDR,
        3,
        "Secure EEPROM",
        &seeprom_read,
    },
    {
        /* SFP 0 */
        0x50,
        I2C_MAIN_MUX_ADDR,
        0,
        "SFP 0",
        &sfp0_read,
    },
    {
        /* SFP 1 */
        0x50,
        I2C_MAIN_MUX_ADDR,
        1,
        "SFP 1",
        &sfp1_read,
    },
};

int 
generic_read (uint8_t addr, uint8_t mux, int chan)
{
    uint8_t temp;
    int res = DEV_PASS;
    
    if (mux != 0xFF) {
        if (i2c_mux(mux, chan, 1)) 
            return (DEV_FAIL);
    }
    if (i2c_read(0, addr, 0, 0, &temp, 1))
        res = DEV_FAIL;
    i2c_mux(mux, 0, 0);

    return (res);
}

int 
seeprom_read (uint8_t addr, uint8_t mux, int chan)
{
    uint8_t temp;
    int res = DEV_PASS;
    
    if (mux != 0xFF) {
        if (i2c_mux(mux, chan, 1)) 
            return (DEV_FAIL);
    }
    if (i2c_read_norsta(0, addr, 0, 1, &temp, 1))
        res = DEV_FAIL;
    i2c_mux(mux, 0, 0);

    return (res);
}

int 
is_rps_present (void)
{
    uint8_t value;
    
    if (syspld_reg_read(SYSPLD_MISC, &value))
        return (0);

    if (value & SYSPLD_RPS_PRESENT)
        return (1);

    return (0);
}

int 
rps_read (uint8_t addr, uint8_t mux, int chan)
{
    uint8_t temp;
    int res = DEV_PASS;

    if (!is_rps_present())
        return (DEV_NOT_PRESENT);
    
    if (mux != 0xFF) {
        if (i2c_mux(mux, chan, 1)) 
            return (DEV_FAIL);
    }
    if (i2c_read(0, addr, 0, 0, &temp, 1))
        res = DEV_FAIL;
    i2c_mux(mux, 0, 0);

    return (res);
}

int 
is_poe_present (void)
{
    uint8_t value;
    
    if (syspld_reg_read(SYSPLD_MISC, &value))
        return (0);

    if (value & SYSPLD_POE_PRESENT)
        return (1);

    return (0);
}

int 
is_poe_enabled (void)
{
    uint8_t value;
    
    if (syspld_reg_read(SYSPLD_MISC, &value))
        return (0);

    if (value & SYSPLD_POE_ENABLE)
        return (0);

    return (1);
}

int 
poe_enable (int enable)
{
    uint8_t value;

    if (enable) {
        if (syspld_reg_read(SYSPLD_RESET_REG_2, &value))
            return (0);

        value &= ~SYSPLD_POECTRL_ENABLE;

        if (syspld_reg_write(SYSPLD_RESET_REG_2, value))
            return (0);

        if (syspld_reg_read(SYSPLD_MISC, &value))
            return (0);

        value &= ~SYSPLD_POE_ENABLE;

        if (syspld_reg_write(SYSPLD_MISC, value))
            return (0);
    }
    else {
        if (syspld_reg_read(SYSPLD_RESET_REG_2, &value))
            return (0);

        value |= SYSPLD_POECTRL_ENABLE;

        if (syspld_reg_write(SYSPLD_RESET_REG_2, value))
            return (0);

        if (syspld_reg_read(SYSPLD_MISC, &value))
            return (0);

        value |= SYSPLD_POE_ENABLE;

        if (syspld_reg_write(SYSPLD_MISC, value))
            return (0);
    }
    
    return (1);
}

int 
poe_eeprom_read (uint8_t addr, uint8_t mux, int chan)
{
    uint8_t temp;
    int res = DEV_PASS;

    if (!is_poe_present())
        return (DEV_NOT_PRESENT);
    
    if (mux != 0xFF) {
        if (i2c_mux(mux, chan, 1)) 
            return (DEV_FAIL);
    }
    if (i2c_read(0, addr, 0, 1, &temp, 1))
        res = DEV_FAIL;
    i2c_mux(mux, 0, 0);

    return (res);
}

int 
poe_controller_read (uint8_t addr, uint8_t mux, int chan)
{
    uint8_t temp[15];
    int res = DEV_PASS;

    if (!is_poe_present())
        return (DEV_NOT_PRESENT);
    

    if (!is_poe_enabled()) {
        poe_enable(1);
        udelay(3000000);
    }
    
    if (mux != 0xFF) {
        if (i2c_mux(mux, chan, 1)) 
            return (DEV_FAIL);
    }
    if (i2c_read(0, addr, 0, 1, temp, 15))
        res = DEV_FAIL;
    
    i2c_mux(mux, 0, 0);

    return (res);
}

int 
is_sfp0_present (void)
{
    uint8_t value;
    
    if (syspld_reg_read(SYSPLD_UPLINK_1, &value))
        return (0);

    if (value & SYSPLD_SFP_0_PRESENT)
        return (1);

    return (0);
}

int 
is_sfp1_present (void)
{
    uint8_t value;
    
    if (syspld_reg_read(SYSPLD_UPLINK_1, &value))
        return (0);

    if (value & SYSPLD_SFP_1_PRESENT)
        return (1);

    return (0);
}

int 
is_sfp2_present (void)
{
    uint8_t value;
    
    if (syspld_reg_read(SYSPLD_UPLINK_2, &value))
        return (0);

    if (value & SYSPLD_SFP_2_PRESENT)
        return (1);

    return (0);
}

int 
is_sfp3_present (void)
{
    uint8_t value;
    
    if (syspld_reg_read(SYSPLD_UPLINK_2, &value))
        return (0);

    if (value & SYSPLD_SFP_3_PRESENT)
        return (1);

    return (0);
}

int 
sfp0_read (uint8_t addr, uint8_t mux, int chan)
{
    uint8_t temp;
    int res = DEV_PASS;

    if (!is_sfp0_present())
        return (DEV_NOT_PRESENT);
    
    if (mux != 0xFF) {
        if (i2c_mux(mux, chan, 1)) 
            return (DEV_FAIL);
    }
    if (i2c_read(0, addr, 0, 0, &temp, 1))
        res = DEV_FAIL;
    i2c_mux(mux, 0, 0);

    return (res);
}

int 
sfp1_read (uint8_t addr, uint8_t mux, int chan)
{
    uint8_t temp;
    int res = DEV_PASS;

    if (!is_sfp1_present())
        return (DEV_NOT_PRESENT);
    
    if (mux != 0xFF) {
        if (i2c_mux(mux, chan, 1)) 
            return (DEV_FAIL);
    }
    if (i2c_read(0, addr, 0, 0, &temp, 1))
        res = DEV_FAIL;
    i2c_mux(mux, 0, 0);

    return (res);
}

int 
sfp2_read (uint8_t addr, uint8_t mux, int chan)
{
    uint8_t temp;
    int res = DEV_PASS;

    if (!is_sfp2_present())
        return (DEV_NOT_PRESENT);
    
    if (mux != 0xFF) {
        if (i2c_mux(mux, chan, 1)) 
            return (DEV_FAIL);
    }
    if (i2c_read(0, addr, 0, 0, &temp, 1))
        res = DEV_FAIL;
    i2c_mux(mux, 0, 0);

    return (res);
}

int 
sfp3_read (uint8_t addr, uint8_t mux, int chan)
{
    uint8_t temp;
    int res = DEV_PASS;

    if (!is_sfp3_present())
        return (DEV_NOT_PRESENT);
    
    if (mux != 0xFF) {
        if (i2c_mux(mux, chan, 1)) 
            return (DEV_FAIL);
    }
    if (i2c_read(0, addr, 0, 0, &temp, 1))
        res = DEV_FAIL;
    i2c_mux(mux, 0, 0);

    return (res);
}

int 
i2c_post_test (int flags)
{
    int i2c_result_array[20] = {0};
    int i2c_verbose = flags & POST_DEBUG;
    int fail = 0, i;
    i2c_dev *i2c_dev_detect;

#ifdef CONFIG_EX2200_12X
    i2c_dev_detect = i2c_dev_detect_ex2200_12x;
#else
    i2c_dev_detect = i2c_dev_detect_ex2200;
#endif

    int i2c_result_size = sizeof(i2c_dev_detect) / sizeof(i2c_dev_detect[0]);

    for (i = 0; i < i2c_result_size; i++) {
        i2c_result_array[i] = i2c_dev_detect[i].op(i2c_dev_detect[i].device, 
            i2c_dev_detect[i].mux, i2c_dev_detect[i].channel);
        if (i2c_result_array[i] == DEV_FAIL)
            fail = 1;
    }

    if (fail) {
        post_log("-- I2C test                             FAILED @\n");
        for (i = 0; i < i2c_result_size; i++) {
            if (i2c_result_array[i] == DEV_FAIL) {
                post_log (" > Expected I2C: %-11s %-20s "
						  "Mux=0x%02x "
						  "Ch=%1d "
						  "Addr=0x%02x @\n",
						  "Not Found",
						  i2c_dev_detect[i].name,
						  i2c_dev_detect[i].mux,
						  i2c_dev_detect[i].channel,
						  i2c_dev_detect[i].device);
            }
            else {
                if (i2c_verbose) {
                    post_log (" > Expected I2C: %-11s %-20s "
						      "Mux=0x%02x "
						      "Ch=%1d "
						      "Addr=0x%02x\n",
						      (i2c_result_array[i]==DEV_PASS) ? 
						      "Found" : "Not Present",
						      i2c_dev_detect[i].name,
						      i2c_dev_detect[i].mux,
						      i2c_dev_detect[i].channel,
						      i2c_dev_detect[i].device);
                }
            }
        }
    }
    else {
        post_log("-- I2C test                             PASSED\n");
        if (i2c_verbose) {
            for (i = 0; i < i2c_result_size; i++) {
                post_log (" > Expected I2C: %-11s %-20s "
						  "Mux=0x%02X "
						  "Ch=%1d "
						  "Addr=0x%02X\n",
						  (i2c_result_array[i]==DEV_PASS) ? 
						  "Found" : "Not Present",
						  i2c_dev_detect[i].name,
						  i2c_dev_detect[i].mux,
						  i2c_dev_detect[i].channel,
						  i2c_dev_detect[i].device);
            }
        }
    }
    
    if (fail)
        return (-1);
    return (0);

}
#endif /* CONFIG_POST & CFG_POST_I2C */
#endif /* CONFIG_POST */
