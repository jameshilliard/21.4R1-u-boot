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

 #ifndef __RPS_ETQ_H
#define __RPS_ETQ_H

#define MAX_TIMER_PARAMS 3
#define MS_PER_TICK 5

typedef struct tdata
{
    int32_t param[MAX_TIMER_PARAMS];
} tData, *ptData;

typedef enum timerType { 
    ONE_SHOT,
    PERIDICALLY
} tTimerType;

void qInit(void);

extern int32_t addTimer(tData tdata, int32_t *tid, int32_t ttype, 
                        int32_t start, int32_t cycle, void (*pFunc)(ptData));
extern int32_t delTimer(int32_t tid);
extern void updateTimer(int32_t ticks);
extern void qDump(void);
extern void timeExpired(ptData ptdata);

#endif	/* __RPS_ETQ_H */
