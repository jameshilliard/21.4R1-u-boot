/*
 * Copyright (c) 2011-2013, Juniper Networks, Inc.
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

 /*
  * ex4300 board configuration file
  */
#ifndef __CONFIG_H
#define __CONFIG_H

/* High Level Configuration Options */
#define CONFIG_BOOKE		1	/* BOOKE */
#define CONFIG_E500		1	/* BOOKE e500 family */
#define CONFIG_E500MC		1	/* BOOKE e500mc family */
#define CONFIG_SYS_BOOK3E_HV	1	/* Category E.HV supported */
#define CONFIG_PPC_P2041	1
#define CONFIG_MPC85xx		1	/* MPC85xx/PQ3 platform */
#define CONFIG_FSL_CORENET	1	/* Freescale CoreNet platform */
#define CONFIG_EX4300		1	/* ex4300 board */
#define CONFIG_PRODUCT_EXSERIES	1	/* EX series product */
#undef CONFIG_MPC8548

/* FIXME: define CONFIG_PRODUCTION once all dev is complete */
#undef CONFIG_PRODUCTION

#ifndef CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_TEXT_BASE		0xfff80000
#endif

#define CONFIG_SYS_FSL_CPC		/* Corenet Platform Cache */
#define CONFIG_SYS_NUM_CPC		CONFIG_NUM_DDR_CONTROLLERS
#define CONFIG_FSL_ELBC			/* Has Enhanced localbus controller */

#define CONFIG_FSL_LAW			/* Use common FSL init code */
#define CONFIG_SYS_FSL_NUM_LAWS         32

#define CONFIG_ENV_OVERWRITE
#define CONFIG_ENABLE_36BIT_PHYS

#define CONFIG_SYS_CLK_FREQ		100000000	/* sysclk */

#define CONFIG_FLASH_CFI_DRIVER
#define CONFIG_SYS_FLASH_CFI
#define CONFIG_SYS_FLASH_EMPTY_INFO
#define CONFIG_SYS_FLASH_QUIET_TEST
#define CONFIG_FLASH_SHOW_PROGRESS	45	/* count down from 45/5: 9..1 */

/* DDR Setup */
#define CONFIG_VERY_BIG_RAM
#define CONFIG_FSL_DDR3
#define CONFIG_SYS_DDR_SDRAM_BASE	0x00000000
#define CONFIG_SYS_SDRAM_BASE		CONFIG_SYS_DDR_SDRAM_BASE

#define CONFIG_NUM_DDR_CONTROLLERS	1
#define CONFIG_DIMM_SLOTS_PER_CTLR	1
#define CONFIG_CHIP_SELECTS_PER_CTRL	(4 * CONFIG_DIMM_SLOTS_PER_CTLR)

#define CONFIG_SPD_EEPROM		/* Use SPD EEPROM for DDR setup*/
#define CONFIG_DDR_SPD
#define CONFIG_SYS_SPD_BUS_NUM		0
#define SPD_EEPROM_ADDRESS		0x55
#define CONFIG_SYS_SDRAM_SIZE		4096	/* for fixed parameter use */
#define CONFIG_DDR_ECC			/* support DDR ECC function */
#define CONFIG_ECC_INIT_VIA_DDRCONTROLLER
#define CONFIG_MEM_INIT_VALUE		0xdeadbeef

/*
 * Force u-boot relocation address & stack to be within 1GB of RAM so that
 * u-boot and pre-maped kernel address can share the same TLB entry.
 * For ex, if RAM size > 1GB and if u-boot relocates to top of the memory
 * at < 2GB, loader will invalidate the TLB entry that maps 0-1GB where kernel
 * will be loaded. This will cause Instruction TLB error.
 */
/* relocate u-boot at the top of 1GB */
#define CONFIG_MEM_RELOCATE_TOP		(1024 << 20)

/*
 * Assign fixed tlb index for mapping RAM. Index should be > 4.
 * Loader uses tlb1 entries[0-4] for pre-mapping the kernel.
 */
#define CONFIG_RAM_TLB_INDEX		8

#define CONFIG_L1_INIT_RAM
#define CONFIG_SYS_INIT_RAM_LOCK	1
#define CONFIG_SYS_INIT_RAM_ADDR	0xfdd00000	/* Initial L1 address */
#ifdef CONFIG_PHYS_64BIT
#define CONFIG_SYS_INIT_RAM_ADDR_PHYS_HIGH 0xf
#define CONFIG_SYS_INIT_RAM_ADDR_PHYS_LOW CONFIG_SYS_INIT_RAM_ADDR
/* The assembler doesn't like typecast */
#define CONFIG_SYS_INIT_RAM_ADDR_PHYS \
	(CONFIG_SYS_INIT_RAM_ADDR_PHYS_HIGH * 1ull << 32) | \
	 CONFIG_SYS_INIT_RAM_ADDR_PHYS_LOW)
#else
#define CONFIG_SYS_INIT_RAM_ADDR_PHYS		CONFIG_SYS_INIT_RAM_ADDR
#define CONFIG_SYS_INIT_RAM_ADDR_PHYS_HIGH	0
#define CONFIG_SYS_INIT_RAM_ADDR_PHYS_LOW	CONFIG_SYS_INIT_RAM_ADDR_PHYS
#endif
/* CONFIG_SYS_INIT_RAM_END renamed as CONFIG_SYS_INIT_RAM_SIZE */
#define CONFIG_SYS_INIT_RAM_SIZE		0x00004000 

#define CONFIG_SYS_GBL_DATA_OFFSET		(CONFIG_SYS_INIT_RAM_SIZE - \
						GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_OFFSET		CONFIG_SYS_GBL_DATA_OFFSET
#define CONFIG_SYS_MONITOR_LEN			(512 * 1024)
#define CONFIG_SYS_MALLOC_LEN			(1024 * 1024)
#define CONFIG_SYS_MONITOR_BASE			CONFIG_SYS_TEXT_BASE

/* Environment */
#define CONFIG_FLASH_SIZE		0x800000
#define CFG_FLASH_SIZE			0x800000
#define CONFIG_ENV_IS_IN_FLASH		1
#define CFG_ENV_IS_IN_FLASH		1
#define CONFIG_ENV_ADDR			(CONFIG_SYS_FLASH_BASE + 0x10000)
#define CFG_ENV_OFFSET			0x10000
#define CFG_ENV_SECTOR			1
#define CONFIG_ENV_SIZE			0x10000
#define CFG_ENV_SIZE			0x10000
#define CONFIG_ENV_SECT_SIZE		0x10000		/* 64K (one sector) */
#define CFG_OPQ_ENV_ADDR		(CONFIG_SYS_FLASH_BASE + 0x20000) /* for RE nv env */
#define CFG_OPQ_ENV_OFFSET		0x20000
#define CFG_OPQ_ENV_SECT_SIZE		0x10000         /* 64K(1x64K sector) for RE nv env */
#define CFG_UPGRADE_ADDR		(CONFIG_SYS_FLASH_BASE + 0XA0000)
#define CFG_UPGRADE_SECT_SIZE		0x10000         /* 64K(1x64K sector) for upgrade */
#define CONFIG_LOADS_ECHO		1		/* echo on for serial download */
#define CONFIG_SYS_LOADS_BAUD_CHANGE	1		/* allow baudrate change */

#define CONFIG_API
#define CONFIG_SYS_MMC_MAX_DEVICE	1
#define CONFIG_DOS_PARTITION
#include <config_cmd_default.h>

#define CONFIG_CMD_IRQ
#define CONFIG_CMD_PING
#define CONFIG_CMD_I2C
#define CONFIG_CMD_MII
#define CONFIG_CMD_PCI
#define CONFIG_CMD_ELF
#define CONFIG_CMD_SETEXPR
#define CONFIG_CMD_NET
#define CONFIG_CMD_EEPROM
#define CONFIG_SYS_I2C_EEPROM_ADDR_LEN  1
#define CMD_SAVEENV
#define CONFIG_CMD_FLASH

#define CONFIG_HWCONFIG

/* USB */
#define CONFIG_CMD_USB
#define CONFIG_USB_STORAGE
#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_FSL
#define CONFIG_EHCI_HCD_INIT_AFTER_RESET
#define CONFIG_USB_SHOW_NO_PROGRESS
#define USB_WRITE_READ			/* enable write and read for usb memory */
#define CONFIG_START_USB

#define __USB_PHY_TYPE utmi

#define USB_MULTI_CONTROLLER		/* JUNOS */

#undef  CONFIG_WATCHDOG
#define CONFIG_EPLD_WATCHDOG		/* EPLD watchdog */
#define CONFIG_WATCHDOG_EXCEPTION

/* PIC */
#define IRQ0_INT_VEC			0x1200
#define IRQ1_INT_VEC			0x1201
#define IRQ2_INT_VEC			0x1202
#define IRQ3_INT_VEC			0x1203
#define IRQ4_INT_VEC			0x1204
#define IRQ5_INT_VEC			0x1205
#define IRQ6_INT_VEC			0x1206
#define IRQ7_INT_VEC			0x1207
#define IRQ8_INT_VEC			0x1208
#define IRQ9_INT_VEC			0x1209
#define IRQ10_INT_VEC			0x120A
#define IRQ11_INT_VEC			0x120B

/* LCD + LED */
#define CONFIG_SHOW_ACTIVITY
#define CFG_LCD_NUM			1

/* Miscellaneous configurable options */
#define CONFIG_SYS_LONGHELP				/* undef to save memory     */
#define CONFIG_CMDLINE_EDITING				/* Command-line editing */
#define CONFIG_SYS_LOAD_ADDR		0x2000000	/* default load address */
#define CONFIG_SYS_PROMPT		"=> "		/* Monitor Command Prompt */
#if defined(CONFIG_CMD_KGDB)
#define CONFIG_SYS_CBSIZE		1024		/* Console I/O Buffer Size */
#else
#define CONFIG_SYS_CBSIZE		256		/* Console I/O Buffer Size */
#endif
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE \
					+ sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_MAXARGS		16		/* max number of command args */
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE	/* Boot Argument Buffer Size */
#define CONFIG_SYS_HZ			1000		/* decrementer freq 1ms ticks */

#define CONFIG_SYS_CACHE_STASHING
#define CONFIG_BACKSIDE_L2_CACHE
#define CONFIG_SYS_INIT_L2CSR0		L2CSR0_L2E
#define CONFIG_BTB			/* toggle branch predition */
#define CONFIG_SYS_CACHELINE_SIZE	64
#define CFG_CACHELINE_SIZE		64
#define CFG_DCACHE_SIZE			32768
#define CONFIG_SYS_DCACHE_SIZE		32768

#define CONFIG_BOARD_EARLY_INIT_F	1		/* call board_pre_init */
#define CONFIG_BOARD_EARLY_INIT_R	1		/* call board_early_init_r function */
#define CONFIG_MISC_INIT_R		1		/* call misc_init_r() */

#define CONFIG_ADDR_MAP
#define CONFIG_SYS_NUM_ADDR_MAP		64		/* number of TLB1 entries */

/* Ports */
#define NUM_EXT_PORTS			52

/*
 * Internal Definitions
 *
 * Boot Flags
 */
#define BOOTFLAG_COLD		0x01	/* Normal Power-On: Boot from FLASH */
#define BOOTFLAG_WARM		0x02	/* Software reboot */

#if defined(CONFIG_CMD_KGDB)
#define CONFIG_KGDB_BAUDRATE		230400		/* speed to run kgdb serial port */
#define CONFIG_KGDB_SER_INDEX		2		/* which serial port to use */
#endif

/* Enable an alternate, more extensive memory test */
#define CONFIG_SYS_ALT_MEMTEST
#define CONFIG_SYS_MEMTEST_START	0x00200000
#define CONFIG_SYS_MEMTEST_END		0x00400000

/*
 * Base addresses -- Note these are effective addresses where the
 * actual resources get mapped (not physical addresses)
 */
#define CONFIG_SYS_CCSRBAR_DEFAULT	0xfe000000	/* CCSRBAR Default */
#define CONFIG_SYS_CCSRBAR		0xfe000000	/* relocated CCSRBAR */
#ifdef CONFIG_PHYS_64BIT
#define CONFIG_SYS_CCSRBAR_PHYS		0xffe000000ull
#else
#define CONFIG_SYS_CCSRBAR_PHYS		CONFIG_SYS_CCSRBAR
#endif
/* PQII uses CONFIG_SYS_IMMR */
#define CONFIG_SYS_IMMR			CONFIG_SYS_CCSRBAR

/* Set the local bus clock 1/8 of platform clock */
#define CONFIG_SYS_LBC_LCRR		LCRR_CLKDIV_8

/*
 * When initializing flash, if we cannot find the manufacturer ID,
 * assume this is the AMD flash associated with the CDS board.
 * This allows booting from a promjet.
 */
#define CONFIG_ASSUME_AMD_FLASH
#define CONFIG_SYS_MAX_FLASH_BANKS	1		/* number of banks */
#define CONFIG_SYS_MAX_FLASH_SECT	142
#define CONFIG_SYS_FLASH_ERASE_TOUT	60000		/* Flash Erase Timeout (ms) */
#define CONFIG_SYS_FLASH_WRITE_TOUT	500		/* Flash Write Timeout (ms) */

#define CONFIG_SYS_EPLD_BASE		0xff000000
#define CONFIG_SYS_FLASH_BASE		0xff800000	/* start of FLASH 8M */

#define CONFIG_SYS_LBC_BASE		0xff000000
#define CONFIG_SYS_LBC_BASE_PHYS	CONFIG_SYS_LBC_BASE
#define CONFIG_SYS_FLASH_BASE_PHYS	CONFIG_SYS_FLASH_BASE
#define CONFIG_SYS_EPLD_BASE_PHYS	CONFIG_SYS_EPLD_BASE

/*
 * FLASH on the Local Bus
 * One banks, 8M, using the CFI driver.
 * Boot from BR0/OR0 bank at 0xff80_0000
 *
 * BR0:
 *    BA (Base Address) = 0xff80_0000 = BR0[0:16] = 1111 1111 1000 0000 0
 *    XBA (Address Space eXtent) = BR0[17:18] = 00
 *    PS (Port Size) = 8 bits = BR0[19:20] = 01
 *    DECC = disable parity check = BR0[21:22] = 00
 *    WP = enable both read and write protect = BR0[23] = 0
 *    MSEL = use GPCM = BR0[24:26] = 000
 *    ATOM = not use atomic operate = BR0[28:29] = 00
 *    Valid = BR0[31] = 1
 *
 * 0      4      8     12    16     20    24     28
 * 1111 1111 1000 0000 0000 1000 0000 0001 = ff800801    BR0
 *
 * OR0:
 *    Addr Mask = 8M = OR0[0:16] = 1111 1111 1000 0000 0
 *    Reserved OR0[17:18] = 00
 *    BCTLD = buffer control not assert = OR0[19] = 1
 *    CSNT = chip select /CS and /WE negated = OR0[20] = 0
 *    ACS = address to CS at same time as address line = OR0[21:22] = 00
 *    XACS = extra address setup = OR0[23] = 0
 *    SCY = 7 clock wait states = OR0[24:27] = 0100
 *    SETA (External Transfer Ack) = OR0[28] = 0
 *    TRLX = use relaxed timing = OR0[29] = 0
 *    EHTR (Extended hold time on read access) = OR0[30] = 0
 *    EAD = use external address latch delay = OR0[31] = 0
 *
 * 0      4      8     12    16     20    24     28
 * 1111 1111 1000 0000 0001 0000 0100 0000 = ff801040    OR0
 */

#define CONFIG_FLASH_BR_PRELIM		(BR_PHYS_ADDR(CONFIG_SYS_FLASH_BASE_PHYS) \
					| BR_PS_8 | BR_V)
#define CONFIG_FLASH_OR_PRELIM		0xff806e65
#define CONFIG_EPLD_BR_PRELIM		(BR_PHYS_ADDR(CONFIG_SYS_EPLD_BASE_PHYS) \
					| BR_PS_16 | BR_V)
#define CONFIG_EPLD_OR_PRELIM		0xffff0e45
#define CONFIG_SYS_BR0_PRELIM		CONFIG_FLASH_BR_PRELIM
#define CONFIG_SYS_OR0_PRELIM		((0xff800ff7 & ~OR_GPCM_SCY & ~OR_GPCM_EHTR) \
					| OR_GPCM_SCY_8 | OR_GPCM_EHTR_CLEAR)
#define CONFIG_SYS_BR1_PRELIM		CONFIG_EPLD_BR_PRELIM
#define CONFIG_SYS_OR1_PRELIM		CONFIG_EPLD_OR_PRELIM

/* Serial Port */
#define CONFIG_CONS_INDEX		1
#define CONFIG_SYS_NS16550
#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_REG_SIZE	1
#define CONFIG_SYS_NS16550_CLK		(get_bus_freq(0)/2)
#define CONFIG_CONSOLE_SELECT		1

#define CONFIG_SYS_BAUDRATE_TABLE	\
	{300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200}

#define CONFIG_SYS_NS16550_COM1		(CONFIG_SYS_CCSRBAR+0x11C500)
#define CONFIG_SYS_NS16550_COM2		(CONFIG_SYS_CCSRBAR+0x11C600)
#define CONFIG_SYS_NS16550_COM3		(CONFIG_SYS_CCSRBAR+0x11D500)
#define CONFIG_SYS_NS16550_COM4		(CONFIG_SYS_CCSRBAR+0x11D600)

/* Use the HUSH parser */
#define CONFIG_SYS_HUSH_PARSER
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "

/* I2C */
#define CONFIG_FSL_I2C			/* Use FSL common I2C driver */
#define CONFIG_HARD_I2C			/* I2C with hardware support */
#define CONFIG_I2C_MULTI_BUS
#define CONFIG_SYS_I2C_SPEED		100000
#define CONFIG_SYS_I2C_SLAVE		0x7F
#define CONFIG_SYS_I2C_EEPROM_ADDR	0x57
#define CONFIG_SYS_I2C_EEPROM_ADDR_LEN	1
#define CONFIG_SYS_I2C_OFFSET		0x118000
#define CONFIG_SYS_I2C2_OFFSET		0x118100
#define CONFIG_I2C_MUX
//#define CONFIG_FSL_I2C_CUSTOM_DFSR      0x10
//#define CONFIG_FSL_I2C_CUSTOM_FDR       0x0E

#define CFG_EEPROM_MAC_OFFSET		56
#define CONFIG_LAST_STAGE_INIT

/* main board I2C */
#define CFG_I2C_CTRL_1			0
#define CFG_I2C_CTRL_2			1

/* I2C controller 1 */
#define CFG_I2C_C1_9548SW1			0x70	/* MUX 1 */
#define CFG_I2C_C1_9548SW1_P0_MAX6581		0x4D	/* MAX6581 */
#define CFG_I2C_C1_9548SW1_P1_UCD9090		0x34	/* UCD9090 */
#define CFG_I2C_C1_9548SW1_P2_R5H30211_UPLINK	0x72	/* R5H30211_UPLINK */
#define CFG_I2C_C1_9548SW1_P3_RTC		0x68	/* RTC DS1672 */
#define CFG_I2C_C1_9548SW1_P4_R5H30211		0x72	/* R5H30211 */
#define CFG_I2C_C1_9548SW1_P5_VOLT_MONITOR	0x40	/* INA219 VOLT MONITOR */
#define CFG_I2C_C1_9548SW1_P6_QSFP_I2C_PORTS	0x50	/* QSFP I2C PORTS */
#define CFG_I2C_C1_9548SW1_P7_DDR_DIMM		0x55	/* DDR DIMM */

/* I2C controller 2 */
#define CFG_I2C_C2_9548SW1			0x70	/* MUX 2 */
#define CFG_I2C_C2_9548SW1_P0_PSU1_1		0x50	/* PSU 1 */
#define CFG_I2C_C2_9548SW1_P0_PSU1_2		0x58	/* PSU 1 */
#define CFG_I2C_C2_9548SW1_P1_PSU2_1		0x50	/* PSU 2 */
#define CFG_I2C_C2_9548SW1_P1_PSU2_2		0x58	/* PSU 2 */
#define CFG_I2C_C2_9548SW1_P2_VIRTUAL_PWR	0x54	/* VIRTUAL PWR */
#define CFG_I2C_C2_9548SW1_P3_POE		0x20	/* POE CONTROLLER */

/*
 * main board EEPROM platform ID
 */
#define I2C_ID_JUNIPER_EX4300_24T	0x0B5D		/* 24x1G RJ45 */
#define I2C_ID_JUNIPER_EX4300_48T	0x0B5C		/* 48x1G RJ45 */
#define I2C_ID_JUNIPER_EX4300_48_P	0x0B5E		/* 48x1G PoE+ RJ45 switch */
#define I2C_ID_JUNIPER_EX4300_24_P	0x0B5F		/* 24x1G RJ45 PoE+ switch */
#define I2C_ID_JUNIPER_EX4300_48_T_BF	0x0B60		/* 48x1G RJ45 back-to-front airflow switch */
#define I2C_ID_JUNIPER_EX4300_48_T_DC	0x0B61		/* 48x1G RJ45 DC power switch */
#define I2C_ID_JUNIPER_EX4300_32F       0x0BD6      /* 32x1G SFP switch */

/* i2c eeprom/board types */
#define CONFIG_BOARD_TYPES		1
  
/* PCIE */
#define CONFIG_SYS_PCIE2_MEM_VIRT	0xE0000000
#define CONFIG_SYS_PCIE2_MEM_BUS	0xE0000000
#define CONFIG_SYS_PCIE2_MEM_PHYS	0xE0000000
#define CONFIG_SYS_PCIE2_MEM_SIZE	0x10000000	/* 256M */
#define CONFIG_SYS_PCIE2_IO_VIRT	0xf8020000
#define CONFIG_SYS_PCIE2_IO_BUS		0x00000000
#define CONFIG_SYS_PCIE2_IO_PHYS	0xf8020000
#define CONFIG_SYS_PCIE2_IO_SIZE	0x00100000	/* 1M */
#define CONFIG_PCI			/* Enable PCI/PCIE */
#define CONFIG_PCIE2			/* PCIE controler 2 */
#define CONFIG_FSL_PCI_INIT		/* Use common FSL init code */
#define CONFIG_FSL_PCIE_RESET		1	/* need PCIe reset errata */

#if defined(CONFIG_PCI)

#define CONFIG_PCI_PNP			/* do pci plug-and-play */
#undef CONFIG_85XX_PCI2

#undef CONFIG_PCI_SCAN_SHOW
#define CFG_PCI_SUBSYS_VENDORID		0x1957		/* Freescale */

#endif  /* CONFIG_PCI */

/* DPAA */
#define CONFIG_SYS_DPAA_FMAN

#ifdef CONFIG_SYS_DPAA_FMAN
#define CONFIG_FMAN_ENET
#define CONFIG_PHY_BROADCOM
#define CONFIG_PHYLIB_10G
#endif

#ifdef CONFIG_FMAN_ENET
#define CONFIG_SYS_FM1_DTSEC1_PHY_ADDR		0x0
#define CONFIG_SYS_TBIPA_VALUE			8
#define CONFIG_MII				/* MII PHY management */
#define CONFIG_ETHPRIME				"FM1@DTSEC1"
#define CONFIG_PHY_GIGE				/* Include GbE speed/duplex detection */
#endif

#define CONFIG_SYS_RX_ETH_BUFFER	8
#define CONFIG_NET_MULTI

#define CONFIG_ETHADDR			00:E0:0C:00:00:3C
#define CONFIG_HAS_ETH1
#define CONFIG_ETH1ADDR			00:E0:0C:00:00:3D
#define CONFIG_HAS_ETH2
#define CONFIG_ETH2ADDR			00:E0:0C:00:00:3E
#define CONFIG_HAS_ETH3
#define CONFIG_ETH3ADDR			00:E0:0C:00:00:3F

/* default location for tftp and bootm */
#define CONFIG_LOADADDR			0x400000
#define CONFIG_BOOTDELAY		1	/* -1 disables auto-boot */
#undef  CONFIG_BOOTARGS
#define CONFIG_BAUDRATE			9600

#define CONFIG_AUTOBOOT_KEYED		/* use key strings to stop autoboot */

/*
* Use ctrl-c to stop booting and enter u-boot command mode.
* ctrl-c is common but not too easy to interrupt the booting by accident.
*/
#define CONFIG_AUTOBOOT_STOP_STR	"\x03"
#define CONFIG_SILENT_CONSOLE		1

#define CONFIG_EXTRA_ENV_SETTINGS			\
		"hwconfig=fsl_ddr:ctlr_intlv=cacheline,"\
		"usb1:dr_mode=host,phy_type=" MK_STR(__USB_PHY_TYPE) "\0"

#define CONFIG_2NDSTAGE_BOOTCOMMAND		\
	"cp.b 0xFFF00000 ${loadaddr} 0x40000;"	\
	"bootelf ${loadaddr}"

#define CONFIG_BOOTCOMMAND		CONFIG_2NDSTAGE_BOOTCOMMAND

#define CONFIG_BOOTCOMMAND_BANK1		\
	"cp.b 0xFFB00000 ${loadaddr} 0x40000;"	\
	"bootelf ${loadaddr}"

#define CFG_UPGRADE_BOOT_STATE_OFFSET	0xA0000		/* from beginning of flash */
#define CFG_UPGRADE_BOOT_SECTOR_SIZE	0x10000		/* 64k */

#define CFG_IMG_HDR_OFFSET		0x30		/* from image address */
#define CFG_CHKSUM_OFFSET		0x100		/* from image address */

#define CONFIG_UBOOT_UPGRADE		1
#define CFG_UPGRADE_LOADER_BANK1	0xFFB00000
#define CFG_UPGRADE_LOADER_BANK0	0xFFF00000
#define CFG_UPGRADE_UBOOT_BANK1		0xFFB80000
#define CFG_UPGRADE_UBOOT_BANK0		0xFFF80000
/* check for ative partition and swizzle flash bank */
#define CONFIG_FLASH_SWIZZLE
#define CONFIG_BOOT_STATE		/* maintain boot/upgrade states */

#define CONFIG_COMMANDS		(CONFIG_CMD_DFL \
				| CFG_CMD_PCI \
				| CFG_CMD_PING \
				| CFG_CMD_I2C \
				| CFG_CMD_MII \
				| CFG_CMD_USB \
				| CFG_CMD_CACHE \
				| CFG_CMD_ELF \
				| CFG_CMD_EEPROM \
				| CFG_CMD_BSP)

#define CONFIG_BOARD_RESET	1	/* board specific reset function */

#define CFG_VERSION_MAJOR	1
#define CFG_VERSION_MINOR	0
#define CFG_VERSION_PATCH	0

/*
 * Till the time we have proper support for HIGH MEM,
 * these work arounds are required
 */
#define CONFIG_RESTRICT_RAM
#define	CONFIG_MAX_MEM_MAPPED	0xC0000000

#endif  /* __CONFIG_H */

