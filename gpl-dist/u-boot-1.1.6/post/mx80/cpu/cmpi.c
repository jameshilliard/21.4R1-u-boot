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

/*
 * CPU test
 * Integer compare instructions:	cmpwi, cmplwi
 *
 * To verify these instructions the test runs them with
 * different combinations of operands, reads the condition
 * register value and compares it with the expected one.
 * The test contains a pre-built table
 * containing the description of each test case: the instruction,
 * the values of the operands, the condition field to save
 * the result in and the expected result.
 */

#ifdef CONFIG_POST

#include <post.h>
#include "cpu_asm.h"

#if CONFIG_POST & CFG_POST_CPU

extern void cpu_post_exec_11(ulong *code, ulong *res, ulong op1);

static struct cpu_post_cmpi_s {
	ulong cmd;
	ulong op1;
	ushort op2;
	ulong cr;
	ulong res;
} cpu_post_cmpi_table[] = {
	{
		OP_CMPWI,
		123,
		123,
		2,
		0x02
	},
	{
		OP_CMPWI,
		123,
		133,
		3,
		0x08
	},
	{
		OP_CMPWI,
		123,
		-133,
		4,
		0x04
	},
	{
		OP_CMPLWI,
		123,
		123,
		2,
		0x02
	},
	{
		OP_CMPLWI,
		123,
		-133,
		3,
		0x08
	},
	{
		OP_CMPLWI,
		123,
		113,
		4,
		0x04
	},
};
static unsigned int cpu_post_cmpi_size =
                sizeof(cpu_post_cmpi_table) / sizeof(struct cpu_post_cmpi_s);

int 
cpu_post_test_cmpi (void)
{
	int ret = 0;
	unsigned int i;

	for (i = 0; i < cpu_post_cmpi_size && ret == 0; i++) {
		struct cpu_post_cmpi_s *test = cpu_post_cmpi_table + i;
		unsigned long code[] = {
			ASM_1IC(test->cmd, test->cr, 3, test->op2),
			ASM_MFCR(3),
			ASM_BLR
		};

		flush_page_to_ram(code);

		ulong res;

		cpu_post_exec_11(code, & res, test->op1);

		/*
		 * Need to check with vendor
		 * for appropriate comments
		 */
		ret = ((res >> (28 - 4 * test->cr)) & 0xe) == test->res ? 0 : -1;

	}

	return ret;
}

#endif
#endif
