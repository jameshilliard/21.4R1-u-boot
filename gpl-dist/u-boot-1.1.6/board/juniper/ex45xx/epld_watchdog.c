/*
 * Copyright (c) 2010, Juniper Networks, Inc.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include "epld.h"
#include "epld_watchdog.h"

#if defined(CONFIG_EPLD_WATCHDOG)
void 
epld_watchdog_init (void)
{
       volatile immap_t *immap = (immap_t *)CFG_IMMR;
       volatile ccsr_pic_t *ccsr_pic_ptr=&immap->im_pic;
       unsigned long temp;

       temp = ccsr_pic_ptr->eivpr1;
       temp &= 0xff000000;
       temp += (IRQ1_POLARITY<<23) + (IRQ1_SENSE<<22) + 
            (IRQ1_PRIORITY<<16) + IRQ1_INT_VEC;
       ccsr_pic_ptr->eivpr1 = temp;
       temp &= 0x7fffffff;  /* enable IRQ1 */
       ccsr_pic_ptr->eivpr1 = temp;

}

void 
epld_watchdog_reset (void)
{
    epld_reg_write(EPLD_WATCHDOG_SW_UPDATE, 0x1);
}

int 
epld_watchdog_disable (void)
{
    return epld_reg_write(EPLD_WATCHDOG_ENABLE, 0x0);
}

int 
epld_watchdog_enable (void)
{
    return epld_reg_write(EPLD_WATCHDOG_ENABLE, 0x1);
}

void 
epld_watchdog_dump (void)
{
    uint16_t tmp;

    if (epld_reg_read(EPLD_WATCHDOG_ENABLE, &tmp))
        printf ("EPLD:   Watchdog 0x%x\n", tmp);
    else {
        printf ("EPLD:   Failed to read reg[EPLD_WDOG_ENABLE]\n");
        return;
    }

    if (epld_reg_read(EPLD_WATCHDOG_LEVEL_DEBUG, &tmp))
        printf ("            Watchdog state 0x%x\n", tmp);
    else
        printf ("            Failed to read reg[%s]\n",
            "EPLD_WDOG_LEVEL_DEBUG");

    if (epld_reg_read(EPLD_WATCHDOG_L1_TIMER, &tmp))
        printf ("            L1 timer =  0x%x\n", tmp);
    else
        printf ("            Failed to read reg[%s]\n","EPLD_WDOG_L1_TIMER");

    if (epld_reg_read(EPLD_WATCHDOG_L2_TIMER, &tmp))
        printf ("            L2 timer =  0x%x\n", tmp);
    else
        printf ("            Failed to read reg[%s]\n","EPLD_WDOG_L2_TIMER");

    if (epld_reg_read(EPLD_WATCHDOG_L3_TIMER, &tmp))
        printf ("            L3 timer =  0x%x\n", tmp);
    else
        printf ("            Failed to read reg[%s]\n","EPLD_WDOG_L3_TIMER");

    if (epld_reg_read(EPLD_WATCHDOG_L4_TIMER, &tmp))
        printf ("            L4 timer =  0x%x\n", tmp);
    else
        printf ("            Failed to read reg[%s]\n","EPLD_WDOG_L4_TIMER");

}

int 
epld_cpu_soft_reset (void)
{
    return epld_reg_write(EPLD_CPU_RESET, EPLD_CPU_SOFT_RESET);
}

int 
epld_cpu_hard_reset (void)
{
    return epld_reg_write(EPLD_CPU_RESET, EPLD_CPU_HARD_RESET);
}

int 
epld_system_reset (void)
{
    return epld_reg_write(EPLD_SYSTEM_RESET, EPLD_SYS_RESET);
}

int 
epld_system_reset_hold (void)
{
    return epld_reg_write(EPLD_SYSTEM_RESET, EPLD_SYS_RESET_HOLD);
}

#endif
