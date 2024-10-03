/*
 * $Id: ls609x_uboot_drv.h,v 1.4.64.1 2009-08-27 06:23:52 dtang Exp $
 *
 * ls609x_uboot_drv.h - defines for basic driver
 *
 * Richa Singla, January-2009
 *
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>
 */

#ifndef __LS609X_UBOOT_DRV_H__
#define __LS609X_UBOOT_DRV_H__

#include <common.h>
#include <octeon_smi.h>
#include <octeon_boot.h>

#define TRUE    (1)
#define FALSE   (0)

#define LS609X_UBOOT_PORT                         0
#define LS609X_FRONTEND_PORT1                     1
#define LS609X_FRONTEND_PORT7                     7
#define LS609X_UPLINK_PORT                        10

/* Global Registers */
#define LS609X_SW_GLOBAL_CONTROL                  0x04

/* Bit Definition for LS609X_SW_GLOBAL_CONTROL */
#define LS609X_SET_MAX_FRAME_1522(sword)          (sword &= ~BIT_10S) 
#define LS609X_DISABLE_INTERRUPTS(sword)          (sword &= ~BIT_7S) 

/********* PORT REG *******************/
#define LS609X_PORT_CONTROL                       0x04
#define LS609X_PCS_CONTROL                        0x01
#define LS609X_PORT_VLAN_MAP                      0x06
#define LS609X_PORT_CONTROL2                      0x08
#define LS609X_SW_GLOBAL_STATS_OPERATION          0x1D
#define LS609X_SW_GLOBAL_STATS_COUNTER3_2         0x1E
#define LS609X_SW_GLOBAL_STATS_COUNTER1_0         0x1F


/* Bit Definition for LS609X_PORT_CONTROL */
#define LS609X_PORT_EGRESS_UNMODIFIED(sword)      (sword &= ~(BIT_13S | BIT_12S)) 
#define LS609X_PORT_FRAME_NORMAL(sword)           (sword &= ~(BIT_9S | BIT_8S)) 
#define LS609X_PORT_DISCARD_UNKNOWN_DA(sword)     (sword &= ~(BIT_3S | BIT_2S))
#define LS609X_PORT_STATE_FORWARDING              (BIT_1S | BIT_0S)
#define LS609X_PORT_STATE_DISABLE(sword)          (sword &= ~(BIT_1S | BIT_0S))

/* Bit Definition for LS609X_PCS_CONTROL */
/* 10 bit made 0*/
#define LS609X_PCS_DISABLE_INBAND_AUTONEG(sword)  (sword &= 0xFBFF) 
#define LS609X_PCS_LINK_UP                        BIT_5S
#define LS609X_PCS_FORCE_LINK                     BIT_4S
#define LS609X_PCS_FULL_DUPLEX                    BIT_3S
#define LS609X_PCS_FORCE_DUPLEX                   BIT_2S
#define LS609X_PCS_FORCE_SPEED_1000(sword)        do {                     \
                                                        sword |= BIT_1S;   \
                                                        sword &= ~(BIT_0S);\
                                                  } while (0) 

/* Bit Definition for LS609X_PORT_VLAN_MAP */
#define LS609X_PORT_LEARN_DISABLE                 BIT_11S
#define LS609X_FWD_ALL_PORT(sword)                do {                     \
                                                       sword &=  0xF800;   \
                                                       sword |=  0x3ff;    \
                                                  }while (0)
#define LS609X_FWD_UBOOT_PORT(sword)              do {                     \
                                                       sword &=  0xF800;   \
                                                       sword |=  BIT_0S;   \
                                                  }while (0)
#define LS609X_FWD_CPU_PORT(sword)                do {                     \
                                                       sword &=  0xF800;   \
                                                       sword |=  BIT_10S;  \
                                                  } while (0)

/* Bit Definition for LS609X_PORT_CONTROL2 */
#define LS609X_DISABLE_802_1Q(sword)              (sword &= ~(BIT_11S | BIT_10S))

/********* PHY REG *******************/
#define LS609X_PHY_CONTROL                        0
#define LS609X_PHY_AUTONEGO_AD                    4
#define LS609X_PHY_PLED_SELECT                    22
#define LS609X_PHY_LED_CONTROL                    24  /*0x18*/
#define LS609X_PHY_LED_MANUAL_OVERRIDE            25  /*0x19*/

/* Bit Definition for LS609X_PHY_CONTROL     */
#define LS609X_PHY_CONTROL_SW_RESET               BIT_15S
#define LS609X_PHY_CONTROL_SPEED_LSB              BIT_13S
#define LS609X_PHY_CONTROL_AUTONEGO               BIT_12S
#define LS609X_PHY_POWER_DOWN_NORMAL(sword)       (sword &= ~BIT_11S)
#define LS609X_PHY_CONTROL_DUPLEX                 BIT_8S
#define LS609X_PHY_CONTROL_SPEED_MSB(sword)       (sword &= ~BIT_6S)

/* Bit Definition for LS609X_PHY_AUTONEGO_AD     */
#define LS609X_PHY_AUTONEGO_AD_100_FULL           BIT_8S
#define LS609X_PHY_AUTONEGO_AD_100_HALF           BIT_7S
#define LS609X_PHY_AUTONEGO_AD_10_FULL            BIT_6S
#define LS609X_PHY_AUTONEGO_AD_10_HALF            BIT_5S

/* Bit Definition for LS609X_PLED_SELECT     */

#define LS609X_PHY_LED_LINK_ACT                   10 /* 1010 */ 
#define LS609X_PHY_LED_INACTIVE                   15 /* 1111 */ 

/* Bit Definition for LS609X_PHY_LED_MANUAL_OVERRIDE */

#define LS609X_PHY_MANOVRDRV_LED_OFF              0x2
#define LS609X_PHY_MANOVRDRV_LED_NORMAL           0x0

/* Bit 7-4*/
#define LS609X_PHY_PRG_LED1(sword,mode)           sword &= 0xFF0F;   \
                                                  sword |= mode << 4;
/* Bit 11-8*/
#define LS609X_PHY_PRG_LED2(sword,mode)           sword &= 0xF0FF;   \
                                                  sword |= mode << 8;

#define LS609X_PHY_PRG_LED0_MAN_OVRDRV(sword, status)   \
                                                  sword &= 0xFFFC; \
                                                  sword |= status;
#define LS609X_PHY_PRG_LED1_MAN_OVRDRV(sword, status) \
                                                  sword &= 0xFFF3; \
                                                  sword |= (status << 2);
#define LS609X_PHY_PRG_LED2_MAN_OVRDRV(sword, status) \
                                                  sword &= 0xFFCF; \
                                                  sword |= (status<<4);


/* Bit Definition for LS609X_PHY_LED_CONTROL  */

#define LS609X_PHY_LED_BLINK_RATE_170             2
#define LS609X_PHY_LED_SR_STR_UP_170              4

/* 11:9 */
#define LS609X_PHY_PRG_BLINK_RATE(sword,rate)     sword &= 0xF1FF;   \
                                                  sword |= rate << 9;

/* 8:6 */
#define LS609X_PHY_PRG_SR_STR_UP(sword,rate)      sword &= 0xFE3F;   \
                                                  sword |= rate << 6;




#define LS609X_SMI_GLOBAL                         0x1B

#define LS609X_SMI_PHY_0                          0x00
#define LS609X_SMI_PORT_0                         0x10
#define LS609X_SMI_PORT_10                        0x1A
#define LS609X_SMI_UPLINK_PORT                    LS609X_SMI_PORT_10 


#define BIT_15S     (1U << 15)
#define BIT_14S     (1 << 14)
#define BIT_13S     (1 << 13)
#define BIT_12S     (1 << 12)
#define BIT_11S     (1 << 11) 
#define BIT_10S     (1 << 10)
#define BIT_9S      (1 << 9)
#define BIT_8S      (1 << 8)
#define BIT_7S      (1 << 7)
#define BIT_6S      (1 << 6)
#define BIT_5S      (1 << 5)
#define BIT_4S      (1 << 4)
#define BIT_3S      (1 << 3)
#define BIT_2S      (1 << 2)
#define BIT_1S      (1 << 1)
#define BIT_0S      1


#define EOK		0	/* Alias for SUCCESS */

/* Definitions for stats collection */
#define LS609X_STATS_BUSY                         BIT_15S
#define LS609X_STATS_OP_PORT_CAPT_ALL             (0x5 << 12)
#define LS609X_STATS_OP_PORT_CLEAR_ALL            (0x2 << 12)
#define LS609X_STATS_OP_PORT_RD_CAPT              (0x4 << 12)
#define LS609X_STATS_HIST_MODE_BOTH               (0x3 << 10)
#define LS609X_STATS_MAX_RETRY                    1000
#define LS609X_STATS_WAIT_BUSY                    500

#define LS609X_STATS_PTR_IN_UNICAST               0x04
#define LS609X_STATS_PTR_IN_BROADCAST             0x06
#define LS609X_STATS_PTR_IN_MULTICAST             0x07
#define LS609X_STATS_PTR_IN_RxErr                 0x1C
#define LS609X_STATS_PTR_IN_FCS_ERR               0x1D
#define LS609X_STATS_PTR_OUT_UNICAST              0x10
#define LS609X_STATS_PTR_OUT_BROADCAST            0x13
#define LS609X_STATS_PTR_OUT_MULTICAST            0x12
#define LS609X_STATS_PTR_OUT_FCS_ERR              0x03

/*********** SMI *****************/
#define ls609x_octeon_write_csr(x,y) cvmx_write_csr(x,y)
#define ls609x_octeon_read_csr(x)    cvmx_read_csr(x)


#define LS609X_OCTEON_MAX_SMI_RETRY_COUNT      10
#define LS609X_OCTEON_MAX_VALID_SMI_REG_ADDR   0x001F
#define LS609X_OCTEON_SMI_SEL_BIT              14
#define LS609X_OCTEON_GPIO_BIT_CFG_TX_OE       0x1
#define LS609X_OCTEON_SMI_SEL_BIT_MASK         (0x1 << LS609X_OCTEON_SMI_SEL_BIT)

#define  LS609X_OCTEON_IO_SEG 2LL

#define  LS609X_OCTEON_ADD_SEG(segment, add)  \
         ((((uint64_t)segment) << 62) | (add))

#define  LS609X_OCTEON_ADD_IO_SEG(add)   \
         LS609X_OCTEON_ADD_SEG(LS609X_OCTEON_IO_SEG, (add))

#define  LS609X_OCTEON_SMI_CMD    LS609X_OCTEON_ADD_IO_SEG(0x0001180000001800ull)
#define  LS609X_OCTEON_SMI_WR_DAT LS609X_OCTEON_ADD_IO_SEG(0x0001180000001808ull)
#define  LS609X_OCTEON_SMI_RD_DAT LS609X_OCTEON_ADD_IO_SEG(0x0001180000001810ull)
#define  LS609X_OCTEON_SMI_CLK    LS609X_OCTEON_ADD_IO_SEG(0x0001180000001818ull)
#define  LS609X_OCTEON_SMI_EN     LS609X_OCTEON_ADD_IO_SEG(0x0001180000001820ull)

#define LS609X_OCTEON_GPIO_BIT_CFGX(offset)  \
        (LS609X_OCTEON_ADD_IO_SEG(0x0001070000000800ull+((offset)*8)))

#define LS609X_OCTEON_GPIO_TX_CLR   0x8001070000000890ull

/* Generic */
#define LS609X_WAIT_USECS           100


/* Switch Specific */

#define LS609X_OCTEON_SW_MAX_SMI_CLK_KHZ 8333
#define LS609X_OCTEON_SW_SMI_CLOCK     31
#define LS609X_OCTEON_SW_SMI_PREABMLE  (1 << 12)
#define LS609X_OCTEON_SW_SAMPLE_LOW    (2 << 8)
#define LS609X_OCTEON_SW_SAMPLE_HIGH   (0 << 16)
#define LS609X_OCTEON_SW_SAMPLE \
        (LS609X_OCTEON_SW_SAMPLE_HIGH | LS609X_OCTEON_SW_SAMPLE_LOW)

typedef union {
    uint64_t u64;

    struct {
        uint64_t reserved                : 47;      /**< Reserved */
        uint64_t phy_op                  : 1;       /**< PHY Opcode
                                                         0=write
                                                         1=read */
        uint64_t reserved1               : 3;       /**< Reserved */
        uint64_t phy_adr                 : 5;       /**< PHY Address */
        uint64_t reserved0               : 3;       /**< Reserved */
        uint64_t reg_adr                 : 5;       /**< PHY Register Offset */
    } s;
} ls609x_octeon_smi_cmd_t;

typedef union {
    uint64_t u64;

    struct {
        uint64_t reserved                : 46;      /**< Reserved */
        uint64_t pending                 : 1;       /**< Read Xaction Pending */
        uint64_t val                     : 1;       /**< Read Data Valid */
        uint64_t dat                     : 16;      /**< Read Data */
    } s;
} ls609x_octeon_smi_rd_dat_t;

typedef union {
    uint64_t u64;

    struct {
        uint64_t reserved                : 46;      /**< Reserved */
        uint64_t pending                 : 1;       /**< Write Xaction Pending */
        uint64_t val                     : 1;       /**< Write Data Valid */
        uint64_t dat                     : 16;      /**< Write Data */
    } s;
} ls609x_octeon_smi_wr_dat_t;

/**************function defs *****************/

int ls609x_drv_pic_init(void);
int ls609x_drv_read_reg (uint16_t phy_adr, uint16_t reg_adr,
                         uint16_t *val);

int ls609x_drv_write_reg (uint16_t phy_adr, uint16_t reg_adr,
                          uint16_t dat);
#endif /* __LS609X_DRV_H__*/
