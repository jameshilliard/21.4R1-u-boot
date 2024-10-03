/*
 * $Id$
 *
 * Copyright (c) 2011-2013, Juniper Networks, Inc. 
 * All rights reserved.
 * 
 * Copyright 2007-2009 Freescale Semiconductor, Inc.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * ACX board configuration file
 *
 */
#ifndef __CONFIG_H
#define __CONFIG_H

#ifdef CONFIG_MK_36BIT
#define CONFIG_PHYS_64BIT
#endif

/* High Level Configuration Options */
#define CONFIG_BOOKE		1	/* BOOKE */
#define CONFIG_E500		1	/* BOOKE e500 family */
#define CONFIG_MPC85xx		1	/* MPC8540/60/55/41/48 */
#define CONFIG_P2020		1
#define CONFIG_ACX		1
//#define CONFIG_MP		1	/* support multiple processors */

#define CONFIG_FSL_ELBC		1	/* Has Enhanced localbus controller */
#define CONFIG_PCI		1	/* Enable PCI/PCIE */
#define CONFIG_PCIE0		1	/* PCIE controler 0 (slot 1) */
//#define CONFIG_PCIE1		1	/* PCIE controler 1 (slot 1) */
//#define CONFIG_PCIE2		1	/* PCIE controler 2 (slot 2) */
//#define CONFIG_PCIE3		1	/* PCIE controler 3 (ULI bridge) */
#define CONFIG_FSL_PCI_INIT	1	/* Use common FSL init code */
#define CONFIG_FSL_PCIE_RESET	1	/* need PCIe reset errata */
#define CONFIG_SYS_PCI_64BIT	1	/* enable 64-bit PCI resources */

#define CONFIG_FSL_LAW		1	/* Use common FSL init code */
//#define CONFIG_E1000		1	/* Defind e1000 pci Ethernet card*/

#define CONFIG_TSEC_ENET		/* tsec ethernet support */
#define CONFIG_ENV_OVERWRITE

#define CONFIG_HW_WATCHDOG	1

#define CONFIG_SYS_CLK_FREQ	100000000 /* sysclk for ACX */
#define CONFIG_DDR_CLK_FREQ	100000000 /* ddrclk for ACX 100MHz */
#define CONFIG_ICS307_REFCLK_HZ	33333000  /* ICS307 clock chip ref freq */
#define CONFIG_GET_CLK_FROM_ICS307	  /* decode sysclk and ddrclk freq
					     from ICS307 instead of switches */

/*
 * These can be toggled for performance analysis, otherwise use default.
 */
#define CONFIG_L2_CACHE			/* toggle L2 cache */
#define CONFIG_BTB			/* toggle branch predition */

#define CONFIG_ENABLE_36BIT_PHYS	1

#ifdef CONFIG_PHYS_64BIT
#define CONFIG_ADDR_MAP			1
#define CONFIG_SYS_NUM_ADDR_MAP		16	/* number of TLB1 entries */
#endif

#define CONFIG_BOARD_EARLY_INIT_F	1	/* Call board_pre_init */
#define CONFIG_LAST_STAGE_INIT			/* set hw.board.type as part of
						   last stage init */

#define CONFIG_PANIC_HANG	/* do not reset board on panic */

/*
 * Base addresses -- Note these are effective addresses where the
 * actual resources get mapped (not physical addresses)
 */
#define CONFIG_SYS_CCSRBAR_DEFAULT	0xff700000	/* CCSRBAR Default */
#define CONFIG_SYS_CCSRBAR		0xf7f00000	/* relocated CCSRBAR */
#ifdef CONFIG_PHYS_64BIT
#define CONFIG_SYS_CCSRBAR_PHYS		0xff7f00000ull	/* physical addr of CCSRBAR */
#else
#define CONFIG_SYS_CCSRBAR_PHYS	CONFIG_SYS_CCSRBAR	/* physical addr of CCSRBAR */
#endif
#define CONFIG_SYS_IMMR		CONFIG_SYS_CCSRBAR	/* PQII uses CONFIG_SYS_IMMR */

#define CONFIG_SYS_PCIE3_ADDR		(CONFIG_SYS_CCSRBAR+0x8000)
#define CONFIG_SYS_PCIE2_ADDR		(CONFIG_SYS_CCSRBAR+0x9000)
#define CONFIG_SYS_PCIE1_ADDR		(CONFIG_SYS_CCSRBAR+0xa000)

/* DDR Setup */
#define CONFIG_VERY_BIG_RAM
#define CONFIG_FSL_DDR3		1
#undef CONFIG_FSL_DDR_INTERACTIVE

#define CFG_UPGRADE_BOOT_IMG_ADDR	0xfff80000
#define CFG_UPGRADE_LOADER_ADDR		0xffe00000

#define CONFIG_MEM_RELOCATE_TOP     (1024 << 20)    /* relocate u-boot at the top of 1GB */

/* ECC will be enabled based on perf_mode environment variable */
/* #define	CONFIG_DDR_ECC */

#define CONFIG_ECC_INIT_VIA_DDRCONTROLLER
#define CONFIG_MEM_INIT_VALUE	0xDeadBeef

#define CONFIG_SYS_DDR_SDRAM_BASE	0x00000000
#define CONFIG_SYS_SDRAM_BASE		CONFIG_SYS_DDR_SDRAM_BASE

#define CONFIG_NUM_DDR_CONTROLLERS	1
#define CONFIG_DIMM_SLOTS_PER_CTLR	1
#define CONFIG_CHIP_SELECTS_PER_CTRL	2

#define CONFIG_SYS_DRAM_TEST
#define CONFIG_SYS_MEMTEST_START	CONFIG_SYS_DDR_SDRAM_BASE
#define CONFIG_SYS_MEMTEST_END		CONFIG_SYS_SDRAM_SIZE
#define CONFIG_SYS_MEMTEST_INCR		4

/* I2C addresses of SPD EEPROMs */
#define CONFIG_SYS_SPD_BUS_NUM		0	/* SPD EEPROM located on I2C bus 0 */
#define SPD_EEPROM_ADDRESS1	0x51	/* CTLR 0 DIMM 0 */

/* These are used when DDR doesn't use SPD.  */
#define CONFIG_SYS_SDRAM_SIZE		gd->ram_size

/* Default settings for "stable" mode */
#define CONFIG_SYS_DDR_CS0_BNDS		0x0000003F
#define CONFIG_SYS_DDR_CS1_BNDS		0x00000000
#define CONFIG_SYS_DDR_CS0_CONFIG	0x80014202
#define CONFIG_SYS_DDR_CS1_CONFIG	0x00000000
#define CONFIG_SYS_DDR_TIMING_3		0x00020000
#define CONFIG_SYS_DDR_TIMING_0		0x00330804
#define CONFIG_SYS_DDR_TIMING_1		0x6f6b4846
#define CONFIG_SYS_DDR_TIMING_2		0x0fa890d4
#define CONFIG_SYS_DDR_MODE_1		0x00421422
#define CONFIG_SYS_DDR_MODE_2		0x00000000
#define CONFIG_SYS_DDR_MODE_CTRL	0x00000000
#define CONFIG_SYS_DDR_INTERVAL		0x61800100
#define CONFIG_SYS_DDR_DATA_INIT	0xdeadbeef
#define CONFIG_SYS_DDR_CLK_CTRL		0x02000000
#define CONFIG_SYS_DDR_TIMING_4		0x00220001
#define CONFIG_SYS_DDR_TIMING_5		0x03402400
#define CONFIG_SYS_DDR_ZQ_CNTL		0x89080600
#define CONFIG_SYS_DDR_WRLVL_CNTL	0x8655A608
#define CONFIG_SYS_DDR_CONTROL		0xE7000000 /* Type = DDR3: ECC enabled, No Interleaving */
#define CONFIG_SYS_DDR_CONTROL2		0x24400011
#define CONFIG_SYS_DDR_CDR1		0x00040000
#define CONFIG_SYS_DDR_CDR2		0x00000000

#define CONFIG_SYS_DDR_ERR_INT_EN	0x0000000d
#define CONFIG_SYS_DDR_ERR_DIS		0x00000000
#define CONFIG_SYS_DDR_SBE		0x00010000

/* Settings that differ for "performance" mode */
#define CONFIG_SYS_DDR_CS0_BNDS_PERF		0x0000007F /* Interleaving Enabled */
#define CONFIG_SYS_DDR_CS1_BNDS_PERF		0x00000000 /* Interleaving Enabled */
#define CONFIG_SYS_DDR_CS1_CONFIG_PERF	0x80014202
#define CONFIG_SYS_DDR_TIMING_1_PERF		0x5d5b4543
#define CONFIG_SYS_DDR_TIMING_2_PERF		0x0fa890ce
#define CONFIG_SYS_DDR_CONTROL_PERF		0xC7004000 /* Type = DDR3: ECC disabled, cs0-cs1 interleaving */

/*
 * The following set of values were tested for DDR2
 * with a DDR3 to DDR2 interposer
 *
#define CONFIG_SYS_DDR_TIMING_3		0x00000000
#define CONFIG_SYS_DDR_TIMING_0		0x00260802
#define CONFIG_SYS_DDR_TIMING_1		0x3935d322
#define CONFIG_SYS_DDR_TIMING_2		0x14904cc8
#define CONFIG_SYS_DDR_MODE_1		0x00480432
#define CONFIG_SYS_DDR_MODE_2		0x00000000
#define CONFIG_SYS_DDR_INTERVAL		0x06180100
#define CONFIG_SYS_DDR_DATA_INIT	0xdeadbeef
#define CONFIG_SYS_DDR_CLK_CTRL		0x03800000
#define CONFIG_SYS_DDR_OCD_CTRL		0x00000000
#define CONFIG_SYS_DDR_OCD_STATUS	0x00000000
#define CONFIG_SYS_DDR_CONTROL		0xC3008000
#define CONFIG_SYS_DDR_CONTROL2		0x04400010
 *
 */

#undef CONFIG_CLOCKS_IN_MHZ

/*
 * Memory map
 *
 * 0x0000_0000	0x7fff_ffff	DDR			2G Cacheable
 * 0x8000_0000	0xbfff_ffff
 * 0xc000_0000	0xcfff_ffff	PCI			256M non-cacheable
 * 0xd000_0000	0xf7ef_ffff
 *
 * 0xf7f0_0000	0xf7ff_ffff	CCSR			1M non-cacheable
 * 0xf800_0000	0xfecf_ffff
 * 0xfed0_0000	0xfed0_ffff	CoProcessor FPGA	64K non-cacheable
 *
 * 0xfee0_0000	0xfee1_ffff	NVRAM			128K non-cacheable
 * 0xfef0_0000	0xfef0_7fff	SYSPLD			32K non-cacheable
 * 0xffc0_0000  0xffff_ffff	Boot Flash		4M
 */

/*
 * Local Bus Definitions
 */
#define CONFIG_SYS_FLASH_BASE		0xffc00000	/* Only 4M out of 8M 
							   visible to CPU*/
#define CONFIG_SYS_FLASH_SIZE		(4*1024*1024)
#ifdef CONFIG_PHYS_64BIT
#define CONFIG_SYS_FLASH_BASE_PHYS	0xfffc00000ull
#else
#define CONFIG_SYS_FLASH_BASE_PHYS	CONFIG_SYS_FLASH_BASE
#endif

#define CONFIG_FLASH_BR_PRELIM	(BR_PHYS_ADDR(CONFIG_SYS_FLASH_BASE_PHYS) | BR_PS_8 | BR_V)
#define CONFIG_FLASH_OR_PRELIM	((~(CONFIG_SYS_FLASH_SIZE - 1) & 0xFFFF8000) | 0x00000030)  /* NOR Options */

#define CONFIG_SYS_FLASH_BANKS_LIST	{CONFIG_SYS_FLASH_BASE}
//#define CONFIG_SYS_FLASH_QUIET_TEST
#define CONFIG_FLASH_SHOW_PROGRESS 45 /* count down from 45/5: 9..1 */

#define CONFIG_SYS_MAX_FLASH_BANKS	1		/* number of banks */
#define CONFIG_SYS_MAX_FLASH_SECT	128		/* sectors per device */
#undef	CONFIG_SYS_FLASH_CHECKSUM
#define CONFIG_SYS_FLASH_ERASE_TOUT	60000		/* Flash Erase Timeout (ms) */
#define CONFIG_SYS_FLASH_WRITE_TOUT	500		/* Flash Write Timeout (ms) */

#define CONFIG_SYS_MONITOR_BASE	TEXT_BASE	/* start of monitor */

#define CONFIG_FLASH_CFI_DRIVER
#define CONFIG_SYS_FLASH_CFI
#define CONFIG_SYS_FLASH_EMPTY_INFO
#define CONFIG_SYS_FLASH_AMD_CHECK_DQ7

#define CONFIG_BOARD_EARLY_INIT_R	/* call board_early_init_r function */

#define CONFIG_SYS_SYSPLD_BASE		0xfef00000
#define CONFIG_SYS_SYSPLD_SIZE		0x00002000UL	/* 8KB */

#define CONFIG_SYS_NVRAM_BASE		0xfee00000
#define CONFIG_SYS_NVRAM_SIZE		0x00020000	/* 128KB */

#define CONFIG_SYS_COP_FPGA_BASE	0xfed00000
#define CONFIG_SYS_COP_FPGA_SIZE	0x00010000	/* 64 KB */

#define CONFIG_SYS_INIT_RAM_LOCK	1
#define CONFIG_SYS_INIT_RAM_ADDR	0xe4010000	/* Initial L1 address */
//#define CONFIG_SYS_INIT_RAM_ADDR	0xffd00000
#define CONFIG_SYS_INIT_RAM_END		0x00004000	/* End of used area in RAM */
/* define to use L2SRAM as initial stack */
#undef CONFIG_SYS_L2_INIT_RAM
#define CONFIG_SYS_INIT_L2_ADDR		0xf8fc0000
#define CONFIG_SYS_INIT_L2_END		0x00040000

#define CONFIG_SYS_GBL_DATA_SIZE	128	/* num bytes initial data */
#define CONFIG_SYS_GBL_DATA_OFFSET	(CONFIG_SYS_INIT_RAM_END - CONFIG_SYS_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_OFFSET	CONFIG_SYS_GBL_DATA_OFFSET

#define CONFIG_SYS_MONITOR_LEN		(512 * 1024) /* Reserve 256 kB for Mon */
#define CONFIG_SYS_MALLOC_LEN		(1024 * 1024)	/* Reserved for malloc */

/*
 * Chip Select 0: FLASH on the Local Bus
 * Size: 4MB (Since only that is what is visible to CPU)
 */ 
#define CONFIG_SYS_BR0_PRELIM	((CONFIG_SYS_FLASH_BASE & 0xFFFF8000) | 0x00000801) /* 8 bit NOR */
#define CONFIG_SYS_OR0_PRELIM	((~(CONFIG_SYS_FLASH_SIZE - 1) & 0xFFFF8000) | 0x00000030)  /* NOR Options */

/*
 * Chip Select 1: SYSPLD on the Local Bus
 * Size: 32KB (Since that is the minimal size for a chip select on eLBC).
 */ 
#define CONFIG_SYS_BR1_PRELIM	((CONFIG_SYS_SYSPLD_BASE & 0xFFFF8000) | 0x00000801)  /* 8 bit SysPLD */
#define CONFIG_SYS_OR1_PRELIM	((~(CONFIG_SYS_SYSPLD_SIZE - 1) & 0xFFFF8000) | 0x00000010)

/*
 * Chip Select 2: NVRAM on the Local Bus
 * Size: 128KB
 */ 
#define CONFIG_SYS_BR2_PRELIM	((CONFIG_SYS_NVRAM_BASE & 0xFFFF8000) | 0x00000801)  /* 8 bit NVRAM */
#define CONFIG_SYS_OR2_PRELIM	((~(CONFIG_SYS_NVRAM_SIZE - 1) & 0xFFFF8000) | 0x00000020)

/*
 * Chip Select 3: Co-Processor FPGA on the Local Bus
 * Size: 64KB
 */ 
#define CONFIG_SYS_BR3_PRELIM	((CONFIG_SYS_COP_FPGA_BASE & 0xFFFF8000) | 0x00000801)  /* 8 bit FPGA */
#define CONFIG_SYS_OR3_PRELIM	((~(CONFIG_SYS_COP_FPGA_SIZE - 1) & 0xFFFF8000) | 0x00000FF7)

/* Serial Port - controlled on board with jumper J8
 * open - index 2
 * shorted - index 1
 */
#define CONFIG_CONS_INDEX	1
#undef	CONFIG_SERIAL_SOFTWARE_FIFO
#define CONFIG_SYS_NS16550
#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_REG_SIZE	1
#define CONFIG_SYS_NS16550_CLK		get_bus_freq(0)

#define CONFIG_SYS_BAUDRATE_TABLE	\
	{300, 600, 1200, 2400, 4800, 9600, 19200, 38400,115200}

#define CONFIG_SYS_NS16550_COM1	(CONFIG_SYS_CCSRBAR+0x4500)
#define CONFIG_SYS_NS16550_COM2	(CONFIG_SYS_CCSRBAR+0x4600)

/* Use the HUSH parser */
#define CONFIG_SYS_HUSH_PARSER
#ifdef	CONFIG_SYS_HUSH_PARSER
#define CONFIG_SYS_PROMPT_HUSH_PS2 "> "
#endif

/*
 * Pass open firmware flat tree
 */
#define CONFIG_OF_LIBFDT		1
#define CONFIG_OF_BOARD_SETUP		1
#define CONFIG_OF_STDOUT_VIA_ALIAS	1

/* I2C */
#define CONFIG_FSL_I2C		/* Use FSL common I2C driver */
#define CONFIG_HARD_I2C		/* I2C with hardware support */
#undef	CONFIG_SOFT_I2C		/* I2C bit-banged */
#define CONFIG_I2C_MULTI_BUS
#define CONFIG_SYS_I2C_SPEED		400000	/* I2C speed and slave address */
#define CONFIG_SYS_I2C_SLAVE		0x7F
#define CONFIG_SYS_I2C_NOPROBES		{0x69}/* Don't probe these addrs */
#define CONFIG_SYS_I2C_OFFSET		0x3000
#define CONFIG_SYS_I2C2_OFFSET		0x3100
//#define CONFIG_FSL_I2C_CUSTOM_FDR	0x3f
//#define CONFIG_FSL_I2C_CUSTOM_DFSR	0x3f

/*
 * I2C2 EEPROM
 */
//#define CONFIG_ID_EEPROM
#ifdef CONFIG_ID_EEPROM
#define CONFIG_SYS_I2C_EEPROM_NXID
#endif
#define CONFIG_SYS_I2C_EEPROM_ADDR	0x51
#define CONFIG_SYS_I2C_EEPROM_ADDR_LEN	1
#define CONFIG_SYS_I2C_SW1_ADDR		0x70    /* I2C Switch   */
#define CONFIG_SYS_I2C_USB2513_ADDR	0x2C    /* USB Hub      */
#define CONFIG_SYS_EEPROM_MAC_OFFSET	56
//#define CONFIG_SYS_EEPROM_BUS_NUM	0

/*
 * General PCI
 * Memory space is mapped 1-1, but I/O space must start from 0.
 */

/* controller 3, Slot 1, tgtid 3, Base address b000 */
#define CONFIG_SYS_PCIE3_MEM_VIRT	0x80000000
#ifdef CONFIG_PHYS_64BIT
#define CONFIG_SYS_PCIE3_MEM_BUS	0xe0000000
#define CONFIG_SYS_PCIE3_MEM_PHYS	0xc00000000ull
#else
#define CONFIG_SYS_PCIE3_MEM_BUS	0x80000000
#define CONFIG_SYS_PCIE3_MEM_PHYS	0x80000000
#endif
#define CONFIG_SYS_PCIE3_MEM_SIZE	0x20000000	/* 512M */
#define CONFIG_SYS_PCIE3_IO_VIRT	0xffc00000
#define CONFIG_SYS_PCIE3_IO_BUS		0x00000000
#ifdef CONFIG_PHYS_64BIT
#define CONFIG_SYS_PCIE3_IO_PHYS	0xfffc00000ull
#else
#define CONFIG_SYS_PCIE3_IO_PHYS	0xffc00000
#endif
#define CONFIG_SYS_PCIE3_IO_SIZE	0x00010000	/* 64k */

/* controller 2, direct to uli, tgtid 2, Base address 9000 */
#define CONFIG_SYS_PCIE2_MEM_VIRT	0xa0000000
#ifdef CONFIG_PHYS_64BIT
#define CONFIG_SYS_PCIE2_MEM_BUS	0xe0000000
#define CONFIG_SYS_PCIE2_MEM_PHYS	0xc20000000ull
#else
#define CONFIG_SYS_PCIE2_MEM_BUS	0xa0000000
#define CONFIG_SYS_PCIE2_MEM_PHYS	0xa0000000
#endif
#define CONFIG_SYS_PCIE2_MEM_SIZE	0x20000000	/* 512M */
#define CONFIG_SYS_PCIE2_IO_VIRT	0xffc10000
#define CONFIG_SYS_PCIE2_IO_BUS		0x00000000
#ifdef CONFIG_PHYS_64BIT
#define CONFIG_SYS_PCIE2_IO_PHYS	0xfffc10000ull
#else
#define CONFIG_SYS_PCIE2_IO_PHYS	0xffc10000
#endif
#define CONFIG_SYS_PCIE2_IO_SIZE	0x00010000	/* 64k */

/* controller 1, Slot 2, tgtid 1, Base address a000 */
#define CONFIG_SYS_PCIE1_MEM_VIRT	0xc0000000
#ifdef CONFIG_PHYS_64BIT
#define CONFIG_SYS_PCIE1_MEM_BUS	0xc0000000
#define CONFIG_SYS_PCIE1_MEM_PHYS	0xc40000000ull
#else
#define CONFIG_SYS_PCIE1_MEM_BUS	0xc0000000
#define CONFIG_SYS_PCIE1_MEM_PHYS	0xc0000000
#endif
#define CONFIG_SYS_PCIE1_MEM_SIZE	0x20000000	/* 512M */
#define CONFIG_SYS_PCIE1_IO_VIRT	0xffc20000
#define CONFIG_SYS_PCIE1_IO_BUS		0x00000000
#ifdef CONFIG_PHYS_64BIT
#define CONFIG_SYS_PCIE1_IO_PHYS	0xfffc20000ull
#else
#define CONFIG_SYS_PCIE1_IO_PHYS	0xffc20000
#endif
#define CONFIG_SYS_PCIE1_IO_SIZE	0x00010000	/* 64k */

#if defined(CONFIG_PCI)

/*PCIE video card used*/
#define VIDEO_IO_OFFSET		CONFIG_SYS_PCIE1_IO_VIRT

/* video */
//#define CONFIG_VIDEO

#if defined(CONFIG_VIDEO)
#define CONFIG_BIOSEMU
#define CONFIG_CFB_CONSOLE
#define CONFIG_VIDEO_SW_CURSOR
#define CONFIG_VGA_AS_SINGLE_DEVICE
#define CONFIG_ATI_RADEON_FB
#define CONFIG_VIDEO_LOGO
/*#define CONFIG_CONSOLE_CURSOR*/
#define CONFIG_SYS_ISA_IO_BASE_ADDRESS VIDEO_IO_OFFSET
#endif

#define CONFIG_NET_MULTI
#define CONFIG_PCI_PNP			/* do pci plug-and-play */

#undef CONFIG_EEPRO100
#undef CONFIG_TULIP
#define CONFIG_RTL8139

#ifndef CONFIG_PCI_PNP
	#define PCI_ENET0_IOADDR	CONFIG_SYS_PCIE3_IO_BUS
	#define PCI_ENET0_MEMADDR	CONFIG_SYS_PCIE3_IO_BUS
	#define PCI_IDSEL_NUMBER	0x11	/* IDSEL = AD11 */
#endif

#define CONFIG_PCI_SCAN_SHOW		/* show pci devices on startup */
#define CONFIG_DOS_PARTITION
//#define CONFIG_SCSI_AHCI

#ifdef CONFIG_SCSI_AHCI
#define CONFIG_SATA_ULI5288
#define CONFIG_SYS_SCSI_MAX_SCSI_ID	4
#define CONFIG_SYS_SCSI_MAX_LUN	1
#define CONFIG_SYS_SCSI_MAX_DEVICE	(CONFIG_SYS_SCSI_MAX_SCSI_ID * CONFIG_SYS_SCSI_MAX_LUN)
#define CONFIG_SYS_SCSI_MAXDEVICE	CONFIG_SYS_SCSI_MAX_DEVICE
#endif /* SCSI */

#endif	/* CONFIG_PCI */


#if defined(CONFIG_TSEC_ENET)
#define CONFIG_API
#ifndef CONFIG_NET_MULTI
#define CONFIG_NET_MULTI	1
#endif

#define CONFIG_MII		1	/* MII PHY management */
#define CONFIG_MII_DEFAULT_TSEC	1	/* Allow unregistered phys */
#define CONFIG_TSEC1	1
#define CONFIG_TSEC1_NAME	"fxp0"

#define CONFIG_PIXIS_SGMII_CMD
#define CONFIG_FSL_SGMII_RISER	1
#define SGMII_RISER_PHY_OFFSET	0x1b

#ifdef CONFIG_FSL_SGMII_RISER
#define CONFIG_SYS_TBIPA_VALUE		0x10 /* avoid conflict with eTSEC4 paddr */
#endif

#define TSEC1_PHY_ADDR		1
#define TSEC2_PHY_ADDR		1
#define TSEC3_PHY_ADDR		2

#define TSEC1_FLAGS		(TSEC_GIGABIT | TSEC_REDUCED)
#define TSEC2_FLAGS		(TSEC_GIGABIT | TSEC_REDUCED)
#define TSEC3_FLAGS		(TSEC_GIGABIT | TSEC_REDUCED)

#define TSEC1_PHYIDX		0
#define TSEC2_PHYIDX		0
#define TSEC3_PHYIDX		0

#define CONFIG_ETHPRIME		"fxp0"

#define CONFIG_PHY_GIGE		1	/* Include GbE speed/duplex detection */
#endif	/* CONFIG_TSEC_ENET */

/*
 * Environment
 */
#define CONFIG_ENV_IS_IN_FLASH	1
#define CONFIG_ENV_ADDR		0xfff60000
#define CONFIG_ENV_SIZE		0x2000
#define CONFIG_ENV_SECT_SIZE	0x20000 /* 128K (one sector) */

#define CONFIG_LOADS_ECHO	1	/* echo on for serial download */
#define CONFIG_SYS_LOADS_BAUD_CHANGE	1	/* allow baudrate change */

/*
 * Command line configuration.
 */
#include <config_cmd_default.h>

#define CONFIG_CMD_IRQ
#define CONFIG_CMD_PING
#define CONFIG_CMD_I2C
#define CONFIG_CMD_MII
#define CONFIG_CMD_ELF
#define CONFIG_CMD_SETEXPR
#define CONFIG_CMD_NET
#define CONFIG_CMD_EEPROM
#define CMD_SAVEENV	
#define CONFIG_CMD_FLASH

#if defined(CONFIG_PCI)
#define CONFIG_CMD_PCI
#if defined(CONFIG_SCSI_AHCI)
#define CONFIG_CMD_SCSI
#endif
#define CONFIG_CMD_EXT2
#endif

/*
 * USB
 */
#define CONFIG_CMD_USB
#define CONFIG_USB_STORAGE
#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_FSL
#define CONFIG_USB_SHOW_NO_PROGRESS
#define CONFIG_EHCI_HCD_INIT_AFTER_RESET
#define USB_WRITE_READ   /* Allow Reading / Writing to USB storage devices */

#undef CONFIG_WATCHDOG			/* watchdog disabled */

/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP			/* undef to save memory	*/
#define CONFIG_CMDLINE_EDITING		/* Command-line editing */
#define CONFIG_SYS_LOAD_ADDR	0x2000000	/* default load address */
#define CONFIG_SYS_PROMPT	"=> "		/* Monitor Command Prompt */
#if defined(CONFIG_CMD_KGDB)
#define CONFIG_SYS_CBSIZE	1024		/* Console I/O Buffer Size */
#else
#define CONFIG_SYS_CBSIZE	256		/* Console I/O Buffer Size */
#endif
#define CONFIG_SYS_PBSIZE (CONFIG_SYS_CBSIZE+sizeof(CONFIG_SYS_PROMPT)+16) /* Print Buffer Size */
#define CONFIG_SYS_MAXARGS	16		/* max number of command args */
#define CONFIG_SYS_BARGSIZE	CONFIG_SYS_CBSIZE	/* Boot Argument Buffer Size */
#define CONFIG_SYS_HZ		1000		/* decrementer freq: 1ms ticks */

/*
 * For booting Linux, the board info and command line data
 * have to be in the first 16 MB of memory, since this is
 * the maximum mapped by the Linux kernel during initialization.
 */
#define CONFIG_SYS_BOOTMAPSZ	(8 << 20)	/* Initial Memory map for Linux*/

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

/*
 * Environment Configuration
 */

/* The mac addresses for all ethernet interface */
#if defined(CONFIG_TSEC_ENET)
#define CONFIG_HAS_ETH0
#define CONFIG_ETHADDR	00:E0:0C:02:00:FD
#define CONFIG_HAS_ETH1
#define CONFIG_ETH1ADDR	00:E0:0C:02:01:FD
#define CONFIG_HAS_ETH2
#define CONFIG_ETH2ADDR	00:E0:0C:02:02:FD
#define CONFIG_HAS_ETH3
#define CONFIG_ETH3ADDR	00:E0:0C:02:03:FD
#endif

#define CONFIG_IPADDR		192.168.1.254

#define CONFIG_HOSTNAME		fortius
#define CONFIG_ROOTPATH		/opt/nfsroot
#define CONFIG_BOOTFILE		uImage
#define CONFIG_UBOOTPATH	u-boot.bin	/* U-Boot image on TFTP server */

#define CONFIG_SERVERIP		192.168.1.1
#define CONFIG_GATEWAYIP	192.168.1.1
#define CONFIG_NETMASK		255.255.255.0

/* default location for tftp and bootm */
#define CONFIG_LOADADDR		1000000

#define CONFIG_BOOTDELAY	1	/* -1 disables auto-boot */
#undef  CONFIG_BOOTARGS			/* the boot command will set bootargs */

#define CONFIG_AUTOBOOT_PROMPT	\
    "Press Ctrl-C in next %d seconds to enter u-boot prompt...\n"
#define CONFIG_AUTOBOOT_STOP_STR    "\x03"

#define CONFIG_BAUDRATE		9600

#define	CONFIG_EXTRA_ENV_SETTINGS	\
 "hw.uart.console=mm:0xF7F04500\0"	\
 "hw.uart.dbgport=mm:0xF7F04500\0"	\
 "netdev=fxp0\0"			\
 "ethact=fxp0\0"			\
 "ethprime=fxp0\0"			\
 "consoledev=ttyS0\0"			\
 "run.slowtests=no\0"

#define CONFIG_NFSBOOTCOMMAND		\
 "setenv bootargs root=/dev/nfs rw "	\
 "nfsroot=$serverip:$rootpath "		\
 "ip=$ipaddr:$serverip:$gatewayip:$netmask:$hostname:$netdev:off " \
 "console=$consoledev,$baudrate $othbootargs;"	\
 "tftp $loadaddr $bootfile;"		\
 "tftp $fdtaddr $fdtfile;"		\
 "bootm $loadaddr - $fdtaddr"

#define CONFIG_RAMBOOTCOMMAND		\
 "setenv bootargs root=/dev/ram rw "	\
 "console=$consoledev,$baudrate $othbootargs;"	\
 "tftp $ramdiskaddr $ramdiskfile;"	\
 "tftp $loadaddr $bootfile;"		\
 "tftp $fdtaddr $fdtfile;"		\
 "bootm $loadaddr $ramdiskaddr $fdtaddr"

#define CONFIG_BOOTCOMMAND		\
 "setenv watchdog.tickle no; "		\
 "cp.b 0xffe00000 $loadaddr 0x100000;"	\
 "bootelf $loadaddr"

#define CONFIG_BOARD_TYPES /* We can have multiple boards */
#define I2C_ID_ACX1000	    0x0551
#define I2C_ID_ACX2000	    0x0552
#define I2C_ID_ACX4000	    0x0553

#define I2C_ID_ACX2100	    0x0562
#define I2C_ID_ACX2200	    0x0563
#define I2C_ID_ACX1100	    0x0564

#endif	/* __CONFIG_H */
