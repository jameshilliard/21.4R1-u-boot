/*
 * Copyright (c) 2009-2012, Juniper Networks, Inc.
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

#include <common.h>
#include <command.h>
#include <pci.h>
#include <usb.h>
#include <asm/processor.h>
#include <asm/mmu.h>
#include <asm/immap_85xx.h>
#include <asm/io.h>
#include <spd.h>
#include <miiphy.h>
#include <i2c.h>
#include <watchdog.h>
#include "tbbcpld.h"
#include "bootcpld_tbb_re_sw.h"

#define MX5_T_NAME_STRING       "MX5-T"
#define MX10_T_NAME_STRING      "MX10-T"
#define MX40_T_NAME_STRING      "MX40-T"
#define MX80_NAME_STRING        "MX80"
#define MX80_T_NAME_STRING      "MX80-T"
#define MX80_P_NAME_STRING      "MX80-P"
#define MX80_48T_NAME_STRING    "MX80-48T"

/*
 * Next-boot information is stored in a tbb cpld scratchpad-1 register.
 * Register format is as follows.
 * ------------------------------------------------------------------------+
 * | BIT 7  |  BIT 6 |  BIT 5 |  BIT 4 |  BIT 3 |  BIT 2 |  BIT 1 |  BIT 0 |
 * +--------------------------+--------+--------+--------+--------+--------+
 * |    NEXT_BOOT             |  X     |  USB   | FLASH0 | FLASH1 |  LAN   |
 * |                          |        |  BOOT  | BOOT   |  BOOT  |  BOOT  |
 * +--------------------------+--------+--------+--------+--------+--------+
 */
#define LAN_BOOT                        0
#define NAND_FLASH_1_BOOT               1
#define NAND_FLASH_0_BOOT               2
#define USB_BOOT                        3
#define NO_BOOT                         4
#define LAN_BOOTMASK                    (1 << LAN_BOOT)
#define NAND_FLASH_1_BOOTMASK           (1 << NAND_FLASH_1_BOOT)
#define NAND_FLASH_0_BOOTMASK           (1 << NAND_FLASH_0_BOOT)
#define USB_BOOTMASK                    (1 << USB_BOOT)
#define NOBOOT_MASK                     (1 << NO_BOOT)
#define NEXTBOOT_B2V(x)                 ((x & 0xe0) >> 5)


extern block_dev_desc_t usb_dev_desc[USB_MAX_STOR_DEV];
extern struct usb_device usb_dev[USB_MAX_DEVICE];
int usb_scan_dev = 0;
int watchdog_tickle_override = 0;
DECLARE_GLOBAL_DATA_PTR;

int get_noboot_flag(void)
{
	int abort = 0;
	/* CPLD scratch pad reg 1 stores boot information */
	volatile u_int8_t *p_nextboot = (u_int8_t *)(CFG_TBBCPLD_BASE + 
                                              BOOTCPLD_TBB_RE_SCRATCH_1_ADDR);

	/*
	 * If noboot is set, ask u-boot to abort autoboot process.
	 * Also clear noboot flag if set. This is to ensure that
	 * we are never forever stuck in noboot state.
	 */
	if (*p_nextboot & NOBOOT_MASK) {
	    abort = 1;
	    *p_nextboot &= ~NOBOOT_MASK;
	}

	return abort;
}

void  set_noboot_flag(void)
{
	/* CPLD scratch pad reg 1 stores boot information */
	volatile u_int8_t *p_nextboot = (u_int8_t *)(CFG_TBBCPLD_BASE + 
                                              BOOTCPLD_TBB_RE_SCRATCH_1_ADDR);

	/*
	 * Set noboot bit.
	 */
	*p_nextboot |= NOBOOT_MASK;
}

static void set_nextboot_device(int dev, int is_netboot)
{
	char dev_str[32];
	if (is_netboot) {
		sprintf(dev_str, "net%d:", dev);
	} else {
		sprintf(dev_str, "disk%d:", dev);
	}
	setenv("loaddev", dev_str);
}

static int is_external_usb_present(int *dev)
{
	unsigned char i;
	struct usb_device *pdev;

	for(i=0;i<USB_MAX_STOR_DEV;i++) {
		if (usb_dev_desc[i].target == 0xff) {
			continue;
		}

		pdev = &usb_dev[usb_dev_desc[i].target -1];
		if (pdev->devnum == -1)
			continue;

		/*
		 * Make sure it's a child device
		 * and port num is 1 (external USB port).
		 */
		if (pdev->parent && pdev->portnr == 1) {
			printf("External USB present\n");
			*dev = 2;
			return 1;
		}
	}

	return 0; /* no external usb device found */
}

static void set_boot_disk(void)
{
	int dev = 0;
	static u_int8_t nextboot = 0xff;
	/* CPLD scratch pad reg 1 stores nextboot information */
	volatile u_int8_t *p_nextboot = (u_int8_t *)(CFG_TBBCPLD_BASE + 
                                              BOOTCPLD_TBB_RE_SCRATCH_1_ADDR);

	nextboot = *p_nextboot;
	if (nextboot == 0) {
		/* if default power on is 0, overwrite default value to 0x6f 
                 * i.e. All devices in mask, External usb is default boot device */
		nextboot = 0x6f; 
		*p_nextboot = nextboot;
	}

	if (nextboot & LAN_BOOTMASK) {
		/* remove lan boot options */
		nextboot &= ~LAN_BOOTMASK;
		*p_nextboot = nextboot;
	}

	if (is_external_usb_present(&dev)) {
		if ((nextboot & USB_BOOTMASK) && 
		    (NEXTBOOT_B2V(nextboot) == USB_BOOT)) {
			set_nextboot_device(dev, 0);
			return;
		} else {
			if (!(nextboot & USB_BOOTMASK) &&
			    (NEXTBOOT_B2V(nextboot) == USB_BOOT)) {
				/* if usb taken out from bootdev mask, move to flash0 */
				nextboot = (nextboot & 0x1f) | (NAND_FLASH_0_BOOT << 5);
				*p_nextboot = nextboot;
			}
		}
	} else if (NEXTBOOT_B2V(nextboot) == USB_BOOT) {
		/* move next boot to flash#0 in case no USB devices */
		nextboot = (nextboot & 0x1f) | (NAND_FLASH_0_BOOT << 5);
		*p_nextboot = nextboot;
	}

	/* both flash#0 and flash#1 are present */
        if ((nextboot & NAND_FLASH_0_BOOTMASK) && 
                (NEXTBOOT_B2V(nextboot) == NAND_FLASH_0_BOOT)) {
                set_nextboot_device(0, 0);
                return;
        } else {
                if (!(nextboot & NAND_FLASH_0_BOOTMASK) &&
                        (NEXTBOOT_B2V(nextboot) == NAND_FLASH_0_BOOT)) {
                        /* if flash#0 taken out from bootdev, move to flash#2 */
                        nextboot = (nextboot & 0x1f) | (NAND_FLASH_1_BOOT << 5);
                        *p_nextboot = nextboot;
                }
        }

        if (NEXTBOOT_B2V(nextboot) == NAND_FLASH_1_BOOT) {
                set_nextboot_device(1, 0);
                return;
        }

        if (NEXTBOOT_B2V(nextboot) == LAN_BOOT) {
                set_nextboot_device(0, 1);
        }

        return;
}

int mx80_disk_scan(int dev)
{
	static u_int8_t nextboot = 0xff;
	static int first_time_enter = 1;
	/* CPLD scratch pad reg 1 stores nextboot information */
	volatile u_int8_t *p_nextboot = (u_int8_t *)(CFG_TBBCPLD_BASE + 
                                              BOOTCPLD_TBB_RE_SCRATCH_1_ADDR);

	if (first_time_enter) {
		nextboot = *p_nextboot;
		printf("Will try to boot from\n");
		if ((nextboot == 0) || (nextboot & USB_BOOTMASK)) {
			printf("USB\n");
		}
		if ((nextboot == 0) || (nextboot & NAND_FLASH_0_BOOTMASK)) {
			printf("nand-flash0\n");
		}
		if ((nextboot == 0) || (nextboot & NAND_FLASH_1_BOOTMASK)) {
			printf("nand-flash1\n");
		}
		if ((nextboot == 0) || (nextboot & LAN_BOOTMASK)) {
			printf("Network\n");
		}
		first_time_enter = 0;

		/* stop watchdog tickle by timer interrupt */
		watchdog_tickle_override = 1;
	}

	if (NEXTBOOT_B2V(nextboot) == LAN_BOOT) {
		printf("Trying to boot from Network\n");
		*p_nextboot = (nextboot & 0x1f) | (USB_BOOT << 5);
	}

	return (usb_disk_scan(dev));
}

int mx80_disk_read(int dev, int lba, int totcnt, char *buf)
{
	static u_int8_t nextboot = 0xff;
	static int first_time_enter = 1;
	/* CPLD scratch pad reg 1 stores nextboot information */
	volatile u_int8_t *p_nextboot = (u_int8_t *)(CFG_TBBCPLD_BASE + 
                                              BOOTCPLD_TBB_RE_SCRATCH_1_ADDR);

	if (first_time_enter) {
		nextboot = *p_nextboot;
		switch (NEXTBOOT_B2V(nextboot)) {
		case USB_BOOT:
			printf("Trying to boot from USB\n");
			*p_nextboot = (nextboot & 0x1f) | (NAND_FLASH_0_BOOT << 5);
			break;

		case NAND_FLASH_0_BOOT:
			printf("Trying to boot from nand-flash0\n");
			*p_nextboot = (nextboot & 0x1f) | (NAND_FLASH_1_BOOT << 5);
			break;

		case NAND_FLASH_1_BOOT:
			printf("Trying to boot from nand-flash1\n");
			*p_nextboot = (nextboot & 0x1f) | (USB_BOOT << 5);
			break;
		}
		first_time_enter = 0;
	}

	/* tickle watchdog explicitly till we are reading usb media */
	tbbcpld_watchdog_tickle();
	return (usb_disk_read(dev, lba, totcnt, buf));
}

#if defined(CONFIG_DDR_ECC) && !defined(CONFIG_ECC_INIT_VIA_DDRCONTROLLER)
extern void ddr_enable_ecc(unsigned int dram_size);
#endif

extern int post_result_cpu;
extern int post_result_mem;
extern int post_result_rom;

extern long int spd_sdram(void);

void sdram_init(void);
int testdram(void);

/*** Below post_word_store  and post_word_load  code is temporary code,
                         this will be modifed  ***/
#ifdef CONFIG_POST
DECLARE_GLOBAL_DATA_PTR;
void post_word_store(ulong a)
{
	volatile ulong  *save_addr_word =
			(volatile ulong *)(POST_STOR_WORD);
	*save_addr_word = a;
}

ulong post_word_load(void)
{
	volatile   ulong *save_addr_word =
			(volatile ulong *)(POST_STOR_WORD);
	return *save_addr_word;
}

/*
 *  Returns 1 if keys pressed to start the power-on long-running tests
 *  Called from board_init_f().
 */
int post_hotkeys_pressed(void)
{
	return 0;   /* No hotkeys supported */
}
#endif
int
post_stop_prompt(void)
{
	if (getenv("boot.nostop")) {
		return 0;
	}

	if (post_result_mem == -1 || post_result_cpu == -1 || post_result_rom == -1) {
		return 1;
	}

	return 0;
}

void hw_mx80_watchdog_reset(void)
{
	tbbcpld_system_reset();
	return ;
}

#ifdef CONFIG_HW_WATCHDOG
void hw_watchdog_reset(void)
{
	/*
	 * u-boot timer interrupt tickles watchdog periodically.
	 * When u-boot starts executing loader, we want timer 
	 * interrupt to stop tickling watchdog. Now if loader
	 * fails loading kernel image. it will fall to loader prompt.
	 * Since watchdog is not tickled anymore, it will reset
	 * the board. During next-boot loader will try to load
	 * from next available device. watchdog_tickle_override
	 * flag is set by usb scan function the first time
	 * it is executed.
	 */
	if (watchdog_tickle_override)
		return;

	tbbcpld_watchdog_tickle();
}

void hw_watchdog_enable(void)
{
	tbbcpld_watchdog_enable();
}
#endif

int board_early_init_f(void)
{
	return 0;
}

int board_id(u_int32_t board_type)
{
       return ((board_type & 0xffff0000) >> 16);
}

int version_major(u_int32_t board_type)
{
       return ((board_type & 0xff00) >> 8);
}

int version_minor(u_int32_t board_type)
{
       return (board_type & 0xff);
}

/*
 * Initialize Local Bus
 */
void local_bus_init(void)
{

}


int checkboard(void)
{
	uint32_t id = 0;
	uint16_t bid = 0;

	/*
	 * i2c controller is initialised in board_init_f->init_func_i2c()
	 * before checkboard is called.
	 */
	/* read assembly ID from main board EEPROM offset 0x4 */
	tbbcpld_eeprom_enable();
	i2c_read(CFG_I2C_EEPROM_ADDR, 4, 1, (uint8_t *)&id, 4);

	bid = board_id(id);

	switch (bid) {
	case I2C_ID_MX5_T:
		puts("Board: MX5-T");
		break;
	case I2C_ID_MX10_T:
		puts("Board: MX10-T");
		break;
	case I2C_ID_MX40_T:
		puts("Board: MX40-T");
		break;
	case I2C_ID_MX80:
		puts("Board: MX80");
		break;
	case I2C_ID_MX80_P:
		puts("Board: MX80-P");
		break;
	case I2C_ID_MX80_T:
		puts("Board: MX80-T");
		break;
	case I2C_ID_MX80_48T:
		puts("Board: MX80-48T");
		break;
	default:
		puts("Board: MX80 Default");
		break;
	}

	printf(" %d.%d\n", version_major(id), version_minor(id));
	printf("CPLD:  Version 0x%2x \n", tbbcpld_version());

	local_bus_init();
	tbbcpld_init();

	return 0;
}

#ifdef DDR_CTRL_DEBUG
#define CLK_ADJ_MIN		0
#define CLK_ADJ_MAX		8
#define CPO_MIN			0x02
#define CPO_MAX			0x0E
#define WR_DATA_DELAY_MIN	0
#define WR_DATA_DELAY_MAX	6

static void mx80_scratchpad_update(void)
{
	u_int8_t scratch1, scratch2;
	u_int8_t clk_adj, cpo, wr_delay;

	tbbcpld_scratchpad_read(&scratch1, &scratch2);

	clk_adj  = DDR_GET_CLK_ADJ(scratch1);
	wr_delay = DDR_GET_WR_DATA_DELAY(scratch1);
	cpo      = DDR_GET_CPO(scratch2);

	/*
	 * both values are zero.
	 * Set all values to minimum.
	 */
	if (!clk_adj && !cpo && !wr_delay){

	    DDR_SET_CLK_ADJ(scratch1, CLK_ADJ_MIN);
	    DDR_SET_WR_DATA_DELAY(scratch1, WR_DATA_DELAY_MIN);
	    DDR_SET_CPO(scratch2, CPO_MIN);

	    tbbcpld_scratchpad_write(scratch1, scratch2);
	    return;
	}

	tbbcpld_watchdog_tickle();
	scratch1++;
	if (scratch1 == 0x80) {

	    	scratch1=0x00;
	        DDR_SET_CLK_ADJ(scratch1, CLK_ADJ_MIN);
	        DDR_SET_WR_DATA_DELAY(scratch1, WR_DATA_DELAY_MIN);
		scratch2++;

		if (DDR_GET_CPO(scratch2) > CPO_MAX) {
		    tbbcpld_watchdog_disable();
		    while(1);
		}
	}
	tbbcpld_scratchpad_write(scratch1, scratch2);

}
#endif

long int initdram(int board_type)
{
	long dram_size = 0;

	puts("Initializing ");

#ifdef DDR_CTRL_DEBUG
	u_int8_t scratch1, scratch2;

	mx80_scratchpad_update();

	tbbcpld_scratchpad_read(&scratch1, &scratch2);
	printf("\n*****************************************************\n");
	printf("CPO = 0x%x, WR_DATA_DELAY = 0x%x, CLK_ADJ = 0x%x \n",
		DDR_GET_CPO(scratch2), DDR_GET_WR_DATA_DELAY(scratch1),
		DDR_GET_CLK_ADJ(scratch1));
	printf("*******************************************************\n");
	tbbcpld_watchdog_enable();
#endif

	dram_size = spd_sdram();

#ifdef DDR_CTRL_DEBUG
	gd->ram_size = dram_size;
	if (testdram() == 0) {
		printf("\n*****************************************************\n");
		printf("SUCCESS: CPO = 0x%x, WR_DATA_DELAY = 0x%x, CLK_ADJ = 0x%x \n",
			DDR_GET_CPO(scratch2), DDR_GET_WR_DATA_DELAY(scratch1),
			DDR_GET_CLK_ADJ(scratch1));
		printf("*******************************************************\n");
	}
	while(1);
#endif

#if defined(CFG_RAMBOOT)
	puts(" DDR: ");
	return dram_size;
#endif

#if defined(CONFIG_DDR_ECC) && !defined(CONFIG_ECC_INIT_VIA_DDRCONTROLLER)
	/*
	 * Initialize and enable DDR ECC.
	 */
	ddr_enable_ecc(dram_size);
	puts("Done ddr_enable_ecc() ....");
	puts("\n");
#endif
	puts("    DDR: ");
	return dram_size;
}

#if defined(CFG_DRAM_TEST)
int
testdram(void)
{
	uint *pstart = (uint *) CFG_MEMTEST_START;
	uint *pend = (uint *) CFG_MEMTEST_END;
	uint *p;

	printf("Testing DRAM from 0x%08x to 0x%08x\n",
		   CFG_MEMTEST_START,
		   CFG_MEMTEST_END);

	printf("DRAM test phase 1:\n");
	for (p = pstart; p < pend; p++)
		*p = 0xaaaaaaaa;

	for (p = pstart; p < pend; p++) {
		if (*p != 0xaaaaaaaa) {
			printf ("DRAM test fails at: %08x\n", (uint) p);
			return 1;
		}
	}

	printf("DRAM test phase 2:\n");
	for (p = pstart; p < pend; p++)
		*p = 0x55555555;

	for (p = pstart; p < pend; p++) {
		if (*p != 0x55555555) {
			printf ("DRAM test fails at: %08x\n", (uint) p);
			return 1;
		}
	}

	printf("DRAM test passed.\n");
	return 0;
}
#endif


int last_stage_init(void)
{
	uchar *namestr = NULL;
	char temp[60];
	uchar enet[6];
	uint8_t esize = 0;
	uint8_t i;
	char *s, *e;
	u_int32_t id = 0;
	bd_t *bd;

	bd = gd->bd;

	/*
	 * Read a valid MAC address and set "ethaddr" environment variable
	 */
	i2c_read(CFG_I2C_EEPROM_ADDR, CFG_EEPROM_MAC_OFFSET, 1, enet, 6);
	i2c_read(CFG_I2C_EEPROM_ADDR, CFG_EEPROM_MAC_OFFSET-1, 1, &esize, 1);

	if ((esize == 0x20) || (esize == 0x40)) {
		/* 
		* As per the specification, the management interface is assigned the
		* last MAC address out of the range programmed in the EEPROM. U-Boot
		* was using the first in the range and this can cause problems when
		* both U-Boot and JUNOS generate network traffic in short order. The
		* switch to which the management interface is connected is exposed to
		* a sudden MAC address change for the same IP in that case and may
		* not forward packets correctly at first. This can cause timeouts and
		* thus cause user-noticable failures.
		*
		* Using the same algorithm from JUNOS to guarantee the same MAC
		* address picked for tsec0 in U-boot.
		*/
		enet[5] += (esize - 1);
    }

	sprintf(temp, "%02x:%02x:%02x:%02x:%02x:%02x",
			enet[0], enet[1], enet[2], enet[3], enet[4], enet[5]);
	s = getenv("eth2addr");
	if (memcmp (s, temp, sizeof(temp)) != 0) {
		setenv("eth2addr", temp);
	}

	/* read assembly ID from main board EEPROM offset 0x4 */
	i2c_read(CFG_I2C_EEPROM_ADDR, 4, 1, (uint8_t *)&id, 4);

	/* environment variable for board type */
        switch (board_id(id)) {
            case I2C_ID_MX5_T:      
                 namestr = MX5_T_NAME_STRING;      
                 break;
            case I2C_ID_MX10_T:     
                 namestr = MX10_T_NAME_STRING;     
                 break;
            case I2C_ID_MX40_T:    
                 namestr = MX40_T_NAME_STRING;     
                 break;
            case I2C_ID_MX80_T:
                 namestr = MX80_T_NAME_STRING;     
                 break;
            case I2C_ID_MX80_P:
                 namestr = MX80_P_NAME_STRING;     
                 break;
            case I2C_ID_MX80:
                 namestr = MX80_NAME_STRING;     
                 break;
            case I2C_ID_MX80_48T: 
                 namestr = MX80_48T_NAME_STRING; 
                 break;
            default:              
                 namestr = MX80_48T_NAME_STRING; 
                 break;
        }

	sprintf(temp,"%s", namestr);
	setenv("hw.board.type", temp);

	/*
	 * set eth active device as eTSEC3
	 */
	setenv("ethact", CONFIG_MPC85XX_TSEC3_NAME);
	
	/*
	 * set bd->bi_enetaddr to active device's MAC (eTSEC3).
	 */
	s = getenv ("eth2addr");
	for (i = 0; i < 6; ++i) {
		bd->bi_enetaddr[i] = s ? simple_strtoul (s, &e, 16) : 0;
		if (s)
			s = (*e) ? e + 1 : e;
	}

	/*
	 * Set bootp_if as fxp0 (eTSEC3) for MX80.
	 * This environment variable is used by JUNOS install scripts
	 */
	setenv("bootp_if", CONFIG_MPC85XX_TSEC3_NAME);

	/*
	 * Set upgrade environment variables
	 */
    sprintf(temp,"0x%08x ", CFG_UPGRADE_LOADER_ADDR);
    setenv("boot.upgrade.loader", temp);
    
    sprintf(temp,"0x%08x ", CFG_UPGRADE_BOOT_IMG_ADDR);
    setenv("boot.upgrade.uboot", temp);

	/* set boot device according to nextboot */
	set_boot_disk();

    return 0;
}

int board_early_init_r(void)
{
	unsigned int i;
	debug("board_early_init_r -- remap TLB for Boot Flash + PROMJET as "
		"caching-inhibited\n");
	for (i = 0; i < 8*1024*1024; i += 32) {
		asm volatile ("dcbi %0,%1": : "b" (CFG_FLASH_BASE), "r" (i));
		asm volatile ("icbi %0,%1": : "b" (CFG_FLASH_BASE), "r" (i));
	}

	/* ***** - local bus: Boot flash + PROMJET  (256 MB)  invalidate */
	mtspr(MAS0, TLB1_MAS0(1, 2, 0)); /* tlbsel=1 esel=2 nv=0 */
	mtspr(MAS1, TLB1_MAS1(0, 1, 0, 0, BOOKE_PAGESZ_4M));	/* V=0 */
	mtspr(MAS2, TLB1_MAS2(E500_TLB_EPN(CFG_FLASH_BASE),
				0, 0, 0, 0, 0, 0, 0, 0));
	mtspr(MAS3, TLB1_MAS3(E500_TLB_RPN(CFG_FLASH_BASE),
				0, 0, 0, 0, 0, 1, 0, 1, 0, 1));
	asm volatile("isync;msync;tlbwe;isync");

	mtspr(MAS0, TLB1_MAS0(1, 3, 0)); /* tlbsel=1 esel=3 nv=0 */
	mtspr(MAS1, TLB1_MAS1(0, 1, 0, 0, BOOKE_PAGESZ_4M));	/* V=0 */
	mtspr(MAS2, TLB1_MAS2(E500_TLB_EPN(CFG_FLASH_BASE + 4 * 1024 * 1024),
				0, 0, 0, 0, 0, 0, 0, 0));
	mtspr(MAS3, TLB1_MAS3(E500_TLB_RPN(CFG_FLASH_BASE + 4 * 1024 * 1024),
				0, 0, 0, 0, 0, 1, 0, 1, 0, 1));
	asm volatile("isync;msync;tlbwe;isync");


	/* *I*G* - local bus: Boot flash + PROMJET  (256 MB) */
	mtspr(MAS0, TLB1_MAS0(1, 2, 0)); /* tlbsel=1 esel=2 nv=0 */
	mtspr(MAS1, TLB1_MAS1(1, 1, 0, 0, BOOKE_PAGESZ_4M));	/* V=1 */
	mtspr(MAS2, TLB1_MAS2(E500_TLB_EPN(CFG_FLASH_BASE),
				0, 0, 0, 0, 1, 0, 1, 0));/* *I*G* */
	mtspr(MAS3, TLB1_MAS3(E500_TLB_RPN(CFG_FLASH_BASE),
				0, 0, 0, 0, 0, 1, 0, 1, 0, 1));
	asm volatile("isync;msync;tlbwe;isync");

	mtspr(MAS0, TLB1_MAS0(1, 3, 0)); /* tlbsel=1 esel=3 nv=0 */
	mtspr(MAS1, TLB1_MAS1(1, 1, 0, 0, BOOKE_PAGESZ_4M));	/* V=1 */
	mtspr(MAS2, TLB1_MAS2(E500_TLB_EPN(CFG_FLASH_BASE + 4 * 1024 * 1024),
				0, 0, 0, 0, 1, 0, 1, 0));/* *I*G* */
	mtspr(MAS3, TLB1_MAS3(E500_TLB_RPN(CFG_FLASH_BASE + 4 * 1024 * 1024),
				0, 0, 0, 0, 0, 1, 0, 1, 0, 1));
	asm volatile("isync;msync;tlbwe;isync");

	printf("Enable CPLD Watchdog\n");
	hw_watchdog_enable();
	
	/* Kick WDT */
	WATCHDOG_RESET();
	
	return 0;
}

unsigned long get_board_sys_clk(ulong dummy)
{
	return gd->bus_clk;
}

unsigned long get_board_ddr_clk(ulong dummy)
{
	return gd->ddr_clk;
}

typedef struct {
	unsigned short datarate_mhz_low;
	unsigned short datarate_mhz_high;
	unsigned char nranks;
	unsigned char clk_adjust;
	unsigned char cpo;
	unsigned char write_data_delay;
	unsigned char force_2T;
} board_specific_parameters_t;



/* ranges for parameters:
   wr_data_delay = 0-6
   clk adjust = 0-8
   cpo 2-0x1E (30)
 */



/* XXX: these values need to be checked for all interleaving modes.  */
/* XXX: No reliable dual-rank 800 MHz setting has been found.  It may
 *      seem reliable, but errors will appear when memory intensive
 *      program is run. */
/* XXX: Single rank at 800 MHz is OK.  */
const board_specific_parameters_t board_specific_parameters[][20] = {
	{	/* memory controller 0 */
	/*        lo|  hi|  num|  clk| cpo|wrdata|2T*/
	/*       mhz| mhz|ranks|adjst|    | delay|  */
		{  0, 333,    2,    6,   7,     3,  0},
		{334, 400,    2,    6,   9,     3,  0},
		{401, 549,    2,    7,   7,     3,  0},
		{550, 680,    2,    1,  10,     5,  0},
		{681, 850,    2,    1,  12,     5,  1}, /* 2T=0 ok? */

		{  0, 333,    1,    6,   7,     3,  0},
		{334, 400,    1,    6,   9,     3,  0},

		{401, 549,    1,    4,   6,     2,  0}, /* MX80: Cntrl#0 */
		{550, 680,    1,    1,  10,     5,  0},
		{681, 850,    1,    1,  12,     5,  0}
	},

	{	 /* memory controller 1 */
	/*        lo|  hi|  num|  clk| cpo|wrdata|2T*/
	/*       mhz| mhz|ranks|adjst|    | delay|  */
		{  0, 333,    2,     6,  7,     3,  0},
		{334, 400,    2,     6,  9,     3,  0},
		{401, 549,    1,    4,   6,     2,  0}, /* MX80: Cntrl#1 */
		{550, 680,    2,     1, 11,     6,  0},
		{681, 850,    2,     1, 13,     6,  1},

		{  0, 333,    1,     6,  7,     3,  0},
		{334, 400,    1,     6,  9,     3,  0},
		{401, 549,    1,     1, 11,     3,  0},
		{550, 680,    1,     1, 11,     6,  0},
		{681, 850,    1,     1, 13,     6,  0}
	}
};


unsigned char compute_clk_adjust(
	unsigned int memctl,
	unsigned int nranks,
	unsigned int nchips_per_rank,
	unsigned int datarate_mhz)
{
#ifdef DDR_CTRL_DEBUG
        u_int8_t scratch;
	if (memctl == 0) {
		tbbcpld_reg_read(BOOTCPLD_TBB_RE_SCRATCH_1_ADDR, &scratch);
    		return (DDR_GET_CLK_ADJ(scratch));
	} else {
	    return 4;
	}
#else
	const board_specific_parameters_t *pbsp =
		&(board_specific_parameters[memctl][0]);
	unsigned int num_params = sizeof(board_specific_parameters[memctl]) /
		sizeof(board_specific_parameters[0][0]);
	unsigned int i;
	for (i = 0; i < num_params; i++) {
		if (datarate_mhz >= pbsp->datarate_mhz_low &&
			datarate_mhz <= pbsp->datarate_mhz_high &&
			nranks == pbsp->nranks)
			return pbsp->clk_adjust;
		pbsp++;
	}
	debug("compute_clk_adjust:  entry not found, using algorithm\n");

	if (datarate_mhz > 533)
		return 1;
	else
		return 6;
#endif

}

unsigned char compute_write_data_delay(
	unsigned int memctl,
	unsigned int nranks,
	unsigned int nchips_per_rank,
	unsigned int datarate_mhz)
{
#ifdef DDR_CTRL_DEBUG
        u_int8_t scratch;
	if (memctl == 0) {
		tbbcpld_reg_read(BOOTCPLD_TBB_RE_SCRATCH_1_ADDR, &scratch);
    		return (DDR_GET_WR_DATA_DELAY(scratch));
	} else {
	    return 2;
	}
#else
	const board_specific_parameters_t *pbsp =
		&(board_specific_parameters[memctl][0]);
	unsigned int num_params = sizeof(board_specific_parameters[memctl]) /
		sizeof(board_specific_parameters[0][0]);
	unsigned int i;
	debug("num_params = %u, memctl = %u, nranks = %u, datarate_mhz = %u\n",
		num_params, memctl, nranks, datarate_mhz);
	for (i = 0; i < num_params; i++) {
		if (datarate_mhz >= pbsp->datarate_mhz_low &&
			datarate_mhz <= pbsp->datarate_mhz_high &&
			nranks == pbsp->nranks)
			return pbsp->write_data_delay;
		pbsp++;
	}
	debug("compute_write_data_delay:  entry not found, using algorithm\n");

	if (datarate_mhz <= 533)
		return 3;
	else
		if (memctl)
			return 6;
		else
			return 5;
#endif
}


unsigned char compute_cpo(
	unsigned int memctl,
	unsigned int nranks,
	unsigned int nchips_per_rank,
	unsigned int datarate_mhz)
{
#ifdef DDR_CTRL_DEBUG
        u_int8_t scratch;
	if (memctl == 0) {
		tbbcpld_reg_read(BOOTCPLD_TBB_RE_SCRATCH_2_ADDR, &scratch);
    		return (DDR_GET_CPO(scratch));
	} else {
	    return 6;
	}
#else
	const board_specific_parameters_t *pbsp =
		&(board_specific_parameters[memctl][0]);
	unsigned int num_params = sizeof(board_specific_parameters[memctl]) /
		sizeof(board_specific_parameters[0][0]);
	unsigned int i;
	unsigned int cpo;

	for (i = 0; i < num_params; i++) {
		if (datarate_mhz >= pbsp->datarate_mhz_low &&
			datarate_mhz <= pbsp->datarate_mhz_high &&
			nranks == pbsp->nranks)
			return pbsp->cpo;
		pbsp++;
	}

	debug("compute_cpo:  entry not found, resorting to algorithm\n");

	/* Note:  This was the old code, but it may not be correct. */
	if (datarate_mhz <= 333)
		cpo = 0x7;
	else if (datarate_mhz <= 400)
		cpo = 0x9;
	else if (datarate_mhz <= 533)
		cpo = 0xb;
	else if (datarate_mhz <= 667)
	    if (memctl == 0)
		cpo = 0xa;
	    else
		cpo = 0xb;
	else
	    if (memctl == 0)
		cpo = 0xc;
	    else
		cpo = 0xd;

	return cpo;
#endif
}


unsigned char compute_2T(
	unsigned int memctl,
	unsigned int nranks,
	unsigned int nchips_per_rank,
	unsigned int datarate_mhz)
{
	const board_specific_parameters_t *pbsp =
		&(board_specific_parameters[memctl][0]);
	unsigned int num_params = sizeof(board_specific_parameters[memctl]) /
		sizeof(board_specific_parameters[0][0]);
	unsigned int i;

	for (i = 0; i < num_params; i++) {
		if (datarate_mhz >= pbsp->datarate_mhz_low &&
			datarate_mhz <= pbsp->datarate_mhz_high &&
			nranks == pbsp->nranks)
			return pbsp->force_2T;
		pbsp++;
	}

	debug("compute_2T:  entry not found, forcing 2T_en=1\n");

	return 1;
}
