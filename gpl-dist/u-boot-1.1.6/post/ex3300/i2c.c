/*
 * Copyright (c) 2010, Juniper Networks, Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>

/******** EX3300 **********/

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
#include <syspld.h>
#include <i2c.h>

typedef int (*p2f) (uint8_t, uint8_t, uint8_t, int);
typedef struct i2c_dev_struct {
    uint8_t dev;
    uint8_t mux;
    uint8_t chan;
    uint8_t ctrl;
    char name[50];
    p2f op;
} i2c_dev;

#if CONFIG_POST & CFG_POST_I2C

#define DEV_NOT_PRESENT 0
#define DEV_FAIL (-1)
#define DEV_PASS 1

extern int i2c_read_norsta(uint8_t chanNum, uint8_t dev_addr, unsigned int offset, int alen, uint8_t* data, int len);
extern int i2c_mux(uint8_t ctrl, uint8_t mux, uint8_t chan, int ena);

int is_rps_present(void);
int is_poe_present(void);
int is_poe_enabled(void);
int poe_enable(int enable);

int generic_read(uint8_t ctrl, uint8_t addr, uint8_t mux, int chan);
int generic_read_2(uint8_t ctrl, uint8_t addr, uint8_t mux, int chan);
int seeprom_read(uint8_t ctrl, uint8_t addr, uint8_t mux, int chan);
int rps_read(uint8_t ctrl, uint8_t addr, uint8_t mux, int chan);
int poe_controller_read(uint8_t ctrl, uint8_t addr, uint8_t mux, int chan);
int poe_eeprom_read(uint8_t ctrl, uint8_t addr, uint8_t mux, int chan);

/* i2c dev list  */
i2c_dev i2c_dev_detect[]=
{
    {
        /* PCA9546A switch */
        I2C_MAIN_MUX_ADDR,
        0xFF,
        0,
        0,
        "PCA9548A Switch",
        &generic_read,
    },
    {
        /* HW Monitor MAX6581 */
        I2C_MAX6581_ADDR,
        I2C_MAIN_MUX_ADDR,
        0,
        0,
        "HW Monitor MAX6581",
        &generic_read,
    },
    {
        /* PoE controller */
        I2C_POE_ADDR,
        I2C_MAIN_MUX_ADDR,
        1,
        0,
        "POE Controller",
        &poe_controller_read,
    },
    {
        /* PoE EEPROM */
        I2C_POE_EEPROM_ADDR,
        I2C_MAIN_MUX_ADDR,
        1,
        0,
        "POE EEPROM",
        &poe_eeprom_read,
    },
    {
        /* RTC */
        I2C_RTC_ADDR,
        I2C_MAIN_MUX_ADDR,
        2,
        0,
        "RTC",
        &generic_read,
    },
    {
        /* Secure EEPROM */
        I2C_MAIN_EEPROM_ADDR,
        I2C_MAIN_MUX_ADDR,
        3,
        0,
        "Secure EEPROM",
        &seeprom_read,
    },
    {
        /* P82B715 bus extender */
        I2C_RPS_ADDR,
        0xFF,
        0,
        1,
        "P82B715 Bus Extender",
        &rps_read,
    },
    {
        /* INA219 power monitor */
        I2C_INA219_ADDR,
        0xFF,
        0,
        1,
        "INA219 Power Monitor",
        &generic_read_2,
    },
};

int 
generic_read (uint8_t ctrl, uint8_t addr, uint8_t mux, int chan)
{
    uint8_t temp;
    int res = DEV_PASS;
    
    if (mux != 0xFF) {
        i2c_mux(ctrl, mux, chan, 1);
    }
    if (i2c_read(ctrl, addr, 0, 0, &temp, 1))
        res = DEV_FAIL;
    if (mux != 0xFF) {
        i2c_mux(ctrl, mux, 0, 0);
    }

    return res;
}

int 
generic_read_2 (uint8_t ctrl, uint8_t addr, uint8_t mux, int chan)
{
    uint8_t temp[2];
    int res = DEV_PASS;
    
    if (mux != 0xFF) {
        i2c_mux(ctrl, mux, chan, 1);
    }
    if (i2c_read(ctrl, addr, 0, 0, temp, 2))
        res = DEV_FAIL;
    if (mux != 0xFF) {
        i2c_mux(ctrl, mux, 0, 0);
    }

    return res;
}

int 
seeprom_read (uint8_t ctrl, uint8_t addr, uint8_t mux, int chan)
{
    uint8_t temp;
    int res = DEV_PASS;
    
    if (mux != 0xFF) {
        i2c_mux(ctrl, mux, chan, 1);
    }
    if (i2c_read_norsta(ctrl, addr, 0, 1, &temp, 1))
        res = DEV_FAIL;
    if (mux != 0xFF) {
        i2c_mux(ctrl, mux, 0, 0);
    }

    return res;
}

int 
is_rps_present (void)
{
    uint8_t value;
    
    if (syspld_reg_read(SYSPLD_MISC, &value))
        return 0;

    if (value & SYSPLD_RPS_PRESENT)
        return 1;

    return 0;
}

int 
rps_read (uint8_t ctrl, uint8_t addr, uint8_t mux, int chan)
{
    uint8_t temp;
    int res = DEV_PASS;

    if (!is_rps_present())
        return DEV_NOT_PRESENT;
    
    if (mux != 0xFF) {
        i2c_mux(ctrl, mux, chan, 1);
    }
    if (i2c_read(ctrl, addr, 0, 0, &temp, 1))
        res = DEV_FAIL;
    if (mux != 0xFF) {
        i2c_mux(ctrl, mux, 0, 0);
    }

    return res;
}

int 
is_poe_present (void)
{
    uint8_t value;
    
    if (syspld_reg_read(SYSPLD_MISC, &value))
        return 0;

    if (value & SYSPLD_POE_PRESENT)
        return 1;

    return 0;
}

int 
is_poe_enabled (void)
{
    uint8_t value;
    
    if (syspld_reg_read(SYSPLD_MISC, &value))
        return 0;

    if (value & SYSPLD_POE_DISABLE)
        return 0;

    return 1;
}

int 
poe_enable (int enable)
{
    uint8_t value;

    if (enable) {
        if (syspld_reg_read(SYSPLD_RESET_REG_2, &value))
            return 0;

        value &= ~SYSPLD_POECTRL_RESET;

        if (syspld_reg_write(SYSPLD_RESET_REG_2, value))
            return 0;

        if (syspld_reg_read(SYSPLD_MISC, &value))
            return 0;

        value &= ~SYSPLD_POE_DISABLE;

        if (syspld_reg_write(SYSPLD_MISC, value))
            return 0;
    }
    else {
        if (syspld_reg_read(SYSPLD_RESET_REG_2, &value))
            return 0;

        value |= SYSPLD_POECTRL_RESET;

        if (syspld_reg_write(SYSPLD_RESET_REG_2, value))
            return 0;

        if (syspld_reg_read(SYSPLD_MISC, &value))
            return 0;

        value |= SYSPLD_POE_DISABLE;

        if (syspld_reg_write(SYSPLD_MISC, value))
            return 0;
    }
    
    return 1;
}

int 
poe_eeprom_read (uint8_t ctrl, uint8_t addr, uint8_t mux, int chan)
{
    uint8_t temp;
    int res = DEV_PASS;

    if (!is_poe_present())
        return DEV_NOT_PRESENT;
    
    if (mux != 0xFF) {
        i2c_mux(ctrl, mux, chan, 1);
    }
    if (i2c_read(ctrl, addr, 0, 1, &temp, 1))
        res = DEV_FAIL;
    if (mux != 0xFF) {
        i2c_mux(ctrl, mux, 0, 0);
    }

    return res;
}

int 
poe_controller_read (uint8_t ctrl, uint8_t addr, uint8_t mux, int chan)
{
    uint8_t temp[15];
    int res = DEV_PASS;

    if (!is_poe_present())
        return DEV_NOT_PRESENT;
    

    if (!is_poe_enabled()) {
        poe_enable(1);
        udelay(3000000);
    }
    
    if (mux != 0xFF) {
        i2c_mux(ctrl, mux, chan, 1);
    }
    if (i2c_read(ctrl, addr, 0, 1, temp, 15))
        res = DEV_FAIL;
    
//    poe_enable(0);
    
    if (mux != 0xFF) {
        i2c_mux(ctrl, mux, 0, 0);
    }

    return res;
}

int 
i2c_post_test (int flags)
{
    int i2c_result_size = sizeof(i2c_dev_detect) / sizeof(i2c_dev_detect[0]);
    int i2c_result_array[20] = {0};
    int i2c_verbose = flags & POST_DEBUG;
    int fail = 0, i;
    unsigned char tmp[15], tmp1[10];

    for (i=0; i<i2c_result_size; i++) {
        i2c_result_array[i] = i2c_dev_detect[i].op(
            i2c_dev_detect[i].ctrl, 
            i2c_dev_detect[i].dev, 
            i2c_dev_detect[i].mux, 
            i2c_dev_detect[i].chan);
        if (i2c_result_array[i] == DEV_FAIL)
            fail = 1;
    }

    if (fail) {
        post_log("-- I2C test                             FAILED @\n");
        for (i = 0; i < i2c_result_size; i++) {
            sprintf(tmp, "C%1d ", i2c_dev_detect[i].ctrl);
            tmp[strlen(tmp) - 1] = 0;
            tmp1[0] = 0;
            if (i2c_dev_detect[i].mux != 0xFF) {
                sprintf(tmp1, "M%02xP%1d %02x", 
                        i2c_dev_detect[i].mux,
                        i2c_dev_detect[i].chan,
                        i2c_dev_detect[i].dev);
            }
            else {
                sprintf(tmp1, "M--P- %02x", i2c_dev_detect[i].dev);
            }
            strcat(tmp, tmp1);
            if (i2c_result_array[i] == DEV_FAIL) {
                post_log (" > I2C: %-11s %-20s %-10s @\n",
                          "Not Found",
                          i2c_dev_detect[i].name,
                          tmp);
            }
            else {
                if (i2c_verbose) {
                    post_log (" > I2C: %-11s %-20s %-10s\n",
                          (i2c_result_array[i]==DEV_PASS) ? 
                          "Found" : "Not Present",
                          i2c_dev_detect[i].name,
                          tmp);
                }
            }
        }
    }
    else {
        post_log("-- I2C test                             PASSED\n");
        if (i2c_verbose) {
            for (i = 0; i < i2c_result_size; i++) {
            sprintf(tmp, "C%1d ", i2c_dev_detect[i].ctrl);
            tmp[strlen(tmp) - 1] = 0;
            tmp1[0] = 0;
            if (i2c_dev_detect[i].mux != 0xFF) {
                sprintf(tmp1, "M%02xP%1d %02x", 
                        i2c_dev_detect[i].mux,
                        i2c_dev_detect[i].chan,
                        i2c_dev_detect[i].dev);
            }
            else {
                sprintf(tmp1, "M--P- %02x", i2c_dev_detect[i].dev);
            }
            strcat(tmp, tmp1);
                post_log (" > I2C: %-11s %-20s %-10s\n",
                          (i2c_result_array[i]==DEV_PASS) ? 
                          "Found" : "Not Present",
                          i2c_dev_detect[i].name,
                          tmp);
            }
        }
    }
    
    if (fail)
        return (-1);
    return 0;

}
#endif /* CONFIG_POST & CFG_POST_I2C */
#endif /* CONFIG_POST */
