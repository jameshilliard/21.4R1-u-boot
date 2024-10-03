/*
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

#ifdef CONFIG_POST && CONFIG_JSRXNLE

#include <post.h>
#include "post_jsrxnle.h"

struct post_test post_list[] =
{
#if CONFIG_POST & CFG_POST_RTC
    {
	"RTC test",
	"rtc",
	"This test verifies the Real Time Clock operation.",
	POST_RAM | POST_MANUAL | POST_NORMAL,
	&rtc_post_test,
	NULL,
	NULL,
	CFG_POST_RTC
    },
#endif
#if CONFIG_POST & CFG_POST_EEPROM
    {
	"EEPROM test",
	"eeprom",
	"This test verifies the onboard eeprom for board ID",
	POST_ROM | POST_MANUAL | POST_ALWAYS,
	&eeprom_post_test,
	NULL,
	NULL,
	CFG_POST_EEPROM
    },
#endif
#if CONFIG_POST & CFG_POST_MEMORY
    {
	"Memory test",
	"memory",
	"This test checks RAM.",
	POST_ROM | POST_ALWAYS,
	&memory_post_test,
	NULL,
	NULL,
	CFG_POST_MEMORY
    },
#endif
#if CONFIG_POST & CFG_POST_ETHER
    {
	"ETHERNET test",
	"ethernet",
	"This test verifies the ETHERNET operation.",
	POST_RAM | POST_ALWAYS | POST_MANUAL,
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
	"This test verifies the USB operation.",
	POST_RAM | POST_NORMAL | POST_MANUAL,
	&usb_post_test,
	NULL,
	NULL,
	CFG_POST_USB
    },
#endif
#if CONFIG_POST & CFG_POST_UBOOT_CRC
    {
	"U-Boot crc test",
	"u-boot-crc",
	"This test verifies the CRC of active u-boot image in boot-flash.",
	POST_RAM | POST_ALWAYS,
	&uboot_crc_post_test,
	NULL,
	NULL,
	CFG_POST_UBOOT_CRC
    },
#endif
#if CONFIG_POST & CFG_POST_SKU_ID
    {
	"SKU-ID test",
	"sku-id",
	"Verifies that SKU-ID in FPGA matches the I2C-ID in the EEPROM.",
	POST_RAM | POST_ALWAYS,
	&sku_id_post_test,
	NULL,
	NULL,
	CFG_POST_SKU_ID
    },
#endif
};

unsigned int post_list_size = sizeof (post_list) / sizeof (struct post_test);

#endif /* CONFIG_POST && CONFIG_JSRXNLE */
