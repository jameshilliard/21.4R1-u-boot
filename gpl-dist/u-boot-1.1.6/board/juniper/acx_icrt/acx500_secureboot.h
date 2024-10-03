/*
 * $Id$
 *
 * acx_secureboot.h -- Platform specific defines for the Juniper ACX
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

#ifndef __ACX500_SECUREBOOT_H__
#define __ACX500_SECUREBOOT_H__

#include "../common/recovery_public_key_info.h"

#define ACX500_ICRT_START  0xfff80000
#define ACX500_ICRT_SIZE   0x80000
#define ACX500_ICRT_END ((ACX500_ICRT_START + ACX500_ICRT_SIZE) - 1)

#define ACX500_UBOOT_START  0xffe80000		    /* after iCRT image */
#define ACX500_UBOOT_SIZE   0x80000
#define	ACX500_UBOOT_END    ((ACX500_UBOOT_START + ACX500_UBOOT_SIZE) - 1)

#define ACX500_UBOOT_ENV_START 0xffe60000
#define ACX500_UBOOT_ENV_SIZE  0x20000

#define ACX500_LOADER_START 0xffd00000
#define ACX500_LOADER_SIZE  0xc0000
#define ACX500_LOADER_END   ((ACX500_LOADER_START + ACX500_LOADER_SIZE) - 1)

#define ACX500_SECUREBLOCK_START 0xffc00000
#define ACX500_SECUREBLOCK_SIZE  0x40000
#define ACX500_SECUREBLOCK_END	 ((ACX500_SECUREBLOCK_START + \
				   ACX500_SECUREBLOCK_SIZE) - 1)

#define ACX_USB_SLICE_NUMBER 0
#define ACX_USB_SUB_PARTITION 0

#define ACX_RECOVERY_IMAGE_NAME "/recover.img"
#define ACX_RAM_ADDR_TO_LOAD_FILE 0x1000000

#define ACX_ICRT_WP_SET 0x8	/* pass 0x8 to to write protect iCRT region */

int acx_tpm_init(void);

/*
 * Capability flag to get capability
 * information from TPM according to
 * sub type value.
 */
#define TPM_CAP_FLAG	0x04

/*
 * Capability Sub Type values
 * Permanent/Volatile type capability data
 */

/*
 * Return the TPM_PERMANENT_FLAGS structure.
 * Each flag in the structure is returned as a byte.
 */
#define	TPM_CAP_FLAG_PERMANENT	0x108

/*
 * Return the TPM_STCLEAR_FLAGS structure.
 * Each flag in the structure is returned as a byte.
 */
#define	TPM_CAP_FLAG_VOLATILE	0x109

/*
 * TPM_CAP_VERSION_VAL -  returns TPM_CAP_VERSION_INFO structure.
 * TPM fills in the structure and returns the information indicating
 * what the TPM currently supports. Sub type is 0x00 for this cap val.
 */
#define TPM_CAP_VERSION_VAL	0x1A

/*
 * Deactivated flag offset in permanent flags data
 */
#define TPM_CAP_DEACTIVATED_OFFSET 0x04

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

extern int get_usb_dev_num(int *dev);

#endif /* __ACX500_SECUREBOOT_H__ */
