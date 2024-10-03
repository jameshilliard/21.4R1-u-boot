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
 * Load instructions:		ldrb, ldr
 *
 * Load data from memory and then check the return with memory content.
 */

#ifdef CONFIG_POST

#include <post.h>
#include "cpu_asm.h"

#if CONFIG_POST & CFG_POST_CPU

extern ulong cpu_post_load_return (ulong *code, ulong *data);

int 
cpu_post_test_ldr (void)
{
    int ret = 0;
    ulong data = 0x1a2b3c4d;
    ulong res;

    if (ret == 0) {
        /* test ldrb */
        static unsigned long code[] =
        {
            ASM_LDR(COND_AL, 0, 1, 1, 1, 0, 1,0, 0, 0, 0),  /* ldrb r0, [r1] */
            ASM_BLR  /* bx r14 */
        };

        res = 0;

        res = cpu_post_load_return(code, &data);

        ret = ((res & 0xFF) == (data & 0xFF)) ? 0 : -1;
    }

    if (ret == 0) {
        /* test ldr */
        static unsigned long code[] =
        {
            ASM_LDR(COND_AL, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0),  /* ldr r0, [r1] */
            ASM_BLR  /* bx r14 */
        };

        res = 0;

        res = cpu_post_load_return(code, &data);

        ret = (res == data) ? 0 : -1;
    }

    return (ret);
}

#endif
#endif
