
/*
 * Copyright (c) 2009-2010, Juniper Networks, Inc.
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
 */

#include <common.h>

/*
 * CRC test of active u-boot.
 */

#ifdef CONFIG_POST

#include <post.h>
#include "post_jsrxnle.h"

#if CONFIG_POST & CFG_POST_UBOOT_CRC

static int
is_valid_uboot_image (uint32_t addr)
{
    uint32_t chksum = 0;
    image_header_t tmp_hdr;

    image_header_t *header = (image_header_t *)(addr + IMG_HEADER_OFFSET);

    /* check if the image has a valid magic number */
    if (header->ih_magic != IH_MAGIC) {
        printf("\nERROR: U-Boot image with missing CRC header!\n");
        return 0;
    }

    /* 
     * Copy the header to temp and zero out the header
     * checksum field, so that we can verify the header
     * checksum. Only when the header checksum is fine,
     * we can rely on the length of the data field.
     */
    memcpy(&tmp_hdr, header, sizeof(image_header_t));
    
    tmp_hdr.ih_hcrc = 0;

    /* calculate the header checksum */
    if ((chksum = crc32(0, (unsigned char *)(&tmp_hdr), 
                        sizeof(image_header_t))) != header->ih_hcrc) {
        printf("\nERROR: U-Boot image header checksum failure!\n");
        return 0;
    }

    /* calculate the data checksum */
    if ((chksum = crc32(0, (unsigned char *)(addr + IMG_DATA_OFFSET), 
                        header->ih_size)) == header->ih_dcrc) {

	return 1;
    } else {   
	printf("\nERROR: Image data checksum failure.\n");
    }

    return 0;
}

int 
uboot_crc_post_test (int flags)
{
    DECLARE_GLOBAL_DATA_PTR;
    uint32_t uboot_start;
    uint8_t cpld_reg_val;

    switch (gd->board_desc.board_type) {
    CASE_ALL_SRXSME_4MB_FLASH_MODELS
        uboot_start = CFG_PRIMARY_UBOOT_START;
        cpld_reg_val = cpld_read(SRX210_CPLD_MISC_REG);
        if (cpld_reg_val & SRX210_CPLD_BOOT_SEC_SEL_MASK) {
            uboot_start = CFG_SECONDARY_UBOOT_START;
        }
        break;
    CASE_ALL_SRXSME_8MB_FLASH_MODELS
        uboot_start = CFG_8MB_FLASH_PRIMARY_UBOOT_START;
        break;
    default:
        return 0;
    }

    if (!is_valid_uboot_image(uboot_start)) {
        SET_POST_RESULT(gd->post_result, UBOOT_CRC_POST_RESULT_MASK,
                        POST_FAILED);
        return -1;
    } else {
        /* set result */
        SET_POST_RESULT(gd->post_result, UBOOT_CRC_POST_RESULT_MASK,
                        POST_PASSED);
    }

    return 0;
}

#endif /* CONFIG_POST & CFG_POST_UBOOT_CRC */
#endif /* CONFIG_POST */ 

