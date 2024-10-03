/*
 * $Id$
 *
 * nand.c -- NAND flash diagnostics for the iCRT
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

#include <scsi.h>
#include <malloc.h>

#ifdef CONFIG_POST

#include <post.h>
#include "acx_icrt_post.h"

#if CONFIG_POST & CFG_POST_NAND

#include <usb.h>

#define USB_WRITE_READ /* Do a NAND Flash Read / write test */

extern block_dev_desc_t usb_dev_desc[USB_MAX_STOR_DEV];

extern unsigned long usb_stor_write(unsigned long device, unsigned long blknr,
	unsigned long blkcnt, unsigned long *buffer);

static int usb_stor_curr_dev; /* Current device */
extern int usb_max_devs; /* number of highest available usb device */
extern char usb_started;
int usb_scan_dev;
extern int dev_usb_index;

int post_result_nand;

#define NUM_TEST_BLKS    8          /* Num of blocks to read write at every boundry */
#define NUM_SKIP_BLKS    (2 * 1024)	/* Test 8 blks every 128K blocks */


int nand_post_test(int flags)
{
    unsigned int i, j, k;
    unsigned long numblk, blksize;

    block_dev_desc_t *stor_dev;
    unsigned int  device = 0;
    struct usb_device *dev = NULL;

    unsigned long read_cnt;
    char *read_buffer = NULL;
    char *write_buffer = NULL;

#if defined(USB_WRITE_READ)
    unsigned long write_cnt;
#endif

    usb_scan_dev = 1;
    post_result_nand = 1;

    POST_LOG(POST_LOG_IMP, "================NAND FLASH TEST START==================\n");
    POST_LOG(POST_LOG_INFO, "The test is destructive. It writes patterns into all the"
	     " detected USB storage\n");
    POST_LOG(POST_LOG_INFO, "devices and reads them back. %d blocks out of every %d "
	     "blocks are checked.\n", NUM_TEST_BLKS, NUM_SKIP_BLKS);
    POST_LOG(POST_LOG_INFO, "(For a device with 512B blocks, this is %d KB on every "
	     "%d MB boundary)\n", (NUM_TEST_BLKS * 512) >> 10, 
	     (NUM_SKIP_BLKS * 512) >> 20);

    if (!usb_started) {
	POST_LOG(POST_LOG_ERR, "USB is stopped. Please issue 'usb start' first before "
		 "running NAND diagnostic.\n");
	    post_result_nand = -1;
	    return -1;
	}

    if (usb_max_devs <= 0) {
	POST_LOG(POST_LOG_ERR, "Error! NO Storage device found\n");
	post_result_nand = -1;
	return -1;
    }

    for (j = 0; j < usb_max_devs; j++) {

	usb_stor_curr_dev = j;
	/* write and read the data to the usb memory stick*/
	stor_dev = usb_stor_get_dev(usb_stor_curr_dev);
	device  = usb_stor_curr_dev;

	device &= 0xff;
	/* 
	 * Setup  device
	 */
	dev = NULL;
	for (i = 0; i < USB_MAX_DEVICE; i++) {
	    dev = usb_get_dev_index(i);
	    if (dev == NULL) {
		POST_LOG(POST_LOG_ERR, "Error! USB datastructures are corrupted\n");
		post_result_nand = -1;
		return -1;
	    }
	    if (dev->devnum == usb_dev_desc[device].target) {
		break;
	    }
	}

	/* Read number of blocks of usb device */	
	numblk = usb_dev_desc[device].lba;
	blksize = usb_dev_desc[device].blksz;

	POST_LOG(POST_LOG_INFO, "\t[Testing dev #%d]\n", j);
	POST_LOG(POST_LOG_INFO, "\tDevice    : %s - %s\n", dev->mf, dev->prod);
	POST_LOG(POST_LOG_INFO, "\tSerial No : %s\n", dev->serial);
	POST_LOG(POST_LOG_INFO, "\tBlk Size  : %d bytes\n", blksize);
	POST_LOG(POST_LOG_INFO, "\tTotal Blks: 0x%X blks (= %d MB)\n", 
		 numblk, (numblk * blksize) >> 20);
	POST_LOG(POST_LOG_INFO, "\tTest Blks : 0x%X blks on every 0x%X blk boundary\n",
		 NUM_TEST_BLKS, NUM_SKIP_BLKS);
	POST_LOG(POST_LOG_INFO, "\t         => %d KB on every %d MB boundary\n", 
		 (NUM_TEST_BLKS * blksize) >> 10, (NUM_SKIP_BLKS * blksize) >> 20);
	POST_LOG(POST_LOG_INFO, "\t         => Total appx %u loops (\'.\' printed"
		 " for every 10 iterations)\n", numblk / NUM_SKIP_BLKS);

	write_buffer = (char*) malloc (blksize * NUM_TEST_BLKS);
	read_buffer = (char*) malloc (blksize * NUM_TEST_BLKS);

#if defined(USB_WRITE_READ)

	POST_LOG(POST_LOG_INFO, "\t\t[Erasing test blks...]\n");
	memset(write_buffer, 0, blksize * NUM_TEST_BLKS);

	for (i = 0, k = 0; i < numblk; i += NUM_SKIP_BLKS, k++) {
	    POST_LOG(POST_LOG_DEBUG, "\t\t\t(blk 0x%X - blk 0x%X)...\n", i, i + 7);
	    write_cnt = usb_stor_write(usb_stor_curr_dev, i, NUM_TEST_BLKS, 
				       (ulong *)write_buffer);
	    if (write_cnt != NUM_TEST_BLKS) {
		POST_LOG(POST_LOG_ERR, "\t\t\tWrite Failed at blks (0x%X - 0x%X)\n",
			 i, i + NUM_TEST_BLKS);
		post_result_nand = -1;
		break;
	    }
	    if (k % 800 == 0)
		POST_LOG(POST_LOG_IMP, "\n\t\t\t");
	    if (k % 10 == 0)
		POST_LOG(POST_LOG_IMP, ".", i);
	}

	if (post_result_nand == -1) {
	    POST_LOG(POST_LOG_ERR, "\n\t\t[Erasing test blks... FAILURE at blks "
		     "(%d - %d)...]\n", i, i + NUM_TEST_BLKS);
	    free(read_buffer);
	    free(write_buffer);
	    goto done;
	} else {
	    POST_LOG(POST_LOG_INFO, "\n\t\t[Erasing test blks... SUCCESS]\n");
	}

	POST_LOG(POST_LOG_INFO, "\t\t[Writing test pattern on test blks...]\n");
	memset(write_buffer, '1', blksize * NUM_TEST_BLKS);
	for (i = 0, k = 0; i < numblk; i += NUM_SKIP_BLKS, k++) {
	    POST_LOG(POST_LOG_DEBUG, "\t\t\t(blk 0x%X - blk 0x%X)...\n",
		     i, i + NUM_TEST_BLKS);
	    write_cnt = usb_stor_write(usb_stor_curr_dev, i, NUM_TEST_BLKS,
				       (ulong *)write_buffer);
	    if (write_cnt != NUM_TEST_BLKS) {
		POST_LOG(POST_LOG_ERR, "\t\t\tWrite Failed at blks (0x%X - 0x%X)\n",
			 i, i + NUM_TEST_BLKS);
		post_result_nand = -1;
		break;
	    }
	    if (k % 800 == 0)
		POST_LOG(POST_LOG_IMP, "\n\t\t\t");
	    if (k % 10 == 0)
		POST_LOG(POST_LOG_IMP, ".", i);
	}
	if (post_result_nand == -1) {
	    POST_LOG(POST_LOG_ERR, "\n\t\t[Writing test pattern on test blks...FAILURE "
		     "at blks (%d - %d)...]\n", i, i + NUM_TEST_BLKS);
	    free(read_buffer);
	    free(write_buffer);
	    goto done;
	} else {
	    POST_LOG(POST_LOG_INFO, "\n\t\t[Writing test pattern on test blks...SUCCESS]\n");
	}

	POST_LOG(POST_LOG_INFO, "\t\t[Reading and Verifying test blks...]\n");
	for (i = 0, k = 0; i < numblk; i += NUM_SKIP_BLKS, k++) {
	    POST_LOG(POST_LOG_DEBUG, "\t\t\t(blk 0x%X - blk 0x%X)...\n",
		     i, i + 7);
	    memset(read_buffer, 0xFF, blksize * NUM_TEST_BLKS);
	    read_cnt = stor_dev->block_read(usb_stor_curr_dev, i, NUM_TEST_BLKS,
					    (ulong *)read_buffer);

	    if (read_cnt != NUM_TEST_BLKS) {
		POST_LOG(POST_LOG_ERR, "Read Failed at blks (%d - %d)\n", i, i + NUM_TEST_BLKS);
		post_result_nand = -1;
		break;
	    }
	    if (memcmp(read_buffer, write_buffer, blksize * NUM_TEST_BLKS)) {
		POST_LOG(POST_LOG_ERR, "Error! Read Data does not match at blks (%d - %d)\n",
			 i, i + NUM_TEST_BLKS);
		post_result_nand = -1;
		break;
	    }
	    if (k % 800 == 0)
		POST_LOG(POST_LOG_IMP, "\n\t\t\t");
	    if (k % 10 == 0)
		POST_LOG(POST_LOG_IMP, ".", i);
	}
	if (post_result_nand == -1) {
	    POST_LOG(POST_LOG_ERR, "\n\t\t[Reading and Verifying test blks...FAILURE at blks"
		     "(%d - %d)...]\n", i, i + NUM_TEST_BLKS);
	    free(read_buffer);
	    free(write_buffer);
	    goto done;
	} else {
	    POST_LOG(POST_LOG_INFO, "\n\t\t[Reading and Verifying test blks... SUCCESS]\n");
	}
#else
	POST_LOG(POST_LOG_INFO, "\t\t[Reading test blks...]\n");
	for (i = 0, k = 0; i < numblk; i += NUM_SKIP_BLKS, k++) {

	    POST_LOG(POST_LOG_DEBUG, "\t\t\t(blk 0x%X - blk 0x%X)...\n", i, i + 7);
	    memset(read_buffer, '0', blksize * NUM_TEST_BLKS);
	    read_cnt = stor_dev->block_read(usb_stor_curr_dev, i, NUM_TEST_BLKS,
					    (ulong *)read_buffer);

	    if (read_cnt != NUM_TEST_BLKS) {
		POST_LOG(POST_LOG_ERR, "Read Failed at blks (%d - %d)\n", i, i + NUM_TEST_BLKS);
		post_result_nand = -1;
		break;
	    }
	    if (k % 800 == 0)
		POST_LOG(POST_LOG_IMP, "\n\t\t\t");
	    if (k % 10 == 0)
		POST_LOG(POST_LOG_IMP, ".", i);
	}

	if (post_result_nand == -1) {
	    POST_LOG(POST_LOG_ERR, "\n\t\t[Reading test blks...FAILURE at blks (%d - %d)...]\n",
		     i, i + NUM_TEST_BLKS);
	    free(read_buffer);
	    free(write_buffer);
	    goto done;
	} else {
	    POST_LOG(POST_LOG_INFO, "\n\t\t[Reading test blks... SUCCESS]\n");
	}
#endif
	free(read_buffer);
	free(write_buffer);
	POST_LOG(POST_LOG_INFO, "\t[NAND flash test on dev %d: %s - %s (Serial: %s)] PASSED.\n",
		 j, dev->mf, dev->prod, dev->serial);
    }
done:
    if (post_result_nand == -1) {
	POST_LOG(POST_LOG_ERR, "\t[NAND flash test on dev %d: %s - %s (Serial: %s)] FAILED.\n",
		 j, dev->mf, dev->prod, dev->serial);
	POST_LOG(POST_LOG_ERR, "\tBailing out...\n");
	POST_LOG(POST_LOG_ERR, "NAND Flash Test FAILED\n");
	POST_LOG(POST_LOG_IMP, "================NAND FLASH TEST END====================\n");
	return -1;
    } else {
	POST_LOG(POST_LOG_INFO, "NAND Flash Test PASSED\n");
	POST_LOG(POST_LOG_IMP, "================NAND FLASH TEST END====================\n");
	return 0;
    }
}
#endif /* CONFIG_POST & CFG_POST_NAND */
#endif /* CONFIG_POST */
