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


#ifndef _DXJ106_H_
#define _DXJ106_H_

void dxj106_init(void);
void dxj106_write_reg(uint32_t reg, uint32_t val);
void dxj106_read_reg (uint32_t reg);
void dxj106_port_stats(uint8_t port);

#define EOK     0
#define EFAIL  -1
#define EINVAL -2
#define TRUE    1
#define FALSE   0

#define DXJ106_MAX_PORTS                  10
#define DXJ106_UBOOT_PORT                  9
#define DXJ106_UBOOT_PORT_PHY_ADDR         7
#define DXJ106_CAVIUM_PORT                63
#define OCTEON_DXJ106_SMI_ADDR            0x0
#define DXJ106_DEVICE_ID                  0x0
#define DXJ106_MTU_DEFAULT_SIZE           1518
#define DXJ106_JUMBO_FRAME_SIZE           9018
#define DXJ106_PORT_SER_PARS_CNF_IPG_CAVIUM  0xC

#define DXJ106_SRX220_UBOOT_PORT           0
#define DXJ106_SRX220_UBOOT_PORT_PHY_ADDR  0

#define DXJ106_TIME_DELAY                 1000
#define DXJ106_CLBK_INTERVAL              (1 * hz)
#define DXJ106_GPIO_INIT_THRESH           10

#define DXJ106_GLOB_CTRL_REG              0x00000058
#define DXJ106_GLOB_CTRL_RESET            0x10000  
#define DXJ106_GLOB_CTRL_INIT_DONE        ((1<<17) | (1<<18))
#define DXJ106_GLOB_CTRL_RSVD14           0x4000   
#define DXJ106_GLOB_CTRL_RSVD19           0x180000
#define DXJ106_GLOB_CTRL_DEV_EN           0x1      
#define DXJ106_GLOB_CTRL_CPU_EN           0x2    
#define DXJ106_GLOB_CTRL_DEV_ID           (DXJ106_DEVICE_ID << 4)
#define DXJ106_GLOB_CTRL_PHY_ADDR         0x0000 
#define DXJ106_GLOB_CTRL_DEV_ID_MASK      0x000001f0
#define DXJ106_GLOB_CTRL_DEV_ID_SHFT      4


#define DXJ106_GLOB_CTRL_VAL                                 \
    (DXJ106_GLOB_CTRL_DEV_EN | DXJ106_GLOB_CTRL_CPU_EN   |   \
     DXJ106_GLOB_CTRL_DEV_ID | DXJ106_GLOB_CTRL_PHY_ADDR |   \
     DXJ106_GLOB_CTRL_RSVD14 | DXJ106_GLOB_CTRL_RSVD19)

#define DXJ106_INIT_WAIT_ATTEMPTS         20

#define DXJ106_PAD_CALIB_CNF_REG          0x00000108
#define DXJ106_PAD_CALIB_CNF_VAL          0x00000040

#define DXJ106_GLOB_PE_CNF_REG          0x0B800000
#define DXJ106_GLOB_PE_IPLEN_CHK_EN     0x8000
#define DXJ106_GLOB_PE_HASH_MODE_SRC    0x0200 
#define DXJ106_GLOB_PE_POLICY_ENG_DIS   0x0080
#define DXJ106_GLOB_PE_DEFAULT          0x0040
#define DXJ106_GLOB_PE_PCLID_MODE       0x0002
#define DXJ106_GLOB_PE_TCAM_WM          0x0001

#define DXJ106_GLOB_PE_CNF_VAL                              \
    (DXJ106_GLOB_PE_IPLEN_CHK_EN | DXJ106_GLOB_PE_TCAM_WM | \
     DXJ106_GLOB_PE_DEFAULT | DXJ106_GLOB_PE_HASH_MODE_SRC)

#define DXJ106_FDB_GLOB_CNF_REG               0x06000000
#define DXJ106_FDB_GLOB_CNF_MAC_VLAN_LKUP_EN  0x8

#define DXJ106_BRG_GLOB_CNF_REG0              0x02040000
#define DXJ106_BRG_GLOB_CNF_PVLAN_EN          (0x1 << 30)

#define DXJ106_PHY_ADDR_REG0                  0x04004030
#define DXJ106_PHY_ADDR_REG1                  0x04804030 

#define DXJ106_LOKI_PHY_ADDR0(port,addr) \
        (((u_int32_t)addr & 0x1f) << (port * 5))

#define DXJ106_LOKI_PHY_ADDR1(port,addr) \
        (((u_int32_t)addr & 0x1f) << ((port - 6) * 5))

#define DXJ106_SRX220_PHY_ADDR0(port,addr) \
        (((u_int32_t)addr & 0x1f) << (port * 5))

#define DXJ106_SRX220_PHY_ADDR1(port,addr) \
        (((u_int32_t)addr & 0x1f) << ((port - 6) * 5))

#define DXJ106_PORT_SERDES_CNF_REG3_BASE 0x0A800034
#define DXJ106_PORT_SERDES_CNF_REG3(port) \
    (DXJ106_PORT_SERDES_CNF_REG3_BASE + DXJ106_PORT_SERDES_OFFSET * port)

#define DXJ106_PORT_SERDES_CNF_VAL3      0x00008600

#define DXJ106_PORT_SERDES_CNF_REG0_BASE 0x0A800028
#define DXJ106_PORT_SERDES_CNF_REG0(port) \
    (DXJ106_PORT_SERDES_CNF_REG0_BASE + DXJ106_PORT_SERDES_OFFSET * port)

#define DXJ106_PORT_SERDES_CNF_VAL0      0x0000601E

#define DXJ106_PORT_MAC_CTRL_REG1_BASE   0x0A800004
#define DXJ106_PORT_MAC_CTRL_REG1(port) \
    (DXJ106_PORT_MAC_CTRL_REG1_BASE + DXJ106_PORT_MAC_OFFSET * port)

#define DXJ106_PORT_MAC_CTRL_VAL1_BASE   0x00000005 

#define DXJ106_PORT_MAC_SA_LSB         0x80
#define DXJ106_PORT_MAC_CTRL_VAL1(port) \
    (DXJ106_PORT_MAC_CTRL_VAL1_BASE + DXJ106_PORT_MAC_SA_LSB * port)

#define DXJ106_CPU_PORT_MAC_CTRL_REG2      0x0A803F08

#define DXJ106_INTR_MASK_REGISTER_BASE            0x0A800024
#define DXJ106_INTR_MASK_REGISTER(port) \
        (DXJ106_INTR_MASK_REGISTER_BASE + DXJ106_PORT_MAC_OFFSET * port)

#define DXJ106_INTR_MASK_ALL                      0x0

#define DXJ106_PORT_MAC_OFFSET                0x100
#define DXJ106_PORT_MAC_CTRL_REG0_BASE        0x0A800000
#define DXJ106_PORT_MAC_CTRL_REG0(port) \
    (DXJ106_PORT_MAC_CTRL_REG0_BASE + DXJ106_PORT_MAC_OFFSET * port)

#define DXJ106_PORT_MAC_CTRL_PORT_ENA         0x1
#define DXJ106_PORT_MAC_CTRL_PORT_X           0x2
#define DXJ106_PORT_MAC_CTRL_MIB_CNT_ENA      0x8000 
#define DXJ106_MAX_FRAME_SIZE_SHIFT           2 
#define DXJ106_MAX_FRAME_SIZE_MASK            0x7FFC

#define DXJ106_MAC_MIB_COUNTER_CONF_REG0 0x04004020 
#define DXJ106_MAC_MIB_COUNTER_CONF_REG1 0x04804020 
#define DXJ106_MAC_MIB_COUNTER_CONF_DONT_CLEAR_ON_READ (1 << 4)

#define DXJ106_PORT_MAC_CTRL_REG2_BASE   0x0A800008
#define DXJ106_PORT_MAC_CTRL_REG2(port) \
    (DXJ106_PORT_MAC_CTRL_REG2_BASE + DXJ106_PORT_MAC_OFFSET * port)

#define DXJ106_PORT_MAC_CTRL_REG2_AN_MD  0x1

#define DXJ106_PORT_AUTONEG_CNF_REG_BASE     0x0A80000C
#define DXJ106_PORT_AUTONEG_CNF_REG(port) \
    (DXJ106_PORT_AUTONEG_CNF_REG_BASE + DXJ106_PORT_MAC_OFFSET * port)

#define DXJ106_PORT_AUTONEG_CNF_RSVD15       (1 << 15)
#define DXJ106_PORT_AUTONEG_CNF_AN_FDPLX_EN  (1 << 13)
#define DXJ106_PORT_AUTONEG_CNF_FDPLX        (1 << 12)
#define DXJ106_PORT_AUTONEG_CNF_AN_FC_EN     (1 << 11)
#define DXJ106_PORT_AUTONEG_CNF_AN_ADV_PAUSE (1 << 9)
#define DXJ106_PORT_AUTONEG_CNF_FC           (1 << 8)
#define DXJ106_PORT_AUTONEG_CNF_AN_SPEED_EN  (1 << 7)
#define DXJ106_PORT_AUTONEG_CNF_GMII_SPEED   (1 << 6)
#define DXJ106_PORT_AUTONEG_CNF_MII_SPEED    (1 << 5)
#define DXJ106_PORT_AUTONEG_CNF_AN_RESTART   (1 << 4)
#define DXJ106_PORT_AUTONEG_CNF_AN_BYPASS    (1 << 3)
#define DXJ106_PORT_AUTONEG_CNF_AN_EN        (1 << 2)
#define DXJ106_PORT_AUTONEG_CNF_F_LNK_UP     (1 << 1)

#define DXJ106_IGRS_PRTBRG_CNF_OFFSET         0x1000
#define DXJ106_IGRS_PRTBRG_CNF_REG0_BASE      0x02000000
#define DXJ106_IGRS_PRTBRG_CNF_REG0(port) \
        (DXJ106_IGRS_PRTBRG_CNF_REG0_BASE + DXJ106_IGRS_PRTBRG_CNF_OFFSET * port)

#define DXJ106_IGRS_PRTBRG_CNF_REG0_PVLAN_TRK 0x01000000
#define DXJ106_IGRS_PRTBRG_CNF_REG0_PVLAN_EN  0x00800000 
#define DXJ106_IGRS_PRTBRG_CNF_REG0_DST_MASK  0x01FFFFFF 
#define DXJ106_IGRS_PRTBRG_CNF_REG0_DST_SHFT  25         
#define DXJ106_IGRS_PRTBRG_CNF_REG0_NMSG2CPU  0x1        
#define DXJ106_IGRS_PRTBRG_CNF_REG0_AL_DIS    0x00008000 
#define DXJ106_IGRS_PRTBRG_CNF_REG0_FWD_USRC  0xFFF8FFFF 

#define DXJ106_IGRS_PRTBRG_CNF_REG1_BASE      0x02000010
#define DXJ106_IGRS_PRTBRG_CNF_REG1(port) \
    (DXJ106_IGRS_PRTBRG_CNF_REG1_BASE + DXJ106_IGRS_PRTBRG_CNF_OFFSET * port)

#define DXJ106_IGRS_PRTBRG_CNF_REG1_TDEV_MASK 0xFFFFFFE0
 
#define DXJ106_VLT_TBL_ACS_CTRL_REG           0x0A00000C
#define DXJ106_VLT_TBL_ACS_CTRL_RW_TRIG       0x00008000

#define DXJ106_VLT_TBL_ACS_DATA_REG0          0x0A000008
#define DXJ106_VLT_TBL_ACS_VLAN_TAB_VALID     0x1
#define DXJ106_VLT_TBL_ACS_NO_SCRTY_BRCH      0x2
#define DXJ106_VLT_DATA0_PORT_MASK            0x00FFFFFF

#define DXJ106_MAKE_CAVIUM_VLANMBR_EGTAGD_DR1(w,port) \
        (w |= (3 << ((port - 4) * 2)))

#define DXJ106_MAKE_PORT_VLANMBR_EGTAGD_DR0(w,port) \
        (w |= (1 << (port * 2 + 24)))

#define DXJ106_VLT_TBL_ACS_DATA_REG1          0x0A000004

#define DXJ106_VLT_TBL_ACS_DATA_REG2          0x0A000000

#define DXJ106_VLT_TBL_ACS_CTRL_REG           0x0A00000C
#define DXJ106_VLT_TBL_ACS_CTRL_RW_TRIG       0x00008000 
#define DXJ106_VLT_TBL_ACS_CTRL_WR_ACT        0x00001000 

#define DXJ106_PORT_VLAN_QOS_CNF_REG0             0x0B800320
#define DXJ106_PORT_VLAN_QOS_CNF_NST_VLAN_EN      0x01000000
#define DXJ106_PORT_VLAN_QOS_CNF_INGR_VLAN_ETH1   0x00400000

#define DXJ106_PORT_VLAN_QOS_CNF_REG1             0x0B800324
#define DXJ106_PORT_VLAN_QOS_CNF_PVID_MD_ALL      0x00008000

#define DXJ106_PORT_VLAN_QOS_PROT_ACS_CTRL_REG    0x0B800328
#define DXJ106_PORT_VLAN_QOS_PROT_ACS_CTRL_TRIG   0x1
#define DXJ106_PORT_VLAN_QOS_PROT_ACS_CTRL_WR     0x2 
#define DXJ106_PORT_VLAN_QOS_PROT_ACS_CTRL_AT_PORT 0x00000400

#define DXJ106_PORT_SERDES_OFFSET        0x100
#define DXJ106_PORT_SERDES_CNF_REG3_BASE 0x0A800034
#define DXJ106_PORT_SERDES_CNF_REG3(port) \
    (DXJ106_PORT_SERDES_CNF_REG3_BASE + DXJ106_PORT_SERDES_OFFSET * port)

#define DXJ106_PORT_SERDES_CNF_VAL3      0x00008600

#define DXJ106_PORT_SERDES_CNF_REG0_BASE 0x0A800028
#define DXJ106_PORT_SERDES_CNF_REG0(port) \
    (DXJ106_PORT_SERDES_CNF_REG0_BASE + DXJ106_PORT_SERDES_OFFSET * port)

#define DXJ106_PORT_SERDES_CNF_REG0_RST  0x1
#define DXJ106_PORT_SERDES_CNF_VAL0      0x0000601E

#define DXJ106_EGRS_VLAN_ETHTYPE_SEL_REG    0x0780001C
#define DXJ106_PORT_SER_PARS_CNF_REG_BASE   0x0A800014
#define DXJ106_PORT_SER_PARS_CNF_REG(port) \
        (DXJ106_PORT_SER_PARS_CNF_REG_BASE + DXJ106_PORT_MAC_OFFSET * port)
#define DXJ106_PORT_SER_PARS_CNF_IPG_MASK   0xFFFFFFF0
#define DXJ106_PORT_SER_PARS_CNF_IPG_ICH5   0x0C

#define DXJ106_SMI_MGMT_REG_0                0x04004054
#define DXJ106_PHY_PAGE_ADDR_REG_OFFSET      22
#define DXJ106_PHY_ADDR_MASK                 0x0000001f
#define DXJ106_PHY_REG_OFFSET_MASK           0x0000001f
#define DXJ106_SMI_MGMT_REG_PHY_ADDR_SHIFT   16
#define DXJ106_SMI_MGMT_REG_PHY_SMI_WR_OP    0
#define DXJ106_SMI_MGMT_REG_PHY_SMI_RD_OP \
        (1 << DXJ106_SMI_MGMT_REG_PHY_SMI_OPCODE_SHIFT)
#define DXJ106_SMI_MGMT_REG_PHY_DATA_MASK    0x0000ffff
#define DXJ106_SMI_PHY_READY_ATTEMPTS        8
#define DXJ106_SMI_MGMT_REG_BUSY             (1 << 28)
#define DXJ106_SMI_MGMT_REG_READ_VALID       (1 << 27)
#define DXJ106_SMI_MGMT_REG_PHY_REG_OFFSET_SHIFT    21
#define DXJ106_SMI_MGMT_REG_PHY_SMI_OPCODE_SHIFT    26

#define DXJ106_PORT_STATUS_REG_BASE          0x0A800010
#define DXJ106_PORT_STATUS_REG(port) \
        (DXJ106_PORT_STATUS_REG_BASE + DXJ106_PORT_MAC_OFFSET * port)


#define DXJ106_SMI_STATUS_POLL_CNT          100
#define DXJ106_SMI_READ_READY_POLL_CNT      100
                                                                                            
#define DXJ106_SMI_PHY_READY_ATTEMPTS       8
                                                                                            
#define DXJ106_SMI_WR_ADDR_MSB_REG          0x00
#define DXJ106_SMI_WR_ADDR_LSB_REG          0x01
#define DXJ106_SMI_WR_DATA_MSB_REG          0x02
#define DXJ106_SMI_WR_DATA_LSB_REG          0x03
                                                                                            
#define DXJ106_SMI_RD_ADDR_MSB_REG          0x04
#define DXJ106_SMI_RD_ADDR_LSB_REG          0x05
#define DXJ106_SMI_RD_DATA_MSB_REG          0x06
#define DXJ106_SMI_RD_DATA_LSB_REG          0x07
                                                                                            
#define DXJ106_SMI_STATUS_REG               0x1f
                                                                                            
#define DXJ106_SMI_STATUS_WRITE_DONE        0x02
#define DXJ106_SMI_STATUS_READ_READY        0x01
                                                                                            
#define DXJ106_MAX_PORTS_PER_SMI_REG        12
                                                                                            
#define DXJ106_SMI_MGMT_REG_0               0x04004054
#define DXJ106_SMI_MGMT_REG_1               0x05004054
                                                                                            
#define DXJ106_SMI_MGMT_REG_BUSY           (1 << 28)
#define DXJ106_SMI_MGMT_REG_READ_VALID     (1 << 27)

#define DXJ106_SMI_MGMT_REG_PHY_ADDR_SHIFT          16
#define DXJ106_SMI_MGMT_REG_PHY_REG_OFFSET_SHIFT    21
#define DXJ106_SMI_MGMT_REG_PHY_SMI_OPCODE_SHIFT    26
                                                                                            
#define DXJ106_SMI_MGMT_REG_PHY_DATA_MASK           0x0000ffff
 
#define DXJ106_SMI_MGMT_REG_PHY_SMI_WR_OP    0
#define DXJ106_SMI_MGMT_REG_PHY_SMI_RD_OP   \
        (1 << DXJ106_SMI_MGMT_REG_PHY_SMI_OPCODE_SHIFT)
                                                                                            
#define DXJ106_PHY_PAGE_ADDR_REG_OFFSET   22
                                                                                            
/* phy address is 5 bits */
#define DXJ106_PHY_ADDR_MASK            0x0000001f
 
/* phy register offset is 5 bits */
#define DXJ106_PHY_REG_OFFSET_MASK      0x0000001f

#define DXJ106_EGRS_VLAN_ETHTYPE_SEL_REG    0x0780001C
#define DXJ106_PORT_SER_PARS_CNF_REG_BASE   0x0A800014
#define DXJ106_PORT_SER_PARS_CNF_REG(port) \
        (DXJ106_PORT_SER_PARS_CNF_REG_BASE + DXJ106_PORT_MAC_OFFSET * port)
#define DXJ106_PORT_SER_PARS_CNF_IPG_MASK   0xFFFFFFF0
#define DXJ106_PORT_SER_PARS_CNF_IPG_ICH5   0x0C

#define DXJ106_BITS_IN_HALF_WORD    16
#define DXJ106_LS_HALF_WORD_MASK    0xFFFF

#define DXJ106_IGRS_PRTBRG_CNF_REG0_ARPMC_TRAP_ENA  0x2000
#define DXJ106_IGRS_PRTBRG_CNF_REG0_ICMP_TRAP_ENA   0x4000
#define DXJ106_IGRS_PRTBRG_CNF_REG0_IGMP_TRAP_ENA   0x1000
#define DXJ106_IGRS_PRTBRG_CNF_REG0_USRC_MASK       0xFFF8FFFF
#define DXJ106_IGRS_PRTBRG_CNF_REG0_USRC_TRAP_2_CPU 0x00020000
                     
#define DXJ106_CASC_HDR_INS_CONF_REG                0x07800004
#define DXJ106_CASC_HDR_INS_CONF_CPUPORT_DASTAG_ENA 0x08000000

/* Stats related */
#define DXJ106_PORTS_PER_BANK            6
#define DXJ106_HIGH_BYTE                 4
#define DXJ106_HIGH_BYTE_SHIFT           32
#define DXJ106_PORT_COUNTER_SIZE         0x80
#define DXJ106_PORTS_0to5_BANK           0x04010000
#define DXJ106_PORTS_6to11_BANK          0x04810000
#define DXJ106_PORTS_12to17_BANK         0x05010000
#define DXJ106_TX_FIFO_ERR_OFFSET        0xC
#define DXJ106_RX_GOOD_PKTS_OFFSET       0x10
#define DXJ106_RX_BCAST_PKTS_OFFSET      0x18
#define DXJ106_RX_MCAST_PKTS_OFFSET      0x1C
#define DXJ106_TX_GOOD_OCTETS_OFFSET     0x38
#define DXJ106_TX_UCAST_PKTS_OFFSET      0x40
#define DXJ106_TX_MCAST_PKTS_OFFSET      0x48
#define DXJ106_TX_BCAST_PKTS_OFFSET      0x4C
#define DXJ106_RX_FIFO_OVERFLOW_OFFSET   0x5C
#define DXJ106_RX_RUNT_FRAMES_OFFSET     0x60
#define DXJ106_RX_OVRSZ_FRAMES_OFFSET    0x68
#define DXJ106_RX_ERR_PKTS_OFFSET        0x70
#define DXJ106_RX_BADCRC_PKTS_OFFSET     0x74

/* CPU Port stats */
#define DXJ106_MAC_ADDR_CNT0_REG              0x020400B0
#define DXJ106_MAC_ADDR_CNT1_REG              0x020400B4
#define DXJ106_MAC_ADDR_CNT2_REG              0x020400B8
#define DXJ106_CPU_INGRESS_PKTS_CNT_REG       0x020400BC
#define DXJ106_CPU_EGRESS_UCAST_PKTS_CNT_REG  0x020400C0
#define DXJ106_CPU_EGRESS_BCAST_PKTS_CNT_REG  0x020400D0
#define DXJ106_CPU_EGRESS_MCAST_PKTS_CNT_REG  0x020400CC

#define DXJ106_CPU_PORT_GOOD_PKTS_TX          0x00000060
#define DXJ106_CPU_PORT_ERR_PKTS_TX           0x00000064
#define DXJ106_CPU_PORT_GOOD_BYTES_TX         0x00000068
#define DXJ106_CPU_PORT_GOOD_PKTS_RX          0x00000070
#define DXJ106_CPU_PORT_BAD_PKTS_RX           0x00000074
#define DXJ106_CPU_PORT_GOOD_BYTES_RX         0x00000078
#define DXJ106_CPU_PORT_BAD_BYTES_RX          0x0000007C
#define DXJ106_CPU_PORT_INTERNAL_DROP_RX      0x0000006C


#define DXJ106_MAC_SA_MIDL_REG          0x04004024
#define DXJ106_MAC_SA_HIGH_REG          0x04004028
#define DXJ106_MAC_SA_LOW_REG           0x0A800004

#define DXJ106_CPU_PORT_CONF_REG        0x000000A0
#define DXJ106_CPU_PORT_MIB_CNT_MODE    0x8
#define DXJ106_CPU_PORT_IF_TYPE_MASK    0xFFFFFFF9
#define DXJ106_CPU_PORT_IF_TYPE_RGMII   0x4 /* RGMII */
#define DXJ106_CPU_PORT_ACTIVE          0x1



#define E1240_PAGE(page) page

#define E1240_MAC_CTRL_REG               21
#define E1240_MAC_CTRL_SGMII_AN_ENA      (1 << 12) 

#define E1240_CPR_CTRL_REG               0
#define E1240_CPR_CTRL_SPEED_MSB         (1 << 6) 
#define E1240_CPR_CTRL_CPR_MD_DUPLEX     (1 << 8)
#define E1240_CPR_CTRL_PWR_DOWN          (1 << 11)
#define E1240_CPR_CTRL_AN_ENA            (1 << 12)
#define E1240_CPR_CTRL_SPEED_LSB         (1 << 13) 
#define E1240_CPR_CTRL_RESET             (1 << 15)

#define E1240_CPR_SPFC_CTRL_REG1         16
#define E1240_CPR_SPFC_CTRL_PWR_DOWN     (1 << 2)

#define E1240_CPR_SPFC_STS_REG1          17
#define E1240_CPR_SPFC_STS_SPEED_MASK    0xC000 /* Bits - 15:14 */
#define E1240_CPR_SPFC_STS_SPEED_SHIFT   14
#define E1240_CPR_SPFC_STS_SPEED_GE      2
#define E1240_CPR_SPFC_STS_SPEED_FE      1
#define E1240_CPR_SPFC_STS_SPEED_ET      0
#define E1240_CPR_SPFC_STS_DUP           (1 << 13)

#define E1240_LED4TO5_REG                19
#define E1240_LED4TO5_CTRL_MASK          0xFF00
#define E1240_LED_CTRL_BLINK             0x4

#define E1240_LED0TO3_CTRL_REG           16
#define E1240_LED2_CTRL_LINK_MASK        0xF0FF

#define E1240_CPR_SP_CTRL_REG2           26
#define E1240_CPR_SP_CTRL_1000TX_TYPE    0x8000

#endif /*  __DXJ106__ */
