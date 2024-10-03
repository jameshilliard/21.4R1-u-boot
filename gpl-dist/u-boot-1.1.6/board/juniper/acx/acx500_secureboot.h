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

#include "../common/upgrade_public_key_info.h"

#define ACX500_UBOOT_START  0xfff80000	   /* after soft reset */
#define ACX500_UBOOT_SIZE   0x80000
#define ACX500_UBOOT_END    ((ACX500_UBOOT_START + ACX500_UBOOT_SIZE) - 1)

#define ACX500_UBOOT_ENV_START 0xfff60000
#define ACX500_UBOOT_ENV_SIZE 0x20000

#define ACX500_LOADER_START 0xffe00000
#define ACX500_LOADER_SIZE  0xc0000
#define ACX500_LOADER_END ((ACX500_LOADER_START + ACX500_LOADER_SIZE) - 1)

#define ACX500_SECUREBLOCK_START 0xffd00000
#define ACX500_SECUREBLOCK_SIZE 0x40000
#define ACX500_SECUREBLOCK_END ((ACX500_SECUREBLOCK_START + ACX500_SECUREBLOCK_SIZE) - 1)

#define ACX_VAR_SLICE_NUMBER 3                  /* da0s3  */
#define ACX_VAR_TMP_SUB_PARTITION 5             /* da0s3f */
#define ACX_UPGRADE_IMAGE_NAME "tmp/upgrade.img" /* file to load from NAND */
#define ACX_RAM_ADDR_TO_LOAD_FILE 0x1000000

#define ACX500_NOR_FLASH_WP_ALL 0x0F
#define DISABLE 1

extern int get_nand_dev_num(int *dev);
extern block_dev_desc_t *usb_stor_get_dev(int index);

typedef enum imageType
{
    ACX_UBOOT_IMAGE,
    ACX_LOADER_IMAGE,
    ACX_SYSPLD_IMAGE,
    ACX_SECURE_BLOCK_IMAGE
} NorImages;

typedef struct {
    char cmd[20];
    uint8_t reg;
    uint8_t bitpos;
    uint8_t setclear_enable;
} GcsrBits;

#define misc 0x24
#define glass_safe_control 0x18
#define uart0_tx_rx_enable 0x1b

GcsrBits secureparams[] = {
    /* cmd name, syspld register, bit number, 0/1 for functionality */
    {"disable-console", misc,
	(KOTINOS_RE_REGS_MISC_RS0_SHUTDN_L_LSB << 1), 0},
    {"disable-cpld-update", glass_safe_control,
	(KOTINOS_RE_REGS_GLASS_SAFE_CONTROL_BOOTCPLD_UPDATE_LSB << 1), 1},
    {"disable-pp-pin", glass_safe_control,
	(KOTINOS_RE_REGS_GLASS_SAFE_CONTROL_TPM_PP_PIN_CONTROL_LSB << 1), 0},
    {"disable-uart0-tx", uart0_tx_rx_enable,
	(KOTINOS_RE_REGS_UART0_TX_RX_ENABLE_UART0_TX_ENABLE_LSB << 1), 0},
    {"disable-uart0-rx", uart0_tx_rx_enable,
	(KOTINOS_RE_REGS_UART0_TX_RX_ENABLE_UART0_RX_ENABLE_LSB << 1), 0}
};

/*
 * Capability flag to get capability
 * information from TPM according to
 * sub type value.
 */
#define TPM_CAP_FLAG    0x04

/*
 * Capability Sub Type values
 * Permanent/Volatile type capability data
 */

/*
 * Return the TPM_PERMANENT_FLAGS structure.
 * Each flag in the structure is returned as a byte.
 */
#define TPM_CAP_FLAG_PERMANENT  0x108

/*
 * Return the TPM_STCLEAR_FLAGS structure.
 * Each flag in the structure is returned as a byte.
 */
#define TPM_CAP_FLAG_VOLATILE   0x109

/*
 * TPM_CAP_VERSION_VAL -  returns TPM_CAP_VERSION_INFO structure.
 * TPM fills in the structure and returns the information indicating
 * what the TPM currently supports. Sub type is 0x00 for this cap val.
 */
#define TPM_CAP_VERSION_VAL     0x1A

/*
 * Deactivated flag offset in permanent flags data
 */
#define TPM_CAP_DEACTIVATED_OFFSET 0x04

/*
 * Life Time Lock bit for PP offset (including 2 bytes structure type)
 */
#define TPM_CAP_PP_LL_OFFSET 0x08

extern void syspld_cpu_reset(void);
extern void set_bios_upgrade(uint8_t value);
extern int get_bios_upgrade(void);
extern void acx_cache_invalidate_flash(void);
extern void acx_cache_enable_flash(void);

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
extern void print_byte_string(uint8_t *data, size_t count);
