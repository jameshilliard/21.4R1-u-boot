/*
 * $Id:
 *
 * watchdog_cpu.h -watchdog cpu declarations.
 *
 *
 * Copyright (c) 2011, Juniper Networks, Inc.
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
 * along with this program. If not, see 
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>
 */

#ifndef __WATCHDOG_CPU_H__
#define __WATCHDOG_CPU_H__

#include <config.h>
/* 
 * watchdog reload values
 */ 
#define PAT_WATCHDOG       -1
#define ENABLE_WATCHDOG     1
#define DISABLE_WATCHDOG    0

/*
 * decleration for reload watchdog
 */ 
void reload_watchdog(int val);
extern void reload_cpld_watchdog(int val);

/* 
 * watchdog cpu reload macros
 */
#ifdef RELOAD_WATCHDOG_SUPPORT
#define RELOAD_WATCHDOG(val) reload_watchdog(val)
#define RELOAD_CPLD_WATCHDOG(val)  reload_cpld_watchdog(val)
#else
#define RELOAD_WATCHDOG(val)
#define RELOAD_CPLD_WATCHDOG(val)
#endif

#endif
