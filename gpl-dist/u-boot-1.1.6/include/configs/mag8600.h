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
 * This file contains the configuration parameters for the Octeon MAG8600 boards.
 */

#ifndef __MAG8600_H
#define __MAG8600_H

#define MAG8600_MSC	  	0x09F2
#define MSC_CPU_REF             50
#define MSC_DEF_DRAM_FREQ       400

#define CONFIG_MIPS32		1  /* MIPS32 CPU core	*/
#define CONFIG_OCTEON		1

#define DRAM_TYPE_SPD		0x80

/* 1 to save 64-bit context on an exception */
#define MIPS64_CONTEXT_SAVE	0
#define OCTEON_MGMT_ENET
/*  added to make compiler happy */
#define FPC_UART_PORT		1
#define CONFIG_FPC_CONSOLE_BAUD	9600

#if !defined(__U_BOOT_HOST__) && !defined(__BYTE_ORDER)
/* Set endian macros for simple exec includes, but not for host utilities */
#define __BYTE_ORDER __BIG_ENDIAN
#endif

/* Defaults to use if bootloader cannot autodetect settings */
#define DEFAULT_ECLK_FREQ_MHZ			400  /* Used if multiplier read fails, and for DRAM refresh rates*/

#define CFG_64BIT_VSPRINTF  1
#define CFG_64BIT_STRTOUL   1

/* Used to control conditional compilation for shared code between simple
** exec and u-boot */
#define CONFIG_OCTEON_U_BOOT


/* let the eth address be writeable */
#define CONFIG_ENV_OVERWRITE 1


/* bootloader usable memory size in bytes for mag8600_board, hard
   code until autodetect */
//#define OCTEON_MEM_SIZE (1*1024*1024*1024ULL)


/* Addresses for various things on boot bus.  These addresses
** are used to configure the boot bus mappings. */

#define OCTEON_CPLD_CHIP_SEL        1
#define OCTEON_CPLD_BASE_ADDR       0xbdb00000


/* Id eeprom stuff */
#define BOARD_EEPROM_TWSI_ADDR      0x56 
#define BOARD_RTC_TWSI_ADDR         0x68

#define MAG8600_EEPROM_JDEC_OFFSET              0x00
#define MAG8600_EEPROM_FORMAT_VERSION_OFFSET    0x02
#define MAG8600_EEPROM_I2CID_OFFSET             0x04
#define MAG8600_EEPROM_MAJOR_REV_OFFSET         0x06
#define MAG8600_EEPROM_MINOR_REV_OFFSET         0x07
#define MAG8600_EEPROM_REV_STRING_OFFSET        0x08
#define MAG8600_EEPROM_SERIAL_NO_OFFSET         0x20
#define MAG8600_EEPROM_MAC_MAGIC_OFFSET         0x34  
#define MAG8600_EEPROM_MAC_VERSION_OFFSET       0x35  
#define MAG8600_EEPROM_MAC_ADDR_OFFSET          0x38  
#define MAG8600_EEPROM_BOARD_DRAM_OFFSET        0x80  

#define JUNIPER_JDEC_CODE                       0x7fb0
#define JUNIPER_EEPROM_FORMAT_V1                0x01
#define JUNIPER_EEPROM_FORMAT_V2                0x02
#define JUNIPER_MAC_MAGIC                       0xad  
#define JUNIPER_MAC_VERSION                     0x01
#define SERIAL_STR_LEN                          0x12


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


/* these are used for updating the u-boot image */
#define CFG_PRIMARY_UBOOT_START   0xbfc00000
#define CFG_PRIMARY_UBOOT_END     0xbfc7ffff
#define CFG_SECONDARY_UBOOT_START 0xbfd00000
#define CFG_SECONDARY_UBOOT_END   0xbfd7ffff
#define CFG_UBOOT_SIZE            0x80000
#define CFG_LOADER_START          0xbfe00000 
#define CFG_LOADER_END            0xbfefffff
#define CFG_LOADER_SIZE           0x100000
#define CFG_LOADER_HDR_OFFSET     0x2FFFC0
#define CFG_LOADER_DATA_OFFSET    0x200000




/* USB bounce buffer */
#define JSRXNLE_USB_BOUNCE_BUFFER_OFFSET (255 * (1024*1024))

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
                                  CFG_CMD_MII          | \
                                  PCI_CONFIG_COMMANDS   | \
                                  CFG_CMD_USB)
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

#ifdef CFG_CMD_EEPROM
#define CFG_I2C_EEPROM_ADDR_LEN 2
#define CFG_I2C_EEPROM_ADDR     BOARD_EEPROM_TWSI_ADDR
#endif

/*
 * Miscellaneous configurable options
 */
#define	CFG_LONGHELP			/* undef to save memory      */
#define	CFG_PROMPT		"> "	/* Monitor Command Prompt    */

#define	CFG_CBSIZE		256		/* Console I/O Buffer Size   */
#define	CFG_PBSIZE (CFG_CBSIZE+sizeof(CFG_PROMPT)+16)  /* Print Buffer Size */
#define	CFG_MAXARGS		64		/* max number of command args*/

#define CFG_MALLOC_LEN		64*1024

#define CFG_BOOTPARAMS_LEN	16*1024

#define CFG_HZ			500000000ull      /* FIXME causes overflow in net.c */

#define CFG_SDRAM_BASE		0x80000000     /* Cached addr */


#define CFG_MEMTEST_START	(CFG_SDRAM_BASE + 0x100000)
#define CFG_MEMTEST_END		(CFG_SDRAM_BASE + 0xffffff)



#define	CONFIG_EXTRA_ENV_SETTINGS					\
	"bf=bootoct bf480000 forceboot numcores=$(numcores)\0"				\
	"autoload=n\0"					\
	""

#define CONFIG_BOOTCOMMAND  \
        "cp.b 0xbfe00000 0x100000 0x100000; bootelf 0x100000"
	
/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */
#define CFG_FLASH_SIZE	(16*1024*1024)	/* Flash size (bytes) */

#define CFG_MAX_FLASH_BANKS	1	/* max number of memory banks */
#define CFG_MAX_FLASH_SECT	(512)	/* max number of sectors on one chip */
#define CFG_FLASH_SECT_SIZE	(64*1024)

#define CFG_FLASH_BASE	(0xbfc00000 - CFG_FLASH_SIZE) /* remap */ 

#define CFG_FLASH_PROTECT_LEN   0x80000  /* protect low 512K */


/* The following #defines are needed to get flash environment right */
#define	CFG_MONITOR_BASE	TEXT_BASE
#define	CFG_MONITOR_LEN		(192 << 10)

#define CFG_INIT_SP_OFFSET	0x400000


#define CFG_FLASH_CFI   1
#define CFG_FLASH_CFI_DRIVER   1

/* timeout values are in ticks */
#define CFG_FLASH_ERASE_TOUT	(2 * CFG_HZ) /* Timeout for Flash Erase */
#define CFG_FLASH_WRITE_TOUT	(2 * CFG_HZ) /* Timeout for Flash Write */

#define	CFG_ENV_IS_IN_FLASH	1

/* Address and size of Primary Environment Sector	*/
#define CFG_ENV_SIZE		(128*1024)
#define CFG_ENV_ADDR		(CFG_FLASH_BASE + CFG_FLASH_SIZE - CFG_ENV_SIZE)

#define CONFIG_FLASH_8BIT

#define CONFIG_NR_DRAM_BANKS	2

#define CONFIG_MEMSIZE_IN_BYTES



/* Defines related to loader initialization */
#define UB_ENV_LOADER_INIT  "loaderInit"
#define UB_ETH_INIT         "1"
#define UB_ETH_DEINIT       ""

/* Environment variable names */
#define MAG8600_BOOTDEVLIST_ENV              "boot.devlist"

#define RX_SYNC_COUNT       5

/*-----------------------------------------------------------------------
 * Cache Configuration
 */
#define CFG_DCACHE_SIZE		(16 * 1024)
#define CFG_ICACHE_SIZE		(32 * 1024)
#define CFG_CACHELINE_SIZE	128

#define CONFIG_USB_OHCI       1
#define CONFIG_USB_STORAGE    1
#define CONFIG_USB_DWC_OTG    1
#define CONFIG_USB_SHOW_NO_PROGRESS
#define OCTEON_EHCI_REG_BASE    0x80016f0000000000ull

#define MSC_DIMM_0_SPD_TWSI_ADDR    0x50


#define MSC_INTERFACE1_DIMM_0_SPD_TWSI_ADDR    0x52
#define MSC_DIMM_1_SPD_TWSI_ADDR    0x52
#define OCTEON_MSC_DRAM_SOCKET_CONFIGURATION \
    {MSC_DIMM_0_SPD_TWSI_ADDR, 0}



#define OCTEON_MSC_DRAM_SOCKET_CONFIGURATION_INTERFACE1 \
    {MSC_INTERFACE1_DIMM_0_SPD_TWSI_ADDR, 0}



#define DRAM_SOCKET_CONFIGURATION               OCTEON_MSC_DRAM_SOCKET_CONFIGURATION
#define DRAM_SOCKET_CONFIGURATION_INTERFACE1    OCTEON_MSC_DRAM_SOCKET_CONFIGURATION_INTERFACE1

#define OCTEON_CN56XX_MSC_DDR_BOARD_DELAY               4200
#define OCTEON_CN56XX_MSC_LMC_DELAY_CLK         7
#define OCTEON_CN56XX_MSC_LMC_DELAY_CMD         0
#define OCTEON_CN56XX_MSC_LMC_DELAY_DQ          6
#define OCTEON_CN56XX_MSC_UNB_DDR_BOARD_DELAY   4600
#define OCTEON_CN56XX_MSC_UNB_LMC_DELAY_CLK             13
#define OCTEON_CN56XX_MSC_UNB_LMC_DELAY_CMD     0
#define OCTEON_CN56XX_MSC_UNB_LMC_DELAY_DQ              8

/* juniper specific */
#define JUNOS_EEPROM_JDEC_OFFSET              0x00
#define JUNOS_EEPROM_FORMAT_VERSION_OFFSET    0x02
#define JUNOS_EEPROM_I2CID_OFFSET             0x04
#define JUNOS_EEPROM_MAJOR_REV_OFFSET         0x06
#define JUNOS_EEPROM_MINOR_REV_OFFSET         0x07
#define JUNOS_EEPROM_REV_STRING_OFFSET        0x08
#define JUNOS_EEPROM_SERIAL_NO_OFFSET         0x20
#define JUNOS_EEPROM_MAC_MAGIC_OFFSET         0x34  
#define JUNOS_EEPROM_MAC_VERSION_OFFSET       0x35  
#define JUNOS_EEPROM_MAC_ADDR_OFFSET          0x38  
#define JUNOS_EEPROM_BOARD_DRAM_OFFSET        0x80  

#define JUNIPER_JDEC_CODE                       0x7fb0
#define JUNIPER_EEPROM_FORMAT_V1                0x01
#define JUNIPER_EEPROM_FORMAT_V2                0x02
#define JUNIPER_MAC_MAGIC                       0xad  
#define JUNIPER_MAC_VERSION                     0x01
#define SERIAL_STR_LEN                          0x12



#define MAG8600_I2C_SUCCESS 1
#define MAG8600_I2C_ERR 0

/*Define OCTEON_CF_16_BIT_BUS if board uses 16 bit CF interface */
#define OCTEON_CF_16_BIT_BUS


/* Octeon true IDE mode compact flash support.  True IDE mode is
** always 16 bit. */
#define OCTEON_CF_TRUE_IDE_CS0_CHIP_SEL     5
#define OCTEON_CF_TRUE_IDE_CS0_ADDR         0x1d040000
#define OCTEON_CF_TRUE_IDE_CS1_CHIP_SEL     6
#define OCTEON_CF_TRUE_IDE_CS1_ADDR         0x1d050000
#define OCTEON_CF_RESET_GPIO		    5

#define CFG_IDE_MAXBUS 1
#define CFG_IDE_MAXDEVICE 2

/* Base address of Common memory for Compact flash */
#define CFG_ATA_BASE_ADDR  OCTEON_CF_COMMON_BASE_ADDR

/* Offset from base at which data is repeated so that it can be
** read as a block */
#define CFG_ATA_DATA_OFFSET     0x100
#define CFG_ATA_ALT_OFFSET      0x400

/* Not sure what this does, probably offset from base
** of the command registers */
#define CFG_ATA_REG_OFFSET      0x200

#define CONFIG_POST		(CFG_POST_UBOOT_CRC | \
				 CFG_POST_MEMORY)

#ifdef CONFIG_POST
#define POST_PASSED        0
#define POST_FAILED        1

#define RTC_POST_RESULT_MASK    (0x00000001)
#define EEPROM_POST_RESULT_MASK (0x00000002)
#define USB_POST_RESULT_MASK    (0x00000004)
#define MEMORY_POST_RESULT_MASK (0x00000008)
#define UBOOT_CRC_POST_RESULT_MASK	(0x00000010)
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

/* octeon ciu Watchdog 0 offsets */
#define CIU_WATCHDOG0_OFFSET 0
#define CIU_PP_POKE0_OFFSET  0

#define MAG8600_BOOTSEQ_FLASH_SECTADDR                 0xbf800000

/* octeon ciu watchdog modes */
#define WATCHDOG_MODE_OFF          0
#define WATCHDOG_MODE_INT          1
#define WATCHDOG_MODE_INT_NMI      2
#define WATCHDOG_MODE_INT_NMI_SRES 3

#define WATCH_DOG_LEN          0xffff
#define WATCH_DOG_ENABLE_MASK  0x00000000000ffff0ull
#define WATCH_DOG_DISABLE_MASK 0x0000000000000003ull
#define WATCH_DOG_LOWER_MASK   0xffffffff



#define FLASH_BOOT_SECTOR_BOTTOM        2
#define FLASH_BOOT_SECTOR_TOP           3

#define IS_PCIE_GEN2_MODEL(board_type)  0 
#if !defined(ASMINCLUDE)
/* Extern definitions */
extern int ethact_init;
extern unsigned char 
read_i2c(unsigned char dev_addr, unsigned char offset,
         unsigned char *val, unsigned char group);
extern unsigned char 
write_i2c(unsigned char dev_addr, unsigned char offset,
          unsigned char val, unsigned char group);

#endif /* !ASMINCLUDE */


#endif	/* __MAG8600_H */
