/*
 * Copyright (c) 2010-2012, Juniper Networks, Inc.
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

#include <mvCtrlEnvLib.h>
#include <common.h>
#include <mvTypes.h>
#include <mvBoardEnvLib.h>
#include <mvCpuIf.h>
#include <mvCtrlEnvLib.h>
#include <mv_mon_init.h>
#include <mvDebug.h>
#include <device/mvDevice.h>
#include <twsi/mvTwsi.h>
#include <eth/mvEth.h>
#include <pex/mvPex.h>
#include <eth-phy/mvEthPhy.h>
#include <gpp/mvGpp.h>
#include <sys/mvSysUsb.h>
#include "ex3300_local.h"

#if defined(MV_INCLUDE_IDMA)
#include <sys/mvSysIdma.h>
#include <idma/mvIdma.h>
extern MV_STATUS mvDmaInit (MV_VOID);
#endif

#if (CONFIG_COMMANDS & CFG_CMD_NAND)
#include <nand.h>
#include <gpp/mvGppRegs.h>
#endif

#if defined(MV_INCLUDE_USB)
#include <usb/mvUsb.h>
#endif

#include <cpu/mvCpu.h>

#ifdef CONFIG_PCI
#include <pci.h>
#endif
#include <pci/mvPciRegs.h>

#include <net.h>
#include <command.h>

#include <post.h>
#include "syspld.h"
#include <i2c.h>
#include <pcie.h>
#include "led.h"
#include "lcd.h"
#include <elf.h>
#include <usb.h>

DECLARE_GLOBAL_DATA_PTR;

#define DCACHE_FLUSH(addr, size)     mvOsCacheFlush(NULL, addr, size)

/* #define MV_DEBUG */
#ifdef MV_DEBUG
#define DB(x) x
#else
#define DB(x)
#endif

/* bootstrap.h */
typedef void (*pFunction)(void);

typedef struct BHR_t
{
    	MV_U8       	blockID;
    	MV_U8       	rsvd0;
    	MV_U16      	nandPageSize;
    	MV_U32      	blockSize;
    	MV_U32      	rsvd1;
    	MV_U32      	sourceAddr;
    	MV_U32      	destinationAddr;
    	pFunction   	executionAddr;
    	MV_U8       	sataPioMode;
    	MV_U8       	rsvd3;
    	MV_U16      	ddrInitDelay;
    	MV_U16     	rsvd2;
    	MV_U8       	ext;
    	MV_U8       	checkSum;
} BHR_t, * pBHR_t;


typedef struct ExtBHR_t
{
	MV_U32 		dramRegsOffs;
	MV_U32		rsrvd1;
	MV_U32 		rsrvd2;
	MV_U32 		rsrvd3;
	MV_U32		rsrvd4;
	MV_U32		rsrvd5;
	MV_U32		rsrvd6;
    	MV_U16      	rsrvd7;
    	MV_U8       	rsrvd8;
   	MV_U8       	checkSum;
}ExtBHR_t, *pExtBHR_t;

#define BOOTROM_SIZE			(12 * 1024)
#define HEADER_SIZE			512
#define BHR_HDR_SIZE        		0x20
#define EXT_HEADER_SIZE			(HEADER_SIZE - BHR_HDR_SIZE)

/* Boot Type - block ID */
#define IBR_HDR_I2C_ID			0x4D
#define IBR_HDR_SPI_ID    		0x5A
#define IBR_HDR_NAND_ID     		0x8B
#define IBR_HDR_SATA_ID     		0x78
#define IBR_HDR_PEX_ID			0x9C
#define IBR_HDR_UART_ID     		0x69
#define IBR_DEF_ATTRIB      		0x00

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
MV_CPU_DEC_WIN mvCpuAddrWinMonExtMap[] = MV_CPU_IF_ADDR_WIN_MON_EXT_MAP_TBL;

void mv_cpu_init(void);
int board_idtype(void);
int board_pci(void);
//MV_U32 switchClkGet(void);
MV_U32 mvTclkGet(void);
MV_U32 mvSysClkGet (void);

int32_t exflashInit (void);
int set_upgrade_state (upgrade_state_t state);
int get_upgrade_state (void);
ulong get_upgrade_address (void);
int set_upgrade_address (ulong addr);
int valid_env (unsigned long addr);
int valid_loader_image (unsigned long addr);
int valid_uboot_image (unsigned long addr);

#ifdef	CFG_FLASH_CFI_DRIVER
MV_VOID mvUpdateNorFlashBaseAddrBank(MV_VOID);
int mv_board_num_flash_banks;
extern flash_info_t	flash_info[]; /* info for FLASH chips */
extern unsigned long flash_add_base_addr (uint flash_index, 
    ulong flash_base_addr);
#endif	/* CFG_FLASH_CFI_DRIVER */

#if defined(MV_INCLUDE_UNM_ETH) || defined(MV_INCLUDE_GIG_ETH)
extern MV_VOID mvBoardEgigaPhySwitchInit(void);
#endif 

extern int post_result_cpu;
extern int post_result_mem;
extern int post_result_ether;
extern int post_result_pci;
extern int ledUpdateEna;

MV_VOID mvMppModuleTypePrint(MV_VOID);

/* Define for SDK 2.0 */
int 
raise (void) 
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
    MV_REG_WRITE(CPU_AHB_MBUS_MASK_INT_REG(whoAmI()), 0);
    MV_REG_WRITE(CPU_INT_MASK_ERROR_REG(whoAmI()), 0);
    MV_REG_WRITE(CPU_INT_MASK_LOW_REG(whoAmI()), 0);
    MV_REG_WRITE(CPU_INT_MASK_HIGH_REG(whoAmI()), 0);
}

/* init for the Master*/
void 
misc_init_r_dec_win (void)
{
//#if defined(CONFIG_NAND_DIRECT)
//    mvUsbInit(1, MV_TRUE);
//#else
    /* USB controller 0 is for internal USB interface */
    mvUsbInit(0, MV_TRUE);
    /* USB controller 1 is for external USB interface */
    mvUsbInit(1, MV_TRUE);
    /* 2 root hubs */
    usb_multi_root_hub = 1;
    usb_num_root_hub = 2;
//#endif

    return;
}


int 
cpuMapInit (void)
{
    return 0;
}

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

    /* Init the Board environment module (device bank params init) */
    mvBoardEnvInit();
    
    /* Init the Controller CPU interface */
    mvCpuIfInit(mvCpuAddrWinMap);

    /* Set up the cache in write through mode */
    MV_REG_BIT_SET(CPU_CTRL_STAT_REG(whoAmI()), CCSR_L2WT);

    /* reset i2c, syspld */
    MV_REG_WRITE(GPP_DATA_OUT_REG(0), 0x0);
    mvGppTypeSet(0, 0xFFFFFFFF, ~(EX3300_GPIO_OUT));
    /* clear 2c, syspld */
    MV_REG_WRITE(GPP_DATA_OUT_REG(0), 0x00C00000);
    mvGppTypeSet(0, 0xFFFFFFFF, ~(EX3300_GPIO_OUT));

    slave.type = ADDR7_BIT;
    slave.address = 0;
    mvTwsiInit(0, CFG_I2C_SPEED, CFG_TCLK, &slave, 0);
    mvTwsiInit(1, CFG_I2C_SPEED, CFG_TCLK, &slave, 0);

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

    led_init();
    /* booting */
    set_led(SYS_LED, LED_GREEN, LED_HW_BLINKING);

    lcd_off();

    /* syspld will move USB HUB out of reset automatically */
    /* PoE present does not work at this point.  Pull poe controller out of
     *  reset and poe port enable no matter P or T SKU.  It will not hurt
     *  T SKU.
     */
    syspld_reg_write(SYSPLD_RESET_REG_2, 
                     SYSPLD_POECTRL_RESET |
                     SYSPLD_MGMT_PHY_RESET |
                     SYSPLD_UPLINK_PHY_RESET |
                     SYSPLD_NAND_FLASH_RESET |
                     SYSPLD_SEEPROM_RESET);
    udelay(10000);
    /* mgmt PHY, NAND controller, SEEPROM, and PoE out of reset */
    syspld_reg_write(SYSPLD_RESET_REG_2, 
                     SYSPLD_UPLINK_PHY_RESET);

    /* external NAND power enable */
    syspld_reg_read(SYSPLD_MISC, &regVal);
    syspld_reg_write(SYSPLD_MISC, 
                     SYSPLD_USB_POWER_ENABLE | regVal);

    /* PoE ports to be disabled in U-Boot */
    syspld_reg_read(SYSPLD_MISC, &regVal);
    regVal |= SYSPLD_POE_DISABLE;
    syspld_reg_write(SYSPLD_MISC, regVal);
 
#if defined(MV_INCLUDE_UNM_ETH) || defined(MV_INCLUDE_GIG_ETH)
    /* Init the PHY or Switch of the board */
    mvBoardEgigaPhySwitchInit();
#endif /* #if defined(MV_INCLUDE_UNM_ETH) || defined(MV_INCLUDE_GIG_ETH) */
	
    return (0);
}

void 
misc_init_r_env (void)
{
    char *env;
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

#if !defined(CONFIG_EX3300)
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
        i2c_mux(0, I2C_MAIN_MUX_ADDR, 3, 1);
        i2c_read_norsta(0, I2C_MAIN_EEPROM_ADDR, 56, 1, enet, 6);
        i2c_read_norsta(0, I2C_MAIN_EEPROM_ADDR, 55, 1, &esize, 1);
        i2c_mux(0, I2C_MAIN_MUX_ADDR, 0, 0);
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
    if (memcmp(s, temp, sizeof(temp)) != 0) {
        setenv("ethaddr", temp);
    }
    
    s = getenv("bootcmd.prior");
    if (s) {
        sprintf(temp,"%s; %s", s, "bootelf CFG_READONLY_LOADER_ADDR");
        setenv("bootcmd", temp);
    }
    else
        setenv("bootcmd", "bootelf CFG_READONLY_LOADER_ADDR");

    return;
}

int 
board_late_init (void)
{
    uint8_t tdata, regVal_8, tdata_1[2];
    unsigned char temp[60];
    char *s;
    int mloop = 1;
    int loopback = 0;
    int ledoff = 0;
    uint32_t ether_fail = 0;
    uint16_t id;
#if defined(CONFIG_READONLY)
    ulong state = get_upgrade_state();
#endif
    int nand_test = 0;
    int test_count = 0;
    int nand_fail = 0;
    ulong nand_length;
    ulong nand_offset;

    /* temperature */
    i2c_mux(0, I2C_MAIN_MUX_ADDR, 0, 1);
    tdata = 0x7f;  /* beta compensation enable */
    i2c_write(0, I2C_MAX6581_ADDR, 0x49, 1, &tdata, 1);
    tdata = 0;  /* beta compensation disable */
    i2c_write(0, I2C_MAX6581_ADDR, 0x49, 1, &tdata, 1);
    tdata = 0x2;  /* reset, extend range (-64) to 191 */
    i2c_write(0, I2C_MAX6581_ADDR, 0x41, 1, &tdata, 1);

    /* volt/power */
    tdata_1[0] = 0x19;  /* 16V */
    tdata_1[1] = 0x9F;
    i2c_write(1, I2C_INA219_ADDR, 0x0, 1, tdata_1, 2);
    tdata_1[0] = 0x1c;  /* calibration */
    tdata_1[1] = 0x84;
    i2c_write(1, I2C_INA219_ADDR, 0x5, 1, tdata_1, 2);

    /* fan */
    syspld_reg_read(SYSPLD_MISC, &regVal_8);
    if (regVal_8 & SYSPLD_POE_PRESENT) {
        /* fan 2 with initial setting 25% duty cycle with PoE present */
        syspld_set_fan(2, 4);
        /* fan 3 with initial setting 25% duty cycle with PoE present */
        syspld_set_fan(3, 4);
    }
    else {
        /* fan 2 with initial setting 12.5% duty cycle */
        syspld_set_fan(2, 2); 
        /* fan 3 with initial setting 12.5% duty cycle */
        syspld_set_fan(3, 2);
    }
    /* RTC */
    i2c_read(0, I2C_RTC_ADDR, 2, 1, &tdata, 1);
    i2c_mux(0, I2C_MAIN_MUX_ADDR, 0, 0);

    if (tdata & 0x80) {  /* RTC voltage low */
        rtc_init();
        rtc_stop();
        rtc_start();
        rtc_set_time(0, 1, 1, 0, 0, 0);  /* default 2000/1/1 00:00:00 */
    }

#ifdef SYSPLD_WD_ISR
    /* enable syspld watchdog */
    syspld_reg_read(SYSPLD_WATCHDOG_ENABLE, &tdata);
    tdata |= SYSPLD_WDOG_ENABLE;
    syspld_reg_write(SYSPLD_WATCHDOG_ENABLE, tdata);
#endif

    /* environment variable for board type */
    id = board_idtype();
    if (id == 0xFFFF)
        id = I2C_ID_JUNIPER_EX3300_24_T;
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

    sprintf(temp, " %d.%d.%d",
	    CFG_VERSION_MAJOR, CFG_VERSION_MINOR, CFG_VERSION_PATCH);
    setenv("boot.intrver", temp);

    if ((s = getenv("boot.test")) != NULL) {
	  mloop = simple_strtoul(s, NULL, 16);
        {
            uint32_t test_delay = 0;

            s = getenv("boot.test.delay");
            if (s != NULL)
                test_delay = simple_strtoul(s, NULL, 10);
            if (test_delay)
                udelay(test_delay);
	  }
         if (memtest(0x400000, gd->ram_size - 4, mloop, 1)) {
             setenv("boot.ram", "FAILED ");
         }
         else {
             setenv("boot.ram", "PASSED ");
         }
    }

    if ((s = getenv("boot.ethtest")) != NULL) {
        loopback = simple_strtoul(s, NULL, 10);/* 0=mac+phy, 1=mac+phy+ext */
        ether_fail = diag_ether_loopback(loopback);
        if (ether_fail) {
            sprintf(temp, "FAILED %08x ", ether_fail);
            setenv("boot.etsec0", temp);
        }
        else {
            setenv("boot.etsec0", "PASSED ");
        }
    }
    
    if ((s = getenv("boot.ledoff")) != NULL) {
        ledoff = simple_strtoul(s, NULL, 10);
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
    return (0);
}

int 
misc_init_r (void)
{
    uint32_t regVal;
    uint32_t *pTemp;
    
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

    /* Cheetah temperature sensors */
    switch (board_idtype()) {
        case I2C_ID_JUNIPER_EX3300_48_T:
        case I2C_ID_JUNIPER_EX3300_48_P:
        case I2C_ID_JUNIPER_EX3300_48_T_BF:
            pTemp = 0xc4000048;
            *pTemp |= BIT21;
            pTemp = 0xcc000048;
            *pTemp |= BIT21;
            break;
        case I2C_ID_JUNIPER_EX3300_24_T:
        case I2C_ID_JUNIPER_EX3300_24_P:
        case I2C_ID_JUNIPER_EX3300_24_T_DC:
        default:
            pTemp = 0xc4000048;
            *pTemp |= BIT21;
            break;
    };

    mvSysClkGet();

    /* unmask timer1 interrupt */
    regVal = MV_REG_READ(CPU_AHB_MBUS_MASK_INT_REG(0));
    MV_REG_WRITE(CPU_AHB_MBUS_MASK_INT_REG(0), regVal|BIT2);
    regVal = MV_REG_READ(CPU_INT_MASK_LOW_REG(0));
    MV_REG_WRITE(CPU_INT_MASK_LOW_REG(0), regVal | BIT9);

    /* Print the juniper internal version of the u-boot */
    printf("\nFirmware Version: --- %02X.%02X.%02X ---\n",
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
    uint8_t regVal;
    
    syspld_reg_read(SYSPLD_MISC, &regVal);
    syspld_reg_write(SYSPLD_MISC, regVal & ~SYSPLD_USB_POWER_ENABLE);
    udelay(100000);

    syspld_reg_read(SYSPLD_RESET_REG_1, &regVal);
    syspld_reg_write(SYSPLD_RESET_REG_1, regVal | SYSPLD_SYS_RESET);
}

void 
mv_cpu_init (void)
{
	char *env;
	volatile unsigned int temp;

    /* Invalidate and Unlock CPU L1 I-cache which was locked in jump.s */
    temp = 0;
    __asm__ __volatile__("mcr    p15, 1, %0, c7, c5, 0" : "=r" (temp));
    __asm__ __volatile__("nop");
    __asm__ __volatile__("nop");
    __asm__ __volatile__("mcr    p15, 1, %0, c9, c0, 1" : "=r" (temp));
    __asm__ __volatile__("nop");
    __asm__ __volatile__("nop");


	/* WA for CPU L2 cache clock ratio limited to 1:3 erratum FE-CPU-180*/
	MV_REG_BIT_RESET(CPU_TIMING_ADJUST_REG(0), BIT28);
	MV_REG_BIT_RESET(CPU_TIMING_ADJUST_REG(1), BIT28);
	while((MV_REG_READ(CPU_TIMING_ADJUST_REG(0)) & BIT28) == BIT28);

	/* enable L2C - Set bit 22 */
    	env = getenv("disL2Cache");
    	if (!env || ((strcmp(env,"no") == 0) || (strcmp(env,"No") == 0)))
		temp |= BIT22;
	else
		temp &= ~BIT22;
	
	asm ("mcr p15, 1, %0, c15, c1, 0": :"r" (temp));


	/* Enable i cache */
	asm ("mrc p15, 0, %0, c1, c0, 0":"=r" (temp));
	temp |= BIT12;
	/* Change reset vector to address 0x0 */
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
    for (i = SPI_FLASH_SW_PROTECT_START; 
         i < SPI_FLASH_SW_PROTECT_END;
         i += SPI_FLASH_SW_PROTECT_INC) {
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
        size = flash_init();
        size += mvFlash_init();
    }
    
    return (size);
}

uint32_t 
getUsbBase (int dev)
{
    return (INTER_REGS_BASE | USB_REG_BASE(dev));
}

uint32_t 
getUsbHccrBase (int dev)
{
    return (getUsbBase(dev) | 0x100);
}

uint32_t 
getUsbHcorBase (int dev)
{
    return (getUsbBase(dev) | 0x140);
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
    return (1);
}

int 
check_upgrade (void)
{
    exflashInit();
    
#if defined(CONFIG_READONLY)
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
        set_upgrade_address(CFG_UPGRADE_BOOT_IMG_ADDR + CFG_CHKSUM_OFFSET);
        reset_cpu();
    }
#endif

#if defined(CONFIG_UPGRADE)
        set_upgrade_state(UPGRADE_READONLY);
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
    int i;

#if defined(CONFIG_SPI_SW_PROTECT)
    exSpiSwLock();
#endif

    /* kludge for I2C reset, neede for upgrade U-boot reading EEPROM */
    slave.type = ADDR7_BIT;
    slave.address = 0;
    mvTwsiInit(0, CFG_I2C_SPEED, CFG_TCLK, &slave, 0);

    for (i = 0; i < 6; i++)
        syspld_reg_read(i, &temp[i]);
    
    syspld_version = temp[0];
    syspld_reg_read(SYSPLD_LAST_RESET, &syspld_LastRst);
    gd->last_reset = syspld_LastRst;

    gd->valid_bid = 0;
    /* ID */
    if (i2c_mux(0, I2C_MAIN_MUX_ADDR, 3, 1) == 0) {
        if (i2c_read_norsta(0, I2C_MAIN_EEPROM_ADDR, 4, 1, (uint8_t *)&id, 4))
            printf("seeprom read failed\n");
        gd->valid_bid = 1;
    }
    i2c_mux(0, I2C_MAIN_MUX_ADDR, 0, 0);
    gd->board_type = swap_ulong(id);

    switch (board_idtype()) {
        case I2C_ID_JUNIPER_EX3300_24_T:
            puts("Board: EX3300-24T");
            break;
        case I2C_ID_JUNIPER_EX3300_24_P:
            puts("Board: EX3300-24P");
            break;
        case I2C_ID_JUNIPER_EX3300_48_T:
            puts("Board: EX3300-48T");
            break;
        case I2C_ID_JUNIPER_EX3300_48_P:
            puts("Board: EX3300-48P");
            break;
        case I2C_ID_JUNIPER_EX3300_24_T_DC:
            puts("Board: EX3300-24T-DC");
            break;
        case I2C_ID_JUNIPER_EX3300_48_T_BF:
            puts("Board: EX3300-48T-BF");
            break;
        default:
            puts("Board: Unknown");
            gd->valid_bid = 0;
            break;
    };
    printf(" %d.%d\n", version_major(), version_minor());
    printf("EPLD:  Version %02x.%02x%02x%02x (0x%02x)\n", 
           syspld_version, temp[5], temp[3], temp[2], 
           syspld_LastRst);
    syspld_reg_write(SYSPLD_LAST_RESET, 0);
    
    checkboard_done = 1;
    
    lcd_init();
    
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

    if (checkboard_done == 0)
        return;

#ifdef SYSPLD_WD_ISR
    regVal = MV_REG_READ(CPU_MAIN_INT_CAUSE_HIGH_REG);
    if (regVal & BIT3) { /* syspld interrupt */
        syspld_watchdog_petting();
        MV_REG_WRITE(CPU_MAIN_INT_CAUSE_HIGH_REG, regVal & (~BIT3));
    }
#endif

    regVal = MV_REG_READ(CPU_INT_LOW_REG(0));
    if ((regVal & BIT9) == 0)  /* not timer1 interrupt */
        return;
    regVal = MV_REG_READ(CPU_AHB_MBUS_CAUSE_INT_REG(0));
    if ((regVal & BIT2) == 0)  /* not timer interrupt */
        return;
    MV_REG_WRITE(CPU_AHB_MBUS_CAUSE_INT_REG(0), regVal & (~BIT2));

    /* Give up this chance.  Do not interrupt mgmt port operation */
    if (egiga_init)
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

    /* 500ms */
    if (tick_count % 5 == 0)  
        lcd_heartbeat();

    /* 100ms */
    if (ledUpdateEna)
        update_led();
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
valid_env (unsigned long addr)
{
    return (1);
}

int 
valid_loader_image (unsigned long addr)
{
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *) addr;

    if (!IS_ELF(*ehdr))
        return (0);
    return (1);
}

int valid_uboot_image (unsigned long addr)
{
    ulong icrc;
    image_header_t *haddr = (image_header_t *) (addr + CFG_IMG_HDR_OFFSET);

    if (swap_ulong(haddr->ih_size) > CFG_MONITOR_LEN)
        return 0;

    if ((icrc = crc32(0, (unsigned char *)(addr+CFG_CHKSUM_OFFSET),
                   swap_ulong(haddr->ih_size))) == 
                   swap_ulong(haddr->ih_dcrc)) {
        return 1;
    }

    return 0;
}

int
set_upgrade_state (upgrade_state_t state)
{
    ulong	upgrade_addr = CFG_FLASH_BASE + CFG_UPGRADE_BOOT_STATE_OFFSET;
    ulong temp[2];

    temp[0] = state;
    temp[1] = get_upgrade_address();

    return  flash_write_direct(upgrade_addr, (char *)temp, 2 * sizeof(ulong));
}

int 
get_upgrade_state (void)
{
    volatile upgrade_state_t *ustate = (upgrade_state_t *) 
        (CFG_FLASH_BASE + CFG_UPGRADE_BOOT_STATE_OFFSET);

    if ((*ustate < UPGRADE_READONLY) || (*ustate > UPGRADE_OK))
    {
        set_upgrade_state(UPGRADE_READONLY);
        return (UPGRADE_READONLY);
    }
    
    return (*ustate);
}

int
set_upgrade_address (ulong addr)
{
    ulong	upgrade_addr = CFG_FLASH_BASE + CFG_UPGRADE_BOOT_STATE_OFFSET;
    ulong temp[2];

    temp[0] = get_upgrade_state();
    temp[1] = addr;

    return  flash_write_direct(upgrade_addr, (char *)temp, 2 * sizeof(ulong));
}

ulong 
get_upgrade_address (void)
{
    volatile ulong *addr = (ulong *) (CFG_FLASH_BASE + 
        CFG_UPGRADE_BOOT_STATE_OFFSET);

    return (addr[1]);
}

int 
set_upgrade_ok (void)
{
#if defined(CONFIG_UPGRADE)
    unsigned char temp[5];
    ulong state = get_upgrade_state();

    if (state == UPGRADE_READONLY) {
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
 * Returns 1 if keys pressed to start the power-on long-running tests
 * Called from board_init_f().
 */
int 
post_hotkeys_pressed (void)
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

    /* Run Post test to validate the cpu,ram,pci and ethernet */
    for (i = 0; i < 4; i++) {
        if (post_run(post_test_name[i],
                      POST_RAM | POST_MANUAL | BOOT_POST_FLAG) != 0)
            debug(" Unable to execute the POST test %s\n", post_test_name[i]);
    }
           
    if (post_result_cpu == -1) {
        setenv("post.cpu", "FAILED ");
    }
    else {
        setenv("post.cpu", "PASSED ");
    }
    if (post_result_mem == -1) {
        setenv("post.ram", "FAILED ");
    }
    else {
        setenv("post.ram", "PASSED ");
    }
    if (post_result_pci == -1) {
        setenv("post.pci", "FAILED ");
    }
    else {
        setenv("post.pci", "PASSED ");
    }
    if (post_result_ether == -1) {
        setenv("post.ethernet", "FAILED ");
    }
    else {
        setenv("post.ethernet", "PASSED ");
    }
    if (gd->valid_bid == 0) {
        setenv("post.eeprom", "FAILED ");
    }
    else {
        setenv("post.eeprom", "PASSED ");
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

#ifdef CONFIG_SHOW_ACTIVITY
void 
show_activity (int arg)
{
}
#endif

