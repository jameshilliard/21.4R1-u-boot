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
#include "rps_etq.h"
#include "rps_envMon.h"

//#define DEBUG

typedef enum envId { 
    TEMP_READ,
    VOLT_READ,
    MAX_ENV_TID
} tEnvID;

tEnvData gEnv;
static int32_t gTid[MAX_ENV_TID];

void 
initEnv (void)
{
    uint8_t tdata, ch;
    int i;

    for (i = 0; i < MAX_TEMP_SENSOR; i++)
    {
        gEnv.temp[i] = 0;
    }
    for (i = 0; i < MAX_VOLT_SENSOR; i++)
    {
        gEnv.volt[i] = 0;
    }
    for (i = 0; i < MAX_ENV_TID; i++)
    {
        gTid[i] = 0;
    }

    /* setup windbond */
    ch = 1 << 6;
    i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
    tdata = 0x80;  /* bank 0 */
    i2c_write(0, CFG_I2C_HW_MON_ADDR, 0x4e, 1, &tdata, 1);
    tdata = 0x0f;
    i2c_write(0, CFG_I2C_HW_MON_ADDR, 0x5d, 1, &tdata, 1);
    tdata = 0x00;
    i2c_write(0, CFG_I2C_HW_MON_ADDR, 0x59, 1, &tdata, 1);
    ch = 0;
    i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
}

char
tempReadSensor (int32_t sensor)
{
    uint8_t ch = 0, tdata = 0;
    int temp = 0;

    if (sensor >= MAX_TEMP_SENSOR)
        return (0);
    
    ch = 1 << 6;
    i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
    switch(sensor) {
        case 0:
            i2c_read(0, CFG_I2C_HW_MON_ADDR, 0x27, 1, &tdata, 1);
            break;
            
        case 1:
            tdata = 0x81;  /* bank 1 */
            i2c_write(0, CFG_I2C_HW_MON_ADDR, 0x4e, 1, &tdata, 1);
            i2c_read(0, CFG_I2C_HW_MON_ADDR, 0x50, 1, &tdata, 2);
            break;
            
        case 2:
            tdata = 0x82;  /* bank 2 */
            i2c_write(0, CFG_I2C_HW_MON_ADDR, 0x4e, 1, &tdata, 1);
            i2c_read(0, CFG_I2C_HW_MON_ADDR, 0x50, 1, &tdata, 2);
            break;
            
        default:
            break;       
    }

    ch = 0;
    i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
#if defined(DEBUG)
    printf("temp %d raw data = 0x%x\n", sensor, tdata);
#endif
    temp = (char)tdata;
    return ((char)temp);

}

void 
tempRead (ptData ptdata)
{
    int i;
    
    for (i = 0; i < MAX_TEMP_SENSOR; i++) {
        gEnv.temp[i] = tempReadSensor(i);
    }

}


char 
voltReadSensor (int32_t sensor)
{
    uint8_t ch, tdata, temp = 0;

    if (sensor >= MAX_VOLT_SENSOR)
        return (0);
    
    ch = 1 << ENV_CHANNEL;
    i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
    switch(sensor) {
        case 0:
            i2c_read(0, CFG_I2C_HW_MON_ADDR, 0x20, 1, &tdata, 1);
            temp = tdata * 16 / 100;
            break;
            
        case 1:
            i2c_read(0, CFG_I2C_HW_MON_ADDR, 0x21, 1, &tdata, 1);
            temp = tdata * 16 / 100;
            break;
            
        case 2:
            i2c_read(0, CFG_I2C_HW_MON_ADDR, 0x22, 1, &tdata, 1);
            temp = tdata * 16 / 100;
            break;
            
        case 3:
            i2c_read(0, CFG_I2C_HW_MON_ADDR, 0x23, 1, &tdata, 1);
            temp = tdata * 2688 / 10000;
            break;
            
        case 4:
            i2c_read(0, CFG_I2C_HW_MON_ADDR, 0x24, 1, &tdata, 1);
            temp = tdata * 608 / 1000;
            break;
            
        case 5:
            tdata = 0x85;  /* bank 5 */
            i2c_write(0, CFG_I2C_HW_MON_ADDR, 0x4e, 1, &tdata, 1);
            i2c_read(0, CFG_I2C_HW_MON_ADDR, 0x51, 1, &tdata, 1);
            temp = (tdata * 16 / 100) * 6;
            break;
            
        default:
            break;
            
    }
    ch = 0;
    i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
    
#if defined(DEBUG)
    printf("volt %d raw data = 0x%x\n", sensor, tdata);
#endif
    return (temp);
}

void 
voltRead (ptData ptdata)
{
    int i;
    
    for (i = 0; i < MAX_VOLT_SENSOR; i++) {
        gEnv.volt[i] = voltReadSensor(i);
    }    
}

void 
envTemp_display (void)
{
    printf("\n");
    printf("temp1 = %d\n", gEnv.temp[0]);
    printf("temp2 = %d\n", gEnv.temp[1]);
    printf("temp3 = %d\n", gEnv.temp[2]);
}

void 
envVolt_display (void)
{
    printf("\n");
    printf("volt0 = %2d.%01d\n", gEnv.volt[0]/10, gEnv.volt[0]%10);
    printf("volt1 = %2d.%01d\n", gEnv.volt[1]/10, gEnv.volt[1]%10);
    printf("volt2 = %2d.%01d\n", gEnv.volt[2]/10, gEnv.volt[2]%10);
    printf("volt3 = %2d.%01d\n", gEnv.volt[3]/10, gEnv.volt[3]%10);
    printf("volt4 = %2d.%01d\n", gEnv.volt[4]/10, gEnv.volt[4]%10);
    printf("volt5 = %2d.%01d\n", gEnv.volt[5]/10, gEnv.volt[5]%10);
}

void
startEnvMon (int32_t start_time, int32_t cycle_time)
{
    tData tdata;
    int32_t tid;
    int32_t start = start_time / MS_PER_TICK;
    int32_t ticks = cycle_time / MS_PER_TICK;

    tdata.param[0] = tdata.param[1] = tdata.param[2] = 0;
    if  (addTimer(tdata, &tid, PERIDICALLY, start, ticks, tempRead) == 0)
    {
        gTid[TEMP_READ] = tid;
    }
    if  (addTimer(tdata, &tid, PERIDICALLY, start+100, ticks, voltRead) == 0)
    {
        gTid[VOLT_READ] = tid;
    }
}

void 
stopEnvMon (void)
{
    int i;

    for (i = TEMP_READ; i < MAX_ENV_TID; i++)
    {
        if (gTid[i])
        {
            delTimer(gTid[i]);
            gTid[i] = 0;
        }
    }
}


/* 
 *
 * envmon commands
 * 
 */
int 
do_envmon (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uint32_t start=0, cycle=0;
    char *cmd = "dump";  // default command
    
    if (argc <= 1)
        goto usage;
    
    if (argc >= 2) cmd   = argv[1];
    if (argc == 4) {
        start = simple_strtoul(argv[2], NULL, 10) / MS_PER_TICK;
        cycle = simple_strtoul(argv[3], NULL, 10) / MS_PER_TICK;
    }

    if (!strncmp(cmd,"start", 3)) { startEnvMon(start, cycle); }
    else if (!strncmp(cmd,"stop", 3)) { stopEnvMon(); }
    else if (!strncmp(cmd,"dump", 2)) { envTemp_display(); envVolt_display(); }
    else if (!strncmp(cmd,"temp", 2)) { envTemp_display(); }
    else if (!strncmp(cmd,"voltage", 2)) { envVolt_display (); }
    else goto usage;
    return (1);

 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return (1);
}

U_BOOT_CMD(
    envmon,    4,  1,  do_envmon,
    "envmon  - environment monitor commands\n",
    "\n"
    "envmon start <start time><cycle time> \n"
    "    - start envmon cycle time in ms\n"
    "envmon stop \n"
    "    - stop envmon\n"
    "envmon dump\n"
    "    - envmon temp and voltage\n"
    "envmon [temp|voltage]\n"
    "    - envmon temp/voltage\n"
);

