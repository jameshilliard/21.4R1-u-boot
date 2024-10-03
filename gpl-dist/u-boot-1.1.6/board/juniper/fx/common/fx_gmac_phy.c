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

#include "rmi/rmigmac.h"
#include "rmi/gmac.h"
#include "rmi/on_chip.h"
#include "rmi/xlr_cpu.h"
#include "fx_common.h"

#define GMAC_DEBUG
#define PHY_STATUS_RETRIES         25000
#define R_INT_MASK_REG             25
#define RMI_GMAC_PORTS_PER_CTRL    4
#define READ_COMMAND               1
#define MGMT_CLOCK_SELECT          0x07 /* divisor = 28 */

extern struct rmi_board_info rmi_board;
static gmac_eth_info_t *recent_phy;
extern u_int8_t config_phy_flag;
extern u_int8_t autoneg_flag;

enum {
    SERDES_CTRL_REG_PHY_ID = 26,
    SGMII_PCS_GMAC0_PHY_ID,
    SGMII_PCS_GMAC1_PHY_ID,
    SGMII_PCS_GMAC2_PHY_ID,
    SGMII_PCS_GMAC3_PHY_ID,
};

uint32_t cb_sgmii_phy_id_map[] = { 0x3, 0X2, 0x1, 0xFF, 0x0, 0xFF, 0xFF, 0xFF};

uint32_t soho_sgmii_phy_id_map[] = {0x1E, 0x1E, 0x1E, 0x1E, 0x1, 0x4, 0x3, 0xFF};


#define NR_PHYS    32
static gmac_eth_info_t *phys[NR_PHYS];

static unsigned int serdes_ctrl_regs[] = {
    [0] = 0x6db0,
    [1] = 0xffff,
    [2] = 0xb6d0,
    [3] = 0x00ff,
    [4] = 0x0000,
    [5] = 0x0000,
    [6] = 0x0005,
    [7] = 0x0001,
    [8] = 0x0000,
    [9] = 0x0000,
    [10] = 0x0000,
};

#define NUM_SERDES_CTRL_REGS    ((int)sizeof(serdes_ctrl_regs) / \
                                 sizeof(unsigned int))

unsigned long gmac_mmio[RMI_GMAC_TOTAL_PORTS] = {
    [0] = (DEFAULT_RMI_PROCESSOR_IO_BASE + PHOENIX_IO_GMAC_0_OFFSET),
    [1] = (DEFAULT_RMI_PROCESSOR_IO_BASE + PHOENIX_IO_GMAC_1_OFFSET),
    [2] = (DEFAULT_RMI_PROCESSOR_IO_BASE + PHOENIX_IO_GMAC_2_OFFSET),
    [3] = (DEFAULT_RMI_PROCESSOR_IO_BASE + PHOENIX_IO_GMAC_3_OFFSET),
    [4] = (DEFAULT_RMI_PROCESSOR_IO_BASE + PHOENIX_IO_GMAC_4_OFFSET),
    [5] = (DEFAULT_RMI_PROCESSOR_IO_BASE + PHOENIX_IO_GMAC_5_OFFSET),
    [6] = (DEFAULT_RMI_PROCESSOR_IO_BASE + PHOENIX_IO_GMAC_6_OFFSET),
    [7] = (DEFAULT_RMI_PROCESSOR_IO_BASE + PHOENIX_IO_GMAC_7_OFFSET),
};

enum xlr_tr_reg {
    XLR_GMAC_TR64 = 0x20,
    XLR_GMAC_TR64_127,
    XLR_GMAC_TR128_255,
    XLR_GMAC_TR256_511,
    XLR_GMAC_TR512_1023,
    XLR_GMAC_TR1024_1518,
    XLR_GMAC_TR1519_1522_VLAN,

    XLR_GMAC_RX_BYTES = 0x27,
    XLR_GMAC_RX_PKT,
    XLR_GMAC_RX_FCS_ERR,
    XLR_GMAC_RX_MC_PKT,
    XLR_GMAC_RX_BC_PKT,
    XLR_GMAC_RX_CTRL_PKT,
    XLR_GMAC_RX_PAUSE_PKT,
    XLR_GMAC_RX_UNK_OP,
    XLR_GMAC_RX_ALGN_ERR,
    XLR_GMAC_RX_LEN_ERR,
    XLR_GMAC_RX_CODE_ERR,
    XLR_GMAC_RX_CSENSE_ERR,
    XLR_GMAC_RX_UNDSZ,
    XLR_GMAC_RX_OVERSZ,
    XLR_GMAC_RX_FRG,
    XLR_GMAC_RX_JBR,
    XLR_GMAC_RX_DRP,

    XLR_GMAC_TX_BYTES = 0x38,
    XLR_GMAC_TX_PKT,
    XLR_GMAC_TX_MC_PKT,
    XLR_GMAC_TX_BC_PKT,
    XLR_GMAC_TX_PAUSE_PKT,
    XLR_GMAC_TX_DEF_PKT,
    XLR_GMAC_TX_EXC_DEF_PKT,
    XLR_GMAC_TX_SINGLE_CLSN_PKT,
    XLR_GMAC_TX_MULTI_CLSN_PKT,
    XLR_GMAC_TX_LATE_CLSN_PKT,
    XLR_GMAC_TX_EXC_CLSN_PKT,
    XLR_GMAC_TX_TOTAL_CLSN_PKT,
    XLR_GMAC_TX_PAUSEF_HONERED_PKT,
    XLR_GMAC_TX_DRP,
    XLR_GMAC_TX_JBR,
    XLR_GMAC_TX_FCS_ERR,
    XLR_GMAC_TX_CTRL_PKT,
    XLR_GMAC_TX_OVERSZ,
    XLR_GMAC_TX_UNDSZ,
    XLR_GMAC_TX_FRG,

    XLR_GMAC_CARRY_REG_1 = 0x4c,
    XLR_GMAC_CARRY_REG_2 = 0x4d,
};

static uint32_t
fx_map_gmac_phy_id (uint32_t port)
{
    if (port < 0 || port > 7) {
        return 0xFFFF;
    }

    if (fx_is_cb()) {
        return cb_sgmii_phy_id_map[port];
    }

    if (fx_is_tor()) {
        return soho_sgmii_phy_id_map[port];
    }

    return 0xFFFF;
}


uint32_t
fx_get_gmac_phy_id (unsigned int mac, int type)
{
    return fx_map_gmac_phy_id(mac);
}


uint32_t
fx_get_gmac_mii_addr (unsigned int mac, int type)
{
    uint32_t addr = 0;
    struct rmi_board_info *rmib = &rmi_board;

    if (fx_is_cb()) {
        addr = gmac_mmio[0];
    }

    if (fx_is_tor()) {
        if (fx_is_wa() && wa_is_rev1())
           addr = gmac_mmio[0];
        else 
           addr = gmac_mmio[4];
    }
    return addr;
}


/* called only for sgmii ports */

uint32_t
fx_get_gmac_serdes_addr (unsigned int mac)
{
    unsigned long addr;

    if (mac < RMI_GMAC_PORTS_PER_CTRL) {
        addr = gmac_mmio[0];
    } else {
        if (fx_is_wa() && wa_is_rev1())
          addr = gmac_mmio[0];
        else
          addr = gmac_mmio[4];
    }
    return addr;
}


uint32_t
fx_get_gmac_pcs_addr (unsigned int mac)
{
    return fx_get_gmac_serdes_addr(mac);
}

/* args : gmac index 0 to 7 */
uint32_t
fx_get_gmac_mmio_addr (unsigned int mac)
{
    return gmac_mmio[mac];
}

int32_t
gmac_phy_mdio_read
(phoenix_reg_t *mmio, int phy_id, int reg, uint32_t *data)
{
    int retVal = 0;
    int i = 0;

    /* setup the phy reg to be read */
    phoenix_write_reg(mmio, MIIM_ADDR, (phy_id << 8) | reg);

    /* perform a read cycle */
    phoenix_write_reg(mmio, MIIM_CMD, READ_COMMAND);

    /* poll for the read cycle to complete */
    for (i = 0; i < PHY_STATUS_RETRIES; i++) {
        udelay(10 * 1000);
        if (phoenix_read_reg(mmio, MIIM_IND) == 0) {
            break;
        }
    }

    /* clear the read cycle */
    phoenix_write_reg(mmio, MIIM_CMD, 0);

    if (i == PHY_STATUS_RETRIES) {
        *data = 0xffffffff;
        printf("Retries failed.\n");
        goto out;
    }

    /* return the read data */
    *data = (uint32_t)phoenix_read_reg(mmio, MIIM_STAT);
    retVal = 1;
out:
    return retVal;
}


int32_t
gmac_phy_mdio_write
(phoenix_reg_t *mmio, int phy_id, int reg, uint32_t data)
{
    int i = 0;

    /* setup the phy reg to be read */
    phoenix_write_reg(mmio, MIIM_ADDR, (phy_id << 8) | reg);

    /* write the data which starts the write cycle */
    phoenix_write_reg(mmio, MIIM_CTRL, data);

    /* poll for the write cycle to complete */
    for (i = 0; i < PHY_STATUS_RETRIES; i++) {
        udelay(10 * 1000);
        if (phoenix_read_reg(mmio, MIIM_IND) == 0) {
            break;
        }
    }

    /* clear the write data/cycle */
    /* phoenix_write_reg(mmio, MIIM_CTRL, 0); */

    if (i == PHY_STATUS_RETRIES) {
        return 0;
    }

    return 1;
}


int
gmac_phy_read (gmac_eth_info_t *this_phy, int reg, uint32_t *data)
{
    recent_phy = this_phy;

    return gmac_phy_mdio_read((phoenix_reg_t *)this_phy->mii_addr,
                              this_phy->phy_id,
                              reg,
                              data);
}


int
gmac_phy_write (gmac_eth_info_t *this_phy, int reg, uint32_t data)
{
    return gmac_phy_mdio_write((phoenix_reg_t *)this_phy->mii_addr,
                               this_phy->phy_id,
                               reg,
                               data);
}


void
gmac_clear_phy_interrupt_mask (gmac_eth_info_t *this_phy)
{
    uint32_t value;

    gmac_phy_read(this_phy, 26, &value);
}


void
gmac_set_phy_interrupt_mask (gmac_eth_info_t *this_phy, uint32_t mask)
{
    gmac_phy_write(this_phy, 25, mask);
}


void
gmac_init_serdes_ctrl_regs (uint32_t mac)
{
    uint32_t data = 0, mmio;
    int phy_id = SERDES_CTRL_REG_PHY_ID;
    int reg = 0;

    mmio = fx_get_gmac_serdes_addr(mac);

    /* initialize SERDES CONTROL Registers */
    for (reg = 0; reg < NUM_SERDES_CTRL_REGS; reg++) {
        data = serdes_ctrl_regs[reg];
        gmac_phy_mdio_write((phoenix_reg_t *)mmio, phy_id, reg, data);
    }

    return;
}


void
gmac_phy_init (gmac_eth_info_t *this_phy)
{
    uint32_t phy_id, mii_mmio;
    uint32_t mac, pcs_mmio, value;
    int nr_val, isxls = chip_processor_id_xls();
    int iteration_count  = 0x0;

    phy_id = this_phy->phy_id;
    mii_mmio = this_phy->mii_addr;
    mac = this_phy->port_id;

    if (isxls) {
        nr_val = NR_PHYS;
    } else {
        nr_val = 4;
    }

    if (phy_id < nr_val) {
        /* lower the mgmt clock freq to minimum possible freq */
        phoenix_write_reg((phoenix_reg_t *)mii_mmio,
                          MIIM_CONF,
                          MGMT_CLOCK_SELECT);
        /* Bring out the MAC controlling this phy out of reset. */
        phoenix_write_reg((phoenix_reg_t *)mii_mmio, MAC_CONF1, 0x35);
        phys[phy_id] = this_phy;
    } else {
        printf("Cannot add phy %d to the list.\n", phy_id);
    }

    if (!isxls) {
        return;
    }

    gmac_init_serdes_ctrl_regs(mac);

    if (mac >= RMI_GMAC_PORTS_PER_CTRL) {
        mac -= RMI_GMAC_PORTS_PER_CTRL;
    }

    pcs_mmio = this_phy->pcs_addr;

    if (!pcs_mmio) {
        return;
    }

    gmac_phy_mdio_read((phoenix_reg_t *)pcs_mmio,
                       SGMII_PCS_GMAC0_PHY_ID + mac,
                       0x4,
                       &value);

    value |= 1;
    gmac_phy_mdio_write((phoenix_reg_t *)pcs_mmio,
                        SGMII_PCS_GMAC0_PHY_ID + mac,
                        0x4,
                        value);
    udelay(100 * 1000);

    if (autoneg_flag) {
        if (fx_is_cb() || ((fx_is_tor()) && this_phy->port_id >= 4)) {
            /* Enable auto-neg */
            gmac_phy_mdio_write((phoenix_reg_t *)pcs_mmio,
                                SGMII_PCS_GMAC0_PHY_ID + mac,
                                0,
                                0x1000);
            udelay(100 * 1000);
            gmac_phy_mdio_write((phoenix_reg_t *)pcs_mmio,
                                SGMII_PCS_GMAC0_PHY_ID + mac,
                                0,
                                0x0200);
        }
    } 

    for (;;) {
        gmac_phy_mdio_read((phoenix_reg_t *)pcs_mmio,
                           SGMII_PCS_GMAC0_PHY_ID + mac,
                           1,
                           &value);
        if ( (value & 0x04) == 0x04) {
            this_phy->sgmii_linkup = 1;
            b2_debug("%s: %d linkup\n", __func__, this_phy->port_id);
            break;
        }

        if (iteration_count++ == 30) {
           b2_debug(
                "[%s]: Timed-out waiting for the SGMII Link Status for port=%u value=0x%x!!\n",
                __FUNCTION__,
                mac,
                value);
            break;
        }
    }
    return;
}


void
gmac_clear_phy_intr (gmac_eth_info_t *this_phy)
{
    phoenix_reg_t *mmio = NULL;
    gmac_eth_info_t *phy;
    int i, nr_val;
    int phy_value = 0;
    int isxls = chip_processor_id_xls();

    if (isxls) {
        nr_val = NR_PHYS;
    } else {
        nr_val = 4;
    }

    for (i = 0; i < nr_val; i++) {
        phy = phys[i];
        if (phy) {
            mmio = (phoenix_reg_t *)this_phy->mmio_addr;
            gmac_phy_read(phy, 26, &phy_value);
        }
    }
    phoenix_write_reg(mmio, IntReg, 0xffffffff);
}


void
rmi_dump_phy_reg (void)
{
    uint32_t data;
    u_int16_t reg;

    reg = 0;

    gmac_phy_read(recent_phy, 0x18, &data);
    printf("%s[%d]: misc_ctrl=0x%x\n", __func__, __LINE__, data);
    /* select read packet counters */
    gmac_phy_write(recent_phy, 0x17, 0xF00);

    /* read out packet counters */
    gmac_phy_read(recent_phy, 0x15, &data);

    printf("%s[%d]: data=0x%x\n", __func__, __LINE__, data);
}

#include <command.h>
/* debug functions */
int
do_rmigmac (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int mac = 0;
    int reg = 0;
    phoenix_reg_t *mmio;

    if ((mac < 0) || (mac > 7)) {
        printf("Invalid mac. \n");
        return 0;
    }

    mac = simple_strtoul(argv[1], 0, 10);
    reg = *argv[2];
    printf("\nreg dump for gmac%d:\n", mac);

    mmio = (phoenix_reg_t *)gmac_mmio[mac];

    printf("r%x:0x%08x\n", MAC_CONF1, phoenix_read_reg(mmio, MAC_CONF1));
    printf("r%x:0x%08x\n", MAC_CONF2, phoenix_read_reg(mmio, MAC_CONF2));
    printf("r%x:0x%08x\n", TxControl, phoenix_read_reg(mmio, TxControl));
    printf("r%x:0x%08x\n", RxControl, phoenix_read_reg(mmio, RxControl));
    printf("r%x:0x%08x\n", CoreControl, phoenix_read_reg(mmio, CoreControl));

    printf("\n");
    printf("r%x:0x%08x\n", SGMII_SPEED, phoenix_read_reg(mmio, SGMII_SPEED));
    printf("r%x:0x%08x\n", IntReg, phoenix_read_reg(mmio, IntReg));
    printf("r%x:0x%08x\n", IntMask, phoenix_read_reg(mmio, IntMask));
    printf("r%x:0x%08x\n", G_TX0_BUCKET_SIZE,
           phoenix_read_reg(mmio, G_TX0_BUCKET_SIZE));
    printf("r%x:0x%08x\n", G_RFR0_BUCKET_SIZE,
           phoenix_read_reg(mmio, G_RFR0_BUCKET_SIZE));
    printf("r%x:0x%08x\n", G_RFR1_BUCKET_SIZE,
           phoenix_read_reg(mmio, G_RFR1_BUCKET_SIZE));

    printf("\n");
    printf("r%x:0x%08x\n", G_JFR0_BUCKET_SIZE,
           phoenix_read_reg(mmio, G_JFR0_BUCKET_SIZE));
    printf("r%x:0x%08x\n", G_JFR1_BUCKET_SIZE,
           phoenix_read_reg(mmio, G_JFR1_BUCKET_SIZE));
    printf("r%x:0x%08x\n", DmaCr0, phoenix_read_reg(mmio, DmaCr0));
    printf("r%x:0x%08x\n", DmaCr2, phoenix_read_reg(mmio, DmaCr2));
    printf("r%x:0x%08x\n", DmaCr3, phoenix_read_reg(mmio, DmaCr3));

    printf("\n");
    printf("r%x:0x%08x\n", RegFrInSpillMemStart0,
           phoenix_read_reg(mmio, RegFrInSpillMemStart0));
    printf("r%x:0x%08x\n", RegFrInSpillMemSize,
           phoenix_read_reg(mmio, RegFrInSpillMemSize));
    printf("r%x:0x%08x\n", FrOutSpillMemStart0,
           phoenix_read_reg(mmio, FrOutSpillMemStart0));
    printf("r%x:0x%08x\n", FrOutSpillMemSize,
           phoenix_read_reg(mmio, FrOutSpillMemSize));

    printf("\n");
    printf("r%x:0x%08x\n", MAC_ADDR0_LO, phoenix_read_reg(mmio, MAC_ADDR0_LO));
    printf("r%x:0x%08x\n", MAC_ADDR0_HI, phoenix_read_reg(mmio, MAC_ADDR0_HI));
    printf("r%x:0x%08x\n", MAC_ADDR_MASK0_LO,
           phoenix_read_reg(mmio, MAC_ADDR_MASK0_LO));
    printf("r%x:0x%08x\n", MAC_ADDR_MASK0_HI,
           phoenix_read_reg(mmio, MAC_ADDR_MASK0_HI));
    printf("r%x:0x%08x\n", MAC_ADDR_MASK1_LO,
           phoenix_read_reg(mmio, MAC_ADDR_MASK1_LO));
    printf("r%x:0x%08x\n", MAC_ADDR_MASK1_HI,
           phoenix_read_reg(mmio, MAC_ADDR_MASK1_HI));
    printf("r%x:0x%08x\n", MAC_FILTER_CONFIG,
           phoenix_read_reg(mmio, MAC_FILTER_CONFIG));

    printf("RxDescInMAC  %d \n", mmio[0x23e]);
    printf("TxDescInMAC  %d \n", mmio[0x23f]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 0);
    printf("RegFrInSpillMem     %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 1);
    printf("RegFrInSpillTmpCnt  %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 2);
    printf("RegFrInFifoWordCnt  %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 3);
    printf("RegFrInSpillMem1    %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 4);
    printf("RegFrInSpillTmpCnt1 %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 5);
    printf("RegFrInFifoWordCnt1 %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 6);
    printf("Class0SpillMemBytes %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 7);
    printf("Class0SpillTmpCount %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 8);
    printf("Class0WordCount     %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 9);
    printf("AddrFifoWord0Count  %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 10);
    printf("Class1SpillMemBytes %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 11);
    printf("Class1SpillTmpCount %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 12);
    printf("Class1WordCount     %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 13);
    printf("AddrFifoWord1Count  %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 14);
    printf("Class2SpillMemBytes %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 15);
    printf("Class2SpillTmpCount %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 16);
    printf("Class2WordCount     %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 17);
    printf("AddrFifoWord2Count  %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 18);
    printf("Class3SpillMemBytes %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 19);
    printf("Class3SpillTmpCount %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 20);
    printf("Class3WordCount     %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 21);
    printf("AddrFifoWord3Count  %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);
    
    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 22);
    printf("FrOutSpillMemBytes  %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 23);
    printf("FrOutSpillTmpCounts %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 24);
    printf("TxDescWordCount     %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);
    
    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 25);
    printf("TmpWordCount        %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 26);
    printf("Tx_A_P              %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    phoenix_write_reg(mmio, XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL, 27);
    printf("FreeOutFifo         %d\n", mmio[XLR_REG_XGS_DBG_DESC_WORD_CNT]);

    printf("Tx+Rx 64 Byte Frames: %d\n", mmio[XLR_GMAC_TR64]);
    printf("Tx+Rx 65-127 Byte Frames: %d\n", mmio[XLR_GMAC_TR64_127]);
    printf("Tx+Rx 128-255 Byte Frames: %d\n", mmio[XLR_GMAC_TR128_255]);
    printf("Tx+Rx 256-511 Byte Frames: %d\n", mmio[XLR_GMAC_TR256_511]);
    printf("Tx+Rx 512-1023 Byte Frames: %d\n", mmio[XLR_GMAC_TR512_1023]);
    printf("Tx+Rx 1024-1518 Byte Frames: %d\n", mmio[XLR_GMAC_TR1024_1518]);
    printf("Tx+Rx 1519-1522 Byte Frames: %d\n", mmio[XLR_GMAC_TR1519_1522_VLAN]);

    printf("Receive bytes : %d\n", mmio[XLR_GMAC_RX_BYTES]);
    printf("Receive Packet Counter : %d\n", mmio[XLR_GMAC_RX_PKT]);
    printf("Receive FCS errored packet : %d\n", mmio[XLR_GMAC_RX_FCS_ERR]);
    printf("Receive Aligment Error counter: %d\n", mmio[XLR_GMAC_RX_ALGN_ERR]);
    printf("Receive Length Error counter: %d\n", mmio[XLR_GMAC_RX_LEN_ERR]);
    printf("Receive Undersize Packet counter: %d\n", mmio[XLR_GMAC_RX_UNDSZ]);
    printf("Receive Oversize Packet Counter: %d\n", mmio[XLR_GMAC_RX_OVERSZ]);
    printf("Receive Dropped Packets: %d\n", mmio[XLR_GMAC_RX_DRP]);
    printf("Receive Fragments frame counter: %d\n", mmio[XLR_GMAC_RX_FRG]);

    printf("Transmit bytes: %d\n", mmio[XLR_GMAC_TX_BYTES]);
    printf("Transmit Packet Counter: %d\n", mmio[XLR_GMAC_TX_PKT]);
    printf("Transmit Drop Frame Counter: %d\n", mmio[XLR_GMAC_TX_DRP]);
    printf("Transmit FCS errored packet : %d\n", mmio[XLR_GMAC_TX_FCS_ERR]);
    printf("Transmit Jabber Frame Counter: %d\n", mmio[XLR_GMAC_TX_JBR]);
    printf("Transmit Undersize Packet counter: %d\n", mmio[XLR_GMAC_TX_UNDSZ]);
    printf("Transmit Oversize Packet Counter: %d\n", mmio[XLR_GMAC_TX_OVERSZ]);
    printf("Transmit Fragments frame counter: %d\n", mmio[XLR_GMAC_TX_FRG]);

    printf("\n");

    return 1;
}


U_BOOT_CMD(
    gmacdumpreg, 3, 1, do_rmigmac,
    "gmacdumpreg - dump all known registers\n",
    "testing\n"
    );

