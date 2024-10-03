/*
 * Copyright (c) 2009-2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * (C) Copyright 2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>

/*
 * CPU test
 *
 * This test checks the arithmetic logic unit (ALU) of CPU.
 * It tests independently various groups of instructions using
 * run-time modification of the code to reduce the memory footprint.
 * For more details refer to post/cpu/ *.c files.
 */

#ifdef CONFIG_POST

#include <watchdog.h>
#include <post.h>

#if CONFIG_POST & CFG_POST_CPU

extern void icache_enable(void);
extern void icache_disable(void);
extern int cpu_post_test_str (void);
extern int cpu_post_test_ldr (void);
extern int cpu_post_test_cmp (void);
extern int cpu_post_test_add (void);
extern int cpu_post_test_rsb (void);
extern int cpu_post_test_mul (void);
extern int cpu_post_test_and (void);
extern int cpu_post_test_or (void);
extern int cpu_post_test_eor (void);

int post_result_cpu;

#define CPU_TESTS_COUNT		9	/* XXX: Update this when any new test is added */		
char  *cpu_test_str[] = {
			"Data movement Str",
			"Data movement Ldr",
			"Compare Cmp",
			"Arithmatic Add",
			"Arithmatic Subtraction",
			"Arithmatic Multiply",
			"Logical bit And",
			"Logical bit Or",
			"Logical bit Eor",
			};

int 
cpu_post_test (int flags)
{
    int ic = icache_status ();
    int  ret = 0;
    int c;
    int cpu_verbose = flags & POST_DEBUG;
    int cpu_test_flag = CPU_TESTS_COUNT;
    
    post_result_cpu = 0;
    
    if (flags & BOOT_POST_FLAG) {
        cpu_verbose = 0;
    }

    if (cpu_verbose) {	
        printf("Executing various CPU instructions with different possible");
        printf("\n operands to verify the ALU.\n");
    }
			
    WATCHDOG_RESET();
    if (ic)
        icache_disable();

    if (ret == 0)
        ret = cpu_post_test_str();
    if (ret == 0) {
        cpu_test_flag--;
        ret = cpu_post_test_ldr();
    }
    if (ret == 0) {
        cpu_test_flag--;
        ret = cpu_post_test_cmp();
    }
    if (ret == 0) {
        cpu_test_flag--;
        ret = cpu_post_test_add();
    }
    if (ret == 0) {
        cpu_test_flag--;
        ret = cpu_post_test_rsb();
    }
    if (ret == 0) {
        cpu_test_flag--;
        ret = cpu_post_test_mul();
    }
    if (ret == 0) {
        cpu_test_flag--;
        ret = cpu_post_test_and();
    }
    if (ret == 0) {
        cpu_test_flag--;
        ret = cpu_post_test_or();
    }
    if (ret == 0) {
        cpu_test_flag--;
        ret = cpu_post_test_eor();
    }
    if (ret == 0) {
        cpu_test_flag--;
    }
    WATCHDOG_RESET();
    if (ic)
        icache_enable();

    WATCHDOG_RESET();
    if (flags & BOOT_POST_FLAG) {
        if (cpu_test_flag) {
            post_result_cpu = -1;
            return (-1);
        }
        else {
            return (0);
        }
    }	
	
    if (cpu_test_flag) {
        post_log("-- CPU test                             FAILED @\n");
    } else {
        post_log("-- CPU test                             PASSED\n");
    }
    
    if (cpu_verbose) {
        for (c = 0; c < (CPU_TESTS_COUNT - cpu_test_flag); c++) {
            printf(" > %-37s", cpu_test_str[c]);
            printf("PASSED\n");
        }
        if (cpu_test_flag) {
            printf(" > %-37s ... ", cpu_test_str[c]);
            printf("FAILED @\n");
        }
    }
		
    return (ret);
}

#endif /* CONFIG_POST & CFG_POST_CPU */
#endif /* CONFIG_POST */
