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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/gpl-2.0.html>.
 */

#ifndef POST_USB_STORAGE_H
#define POST_USB_STORAGE_H

#define USB_INTERNAL_DEVNUM  0

#define USB_READ_BLK_START   0
#define USB_READ_BLK_CNT    10
#define USB_WRITE_BLK_START  2
#define USB_WRITE_BLK_CNT    4

extern block_dev_desc_t usb_dev_desc[USB_MAX_STOR_DEV];
extern int usb_stor_curr_dev; /* Current device */ 

#endif /* POST_USB_STORAGE_H */
