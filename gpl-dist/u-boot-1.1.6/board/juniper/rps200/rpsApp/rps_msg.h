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

#ifndef __RPS_MSG__
#define __RPS_MSG__

typedef enum msgState 
{
    POLLING,
    I2C_CMD,
    I2C_REQ,
    I2C_REPLY
} MsgState;

extern MsgState msg_state[];

extern void initMsg(void);
extern void startMsgMon(int32_t start_time, int32_t cycle_time);
extern void stopMsgMon(void);

#endif /*__RPS_MSG__*/
