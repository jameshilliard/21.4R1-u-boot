/*
 * Copyright (c) 2009-2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#ifdef CONFIG_MX80
#ifdef CONFIG_POST

#include <configs/mx80.h>
#include <command.h>
#include <post.h>

#if CONFIG_POST & CFG_POST_CPLD
#define BOOTCPLD_TBB_RE_SCRATCH_1_ADDR 0x00000023
#define BOOTCPLD_TBB_RE_SCRATCH_2_ADDR 0x00000024

int cpld_post_test(int flags)
{

	int cpld_debug_flag;
	int boot_flag_post;
	int cpld_fail1 = 0;
	int cpld_fail2 = 0;
	uint8_t val;
	uint8_t reg_val;
	uint8_t str[] = "** TEST **";
	int i;

	if (flags & POST_DEBUG) {
		cpld_debug_flag = 1;
	}

	if (flags & BOOT_POST_FLAG) {
		boot_flag_post = 1;
	}

	printf("BOOT CPLD VERSION: %d\n",tbbcpld_version());
	printf("The test writes a value to scratch register, reads it and"
		" then compares both\n\n");

	/*
	 * Save the CPLD scratch1 register value as it is being used
	 * for storing next boot option
	 */
	if (tbbcpld_reg_read(BOOTCPLD_TBB_RE_SCRATCH_1_ADDR, &reg_val) == 0) {
		printf("Unable to save the contents of CPLD SCRATCH1 register\n");
		cpld_fail1 = -1;
		goto end;
	}

	/*
	 * Write a value into scratch register, read it and then compare.
	 * The read value should match with the value written earlier
	 */
	printf("Testing BOOTCPLD_TBB_RE_SCRATCH_1 Register: 0x%x\n",
		BOOTCPLD_TBB_RE_SCRATCH_1_ADDR);
	for (i=0 ; str[i] != '\0'; i++) {
		if (tbbcpld_reg_write(BOOTCPLD_TBB_RE_SCRATCH_1_ADDR, str[i]) == 0) {
			printf("Unable to write to CPLD SCRATCH1 register\n");
			cpld_fail1 = -1;
			break;
		}
		if (tbbcpld_reg_read(BOOTCPLD_TBB_RE_SCRATCH_1_ADDR, &val) == 0) {
			printf("Unable to read from CPLD SCRATCH1 register\n");
			cpld_fail1 = -1;
			break;
		}
		if (val != str[i]) {
			printf("The written character and read character from\n"
				"CPLD SCRATCH1 register did not match\n");
			cpld_fail1 = -1;
			break;
		}
	}
        
	/*
	 * Restore the value
	 */
	if (tbbcpld_reg_write(BOOTCPLD_TBB_RE_SCRATCH_1_ADDR, reg_val) == 0) {
		printf("Unable to restore the contents of CPLD SCRATCH1 register\n");
		cpld_fail1 = -1;
		goto end;
	}

	/*
	 * Save the CPLD scratch2 register value as it is being used
	 * for storing next boot option
	 */
	if (tbbcpld_reg_read(BOOTCPLD_TBB_RE_SCRATCH_2_ADDR, &reg_val) == 0) {
		printf("Unable to save the contents of CPLD SCRATCH2 register\n");
		cpld_fail2 = -1;
	}

	printf("Testing BOOTCPLD_TBB_RE_SCRATCH_2 Register: 0x%x\n",
		BOOTCPLD_TBB_RE_SCRATCH_2_ADDR);
	for (i=0 ; str[i] != '\0'; i++) {
		if (tbbcpld_reg_write(BOOTCPLD_TBB_RE_SCRATCH_2_ADDR, str[i]) == 0) {
			printf("Unable to write to CPLD SCRATCH2 register\n");
			cpld_fail2 = -1;
			break;
		}
		if (tbbcpld_reg_read(BOOTCPLD_TBB_RE_SCRATCH_2_ADDR, &val) == 0) {
			printf("Unable to read from CPLD SCRATCH2 register\n");
			cpld_fail2 = -1;
			break;
		}
		if (val != str[i]) {
			printf("The written character and read character from\n"
				"CPLD SCRATCH2 register did not match\n");
			cpld_fail2 = -1;
			break;
		}
	}

	/*
	 * Restore the value
	 */
	if (tbbcpld_reg_write(BOOTCPLD_TBB_RE_SCRATCH_2_ADDR, reg_val) == 0) {
		printf("Unable to restore the contents of CPLD SCRATCH2 register\n");
		cpld_fail2 = -1;
	}

end:
	if (cpld_fail1) {
		printf( "\n" );
		printf( " ---Test on CPLD SCRATCH1 register has failed---\n ");
	}
	
	if (cpld_fail2) {
		printf ( "\n" );
		printf( " ---Test on CPLD SCRATCH2 register has failed---\n ");
	}

	if (!cpld_fail1 && !cpld_fail2) {
		printf( "\n" );
		printf("CPLD test completed, 1 pass, 0 errors\n");
		return 0;
	} else {
		printf( "\n" );
		printf("CPLD test failed, 0 pass, 1 errors\n");
		return -1;
	}
}

#endif /* CONFIG_POST & CFG_POST_PCI */
#endif /* CONFIG_POST */
#endif /* CONFIG_MX80 */
