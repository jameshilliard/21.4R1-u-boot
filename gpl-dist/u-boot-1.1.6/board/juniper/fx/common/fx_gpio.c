/*
 * Copyright (c) 2010, Juniper Networks, Inc.
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



/*****************************************************
  RMI GPIO API

  Author: Bilahari Akkiraju
******************************************************/

#include <common.h>
#include "fx_common.h"
#include "post.h"

volatile uint32_t *gpio_io_reg   = (volatile uint32_t *)XLR_GPIO_IO_REG;
volatile uint32_t *gpio_data_reg = (volatile uint32_t *)XLR_GPIO_DATA_REG;

void
gpio_cfg_io_set(uint32_t io_mask)
{
    post_printf(POST_VERB_DBG, "Writing 0x%x to cfg reg of gpio\n",io_mask);
   *gpio_io_reg = io_mask;
}

void 
gpio_write_data_set(uint32_t data_val)
{
    post_printf(POST_VERB_DBG, "Writing 0x%x to data reg of gpio\n",data_val);
   *gpio_data_reg = data_val;
}

