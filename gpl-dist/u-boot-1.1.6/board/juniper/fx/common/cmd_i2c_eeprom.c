/*
 * Copyright (c) 2009-2010, Juniper Networks, Inc.
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
 * You should have received a copy of the GNU General Public
 * License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <asm/byteorder.h>

extern void eeprom_log_init();
extern int print_eeprom(const char *fmt, ...);
extern int erase_eeprom_log();
extern int eeprom_log_dump();

int
do_dump ()
{
    eeprom_log_dump();
}


int
do_hw ()
{
    int i = 1;

    printf("in do_hw");

    while (eeprom_printf("Hello World %d:", i) > 0) {
        printf("\nWritten mgs %d", i);
        i++;
    }

    return 0;
}


int
do_erase ()
{
    erase_eeprom_log();
    return 0;
}


U_BOOT_CMD(
    hw,  1,      1,      do_hw,
    "hw   - dump test messages to eeprom\n",
    "no args\n"
    "    - user defined implementation\n"
    );


U_BOOT_CMD(
    erase_log,  1,      1,      do_erase,
    "erase_log  - erase eeprom log\n",
    "no args\n"
    "    - user defined implementation\n"
    );


U_BOOT_CMD(
    dump_log,  1,      1,      do_dump,
    "dump_log  - dump eeprom log\n",
    "no args\n"
    "    - user defined implementation\n"
    );

