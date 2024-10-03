/*
 * Copyright (c) 2008-2011, Juniper Networks, Inc.
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
 * rps200 board configuration file
 *
 * Please refer to doc/README.rps200 for more info.
 *
 */

#ifndef __CONFIG_H
#define __CONFIG_H
#include "../../board/mv_feroceon/mv_kw/mvSysHwConfig.h"

#define CONFIG_RPS200
#define CONFIG_DISPLAY_BOARDINFO  /* checkboard */
#define CONFIG_EXTENDED_BDINFO

#define CONFIG_TWSI_SINGLE_MASTER
#define CONFIG_TWSI_REDUCE_DELAY
#define CONFIG_SIMPLIFY_DISPLAY

/* MPP */
#define RPS_MPP0_7              0x11110300
#define RPS_MPP8_15             0x11111111
#define RPS_GPP_OE              0xfffffffc
#define RPS_GPP_VAL	            0x00000001


/********************/
/* MV DEV SUPPORTS  */
/********************/	
//#define CONFIG_PCI           /* pci support               */
#undef CONFIG_PCI           /* pci support               */
#undef CONFIG_PCI_1         /* sec pci interface support */

/********************/
/* Environment variables */
/********************/	


#define CFG_ENV_IS_IN_FLASH		1

/**********************************/
/* Marvell Monitor Extension      */
/**********************************/
#define enaMonExt()( /*(!getenv("enaMonExt")) ||\*/\
		     ( getenv("enaMonExt") && \
                       ((!strcmp(getenv("enaMonExt"),"yes")) ||\
		       (!strcmp(getenv("enaMonExt"),"Yes"))) \
		     )\
		    )

/********/
/* CLKs */
/********/
#ifndef __ASSEMBLY__
extern unsigned int mvSysClkGet(void);
extern unsigned int mvTclkGet(void);

#define UBOOT_CNTR		0		/* counter to use for uboot timer */
#define MV_TCLK_CNTR		1		/* counter to use for uboot timer */
#define MV_REF_CLK_DEV_BIT	1000		/* Number of cycle to eanble timer */
#define MV_REF_CLK_BIT_RATE	100000		/* Ref clock frequency */
#define MV_REF_CLK_INPUT_GPP	6		/* Ref clock frequency input */

#define CFG_HZ			1000
#define CFG_TCLK                mvTclkGet()
#define CFG_BUS_HZ              mvSysClkGet()
#define CFG_BUS_CLK             CFG_BUS_HZ
#endif

/********************/
/* Dink PT settings */
/********************/
#define CFG_MV_PT

#ifdef CFG_MV_PT
#define CFG_PT_BASE  (CFG_MALLOC_BASE - 0x40000)
#endif /* #ifdef CFG_MV_PT */


/*************************************/
/* High Level Configuration Options  */
/* (easy to change)		     */
/*************************************/
#define CONFIG_MARVELL		1
#define CONFIG_ARM926EJS	1		/* CPU */


/* commands */

#define CONFIG_BOOTP_MASK	(CONFIG_BOOTP_DEFAULT | \
				 CONFIG_BOOTP_BOOTFILESIZE)


/* Default U-Boot supported features */
#if defined(MV_TINY_IMAGE)

#define CONFIG_COMMANDS (CFG_CMD_FLASH \
			 | CFG_CMD_ELF	\
			 | CFG_CMD_LOADB	\
			 | CFG_CMD_BDI \
			 | CFG_CMD_I2C \
			 | CFG_CMD_EEPROM \
			 | CFG_CMD_ENV	\
			 | CFG_CMD_NET	\
			 | CFG_CMD_PING \
			 | CFG_CMD_MEMORY \
			 | CFG_CMD_RUN)	
#else
#define CONFIG_COMMANDS ( CFG_CMD_DHCP	\
			 | CFG_CMD_ELF	\
			 | CFG_CMD_LOADB	\
			 | CFG_CMD_BDI \
			 | CFG_CMD_I2C \
			 | CFG_CMD_EEPROM \
			 | CFG_CMD_NET \
			 | CFG_CMD_PING \
			 | CFG_CMD_FLASH	\
			 | CFG_CMD_MEMORY	\
			 | CFG_CMD_ENV	\
			 | CFG_CMD_CONSOLE	\
			 | CFG_CMD_RUN	\
			 | CFG_CMD_MISC)	
						 
#endif


/* this must be included AFTER the definition of CONFIG_COMMANDS (if any) */
#include <cmd_confdefs.h>

#define	CFG_MAXARGS	16		/* max number of command args	*/


/* which initialization functions to call for this board */
#define CONFIG_MISC_INIT_R	1      /* after relloc initialization*/

#define CONFIG_ENV_OVERWRITE    /* allow to change env parameters */

#undef	CONFIG_WATCHDOG		/* watchdog disabled		*/

/* Cache */
#define CFG_CACHELINE_SIZE	32	


/* global definetions. */
#define	CFG_SDRAM_BASE		0x00000000


#define CFG_RESET_ADDRESS	0xffff0000

#define CFG_MALLOC_BASE		(3 << 20) /* 3M */

/*
 * When locking data in cache you should point the CFG_INIT_RAM_ADDRESS
 * To an unused memory region. The stack will remain in cache until RAM
 * is initialized 
*/
#define	CFG_MALLOC_LEN		(1 << 20)	/* (default) Reserve 1MB for malloc*/

#define CFG_GBL_DATA_SIZE	128  /* size in bytes reserved for init data */

#undef CONFIG_INIT_CRITICAL		/* critical code in start.S */

#define RPS_APP_START   (0xf8080000)
/* bootelf RPS_APP_START */
#define CONFIG_BOOTCOMMAND	\
    "bootelf 0xF8080000"

/********/
/* DRAM */
/********/

#define CFG_DRAM_BANKS		4

/* this defines whether we want to use the lowest CAL or the highest CAL available,*/
/* we also check for the env parameter CASset.					  */
#define MV_MIN_CAL

#define CFG_MEMTEST_START     0x00400000
#define CFG_MEMTEST_END       0x007fffff

/********************/
/* Serial + parser  */
/********************/
/*
 * The following defines let you select what serial you want to use
 * for your console driver.
 */
#define CONFIG_BAUDRATE         9600   /* console baudrate = 9600    */
#define CFG_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600 }
 
#define CFG_DUART_CHAN		0		/* channel to use for console */

#define CFG_INIT_CHAN1
#define CFG_INIT_CHAN2


#define CONFIG_LOADS_ECHO       0       /* echo off for serial download */
#define CFG_LOADS_BAUD_CHANGE           /* allow baudrate changes       */

#define CFG_CONSOLE_INFO_QUIET  /* don't print In/Out/Err console assignment. */

/* parser */
/* don't chang the parser if you want to load Linux(if you cahnge it to HUSH the cmdline will
	not pass to the kernel correctlly???) */
#define CFG_HUSH_PARSER
//#define CONFIG_AUTO_COMPLETE

#define CFG_PROMPT_HUSH_PS2	"> "

#define	CFG_LONGHELP			/* undef to save memory		*/
#define	CFG_PROMPT	"=> "		/* Monitor Command Prompt */
#define	CFG_CBSIZE	256		/* Console I/O Buffer Size	*/
#define	CFG_PBSIZE (CFG_CBSIZE+sizeof(CFG_PROMPT)+16) /* Print Buffer Size */

/************/
/* ETHERNET */
/************/
/* to change the default ethernet port, use this define (options: 0, 1, 2) */
#define CONFIG_NET_MULTI

#define ETHADDR          	"00:E0:0C:00:00:FD"
#define ETH1ADDR          	"00:E0:0C:00:01:FD"
#define ENV_ETH_PRIME			"egiga0"

/************/
/* USB	    */
/************/

#define ENV_USB0_MODE	"host"
#define ENV_USB1_MODE	"host"

#undef CONFIG_HOSTNAME
#undef CONFIG_ROOTPATH
#undef CONFIG_BOOTFILE 

#define CONFIG_IPADDR		192.168.1.253

#define CONFIG_SERVERIP		192.168.1.1

#define	CFG_LOAD_ADDR		0x00400000	/* default load address	*/

#undef	CONFIG_BOOTARGS

#define CONFIG_BOOTDELAY	1 		/* by default no autoboot */


/********/
/* I2C  */
/********/
#define CFG_I2C_EEPROM_ADDR_LEN 1
#define CFG_I2C_MULTI_EEPROMS
#define CFG_I2C_SPEED   100000		/* I2C speed default */

#define CFG_I2C_PCA9548_ADDR   0x77
#define CFG_I2C_PCA9546_ADDR   0x70
/* PS1 mux 0x77 chan 7, mux 70 chan 0 */
/* PS2 mux 0x77 chan 7, mux 70 chan 1 */
/* PS3 mux 0x77 chan 7, mux 70 chan 2 */
#define CFG_I2C_PS_EEPROM_ADDR   0x51
/* ID EEPROM mux 0x77 chan 6 */
#define CFG_I2C_ID_EEPROM_ADDR   0x52
/* SECURE ID EEPROM mux 0x77 chan 7, mux 70 chan 3 */
#define CFG_I2C_SECURE_ID_EEPROM_ADDR   0x50
#define CFG_I2C_SYSPLD_ADDR   0x31
#define CFG_I2C_CPU_SLAVE_ADDR   0x60

#define CFG_I2C_ID_EEPROM_CHAN   0x6
#define I2C_ID_EX200_RPS    0x0928  /* EX200 Redundant Power Supply */

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

#define CFG_EXTRA_FLASH_WIDTH	4	/* 32 bit */
#define CFG_BOOT_FLASH_WIDTH	1	/* 8 bit */

#define CFG_FLASH_ERASE_TOUT	120000/1000	/* 120000 - Timeout for Flash Erase (in ms) */
#define CFG_FLASH_WRITE_TOUT	500	/* 500 - Timeout for Flash Write (in ms) */
#define CFG_FLASH_LOCK_TOUT	500	/* 500- Timeout for Flash Lock (in ms) */

/***************************/
/* CFI FLASH organization  */
/***************************/
#define CFG_FLASH_CFI_DRIVER
#define CFG_FLASH_CFI		1
#undef CFG_FLASH_USE_BUFFER_WRITE
#define CFG_FLASH_QUIET_TEST
#define CFG_FLASH_BANKS_LIST	{BOOTDEV_CS_BASE}

#define CFG_FLASH_BASE		0xF8000000
#define CFG_FLASH_SIZE		0x800000

#define MONITOR_HEADER_LEN	0x200

#define CFG_ENV_SECT_SIZE			0x10000
#define CFG_MONITOR_LEN			(512 << 10) /* Reserve 512 kB for Monitor */
#define CFG_MONITOR_IMAGE_OFFSET			CFG_ENV_SECT_SIZE   /* offset of the monitor from the u-boot image */
#define CFG_MONITOR_BASE			 CFG_FLASH_BASE
#define CFG_ENV_OFFSET			(CFG_FLASH_SIZE - CFG_ENV_SECT_SIZE)
#define CFG_ENV_ADDR			(CFG_FLASH_BASE + CFG_FLASH_SIZE - CFG_ENV_SECT_SIZE)
#define CFG_ENV_SIZE			0x2000

/*-----------------------------------------------------------------------
 * NAND-FLASH stuff
 *-----------------------------------------------------------------------
 */
//#define CFG_NAND_LEGACY
//#define CFG_NAND_BASE	0xF8000000

#define CFG_MAX_NAND_DEVICE	1	/* Max number of NAND devices		*/
#define SECTORSIZE 512
//#define NAND_NO_RB

#define ADDR_COLUMN 1
#define ADDR_PAGE 2
#define ADDR_COLUMN_PAGE 3

#define NAND_ChipID_UNKNOWN	0x00
#define NAND_MAX_FLOORS 1
#define NAND_MAX_CHIPS 1

#if 0
#define NAND_DISABLE_CE(nand)
#define NAND_ENABLE_CE(nand)
#define NAND_CTL_CLRALE(nandptr)
#define NAND_CTL_SETALE(nandptr)
#define NAND_CTL_CLRCLE(nandptr)
#define NAND_CTL_SETCLE(nandptr)
#define NAND_WAIT_READY(nand)
#endif

/* These are for 8-bit Flash boards */
#define WRITE_NAND_COMMAND(d, adr) do{ *(volatile __u8 *)((unsigned long)adr + 0x1) = (__u8)(d); } while(0)
#define WRITE_NAND_ADDRESS(d, adr) do{ *(volatile __u8 *)((unsigned long)adr + 0x2) = (__u8)(d); } while(0)
#define WRITE_NAND(d, adr) do{ *(volatile __u8 *)((unsigned long)adr) = (__u8)d; } while(0)
#define READ_NAND(adr) ((volatile unsigned char)(*(volatile __u8 *)(unsigned long)adr))

#if 0
/* These are for 16-bit Flash boards */
#define WRITE_NAND_COMMAND(d, adr) do{ *(volatile __u8 *)((unsigned long)adr + 0x2) = (__u8)(d); } while(0)
#define WRITE_NAND_ADDRESS(d, adr) do{ *(volatile __u8 *)((unsigned long)adr + 0x4) = (__u8)(d); } while(0)
#define WRITE_NAND(d, adr) do{ *(volatile __u16 *)((unsigned long)adr) = (__u16)d; } while(0)
#define READ_NAND(adr) ((volatile unsigned char)(*(volatile __u16 *)(unsigned long)adr))
#endif

/*****************/
/* others        */
/*****************/
#define CFG_DFL_MV_REGS		0xd0000000 	/* boot time MV_REGS */
#define CFG_MV_REGS		INTER_REGS_BASE /* MV Registers will be mapped here */

#undef CONFIG_USE_IRQ
#define CONFIG_STACKSIZE	(1 << 20)	/* regular stack - up to 4M (in case of exception)*/
#define CONFIG_NR_DRAM_BANKS 	4 
#ifdef CONFIG_USE_IRQ
#define CONFIG_STACKSIZE_IRQ    (4*1024)    /* IRQ stack */
#define CONFIG_STACKSIZE_FIQ    (4*1024)    /* FIQ stack */
#endif

#define BOARD_LATE_INIT

#define CONFIG_AUTOBOOT_KEYED		/* use key strings to stop autoboot */
#define CONFIG_AUTOBOOT_STOP_STR	"\x03" /* ctrl-c */
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
#endif							/* __CONFIG_H */
