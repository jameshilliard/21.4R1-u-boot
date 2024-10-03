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
extern int uart_post_test(int flags);
extern int pci_post_test(int flags);
extern int ether_post_test(int flags);
extern int watchdog_post_test(int flags);
extern int cpld_post_test(int flags);

struct post_test post_list[] =
{
#if CONFIG_POST & CFG_POST_MEMORY
	{
		"The memory test",
		"memory",
		"This test verifies the RAM - "
			" Tests address bus,data bus and memory location",
		POST_ROM | POST_POWERON | POST_SLOWTEST | POST_PREREL,
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
		"This test verifies the RAM - "
			" Tests address bus,data bus and memory location",
		POST_RAM | POST_ALWAYS | POST_MANUAL,
		&memory_post_test_diag,
		NULL,
		NULL,
		CFG_POST_MEMORY_RAM
	},
#endif
#if CONFIG_POST & CFG_POST_CPU
	{
		"CPU test",
		"cpu",
		"This test verifies the arithmetic logic unit of"
			" CPU.",
		POST_RAM | POST_ALWAYS,
		&cpu_post_test,
		NULL,
		NULL,
		CFG_POST_CPU
	},
#endif
#if CONFIG_POST & CFG_POST_USB
	{
		"USB test",
		"usb",
		"This test verifies the USB Device.",
		POST_RAM | POST_MANUAL,
		&usb_post_test,
		NULL,
		NULL,
		CFG_POST_USB
	},
#endif
#if CONFIG_POST & CFG_POST_UART
	{
		"UART test",
		"uart",
		"This test verifies the UART operation - "
			"Type the string *** UART Test String ***\r\n",
		POST_RAM | POST_SLOWTEST | POST_MANUAL,
		&uart_post_test,
		NULL,
		NULL,
		CFG_POST_UART
	},
#endif
#if CONFIG_POST & CFG_POST_PCI
	{
		"PCI test",
		"pci",
		"This test verifies the PCIE Devices.",
		POST_RAM | POST_ALWAYS | POST_MANUAL,
		&pci_post_test,
		NULL,
		NULL,
		CFG_POST_PCI
	},
#endif
#if CONFIG_POST & CFG_POST_ETHER
	{
		"ethernet test",
		"ethernet",
		"This test verifies the ETHERNET operation - "
			"Checks internal and external loop back of each port",
		POST_RAM | POST_ALWAYS ,
		&ether_post_test,
		NULL,
		NULL,
		CFG_POST_ETHER
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
#if CONFIG_POST & CFG_POST_CPLD
        {
                "CPLD test",
                "cpld",
                "This test verifies the CPLD Device.",
                POST_RAM | POST_MANUAL,
                &cpld_post_test,
                NULL,
                NULL,
                CFG_POST_CPLD
        },
#endif
};
unsigned int post_list_size = sizeof(post_list) / sizeof(struct post_test);

#endif /* CONFIG_POST */
