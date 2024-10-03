/*
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
#include <config.h>
#include <asm/types.h>
#include <asm/fsl_i2c.h>
#include <asm/io.h>
#include <i2c.h>
#include "cmd_ex43xx.h"
#include "epld.h"
#include "epld_watchdog.h"
#include "led.h"
#include "lcd.h"
#include <tsec.h>
#include <net.h>
#include <pci.h>
#include <asm/processor.h>
#include <asm/immap_85xx.h>
#include <asm/fsl_ddr_sdram.h>
#include <asm/mmu.h>
#include <asm/fsl_law.h>
#include <spd.h>
#include <spd_sdram.h>
#include <usb.h>
#include "ex43xx_local.h"
#include <asm/fsl_pci.h>
#include <asm/fsl_serdes.h>
#include <fm_eth.h>
#include <fsl_mdio.h>
#include <asm/fsl_dtsec.h>
#if defined(CONFIG_POST)
#include <post.h>
#endif

#define	EX43XX_DEBUG
#undef debug
#ifdef	EX43XX_DEBUG
#define debug(fmt,args...)	printf(fmt ,##args)
#else
#define debug(fmt,args...)
#endif

DECLARE_GLOBAL_DATA_PTR;

extern phys_size_t fixed_sdram(void);
extern void ddr_enable_ecc(unsigned int);
extern uint read_phy_reg_private(struct tsec_private *priv, uint regnum);
extern void write_phy_reg_private(struct tsec_private *priv, uint regnum, uint value);
int secure_eeprom_read(unsigned dev_addr, unsigned offset,
    uchar *buffer, unsigned cnt);

struct pci_info {
    u16     cfg;
};

/* The cfg field is a bit mask in which each bit represents the value of
 * cfg_IO_ports[] signal and the bit is set if the interface would be
 * enabled based on the value of cfg_IO_ports[] signal
 *
 * On MPC86xx/PQ3 based systems:
 *   we extract cfg_IO_ports from GUTS register PORDEVSR
 *
 * cfg_IO_ports only exist on systems w/PCIe (we set cfg 0 for systems
 * without PCIe)
 */

#if defined(CONFIG_MPC8540) || defined(CONFIG_MPC8560)
static struct pci_info pci_config_info[] =
{
    [LAW_TRGT_IF_PCI] = {
	.cfg =   0,
    },
};
#elif defined(CONFIG_MPC8541) || defined(CONFIG_MPC8555)
static struct pci_info pci_config_info[] =
{
    [LAW_TRGT_IF_PCI] = {
	.cfg =   0,
    },
};
#elif defined(CONFIG_MPC8536)
static struct pci_info pci_config_info[] =
{
    [LAW_TRGT_IF_PCI] = {
	.cfg =   0,
    },
    [LAW_TRGT_IF_PCIE_1] = {
	.cfg =   (1 << 2) | (1 << 3) | (1 << 5) | (1 << 7),
    },
    [LAW_TRGT_IF_PCIE_2] = {
	.cfg =   (1 << 5) | (1 << 7),
    },
    [LAW_TRGT_IF_PCIE_3] = {
	.cfg =   (1 << 7),
    },
};
#elif defined(CONFIG_MPC8544)
static struct pci_info pci_config_info[] =
{
    [LAW_TRGT_IF_PCI] = {
	.cfg =   0,
    },
    [LAW_TRGT_IF_PCIE_1] = {
	.cfg =   (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) |
		 (1 << 6) | (1 << 7),
    },
    [LAW_TRGT_IF_PCIE_2] = {
	.cfg =   (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7),
    },
    [LAW_TRGT_IF_PCIE_3] = {
	.cfg =   (1 << 6) | (1 << 7),
    },
};
#elif defined(CONFIG_MPC8548)
static struct pci_info pci_config_info[] =
{
    [LAW_TRGT_IF_PCI_1] = {
	.cfg =   0,
    },
    [LAW_TRGT_IF_PCI_2] = {
	.cfg =   0,
    },
    /* PCI_2 is always host and we dont use iosel to determine enable/disable */
    [LAW_TRGT_IF_PCIE_1] = {
	.cfg =   (1 << 3) | (1 << 4) | (1 << 7),
    },
};
#elif defined(CONFIG_MPC8568)
static struct pci_info pci_config_info[] =
{
    [LAW_TRGT_IF_PCI] = {
	.cfg =   0,
    },
    [LAW_TRGT_IF_PCIE_1] = {
	.cfg =   (1 << 3) | (1 << 4) | (1 << 7),
    },
};
#elif defined(CONFIG_MPC8569)
static struct pci_info pci_config_info[] =
{
    [LAW_TRGT_IF_PCIE_1] = {
	.cfg =   (1 << 0) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) |
		 (1 << 8) | (1 << 0xc) | (1 << 0xf),
    },
};
#elif defined(CONFIG_MPC8572)
static struct pci_info pci_config_info[] =
{
    [LAW_TRGT_IF_PCIE_1] = {
	.cfg =   (1 << 2) | (1 << 3) | (1 << 7) |
		 (1 << 0xb) | (1 << 0xc) | (1 << 0xf),
    },
    [LAW_TRGT_IF_PCIE_2] = {
	.cfg =   (1 << 3) | (1 << 7),
    },
    [LAW_TRGT_IF_PCIE_3] = {
	.cfg =   (1 << 7),
    },
};
#elif defined(CONFIG_MPC8610)
static struct pci_info pci_config_info[] =
{
    [LAW_TRGT_IF_PCI_1] = {
	.cfg =   0,
    },
    [LAW_TRGT_IF_PCIE_1] = {
	.cfg =   (1 << 1) | (1 << 4),
    },
    [LAW_TRGT_IF_PCIE_2] = {
	.cfg =   (1 << 0) | (1 << 4),
    },
};
#elif defined(CONFIG_MPC8641)
static struct pci_info pci_config_info[] =
{
    [LAW_TRGT_IF_PCIE_1] = {
	.cfg =   (1 << 2) | (1 << 3) | (1 << 5) | (1 << 6) |
		 (1 << 7) | (1 << 0xe) | (1 << 0xf),
    },
};
#elif defined(CONFIG_P1011) || defined(CONFIG_P1020)
static struct pci_info pci_config_info[] =
{
    [LAW_TRGT_IF_PCIE_1] = {
	.cfg =   (1 << 0) | (1 << 6) | (1 << 0xe) | (1 << 0xf),
    },
    [LAW_TRGT_IF_PCIE_2] = {
	.cfg =   (1 << 0xe),
    },
};
#elif defined(CONFIG_P2010) || defined(CONFIG_P2020)
static struct pci_info pci_config_info[] =
{
    [LAW_TRGT_IF_PCIE_1] = {
	.cfg =   (1 << 0) | (1 << 2) | (1 << 4) | (1 << 6) |
		 (1 << 0xd) | (1 << 0xe) | (1 << 0xf),
    },
    [LAW_TRGT_IF_PCIE_2] = {
	.cfg =   (1 << 2) | (1 << 0xe),
    },
    [LAW_TRGT_IF_PCIE_3] = {
	.cfg =   (1 << 2) | (1 << 4),
    },
};
#elif defined(CONFIG_FSL_CORENET)
#else
#error Need to define pci_config_info for processor
#endif

extern int usb_stor_curr_dev; /* current device */
extern int i2c_mfgcfg;
extern int i2c_manual;

int post_result_cpu = 0;
int post_result_mem = 0;
int post_result_rom = 0;
int i2c_post = 0;

int usb_scan_dev = 0;
int post_result_ether = 0;
int post_result_pci = 0;
static struct tsec_private *mgmt_priv = NULL;
static int mdio_inuse = 0;
static uint8_t curmode = 0xff;

static uint16_t version_epld(void);

int
console_silent_init (void)
{

#ifdef CONFIG_SILENT_CONSOLE
    if (getenv("silent") != NULL)
	gd->flags |= GD_FLG_SILENT;
#endif

    return 0;
}

flash_banks_t
get_booted_bank (void)
{
    uint16_t val_t;

    epld_reg_read(EPLD_BOOT_CTL, &val_t);
    if (val_t & EPLD_CURR_PARTITION) {
	return FLASH_BANK1;
    } else {
	return FLASH_BANK0;
    }
}

int
flash_swizzled (void)
{
    uint16_t val_t;

    epld_reg_read(EPLD_BOOT_CTL, &val_t);
    if (val_t & EPLD_BOOT_FAILURE) {
	/* flash swizzled */
	return 1;
    } else {
	return 0;
    }

    return 0;
}

int
set_upgrade_state (upgrade_state_t state)
{
    ulong upgrade_addr = (CONFIG_SYS_FLASH_BASE +
			  CFG_UPGRADE_BOOT_STATE_OFFSET);
    ulong ustate = state;
    int ret;

    ret = flash_write_direct(upgrade_addr, (char *)&ustate, sizeof(ulong));
    return ret;
}

uint32_t
get_upgrade_state (void)
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

int
valid_env (unsigned long addr)
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

int
valid_uboot_image (unsigned long addr)
{
    uint32_t icrc;
    image_header_t *haddr = (image_header_t *)(addr + IMG_HEADER_OFFSET);

    if ((icrc = crc32(0, (unsigned char *)(addr + IMG_DATA_OFFSET),
	    haddr->ih_size)) == haddr->ih_dcrc) {
	printf("crc: 0x%x / 0x%x\n", icrc, haddr->ih_dcrc);
	return 1;
    }
    printf("crc: 0x%x / 0x%x\n", icrc, haddr->ih_dcrc);
    return 0;
}

/* Set bank0 as current partition */
int
set_curr_bank0 (void)
{
    uint16_t val_t;

    epld_reg_read(EPLD_BOOT_CTL, &val_t);
    val_t &= ~(EPLD_CURR_PARTITION);
    epld_reg_write(EPLD_BOOT_CTL, val_t);

    return 0;
}

/* Set bank1 as current partition */
int
set_curr_bank1 (void)
{
    uint16_t val_t;

    epld_reg_read(EPLD_BOOT_CTL, &val_t);
    val_t |= EPLD_CURR_PARTITION;
    epld_reg_write(EPLD_BOOT_CTL, val_t);

    return 0;
}

void
boot_next_bank (void)
{
    uint16_t val_t;

    /* restart timer on reset */
    epld_reg_read(EPLD_BOOT_CTL, &val_t);
    val_t &= ~(EPLD_BOOT_OK);
    epld_reg_write(EPLD_BOOT_CTL, val_t);

    /* do reset */
    epld_reg_write(EPLD_CPU_RESET, EPLD_CPU_HARD_RESET);

}

int
set_next_bootbank (flash_banks_t bank)
{
    printf("Flash: Bank %d is active\n", bank);
    printf("switch bank[%d]->bank[%d]\n", gd->flash_bank, bank);

    if (bank == FLASH_BANK1) {
	set_curr_bank1();
    } else {
	set_curr_bank0();
    }
    return 0;
}

/*
 * As per the CPLD swizzle implementation on EX
 * platforms, by default when we are booted from
 * bank1, only the lower 4Mb of flash is 
 * visible/accessible.
 * However, there are situations where software
 * need to access upper 4Mb flash as well. One
 * such situation is, in case of P2041 processors,
 * we have RCW stored at beginning of the flash (that
 * is upper 4Mb region). The other case is, when we try
 * reading/writing to the U-Boot env area.
 * Provide following utility functions:
 *     (a) enable_both_banks
 *     (b) disable_bank1_access
 * for software to expose/disable the whole flash access
 * respectively.
 *
 *    +---------+ 0xff800000
 *    |         |
 *    |  bank1  |
 *    |         |
 *    +---------+ 0xffc00000
 *    |         |
 *    |  bank0  |
 *    |         |
 *    +---------+ 0xffffffff
 *
 */

int
enable_both_banks (void)
{
    uint16_t val_t;

    epld_reg_read(EPLD_BOOT_CTL, &val_t);
    val_t |= EPLD_ENTIRE_FLASH_ACCESS;
    epld_reg_write(EPLD_BOOT_CTL, val_t);

    return 0;
}

int
enable_single_bank_access (void)
{
    uint16_t val_t;

    epld_reg_read(EPLD_BOOT_CTL, &val_t);
    val_t &= ~(EPLD_ENTIRE_FLASH_ACCESS);
    epld_reg_write(EPLD_BOOT_CTL, val_t);

    return 0;
}

/*
 * The flash present on EX4300 deos not support parallel read/writes,
 * hence we need to do flash writes only after relocating to DDR and
 * uboot is executing from flash.
 */
int
check_upgrade (void)
{
    flash_banks_t current_bank;

    gd->flash_bank = get_booted_bank();
    current_bank = gd->flash_bank;

    if (current_bank == FLASH_BANK0) {
	/* Expose the entire flash so that from bank0 we can write to the
	 * U-Boot env.
	 */
	enable_both_banks();
    }

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
	    printf("Flash swizzled...booting with bank[%d]\n", current_bank);
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
	    set_curr_bank0();
	    if (valid_env(CONFIG_SYS_FLASH_BASE) &&
		valid_elf_image(CFG_UPGRADE_LOADER_BANK0) &&
		valid_uboot_image(CFG_UPGRADE_UBOOT_BANK0)) {
		set_upgrade_state(UPGRADE_CHECK);
	    } else {
		/* upgrade not proper - upgrade failed */
		set_upgrade_state(INITIAL_STATE_B1);
	    }
	    set_curr_bank1();
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
		/*upgrade not proper..failed */
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

int
set_upgrade_ok (void)
{
    char temp[32];
    flash_banks_t current_bank;
    upgrade_state_t ustate;
    uint32_t set_state, state;
    uint16_t val_t;

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

    gd->flags |= GD_FLG_SILENT;
    saveenv();
    gd->flags &= ~GD_FLG_SILENT;

    epld_reg_read(EPLD_BOOT_CTL, &val_t);
    val_t |= EPLD_BOOT_OK;
    epld_reg_write(EPLD_BOOT_CTL, val_t);

    return 0;
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

static uint16_t
version_epld (void)
{
    volatile uint16_t ver;

    epld_reg_read(EPLD_WATCHDOG_SW_UPDATE, (uint16_t *)&ver);
    return ver;
}

static int
mem_ecc (void)
{
    return 1;
}

int
board_early_init_f (void)
{
    volatile ccsr_gpio_t *gpio = (void *)(CONFIG_SYS_MPC85xx_GPIO_ADDR);
    ccsr_gur_t *gur = (ccsr_gur_t *)(CONFIG_SYS_MPC85xx_GUTS_ADDR);
    volatile uint16_t epldMiscCtrl;
    volatile uint16_t epldSysReset;

    /* board only uses the DDR_MCK0/1, so disable the DDR_MCK2/3 */
    setbits_be32(&gur->ddrclkdr, 0x000f000f);

    /* gpio resets */

    /*
     * gpio pins 0, 2, 3, 16, and 17 are used as resets
     * and these signals should be configured as output
     */
    setbits_be32(&gpio->gpdir, 0xb000c000);
    /*
     * GPIO 0 -- uplink module reset
     * GPIO 2 -- PFE 56648J reset
     * GPIO 3 -- MGMT PHY 54616S reset
     * GPIO 16 -- i2c1 reset
     * GPIO 17 -- i2c2 reset
     */
    clrsetbits_be32(&gpio->gpdat, 0x00000000, 0xb000c000);

    /* syspld resets */

    /* R5H30211 de-assert */
    epld_reg_read(EPLD_SYS_LED, (uint16_t *)&epldMiscCtrl);
    epldMiscCtrl |= EPLD_R5H30211_RST;
    epld_reg_write(EPLD_SYS_LED, epldMiscCtrl);

    /* De-assert USB CONS, POE, and Front module I2C */
    epld_reg_read(EPLD_SYSTEM_RESET, (uint16_t *)&epldSysReset);
    epldSysReset &= ~(EPLD_USB_CONS_RST | EPLD_POE_RST |
		      EPLD_FMOD_I2C_RST);
    epld_reg_write(EPLD_SYSTEM_RESET, epldSysReset);

    return 0;
}

int
board_eth_init(bd_t *bis)
{
#ifdef CONFIG_FMAN_ENET
    struct fsl_pq_mdio_info dtsec_mdio_info;
    unsigned int i, slot;
    int lane;

    printf("Initializing Fman\n");

    dtsec_mdio_info.regs =
      (struct tsec_mii_mng *)CONFIG_SYS_FM1_DTSEC1_MDIO_ADDR;
    dtsec_mdio_info.name = DEFAULT_FM_MDIO_NAME;

    /* Register the real 1G MDIO bus */
    fsl_pq_mdio_init(bis, &dtsec_mdio_info);

    /* Program the three on-board SGMII PHY addresses. */
    fm_info_set_phy_address(FM1_DTSEC1, CONFIG_SYS_FM1_DTSEC1_PHY_ADDR);

    fm_info_set_mdio(FM1_DTSEC1, miiphy_get_dev_by_name(DEFAULT_FM_MDIO_NAME));

    cpu_eth_init(bis);
#endif
}

int
board_early_init_r (void)
{
    const u8 flash_esel = find_tlb_idx((void *)CONFIG_SYS_FLASH_BASE, 1);

    /* Remap boot flash to cache inhibited so that flash
     * can be erased properly.
     */

    /* Flush d-cache and invalidate i-cache of any FLASH data */
    flush_dcache();
    invalidate_icache();

    /* invalidate existing TLB entry for flash */
    disable_tlb(flash_esel);

    set_tlb(1, CONFIG_SYS_FLASH_BASE, CONFIG_SYS_FLASH_BASE_PHYS,
	MAS3_SX|MAS3_SW|MAS3_SR, MAS2_I|MAS2_G,
	0, flash_esel, BOOKE_PAGESZ_16M, 1);

    usb_scan_dev = 0;
    return 0;
}

int
checkboard (void)
{
    volatile uint16_t epld_ver, epldLastRst;
    unsigned char ch = 0x10, mcfg;
    uint32_t id, bid = 0;
    int ret_i2c = 0;
    ccsr_gur_t *gur = (ccsr_gur_t *)CONFIG_SYS_MPC85xx_GUTS_ADDR;
    int i;

    /*
     * Display the RCW, so that no one gets confused as to what RCW
     * we're actually using for this boot.
     */
    puts("Reset Configuration Word (RCW):");
    for (i = 0; i < ARRAY_SIZE(gur->rcwsr); i++) {
	u32 rcw = in_be32(&gur->rcwsr[i]);

	if ((i % 4) == 0)
	    printf("\n       %08x:", i * 4);
	printf(" %08x", rcw);
    }
    puts("\n");

    i2c_init (CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
    i2c_set_bus_num(CFG_I2C_CTRL_1);

    /* read assembly ID from main board EEPROM offset 0x4 */
    /* select i2c controller 1, mux channel 4 */
    ch = 0x10;
    ret_i2c = i2c_write(CFG_I2C_C1_9548SW1, 0, 0, &ch, 1);

    gd->secure_eeprom = 0;
    if (i2c_read_norsta(CFG_I2C_C1_9548SW1_P4_R5H30211, 4, 1,
			(uint8_t *)&id, 4) == 0) {
	i2c_read_norsta(CFG_I2C_C1_9548SW1_P4_R5H30211, 0x72, 1, (uint8_t *)&mcfg, 1);
	if (id != 0xFFFFFFFF)
	gd->secure_eeprom = 1;
    }

    ch = 0;
    i2c_write(CFG_I2C_C1_9548SW1, 0, 0, &ch, 1); /* disable mux channel */
    gd->mem_cfg = mcfg;
    bid = (id & 0xffff0000) >> 16;

    /* select i2c controller 1 */
    i2c_set_bus_num(CFG_I2C_CTRL_1);

    gd->valid_bid = 1;
    gd->board_type = id;

    switch (board_id()) {
    case I2C_ID_JUNIPER_EX4300_24T:
	puts("Board: EX4300-24T");
	break;
    case I2C_ID_JUNIPER_EX4300_48T:
	puts("Board: EX4300-48T");
	break;
    case I2C_ID_JUNIPER_EX4300_24_P:
	puts("Board: EX4300-24P");
	break;
    case I2C_ID_JUNIPER_EX4300_48_P:
	puts("Board: EX4300-48P");
	break;
    case I2C_ID_JUNIPER_EX4300_48_T_BF:
	puts("Board: EX4300-48T-BF");
	break;
    case I2C_ID_JUNIPER_EX4300_48_T_DC:
	puts("Board: EX4300-48T-DC");
	break;
    case I2C_ID_JUNIPER_EX4300_32F:
        puts("Board: EX4300-32F");
        break;
    default:
	puts("Board: Unknown");
	break;
    }

    printf(" %d.%d\n", version_major(), version_minor());
    epld_ver = version_epld();
    epld_reg_read(EPLD_LAST_RESET, (uint16_t *)&epldLastRst);
    printf("EPLD:  Version %d.%d (0x%02x)\n", (epld_ver & 0xff00) >> 8,
	   (epld_ver & 0x00C0) >> 6, epldLastRst & 0xFF);
    epld_reg_write(EPLD_LAST_RESET, 0);
    gd->last_reset = epldLastRst & 0xFF;

    return 0;
}

phys_size_t
initdram (int board_type)
{
    phys_size_t dram_size = 0;

    puts("Initializing\n");

    dram_size = fsl_ddr_sdram();
    dram_size = setup_ddr_tlbs(dram_size / 0x100000);
    dram_size *= 0x100000;

    puts("    DDR: ");
    return dram_size;
}

#if defined(CONFIG_SYS_DRAM_TEST)
int
testdram (void)
{
    uint *pstart = (uint *) CONFIG_SYS_MEMTEST_START;
    uint *pend = (uint *) CONFIG_SYS_MEMTEST_END;
    uint *p;

    printf("Testing DRAM from 0x%08x to 0x%08x\n",
	    CONFIG_SYS_MEMTEST_START,CONFIG_SYS_MEMTEST_END);

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
/* Initialize PCI Devices, report devices found */
#ifndef CONFIG_PCI_PNP
static struct pci_config_table pci_java_config_table[] = {
    { PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID,
      PCI_IDSEL_NUMBER, PCI_ANY_ID,
      pci_cfgfunc_config_device, {PCI_ENET0_IOADDR,
      PCI_ENET0_MEMADDR,
      PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER }
    },
    { }
};
#endif

#ifndef CONFIG_FSL_CORENET
int
is_fsl_pci_cfg (enum law_trgt_if trgt, u32 io_sel)
{
    return ((1 << io_sel) & pci_config_info[trgt].cfg);
}
#endif

struct pci_controller hose = {
#ifndef CONFIG_PCI_PNP
    config_table: pci_java_config_table,
#endif
};
#endif	/* CONFIG_PCI */

#ifdef CONFIG_PCIE1
struct pci_controller pcie1_hose[1];
#endif

static struct pci_controller pci1_hose;
void
pci_init_board (void)
{
    fsl_pcie_init_board(0);
}

void
set_console_env (void)
{
    if (gd->console == 2)
	setenv("hw.uart.console", "mm:0xfe11c600");
    else
	setenv("hw.uart.console", "mm:0xfe11c500");
}

int
misc_init_r (void)
{
    int i;
    volatile uint16_t fanCtrl;

    for (i = 0; i < CFG_LCD_NUM; i++)
	lcd_init(i);

    /* EPLD */
    epld_watchdog_init();

    /* firmware version */
    setenv("boot.firmware.verstr", version_string);

    set_console_env();

    /* Print the juniper internal version of the u-boot */
    printf("\nFirmware Version: %02X.%02X.%02X\n",
	   CFG_VERSION_MAJOR, CFG_VERSION_MINOR, CFG_VERSION_PATCH);

    epld_reg_read(EPLD_FAN_CTRL_0_1, (uint16_t *)&fanCtrl);

    /*
     * Writing 0xa to bits (15:12) and (7:4) to cpld register at offset 0x78
     * this will reduce fan speed (to 0xa) during uboot phase thereby reducing
     * the noise level during bootup (low level of acoustics noise given that
     * the fan is present)
     */
    if (fanCtrl & FAN_0_PRNT) {
	fanCtrl = fanCtrl | FAN_0_LED_ON;
	fanCtrl = fanCtrl & FAN_0_MASK_BITS;
	fanCtrl = fanCtrl | FAN_0_DEFAULT_SPEED;
    }

    if (fanCtrl & FAN_1_PRNT) {
	fanCtrl = fanCtrl | FAN_1_LED_ON;
	fanCtrl = fanCtrl & FAN_1_MASK_BITS;
	fanCtrl = fanCtrl | FAN_1_DEFAULT_SPEED;
    }

    epld_reg_write(EPLD_FAN_CTRL_0_1, fanCtrl);

    return 0;
}

int
board_start_usb (void)
{
    usb_multi_root_hub = 1;
    usb_num_root_hub = 2;

    return 1;
}

int
last_stage_init (void)
{

    /*
     * NOTE: In case rtc has to be inited, do it here.
     * As of now we are not initializing rtc under u-boot.
     */
    return 0;
}

/* retrieve MAC address from main board EEPROM */
void
board_get_enetaddr (uchar * enet)
{
    unsigned char ch = 1 << 4;  /* channel 4 */
    char temp[60], tmp;
    char *s;
    uint8_t esize = 0x40;
    uint16_t id;
    bd_t *bd = gd->bd;
    int mloop = 1;
    int i;
#if defined(CONFIG_EX4300)
    int state = get_upgrade_state();
#endif
    /* environment variable for board type */
    id = board_id();

    i2c_set_bus_num(CFG_I2C_CTRL_1);
    i2c_write(CFG_I2C_C1_9548SW1, 0, 0, &ch, 1);

    enet[0] = 0x00;
    enet[1] = 0xE0;
    enet[2] = 0x0C;
    enet[3] = 0x00;
    enet[4] = 0x00;
    enet[5] = 0x00;

    if (gd->secure_eeprom) {
	secure_eeprom_read(CFG_I2C_C1_9548SW1_P4_R5H30211,
	    CFG_EEPROM_MAC_OFFSET, enet, 6);
	secure_eeprom_read(CFG_I2C_C1_9548SW1_P4_R5H30211,
	    CFG_EEPROM_MAC_OFFSET-1, &esize, 1);
    }

    ch = 0;
    i2c_write(CFG_I2C_C1_9548SW1, 0, 0, &ch, 1);
    /*
     * On EX4300, FM1@DTSEC1 is the management interface which is in RGMII mode.
     * The rest 2 interfaces FM1@DTSEC2 and FM1@DTSEC3 are not used currently and are
     * connected to the uplink modules.
     */
    if (esize == 0x40) {
	enet[5] += (esize - 1);
    }
    tmp = enet[5];
    sprintf(temp, "%02x:%02x:%02x:%02x:%02x:%02x",
	enet[0], enet[1], enet[2], enet[3], enet[4], tmp);
    s = getenv("ethaddr");
    if (memcmp(s, temp, sizeof(temp)) != 0) {
	setenv("ethaddr", temp);
    }

    for (i = 0; i < 5; ++i) {
	bd->bi_enetaddr[i] = enet[i];
    }

    bd->bi_enetaddr[5] = tmp;

    /*
     * In case the board id is not populated or
     * read incorrectly, try to set it to default
     * board type
     */
    if (id == 0xFFFF)
	id = I2C_ID_JUNIPER_EX4300_24T;

    s = getenv("ethprime");
    if (!s)
	setenv("ethprime", CONFIG_ETHPRIME);

    sprintf(temp,"%04x ", id);
    setenv("hw.board.type", temp);

    /* environment variable for last reset */
    sprintf(temp,"%02x ", gd->last_reset);
    setenv("hw.board.reset", temp);

#if defined(CONFIG_EX4300)
    sprintf(temp,"%d ", state);
    setenv("boot.state", temp);
#endif

#if defined(CONFIG_UPGRADE)
    s = (char *) (CFG_UPGRADE_BOOT_IMG_ADDR + 4);
    setenv("boot.upgrade.ver", s);
    s = (char *) (CFG_READONLY_BOOT_IMG_ADDR + 4);
    setenv("boot.readonly.ver", s);
#endif

    /* set the internal uboot version */
    sprintf(temp, " %d.%d.%d",
	CFG_VERSION_MAJOR, CFG_VERSION_MINOR, CFG_VERSION_PATCH);
    setenv("boot.intrver", temp);

    if ((s = getenv("boot.test")) != NULL) {
	mloop = simple_strtoul(s, NULL, 16);

	/*
	 * Since we relocate to the top 1GB,
	 * memtest should test only the possible
	 * ranges which u-boot will not use
	 */
	if (memtest(0x2000, CONFIG_MEM_RELOCATE_TOP - 0x1000000, mloop, 1)) {
	    setenv ("boot.ram", "FAILED ");
	} else {
	    /*
	     * If the first region succeeds then
	     * go ahead with the second 1GB range
	     */
	    if (memtest(CONFIG_MEM_RELOCATE_TOP + 0x2000,
		  bd->bi_memsize - 0x2000, mloop, 1)) {
		setenv ("boot.ram", "FAILED ");
	    } else {
		setenv ("boot.ram", "PASSED ");
	    }
	}
    }

    /* uboot/loader memory map */
    sprintf(temp,"0x%08x ", CONFIG_SYS_FLASH_BASE);
    setenv("boot.flash.start", temp);

    sprintf(temp,"0x%08x ", CFG_FLASH_SIZE);
    setenv("boot.flash.size", temp);

    sprintf(temp,"0x%08x ", CFG_ENV_OFFSET);
    setenv("boot.env.start", temp);

    sprintf(temp,"0x%08x ", CFG_ENV_SIZE);
    setenv("boot.env.size", temp);

    sprintf(temp,"0x%08x ", CFG_OPQ_ENV_OFFSET);
    setenv("boot.opqenv.start", temp);

    sprintf(temp,"0x%08x ", CFG_OPQ_ENV_SECT_SIZE);
    setenv("boot.opqenv.size", temp);

    sprintf(temp,"0x%08x ", CFG_UPGRADE_BOOT_STATE_OFFSET);
    setenv("boot.upgrade.state", temp);

    sprintf(temp,"0x%08x ", CFG_UPGRADE_LOADER_BANK1 - CONFIG_SYS_FLASH_BASE);
    setenv("boot.bank1.loader", temp);

    sprintf(temp,"0x%08x ", CFG_UPGRADE_UBOOT_BANK1 - CONFIG_SYS_FLASH_BASE);
    setenv("boot.bank1.uboot", temp);

    sprintf(temp,"0x%08x ", CFG_UPGRADE_LOADER_BANK0 - CONFIG_SYS_FLASH_BASE);
    setenv("boot.bank0.loader", temp);

    sprintf(temp,"0x%08x ", CFG_UPGRADE_UBOOT_BANK0 - CONFIG_SYS_FLASH_BASE);
    setenv("boot.bank0.uboot", temp);

    s = getenv("bootcmd.prior");
    if (s) {
	sprintf(temp, "%s;%s", s,
	    gd->flash_bank ? CONFIG_BOOTCOMMAND_BANK1 : CONFIG_BOOTCOMMAND);
	setenv("bootcmd", temp);
    } else {
	setenv("bootcmd",
	    gd->flash_bank ? CONFIG_BOOTCOMMAND_BANK1 : CONFIG_BOOTCOMMAND);
    }

    sprintf(temp,"0x%02x ", CFG_I2C_C1_9548SW1_P4_R5H30211);
    setenv("boot.ideeprom", temp);

    if (gd->env_valid == 3) {
	/* save default environment */
	gd->flags |= GD_FLG_SILENT;
	saveenv();
	gd->flags &= ~GD_FLG_SILENT;
	gd->env_valid = 1;
    }

}

#ifdef CONFIG_SHOW_ACTIVITY
void
board_show_activity (ulong timestamp)
{
    static int link_status = 0;
    int i, temp, temp_page;
    volatile uint16_t temp1;
    PortDuplex port_duplexity = 0;
    PortSpeed port_speed = 0;

    if (epld_reg_read(EPLD_WATCHDOG_ENABLE, (uint16_t *)&temp1)) {
	if (temp1 & 0x1)  /* watchdog enabled */
	    epld_reg_write(EPLD_WATCHDOG_SW_UPDATE, 0x1);
    }

    volatile uint16_t ledctrl;
    /*
     * update the management i/f LED, every 2 seconds,
     * no requirement of any mode settings
     */
    if (mgmt_priv && (timestamp % (2 * CONFIG_SYS_HZ) == 0)) {
	temp_page = read_phy_reg_private(mgmt_priv, 0x16);
	write_phy_reg_private(mgmt_priv, 0x16, 0x0);
	temp = read_phy_reg_private(mgmt_priv, 0x11);
	write_phy_reg_private(mgmt_priv, 0x16, temp_page);
	if ((temp & 0xe400) != (link_status & 0xe400)) {
	    link_status = temp;
	    if (link_status & MIIM_88E1011_PHYSTAT_LINK) {
		if (link_status & MIIM_88E1011_PHYSTAT_DUPLEX)
		    port_duplexity = FULL_DUPLEX;
		else
		    port_duplexity = HALF_DUPLEX;
		if (link_status & MIIM_88E1011_PHYSTAT_GBIT)
		    port_speed = SPEED_1G;
		else if (link_status & MIIM_88E1011_PHYSTAT_100)
		    port_speed = SPEED_100M;
		else
		    port_speed = SPEED_10M;

		set_port_led(0, LINK_UP, port_duplexity, port_speed);
	    } else {
		set_port_led(0, LINK_DOWN, 0, 0);  /* don't care */
	    }
	}
    }

    /* Make the SYSTEM LED BLINK */
    if (timestamp % (CONFIG_SYS_HZ / 2) == 0) {
	if (epld_reg_read(EPLD_FRONT_LED_CTRL, (uint16_t *)&ledctrl)) {
	    /* check if the SYSTEM LED GREEN is ON */
	    if (ledctrl & SYS_LED_GREEN_ON) {
		ledctrl = SYS_LED_GREEN_OFF; /* Turn if OFF */
	    } else {
		ledctrl = SYS_LED_GREEN_ON; /* Turn it ON */
	    }
	    epld_reg_write(EPLD_FRONT_LED_CTRL, ledctrl);
	}
    }

    /* 500ms */
    if (timestamp % (CONFIG_SYS_HZ / 2) == 0) {
	for (i = 0; i < CFG_LCD_NUM; i++)
	    lcd_heartbeat(i);
    }
}

void
show_activity (int arg)
{
}
#endif

#ifdef CONFIG_POST
DECLARE_GLOBAL_DATA_PTR;

/* post test name declartion */
static char *post_test_name[] = { "cpu", "ram", "pci", "ethernet"};

void
post_run_ex43xx (void)
{
    int i;

    /*
     * Run Post test to validate the cpu, ram, pci and ethernet 
     * (mac, phy loopback) 
     */
    for (i = 0; i < sizeof(post_test_name) / sizeof(char *); i++) {
	if (post_run(post_test_name[i],
		POST_RAM | POST_MANUAL) != 0)
	    printf(" Unable to execute the POST test %s \n",
	post_test_name[i]);
    }

    if (post_result_pci == -1) {
	setenv("post.pci", "FAILED ");
	printf("POST: pci FAILED\n");
    } else {
	setenv("post.pci", "PASSED ");
    }
    if (post_result_ether == -1) {
	setenv("post.ethernet", "FAILED ");
	printf("POST: ethernet FAILED\n");
    } else {
	setenv("post.ethernet", "PASSED ");
    }
    if (gd->valid_bid == 0) {
	setenv("post.eeprom", "FAILED ");
	printf("POST: eeprom FAILED\n");
    } else {
	setenv("post.eeprom", "PASSED ");
    }
}

/*
 * Returns 1 if keys pressed to start the power-on long-running tests
 * Called from board_init_f().
 */
int 
post_hotkeys_pressed (void)
{
    return 0;   /* No hotkeys supported */
}

#endif

int
post_stop_prompt (void)
{
    char *s;
    int mem_result;

    if (getenv("boot.nostop")) return (0);

    if (((s = getenv("boot.ram")) != NULL) &&
	((strcmp(s, "PASSED") == 0) ||
	 (strcmp(s, "FAILED") == 0))) {
	mem_result = 0;
    } else {
	mem_result = post_result_mem;
    }
    if (mem_result == -1 ||
	post_result_ether == -1 ||
	gd->valid_bid == 0)
	return 1;

    return 0;
}

int
secure_eeprom_read (unsigned dev_addr, unsigned offset,
			uchar *buffer, unsigned cnt)
{
    int alen = (offset > 255) ? 2 : 1;

    return i2c_read_norsta(dev_addr, offset, alen, buffer, cnt);
}

#ifdef CONFIG_CONSOLE_SELECT
int console_select(void)
{
    volatile uint16_t epldMiscCtrl;
    char tmp[64];
    int i;

    epld_reg_read(EPLD_MISC_CONTROL, (uint16_t *)&epldMiscCtrl);
    i = getenv_f("hw.uart.console", tmp, sizeof (tmp));

    if (!strcmp(tmp, "mm:0xfe11c600")) {
	if (epldMiscCtrl & 0x01) {
	    gd->console = 2; /* UART 1 */
	} else {
	    gd->console = 1; /* default to UART0 */
	}
    } else {
	gd->console = 1; /* default to UART0 */
    }

    return 0;
}
#endif

#ifdef CONFIG_BOARD_RESET
void
board_reset (void)
{
    epld_system_reset();
}
#endif
