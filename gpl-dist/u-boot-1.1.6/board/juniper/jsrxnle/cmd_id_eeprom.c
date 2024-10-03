/*
 * Copyright (c) 2007-2011, Juniper Networks, Inc.
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


/*
 * ID EEPROM support for JSRXNLE boards
 */

#include <common.h>
#include <config.h>
#include <watchdog.h>
#include <command.h>
#include <image.h>
#include <asm/byteorder.h>
#include <octeon_boot.h>
#include <octeon_hal.h>
#include <i2c.h>

typedef unsigned char boolean_t;

#ifndef TRUE
#define TRUE ((boolean_t)(0==0))
#endif
#ifndef FALSE
#define FALSE (!TRUE)
#endif

DECLARE_GLOBAL_DATA_PTR;

uchar
read_eeprom(uchar offset)
{
    uchar val, group, dev_addr, err;

    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX650_MODELS
    CASE_ALL_SRX550_MODELS
        group = SRXSME_I2C_GROUP_MIDPLANE;
        dev_addr = MIDPLANE_BOARD_EEPROM_TWSI_ADDR;
        break;
    default:
        group = 0;
        dev_addr = BOARD_EEPROM_TWSI_ADDR;
        break;
    }

    if (JSRXNLE_I2C_SUCCESS == 
        (err = read_i2c(dev_addr, offset, &val, group))) {
        return val;
    }
    else {
        return JSRXNLE_I2C_ERR;
    }
}

uchar
write_eeprom(uchar offset, uchar val)
{
    uchar group, dev_addr;

    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX650_MODELS
    CASE_ALL_SRX550_MODELS
        group = SRXSME_I2C_GROUP_MIDPLANE;
        dev_addr = MIDPLANE_BOARD_EEPROM_TWSI_ADDR;
        break;
    default:
        group = 0;
        dev_addr = BOARD_EEPROM_TWSI_ADDR;
        break;
    }
    return (write_i2c(dev_addr, offset, val, group));
}

static void
eeprom_print_at24c (void)
{
    unsigned int i = 0;
    unsigned int val;
    
    while (i < 48) {
        val = read_eeprom(i);
        printf("0x%x ", val);
        if ((i % 10) == 0)
            printf("\n");
        i++;
    }
    printf("\n");
}

static int
do_eeprom_at24c (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int rcode;

    switch (argc) {
    case 0:
    case 1:
        printf ("Usage:\n%s\n", cmdtp->usage);
        rcode = 1;
        break;

    case 2:
        if (strcmp(argv[1],"read") == 0) {
            eeprom_print_at24c();
        } else {
            printf ("Usage:\n%s\n", cmdtp->usage);
            rcode = 1;
        }
        break;
    case 3:
        if (strcmp(argv[1],"read") == 0) {
            uint8_t val;
            uint8_t offset = (uint8_t)simple_strtoul(argv[2], NULL, 16);
            
            val = read_eeprom(offset);
            printf("read_eeprom: offset 0x%x value 0x%x\n", offset, val);
        } else {
            printf ("Usage:\n%s\n", cmdtp->usage);
            rcode = 1;
        }
        break;
    case 4:
        if (strcmp(argv[1],"write") == 0) {
            uint8_t offset = (uint8_t)simple_strtoul(argv[2], NULL, 16);
            uint8_t val = (uint32_t)simple_strtoul(argv[3], NULL, 16);
            
            if (!write_eeprom(offset, val)){
                printf( "write_eeprom: failed to write to offset 0x%x \n",
                        offset);
            }
            else {
                printf("write_eeprom: offset 0x%x value 0x%x\n", offset, val);
            }
        } else {
            printf ("Usage:\n%s\n", cmdtp->usage);
            rcode = 1;
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
	id_eeprom, 10, 1, do_eeprom_at24c,
	"id_eeprom     - peek/poke EEPROM\n",
	"id_eeprom read  <offset>\n"
	"    - read the specified EEPROM offset\n"
	"id_eeprom write <offset> <value>\n"
	"    - write 'value' at the specified EEPROM offset\n"
);