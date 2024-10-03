/*
 * $Id: ls609x_uboot_drv_cmd.c,v 1.2.228.1 2009-08-27 06:23:45 dtang Exp $
 *
 * ls609x_uboot_drv_cmd.c - 88E6097 basic driver uboot cmd
 * for debugging
 *
 * Richa Singla, January-2009
 *
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
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>
 */

#include <common.h>
#include <command.h>

#include "ls609x_uboot_drv.h"



int 
do_ls609x (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int       rcode = 0;
    uint16_t smi_addr; 
    uint16_t reg_adr;
    uint16_t port;
    uint16_t val;

    switch (argc) {
    case 0:
    case 1:
    case 2:
        printf("Usage:\n%s\n", cmdtp->usage);
        rcode = 1;
        break;

    case 3:
       if (strcmp(argv[1],"stats") == 0) {

            port = simple_strtoul(argv[2], NULL, 16);
            ls609x_drv_eth_getstats(port);

        } else  if ((strcmp(argv[1],"diag") == 0) &&
                  (strcmp(argv[2],"enable") == 0)){

            ls609x_uboot_drv_diag_init();

        } else  if ((strcmp(argv[1],"diag") == 0) &&
                  (strcmp(argv[2],"disable") == 0)){

            ls609x_uboot_drv_diag_deinit();

        } else {
            printf("Usage:\n%s\n", cmdtp->usage);
            rcode = 1;
        }
        break;


    case 4: 
        if (strcmp(argv[1],"read") == 0) {

            smi_addr = simple_strtoul(argv[2], NULL, 16);
            reg_adr  = simple_strtoul(argv[3], NULL, 16);
            ls609x_rd_reg (smi_addr, reg_adr, &val);

            printf(" Value of register is 0x%x \n", val);
        }
        else {
            printf("Usage:\n%s\n", cmdtp->usage);
            rcode = 1;
        }
        break;

    case 5:
        if (strcmp(argv[1],"write") == 0) {
            smi_addr = simple_strtoul(argv[2], NULL, 16);
            reg_adr  = simple_strtoul(argv[3], NULL, 16);
            val      = simple_strtoul(argv[4], NULL, 16);

            ls609x_wr_reg (smi_addr, reg_adr, val);

            ls609x_rd_reg (smi_addr, reg_adr, &val);
            printf(" Value written to register is 0x%x \n", val);
        
        }
        else {
            printf("Usage:\n%s\n", cmdtp->usage);
            rcode = 1;
        }
        break;

    default:
        printf("Usage:\n%s\n", cmdtp->usage);
        rcode = 1;
        break;
    }   
}

U_BOOT_CMD(
	ls609x,      5,      0,      do_ls609x,
	"ls609x_read_reg - Read 88E6097 register\n",
	" [smi address]  -  smi address.\n"
	" [reg address]  -  reg address.\n"
);
