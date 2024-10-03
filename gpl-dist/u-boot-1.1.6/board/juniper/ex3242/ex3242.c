/*
 * Copyright (c) 2006-2012, Juniper Networks, Inc.
 * All rights reserved.
 *
 * Copyright 2004 Freescale Semiconductor.
 *
 * (C) Copyright 2002 Scott McNutt <smcnutt@artesyncp.com>
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

#include <common.h>
#include <i2c.h>
#include "fsl_i2c.h"
#include "epld.h"
#include "epld_watchdog.h"
#include "led.h"
#include "lcd.h"
#include "tsec.h"
#if defined(CONFIG_NOR)
#include "nor.h"
#endif
#include <pci.h>
#include <asm/processor.h>
#include <asm/immap_85xx.h>
#include <asm/io.h>
#include <spd.h>
#include <usb.h>
#include <nand.h>
#if 0
#include <miiphy.h>
#endif

#if defined(CONFIG_DDR_ECC)
extern void ddr_enable_ecc(unsigned int dram_size, int enable);
#endif

#define	EX3242_DEBUG
#undef debug
#ifdef	EX3242_DEBUG
#define debug(fmt,args...)	printf (fmt ,##args)
#else
#define debug(fmt,args...)
#endif

DECLARE_GLOBAL_DATA_PTR;

int secure_eeprom_read (unsigned dev_addr, unsigned offset, uchar *buffer, unsigned cnt);
int secure_eeprom_write (unsigned i2c_addr, int addr, void *in_buf, int len );

typedef enum upgrade_state 
{
    UPGRADE_READONLY = 0,  /* set by readonly u-boot */
    UPGRADE_START,      /* set by loader/JunOS */
    UPGRADE_CHECK,      /* set by readonly U-boot */
    UPGRADE_TRY,          /* set by readonly U-boot */
    UPGRADE_OK             /* set by upgrade U-boot */
} upgrade_state_t;

int usb_scan_dev = 0;
extern long int spd_sdram(void);

extern int usb_stor_curr_dev; /* current device */

static struct tsec_private *mgmt_priv = NULL;

extern int memtest (ulong saddr, ulong eaddr, int mloop, int display);
extern uint32_t diag_ether_loopback(int loopback); 
extern int ether_tsec_reinit(struct eth_device *dev);

int check_internal_usb_storage_present(void);
void toggle_usb(void);
uint16_t version_epld(void);

extern int post_result_cpu;
extern int post_result_mem;
extern int post_result_rom;
extern int post_result_ether;

/* UPM (User Programmable Machine) table for NAND interface */
static uint32_t UPMAtable[] =
{
    0x0ff03c30, 0x0ff03c30, 0x0ff03c34, 0x0ff33c30, /* 0 - 3 */
    0xfff33c31, 0xfffffc30, 0xfffffc30, 0xfffffc30, /* 4 - 7 */
    0x0faf3c30, 0x0faf3c30, 0x0faf3c30, 0x0fff3c34, /* 8 - 11 */
    0xffff3c31, 0xfffffc30, 0xfffffc30, 0xfffffc30, /* 12 - 15 */
    0x0fa3fc30, 0x0fa3fc30, 0x0fa3fc30, 0x0ff3fc34, /* 16 - 19 */
    0xfff3fc31, 0xfffffc30, 0xfffffc30, 0xfffffc30, /* 20 - 23 */
    0x0ff33c30, 0x0fa33c30, 0x0fa33c34, 0x0ff33c30, /* 24 - 27 */
    0xfff33c31, 0xfff0fc30, 0xfff0fc30, 0xfff0fc30, /* 28 - 31 */
    0xfff3fc30, 0xfff3fc30, 0xfff6fc30, 0xfffcfc30, /* 32 - 35 */
    0xfffcfc30, 0xfffcfc30, 0xfffcfc30, 0xfffcfc30, /* 36 - 39 */
    0xfffcfc30, 0xfffcfc30, 0xfffcfc30, 0xfffcfc30, /* 40 - 43 */
    0xfffdfc30, 0xfffffc30, 0xfffffc30, 0xfffffc31, /* 44 - 47 */
    0xfffffc30, 0xfffffc00, 0xfffffc00, 0xfffffc00, /* 48 - 51 */
    0xfffffc00, 0xfffffc00, 0xfffffc00, 0xfffffc00, /* 52 - 55 */
    0xfffffc00, 0xfffffc00, 0xfffffc00, 0xfffffc01, /* 56 - 59 */
    0xfffffc00, 0xfffffc00, 0xfffffc00, 0xfffffc01, /* 60 - 63 */
};

void
UPMA_prog (void)
{
    int i;

    /* OP set to write to RAM array command */
    *(uint32_t *)CFG_MAMR = MAMR_WRITE_UPM;

    /* write to RAM array */
    for (i = 0; i < 64; i++) {
        *(uint32_t *)CFG_MDR = UPMAtable[i];
        /* dummy write */
        *(uint8_t *)CFG_NAND_BASE = 0;
        udelay(1);
    }
    
    /* OP set to normal mode */
    *(uint32_t *)CFG_MAMR = 0;

    /* wait till write finish */
    while (*(uint32_t *)CFG_MAMR != 0);
}

void
UPMA_dump (void)
{
    int i;
    uint8_t temp;
    uint32_t data;

    /* OP set to read from RAM array command */
    *(uint32_t *)CFG_MAMR = MAMR_READ_UPM;

    printf("RAM array:\n");

    /* write to RAM array */
    for (i = 0; i < 64; i++) {
        /* dummy read */
        temp = *(uint8_t *)CFG_NAND_BASE;
        data = *(uint32_t *)CFG_MDR;
        if (i % 4 == 0)
            printf("\n");
        printf("%08x ", data);
        udelay(1);
    }
    printf("\n\n");
    
    /* OP set to normal mode */
    *(uint32_t *)CFG_MAMR = 0;

    /* wait till write finish */
    while (*(uint32_t *)CFG_MAMR != 0);
}

/* UPM pattern */
void
run_pattern_1 (uint32_t command, uint32_t pattern_offset)
{
    /* MAMR */
    *(uint32_t *)CFG_MAMR |= (MAMR_RUN_PATTERN | pattern_offset);
    return;
}

/* command */
void
run_pattern_2 (uint32_t command, uint32_t pattern_offset)
{
    /* MAR */
    *(uint32_t *)CFG_MAR = (command << 24);

    /* dummy write */
    *(uint8_t *)CFG_NAND_BASE = 0;
    return;
}

/* reset */
void
run_pattern_3 (uint32_t command, uint32_t pattern_offset)
{
    /* MAMR set to normal mode */
    *(uint32_t *)CFG_MAMR &= MAMR_OP_NORMAL;

    /* wait till write finish */
    while (((*(uint32_t *)CFG_MAMR) & MAMR_OP_MASK) != 0);
    
    return;
}

void
run_pattern (uint32_t command, uint32_t pattern_offset)
{
    run_pattern_1(command, pattern_offset);
    run_pattern_2(command, pattern_offset);
    run_pattern_3(command, pattern_offset);
}

int 
nand_ready (void)
{
    volatile immap_t *immap = (immap_t *) CFG_CCSRBAR;
    volatile ccsr_gur_t *gur = &immap->im_gur;
    
    return (gur->gpindr & GPIO_NAND_READY);
}

int
wait_n (int n)
{
    int i = 0;
    int temp = 0;

    for (i = 0; i < n; i++)
        temp = nand_ready();

    return temp;
}

static int 
ex_nand_dev_ready (struct mtd_info *mtdinfo)
{
    volatile immap_t *immap = (immap_t *) CFG_CCSRBAR;
    volatile ccsr_gur_t *gur = &immap->im_gur;
    
    return (gur->gpindr & GPIO_NAND_READY);
}

static void 
ex_nand_select_chip (struct mtd_info *mtd, int chip)
{
    /* do nothing */
    return;
}

static void 
ex_nand_read_buf(struct mtd_info *mtd, u_char *buf, int len)
{
    int i;
    struct nand_chip *this = mtd->priv;

    for (i = 0; i < len; i++) {
        buf[i] = readb(this->IO_ADDR_R);
        wait_n(1);
    }
}

static void 
ex_nand_write_buf(struct mtd_info *mtd, const u_char *buf, int len)
{
    int i;

    run_pattern_1(0, MAD_WRITE_DATA_PATTERN);
    wait_n(CONFIG_NAND_WR_DELAY_EX);
    for (i = 0; i < len; i++) {
        run_pattern_2(buf[i], 0);
        wait_n(CONFIG_NAND_CMD_DELAY_EX);
    }
    run_pattern_3(0, 0);
    run_pattern(NAND_CMD_PAGEPROG, MAD_WRITE_COMMAND_PATTERN);
    udelay(CONFIG_NAND_WRITE_DELAY_EX);
}

static void 
ex_nand_command_lp (struct mtd_info *mtd, unsigned command, int column, int page_addr)
{
    register struct nand_chip *this = mtd->priv;

    /* Emulate NAND_CMD_READOOB */
    if (command == NAND_CMD_READOOB) {
        column += mtd->oobblock;
        command = NAND_CMD_READ0;
    }

    /* command */
    run_pattern(command, MAD_WRITE_COMMAND_PATTERN);
    wait_n(CONFIG_NAND_DELAY_EX);

    if (column != -1 || page_addr != -1) {
        /* Serially input address */
        run_pattern_1(0, MAD_WRITE_ADDRESS_PATTERN);
        wait_n(CONFIG_NAND_CMD_DELAY_EX);
        if (column != -1) {
            /* column address */
            if (command == NAND_CMD_READID) {
                run_pattern_2(0, 0);
            }
            else {
                run_pattern_2(column & 0xff, 0);
                wait_n(CONFIG_NAND_CMD_DELAY_EX);
                run_pattern_2(column >> 8, 0);
                wait_n(CONFIG_NAND_CMD_DELAY_EX);
            }
        }
        
        if (page_addr != -1) {
            /* page address */
            run_pattern_2(page_addr & 0xff, 0);
            wait_n(CONFIG_NAND_CMD_DELAY_EX);
            run_pattern_2((page_addr >> 8) & 0xff, 0);
            wait_n(CONFIG_NAND_CMD_DELAY_EX);
            if (this->chipsize > (128 << 20)) {
                run_pattern_2((page_addr >> 16) & 0xff, 0);
                wait_n(CONFIG_NAND_CMD_DELAY_EX);
            }
        }
        run_pattern_3(0, 0);
    }

    /*
     * program and erase have their own busy handlers
     * status and sequential in needs no delay
     */
    switch (command) {
        case NAND_CMD_CACHEDPROG:
        case NAND_CMD_PAGEPROG:
        case NAND_CMD_ERASE1:
        case NAND_CMD_ERASE2:
        case NAND_CMD_SEQIN:
        case NAND_CMD_STATUS:
            return;

        case NAND_CMD_RESET:
            if (this->dev_ready)
                break;
            udelay(this->chip_delay);
            /* command */
            run_pattern(NAND_CMD_STATUS, 8);
            while (!(this->read_byte(mtd) & 0x40));
            return;

        case NAND_CMD_READ0:
            /* command */
            run_pattern(NAND_CMD_READSTART, 8);
            /* Fall through into ready check */

        /* This applies to read commands */
        default:
        
            /*
             * If we don't have access to the busy pin, we apply the given
             * command delay
             */
            if (!this->dev_ready) {
                udelay (this->chip_delay);
                return;
            }
    }

    /* Apply this short delay always to ensure that we do wait tWB in
     * any case on any machine. 
     */
    udelay (100);
    /* wait until command is processed */
    while (!this->dev_ready(mtd));
}

int 
board_nand_init (struct nand_chip *nand)
{    
    UPMA_prog();

    /* initialize MAMR */
    *(uint32_t *)CFG_MAMR = 0x00004440;

    nand->eccmode = NAND_ECC_SOFT;
    nand->options = NAND_SAMSUNG_LP_OPTIONS;
    nand->chip_delay = 25;
    nand->hwcontrol  = NULL;
    nand->select_chip = ex_nand_select_chip;
    nand->dev_ready	= ex_nand_dev_ready;
    nand->read_buf	= ex_nand_read_buf;
    nand->write_buf	= ex_nand_write_buf;
    nand->cmdfunc = ex_nand_command_lp;

    return (0);
}

#ifdef CONFIG_NAND_DIRECT
void
ex_nand_init (void)
{
    int bid = board_id();

    if ((bid == I2C_ID_JUNIPER_EX4210_24_P) || 
        (bid == I2C_ID_JUNIPER_EX4210_48_P)) {
        printf("NAND:  ");
        nand_init();
    }
}
#endif

void local_bus_init(void);

int console_silent_init (void)
{

#ifdef CONFIG_SILENT_CONSOLE
	if (getenv("silent") != NULL)
		gd->flags |= GD_FLG_SILENT;
#endif

	return (0);
}

int set_upgrade_state (upgrade_state_t state)
{
    ulong	upgrade_addr = CFG_FLASH_BASE + CFG_UPGRADE_BOOT_STATE_OFFSET;
    ulong ustate = state;
    int flash_write_direct (ulong addr, char *src, ulong cnt);

    return  flash_write_direct(upgrade_addr, (char *)&ustate, sizeof(ulong));
}

int get_upgrade_state (void)
{
    volatile upgrade_state_t *ustate = (upgrade_state_t *) (CFG_FLASH_BASE + CFG_UPGRADE_BOOT_STATE_OFFSET);

    if ((*ustate < UPGRADE_READONLY) || (*ustate > UPGRADE_OK))
    {
        set_upgrade_state(UPGRADE_READONLY);
        return UPGRADE_READONLY;
    }
    
    return (*ustate);
}

int valid_env (unsigned long addr)
{
    return 1;
}

int valid_uboot_image (unsigned long addr)
{
    ulong icrc;
    image_header_t *haddr = (image_header_t *) (addr + CFG_IMG_HDR_OFFSET);

    if ((icrc = crc32(0, (unsigned char *)(addr+CFG_CHKSUM_OFFSET),
                   CFG_MONITOR_LEN -CFG_CHKSUM_OFFSET)) == haddr->ih_dcrc) {
//        printf("0x%x/0x%x\n", icrc, haddr->ih_dcrc);
        return 1;
    }

    return 0;
}

int check_upgrade (void)
{
#if defined(CONFIG_READONLY)
    ulong addr;
    extern int valid_elf_image (unsigned long addr);
    upgrade_state_t state = get_upgrade_state();
    
    if (state == UPGRADE_START) {
        /* verify upgrade */
        if (valid_env(CFG_FLASH_BASE) &&
             valid_elf_image(CFG_UPGRADE_LOADER_ADDR) &&
             valid_uboot_image(CFG_UPGRADE_BOOT_IMG_ADDR)) {
             set_upgrade_state(UPGRADE_CHECK);
        }
        else {
             set_upgrade_state(UPGRADE_READONLY);
        }
    }
    else if (state == UPGRADE_TRY) {
        /* Something is wrong.  Upgrade did not boot up.  Clear the state.  */
        set_upgrade_state(UPGRADE_READONLY);
    }
    
    state = get_upgrade_state();
    /* boot from upgrade check */
    if ((state == UPGRADE_OK) || (state == UPGRADE_CHECK)) {
        
        /* First time to boot Upgrade */
        set_upgrade_state(UPGRADE_TRY);
        
        /* select upgrade image to be boot from */
        addr = CFG_UPGRADE_BOOT_ADDR;
        ((ulong (*)())addr) ();;
    }
    
#if 0    
       udelay(3000000);
	if (tstc()) {	/* we got a key press	*/
	    while (tstc ())    
	        (void) getc();  /* consume input	*/
	    printf("Boot:  1. upgrade  2. readonly\n");
           if (getc() == '1') {
               printf("Boot: upgrade\n");
           addr = CFG_UPGRADE_LOADER_ADDR;
           ((ulong (*)())addr) ();;
           }
	}
#endif
#endif
    return 0;
}

int set_upgrade_ok (void)
{
    uint16_t epld_version;
    
#if defined(CONFIG_UPGRADE)
    unsigned char temp[5];
    ulong state = get_upgrade_state();

    /* Upgrade U-boot is running from RAM. */
    if (state == UPGRADE_TRY) {
        set_upgrade_state(UPGRADE_OK);
        state = get_upgrade_state();
    }

    /* environment variable current U-boot image */
    setenv("boot.current", "upgrade");

    sprintf(temp,"%d ", state);
    setenv("boot.state", temp);
#endif

    epld_version = version_epld();
    if ((epld_version >> 8) >= 7) {
        /* In case of internal storage not detected, toggle EPLD register 6
         * bit 1 to kill and restore 3.3V to ISP1564 (power cycle) and then
         * reset.  The internal USB storage shall be detected after bootup.
         * This workaround is applicable to EPLD version 7 and above.
         */
        if (!check_internal_usb_storage_present()) {
             toggle_usb();
             epld_system_reset();
        }
    }

    return  0;
}

int board_id(void)
{
       return ((gd->board_type & 0xffff0000) >> 16);
}

int version_major(void)
{
       return ((gd->board_type & 0xff00) >> 8);
}

int version_minor(void)
{
       return (gd->board_type & 0xff);
}

uint16_t version_epld(void)
{
       volatile uint16_t ver;

       epld_reg_read(EPLD_WATCHDOG_SW_UPDATE, (uint16_t *)&ver);
       return (ver);
}

int mem_ecc(void)
{
       if (!gd->valid_bid)
           return 0;

       switch (board_id()) {
           case I2C_ID_JUNIPER_EX4200_24_F:
           case I2C_ID_JUNIPER_EX4200_24_T:
           case I2C_ID_JUNIPER_EX4200_24_P:
           case I2C_ID_JUNIPER_EX4200_48_T:
           case I2C_ID_JUNIPER_EX4200_48_P:
           case I2C_ID_JUNIPER_EX4210_24_P:
           case I2C_ID_JUNIPER_EX4210_48_P:
		 return 1;
               break;
           case I2C_ID_JUNIPER_EX3200_24_T:
           case I2C_ID_JUNIPER_EX3200_24_P:
           case I2C_ID_JUNIPER_EX3200_48_T:
           case I2C_ID_JUNIPER_EX3200_48_P:
		if (version_major() >= 2)
			return 0;

		 if (((gd->mem_cfg >= 0x30) && (gd->mem_cfg <= 0x33)) &&
		     ((gd->mem_cfg & 0x1) == 0))
		     return 1;
		 return 0;
               break;
           default:
		 return 0;
               break;
       };
       return 0;
}


int mem_size(void)
{
    if (!gd->valid_bid)
        return CFG_SDRAM_SIZE_EX3200;

    if (version_major() >= 2) {
       switch (board_id()) {
           case I2C_ID_JUNIPER_EX4200_24_F:
           case I2C_ID_JUNIPER_EX4200_24_T:
           case I2C_ID_JUNIPER_EX4200_24_P:
           case I2C_ID_JUNIPER_EX4200_48_T:
           case I2C_ID_JUNIPER_EX4200_48_P:
           case I2C_ID_JUNIPER_EX4210_24_P:
           case I2C_ID_JUNIPER_EX4210_48_P:
		 return CFG_SDRAM_SIZE_EX4200;
               break;
           case I2C_ID_JUNIPER_EX3200_24_T:
           case I2C_ID_JUNIPER_EX3200_24_P:
           case I2C_ID_JUNIPER_EX3200_48_T:
           case I2C_ID_JUNIPER_EX3200_48_P:
		 return CFG_SDRAM_SIZE_EX3200;
               break;
           default:
		 return CFG_SDRAM_SIZE_EX3200;
               break;
       };
    }
    else {
        if (((gd->mem_cfg >= 0x30) && (gd->mem_cfg <= 0x33)) &&
	     ((gd->mem_cfg & 0x2) == 0x2))
            return CFG_SDRAM_SIZE_EX4200;
    }
	
    return CFG_SDRAM_SIZE_EX3200;
}

int board_early_init_f (void)
{
	volatile immap_t *immap = (immap_t *) CFG_CCSRBAR;
	volatile ccsr_gur_t *gur = &immap->im_gur;
	volatile ccsr_lbc_t *lbc = &immap->im_lbc;
       volatile uint16_t epldMiscCtrl;
//       volatile uint32_t *srdscr;
	   
	lbc->ltedr = 0x20000000;  /* PARD - disable parity error checking */
	lbc->lbcr |= 0x20000;     /* LPBSE - LGPL4 parity byte select output */

	gur->clkocr = 0x80000000;	/* enable CLK_OUT */

	gur->gpiocr = 0x00010200; /* enable GPIN[0-7] and GPOUT[0-7] */
       /* toggle all devices */
	gur->gpoutdr = 0x04000000; /* all in reset */        
       udelay(10000);
	gur->gpoutdr = 0x19000000; /* ISP1564, ST72681, W83782G, I2C out of reset */        
	
       /* enable secure EEPROM */
       /* L24f */
       epld_reg_read(EPLD_MISC_CONTROL_0, (uint16_t *)&epldMiscCtrl);
       epld_reg_write(EPLD_MISC_CONTROL_0, epldMiscCtrl |EPLD_REMOVE_SIC_RESET);
       /* all other Java SKU */
       epld_reg_read(EPLD_MISC_CONTROL, (uint16_t *)&epldMiscCtrl);
       epld_reg_write(EPLD_MISC_CONTROL, epldMiscCtrl |EPLD_UPLINK_HOTSWAP_LED_ON);

#if 0
       /* Set transmit equalization of SerDes 1 and 2 to 2x. */
       srdscr = 0xfefe0f04;
       *srdscr |= 0x6600;
       srdscr = 0xfefe0f10;
       *srdscr |= 0x6600;
#endif

       lcd_off();
	return 0;
}

int checkboard (void)
{
       volatile uint16_t epldMiscCtrl, epld_ver, epldLastRst;
       unsigned char ch = 0x10, mcfg;
       uint32_t id, bid = 0;
       unsigned char swJumper;

       i2c_controller(CFG_I2C_CTRL_1);
	i2c_init (CFG_I2C_SPEED, CFG_I2C_SLAVE);

       /* read assembly ID from main board EEPROM offset 0x4 */
       /* select i2c controller 1 */
       ch = 0x10;
	i2c_write(CFG_I2C_CTRL_1_SW_ADDR, 0, 0, &ch, 1);
	gd->secure_eeprom = 0;
	if (i2c_read_norsta(CFG_I2C_SECURE_EEPROM_ADDR, 4, 1, (uint8_t *)&id, 4) == 0) {
	    i2c_read_norsta(CFG_I2C_SECURE_EEPROM_ADDR, 0x72, 1, (uint8_t *)&mcfg, 1);
	    gd->secure_eeprom = 1;
	}
       else {
	    i2c_read(CFG_I2C_MAIN_EEPROM_ADDR, 4, 1, (uint8_t *)&id, 4);
	    i2c_read(CFG_I2C_MAIN_EEPROM_ADDR, 0x72, 1, (uint8_t *)&mcfg, 1);
       }
       ch = 0;
       i2c_write(0x70, 0, 0, &ch, 1); /* disable mux channel */
	gd->mem_cfg = mcfg;
       bid = (id & 0xffff0000) >> 16;

       if (i2c_controller_read(CFG_I2C_CTRL_2, CFG_I2C_CTRL_2_IO_ADDR, 0x4, 1, &swJumper, 1) == 0) {
           if ((swJumper & 0x80) == 0) {
               /* force to default */
	        gd->mem_cfg = 0xff;
               bid = 0xffff;
           }
       }
       
       /* select i2c controller 1 */
       i2c_controller(CFG_I2C_CTRL_1);
#if CFG_TSEC_KEYPRESS_ABORT    
       if (tstc()) {	/* we got a key press	*/
           if (getc() == 'r') {
               /* force to default */
	        gd->mem_cfg = 0xff;
               bid = 0xffff;
           }
       }
#endif /* CFG_TSEC_KEYPRESS_ABORT */

       /* default EX3200-24T */
       /* assembly ID (2) + major version (2) + minor version (0) */
	if ((bid < I2C_ID_JUNIPER_EX3200_24_T) || (bid > I2C_ID_JUNIPER_EX4210_24_P)) {
	    gd->board_type = 0xffffffff;
           gd->valid_bid = 0;
       }
       else {
           gd->valid_bid = 1;
	    gd->board_type = id;
       }


       switch (board_id()) {
           case I2C_ID_JUNIPER_EX3200_24_T:
               puts("Board: EX3200-24T");
               break;
           case I2C_ID_JUNIPER_EX3200_24_P:
               puts("Board: EX3200-24POE");
               break;
           case I2C_ID_JUNIPER_EX3200_48_T:
               puts("Board: EX3200-48T");
               break;
           case I2C_ID_JUNIPER_EX3200_48_P:
               puts("Board: EX3200-48POE");
               break;
           case I2C_ID_JUNIPER_EX4200_24_F:
               puts("Board: EX4200-24F");
               break;
           case I2C_ID_JUNIPER_EX4200_24_T:
               puts("Board: EX4200-24T");
               break;
           case I2C_ID_JUNIPER_EX4200_24_P:
               puts("Board: EX4200-24POE");
               break;
           case I2C_ID_JUNIPER_EX4200_48_T:
               puts("Board: EX4200-48T");
               break;
           case I2C_ID_JUNIPER_EX4200_48_P:
               puts("Board: EX4200-48POE");
               break;
           case I2C_ID_JUNIPER_EX4210_24_P:
               puts("Board: EX4200-24PX");
               break;
           case I2C_ID_JUNIPER_EX4210_48_P:
               puts("Board: EX4200-48PX");
               break;
           default:
               puts("Board: Unknown");
               break;
       };
       printf(" %d.%d\n", version_major(), version_minor());
       epld_ver = version_epld();
       epld_reg_read(EPLD_LAST_RESET, (uint16_t *)&epldLastRst);
       printf("EPLD:  Version %d.%d (0x%x)\n", (epld_ver & 0xff00) >> 8,
              (epld_ver & 0x00C0) >> 6, epldLastRst);
       epld_reg_write(EPLD_LAST_RESET, 0);
       gd->last_reset = epldLastRst;

       /* remove the wrong secure EEPROM reset setting. */
       if (board_id() == I2C_ID_JUNIPER_EX4200_24_F) {
           epld_reg_read(EPLD_MISC_CONTROL, (uint16_t *)&epldMiscCtrl);
           epld_reg_write(EPLD_MISC_CONTROL, epldMiscCtrl & (~EPLD_UPLINK_HOTSWAP_LED_ON));
       }
       else if ((bid >= I2C_ID_JUNIPER_EX3200_24_T) && (bid <= I2C_ID_JUNIPER_EX4210_24_P)){
           epld_reg_read(EPLD_MISC_CONTROL_0, (uint16_t *)&epldMiscCtrl);
           epld_reg_write(EPLD_MISC_CONTROL_0, epldMiscCtrl & (~EPLD_REMOVE_SIC_RESET));
       }

	/*
	 * Initialize local bus.
	 */
	local_bus_init ();
	
	return 0;
}

long int
initdram(int board_type)
{
	long dram_size = 0;
#if 0
       int bid = board_id();
#endif

	puts("Initializing ");

#if defined(CONFIG_DDR_DLL)
	{
	    volatile immap_t *immap = (immap_t *)CFG_IMMR;
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

#if defined(CONFIG_DDR_ECC)
	/*
	 * Initialize and enable DDR ECC.
	 */
	if (mem_ecc()) {
	    ddr_enable_ecc(dram_size, 1);
	}
#endif

	return dram_size;
}

/*
 * Initialize Local Bus
 */
void
local_bus_init(void)
{
	volatile immap_t *immap = (immap_t *)CFG_IMMR;
#if 0
	volatile ccsr_gur_t *gur = &immap->im_gur;
#endif
	volatile ccsr_lbc_t *lbc = &immap->im_lbc;

	uint clkdiv;
	uint lbc_hz;
	sys_info_t sysinfo;
#if 0
	uint temp_lbcdll;
#endif

	/*
	 * Errata LBC11.
	 * Fix Local Bus clock glitch when DLL is enabled.
	 *
	 * If localbus freq is < 66Mhz, DLL bypass mode must be used.
	 * If localbus freq is > 133Mhz, DLL can be safely enabled.
	 * Between 66 and 133, the DLL is enabled with an override workaround.
	 */

	get_sys_info(&sysinfo);
	clkdiv = lbc->lcrr & 0x0f;
	lbc_hz = sysinfo.freqSystemBus / 1000000 / clkdiv;

	if (lbc_hz < 66) {
		lbc->lcrr |= 0x80000000;	/* DLL Bypass */

	} else if (lbc_hz >= 133) {
		lbc->lcrr &= (~0x80000000);		/* DLL Enabled */

	} else {
		lbc->lcrr &= (~0x8000000);	/* DLL Enabled */
		udelay(200);

#if 0
		/*
		 * Sample LBC DLL ctrl reg, upshift it to set the
		 * override bits.
		 */
		temp_lbcdll = gur->lbcdllcr;
		gur->lbcdllcr = (((temp_lbcdll & 0xff) << 16) | 0x80000000);
		asm("sync;isync;msync");
#endif
	}
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

#if defined(CONFIG_PCI)

/*
 * Initialize PCI Devices, report devices found.
 */

#ifndef CONFIG_PCI_PNP
static struct pci_config_table pci_java_config_table[] = {
    { PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID,
      PCI_IDSEL_NUMBER, PCI_ANY_ID,
      pci_cfgfunc_config_device, { PCI_ENET0_IOADDR,
				   PCI_ENET0_MEMADDR,
				   PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER
      } },
    { }
};
#endif

static struct pci_controller hose = {
#ifndef CONFIG_PCI_PNP
	config_table: pci_java_config_table,
#endif
};

#endif	/* CONFIG_PCI */

void
pci_init_board(void)
{
#ifdef CONFIG_PCI
	pci_mpc85xx_init(&hose);
#endif
}

int misc_init_r (void)
{
    uint8_t muxport, resetStatus, resetMask = 0, temp;
    volatile uint16_t pot, epldStatus;
    unsigned char ch;
    int i, bid = board_id();

	/* 2nd I2C controller */
       i2c_controller(CFG_I2C_CTRL_2);

       /* PCA9506 I/O expander configure all output pins */
       i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, 0x18, 0);
       i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, 0x19, 0);
       i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, 0x1A, 0);
       i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, 0x1B, 0);
       i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, 0x1C, 0x80);

       /* not invert all reset pins (active low) */
       i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, 0x10, 0x0);
       i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, 0x11, 0x0);
       i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, 0x12, 0x0);
       i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, 0x13, 0x0);
       i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, 0x14, 0x0);

#if 0
       /* temporary */
       i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, 0x8, 0xFF);
       i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, 0x9, 0xFF);
       i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, 0xA, 0xFF);
       i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, 0xB, 0xFF);
       i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, 0xC, 0xFF);
#endif

       /* Init reset pins output to low (0).  Leaving all PHYs in reset. */
       /* Enable management port PHY */
       i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, 0xA, 0x00);
       i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, 0xB, 0x80);
       /* Pull MACs to reset.  LED select RJ45. */
       i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, 0xC, 0x78);

       /* MAC 2 temporary set to db00 (1G) mode */
//       i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, 0x08, 0x00);
       /* LEDs are off.   MAC#2 port mode set (default).  */
       i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, 0x08, 0x01);
       i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, 0x09, 0x00); 

       /* Latte24SFP is different in I2C.  Handle I/O expanders on ch 2 & 3.  */
       if (board_id() == I2C_ID_JUNIPER_EX4200_24_F) {
           /* enable mux ch 2 */
           ch = 1 << 2;
           i2c_write(CFG_I2C_CTRL_2_SW_ADDR, 0, 0, &ch, 1);
           
           /* Tx_Dis is output.  Mod-Def and LOS are input */
           i2c_reg_write(CFG_I2C_CTRL_2_SFP_IO_ADDR, 0x18, 0x00);
           i2c_reg_write(CFG_I2C_CTRL_2_SFP_IO_ADDR, 0x19, 0xF0);
           i2c_reg_write(CFG_I2C_CTRL_2_SFP_IO_ADDR, 0x1A, 0xFF);
           i2c_reg_write(CFG_I2C_CTRL_2_SFP_IO_ADDR, 0x1B, 0xFF);
           i2c_reg_write(CFG_I2C_CTRL_2_SFP_IO_ADDR, 0x1C, 0xFF);
           
           /* not invert all reset pins (active low) */
           i2c_reg_write(CFG_I2C_CTRL_2_SFP_IO_ADDR, 0x10, 0x0);
           i2c_reg_write(CFG_I2C_CTRL_2_SFP_IO_ADDR, 0x11, 0x0);
           i2c_reg_write(CFG_I2C_CTRL_2_SFP_IO_ADDR, 0x12, 0x0);
           i2c_reg_write(CFG_I2C_CTRL_2_SFP_IO_ADDR, 0x13, 0x0);
           i2c_reg_write(CFG_I2C_CTRL_2_SFP_IO_ADDR, 0x14, 0x0);

           /* Leaving SFP in Tx_Dis */
           i2c_reg_write(CFG_I2C_CTRL_2_SFP_IO_ADDR, 0x08, 0xFF);
           i2c_reg_write(CFG_I2C_CTRL_2_SFP_IO_ADDR, 0x09, 0x0F); 

           /* disable mux ch 2 */
           ch = 0;
           i2c_write(CFG_I2C_CTRL_2_SW_ADDR, 0, 0, &ch, 1);           

           /* enable mux ch 3 */
           ch = 1 << 3;
           i2c_write(CFG_I2C_CTRL_2_SW_ADDR, 0, 0, &ch, 1);
           
           /* Tx_Dis is output.  Mod-Def and LOS are input */
           i2c_reg_write(CFG_I2C_CTRL_2_SFP_IO_ADDR, 0x18, 0x00);
           i2c_reg_write(CFG_I2C_CTRL_2_SFP_IO_ADDR, 0x19, 0xF0);
           i2c_reg_write(CFG_I2C_CTRL_2_SFP_IO_ADDR, 0x1A, 0xFF);
           i2c_reg_write(CFG_I2C_CTRL_2_SFP_IO_ADDR, 0x1B, 0xFF);
           i2c_reg_write(CFG_I2C_CTRL_2_SFP_IO_ADDR, 0x1C, 0xFF);
           
           /* not invert all reset pins (active low) */
           i2c_reg_write(CFG_I2C_CTRL_2_SFP_IO_ADDR, 0x10, 0x0);
           i2c_reg_write(CFG_I2C_CTRL_2_SFP_IO_ADDR, 0x11, 0x0);
           i2c_reg_write(CFG_I2C_CTRL_2_SFP_IO_ADDR, 0x12, 0x0);
           i2c_reg_write(CFG_I2C_CTRL_2_SFP_IO_ADDR, 0x13, 0x0);
           i2c_reg_write(CFG_I2C_CTRL_2_SFP_IO_ADDR, 0x14, 0x0);

           /* Leaving SFP in Tx_Dis */
           i2c_reg_write(CFG_I2C_CTRL_2_SFP_IO_ADDR, 0x08, 0xFF);
           i2c_reg_write(CFG_I2C_CTRL_2_SFP_IO_ADDR, 0x09, 0x0F); 

           /* disable mux ch 3 */
           ch = 0;
           i2c_write(CFG_I2C_CTRL_2_SW_ADDR, 0, 0, &ch, 1);           
       }
	/* Default 1st I2C controller */
       i2c_controller(CFG_I2C_CTRL_1);

       led_init();
       port_led_init();

       /* LCD contrast */
       muxport = 0x1 << 3;  /* port 3 */
       pot = 0x0000;      /* default POT setup */

       /* enable mux port 3 */
       i2c_write(CFG_I2C_CTRL_1_SW_ADDR, 0, 0, &muxport, 1);  
        /* default contrast */
       i2c_write(CFG_I2C_LCD_POT_ADDR, 0, 0, (uint8_t*)&pot, 2);  
       /* disable mux port 3 */
       muxport = 0;
       i2c_write(CFG_I2C_CTRL_1_SW_ADDR, 0, 0, &muxport, 1);  

       lcd_init(); 
       udelay(200000);  /* 200 ms */
       lcd_init(); 
       
       i2c_controller(CFG_I2C_CTRL_2);
       temp = 0x78;
       /* pull MACs out of reset one by one at 100ms apart. */
       for (i = 0; i < 3; i++) {
            temp |= 1 << i;
            i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, 0xC, temp);
            udelay(100000);
       }
       i2c_controller(CFG_I2C_CTRL_1);
       
       /* errata FEr#1095.  Cheetah clock stabilization after reset */
       if (epld_reg_read(EPLD_INT_STATUS2, (uint16_t *)&epldStatus)) {
            /* Single Cheetah */
            if (!(epldStatus & EPLD_MAC_IN_DONE_0)) {
                resetMask |= 1 << 0;
            }
            /* Two Cheetahs */
            if ((bid == I2C_ID_JUNIPER_EX3200_48_T) ||
                (bid == I2C_ID_JUNIPER_EX3200_48_P) ||
                (bid == I2C_ID_JUNIPER_EX4200_24_F) ||
                (bid == I2C_ID_JUNIPER_EX4200_24_T) ||
                (bid == I2C_ID_JUNIPER_EX4200_24_P) ||
                (bid == I2C_ID_JUNIPER_EX4210_24_P)){
                if (!(epldStatus & EPLD_MAC_IN_DONE_1)) {
                    resetMask |= 1 << 1;
                }
            }
            /* Three Cheetahs */
            if ((bid == I2C_ID_JUNIPER_EX4200_48_T) || 
                (bid == I2C_ID_JUNIPER_EX4200_48_P) ||
                (bid == I2C_ID_JUNIPER_EX4210_48_P)){
                if (!(epldStatus & EPLD_MAC_IN_DONE_2)) {
                    resetMask |= 1 << 2;
                }
            }
            if (resetMask) {
                i2c_controller(CFG_I2C_CTRL_2);
                resetStatus = i2c_reg_read(CFG_I2C_CTRL_2_IO_ADDR, 0xC);
                i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, 0xC, resetStatus & (!resetMask));
                udelay(200000);
                i2c_reg_write(CFG_I2C_CTRL_2_IO_ADDR, 0xC, resetStatus | resetMask);
                i2c_controller(CFG_I2C_CTRL_1);
            }
       }


#if defined(CONFIG_NOR)
	/*
	 * FAT filesystem in NOR flash.
	 */
	nor_init(0);
#endif
    
       /* EPLD */
       epld_watchdog_init();
#if 0
       epld_watchdog_enable();
#endif

       /* booting */
       set_led(MIDDLE_LED, LED_GREEN, LED_SLOW_BLINKING);

       /* Print the juniper internal version of the u-boot */
       printf("\nFirmware Version: --- %02X.%02X.%02X ---\n",
	      CFG_VERSION_MAJOR, CFG_VERSION_MINOR, CFG_VERSION_PATCH);
       
	return 0;
}

int last_stage_init(void)
{
	volatile immap_t *immap = (immap_t *) CFG_CCSRBAR;
	volatile ccsr_gur_t *gur = &immap->im_gur;
	struct eth_device *dev;
       volatile uint16_t epldMiscCtrl;
//       volatile uint16_t epldPBRegister;
       unsigned char ch, tdata;
       char *s;
       int loopback = 0;
       uint32_t ether_fail = 0;
	sys_info_t sysinfo;
       unsigned char temp[20];
       int bid = board_id();
    int nand_test = 0;
    int test_count = 0;
    ulong nand_fail = 0;
    ulong nand_length;
    ulong nand_offset;


       if (!(gur->gpindr & 0x10000000))  /* uplink present */
       {
	    gur->gpoutdr |= 0xe0000000; /* enable I2C uplink and uplink */        
       }
	   
       if (bid != I2C_ID_JUNIPER_EX4200_24_F) 
	    gur->gpoutdr |= 0x02000000; /* enable POE controller */
	   
       dev = eth_get_dev_by_name(CONFIG_MPC85XX_TSEC1_NAME);
       if (dev)
           mgmt_priv = (struct tsec_private *)dev->priv;;

       /* fan setup */
       i2c_controller(CFG_I2C_CTRL_1);
       ch = 1 << 6;
       i2c_write(CFG_I2C_CTRL_1_SW_ADDR, 0, 0, &ch, 1);
       tdata = 0x0;
       i2c_write(CFG_I2C_HW_MON_ADDR, 0x5c, 1, &tdata, 1);
       tdata = 0x84;  /* bank 4 */
       i2c_write(CFG_I2C_HW_MON_ADDR, 0x4e, 1, &tdata, 1);
       tdata = 0x0;
       i2c_write(CFG_I2C_HW_MON_ADDR, 0x5c, 1, &tdata, 1);
       tdata = 0x80;  /* bank 0 */
       i2c_write(CFG_I2C_HW_MON_ADDR, 0x4e, 1, &tdata, 1);
       i2c_read(CFG_I2C_HW_MON_ADDR, 0x47, 1, &tdata, 1);
       tdata |= 0xf0;
       i2c_write(CFG_I2C_HW_MON_ADDR, 0x47, 1, &tdata, 1);
       i2c_read(CFG_I2C_HW_MON_ADDR, 0x4b, 1, &tdata, 1);
       tdata |= 0xc0;
       i2c_write(CFG_I2C_HW_MON_ADDR, 0x4b, 1, &tdata, 1);
       tdata = 0xc0;
       /* all fan with 75% duty cycle */
       i2c_write(CFG_I2C_HW_MON_ADDR, 0x5b, 1, &tdata, 1);
       i2c_write(CFG_I2C_HW_MON_ADDR, 0x5e, 1, &tdata, 1);
       i2c_write(CFG_I2C_HW_MON_ADDR, 0x5f, 1, &tdata, 1);
       ch = 0;
       i2c_write(CFG_I2C_CTRL_1_SW_ADDR, 0, 0, &ch, 1);

       /* temperature sensor setup */
       i2c_controller(CFG_I2C_CTRL_1);
       ch = 1 << 6;
       i2c_write(CFG_I2C_CTRL_1_SW_ADDR, 0, 0, &ch, 1);
       tdata = 0x80;  /* bank 0 */
       i2c_write(CFG_I2C_HW_MON_ADDR, 0x4e, 1, &tdata, 1);
       i2c_read(CFG_I2C_HW_MON_ADDR, 0x5d, 1, &tdata, 1);
       tdata |= 0x0e;
       i2c_write(CFG_I2C_HW_MON_ADDR, 0x5d, 1, &tdata, 1);
       i2c_read(CFG_I2C_HW_MON_ADDR, 0x59, 1, &tdata, 1);
       tdata |= 0x70;
       i2c_write(CFG_I2C_HW_MON_ADDR, 0x59, 1, &tdata, 1);
       i2c_read(CFG_I2C_HW_MON_ADDR, 0x4b, 1, &tdata, 1);
       tdata &= 0xf3;
       tdata |= 0x08;
       i2c_write(CFG_I2C_HW_MON_ADDR, 0x4b, 1, &tdata, 1);

       if ((bid == I2C_ID_JUNIPER_EX3200_24_P) ||
           (bid == I2C_ID_JUNIPER_EX3200_24_T)){
           i2c_read(CFG_I2C_HW_MON_ADDR, 0x4c, 1, &tdata, 1);
           tdata &= 0xef;
           tdata |= 0x10;
           i2c_write(CFG_I2C_HW_MON_ADDR, 0x4c, 1, &tdata, 1);
           i2c_read(0x48, 0x01, 1, &tdata, 1);
           tdata &= 0xfe;
           tdata |= 0x01;
           i2c_write(0x48, 0x01, 1, &tdata, 1);
           tdata = 0x80;  /* bank 0 */
           i2c_write(CFG_I2C_HW_MON_ADDR, 0x4e, 1, &tdata, 1);
       }
       
       ch = 0;
       i2c_write(CFG_I2C_CTRL_1_SW_ADDR, 0, 0, &ch, 1);

       get_sys_info(&sysinfo);
       if (sysinfo.freqSystemBus == 300000000) {
           i2c_controller(CFG_I2C_CTRL_2);
           i2c_fdr(0x2b, CFG_I2C_SLAVE);  /* 97KHz */
           i2c_controller(CFG_I2C_CTRL_1);
           i2c_fdr(0x2b, CFG_I2C_SLAVE);  /* 97KHz */
       }

       if ((s = getenv("boot.ethtest")) != NULL) {
           loopback = simple_strtoul (s, NULL, 10);  /* 0=mac+phy, 1=mac+phy+ext */
           ether_fail = diag_ether_loopback(loopback);
           if (ether_fail) {
               sprintf(temp, "FAILED %08x ", ether_fail);
               setenv("boot.etsec0", temp);
           }
           else {
               setenv ("boot.etsec0", "PASSED ");
           }
       }

#ifdef CONFIG_NAND_DIRECT
    if ((s = getenv("boot.nandtest")) != NULL) {
        nand_test = simple_strtoul (s, NULL, 10);
        if (nand_test) {
            s = getenv("nand.offset");
            nand_offset = simple_strtoul (s, NULL, 16);
            s = getenv("nand.length");
            nand_length = simple_strtoul (s, NULL, 16);
            s = getenv("nand.test_count");
            test_count = simple_strtoul (s, NULL, 10);
            nand_fail = nand_erase_rw (nand_offset, nand_length, test_count);
            setenv("boot.nandtest", NULL);
            saveenv ();
        }
        if (nand_fail == NAND_SIZE_ERR) {
            sprintf(temp, "NAND SIZE FAILED");
        } else if (nand_fail == NAND_ERASE_ERR) {
            sprintf(temp, "NAND ERASE FAILED");
        } else if (nand_fail == NAND_WRITE_ERR) {
            sprintf(temp, "NAND WRITE FAILED");
        } else if (nand_fail == NAND_READ_ERR) {
            sprintf(temp, "NAND READ FAILED");
        } else if (nand_fail == NAND_VERIFY_ERR) {
            sprintf(temp, "DATA VERIFY FAILED");
        } else {
            sprintf(temp, "PASSED ");
        }
        setenv("boot.nandresult", temp);
    }
#endif
	return 0;
}

/* retrieve MAC address from main board EEPROM */
void board_get_enetaddr (uchar * enet)
{
    unsigned char ch = 1 << 4;  /* port 4 */
    unsigned char temp[60];
    char *s;
    uint8_t esize = 0;
    bd_t *bd = gd->bd;
    int mloop = 1;
#if defined(CONFIG_READONLY)
    ulong state = get_upgrade_state();
#endif
    
    i2c_controller(CFG_I2C_CTRL_1);
    i2c_write(CFG_I2C_CTRL_1_SW_ADDR, 0, 0, &ch, 1);

    if (gd->valid_bid == 0) {
        enet[0] = 0x00;
        enet[1] = 0xE0;
        enet[2] = 0x0C;
        enet[3] = 0x00;
        enet[4] = 0x00;
        enet[5] = 0xFD;
    }
    else {
        if (gd->secure_eeprom) {
            secure_eeprom_read(CFG_I2C_SECURE_EEPROM_ADDR, CFG_EEPROM_MAC_OFFSET, enet, 6);
            secure_eeprom_read(CFG_I2C_SECURE_EEPROM_ADDR, CFG_EEPROM_MAC_OFFSET-1, &esize, 1);
        }
        else {
            eeprom_read(CFG_I2C_MAIN_EEPROM_ADDR, CFG_EEPROM_MAC_OFFSET, enet, 6);
            eeprom_read(CFG_I2C_MAIN_EEPROM_ADDR, CFG_EEPROM_MAC_OFFSET-1, &esize, 1);
        }
    }
    
    ch = 0;
    i2c_write(CFG_I2C_CTRL_1_SW_ADDR, 0, 0, &ch, 1);

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
    s = getenv("ethaddr");
    if (memcmp (s, temp, sizeof(temp)) != 0) {
        setenv("ethaddr", temp);
    }

    /* environment variable for board type */
    sprintf(temp,"%04x ", board_id());
    setenv("hw.board.type", temp);

    /* environment variable for last reset */
    sprintf(temp,"%02x ", gd->last_reset);
    setenv("hw.board.reset", temp);

#if defined(CONFIG_READONLY)
    /* environment variable current U-boot image */
    sprintf(temp,"%s ", "readonly");
    setenv("boot.current", temp);
#endif

#if defined(CONFIG_READONLY)
    sprintf(temp,"%d ", state);
    setenv("boot.state", temp);
#endif

#if defined(CONFIG_UPGRADE)
    s = (char *) (CFG_UPGRADE_BOOT_IMG_ADDR + 4);
    setenv("boot.upgrade.ver", s);
    s = (char *) (CFG_READONLY_BOOT_IMG_ADDR + 4);
    setenv("boot.readonly.ver", s);
#endif

#if defined(CONFIG_READONLY)
    s = NULL;
    if (state >= UPGRADE_CHECK) {
        s = (char *) (CFG_UPGRADE_BOOT_IMG_ADDR + 4);
    }
    setenv("boot.upgrade.ver", s);
    s = (char *) (CFG_READONLY_BOOT_IMG_ADDR + 4);
    setenv("boot.readonly.ver", s);
#endif

    if ((s = getenv("boot.test")) != NULL) {
	  mloop = simple_strtoul (s, NULL, 16);
         if (memtest(0x2000, bd->bi_memsize - 0x100000, mloop, 1)) {
             setenv ("boot.ram", "FAILED ");
         }
         else {
             setenv ("boot.ram", "PASSED ");
         }
    }

    /* set the internal uboot version */
    sprintf(temp, " %d.%d.%d",
	    CFG_VERSION_MAJOR, CFG_VERSION_MINOR, CFG_VERSION_PATCH);
    setenv("boot.intrver", temp);

    /* uboot/loader memory map */
#if defined(CONFIG_READONLY) || defined(CONFIG_UPGRADE)
    sprintf(temp,"0x%08x ", CFG_OPQ_ENV_ADDR);
    setenv("boot.opqenv.start", temp);

    sprintf(temp,"0x%08x ", CFG_OPQ_ENV_SECT_SIZE);
    setenv("boot.opqenv.size", temp);

    sprintf(temp,"0x%08x ", CFG_FLASH_BASE + CFG_UPGRADE_BOOT_STATE_OFFSET);
    setenv("boot.upgrade.state", temp);
    
    sprintf(temp,"0x%08x ", CFG_UPGRADE_LOADER_ADDR);
    setenv("boot.upgrade.loader", temp);
    
    sprintf(temp,"0x%08x ", CFG_UPGRADE_BOOT_IMG_ADDR);
    setenv("boot.upgrade.uboot", temp);

    sprintf(temp,"0x%08x ", CFG_READONLY_LOADER_ADDR);
    setenv("boot.readonly.loader", temp);
    
    sprintf(temp,"0x%08x ", CFG_READONLY_BOOT_IMG_ADDR);
    setenv("boot.readonly.uboot", temp);
#endif

    s = getenv("bootcmd.prior");
    if (s) {
        sprintf(temp,"%s; %s", s, CONFIG_BOOTCOMMAND);
        setenv("bootcmd", temp);
    }
    else
        setenv("bootcmd", CONFIG_BOOTCOMMAND);

    if (gd->secure_eeprom) {
        sprintf(temp,"0x%02x ", CFG_I2C_SECURE_EEPROM_ADDR);
        setenv("boot.ideeprom", temp);
    }
    else {
        sprintf(temp,"0x%02x ", CFG_I2C_MAIN_EEPROM_ADDR);
        setenv("boot.ideeprom", temp);
    }
            
#if 0
    sprintf(temp, "%02x:%02x:%02x:%02x:%02x:%02x",
                enet[0], enet[1], enet[2], enet[3], enet[4], enet[5]+1);
    s = getenv("eth1addr");
    if (memcmp (s, temp, sizeof(temp)) != 0) {
        setenv("eth1addr", temp);
    }
#endif    
}

#ifdef CONFIG_SHOW_ACTIVITY
void board_show_activity (ulong timestamp)
{
    static int link_status = 0;
    int temp, temp_page;
    volatile uint16_t temp1;
    extern uint read_phy_reg(struct tsec_private *priv, uint regnum);
    extern void write_phy_reg(struct tsec_private *priv, uint regnum, uint value);

    if (epld_reg_read(EPLD_WATCHDOG_ENABLE, (uint16_t *)&temp1)) {
        if (temp1 & 0x1)  /* watchdog enabled */
            epld_reg_write(EPLD_WATCHDOG_SW_UPDATE, 0x1);  /* kick watchdog */
    }

    /* 2 seconds */
    if (mgmt_priv && (timestamp % (2 * CFG_HZ) == 0)) {
        temp_page = read_phy_reg(mgmt_priv, 0x16);
        temp = read_phy_reg(mgmt_priv, 0x11);
        write_phy_reg(mgmt_priv, 0x16, temp_page);
        if ((temp & 0xe400) != (link_status & 0xe400)) {
            link_status = temp;
            if (link_status & MIIM_88E1011_PHYSTAT_LINK)
                set_port_led(0, LINK_UP,
                                  (link_status & MIIM_88E1011_PHYSTAT_DUPLEX) ? FULL_DUPLEX : HALF_DUPLEX,
                                  (link_status & MIIM_88E1011_PHYSTAT_GBIT) ? SPEED_1G :
                                  ((link_status & MIIM_88E1011_PHYSTAT_100) ? SPEED_100M : SPEED_10M));
            else
                set_port_led(0, LINK_DOWN, 0, 0);  /* don't care */                                  
        }
    }
	
    /* 500ms */
    if (timestamp % (CFG_HZ / 2) == 0)  
        lcd_heartbeat ();

    /* 100ms */
    if (timestamp % (CFG_HZ /10) == 0) {
        if (!i2c_bus_busy())
            update_led ();
    }
}

void show_activity(int arg)
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
int post_hotkeys_pressed(void)
{
	return 0;   /* No hotkeys supported */
}

#endif

int post_stop_prompt (void)
{
    char *s;
    int mem_result;
    
    if (getenv("boot.nostop")) return 0;

    if (((s = getenv("boot.ram")) != NULL) &&
         ((strcmp(s,"PASSED") == 0) ||
         (strcmp(s,"FAILED") == 0))) {
         mem_result = 0;
    }
    else {
         mem_result = post_result_mem;
    }

    if( mem_result == -1 || post_result_cpu == -1 ||
        post_result_ether == -1 || post_result_rom == -1 ||
        gd->valid_bid == 0)
        return 1;
    return 0;
}

int secure_eeprom_read (unsigned dev_addr, unsigned offset, uchar *buffer, unsigned cnt)
{
    int alen = (offset > 255) ? 2 : 1;
    
    return i2c_read_norsta(dev_addr, offset, alen, buffer, cnt);
}

int check_internal_usb_storage_present (void)
{
    int i;
    struct usb_device *dev = NULL;
    extern int usb_max_devs;

    if (usb_max_devs > 0) { /* # of USB storage devices */
        for (i = 0; i < USB_MAX_DEVICE; i++) {
            dev = usb_get_dev_index(i);
            if (dev == NULL)
                break;
            if (dev->portnr == 2)  /* Internal USB storage on port #2 */
                return 1;
        }
    }
    return 0;
}

void toggle_usb (void)
{
    volatile uint16_t temp;

    /* 
     * Use previous unused bit 1 on SYSPLD register 
     * EPLD_WATCHDOG_ENABLE to control 3.3V
     * to ISP1564.
     */
    epld_reg_read(EPLD_WATCHDOG_ENABLE, (uint16_t *)&temp);
    temp |= 0x2;
    /* kill 3.3V to ISP1564 */
    epld_reg_write(EPLD_WATCHDOG_ENABLE, temp);
    udelay(10000);  /* 10 ms */
    epld_reg_read(EPLD_WATCHDOG_ENABLE, (uint16_t *)&temp);
    temp &= ~0x2;
    /* restore 3.3V to ISP1564 */
    epld_reg_write(EPLD_WATCHDOG_ENABLE, temp);
}

void
reset_phy (void)
{
    struct eth_device *dev;

    /* reset mgmt port phy */
    dev = eth_get_dev_by_name(CONFIG_MPC85XX_TSEC1_NAME);
    ether_tsec_reinit(dev);
}

/*********************************************************/

uint32_t 
single_bit_ecc_errors (void)
{
    /* not implemented */
    return (0);
}

uint32_t 
double_bits_ecc_errors (void)
{
    /* not implemented */
    return (0);
}
