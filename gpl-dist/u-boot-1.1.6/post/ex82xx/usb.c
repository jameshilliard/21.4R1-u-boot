/*
 * Copyright (c) 2010, Juniper Networks, Inc.
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

#ifdef CONFIG_POST
#include <config.h>
#include <post.h>

#if CONFIG_POST & CFG_POST_USB
#include <usb.h>
#include <scsi.h>
#include <malloc.h>

extern block_dev_desc_t usb_dev_desc[USB_MAX_STOR_DEV];
DECLARE_GLOBAL_DATA_PTR;

#define USB_MAX_WRITE_BLK   20
#define USB_MAX_STOR_DEV    5
#define USB_STOR_TRANSPORT_GOOD     0
#define USB_STOR_TRANSPORT_FAILED   -1
#define USB_STOR_TRANSPORT_ERROR    -2

typedef int (*trans_cmnd)(ccb *, struct usb_data *);
typedef int (*trans_reset)(struct usb_data *);
struct usb_data {
	struct usb_device *pusb_dev;        /* this usb_device */
	unsigned int      flags;            /* from filter initially */
	unsigned char     ifnum;            /* interface number */
	unsigned char     ep_in;            /* in endpoint */
	unsigned char     ep_out;           /* out ....... */
	unsigned char     ep_int;           /* interrupt . */
	unsigned char     subclass;         /* as in overview */
	unsigned char     protocol;         /* .............. */
	unsigned char     attention_done;   /* force attn on first cmd */
	unsigned short    ip_data;          /* interrupt data */
	int               action;           /* what to do */
	int               ip_wanted;        /* needed */
	int               *irq_handle;      /* for USB int requests */
	unsigned          int irqpipe;      /* pipe for release_irq */
	unsigned char     irqmaxp;          /* max packed for irq Pipe */
	unsigned char     irqinterval;      /* Intervall for IRQ Pipe */
	ccb               *srb;             /* current srb */
	trans_reset       transport_reset;  /* reset routine */
	trans_cmnd        transport;        /* transport routine */
};

static unsigned long usb_stor_write(unsigned long device, unsigned long blknr,
							 unsigned long blkcnt, unsigned long *buffer);
static int usb_write_10(ccb *srb, struct usb_data *ss, unsigned long start,
						unsigned short blocks);
void usb_show_progress(void);

struct usb_device *usb_get_dev_index(int index);
static int usb_stor_curr_dev; /* Current device */
static ccb usb_ccb;

static int usb_request_sense(ccb *srb, struct usb_data *ss)
{
	char *ptr;

	ptr = (char *)srb->pdata;
	memset(&srb->cmd[0], 0, 12);
	srb->cmd[0] = SCSI_REQ_SENSE;
	srb->cmd[1] = srb->lun << 5;
	srb->cmd[4] = 18;
	srb->datalen = 18;
	srb->pdata = &srb->sense_buf[0];
	srb->cmdlen = 12;
	ss->transport(srb, ss);
	post_log("Request Sense returned %02X %02X %02X\n",
			 srb->sense_buf[2], srb->sense_buf[12], srb->sense_buf[13]);
	srb->pdata = (uchar *)ptr;
	return (0);
}


static int usb_test_unit_ready(ccb *srb, struct usb_data *ss)
{
	int retries = 10;

	do {
		memset(&srb->cmd[0], 0, 12);
		srb->cmd[0] = SCSI_TST_U_RDY;
		srb->cmd[1] = srb->lun << 5;
		srb->datalen = 0;
		srb->cmdlen = 12;
		if (ss->transport(srb, ss) == USB_STOR_TRANSPORT_GOOD) {
			return (0);
		}
		usb_request_sense(srb, ss);
		wait_ms(100);
	} while (retries--);
	return (-1);
}


static int usb_write_10(ccb *srb, struct usb_data *ss, unsigned long start,
						unsigned short blocks)
{
	memset(&srb->cmd[0], 0, 12);
	srb->cmd[0] = SCSI_WRITE10;
	srb->cmd[1] = srb->lun << 5;
	srb->cmd[2] = ((unsigned char)(start >> 24)) & 0xff;
	srb->cmd[3] = ((unsigned char)(start >> 16)) & 0xff;
	srb->cmd[4] = ((unsigned char)(start >> 8)) & 0xff;
	srb->cmd[5] = ((unsigned char)(start)) & 0xff;
	srb->cmd[7] = ((unsigned char)(blocks >> 8)) & 0xff;
	srb->cmd[8] = (unsigned char)blocks & 0xff;
	srb->cmdlen = 12;
	return (ss->transport(srb, ss));
}


static unsigned long usb_stor_write(unsigned long device,
							 unsigned long blknr,
							 unsigned long blkcnt,
							 unsigned long *buffer)
{
	unsigned long start, blks, buf_addr;
	unsigned short smallblks;
	struct usb_device *dev;
	int retry, i;
	ccb *srb = &usb_ccb;

	if (blkcnt == 0) {
		return (0);
	}

	device &= 0xff;
	/* Setup  device
	 */
	dev = NULL;
	for (i = 0; i < USB_MAX_DEVICE; i++) {
		dev = usb_get_dev_index(i);
		if (dev == NULL) {
			return (0);
		}
		if (dev->devnum == usb_dev_desc[device].target) {
			break;
		}
	}

	usb_disable_asynch(1); /* asynch transfer not allowed */
	srb->lun = usb_dev_desc[device].lun;
	buf_addr = (unsigned long)buffer;
	start = blknr;
	blks = blkcnt;
	if (usb_test_unit_ready(srb, (struct usb_data *)dev->privptr)) {
		post_log("Device NOT ready\n   Request Sense returned %02X %02X %02X\n",
				 srb->sense_buf[2], srb->sense_buf[12], srb->sense_buf[13]);
		return (0);
	}
	do {
		retry = 0;
		srb->pdata = (unsigned char *)buf_addr;
		if (blks > USB_MAX_WRITE_BLK) {
			smallblks = USB_MAX_WRITE_BLK;
		} else {
			smallblks = (unsigned short)blks;
		}
retry_it:
		if (smallblks == USB_MAX_WRITE_BLK) {
			usb_show_progress();
		}
		srb->datalen = usb_dev_desc[device].blksz * smallblks;
		srb->pdata = (unsigned char *)buf_addr;
		if (usb_write_10(srb, (struct usb_data *)dev->privptr, start,
						 smallblks)) {
			post_log("Write  ERROR\n");
			usb_request_sense(srb, (struct usb_data *)dev->privptr);
			if (retry--) {
				goto retry_it;
			}
			blkcnt -= blks;
			break;
		}
		start += smallblks;
		blks -= smallblks;
		buf_addr += srb->datalen;
	} while (blks != 0);
	usb_disable_asynch(0); /* asynch transfer allowed */
	return (blkcnt);
}


#define USB_READ_WRITE_ENABLE 0x14603489 /* Just coded to avoid any errors */

static int usb_stor_curr_dev;            /* Current device */
static int usb_enable_rw = 0;            /* flag to enable read write operation*/
extern int usb_max_devs;                 /* number of highest available usb device */

int usb_post_fast_read_test(int flags)
{
	return (usb_post_test(flags));
}


int usb_post_slow_read_test(int flags)
{
	return (usb_post_test(flags | POST_SLOWTEST));
}


int usb_post_fast_read_write_test(int flags)
{
	int ret = 0;

	usb_enable_rw = USB_READ_WRITE_ENABLE;
	ret = usb_post_test(flags);
	usb_enable_rw = 0;

	return (ret);
}


int usb_post_slow_read_write_test(int flags)
{
	int ret = 0;

	usb_enable_rw = USB_READ_WRITE_ENABLE;
	ret = usb_post_test(flags | POST_SLOWTEST);
	usb_enable_rw = 0;

	return (ret);
}


int usb_post_test(int flags)
{
	unsigned long i, j, k = 0;
	unsigned long increment;
	block_dev_desc_t *stor_dev;
	unsigned long device;
	struct usb_device *dev;
	unsigned long numblk = 0;
	const unsigned long blkcnt = 10;
	unsigned long read_cnt;
	char *buffer = 0;
	unsigned long write_cnt;

	if (!EX82XX_RECPU) {
		post_log("USB test not supported on this platform \n");
		return (-1);
	}

	post_log("\nUSB %s Test.........\n",
			 (usb_enable_rw == USB_READ_WRITE_ENABLE) ? "Read/Write" : "Read");

	/* Stop the USB */
	if (usb_stop(0) < 0) {
		post_log("usb_stop failed\n");
		return (-1);
	}

	/* Init the USB */
	if (usb_init() < 0) {
		post_log("usb_init failed\n");
		return (-1);
	}

	/*
	 *   check whether a storage device is attached (assume that it's
	 *   a USB memory stick, since nothing else should be attached).
	 */
	usb_stor_curr_dev = usb_stor_scan(1);

	if (usb_stor_curr_dev == -1) {
		post_log("No device found\n");
		return (-1);
	}

	for (j = 0; j < usb_max_devs; j++) {
		usb_stor_curr_dev = j;
		/* write and read the data to the usb memory stick*/
		stor_dev = usb_stor_get_dev(usb_stor_curr_dev);
		device  = usb_stor_curr_dev;

		device &= 0xff;

		/* Setup  device
		 */
		dev = NULL;
		for (i = 0; i < USB_MAX_DEVICE; i++) {
			dev = usb_get_dev_index(i);
			if (dev == NULL) {
				return (-1);
			}
			if (dev->devnum == usb_dev_desc[device].target) {
				break;
			}
		}


		/* Read number of blocks of usb device */
		numblk = usb_dev_desc[device].lba;


		post_log("USB Device: %d %s %s %s \n",
				 device, dev->mf, dev->prod, dev->serial);

		post_log("Num Blocks: %d Block Size: %d Total Size: %dM\n",
				 numblk, usb_dev_desc[device].blksz, numblk *
				 usb_dev_desc[device].blksz / 1048576);


		buffer = (char *)malloc(usb_dev_desc[device].blksz * blkcnt);
		memset(buffer, '1', usb_dev_desc[device].blksz * blkcnt);

		k = 0;

		if (flags & POST_SLOWTEST) {
			post_log("running [SLOW TEST]\n");
			increment = 100;
		} else {
			post_log("running [FAST TEST]\n");
			increment = 4 * 1024;
		}

		/* Slow test : test all blocks */
		/* Fast test : test 1 out of 4k blocks */

		for (i = 0; i < numblk ; i += increment) {
			if (flags & POST_MANUAL) {
				if (ctrlc()) {
					post_log('\n');
					post_log("\nAborted USB Test......\n");
					return (-1);
				}
			}

			if (usb_enable_rw == USB_READ_WRITE_ENABLE) {
				write_cnt = usb_stor_write(usb_stor_curr_dev,
										   i, blkcnt, (ulong *)buffer);
				if (!write_cnt) {
					post_log("Write Failed at block 0x%x\n", i);
					goto done;
				}
			}

			k++;
			if (0 == k % 10) {
				post_log(".");
			}

			if (0 == k % 800) {
				post_log("\n");
			}

			udelay(5000);

			read_cnt = stor_dev->block_read(usb_stor_curr_dev,
											i, blkcnt, (ulong *)buffer);
			if (!read_cnt) {
				post_log("Read Failed at block 0x%x\n", i);
				goto done;
			}

			if (usb_enable_rw == USB_READ_WRITE_ENABLE) {
				if (read_cnt != write_cnt) {
					post_log("Read and write mismatch at block %d\n", i);
					goto done;
				}
			}
		}

		post_log("\n");
	}

	free(buffer);
	return (0);

done:
	free(buffer);
	return (-1);
}


#endif /* CONFIG_POST & CFG_POST_USB */
#endif /* CONFIG_POST */
