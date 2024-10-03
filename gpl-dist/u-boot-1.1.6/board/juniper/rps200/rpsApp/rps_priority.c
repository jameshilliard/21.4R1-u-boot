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
#include "rps_etq.h"
#include "rps_command.h"
#include "rps_cpld.h"
#include "rps_psMon.h"
#include "rps_priority.h"

#define MAXIMUM_MAX 3

typedef struct
{
    uint8_t state;
    uint8_t priority_requested;
    uint8_t priority_remapped;
    uint8_t priority_actual;
    int32_t req_12V;
    int32_t req_48V;
} tPower;

typedef struct
{
    int32_t port;
    uint8_t priority;
    uint8_t state;
} tPriority;

tPower rps_priority_state[CPLD_MAX_PORT];
tPriority rps_priority_port[CPLD_MAX_PORT];
uint8_t max_active = 0;
int low_rps_priority_port_index = 0;
extern char *switchSN[CPLD_MAX_PORT][MAX_SN_SZ];


int 
isPortArmedWontBackupCache (int32_t port)
{
    int ret = 0;
    int i;

    for (i = 0; i < CPLD_MAX_PORT; i++) {
        if (rps_priority_port[i].port == port) {
            break;
        }
    }

    if (i > low_rps_priority_port_index)
        ret = 1;

    return (ret);
}

int 
getNumPortActiveBackupCache (void)
{
    int i, num = 0;

    for (i = 0; i < CPLD_MAX_PORT; i++) {
        if (rps_priority_port[i].state == 0x2)
            num++;
    }
    return (num);
}

void 
updatePriorityPortCache (uint8_t p0_2, uint8_t p3_5)
{
    uint8_t portState[2];
    int i, j;

    portState[0] = p0_2;
    portState[1] = p3_5;

    for (i = 0; i < CPLD_MAX_PORT; i++) {
        j = rps_priority_port[i].port;
        rps_priority_port[i].state = ((portState[j/3]>>((j%3)*2)) & 0x3);
        low_rps_priority_port_index = i;
    }
}

uint8_t 
getPriorityRequested (int port)
{
    if (port >= CPLD_MAX_PORT)
        return (0);

    return(rps_priority_state[port].priority_requested);
}

uint8_t 
getPriorityRemapped (int port)
{
    if (port >= CPLD_MAX_PORT)
        return (0);

    return(rps_priority_state[port].priority_remapped);
}

uint8_t 
getPriorityActualCache (int port)
{
    if (port >= CPLD_MAX_PORT)
        return (0);

    return(rps_priority_state[port].priority_actual);
}

int32_t 
getPowerRequested (int32_t port)
{
    if (port >= CPLD_MAX_PORT)
        return (0);

    return (rps_priority_state[port].req_12V + 
            rps_priority_state[port].req_48V);
}

int32_t 
get12VPowerRequested (int32_t port)
{
    if (port >= CPLD_MAX_PORT)
        return (0);

    return(rps_priority_state[port].req_12V);
}

int32_t 
get48VPowerRequested (int32_t port)
{
    if (port >= CPLD_MAX_PORT)
        return (0);

    return(rps_priority_state[port].req_48V);
}

int32_t 
getPriority (int32_t port, uint8_t *data)
{
    uint8_t temp;
    
    if (port >= CPLD_MAX_PORT)
        return (-1);

    if (cpld_read_direct(CPLD_FIRST_PRIORITY_REG + port/2, &temp, 1))
        return (-1);
        
    *data = (port % 2) ? ((temp & 0x38) >> 3) : (temp & 0x07);
    
    return (0);
}

int32_t 
setPriority (int32_t port, uint8_t value)
{
    uint8_t temp, newPriority;
    
    if (port >= CPLD_MAX_PORT)
        return (-1);

    if (cpld_read(CPLD_FIRST_PRIORITY_REG + port/2, &temp, 1))
        return (-1);
        
    newPriority = (port % 2) ? ((temp & 0x07) | ((value & 0x07) <<3))
                        : ((temp & 0x38) | (value & 0x07));
    
    if (cpld_reg_write(CPLD_FIRST_PRIORITY_REG + port/2, newPriority))
        return (-1);
    return (0);
}

uint8_t 
getMaxActiveCache (void)
{
    return (max_active);
}

int32_t 
getMaxActive (uint8_t *data)
{
    uint8_t temp;
    
    if (cpld_read(MAX_ACTIVE, &temp, 1))
        return (-1);
        
    *data = temp & 0x03;
    
    return (0);
}

int32_t 
setMaxActive (uint8_t value)
{
    uint8_t temp;

    /* 
     * Value is the scale factor, can be only 1 or 2 
     * Default is 1 
     */
    
    if (cpld_read(MAX_ACTIVE, &temp, 1))
        return (-1);
    
    temp &= 0xf0;
    if (value == SCALE_2) 
        temp |= SCALE_2_VAL; 
    else
        temp |= SCALE_1_VAL;

    if (cpld_reg_write(MAX_ACTIVE, temp))
        return (-1);
    
    return (0);
}

void 
initPriority (void)
{
    int i;
    
    memset(rps_priority_state, 0, CPLD_MAX_PORT * sizeof(tPower));
    memset(rps_priority_port, 0, CPLD_MAX_PORT * sizeof(tPriority));

    for (i = 0; i < CPLD_MAX_PORT; i++) {
        rps_priority_port[i].port = CPLD_MAX_PORT - i;
    }
}

void 
priorityRemap (void)
{
    uint8_t currPri[3], newPri[CPLD_MAX_PORT];
    int i, j, k = 0, newPriCount = 0, priMapped = RPS_MAX_PRIORITY;

    if (cpld_read_direct(CPLD_FIRST_PRIORITY_REG, currPri, 3))
        return;
    
    memset(newPri, 0xff, CPLD_MAX_PORT);
    memset(rps_priority_port, 0, CPLD_MAX_PORT * sizeof(tPriority));

    for (i = 0; i < CPLD_MAX_PORT; i++) {
        rps_priority_port[i].port = CPLD_MAX_PORT - i;
        rps_priority_state[i].priority_remapped = 0;
    }

    /* remapping priority 6->0 */
    for (i = RPS_MAX_PRIORITY; i >= 0; i--) {
        for (j = CPLD_MAX_PORT-1; j >= 0; j--) {
            if (newPri[j] != 0xff)
                continue;
            
            if (rps_priority_state[j].priority_requested == i) {
                if (rps_priority_state[j].priority_requested) {
                    if (isPortUp(j)) {
                        rps_priority_state[j].priority_remapped = priMapped;
                        priMapped--;
                    }
                    else {
                        rps_priority_state[j].priority_requested = 0;
                        rps_priority_state[j].req_12V = 0;
                        rps_priority_state[j].req_48V = 0;
                        rps_priority_state[j].priority_remapped = 0;
                        switchSN[j][0] = '\0';
                    }
                }
                else {
                    rps_priority_state[j].priority_remapped = 0;
                }
                rps_priority_state[j].priority_actual = 
                                ((currPri[j/2] & (7<<((j%2)*3))) >> ((j%2)*3));
                if (rps_priority_state[j].priority_actual != 
                    rps_priority_state[j].priority_remapped) {
                    newPri[j] = rps_priority_state[j].priority_remapped;
                    newPriCount++;
                }
            }
        }
    }

    /* build priority_remapped high(6) to low(0) array */
    for (i = RPS_MAX_PRIORITY; i >= 0; i--) {
        for (j = CPLD_MAX_PORT-1; j >= 0; j--) {
            if (rps_priority_state[j].priority_remapped == i) {
                rps_priority_port[k].port = j;
                rps_priority_port[k].priority= 
                    rps_priority_state[j].priority_remapped;
                k++;
            }
            if (k>CPLD_MAX_PORT)
                break;
        }
    }

    if (newPriCount) {
        for (i = 0; i < CPLD_MAX_PORT; i++) {
            if (newPri[i] != 0xff) {
                if (setPriority(i, newPri[i]) == 0) {
                    rps_priority_state[i].priority_actual = newPri[i];
                }
            }
        }
    }
    
    return;
}

void 
priorityControl (int32_t port, int32_t pri, int32_t pwr_12V, int32_t pwr_48V)
{

    if (port >= CPLD_MAX_PORT)
        return;
    
    if (pri > RPS_MAX_PRIORITY)
        return;

    rps_priority_state[port].priority_requested = pri;
    rps_priority_state[port].req_12V = pwr_12V;
    rps_priority_state[port].req_48V = pwr_48V;

    priorityRemap();
    return;
}

void 
pri_Handler (ptData ptdata)
{
    if (getPortState(ptdata->param[0]) != 2)
        return;
    
    setPriority(ptdata->param[0], 0);
    udelay(1000);
    priorityControl
        (ptdata->param[0],
         ptdata->param[1],
         ptdata->param[2] >> 16,
         ptdata->param[2] & 0xFFFF);
}

void 
display_priority (void)
{
    int i;
    extern void rps_show_priority(void);

    rps_show_priority();
    
    printf("\nPort    Pri_map\n");
    for (i = 0; i < CPLD_MAX_PORT; i++) {
        printf("%4d    %4d\n",
            rps_priority_port[i].port, 
            rps_priority_port[i].priority);
    }
    
}

/* 
 *
 * portpri commands
 * 
 */
int 
do_portpri (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char *cmd = "status";  /* default command */
    
    if (argc <= 1)
        goto usage;
    
    if (argc >= 2) cmd   = argv[1];

    if (!strncmp(cmd,"status", 4)) { display_priority(); }
    else goto usage;
    return (1);

 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return (1);
}

U_BOOT_CMD(
    portpri,    2,  1,  do_portpri,
    "portpri - priority status command\n",
    "\n"
    "portpri status\n"
    "    - port priority status\n"
);

