/*
 * $Id: platform_srxsme.c,v 1.5.78.2 2009-09-24 13:07:15 jyan Exp $
 *
 * platform_srxsme.c: Platform releated bootsequencing
 *		      Implementation for srxsme platforms.
 *
 * Copyright (c) 2009-2015, 2020, Juniper Networks, Inc.
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
 * along with this program. If not, see
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>
 */

#include <common.h>
#include <watchdog_cpu.h>
#include <cvmx.h>
#include <lib_octeon_shared.h>
#include <octeon_boot.h>
#include <platform_srxsme.h>
#include <exports.h>

#ifdef CONFIG_API
#include <usb.h>
#endif

#define BOOTSEQ_DEBUG	0

#if BOOTSEQ_DEBUG
#define BSDEBUG(fmt, args...) printf(fmt, ## args)
#else
#define BSDEBUG(fmt, args...)
#endif

uint32_t num_usb_disks(void);
void enable_ciu_watchdog(int);
void disable_ciu_watchdog(void);
void pat_ciu_watchdog(void);
void srxsme_show_bootdev_list(void);
void reload_watchdog(int);
void srxsme_bootseq_init(void);

/* static declerations */
static unsigned int num_boot_disks(void);

static unsigned char srxsme_bootdev[MAX_DISK_STR_LEN];
static srxsme_device_t *srxsme_map = NULL;
static uint8_t	*platform_name;
static uint8_t  *platform_default_bootlist = NULL;
static uint32_t  num_bootable_media;
static uint32_t bootseq_sector_start_addr;
static uint32_t bootseq_sector_len;

/* Default device is the first device in the following list */
#define SRXLE_NF_MODELS_DEF_BOOT_LIST   "nand-flash:usb"
#define SRXLE_CF_MODELS_DEF_BOOT_LIST   "internal-compact-flash:usb"
#define SRXMR_DEF_BOOT_LIST   "internal-compact-flash:usb:external-compact-flash"

/*
 * These are the boot device maps for the various SRXSME platforms
 * If any new devices are added in the future, please ensure that
 * srxsme_is_next_bootdev_present() is updated to handle that device
 */
static srxsme_device_t srxle_nf_models_dev_map[] = {
	{
	    "nand-flash",	    /* device name (user visible) */
	    DISK_ZERO_STR,	    /* device string */
	    SD_DEVICE_ENABLED	    /* device flags */
	},
	{
	    "usb",
	    DISK_FIRST_STR,
	    SD_DEVICE_ENABLED
	}
};

static srxsme_device_t srxle_cf_models_dev_map[] = {
	{
	    "usb",	 
	    DISK_ZERO_STR,
	    SD_DEVICE_ENABLED
	},
	{
	    "internal-compact-flash",
	    DISK_FIRST_STR,
	    SD_DEVICE_ENABLED
	}
};

static srxsme_device_t srxmr_dev_map[] = {
	{
	    "usb",	 
	    DISK_ZERO_STR,
	    SD_DEVICE_ENABLED
	},
	{
	    "internal-compact-flash",
	    DISK_FIRST_STR,
	    SD_DEVICE_ENABLED
	},
	{
	    "external-compact-flash",
	    DISK_SECOND_STR,
	    SD_DEVICE_ENABLED
	}
};

extern uint8_t boot_sector_flag;

typedef int (* disk_read_vector) (int dev, int lba, int totcnt, char *buf);


#define NBASE_DECIMAL			10
#define NBASE_HEX			16

#define RECOVERY_SLICE_ENV		"recovery_slice"
#define BOOTSEQ_DISABLE_ENV		"boot.btsq.disable"

#define TRUE				1
#define FALSE				0

#define MBR_BUF_SIZE			512

#define MAX_NUM_SLICES			4
#define FIRST_SLICE			1
#define IS_NUMERAL(num) 		(num >= 0 && num <= 9 ? TRUE : FALSE)
#define IS_VALID_SLICE(slice)		(slice >= FIRST_SLICE &&	      \
					 slice <= MAX_NUM_SLICES	      \
					 ? TRUE : FALSE)

#define IS_BOOT_SEQ_DISABLED()	(getenv(BOOTSEQ_DISABLE_ENV) != NULL &&	      \
				 simple_strtoul(getenv(BOOTSEQ_DISABLE_ENV),  \
						NULL, NBASE_DECIMAL) != 0     \
				 ? TRUE : FALSE)

#define LIST_MAX_LEN            256

/**
 * @brief
 * This function returns the recovery slice as specified by
 * environment variable.
 */
static inline int
get_recovery_slice (void)
{
    int aslice;
	    
    aslice = simple_strtoul(getenv(RECOVERY_SLICE_ENV), NULL, NBASE_DECIMAL);

    if (!IS_VALID_SLICE(aslice)) {
	BSDEBUG("Invalid recovery slice %d\n", aslice);
	aslice = DEFAULT_RECOVERY_SLICE;
    }

    return aslice;
}

/**
 * @brief
 * Little endian 16-bit shuffling.
 *
 * @param[in] pp
 * Pointer to 16-bit value.
 */
static uint16_t
le16dec(const void *pp)
{
    unsigned char const *p = (unsigned char const *)pp;

    return ((p[1] << 8) | p[0]);
}

/**
 * @brief
 * Check if the buffer given is a Master Boot Record.
 *
 * @param[in] sector_buf
 * Pointer to the 512-bytes buffer containing data
 * read out from master boot record of a device.
 */
static int
ismbr (uint8_t *sector_buf)
{
    uint8_t *p;
    uint32_t index, sum;
    uint16_t magic;

    magic = le16dec(sector_buf + DISK_PART_MAGIC_OFFSET);

    /* check if 0xAA, 0x55 are present */
    if (magic != DISK_PART_MAGIC) {
	return 0;
    }

    /*
     * What follows is to avoid false positives when presented
     * with a FAT12, FAT16 or FAT32 file system. The boot sector
     * has the same magic number as a MBR.
     */
    for (index = 0; index < NUM_DISK_PART; index++) {
	p = sector_buf + DISK_PART_TBL_OFFSET + index * DISK_PART_SIZE;
	if (p[0] != 0 && p[0] != ACTIVE_FLAG) {
	    return (0);
	}
    }

    sum = 0;
    for (index = 0; index < NUM_DISK_PART * DISK_PART_SIZE; index++) {
	sum += sector_buf[DISK_PART_TBL_OFFSET + index];
	if (sum != 0) {
	    return (1);
	}
    }

    /*
     * At this point we could have an empty MBR. We treat that
     * as not having a MBR at all. Without slices, there cannot
     * be any partitions and/or file systems to open anyway.
     * The FAT file system code will correctly reject this as
     * a boot sector.
     */
    return (0);
}

/**
 * @brief
 * Read master boot record from given device
 *
 * @param[in] dev
 * Device index in u-boot device list.
 *
 * @param[out] buf
 * A 512-byte buffer in which MBR should be copied.
 *
 * @warning
 * buf should be at least 512-byte long.
 */
static int
read_device_mbr (int dev, uint8_t *buf)
{
    DECLARE_GLOBAL_DATA_PTR;
    disk_read_vector rvec;

    /*
     * Get the disk read vector from Jump table.
     */
    rvec = gd->jt[XF_disk_read];

    if (!rvec) {
	BSDEBUG("Error: Jump table not initialized!\n");
	return -1;
    }

    /* Read the MBR. */
    if (rvec(dev, 0, 1, buf) != 0) {
	BSDEBUG("Error: Cannot read MBR on device %d\n", dev);
	return -1;
    }

    if (!ismbr(buf)) {
	BSDEBUG("Error: Invalid entries in MBR in device %d\n", dev);
	return -1;
    }

    return 0;
}

/**
 * @brief
 * Get the active slice for given device.
 *
 * @param[in] dev
 * Index into the device list.
 */
int
get_active_slice (int dev)
{
    uint8_t buf[MBR_BUF_SIZE];
    disk_part_t *dp;
    int idx = 0, found = 0;

    if (read_device_mbr(dev, buf) == 0) {
	dp = (disk_part_t *)(buf + DISK_PART_TBL_OFFSET);

	/* for each entry in MBR find slice marked active */
	for (idx = 0; idx < MAX_NUM_SLICES; idx++, dp++) {

	    /* check if the slice is active */
	    if (dp->boot_flag == ACTIVE_FLAG) {
		found = TRUE;
		break;
	    }
	}
    }

    return (found ? (idx + 1) : -1);
}

/**
 * @brief
 * Checks if given slice number in the given device
 * is marked as a BSD slice in MBR.
 *
 * @warning
 * Slice number are expected from 1 to 4.
 *
 * @param[in] dev
 * Index into the device list.
 *
 * @param[in] slice
 * Slice number to check.
 */
int
is_bsd_slice (int dev, int slice)
{
    uint8_t buf[MBR_BUF_SIZE];
    disk_part_t dp;

    if (!IS_VALID_SLICE(slice)) {
	return FALSE;
    }

    /*
     * Slice number are from 1 to 4,
     * but indexing on disk goes 0 to 3.
     * Adjust slice value for indexing.
     */
    slice--;

    if (read_device_mbr(dev, buf) == 0) {
	memcpy(&dp, buf + DISK_PART_TBL_OFFSET + slice *
	       DISK_PART_SIZE, sizeof(dp));

	/* if slice type is BSD */
	if (dp.part_type == BSD_MAGIC) {
	    return TRUE;
	}
    }

    return FALSE;
}

/**
 * @brief
 * Checks the number of BSD slices in given device.
 *
 * @param[in] dev
 * Index into device list.
 */
int
get_num_bsd_slices (int dev)
{
    uint8_t buf[MBR_BUF_SIZE];
    disk_part_t *dp;
    int bsd_slices = 0, counter;

    if (read_device_mbr(dev, buf) == 0) {
	dp = (disk_part_t *)(buf + DISK_PART_TBL_OFFSET);

	for (counter = 0; counter < MAX_NUM_SLICES; counter++, dp++) {
	    /* if slice type is BSD, increment the counter. */
	    if (dp->part_type == BSD_MAGIC) {
		bsd_slices++;
	    }
	}
    }

    return bsd_slices;
}

/**
 * @brief
 * This function is the heart of slicing logic, i.e.
 * next slice to be tried on a given device.
 * <p>
 * Input to this function is device index and the current
 * slice that is being tried. Based on the current slice
 * that is being tried it decides on next slice to be tried.
 * Here is the function's behaviour in differnt scenarios.
 * <p>
 * <ul>
 *	<li> If current slice is invalid, return "reset" slice
 *	     as next slice.
 *	<li> Increment the current slice and take it as next slice.
 *	     then do following:
 *	     <ul>
 *		<li> check if its active slice, return "reset as next
 *		     slice, because if next slice to be tried is same as
 *		     active slice, it means that all slices have been tried.
 *		<li> If its next slice to be tried is not active, check if
 *		     its recovery slice. If its recovery slice, skip this
 *		     slice and try next.
 *		<li> otherwise return this slice as next slice.
 *	    </ul>
 * </ul>
 *
 * @param[in] curr_slice
 * Current slice is being tried and based on which next slice
 * should be calculated.
 */
int
get_next_slice_for_device (int dev, int curr_slice)
{
    int next_slice = SELECT_ACTIVE_SLICE, active_slice, recovery_slice;
    int done = 0;

    /* in case of invalid slice reset the next slice */
    if (!IS_VALID_SLICE(curr_slice)) {
	goto cleanup;
    } else {
	/*
	 * Get the active slice for this device.
	 * We will use this to check if we have tried all slices.
	 */
	active_slice = get_active_slice(dev);

	/*
	 * Something is seriously wrong if finding active slice
	 * failed. Better go by defaults.
	 */
	if (active_slice < 0) {
	    BSDEBUG("Invalid active slice for device %d\n", dev);
	    goto cleanup;
	}

	recovery_slice = get_recovery_slice();

	/*
	 * Using current slice as starting index, we will
	 * move to next slice by incrementing it by one.
	 */
	next_slice = curr_slice + 1;
	next_slice = (next_slice <= MAX_NUM_SLICES ? next_slice : FIRST_SLICE);

	/*
	 * Starting from the slice will circulate the in the
	 * slice list until we find a valid next slice for
	 * given slice.
	 */
	do {
	    BSDEBUG("Next candidate slice: %d\n", next_slice);

	    /*
	     * If next slice that we calculated is same as active.
	     * we have tried all slices. So reset the slice.
	     */
	    if (next_slice == active_slice) {
		BSDEBUG("%d slice is same as active. Resetting slice.\n");
		next_slice = SELECT_ACTIVE_SLICE;
		break;
	    }

	    /*
	     * Keep trying the next slice until we reach
	     * a valid BSD slice.
	     */
	    if (!is_bsd_slice(dev, next_slice)) {
		BSDEBUG("Invalid next slice %d\n", next_slice);
		next_slice++;
		next_slice = (next_slice <= MAX_NUM_SLICES ? next_slice : FIRST_SLICE);
		continue;
	    }

	    /*
	     * Only if number of bsd slices are greater than one, we skip
	     * recovery slice. This will prevent user to set recover_slice
	     * to 1 on JUNOS images which support only one
	     * partition.
	     */
	    if (get_num_bsd_slices(dev) > 1 &&
		next_slice == recovery_slice) {
		BSDEBUG("%d slice is recovery slice. Skipping.\n", next_slice);
		next_slice++;
		next_slice = (next_slice <= MAX_NUM_SLICES ? next_slice : FIRST_SLICE);
		continue;
	    }

	    done = 1;
	} while (!done);
    }

cleanup:
    return next_slice;
}

/**
 * @brief
 * Get the device index from the devices defined
 * in the list. For example, 0 from disk0:, 1
 * from disk1: etc.
 *
 * @param[in] dev_name
 * Device name string from which ID is to be extracted.
 */
int
get_devid_from_device (const uint8_t *dev_name)
{
    int dev_unit = -1;

    if (dev_name == NULL) {
	dev_unit = -1;
	goto cleanup;
    }

    int len = strlen(dev_name);

    if (!len) {
	dev_unit = -1;
	goto cleanup;
    }

    /*
     * Devices are named disk0, disk1. Where
     * numbers appended to their name are unit or device
     * id. We can use these appended number to call
     * disk read vector. Take the number out from name.
     */
    if (dev_name[len - 1] == ':') {
	dev_unit = dev_name[len - 2] - '0';
    } else {
	dev_unit = dev_name[len - 1] - '0';
    }

    /*
     * check if its a valid number.
     */
    if (IS_NUMERAL(dev_unit))
	goto cleanup;
    else {
	dev_unit = -1;
    }

cleanup:
    return dev_unit;
}

/*
 * handle_booseq_record handles the record management on flash
 * it gives the last valid record when bootseq_command is
 * READ_BOOTSEQ_RECORD and writes the record to proper offset
 * in flash when bootseq_command is WRITE_BOOTSEQ_RECORD
 */

int32_t
handle_booseq_record (srxsme_bootseqinfo_t *bootinfo, uint32_t magiclen,
		      uint8_t *magic, uint32_t bootseq_command) {

    uint8_t		  bootseq_sector_buffer[64 * 1024];
    uint8_t		 *temp;
    uint32_t		  count;
    uint32_t		  num_bootseq_rec;
    uint32_t		  rec_count;
    uint32_t		  bootseq_sector_end_addr;
    uint32_t		  need_erase = 0;
    srxsme_bootseqinfo_t *bootrecord;


    if ((bootinfo == NULL) || (magic == NULL)) {
	printf("bootinfo or magic is null\n");
	return -1;
    }

    /* copy contents of bootseq sector from flash to temporary buffer */
    memcpy (bootseq_sector_buffer, (void *)bootseq_sector_start_addr,
					  bootseq_sector_len);

    /* cast temp buffer to pointer boot_sequence_t */
    bootrecord = (srxsme_bootseqinfo_t *)bootseq_sector_buffer;

    /* maximum number of records that could fit in the sector */
    num_bootseq_rec = bootseq_sector_len / sizeof(srxsme_bootseqinfo_t);

    /*
     * loop through temp buffer by using num_bootseq_rec
     * to find the last valid record by using magic signature
     */
    for (rec_count = 0; rec_count < num_bootseq_rec; rec_count++) {
	if (memcmp(bootrecord->sb_magic, magic, magiclen) != 0)
	    break;
	bootrecord++;
    }

    if (bootseq_command == READ_BOOTSEQ_RECORD) {
	/*
	 * if there is no valid record on in the flash
	 * [could happen for first time], bootrecord will be
	 * pointing to begininig of buffer else adjust bootrecord
	 * to point to previous record before reading record.
	 */
	if (rec_count != 0)
	    bootrecord--;

	/*
	 * check the magic of previous record, as we dont know, did
	 * we find magic or loop terminated
	 * if magic is found copy the record else return 0xff in all feilds.
	 */
	if (memcmp(bootrecord->sb_magic, magic, magiclen) == 0)
	    memcpy(bootinfo, (void *)bootrecord, sizeof(srxsme_bootseqinfo_t));
	else
	    memset(bootinfo, 0xff, sizeof(srxsme_bootseqinfo_t));

       return 0;
    } else if (bootseq_command == WRITE_BOOTSEQ_RECORD) {

	/* compute the flash sector end address */
	bootseq_sector_end_addr =  bootseq_sector_start_addr +
					      bootseq_sector_len - 1;

	/* check where we are trying to write next record and verify
	 * all the bytes are 0xff before writing, if any bytes [size of
	 * record] has got value other than 0xff will cause flash writes
	 * to fail. This could happen only when this sector has been
	 * over written with data unrelated to bootsequencing
	 *
	 * if we find that sector has data inconsistency/unrelated
	 * to bootseq, will request force erase by setting need_erase
	 * of the sector and create new record in begining
	 * of the sector
	 *
	 */
	if (rec_count < num_bootseq_rec) {
	    /*
	     * bootrecord will be pointing to the offset where new
	     * record will be written
	     */
	    temp = (uint8_t *)bootrecord;

	    /*
	     * loop through all bytes to find if all bytes has 0xff
	     */
	    for (count = 0; count < sizeof(srxsme_bootseqinfo_t);
							count++) {
		if (*temp != 0xff) {
		    need_erase = 1;
		    printf("Invalid data found during creation of"
			   " new record\n");
		    break;
		}
		temp++;
	    }
	} else {
	    /*
	     * setting need_erase to one here to indicate sector is
	     * full and want to erase all records and start writing
	     * new record from begining of sector
	     */
	    need_erase = 1;
	}
	/*
	 * unprotect the sector for writing
	 */
	if (flash_sect_protect(0, bootseq_sector_start_addr,
			       bootseq_sector_end_addr) != 0) {
	    printf("Boot Sequening sector protect off failed\n");
	    return -1;
	}

	/*
	 * check whether sector is full by all valid record,so
	 * have to erase and start from begining or have found
	 * and inconsistency is any record in which a force erase
	 * is requested.
	 */
	if (need_erase) {
	    memset(bootseq_sector_buffer, 0xff,  bootseq_sector_len);

	    bootrecord = (srxsme_bootseqinfo_t *)bootseq_sector_buffer;

	    /* erase flash sector */
	    if (flash_sect_erase(bootseq_sector_start_addr,
				 bootseq_sector_end_addr) !=0 ) {
		printf("Boot Sequening sector erase failed\n");
		return -1;
	    }
	}

	/* add magic signature to the record */
	memcpy(bootinfo->sb_magic, magic, magiclen);

	/* set the current boot record version */
	bootinfo->sb_slice.sl_fields.version = CURR_BOOT_REC_VERSION;

	/*
	 * compute the offset where new record can be
	 * written from the flash sector beginning.
	 */
	count =  (uint8_t *)bootrecord - bootseq_sector_buffer;

	/*
	 * write only the new record
	 */
	if (flash_write((uint8_t *)bootinfo, bootseq_sector_start_addr + count,
			sizeof(srxsme_bootseqinfo_t)) != 0) {
	    printf("Boot Sequening write failed\n");
	    return -1;
	}
	/*
	 * protect the sector
	 */
	if (flash_sect_protect(1, bootseq_sector_start_addr,
			       bootseq_sector_end_addr) != 0) {
	    printf("Boot Sequening sector protect on failed\n");
	    return -1;
	}

	return 0;
    } else {
	printf("unknown bootsequencing command\n");
	return -1;
    }
    return 0;
}

int
srxmr_select_and_enable_cf (int disk_num)
{
    int err = 0;

    switch (disk_num) {
    case DISK_SECOND:
	/* if its extenal disk requst, enable it. */
	if (octeon_gpio_value(SRX650_GPIO_CF_CD1_L) == 0) {
	    enable_external_cf();
	}
	ide_init();
	break;

    case DISK_FIRST:
	/* for internal, deselect external and do ide init again. */
	if (octeon_gpio_value(SRX650_GPIO_CF_CD1_L) == 0) {
	    disable_external_cf();
	}
	ide_init();
	if ((cf_disk_scan(0) <= 0) &&
	    (octeon_gpio_value(SRX650_GPIO_CF_CD1_L) == 0)) {
	    err = -1;
	}
	break;
    }

    return err;
}

/*
 * srxsme_is_next_bootdev_present  determinses
 * is selected nextbootdev in the list is present or
 * not
 */
static int
srxsme_is_next_bootdev_present (unsigned int nextbootdev)
{
    DECLARE_GLOBAL_DATA_PTR;

    switch (nextbootdev) {
    case DISK_ZERO:
        /*
         * On SRXLE NAND flash based platforms the internal nand flash is always
         * assumed to be present
         * On SRXLE Compact flash based platforms and SRXMR platforms, the USB
         * (disk0) may or may not be present
         */
        switch (gd->board_desc.board_type)  {
        CASE_ALL_SRXLE_NF_MODELS
            return 1;
        CASE_ALL_SRXLE_CF_MODELS
        CASE_ALL_SRXMR_MODELS
            return (num_usb_disks() >= 1) ? 1 : 0;
        default:
            /* Return 0 if the platform is not one of the above */
            return 0;
        }
	break;

    case DISK_FIRST:
        switch (gd->board_desc.board_type)  {
        CASE_ALL_SRXLE_NF_MODELS
            /* SRXLE NAND flash platforms: Disk-1 is external USB */
            return (num_usb_disks() > 1) ? 1 : 0;
        CASE_ALL_SRXLE_CF_MODELS
            /* SRXLE Compact flash platforms: Disk-1 is hot pluggable CF */
            return (srxle_cf_present());
        CASE_ALL_SRXMR_MODELS
            /* On Thor, find out whether primary compact flash is present or
             * not. If present, adjust boot bus maps to access it and
             * return 1 else return 0
             */
            return ((cf_disk_scan(0) > 0) &&
                    ((octeon_gpio_value(SRX650_GPIO_CF_CD1_L) != 0) ||
                    ((octeon_gpio_value(SRX650_GPIO_CF_CD1_L) == 0) &&
                    (octeon_gpio_value(SRX650_GPIO_EXT_CF_PWR) == 0))))
                    ? 1 : 0;
        default:
            /* Return 0 if the platform is not one of the above */
            return 0;
        }
	break;

    case DISK_SECOND:
        switch (gd->board_desc.board_type)  {
        CASE_ALL_SRXMR_MODELS
            /*
             * On Thor find out whether secondary compact flash
             * is present or not. If presert adjust boot bus
             * maps to access it and return 1 else return 0
             */
            return (octeon_gpio_value(SRX650_GPIO_CF_CD1_L) == 0) ? 1 : 0;
        default:
            /* Return 0 if the platform is not one of the above */
            return 0;
        }
	break;

    default:
        /* Return 0 for any other disks. If any new disks are added in the
         * srxsme_map, A case should be added here to return 1 when the disk
         * is present on the right platform, or else 0 should be returned.
         */
	return 0;
    }
}

static void
disable_required_media (srxsme_device_t *devlist, uint32_t numdevices)
{
    int bcount = 0;
    char *bootlist = NULL;

    if ((bootlist = getenv(SRXSME_BOOTDEVLIST_ENV)) != NULL) {
        for (bcount = 0; bcount < numdevices; bcount++) {
            BSDEBUG("Checking if media %s should be disabled... ",
                    devlist[bcount].sd_devname);
            if (strstr(bootlist, 
                       (const char *)devlist[bcount].sd_devname) != NULL) {
                BSDEBUG("NO\n"); 
            } else {
                BSDEBUG("YES\n");
                /* disable the device */
                devlist[bcount].sd_flags &= ~SD_DEVICE_ENABLED;
            }
        }
    }
}

static void
print_enabled_media (void)
{
    int bcount = 0;
    char *bootlist = NULL;

    printf("Boot Media: ");

    for (bcount = 0; bcount < num_bootable_media; bcount++) {
        /* disable the device */
        if (srxsme_map[bcount].sd_flags & SD_DEVICE_ENABLED) {
            printf("%s ", srxsme_map[bcount].sd_devname);
        }
    }
    printf("\n");
}

void
get_bootmedia_list (char *devlist)
{
    int bcount = 0;

    for (bcount = 0; bcount < num_bootable_media; bcount++) {
        if (bcount)
            devlist += sprintf(devlist, ", ");

        devlist += sprintf(devlist, "%s", srxsme_map[bcount].sd_devname);
    }
}

int
srxsme_is_valid_devname (const char *devname)
{
    int bcount, found = 0;

    for (bcount = 0; bcount < num_bootable_media; bcount++) {
        if (!strcmp((const char *)srxsme_map[bcount].sd_devname, devname)) {
            found = 1;
            break;
        }
    }

    return (found);
}

int
srxsme_is_device_enabled (int dev_idx)
{
    if (dev_idx < 0 || dev_idx >= num_bootable_media) {
	return 0;
    }

    return (srxsme_map[dev_idx].sd_flags & SD_DEVICE_ENABLED);
}

int
srxsme_is_valid_devlist (const char *devlist)
{
    char *tok;
    char list[LIST_MAX_LEN];

    memcpy(list, devlist, LIST_MAX_LEN);
    tok = strtok(list, ",:");

    while (tok != NULL) {
        if (!srxsme_is_valid_devname(tok))
            return 0;

        tok = strtok (NULL, ",:");
    }

    return (1);
}

static void
srxsme_validate_bootlist (void)
{
    /*
     * Check if valid boot list is defined in environments. If no,
     * go by defaults. This is because we will not try the
     * devices which are absent in this list.
     */
    if ((getenv(SRXSME_BOOTDEVLIST_ENV)) == NULL) {
        setenv(SRXSME_BOOTDEVLIST_ENV, platform_default_bootlist);
        saveenv();
    } else {
        /* 
         * If boot.devlist is not null then check if it has all valid devices.
         * if not then set boot.devlist to default devices.
         */
        if (!srxsme_is_valid_devlist(getenv(SRXSME_BOOTDEVLIST_ENV))) {
            printf("WARNING: Found one or more invalid media in boot list. "
                   "Falling to defaults.\n");
            setenv(SRXSME_BOOTDEVLIST_ENV, platform_default_bootlist);
            saveenv();
        }
    }
}

/*
 * Initialize the bootsequencing logic and the
 * the data structures.
 */
void
srxsme_bootseq_init (void)
{
    DECLARE_GLOBAL_DATA_PTR;

    /* Decide platform name */
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRXLE_MODELS
        platform_name ="srxle";
        break;
    CASE_ALL_SRXMR_MODELS
        platform_name ="srxmr";
        break;
    default:
        printf("Unknown platform type\n");
        return;
    }

    /* Decide boot-sequencing sector start address */
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRXSME_4MB_FLASH_MODELS
        bootseq_sector_start_addr = CFG_FLASH_BASE +
                    simple_strtoul(getenv("boot.btsq.start"), NULL, 16);
        break;
    CASE_ALL_SRXSME_8MB_FLASH_MODELS
        bootseq_sector_start_addr = CFG_8MB_FLASH_BASE +
                    simple_strtoul(getenv("boot.btsq.start"), NULL, 16);
        break;
    }
    bootseq_sector_len = simple_strtoul(getenv("boot.btsq.len"), NULL, 16);

    /* Decide boot devices */
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRXLE_NF_MODELS
        srxsme_map = srxle_nf_models_dev_map;
        platform_default_bootlist = SRXLE_NF_MODELS_DEF_BOOT_LIST;
        num_bootable_media = sizeof(srxle_nf_models_dev_map) / sizeof(srxsme_device_t);
        break;
    CASE_ALL_SRXLE_CF_MODELS
        srxsme_map = srxle_cf_models_dev_map;
        platform_default_bootlist = SRXLE_CF_MODELS_DEF_BOOT_LIST;
        num_bootable_media = sizeof(srxle_cf_models_dev_map) / sizeof(srxsme_device_t);
        break;
    CASE_ALL_SRX650_MODELS
        srxsme_map = srxmr_dev_map;
        platform_default_bootlist = SRXMR_DEF_BOOT_LIST;
        num_bootable_media = sizeof(srxmr_dev_map) / sizeof(srxsme_device_t);
        break;
    }

    srxsme_validate_bootlist();
    disable_required_media(srxsme_map, num_bootable_media);
    print_enabled_media();
}

/*
 * srxsme_show_bootdev_list displays the bootable device
 * for the platform.
 */
void
srxsme_show_bootdev_list (void)
{
   unsigned int i;

   printf("Platform: %s\n", platform_name);
   for (i = 0; i< num_bootable_media; i++) {
       printf("    %s\n", srxsme_map[i].sd_devname);
   }
}

/*
 * srxsme_set_next_bootdev selects the "bootdev"
 * as next boot device
 */
int32_t
srxsme_set_next_bootdev (uint8_t *bootdev){
    uint32_t i = 0;
    srxsme_bootseqinfo_t current_bootinfo;
    srxsme_bootseqinfo_t previous_bootinfo;

    if (IS_BOOT_SEQ_DISABLED()) {
	return 0;
    }

    /*
     * Read bootseq record info from flash
     */
    handle_booseq_record(&previous_bootinfo, BOOTSEQ_MAGIC_LEN,
			 BOOTSEQ_MAGIC_STR, READ_BOOTSEQ_RECORD);

    /*
     * copy entire previous boot record read from flash to the current
     * record that will be written on to flash	with some feilds modified.
     */
    memcpy(&current_bootinfo, &previous_bootinfo,
	   sizeof(srxsme_bootseqinfo_t));

    for (i = 0; i < num_bootable_media; i++) {
	if (strcmp((const char *)bootdev,
		  (const char *)srxsme_map[i].sd_devname) == 0) {
	    current_bootinfo.sb_boot_success = 0;
	    current_bootinfo.sb_retry_count = 0;
	    printf("Setting next boot dev %s\n", bootdev);
	    current_bootinfo.sb_next_bootdev = i;
	    current_bootinfo.sb_cur_bootdev = i;
	    current_bootinfo.sb_slice.sl_fields.next_slice = SELECT_ACTIVE_SLICE;
	    current_bootinfo.sb_slice.sl_fields.boot_mode = BOOT_MODE_NORMAL;
	    handle_booseq_record(&current_bootinfo, BOOTSEQ_MAGIC_LEN,
				 BOOTSEQ_MAGIC_STR, WRITE_BOOTSEQ_RECORD);
	    break;
	}
    }
    if (i == num_bootable_media) {
	printf("Invalid device %s\n", bootdev);
	return -1;
    }
    return 0;
}

/*
 * Returns the first device present in the
 * boot.devlist
 */
static int get_default_dev (const char *bootlist)
{
    char *bootdev = NULL;
    int i = 0;
    char devlist[LIST_MAX_LEN];

    if (NULL == bootlist) {
        printf("Bootlist is NULL\n");
        return -1;
    }
    memcpy(devlist, bootlist, LIST_MAX_LEN);
    bootdev = strtok(devlist, ",:");
    if (!bootdev) {
        printf("Empty SRXSME_BOOTDEVLIST_ENV\n");
        return -1;
    }
    /*
     * reset the retry count, set the next boot dev to
     * first dev in bootdev list environment.
     */
    for (i = 0; i < num_bootable_media; i++) {
        if (strcmp(bootdev,
                  srxsme_map[i].sd_devname) == 0) {
            return i;
        }
    }
    /* Should never get here */
    printf("boot.devlist is invalid\n");
    return -1;
}

/*
 * Reset the boot sequencing information.
 * This is needed when someone changes
 * boot.devlist.
 */
void
srxsme_reset_bootseq (const char *bootlist)
{
    srxsme_bootseqinfo_t binfo;
    int i = 0;

    i = get_default_dev(bootlist); 

    /*
     * reset the retry count, set the next boot dev to
     * first dev in bootdev list environment.
     */
    binfo.sb_next_bootdev = i;
    binfo.sb_cur_bootdev  = i;
    binfo.sb_retry_count  = 0;
    binfo.sb_boot_success = 0;

    /* reset the slice and set boot mode to normal */
    binfo.sb_slice.sl_fields.next_slice = SELECT_ACTIVE_SLICE;
    binfo.sb_slice.sl_fields.boot_mode = BOOT_MODE_NORMAL;

    handle_booseq_record(&binfo, BOOTSEQ_MAGIC_LEN, BOOTSEQ_MAGIC_STR,
			 WRITE_BOOTSEQ_RECORD);
}

/* API to jumptable, for loader use */
int32_t
srxsme_get_next_bootdev_jt (uint8_t *bootdev)
{
    return srxsme_get_next_bootdev(bootdev, 1);
}

/*
 * srxsme_get_next_bootdev srxsme platform specfic implementation
 * of boot sequencing and returns
 */
int32_t
srxsme_get_next_bootdev (uint8_t *bootdev, int32_t update)
{
    unsigned int bootsuccess;
    unsigned int next_bootdev, cur_bootdev, tmp_bootdev;
    unsigned int retrycount = 0;
    unsigned int count = 0;
    int disk_default = 0;
    int slice, curr_slice, device_id, boot_mode;
    uint8_t slice_buffer[8];

    srxsme_bootseqinfo_t previous_bootinfo;
    srxsme_bootseqinfo_t current_bootinfo;
    
    /*
     * If boot sequencing is disabled, don't
     * do anything. currdev will anyways be
     * set to the default device.
     */
    if (IS_BOOT_SEQ_DISABLED()) {
	return 0;
    }

    /*
     * set the default disk to
     * first dev in bootdev list environment.
     */
    disk_default = get_default_dev(getenv(SRXSME_BOOTDEVLIST_ENV));

    /*
     * tickle watchdog
     */
    reload_watchdog(PAT_WATCHDOG);

    /*
     * Read bootseq record info from flash
     */
    handle_booseq_record(&previous_bootinfo, BOOTSEQ_MAGIC_LEN,
			 BOOTSEQ_MAGIC_STR, READ_BOOTSEQ_RECORD);

    /*
     * If boot record version is incompatible, go by defaults
     */
    if (!VALID_BOOT_REC_VERSION(previous_bootinfo.sb_slice.sl_fields.version)) {
	BSDEBUG("Boot record with invalid version %d.\n",
		previous_bootinfo.sb_slice.sl_fields.version);
	previous_bootinfo.sb_slice.sl_fields.next_slice = SELECT_ACTIVE_SLICE;
	previous_bootinfo.sb_slice.sl_fields.boot_mode = BOOT_MODE_NORMAL;
    }

    /*
     * copy entire previous boot record read from flash to the current
     * record that will be written on to flash	with some feilds modified.
     */
    memcpy(&current_bootinfo, &previous_bootinfo,
					    sizeof(srxsme_bootseqinfo_t));
    /* validate the record itself */
    if (memcmp(previous_bootinfo.sb_magic, BOOTSEQ_MAGIC_STR,
					   BOOTSEQ_MAGIC_LEN) != 0) {

	printf("No valid bootsequence record found,continuing with default\n");

	current_bootinfo.sb_retry_count  = 0;
	current_bootinfo.sb_next_bootdev = disk_default;
	current_bootinfo.sb_cur_bootdev  = disk_default;
	current_bootinfo.sb_boot_success = 0;

	/* reset the slice and set boot mode to normal */
	current_bootinfo.sb_slice.sl_fields.next_slice = SELECT_ACTIVE_SLICE;
	current_bootinfo.sb_slice.sl_fields.boot_mode = BOOT_MODE_NORMAL;

	strcpy(bootdev, srxsme_map[disk_default].sd_diskname);

	/*
	 * Try to get an active slice to be appended to default
	 * disk we are returning. Also if successful, get the
	 * next slice to boot from.
	 *
	 * get_devid_from_device is back
	 * bone of slicing infrastructure. If its failing,
	 * we have something seriously wrong here. Better
	 * we continue with old sequencing scheme where we
	 * don't append any slicing info to device. If in case,
	 * things are bad only here, then loader will still
	 * be able to boot.
	 */
	device_id = get_devid_from_device(bootdev);
	if ((curr_slice = get_active_slice(device_id)) > 0) {
	    BSDEBUG("%d is active on device %d\n", curr_slice, device_id);
	    slice = get_next_slice_for_device(device_id, curr_slice);

	    /*
	     * if we got a reset slice in return, it means we
	     * can't continue with the same media next time.
	     * change the media.
	     */
	    if (slice == SELECT_ACTIVE_SLICE) {
		BSDEBUG("Next slice for %d is reset. Flipping next device\n");
		next_bootdev = disk_default + 1;
		next_bootdev = next_bootdev % num_bootable_media;
		current_bootinfo.sb_next_bootdev = next_bootdev;
	    }

	    current_bootinfo.sb_slice.sl_fields.next_slice = slice;
	} else {
	    /*
	     * If no slicing information was found, it means
	     * there was something wrong with reading MBR.
	     * Either MBR was corrupt or we had some problem
	     * in reading it. Continue with default slice.
	     */
	    printf("WARNING: No MBR found in media %s. "
		   "Using slice %d as default\n",
		   bootdev, DEFAULT_SLICE_TO_TRY);
	    slice = curr_slice = DEFAULT_SLICE_TO_TRY;
	    next_bootdev = disk_default + 1;
	    next_bootdev %= num_bootable_media;

	    current_bootinfo.sb_next_bootdev = next_bootdev;

	    /* try active of next device */
	    current_bootinfo.sb_slice.sl_fields.next_slice = SELECT_ACTIVE_SLICE;
	}


	/*
	 * If everything was fine, slice will have a valid slice
	 * in it. If still an invalid slice, use default slice
	 */
	if (!IS_VALID_SLICE(curr_slice)) {
	    BSDEBUG("Invalid slice %d. Using default slice.\n", curr_slice);
	    curr_slice = DEFAULT_SLICE_TO_TRY;
	}

	/* append slice information to device */
	sprintf(slice_buffer, "s%d", curr_slice);
	strcat(bootdev, slice_buffer);

	printf("[%d]Booting from %s slice %d\n", current_bootinfo.sb_retry_count,
	       srxsme_map[disk_default].sd_devname , curr_slice);

    if (update) {
        handle_booseq_record(&current_bootinfo, BOOTSEQ_MAGIC_LEN,
                BOOTSEQ_MAGIC_STR, WRITE_BOOTSEQ_RECORD);
    }

	return 0;
    }
    /*
     * find out whats is the retry count, this count
     * indicates how many times loader has looped
     * to boot kernel and failed
     */
    retrycount = previous_bootinfo.sb_retry_count;

    /*
     * validate the data present in record needed for bootup
     */
    cur_bootdev  = previous_bootinfo.sb_cur_bootdev;
    next_bootdev = previous_bootinfo.sb_next_bootdev;
    bootsuccess  = previous_bootinfo.sb_boot_success;
    curr_slice = previous_bootinfo.sb_slice.sl_fields.next_slice;
    boot_mode = previous_bootinfo.sb_slice.sl_fields.boot_mode;

    /*
     * validate the values present in record related to
     * bootseq, if any feild is found invalid just boot from
     * internal nand flash.
     */
    if ((cur_bootdev >= num_bootable_media)  ||
	(next_bootdev >= num_bootable_media) ||
	(bootsuccess > 1)) {
	/*
	 *  found invalid data
	 *  try booting only from default device
	 *  and also create and write new record
	 *  with proper values
	 */
	current_bootinfo.sb_retry_count  = 0;
	current_bootinfo.sb_next_bootdev = disk_default;
	current_bootinfo.sb_cur_bootdev  = disk_default;
	current_bootinfo.sb_boot_success = 0;
	strcpy(bootdev, srxsme_map[disk_default].sd_diskname);

	/* reset the slice and set boot mode to normal */
	current_bootinfo.sb_slice.sl_fields.next_slice = SELECT_ACTIVE_SLICE;
	current_bootinfo.sb_slice.sl_fields.boot_mode = BOOT_MODE_NORMAL;

	 /*
	 * Try to get an active slice to be appended to default
	 * disk we are returning.
	 */
	device_id = get_devid_from_device(bootdev);
	if ((slice = get_active_slice(device_id)) > 0) {
	    sprintf(slice_buffer, "s%d", slice);
	    strcat(bootdev, slice_buffer);
	}

	printf("Invalid boot sequence records found,continuing with default\n");
    if (update) {
        handle_booseq_record(&current_bootinfo, BOOTSEQ_MAGIC_LEN,
                BOOTSEQ_MAGIC_STR, WRITE_BOOTSEQ_RECORD);
    }
	return 0;
    }

    /*
     * if retrycount exceeds the defined BOOTSEQ_MAX_RETRIES,
     * disable watchdog, so this time even though kernel is not
     * found in any media loader just stay in loader prompt
     */
    if ((retrycount >= BOOTSEQ_MAX_RETRIES) &&
	(bootsuccess != 1)) {
	/*
	 * reset the retry count, set the next boot dev to
	 * default dev.
	 */
	current_bootinfo.sb_retry_count  = 0;
	current_bootinfo.sb_next_bootdev = disk_default;
	current_bootinfo.sb_cur_bootdev  = disk_default;
	current_bootinfo.sb_boot_success = 0;

	/* reset the slice and set boot mode to normal */
	current_bootinfo.sb_slice.sl_fields.next_slice = SELECT_ACTIVE_SLICE;
	current_bootinfo.sb_slice.sl_fields.boot_mode = BOOT_MODE_NORMAL;

	strcpy(bootdev, srxsme_map[disk_default].sd_diskname);

    if (update) {
        handle_booseq_record(&current_bootinfo, BOOTSEQ_MAGIC_LEN,
                BOOTSEQ_MAGIC_STR, WRITE_BOOTSEQ_RECORD);
    }
	reload_watchdog(DISABLE_WATCHDOG);
	return -1;
    } else if ((retrycount == NORMAL_MODE_BOOTSEQ_RETRIES) &&
	       (bootsuccess != 1)) {

	printf("WARNING: Failed to boot normally. "
	       "Switching to recovery mode.\n");

	/* 
	 * Return a device which is enabled. If no device is enabled,
	 * return error and reset boot sequencing. As a result of returning error,
	 * loader will fall to loader prompt.
	 */
	for (tmp_bootdev = disk_default; tmp_bootdev < num_bootable_media;
	     tmp_bootdev++) {
	    if (srxsme_is_device_enabled(tmp_bootdev)) {
		break;
	    }
	}

	/* No device is enabled in boot.devlist. Return error. */
	if (tmp_bootdev == num_bootable_media) {
	    srxsme_reset_bootseq(getenv(SRXSME_BOOTDEVLIST_ENV));
	    reload_watchdog(DISABLE_WATCHDOG);
	    return -1;
	}

	strcpy(bootdev, srxsme_map[tmp_bootdev].sd_diskname);

	/*
	 * Normal tries were done in previous invocations. we
	 * will enter in recovery mode and try the default media now.
	 */
	next_bootdev = tmp_bootdev + 1;
	next_bootdev %= num_bootable_media;

	/*
	 * Set the next boot dev to default dev but
	 * dont reset the retry count rather increase it,
	 * because next we will try in recovery mode.
	 */
	current_bootinfo.sb_retry_count  += 1;
	current_bootinfo.sb_next_bootdev = next_bootdev;
	current_bootinfo.sb_cur_bootdev  = tmp_bootdev;
	current_bootinfo.sb_boot_success = 0;

	/* set the slice to recovery and set boot mode to recovery */
	current_bootinfo.sb_slice.sl_fields.boot_mode = BOOT_MODE_RECOVERY;

	current_bootinfo.sb_slice.sl_fields.next_slice =get_recovery_slice();
	BSDEBUG("%s: Recovery slice: %d\n", __func__,
		current_bootinfo.sb_slice.sl_fields.next_slice);

	sprintf(slice_buffer, "s%d",
		current_bootinfo.sb_slice.sl_fields.next_slice);
	strcat(bootdev, slice_buffer);

	printf("[%d]Booting from %s slice %d\n", current_bootinfo.sb_retry_count,
	       srxsme_map[tmp_bootdev].sd_devname,
	       current_bootinfo.sb_slice.sl_fields.next_slice);

    if (update) {
        handle_booseq_record(&current_bootinfo, BOOTSEQ_MAGIC_LEN,
                BOOTSEQ_MAGIC_STR, WRITE_BOOTSEQ_RECORD);
    }

	return 0;
    }

    /*
     * check "bootsuccess", this is needed to manage
     * retry_count
     */
    if (bootsuccess) {
	/*
	 * clear retry count, indicates that kernel was booted last
	 * time, and now we are trying to boot from other media
	 */
	current_bootinfo.sb_retry_count = 0;

    /* if boot success, don't change boot device */
    next_bootdev = cur_bootdev;

	/*
	 * If box boot up successfully, from older JUNOS images, slice
	 * might not be set to reset. Set it to reset explicitly.
	 * Reset the boot mode also.
	 */
	curr_slice = SELECT_ACTIVE_SLICE;
	boot_mode = BOOT_MODE_NORMAL;
    } else {
	/*
	 * loader is trying to boot kernel from media available
	 * but failing, so increment retry count each time to
	 * keep track of how many time loader tried in this path
	 * since last kernl boot
	 */
	current_bootinfo.sb_retry_count = retrycount + 1;
	BSDEBUG("Retry count: %d\n", current_bootinfo.sb_retry_count);
    }
    /*
     * Now decide which device to boot from
     * reset: disk0:
     * request system reboot: disk0:
     * request system reboot media internal: disk0:
     * request system reboot media usb: disk1:
     *
     */
    while (count < num_bootable_media) {

	strcpy(bootdev, srxsme_map[disk_default].sd_diskname);

	/*
	 * check is 'nextboot' is present try to boot from next by traversing
	 * boot list
	 */
	if (srxsme_is_next_bootdev_present(next_bootdev) &&
	    (srxsme_map[next_bootdev].sd_flags & SD_DEVICE_ENABLED)) {

	    strcpy(bootdev, srxsme_map[next_bootdev].sd_diskname);
	    BSDEBUG("Using %s for booting\n", bootdev);

	    device_id = get_devid_from_device(bootdev);

	    /*
	     * device booting from
	     */
	    current_bootinfo.sb_cur_bootdev = next_bootdev;

	    /*
	     * if boot mode is recovery, we don't do much
	     * except for flipping the boot media. Slice
	     * remains the same.
	     */
	    if (boot_mode == BOOT_MODE_RECOVERY) {

		slice = get_recovery_slice();

		BSDEBUG("Recover Mode: Got Slice %d as recover\n", slice);

		/*
		 * If already chosen slice from boot record is not
		 * recovery slice, set it to recovery slice.
		 */
		if (curr_slice != slice)
		    curr_slice = slice;

		BSDEBUG("Boot mode is recovery. Flipping media.\n");

		/*
		 * if we are in recovery mode, there
		 * is only one slice to try. We can
		 * flip the next boot device right away
		 * as we don't have to wait to try other slices.
		 */
		next_bootdev++;
		next_bootdev %= num_bootable_media;
		current_bootinfo.sb_slice.sl_fields.boot_mode = BOOT_MODE_RECOVERY;
	    } else {
		/*
		 * If current slice points to reset. we need to try
		 * from active.
		 */
		if (curr_slice == SELECT_ACTIVE_SLICE) {
		    curr_slice = get_active_slice(device_id);
		    BSDEBUG("Current slice is reset. Reading active(%d).\n",
			    curr_slice);
		}

		/*
		 * If for reasons, we couldn't find active,
		 * go by default slice.
		 */
		if (!IS_VALID_SLICE(curr_slice)) {
		    BSDEBUG("Invalid current slice %d. Using default\n",
			    curr_slice);
		    curr_slice = DEFAULT_SLICE_TO_TRY;
		}

		slice = get_next_slice_for_device(device_id, curr_slice);
		BSDEBUG("Next slice for %d is %d\n", curr_slice, slice);

		/*
		 * flip device only when getting other slice info failed. Get
		 * next slice function will fail only when there is no MBR or
		 * next slice cylcing has reached.
		 */
		if (slice == SELECT_ACTIVE_SLICE) {
		    BSDEBUG("Boot mode is not recovery but next slice is reset."
			    " Flipping media.\n");
		    next_bootdev++;
		    next_bootdev %= num_bootable_media;
		}

		current_bootinfo.sb_slice.sl_fields.boot_mode = BOOT_MODE_NORMAL;
	    }

	    sprintf(slice_buffer, "s%d", curr_slice);
	    strcat(bootdev, slice_buffer);

	    printf("[%d]Booting from %s slice %d\n", current_bootinfo.sb_retry_count,
		   srxsme_map[current_bootinfo.sb_cur_bootdev].sd_devname,
		   curr_slice);

	    current_bootinfo.sb_slice.sl_fields.next_slice = slice;

	    BSDEBUG("Next Boot Dev: %d Slice: %d\n", next_bootdev, slice);


	    /*
	     * device the loader should try booting next time
	     */
	    current_bootinfo.sb_next_bootdev = next_bootdev;

	    /*
	     * clear bootsuccess
	     */
	    current_bootinfo.sb_boot_success = 0;

	    /*
	     * able to determine which device to boot from
	     * come out of while
	     */
	    break;

	} else {
	    BSDEBUG("Device %d is not present or enabled.\n", next_bootdev);
	    /*
	     * if the device was not found, just increment the list
	     * and check is that device device is avaliable and can be
	     * booted from
	     */
	    next_bootdev++;
	    next_bootdev = next_bootdev % num_bootable_media;
	    curr_slice = SELECT_ACTIVE_SLICE;
	}
	count++;
    }

    if (count == num_bootable_media) {
	/*
	 * reset the retry count, set the next boot dev to
	 * default dev.
	 */
	current_bootinfo.sb_retry_count  = 0;
	current_bootinfo.sb_next_bootdev = disk_default;
	current_bootinfo.sb_cur_bootdev  = disk_default;
	current_bootinfo.sb_boot_success = 0;

	/* reset the slice and set boot mode to normal */
	current_bootinfo.sb_slice.sl_fields.next_slice = SELECT_ACTIVE_SLICE;
	current_bootinfo.sb_slice.sl_fields.boot_mode = BOOT_MODE_NORMAL;

	strcpy(bootdev, srxsme_map[disk_default].sd_diskname);

    if (update) {
        handle_booseq_record(&current_bootinfo, BOOTSEQ_MAGIC_LEN,
                BOOTSEQ_MAGIC_STR, WRITE_BOOTSEQ_RECORD);
    }
	reload_watchdog(DISABLE_WATCHDOG);
	return -1;
    }

    /* handle_booseq_record for write cmd can fail if flash
     * problems are found and if write fails to write new record needed
     * for nextboot up and if previous	record is valid and pointing
     * to some device, the loader always tries to boot from it,
     * so rather than allowing loader trying some device,
     * force it to try default always.
     */
    if (update && handle_booseq_record(&current_bootinfo, BOOTSEQ_MAGIC_LEN,
			 BOOTSEQ_MAGIC_STR, WRITE_BOOTSEQ_RECORD) == -1) {
	printf("Failed to create new record, Booting from default\n");
	strcpy(bootdev, srxsme_map[disk_default].sd_diskname);
    }
    return 0;
}

/*
 * get_next_bootdev reads next bootdev from bootflash
 */
uint8_t
get_next_bootdev (void)
{
    srxsme_bootseqinfo_t next_bootinfo;
    uint8_t bootdev;

    /*
     * Read bootseq record info from flash
     */
    handle_booseq_record(&next_bootinfo, BOOTSEQ_MAGIC_LEN,
			 BOOTSEQ_MAGIC_STR, READ_BOOTSEQ_RECORD);

    bootdev = next_bootinfo.sb_next_bootdev;
    if (next_bootinfo.sb_next_bootdev >= num_bootable_media) {
	bootdev = DISK_FIRST;
    }
    return bootdev;
}

/*
 * reload_watchdog: common routine for branching out
 * of watchdog functionality
 */
void
reload_watchdog (int32_t val)
{
    static int32_t lastwdogtime = WATCHDOG_MODE_INT_NMI_SRES;

    if (val == PAT_WATCHDOG) {
	/*
	 * use lastwdogtime platform specific patting.
	 */
	pat_ciu_watchdog();
    } else if (val == DISABLE_WATCHDOG) {
	/*
	 * disable watchdog
	 */
	disable_ciu_watchdog();
    } else if (val == ENABLE_WATCHDOG) {
	/*
	 * enable watchdog
	 */
	lastwdogtime = WATCHDOG_MODE_INT_NMI_SRES;
	enable_ciu_watchdog(WATCHDOG_MODE_INT_NMI_SRES);
    }
}

#ifdef CONFIG_API
extern unsigned long usb_stor_read(int device, unsigned long blknr,
        unsigned long blkcnt, unsigned long *buffer);

static block_dev_desc_t* dummy_usb_disk = NULL;

int32_t
is_srx550_dummy_usb(block_dev_desc_t * dev_desc)
{
    if (dummy_usb_disk && dev_desc == dummy_usb_disk)
        return 1;

    return 0;
}

block_dev_desc_t *
srx550_init_dummy_usbdev( void )
{
    block_dev_desc_t *dev_desc;

    if (dummy_usb_disk)
        return dummy_usb_disk;

    dev_desc = malloc(sizeof(block_dev_desc_t));
    if (!dev_desc) {
        printf("Warning !!! Malloc dummy USB failed \r\n");
        return NULL;
    }
    memset(dev_desc, 0, sizeof(block_dev_desc_t));
    dev_desc->target  = 0xff;
    dev_desc->if_type = IF_TYPE_USB;
    dev_desc->dev     = 0;
    dev_desc->part_type  = PART_TYPE_UNKNOWN;
    dev_desc->block_read = usb_stor_read;
#ifdef USB_WRITE_READ
    dev_desc->block_write = usb_stor_write;
#endif
    dev_desc->type = DEV_TYPE_UNKNOWN;

    dev_desc->lba = 0x100000;
    dev_desc->blksz = 512;

    dummy_usb_disk = dev_desc;

    return dev_desc;
}

void
set_last_booseq_record_finish(void)
{
    srxsme_bootseqinfo_t bootinfo;

    handle_booseq_record(&bootinfo, BOOTSEQ_MAGIC_LEN,
			 BOOTSEQ_MAGIC_STR, READ_BOOTSEQ_RECORD);


    bootinfo.sb_boot_success = 1;
    handle_booseq_record(&bootinfo, BOOTSEQ_MAGIC_LEN,
            BOOTSEQ_MAGIC_STR, WRITE_BOOTSEQ_RECORD);
}

void
do_reset_jt (void)
{
    set_last_booseq_record_finish();
    do_reset();
}

uint32_t
srxsme_get_num_usb_disks( void )
{
    return num_usb_disks();
}

#endif

