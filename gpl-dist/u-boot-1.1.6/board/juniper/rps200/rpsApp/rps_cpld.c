/*
 * Copyright (c) 2008-2010, Juniper Networks, Inc.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include "rps.h"
#include <exports.h>
#include "rps_command.h"
#include "rps_cpld.h"
#if defined(PLD_CACHE_ENABLE) 
#include "rps_etq.h"
extern void psUpdate(uint8_t ps, uint8_t fan);
extern void portUpdate(uint8_t ps, uint8_t fan);
#endif

//#define DEBUG_CPLD_MEMORY

#define BITS_PER_PORT_COUNT 5
#define PLD_POLLING_START 70
#define PLD_POLLING_PERIOD 100

#define I2C_WRITE_MAX 5

#define PS2_MASK 0xC

static const uint8_t cpld_reg_offset[BITS_PER_PORT_COUNT][CPLD_MAX_PORT] = 
    {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
      {0x00, 0x00, 0x00, 0x01, 0x01, 0x01},
      {0x00, 0x00, 0x01, 0x01, 0x02, 0x02},
      {0x00, 0x00, 0x01, 0x01, 0x02, 0x02}};
      
static const uint8_t cpld_reg_shift[BITS_PER_PORT_COUNT][CPLD_MAX_PORT] = 
    {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
      {0x00, 0x01, 0x02, 0x03, 0x04, 0x05},
      {0x00, 0x02, 0x04, 0x00, 0x02, 0x04},
      {0x00, 0x03, 0x00, 0x03, 0x00, 0x03},
      {0x00, 0x04, 0x00, 0x04, 0x00, 0x04}};

static const uint8_t cpld_reg_mask[BITS_PER_PORT_COUNT][CPLD_MAX_PORT] = 
    {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
      {0x01, 0x02, 0x04, 0x08, 0x10, 0x20},
      {0x03, 0x0c, 0x30, 0x03, 0x0c, 0x30},
      {0x07, 0x38, 0x07, 0x38, 0x07, 0x38},
      {0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0}};

struct cpld_t rps_pld;

int gI2cCpuError = 0;
int gI2cPldError = 0;
int gI2cCpuReset = 0;
int gI2cPldReset = 0;
#define I2C_RESET_THRESHOLD 3

void cpld_dump (void);

#if defined(PLD_CACHE_ENABLE) 
void cpld_update_cache(ptData ptdata);
#endif

int 
cpld_init (void)
{
#if defined(PLD_CACHE_ENABLE) 
    int32_t tid;
    tData tdata;
#if !defined(DEBUG_CPLD_MEMORY)    
    uint8_t data[CPLD_LAST];
    uint8_t ch = 1 << CPLD_CHANNEL;
#endif

#if !defined(DEBUG_CPLD_MEMORY)    
    i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
    if (i2c_read(0, CPLD_I2C_ADDRESS, I2C_ADDRESS, 1, data, CPLD_LAST) == 0) 
        memcpy(&rps_pld.u.data[I2C_ADDRESS], data, CPLD_LAST);

    ch = 0;
    i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
#else
    memset(&rps_pld.u.data[I2C_ADDRESS], 0, CPLD_LAST);
#endif

    /* force PS_2 coming up first */
    rps_pld.u.cpld.ps_working &= PS2_MASK;
    psUpdate(rps_pld.u.cpld.ps_working, rps_pld.u.cpld.ps_fan_fail);
    portUpdate(rps_pld.u.cpld.port_state[0], rps_pld.u.cpld.port_state[1]);

    tdata.param[0] = tdata.param[1] = tdata.param[2] = 0;
    addTimer(tdata, &tid, PERIDICALLY, PLD_POLLING_START, PLD_POLLING_PERIOD,
             cpld_update_cache);
#endif
    return (0);
}

#if defined(PLD_CACHE_ENABLE) 
void 
cpld_update_cache (ptData ptdata)
{
    uint8_t ch = 1 << CPLD_CHANNEL;

#if !defined(DEBUG_CPLD_MEMORY)    
    uint8_t data[CPLD_LAST];
#endif

    /* priority, state */
    /* LEDs */
    /* write protect, misc enable, unit present, voltage trip point, ps,
     * max active. 
     */
#if !defined(DEBUG_CPLD_MEMORY)    
    if (i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1))
        gI2cCpuError++;
    else {
        if (i2c_read(0, CPLD_I2C_ADDRESS, PORT_1_2_PRIORITY, 1, data, 20))
            gI2cPldError++;
        else {
            memcpy(&rps_pld.u.data[PORT_1_2_PRIORITY], data, 20);
            ch = 0;
            i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
            psUpdate(rps_pld.u.cpld.ps_working, rps_pld.u.cpld.ps_fan_fail);
            portUpdate(rps_pld.u.cpld.port_state[0], 
                       rps_pld.u.cpld.port_state[1]);
        }
    }
    
    if (gI2cCpuError > I2C_RESET_THRESHOLD) {
        i2c_reset();
        gI2cCpuReset++;
        gI2cCpuError = 0;
    }
    
    if (gI2cPldError > I2C_RESET_THRESHOLD) {
        i2c_cpld_reset();
        gI2cPldReset++;
        gI2cPldError = 0;
    }
    
#else
    psUpdate(rps_pld.u.cpld.ps_working, rps_pld.u.cpld.ps_fan_fail);
    portUpdate(rps_pld.u.cpld.port_state[0], rps_pld.u.cpld.port_state[1]);
#endif
}
#endif

int 
cpld_reg_read (int32_t reg, uint8_t *val)
{
    uint8_t temp;
#if !defined(PLD_CACHE_ENABLE) 
    uint8_t ch = 1 << CPLD_CHANNEL;
#endif

#if defined(PLD_CACHE_ENABLE) 
    temp = rps_pld.u.data[reg];
#else
    i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
    if (i2c_read(0, CPLD_I2C_ADDRESS, reg, 1, &temp, 1)) {
        ch = 0;
        i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
        return (-1);
    }
    
    ch = 0;
    i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
        
    rps_pld.u.data[reg] = temp;
#endif

    *val = temp;
    return (0);
}

int 
cpld_reg_read_direct (int32_t reg, uint8_t *val)
{
    uint8_t temp;
    uint8_t ch = 1 << CPLD_CHANNEL;
    
#if defined(DEBUG_CPLD_MEMORY)
    return (cpld_reg_read(reg, val));
#endif

    i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
    if (i2c_read(0, CPLD_I2C_ADDRESS, reg, 1, &temp, 1))  {
        ch = 0;
        i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
        return (-1);
    }
    
    ch = 0;
    i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
    
    rps_pld.u.data[reg] = temp;
    *val = temp;
    return (0);
}

int 
cpld_reg_write (int32_t reg, uint8_t val)
{
    uint8_t temp = val;
    uint8_t ch = 1 << CPLD_CHANNEL;
        
#if defined(DEBUG_CPLD_MEMORY)
    rps_pld.u.data[reg] = val;
    return (0);
#endif
    i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
    if (i2c_write(0, CPLD_I2C_ADDRESS, reg, 1, &temp, 1)) {
        ch = 0;
        i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
        return (-1);
    }
    
    ch = 0;
    i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
    
    rps_pld.u.data[reg] = val;
    return (0);
}

int 
cpld_read (int32_t offset, uint8_t *data, int32_t length)
{
#if !defined(PLD_CACHE_ENABLE) 
    uint8_t ch = 1 << CPLD_CHANNEL;
#endif
    
#if defined(PLD_CACHE_ENABLE) || defined(DEBUG_CPLD_MEMORY)
    memcpy(data, &rps_pld.u.data[offset], length);
#else
    if (i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1)) {
        gI2cCpuError++;
        return (-1);
    }
    if (i2c_read(0, CPLD_I2C_ADDRESS, offset, 1, data, length)) {
        gI2cPldError++;
        ch = 0;
        i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
        return (-1);
    }

    ch = 0;
    i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
    
#endif

    return (0);
}

int 
cpld_read_direct (int32_t offset, uint8_t *data, int32_t length)
{
    uint8_t ch = 1 << CPLD_CHANNEL;
    
#if defined(DEBUG_CPLD_MEMORY) 
    memcpy(data, &rps_pld.u.data[offset], length);
#else
    i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
    if (i2c_read(0, CPLD_I2C_ADDRESS, offset, 1, data, length)) {
        ch = 0;
        i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
        return (-1);
    }

    ch = 0;
    i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
    
    memcpy(&rps_pld.u.data[offset], data, length);
#endif

    return (0);
}

int 
cpld_write (int32_t offset, uint8_t *data, int32_t length)
{
    uint8_t ch = 1 << CPLD_CHANNEL;
    uint8_t temp[10];
    int32_t len;
    int i;
    
#if defined(DEBUG_CPLD_MEMORY)
    memcpy(&rps_pld.u.data[offset], data, length);
    return (0);
#endif
    if (i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1)) {
        gI2cCpuError++;
        return (-1);
    }
    len = (length >= 10) ? 10 : length;
    for (i = 0; i < I2C_WRITE_MAX; i++) {
        if (i2c_write(0, CPLD_I2C_ADDRESS, offset, 1, data, length)) {
            ch = 0;
            i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
            return (-1);
        }
        if (i2c_read(0, CPLD_I2C_ADDRESS, offset, 1, temp, length)) {
            ch = 0;
            i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
            return (-1);
        }
        if (memcmp(data, temp, len) == 0)
            break;
        else
            gI2cPldError++;
    }

    ch = 0;
    i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
    
    memcpy(&rps_pld.u.data[offset], data, length);
    return (0);
}

uint8_t 
cpld_get_reg_offset (int32_t bits, int32_t port)
{
    if ((bits <= BITS_PER_PORT_COUNT) && (port < CPLD_MAX_PORT))
        return (cpld_reg_offset[bits][port]);

    return (0);
}

uint8_t 
cpld_get_reg_shift (int32_t bits, int32_t port)
{
    if ((bits <= BITS_PER_PORT_COUNT) && (port < CPLD_MAX_PORT))
        return (cpld_reg_shift[bits][port]);

    return (0);
}

uint8_t 
cpld_get_reg_mask (int32_t bits, int32_t port)
{
    if ((bits <= BITS_PER_PORT_COUNT) && (port < CPLD_MAX_PORT))
        return cpld_reg_mask[bits][port];

    return (0);
}

void 
cpld_dump (void)
{
    int i;

    for (i = I2C_ADDRESS; i < CPLD_LAST; i++) {
        printf("CPLD addr 0x%02x = 0x%02x.\n", i, rps_pld.u.data[i]);
    }
    printf("\n");
}

int 
do_cpld (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uint32_t addr = 0;
    uint8_t value = 0;
    char cmd1, cmd2;

    if (argc <= 2)
        goto usage;
    
    cmd1 = argv[1][0];
    cmd2 = argv[2][0];
    
    switch (cmd1) {
        case 'r':       /* Register */
        case 'R':
            switch (cmd2) {
                case 'd':   /* dump */
                case 'D':
                    cpld_dump();
                    break;
                    
                case 'r':   /* read */
                case 'R':
                    if (argc <= 3)
                        goto usage;
                    
                    addr = simple_strtoul(argv[3], NULL, 16);
                    if (cpld_reg_read(addr, &value))
                        printf("CPLD addr 0x%02x read failed.\n", addr);
                    else
                        printf("CPLD addr 0x%02x = 0x%02x.\n", addr, value);
                    break;
                    
                case 'w':   /* write */
                case 'W':
                    if (argc <= 4)
                        goto usage;
                    
                    addr = simple_strtoul(argv[3], NULL, 16);
                    value = simple_strtoul(argv[4], NULL, 16);
                    if (cpld_reg_write(addr, value))
                        printf("CPLD write addr 0x%02x value 0x%02x failed.\n",
                               addr, value);
                    else {
                        if (cpld_reg_read_direct(addr, &value))
                            printf("CPLD addr 0x%02x read back failed.\n",
                                   addr);
                        else
                            printf("CPLD read back addr 0x%02x = 0x%02x.\n",
                                   addr, value);
                    }
                    break;
                    
                default:
                    printf("???");
                    break;
            }
            
            break;
            
            
        default:
            printf("???");
            break;
    }

    return (1);
 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return (1);
}

U_BOOT_CMD(
    cpld,   5,  1,  do_cpld,
    "cpld    - CPLD test commands\n",
    "\n"
    "cpld register read <address>\n"
    "    - read CPLD register\n"
    "cpld register write <address> <value>\n"
    "    - write CPLD register\n"
    "cpld register dump\n"
    "    - dump CPLD registers\n"
);

