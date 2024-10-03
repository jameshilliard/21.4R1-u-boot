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


#ifdef CONFIG_POST

#include <post.h>
#include <watchdog.h>

#if CONFIG_POST & CFG_POST_MEMORY_ADDR_RAM

static int mem_fail_flag;
DECLARE_GLOBAL_DATA_PTR;
#define DISP_MEM_COUNT_10MB 2500000
#define COUNT_RESET_WDT_VAL 1024

static void
memory_post_addr_test (unsigned long start, unsigned long size)
{
	unsigned long i = 0;
	unsigned long k = 0;
	ulong *mem      = (ulong *)start;

	WATCHDOG_RESET();

	for (i = 0; i < (size / sizeof(ulong)); i++) {

		if (ctrlc()) {
			putc('\n');
			break;
		}

		gd->memory_addr = (mem + i);
		mem[i] = (mem + i);
		k++;
		if (k == DISP_MEM_COUNT_10MB) {
			printf("Memory at %08lx, wrote %08lx\n",mem + i, mem[i]);
			k = 0;
		}
		if ( size == (ulong)(mem +i)) {
			printf("Memory at %08lx, wrote %08lx\n",mem + i, mem[i]);
			break;
		}

		if (i % COUNT_RESET_WDT_VAL == 0) {
			WATCHDOG_RESET();
		}
	}

	k = 0;
	for (i = 0; i < (size / sizeof(ulong)); i++) {

		if (ctrlc()) {
			putc('\n');
			break;
		}

		k++;
		gd->memory_addr = (mem + i);
		if (k == DISP_MEM_COUNT_10MB) {
			printf("Memory at %08lx, Read %08lx\n",mem + i, mem[i]);
			k = 0;
		}

		if (size == (ulong)(mem +i)) {
			if (mem[i] != (ulong)(mem +i)) {
				post_log("\n");
				mem_fail_flag++;
				printf("Memory error at %08lx, wrote %08lx, read %08lx mismatch\n",
						(mem + i), (mem + i), mem[i]);
				post_log("\n");
			} else {
				printf("Memory at %08lx, Read %08lx\n", mem + i, mem[i]);
				gd->memory_addr = 0x55;
			}
			break;
		}

		if (mem[i] != (ulong)(mem +i)) {
			post_log("\n");
			mem_fail_flag++;
			printf("Memory error at %08lx, wrote %08lx, read %08lx mismatch\n",
					(mem + i), (mem + i), mem[i]);
			post_log("\n");
		}

		if (i % COUNT_RESET_WDT_VAL == 0) {
			WATCHDOG_RESET();
		}
	}
}

int
memory_address_post_test_diag (int flags)
{
	int ret = 0;
	mem_fail_flag    = 0;
	bd_t *bd = gd->bd;

	post_log(" ------- Testing Address lines in memory  ---------\n\n");
	post_log(" The size of memory %lu\n\n",(ulong)bd->bi_memsize);

	memory_post_addr_test(0x100000, bd->bi_memsize);

	post_log("\n\n");

	if (mem_fail_flag) {
		printf(" ------------ ADDRESS LINES TEST: FAILED  ------------\n");
		ret = -1;
	} else {
		printf(" ------------ ADDRESS LINES TEST: PASSED   ------------\n");
	}
	return ret;
}

#endif /* CONFIG_POST & CFG_POST_MEMORY_ADDR_RAM */
#endif /* CONFIG_POST */
