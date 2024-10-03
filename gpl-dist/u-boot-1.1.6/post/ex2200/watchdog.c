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

/*********** EX3242 *******************/

/*
 * Watchdog test
 *
 * The test verifies the watchdog timer operation.
 * On the first iteration, the test routine enables the watchdog timer
 * interrupt.if the system does not reboot,the watchdog timer interrupt is
 * not operational and the test fails.In the second iteration the watchdog
 * timer with out rebooot the test should pass.
 * Note: Currently this condition, if(flag & POST_REBOOT) is true is not working.
 *       Every time watchdog command is typed, the sytem will reboot.
 */

#ifdef CONFIG_POST
#include <post.h>

#if CONFIG_POST & CFG_POST_WATCHDOG
extern void cpu_watchdog_init(uint32_t ms);
extern void cpu_watchdog_enable(void);

int 
watchdog_post_test (int flags)
{
    
#ifdef DEBUG_TEST
    post_log("In watchdog timer function\n");
#endif
    if (flags & POST_REBOOT) {
        /* Test passed */
        return (0);
    } 
    else {
        int i, elaspe_ticks = 0;
	
        cpu_watchdog_init(8000);  /* 8 seconds */
        cpu_watchdog_enable();

        post_log("This tests the functionality of CPU watchdog.\n");
        post_log("This test will RESET the CPU within 10 seconds ");
        post_log("and control will be\n");
        post_log("back to the U-Boot prompt\n\n");

        for (i = 0; i < 10000; i++) /* 10 seconds */
		    udelay(1000);
    
        /*
         * If we have reached this point, the watchdog timer
         * does not work
         */
        return (-1);
    }
}
#endif /* CONFIG_POST & CFG_POST_WATCHDOG */
#endif /* CONFIG_POST */
