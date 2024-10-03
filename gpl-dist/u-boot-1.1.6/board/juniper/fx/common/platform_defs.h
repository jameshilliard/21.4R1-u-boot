/*
 * Copyright (c) 2010-2013, Juniper Networks, Inc.
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
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>.
 */

#ifndef PLATFORM_DEFS_H
#define PLATFORM_DEFS_H

/*
 * !!!!!!!!!!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 * The following are the LOG EEPROM Offsets. First 256 bytes
 * are reserved for the use of booting and upgrading. There is
 * a similar copy of these offsets in the JUNOS at:
 * dcp/platform/tfxpc/sys/boot/platform_defs.h
 * Please update both the files simultaneously.
 */
#define EEPROM_VALID_KEY_OFFSET         0x00
#define LAST_BOOTED_PARTITION_OFFSET    0x01

/*
 * Now the BOOT STATUS is a 32 bit value with first 16 bits
 * representing the boot request flag and the later representing
 * the boot status flag.
 */
#define SYSTEM_BOOT_STATUS_OFFSET       0x02
#define UPGRADED_PARTITION_OFFSET       0x06
#define UPGRADE_STATUS_OFFSET           0x07
#define BOOT_NUM_TRIES_PART0_OFFSET     0x08
#define BOOT_NUM_TRIES_PART1_OFFSET     0x09
#define UPG_NUM_TRIES_PART0_OFFSET      0x0A
#define UPG_NUM_TRIES_PART1_OFFSET      0x0B
#define KERNEL_SLICE1_NUM_TRIES_OFFSET  0x0C
#define KERNEL_SLICE2_NUM_TRIES_OFFSET  0x0D

/*
 * LAST_BOOT_STATUS_OFFSET maintains the status of the system
 * before it was reset for any reason. This is written just
 * before the SYSTEM_BOOT_STATUS is updated in the current
 * booting.
 */
#define LAST_BOOT_STATUS_OFFSET         0x0E

/*
 * LAST_BOOT_UBOOT_OFFSET holds the last booted U-Boot partition. It is used
 * only by JUNOS and U-Boot does not use this offset except to initialize it
 * on a fresh board.
 */ 
#define LAST_BOOT_UBOOT_OFFSET          0x12

/*
 * Increment this in case new offsets are added.
 * This is used to clear the offsets on a fresh
 * board.
 */
#define MAX_NUM_BOOTING_OFFSETS         0x12

#define TOR_MODE_OFFSET                 0x20

#define POST_NUM_TESTS_OFFSET           0x50
#define POST_STATUS_OFFSET              0x52

/*
 * Reserving an offset to determine if we have to run
 * eye_finder or not. This is the last offset of the
 * EEPROM.
 */
#define FX_EYE_FINDER_OFFSET            0xfd

/*
 * These definitions are used in the Kernel/application later for 
 * display purposes.
 *
 * Read the LOG EEPROM POST test status offset and
 * test strings.
 */
#define POST_UNKNOWN_BOARD_ERROR    15
#define POST_UNKNOWN_BOARD_ERR_STR  "Error: Unknown Board"

/* Test Macros */
#define POST_GMAC_LPBK_ID           0
#define POST_GMAC_LPBK_TEST_STR     "Mac Loopback Test"

#define POST_GMAC_RX_ERROR          1
#define POST_GMAC_RX_ERR_STR        "Error: RX err in Loopback"

#define POST_GMAC_TX_ERROR          2
#define POST_GMAC_TX_ERR_STR        "Error: TX err in Loopback"

#define POST_USB_PROBE_ID           1
#define POST_USB_PROBE_TEST_STR     "USB  Probe Test"

#define POST_USB_PROBE_FAIL         1
#define POST_USB_PROBE_ERR_STR      "Error: USB Probe failed"

#define POST_USB_NEW_DEV            2
#define POST_USB_ND_ERR_STR         "Error: New device found"

#define POST_USB_DEV_ABSENT         3
#define POST_USB_AD_ERR_STR         "Error: Expected Device Absent"

#define POST_DMA_TXR_ID             2
#define POST_DMA_TXR_TEST_STR       "DMA Engine Transfer test"

#define POST_DMA_ERROR1             1
#define POST_DMA_NRM_STR            "Error: Normal DMA Xfer"

#define POST_DMA_ERROR2             2
#define POST_DMA_ERR_STR            "Error: Concurrent DMA Xfer"

#define POST_MULT_CORE_MEM_ID       3
#define POST_MULT_CORE_MEM_TEST_STR "Multi-Core Memory Test"

#define POST_USB_STORAGE_TEST_STR   "USB Storage Test"

#define POST_MULTI_CORE_ERROR       1
#define POST_MULTI_CORE_ERR_STR     "Error: Multi-core Memory Test Fail"

#define POST_I2C_PROBE_ID           4
#define POST_I2C_PROBE_TEST_STR     "Generic I2C Probe Test"

#define POST_I2C_PROBE_ND_ERROR     1
#define POST_I2C_PROBE_ND_ERR_STR   "Error: Expected device not found"

#define POST_I2C_PROBE_AD_ERROR     2
#define POST_I2C_PROBE_AD_ERR_STR   "Error: Unexpected device found"

#define POST_I2C_PROBE_MUX_ERROR    3
#define POST_I2C_PROBE_MUX_ERR_STR  "Error: Mux error during probe"

#define POST_MAX_TEST_ERR_TYPES     5

enum tor_modes {
    TOR_MODE_LA = 0x01,
    TOR_MODE_WAS = TOR_MODE_LA,
    TOR_MODE_PA = 0x02,
    TOR_MODE_WA = TOR_MODE_PA,
    TOR_MODE_SOHO_SIM = 0x03,
    TOR_MODE_UFAB = 0x04,
    TOR_MODE_UNDEFINED = 0xff,
};
#endif
