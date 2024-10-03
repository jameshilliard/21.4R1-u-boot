/*
 * Copyright (c) 2009-2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <common.h>
#include <command.h>
#include "bootcpld_tbb_re_sw.h"
#include "bootcpld_tbb_re_sw_struct.h"

int tbbcpld_valid_read_reg(int reg)
{
    if (reg <= BOOTCPLD_TBB_RE_SPI_RE_WDATH_ADDR)
        return 1;
    else 
        return 0;
}

int tbbcpld_valid_write_reg(int reg)
{
    if ((reg <= BOOTCPLD_TBB_RE_SPI_RE_WDATH_ADDR) &&
        (reg != BOOTCPLD_TBB_RE_RESET_REASON_ADDR))
        return 1;
    else 
        return 0;
}

int tbbcpld_reg_read(int reg, uint8_t *val)
{
    volatile uint8_t *tbbcpld = (uint8_t *)CFG_TBBCPLD_BASE;
    
    if (tbbcpld_valid_read_reg(reg)) {
        *val = tbbcpld[reg];
        return 1;
    }
    return 0;
}

int tbbcpld_reg_write(int reg, uint8_t val)
{
    volatile uint8_t *tbbcpld = (uint8_t *)CFG_TBBCPLD_BASE;
    
    if (tbbcpld_valid_write_reg(reg)) {
        tbbcpld[reg] = val;
        return 1;
    }
    return 0;
}

int tbbcpld_version(void)
{
    uint8_t version;
    if (tbbcpld_reg_read(BOOTCPLD_TBB_RE_CPLD_REV_ADDR, &version))
        return version;
    else
        return 0;
}

int tbbcpld_soft_reset(void)
{
#if 0
    volatile bootcpld_tbb_re *tbbcpld = (bootcpld_tbb_re *)CFG_TBBCPLD_BASE;
#endif
    return 1;
}

int tbbcpld_powercycle_reset(void)
{
    volatile bootcpld_tbb_re *tbbcpld = (bootcpld_tbb_re *)CFG_TBBCPLD_BASE;

    tbbcpld->RESET_REGISTER |= BOOTCPLD_TBB_RE_RESET_REGISTER_POWER_CYCLE;
    return 1;
}

int tbbcpld_system_reset(void)
{
    volatile bootcpld_tbb_re *tbbcpld = (bootcpld_tbb_re *)CFG_TBBCPLD_BASE;

    tbbcpld->RESET_REGISTER |= BOOTCPLD_TBB_RE_RESET_REGISTER_MAIN_RESET;

    return 1;
}

void tbbcpld_eeprom_enable(void)
{
    volatile bootcpld_tbb_re *tbbcpld = (bootcpld_tbb_re *)CFG_TBBCPLD_BASE;

    tbbcpld->CONTROL_REGISTER |= BOOTCPLD_TBB_RE_CONTROL_REGISTER_P8572_I2C_EEPROM_EN;
}

void tbbcpld_eeprom_disable(void)
{
    volatile bootcpld_tbb_re *tbbcpld = (bootcpld_tbb_re *)CFG_TBBCPLD_BASE;

    tbbcpld->CONTROL_REGISTER &= ~BOOTCPLD_TBB_RE_CONTROL_REGISTER_P8572_I2C_EEPROM_EN;
}


void tbbcpld_watchdog_disable(void)
{
    volatile bootcpld_tbb_re *tbbcpld = (bootcpld_tbb_re *)CFG_TBBCPLD_BASE;

    tbbcpld->CONTROL_REGISTER &= ~BOOTCPLD_TBB_RE_CONTROL_REGISTER_WDT_ENA;
}

void tbbcpld_watchdog_enable(void)
{
    volatile bootcpld_tbb_re *tbbcpld = (bootcpld_tbb_re *)CFG_TBBCPLD_BASE;

    tbbcpld->CONTROL_REGISTER |= BOOTCPLD_TBB_RE_CONTROL_REGISTER_WDT_ENA;
}

void tbbcpld_watchdog_tickle(void)
{
    volatile bootcpld_tbb_re *tbbcpld = (bootcpld_tbb_re *)CFG_TBBCPLD_BASE;

    /* Write zero to watchdog lbyte to restart wdog timer */
    tbbcpld->WTCHDG_LBYTE_REG = 0x00;
}


void tbbcpld_watchdog_init(void)
{
    return;
}

void tbbcpld_swizzle_enable(void)
{
    volatile bootcpld_tbb_re *tbbcpld = (bootcpld_tbb_re *)CFG_TBBCPLD_BASE;

    tbbcpld->CONTROL_REGISTER |= BOOTCPLD_TBB_RE_CONTROL_REGISTER_FLASH_SWIZZLE_ENA;
}

void tbbcpld_swizzle_disable(void)
{
    volatile bootcpld_tbb_re *tbbcpld = (bootcpld_tbb_re *)CFG_TBBCPLD_BASE;

    tbbcpld->CONTROL_REGISTER &= ~BOOTCPLD_TBB_RE_CONTROL_REGISTER_FLASH_SWIZZLE_ENA;
}

void tbbcpld_scratchpad_write(u_int8_t scratch1, u_int8_t scratch2)
{
    volatile bootcpld_tbb_re *tbbcpld = (bootcpld_tbb_re *)CFG_TBBCPLD_BASE;

    tbbcpld->SCRATCH_1 = scratch1;
    tbbcpld->SCRATCH_2 = scratch2;
}

void tbbcpld_scratchpad_read(u_int8_t *scratch1, u_int8_t *scratch2)
{
    volatile bootcpld_tbb_re *tbbcpld = (bootcpld_tbb_re *)CFG_TBBCPLD_BASE;

    *scratch1 = tbbcpld->SCRATCH_1;
    *scratch2 = tbbcpld->SCRATCH_2;
}

void tbbcpld_init(void)
{
    volatile bootcpld_tbb_re *tbbcpld = (bootcpld_tbb_re *)CFG_TBBCPLD_BASE;

    /*
     * Take all devices out of reset (except uKern CPLD)
     * DUART reset is active low.
     */
    tbbcpld->RESET_REGISTER = BOOTCPLD_TBB_RE_RESET_REGISTER_UKERN_BOOTCPLD_RESET
                              | BOOTCPLD_TBB_RE_RESET_REGISTER_DUART_RESET;
    udelay(1000); /* 1 ms : Just paranoid */

    /*
     * Take PCIe-to-PCI bridge out of reset.
     * Don't go by the bit name. Clear bit to get
     * the bridge out of reset.
     */
    tbbcpld->CONTROL_REGISTER_2 &= 
	         ~BOOTCPLD_TBB_RE_CONTROL_REGISTER_2_P8572_PCI_RST_L;
    udelay(1000); /* 1 ms : Just paranoid */

    /* Set RE LED blinking */
    tbbcpld->LED_SYSTEM_OK = BOOTCPLD_TBB_RE_LED_SYSTEM_OK_RE_COMING_UP;
}


void tbbcpld_reg_dump_all(void)
{
    int i;
    volatile uint8_t *tbbcpld = (uint8_t *)CFG_TBBCPLD_BASE;

    for (i=0; i < BOOTCPLD_TBB_RE_SPI_RE_WDATH_ADDR; i++)
	printf("tbbcpld[0x%x] = 0x%x\n", i, tbbcpld[i]);
}


/* tbb cpld commands
 *
 */
int do_tbbcpld(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{

    char cmd1;
    uint32_t reg;
    u_int8_t  value;
    
    if (argc < 2)
        goto usage;
    
    cmd1 = argv[1][0];
    
    switch (cmd1) {
        case 'd':       /* dump */
        case 'D':
            tbbcpld_reg_dump_all();
            break;

        case 'r':       /* read */
        case 'R':
            if (argc <= 3)
                goto usage;
            
            reg = simple_strtoul(argv[2], NULL, 16);
            if (!tbbcpld_reg_read(reg, &value)) {
                printf("tbb cpls reg write failed\n");
            } else {
	        printf("tbbcpld[0x%x] = 0x%x\n", reg, value);
            }
            break;
                        
        case 'w':       /* write */
        case 'W':
            if (argc <= 4)
                goto usage;
            
            reg = simple_strtoul(argv[2], NULL, 16);
            value = simple_strtoul(argv[3], NULL, 16);
            if (!tbbcpld_reg_write(reg, value)) {
                printf("tbb cpls reg write failed\n");
            }
            break;
                        
        case 'n':
	    set_noboot_flag();
            break;
                        
        default:
            printf("???");
            break;
    }

    return 1;
 usage:
    printf("Usage:\n%s\n", cmdtp->usage);
    return 1;
}
/***************************************************/

U_BOOT_CMD(
    tbbcpld,    7,    1,     do_tbbcpld,
    "tbbcpld - tbb cpld register read/write utility\n",
    "\n"
    "tbbcpld read <addr> <count>\n"
    "    - read # regs from tbb cpld \n"
    "tbbcpld write <addr> <val> \n"
    "    - write <val> to tbb cpld register \n"
    "tbbcpld dump \n"
    "    - tbb cpld register dump \n"
    "tbbcpld noboot \n"
    "    - set noboot mask \n"
);
