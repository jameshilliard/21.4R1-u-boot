/*
 * $Id$
 *
 * usb.c -- Diagnostics for the iCRT USB enumeration
 *
 * Copyright (c) 2011-2014, Juniper Networks, Inc.
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

#include <common.h>

#include <configs/mx80.h>
#include <acx_i2c_usb2513.h>

#ifdef CONFIG_POST

#include <post.h>
#include "acx_icrt_post.h"

#if CONFIG_POST & CFG_POST_USB

#include <usb.h>

extern struct usb_device usb_dev[USB_MAX_DEVICE];
extern char usb_started;
int post_result_usb;

static struct exp_device {
	char mf[32];
	char prod[32];
	int parent;
	int parent_portnum;
	char comments[32];
} exp_devices[] = {
	{"Philips", "EHCI", 0, 0, "ROOT PORT"},	/* Root controller */
	{USB2513_MFR_STRING, USB2513_PROD_STRING, 1, 1, "USB HUB"},				/* Hub */
	{"        ", "USB DISK 2.0", 2, 3, "NAND FLASH"},	/* NAND Flash */
};


static int get_parent(struct usb_device *parent)
{
	int i;	
	for (i = 0; i < USB_MAX_DEVICE; i++) {
		if ((usb_dev[i].devnum != -1) && (parent == &usb_dev[i])) {
			return (i + 1);
		}
	}
	return 0;
}

#undef  INJECT_ERROR

int usb_post_test(int flags)
{
	unsigned int i = 0, j = 0;
	struct usb_device *dev = NULL;
	struct exp_device *search_dev = NULL;
	int numdevs, found;
	int exp_num_devices = sizeof(exp_devices) / sizeof(exp_devices[0]);
	post_result_usb = 1;
	char tmp[10];

#ifdef INJECT_ERROR
	exp_devices[1].serial[2] &= 0;
#endif

	POST_LOG(POST_LOG_IMP, "\n======================USB TEST START===================\n");
	POST_LOG(POST_LOG_INFO, "This tests the connectivity to on board USB components\n");
	POST_LOG(POST_LOG_INFO, "Enumerated devices are validated against the set of pre-known\n");
	POST_LOG(POST_LOG_INFO, "expected devices list to verify everything is OK & expected.\n\n");

	POST_LOG(POST_LOG_INFO, "Expected Device list (total %d devices):\n", exp_num_devices);
	POST_LOG(POST_LOG_INFO, "DevNum \t Parent  ParentPort \tManufacturer \t Device\n");
	POST_LOG(POST_LOG_INFO, "------ \t ------  ---------- \t------------ \t ------\n");
	for (i = 0; i < exp_num_devices; i++) {
		if (!exp_devices[i].parent) 
			sprintf(tmp, "N.A. ");
		else
			sprintf(tmp, "dev-%d", exp_devices[i].parent);
		POST_LOG(POST_LOG_INFO, "dev-%d \t %-6s  %-10d \t%-10s \t %-10s \t[%s]\n",
			     i + 1, tmp, exp_devices[i].parent_portnum, exp_devices[i].mf, exp_devices[i].prod,
				 exp_devices[i].comments);
	}

	if (!usb_started) {
		POST_LOG(POST_LOG_ERR, "USB is stopped. Starting usb...\n");
		usb_init();
	}

	numdevs = 0;
	for (i = 0; i < USB_MAX_DEVICE; i++) {
		if (usb_dev[i].devnum != -1)
			numdevs++;
	}
	POST_LOG(POST_LOG_INFO, "\nActually Enumerated USB devices (%d devices) ...\n", numdevs);
	POST_LOG(POST_LOG_INFO, "DevNum \t Parent  ParentPort \tManufacturer \t Device\n");
	POST_LOG(POST_LOG_INFO, "------ \t ------  ---------- \t------------ \t ------\n");
	for (i = 0; i < USB_MAX_DEVICE; i++) {
		if (usb_dev[i].devnum != -1) {
			if (!get_parent(usb_dev[i].parent)) 
				sprintf(tmp, "N.A. ");
			else
				sprintf(tmp, "dev-%d", get_parent(usb_dev[i].parent));
			POST_LOG(POST_LOG_INFO, "dev-%d \t %-6s  %-10d \t%-10s \t %-10s\n",
				     i + 1, tmp, usb_dev[i].portnr, usb_dev[i].mf, usb_dev[i].prod);
		}
	}

	if (numdevs < exp_num_devices) {
		POST_LOG(POST_LOG_ERR, "\n\tError! Atleast %d USB devices not enumerated."
							   "Only %d devices identified\n", exp_num_devices - numdevs, numdevs);
		post_result_usb = -1;
	} else if (numdevs > exp_num_devices) {
		POST_LOG(POST_LOG_ERR, "\nUmmm ... %d additional devices identified, this may be OK (devices may"
							   "be plugged in external USB port)\n", numdevs - exp_num_devices);
	} else {
		POST_LOG(POST_LOG_INFO, "Num of USB devices is OK\n", numdevs);
	}

	POST_LOG(POST_LOG_INFO, "\tMatching Enumerated devices with expected Device list ...\n");
	for (i = 0; i < exp_num_devices; i++) {

		search_dev = &exp_devices[i];
		found = 0;

		POST_LOG(POST_LOG_INFO, "\t\tLooking for [%s] [%s - %s]...",
			    search_dev->comments, search_dev->mf, search_dev->prod);

		for (j = 0; (j < USB_MAX_DEVICE) && !found; j++) {
			dev = &usb_dev[j];
			if ((dev->devnum == -1) ||
				(dev->descriptor.bDescriptorType != USB_DT_DEVICE))
				break;
			if (!strcmp(dev->mf, search_dev->mf) &&
				!strcmp(dev->prod, search_dev->prod) &&
				(search_dev->parent == get_parent(dev->parent)) &&
				(dev->portnr == search_dev->parent_portnum))
				found = 1;
		}

		if (!found) {
			post_result_usb = -1;
			POST_LOG(POST_LOG_ERR, "\n\t\tError! Expected USB device [%s] [%s - %s] NOT Found!\n",
				     search_dev->comments, search_dev->mf, search_dev->prod);
		} else {
			POST_LOG(POST_LOG_INFO, "found\n");
		}
	}

	if (post_result_usb == -1) {
		POST_LOG(POST_LOG_ERR, "\tError! Enumerated devices DO NOT match the expected Device list!\n");
		POST_LOG(POST_LOG_ERR, "\tList of Enumerated devices:\n");
		POST_LOG(POST_LOG_INFO, "DevNum \t Parent  ParentPort \tManufacturer \t Device\n");
		POST_LOG(POST_LOG_INFO, "------ \t ------  ---------- \t------------ \t ------\n");
		for (i = 0; i < USB_MAX_DEVICE; i++) {
			if (usb_dev[i].devnum != -1) {
				if (!get_parent(usb_dev[i].parent)) 
					sprintf(tmp, "N.A. ");
				else
					sprintf(tmp, "dev-%d", get_parent(usb_dev[i].parent));
				POST_LOG(POST_LOG_INFO, "dev-%d \t %-6s  %-10d \t%-10s \t %-10s\n",
					     i + 1, tmp, usb_dev[i].portnr, usb_dev[i].mf, usb_dev[i].prod);
			}
		}
		POST_LOG(POST_LOG_INFO, "Final USB Test Result = FAILED\n");
		POST_LOG(POST_LOG_IMP, "======================USB TEST END=====================\n");
		return -1;
	} else {
		POST_LOG(POST_LOG_INFO, "\tSuccess! Enumerated devices MATCHES the expected Device list!\n");
		POST_LOG(POST_LOG_INFO, "Final USB Test Result = PASSED\n");
		POST_LOG(POST_LOG_IMP, "======================USB TEST END=====================\n");
		return 0;
	}

	return 0;

}

#endif /* CONFIG_POST & CFG_POST_USB */
#endif /* CONFIG_POST */
