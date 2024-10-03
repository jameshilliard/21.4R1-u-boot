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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or 
modify this File in accordance with the terms and conditions of the General 
Public License Version 2, June 1991 (the "GPL License"), a copy of which is 
available along with the File in the license.txt file or by writing to the Free 
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or 
on the worldwide web at http://www.gnu.org/licenses/gpl.txt. 

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED 
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY 
DISCLAIMED.  The GPL License provides additional details about this warranty 
disclaimer.

*******************************************************************************/

#include <common.h>
#include "mvTypes.h"
#include "mvBoardEnvLib.h"
#include "mvCpuIf.h"
#include "mvCtrlEnvLib.h"
#include "mv_mon_init.h"
#include "mvDebug.h"
#include "device/mvDevice.h"
#include "twsi/mvTwsi.h"
#include "eth/mvEth.h"
#include "pex/mvPex.h"
#include "gpp/mvGpp.h"
#include "sys/mvSysUsb.h"
#include "bootstrap_def.h"
#if defined(CONFIG_POST) && defined(CONFIG_EX2200)
#include <post.h>
#endif

#if defined(MV_INCLUDE_USB)
#include "usb/mvUsb.h"
#endif

#include "cpu/mvCpu.h"

#ifdef CONFIG_PCI
#include <pci.h>
#endif
#include "pci/mvPciRegs.h"

#include "net.h"
#include <command.h>

#include "syspld.h"
#include "i2c.h"
#include <pcie.h>
#include "led.h"
#include "elf.h"
#include "usb.h"

DECLARE_GLOBAL_DATA_PTR;

/* #define MV_DEBUG */
#ifdef MV_DEBUG
#define DB(x) x
#else
#define DB(x)
#endif

#undef SYSPLD_WD_ISR
//#define BM_WORKAROUND
#undef BM_WORKAROUND

typedef enum upgrade_state 
{
    UPGRADE_READONLY = 0,  /* set by readonly u-boot */
    UPGRADE_START,      /* set by loader/JunOS */
    UPGRADE_CHECK,      /* set by readonly U-boot */
    UPGRADE_TRY,          /* set by readonly U-boot */
    UPGRADE_OK             /* set by upgrade U-boot */
} upgrade_state_t;

int usb_scan_dev = 0;
int checkboard_done = 0;

/* CPU address decode table. */
MV_CPU_DEC_WIN mvCpuAddrWinMap[] = MV_CPU_IF_ADDR_WIN_MAP_TBL;

extern int i2c_read_norsta(uint8_t chanNum, uchar dev_addr, unsigned int offset, int alen, uchar* data, int len);
extern int i2c_mux(uint8_t mux, uint8_t chan, int ena);
extern void rtc_init(void);
extern void rtc_stop(void);
extern void rtc_start(void);
extern void rtc_set_time(int yy, int mm, int dd, int hh, int mi, int ss);
extern void syspld_watchdog_petting(void);

extern void flush_console_input(void);
extern int readline(const char *const prompt);
extern unsigned long usb_stor_write(int device, unsigned long blknr,
				    unsigned long blkcnt,
				    unsigned long *buffer);



void mv_cpu_init(void);
int board_idtype(void);
int board_pci(void);
MV_U32 switchClkGet(void);
MV_U32 mvTclkGet(void);
MV_U32 mvSysClkGet (void);

int32_t exflashInit (void);
int set_upgrade_state (upgrade_state_t state);
int get_upgrade_state (void);
int valid_env (unsigned long addr);
int valid_loader_image (unsigned long addr);
int valid_uboot_image (unsigned long addr);

#ifdef	CFG_FLASH_CFI_DRIVER
MV_VOID mvUpdateNorFlashBaseAddrBank(MV_VOID);
int mv_board_num_flash_banks;
extern flash_info_t	flash_info[]; /* info for FLASH chips */
extern unsigned long flash_add_base_addr (uint flash_index, ulong flash_base_addr);
#endif	/* CFG_FLASH_CFI_DRIVER */

#if defined(MV_INCLUDE_UNM_ETH) || defined(MV_INCLUDE_GIG_ETH)
extern MV_VOID mvBoardEgigaPhySwitchInit(void);
#endif 

#if defined(CONFIG_SPI_SW_PROTECT)
extern int ex_spi_sw_protect_set (uint32_t offset, uint8_t *pdata, uint32_t len);
extern int ex_spi_sw_protect_get (uint32_t offset, uint8_t *pdata, uint32_t len);
#endif

extern int post_result_cpu;
extern int post_result_mem;
extern int post_result_ether;
extern int post_result_pci;
extern int ledUpdateEna;

extern char console_buffer[];
extern block_dev_desc_t usb_dev_desc[USB_MAX_STOR_DEV];
extern struct usb_device usb_dev[USB_MAX_DEVICE];
extern char console_buffer[];


MV_VOID mvMppModuleTypePrint(MV_VOID);

/* Define for SDK 2.0 */
int 
raise(void) 
{
    return (0);
}

void 
print_mvBanner (void)
{
    return;
}

void 
print_dev_id (void)
{
    return;
}

void 
ext_bdinfo (void)
{
    char name[30];

    name[0] = 0;
    mvCtrlModelRevNameGet(name);
    printf("Soc         = %s (DDR2)\n",  name);
    name[0] = 0;
    mvCpuNameGet(name);
    printf("CPU         = %s\n",  name);
    printf("CPU clock   = %d Mhz\n",  gd->cpu_clk/1000000);
    printf("bus clock   = %d Mhz\n",  gd->bus_clk/1000000);
    printf("switch clock= %d Mhz\n",  gd->sw_clk/1000000);
    printf("Tclock      = %d Mhz\n",  gd->tclk/1000000);
    printf("D-Cache     =  16 kB\n");
    printf("I-Cache     =  16 kB\n");
    printf("L2-Cache    = 256 kB\n");
    printf("memstart    = 00000000\n");
    printf("memsize     = %08x\n", gd->ram_size);
    printf("flashstart  = %08x\n", DEVICE_SPI_BASE);
    printf("flashsize   = %08x\n", gd->flash_size);
}

void 
maskAllInt (void)
{
    /* mask all external interrupt sources */
    MV_REG_WRITE(CPU_MAIN_IRQ_MASK_REG, 0);
    MV_REG_WRITE(CPU_MAIN_FIQ_MASK_REG, 0);
    MV_REG_WRITE(CPU_ENPOINT_MASK_REG, 0);
    MV_REG_WRITE(CPU_MAIN_IRQ_MASK_HIGH_REG, 0);
    MV_REG_WRITE(CPU_MAIN_FIQ_MASK_HIGH_REG, 0);
    MV_REG_WRITE(CPU_ENPOINT_MASK_HIGH_REG, 0);
}

/* init for the Master*/
void 
misc_init_r_dec_win (void)
{
#if defined(MV_INCLUDE_USB)
#if !defined(CONFIG_SIMPLIFY_DISPLAY)
    printf("USB 0: host mode\n");
#endif
    mvUsbInit(0, MV_TRUE);
#endif

    return;
}

/*
 * Miscellaneous platform dependent initialisations
 */

extern int interrupt_init (void);

int 
board_init (void)
{
    MV_TWSI_ADDR slave;
    unsigned int i;
    uint8_t regVal;

    maskAllInt();

    /* must initialize the int in order for udelay to work */
    mvTclkGet();
    interrupt_init();

    /* Init the Controller CPU interface */
    mvCpuIfInit(mvCpuAddrWinMap);

    /* Init the Controlloer environment module (MPP init) */
    MV_REG_WRITE(MPP_CONTROL_REG0, 0x01002222);
    /*
     * For EX2200 24 & 48 SKUs, MPPSEL 13 & 14 to be configured as GPIOs.
     * For EX2200 12X, MPPSEL 13 & 14 to be configured as UART TXD & RXD.
     */ 
#if !defined(CONFIG_EX2200_12X)
    MV_REG_WRITE(MPP_CONTROL_REG1, 0x20003311);
#else
    MV_REG_WRITE(MPP_CONTROL_REG1, 0x23303311);
#endif
    MV_REG_WRITE(MPP_CONTROL_REG2, 0x00000002);

    /* set GPP polarity */
#ifdef SYSPLD_WD_ISR
    mvGppPolaritySet(0, 0xFFFFFFFF, 0x10);  /* invert GPIO4 */
#else
    mvGppPolaritySet(0, 0xFFFFFFFF, 0);
#endif
    mvGppPolaritySet(1, 0xFFFFFFFF, 0x0);

    /* enable GPP output */
    mvGppTypeSet(0, 0xFFFFFFFF, ~0x480420A0);
#ifdef BM_WORKAROUND
    mvGppTypeSet(1, 0xFFFFFFFF, 0xFFFF1F9C);
#else
    mvGppTypeSet(1, 0xFFFFFFFF, ~0x00000081);
#endif

#ifdef SYSPLD_WD_ISR
    /* level interrupt GPIO4, syspld int */
    gppRegSet(0, GPP_INT_LVL_REG(0), 0xFFFFFFFF, 0x10); 
#endif

    /* reset W38782, i2c, syspld */
    MV_REG_WRITE(GPP_DATA_OUT_REG(0), 0x00000020);
    mvGppTypeSet(0, 0xFFFFFFFF, ~0x480420A0);
    /* reset seeprom */
    MV_REG_WRITE(GPP_DATA_OUT_REG(1), 0x00000000);
#ifdef BM_WORKAROUND
    mvGppTypeSet(1, 0xFFFFFFFF, 0xFFFF1F9C);
#else
    mvGppTypeSet(1, 0xFFFFFFFF, ~0x00000081);
#endif
    udelay(400);
    /* clear W38782, i2c, syspld */
    MV_REG_WRITE(GPP_DATA_OUT_REG(0), 0x00042000);
    mvGppTypeSet(0, 0xFFFFFFFF, ~0x480420A0);
    /* clear seeprom */
#ifdef BM_WORKAROUND
    MV_REG_WRITE(GPP_DATA_OUT_REG(1), 0x00002000);
    mvGppTypeSet(1, 0xFFFFFFFF, 0xFFFF1F9C);
#else
    MV_REG_WRITE(GPP_DATA_OUT_REG(1), 0x00000001);
    mvGppTypeSet(1, 0xFFFFFFFF, ~0x00000081);
#endif

    slave.type = ADDR7_BIT;
    slave.address = 0;
    mvTwsiInit(0, CFG_I2C_SPEED, CFG_TCLK, &slave, 0);

    /* arch number of Integrator Board */
    gd->bd->bi_arch_number = 527;

    /* adress of boot parameters */
    gd->bd->bi_boot_params = 0x00000100;

    /* relocate the exception vectors */
    /* U-Boot is running from DRAM at this stage */
    for (i = 0; i < 0x100; i+=4) {
        *(unsigned int *)(0x0 + i) = *(unsigned int*)(TEXT_BASE + i);
    }
	
    /* Update NOR flash base address bank for CFI driver */
#ifdef	CFG_FLASH_CFI_DRIVER
    mvUpdateNorFlashBaseAddrBank();
#endif	/* CFG_FLASH_CFI_DRIVER */

#if !defined(CONFIG_EX2200_12X)
        i2c_mux(I2C_MAIN_MUX_ADDR, 2, 1);
        /* mgmt PHY, NAND controller out of reset */
        /* syspld will move USB HUB out of reset automatically */
        /* kludge for USB reset, it shall set 0x2 instead. */
        /* PoE present does not work at this point.  Pull poe controller out of
         *  reset and poe port enable no mater P or T SKU.  It will not hurt
         *  T SKU.
         */
        syspld_reg_write(SYSPLD_RESET_REG_2, 0xA);

        syspld_reg_read(SYSPLD_MISC, &regVal);
        /* PoE port disable */
        regVal |= SYSPLD_POE_ENABLE;
        syspld_reg_write(SYSPLD_MISC, regVal);
        i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);
#endif    

#if defined(MV_INCLUDE_UNM_ETH) || defined(MV_INCLUDE_GIG_ETH)
    /* Init the PHY or Switch of the board */
    mvBoardEgigaPhySwitchInit();
#endif /* #if defined(MV_INCLUDE_UNM_ETH) || defined(MV_INCLUDE_GIG_ETH) */
	
#ifdef SYSPLD_WD_ISR
    /* enable GPIO0-7 int */
    MV_REG_WRITE(CPU_MAIN_IRQ_MASK_HIGH_REG, 0x8);
#endif

    return (0);
}

void 
misc_init_r_env (void)
{
    char *env;
    char str;
#if !defined(CONFIG_PRODUCTION)
    char tmp_buf[12];
    unsigned int malloc_len;
#endif
    int board_id = mvBoardIdGet();	
    unsigned char temp[60];
    uint8_t  enet[6], esize;
    char *s;
	
    /* arch number according to board ID */
    gd->bd->bi_arch_number = board_id;

#if 0
    /* update the CASset env parameter */
    env = getenv("CASset");
    if (!env )
    {
#ifdef MV_MIN_CAL
        setenv("CASset", "min");
#else
        setenv("CASset", "max");
#endif
    }
#endif

#if !defined(CONFIG_PRODUCTION)
    /* Pex mode */
    setenv("pexMode", "RC");

    /* CPU streaming */
    env = getenv("enaCpuStream");
    if (!env || ((strcmp(env, "no") == 0) || (strcmp(env, "No") == 0)))
        setenv("enaCpuStream", "no");
    else
        setenv("enaCpuStream", "yes");

    /* Write allocation */
    env = getenv("enaWrAllo");
    if (!env || (((strcmp(env, "no") == 0) || (strcmp(env, "No") == 0))))
        setenv("enaWrAllo", "no");
    else
        setenv("enaWrAllo", "yes");

    env = getenv("disL2Cache");
    if (!env || ((strcmp(env, "no") == 0) || (strcmp(env, "No") == 0)))
        setenv("disL2Cache", "no"); 
    else
        setenv("disL2Cache", "yes");

    env = getenv("setL2CacheWT");
    if (!env || ((strcmp(env, "yes") == 0) || (strcmp(env, "Yes") == 0)))
        setenv("setL2CacheWT", "yes"); 
    else
        setenv("setL2CacheWT", "no");

    env = getenv("disL2Prefetch");
    if (!env || ((strcmp(env, "yes") == 0) || (strcmp(env, "Yes") == 0))) {
        setenv("disL2Prefetch", "yes"); 

        /* ICache Prefetch */
        env = getenv("enaICPref");
        if (env && (((strcmp(env, "no") == 0) || (strcmp(env, "No") == 0))))
            setenv("enaICPref", "no");
        else
            setenv("enaICPref", "yes");

        /* DCache Prefetch */
        env = getenv("enaDCPref");
        if (env && (((strcmp(env, "no") == 0) || (strcmp(env, "No") == 0))))
            setenv("enaDCPref","no");
        else
            setenv("enaDCPref", "yes");
    }
    else {
        setenv("disL2Prefetch", "no");
        setenv("enaICPref", "no");
        setenv("enaDCPref", "no");
    }
#endif

#if !defined(CONFIG_EX2200)
    /* Malloc length */
    env = getenv("MALLOC_len");
    malloc_len =  simple_strtoul(env, NULL, 10) << 20;
    if (malloc_len == 0){
        sprintf(tmp_buf, "%d", (CFG_MALLOC_LEN >> 20));
        setenv("MALLOC_len", tmp_buf);
    }
#endif

    /* primary network interface */
    env = getenv("ethprime");
    if (!env)
        setenv("ethprime", ENV_ETH_PRIME);
 
    if (gd->valid_bid == 0) {
        enet[0] = 0x00;
        enet[1] = 0x00;
        enet[2] = 0x00;
        enet[3] = 0x00;
        enet[4] = 0x51;
        enet[5] = 0x80;
    }
    else {
            i2c_mux(I2C_MAIN_MUX_ADDR, 3, 1);
            i2c_read_norsta(0, I2C_MAIN_EEPROM_ADDR, 56, 1, enet, 6);
            i2c_read_norsta(0, I2C_MAIN_EEPROM_ADDR, 55, 1, &esize, 1);
            i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);
    }

    if ((enet[0] |enet[1] | enet[2] | enet[3] | enet[4] | enet[5]) && 
        ((esize == 0x20) || (esize == 0x40))) {
        sprintf(temp, "%02x:%02x:%02x:%02x:%02x:%02x",
                enet[0], enet[1], enet[2], enet[3], enet[4], enet[5]+(esize-1));
    }
    else {
        enet[0] = 0x00;
        enet[1] = 0x00;
        enet[2] = 0x00;
        enet[3] = 0x00;
        enet[4] = 0x51;
        enet[5] = 0x80;
        
        sprintf(temp, "%02x:%02x:%02x:%02x:%02x:%02x",
                enet[0], enet[1], enet[2], enet[3], enet[4], enet[5]);
    }
    
    s = getenv("ethaddr");
    if (memcmp (s, temp, sizeof(temp)) != 0) {
        setenv("ethaddr", temp);
    }
    
#if 0
    s = getenv("bootcmd.prior");
    if (s) {
        sprintf(temp,"%s; %s", s, "bootelf CFG_READONLY_LOADER_ADDR");
        setenv("bootcmd", temp);
    }
    else
        setenv("bootcmd", "bootelf CFG_READONLY_LOADER_ADDR");
#endif

    return;
}

int 
board_late_init (void)
{
    uint8_t tdata, regVal_8;
    unsigned char temp[60];
    char *s, *str;
    int mloop = 1;
    int loopback = 0;
    int ledoff = 0;
    uint32_t ether_fail = 0;
    uint32_t regVal;
    uint16_t id;
#if defined(CONFIG_READONLY)
    ulong state = get_upgrade_state();
#endif

    /* temperature diode setup */
    if (mvSwitchReadReg(0, 0x48, &regVal) == 0) {
        regVal |= BIT21;
        mvSwitchWriteReg(0, 0x48, regVal);
    }
    if (mvSwitchReadReg(1, 0x48, &regVal) == 0) {
        regVal |= BIT21;
        mvSwitchWriteReg(1, 0x48, regVal);
    }

    /* default turn off LED for uplink ports */
    if (mvSwitchGetDevicesNum() == 2) {  /* 2 Cheetahs */
        mvSwitchWriteReg(0, 0x4005110, 0x231);
        mvSwitchWriteReg(0, 0x4805100, 0x20800000);
        mvSwitchWriteReg(0, 0x4805104, 0xaabbffff);
        mvSwitchWriteReg(1, 0x5005110, 0x231);
        mvSwitchWriteReg(1, 0x5805100, 0x20800000);
        mvSwitchWriteReg(1, 0x5805104, 0xaabbffff);
    }
    else {  /* 1 Cheetah */
        mvSwitchWriteReg(0, 0x4005110, 0x231);
        mvSwitchWriteReg(0, 0x5005110, 0x231);
        mvSwitchWriteReg(0, 0x4805100, 0x20800000);
        mvSwitchWriteReg(0, 0x5805100, 0x20800000);
        mvSwitchWriteReg(0, 0x4805104, 0xaabbffff);
        mvSwitchWriteReg(0, 0x5805104, 0xaabbffff);
    }

#if !defined(CONFIG_EX2200_12X)    
        /* setup windbond */
        /* temperature */
        i2c_mux(I2C_MAIN_MUX_ADDR, 2, 1);
        tdata = 0x80;  /* bank 0 */
        i2c_write(0, I2C_WINBOND_ADDR, 0x4e, 1, &tdata, 1);
        i2c_read(0, I2C_WINBOND_ADDR, 0x5d, 1, &tdata, 1);
        tdata |= 0x0e;
        i2c_write(0, I2C_WINBOND_ADDR, 0x5d, 1, &tdata, 1);
        i2c_read(0, I2C_WINBOND_ADDR, 0x59, 1, &tdata, 1);
        tdata &= ~0x70;
        tdata |= 0x60;
        i2c_write(0, I2C_WINBOND_ADDR, 0x59, 1, &tdata, 1);

        /* fan */
        i2c_read(0, I2C_WINBOND_ADDR, 0x47, 1, &tdata, 1);
        tdata |= 0xf0;
        i2c_write(0, I2C_WINBOND_ADDR, 0x47, 1, &tdata, 1);
        i2c_read(0, I2C_WINBOND_ADDR, 0x4b, 1, &tdata, 1);
        tdata |= 0xc0;
        i2c_write(0, I2C_WINBOND_ADDR, 0x4b, 1, &tdata, 1);

        syspld_reg_read(SYSPLD_MISC, &regVal_8);
        if (regVal_8 & SYSPLD_POE_PRESENT)
            /* all fans with initial setting 25% duty cycle with PoE present */
            tdata = 0x40;
        else
            /* all fans with initial setting 8.2% duty cycle */
            tdata = 0x15;

        i2c_write(0, I2C_WINBOND_ADDR, 0x5b, 1, &tdata, 1);
        i2c_write(0, I2C_WINBOND_ADDR, 0x5e, 1, &tdata, 1);
        i2c_write(0, I2C_WINBOND_ADDR, 0x5f, 1, &tdata, 1);

        /* RTC */
        i2c_read(0, I2C_RTC_ADDR, 2, 1, &tdata, 1);
        i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);

        if (tdata & 0x80) {  /* RTC voltage low */
            rtc_init();
            rtc_stop();
            rtc_start();
            rtc_set_time(0, 1, 1, 0, 0, 0);  /* default 2000/1/1 00:00:00 */
        }
#endif    

#ifdef SYSPLD_WD_ISR
    /* enable syspld watchdog */
    syspld_reg_read(SYSPLD_WATCHDOG_ENABLE, &tdata);
    tdata |= SYSPLD_WDOG_ENABLE;
    syspld_reg_write(SYSPLD_WATCHDOG_ENABLE, tdata);
#endif

    /* environment variable for board type */
    id = board_idtype();
    if (id == 0xFFFF)
        id = I2C_ID_JUNIPER_EX2200_24_T;
    sprintf(temp,"%04x ", id);
    setenv("hw.board.type", temp);

    /* environment variable for last reset */
    sprintf(temp,"%02x ", gd->last_reset);
    setenv("hw.board.reset", temp);
 
#if defined(CONFIG_UPGRADE)
    /* environment variable upgrade loader ELF addr */
    sprintf(temp,"0x%08x ", CFG_UPGRADE_LOADER_ADDR);
    setenv("loadaddr", temp);
#else
    /* environment variable readonly loader ELF addr */
    sprintf(temp,"0x%08x ", CFG_READONLY_LOADER_ADDR);
    setenv("loadaddr", temp);
#endif

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
    s = (char *) (CFG_UPGRADE_BOOT_IMG_ADDR + CFG_VERSION_STRING_OFFSET);
    setenv("boot.upgrade.ver", s);
    s = (char *) (CFG_READONLY_BOOT_IMG_ADDR + CFG_VERSION_STRING_OFFSET);
    setenv("boot.readonly.ver", s);
#endif

#if defined(CONFIG_READONLY)
    s = NULL;
    if (state >= UPGRADE_CHECK) {
        s = (char *) (CFG_UPGRADE_BOOT_IMG_ADDR + CFG_VERSION_STRING_OFFSET);
    }
    setenv("boot.upgrade.ver", s);
    s = (char *) (CFG_READONLY_BOOT_IMG_ADDR + CFG_VERSION_STRING_OFFSET);
    setenv("boot.readonly.ver", s);
#endif

    /* set the internal uboot version */
    sprintf(temp, " %d.%d.%d",
	    CFG_VERSION_MAJOR, CFG_VERSION_MINOR, CFG_VERSION_PATCH);
    setenv("boot.intrver", temp);

    if ((s = getenv("boot.test")) != NULL) {
	  mloop = simple_strtoul (s, NULL, 16);
        {
            uint32_t test_delay = 0;

            s = getenv("boot.test.delay");
            if (s != NULL)
                test_delay = simple_strtoul(s, NULL, 10);
            if (test_delay)
                udelay(test_delay);
	  }
         if (memtest(0x400000, gd->ram_size - 4, mloop, 1)) {
             setenv ("boot.ram", "FAILED ");
         }
         else {
             setenv ("boot.ram", "PASSED ");
         }
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
    
    if ((s = getenv("boot.ledoff")) != NULL) {
        ledoff = simple_strtoul (s, NULL, 10);
        if (ledoff) {
            ledUpdateEna = 0;
        }
        else
            ledUpdateEna = 1;
    }
    
    /* uboot/loader memory map */
    sprintf(temp,"0x%08x ", CFG_FLASH_BASE);
    setenv("boot.flash.start", temp);

    sprintf(temp,"0x%08x ", CFG_FLASH_SIZE);
    setenv("boot.flash.size", temp);

    sprintf(temp,"0x%08x ", CFG_ENV_OFFSET);
    setenv("boot.env.start", temp);

    sprintf(temp,"0x%08x ", CFG_ENV_SIZE);
    setenv("boot.env.size", temp);

    sprintf(temp,"0x%08x ", CFG_OPQ_OFFSET);
    setenv("boot.opqenv.start", temp);

    sprintf(temp,"0x%08x ", CFG_OPQ_ENV_SECT_SIZE);
    setenv("boot.opqenv.size", temp);

    sprintf(temp,"0x%08x ", CFG_UPGRADE_BOOT_STATE_OFFSET);
    setenv("boot.upgrade.state", temp);
    
    sprintf(temp,"0x%08x ", CFG_UPGRADE_LOADER_ADDR - CFG_FLASH_BASE);
    setenv("boot.upgrade.loader", temp);
    
    sprintf(temp,"0x%08x ", CFG_UPGRADE_BOOT_IMG_ADDR - CFG_FLASH_BASE);
    setenv("boot.upgrade.uboot", temp);

    sprintf(temp,"0x%08x ", CFG_READONLY_LOADER_ADDR - CFG_FLASH_BASE);
    setenv("boot.readonly.loader", temp);
    
    sprintf(temp,"0x%08x ", CFG_READONLY_BOOT_IMG_ADDR - CFG_FLASH_BASE);
    setenv("boot.readonly.uboot", temp);

    s = getenv("bootcmd.prior");
    if (s) {
        sprintf(temp,"%s; %s", s, CONFIG_BOOTCOMMAND);
        setenv("bootcmd", temp);
    }
    else
        setenv("bootcmd", CONFIG_BOOTCOMMAND);

    sprintf(temp,"0x%02x ", I2C_MAIN_EEPROM_ADDR);
    setenv("boot.ideeprom", temp);

    if (gd->env_valid == 3) {
        /* save default environment */
		gd->flags |= GD_FLG_SILENT;
		saveenv();
		gd->flags &= ~GD_FLG_SILENT;
		gd->env_valid = 1;
    }

    return (0);
}

int 
misc_init_r (void)
{
    uint32_t regVal;
#if !defined(CONFIG_SIMPLIFY_DISPLAY)
    char name[128];
	
    mvCpuNameGet(name);
    printf("\nCPU : %s\n",  name);
#endif

    /* init special env variables */
    misc_init_r_env();

    mv_cpu_init();

#if defined(MV_INCLUDE_MONT_EXT)
    if (enaMonExt()) {
#if !defined(CONFIG_SIMPLIFY_DISPLAY)
        printf("\n      Marvell monitor extension:\n");
#endif
        mon_extension_after_relloc();
    }
#if !defined(CONFIG_SIMPLIFY_DISPLAY)
    printf("\n");		
#endif
#endif /* MV_INCLUDE_MONT_EXT */

#if !defined(CONFIG_SIMPLIFY_DISPLAY)
    printf("\n");		
#endif
    /* init the units decode windows */
    misc_init_r_dec_win();

    if (board_pci()) {
        pci_init();
    }

    mvSysClkGet();
    switchClkGet();

    led_init();
    /* booting */
#if !defined(CONFIG_EX2200_12X)
    set_led(SYS_LED, LED_GREEN, LED_SLOW_BLINKING);
#else
    set_led(SYS_LED, LED_GREEN, LED_HW_BLINKING);
#endif
    /* unmask timer2 interrupt */
    regVal = MV_REG_READ(CPU_AHB_MBUS_MASK_INT_REG);
    MV_REG_WRITE(CPU_AHB_MBUS_MASK_INT_REG, regVal|BIT2);
    MV_REG_WRITE(CPU_MAIN_IRQ_MASK_REG, BIT1);

    /* Print the juniper internal version of the u-boot */
    printf("\nFirmware Version:%02X.%02X.%02X \n",
	   CFG_VERSION_MAJOR, CFG_VERSION_MINOR, CFG_VERSION_PATCH);
    return (0);
}

MV_U32 
mvTclkGet (void)
{
    static MV_U32 tclk = 0;
    
    /* get it only on first time */
    if (tclk == 0) {
        tclk = gd->tclk = mvBoardTclkGet();
    }

    return (tclk);
}

MV_U32 
mvSysClkGet (void)
{
    /* get it only on first time */
    if (gd->bus_clk == 0)
        gd->bus_clk = mvBoardSysClkGet();

    return (gd->bus_clk);
}

/* exported for EEMBC */
MV_U32 
mvGetRtcSec (void)
{
    return (0);
}

void 
reset_cpu (void)
{
    MV_REG_BIT_SET(CPU_RSTOUTN_MASK_REG , BIT2);
    MV_REG_BIT_SET(CPU_SYS_SOFT_RST_REG , BIT0);
}

void 
mv_cpu_init (void)
{
#if !defined(CONFIG_PRODUCTION)
    char *env;
#endif
    volatile unsigned int temp;

#if defined(CONFIG_PRODUCTION)
    /* enaWrAllo = no */
    {
        __asm__ __volatile__("mrc    p15, 1, %0, c15, c1, 0" : "=r" (temp));
        temp &= ~BIT28;
        __asm__ __volatile__("mcr    p15, 1, %0, c15, c1, 0" :: "r" (temp));
    }
#else
    /*CPU streaming & write allocate */
    env = getenv("enaWrAllo");
    if (env && ((strcmp(env, "yes") == 0) ||  (strcmp(env, "Yes") == 0)))  
    {
        __asm__ __volatile__("mrc    p15, 1, %0, c15, c1, 0" : "=r" (temp));
        temp |= BIT28;
        __asm__ __volatile__("mcr    p15, 1, %0, c15, c1, 0" :: "r" (temp));
    }
    else
    {
        __asm__ __volatile__("mrc    p15, 1, %0, c15, c1, 0" : "=r" (temp));
        temp &= ~BIT28;
        __asm__ __volatile__("mcr    p15, 1, %0, c15, c1, 0" :: "r" (temp));
    }
#endif

#if defined(CONFIG_PRODUCTION)
    /* enaCpuStream = no */
    {
        __asm__ __volatile__("mrc    p15, 1, %0, c15, c1, 0" : "=r" (temp));
        temp &= ~BIT29;
        __asm__ __volatile__("mcr    p15, 1, %0, c15, c1, 0" :: "r" (temp));
    }
#else
    env = getenv("enaCpuStream");
    if (!env || (strcmp(env, "no") == 0) ||  (strcmp(env, "No") == 0) )
    {
        __asm__ __volatile__("mrc    p15, 1, %0, c15, c1, 0" : "=r" (temp));
        temp &= ~BIT29;
        __asm__ __volatile__("mcr    p15, 1, %0, c15, c1, 0" :: "r" (temp));
    }
    else
    {
        __asm__ __volatile__("mrc    p15, 1, %0, c15, c1, 0" : "=r" (temp));
        temp |= BIT29;
        __asm__ __volatile__("mcr    p15, 1, %0, c15, c1, 0" :: "r" (temp));
    }
#endif

#if defined(CONFIG_SIMPLIFY_DISPLAY)
    /* Verifay write allocate and streaming */
    __asm__ __volatile__("mrc    p15, 1, %0, c15, c1, 0" : "=r" (temp));
#else
    /* Verifay write allocate and streaming */
    printf("\n");
	__asm__ __volatile__("mrc    p15, 1, %0, c15, c1, 0" : "=r" (temp));
    if (temp & BIT29)
        printf("Streaming enabled \n");
    else
        printf("Streaming disabled \n");
    if (temp & BIT28)
        printf("Write allocate enabled\n");
    else
        printf("Write allocate disabled\n");
#endif

#if defined(CONFIG_PRODUCTION)
    /* enaDCPref = no  */    {
        MV_REG_BIT_RESET( CPU_CONFIG_REG , CCR_DCACH_PREF_BUF_ENABLE);
    }
#else
    /* DCache Pref  */
    env = getenv("enaDCPref");
    if (env && ((strcmp(env, "yes") == 0) ||  (strcmp(env, "Yes") == 0)))
    {
        MV_REG_BIT_SET( CPU_CONFIG_REG , CCR_DCACH_PREF_BUF_ENABLE);
    }

    if (env && ((strcmp(env, "no") == 0) ||  (strcmp(env, "No") == 0)))
    {
        MV_REG_BIT_RESET( CPU_CONFIG_REG , CCR_DCACH_PREF_BUF_ENABLE);
    }
#endif

#if defined(CONFIG_PRODUCTION)
    /* enaICPref = no  */    {
        MV_REG_BIT_RESET( CPU_CONFIG_REG , CCR_ICACH_PREF_BUF_ENABLE);
    }
#else
    /* ICache Pref  */
    env = getenv("enaICPref");
    if (env && ((strcmp(env, "yes") == 0) ||  (strcmp(env, "Yes") == 0)))
    {
        MV_REG_BIT_SET( CPU_CONFIG_REG , CCR_ICACH_PREF_BUF_ENABLE);
    }

    if (env && ((strcmp(env, "no") == 0) ||  (strcmp(env, "No") == 0)))
    {
        MV_REG_BIT_RESET( CPU_CONFIG_REG , CCR_ICACH_PREF_BUF_ENABLE);
    }
#endif

#if defined(CONFIG_PRODUCTION)
    /* setL2CacheWT = yes */
    {
        temp |= BIT4;
    }
#else
    /* Set L2C WT mode - Set bit 4 */
    temp = MV_REG_READ(CPU_L2_CONFIG_REG);
    env = getenv("setL2CacheWT");
    if (!env || ( (strcmp(env, "yes") == 0) || (strcmp(env, "Yes") == 0) ) )
    {
        temp |= BIT4;
    }
    else
        temp &= ~BIT4;
#endif
    MV_REG_WRITE(CPU_L2_CONFIG_REG, temp);

    /* L2Cache settings */
    asm ("mrc p15, 1, %0, c15, c1, 0":"=r" (temp));

#if defined(CONFIG_PRODUCTION)
    /* disL2Prefetch = yes */
    {
        temp |= BIT24;
    }
#else
    /* Disable L2C pre fetch - Set bit 24 */
    env = getenv("disL2Prefetch");
    if (env && ( (strcmp(env, "no") == 0) || (strcmp(env, "No") == 0) ) )
        temp &= ~BIT24;
    else
        temp |= BIT24;
#endif

#if defined(CONFIG_PRODUCTION)
    /* disL2Cache = no */
    {
        temp |= BIT22;
    }
#else
    /* enable L2C - Set bit 22 */
    env = getenv("disL2Cache");
    if (!env || ((strcmp(env, "no") == 0) || (strcmp(env, "No") == 0)))
        temp |= BIT22;
    else
        temp &= ~BIT22;
#endif

    asm ("mcr p15, 1, %0, c15, c1, 0": :"r" (temp));

    /* Enable i cache */
    asm ("mrc p15, 0, %0, c1, c0, 0":"=r" (temp));
    temp |= BIT12;
    asm ("mcr p15, 0, %0, c1, c0, 0": :"r" (temp));

    /* Change reset vector to address 0x0 */
    asm ("mrc p15, 0, %0, c1, c0, 0":"=r" (temp));
    temp &= ~BIT13;
    asm ("mcr p15, 0, %0, c1, c0, 0": :"r" (temp));
}


/*******************************************************************************
* mvUpdateNorFlashBaseAddrBank - 
*
* DESCRIPTION:
*       This function update the CFI driver base address bank with on board NOR
*       devices base address.
*
* INPUT:
*
* OUTPUT:
*
* RETURN:
*       None
*
*******************************************************************************/
#ifdef	CFG_FLASH_CFI_DRIVER
MV_VOID 
mvUpdateNorFlashBaseAddrBank (MV_VOID)
{
    
    MV_U32 devBaseAddr;
    MV_U32 devNum = 0;
    int i;

    /* Update NOR flash base address bank for CFI flash init driver */
    for (i = 0 ; i < CFG_MAX_FLASH_BANKS_DETECT; i++)
    {
	devBaseAddr = mvBoardGetDeviceBaseAddr(i, BOARD_DEV_NOR_FLASH);
	if (devBaseAddr != 0xFFFFFFFF)
	{
	    flash_add_base_addr(devNum, devBaseAddr);
	    devNum++;
	}
    }
    mv_board_num_flash_banks = devNum;

    /* Update SPI flash count for CFI flash init driver */
    /* Assumption only 1 SPI flash on board */
    for (i = 0 ; i < CFG_MAX_FLASH_BANKS_DETECT; i++)
    {
    	devBaseAddr = mvBoardGetDeviceBaseAddr(i, BOARD_DEV_SPI_FLASH);
    	if (devBaseAddr != 0xFFFFFFFF)
		mv_board_num_flash_banks += 1;
    }

}
#endif	/* CFG_FLASH_CFI_DRIVER */

#if defined(CONFIG_SPI_SW_PROTECT)
void
exSpiSwLock (void)
{
    int i;
    uint8_t data;

    /* SPI SW protect WRLR */
    for (i = 0; i < 0xFFFFF; i += 0x10000) {
        if (ex_spi_sw_protect_get(i, &data, 1) != 0)
            break;
        if ((data & 0x3) != 0x3) {
            data = 0x3;
            /* re-protect this sector */
            ex_spi_sw_protect_set(i, &data, 1);
        }
    }
}
#endif

int32_t 
exflashInit (void)
{
    static int32_t size = 0;

    if (size == 0) {
        size = flash_init ();
        size += mvFlash_init();
    }
    
    return (size);
}

uint32_t 
getUsbBase (int dev)
{
    return (INTER_REGS_BASE | USB_REG_BASE(0));
}

uint32_t 
getUsbHccrBase (int dev)
{
    return (INTER_REGS_BASE | MV_USB_CORE_CAP_LENGTH_REG(0));
}

uint32_t 
getUsbHcorBase (int dev)
{
    return (INTER_REGS_BASE | MV_USB_CORE_CMD_REG(0));
}

int
board_idtype (void)
{
    return ((gd->board_type & 0xffff0000) >> 16);
}

int 
version_major (void)
{
    return ((gd->board_type & 0xff00) >> 8);
}

int 
version_minor (void)
{
    return (gd->board_type & 0xff);
}

int 
board_pci (void)
{
    int id = board_idtype();

    if ((id == I2C_ID_JUNIPER_EX2200_48_T) ||
        (id == I2C_ID_JUNIPER_EX2200_48_P))
        return (1);
    else
        return (0);
}

int 
check_upgrade (void)
{
    exflashInit();
    
#if defined(CONFIG_READONLY)
    ulong addr, src, dest;
    upgrade_state_t state = get_upgrade_state();
    
    if (state == UPGRADE_START) {
        /* verify upgrade */
        if (valid_env(CFG_ENV_ADDR) &&
             valid_loader_image(CFG_UPGRADE_LOADER_ADDR) &&
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

        /* copy U-boot to memroy */
        dest = CFG_UPGRADE_UBOOT_LOAD_ADDR;
        src = CFG_UPGRADE_BOOT_IMG_ADDR + CFG_DOIMGAGE_HDR_SIZE;
        memcpy(dest, src, CFG_MONITOR_LEN);
        
        /* select upgrade image to be boot from */
        addr = CFG_UPGRADE_UBOOT_LOAD_ADDR;
        ((ulong (*)())addr) ();;
    }
#endif

    return (0);
}

int 
checkboard (void)
{
    uint32_t id = 0xffffffff;
    uint8_t syspld_version = 0x0, syspld_LastRst;
    uint8_t temp[6];
    MV_TWSI_ADDR slave;

#if defined(CONFIG_SPI_SW_PROTECT)
    exSpiSwLock();
#endif

    /* kludge for I2C reset, neede for upgrade U-boot reading EEPROM */
    slave.type = ADDR7_BIT;
    slave.address = 0;
    mvTwsiInit(0, CFG_I2C_SPEED, CFG_TCLK, &slave, 0);

    syspld_read(0, temp, 6);
    syspld_version = temp[0];
    gd->valid_bid = 0;
    syspld_reg_read(SYSPLD_LAST_RESET, &syspld_LastRst);
    gd->last_reset = syspld_LastRst;

    /* ID */
    if (i2c_mux(I2C_MAIN_MUX_ADDR, 3, 1) == 0) {
        if (i2c_read_norsta(0, I2C_MAIN_EEPROM_ADDR, 4, 1, (uint8_t *)&id, 4))
        printf("seeprom read failed\n");
        gd->valid_bid = 1;
    }
    i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);
    gd->board_type = swap_ulong(id);

    switch (board_idtype()) {
        case I2C_ID_JUNIPER_EX2200_12_T:
            puts("Board: EX2200-C-12T-2G");
            break;
        case I2C_ID_JUNIPER_EX2200_12_P:
            puts("Board: EX2200-C-12P-2G");
            break;
        case I2C_ID_JUNIPER_EX2200_24_T:
            puts("Board: EX2200-24T-4G");
            break;
        case I2C_ID_JUNIPER_EX2200_24_P:
            puts("Board: EX2200-24P-4G");
            break;
        case I2C_ID_JUNIPER_EX2200_48_T:
            puts("Board: EX2200-48T-4G");
            break;
        case I2C_ID_JUNIPER_EX2200_48_P:
            puts("Board: EX2200-48P-4G");
            break;
        case I2C_ID_JUNIPER_EX2200_24_T_DC:
            puts("Board: EX2200-24T-DC-4G");
            break;
        default:
            puts("Board: Unknown");
            gd->valid_bid = 0;
            break;
    };
    printf(" %d.%d\n", version_major(), version_minor());
#if !defined(CONFIG_EX2200_12X)    
    printf("EPLD:  Version %02x.%02x%02x%02x (0x%02x)\n",
           syspld_version, temp[5], temp[3], temp[2], 
           syspld_LastRst);
#else    
    printf("EPLD:  Version %02x (0x%02x)\n", syspld_version,
           syspld_LastRst);
#endif    
    syspld_reg_write(SYSPLD_LAST_RESET, 0);
    checkboard_done = 1;
    return (0);
}

int 
post_stop_prompt (void)
{
    char *s;
    int mem_result;
    
    if (getenv("boot.nostop")) return (0);

    if (((s = getenv("boot.ram")) != NULL) &&
         ((strcmp(s,"PASSED") == 0) ||
         (strcmp(s,"FAILED") == 0))) {
         mem_result = 0;
    }
    else {
         mem_result = post_result_mem;
    }

    if (mem_result == -1 || post_result_cpu == -1 ||
        post_result_ether == -1 ||
        gd->valid_bid == 0)
        return (1);
    return (0);
}

void 
ex_irq_handler (void)
{
    static int tick_count = 0;
    volatile uint32_t regVal;
    uint16_t link_status;
    static uint16_t link_status_old = 0;
    extern unsigned int egiga_init;
    extern int i2c_mux_busy;

    if (checkboard_done == 0)
        return;

#ifdef SYSPLD_WD_ISR
    regVal = MV_REG_READ(CPU_MAIN_INT_CAUSE_HIGH_REG);
    if (regVal & BIT3) { /* syspld interrupt */
        syspld_watchdog_petting();
        MV_REG_WRITE(CPU_MAIN_INT_CAUSE_HIGH_REG, regVal & (~BIT3));
    }
#endif

    regVal = MV_REG_READ(CPU_AHB_MBUS_CAUSE_INT_REG);
    if (regVal & BIT2 == 0)  /* not timer2 interrupt */
        return;
    MV_REG_WRITE(CPU_AHB_MBUS_CAUSE_INT_REG, regVal & (~BIT2));

    /* Give up this chance.  Do not interrupt mgmt port operation */
    if (egiga_init || i2c_mux_busy)
        return;
        
    tick_count++;
    if (tick_count >= 20)  /* 2 seconds */
        tick_count = 0;

    /* 2 seconds */
    if (eth_get_dev() && (tick_count == 0)) {
        mvEthPhyRegRead(1, 17, &link_status);
        if ((link_status_old & BIT10) != (link_status & BIT10)) {
            if (link_status & BIT10)  /* link up */
                set_mgmt_led(LINK_UP,
                                  (link_status & BIT13) ? FULL_DUPLEX : HALF_DUPLEX,
                                  (link_status & BIT14) ? SPEED_100M : SPEED_10M);
            else
                set_mgmt_led(LINK_DOWN, 0, 0);  /* don't care */  
            
            link_status_old = link_status;
        }
    }

    /* 100ms */
    if (ledUpdateEna)
        update_led ();
}

uint8_t 
crc_8 (uint32_t start, uint32_t len, uint8_t csum)
{
    register uint8_t sum = csum;
    volatile uint8_t* startp = (volatile uint8_t*)start;

    if (len == 0)
        return (sum);

    do {
        sum += *startp;
        startp++;
    } while(--len);

    return (sum);
}

int 
check_main_header (BHR_t *pBHR, uint8_t headerID)
{
    uint8_t	 chksum;

    /* Verify Checksum */
    chksum = crc_8((MV_U32)pBHR, sizeof(BHR_t) -1, 0);

    if (chksum != pBHR->checkSum)
        return (-1);

    /* Verify Header */
    if (pBHR->blockID != headerID)
        return (-1);
	
    /* Verify Alignment */
    if (pBHR->blockSize & 0x3)
        return (-1);

    if ((cpu_to_le32(pBHR->destinationAddr) & 0x3) && (cpu_to_le32(pBHR->destinationAddr) != 0xffffffff))
        return (-1);

    if ((cpu_to_le32(pBHR->sourceAddr) & 0x3) && (pBHR->blockID != IBR_HDR_SATA_ID))
        return (-1);

    return (0);
}

/*
 * Check the extended header and execute the image
 */
int 
check_ext_header (ExtBHR_t *extBHR)
{
    uint8_t	 chksum;

    /* Caclulate abd check the checksum to valid */
    chksum = crc_8((uint32_t)extBHR , EXT_HEADER_SIZE -1, 0);
    if (chksum != (*(uint8_t*)((uint32_t)extBHR + EXT_HEADER_SIZE - 1)))
        return (-1);

    return (0);
}

int 
valid_env (unsigned long addr)
{
    return (1);
}

int 
valid_loader_image (unsigned long addr)
{
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *) addr;

    if (!IS_ELF (*ehdr))
        return (0);
    return (1);
}

int 
valid_uboot_image (unsigned long addr)
{
    BHR_t*        tmpBHR = (BHR_t*)addr;
    ExtBHR_t*   extBHR = (ExtBHR_t*)(addr + BHR_HDR_SIZE);

    if (check_main_header(tmpBHR, IBR_HDR_SPI_ID))
        return (0);

    if (check_ext_header(extBHR))
        return (0);

    return (1);
}

int
set_upgrade_state (upgrade_state_t state)
{
    ulong	upgrade_addr = CFG_FLASH_BASE + CFG_UPGRADE_BOOT_STATE_OFFSET;
    ulong ustate = state;
    extern int flash_write_direct (ulong addr, char *src, ulong cnt);

    return  flash_write_direct(upgrade_addr, (char *)&ustate, sizeof(ulong));
}

int 
get_upgrade_state (void)
{
    volatile upgrade_state_t *ustate = (upgrade_state_t *) (CFG_FLASH_BASE + CFG_UPGRADE_BOOT_STATE_OFFSET);

    if ((*ustate < UPGRADE_READONLY) || (*ustate > UPGRADE_OK))
    {
        set_upgrade_state(UPGRADE_READONLY);
        return (UPGRADE_READONLY);
    }
    
    return (*ustate);
}

int 
set_upgrade_ok (void)
{
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

    return  (0);
}
#ifdef CONFIG_POST
void 
post_word_store (ulong a)
{
}

ulong 
post_word_load (void)
{
    return (0);
}

/*
 *  *  *  * Returns 1 if keys pressed to start the power-on long-running tests
 *   *   *   * Called from board_init_f().
 *    *    *    */
int 
post_hotkeys_pressed(void)
{
    return (0);   /* No hotkeys supported */
}

#endif

/* post test name declartion */
static char *post_test_name[4] = {"cpu","ram","pci","ethernet"};

void
ex_post_run (void)
{
    int i;
    
    /* Run Post test to validate the cpu,ram,pci and ethernet(mac,phy loopback) */
    for (i = 0; i < 4; i++) {
        if (post_run (post_test_name[i],
		    POST_RAM | POST_MANUAL | BOOT_POST_FLAG) != 0)
		    debug(" Unable to execute the POST test %s\n", post_test_name[i]);
    }
           
    if (post_result_cpu == -1) {
        setenv ("post.cpu", "FAILED ");
    }
    else {
        setenv ("post.cpu", "PASSED ");
    }
    if (post_result_mem == -1) {
        setenv ("post.ram", "FAILED ");
    }
    else {
        setenv ("post.ram", "PASSED ");
    }
    if (post_result_pci == -1) {
        setenv ("post.pci", "FAILED ");
    }
    else {
        setenv ("post.pci", "PASSED ");
    }
    if (post_result_ether == -1) {
        setenv ("post.ethernet", "FAILED ");
    }
    else {
        setenv ("post.ethernet", "PASSED ");
    }
    if (gd->valid_bid == 0) {
        setenv ("post.eeprom", "FAILED ");
    }
    else {
        setenv ("post.eeprom", "PASSED ");
    }
           
    /* display POST error only. */
    if (post_result_cpu == -1) {
        printf("POST: cpu FAILED\n");
    }
    if (post_result_mem == -1) {
        printf("POST: ram FAILED\n");
    }
    if (post_result_pci == -1) {
        printf("POST: pci FAILED\n");
    }
    if (post_result_ether == -1) {
        printf("POST: ethernet FAILED\n");
    }
    if (gd->valid_bid == 0) {
        printf("POST: eeprom FAILED\n");
    }
}

uint32_t 
single_bit_ecc_errors (void)
{
    uint32_t count = MV_REG_READ(SDRAM_SINGLE_BIT_ERR_CNTR_REG);
    
    MV_REG_WRITE(SDRAM_SINGLE_BIT_ERR_CNTR_REG, 0);
    return count;
}

uint32_t 
double_bits_ecc_errors (void)
{
    uint32_t count = MV_REG_READ(SDRAM_DOUBLE_BIT_ERR_CNTR_REG);
    
    MV_REG_WRITE(SDRAM_DOUBLE_BIT_ERR_CNTR_REG, 0);
    return count;
}


int get_unattended_mode(void)
{

    char *str;
    uint8_t unattn_mode;


    str = getenv("boot_unattended");
    
    
    if (str != NULL) {
	unattn_mode = simple_strtoul(str, NULL, 10);
    } else {
	unattn_mode = 0;

    }
    
    return unattn_mode;
        
}

int clear_nand_data (void)
{

    int nand_dev;
    unsigned long numblk, blksize, write_cnt, start, numblk_erase;
    unsigned long numblk_loop;
    char *write_buffer = NULL;
    char response[256] = { 0, };
    int len;


    /*
     * Clear the previous console input
     */
    flush_console_input();


    /*
     * Confirm that user wants destructive writes
     * if No exit, if no input syspld watchdog will reset
     * the board upon expiry
     *
     */
    printf("\n\nWARNING: Do you want to erase the contents"
	   " of NAND Flash (destructive)? [Y/N] (Default: N) ");

    /* Read the user response */
    len = readline("");

    if (len > 0) {
	strcpy(response, console_buffer);
	if (response[0] == 'y' || response[0] == 'Y') {
    
	    /* NAND device number */
	    nand_dev = 0;

	    /* Get Flash size in number of blocks and block size */
	    numblk = usb_dev_desc[nand_dev].lba;
	    blksize = usb_dev_desc[nand_dev].blksz;

	    /*
	     * Write all '1's to buffer
	     */
	    write_buffer = (char*) malloc(blksize);
	    if (!write_buffer) {
		printf("\n Memory allocation failed for buffer!!!");
		return 0;
	    }
	    memset(write_buffer, '1', blksize);

	    /*
	     * Print size & number of blocks
	     */
	    printf("\n Clearing NAND Flash - Blocks: %ld  Size %d MB",
		   numblk, (numblk * blksize) >> 20);

	    /* Erase 100,000 LBAs at one time */
	    numblk_loop = numblk / 100000;
	    start = 0;
	    numblk_erase = 100000;

	    while (numblk_loop) {

		printf("\n Erasing LBA from %07ld to %07ld", start,
		       (start + numblk_erase));

		memset(write_buffer, '1', blksize);

		/*
		 * Do the actual write
		 */
		write_cnt = usb_stor_write(nand_dev, start, numblk_erase,
					   (ulong *)write_buffer);
		if (!write_cnt) {
		    printf("\n Write Failed at block 0x%x\n", start);
		}

		start = start + numblk_erase;
		numblk_loop--;
	    }

	    /* erase the last remaining blocks */
	    numblk_erase = numblk - start;
	    write_cnt = usb_stor_write(nand_dev, start, numblk_erase,
				       (ulong *)write_buffer);
	    if (!write_cnt) {
		printf("\n Write Failed at block 0x%x\n", start);
	    }
	    start = start + numblk_erase;

	    printf("\n *Erased %ld Blocks\n", start);

	    /* Indicate that we have cleared NAND Flash */
	    return 1;
	}
    }

    /* Indicate that we have not cleared NAND Flash */
    return 0;
}

int erase_nand(void)
{
    /*
     * Check & clear NAND contents
     * depending on user response
     */

    if (clear_nand_data()) {
	    /*
	     * Clear unattended boot mode, clear the bootloader password.
	     */
	    setenv("password", NULL);
	    setenv("boot_unattended", NULL);
	    saveenv (); 

	    return 1;
    }
    return 0;
}
