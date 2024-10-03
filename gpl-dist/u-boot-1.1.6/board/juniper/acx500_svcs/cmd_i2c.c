/*
 * Copyright (c) 2014, Juniper Networks, Inc.
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

#include <common.h>
#include <config.h>
#include <command.h>
#include <asm/byteorder.h>
#include <octeon_boot.h>
#include <octeon_hal.h>
#include <i2c.h>

unsigned char
read_i2c (unsigned char dev_addr, unsigned char offset,
          unsigned char *val, unsigned char group)
{
    DECLARE_GLOBAL_DATA_PTR;

    if (group != 1) {
        printf(" invalid group for this platform \n" );
        return 0;
    }

    if (i2c_read8_generic(dev_addr, offset, val, !!group)) {
        printf("ERR: Failed to read the I2C addr 0x%x \n", offset);
        return 0;
    }

    return 1;
}

unsigned char
write_i2c (unsigned char dev_addr, unsigned char offset,
           unsigned char val, unsigned char group)
{
    
    DECLARE_GLOBAL_DATA_PTR;

    if (group != 1) {
        printf(" invalid group for this platform \n" );
        return 0;
    }

    if (i2c_write8_generic(dev_addr, offset, 
                           val, !!group)) {
        printf("ERR: Failed to write the I2C addr 0x%x \n", offset);
        return 0;
    }

    return 1;
}

static int
do_i2c (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int     rcode;
    uint8_t val;
    uint8_t group;
    uint8_t dev_addr;
    uint8_t offset;

    switch (argc) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
        printf ("Usage:\n%s\n", cmdtp->usage);
        rcode = 1;
        break;
    case 5:
        if (strcmp(argv[1],"read") == 0) {
            group = (uint8_t)simple_strtoul(argv[2], NULL, 16);
            dev_addr = (uint8_t)simple_strtoul(argv[3], NULL, 16);
            offset = (uint8_t)simple_strtoul(argv[4], NULL, 16);

            rcode = read_i2c(dev_addr, offset, &val, group);
            if (rcode == I2C_FAILURE) {
                printf(" I2c read error\n");
            } else {
                printf(" Dev addr : 0x%x, offset 0x%x, value 0x%x\n",
                       dev_addr, offset, val);
            }
            break;
        } else {
            printf ("Usage:\n%s\n", cmdtp->usage);
            rcode = 1;
            break;
        }

    case 6:
        if (strcmp(argv[1],"write") == 0) {
            group = (uint8_t)simple_strtoul(argv[2], NULL, 16);
            dev_addr = (uint8_t)simple_strtoul(argv[3], NULL, 16);
            offset = (uint8_t)simple_strtoul(argv[4], NULL, 16);
            val = (uint8_t)simple_strtoul(argv[5], NULL, 16);

            rcode = write_i2c(dev_addr, offset, val, group);
            if (rcode == I2C_FAILURE) {
                printf( " I2c write error\n" );
            } else {
                printf(" Dev addr : 0x%x, offset 0x%x, written 0x%x\n",
                       dev_addr, offset, val);
            }
        }
        break;

    default:
        printf ("Usage:\n%s\n", cmdtp->usage);
        rcode = 1;
        break;
    }
    return rcode;
}

U_BOOT_CMD(
	i2c, 10, 1, do_i2c,
	"i2c - read/write on i2c bus\n",
	"i2c read <i2c group> <dev_addr> <offset> \n"
	"    - read the specified offset from dev_addr which belongs "
	"to specified i2c group\n"
	"i2c write <i2c group> <dev_addr> <offset> <val>\n"
	"    - write val to the specified offset from dev_addr which belongs "
	"to specified i2c group\n"
);
