
/*
 * $Id$
 *
 * tests.c -- List of diagnostics on the ACX platform
 *
 * Rajat Jain, Feb 2011
 *
 * Copyright (c) 2011-2014, Juniper Networks, Inc.
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
 *
 * Be sure to mark tests to be run before relocation as such with the
 * CFG_POST_PREREL flag so that logging is done correctly if the
 * logbuffer support is enabled.
 */

#include <common.h>

#ifdef CONFIG_POST

#include <post.h>

extern int memory_post_test_diag(int flags);
extern int cpu_post_test(int flags);
extern int usb_post_test(int flags);
extern int nand_post_test(int flags);
extern int ether_post_test(int flags);
extern int watchdog_post_test(int flags);
extern int syspld_post_test(int flags);
extern int nor_post_test(int flags);
#if CONFIG_POST & CFG_POST_SECUREBOOT
extern int secureboot_pb_post_test(int flags);
extern int secureboot_recovery_jumper_post_test(int flags);
extern int secureboot_nor_wp_post_test(int flags);
extern int secureboot_sreset_post_test(int flags);
extern int secureboot_nvram_wp_post_test(int flags);
extern int secureboot_uart_tx_post_test(int flags);
extern int secureboot_uart_rx_post_test(int flags);
extern int secureboot_gcr_post_test(int flags);
#endif /* CONFIG_POST & CFG_POST_SECUREBOOT */

struct post_test post_list[] =
{
#if CONFIG_POST & CFG_POST_SECUREBOOT
	{
		"Recovery Push Button Test",
		"pb",
		"This test verifies that the recovery push button on the "\
			"is working fine.",
		POST_RAM | POST_MANUAL,
		&secureboot_pb_post_test,
		NULL,
		NULL,
		CFG_POST_SECUREBOOT
	},
	{
		"Recovery Jumper Test",
		"jumper",
		"This test verifies the recovery jumper",
		POST_RAM | POST_MANUAL,
		&secureboot_recovery_jumper_post_test,
		NULL,
		NULL,
		CFG_POST_SECUREBOOT
	},
	{
		"NOR flash write protect test",
		"norwp",
		"This test checks the NOR flash write protection "\
			"mechanism on the board.",
		POST_RAM | POST_MANUAL,
		&secureboot_nor_wp_post_test,
		NULL,
		NULL,
		CFG_POST_SECUREBOOT
	},
	{
		"CPU only soft reset test",
		"cpusr",
		"This test verifies that the CPU only softreset "\
			"functionality is working fine",
		POST_RAM | POST_MANUAL,
		&secureboot_sreset_post_test,
		NULL,
		NULL,
		CFG_POST_SECUREBOOT
	},
	{
		"NVRAM write protection test",
		"nvramwp",
		"This test verifies the nvram write protection "\
			"functionality of the chassis",
		POST_RAM | POST_MANUAL,
		&secureboot_nvram_wp_post_test,
		NULL,
		NULL,
		CFG_POST_SECUREBOOT
	},
	{
		"UART transmit disable test",
		"uartcontrol_tx",
		"This test verifies the UART transmit disable "\
			"functionality of the chassis",
		POST_RAM | POST_MANUAL,
		&secureboot_uart_tx_post_test,
		NULL,
		NULL,
		CFG_POST_SECUREBOOT
	},
	{
		"UART receive disable test",
		"uartcontrol_rx",
		"This test verifies the UART receive disable "\
			"functionality of the chassis",
		POST_RAM | POST_MANUAL,
		&secureboot_uart_rx_post_test,
		NULL,
		NULL,
		CFG_POST_SECUREBOOT
	},
	{
		"Glass safe test",
		"gcr",
		"This test verifies that the glass safe lock is working",
		POST_RAM | POST_MANUAL,
		&secureboot_gcr_post_test,
		NULL,
		NULL,
		CFG_POST_SECUREBOOT
	},
#endif /* CONFIG_POST & CFG_POST_SECUREBOOT */
#if CONFIG_POST & CFG_POST_CPU
	{
		"CPU test",
		"cpu",
		"This test verifies the arithmetic logic unit of CPU.",
		POST_RAM | POST_ALWAYS,
		&cpu_post_test,
		NULL,
		NULL,
		CFG_POST_CPU
	},
#endif
#if CONFIG_POST & CFG_POST_MEMORY
	{
		"The memory test",
		"memory",
		"This test verifies the RAM  & is run from ROM - Tests address bus, data bus and memory chip(s).\n",
		POST_ROM | POST_POWERON | POST_NORMAL | POST_SLOWTEST | POST_PREREL,
		&memory_post_test_diag,
		NULL,
		NULL,
		CFG_POST_MEMORY
	},
#endif
#if CONFIG_POST & CFG_POST_MEMORY_RAM
	{
		"The memory test",
		"ram",
		"This test verifies the RAM & is run from RAM - Tests address bus, data bus and memory chip(s).\n"
		"  It test all memory using except the ram uboot has relocated to (and around 8K memory @ 0x00000000 used for exception vectors).\n"
		"  (Very few of that tests that can be run from ROM only are ommited)",
		POST_RAM | POST_MANUAL,
		&memory_post_test_diag,
		NULL,
		NULL,
		CFG_POST_MEMORY_RAM
	},
#endif
#if CONFIG_POST & CFG_POST_SYSPLD
    {
        "SYSPLD test",
        "syspld",
        "This test verifies the SYSPLD Device register access. It uses Walking bits test\n"
		"  to verify the data bus to the SysPLD as well",
        POST_RAM | POST_ALWAYS,
        &syspld_post_test,
        NULL,
        NULL,
        CFG_POST_SYSPLD
    },
#endif
#if CONFIG_POST & CFG_POST_ETHER
	{
		"ethernet test",
		"ethernet",
		"This test verifies the ETHERNET operation - "
			"  Checks MAC and PHY loop back of mgmt port",
		POST_RAM | POST_MANUAL,
		&ether_post_test,
		NULL,
		NULL,
		CFG_POST_ETHER
	},
#endif
#if CONFIG_POST & CFG_POST_USB
	{
		"USB test",
		"usb",
		"This test verifies the USB Device.",
		POST_RAM | POST_MANUAL | POST_POWERON,
		&usb_post_test,
		NULL,
		NULL,
		CFG_POST_USB
	},
#endif
#if CONFIG_POST
	{
		"NOR test",
		"nor",
		"This test verifies the NOR flash Device (destructive test).",
		POST_RAM | POST_MANUAL,
		&nor_post_test,
		NULL,
		NULL,
		CFG_POST_NAND
	},
#endif
#if CONFIG_POST & CFG_POST_NAND
	{
		"NAND test",
		"nand",
		"This test verifies the NAND flash Device (destructive test).",
		POST_RAM | POST_MANUAL,
		&nand_post_test,
		NULL,
		NULL,
		CFG_POST_NAND
	},
#endif
#if CONFIG_POST & CFG_POST_WATCHDOG
	{
		"Watchdog timer test",
		"watchdog",
		"This test checks the watchdog timer - "
			"When watchdog timer interrupt overflows then system reboot",
		POST_RAM | POST_POWERON | POST_SLOWTEST | POST_MANUAL | POST_REBOOT,
		&watchdog_post_test,
		NULL,
		NULL,
		CFG_POST_WATCHDOG
	},
#endif
};
unsigned int post_list_size = sizeof(post_list) / sizeof(struct post_test);

#endif /* CONFIG_POST */
