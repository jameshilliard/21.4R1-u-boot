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
 * You should have received a copy of the GNU General Public
 * License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <common/fx_common.h>
#include <soho/soho_cpld.h>
#include <watchdog_cpu.h>
#include <common/fx_cpld.h>

#define RST_STATUS_REG_RST_TYPE_MASK        (0x1F)
#define RST_STATUS_REG_PARTITION_NUM_MASK   (0x20)
#define WATCHDOG_REG_VALUE_MASK             (0x7F)
#define MAX_WATCHDOG_TIMER_VALUE            (0x7F)
#define LOADER_WATCHDOG_TIMER_VALUE         (0x1F)

uint8_t *cpld_iobase;
uint8_t *reset_cpld_iobase;
boolean      fx_cpld_init_done = FALSE;
boolean      fx_cpld_disable_wd = FALSE;
extern uint16_t assm_id;

extern u_int8_t cb_cpld_get_version(void);
extern u_int8_t cb_cpld_init_dev();
extern uint32_t get_boot_status(void);
extern uint8_t fx_is_tor(void);

/* This file has the functions for the BOOT CPLD on the FX products. The BOOT
 * CPLD is on a local bus using Chip Select 1 and uses the XLS_IO_DEV_FLASH
 * controller. From boot1_5 the base address of the cpld lies at 0xbfb00000
 * and once the flash is rebased to 0xbc000000 in boot2, the cpld base address
 * lies at 0xbd900000 (25M offset from the base)
 */

/* Function : fx_cpld_early_init
 * Arguments: cs       -  chip select
 *            offset   -  value to be set in CS_BASE_ADDRESS. This will be
 *                        the offset of the CPLD base address from FLASH_BAR.
 *            mask     -  This specifies the address space size from the
 *                        base address and the increment size is 64K
 * Return   : None
 * Description:  This function sets up the Chip Select Area registers to
 *               access the CPLD registers. Once the base address register
 *               is set, the CPLD base adress is at:
 *               (((Value at FLASH_BAR >> 8) + CS_BASE_ADDRESS(n)) << 16)
 * Notes:        Though the RMI Doc refers to the controller as a Flash 
 *               controller, it is a peripheral IO controller.
 */
void 
fx_cpld_early_init (void)
{
    uint32_t reset_cpld_cs;

    *(reg_t *)(CS_BASE_ADDRESS(FX_MCPLD_CS)) = 0xb0;
    *(reg_t *)(CS_ADDRESS_MASK(FX_MCPLD_CS)) = 0xF;
    *(reg_t *)(CS_DEV_PARAM_ADDRESS(FX_MCPLD_CS)) = 0x0;
    cpld_iobase = (u_int8_t *)(((((*((uint32_t *)(FX_FLASH_BAR))) >> 8) &
                                     (0xff00)) + 0xb0) << 16);
    cpld_iobase = (u_int8_t *)(phys_to_kseg1((u_int32_t)cpld_iobase));

    debug("\ncpld_init start\n");
    if (fx_has_reset_cpld() || fx_is_pa()) {
        reset_cpld_cs = (fx_is_pa()) ? PA_SCPLD_CS : SOHO_RESET_CPLD_CS;
        *(reg_t *)(CS_BASE_ADDRESS(reset_cpld_cs)) = 0xa0;
        *(reg_t *)(CS_ADDRESS_MASK(reset_cpld_cs)) = 0xF;
        *(reg_t *)(CS_DEV_PARAM_ADDRESS(reset_cpld_cs)) = 0x0;
        *(reg_t *)(CS_DEV_OE_ADDRESS(reset_cpld_cs)) = 0x4f240e22;
        *(reg_t *)(CS_DEV_TIME_ADDRESS(reset_cpld_cs)) = 0xCB2;
        reset_cpld_iobase = (u_int8_t *)(((((*((uint32_t *)
                                            (FX_FLASH_BAR))) >> 8) &
                                            (0xff00)) + 0xa0) << 16);
        reset_cpld_iobase = (u_int8_t *)(phys_to_kseg1(
                                            (u_int32_t)reset_cpld_iobase));
    } 

    fx_cpld_init_done = TRUE;
    debug("\ncpld_init done -  master CPLD base address: 0x%x\n", cpld_iobase);
    if (fx_has_reset_cpld()) {
        debug("\ncpld_init done -  reset CPLD base address: 0x%x\n", 
              reset_cpld_iobase);
    }

    if (fx_is_pa()) {
        /* put the netlogic phy in reset */
        reset_cpld_iobase[0xE0] = 0xFF; 
        reset_cpld_iobase[0xE1] = 0xFF; 
        reset_cpld_iobase[0xE2] = 0xFF; 
        reset_cpld_iobase[0xE3] = 0xFF; 
        reset_cpld_iobase[0xE5] = 0xFF; 
    }

    if (fx_has_i2c_cpld()) {
        debug("%s: cpld_iobase=%p\n", __func__, cpld_iobase);
        tor_cpld_i2c_controller_reset();
    }
}

void
fx_cpld_init (struct pio_dev *mcpld, struct pio_dev *scpld, struct pio_dev *acc_fpga,
              struct pio_dev *reset_cpld)
{
    u_int8_t cpld_version;

    if (fx_is_cb()) {
        cb_cpld_init(mcpld);
        cpld_iobase = (u_int8_t *)mcpld->iobase;
        fx_cpld_init_done = TRUE;
    }
    if (fx_is_tor() ||  fx_is_wa()) {
        soho_cpld_init(mcpld, scpld, acc_fpga, reset_cpld);
        cpld_iobase = (u_int8_t *)mcpld->iobase;
        reset_cpld_iobase = reset_cpld->iobase;
        fx_cpld_init_done = TRUE;
    }
    if (fx_cpld_init_done == TRUE) {
        cpld_version = fx_cpld_get_version();
        debug("%s: FX CPLD version=0x%x\n", __func__, cpld_version);
    }
}

extern uint8_t g_board_ver;
static uint32_t
fx_get_boot_cpld_base (void)
{
    if (fx_is_soho() && fx_get_board_ver() >= 2) {
        debug("%s[%d]: reset_cpld_iobase=0x%x\n", __func__, __LINE__,
               reset_cpld_iobase);
        return reset_cpld_iobase;;
    } else {
        debug("%s[%d]: cpld_iobase=0x%x\n", __func__, __LINE__,
               cpld_iobase);
        return cpld_iobase;
    }
}

u_int8_t 
fx_cpld_read (u_int8_t *cpld_base, u_int8_t offset)
{
    if (fx_cpld_init_done == TRUE) {
        return (cpld_base[offset]);
    }

    return (0xFF);
}

int 
fx_cpld_write (u_int8_t *cpld_base, u_int8_t offset, u_int8_t value)
{
    if (fx_cpld_init_done == TRUE) {
        cpld_base[offset] = value;
        return 0;
    }
    return -1; 
}

/* Function: fx_cpld_set_bootkey
 * Arguments: None
 * Return: None
 * Description: This function sets the BOOT_KEY in the CPLD. This is set
 *              only when the u-boot is completely booted. Once set, the 
 *              CPLD would not reset the CPU on its timer expiry. BOOT_KEY
 *              is also set when a key press is detected to enter the 
 *              BOOT1_5 shell. This does not guarantee that the u-boot image
 *              is good but makes sure that CPLD does not reset the CPU. When
 *              user exits BOOT1_5 shell, the CPU is reset and is allowed to
 *              boot from the same partition.
 */

void 
fx_cpld_set_bootkey (void)
{
    uint8_t *cpld_base = (uint8_t *)fx_get_boot_cpld_base();
    uint8_t value;

    debug("\nSetting the CPLD BOOT KEY");
    if (fx_cpld_write(cpld_base, BOOT_KEY_REG_OFFSET, BOOT_KEY_VALUE) == -1) {
        printf("\nError: Unable to set CPLD Boot Key\n");
    }	
    value = fx_cpld_read(cpld_base, BOOT_KEY_REG_OFFSET);

    /* turn on reset enable bit */
    fx_cpld_write(cpld_base, CPU_RESET_EN_REG_OFFSET, 0x1);
}



/* The following functions get and set the information in two registers.
 * Both the regiters are 1 Byte wide.
 * Here is the layout of the two:
 * 
 * CPU_RESET_STATUS REGISTER (Read Only)
 * -----------------------------------------------------------------------
 * |UNUSED|  CURRENT  |   CPLD    |WATCHDOG|  S/W  |PUSH BUTTON| COLD BOOT|
 * |______|_PARTITION_|___RESET___|__RESET_|_RESET_|___RESET___|___RESET__|
 *   6-7       5            4         3        2         1           0
 *
 * CPU_RESET Register (RW)
 * -----------------------------------------------------------------------
 * |             UNUSED                         |  NEXT BOOT  | SOFTWARE  |
 * |____________________________________________|__PARTITION__|___RESET___|
 *                    2-7                               1           0
 *
 *  The CPLD follows the NEXT BOOT PARTITION from the CPU_RESET Register
 *  only when the reset is either a watchdog reset or a S/W reset. In all
 *  other kinds of resets, CPU_RESET_STATUS Register is followed.
 */

u_int8_t 
fx_cpld_get_reset_type (void)
{
    u_int8_t reset_type = 0;
    uint8_t *cpld_base = (uint8_t *)fx_get_boot_cpld_base();

    reset_type = fx_cpld_read(cpld_base, CPU_RESET_STATUS_REG_OFFSET);
    reset_type &= RST_STATUS_REG_RST_TYPE_MASK;
    return (reset_type);
}

u_int8_t 
fx_cpld_get_current_partition (void)
{
    uint8_t *cpld_base = (uint8_t *)fx_get_boot_cpld_base();

    u_int8_t partition = fx_cpld_read(cpld_base, CPU_RESET_STATUS_REG_OFFSET);
    partition = ((partition & RST_STATUS_REG_PARTITION_NUM_MASK) >> 5);
    debug("CURRENT PARTITION: %d\n", partition);
    return (partition);
}

void 
fx_cpld_set_next_boot_partition (u_int8_t partition)
{
    uint8_t *cpld_base = (uint8_t *)fx_get_boot_cpld_base();

    debug("Setting CPLD Next boot partition to: 0x%x\n", partition);
    fx_cpld_write(cpld_base, CPU_RESET_REG_OFFSET, (partition << 1));
}

void 
fx_cpld_issue_sw_reset (void)
{
    uint8_t *cpld_base = (uint8_t *)fx_get_boot_cpld_base();
    uint8_t buf = fx_cpld_read(cpld_base, CPU_RESET_REG_OFFSET);

    /*
     * The Boot Key needs to be set before issuing a S/W reset since
     * the board has to be in a booted state for the reset to be
     * handled by the CPLD.
     */

    fx_cpld_set_bootkey();
    
    /* turn on reset enable bit */
    fx_cpld_write(cpld_base, CPU_RESET_EN_REG_OFFSET, 0x1);

    debug("\nIssuing a Software reset.. will boot from partition: %x\n",
                  (buf) ? (buf - 1) : (buf));
    if (buf) {
        if (fx_cpld_write(cpld_base, CPU_RESET_REG_OFFSET, 0x3) == -1) {
            printf("\nError: Cannot Reset through CPLD!!\n");
        }     
    } else {
        if (fx_cpld_write(cpld_base, CPU_RESET_REG_OFFSET, 0x1) == -1) {
            printf("\nError: Cannot Reset through CPLD!!\n");
        }    
    }
    /*
     * Its observed that CPLD is not issuing a reset but 
     * that happens very rarely. Keeping this print
     * to see if we encounter this problem anytime.
     */
    printf("\n FATAL: CPLD unable to RESET with data: 0x%x to Reg: 0x%x\n",
           (buf) ? 0x03 : 0x01, CPU_RESET_REG_OFFSET);
    /*
     * Extra lines so that console reset will not get
     * rid of the above line.
     */
    printf("\n\n\n\n\n");
}

/* The watchdog functionality of the CPLD is used after the u-boot
 * is booted successfully. Any watchdog reset can assume that the
 * u-boot is good and depending on the boot_status can determine if
 * the jloader caused the reset or the kernel. The watchdog functionality
 * needs to be exported to the jloader
 */

void 
fx_cpld_enable_watchdog (u_int8_t enable, u_int8_t value)
{
    uint8_t *cpld_base = (uint8_t *)fx_get_boot_cpld_base();

    if (!fx_cpld_disable_wd) {
        if (enable) {
            debug("\nEnabling the watchdog with timer value: 0x%x\n",
                          value);
        } else {
            debug("\nDisabling the watchdog!!\n");
        }	
        fx_cpld_write(cpld_base, WATCHDOG_REG_OFFSET, 
                      ((enable << 7) | (value & WATCHDOG_REG_VALUE_MASK)));
    }
}

uint8_t
fx_cpld_get_full_flash_mode (void)
{
    uint8_t *cpld_base = (uint8_t *)fx_get_boot_cpld_base();

    return(fx_cpld_read(cpld_base, FULL_FLASH_MODE_REG_OFFSET));
}

void 
fx_cpld_enable_full_flash_mode (void)
{
    uint8_t *cpld_base = (uint8_t *)fx_get_boot_cpld_base();

    fx_cpld_write(cpld_base, FULL_FLASH_MODE_REG_OFFSET, 0x1);
    debug("\nEnabling the FULL FLASH MODE!!\n");
}

void
fx_cpld_disable_full_flash_mode (void)
{
    uint8_t *cpld_base = (uint8_t *)fx_get_boot_cpld_base();

    fx_cpld_write(cpld_base, FULL_FLASH_MODE_REG_OFFSET, 0x0);
    debug("\ndisabling the FULL FLASH MODE!!\n");
}

u_int8_t 
fx_cpld_get_version (void)
{
    if (fx_is_tor()) {
        return (soho_cpld_get_version());
    }

    if (fx_is_cb()) {
        return (cb_cpld_get_version());
    }

    return (0xFF);
}

void
reload_watchdog (int32_t val)
{
    switch (val) {
    case PAT_WATCHDOG:
        /* Do not PAT the CPLD watchdog as this is used as a timer
         * instead of a watchdog. The timer needs to be disabled only
         * when the booting of the loader is complete.
         */ 
        return;
        break;

    case DISABLE_WATCHDOG:
        
        /* The watchdog is disabled only in the following situation:
         * 1. When user inputs a key to enter the loader prompt.
         *
         * When the kernel starts booting, we set the WD to MAX
         * to be able to detect any failures in the kernel.
         */
        if (get_boot_status() == BS_LOADER_BOOTING_KERNEL) {
            debug("\nSetting WD to MAX since Kernel is booting!!\n");
            fx_cpld_enable_watchdog(1, MAX_WATCHDOG_TIMER_VALUE);
        }
        if (get_boot_status() == BS_LOADER_DROPPED_TO_PROMPT) {
            debug("\nDisabling WD to fall to loader prompt!!\n");
            fx_cpld_enable_watchdog(0, 0);
        }
        break;

    case ENABLE_WATCHDOG:

        /* We use the reload_watchdog for enable only when the execution jumps
         * from loader to the kernel. The only other place we need to enable
         * the watchdog is when loader is started but that is directly set from
         * the CPLD functions
         */
        if (get_boot_status() == BS_LOADER_BOOTING_KERNEL) {
            debug("\nEnabling WD to MAX since Kernel is booting!!\n");
            fx_cpld_enable_watchdog(1, MAX_WATCHDOG_TIMER_VALUE);
        }
        if (get_boot_status() == BS_UBOOT_BOOTING) {
            debug("\nSetting WD to loader WD Time since loading the loader!!\n");
            fx_cpld_enable_watchdog(1, LOADER_WATCHDOG_TIMER_VALUE);
        }
        break;

    default:
        /* Do Nothing */
        break;
    }
}

u_int8_t
fx_cpld_init_dev (void)
{
    if (fx_is_cb()) {
        return (cb_cpld_init_dev());
    } else if (fx_is_tor()) {
        return (soho_cpld_init_dev());
    }
}

void
fx_cpld_dump (void)
{
    boolean init_done;
    uint8_t val = 0;
    uint8_t *cpld_base = (uint8_t *)fx_get_boot_cpld_base();

    printf("\nFX CPLD STATUS\n");
    printf("*******************\n");

    /* Set the init_done to TRUE so that the 
     * actual information is read from the 
     * CPLD registers instead of bypassing it.
     * Restore the value back at the end of this
     * routine
     */ 
    
    init_done = fx_cpld_init_done;
    fx_cpld_init_done = TRUE;

    printf("\nCPLD Version: %d\n", fx_cpld_get_version());

    val = fx_cpld_read(cpld_base, BOOT_KEY_REG_OFFSET);
    if (val == BOOT_KEY_VALUE) {
        printf("\nThe BOOT KEY is set\n");
    } else {
        printf("\nThe BOOT KEY is NOT set\n");
    }

    val = fx_cpld_read(cpld_base, FULL_FLASH_MODE_REG_OFFSET);
    if (val == 0x1) {
        printf("\nEntire 8MB of flash is open\n");
    } else {
        printf("\nEntire Flash is not enabled\n");
    }

    printf("\nLast Reboot Reason:"); 
    val = fx_cpld_get_reset_type();
    switch (val) {
    case COLD_BOOT_RESET:
        printf("COLD BOOT\n");
        break;

    case PUSH_BUTTON_RESET:
        printf("PUSH BUTTON RESET\n");
        break;

    case SOFTWARE_RESET:
        printf("SOFTWARE RESET\n");
        break;

    case WATCHDOG_TIMER_RESET:
        printf("WATCHDOG RESET\n");
        break;

    case CPLD_RESET:
        printf("CPLD RESET\n");
        break;

    default:
        printf("Unknown reset\n");
        break;
    }

    printf("\nCurrent Partition: %d\n", fx_cpld_get_current_partition());

    val = fx_cpld_read(cpld_base, CPU_RESET_REG_OFFSET);
    printf("\nCPU_RESET_REG: 0x%x\n", val);
    printf("\nNext Boot Partition: %d\n", (val >> 1));

    val = fx_cpld_read(cpld_base, WATCHDOG_REG_OFFSET);
    if (val & 0x80) {
        printf("\nWatchdog is Enabled with 0x%x units remaining\n",
               (val & 0x7F));
    } else {
        printf("\nWatchdog is disabled\n");
    }

    fx_cpld_init_done = init_done;
}

void fx_cpld_reset_to_next_partition (void)
{
    uint8_t cur_partition = fx_cpld_get_current_partition();
    uint8_t next_partition = get_next_partition(cur_partition);

    printf("\nCurrent partition: %d\n", cur_partition);

    printf("\nResetting to boot from partition: %d !!!!!\n\n\n", 
            next_partition);
    fx_cpld_set_next_boot_partition(next_partition);
    fx_cpld_issue_sw_reset();   
}

void
fx_cpld_qsfp_enable (void)
{
    uint8_t cpld_sfp_base = 0x00;

    soho_cpld_write(QSFP_PHY_RST_REG0_OFFSET, 0xff);
    soho_cpld_write(QSFP_PHY_RST_REG1_OFFSET, 0xff);

    soho_cpld_write(QSFP_PHY_RST_REG0_OFFSET, 0x00);
    soho_cpld_write(QSFP_PHY_RST_REG1_OFFSET, 0x00);

    udelay(FX_QSFP_RESET_DELAY);

    for (cpld_sfp_base= QSFP0_SLAVE_REG_OFFSET; cpld_sfp_base <
         QSFP7_SLAVE_REG_OFFSET; cpld_sfp_base++)
        soho_cpld_write(cpld_sfp_base, 0x20);

    for (cpld_sfp_base= QSFP8_SLAVE_REG_OFFSET ; cpld_sfp_base <
         QSFP15_SLAVE_REG_OFFSET; cpld_sfp_base++)
        soho_cpld_write(cpld_sfp_base, 0x20);

}

