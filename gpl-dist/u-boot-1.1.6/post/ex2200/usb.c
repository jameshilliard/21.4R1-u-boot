/*
 * Copyright (c) 2009-2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * (C) Copyright 2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>

/*************************** EX3242 ***************************/

#include <scsi.h>
#include <malloc.h>

#ifdef CONFIG_POST

#include <post.h>

#if CONFIG_POST & CFG_POST_USB
#include <usb.h>

extern block_dev_desc_t usb_dev_desc[USB_MAX_STOR_DEV];

static int usb_stor_curr_dev; /* Current device */ 
extern int usb_max_devs; /* number of highest available usb device */
extern int usb_scan_dev;
extern int dev_usb_index;

int 
usb_post_test (int flags)
{
    unsigned int i =0;
    unsigned int j = 0;
    unsigned int k = 0;
    block_dev_desc_t *stor_dev;
    unsigned int  device = 0;
    struct usb_device *dev = NULL;
    unsigned long numblk = 0;
    unsigned long blkcnt = 10;
    unsigned long read_cnt;
    char      *buffer = 0;
    int usb_flag = 0;
    int usb_pass_flag = 0;
    
    usb_scan_dev =1;
    usb_flag =0;
    
    /* Stop the USB */
    if (flags & POST_DEBUG) {
        usb_flag =1;
    }
    if (usb_flag)
        post_log("This tests the onboard USB flash \n");
    
    if (usb_stop(0) < 0) {
        post_log("-- USB test                             FAILED @\n");
        if (usb_flag)
            post_log(" > USB stop                             FAILED @\n");
        return (-1);
    }

    /* Init the USB */
    if (usb_init() < 0) {
        post_log("-- USB test                             FAILED @\n");
        if (usb_flag)
            post_log(" > USB init                             FAILED @\n");
        return (-1);
    }

    /*
     *   check whether a storage device is attached (assume that it's
     *   a USB memory stick, since nothing else should be attached).
     */
    usb_stor_curr_dev = usb_stor_scan(1);

    if (usb_stor_curr_dev == -1) {
        post_log("-- USB test                             FAILED @\n");
        if (usb_flag)
            post_log(" > USB device found                     FAILED @\n");
        return (-1);
    }

    for (j = 0; j < usb_max_devs; j++) {
        usb_stor_curr_dev = j;
        /* write and read the data to the usb memory stick*/
        stor_dev=usb_stor_get_dev(usb_stor_curr_dev);
        device  = usb_stor_curr_dev;

        device &= 0xff;
        /* Setup  device */
        dev=NULL;
        for (i = 0; i < USB_MAX_DEVICE; i++) {
            dev=usb_get_dev_index(i);
            if (dev==NULL) {
                post_log("-- USB test                             FAILED @\n");
                if (usb_flag)
                    post_log(" > USB device found                     FAILED @\n");
                return (-1);
            }
            if (dev->devnum == usb_dev_desc[device].target)
                break;
        }

        /* Read number of blocks of usb device */	
        numblk = usb_dev_desc[device].lba;  /* 0 to usb_dev_desc[]->lba - 1 */
        buffer = (char*) malloc (usb_dev_desc[device].blksz * blkcnt);
        memset(buffer,'1', usb_dev_desc[device].blksz * blkcnt); 
        k = 0;
        /* test 1st 2.5M block by block. after that, 10 block out of 1024 */
        for (i = 0; i < numblk ; i = (i < 5120) ? (i + blkcnt) : (i + 1024)) {
            k++;
            if (usb_flag) {
                if (0 == k % 10)
                    post_log (".");
                if (0 == k % 800)
                    post_log ("\n");
            }
            udelay(100000);
            read_cnt = stor_dev->block_read(usb_stor_curr_dev, i, blkcnt, (ulong *)buffer);
			
            if (i == 5000) 
                i =numblk;
				
            if (!read_cnt) {
                goto done;
            }
        }
        if (usb_flag)
            post_log ("\n");
    }
    usb_pass_flag =1;
done:
    free(buffer);
    if (usb_pass_flag) {
        post_log("-- USB test                             PASSED\n");
        if (usb_flag) {
            post_log(" > The number of USB device found %d\n",dev_usb_index);
            post_log(" > The number of storage device found %d\n",usb_max_devs);
            post_log(" > Device: %d %s %s %s %dM\n",
					  device,dev->mf,dev->prod,dev->serial,
					  numblk*usb_dev_desc[device].blksz/1048576);
        }
        return (0);
    }
    else {
        post_log("-- USB test                             FAILED @\n");
        post_log ("Read Failed at block 0x%x\n", i);
        if (usb_flag) {
            post_log(" > The number of USB device found %d\n",dev_usb_index);
            post_log(" > The number of storage device found %d\n",usb_max_devs);
            post_log(" > Device: %d %s %s %s %dM\n",
					  device,dev->mf,dev->prod,dev->serial,
					  numblk*usb_dev_desc[device].blksz/1048576);
        }
        return (-1);
    }
}
#endif /* CONFIG_POST & CFG_POST_USB */
#endif /* CONFIG_POST */
