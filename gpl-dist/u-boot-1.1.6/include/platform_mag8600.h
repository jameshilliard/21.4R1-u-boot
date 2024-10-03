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


#ifndef __PLATFORM_MAG8600__H__
#define __PLATFORM_MAG8600__H__

/* Max retries */
#define NORMAL_MODE_BOOTSEQ_RETRIES  12
#define BOOTSEQ_MAX_RETRIES	    (NORMAL_MODE_BOOTSEQ_RETRIES + 4)

#define MAG8600_MAX_BOOTDEVS	    3

#define DEFAULT_SECTOR_SIZE	    512
#define NUM_DISK_PART		    4
#define DISK_PART_SIZE		    0x10
#define DISK_PART_TBL_OFFSET	    0x1be
#define DISK_PART_MAGIC_OFFSET	    0x1fe
#define DISK_PART_MAGIC		    0xAA55
#define BSD_MAGIC		    0xa5
#define ACTIVE_FLAG		    0x80

typedef struct disk_part {
    uint8_t	boot_flag;	    /* active				*/
    uint8_t	head;		    /* start head			*/
    uint8_t	sector;		    /* start sector			*/
    uint8_t	cylinder;	    /* start cylinder			*/
    uint8_t	part_type;	    /* type of partition.		*/
    uint8_t	end_head;	    /* end of head			*/
    uint8_t	end_sector;	    /* end of sector			*/
    uint8_t	end_cylinder;	    /* end of cylinder			*/
    uint8_t	start_sec[4];	    /* start sector from 0		*/
    uint8_t	size_sec[4];	    /* number of sectors in partition	*/
} disk_part_t;

/* Mask for different bootable devices */
#define DISK_ZERO    0x0
#define DISK_FIRST   0x1
#define DISK_SECOND  0x2
#define DISK_THIRD   0x3
#define DISK_DEFAULT_MAG8600 DISK_FIRST
#define DISK_MAX_MAG8600   DISK_THIRD
#define DISK_USB0_MAG8600    DISK_ZERO
#define IS_DISK_USB_MAG8600(dev) ((dev) == DISK_USB0_MAG8600 

#define DISK_ZERO_STR	 "disk0"
#define DISK_FIRST_STR	 "disk1"
#define DISK_SECOND_STR  "disk2"
#define DISK_DEFAULT_MAG8600_STR DISK_FIRST_STR

#define BOOT_SUCCESS	   0x1
#define MAX_DISK_STR_LEN   16

#define BOOTSEQ_MAGIC_LEN  4
#define BOOTSEQ_MAGIC_STR  "BTSQ"

#define READ_BOOTSEQ_RECORD   0x01
#define WRITE_BOOTSEQ_RECORD  0x02

#define REBOOT_REASON_LEN     2
#define MAX_DUMMY_LEN	      1
#define SELECT_ACTIVE_SLICE		0
#define BOOT_MODE_NORMAL		0
#define BOOT_MODE_RECOVERY		1
#define DEFAULT_RECOVERY_SLICE		4
#define DEFAULT_SLICE_TO_TRY		1
#define CURR_BOOT_REC_VERSION		0x00
#define MIN_BOOT_REC_VER_SUPPORTED	0x00
#define VALID_BOOT_REC_VERSION(x)	((uint8_t)x >= MIN_BOOT_REC_VER_SUPPORTED    \
					 && (uint8_t)x != 0x0F ? 1 : 0)

typedef union slice_info {
    uint8_t sl_info;
    struct {
	uint8_t version:4;	/* boot record version */
	uint8_t boot_mode:1;	/* boot mode, recovery or normal */
	uint8_t next_slice:3;	/* next slice to be tried for booting */
    }sl_fields;
} slice_info_t;

/*
 * mag_bootseqinfo is used by bootsequening
 */
typedef struct mag_bootseqinfo {
    uint8_t sb_magic[BOOTSEQ_MAGIC_LEN];
    uint8_t sb_reset_reason[REBOOT_REASON_LEN];
    uint8_t sb_retry_count;
    uint8_t sb_next_bootdev;
    uint8_t sb_boot_success;
    uint8_t sb_cur_bootdev;
    slice_info_t sb_slice;
    uint8_t sb_dummy[MAX_DUMMY_LEN];
} mag_bootseqinfo_t;

#define SD_DEVICE_ENABLED	(0x01UL << 0)

/*
 * srxsme_device is used for mapping device to corrosponding "disk" names
 */
typedef struct mag_device {
    char  *sd_devname;
    char  *sd_diskname;
    uint32_t sd_flags;
} mag_device_t;

extern uint8_t get_next_bootdev(void);

extern uint32_t get_uboot_version (void);
#endif
