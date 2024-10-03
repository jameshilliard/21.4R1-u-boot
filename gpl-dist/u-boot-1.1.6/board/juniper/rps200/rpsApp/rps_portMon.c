/*
 * Copyright (c) 2008-2011, Juniper Networks, Inc.
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
#include "rps_cpld.h"
#include "rps_led.h"
#include "rps_portMon.h"
#include "rps_priority.h"

static int32_t gPortTid = 0, gPortPresentTid = 0;
static uint8_t gPortState[2];
static uint8_t gPortPresent;
static uint8_t gPortTP[3];
static int32_t gPortI2cTimeout[CPLD_MAX_PORT];
static int32_t gI2cError[2];

static char *port_state [4] = 
 { "Off",
    "Armed",
    "Backed-up",
    "Oversubscribed"
};

uint8_t 
getPortState (int port)
{
    if (port >= CPLD_MAX_PORT)
        return (0);

    return ((gPortState[port/3]>>((port%3)*2)) & 0x3);
}

char * 
portStateString (uint8_t state)
{
    if (state > 0x3)
        return (NULL);
    
    return (port_state[state]);
}

void 
initPortMon (void)
{
    int i;
    
    gPortTid = 0;
    gPortState[0] = gPortState[1] = 0;
    gI2cError[0] = gI2cError[1] = 0;
    for (i = 0; i < CPLD_MAX_PORT; i++) {
        gPortI2cTimeout[i] = 0;
    }
}

void 
portUpdate (uint8_t p0_2, uint8_t p3_5)
{
    uint8_t ps[2];
    int i, max, backup, wontBackFlag = 0;

    ps[0] = p0_2;
    ps[1] = p3_5;

    /* port LED */
    if ((ps[0] != gPortState[0]) || (ps[1] != gPortState[1])) {
        updatePriorityPortCache(p0_2, p3_5);
        backup = getNumPortActiveBackupCache();
        max = getMaxActiveCache();
        if (backup >= max)
            wontBackFlag = 1;
        for (i = 0; i < CPLD_MAX_PORT; i++) {
            if ((ps[i/3] & (0x3<<((i%3)*2))) != 
                (gPortState[i/3] & (0x3<<((i%3)*2)))) {
                switch ((ps[i/3]>>((i%3)*2)) & 0x3) {
                    case 0x00:
                        set_port_led(i, LED_OFF);
                        break;
                    case 0x01:
                        if (wontBackFlag) {
                            if (isPortArmedWontBackupCache(i))
                                set_port_led(i, LED_AMBER);
                            else
                                set_port_led(i, LED_GREEN);
                        }
                        else
                            set_port_led(i, LED_GREEN);
                        break;
                    case 0x02:
                        set_port_led(i, LED_GREEN_BLINKING);
                        break;
                    case 0x03:
                        set_port_led(i, LED_AMBER_BLINKING);
                        break;
                }
            }
        }
        gPortState[0] = ps[0];
        gPortState[1] = ps[1];
    }
}

void 
portMonitor (ptData ptdata)
{
    uint8_t data[2];
    int i;
    
    if (cpld_read_direct(PORT_1_2_3_STATE, data, 2) == 0) {
        /* fan failure LED */
        portUpdate(data[0], data[1]);
    }
    
    if (cpld_read_direct(SWITCH_I2C_TIMEOUT, data, 1) == 0) {
        /* port I2C timeout */
        if (data[0]) {
            for (i = 0; i < CPLD_MAX_PORT; i++) {
                if (data[0] & (1<<i))
                    gPortI2cTimeout[i]++;
            }
            cpld_reg_write(SWITCH_I2C_TIMEOUT, 0xFF);
        }
    }
    if (cpld_read_direct(I2C_STATUS, data, 1) == 0) {
        /* I2C error */
        if (data[0] & 0xC0) {
            if (data[0] & 0x80)
                gI2cError[0]++;
            if (data[0] & 0x40)
                gI2cError[1]++;
            cpld_reg_write(I2C_STATUS, 0xC0);
        }
    }
}

void 
port_display (void)
{
    int i;
    
    printf("\n");
    for (i = 0; i < CPLD_MAX_PORT; i++) {
        printf("Port %d -- %s\n", i, portStateString(getPortState(i)));
    }
}

void 
startPortMon (int32_t start_time, int32_t cycle_time)
{
    tData tdata;
    int32_t tid;
    int32_t start = start_time / MS_PER_TICK;
    int32_t ticks = cycle_time / MS_PER_TICK;

    tdata.param[0] = tdata.param[1] = tdata.param[2] = 0;
    if  (addTimer(tdata, &tid, PERIDICALLY, start, ticks, portMonitor) == 0)
    {
        gPortTid = tid;
    }
}

void 
stopPortMon (void)
{
    delTimer(gPortTid);
    gPortTid = 0;
}


int32_t 
isPortPresent (int port)
{
    if (port >= CPLD_MAX_PORT)
        return (0);
    
    return (gPortPresent & (1 << port));
}

int32_t 
isPortUp (int port)
{
    uint8_t tp = 0;
    
    if (port >= CPLD_MAX_PORT)
        return (0);
    return (gPortPresent & (1 << port));

}

void 
initPortPresentMon (void)
{
    gPortPresentTid = 0;
    gPortPresent = 0;
    gPortTP[0] = gPortTP[1] = gPortTP[2] = 0;
}

void 
portPresentUpdate (uint8_t present)
{
    uint8_t data[3];
    int portChanged = 0;

    if (gPortPresent != present) {
        
        gPortPresent = present;
        portChanged = 1;
    }
    if (portChanged) {
        /* port state changed */
        priorityRemap();
    }

}

void 
portPresentMonitor (ptData ptdata)
{
    uint8_t portPresent;
    
    if (cpld_read_direct(UNIT_PRESENT, &portPresent, 1) == 0) {
        portPresentUpdate(portPresent & 0x3F);
    }
}

void 
startPortPresentMon (int32_t start_time, int32_t cycle_time)
{
    tData tdata;
    int32_t tid;
    int32_t start = start_time / MS_PER_TICK;
    int32_t ticks = cycle_time / MS_PER_TICK;

    tdata.param[0] = tdata.param[1] = tdata.param[2] = 0;
    if  (addTimer(tdata, &tid, PERIDICALLY, start, ticks, 
                  portPresentMonitor) == 0)
    {
        gPortPresentTid = tid;
    }
}

void 
stopPortPresentMon (void)
{
    delTimer(gPortPresentTid);
    gPortPresentTid = 0;
}

void 
resetPortI2C (int port)
{
    uint8_t temp;
    
    if (port >= CPLD_MAX_PORT)
        return;

    cpld_reg_read_direct(SWITCH_I2C_RESET, &temp);
    temp |= (1<<port);
    cpld_reg_write(SWITCH_I2C_RESET, temp);
    temp &= ~(1<<port);
    cpld_reg_write(SWITCH_I2C_RESET, temp);
}

int32_t 
getPortI2CTimeout (int port)
{
    if (port >= CPLD_MAX_PORT)
        return (0);

    return (gPortI2cTimeout[port]);
}

void 
clearPortI2CTimeout (int port)
{
    if (port >= CPLD_MAX_PORT)
        return;

    gPortI2cTimeout[port] = 0;
}

int32_t 
getI2CErrorProtocol (void)
{
    return (gI2cError[0]);
}

void
clearI2CErrorProtocol (void)
{
    gI2cError[0] = 0;
}

int32_t 
getI2CErrorAddress (void)
{
    return (gI2cError[1]);
}

void 
clearI2CErrorAddress (void)
{
    gI2cError[1] = 0;
}

/* 
 *
 * portmon commands
 * 
 */
int 
do_portmon (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    ulong start=0, cycle=0;
    char *cmd = "dump";  /* default command */
    
    if (argc <= 1)
        goto usage;
    
    if (argc >= 2) cmd   = argv[1];
    if (argc == 4) {
        start = simple_strtoul(argv[2], NULL, 10) / MS_PER_TICK;
        cycle = simple_strtoul(argv[3], NULL, 10) / MS_PER_TICK;
    }

    if (!strncmp(cmd,"start", 4)) { startPortMon(start, cycle); }
    else if (!strncmp(cmd,"stop", 3)) { stopPortMon(); }
    else if (!strncmp(cmd,"status", 4)) { port_display(); }
    else goto usage;
    return (1);

 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return (1);
}

U_BOOT_CMD(
    portmon,    4,  1,  do_portmon,
    "portmon - port monitor commands\n",
    "\n"
    "portmon start <start time> <cycle time> \n"
    "    - start port monitor cycle time in ms\n"
    "portmon stop \n"
    "    - stop port monitor\n"
    "portmon status\n"
    "    - port status\n"
);

