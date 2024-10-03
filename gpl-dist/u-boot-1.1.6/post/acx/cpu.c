/*
 * Copyright (c) 2011-2012, Juniper Networks, Inc.
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

extern void enable_icache(void);
extern void disable_icache(void);
extern int cpu_post_test_cmp(void);
extern int cpu_post_test_cmpi(void);
extern int cpu_post_test_two(void);
extern int cpu_post_test_twox(void);
extern int cpu_post_test_three(void);
extern int cpu_post_test_threex(void);
extern int cpu_post_test_threei(void);
extern int cpu_post_test_andi(void);
extern int cpu_post_test_srawi(void);
extern int cpu_post_test_rlwnm(void);
extern int cpu_post_test_rlwinm(void);
extern int cpu_post_test_rlwimi(void);
extern int cpu_post_test_store(void);
extern int cpu_post_test_load(void);
extern int cpu_post_test_cr(void);
extern int cpu_post_test_b(void);
extern int cpu_post_test_multi(void);
#ifndef CONFIG_E500
extern int cpu_post_test_string(void);
#endif
extern int cpu_post_test_complex(void);

/* JUNOS begin */
int cpu_dbg;
int cpu_test_flag;
int boot_flag_post;
int post_result_cpu;
#define CPU_TESTS_COUNT 17
char  *cpu_test_str[] = {"Compare(cmp) Instructions",
	"Compare Immediate(cmpi) Instructions",
	"Arithemetic & Logical Binary(2 operand)Instructions-1",
	"Arithemetic & Logical Binary(2 operand)Instructions-2",
	"Arithmetic Ternary Instructions",
	"Logical & Shift Ternary Instructions",
	"Logical Immediate Ternary Instructions",
	"Logical AND Instructions",
	"Shift Instructions",
	"Shift(Rotate) Instructions",
	"Shift(Rotate) Immediate Instructions-1",
	"Shift(Rotate) Immediate Instructions-2",
	"Store Instructions",
	"Load Instructions",
	"Condition register(CR) Instructions",
	"Load/Store Mulit word Instructions",
	"Complex calculations"};
/* JUNOS end */

ulong cpu_post_makecr(long v)
{
	ulong cr = 0;

	if (v < 0) {
		cr |= 0x80000000;
	}
	if (v > 0) {
		cr |= 0x40000000;
	}
	if (v == 0) {
		cr |= 0x20000000;
	}
	return cr;
}

int cpu_post_test(int flags)
{
	int ic          = icache_status ();
	int ret         = 0;
	int c           = 0;

	/* JUNOS begin */
	post_result_cpu = 1;
	cpu_dbg         = 0;
	boot_flag_post  = 0;
	cpu_test_flag   = CPU_TESTS_COUNT;
	if (flags & POST_DEBUG) {
		cpu_dbg = 1;
	}
	if (flags & BOOT_POST_FLAG) {
		boot_flag_post = 1;
	}
	if (!boot_flag_post) {
		printf(" Executing various CPU instructions with different \
		         possible");
		printf("\n operands to verify the ALU.\n\n");
	}
	/* JUNOS end */

	WATCHDOG_RESET();
	if (ic) {
		disable_icache();
	}
	if (ret == 0) {
		ret = cpu_post_test_cmp();
	}
	if (ret == 0) {
		cpu_test_flag--;
		ret = cpu_post_test_cmpi();
	}
	if (ret == 0) {
		cpu_test_flag--;
		ret = cpu_post_test_two();
	}
	if (ret == 0) {
		cpu_test_flag--;
		ret = cpu_post_test_twox();
	}
	WATCHDOG_RESET();
	if (ret == 0) {
		cpu_test_flag--;
		ret = cpu_post_test_three();
	}
	if (ret == 0) {
		cpu_test_flag--;
		ret = cpu_post_test_threex();
	}
	if (ret == 0) {
		cpu_test_flag--;
		ret = cpu_post_test_threei();
	}
	if (ret == 0) {
		cpu_test_flag--;
		ret = cpu_post_test_andi();
	}
	WATCHDOG_RESET();
	if (ret == 0) {
		cpu_test_flag--;
		ret = cpu_post_test_srawi();
	}
	if (ret == 0) {
		cpu_test_flag--;
		ret = cpu_post_test_rlwnm();
	}
	if (ret == 0) {
		cpu_test_flag--;
		ret = cpu_post_test_rlwinm();
	}
	if (ret == 0) {
		cpu_test_flag--;
		ret = cpu_post_test_rlwimi();
	}
	WATCHDOG_RESET();
	if (ret == 0) {
		cpu_test_flag--;
		ret = cpu_post_test_store();
	}
	if (ret == 0) {
		cpu_test_flag--;
		ret = cpu_post_test_load();
	}
	if (ret == 0) {
		cpu_test_flag--;
		ret = cpu_post_test_cr();
	}
	WATCHDOG_RESET();
	if (ret == 0) {
		cpu_test_flag--;
		ret = cpu_post_test_multi();
	}
	WATCHDOG_RESET();
#ifndef CONFIG_E500
	if (ret == 0) {
		ret = cpu_post_test_string();
	}
#endif /*!CONFIG_E500*/
	if (ret == 0) {
		cpu_test_flag--;
		ret = cpu_post_test_complex();
	}
	if (ret == 0) {
		cpu_test_flag--;
	}
	WATCHDOG_RESET();

	if (ic) {
		enable_icache();
	}

	WATCHDOG_RESET();

	/* JUNOS begin */
	if (boot_flag_post) {
		if (cpu_test_flag) {
			post_result_cpu = -1;
			return -1;
		} else {
			return 0;
		}
	}	

	if (cpu_test_flag) {
		post_log("CPU test failed, 0 pass, 1 errors\n\n");
	} else {
		post_log("CPU test completed, 1 pass, 0 errors\n\n");
	}
	if (cpu_dbg) {
		for (c = 0; c < (CPU_TESTS_COUNT - cpu_test_flag); c++) {
			printf("\t%55s ... ",cpu_test_str[c]);
			printf("PASSED\n");
		}
		if (cpu_test_flag) {
			printf("\t%55s ... ",cpu_test_str[c]);
			printf("FAILED\n");
		}
	}
	/* JUNOS end */

	return ret;
}

#endif /* CONFIG_POST & CFG_POST_CPU */
#endif /* CONFIG_POST */
