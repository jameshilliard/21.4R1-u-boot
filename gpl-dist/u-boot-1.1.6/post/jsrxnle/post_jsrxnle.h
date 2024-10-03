/*
 * Copyright (c) 2010, Juniper Networks, Inc.
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
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>.
 */

#ifndef _POST_JSRXNLE_H
#define _POST_JSRXNLE_H

#if (defined(CONFIG_POST) && defined(CONFIG_JSRXNLE))
int rtc_post_test (int flags);
int eeprom_post_test(int flags);
int memory_post_test(int flags);
int ether_post_test (int flags);
int usb_post_test (int flags);
int uboot_crc_post_test(int flags);
int sku_id_post_test (int flags);

unsigned int compute_sku_id (void);

/* PASS: Clear the bit. Fail: Set it */
#define SET_POST_RESULT(reg, mask, result) \
            reg = (result == POST_PASSED \
            ? (reg & (~mask)) \
            : (reg | mask))

#endif /* #if (defined(CONFIG_POST) && defined(CONFIG_JSRXNLE)) */
#endif /* #ifndef _POST_JSRXNLE_H */
