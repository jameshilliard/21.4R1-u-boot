/*
 * Copyright (c) 2006-2013, Juniper Networks, Inc.
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
 * ex3242 board configuration file
 *
 * Please refer to doc/README.ex3242 for more info.
 *
 */
#ifndef __CONFIG_H
#define __CONFIG_H

/* High Level Configuration Options */
#define CONFIG_BOOKE		1	/* BOOKE */
#define CONFIG_E500			1	/* BOOKE e500 family */
#define CONFIG_MPC85xx		1	/* MPC8540/60/55/41/48/33/44 */
#define CONFIG_MPC8544		1	/* MPC8544 */
#define CONFIG_EX3242		1	/* ex3242 board */

#define CFG_NAND_QUIET_TEST
#define CONFIG_NAND_CMD_DELAY_EX		7
#define CONFIG_NAND_WR_DELAY_EX			5
#define CONFIG_NAND_DELAY_EX			3
#define CONFIG_NAND_WRITE_DELAY_EX		700
#define MAMR_OP_NORMAL					0xCFFFFFC0
#define MAMR_OP_MASK					0x30000000
#define MAMR_NORMAL_OP					0
#define MAMR_WRITE_UPM					0x10000000
#define MAMR_READ_UPM					0x20000000
#define MAMR_RUN_PATTERN				0x30000000
#define MAD_READ_DATA_PATTERN			0x0
#define MAD_WRITE_COMMAND_PATTERN		0x8
#define MAD_WRITE_ADDRESS_PATTERN		0x10
#define MAD_WRITE_DATA_PATTERN			0x18
#define GPIO_NAND_READY					0x08000000


#define USB_WRITE_READ         /* enbale write and read for usb memory */ 

#define CONFIG_NOR
#define CONFIG_PCI

/* use local modified tsec driver for SGMII interface */
//#define CONFIG_TSEC_ENET 		/* tsec ethernet support */
#define CONFIG_ENV_OVERWRITE
#undef CONFIG_SPD_EEPROM		/* Use SPD EEPROM for DDR setup*/
#undef CONFIG_DDR_DLL			/* possible DLL fix needed */
#undef CONFIG_DDR_2T_TIMING		/* Sets the 2T timing bit */

#define CONFIG_DDR_ECC			/* only for ECC DDR module */
#undef CONFIG_ECC_INIT_VIA_DDRCONTROLLER	/* DDR controller or DMA? */
#define CONFIG_MEM_INIT_VALUE		0xDeadBeef


/*
 * When initializing flash, if we cannot find the manufacturer ID,
 * assume this is the AMD flash associated with the CDS board.
 * This allows booting from a promjet.
 */
#define CONFIG_ASSUME_AMD_FLASH

#define MPC85xx_DDR_SDRAM_CLK_CNTL	/* 85xx has clock control reg */

#define CONFIG_SYS_CLK_FREQ	100000000 /* sysclk for java */

/*
 * These can be toggled for performance analysis, otherwise use default.
 */
#define CONFIG_L2_CACHE		    	    /* toggle L2 cache 	*/
#define CONFIG_BTB			    /* toggle branch predition */
#define CONFIG_ADDR_STREAMING		    /* toggle addr streaming   */

/*
 * Only possible on E500 Version 2 or newer cores.
 */
#define CONFIG_ENABLE_36BIT_PHYS	1


#define CONFIG_BOARD_EARLY_INIT_F	1	/* Call board_pre_init */
#undef CONFIG_BOARD_EARLY_INIT_R
#define CONFIG_MISC_INIT_R	1	/* call misc_init_r()		*/

/* Enable an alternate, more extensive memory test */
#define CFG_ALT_MEMTEST
#define CONFIG_LOOPW            1       /* enable loopw command         */

#undef	CFG_DRAM_TEST			/* memory test, takes time */
#define CFG_MEMTEST_START	0x00200000	/* memtest works on */
#define CFG_MEMTEST_END		0x00400000

/*
 * Base addresses -- Note these are effective addresses where the
 * actual resources get mapped (not physical addresses)
 */
#define CFG_CCSRBAR_DEFAULT 	0xff700000	/* CCSRBAR Default */
#define CFG_CCSRBAR		0xfef00000	/* relocated CCSRBAR */
#define CFG_IMMR		CFG_CCSRBAR	/* PQII uses CFG_IMMR */

#define CFG_UPGRADE_BOOT_STATE_OFFSET  0xE000 /* from beginning of flash */
#define CFG_UPGRADE_BOOT_SECTOR_SIZE  0x2000  /* 8k */

#define CFG_IMG_HDR_OFFSET 0x30 /* from image address */
#define CFG_CHKSUM_OFFSET 0x100 /* from image address */

#define CFG_UPGRADE_BOOT_IMG_ADDR  0xffe80000
#define CFG_UPGRADE_LOADER_ADDR 0xffe00000
#define CFG_UPGRADE_BOOT_ADDR  0xffeffffc

#define CFG_READONLY_BOOT_IMG_ADDR  0xfff80000
#define CFG_READONLY_LOADER_ADDR 0xfff00000
#define CFG_READONLY_BOOT_ADDR  0xfffffffc

/*
 * DDR Setup
 */
#define CFG_DDR_SDRAM_BASE	0x00000000	/* DDR is system memory*/
#define CFG_SDRAM_BASE		CFG_DDR_SDRAM_BASE

#if defined(CONFIG_SPD_EEPROM)
#define CFG_DDR_CS                4
/*
 * Determine DDR configuration from I2C interface.
 */
#define SPD_EEPROM_ADDRESS      {0x54, 0x55}	/* DDR i2c spd addresses	*/

#else
/*
  * Manually set up DDR parameters
  */
#define CFG_SDRAM_SIZE	512		/* DDR is 512MB */
#define CFG_SDRAM_SIZE_EX3200	512             /* DDR is 512MB */
#define CFG_SDRAM_SIZE_EX4200	1024            /* DDR is 1G */
#define CFG_DDR_CS0_BNDS	0x0000001F	/* 0-512MB */
#define CFG_DDR_CS0_CONFIG	0x80010202
#define CFG_DDR_3BIT_BANKS_CONFIG  	0x00004000 /* 3 bit logical banks */
#define CFG_DDR_REFREC	0x00010000
#define CFG_DDR_TIMING_0	0x00260802
//#define CFG_DDR_TIMING_2	0x02A04CC8 /* optimize Espresso */
//#define CFG_DDR_TIMING_2	0x032048C8
//#define CFG_DDR_CFG	0xC3008008
//#define CFG_DDR_MODE	0x00400452
//#define CFG_DDR_MODE	0x00460452  
#define CFG_DDR_MODE_REV1_0	0x00060452  
#define CFG_DDR_MODE_2	0x0
#define CFG_DDR_INTERVAL	0x06090100
#define CFG_DDR_INTERVAL_CCB_300       0x04880100
//#define CFG_DDR_CLK_CTRL	0x03800000
#define CFG_DDR_CLK_CTRL	0x02800000 /* latest */
//#define CFG_DDR_CLK_CTRL	0x02000000 /* optimize Espresso */
#define CFG_DDRCDR	0x0

#define CFG_DDR_CAS5_TIMING_1	0x39397322 /* old */
#define CFG_DDR_CAS5_TIMING_2	0x03204CC8 /* latest *//* old */
#define CFG_DDR_CAS5_CFG	0xC3008000 /* old */
#define CFG_DDR_CAS5_MODE	0x00040452  /* old */
#define CFG_DDR_CAS5_CFG_2	0x04400000

#undef DDR_CAS_5
#undef DDR_CAS_4
#define DDR_CAS_3

#if defined(DDR_CAS_5)
#define CFG_DDR_TIMING_1	0x39397322 /* old */
#define CFG_DDR_TIMING_2	0x03204CC8 /* latest *//* old */
#define CFG_DDR_CFG	0xC3008000 /* old */
#define CFG_DDR_MODE	0x00040452  /* old */
#define CFG_DDR_CFG_2	0x04400000
#endif

#if defined(DDR_CAS_4)
/* CAS latency 4 */
#define CFG_DDR_TIMING_1	0x39377322 /* new 08302007*/
#define CFG_DDR_TIMING_2	0x03184CC8 /* new 08302007*/
#define CFG_DDR_CFG	0xC3000000 /* new 08302007*/
#define CFG_DDR_MODE	0x00060442   /* new 08302007*/
#define CFG_DDR_CFG_2	0x04400000
#endif

#if defined(DDR_CAS_3)
/* CAS latency 3 */
#define CFG_DDR_TIMING_1	0x39352322  /* new 10272007 for 1G */
//#define CFG_DDR_TIMING_1	0x39350322 /* new 08302007*/
#define CFG_DDR_TIMING_2	0x13104CC8 /* new 08302007*/
#define CFG_DDR_CFG	0xC3000000 /* new 08302007*/
#define CFG_DDR_MODE	0x000E0432   /* new 08302007*/
#define CFG_DDR_CFG_2	0x04401000
#endif
#endif

#undef CONFIG_CLOCKS_IN_MHZ


/*
 * Local Bus Definitions
 */

/*
 * Local Bus
 * Flash  0xff80_0000 - 0xffff_ffff      8M
 * EPLD  0xff00_0000 - 0xff00_ffff  64K
 */

#define CFG_EPLD_BASE		0xff000000
#define CFG_FLASH_BASE		0xff800000	/* start of FLASH 8M */

/*
 * FLASH on the Local Bus
 * One banks, 8M, using the CFI driver.
 * Boot from BR0/OR0 bank at 0xff80_0000
 *
 * BR0:
 *    BA (Base Address) = 0xff80_0000 = BR0[0:16] = 1111 1111 1000 0000 0
 *    XBA (Address Space eXtent) = BR0[17:18] = 00
 *    PS (Port Size) = 16 bits = BR0[19:20] = 10
 *    DECC = disable parity check = BR0[21:22] = 00
 *    WP = enable both read and write protect = BR0[23] = 0
 *    MSEL = use GPCM = BR0[24:26] = 000
 *    ATOM = not use atomic operate = BR0[28:29] = 00
 *    Valid = BR0[31] = 1
 *
 * 0      4      8     12    16     20    24     28
 * 1111 1111 1000 0000 0001 0000 0000 0001 = ff801001    BR0
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

#define CFG_BR0_PRELIM		0xff801001
#define CFG_OR0_PRELIM		0xff800f37

#define CFG_FLASH_BANKS_LIST	{CFG_FLASH_BASE}
#define CFG_MAX_FLASH_BANKS	1		/* number of banks */
#define CFG_MAX_FLASH_SECT	142		/* sectors per device */
#undef	CFG_FLASH_CHECKSUM
#define CFG_FLASH_ERASE_TOUT	60000	/* Flash Erase Timeout (ms) */
#define CFG_FLASH_WRITE_TOUT	500	/* Flash Write Timeout (ms) */

#define CFG_MONITOR_BASE    	TEXT_BASE	/* start of monitor */

#define CFG_FLASH_CFI_DRIVER
#define CFG_FLASH_CFI
#define CFG_FLASH_EMPTY_INFO
#define CFG_FLASH_PROTECTION

/*
 * EPLD on the Local Bus (LCD)
 * BR1/OR1 0xff00_0000 - 0xff00_ffff (64k)
 *
 * BR1:
 *    BA (Base Address) = 0xff00_0000 = BR1[0:16] = 1111 1111 0000 0000 0
 *    XBA (Address Space eXtent) = BR1[17:18] = 00
 *    PS (Port Size) = 16 bits = BR1[19:20] = 10
 *    DECC = disable parity check = BR1[21:22] = 00
 *    WP = enable both read and write protect = BR1[23] = 0
 *    MSEL = use GPCM = BR1[24:26] = 000
 *    ATOM = not use atomic operate = BR1[28:29] = 00
 *    Valid = BR1[31] = 1
 *
 * 0      4      8     12    16     20    24     28
 * 1111 1111 0000 0000 0001 0000 0000 0001 = ff001001    BR1
 *
 * OR1:
 *    Addr Mask = 64K = OR1[0:16] = 1111 1111 1111 1111 0
 *    Reserved OR1[17:18] = 00
 *    BCTLD = buffer control not assert = OR1[19] = 1
 *    CSNT = chip select /CS and /WE negated = OR1[20] = 0
 *    ACS = address to CS at same time as address line = OR1[21:22] = 00
 *    XACS = extra address setup = OR1[23] = 0
 *    SCY = 7 clock wait states = OR1[24:27] = 0100
 *    SETA (External Transfer Ack) = OR1[28] = 0
 *    TRLX = use relaxed timing = OR1[29] = 0
 *    EHTR (Extended hold time on read access) = OR1[30] = 0
 *    EAD = use external address latch delay = OR1[31] = 0
 *
 * 0      4      8     12    16     20    24     28
 * 1111 1111 1111 1111 0001 0000 0100 0000 = ffff1040    OR1
 */

#define CFG_BR1_PRELIM		0xff001001
#define CFG_OR1_PRELIM		0xffff0f17

/*
 * NAND on the Local Bus
 * BR2/OR2 0xff01_0000 - 0xff01_7fff (32k)
 *
 * BR2:
 *    BA (Base Address) = 0xff01_0000 = BR2[0:16] = 1111 1111 0000 0001 0
 *    XBA (Address Space eXtent) = BR2[17:18] = 00
 *    PS (Port Size) = 8 bits = BR2[19:20] = 01
 *    DECC = disable parity check = BR2[21:22] = 00
 *    WP = enable both read and write protect = BR2[23] = 0
 *    MSEL = use UPMA = BR2[24:26] = 100
 *    ATOM = not use atomic operate = BR2[28:29] = 00
 *    Valid = BR2[31] = 1
 *
 * 0    4    8   12   16   20   24   28
 * 1111 1111 0000 0001 0000 1000 1000 0001 = ff010881    BR2
 *
 * OR2:
 *    Addr Mask = 32K = OR2[0:16] = 1111 1111 1111 1111 1
 *    Reserved OR2[17:18] = 00
 *    BCTLD = buffer control not assert = OR2[19] = 1
 *    Reserved = OR2[20:22] = 000
 *    BI = no burst support = OR2[23] = 1
 *    Reserved = OR2[24:28] = 00000
 *    TRLX = normal timing = OR2[29] = 0
 *    EHTR = normal timing = OR2[30] = 0
 *    EAD = no additional bus clock cycle = OR2[31] = 0
 *
 * 0    4    8   12   16   20   24   28
 * 1111 1111 1111 1111 1001 0001 0000 0000 = ffff9100    OR2
 */

#define CFG_BR2_PRELIM		0xff010881
#define CFG_OR2_PRELIM		0xffff9100

#define CFG_NAND_BASE		0xff010000      /* 32k */

#define CFG_LBC_LCRR		0x00030008    /* LB clock ratio reg */
#define CFG_LBC_LBCR		0x00000000    /* LB config reg */

#define CONFIG_L1_INIT_RAM
#define CFG_INIT_RAM_LOCK 	1
#define CFG_INIT_RAM_ADDR	0xfee00000	/* Initial RAM address */
#define CFG_INIT_RAM_END    	0x4000	    /* End of used area in RAM */

#define POST_STOR_WORD  CFG_INIT_RAM_ADDR 

#define CFG_GBL_DATA_SIZE  	128	    /* num bytes initial data */
#define CFG_GBL_DATA_OFFSET	(CFG_INIT_RAM_END - CFG_GBL_DATA_SIZE)
#define CFG_INIT_SP_OFFSET	CFG_GBL_DATA_OFFSET

#if defined(CONFIG_UPGRADE) || defined(CONFIG_READONLY)
#define CFG_MONITOR_LEN	    	(512 * 1024) /* Reserve 512 kB for Mon */
#else
#define CFG_MONITOR_LEN	    	(448 * 1024) /* Reserve 512 kB for Mon */
#endif
#define CFG_MALLOC_LEN	    	(256 * 1024)	/* Reserved for malloc */

/* Serial Port */
#define CONFIG_CONS_INDEX     1
#undef	CONFIG_SERIAL_SOFTWARE_FIFO
#define CFG_NS16550
#define CFG_NS16550_SERIAL
#define CFG_NS16550_REG_SIZE    1
#define CFG_NS16550_CLK		get_bus_freq(0)

#define CFG_BAUDRATE_TABLE  \
	{300, 600, 1200, 2400, 4800, 9600, 19200, 38400,115200}

#define CFG_NS16550_COM1        (CFG_CCSRBAR+0x4500)
#define CFG_NS16550_COM2        (CFG_CCSRBAR+0x4600)

/* Use the HUSH parser */
#define CFG_HUSH_PARSER
#ifdef  CFG_HUSH_PARSER
#define CFG_PROMPT_HUSH_PS2 "> "
#endif

/* pass open firmware flat tree */
#undef CONFIG_OF_FLAT_TREE
#undef CONFIG_OF_BOARD_SETUP

#if 0
/* maximum size of the flat tree (8K) */
#define OF_FLAT_TREE_MAX_SIZE	8192

#define OF_CPU			"PowerPC,8548@0"
#define OF_SOC			"soc8548@e0000000"
#define OF_TBCLK		(bd->bi_busfreq / 8)
#define OF_STDOUT_PATH		"/soc8548@e0000000/serial@4600"
#endif 

/*
 * I2C
 */
#undef CONFIG_FSL_I2C		/* Use local modified FSL common I2C driver */
#define CONFIG_HARD_I2C		/* I2C with hardware support*/
#undef	CONFIG_SOFT_I2C			/* I2C bit-banged */
#define CONFIG_LAST_STAGE_INIT  /* init 2nd I2C controller */
#define CFG_I2C_SPEED		400000	/* I2C speed and slave address */
#define CFG_I2C_SLAVE		0x7F
#define CFG_I2C_NOPROBES        {0x69}	/* Don't probe these addrs */
#define CFG_I2C_OFFSET		0x3000
#define CFG_I2C2_OFFSET	0x3100
#define CFG_I2C_CTRL_1_SW_ADDR     0x70
#define CFG_I2C_CTRL_2_SW_ADDR     0x71
#define CFG_I2C_CTRL_2_IO_ADDR      0x22
#define CFG_I2C_CTRL_2_SFP_IO_ADDR      0x20
#define CFG_I2C_CTRL_1    0
#define CFG_I2C_CTRL_2    1
//#define CFG_I2C_DDR_CHAN    0x20    /* I2C controller 1, channel 5 PCA9548 */
/* SW1 */
#define CFG_I2C_LCD_POT_ADDR         0x2D /* channel 7 */
#define CFG_I2C_PCI_CLK_ADDR          0x69 /* channel 7 */
#define CFG_I2C_TEMP_1_ADDR           0x18 /* channel 6 */
#define CFG_I2C_TEMP_2_ADDR           0x19 /* channel 6 */
#define CFG_I2C_HW_MON_ADDR          0x29 /* channel 6 */
#define CFG_I2C_POE_CTLR_ADDR        0x2C /* channel 5 */
#define CFG_I2C_POE_EEPROM_ADDR   0x53 /* channel 5 */
#define CFG_I2C_MAIN_EEPROM_ADDR   0x52 /* channel 4 */
#define CFG_I2C_PS_EEPROM_ADDR   0x51 /* channel 1 & 2*/
#define CFG_I2C_SECURE_EEPROM_ADDR  0x72 /* channel 4 */
/* SW2 */
#define CFG_I2C_UPLINK_EEPROM_ADDR   0x57 /* channel 1 */

/*
 * main board EEPROM
 */
#define CFG_I2C_EEPROM_ADDR CFG_I2C_MAIN_EEPROM_ADDR
#define CFG_I2C_EEPROM_ADDR_LEN 1
#define CFG_EEPROM_PAGE_WRITE_BITS 3
#define CFG_EEPROM_PAGE_WRITE_DELAY_MS 10
#define CFG_EEPROM_MAC_OFFSET 56

/*
 * main board EEPROM platform ID
 */
#define CONFIG_BOARD_TYPES	1	/* support board types		*/

#define I2C_ID_JUNIPER_SUMATRA              0x091E  /* Java Prototype */
#define I2C_ID_JUNIPER_EX3200_24_T       0x091F  /* Java Espresso */
#define I2C_ID_JUNIPER_EX3200_24_P       0x0920  /* Java Espresso POE */
#define I2C_ID_JUNIPER_EX3200_48_T       0x0921  /* Java Espresso */
#define I2C_ID_JUNIPER_EX3200_48_P       0x0922  /* Java Espresso POE */
#define I2C_ID_JUNIPER_EX4200_24_F       0x0923  /* Java Latte SFP */
#define I2C_ID_JUNIPER_EX4200_24_T       0x0924  /* Java Latte */
#define I2C_ID_JUNIPER_EX4200_24_P       0x0925  /* Java Latte POE */
#define I2C_ID_JUNIPER_EX4200_48_T       0x0926  /* Java Latte */
#define I2C_ID_JUNIPER_EX4200_48_P       0x0927  /* Java Latte POE */
#define I2C_ID_JUNIPER_EX4210_48_P       0x09E9  /* EX4210 48-port POE+ */                                                           
#define I2C_ID_JUNIPER_EX4210_24_P       0x09EA  /* EX4210 24-port POE+ */                                                           

/*
 * General PCI
 * Addresses are mapped 1-1.
 */
#define CFG_PCI1_MEM_BASE	0x80000000
#define CFG_PCI1_MEM_PHYS	CFG_PCI1_MEM_BASE
#define CFG_PCI1_MEM_SIZE	0x40000000	/* 1G */
#define CFG_PCI1_IO_BASE	0x00000000
#define CFG_PCI1_IO_PHYS	0xe2000000
#define CFG_PCI1_IO_SIZE	0x00100000	/* 1M */

/*
 * PCI Express
 */
#define CONFIG_PCIE   		1
#define CFG_PCIE1_MEM_BASE 0xc0000000
#define CFG_PCIE1_MEM_PHYS	CFG_PCIE1_MEM_BASE
#define CFG_PCIE1_MEM_SIZE	0x10000000	/* 256M */
#define CFG_PCIE1_OFFSET		0xa000

#define CFG_PCIE2_MEM_BASE 0xd0000000
#define CFG_PCIE2_MEM_PHYS	CFG_PCIE2_MEM_BASE
#define CFG_PCIE2_MEM_SIZE	0x10000000	/* 256M */
#define CFG_PCIE2_OFFSET		0x9000

#define CFG_PCIE3_MEM_BASE 0xe0000000
#define CFG_PCIE3_MEM_PHYS	CFG_PCIE3_MEM_BASE
#define CFG_PCIE3_MEM_SIZE	0x10000000	/* 256M */
#define CFG_PCIE3_OFFSET		0xb000

#if defined(CONFIG_PCI)

#define CONFIG_NET_MULTI
#define CONFIG_PCI_PNP	               	/* do pci plug-and-play */
#undef CONFIG_85XX_PCI2

#undef CONFIG_EEPRO100
#undef CONFIG_TULIP

#undef CONFIG_PCI_SCAN_SHOW		/* show pci devices on startup */
#define CFG_PCI_SUBSYS_VENDORID 0x1957  /* Freescale */

#endif	/* CONFIG_PCI */


//#if defined(CONFIG_TSEC_ENET)

#ifndef CONFIG_NET_MULTI
#define CONFIG_NET_MULTI 	1
#endif

#define CONFIG_MII		1	/* MII PHY management */
#define CONFIG_MPC85XX_TSEC1	1
#define CONFIG_MPC85XX_TSEC1_NAME	"eTSEC0"
#define CONFIG_MPC85XX_TSEC3	1
#define CONFIG_MPC85XX_TSEC3_NAME	"eTSEC2"
#undef CONFIG_MPC85XX_FEC

#define TSEC1_PHY_ADDR		0
#define TSEC2_PHY_ADDR		1
#define TSEC3_PHY_ADDR		2
#define TSEC4_PHY_ADDR		3

#define TSEC1_PHYIDX		0
#define TSEC2_PHYIDX		1
#define TSEC3_PHYIDX		2
#define TSEC4_PHYIDX		3

/* Options are: eTSEC[0-3] */
#define CONFIG_ETHPRIME		"eTSEC0"

//#endif	/* CONFIG_TSEC_ENET */

/*
 * USB configuration
 */
#define CONFIG_USB_EHCI		1
#define CONFIG_USB_STORAGE	1
#define CONFIG_DOS_PARTITION
#define CONFIG_USB_SHOW_NO_PROGRESS

#define USB_OHCI_VEND_ID 		0x1131
#define USB_OHCI_DEV_ID	 	0x1561
#define USB_EHCI_DEV_ID	 	0x1562 /* EHCI not implemented */

/* NAND test read/write blks/pattern definitions */
#define NAND_TEST_DEV_ID            0
#define NAND_TEST_START_BLK         2
#define NAND_TEST_NUM_BLK           4
#define NAND_READ_SAVE_ADDR         0x1000000
#define NAND_WRITE_PATTERN_ADDR     0x1001000
#define NAND_READ_PATTERN_ADDR      0x2000000
#define NAND_WRITE_PATTERN          0xFFFFFFFF
#define NAND_READ_WRITE_SIZE        0x800   /* 4 blk * 512 Bytes */

/*
 * LCD + LED
 */
#define CONFIG_SHOW_ACTIVITY

/*
 * Environment
 */
#define CFG_ENV_IS_IN_FLASH	1
#if defined(CONFIG_UPGRADE) || defined(CONFIG_READONLY)
#define CFG_ENV_ADDR		(CFG_FLASH_BASE)
#define CFG_ENV_SECT_SIZE	0x2000	/* 8K(1x8K sector) for env */
#define CFG_OPQ_ENV_ADDR        (CFG_FLASH_BASE + 0x2000) /* for RE nv env */
#define CFG_OPQ_ENV_SECT_SIZE   0x2000  /* 8K(1x8K sector) for RE nv env */
#define CFG_UPGRADE_ADDR		(CFG_FLASH_BASE + 0XE000)
#define CFG_UPGRADE_SECT_SIZE	0x2000	/* 8K(1x8K sector) for upgrade */
#else
#define CFG_ENV_ADDR		(CFG_MONITOR_BASE + 0x70000)
#define CFG_ENV_SECT_SIZE	0x4000	/* 16K(2x8K sector) for env */
#endif
#define CFG_ENV_SIZE		0x2000

#define CONFIG_LOADS_ECHO	1	/* echo on for serial download */
#define CFG_LOADS_BAUD_CHANGE	1	/* allow baudrate change */
#define CONFIG_RESET_PHY_R      1       /* reset_phy() */

#if defined(CONFIG_PCI)
#define  CONFIG_COMMANDS_TEMP	(CONFIG_CMD_DFL \
				| CFG_CMD_PCI \
				| CFG_CMD_PING \
				| CFG_CMD_I2C \
				| CFG_CMD_MII \
				| CFG_CMD_USB \
	                     | CFG_CMD_DIAG \
				| CFG_CMD_CACHE \
				| CFG_CMD_ELF \
				| CFG_CMD_FAT \
				| CFG_CMD_EEPROM \
				| CFG_CMD_BSP)
#else
#define  CONFIG_COMMANDS_TEMP	(CONFIG_CMD_DFL \
				| CFG_CMD_PING \
				| CFG_CMD_I2C \
				| CFG_CMD_MII \
				| CFG_CMD_USB \
	                     | CFG_CMD_DIAG \
				| CFG_CMD_CACHE \
				| CFG_CMD_ELF \
				| CFG_CMD_FAT \
				| CFG_CMD_EEPROM \
				| CFG_CMD_BSP)
#endif

#if defined(CONFIG_NAND_DIRECT)
#define CONFIG_COMMANDS (CONFIG_COMMANDS_TEMP | CFG_CMD_NAND)
#else
#define CONFIG_COMMANDS (CONFIG_COMMANDS_TEMP)
#endif
/*  *  *  *  *  * Power On Self Test support * * * * *  */
#define CONFIG_POST ( CFG_POST_MEMORY  \
					| CFG_POST_MEMORY_RAM  \
                                   | CFG_POST_WATCHDOG \
					| CFG_POST_LOG \
					| CFG_POST_UART \
					| CFG_POST_I2C  \
					| CFG_POST_CPU  \
					| CFG_POST_USB  \
					| CFG_POST_PCI \
					| CFG_POST_SUMATRA \
					| CFG_POST_ETHER)
                       

#ifdef CONFIG_POST
#define CFG_CMD_POST_DIAG CFG_CMD_DIAG
#else
#define CFG_CMD_POST_DIAG       0
#endif

#include <cmd_confdefs.h>

#undef CONFIG_WATCHDOG			/* watchdog disabled */
#define CONFIG_EPLD_WATCHDOG		/* EPLD watchdog */

/*
 * PIC 
 */
 /* MPC8544 12 External Interrupts 0x1200 - 0x120B */
#define IRQ0_INT_VEC            0x1200
#define IRQ1_INT_VEC            0x1201
#define IRQ2_INT_VEC            0x1202
#define IRQ3_INT_VEC            0x1203
#define IRQ4_INT_VEC            0x1204
#define IRQ5_INT_VEC            0x1205
#define IRQ6_INT_VEC            0x1206
#define IRQ7_INT_VEC            0x1207
#define IRQ8_INT_VEC            0x1208
#define IRQ9_INT_VEC            0x1209
#define IRQ10_INT_VEC            0x120A
#define IRQ11_INT_VEC            0x120B

/*
 * Ports 
 */
#define NUM_EXT_PORTS        50

/*
 * Miscellaneous configurable options
 */
#define CFG_LONGHELP			/* undef to save memory	*/
#define CFG_LOAD_ADDR	0x2000000	/* default load address */
#define CFG_PROMPT	"=> "		/* Monitor Command Prompt */
#if (CONFIG_COMMANDS & CFG_CMD_KGDB)
#define CFG_CBSIZE	1024		/* Console I/O Buffer Size */
#else
#define CFG_CBSIZE	256		/* Console I/O Buffer Size */
#endif
#define CFG_PBSIZE (CFG_CBSIZE+sizeof(CFG_PROMPT)+16) /* Print Buffer Size */
#define CFG_MAXARGS	16		/* max number of command args */
#define CFG_BARGSIZE	CFG_CBSIZE	/* Boot Argument Buffer Size */
#define CFG_HZ		1000		/* decrementer freq: 1ms ticks */
#define CFG_CONSOLE_INFO_QUIET	1       /* don't print console @ startup*/

/*
 * For booting Linux, the board info and command line data
 * have to be in the first 8 MB of memory, since this is
 * the maximum mapped by the Linux kernel during initialization.
 */
#define CFG_BOOTMAPSZ	(8 << 20) 	/* Initial Memory map for Linux*/

/* Cache Configuration */
#define CFG_DCACHE_SIZE	32768
#define CFG_CACHELINE_SIZE	32
#if (CONFIG_COMMANDS & CFG_CMD_KGDB)
#define CFG_CACHELINE_SHIFT	5	/*log base 2 of the above value*/
#endif

/*
 * Internal Definitions
 *
 * Boot Flags
 */
#define BOOTFLAG_COLD	0x01		/* Normal Power-On: Boot from FLASH */
#define BOOTFLAG_WARM	0x02		/* Software reboot */

#if (CONFIG_COMMANDS & CFG_CMD_KGDB)
#define CONFIG_KGDB_BAUDRATE	230400	/* speed to run kgdb serial port */
#define CONFIG_KGDB_SER_INDEX	1	/* which serial port to use */
#endif


/*
 * Environment Configuration
 */

/* The mac addresses for all ethernet interface */
//#if defined(CONFIG_TSEC_ENET)
#define CONFIG_ETHADDR   00:E0:0C:00:00:FD
#define CONFIG_HAS_ETH1
#define CONFIG_ETH1ADDR  00:E0:0C:00:01:FD
//#endif

#undef CONFIG_HOSTNAME
#undef CONFIG_ROOTPATH
#undef CONFIG_BOOTFILE 

#undef CONFIG_IPADDR
#undef CONFIG_NETMASK
#undef CONFIG_GATEWAYIP
#undef CONFIG_SERVERIP

#define CONFIG_LOADADDR  0x100000   /*default location for tftp and bootm*/

#define CONFIG_BOOTDELAY 1       /* -1 disables auto-boot */
#undef  CONFIG_BOOTARGS           /* the boot command will set bootargs*/

#define CONFIG_BAUDRATE	9600

#define	CONFIG_EXTRA_ENV_SETTINGS				     \
   "hw.uart.console=mm:0xfef04500\0"

#undef CONFIG_NFSBOOTCOMMAND	                                        \

#undef CONFIG_RAMBOOTCOMMAND	                                        \

#if defined(CONFIG_UPGRADE)
#define CONFIG_BOOTCOMMAND  \
   "cp.b 0xffe00000 $loadaddr 0x40000; bootelf $loadaddr"
#else
#define CONFIG_BOOTCOMMAND  \
   "cp.b 0xfff00000 $loadaddr 0x40000; bootelf $loadaddr"
#endif

#define CONFIG_AUTOBOOT_KEYED		/* use key strings to stop autoboot */
/* 
 * Using ctrl-c to stop booting and entering u-boot command mode.  
 * Space is too easy to break booting by accident.  Ctrl-c is common
 * but not too easy to interrupt the booting by accident.
 */
#define CONFIG_AUTOBOOT_STOP_STR	"\x03"
#define CONFIG_SILENT_CONSOLE	0

/* Local bus UPMA */
#define CFG_MAMR	(CFG_CCSRBAR | 0x5070)      /* mode register */
#define CFG_MAR		(CFG_CCSRBAR | 0x5068)      /* memory address register */
#define CFG_MDR		(CFG_CCSRBAR | 0x5088)      /* data register */

#define NAND_MAX_CHIPS		1
#define CFG_MAX_NAND_DEVICE	NAND_MAX_CHIPS

#define	CFG_VERSION_MAJOR	1
#define	CFG_VERSION_MINOR	0
#define	CFG_VERSION_PATCH	0

#endif	/* __CONFIG_H */
