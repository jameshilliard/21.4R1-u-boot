/*
 * $Id$
 *
 * watchdog.c -- Diagnostics for the SysPLD RE watchdog
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

/*
 * Watchdog test
 *
 * The test verifies the watchdog timer operation.
 * On the first iteration, the test routine enables the watchdog timer
 * interrupt.if the system does not reboot,the watchdog timer interrupt is
 * not operational and the test fails.In the second iteration the watchdog
 * timer with out rebooot the test should pass.
 */

#ifdef CONFIG_POST
#include <post.h>
#include <acx_syspld.h>
#include <asm-ppc/processor.h>

#if CONFIG_POST & CFG_POST_WATCHDOG
#define WDTI 0x6d120000

#define WDT_IN_PROGRESS 0xF3862687

int post_result_watchdog;
extern uint32_t nvram_get_uboot_wdt_test(void);
extern void nvram_set_uboot_wdt_test(uint32_t);
extern uint8_t nvram_get_uboot_reboot_reason(void);

int watchdog_post_test(int flags)
{
	unsigned long long base;
	int secs, ints;

	post_result_watchdog = 1;

	post_log("\n=================WATCHDOG TEST START===================\n");

	if (nvram_get_uboot_wdt_test() == WDT_IN_PROGRESS) {
		nvram_set_uboot_wdt_test(0);
		post_log("\n=================WATCHDOG TEST END=====================\n");
		if (nvram_get_uboot_reboot_reason() == RESET_REASON_WATCHDOG) {
			post_log("Watchdog test PASSED!\n");
			return 0;
		} else {
			post_log("Watchdog Test FAILED (Some other condition caused reset instead of watchdog)\n");
			post_result_watchdog = -1;
			return -1;
		}
	}

	nvram_set_uboot_wdt_test(WDT_IN_PROGRESS);

	ulong clk = get_tbclk();
	syspld_set_wdt_timeout(40);

	post_log("This tests the functionality of SysPLD watchdog.\n");
	post_log("This test should cause an SRESET (u-boot crash) in roughly %d seconds"
		 "followed by HRESET (Processor reset) in about 86 seconds\n", 
		 syspld_get_wdt_timeout());
	post_log("If this does not happen, the watchdog test is a failure\n");
	post_log("It will bring back the U-Boot prompt, and this test should be run again to see the result.\n\n");

	base = get_ticks();
	ints = disable_interrupts();

	syspld_swizzle_disable();
	syspld_watchdog_enable();

	for(secs = 1; secs < 70; secs++) {
		while (((unsigned long long)get_ticks() - base) / secs < clk);
		printf("%d..", secs);
	}

	if (ints)
		enable_interrupts ();
	/*
	 * If we have reached this point, the watchdog timer
	 * does not work
	 */
	post_log("\nWatchdog test FAILED! Watchdog did not reset the processor in 70 seconds!\n");
	post_result_watchdog = -1;
	return -1;
}

#endif /* CONFIG_POST & CFG_POST_WATCHDOG */
#endif /* CONFIG_POST */
