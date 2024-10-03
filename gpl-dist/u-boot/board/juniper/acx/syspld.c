/*
 * $Id$
 *
 * syspld.c -- ACX syspld related routines
 *
 * Rajat Jain, Feb 2011
 *
 * Copyright (c) 2011-2013, Juniper Networks, Inc.
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

#include <common.h>
#include <command.h>
#include "fortius_syspld_struct.h"
#include "fortius_f_syspld_struct.h"
#include "fortius_m_syspld_struct.h"
#include "syspld.h"

DECLARE_GLOBAL_DATA_PTR;

static volatile fortius_syspld *syspld_g = (volatile fortius_syspld *)CONFIG_SYS_SYSPLD_BASE;
static volatile fortius_f_syspld *syspld_f = (volatile fortius_f_syspld *)CONFIG_SYS_SYSPLD_BASE;
static volatile fortius_m_syspld *syspld_m = (volatile fortius_m_syspld *)CONFIG_SYS_SYSPLD_BASE;

uint8_t syspld_reg_read(unsigned int reg)
{
    volatile uint8_t *ptr = (volatile uint8_t *) CONFIG_SYS_SYSPLD_BASE;
    uint8_t val;
    
    val = ptr[reg];
    __asm__ __volatile__ ("eieio");
    return val;
}

void syspld_reg_write(unsigned int reg, uint8_t val)
{
    volatile uint8_t *ptr = (volatile uint8_t *) CONFIG_SYS_SYSPLD_BASE;

    ptr[reg] = val;
    __asm__ __volatile__ ("eieio");
}

#define ACX_VARIANT_SET_BIT_BY_NAME(reg, f_bitpos, g_bitpos, m_bitpos)	\
do {									\
    switch(gd->board_type) {						\
    case I2C_ID_ACX1000:						\
    case I2C_ID_ACX1100:						\
	syspld_f->re.reg |= (1 << FORTIUS_F_##f_bitpos);		\
	break;								\
    case I2C_ID_ACX4000:						\
	syspld_m->re.reg |= (1 << FORTIUS_M_##m_bitpos);		\
	break;								\
    case I2C_ID_ACX2000:						\
    case I2C_ID_ACX2100:						\
    case I2C_ID_ACX2200:						\
    default:								\
	syspld_g->re.reg |= (1 << g_bitpos);				\
    }									\
    __asm__ __volatile__ ("eieio");					\
} while(0)

#define ACX_VARIANT_SET_BIT(reg, bitpos)				\
do {									\
    switch(gd->board_type) {						\
    case I2C_ID_ACX1000:						\
    case I2C_ID_ACX1100:						\
	syspld_f->re.reg |= (1 << FORTIUS_F_##bitpos);			\
	break;								\
    case I2C_ID_ACX4000:						\
	syspld_m->re.reg |= (1 << FORTIUS_M_##bitpos);			\
	break;								\
    case I2C_ID_ACX2000:						\
    case I2C_ID_ACX2100:						\
    case I2C_ID_ACX2200:						\
    default:								\
	syspld_g->re.reg |= (1 << bitpos);				\
	break;								\
    }									\
    __asm__ __volatile__ ("eieio");					\
} while(0)

#define ACX_VARIANT_CLEAR_BIT_BY_NAME(reg, f_bitpos, g_bitpos, m_bitpos)\
do {									\
    switch(gd->board_type) {				    		\
    case I2C_ID_ACX1000:						\
    case I2C_ID_ACX1100:						\
	syspld_f->re.reg &= ~(1 << FORTIUS_F_##f_bitpos);		\
	break;								\
    case I2C_ID_ACX4000:						\
	syspld_m->re.reg &= ~(1 << FORTIUS_M_##m_bitpos);		\
	break;								\
    case I2C_ID_ACX2000:						\
    case I2C_ID_ACX2100:						\
    case I2C_ID_ACX2200:						\
    default:								\
	syspld_g->re.reg &= ~(1 << g_bitpos);				\
	break;								\
    }									\
    __asm__ __volatile__ ("eieio");					\
} while(0)

#define ACX_VARIANT_CLEAR_BIT(reg, bitpos)				\
do {									\
    switch(gd->board_type) {						\
    case I2C_ID_ACX1000:						\
    case I2C_ID_ACX1100:						\
	syspld_f->re.reg &= ~(1 << FORTIUS_F_##bitpos);			\
	break;								\
    case I2C_ID_ACX4000:						\
	syspld_m->re.reg &= ~(1 << FORTIUS_M_##bitpos);			\
	break;								\
    case I2C_ID_ACX2000:						\
    case I2C_ID_ACX2100:						\
    case I2C_ID_ACX2200:						\
    default:								\
	syspld_g->re.reg &= ~(1 << bitpos);				\
	break;								\
    }									\
    __asm__ __volatile__ ("eieio");					\
} while(0)

#define ACX_VARIANT_SET_VAL(reg, val)					\
do {									\
    switch(gd->board_type) {						\
    case I2C_ID_ACX1000:						\
    case I2C_ID_ACX1100:						\
	syspld_f->re.reg = val;						\
	break;								\
    case I2C_ID_ACX4000:						\
	syspld_m->re.reg = val;						\
	break;								\
    case I2C_ID_ACX2000:						\
    case I2C_ID_ACX2100:						\
    case I2C_ID_ACX2200:						\
    default:								\
	syspld_g->re.reg = val;						\
	break;								\
    }									\
    __asm__ __volatile__ ("eieio");					\
} while(0)

#define ACX_VARIANT_BIT(bit)						\
    ((gd->board_type == I2C_ID_ACX1000)? FORTIUS_F_##bit :		\
	((gd->board_type == I2C_ID_ACX4000)? FORTIUS_M_##bit :		\
	 bit))
	
#define ACX_VARIANT_IS_BIT_SET(val, bit)				\
    ((gd->board_type == I2C_ID_ACX1000)? (val & 1<<FORTIUS_F_##bit) :	\
	((gd->board_type == I2C_ID_ACX4000) ?				\
	 (val & 1<<FORTIUS_M_##bit) : (val & 1<<bit)))

#define ACX_VARIANT_GET_VAL(reg)					\
    ((gd->board_type == I2C_ID_ACX1000)? syspld_f->re.reg :		\
	((gd->board_type == I2C_ID_ACX4000) ?				\
	 syspld_m->re.reg : syspld_g->re.reg))


uint8_t syspld_scratch1_read(void)
{
    return ACX_VARIANT_GET_VAL(scratch_1);
}

void syspld_scratch1_write(uint8_t val)
{
    ACX_VARIANT_SET_VAL(scratch_1, val);
}

void syspld_scratch2_write(uint8_t val)
{
    /* There IS no scratch2 register in Fortius. 
     * Use a dummy write on a readonly register just to drive the desired 
     * pattern on the data bus.
     */
    ACX_VARIANT_SET_VAL(version, val);
}

int syspld_version(void)
{
    return ACX_VARIANT_GET_VAL(version);
}

int syspld_revision(void)
{
    return ACX_VARIANT_GET_VAL(revision);
}

int syspld_get_logical_bank(void)
{
    int val = ACX_VARIANT_GET_VAL(flash_swizzle_ctrl);
    
    if (ACX_VARIANT_IS_BIT_SET(val, RE_REGS_FLASH_SWIZZLE_CTRL_ADDR_BIT_A23_MSB))
	return 1;
    else
	return 0;
}

void syspld_set_logical_bank(uint8_t bank)
{
    if (bank)
	ACX_VARIANT_SET_BIT(flash_swizzle_ctrl, RE_REGS_FLASH_SWIZZLE_CTRL_ADDR_BIT_A23_MSB);
    else
	ACX_VARIANT_CLEAR_BIT(flash_swizzle_ctrl, RE_REGS_FLASH_SWIZZLE_CTRL_ADDR_BIT_A23_MSB);
}

void syspld_cpu_reset(void)
{
    ACX_VARIANT_CLEAR_BIT(main_reset_cntrl, RE_REGS_MAIN_RESET_CNTRL_CPLD_CPU_HRESET_MSB);
}

void syspld_watchdog_disable(void)
{
    ACX_VARIANT_CLEAR_BIT(core_watchdog_timer_cntrl, RE_REGS_CORE_WATCHDOG_TIMER_CNTRL_TIMER_EN_MSB);
}

void syspld_watchdog_enable(void)
{
    ACX_VARIANT_SET_BIT(core_watchdog_timer_cntrl, RE_REGS_CORE_WATCHDOG_TIMER_CNTRL_TIMER_EN_MSB);
}

void syspld_watchdog_tickle(void)
{
    ACX_VARIANT_CLEAR_BIT(core_watchdog_timer_cntrl, RE_REGS_CORE_WATCHDOG_TIMER_CNTRL_TIMER_RESTART_MSB);
    ACX_VARIANT_SET_BIT(core_watchdog_timer_cntrl, RE_REGS_CORE_WATCHDOG_TIMER_CNTRL_TIMER_RESTART_MSB);
}

uint8_t syspld_get_wdt_timeout(void)
{
    return (ACX_VARIANT_GET_VAL(core_watchdog_timer_count) + 2) / 3;
}

void syspld_set_wdt_timeout(uint8_t secs)
{
    ACX_VARIANT_SET_VAL(core_watchdog_timer_count, secs * 3);
}

void syspld_swizzle_enable(void)
{
    ACX_VARIANT_SET_BIT(flash_swizzle_ctrl, RE_REGS_FLASH_SWIZZLE_CTRL_FLASH_AUTO_SWIZZLE_ENA_MSB);
}

void syspld_swizzle_disable(void)
{
    ACX_VARIANT_CLEAR_BIT(flash_swizzle_ctrl, RE_REGS_FLASH_SWIZZLE_CTRL_FLASH_AUTO_SWIZZLE_ENA_MSB);
}

uint8_t syspld_get_reboot_reason(void)
{
    uint8_t reason = ~ACX_VARIANT_GET_VAL(main_reset_reason); /* register values are active low */
    uint8_t retval;
    
    if (ACX_VARIANT_IS_BIT_SET(reason, RE_REGS_MAIN_RESET_REASON_HRESET_CPLDBIT_SET_MSB) || 
	ACX_VARIANT_IS_BIT_SET(reason, RE_REGS_MAIN_RESET_REASON_CPLD_SELF_RESET_DRIVEN_MSB)) {
	retval = RESET_REASON_SW_INITIATED;
    } else if (ACX_VARIANT_IS_BIT_SET(reason, RE_REGS_MAIN_RESET_REASON_HRESET_REQ_STATUS_MSB)) {
	retval = RESET_REASON_HW_INITIATED;
    } else if (ACX_VARIANT_IS_BIT_SET(reason, RE_REGS_MAIN_RESET_REASON_CPLD_DRIVEN_HRESET_ON_SWIZZLE_MSB)) {
	retval = RESET_REASON_SWIZZLE;
    } else if (ACX_VARIANT_IS_BIT_SET(reason, RE_REGS_MAIN_RESET_REASON_WATCHDOG_HRESET_MSB)) {
	retval = RESET_REASON_WATCHDOG;
    } else {
	retval = RESET_REASON_POWERCYCLE;
    }

    /*
     * Always clear all reasons
     */
    ACX_VARIANT_SET_BIT(main_reset_reason, RE_REGS_MAIN_RESET_REASON_HRESET_CPLDBIT_SET_MSB);
    ACX_VARIANT_SET_BIT(main_reset_reason, RE_REGS_MAIN_RESET_REASON_CPLD_SELF_RESET_DRIVEN_MSB);
    ACX_VARIANT_SET_BIT(main_reset_reason, RE_REGS_MAIN_RESET_REASON_HRESET_REQ_STATUS_MSB);
    ACX_VARIANT_SET_BIT(main_reset_reason, RE_REGS_MAIN_RESET_REASON_WATCHDOG_HRESET_MSB);
    ACX_VARIANT_SET_BIT(main_reset_reason, RE_REGS_MAIN_RESET_REASON_CPLD_DRIVEN_HRESET_ON_SWIZZLE_MSB);

    return retval;
}

void syspld_reset_usb_phy(void)
{
    /* 
     * Only the early Fortius-G(s) had the USB PHY reset pin as active low
     */
    if (((gd->board_type == I2C_ID_ACX2000) ||
         (gd->board_type == I2C_ID_ACX2100) ||
        (gd->board_type == I2C_ID_ACX2200)) &&
        (syspld_version() < 2)) {
	ACX_VARIANT_CLEAR_BIT(ext_device_reset_cntrl, 
			    RE_REGS_EXT_DEVICE_RESET_CNTRL_USB_PHY_RESET_MSB);
	/* As per USB PHY datasheet */
	udelay(1); /* 1 us */
	ACX_VARIANT_SET_BIT(ext_device_reset_cntrl, 
			    RE_REGS_EXT_DEVICE_RESET_CNTRL_USB_PHY_RESET_MSB);
    } else {
	ACX_VARIANT_SET_BIT(ext_device_reset_cntrl, 
			    RE_REGS_EXT_DEVICE_RESET_CNTRL_USB_PHY_RESET_MSB);
	/* As per USB PHY datasheet */
	udelay(1); /* 1 us */
	ACX_VARIANT_CLEAR_BIT(ext_device_reset_cntrl, 
			    RE_REGS_EXT_DEVICE_RESET_CNTRL_USB_PHY_RESET_MSB);
    }
    /* Not specified in datasheet - use an arbitrarily large value */
    udelay(30000); /* 30 ms */ 
}

void syspld_init(void)
{
    /*
     * Turn on the System LED in green blinking mode.
     */
    ACX_VARIANT_SET_VAL(sys_led_cntrl, 
			(1 << ACX_VARIANT_BIT(RE_REGS_SYS_LED_CNTRL_SYS_GREEN_LED_EN_MSB)) | 
			(1 << ACX_VARIANT_BIT(RE_REGS_SYS_LED_CNTRL_SYS_GREEN_MODE_MSB)));

    /*
     * Put all devices in reset - except USB PHY
     */
    ACX_VARIANT_CLEAR_BIT(ext_device_reset_cntrl, 
			  RE_REGS_EXT_DEVICE_RESET_CNTRL_PHY_CPU_MGMT_RESET_MSB);
    ACX_VARIANT_CLEAR_BIT(ext_device_reset_cntrl, 
			  RE_REGS_EXT_DEVICE_RESET_CNTRL_I2C_SW_1_RESET_MSB);
    ACX_VARIANT_CLEAR_BIT_BY_NAME(ext_device_reset_cntrl, 
			  RE_REGS_EXT_DEVICE_RESET_CNTRL_CPU_DDR_RESET_MSB,
			  RE_REGS_EXT_DEVICE_RESET_CNTRL_CPU_DDR_RESET_MSB,
			  RE_REGS_EXT_DEVICE_RESET_CNTRL_HW_MONITOR_IC_RESET_MSB);
    ACX_VARIANT_CLEAR_BIT(ext_device_reset_cntrl, 
			  RE_REGS_EXT_DEVICE_RESET_CNTRL_USB_SW_RESET_MSB);
    ACX_VARIANT_CLEAR_BIT(ext_device_reset_cntrl, 
			  RE_REGS_EXT_DEVICE_RESET_CNTRL_USB_FLASH_CTRL_RESET_MSB);
    /* Arbitrary value larger than reset pulse width for all devices */
    udelay(30000); /* 30 ms */

    ACX_VARIANT_SET_BIT(ext_device_reset_cntrl, 
			  RE_REGS_EXT_DEVICE_RESET_CNTRL_PHY_CPU_MGMT_RESET_MSB);
    ACX_VARIANT_SET_BIT(ext_device_reset_cntrl, 
			  RE_REGS_EXT_DEVICE_RESET_CNTRL_I2C_SW_1_RESET_MSB);
    ACX_VARIANT_SET_BIT_BY_NAME(ext_device_reset_cntrl, 
			  RE_REGS_EXT_DEVICE_RESET_CNTRL_CPU_DDR_RESET_MSB,
			  RE_REGS_EXT_DEVICE_RESET_CNTRL_CPU_DDR_RESET_MSB,
			  RE_REGS_EXT_DEVICE_RESET_CNTRL_HW_MONITOR_IC_RESET_MSB);
    ACX_VARIANT_SET_BIT(ext_device_reset_cntrl, 
			  RE_REGS_EXT_DEVICE_RESET_CNTRL_USB_SW_RESET_MSB);
    ACX_VARIANT_SET_BIT(ext_device_reset_cntrl, 
			  RE_REGS_EXT_DEVICE_RESET_CNTRL_USB_FLASH_CTRL_RESET_MSB);
    /* This is enough, since we have a greater delay post USB PHY reset */
    udelay(5000); /* 5 ms */ 
}


void syspld_reg_dump_all(void)
{
    int i;

    for (i=0; i < sizeof(fortius_syspld); i++)
	printf("syspld[0x%x] = 0x%x\n", i, syspld_reg_read(i));
}

