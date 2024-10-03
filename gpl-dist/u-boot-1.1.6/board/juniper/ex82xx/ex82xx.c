/*
 * Copyright (c) 2008-2011, Juniper Networks, Inc.
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
#include <config.h>
#include <i2c.h>
#include "fsl_i2c.h"
#include <pci.h>
#include <asm/processor.h>
#include <asm/immap_85xx.h>
#include <spd.h>
#include <usb.h>
#include "lc_cpld.h"
#include "ex82xx_devices.h"
#include "ex82xx_common.h"
#include <ex82xx_i2c.h>
#include <pcie.h>
#include "ccb_iic.h"
#include "gvcb.h"
#include <net.h>
#include <brgphyreg.h>
#include <led.h>
#include <asm/io.h>

DECLARE_GLOBAL_DATA_PTR;
#define MAC_ADDR_BYTES    6

typedef enum flash_banks {
	FLASH_BANK0 = 0,
	FLASH_BANK1
} flash_banks_t;

typedef enum upgrade_state {
	INITIAL_STATE_B0 = 0,   /*Set by Bank 0 u-boot*/
	INITIAL_STATE_B1,       /*Set by Bank 1 u-boot*/
	UPGRADE_START_B0,       /*Set by JunOS*/
	UPGRADE_START_B1,       /*Set by JunOS*/
	UPGRADE_CHECK,
	UPGRADE_TRY_B0,         /*Set by Bank 1 u-boot*/
	UPGRADE_TRY_B1,         /*Set by Bank 0 u-boot*/
	UPGRADE_OK_B0,          /*Set by Bank 0 u-boot*/
	UPGRADE_OK_B1,          /*Set by Bank 1 u-boot*/
	UPGRADE_STATE_END
} upgrade_state_t;


int usb_scan_dev = 0;
unsigned int lc_2xs_44ge_48p = 0;
extern void set_current_addr(void);
#if defined (CONFIG_DDR_ECC)
extern void ddr_enable_ecc(unsigned int dram_size);
#endif
extern long int spd_sdram(void);
extern gvcb_fpga_regs_t *ccb_get_regp(void);
extern void (*board_usb_err_recover)(void);

unsigned char get_master_re(void);
void assign_ip_mac_addr(void);
void set_caffeine_default_environment(void);
int bootcpld_init(void);
int get_assembly_id_as_string(unsigned char id_eeprom_addr,
							  unsigned char* str_assembly_id);
u_int16_t get_assembly_id(unsigned char id_eeprom_addr);
void local_bus_init(void);
unsigned char get_slot_id(void);
void keep_alive_master(void);
int get_cpu_type(void);
uint eeprom_valid(uchar i2c_addr);
int find_cpu_type(void);
void (*ex82xx_cpu_reset)(void) = NULL;
void ex82xx_btcpld_cpu_reset(void);
void init_ex82xx_platform_funcs(void);
int set_re_state(uint8_t);
int get_bp_eeprom_access(void);
int get_ore_state(uint8_t*);
int get_upgrade_state(void);
int check_upgrade(void);
int set_upgrade_state(upgrade_state_t state);
int valid_env(unsigned long addr);
int valid_uboot_image(unsigned long addr);
flash_banks_t get_booted_bank(void);
int set_next_bootbank(flash_banks_t bank);
int boot_next_bank(void);
int enable_bank0(void);
int disable_bank0(void);
int set_upgrade_ok(void);
void ex82xx_usb_access_test(void);
void btcpld_swizzle_disable(void);
int disable_bank0_atprompt(void);
void ex82xx_usb_err_recover(void);

uint8_t ex82xx_lc_mode = 0;
int autoboot_abort;
u_int16_t caff_lctype;
u_int32_t ram_init_done = 0;
unsigned char g_enable_scb_keep_master_alive = 0;
#ifdef CONFIG_POST
ulong post_word;
#endif
struct tsec_private *mgmt_priv = NULL;
extern char version_string[];

#define swap_ulong(x)						\
	({ unsigned long x_ = (unsigned long)x;	\
	   (unsigned long)(						\
		   ((x_ & 0x000000FFUL) << 24) |	\
		   ((x_ & 0x0000FF00UL) <<  8) |	\
		   ((x_ & 0x00FF0000UL) >>  8) |	\
		   ((x_ & 0xFF000000UL) >> 24) );	\
	 })


int
board_early_init_f(void)
{
	return 0;
}

int
checkboard(void)
{
	volatile immap_t *immap = (immap_t *)CFG_CCSRBAR;
	volatile ccsr_gur_t *gur = &immap->im_gur;
	u_int16_t assembly_id;

#ifdef DEBUG
	uint pci_clk_sel = gur->porpllsr & 0x8000;  /* PORPLLSR[16] */
#endif

	i2c_controller(CFG_I2C_CTRL_1);    /*select the I2C controller 1*/
	if (EX82XX_RECPU) {
		assembly_id = get_assembly_id(EX82XX_LOCAL_ID_EEPROM_ADDR);
		switch (assembly_id) {
		case EX82XX_GRANDE_CHASSIS:
			puts("Board: EX8208-SCB\n");
			break;
		case EX82XX_VENTI_CHASSIS:
			puts("Board: EX8216-SCB\n");
			break;
		default:
			puts("Board: Unknown\n");
		}
	} else if (EX82XX_LCPU) {
		assembly_id = get_assembly_id(EX82XX_LC_ID_EEPROM_ADDR);
		switch (assembly_id) {
		case EX82XX_LC_48T_BRD_ID:
		case EX82XX_LC_48T_NM_BRD_ID:
			puts("Board: EX8200-48T\n");
			break;
		case EX82XX_LC_48T_ES_BRD_ID:
			puts("Board: EX8200-48T-ES\n");
			break;
		case EX82XX_LC_48F_BRD_ID:
		case EX82XX_LC_48F_NM_BRD_ID:
			puts("Board: EX8200-48F\n");
			break;
		case EX82XX_LC_48F_ES_BRD_ID:
			puts("Board: EX8200-48F-ES\n");
			break;
		case EX82XX_LC_8XS_BRD_ID:
		case EX82XX_LC_8XS_NM_BRD_ID:
			puts("Board: EX8200-8XS\n");
			break;
		case EX82XX_LC_8XS_ES_BRD_ID:
			puts("Board: EX8200-8XS-ES\n");
			break;
		case EX82XX_LC_36XS_BRD_ID:
			puts("Board: EX8200-36XS\n");
			break;
		case EX82XX_LC_40XS_BRD_ID:
		case EX82XX_LC_40XS_NM_BRD_ID:
			puts("Board: EX8200-40XS\n");
			break;
		case EX82XX_LC_48P_BRD_ID:
			puts("Board: EX8200-48P\n");
			break;
		case EX82XX_LC_48TE_BRD_ID:
			puts("Board: EX8200-48TE\n");
			break;
		case EX82XX_LC_2X4F40TE_BRD_ID:
			puts("Board: EX8200-2X4F40TE\n");
			break;
		case EX82XX_LC_2XS_44GE_BRD_ID:
			puts("Board: EX8200-2XS-44GE\n");
			break;
		case EX82XX_LC_40XS_ES_BRD_ID:
			puts("Board: EX8200-40XS-ES\n");
			break;
		default:
			puts("Board: Unknown\n");
		}
	}

	debug("    PCI: %d bit, %s MHz, %s\n",
		  32, "33",
		  pci_clk_sel ? "sync" : "async");

	/*
	 * Initialize local bus.
	 */
	local_bus_init();

	return 0;
}

/*
 * Check whether the board is 2XS_44GE/48P or not.
 */
int
is_2xs_44ge_48p_board(void)
{
    if (EX82XX_LCPU) {
	u_int16_t assembly_id;

	assembly_id = get_assembly_id(EX82XX_LC_ID_EEPROM_ADDR);
	if ((assembly_id == EX82XX_LC_2XS_44GE_BRD_ID) ||
	   (assembly_id == EX82XX_LC_48P_BRD_ID)       ||
	   (assembly_id == EX82XX_LC_48TE_BRD_ID)      ||
	   (assembly_id == EX82XX_LC_2X4F40TE_BRD_ID)) {
		return TRUE;
	} else {
		return FALSE;
	}
    } else {
	return FALSE;
    }
}

long int
initdram(int board_type)
{
	long dram_size = 0;

	puts("Initializing\n");

#if defined (CONFIG_DDR_DLL)
	{
		volatile immap_t *immap = (immap_t *)CFG_IMMR;
		/*
		 * Work around to stabilize DDR DLL MSYNC_IN.
		 * Errata DDR9 seems to have been fixed.
		 * This is now the workaround for Errata DDR11:
		 *    Override DLL = 1, Course Adj = 1, Tap Select = 0
		 */

		volatile ccsr_gur_t *gur = &immap->im_gur;

		gur->ddrdllcr = 0x81000000;
		asm ("sync;isync;msync");
		udelay(200);
	}
#endif

	dram_size = spd_sdram();

#if defined (CONFIG_DDR_ECC) && !defined (CONFIG_ECC_INIT_VIA_DDRCONTROLLER)
	/*
	 * Initialize and enable DDR ECC.
	 */
	ddr_enable_ecc(dram_size);
#endif

	puts("    DDR: ");
	return dram_size;
}

/*
 * Initialize Local Bus
 */
void
local_bus_init(void)
{
	volatile immap_t *immap = (immap_t *)CFG_IMMR;

	volatile ccsr_lbc_t *lbc = &immap->im_lbc;

	uint clkdiv;
	uint lbc_hz;
	sys_info_t sysinfo;

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
		lbc->lcrr |= 0x80000000;        /* DLL Bypass */
	} else if (lbc_hz >= 133) {
		lbc->lcrr &= (~0x80000000);     /* DLL Enabled */
	} else {
		lbc->lcrr &= (~0x8000000);      /* DLL Enabled */
		udelay(200);
	}
}

#if defined (CFG_DRAM_TEST)
int
testdram(void)
{
	uint *pstart = (uint *)CFG_MEMTEST_START;
	uint *pend = (uint *)CFG_MEMTEST_END;
	uint *p;

	printf("Testing DRAM from 0x%08x to 0x%08x\n",
		   CFG_MEMTEST_START,
		   CFG_MEMTEST_END);

	printf("DRAM test phase 1:\n");
	for (p = pstart; p < pend; p++) {
		*p = 0xaaaaaaaa;
	}

	for (p = pstart; p < pend; p++) {
		if (*p != 0xaaaaaaaa) {
			printf("DRAM test fails at: %08x\n", (uint)p);
			return 1;
		}
	}

	printf("DRAM test phase 2:\n");
	for (p = pstart; p < pend; p++) {
		*p = 0x55555555;
	}

	for (p = pstart; p < pend; p++) {
		if (*p != 0x55555555) {
			printf("DRAM test fails at: %08x\n", (uint)p);
			return 1;
		}
	}

	printf("DRAM test passed.\n");
	return 0;
}

#endif

void
pci_init_board(void)
{
#ifdef CONFIG_PCI
	pci_mpc85xx_init(&hose);
#endif
}

int
misc_init_r(void)
{
	unsigned char btcpld, ctrlcpld, exp_cpld, dcap_fpga;
	ulong gvcbc_rev_min, gvcbc_rev_maj, revision;
	static gvcb_fpga_regs_t* p_ccb_regs = 0;
	uint8_t data;	

	/* Default 1st I2C controller */
	i2c_controller(CFG_I2C_CTRL_1);

	assign_ip_mac_addr();

	set_caffeine_default_environment();

	printf("\nFirmware Version: --- %02X.%02X.%02X ---\n",CFG_VERSION_MAJOR,    \
			CFG_VERSION_MINOR,    \
			CFG_VERSION_PATCH);

	printf("CPLD/FPGA image versions on the board:\n\t");
	if (EX82XX_RECPU) {
		p_ccb_regs = (gvcb_fpga_regs_t*)ccb_get_regp();
		revision = swap_ulong(p_ccb_regs->gvcb_fpga_rev);
		gvcbc_rev_min = (revision & 0xff);
		gvcbc_rev_maj = (revision >> 8) & 0xff;
		printf("GCBC FPGA = %02X.%02X |", gvcbc_rev_maj, gvcbc_rev_min);
	}
	btcpld = *(volatile unsigned char *)0xff000000;
	printf(" BTCPLD = %02X ", btcpld);
	if (EX82XX_LCPU) {
		if (is_2xs_44ge_48p_board()) {
			/* doublecap linecards */
			dcap_fpga = *(volatile unsigned char *)
				(CFG_CPLD_BASE + LC_CPLD_REVISION);
			printf(" DCAP FPGA = %02X ", dcap_fpga);
		} else {
			/* legacy caffeine linecards */
			ctrlcpld = *(volatile unsigned char *)
				(CFG_CTRL_CPLD_BASE + LC_CTRL_CPLD_VERSION);
			printf("| CTRLCPLD = %02X ", ctrlcpld);
			switch (caff_lctype) {
				case EX82XX_LC_48F_BRD_ID:
				case EX82XX_LC_48F_NM_BRD_ID:
                                case EX82XX_LC_48F_ES_BRD_ID:
					exp_cpld = *(volatile unsigned char *)
						(CFG_XCTL_CPLD_BASE + LC_XCTL_CPLD_VERSION);
					printf("| EXP-CPLD = %02X ", exp_cpld);
					break;
			}
		}		
	}
	printf("\n");
	if (!valid_env(CFG_FLASH_BASE)) {
		saveenv();
	}

	/* enable the upgrade bit (0x04) in the btcpld BOOT_CTLSTAT
	 * register to access the entire flash.
	 */
	lc_cpld_reg_read(LC_CPLD_BOOT_CTLSTAT, &data);
	lc_cpld_reg_write(LC_CPLD_BOOT_CTLSTAT, data | 0x04);

	return 0;
}

int
last_stage_init(void)
{
	struct eth_device *dev;

	dev = eth_get_dev_by_name(CONFIG_MPC85XX_TSEC1_NAME);
	if (dev) {
		mgmt_priv = (struct tsec_private*) dev->priv;
	}
	return 0;
}

void
set_caffeine_default_environment(void)
{
	char str_cpu_assembly_id[16];
	char str_brd_assembly_id[16];
	char str_cpu_idaddr[16];
	char str_brd_idaddr[16];
	u_int16_t assembly_id, caff_brd_asm_id;
    unsigned char temp[120];
    char *s;
	u_int8_t slot_id;

	/* needed for accessing the ID eeproms to get assembly information*/
	caff_lctype = 0xffff;

	i2c_controller(CFG_I2C_CTRL_1);

	if (EX82XX_RECPU) {
		get_assembly_id_as_string(EX82XX_LOCAL_ID_EEPROM_ADDR,
								  str_cpu_assembly_id);

		sprintf(str_cpu_idaddr, "0x%02X", EX82XX_LOCAL_ID_EEPROM_ADDR);
		sprintf(str_brd_idaddr, "0x%02X", EX82XX_LC_ID_EEPROM_ADDR);
		sprintf(str_brd_assembly_id, "0x%02x%02x",
				((EX82XX_GSCB_ASSEMBLY_ID & 0xFF00) >> 8),
				((EX82XX_GSCB_ASSEMBLY_ID & 0xFF)));
		setenv("hw.board.type", str_brd_assembly_id);
		setenv("hw.board.idaddr", str_brd_idaddr);
		setenv("hw.smb.id", str_cpu_assembly_id);
		setenv("hw.smb.idaddr", str_cpu_idaddr);
		if (strcmp(str_cpu_assembly_id, "0x051F") == 0) {     
			caff_brd_asm_id = EX82XX_VCB_ASSEMBLY_ID;
			setenv("hw.boardtype", "ex8216");
		} else {
			caff_brd_asm_id = EX82XX_GSCB_ASSEMBLY_ID;
			setenv("hw.boardtype", "ex8208");
		}

		sprintf(str_brd_assembly_id, "0x%02x%02x",
				((caff_brd_asm_id & 0xFF00) >> 8), ((caff_brd_asm_id & 0xFF)));
		setenv("hw.board.type", str_brd_assembly_id);

		setenv("hw.board.series", "ex82xx-re");
		setenv("bootp_if", CONFIG_MPC85XX_TSEC1_NAME);
	} else {    /* LCPU */
		get_assembly_id_as_string(EX82XX_LOCAL_ID_EEPROM_ADDR,
								  str_cpu_assembly_id);
		get_assembly_id_as_string(EX82XX_LC_ID_EEPROM_ADDR,
								  str_brd_assembly_id);

		sprintf(str_cpu_idaddr, "0x%02X", EX82XX_LOCAL_ID_EEPROM_ADDR);
		sprintf(str_brd_idaddr, "0x%02X", EX82XX_LC_ID_EEPROM_ADDR);

		if (strcmp(str_brd_assembly_id, "0xFFFF") == 0) {
			/*LC id eeprom failed*/
			setenv("hw.board.type", "0x092F");
		} else {
			setenv("hw.board.type", str_brd_assembly_id);
		}

		setenv("hw.board.series", "ex82xx-lc");
		assembly_id = get_assembly_id(EX82XX_LC_ID_EEPROM_ADDR);
		caff_lctype = assembly_id;
		switch (assembly_id) {
		case EX82XX_LC_48T_BRD_ID:
		case EX82XX_LC_48T_NM_BRD_ID:
			setenv("hw.boardtype", "ex8200-48t");
			break;
		case EX82XX_LC_48T_ES_BRD_ID:
			setenv("hw.boardtype", "ex8200-48t-es");
			break;
		case EX82XX_LC_48F_BRD_ID:
		case EX82XX_LC_48F_NM_BRD_ID:
			setenv("hw.boardtype", "ex8200-48f");
			break;
		case EX82XX_LC_48F_ES_BRD_ID:
			setenv("hw.boardtype", "ex8200-48f-es");
			break;
		case EX82XX_LC_8XS_BRD_ID:
		case EX82XX_LC_8XS_NM_BRD_ID:
			setenv("hw.boardtype", "ex8200-8xs");
			break;
		case EX82XX_LC_8XS_ES_BRD_ID:
			setenv("hw.boardtype", "ex8200-8xs-es");
			break;
		case EX82XX_LC_36XS_BRD_ID:
			setenv("hw.boardtype", "ex8200-36xs");
			break;
		case EX82XX_LC_40XS_BRD_ID:
		case EX82XX_LC_40XS_NM_BRD_ID:
			setenv("hw.boardtype", "ex8200-40xs");
			break;
		case EX82XX_LC_48P_BRD_ID:
			setenv("hw.boardtype", "ex8200-48p");
			break;
		case EX82XX_LC_48TE_BRD_ID:
			setenv("hw.boardtype", "ex8200-48te");
			break;
		case EX82XX_LC_2X4F40TE_BRD_ID:
			setenv("hw.boardtype", "ex8200-2x4f40te");
			break;
		case EX82XX_LC_2XS_44GE_BRD_ID:
			setenv("hw.boardtype", "ex8200-2xs-44ge");
			break;
		case EX82XX_LC_40XS_ES_BRD_ID:
			setenv("hw.boardtype", "ex8200-40xs-es");
			break;
		default:
			setenv("hw.boardtype", "Unknown");
		}

		/* Enable [space] as alternate character to stop at
		   LC uboot prompt */
		setenv("bootstopkey2", " ");
		setenv("hw.board.idaddr", str_brd_idaddr);
		setenv("hw.smb.id", str_cpu_assembly_id);
		setenv("hw.smb.idaddr", str_cpu_idaddr);
		setenv("bootp_if", CONFIG_MPC85XX_TSEC1_NAME);
		setenv("vfs.root.mountfrom", "ufs:/dev/md0");
	}
	setenv("bootcmd", CONFIG_BOOTCOMMAND);

	setenv("hw.uart.console", "mm:0xFEF04500");
	sprintf(temp, "00%02x00%02x", CFG_VERSION_MAJOR, 
			    CFG_VERSION_MINOR);
	setenv("boot.firmware.ver", temp);      
	setenv("boot.firmware.verstr", version_string);
	sprintf(temp, "%d.%d.%d",
	    CFG_VERSION_MAJOR, CFG_VERSION_MINOR, CFG_VERSION_PATCH);
	setenv("boot.ver", temp);

	if (!LCMODE_IS_STANDALONE) {
		s = getenv("bootcmd.prior");
		if (s) {
			sprintf(temp, "%s;%s", s,
			    gd->flash_bank ? CONFIG_BOOTCOMMAND_BANK1 : CONFIG_BOOTCOMMAND);
			setenv("bootcmd", temp);
		} else {
			setenv("bootcmd",
			    gd->flash_bank ? CONFIG_BOOTCOMMAND_BANK1 : CONFIG_BOOTCOMMAND);
		}
	}
	slot_id = get_slot_id();
	sprintf(temp, "%d", slot_id);
	setenv("hw.slotid", temp);

	if (!(getenv("use_my_bootdelay"))) {
		setenv("bootdelay", "1");
	}
}


#ifdef CONFIG_SHOW_ACTIVITY
/*
 * We have one LED @ offset 0x03 of the boot CPLD for all the boards.
 * This can be used to show the activity.
 */
void
toggle_cpu_led(void)
{
	volatile u_int8_t *cpu_led = (CFG_CPLD_BASE + 0x03);
	*cpu_led = (*cpu_led ^ 1);
}

void
board_show_activity(ulong timestamp)
{
	static uint32_t link_st_spd = 0;
	uint32_t link_st_spd_t;

	/* 500ms */
	if ((timestamp % (CFG_HZ / 2)) == 0) {
		toggle_cpu_led();
		/* update mgmt port LEDs */
		if (mgmt_priv) {
			link_st_spd_t = read_phy_reg(mgmt_priv, BRGPHY_MII_AUXSTS);
			link_st_spd_t = (link_st_spd_t & 0x0704);
			if ((link_st_spd_t != link_st_spd)) {
				link_st_spd = link_st_spd_t;
				set_port_led(link_st_spd);
			}
		}
	}
	/* 3s */
	if ((timestamp % (3 * CFG_HZ)) == 0) {
		if (EX82XX_RECPU) {
			keep_alive_master();
		}
	}
}

void
show_activity(int arg)
{
}
#endif

void
keep_alive_master(void)
{
	static unsigned char im_master_curr_state = 0;
	static unsigned char im_master_prev_state = 0;

	if (g_enable_scb_keep_master_alive) {
		/* If this board is master.., then keep it alive */
		static gvcb_fpga_regs_t* p_ccb_regs = 0;

		/* If we dont have valid access address, then get it */
		if ((p_ccb_regs == (gvcb_fpga_regs_t*)0) ||
			(p_ccb_regs == (gvcb_fpga_regs_t*)0xFFFFFFFF)) {
			p_ccb_regs = (gvcb_fpga_regs_t*)ccb_get_regp();
			if (p_ccb_regs == (gvcb_fpga_regs_t*)0) {
				printf("SCB Fpga not found , disabling keep alive timer \n");
				g_enable_scb_keep_master_alive = 0;
				return;
			}
		}
		if (swap_ulong(p_ccb_regs->gvcb_mstr_cnfg) & CCB_MCONF_IM_MASTER) {
			/* do this only if this board is already a master*/
			p_ccb_regs->gvcb_mstr_alive = swap_ulong(1);
			udelay(1); /*1us delay reqd. after FPGA write*/
			im_master_curr_state = 1;
		} else {
			im_master_curr_state = 0;
		}

		im_master_prev_state = im_master_curr_state;
	}
}

void
show_mastership_status(void)
{
	static gvcb_fpga_regs_t* p_ccb_regs = 0;
	unsigned int slot_id;

	/* If we dont have valid access address, then get it */
	if ((p_ccb_regs == (gvcb_fpga_regs_t*)0) ||
		(p_ccb_regs == (gvcb_fpga_regs_t*)0xFFFFFFFF)) {
		p_ccb_regs = (gvcb_fpga_regs_t*)ccb_get_regp();
	}

	slot_id = !((swap_ulong(p_ccb_regs->gvcb_slot_presence) >> 18) & 0x1);

	printf("SCB-%d Mastership status: \n", slot_id);

	if (swap_ulong(p_ccb_regs->gvcb_mstr_cnfg) & GVCB_MCONF_IM_MASTER) {
		printf("  This SCB[%d] is master \n", slot_id);
	}
	if (swap_ulong(p_ccb_regs->gvcb_mstr_cnfg) & GVCB_MCONF_HE_MASTER) {
		printf("  Other SCB is master \n");
	}
	if (swap_ulong(p_ccb_regs->gvcb_mstr_cnfg) & GVCB_MCONF_IM_RDY) {
		printf("  This SCB[%d] is master ready\n", slot_id);
	}
	if (swap_ulong(p_ccb_regs->gvcb_mstr_cnfg) & GVCB_MCONF_HE_RDY) {
		printf("  Other SCB is master ready \n");
	}
	if (swap_ulong(p_ccb_regs->gvcb_mstr_cnfg) & GVCB_MCONF_BOOTED) {
		printf("  SCB[%d] Booted\n", slot_id);
	}
	if (swap_ulong(p_ccb_regs->gvcb_mstr_cnfg) & GVCB_MCONF_RELINQUISH) {
		printf("  SCB[%d] Mastership Relinquished\n", slot_id);
	}
}

void
request_mastership_status(void)
{
	static gvcb_fpga_regs_t* p_ccb_regs;

	/* If we dont have valid access address, then get it */
	if ((p_ccb_regs == (gvcb_fpga_regs_t*)0) ||
		(p_ccb_regs == (gvcb_fpga_regs_t*)0xFFFFFFFF)) {
		p_ccb_regs = (gvcb_fpga_regs_t *)ccb_get_regp();
	}

	p_ccb_regs->gvcb_mstr_alive_cnt = swap_ulong(0x105);
	p_ccb_regs->gvcb_mstr_alive = swap_ulong(1);
	p_ccb_regs->gvcb_mstr_cnfg = swap_ulong(GVCB_MCONF_BOOTED);
	g_enable_scb_keep_master_alive = 1;
}

void
release_mastership_status(void)
{
	static gvcb_fpga_regs_t* p_ccb_regs = 0;
	ulong value;

	/* If we dont have valid access address, then get it */
	if ((p_ccb_regs == (gvcb_fpga_regs_t*)0) ||
		(p_ccb_regs == (gvcb_fpga_regs_t*)0xFFFFFFFF)) {
		p_ccb_regs = (gvcb_fpga_regs_t*)ccb_get_regp();
	}

	value = swap_ulong(p_ccb_regs->gvcb_mstr_cnfg);
	value = (value & ~GVCB_MCONF_RELINQUISH) | GVCB_MCONF_RELINQUISH;
	p_ccb_regs->gvcb_mstr_cnfg = swap_ulong(value);

	g_enable_scb_keep_master_alive = 0;
}

/* 
 * Do the Init of BootCPLD for LVCPU and RECPU 
 */
int
bootcpld_init(void)
{
	unsigned char btcpld_reg;

	if (EX82XX_LCPU) {
		btcpld_reg = *(unsigned char*)(CFG_CPLD_BASE + 0x01);
		if (is_2xs_44ge_48p_board()) {
			/* doublecap linecards */
			btcpld_reg |= 0x10;
			lc_cpld_reg_write(LC_CPLD_RESET_CTRL, btcpld_reg);
			udelay(200000);     /*200ms delay*/
		} else {
			/* legacy caffeine linecards */
			btcpld_reg |= 0x26;
			lc_cpld_reg_write(LC_CPLD_RESET_CTRL, btcpld_reg);
			udelay(200000);     /*200ms delay*/
			btcpld_reg &= (~(0x26));
			lc_cpld_reg_write(LC_CPLD_RESET_CTRL, btcpld_reg);
		}

		/* Keep the VCPU in reset until LCPU takes it out of reset */
		lc_cpld_reg_write(LC_CPLD_VCPU_RESET_CTRL, 0x01);
	} else if (EX82XX_RECPU) {
		btcpld_reg = *(unsigned char*)(CFG_CPLD_BASE + 0x01);
		btcpld_reg |= 0xbe;
		lc_cpld_reg_write(LC_CPLD_RESET_CTRL, btcpld_reg);

		udelay(200000);
		btcpld_reg &= (~(0xbe));
		lc_cpld_reg_write(LC_CPLD_RESET_CTRL, btcpld_reg);
	}
	return 0;
}

int
get_boot_string(void)
{
	unsigned char slot_id;
	unsigned char install_cmd[64], server_ip[16];
	unsigned char *env, master_re_slot;

	slot_id = get_slot_id();
	env = getenv("serverip");
	if (env == NULL) {
		sprintf(server_ip, "%s", CFG_BASE_IRI_IP);
	} else {
		sprintf(server_ip, "%s", env);
	}
	sprintf(install_cmd,
			"install tftp://%s/tftpboot/%s",
			server_ip,
			EX82XX_LC_PKG_NAME);

	setenv("run", install_cmd);
	master_re_slot = get_master_re();
	
	if (is_2xs_44ge_48p_board()) {
		lc_2xs_44ge_48p = 1;
	}

	if (master_re_slot == 1) {  /* master RE in slot 1*/
		(lc_2xs_44ge_48p)? setenv("ethact", CONFIG_MPC85XX_TSEC3_NAME) :
						   setenv("ethact", CONFIG_MPC85XX_TSEC2_NAME);
	} else { /*master RE in slot 0*/
		(lc_2xs_44ge_48p)? setenv("ethact", CONFIG_MPC85XX_TSEC2_NAME) :
						   setenv("ethact", CONFIG_MPC85XX_TSEC1_NAME);
	}
	set_current_addr();
	eth_set_current();

	return 0;
}

/*
 * get the assembly id from the i2c eeprom
 */
uint16_t
get_assembly_id(unsigned char id_eeprom_addr)
{
	u_int16_t asm_id = 0xffff;
	u_int8_t data[2];
	int32_t ret;

	if ((ret = eeprom_read(id_eeprom_addr, EX82XX_ASSEMBLY_ID_OFFSET,
						   data, 2)) != 0) {
		printf("I2C read from EEPROM device 0x%x failed.\n", id_eeprom_addr);
	} else {
		asm_id = (data[0] & 0xff) << 8 | (data[1] & 0xff);
	}

	return asm_id;
}

/*
 * Assumes str_assembly_id[] has enough memory allocated
 */
int
get_assembly_id_as_string(unsigned char id_eeprom_addr,
							  unsigned char* str_assembly_id)
{
	u_int16_t assembly_id;

	assembly_id = get_assembly_id(id_eeprom_addr);
	sprintf(str_assembly_id, "0x%02X%02X", ((assembly_id & 0xFF00) >> 8),
			((assembly_id & 0xFF)));

	return 0;
}

unsigned char
get_slot_id(void)
{
	/* If this board is master.., then keep it alive */
	static gvcb_fpga_regs_t* p_ccb_regs = 0;
	unsigned int slot_id;
	uchar lc_slot_id = 0xff;

	if (EX82XX_RECPU) {
		/* If we dont have valid access address, then get it */
		if ((p_ccb_regs == (gvcb_fpga_regs_t*)0) ||
			(p_ccb_regs == (gvcb_fpga_regs_t*)0xFFFFFFFF)) {
			p_ccb_regs = (gvcb_fpga_regs_t*)ccb_get_regp();
		}
		slot_id = (swap_ulong(p_ccb_regs->gvcb_slot_presence));
		slot_id &= 0x0000f000;
		slot_id >>= 12;
		if ((slot_id != 0) && (slot_id != 1)) {
			/* set to default group address in standalone mode */
			printf("*** slot id not [0 | 1] : undefined[%d] - assign 0\n",
			    slot_id);
			slot_id = 0;
		}
		return (unsigned char)(slot_id & 0xFF);
	} else {
		lc_cpld_reg_read(LC_CPLD_STATUS, &lc_slot_id);
		lc_slot_id &= LC_CPLD_SLOT_MASK;
		return lc_slot_id;
	}
}

unsigned char
get_master_re(void)
{
	unsigned char master_re_slot;

	if (!EX82XX_RECPU) {
		master_re_slot = *(unsigned char*)(CFG_CPLD_BASE + 0x08);
		master_re_slot = (master_re_slot & 0x80) >> 7;
		return master_re_slot;
	}
	return 0;
}

void
assign_ip_mac_addr(void)
{
	uchar enetaddr[18];
	uchar enetname[18];
	uchar ipaddr[18];
	uchar ipname[18];
	u_int32_t slot_id;
	unsigned char master_re_slot;
	uchar chip_type = 0, intf_id = 0;
	/* Base Mac address */
	uchar enetmacaddr[6] = { 0x00, 0x0B, 0xCA, 0xFE, 0x00, 0x00 };
	/* Base IP address */
	uchar base_iri_ip[4] = { 128, 0, 0, 1 };
	int busdevfunc, i;
	uint plx_base_addr;       /* base address */
	volatile ulong *pcie_scratch_pad_base;
	static gvcb_fpga_regs_t* p_ccb_regs = 0;
	int bp_access_retry_count, access;
	unsigned int pex_device_id = 0;

	sprintf(ipname, "ipaddr");

	if (!EX82XX_RECPU) {
		if (is_2xs_44ge_48p_board()) {
			lc_2xs_44ge_48p = 1;
		}			
		/* Since this is the first switch . */
		(lc_2xs_44ge_48p)? (pex_device_id = PCIE_PEX8509_DID) :
						   (pex_device_id = PCIE_PEX8508_DID);
		busdevfunc = pcie_find_device(PCIE_PLXTECH_VID,
									  pex_device_id,
									  0); /* get PCI Device ID */
		if (busdevfunc == -1) {
			printf("Error PCI- PLX SWITCH  (%04X,%04X) not found\n",
				   PCIE_PLXTECH_VID, pex_device_id);
		}

		pcie_read_config_dword(busdevfunc, PCIE_BASE_ADDRESS_0, &plx_base_addr);
		pcie_scratch_pad_base =
			(volatile unsigned long*)(plx_base_addr + (64 * 1024) + 0xB0);
	}


	/* In the case of RE , we should not touch the IP address  address.
	 * whatever user has programmed shall be used.
	 * so just handle the case for LCPU
	 */
	if (EX82XX_RECPU) {
		ulong group = 0, device = 0;
		int offset = 0;
		uint8_t tdata[256];
		u8 *a = (u8*)&offset;
		int alen = 1, bp_eeprom_r, ret;
		union {
			uint8_t  d8;
			uint8_t  da[4];
			uint16_t d16;
			uint32_t d32;
		} val;
		uint16_t mac_addr_cnt;
		union {
			unsigned long long llma;
			uint8_t            ma[8];
		} mac;
		u_int16_t assembly_id;

		i2c_controller(CFG_I2C_CTRL_1);
		assembly_id = get_assembly_id(EX82XX_LOCAL_ID_EEPROM_ADDR);
		switch (assembly_id) {
		case EX82XX_GRANDE_CHASSIS:
			group = 0x71;  /*Signal Backplane*/
			break;
		case EX82XX_VENTI_CHASSIS:
			group = 0xBE; /*Mid plane*/
			break;
		deafult:
			group = 0x71;
		}

		device = 0x51;
		alen = 1;
		offset = 0x36;
		mac.llma = 0;
		bp_eeprom_r = 0;

		/* If we dont have valid access address, then get it */
		if ((p_ccb_regs == (gvcb_fpga_regs_t*)0) ||
			(p_ccb_regs == (gvcb_fpga_regs_t*)0xFFFFFFFF)) {
			p_ccb_regs = (gvcb_fpga_regs_t*)ccb_get_regp();
		}

		slot_id = get_slot_id();

		/* If ORE is present, check whether access to BP is free */
		if (ore_present(slot_id)) {
			bp_access_retry_count = 10;
			while (bp_access_retry_count--) {
				access = get_bp_eeprom_access();
				if (access == 0) {
					break;
				}
				udelay(100000);
			}
			if (access) {
				printf("Timeout: Taking the BP access control\n");
				set_re_state(RE_STATE_EEPROM_ACCESS);
			}
		}
		reset_bp_renesas();
		debug("\nAssigning MAC addr for the board on slot-%d ... \n", slot_id);

		if ((ret = ccb_iic_write((u_int8_t)group, (u_int8_t)device,
								 alen, &a[4 - alen])) != 0) {
			bp_eeprom_r = 0;
			printf("I2C read from device 0x%x failed.\n", device);
		} else {
			alen = 2;
			if ((ret = ccb_iic_read((u_int8_t)group, (u_int8_t)device,
									alen, tdata)) != 0) {
				bp_eeprom_r = 0;
				printf("I2C read from device 0x%x failed.\n", device);
			} else {
				bp_eeprom_r = 1;
				for (i = 0; i < alen; i++) {
					val.da[i] = tdata[i];
				}
			}
		}
		mac_addr_cnt = val.d16;

		if (bp_eeprom_r) {
			alen = 1;
			offset = 0x38;
			if ((ret = ccb_iic_write((u_int8_t)group, (u_int8_t)device,
									 alen, &a[4 - alen])) != 0) {
				bp_eeprom_r = 0;
				printf("I2C read from device 0x%x failed.\n", device);
			} else {
				alen = 6;
				if ((ret = ccb_iic_read((u_int8_t)group, (u_int8_t)device,
										alen, tdata)) != 0) {
					bp_eeprom_r = 0;
					printf("I2C read from device 0x%x failed.\n", device);
				} else {
					bp_eeprom_r = 1;
					for (i = 0; i < alen; i++) {
						mac.ma[i + 2] = tdata[i];
					}
				}
			}
		}

		set_re_state(RE_STATE_BOOTING);

		if (slot_id) {
			mac.llma += (mac_addr_cnt - 2);
		} else {
			mac.llma += (mac_addr_cnt - 1);
		}

		if (bp_eeprom_r) {
			sprintf(enetaddr, "%02X:%02X:%02X:%02X:%02X:%02X", mac.ma[2],
					mac.ma[3], mac.ma[4], mac.ma[5],
					mac.ma[6], mac.ma[7]);
			sprintf(enetname, "ethaddr");
			setenv(enetname, enetaddr);
			debug("%s - %s (BP)\n", enetname, enetaddr);
		}
		/*
		 * The MAC address for the internal routing interfaces has to be allocated
		 * based on the IRI scheme for ex82xx systems.
		 */
		enetmacaddr[4] = slot_id;

		chip_type = EX82XX_IRI_RE_CHIP_TYPE;
		for (i = 0; i < 4; i++) {
			if (bp_eeprom_r  && (i == 0)) {
				continue;
			}

			if (slot_id) {  /*RE in slot 1*/
				if (i == 3) {
					intf_id = 1;
				} else if (i == 1) {
					intf_id = 0;
				} else {
					intf_id = i;
				}
			} else {    /*RE in slot 0*/
				if (i == 3) {
					intf_id = 0;
				} else {
					intf_id = i;
				}
			}
			enetmacaddr[5] = ((chip_type << EX82XX_CHIP_TYPE_SHIFT) &
							  EX82XX_CHIP_TYPE_MASK) |
							 (intf_id & EX82XX_IRI_IF_MASK);
			sprintf(enetaddr, "%02X:%02X:%02X:%02X:%02X:%02X", enetmacaddr[0],
					enetmacaddr[1], enetmacaddr[2], enetmacaddr[3],
					enetmacaddr[4], enetmacaddr[5]);
			if (i == 0) {
				sprintf(enetname, "ethaddr");
			} else {
				sprintf(enetname, "eth%daddr", i);
			}

			setenv(enetname, enetaddr);
			debug("%s - %s \n", enetname, enetaddr);
		}
	}

	/* Assign network settings only when the LC is in chassis. 
	   In Standalone mode, dont override the settings set by the user */
	if ((EX82XX_LCPU) && !LCMODE_IS_STANDALONE) {
		slot_id = get_slot_id();
		debug("\nAssigning MAC addr for the board on slot-%d ... \n", slot_id);
		enetmacaddr[4] = slot_id + 2;

		chip_type = EX82XX_IRI_LCPU_CHIP_TYPE;

		for (i = 0; i < 4; i++) {
			enetmacaddr[5] = ((chip_type << EX82XX_CHIP_TYPE_SHIFT) &
							  EX82XX_CHIP_TYPE_MASK) |
							 (i & EX82XX_IRI_IF_MASK);
			sprintf(enetaddr, "%02X:%02X:%02X:%02X:%02X:%02X", enetmacaddr[0],
					enetmacaddr[1], enetmacaddr[2], enetmacaddr[3],
					enetmacaddr[4], enetmacaddr[5]);

			master_re_slot = get_master_re();
			if (is_2xs_44ge_48p_board()) {
				if (i == 0) {
					sprintf(enetname, "ethaddr");
					setenv(enetname, enetaddr);
				}

				if (master_re_slot) {
					/* when master RE is in slot 1, use the
					 * me2 interface from doublecap card to
					 * connect to RE.
					 */
					if (i > 0 && i <= 2) {
						sprintf(enetname, "eth%daddr", i);
						setenv(enetname, enetaddr);
					}
				} else {
					/* when master RE is in slot 0, use the
					 * me1 interface from doublecap card to
					 * connect to RE.
					 */
					if (i < 2) {
						sprintf(enetname, "eth%daddr", i+1);
						setenv(enetname, enetaddr);
					}
				}
			} else {
				if (i == 0) {
					sprintf(enetname, "ethaddr");
				} else {
					sprintf(enetname, "eth%daddr", i);
				}
				setenv(enetname, enetaddr);
			}
			debug("%s - %s \n", enetname, enetaddr);
		}

		debug("\n");

		/* set the  gateway IP and Server IP  to 128.0.0.1*/
		sprintf(ipaddr, "%d.%d.%d.%d", base_iri_ip[0],
				base_iri_ip[1], base_iri_ip[2], 1);

		setenv("serverip", ipaddr);
		setenv("gatewayip", ipaddr);
		setenv("netmask", "255.0.0.0");

		debug("\nAssigning IP addr for the board ... \n");

		/* set the IRI ip address for this LCPU */
		base_iri_ip[3] = EX82XX_IRI_IP_LCPU_BASE + slot_id;

		sprintf(ipaddr, "%d.%d.%d.%d", base_iri_ip[0],
				base_iri_ip[1], base_iri_ip[2], base_iri_ip[3]);

		setenv(ipname, ipaddr);

		debug("%s - %s \n", ipname, ipaddr);
		debug("\n");

		/* We should provide the base address for VCPU through PCIE
		 * scratch pad registers. */
		enetmacaddr[5] =
			((EX82XX_IRI_VCPU_CHIP_TYPE << EX82XX_CHIP_TYPE_SHIFT) &
			 EX82XX_CHIP_TYPE_MASK);
		pcie_scratch_pad_base[0] = enetmacaddr[2] << 24 | enetmacaddr[3] <<
								   16 |
								   enetmacaddr[4] << 8 | enetmacaddr[5];
		pcie_scratch_pad_base[1] = enetmacaddr[0] << 8 | enetmacaddr[1];

		/* Pass the IP address to VNPU based on slot_id */
		base_iri_ip[3] = EX82XX_IRI_IP_VCPU_BASE + slot_id;
		pcie_scratch_pad_base[2] = base_iri_ip[0] << 24 | base_iri_ip[1] <<
								   16 |
								   base_iri_ip[2] << 8 | base_iri_ip[3];

	}
	if (EX82XX_LCPU) {
		/* Bring the VCPU out of reset */
		debug("\nBringing VCPU out of reset ..... \n");
		lc_cpld_reg_write(LC_CPLD_VCPU_RESET_CTRL, 0x00);
	}
}

int
lcd_printf_2(const char *fmt, ...)
{
	return -1;
}

void
set_current_addr(void)
{
	/*
	 *  Fill the bi_enetaddr variable in the bd structure with
	 *  the value of active interface's MAC address. So loader
	 *  can use the MAC addr of active interface instead of TSEC0's
	 *  MAC address.
	 */
	char ethaddr_if[10];
	int tsec_if, i;
	char *s, *e;
	bd_t *bd;

	bd = gd->bd;
	s = getenv("ethact");
	if (s) {
		if (strncmp(s, "me", 2) == 0) {
			if ((s[2] >= 0x30) && (s[2] < 0x34)) {
				tsec_if = s[2] - 0x30;
			} else {
				tsec_if = 0;
			}
			if (tsec_if) {
				if (is_2xs_44ge_48p_board() && (tsec_if == 2)) {
					/* when using me2 interface, we need to ensure
					 * that we are using eth1addr for ethaddr.
					 */
					sprintf(ethaddr_if, "eth%daddr", (tsec_if-1));
				} else {
					sprintf(ethaddr_if, "eth%daddr", tsec_if);
				}
			} else {
				sprintf(ethaddr_if, "ethaddr");
			}

			s = getenv(ethaddr_if);
			if (s) {
				for (i = 0; i < 6; ++i) {
					bd->bi_enetaddr[i] = s ? simple_strtoul(s, &e, 16) : 0;
					if (s) {
						s = (*e) ? e + 1 : e;
					}
				}
			}
		}
	}
}

void
btcpld_cpu_hard_reset(void)
{
	unsigned char *btcpld_reset = (CFG_CPLD_BASE + 0x01);

	*btcpld_reset = 0x01;
}

int
set_upgrade_ok(void)
{
	unsigned char temp[32];
	flash_banks_t current_bank;
	upgrade_state_t ustate;
	ulong set_state, state = get_upgrade_state();

	current_bank = gd->flash_bank;
	if (current_bank == FLASH_BANK1) {
		ustate = UPGRADE_OK_B1;
	} else {
		ustate = UPGRADE_OK_B0;
	}
	if (state == ustate) {
		set_state = 0;
	} else {
		set_state = 1;
	}
	if (set_state) {
		set_upgrade_state(ustate);
	}

	ustate = get_upgrade_state();

	sprintf(temp, "%d ", ustate);
	setenv("boot.state", temp);

	sprintf(temp, "%d", current_bank);
	setenv("boot.primary.bank", temp);

	*(unsigned char *)(CFG_CPLD_BASE + LC_CPLD_BOOT_CTLSTAT) = 
		is_2xs_44ge_48p_board() ? 0x05 : 0x01;

	if (EX82XX_RECPU) {
		if (!check_internal_usb_storage_present()) {
			ex82xx_usb_err_recover();
		}
		ex82xx_usb_access_test();
	}

	return 0;
}

int
disable_bank0(void)
{
	volatile u_int8_t *cpld_bank_sel = 
		(u_int8_t*)(CFG_CPLD_BASE + LC_CPLD_BOOT_CTLSTAT);

	*cpld_bank_sel |= is_2xs_44ge_48p_board() ? ~0x04 : 0x02;
}

int
enable_bank0(void)
{
	volatile u_int8_t *cpld_bank_sel = 
		(u_int8_t*)(CFG_CPLD_BASE + LC_CPLD_BOOT_CTLSTAT);

	*cpld_bank_sel |= is_2xs_44ge_48p_board() ? 0x04 : ~0x02;
}

int
boot_next_bank(void)
{
	/* do reset */
	volatile unsigned char *cpld_addr = (CFG_CPLD_BASE + 0x0A);
	volatile uint8_t *btcpld_rstreason = (CFG_CPLD_BASE + LC_CPLD_RESET_REASON);
	volatile uint8_t *btcpld_scratchpad = (CFG_CPLD_BASE + LC_CPLD_SCRATCH_REG);
	uint8_t val_t;

	/* 
	 * Save reboot reason in the scratch reg.
	 *
	 * If Bank1 is the active bank, switching to bank1
	 * from bank0 clears the original reboot reason.
	 * So save it if there is a bank switch.
	 */
	val_t = *btcpld_rstreason;
	val_t &= BOOT_REBOOT_REASON_MASK;
	*btcpld_scratchpad = val_t;

	*cpld_addr = ((*cpld_addr) | (1 << 2) | (1 << 7));  /*Enable next boot bank*/
	*cpld_addr = ((*cpld_addr) & (~0x01));              /*Restart timer on reset*/
	*cpld_addr = ((*cpld_addr) & (~(1 << 2)));
	*(volatile unsigned char *)(CFG_CPLD_BASE + 0x01) = 0x01;
}

int
set_next_bootbank(flash_banks_t bank)
{
	volatile unsigned char *cpld_addr = (CFG_CPLD_BASE + 0x0a), cpld_val;
	unsigned char select_bank;

	printf("FLASH BANK: %d\n", gd->flash_bank);
	printf("Switching to FLASH BANK: %d\n", bank);
	cpld_val = *cpld_addr;

	if (bank == FLASH_BANK1) {
		select_bank = (1 << 6);
		cpld_val |= select_bank;
		*cpld_addr = cpld_val;
	} else {
		select_bank = ~(1 << 6);
		cpld_val &= select_bank;
		*cpld_addr = cpld_val;
	}
	return 0;
}

flash_banks_t
get_booted_bank(void)
{
	flash_banks_t current_bank;
	volatile unsigned char *cpld_addr = (CFG_CPLD_BASE + 0x0A);

	current_bank = ((*(unsigned char *)(cpld_addr)) & 0x02);

	if (current_bank) {
		return FLASH_BANK1;
	} else {
		return FLASH_BANK0;
	}
}

int
valid_uboot_image(unsigned long addr)
{
	ulong icrc;
	image_header_t *haddr = (image_header_t *)(addr + CFG_IMG_HDR_OFFSET);

	if ((icrc = crc32(0, (unsigned char *)(addr + CFG_CHKSUM_OFFSET),
					  CFG_MONITOR_LEN - CFG_CHKSUM_OFFSET)) ==
		haddr->ih_dcrc) {
		printf("crc: 0x%x / 0x%x\n", icrc, haddr->ih_dcrc);
		return 1;
	}
	printf("crc: 0x%x / 0x%x\n", icrc, haddr->ih_dcrc);
	return 0;
}

int
valid_env(unsigned long addr)
{
	ulong icrc;
	volatile u_int32_t *flenv_crc = (volatile u_int32_t *)addr;

	icrc = crc32(0, (addr + 0x04), (CFG_ENV_SIZE - 0x04));
	if (icrc != *flenv_crc) {
		printf("Invalid environment: Bad CRC\n");
		return 0;
	}
	return 1;
}

int
set_upgrade_state(upgrade_state_t state)
{
	ulong upg_state_addr = (CFG_FLASH_BASE + CFG_UPGRADE_BOOT_STATE_OFFSET);
	ulong ustate = state;

	int flash_write_direct(ulong addr, char *src, ulong cnt);

	return flash_write_direct(upg_state_addr, (char *)&ustate, sizeof(ulong));
}

int
get_upgrade_state(void)
{
	volatile unsigned long ustate_t, *ustate =
		(upgrade_state_t *)(CFG_FLASH_BASE +
							CFG_UPGRADE_BOOT_STATE_OFFSET);
	flash_banks_t current_bank;

	current_bank = gd->flash_bank;

	ustate_t = *ustate;
	if ((ustate_t < INITIAL_STATE_B0) || (ustate_t >= UPGRADE_STATE_END)) {
		if (current_bank == FLASH_BANK0) {
			set_upgrade_state(INITIAL_STATE_B0);
		} else {
			set_upgrade_state(INITIAL_STATE_B1);
		}
	}
	return *ustate;
}

int
check_upgrade(void)
{
	ulong addr;
	flash_banks_t current_bank;
	volatile u_int8_t *cpld_ver_reg = (CFG_CPLD_BASE), cpld_ver;
	u_int8_t exp_cpld_ver;
	u_int8_t bootctl;

	extern int valid_elf_image(unsigned long addr);

	cpld_ver = *cpld_ver_reg;

	gd->flash_bank = get_booted_bank();
	current_bank = gd->flash_bank;

	if (current_bank == FLASH_BANK0) {
		bootctl = *(unsigned char*)
			(CFG_CPLD_BASE + LC_CPLD_BOOT_CTLSTAT);
		bootctl |= 0x04;
		lc_cpld_reg_write(LC_CPLD_BOOT_CTLSTAT, bootctl);
		bootctl = *(unsigned char*)
			(CFG_CPLD_BASE + LC_CPLD_BOOT_CTLSTAT);
	}

	upgrade_state_t state = get_upgrade_state();
	if (ram_init_done == 1) {
		printf("FLASH bank: %d\n", current_bank);
	}

	if ((state == UPGRADE_START_B0) &&
		(ram_init_done == 0x01)) {
		if (current_bank == FLASH_BANK1) {
			enable_bank0();     
			if (valid_env(CFG_FLASH_BASE) &&
				valid_elf_image(CFG_UPGRADE_LOADER_BANK0) &&
				valid_uboot_image(CFG_UPGRADE_UBOOT_BANK0)) {
				set_upgrade_state(UPGRADE_CHECK);
			} else {
				/*upgrade not proper-upgrade failed*/
				set_upgrade_state(INITIAL_STATE_B1);
			}
			disable_bank0();
		} else {
			if (valid_env(CFG_FLASH_BASE) &&
				valid_elf_image(CFG_UPGRADE_LOADER_BANK0) &&
				valid_uboot_image(CFG_UPGRADE_UBOOT_BANK0)) {
				set_upgrade_state(UPGRADE_TRY_B0);
			}
		}
	}
	if ((state == UPGRADE_START_B1) &&
		(ram_init_done == 0x01)) {
		if (current_bank == FLASH_BANK0) {
			if (valid_env(CFG_FLASH_BASE) &&
				valid_elf_image(CFG_UPGRADE_LOADER_BANK1) &&
				valid_uboot_image(CFG_UPGRADE_UBOOT_BANK1)) {
				set_upgrade_state(UPGRADE_CHECK);
			} else {
				/*upgrade not proper..failed*/
				set_upgrade_state(INITIAL_STATE_B1);
			}
		} else {
			/*check bank1 image.***BANK1 is also seen at BANK0 address by the CPU*/
			/*may be we can use a different Macro*/
			if (valid_env(CFG_FLASH_BASE) &&
				valid_elf_image(CFG_UPGRADE_LOADER_BANK0) &&
				valid_uboot_image(CFG_UPGRADE_UBOOT_BANK0)) {
				set_upgrade_state(UPGRADE_TRY_B1);
			}
		}
	}
	state = get_upgrade_state();
	/*upgrade: check the other bank*/
	if (state == UPGRADE_CHECK) {
		if (current_bank == FLASH_BANK0) {
			set_upgrade_state(UPGRADE_TRY_B1);
			set_next_bootbank(FLASH_BANK1);
		} else {
			set_upgrade_state(UPGRADE_TRY_B0);
			set_next_bootbank(FLASH_BANK0);
		}
		boot_next_bank();
	}
	if (state == UPGRADE_TRY_B0) {
		if (current_bank != FLASH_BANK0) {
			/*upgraded bank failed to boot*/
			set_upgrade_state(INITIAL_STATE_B1);
		}
	}
	if (state == UPGRADE_TRY_B1) {
		if (current_bank != FLASH_BANK1) {
			/*upgraded bank failed to boot*/
			set_upgrade_state(INITIAL_STATE_B0);
		}
	}
	if (state == UPGRADE_OK_B0) {
		if (current_bank != FLASH_BANK0) {
			set_upgrade_state(UPGRADE_TRY_B0);
			set_next_bootbank(FLASH_BANK0);
			boot_next_bank();
		}
	}
	if (state == UPGRADE_OK_B1) {
		if (current_bank != FLASH_BANK1) {
			set_upgrade_state(UPGRADE_TRY_B1);
			set_next_bootbank(FLASH_BANK1);
			boot_next_bank();
		}
	}
	return 0;
}

uint
eeprom_valid(uchar i2c_addr)
{
	uchar buf[2];

	buf[0] = buf[1] = 0x00;

	i2c_read(i2c_addr, 0, 1, (uchar*)buf, sizeof(buf));
	if ((buf[0] == 0x7F) && (buf[1] == 0xB0)) {
		return 1;
	} else {
		return 0;   /*Invalid Data*/
	}
}

/*
 * This function reads the IDEEPROM and identifies the CPU type
 */
int
get_cpu_type(void)
{
	uchar board_type;
	int caf_cpu_type;
	uchar i2c_addr;
	u_int16_t assembly_id;

	i2c_controller(CFG_I2C_CTRL_1);
	i2c_addr = EX82XX_LOCAL_ID_EEPROM_ADDR;

	if (!eeprom_valid(i2c_addr)) {
		board_type = 0xFF;
	} else {
		caf_cpu_type = 0;
		assembly_id = get_assembly_id(i2c_addr);

		switch (assembly_id) {
		case EX82XX_PGSCB_ASSEMBLY_ID:
		case EX82XX_GSCB_ASSEMBLY_ID:
		case EX82XX_GRANDE_CHASSIS:
		case EX82XX_VENTI_CHASSIS:
			board_type = RECPU;
			break;
		case EX82XX_PLVCPU_ASSEMBLY_ID:
		case EX82XX_LC_2XS_44GE_BRD_ID:
		case EX82XX_LC_2X4F40TE_BRD_ID: 
		case EX82XX_LC_48P_BRD_ID:
		case EX82XX_LC_48TE_BRD_ID:
		case EX82XX_LC_48T_ES_BRD_ID:
		case EX82XX_LC_48F_ES_BRD_ID:
		case EX82XX_LC_8XS_ES_BRD_ID:
		case EX82XX_LC_40XS_ES_BRD_ID:
			board_type = LCPU;
			break;
		default:
			board_type = 0xFF;
			break;
		}
	}
	caf_cpu_type  = board_type;
	return caf_cpu_type;
}

int
find_cpu_type(void)
{
	volatile immap_t *immap = (immap_t *)CFG_IMMR;
	volatile ccsr_lbc_t *memctl = &immap->im_lbc;
	int type = 0;
	u_int16_t assembly_id;

	type = get_cpu_type();  /*Identify the CPU type from IDEEPROM data*/

	if ((type > 0) && (type <= CFG_MAX_CPU_COUNT)) {
		gd->ccpu_type = type;
	} else {
		gd->ccpu_type = 0x00;
	}

	/* To bring the i2c switches on lc out of reset */
	if (EX82XX_LCPU) {
		volatile immap_t* immap = (immap_t*)CFG_CCSRBAR;
		volatile ccsr_gur_t* gur = &immap->im_gur;
		gur->gpiocr  = gur->gpiocr  | 0x00000200;   /* configure GPOUT pins as output */
		gur->gpoutdr = gur->gpoutdr | 0x00000080;   /* SET GPOUT[24] */

		/*
		 * If the Double Cap line cards are power-cycled then the I2C 
		 * switches in the DCAP FPGA retain their state. This means 
		 * that if an I2C channel was opened prior to power-cycle, it 
		 * remains open after power-cycle.
		 * Actual failure scenario: Double Cap cards have board idprom
		 * at 0x51 on I2C switch 0x71. It also has SFP EEPROM at 0x51,
		 * channels 4, 5, 6, and 7 on I2C switch 0x71. Now, if we
		 * access SFP EEPROM, power-cycle the card and try to access
		 * the board id eeprom at 0x51, it returns junk values (or 
		 * at times may result in crash) and board boot up hangs 
		 * because board id eeprom has junk values.
		 * To fix this problem, read the EEPROM at 0x57 (in Double Cap,
		 * we are having both 0x51 and 0x57 with same exact values) 
		 * to identify the board type _and_ the ID EEPROM at 0x51 will 
		 * only be used by master RE as in other legacy Caffeine cards.
		 */
		assembly_id = get_assembly_id(EX82XX_LOCAL_ID_EEPROM_ADDR);
		switch (assembly_id) {
		case EX82XX_LC_48P_BRD_ID:
		case EX82XX_LC_48TE_BRD_ID:
		case EX82XX_LC_2X4F40TE_BRD_ID:
		case EX82XX_LC_2XS_44GE_BRD_ID:
			/*
			 * Reset the I2c Switch 0x71 (CFG_I2C_CTRL_2_SW_ADDR),
			 * while board boots up so that contention for 0x51
			 * IDEEPROM is eliminated.
			 */
			if (i2c_write(CFG_I2C_CTRL_2_SW_ADDR , 0x0, 1,
				      NULL, 0) != 0) {
				printf("I2C write to device %x failed.\n",
				       CFG_I2C_CTRL_2_SW_ADDR);
			}
			break;
		default:
			break;
		}
	}

	if (is_2xs_44ge_48p_board()) {
#if defined(CFG_BR0_PRELIM) && defined(CFG_OR0_PRELIM_2XS_44GE_48P)
		memctl->br0 = CFG_BR0_PRELIM;
		memctl->or0 = CFG_OR0_PRELIM_2XS_44GE_48P;
#endif

#if defined(CFG_BR2_PRELIM) && defined(CFG_OR2_PRELIM_2XS_44GE_48P)
		memctl->br2 = CFG_BR2_PRELIM;
		memctl->or2 = CFG_OR2_PRELIM_2XS_44GE_48P;
#endif

#if defined(CFG_BR3_PRELIM) && defined(CFG_OR3_PRELIM_2XS_44GE_48P)
		memctl->br3 = CFG_BR3_PRELIM;
		memctl->or3 = CFG_OR3_PRELIM_2XS_44GE_48P;
#endif	
	}

	return 0;
}

/*
 * Disable core1 on multicore P2020E CPU's. In Caffeine doublecap linecards,
 * we use P2020E (dual core) processors and we disable core1 thro'
 * boot holdoff mode in hardware. So, in software, set the devdisr (device
 * disable register) appropriately to disable core1. Also note that, we don't
 * use CONFIG_MP in our config file, which effectively means that we have
 * disabled MP in doublecap. 
 */
int
disable_p2020e_core1(void)
{
	volatile immap_t *immap = (immap_t *)CFG_CCSRBAR;
	volatile ccsr_gur_t *gur = &immap->im_gur;
	uint32_t svr;

	/* if it's not doublecap linecard, just return. */
	if (!is_2xs_44ge_48p_board()) {
		return (FALSE);
	}
	
	/* if it's doublecap linecard based on P2010, just return. */
	svr = SVR_VER(get_svr());
	if (svr == SVR_P2010 || svr == SVR_P2010_E) {
		return (FALSE);
	}

	/* disable core1 */
	out_be32(&gur->devdisr, in_be32(&gur->devdisr) | MPC85xx_DEVDISR_CPU1);

	/* disable core1 time base at the platform */
	out_be32(&gur->devdisr, in_be32(&gur->devdisr) | MPC85xx_DEVDISR_TB1);

	return (EOK);
}

void
ex82xx_btcpld_cpu_reset(void)
{
	unsigned char *cpld_cpu_reset = (unsigned char*)(CFG_CPLD_BASE + 0x1);

	*cpld_cpu_reset = 0x1;
}

void
init_ex82xx_platform_funcs(void)
{
	/* Init ex82xx specific function pointers here */
	ex82xx_cpu_reset = ex82xx_btcpld_cpu_reset;
	board_usb_err_recover = ex82xx_usb_err_recover;
}

int
write_i2cs_scratch(uint8_t val, uint8_t slot)
{
	uint8_t group, device, tdata[2];
	uint8_t offset;
	int ret;
	uint16_t assembly_id;

	i2c_controller(CFG_I2C_CTRL_1);
	assembly_id = get_assembly_id(EX82XX_LOCAL_ID_EEPROM_ADDR);
	switch (assembly_id) {
	case EX82XX_GRANDE_CHASSIS:
		group = CFG_EX8208_SCB_SLOT_BASE + slot;
		break;
	case EX82XX_VENTI_CHASSIS:
		group = CFG_EX8216_CB_SLOT_BASE + slot;
		break;
	deafult:
		group = 0x00;
	}

	device = SCB_SLAVE_CPLD_ADDR;
	offset = I2CS_SCRATCH_PAD_REG_00;

	tdata[0] = ccb_i2cs_set_odd_parity(offset & 0xFF);
	tdata[1] = ccb_i2cs_set_odd_parity(val);
	if ((ret = ccb_iic_write (group, device, 2, tdata)) != 0) {
		printf ("I2CS write @ 0x%x to scratch pad failed.\n", device);
		goto ret_err;
	}
	return 0;

ret_err:
	return -1;
}

int
read_i2cs_scratch(uint8_t* val, uint8_t slot)
{
	uint8_t group, device, tdata[1];
	uint8_t offset;
	int ret;
	uint16_t assembly_id;

	i2c_controller(CFG_I2C_CTRL_1);
	assembly_id = get_assembly_id(EX82XX_LOCAL_ID_EEPROM_ADDR);
	switch (assembly_id) {
	case EX82XX_GRANDE_CHASSIS:
		group = CFG_EX8208_SCB_SLOT_BASE + slot;
		break;
	case EX82XX_VENTI_CHASSIS:
		group = CFG_EX8216_CB_SLOT_BASE + slot;
		break;
	deafult:
		group = 0x00;
	}

	device = SCB_SLAVE_CPLD_ADDR;
	offset = I2CS_SCRATCH_PAD_REG_00;

	tdata[0] = ccb_i2cs_set_odd_parity(offset & 0xFF);
	if ((ret = ccb_iic_write(group, device, 1, tdata)) != 0) {
		printf ("I2CS write to device 0x%x failed.\n", device);
		goto ret_err;
	} else {
		if ((ret = ccb_iic_read(group, device, 1, tdata)) != 0) {
			printf ("I2C read from device 0x%x failed.\n", device);
			goto ret_err;
		} else {
			*val = tdata[0];
		}
	}

	return 0;

ret_err:
	return -1;
}

int
set_re_state(uint8_t val)
{
	uint8_t slot;
	int ret_err;
	gvcb_fpga_regs_t *scbc_base;

	if (!EX82XX_RECPU) {
		return 0;
	}

	slot = get_slot_id();
	ret_err = write_i2cs_scratch(val, slot);

	if (val == RE_STATE_BOOTING) {
		/* setup the I2C MUX select Reg to select BP */
		scbc_base = ccb_get_regp();
		scbc_base->gvcb_i2cs_mux_ctrl = swap_ulong(0x3880);
	}

	return ret_err;
}

int
get_ore_state(uint8_t* val)
{
	uint8_t slot;
	int ret_err;

	/* slot = ORE slot */
	if (get_slot_id()) {
		slot = SLOT_SCB0;
	} else {
		slot = SLOT_SCB1;
	}

	ret_err = read_i2cs_scratch(val, slot);

	return ret_err;
}

int
get_bp_eeprom_access(void)
{
	uint8_t slot, val;
	int ret_err;

	slot = get_slot_id();
	ret_err = get_ore_state(&val);
	if (ret_err) {
		goto bp_access_fail;
	}
	val = (val & 0x7F);

	if (val == RE_STATE_EEPROM_ACCESS) {
		goto bp_access_fail;
	}

	/* get the access */
	val = RE_STATE_EEPROM_ACCESS;
	ret_err = set_re_state(val);
	if (ret_err) {
		goto bp_access_fail;
	}
	
	if (slot == SLOT_SCB0) {
		return 0;
	}
	/* If RE1 */
	val = 0;
	ret_err = get_ore_state(&val);
	if (ret_err) {
		goto bp_access_fail;
	}
	val = (val & 0x7F);
	if (val != RE_STATE_EEPROM_ACCESS) {
		return 0;
	}
	ret_err = set_re_state(0);
	if (ret_err) {
		goto bp_access_fail;
	}
	return 1;

bp_access_fail:
	return -1;
}

/*
 * Reset backplane renesas chip
 */
void
reset_bp_renesas(void)
{
	gvcb_fpga_regs_t *scbc_base;
	uint32_t val;

	scbc_base = ccb_get_regp();
	val = 0x00;
	/* Keep the devices in reset */
	scbc_base->gvcb_device_ctrl = swap_ulong(val);
	udelay(500);
	/* Bring the security chip out of reset */
	val |= GVCB_RST_CNTL_SEC_RESET;
	scbc_base->gvcb_device_ctrl = swap_ulong(val);
	/* 250ms delay after renesas reset */
	udelay(250000);
}

/*
 * workaround for the external USB issue -
 * Tell the I2CS CPLD to powercycle the board in case
 * of USB error
 */
void
ex82xx_usb_err_recover(void)
{
	uint8_t val;
	int32_t ret;

	debug("*** USB err\n");
	ret = read_i2cs_cpld(I2CS_CPLD_I2C_RST_REG, &val);
	if (ret == 0) {
		val |= BIT_POWERCYCLE_REG07;
		write_i2cs_cpld(I2CS_CPLD_I2C_RST_REG, val);
	}
}

/*
 * Check whether internal USB flash has been
 * detected.
 */
int
check_internal_usb_storage_present(void)
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

/*
 * Disable flash swizzle in boot CPLD.
 */
void
btcpld_swizzle_disable(void)
{
	volatile uint8_t *btcpld_swizzle;

	btcpld_swizzle = (uint8_t*)(CFG_CPLD_BASE + BTCPLD_FLASH_SWIZZLE_REG);
	*btcpld_swizzle |= BTCPLD_REG0A_CPU_BOOTED;
}

/*
 * USB host controller problem:
 *    The problem is that ISP1564 USB host controller has a 
 * POR problem that releate to the transceiver and the clocking
 * during reset. The failure mode is varied, and to the most part
 * easy to detect: failure to detect the internal Nand flash.
 * There's one failure mode wherein USB functions and sectors can be
 * read and written to the Nand flash, but certain data can cause the
 * PLL to fail. The data in question is the long sequence of 1's,
 * because that yields the least number of transitions between HIGH and
 * LOW voltages. When this happens data gets corrupted, with or without
 * CRC errors on the USB bus! when CRC errors are detected, the UBS-Nand
 * flash controller keeps silent and forces a retransmit. This repeats
 * ad infinitum. The USB driver timesout after 5 seconds and aborts the
 * transfer. This wedges the USB host controller and USB stops working
 * from that moment on. This triggers our workaround.
 *
 * If the sector data (all 0xff's) got corrupted without USB CRC error,
 * it will be written to the NAND flash, from which we subsequently read.
 * If we dont read all 0xff's back, we know the USB host controller didnot
 * properly come out of POR and we apply the workaournd.
 *
 * USB access test:  
 *  - Write the value 0xFF to blocks 2 - 5 of USB disk
 *  - read back blocks 2 - 5 and verify the values.
 *  - powercycle the board if there is any USB error 
 *    or mismatch in the data read.
 *  - exit and boot normally if there are no errors.
 */
void
ex82xx_usb_access_test(void)
{
	uint32_t mem, cnt;
	int32_t curr_device, block, count;
	int32_t c, disk;
	char *s;
	volatile uint32_t *buffer;
	struct usb_device *dev = NULL;
	block_dev_desc_t *stor_dev;
	unsigned long n;
	uint32_t ext_usb_present = 0;

	/* find the current active disk */
	s = getenv("loaddev");
	if (!strncmp(s, "disk1", 5)) {
		debug("Curr device: 1\n");
		curr_device = 1;
	} else {
		debug("Curr device: 0\n");
		curr_device = 0;
	}

	for (c = 0; c < USB_MAX_DEVICE; c++) {
		dev = usb_get_dev_index(c);
		if (dev == NULL)
			break;
		if (dev->portnr == 1) {
			ext_usb_present = 1;
		}
	}

	if (curr_device == 0) {
		/* internal USB disk */
		if (ext_usb_present) {
			disk = 1;
		} else {
			disk = 0;
		}
	} else {
		/* external USB */
		disk = 0;
	}

	mem = CFG_USBCHK_MEM_READ_ADDR;
	buffer = (uint32_t*)mem;
	block = 2;
	count = 4; /* blocks 2 - 5 (4 x 512 bytes) */

	for (cnt = 0; cnt < 0x800; cnt += 4) {
		/* Fill 2K of memory with 0xFF */
		*buffer = 0xFFFFFFFF;
		buffer++;
	}

	stor_dev = usb_stor_get_dev(disk);

	/* write FF to blocks 2 - 5 of USB disk */
	n = stor_dev->block_write(disk, block, count, (ulong*)mem);
	if (n == count) {
		debug ("write %ld blocks: %s - dev[%d] blk[%d] cnt[%d]\n",
		    n, "OK", disk, block, count);
	} else {
		debug ("write %ld blocks: %s - dev[%d] blk[%d] cnt[%d]\n",
		    n, "ERROR", disk, block, count);
		/* error writing to USB, recover */
		ex82xx_usb_err_recover();
	}

	mem = CFG_USBCHK_MEM_WRITE_ADDR;
	/* Read USB disk blocks 2 - 5 */
	n = stor_dev->block_read(disk, block, count, (ulong *)mem);
	if (n == count) {
		debug ("read %ld blocks: %s - dev[%d] blk[%d] cnt[%d]\n",
		    n, "OK", disk, block, count);
	} else {
		debug ("read %ld blocks: %s - dev[%d] blk[%d] cnt[%d]\n",
		    n, "ERROR", disk, block, count);
		/* Error reading USB, recover */
		ex82xx_usb_err_recover();
	}

	buffer = (uint32_t*)mem;
	/* Verify the values read from USB disk */
	for (cnt = 0; cnt < 0x800; cnt += 4) {
		if (*buffer != 0xFFFFFFFF) {
			debug("USB read: wrong data[%x != 0xFFFFFFFF]!!\n", *buffer);
			/* Unexpected value read from USB, recover */
			ex82xx_usb_err_recover();
			return;
		}
		buffer++;
	}
}

int disable_bank0_atprompt(void)
{
	volatile u_int8_t *cpld_bank_sel = 
		(u_int8_t*)(CFG_CPLD_BASE + LC_CPLD_BOOT_CTLSTAT);

	if (gd->flash_bank == FLASH_BANK1) {
		*cpld_bank_sel |= (lc_2xs_44ge_48p) ? ~0x04 : 0x02;
	}
}

#ifdef CONFIG_POST
DECLARE_GLOBAL_DATA_PTR;

void post_word_store(ulong a)
{
	volatile ulong *save_addr_word =
		(volatile ulong *)(POST_STOR_WORD);
	*save_addr_word = a;
}

ulong post_word_load(void)
{
	volatile ulong *save_addr_word =
		(volatile ulong *)(POST_STOR_WORD);
	return *save_addr_word;
}

int post_hotkeys_pressed(void)
{
	return 0;   /* No hotkeys supported */
}

#endif


/*********************************************************/
