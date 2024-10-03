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
 * Store instructions:		strb, str
 *
 * Store data to memory and then check the memory content.
 */

#ifdef CONFIG_POST

#include <post.h>
#include "cpu_asm.h"

#if CONFIG_POST & CFG_POST_CPU

extern void cpu_post_exec_11 (ulong *code, ulong *res, ulong op1);

#define TEST_STR_PATTERN    0xa1b2c3d4

int 
cpu_post_test_str (void)
{
    int ret = 0;
    ulong res;

    if (ret == 0) {
        /* test strb */
        static unsigned long code[] =
        {
            ASM_STR(COND_AL, 0, 1, 1, 1, 0, 1, 2, 0, 0, 0),  /* strb r2, [r1] */
            ASM_BLR  /* bx r14 */
        };

        res = 0;

        cpu_post_exec_11(code, &res, TEST_STR_PATTERN & 0xFF);

        ret = (res == (TEST_STR_PATTERN & 0xFF)) ? 0 : -1;
    }

    if (ret == 0) {
        /* test str */
        static unsigned long code[] =
        {
            ASM_STR(COND_AL, 0, 1, 1, 0, 0, 1, 2, 0, 0, 0),  /* str r2, [r1] */
            ASM_BLR  /* bx r14 */
        };

        res = 0;

        cpu_post_exec_11(code, &res, TEST_STR_PATTERN);

        ret = (res == TEST_STR_PATTERN) ? 0 : -1;
    }

    return (ret);
}

#endif
#endif
