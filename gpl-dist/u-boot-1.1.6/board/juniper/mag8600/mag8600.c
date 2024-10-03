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
#include <watchdog_cpu.h>
#include <octeon_boot.h>
#include <platform_mag8600.h>
#include <exports.h>

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
void mag8600_show_bootdev_list(void);
void reload_watchdog(int);
void mag8600_bootseq_init(void);

/* static declerations */
static unsigned int num_boot_disks(void);

static unsigned char mag8600_bootdev[MAX_DISK_STR_LEN];
static mag_device_t *mag8600_map = NULL;
static uint8_t	*platform_name;
static uint8_t  *platform_default_bootlist = NULL;
static uint32_t  num_bootable_media;
static uint32_t bootseq_sector_start_addr = 0;
static uint32_t bootseq_sector_len = 0;

#define MAG8600_DEF_BOOT_LIST		"usb:internal-compact-flash"

#define RECOVERY_SLICE_ENV		"recovery_slice"
#define BOOTSEQ_DISABLE_ENV		"boot.btsq.disable"

static mag_device_t mag8600_dev_map[] = {
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
};


extern uint8_t boot_sector_flag;

typedef int (* disk_read_vector) (int dev, int lba, int totcnt, char *buf);


#define NBASE_DECIMAL			10
#define NBASE_HEX			16



#define MBR_BUF_SIZE			512

#define MAX_NUM_SLICES			4
#define FIRST_SLICE			1
#define IS_NUMERAL(num) 		(num >= 0 && num <= 9 ? 1 : 0)
#define IS_VALID_SLICE(slice)		(slice >= FIRST_SLICE &&	      \
                                         slice <= MAX_NUM_SLICES	      \
                                         ? 1 : 0)

#define IS_BOOT_SEQ_DISABLED()	(getenv(BOOTSEQ_DISABLE_ENV) != NULL &&	      \
                                 simple_strtoul(getenv(BOOTSEQ_DISABLE_ENV),  \
                                                NULL, NBASE_DECIMAL) != 0     \
                                 ? 1: 0)

#define LIST_MAX_LEN            256

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
le16dec (const void *pp)
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
    disk_read_vector  rvec;

    /*
     * Get the disk read vector from Jump table.
     */
    rvec = gd->jt[XF_disk_read];

    if (!rvec) {
        BSDEBUG("Error: Jump table not initialized!\n");
        return -1;
    }

    /* Read the MBR. */
    if (rvec(dev, 0, 1, (char *)buf) != 0) {
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
    uint8_t       buf[MBR_BUF_SIZE];
    disk_part_t  *dp;
    int           idx = 0, found = 0;

    if (read_device_mbr(dev, buf) == 0) {
        dp = (disk_part_t *)(buf + DISK_PART_TBL_OFFSET);

        /* for each entry in MBR find slice marked active */
        for (idx = 0; idx < MAX_NUM_SLICES; idx++, dp++) {

            /* check if the slice is active */
            if (dp->boot_flag == ACTIVE_FLAG) {
                found = 1;
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
    uint8_t      buf[MBR_BUF_SIZE];
    disk_part_t  dp;

    if (!IS_VALID_SLICE(slice)) {
        return 0;
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
            return 1;
        }
    }

    return 0;
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
    uint8_t       buf[MBR_BUF_SIZE];
    disk_part_t  *dp;
    int           bsd_slices = 0, counter;

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

    int len = strlen((char *)dev_name);

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
    if (IS_NUMERAL(dev_unit)) {
        goto cleanup;
    } else {
        dev_unit = -1;
    }

cleanup:
    return dev_unit;
}

#define MAX_SIZE 0x1000
int32_t
handle_booseq_record (mag_bootseqinfo_t *bootinfo, uint32_t magiclen,
                      uint8_t *magic, uint32_t bootseq_command)
{
    uint8_t             bootseq_sector_buffer[MAX_SIZE];
    uint8_t            *temp;
    uint32_t            count;
    uint32_t            num_bootseq_rec;
    uint32_t            rec_count;
    uint32_t            bootseq_sector_end_addr;
    uint32_t            need_erase = 0;
    mag_bootseqinfo_t  *bootrecord;


    if ((bootinfo == NULL) || (magic == NULL)) {
        printf("bootinfo or magic is null\n");
        return -1;
    }

    /* copy contents of bootseq sector from flash to temporary buffer */
    memcpy(bootseq_sector_buffer, (void *)bootseq_sector_start_addr,
           MAX_SIZE);

    /* cast temp buffer to pointer boot_sequence_t */
    bootrecord = (mag_bootseqinfo_t *)bootseq_sector_buffer;

    /* maximum number of records that could fit in the sector */
    num_bootseq_rec = MAX_SIZE/ sizeof(mag_bootseqinfo_t);

    /*
     * loop through temp buffer by using num_bootseq_rec
     * to find the last valid record by using magic signature
     */
    for (rec_count = 0; rec_count < num_bootseq_rec; rec_count++) {
        if (memcmp(bootrecord->sb_magic, magic, magiclen) != 0) {
            break;
        }
        bootrecord++;
    }

    if (bootseq_command == READ_BOOTSEQ_RECORD) {
        /*
         * if there is no valid record on in the flash
         * [could happen for first time], bootrecord will be
         * pointing to begininig of buffer else adjust bootrecord
         * to point to previous record before reading record.
         */
        if (rec_count != 0) {
            bootrecord--;
        }

        /*
         * check the magic of previous record, as we dont know, did
         * we find magic or loop terminated
         * if magic is found copy the record else return 0xff in all feilds.
         */
        if (memcmp(bootrecord->sb_magic, magic, magiclen) == 0) {
            memcpy(bootinfo, (void *)bootrecord, sizeof(mag_bootseqinfo_t));
        } else {
            memset(bootinfo, 0xff, sizeof(mag_bootseqinfo_t));
            printf("bootinfo magic %x \n",bootinfo->sb_magic);
        }
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
            for (count = 0; count < sizeof(mag_bootseqinfo_t); count++) {
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

            bootrecord = (mag_bootseqinfo_t *)bootseq_sector_buffer;

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
                        sizeof(mag_bootseqinfo_t)) != 0) {
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

/*
 * mag8600_is_next_bootdev_present  determinses
 * is selected nextbootdev in the list is present or
 * not
 */
static int
mag8600_is_next_bootdev_present (unsigned int nextbootdev)
{

    switch (nextbootdev) {
    case DISK_ZERO:
        return 1;
        break;
    case DISK_FIRST:
        return 1;
        break;
    default:
        return 0;
    }
    return 0;
}

static void
disable_required_media (mag_device_t *devlist, uint32_t numdevices)
{
    int bcount = 0;
    char *bootlist = NULL;

    if ((bootlist = getenv(MAG8600_BOOTDEVLIST_ENV)) != NULL) {
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

    printf("Boot Media: ");

    for (bcount = 0; bcount < num_bootable_media; bcount++) {
        /* disable the device */
        if (mag8600_map[bcount].sd_flags & SD_DEVICE_ENABLED) {
            printf("%s ", mag8600_map[bcount].sd_devname);
        }
    }
    printf("\n");
}

void
get_bootmedia_list (char *devlist)
{
    int bcount = 0;

    for (bcount = 0; bcount < num_bootable_media; bcount++) {
        if (bcount) {
            devlist += sprintf(devlist, ", ");
        }

        devlist += sprintf(devlist, "%s", mag8600_map[bcount].sd_devname);
    }
}

int
mag8600_is_valid_devname (const char *devname)
{
    int bcount, found = 0;

    for (bcount = 0; bcount < num_bootable_media; bcount++) {
        if (!strcmp((const char *)mag8600_map[bcount].sd_devname, devname)) {
            found = 1;
            break;
        }
    }

    return (found);
}

mag8600_is_valid_devlist (const char *devlist)
{
    char *tok;
    char list[LIST_MAX_LEN];

    memcpy(list, devlist, LIST_MAX_LEN);
    tok = strtok(list, ",:");

    while (tok != NULL) {
        if (!mag8600_is_valid_devname(tok)) {
            return 0;
        }

        tok = strtok (NULL, ",:");
    }

    return 1;
}

static void
mag8600_validate_bootlist (void)
{
    /*
     * Check if valid boot list is defined in environments. If no,
     * go by defaults. This is because we will not try the
     * devices which are absent in this list.
     */
    if ((getenv(MAG8600_BOOTDEVLIST_ENV)) == NULL) {
        setenv(MAG8600_BOOTDEVLIST_ENV, (char *)platform_default_bootlist);
    } else {
        /* 
         * If boot.devlist is not null then check if it has all valid devices.
         * if not then set boot.devlist to default devices.
         */
        if (!mag8600_is_valid_devlist(getenv(MAG8600_BOOTDEVLIST_ENV))) {
            printf("WARNING: Found one or more invalid media in boot list. "
                   "Falling to defaults.\n");
            setenv(MAG8600_BOOTDEVLIST_ENV, (char *)platform_default_bootlist);
        }
    }
}
/*
 * mag8600_show_bootdev_list displays the bootable device
 * for the platform.
 */
void
mag8600_show_bootdev_list (void)
{
    unsigned int i;

    printf("Platform: %s\n", platform_name);
    for (i = 0; i< num_bootable_media; i++) {
        printf("    %s\n", mag8600_map[i].sd_devname);
    }
}

void
mag8600_bootseq_init (void)
{
    mag8600_map               = mag8600_dev_map;
    num_bootable_media        = sizeof(mag8600_dev_map) / sizeof(mag_device_t);
    platform_name             = "mag8600";
    platform_default_bootlist = MAG8600_DEF_BOOT_LIST;

    bootseq_sector_start_addr = MAG8600_BOOTSEQ_FLASH_SECTADDR;
    bootseq_sector_len        = 0x20000;

    mag8600_validate_bootlist();
    disable_required_media(mag8600_map, num_bootable_media);
    print_enabled_media();
}

int32_t
mag8600_set_next_bootdev (uint8_t *bootdev){
    uint32_t           i = 0;
    mag_bootseqinfo_t  current_bootinfo;
    mag_bootseqinfo_t  previous_bootinfo;

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
           sizeof(mag_bootseqinfo_t));

    for (i = 0; i < num_bootable_media; i++) {
        if (strcmp((const char *)bootdev,
                   (const char *)mag8600_map[i].sd_devname) == 0) {
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
 * mag8600_get_next_bootdev mag8600 platform specfic implementation
 * of boot sequencing and returns
 */
int32_t
mag8600_get_next_bootdev (uint8_t *bootdev)
{
    unsigned int       bootsuccess;
    unsigned int       next_bootdev, cur_bootdev;
    unsigned int       retrycount = 0;
    unsigned int       count = 0;
    uint8_t            disk_default = 0;
    int                slice, curr_slice, device_id, boot_mode;
    uint8_t            slice_buffer[8];
    mag_bootseqinfo_t  previous_bootinfo;
    mag_bootseqinfo_t  current_bootinfo;

    /*
     * If boot sequencing is disabled, don't
     * do anything. currdev will anyways be
     * set to the default device.
     */
    if (IS_BOOT_SEQ_DISABLED()) {
        return 0;
    }

    disk_default = DISK_DEFAULT_MAG8600;

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
    if (getenv(BOOTSEQ_DISABLE_ENV) != NULL) { 
        if (!VALID_BOOT_REC_VERSION(previous_bootinfo.sb_slice.sl_fields.version))  
        {
            BSDEBUG("Boot record with invalid version %d.\n",
                previous_bootinfo.sb_slice.sl_fields.version);
            previous_bootinfo.sb_slice.sl_fields.next_slice = SELECT_ACTIVE_SLICE;
            previous_bootinfo.sb_slice.sl_fields.boot_mode = BOOT_MODE_NORMAL;
        }
    }
    else
	memset(&previous_bootinfo,0,sizeof(mag_bootseqinfo_t));
    /*

    /*
     * copy entire previous boot record read from flash to the current
     * record that will be written on to flash	with some feilds modified.
     */
    memcpy(&current_bootinfo, &previous_bootinfo,
           sizeof(mag_bootseqinfo_t));
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

        strcpy(bootdev, (char *)mag8600_map[disk_default].sd_diskname);

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
               mag8600_map[disk_default].sd_devname , curr_slice);

        handle_booseq_record(&current_bootinfo, BOOTSEQ_MAGIC_LEN,
                             BOOTSEQ_MAGIC_STR, WRITE_BOOTSEQ_RECORD);

        return 0;
    }
    /*
     * find out whats is the retry count, this count
     * indicates how many times loader has looped
     * to boot kernel and failed
     */
    retrycount   = previous_bootinfo.sb_retry_count;

    /*
     * validate the data present in record needed for bootup
     */
    cur_bootdev  = previous_bootinfo.sb_cur_bootdev;
    next_bootdev = previous_bootinfo.sb_next_bootdev;
    bootsuccess  = previous_bootinfo.sb_boot_success;
    curr_slice   = previous_bootinfo.sb_slice.sl_fields.next_slice;
    boot_mode    = previous_bootinfo.sb_slice.sl_fields.boot_mode;

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
        strcpy(bootdev, mag8600_map[disk_default].sd_diskname);

        /* reset the slice and set boot mode to normal */
        current_bootinfo.sb_slice.sl_fields.next_slice = SELECT_ACTIVE_SLICE;
        current_bootinfo.sb_slice.sl_fields.boot_mode  = BOOT_MODE_NORMAL;

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
        handle_booseq_record(&current_bootinfo, BOOTSEQ_MAGIC_LEN,
                             BOOTSEQ_MAGIC_STR, WRITE_BOOTSEQ_RECORD);
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

        strcpy(bootdev, mag8600_map[disk_default].sd_diskname);

        handle_booseq_record(&current_bootinfo, BOOTSEQ_MAGIC_LEN,
                             BOOTSEQ_MAGIC_STR, WRITE_BOOTSEQ_RECORD);
        reload_watchdog(DISABLE_WATCHDOG);
        return -1;
    } else if ((retrycount == NORMAL_MODE_BOOTSEQ_RETRIES) &&
               (bootsuccess != 1)) {

        printf("WARNING: Failed to boot normally. "
               "Switching to recovery mode.\n");

        strcpy(bootdev, mag8600_map[disk_default].sd_diskname);

        /*
         * Normal tries were done in previous invocations. we
         * will enter in recovery mode and try the default media now.
         */
        next_bootdev  = disk_default + 1;
        next_bootdev %= num_bootable_media;

        /*
         * Set the next boot dev to default dev but
         * dont reset the retry count rather increase it,
         * because next we will try in recovery mode.
         */
        current_bootinfo.sb_retry_count  += 1;
        current_bootinfo.sb_next_bootdev  = next_bootdev;
        current_bootinfo.sb_cur_bootdev   = disk_default;
        current_bootinfo.sb_boot_success  = 0;

        /* set the slice to recovery and set boot mode to recovery */
        current_bootinfo.sb_slice.sl_fields.boot_mode = BOOT_MODE_RECOVERY;

        current_bootinfo.sb_slice.sl_fields.next_slice = get_recovery_slice();
        BSDEBUG("%s: Recovery slice: %d\n", __func__,
                current_bootinfo.sb_slice.sl_fields.next_slice);

        sprintf(slice_buffer, "s%d",
                current_bootinfo.sb_slice.sl_fields.next_slice);
        strcat(bootdev, slice_buffer);

        printf("[%d]Booting from %s slice %d\n", current_bootinfo.sb_retry_count,
               mag8600_map[disk_default].sd_devname,
               current_bootinfo.sb_slice.sl_fields.next_slice);

        handle_booseq_record(&current_bootinfo, BOOTSEQ_MAGIC_LEN,
                             BOOTSEQ_MAGIC_STR, WRITE_BOOTSEQ_RECORD);

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

        /*
         * If box boot up successfully, from older JUNOS images, slice
         * might not be set to reset. Set it to reset explicitly.
         * Reset the boot mode also.
         */
        curr_slice = SELECT_ACTIVE_SLICE;
        boot_mode  = BOOT_MODE_NORMAL;
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

        strcpy(bootdev, mag8600_map[disk_default].sd_diskname);

        /*
         * check is 'nextboot' is present try to boot from next by traversing
         * boot list
         */
        if (mag8600_is_next_bootdev_present(next_bootdev) &&
            (mag8600_map[next_bootdev].sd_flags & SD_DEVICE_ENABLED)) {

            strcpy(bootdev, mag8600_map[next_bootdev].sd_diskname);
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
                if (curr_slice != slice) {
                    curr_slice = slice;
                }

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
                   mag8600_map[current_bootinfo.sb_cur_bootdev].sd_devname,
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
        current_bootinfo.sb_slice.sl_fields.boot_mode  = BOOT_MODE_NORMAL;

        strcpy(bootdev, mag8600_map[disk_default].sd_diskname);

        handle_booseq_record(&current_bootinfo, BOOTSEQ_MAGIC_LEN,
                             BOOTSEQ_MAGIC_STR, WRITE_BOOTSEQ_RECORD);
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
    if (handle_booseq_record(&current_bootinfo, BOOTSEQ_MAGIC_LEN,
                             BOOTSEQ_MAGIC_STR, WRITE_BOOTSEQ_RECORD) == -1) {
        printf("Failed to create new record, Booting from default\n");
        strcpy(bootdev, mag8600_map[disk_default].sd_diskname);
    }
    return 0;
}

/*
 * get_next_bootdev reads next bootdev from bootflash
 */
uint8_t
get_next_bootdev (void)
{
    mag_bootseqinfo_t  next_bootinfo;
    uint8_t            bootdev;
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
