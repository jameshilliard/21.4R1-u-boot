/*
 * Copyright (c) 2008-2013, Juniper Networks, Inc.
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
#include "mvCtrlEnvLib.h"
#include "mvCpuIf.h"
#include "eth-phy/mvEthPhy.h"
#include "twsi/mvTwsi.h"
#include "mvCpu.h"
#include "mv_mon_init.h"
#include "mvDebug.h"
#include "sys/mvSysUsb.h"

#include <command.h> 
#if (CONFIG_COMMANDS & CFG_CMD_NAND)
#include <nand.h>
#endif

#include "gpp/mvGppRegs.h"

#if defined(MV_INCLUDE_USB)
#include "usb/mvUsb.h"
#endif

#define I2C_RST             (1 << 17)  
#define CPU_MAIN_IRQ_MASK   0x20204
#define CPU_MAIN_FIQ_MASK   0x20208
#define CPU_END_POIN_MASK   0x2020c
#define MPP_CONTROL_VAL1    0x01222222
#define MPP_CONTROL_VAL2    0x20000011
#define MPP_CONTROL_VAL3    0x00000002

#ifdef  CFG_FLASH_CFI_DRIVER
MV_VOID mvUpdateNorFlashBaseAddrBank(MV_VOID);
int mv_board_num_flash_banks;
extern flash_info_t flash_info[]; /* info for FLASH chips */
extern unsigned long flash_add_base_addr (uint flash_index, ulong flash_base_addr);
#endif  /* CFG_FLASH_CFI_DRIVER */

#define RPS_IMAGE_SZ             (0x40000) 
#define RPS_UPG_PARAM_MAX_STR_SZ (11)
#define RPS_VER_LEN              (4)

#define CONFIG_PRODUCTION
char image_buf[RPS_IMAGE_SZ];

DECLARE_GLOBAL_DATA_PTR;

/* CPU address decode table. */
MV_CPU_DEC_WIN mvCpuAddrWinMap[] = MV_CPU_IF_ADDR_WIN_MAP_TBL;

void mv_cpu_init(void);

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
maskAllInt (void)
{
        /* mask all external interrupt sources */
        MV_REG_WRITE(CPU_MAIN_IRQ_MASK, 0);
        MV_REG_WRITE(CPU_MAIN_FIQ_MASK, 0);
        MV_REG_WRITE(CPU_END_POIN_MASK, 0);
}

/* init for the Master*/
void
misc_init_r_dec_win (void)
{

    return;
}


/*
 * Miscellaneous platform dependent initialisations
 */

extern int interrupt_init (void);
extern int i2c_read(uchar chanNum, uchar chip, 
                    uint addr, int alen, uchar *buffer, int len);
extern int i2c_write(uchar chanNum, uchar chip, 
                     uint addr, int alen, uchar *buffer, int len);
MV_U32 mvSysClkGet(void);
int32_t exflashInit (void);
int 
board_init (void)
{
#if defined(MV_INCLUDE_TWSI)
	MV_TWSI_ADDR slave;
#endif
	unsigned int i;

	mvTclkGet();
	maskAllInt();
	/* must initialize the int in order for udelay to work */
	interrupt_init();

	/* 
	 * Init the Controller CPU interface 
	 */
	mvCpuIfInit(mvCpuAddrWinMap);

	/*
	 * Init the Controlloer environment module (MPP init) 
	 */
	MV_REG_WRITE(MPP_CONTROL_REG0, MPP_CONTROL_VAL1);
	MV_REG_WRITE(MPP_CONTROL_REG1, MPP_CONTROL_VAL2);
	MV_REG_WRITE(MPP_CONTROL_REG2, MPP_CONTROL_VAL3);
	/* 
	 * I2C Reset set up as o/p
	 */
	MV_REG_WRITE(GPP_DATA_OUT_EN_REG(0), (MV_REG_READ(GPP_DATA_OUT_EN_REG(0)) 
		& (~I2C_RST)));
	udelay(10000);
	/*
	 * I2C reset from GPIO
	 */
	MV_REG_WRITE(GPP_DATA_OUT_REG(0),(MV_REG_READ(GPP_DATA_OUT_REG(0)) | I2C_RST));    
	udelay(1000);
	MV_REG_WRITE(GPP_DATA_OUT_REG(0),(MV_REG_READ(GPP_DATA_OUT_REG(0)) & ~I2C_RST));    
	udelay(1000);
	MV_REG_WRITE(GPP_DATA_OUT_REG(0),(MV_REG_READ(GPP_DATA_OUT_REG(0)) | I2C_RST));    
	udelay(1000);
	slave.type = ADDR7_BIT;
	slave.address = CFG_I2C_CPU_SLAVE_ADDR;
	mvTwsiInit(0, CFG_I2C_SPEED, CFG_TCLK, &slave, 0);

        /* arch number of Integrator Board */
        gd->bd->bi_arch_number = 526;

        /* adress of boot parameters */
        gd->bd->bi_boot_params = 0x00000100;

	/* relocate the exception vectors */
	/* U-Boot is running from DRAM at this stage */
	for (i = 0; i < 0x100; i += 4)
	{
		*(unsigned int *)(0x0 + i) = *(unsigned int*)(TEXT_BASE + i);

	}
	/* 
	 * Update NOR flash base address bank for CFI driver
	 */
#ifdef  CFG_FLASH_CFI_DRIVER
	mvUpdateNorFlashBaseAddrBank();
#endif  
	/* i cache can work without MMU,
	   d cache can not work without MMU:
	 i cache already enable in start.S */
	
	
	mvBoardEgigaPhySwitchInit();
	return (0);
}

void
misc_init_r_env (void)
{
        char *env;
        char tmp_buf[10];
        unsigned int malloc_len;

        /* update the CASset env parameter */
        env = getenv("CASset");
        if (!env )
        {
#ifdef MV_MIN_CAL
                setenv("CASset","min");
#else
                setenv("CASset","max");
#endif
        }
        /* Monitor extension */
        setenv("enaMonExt","yes");

        env = getenv("enaFlashBuf");
        if (((strcmp(env,"no") == 0) || (strcmp(env,"No") == 0)))
                setenv("enaFlashBuf","no");
        else
                setenv("enaFlashBuf","yes");

	/* CPU streaming */
        env = getenv("enaCpuStream");
        if (!env || ((strcmp(env,"no") == 0) || (strcmp(env,"No") == 0)))
                setenv("enaCpuStream","no");
        else
                setenv("enaCpuStream","yes");


        /* Malloc length */
        env = getenv("MALLOC_len");
        malloc_len =  simple_strtoul(env, NULL, 10) << 20;
        if (malloc_len == 0){
                sprintf(tmp_buf,"%d",CFG_MALLOC_LEN>>20);
                setenv("MALLOC_len",tmp_buf);}
         
        /* primary network interface */
        env = getenv("ethprime");

	if (!env)
		setenv("ethprime",ENV_ETH_PRIME);

        if (!enaMonExt()){
		setenv("disaMvPnp","no");
	}

	/* Disable PNP config of Marvel memory controller devices. */
        env = getenv("disaMvPnp");
        if (!env)
                setenv("disaMvPnp","no");

	/* MAC addresses */
        env = getenv("ethaddr");
        if (!env)
                setenv("ethaddr",ETHADDR);
        
#if defined(MV_INCLUDE_PCI)
	env = getenv("pciMode");
	if (!env)
	{
                setenv("pciMode",ENV_PCI_MODE);
	}
	
	env = getenv("pciMode");
	if (env && ((strcmp(env,"device") == 0) || (strcmp(env,"Device") == 0)))
			MV_REG_WRITE(PCI_ARBITER_CTRL_REG(0),
			(MV_REG_READ(PCI_ARBITER_CTRL_REG(0) & BIT31)));

#endif /* (MV_INCLUDE_PCI) */

			setenv("bootcmd", CONFIG_BOOTCOMMAND);

        return;
}

#ifdef BOARD_LATE_INIT
int
board_late_init (void)
{
	char *s;
	int mloop = 1, mresult;
	bd_t *bd = gd->bd;
#if (CONFIG_COMMANDS & CFG_CMD_NAND)
	nand_info_t *nand;
	unsigned long size = 0x800, addr = 0x500000;
	int nresult;
	extern int nand_curr_device;
	extern nand_info_t nand_info[CFG_MAX_NAND_DEVICE];
	extern int cpld_nand_write(int enable);

	if (CFG_MAX_NAND_DEVICE && nand_info[0].size) {
	    nand_curr_device = 0;
	    nand = &nand_info[nand_curr_device];
    
	    nresult = nand_read(nand, 0, &size, (u_char *)addr);
	    gd->post_nand = (nresult == 0) ? 0 : -1;
	    cpld_nand_write(0);
	}
	else
	    gd->post_nand = -1;    
#endif

	if ((s = getenv("boot.test")) != NULL) {
		mloop = simple_strtoul (s, NULL, 16);
		if (mresult = memtest(0x400000, 
            bd->bi_dram[0].size+bd->bi_dram[1].size, mloop, 1)) {
			setenv ("boot.ram", "FAILED ");
		}
		else {
			setenv ("boot.ram", "PASSED ");
		}
	}
	else {
		mresult = memtest(0x400000, 0x500000, 1, 0);
	}
	gd->post_ram = (mresult == 0) ? 0 : -1;
	    
	return (0);
}
#endif

int 
misc_init_r (void)
{
	char *env;

	/* set as 926 */
	env = getenv("cpuName");
	if (!env)
			setenv("cpuName","926");
	/* init special env variables */
	misc_init_r_env();
	mv_cpu_init();

#if defined(MV_INCLUDE_MONT_EXT)
	if (enaMonExt()){
			mon_extension_after_relloc();
	}
#endif /* MV_INCLUDE_MONT_EXT */


	/* init the units decode windows */
	misc_init_r_dec_win();
    
	mvSysClkGet();
	
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


MV_U32
mvGetRtcSec (void)
{
        return (0);
}

int32_t
exflashInit (void)
{
	static int32_t size = 0;

	if (size == 0) {
		size = flash_init() + mvFlash_init();
	}
	return (size);
}

void 
ext_bdinfo (void)
{
	char name[15];
    
	mvCtrlModelRevNameGet(name);
	printf("Soc         = %s (DDR2)\n",  name);
	printf("CPU clock   = %d Mhz\n",  gd->cpu_clk/1000000);
	printf("bus clock   = %d Mhz\n",  gd->bus_clk/1000000);
	printf("Tclock      = %d Mhz\n",  gd->tclk/1000000);
	printf("D-Cache     =  16 kB\n");
	printf("I-Cache     =  16 kB\n");
	printf("memstart    = 00000000\n");
	printf("memsize     = %08x\n", gd->ram_size);
	printf("flashstart  = %08x\n", CFG_FLASH_BASE);
	printf("flashsize   = %08x\n", gd->flash_size);
}
 
void 
i2c_cpld_reset (void)
{
        MV_REG_BIT_RESET( GPP_DATA_OUT_REG(0) ,(1 << 0));
        MV_REG_BIT_RESET( GPP_DATA_OUT_EN_REG(0) ,(1 << 0));
        MV_REG_BIT_SET( GPP_DATA_OUT_REG(0) ,(1 << 0));
        MV_REG_BIT_SET( GPP_DATA_OUT_EN_REG(0) ,(1 << 0));
}

void 
i2c_reset (void)
{
	MV_TWSI_ADDR slave;
    
	slave.type = ADDR7_BIT;
	slave.address = CFG_I2C_CPU_SLAVE_ADDR;
	mvTwsiInit(0, CFG_I2C_SPEED, CFG_TCLK, &slave, 0);
}

void 
reset_cpu (void)
{
    
        MV_REG_BIT_SET( CPU_RSTOUTN_MASK_REG , BIT2);
        MV_REG_BIT_SET( CPU_SYS_SOFT_RST_REG , BIT0);
    
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
    /* Verify write allocate and streaming */
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
    /* enaDCPref = no  */    
    {
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
    /* enaICPref = no  */    
    {
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

#define CPU_TIMER_CONTROL			0x20300
#define WATCHDOG_ENABLE_BIT        BIT4
#define CPU_WATCHDOG_RELOAD			0x20320
#define CPU_WATCHDOG_TIMER			0x20324
void
watchdog_init (uint32_t ms)
{
    uint32_t temp = 0;

    temp = (mvTclkGet() / 1000) * ms;

    MV_REG_WRITE(CPU_WATCHDOG_RELOAD, temp);
    MV_REG_BIT_RESET( CPU_TIMER_CONTROL, WATCHDOG_ENABLE_BIT);
    MV_REG_BIT_RESET( CPU_AHB_MBUS_CAUSE_INT_REG, BIT3); /* no WD interrupt */
}

void 
watchdog_disable (void)
{
    MV_REG_BIT_RESET( CPU_RSTOUTN_MASK_REG, BIT1);
    MV_REG_BIT_RESET( CPU_TIMER_CONTROL, WATCHDOG_ENABLE_BIT);
}

void 
watchdog_enable (void)
{
    MV_REG_WRITE(CPU_WATCHDOG_TIMER, MV_REG_READ(CPU_WATCHDOG_RELOAD));
    MV_REG_BIT_SET( CPU_RSTOUTN_MASK_REG, BIT1);
    MV_REG_BIT_SET( CPU_TIMER_CONTROL, WATCHDOG_ENABLE_BIT);
}

void 
watchdog_reset (void)
{
    MV_REG_WRITE(CPU_WATCHDOG_TIMER, MV_REG_READ(CPU_WATCHDOG_RELOAD));
}

#if (CONFIG_COMMANDS & CFG_CMD_NAND)
#define	MASK_CLE	0x01
#define	MASK_ALE	0x02

extern int cpld_nand_write(int enable);

static void 
rps_hwcontrol (struct mtd_info *mtd, int cmd)
{
	struct nand_chip *this = mtd->priv;
	ulong IO_ADDR_W = (ulong) this->IO_ADDR_W;
	/* TODO add auto detect for nand width */
	MV_U32 shift = 0;
	
	if (shift == MV_ERROR)
	{
		printf("No NAND detection\n");
		shift = 0;
	}

	IO_ADDR_W &= ~((MASK_ALE|MASK_CLE) << shift);
	switch (cmd) {
		case NAND_CTL_SETCLE: IO_ADDR_W |= (MASK_CLE << shift); break;
		case NAND_CTL_SETALE: IO_ADDR_W |= (MASK_ALE << shift); break;
	}
	this->IO_ADDR_W = (void *) IO_ADDR_W;
}

static int 
rps_dev_ready (struct mtd_info *mtdinfo)
{
	return (MV_REG_READ(GPP_DATA_IN_REG(0)) & (1 << 2) ? 1 : 0);
}

#endif

/* i2c slave read/write */

MV_BOOL 
twsiSlaveTimeoutChk (MV_U32 timeout, MV_U32 timeoutLimit, MV_U8 *pString)
{
	if (timeout >= timeoutLimit)
	{
		mvOsPrintf("%s",pString);
		return (MV_TRUE);
	}
	return (MV_FALSE);
	
}

MV_STATUS 
twsiSlaveDataReceive (MV_U8 *pBlock, MV_U32 blockSize, MV_U32 *pSize)
{
	MV_U32 timeout, temp, blockSizeRd = blockSize, count = 0;
	if (NULL == pBlock)
		return (MV_BAD_PARAM);

	*pSize = 0;

	/* wait for Int to be Set */
	timeout = 0;
	while (!twsiMainIntGet(0) && (timeout++ < TWSI_TIMEOUT_VALUE)) {
		udelay(10);
	}    

	/* check for timeout */
	if (MV_TRUE == twsiTimeoutChk(timeout,
        "TWSI: twsiSlaveDataReceive ERROR - Read Data int Time out .\n"))
		return (MV_TIMEOUT);

	while(1)
	{
		if (blockSizeRd == 0)
			return  (MV_NO_RESOURCE);
        
		twsiIntFlgClr(0);
		/* wait for Int to be Set */
		timeout = 0;

		while (!twsiMainIntGet(0) && (timeout++ < TWSI_TIMEOUT_VALUE)) {
			udelay(10);
		}    
		/* check for timeout */
		if (MV_TRUE == twsiTimeoutChk(timeout,
            "TWSI: twsiSlaveDataReceive ERROR - Read Data Int Time out .\n"))
			return (MV_TIMEOUT);

		/* check the status */
		temp = twsiStsGet(0);
		if (temp == TWSI_SLA_REC_WR_DATA_AF_REC_SLA_AD_ACK_TRAN)
		{
			/* read the data*/
			*pBlock = (MV_U8)MV_REG_READ(TWSI_DATA_REG(0));
			/*
			mvOsPrintf("TWSI: twsiSlaveDataReceive  place %d read %x \n",\
						blockSize - blockSizeRd,*pBlock);
			*/
			pBlock++;
			blockSizeRd--;
			count++;
		}
		if (temp == TWSI_SLA_REC_STOP_OR_REPEATED_STRT_CON)
		{
			*pSize = count;
			mvOsPrintf("TWSI: twsiDataReceive ERROR"
                       " - status %x in Rd Terminate\n",temp);
			return (MV_OK);
		}
		
	}

	*pSize = count;
	return (MV_OK);
}
MV_STATUS 
twsiSlaveDataTransmit (MV_U8 *pBlock, MV_U32 blockSize)
{
	MV_U32 timeout, temp, blockSizeWr = blockSize;

	if (NULL == pBlock)
		return (MV_BAD_PARAM);

	/* wait for Int to be Set */
	timeout = 0;
	while( !twsiMainIntGet(0) && (timeout++ < TWSI_TIMEOUT_VALUE)) {
		udelay(10);
	}    

	/* check for timeout */
	if (MV_TRUE == twsiTimeoutChk(timeout,"TWSI: twsiSlaveDataTransmit ERROR"
        " - Read Data Int TimeOut.\n"))
		return (MV_TIMEOUT);

	while(blockSizeWr)
	{
		/* write the data*/
		MV_REG_WRITE(TWSI_DATA_REG(0),(MV_U32)*pBlock);
		/*
		mvOsPrintf("TWSI: twsiSlaveDataTransmit place = %d write %x \n",\
						blockSize - blockSizeWr, *pBlock);
		*/
		pBlock++;
		blockSizeWr--;

		twsiIntFlgClr(0);

		/* wait for Int to be Set */
		timeout = 0;
		while( !twsiMainIntGet(0) && (timeout++ < TWSI_TIMEOUT_VALUE)) {
			udelay(10);
		}    

		/* check for timeout */
		if (MV_TRUE == twsiTimeoutChk(timeout,
            "TWSI: twsiSlaveDataTransmit ERROR - Read Data Int TimeOut.\n"))
			return (MV_TIMEOUT);

		/* check the status */
		temp = twsiStsGet(0);
		if (temp == TWSI_SLA_TRA_RD_DATA_ACK_REC)
		{
			continue;
		}
		if (temp == TWSI_SLA_TRA_RD_DATA_ACK_NOT_REC)
		{
			return (MV_OK);
		}
		else
		{
			mvOsPrintf("TWSI: twsiSlaveDataTransmit ERROR"
                       " - status %x in write trans\n",temp);
			return (MV_FAIL);
		}
		
	}

	return (MV_OK);
}

MV_STATUS 
twsiSlaveWrite (MV_U8 *pBlock, MV_U32 blockSize, MV_U32 *pSize, MV_U32 iwait)
{
	MV_U32 timeout = 0;
	MV_U32 temp;
    
	if (NULL == pBlock)
		return (MV_BAD_PARAM);

	while (timeout++ < iwait)
	{
		/* check the status */
		temp = twsiStsGet(0);
		if (twsiMainIntGet(0) && 
            (temp == TWSI_SLA_REC_AD_PLS_WR_BIT_ACK_TRA))
		{
			break;
		}
		udelay(1000);
	}
	/* check for timeout */
	if (MV_TRUE == twsiSlaveTimeoutChk(timeout, iwait, 
             "TWSI: twsiSlaveWrite ERROR - Slave Read Data int Time out .\n"))
		return (MV_TIMEOUT);

	if (MV_OK != twsiSlaveDataReceive(pBlock, blockSize, pSize)) 
	{
		return (MV_FAIL);
	}

	return (MV_OK);
}

MV_STATUS 
twsiSlaveRead (MV_U8 *pBlock, MV_U32 blockSize, MV_U32 iwait)
{
	MV_U32 timeout = 0;
	MV_U32 temp;
    
	if (NULL == pBlock)
		return (MV_BAD_PARAM);

	while (timeout++ < iwait)
	{
		/* check the status */
		temp = twsiStsGet(0);
		if (twsiMainIntGet(0) && 
            (temp == TWSI_SLA_REC_AD_PLS_RD_BIT_ACK_TRA))
		{
			break;
		}
		udelay(1000);
	}
    
	/* check for timeout */
	if (MV_TRUE == twsiSlaveTimeoutChk(timeout, TWSI_TIMEOUT_VALUE, 
              "TWSI: twsiSlaveRead ERROR - Slave Read Data int Time out .\n"))
		return (MV_TIMEOUT);

	if (MV_OK != twsiSlaveDataTransmit(pBlock, blockSize)) 
	{
		return (MV_FAIL);
	}

	return (MV_OK);
}

MV_STATUS 
i2c_slave_write_wait (MV_U8 slave, MV_U8 *pBlock, 
                      MV_U32 blockSize, MV_U32 *pSize, MV_U32 iwait)
{
	return  (twsiSlaveWrite(pBlock, blockSize, pSize, iwait));
}

MV_STATUS 
i2c_slave_read_wait (MV_U8 slave, MV_U8 *pBlock, 
                     MV_U32 blockSize, MV_U32 iwait)
{
	return  (twsiSlaveRead(pBlock, blockSize, iwait));
}

int
board_id (void)
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

uint16_t 
version_epld (void)
{
       uint8_t ver = 0;

	i2c_read(0, CFG_I2C_SYSPLD_ADDR, 0x30, 1, &ver, 1);
       return (ver);
}


int 
check_upgrade (void)
{
	return (0);
}

int 
checkboard (void)
{
       unsigned char ch = 0x0;
       uint32_t id = 0;
       uint8_t tmp[6];
       uint8_t temp;
       uint8_t cpld_version = 0x0;
       extern int cpld_reg_write(int reg, uint8_t val);
       extern int cpld_reg_read(int reg, uint8_t *val);

       /* EPLD Vcc20v_en */
       cpld_reg_read(0x0E, &temp);
       temp |= 0x4;
       cpld_reg_write(0x0E, temp);

       /* EPLD post */
       cpld_reg_read(0x31, &temp);
       temp = 0x55;
       cpld_reg_write(0x31, temp);
       cpld_reg_read(0x31, &temp);
       gd->post_cpld = (temp == 0x55) ? 0 : -1;
       temp = 0x0;
       cpld_reg_write(0x31, temp);
    
       ch = 1 << 6;
	i2c_write(0, CFG_I2C_PCA9548_ADDR, 0, 0, &ch, 1);
       /* ID */
	i2c_read(0, CFG_I2C_ID_EEPROM_ADDR, 4, 1, (uint8_t *)&id, 4);
       ch = 0;
	i2c_write(0, CFG_I2C_PCA9548_ADDR, 0, 0, &ch, 1);
	gd->board_type = swap_ulong(id);

       /* board info */
       cpld_read(0x30, tmp, 6);
       cpld_version = tmp[0];

       printf("Board: EX-RPS-PWR %d.%d\n", 
              version_major(), version_minor());
       printf("EPLD:  Version %02x.%02x%02x%02x\n", 
           cpld_version, tmp[5], tmp[3], tmp[2]); 
    
       /* PoE+ PS??? */

	return (0);
}

void 
rps_reset (void)
{
	do_reset (NULL, 0, 0, NULL);
}

int 
post_stop_prompt (void)
{
    char *s;
    int mem_result;

    if (gd->post_ram == -1) {
         setenv ("post.ram", "FAILED ");
         printf("POST: RAM FAILED\n");
    }
        
    if (gd->post_cpld == -1) {
         setenv ("post.cpld", "FAILED ");
         printf("POST: CPLD FAILED\n");
    }
        
    if (gd->post_nand == -1) {
         setenv ("post.nand", "FAILED ");
         printf("POST: NAND FAILED\n");
    }
        
    if (getenv("boot.nostop")) return (0);

    if (((s = getenv("boot.ram")) != NULL) &&
         ((strcmp(s,"PASSED") == 0) ||
         (strcmp(s,"FAILED") == 0))) {
         mem_result = 0;
    }
    else {
         mem_result = gd->post_ram;
    }

    if ( gd->post_cpld == -1 || mem_result == -1)
        return (1);
    return (0);
}

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

void
upgrade_rps_image (void) {
    cmd_tbl_t *tmpcmdtp;
    char srcAddr[RPS_UPG_PARAM_MAX_STR_SZ];
    char destAddr[RPS_UPG_PARAM_MAX_STR_SZ];
    char imgSize[RPS_UPG_PARAM_MAX_STR_SZ];
    char pgmSize[RPS_UPG_PARAM_MAX_STR_SZ];
    char *image;
    char ver[RPS_VER_LEN];

    sprintf(srcAddr, "0x%x", image_buf);
    sprintf(destAddr, "0x%x", RPS_APP_START);
    sprintf(imgSize, "+0x%x", RPS_IMAGE_SZ);
    sprintf(pgmSize, "0x%x", RPS_IMAGE_SZ);

    /* 
     * tftp retry count
     */
    setenv("netretry", "once");
    image = getenv("upg_img");
    if (image == NULL) {
       setenv("upg_stat", "fail");
       saveenv();
       return;
    }    
    watchdog_disable();

    char* argv[] = {"tftp", srcAddr, image};
    char* argv1[] = {"protect", "off", destAddr, imgSize};
    char* argv2[] = {"erase", destAddr, imgSize};
    char* argv3[] = {"cp.b", srcAddr, destAddr, pgmSize};

    watchdog_disable();
    tmpcmdtp = find_cmd("tftp");
    if (do_tftpb (tmpcmdtp, 0, 3, argv)) {
        /* 
         * tftp failed 
         */
       setenv("upg_stat", "fail-tftp");
       watchdog_enable();
       saveenv();
       return;
    }    
    tmpcmdtp = find_cmd("protect");
    if (do_protect(tmpcmdtp, 0, 4, argv1)) {
       setenv("upg_stat", "fail-flash");
       watchdog_enable();
       saveenv();
       return;
    }    
    tmpcmdtp = find_cmd("erase");
    if (do_flerase(tmpcmdtp, 0, 3, argv2)) {
       setenv("upg_stat", "fail-flash");
       watchdog_enable();
       saveenv();
       return;
    }    
    tmpcmdtp = find_cmd("cp");
    if (do_mem_cp(tmpcmdtp, 0, 4, argv3)) {
       setenv("upg_stat", "fail-flash");
       watchdog_enable();
       saveenv();
       return;
    }    
    /* 
     * The curent version now becomes the prev version
     */
    strncpy(ver, getenv("cur_version"), RPS_VER_LEN);
    setenv("prev_version", ver);
    setenv("upg_stat", "pass");
    saveenv();
    /* 1 sec delay before reset */
    udelay(1000000);
    do_reset();
}

#ifdef  CFG_FLASH_CFI_DRIVER
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
#endif  /* CFG_FLASH_CFI_DRIVER */

