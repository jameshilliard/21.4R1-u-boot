/*
 * Copyright (c) 2007-2010, Juniper Networks, Inc.
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
#include <octeon_smi.h>
#include <dxj106.h>

/*
 * Store switch port phy address
 */
static uint16_t phy_addr[10];

static int dxj106_phy_smi_ready(uint32_t, int, uint32_t *);

static int
dxj106_smi_write_reg (uint32_t reg_addr, uint32_t value)
{
    int          ret = 0;
    uint16_t     ms_sw = 0;
    uint16_t     ls_sw = 0;
    uint32_t     cnt = 0;
    uint16_t     status = 0;

    /* send register address to dx over smi (half-word at a time) */
    ms_sw = (uint16_t)(reg_addr >> DXJ106_BITS_IN_HALF_WORD);
    ls_sw = (uint16_t)(reg_addr & DXJ106_LS_HALF_WORD_MASK);

    ret = octeon_smi_write(DXJ106_SMI_WR_ADDR_MSB_REG, ms_sw);
    if (ret != 0) {
        printf("failed to write DX smi write address reg(high)\n");
        goto err_out;
    }

    ret = octeon_smi_write(DXJ106_SMI_WR_ADDR_LSB_REG, ls_sw);
    if (ret != 0) {
        printf("failed to write DX smi write address reg(low)\n");
        goto err_out;
    }

    /* send register data to dx over smi (half-word at a time) */
    ms_sw = (uint16_t)(value >> DXJ106_BITS_IN_HALF_WORD);
    ls_sw = (uint16_t)(value & DXJ106_LS_HALF_WORD_MASK);

    ret = octeon_smi_write(DXJ106_SMI_WR_DATA_MSB_REG, ms_sw);
    if (ret != 0) {
        printf("failed setting DX smi write data reg (msb)\n");
        goto err_out;
    }

    ret = octeon_smi_write(DXJ106_SMI_WR_DATA_LSB_REG, ls_sw);
    if (ret != 0) {
        printf("failed setting DX smi write data reg (lsb)\n");
        goto err_out;
    }
    udelay(1000);
    /* wait till smi write completes */
    cnt = DXJ106_SMI_STATUS_POLL_CNT;
    while (cnt--) {

        ret = octeon_smi_read( DXJ106_SMI_STATUS_REG, &status);
        if (ret != 0) {
            printf("failed to read DX smi status reg\n");
            goto err_out;
        }

        if (status & DXJ106_SMI_STATUS_WRITE_DONE) {
            break;
        }

        if (!cnt) {
            printf("failed waiting for write complete even after %u attempts\n",
                   DXJ106_SMI_STATUS_POLL_CNT);
            ret = EFAIL;
            goto err_out;
        }
    }

err_out:
    return ret;
}

static int 
dxj106_smi_read_reg (uint32_t reg_addr, uint32_t *value)
{
    int          ret = 0;
    uint16_t     ms_sw = 0;
    uint16_t     ls_sw = 0;
    uint32_t     cnt = 0;
    uint16_t     status = 0;

    /* send register address to dx over smi (half-word at a time) */
    ms_sw = (uint16_t)(reg_addr >> DXJ106_BITS_IN_HALF_WORD);
    ls_sw = (uint16_t)(reg_addr & DXJ106_LS_HALF_WORD_MASK);

    ret = octeon_smi_write(DXJ106_SMI_RD_ADDR_MSB_REG, ms_sw);
    if (ret != 0) {
        printf("failed to write DX smi read address reg\n");
        goto err_out;
    }

    ret = octeon_smi_write(DXJ106_SMI_RD_ADDR_LSB_REG, ls_sw);
    if (ret != 0) {
        printf("failed to write DX smi read address reg\n");
        goto err_out;
    }

    /* wait till smi read is ready */
    cnt = DXJ106_SMI_STATUS_POLL_CNT;
    while (cnt--) {
        
        udelay(1000);

        ret = octeon_smi_read(DXJ106_SMI_STATUS_REG, &status);
        if (ret != 0) {
            printf("failed to read DX smi status reg\n");
            goto err_out;
        }

        if (status & DXJ106_SMI_STATUS_READ_READY) {
            break;
        }

        if (!cnt) {
            printf("failed waiting for read ready even after %u attempts\nstatus %x\n", 
                   DXJ106_SMI_STATUS_POLL_CNT,status);
            ret = EFAIL;
            goto err_out;
        }
    }

    /* receive register data from dx over smi (half-word at a time) */
    ret = octeon_smi_read(DXJ106_SMI_RD_DATA_MSB_REG, &ms_sw);
    if (ret != 0) {
        printf("failed to read DX smi read data reg\n");
        goto err_out;
    }

    ret = octeon_smi_read(DXJ106_SMI_RD_DATA_LSB_REG, &ls_sw);
    if (ret != 0) {
        printf("failed to read DX smi read data reg\n");
        goto err_out;
    }

    *value = (((uint32_t)ms_sw << DXJ106_BITS_IN_HALF_WORD) | 
              ((uint32_t)ls_sw & DXJ106_LS_HALF_WORD_MASK));

err_out:
    return ret;
}

static int
dxj106_set_phy_page (uint8_t port, uint8_t phy_adr, uint8_t page_num)
{
    int     ret = 0;
    uint32_t    smi_mgmt_reg = 0;
    uint32_t    smi_reg_val = 0;
        
    smi_mgmt_reg = ((port < DXJ106_MAX_PORTS_PER_SMI_REG) ?
                    DXJ106_SMI_MGMT_REG_0 : DXJ106_SMI_MGMT_REG_1);

    /* check if phy is ready for smi write */
    if (!dxj106_phy_smi_ready(smi_mgmt_reg, TRUE, &smi_reg_val)) {
        ret = EFAIL;
        printf("failed waiting for phy smi ready\n");
        goto err_out;
    }

    /* data (page number) to write to over smi */
    smi_reg_val = page_num;

    /* phy address to write to over smi */
    phy_adr &= DXJ106_PHY_ADDR_MASK;
    smi_reg_val |= (phy_adr << DXJ106_SMI_MGMT_REG_PHY_ADDR_SHIFT);

    /* phy register offset to write over smi */
    smi_reg_val |= (DXJ106_PHY_PAGE_ADDR_REG_OFFSET << 
                    DXJ106_SMI_MGMT_REG_PHY_REG_OFFSET_SHIFT);

    /* mark smi operation as write */
    smi_reg_val |= DXJ106_SMI_MGMT_REG_PHY_SMI_WR_OP;

    /* do the smi write now */
    ret = dxj106_smi_write_reg(smi_mgmt_reg, smi_reg_val);
    if (ret != 0) {
        goto err_out;
    }

err_out:
    return ret;
}



/*
 * Function to set MTU of the port
 */
static int
dxj106_port_mtu_set (int port, uint16_t  mtu)
{
    uint32_t  dword = 0;

    dxj106_smi_read_reg(DXJ106_PORT_MAC_CTRL_REG0(port), &dword);
    
    dword &= ~DXJ106_MAX_FRAME_SIZE_MASK;
    dword |= (((mtu >> 1) << DXJ106_MAX_FRAME_SIZE_SHIFT) & 
              DXJ106_MAX_FRAME_SIZE_MASK);

    dxj106_smi_write_reg(DXJ106_PORT_MAC_CTRL_REG0(port), dword);
    
    return EOK;
}

/*
 * Function to activate the port. Enables port.
 */
static int
dxj106_start_port (int port)
{
    uint32_t dword = 0;
    int      ret = 0;
    
    dxj106_smi_read_reg(DXJ106_PORT_MAC_CTRL_REG0(port), &dword);
    dword |= (DXJ106_PORT_MAC_CTRL_PORT_ENA | 
              DXJ106_PORT_MAC_CTRL_MIB_CNT_ENA);
    dxj106_smi_write_reg(DXJ106_PORT_MAC_CTRL_REG0(port), dword);

    return ret;
}


static int
dxj106_phy_smi_ready (uint32_t smi_mgmt_reg,
                      int write, uint32_t *smi_reg_val)
{
    uint8_t    cnt = 0;
    int        ret = FALSE;

    *smi_reg_val = 0;

    cnt = DXJ106_SMI_PHY_READY_ATTEMPTS;
    while (cnt--) {
        ret = dxj106_smi_read_reg(smi_mgmt_reg, smi_reg_val);
        if (ret != 0) {
            /* retry */
            continue;
        }

        /* 
         * busy bit reset for ready for write and read-valid bit
         * set for ready for read 
         */
        if ((write && !(*smi_reg_val & DXJ106_SMI_MGMT_REG_BUSY)) || 
            (!write && (*smi_reg_val & DXJ106_SMI_MGMT_REG_READ_VALID))) {
            ret = TRUE;
            break;
        }

        if (!cnt) {
            printf("failed to read smi status even after %u attempts\n", 
                   DXJ106_SMI_PHY_READY_ATTEMPTS);
            *smi_reg_val = 0;
            ret = FALSE;
        }
    }

    return ret;
}


static int
dxj106_read_phy_reg (uint8_t port,
                     uint8_t phy_adr, uint8_t page_num,
                     uint8_t reg_offset, uint16_t *value)
{
    int    ret = 0;
    uint32_t   smi_mgmt_reg = 0;
    uint32_t   smi_reg_val = 0;

    if (!value) {
        printf("invalid arguments error\n");
        ret = EINVAL;
        goto err_out;
    }

    /* smi register 0 or smi register 1 ? */
    smi_mgmt_reg = ((port < DXJ106_MAX_PORTS_PER_SMI_REG) ? 
                    DXJ106_SMI_MGMT_REG_0 : DXJ106_SMI_MGMT_REG_1);

    /* update the phy page number */
    ret = dxj106_set_phy_page(port, phy_adr, page_num);
    if (ret != 0) {
        printf("failed to set phy page for port %u\n", port);
        goto err_out;
    }

    /* check if phy is ready for smi send */
    if (!dxj106_phy_smi_ready(smi_mgmt_reg, TRUE, &smi_reg_val)) {
        ret = EFAIL;
        printf("failed waiting for phy smi ready\n");
        goto err_out;
    }

    /* send the register read command to phy over smi */

    /* phy address to read from, over smi */
    phy_adr &= DXJ106_PHY_ADDR_MASK;
    smi_reg_val = (phy_adr << DXJ106_SMI_MGMT_REG_PHY_ADDR_SHIFT);

    /* phy register offset to read from, over smi */
    reg_offset &= DXJ106_PHY_REG_OFFSET_MASK;
    smi_reg_val |= (reg_offset << DXJ106_SMI_MGMT_REG_PHY_REG_OFFSET_SHIFT);

    /* mark smi operation as read */
    smi_reg_val |= DXJ106_SMI_MGMT_REG_PHY_SMI_RD_OP;

    /* write to smi register of dx */
    ret = dxj106_smi_write_reg(smi_mgmt_reg, smi_reg_val);
    if (ret != 0) {
        goto err_out;
    }

    /* receive the data from phy over smi */
    if (!dxj106_phy_smi_ready(smi_mgmt_reg, FALSE, &smi_reg_val)) {
        ret = EFAIL;
        printf("failed waiting for phy smi read done\n");
        goto err_out;
    }

    /* data is available in smi register */
    *value = smi_reg_val & DXJ106_SMI_MGMT_REG_PHY_DATA_MASK;

err_out:
    return ret;
}


static int 
dxj106_write_phy_reg (uint8_t port,
                      uint8_t phy_adr, uint8_t page_num,
                      uint8_t reg_offset, uint16_t value)
{
    int  ret = 0;
    uint32_t smi_mgmt_reg = 0;
    uint32_t smi_reg_val = 0;
        
    /* smi register 0 or smi register 1 ? */
    smi_mgmt_reg = ((port < DXJ106_MAX_PORTS_PER_SMI_REG) ? 
                    DXJ106_SMI_MGMT_REG_0 : DXJ106_SMI_MGMT_REG_1);

    /* update the phy page number */
    ret = dxj106_set_phy_page(port, phy_adr, page_num);
    if (ret != 0) {
        printf("failed to set phy page for port %u\n", port);
        goto err_out;
    }

    /* check if phy is ready for smi send */
    if (!dxj106_phy_smi_ready(smi_mgmt_reg, TRUE, &smi_reg_val)) {
        ret = EFAIL;
        printf("failed waiting for phy smi ready\n");
        goto err_out;
    }

    /* send the register write command to phy over smi */

    /* phy address to read from, over smi */
    phy_adr &= DXJ106_PHY_ADDR_MASK;
    smi_reg_val = (phy_adr << DXJ106_SMI_MGMT_REG_PHY_ADDR_SHIFT);

    /* phy register offset to read from, over smi */
    reg_offset &= DXJ106_PHY_REG_OFFSET_MASK;
    smi_reg_val |= (reg_offset << DXJ106_SMI_MGMT_REG_PHY_REG_OFFSET_SHIFT);

    /* mark smi operation as write */
    smi_reg_val |= DXJ106_SMI_MGMT_REG_PHY_SMI_WR_OP;

    /* data to write to, over smi */
    smi_reg_val |= value;

    /* write to smi register of dx */
    ret = dxj106_smi_write_reg(smi_mgmt_reg, smi_reg_val);
    if (ret != 0) {
        goto err_out;
    }

err_out:
    return ret;
}



static int
dxj106_init_port (int port)
{
    int      ret = 0;
    uint32_t dword = 0;
    uint32_t device_id = 0;
    uint8_t  uboot_mac[6];

    DECLARE_GLOBAL_DATA_PTR;
    
    /* mask all interrupts */
    dxj106_smi_write_reg(DXJ106_INTR_MASK_REGISTER(port), 
                         DXJ106_INTR_MASK_ALL);
   
    /* port specific settings */
    if (port == DXJ106_CAVIUM_PORT) {

        /* Configure CPU port */
        dxj106_smi_read_reg(DXJ106_CPU_PORT_CONF_REG, &dword);
        dword &= ~DXJ106_CPU_PORT_MIB_CNT_MODE;
        dword &= DXJ106_CPU_PORT_IF_TYPE_MASK;
        dword |= DXJ106_CPU_PORT_IF_TYPE_RGMII;
        dword |= DXJ106_CPU_PORT_ACTIVE;
        dxj106_smi_write_reg(DXJ106_CPU_PORT_CONF_REG, dword);

        dxj106_smi_read_reg(DXJ106_PORT_SER_PARS_CNF_REG(port), &dword);
        dword &= DXJ106_PORT_SER_PARS_CNF_IPG_MASK;
        dword |= DXJ106_PORT_SER_PARS_CNF_IPG_CAVIUM;
        dxj106_smi_write_reg(DXJ106_PORT_SER_PARS_CNF_REG(port), dword);

        /* Set mode to RGMII is MAC CTRL REG 2 */
        dxj106_smi_write_reg(DXJ106_CPU_PORT_MAC_CTRL_REG2, 0x10);

         /* Enable CPU port as cascading port */
        dxj106_smi_read_reg(DXJ106_CASC_HDR_INS_CONF_REG, &dword);
        dword |= DXJ106_CASC_HDR_INS_CONF_CPUPORT_DASTAG_ENA;
        dxj106_smi_write_reg(DXJ106_CASC_HDR_INS_CONF_REG, dword);
        
        /* Set 1000mbps, full-duplex and flow-control ON, link UP */
        dword  = ( DXJ106_PORT_AUTONEG_CNF_RSVD15     |
                   DXJ106_PORT_AUTONEG_CNF_FDPLX      |
                   DXJ106_PORT_AUTONEG_CNF_GMII_SPEED |
                   DXJ106_PORT_AUTONEG_CNF_FC         |
                   DXJ106_PORT_AUTONEG_CNF_F_LNK_UP);
        
        dxj106_smi_write_reg(DXJ106_PORT_AUTONEG_CNF_REG(port), dword);

        /*
         * USe MAC address of uboot port for
         * capturing CPU bound packet stats.
         */

        memcpy((void *)uboot_mac, &(gd->mac_desc.mac_addr_base), 6);
        /* Write MAC[0:3] of CPU port as DA MAC for ingress pkts */
        dword = ((uint32_t)(*(uint32_t *)&uboot_mac[0]));
        dxj106_smi_write_reg(DXJ106_MAC_ADDR_CNT0_REG, dword);

        /* 
         * Write MAC[4:5] of CPU port as DA MAC for ingress pkts
         * Write MAC[0:1] of CPU port as SA MAC for egress pkts
         */
        dword =  (*(uint16_t *)&uboot_mac[4]);
        dword |= ((*(uint16_t *)&uboot_mac[0]) << 16);
        dxj106_smi_write_reg(DXJ106_MAC_ADDR_CNT1_REG, dword); 

        /* Write MAC[2:5] of CPU port as SA MAC for egress pkts */
        dword = ((uint32_t)(*(uint32_t *)&uboot_mac[2]));
        dxj106_smi_write_reg(DXJ106_MAC_ADDR_CNT2_REG, dword);

    } else { /* Front end port */

        /* set SGMII mode */
        dxj106_smi_read_reg(DXJ106_PORT_MAC_CTRL_REG0(port), &dword);
        dword &= ~DXJ106_PORT_MAC_CTRL_PORT_X;
        dxj106_smi_write_reg(DXJ106_PORT_MAC_CTRL_REG0(port), dword);

        /* Settings common to both front end and back end ports */
        dxj106_smi_write_reg(DXJ106_PORT_SERDES_CNF_REG3(port), 
                             DXJ106_PORT_SERDES_CNF_VAL3);

        /* set outamp */
        dxj106_smi_write_reg(DXJ106_PORT_SERDES_CNF_REG0(port), 
                             DXJ106_PORT_SERDES_CNF_VAL0);
        /* set periodic flow control to false */
        dxj106_smi_write_reg(DXJ106_PORT_MAC_CTRL_REG1(port), 
                             DXJ106_PORT_MAC_CTRL_VAL1(port));

        /* Set SGMII In-Band Auto-neg mode */
        dxj106_smi_read_reg(DXJ106_PORT_MAC_CTRL_REG2(port), &dword);
        dword |= DXJ106_PORT_MAC_CTRL_REG2_AN_MD;
        dxj106_smi_write_reg( DXJ106_PORT_MAC_CTRL_REG2(port), dword);
        

        /* enable auto-neg on duplex, speed and flow control */
        dxj106_smi_read_reg(DXJ106_PORT_AUTONEG_CNF_REG(port), &dword);
        
        /* Set autoneg on speed, duplicity and flow-control */
        dword |= ( DXJ106_PORT_AUTONEG_CNF_RSVD15      |
                   DXJ106_PORT_AUTONEG_CNF_AN_FDPLX_EN |
                   DXJ106_PORT_AUTONEG_CNF_AN_SPEED_EN |
                   DXJ106_PORT_AUTONEG_CNF_AN_FC_EN    |
                   DXJ106_PORT_AUTONEG_CNF_AN_RESTART  |
                   DXJ106_PORT_AUTONEG_CNF_AN_EN);
        /* do not bypass Auto-neg */
        dword &= (~DXJ106_PORT_AUTONEG_CNF_AN_BYPASS);
        
        dxj106_smi_write_reg(DXJ106_PORT_AUTONEG_CNF_REG(port), dword);

        

         /* Disable "auto-learning" and enable "trap-to-CPU" */
        dxj106_smi_read_reg(DXJ106_GLOB_CTRL_REG, &dword);
        device_id = ((dword & DXJ106_GLOB_CTRL_DEV_ID_MASK) >> 
                     DXJ106_GLOB_CTRL_DEV_ID_SHFT);

        dxj106_smi_read_reg(DXJ106_IGRS_PRTBRG_CNF_REG0(port), &dword);
        dword &= ~DXJ106_IGRS_PRTBRG_CNF_REG0_NMSG2CPU;
        dword |= (DXJ106_IGRS_PRTBRG_CNF_REG0_ARPMC_TRAP_ENA |
                  DXJ106_IGRS_PRTBRG_CNF_REG0_ICMP_TRAP_ENA  |
                  DXJ106_IGRS_PRTBRG_CNF_REG0_IGMP_TRAP_ENA);
        dword |= DXJ106_IGRS_PRTBRG_CNF_REG0_AL_DIS; 
        dword &= DXJ106_IGRS_PRTBRG_CNF_REG0_USRC_MASK;
        dword |= DXJ106_IGRS_PRTBRG_CNF_REG0_USRC_TRAP_2_CPU;
        dxj106_smi_write_reg(DXJ106_IGRS_PRTBRG_CNF_REG0(port), dword);
        
        
        /* Update register1 fields */
        dxj106_smi_read_reg(DXJ106_IGRS_PRTBRG_CNF_REG1(port), &dword);
        dword &= DXJ106_IGRS_PRTBRG_CNF_REG1_TDEV_MASK;
        dword |= device_id; /* set device id */
        dxj106_smi_write_reg(DXJ106_IGRS_PRTBRG_CNF_REG1(port), dword);

        /* 
         * Assign temp MAC address for ubooting from this port
         */
        dxj106_smi_write_reg(DXJ106_MAC_SA_HIGH_REG,
                            (uint32_t)*((uint32_t *)&uboot_mac[2]));
        

        dxj106_smi_read_reg(DXJ106_MAC_SA_MIDL_REG, &dword);
        dword &= 0xFFFFFF00;
        dword |= uboot_mac[1];
        dxj106_smi_write_reg(DXJ106_MAC_SA_MIDL_REG, dword);
        
        
        dxj106_smi_read_reg(DXJ106_MAC_SA_LOW_REG, &dword);
        dword &= 0x807F;
        dword |= (uboot_mac[0] << 7);
        dxj106_smi_write_reg(DXJ106_MAC_SA_LOW_REG, dword);
    }

    /* configure default mtu */
    ret = dxj106_port_mtu_set(port, DXJ106_MTU_DEFAULT_SIZE);
    if (ret) {
        return ret;
    }
    

    /* Start the port explicitly */
    ret = dxj106_start_port(port);
    

    return ret;
}


static int
dxj106_set_phy (unsigned int port)
{
    uint16_t sword = 0;

    /* Set E1240 SGMII auto-neg enable */
    dxj106_read_phy_reg(port, phy_addr[port], 
                        E1240_PAGE(2), E1240_MAC_CTRL_REG, &sword);
    dxj106_write_phy_reg(port, phy_addr[port],
                         E1240_PAGE(2), E1240_MAC_CTRL_REG, 
                         (sword | E1240_MAC_CTRL_SGMII_AN_ENA));

    /* Power down first */
    dxj106_read_phy_reg(port, phy_addr[port], 
                        E1240_PAGE(0), E1240_CPR_SPFC_CTRL_REG1, &sword);
    dxj106_write_phy_reg(port, phy_addr[port],
                         E1240_PAGE(0), E1240_CPR_SPFC_CTRL_REG1, 
                         (sword | E1240_CPR_SPFC_CTRL_PWR_DOWN));

    udelay(DXJ106_TIME_DELAY);
                     
    /* Power up from register 16 */
    dxj106_read_phy_reg(port, phy_addr[port], 
                        E1240_PAGE(0), E1240_CPR_SPFC_CTRL_REG1, &sword);
    dxj106_write_phy_reg(port, phy_addr[port],
                         E1240_PAGE(0),E1240_CPR_SPFC_CTRL_REG1,
                         (sword & ~E1240_CPR_SPFC_CTRL_PWR_DOWN));

    /* Set copper control registers */
    dxj106_read_phy_reg(port, phy_addr[port], 
                        E1240_PAGE(0), E1240_CPR_CTRL_REG, &sword);
    sword &= ~E1240_CPR_CTRL_PWR_DOWN;   /* power up */
    sword |=  E1240_CPR_CTRL_RESET;      /* soft reset */
    sword |=  E1240_CPR_CTRL_AN_ENA;     /* autonegotiate */
    dxj106_write_phy_reg(port, phy_addr[port], 
                         E1240_PAGE(0), E1240_CPR_CTRL_REG, sword);

    udelay(DXJ106_TIME_DELAY);
    
    dxj106_read_phy_reg(port, phy_addr[port], 
                        E1240_PAGE(3), E1240_LED4TO5_REG, &sword);
    sword &= E1240_LED4TO5_CTRL_MASK;
    sword |= E1240_LED_CTRL_BLINK;
    dxj106_write_phy_reg(port, phy_addr[port],
                         E1240_PAGE(3),E1240_LED4TO5_REG, sword);

    dxj106_read_phy_reg(port, phy_addr[port], 
                        E1240_PAGE(3), E1240_LED0TO3_CTRL_REG, &sword);
    sword &= E1240_LED2_CTRL_LINK_MASK;
    dxj106_write_phy_reg(port, phy_addr[port],
                         E1240_PAGE(3), E1240_LED0TO3_CTRL_REG, sword);
    
    /* Set 1000 Base-T transmitter type to class A */
    dxj106_read_phy_reg(port, phy_addr[port], 
                        E1240_PAGE(0), E1240_CPR_SP_CTRL_REG2, &sword);
    sword |= E1240_CPR_SP_CTRL_1000TX_TYPE;
    dxj106_write_phy_reg(port, phy_addr[port],
                         E1240_PAGE(0), E1240_CPR_SP_CTRL_REG2, sword);

    return EOK;
}

void 
dxj106_init (void)
{
    int      err = 0, cnt = 0, i = 0;
    uint32_t dword = 0, data0 = 0;
    uint32_t dxj106_uboot_port = 0;

    DECLARE_GLOBAL_DATA_PTR;

    octeon_smi_set_clock();
    octeon_smi_init();

    /* Reset Device */
    dxj106_smi_write_reg(DXJ106_GLOB_CTRL_REG, 
                         (dword | DXJ106_GLOB_CTRL_RESET | 
                          (OCTEON_DXJ106_SMI_ADDR  << 9)));

    for (cnt = DXJ106_INIT_WAIT_ATTEMPTS; cnt > 0; cnt--) {
        udelay(DXJ106_TIME_DELAY * 5);
        dxj106_smi_read_reg(DXJ106_GLOB_CTRL_REG, &dword);
        if ((dword & DXJ106_GLOB_CTRL_INIT_DONE) == DXJ106_GLOB_CTRL_INIT_DONE) {
            break;
        }
    }

    if (!cnt) {
#ifdef DEBUG
        printf("Error: Timed out in DX106 init\n");
#endif
    }

    /* PCI Calibration */
    dxj106_smi_write_reg(DXJ106_PAD_CALIB_CNF_REG, DXJ106_PAD_CALIB_CNF_VAL);


    dxj106_smi_write_reg(DXJ106_GLOB_PE_CNF_REG, DXJ106_GLOB_PE_CNF_VAL);

    /* enable device */
    dxj106_smi_write_reg(DXJ106_GLOB_CTRL_REG, DXJ106_GLOB_CTRL_VAL);

    /*
     * Uboot port numbering and PHY address is different for SRX210 and
     * SRX220.
     */
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX210_MODELS
        dxj106_uboot_port = DXJ106_UBOOT_PORT;

        /* To be on the safer side, disable other user ports */
        for (i = 0; i < 10 ; i++) {
            if ( i != dxj106_uboot_port) {
                dxj106_smi_read_reg(DXJ106_PORT_MAC_CTRL_REG0(i), &dword);
                dword &= ~DXJ106_PORT_MAC_CTRL_PORT_ENA;
                dxj106_smi_write_reg(DXJ106_PORT_MAC_CTRL_REG0(i), dword);
            }
        }
        
        /* Assign PHY address */
        phy_addr[dxj106_uboot_port] = DXJ106_UBOOT_PORT_PHY_ADDR;

        data0 = DXJ106_LOKI_PHY_ADDR1(dxj106_uboot_port,
                                      DXJ106_UBOOT_PORT_PHY_ADDR);

        dxj106_smi_write_reg(DXJ106_PHY_ADDR_REG1, data0);
        break;

    CASE_ALL_SRX220_MODELS
        dxj106_uboot_port = DXJ106_SRX220_UBOOT_PORT;

        /* To be on the safer side, disable other user ports */
        for (i = 0; i < 10 ; i++) {
            if ( i != dxj106_uboot_port) {
                dxj106_smi_read_reg(DXJ106_PORT_MAC_CTRL_REG0(i), &dword);
                dword &= ~DXJ106_PORT_MAC_CTRL_PORT_ENA;
                dxj106_smi_write_reg(DXJ106_PORT_MAC_CTRL_REG0(i), dword);
            }
        }
        
        /* Assign PHY address */
        phy_addr[dxj106_uboot_port] = DXJ106_SRX220_UBOOT_PORT_PHY_ADDR;
        data0 = DXJ106_SRX220_PHY_ADDR0(DXJ106_SRX220_UBOOT_PORT,
                                        DXJ106_SRX220_UBOOT_PORT_PHY_ADDR);

        dxj106_smi_write_reg(DXJ106_PHY_ADDR_REG0, data0);
        break;
    default:
        break;
    }

    dxj106_set_phy(dxj106_uboot_port);

    err = dxj106_init_port(DXJ106_CAVIUM_PORT);
    if (err) {
        printf("dxj106 module - failed to initialize CAVIUM port\n");
        return;
    }

    err = dxj106_init_port(dxj106_uboot_port);
    if (err) {
        printf("dxj106 module - failed to initialize uboot port\n");
        return;
    }

    dxj106_smi_read_reg(DXJ106_GLOB_CTRL_REG, &dword);
#ifdef DEBUG
    printf("Init End - Global reg:0x%x08 - %d\n", dword,__LINE__);
#endif

    return;
}

void
dxj106_read_reg (uint32_t reg)
{
    uint32_t val = 0;

    dxj106_smi_read_reg(reg, &val);
    printf("Register 0x%08x = 0x%x\n", reg, val);
}

void
dxj106_write_reg (uint32_t reg, uint32_t val)
{
    uint32_t tmp = 0;

    dxj106_smi_write_reg(reg,val);
    dxj106_smi_read_reg(reg, &tmp);
    printf("Register 0x%08x - Wrote 0x%08x : Read back 0x%08x\n",
        reg, val, tmp);
}

void
dxj106_port_stats (uint8_t port)
{
    uint32_t  reg_addr = 0;
    uint32_t  hi_32 = 0;
    uint32_t  lo_32 = 0;
    uint32_t  val = 0;
    uint32_t  cnt = 0;
    uint32_t  DXJ106_MAC_COUNTERS_BASE[] = {DXJ106_PORTS_0to5_BANK,
                                            DXJ106_PORTS_6to11_BANK,
                                            DXJ106_PORTS_12to17_BANK};
    if (port == 63) {
        /* Stats for CPU port */
        printf("\n=====Bridge statistics for CPU port ========\n");
        dxj106_smi_read_reg(DXJ106_CPU_INGRESS_PKTS_CNT_REG, &cnt);
        printf("Pkts received on CPU port: %d\n", cnt);

        dxj106_smi_read_reg(DXJ106_CPU_EGRESS_UCAST_PKTS_CNT_REG, &cnt);
        printf("Unicast Pkts transmitted on  CPU port: %d\n", cnt);

        dxj106_smi_read_reg(DXJ106_CPU_EGRESS_BCAST_PKTS_CNT_REG, &cnt);
        printf("Broadcast Pkts transmitted on  CPU port: %d\n", cnt);

        dxj106_smi_read_reg(DXJ106_CPU_EGRESS_MCAST_PKTS_CNT_REG, &cnt);
        printf("Multicast Pkts transmitted on  CPU port: %d\n", cnt);

        
        printf("\n=====MIB counters for CPU port ========\n");
        dxj106_smi_read_reg(DXJ106_CPU_PORT_GOOD_PKTS_TX, &cnt);
        printf("Good packets transmitted: : %d\n", cnt);

        dxj106_smi_read_reg(DXJ106_CPU_PORT_ERR_PKTS_TX, &cnt);
        printf("Errored packets TX: %d\n", cnt);

        dxj106_smi_read_reg(DXJ106_CPU_PORT_GOOD_BYTES_TX, &cnt);
        printf("Good octets transmitted: %d\n", cnt);

        dxj106_smi_read_reg(DXJ106_CPU_PORT_GOOD_PKTS_RX, &cnt);
        printf("Good packets received: %d\n", cnt);

        dxj106_smi_read_reg(DXJ106_CPU_PORT_BAD_PKTS_RX, &cnt);
        printf("Bad packets received: %d\n", cnt);

        dxj106_smi_read_reg(DXJ106_CPU_PORT_GOOD_BYTES_RX, &cnt);
        printf("Good octets received: %d\n", cnt);

        dxj106_smi_read_reg(DXJ106_CPU_PORT_BAD_BYTES_RX, &cnt);
        printf("Bad octets received: %d\n", cnt);

        dxj106_smi_read_reg(DXJ106_CPU_PORT_INTERNAL_DROP_RX, &cnt);
        printf("Packets dropped due to intrenal error RX: %d\n", cnt);
        
        return;
    }

    /* Stats for front-end ports */

    reg_addr = DXJ106_MAC_COUNTERS_BASE[port/DXJ106_PORTS_PER_BANK] + 
        DXJ106_PORT_COUNTER_SIZE * (port % DXJ106_PORTS_PER_BANK);

    printf("\n=====Statistics for port %d========\n",port);

    /* Get the 64 bit Rx Good Octects */
    dxj106_smi_read_reg(reg_addr, &lo_32);
    dxj106_smi_read_reg(reg_addr + DXJ106_HIGH_BYTE, &hi_32);
    printf("RX good octets: %d%d\n",hi_32,lo_32);

    dxj106_smi_read_reg(reg_addr + DXJ106_RX_GOOD_PKTS_OFFSET, &val);
    printf("RX unicast packets: %d\n", val); 

    dxj106_smi_read_reg(reg_addr + DXJ106_RX_BCAST_PKTS_OFFSET, &val);
    printf("RX broadcast packets: %d\n", val); 

    dxj106_smi_read_reg(reg_addr + DXJ106_RX_MCAST_PKTS_OFFSET, &val);
    printf("RX multicast packets: %d\n", val); 

    dxj106_smi_read_reg(reg_addr + DXJ106_RX_FIFO_OVERFLOW_OFFSET, &val);
    printf(" RX fifo overflow: %d\n", val);

    dxj106_smi_read_reg(reg_addr + DXJ106_RX_RUNT_FRAMES_OFFSET, &val);
    printf("RX undersize frames: %d\n", val);

    dxj106_smi_read_reg(reg_addr + DXJ106_RX_OVRSZ_FRAMES_OFFSET, &val);
    printf("RX oversize frames: %d\n", val);

    dxj106_smi_read_reg(reg_addr + DXJ106_RX_BADCRC_PKTS_OFFSET, &val);
    printf("RX CRC errors: %d\n", val);

    dxj106_smi_read_reg(reg_addr + DXJ106_TX_GOOD_OCTETS_OFFSET,&lo_32);
    dxj106_smi_read_reg(reg_addr + DXJ106_TX_GOOD_OCTETS_OFFSET + DXJ106_HIGH_BYTE,
                  &hi_32);
    printf("TX good octets: %d%d\n", hi_32, lo_32);

    dxj106_smi_read_reg(reg_addr + DXJ106_TX_UCAST_PKTS_OFFSET, &val);
    printf("TX unicast packets: %d\n", val);

    dxj106_smi_read_reg(reg_addr + DXJ106_TX_MCAST_PKTS_OFFSET, &val);
    printf("TX multicast packets: %d\n", val);

    dxj106_smi_read_reg(reg_addr + DXJ106_TX_BCAST_PKTS_OFFSET, &val);
    printf("TX broadcast packets: %d\n", val);

    dxj106_smi_read_reg(reg_addr + DXJ106_TX_FIFO_ERR_OFFSET, &val);
    printf("TX fifo underrun / CRC erros: %d\n", val);
}
