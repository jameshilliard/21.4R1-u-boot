/*
 * Copyright (c) 2008-2012, Juniper Networks, Inc.
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
 * EX82xx board configuration file
 */
#ifndef __CONFIG_H
#define __CONFIG_H

/* High Level Configuration Options */
#define CONFIG_BOOKE		1	/* BOOKE */
#define CONFIG_E500		1	/* BOOKE e500 family */
#define CONFIG_MPC85xx		1	/* MPC8540/60/55/41/48/33/44 */
#define CONFIG_MPC8548		1	/* MPC8548 */
#define CONFIG_EX82XX		1	/* ex82xx board */

#define CONFIG_CCB_FPGA	1
#define CCB_FPGA_TEST_SUPPORT	1

#undef CONFIG_PCI		        /* ex82xx doesn't use PCI */

#define  CONFIG_PHY_GIGE

#define CONFIG_ENV_OVERWRITE

#define CONFIG_SPD_EEPROM		/* Use SPD EEPROM for DDR setup*/
#undef CONFIG_DDR_DLL			/* possible DLL fix needed */
#undef CONFIG_DDR_2T_TIMING		/* Sets the 2T timing bit */

#define CONFIG_DDR_ECC			/* only for ECC DDR module */
#undef  CONFIG_ECC_INIT_VIA_DDRCONTROLLER		/* DDR controller or DMA? */
#define CONFIG_MEM_INIT_VALUE		0x00000000

/*NEEDED to enable booting based on Juniper ID eeprom format*/
#define CONFIG_JUNIPER_ID_EEPROM	1

#define CFG_EX82XX_ID_EEPROM			1
#define EX82XX_LOCAL_ID_EEPROM_ADDR		0x57
#define EX82XX_LC_ID_EEPROM_ADDR		0x51
#define EX82XX_I2CS_ID_EEPROM_ADDR		0x51
#define EX82XX_ASSEMBLY_ID_OFFSET		0x04
#define EX82XX_ID_EEPROM_MAC_BASE_OFFSET	0x38
#define EX82XX_ID_EEPROM_MAC_COUNT_OFFSET	0x36 
#define EX82XX_ID_EERPROM_GROUP		1

#define EX82XX_PGSCB_ASSEMBLY_ID	0x0928
#define EX82XX_PLVCPU_ASSEMBLY_ID	0x092F
#define EX82XX_GSCB_ASSEMBLY_ID		0x0929
#define EX82XX_VCB_ASSEMBLY_ID		0x0949
#define EX82XX_GRANDE_CHASSIS		0x051E
#define EX82XX_VENTI_CHASSIS		0x051F	

/*IRI related*/
#define EX82XX_IRI_RE_CHIP_TYPE		0
#define EX82XX_IRI_LCPU_CHIP_TYPE	0
#define EX82XX_IRI_VCPU_CHIP_TYPE	1
#define EX82XX_CHIP_TYPE_SHIFT		4
#define EX82XX_CHIP_TYPE_MASK		0xF0
#define EX82XX_IRI_IF_MASK			0xF
#define EX82XX_IRI_IP_LCPU_BASE		16
#define EX82XX_IRI_IP_VCPU_BASE		32

#define RTC_I2C_ADDR	0x68 /* PGSCB RTC I2C Address */

#ifndef CONFIG_SPD_EEPROM
#error	("CONFIG_SPD_EEPROM is required")
#endif

/*PCI memory Base address for FPGA*/
#define CFG_FPGA_PCI_BADDR		0x20000000 /* FIXME: jnpr_caffeine. Enter correct value for FPGA PCI Base address */

/*
 * When initializing flash, if we cannot find the manufacturer ID,
 * assume this is the AMD flash associated with the CDS board.
 * This allows booting from a promjet.
 */
#define CONFIG_ASSUME_AMD_FLASH

#define MPC85xx_DDR_SDRAM_CLK_CNTL	/* 85xx has clock control reg */


#define CONFIG_SYS_CLK_FREQ		66666666 /* sysclk for caffeine */

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
#define CONFIG_MISC_INIT_R	1	/* call misc_init_r()		*/

#define CFG_ALT_MEMTEST
#undef CFG_DRAM_TEST					/* memory test, takes time */
#define CFG_MEMTEST_START   0x10000000  /* memtest works on */
#define CFG_MEMTEST_END     0x7FFFFFFF
#define CFG_MEMTEST_START_FL    0x00000000  /* memtest works on */
#define CFG_MEMTEST_END_LC  0x3FFFFFFF


/*
 * Base addresses -- Note these are effective addresses where the
 * actual resources get mapped (not physical addresses)
 */
#define CFG_CCSRBAR_DEFAULT	0xff700000	/* CCSRBAR Default */
#define CFG_CCSRBAR				0xFEF00000	/* relocated CCSRBAR */
#define CFG_IMMR				CFG_CCSRBAR	/* PQII uses CFG_IMMR */



/*
 * DDR Setup
 */
#define CFG_DDR_SDRAM_BASE	0x00000000	/* DDR is system memory*/
#define CFG_SDRAM_BASE		CFG_DDR_SDRAM_BASE
#define CONFIG_VERY_BIG_RAM	1 /* SDRAMs larger than 256MBytes */

#define CFG_DDR_CS			4
          
		  
/*
 * Determine DDR configuration from I2C interface.
 */
#define SPD_EEPROM_ADDRESS		{0x50}	/* DDR i2c spd addresses	*/

#undef CONFIG_CLOCKS_IN_MHZ

/*
 * Manually setup DDR3 parameters for 2XS_44GE and 48P line cards.
 */
#define CONFIG_SYS_DDR3_CS0_BNDS		0x0000007F
#define CONFIG_SYS_DDR3_CS0_CONFIG		0x80014302 	
#define CONFIG_SYS_DDR3_CS0_CONFIG_2	0x00000000
#define CONFIG_SYS_DDR3_TIMING_0		0x00330104
#define CONFIG_SYS_DDR3_TIMING_1		0x6f6b4846
#define CONFIG_SYS_DDR3_TIMING_2		0x0fa8c0cf
#define CONFIG_SYS_DDR3_TIMING_3		0x00040000	
#define CONFIG_SYS_DDR3_TIMING_4		0x00220001
#define CONFIG_SYS_DDR3_TIMING_5		0x02401400
#define CONFIG_SYS_DDR3_MODE			0x00421421
#define CONFIG_SYS_DDR3_MODE_2			0x04000000
#define CONFIG_SYS_DDR3_MD_CNTL 		0x00000000
#define CONFIG_SYS_DDR3_INTERVAL		0x09300100
#define CONFIG_SYS_DDR3_DATA_INIT		0xdeadbeef
#define CONFIG_SYS_DDR3_CLK_CNTL		0x02800000
#define CONFIG_SYS_DDR3_INIT_ADDR		0x00000000
#define CONFIG_SYS_DDR3_INIT_EXT_ADDR	0x00000000
#define CONFIG_SYS_DDR3_ZQ_CNTL 		0x88000000
#define CONFIG_SYS_DDR3_WRLVL_CNTL		0x8645a607
#define CONFIG_SYS_DDR3_SR_CNTR 		0x00000000
#define CONFIG_SYS_DDR3_RCW_1			0x00050000
#define CONFIG_SYS_DDR3_RCW_2			0x00000000
#define CONFIG_SYS_SDRAM_CFG			0xC7000000
#define CONFIG_SYS_SDRAM_CFG_2			0x24401054
#define CONFIG_SYS_SDRAM_CFG_INITIAL	0x47000000
#define CONFIG_SYS_SDRAM_CFG_NO_ECC 	0xf7010000
#define CONFIG_SYS_DDR3_IP_REV1 		0x00020403
#define CONFIG_SYS_DDR3_IP_REV2 		0x00000100
#define CONFIG_SYS_DDR3_ERR_INT_EN		0x00000000
#define CONFIG_SYS_DDR3_CDR1			0x00000000
#define CONFIG_SYS_DDR3_CDR2			0x00000000
#define SDRAM_CFG_MEM_EN				0x80000000
#define CONFIG_SYS_DDR3_ERR_DISABLE 	0x0000000d
#define CONFIG_SYS_DDR3_ERR_SBE 		0x00ff0000
#define CONFIG_SYS_DDR3_DINIT			0x00000010

/*
 * Local Bus Definitions
 */

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
 *    BR1[27] = Reserved = 0
 *    ATOM = not use atomic operate = BR0[28:29] = 00
 *    BR1[30] = Reserved = 0
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

#define CFG_FLASH_BASE               0xff800000	/* start of FLASH 8M */

#define CFG_BR0_PRELIM               0xff800801
#define CFG_OR0_PRELIM               0xff806e65
#define CFG_OR0_PRELIM_2XS_44GE_48P  0xff800800

#define CFG_FLASH_BANKS_LIST	{CFG_FLASH_BASE}
#define CFG_MAX_FLASH_BANKS	1		/* number of banks */
#define CFG_MAX_FLASH_SECT	142		/* sectors per device */
#undef CFG_FLASH_CHECKSUM
#define CFG_FLASH_ERASE_TOUT	60000	/* Flash Erase Timeout (ms) */
#define CFG_FLASH_WRITE_TOUT	500	/* Flash Write Timeout (ms) */

#define CFG_MONITOR_BASE		TEXT_BASE	/* start of monitor */

#define CFG_FLASH_CFI_DRIVER
#define CFG_FLASH_CFI
#define CFG_FLASH_EMPTY_INFO

/*
 * DUART (SCB) on the Local Bus 
 * BR1/OR1 0xff20_0000 - 0xff2f_ffff (1M)
 *
 * BR1:
 *    BA (Base Address) = 0xff00_0000 = BR1[0:16] = 1111 1111 0020 0000 0
 *    XBA (Address Space eXtent) = BR1[17:18] = 00
 *    PS (Port Size) = 8 bits = BR1[19:20] = 01
 *    DECC = disable parity check = BR1[21:22] = 00
 *    WP = enable both read and write protect = BR1[23] = 0
 *    MSEL = use GPCM = BR1[24:26] = 000
 *    BR1[27] = Reserved = 0
 *    ATOM = not use atomic operate = BR1[28:29] = 00
 *    BR1[30] = Reserved = 0
 *    Valid = BR1[31] = 1
 *
 * 0      4      8     12    16     20    24     28
 * 1111 1111 0010 0000 0000 1000 0000 0001 = ff000801    BR1
 *
 * OR1:
 *    Addr Mask = 1M = OR1[0:16] = 1111 1111 1111 0000 0
 *    Reserved OR1[17:18] = 00
 *    BCTLD = buffer control not assert = OR1[19] = 1
 *    CSNT = chip select /CS and /WE negated = OR1[20] = 1 
 *    ACS = address to CS at same time as address line = OR1[21:22] = 00
 *    XACS = extra address setup = OR1[23] = 0
 *    SCY = 7 clock wait states = OR1[24:27] = 0001
 *    SETA (External Transfer Ack) = OR1[28] = 1
 *    TRLX = use relaxed timing = OR1[29] = 1
 *    EHTR (Extended hold time on read access) = OR1[30] = 0
 *    EAD = use external address latch delay = OR1[31] = 0
 *
 * 0      4      8     12    16     20    24     28
 * 1111 1111 1111 0000 0001 1000 0001 1100 = fff0181c    OR1
 */

#define CFG_BR1_PRELIM		0xff200801
#define CFG_OR1_PRELIM		0xfff00835 

#define CFG_CPLD_BASE		0xFF000000
#define CFG_CTRL_CPLD_BASE	0xFF100000
#define CFG_XCTL_CPLD_BASE	0xFF400000

/*
 * LC control CPLD on the Local Bus 
 * BR2/OR2 0xff10_0000 - 0xff1f_ffff (1M)
 *
 * BR2:
 *    BA (Base Address) = 0xff00_0000 = BR1[0:16] = 1111 1111 0001 0000 0
 *    XBA (Address Space eXtent) = BR1[17:18] = 00
 *    PS (Port Size) = 8 bits = BR1[19:20] = 01
 *    DECC = disable parity check = BR1[21:22] = 00
 *    WP = enable both read and write protect = BR1[23] = 0
 *    MSEL = use GPCM = BR1[24:26] = 000
 *    BR1[27] = Reserved = 0
 *    ATOM = not use atomic operate = BR1[28:29] = 00
 *    BR1[30] = Reserved = 0
 *    Valid = BR1[31] = 1
 *
 * 0      4      8     12    16     20    24     28
 * 1111 1111 0001 0000 0000 1000 0000 0001 = ff100801    BR2
 *
 * OR2:
 *    Addr Mask = 1M = OR1[0:16] = 1111 1111 1111 0000 0
 *    Reserved OR1[17:18] = 00
 *    BCTLD = buffer control not assert = OR1[19] = 0
 *    CSNT = chip select /CS and /WE negated = OR1[20] = 1 
 *    ACS = address to CS at same time as address line = OR1[21:22] = 00
 *    XACS = extra address setup = OR1[23] = 0
 *    SCY = 7 clock wait states = OR1[24:27] = 0001
 *    SETA (External Transfer Ack) = OR1[28] = 1
 *    TRLX = use relaxed timing = OR1[29] = 1
 *    EHTR (Extended hold time on read access) = OR1[30] = 0
 *    EAD = use external address latch delay = OR1[31] = 0
 *
 * 0      4      8     12    16     20    24     28
 * 1111 1111 1111 0000 0001 1000 0001 1100 = fff0181c    OR2
 */

#define CFG_BR2_PRELIM               0xff100801
#define CFG_OR2_PRELIM               0xfff0081C
#define CFG_OR2_PRELIM_2XS_44GE_48P  0xfff00000

#define CFG_BR5_PRELIM               0xff400801
#define CFG_OR5_PRELIM               0xfff0081C

/*
 * Boot CPLD on the Local Bus 
 * BR3/OR3 0xff00_0000 - 0xff0f_ffff (1M)
 *
 * BR3:
 *    BA (Base Address) = 0xff00_0000 = BR1[0:16] = 1111 1111 0000 0000 0
 *    XBA (Address Space eXtent) = BR1[17:18] = 00
 *    PS (Port Size) = 8 bits = BR1[19:20] = 01
 *    DECC = disable parity check = BR1[21:22] = 00
 *    WP = enable both read and write protect = BR1[23] = 0
 *    MSEL = use GPCM = BR1[24:26] = 000
 *    BR1[27] = Reserved = 0
 *    ATOM = not use atomic operate = BR1[28:29] = 00
 *    BR1[30] = Reserved = 0
 *    Valid = BR1[31] = 1
 *
 * 0      4      8     12    16     20    24     28
 * 1111 1111 0000 0000 0000 1000 0000 0001 = ff100801    BR3
 *
 * OR3:
 *    Addr Mask = 1M = OR1[0:16] = 1111 1111 1111 0000 0
 *    Reserved OR1[17:18] = 00
 *    BCTLD = buffer control not assert = OR1[19] = 1
 *    CSNT = chip select /CS and /WE negated = OR1[20] = 1 
 *    ACS = address to CS at same time as address line = OR1[21:22] = 00
 *    XACS = extra address setup = OR1[23] = 0
 *    SCY = 7 clock wait states = OR1[24:27] = 0001
 *    SETA (External Transfer Ack) = OR1[28] = 1
 *    TRLX = use relaxed timing = OR1[29] = 1
 *    EHTR (Extended hold time on read access) = OR1[30] = 0
 *    EAD = use external address latch delay = OR1[31] = 0
 *
 * 0      4      8     12    16     20    24     28
 * 1111 1111 1111 0000 0001 1000 0001 1100 = fff0181c    OR3
 */


#define CFG_BR3_PRELIM               0xff000801
#define CFG_OR3_PRELIM               0xfff01814
#define CFG_OR3_PRELIM_2XS_44GE_48P  0xfff00000

/*
 * Local Bus
 * Flash  0xff80_0000 - 0xffff_ffff  8M
 * CPLD   0xff00_0000 - 0xff10_0000  1M
 */
#define CFG_FLASH_LBC_BASE	0xff800000	 /* 8M boot flash */
#define CFG_CPLD_LBC_BASE	0xff000000     /* 1M */


#define CFG_LBC_LCRR		0x00030008    /* LB clock ratio reg */
#define CFG_LBC_LBCR		0x00000000    /* LB config reg */

#define CONFIG_L1_INIT_RAM
#define CFG_INIT_RAM_LOCK	1
#define CFG_INIT_RAM_ADDR	0xe4010000	/* Initial RAM address */
#define CFG_INIT_RAM_END		0x4000	    /* End of used area in RAM */

#define POST_STOR_WORD  CFG_INIT_RAM_ADDR

#define CFG_GBL_DATA_SIZE		128	    /* num bytes initial data */
#define CFG_GBL_DATA_OFFSET	(CFG_INIT_RAM_END - CFG_GBL_DATA_SIZE)
#define CFG_INIT_SP_OFFSET	CFG_GBL_DATA_OFFSET

#define CFG_MONITOR_LEN			(512 * 1024) /* Reserve 256 kB for Mon */
#define CFG_MALLOC_LEN			(128 * 1024)	/* Reserved for malloc */

/* Serial Port */
#define CONFIG_CONS_INDEX		1

#undef CONFIG_SERIAL_SOFTWARE_FIFO
#define CFG_NS16550
#define CFG_NS16550_SERIAL
#define CFG_NS16550_REG_SIZE	1
#define CFG_NS16550_CLK		get_bus_freq(0)
#define CFG_16C752B_CLK		18432000	/* 18.432 MHZ clock for SCB DUART*/

#define CFG_16C752B
#define CFG_DUART_BASE		0xff200000   

#define CFG_16C752B_COM1		CFG_DUART_BASE 	       
#define CFG_16C752B_COM2		(CFG_DUART_BASE + 0x10000)

#define CFG_MAX_CPU_COUNT	0x03		/* No. of CPUs running the same u-boot image */	
#define RECPU				0x01			/* Start from 0x01 */
#define LCPU				0x02
#define VCPU				0x03


#define EX82XX_RECPU ((gd->ccpu_type == RECPU))
#define EX82XX_LCPU  ((gd->ccpu_type == LCPU))
#define EX82XX_VCPU  ((gd->ccpu_type == VCPU))

#define EX82XX_PRECPU_BRD_ID     0x0928
#define EX82XX_RECPU_BRD_ID      0x0929
#define EX82XX_PLVCPU_BRD_ID     0x092F
#define EX82XX_LVCPU_BRD_ID      0x092F
#define EX82XX_LC_48T_BRD_ID     0x0931
#define EX82XX_LC_48T_ES_BRD_ID  0x0B0F  /* Caffeine LC 48x1G RJ45 ES */
#define EX82XX_LC_48F_BRD_ID     0x0930  /* Caffeine LC 48x1G SFP */
#define EX82XX_LC_48F_ES_BRD_ID  0x0B0E  /* Caffeine LC 48x1G SFP ES */
#define EX82XX_LC_8XS_BRD_ID     0x094E  /* Caffeine LC 8x10G SFP+ */
#define EX82XX_LC_8XS_ES_BRD_ID  0x0B0D  /* Caffeine LC 8x10G SFP+ ES */
#define EX82XX_LC_36XS_BRD_ID    0x094F  /* Caffeine LC 36x10G SFP+ */
#define EX82XX_LC_40XS_BRD_ID    0x0959  /* Caffeine LC 40x10G SFP+ */
#define EX82XX_LC_40XS_ES_BRD_ID 0x0B10  /* Caffeine LC 40x10G SFP+ ES */
#define EX82XX_LC_48P_BRD_ID	0x09CA  /* Caffeine LC 48x1G */
#define EX82XX_LC_48TE_BRD_ID	0x09DC	/* Caffeine LC 48TE, non-POE */
#define EX82XX_LC_2XS_44GE_BRD_ID	0x09CB  /* Caffeine LC 2x10G+44x1G */
#define EX82XX_LC_2X4F40TE_BRD_ID	0x09DD  /* Caffeine LC 2XS44GE, non-POE */
#define EX82XX_LC_8XS_NM_BRD_ID    0x0B2B  /* 8XS (8 x10G) w/ NG Sentinel Memory */
#define EX82XX_LC_48T_NM_BRD_ID    0x0B2C  /* 48T (48 x1G) w/ NG Sentinel Memory */ 
#define EX82XX_LC_48F_NM_BRD_ID    0x0B2D  /* 48F (48 x1G SFP) w/ NG Sentinel Memory */
#define EX82XX_LC_40XS_NM_BRD_ID   0x0B2E  /* 40XS (40 x1G/10g SFP+) w/ NG Sentinel Memory */


#define CFG_BAUDRATE_TABLE  \
	{300, 600, 1200, 2400, 4800, 9600, 19200, 38400,115200}

#define CFG_NS16550_COM1		(CFG_CCSRBAR+0x4500)
#define CFG_NS16550_COM2		(CFG_CCSRBAR+0x4600)

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
#undef CONFIG_SOFT_I2C			/* I2C bit-banged */
#define CONFIG_LAST_STAGE_INIT  /* init 2nd I2C controller */
#define CFG_I2C_SPEED		400000	/* I2C speed and slave address */
#define CFG_I2C_EEPROM_ADDR	0x57
#define CFG_I2C_SLAVE		0x7F
#define CFG_I2C_EEPROM_ADDR_LEN 1
#define CFG_I2C_OFFSET		0x3000
#define CFG_I2C2_OFFSET	0x3100
#define CFG_I2C_CTRL_1_SW_ADDR	0x70
#define CFG_I2C_CTRL_2_SW_ADDR	0x71
#define CFG_I2C_CTRL_2_IO_ADDR	0x20
#define CFG_I2C_CTRL_1		1 
#define CFG_I2C_CTRL_2		2
#define CFG_I2C_DDR_CHAN	0x20    /* I2C controller 1, channel 5 PCA9548 */

/* PCI Express */
#define CONFIG_PCIE			1
#define CFG_PCIE_MEM_BASE	0x80000000
#define CFG_PCIE_MEM_SIZE	0x40000000

#if defined(CONFIG_PCI)

#define CONFIG_NET_MULTI
#define CONFIG_PCI_PNP	               	/* do pci plug-and-play */
#undef CONFIG_85XX_PCI2

#undef CONFIG_EEPRO100
#undef CONFIG_TULIP

#undef CONFIG_PCI_SCAN_SHOW		/* show pci devices on startup */
#define CFG_PCI_SUBSYS_VENDORID	0x1957  /* Freescale */

#endif	/* CONFIG_PCI */

#ifndef CONFIG_NET_MULTI
#define CONFIG_NET_MULTI 	1
#endif

#define CONFIG_MII		1	/* MII PHY management */
#define CONFIG_MPC85XX_TSEC1	1
#define CONFIG_MPC85XX_TSEC1_NAME	"me0"
#define CONFIG_MPC85XX_TSEC2	1
#define CONFIG_MPC85XX_TSEC2_NAME	"me1"
#define CONFIG_MPC85XX_TSEC3	1
#define CONFIG_MPC85XX_TSEC3_NAME	"me2"
#define CONFIG_MPC85XX_TSEC4    1
#define CONFIG_MPC85XX_TSEC4_NAME   "me3"
#undef CONFIG_MPC85XX_FEC


#define TSEC1_PHY_ADDR		0
#define TSEC2_PHY_ADDR		1
#define TSEC3_PHY_ADDR		2
#define TSEC4_PHY_ADDR		3


#define TSEC1_PHYIDX		0
#define TSEC2_PHYIDX		0
#define TSEC3_PHYIDX		0
#define TSEC4_PHYIDX		0

/*
 * USB configuration
 */
#define USB_WRITE_READ         /* enable write and read for usbmemory stick */ 
#define CONFIG_USB_STORAGE	
#define CONFIG_USB_EHCI		 
#define USB_OHCI_VEND_ID	0x1131
#define USB_OHCI_DEV_ID		0x1561
#define USB_EHCI_DEV_ID		0x1562 /* EHCI not implemented */
#define CONFIG_USB_PCIE		1   /*USB device is connected to the PCIE interface*/

#define CONFIG_DOS_PARTITION

#define CONFIG_SHOW_ACTIVITY	1

/*
 * Environment
 */
#define CFG_ENV_IS_IN_FLASH	1
#define CFG_ENV_ADDR		(CFG_FLASH_BASE)
#define CFG_ENV_SECT_SIZE	0x2000	/* 8K(one sector) for env */
#define CFG_ENV_SIZE		0x2000

#define CONFIG_LOADS_ECHO	1	/* echo on for serial download */
#define CFG_LOADS_BAUD_CHANGE	1	/* allow baudrate change */

#if defined(CONFIG_PCI)
#define  CONFIG_COMMANDS	(CONFIG_CMD_DFL \
				| CFG_CMD_PCI \
				| CFG_CMD_PING \
                | CFG_CMD_DHCP \
				| CFG_CMD_I2C \
				| CFG_CMD_MII \
                | CFG_CMD_DATE \
                | CFG_CMD_USB \
                | CFG_CMD_DIAG \
				| CFG_CMD_CACHE \
				| CFG_CMD_ELF \
				| CFG_CMD_EEPROM \
				| CFG_CMD_BSP)
#else
#define  CONFIG_COMMANDS	(CONFIG_CMD_DFL \
				| CFG_CMD_PING \
                | CFG_CMD_DHCP \
				| CFG_CMD_I2C \
				| CFG_CMD_MII \
				| CFG_CMD_USB \
                | CFG_CMD_DATE \
				| CFG_CMD_CACHE \
				| CFG_CMD_ELF \
				| CFG_CMD_EEPROM \
				| CFG_CMD_BSP)
#endif

/*  *  *  *  *  * Power On Self Test support * * * * *  */
#define CONFIG_POST ( CFG_POST_WATCHDOG \
					| CFG_POST_MEMORY  \
					| CFG_POST_MEMORY_RAM \
					| CFG_POST_I2C  \
					| CFG_POST_CPU  \
					| CFG_POST_USB  \
					| CFG_POST_UART  \
					| CFG_POST_PCIE \
					| CFG_POST_ETHER \
                    | CFG_POST_VOLTAGE_MONITOR)
                       

#ifdef CONFIG_POST
#define CFG_CMD_POST_DIAG CFG_CMD_DIAG
#else
#define CFG_CMD_POST_DIAG       0
#endif

#include <cmd_confdefs.h>

#undef  CONFIG_WATCHDOG				/* watchdog disabled */
#define CONFIG_CPLD_WATCHDOG		/* CPLD watchdog */

/*
 * PIC 
 */
 /* MPC8548 12 External Interrupts 0x1200 - 0x120B */
#define IRQ0_INT_VEC		0x1200
#define IRQ1_INT_VEC		0x1201
#define IRQ2_INT_VEC		0x1202
#define IRQ3_INT_VEC		0x1203
#define IRQ4_INT_VEC		0x1204
#define IRQ5_INT_VEC		0x1205
#define IRQ6_INT_VEC		0x1206
#define IRQ7_INT_VEC		0x1207
#define IRQ8_INT_VEC		0x1208
#define IRQ9_INT_VEC		0x1209
#define IRQ10_INT_VEC		0x120A
#define IRQ11_INT_VEC		0x120B

/*
 * Ports 
 */
#define NUM_EXT_PORTS		24

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
#define CONFIG_ETHADDR	00:E0:0C:00:00:FD

#undef CONFIG_IPADDR
#undef CONFIG_SERVERIP
#undef CONFIG_GATEWAYIP
#undef CONFIG_NETMASK

#define CONFIG_LOADADDR		0x400000   /*default location for tftp and bootm*/

#define CONFIG_BOOTDELAY	1       /* -1 disables auto-boot */
#undef  CONFIG_BOOTARGS           /* the boot command will set bootargs*/

#define CONFIG_BAUDRATE		9600

#define CONFIG_SECONDSTAGEBOOTCOMMAND \
   "cp.b 0xFFF00000 $loadaddr 0x40000;"		\
   "bootelf $loadaddr"                    


#define CONFIG_BOOTCOMMAND	CONFIG_SECONDSTAGEBOOTCOMMAND

#define CONFIG_BOOTCOMMAND_BANK1 \
    "cp.b 0xFFB00000 $loadaddr 0x40000;" \
    "bootelf $loadaddr"

#define EX82XX_LC_PKG_NAME	"jpfe-ex82x-install-ex.tgz"
#define CFG_BASE_IRI_IP		"128.0.0.1"

#define CFG_UPGRADE_LOADER_BANK1	0xFFB00000
#define CFG_UPGRADE_LOADER_BANK0	0xFFF00000
#define CFG_UPGRADE_UBOOT_BANK1		0xFFB80000	
#define CFG_UPGRADE_UBOOT_BANK0		0xFFF80000	

#define CFG_UPGRADE_BOOT_STATE_OFFSET	0xE000 /* from beginning of flash */
#define CFG_UPGRADE_BOOT_SECTOR_SIZE	0x2000  /* 8k */

#define CFG_IMG_HDR_OFFSET	0x30 /* from image address */
#define CFG_CHKSUM_OFFSET	0x100 /* from image address */

#define CFG_CONSOLE_INFO_QUIET	1
#define CONFIG_USB_SHOW_NO_PROGRESS

#define CONFIG_PCA9557	1
#define CONFIG_RTC1672	1

#define CFG_VERSION_MAJOR	3
#define CFG_VERSION_MINOR	6
#define CFG_VERSION_PATCH	0

#define CONFIG_AUTOBOOT_KEYED    /* use key strings to stop autoboot */
/*
 * Use ctrl-c to stop booting and enter u-boot command mode.
 * Ctrl-c is common but not too easy to interrupt the booting by accident.
 */
#define CONFIG_AUTOBOOT_STOP_STR	"\x03"

/* RE boot states */
#define RE_STATE_EEPROM_ACCESS	0x01
#define RE_STATE_IN_UBOOT		0x02
#define RE_STATE_BOOTING		0x03

#define SLOT_SCB0	0x00
#define SLOT_SCB1	0x01
#define I2CS_SCRATCH_PAD_REG_00	0x00

#define CFG_LC_MODE_ENV		"hw.lc.mode"
#define LC_MODE_CHASSIS		0
#define LC_MODE_DIAGNOSTIC	1
#define LC_MODE_STANDALONE	2
#define LC_MODE_MAX_VAL		LC_MODE_STANDALONE

#define CFG_EX8208_SCB_SLOT_BASE		0x68
#define CFG_EX8216_CB_SLOT_BASE			0x7E

#define LCMODE_IS_STANDALONE	(ex82xx_lc_mode == LC_MODE_STANDALONE)
#define LCMODE_IS_DIAGNOSTIC	(ex82xx_lc_mode == LC_MODE_DIAGNOSTIC)

/* Memory addresses used for USB read/write operation */
#define CFG_USBCHK_MEM_READ_ADDR	0x200000
#define CFG_USBCHK_MEM_WRITE_ADDR	0x400000

#endif	/* __CONFIG_H */
