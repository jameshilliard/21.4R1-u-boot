/*
 * $Id$
 *
 * cmd_acx.c -- ACX specific uboot commands
 *
 * Rajat Jain, Feb 2011
 * Samuel Jacob, Sep 2011
 *
 * Copyright (c) 2011-2012, Juniper Networks, Inc.
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
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/gpl-2.0.html>.
 */

#include <common.h>
#include <command.h>
#include "syspld.h"

int do_activebank (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	if (argc == 1) {
		printf("Active logical bank = bank%d\n", syspld_get_logical_bank());
		return 0;
	}

	if (strcmp(argv[1], "bank0") == 0) {
		syspld_set_logical_bank(0);
	}
	else if (strcmp(argv[1], "bank1") == 0) {
		syspld_set_logical_bank(1);
	}
	else {
		cmd_usage(cmdtp);
		return 1;
	}
	printf("Changed to active logical bank to %s\n", argv[1]);
	return 0;
}

/*
 * syscpld commands
 */
int do_syspld(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	uint32_t reg;
	u_int8_t  value;
    
	if (argc < 2) {
		cmd_usage(cmdtp);
		return 1;
	}

	if (strcmp(argv[1], "dump") == 0) {
		syspld_reg_dump_all();
	} else if (strcmp(argv[1], "read") == 0) {
	    	if (argc != 3) {
		    cmd_usage(cmdtp);
		    return 1;
		}
		reg = simple_strtoul(argv[2], NULL, 16);
		value = syspld_reg_read(reg);
		printf("syspld[0x%x] = 0x%x\n", reg, value);
	} else if (strcmp(argv[1], "write") == 0) {
	    	if (argc != 4) {
		    cmd_usage(cmdtp);
		    return 1;
		}
		reg = simple_strtoul(argv[2], NULL, 16);
		value = simple_strtoul(argv[3], NULL, 16);
		syspld_reg_write(reg, value);
	} else {
	    	cmd_usage(cmdtp);
		return 1;
	}

	return 0;
}

U_BOOT_CMD(
	activebank, 2, 0, do_activebank,
	"[bank0|bank1] - Print/change the Active Flash Logical Bank",
	" - Prints the active logical bank\n"
	"activebank <bank0 | bank1> - Changes the active bank to either bank0 or bank1"
);

U_BOOT_CMD(
	syspld, 7, 1, do_syspld,
	"Read/write syspld registers",
	" - read <addr>\n"
	"syspld write <addr> <val> \n"
	"syspld dump \n"
);

