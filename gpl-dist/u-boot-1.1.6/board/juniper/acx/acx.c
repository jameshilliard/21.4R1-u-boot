/*
 * $Id$
 *
 * acx.c -- Platform specific code for the Juniper ACX Product Family
 *
 * Rajat Jain, Feb 2011
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
#include <command.h>
#include <pci.h>
#include <usb.h>
#include <asm/processor.h>
#include <asm/mmu.h>
#include <asm/immap_85xx.h>
#include <asm/io.h>
#include <spd.h>
#include <miiphy.h>
#include <i2c.h>
#include <watchdog.h>
#include <acx_syspld.h>
#include <exports.h> /* usb_disk_scan */
#include <acx_i2c_usb2513.h>

#define ACX1000_NAME_STRING	"ACX-1000"
#define ACX2000_NAME_STRING	"ACX-2000"
#define ACX4000_NAME_STRING	"ACX-4000"
#define ACX2100_NAME_STRING     "ACX-2100"
#define ACX2200_NAME_STRING     "ACX-2200"
#define ACX1100_NAME_STRING     "ACX-1100"
#define ACX500_O_POE_DC_NAME_STRING  "ACX-500-O-POE-DC"
#define ACX500_O_POE_AC_NAME_STRING  "ACX-500-O-POE-AC"
#define ACX500_O_DC_NAME_STRING      "ACX-500-O-DC"
#define ACX500_O_AC_NAME_STRING      "ACX-500-O-AC"
#define ACX500_I_DC_NAME_STRING      "ACX-500-DC"
#define ACX500_I_AC_NAME_STRING      "ACX-500-AC"

/*
 * Next-boot information is stored in NVRAM
 * Register format is as follows.
 * ------------------------------------------------------------------------+
 * | BIT 7  |  BIT 6 |  BIT 5 |  BIT 4 |  BIT 3 |  BIT 2 |  BIT 1 |  BIT 0 |
 * +--------------------------+--------+--------+--------+--------+--------+
 * |    NEXT_BOOT             |  X     |  USB   | FLASH0 | FLASH1 |  X     |
 * |                          |        |  BOOT  | BOOT   |  BOOT  |        |
 * +--------------------------+--------+--------+--------+--------+--------+
 * NEXT_BOOT =
 *             1 => FLASH-0
 *             2 => FLASH-1
 *             3 => USB
 */
#define NAND_FLASH_0_BOOT               2
#define USB_BOOT                        3

#define NAND_FLASH_0_BOOTMASK           (1 << NAND_FLASH_0_BOOT)
#define USB_BOOTMASK                    (1 << USB_BOOT)

#define BOOT_DEVICE(x)                  ((x & 0xe0) >> 5)

#define DEFAULT_CURRBOOTDEV ((USB_BOOT << 5) |	\
	(NAND_FLASH_0_BOOTMASK | USB_BOOTMASK))

#define NAND_CURRBOOTDEV ((NAND_FLASH_0_BOOT << 5) | \
			  (NAND_FLASH_0_BOOTMASK))


/* ========= dual root partition logic defines begin =======================*/
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

#define ACX_NAND0                       0
#define SELECT_ACTIVE_SLICE		0
#define DEFAULT_SLICE_TO_TRY		1

#if ENABLE_DR_DEBUG 
#define DR_DEBUG(fmt, args...)	printf(fmt, ## args)
#else
#define DR_DEBUG(fmt, args...)
#endif

#define TRUE				1
#define FALSE				0
#define MBR_BUF_SIZE			512
#define MAX_NUM_SLICES			4
#define FIRST_SLICE			1
#define IS_NUMERAL(num) 		(num >= 0 && num <= 9 ? TRUE : FALSE)
#define IS_VALID_SLICE(slice)		(slice >= FIRST_SLICE &&	      \
					 slice <= MAX_NUM_SLICES	      \
					 ? TRUE : FALSE)

/* ========= dual root partition logic defines end   =======================*/

extern void flush_console_input(void);
extern int readline(const char *const prompt);
extern unsigned long usb_stor_write(int device, unsigned long blknr,
				    unsigned long blkcnt,
				    unsigned long *buffer);
extern void acx_measured_boot(void);
extern block_dev_desc_t usb_dev_desc[USB_MAX_STOR_DEV];
extern struct usb_device usb_dev[USB_MAX_DEVICE];
extern char console_buffer[];
int usb_scan_dev = 0;
int watchdog_tickle_override = 0;
DECLARE_GLOBAL_DATA_PTR;

typedef struct nvram_params {
    struct {
	uint8_t  version;
	uint8_t  junos_reboot_reason;
	uint8_t  uboot_reboot_reason;
	uint8_t  bdev_nextboot;
	uint32_t re_memsize;
	uint32_t re_memsize_crc;
	uint8_t  noboot_flag;
	    #define NOBOOT_FLAG 0x41
	uint8_t  misc;
	uint8_t  next_slice;
	uint8_t  reboot_overheat_flag; /* set the flag if the reboot is due to
	                                  over heating */
	    #define REBOOT_OVHEAT_FLAG 0x1
	uint32_t reboot_wait_time;     /* wait time in min if the above flag
	                                  set */
	uint32_t reboot_wait_time_crc; /* crc for the field */
	uint8_t  default_board; /* flag to indicate default board type */
	uint8_t  unattended_mode;
	    #define PASSWORD_LEN 256
	char     boot_loader_pwd[PASSWORD_LEN + 1];
	uint8_t  pwd_flag;
	uint8_t  hash_type; /* last bit indicates if the hash type is set */
	uint8_t  bios_upgrade; /* BIOS upgrade request through JUNOS CLI */
            #define ICRT_VERSION_LEN 64
        char     icrt_version[ICRT_VERSION_LEN];
        uint8_t  res[1697]; /* = what remains of the 2048 byte re nvram area */
    } re_side;

    /*
     * Scratchpad area in NVRAM for u-boot
     */
    struct {
	uint32_t  post_word;
	uint32_t  wdt_test;
	uint32_t  sreset_test;
	uint8_t   reboot_reason;
	uint8_t   res[2035];
    } uboot_side;

} nvram_params_t;

/*
 * PFE uses 124KB of NVRAM
 */
#define CFG_NVRAM_PFE_USED (124 * 1024)

#define ACX500_NOR_FLASH_WP_ALL 0x0F

static volatile nvram_params_t *nvram = (volatile nvram_params_t *) 
					(CFG_NVRAM_BASE + CFG_NVRAM_PFE_USED);

/*
 * Check the reboot is beacuse of over heating
 * if yes load the bootdelay environment variable with
 * reboot wait_time value
 */
void check_reboot_due_to_over_heat (void)
{
    char delay_str[12];
    uint8_t buf[1 + sizeof(nvram->re_side.reboot_wait_time)];

    if (REBOOT_OVHEAT_FLAG == nvram->re_side.reboot_overheat_flag) {
	/*
	 * Ensure CRC is correct
	 */
	buf[0] = nvram->re_side.reboot_overheat_flag;
	memcpy(&buf[1], (void *)&(nvram->re_side.reboot_wait_time),
	       sizeof(nvram->re_side.reboot_wait_time));
	/*
	 * Check the crc field
	 */
	if (nvram->re_side.reboot_wait_time_crc != crc32(0, buf, sizeof(buf))) {
	    /*
	     * Clear the flag so that next reboot will be fine 
	     */
	    nvram->re_side.reboot_overheat_flag = 0;
	    printf("** Bad reboot wait time Checksum in NVRAM **\n");
	    return;
	}
	sprintf(delay_str, "%d", (nvram->re_side.reboot_wait_time * 60));
	setenv("bootdelay", delay_str);
	/*
	 * Reset the flag 
	 */
	nvram->re_side.reboot_overheat_flag = 0;
    }
}
uint32_t nvram_get_uboot_wdt_test(void)
{
    return nvram->uboot_side.wdt_test;
}

void nvram_set_uboot_wdt_test(uint32_t val)
{
    nvram->uboot_side.wdt_test = val;
}

uint8_t nvram_get_uboot_reboot_reason(void)
{
    return nvram->uboot_side.reboot_reason;
}

void nvram_set_uboot_reboot_reason(uint8_t val)
{
    nvram->uboot_side.reboot_reason = val;
}

uint32_t nvram_get_uboot_sreset_test(void)
{
	return nvram->uboot_side.sreset_test;
}

void nvram_set_uboot_sreset_test(uint32_t val)
{
	nvram->uboot_side.sreset_test = val;
}

int get_noboot_flag(void)
{
	/*
	 * If noboot is set, ask u-boot to abort autoboot process.
	 * Also clear noboot flag if set. This is to ensure that
	 * we are never forever stuck in noboot state.
	 */
	if (nvram->re_side.noboot_flag == NOBOOT_FLAG) {
		nvram->re_side.noboot_flag = 0;
		return 1;
	}

	return 0;
}

static void set_noboot_flag(void)
{
	/*
	 * Set noboot bit.
	 */
	nvram->re_side.noboot_flag = NOBOOT_FLAG;
}

static void set_boot_device(int dev, int is_netboot, int slice)
{
	char dev_str[32];
	if (is_netboot) {
		sprintf(dev_str, "net%d:", dev);
	} else {
            if (slice) {
                sprintf(dev_str, "disk%ds%d:", dev, slice);
            } else {
		sprintf(dev_str, "disk%d:", dev);
            }
	}
	setenv("loaddev", dev_str);
}

void nvram_set_bootdev_usb(int dev)
{
	uint8_t bootinfo;

	bootinfo = nvram->re_side.bdev_nextboot;

	/* tell loader to load USB */
	set_boot_device(dev, 0, 0);
	/* mark nand as the next boot dev */
	nvram->re_side.bdev_nextboot =
		(NAND_FLASH_0_BOOT << 5) | (bootinfo & 0x1f) | USB_BOOTMASK;
	/* in next boot attempt from nand flash start from active slice */
	nvram->re_side.next_slice = SELECT_ACTIVE_SLICE;
}

int is_external_usb_present(int *dev)
{
	unsigned char i;
	struct usb_device *pdev;

	for(i=0;i<USB_MAX_STOR_DEV;i++) {
		if (usb_dev_desc[i].target == 0xff) {
			continue;
		}

		pdev = &usb_dev[usb_dev_desc[i].target -1];
		if (pdev->devnum == -1)
			continue;

		/*
		 * On Fortius Board, make sure USB is plugged into 1 of 2 ext USB ports.
		 */
		if (pdev->parent && ((pdev->portnr == 1) || (pdev->portnr == 2))) {
			*dev = 1;
			return 1;
		}
	}

	return 0; /* no external usb device found */
}

/*
 * Function to get NAND device number based on port
 * number. NAND is connected on Port 3 of USB hub
 * device.
 *
 */
int get_nand_dev_num(int *dev)
{
	unsigned char i;
	struct usb_device *pdev;

	for (i=0;i<USB_MAX_STOR_DEV;i++) {
		if (usb_dev_desc[i].target == 0xff) {
			continue;
		}

		pdev = &usb_dev[usb_dev_desc[i].target -1];
		if (pdev->devnum == -1)
			continue;

		if (pdev->parent && (pdev->portnr == 3)) {
			*dev = i;
			return 1;
		}
	}

	return 0;
}

/*
 * Reset the next slice to boot from active slice
 * on reboot
 */
void reset_nand_next_slice (void)
{

    /*
     * If unattended mode, just reset slice
     * else set default boot options
     */
    if (!(nvram->re_side.unattended_mode)) {
	nvram->re_side.bdev_nextboot = DEFAULT_CURRBOOTDEV;
    }
    nvram->re_side.next_slice = SELECT_ACTIVE_SLICE;
}

/*
 * Function to over write the NAND contents so that
 * all the data is overwritten with 0xFF in case of
 * truck roll situations to recover the box using
 * USB install media.
 */
int clear_nand_data (void)
{

    int nand_dev;
    unsigned long numblk, blksize, write_cnt, start, numblk_erase;
    unsigned long numblk_loop;
    char *write_buffer = NULL;
    char response[CFG_CBSIZE] = { 0, };
    int len;


    /*
     * Clear the previous console input
     */
    flush_console_input();


    /* Reset flash swizzle counter to give enough time */
    syspld_swizzle_disable();
    syspld_swizzle_enable();


    /*
     * Confirm that user wants destructive writes
     * if No exit, if no input syspld watchdog will reset
     * the board upon expiry
     *
     */
    printf("\n\nWARNING: Do you want to erase the contents"
	   " of NAND Flash (destructive)? [Y/N] (Default: N) ");

    /* Read the user response */
    len = readline("");

    if (len > 0) {
	strcpy(response, console_buffer);
	if (response[0] == 'y' || response[0] == 'Y') {

	    /* Get NAND device number */
	    if (!get_nand_dev_num(&nand_dev)) {
		printf("\n Error getting NAND device number..");
		return 0;
	    } 

	    /* Get Flash size in number of blocks and block size */
	    numblk = usb_dev_desc[nand_dev].lba;
	    blksize = usb_dev_desc[nand_dev].blksz;

	    /*
	     * Write all '1's to buffer
	     */
	    write_buffer = (char*) malloc(blksize);
	    if (!write_buffer) {
		printf("\n Memory allocation failed for buffer!!!");
		return 0;
	    }
	    memset(write_buffer, '1', blksize);

	    /*
	     * Print size & number of blocks
	     */
	    printf("\n Clearing NAND Flash - Blocks: %ld  Size %d MB",
		   numblk, (numblk * blksize) >> 20);

	    /* Erase 100,000 LBAs at one time */
	    numblk_loop = numblk / 100000;
	    start = 0;
	    numblk_erase = 100000;

	    while (numblk_loop) {

		printf("\n Erasing LBA from %07ld to %07ld", start,
		       (start + numblk_erase));

		/*
		 * Reset syspld watchdog timer, so that we have enough time
		 * if some thing goes wrong, it can reset the board
		 *
		 */
		syspld_swizzle_disable();
		syspld_swizzle_enable();

		memset(write_buffer, '1', blksize);

		/*
		 * Do the actual write
		 */
		write_cnt = usb_stor_write(nand_dev, start, numblk_erase,
					   (ulong *)write_buffer);
		if (!write_cnt) {
		    printf("\n Write Failed at block 0x%x\n", start);
		}

		start = start + numblk_erase;
		numblk_loop--;
	    }

	    /* Reset back flash swizzle counter for remaining writes */
	    syspld_swizzle_disable();
	    syspld_swizzle_enable();

	    /* erase the last remaining blocks */
	    numblk_erase = numblk - start;
	    write_cnt = usb_stor_write(nand_dev, start, numblk_erase,
				       (ulong *)write_buffer);
	    if (!write_cnt) {
		printf("\n Write Failed at block 0x%x\n", start);
	    }
	    start = start + numblk_erase;

	    printf("\n Erased %ld Blocks\n", start);


	    /* 
	     * disable flash swizzle as it has been already disabled
	     * before this function
	     *
	     */
	    syspld_swizzle_disable();

	    /* Indicate that we have cleared NAND Flash */
	    return 1;
	}
    }

    /* Indicate that we have not cleared NAND Flash */
    return 0;
}

/*
 * Clear bootloader password in case of NAND is erased in truck
 * roll situations
 */
static
void clear_password(void)
{
    nvram->re_side.pwd_flag = 0;
    setenv("password", NULL);

    return;
}

int erase_nand(void)
{
    /*
     * Check & clear NAND contents
     * depending on user response
     */
    if (clear_nand_data()) {
	    /*
	     * Clear unattended boot mode, clear the bootloader password.
	     */
	    nvram->re_side.unattended_mode = 0;
	    clear_password();
	    return 1;
    }

    return 0;
}

static int calculate_next_boot_dev(int currboot) {

	uint8_t retval = 0;

	switch (BOOT_DEVICE(currboot)) {

	case USB_BOOT:
		if (currboot & NAND_FLASH_0_BOOTMASK)
			retval = NAND_FLASH_0_BOOT;
		else
			retval = USB_BOOT;
		break;

	case NAND_FLASH_0_BOOT:
		retval = USB_BOOT;
		break;

	default:
		retval = USB_BOOT;
		break;
	}

	if (retval == USB_BOOT) {
		/* 
		 * When falling to USB boot, reset everything to default
		 */
		return DEFAULT_CURRBOOTDEV;
	} else {
		return (retval << 5) | (currboot & 0x1f);
	}
}

/* ================== dual root partition begin ====== */

static int
dr_disk_read (int dev, int lba, int totcnt, uint8_t *buf)
{
	DR_DEBUG("dr_disk_read %d, %d, %d, %d\n", dev, lba, totcnt, buf);
	/* tickle watchdog explicitly till we are reading usb media */
	syspld_watchdog_tickle();
	return (usb_disk_read(dev, lba, totcnt, buf));
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

	DR_DEBUG("ismbr... \n");
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
			DR_DEBUG("... true\n");
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
	DR_DEBUG("read_device_mbr %d\n", dev);
	/* Read the MBR. */
	if (dr_disk_read(dev, 0, 1, buf) != 0) {
		DR_DEBUG("Error: Cannot read MBR on device %d\n", dev);
		return -1;
	}

	if (!ismbr(buf)) {
		DR_DEBUG("Error: Invalid entries in MBR in device %d\n", dev);
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

	DR_DEBUG("get_active_slice %d, found = %d, %d\n", dev, found, idx+1);
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

	DR_DEBUG("is_bsd_slice %d, %d\n", dev, slice);
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

	DR_DEBUG("get_num_bsd_slices %d , return %d\n", dev, bsd_slices);
	return bsd_slices;
}
/* ================== dual root partition ends  ====== */

static u_int8_t currbootdev;
static u_int8_t currslice;

/* Basically this simple sets the loaddev env variable
 * of the loader with the appropriate disk device that needs
 * to be booted from. It does this by seting it to the device 
 * mentioned in BOOT_DEVICE(NEXT_BOOT_DEVICE). Also it does sanity 
 * testing that device mentined in BOOT_DEVICE() has its flag
 * set in the LSB of the SCRATCH1. If not, it is moved to the
 * next device.
 *
 * The next slice to try value in re_side of nvram data is the slice
 * to try in case we want to boot from nand flash0.
 *
 * This can have three values:
 *  i)  0 (defined as SELECT_ACTIVE_SLICE)
 *      This means we need to search for the MBR to find the slice
 *      marked active and use that slice to boot from. This can be
 *      either slice 1 or slice 2
 *
 *  ii) 1
 *      This explicitly tells us to use slice 1 to boot from.
 *
 *  iii)2
 *      This explicitly tells us to use slice 2 to boot from.
 *
 * This function takes care of dual root partition logic. The logic
 * goes like this.
 *
 * 1. If the nvram contains bad values, reset it to sane values.
 *      Which is USB boot next, USB & NAND-FLASH0 enabled in mask.
 *      & the next slice to try is SELECT_ACTIVE_SLICE.
 *
 * 2. If we are attempting to boot from USB; and we have a usb disk
 *    attached, use it to load the kernel. 
 *      -> and, set the next boot device to nand-flash0, and next slice
 *          to SELECT_ACTIVE_SLICE
 *
 * 3. If we are attempting to boot from nand-flash0 then;
 *  3a. Does the nand flash0 mbr indicates that we have a dual root
 *      partition nand flash; (num bsd slices = 4 (MAX_NUM_SLICES))
 *      no> use the nand flash0 slice 1 (DEFAULT_SLICE_TO_TRY) to
 *          load kernel
 *          and, set next the next boot device to USB, and
 *          next slice to SELECT_ACTIVE_SLICE
 *
 *      yes> what is the curr slice which we are using to boot now?
 *          0(SELECT_ACTIVE_SLICE)> read mbr; find active slice; tell
 *              loader to use it
 *              and, set the next slice to the other slice that has
 *              root fs. (2, if active slice is 1; 1 if active slice is 2)
 *
 *          1 or 2> tell loader to use it;
 *              and, set the next boot device to be USB
 *              and, set the next slice to be 0 (SELECT_ACTIVE_SLICE)
 *
 *
 * Loader get the device to load kernel via the "loaddev" environment
 * variable.
 *
 * disk1    -> external usb
 * disk0    -> nand flash (loader picks up the active slice in the media to
 *              boot from)
 * disk0s1  -> nand flash slice 1
 * disk0s2  -> nand flash slice 2
 *
 */
static void set_boot_disk(void)
{
	int dev = 0;
	int active_slice = 0;
	int nand_dev_num = 0;

	currbootdev = nvram->re_side.bdev_nextboot;
	currslice   = nvram->re_side.next_slice;

	if (!(currbootdev & (NAND_FLASH_0_BOOTMASK | USB_BOOTMASK)) ||
		((BOOT_DEVICE(currbootdev) != USB_BOOT) && 
		(BOOT_DEVICE(currbootdev) != NAND_FLASH_0_BOOT))) {
		/* 
		 * Junk Value, reset it to default value
		 * i.e. All devices in mask, External usb is default boot device
		 */
		printf("Resetting the list of boot devices to default (USB, NAND-0)\n");
		currbootdev = DEFAULT_CURRBOOTDEV;
		nvram->re_side.bdev_nextboot = currbootdev;
		nvram->re_side.next_slice = SELECT_ACTIVE_SLICE;
	}

	/* Disable USB booting, if unattended mode is ON */
	if (nvram->re_side.unattended_mode) {
	    currbootdev = NAND_CURRBOOTDEV;
	    nvram->re_side.bdev_nextboot  = currbootdev;
	} else {
	    /* If USB was disabled before, enable now */
	    currbootdev |= USB_BOOTMASK;
	    nvram->re_side.bdev_nextboot  = currbootdev;
	}

	if (BOOT_DEVICE(currbootdev) == USB_BOOT) {
		if ((currbootdev & USB_BOOTMASK) && is_external_usb_present(&dev)) {
			printf("Will attempt to boot from External USB drive\n");
			set_boot_device(dev, 0, 0);
			goto bootup;
		} else {
			currbootdev = calculate_next_boot_dev(currbootdev);
		}
	} 
		
	if (BOOT_DEVICE(currbootdev) == NAND_FLASH_0_BOOT) {
		if (currbootdev & NAND_FLASH_0_BOOTMASK) { 

			if (!(get_nand_dev_num(&nand_dev_num))) {
			    DR_DEBUG("\n Error getting NAND Device Number...");
			}

			if (get_num_bsd_slices(nand_dev_num) != MAX_NUM_SLICES) {
				/* no dual root formatted media */
				set_boot_device(0, 0, DEFAULT_SLICE_TO_TRY);
				goto bootup;
			} else {
				/* dual root formatted media */

				/* 
				 * In case of power cycle dont just go to other
				 * slice
				 *
				 */
				if(nvram_get_uboot_reboot_reason() ==
				   RESET_REASON_POWERCYCLE) {
					currslice = SELECT_ACTIVE_SLICE;
				}

				if (currslice == SELECT_ACTIVE_SLICE) {
					/* first time trying to boot to nand flash */
					active_slice = get_active_slice(nand_dev_num);
					if (active_slice != -1) {
						/* good : got the active slice to try */
						set_boot_device(0, 0, active_slice);
						nvram->re_side.next_slice = ((active_slice == 1)? 2 : 1);
						DR_DEBUG("set_boot_disk: first time, active = %d, next = %d\n",
							active_slice, nvram->re_side.next_slice);
						return;
					} else {
						/* mbr read failed?? no active slice */
						DR_DEBUG("set_boot_disk: Not able to find the active slice."
							"Resetting the list of boot devices to default (USB, NAND-0)\n");
						currbootdev = DEFAULT_CURRBOOTDEV;
						nvram->re_side.bdev_nextboot = currbootdev;
						nvram->re_side.next_slice = SELECT_ACTIVE_SLICE;
						if (!(nvram->re_side.unattended_mode)) {
						    if (is_external_usb_present(&dev)) {
						    	DR_DEBUG("Will attempt to boot from External USB drive\n");
							set_boot_device(dev, 0, 0);
							goto bootup;
						    }
						}
						currbootdev = calculate_next_boot_dev(currbootdev);
						set_boot_device(0, 0, 0);
						DR_DEBUG("mbr read failed. no usb found. stopping load\n");
						goto nobootup;
					}
				} else {
					/* we did attempt to boot from nand flash before
					 * but that was unsuccessful and that is why we are
					 * here now
					 */
					DR_DEBUG(": trying the next slice %d\n", currslice);
					/* tell loader to try next slice */
					set_boot_device(0, 0, currslice);
					/* also, if that fails to boot, we must go to usb */
					/* Disable USB booting, if unattended mode is ON */
					if (nvram->re_side.unattended_mode) {
					    currbootdev = NAND_CURRBOOTDEV;
					    nvram->re_side.bdev_nextboot  = currbootdev;
					} else {
					    /* Enable default if not unattended mode */
					    currbootdev = DEFAULT_CURRBOOTDEV;
					}
					nvram->re_side.next_slice = SELECT_ACTIVE_SLICE;
					return;
				}
			}
		} else {
			currbootdev = calculate_next_boot_dev(currbootdev);
		}
	}

nobootup:
	printf("Can't boot JunOS since no valid boot devices specified by user.\n");
	printf("Falling to default config. Will try all devices on next boot..\n");
	set_noboot_flag();

bootup:
	/* 
	 * Move next boot to next device in case can't boot from current device.
	 */
	nvram->re_side.bdev_nextboot = calculate_next_boot_dev(currbootdev);
	if (nvram->re_side.bdev_nextboot == DEFAULT_CURRBOOTDEV) {
		nvram->re_side.next_slice = SELECT_ACTIVE_SLICE;
	}

	return;
}

int
acx_disk_scan (int dev)
{
	static int first_time_enter = 1;
	char *loaddev;

	if (first_time_enter) {

		loaddev = getenv("loaddev");

		if (loaddev) {
			printf("Will try to boot from\n");
			if (memcmp(loaddev, "disk1", 5) == 0) {
				if (currbootdev & USB_BOOTMASK)
					printf("USB\n");
				if (currbootdev & NAND_FLASH_0_BOOTMASK)
					printf("nand-flash0\n");
			} else if (memcmp(loaddev, "disk0", 5) == 0) {
				if (currbootdev & NAND_FLASH_0_BOOTMASK)
					printf("nand-flash0\n");
			}
		}
		first_time_enter = 0;

		/* stop watchdog tickle by timer interrupt */
		watchdog_tickle_override = 1;
	}

	return usb_disk_scan(dev);
}

static int get_usb_index(struct usb_device *dev)
{
	int i;
	for (i = 0; i < USB_MAX_DEVICE; i++) {
		if ((usb_dev[i].devnum != -1) && (dev == &usb_dev[i])) {
			return (i);
		}
	}
	return -1;
}


int acx_disk_read(int dev, int lba, int totcnt, char *buf)
{
	static int first_time_enter = 1;
	struct usb_device *udev;

	if (first_time_enter) {
		int i;

		for(i=0;i<USB_MAX_DEVICE;i++) {
			udev = &usb_dev[i];
			if ((udev->devnum != -1) && (udev->devnum == usb_dev_desc[dev].target)) {			
				break;
			}
		}

		if (udev->devnum != -1 && i < USB_MAX_DEVICE) {
			if ((get_usb_index(udev->parent) == 1) && (udev->portnr == 3))
				printf("Trying to boot from nand-flash0\n");
			else
				printf("Trying to boot from USB\n");
		}

		first_time_enter = 0;
	}

	/* tickle watchdog explicitly till we are reading usb media */
	syspld_watchdog_tickle();
	return (usb_disk_read(dev, lba, totcnt, buf));
}

unsigned long acx_re_memsize;

static void cal_re_memsize(void)
{
#define MB 0x100000UL

    unsigned int nvram_corrupt = 0;

    if (nvram->re_side.re_memsize_crc != 
	crc32(0, (const unsigned char*)&nvram->re_side.re_memsize, 4)) {
	/* Ensure CRC is correct */
	printf("** Bad RE memsize Checksum in NVRAM **\n");
	nvram_corrupt = 1;

    } else if (((gd->board_type == I2C_ID_ACX1000) ||
                (gd->board_type == I2C_ID_ACX1100) ||
                (gd->board_type >= I2C_ID_ACX500_O_POE_DC) ||
                (gd->board_type <= I2C_ID_ACX500_I_AC)) && 
	       ((nvram->re_side.re_memsize < (100 * MB)) || 
	       (nvram->re_side.re_memsize > (900 * MB)))) {
	/* Ensure within allowed limits for ACX1000 */
	printf("** Bad RE memsize in NVRAM (0x%X bytes) **\n", 
	       nvram->re_side.re_memsize);
	nvram_corrupt = 1;

    } else if (((gd->board_type == I2C_ID_ACX2000) ||
                (gd->board_type == I2C_ID_ACX4000) ||
                (gd->board_type == I2C_ID_ACX2100) ||
                (gd->board_type == I2C_ID_ACX2200)) && 
               ((nvram->re_side.re_memsize < (200 * MB)) ||
	       (nvram->re_side.re_memsize > (1800 * MB)))) {
	/* Ensure within allowed limits for ACX2000 / ACX4000 */
	printf("** Bad RE memsize in NVRAM (0x%X bytes) **\n", 
	       nvram->re_side.re_memsize);
	nvram_corrupt = 1;
    }

    /* 
     * Reset to default memsize if required
     * Also recalculate the CRC and write so this 
     * does not have to be done again
     */
    if (nvram_corrupt) {
        switch(gd->board_type) {
            case I2C_ID_ACX1000:
            case I2C_ID_ACX1100:
            case I2C_ID_ACX500_O_POE_DC:
            case I2C_ID_ACX500_O_POE_AC:
            case I2C_ID_ACX500_O_DC:
            case I2C_ID_ACX500_O_AC:
            case I2C_ID_ACX500_I_DC:
            case I2C_ID_ACX500_I_AC:
	        nvram->re_side.re_memsize = ACX1000_DEFAULT_RE_MEMSIZE;
                break;
            case I2C_ID_ACX2000:
            case I2C_ID_ACX2100:
            case I2C_ID_ACX2200:
            case I2C_ID_ACX4000:
            default:
	        nvram->re_side.re_memsize = ACX2000_DEFAULT_RE_MEMSIZE;
                break;
        }
	nvram->re_side.re_memsize_crc = crc32(0, (const unsigned char*)
					      &nvram->re_side.re_memsize, 4);
	printf("Reseting RE memsize to default in NVRAM (%u MB)\n", 
	       nvram->re_side.re_memsize >> 20);	    
    }

    /* Actually use the memsize specified in NVRAM */
    acx_re_memsize = nvram->re_side.re_memsize;
}

extern int post_result_cpu;
extern int post_result_mem;
extern int post_result_rom;
extern int post_result_syspld;
extern int post_result_ether;
extern int post_result_usb;
extern int post_result_nand;
extern int post_result_nor;
extern int post_result_watchdog;

extern long int fixed_sdram(int board_type);

void sdram_init(void);
int testdram(void);


void hw_acx_watchdog_reset(void)
{
	syspld_cpu_reset();
	return ;
}

#ifdef CONFIG_HW_WATCHDOG
void hw_watchdog_reset(void)
{
	/*
	 * u-boot timer interrupt tickles watchdog periodically.
	 * When u-boot starts executing loader, we want timer 
	 * interrupt to stop tickling watchdog. Now if loader
	 * fails loading kernel image. it will fall to loader prompt.
	 * Since watchdog is not tickled anymore, it will reset
	 * the board. During next-boot loader will try to load
	 * from next available device. watchdog_tickle_override
	 * flag is set by usb scan function the first time
	 * it is executed.
	 */
	if (watchdog_tickle_override)
		return;

	syspld_watchdog_tickle();
}

void hw_watchdog_enable(void)
{
	syspld_watchdog_enable();
}
#endif

#ifdef CONFIG_POST

void post_word_store(ulong a)
{
	nvram->uboot_side.post_word = a;
}

ulong post_word_load(void)
{
	return nvram->uboot_side.post_word;
}

/*
 *  Returns 1 if there is a need to run the SLOW tests
 *  Called from board_init_f().
 */
int post_hotkeys_pressed(void)
{
	char *slowtest_run;
	unsigned delay = 5, abort = 0;

	slowtest_run = getenv("run.slowtests");
	if (!slowtest_run || (memcmp(slowtest_run, "yes", 3) != 0))
		return 0;

	printf("You have chosen to boot in SLOWTEST mode (This can be changed via "
		"\"run.slowtests\" u-boot env variable).\nThis causes detailed diagnostics"
		" to be run which may take a lot of time to run.\nPlease press any "
		"key to abort this mode and continue to boot in NORMAL mode: %2d", delay);

	while ((delay > 0) && (!abort)) { 
		int i; 
 
		--delay; 
		/* delay 100 * 10ms */ 
		for (i=0; !abort && i<100; ++i) { 
			if (tstc()) {   /* we got a key press   */ 
				abort  = 1; /* don't got to SLOWTEST mode */
				delay = 0;  /* no more delay    */ 
				(void) getc();  /* consume input    */ 
				break; 
			} 
			udelay (10000); 
		} 

		printf ("\b\b\b%2d ", delay); 
	}

	if (abort) {
		printf("\nEntering NORMAL mode...\n");
		return 0;
	} else {
		printf("\nEntering SLOWTEST mode...\n");
		return 1;
	}
}
int
post_stop_prompt(void)
{
	if (getenv("boot.nostop")) {
		return 0;
	}

	if (post_result_mem == -1 || post_result_cpu == -1 || post_result_rom == -1 ||
	    post_result_syspld == -1 || post_result_ether == -1 || post_result_usb == -1 ||
		post_result_nand == -1 || post_result_watchdog == -1 || post_result_nor == -1) {
		printf("POST Failed! Dropping to u-boot prompt...\n");
		return 1;
	}

	return 0;
}
#endif

int board_early_init_f(void)
{
	/*
	 * Need to do this early to bring I2C switch, security chip out of reset,
	 * needed to identify board I2C ID etc.
	 */
	syspld_init();
	nvram_set_uboot_reboot_reason(syspld_get_reboot_reason());
	return 0;
}

static int board_i2c_id(u_int32_t board_id)
{
	return ((board_id & 0xffff0000) >> 16);
}

int version_major(u_int32_t board_type)
{
	return ((board_type & 0xff00) >> 8);
}

int version_minor(u_int32_t board_type)
{
	return (board_type & 0xff);
}

char *get_reboot_reason_str(void) 
{
	switch (nvram_get_uboot_reboot_reason()) {

	case RESET_REASON_POWERCYCLE:
		return "power.cycle";
	case RESET_REASON_WATCHDOG:
		return "watchdog";
	case RESET_REASON_HW_INITIATED:
		return "hw.initiated";
	case RESET_REASON_SW_INITIATED:
		return "sw.initiated";
	case RESET_REASON_SWIZZLE:
		return "flash.swizzle";
	}

	return "indeterminate";
}

int checkboard(void)
{
	uint32_t id = 0;
	uint16_t i2cid = 0;
	uint8_t  tmp;

	tmp = 0x20; /* Open Channel 5 of I2C SW1 */
#ifdef DEBUG_STUFF
{
	int i = 0; uint32_t id = 0;;
	printf("\n\nEEPROM Contents:\n");

	i2c_write(CFG_I2C_SW1_ADDR, 0, 1, &tmp, 1);

	for(i = 0; i < 256; i += 4) {
		i2c_read(CFG_I2C_EEPROM_ADDR, i, 1, (uint8_t *)&id, 4);
		if (i % 16 == 0)
			printf("\n%02X: ", i);
		printf(" %08X", id); 
	} 
	printf("\n\n");
}
#endif

	/*
	 * i2c controller is initialised in board_init_f->init_func_i2c()
	 * before checkboard is called.
	 */
	/* read assembly ID from main board EEPROM offset 0x4 */
	tmp = 0x20;
	i2c_write(CFG_I2C_SW1_ADDR, 0, 1, &tmp, 1);
	i2c_read(CFG_I2C_EEPROM_ADDR, 4, 1, (uint8_t *)&id, 4);
	i2cid = board_i2c_id(id); /* To change the I2C ID, here is the place */

	switch (i2cid) {
        case I2C_ID_ACX500_O_POE_DC:
               /*
                * only reset the flag as already earlier defaulted
                * to ACX1000
                *
                */
               if (nvram->re_side.default_board)
               {
                   nvram->re_side.default_board = 0;
                   puts("\n Clearing default_board flag");
               }
                puts("\nBoard: " ACX500_O_POE_DC_NAME_STRING);
                break;
        case I2C_ID_ACX500_O_POE_AC:
               /*
                * only reset the flag as already earlier defaulted
                * to ACX1000
                *
                */
               if (nvram->re_side.default_board)
               {
                   nvram->re_side.default_board = 0;
                   puts("\n Clearing default_board flag");
               }
                puts("\nBoard: " ACX500_O_POE_AC_NAME_STRING);
                break;
        case I2C_ID_ACX500_O_DC:
                /*
                 * only reset the flag as already earlier defaulted
                 * to ACX1000
                 *
                 */
                if (nvram->re_side.default_board)
                {
                    nvram->re_side.default_board = 0;
                    puts("\n Clearing default_board flag");
                }
                puts("\nBoard: " ACX500_O_DC_NAME_STRING);
                break;
        case I2C_ID_ACX500_O_AC:
                /*
                 * only reset the flag as already earlier defaulted
                 * to ACX1000
                 *
                 */
                if (nvram->re_side.default_board)
                {
                    nvram->re_side.default_board = 0;
                    puts("\n Clearing default_board flag");
                }
                puts("\nBoard: " ACX500_O_AC_NAME_STRING);
                break;
        case I2C_ID_ACX500_I_DC:
                /*
                * only reset the flag as already earlier defaulted
                * to ACX1000
                *
                */
               if (nvram->re_side.default_board)
               {
                   nvram->re_side.default_board = 0;
                   puts("\n Clearing default_board flag");
               }
                puts("\nBoard: " ACX500_I_DC_NAME_STRING);
                break;
       case I2C_ID_ACX500_I_AC:
                /*
                * only reset the flag as already earlier defaulted
                * to ACX1000
                *
                */
               if (nvram->re_side.default_board)
               {
                   nvram->re_side.default_board = 0;
                   puts("\n Clearing default_board flag");
               }
                puts("\nBoard: " ACX500_I_AC_NAME_STRING);
                break;
        case I2C_ID_ACX1000:
               /*
                * only reset the flag as already earlier defaulted
                * to ACX1000
                *
                */
               if (nvram->re_side.default_board) {
                   nvram->re_side.default_board = 0;
                   puts("\n Clearing default_board flag");
               }
                puts("\nBoard: " ACX1000_NAME_STRING);
                break;
        case I2C_ID_ACX1100:
               /*
                * only reset the flag as already earlier defaulted
                * to ACX1100
                *
                */
               if (nvram->re_side.default_board) {
                   nvram->re_side.default_board = 0;
                   puts("\n Clearing default_board flag");
               }
                puts("\nBoard: " ACX1100_NAME_STRING);
                break;
        case I2C_ID_ACX2000:
               /*
                * If it was defaulted to ACX 1000 board type earlier
                * reset the flag and initialize the values correctly
                *
                */
               if (nvram->re_side.default_board) {
                   nvram->re_side.default_board = 0;
                   nvram->re_side.re_memsize = ACX2000_DEFAULT_RE_MEMSIZE;
                   nvram->re_side.re_memsize_crc =
                       crc32(0, (const unsigned char*)
                             &nvram->re_side.re_memsize, 4);
                   puts("\n Clearing default_board flag");
               }
               puts("\nBoard: " ACX2000_NAME_STRING);
               break;
        case I2C_ID_ACX2100:
               /*
                * If it was defaulted to ACX 1000 board type earlier
                * reset the flag and initialize the values correctly
                *
                */
               if (nvram->re_side.default_board)
               {
                   nvram->re_side.default_board = 0;
                   nvram->re_side.re_memsize = ACX2000_DEFAULT_RE_MEMSIZE;
                   nvram->re_side.re_memsize_crc =
                       crc32(0, (const unsigned char*)
                             &nvram->re_side.re_memsize, 4);
                   puts("\n Clearing default_board flag");
               }
               puts("\nBoard: " ACX2100_NAME_STRING);
               break;
        case I2C_ID_ACX2200:
               /*
                * If it was defaulted to ACX 1000 board type earlier
                * reset the flag and initialize the values correctly
                *
                */
               if (nvram->re_side.default_board) {
                   nvram->re_side.default_board = 0;
                   nvram->re_side.re_memsize = ACX2000_DEFAULT_RE_MEMSIZE;
                   nvram->re_side.re_memsize_crc =
                       crc32(0, (const unsigned char*)
                             &nvram->re_side.re_memsize, 4);
                   puts("\n Clearing default_board flag");
               }
               puts("\nBoard: " ACX2200_NAME_STRING);
               break;
        case I2C_ID_ACX4000:
               /*
                * If it was defaulted to ACX 1000 board type earlier
                * reset the flag and initialize the values correctly
                *
                */
               if (nvram->re_side.default_board) {
                   nvram->re_side.default_board = 0;
                   nvram->re_side.re_memsize = ACX2000_DEFAULT_RE_MEMSIZE;
                   nvram->re_side.re_memsize_crc =
                       crc32(0, (const unsigned char*)
                             &nvram->re_side.re_memsize, 4);
                   puts("\n Clearing default_board flag");
               }
                puts("\nBoard: " ACX4000_NAME_STRING);
                break;
        default:
               /*
                * Initialize the flag to indicate, we have a default case
                * since ID EEPROM does not have correct board id or not
                * programmed yet.
                *
                * On new boards this location will have random value, then
                * it will be initialized to '1' and will have value '1' until
                * ID EEPROM is programmed, then it will be set to '0'
                *
                * Valid values for ACX platform types are 0x551 (ACX1000),
                * 0x552 (ACX2000), 0x553 (ACX4000), 0x562 (ACX2100),
                * 0x563 (ACX2200) and 0x564 (ACX1100).
                *
                */

		nvram->re_side.default_board = 1;

		puts("Board: No valid ACX board identified! Assuming ACX1000 by default...\n\r");
		puts("\nBoard: " ACX1000_NAME_STRING);
		i2cid = I2C_ID_ACX1000;
		break;
	}

	/* Save for global use by all */
	gd->board_type = i2cid;

	printf(" %u.%u\n", version_major(id), version_minor(id));
	printf("SysPLD:Version %u Rev %u\n", syspld_version(), syspld_revision());
	printf("U-boot booting from: Bank-%d (Active Bank)\n", syspld_get_logical_bank());
	printf("Last reboot reason : %s\n", get_reboot_reason_str());

	return 0;
}

long int initdram(int board_type)
{
	long dram_size = 0;


	puts("Initializing ");

	dram_size = fixed_sdram(board_type);

	puts("    DDR: ");
	return dram_size;
}

#if defined(CFG_DRAM_TEST)
int
testdram(void)
{
	uint *pstart = (uint *) CFG_SDRAM_BASE;
	uint *pend = (uint *) (CFG_SDRAM_BASE + CFG_SDRAM_SIZE);
	uint *p;

	/*
	 * A simple DRAM test like this takes 8 seconds to execute
	 * It makes sense to NOT do this at every SW reboot to speedd up boot.
	 * It is OK because since SW rebooted the chassis, most likely the RAM
	 * is OK. In case it is not, the user can always power cycle, or at WDT
	 * expiry this shall be run anyway.
	 */
	if (nvram_get_uboot_reboot_reason() == RESET_REASON_SW_INITIATED) {
	    return 0;
	}

	printf("DRAM test phase 1:\n");
	for (p = pstart; p < pend; p += CFG_MEMTEST_INCR/4)
		*p = 0xaaaaaaaa;

	for (p = pstart; p < pend; p += CFG_MEMTEST_INCR/4) {
		if (*p != 0xaaaaaaaa) {
			printf ("DRAM test fails at: %08x\n", (uint) p);
			return 1;
		}
	}

	printf("DRAM test phase 2:\n");
	for (p = pstart; p < pend; p += CFG_MEMTEST_INCR/4)
		*p = 0x55555555;

	for (p = pstart; p < pend; p += CFG_MEMTEST_INCR/4) {
		if (*p != 0x55555555) {
			printf ("DRAM test fails at: %08x\n", (uint) p);
			return 1;
		}
	}

	printf("DRAM test passed.\n");
	return 0;
}
#endif

static void retrive_password(void)
{
	char * pwd = (char*)&(nvram->re_side.boot_loader_pwd);

	/* read if the password has been set in nvram */
	/* if not set, clear the "password" environment variable */
	if (!nvram->re_side.pwd_flag) {
		setenv("password", NULL);
		return;
	}

	printf("Recovery password is set.\n");
	setenv("password", pwd);

	return;
}

void acx_cache_invalidate_flash(void)
{
	unsigned int loop;

        debug("acx_cache_invalidate_flash -- remap TLB for Boot Flash as caching-inhibited\n");
        for (loop = 0; loop < CFG_FLASH_SIZE; loop += 32) {
                asm volatile ("dcbi %0,%1": : "b" (CFG_FLASH_BASE), "r" (loop));
                asm volatile ("icbi %0,%1": : "b" (CFG_FLASH_BASE), "r" (loop));
        }

        /* ***** - local bus: Boot flash invalidate */
        mtspr(MAS0, TLB1_MAS0(1, 2, 0)); /* tlbsel=1 esel=2 nv=0 */
        mtspr(MAS1, TLB1_MAS1(0, 1, 0, 0, BOOKE_PAGESZ_4M));    /* V=0 */
        mtspr(MAS2, TLB1_MAS2(E500_TLB_EPN(CFG_FLASH_BASE),
                                0, 0, 0, 0, 0, 0, 0, 0));
        mtspr(MAS3, TLB1_MAS3(E500_TLB_RPN(CFG_FLASH_BASE),
                                0, 0, 0, 0, 0, 1, 0, 1, 0, 1));
        asm volatile("isync;msync;tlbwe;isync");

        /* *I*G* - local bus: Boot flash*/
        mtspr(MAS0, TLB1_MAS0(1, 2, 0)); /* tlbsel=1 esel=2 nv=0 */
        mtspr(MAS1, TLB1_MAS1(1, 1, 0, 0, BOOKE_PAGESZ_4M));    /* V=1 */
        mtspr(MAS2, TLB1_MAS2(E500_TLB_EPN(CFG_FLASH_BASE),
                                0, 0, 0, 0, 1, 0, 1, 0));/* Non-cacheable */
        mtspr(MAS3, TLB1_MAS3(E500_TLB_RPN(CFG_FLASH_BASE),
                                0, 0, 0, 0, 0, 1, 0, 1, 0, 1));
        asm volatile("isync;msync;tlbwe;isync");

}

void acx_cache_enable_flash(void)
{
        /* ***** - local bus: Boot flash invalidate */
        debug("\n Invalidating cache ");
        mtspr(MAS0, TLB1_MAS0(1, 2, 0)); /* tlbsel=1 esel=2 nv=0 */
        mtspr(MAS1, TLB1_MAS1(0, 1, 0, 0, BOOKE_PAGESZ_4M));    /* V=0 */
        mtspr(MAS2, TLB1_MAS2(E500_TLB_EPN(CFG_FLASH_BASE),
                                0, 0, 0, 0, 0, 0, 0, 0));
        mtspr(MAS3, TLB1_MAS3(E500_TLB_RPN(CFG_FLASH_BASE),
                                0, 0, 0, 0, 0, 1, 0, 1, 0, 1));
        asm volatile("isync;msync;tlbwe;isync");

        /* cacheable - local bus: Boot flash*/
        mtspr(MAS0, TLB1_MAS0(1, 2, 0)); /* tlbsel=1 esel=2 nv=0 */
        mtspr(MAS1, TLB1_MAS1(1, 1, 0, 0, BOOKE_PAGESZ_4M));    /* V=1 */
        mtspr(MAS2, TLB1_MAS2(E500_TLB_EPN(CFG_FLASH_BASE),
                                0, 0, 0, 1, 0, 0, 1, 0));/* cacheable */
        mtspr(MAS3, TLB1_MAS3(E500_TLB_RPN(CFG_FLASH_BASE),
                                0, 0, 0, 0, 0, 1, 0, 1, 0, 1));
        asm volatile("isync;msync;tlbwe;isync");
}

int last_stage_init(void)
{
	char temp[60];
	uchar enet[6];
	uint16_t esize = 0;
	uint8_t i;
	char *s, *e;
	bd_t *bd;

	bd = gd->bd;

	/*
	 * Read a valid MAC address and set "ethaddr" environment variable
	 */
	i2c_read(CFG_I2C_EEPROM_ADDR, CFG_EEPROM_MAC_OFFSET, 1, enet, 6);
	i2c_read(CFG_I2C_EEPROM_ADDR, CFG_EEPROM_MAC_OFFSET-2, 1, (unsigned char*)&esize, 1);
	i2c_read(CFG_I2C_EEPROM_ADDR, CFG_EEPROM_MAC_OFFSET-1, 1, ((unsigned char*)&esize) + 1, 1);

	if (esize) {
		/* 
		* As per the specification, the management interface is assigned the
		* last MAC address out of the range programmed in the EEPROM. U-Boot
		* was using the first in the range and this can cause problems when
		* both U-Boot and JUNOS generate network traffic in short order. The
		* switch to which the management interface is connected is exposed to
		* a sudden MAC address change for the same IP in that case and may
		* not forward packets correctly at first. This can cause timeouts and
		* thus cause user-noticable failures.
		*
		* Using the same algorithm from JUNOS to guarantee the same MAC
		* address picked for tsec0 in U-boot.
		*/
		*(uint16_t *)(enet + 4) += (esize - 1);
	}

	sprintf(temp, "%02x:%02x:%02x:%02x:%02x:%02x",
			enet[0], enet[1], enet[2], enet[3], enet[4], enet[5]);
	s = getenv("eth0addr");
	if (memcmp (s, temp, sizeof(temp)) != 0) {
		setenv("eth0addr", temp);
	}

	switch (gd->board_type) {
	case I2C_ID_ACX1000:
		sprintf(temp, "%s", ACX1000_NAME_STRING);
		break;
	case I2C_ID_ACX2000:
		sprintf(temp, "%s", ACX2000_NAME_STRING);
		break;
	case I2C_ID_ACX4000:
		sprintf(temp, "%s", ACX4000_NAME_STRING);
		break;
        case I2C_ID_ACX2100:
                sprintf(temp, "%s", ACX2100_NAME_STRING);
                break;
        case I2C_ID_ACX2200:
                sprintf(temp, "%s", ACX2200_NAME_STRING);
                break;
        case I2C_ID_ACX1100:
                sprintf(temp, "%s", ACX1100_NAME_STRING);
                break;
        case I2C_ID_ACX500_O_POE_DC:
                sprintf(temp, "%s", ACX500_O_POE_DC_NAME_STRING);
                break;
        case I2C_ID_ACX500_O_POE_AC:
                sprintf(temp, "%s", ACX500_O_POE_AC_NAME_STRING);
                break;
        case I2C_ID_ACX500_O_DC:
                sprintf(temp, "%s", ACX500_O_DC_NAME_STRING);
                break;
        case I2C_ID_ACX500_O_AC:
                sprintf(temp, "%s", ACX500_O_AC_NAME_STRING);
                break;
        case I2C_ID_ACX500_I_DC:
                sprintf(temp, "%s", ACX500_I_DC_NAME_STRING);
                break;
        case I2C_ID_ACX500_I_AC:
                sprintf(temp, "%s", ACX500_I_AC_NAME_STRING);
                break;
        default:
		sprintf(temp, "ACX Default");
		break;
	}

	setenv("hw.board.type", temp);

	/*
	 * set eth active device as eTSEC1 
	 */
	setenv("ethact", CONFIG_MPC85XX_TSEC1_NAME);
	
	/*
	 * set bd->bi_enetaddr to active device's MAC (eTSEC1).
	 */
	s = getenv ("eth0addr");
	for (i = 0; i < 6; ++i) {
		bd->bi_enetaddr[i] = s ? simple_strtoul (s, &e, 16) : 0;
		if (s)
			s = (*e) ? e + 1 : e;
	}

	/*
	 * Set bootp_if as fxp0 (eTSEC1) for ACX.
	 * This environment variable is used by JUNOS install scripts
	 */
	setenv("bootp_if", CONFIG_MPC85XX_TSEC1_NAME);

	/*
	 * Set upgrade environment variables
	 */
	sprintf(temp,"0x%08x ", CFG_UPGRADE_LOADER_ADDR);
	setenv("boot.upgrade.loader", temp);

	sprintf(temp,"0x%08x ", CFG_UPGRADE_BOOT_IMG_ADDR);
	setenv("boot.upgrade.uboot", temp);

	/*
	 * Set reboot reason
	 */
	setenv("reboot.reason", get_reboot_reason_str());

	switch (nvram_get_uboot_reboot_reason()) {

	case RESET_REASON_POWERCYCLE:
		nvram->re_side.uboot_reboot_reason = (1 << RESET_REASON_POWERCYCLE);
		break;
	case RESET_REASON_WATCHDOG:
		nvram->re_side.uboot_reboot_reason = (1 << RESET_REASON_WATCHDOG);
		break;
	case RESET_REASON_HW_INITIATED:
		nvram->re_side.uboot_reboot_reason = (1 << RESET_REASON_HW_INITIATED);
		break;
	case RESET_REASON_SW_INITIATED:
		nvram->re_side.uboot_reboot_reason = 0;	/* JunOS doesn't want to get SW initiated reasons */
		break;
	case RESET_REASON_SWIZZLE:
		nvram->re_side.uboot_reboot_reason = (1 << RESET_REASON_HW_INITIATED);	/* For Junos this is HW initiated only */
		break;
	}

	/* set boot device according to nextboot */
	set_boot_disk();

	/* Calculate the memory to be handed over to RE */
	cal_re_memsize();

	/* get the recovery password, if set, from nvram */
	retrive_password();

	/*
	 * If using default environment, save the environment
	 * now.
	 */
	if (gd->env_valid == 3) {

	    if (IS_BOARD_ACX500(gd->board_type)) {
		syspld_nor_flash_wp_set(~ACX500_NOR_FLASH_WP_ALL);
	    }

	    /* save default environment */
	    gd->flags |= GD_FLG_SILENT;
	    saveenv();
	    gd->flags &= ~GD_FLG_SILENT;
	    gd->env_valid = 1;
	}

	/*
	 * Execute ACX-500 Secure Boot Functionality.
	 * Initialize TPM, Measure U-Boot/Loader, Soft Reset CPU
	 * Store the hash values in environment variables, Set the
	 * Glass Safe Register according to Security Posture etc.
	 */
	switch (gd->board_type) {

	    case I2C_ID_ACX500_O_POE_DC:
	    case I2C_ID_ACX500_O_POE_AC:
	    case I2C_ID_ACX500_O_DC:
	    case I2C_ID_ACX500_O_AC:
	    case I2C_ID_ACX500_I_DC:
	    case I2C_ID_ACX500_I_AC:

		/*
		 * If Push Button is pressed, ask whether user
		 * wants to clear all the data in NAND Flash along
		 * with boot loader password used by U-Boot
		 * if yes, clear NAND and boot loader password
		 */
		if (syspld_push_button_detect()) {
		    clear_nand_data();
		}

		/*
		 * Check if upgrade images are present in NAND,
		 * upgrade them & Measure Loader to TPM & get
		 * the U-Boot measurement to pass to Kernel
		 *
		 */
    		acx_measured_boot();
		setenv ("icrt.ver", (char *) nvram->re_side.icrt_version);
	        break;

	    default:
		break;
	}

	return 0;
}

int board_early_init_r(void)
{
	unsigned int i;

	debug("board_early_init_r -- remap TLB for Boot Flash as caching-inhibited\n");
	for (i = 0; i < CFG_FLASH_SIZE; i += 32) {
		asm volatile ("dcbi %0,%1": : "b" (CFG_FLASH_BASE), "r" (i));
		asm volatile ("icbi %0,%1": : "b" (CFG_FLASH_BASE), "r" (i));
	}

	/* ***** - local bus: Boot flash invalidate */
	mtspr(MAS0, TLB1_MAS0(1, 2, 0)); /* tlbsel=1 esel=2 nv=0 */
	mtspr(MAS1, TLB1_MAS1(0, 1, 0, 0, BOOKE_PAGESZ_4M));	/* V=0 */
	mtspr(MAS2, TLB1_MAS2(E500_TLB_EPN(CFG_FLASH_BASE),
				0, 0, 0, 0, 0, 0, 0, 0));
	mtspr(MAS3, TLB1_MAS3(E500_TLB_RPN(CFG_FLASH_BASE),
				0, 0, 0, 0, 0, 1, 0, 1, 0, 1));
	asm volatile("isync;msync;tlbwe;isync");

	/* *I*G* - local bus: Boot flash*/
	mtspr(MAS0, TLB1_MAS0(1, 2, 0)); /* tlbsel=1 esel=2 nv=0 */
	mtspr(MAS1, TLB1_MAS1(1, 1, 0, 0, BOOKE_PAGESZ_4M));	/* V=1 */
	mtspr(MAS2, TLB1_MAS2(E500_TLB_EPN(CFG_FLASH_BASE),
				0, 0, 0, 0, 1, 0, 1, 0));/* Non-cacheable */
	mtspr(MAS3, TLB1_MAS3(E500_TLB_RPN(CFG_FLASH_BASE),
				0, 0, 0, 0, 0, 1, 0, 1, 0, 1));
	asm volatile("isync;msync;tlbwe;isync");

	/*
	 * We could not bring the USB PHY out of reset earlier since
	 * some boards have the USB reset line as active high, and some 
	 * as active low - and the board type was not known then. 
	 * Do it now.
	 */
	syspld_reset_usb_phy();

	/*
	 * Program the USB hub on Fortius via the SMB
	 * This is needed before the USB enumeration so that NAND flash
	 * and other USB devices can be detected
	 */
	extern int i2c_usb2513_init(void);
	if (i2c_usb2513_init())
		printf ("USB HUB initialization failed\n");

	return 0;
}

static int usb2513_write(uint8_t reg, uint8_t val)
{
	/*
	 * The USB2513 chip expects the num of bytes in the first byte,
	 * The actual data follows it. We always need to write 1 byte of 
	 * data, thus send 2 bytes 
	 */
	uint8_t tmp[2];
	tmp[0] = sizeof(uint8_t);
	tmp[1] = val;

	return i2c_write(CFG_I2C_USB2513_ADDR, reg, 1, tmp, 2);
}

/*
 * Write the string in UTF-16LE to the specified
 * location
 */
static int
i2c_usb2513_write_utf16le (u_int8_t reg, char *str)
{
	int ind;
	int error = 0;

	for (ind = 0; ind < strlen(str); ind++) {
		error = usb2513_write(reg + (ind * 2),  str[ind]);
		if (error) {
			break;
		}

		error = usb2513_write(reg + (ind * 2) + 1,  0);
		if (error) {
			break;
		}
	}

	return error;
}

/*
 * Initialization for usb2513
 * On fortius, the following is the downstream configuration
 * port 1 : usb connector 1
 * port 2 : usb connector 2
 * port 3 : usb mass storage controller
 */

int
i2c_usb2513_init (void)
{
	int error;
	u_int16_t ind;
	uint8_t tmp, config_data3;

	/*
	 * Open the I2C multiplxer for USB hub ac access
	 */
	tmp = 0x8;
	i2c_write(CFG_I2C_SW1_ADDR, 0, 1, &tmp, 1);


	/*
	 * Program the Vendor Id
	 */
	error = usb2513_write(USB2513_VENDORID_LSB, USB2513_VENDOR_ID_SMSC_LSB);
	if (error)
		return error;

	error = usb2513_write(USB2513_VENDORID_MSB, USB2513_VENDOR_ID_SMSC_MSB);
	if (error)
		return error;

	/*
	 * Program the Product Id
	 */
	error = usb2513_write(USB2513_PRODID_LSB, USB2513_PRODID_USB2513_LSB);
	if (error)
		return error;

	error = usb2513_write(USB2513_PRODID_MSB,USB2513_PRODID_USB2513_MSB );
	if (error)
		return error;

	/*
	 * Program the Device Id
	 */
	error = usb2513_write(USB2513_DEVID_LSB, USB2513_DEVID_DEFAULT_LSB);
	if (error)
		return error;

	error = usb2513_write(USB2513_DEVID_MSB, USB2513_DEVID_DEFAULT_MSB);
	if (error)
		return error;

	/*
	 * Program the config byte 1
	 */
	error = usb2513_write(USB2513_CONF1, USB2513_CONF1_MTT_EN |
			 USB2513_CONF1_EOP_DIS |
			 USB2513_CONF1_CURR_SNS_PERPORT |
			 USB2513_CONF1_PERPORT_POWER_SW);
	if (error)
		return error;

	/*
	 * Program the config byte 2
	 */
	error = usb2513_write(USB2513_CONF2, USB2513_CONF2_OC_TIMER_8MS);
	if (error)
		return error;

	/*
	 * There are no LED pins on the USB2513. Program to default.
	 * This should report to the upstream host that the hub has
	 * no LEDs. Enable support for strings as well.
	 */
        config_data3 = USB2513_CONF3_LED_MODE_SPEED |
                       USB2513_CONF3_STRING_SUPPORT_EN;

        if (IS_BOARD_ACX500(gd->board_type)) {
            /*
             * On Kotinos, USB Hub downstream port connection is different from
             * fortius, so port mapping feature is used to match with fortius
             * port connection. 
             * Port mapping would be:
             * Port 1 -> Port 3 (Internal Flash controller port)
             * Port 2 -> Port 1 (External USB port)
             * Port 3 -> Port 2 (Unused on Kotinos)
             */
            config_data3 |= USB2513_CONF3_PORTMAP_EN;
        }

	error = usb2513_write(USB2513_CONF3, config_data3);
	if (error)
		return error;

        if (IS_BOARD_ACX500(gd->board_type)) {
            /*
             * Map the physical port 1 to port3 and port2 to port1 to match with
             * fortius
             */
            error = usb2513_write(USB2513_PORT_MAP12, USB2513_PORTMAP_P2_TO_P1 |
                                  USB2513_PORTMAP_P1_TO_P3);
            if (error)
                return error;

            /*
             * Map port 3 to port 2
             */
            error = usb2513_write(USB2513_PORT_MAP34, USB2513_PORTMAP_P3_TO_P2);
            if (error)
                return error;

            /*
             * Disable remaining ports those are not available on usb2513 (3
             * downstream ports) device
             */
            error = usb2513_write(USB2513_PORT_MAP56, 0x00);
            if (error)
                return error;

            error = usb2513_write(USB2513_PORT_MAP7, 0x00);
            if (error)
                return error;
        }

	/*
	 * Program all ports on the usb hub as removable
	 */
	error = usb2513_write(USB2513_NONREM_DEV, 0);
	if (error)
		return error;

	/*
	 * USB2513Bi is a self-powered, three port hub. Enable all ports
	 */

	error = usb2513_write(USB2513_PORTDIS_SELF, 0);
	if (error)
		return error;

	/*
	 * USB2513Bi is configured to operate in self-powered mode.
	 * This register has no significance. Write the default value
	 * of 0 just the same.
	 */
	error = usb2513_write(USB2513_PORTDIS_BUS, 0);
	if (error)
		return error;

	/*
	 * Configure the hub to draw a maximum of 2ma from the upstream
	 * port in self-powered mode.
	 */
	error = usb2513_write(USB2513_MAXPOWER_SELF, USB2513_MAXPOWER_SELF_2MA);
	if (error)
		return error;

	error = usb2513_write(USB2513_MAXPOWER_BUS, USB2513_MAXPOWER_BUS_100MA);
	if (error)
		return error;

	error = usb2513_write(USB2513_HUBC_MAX_I_SELF, USB2513_HUBC_MAX_I_SELF_2MA);
	if (error)
		return error;

	error = usb2513_write(USB2513_HUBC_MAX_I_BUS, USB2513_HUBC_MAX_I_BUS_100MA);
	if (error)
		return error;

	error = usb2513_write(USB2513_POWERON_TIME, USB2513_POWERON_TIME_100MS);
	if (error)
		return error;

	/*
	 * Setup the language ID as English
	 */
	error = usb2513_write(USB2513_LANGID_HIGH, USB2513_LANGID_ENGLISH_US_MSB);
	if (error)
		return error;

	error = usb2513_write(USB2513_LANGID_LOW, USB2513_LANGID_ENGLISH_US_LSB);
	if (error)
		return error;

	/*
	 * Now program the manufacter string
	 */
	error = usb2513_write(USB2513_MFG_STR_LEN, strlen(USB2513_MFR_STRING));
	if (error)
		return error;

	error = i2c_usb2513_write_utf16le(USB2513_MFG_STR_BASE, USB2513_MFR_STRING);
	if (error)
		return error;

	/*
	 * Now program the product string
	 */
	error = usb2513_write(USB2513_PROD_STR_LEN, strlen(USB2513_PROD_STRING));
	if (error)
		return error;

	error = i2c_usb2513_write_utf16le(USB2513_PROD_STR_BASE, USB2513_PROD_STRING);
	if (error)
		return error;

	/*
	 * Now program the serial number
	 */
	error = usb2513_write(USB2513_SER_STR_LEN, strlen(USB2513_SERNO_STRING));
	if (error)
		return error;

	error = i2c_usb2513_write_utf16le(USB2513_SER_STR_BASE, USB2513_SERNO_STRING);
	if (error)
		return error;

	/*
	 * Enable battery charging on all ports
	 */
	error = usb2513_write(USB2513_BAT_CHARG_EN, USB2513_BAT_CHARG_EN_PORT1 |
			USB2513_BAT_CHARG_EN_PORT2 |
			USB2513_BAT_CHARG_EN_PORT3);
	if (error)
		return error;

	/*
	 * Now program all other registers to the default value of 0. This
	 * is important and the hub will fail to work reliably without this.
	 */
        if (IS_BOARD_ACX500(gd->board_type)) {
            for (ind = USB2513_BAT_CHARG_EN + 1; ind <= USB2513_PORT_SWAP; ind++) {
                error = usb2513_write((u_int8_t)ind, 0);
                if (error)
                    return error;
            }
        } else {
            for (ind = USB2513_BAT_CHARG_EN + 1; ind <= USB2513_PORT_MAP7; ind++) {
                error = usb2513_write((u_int8_t)ind, 0);
                if (error)
                    return error;
            }
        }
	/*
	 * Now, trigger an attach event to the host.
	 */
	error = usb2513_write(USB2513_STATCMD, USB2513_STATCMD_USB_ATTACH);
	if (error)
		return error;

	/*
	 * Close the I2C multiplxer for USB hub access
	 */
	tmp = 0x20;
	i2c_write(CFG_I2C_SW1_ADDR, 0, 1, &tmp, 1);

	return 0;
}


int get_unattended_mode(void)
{
	if (nvram->re_side.unattended_mode) {
		setenv("boot_unattended", "1");
		return 1;
	}

	return 0;
}

int get_bios_upgrade(void)
{
    return (nvram->re_side.bios_upgrade);
}

void set_bios_upgrade(uint8_t value)
{
    nvram->re_side.bios_upgrade = value;
}

int is_acx500_board(void)
{
    if (IS_BOARD_ACX500(gd->board_type))
	return 1;
    else
	return 0;
}
