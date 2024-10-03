/*
 * $Id$
 *
 * nor.c -- NOR flash diagnostics for the ACX platform.
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

#include <flash.h>
#include <malloc.h>

#ifdef CONFIG_POST

#include <post.h>
#include "acx_post.h"

int post_result_nor;

#define NOR_TEST_SECTOR_ADDR    CFG_FLASH_BASE
#define NOR_TEST_SECTOR_LAST_ADDR    (CFG_FLASH_BASE + 128 * 1024 - 1)

int nor_post_test(int flags)
{
	unsigned int i;
	char buffer[8];

	post_result_nor = 1;

	POST_LOG(POST_LOG_IMP, "================NOR FLASH TEST START==================\n");
	for (i = 0; i < 8; i++) {
		buffer[i] = 1 << i;
	}

	flash_sect_erase(NOR_TEST_SECTOR_ADDR, NOR_TEST_SECTOR_LAST_ADDR);
	flash_write(buffer, NOR_TEST_SECTOR_ADDR, 8);

	if(memcmp(buffer, (void*)NOR_TEST_SECTOR_ADDR, 8) != 0) {
		POST_LOG(POST_LOG_ERR, "Error, NOR test failed. [Erase / Program"
			 "/ Verify cycle failed for Walking 1s]\n");
		post_result_nor = -1;
		return -1;
	}

	for (i = 0; i < 8; i++) {
		buffer[i] = ~(1 << i);
	}

	flash_sect_erase(NOR_TEST_SECTOR_ADDR, NOR_TEST_SECTOR_LAST_ADDR);
	flash_write(buffer, NOR_TEST_SECTOR_ADDR, 8);

	if(memcmp(buffer, (void*)NOR_TEST_SECTOR_ADDR, 8) != 0) {
		POST_LOG(POST_LOG_ERR, "Error, NOR test failed. [Erase / Program"
			 "/ Verify cycle failed for Walking 0s]\n");
		post_result_nor = -1;
		return -1;
	}

	if (flags & (POST_MANUAL | POST_SLOWTEST | POST_POWERON)) 
		POST_LOG(POST_LOG_IMP, "NOR Test Passed!\n");
	POST_LOG(POST_LOG_IMP, "================NOR FLASH TEST END====================\n");

	return 0;

}
#endif /* CONFIG_POST */
