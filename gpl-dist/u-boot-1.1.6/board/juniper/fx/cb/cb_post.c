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

#include<common/post.h>

/* cb specific POST list */
diag_t cb_post_tests[ ]=
{
     POST_I2C_PROBE_TEST,
     POST_GMAC_LPBK_TEST,
     POST_USB_PROBE_TEST,
     POST_MULT_CORE_MEM_TEST,
     POST_DMA_TXR_TEST,
     POST_USB_STORAGE_TEST,
     POST_TEST_LIST_TERMINATOR
};

