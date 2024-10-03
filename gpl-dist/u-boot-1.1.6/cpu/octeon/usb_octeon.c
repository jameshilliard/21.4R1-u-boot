/*-
 * Copyright (c) 2011, Juniper Networks, Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>

DECLARE_GLOBAL_DATA_PTR;

int
usb_lowlevel_init(void)
{
    switch (gd->board_desc.board_type)
    {
#ifdef CONFIG_JSRXNLE
        CASE_ALL_SRX550_MODELS
            return ehci_usb_lowlevel_init();
#endif
        default:
            return dwc_usb_lowlevel_init();
    }
}

int
usb_lowlevel_stop(uint32_t port_n)
{
    switch (gd->board_desc.board_type)
    {
#ifdef CONFIG_JSRXNLE
        CASE_ALL_SRX550_MODELS
            return ehci_usb_lowlevel_stop();
#endif
        default:
            return dwc_usb_lowlevel_stop();
    }
}

int
submit_bulk_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
    int length)
{

    switch (gd->board_desc.board_type)
    {
#ifdef CONFIG_JSRXNLE
        CASE_ALL_SRX550_MODELS
            return ehci_submit_async(dev, pipe, buffer, length, NULL);
#endif
        default:
            return dwc_submit_common_msg(dev, pipe, buffer, length, NULL);
    }
}

int
submit_control_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
    int length, struct devrequest *setup)
{
    switch (gd->board_desc.board_type)
    {
#ifdef CONFIG_JSRXNLE
        CASE_ALL_SRX550_MODELS
            return ehci_submit_control_msg(dev, pipe, buffer, length, setup);
#endif
        default:
            return dwc_submit_control_msg(dev, pipe, buffer, length, setup);
    }
}

int
submit_int_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
    int length, int interval)
{
    switch (gd->board_desc.board_type)
    {
#ifdef CONFIG_JSRXNLE
        CASE_ALL_SRX550_MODELS
            return ehci_submit_int_msg(dev, pipe, buffer, length, interval);
#endif
        default:
            return dwc_submit_int_msg(dev, pipe, buffer, length, interval);
    }
}

