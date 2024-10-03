/*
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 * Copyright 2004,2005 Cavium Networks
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

/*
 * This file contains the configuration parameters for the Octeon JSRXNLE boards.
 */

#ifndef __JSRXNLE_H
#define __JSRXNLE_H

#define CPU_REF                 50
#define DDR_CLOCK_MHZ           266
#define SRX100_DDR_CLOCK_MHZ    266
#define SRX110_DDR_CLOCK_MHZ    266
#define SRX210_DDR_CLOCK_MHZ    200
#define SRX220_DDR_CLOCK_MHZ    266
#define SRX240_DDR_CLOCK_MHZ    333
#define SRX650_DDR_CLOCK_MHZ    333
#define SRX210_533_DDR_CLOCK_MHZ  266
#define SRX550_DDR_CLOCK_MHZ    533

#define CONFIG_MIPS32		1  /* MIPS32 CPU core	*/
#define CONFIG_OCTEON		1

#define SRX220_CPU_CORE_FREQ_MULTIPLIER    14

#define FLASH_BASE 0xbfc00000

#define CF_SATA_PORT 0
#define SSD_SATA_PORT 1

/* 1 to save 64-bit context on an exception */
#define MIPS64_CONTEXT_SAVE	0
/* FPC port to connect to FPCs for diagnosis */
#define FPC_UART_PORT		1
#define CONFIG_FPC_CONSOLE_BAUD	115200

#if !defined(__U_BOOT_HOST__) && !defined(__BYTE_ORDER)
/* Set endian macros for simple exec includes, but not for host utilities */
#define __BYTE_ORDER __BIG_ENDIAN
#endif

/* Defaults to use if bootloader cannot autodetect settings */
#define DEFAULT_ECLK_FREQ_MHZ			400  /* Used if multiplier read fails, and for DRAM refresh rates*/


/* For CN5020 the DDR reference and CPU reference are the same, so
** we hard code them. */ 
#define CN5020_FORCED_DDR_AND_CPU_REF_HZ   (50000000)


/* Used to control conditional compilation for shared code between simple
** exec and u-boot */
#define CONFIG_OCTEON_U_BOOT


/* let the eth address be writeable */
#define CONFIG_ENV_OVERWRITE 1


/* bootloader usable memory size in bytes for jsrxnle_board, hard
   code until autodetect */
#define OCTEON_MEM_SIZE (1*1024*1024*1024ULL)

/* Addresses for various things on boot bus.  These addresses
** are used to configure the boot bus mappings. */

#define OCTEON_CPLD_CHIP_SEL        1
#define OCTEON_CPLD_BASE_ADDR       0x1fbf0000

#define OCTEON_SRX220_CPLD_BASE_ADDR    0x1e000000

#define SRX650_CPLD_I2C_ADDR  0x47
#define SRX650_SRE_POWER_PLD  0x77

#define SRX650_I2C_GROUP_TWSI0      0x0
#define SRX650_I2C_GROUP_SYSTEM_IO  0x14

/* Id eeprom stuff */
#define BOARD_EEPROM_TWSI_ADDR      0x51 
#define BOARD_RTC_TWSI_ADDR         0x68
#define MIDPLANE_BOARD_EEPROM_TWSI_ADDR 0x54 /*this eeprom is on mid-plane*/
#define SRX650_MAC_ADDR_COUNT         255  

#define JSRXNLE_EEPROM_JDEC_OFFSET              0x00
#define JSRXNLE_EEPROM_FORMAT_VERSION_OFFSET    0x02
#define JSRXNLE_EEPROM_I2CID_OFFSET             0x04
#define JSRXNLE_EEPROM_MAJOR_REV_OFFSET         0x06
#define JSRXNLE_EEPROM_MINOR_REV_OFFSET         0x07
#define JSRXNLE_EEPROM_REV_STRING_OFFSET        0x08
#define JSRXNLE_EEPROM_SERIAL_NO_OFFSET         0x20
#define JSRXNLE_EEPROM_MAC_MAGIC_OFFSET         0x34  
#define JSRXNLE_EEPROM_MAC_VERSION_OFFSET       0x35  
#define JSRXNLE_EEPROM_MAC_ADDR_OFFSET          0x38  
#define JSRXNLE_EEPROM_BOARD_DRAM_OFFSET        0x80  

#define JUNIPER_JDEC_CODE                       0x7fb0
#define JUNIPER_EEPROM_FORMAT_V1                0x01
#define JUNIPER_EEPROM_FORMAT_V2                0x02
#define JUNIPER_MAC_MAGIC                       0xad  
#define JUNIPER_MAC_VERSION                     0x01
#define SERIAL_STR_LEN                          0x0c

/* SRX110 CPLD registers */
#define SRX110_CPLD_SKU_ID                    0x36

/* SRX110 SKU-ID register bits */
#define SRX110_SKU_ID_VDSL_PRESENT     0x01
#define SRX110_SKU_ID_VDSL_B           0x02
#define SRX110_SKU_ID_EXP_CARD_PRESENT 0x04
#define SRX110_SKU_ID_WLAN_PRESENT     0x08
#define SRX110_SKU_ID_SUPERSET         0x10
#define SRX110_SKU_ID_DUAL_RADIO       0x20

#define SRX100_MICRON_REV               2

/* SRX210 CPLD registers */
#define SRX210_CPLD_VER_REG             0
#define SRX210_CPLD_LED_CTL_REG         1
#define SRX210_CPLD_RESET_REG           2
#define SRX210_CPLD_MISC_REG            3
#define SRX210_CPLD_WATCHDOG_REG        4
#define SRX210_CPLD_INT_MASK_REG        5
#define SRX210_CPLD_INT_STATUS_REG      6
#define SRXNLE_FPGA_CONF_REG            7
#define SRXNLE_FAN_TACHO_REG            8
#define SRXNLE_CPLD_MISC2_REG           0xb
#define SRX110_DEVICE_RESET_REG         0x32
#define SRX110_PCI_USB_RESET_BIT        0x01

/* SRX220 CPLD registers */
#define SRX220_CPLD_RESET_REG           2
#define SRX220_CPLD_SYS_RST_REG         0x40
#define SRX220_CPLD_SYS_RST_EN_REG      0x44

#define SRX220_CPLD_CF_STATUS_REG       0x4D
#define SRX220_CPLD_CF_CTRL_REG         0x53
#define SRX220_CPLD_CF_CTRL_MASK_REG    0x5A
#define SRX220_CPLD_HW_RST_STATUS_REG   0x80
/* proto_id register addresses */
#define SRX210_PROTO_ID_REG             0xf
#define SRX100_PROTO_ID_REG             0xf

#define SRXNLE_HA_LED_GREEN_MASK        (1 << 0)
#define SRXNLE_HA_LED_RED_MASK          (1 << 1)
#define SRXNLE_HA_LED_AMBER_MASK        (SRXNLE_HA_LED_GREEN_MASK \
                                         | SRXNLE_HA_LED_RED_MASK)

#define SRXNLE_STAT_LED_RED_MASK        (1 << 2)
#define SRXNLE_STAT_LED_GREEN_MASK      (1 << 3)
#define SRXNLE_STAT_LED_AMBER_MASK      (SRXNLE_STAT_LED_GREEN_MASK \
                                         | SRXNLE_STAT_LED_RED_MASK)

#define SRXNLE_ALRM_LED_GREEN_MASK      (1 << 4)
#define SRXNLE_ALRM_LED_RED_MASK        (1 << 5)
#define SRXNLE_ALRM_LED_AMBER_MASK      (SRXNLE_ALRM_LED_GREEN_MASK \
                                         | SRXNLE_ALRM_LED_RED_MASK)

#define SRXNLE_PWR_LED_GREEN_MASK       (1 << 6)
#define SRXNLE_PWR_LED_RED_MASK         (1 << 7)
#define SRXNLE_PWR_LED_AMBER_MASK       (SRXNLE_PWR_LED_GREEN_MASK \
                                         | SRXNLE_PWR_LED_RED_MASK)

#define SRXNLE_CPLD_ENABLE_FLASH_WRITE  (1 << 0)
#define SRX210_CPLD_BOOT_SEC_CTL_MASK   (1 << 4)
#define SRX210_CPLD_BOOT_SEC_SEL_MASK   (1 << 5)
#define SRX210_CPLD_RESET_DEV_MASK      (1 << 1)

/* SRX210 RESET reg bits */
#define SRX210_RESET_PIM                (1 << 0x00)
#define SRX210_RESET_RENESSAS           (1 << 0x01)
#define SRX210_RESET_CPU                (1 << 0x02)
#define SRX210_RESET_ETH                (1 << 0x03)
#define SRX210_RESET_POE                (1 << 0x04)

/* SRX220 RESET reg bits */
#define SRX220_RESET_MPIM1              (1 << 0x00)
#define SRX220_RESET_MPIM2              (1 << 0x05)
#define SRX220_RESET_RENESSAS           (1 << 0x01)
#define SRX220_RESET_ETH                (1 << 0x03)
#define SRX220_RESET_POE                (1 << 0x04)

#define SRX220_ENABLE_CPU_RESET         0x1
#define SRX220_RESET_CPU                0x0

#define SRX220_CF_ENABLE_PWR            (1 << 0x00)
#define SRX220_CF_ENABLE                (1 << 0x01)

#define SRX220_CF_PRESENT_MASK          0x01

/* SRX240 RESET reg bits */
#define SRX240_RESET_MPIM1              (1 << 0x00)
#define SRX240_RESET_MPIM2              (1 << 0x05)
#define SRX240_RESET_MPIM3              (1 << 0x06)
#define SRX240_RESET_MPIM4              (1 << 0x07)
#define SRX240_RESET_RENESSAS           (1 << 0x01)
#define SRX240_RESET_CPU                (1 << 0x02)
#define SRX240_RESET_ETH                (1 << 0x03)
#define SRX240_RESET_POE                (1 << 0x04)


#define SRX100_RESET_DEVICES            (SRX210_RESET_RENESSAS      \
                                         | SRX210_RESET_ETH)

#define SRX110_RESET_DEVICES            (SRX210_RESET_RENESSAS      \
                                         | SRX210_RESET_ETH)

#define SRX210_RESET_DEVICES            (SRX210_RESET_PIM           \
                                         | SRX210_RESET_RENESSAS    \
                                         | SRX210_RESET_ETH         \
                                         | SRX210_RESET_POE)

#define SRX220_RESET_DEVICES            (SRX220_RESET_MPIM1         \
                                         | SRX220_RESET_MPIM2       \
                                         | SRX220_RESET_RENESSAS    \
                                         | SRX220_RESET_ETH         \
                                         | SRX220_RESET_POE)

#define SRX240_RESET_DEVICES            (SRX240_RESET_MPIM1         \
                                         | SRX240_RESET_MPIM2       \
                                         | SRX240_RESET_MPIM3       \
                                         | SRX240_RESET_MPIM4       \
                                         | SRX240_RESET_RENESSAS    \
                                         | SRX240_RESET_ETH         \
                                         | SRX240_RESET_POE)


#define CFG_64BIT_VSPRINTF  1
#define CFG_64BIT_STRTOUL   1


/* SRX550 CPLD registers */

#define SRX550_CPLD_I2C_MUX_SEL_REG     0x5C
/* SRX650 CPLD registers */

#define SRX650_CPLD_LED_REG             0x2
#define SRX650_CPLD_RESET_PIM2_REG      0x7
#define SRX650_CPLD_POWER_REG           0xb
#define SRX650_CPLD_MASTER_CTL_REG      0xc
#define SRX650_CPLD_HRTBT_REG           0xf
#define SRX650_SYSIO_LED_REG1           0xb
#define SRX650_SYSIO_LED_REG2           0xc
#define SRX650_SYSIO_LED_REG3           0xd

#define SRX650_IS_OTHER_RE_PRESENT(x)   (!(x & (1 << 4)))
#define SRX650_IS_MASTER(x)             (x & (1 << 0))
#define SRX650_MASTER_WAIT_CNT          30

#define SRX650_SET_UBOOT_DONE(x)        (x |= (1 << 7))
#define SRX650_SET_A_B_BIT(x)           (x |= (1 << 0))
#define SRX650_CLR_A_B_BIT(x)           (x &= ~(1 << 0))

#define SRX650_RE_SLOT_BIT              7
#define SRX650_RE_SLOT_UPPER            1
#define SRX650_RE_SLOT_LOWER            0
#define SRX650_RESET_PIM2_MASK          0xff

#define SRX650_RE_LED_INIT_MASK         0xc7 /* HA    : Bit 0: Off 
                                              * Pwr   : Bit 2: Green
                                              * Status: Bit 4: Amber
                                              * Alarm : Bit 6: Off
                                              */
#define SRX650_SYSIO_LED1_INIT_MASK     0x20 /* HA    : Bit 0: Off 
                                              * FAN   : Bit 2: Off
                                              * PWR   : Bit 4: Green
                                              * Alarm : Bit 6: Off
                                              */

#define SRX650_I2C1_DEV_ADDR            0x70 /* I2C Expander #1 */
#define SRX650_PIO_IN_REG               0x0
#define SRX650_PIO_OUT_REG              0x1
#define SRX650_PIO_CONFIG_REG           0x3
#define SRX650_CLR_SYSTEM_RESET(x)      (x |= 0x80)

/* I2C Expander #1: For System Reset and Zarlink Clock
 *
 * Bit0: Out: Ref Clock
 * Bit1: Out: OOR Select
 * Bit2: Out: Mode Select
 * Bit3: Out: TIE Clear
 * Bit4: In : Premier Ref Clock Failure
 * Bit5: In : Secondary Ref Clock Failure
 * Bit6: In : Clock locked
 * Bit7: Out: System Reset
 */
/* Bit as 0 means output */
#define SRX650_I2C1_CONFIG_MASK         0x70

/* SRX650 GPIO defines */
#define SRX650_GPIO_RELINQUISH_PIN   1
#define SRX650_GPIO_BOOTED_PIN       2
#define SRX650_GPIO_CF_LED_L         3 /* Indicates external CF detected 
                                        * and in use 
                                        */
#define SRX650_GPIO_EXT_CF_PWR       4 /* This pin is set to turn on 
                                        * supply power to external CF
                                        */
#define SRX650_GPIO_IDE_CF_CS        5 /* Set to enable external CF */
#define SRX650_GPIO_EXT_CF_WE        6 /* External CF write enable */
#define SRX650_GPIO_CF_CD1_L         7 /* External CF detection PIN */


/* Set bootdelay to 0 for immediate boot */
#define CONFIG_BOOTDELAY	1	/* autoboot after X seconds	*/

#define CONFIG_BAUDRATE		                9600
#define CONFIG_DOWNLOAD_BAUDRATE		115200

/* valid baudrates */
#define CFG_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200, 230400, 460800 }

#define	CONFIG_TIMESTAMP		/* Print image info with timestamp */
#undef	CONFIG_BOOTARGS

#define CFG_UBOOT_METADATA_OFFSET       0x500
#define CFG_UBOOT_METADATA_SIZE         0x100
#define CFG_UBOOT_METADATA_END          (CFG_UBOOT_METADATA_OFFSET \
                                         + CFG_UBOOT_METADATA_SIZE)

#define CONFIG_IDENT_STRING             ""


#define CFG_LOAD_ADDR       0x100000
#define CFG_BOOTOCT_ADDR    0x100000

/* SRX240 loader defines */
#define SRX240_LOADER_NOT_STARTED    0 /* Loader not loaded */
#define SRX240_LOADER_INIT_PROGRESS  1 /* Loader init in progress */
#define SRX240_LOADER_INIT_COMPLETE  2 /* Loader init complete */    

/* these are used for updating the u-boot image */

#define CFG_UBOOT_SIZE            0x100000
#define CFG_PRIMARY_UBOOT_START   FLASH_BASE  
#define CFG_PRIMARY_UBOOT_END     (CFG_PRIMARY_UBOOT_START + CFG_UBOOT_SIZE - 1) 
#define CFG_SECONDARY_UBOOT_START 0xbfd00000
#define CFG_SECONDARY_UBOOT_END   (CFG_SECONDARY_UBOOT_START + CFG_UBOOT_SIZE - 1) 
#define CFG_LOADER_START          0xbfe00000
#define CFG_LOADER_END            0xbfefffff
#define CFG_LOADER_SIZE           0x100000
#define CFG_LOADER_HDR_OFFSET     0x2FFFC0
#define CFG_LOADER_DATA_OFFSET    0x200000


/* Minipim led register */
#define MINPIM_SRX210_BASE               0x1FBEull
#define MIO_BOOT_REG_CFG_EN	             (0x01ull    << 31)
#define MIO_BOOT_REG_CFG_WIDTH	         (0x0ull     << 28)
#define MIO_BOOT_REG_CFG_SIZE            (0x001ull   << 16)
/* Default value for boot bus region's timing register */
#define MIO_BOOT_REG_TIM_RESET_VAL       ~(0xFull << 60)

#define MIO_BOOT_REG_CFG_SIZE_64K        0 
#define SRX220_MPIM1_BASE                0x1E01ull
#define SRX220_MPIM2_BASE                0x1E02ull
#define SRX220_VOICE_BASE                0x1E03ull

#define SRX220_MPIM_DETECT_REG           0x60
#define SRX220_MPIM_SEL_MUX_REG          0x0C
#define SRX220_MPIM0_MUX_MASK            0x02
#define SRX220_MPIM1_MUX_MASK            0x03

#define SRX220_TOTAL_MPIMS               2

#define SRX240_MPIM1_BASE                0x1FBEull
#define SRX240_MPIM2_BASE                0x1FBDull
#define SRX240_MPIM3_BASE                0x1FBCull
#define SRX240_MPIM4_BASE                0x1FBBull

#define MPIM_LED_OFFSET                  0x0301

#define JSRXNLE_MPIM_PRESENCE_MASK       0x40
#define SRX240_MPIM_SEL_MUX_REG          0x0C
#define SRX240_MPIM0_MUX_MASK            0x02
#define SRX240_MPIM1_MUX_MASK            0x03
#define SRX240_MPIM2_MUX_MASK            0x00
#define SRX240_MPIM3_MUX_MASK            0x01

#define SRX240_TOTAL_MPIMS               4

/*
 * These values are used for 8MB bootflash. The following platforms
 * are using 8MB bootflash:
 *   - SRX220
 *   - SRX650
 *   - SRX550
 */
#define CFG_8MB_FLASH_PRIMARY_UBOOT_START   0xbf400000
#define CFG_8MB_FLASH_PRIMARY_UBOOT_END     (CFG_8MB_FLASH_PRIMARY_UBOOT_START + CFG_UBOOT_SIZE - 1) 
#define CFG_8MB_FLASH_SECONDARY_UBOOT_START 0xbf500000
#define CFG_8MB_FLASH_SECONDARY_UBOOT_END   (CFG_8MB_FLASH_SECONDARY_UBOOT_START + CFG_UBOOT_SIZE - 1) 
#define CFG_8MB_FLASH_LOADER_START          0xbf600000
#define CFG_8MB_FLASH_LOADER_P1_START  CFG_8MB_FLASH_LOADER_START	/*2m - 3m */
#define CFG_8MB_FLASH_LOADER_P1_END            0xbf6fffff

/* for Veloader , write to 2 parts , 2m-3m, 4m-5m */
#define CFG_8MB_FLASH_LOADER_SIZE          0x200000
#define CFG_8MB_LOADER_P1_SIZE             0x100000
#define CFG_8MB_LOADER_P1_DATA_SIZE       (0x100000 - 0x40)
#define CFG_8MB_LOADER_P2_SIZE             0x100000

#define CFG_8MB_FLASH_LOADER_P1_START  CFG_8MB_FLASH_LOADER_START	/*2m - 3m */
#define CFG_8MB_FLASH_LOADER_P2_START  (CFG_8MB_FLASH_PRIMARY_UBOOT_START + 0x400000)   /*4m - 5m */
#define CFG_8MB_FLASH_LOADER_P2_END    (CFG_8MB_FLASH_PRIMARY_UBOOT_START + 0x4fffff)
#define CFG_8MB_LOADER_HDR_OFFSET       (CFG_8MB_FLASH_LOADER_P1_START + CFG_8MB_LOADER_P1_SIZE - 0x40)
#define CFG_8MB_LOADER_DATA_OFFSET      CFG_8MB_FLASH_LOADER_P1_START

/* Memory reserve at top of about 256M */
#define JSRXNLE_RESERVE_TOP (255 * (1024*1024))
/* USB bounce buffer */
#define JSRXNLE_USB_BOUNCE_BUFFER_OFFSET (254 * (1024*1024))
#define JSRXNLE_LOADER_VERIFY_BUFFER_OFFSET (252 * (1024*1024))
#define RELOCATE_HIGH
#define JSRXNLE_RELOCATE_TOP 		(252 * (1024*1024))  /* 252 m */
#define JSRXNLE_RELOCATE_BOT 		(250 * (1024*1024))  /* 250 */
#define JSRXNLE_RELOCATE_RESERVE_LEN 	(JSRXNLE_RELOCATE_TOP - JSRXNLE_RELOCATE_BOT)
#define JSRXNLE_RESERVE_BOT  JSRXNLE_RELOCATE_BOT

/*
** Define CONFIG_OCTEON_PCI_HOST = 1 to map the pci devices on the
** bus.  Define CONFIG_OCTEON_PCI_HOST = 0 for target mode when the
** host system performs the pci bus mapping instead.  Note that pci
** commands are enabled to allow access to configuration space for
** both modes.
*/
#ifndef CONFIG_OCTEON_PCI_HOST
#define CONFIG_OCTEON_PCI_HOST	1
#endif
/*
** Define CONFIG_PCI only if the system is known to provide a PCI
** clock to Octeon.  A bus error exception will occur if the inactive
** Octeon PCI core is accessed.  U-boot is not currently configured to
** recover when a exception occurs.
*/
#if CONFIG_OCTEON_PCI_HOST 
#define CONFIG_PCI
#endif
/*-----------------------------------------------------------------------
 * PCI Configuration
 */
#if defined(CONFIG_PCI)

#define PCI_CONFIG_COMMANDS (CFG_CMD_PCI)

#if (CONFIG_OCTEON_PCI_HOST) && !defined(CONFIG_OCTEON_FAILSAFE)
#define CONFIG_PCI_PNP
#endif /* CONFIG_OCTEON_PCI_HOST */

#else  /* CONFIG_PCI */
#define PCI_CONFIG_COMMANDS (0)
#endif /* CONFIG_PCI */

/*
** Enable internal Octeon arbiter.
*/
#define USE_OCTEON_INTERNAL_ARBITER

/* Define this to enable built-in octeon ethernet support */
#define OCTEON_RGMII_ENET

/* Enable Octeon built-in networking if either SPI or RGMII support is enabled */
#if defined(OCTEON_RGMII_ENET) || defined(OCTEON_SPI4000_ENET)
#define OCTEON_INTERNAL_ENET
#endif

/*-----------------------------------------------------------------------
 * U-boot Commands Configuration
 */
/* Srecord loading seems to be busted - checking for ctrl-c eats bytes */
#define CONFIG_COMMANDS		 (CONFIG_CMD_DFL        | \
                                  CFG_CMD_ELF           | \
                                  CFG_CMD_OCTEON        | \
                                  CFG_CMD_LOADB         | \
                                  CFG_CMD_FLASH         | \
                                  CFG_CMD_ENV           | \
                                  CFG_CMD_FAT           | \
                                  CFG_CMD_IDE           | \
                                  CFG_CMD_RUN           | \
                                  CFG_CMD_EEPROM        | \
                                  CFG_CMD_ASKENV        | \
                                  CFG_CMD_NET           | \
                                  CFG_CMD_DHCP          | \
                                  CFG_CMD_PING          | \
                                  PCI_CONFIG_COMMANDS   | \
                                  CFG_CMD_USB)

#define HAVE_BLOCK_DEVICE

#include <cmd_confdefs.h>

/*-----------------------------------------------------------------------
 * Networking Configuration
 */
#if (CONFIG_COMMANDS & CFG_CMD_NET)
#define CONFIG_NET_MULTI
#define CONFIG_BOOTP_DEFAULT		(CONFIG_BOOTP_SUBNETMASK | \
					CONFIG_BOOTP_GATEWAY	 | \
					CONFIG_BOOTP_HOSTNAME	 | \
					CONFIG_BOOTP_BOOTPATH)

#define CONFIG_BOOTP_MASK		CONFIG_BOOTP_DEFAULT
#endif /* CONFIG_COMMANDS & CFG_CMD_NET */

/*
 * Based on lab experiments we have noticed that retrying
 * only for 5 times is not sufficient when the server becomes
 * available after 5 retries but within 25 retries,so increasing
 * it to 25.
 */
#define CONFIG_NET_RETRY_COUNT 25

#define CONFIG_AUTO_COMPLETE 1
#define CFG_CONSOLE_INFO_QUIET 1

#define CONFIG_DOS_PARTITION 1
#define CONFIG_ISO_PARTITION 1

#ifdef CFG_CMD_EEPROM
#define CFG_I2C_EEPROM_ADDR_LEN 2
#define CFG_I2C_EEPROM_ADDR     BOARD_EEPROM_TWSI_ADDR
#endif

/*
 * Miscellaneous configurable options
 */
#define	CFG_LONGHELP			/* undef to save memory      */
#define	CFG_PROMPT		"=> "	/* Monitor Command Prompt    */

#define	CFG_CBSIZE		256		/* Console I/O Buffer Size   */
#define	CFG_PBSIZE (CFG_CBSIZE+sizeof(CFG_PROMPT)+16)  /* Print Buffer Size */
#define	CFG_MAXARGS		64		/* max number of command args*/

#define CFG_MALLOC_LEN		96*1024

#define CFG_BOOTPARAMS_LEN	16*1024

#define CFG_HZ			    1100000000ull      /* FIXME causes overflow in net.c */

#define CFG_SDRAM_BASE		0x80000000     /* Cached addr */


#define CFG_MEMTEST_START	(CFG_SDRAM_BASE + 0x100000)
#define CFG_MEMTEST_END		(CFG_SDRAM_BASE + 0xffffff)



#define	CONFIG_EXTRA_ENV_SETTINGS					\
	"bf=bootoct bf480000 forceboot numcores=$(numcores)\0"		\
	"autoload=n\0"					\
	"nand_error=0\0"				\
	""

#define CONFIG_BOOTCOMMAND  \
        "cp.b 0xbfe00000 0x100000 0x100000; bootelf 0x100000"
	
/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */
#define CFG_FLASH_SIZE	(4*1024*1024)	/* Flash size (bytes) */
#define CFG_8MB_FLASH_SIZE (8*1024*1024)

#define CFG_MAX_FLASH_BANKS	1	/* max number of memory banks */
#define CFG_MAX_FLASH_SECT	(512)	/* max number of sectors on one chip */
#define CFG_FLASH_SECT_SIZE	(64*1024)

#define CFG_FLASH_BASE	FLASH_BASE
#define CFG_8MB_FLASH_BASE (FLASH_BASE -  CFG_8MB_FLASH_SIZE) /* Remapped base of flash */

#define CFG_FLASH_PROTECT_LEN   0x80000  /* protect low 512K */


/* The following #defines are needed to get flash environment right */
#define	CFG_MONITOR_BASE	FLASH_BASE
#define	CFG_MONITOR_LEN		(192 << 10)

#define CFG_INIT_SP_OFFSET	0x400000


#define CFG_FLASH_CFI   1
#define CFG_FLASH_CFI_DRIVER   1

/* timeout values are in ticks */
#define CFG_FLASH_ERASE_TOUT	(2 * CFG_HZ) /* Timeout for Flash Erase */
#define CFG_FLASH_WRITE_TOUT	(2 * CFG_HZ) /* Timeout for Flash Write */

#define	CFG_ENV_IS_IN_FLASH	1

/* Address and size of Primary Environment Sector	*/
#define CFG_ENV_SIZE		(8*1024)
#define CONFIG_ENV_SIZE		CFG_ENV_SIZE	
#define CFG_ENV_ADDR		(CFG_FLASH_BASE + CFG_FLASH_SIZE - CFG_ENV_SIZE)
#define CFG_8MB_FLASH_ENV_ADDR (CFG_8MB_FLASH_BASE + CFG_8MB_FLASH_SIZE - CFG_ENV_SIZE)

#define CONFIG_FLASH_8BIT

#define CONFIG_NR_DRAM_BANKS	2

#define CONFIG_MEMSIZE_IN_BYTES


/* these are used for updating the ushell image */
#define CFG_USHELL_SIZE          0x70000
#define CFG_4MB_USHELL_START     0xbff80000
#define CFG_4MB_USHELL_END       CFG_4MB_USHELL_START + CFG_USHELL_SIZE - 1
#define CFG_8MB_USHELL_START     0xbf780000
#define CFG_8MB_USHELL_END       CFG_8MB_USHELL_START + CFG_USHELL_SIZE - 1
#define CFG_550M_USHELL_SIZE      0x100000
#define CFG_550M_USHELL_END       CFG_8MB_USHELL_START + CFG_550M_USHELL_SIZE - 1
#define CFG_MDK_MAGIC_KEY        0x3c1c0000
#define CFG_MDK_DRAM_START_ADDR  0x80200000

/* Regex PCIE defines */
#define SRX650_PCIE_REGEX_PORT  1
#define SRX650_PCIE_REGEX_BUS   0
#define SRX650_PCIE_REGEX_DEV   0
#define SRX650_PCIE_REGEX_FUNC  0

/* Defines related to interface */
#define CVMX_INTERFACE_SPEED_XGMII_3125M_STR "3.125G"
#define CVMX_INTERFACE_SPEED_XGMII_1000M_STR "1G"
#define CVMX_INTERFACE_SPEED_XAUI_STR        "10G"

#define CVMX_SRX240_IFACE 0
#define CVMX_SRX650_IFACE 1
#define CVMX_SRX550_IFACE 0
#define CVMX_SRX320_IFACE 0

/* Defines related to loader initialization */
#define UB_ENV_LOADER_INIT  "loaderInit"
#define UB_ETH_INIT         "1"
#define UB_ETH_DEINIT       ""

/* Environment variable names */
#define SRXSME_BOOTDEVLIST_ENV              "boot.devlist"

#define RX_SYNC_COUNT       5

/* enable api for veloaer */
#define CONFIG_API         1
#define USB_MAX_STOR_DEV_API   2

/*-----------------------------------------------------------------------
 * Cache Configuration
 */
#define CFG_DCACHE_SIZE		( 8 * 1024)
#define CFG_ICACHE_SIZE		(32 * 1024)
#define CFG_CACHELINE_SIZE	128

#define CONFIG_USB_OHCI       1
#define CONFIG_USB_EHCI
#define CONFIG_USB_STORAGE    1
#define CONFIG_USB_DWC_OTG    1
#define CONFIG_USB_SHOW_NO_PROGRESS
#define OCTEON_EHCI_REG_BASE    0x80016f0000000000ull

#define SRX210_CPLD_REV_VER_80  0x80
#define SRX210_CPLD_REV_VER_8F  0x8F

/*
 * Board I2c ids
 */
#define I2C_ID_JSRXNLE_SRX210_LOWMEM_CHASSIS     0x0520  /* LOKI RE Lowmem */
#define I2C_ID_JSRXNLE_SRX210_HIGHMEM_CHASSIS    0x0521  /* LOKI RE Highmem */
#define I2C_ID_JSRXNLE_SRX210_POE_CHASSIS        0x0522  /* LOKI RE POE */
#define I2C_ID_JSRXNLE_SRX210_VOICE_CHASSIS      0x0523  /* LOKI RE VOICE */
#define I2C_ID_SRXSME_SRX210BE_CHASSIS           0x0544  /* LOKI RE 600mhz LOWMEM */
#define I2C_ID_SRXSME_SRX210HE_CHASSIS           0x0545  /* LOKI RE 600mhz HIGHMEM */
#define I2C_ID_SRXSME_SRX210HE_POE_CHASSIS       0x0546  /* LOKI RE 600mhz POE */
#define I2C_ID_SRXSME_SRX210HE_VOICE_CHASSIS     0x0547  /* LOKI RE 600mhz VOICE */
#define I2C_ID_SRXSME_SRX210HE2_CHASSIS          0x056C  /* LOKI v2 600mhz RE HIGH MEM */
#define I2C_ID_SRXSME_SRX210HE2_POE_CHASSIS      0x056D  /* LOKI v2 600mhz RE POE */
#define I2C_ID_JSRXNLE_SRX220_HIGHMEM_CHASSIS    0x0525  /* VALI RE HIGH MEM */
#define I2C_ID_JSRXNLE_SRX220_POE_CHASSIS        0x0526  /* VALI RE POE */
#define I2C_ID_JSRXNLE_SRX220_VOICE_CHASSIS      0x0527  /* VALI RE VOICE */
#define I2C_ID_SRXSME_SRX220H2_CHASSIS           0x056E  /* VALI v2 RE HIGH MEM */
#define I2C_ID_SRXSME_SRX220H2_POE_CHASSIS       0x056F  /* VALI v2 RE POE */
#define I2C_ID_JSRXNLE_SRX240_LOWMEM_CHASSIS     0x0528  /* VIDAR RE LOW MEM */
#define I2C_ID_JSRXNLE_SRX240_HIGHMEM_CHASSIS    0x0529  /* VIDAR RE HIGH MEM*/
#define I2C_ID_JSRXNLE_SRX240_POE_CHASSIS        0x052A  /* VIDAR RE POE */
#define I2C_ID_JSRXNLE_SRX240_VOICE_CHASSIS      0x052B  /* VIDAR RE VOICE */
#define I2C_ID_JSRXNLE_SRX650_CHASSIS            0x052E  /* PM3 THOR */
#define I2C_ID_JSRXNLE_SRX100_LOWMEM_CHASSIS     0x0530  /* NARFI LOW MEM CHASSIS */
#define I2C_ID_JSRXNLE_SRX100_HIGHMEM_CHASSIS    0x0531  /* NARFI HIGH MEM CHASSIS */
#define I2C_ID_SRXSME_SRX100H2_CHASSIS           0x0568  /* NARFI HIGH MEM v2 CHASSIS */
#define I2C_ID_SRXSME_SRX110B_VA_CHASSIS         0x0532  /* Sigyn base-Memory, VDSL-A, 3G Chassis */
#define I2C_ID_SRXSME_SRX110H_VA_CHASSIS         0x0533  /* Sigyn high-Memory, VDSL-A, 3G Chassis */
#define I2C_ID_SRXSME_SRX110B_VB_CHASSIS         0x0538  /* Sigyn Base Memory, VDSL-B, 3G Chassis */
#define I2C_ID_SRXSME_SRX110H_VB_CHASSIS         0x0539  /* Sigyn High Memory, VDSL-B, 3G Chassis */
#define I2C_ID_SRXSME_SRX110B_WL_CHASSIS         0x053A  /* Sigyn Base Memory, 802.11N Chassis */
#define I2C_ID_SRXSME_SRX110H_WL_CHASSIS         0x053F  /* Sigyn high-memory, 802.11N Dual Radio Chassis */
#define I2C_ID_SRXSME_SRX110H_VA_WL_CHASSIS      0x0540  /* Sigyn high-memory, VDSL-A, 802.11N dual-radio Chassis */
#define I2C_ID_SRXSME_SRX110H_VB_WL_CHASSIS      0x0541  /* Sigyn high-memory, VDSL-B, 802.11N dual-radio Chassis */
#define I2C_ID_SRXSME_SRX110H2_VA_CHASSIS        0x0569  /* SRX110 v2 Base Memory Chassis VDSL Annex-A */
#define I2C_ID_SRXSME_SRX110H2_VB_CHASSIS        0x056A  /* SRX110 v2 High Memory VDSL Annex-B */
#define I2C_ID_JSRXNLE_SRX240B2_CHASSIS          0x0542  /* SRX240 v2 Low Memory Chassis  */
#define I2C_ID_JSRXNLE_SRX240H2_CHASSIS          0x0543  /* SRX240 v2 High Memory Chassis */
#define I2C_ID_JSRXNLE_SRX240H_DC_CHASSIS        0x0548  /* SRX240 DC CHASSIS */
#define I2C_ID_SRX550_CHASSIS                    0x054F  /* MAGNI */
#define I2C_ID_JSRXNLE_SRX240H2_POE_CHASSIS      0x055A  /* SRX240 v2 High Memory PoE Chassis */
#define I2C_ID_JSRXNLE_SRX240H2_DC_CHASSIS       0x055B  /* SRX240 v2 High Memory DC Chassis  */
#define I2C_ID_SRX550M_CHASSIS                   0x058E  /* SRX550M Chassis */
#define I2C_ID_JSRXNLE_SRX300_CHASSIS            0x058F  /* Sword Chassis */
#define I2C_ID_JSRXNLE_SRX320_CHASSIS            0x0590  /* Sword-M Chassis */
#define I2C_ID_JSRXNLE_SRX320_POE_CHASSIS        0x0591  /* Sword-M-POE Chassis */
#define I2C_ID_JSRXNLE_SRX340_CHASSIS            0x0592  /* Trident Chassis */
#define I2C_ID_JSRXNLE_SRX345_CHASSIS            0x0593  /* Trident+ Chassis */
#define I2C_ID_JSRXNLE_SRX300_LEM_CHASSIS        0x0594  /* Sword LEM Chassis */
#define I2C_ID_JSRXNLE_SRX320_LEM_CHASSIS        0x0595  /* Sword-M LEM Chassis */
#define I2C_ID_JSRXNLE_SRX320_POE_LEM_CHASSIS    0x0596  /* Sword-M-POE LEM Chassis */
#define I2C_ID_JSRXNLE_SRX340_LEM_CHASSIS        0x0597  /* Trident LEM Chassis */
#define I2C_ID_JSRXNLE_SRX345_LEM_CHASSIS        0x0598  /* Trident+ LEM Chassis */

#define CASE_ALL_SRX100_MODELS                    \
    case I2C_ID_JSRXNLE_SRX100_LOWMEM_CHASSIS:    \
    case I2C_ID_JSRXNLE_SRX100_HIGHMEM_CHASSIS:   \
    case I2C_ID_SRXSME_SRX100H2_CHASSIS:

#define CASE_ALL_SRX110_MODELS                 \
    case I2C_ID_SRXSME_SRX110B_VA_CHASSIS:     \
    case I2C_ID_SRXSME_SRX110H_VA_CHASSIS:     \
    case I2C_ID_SRXSME_SRX110B_VB_CHASSIS:     \
    case I2C_ID_SRXSME_SRX110H_VB_CHASSIS:     \
    case I2C_ID_SRXSME_SRX110B_WL_CHASSIS:     \
    case I2C_ID_SRXSME_SRX110H_WL_CHASSIS:     \
    case I2C_ID_SRXSME_SRX110H_VA_WL_CHASSIS:  \
    case I2C_ID_SRXSME_SRX110H_VB_WL_CHASSIS:  \
    case I2C_ID_SRXSME_SRX110H2_VA_CHASSIS:    \
    case I2C_ID_SRXSME_SRX110H2_VB_CHASSIS:

#define CASE_ALL_SRX210_MODELS                  \
    case I2C_ID_JSRXNLE_SRX210_LOWMEM_CHASSIS:  \
    case I2C_ID_JSRXNLE_SRX210_HIGHMEM_CHASSIS: \
    case I2C_ID_JSRXNLE_SRX210_POE_CHASSIS:     \
    case I2C_ID_JSRXNLE_SRX210_VOICE_CHASSIS:   \
    case I2C_ID_SRXSME_SRX210BE_CHASSIS:        \
    case I2C_ID_SRXSME_SRX210HE_CHASSIS:        \
    case I2C_ID_SRXSME_SRX210HE_POE_CHASSIS:    \
    case I2C_ID_SRXSME_SRX210HE_VOICE_CHASSIS:  \
    case I2C_ID_SRXSME_SRX210HE2_CHASSIS:       \
    case I2C_ID_SRXSME_SRX210HE2_POE_CHASSIS:
		
#define CASE_ALL_SRX210_533_MODELS              \
    case I2C_ID_SRXSME_SRX210BE_CHASSIS:        \
    case I2C_ID_SRXSME_SRX210HE_CHASSIS:        \
    case I2C_ID_SRXSME_SRX210HE_POE_CHASSIS:    \
    case I2C_ID_SRXSME_SRX210HE_VOICE_CHASSIS:  \
    case I2C_ID_SRXSME_SRX210BE_CHASSIS:        \
    case I2C_ID_SRXSME_SRX210HE_CHASSIS:        \
    case I2C_ID_SRXSME_SRX210HE_POE_CHASSIS:    \
    case I2C_ID_SRXSME_SRX210HE_VOICE_CHASSIS:  \
    case I2C_ID_SRXSME_SRX210HE2_CHASSIS:       \
    case I2C_ID_SRXSME_SRX210HE2_POE_CHASSIS
		
#define CASE_ALL_SRX220_MODELS                    \
    case I2C_ID_JSRXNLE_SRX220_HIGHMEM_CHASSIS:   \
    case I2C_ID_JSRXNLE_SRX220_POE_CHASSIS:       \
    case I2C_ID_JSRXNLE_SRX220_VOICE_CHASSIS:     \
    case I2C_ID_SRXSME_SRX220H2_CHASSIS:          \
    case I2C_ID_SRXSME_SRX220H2_POE_CHASSIS:

#define CASE_ALL_SRX240_MODELS                  \
    case I2C_ID_JSRXNLE_SRX240_LOWMEM_CHASSIS:  \
    case I2C_ID_JSRXNLE_SRX240_HIGHMEM_CHASSIS: \
    case I2C_ID_JSRXNLE_SRX240_POE_CHASSIS:     \
    case I2C_ID_JSRXNLE_SRX240_VOICE_CHASSIS:   \
    case I2C_ID_JSRXNLE_SRX240H_DC_CHASSIS:	\
    case I2C_ID_JSRXNLE_SRX240B2_CHASSIS:	\
    case I2C_ID_JSRXNLE_SRX240H2_CHASSIS:	\
    case I2C_ID_JSRXNLE_SRX240H2_POE_CHASSIS:	\
    case I2C_ID_JSRXNLE_SRX240H2_DC_CHASSIS:	

#define CASE_ALL_SRX320_MODELS          \
    case I2C_ID_JSRXNLE_SRX300_CHASSIS: \
    case I2C_ID_JSRXNLE_SRX320_CHASSIS: \
    case I2C_ID_JSRXNLE_SRX320_POE_CHASSIS: \
    case I2C_ID_JSRXNLE_SRX300_LEM_CHASSIS: \
    case I2C_ID_JSRXNLE_SRX320_LEM_CHASSIS: \
    case I2C_ID_JSRXNLE_SRX320_POE_LEM_CHASSIS:

#define CASE_ALL_SRX550_MODELS      \
    case I2C_ID_SRX550_CHASSIS:     \
    case I2C_ID_SRX550M_CHASSIS:

#define CASE_ALL_SRX650_MODELS \
    case I2C_ID_JSRXNLE_SRX650_CHASSIS:

#define CASE_ALL_SRXLE_MODELS \
    CASE_ALL_SRX100_MODELS      \
    CASE_ALL_SRX110_MODELS      \
    CASE_ALL_SRX210_MODELS      \
    CASE_ALL_SRX220_MODELS      \
    CASE_ALL_SRX240_MODELS      \
    CASE_ALL_SRX550_MODELS

#define CASE_ALL_SRXMR_MODELS   \
    CASE_ALL_SRX650_MODELS

#define CASE_ALL_SRXSME_4MB_FLASH_MODELS \
    CASE_ALL_SRX100_MODELS      \
    CASE_ALL_SRX110_MODELS      \
    CASE_ALL_SRX210_MODELS      \
    CASE_ALL_SRX240_MODELS

#define CASE_ALL_SRXSME_8MB_FLASH_MODELS \
    CASE_ALL_SRX220_MODELS      \
    CASE_ALL_SRX550_MODELS       \
    CASE_ALL_SRX650_MODELS

#define CASE_ALL_SRXSME_8MB_USHELL_MODELS \
    CASE_ALL_SRX550_MODELS \
    CASE_ALL_SRX650_MODELS 

#define CASE_ALL_SRXSME_4MB_USHELL_MODELS \
    CASE_ALL_SRX240_MODELS 

/* SRXLE platforms which have NAND-Flash as primary media */
#define CASE_ALL_SRXLE_NF_MODELS \
    CASE_ALL_SRX100_MODELS       \
    CASE_ALL_SRX210_MODELS       \
    CASE_ALL_SRX240_MODELS

/* SRXLE platforms which have Compact-Flash as primary media */
#define CASE_ALL_SRXLE_CF_MODELS \
    CASE_ALL_SRX110_MODELS       \
    CASE_ALL_SRX220_MODELS       \
    CASE_ALL_SRX550_MODELS

#define IS_PCIE_GEN2_MODEL(board_type)                       \
    (((uint16_t)board_type) ==  I2C_ID_SRX550_CHASSIS   ||   \
     ((uint16_t)board_type) ==  I2C_ID_SRX550M_CHASSIS)   

#define IS_DDR3_MODEL(board_type)                       \
    (((uint16_t)board_type) == I2C_ID_SRX550_CHASSIS    ||   \
     ((uint16_t)board_type) == I2C_ID_SRX550M_CHASSIS)  

#define CASE_ALL_DDR3_MODELS \
        CASE_ALL_SRX550_MODELS

#define IS_PLATFORM_SRX100(board_type) \
                 ((((uint16_t)board_type) == I2C_ID_JSRXNLE_SRX100_LOWMEM_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_JSRXNLE_SRX100_HIGHMEM_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_SRXSME_SRX100H2_CHASSIS))

#define IS_PLATFORM_SRX110(board_type) \
                    ((((uint16_t)board_type) == I2C_ID_SRXSME_SRX110B_VA_CHASSIS)    \
                  || (((uint16_t)board_type) == I2C_ID_SRXSME_SRX110H_VA_CHASSIS)    \
                  || (((uint16_t)board_type) == I2C_ID_SRXSME_SRX110B_VB_CHASSIS)    \
                  || (((uint16_t)board_type) == I2C_ID_SRXSME_SRX110H_VB_CHASSIS)    \
                  || (((uint16_t)board_type) == I2C_ID_SRXSME_SRX110B_WL_CHASSIS)    \
                  || (((uint16_t)board_type) == I2C_ID_SRXSME_SRX110H_WL_CHASSIS)    \
                  || (((uint16_t)board_type) == I2C_ID_SRXSME_SRX110H_VA_WL_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_SRXSME_SRX110H_VB_WL_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_SRXSME_SRX110H2_VA_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_SRXSME_SRX110H2_VB_CHASSIS))

#define IS_PLATFORM_SRX210(board_type) \
                 ((((uint16_t)board_type) == I2C_ID_JSRXNLE_SRX210_LOWMEM_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_JSRXNLE_SRX210_HIGHMEM_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_JSRXNLE_SRX210_POE_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_JSRXNLE_SRX210_VOICE_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_SRXSME_SRX210BE_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_SRXSME_SRX210HE_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_SRXSME_SRX210HE_POE_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_SRXSME_SRX210HE_VOICE_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_SRXSME_SRX210HE2_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_SRXSME_SRX210HE2_POE_CHASSIS))

#define IS_PLATFORM_SRX210_533(board_type) \
                 ((((uint16_t)board_type) == I2C_ID_SRXSME_SRX210BE_CHASSIS)         \
                  || (((uint16_t)board_type) == I2C_ID_SRXSME_SRX210HE_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_SRXSME_SRX210HE_POE_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_SRXSME_SRX210HE_VOICE_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_SRXSME_SRX210HE2_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_SRXSME_SRX210HE2_POE_CHASSIS))

#define IS_PLATFORM_SRX220(board_type) \
                 ((((uint16_t)board_type) == I2C_ID_JSRXNLE_SRX220_HIGHMEM_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_JSRXNLE_SRX220_POE_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_JSRXNLE_SRX220_VOICE_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_SRXSME_SRX220H2_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_SRXSME_SRX220H2_POE_CHASSIS))

#define IS_PLATFORM_SRX240(board_type) \
                 ((((uint16_t)board_type) == I2C_ID_JSRXNLE_SRX240_LOWMEM_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_JSRXNLE_SRX240_HIGHMEM_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_JSRXNLE_SRX240_POE_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_JSRXNLE_SRX240_VOICE_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_JSRXNLE_SRX240H_DC_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_JSRXNLE_SRX240B2_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_JSRXNLE_SRX240H2_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_JSRXNLE_SRX240H2_POE_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_JSRXNLE_SRX240H2_DC_CHASSIS))

#define IS_PLATFORM_SRX550(board_type) \
                 ((((uint16_t)board_type) == I2C_ID_SRX550_CHASSIS) \
                  || (((uint16_t)board_type) == I2C_ID_SRX550M_CHASSIS)) 


#define IS_PLATFORM_SRXLE(board_type)                     \
                 (IS_PLATFORM_SRX100(board_type)      ||  \
                  IS_PLATFORM_SRX110(board_type)      ||  \
                  IS_PLATFORM_SRX210(board_type)      ||  \
                  IS_PLATFORM_SRX220(board_type)      ||  \
                  IS_PLATFORM_SRX240(board_type)      ||  \
                  IS_PLATFORM_SRX550(board_type))

#define IS_PLATFORM_SRX650(board_type) \
                 (((uint16_t)board_type) == I2C_ID_JSRXNLE_SRX650_CHASSIS)



/* valid only for srx650 */
#define I2C_SWITCH1 0x71
#define I2C_SWITCH2 0x72
#define I2C_SWITCH3 0x73
#define I2C_SWITCH4 0x74

#define I2C_SWITCH_CTRL_REG 0x00
#define ENABLE 1
#define DISABLE 0

#define JSRXNLE_I2C_SUCCESS 1
#define JSRXNLE_I2C_ERR 0

/*Define OCTEON_CF_16_BIT_BUS if board uses 16 bit CF interface */
#define OCTEON_CF_16_BIT_BUS


/* Octeon true IDE mode compact flash support.  True IDE mode is
** always 16 bit. */
#define OCTEON_CF_TRUE_IDE_CS0_CHIP_SEL     5
#define OCTEON_CF_TRUE_IDE_CS0_ADDR         0x1d040000
#define OCTEON_CF_TRUE_IDE_CS1_CHIP_SEL     6
#define OCTEON_CF_TRUE_IDE_CS1_ADDR         0x1d050000

#define CFG_IDE_MAXBUS 1
#define CFG_IDE_MAXDEVICE 2

#define CFG_IDE_MAXDEVICE_API   1

/* currently we have only one ide device on pci bus */
#define SRXLE_CFG_PCI_IDE_MAXDEVICE 1

/* Base address of Common memory for Compact flash */
#define CFG_ATA_BASE_ADDR  OCTEON_CF_COMMON_BASE_ADDR

/* Offset from base at which data is repeated so that it can be
** read as a block */
#define CFG_ATA_DATA_OFFSET     0x100
#define CFG_ATA_ALT_OFFSET      0x400

/* Not sure what this does, probably offset from base
** of the command registers */
#define CFG_ATA_REG_OFFSET      0x200

#define CONFIG_POST		(CFG_POST_EEPROM    | \
				 CFG_POST_USB	    | \
				 CFG_POST_UBOOT_CRC | \
				 CFG_POST_MEMORY    | \
				 CFG_POST_SKU_ID)

#ifdef CONFIG_POST
#define POST_PASSED        0
#define POST_FAILED        1

#define RTC_POST_RESULT_MASK    (0x00000001)
#define EEPROM_POST_RESULT_MASK (0x00000002)
#define USB_POST_RESULT_MASK    (0x00000004)
#define MEMORY_POST_RESULT_MASK (0x00000008)
#define UBOOT_CRC_POST_RESULT_MASK	(0x00000010)
#define SKU_ID_POST_RESULT_MASK (0x00000020)
#endif

/* config auto boot stop */
#define CONFIG_AUTOBOOT_PROMPT "Press SPACE to abort autoboot in %d seconds\n"
#define CONFIG_AUTOBOOT_STOP_STR  " "
#define CONFIG_AUTOBOOT_KEYED  1

/* key to abort auto loading of kernel in loader */
#define CONFIG_AUTOBOOT_LOAD_ABORT_STR   {0x03, '\0'}

/* debug data register */
#define  CN5020_DBG_DATA_REG 0x80011F00000001E8ull
#define  CN5230_DBG_DATA_REG 0x80011F0000008510ull
#define  CN5600_DBG_DATA_REG 0x80011F0000008510ull
#define  CN6300_DBG_DATA_REG 0x8001180000001600ull

/* octeon ciu Watchdog 0 offsets */
#define CIU_WATCHDOG0_OFFSET 0
#define CIU_PP_POKE0_OFFSET  0

/* octeon ciu watchdog modes */
#define WATCHDOG_MODE_OFF          0
#define WATCHDOG_MODE_INT          1
#define WATCHDOG_MODE_INT_NMI      2
#define WATCHDOG_MODE_INT_NMI_SRES 3

#define WATCH_DOG_LEN          0xffff
#define WATCH_DOG_ENABLE_MASK  0x00000000000ffff0ull
#define WATCH_DOG_DISABLE_MASK 0x0000000000000003ull
#define WATCH_DOG_LOWER_MASK   0xffffffff

#define SRXLE_BOOTSEQ_TOPBOOT_FLASH_SECTADDR         0xbfffa000
#define SRXLE_BOOTSEQ_BOTBOOT_FLASH_SECTADDR         0xbffe0000
#define SRXMR_BOOTSEQ_FLASH_SECTADDR                 0xbfbe0000

#define SRXLE_TOPBOOT_BOOTSEQ_SECT_LEN   (8 * 1024)
#define SRXLE_BOTBOOT_BOOTSEQ_SECT_LEN   (64 * 1024)
#define SRXMR_BOOTSEQ_SECT_LEN           (64 * 1024)

/* Enable reload watchdog support */
#define RELOAD_WATCHDOG_SUPPORT 1

#define FLASH_BOOT_SECTOR_BOTTOM        2
#define FLASH_BOOT_SECTOR_TOP           3

#define SRXSME_I2C_GROUP_MIDPLANE 0x15

#if !defined(ASMINCLUDE)
/* Extern definitions */
extern int ethact_init;
extern int srx650_get_re_slot(void);
extern int srx650_is_master(void);
extern void do_link_update(void);
extern int mdk_init(void);
extern void enable_external_cf(void);
extern void disable_external_cf(void);
extern int cf_disk_scan(int dev);
extern int srxle_cf_present(void);
extern void srxle_cf_enable(void);
extern unsigned char 
read_i2c(unsigned char dev_addr, unsigned char offset,
         unsigned char *val, unsigned char group);
extern unsigned char 
write_i2c(unsigned char dev_addr, unsigned char offset,
          unsigned char val, unsigned char group);

#endif /* !ASMINCLUDE */

#endif	/* __JSRXNLE_H */

