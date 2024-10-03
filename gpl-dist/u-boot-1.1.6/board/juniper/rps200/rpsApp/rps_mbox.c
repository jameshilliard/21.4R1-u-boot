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

#include "rps.h"
#include "rps_string.h"
#include "rps_cpld.h"
#include "rps_mbox.h"

int32_t 
get_mbox (int32_t port, uint8_t *data)
{
    return (cpld_read_direct(PORT_1_IN_MAILBOX+port*2, data, 2));
}

int32_t 
set_mbox (int32_t port, uint8_t *data)
{
    return (cpld_write(PORT_1_IN_MAILBOX+port*2, data, 2));
}

int32_t 
get_mboxes (uint8_t *data)
{
    return (cpld_read_direct(PORT_1_IN_MAILBOX, data, 2*CPLD_MAX_PORT));
}

int32_t 
set_mboxes (uint8_t *data)
{
    return (cpld_write(PORT_1_IN_MAILBOX, data, 2*CPLD_MAX_PORT));
}

int32_t
clear_mboxes (void)
{
    uint8_t data[2*CPLD_MAX_PORT];

    memset(data, 0x0, sizeof(data));
    return (set_mboxes(data));
}

