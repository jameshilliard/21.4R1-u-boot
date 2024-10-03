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
#include <exports.h>
#include <vsprintf.h>
#include "acx500_syspld_struct.h"
#include "acx500_secureboot.h"

DECLARE_GLOBAL_DATA_PTR;

extern int acx_get_fit_image(const void *fit, char *name,
			     ulong *addr, ulong *size);
extern int acx_upgrade_loader(ulong data, ulong data_size,
			      ulong start, ulong end);
extern int acx_upgrade_uboot(ulong data, ulong data_size,
			     ulong start, ulong end);
extern int acx_measure_loader(ulong start, ulong size);
extern int acx_load_file(char *filename, ulong addr,
			 block_dev_desc_t *dev_desc,
			 int part, int subpart);
extern void reset_nand_next_slice (void);
extern void acx_measure_bank1_uboot(ulong start, ulong size);
extern void acx_read_and_set_env(int pcr_reg);
extern uint32_t tpm_raw_transfer(uint8_t *command, uint8_t *response, size_t response_length);
extern int tpm_read_int_cert(void *data, size_t count);

int acx_load_file_from_nand(void)
{
    int dev, part, subpart;
    block_dev_desc_t *dev_desc;
    int ret;
    char file[256];

    if (!get_nand_dev_num(&dev)) {
	printf("\n Error getting USB device number..");
	return 1;
    }
    dev_desc = usb_stor_get_dev(dev);
    part = ACX_VAR_SLICE_NUMBER;
    subpart = ACX_VAR_TMP_SUB_PARTITION;
    snprintf(&file[0], sizeof(file), "%s", ACX_UPGRADE_IMAGE_NAME);

    ret = acx_load_file(&file[0], ACX_RAM_ADDR_TO_LOAD_FILE,
			dev_desc, part, subpart);

    return ret;
}

void acx_chk_upgrade(void)
{
    int uboot_not_upgraded = 0xff, loader_upgrade = 0xff;
    int bank = 0;
    void *fit;
    char file[256];
    ulong data, data_size, size;
    volatile uint8_t *src, *dest;

    /* RAM address where FIT loaded, point to it */
    fit = (void *) ACX_RAM_ADDR_TO_LOAD_FILE;

    /*
     * If already upgraded and rebooting, don't do
     * update again
     */
    if (get_bios_upgrade() == 0x1) {

	/* Load file from NAND first */
	if (acx_load_file_from_nand()) {
	    set_bios_upgrade(0x0);
	    return;
	}

	/*
	 * Check if both loader/u-boot images present
	 * otherwise reject image
	 */
	snprintf(&file[0], sizeof(file), "%s", "loader");
	if (acx_get_fit_image(fit, &file[0], &data, &data_size)) {
	    snprintf(&file[0], sizeof(file), "%s", "uboot");
	    if (!(acx_get_fit_image(fit, &file[0], &data, &data_size))) {
		printf("\Error!!!, expecting both U-Boot/Loader images to be present");
		set_bios_upgrade(0x0);
		return;
	    }
	}

	/* Verify the signature etc. first */
	if (fit_all_image_verify(fit)) {


	    /*
	     * Read PCR registers 0 & 1 and display the measurements done by
	     * iCRT for existing U-Boot images.
	     */
	    printf("\nMeasurement values before U-Boot upgrade...");
	    acx_read_and_set_env(0);
	    acx_read_and_set_env(1);

	    /* 
	     * Save current bank booted from, to restore back if no
	     * image upgraded
	     */ 
	    bank = syspld_get_logical_bank();

	    /* Program Bank0 first. */
	    syspld_set_logical_bank(0);

	    printf("\nUpgrading images in Bank0.");

	    /* Get image name, address, length to program to NOR flash */
	    snprintf(&file[0], sizeof(file), "%s", "loader");

	    if (acx_get_fit_image(fit, &file[0], &data, &data_size)) {

		/* Upgrade loader first */
		syspld_set_logical_bank(0);
		loader_upgrade = acx_upgrade_loader(data, data_size,
							ACX500_LOADER_START,
							ACX500_LOADER_END);
	    }

	    /* Upgrade Uboot next in this bank */
	    snprintf(&file[0], sizeof(file), "%s", "uboot");
	    if (acx_get_fit_image(fit, &file[0], &data, &data_size)) {
		uboot_not_upgraded = acx_upgrade_uboot(data, data_size,
						       ACX500_UBOOT_START,
						       ACX500_UBOOT_END);

		/*
		 * If U-Boot is upgraded, reset back to iCRT so that
		 * measurement can take place again
		 */
		if (uboot_not_upgraded == 0x0) {

		    printf("\nSaving Environment data...\n");
		    saveenv();

		    /*
		     * Reset slice back to boot from active slice
		     */
		    reset_nand_next_slice();

		    /*
		     * Set flag to request BIOS upgrade in Bank1.
		     */
		    set_bios_upgrade(0x2);

		    printf("\nResetting board to execute upgraded U-Boot image");
		    printf("\n");
		    syspld_self_reset();
		    do_reset();

		    /* Wait till reset is done */
		    while(1)
			;
		}
	    }

	    /*
	     * If no U-Boot upgrade, and only loader upgrade, upgrade loader
	     * in bank1 also
	     */
	    syspld_set_logical_bank(1);

	    /* Get image name, address, length to program to NOR flash */
	    snprintf(&file[0], sizeof(file), "%s", "loader");
	    if (acx_get_fit_image(fit, &file[0], &data, &data_size)) {

		/* Upgrade loader */
		loader_upgrade = acx_upgrade_loader(data, data_size,
						    ACX500_LOADER_START,
						    ACX500_LOADER_END);
	    }

	    /*
	     * restore bank to boot from
	     */
	    syspld_set_logical_bank(bank);
	} else {
	    printf("\nError!!! Invalid Image....");
	}

	/*
	 * Check if the device has successfully booted from Bank0 and
	 * BIOS upgrade in Bank1 flag is set.
 	 */
    } else if ((get_bios_upgrade() == 0x2) &&
               (syspld_get_logical_bank() == 0x0)) {

            /*
	     * Read PCR registers 0 & 1 and display the measurements done by
	     * iCRT for existing U-Boot images.
	     */
	    printf("\n Measurement values before U-Boot upgrade...");
	    acx_read_and_set_env(0);
	    acx_read_and_set_env(1);

	    printf("Upgrading images in Bank1.\n");

	    src = (uint8_t *) ACX500_UBOOT_START;
	    dest = (uint8_t *) ACX_RAM_ADDR_TO_LOAD_FILE;
	    size = ((ACX500_UBOOT_END - ACX500_UBOOT_START) + 1);
	  
	    /* Read from NOR (bank0) to RAM */
	    while (size-- > 0) {
		*dest++ = *src++;
	    }

	    data = ACX_RAM_ADDR_TO_LOAD_FILE;
	    data_size = ((ACX500_UBOOT_END - ACX500_UBOOT_START) + 1);

	    /* Program it in bank1 U-Boot */
	    syspld_set_logical_bank(1);

	    uboot_not_upgraded = acx_upgrade_uboot(data, data_size,
						       ACX500_UBOOT_START,
						       ACX500_UBOOT_END);

	    printf("\nSaving Environment data...\n");
	    saveenv();

	    /* Read loader image from bank0 */
	    syspld_set_logical_bank(0);

	    /* Copy image to RAM */
	    src = (uint8_t *) ACX500_LOADER_START;
	    dest = (uint8_t *) ACX_RAM_ADDR_TO_LOAD_FILE;
	    size =((ACX500_LOADER_END - ACX500_LOADER_START) + 1);
	   
	    /* Read from NOR (bank0) to RAM */
	    while (size-- > 0) {
		*dest++ = *src++;
	    }

	    /* Program it in bank1 */
	    syspld_set_logical_bank(1);

	    loader_upgrade = acx_upgrade_loader(data, data_size,
						    ACX500_LOADER_START,
						    ACX500_LOADER_END);

	    /* 
	     * Measure upgraded U-Boot in bank1 since
	     * iCRT measured prior to upgrade in PCR1 register
	     */
	    acx_measure_bank1_uboot(ACX500_UBOOT_START,
				    ACX500_UBOOT_END);

	    syspld_set_logical_bank(0);
    }

#ifdef DEBUG
    printf("\nResetting BIOS upgrade flag...\n");
#endif

    /*
     * Reset the flag, so that on next reboot we can 
     * check flag again
     */
    set_bios_upgrade(0x0);
}


void acx_measured_boot(void)
{

    /* Switch off write protection */
    syspld_nor_flash_wp_set(~ACX500_NOR_FLASH_WP_ALL);

    /* Check if we need to upgrade images */
    acx_chk_upgrade();

    /* Disable SYSPLD update, Write Protect NOR Flash */
    syspld_cpld_update(DISABLE);
    syspld_nor_flash_wp_set(ACX500_NOR_FLASH_WP_ALL);

    /* Close the glass safe */
    if (!(syspld_close_glass_safe()))
	printf("\nError!!! Glass Safe is not closed..."); 

    /* Measure loader image to TPM PCR register */
    acx_measure_loader(ACX500_LOADER_START, ACX500_LOADER_SIZE);

#ifdef DEBUG
    printf("\nDone CRTM U-Boot, Now will jump to loader...\n");
#endif

    return;
}

int do_tpm_test(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    unsigned int rc;
    char data_val[0x1ff];
    uint8_t *data = (uint8_t *) data_val;
    int count = 100;
    uint8_t int_cert[2048];
    uint8_t *int_cert_ptr = int_cert;
    uint8_t response[512];
    uint8_t *resp_ptr;
    uint8_t int_cert_loop_bytes;
    int int_cert_bytes;
    int count_cert;
    size_t response_length = sizeof(response);
    uint8_t command[] = { 0x00, 0xc1, 0x00, 0x00, 0x00, 0x16, 0x00,
			  0x00, 0x00, 0xcf, 0x30, 0x00, 0x00, 0x01,
			  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00 };

    printf("\n\nStarting TPM test commands...");
    printf("\nPlease power cycle the board after test is finished...\n\n");


    memset(data_val, 0x0, sizeof(data_val));
    printf("\nTPM: Self Test Full...");
    rc = tpm_self_test_full();
    if (!rc) {
	printf("\nTPM: Command Success\n");
    }

    /*
     * Get TPM_CAP_VERSION_INFO structure contents
     * Capability flag = TPM_CAP_VERSION_VAL
     * Sub Flag = 0x0
     * Refer to TPM Main - Part 2 TPM Structures
     * section 21
     */
    printf("\nTPM: Reading TPM_CAP_VERSION_VAL structure...");
    rc = tpm_get_capability(TPM_CAP_VERSION_VAL, 0x0, data, count);
    if (!rc) {
	printf("\nTPM: Command Success");
	printf("\nTPM: TPM_CAP_VERSION_VAL contents: \n");
	print_byte_string(data, count);
    } else {
	printf("\nTPM Test Error...");
	return 0;
    }


    /*
     * Get TPM_STCLEAR_FLAGS structure contents
     * Capability flag = TPM_CAP_FLAG
     * Sub Flag = TPM_CAP_FLAG_VOLATILE
     */
    memset(data_val, 0x0, sizeof(data_val));
    printf("\nTPM: Reading TPM_CAP_FLAG_VOLATILE structure...");
    rc = tpm_get_capability(TPM_CAP_FLAG, TPM_CAP_FLAG_VOLATILE, data, count);
    if (!rc) {
	printf("\nTPM: Command Success");
        printf("\nTPM: TPM_CAP_FLAG_VOLATILE contents: \n");
        print_byte_string(data, count);
    } else {
	printf("\nTPM Test Error...");
	return 0;
    }

    printf("\nTPM: Enable TPM...");
    rc = tpm_tsc_physical_presence(TPM_PHYSICAL_PRESENCE_PRESENT);
    if (rc) {
	printf("\nTPM Test Error...");
	return 0;
    }

    rc = tpm_physical_enable();
    if (!rc) {
	printf("\nTPM: Command Success");
    }

    printf("\n\nTPM: Clear Physical De-Activated bit");
    rc = tpm_physical_set_deactivated(0);
    if (!rc) {
	printf("\nTPM: Command Success");
    }

    count = 0x500;
    memset(data_val, 0x0, sizeof(data_val));
    printf("\n\nTPM: Read Public EK...");
    rc = tpm_read_pubek(data, count);
    if (!rc) {
	printf("\nTPM: Command Success");
        printf("\nTPM: Public EK: \n");
        print_byte_string(data, count);
    } else {
	printf("\nTPM Test Error...");
	return 0;
    }

    /*
     * Get TPM_PERMANENT_FLAGS structure contents
     * Capability flag = TPM_CAP_FLAG
     * Sub Flag = TPM_CAP_FLAG_PERMANENT
     */
    count = 100;
    memset(data_val, 0x0, sizeof(data_val));
    printf("\nTPM: Reading  TPM_CAP_FLAG_PERMANENT structure...");
    rc = tpm_get_capability(TPM_CAP_FLAG, TPM_CAP_FLAG_PERMANENT, data, count);
    if (!rc) {
        printf("\nTPM: Command Success");
        printf("\nTPM: TPM_CAP_FLAG_PERMANENT contents: \n");
        print_byte_string(data, count);
    } else {
	printf("\nTPM Test Error...");
        return 0;
    }

    /*
     * Check Life time lock bit of physical presence.
     * If not set, set it with option of controlling Physical Presence
     * either through software or hardware.
     * Offset values includes structure ID (2 bytes)
     */
    memset(data_val, 0x0, sizeof(data_val));
    printf("\nTPM: Life Time Lock Bit for PP...");
    if (data[TPM_CAP_PP_LL_OFFSET] == 0x00) {
        rc = tpm_tsc_physical_presence(TPM_PHYSICAL_PRESENCE_HW_ENABLE |
				       TPM_PHYSICAL_PRESENCE_CMD_ENABLE |
				       TPM_PHYSICAL_PRESENCE_LIFETIME_LOCK);
        rc = tpm_get_capability(TPM_CAP_FLAG, TPM_CAP_FLAG_PERMANENT, data, count);
        if (data[TPM_CAP_PP_LL_OFFSET] == 0x00) {
	    printf("\nTPM Test Error...");
	    return 0;
	} else {
	    printf("set.");
	    printf("\nTPM: TPM_CAP_FLAG_PERMANENT contents: \n");
	    print_byte_string(data, count);
	}
    } else {
	printf("set.");
    }

    memset(data_val, 0x0, sizeof(data_val));
    printf("\nTPM: PCR Register 15 contents...");
    count = 20;
    rc = tpm_pcr_read(15, data, count);
    if (!rc) {
	puts("\nTPM: PCR 15 content:\n");
	print_byte_string(data, count);
    } else {
	printf("\nTPM Test Error...");
	return 0;
    }

    memset(data_val, 0x0, sizeof(data_val));
    printf("\nTPM: PCR Register 20 contents...");
    rc = tpm_pcr_read(20, data, count);
    if (!rc) {
	puts("\nTPM: PCR 20 content:\n");
	print_byte_string(data, count);
    } else {
	printf("\nTPM Test Error...");
	return 0;
    }


    /*
     * Read and match with Juniper Intermediate Certificate
     * raw data
     */

    /* Send command in chunks of 256 bytes to read */
    for (int_cert_loop_bytes = 0; int_cert_loop_bytes <=5; int_cert_loop_bytes++)
    {
	command[16] = int_cert_loop_bytes;
	if (int_cert_loop_bytes == 5) {
	    command[20] = 0x0;
	    command[21] = 0x97;
	}

	response_length = sizeof(response);
	rc = tpm_raw_transfer(command, response, response_length);

	if (!rc) {
		resp_ptr = (response + 0x0e);

		if (int_cert_loop_bytes == 5) {
		    response_length = 0x97;
#ifdef DEBUG
		    print_byte_string(resp_ptr, 0x97);
#endif
		} else {
		    response_length = 256;
#ifdef DEBUG
		    print_byte_string(resp_ptr, 256);
#endif
		}

		/* Strip header from response */
		for (count = 0; count < response_length; count++)
		{
		    *int_cert_ptr++ = *resp_ptr++;
                }
        }
    }

    int_cert_bytes = sizeof(jnpr_intermediate_cert);

#ifdef DEBUG
    printf("\n Intermediate certificate size is %d bytes", int_cert_bytes);
#endif

    int_cert_ptr = int_cert;
    for(count_cert = 0; count_cert < int_cert_bytes; count_cert++)
    {
	if (*int_cert_ptr == jnpr_intermediate_cert[count_cert]) {
	    int_cert_ptr++;
	} else {
	    printf("\n Intermediate Certificate mismatch at %d byte", count_cert);
	    printf("\nTPM Test Error...");
	    return 0;
	    break;
	}
    }

    if (count_cert == int_cert_bytes) {
	printf("\nTPM: Juniper 10 year Intermediate Certificate matched ");
    }

    printf("\n\nTPM Test Passed...\n\n\n");

    return 0;
}

U_BOOT_CMD(tpm_test, 1, 1, do_tpm_test,
           "tpm_test - Test TPM\n",
           "Test TPM and display pass/fail\n");


int
do_secure_params (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int i;
    uint8_t reg_val, bitpos;

    switch (gd->board_type) {
	case I2C_ID_ACX500_O_POE_DC:
	case I2C_ID_ACX500_O_POE_AC:
	case I2C_ID_ACX500_O_DC:
	case I2C_ID_ACX500_O_AC:
	case I2C_ID_ACX500_I_DC:
	case I2C_ID_ACX500_I_AC:
	    break;

	default:
	    return 0;
	    break;
    }

    if (argc < 3)
	return CMD_RET_USAGE;

    /*
     * Loop through the table to see which bits need to be enabled/
     * disabled
     */
    for (i = 0; i < (sizeof(secureparams)/sizeof(GcsrBits)); i++)
    {
	if (strcmp(argv[1], secureparams[i].cmd) == 0) {
	    /* Read current register value */
	    reg_val = syspld_reg_read(secureparams[i].reg);

	    printf("\nReg %x = %x", secureparams[i].reg, reg_val);

	    /* Get the spcific bit value */
	    bitpos = (1 << secureparams[i].bitpos);

	    printf("\nbit = %x   value = %x", bitpos, reg_val);

	    /*
	     * Does user want to enable/disable param?
	     */
	    if ((strcmp(argv[2],"yes") == 0)) {
		/* 
		 * Check whether we have to set/clear bit for enable/disable of 
		 * param
		 */
		if (secureparams[i].setclear_enable) {
		    printf("\n yes & setclear=1 ");

		    /* set the bit */
		    reg_val = (reg_val | bitpos);
		} else {
		    printf("\nyes & setclear=0 ");

		    /* clear the bit */
		    reg_val = (reg_val & ~(bitpos));

		    printf("\nreg_val writing = %x ", reg_val);
		}
	    } else if ((strcmp(argv[2],"no") == 0)) {
		/* If set to enable/disable, just do reverse */
		if (secureparams[i].setclear_enable) {
		    printf("\n no & setclear=1 ");
		    /* clear the bit */
		    reg_val = (reg_val & ~bitpos);
		} else {
		    printf("\n no & setclear = 0");
		    /* set the bit */
		    reg_val = (reg_val | bitpos);
		}
	    }
	    printf("\nbit = %x   value = %x", bitpos, reg_val);

	    /* Write to register */
	    syspld_reg_write(secureparams[i].reg, reg_val);

	    reg_val = syspld_reg_read(secureparams[i].reg);
	    printf("\nReg %x = %x\n\n", secureparams[i].reg, reg_val);
	    break;
	}
    }
    return 0;
}


/******************************************************************************/

U_BOOT_CMD(
	    secureparam, 3, 0, do_secure_params,
	    "secureparam    - Set/Clear params in secure block\n",
	    "\n"
	    "secureparam disable-console yes/no\n"
	    "secureparam disable-cpld-update yes/no\n"
	  );
