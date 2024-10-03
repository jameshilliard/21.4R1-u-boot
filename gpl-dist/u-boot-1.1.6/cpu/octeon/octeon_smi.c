/*
 * Copyright (c) 2007-2011, Juniper Networks, Inc.
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

#include <common.h>
#include <octeon_boot.h>
#include <octeon_smi.h>
#include <dxj106.h>

uint16_t 
octeon_smi_write (uint16_t reg_adr, uint16_t dat)
{
    uint16_t    smi_retry_count = 0;
    cvmx_smi_cmd_t    smi_cmd;
    cvmx_smi_rd_dat_t smi_rd_data;
    cvmx_smi_wr_dat_t smi_wr_data;
    uint16_t    phy_adr = OCTEON_DXJ106_SMI_ADDR;

    /* Check register address validity */
    if (reg_adr > OCTEON_MAX_VALID_SMI_REG_ADDR) {
        printf("SMI: Invalid register 0x%x to write\n", reg_adr);
        return 1;
    }


    /* write data to SMI_WR_DAT */
    smi_wr_data.s.dat = dat;
    cvmx_write_csr(CVMX_SMI_WR_DAT, smi_wr_data.u64);

    /* write the SMI_CMD register */
    smi_cmd.s.phy_adr = phy_adr;
    smi_cmd.s.reg_adr = reg_adr;
    smi_cmd.s.phy_op = 0; /* write op */

    cvmx_write_csr(CVMX_SMI_CMD, smi_cmd.u64);

    /* check for write data valid */
    do {
        udelay(1000);
        smi_retry_count++;
        smi_rd_data.u64 = (unsigned long long)cvmx_read_csr(CVMX_SMI_WR_DAT);
        if (smi_rd_data.s.val) {
            break;
        }
    } while (smi_retry_count < OCTEON_MAX_SMI_RETRY_COUNT);

    if (smi_retry_count >= OCTEON_MAX_SMI_RETRY_COUNT) {
        printf("SMI: Write operation timed out for register 0x%x\n", reg_adr);
        return 2;
    }
     
    return 0;
}

uint16_t 
octeon_smi_read (uint16_t reg_adr, uint16_t *val)
{
    cvmx_smi_cmd_t    smi_cmd;
    cvmx_smi_rd_dat_t smi_rd_data;
    uint16_t    smi_retry_count = 0;
    uint16_t    phy_adr = OCTEON_DXJ106_SMI_ADDR;

    /* Check register address validity */
    if (reg_adr > OCTEON_MAX_VALID_SMI_REG_ADDR) {
        printf("SMI: Invalid register 0x%x to read\n", reg_adr);
        return 1;
    }

    smi_cmd.s.phy_adr = phy_adr;
    smi_cmd.s.reg_adr = reg_adr;
    smi_cmd.s.phy_op = 1; /* read op */

    cvmx_write_csr(CVMX_SMI_CMD, smi_cmd.u64);
    
    do {
        udelay(1000);
        smi_retry_count++;
        smi_rd_data.u64 = (unsigned long long)cvmx_read_csr(CVMX_SMI_RD_DAT);
        if (smi_rd_data.s.val) {
            break;
        }
    } while (smi_retry_count < OCTEON_MAX_SMI_RETRY_COUNT);

    if (smi_retry_count >= OCTEON_MAX_SMI_RETRY_COUNT) {
        printf("SMI: Read operation timed out for register 0x%x\n", reg_adr);
        return 2;
    }

    *val =  (0xFFFF & (uint16_t)smi_rd_data.s.dat);

    return 0;
}

static void
octeon_set_smi_target (uint8_t dev)
{

    /* Enable bit for output */
    cvmx_write_csr(CVMX_GPIO_BIT_CFGX(OCTEON_SMI_SEL_BIT),
                   OCTEON_GPIO_BIT_CFG_TX_OE);

    if (dev == OCTEON_SMI_DEV_SWITCH) {
        /* Drive low to select switch */
        cvmx_write_csr(CVMX_GPIO_TX_CLR,OCTEON_SMI_SEL_BIT_MASK);
    } else {
        /* Drive high to select mPIM */
        cvmx_write_csr(CVMX_GPIO_TX_SET,OCTEON_SMI_SEL_BIT_MASK);
    }
    
    cvmx_write_csr(CVMX_GPIO_BIT_CFGX(OCTEON_SMI_SEL_BIT),0);
}

int 
octeon_smi_init (void)
{
    unsigned long long val = 0;

    /* select cavium SMI */
    octeon_set_smi_target(OCTEON_SMI_DEV_SWITCH);
    
    val = (unsigned long long)cvmx_read_csr(CVMX_SMI_EN);
    val |= 0x1; /* Enable SMI transcations */
    cvmx_write_csr(CVMX_SMI_EN, val);

    return 0;
}

int
octeon_smi_set_clock(void)
{
    unsigned long long val = 0;
    unsigned int       cpu_clk_mhz, phase;

    DECLARE_GLOBAL_DATA_PTR;

    val = (unsigned long long)cvmx_read_csr(CVMX_SMI_CLK);
    
    val &= 0xFFFFFFFFFFFFFF00;

#ifdef CONFIG_JSRXNLE
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX210_MODELS
        cpu_clk_mhz = gd->cpu_clock_mhz;
        phase = (cpu_clk_mhz * 1000) / (MPIM_OCTEON_SW_MAX_SMI_CLK_KHZ * 2) + 1;
        val = MPIM_OCTEON_SAMPLE | phase;
        break;
    CASE_ALL_SRX220_MODELS
        val = OCTEON_SRX220_SMI_CLOCK_VAL;
        break;
    default:
        val = OCTEON_SMI_CLOCK_VAL;
        break;
    }
#endif
    cvmx_write_csr(CVMX_SMI_CLK, val);

    return 0;
}

