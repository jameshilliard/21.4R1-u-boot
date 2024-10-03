/*
 * Copyright (c) 2010-2011, Juniper Networks, Inc.
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
 * ex45xx board configuration file
 *
 * Please refer to doc/README.ex45xx for more info.
 *
 */
#ifndef __CONFIG_H
#define __CONFIG_H

/* High Level Configuration Options */
#define CONFIG_BOOKE		1	/* BOOKE */
#define CONFIG_E500			1	/* BOOKE e500 family */
#define CONFIG_MPC85xx		1	/* MPC8540/60/55/41/48/33/44 */
#define CONFIG_MPC8548		1	/* MPC8548 */
#define CONFIG_EX45XX		1	/* ex45xx board */
#define CONFIG_EXTENDED_BDINFO

#define USB_WRITE_READ         /* enable write and read for usb memory */ 
#define CONFIG_PRODUCTION

#define CONFIG_PCI

/* use local modified tsec driver for SGMII interface */
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
#define CONFIG_BOARD_EARLY_INIT_R 1
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
#define CFG_SDRAM_SIZE	1024		/* DDR is 1G */
#define CFG_SDRAM_SIZE_EX45xx	1024            /* DDR is 1G */
#define CFG_DDR_CS0_BNDS	0x0000001F	/* 0-512MB */
#define CFG_DDR_CS0_CONFIG	0x80010202
#define CFG_DDR_3BIT_BANKS_CONFIG  	0x00004000 /* 3 bit logical banks */
#define CFG_DDR_REFREC	0x00010000
#define CFG_DDR_TIMING_0	0x00260802
#define CFG_DDR_MODE_REV1_0	0x00060452  
#define CFG_DDR_MODE_2	0x0
#define CFG_DDR_INTERVAL	0x06090100
#define CFG_DDR_INTERVAL_CCB_300       0x04880100
#define CFG_DDR_CLK_CTRL	0x02800000 /* latest */
#define CFG_DDRCDR	0x0

/* CAS latency 3 */
#define CFG_DDR_TIMING_1	0x39352322  /* new 10272007 for 1G */
#define CFG_DDR_TIMING_2	0x13104CC8 /* new 08302007*/
#define CFG_DDR_CFG	0xC3000000 /* new 08302007*/
#define CFG_DDR_MODE	0x000E0432   /* new 08302007*/
#define CFG_DDR_CFG_2	0x04401000
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
#define CFG_OR1_PRELIM		0xffff0f47


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
	{9600, 19200, 38400, 115200}

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

/*
 * main board I2C
 */
#define CFG_I2C_CTRL_1    0
#define CFG_I2C_CTRL_2    1
/* I2C controller 1 */
#define CFG_I2C_C1_9548SW1 0x70 /* Mux */ /* U17 */
#define CFG_I2C_C1_9548SW1_ENV_0 0x29 /* W83782D_0 */
#define CFG_I2C_C1_9548SW1_ENV_1 0x2A /* W83782D_1 */
#define CFG_I2C_C1_9548SW1_P1_PS 0x54 /* power supply */
#define CFG_I2C_C1_9548SW1_P2_PS 0x54 /* power supply */
#define CFG_I2C_C1_9548SW1_P3_AD5245_1 0x2C /* front LCD */
#define CFG_I2C_C1_9548SW1_P3_AD5245_2 0x2D /* back LCD */
#define CFG_I2C_C1_9548SW1_P4_SEEPROM 0x72 /* Renesas R5H30211 */
#define CFG_I2C_C1_9548SW1_P5_RTC 0x51 /* RTC M41T11M6E */
#define CFG_I2C_C1_9548SW1_P5_EEPROM 0x53 /* EEPROM AT24C02 */
#define CFG_I2C_C1_9548SW1_P5_FAN 0x54 /* FAN */
#define CFG_I2C_C1_9548SW1_P6_PWR1220 0x40 /* PWR1220 */
#define CFG_I2C_C1_9548SW1_P6_TEMP_0 0x19 /* NE1617A_0*/
#define CFG_I2C_C1_9548SW1_P6_TEMP_1 0x1A /* NE1617A_1*/
#define CFG_I2C_C1_9548SW1_P6_ICS_0 0x6E /* ICS9DB803 */
#define CFG_I2C_C1_9548SW1_P7_MUX_0 0x51 /* ADI MUX 0 */
#define CFG_I2C_C1_9548SW1_P7_MUX_1 0x52 /* ADI MUX 1 */
#define CFG_I2C_C1_9548SW1_P7_MUX_2 0x53 /* ADI MUX 2 */
#define CFG_I2C_C1_9548SW1_P7_MUX_3 0x54 /* ADI MUX 3 */
#define CFG_I2C_C1_9548SW1_P7_ICS_1 0x6E /* ICS9DB803 */
#define CFG_I2C_C1_9548SW1_P7_PEX8618 0x38 /* PEX8618 */
#define CFG_I2C_C1_9548SW1_P7_MK1493 0x69 /* MK1493-03B */

/* I2C controller 2 */
#define CFG_I2C_C2_9546SW1     0x71  /* U19 */
#define CFG_I2C_C2_9546SW1_P0_9548SW2     0x74  /* U51 */
#define CFG_I2C_C2_9546SW1_P0_9548SW2_P0_SFPP_0     0x50
#define CFG_I2C_C2_9546SW1_P0_9548SW2_P1_SFPP_1     0x50
#define CFG_I2C_C2_9546SW1_P0_9548SW2_P2_SFPP_2     0x50
#define CFG_I2C_C2_9546SW1_P0_9548SW2_P3_SFPP_3     0x50
#define CFG_I2C_C2_9546SW1_P0_9548SW2_P4_SFPP_4     0x50
#define CFG_I2C_C2_9546SW1_P0_9548SW2_P5_SFPP_5     0x50
#define CFG_I2C_C2_9546SW1_P0_9548SW2_P6_SFPP_6     0x50
#define CFG_I2C_C2_9546SW1_P0_9548SW2_P7_SFPP_7     0x50
#define CFG_I2C_C2_9546SW1_P1_9548SW3     0x74  /* U53 */
#define CFG_I2C_C2_9546SW1_P1_9548SW3_P0_SFPP_8     0x50
#define CFG_I2C_C2_9546SW1_P1_9548SW3_P1_SFPP_9     0x50
#define CFG_I2C_C2_9546SW1_P1_9548SW3_P2_SFPP_10     0x50
#define CFG_I2C_C2_9546SW1_P1_9548SW3_P3_SFPP_11     0x50
#define CFG_I2C_C2_9546SW1_P1_9548SW3_P4_SFPP_12     0x50
#define CFG_I2C_C2_9546SW1_P1_9548SW3_P5_SFPP_13     0x50
#define CFG_I2C_C2_9546SW1_P1_9548SW3_P6_SFPP_14     0x50
#define CFG_I2C_C2_9546SW1_P1_9548SW3_P7_SFPP_15     0x50
#define CFG_I2C_C2_9546SW1_P2_9548SW4     0x74  /* U52 */
#define CFG_I2C_C2_9546SW1_P2_9548SW4_P0_SFPP_16     0x50
#define CFG_I2C_C2_9546SW1_P2_9548SW4_P1_SFPP_17     0x50
#define CFG_I2C_C2_9546SW1_P2_9548SW4_P2_SFPP_18     0x50
#define CFG_I2C_C2_9546SW1_P2_9548SW4_P3_SFPP_19     0x50
#define CFG_I2C_C2_9546SW1_P2_9548SW4_P4_SFPP_20     0x50
#define CFG_I2C_C2_9546SW1_P2_9548SW4_P5_SFPP_21     0x50
#define CFG_I2C_C2_9546SW1_P2_9548SW4_P6_SFPP_22     0x50
#define CFG_I2C_C2_9546SW1_P2_9548SW4_P7_SFPP_23     0x50
#define CFG_I2C_C2_9546SW1_P3_9548SW5     0x74  /* U67 */
#define CFG_I2C_C2_9546SW1_P3_9548SW5_P0_SFPP_24     0x50
#define CFG_I2C_C2_9546SW1_P3_9548SW5_P1_SFPP_25     0x50
#define CFG_I2C_C2_9546SW1_P3_9548SW5_P2_SFPP_26     0x50
#define CFG_I2C_C2_9546SW1_P3_9548SW5_P3_SFPP_27     0x50
#define CFG_I2C_C2_9546SW1_P3_9548SW5_P4_SFPP_28     0x50
#define CFG_I2C_C2_9546SW1_P3_9548SW5_P5_SFPP_29     0x50
#define CFG_I2C_C2_9546SW1_P3_9548SW5_P6_SFPP_30     0x50
#define CFG_I2C_C2_9546SW1_P3_9548SW5_P7_SFPP_31     0x50

#define CFG_I2C_C2_9546SW2    0x77  /* U20 */
#define CFG_I2C_C2_9546SW2_P0_9548SW6    0x74  /* U68 */
#define CFG_I2C_C2_9546SW2_P0_9548SW6_P0_SFPP_32     0x50
#define CFG_I2C_C2_9546SW2_P0_9548SW6_P1_SFPP_33     0x50
#define CFG_I2C_C2_9546SW2_P0_9548SW6_P2_SFPP_34     0x50
#define CFG_I2C_C2_9546SW2_P0_9548SW6_P3_SFPP_35     0x50
#define CFG_I2C_C2_9546SW2_P0_9548SW6_P4_SFPP_36     0x50
#define CFG_I2C_C2_9546SW2_P0_9548SW6_P5_SFPP_37     0x50
#define CFG_I2C_C2_9546SW2_P0_9548SW6_P6_SFPP_38     0x50
#define CFG_I2C_C2_9546SW2_P0_9548SW6_P7_SFPP_39     0x50
#define CFG_I2C_C2_9546SW2_P1_M2_I2C     0x53
#define CFG_I2C_C2_9546SW2_P2_M1_80_I2C     0x53
#define CFG_I2C_C2_9546SW2_P3_M1_40_I2C     0x53

#define CFG_I2C_C2_9506EXP1     0x21  /* U16 */
#define CFG_I2C_C2_9546SW1_P0_9506EXP2     0x24  /* U55 */
#define CFG_I2C_C2_9546SW1_P1_9506EXP3     0x20  /* U65 */
#define CFG_I2C_C2_9546SW1_P1_9506EXP4     0x22  /* U66 */
#define CFG_I2C_C2_9546SW1_P1_9506EXP5     0x24  /* U64 */
#define CFG_I2C_C2_9546SW1_P2_9506EXP6     0x20  /* U207 */
#define CFG_I2C_C2_9546SW1_P2_9506EXP7     0x22  /* U210 */
#define CFG_I2C_C2_9546SW1_P2_9506EXP8     0x24  /* U208 */

/*
 * M1-SFP+ I2C
 */
/* rev 2 */
#define CFG_I2C_M1_SFPP_9548SW1                     0x70  /* U8 */
#define CFG_I2C_M1_SFPP_9548SW1_P0_SFPP_1           0x50
#define CFG_I2C_M1_SFPP_9548SW1_P1_SFPP_0           0x50
#define CFG_I2C_M1_SFPP_9548SW1_P2_SFPP_3           0x50
#define CFG_I2C_M1_SFPP_9548SW1_P3_SFPP_2           0x50
#define CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1         0x20  /* U7 */
#define CFG_I2C_M1_SFPP_9548SW1_P5_TCPU             0x60
#define CFG_I2C_M1_SFPP_9548SW1_P5_LM75             0x48
#define CFG_I2C_M1_SFPP_9548SW1_P6_SEEPROM          0x72

/* rev 1 */
#define CFG_I2C_M1_SFPP_SEEPROM_REV1                0x52
#define CFG_I2C_M1_SFPP_9548SW1_P5_ADT7410_REV1     0x48

/*
 * M2-LBK I2C
 */
/* rev 2 */
#define CFG_I2C_M2_LBK_9548SW1                      0x70  /* U3 */
#define CFG_I2C_M2_LBK_9548SW1_P0_9506EXP1          0x20  /* U5 */
#define CFG_I2C_M2_LBK_9548SW1_P1_SEEPROM           0x72
#define CFG_I2C_M2_LBK_9548SW1_P2_TCPU              0x60
#define CFG_I2C_M2_LBK_9548SW1_P2_LM75              0x48

/* rev 1 */
#define CFG_I2C_M2_LBK_9543SW1_REV1                 0x70  /* U3 */
#define CFG_I2C_M2_LBK_9543SW1_P0_9506EXP1_REV1     0x20  /* U5 */
#define CFG_I2C_M2_LBK_9543SW1_P1_TCPU_REV1         0x60
#define CFG_I2C_M2_LBK_9548SW1_P0_ADT7410_REV1      0x48
#define CFG_I2C_M2_LBK_SEEPROM_REV1                 0x52

/*
 * M2-LBK I2C
 */
#define CFG_I2C_M2_OPTIC_9543SW1_REV1                 0x70
#define CFG_I2C_M2_OPTIC_SEEPROM_REV1                 0x62

/*
 * M1/M2 misc
 */
#define CFG_I2C_C2_9546SW2_M1_40_CHAN    3
#define CFG_I2C_C2_9546SW2_M1_80_CHAN    2
#define CFG_I2C_C2_9546SW2_M2_CHAN    1

/* M1/M2 test circuit CPU channel */
#define CFG_I2C_C2_9546SW2_M1_CPU_CHAN    5
#define CFG_I2C_C2_9546SW2_M2_CPU_CHAN    2


/*
 * main board EEPROM
 */
#define CFG_I2C_EEPROM_ADDR CFG_I2C_C1_9548SW1_P4_SEEPROM
#define CFG_I2C_EEPROM_ADDR_LEN 1
#define CFG_EEPROM_PAGE_WRITE_BITS 3
#define CFG_EEPROM_PAGE_WRITE_DELAY_MS 10
#define CFG_EEPROM_MAC_OFFSET 56

/*
 * main board EEPROM platform ID
 */
#define I2C_ID_JUNIPER_EX4500_40F       0x095a  /* Java Tsunami T40 */
#define I2C_ID_JUNIPER_EX4500_20F       0x095b  /* Java Tsunami T20 */

/*
 * M1/M2 card EEPROM platform ID
 */
#define I2C_ID_TSUNAMI_UPLINK_SFPPLUS_4PORT_PIC     0x02FF
#define I2C_ID_TSUNAMI_M2_LOOPBACK_PIC              0x02D2
#define I2C_ID_EX4500_UPLINK_XFP_4PORT_PIC              0x0A1D
#define I2C_ID_EX4500_UPLINK_M2_OPTICAL_PIC             0x0A1E
#define I2C_ID_EX4500_UPLINK_M2_LEGACY_2PORT_PIC        0x0A1F

/*
 * Power supply EEPROM platform ID
 */
#define I2C_ID_EX4500F_PWR_1050                 0x043C  /* 1050W */

/*
 * misc board ID
 */
#define CONFIG_BOARD_TYPES	1	/* support board types		*/

#define CFG_EX4500_T40_INDEX            0
#define CFG_EX4500_T20_INDEX            0
#define CFG_EX4500_M1_40_INDEX          1
#define CFG_EX4500_M1_80_INDEX          2
#define CFG_EX4500_M2_INDEX             3
#define CFG_EX4500_LAST_CARD            4

/*
 * General PCI
 * Addresses are mapped 1-1.
 */
#define CFG_PCI1_MEM_BASE	0x80000000
#define CFG_PCI1_MEM_PHYS	CFG_PCI1_MEM_BASE
#define CFG_PCI1_MEM_SIZE	0x10000000	/* 256M */
#define CFG_PCI1_IO_BASE	0x00000000
#define CFG_PCI1_IO_PHYS	0xe2000000
#define CFG_PCI1_IO_SIZE	0x00100000	/* 1M */

/*
 * PCI Express
 */
#define CONFIG_PCIE   		1
#define CFG_PCIE_MEM_BASE 0x90000000
#define CFG_PCIE_MEM_PHYS	CFG_PCIE_MEM_BASE
#define CFG_PCIE_MEM_SIZE	0x40000000	/* 1G */

/*
 * PCI/PCIe ID
 */
#define PCI_MPC8548_VENDOR_ID  0x1057
#define PCI_MPC8548_DEVICE_ID  0x0013
#define PCIE_MPC8548_VENDOR_ID  0x1957
#define PCIE_MPC8548_DEVICE_ID  0x0013
#define PCIE_LION_VENDOR_ID 0x11ab
#define PCIE_LION_DEVICE_ID 0xe041
#define PCIE_PEX8618_VENDOR_ID  0x10b5
#define PCIE_PEX8618_DEVICE_ID  0x8618


#if defined(CONFIG_PCI)

#define CONFIG_NET_MULTI
#define CONFIG_PCI_PNP	               	/* do pci plug-and-play */
#undef CONFIG_85XX_PCI2

#undef CONFIG_PCI_SCAN_SHOW		/* show pci devices on startup */
#define CFG_PCI_SUBSYS_VENDORID 0x1957  /* Freescale */

#endif	/* CONFIG_PCI */


#ifndef CONFIG_NET_MULTI
#define CONFIG_NET_MULTI 	1
#endif

#define CONFIG_MII		1	/* MII PHY management */
#define CONFIG_MPC85XX_TSEC1	1
#define CONFIG_MPC85XX_TSEC1_NAME	"TSEC0"
#define CONFIG_MPC85XX_TSEC2	1
#define CONFIG_MPC85XX_TSEC2_NAME	"TSEC1"
#define CONFIG_MPC85XX_TSEC3	1
#define CONFIG_MPC85XX_TSEC3_NAME	"TSEC2"
#define CONFIG_MPC85XX_TSEC4	1
#define CONFIG_MPC85XX_TSEC4_NAME	"TSEC3"
#undef CONFIG_MPC85XX_FEC

#define TSEC1_PHY_ADDR		5 /* 88E1145 */
#define TSEC2_PHY_ADDR		4 /* 88E1145 */
#define TSEC3_PHY_ADDR		6 /* 88E1145 */
#define TSEC4_PHY_ADDR		7 /* 88E1145 */
#define TSEC4_PHY_ADDR_1	1 /* 88E1112 */

/* all four TSECs share the same smi interface */
#define TSEC1_PHYIDX		0
#define TSEC2_PHYIDX		0
#define TSEC3_PHYIDX		0
#define TSEC4_PHYIDX		0

/* Options are: TSEC[0-3] */
#define CONFIG_ETHPRIME		"TSEC3"

/*
 * USB configuration
 */
#define CONFIG_USB_EHCI		1
#define CONFIG_USB_STORAGE	1
#define CONFIG_DOS_PARTITION
#define CONFIG_USB_SHOW_NO_PROGRESS

#define USB_OHCI_VEND_ID 		0x1131
#define USB_EHCI_VEND_ID 		0x1131
#define USB_OHCI_DEV_ID	 	0x1561
#define USB_EHCI_DEV_ID	 	0x1562

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
#define CFG_LCD_NUM 1

/*
 * Environment
 */
#define CFG_FLASH_SIZE      	0x800000
#define CFG_ENV_IS_IN_FLASH	1
#define CFG_ENV_ADDR		(CFG_FLASH_BASE)
#define CFG_ENV_OFFSET		0
#define CFG_ENV_SECTOR		0
#define CFG_ENV_SECT_SIZE	0x2000	/* 8K(1x8K sector) for env */
#define CFG_OPQ_ENV_ADDR        (CFG_FLASH_BASE + 0x2000) /* for RE nv env */
#define CFG_OPQ_ENV_OFFSET 0x2000
#define CFG_OPQ_ENV_SECT_SIZE   0x2000  /* 8K(1x8K sector) for RE nv env */
#define CFG_UPGRADE_ADDR		(CFG_FLASH_BASE + 0XE000)
#define CFG_UPGRADE_SECT_SIZE	0x2000	/* 8K(1x8K sector) for upgrade */
#define CFG_ENV_SIZE		0x2000

#define CONFIG_LOADS_ECHO	1	/* echo on for serial download */
#define CFG_LOADS_BAUD_CHANGE	1	/* allow baudrate change */
#define CONFIG_RESET_PHY_R      1       /* reset_phy() */

#define  CONFIG_COMMANDS	(CONFIG_CMD_DFL \
				| CFG_CMD_PCI \
				| CFG_CMD_PING \
				| CFG_CMD_I2C \
				| CFG_CMD_MII \
				| CFG_CMD_USB \
				| CFG_CMD_CACHE \
				| CFG_CMD_ELF \
				| CFG_CMD_EEPROM \
				| CFG_CMD_BSP)

/*  *  *  *  *  * Power On Self Test support * * * * *  */
#define CONFIG_POST ( CFG_POST_MEMORY  \
					| CFG_POST_MEMORY_RAM  \
                                   | CFG_POST_WATCHDOG \
					| CFG_POST_UART \
					| CFG_POST_CPU \
					| CFG_POST_I2C \
					| CFG_POST_USB \
					| CFG_POST_PCI \
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
 /* MPC8548 12 External Interrupts 0x1200 - 0x120B */
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
#define NUM_EXT_PORTS        52

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
#define CONFIG_ETHADDR   00:E0:0C:00:00:3C
#define CONFIG_HAS_ETH1
#define CONFIG_ETH1ADDR  00:E0:0C:00:00:3D
#define CONFIG_HAS_ETH2
#define CONFIG_ETH2ADDR  00:E0:0C:00:00:3E
#define CONFIG_HAS_ETH3
#define CONFIG_ETH3ADDR  00:E0:0C:00:00:3F

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
   "bootelf 0xffe00000"
#else
#define CONFIG_BOOTCOMMAND  \
   "bootelf 0xfff00000"
#endif

#define CONFIG_AUTOBOOT_KEYED		/* use key strings to stop autoboot */
/* 
 * Using ctrl-c to stop booting and entering u-boot command mode.  
 * Space is too easy to break booting by accident.  Ctrl-c is common
 * but not too easy to interrupt the booting by accident.
 */
#define CONFIG_AUTOBOOT_STOP_STR	"\x03"
#define CONFIG_SILENT_CONSOLE	0

#define	CFG_VERSION_MAJOR	1
#define	CFG_VERSION_MINOR	0
#define	CFG_VERSION_PATCH	0

#endif	/* __CONFIG_H */
