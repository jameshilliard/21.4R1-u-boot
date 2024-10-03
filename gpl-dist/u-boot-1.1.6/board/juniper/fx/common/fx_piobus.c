/*
 * Copyright (c) 2009-2013, Juniper Networks, Inc.
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
 * You should have received a copy of the GNU General Public
 * License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include "linux/types.h"
#include "common.h"
#include "configs/rmi_boards.h"
#include <rmi/shared_structs.h>
#include <rmi/board.h>
#include <rmi/physaddrspace.h>
#include <rmi/bridge.h>
#include <rmi/pioctrl.h>
#include <rmi/cpld.h>
#include <rmi/flashdev.h>
#include <rmi/cfiflash.h>
#include <rmi/gpio.h>

#include "fx_common.h"
#include "fx_cpld.h"

#define MAX_CHIP_SELECTS    10

/* Currently these are the devices supported on PIO bus */
struct pio_dev fx_mcpld_dev;
struct pio_dev tor_scpld_dev;
struct pio_dev pa_acc_fpga_dev;
struct pio_dev reset_cpld_dev;
struct pio_dev cfi_flash_dev;
extern void fx_cpld_init_dev(void);
extern uint8_t fx_is_soho(void);
extern uint8_t fx_is_pa(void);
extern uint8_t fx_is_cb(void);
extern uint8_t fx_is_wa(void);
extern uint8_t fx_is_tor(void);
extern uint8_t fx_has_reset_cpld(void);

void
fx_piobus_setup (struct rmi_board_info *rmib)
{
    struct piobus_info *pio;
    uint32_t reset_cpld_cs;

    pio = &rmib->pio;

    memset(pio, 0, sizeof(struct piobus_info));

    pio->cfi_flash = 1;
    pio->cpld = 1;
    if (fx_is_tor()) {
        pio->tor_scpld= 1;
    }
    if ((fx_is_pa()) || (fx_is_wa())) {
        pio->pa_acc_fpga= 1;
    }

    if (fx_has_reset_cpld()) {
        pio->soho_reset_cpld= 1;
    }
    /* Initialize the piobus devices with the default values */
    memset(&fx_mcpld_dev, 0, sizeof(struct pio_dev));
    memset(&tor_scpld_dev, 0, sizeof(struct pio_dev));
    memset(&pa_acc_fpga_dev, 0, sizeof(struct pio_dev));
    memset(&reset_cpld_dev, 0, sizeof(struct pio_dev));
    memset(&cfi_flash_dev, 0, sizeof(struct pio_dev));

    fx_mcpld_dev.enabled = 1;
    fx_mcpld_dev.chip_sel = FX_MCPLD_CS;
    fx_mcpld_dev.offset = (MB(16 + 8 + 1));
    fx_mcpld_dev.size = MB(1);

    if (fx_is_tor()) {
        tor_scpld_dev.enabled = 1;
        tor_scpld_dev.chip_sel = PA_SCPLD_CS;
        tor_scpld_dev.offset = (MB(16 + 8 + 1 + 1));
        tor_scpld_dev.size = MB(1);
    }

    if ((fx_is_pa()) || (fx_is_wa())) {
        pa_acc_fpga_dev.enabled = 1;
        pa_acc_fpga_dev.chip_sel = PA_ACC_FPGA_CS;
        pa_acc_fpga_dev.offset = (MB(16 + 8 + 1 + 1 + 1));
        pa_acc_fpga_dev.size = MB(1);
    }

    if (fx_has_reset_cpld()) {
        if (fx_is_pa()) {
            reset_cpld_cs = PA_RESET_CPLD_CS;
        } else if (fx_is_soho()) {
            reset_cpld_cs = SOHO_RESET_CPLD_CS;
        }

        reset_cpld_dev.enabled = 1;
        reset_cpld_dev.chip_sel = reset_cpld_cs;
        if (fx_is_pa()) {
            reset_cpld_dev.offset = (MB(16 + 8 + 1 + 1 + 1 + 1));
        } else if (fx_is_soho()) {
            reset_cpld_dev.offset = (MB(16 + 8 + 1 + 1 + 1));
        }
        reset_cpld_dev.size = MB(1);
    }

    cfi_flash_dev.enabled = 1;
    cfi_flash_dev.chip_sel = FX_BOOT_FLASH_CS;
    cfi_flash_dev.offset = MB(0);
    cfi_flash_dev.size = MB(16);
}

void 
fx_tweak_pio_cs (struct rmi_board_info *rmib)
{
    rmib->pio.cpld = 1;
    rmib->pio.tor_scpld = 1;
    rmib->pio.pa_acc_fpga = 1;
    rmib->pio.soho_reset_cpld= 1;

    rmib->bfinfo.primary_flash_type = FLASH_TYPE_NOR;
    rmib->bfinfo.secondary_flash_type = FLASH_TYPE_NONE;
    /* If booted from nor, chip select for nor flash is
     *     CS_BOOT_FLASH(0) and chip select for nand flash is
     * CS_SECONDARY_FLASH(2)
     */
    cfi_flash_dev.chip_sel = FX_BOOT_FLASH_CS;
    rmib->bfinfo.primary_flash_type = FLASH_TYPE_NOR;
    rmib->bfinfo.secondary_flash_type = FLASH_TYPE_NAND;
}

static inline void 
setup_chip_sel (phoenix_reg_t *mmio, uint32_t csl, 
               uint32_t bar, uint32_t mask)
{
    cpu_write_reg(mmio, reg_offset(csl, FLASH_CSBASE_ADDR_REG),bar);
    cpu_write_reg(mmio, reg_offset(csl, FLASH_CSADDR_MASK_REG),mask);

};


void 
fx_init_piobus (struct rmi_board_info *rmib)
{
    struct piobus_info *pio;

    phoenix_reg_t *bridge_mmio = phoenix_io_mmio(PHOENIX_IO_BRIDGE_OFFSET);
    phoenix_reg_t *iobus_mmio  = phoenix_io_mmio(PHOENIX_IO_FLASH_OFFSET);

    uint32_t bar = 0;
    uint32_t mask = 0, size;
    uint32_t en = 0x1;
    uint32_t value = 0;
    uint32_t iobus_addr_space_start = 0;
    uint32_t iobus_addr_space_size  = 0;
    unsigned long cs_base = 0;
    unsigned int chip_sel = 0;
    int i = 0;

    pio = &rmib->pio;

    iobus_addr_space_start = PIO_BASE_PHYS;
    iobus_addr_space_size = PIO_BASE_SIZE;

    /* program the bridge */
    bar = ((iobus_addr_space_start >> 24) << 16);
    mask = ( (iobus_addr_space_size >> 24) - 1 ) << 5;
    cpu_write_reg(bridge_mmio, FLASH_BAR, (bar | mask | en));

    rmi_add_phys_region((uint64_t)iobus_addr_space_start,
                        (uint64_t)iobus_addr_space_size,
                        FLASH_CONTROLLER_SPACE);

    cs_base = iobus_addr_space_start & (iobus_addr_space_size - 1);

    /* First map all chip selects to upper 64kb of the iobus_addr_space region */
    value = cs_base + iobus_addr_space_size - KB(64);
    for (i = 0; i < MAX_CHIP_SELECTS; i++) {
        setup_chip_sel(iobus_mmio, i, (value >> 16), 0);
    }

    /* map chip selects for available devices */
    if (cfi_flash_dev.enabled) {
        bar = cfi_flash_dev.offset;
        size = cfi_flash_dev.size;

        mask = ((size / KB(64)) - 1) & 0xffff;
        chip_sel = cfi_flash_dev.chip_sel;

        setup_chip_sel(iobus_mmio, chip_sel, (bar >> 16), mask);
        cfi_flash_dev.iobase = (unsigned long)(PIO_BASE_PHYS + bar);
        cfi_flash_dev.iobase = (unsigned long)
                               phys_to_kseg1(cfi_flash_dev.iobase);
    }

    if (fx_mcpld_dev.enabled) {
        bar = fx_mcpld_dev.offset;
        size = fx_mcpld_dev.size;

        mask = ((size / KB(64)) - 1) & 0xffff;
        chip_sel = fx_mcpld_dev.chip_sel;

        setup_chip_sel(iobus_mmio, chip_sel, (bar >> 16), mask);
        fx_mcpld_dev.iobase = (unsigned long)(PIO_BASE_PHYS + bar);
        fx_mcpld_dev.iobase = (unsigned long)
                          phys_to_kseg1(fx_mcpld_dev.iobase);
    }

    if (fx_is_tor()) {
        if (tor_scpld_dev.enabled) {
            bar = tor_scpld_dev.offset;
            size = tor_scpld_dev.size;
            mask = ((size / KB(64)) - 1) &0xffff;
            chip_sel = tor_scpld_dev.chip_sel;

            setup_chip_sel(iobus_mmio, chip_sel, (bar >> 16), mask);
            tor_scpld_dev.iobase = (unsigned long)(PIO_BASE_PHYS + bar);
            tor_scpld_dev.iobase = (unsigned long)
                                    phys_to_kseg1(tor_scpld_dev.iobase);
        }
    }

    if ((fx_is_pa()) ||  (fx_is_wa())) {
        if (pa_acc_fpga_dev.enabled) {
            bar = pa_acc_fpga_dev.offset;
            size = pa_acc_fpga_dev.size;

            mask = ((size / KB(64)) - 1) &0xffff;
            chip_sel = pa_acc_fpga_dev.chip_sel;

            setup_chip_sel(iobus_mmio, chip_sel, (bar >> 16), mask);
            pa_acc_fpga_dev.iobase = (unsigned long)(PIO_BASE_PHYS + bar);
            pa_acc_fpga_dev.iobase = (unsigned long)
                                    phys_to_kseg1(pa_acc_fpga_dev.iobase);
        }
    } 
    
    if (fx_has_reset_cpld()) {
        if (reset_cpld_dev.enabled) {
            bar = reset_cpld_dev.offset;
            size = reset_cpld_dev.size;

            mask = ((size / KB(64)) - 1) &0xffff;
            chip_sel = reset_cpld_dev.chip_sel;

            setup_chip_sel(iobus_mmio, chip_sel, (bar >> 16), mask);
            reset_cpld_dev.iobase = (unsigned long)(PIO_BASE_PHYS + bar);
            reset_cpld_dev.iobase = (unsigned long)
                                    phys_to_kseg1(reset_cpld_dev.iobase);
        }
    }
}

void
fx_init_piobus_devices (void)
{
    fx_cpld_init(&fx_mcpld_dev, &tor_scpld_dev, &pa_acc_fpga_dev,
                 &reset_cpld_dev);
    fx_cpld_init_dev();
}
