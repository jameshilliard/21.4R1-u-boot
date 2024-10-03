/*
 * Copyright (c) 2009-2010, Juniper Networks, Inc.
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

#include <common.h>
#include <part.h>
#include <watchdog.h>
#include <command.h>
#include <rmi/pci-phoenix.h>
#include <rmi/on_chip.h>
#include <rmi/byteorder.h>
#include <rmi/gpio.h>
#include <rmi/bridge.h>
#include <malloc.h>
#include <usb.h>

#include <common/post.h>

int diag_post_usb_init(void);

typedef struct DIAG_USB_DEVICE_INFO_T {
    char *manufacturer;
    char *product;
    char *serial;
} diag_usb_device_info_t;

diag_usb_device_info_t diag_usb_info[] = {
    { "RMI",
      "EHCI Root Hub",
      "2.0 HIGHSPEED HUB"
    }
};

int
diag_post_usb_init ()
{
    debug("%s:\n", __FUNCTION__);

    return 1;
}


int diag_post_usb_run(void);


int
diag_post_usb_run (void)
{
    struct usb_device *dev;
    int i;
    int status = 0;
    int dev_class;

    /*  
     *  Possible combinations:
     *  dev 0 Hub, dev 1 Storage device -- pass
     *  dev 0 Hub -- fail
     *  no devices -- fail
     */

    for (i = 0; i < USB_MAX_DEVICE; i++) {
        dev = usb_get_dev_index(i);
        if (dev == NULL) {
            if (i == 1) {
                post_printf(POST_VERB_DBG,
                            "Error: Storage device not found\n");

                status = -1;
            }
            break;
        }

        dev_class = dev->config.if_desc[0].bInterfaceClass;

        // Compare it
        switch(i) {
            case 0:
                if (dev_class == USB_CLASS_HUB) {
                    if ((strcmp(dev->mf, diag_usb_info[0].manufacturer)==0) &&
                        (strcmp(dev->prod, diag_usb_info[0].product) == 0) &&
                        (strcmp(dev->serial, diag_usb_info[0].serial) == 0)) {
                        post_printf(POST_VERB_DBG,
                                    "Matched & Found %s %s %s \n",
                                    dev->mf,
                                    dev->prod,
                                    dev->serial);
                        break;
                    }
                }
                post_printf(POST_VERB_STD,
                            "Error: HUB not found %s %s %s \n",
                            diag_usb_info[0].manufacturer,
                            diag_usb_info[0].product,
                            diag_usb_info[0].serial);
                status = -1;
                break;
                
            case 1:
                if (dev_class == USB_CLASS_MASS_STORAGE) {
                    /* storage device */
                    post_printf(POST_VERB_DBG,
                                "Found storage device %s %s %s \n",
                                dev->mf,
                                dev->prod,
                                dev->serial);
                    break;
                }
                post_printf(POST_VERB_DBG,
                            "Error: Storage device not found %s %s %s\n",
                            dev->mf,
                            dev->prod,
                            dev->serial);

                status = -1;
                break;
                
            default:
                break;
        }
    }
        
    if (status) {
        post_test_status_set(POST_USB_PROBE_ID, POST_USB_DEV_ABSENT);
    }

    return status;
}


int
diag_post_usb_clean (void)
{
    int status = 1;

    debug("%s:\n", __FUNCTION__);

    return status;
}


typedef struct DIAG_USB_T {
    char *test_name;
    int (*init)(void);
    int (*run)(void);
    int (*clean)(void);
} diag_usb_t;


diag_usb_t diag_post_usb = {
    "USB Probe Test\n",
    diag_post_usb_init,
    diag_post_usb_run,
    diag_post_usb_clean
};


int
diag_usb_post_main (void)
{
    int status = 0;

    debug("%s: \n", __FUNCTION__);

    post_printf(POST_VERB_STD,"Test Start: %s \n", diag_post_usb.test_name);

    status = diag_post_usb.init();

    if (status == 0) {
        goto diag_usb_post_main_exit;
    }

    status = diag_post_usb.run();

    if (status == 0) {
        goto diag_usb_post_main_exit;
    }




diag_usb_post_main_exit:

    diag_post_usb.clean();
    post_printf(POST_VERB_STD,"%s %s \n",
           diag_post_usb.test_name,
           status == 1 ? "PASS" : "FAILURE");
    post_printf(POST_VERB_STD,"Test End: %s \n", diag_post_usb.test_name);
    return status;
}


int
diag_post_usb_main (void)
{
    int status = 1;

    status = diag_usb_post_main();
    return status;
}


