/*
 * Copyright (c) 2008-2010, Juniper Networks, Inc.
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

#ifndef __RPS_PORTMON_H
#define __RPS_PORTMON_H

extern void initPortMon(void);
extern void startPortMon(int32_t start_time, int32_t cycle_time);
extern void stopPortMon(void);
extern void portUpdate(uint8_t ps, uint8_t fan);
extern uint8_t getPortState(int port);
extern char * portStateString(uint8_t state);
extern void initPortPresentMon(void);
extern void startPortPresentMon(int32_t start_time, int32_t cycle_time);
extern void stopPortPresentMon(void);
extern void portUpdate(uint8_t ps, uint8_t fan);
extern int32_t isPortPresent(int port);
extern int32_t isPortUp(int port);
extern void resetPortI2C(int port);
extern int32_t getPortI2CTimeout(int port);
extern void clearPortI2CTimeout(int port);
extern int32_t getI2CErrorProtocol(void);
extern void clearI2CErrorProtocol(void);
extern int32_t getI2CErrorAddress(void);
extern void clearI2CErrorAddress(void);

#endif	/* __RPS_PORTMON_H */
