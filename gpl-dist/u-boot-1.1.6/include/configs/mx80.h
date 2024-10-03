/*
 * Copyright (c) 2009-2012, Juniper Networks, Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

/*
 * MX80  board configuration file
 *
 */
#ifndef __CONFIG_H
#define __CONFIG_H

/* High Level Configuration Options */
#define CONFIG_BOOKE		1	/* BOOKE */
#define CONFIG_E500		1	/* BOOKE e500 family */
#define CONFIG_MPC85xx		1	/* MPC8540/60/55/41/48 */
#define CONFIG_MPC8572		1
#define CONFIG_MULTICORE 1
#define CONFIG_MX80		1
#define CONFIG_PCIE		1	/* new pcie driver to compile */

#define CONFIG_ENV_OVERWRITE
#define CONFIG_SPD_EEPROM		/* Use SPD EEPROM for DDR setup */
#undef CONFIG_DDR_DLL
#undef CONFIG_DDR_2T_TIMING		/* Sets the 2T timing bit */

//#define CONFIG_DDR_ECC			/* only for ECC DDR module */
#define CONFIG_ECC_INIT_VIA_DDRCONTROLLER	/* DDR controller or DMA? */
#define CONFIG_MEM_INIT_VALUE		0xDeadBeef
#define SPD_EEPROM_ADDRESS	0x50		/* DDRC1 DIMM */
#define SPD_EEPROM_ADDRESS1	SPD_EEPROM_ADDRESS

#define CONFIG_NUM_DDR_CONTROLLERS	2
#define CONFIG_NUM_DIMMS_PER_DDR        1

#ifdef	CONFIG_NUM_DDR_CONTROLLERS
#define CONFIG_MEM_INIT_VALUE2		0xcafebabe
#define SPD_EEPROM_ADDRESS2	0x50		/* DDRC2 DIMM */
#endif

#define CONFIG_DDR_ECC_CMD
#define CONFIG_HW_WATCHDOG	1

/*
 * When initializing flash, if we cannot find the manufacturer ID,
 * assume this is the AMD flash associated with the CDS board.
 * This allows booting from a promjet.
 */
#define CONFIG_ASSUME_AMD_FLASH

#define MPC85xx_DDR_SDRAM_CLK_CNTL	/* 85xx has clock control reg */

#ifndef __ASSEMBLY__
extern unsigned long calculate_board_sys_clk(unsigned long dummy);
extern unsigned long calculate_board_ddr_clk(unsigned long dummy);
extern unsigned long get_board_sys_clk(unsigned long dummy);
extern unsigned long get_board_ddr_clk(unsigned long dummy);
#endif

#define CONFIG_SYS_CLK_FREQ	66666666   /* 66666666 : sysclk for MPC85xx */
#define CONFIG_DDR_CLK_FREQ	66666666  /* ddrclk for MPC85xx */
/*
 * These can be toggled for performance analysis, otherwise use default.
 */
#define CONFIG_L2_CACHE			/* toggle L2 cache */
#define CONFIG_BTB			/* toggle branch predition */
#define CONFIG_ADDR_STREAMING		/* toggle addr streaming */
#undef CONFIG_CLEAR_LAW0		/* Clear LAW0 in cpu_init_r */

#define CONFIG_BOARD_EARLY_INIT_R	/* Call board_pre_init */

#define CFG_MEMTEST_START	0x00000000	/* memtest works on */
#define CFG_MEMTEST_END		0x80000000
#define CFG_ALT_MEMTEST
#define CFG_DRAM_TEST
#define CONFIG_PANIC_HANG	/* do not reset board on panic */
#define CONFIG_LAST_STAGE_INIT  /* set hw.board.type as part of last stage init */
/*
 * Base addresses -- Note these are effective addresses where the
 * actual resources get mapped (not physical addresses)
 */
#define CFG_CCSRBAR_DEFAULT	0xff700000	/* CCSRBAR Default */
#define CFG_CCSRBAR		0xe0000000	/* relocated CCSRBAR */
#define CFG_IMMR		CFG_CCSRBAR	/* PQII uses CFG_IMMR */

#define CFG_UPGRADE_BOOT_IMG_ADDR  	0xfff80000
#define CFG_UPGRADE_LOADER_ADDR 	0xffe00000


#define CFG_PCIE3_ADDR		(CFG_CCSRBAR+0x8000)
#define CFG_PCIE2_ADDR		(CFG_CCSRBAR+0x9000)
#define CFG_PCIE1_ADDR		(CFG_CCSRBAR+0xa000)

/*
 * DDR Setup
 */
#define CFG_DDR_SDRAM_BASE	0x00000000	/* DDR is system memory*/
#define CFG_SDRAM_BASE		CFG_DDR_SDRAM_BASE
#define CONFIG_VERY_BIG_RAM

#define CFG_SDRAM_SIZE	512		/* DDR is 512MB */

#define CFG_DDR_CS0_BNDS	0x0000001F
#define CFG_DDR_CS0_CONFIG	0x80010202	/* Enable, no interleaving */
#define CFG_DDR_EXT_REFRESH	0x00020000
#define CFG_DDR_TIMING_0	0x00260802
#define CFG_DDR_TIMING_1	0x626b2634
#define CFG_DDR_TIMING_2	0x062874cf
#define CFG_DDR_MODE_1		0x00440462
#define CFG_DDR_MODE_2		0x00000000
#define CFG_DDR_INTERVAL	0x0c300100
#define CFG_DDR_DATA_INIT	0xdeadbeef
#define CFG_DDR_CLK_CTRL	0x00800000
#define CFG_DDR_OCD_CTRL	0x00000000
#define CFG_DDR_OCD_STATUS	0x00000000
#define CFG_DDR_CONTROL		0xc3000008	/* Type = DDR2 */
#define CFG_DDR_CONTROL2	0x24400000

#define CFG_DDR_ERR_INT_EN	0x0000000d
#define CFG_DDR_ERR_DIS		0x00000000
#define CFG_DDR_SBE		0x00010000
 /* Not used in fixed_sdram function */
#define CFG_DDR_MODE		0x00000022
#define CFG_DDR_CS1_BNDS	0x00000000
#define CFG_DDR_CS2_BNDS	0x00000FFF	/* Not done */
#define CFG_DDR_CS3_BNDS	0x00000FFF	/* Not done */
#define CFG_DDR_CS4_BNDS	0x00000FFF	/* Not done */
#define CFG_DDR_CS5_BNDS	0x00000FFF	/* Not done */

/*
 * Make sure required options are set
 */
#ifndef CONFIG_SPD_EEPROM
#error ("CONFIG_SPD_EEPROM is required")
#endif

#undef CONFIG_CLOCKS_IN_MHZ

/*
 * Memory map
 *
 * 0x0000_0000	0x7fff_ffff	DDR			2G Cacheable
 *
 * 0x8000_0000	0xbfff_ffff	PCI Express Mem		1G non-cacheable
 *
 * 0xc000_0000	0xdfff_ffff	PCI			512M non-cacheable
 *
 * 0xe100_0000	0xe3ff_ffff	PCI IO range		4M non-cacheable
 *
 * Localbus cacheable (TBD)
 * 0xXXXX_XXXX	0xXXXX_XXXX	SRAM			YZ M Cacheable
 *
 * Localbus non-cacheable
 *
 * 0xe000_0000	0xe80f_ffff	Promjet/free		128M non-cacheable
 * 0xe800_0000	0xefff_ffff	FLASH			128M non-cacheable
 * 0xf000_0000  0xf00f_ffff	NAND FLASH		1M non-cacheable(after relocate)
 * 0xffc0_0000  0xffc2_ffff	PCIE-IO			256K * 3
 * 0xffd0_0000	0xffd0_3fff	L1 for stack		16K Cacheable TLB0
 * 0xffdf_0000	0xffdf_7fff	PIXIS			32K non-cacheable TLB0
 * 0xffe0_0000	0xffef_ffff	CCSR			1M non-cacheable
 * 0xfff0_0000  0xfff3_ffff	NAND FLASH		256K non-cacheable(before relocate)
 */

/*
 * Local Bus Definitions & Clock Setup
 */
#define CFG_LCRR		(LCRR_DBYP | LCRR_CLKDIV_16)
#define CFG_FLASH_BASE		0xff800000	/* start of FLASH 128M */
#define CFG_FLASH_BANKS_LIST	{CFG_FLASH_BASE}

#define CFG_MAX_FLASH_BANKS	1		/* number of banks */
#define CFG_MAX_FLASH_SECT	1024		/* sectors per device */
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
/******************************************************************************/
/*
 * Local Bus Definitions
 */

/* NOR flash config */
/*
 * FLASH on the Local Bus
 * One bank, 8-bit, 8MB.
 * BR0/OR0 : 0xff80_0000 - 0xffff_ffff (8M)
 *
 * BR0:
 *    BA (Base Address) = 0xff80_0000    = BRx[0:16] = 1111 1111 1000 0000 0
 *    Reserved                           = BRx[17:18] = 00
 *    PS (Port Size) = 8 bits            = BRx[19:20] = 01
 *    DECC = disable parity check        = BRx[21:22] = 00
 *    WP = enable both RD and WR access  = BRx[23] = 0
 *    MSEL = use GPCM                    = BRx[24:26] = 000
 *    ATOM = not use atomic operate      = BRx[28:29] = 00
 *    Valid                              = BRx[31] = 1
 *
 * 0    4    8    12   16   20   24   28
 * 1111 1111 1000 0000 0000 1000 0000 0001 = ff800801    BR0
 *
 * OR0:
 *    Addr Mask = 8M                           = ORx[0:16] = 1111 1111 1000 0000 0
 *    Reserved                                 = ORx[17:18] = 00
 *    BCTLD = buffer control not assert        = ORx[19] = 1
 *    CSNT = chip select /CS and /WE negated   = ORx[20] = 0
 *    ACS = address to CS at same time as addr line = ORx[21:22] = 00
 *    XACS = extra address setup               = ORx[23] = 0
 *    SCY = Cycle length in bus clock          = ORx[24:27] = 0100
 *    SETA = External address termination      = ORx[28] = 0
 *    TRLX = use relaxed timing                = ORx[29] = 0
 *    EHTR (Extended hold time on read access) = ORx[30] = 0
 *    EAD = use external address latch delay   = ORx[31] = 0
 *
 * 0    4    8    12   16   20   24   28
 * 1111 1111 1000 0000 0001 0000 0100 0000 = ff801040    OR0
 */
#define CFG_FLASH_BR_PRELIM	0xff800801
#define CFG_FLASH_OR_PRELIM	0xff806e65
#define CFG_BR0_PRELIM		CFG_FLASH_BR_PRELIM /* NOR Base address */
#define CFG_OR0_PRELIM		CFG_FLASH_OR_PRELIM /* NOR Options */

/*
 * TBB CPLD
 */
/*
 * TBB RE CPLS on the Local Bus
 * Size: 64KB.
 * BR3/OR3 : 0xff70_0000 - 0xff70_ffff (64k)
 *
 * BR3:
 *    BA (Base Address) = 0xff70_0000    = BRx[0:16] = 1111 1111 0111 0000 0
 *    Reserved                           = BRx[17:18] = 00
 *    PS (Port Size) = 16 bits           = BRx[19:20] = 10
 *    DECC = disable parity check        = BRx[21:22] = 00
 *    WP = enable both RD and WR access  = BRx[23] = 0
 *    MSEL = use GPCM                    = BRx[24:26] = 000
 *    ATOM = not use atomic operate      = BRx[28:29] = 00
 *    Valid                              = BRx[31] = 1
 *
 * 0    4    8    12   16   20   24   28
 * 1111 1111 0111 0000 0001 0000 0000 0001 = ff708001    BR3
 *
 * OR3:
 *    Addr Mask = 64K                          = ORx[0:16] = 1111 1111 1111 1111 0
 *    Reserved                                 = ORx[17:18] = 00
 *    BCTLD = buffer control not assert        = ORx[19] = 1
 *    CSNT = chip select /CS and /WE negated   = ORx[20] = 0
 *    ACS = address to CS at same time as addr line = ORx[21:22] = 00
 *    XACS = extra address setup               = ORx[23] = 0
 *    SCY = Cycle length in bus clock          = ORx[24:27] = 0100
 *    SETA = External address termination      = ORx[28] = 0
 *    TRLX = use relaxed timing                = ORx[29] = 0
 *    EHTR (Extended hold time on read access) = ORx[30] = 0
 *    EAD = use external address latch delay   = ORx[31] = 0
 *
 * 0    4    8    12   16   20   24   28
 * 1111 1111 1111 1111 0001 0000 0100 0000 = ffff1040    OR3
 */
#define CFG_TBBCPLD_BASE	0xFF700000
#define CFG_TBBCPLD_SIZE	0x00010000	/* 64KB */

#define CFG_BR3_PRELIM		0xFF700801
#define CFG_OR3_PRELIM		0xFFFF1040
/*
 * External UART
 */
/*
 * External UART 
 * Size: 64KB.
 * BR4/OR4 : 0xff60_0000 - 0xff60_ffff (64k)
 *
 * BR4:
 *    BA (Base Address) = 0xff60_0000    = BRx[0:16] = 1111 1111 0110 0000 0
 *    Reserved                           = BRx[17:18] = 00
 *    PS (Port Size) = 16 bits           = BRx[19:20] = 10
 *    DECC = disable parity check        = BRx[21:22] = 00
 *    WP = enable both RD and WR access  = BRx[23] = 0
 *    MSEL = use GPCM                    = BRx[24:26] = 000
 *    ATOM = not use atomic operate      = BRx[28:29] = 00
 *    Valid                              = BRx[31] = 1
 *
 * 0    4    8    12   16   20   24   28
 * 1111 1111 0110 0000 0001 0000 0000 0001 = ff601001    BR4
 *
 * OR4:
 *    Addr Mask = 64K                          = ORx[0:16] = 1111 1111 1111 1111 0
 *    Reserved                                 = ORx[17:18] = 00
 *    BCTLD = buffer control not assert        = ORx[19] = 1
 *    CSNT = chip select /CS and /WE negated   = ORx[20] = 0
 *    ACS = address to CS at same time as addr line = ORx[21:22] = 00
 *    XACS = extra address setup               = ORx[23] = 0
 *    SCY = Cycle length in bus clock          = ORx[24:27] = 0100
 *    SETA = External address termination      = ORx[28] = 0
 *    TRLX = use relaxed timing                = ORx[29] = 0
 *    EHTR (Extended hold time on read access) = ORx[30] = 0
 *    EAD = use external address latch delay   = ORx[31] = 0
 *
 * 0    4    8    12   16   20   24   28
 * 1111 1111 1111 1111 0001 0000 0100 0000 = ffff1040    OR4
 */
#define CFG_EXTUARTA_BASE	0xFF600000
#define CFG_EXTUARTB_BASE	0xFF500000
#define CFG_EXTUART_SIZE	0x00010000	/* 64KB */

#define CFG_BR4_PRELIM		0xFF600801
#define CFG_OR4_PRELIM		0xFFFF0835

#define CFG_BR5_PRELIM		0xFF500801
#define CFG_OR5_PRELIM		0xFFFF0835


/******************************************************************************/

#undef CONFIG_FSL_PIXIS	   /* Do not use boards/freescale/common PIXIS code 
				for MPC8572DS, it doesn't work */
/* define to use L1 as initial stack */
#define CONFIG_L1_INIT_RAM	1
#define CFG_INIT_L1_LOCK	1
#define CFG_INIT_L1_ADDR	0xe4010000	/* Initial L1 address */
#define CFG_INIT_L1_END		0x00004000	/* End of used area in RAM */

#ifdef CFG_INIT_L1_LOCK
#define CFG_INIT_RAM_LOCK	1		/* Support legacy 85xx code */
#endif

#define POST_STOR_WORD		CFG_INIT_RAM_ADDR

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

/* pass open firmware flat tree */
#undef CONFIG_OF_FLAT_TREE	/* SAS : Changed from std mpc8572ds config */
#undef CONFIG_OF_BOARD_SETUP	/* SAS : Changed from std mpc8572ds config */

/* maximum size of the flat tree (8K) */
#define OF_FLAT_TREE_MAX_SIZE	8192

#define OF_CPU			"PowerPC,8572@0"
#define OF_SOC			"soc8572@ffe00000"
#define OF_TBCLK		(bd->bi_busfreq / 8)
#define OF_STDOUT_PATH		"/soc8572@ffe00000/serial@4500"

/* I2C */
#define CONFIG_FSL_I2C		/* Use FSL common I2C driver */
#define CONFIG_HARD_I2C		/* I2C with hardware support */
#undef	CONFIG_SOFT_I2C		/* I2C bit-banged */
#define CFG_I2C_SPEED		400000	/* I2C speed and slave address */
#define CFG_I2C_EEPROM_ADDR	0x51 /* IDEEPROM shared between RE & PFE */
#define CFG_I2C_EEPROM_ADDR_LEN 1   /* SAS ADDED: JUNOS requires this set. */
#define CFG_EEPROM_MAC_OFFSET   56
#define CFG_I2C_SLAVE		0x7F
#define CFG_I2C_NOPROBES	{0x69}	/* Don't probe these addrs */
#define CFG_I2C_OFFSET		0x3000	/* I2C Cotroller #1 */

/* controller 1, Slot 1, tgtid 1, Base address c000 */
#define CFG_PCIE1_MEM_BASE	0xc0000000
#define CFG_PCIE1_MEM_PHYS	CFG_PCIE1_MEM_BASE
#define CFG_PCIE1_MEM_SIZE	0x10000000	/* 256M */

#ifndef CONFIG_NET_MULTI
#define CONFIG_NET_MULTI	1
#endif

#define CONFIG_TSEC_TBI
#define CFG_TBIPA_VALUE		0x10
#define CONFIG_SGMII_RISER
#define TSEC1_SGMII_PHY_ADDR_OFFSET 0x1c
#define CONFIG_MII		1	/* MII PHY management */
#define CONFIG_MII_DEFAULT_TSEC	1	/* Allow unregistered phys */

#define CONFIG_MPC85XX_TSEC1		1
#define CONFIG_MPC85XX_TSEC1_NAME	"me0"
#define CONFIG_MPC85XX_TSEC2		1    
#define CONFIG_MPC85XX_TSEC2_NAME	"em0"
#define CONFIG_MPC85XX_TSEC3		1   
#define CONFIG_MPC85XX_TSEC3_NAME	"fxp0"
#define CONFIG_MPC85XX_TSEC4		1   
#define CONFIG_MPC85XX_TSEC4_NAME	"em1"

#define TSEC1_PHY_ADDR		2	/* SAS CHANGE FAKE WRONG ADDR */
#define TSEC2_PHY_ADDR		1
#define TSEC3_PHY_ADDR		0	/* MX80  TSEC3 PHY address 
                                         * Don't add any offset to it
                                         */
#define TSEC4_PHY_ADDR		3

#define TSEC1_FLAGS            (TSEC_GIGABIT)
#define TSEC2_FLAGS            (TSEC_GIGABIT | TSEC_REDUCED)
#define TSEC3_FLAGS            (TSEC_GIGABIT | TSEC_REDUCED)
#define TSEC4_FLAGS            (TSEC_GIGABIT | TSEC_REDUCED)

#define TSEC1_PHYIDX		0
#define TSEC2_PHYIDX		1
#define TSEC3_PHYIDX		0    /* TSEC1 MDIO interface controls TSEC3 PHY */
#define TSEC4_PHYIDX		0

#define CONFIG_ETHPRIME		"fxp0"

#define CONFIG_PHY_GIGE		1	/* Include GbE speed/duplex detection */

/*
 * Environment
 */
#if !defined(CFG_RAMBOOT)
#define CONFIG_CMD_ENV
#define CFG_ENV_IS_IN_FLASH     1
#define CFG_ENV_ADDR            0xFFD00000 
#define CFG_ENV_SECT_SIZE       0x10000 /* 64k (one sector) for env */
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
#if defined(CONFIG_PCI)
#define  CONFIG_COMMANDS	(CONFIG_CMD_DFL \
				| CFG_CMD_PCI \
				| CFG_CMD_PING \
				| CFG_CMD_I2C \
				| CFG_CMD_DIAG  \
				| CFG_CMD_MII \
				| CFG_CMD_USB 	\
				| CFG_CMD_CACHE \
				| CFG_CMD_ELF \
				| CFG_CMD_EEPROM \
				| CFG_CMD_BSP)
#else
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
#endif

#define CONFIG_POST		(CFG_POST_MEMORY \
				| CFG_POST_MEMORY_RAM \
				| CFG_POST_CPU \
				| CFG_POST_WATCHDOG \
				| CFG_POST_USB \
				| CFG_POST_UART \
				| CFG_POST_ETHER \
				| CFG_POST_PCI \
				| CFG_POST_CPLD \
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

/* The mac addresses for all ethernet interface */
#if defined(CONFIG_TSEC_ENET)
#define CONFIG_HAS_ETH0
#define CONFIG_ETHADDR	04:00:00:00:00:0A
#define CONFIG_HAS_ETH1
#define CONFIG_ETH1ADDR	04:00:00:00:00:0B
#define CONFIG_HAS_ETH2
#define CONFIG_ETH2ADDR	04:00:00:00:00:0C
#define CONFIG_HAS_ETH3
#define CONFIG_ETH3ADDR	04:00:00:00:00:0D
#endif

#undef CONFIG_HOSTNAME
#define CONFIG_ROOTPATH		/opt/nfsroot
#undef CONFIG_BOOTFILE

#define CONFIG_IPADDR		192.168.35.88  /* BACCHUS router address */
#define CONFIG_GATEWAYIP	192.168.35.254
#define CONFIG_NETMASK		255.255.255.0
#define CONFIG_SERVERIP		172.17.28.28

/* default location for tftp and bootm */
#define CONFIG_LOADADDR		1000000

#define CONFIG_BOOTDELAY 1	/* -1 disables auto-boot SAS CHANGE: 1 sec tooo long */
#undef  CONFIG_BOOTARGS		/* the boot command will set bootargs */
#define CONFIG_AUTOBOOT_KEYED           /* use key strings to stop autoboot */
/* 
 *  * Using ctrl-c to stop booting and entering u-boot command mode.  
 *   * Space is too easy to break booting by accident.  Ctrl-c is common
 *    * but not too easy to interrupt the booting by accident.
 *     */
#define CONFIG_AUTOBOOT_STOP_STR        "\x03"

#define CONFIG_BAUDRATE	9600

#define UART1_BASE		(CFG_CCSRBAR + 0x4500)

/* 
 * Remember to change uart base address if CCSR BAR is changed
 */
#define	CONFIG_EXTRA_ENV_SETTINGS				\
 "hw.uart.console=mm:0xf7f04500\0"				\
 "hw.uart.dbgport=mm:0xf7f04500\0"				\
 "netdev=fxp0\0"						\
 "ethact=fxp0\0"						\
 "ethprime=fxp0\0"						\
 "consoledev=ttyS0\0"				\
 "memctl_intlv_ctl=2\0"                  \

 /*
 * USB configuration
 */
#define CONFIG_USB_EHCI		1
#define CONFIG_USB_STORAGE	1
#define CONFIG_DOS_PARTITION
#define CONFIG_USB_SHOW_NO_PROGRESS
#define CONFIG_USB_PCIE		1   /*USB device is connected to the PCIE interface*/

#define CONFIG_BOOTCOMMAND  \
   "cp.b 0xffe00000 $loadaddr 0x100000; bootelf $loadaddr"

#define I2C_ID_MX80_48T                  0x098E  /* MX80  TBB, with fixed */
#define I2C_ID_MX80                      0x098F  /* MX80  TBB, with MICs */
#define I2C_ID_MX80_T                    0x0558  /* MX80 with new OCXO */
#define I2C_ID_MX5_T                     0x0556
#define I2C_ID_MX10_T                    0x0555
#define I2C_ID_MX40_T                    0x0554
#define I2C_ID_MX80_P                    0x055D  /* MX80 with PTP clk */


#if 0   /* Used for DDR controller debug */
#define DDR_CTRL_DEBUG		
#define SPD_DEBUG	
#endif

#ifdef DDR_CTRL_DEBUG
#define DDR_GET_CLK_ADJ(x) 	 ((x) & 0xf)
#define DDR_GET_WR_DATA_DELAY(x) (((x)>>4) & 0x7)
#define DDR_GET_CPO(x) 		 ((x) & 0x1f)

#define DDR_SET_CLK_ADJ(x, val)  (x = val)
#define DDR_SET_WR_DATA_DELAY(x, val)	\
    				(x = ((x & ~0x70) | ((val & 0x7) << 4)))
#define DDR_SET_CPO(x, val)	 (x = ((x & ~0x1f) | (val & 0x1f)))
#endif

#endif	/* __CONFIG_H */
