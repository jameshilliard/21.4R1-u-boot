/* 
 * $Id$
 *
 * acx_secureboot.c -- Platform specific code for the Juniper ACX
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
#include <acx_syspld.h>
#include <tpm.h>
#include <hash.h>
#include <sha1.h>
#include <ffs.h>
#include <vsprintf.h>
#include <exports.h>
#include "acx500_syspld_struct.h"
#include "acx500_secureboot.h"

DECLARE_GLOBAL_DATA_PTR;

extern u32 vendor_dev_id;
extern int acx_load_file(char *filename, ulong addr,
		  block_dev_desc_t *dev_desc,
		  int part, int subpart);
extern block_dev_desc_t *usb_stor_get_dev(int index);
extern int acx_get_fit_image(const void *fit, char *name,
			     ulong *addr, ulong *size);
extern int acx_upgrade_loader(ulong data, ulong data_size,
			      ulong start, ulong end);
extern int acx_upgrade_uboot(ulong data, ulong data_size,
			     ulong start, ulong end);
extern int acx_upgrade_icrt(ulong data, ulong data_size,
			    ulong start, ulong end);
extern void acx_measure_uboot(ulong start, ulong size);
extern void print_byte_string(uint8_t *data, size_t count);

int acx_load_file_from_usb(void)
{
    int dev, part, subpart;
    block_dev_desc_t *dev_desc;
    int ret;
    char file[256];

    if (!get_usb_dev_num(&dev)) {
	printf("\n Error getting USB device number..");
	return 1;
    }

    dev_desc = usb_stor_get_dev(dev);
    part = ACX_USB_SLICE_NUMBER;
    subpart = ACX_USB_SUB_PARTITION;

    snprintf(&file[0], sizeof(file), "%s", ACX_RECOVERY_IMAGE_NAME);

    ret = acx_load_file(&file[0], ACX_RAM_ADDR_TO_LOAD_FILE,
			dev_desc, part, subpart);

    return ret;
}

int acx_upgrade_images(void)
{
    int uboot_upgrade = 0xff, loader_upgrade = 0xff; 
    int bank = 0;
    int icrt_upgrade = 0xff;
    void *fit;
    char file[256];
    ulong data, data_size;
    int ret_val = 0;

    /* RAM address where FIT is loaded */
    fit = (void *) ACX_RAM_ADDR_TO_LOAD_FILE;

    /* Load FIT file from usb first */
    if (acx_load_file_from_usb()) {
	return 0;
    }

    if (fit_all_image_verify(fit)) {

	    /* Save current bank booted from */
	    bank = syspld_get_logical_bank();

            /* Get image name, address, length to program to NOR flash */
            /* Upgrade loader first */
            snprintf(&file[0], sizeof(file), "%s", "loader");
            if (acx_get_fit_image(fit, &file[0], &data, &data_size)) {

		syspld_set_logical_bank(0);
		loader_upgrade = acx_upgrade_loader(data, data_size,
						    ACX500_LOADER_START,
						    ACX500_LOADER_END);


		syspld_set_logical_bank(1);
		loader_upgrade = acx_upgrade_loader(data, data_size,
						    ACX500_LOADER_START,
						    ACX500_LOADER_END);

		/* Set 2 for loader upgrade */
		ret_val = 2;
            }

            /* Upgrade Uboot next in this bank */
            snprintf(&file[0], sizeof(file), "%s", "uboot");
            if (acx_get_fit_image(fit, &file[0], &data, &data_size)) {

		syspld_set_logical_bank(0);
                uboot_upgrade = acx_upgrade_uboot(data, data_size,
						  ACX500_UBOOT_START,
						  ACX500_UBOOT_END);

		syspld_set_logical_bank(1);
		uboot_upgrade = acx_upgrade_uboot(data, data_size,
						  ACX500_UBOOT_START,
						  ACX500_UBOOT_END);

		/* Set to 1 for U-Boot upgrade */
		ret_val |= 1;
	    }

	    /* Upgrade iCRT next in this bank */
	    snprintf(&file[0], sizeof(file), "%s", "icrt");
	    if (acx_get_fit_image(fit, &file[0], &data, &data_size)) {

		syspld_set_logical_bank(0);
		icrt_upgrade = acx_upgrade_icrt(data, data_size,
						ACX500_ICRT_START,
						ACX500_ICRT_END);

		syspld_set_logical_bank(1);
		icrt_upgrade = acx_upgrade_icrt(data, data_size,
						ACX500_ICRT_START,
						ACX500_ICRT_END);

		/* Set 4 for iCRT upgrade */
		ret_val |= 4;
	    }

	    /* Restore the bank */
            syspld_set_logical_bank(bank);
    }

    return ret_val;
}

void acx_tpm_chk_deactivated(void)
{
    uint8_t data[50];
    unsigned int rc;
    int count = 50;

    /*
     * Get TPM_PERMANENT_FLAGS structure contents
     * Capability flag = TPM_CAP_FLAG
     * Sub Flag = TPM_CAP_FLAG_PERMANENT
     */
    rc = tpm_get_capability(TPM_CAP_FLAG, TPM_CAP_FLAG_PERMANENT, data, count);
    if (rc) {
        printf("\nTPM: error getting TPM_CAP_FLAG_PERMANENT contents!!!\n");
	return;
    }

    /*
     * Enable TPM first to execute the required commands
     */
    rc = tpm_tsc_physical_presence(TPM_PHYSICAL_PRESENCE_PRESENT);
    rc = tpm_physical_enable();

    if (data[TPM_CAP_DEACTIVATED_OFFSET] == 0x01) {
        rc = tpm_physical_set_deactivated(0);
	rc = tpm_get_capability(TPM_CAP_FLAG, TPM_CAP_FLAG_PERMANENT, data, count);
	if (data[TPM_CAP_DEACTIVATED_OFFSET] == 0x01) {
	    printf("\nError!!!, clearing deactivated flag\n");
	} else {
	    printf("\nResetting board to activate TPM...\n");
	    do_reset();
	}
    }
}

int acx_tpm_init(void)
{
    unsigned int rc;
#if TPM_DEBUG
    char data_val[600];
    uint8_t *data = (uint8_t *) data_val;
#endif

    /* 
     * Reset flash swizzle counter to give enough time 
     * for tpm init etc. If TPM init is taking more time
     * in case of error scenarios this will give enough
     * time to bail out.
     */
    syspld_swizzle_disable();
    syspld_swizzle_enable();

    rc = tpm_init();
    if (!rc) {
	rc = tpm_startup(TPM_ST_CLEAR);
	if (!rc) {
	    rc = tpm_self_test_full();
	    if (!rc) {

		/*
		 * Ignore error as life time lock already set, but if it is
		 * not make sure this command is executed, and in case of error
		 * next command will fail
		 */
		rc =
		    tpm_tsc_physical_presence(TPM_PHYSICAL_PRESENCE_CMD_ENABLE);
		acx_tpm_chk_deactivated();
		printf("TPM initialized\n");
	    } else {
		printf("TPM: Self Test Error.\n");
		return 1;
	    }
	} else {
	    printf("TPM: Startup Error.\n");
	    return 1;
	}
    } else {
	printf("TPM: Init Error.\n");
	return 1;
    }

#if TPM_DEBUG

    /*
     * Get TPM_CAP_VERSION_INFO structure contents
     * Capability flag = TPM_CAP_VERSION_VAL
     * Sub Flag = 0x0
     * Refer to TPM Main - Part 2 TPM Structures
     * section 21
     */
    int count = 100;
    rc = tpm_get_capability(TPM_CAP_VERSION_VAL, 0x0, data, count);
    if (!rc) {
	printf("\nTPM: TPM_CAP_VERSION_VAL contents: \n");
	print_byte_string(data, count);
    } else {
	printf("\nTPM: error getting TPM_CAP_VERSION_VAL contents!!!\n");
    }

    /*
     * Get TPM_PERMANENT_FLAGS structure contents
     * Capability flag = TPM_CAP_FLAG
     * Sub Flag = TPM_CAP_FLAG_PERMANENT
     */
    rc = tpm_get_capability(TPM_CAP_FLAG, TPM_CAP_FLAG_PERMANENT, data, count);
    if (!rc) {
        printf("\nTPM: TPM_CAP_FLAG_PERMANENT contents: \n");
        print_byte_string(data, count);
    } else {
        printf("\nTPM: error getting TPM_CAP_FLAG_PERMANENT contents!!!\n");
    }

    /*
     * Get TPM_STCLEAR_FLAGS structure contents
     * Capability flag = TPM_CAP_FLAG
     * Sub Flag = TPM_CAP_FLAG_VOLATILE
     */
    rc = tpm_get_capability(TPM_CAP_FLAG, TPM_CAP_FLAG_VOLATILE, data, count);
    if (!rc) {
        printf("\nTPM: TPM_CAP_FLAG_VOLATILE contents: \n");
        print_byte_string(data, count);
    } else {
        printf("\nTPM: error getting TPM_CAP_FLAG_VOLATILE contents!!!\n");
    }

    count = 512;
    rc = tpm_read_pubek(data, count);
    if (!rc) {
	printf("\nTPM: Public EK: \n");
	print_byte_string(data, count);
    } else {
	printf("\nTPM: error reading Public EK!!!\n");
    }

    printf("\nEnable TPM...");
    rc = tpm_tsc_physical_presence(TPM_PHYSICAL_PRESENCE_PRESENT);
    if (rc)
	printf("Error!!!");

    rc = tpm_physical_enable();
    if (!rc)
	printf("Done");
#endif

    return 0;
}

void acx_icrt_uboot_jump(void)
{
    unsigned int i;
    int rc;

    i = syspld_reg_read(0x19);
    if (i & 0x01) {
	printf("\nError in iCRT execution...\n");
    } else {
	/* Initialize the TPM Device */
	rc = acx_tpm_init();

	/* Measure UBOOT image to TPM PCR register */
	if (!rc)
	    acx_measure_uboot(ACX500_UBOOT_START, ACX500_UBOOT_SIZE);

#ifdef DEBUG
	printf("\nJumping to CRTM U-Boot\n\n");
#endif
	/* Write protect NOR Flash iCRT region */
	syspld_nor_flash_wp_set(ACX_ICRT_WP_SET);

	syspld_reg_write(0x19, 0x01);
	while (1)
	    ;

    }

    return;
}
