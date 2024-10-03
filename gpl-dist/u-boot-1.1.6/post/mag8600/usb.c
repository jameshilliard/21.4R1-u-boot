/*
 * Copyright (c) 2011, Juniper Networks, Inc.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. if not ,see
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0,html>  
 */

#include <common.h>


#include <scsi.h>
#include <malloc.h>

#ifdef CONFIG_POST

#include <post.h>

#if CONFIG_POST & CFG_POST_USB
#include <usb.h>

#define	NUM_TEST_BLOCKS	    2

#define SET_USB_POST_RESULT(reg, result) \
	    reg = (result == POST_PASSED \
	    ? (reg & (~USB_POST_RESULT_MASK)) \
	    : (reg | USB_POST_RESULT_MASK))

extern block_dev_desc_t usb_dev_desc[USB_MAX_STOR_DEV];

static int usb_stor_curr_dev; /* Current device */ 
extern int usb_max_devs; /* number of highest available usb device */
static int usb_flag ;
int usb_scan_dev;
extern int dev_usb_index;

int 
usb_post_test (int flags)
{
    DECLARE_GLOBAL_DATA_PTR;
    unsigned int       i = 0;
    unsigned int       j = 0;
    unsigned int       k = 0;
    block_dev_desc_t  *stor_dev;
    unsigned int       device = 0;
    struct usb_device *dev = NULL;
    unsigned long      numblk = 0;
    unsigned long      blkcnt = NUM_TEST_BLOCKS;
    unsigned long      read_cnt, recheck_read_cnt = 0;
    char              *buffer = 0, *recheck_buffer = NULL;
    int                usb_pass_flag = 0;

    usb_scan_dev = 1;
    usb_flag     = 0;
	
    if (flags & POST_DEBUG) {
	usb_flag = 1;
    }
	
	
    /* 
     * The USB should be initialized before this post call
     * This would be in the board.c
     */
    
    for (j = 0; j < usb_max_devs; j++) {
    	usb_stor_curr_dev = j;
	
	/* write and read the data to the usb memory stick*/
	stor_dev = usb_stor_get_dev(usb_stor_curr_dev);

	device   = usb_stor_curr_dev;

	device  &= 0xff;
	/*
	 * Setup device
	 */
	dev=NULL;
	for (i=0;i<USB_MAX_DEVICE;i++) {
	    dev = usb_get_dev_index(i);
	    if(dev==NULL) {
		SET_USB_POST_RESULT(gd->post_result, POST_FAILED);
		return -1;
	    }
	    if(dev->devnum == usb_dev_desc[device].target){
		break;
	    }
	}


	/* Read number of blocks of usb device */	
	numblk = usb_dev_desc[device].lba;  /* 0 to usb_dev_desc[]->lba - 1 */
	buffer = (char*) malloc (usb_dev_desc[device].blksz * blkcnt);
	memset(buffer,'1', usb_dev_desc[device].blksz * blkcnt); 

	recheck_buffer = (char*)malloc(usb_dev_desc[device].blksz * blkcnt);
	memset(recheck_buffer, '1', usb_dev_desc[device].blksz * blkcnt);
	
	i = 0;			
	
	read_cnt = stor_dev->block_read(usb_stor_curr_dev, i, blkcnt, (ulong *)buffer);
			
	if (!read_cnt) {
	    goto done;
	}

	udelay(1000);

	i = 0;
	blkcnt = NUM_TEST_BLOCKS;
	
	/* re-read the same sectors again */
	recheck_read_cnt = stor_dev->block_read(usb_stor_curr_dev, i, blkcnt, (ulong *)recheck_buffer);
	
	
	/*
	 * read failed or the two reads are not of same size, test
	 * failed. 
	 */
	if (!recheck_read_cnt || (recheck_read_cnt != read_cnt)) {
	    goto done;
	}

	/* compare the two buffer if we read same stuff */
	if (memcmp(buffer, recheck_buffer, read_cnt)) {
	    goto done;
	}
    }
    
    usb_pass_flag = 1;
	
done:
    free(buffer);
    free(recheck_buffer);

    if (usb_pass_flag) {
	SET_USB_POST_RESULT(gd->post_result, POST_PASSED);
	return 0;
    } else {
	post_log("USB Test: Failed\n");
	SET_USB_POST_RESULT(gd->post_result, POST_FAILED);
	return -1;
    }
}
#endif /* CONFIG_POST & CFG_POST_USB */
#endif /* CONFIG_POST */

