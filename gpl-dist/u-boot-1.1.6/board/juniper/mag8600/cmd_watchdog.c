/*
 * Copyright (c) 2011, Juniper Networks, Inc.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. if not ,see
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0,html>  
 */


#include <watchdog_cpu.h>
#include <common.h>
#include <command.h>

int32_t 
do_watchdog_cmd (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int32_t  rcode = 0;
    uint64_t val;

    switch (argc) {
    case 0:
    case 1:
        printf ("Usage:\n%s\n", cmdtp->usage);
        rcode = 1;
        break;

    case 2: 
        if (strcmp(argv[1], "show") == 0) {
            dump_ciu_watchdog_reg();

        } else if (strcmp(argv[1], "stop") == 0) {
            RELOAD_WATCHDOG(DISABLE_WATCHDOG);

        } else if (strcmp(argv[1], "start") == 0) {
            RELOAD_WATCHDOG(ENABLE_WATCHDOG);

        } else if (strcmp(argv[1], "pat") == 0) {
            RELOAD_WATCHDOG(PAT_WATCHDOG);

        } else {
            printf("Usage:\n%s\n", cmdtp->usage);
            rcode = 1;
        }
        break;

    default:
        printf("Usage:\n%s\n", cmdtp->usage);
        rcode = 1;
        break;
    }
    return rcode;
}

U_BOOT_CMD(
    watchdog,      3,      0,      do_watchdog_cmd,
    "watchdog <start | stop | show | pat>\n",
    "start: starts cpu watchdog\n"
    "stop:  stop cpu watchdog\n"
    "show:  shows wathcdog register\n"
    "pat:   tickle watchdog\n"
);
