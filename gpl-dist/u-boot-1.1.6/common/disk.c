/*
 * $Id: disk.c,v 1.2 2009-04-29 21:33:21 vishalg Exp $
 *
 * disk.c: Disk functions implementation
 *
 * Copyright (c) 2009-2011, Juniper Networks, Inc.
 * All rights reserved.
 *
 */

#include <common.h>
#ifdef CONFIG_JSRXNLE
#include <platform_srxsme.h>
#endif
#ifdef CONFIG_MAG8600
#include <platform_mag8600.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

int
disk_export_scan (int dev)
{
#ifdef CONFIG_JSRXNLE
    int max_disks = DISK_MAX_SRXMR - 1;
    
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX650_MODELS
	/* 
	 * For SRX650 we assume devices (internal, external CF and USB 0)
	 * to be present. We check for usb 1 separately. If device is not 
	 * present then that will be take care by next boot sequence code 
	 * or disk_read function
	 */
	if (IS_DISK_USB_SRXMR(dev + 1)) {
		(void)usb_disk_scan(dev);
	}
	if (num_usb_disks() > 1)
	    max_disks++;

	if (dev < 0)
	    return (0);
	else if (dev < max_disks)
	    return (dev + 1);
    break;
    CASE_ALL_SRXLE_CF_MODELS
        /*
         * For SRXLE CF models, we have the internal compact flash and USB as
         * the boot devices.
         */
        if (dev < 1) {
            if (dev == -1) {
                (void)usb_disk_scan(dev);
            }
            if (dev == 0) {
                if (srxle_cf_present()) {
                    srxle_cf_enable();
                }
            }
            return (dev + 1);
        }
    break;
    default:
        break;
    }
#endif
#ifdef CONFIG_MAG8600
        int max_disks = DISK_MAX_MAG8600 - 1;
        switch (dev) {
            case DISK_USB0_MAG8600:     
	         (void)usb_disk_scan(dev);
       		  break;
	    case DISK_DEFAULT_MAG8600:
    		 break;
        }
	if (dev < max_disks)
	    return (dev + 1);
#endif
	return (-1);
}

int
disk_export_read (int dev, int lba, int totcnt, char *buf)
{
#ifdef CONFIG_JSRXNLE
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX650_MODELS
	if (IS_DISK_USB_SRXMR(dev)) {
	    if (dev == DISK_USB1_SRXMR && num_usb_disks() >= 2)
	        return (usb_disk_read(1, lba, totcnt, buf));
	    else
	        return (usb_disk_read(0, lba, totcnt, buf));
	} else {
	    return (cf_disk_read(dev, lba, totcnt, buf));
	}
    break;

    CASE_ALL_SRXLE_CF_MODELS
        if (dev == DISK_DEFAULT_SRXLE_CF_MODELS) {
            return (cf_disk_read(dev, lba, totcnt, buf));
        } else if (dev == DISK_SECOND) {
            return (ssd_disk_read(dev, lba, totcnt, buf));
        } else {
            return (usb_disk_read(0, lba, totcnt, buf));
        }
    break;
    default:
        break;
    }
#endif
#ifdef CONFIG_MAG8600
        if (dev == DISK_DEFAULT_MAG8600) {
            return (cf_disk_read(dev, lba, totcnt, buf));
        } else if (dev == DISK_USB0_MAG8600) {
            return (usb_disk_read(0, lba, totcnt, buf));
        }

#endif
	return (-1);
}

int
disk_export_write (int dev, int lba, int cnt, void *buf)
{
#ifdef CONFIG_JSRXNLE
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX650_MODELS
	if (IS_DISK_USB_SRXMR(dev)) {
	    return (usb_disk_write(dev, lba, cnt, buf));
	} else {
	    return (cf_disk_write(dev, lba, cnt, buf));
	}
    break;

    CASE_ALL_SRXLE_CF_MODELS
        if (dev == DISK_DEFAULT_SRXLE_CF_MODELS) {
            return (cf_disk_write(dev, lba, cnt, buf));
        } else if (dev == DISK_SECOND) {
            return (ssd_disk_write(dev, lba, cnt, buf));
        } else {
            return (usb_disk_write(dev, lba, cnt, buf));
        }
    break;

    default:
        break;
    }
#endif
#ifdef CONFIG_MAG8600
        if (dev == DISK_DEFAULT_MAG8600) {
	    return (cf_disk_write(dev, lba, cnt, buf));
        } else if (dev == DISK_USB0_MAG8600) {
            return (usb_disk_write(dev, lba, cnt, buf));
        }
#endif
	return (-1);
}

