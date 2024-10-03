/*
 * Copyright (c) 2011, Juniper Networks, Inc.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. if not ,see
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0,html>  
 */


#include <watchdog_cpu.h>
#include <common.h>
#include <command.h>
#include <octeon_boot.h>
#include <lib_octeon_shared.h>
#include <lib_octeon.h>
#include <jsrxnle_ddr_params.h>
#include <octeon_hal.h>
void enable_ciu_watchdog(int mode);
void disable_ciu_watchdog(void);
void pat_ciu_watchdog(void);
uchar mag8600_read_eeprom (uchar offset);


/* set the ethaddr */
void
board_get_enetaddr (uchar * enet)
{
    unsigned char temp[20];
    DECLARE_GLOBAL_DATA_PTR;

    memcpy(enet, &gd->mac_desc.mac_addr_base[0], 6);
    sprintf(temp, "%02x:%02x:%02x:%02x:%02x:%02x",
            enet[0], enet[1], enet[2], enet[3], enet[4], enet[5]);
    setenv("ethaddr", temp);
    return;
}


#if defined(CONFIG_PCI)
extern void init_octeon_pci (void);

void
pci_init_board (void)
{
}
#endif

int
checkboard (void)
{
    /* Force us into GMII mode, enable interface */
    /* NOTE:  Applications expect this to be set appropriately
    ** by the bootloader, and will configure the interface accordingly */
    cvmx_write_csr (CVMX_GMXX_INF_MODE (0), 0x3);

    /* Enable SMI to talk with the GMII switch */
    cvmx_write_csr (CVMX_SMI_EN, 0x1);
    cvmx_read_csr(CVMX_SMI_EN);

    return 0;
}



int
early_board_init (void)
{
    int cpu_ref = 50;
    int i;
    DECLARE_GLOBAL_DATA_PTR;

    memset((void *)&(gd->mac_desc), 0x0, sizeof(octeon_eeprom_mac_addr_t));
    memset((void *)&(gd->clock_desc), 0x0, sizeof(octeon_eeprom_clock_desc_t));
    
    /* NOTE: this is early in the init process, so the serial port is not yet configured */
    gd->ddr_clock_mhz = MSC_DEF_DRAM_FREQ;
    /* CN56XX has a fixed internally generated 50 MHz reference clock */
    gd->ddr_ref_hertz = 50000000;

    /* Determine board type/rev */
    strncpy((char *)(gd->board_desc.serial_str), "unknown", SERIAL_LEN);
    gd->flags                 |= GD_FLG_BOARD_DESC_MISSING;
    gd->board_desc.rev_minor   = 0;
    gd->board_desc.board_type  = MAG8600_MSC;
    gd->board_desc.rev_major   = 1;

    gd->ddr_clock_mhz = MSC_DEF_DRAM_FREQ;

    /* Make up some MAC addresses */
    gd->mac_desc.count            = 255;
    gd->mac_desc.mac_addr_base[0] = 0x00;
    gd->mac_desc.mac_addr_base[1] = 0xDE;
    gd->mac_desc.mac_addr_base[2] = 0xAD;
    gd->mac_desc.mac_addr_base[3] = (gd->board_desc.rev_major<<4) | gd->board_desc.rev_minor;
    gd->mac_desc.mac_addr_base[4] = gd->board_desc.serial_str[0];
    gd->mac_desc.mac_addr_base[5] = 0x00;


    cpu_ref           = MSC_CPU_REF ;
    gd->ddr_clock_mhz = MSC_DEF_DRAM_FREQ;


    /* Read CPU clock multiplier */
    uint64_t data;
    
    data = cvmx_read_csr(CVMX_PEXP_NPEI_DBG_DATA);

    data = data >> 18;
    data &= 0x1f;

    gd->cpu_clock_mhz = data * cpu_ref;

    return 0;

}

uchar
mag8600_read_eeprom (uchar offset)
{
    DECLARE_GLOBAL_DATA_PTR;
    uchar buf[1] = { 0 };

    if (i2c_read8_generic(BOARD_EEPROM_TWSI_ADDR, offset, buf, 0)) {
        printf("ERR::  Failed to read the I2C addr 0x%x \n", offset);
    }
    return buf[0];
}
/*
 * function to disable ciu watchdog
 */ 
void
disable_ciu_watchdog (void)
{
    uint64_t val;

    val = cvmx_read_csr(CVMX_CIU_WDOGX(CIU_WATCHDOG0_OFFSET));
    val &= ~WATCH_DOG_DISABLE_MASK;
    cvmx_write_csr(CVMX_CIU_WDOGX(CIU_WATCHDOG0_OFFSET), val);
}

/* 
 * function to enable ciu watchdog
 */ 
void
enable_ciu_watchdog (int mode)
{
    uint64_t val;

    val = (WATCH_DOG_ENABLE_MASK | mode);
    cvmx_write_csr(CVMX_CIU_WDOGX(CIU_WATCHDOG0_OFFSET), val);
}

/* 
 * function to tickle ciu watchdog
 */ 
void
pat_ciu_watchdog (void)
{
    uint64_t val = 0;

    cvmx_write_csr(CVMX_CIU_PP_POKEX(CIU_PP_POKE0_OFFSET), val);
}

/*
 * functions dumps ciu watchdog cout register
 */ 
void
dump_ciu_watchdog_reg (void)
{
    uint64_t val;
    uint32_t valu, vall;

    val  = cvmx_read_csr(CVMX_CIU_WDOGX(CIU_WATCHDOG0_OFFSET));
    vall = (val & WATCH_DOG_LOWER_MASK);
    valu = (val >> 32);
    printf("watchdog reg 0 value: = %08x%08x\n", valu, vall);
}

/*
 * FIXME: POST framework while running from flash uses a reserved 
 * space in RAM for its flags. These flags are about boot mode
 * and we don't depend on them. So we are not using them right now.
 */
DECLARE_GLOBAL_DATA_PTR;
void
post_word_store (ulong a)
{
#ifdef FIXME_LATER
    volatile long *save_addr_word = (volatile ulong *)(POST_STOR_WORD);
    *save_addr_word = a;
#endif
}

ulong
post_word_load (void)
{
#ifndef FIXME_LATER
    ulong a = 0;
    return a;
#else
    volatile long *save_addr_word = (volatile ulong *)(POST_STOR_WORD);           
    return *save_addr_word;
#endif
}

int
post_hotkeys_pressed (void)
{
    return 0; /* No hotkeys supported */
}

void
reload_watchdog (int32_t val)
{
    static int32_t lastwdogtime = WATCHDOG_MODE_INT_NMI_SRES;

    if (val == PAT_WATCHDOG) {
	/*
	 * use lastwdogtime platform specific patting.
	 */
	pat_ciu_watchdog();
    } else if (val == DISABLE_WATCHDOG) {
	/*
	 * disable watchdog
	 */
	disable_ciu_watchdog();
    } else if (val == ENABLE_WATCHDOG) {
	/*
	 * enable watchdog
	 */
	lastwdogtime = WATCHDOG_MODE_INT_NMI_SRES;
	enable_ciu_watchdog(WATCHDOG_MODE_INT_NMI_SRES);
    }
}

int
msc_board_phy_rst(void)
{
    printf("Reset MSC MGT port phy\n");
    oct_miiphy_write (6, 0, 0x8000); //rst phy
    oct_miiphy_write (6, 0, 0x0800); // pwr down
    oct_miiphy_write (6, 0, 0x3100); // pwr up

    return 0;
}

