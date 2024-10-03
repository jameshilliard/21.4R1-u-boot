/*
 * Copyright (c) 2011-2013, Juniper Networks, Inc.
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
#include "epld.h"
#include "epld_watchdog.h"

#if defined(CONFIG_EPLD_WATCHDOG)
void
epld_watchdog_init (void)
{
    volatile ccsr_pic_t *ccsr_pic_ptr = (void *)CONFIG_SYS_MPC8xxx_PIC_ADDR;
    unsigned long temp;

    temp = ccsr_pic_ptr->eivpr3;
    temp &= 0xff000000;
    temp += (1 << 22) + (IRQ1_PRIORITY << 16) + IRQ3_INT_VEC;
    ccsr_pic_ptr->eivpr3 = temp;
    temp &= 0x7fffffff;  /* enable IRQ3 */
    ccsr_pic_ptr->eivpr3 = temp;
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
	printf("EPLD:   Watchdog 0x%x\n", tmp);
    else {
	printf("EPLD:   Failed to read reg[EPLD_WDOG_ENABLE]\n");
	return;
    }

    if (epld_reg_read(EPLD_WATCHDOG_LEVEL_DEBUG, &tmp))
	printf("            Watchdog state 0x%x\n", tmp);
    else
	printf("            Failed to read reg[%s]\n",	
		"EPLD_WDOG_LEVEL_DEBUG");

    if (epld_reg_read(EPLD_WATCHDOG_L1_TIMER, &tmp))
	printf("            L1 timer =  0x%x\n", tmp);
    else
	printf("            Failed to read reg[%s]\n","EPLD_WDOG_L1_TIMER");

    if (epld_reg_read(EPLD_WATCHDOG_L2_TIMER, &tmp))
	printf("            L2 timer =  0x%x\n", tmp);
    else
	printf("            Failed to read reg[%s]\n","EPLD_WDOG_L2_TIMER");

    if (epld_reg_read(EPLD_WATCHDOG_L3_TIMER, &tmp))
	printf("            L3 timer =  0x%x\n", tmp);
    else
	printf("            Failed to read reg[%s]\n","EPLD_WDOG_L3_TIMER");

    if (epld_reg_read(EPLD_WATCHDOG_L4_TIMER, &tmp))
	printf("            L4 timer =  0x%x\n", tmp);
    else
	printf("            Failed to read reg[%s]\n","EPLD_WDOG_L4_TIMER");
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
