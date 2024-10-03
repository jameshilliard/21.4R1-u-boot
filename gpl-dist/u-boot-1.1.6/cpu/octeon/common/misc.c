/*
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. if not ,see
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0,html>  
 */

#include <config.h>
#include <common.h>

int
octeon_get_board_major_rev (void)
{
    DECLARE_GLOBAL_DATA_PTR;
    return (gd->board_desc.rev_major);
}

int
octeon_get_board_minor_rev (void)
{
    DECLARE_GLOBAL_DATA_PTR;
    return (gd->board_desc.rev_minor);
}

/* we can have only 3 major revision digits */
#define IS_VALID_VERSION_STRING(str) (str[1] == '.'	\
				      || str[2] == '.'	\
				      || str[3] == '.' ? 1 : 0)
extern const char uboot_api_ver_string[];

/*
 * This function expects a loader with a header containing crc in
 * the beginning. Its purpose is to strip this CRC header from begining
 * and move it to the end.
 */
uint32_t
shuffle_loader_hdr (uint32_t addr, uint32_t ldr_partition_len)
{
    uint32_t ldr_addr, hdr_addr;
    image_header_t header;
    
    header = *((image_header_t *)addr);
    ldr_addr = addr + sizeof(image_header_t);
    memcpy((void *)addr, ldr_addr, ldr_partition_len);
    ldr_addr = addr;
    hdr_addr = addr + ldr_partition_len - sizeof(image_header_t);
    memcpy((void *)hdr_addr, (void *)&header, sizeof(image_header_t));

    return ldr_addr;
}

/*
 * WARNING: This function holds true as long as the image 
 * header for the loader lies at the last *sizeof(image_header_t)* 
 * bytesof the loader partition.
 *
 * So if the fashion in which the loader is being upgraded
 * changes, please revisit this function. Or while upgrading
 * the loader always make sure that the image header is written
 * in the last of the partition.
 */
int
is_valid_loader_image (uint32_t addr, uint32_t partition_len)
{
    uint32_t chksum = 0;
    image_header_t tmp_hdr;
    image_header_t* header = 
        (image_header_t*)(addr + partition_len - sizeof(image_header_t));

    if (header->ih_magic != IH_MAGIC) {
        printf("\nWARNING: Image with missing CRC header.\n");
        return 0;
    }
    
    memcpy(&tmp_hdr, header, sizeof(image_header_t));

    tmp_hdr.ih_hcrc = 0;

    if ((chksum = crc32(0, (uint8_t *)(&tmp_hdr),
                        sizeof(image_header_t))) != header->ih_hcrc) {
        printf("\nWARNING: Image header checksum failure.\n");
        return 0;
    }
    
    if ((chksum = crc32(0, (uint8_t *)(addr), 
                        header->ih_size)) == header->ih_dcrc) {
        return 1;
    }
    
    printf("\nWARNING: Image data checksum failure.\n");
    return 0;
}

uint32_t
get_uboot_version (void)
{
    uint32_t idx = 0;
    uint8_t mjr_digits[4];
    uint32_t major = 0, minor = 0;

    char *ver_string = uboot_api_ver_string;

    if (!IS_VALID_VERSION_STRING(ver_string)) {
	return 0;
    }

    for (idx = 0; idx < 7; idx++) {
	if (ver_string[idx] == '.') {
	    mjr_digits[idx] = '\0';
	    break;
	}
	mjr_digits[idx] = ver_string[idx];
    }
    
    idx++;

    major = simple_strtoul(mjr_digits, NULL, 10);
    minor = simple_strtoul(&ver_string[idx], NULL, 10);

    if (major >= 0 && major <= 255
	&& minor >= 0 && minor <= 255) {
	return ((major << 16) | minor);
    }

    return 0;
}

int
is_valid_uboot_image (uint32_t addr, uint32_t allocated_len,
                      uint8_t validate_size)
{
    uint32_t        chksum = 0;
    image_header_t  tmp_hdr;

    image_header_t *header = 
        (image_header_t *)(addr + IMG_HEADER_OFFSET);

    /* check if the image has a valid magic number */
    if (header->ih_magic != IH_MAGIC) {
        printf("\nWARNING: Image with missing CRC header.\n");
        return 0;
    }

    /* Copy the header to temp and zero out the header
     * checksum field, so that we can verify the header
     * checksum. Only when the header checksum is fine,
     * we can rely on the length of the data field.
     */
    memcpy(&tmp_hdr, header, sizeof(image_header_t));
    
    tmp_hdr.ih_hcrc = 0;

    /* calculate the header checksum */
    if ((chksum = crc32(0, (unsigned char *)(&tmp_hdr), 
                        sizeof(image_header_t))) != header->ih_hcrc) {
        printf("WARNING: Image header checksum failure.\n");
        return 0;
    }

    /* calculate the data checksum */
    if ((chksum = crc32(0, (unsigned char *)(addr + IMG_DATA_OFFSET), 
                        header->ih_size)) == header->ih_dcrc) {

        /* 
         * Check if the size of image can fit in the partition.
         * If the check is being done on the image present in
         * boot flash, the checksum would have already failed.
         * So this case would essentially run when u-boot is
         * being upgraded.
         */
        if (validate_size) {
            if ((header->ih_size + IMG_DATA_OFFSET) > allocated_len) {
                printf("WARNING: Image size greater than allocated.\n");
                return 0;
            }
        }
        
        return 1;
    }
    
    printf("\nWARNING: Image data checksum failure.\n");
    return 0;
}
 
int
bootloader_upgrade (uint32_t addr, uint32_t uboot_start, 
                    uint32_t uboot_end, uint32_t size)
{
    if (flash_sect_protect(0, uboot_start, uboot_end)) {
        printf("Un-protect erase failed\n");
        return 1;
    }
    
    if (flash_sect_erase(uboot_start, uboot_end)) {
        printf("Flash erase failed\n");
        return 1;
    }

    if (flash_write((uchar *) addr, uboot_start, size)) {
        printf("Writing to flash failed\n");
        return 1;
    }

    return 0;
}
