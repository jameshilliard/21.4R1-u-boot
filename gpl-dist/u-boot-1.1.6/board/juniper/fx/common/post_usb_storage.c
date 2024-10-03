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

#include <common.h>

/*************************** FX ***************************/

#include <usb.h>
#include "post.h"
#include "post_usb_storage.h"

int 
diag_post_usb_storage_init (void)
{
    debug("%s:\n", __FUNCTION__);

    return (1);
}

int 
diag_post_usb_storage_run (void)
{
    struct usb_device *dev = NULL;
    block_dev_desc_t *stor_dev;
    unsigned long numblk = 0;
    unsigned long blkcnt = USB_READ_BLK_CNT;
    unsigned long bytecnt;
    unsigned long wbytecnt;
    unsigned long read_cnt;
    unsigned long write_cnt;
    char *buf = 0;
    char *buf_old = 0;
    char wpattern[2] = {0x55, 0xaa};
    int status = 0;
    int i, j = 0;
    long k = 0;

    if (usb_stor_curr_dev == -1) {
        post_printf(POST_VERB_DBG,
                    "Error: Storage device not found\n");
        return -1;
    }

    /* test read/write to internal USB storage */
    stor_dev = usb_stor_get_dev(0);

    for (i = 0; i < USB_MAX_DEVICE; i++) {
        dev = usb_get_dev_index(i);
        if (dev == NULL) {
            post_printf(POST_VERB_DBG,
                        "Error: Storage device not found\n");
            status = -1;
            goto done;
        }

        /* found internal USB (0) */
        if (dev->devnum == usb_dev_desc[USB_INTERNAL_DEVNUM].target)
            break;
    }

    numblk = usb_dev_desc[USB_INTERNAL_DEVNUM].lba;
    bytecnt = usb_dev_desc[USB_INTERNAL_DEVNUM].blksz * blkcnt;
    wbytecnt = usb_dev_desc[USB_INTERNAL_DEVNUM].blksz * USB_WRITE_BLK_CNT;

    buf = (char*)malloc(bytecnt);
    buf_old = (char*)malloc(wbytecnt);
    if ((buf == NULL) || (buf_old == NULL)) {
        status = -1;
        goto done;
    }

    /* write test blk 2 - 5 */
    read_cnt = stor_dev->block_read(0,
                                    USB_WRITE_BLK_START, 
                                    USB_WRITE_BLK_CNT, 
                                    (ulong *)buf_old);
    if (read_cnt != USB_WRITE_BLK_CNT) {
        post_printf(POST_VERB_DBG,
                    "Error: Storage device read failed.\n");
        status = -1;
        goto done;
    }

    /* write pattern test */
    for (i = 0; i < sizeof(wpattern); i++) {
        memset(buf, wpattern[i], bytecnt); 
        write_cnt = stor_dev->block_write(0, 
                                          USB_WRITE_BLK_START, 
                                          USB_WRITE_BLK_CNT, 
                                          (ulong*)buf);
        if (write_cnt != USB_WRITE_BLK_CNT) {
            post_printf(POST_VERB_DBG,
                        "Error: Storage device write failed.\n");
            status = -1;
            goto done;
        }

        memset(buf, 0xff, bytecnt); 
        read_cnt = stor_dev->block_read(0,
                                        USB_WRITE_BLK_START, 
                                        USB_WRITE_BLK_CNT, 
                                        (ulong *)buf);

        if (read_cnt != USB_WRITE_BLK_CNT) {
            post_printf(POST_VERB_DBG,
                        "Error: Storage device read failed.\n");
            status = -1;
            goto done;
        }
        /* verify */
        for (j = 0; j < wbytecnt; j++) {
            if (buf[j] != wpattern[i]) {
                post_printf(POST_VERB_DBG,
                            "Error: Storage device data %02x failed.\n",
                            wpattern[i]);
                status = -1;
                goto done;
            }
        }
    }
        
    /* restore blk 2 - 4 */
    write_cnt = stor_dev->block_write(0,
                                      USB_WRITE_BLK_START, 
                                      USB_WRITE_BLK_CNT, 
                                      (ulong*)buf_old);
    
    if (write_cnt != USB_WRITE_BLK_CNT) {
        post_printf(POST_VERB_DBG,
                    "Error: Storage device restore failed.\n");
        status = -1;
        goto done;
    }

    /* read test, access check (no verification).  slow */
    printf("\n");
    
    /* test 1st 10M blk by blk. after that, 10 blk out of 1024 */
    for (i = 0; i < (numblk - blkcnt);
         i = (i < 20480) ? (i + blkcnt) : (i + 1024)) {
        k++;

        /* show activity */
        if (0 == k % 100)
            printf(".");
        if (0 == k % 8000)
            printf("\n");

        read_cnt = stor_dev->block_read(0, i, blkcnt, (ulong *)buf);

        if (read_cnt != blkcnt) {
            post_printf(POST_VERB_DBG,
                        "Error: Storage device read failed at blk 0x%x\n",
                        i);
            status = -1;
            goto done;
        }
    }
    printf("\n");
    
done:
    if (buf) {
        free(buf);
    }
    if (buf_old) {
        free(buf_old);
    }

    return status;

}

int 
diag_post_usb_storage_clean (void)
{
    debug("%s:\n", __FUNCTION__);

    return 1;
}

