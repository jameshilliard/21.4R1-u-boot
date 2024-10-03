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
 * Arithmatic instructions:	add, rsb, mul
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

struct cpu_post_arithm_s
{
    ulong op1;
    ulong op2;
    ulong res;
};

static struct cpu_post_arithm_s cpu_post_add_table[] =
{
    {
	5,
	4,
	9
    },
    {
	4,
	5,
	9
    },
};
static unsigned int cpu_post_add_size =
    sizeof (cpu_post_add_table) / sizeof (struct cpu_post_arithm_s);

static struct cpu_post_arithm_s cpu_post_rsb_table[] =
{
    {
	5,
	4,
	1
    },
    {
	4,
	5,
	-1
    },
};
static unsigned int cpu_post_rsb_size =
    sizeof (cpu_post_rsb_table) / sizeof (struct cpu_post_arithm_s);

static struct cpu_post_arithm_s cpu_post_mul_table[] =
{
    {
	5,
	4,
	20
    },
    {
	4,
	-5,
	-20
    },
    {
	-4,
	-5,
	20
    },
};
static unsigned int cpu_post_mul_size =
    sizeof (cpu_post_mul_table) / sizeof (struct cpu_post_arithm_s);

int 
cpu_post_test_add (void)
{
    int ret = 0;
    unsigned int i;

    for (i = 0; i < cpu_post_add_size && ret == 0; i++)
    {
	struct cpu_post_arithm_s *test = cpu_post_add_table + i;
    	static unsigned long code[] =
	{
	    ASM_ADD(COND_AL, 0, 0, 2, 3, 0, 3, 0, 0),  /* add r3, r2, r3 */
	    ASM_STR(COND_AL, 0, 1, 1, 0, 0, 1, 3, 0, 0, 0), /* str r3, [r1] */
	    ASM_BLR  /* bx r14 */
	};
	
	ulong res;

	cpu_post_exec_12 (code, &res, test->op1, test->op2);

	ret = (res == test->res) ? 0 : -1;
    }

    return (ret);
}

int 
cpu_post_test_rsb (void)
{
    int ret = 0;
    unsigned int i;

    for (i = 0; i < cpu_post_rsb_size && ret == 0; i++)
    {
	struct cpu_post_arithm_s *test = cpu_post_rsb_table + i;
    	static unsigned long code[] =
	{
	    ASM_RSB(COND_AL, 0, 0, 3, 3, 0, 2, 0, 0),  /* rsb r3, r3, r2 */
	    ASM_STR(COND_AL, 0, 1, 1, 0, 0, 1, 3, 0, 0, 0), /* str r3, [r1] */
	    ASM_BLR  /* bx r14 */
	};
	
	ulong res;

	cpu_post_exec_12 (code, &res, test->op1, test->op2);

	ret = (res == test->res) ? 0 : -1;
    }

    return (ret);
}

int 
cpu_post_test_mul (void)
{
    int ret = 0;
    unsigned int i;

    for (i = 0; i < cpu_post_mul_size && ret == 0; i++)
    {
	struct cpu_post_arithm_s *test = cpu_post_mul_table + i;
    	static unsigned long code[] =
	{
	    ASM_MUL(COND_AL, 0, 3, 3, 2),  /* mul r3, r2, r3 */
	    ASM_STR(COND_AL, 0, 1, 1, 0, 0, 1, 3, 0, 0, 0), /* str r3, [r1] */
	    ASM_BLR  /* bx r14 */
	};
	
	ulong res;

	cpu_post_exec_12 (code, &res, test->op1, test->op2);

	ret = (res == test->res) ? 0 : -1;
    }

    return (ret);
}

#endif
#endif
