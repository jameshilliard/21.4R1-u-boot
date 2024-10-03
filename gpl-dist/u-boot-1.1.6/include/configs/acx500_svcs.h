/*
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * Copyright (c) 2014, Juniper Networks, Inc.
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

#if !defined(__ACX500_SVCS_H)
#define __ACX500_SVCS_H

#define BOARD_TYPE_ACX500_SVCS          0x33
#define ACX500_SVCS_DEF_DRAM_FREQ            533 /* Lets get it working at 533 */

#define DRAM_TYPE_SPD                   0x80 

#define CONFIG_MIPS32		1  /* MIPS32 CPU core	*/
#define CONFIG_OCTEON		1
#define CONFIG_OCTEON3

/* 1 to save 64-bit context on an exception */
#define MIPS64_CONTEXT_SAVE	0

#define OCTEON_MGMT_ENET

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

/* Set bootdelay to 0 for immediate boot */
#define CONFIG_BOOTDELAY            1    /* autoboot after X seconds	*/
#define CONFIG_BAUDRATE             9600

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
#define CFG_UBOOT_START           0xbfc00000
#define CFG_UBOOT_END             0xbfc7ffff
#define CFG_UBOOT_SIZE            0x80000

/*
** Define CONFIG_OCTEON_PCI_HOST = 1 to map the pci devices on the
** bus.  Define CONFIG_OCTEON_PCI_HOST = 0 for target mode when the
** host system performs the pci bus mapping instead.  Note that pci
** commands are enabled to allow access to configuration space for
** both modes.
*/
#ifndef CONFIG_OCTEON_PCI_HOST
#define CONFIG_OCTEON_PCI_HOST	0
#endif

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
                                  CFG_CMD_PING          | \
                                  CFG_CMD_NET           | \
                                  CFG_CMD_ELF           | \
                                  CFG_CMD_OCTEON        | \
                                  CFG_CMD_LOADB         | \
                                  CFG_CMD_FLASH         | \
                                  CFG_CMD_ENV           | \
                                  CFG_CMD_FAT           | \
                                  CFG_CMD_RUN           | \
                                  CFG_CMD_ASKENV)

#include <cmd_confdefs.h>

#undef CFG_CMD_USB
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

#define CFG_LATE_BOARD_INIT     1

#define ACX500_SVCS_TWSI_1                 1
#define ACX500_SVCS_CPLD_GP1               (uint8_t)0xd4
#define ACX500_SVCS_CPLD_GP2               (uint8_t)0xd5
#define ACX500_SVCS_CPLD_GP3               (uint8_t)0xd6
#define ACX500_SVCS_CPLD_GP4               (uint8_t)0xd7
#define ACX500_SVCS_CPLD_ADDR              (uint8_t)0x7f
#define ACX500_SVCS_CPLD_IMAGE_REG         (uint8_t)0xd3
#define ACX500_SVCS_CPLD_INTR_STATUS       (uint8_t)0xcf
#define ACX500_SVCS_CPLD_PFE_INTR_STATUS   (uint8_t)0xcb
#define ACX500_SVCS_CPLD_FLASH_SWIZZLE     (uint8_t)0xc4
#define ACX500_SVCS_FLASH_SWIZZLE_ENABLE   (uint8_t)0x1

#define ACX500_SVCS_CPLD                0x7f
#define ACX500_SVCS_I2C_GRP             0x1
#define ACX500_SVCS_FLASH_BANK          0x2
#define ACX500_SVCS_FLASH_SWIZZLE       0xc4
#define ACX500_SVCS_FLASH_BANK_SHIFT    0x1

#define UBOOT_BANK_0                    0x0
#define UBOOT_BANK_1                    0x1

#define I2C_SUCCESS                     1
#define I2C_FAILURE                     0

/*
 * Miscellaneous configurable options
 */
#define	CFG_LONGHELP			/* undef to save memory      */
#define	CFG_PROMPT		"=> "	/* Monitor Command Prompt    */

#define	CFG_CBSIZE		256		/* Console I/O Buffer Size   */
#define	CFG_PBSIZE (CFG_CBSIZE+sizeof(CFG_PROMPT)+16)  /* Print Buffer Size */
#define	CFG_MAXARGS		64		/* max number of command args*/

#define CFG_MALLOC_LEN		64*1024

#define CFG_BOOTPARAMS_LEN	16*1024

#define CFG_HZ		    1100000000ull

#define CFG_SDRAM_BASE		0x80000000     /* Cached addr */


#define CFG_MEMTEST_START	(CFG_SDRAM_BASE + 0x100000)
#define CFG_MEMTEST_END		(CFG_SDRAM_BASE + 0xffffff)



#define	CONFIG_EXTRA_ENV_SETTINGS					\
	"bf=bootoct bf480000 forceboot numcores=$(numcores)\0"				\
	"autoload=n\0"					\
	""

/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */

#define FLASH_BASE 0xbfc00000

#define CFG_FLASH_SIZE	(4*1024*1024)	/* Flash bank size (bytes) */
#define CFG_FLASH_BASE	FLASH_BASE

#define CFG_MAX_FLASH_BANKS	1	/* max number of memory banks */
#define CFG_MAX_FLASH_SECT	(512)	/* max number of sectors on one chip */
#define CFG_FLASH_SECT_SIZE	(64*1024)

#define CFG_FLASH_PROTECT_LEN   0x80000  /* protect low 512K */

/* The following #defines are needed to get flash environment right */
#define	CFG_MONITOR_BASE	FLASH_BASE
#define	CFG_MONITOR_LEN		(192 << 10)

#define CFG_INIT_SP_OFFSET	0x400000


/* CFI Driver */
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
#define UB_ETH_INIT         "1"

/* Environment variable names */
#define RX_SYNC_COUNT       5

/*-----------------------------------------------------------------------
 * Cache Configuration
 */
#define CFG_DCACHE_SIZE		(32 * 1024)
#define CFG_ICACHE_SIZE		(78 * 1024)
#define CFG_CACHELINE_SIZE	128

#define CONFIG_POST		CFG_POST_MEMORY

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

#define FLASH_BOOT_SECTOR_BOTTOM        2
#define FLASH_BOOT_SECTOR_TOP           3

#define IS_PCIE_GEN2_MODEL(board_type) \
    (((uint16_t)board_type) ==  BOARD_TYPE_ACX500_SVCS)

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


#endif	/* __ACX500_SVCS_H */
