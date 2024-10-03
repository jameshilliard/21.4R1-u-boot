/*
 * $Id$
 *
 * ex_common.c - platform definitions common for all the EX platforms
 *
 * Copyright (c) 2011-2013, Juniper Networks, Inc.
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
#include <exports.h>
#include <watchdog_cpu.h>
#include <asm/processor.h>
#include "ex_common.h"

#define BOOTSEQ_DEBUG	1

#if BOOTSEQ_DEBUG
#define BSDEBUG(fmt, args...) printf(fmt, ## args)
#else
#define BSDEBUG(fmt, args...)
#endif

#define TRUE		1
#define FALSE		0
#define WDOG_TIMEOUT	20000 /* ~20 sec is max for arm based platforms */

DECLARE_GLOBAL_DATA_PTR;

enum wdog_state {
    WDOG_UNPROGRAMMED,
    WDOG_DISABLED,
    WDOG_ENABLED
};

static enum wdog_state watchdog_state;

#if defined(CONFIG_EX62XX) || defined(CONFIG_EX45XX) || defined(CONFIG_EX4300)
#define EX_CPU_WATCHDOG_INIT(x)
#define EX_CPU_WATCHDOG_ENABLE()	e500_watchdog_enable()
#define EX_CPU_WATCHDOG_DISABLE()	e500_watchdog_disable()
#define EX_CPU_WATCHDOG_RESET() 	e500_watchdog_reset()
#define EX_CPU_WDT_TP			0x21 /* ~50 secs for ppc e500 based platforms */
#define EX_CPU_WDT_MASK         0x3f /* Max timer val for e500 based platforms */
#define TCR_WDTP(x) 			(((x & 0x3) << 30) | ((x & 0x3c) << 15))


/* Enable the e500 watchdog. Set the timeout as specified by EX_CPU_WDT_TP */
void
e500_watchdog_enable(void)
{
    uint32_t val;

    /* clear the status */
    mtspr(SPRN_TSR, TSR_ENW | TSR_WIS); 
    /* now enable the watchdog */
    val = mfspr(SPRN_TCR);
    val |= (TCR_WIE | TCR_WRC(WRC_CHIP) | TCR_WDTP(EX_CPU_WDT_TP)); 
    mtspr(SPRN_TCR, val);
}

/* Pat the e500 watchdog */
void
e500_watchdog_reset(void)
{
    uint32_t reg_data;

    /* Clear TSR(WIS) bit by writing 1 */
    reg_data = (TSR_WIS | TSR_ENW);
    mtspr(SPRN_TSR, reg_data);
}

/* Disable the e500 watchdog */
void
e500_watchdog_disable(void)
{
    uint32_t regdata;

    regdata = mfspr(SPRN_TCR);
	/*
	 * Clearing interrupt-enable and programming
	 * max timer val
	 */	 
    regdata &= ~(TCR_WIE | TCR_WDTP(EX_CPU_WDT_MASK));
    mtspr(SPRN_TCR, regdata);

    /* clear TSR(WIS,ENW) bits */
    regdata = (TSR_WIS | TSR_ENW);
    mtspr(SPRN_TSR, regdata);
}
#endif

int
atoh (char *string)
{
    int res = 0;

    while (*string != 0)
    {
	res *= 16;
	if (*string >= '0' && *string <= '9')
	    res += *string - '0';
	else if (*string >= 'A' && *string <= 'F')
	    res += *string - 'A' + 10;
	else if (*string >= 'a' && *string <= 'f')
	    res += *string - 'a' + 10;
	else
	    break;
	string++;
    }

    return (res);
}

int
atod (char *string)
{
    int res = 0;

    while (*string != 0)
    {
	res *= 10;
	if (*string >= '0' && *string <= '9')
	    res += *string - '0';
	else
	    break;
	string++;
    }

    return (res);
}

int
twos_complement (uint8_t temp)
{
    return ((int8_t)temp);
}

int
twos_complement_16 (uint16_t temp)
{
    return ((int16_t)temp);
}

/**
 * @brief
 * Little endian 16-bit decoding.
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
ismbr(char *sector_buf)
{
    char *p;
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
read_device_mbr (int dev, char *buf)
{
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
static int
get_active_slice (int dev)
{
    char buf[MBR_BUF_SIZE];
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
 * Get the device index from the devices defined
 * in the list. For example, 0 from disk0:, 1
 * from disk1: etc.
 *
 * @param[in] dev_name
 * Device name string from which ID is to be extracted.
 */
int
get_devid_from_device (const char *dev_name)
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

/**
 * @brief
 * ex platform specfic implementation of boot sequencing
 *
 * @param[out] bootdev
 * A 64-byte buffer in which bootdevice name should be copied.
 */
int32_t
ex_get_next_bootdev (char *bootdev)
{

#if defined(CONFIG_EX62XX)
    /* On LCPU, bootsequencing doesn't make any sense, so just return */
    if (EX62XX_LC)
	return 0;
#endif

    char *loaddev;
    char *btsuccess;
    char *btsq = NULL;
    char diskname[64];
    char slice_buffer[8];
    int device_id;
    int slice;
    int new_slice;
    int btsq_disabled = 1; /* By default boot seqencing is disabled */

    loaddev = getenv("loaddev");

    /* check if user has enabled boot sequencing */
    btsq = getenv(BOOTSEQ_DISABLE_ENV);
    if (btsq != NULL)
	btsq_disabled = simple_strtoul(btsq, NULL, NBASE_DECIMAL);

    if (btsq_disabled) {
        /*
         * boot sequencing is disabled. don't mess up with the
         * natural boot sequence in this case.
         */
        BSDEBUG("bootsequencing is disabled\n");
        if (loaddev != NULL) {
                strcpy(bootdev, loaddev);
        }
    } else {
        BSDEBUG("bootsequencing is enabled\n");
        btsuccess = getenv("boot.success");
        if (btsuccess && !strncmp(btsuccess, "1", 1)) {
            /*
             * bootsuccess variable exists and set to 1. again don't
             * mess up with the natural boot sequence in this case.
             */
            BSDEBUG("bootsuccess is set\n");
            if (loaddev != NULL) {
                    strcpy(bootdev, loaddev);
            }
        } else {
            BSDEBUG("bootsuccess is not set\n");
            /*
             * bootsuccess variable exists but not set to 1, which means
             * last boot from current boot slice was not successful. we
             * need to boot from alternate slice in this case. if boot
             * slice number is provided in the loaddev variable, then
             * flip the slice to get the new boot slice.
             */
            if (loaddev && !strncmp(loaddev, "disk0s2", 7)) {
                strcpy(bootdev, "disk0s1:");
            } else if (loaddev && !strncmp(loaddev, "disk0s1", 7)) {
                strcpy(bootdev, "disk0s2:");
            } else {
                /* 
                 * if no slice number is supplied, then by default we always
                 * boot from active slice of the disk. get the active slice
                 * and flip the slice to boot from alternate slice. before
                 * getting the active slice, we need to find the boot device
                 */
                device_id = get_devid_from_device(loaddev);
                if (device_id < 0) {
                    /* 
                     * if no device id supplied, then we always boot from 
                     * internal disk (i.e. disk0)
                     */
                    device_id = 0;
                }
                sprintf(diskname, "disk%d", device_id);
                /* 
                 * if we're booting from internal disk, only then flip 
                 * the slice to get new boot slice. don't change the boot
                 * slice in case of external disks.
                 */
                if ((device_id == 0) && 
                    ((slice = get_active_slice(device_id)) > 0)) {
                    /* 
                     * flip the slice. if current slice is 1 then make it 
                     * 2. if current slice is 2 then make it 1 
                     */
                    new_slice = 3 - slice;
                    BSDEBUG("old boot slice = %d, new boot slice = %d\n", 
                        slice, new_slice);
                    /* append slice information to device */
                    sprintf(slice_buffer, "s%d:", new_slice);
                    strcat(diskname, slice_buffer);
                }
                strcpy(bootdev, diskname);
            }
        }
        /* reset the bootsuccess nvram variables */
        setenv("boot.success", "0");
    }
    BSDEBUG("new boot device = %s\n", bootdev);
    setenv("loaddev", bootdev);

    gd->flags |= GD_FLG_SILENT;
    saveenv();
    gd->flags &= ~GD_FLG_SILENT;

    return 0;
}

/*
 * Implement the watchdog functionality. We can not use the external (syspld/
 * btcpld) watchdog as the max timeout with those are in order of msec, while we
 * need a timeout in order of seconds. Hence we use the cpu's internal watchdog.
 */
void
ex_reload_watchdog (int32_t val)
{
    char *loaddev;

#if defined(CONFIG_EX62XX)
    /* 
     * On 62XX_LCPU, we do not want to mess up watchdog as bootsequencing
     * doesn't make any sense on 82XX_LCPU
     */
    if (EX62XX_LC)
	return;
#endif

    /*
     * First things first. We don't want to mess up with watchdog if we're
     * to boot from external usb. It can take really long to boot from 
     * external usb and watchdog timer would always expire in that case. 
     */
    loaddev = getenv("loaddev");
    if (loaddev && !strncmp(loaddev, "disk1", 5)) {
        return;
    }

    switch (val) {
    case ENABLE_WATCHDOG:
        if (watchdog_state == WDOG_UNPROGRAMMED) {
            /*
             * We have to chose a very large timeout. This is to avoid wdog 
             * from biting when the loader is still trying to load the kernel.
             * So lets be ultra conservation here by chosing a timeout of ~50
             * seconds (~20 sec for arm based platforms whenever support for
             * arm added in future, as that is the max available on those
             * platforms) . This is enough to avoid false watchdog triggers.
             * If the loader is stuck for this long, then it really means
             * something is wrong, and we need to reset the cpu in order to 
             * recover.
             */
            EX_CPU_WATCHDOG_INIT(WDOG_TIMEOUT);
            EX_CPU_WATCHDOG_ENABLE();
            watchdog_state = WDOG_ENABLED;
        } else if (watchdog_state == WDOG_DISABLED) {
            EX_CPU_WATCHDOG_ENABLE();
            watchdog_state = WDOG_ENABLED;
        }
        EX_CPU_WATCHDOG_RESET();
        break;
    case PAT_WATCHDOG:
        /*
         * The difference b/w ENABLE and PAT here is that PAT would not do
         * anything if the watchdog has been explicitly disabled earlier.
         */
        if (watchdog_state == WDOG_UNPROGRAMMED) {
            EX_CPU_WATCHDOG_INIT(WDOG_TIMEOUT);
            EX_CPU_WATCHDOG_ENABLE();
            watchdog_state = WDOG_ENABLED;
            EX_CPU_WATCHDOG_RESET();
        } else if (watchdog_state == WDOG_ENABLED) {
            EX_CPU_WATCHDOG_RESET();
        }
        break;
    case DISABLE_WATCHDOG:
        if (watchdog_state == WDOG_ENABLED) {
            EX_CPU_WATCHDOG_DISABLE();
            watchdog_state = WDOG_DISABLED;
        }
        break;
    default:
        /* Do Nothing */
        break;
    }
}

