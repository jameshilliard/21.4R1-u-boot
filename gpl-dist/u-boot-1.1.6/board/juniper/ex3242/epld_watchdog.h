/*
 * Copyright (c) 2007-2010, Juniper Networks, Inc.
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

#ifndef __EPLD_WATCHDOG__
#define __EPLD_WATCHDOG__

extern void epld_watchdog_init (void);
extern void epld_watchdog_reset (void);
extern int epld_watchdog_disable (void);
extern int epld_watchdog_enable (void);
extern void epld_watchdog_dump (void);
extern int epld_cpu_soft_reset (void);
extern int epld_cpu_hard_reset (void);
extern int epld_system_reset (void);
extern int epld_system_reset_hold (void);

#endif /*__EPLD_WATCHDOG__*/
