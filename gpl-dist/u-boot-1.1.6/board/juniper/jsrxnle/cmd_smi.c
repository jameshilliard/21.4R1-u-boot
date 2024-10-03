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
 * JSRXNLE SMI support
 */
#include <common.h>
#include <command.h>
#include <dxj106.h>

int do_smi (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
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
            uint32_t reg_addr  = (uint32_t)simple_strtoul(argv[2], NULL, 16);

            printf("smi_read: reg 0x%x\n", reg_addr);
            dxj106_read_reg(reg_addr);
        } else if (strcmp(argv[1],"stat") == 0) {
            uint32_t port  = (uint8_t)simple_strtoul(argv[2], NULL, 16);
            
            dxj106_port_stats(port);
        } else {
            printf("Usage:\n%s\n", cmdtp->usage);
           rcode = 1;
        }
        break;
        
    case 4:
        if (strcmp(argv[1],"write") == 0) {
            uint32_t reg_addr  = (uint32_t)
                                 simple_strtoul(argv[2], NULL, 16);
            uint32_t dat  = (uint32_t)
                            simple_strtoul(argv[3], NULL, 16);

            octeon_smi_write(reg_addr, dat);
            printf("smi_write: reg 0x%x  data wrote 0x%x\n",
                    reg_addr, dat);
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
        smi,  5,  1,  do_smi,
        "smi     - peek/poke SMI devices\n",
        "smi read  <register>\n"
        "    - read the specified SMI register\n"
        "smi stat  <port>\n"
        "    - stats of the particular switch port\n"
        "smi write <register> <value>\n"
        "    - write 'value' to the specified SMI register\n"
);
