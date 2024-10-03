/*
 * Copyright (c) 2010-2012, Juniper Networks, Inc.
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

#ifndef __CONFIG_H
#define __CONFIG_H

#include "../../board/mv_feroceon/mv_dd/mvSysHwConfig.h"


/********************/
/* MV DEV SUPPORTS  */
/********************/	
#define CONFIG_PCI           /* pci support               */
#undef CONFIG_PCI_1         /* sec pci interface support */

/********************/
/* Environment variables */
/********************/	


#define CFG_ENV_IS_IN_FLASH		1

/**********************************/
/* Marvell Monitor Extension      */
/**********************************/
/* always true for EX3300 */
#define enaMonExt() 1

/*Dual CPU support*/
#define MASTER_CPU	0
#define SLAVE_CPU	1

/********/
/* CLKs */
/********/
#ifndef __ASSEMBLY__
extern unsigned int mvSysClkGet(void);
extern unsigned int mvTclkGet(void);
#define UBOOT_CNTR      0           /* counter to use for uboot timer */
#define MV_TCLK_CNTR    1		    /* counter to use for uboot timer */
#define MV_REF_CLK_DEV_BIT  1000    /* Number of cycle to enable timer */
#define MV_REF_CLK_BIT_RATE 100000  /* Ref clock frequency */
#define MV_REF_CLK_INPUT_GPP    6   /* Ref clock frequency input */

#define CFG_HZ			1000
#define CFG_TCLK                mvTclkGet()
#define CFG_BUS_HZ              mvSysClkGet()
#define CFG_BUS_CLK             CFG_BUS_HZ
#endif

/*
 * When locking data in cache you should point the CFG_INIT_RAM_ADDRESS
 * To an unused memory region. The stack will remain in cache until RAM
 * is initialized 
*/

#define CFG_GBL_DATA_SIZE   256     /* size in bytes reserved for init data */

#define CFG_MALLOC_LEN      (1<<20) /* (default) Reserve ~1MB for malloc*/

#define CFG_MALLOC_BASE     0x100000    /* 1M */ 

#undef CONFIG_INIT_CRITICAL		/* critical code in start.S */

/********************/
/* Dink PT settings */
/********************/
#define CFG_MV_PT

#ifdef CFG_MV_PT
#define CFG_PT_BASE  (CFG_MALLOC_BASE - 0x20000)
#define CFG_PT_BASE_SLAVE_CPU  (CFG_PT_BASE - 0x20000)
#endif /* #ifdef CFG_MV_PT */


/*************************************/
/* High Level Configuration Options  */
/* (easy to change)		     */
/*************************************/
#define CONFIG_MARVELL		1
#define CONFIG_ARM926EJS	1		/* CPU */
#define CONFIG_EX3300		1
#define CONFIG_SIMPLIFY_DISPLAY
#define CONFIG_DISPLAY_BOARDINFO  /* checkboard */
#define CONFIG_EXTENDED_BDINFO

#define CONFIG_SPI_SW_PROTECT

#define CFG_NAND_QUIET_TEST
#define USB_MULTI_CONTROLLER

#define CONFIG_PRODUCTION

#define CONFIG_SHOW_ACTIVITY

#define USB_WRITE_READ  /* USB write sub-command */

/* commands */

#define CONFIG_BOOTP_MASK	(CONFIG_BOOTP_DEFAULT | \
				 CONFIG_BOOTP_BOOTFILESIZE)


/* Default U-Boot supported features */
#define CONFIG_COMMANDS_TEMP ( CFG_CMD_DHCP	\
			 | CFG_CMD_ELF	\
			 | CFG_CMD_I2C \
			 | CFG_CMD_EEPROM \
			 | CFG_CMD_PCI \
			 | CFG_CMD_NET \
			 | CFG_CMD_PING \
			 | CFG_CMD_BSP	\
			 | CFG_CMD_BDI \
			 | CFG_CMD_FLASH	\
			 | CFG_CMD_MEMORY	\
			 | CFG_CMD_ENV	\
			 | CFG_CMD_BOOTD	\
			 | CFG_CMD_CONSOLE	\
			 | CFG_CMD_RUN	\
			 | CFG_CMD_MISC	\
			 | CFG_CMD_USB \
			 )
			 
#if defined(CONFIG_NAND_DIRECT)
#define CONFIG_COMMANDS (CONFIG_COMMANDS_TEMP | CFG_CMD_NAND)
#else
#define CONFIG_COMMANDS (CONFIG_COMMANDS_TEMP)
#endif

/*  *  *  *  *  * Power On Self Test support * * * * *  */
#define CONFIG_POST ( CFG_POST_MEMORY  \
					| CFG_POST_MEMORY_RAM  \
					| CFG_POST_I2C  \
					| CFG_POST_CPU  \
					| CFG_POST_USB \
					| CFG_POST_PCI \
					| CFG_POST_ETHER \
					| CFG_POST_UART \
					| CFG_POST_WATCHDOG)
						 
/* this must be included AFTER the definition of CONFIG_COMMANDS (if any) */
#include <cmd_confdefs.h>

#define	CFG_MAXARGS     16      /* max number of command args	*/

/* which initialization functions to call for this board */
#define CONFIG_MISC_INIT_R	1      /* after relloc initialization*/
#undef CONFIG_DISPLAY_MEMMAP    /* at the end of the boot show the memory map*/

#define CONFIG_ENV_OVERWRITE    /* allow to change env parameters */

#undef	CONFIG_WATCHDOG		/* watchdog disabled		*/

/* Cache */
#define CFG_CACHELINE_SIZE	32	

/* global definetions. */
#define	CFG_SDRAM_BASE		0x00000000

#define CFG_RESET_ADDRESS	0xffff0000

#define CFG_SYSPLD_ADDRESS DEVICE_CS1_BASE

/********/
/* DRAM */
/********/

/* whether we want to use the lowest CAL or the highest CAL available,*/
/* we also check for the env parameter CASset.                      */
#define MV_MIN_CAL

#define CFG_MEMTEST_START     0x00300000
#define CFG_MEMTEST_END       0x00400000

/********/
/* RTC  */
/********/

/********************/
/* Serial + parser  */
/********************/
/*
 * The following defines let you select what serial you want to use
 * for your console driver.
 */
#define CONFIG_BAUDRATE         9600
#define CFG_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, \
                            115200, 230400, 460800, 921600 }
#define CFG_DUART_CHAN		0		/* channel to use for console */

#define CFG_INIT_CHAN1
#define CFG_INIT_CHAN2


#define CONFIG_LOADS_ECHO       0       /* echo off for serial download */
#define CFG_LOADS_BAUD_CHANGE           /* allow baudrate changes       */

#define CFG_CONSOLE_INFO_QUIET  /* don't print In/Out/Err console assignment.*/
#define CONFIG_SILENT_CONSOLE  /* define for Pex complince */
#define CONFIG_AUTOBOOT_KEYED		/* use key strings to stop autoboot */
#define CONFIG_AUTOBOOT_STOP_STR	"\x03" /* ctrl-c */

/* parser */
#define CFG_HUSH_PARSER 
#undef CONFIG_AUTO_COMPLETE

#define CFG_PROMPT_HUSH_PS2	"> "

#define	CFG_LONGHELP        /* undef to save memory		*/
#define	CFG_PROMPT	"=> "	/* Monitor Command Prompt	*/
#define	CFG_CBSIZE	1024    /* Console I/O Buffer Size	*/
#define	CFG_PBSIZE (CFG_CBSIZE+sizeof(CFG_PROMPT)+16)   /* Print Buffer Size */

/************/
/* ETHERNET */
/************/
#define CONFIG_NET_MULTI

#undef CONFIG_HOSTNAME
#undef CONFIG_ROOTPATH
#undef CONFIG_BOOTFILE 

#undef CONFIG_IPADDR
#undef CONFIG_NETMASK
#undef CONFIG_GATEWAYIP
#undef CONFIG_SERVERIP

#define ETHADDR          	"00:00:00:00:51:81"
#define ETH1ADDR          	"00:00:00:00:51:82"
#define ENV_ETH_PRIME			"egiga0"

/************/
/* PCI	    */
/************/
#if defined(MV_INCLUDE_PCI)

#if defined(DB_PEX_PCI)
#define ENV_PCI_MODE	"device"	/* PCI */
#else
#define ENV_PCI_MODE	"host"		/* PCI */
#endif

#endif

/************/
/* USB	    */
/************/

#define CONFIG_USB_EHCI		1
#define CONFIG_USB_STORAGE	1
#define CONFIG_DOS_PARTITION
#define CONFIG_USB_SHOW_NO_PROGRESS
#define LITTLEENDIAN		1

#if defined(CONFIG_NAND_DIRECT)
#define CFG_USB_EHCI_BASE getUsbBase(1)
#define CFG_USB_EHCI_HCCR_BASE getUsbHccrBase(1)
#define CFG_USB_EHCI_HCOR_BASE getUsbHcorBase(1)
#else
#define CFG_USB_EHCI_BASE getUsbBase(0)
#define CFG_USB_EHCI_HCCR_BASE getUsbHccrBase(0)
#define CFG_USB_EHCI_HCOR_BASE getUsbHcorBase(0)
#endif

/***************************************/
/* LINUX BOOT and other ENV PARAMETERS */
/***************************************/

#define	CFG_LOAD_ADDR		0x02000000  /* default load address	*/

/* auto boot*/
#define CONFIG_BOOTDELAY	1 		/* by default no autoboot */

/********/
/* I2C  */
/********/
#define CONFIG_TWSI_SINGLE_MASTER
#define CONFIG_TWSI_REDUCE_DELAY

#define CFG_I2C_EEPROM_ADDR_LEN 	1
#define CFG_I2C_MULTI_EEPROMS
#define CFG_I2C_SPEED   		100000		/* I2C speed default */

/* CPU I2C settings */
#define I2C_MAIN_MUX_ADDR       0x70
#define I2C_RPS_ADDR            0x61 /* ctrl 1 */ 
#define I2C_INA219_ADDR         0x40 /* ctrl 1 */
#define I2C_MAX6581_ADDR        0x4D /* ctrl 0 mux 0x70 ch 0 */
#define I2C_POE_EEPROM_ADDR     0x53 /* ctrl 0 mux 0x70 ch 1 */
#define I2C_POE_ADDR            0x2C /* ctrl 0 mux 0x70 ch 1 */
#define I2C_POWR1014A_ADDR      0x2B /* ctrl 0 mux 0x70 ch 2 */
#define I2C_RTC_ADDR            0x51 /* ctrl 0 mux 0x70 ch 2 */
#define I2C_MAIN_EEPROM_ADDR    0x72 /* ctrl 0 mux 0x70 ch 3 */

#define I2C_ID_JUNIPER_EX3300_48_P      0x09EB  /* EX3300 48-port POE */
#define I2C_ID_JUNIPER_EX3300_48_T      0x09EC  /* EX3300 48-port Non POE */
#define I2C_ID_JUNIPER_EX3300_24_P      0x09ED  /* EX3300 24-port POE */
#define I2C_ID_JUNIPER_EX3300_24_T      0x09EE  /* EX3300 24-port Non POE */
#define I2C_ID_JUNIPER_EX3300_48_T_BF   0x09EF  /* EX3300 48-port BF */
#define I2C_ID_JUNIPER_EX3300_24_T_DC   0x09F0  /* EX3300 24-port DC Pwr */


/********/
/* PCI  */
/********/
#ifdef CONFIG_PCI
 #define CONFIG_PCI_HOST PCI_HOST_FORCE /* select pci host function     */
 #define CONFIG_PCI_PNP          	/* do pci plug-and-play         */

 #undef CONFIG_SK98			/* yukon */
 #undef CONFIG_DRIVER_RTL8029
 #undef CONFIG_EEPRO100	

#endif /* #ifdef CONFIG_PCI */

#define PCI_HOST_ADAPTER 0              /* configure ar pci adapter     */
#define PCI_HOST_FORCE   1              /* configure as pci host        */
#define PCI_HOST_AUTO    2              /* detected via arbiter enable  */

/***********************/
/* FLASH organization  */
/***********************/

/*
 * When CFG_MAX_FLASH_BANKS_DETECT is defined, the actual number of Flash
 * banks has to be determined at runtime and stored in a gloabl variable
 * mv_board_num_flash_banks. The value of CFG_MAX_FLASH_BANKS_DETECT is only
 * used instead of CFG_MAX_FLASH_BANKS to allocate the array flash_info, and
 * should be made sufficiently large to accomodate the number of banks that
 * might actually be detected.  Since most (all?) Flash related functions use
 * CFG_MAX_FLASH_BANKS as the number of actual banks on the board, it is
 * defined as mv_board_num_flash_banks.
 */
#define CFG_MAX_FLASH_BANKS_DETECT	5
#ifndef __ASSEMBLY__
extern int mv_board_num_flash_banks;
#endif
#define CFG_MAX_FLASH_BANKS (mv_board_num_flash_banks)

#define CFG_MAX_FLASH_SECT	300	/* max number of sectors on one chip */
#define CFG_FLASH_PROTECTION    1

#define CFG_BOOT_FLASH_WIDTH	1	/* 8 bit */

#define CFG_FLASH_ERASE_TOUT	120000/1000	/* 120000ms - Timeout for Erase */
#define CFG_FLASH_WRITE_TOUT	500	/* 500 - Timeout for Flash Write (in ms) */
#define CFG_FLASH_LOCK_TOUT	500	/* 500- Timeout for Flash Lock (in ms) */


/***************************/
/* CFI FLASH organization  */
/***************************/
#define CFG_FLASH_CFI_DRIVER
#define CFG_FLASH_CFI		1
#define CFG_FLASH_USE_BUFFER_WRITE
#define CFG_FLASH_QUIET_TEST
#define CFG_FLASH_BANKS_LIST	{BOOTDEV_CS_BASE}
#if defined(__BE)
#define CFG_WRITE_SWAPPED_DATA
#endif

#define CFG_FLASH_BASE		0xFF800000
#define CFG_FLASH_SIZE      	0x800000
#define CFG_ENV_SECT_SIZE			0x10000		 
#define MONITOR_HEADER_LEN	0x0
#define CFG_MONITOR_LEN     (512 << 10) /* Reserve 512 kB for Monitor */
#define CFG_MONITOR_IMAGE_OFFSET	0	/* offset of the monitor  */
#define CFG_MONITOR_BASE		(CFG_FLASH_BASE + 0x780000)
#define CFG_ENV_OFFSET    		0
#define CFG_ENV_ADDR    		CFG_FLASH_BASE
#define CFG_ENV_SIZE		0x2000
#define CFG_OPQ_OFFSET    		0x10000
#define CFG_OPQ_ENV_ADDR        (CFG_FLASH_BASE + 0x10000) /* for RE nv env */
#define CFG_OPQ_ENV_SECT_SIZE   0x10000  /* 64K(1x64K sector) for RE nv env */

#define CFG_UPGRADE_BOOT_STATE_OFFSET  0xF0000 /* from beginning of flash */
#define CFG_UPGRADE_BOOT_SECTOR_SIZE  0x10000  /* 64k */

#define CFG_IMG_HDR_OFFSET 0x30 /* from image address */
#define CFG_CHKSUM_OFFSET 0x100 /* from image address */

#define CFG_UPGRADE_BOOT_IMG_ADDR  (CFG_FLASH_BASE + 0x680000)
#define CFG_UPGRADE_LOADER_ADDR (CFG_FLASH_BASE + 0x600000)

#define CFG_READONLY_BOOT_IMG_ADDR  (CFG_FLASH_BASE + 0x780000)
#define CFG_READONLY_LOADER_ADDR (CFG_FLASH_BASE + 0x700000)

#define CFG_READONLY_UBOOT_LOAD_ADDR  0x200000

#define CFG_UPGRADE_UBOOT_LOAD_ADDR  0x300000

#define CFG_VERSION_STRING_OFFSET  0x158

#define CFG_JUMP_OFFSET			0x10000

/*-----------------------------------------------------------------------
 * NAND-FLASH stuff
 *-----------------------------------------------------------------------*/
/* Use the new NAND code. */

#if defined(CONFIG_NAND_DIRECT)
#define CFG_MAX_NAND_DEVICE     1       /* Max number of NAND devices */
#define NAND_MAX_CHIPS          CFG_MAX_NAND_DEVICE
#endif

#define BOARD_LATE_INIT


/*****************/
/* others        */
/*****************/
#define CFG_DFL_MV_REGS		0xd0000000 	/* boot time MV_REGS */
#define CFG_MV_REGS     INTER_REGS_BASE /* MV Registers will be mapped here */

#define CONFIG_USE_IRQ
#define CONFIG_STACKSIZE	(1 << 20)	/* regular stack - up to 4M */
#ifdef CONFIG_USE_IRQ
#define CONFIG_STACKSIZE_IRQ	(4*1024)	/* IRQ stack */
#define CONFIG_STACKSIZE_FIQ	(4*1024)	/* FIQ stack */
#endif
#define CONFIG_NR_DRAM_BANKS 	4 

#define swap_ushort(x) \
	({ unsigned short x_ = (unsigned short)x; \
	 (unsigned short)( \
		((x_ & 0x00FFU) << 8) | ((x_ & 0xFF00U) >> 8) ); \
	})
#define swap_ulong(x) \
	({ unsigned long x_ = (unsigned long)x; \
	 (unsigned long)( \
		((x_ & 0x000000FFUL) << 24) | \
		((x_ & 0x0000FF00UL) <<	 8) | \
		((x_ & 0x00FF0000UL) >>	 8) | \
		((x_ & 0xFF000000UL) >> 24) ); \
	})

#define swap_ull(x) \
	({ unsigned long long x_ = (unsigned long long)x; \
	 (unsigned long long)( \
		((x_ & 0x00000000FFFFFFFFULL) << 32) | \
		((x_ & 0xFFFFFFFF00000000ULL) >> 32) ); \
	})

#define CONFIG_LOADADDR  0x400000   /*default location for tftp and bootm*/
#if defined(CONFIG_UPGRADE)
#define CONFIG_BOOTCOMMAND  \
   "bootelf 0xffe00000"
#else
#define CONFIG_BOOTCOMMAND  \
   "bootelf 0xfff00000"
#endif

/* Enable an alternate, more extensive memory test */
#define CFG_ALT_MEMTEST
#define CFG_STATIC_MEMTEST_PATTERN
#define CONFIG_LOOPW            1       /* enable loopw command         */

#define RAMTEST_64BIT
#define CFG_64BIT_VSPRINTF  1
#define CFG_64BIT_STRTOUL   1

#define	CFG_VERSION_MAJOR	1
#define	CFG_VERSION_MINOR	0
#define	CFG_VERSION_PATCH	0

#endif							/* __CONFIG_H */
