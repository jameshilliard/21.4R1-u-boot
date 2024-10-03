/*
 * Copyright (c) 2008-2011, Juniper Networks, Inc.
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

#ifndef __RPS_PRIORITY_H
#define __RPS_PRIORITY_H

extern void initPriority(void);
extern void priorityControl(int32_t port, int32_t pri,
                            int32_t pwr_12V, int32_t pwr_48V);
extern uint8_t getPriorityRequested(int32_t port);
extern uint8_t getPriorityRemapped(int port);
extern uint8_t getPriorityActualCache(int port);
extern int32_t getPowerRequested(int32_t port);
extern int32_t get12VPowerRequested(int32_t port);
extern int32_t get48VPowerRequested(int32_t port);
extern uint8_t getMaxActiveCache(void);
extern void updatePriorityPortCache(uint8_t p0_2, uint8_t p3_5);
extern int getNumPortActiveBackupCache(void);
extern int isPortArmedWontBackupCache(int32_t port);
extern void priorityRemap(void);
extern void updateMax(void);
extern void pri_Handler(ptData ptdata);

#define     SCALE_1         1
#define     SCALE_2         2
#define     SCALE_1_VAL     0x03
#define     SCALE_2_VAL     0x0E

#endif	/* __RPS_PRIORITY_H */
