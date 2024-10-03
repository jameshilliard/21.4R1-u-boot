/*
 * $Id$
 *
 * fx_common.h
 *
 * Copyright (c) 2009-2013, Juniper Networks, Inc.
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
 * You should have received a copy of the GNU General Public
 * License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __FX_COMMON_H__
#define __FX_COMMON_H__

#include <common.h>

#define RMI_GMAC_TOTAL_PORTS       8

#ifdef  FX_COMMON_DEBUG
#define debug(fmt,args...)      printf (fmt ,##args)
#else
#define debug(fmt,args...)
#endif

#define b2_debug(fmt, args...)                          \
    do {                                                \
        if (getenv("boot_verbose")) {                   \
            printf(fmt ,##args);                        \
        }                                               \
    } while (0)

uint64_t get_macaddr(void);
uint8_t fx_is_cb(void);
uint8_t fx_is_soho(void);
uint8_t fx_is_pa(void);
uint8_t fx_is_qfx3500(void);
uint8_t fx_is_sfp_port(uint32_t gmac_id);
uint8_t fx_is_tor(void);
uint8_t fx_has_reset_cpld(void);
uint8_t fx_has_rj45_mgmt_board(void);
uint8_t fx_is_qfx3500_sfp_mgmt_board(void);
uint8_t fx_use_i2c_cpld(uint8_t i2c_addr);
uint8_t fx_has_i2c_cpld(void);
uint8_t wa_is_rev3_or_above(void);

extern uint16_t g_board_type;
extern uint16_t g_mgmt_board_type;
extern int usb_max_devs;

#define XLR_GPIO_IO_REG    0xbef18008
#define XLR_GPIO_DATA_REG  0xbef1800C

extern void rmi_print_board_map(void);

#endif //__FX_COMMON_H__
