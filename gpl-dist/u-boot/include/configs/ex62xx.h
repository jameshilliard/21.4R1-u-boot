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
 * <http://www.gnu.org/licenses/gpl-2.0.html>.
 */

/*
 * ex62xx board configuration file
 */
#ifndef __CONFIG_H
#define __CONFIG_H

#define CONFIG_PRODUCTION

/* High Level Configuration Options */
#define CONFIG_BOOKE		1	/* BOOKE */
#define CONFIG_E500			1	/* BOOKE e500 family */
#define CONFIG_MPC85xx		1	/* MPC8540/60/55/41/48 */
#define CONFIG_P2020		1
#define CONFIG_P2020DS		1
#define CONFIG_EX62XX		1	/* ex62xx board */
#define	CONFIG_PRODUCT_EXSERIES	1	/* EX series product */

#define CONFIG_FSL_ELBC		/* Has Enhanced localbus controller */
#define CONFIG_PCI			/* Enable PCI/PCIE */
#define CONFIG_PCIE2		1	/* PCIE controler 2 (slot 2) */
#define CONFIG_FSL_PCI_INIT	1	/* Use common FSL init code */
#define CONFIG_FSL_PCIE_RESET	1	/* need PCIe reset errata */

#define CONFIG_FSL_LAW		1	/* Use common FSL init code */

#define CONFIG_TSEC_ENET 	/* tsec ethernet support */
#define CONFIG_ENV_OVERWRITE

#define CONFIG_SYS_CLK_FREQ		66666666	/* sysclk */
#define CONFIG_DDR_CLK_FREQ		66666666	/* ddrclk */

/*
 * These can be toggled for performance analysis, otherwise use default.
 */
#define CONFIG_L2_CACHE			/* toggle L2 cache 	*/
#define CONFIG_BTB			    /* toggle branch predition */

#define CONFIG_ENABLE_36BIT_PHYS	1

#define CONFIG_SYS_ALT_MEMTEST
#define CONFIG_SYS_MEMTEST_START	0x10000000	/* memtest works on */
#define CONFIG_SYS_MEMTEST_END		0x7fffffff

/*
 * Base addresses -- Note these are effective addresses where the
 * actual resources get mapped (not physical addresses)
 */
#define CONFIG_SYS_CCSRBAR_DEFAULT	0xff700000	/* CCSRBAR Default */
#define CONFIG_SYS_CCSRBAR			0xfef00000	/* relocated CCSRBAR */
/* physical addr of CCSRBAR */
#define CONFIG_SYS_CCSRBAR_PHYS		CONFIG_SYS_CCSRBAR
/* PQII uses CONFIG_SYS_IMMR */
#define CONFIG_SYS_IMMR				CONFIG_SYS_CCSRBAR

#define CONFIG_SYS_PCIE2_MEM_BUS	0xa0000000
#define CONFIG_SYS_PCIE2_MEM_PHYS	0xa0000000
#define CONFIG_SYS_PCIE2_MEM_SIZE	0x20000000      /* 512M */
#define CONFIG_SYS_PCIE2_IO_BUS		0x00000000
#define CONFIG_SYS_PCIE2_IO_PHYS	0xffc10000
#define CONFIG_SYS_PCIE2_IO_SIZE	0x00010000      /* 64k */

/* DDR Setup */
#define CONFIG_VERY_BIG_RAM
#define CONFIG_FSL_DDR3		1
#undef CONFIG_FSL_DDR_INTERACTIVE
/* 
 * force u-boot relocation address & stack to be within 1GB
 * of RAM so that u-boot and pre-maped kernel address can share the same
 * TLB entry.. 
 * [XXX:ex:- if RAM sz > 1GB and if u-boot relocates to top of the memory @ <2GB, 
 * loader will invalidate the TLB entry that maps 0-1GB where kernel will be
 * loaded. This will cause Instruction TLB error]
 */
#define CONFIG_MEM_RELOCATE_TOP	    (1024 << 20)    /* relocate u-boot at the top of 1GB */
/* 
 * Assign fixed tlb index for mapping RAM.
 * Index should be > 4.
 * loader uses tlb1 entries[0-4] for pre-mapping the kernel
 */
#define CONFIG_RAM_TLB_INDEX	8

#define	CONFIG_DDR_ECC
#define CONFIG_ECC_INIT_VIA_DDRCONTROLLER
#define CONFIG_MEM_INIT_VALUE	0x00000000

#define CONFIG_SYS_LC_SDRAM_SIZE		1024
#define CONFIG_SYS_SDRAM_SIZE			CONFIG_SYS_LC_SDRAM_SIZE
#define CONFIG_SYS_DDR3_CS0_BNDS		0x0000001F
#define CONFIG_SYS_DDR3_CS0_CONFIG		0x80014202 	/* 14 row, 10 col */
#define CONFIG_SYS_DDR3_CS0_CONFIG_2		0x00000000

#define CONFIG_SYS_DDR3_CS1_BNDS		0x0020003F
#define CONFIG_SYS_DDR3_CS1_CONFIG		0x80014202 	/* 14 row, 10 col */
#define CONFIG_SYS_DDR3_CS1_CONFIG_2		0x00000000

#define CONFIG_SYS_DDR3_TIMING_0		0x00330104
#define CONFIG_SYS_DDR3_TIMING_1		0x5c5b0644   /* 0x6f6b4846 */
#define CONFIG_SYS_DDR3_TIMING_2		0x0fa880cf
#define CONFIG_SYS_DDR3_TIMING_3		0x00020000	
#define CONFIG_SYS_DDR3_TIMING_4		0x00000001
#define CONFIG_SYS_DDR3_TIMING_5		0x00001600
#define CONFIG_SYS_DDR3_MODE			0x00401420
#define CONFIG_SYS_DDR3_MODE_2			0x04000000
#define CONFIG_SYS_DDR3_MD_CNTL 		0x00000000
//#define CONFIG_SYS_DDR3_INTERVAL		0x08200100	/* 533 DDR data rete */
#define CONFIG_SYS_DDR3_INTERVAL		0x0A280100
#define CONFIG_SYS_DDR3_DATA_INIT		0x00000000
#define CONFIG_SYS_DDR3_CLK_CNTL		0x02800000
#define CONFIG_SYS_DDR3_INIT_ADDR		0x00000000
#define CONFIG_SYS_DDR3_INIT_EXT_ADDR		0x00000000
#define CONFIG_SYS_DDR3_ZQ_CNTL 		0x89080600
#define CONFIG_SYS_DDR3_WRLVL_CNTL		0x8645a607
#define CONFIG_SYS_DDR3_SR_CNTR 		0x00000000
#define CONFIG_SYS_DDR3_RCW_1			0x00050000
#define CONFIG_SYS_DDR3_RCW_2			0x00000000
#define CONFIG_SYS_SDRAM_CFG			0xe70c0008
#define CONFIG_SYS_SDRAM_CFG_2			0x24401011	/* MD_EN - mirrored DIMM */
#define CONFIG_SYS_SDRAM_CFG_INITIAL		0x670c0008
#define CONFIG_SYS_SDRAM_CFG_NO_ECC		0xe70c0008
#define CONFIG_SYS_DDR3_IP_REV1 		0x00020403
#define CONFIG_SYS_DDR3_IP_REV2 		0x00000100
#define CONFIG_SYS_DDR3_ERR_INT_EN		0x00000000
#define CONFIG_SYS_DDR3_CDR1			0x00000000
#define CONFIG_SYS_DDR3_CDR2			0x00000000
#define SDRAM_CFG_MEM_EN			0x80000000
#define CONFIG_SYS_DDR3_ERR_DISABLE		0x00000000
#define CONFIG_SYS_DDR3_ERR_SBE 		0x00000000
#define CONFIG_SYS_DDR3_DINIT			0x00000010



#define CONFIG_SYS_DDR_SDRAM_BASE	0x00000000
#define CONFIG_SYS_SDRAM_BASE		CONFIG_SYS_DDR_SDRAM_BASE

#define CONFIG_NUM_DDR_CONTROLLERS		1
#define CONFIG_DIMM_SLOTS_PER_CTLR		1
#define CONFIG_CHIP_SELECTS_PER_CTRL		2

/* program ddrcdr register */
#define CONFIG_SYS_DDRCDR1_VALUE	0x000c0000

#define CONFIG_SPD_EEPROM		/* Use SPD EEPROM for DDR setup*/
#define CONFIG_DDR_SPD
/* I2C addresses of SPD EEPROMs */
#define CONFIG_SYS_SPD_BUS_NUM	0		/* SPD EEPROM located on I2C bus 0 */
#define SPD_EEPROM_ADDRESS		0x50	/* CTLR 0 DIMM 0 */

#undef CONFIG_CLOCKS_IN_MHZ

/*
 * Memory map
 *
 * 0x0000_0000	0x7fff_ffff	DDR			2G Cacheable
 * 0x8000_0000	0xbfff_ffff	PCI Express Mem		1G non-cacheable
 *
 * Localbus (0xff00_0000	0xffff_ffff		16M)
 *
 * 0xff00_0000	0xff0f_ffff	SCB bootcpld	1M
 * 0xff10_0000	0xff1f_ffff	LC control cpld	1M
 * 0xff20_0000	0xff2f_ffff	LC bootcpld		1M
 * 0xff30_0000	0xff30_ffff	lbc UART 0			64K
 * 0xff31_0000	0xff31_ffff	lbc UART 1			64K
 * 0xff40_0000	0xff4f_ffff	SCB cntl FPGA	1M
 * 0xff70_0000	0xff73_ffff NAND_CE1		256K
 * 0xff74_0000	0xff77_ffff NAND_CE2		256K
 *
 * 0xffe0_0000	0xffef_ffff	CCSR			1M
 */

/*
 * Local Bus Definitions
 */

#define CONFIG_SYS_LBC_BASE			0xff000000
#define CONFIG_SYS_LBC_BASE_PHYS	CONFIG_SYS_LBC_BASE

/* Boot flash */
#define CONFIG_SYS_FLASH_BASE		0xff800000	/* start of FLASH 8M */
#define CONFIG_SYS_FLASH_BASE_PHYS	CONFIG_SYS_FLASH_BASE

#define CONFIG_FLASH_BR_PRELIM	(BR_PHYS_ADDR(CONFIG_SYS_FLASH_BASE_PHYS) | BR_PS_8 | BR_V)
#define CONFIG_FLASH_OR_PRELIM	0xff806e65

#define CONFIG_SYS_FLASH_BANKS_LIST	{CONFIG_SYS_FLASH_BASE}
#define CONFIG_SYS_FLASH_QUIET_TEST
#define CONFIG_FLASH_SHOW_PROGRESS	45	/* count down from 45/5: 9..1 */

#define CONFIG_SYS_MAX_FLASH_BANKS	1		/* number of banks */
#define CONFIG_SYS_MAX_FLASH_SECT	142		/* sectors per device */
#undef	CONFIG_SYS_FLASH_CHECKSUM
#define CONFIG_SYS_FLASH_ERASE_TOUT	60000	/* Flash Erase Timeout (ms) */
#define CONFIG_SYS_FLASH_WRITE_TOUT	500		/* Flash Write Timeout (ms) */

#define CONFIG_SYS_MONITOR_BASE	CONFIG_SYS_TEXT_BASE	/* start of monitor */

#define CONFIG_FLASH_CFI_DRIVER
#define CONFIG_SYS_FLASH_CFI
#define CONFIG_SYS_FLASH_EMPTY_INFO

/* NAND flash */
#define CONFIG_SYS_NAND_BASE		0xff700000
#define CONFIG_SYS_NAND_BASE_PHYS	CONFIG_SYS_NAND_BASE
#define CONFIG_SYS_NAND_BASE_LIST	{ CONFIG_SYS_NAND_BASE,\
				CONFIG_SYS_NAND_BASE + 0x40000}

#define CONFIG_SYS_MAX_NAND_DEVICE	2
#define CONFIG_MTD_NAND_VERIFY_WRITE
#define CONFIG_CMD_NAND				1
#define CONFIG_NAND_FSL_ELBC		1
#define CONFIG_SYS_NAND_BLOCK_SIZE	(128 * 1024)

/* NAND flash config */
#define CONFIG_NAND_BR1_PRELIM	(BR_PHYS_ADDR(CONFIG_SYS_NAND_BASE_PHYS) \
				| (2 << BR_DECC_SHIFT)	/* Use HW ECC */ \
				| BR_PS_8		/* Port Size = 8bit */ \
				| BR_MS_FCM		/* MSEL = FCM */ \
				| BR_V)			/* valid */
#define CONFIG_NAND_OR_PRELIM	(0xFFFC0000	/* length 256K */ \
				| OR_FCM_PGS	/* Large Page*/ \
				| OR_FCM_CSCT \
				| OR_FCM_CST \
				| OR_FCM_CHT \
				| OR_FCM_SCY_1 \
				| OR_FCM_TRLX \
				| OR_FCM_EHTR)

#define CONFIG_NAND_BR2_PRELIM  (BR_PHYS_ADDR((CONFIG_SYS_NAND_BASE_PHYS + 0x40000)) \
				| (2 << BR_DECC_SHIFT)	/* Use HW ECC */ \
				| BR_PS_8		/* Port Size = 8bit */ \
				| BR_MS_FCM		/* MSEL = FCM */ \
				| BR_V)			/* valid */


/* cpld/fpga */
#define CFG_SCB_BTCPLD_BASE		0xff000000
#define CFG_LC_BTCPLD_BASE		0xff200000
#define CFG_LC_CTLCPLD_BASE		0xff100000
#define CFG_LC_XCPLD_BASE		0xff400000
#define CFG_JCBC_FPGA_BASE		0xff400000

/* local bus DUART */
#define CFG_LBC_UART0			0xff300000
#define CFG_LBC_UART1			0xff310000


#define CONFIG_SYS_INIT_RAM_LOCK	1
#define CONFIG_SYS_INIT_RAM_ADDR	0xe4010000	/* Initial L1 address */
#define CONFIG_SYS_INIT_RAM_END		0x4000	/* End of used area in RAM */


#define CONFIG_SYS_GBL_DATA_SIZE	128	/* num bytes initial data */
#define CONFIG_SYS_GBL_DATA_OFFSET	(CONFIG_SYS_INIT_RAM_END \
			- CONFIG_SYS_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_OFFSET	CONFIG_SYS_GBL_DATA_OFFSET

#define CONFIG_SYS_MONITOR_LEN		(512 * 1024) /* Reserve 256 kB for Mon */
#define CONFIG_SYS_MALLOC_LEN		(1024 * 1024)	/* Reserved for malloc */


/* chip select configs */
#define CONFIG_SYS_BR0_PRELIM  CONFIG_FLASH_BR_PRELIM	/* NOR Base Address */
#define CONFIG_SYS_OR0_PRELIM  CONFIG_FLASH_OR_PRELIM	/* NOR Options */
#define CONFIG_SYS_BR1_PRELIM  CONFIG_NAND_BR1_PRELIM	/* NAND Base Address */
#define CONFIG_SYS_OR1_PRELIM  CONFIG_NAND_OR_PRELIM	/* NAND Options */
#define CONFIG_SYS_BR2_PRELIM  CONFIG_NAND_BR2_PRELIM	/* NAND Base Address */
#define CONFIG_SYS_OR2_PRELIM  CONFIG_NAND_OR_PRELIM	/* NAND Options */

#define CONFIG_SYS_LC_BR2_PRELIM  (BR_PHYS_ADDR(CFG_LC_CTLCPLD_BASE) \
				  | BR_PS_8 \
				  | BR_V)
#define CONFIG_SYS_LC_OR2_PRELIM  0xfff00000

#define CONFIG_SYS_LC_BR3_PRELIM  (BR_PHYS_ADDR(CFG_LC_BTCPLD_BASE) \
				  | BR_PS_8 \
				  | BR_V)
#define CONFIG_SYS_LC_OR3_PRELIM  0xfff00000

/* ex62xx 48F xcpld - cs4 */
#define CONFIG_SYS_XCPLD_BR4_PRELIM	(BR_PHYS_ADDR(CFG_LC_XCPLD_BASE) \
				  | BR_PS_8 \
				  | BR_V)

#define CONFIG_SYS_XCPLD_OR4_PRELIM	0xfff00000

/* scb boot cpld - cs4 */
#define CONFIG_SYS_BR4_PRELIM	(BR_PHYS_ADDR(CFG_SCB_BTCPLD_BASE) \
				| BR_PS_8 \
				| BR_V)
#define CONFIG_SYS_OR4_PRELIM	0xfff00000

/* scb ctl fpgs - cs5 */
#define CONFIG_SYS_BR5_PRELIM	(BR_PHYS_ADDR(CFG_JCBC_FPGA_BASE) \
				| BR_PS_16 \
				| BR_V)
#define CONFIG_SYS_OR5_PRELIM	0xfff00000

/* lbc uart 0 */
#define CONFIG_SYS_BR6_PRELIM	(BR_PHYS_ADDR(CFG_LBC_UART0) \
				| BR_PS_8 \
				| BR_V)
#define CONFIG_SYS_OR6_PRELIM	0xffff0835
/* lbc uart 1 */
#define CONFIG_SYS_BR7_PRELIM	(BR_PHYS_ADDR(CFG_LBC_UART1) \
				| BR_PS_8 \
				| BR_V)
#define CONFIG_SYS_OR7_PRELIM	0xffff0835


/* Serial Port */
#define CONFIG_CONS_INDEX	1
#undef	CONFIG_SERIAL_SOFTWARE_FIFO
#define CONFIG_SYS_NS16550
#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_REG_SIZE	1
#define CONFIG_SYS_NS16550_CLK		get_bus_freq(0)

#define CONFIG_SYS_NS16550_COM1	(CONFIG_SYS_CCSRBAR + 0x4500)
#define CONFIG_SYS_NS16550_COM2	(CONFIG_SYS_CCSRBAR + 0x4600)
#define CONFIG_SILENT_CONSOLE
#define CONFIG_SYS_CONSOLE_INFO_QUIET

#define CONFIG_SYS_BAUDRATE_TABLE \
	{300, 600, 1200, 2400, 4800, 9600, 19200, 38400,115200}

/* Use the HUSH parser */
#define CONFIG_SYS_HUSH_PARSER
#ifdef	CONFIG_SYS_HUSH_PARSER
#define CONFIG_SYS_PROMPT_HUSH_PS2 "> "
#endif


/* I2C */
#define CONFIG_FSL_I2C		/* Use FSL common I2C driver */
#define CONFIG_HARD_I2C		/* I2C with hardware support */
#undef	CONFIG_SOFT_I2C		/* I2C bit-banged */
#define CONFIG_I2C_MULTI_BUS
#define CONFIG_SYS_I2C_SPEED		100000	/* I2C speed and slave address */
#define CONFIG_SYS_I2C_EEPROM_ADDR	0x57
#define CONFIG_SYS_I2C_SLAVE		0x7F
#define CONFIG_SYS_I2C_OFFSET		0x3000
#define CONFIG_SYS_I2C2_OFFSET		0x3100

#define CONFIG_I2C_MUX

#define CONFIG_I2C_BUS_SMB		0
#define CONFIG_I2C_BUS_MASTER	1

/* i2c eeprom/board types */
#define CONFIG_BOARD_TYPES	1

#define EX62XX_LOCAL_ID_EEPROM_ADDR		0x57
#define EX62XX_SCB_ID_EEPROM_ADDR		0x51
#define EX62XX_LC_ID_EEPROM_ADDR		0x51
#define SCB_SLAVE_CPLD_ADDR				0x54

#define EX62XX_ASSEMBLY_ID_OFFSET		0x04

#define EX62XX_LC48T_ASSEMBLY_ID	0x09E3
#define EX62XX_LC48P_ASSEMBLY_ID	0x09E4
#define EX62XX_LC48F_ASSEMBLY_ID	0x0B9A
#define EX6208_JSCB_ASSEMBLY_ID		0x09E5
#define EX6208_CHASSIS_ID			0x0549


#if defined(CONFIG_TSEC_ENET)

#define CONFIG_NET_MULTI	1

#define CONFIG_MII		1	/* MII PHY management */
#define CONFIG_MII_DEFAULT_TSEC	1	/* Allow unregistered phys */
#define CONFIG_TSEC1	1
#define CONFIG_TSEC1_NAME	"me0"
#define CONFIG_TSEC2	1
#define CONFIG_TSEC2_NAME	"me1"
#define CONFIG_TSEC3	1
#define CONFIG_TSEC3_NAME	"me2"
#define CONFIG_TSEC_USE_HWUNIT_IDX

#define CONFIG_SYS_TBIPA_VALUE		31 /* avoid conflict with eTSEC4 paddr */

#define TSEC1_PHY_ADDR		3
#define TSEC2_PHY_ADDR		31
#define TSEC3_PHY_ADDR		31

#define TSEC1_FLAGS		(TSEC_GIGABIT | TSEC_REDUCED)
#define TSEC2_FLAGS		(TSEC_GIGABIT | TSEC_SGMII)
#define TSEC3_FLAGS		(TSEC_GIGABIT | TSEC_SGMII)

#define TSEC1_PHYIDX		0
#define TSEC2_PHYIDX		1
#define TSEC3_PHYIDX		2

#define CONFIG_ETHPRIME		"eTSEC1"

#define CONFIG_PHY_GIGE		1	/* Include GbE speed/duplex detection */
#endif	/* CONFIG_TSEC_ENET */

/*
 * Environment
 */
#define CONFIG_ENV_IS_IN_FLASH	1
#define CONFIG_ENV_ADDR			CONFIG_SYS_FLASH_BASE	
#define CONFIG_ENV_SIZE			0x2000
#define CONFIG_ENV_SECT_SIZE	0x2000 /* 8K (one sector) */

#define CONFIG_LOADS_ECHO	1	/* echo on for serial download */
#define CONFIG_SYS_LOADS_BAUD_CHANGE	1	/* allow baudrate change */


#define CONFIG_API
#define CONFIG_DOS_PARTITION

#include <config_cmd_default.h>

#define CONFIG_CMD_IRQ
#define CONFIG_CMD_PING
#define CONFIG_CMD_I2C
#define CONFIG_CMD_MII
#define CONFIG_CMD_ELF
#define CONFIG_CMD_SETEXPR
#define CONFIG_CMD_NET
#define CONFIG_CMD_EEPROM
#define CONFIG_SYS_I2C_EEPROM_ADDR_LEN	1
#define CMD_SAVEENV	
#define CONFIG_CMD_FLASH

/*
 * USB
 */
#define CONFIG_CMD_USB
#define CONFIG_USB_STORAGE
#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_FSL
#define CONFIG_EHCI_HCD_INIT_AFTER_RESET
#define CONFIG_USB_SHOW_NO_PROGRESS

#define CONFIG_START_USB

#undef CONFIG_WATCHDOG			/* watchdog disabled */
#define CONFIG_WATCHDOG_EXCEPTION

/* mfgtest configs */
#define CONFIG_MFGTEST_WD_TO	    20000   /* 20000 ticks (4 secs. 1 tick = 200us) */
#define CONFIG_MEMTEST_RANGE1_START	0x100000
#define CONFIG_MEMTEST_RANGE1_END	0x30000000
#define CONFIG_MEMTEST_RANGE2_START	0x40000000
#define CONFIG_MEMTEST_RANGE2_END	0x7fffffff
#define CONFIG_USB_TEST_BUF1		0x100000
#define CONFIG_USB_TEST_BUF2		0x1000000
#define CONFIG_USB_TEST_NUMBLKS		128

/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP			/* undef to save memory	*/
#define CONFIG_CMDLINE_EDITING		/* Command-line editing */
#define CONFIG_SYS_LOAD_ADDR	0x2000000	/* default load address */
#define CONFIG_SYS_PROMPT		"=> "	/* Monitor Command Prompt */
#if defined(CONFIG_CMD_KGDB)
#define CONFIG_SYS_CBSIZE	1024	/* Console I/O Buffer Size */
#else
#define CONFIG_SYS_CBSIZE	256		/* Console I/O Buffer Size */
#endif
#define CONFIG_SYS_PBSIZE (CONFIG_SYS_CBSIZE \
		+ sizeof(CONFIG_SYS_PROMPT) + 16)	/* Print Buffer Size */
#define CONFIG_SYS_MAXARGS	16		/* max number of command args */
#define CONFIG_SYS_BARGSIZE	CONFIG_SYS_CBSIZE	/* Boot Argument Buffer Size */
#define CONFIG_SYS_HZ		1000	/* decrementer freq: 1ms ticks */

/*
 * For booting Linux, the board info and command line data
 * have to be in the first 16 MB of memory, since this is
 * the maximum mapped by the Linux kernel during initialization.
 */
#define CONFIG_SYS_BOOTMAPSZ	(16 << 20)	/* Initial Memory map for Linux*/

#define CONFIG_BOARD_EARLY_INIT_F	1	/* call board_pre_init */
#define CONFIG_MISC_INIT_R			1	/* call misc_init_r() */
#define CONFIG_MISC_INIT_F			1	/* call board misc_init_f */
#define CONFIG_LAST_STAGE_INIT		1   /* call last_stage_init */

/*
 * Internal Definitions
 *
 * Boot Flags
 */
#define BOOTFLAG_COLD	0x01		/* Normal Power-On: Boot from FLASH */
#define BOOTFLAG_WARM	0x02		/* Software reboot */

#if defined(CONFIG_CMD_KGDB)
#define CONFIG_KGDB_BAUDRATE	230400	/* speed to run kgdb serial port */
#define CONFIG_KGDB_SER_INDEX	2	/* which serial port to use */
#endif



/* u-boot image runs on multiple board types */
#define CONFIG_UBOOT_MULTIBOARDS	1
#define BOARD_SCB			0x01	/* Start from 0x01 */
#define BOARD_LC			0x02

#define EX62XX_SCB	(gd->board_type == BOARD_SCB)
#define EX62XX_LC	(gd->board_type == BOARD_LC)

#define CONFIG_SHOW_ACTIVITY

/*
 * Environment Configuration
 */

/* The mac addresses for ethernet interface */
#if defined(CONFIG_TSEC_ENET)
#define CONFIG_HAS_ETH0
#define CONFIG_ETHADDR	00:E0:0C:00:00:FD
#define CONFIG_HAS_ETH1
#endif

/* default location for tftp and bootm */
#define CONFIG_LOADADDR		0x1000000
#define CONFIG_BOOTDELAY	1	/* -1 disables auto-boot */
#undef  CONFIG_BOOTARGS		/* the boot command will set bootargs */
#define CONFIG_BAUDRATE		9600

#define CONFIG_AUTOBOOT_KEYED	/* use key strings to stop autoboot */

/*
 * Use ctrl-c to stop booting and enter u-boot command mode.
 * ctrl-c is common but not too easy to interrupt the booting by accident.
 */
#define CONFIG_AUTOBOOT_STOP_STR	"\x03"

#define CONFIG_2NDSTAGE_BOOTCOMMAND \
	"cp.b 0xFFF00000 ${loadaddr} 0x40000;" \
	"bootelf ${loadaddr}"

#define CONFIG_BOOTCOMMAND	CONFIG_2NDSTAGE_BOOTCOMMAND

#define CONFIG_BOOTCOMMAND_BANK1 \
	"cp.b 0xFFB00000 ${loadaddr} 0x40000;" \
	"bootelf ${loadaddr}"

#define EX62XX_LC_PKG_NAME	"jpfe-ex62x-install-ex.tgz"
#define CFG_BASE_IRI_IP		"128.0.0.1"

/*IRI related*/
#define EX62XX_IRI_RE_CHIP_TYPE		0
#define EX62XX_IRI_LCPU_CHIP_TYPE	0
#define EX62XX_IRI_VCPU_CHIP_TYPE	1
#define EX62XX_CHIP_TYPE_SHIFT		4
#define EX62XX_CHIP_TYPE_MASK		0xF0
#define EX62XX_IRI_IF_MASK		0xF
#define EX62XX_IRI_IP_LCPU_BASE		16

/* image upgrade */
#define CONFIG_UBOOT_UPGRADE	1

#define CFG_UPGRADE_LOADER_BANK1	0xFFB00000
#define CFG_UPGRADE_LOADER_BANK0	0xFFF00000
#define CFG_UPGRADE_UBOOT_BANK1		0xFFB80000
#define CFG_UPGRADE_UBOOT_BANK0		0xFFF80000
/* check for ative partition and swizzle flash bank */
#define CONFIG_FLASH_SWIZZLE
#define CONFIG_BOOT_STATE	/* maintain boot/upgrade states */

#define CFG_UPGRADE_BOOT_STATE_OFFSET	0xE000 /* from beginning of flash */
#define CFG_UPGRADE_BOOT_SECTOR_SIZE	0x2000  /* 8k */

#define CFG_VERSION_MAJOR	2
#define CFG_VERSION_MINOR	1
#define CFG_VERSION_PATCH	0
#define CONFIG_PRODUCTION

/* RE boot states */
#define RE_STATE_EEPROM_ACCESS	0x01
#define RE_STATE_IN_UBOOT		0x02
#define RE_STATE_BOOTING		0x03

#define SLOT_SCB0	0x00
#define SLOT_SCB1	0x01
#define I2CS_SCRATCH_PAD_REG_00	0x00
#define I2CS_SEMAPHORE_REG_60	0x60

#define CFG_LC_MODE_ENV		"hw.lc.mode"
#define LC_MODE_CHASSIS		0
#define LC_MODE_DIAGNOSTIC	1
#define LC_MODE_STANDALONE	2
#define LC_MODE_MAX_VAL		LC_MODE_STANDALONE

#define CFG_EX6208_SCB_SLOT_BASE	0x4

#define LCMODE_IS_STANDALONE	(ex62xx_lc_mode == LC_MODE_STANDALONE)
#define LCMODE_IS_DIAGNOSTIC	(ex62xx_lc_mode == LC_MODE_DIAGNOSTIC)

#define CONFIG_BOARD_RESET  1		/* board specific reset function */
#define CONFIG_BOARD_EARLY_INIT_R	/* call board_early_init_r function */

#endif	/* __CONFIG_H */
