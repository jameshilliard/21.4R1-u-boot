/*
 * Copyright (c) 2008-2011, Juniper Networks, Inc.
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
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/gpl-2.0.html>.
 */

#ifndef __LC_CPLD__
#define __LC_CPLD__

enum {
	LC_CPLD_REVISION = 0x00,  /* LCPU Boot CPLD revision reg */
	LC_CPLD_RESET_CTRL = 0x01,
	LC_CPLD_RESET_REASON = 0x02,
	LC_CPLD_CTRL = 0x03,
	LC_CPLD_SYS_TIMER = 0x04,
	LC_CPLD_WATCHDOG_HI = 0x05,
	LC_CPLD_WATCHDOG_LO = 0x06,
	LC_CPLD_VCPU_RESET_CTRL = 0x07,     /* specific for resetting VCPU */
	LC_CPLD_STATUS = 0x08,              /* Line card status reg */
	LC_CPLD_FLASH_WP = 0x09,            /* Boot and NAND Flash device Protection */
	LC_CPLD_BOOT_CTLSTAT = 0x0A,        /* Upgrade bit */
	LC_CPLD_IRQ_STATUS = 0x0E,
	LC_CPLD_IRQ_ENABLE = 0x0F,
	LC_CPLD_TIMER_IRQ_STATUS = 0x12,
	LC_CPLD_TIMER_IRQ_ENABLE = 0x13,
	LC_CPLD_FPGA_PROG_STATUS = 0x20,    /* FPGA programming status reg */
	LC_CPLD_FPGA_CFG = 0x21,            /* FPGA programming data reg */
	LC_CPLD_SCRATCH_REG = 0x30,         /* scratch pad register */
};

/* Note : only registers meaningful for u-boot are defined here */
enum {
    LC_CTRL_CPLD_SCRATCH = 0x00,  /* scratch reg */
    LC_CTRL_CPLD_ID = 0x01,  /* Ctrlpld ID */
    LC_CTRL_CPLD_VERSION = 0x02,
    LC_CTRL_SNTL_GLBL_RST = 0x2F, /*Sentinel Global Reset*/
    LC_CTRL_PHY_RST = 0x34,/*Phy reset*/
    LC_CTRL_PFE_RST = 0x35,/*PFE reset*/
    LC_CTRL_TCAM_CRST = 0x36,/*TCAM core reset*/
    LC_CTRL_TCAM_SRST = 0x37,/*TCAM system reset*/
    LC_CTRL_PFE_GLBL_DLL_OFF = 0x38,/*PFE DLL Global Off*/
    LC_CTRL_2XS_44GE_48P_DLL_OFF = 0x39,  /* PFE DLL OFF for 2XS_44GE & 48P */
    LC_CTRL_TCAM_HSIM = 0x3B,  /* TCAM HSIM output */
    LC_CTRL_2XS_44GE_48P_PFE_CJ_RST = 0x3C,  /* Ch-J reset for 2XS_44GE & 48P */	
    LC_CTRL_SWITCH_SRST = 0x3D,/*Switch reset*/
    LC_CTRL_SFP_LINK_ACT_LED_GLOBAL = 0x50,/* Link/Act LED global control */
    LC_CTRL_CPLD_STATUS_LED = 0x61,  /* Ctrlpld status LED's */
    LC_CTRL_EXP_CPLD_RESET = 0x63,  /* Expander CPLD reset */
    LC_CTRL_LION_DEV_RESET = 0x35,  /* Lion device reset */
    LC_CTRL_DB_CPLD_CTRL_REG = 0x70,  /* DB CPLD Control Register */
    LC_CTRL_DB_40XS_PFE_LION_RESET = 0x84,  /* pumaj1, lion0 reset on 40xs */        
    LC_CTRL_DB_40XS_PFE_DLL_OFF = 0x85,  /* pfe1 dll off on DB 40xs */
    LC_CTRL_DB_40XS_TCAM_CRST = 0x86,  /* tcam core reset1 on DB 40XS */
    LC_CTRL_DB_40XS_TCAM_SRST = 0x87,  /* tcam system reset1 on DB 40XS */

};

/* reboot reason mask */
#define BOOT_REBOOT_REASON_MASK	0xfc

#define LC_XCTL_CPLD_VERSION	0x2

/* Reg 50. LED software override bit */
#define CTRL_CPLD_LINK_ACT_LED_SW_OVRRIDE    (1 << 0)

/* Reg 0x3d. Switch reset */
#define PCIEX_SWITCH_1_RESET    (1 << 3)
#define PCIEX_SWITCH_0_RESET    (1 << 2)
#define ETH_SWITCH_1_RESET      (1 << 1)
#define ETH_SWITCH_0_RESET      (1 << 0)

/* Reg 0x70 bit definition */
#define DB_CPLD_RW_BUSY         (1 << 4)

#define LC_CPLD_FIRST_REG (LC_CPLD_REVISION)
#define LC_CPLD_LAST_REG (LC_CPLD_FPGA_CFG)

/* LC_CPLD_REVISION */
#define LC_CPLD_REV_0 (1 << 0)
#define LC_CPLD_REV_1 (1 << 1)
#define LC_CPLD_REV_2 (1 << 2)
#define LC_CPLD_REV_3 (1 << 3)
#define LC_CPLD_REV_4 (1 << 4)
#define LC_CPLD_REV_5 (1 << 5)
#define LC_CPLD_REV_6 (1 << 6)
#define LC_CPLD_REV_7 (1 << 7)

/* LC_CPLD_RESET_CTRL */
#define LC_CPLD_RESET_CPU (1 << 0)
#define LC_CPLD_SFPGA1_RESET (1 << 1)
#define LC_CPLD_SFPGA2_RESET (1 << 2)
#define LC_CPLD_LPHY_RESET (1 << 5)
#define LC_CPLD_BOOT_FLASH_RESET (1 << 6)

/* LC_CPLD_RESET_REASON */
#define LC_CPLD_WDT_LEVEL2 (1 << 2)
#define LC_CPLD_WDT_LEVEL3 (1 << 3)
#define LC_CPLD_WDT_LEVEL4 (1 << 4)

/* LC_CPLD_CTRL */
#define LC_CPLD_CPU_BOOT_LED (1 << 0)
#define LC_CPLD_FLASH_PARTITION_SELECT (1 << 4)
#define LC_CPLD_ALLOW_FLASH_SWIZZLE (1 << 5)
#define LC_CPLD_WDT_ENABLE (1 << 6)
#define LC_CPLD_ENABLE_SYS_TIMER (1 << 7)

/* LC_CPLD_SYS_TIMER */
#define LC_CPLD_SYS_TIMER_0 (1 << 0)
#define LC_CPLD_SYS_TIMER_1 (1 << 1)
#define LC_CPLD_SYS_TIMER_2 (1 << 2)
#define LC_CPLD_SYS_TIMER_3 (1 << 3)
#define LC_CPLD_SYS_TIMER_4 (1 << 4)
#define LC_CPLD_SYS_TIMER_5 (1 << 5)
#define LC_CPLD_SYS_TIMER_6 (1 << 6)
#define LC_CPLD_SYS_TIMER_7 (1 << 7)

/* LC_CPLD_WATCHDOG_HI */
#define LC_CPLD_WDT_HI_0 (1 << 0)
#define LC_CPLD_WDT_HI_1 (1 << 1)
#define LC_CPLD_WDT_HI_2 (1 << 2)
#define LC_CPLD_WDT_HI_3 (1 << 3)
#define LC_CPLD_WDT_HI_4 (1 << 4)
#define LC_CPLD_WDT_HI_5 (1 << 5)
#define LC_CPLD_WDT_HI_6 (1 << 6)
#define LC_CPLD_WDT_HI_7 (1 << 7)

/* LC_CPLD_WATCHDOG_LO */
#define LC_CPLD_WDT_LO_0 (1 << 0)
#define LC_CPLD_WDT_LO_1 (1 << 1)
#define LC_CPLD_WDT_LO_2 (1 << 2)
#define LC_CPLD_WDT_LO_3 (1 << 3)
#define LC_CPLD_WDT_LO_4 (1 << 4)
#define LC_CPLD_WDT_LO_5 (1 << 5)
#define LC_CPLD_WDT_LO_6 (1 << 6)
#define LC_CPLD_WDT_LO_7 (1 << 7)

/* LC_CPLD_VCPU_RESET_CTRL */
#define LC_CPLD_VCPU_HRST (1 << 0)
#define LC_CPLD_VCPU_SRST (1 << 1)

/* LC_CPLD_STATUS */
#define LC_CPLD_SLOT_MASK  (0x1F)
#define LC_CPLD_SLOT_ID_PARITY (1 << 5)
#define LC_CPLD_LC_STANDALONE (1 << 6)
#define LC_CPLD_MASTER_SELECT (1 << 7)

/* LC_CPLD_FLASH_WP */
#define LC_CPLD_BOOT_FLASH_WP (1 << 0)  /* 1 = Not protect, 0 = protect */
#define LC_CPLD_NAND_FLASH_WP (1 << 1)  /* 1 = Not protect, 0 = protect */

/* LC_CPLD_IRQ_STATUS */
#define LC_CPLD_RE_SWITCHOVER_INT (1 << 0)  

/* LC_CPLD_IRQ_ENABLE */
#define LC_CPLD_ENABLE_RE_SWITCHOVER (1 << 0)
#define LC_CPLD_ENABLE_I2CS_INT (1 << 1)

/* LC_CPLD_TIMER_IRQ_STATUS */
#define LC_CPLD_TIMER_IRQ_0 (1 << 0)
#define LC_CPLD_TIMER_IRQ_1 (1 << 1)

/* LC_CPLD_TIMER_IRQ_ENABLE */
#define LC_CPLD_ENABLE_TIMER_IRQ_0 (1 << 0)
#define LC_CPLD_ENABLE_TIMER_IRQ_1 (1 << 1)

/* LC_CPLD_FPGA_PROG_STATUS */
#define LC_CPLD_FPGA_PROG (1 << 0)
#define LC_CPLD_FPGA_INIT (1 << 1)
#define LC_CPLD_FPGA1_DONE (1 << 2)
#define LC_CPLD_FPGA2_DONE (1 << 3)
#define LC_CPLD_READY_NEXT (1 << 4)

/* LC_CPLD_FPGA_CFG */
#define LCPLD_PROG_DATA_0 (1 << 0)
#define LCPLD_PROG_DATA_1 (1 << 1)
#define LCPLD_PROG_DATA_2 (1 << 2)
#define LCPLD_PROG_DATA_3 (1 << 3)
#define LCPLD_PROG_DATA_4 (1 << 4)
#define LCPLD_PROG_DATA_5 (1 << 5)
#define LCPLD_PROG_DATA_6 (1 << 6)
#define LCPLD_PROG_DATA_7 (1 << 7)

/* BootCPLD regs */
#define BTCPLD_FLASH_SWIZZLE_REG 0x0A

#define BTCPLD_REG0A_CPU_BOOTED  (1 << 0)

/* prototypes */
int lc_cpld_valid_reg_read (int);
int lc_cpld_valid_reg_write (int);
int lc_cpld_reg_read (int, uint8_t*);
int lc_cpld_reg_write(int, uint8_t);
int lc_cpld_vcpu_hard_reset (void);
int lc_cpld_vcpu_soft_reset (void);
#endif /*__LC_CPLD__*/
