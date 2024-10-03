/*
 * Copyright (c) 2010-2011, Juniper Networks, Inc.
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

/* This header file contains the extra config settings required for
 * ex2200 12X skus in addition to ex2200 24x skus.
 */ 
#ifndef __CONFIG_H
#include "ex2200.h"

#define CONFIG_EX2200_12X           1

#define CFG_DUART_CHAN              0            /* channel to use for console */
#define CFG_INIT_CHAN1
#define CFG_INIT_CHAN2

#endif                                           /* __CONFIG_H */
