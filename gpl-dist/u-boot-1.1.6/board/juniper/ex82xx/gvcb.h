/*
 * Copyright (c) 2008-2010, Juniper Networks, Inc.
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

#ifndef __GVCB_H__
#define __GVCB_H__

#include <linux/types.h>


#define PCI_VENDORID(x)         ((x) & 0xFFFF)
#define PCI_DEVICEID(x)         (((x) >> 16) & 0xFFFF)

#define GVCB_GET_SOFTC(dev)        ((dev)->si_drv1)
#define GVCB_SET_SOFTC(dev, softc) ((dev)->si_drv1 = (softc))
#define CHASSIS_EX82XX_MAX_CB    3



typedef struct gvcb_fpga_regs_ {
	volatile u_int32_t gvcb_int_src;            /* 0x0, interrupt status register */
	volatile u_int32_t gvcb_int_en;             /* 0x4, interrupt mask register */
	volatile u_int32_t gvcb_i2cs_mux_ctrl;      /* 0x8, i2c select register */
	volatile u_int32_t gvcb_timer_cnt;          /* 0xC, timer count register */
	volatile u_int32_t gvcb_slot_presence;      /* 0x10, slot presence register */
	volatile u_int32_t gvcb_ch_presence;        /* 0x14, presence of various chassis */
	volatile u_int32_t gvcb_soft_reset;         /* 18, not used for Grande */

	volatile u_int32_t gvcb_pwr_status;         /* 0x1C, power status register */
	volatile u_int32_t gvcb_i2cs_int;           /* 0x20, i2c slave interrupt register */
	volatile u_int32_t gvcb_i2cs_int_en;        /*24 i2c slave mask register */

	volatile u_int32_t gvcb_i2cs_int2_reg;      /* 0x28, i2c slave interrupt reg for venti */
	volatile u_int32_t gvcb_i2cs_int2_en ;      /* 0x2c, i2c slave interrupt enable register */

	volatile u_int32_t gvcb_fan_cntrl_status;   /* 0x30, fan control status register */
	volatile u_int32_t reserved2;               /* 0x34, not used for Grade */
	volatile u_int32_t reserved3;               /* 0x38, not used for Grande */

	volatile u_int32_t gvcb_mstr_cnfg;          /* 0x3C, master config register */
	volatile u_int32_t gvcb_mstr_alive;         /* 0x40, master alive register */
	volatile u_int32_t gvcb_mstr_alive_cnt;     /* 0x44, master alive count register */

	volatile u_int32_t reserved4[6];            /* 0x48-0x5c */

	volatile u_int32_t gvcb_fpga_rev;           /* 0x60, fpga revision version */
	volatile u_int32_t gvcb_fpga_date;          /* 0x64, fpga date */

	volatile u_int32_t gvcb_sgbus_error;        /* 0x68, sgbus error register */
	volatile u_int32_t gvcb_sgbus_int;          /* 0x6C, sgbus interrupt register */
	volatile u_int32_t gvcb_sgbus_int_en;       /* 0x70, sgbus interrupt enable */

	volatile u_int32_t gvcb_sgbus_err_addr;     /* 0x74, sgbus error address register */
	volatile u_int32_t gvcb_sgbus_data;         /* 0x78, sgbus data */
	volatile u_int32_t gvcb_sgbus_reset;        /* 0x7C, sgbus reset register */
	volatile u_int32_t gvcb_sgbus_addr;         /* 0x80, sgbus address register */
	volatile u_int32_t gvcb_sgbus_ctrl;         /* 0x84, sbus control register */

	volatile u_int32_t reserved6;               /* 0x88, Not defined for Grande */
	volatile u_int32_t reserved5;               /* 0x8C, Not defined for Grande */

	volatile u_int32_t gvcb_srdes_ctrl;         /* 0x90, serdes control register */
	volatile u_int32_t gvcb_device_ctrl;        /* 0x94, device control register */
	volatile u_int32_t gvcb_scratch_reg;        /* 0x98, fpga scratch register */
} gvcb_fpga_regs_t;


/*
 * Defines for Interrupts
 */
/* int_src 0x0 */
#define GVCB_ISRC_SGLINK_MASTER_ERR          (1 << 0)
#define GVCB_ISRC_SGLINK_INT                 (1 << 1)
#define GVCB_ISRC_SGLINK_LINK_STATE          (1 << 2)
#define GVCB_ISRC_CSW_DEBUG_PHY              (1 << 10)
#define GVCB_ISRC_VSW_DEBUG_PHY              (1 << 11)
#define GVCB_ISRC_CH_PRS_CHG                 (1 << 16)
#define GVCB_ISRC_FAN_FAIL                   (1 << 20)
#define GVCB_ISRC_PEM_STATUS_CHG             (1 << 21)
#define GVCB_ISRC_I2CS_INT                   (1 << 23)
#define GVCB_ISRC_TIMER_INT                  (1 << 25)
#define GVCB_ISRC_SECURITY_CHIP              (1 << 30)

/* i2c cntrl */
#define GVCB_I2C_SRC_SELECT_SCBC             (1 << 0)
#define GVCB_I2C_FPGA_SOFT_RST               (1 << 0)
#define GVCB_I2C_FPGA_SOFT_RST_ACK           (1 << 1)
#define GVCB_I2C_FPGA_RST_REASON             (1 << 6)  /* set to 1 for soft reset */
#define GVCB_I2C_MUX_SELECT_MASK             (0x807F)

/* Timer enable/disable 0x0C */
#define GVCB_TIMER_ENABLE                    (1 << 31)
#define GVCB_TIMER_VALUE_SET_MASK            (0xFF0000)

/* Chassis presence  0x14 */
#define GVCB_CHASSIS_PRESENCE_FPC(slot)      (1 << slot)
#define GVCB_CHASSIS_PRESENCE_SCB(slot)      (1 << (16 + slot))
#define GVCB_CHASSIS_PRESENCE_PSU(slot)      (1 << (19 + slot))
#define GVCB_CHASSIS_PRESENCE_FTC(slot)      (1 << (25 + slot))
#define GVCB_CHASSIS_PRESENCE_VNPU(slot)     (1 << (27 + slot))

/* Pwr Status registers 0x1C */
#define GVCB_CHASSIS_PWR_STATUS_PEM(slot)    (1 << slot)
#define GVCB_CHASSIS_PEM0_TYPE_AC(slot)      (1 << (6 + slot))

/* i2cs register 0x20 */
#define GVCB_I2CS_DPC_INT(slot)              (1 << slot)
#define GVCB_I2CS_GSCB_INT(slot)             (1 << (16 + slot))
#define GVCB_I2CS_PSU_INT(slot)              (1 << (19 + slot))
#define GVCB_I2CS_FTC_INT(slot)              (1 << (25 + slot))
#define GVCB_I2CS_VNPU_INT(slot)             (1 << (27 + slot))
#define GVCB_I2CS_LCD_INT                    (1 << 29)

/* Fan mastership fault 0x30 */
#define GVCB_FAN_TRAY1_STATUS                (1 << 0)
#define GVCB_FAN_TRAY2_STATUS                (1 << 1)
#define GVCB_FAN_CTRL1_MSHIP_FAULT           (1 << 4)
#define GVCB_FAN_CTRL2_MSHIP_FAULT           (1 << 5)
#define GVCB_FAN_CTRL1_MAX_SPEED_SET         (1 << 16)
#define GVCB_FAN_CTRL2_MAX_SPEED_SET         (1 << 17)

#define GVCB_FAN_ORIENTATION_NORMAL          (1 << 28)
#define GVCB_FAN_ORIENTATION_REVERSE         (1 << 29)

/* Mastership status */
/*
 * Macro definitions for Mastership Config
 */
#define GVCB_MCONF_BOOTED                    (1 << 0)
#define GVCB_MCONF_RELINQUISH                (1 << 1)
#define GVCB_MCONF_IM_RDY                    (1 << 3)
#define GVCB_MCONF_IM_MASTER                 (1 << 4)
#define GVCB_MCONF_HE_RDY                    (1 << 5)
#define GVCB_MCONF_HE_MASTER                 (1 << 6)

/*
 * Macro definitions for Mastership Alive Count
 */
#define GVCB_MSTR_ALIVE_CNT_MSK              (0x7F)
#define GVCB_MSTR_ALIVE_CNT_OPER_SIG         (1 << 8)

/*
 * Macro definitions for SGBUS Error 0x68
 */
#define GVCB_SGBUS_ERROR_UNKNOWN_SLAVE       (1 << 0)
#define GVCB_SGBUS_ERROR_ILLEGAL_REPLY       (1 << 1)
#define GVCB_SGBUS_ERROR_CPU_ADDR_RANGE      (1 << 2)
#define GVCB_SGBUS_ERROR_TIME_OUT            (1 << 3)
#define GVCB_SGBUS_ERROR_REPLY_CRC           (1 << 4)
#define GVCB_SGBUS_ERROR_REPLY               (1 << 5)
#define GVCB_SGBUS_ERROR_GE_PHY_RX_OVRFLOW   (1 << 6)
#define GVCB_SGBUS_ERROR_GE_PHY_RX_CV        (1 << 7)
#define GVCB_SGBUS_ERROR_MASK                (0xff)

/*
 * Macro definitions for SGBUS INT 0x6C
 */

#define GVCB_SGBUS_SCB0_INT                   (1 << 0)
#define GVCB_SGBUS_SCB1_INT                   (1 << 1)
#define GVCB_SGBUS_GSIB_INT                   (1 << 2)

/*
 * Macro definitions for SGBUS reset 0x7C
 */
#define GVCB_SGBUS_SCB0_INT                   (1 << 0)
#define GVCB_SGBUS_SCB1_INT                   (1 << 1)
#define GVCB_SGBUS_GSIB_INT                   (1 << 2)

/*
 * SGBUS Ctrl 0x84
 */

/*
 * Macro definitions for SGBUS Control
 */
#define GVCB_SGBUS_CTRL_START                (1 << 0)
#define GVCB_SGBUS_CTRL_BUSY                 GVCB_SGBUS_CTRL_START
#define GVCB_SGBUS_CTRL_READ                 (1 << 1)
#define GVCB_SGBUS_CTRL_WRITE                (0 << 1)
#define GVCB_SGBUS_CTRL_SEL(slot)            (((slot) & 0x3) << 8)
#define GVCB_SGBUS_CTRL_GET_SLOT(val)        (((val) >> 8) & 0x3)

/*
 * Macro definitions for SERDES CTRL 0x90
 */
#define GVCB_SERDES_CTRL_CH_STATUS(channel)   (1 << channel)
#define GVCB_SERDES_CTRL_CH_RESET(channel)    (1 << (8 + channel))
#define GVCB_SERDES_CTRL_CH_LBCK(channel)     (1 << (16 + channel))
#define GVCB_SERDES_CTRL_CH_PLL_LOCK(channel) (1 << (24 + channel))

/*
 * Macro definitions for Reset Cntrl 0x94
 */
#define GVCB_RST_CNTL_CSW_RESET              (1 << 7)
#define GVCB_RST_CNTL_VSW_RESET              (1 << 8)
#define GVCB_RST_CNTL_SEC_RESET              (1 << 12)

#define GVCBIO_SIGINTR     _IOW('m', 1, gvcb_sigint_t)      /* signal on cb ints */
#define GVCBIO_RESETSIG    _IOW('m', 2, u_int32_t)          /* enable & ack sigs */
#define GVCBIO_GETINTSRC   _IOR('m', 3, u_int32_t)          /* get INT_SRC reg */
#define GVCBIO_SETINTSRC   _IOW('m', 4, u_int32_t)          /* set INT_SRC reg */
#define GVCBIO_RESET_FPGA  _IOR('m', 5, u_int32_t)          /* Reset FPGA */
#define GVCBIO_GETPRESENCE _IOR('m', 6, u_int32_t)          /* bd presence */
#define GVCBIO_GETPWRSTAT  _IOR('m', 7, u_int8_t)           /* power status */
#define GVCBIO_SETI2CSINT  _IOW('m', 8, u_int32_t)          /* set I2CS INT reg */
#define GVCBIO_GETI2CSINT  _IOR('m', 9, u_int32_t)          /* get I2CS INT reg */
#define GVCBIO_SETI2CSINT_EN  _IOW('m', 10, u_int32_t)      /* set I2CS INT reg */
#define GVCBIO_GETI2CSINT_EN  _IOR('m', 11, u_int32_t)      /* get I2CS INT reg */
#define GVCBIO_GETFANSTAT  _IOR('m', 12, u_int32_t)         /* get fan status */
#define GVCBIO_SETFANCTRL  _IOW('m', 13, u_int32_t)
#define GVCBIO_GETMASTRCFG _IOR('m', 14, u_int32_t)         /* mastership status */
#define GVCBIO_SETMASTRCFG _IOW('m', 15, u_int8_t)          /* mastership control */
#define GVCBIO_SETFANSTAT  _IOW('m', 16, u_int32_t)         /* set fan status */
#define GVCBIO_SETI2C_GROUP _IOW('m', 17, u_int32_t)        /* set i2c debug flag */
#define GVCBIO_DEBUG_FLAG  _IOW('m', 18, u_int32_t)         /* set degbug flag */
#define GVCBIO_IIC_IOCTL_SET_CMD  _IOW('m', 19, u_int32_t)  /* set offset for lseek */
#define GVCBIO_TEST_CONT_READ _IOW('m', 20, u_int32_t)      /*test contineous read */

/*
 * Defines for Mastership Config
 */
#define CCB_MCONF_BOOTED                    (1 << 0)
#define CCB_MCONF_RELINQUISH                (1 << 1)
#define CCB_MCONF_IM_RDY                    (1 << 3)
#define CCB_MCONF_IM_MASTER                 (1 << 4)
#define CCB_MCONF_HE_RDY                    (1 << 5)
#define CCB_MCONF_HE_MASTER                 (1 << 6)

#define PCI_JNX_VENDORID		0x1172
#define PCI_JNX_DEVICEID_CCB    0x0032
#define PCI_JNX_DEVICEID_VCBC	0x0033

#endif /* __GVCB_H_ */
