/*
 * Copyright (c) 2020, Juniper Networks, Inc.
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
#include <config.h>
#include <watchdog.h>
#include <command.h>
#include <watchdog_cpu.h>
#include <cvmx.h>
#include <lib_octeon_shared.h>
#include <octeon_boot.h>
#include <platform_srxsme.h>
#include <exports.h>


/*
* Valid loaderdev format:
* Valid device strings:                     For device types:
*
* <type_name>                               DEV_TYP_STOR, DEV_TYP_NET
* <type_name><unit>                         DEV_TYP_STOR, DEV_TYP_NET
* <type_name><unit>:                        DEV_TYP_STOR, DEV_TYP_NET
* <type_name><unit>:<slice>                 DEV_TYP_STOR
* <type_name><unit>:<slice>.                DEV_TYP_STOR
* <type_name><unit>:<slice>.<partition>     DEV_TYP_STOR
*/
void
update_loaderdev (void)
{
    DECLARE_GLOBAL_DATA_PTR;
    unsigned char bootdev[MAX_DISK_STR_LEN] = {0};

    srxsme_get_next_bootdev(bootdev, 0);
    /* the bootdev always format like disk0 or disk1s1 */
    if (bootdev[5] == 's') {
	bootdev[5] = ':';
    }

    if (IS_PLATFORM_SRX550(gd->board_desc.board_type)) {
        uint32_t usb_disk_num;
	usb_disk_num = srxsme_get_num_usb_disks();

        /* for srx550, disk0 means USB */
        if (strncmp(bootdev, "disk0", strlen("disk0")) == 0 && usb_disk_num == 0) {
            printf("Request boot from USB, "
		   "but no USB disk exist, "
		   "boot from internal-CF... \r\n");
            setenv("loaderdev", "disk1");
            return;
        }
    }
    setenv("loaderdev", bootdev);
}

static int
do_nextboot(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int i;
    unsigned char bootdev[64];

    switch (argc) {
    case 0:
    case 1:
        printf ("Usage:\n%s\n", cmdtp->usage);
        srxsme_show_bootdev_list();

        srxsme_get_next_bootdev(bootdev, 0);
        printf("Current bootdev: %s\r\n", bootdev);
        printf("Current act slice: %d\r\n", get_active_slice(0));

        return 1;

    case 2:
        if (srxsme_is_valid_devname(argv[1])) {
            srxsme_set_next_bootdev(argv[1]);
            update_loaderdev();
            printf("Set next boot device: %s\r\n", argv[1]);
            return 0;
        }

        srxsme_show_bootdev_list();
        return 1;

    default:
        srxsme_show_bootdev_list();
        printf ("Usage:\n%s\n", cmdtp->usage);
        return 1;
    }
}

U_BOOT_CMD(
        nextboot, 2, 1, do_nextboot,
        "nextboot - get/set nextboot",
        "nextboot   \n"
        "    - read nextboot device\n"
        "nextboot  <dev>\n"
        "    - set nextboot device\n"
        );
