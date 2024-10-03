/*
 * Copyright (c) 2007-2010, Juniper Networks, Inc.
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
 * CPLD support for JSRXNLE boards
 */
#include <common.h>
#include <command.h>

int do_cpld (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int rcode = 0;

    switch (argc) {
    case 0:
    case 1:
    case 2:
	printf ("Usage:\n%s\n", cmdtp->usage);
	rcode = 1;
        break;
            
    case 3:
        if (strcmp(argv[1],"read") == 0) {
            uint8_t reg_addr  = (uint8_t)simple_strtoul(argv[2], NULL, 16);
            uint8_t val;
    
            val = cpld_read(reg_addr);
            printf("reading cpld: register 0x%x val 0x%x\n", reg_addr, val);

        } else {
            printf("Usage:\n%s\n", cmdtp->usage);
            rcode = 1;
        }
        break;
        
    case 4:
	if (strcmp(argv[1],"write") == 0) {
            uint8_t reg_addr  = (uint8_t)
                                simple_strtoul(argv[2], NULL, 16);
            uint8_t val  = (uint8_t)
                            simple_strtoul(argv[3], NULL, 16);
    
            cpld_write(reg_addr, val);
            printf("wrote to cpld: register 0x%x val 0x%x\n", reg_addr, val);
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
        cpld,  5,  1,  do_cpld,
        "cpld     - peek/poke CPLD\n",
        "read  <register>\n"
        "    - read the specified CPLD register\n"
        "write <register> <value>\n"
        "    - write 'value' to the specified CPLD register\n"
);
