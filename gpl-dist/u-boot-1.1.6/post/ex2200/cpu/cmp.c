/*
 * Copyright (c) 2009-2010, Juniper Networks, Inc.
 * All rights reserved.
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
 * Integer compare instructions:	cmp
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

extern void cpu_post_exec_12 (ulong *code, ulong *res, ulong op1, ulong op2);

static struct cpu_post_cmp_s
{
    ulong op1;
    ulong op2;
    ulong res;
} cpu_post_cmp_table[] =
{
    {
	123,
	123,
	0x06
    },
    {
	123,
	133,
	0x08
    },
    {
	123,
	-133,
	0x0
    },
    {
	123,
	123,
	0x06
    },
    {
	123,
	-133,
	0x0
    },
    {
	123,
	113,
	0x02
    },
};
static unsigned int cpu_post_cmp_size =
    sizeof (cpu_post_cmp_table) / sizeof (struct cpu_post_cmp_s);

int 
cpu_post_test_cmp (void)
{
    int ret = 0;
    unsigned int i;

    for (i = 0; i < cpu_post_cmp_size && ret == 0; i++)
    {
	struct cpu_post_cmp_s *test = cpu_post_cmp_table + i;
    	static unsigned long code[] =
	{
	    ASM_CMP(COND_AL, 0, 2, 0, 0, 3, 0, 0),  /* cmp r2, r3 */
	    ASM_MRS(COND_AL, 0, 0),  /* mrs r0,cpsr */
	    ASM_STR(COND_AL, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0), /* str r0, [r1] */
	    ASM_BLR  /* bx r14 */
	};
	
	ulong res;

	cpu_post_exec_12 (code, &res, test->op1, test->op2);

	ret = ((res >> COND_SH) & 0xf) == test->res ? 0 : -1;
    }

    return (ret);
}

#endif
#endif
