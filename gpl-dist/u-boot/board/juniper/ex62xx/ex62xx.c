/*
 * Copyright (c) 2010-2013, Juniper Networks, Inc.
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
#include <config.h>
#include <asm/fsl_i2c.h>
#include <asm/fsl_law.h>
#include <asm/immap_85xx.h>
#include <asm/io.h>
#include <asm/processor.h>
#include <i2c.h>
#include <led.h>
#include <net.h>
#include <tsec.h>
#include <usb.h>

#include <ccb_iic.h>
#include <cpld.h>
#include <ex62xx_common.h>
#include <jcb.h>


DECLARE_GLOBAL_DATA_PTR;


int flash_write_direct(ulong addr, char* src, ulong cnt);
int find_boardtype(void);
int flash_swizzled(void);
int cpu_lc_init_f(void);
phys_size_t fixed_sdram(void);
void assign_ip_mac_addr(void);
#ifdef CONFIG_SILENT_CONSOLE
void disable_console_out(void);
void enable_console_out(void);
#endif

extern int valid_elf_image(unsigned long addr);
extern jcb_fpga_regs_t *ccb_get_regp(void);

uint8_t ex62xx_lc_mode = 0;

struct tsec_private *mgmt_priv = NULL;
extern char version_string[];
extern int i2cs_read(uint8_t group, uint8_t device, uint addr, int alen,
	       uint8_t *data, int dlen);
extern int i2cs_write(uint8_t group, uint8_t device, uint addr, int alen,
	       uint8_t *data, int dlen);

int board_early_init_f(void)
{
	return 0;
}

#if defined(CONFIG_UBOOT_MULTIBOARDS)
int find_boardtype(void)
{
	uint16_t assembly_id;
	unsigned long board_type;

	/* Init I2C */
	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);

	assembly_id = get_assembly_id(EX62XX_LOCAL_ID_EEPROM_ADDR);
	
	switch (assembly_id) {
	case EX6208_CHASSIS_ID:
	case EX6208_JSCB_ASSEMBLY_ID:
		board_type = BOARD_SCB;
		break;
	case EX62XX_LC48T_ASSEMBLY_ID:
		board_type = BOARD_LC;
		break;
	case EX62XX_LC48P_ASSEMBLY_ID:
		board_type = BOARD_LC;
		break;
	case EX62XX_LC48F_ASSEMBLY_ID:
		board_type = BOARD_LC;
		break;
	default:
		board_type = 0xFF;
	}

	gd->board_type = board_type;

	/* setup br/or registers for LC */
	if (EX62XX_LC) {
	    cpu_lc_init_f();
	}
	return 0;
}
#endif

int cpu_lc_init_f(void)
{
#if defined(CONFIG_SYS_LC_BR2_PRELIM)
	set_lbc_br(2, CONFIG_SYS_LC_BR2_PRELIM);
#endif
#if defined(CONFIG_SYS_LC_OR2_PRELIM)
	set_lbc_or(2, CONFIG_SYS_LC_OR2_PRELIM);
#endif
#if defined(CONFIG_SYS_LC_BR3_PRELIM)
	set_lbc_br(3, CONFIG_SYS_LC_BR3_PRELIM);
#endif
#if defined(CONFIG_SYS_LC_OR3_PRELIM)
	set_lbc_or(3, CONFIG_SYS_LC_OR3_PRELIM);
#endif
    uint16_t assembly_id;

    assembly_id = get_assembly_id(EX62XX_LOCAL_ID_EEPROM_ADDR);

    /* The 48F LC has a expander cpld on CS4 */
    if (assembly_id == EX62XX_LC48F_ASSEMBLY_ID) {
	set_lbc_or(4, CONFIG_SYS_XCPLD_OR4_PRELIM);
	set_lbc_br(4, CONFIG_SYS_XCPLD_BR4_PRELIM);
    }

    return 0;
}

int checkboard(void)
{
	uint16_t assembly_id;
	unsigned long board_type;

	assembly_id = get_assembly_id(EX62XX_LOCAL_ID_EEPROM_ADDR);
	
	switch (assembly_id) {
	case EX6208_CHASSIS_ID:
	case EX6208_JSCB_ASSEMBLY_ID:
		board_type = BOARD_SCB;
		puts("Board: EX6200-SRE64-4XS\n");
		break;
	case EX62XX_LC48T_ASSEMBLY_ID:
		board_type = BOARD_LC;
		puts("Board: EX6200-48T\n");
		break;
	case EX62XX_LC48P_ASSEMBLY_ID:
		board_type = BOARD_LC;
		puts("Board: EX6200-48P\n");
		break;
	case EX62XX_LC48F_ASSEMBLY_ID:
		board_type = BOARD_LC;
		puts("Board: EX6200-48F\n");
		break;
	default:
		board_type = 0xFF;
		puts("Board: Unknown\n");
	}

	return 0;
}

phys_size_t initdram(int board_type)
{
	phys_size_t dram_size = 0;

	puts("Initializing\n");

	if (EX62XX_SCB) {
	    dram_size = fsl_ddr_sdram();
	} else {
	    dram_size = fixed_sdram();
	    if (set_ddr_laws(CONFIG_SYS_DDR_SDRAM_BASE,
			     dram_size,
			     LAW_TRGT_IF_DDR) < 0) {
		printf("ERROR setting Local Access Windows for DDR\n");
		return 0;
	    };
	}
	dram_size = setup_ddr_tlbs(dram_size / 0x100000);
	dram_size *= 0x100000;

	puts("    DDR: ");
	return dram_size;
}

/*
 * Fixed sdram init -- doesn't use serial presence detect.
 */

phys_size_t fixed_sdram(void)
{
	volatile ccsr_ddr_t *ddr = (ccsr_ddr_t *)CONFIG_SYS_MPC85xx_DDR_ADDR;
	uint d_init;

	/* Don't enable the memory controller now. */
	ddr->sdram_cfg = CONFIG_SYS_SDRAM_CFG_INITIAL;

	ddr->cs0_bnds = CONFIG_SYS_DDR3_CS0_BNDS;
	ddr->cs0_config = CONFIG_SYS_DDR3_CS0_CONFIG;
	ddr->cs0_config_2 = CONFIG_SYS_DDR3_CS0_CONFIG_2;
	ddr->cs1_bnds = CONFIG_SYS_DDR3_CS1_BNDS;
	ddr->cs1_config = CONFIG_SYS_DDR3_CS1_CONFIG;
	ddr->cs1_config_2 = CONFIG_SYS_DDR3_CS1_CONFIG_2;
	/* CONFIG_SYS_DDR3_TIMING_3 sets EXT_REFREC(trfc) to 32 clks. */
	ddr->timing_cfg_3 = CONFIG_SYS_DDR3_TIMING_3;
	ddr->timing_cfg_0 = CONFIG_SYS_DDR3_TIMING_0;
	ddr->timing_cfg_1 = CONFIG_SYS_DDR3_TIMING_1;
	ddr->timing_cfg_2 = CONFIG_SYS_DDR3_TIMING_2;
	ddr->sdram_cfg_2 = CONFIG_SYS_SDRAM_CFG_2;
	ddr->sdram_mode = CONFIG_SYS_DDR3_MODE;
	ddr->sdram_mode_2 = CONFIG_SYS_DDR3_MODE_2;
	ddr->sdram_md_cntl = CONFIG_SYS_DDR3_MD_CNTL;
	ddr->sdram_interval = CONFIG_SYS_DDR3_INTERVAL;
	ddr->sdram_data_init = CONFIG_SYS_DDR3_DATA_INIT;
	ddr->sdram_clk_cntl = CONFIG_SYS_DDR3_CLK_CNTL;
	ddr->init_addr = CONFIG_SYS_DDR3_INIT_ADDR;
	ddr->init_ext_addr = CONFIG_SYS_DDR3_INIT_EXT_ADDR;
	ddr->timing_cfg_4 = CONFIG_SYS_DDR3_TIMING_4;
	ddr->timing_cfg_5 = CONFIG_SYS_DDR3_TIMING_5;
	ddr->ddr_zq_cntl = CONFIG_SYS_DDR3_ZQ_CNTL;
	ddr->ddr_wrlvl_cntl = CONFIG_SYS_DDR3_WRLVL_CNTL;
	ddr->ddr_sr_cntr = CONFIG_SYS_DDR3_SR_CNTR;
	ddr->ddr_sdram_rcw_1 = CONFIG_SYS_DDR3_RCW_1;
	ddr->ddr_sdram_rcw_2 = CONFIG_SYS_DDR3_RCW_2;
	ddr->ddr_cdr1 = CONFIG_SYS_DDR3_CDR1;

#if defined (CONFIG_DDR_ECC)
	ddr->err_disable = CONFIG_SYS_DDR3_ERR_DISABLE;
	ddr->err_sbe = CONFIG_SYS_DDR3_ERR_SBE;
#endif

#if defined (CONFIG_DDR_ECC) && defined (CONFIG_ECC_INIT_VIA_DDRCONTROLLER)
	ddr->sdram_cfg_2 |= CONFIG_SYS_DDR3_DINIT; /* DINIT = 1 */
#endif
	asm("sync;isync");
	udelay(500);

	unsigned int temp_sdram_cfg;	
	/* Set, but do not enable the memory yet. */
	temp_sdram_cfg = ddr->sdram_cfg;
	temp_sdram_cfg &= ~(SDRAM_CFG_MEM_EN);
	ddr->sdram_cfg = temp_sdram_cfg;

	/*
	 * At least 500 micro-seconds (in case of DDR3) must
	 * elapse between the DDR clock setup and the DDR config
	 * enable. For now to be on the safer side, wait for 1000
	 * micro-seconds and optimize later after the bringup.
	 */
	asm("sync;isync");
	udelay(1000);

	/* Let the controller go. Enable. */
	ddr->sdram_cfg = temp_sdram_cfg | SDRAM_CFG_MEM_EN | 0x20000000;
#if defined (CONFIG_DDR_ECC)
	/* Enable ECC checking. */
	ddr->sdram_cfg = CONFIG_SYS_SDRAM_CFG | 0x20000000;
#else
	/* ddr->sdram_cfg = CONFIG_SYS_SDRAM_CFG_NO_ECC; */
#endif
	asm("sync; isync");
	udelay(500);

#if defined (CONFIG_DDR_ECC) && defined (CONFIG_ECC_INIT_VIA_DDRCONTROLLER)
	while((ddr->sdram_cfg_2 & (CONFIG_SYS_DDR3_DINIT)) != 0) {
	    udelay(1000);
	}
	asm("sync; isync");
	udelay(500);
#endif
	return CONFIG_SYS_SDRAM_SIZE * 1024 * 1024;

}

void pci_init_board(void) 
{
}

int misc_init_f(void)
{
	if (EX62XX_LC)
	    cpu_lc_init_f();

	/* 
	 * reset devices on the board
	 */
	btcpld_reset_sequence();
	ctlpld_reset_sequence();

	return 0;
}

int misc_init_r(void)
{
	uint8_t val_t, major, minor;
	uint8_t jcbc_rev_min, jcbc_rev_maj;
	uint16_t assembly_id;
	char *s;

	i2c_set_bus_num(CONFIG_I2C_BUS_SMB);
	assembly_id = get_assembly_id(EX62XX_LOCAL_ID_EEPROM_ADDR);

	/* select default i2c - 1st I2C controller */
	i2c_set_bus_num(CONFIG_I2C_BUS_SMB);

	if (EX62XX_SCB) {
	    /* Init i2cs */
	    ccb_i2c_init();
	}

	assign_ip_mac_addr();

	/* PCIe workaround */
	if (EX62XX_LC) {
	    /* XXX pcie workaround....valid for SCB???? */
	    volatile uint32_t *ccsr_srdscr3 = (volatile uint32_t *)(0xfefe300c);
	    *ccsr_srdscr3 = 0x1313;
	}

	set_caffeine_default_environment();

	printf("\nFirmware Version: --- %02X.%02X.%02X ---\n",
	    CFG_VERSION_MAJOR, \
	    CFG_VERSION_MINOR, \
	    CFG_VERSION_PATCH);

	printf("CPLD/FPGA image versions on the board:\n\t");
	if (EX62XX_SCB) {
		get_jcbc_rev(&jcbc_rev_maj, &jcbc_rev_min);
		printf("JCBC FPGA = %02x.%02x |", jcbc_rev_maj, jcbc_rev_min);
	}
	btcpld_read(BOOT_FPGA_VER, &val_t);
	printf(" BTCPLD = %02X ", val_t);
	printf("\n\t");

	if (assembly_id == EX62XX_LC48F_ASSEMBLY_ID) {
		/* 
		 * For accessing the xcpld registers,
		 * the xcpld needs to be taken out of
		 * reset. The xcpld reset is done
		 * through the same signal which is
		 * used to get the POE module out of
		 * reset on 48P cards.
		*/
		ctlcpld_read(JLC_CTL_POE_CNTL, &val_t);
		val_t &= ~POE_RESET;
		ctlcpld_write(JLC_CTL_POE_CNTL, val_t);
		udelay(10000);

		xcpld_read(XCPLD_MAJOR_VER, &major);
		xcpld_read(XCPLD_MINOR_VER, &minor);
		printf(" XCPLD  = %02X.%02X \n", major, minor);
	}

	if (EX62XX_LC) {
		ex62xx_lc_mode = 0;
		/* Is LC in standalone mode? */
		s = getenv(CFG_LC_MODE_ENV);
		if (s) {
			ex62xx_lc_mode = simple_strtoul(s, NULL, 16);
			if (ex62xx_lc_mode > LC_MODE_MAX_VAL) {
				ex62xx_lc_mode = 0;
			}
			if (LCMODE_IS_STANDALONE) {
				printf(" *** LC is in Standalone mode *** \n");
			}
			if (LCMODE_IS_DIAGNOSTIC) {
				printf(" *** LC is in Diagnostics mode *** \n");
			}
		}
	}
	/* standalone jumper present? */
	val_t = 0;
	btcpld_read(BOOT_LC_STATUS, &val_t);
	if (val_t & BOOT_STANDALONE_JMPR) {
		printf(" *** Standalone Jumper present *** \n");
		if (EX62XX_LC) {
			ex62xx_lc_mode = LCMODE_IS_STANDALONE;
		}
	}

	if (!valid_env(CONFIG_SYS_FLASH_BASE)) {
		saveenv();
	}

	return 0;
}

int last_stage_init(void)
{
	struct eth_device *dev;

	dev = eth_get_dev_by_name(CONFIG_TSEC1_NAME);
	if (dev) {
		mgmt_priv = (struct tsec_private*) dev->priv;
	}

	if ((EX62XX_LC) && !LCMODE_IS_STANDALONE) {
		get_boot_string();
	}

	return 0;
}

int board_start_usb(void)
{
	usb_multi_root_hub = 0;
	usb_num_root_hub = 1;

	/* USB device present only on SCB */
	if (EX62XX_SCB)
		return TRUE;
	else
		return FALSE;
}

int board_init_nand(void)
{
	/* NAND device present only on SCB */
	if (EX62XX_SCB) {
#ifndef CONFIG_PRODUCTION
		return TRUE;
#else
		/* NAND device will not be initialized */
		return FALSE;
#endif
	} else {
		return FALSE;
	}
}

void set_caffeine_default_environment(void)
{
	char str_brd_asmid[16];
	uint16_t assembly_id;
	char temp[120];
	char *s;
	uint8_t slot_id;

	if (EX62XX_SCB) {
		i2c_set_bus_num(CONFIG_I2C_BUS_SMB);

		sprintf(str_brd_asmid, "0x%02x%02x",
		    ((EX6208_JSCB_ASSEMBLY_ID & 0xFF00) >> 8),
		    ((EX6208_JSCB_ASSEMBLY_ID & 0xFF)));
		setenv("hw.board.type", str_brd_asmid);

		setenv("hw.boardtype", "ex6210");
		setenv("hw.board.series", "ex62xx-re");
	} else {    /* LCPU */
		i2c_set_bus_num(CONFIG_I2C_BUS_SMB);
		get_assembly_id_as_string(EX62XX_LOCAL_ID_EEPROM_ADDR,
		    (uint8_t *)str_brd_asmid);


		setenv("hw.board.type", str_brd_asmid);
		setenv("hw.board.series", "ex62xx-lc");
		assembly_id = get_assembly_id(EX62XX_LOCAL_ID_EEPROM_ADDR);
		switch (assembly_id) {
		case EX62XX_LC48T_ASSEMBLY_ID:
			setenv("hw.boardtype", "ex6200-48t");
			break;
		case EX62XX_LC48P_ASSEMBLY_ID:
			setenv("hw.boardtype", "ex6200-48p");
			break;
		case EX62XX_LC48F_ASSEMBLY_ID:
			setenv("hw.boardtype", "ex6200-48f");
			break;
		default:
			printf("\n*** board type unknown ***\n\n");
			setenv("hw.boardtype", "Unknown");
		}

		/* 
		 * Enable [space] as alternate character to stop at
		 * LC uboot prompt 
		 */
		setenv("bootstopkey2", " ");
		setenv("vfs.root.mountfrom", "ufs:/dev/md0");
	}
	setenv("bootcmd", CONFIG_BOOTCOMMAND);
	setenv("hw.uart.console", "mm:0xFEF04500");

	/* firmware version */
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
 * CPU_UP led can be used to show the activity.
 */
void toggle_cpu_led(void)
{
	uint8_t cpu_led;

	btcpld_read(BOOT_CONTROL, &cpu_led);
	cpu_led ^= BOOT_CPU_UP_LED;
	btcpld_write(BOOT_CONTROL, cpu_led);
}

void board_show_activity(ulong timestamp)
{
	static uint32_t link_st_spd = 0xffff;
	uint32_t link_st_spd_t;

	/* 500ms */
	if ((timestamp % (CONFIG_SYS_HZ / 2)) == 0) {
		toggle_cpu_led();

		/* update mgmt port LEDs */
		if (mgmt_priv && EX62XX_SCB) {
			link_st_spd_t = read_phy_reg(mgmt_priv, MIIM_BCM54xx_AUXSTATUS);
			link_st_spd_t = (link_st_spd_t & 0x0704);
			if ((link_st_spd_t != link_st_spd)) {
				link_st_spd = link_st_spd_t;
				set_port_led(link_st_spd);
			}
		}
	}
	/* 3s */
	if ((timestamp % (3 * CONFIG_SYS_HZ)) == 0) {
		if (EX62XX_SCB) {
			keep_alive_master();
		}
	}
}

void show_activity(int arg)
{
}

void ex62xxcb_phy_led_select(uint32_t copper)
{
	jcb_fpga_regs_t* p_ccb_regs;

	p_ccb_regs = (jcb_fpga_regs_t *)ccb_get_regp();

	if (copper) {
	    /* select copper port LED */
	    p_ccb_regs->jcb_phy_cntl |= JCB_PHY_CTL_COPPER;
	} else {
	    /* select SFP port LED */
	    p_ccb_regs->jcb_phy_cntl &= ~(JCB_PHY_CTL_COPPER);
	}
}
#endif

/* 
 * bootcpld controlled devices reset sequence 
 */
void btcpld_reset_sequence(void)
{
	uint8_t val_t;

	btcpld_read(BOOT_RST_CTL, &val_t);
	/* assert pex and dram reset */
	val_t |= BOOT_PEX_RESET;
	if (EX62XX_SCB) {
		val_t &= ~(BOOT_DRAM_RESET_L);
	} else {		/* EX62XX_LC */
		val_t |= BOOT_DRAM_RESET_L;
	}
	btcpld_write(BOOT_RST_CTL, val_t);
	debug("btcpld rst: %x\n", val_t);

	udelay(100000);

	/* deassert pex and dram reset */
	val_t &= ~(BOOT_PEX_RESET);
	if (EX62XX_SCB) {
		val_t |= BOOT_DRAM_RESET_L;
	} else {		/* EX62XX_LC */
		val_t &= ~(BOOT_DRAM_RESET_L);
	}
	btcpld_write(BOOT_RST_CTL, val_t);
	debug("btcpld rst: 0x%x\n", btcpld_read(BOOT_RST_CTL));
}

/*
 * ctlpld controlled devices reset sequence
 */
void ctlpld_reset_sequence(void)
{
	if (EX62XX_LC) {
	    /* CHJ reset */
	    ctlcpld_write(0x3c, 0x44);
	    udelay(1000000);
	    ctlcpld_write(0x3c, 0x00);
	    return;
	}

	jcb_fpga_regs_t* p_ccb_regs;

	p_ccb_regs = (jcb_fpga_regs_t *)ccb_get_regp();
	
	/* assert reset */
	/* DUART_N */
	p_ccb_regs->jcb_device_ctrl = 0x00 | JCB_RST_CNTL_DUART;
	udelay(10000);
	
	p_ccb_regs->jcb_device_ctrl = (JCB_RST_CNTL_USB_PHY |
	    JCB_RST_CNTL_PLL | JCB_RST_CNTL_MGMT_PHY |
	    JCB_RST_CNTL_ETH_SW | JCB_RST_CNTL_SECURITY | JCB_RST_CNTL_LMAC) &
	    (~JCB_RST_CNTL_DUART);

	udelay(1000000);
	p_ccb_regs->jcb_device_ctrl |= JCB_RST_CNTL_USB_HUB;
	udelay(10000);
	p_ccb_regs->jcb_device_ctrl |= JCB_RST_CNTL_USB_NAND;
	udelay(1000000);
	debug("ctlpld rst: 0x%x\n", p_ccb_regs->jcb_device_ctrl);
}

void get_boot_string(void)
{
	char install_cmd[64], server_ip[16];
	char *env;
	uint8_t slot_id;
	uint8_t master_re_slot;

	if (!EX62XX_LC) {
		return;
	}

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
	    EX62XX_LC_PKG_NAME);

	setenv("run", install_cmd);
	master_re_slot = get_master_re();
	if (master_re_slot) {
		setenv("ethact", CONFIG_TSEC3_NAME);
	} else { /*backup RE*/
		setenv("ethact", CONFIG_TSEC2_NAME);
	}
	set_current_addr();
	eth_set_current();
}

/*
 * get the assembly id from the i2c eeprom
 */
uint16_t get_assembly_id(uint8_t eeprom_addr)
{
	uint16_t asm_id;
	uint8_t data[2];
	int ret;

	i2c_set_bus_num(CONFIG_I2C_BUS_SMB);
	if ((ret = i2c_read(eeprom_addr, EX62XX_ASSEMBLY_ID_OFFSET, 1,
	    (uint8_t *)data, sizeof(data))) != 0) {
		asm_id = 0xffff;
		printf("***I2C read from EEPROM device 0x%x failed.\n", eeprom_addr);
	} else {
		asm_id = ((data[0] << 8) | data[1]);
	}

	return asm_id;
}

/*
 * Assumes str_assembly_id[] has enough memory allocated
 */
int get_assembly_id_as_string(uint8_t ideprom_addr, uint8_t* str_asmid)
{
	uint16_t asm_id;

	asm_id = get_assembly_id(ideprom_addr);
	sprintf(str_asmid, "0x%02X%02X", ((asm_id & 0xFF00) >> 8),
	    (asm_id & 0xFF));

	return 0;
}

uint8_t get_slot_id(void)
{
	uint8_t slot_id;

	if (EX62XX_SCB) {
		return scb_slot_id();
	} else {
		btcpld_read(BOOT_LC_STATUS, &slot_id);
		return (slot_id & BOOT_SLOT_ID_MASK);
	}
}

uint8_t get_master_re(void)
{
	uint8_t master_re;

	if (!EX62XX_SCB) {
		btcpld_read(BOOT_LC_STATUS, &master_re);
		master_re &= BOOT_RE_MASTER;
		return master_re ? SLOT_SCB1 : SLOT_SCB0;
	}
	return 0;
}

void set_current_addr(void)
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
			if ((s[2] >= 0x30) && (s[2] < 0x34)) {	/* >= '0'  < '4' */
				tsec_if = s[2] - 0x30; /*s[2] is ascii */
			} else {
				tsec_if = 0;
			}
			if (tsec_if) {
				sprintf(ethaddr_if, "eth%daddr", tsec_if);
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

void assign_ip_mac_addr(void)
{
	uchar enetaddr[18];
	uchar enetname[16];
	uchar ipaddr[16];
	uchar ipname[16];
	uint32_t slot_id;
	uint8_t chip_type = 0, intf_id = 0;
	/* Base Mac address */
	uint8_t enetmacaddr[6] = { 0x00, 0x0B, 0xCA, 0xFE, 0x01, 0x02 };
	/* Base IP address */
	uint8_t base_iri_ip[4] = { 128, 0, 0, 1 };
	int32_t busdevfunc, i;
	int32_t bp_access_retry_count, access;

	sprintf(ipname, "ipaddr");

	/* In the case of RE , we should not touch the IP address address.
	 * whatever user has programmed shall be used.
	 * so just handle the case for LCPU
	 */
	if (EX62XX_SCB) {
	        
		uint8_t group = 0, device = 0;
		int32_t offset = 0;
		uint8_t tdata[256];
		uint8_t *a = (uint8_t*)&offset;
		int alen, bp_eeprom_r, ret, dlen;
		union {
			uint8_t d8;
			uint8_t da[4];
			uint16_t d16;
			uint32_t d32;
		} val;
		uint16_t mac_addr_cnt;
		union {
			unsigned long long llma;
			uint8_t ma[8];
		} mac;
		uint16_t assembly_id;

		mac.llma = 0;
		bp_eeprom_r = 0;

		slot_id = get_slot_id();

		/* If ORE is present, check whether access to BP is free */
		if (ore_present(slot_id)) {
			bp_access_retry_count = 10;
			while (bp_access_retry_count--) {
				access = get_bp_eeprom_access();
				if (access == 0) {
					break;
				}
				udelay(100000);		/* 100ms delay */
			}
			if (access) {
				printf("Timeout: Taking the BP access control\n");
				set_re_state(RE_STATE_EEPROM_ACCESS);
			}
		}

		debug("\nAssigning MAC addr for the board on slot-%d ... \n", slot_id);

		group = 0x1c;  /*Signal Backplane*/
		device = 0x51;
		offset = 0x36;
		alen = 1;
		dlen = 2;
		ret = i2cs_read(group, device, offset, alen, tdata, dlen); 
		if (ret != 0) {
		    bp_eeprom_r = 0;
		    printf("i2c read from device 0x%x failed.\n", device);
		} else {
		    bp_eeprom_r = 1;
		    for (i = 0; i < dlen; i++) {
			val.da[i] = tdata[i];
		    }
		}
		mac_addr_cnt = val.d16;

		if (bp_eeprom_r) {
			alen = 1;
			dlen = 6;
			offset = 0x38;
			ret = i2cs_read(group, device, offset, alen, tdata, dlen);
			if (ret != 0) {
			    bp_eeprom_r = 0;
			    printf("i2c read from device 0x%x failed.\n", device);
			} else {
			    bp_eeprom_r = 1;
			    for (i = 0; i < dlen; i++) {
				mac.ma[i + 2] = tdata[i];
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
		 * The MAC address for the internal routing interfaces 
		 * has to be allocated based on the IRI scheme for 
		 * ex62xx systems.
		 */
		enetmacaddr[4] = slot_id;

		chip_type = EX62XX_IRI_RE_CHIP_TYPE;
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
			enetmacaddr[5] = ((chip_type << EX62XX_CHIP_TYPE_SHIFT) &
			    EX62XX_CHIP_TYPE_MASK) |
			    (intf_id & EX62XX_IRI_IF_MASK);
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

	/* 
	 * Assign network settings only when the LC is in chassis.
	 * In Standalone mode, dont override the settings set by the user 
	 */
	if ((EX62XX_LC) && !LCMODE_IS_STANDALONE) {
		slot_id = get_slot_id();
		debug("\nAssigning MAC addr for the board on slot-%d \n", slot_id);
		enetmacaddr[4] = slot_id + 2;

		chip_type = EX62XX_IRI_LCPU_CHIP_TYPE;

		uint8_t re1_is_master = 0;
		int set_ethaddr = 0;
		sprintf(enetname, "ethaddr");
		re1_is_master = get_master_re();
		for (i = 0; i < 2; i++) {
			enetmacaddr[5] = ((chip_type << EX62XX_CHIP_TYPE_SHIFT) &
			    EX62XX_CHIP_TYPE_MASK |
			    (i & EX62XX_IRI_IF_MASK));
			sprintf(enetaddr, "%02X:%02X:%02X:%02X:%02X:%02X", enetmacaddr[0],
			    enetmacaddr[1], enetmacaddr[2], enetmacaddr[3],
			    enetmacaddr[4], enetmacaddr[5]);

			/* active interfaces used on this board XXX */
			sprintf(enetname, "eth%daddr", i+1);

			setenv(enetname, enetaddr);
			debug("%s - %s \n", enetname, enetaddr);
			if (re1_is_master) {
				if (i == 1) {
					set_ethaddr = 1;
				}
			} else {
				if (i == 0) {
					set_ethaddr = 1;
				}
			}
			if (set_ethaddr) {
				sprintf(enetname, "ethaddr");
				setenv(enetname, enetaddr);
				debug("%s - %s \n", enetname, enetaddr);
				set_ethaddr = 0;
			}
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
		base_iri_ip[3] = EX62XX_IRI_IP_LCPU_BASE + slot_id;

		sprintf(ipaddr, "%d.%d.%d.%d", base_iri_ip[0],
		    base_iri_ip[1], base_iri_ip[2], base_iri_ip[3]);

		setenv(ipname, ipaddr);

		debug("%s - %s \n", ipname, ipaddr);
		debug("\n");
	}
}

int set_upgrade_ok(void)
{
	char temp[32];
	flash_banks_t current_bank;
	upgrade_state_t ustate;
	uint32_t set_state, state;
	uint8_t val_t;
   
	state = get_upgrade_state();
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

	btcpld_read(BOOT_BOOT_CTLSTAT, &val_t);
	val_t |= BOOT_OK;
	btcpld_write(BOOT_BOOT_CTLSTAT, val_t);

	return 0;
}

int disable_bank0(void)
{
	uint8_t val_t;

	btcpld_read(BOOT_BOOT_CTLSTAT, &val_t);
	val_t &= ~(BOOT_FLASH_UPGRADE);
	btcpld_write(BOOT_BOOT_CTLSTAT, val_t);
}

int enable_bank0(void)
{
	uint8_t val_t;

	btcpld_read(BOOT_BOOT_CTLSTAT, &val_t);
	val_t |= BOOT_FLASH_UPGRADE;
	btcpld_write(BOOT_BOOT_CTLSTAT, val_t);
}

void boot_next_bank(void)
{
	uint8_t val_t;

	/* enable next boot bank */
	btcpld_read(BOOT_BOOT_CTLSTAT, &val_t);
	val_t |= (BOOT_FLASH_UPGRADE | BOOT_NEXT_PARTITION);
	btcpld_write(BOOT_BOOT_CTLSTAT, val_t);

	/* restart timer on reset */
	btcpld_read(BOOT_BOOT_CTLSTAT, &val_t);
	val_t &= ~(BOOT_OK);
	btcpld_write(BOOT_BOOT_CTLSTAT, val_t);
	val_t &= ~(BOOT_FLASH_UPGRADE);
	btcpld_write(BOOT_BOOT_CTLSTAT, val_t);

	/* save reboot reason */
	btcpld_read(BOOT_RST_REASON, &val_t);
	val_t &= 0xfc;
	btcpld_write(0x30, val_t);

	/* do reset */
	btcpld_write(BOOT_RST_CTL, BOOT_CPU_RESET);

}

int set_next_bootbank(flash_banks_t bank)
{
	uint8_t val_t;
	uint8_t select_bank;

	printf("Flash: Bank %d is active\n", bank);
	printf("switch bank[%d]->bank[%d]\n", gd->flash_bank, bank);
	btcpld_read(BOOT_BOOT_CTLSTAT, &val_t);

	if (bank == FLASH_BANK1) {
		val_t |= BOOT_SET_NEXT_PARTITION;
		btcpld_write(BOOT_BOOT_CTLSTAT, val_t);
	} else {
		val_t &= ~(BOOT_SET_NEXT_PARTITION);
		btcpld_write(BOOT_BOOT_CTLSTAT, val_t);
	}
	return 0;
}

flash_banks_t get_booted_bank(void)
{
	uint8_t val_t;

	btcpld_read(BOOT_BOOT_CTLSTAT, &val_t);
	if (val_t & BOOT_PARTITION) {
		return FLASH_BANK1;
	} else {
		return FLASH_BANK0;
	}
}

int valid_uboot_image(unsigned long addr)
{
	uint32_t icrc;
	image_header_t *haddr = (image_header_t *)(addr + IMG_HEADER_OFFSET);

	return 1;

	if ((icrc = crc32(0, (unsigned char *)(addr + IMG_DATA_OFFSET),
		haddr->ih_size)) == haddr->ih_dcrc) {
		printf("crc: 0x%x / 0x%x\n", icrc, haddr->ih_dcrc);
		return 1;
	}
	printf("crc: 0x%x / 0x%x\n", icrc, haddr->ih_dcrc);
	return 0;
}

int valid_env(unsigned long addr)
{
	ulong icrc;
	volatile uint32_t *flenv_crc = (volatile uint32_t *)addr;

	icrc = crc32(0, (addr + 0x04), (CONFIG_ENV_SIZE - 0x04));
	if (icrc != *flenv_crc) {
		printf("Invalid environment: Bad CRC\n");
		return 0;
	}
	return 1;
}

int set_upgrade_state(upgrade_state_t state)
{
	ulong upg_st_addr = (CONFIG_SYS_FLASH_BASE +
	    CFG_UPGRADE_BOOT_STATE_OFFSET);
	ulong ustate = state;
	int ret;

	ret = flash_write_direct(upg_st_addr, (char *)&ustate,
	    sizeof(ustate));

	return (ret);
}

uint32_t get_upgrade_state(void)
{
	uint32_t ustate_t;
	volatile uint32_t *ustate = (CONFIG_SYS_FLASH_BASE +
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

int flash_swizzled(void)
{
    uint8_t val_t;

    btcpld_read(BOOT_RST_REASON, &val_t);
    if (val_t & BOOT_FLASH_SWZL_RST) {
	/* flash swizzled */
	return (1);
    } else {
	return (0);
    }
}

int check_upgrade(void)
{
	flash_banks_t current_bank;

	gd->flash_bank = get_booted_bank();
	current_bank = gd->flash_bank;

	upgrade_state_t state = get_upgrade_state();


	if (flash_swizzled()) {
	    if (gd->flags & GD_FLG_RELOC) {
		if (current_bank == FLASH_BANK0) {
		    /* B1 failed to boot */
		    set_upgrade_state(UPGRADE_TRY_B1);
		} else {
		    /* Bank 1*/
		    /* B0 failed to boot */
		    set_upgrade_state(UPGRADE_TRY_B0);
		}

	    } else {
		/* 
		 * Flash swizzled and code is in Flash. Proceed with booting
		 * from the current bank. Current bank will be marked active
		 * when code runs from RAM.
		 */
		printf("flash swizzled...booting with bank[%d]\n", current_bank);
		return (0);
	    }
	}
	state = get_upgrade_state();

	/* code is in RAM */
	if (gd->flags & GD_FLG_RELOC) {
		printf("FLASH bank: %d\n", current_bank);
	}

	if ((state == UPGRADE_START_B0) &&
	    (gd->flags & GD_FLG_RELOC)) {
		if (current_bank == FLASH_BANK1) {
			enable_bank0();     
			if (valid_env(CONFIG_SYS_FLASH_BASE) &&
				valid_elf_image(CFG_UPGRADE_LOADER_BANK0) &&
				valid_uboot_image(CFG_UPGRADE_UBOOT_BANK0)) {
				set_upgrade_state(UPGRADE_CHECK);
			} else {
				/* upgrade not proper - upgrade failed */
				set_upgrade_state(INITIAL_STATE_B1);
			}
			disable_bank0();
		} else {
			if (valid_env(CONFIG_SYS_FLASH_BASE) &&
				valid_elf_image(CFG_UPGRADE_LOADER_BANK0) &&
				valid_uboot_image(CFG_UPGRADE_UBOOT_BANK0)) {
				set_upgrade_state(UPGRADE_TRY_B0);
			}
		}
	}
	if ((state == UPGRADE_START_B1) &&
		(gd->flags & GD_FLG_RELOC)) {
		if (current_bank == FLASH_BANK0) {
			if (valid_env(CONFIG_SYS_FLASH_BASE) &&
				valid_elf_image(CFG_UPGRADE_LOADER_BANK1) &&
				valid_uboot_image(CFG_UPGRADE_UBOOT_BANK1)) {
				set_upgrade_state(UPGRADE_CHECK);
			} else {
				/*upgrade not proper..failed*/
				set_upgrade_state(INITIAL_STATE_B1);
			}
		} else {
			/* 
			 * check bank1 image.
			 * BANK1 is also seen at BANK0 address by the CPU
			 */
			if (valid_env(CONFIG_SYS_FLASH_BASE) &&
				valid_elf_image(CFG_UPGRADE_LOADER_BANK0) &&
				valid_uboot_image(CFG_UPGRADE_UBOOT_BANK0)) {
				set_upgrade_state(UPGRADE_TRY_B1);
			}
		}
	}
	state = get_upgrade_state();
	/* upgrade: check the other bank */
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
			/* upgraded bank failed to boot */
			set_upgrade_state(INITIAL_STATE_B1);
		}
	}
	if (state == UPGRADE_TRY_B1) {
		if (current_bank != FLASH_BANK1) {
			/* upgraded bank failed to boot */
			set_upgrade_state(INITIAL_STATE_B0);
		}
	}
	if (state == UPGRADE_OK_B0) {
		if (current_bank != FLASH_BANK0) {
			set_next_bootbank(FLASH_BANK0);
			boot_next_bank();
		}
	}
	if (state == UPGRADE_OK_B1) {
		if (current_bank != FLASH_BANK1) {
			set_next_bootbank(FLASH_BANK1);
			boot_next_bank();
		}
	}
	return 0;
}

void ex62xx_btcpld_cpu_reset(void)
{
	btcpld_write(BOOT_RST_CTL, BOOT_CPU_RESET);
}

int write_i2cs_scratch(uint8_t val, uint8_t slot)
{
	uint8_t group, device, tdata[2];
	uint8_t offset;
	int ret;
	uint16_t assembly_id;

	group = CFG_EX6208_SCB_SLOT_BASE + slot;

	device = SCB_SLAVE_CPLD_ADDR;
	offset = I2CS_SEMAPHORE_REG_60;

	offset = ccb_i2cs_set_odd_parity(offset);
	tdata[0] = ccb_i2cs_set_odd_parity(val);
	ret = i2cs_write(group, device, offset, 1, tdata, 1);
	if (ret != 0) {
	    printf ("I2CS write @ 0x%x to scratch pad failed.\n", device);
	    goto ret_err;
	}
	return 0;

ret_err:
	return -1;
}

int read_i2cs_scratch(uint8_t* val, uint8_t slot)
{
	uint8_t group, device, tdata[1];
	uint8_t offset;
	int ret;
	uint16_t assembly_id;

	group = CFG_EX6208_SCB_SLOT_BASE + slot;

	device = SCB_SLAVE_CPLD_ADDR;
	offset = I2CS_SEMAPHORE_REG_60;

	offset = ccb_i2cs_set_odd_parity(offset);
	ret = i2cs_read(group, device, offset, 1, tdata, 1);
	if (ret != 0) {
	    printf ("I2CS write to device 0x%x failed.\n", device);
	    goto ret_err;
	} else {
	    *val = tdata[0];
	}
	return 0;

ret_err:
	return -1;
}

int set_re_state(uint8_t val)
{
	uint8_t slot;
	int ret_err;

	if (!EX62XX_SCB) {
		return 0;
	}

	slot = get_slot_id();
	ret_err = write_i2cs_scratch(val, slot);

	if (val == RE_STATE_BOOTING) {
		/* deselct i2c mux */
		ccb_iic_ctlr_group_select(JCB_I2C_DESELECT_I2C_MUX);
	}

	return ret_err;
}

int get_ore_state(uint8_t* val)
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

int get_bp_eeprom_access(void)
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
 * Disable flash swizzle in boot CPLD.
 */
void btcpld_swizzle_disable(void)
{
	uint8_t val_t;

	btcpld_read(BOOT_BOOT_CTLSTAT, &val_t);
	val_t |= BOOT_OK; 
	btcpld_read(BOOT_BOOT_CTLSTAT, val_t);
}

void disable_bank0_atprompt(void)
{
	uint8_t val_t;

	if (gd->flash_bank == FLASH_BANK1) {
		btcpld_read(BOOT_BOOT_CTLSTAT, &val_t);
		val_t &= ~(BOOT_FLASH_UPGRADE);
		btcpld_write(BOOT_BOOT_CTLSTAT, val_t);
	}
}

int board_eth_init(bd_t *bis)
{
	struct tsec_info_struct tsec_info[4];
	volatile ccsr_gur_t *gur = (void *)(CONFIG_SYS_MPC85xx_GUTS_ADDR);
	int num = 0;

	if (EX62XX_LC) {
#ifdef CONFIG_TSEC2
	    SET_STD_TSECV2_INFO(tsec_info[num], 2);
	    if (!(gur->pordevsr & MPC85xx_PORDEVSR_SGMII2_DIS))
		tsec_info[num].flags |= TSEC_SGMII;
	    num++;
#endif
#ifdef CONFIG_TSEC3
	    SET_STD_TSECV2_INFO(tsec_info[num], 3);
	    if (!(gur->pordevsr & MPC85xx_PORDEVSR_SGMII3_DIS))
		tsec_info[num].flags |= TSEC_SGMII;
	    num++;
#endif
	} else {
#ifdef CONFIG_TSEC1
	SET_STD_TSEC_INFO(tsec_info[num], 1);
	num++;
#endif
#ifdef CONFIG_TSEC2
	SET_STD_TSEC_INFO(tsec_info[num], 2);
	if (!(gur->pordevsr & MPC85xx_PORDEVSR_SGMII2_DIS))
		tsec_info[num].flags |= TSEC_SGMII;
	num++;
#endif
#ifdef CONFIG_TSEC3
	SET_STD_TSEC_INFO(tsec_info[num], 3);
	if (!(gur->pordevsr & MPC85xx_PORDEVSR_SGMII3_DIS))
		tsec_info[num].flags |= TSEC_SGMII;
	num++;
#endif
	}

	if (!num) {
		printf("No TSECs initialized\n");

		return 0;
	}

	tsec_eth_init(bis, tsec_info, num);

	return (num);
}

void board_reset(void)
{
    ex62xx_btcpld_cpu_reset();

    /* reset failed */
    printf("*** resetting board failed...\n");
    while(1);
}

int board_early_init_r(void)
{
    /* add any early init_r functions */

    return (0);
}

#define FLASH_CMD_PROTECT_SET      0x01
#define FLASH_CMD_PROTECT_CLEAR    0xD0
#define FLASH_CMD_CLEAR_STATUS     0x50
#define FLASH_CMD_PROTECT          0x60

#define AMD_CMD_RESET              0xF0
#define AMD_CMD_WRITE              0xA0
#define AMD_CMD_ERASE_START        0x80
#define AMD_CMD_ERASE_SECTOR       0x30
#define AMD_CMD_UNLOCK_START       0xAA
#define AMD_CMD_UNLOCK_ACK         0x55

#define AMD_STATUS_TOGGLE          0x40

/* simple flash 16 bits write, do not accross sector */
int flash_write_direct(ulong addr, char* src, ulong cnt)
{
	volatile uint8_t* faddr = (uint8_t*)addr;
	volatile uint8_t* faddrs = (uint8_t*)addr;
	volatile uint8_t* faddr1 = (uint8_t*)((addr & 0xFFFFF000) | 0xAAA);
	volatile uint8_t* faddr2 = (uint8_t*)((addr & 0xFFFFF000) | 0x554);
	uint8_t* data = (uint8_t*)src;
	ulong start, saddr;
	int i;

	if ((addr >= 0xff800000) && (addr < 0xff80ffff)) {          /* 8k sector */
		saddr = addr & 0xffffe000;
	} else if ((addr >= 0xff810000) && (addr < 0xffefffff)) {   /* 64k sector */
		saddr = addr & 0xffff0000;
	} else if ((addr >= 0xfff00000) && (addr < 0xffffffff)) {   /* 8k sector */
		saddr = addr & 0xffffe000;
	} else {
		printf("flash address 0x%x out of range 0xFF800000 - 0xFFFFFFFF\n",
			   addr);
		return 1;
	}
	faddrs = (uint8_t*)saddr;
	faddr1 = (uint8_t*)((saddr & 0xFFFFF000) | 0xAAA);
	faddr2 = (uint8_t*)((saddr & 0xFFFFF000) | 0x554);

	/* un-protect sector */
	*faddrs = FLASH_CMD_CLEAR_STATUS;
	*faddrs = FLASH_CMD_PROTECT;
	*faddrs = FLASH_CMD_PROTECT_CLEAR;
	start = get_timer(0);
	while ((*faddrs & AMD_STATUS_TOGGLE) != (*faddrs & AMD_STATUS_TOGGLE)) {
		if (get_timer(start) > 0x2000) {
			*faddrs = AMD_CMD_RESET;
			return 1;
		}
		udelay(1);
	}

	/* erase sector */
	*faddr1 = AMD_CMD_UNLOCK_START;
	*faddr2 = AMD_CMD_UNLOCK_ACK;
	*faddr1 = AMD_CMD_ERASE_START;
	*faddr1 = AMD_CMD_UNLOCK_START;
	*faddr2 = AMD_CMD_UNLOCK_ACK;
	*faddrs = AMD_CMD_ERASE_SECTOR;
	start = get_timer(0);
	while ((*faddrs & AMD_STATUS_TOGGLE) != (*faddrs & AMD_STATUS_TOGGLE)) {
		if (get_timer(start) > 0x2000) {
			*faddrs = AMD_CMD_RESET;
			return 1;
		}
		udelay(1);
	}

	for (i = 0; i < cnt; i++) {
		/* write data byte */
		*faddr1 = AMD_CMD_UNLOCK_START;
		*faddr2 = AMD_CMD_UNLOCK_ACK;
		*faddr1 = AMD_CMD_WRITE;

		*(faddr + i) = data[i];
		start = get_timer(0);
		while ((*faddrs & AMD_STATUS_TOGGLE) !=
			   (*faddrs & AMD_STATUS_TOGGLE)) {
			if (get_timer(start) > 0x100) {
				*faddrs = AMD_CMD_RESET;
				return 1;
			}
			udelay(1);
		}
	}

	/* protect sector */
	*faddrs = FLASH_CMD_CLEAR_STATUS;
	*faddrs = FLASH_CMD_PROTECT;
	*faddrs = FLASH_CMD_PROTECT_SET;
	start = get_timer(0);
	while ((*faddrs & AMD_STATUS_TOGGLE) != (*faddrs & AMD_STATUS_TOGGLE)) {
		if (get_timer(start) > 0x2000) {
			*faddrs = AMD_CMD_RESET;
			return 1;
		}
		udelay(1);
	}

	return 0;
}

#ifdef CONFIG_SILENT_CONSOLE
void disable_console_out(void)
{
    /* silent console */
    gd->flags |= GD_FLG_SILENT;
}

void enable_console_out(void)
{
    gd->flags &= ~GD_FLG_SILENT;
}
#endif

/*********************************************************/
