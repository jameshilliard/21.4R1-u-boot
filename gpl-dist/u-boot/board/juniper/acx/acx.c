/*
 * $Id$
 *
 * acx.c -- Platform specific code for the Juniper ACX Product Family
 *
 * Rajat Jain, Feb 2011
 * Samuel Jacob, Sep 2011
 *
 * Copyright (c) 2011-2013, Juniper Networks, Inc.
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
#include <asm/mmu.h>
#include <asm/cache.h>
#include <fdt_support.h>
#include <asm/fsl_law.h>
#include <netdev.h>
#include <usb.h>
#include <watchdog.h>
#include <exports.h>
#include "syspld.h"
#include "i2c_usb2513.h"

long int fixed_sdram(int board_type);

#define MAX_MODEL_NAME		10

#define KB			(1024)
#define MB			(1024*KB)

#define PAGE_SIZE		(4*KB)

DECLARE_GLOBAL_DATA_PTR;

/*
 * Next-boot information is stored in NVRAM
 * Register format is as follows.
 * ------------------------------------------------------------------------+
 * | BIT 7  |  BIT 6 |  BIT 5 |  BIT 4 |  BIT 3 |  BIT 2 |  BIT 1 |  BIT 0 |
 * +--------------------------+--------+--------+--------+--------+--------+
 * |    NEXT_BOOT             |  X     |  USB   | FLASH0 | FLASH1 |  X     |
 * |                          |        |  BOOT  | BOOT   |  BOOT  |        |
 * +--------------------------+--------+--------+--------+--------+--------+
 * NEXT_BOOT =
 *             1 => FLASH-0
 *             2 => FLASH-1
 *             3 => USB
 */
#define ACX_BOOTCONF_MASK		0x1f
#define ACX_NEXTBOOT_MASK		0xe0
#define ACX_NEXTBOOT_B2V(x)		((x & ACX_NEXTBOOT_MASK) >> 5)
#define ACX_USB_BOOT			3
#define ACX_NAND_FLASH0_BOOT		2
#define ACX_USB_BOOTMSK			(1 << ACX_USB_BOOT)
#define ACX_NAND_FLASH0_BOOTMSK		(1 << ACX_NAND_FLASH0_BOOT)

#define ACX_DEFAULT_NEXTBOOT		(ACX_USB_BOOTMSK | ACX_NAND_FLASH0_BOOTMSK)

typedef struct {
	uint8_t		version;
	uint8_t		junos_reboot_reason;
	uint8_t		uboot_reboot_reason;
	uint8_t		bdev_nextboot;
	uint32_t	re_memsize;
	uint32_t	re_memsize_crc;
	uint8_t		noboot_flag;
	uint8_t		misc;
} nvram_re_t;

typedef struct {
	uint32_t	post_word;
	uint32_t	wdt_test;
	uint8_t		reboot_reason;
} nvram_uboot_t;

typedef struct nvram_params {
	nvram_re_t	re_side;
	char		pad1[(PAGE_SIZE/2)-sizeof(nvram_re_t)];

    	/*
     	 * Scratchpad area in NVRAM for u-boot
     	 */
 	nvram_uboot_t	uboot_side;
	char		pad2[(PAGE_SIZE/2)-sizeof(nvram_uboot_t)];
} nvram_params_t;

/*
 * PFE uses 124KB of NVRAM
 */
#define CONFIG_SYS_NVRAM_PFE_USED	(124 * 1024)

static volatile nvram_params_t *nvram = (volatile nvram_params_t *) 
					(CONFIG_SYS_NVRAM_BASE +
					 CONFIG_SYS_NVRAM_PFE_USED);

typedef struct {
	char		model_name[MAX_MODEL_NAME];
	uint32_t	mem_min;
	uint32_t	mem_max;
	uint32_t	mem_default;
} acx_model_info_t;

static acx_model_info_t acx_model_info[] = {
	{"ACX-1000", 100*MB,  900*MB, 750*MB},
	{"ACX-2000", 100*MB, 1900*MB, 1500*MB},
	{"ACX-4000", 100*MB, 1900*MB, 1500*MB}
};

static acx_model_info_t *get_acx_model(int i2cid)
{
	int index;

	index = i2cid - I2C_ID_ACX1000;
	if (index < 0 || index > 3) {
	    index = 0;
	}

	return &acx_model_info[index];
}

uint32_t nvram_get_uboot_wdt_test(void)
{
	return nvram->uboot_side.wdt_test;
}

void nvram_set_uboot_wdt_test(uint32_t val)
{
	nvram->uboot_side.wdt_test = val;
}

uint8_t nvram_get_uboot_reboot_reason(void)
{
	return nvram->uboot_side.reboot_reason;
}

void nvram_set_uboot_reboot_reason(uint8_t val)
{
	nvram->uboot_side.reboot_reason = val;
}

static void set_boot_device(int dev, int is_netboot)
{
	char dev_str[32];

	if (is_netboot) {
		sprintf(dev_str, "net%d:", dev);
	} else {
		sprintf(dev_str, "disk%d:", dev);
	}
	setenv("loaddev", dev_str);
	printf("set_boot_device() %s\n", dev_str);
}

/*
 * Returns the number of USB storage devices attached to the system
 */ 
static int usb_stroage_count(void)
{
	unsigned char i;
	int result = 0;

	for(i = 0; i < USB_MAX_STOR_DEV; i++) {
		block_dev_desc_t *usb_dev_desc;

		usb_dev_desc = usb_stor_get_dev(i);
		if (!usb_dev_desc || usb_dev_desc->target == 0xff) {
			continue;
		}
		result++;
	}

	return result;
}

static void set_boot_disk(void)
{
	int usb_present, curr_boot_device;
	int usb_allowed, nand_allowed;

	/* If a USB storage device is attached then the count > 1 */
	usb_present = usb_stroage_count() > 1;
start:
	/* Find what all devices are allowed to boot from */
	usb_allowed = nvram->re_side.bdev_nextboot & ACX_USB_BOOTMSK;
	nand_allowed = nvram->re_side.bdev_nextboot & ACX_NAND_FLASH0_BOOT;
	/* Get current boot device from the bit field */
	curr_boot_device = ACX_NEXTBOOT_B2V(nvram->re_side.bdev_nextboot);

	/* Sanity check */
	if ((!usb_allowed && !nand_allowed) ||
	    !(curr_boot_device & ACX_BOOTCONF_MASK)) {
		nvram->re_side.bdev_nextboot = ACX_DEFAULT_NEXTBOOT;
		goto start;
	}

	/* 
	 * If USB attached choose whether to boot from NAND Flash or USB 
	 * based on user configuration
	 */
	if (usb_present) {
		/* Skip USB if user doesnt want to boot from it */
		if (!usb_allowed && curr_boot_device == ACX_USB_BOOTMSK) {
			curr_boot_device = ACX_NAND_FLASH0_BOOTMSK;
	    	}

		/* Skip NAND if user doesnt want to boot from it*/
		if (!nand_allowed &&
		    curr_boot_device == ACX_NAND_FLASH0_BOOTMSK) {
		    curr_boot_device = ACX_USB_BOOTMSK;
		}
	} else {
		/* 
		 * If a USB mass storage device is not attached then boot from
		 * NAND Flash
		 */
		curr_boot_device = ACX_NAND_FLASH0_BOOTMSK;
	}

	/* Set the next boot device in the NVRAM and loaddev env variable */
	if (curr_boot_device == ACX_USB_BOOTMSK) {
		nvram->re_side.bdev_nextboot |= ACX_NAND_FLASH0_BOOTMSK << 5;
		set_boot_device(1, 0);
	} else {
		nvram->re_side.bdev_nextboot |= ACX_USB_BOOTMSK << 5;
		set_boot_device(0, 0);
	}
}

static uint32_t get_re_memsize(void)
{
	uint32_t re_memsize, crc;
	acx_model_info_t *acx_model;

	re_memsize = nvram->re_side.re_memsize;
	acx_model = get_acx_model(gd->board_type);
	crc = crc32(0, (const unsigned char*)&nvram->re_side.re_memsize, 4);
	if (nvram->re_side.re_memsize_crc != crc) {
		printf("** Bad RE memsize Checksum in NVRAM **\n");
	} else if (re_memsize < acx_model->mem_min ||
		   re_memsize > acx_model->mem_max) {
		printf("** Bad RE memsize in NVRAM (0x%X bytes) **\n",
		       re_memsize);
	} else {
		/* Everything is good lets use the stored value */
		return re_memsize;
	}

	/* NVRAM is corrupt use default value */
	return acx_model->mem_default;
}

#ifdef CONFIG_HW_WATCHDOG
void hw_watchdog_reset(void)
{
	char *env_wdog;
	int tickle_override;
	static int inside = 0;

	/* getenv() calls WATCHDOG_RESET() and we are calling getenv().
	 * Timer interrupt can come while we are inside this function and timer 
	 * ISR calls WATCHDOG_RESET().
	 * To avoid any infinite loop - below check is needed.
	 */
	if (inside) {
	    return;
	}
	inside++;

	env_wdog = getenv("watchdog.tickle");

	tickle_override = env_wdog && memcmp(env_wdog, "no", 2) == 0;
	/* 
	 * Dont tickle watchdog if watchdog.tickle is set to "no". 
	 * It is set by autoboot command so that failure in loader will reset
	 * the system.
	 */
	if (!tickle_override) {
	    syspld_watchdog_tickle();
	}

	inside--;
}

void hw_watchdog_enable(void)
{
	syspld_watchdog_enable();
}
#endif

#ifdef CONFIG_POST
void post_word_store(ulong a)
{
	nvram->uboot_side.post_word = a;
}

ulong post_word_load(void)
{
	return nvram->uboot_side.post_word;
}

/*
 *  Returns 1 if there is a need to run the SLOW tests
 */
int post_hotkeys_pressed(void)
{
	char *slowtest_run;

	slowtest_run = getenv("run.slowtests");
	/* if long test(slow) is not requested */
	if (!slowtest_run || (memcmp(slowtest_run, "yes", 3) != 0)) {
		return 0;
	}

	/* if long test is requested but user pressed ctrlc() skip the test */
	return !ctrlc();
}
#endif

int board_early_init_f(void)
{
	/*
	 * Need to do this early to bring I2C switch, security chip out of reset,
 	 * needed to identify board I2C ID etc.
	 */
	syspld_init();
	nvram_set_uboot_reboot_reason(syspld_get_reboot_reason());

	return 0;
}

char *get_reboot_reason_str(int reboot_reason) 
{
	static char ch_reason[32];

	switch (reboot_reason) {
	case RESET_REASON_POWERCYCLE:
		return "power cycle";
	case RESET_REASON_WATCHDOG:
		return "watchdog";
	case RESET_REASON_HW_INITIATED:
		return "hw initiated";
	case RESET_REASON_SW_INITIATED:
		return "sw initiated";
	case RESET_REASON_SWIZZLE:
		return "flash swizzle";
	}

	sprintf(ch_reason, "indeterminate(%d)", reboot_reason);
	return &ch_reason[0];
}

int checkboard(void)
{
	uint32_t    		id = 0;
	uint16_t    		i2cid = 0;
	uint8_t	    		tmp;
	int	    		reboot_reason;
	acx_model_info_t 	*acx_model;

	/*
	 * i2c controller is initialised in board_init_f->init_func_i2c()
	 * before checkboard is called.
	 */
	/* read assembly ID from main board EEPROM offset 0x4 */
	tmp = 0x20;
	i2c_write(CONFIG_SYS_I2C_SW1_ADDR, 0, 1, &tmp, 1);
	i2c_read(CONFIG_SYS_I2C_EEPROM_ADDR, 4, 1, (uint8_t *)&id, 4);
	i2cid = (id & 0xffff0000) >> 16;

	if (i2cid != I2C_ID_ACX1000 &&
	    i2cid != I2C_ID_ACX2000 &&
            i2cid != I2C_ID_ACX2100 &&
            i2cid != I2C_ID_ACX2200 &&
            i2cid != I2C_ID_ACX1100 &&
	    i2cid != I2C_ID_ACX4000) {
		printf("Board: 0x%x is not a valid ACX board id! Assuming ACX1000 ...\n", i2cid);
		i2cid = I2C_ID_ACX1000;
	}
	acx_model = get_acx_model(i2cid);
	printf("Board: %s\n", acx_model->model_name);

	/* Save for global use by all */
	gd->board_type = i2cid;

	printf("SysPLD:Version %u Rev %u\n", syspld_version(), syspld_revision());
	printf("U-boot booting from: Bank-%d (Active Bank)\n", syspld_get_logical_bank());
	reboot_reason = nvram_get_uboot_reboot_reason();
	printf("Last reboot reason : %s\n", get_reboot_reason_str(reboot_reason));

	return 0;
}

phys_size_t initdram(int board_type)
{
	phys_size_t dram_size = 0;

	puts("Initializing....");

	dram_size = fixed_sdram(board_type);

	if (set_ddr_laws(CONFIG_SYS_DDR_SDRAM_BASE, dram_size,
			 LAW_TRGT_IF_DDR) < 0) {
		printf("ERROR setting Local Access Windows for DDR\n");
		return 0;
	};
	dram_size = setup_ddr_tlbs(dram_size / 0x100000);
	dram_size *= 0x100000;

	puts("    DDR: ");
	return dram_size;
}

#if defined(CONFIG_SYS_DRAM_TEST)
int testdram(void)
{
	uint *pstart = (uint *) CONFIG_SYS_MEMTEST_START;
	uint *pend = (uint *) CONFIG_SYS_MEMTEST_END;
	uint *p;

	return 0;

	/*
	 * A simple DRAM test like this takes 8 seconds to execute.
	 * It makes sense to NOT do this at every SW reboot to speed up boot.
	 * It is OK because since SW rebooted the chassis, most likely the RAM
	 * is OK. In case it is not, the user can always power cycle, or at WDT
	 * expiry this shall be run anyway.
	 */
	if (nvram_get_uboot_reboot_reason() == RESET_REASON_SW_INITIATED) {
	    return 0;
	}

	printf("DRAM test phase 1:\n");
	for (p = pstart; p < pend; p += CONFIG_SYS_MEMTEST_INCR / 4)
		*p = 0xaaaaaaaa;

	for (p = pstart; p < pend; p += CONFIG_SYS_MEMTEST_INCR / 4) {
		if (*p != 0xaaaaaaaa) {
			printf ("DRAM test fails at: %08x\n", (uint) p);
			return 1;
		}
	}

	printf("DRAM test phase 2:\n");
	for (p = pstart; p < pend; p += CONFIG_SYS_MEMTEST_INCR / 4)
		*p = 0x55555555;

	for (p = pstart; p < pend; p += CONFIG_SYS_MEMTEST_INCR / 4) {
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
	char temp[60];
	uchar enet[6];
	uint16_t esize = 0;
	uint8_t i;
	char *s, *e;
	bd_t *bd;
	int reboot_reason;
	acx_model_info_t *acx_model;

	bd = gd->bd;

	/*
	 * Read a valid MAC address and set "ethaddr" environment variable
	 */
	i2c_read(CONFIG_SYS_I2C_EEPROM_ADDR, CONFIG_SYS_EEPROM_MAC_OFFSET, 1,
		 enet, 6);
	i2c_read(CONFIG_SYS_I2C_EEPROM_ADDR, CONFIG_SYS_EEPROM_MAC_OFFSET-2, 1,
		 (unsigned char*)&esize, 1);
	i2c_read(CONFIG_SYS_I2C_EEPROM_ADDR, CONFIG_SYS_EEPROM_MAC_OFFSET-1, 1,
		 ((unsigned char*)&esize) + 1, 1);

	if (esize) {
		/* 
		* As per the specification, the management interface is
		* assigned the last MAC address out of the range
		* programmed in the EEPROM. U-Boot was using the first
		* in the range and this can cause problems when both
		* U-Boot and JUNOS generate network traffic in shorti
		* order. The switch to which the management interface
		* is connected is exposed to a sudden MAC address
		* change for the same IP in that case and may not
		* forward packets correctly at first. This can cause
		* timeouts and thus cause user-noticable failures.
		*
		* Using the same algorithm from JUNOS to guarantee the
		* same MAC address picked for tsec0 in U-boot.
		*/
		*(uint16_t *)(enet + 4) += (esize - 1);
	}

	sprintf(temp, "%02x:%02x:%02x:%02x:%02x:%02x",
			enet[0], enet[1], enet[2], enet[3], enet[4], enet[5]);
	s = getenv("eth0addr");
	if (memcmp (s, temp, sizeof(temp)) != 0) {
		setenv("eth0addr", temp);
	}

	acx_model = get_acx_model(gd->board_type);
	sprintf(temp, "%s", acx_model->model_name);
	setenv("hw.board.type", temp);

	/*
	 * set eth active device as eTSEC1 
	 */
	setenv("ethact", CONFIG_TSEC1_NAME);
	
	/*
	 * set bd->bi_enetaddr to active device's MAC (eTSEC1).
	 */
	s = getenv ("eth0addr");
	for (i = 0; i < 6; ++i) {
		bd->bi_enetaddr[i] = s ? simple_strtoul (s, &e, 16) : 0;
		if (s)
			s = (*e) ? e + 1 : e;
	}

	/*
	 * Set bootp_if as fxp0 (eTSEC1) for ACX.
	 * This environment variable is used by JUNOS install scripts
	 */
	setenv("bootp_if", CONFIG_TSEC1_NAME);

	/*
	 * Set upgrade environment variables
	 */
	sprintf(temp,"0x%08x ", CFG_UPGRADE_LOADER_ADDR);
	setenv("boot.upgrade.loader", temp);

	sprintf(temp,"0x%08x ", CFG_UPGRADE_BOOT_IMG_ADDR);
	setenv("boot.upgrade.uboot", temp);

	reboot_reason = nvram_get_uboot_reboot_reason();
	setenv("reboot.reason", get_reboot_reason_str(reboot_reason));

	if (reboot_reason == RESET_REASON_SWIZZLE) {
		/* For Junos this is HW initiated only */
		nvram->re_side.uboot_reboot_reason = 1 << RESET_REASON_HW_INITIATED;
	} else if (reboot_reason == RESET_REASON_SW_INITIATED) {
		/* JunOS doesn't want to get SW initiated reasons */
		nvram->re_side.uboot_reboot_reason = 0;
	} else {
		/* Pass the actual reason */
		nvram->re_side.uboot_reboot_reason = 1 << reboot_reason;
	}

	/* set boot device according to nextboot */
	set_boot_disk();

	/* Calculate the memory to be handed over to RE */
	gd->ram_size = get_re_memsize();
	gd->bd->bi_memsize = gd->ram_size;
	printf("RE memsize %d MB\n", gd->ram_size/(1024*1024));

	/* uboot image is stable - disable flash swizzle and enable hw_watchdog */
	syspld_swizzle_disable();
	hw_watchdog_enable();

	return 0;
}

static int usb2513_write(uint8_t reg, uint8_t val)
{
	/*
	 * The USB2513 chip expects the num of bytes in the first byte,
	 * The actual data follows it. We always need to write 1 byte of 
	 * data, thus send 2 bytes 
	 */
	uint8_t tmp[2];
	tmp[0] = sizeof(uint8_t);
	tmp[1] = val;

	return i2c_write(CONFIG_SYS_I2C_USB2513_ADDR, reg, 1, tmp, 2);
}

/*
 * Write the string in UTF-16LE to the specified
 * location
 */
static int i2c_usb2513_write_utf16le(u_int8_t reg, char *str)
{
	int ind;
	int error = 0;

	for (ind = 0; ind < strlen(str); ind++) {
		error = usb2513_write(reg + (ind * 2),  str[ind]);
		if (error) {
			break;
		}

		error = usb2513_write(reg + (ind * 2) + 1,  0);
		if (error) {
			break;
		}
	}

	return error;
}

/*
 * Initialization for usb2513
 * On fortius, the following is the downstream configuration
 * port 1 : usb connector 1
 * port 2 : usb connector 2
 * port 3 : usb mass storage controller
 */
static int i2c_usb2513_init(void)
{
	int error;
	u_int16_t ind;
	uint8_t tmp;

	/*
	 * Open the I2C multiplxer for USB hub ac access
	 */
	tmp = 0x8;
	i2c_write(CONFIG_SYS_I2C_SW1_ADDR, 0, 1, &tmp, 1);


	/*
	 * Program the Vendor Id
	 */
	error = usb2513_write(USB2513_VENDORID_LSB, USB2513_VENDOR_ID_SMSC_LSB);
	if (error)
		return error;

	error = usb2513_write(USB2513_VENDORID_MSB, USB2513_VENDOR_ID_SMSC_MSB);
	if (error)
		return error;

	/*
	 * Program the Product Id
	 */
	error = usb2513_write(USB2513_PRODID_LSB, USB2513_PRODID_USB2513_LSB);
	if (error)
		return error;

	error = usb2513_write(USB2513_PRODID_MSB,USB2513_PRODID_USB2513_MSB );
	if (error)
		return error;

	/*
	 * Program the Device Id
	 */
	error = usb2513_write(USB2513_DEVID_LSB, USB2513_DEVID_DEFAULT_LSB);
	if (error)
		return error;

	error = usb2513_write(USB2513_DEVID_MSB, USB2513_DEVID_DEFAULT_MSB);
	if (error)
		return error;

	/*
	 * Program the config byte 1
	 */
	error = usb2513_write(USB2513_CONF1, USB2513_CONF1_MTT_EN |
			 USB2513_CONF1_EOP_DIS |
			 USB2513_CONF1_CURR_SNS_PERPORT |
			 USB2513_CONF1_PERPORT_POWER_SW);
	if (error)
		return error;

	/*
	 * Program the config byte 2
	 */
	error = usb2513_write(USB2513_CONF2, USB2513_CONF2_OC_TIMER_8MS);
	if (error)
		return error;

	/*
	 * There are no LED pins on the USB2513. Program to default.
	 * This should report to the upstream host that the hub has
	 * no LEDs. Enable support for strings as well.
	 */
	error = usb2513_write(USB2513_CONF3, USB2513_CONF3_LED_MODE_SPEED | 
			USB2513_CONF3_STRING_SUPPORT_EN);
	if (error)
		return error;

	/*
	 * Program all ports on the usb hub as removable
	 */
	error = usb2513_write(USB2513_NONREM_DEV, 0);
	if (error)
		return error;

	/*
	 * USB2513Bi is a self-powered, three port hub. Enable all ports
	 */

	error = usb2513_write(USB2513_PORTDIS_SELF, 0);
	if (error)
		return error;

	/*
	 * USB2513Bi is configured to operate in self-powered mode.
	 * This register has no significance. Write the default value
	 * of 0 just the same.
	 */
	error = usb2513_write(USB2513_PORTDIS_BUS, 0);
	if (error)
		return error;

	/*
	 * Configure the hub to draw a maximum of 2ma from the upstream
	 * port in self-powered mode.
	 */
	error = usb2513_write(USB2513_MAXPOWER_SELF, USB2513_MAXPOWER_SELF_2MA);
	if (error)
		return error;

	error = usb2513_write(USB2513_MAXPOWER_BUS, USB2513_MAXPOWER_BUS_100MA);
	if (error)
		return error;

	error = usb2513_write(USB2513_HUBC_MAX_I_SELF, USB2513_HUBC_MAX_I_SELF_2MA);
	if (error)
		return error;

	error = usb2513_write(USB2513_HUBC_MAX_I_BUS, USB2513_HUBC_MAX_I_BUS_100MA);
	if (error)
		return error;

	error = usb2513_write(USB2513_POWERON_TIME, USB2513_POWERON_TIME_100MS);
	if (error)
		return error;

	/*
	 * Setup the language ID as English
	 */
	error = usb2513_write(USB2513_LANGID_HIGH, USB2513_LANGID_ENGLISH_US_MSB);
	if (error)
		return error;

	error = usb2513_write(USB2513_LANGID_LOW, USB2513_LANGID_ENGLISH_US_LSB);
	if (error)
		return error;

	/*
	 * Now program the manufacter string
	 */
	error = usb2513_write(USB2513_MFG_STR_LEN, strlen(USB2513_MFR_STRING));
	if (error)
		return error;

	error = i2c_usb2513_write_utf16le(USB2513_MFG_STR_BASE, USB2513_MFR_STRING);
	if (error)
		return error;

	/*
	 * Now program the product string
	 */
	error = usb2513_write(USB2513_PROD_STR_LEN, strlen(USB2513_PROD_STRING));
	if (error)
		return error;

	error = i2c_usb2513_write_utf16le(USB2513_PROD_STR_BASE, USB2513_PROD_STRING);
	if (error)
		return error;

	/*
	 * Now program the serial number
	 */
	error = usb2513_write(USB2513_SER_STR_LEN, strlen(USB2513_SERNO_STRING));
	if (error)
		return error;

	error = i2c_usb2513_write_utf16le(USB2513_SER_STR_BASE, USB2513_SERNO_STRING);
	if (error)
		return error;

	/*
	 * Enable battery charging on all ports
	 */
	error = usb2513_write(USB2513_BAT_CHARG_EN, USB2513_BAT_CHARG_EN_PORT1 |
			USB2513_BAT_CHARG_EN_PORT2 |
			USB2513_BAT_CHARG_EN_PORT3);
	if (error)
		return error;

	/*
	 * Now program all other registers to the default value of 0. This
	 * is important and the hub will fail to work reliably without this.
	 */
	for (ind = USB2513_BAT_CHARG_EN + 1; ind <= USB2513_PORT_MAP7; ind++) {
		error = usb2513_write((u_int8_t)ind, 0);
		if (error)
			return error;
	}
		
	/*
	 * Now, trigger an attach event to the host.
	 */
	error = usb2513_write(USB2513_STATCMD, USB2513_STATCMD_USB_ATTACH);
	if (error)
		return error;

	/*
	 * Close the I2C multiplxer for USB hub access
	 */
	tmp = 0x20;
	i2c_write(CONFIG_SYS_I2C_SW1_ADDR, 0, 1, &tmp, 1);

	return 0;
}

int board_early_init_r(void)
{
	const unsigned int flashbase = CONFIG_SYS_FLASH_BASE;
	const u8 flash_esel = find_tlb_idx((void *)flashbase, 1);

	/*
	 * Remap Boot flash + PROMJET region to caching-inhibited
	 * so that flash can be erased properly.
	 */

	/* Flush d-cache and invalidate i-cache of any FLASH data */
	flush_dcache();
	invalidate_icache();

	/* invalidate existing TLB entry for flash + promjet */
	disable_tlb(flash_esel);

	set_tlb(1, flashbase, CONFIG_SYS_FLASH_BASE_PHYS,
			MAS3_SX|MAS3_SW|MAS3_SR, MAS2_I|MAS2_G,
			0, flash_esel, BOOKE_PAGESZ_4M, 1);

	/*
	 * We could not bring the USB PHY out of reset earlier since
	 * some boards have the USB reset line as active high, and some 
	 * as active low - and the board type was not known then. 
	 * Do it now.
	 */
	syspld_reset_usb_phy();

	/*
	 * Program the USB hub on Fortius via the SMB
	 * This is needed before the USB enumeration so that NAND flash
	 * and other USB devices can be detected
	 */
	if (i2c_usb2513_init()) {
		printf ("USB HUB initialization failed\n");
	}

	return 0;
}

#if defined(CONFIG_OF_BOARD_SETUP)
void ft_board_setup(void *blob, bd_t *bd)
{
	phys_addr_t base;
	phys_size_t size;

	ft_cpu_setup(blob, bd);

	base = getenv_bootm_low();
	size = getenv_bootm_size();

	fdt_fixup_memory(blob, (u64)base, (u64)size);

#ifdef CONFIG_PCIE3
	ft_fsl_pci_setup(blob, "pci0", &pcie3_hose);
#endif
#ifdef CONFIG_PCIE2
	ft_fsl_pci_setup(blob, "pci1", &pcie2_hose);
#endif
#ifdef CONFIG_PCIE1
	ft_fsl_pci_setup(blob, "pci2", &pcie1_hose);
#endif
}
#endif

int board_start_usb(void)
{
    return 1;
}

/* 
 * pci_init_board() is need to be defined becuase pci_init() calls it.
 * But it is not implemented in ACX platform because of the following reasons
 * 1) PFE owns all the PCIe devices.
 * 2) RE should not touch any PCIe devices/controller(AMP requirement).
 * 3) There is no uboot diags for PCIe.
 */
void pci_init_board(void)
{

}

