/*
 * Copyright (c) 2007-2012, Juniper Networks, Inc.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <watchdog.h>
#include <pci.h>
#include <asm/processor.h>
#include <asm/immap_85xx.h>
#include <spd.h>
#include <miiphy.h>
#include "tsec.h"
#include <spd_sdram.h>
#include <i2c.h>
#include <usb.h>
#include "eeprom.h"
#include "fsl_i2c.h"
#include "recb_iic.h"
#include "recb.h"
#include <platform_srx3000.h>

#if defined(CONFIG_DDR_ECC) && !defined(CONFIG_ECC_INIT_VIA_DDRCONTROLLER)
extern void ddr_enable_ecc(unsigned int dram_size);
#endif

#define LAN_BOOT                        0
#define DISK_BOOT                       1
#define FLASH_BOOT                      2
#define USB_BOOT                        3
#define LAN_BOOTMASK                    (1 << LAN_BOOT)
#define DISK_BOOTMASK                   (1 << DISK_BOOT)
#define FLASH_BOOTMASK                  (1 << FLASH_BOOT)
#define USB_BOOTMASK                    (1 << USB_BOOT)
#define NEXTBOOT_B2V(x)                 ((x & 0xe0) >> 5)

#define SSRXB_I2C_GROUP_SELECT_LOCAL_RE         0x18
#define SSRXB_I2C_GROUP_SELECT_REMOTE_RE        0x19
#define SSRXB_I2C_GROUP_SELECT_FABIO            0x0F
#define SSRXB_I2C_GROUP_SELECT_SYSIO            0x0F

#define RECB_IDEEPROM                           0x51 /* RECB IDEEPROM */
#define SSRXB_SLAVE_CPLD_ADDR                   0x54
#define EEPROM_MAINBOARD                        0x57
#define I2CS_CTRL_ADDR                          0x12
#define I2CS_FABIO_CTRL_ADDR_0                  0x7E
#define I2CS_FABIO_CTRL_ADDR_1                  0x7D

#define I2CS_RECB_LED_BLINK_CTRL_VALUE          0x44
#define I2CS_FABIO_PWR_LED_BLINK_CTRL_VALUE     0x10
#define I2CS_RE0_FABIO_LED_GREEN_CTRL_VALUE     0x01
#define I2CS_RE0_FABIO_LED_BLINK_CTRL_VALUE     0x40
#define I2CS_RE1_FABIO_LED_GREEN_CTRL_VALUE     0x20
#define I2CS_RE1_FABIO_LED_BLINK_CTRL_VALUE     0x20
#define I2CS_SYSIO_HA_MUX_REG                   0x44
#define BYTE_LENGTH                             4
#define TX_DATA                                 128

#define PLL_RATIO_STATUS_REG    0x00c6400c
#define BOOT_MODE_STATUS_REG    0x85370000
#define IMP_STATUS_REG          0x0003007f
#define DEV_STATUS_REG          0x829fea67
#define DEB_MODE_REG            0x0f000000
#define POR_REG                 0x00001940
#define DEV_CTRL_REG            0x50000000
#define DEV_CTRL_REG_REV_II     0x51000000
#define A1_CHASSIS_ID           0x1

extern int post_result_cpu;
extern int post_result_mem;
extern int post_result_rom;
extern int post_result_ether;
extern int post_result_pci;

/*
 *  * Exported functions to support USB storage
 *   */

extern block_dev_desc_t usb_dev_desc[USB_MAX_STOR_DEV];
extern int usb_max_devs; /* number of highest available usb device */
extern char usb_started;
extern unsigned long usb_stor_read(int device, unsigned long blknr, unsigned long blkcnt, unsigned long *buffer);
extern block_dev_desc_t * sata_get_dev(int dev);


extern int usb_stor_curr_dev; /* current device */

static void local_bus_init(void);
static void parkes_init(void);
static void hw_watchdog_enable(void);
static void config_ha(void);
static void hdre_cntl_reset (uint8_t);
static int check_cb1_is_crm(void);

void mgmt_i2cphy_setup(void);
int mgmt_i2cphy_write(unsigned char reg, unsigned short value);
int mgmt_i2cphy_read(unsigned char reg, unsigned short *value);
int i2cphy_read(char *devname, unsigned char addr, unsigned char reg, unsigned short *value);
int i2cphy_write(char *devname, unsigned char addr, unsigned char reg, unsigned short value);
static recb_fpga_regs_t *global_recb_p = 0;

DECLARE_GLOBAL_DATA_PTR;
#define KERN_MEM_RESERVED 0x100000

void 
get_processor_ver(uint *major , uint *minor)
{
	uint svr = get_svr();
	*major = SVR_MAJ(svr);
	*minor = SVR_MIN(svr);
}
        
static void
set_nextboot_device(int dev, int is_netboot)
{
	char dev_str[32];
	if (is_netboot) {
		sprintf(dev_str, "net%d:", dev);
	} else {
		sprintf(dev_str, "disk%d:", dev);
	}
	setenv("loaddev", dev_str);
}

static void
set_boot_disk (void)
{
	int dev = 0;
	static u_int32_t nextboot = 0xff;
	block_dev_desc_t *stor_dev = NULL;
	volatile u_int32_t *p_nextboot = (u_int32_t *)(CFG_PARKES_BASE + 0x150);

	nextboot = *p_nextboot;
	if (nextboot == 0) {
		/* if default power on is 0, overwrite default value to 0x6f */
		nextboot = 0x6f;
		*p_nextboot = nextboot;
	}

	if (nextboot & LAN_BOOTMASK) {
		/* remove lan boot options */
		nextboot &= ~LAN_BOOTMASK;
		*p_nextboot = nextboot;
	}

	if (usb_dev_desc[dev].target != 0xff) {
		if ((nextboot & USB_BOOTMASK) && 
		    (NEXTBOOT_B2V(nextboot) == USB_BOOT)) {
			set_nextboot_device(dev, 0);
			return;
		} else {
			if (!(nextboot & USB_BOOTMASK) &&
			    (NEXTBOOT_B2V(nextboot) == USB_BOOT)) {
				/* if usb taken out from bootdev, move to flash */
				nextboot = (nextboot & 0x1f) | (FLASH_BOOT << 5);
				*p_nextboot = nextboot;
			}
			while (usb_dev_desc[++dev].target != 0xff);
		}
	} else if (NEXTBOOT_B2V(nextboot) == USB_BOOT) {
		/* move next boot to flash in case no USB devices */
		nextboot = (nextboot & 0x1f) | (FLASH_BOOT << 5);
		*p_nextboot = nextboot;
	}

	/* check for flash */
	stor_dev = sata_get_dev(1);

	if(stor_dev->target != 0xff) {
		/* both flash and disk are present */
		if ((nextboot & FLASH_BOOTMASK) && 
		    (NEXTBOOT_B2V(nextboot) == FLASH_BOOT)) {
			set_nextboot_device(dev, 0);
			return;
		} else {
			if (!(nextboot & FLASH_BOOTMASK) &&
			    (NEXTBOOT_B2V(nextboot) == FLASH_BOOT)) {
				/* if flash taken out from bootdeva, move to disk */
				nextboot = (nextboot & 0x1f) | (DISK_BOOT << 5);
				*p_nextboot = nextboot;
			}
			dev++;
		}

		if (NEXTBOOT_B2V(nextboot) == DISK_BOOT) {
			set_nextboot_device(dev, 0);
			return;
		}
	} else {
		/* only flash or disk is present */
		stor_dev = sata_get_dev(0);
		if (stor_dev->target == 1) {
			/* only flash present */
			if (NEXTBOOT_B2V(nextboot) == FLASH_BOOT) {
				set_nextboot_device(dev, 0);
				return;
			}

			if (NEXTBOOT_B2V(nextboot) == DISK_BOOT) {
				/* move next boot to flash in case no USB devices */
				nextboot = (nextboot & 0x1f) | (LAN_BOOT << 5);
				*p_nextboot = nextboot;
			}
		} else {
			/* only hard disk present */
			if (NEXTBOOT_B2V(nextboot) == FLASH_BOOT) {
				/* move next boot to flash in case no USB devices */
				nextboot = (nextboot & 0x1f) | (DISK_BOOT << 5);
				*p_nextboot = nextboot;
			}

			if (NEXTBOOT_B2V(nextboot) == DISK_BOOT) {
				set_nextboot_device(dev, 0);
				return;
			}
		}
	}

	if (NEXTBOOT_B2V(nextboot) == LAN_BOOT) {
		set_nextboot_device(0, 1);
	}
	
}

int board_early_init_f (void)
{
	return 0;
}

int checkboard (void)
{
	volatile immap_t *immap = (immap_t *) CFG_CCSRBAR;
	volatile ccsr_gur_t *gur = &immap->im_gur;
    /*PARKES_FPGA_BOARD_STATUS 0x8 */    
	volatile u_int32_t   *parkes_bs = (u_int32_t *)(CFG_PARKES_BASE + 0x8);
	volatile u_int32_t   *reset_his = (u_int32_t *)(CFG_PARKES_BASE + 0x10);
	volatile u_int32_t   *uart_mux = (u_int32_t *)(CFG_PARKES_BASE + 0xc0);
	volatile u_int32_t   *parkes_ver = (u_int32_t *)(CFG_PARKES_BASE + 0x0);
	volatile u_int32_t   *def_wd = (u_int32_t *)(CFG_PARKES_BASE + 0x18);
	volatile u_int32_t  *wdt_reg = (u_int32_t *)(CFG_PARKES_BASE + 0x18);
	volatile u_int32_t  *wdt_timer_reg = (u_int32_t *)(CFG_PARKES_BASE + 0x20);
	volatile u_int32_t  *parkes_reset_t = (u_int32_t *)(CFG_PARKES_BASE + 0x0C);
	volatile u_int32_t   *parkes_reset_ctrl = (u_int32_t *)(CFG_PARKES_BASE + 0x14);
	uint major, minor; 
	  
	/* PCI slot in USER bits CSR[6:7] by convention. */
	uint pci_slot = get_pci_slot ();
	uint pci_dual = get_pci_dual ();            /* PCI DUAL in CM_PCI[3] */
	uint pci1_32 = gur->pordevsr & 0x10000;     /* PORDEVSR[15] */
	uint pci1_clk_sel = gur->porpllsr & 0x8000; /* PORPLLSR[16] */
	uint pci2_clk_sel = gur->porpllsr & 0x4000; /* PORPLLSR[17] */
	uint pci1_speed = get_clock_freq ();        /* PCI PSPEED in [4:5] */
	uint cpu_board_rev = get_cpu_board_revision ();

	printf ("Board: CDS Version 0x%02x, PCI Slot %d\n",
		get_board_version (), pci_slot);

	printf ("CPU Board Revision %d.%d (0x%04x)\n",
		CPU_BOARD_MAJOR (cpu_board_rev),
		CPU_BOARD_MINOR (cpu_board_rev), cpu_board_rev);

	printf ("    PCI1: %d bit, %s MHz, %s\n",
		(pci1_32) ? 32 : 64,
		(pci1_speed == 33000000) ? "33" :
		(pci1_speed == 66000000) ? "66" : "unknown",
		pci1_clk_sel ? "sync" : "async");

	if (pci_dual) {
		printf ("    PCI2: 32 bit, 66 MHz, %s\n",
			pci2_clk_sel ? "sync" : "async");
	} else {
		printf ("    PCI2: disabled\n");
	}

	printf("Parkes Version : 0x%08x\n", *parkes_ver);
	printf("Parkes reset history = 0x%08x\n", *reset_his);
	printf("Parkes default uart mux = 0x%08x\n", *uart_mux);
	printf("Parkes default watchdog = 0x%08x\n", *def_wd);

	*parkes_reset_ctrl = 0x0000;
	printf("reset = 0x%x\n", *parkes_reset_ctrl);
	/* Enable Switch reset & HRESET */
	*parkes_reset_t = 0x7;
	printf("Enable the Switch reset & Hreset = 0x%x\n", *parkes_reset_t);
	/*Enable the watchdog*/
	*wdt_reg |= 0x1;
	printf("Enable the watchdog = 0x%08x\n", *wdt_reg);
	/* load watchdog timer value less then 5 minutes */
	*wdt_timer_reg = 0x19828;
	printf("Load the watchdog timer value = 0x%x\n", *wdt_timer_reg);

	/*
	 * Initialize local bus.
	 */
	local_bus_init ();

	/*
	 * Hack TSEC 3 and 4 IO voltages.
	 */
	gur->tsec34ioovcr = 0xe7e0; /*  1110 0111 1110 0xxx */
	/*
	 * Displaying the global utlity register
	 */
	if (gur->porpllsr == PLL_RATIO_STATUS_REG) {
		printf("Correct : POR - PLL ratio status register = 0x%08x\n",
				gur->porpllsr);
	} else {
		printf("Incorrect : POR - PLL ratio status register = 0x%08x\n",
				gur->porpllsr);
	}

	if (gur->porbmsr == BOOT_MODE_STATUS_REG) {
		printf("Correct : POR - Boot mode status register = 0x%08x\n",
				gur->porbmsr);
	} else {
		printf("Incorrect : POR - Boot mode status register = 0x%08x\n",
				gur->porbmsr);
	}

	if (gur->porimpscr == IMP_STATUS_REG) {
		printf("Correct : POR - I/O impedance status and control register"
				" = 0x%08x\n", gur->porimpscr);
	} else {
		printf("Incorrect : POR - I/O impedance status and control register"
				" = 0x%08x\n", gur->porimpscr);
	}

	if (gur->pordevsr == DEV_STATUS_REG) {
		printf("Correct : POR - I/O device status regsiter = 0x%08x\n",
				gur->pordevsr);
	} else {
		printf("Incorrect : POR - I/O device status regsiter = 0x%08x\n",
				gur->pordevsr);
	}
	if (gur->pordbgmsr == DEB_MODE_REG) {
		printf("Correct : POR - Debug mode status register = 0x%08x\n",
				gur->pordbgmsr);
	} else {
		printf("Incorrect : POR - Debug mode status register = 0x%08x\n",
				gur->pordbgmsr);
	}
    /*
	 * MPC8548 Product Family Device Migration from Rev 2.0.1 to 2.1.2.
	 * has fixed DDR II IO voltage biasing.If 2.0 apply the voltage settings.o
	 */
	get_processor_ver(&major, &minor);

	if (major == 2 && minor != 0) {
	    if (gur->devdisr == DEV_CTRL_REG_REV_II) {
		    printf("Correct : Device disable control register = 0x%08x\n",
				gur->devdisr);
	    } else {
		    printf("Incorrect :Device disable control register = 0x%08x\n",
			    	gur->devdisr);
	    }
    } else { 
        if (gur->devdisr == DEV_CTRL_REG) {
            printf("Correct : Device disable control register = 0x%08x\n",
		    		gur->devdisr);
	    } else {
		    printf("Incorrect :Device disable control register = 0x%08x\n",
			    	gur->devdisr);
	    }
    }        

	return 0;
}

long int
initdram (int board_type)
{
	long dram_size = 0;
	volatile immap_t *immap = (immap_t *)CFG_IMMR;

	puts("Initializing\n");

#if defined(CONFIG_DDR_DLL)
	{
		/*
		 * Work around to stabilize DDR DLL MSYNC_IN.
		 * Errata DDR9 seems to have been fixed.
		 * This is now the workaround for Errata DDR11:
		 *    Override DLL = 1, Course Adj = 1, Tap Select = 0
		 */

		volatile ccsr_gur_t *gur= &immap->im_gur;

		gur->ddrdllcr = 0x81000000;
		asm("sync;isync;msync");
		udelay(200);
	}
#endif
	dram_size = spd_sdram();

#if defined(CONFIG_DDR_ECC) && !defined(CONFIG_ECC_INIT_VIA_DDRCONTROLLER)
	/*
	 * Initialize and enable DDR ECC.
	 */
	ddr_enable_ecc(dram_size);
#endif
	/*
	 * SDRAM Initialization
	 */
	parkes_init();

	puts("    DDR: ");
	return dram_size;
}

/*
 * Initialize Local Bus
 */
void
local_bus_init (void)
{
	volatile immap_t *immap = (immap_t *)CFG_IMMR;
	volatile ccsr_gur_t *gur = &immap->im_gur;
	volatile ccsr_lbc_t *lbc = &immap->im_lbc;

	uint clkdiv;
	uint lbc_hz;
	sys_info_t sysinfo;

	get_sys_info(&sysinfo);
	clkdiv = (lbc->lcrr & 0x0f) * 2;
	lbc_hz = sysinfo.freqSystemBus / 1000000 / clkdiv;

	gur->lbiuiplldcr1 = 0x00078080;
	if (clkdiv == 16) {
		gur->lbiuiplldcr0 = 0x7c0f1bf0;
	} else if (clkdiv == 8) {
		gur->lbiuiplldcr0 = 0x6c0f1bf0;
	} else if (clkdiv == 4) {
		gur->lbiuiplldcr0 = 0x5c0f1bf0;
	}

	lbc->lcrr |= 0x00030000;

	asm("sync;isync;msync");
}

volatile u_int32_t  *wdt_reg_dis_rom = (u_int32_t *)(CFG_PARKES_BASE + 0x18);
volatile u_int32_t   *parkes_sc_pad = (u_int32_t *) (CFG_PARKES_BASE + 0x04);
#define ONEMB_OFFSET  0x100000
#define PARKES_SC_PAD_ZERO 0x00
#define PARKES_SC_PAD_MAX_ROM_VAL 6
#define PARKES_SC_PAD_ROM_TST_FAIL 3
#define I2C_TST_FAIL               1
#define DEV_STUCK                  2
#undef DEBUG
#ifdef DEBUG
#define DEBUGF(x...) printf(x)
#else
#define DEBUGF(x...)
#endif /* DEBUG */
struct boot_test_name_item {
	u_int32_t offset;
	char *description;
};
static  struct boot_test_name_item boot_test_name_tbl[] = {
	{ 1, "I2C ctrl-1 access to SPD_EEPROM"},
	{ 2, "Stuck at TAKE All Device Reset"},
	{ 3, "Memory test from ROM"},
	{ 4, "Relocate to RAM"},
	{ 5, "U-boot running in RAM"}
};
void
fpga_test_mode (void)
{
	/* Check why board got reset */
	u_int32_t readback;
	if (*parkes_sc_pad != PARKES_SC_PAD_ZERO) {
		if (*parkes_sc_pad >= ONEMB_OFFSET) {
			printf("\n------------------------------------"
					"-------------------------------\n");
			printf("\n\nFAILED: Memory address at 0x%08x\n\n", *parkes_sc_pad);
			printf("\n-------------------------------------"
					"------------------------------\n");
			*wdt_reg_dis_rom = 0x00;
			for (; ;);
		}
		if (*parkes_sc_pad <= PARKES_SC_PAD_MAX_ROM_VAL) {
			int i = 0;
			for ( i = 0; i < (PARKES_SC_PAD_MAX_ROM_VAL - 1); i++) {
				if (*parkes_sc_pad > boot_test_name_tbl[i].offset) {
					DEBUGF("PASS: %d -  %s\n",
							boot_test_name_tbl[i].offset,
							boot_test_name_tbl[i].description);
				} else {
					if (*parkes_sc_pad  == PARKES_SC_PAD_ROM_TST_FAIL ||
							*parkes_sc_pad  == (PARKES_SC_PAD_ROM_TST_FAIL + 1)) {
						gd->memory_addr = 0xff;
						printf("\n--------------------------------"
								"-----------------------------------\n");
						printf("FAILED: %s\n", boot_test_name_tbl[i].description);
						printf("\n--------------------------------"
								"-----------------------------------\n");
						break;
					}
					if (*parkes_sc_pad  == I2C_TST_FAIL || *parkes_sc_pad == DEV_STUCK) {
						printf("\n---------------------------------"
								"----------------------------------\n");
						printf("FAILED: %s\n", boot_test_name_tbl[i].description);
						printf("\n---------------------------------"
								"----------------------------------\n");
						*wdt_reg_dis_rom = 0x00;
						for (; ;);
					}
				}
			}
		}
	}
	*parkes_sc_pad = TEST_AA_PAT;
	readback = *parkes_sc_pad;
	if (readback == TEST_AA_PAT) {
		DEBUGF("Wrote and read pattern -AA- to FPGA scratch pad reg\n");
	} else {
		printf("Failed to readback the pattern -AA- from FPGA scratch pad reg\n");
	}
	*parkes_sc_pad = TEST_55_PAT;
	readback = *parkes_sc_pad;
	if (readback == TEST_55_PAT) {
		DEBUGF("Wrote and read pattern -55- to FPGA scratch pad reg\n");
	} else {
		printf("Failed to readback the pattern -55- from FPGA scratch pad reg\n");
	}
}

/*
 * Initialize SDRAM memory on the Local Bus.
 */
void
parkes_init (void)
{
	volatile immap_t    *immap = (immap_t *)CFG_CCSRBAR;
	volatile ccsr_gur_t *gur = (ccsr_gur_t *)&immap->im_gur;
#if defined(CFG_OR1_PRELIM) && defined(CFG_BR1_PRELIM)

	volatile ccsr_lbc_t *lbc = &immap->im_lbc;

	puts("PARKES: ");

	print_size (CFG_PARKES_SIZE * 1024 * 1024, "\n");

	/*
	 * Setup PARKES Base and Option Registers
	 */
	lbc->or1 = CFG_OR1_PRELIM;
	asm("msync");

	lbc->br1 = CFG_BR1_PRELIM;
	asm("msync");

	puts("  TAKE ALL DEVICES OUT OF RESET\n");
	*parkes_sc_pad = 2;

	global_recb_p = (uint32_t *)CFG_PARKES_BASE;
	recb_fpga_regs_t *recb_p = (recb_fpga_regs_t *)global_recb_p;
	/* Added the power enable ON/OFF and HDD ON/OFF bit during init.*/
	recb_p->recb_control_register |= 0xfffff040;
	recb_p->recb_control_register &= 0xfffff1c8;
	udelay(3000000);
	recb_p->recb_control_register |= 0xfffff3c8;

#endif  /* enable SDRAM init */
	if (gur->devdisr & 0x20000000) {
		puts("FATAL error - PCIe is disabled but POR\n");
	}
}

#if defined(CFG_DRAM_TEST)
int
testdram (void)
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

#if defined(CONFIG_PCI)
/* For some reason the Tundra PCI bridge shows up on itself as a
 * different device.  Work around that by refusing to configure it.
 */
void dummy_func (struct pci_controller* hose, pci_dev_t dev, struct pci_config_table *tab) { }

static struct pci_config_table pci_jaus_config_table[] = {
	{0x10e3, 0x0513, PCI_ANY_ID, 1, 3, PCI_ANY_ID, dummy_func, {0,0,0}},
};

static struct pci_controller hose[] = {
	{ config_table: pci_jaus_config_table,},
#ifdef CONFIG_MPC85XX_PCI2
	{},
#endif
};

#endif  /* CONFIG_PCI */

void
pci_init_board (void)
{
#ifdef CONFIG_PCI
	pci_mpc85xx_init((struct pci_controller *)&hose);
#endif
}

#ifdef CONFIG_BOARD_EARLY_INIT_R
int board_early_init_r (void)
{
	hw_watchdog_enable();
	
	/* Kick WDT */
	WATCHDOG_RESET();
	
	return 0;
}
#endif

int last_stage_init (void)
{
	u_int32_t  temp;
	volatile immap_t    *immap = (immap_t *)CFG_CCSRBAR;
	volatile ccsr_gur_t *gur = (ccsr_gur_t *)&immap->im_gur;
	volatile u_int32_t   *reset_his = (u_int32_t *)(CFG_PARKES_BASE + 0x10);
	char his_str[32];
	uchar pm[8];
	int i;
	bd_t *bd = gd->bd;	
	int hdre;

	hdre = cb_is_hdre();
	/* start USB for non-HDRE */
	if (!hdre && usb_stop(0) < 0) {
		printf ("usb_stop failed\n");
		return -1;
	}
	if (!hdre && usb_init() < 0) {
		printf ("usb_init failed\n");
		return -1;
	}
	/*
	 * check whether a storage device is attached (assume that it's
	 * a USB memory stick, since nothing else should be attached).
	 */
	if (!hdre) {
		usb_stor_curr_dev = usb_stor_scan(1);
	}
	/* set the boot flag to 12 if usb init is done */
	gd->boot_up_flag = 12;

	/* Read a valid MAC address from eeprom and set to "ethaddr" env */
	i2c_controller(CFG_I2C_CTRL_1);
	i2c_read(0x57, 0x38, 1, pm, 6);
	for (i=0; i<6; i++)
		if (pm[i] != 0 && pm[i] != 0xff)
			break;
			
	if (i < 6) {
		/* if valid mac, set to "ethaddr" env */
		sprintf(his_str, "%02x:%02x:%02x:%02x:%02x:%02x", pm[0], pm[1], pm[2], pm[3], pm[4], pm[5]);
		setenv("ethaddr", his_str);
	}	    
	
	printf("POR = 0x%08x and reset history = 0x%08x\n", 
		gur->devdisr, *reset_his);

	temp = *reset_his;
	sprintf(his_str, "0x%08x", temp);
	setenv("hw.resethistory", his_str);
	*reset_his = 1;
	
	/* change the phy reg 0x1c to AN mode */
	miiphy_write(CONFIG_MPC85XX_TSEC4_NAME,
			TSEC4_PHY_ADDR, 0x1c, 0xd7ab);
	
	miiphy_read(CONFIG_MPC85XX_TSEC4_NAME,
			TSEC4_PHY_ADDR, 0x1c, &temp);

	printf(" value for reg 0x1c in phy %d = 0x%04x\n", TSEC4_PHY_ADDR, temp);
	
	if (!hdre) {
		/* set boot device according to nextboot for non-HDRE */
		set_boot_disk();
	}

	bd->bi_memsize -= (KERN_MEM_RESERVED * 2);
	bd->bi_memstart += KERN_MEM_RESERVED;

	return 0;
}

#ifdef CONFIG_SHOW_ACTIVITY
void board_show_activity (ulong timestamp)
{
	static int link_status[4] = {0, 0, 0, 0};
	int temp[4];
	u8  phyaddr;
	struct tsec_private *tsec_priv = NULL;
	extern uint read_phy_reg(struct tsec_private *priv, uint regnum);
	extern struct tsec_private *get_priv_for_phy(unsigned char phyaddr);

	/* 2 seconds */
	if (timestamp % (2 * CFG_HZ) == 0) {
		for (phyaddr = 0; phyaddr < 4; phyaddr++)
		{
			tsec_priv = get_priv_for_phy(phyaddr);
			if (tsec_priv) {
				temp[phyaddr] = read_phy_reg(tsec_priv, 17);
				if ((temp[phyaddr] & 0xe400) != (link_status[phyaddr] & 0xe400)) {
					link_status[phyaddr] = temp[phyaddr];
					printf("phyaddr %d - link: %s speed: %d and %s deplex mode \n",
						phyaddr,
						(link_status[phyaddr] & MIIM_88E1011_PHYSTAT_LINK) ? "up" : "down",
						(link_status[phyaddr] & MIIM_88E1011_PHYSTAT_GBIT) ? 1000 : 100,
						(link_status[phyaddr] & MIIM_88E1011_PHYSTAT_DUPLEX) ? "full" : "half");
				}
			}
		}
	}
}

void show_activity (int arg)
{
}
#endif

/*** Below post_word_store  and post_word_load  code is temporary code,
			 this will be modifed  ***/
#ifdef CONFIG_POST
DECLARE_GLOBAL_DATA_PTR;
void post_word_store (ulong a)
{
	volatile ulong  *save_addr_word =
			(volatile ulong *)(POST_STOR_WORD);
	*save_addr_word = a;
}

ulong post_word_load (void)
{
	volatile   ulong *save_addr_word =
			(volatile ulong *)(POST_STOR_WORD);
	return *save_addr_word;
}

/*
 *  *  *  * Returns 1 if keys pressed to start the power-on long-running tests
 *   *   *   * Called from board_init_f().
 *    *    *    */
int post_hotkeys_pressed (void)
{
	return 0;   /* No hotkeys supported */
}

#endif

#ifdef CONFIG_HW_WATCHDOG
void hw_watchdog_reset (void)
{
	volatile u_int32_t   *wdt_reg = (u_int32_t *)(CFG_PARKES_BASE + 0x20);
   
	*wdt_reg = 0;
}

/*
void hw_watchdog_disable(void)
{
	volatile u_int32_t *wdt_reg = (u_int32_t *)(CFG_PARKES_BASE + 0x18); 
 
	*wdt_reg &= ~0x1;   
}
*/

void hw_watchdog_enable (void)
{
	volatile u_int32_t  *wdt_reg = (u_int32_t *)(CFG_PARKES_BASE + 0x18);
	
	*wdt_reg |= 0x1;
}

#endif

int
post_stop_prompt (void)
{
	if (getenv("boot.nostop")) {
		return 0;
	}

	if (post_result_mem == -1 || post_result_cpu == -1
		|| post_result_rom == -1 || post_result_ether == -1
		|| post_result_pci == -1) {
		return 1;
	}

	return 0;
}

/**** APIs to configure PHYs on fabio via I2C *************/

#define BCM5461_PHYID		0x2060C0
#define MGMT_PHY_DEV		"gMGT"
#define MGMT_I2CPHY_ADDR 	0x4E
#define PCA9577_DEV_ADDR	0x1B
#define PCA9577_DEV_ADDR_HA2	0x1A

int i2cphy_read (char *devname, unsigned char addr, unsigned char reg, unsigned short *value)
{    
	if (addr != MGMT_I2CPHY_ADDR)
		return -1;
	
	return (mgmt_i2cphy_read(reg, value));
}

int i2cphy_write (char *devname, unsigned char addr, unsigned char reg, unsigned short value)
{    
	if (addr != MGMT_I2CPHY_ADDR)
		return -1;
	
	return (mgmt_i2cphy_write(reg, value));
}

int mgmt_i2cphy_read (unsigned char reg, unsigned short *value)
{
	int ret = 0;
	uchar *p = (uchar *)value;
	int cc = current_i2c_controller();
	
	i2c_controller(CFG_I2C_CTRL_2);
	p[0] = i2c_reg_read(MGMT_I2CPHY_ADDR, reg);
	p[1] = i2c_reg_read(MGMT_I2CPHY_ADDR, reg);
	if (p[0] == -1 || p[1] == -1) {
		printf("Read of PHY_ID from I2C CTRL 2 failed\n");
		ret = -1;
	}
	i2c_controller(cc);
	
	return ret;
}

int mgmt_i2cphy_write (unsigned char reg, unsigned short value)
{
	uchar *p = (uchar *)&value;
	int cc = current_i2c_controller();
	int ret = 0;
	
	i2c_controller(CFG_I2C_CTRL_2);
	if (i2c_reg_write(MGMT_I2CPHY_ADDR, reg, p[0]) == -1 ||
			i2c_reg_write(MGMT_I2CPHY_ADDR, reg, p[1]) == -1 ) {
		ret = -1;
	}
	i2c_controller(cc);
	
	return ret;
}
#define REG_OFFSET_I2C2 0x1c
#define EN_SGMII_MODE  0xFC0C
#define EN_LINK_LED    0xB41C

void mgmt_i2cphy_setup (void)
{
	ushort phy_reg = 0;
	uint phy_ID;    
	  
	mgmt_i2cphy_read(PHY_PHYIDR1, &phy_reg);
	phy_ID = (phy_reg & 0xffff) << 16;
	mgmt_i2cphy_read(PHY_PHYIDR2, &phy_reg);
	phy_ID |= (phy_reg & 0xffff);
	
	if ((phy_ID & 0xfffffff0) == BCM5461_PHYID) {
		miiphy_register(MGMT_PHY_DEV, i2cphy_read, i2cphy_write);
		/* Set SGMII mode */
		if (mgmt_i2cphy_write(REG_OFFSET_I2C2, EN_SGMII_MODE) == -1) {
			printf(" Setting of  PHY BCM5461 to SGMII mode from I2C CTRL 2 failed\n");
		} else {
			printf("Set PHY BCM5461 to SGMII mode done!\n");
		}
		/* Set link LED */
		if (mgmt_i2cphy_write(REG_OFFSET_I2C2,EN_LINK_LED) == -1) {
			printf("Setting of Link Led  from I2C CTRL 2 failed\n");
		}
	} else {
		printf("Unknown MGMT PHY. (ID = 0x%x) \n", phy_ID);
	}
	/* set boot flag to 9 if i2cphy setup is done */
	gd->boot_up_flag = 9;
}

#define HA_TXEN   0x0c
#define HA_IO_DIR 0xB3

void config_ha ()
{
    /*PARKES_FPGA_BOARD_STATUS 0x8 */    
    volatile u_int32_t   *parkes_bs = (u_int32_t *)(CFG_PARKES_BASE + 0x8);    
    int ret, len = 1;
    int alen = 1;
    int cc = current_i2c_controller();
    int device = SSRXB_SLAVE_CPLD_ADDR;
    int offset = I2CS_SYSIO_HA_MUX_REG;
    u8 *a = (u8*)&offset;
    uint8_t tdata[TX_DATA];
	char p[0];
	
    memset(tdata, 0, sizeof(tdata));
    /* Field Name    :   chassis_id 
     * Bit Range     :   7:4 4 bit 
     * Summary       :   Chassis ID number
     */
    if (((*parkes_bs >> 4) & 0xF) == A1_CHASSIS_ID) {
        /*1: Check whether we are in HA mode
         *2: DEvice addr = 0x54; offset:0x44; group 0x0F
         *3: read 0x44 I2CS_HAMUX_CTL in SYSIO I2CS FPGA 
         *4: if Value equal to 0x02 then I'm in Ha mode
         *5: If value equal to 0x01 then Ports are conigured for revenue mode
         */   
        offset = recb_i2cs_set_odd_parity((u_int8_t)(offset&0xFF)); /* add parity to offset */
        if((ret = recb_iic_write(SSRXB_I2C_GROUP_SELECT_SYSIO, (u_int8_t) device, alen,
                                                    &a[BYTE_LENGTH - alen])) != 0) {
            printf ("I2C read from device 0x%x failed.\n", device);
        } else {
            if ((ret = recb_iic_read(SSRXB_I2C_GROUP_SELECT_SYSIO, (u_int8_t) device,
                     len, tdata)) != 0) {
                printf ("I2C read from device 0x%x failed.\n", device);
            } else { 
                if (tdata[0] & 0x2) {
    	            printf("Configure HA port A1-EMERALD.\n");
	                i2c_controller(CFG_I2C_CTRL_2);
    	            /* Enbale HA TXEN_L  */
	                if (i2c_reg_write(PCA9577_DEV_ADDR, 1, 0x00) == -1) {
		                printf(" Write from I2C CTRL 2 failed for configuring"
			    	         " HA port: Enableing HA port failed\n");
	                }
	                if (i2c_reg_write(PCA9577_DEV_ADDR, 2, 0xf0) == -1) {
		                printf(" Write from I2C CTRL 2 failed for configuring"
			    	         " HA port: Enableing HA port failed\n");
	                }
	                if (i2c_reg_write(PCA9577_DEV_ADDR, 3, 0xfb) == -1) {
		                printf(" Write from I2C CTRL 2 failed for configuring"
			    	         " HA port: Enableing HA port failed\n");
	                }
                    /*Enable the IInd HA Port*/
	                if (i2c_reg_write(PCA9577_DEV_ADDR_HA2, 1, 0x00) == -1) {
		                printf(" Write from I2C CTRL 2 failed for configuring"
			    	         " HA port: Enableing HA port failed\n");
	                }
	                if (i2c_reg_write(PCA9577_DEV_ADDR_HA2, 2, 0xf0) == -1) {
		                printf(" Write from I2C CTRL 2 failed for configuring"
			    	         " HA port: Enableing HA port failed\n");
	                }
	                if (i2c_reg_write(PCA9577_DEV_ADDR_HA2, 3, 0xfb) == -1) {
		                printf(" Write from I2C CTRL 2 failed for configuring"
			    	         " HA port: Enableing HA port failed\n");
	                }
                }         
            } 
        }    
    } else {        
    	printf("Configure HA port A2/A10.\n");
	    i2c_controller(CFG_I2C_CTRL_2);
    	/* Enbale HA TXEN_L, HA_RS=1, clean BCM5416 reset */
	    if (i2c_reg_write(PCA9577_DEV_ADDR, 1, HA_TXEN) == -1) {
		    printf(" Write from I2C CTRL 2 failed for configuring"
			    	" HA port: Enableing HA port failed\n");
	    }
    	p[0] = i2c_reg_read(PCA9577_DEV_ADDR, 1);
	    if (p[0] == -1 ) {
		    printf("Read of PHY_ID from I2C CTRL 2 failed\n");
    	}
	    /* no polarity inversion */
    	if (i2c_reg_write(PCA9577_DEV_ADDR, 2, 0) == -1) {
	    	printf(" Write I2C CTRL 2 failed for configuring"
		    		" HA port: setting of polarity inversion failed\n");
	    }
    	p[0] = i2c_reg_read(PCA9577_DEV_ADDR, 2);
	    if (p[0] == -1 ) {
		    printf("Read of PHY_ID from I2C CTRL 2 failed\n");
	    }
	    /* set I/O direction */
    	if (i2c_reg_write(PCA9577_DEV_ADDR, 3, HA_IO_DIR) == -1) {
	    	printf(" Write I2C CTRL 2 failed for configuring"
		    		" HA port: setting of I/O direction failed\n");
	    }   
    	p[0] = i2c_reg_read(PCA9577_DEV_ADDR, 3);
	    if (p[0] == -1 ) {
		    printf("Read of PHY_ID from I2C CTRL 2 failed\n");
    	}
        
        i2c_controller(CFG_I2C_CTRL_1);
        p[0] = i2c_reg_read(EEPROM_MAINBOARD, 0x40);
        if(p[0] != 0xff) {
        /*Check whether CB1 is CRM. If yes then lift the reset of it!!! */        
            if (check_cb1_is_crm() == 1) {
                hdre_cntl_reset(0x7F);
            }    
        }

	    i2c_controller(cc);
    	/* Delay 1ms for BCM5416 */
	    udelay(1000);
    }   
}

static void *bounce_buffer = NULL;
int usb_scan_dev = 0;

#define MAX_LOADER_DEVICES  16

typedef struct dev_map {
	int interace;
	int intf_dev;
} dev_map_t;

static dev_map_t dev_pair[MAX_LOADER_DEVICES];

DECLARE_GLOBAL_DATA_PTR;

static void
set_disk_dev_map (int loader_dev, int intf, int intf_dev)
{
	dev_pair[loader_dev].interace = intf;
	dev_pair[loader_dev].intf_dev = intf_dev;
}

static void
get_disk_dev_map (int loader_dev, int *intf, int *intf_dev)
{
	*intf = dev_pair[loader_dev].interace;
	*intf_dev = dev_pair[loader_dev].intf_dev;
}

static void 
disable_parkes_sreset(void)
{
	volatile u_int32_t *parkes_reset = (u_int32_t *)(CFG_PARKES_BASE + 0x0C);
	*parkes_reset &= ~0x4;
}

int
usb_disk_scan (int dev)
{
	int i;
	static int last_usb_dev = -1;
	static int first_time_enter = 1;
	static u_int32_t nextboot = 0xff;
	block_dev_desc_t *stor_dev = NULL, *sdev = NULL;
	volatile u_int32_t *p_nextboot = (u_int32_t *)(CFG_PARKES_BASE + 0x150);

	if (!usb_started)
		usb_init();

	if (first_time_enter) {
		nextboot = *p_nextboot;
		printf("Will try to boot from\n");
		if ((nextboot == 0) || (nextboot & USB_BOOTMASK)) {
			printf("USB\n");
		}
		if ((nextboot == 0) || (nextboot & FLASH_BOOTMASK)) {
			printf("Compact Flash\n");
		}
		if ((nextboot == 0) || (nextboot & DISK_BOOTMASK)) {
			printf("Hard Disk\n");
		}
		if ((nextboot == 0) || (nextboot & LAN_BOOTMASK)) {
			printf("Network\n");
		}
		first_time_enter = 0;
	}

	bounce_buffer = (char *)(gd->ram_size - (1*1024*1024));

	if (dev < 0) {
		/* init dev mapping table */
		for (i = 0; i < MAX_LOADER_DEVICES; i++) {
			dev_pair[i].interace = IF_TYPE_UNKNOWN;
			dev_pair[i].intf_dev = 0xff;
		}
	}

	dev = (dev < 0) ? 0 : dev + 1;
	while (dev < usb_max_devs) {
		if (usb_dev_desc[dev].target != 0xff) {
			set_disk_dev_map(dev, IF_TYPE_USB, dev);
			last_usb_dev = dev;
			return (dev);
		}
	}

	if ((dev - (last_usb_dev + 1)) < CFG_SATA_MAX_DEVICE) {
		stor_dev = sata_get_dev(dev - (last_usb_dev + 1));
		if(stor_dev->target != 0xff) {
			/* if both hd adn cf installed, mapping cf first hd second */
			sdev = sata_get_dev(CFG_SATA_MAX_DEVICE - 1);
			if (sdev->target != 0xff) {
				set_disk_dev_map(dev, IF_TYPE_SCSI,
								CFG_SATA_MAX_DEVICE - (dev - last_usb_dev));
			} else {
				set_disk_dev_map(dev, IF_TYPE_SCSI, dev - (last_usb_dev + 1));
			}
			return (dev);
		}
	}

	if (NEXTBOOT_B2V(nextboot) == LAN_BOOT) {
		printf("Trying to boot from Network\n");
		*p_nextboot = (nextboot & 0x1f) | (USB_BOOT << 5);
		disable_parkes_sreset();
	}

	return (-1);
}

int
usb_disk_read (int dev, int lba, int totcnt, char *buf)
{
	unsigned long rd;
	int cnt;
	int max_sector;
	int intf, intf_dev;
	static int first_time_enter = 1;
	block_dev_desc_t *stor_dev = NULL;
	volatile u_int32_t *p_nextboot = (u_int32_t *)(CFG_PARKES_BASE + 0x150);

	get_disk_dev_map(dev, &intf, &intf_dev);

	switch (intf) {
	case IF_TYPE_USB:
		if (intf_dev < 0 || intf_dev >= usb_max_devs)
			return (-1);

		if (bounce_buffer == NULL)
			return (-1);

		if (first_time_enter) {
			printf("Trying to boot from USB\n");
			*p_nextboot = (*p_nextboot & 0x1f) | (FLASH_BOOT << 5);
			first_time_enter = 0;
			disable_parkes_sreset();
		}

		while (totcnt > 0) {
			cnt = (totcnt <= 16) ? totcnt : 16;
			rd = usb_stor_read(intf_dev, lba, cnt, bounce_buffer);
			if (rd == 0)
				return (totcnt);
			memcpy(buf, bounce_buffer, cnt * 512);
			buf += cnt * 512;
			lba += cnt;
			totcnt -= cnt;
		}
		break;

	case IF_TYPE_SCSI:
		if (bounce_buffer == NULL)
			return (-1);

		stor_dev = sata_get_dev(intf_dev);
		if (stor_dev->type == DEV_TYPE_UNKNOWN) {
			return (-1);
		}

		if (first_time_enter) {
			if (stor_dev->target == 1) {
				printf("Trying to boot from Compact Flash\n");
				*p_nextboot = (*p_nextboot & 0x1f) | (DISK_BOOT << 5);
			}
			if (stor_dev->target == 0) {
				printf("Trying to boot from Hard Disk\n");
				*p_nextboot = (*p_nextboot & 0x1f) | (USB_BOOT << 5);

			}
			first_time_enter = 0;
			disable_parkes_sreset();
		}

		/* sata hd is 16 and pata cf is 1 */
	if (!stor_dev->use_multiword_dma)
		max_sector = (intf_dev == 0) ? 16 : 1;
	else
		max_sector = 16;

		while (totcnt > 0) {
			cnt = (totcnt <= max_sector) ? totcnt : max_sector;
			rd = stor_dev->block_read(intf_dev, lba, cnt, (ulong *)bounce_buffer);
			if (rd == 0)
				return (totcnt);
			memcpy(buf, bounce_buffer, cnt * 512);
			buf += cnt * 512;
			lba += cnt;
			totcnt -= cnt;
		}
		break;

	default:
		return (-1);
	}
	return (totcnt);
}


static void
recb_i2cs_set_odd_parity_func (uint8_t *tdata)
{
	/* Need parity generation for the data & offset */
	/* Add parity to offset */
	tdata[0] = recb_i2cs_set_odd_parity(tdata[0]);
	/* Add parity to data */
	tdata[1] = recb_i2cs_set_odd_parity(tdata[1]);
}

static int
check_cb1_is_crm(void) 
{
    int  ret, len = 2;
    int offset = 4;
    uint8_t tdata[TX_DATA];
    u8 *a = (u8*)&offset;
    int alen = 1;

    if ((ret = recb_iic_write (SSRXB_I2C_GROUP_SELECT_REMOTE_RE, RECB_IDEEPROM, 
                                    alen, &a[4 - alen])) != 0) {
        printf ("I2C read from device 0x%x failed.\n", RECB_IDEEPROM);
        return -1;
    } else if ((ret = recb_iic_read (SSRXB_I2C_GROUP_SELECT_REMOTE_RE, RECB_IDEEPROM,
                                    len, tdata)) != 0) {
        printf ("I2C read from device 0x%x failed.\n", RECB_IDEEPROM);
        return -1;
    }
	
    if((tdata[0] == 0x9) && (tdata[1] == 0xc8)) {
        printf("CB1 IS CRM!!!\n");
        return 1;    
    } else {
        return 0;//Not an CRM card
    }    
}

/*Lift the reset of CRM/HDRE */
static void
hdre_cntl_reset (uint8_t value)
{
    int i, ret, len = 1;
    int alen = 1;
    uint8_t tdata[TX_DATA];
    int offset = 0x17;
    u8 *a = (u8*)&offset;
    
    /* RESET VALUE */
    len = 1;                
    tdata[1] = value;
                                    
    for (i = 0; i < alen; i++) {
       tdata[i] = a[BYTE_LENGTH - alen + i];
    }
                                        
    recb_i2cs_set_odd_parity_func(tdata);
    if ((ret = recb_iic_write(SSRXB_I2C_GROUP_SELECT_REMOTE_RE, 
            RECB_SLAVE_CPLD_ADDR,len + alen, tdata))) {
        printf("RECB Not able to access CRM.\n" );
    }
}

static void
recb_led_blink (int group, int device, int offset)
{
	int i, ret, len = 1;
	int alen = 1;
	uint8_t tdata[TX_DATA];
	u8 *a = (u8*)&offset;

	/* To blink led set the value in I2CS Led control reg */
	tdata[1] = I2CS_RECB_LED_BLINK_CTRL_VALUE;

	for (i = 0; i < alen; i++) {
		tdata[i] = a[BYTE_LENGTH - alen + i];
	}

	recb_i2cs_set_odd_parity_func(tdata);

	if ((ret = recb_iic_write((uint8_t)group, (uint8_t)device,
					len + alen, tdata))) {
		printf("RECB LED:I2C write to EEROM device 0x%x failed.\n", device);
	}
}

static void
fabio_led_blink (int group, int device, int offset, recb_fpga_regs_t *recb_p)
{
	int i, ret, len = 1;
	int alen = 1;
	uint8_t tdata[TX_DATA];
	u8 *a = (u8*)&offset;
	uint32_t board_status = 0;

	recb_master_slave_sm_t *master_slave = &recb_p->ms_arb;

	/* Setting the mastership bits in master slave config register */
	master_slave->recb_mas_slv_sm_sw_disable |= 0x03;
	tdata[1] = I2CS_FABIO_PWR_LED_BLINK_CTRL_VALUE;

	for (i = 0; i < alen; i++) {
		tdata[i] = a[BYTE_LENGTH - alen + i];
	}

	recb_i2cs_set_odd_parity_func(tdata);

	if ((ret = recb_iic_write((uint8_t)group, (uint8_t)device,
					len + alen, tdata))) {
		printf("PWR LED: I2C write to EEROM device 0x%x failed.\n", device);
	}

	/* Read the PWR led register */
	if ((ret = recb_iic_read((uint8_t)group, (uint8_t)device,
					len, tdata)) != 0) {
		printf("PWR LED: I2C read from device 0x%x failed.\n", device);
	} 

	board_status = (recb_p->recb_board_status >> 8);
	/* Based on the board_status detect RE is in which slot */
	if (board_status & 0x01) {
		/* offset :  I2CS Fabio Led control address 0 */
		offset = I2CS_FABIO_CTRL_ADDR_0;
		/* To blink led set the value in fabio I2CS Led control reg */
		tdata[1] = I2CS_RE0_FABIO_LED_GREEN_CTRL_VALUE;
		/* Added the Read value of PWR led to Fabio Green led value in RE0 */
		tdata[1] |= tdata[0];

		for (i = 0; i < alen; i++) {
			tdata[i] = a[BYTE_LENGTH - alen + i];
		}

		recb_i2cs_set_odd_parity_func(tdata);

		if ((ret = recb_iic_write((uint8_t)group, (uint8_t)device,
						len + alen, tdata))) {
			printf("FABIO LED GREEN: I2C write to EEROM device 0x%x failed.\n",
				device);
		}

		/* Read the Green led register */
		if ((ret = recb_iic_read((uint8_t)group, (uint8_t)device,
						len, tdata)) != 0) {
			printf("FABIO LED GREEN: I2C read from device 0x%x failed.\n",
				device);
		} 

		/* offset :  I2CS Led control address */
		offset = I2CS_CTRL_ADDR;
		/* To blink led set the value in I2CS Led control reg */
		tdata[1] = I2CS_RE0_FABIO_LED_BLINK_CTRL_VALUE;
		/* Added the Read value of Green led to Fabio Blink Green led value in RE0 */
		tdata[1] |= tdata[0];

		for (i = 0; i < alen; i++) {
			tdata[i] = a[BYTE_LENGTH - alen + i];
		}

		recb_i2cs_set_odd_parity_func(tdata);

		if ((ret = recb_iic_write((uint8_t)group, (uint8_t)device,
						len + alen, tdata))) {
			printf("FABIO LED BLINKING: I2C write to EEROM device 0x%x failed.\n",
				device);
		}
	} else {
		/* offset :  I2CS Fabio Led control address 1 */
		offset = I2CS_FABIO_CTRL_ADDR_1;
		/* To blink led set the value in fabio I2CS Led control reg */
		tdata[1] = I2CS_RE1_FABIO_LED_GREEN_CTRL_VALUE;
		/* Added the Read value of PWR led to Fabio Green led value in RE1 */
		tdata[1] |= tdata[0];

		for (i = 0; i < alen; i++) {
			tdata[i] = a[BYTE_LENGTH - alen + i];
		}

		recb_i2cs_set_odd_parity_func(tdata);

		if ((ret = recb_iic_write((uint8_t)group, (uint8_t)device,
						len + alen, tdata))) {
			printf("FABIO LED GREEEN: I2C write to EEROM device 0x%x failed.\n",
				device);
		}

		/* Read the Green led register */
		if ((ret = recb_iic_read((uint8_t)group, (uint8_t)device,
						len, tdata)) != 0) {
			printf("FABIO LED GREEN: I2C read from device 0x%x failed.\n",
				device);
		}

		/* offset :  I2CS Led control address */
		offset = I2CS_CTRL_ADDR;
		/* To blink led set the value in I2CS Led control reg */
		tdata[1] = I2CS_RE1_FABIO_LED_BLINK_CTRL_VALUE;
		/* Added the Read value of Green led to Fabio Blink Green led value in RE1 */
		tdata[1] |= tdata[0];

		for (i = 0; i < alen; i++) {
			tdata[i] = a[BYTE_LENGTH - alen + i];
		}

		recb_i2cs_set_odd_parity_func(tdata);

		if ((ret = recb_iic_write((uint8_t)group, (uint8_t)device,
						len + alen, tdata))) {
			printf("FABIO LED BLINKING: I2C write to EEROM device 0x%x failed.\n",
				device);
		}
	}
	/* Clearing the mastership bits in master slave config register */
	master_slave->recb_mas_slv_sm_sw_disable = 0x00;
}

int 
misc_init_r (void)
{
	/* skip iic for HDRE */
	if (cb_is_hdre()) {
		goto init_ha;
	}
	/* set boot flag to 5 if cpld access is failed */ 
	gd->boot_up_flag = 5;
	/* RECB FPGA Init */
	recb_iic_init();
	/* Enable the Recb Led Blink during bootup */
	recb_led_blink(SSRXB_I2C_GROUP_SELECT_LOCAL_RE,
			SSRXB_SLAVE_CPLD_ADDR,
			I2CS_CTRL_ADDR);

	/* Enable the Power & Fabio led during bootup */
	fabio_led_blink(SSRXB_I2C_GROUP_SELECT_FABIO,
			SSRXB_SLAVE_CPLD_ADDR,
			I2CS_CTRL_ADDR,
			(recb_fpga_regs_t *)CFG_PARKES_BASE);

init_ha:
	/* set boot flag to 6 if HA port is not accessible */
	gd->boot_up_flag = 6;
	/* Enable HA port */
	config_ha();

	return 0;
}

/*
 *  check if RE type is HDRE based on "hdre" env
 */
int
cb_is_hdre()
{
	char *hdre;

	hdre = getenv("hdre");
	if (hdre && *hdre == HDRE_ENV_SET) {
		return 1;
	} else {
		return 0;
	}
}

/*
 *  HA port control via BCM led proc
 */

/*
 * Broadcom LED processor u-code to get HA port link/activity status for A2A10
 * PORT 22    ; 0x2A 0x16
 * PUSHST 0   ; 0x32 0x0
 * PUSHST 1   ; 0x32 0x1
 * TOR        ; 0xB7
 * PUSH (0xc0) ; 0x26 0xC0
 * PACK       ; 0x87
 * PACK       ; 0x87
 * SEND   2   ; 0x3A 0x2
 */
static const uchar bcm_ha_led_ucode[] = {
	0x2A, 0x16, 0x32, 0x00, 0x32, 0x01, 0xB7, 0x26, 0xC0,
	0x87, 0x87, 0x3A, 0x02, 0x00, 0x00, 0x00
};

/*
 * This function load u-code to BCM LED processor
 */
static void
bcm_ledproc_load (ulong base, const uchar *program, int bytes)
{
	int     offset;

	if (!base) {
		base = BCM_CMIC_PCI_BASE_ADDR;
	}

	/* Stop LED processor */
	BCM_PCI_WRITE(base, CMIC_LED_CTRL, 0);

	/* Load u-code to program RAM */
	for (offset = 0; offset < CMIC_LED_PROGRAM_RAM_SIZE; offset++) {
		BCM_PCI_WRITE(base,
					  CMIC_LED_PROGRAM_RAM(offset),
					  (offset < bytes) ? (uint32_t) program[offset] : 0);
	}

	/* clear Data RAM */
	for (offset = 0x80; offset < CMIC_LED_DATA_RAM_SIZE; offset++) {
		BCM_PCI_WRITE(base,
					  CMIC_LED_DATA_RAM(offset),
					  0);
	}

	/* Start LED processor */
	BCM_PCI_WRITE(base, CMIC_LED_CTRL, 1);
}

void
init_ha_led (ulong pci_base)
{
	bcm_ledproc_load(pci_base, bcm_ha_led_ucode, sizeof(bcm_ha_led_ucode));
}

/* we can have only 3 major revision digits */
#define IS_VALID_VERSION_STRING(str) (str[1] == '.'	\
				      || str[2] == '.'	\
				      || str[3] == '.' ? 1 : 0)

uint32_t
get_uboot_version (void)
{
	uint32_t idx = 0;
	uint8_t mjr_digits[4];
	uint32_t major = 0, minor = 0;

	char *ver_string = uboot_api_ver_string;
    
	if (!IS_VALID_VERSION_STRING(ver_string)) {
		return 0;
	}

	for (idx = 0; idx < 4; idx++) {
		if (ver_string[idx] == '.') {
			mjr_digits[idx] = '\0';
			break;
		}
		mjr_digits[idx] = ver_string[idx];
	}
    
	idx++;

	major = simple_strtoul(mjr_digits, NULL, 10);
	minor = simple_strtoul(&ver_string[idx], NULL, 10);

	if (major <= 255 && minor <= 255) {
		return ((major << 16) | minor);
	}

	return 0;
}
