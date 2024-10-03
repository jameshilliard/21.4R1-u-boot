/*
 * Copyright (c) 2007-2011, Juniper Networks, Inc.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
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
#include "rps_psMon.h"
#include "rps_envMon.h"
#include "rps_portMon.h"
#include "rps_led.h"
#include "rps_priority.h"
#include "rps_mbox.h"
#include "rps_msg.h"

extern char *rps_model;
extern char rps_sn[];
extern char uboot_version_string[];
extern char *rps_version;
extern uint32_t upSec;
extern int gI2cCpuError;
extern int gI2cCpuReset;
extern int gI2cPldError;
extern int gI2cPldReset;

void
rps_show_version (void)
{
    printf("RPS-%s\n", rps_sn);
    printf("     Model: %s\n", rps_model);
    printf("     RPS Firmware Version [%s]\n", rps_version);
    printf("     RPS U-boot   Version [%s]\n", 
           uboot_version_string);
}

void 
rps_show_powerSupply (void)
{
    uint16_t id;
    int i;
    uint8_t   psu_pwr_good;
    uint16_t  psState;

    psState = getPSStatus();
    printf("RPS-%s\n", rps_sn);
    printf("PSU Slot Status    Description\n");
    for (i = 0; i < CPLD_MAX_PS; i++) {
        id = getPSID(i);
	psu_pwr_good = (psState & (1<<(i*3) + 1)) ? 1 : 0; 

    if (id == I2C_ID_EX32X42X_PWR_320) {

		if (!psu_pwr_good) {
			printf("%8d Offline   320W AC\n", i);

		} else {
			printf("%8d Online    320W AC\n", i);
		}
	}
        else if (id == I2C_ID_EX32X42X_PWR_600) {
		if (!psu_pwr_good) {
			printf("%8d Offline   600W AC\n", i);

		} else {
			printf("%8d Online    600W AC\n", i);
		}
        }
        else if (id == I2C_ID_EX32X42X_PWR_930) {
		if (!psu_pwr_good) {
			printf("%8d Offline   930W AC\n", i);

		} else {
			printf("%8d Online    930W AC\n", i);
		}
        }
        else if (id == I2C_ID_EX32X42X_PWR_190_DC) {
		if (!psu_pwr_good) {
			printf("%8d Offline   190W DC\n", i);

		} else {
			printf("%8d Online    190W DC\n", i);
		}
        }
        else {
		        printf("%8d Empty     ---\n", i);
        }
    }
}

void 
rps_show_led (void)
{
    int i = 0;
    
    printf("RPS-%s\n", rps_sn);
    printf("    RPS Fan: %s\n", get_fanfail_led_state());
    printf("    RPS System Status: %s\n", get_rps_led_state());
    printf("RPS Port LED Status\n");
    printf("RPS Port  Status\n");
    for (i = 0; i < CPLD_MAX_PORT; i++)
        printf("%8d %s\n", i, get_port_led_state(i));
}

void 
rps_show_status (void)
{
    int i = 0;

    printf("RPS-%s\n", rps_sn);
    printf("RPS Port Status          Priority  Power-Requested\n");
    for (i = 0; i < CPLD_MAX_PORT; i++) {
        printf("%8d  %-16s%-8d  %4dW\n", i, 
            portStateString(getPortState(i)), 
            getPriorityRequested(i),
            getPowerRequested(i));
    }
}

void 
rps_show_priority (void)
{
    int i = 0;
    uint8_t val;

    printf("RPS Port    Status          Pri_req  Pri_map  Pri_pld"
           "    Power-Requested\n");
    for (i = 0; i < CPLD_MAX_PORT; i++) {
        printf("%8d    %-16s   %4d   %4d   %4d        %4dW\n", i, 
            portStateString(getPortState(i)), 
            getPriorityRequested(i),
            getPriorityRemapped(i),
            getPriorityActualCache(i),
            getPowerRequested(i));
    }
}

void 
rps_show_uptime (void)
{
    uint32_t rdays = upSec/86400;
    uint32_t rhours = upSec % 86400;
    uint32_t rmins = upSec % 86400;
    uint32_t rsecs = 0;

    rhours = rhours /3600;
    rsecs = rmins = rmins % 3600;
    rmins = rmins /60;
    rsecs = rsecs % 60;

    printf("\nUptime (approx): %4d days %2d hour %2d min %2d sec\n",
        rdays, rhours, rmins, rsecs);
}

int 
do_rps_show (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char *cmd1 = "redundant-power-system";
    char *cmd2 = "status";
    
    if (argc < 3)
        goto usage;
    
    if (argc >= 3) {cmd1 = argv[1]; cmd2 = argv[2];}
    
    if (strcmp(cmd1,"redundant-power-system")) {
        goto usage; 
    }
    else {
        if (!strcmp(cmd2,"version")) { 
            rps_show_version();
        }
        else if (!strcmp(cmd2,"led")) {
            rps_show_led();
        }
        else if (!strcmp(cmd2,"status")) { 
            rps_show_status();
        }
        else if (!strcmp(cmd2,"power-supply")) { 
            rps_show_powerSupply();
        }
        else if (!strcmp(cmd2,"priority")) { 
            rps_show_priority();
        }
        else if (!strcmp(cmd2,"uptime")) { 
            rps_show_uptime();
        }
        else
            goto usage; 
    }

    return 1;
 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return 1;
}


int 
do_rps_set (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char *cmd1 = "redundant-power-system";
    char *cmd2 = "port";
    int port = 0, priority = 0, power_12V = 0, power_48V = 0;
    
    if (argc < 9)
        goto usage;
    
    if (strcmp(cmd1,"redundant-power-system"))
        goto usage; 
    if (strcmp(cmd2,"port"))
        goto usage; 

    port            = simple_strtoul(argv[3], NULL, 10);
    priority        = simple_strtoul(argv[5], NULL, 10);
    power_12V   = simple_strtoul(argv[7], NULL, 10);
    power_48V   = simple_strtoul(argv[8], NULL, 10);
    
    priorityControl(port, priority, power_12V, power_48V);

    return 1;
 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return 1;
}

static int 
atoh (char* string)
{
    int res = 0;

    while (*string != 0)
    {
        res *= 16;
        if (*string >= '0' && *string <= '9')
            res += *string - '0';
        else if (*string >= 'A' && *string <= 'F')
            res += *string - 'A' + 10;
        else if (*string >= 'a' && *string <= 'f')
            res += *string - 'a' + 10;
        else
            break;
        string++;
    }

    return res;
}

/* I2C commands
 *
 * Syntax:
 */
int 
do_i2c (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char cmd1;
    char *cmd2 = "port";
    ulong port = 0;
    int i = 0;
    uint8_t mbox[2] = {0, 0x40};  /* RPS up */

    if (argc < 2)
        goto usage;
    
    cmd1 = argv[1][0];
    if ((cmd1 == 'r') ||(cmd1 == 'R')) { /* reset */
        cmd2 = argv[2];
        if (!strcmp(cmd2,"cpu")) { 
            i2c_reset();
        }
        else if (!strcmp(cmd2,"cpld")) { 
            i2c_cpld_reset();
        }
        else if (!strcmp(cmd2,"port")) { 
            port = simple_strtoul(argv[4], NULL, 16);
            resetPortI2C(port);
            msg_state[port] = POLLING;
            set_mbox(port, mbox);
        }
        return 1;
    }
    else if ((cmd1 == 'e') ||(cmd1 == 'E')) { /* error */
        if (argc < 3)
            goto usage;

        if ((argv[2][0] == 'g') ||(argv[2][0] == 'G')) {
            printf("I2C error protocol : %d\n", getI2CErrorProtocol());
            printf("I2C error address  : %d\n", getI2CErrorAddress());
            printf("I2C CPU error      : %d\n", gI2cCpuError);
            printf("I2C reset          : %d\n", gI2cCpuReset);
            printf("PLD error          : %d\n", gI2cPldError);
            printf("PLD reset          : %d\n", gI2cPldReset);
        }
        else {
            clearI2CErrorProtocol();
            clearI2CErrorAddress();
            gI2cCpuReset = 0;
            gI2cPldReset = 0;
        }            
        return 1;
    }
    else if ((cmd1 == 't') ||(cmd1 == 'T')) {  /* timeout */
        if (argc < 3)
            goto usage;

        if ((argv[2][0] == 'g') ||(argv[2][0] == 'G')) {
            if (argc == 3) {
                for (i = 0; i < CPLD_MAX_PORT; i++) {
                    printf("port %d i2c timeout : %d\n", i, 
                           getPortI2CTimeout(i));
                }
            }
            else {
                port = simple_strtoul(argv[3], NULL, 16);
                if (port < CPLD_MAX_PORT) {
                    printf("port %d i2c timeout : %d\n", port, 
                           getPortI2CTimeout(port-1));
                }
            }
        }
        else {
            if (argc == 3) {
                for (i = 0; i < CPLD_MAX_PORT; i++) {
                    clearPortI2CTimeout(i);
                }
            }
            else {
                port = simple_strtoul(argv[3], NULL, 16);
                if (port < CPLD_MAX_PORT) {
                    clearPortI2CTimeout(port);
                }
            }
        }
        return 1;
    }

    return 1;
 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return 1;
}
/***************************************************/


U_BOOT_CMD(
    show,    3,  1,  do_rps_show,
    "show    - redundant-power-system show commands\n",
    "\n"
    "show redundant-power-system version\n"
    "    - Redundant Power System version\n"
    "show redundant-power-system power-supply\n"
    "    - Redundant Power System power supply status\n"
    "show redundant-power-system led\n"
    "    - Redundant Power System LED state\n"
    "show redundant-power-system status\n"
    "    - Redundant Power System status\n"
    "show redundant-power-system priority\n"
    "    - Redundant Power System priority\n"
    "show redundant-power-system uptime\n"
    "    - Redundant Power System uptime\n"
);

U_BOOT_CMD(
    set,    9,  1,  do_rps_set,
    "set     - redundant-power-system configuration set commands\n",
    "\n"
    "set redundant-power-system port <no> priority <0..6> "
    "power-request <12V> <48V>\n"
    "    - set port 0-5 priority and power request\n"
);

U_BOOT_CMD(
    i2c,    4,  1,  do_i2c,
    "i2c     - I2C test commands\n",
    "\n"
    "i2c reset [cpu | cpld | port <number>]\n"
    "    - reset i2c [port 0..5]\n"
    "i2c timeout [get | clear] [<port>]\n"
    "    - i2c port [0..5] timeout count\n"
    "i2c error [get | clear]\n"
    "    - i2c error count\n"
);

