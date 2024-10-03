/*
 * Copyright (c) 2009-2010, Juniper Networks, Inc.
 * All rights reserved.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * mpc8548cds board configuration file
 *
 * Please refer to doc/README.mpc85xxcds for more info.
 *
 */
#ifndef __CONFIG_H
#define __CONFIG_H

/* High Level Configuration Options */
#define CONFIG_BOOKE		1	/* BOOKE */
#define CONFIG_E500			1	/* BOOKE e500 family */
#define CONFIG_MPC85xx		1	/* MPC8540/60/55/41/48 */
#define CONFIG_MPC8548		1	/* MPC8548 specific */
#define CONFIG_MPC8548CDS	1	/* MPC8548CDS board specific */
#define CONFIG_SRX3000		1	/* Aus REP board */

#undef USB_WRITE_READ         	/* enbale write and read for usbmemory stick */ 

#define CONFIG_PCI
#undef CONFIG_TSEC_ENET 		/* tsec ethernet support */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_SPD_EEPROM		/* Use SPD EEPROM for DDR setup*/
#define CONFIG_DDR_DLL			/* possible DLL fix needed */
#define CONFIG_DDR_2T_TIMING	/* Sets the 2T timing bit */

#define CONFIG_DDR_ECC			/* only for ECC DDR module */
#define CONFIG_ECC_INIT_VIA_DDRCONTROLLER	/* DDR controller or DMA? */
#define CONFIG_MEM_INIT_VALUE		0x0

#define RTC_I2C_ADDR	0x68 /* PGSCB RTC I2C Address */
#define CONFIG_RTC1672	1

/*
 * When initializing flash, if we cannot find the manufacturer ID,
 * assume this is the AMD flash associated with the CDS board.
 * This allows booting from a promjet.
 */
#define CONFIG_ASSUME_AMD_FLASH

#define MPC85xx_DDR_SDRAM_CLK_CNTL	/* 85xx has clock control reg */

#ifndef __ASSEMBLY__
extern unsigned long get_clock_freq(void);
#endif
#define CONFIG_SYS_CLK_FREQ	get_clock_freq() /* sysclk for MPC85xx */

/*
 * These can be toggled for performance analysis, otherwise use default.
 */
#define CONFIG_L2_CACHE		    	    /* toggle L2 cache 	*/
#define CONFIG_BTB			    		/* toggle branch predition */
#define CONFIG_ADDR_STREAMING		    /* toggle addr streaming   */

/*
 * Only possible on E500 Version 2 or newer cores.
 */
#define CONFIG_ENABLE_36BIT_PHYS	1


#define CONFIG_BOARD_EARLY_INIT_F	1		/* Call board_pre_init */

/* Turn mem test on, when need */
#undef	CFG_DRAM_TEST						/* memory test, takes time */
#define CFG_ALT_MEMTEST                     /* Select full-featured memory test */
#define CFG_MEMTEST_START		0x00200000	/* memtest works on */
#define CFG_MEMTEST_END			0x00400000

/*
 * Base addresses -- Note these are effective addresses where the
 * actual resources get mapped (not physical addresses)
 */
#define CFG_CCSRBAR_DEFAULT 	0xff700000	/* CCSRBAR Default */
#define CFG_CCSRBAR				0xe0000000	/* relocated CCSRBAR */
#define CFG_IMMR				CFG_CCSRBAR	/* PQII uses CFG_IMMR */

/*
 * DDR Setup
 */
#define CFG_DDR_SDRAM_BASE		0x00000000	/* DDR is system memory*/
#define CFG_SDRAM_BASE			CFG_DDR_SDRAM_BASE

#define SPD_EEPROM_ADDRESS		0x52        /* DDR DIMM */

/*
 * Make sure required options are set
 */
#ifndef CONFIG_SPD_EEPROM
#error ("CONFIG_SPD_EEPROM is required")
#endif

#undef CONFIG_CLOCKS_IN_MHZ


/*
 * Local Bus Definitions
 */

/*
 * FLASH on the Local Bus
 *
 * BR0:
 *    Base address 0 = 0xff00_0000 = BR1[0:16] = 1111 1111 0000 0000 0
 *    Port Size = 8 bits = BRx[19:20] = 01
 *    Use GPCM = BRx[24:26] = 000
 *    Valid = BRx[31] = 1
 *
 * 0    4    8    12   16   20   24   28
 * 1111 1111 0000 0000 0000 1000 0000 0001 = ff000801    BR0
 *
 * OR0:
 *    Addr Mask = 16M = ORx[0:16] = 1111 1111 0000 0000 0
 *    Reserved ORx[17:18] = 11, confusion here?
 *    CSNT = ORx[20] = 1
 *    ACS = half cycle delay = ORx[21:22] = 11
 *    SCY = 6 = ORx[24:27] = 0110
 *    TRLX = use relaxed timing = ORx[29] = 1
 *    EAD = use external address latch delay = OR[31] = 1
 *
 * 0    4    8    12   16   20   24   28
 * 1111 1111 0000 0000 0110 1110 0110 0101 = ff006e65    ORx
 *
 * 0    4    8    12   16   20   24   28
 * 1111 1111 0000 0000 0000 1000 0000 0001 = ff000801    BR0
 *
 * PARKES 
 *
 * BR1:
 *    Base address 0 = 0xfef0_0000 = BR1[0:16] = 1111 1110 1111 0000 0
 *    Port Size = 32 bits = BRx[19:20] = 11
 *    Use GPCM = BRx[24:26] = 000
 *    Valid = BRx[31] = 1
 *
 * 0    4    8    12   16   20   24   28
 * 1111 1111 0000 0000 0000 1000 0000 0001 = fef00801    BR0
 *
 * OR0:
 *    Addr Mask = 16M = ORx[0:16] = 1111 1111 0000 0000 0
 *    Reserved ORx[17:18] = 11, confusion here?
 *    CSNT = ORx[20] = 1
 *    ACS = half cycle delay = ORx[21:22] = 11
 *    SCY = 6 = ORx[24:27] = 0110
 *    TRLX = use relaxed timing = ORx[29] = 1
 *    EAD = use external address latch delay = OR[31] = 1
 *
 * 0    4    8    12   16   20   24   28
 * 1111 1111 0000 0000 0110 1110 0110 0101 = ff006e65    ORx
 *
 * 0    4    8    12   16   20   24   28
 * 1111 1111 0000 0000 0000 1000 0000 0001 = ff000801    BR0
 *
 */

#define CFG_LBC_BASE        0xf8000000  /* start of local bus addresss */

#define CFG_FLASH_BASE		0xff000000	/* start of FLASH 8M */

#define CFG_BR0_PRELIM		0xff000801
#define CFG_BR1_PRELIM		0xfef01801

#define	CFG_OR0_PRELIM		0xff006e65
#define	CFG_OR1_PRELIM		0xfff06e65

#define CFG_FLASH_BANKS_LIST	{CFG_FLASH_BASE}
#define CFG_MAX_FLASH_BANKS	    1		/* number of banks */
#define CFG_MAX_FLASH_SECT	    256		/* sectors per device */
#undef	CFG_FLASH_CHECKSUM
#define CFG_FLASH_ERASE_TOUT	60000	/* Flash Erase Timeout (ms) */
#define CFG_FLASH_WRITE_TOUT	500	/* Flash Write Timeout (ms) */

#define CFG_MONITOR_BASE    	TEXT_BASE	/* start of monitor */

#define CFG_FLASH_CFI_DRIVER
#define CFG_FLASH_CFI
#define CFG_FLASH_EMPTY_INFO

#define CFG_PARKES_BASE     	0xfef00000	/* Localbus SDRAM */
#define CFG_PARKES_SIZE	    	1			/* LBC SDRAM is 64MB */

#define CONFIG_L1_INIT_RAM
#define CFG_INIT_RAM_LOCK 		1
#define CFG_INIT_RAM_ADDR		0xe4010000	/* Initial RAM address */
#define CFG_INIT_RAM_END    	0x4000	    /* End of used area in RAM */

#define POST_STOR_WORD  		CFG_INIT_RAM_ADDR 

#define CFG_GBL_DATA_SIZE  		128	    	/* num bytes initial data */
#define CFG_GBL_DATA_OFFSET		(CFG_INIT_RAM_END - CFG_GBL_DATA_SIZE)
#define CFG_INIT_SP_OFFSET		CFG_GBL_DATA_OFFSET

#define CFG_MONITOR_LEN	    	(640 * 1024) /* Reserve 384 kB for Mon */
#define CFG_MALLOC_LEN	    	(512 * 1024)	/* Reserved for malloc */

#define CONFIG_BOARD_EARLY_INIT_R	1
#define CONFIG_MISC_INIT_R	1	/* call misc_init_r()		*/
/* Parkes Watchdog */
#define CONFIG_HW_WATCHDOG			1
/* #define INIT_FUNC_WATCHDOG_RESET	hw_watchdog_reset, */

/* Serial Port */
#define CONFIG_CONS_INDEX       	1
#undef	CONFIG_SERIAL_SOFTWARE_FIFO
#define CFG_NS16550
#define CFG_NS16550_SERIAL
#define CFG_NS16550_REG_SIZE    	-4
#define CFG_NS16550_CLK         	33792000

#define CFG_BAUDRATE_TABLE  \
	{300, 600, 1200, 2400, 4800, 9600, 19200, 38400,115200}

#define CFG_NS16550_COM1        (CFG_PARKES_BASE+0x200)
#define CFG_NS16550_COM2        (CFG_PARKES_BASE+0x220)

/* Use the HUSH parser */
#define CFG_HUSH_PARSER
#ifdef  CFG_HUSH_PARSER
#define CFG_PROMPT_HUSH_PS2 "> "
#endif

#define TEST_AA_PAT   0XAA
#define TEST_55_PAT   0X55
#define CHECKERBOARD_PATTERN_55 0x55555555
#define CHECKERBOARD_PATTERN_AA 0xaaaaaaaa

 /*
 * I2C
 */
#define CONFIG_FSL_I2C				/* Use FSL common I2C driver */
#define CONFIG_HARD_I2C				/* I2C with hardware support*/
#undef	CONFIG_SOFT_I2C				/* I2C bit-banged */
#define CFG_I2C_SPEED		400000	/* I2C speed and slave address */
#define CFG_I2C_EEPROM_ADDR	0x57
#define CFG_I2C_SLAVE		0x7F
#define CFG_I2C_NOPROBES	{0x69}	/* Don't probe these addrs */
#define CFG_I2C_OFFSET		0x3000
#define CFG_I2C2_OFFSET		0x3100

#define CFG_I2C_EEPROM_ADDR_LEN 1
#define CFG_I2C_CTRL_1    		1 
#define CFG_I2C_CTRL_2    		2

#define CONFIG_LAST_STAGE_INIT 

/*
 * main board EEPROM platform ID
 */
#define CONFIG_BOARD_TYPES	1	/* support board types		*/

/*
 * General PCI
 * Addresses are mapped 1-1.
 */
#define CFG_PCI1_MEM_BASE	0x80000000
#define CFG_PCI1_MEM_PHYS	CFG_PCI1_MEM_BASE
#define CFG_PCI1_MEM_SIZE	0x20000000	/* 512M */
#define CFG_PCI1_IO_BASE	0x00000000
#define CFG_PCI1_IO_PHYS	0xe2000000
#define CFG_PCI1_IO_SIZE	0x00100000	/* 1M */

#define CFG_PCI2_MEM_BASE	0xa0000000
#define CFG_PCI2_MEM_PHYS	CFG_PCI2_MEM_BASE
#define CFG_PCI2_MEM_SIZE	0x20000000	/* 512M */
#define CFG_PCI2_IO_BASE	0x00000000
#define CFG_PCI2_IO_PHYS	0xe2100000
#define CFG_PCI2_IO_SIZE	0x00100000	/* 1M */

/* PCI Express */
#define CONFIG_PCIE             1   
#define CFG_PCIE_MEM_BASE       0xE8000000
#define CFG_PCIE_MEM_SIZE       0x04000000    /* 64M */
#define CONFIG_SATA_SIL3132     1
#define CFG_SATA_MAX_DEVICE     2
#define CFG_SATA_MAX_DISK       1
#define CONFIG_LBA48

#if defined(CONFIG_PCI)

#define CONFIG_NET_MULTI
#define CONFIG_PCI_PNP	               	/* do pci plug-and-play */
#define CONFIG_85XX_PCI2

#undef CONFIG_EEPRO100
#undef CONFIG_TULIP

#undef  CONFIG_PCI_SCAN_SHOW		/* hide pci devices on startup */
#define CFG_PCI_SUBSYS_VENDORID 0x1057  /* Motorola */

#endif	/* CONFIG_PCI */

#undef CONFIG_SHOW_ACTIVITY

//#if defined(CONFIG_TSEC_ENET)

#ifndef CONFIG_NET_MULTI
#define CONFIG_NET_MULTI 	1
#endif


#define CONFIG_MII                  1  /* MII PHY management */
#define CONFIG_MPC85XX_TSEC2        1
#define CONFIG_MPC85XX_TSEC2_NAME   "eTSEC1"
#define CONFIG_MPC85XX_TSEC3        1
#define CONFIG_MPC85XX_TSEC3_NAME   "eTSEC2"
#define CONFIG_MPC85XX_TSEC4        1
#define CONFIG_MPC85XX_TSEC4_NAME   "eTSEC3"
#undef CONFIG_MPC85XX_FEC

#define TSEC1_PHY_ADDR		0
#define TSEC2_PHY_ADDR		0
#define TSEC3_PHY_ADDR		2
#define TSEC4_PHY_ADDR		1

#define TSEC1_PHYIDX		0
#define TSEC2_PHYIDX		0
#define TSEC3_PHYIDX		0
#define TSEC4_PHYIDX		0

/* Options are: eTSEC[0-3] */
#define CONFIG_ETHPRIME		"eTSEC0"

//#endif	/* CONFIG_TSEC_ENET */
/*
 * USB configuration
 */
#define CONFIG_USB_OHCI		1
#define CONFIG_USB_STORAGE	1
#define CONFIG_DOS_PARTITION

#define USB_OHCI_VEND_ID 		0x1131
#define USB_OHCI_DEV_ID	 		0x1561
#define USB_EHCI_DEV_ID	 		0x1562 /* EHCI not implemented */

#define USB_OHCI_VEND_ID_UPD720102	0x1033
#define USB_OHCI_DEV_ID_UPD720102	0x0035
#define USB_EHCI_DEV_ID_UPD720102	0x00E0 /* EHCI not implemented */

/*
 * Environment
 */
#define CFG_ENV_IS_IN_FLASH		1
#define CFG_ENV_ADDR			(CFG_MONITOR_BASE + 0xA0000)
#define CFG_ENV_SECT_SIZE		0x20000	/* 128K(one sector) for env */
#define CFG_ENV_SIZE			0x2000


#define CONFIG_LOADS_ECHO		1	/* echo on for serial download */
#define CFG_LOADS_BAUD_CHANGE	1	/* allow baudrate change */

#if defined(CONFIG_PCI)
#define  CONFIG_COMMANDS	(CONFIG_CMD_DFL \
				| CFG_CMD_PCI 	\
				| CFG_CMD_PING 	\
				| CFG_CMD_I2C 	\
				| CFG_CMD_MII 	\
				| CFG_CMD_USB 	\
                | CFG_CMD_DIAG 	\
				| CFG_CMD_DATE 	\
				| CFG_CMD_CACHE \
				| CFG_CMD_ELF 	\
				| CFG_CMD_FAT 	\
				| CFG_CMD_EEPROM \
				| CFG_CMD_BSP  	\
				| CFG_CMD_CACHE)
#else
#define  CONFIG_COMMANDS	(CONFIG_CMD_DFL \
				| CFG_CMD_PING 	\
				| CFG_CMD_I2C 	\
				| CFG_CMD_MII 	\
				| CFG_CMD_USB 	\
                | CFG_CMD_DIAG 	\
				| CFG_CMD_DATE 	\
				| CFG_CMD_CACHE	\
				| CFG_CMD_ELF 	\
				| CFG_CMD_FAT 	\
				| CFG_CMD_EEPROM \
				| CFG_CMD_BSP  	\
				| CFG_CMD_CACHE)
#endif

#define CONFIG_POST (CFG_POST_MEMORY \
				| CFG_POST_MEMORY_RAM \
				| CFG_POST_MEMORY_ADDR_RAM \
				| CFG_POST_MEMORY_SLOW_TEST_RAM \
				| CFG_POST_CPU \
				| CFG_POST_WATCHDOG \
				| CFG_POST_USB \
				| CFG_POST_UART \
				| CFG_POST_ETHER \
				| CFG_POST_PCI \
				| CFG_POST_LOG)


#ifdef CONFIG_POST
#define CFG_CMD_POST_DIAG CFG_CMD_DIAG
#else
#define CFG_CMD_POST_DIAG       0
#endif

#include <cmd_confdefs.h>

#undef CONFIG_WATCHDOG				/* watchdog disabled */

/*
 * Miscellaneous configurable options
 */
#define CFG_LONGHELP				/* undef to save memory	*/
#define CFG_LOAD_ADDR	0x2000000	/* default load address */
#define CFG_PROMPT		"=> "			/* Monitor Command Prompt */
#if (CONFIG_COMMANDS & CFG_CMD_KGDB)
#define CFG_CBSIZE		1024			/* Console I/O Buffer Size */
#else
#define CFG_CBSIZE		256				/* Console I/O Buffer Size */
#endif
#define CFG_PBSIZE 		(CFG_CBSIZE+sizeof(CFG_PROMPT)+16) /* Print Buffer Size */
#define CFG_MAXARGS		16				/* max number of command args */
#define CFG_BARGSIZE	CFG_CBSIZE	/* Boot Argument Buffer Size */
#define CFG_HZ			1000			/* decrementer freq: 1ms ticks */

/*
 * For booting Linux, the board info and command line data
 * have to be in the first 8 MB of memory, since this is
 * the maximum mapped by the Linux kernel during initialization.
 */
#define CFG_BOOTMAPSZ	(8 << 20) 	/* Initial Memory map for Linux*/

/* Cache Configuration */
#define CFG_DCACHE_SIZE		32768
#define CFG_CACHELINE_SIZE	32
#if (CONFIG_COMMANDS & CFG_CMD_KGDB)
#define CFG_CACHELINE_SHIFT	5		/*log base 2 of the above value*/
#endif

/*
 * Internal Definitions
 *
 * Boot Flags
 */
#define BOOTFLAG_COLD	0x01			/* Normal Power-On: Boot from FLASH */
#define BOOTFLAG_WARM	0x02			/* Software reboot */

#if (CONFIG_COMMANDS & CFG_CMD_KGDB)
#define CONFIG_KGDB_BAUDRATE	230400	/* speed to run kgdb serial port */
#define CONFIG_KGDB_SER_INDEX	2		/* which serial port to use */
#endif

#define CONFIG_NET_RETRY_COUNT 	1000

/*
 * Environment Configuration
 */

/* The mac addresses for all ethernet interface */
#if defined(CONFIG_TSEC_ENET)
#define CONFIG_ETHADDR   00:E0:0C:00:00:FD
#define CONFIG_HAS_ETH1
#define CONFIG_ETH1ADDR  00:E0:0C:00:01:FD
#define CONFIG_HAS_ETH2
#define CONFIG_ETH2ADDR  00:E0:0C:00:02:FD
#define CONFIG_HAS_ETH3
#define CONFIG_ETH3ADDR  00:E0:0C:00:03:FD
#endif

#define CONFIG_IPADDR    10.150.52.34

#define CONFIG_SERVERIP  10.150.52.32
#define CONFIG_GATEWAYIP 10.150.55.254
#define CONFIG_NETMASK   255.255.248.0

#define CONFIG_LOADADDR  200000     /*default location for tftp and bootm*/

#define CONFIG_BOOTDELAY 3          /* -1 disables auto-boot */
#undef  CONFIG_BOOTARGS             /* the boot command will set bootargs*/

#define CONFIG_BAUDRATE 	9600

#define	CONFIG_EXTRA_ENV_SETTINGS				        				\
   "netdev=eth0\0"                                                      \
   "consoledev=ttyS1\0"                                                 \
   "ethact=eTSEC3\0"                                                    \
   "hw.uart.console=mm:0xfef00200,rs:2\0"                               \
   "hw.uart.dbgport=mm:0xfef00200,rs:2\0"                               \
   "use_bootp=1\0"                                                      \
   "bootcmd=cp.l 0xffe00000 0x100000 0x30000; bootelf 0x100000\0"    

#define CONFIG_AUTOBOOT_KEYED           /* use key strings to stop autoboot */
/*
 * Using ctrl-c to stop booting and entering u-boot command mode.            
 * Space is too easy to break booting by accident.  Ctrl-c is common
 * but not too easy to interrupt the booting by accident.
 */               
#define CONFIG_AUTOBOOT_STOP_STR        "\x03"
#define CONFIG_SILENT_CONSOLE   0
#endif	/* __CONFIG_H */
