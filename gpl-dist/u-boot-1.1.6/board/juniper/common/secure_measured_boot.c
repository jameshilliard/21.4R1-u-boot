/*
 * $Id$
 *
 * secure_measured_boot.c -- Platform specific code for the Juniper ACX
 * Product Family Secure boot code.
 *
 * Venkanna Thadishetty March 2014 
 *
 * Copyright (c) 2014, Juniper Networks, Inc.
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
#include <asm/processor.h>
#include <acx_syspld.h>
#include <tpm.h>
#include <hash.h>
#include <sha1.h>
#include <ffs.h>
#include <exports.h>
#include <vsprintf.h>

DECLARE_GLOBAL_DATA_PTR;

/* Offset from start of U-Boot */
#define ACX500_UBOOT_ENV_OFFSET 0x20000 

/* Size of environment data */
#define ACX500_UBOOT_ENV_SIZE 0x20000

static u8 loader_bank0_hash[SHA1_SUM_LEN];
static u8 loader_bank1_hash[SHA1_SUM_LEN];
static u8 uboot_bank0_hash[SHA1_SUM_LEN];
static u8 uboot_bank1_hash[SHA1_SUM_LEN];

extern void print_byte_string(uint8_t *, size_t);
extern int get_nand_dev_num(int *dev);
extern block_dev_desc_t *usb_stor_get_dev(int index);
extern int flash_write (char *src, ulong addr, ulong cnt);
extern void acx_cache_invalidate_flash(void);
extern void acx_cache_enable_flash(void);

int acx_upgrade_nor(ulong addr, ulong start, ulong length, ulong end)
{
    int ret_val;

    /* Reset flash swizzle counter to give enough time program */
    syspld_swizzle_disable();
    syspld_swizzle_enable();

    /*
     * Un-protect sectors
     */
    ret_val = flash_sect_protect(0, start, end);
    if (ret_val != 0)
        return 1;

    /*
     * Erase the sectors
     */
    printf("\nerasing");
    ret_val = flash_sect_erase(start, end);
    if (ret_val != 0)
	return 1;

    /*
     * Copy the image
     */
    ret_val = flash_write((char *) addr, start, length);
    if (ret_val != 0)
        return ret_val;
    else
	printf("\nDone.\n");

    /*
     * Protect sectors back
     */
    ret_val = flash_sect_protect(1, start, end);

    /* 
     * Ignore the error on protect ON, because we have already
     * programmed NOR Flash
     */
    return 0;
}

int acx_load_file(char *filename, ulong addr,
		  block_dev_desc_t *dev_desc,
		  int part, int subpart)
{
    int error;
    ulong length = 0;

    error = ffs_probe(dev_desc, part, subpart);
    if (error != 0) {
        debug("Error: could not mount device \n");
        return 1;
    }

    /*
     * Get file length, if file exists
     */
    error = ffs_getfilelength(dev_desc, part, subpart, filename, &length);
    if ((error != 0) || (length <= 1024)) {
        printf("Error: file %s not found or length is of not expected\n",
	       filename);
        return 1;
    } else {
	/*
	 * Maximum possible twice the size of
	 * combined iCRT, U-Boot, Loader and SYSPLD image sizes
	 * Otherwise, SYSPLD will trigger the swizzle to other bank.
	 * Restrict the size rather than adding patting of SYSPLD swizzle
	 * time in the file system code to load very large files and taking time
	 * in U-Boot
	 */
	if (length > 0x300000) {
	    printf("\nError!!! ");
	    printf("\nFile %s of size %ld is of not expected size",
		    filename, length);
	    return 1;
	} else {
	    printf("\nFile %s exists and size = %ld ", filename, length);
	    printf("\nLoading file...");
	}
    }

    /* Reset flash swizzle counter to give enough time load file */
    syspld_swizzle_disable();
    syspld_swizzle_enable();

    /*
     * Read the file to RAM address
     */
    error = ffs_read(dev_desc, part, subpart, filename, (char *)addr, length);
    if (error != 0) {
        printf("Error reading %ld bytes from %s", length, filename);
        return 1;
    } else {
        printf("\nLoaded %s to RAM successfully\n", filename);
    }

    return 0;
}

int acx_upgrade_uboot(ulong data, ulong data_size, ulong start, ulong end)
{
    int ret_val;

    printf("\nUpdating U-Boot...\n");

    ret_val = acx_upgrade_nor(data, start, data_size, end);
    if (ret_val == 0) {
	printf("\nU-Boot upgraded successfully");
	setenv("boot.uboot.upgrade", "pass");
    } else {
	printf("\nError!!! - U-Boot not upgraded");
	setenv("boot.uboot.upgrade", "fail");
    }

    return ret_val;
}

int acx_upgrade_loader(ulong data, ulong data_size, ulong start, ulong end)
{
    int ret_val;

    printf("\nUpdating Loader...\n");

    /* Update in NOR flash */
    ret_val = acx_upgrade_nor(data, start, data_size, end);

    if (ret_val == 0) {
	printf("\nLoader upgraded successfully");
	setenv("boot.loader.upgrade", "pass");
    } else {
	printf("\nError!!! - Loader not upgraded");
	setenv("boot.loader.upgrade", "fail");
    }

    return ret_val;
}

int acx_upgrade_icrt(ulong data, ulong data_size, ulong start, ulong end)
{
    int ret_val;

    printf("\nUpdating iCRT...\n");

    /* Update in NOR flash */
    ret_val = acx_upgrade_nor(data, start, data_size, end);

    if (ret_val == 0) {
	printf("\niCRT upgraded successfully");
    } else {
	printf("\nError!!! - iCRT not upgraded");
    }

    return ret_val;
}

int acx_get_fit_image(const void *fit, char *name, ulong *addr, ulong *size)
{
    char *desc;
    const void *data;
    int images_noffset, noffset, ndepth, ret, count;
    size_t data_size;


    /* Find images parent node offset */
    images_noffset = fdt_path_offset(fit, FIT_IMAGES_PATH);
    if (images_noffset < 0) {
	printf("Can't find images parent node '%s' (%s)\n",
		FIT_IMAGES_PATH, fdt_strerror(images_noffset));
	return 0;
    }

    /* Process FIT subnodes checking for images */
    for (ndepth = 0, count = 0,
	 noffset = fdt_next_node(fit, images_noffset, &ndepth);
	 (noffset >= 0) && (ndepth > 0);
	 noffset = fdt_next_node(fit, noffset, &ndepth)) {

	if (ndepth == 1) {
	    ret = fit_get_desc(fit, noffset, &desc);
	    if (!ret) {
		if (desc != NULL) {
		    if (!(strcmp(desc, name))) {
			ret = fit_image_get_data(fit, noffset,
						 &data, &data_size);
			if (!ret) {
			    *addr = (ulong) data;
			    *size = (ulong) data_size;

			    return 1;
			}
		    }
		}
	    }
	}
    }

    return 0;
}

int acx_clear_nor_secure_block(ulong start, ulong end)
{
    if (!(flash_sect_protect (0, start, end))) {
	return (flash_sect_erase (start, end));
    }
    else
	return 1;
}

void acx_hash_image(u8 *output, ulong start, ulong size)
{
    void *buf;

    /*
     * hash the uboot and store the hash
     * in global variable uboot_hash[]
     */
    buf = map_sysmem(start, size);
    sha1_csum_wd(buf, size, output, CHUNKSZ_SHA1);
    unmap_sysmem(buf);
}

void acx_read_and_set_env(int pcr_reg)
{
    char data_val[SHA1_SUM_LEN];
    char temp[SHA1_SUM_LEN * 3];
    uint8_t *pcr_data = (uint8_t *) data_val;
    uint32_t rc, count;

    count = SHA1_SUM_LEN;
    rc = tpm_pcr_read(pcr_reg, pcr_data, count);
    if (rc) {
	printf("\n TPM: error reading pcr %d register!!!\n", pcr_reg);
	return;
    }

    /*
     * Set the environment variable with read value
     */
    for (count=0; count < SHA1_SUM_LEN; count++)
	snprintf(&temp[count*2], (sizeof(char) * 3),
		 "%02x ",
		 data_val[count]);

    switch (pcr_reg) {
	case 0:
	    printf("\nU-Boot0 measurement:\n");
	    setenv("boot.uboot0.extend", temp);
	    break;
	case 1:
	    printf("\nU-Boot1 measurement:\n");
	    setenv("boot.uboot1.extend", temp);
	    break;
	case 2:
	    printf("\nLoader0 measurement:\n");
	    setenv("boot.loader0.extend", temp);
	    break;
	case 3:
	    printf("\nLoader1 measurement:\n");
	    setenv("boot.loader1.extend", temp);
	    break;
	default:
	    break;
    }

    print_byte_string(pcr_data, SHA1_SUM_LEN);
}

int acx_extend_hash(int pcr_reg, uint8_t* hash)
{
    char data_val[SHA1_SUM_LEN];
    uint32_t err=0;

#ifdef DEBUG
    uint32_t count;
    printf("\nMeasuring Loader to PCR reg %d", pcr_reg);
    printf("\n");

    count = SHA1_SUM_LEN;
    for (count=0; count < SHA1_SUM_LEN; count++)
	printf(" %02x", hash[count]);
#endif

    /* Measure the hash to PCR register */
    err = tpm_extend(pcr_reg, (void *) hash, (void *) &data_val[0]);
    if (err)
	return err;

    return 0;
}

void acx500_compare_banks(void)
{
    uint8_t data1_val[SHA1_SUM_LEN];
    uint8_t data2_val[SHA1_SUM_LEN];
    uint32_t rc, count;
    int compare_fail = 0;

    /*
     * Read PCR Register 0 and 1 for comparing iCRT in bank0 & bank1 hashes
     */
    count = SHA1_SUM_LEN;
    rc = tpm_pcr_read(0, &data1_val[0], count);
    if (rc) {
	printf("\n TPM: error reading pcr 0 register!!!\n");
	printf("\nFail...\n");
	return;
    }

    count = SHA1_SUM_LEN;
    rc = tpm_pcr_read(1, &data2_val[0], count);
    if (rc) {
	printf("\n TPM: error reading pcr 1 register!!!\n");
	printf("\nFail...\n");
	return;
    }

    for (count=0; count < SHA1_SUM_LEN; count++)
    {
	if (data1_val[count] != data2_val[count]) {
	    printf("\n U-Boot in both banks does not match!!!");
	    compare_fail = 1;
	    break;
	}
    }

    count = SHA1_SUM_LEN;
    rc = tpm_pcr_read(2, &data1_val[0], count);
    if (rc) {
	printf("\n TPM: error reading pcr 2 register!!!\n");
	printf("\nFail...\n");
	return;
    }

    count = SHA1_SUM_LEN;
    rc = tpm_pcr_read(3, &data2_val[0], count);
    if (rc) {
	printf("\n TPM: error reading pcr 3 register!!!\n");
	printf("\nFail...\n");
	return;
    }

    for (count=0; count < SHA1_SUM_LEN; count++)
    {
	if (data1_val[count] != data2_val[count]) {
	    printf("\n Loader in both banks does not match!!!");
	    compare_fail |= 0x02;
	    break;
	} 	
    }

    if (compare_fail) {
	printf("\nFail...\n");
    } else {
	printf("\nPass...\n");
    }

    return;
}

int acx_measure_loader(ulong start, ulong size)
{
    int bank = 0;
    uint32_t err=0;

    /* Reset flash swizzle counter to give enough time measure to TPM */
    syspld_swizzle_disable();
    syspld_swizzle_enable();

    /* Save current bank booted from */
    bank = syspld_get_logical_bank();

    /* Measure loader image to TPM PCR register */
    syspld_set_logical_bank(0);
    acx_cache_enable_flash();
    acx_hash_image(loader_bank0_hash, start, size);
    acx_cache_invalidate_flash();

    syspld_set_logical_bank(1);
    acx_cache_enable_flash();
    acx_hash_image(loader_bank1_hash, start, size);
    acx_cache_invalidate_flash();

    /* Restore the bank */
    syspld_set_logical_bank(bank);

    /* Extend loader hashes to PCR reg 2 & 3 */
    err = acx_extend_hash(2, loader_bank0_hash);
    if (err)
	return err;

    err = acx_extend_hash(3, loader_bank1_hash);
    if (err)
	return err;

    /*
     * Read PCR registers 0 & 1 and set environment variables
     * for U-Boot measurement of bank0 and bank1 to pass to Kernel
     * as iCRT already executed and it can't pass values to U-Boot
     * as U-Boot jump is through soft reset of CPU.
     */
    acx_read_and_set_env(0);
    acx_read_and_set_env(1);

    /*
     * Read PCR registers 2 & 3 and set environment variables
     * for Loader measurement of bank0 and bank1 to pass to Kernel
     */
    acx_read_and_set_env(2);
    acx_read_and_set_env(3);

    return 0;
}

void acx_measure_bank1_uboot(ulong start, ulong size)
{
    /* Calculate hash */
    acx_cache_enable_flash();
    acx_hash_image(uboot_bank1_hash, start, size);
    acx_cache_invalidate_flash();

    /* Extend to TPM PCR 1 register */
    acx_extend_hash(1, uboot_bank1_hash);
}

void acx_measure_uboot(ulong start, ulong size)
{
    int bank = 0;

    /* Save current bank booted from */
    bank = syspld_get_logical_bank();

    /* Measure uboot image to TPM PCR register */
    syspld_set_logical_bank(0);
    acx_cache_enable_flash();
    acx_hash_image(uboot_bank0_hash, start, size);
    acx_cache_invalidate_flash();

    syspld_set_logical_bank(1);
    acx_cache_enable_flash();
    acx_hash_image(uboot_bank1_hash, start, size);
    acx_cache_invalidate_flash();

    /* Restore the bank */
    syspld_set_logical_bank(bank);

    /* Extend uboot hashes to PCR reg 0 & 1 */
    acx_extend_hash(0, uboot_bank0_hash);
    acx_extend_hash(1, uboot_bank1_hash);
}
