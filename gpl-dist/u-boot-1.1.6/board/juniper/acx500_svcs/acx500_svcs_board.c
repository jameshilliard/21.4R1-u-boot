/*
 * Copyright (c) 2014, Juniper Networks, Inc.
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
#include <cvmx-pemx-defs.h>
#include <cvmx.h>

#define BAR2_ENB          0x1
#define BAR2_ENB_SHIFT    0x3
#define BAR2_ENDIAN_SWAP  0x1
#define BAR2_ENDIAN_SHIFT 0x1

void pcie_ep_init(void)
{
    uint64_t pem0_bar_ctl;

    pem0_bar_ctl = cvmx_read_csr(CVMX_PEMX_BAR_CTL(0));

    pem0_bar_ctl |= (BAR2_ENB << BAR2_ENB_SHIFT);
    pem0_bar_ctl |= (BAR2_ENDIAN_SWAP << BAR2_ENDIAN_SHIFT);

    cvmx_write_csr(CVMX_PEMX_BAR_CTL(0), pem0_bar_ctl);
}

#define KERNEL_IMAGE        0x0
#define DIAG_IMAGE          0x1
#define HW_RC_IMAGE         0x2
#define HW_PASSTHRU_IMAGE   0x3

extern int get_uboot_bank(void);
extern char console_buffer[];

int
early_board_init (void)
{
    int cpu_ref = 50;
    int i;
    DECLARE_GLOBAL_DATA_PTR;
    static int mac_addr_index = 0;

    memset((void *)&(gd->clock_desc), 0x0, sizeof(octeon_eeprom_clock_desc_t));

    /* NOTE: this is early in the init process, so the serial port is not yet configured */
    gd->board_desc.board_type  = BOARD_TYPE_ACX500_SVCS;
    gd->ddr_clock_mhz = ACX500_SVCS_DEF_DRAM_FREQ;

    /* Octeon has a fixed internally generated 50 MHz reference clock */
    gd->ddr_ref_hertz = 50000000;

    /* Determine board type/rev */
    strncpy((char *)(gd->board_desc.serial_str), "unknown", SERIAL_LEN);
    gd->flags                 |= GD_FLG_BOARD_DESC_MISSING;
    gd->board_desc.rev_minor   = 0;
    gd->board_desc.rev_major   = 1;

    /*
     * Make up some MAC addresses
     * Ideally this should be read from EEPROM which
     * is not available on ACX500_SVCS from CAVIUM uboot.
     * Since this is only internal links it can be 
     * safely hardcoded for uboot 
     */
    gd->mac_desc.count            = 255;
    gd->mac_desc.mac_addr_base[0] = 0x28;
    gd->mac_desc.mac_addr_base[1] = 0x8a;
    gd->mac_desc.mac_addr_base[2] = 0x1c;
    gd->mac_desc.mac_addr_base[3] = 0x10;
    gd->mac_desc.mac_addr_base[4] = 0x00;
    gd->mac_desc.mac_addr_base[5] = 0x00;

    return 0;
}

int
late_board_init (void)
{
    uint8_t dev_addr;
    uint8_t offset;
    uint8_t val;
    uint8_t group;

    dev_addr = ACX500_SVCS_CPLD_ADDR;
    offset = ACX500_SVCS_CPLD_FLASH_SWIZZLE;
    group = ACX500_SVCS_TWSI_1;

    debug("Disabling flash swizzle\n");

    if (!read_i2c(dev_addr, offset, &val, group)) {
        puts("Disable flash swizzle failed\n");
        return 1;
    }

    val &= ~ACX500_SVCS_FLASH_SWIZZLE_ENABLE;

    /* Disable the swizzle logic now */
    if (!write_i2c(dev_addr, offset, val, group)) {
        puts("Disable flash swizzle failed\n");
        return 1;
    }

    /*
     * u-boot has booted up and is ready to boot
     * whatever image the master wants it to.
     * Indicate our boot status to the master
     * through the cpld bit.
     */
    offset = ACX500_SVCS_CPLD_PFE_INTR_STATUS;
    val = 0;

    if (!read_i2c(dev_addr, offset, &val, group)) {
        puts("Reading PFE intr status register failed\n");
        return 1;
    }

    val |= 0x1;
    if (!write_i2c(dev_addr, offset, val, group)) {
        puts("Setting PFE intr status register failed\n");
        return 1;
    }

    return 0;
}

int checkboard (void)
{
    int clk_to_use = 0;	/* Clock used for DLM0 */
    int bank;

    debug("Configuring DLM0 for SGMII/DISABLED: ");
    octeon_configure_qlm(0, 2500, CVMX_QLM_MODE_SGMII_SGMII,
            0, 0, clk_to_use, 1);
    debug("Done.\n");

    if ((bank = get_uboot_bank()) == -1) {
        puts("Error trying to read the Boot Bank\n");
    } else {
        printf("U-boot booting from: Bank-%d (Active Bank)\n", bank);
    }

    return 0;
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
}

ulong
post_word_load (void)
{
    return 0;
}

int
post_hotkeys_pressed (void)
{
    return 0; /* No hotkeys supported */
}

static uint32_t
read_load_address (void)
{
    uint8_t dev_addr;
    uint8_t offset;
    uint8_t val;
    uint8_t group;
    uint32_t load_addr = 0x0;

    dev_addr = ACX500_SVCS_CPLD_ADDR;
    offset = ACX500_SVCS_CPLD_GP4;
    group = ACX500_SVCS_TWSI_1;
    val = 0;

    if (!read_i2c(dev_addr, offset, (uint8_t *)&val, group)) {
        puts("Reading interrupt status register failed\n");
        return 0;
    }
    load_addr |= ((val & 0xff) << 24);

    offset = ACX500_SVCS_CPLD_GP3;
    val = 0;
    if (!read_i2c(dev_addr, offset, (uint8_t *)&val, group)) {
        puts("Reading interrupt status register failed\n");
        return 0;
    }
    load_addr |= ((val & 0xff) << 16);

    offset = ACX500_SVCS_CPLD_GP2;
    val = 0;
    if (!read_i2c(dev_addr, offset, (uint8_t *)&val, group)) {
        puts("Reading interrupt status register failed\n");
        return 0;
    }
    load_addr |= ((val & 0xff) << 8);

    offset = ACX500_SVCS_CPLD_GP1;
    val = 0;
    if (!read_i2c(dev_addr, offset, (uint8_t *)&val, group)) {
        puts("Reading interrupt status register failed\n");
        return 0;
    }
    load_addr |= (val & 0xff);

    debug("load add => 0x%x\n", load_addr);
    return load_addr;
}

/*
 * Checks if there is any image for the Service Processor
 * to boot. In case there is any, calls the appropriate
 * function to boot that image.
 */
void
boot_image_check (void)
{
    uint8_t dev_addr;
    uint8_t offset;
    uint8_t val;
    uint8_t group;
    uint8_t image;
    uint8_t buf[16];
    char *argv[CFG_MAXARGS + 1];
    uint32_t load_addr, argc;

    dev_addr = ACX500_SVCS_CPLD_ADDR;
    offset = ACX500_SVCS_CPLD_INTR_STATUS;
    group = ACX500_SVCS_TWSI_1;
    val = 0;

    if (!read_i2c(dev_addr, offset, &val, group)) {
        puts("Reading interrupt status register failed\n");
        return;
    }

    /* Check if the 'download done' bit has been set */
    if (val & 0x1) {
        /*
         * Ok, so the image has been downloaded to the Service Processor's
         * DDR by the master. Steps to follow now
         * 1. Check the image that has been downloaded
         * 2. Read the image' load address register
         * 3. Create the argv array of parameters to be passed
         * 4. Clear the 'download done' bit in the cpld(W1C)
         * 5. Based on value read in 1, call the appropriate function
         */
        offset = ACX500_SVCS_CPLD_IMAGE_REG;
        val = 0;
        if (!read_i2c(dev_addr, offset, &val, group)) {
            puts("Reading image register failed\n");
            return;
        }
        image = val;

        /*
         * Read the 'load address' register to find
         * out the location of the downloaded image.
         */
        load_addr = read_load_address();

        if (load_addr == 0) {
            puts("Read incorrect load address\n");
            return;
        }

        switch (val) {
        case KERNEL_IMAGE:
        case DIAG_IMAGE:
            strcpy(console_buffer, "bootelf ");
            sprintf(buf, "0x%x", load_addr);
            strcat(console_buffer, buf);
            argc = make_argv(console_buffer, sizeof(argv)/sizeof(argv[0]), argv);

            offset = ACX500_SVCS_CPLD_INTR_STATUS;
            val = 0x0;
            if (!read_i2c(dev_addr, offset, &val, group)) {
                puts("Reading interrupt status register failed\n");
                return;
            }

            val |= 0x1;
            if (!write_i2c(dev_addr, offset, val, group)) {
                puts("W1C to intr status register failed\n");
                return;
            }

            do_bootelf(NULL, 0, argc, argv);
            break;

        case HW_RC_IMAGE:
        case HW_PASSTHRU_IMAGE:
            strcpy(console_buffer, "bootoct ");
            sprintf(buf, "0x%x", load_addr);
            strcat(console_buffer, buf);
            strcat(console_buffer, " coremask=0xf");

            argc = make_argv(console_buffer, sizeof(argv)/sizeof(argv[0]), argv);

            offset = ACX500_SVCS_CPLD_INTR_STATUS;
            val = 0x0;
            if (!read_i2c(dev_addr, offset, &val, group)) {
                puts("Reading interrupt status register failed\n");
                return;
            }

            val |= 0x1;
            if (!write_i2c(dev_addr, offset, val, group)) {
                puts("W1C to intr status register failed\n");
                return;
            }

            do_bootocteon(NULL, 0, argc, argv);
            break;

        default:
            break;
        }

    }
}
