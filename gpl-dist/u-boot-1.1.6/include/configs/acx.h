/*
 * $Id$
 *
 * acx.h -- ACX Board configuration file for u-boot
 *
 * Rajat Jain, Feb 2011
 *
 * Copyright (c) 2011-2014, Juniper Networks, Inc.
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

#ifndef __CONFIG_H
#define __CONFIG_H

/* High Level Configuration Options */
#define CONFIG_BOOKE		1	/* BOOKE */
#define CONFIG_E500		1	/* BOOKE e500 family */
#define CONFIG_MPC85xx		1	/* MPC8540/60/55/41/48 */
#define CONFIG_MULTICORE	1
#define CONFIG_P2020    	1
#define CONFIG_ACX  1

#define CONFIG_ENV_OVERWRITE
#define CONFIG_HW_WATCHDOG	1

/*
 * When initializing flash, if we cannot find the manufacturer ID,
 * assume this is the AMD flash associated with the CDS board.
 * This allows booting from a promjet.
 */
#define CONFIG_ASSUME_AMD_FLASH

#define CONFIG_SYS_CLK_FREQ	100000000  /* SYSCLK for ACX  = 100 MHz*/

/*
 * These can be toggled for performance analysis, otherwise use default.
 */
#define CONFIG_L2_CACHE			/* toggle L2 cache */
#define CONFIG_BTB			/* toggle branch predition */
#define CONFIG_ADDR_STREAMING		/* toggle addr streaming */
#undef CONFIG_CLEAR_LAW0		/* Clear LAW0 in cpu_init_r */

#define CONFIG_BOARD_EARLY_INIT_F 1 /* Call board_pre_init */
#define CONFIG_BOARD_EARLY_INIT_R   /* Call board_pre_init */

#define CONFIG_PANIC_HANG	/* do not reset board on panic */
#define CONFIG_LAST_STAGE_INIT  /* set hw.board.type as part of last 
				   stage init */

#define USB_WRITE_READ		/* Allow Reading / Writing to USB storage devices */
/*
 * Base addresses -- Note these are effective addresses where the
 * actual resources get mapped (not physical addresses)
 */
#define CFG_CCSRBAR_DEFAULT	0xFF700000	/* CCSRBAR Default */
#define CFG_CCSRBAR		0xF7F00000	/* relocated CCSRBAR */
#define CFG_IMMR		CFG_CCSRBAR	/* PQII uses CFG_IMMR */

#define CFG_UPGRADE_BOOT_IMG_ADDR  	0xFFF80000
#define CFG_UPGRADE_LOADER_ADDR 	0xFFE00000


#define CFG_PCIE3_ADDR		(CFG_CCSRBAR+0x8000)
#define CFG_PCIE2_ADDR		(CFG_CCSRBAR+0x9000)
#define CFG_PCIE1_ADDR		(CFG_CCSRBAR+0xa000)

#define CFG_USB_ADDR		(CFG_CCSRBAR+0x22000)

/*
 * Memory map
 *
 * 0.00 GB +----------------------------------------+
 *         |                                        |  0x00000000
 *         |                                        |
 *         |                                        |
 *         |                                        |   Note1: Models with 1GB
 *         |                                        |   will have only 1GB here.
 *         |               RAM (2 GB)               |
 *         |                                        |
 *         |                                        |
 *         |                                        |
 *         |                                        |
 *         |                                        |
 *         |                                        |
 *         |                                        |
 *         |                                        |  0x7FFFFFFF
 * 2.00 GB +----------------------------------------+
 *         |                                        |  0x80000000
 *         |                                        |
 *         |              Empty (1 GB)              |
 *         |                                        |
 *         |                                        |
 *         |                                        |
 *         |                                        |  0xBFFFFFFF
 * 3.00 GB +----------------------------------------+
 *         | On Fortius Bd - PCIe Cntrl 1 (256 MB)  |  0xC0000000
 *         | On Eval Bd    - PCIe Cntrl 2 (256 MB)  |  0xCFFFFFFF
 * 3.25 GB +----------------------------------------+
 *         |                                        |  0xD0000000
 *         |               Empty                    |
 *         |              (639 MB)                  |
 *         |                                        |
 *         |                                        |  0xF7EFFFFF
 * (3GB +  +----------------------------------------+
 *  895 MB)|            CCSRBAR (1 MB)              |  0xF7F00000
 *         |                                        |  0xF7FFFFFF
 * (3GB +  +----------------------------------------+
 *  896 MB)|                                        |  0xF8000000
 *         |            Empty (109 MB)              |
 *         |                                        |  0xFECFFFFF
 * (3GB +  +----------------------------------------+
 * 1005 MB)|           CoProcessor FPGA             |  0xFED00000
 *         |               (64 KB)                  |  0xFED0FFFF
 *         +----------------------------------------+
 *         |            Empty (960 KB)              |
 * (3GB +  |----------------------------------------+
 * 1006 MB)|            NVRAM (128 KB)              |  0xFEE00000 [PFE to use first 124K; RE to use last 4K]
 *         |                                        |  0xFEE1FFFF
 *         +----------------------------------------+
 *         |            Empty (896 KB)              |
 * (3GB +  +----------------------------------------+
 * 1007 MB)|           SYSPLD (32 KB)               |  0xFEF00000 [LBC ChipSel minsize is 32KB]
 *         |                                        |  0xFEF07FFF [Only 2 X 4KB pages should be opened via TLB]
 *         +----------------------------------------+
 *         |            Empty (12 MB + 992 KB)      |
 *         |                                        |
 *         |                                        |
 * (3GB +  +----------------------------------------+
 * 1020 MB)|         Boot Flash (4 MB)              |  0xFFC00000 [Only 4MB visible to CPU
 *         |                                        |  0xFFFFFFFF  due to flash swizzle feature]
 * 4.00 GB +----------------------------------------+
 * 
 */
/******************************************************************************
 * Memory Map
 ******************************************************************************/

/*
 * DDR
 */
#define CFG_SDRAM_BASE	0x00000000	/* DDR is system memory*/
#define CFG_SDRAM_SIZE	gd->ram_size  
#define CONFIG_VERY_BIG_RAM

/*
 * PCI-E
 */
#define CFG_PCIE1_MEM_BASE	0xC0000000
#define CFG_PCIE1_MEM_PHYS	CFG_PCIE1_MEM_BASE
#define CFG_PCIE1_MEM_SIZE	0x10000000	/* 256M */

/*
 * Coprocessor FPGA
 */
#define CFG_COP_FPGA_BASE	0xFED00000
#define CFG_COP_FPGA_SIZE	0x00010000     /* 64 KB */

/*
 * NVRAM
 */
#define CFG_NVRAM_BASE	0xFEE00000
#define CFG_NVRAM_SIZE	0x00020000     /* 128 KB */

/*
 * SysPLD
 */
#define CFG_SYSPLD_BASE	0xFEF00000
#define CFG_SYSPLD_SIZE	0x00002000UL	/* 8KB */

/*
 * Boot Flash
 */
#define CFG_FLASH_BASE		0xFFC00000   /* start of FLASH - only 4M out
					       	of 8MB visible to CPU */
#define CFG_FLASH_SIZE		(4 * 1024 *1024)

/******************************************************************************/
/*
 * TPM
 */
/* Generic TPM interfaced through LPC bus */
#define CONFIG_TPM				/* Include TPM routines	    */
#define CONFIG_TPM_TIS_LPC			/* TPM is conncted over LPC */
#define CONFIG_CMD_TPM				/* Include TPM Commands	    */
#define CONFIG_TPM_AUTH_SESSIONS		/* Inlcude session protocol routines */
#define CONFIG_CMD_HASH				/* Include hash command routine */
#define CONFIG_HASH_VERIFY			/* Include Verification of Hash */
#define CONFIG_SHA1				/* Include SHA1 algorithm routines */
#define CONFIG_CMD_SHA1SUM			/* Include sha1 sum commands for hash */
#define CONFIG_SHA256				/* Include SHA256 algorithm routines */
#define CONFIG_TPM_TIS_BASE_ADDRESS        0xFED40000	/* Base address of TPM over LPC */


#define CONFIG_FIT 1				/* Include FIT routines	*/
#define CONFIG_FIT_VERBOSE 1			/* Enable to display FIT format errors/warnings */
#define CONFIG_OF_LIBFDT 1			/* Include FDT Library	*/
#define CONFIG_FIT_SIGNATURE 1			/* Support FIT signatures */
#define CONFIG_RSA 1				/* Include RSA routines	*/

#define IMAGE_ENABLE_VERIFY 1			/* Enable signature verification */

#define IMAGE_ENABLE_SHA256 1			/* Enable SHA256 in FIT hash verification */
#define IMAGE_ENABLE_SHA1 1			/* Enable SHA1 in FIT hash verification	*/

/*
 * FDT is separate file
 */
#define CONFIG_OF_SEPARATE 1


/*******************************************************************************/
/*
 * DDR SDRAM Setup
 */

#define CONFIG_ECC_INIT_VIA_DDRCONTROLLER
#define CONFIG_MEM_INIT_VALUE		0xDeadBeef
#define CONFIG_NUM_DDR_CONTROLLERS	1

#define CONFIG_DDR_CLK_FREQ	100000000  /* DDRCLK for ACX  = 100 MHz*/

/* A premilinary memory test after memory init */
#define CFG_DRAM_TEST
#define CFG_MEMTEST_START	(CFG_SDRAM_BASE + 0x10000000)
#define CFG_MEMTEST_END		(CFG_SDRAM_BASE + CFG_SDRAM_SIZE - 1)
#define CFG_MEMTEST_INCR	0x00000004  /* Should be in multiples 
					       of 4 bytes */
#define CFG_ALT_MEMTEST

#undef CONFIG_CLOCKS_IN_MHZ


/******************************************************************************/

/*
 * Flash Configuration
 */
#define CFG_LCRR		(LCRR_DBYP | LCRR_CLKDIV_16)


#define CFG_FLASH_BANKS_LIST	{CFG_FLASH_BASE}

#define CFG_MAX_FLASH_BANKS	1		/* number of banks */

/* Number of VISIBLE sectors per device (Only 4MB visible to CPU) */
#define CFG_MAX_FLASH_SECT	64
#undef	CFG_FLASH_CHECKSUM
#define CFG_FLASH_ERASE_TOUT	60000		/* Flash Erase Timeout (ms) */
#define CFG_FLASH_WRITE_TOUT	500		/* Flash Write Timeout (ms) */

#define CFG_MONITOR_BASE	TEXT_BASE	/* start of monitor */

#if (CFG_MONITOR_BASE < CFG_FLASH_BASE)
#define CFG_RAMBOOT
#else
#undef CFG_RAMBOOT
#endif

#define CFG_FLASH_CFI_DRIVER
#define CFG_FLASH_CFI
#define CFG_FLASH_EMPTY_INFO
#define CFG_FLASH_AMD_CHECK_DQ7

/******************************************************************************
 * Local Bus Definitions
 ******************************************************************************/

/*
 * Chip Select 3: Co-Processor FPGA on the Local Bus
 * Size: 64KB
 */
#define CFG_BR3_PRELIM	 ((CFG_COP_FPGA_BASE & 0xFFFF8000) | 0x00000801)  /* 8 bit FPGA */
#define CFG_OR3_PRELIM	 ((~(CFG_COP_FPGA_SIZE - 1) & 0xFFFF8000) | 0x00000FF7)

/*
 * Chip Select 2: NVRAM on the Local Bus
 * Size: 128KB
 */
#define CFG_BR2_PRELIM	((CFG_NVRAM_BASE & 0xFFFF8000) | 0x00000801)  /* 8 bit NVRAM */
#define CFG_OR2_PRELIM	((~(CFG_NVRAM_SIZE - 1) & 0xFFFF8000) | 0x00000020)

/*
 * Chip Select 1: SYSPLD on the Local Bus
 * Size: 32KB (Since that is the minimal size for a chip select on eLBC).
 */
#define CFG_BR1_PRELIM	((CFG_SYSPLD_BASE & 0xFFFF8000) | 0x00000801)  /* 8 bit SysPLD */
#define CFG_OR1_PRELIM	((~(CFG_SYSPLD_SIZE - 1) & 0xFFFF8000) | 0x00000010)

/*
 * Chip Select 0: FLASH on the Local Bus
 * Size: 4MB (Since only that is what is visible to CPU)
 */
#define CFG_BR0_PRELIM	((CFG_FLASH_BASE & 0xFFFF8000) | 0x00000801) /* 8 bit NOR */
#define CFG_OR0_PRELIM	((~(CFG_FLASH_SIZE - 1) & 0xFFFF8000) | 0x00000030)  /* NOR Options */

/******************************************************************************/

/* Define to use L1 as initial stack */
#define CONFIG_L1_INIT_RAM	1
#define CFG_INIT_L1_LOCK	1
#define CFG_INIT_L1_ADDR	0xe4010000	/* Initial L1 address */
#define CFG_INIT_L1_END		0x00004000	/* End of used area in RAM */

#ifdef CFG_INIT_L1_LOCK
#define CFG_INIT_RAM_LOCK	1		/* Support legacy 85xx code */
#endif

/* define to use L2SRAM as initial stack */
#undef CONFIG_L2_INIT_RAM
#define CFG_INIT_L2_ADDR	0xf8fc0000
#define CFG_INIT_L2_END		0x00040000	/* End of used area in RAM */

#ifdef CONFIG_L1_INIT_RAM
#define CFG_INIT_RAM_ADDR	CFG_INIT_L1_ADDR
#define CFG_INIT_RAM_END	CFG_INIT_L1_END
#else
#define CFG_INIT_RAM_ADDR	CFG_INIT_L2_ADDR
#define CFG_INIT_RAM_END	CFG_INIT_L2_END
#endif

#define CFG_GBL_DATA_SIZE	128	/* num bytes initial data */
#define CFG_GBL_DATA_OFFSET	(CFG_INIT_RAM_END - CFG_GBL_DATA_SIZE)
#define CFG_INIT_SP_OFFSET	CFG_GBL_DATA_OFFSET

#define CFG_MONITOR_LEN		(512 * 1024)  /*  */
#define CFG_MALLOC_LEN		(1024 * 1024)	/* Reserved for malloc */

/* Serial Port - controlled on board with jumper J8
 * open - index 2
 * shorted - index 1
 */
#define CONFIG_CONS_INDEX	1
#undef	CONFIG_SERIAL_SOFTWARE_FIFO
#define CFG_NS16550
#define CFG_NS16550_SERIAL
#define CFG_NS16550_REG_SIZE	1
#define CFG_NS16550_CLK		get_bus_freq(0)

#define CFG_BAUDRATE_TABLE	\
	{300, 600, 1200, 2400, 4800, 9600, 19200, 38400,115200}

#define CFG_NS16550_COM1	(CFG_CCSRBAR+0x4500)
#define CFG_NS16550_COM2	(CFG_CCSRBAR+0x4600)

/* Use the HUSH parser */
#define CFG_HUSH_PARSER
#ifdef	CFG_HUSH_PARSER
#define CFG_PROMPT_HUSH_PS2 "> "
#endif

/* I2C */
#define CONFIG_FSL_I2C		/* Use FSL common I2C driver */
#define CONFIG_HARD_I2C		/* I2C with hardware support */
#undef	CONFIG_SOFT_I2C		/* I2C bit-banged */
#define CFG_I2C_SPEED		400000	/* I2C speed and slave address */
#define CFG_I2C_EEPROM_ADDR_LEN 1   /* SAS ADDED: JUNOS requires this set. */
#define CFG_I2C_SLAVE		0x7F
#define CFG_I2C_NOPROBES	{0x69}	/* Don't probe these addrs */
#define CFG_I2C_OFFSET		0x3000	/* I2C Cotroller #1 */
#define CFG_I2C_EEPROM_ADDR	0x51    /* ACX IDEEPROM */
#define CFG_I2C_SW1_ADDR   	0x70    /* I2C Switch   */
#define CFG_I2C_USB2513_ADDR   	0x2C    /* USB Hub      */
#define CFG_EEPROM_MAC_OFFSET   56

#ifndef CONFIG_NET_MULTI
#define CONFIG_NET_MULTI	1
#endif

#define CONFIG_MII		1	/* MII PHY management */
#define CONFIG_MII_DEFAULT_TSEC	1	/* Allow unregistered phys */

#define CONFIG_MPC85XX_TSEC1		1
#define CONFIG_MPC85XX_TSEC1_NAME	"fxp0"

#define TSEC1_PHY_ADDR	    1
#define TSEC1_FLAGS            (TSEC_GIGABIT | TSEC_REDUCED)
#define TSEC1_PHYIDX		0

#define CONFIG_ETHPRIME		"fxp0"

#define CONFIG_PHY_GIGE		1	/* Include GbE speed/duplex detection */

/* Default RE memsize in case NVRAM gets corrupted */
#define ACX1000_DEFAULT_RE_MEMSIZE 0x30000000UL
#define ACX2000_DEFAULT_RE_MEMSIZE 0x60000000UL

/*
 * Environment
 */
#if !defined(CFG_RAMBOOT)
#define CONFIG_CMD_ENV
#define CFG_ENV_IS_IN_FLASH     1
#define CFG_ENV_ADDR            0xFFF60000 
#define CFG_ENV_SECT_SIZE       (128 * 1024) /* 128k for env*/
#define CFG_ENV_SIZE            0x2000
#else
#define CFG_ENV_IS_NOWHERE      1       /* Store ENV in memory only */
#define CFG_ENV_ADDR            (CFG_MONITOR_BASE - 0x2000)
#define CFG_ENV_SIZE            0x2000
#endif

#define CFG_LOADS_BAUD_CHANGE	1	/* allow baudrate change */

/*
 * Command line configuration.
 */
#define  CONFIG_COMMANDS	(CONFIG_CMD_DFL \
				| CFG_CMD_PING \
				| CFG_CMD_I2C \
				| CFG_CMD_MII \
				| CFG_CMD_DIAG  \
				| CFG_CMD_USB 	\
				| CFG_CMD_CACHE \
				| CFG_CMD_ELF \
				| CFG_CMD_EEPROM \
				| CFG_CMD_BSP)

#define CONFIG_POST		(CFG_POST_MEMORY \
				| CFG_POST_SECUREBOOT \
				| CFG_POST_MEMORY_RAM \
				| CFG_POST_CPU \
				| CFG_POST_WATCHDOG \
				| CFG_POST_USB \
				| CFG_POST_NAND \
				| CFG_POST_UART \
				| CFG_POST_ETHER \
				| CFG_POST_SYSPLD \
				| CFG_POST_LOG)

#ifdef CONFIG_POST
#define CFG_CMD_POST_DIAG CFG_CMD_DIAG
#else
#define CFG_CMD_POST_DIAG       0
#endif

#include <cmd_confdefs.h>
#define CONFIG_JFFS2_CMDLINE

#undef CONFIG_WATCHDOG			/* watchdog disabled */

/*
 * Miscellaneous configurable options
 */
#define CFG_LONGHELP			/* undef to save memory	*/
#define CFG_LOAD_ADDR	0x2000000	/* default load address */
#define CONFIG_SYS_LOAD_ADDR CFG_LOAD_ADDR /* default load address */
#define CFG_PROMPT	"=> "		/* Monitor Command Prompt */
#if defined(CONFIG_CMD_KGDB)
#define CFG_CBSIZE	1024		/* Console I/O Buffer Size */
#else
#define CFG_CBSIZE	256		/* Console I/O Buffer Size */
#endif
#define CFG_PBSIZE (CFG_CBSIZE+sizeof(CFG_PROMPT)+16) /* Print Buffer Size */
#define CFG_MAXARGS	16		/* max number of command args */
#define CFG_BARGSIZE	CFG_CBSIZE	/* Boot Argument Buffer Size */
#define CFG_HZ		1000		/* decrementer freq: 1ms ticks */

/*
 * For booting Linux, the board info and command line data
 * have to be in the first 8 MB of memory, since this is
 * the maximum mapped by the Linux kernel during initialization.
 */
#define CFG_BOOTMAPSZ	(8 << 20)	/* Initial Memory map for Linux*/

/* Cache Configuration */
#define CFG_DCACHE_SIZE	32768
#define CFG_CACHELINE_SIZE	32
#if defined(CONFIG_CMD_KGDB)
#define CFG_CACHELINE_SHIFT	5	/*log base 2 of the above value*/
#endif

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

#undef CONFIG_HOSTNAME
#define CONFIG_ROOTPATH		/opt/nfsroot
#undef CONFIG_BOOTFILE

#define CONFIG_IPADDR		10.216.66.70
#define CONFIG_GATEWAYIP	10.216.79.254
#define CONFIG_NETMASK		255.255.240.0
#define CONFIG_SERVERIP		10.216.64.64

/* default location for tftp and bootm */
#define CONFIG_LOADADDR		1000000

#define CONFIG_BOOTDELAY 1	/* -1 disables auto-boot */
#define CONFIG_BOOT_RETRY_TIME -1
#define CONFIG_RESET_TO_RETRY 1
#undef  CONFIG_BOOTARGS		/* the boot command will set bootargs */
#define CONFIG_AUTOBOOT_KEYED   /* use key strings to stop autoboot */
#define CONFIG_AUTOBOOT_PASSWORD    /* check password before stopping autoboot */
#define CONFIG_PASSWORD_RETRY_CNT 10 /* Give retry of 10 times before erasing NAND Flash */

/* 
 * Using ctrl-c to stop booting and entering u-boot command mode.  
 * Space is too easy to break booting by accident.  Ctrl-c is common
 * but not too easy to interrupt the booting by accident.
 */
#define CONFIG_AUTOBOOT_PROMPT   \
    "Press Ctrl-C in next %d seconds to enter u-boot prompt...\n" 
#define CONFIG_AUTOBOOT_STOP_STR        "\x03"
#define CONFIG_AUTOBOOT_RECOVERY_PASSWORD_TIMEOUT 60
#define DEBUG_PASSWORD 0
#define DEBUG_BOOTKEYS 0

#define CONFIG_AUTOBOOT_PASSWORD_PROMPT \
    "Enter password:"


#define CONFIG_BAUDRATE	9600

#define UART1_BASE		(CFG_CCSRBAR + 0x4500)

/* 
 * Remember to change uart base address if CCSR BAR is changed
 */
#define	CONFIG_EXTRA_ENV_SETTINGS				\
 "getnew=tftp 0x1000000 bootstore/u-boot.bin; protect off 0xfff60000 0xffffffff; erase 0xfff60000 0xffffffff; cp.b 0x1000000 0xfff80000 0x80000;"\
 " protect on 0xfff60000 0xffffffff;\0"\
 "hw.uart.console=mm:0xF7F04500\0"				\
 "hw.uart.dbgport=mm:0xF7F04500\0"				\
 "netdev=fxp0\0"						\
 "ethact=fxp0\0"						\
 "ethprime=fxp0\0"						\
 "consoledev=ttyS0\0"					\
 "showmacnum=eeprom read 0x1000000 0x36 2;md.w 0x1000000 1\0"   \
 "run.slowtests=no\0"

/*
 * USB configuration
 */
#define CONFIG_USB_EHCI		1
#define CONFIG_USB_STORAGE	1
#define CONFIG_DOS_PARTITION
#define CONFIG_USB_SHOW_NO_PROGRESS
#define CONFIG_CMD_USB

#define CONFIG_BOOTCOMMAND  \
   "cp.b 0xffe00000 $loadaddr 0x100000; bootelf $loadaddr"

#define I2C_ID_ACX1000               0x0551
#define I2C_ID_ACX2000               0x0552
#define I2C_ID_ACX4000               0x0553
#define I2C_ID_ACX2100               0x0562
#define I2C_ID_ACX2200               0x0563
#define I2C_ID_ACX1100               0x0564
#define I2C_ID_ACX500_O_POE_DC       0x0577  /* Kotinos Outdoor DC Chassis */
#define I2C_ID_ACX500_O_POE_AC       0x0578  /* Kotinos Outdoor AC Chassis */
#define I2C_ID_ACX500_O_DC           0x0579  /* Kotinos Outdoor Minus DC Chassis */
#define I2C_ID_ACX500_O_AC           0x057a  /* Kotinos Outdoor Minus AC Chassis */
#define I2C_ID_ACX500_I_DC           0x057b  /* Kotinos Indoor DC Chassis */
#define I2C_ID_ACX500_I_AC           0x057c  /* Kotinos Indoor AC Chassis */

#define CASE_ALL_I2CID_ACX500_CHASSIS            \
    case I2C_ID_ACX500_O_POE_DC:                 \
    case I2C_ID_ACX500_O_POE_AC:                 \
    case I2C_ID_ACX500_O_DC:                     \
    case I2C_ID_ACX500_O_AC:                     \
    case I2C_ID_ACX500_I_DC:                     \
    case I2C_ID_ACX500_I_AC

#define CASE_ALL_I2CID_ACX500_OUTDOOR_CHASSIS    \
    case I2C_ID_ACX500_O_POE_DC:                 \
    case I2C_ID_ACX500_O_POE_AC:                 \
    case I2C_ID_ACX500_O_DC:                     \
    case I2C_ID_ACX500_O_AC                     

#define CASE_ALL_I2CID_ACX500_INDOOR_CHASSIS     \
    case I2C_ID_ACX500_I_DC:                     \
    case I2C_ID_ACX500_I_AC

#define IS_BOARD_ACX500(x)   ( ((x) ==  I2C_ID_ACX500_O_POE_DC) || \
                               ((x) ==  I2C_ID_ACX500_O_POE_AC) || \
                               ((x) ==  I2C_ID_ACX500_O_DC) ||     \
                               ((x) ==  I2C_ID_ACX500_O_AC) ||     \
                               ((x) ==  I2C_ID_ACX500_I_DC) ||     \
                               ((x) ==  I2C_ID_ACX500_I_AC) )

#define CONFIG_BOARD_TYPES /* We can have multiple boards */

/*
 * Set the bootloader verion in the env variable "ver"
 */
#define CONFIG_VERSION_VARIABLE 1 
#define CONFIG_SILENT_CONSOLE 1

#endif	/* __CONFIG_H */
