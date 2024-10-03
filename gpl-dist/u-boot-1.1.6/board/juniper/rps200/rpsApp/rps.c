/*
 * Copyright (c) 2008-2013, Juniper Networks, Inc.
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
#include "rps_cli.h"
#include "rps_cpld.h"
#include "rps_envMon.h"
#include "rps_portMon.h"
#include "rps_psMon.h"
#include "rps_priority.h"
#include "rps_msg.h"
#include "rps_led.h"

int rps_loop = 1;
uint32_t oldticks;
uint32_t upSec = 0;
uint32_t accuTicks = 0;
uint32_t gTicksPerMS = 1;
uint32_t gMaxMS= MAX_TICKS;

char *rps_model = "EX-RPS-PWR";
char *rps_version = "1.1";  /* 1.1 */
/* misc */
char rps_sn[13];
char rps_version_string[12];
char uboot_version_string[12];

int 
read_SN (void)
{
    uint8_t ch = 1 << EEPROM_CHANNEL;
    memset(rps_version_string, 0x0, sizeof(rps_version_string));
    memset(uboot_version_string, 0x0, sizeof(uboot_version_string));
    memcpy(rps_version_string, rps_version,  sizeof(rps_version));
    setenv("cur_version", rps_version);
    memcpy(uboot_version_string, "1.1.6", sizeof("1.1.6"));

    i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
    if (i2c_read(0, CFG_EEPROM_ADDR, 0x20, 1, (uchar *)rps_sn, 12)) {
        ch = 0;
        i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
        return (-1);
    }

    ch = 0;
    i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);

    return (0);
}

void 
rps_cliHandler (ptData ptdata)
{
    cli_loop();
}

int 
rps (int argc, char *argv[])
{
    uint32_t ticks, tclk;
    int32_t tid, deltaT, oldDiff = 0;
    tData tdata;

    /* Print the ABI version */
    app_startup(argv);
    set_rps_led(LED_GREEN_BLINKING);

    printf("\n");
    printf("    ______   ______   _____ \n");
    printf("   |      \\ |      \\ / ____\\ \n");
    printf("   | |--) / | |--) / \\_\\ \n");
    printf("   | |-\\ \\  | |---    ___\\ \\ \n");
    printf("   |_|  \\_\\ |_|      \\_____/ \n");
    printf("\n");
    printf("\nFirmware Version - %s\n", rps_version);
    printf ("%s", RPS_PROMPT);

    tclk = get_tbclk();

    if (tclk >= 1000) {
        gTicksPerMS = tclk / 1000;
        gMaxMS = MAX_TICKS / gTicksPerMS;
    }

    read_SN();

    qInit();
    oldticks = ticks = get_timer(0) / gTicksPerMS;
    initEnv();
    initPSMon();
    initPortMon();
    initMsg();
    initPortPresentMon();
    initPriority();
    cpld_init();
    startEnvMon(150, 10000);
    startPortPresentMon(300, 170);
    startPortMon(500, 950);
    startPSMon(300, 1050);
    startMsgMon(1000, 100);
    
    tdata.param[0] = tdata.param[1] = tdata.param[2] = 0;
    addTimer(tdata, &tid, PERIDICALLY, 10, 10, rps_cliHandler);

    watchdog_init(5000);  /* set watchdog for 5 seconds */
    watchdog_enable();  /* enable watchdog */

    set_rps_led(LED_GREEN);
    if (!(strncmp(getenv("scale"), "yes", strlen("yes"))))     
        setMaxActive(SCALE_2);
    else
        setMaxActive(SCALE_1);

    while(rps_loop)
    {
        ticks = get_timer(0) / gTicksPerMS;
        /* taking care of tick wrap around */
        if (ticks < oldticks)
            deltaT = (gMaxMS - oldticks) + ticks;
        else
            deltaT = ticks - oldticks;

        /* uptime ticks algorithm */
        if (ticks < oldDiff)  
            accuTicks += (gMaxMS - oldDiff) + ticks;
        else
            accuTicks += (ticks - oldDiff);
        oldDiff = ticks;
        if (accuTicks > 1000) {
            upSec += accuTicks/1000;
            accuTicks %= 1000;
            watchdog_reset();  /* reset ~ every second */
        }

        /* task scheduling */
        if (deltaT >= MS_PER_TICK)
        {
            updateTimer(deltaT/MS_PER_TICK);
            oldticks = ticks;
        }
    }
    set_rps_led(LED_AMBER);
    udelay(10000);

    return (0);
}
