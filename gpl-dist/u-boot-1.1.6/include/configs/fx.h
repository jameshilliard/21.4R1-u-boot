/*
 * Copyright (c) 2009-2013, Juniper Networks, Inc.
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
 * You should have received a copy of the GNU General Public
 * License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef FX_H
#define FX_H

#include "rmi_boards.h"

#ifdef CONFIG_FX

#define CONFIG_AUTOBOOT_KEYED    /* use key strings to stop autoboot */
#define CFG_CONSOLE_INFO_QUIET            1
/*
 * Use ctrl-c to stop booting and enter u-boot command mode.
 */
#define CONFIG_AUTOBOOT_STOP_STR        "\x03"
    
#define CONFIG_ENV_OVERWRITE              1
#define CONFIG_VERSION_VARIABLE           1

#define RELOAD_WATCHDOG_SUPPORT           1
#define CFG_FLASH_CFI
#define CFG_FLASH_CFI_DRIVER              1
#define CFG_ENV_OFFSET                    0x3F0000
#define CONFIG_USB_SHOW_NO_PROGRESS
#define CFG_ENV_ADDR                      (0xBC000000 + CFG_ENV_OFFSET)
#define CFG_ENV_SECT_SIZE                 0x10000
#define CFG_ENV_SIZE                      0x10000
#define CFG_PROMPT                        "QFX-XXXX #"
#define CFG_MAX_FLASH_BANKS               1 
#define CFG_64BIT_VSPRINTF                1
#define CFG_64BIT_STRTOUL                 1

#define USB_WRITE_READ 

/* Use the HUSH parser */
#define CFG_HUSH_PARSER
#ifdef  CFG_HUSH_PARSER
#define CFG_PROMPT_HUSH_PS2               CFG_PROMPT
#endif

#define FX_EEPROM_BUS                     0x00
#define FX_BOARD_EEPROM_ADDR              0x51
#define FX_MGMT_BOARD_EEPROM_I2C_ADDR     0x53
#define FX_MAC_ADDR_MAGIC_CODE            0xAD

#define FX_BOARD_ID_OFFSET                0x04
#define FX_BOARD_VERSION_OFFSET           0x06
#define FX_MAC_ADDR_MAGIC_CODE_OFFSET     0x34
#define FX_MAC_ADDR_COUNT_OFFSET          0x36
#define FX_MAC_ADDR_BASE_OFFSET           0x38

#define FX_MAJOR_REV_OFFSET               0x06

#define FX_IO_EXPANDER_BUS                0x01
#define FX_IO_EXPANDER_ADDR               0x38

#define FX_IO_EXPANDER_FPGA_ASSERT_RST_OFFSET    0x1
#define FX_IO_EXPANDER_FPGA_DEASSERT_RST_OFFSET  0x3

#define FX_EYE_FINDER_OFFSET              0xfd

#define CONFIG_MISC_INIT_R

/* Offsets used for upgrades */
#define CFG_UPG_LOADER_PARTITION0_ADDR    0xBC200000
#define CFG_UPG_LOADER_PARTITION1_ADDR    0xBC600000
#define CFG_FLASH_BASE_PARTITION1         0xBC400000
#define CFG_FLASH_BASE_PARTITION0         CFG_FLASH_BASE
#define CONFIG_BOOTCOMMAND_PARTITION0 \
               "bootelf 0xbc200000"
#define CONFIG_BOOTCOMMAND_PARTITION1 \
	       "bootelf 0xbc600000"

#define CFG_POST_VERBOSE      0x1

/*THough the flash is divided into two 4MB partitions, for upgrade the
 * base is considered 0xbc000000 and the full 8MB of the flash is open.
 */
#define CFG_FLASH_SIZE                    0x800000 


/*
 * The Following are the Chip Select values for setting up
 * access to the BOOT CPLD. These values are only valid
 * before the base address is re-based to 0xbc000000.
 * Once it is rebased, the CPLD is re-initialized with a
 * different base address and mask. Those values are populated
 * into a device address map in fx_piobus_setup().
 */

#define BOOT_CPLD_CHIP_SEL                0x01
#define BOOT_CPLD_CS_BASE_ADDR_OFFSET     0xB0
#define BOOT_CPLD_CS_MASK                 0x3F

#define CFG_VERSION_MAJOR                 1
#define CFG_VERSION_MINOR                 1
#define CFG_VERSION_PATCH                 8

#define LOG_EEPROM_I2C_BUS              g_log_eprom_bus
#define LOG_EEPROM_I2C_ADDR             0x57
#define LOG_EEPROM_DEV_WORD_LEN         0x02

#define CFG_WA_MGMT_ETHER_NAME            "me6"

/* Board IDs and TOR MODE for QFX Series */
#define QFX3600_M_BOARD_ID                0x09a4
#define QFX3600_M_BOARD_ID_AFO            0x0B76
#define QFX3600_SE_BOARD_ID_AFI           0x0B71
#define QFX3600_SE_BOARD_ID_AFO           0x0B72
#define QFX5500_BOARD_ID                  0x09a2
#define WESTERNAVENUE_BOARD_ID            0x09a4
#define QFX3500_BOARD_ID                  0x09a5
#define QFX_CONTROL_BOARD_ID              0x053c
#define WESTERNAVENUE_SE_BOARD_ID_AFI     0x0B71
#define WESTERNAVENUE_SE_BOARD_ID_AFO     0x0B72
#define WESTERNAVENUE_BOARD_ID_AFO        0x0B76

#define CASE_UFABRIC_WESTERNAVENUE_SERIES \
    case WESTERNAVENUE_BOARD_ID:          \
    case WESTERNAVENUE_BOARD_ID_AFO

#define CASE_SE_WESTERNAVENUE_SERIES      \
    case WESTERNAVENUE_SE_BOARD_ID_AFI:   \
    case WESTERNAVENUE_SE_BOARD_ID_AFO

#define PRODUCT_IS_OF_UFABRIC_WA_SERIES(q)  \
    ((q) == WESTERNAVENUE_BOARD_ID       || \
     (q) == WESTERNAVENUE_BOARD_ID_AFO)

#define PRODUCT_IS_OF_SE_WA_SERIES(r)       \
    ((r) == WESTERNAVENUE_SE_BOARD_ID_AFI ||\
     (r) == WESTERNAVENUE_SE_BOARD_ID_AFO)

#define QFX3500_RJ45_MGMT_BOARD_ID        0x09da
#define QFX3500_SFP_MGMT_BOARD_ID         0x0b3f
#define QFX3500_SFP_PTF_MGMT_BOARD_ID     0x0b5a
#define QFX3500_48S4Q_AFO_BOARD_ID        0x0b70

#define CASE_UFABRIC_QFX3600_SERIES \
    case QFX3600_M_BOARD_ID:          \
    case QFX3600_M_BOARD_ID_AFO

#define CASE_SE_QFX3600_SERIES      \
    case QFX3600_SE_BOARD_ID_AFI:   \
    case QFX3600_SE_BOARD_ID_AFO

#define PRODUCT_IS_OF_UFABRIC_QFX3600_SERIES(q)  \
    ((q) == QFX3600_M_BOARD_ID       || \
     (q) == QFX3600_M_BOARD_ID_AFO)

#define PRODUCT_IS_OF_SE_QFX3600_SERIES(r)       \
    ((r) == QFX3600_SE_BOARD_ID_AFI ||\
     (r) == QFX3600_SE_BOARD_ID_AFO)

#else

#error "CONFIG_FX not defined... should not be using fx.h"

#endif /*CONFIG_FX*/
#endif /*FX_H*/

