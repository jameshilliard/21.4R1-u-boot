/*
 * $Id$
 *
 * cmd_acx.c -- ACX specific uboot commands
 *
 * Rajat Jain, Feb 2011
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
#include <acx_syspld.h>
#ifdef CFG_HUSH_PARSER
#include <hush.h>
#endif

extern int acx_upgrade_image(int);

int temp_sens_read (unsigned long sensor_addr, unsigned int channel, 
		    signed char *data);
extern int is_external_usb_present(int *dev);
extern void nvram_set_bootdev_usb(int dev);
extern void acx500_compare_banks(void);
extern int is_acx500_board(void);

int 
do_activebank (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    if (argc == 1) {
	printf("Curently Active Logical Bank = bank%d\n", syspld_get_logical_bank());
	return 0;
    } else if (argc == 2) {
	if (strcmp(argv[1], "bank0") == 0) {
	    printf("Setting Currently Active Logical Bank to %s...\n", argv[1]);
	    syspld_set_logical_bank(0);
	}
	else if (strcmp(argv[1], "bank1") == 0) {
	    printf("Setting Currently Active Logical Bank to %s...\n", argv[1]);
	    syspld_set_logical_bank(1);
	}
	else if (strcmp(argv[1], "compare") == 0) {
	    if (is_acx500_board()) {
		acx500_compare_banks();
		return 0;
	    } else {
		printf("\n not valid option on this ACX type board");
	    }
	}
	else
	    goto usage;
	printf("Curently Active Logical Bank = bank%d\n", syspld_get_logical_bank());
	return 0;
    } 
usage: 
    printf("Usage:\nactivebank\n");
    printf("%s\n", cmdtp->help);
    return 1;
}

/*
 * syscpld commands
 */
int 
do_syspld (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{

    char cmd1;
    uint32_t reg;
    u_int8_t  value;
    
    if (argc < 2)
        goto usage;
    
    cmd1 = argv[1][0];
    
    switch (cmd1) {
        case 'd':       /* dump */
        case 'D':
            syspld_reg_dump_all();
            break;

        case 'r':       /* read */
        case 'R':
            if (argc != 3)
                goto usage;
            
            reg = simple_strtoul(argv[2], NULL, 16);
            value = syspld_reg_read(reg);
	        printf("syspld[0x%x] = 0x%x\n", reg, value);
            break;
                        
        case 'w':       /* write */
        case 'W':
            if (argc != 4)
                goto usage;
            
            reg = simple_strtoul(argv[2], NULL, 16);
            value = simple_strtoul(argv[3], NULL, 16);
            syspld_reg_write(reg, value);
            break;
                        
        default:
            printf("???");
            break;
    }

    return 1;
 usage:
    printf("Usage:\n%s\n", cmdtp->usage);
    return 1;
}

int
do_bootfrom (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int dev;
	int ret = 0;

	if (argc != 2) {
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	if (strcmp("usb", argv[1])) {
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	if (!is_external_usb_present(&dev)) {
		printf("Error: No devices plugged in external USB slot.\n");
		return 1;
	} else {
		nvram_set_bootdev_usb(dev);
	}

#ifndef CFG_HUSH_PARSER
	if (run_command(getenv ("bootcmd"), flag) < 0) {
		ret = 1;
	}
#else
	if (parse_string_outer(getenv("bootcmd"),
			       FLAG_PARSE_SEMICOLON | FLAG_EXIT_FROM_LOOP)
	    != 0 ) {
		ret = 1;
	}
#endif

	return ret;
}

int 
do_temp_sens_read (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uint8_t sensor, channel;
    int8_t temp;
    int return_val = 0;

    if (argc != 3) {
	printf ("Usage:\n%s\n", cmdtp->usage); 
	return 1;
    } 
    sensor   = simple_strtoul(argv[1], NULL, 10);
    channel  = simple_strtoul(argv[2], NULL, 10);
    return_val = temp_sens_read (sensor, channel, &temp);
    if (return_val == -1) {
	printf("Error in reading temperature\n");
	return -1; 
    } 
    printf("Temperature is %d degrees C / %d degrees F\n", temp, (((temp * 9)/5) + 32)); 
    return 0;
}

/***************************************************/
 
U_BOOT_CMD( 
	   tempread, 3,    1,     do_temp_sens_read,
	   "tempread  sensor channel   - \n",
	   "Read the temperature value of a channel of particular"
	   "temperature sensor "
	   );

U_BOOT_CMD(
	   activebank,	CFG_MAXARGS,	0,	do_activebank,
	   "activebank    - Report / change the Active Flash Logical Bank\n",
	   "\n         - Report the current Active logical bank\n"
	   "activebank bank0\n"
	   "         - Make the bank0 as the currently active bank\n"
	   "activebank bank1\n"
	   "         - Make the bank1 as the currently active bank\n"
	   );

U_BOOT_CMD(
	   syspld,    7,    1,     do_syspld,
	   "syspld - ACX syspld register read/write utility\n",
	   "\n"
	   "syspld read <addr>\n"
	   "    - read reg from syspld \n"
	   "syspld write <addr> <val> \n"
	   "    - write <val> to syspld register \n"
	   "syspld dump \n"
	   "    - syspld register dump \n"
	   );

U_BOOT_CMD(
	   bootfrom,  2,    0,     do_bootfrom,
	   "bootfrom - boot from the specified boot device\n",
	   "bootfrom usb\n"
	   "         - boot from the plugged in usb\n"
	   );
