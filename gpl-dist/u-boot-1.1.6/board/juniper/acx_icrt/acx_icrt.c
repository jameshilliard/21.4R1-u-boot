/*
 * $Id$
 *
 * acx_icrt.c -- Platform sepcific code for the iCRT of ACX500 box
 *
 * Amit Verma, March 2014
 *
 * Copyright (c) 2011-2014, Juniper Networks, Inc.
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
#include <acx_syspld.h>
#include <exports.h> /* usb_disk_scan */
#include <acx_i2c_usb2513.h>

#define ACX1000_NAME_STRING	"ACX-1000"
#define ACX2000_NAME_STRING	"ACX-2000"
#define ACX4000_NAME_STRING	"ACX-4000"
#define ACX2100_NAME_STRING     "ACX-2100"
#define ACX2200_NAME_STRING     "ACX-2200"
#define ACX1100_NAME_STRING     "ACX-1100"
#define ACX500_O_POE_DC_NAME_STRING  "ACX-500-O-POE-DC"
#define ACX500_O_POE_AC_NAME_STRING  "ACX-500-O-POE-AC"
#define ACX500_O_DC_NAME_STRING      "ACX-500-O-DC"
#define ACX500_O_AC_NAME_STRING      "ACX-500-O-AC"
#define ACX500_I_DC_NAME_STRING      "ACX-500-I-DC"
#define ACX500_I_AC_NAME_STRING      "ACX-500-I-AC"

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
#define NAND_FLASH_0_BOOT               2
#define USB_BOOT                        3

#define NAND_FLASH_0_BOOTMASK           (1 << NAND_FLASH_0_BOOT)
#define USB_BOOTMASK                    (1 << USB_BOOT)

#define BOOT_DEVICE(x)                  ((x & 0xe0) >> 5)

#define DEFAULT_CURRBOOTDEV ((USB_BOOT << 5) |	\
	(NAND_FLASH_0_BOOTMASK | USB_BOOTMASK))


/* ========= dual root partition logic defines begin =======================*/
#define DEFAULT_SECTOR_SIZE	    512
#define NUM_DISK_PART		    4
#define DISK_PART_SIZE		    0x10
#define DISK_PART_TBL_OFFSET	    0x1be
#define DISK_PART_MAGIC_OFFSET	    0x1fe
#define DISK_PART_MAGIC		    0xAA55
#define BSD_MAGIC		    0xa5
#define ACTIVE_FLAG		    0x80

typedef struct disk_part {
    uint8_t	boot_flag;	    /* active				*/
    uint8_t	head;		    /* start head			*/
    uint8_t	sector;		    /* start sector			*/
    uint8_t	cylinder;	    /* start cylinder			*/
    uint8_t	part_type;	    /* type of partition.		*/
    uint8_t	end_head;	    /* end of head			*/
    uint8_t	end_sector;	    /* end of sector			*/
    uint8_t	end_cylinder;	    /* end of cylinder			*/
    uint8_t	start_sec[4];	    /* start sector from 0		*/
    uint8_t	size_sec[4];	    /* number of sectors in partition	*/
} disk_part_t;

#define ACX_NAND0                       0
#define SELECT_ACTIVE_SLICE		0
#define DEFAULT_SLICE_TO_TRY		1

#if ENABLE_DR_DEBUG 
#define DR_DEBUG(fmt, args...)	printf(fmt, ## args)
#else
#define DR_DEBUG(fmt, args...)
#endif

#define TRUE				1
#define FALSE				0
#define MBR_BUF_SIZE			512
#define MAX_NUM_SLICES			4
#define FIRST_SLICE			1
#define IS_NUMERAL(num) 		(num >= 0 && num <= 9 ? TRUE : FALSE)
#define IS_VALID_SLICE(slice)		(slice >= FIRST_SLICE &&	      \
					 slice <= MAX_NUM_SLICES	      \
					 ? TRUE : FALSE)

/* ========= dual root partition logic defines end   =======================*/

extern void acx_icrt_uboot_jump(void);
extern int acx_upgrade_images(void);
extern block_dev_desc_t usb_dev_desc[USB_MAX_STOR_DEV];
extern struct usb_device usb_dev[USB_MAX_DEVICE];
int usb_scan_dev = 0;
int watchdog_tickle_override = 0;
unsigned long acx_re_memsize;
extern int post_result_cpu;
extern int post_result_mem;
extern int post_result_rom;
extern int post_result_syspld;
extern int post_result_usb;
extern int post_result_nand;
extern int post_result_nor;
extern int post_result_watchdog;
extern int post_result_ether;

extern long int fixed_sdram(int board_type);

void sdram_init(void);
int testdram(void);

extern char version_string[];

extern enum imageType
{
    ACX_UBOOT_IMAGE,
    ACX_LOADER_IMAGE,
    ACX_SYSPLD_IMAGE,
    ACX_SECURE_BLOCK_IMAGE
} NorImages;
DECLARE_GLOBAL_DATA_PTR;

typedef struct nvram_params {
    struct {
	uint8_t  version;
	uint8_t  junos_reboot_reason;
	uint8_t  uboot_reboot_reason;
	uint8_t  bdev_nextboot;
	uint32_t re_memsize;
	uint32_t re_memsize_crc;
	uint8_t  noboot_flag;
	    #define NOBOOT_FLAG 0x41
	uint8_t  misc;
	uint8_t  next_slice;
	uint8_t  reboot_overheat_flag; /* set the flag if the reboot is due to
	                                  over heating */
	    #define REBOOT_OVHEAT_FLAG 0x1
	uint32_t reboot_wait_time;     /* wait time in min if the above flag
	                                  set */
	uint32_t reboot_wait_time_crc; /* crc for the field */
	uint8_t  default_board; /* flag to indicate default board type */
	uint8_t  unattended_mode;
	    #define PASSWORD_LEN 256
	char     boot_loader_pwd[PASSWORD_LEN + 1];
	uint8_t  pwd_flag;
	uint8_t  hash_type; /* last bit indicates if the hash type is set */
	uint8_t  bios_upgrade; /* BIOS upgrade request through JUNOS CLI */
            #define ICRT_VERSION_LEN 64
	char     icrt_version[ICRT_VERSION_LEN];
	uint8_t  res[1697]; /* = what remains of the 2048 byte re nvram area */
    } re_side;

    /*
     * Scratchpad area in NVRAM for u-boot
     */
    struct {
	uint32_t  post_word;
	uint32_t  wdt_test;
	uint32_t  sreset_test;
	uint8_t   reboot_reason;
	uint8_t   res[2035];
    } uboot_side;

} nvram_params_t;

/*
 * PFE uses 124KB of NVRAM
 */
#define CFG_NVRAM_PFE_USED (124 * 1024)

static volatile nvram_params_t *nvram = (volatile nvram_params_t *) 
					(CFG_NVRAM_BASE + CFG_NVRAM_PFE_USED);

/*
 * Check the reboot is beacuse of over heating
 * if yes load the bootdelay environment variable with
 * reboot wait_time value
 */
void check_reboot_due_to_over_heat(void)
{
    char delay_str[12];
    uint8_t buf[1 + sizeof(nvram->re_side.reboot_wait_time)];

    if (REBOOT_OVHEAT_FLAG == nvram->re_side.reboot_overheat_flag) {
	/*
	 * Ensure CRC is correct
	 */
	buf[0] = nvram->re_side.reboot_overheat_flag;
	memcpy(&buf[1], (void *)&(nvram->re_side.reboot_wait_time),
	       sizeof(nvram->re_side.reboot_wait_time));
	/*
	 * Check the crc field
	 */
	if (nvram->re_side.reboot_wait_time_crc != crc32(0, buf, sizeof(buf))) {
	    /*
	     * Clear the flag so that next reboot will be fine 
	     */
	    nvram->re_side.reboot_overheat_flag = 0;
	    printf("** Bad reboot wait time Checksum in NVRAM **\n");
	    return;
	}
	sprintf(delay_str, "%d", (nvram->re_side.reboot_wait_time * 60));
	setenv("bootdelay", delay_str);
	/*
	 * Reset the flag 
	 */
	nvram->re_side.reboot_overheat_flag = 0;
    }
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

uint32_t nvram_get_uboot_sreset_test(void)
{
	return nvram->uboot_side.sreset_test;
}

void nvram_set_uboot_sreset_test(uint32_t val)
{
	nvram->uboot_side.sreset_test = val;
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
		 * On Fortius Board, make sure USB is plugged into 1 of 2 ext USB ports.
		 */
		if (pdev->parent && ((pdev->portnr == 1) || (pdev->portnr == 2))) {
			*dev = 1;
			return 1;
		}
	}

	return 0; /* no external usb device found */
}

/*
 * Function to get USB device number based on port
 * number. USB is connected on Port 1 of USB hub
 * device.
 *
 */
int get_usb_dev_num(int *dev)
{
        unsigned char i;
        struct usb_device *pdev;

        for (i=0;i<USB_MAX_STOR_DEV;i++) {
                if (usb_dev_desc[i].target == 0xff) {
                        continue;
                }

                pdev = &usb_dev[usb_dev_desc[i].target -1];
                if (pdev->devnum == -1)
                        continue;

                if (pdev->parent && (pdev->portnr == 1)) {
                        *dev = i;
                        return 1;
                }
        }

        return 0;
}

static void cal_re_memsize(void)
{
#define MB 0x100000UL

    unsigned int nvram_corrupt = 0;

    if (nvram->re_side.re_memsize_crc != 
	crc32(0, (const unsigned char*)&nvram->re_side.re_memsize, 4)) {
	/* Ensure CRC is correct */
	printf("** Bad RE memsize Checksum in NVRAM **\n");
	nvram_corrupt = 1;

    } else if (((gd->board_type == I2C_ID_ACX1000) ||
                (gd->board_type == I2C_ID_ACX1100) ||
                 IS_BOARD_ACX500(gd->board_type))  && 
	       ((nvram->re_side.re_memsize < (100 * MB)) || 
	       (nvram->re_side.re_memsize > (900 * MB)))) {
	/* Ensure within allowed limits for ACX1000 */
	printf("** Bad RE memsize in NVRAM (0x%X bytes) **\n", 
	       nvram->re_side.re_memsize);
	nvram_corrupt = 1;

    } else if (((gd->board_type == I2C_ID_ACX2000) ||
                (gd->board_type == I2C_ID_ACX4000) ||
                (gd->board_type == I2C_ID_ACX2100) ||
                (gd->board_type == I2C_ID_ACX2200)) && 
               ((nvram->re_side.re_memsize < (200 * MB)) ||
	       (nvram->re_side.re_memsize > (1800 * MB)))) {
	/* Ensure within allowed limits for ACX2000 / ACX4000 */
	printf("** Bad RE memsize in NVRAM (0x%X bytes) **\n", 
	       nvram->re_side.re_memsize);
	nvram_corrupt = 1;
    }

    /* 
     * Reset to default memsize if required
     * Also recalculate the CRC and write so this 
     * does not have to be done again
     */
    if (nvram_corrupt) {
        switch(gd->board_type) {
            case I2C_ID_ACX1000:
            case I2C_ID_ACX1100:
            CASE_ALL_I2CID_ACX500_CHASSIS:            
	        nvram->re_side.re_memsize = ACX1000_DEFAULT_RE_MEMSIZE;
                break;
            case I2C_ID_ACX2000:
            case I2C_ID_ACX2100:
            case I2C_ID_ACX2200:
            case I2C_ID_ACX4000:
            default:
	        nvram->re_side.re_memsize = ACX2000_DEFAULT_RE_MEMSIZE;
                break;
        }
	nvram->re_side.re_memsize_crc = crc32(0, (const unsigned char*)
					      &nvram->re_side.re_memsize, 4);
	printf("Reseting RE memsize to default in NVRAM (%u MB)\n", 
	       nvram->re_side.re_memsize >> 20);	    
    }

    /* Actually use the memsize specified in NVRAM */
    acx_re_memsize = nvram->re_side.re_memsize;
}

void hw_acx_watchdog_reset(void)
{
	syspld_cpu_reset();
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

	syspld_watchdog_tickle();
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
 *  Called from board_init_f().
 */
int post_hotkeys_pressed(void)
{
	char *slowtest_run;
	unsigned int delay = 5, abort = 0;

	slowtest_run = getenv("run.slowtests");
	if (!slowtest_run || (memcmp(slowtest_run, "yes", 3) != 0))
		return 0;

	printf("You have chosen to boot in SLOWTEST mode (This can be changed via "
		"\"run.slowtests\" u-boot env variable).\nThis causes detailed diagnostics"
		" to be run which may take a lot of time to run.\nPlease press any "
		"key to abort this mode and continue to boot in NORMAL mode: %2d", delay);

	while ((delay > 0) && (!abort)) { 
		int i; 
 
		--delay; 
		/* delay 100 * 10ms */ 
		for (i=0; !abort && i<100; ++i) { 
			if (tstc()) {   /* we got a key press   */ 
				abort  = 1; /* don't got to SLOWTEST mode */
				delay = 0;  /* no more delay    */ 
				(void) getc();  /* consume input    */ 
				break; 
			} 
			udelay (10000); 
		} 

		printf ("\b\b\b%2d ", delay); 
	}

	if (abort) {
		printf("\nEntering NORMAL mode...\n");
		return 0;
	} else {
		printf("\nEntering SLOWTEST mode...\n");
		return 1;
	}
}

int post_stop_prompt(void)
{
	if (getenv("boot.nostop")) {
		return 0;
	}

	if (post_result_mem == -1 ||
	    post_result_cpu == -1 ||
	    post_result_rom == -1 ||
	    post_result_syspld == -1 ||
	    post_result_usb == -1 ||
	    post_result_nand == -1 ||
	    post_result_watchdog == -1 ||
	    post_result_nor == -1) {
		printf("POST Failed! Dropping to u-boot prompt...\n");
		return 1;
	}

	return 0;
}
#endif

int board_early_init_f(void)
{
	/*
	 * Need to do this early to bring I2C switch, security chip out of reset,
	 * needed to identify board I2C ID etc.
	 */
	syspld_init();
	return 0;
}

static int board_i2c_id(u_int32_t board_id)
{
	return ((board_id & 0xffff0000) >> 16);
}

int version_major(u_int32_t board_type)
{
	return ((board_type & 0xff00) >> 8);
}

int version_minor(u_int32_t board_type)
{
	return (board_type & 0xff);
}

char *get_reboot_reason_str(void) 
{
	switch (nvram_get_uboot_reboot_reason()) {

	case RESET_REASON_POWERCYCLE:
		return "power.cycle";
	case RESET_REASON_WATCHDOG:
		return "watchdog";
	case RESET_REASON_HW_INITIATED:
		return "hw.initiated";
	case RESET_REASON_SW_INITIATED:
		return "sw.initiated";
	case RESET_REASON_SWIZZLE:
		return "flash.swizzle";
	}

	return "indeterminate";
}

int checkboard(void)
{
	uint32_t id = 0;
	uint16_t i2cid;
	uint8_t  tmp;

	tmp = 0x20; /* Open Channel 5 of I2C SW1 */
#ifdef DEBUG_STUFF
{
	int i = 0;
	printf("\n\nEEPROM Contents:\n");

	i2c_write(CFG_I2C_SW1_ADDR, 0, 1, &tmp, 1);

	for(i = 0; i < 256; i += 4) {
		i2c_read(CFG_I2C_EEPROM_ADDR, i, 1, (uint8_t *)&id, 4);
		if (i % 16 == 0)
			printf("\n%02X: ", i);
		printf(" %08X", id); 
	} 
	printf("\n\n");
}
#endif

	/*
	 * i2c controller is initialised in board_init_f->init_func_i2c()
	 * before checkboard is called.
	 */
	/* read assembly ID from main board EEPROM offset 0x4 */
	tmp = 0x20;
	i2c_write(CFG_I2C_SW1_ADDR, 0, 1, &tmp, 1);
	i2c_read(CFG_I2C_EEPROM_ADDR, 4, 1, (uint8_t *)&id, 4);
	i2cid = board_i2c_id(id); /* To change the I2C ID, here is the place */

	switch (i2cid) {
        case I2C_ID_ACX500_O_POE_DC:
               /*
                * only reset the flag as already earlier defaulted
                * to ACX1000
                *
                */
               if (nvram->re_side.default_board) {
                   nvram->re_side.default_board = 0;
                   puts("\n Clearing default_board flag");
               }
                puts("\nBoard: " ACX500_O_POE_DC_NAME_STRING);
                break;
        case I2C_ID_ACX500_O_POE_AC:
               /*
                * only reset the flag as already earlier defaulted
                * to ACX1000
                *
                */
               if (nvram->re_side.default_board) {
                   nvram->re_side.default_board = 0;
                   puts("\n Clearing default_board flag");
               }
                puts("\nBoard: " ACX500_O_POE_AC_NAME_STRING);
                break;
        case I2C_ID_ACX500_O_DC:
                /*
                 * only reset the flag as already earlier defaulted
                 * to ACX1000
                 *
                 */
                if (nvram->re_side.default_board) {
                    nvram->re_side.default_board = 0;
                    puts("\n Clearing default_board flag");
                }
                puts("\nBoard: " ACX500_O_DC_NAME_STRING);
                break;
        case I2C_ID_ACX500_O_AC:
                /*
                 * only reset the flag as already earlier defaulted
                 * to ACX1000
                 *
                 */
                if (nvram->re_side.default_board) {
                    nvram->re_side.default_board = 0;
                    puts("\n Clearing default_board flag");
                }
                puts("\nBoard: " ACX500_O_AC_NAME_STRING);
                break;
        case I2C_ID_ACX500_I_DC:
                /*
                * only reset the flag as already earlier defaulted
                * to ACX1000
                *
                */
               if (nvram->re_side.default_board) {
                   nvram->re_side.default_board = 0;
                   puts("\n Clearing default_board flag");
               }
                puts("\nBoard: " ACX500_I_DC_NAME_STRING);
                break;
       case I2C_ID_ACX500_I_AC:
                /*
                * only reset the flag as already earlier defaulted
                * to ACX1000
                *
                */
               if (nvram->re_side.default_board) {
                   nvram->re_side.default_board = 0;
                   puts("\n Clearing default_board flag");
               }
                puts("\nBoard: " ACX500_I_AC_NAME_STRING);
                break;
        case I2C_ID_ACX1000:
               /*
                * only reset the flag as already earlier defaulted
                * to ACX1000
                *
                */
               if (nvram->re_side.default_board) {
                   nvram->re_side.default_board = 0;
                   puts("\n Clearing default_board flag");
               }
                puts("\nBoard: " ACX1000_NAME_STRING);
                break;
        case I2C_ID_ACX1100:
               /*
                * only reset the flag as already earlier defaulted
                * to ACX1100
                *
                */
               if (nvram->re_side.default_board) {
                   nvram->re_side.default_board = 0;
                   puts("\n Clearing default_board flag");
               }
                puts("\nBoard: " ACX1100_NAME_STRING);
                break;
        case I2C_ID_ACX2000:
               /*
                * If it was defaulted to ACX 1000 board type earlier
                * reset the flag and initialize the values correctly
                *
                */
               if (nvram->re_side.default_board) {
                   nvram->re_side.default_board = 0;
                   nvram->re_side.re_memsize = ACX2000_DEFAULT_RE_MEMSIZE;
                   nvram->re_side.re_memsize_crc =
                       crc32(0, (const unsigned char*)
                             &nvram->re_side.re_memsize, 4);
                   puts("\n Clearing default_board flag");
               }
               puts("\nBoard: " ACX2000_NAME_STRING);
               break;
        case I2C_ID_ACX2100:
               /*
                * If it was defaulted to ACX 1000 board type earlier
                * reset the flag and initialize the values correctly
                *
                */
               if (nvram->re_side.default_board) {
                   nvram->re_side.default_board = 0;
                   nvram->re_side.re_memsize = ACX2000_DEFAULT_RE_MEMSIZE;
                   nvram->re_side.re_memsize_crc =
                       crc32(0, (const unsigned char*)
                             &nvram->re_side.re_memsize, 4);
                   puts("\n Clearing default_board flag");
               }
               puts("\nBoard: " ACX2100_NAME_STRING);
               break;
        case I2C_ID_ACX2200:
               /*
                * If it was defaulted to ACX 1000 board type earlier
                * reset the flag and initialize the values correctly
                *
                */
               if (nvram->re_side.default_board) {
                   nvram->re_side.default_board = 0;
                   nvram->re_side.re_memsize = ACX2000_DEFAULT_RE_MEMSIZE;
                   nvram->re_side.re_memsize_crc =
                       crc32(0, (const unsigned char*)
                             &nvram->re_side.re_memsize, 4);
                   puts("\n Clearing default_board flag");
               }
               puts("\nBoard: " ACX2200_NAME_STRING);
               break;
        case I2C_ID_ACX4000:
               /*
                * If it was defaulted to ACX 1000 board type earlier
                * reset the flag and initialize the values correctly
                *
                */
               if (nvram->re_side.default_board) {
                   nvram->re_side.default_board = 0;
                   nvram->re_side.re_memsize = ACX2000_DEFAULT_RE_MEMSIZE;
                   nvram->re_side.re_memsize_crc =
                       crc32(0, (const unsigned char*)
                             &nvram->re_side.re_memsize, 4);
                   puts("\n Clearing default_board flag");
               }
                puts("\nBoard: " ACX4000_NAME_STRING);
                break;
        default:
               /*
                * Initialize the flag to indicate, we have a default case
                * since ID EEPROM does not have correct board id or not
                * programmed yet.
                *
                * On new boards this location will have random value, then
                * it will be initialized to '1' and will have value '1' until
                * ID EEPROM is programmed, then it will be set to '0'
                *
                * Valid values for ACX platform types are 0x551 (ACX1000),
                * 0x552 (ACX2000), 0x553 (ACX4000), 0x562 (ACX2100),
                * 0x563 (ACX2200) and 0x564 (ACX1100).
                *
                */

		nvram->re_side.default_board = 1;

		puts("Board: No valid ACX board identified! Assuming ACX1000 by default...\n\r");
		puts("\nBoard: " ACX1000_NAME_STRING);
		i2cid = I2C_ID_ACX1000;
		break;
	}

	/* Save for global use by all */
	gd->board_type = i2cid;

	printf(" %u.%u\n", version_major(id), version_minor(id));
	printf("SysPLD:Version %u Rev %u\n", syspld_version(), syspld_revision());
	printf("iCRT booting from: Bank-%d (Active Bank)\n", syspld_get_logical_bank());

	return 0;
}

long int initdram(int board_type)
{
	long dram_size = 0;


	puts("Initializing ");

	dram_size = fixed_sdram(board_type);

	puts("    DDR: ");
	return dram_size;
}

#if defined(CFG_DRAM_TEST)
int testdram(void)
{
	uint *pstart = (uint *) CFG_SDRAM_BASE;
	uint *pend = (uint *) (CFG_SDRAM_BASE + CFG_SDRAM_SIZE);
	uint *p;

	/*
	 * A simple DRAM test like this takes 8 seconds to execute
	 * It makes sense to NOT do this at every SW reboot to speedd up boot.
	 * It is OK because since SW rebooted the chassis, most likely the RAM
	 * is OK. In case it is not, the user can always power cycle, or at WDT
	 * expiry this shall be run anyway.
	 */
	if (syspld_get_reboot_reason() == RESET_REASON_SW_INITIATED) {
	    return 0;
	}

	printf("DRAM test phase 1:\n");
	for (p = pstart; p < pend; p += CFG_MEMTEST_INCR/4)
		*p = 0xaaaaaaaa;

	for (p = pstart; p < pend; p += CFG_MEMTEST_INCR/4) {
		if (*p != 0xaaaaaaaa) {
			printf ("DRAM test fails at: %08x\n", (uint) p);
			return 1;
		}
	}

	printf("DRAM test phase 2:\n");
	for (p = pstart; p < pend; p += CFG_MEMTEST_INCR/4)
		*p = 0x55555555;

	for (p = pstart; p < pend; p += CFG_MEMTEST_INCR/4) {
		if (*p != 0x55555555) {
			printf ("DRAM test fails at: %08x\n", (uint) p);
			return 1;
		}
	}

	printf("DRAM test passed.\n");
	return 0;
}
#endif

void acx_cache_invalidate_flash(void)
{
        unsigned int loop;

        debug("acx_cache_invalidate_flash -- remap TLB for Boot Flash as caching-inhibited\n");
        for (loop = 0; loop < CFG_FLASH_SIZE; loop += 32) {
                asm volatile ("dcbi %0,%1": : "b" (CFG_FLASH_BASE), "r" (loop));
                asm volatile ("icbi %0,%1": : "b" (CFG_FLASH_BASE), "r" (loop));
        }

        /* ***** - local bus: Boot flash invalidate */
        mtspr(MAS0, TLB1_MAS0(1, 2, 0)); /* tlbsel=1 esel=2 nv=0 */
        mtspr(MAS1, TLB1_MAS1(0, 1, 0, 0, BOOKE_PAGESZ_4M));    /* V=0 */
        mtspr(MAS2, TLB1_MAS2(E500_TLB_EPN(CFG_FLASH_BASE),
                                0, 0, 0, 0, 0, 0, 0, 0));
        mtspr(MAS3, TLB1_MAS3(E500_TLB_RPN(CFG_FLASH_BASE),
                                0, 0, 0, 0, 0, 1, 0, 1, 0, 1));
        asm volatile("isync;msync;tlbwe;isync");

        /* *I*G* - local bus: Boot flash*/
        mtspr(MAS0, TLB1_MAS0(1, 2, 0)); /* tlbsel=1 esel=2 nv=0 */
        mtspr(MAS1, TLB1_MAS1(1, 1, 0, 0, BOOKE_PAGESZ_4M));    /* V=1 */
        mtspr(MAS2, TLB1_MAS2(E500_TLB_EPN(CFG_FLASH_BASE),
                                0, 0, 0, 0, 1, 0, 1, 0));/* Non-cacheable */
        mtspr(MAS3, TLB1_MAS3(E500_TLB_RPN(CFG_FLASH_BASE),
                                0, 0, 0, 0, 0, 1, 0, 1, 0, 1));
        asm volatile("isync;msync;tlbwe;isync");

}

void acx_cache_enable_flash(void)
{
        /* ***** - local bus: Boot flash invalidate */
        debug("\n Invalidating cache ");
        mtspr(MAS0, TLB1_MAS0(1, 2, 0)); /* tlbsel=1 esel=2 nv=0 */
        mtspr(MAS1, TLB1_MAS1(0, 1, 0, 0, BOOKE_PAGESZ_4M));    /* V=0 */
        mtspr(MAS2, TLB1_MAS2(E500_TLB_EPN(CFG_FLASH_BASE),
                                0, 0, 0, 0, 0, 0, 0, 0));
        mtspr(MAS3, TLB1_MAS3(E500_TLB_RPN(CFG_FLASH_BASE),
                                0, 0, 0, 0, 0, 1, 0, 1, 0, 1));
        asm volatile("isync;msync;tlbwe;isync");

        /* cacheable - local bus: Boot flash*/
        mtspr(MAS0, TLB1_MAS0(1, 2, 0)); /* tlbsel=1 esel=2 nv=0 */
        mtspr(MAS1, TLB1_MAS1(1, 1, 0, 0, BOOKE_PAGESZ_4M));    /* V=1 */
        mtspr(MAS2, TLB1_MAS2(E500_TLB_EPN(CFG_FLASH_BASE),
                                0, 0, 0, 1, 0, 0, 1, 0));/* cacheable */
        mtspr(MAS3, TLB1_MAS3(E500_TLB_RPN(CFG_FLASH_BASE),
                                0, 0, 0, 0, 0, 1, 0, 1, 0, 1));
        asm volatile("isync;msync;tlbwe;isync");
}

int last_stage_init(void)
{
	char temp[60];
	uchar enet[6];
	uint16_t esize = 0;
	uint8_t i;
	char *s, *e;
	bd_t *bd;
	int dev;
        int uboot_upgrade = 0;

	bd = gd->bd;

	/*
	 * Read a valid MAC address and set "ethaddr" environment variable
	 */
	i2c_read(CFG_I2C_EEPROM_ADDR, CFG_EEPROM_MAC_OFFSET, 1, enet, 6);
	i2c_read(CFG_I2C_EEPROM_ADDR, CFG_EEPROM_MAC_OFFSET-2, 1, (unsigned char*)&esize, 1);
	i2c_read(CFG_I2C_EEPROM_ADDR, CFG_EEPROM_MAC_OFFSET-1, 1, ((unsigned char*)&esize) + 1, 1);

	if (esize) {
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
		*(uint16_t *)(enet + 4) += (esize - 1);
	}

	sprintf(temp, "%02x:%02x:%02x:%02x:%02x:%02x",
			enet[0], enet[1], enet[2], enet[3], enet[4], enet[5]);
	s = getenv("eth0addr");
	if (memcmp (s, temp, sizeof(temp)) != 0) {
		setenv("eth0addr", temp);
	}

	switch (gd->board_type) {
	case I2C_ID_ACX1000:
		sprintf(temp, "%s", ACX1000_NAME_STRING);
		break;
	case I2C_ID_ACX2000:
		sprintf(temp, "%s", ACX2000_NAME_STRING);
		break;
	case I2C_ID_ACX4000:
		sprintf(temp, "%s", ACX4000_NAME_STRING);
		break;
        case I2C_ID_ACX2100:
                sprintf(temp, "%s", ACX2100_NAME_STRING);
                break;
        case I2C_ID_ACX2200:
                sprintf(temp, "%s", ACX2200_NAME_STRING);
                break;
        case I2C_ID_ACX1100:
                sprintf(temp, "%s", ACX1100_NAME_STRING);
                break;
        case I2C_ID_ACX500_O_POE_DC:
                sprintf(temp, "%s", ACX500_O_POE_DC_NAME_STRING);
                break;
        case I2C_ID_ACX500_O_POE_AC:
                sprintf(temp, "%s", ACX500_O_POE_AC_NAME_STRING);
                break;
        case I2C_ID_ACX500_O_DC:
                sprintf(temp, "%s", ACX500_O_DC_NAME_STRING);
                break;
        case I2C_ID_ACX500_O_AC:
                sprintf(temp, "%s", ACX500_O_AC_NAME_STRING);
                break;
        case I2C_ID_ACX500_I_DC:
                sprintf(temp, "%s", ACX500_I_DC_NAME_STRING);
                break;
        case I2C_ID_ACX500_I_AC:
                sprintf(temp, "%s", ACX500_I_AC_NAME_STRING);
                break;
        default:
		sprintf(temp, "ACX Default");
		break;
	}

	setenv("hw.board.type", temp);

	/*
	 * set eth active device as eTSEC1 
	 */
	setenv("ethact", CONFIG_MPC85XX_TSEC1_NAME);
	
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
	setenv("bootp_if", CONFIG_MPC85XX_TSEC1_NAME);

	/*
	 * Set upgrade environment variables
	 */
	sprintf(temp,"0x%08x ", CFG_UPGRADE_LOADER_ADDR);
	setenv("boot.upgrade.loader", temp);

	sprintf(temp,"0x%08x ", CFG_UPGRADE_BOOT_IMG_ADDR);
	setenv("boot.upgrade.uboot", temp);

	/* Calculate the memory to be handed over to RE */
	cal_re_memsize();

        /*
         * Execute ACX-500 Secure Boot Functionality.
         * Initialize TPM, Measure U-Boot/Loader, Soft Reset CPU
         * Store the hash values in environment variables, Set the
         * Glass Safe Register according to Security Posture etc.
         */
        switch (gd->board_type) {
            case I2C_ID_ACX500_O_POE_DC:
            case I2C_ID_ACX500_O_POE_AC:
            case I2C_ID_ACX500_O_DC:
            case I2C_ID_ACX500_O_AC:
            case I2C_ID_ACX500_I_DC:
            case I2C_ID_ACX500_I_AC:
                /* Jumper detect check */
                if (syspld_jumper_detect()) {
                    printf("Recovery Jumper is installed.\n");
                } else {
                    printf("Recovery Jumper is not installed.\n");
                }

                if (is_external_usb_present(&dev)) {
                    printf("USB presence detected: dev %d\n", dev);
                    uboot_upgrade = acx_upgrade_images();

		    if (uboot_upgrade)
			printf("\nRecovery Image processed, Please eject USB.\n");
                } else {
                    printf("USB presence not detected.\n");
                }

                strncpy((char *) nvram->re_side.icrt_version, version_string,
                        ICRT_VERSION_LEN);

                acx_icrt_uboot_jump();
                break;
            default:
                break;
        }

	return 0;
}

int board_early_init_r(void)
{
	unsigned int i;

	debug("board_early_init_r -- remap TLB for Boot Flash as caching-inhibited\n");
	for (i = 0; i < CFG_FLASH_SIZE; i += 32) {
		asm volatile ("dcbi %0,%1": : "b" (CFG_FLASH_BASE), "r" (i));
		asm volatile ("icbi %0,%1": : "b" (CFG_FLASH_BASE), "r" (i));
	}

	/* ***** - local bus: Boot flash invalidate */
	mtspr(MAS0, TLB1_MAS0(1, 2, 0)); /* tlbsel=1 esel=2 nv=0 */
	mtspr(MAS1, TLB1_MAS1(0, 1, 0, 0, BOOKE_PAGESZ_4M));	/* V=0 */
	mtspr(MAS2, TLB1_MAS2(E500_TLB_EPN(CFG_FLASH_BASE),
				0, 0, 0, 0, 0, 0, 0, 0));
	mtspr(MAS3, TLB1_MAS3(E500_TLB_RPN(CFG_FLASH_BASE),
				0, 0, 0, 0, 0, 1, 0, 1, 0, 1));
	asm volatile("isync;msync;tlbwe;isync");

	/* *I*G* - local bus: Boot flash*/
	mtspr(MAS0, TLB1_MAS0(1, 2, 0)); /* tlbsel=1 esel=2 nv=0 */
	mtspr(MAS1, TLB1_MAS1(1, 1, 0, 0, BOOKE_PAGESZ_4M));	/* V=1 */
	mtspr(MAS2, TLB1_MAS2(E500_TLB_EPN(CFG_FLASH_BASE),
				0, 0, 0, 0, 1, 0, 1, 0));/* Non-cacheable */
	mtspr(MAS3, TLB1_MAS3(E500_TLB_RPN(CFG_FLASH_BASE),
				0, 0, 0, 0, 0, 1, 0, 1, 0, 1));
	asm volatile("isync;msync;tlbwe;isync");

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
	extern int i2c_usb2513_init(void);
	if (i2c_usb2513_init())
		printf ("USB HUB initialization failed\n");

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

	return i2c_write(CFG_I2C_USB2513_ADDR, reg, 1, tmp, 2);
}

/*
 * Write the string in UTF-16LE to the specified
 * location
 */
static int i2c_usb2513_write_utf16le (u_int8_t reg, char *str)
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
int i2c_usb2513_init (void)
{
	int error;
	u_int16_t ind;
	uint8_t tmp, config_data3;

	/*
	 * Open the I2C multiplxer for USB hub ac access
	 */
	tmp = 0x8;
	i2c_write(CFG_I2C_SW1_ADDR, 0, 1, &tmp, 1);


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
        config_data3 = USB2513_CONF3_LED_MODE_SPEED |
                       USB2513_CONF3_STRING_SUPPORT_EN;

        if (IS_BOARD_ACX500(gd->board_type)) {
            /*
             * On Kotinos, USB Hub downstream port connection is different from
             * fortius, so port mapping feature is used to match with fortius
             * port connection. 
             * Port mapping would be:
             * Port 1 -> Port 3 (Internal Flash controller port)
             * Port 2 -> Port 1 (External USB port)
             * Port 3 -> Port 2 (Unused on Kotinos)
             */
            config_data3 |= USB2513_CONF3_PORTMAP_EN;
        }

	error = usb2513_write(USB2513_CONF3, config_data3);
	if (error)
		return error;

        if (IS_BOARD_ACX500(gd->board_type)) {
            /*
             * Map the physical port 1 to port3 and port2 to port1 to match with
             * fortius
             */
            error = usb2513_write(USB2513_PORT_MAP12, USB2513_PORTMAP_P2_TO_P1 |
                                  USB2513_PORTMAP_P1_TO_P3);
            if (error)
                return error;

            /*
             * Map port 3 to port 2
             */
            error = usb2513_write(USB2513_PORT_MAP34, USB2513_PORTMAP_P3_TO_P2);
            if (error)
                return error;

            /*
             * Disable remaining ports those are not available on usb2513 (3
             * downstream ports) device
             */
            error = usb2513_write(USB2513_PORT_MAP56, 0x00);
            if (error)
                return error;

            error = usb2513_write(USB2513_PORT_MAP7, 0x00);
            if (error)
                return error;
        }

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
	if (IS_BOARD_ACX500(gd->board_type)) {
	    for (ind = USB2513_BAT_CHARG_EN + 1; ind <= USB2513_PORT_SWAP; ind++) {
                error = usb2513_write((u_int8_t)ind, 0);
                if (error)
                    return error;
            }
        } else {
	    for (ind = USB2513_BAT_CHARG_EN + 1; ind <= USB2513_PORT_MAP7; ind++) {
                error = usb2513_write((u_int8_t)ind, 0);
                if (error)
                    return error;
            }
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
	i2c_write(CFG_I2C_SW1_ADDR, 0, 1, &tmp, 1);

	return 0;
}

int get_bios_upgrade(void)
{
    return (nvram->re_side.bios_upgrade);
}

void set_bios_upgrade(uint8_t value)
{
    nvram->re_side.bios_upgrade = value;
}

int is_acx500_board(void)
{
    if (IS_BOARD_ACX500(gd->board_type))
	return 1;
    else
	return 0;
}
