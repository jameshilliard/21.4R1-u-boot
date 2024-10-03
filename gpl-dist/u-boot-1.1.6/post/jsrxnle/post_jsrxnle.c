/*
 * Copyright (c) 2009-2010, Juniper Networks, Inc.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <console.h>
#include <watchdog.h>
#include <post.h>


#ifdef CONFIG_LOGBUFFER
#include <logbuff.h>
#endif

#if (defined(CONFIG_POST) && defined(CONFIG_JSRXNLE))

DECLARE_GLOBAL_DATA_PTR;

int
post_log_result_env(void)
{

#if CONFIG_POST & CFG_POST_USB
    if (gd->post_result & USB_POST_RESULT_MASK)
	setenv("post.usb", "FAILED");
    else
	setenv("post.usb", "PASSED");
#endif

#if CONFIG_POST & CFG_POST_RTC
    if (gd->post_result & RTC_POST_RESULT_MASK)
	setenv("post.rtc", "FAILED");
    else
	setenv("post.rtc", "PASSED");
#endif

#if CONFIG_POST & CFG_POST_EEPROM
    if (gd->post_result & EEPROM_POST_RESULT_MASK)
	setenv("post.eeprom", "FAILED");
    else
	setenv("post.eeprom", "PASSED");
#endif

#if CONFIG_POST & CFG_POST_MEMORY
    if (gd->post_result & MEMORY_POST_RESULT_MASK)
	setenv("post.memory", "FAILED");
    else
	setenv("post.memory", "PASSED");
#endif

#if CONFIG_POST & CFG_POST_UBOOT_CRC
    if (gd->post_result & UBOOT_CRC_POST_RESULT_MASK) {
	setenv("post.uboot-crc", "FAILED");
    } else {
	setenv("post.uboot-crc", "PASSED");
    }
#endif

    return 0;
}

int
post_log_result_flash(void)
{
    return 0;
}

#endif /* CONFIG_POST && CONFIG_JSRXNLE */

