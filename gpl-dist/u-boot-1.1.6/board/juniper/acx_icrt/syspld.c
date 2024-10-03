/*
 * $Id$
 *
 * syspld.c -- iCRT syspld access routines
 *
 * Amit Verma, March 2014
 *
 * Copyright (c) 2011-2014, Juniper Networks, Inc.
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
#include "syspld_js_defs_struct.h" /* Common JS Word definitions for syspld */
#include "acx2000_syspld_struct.h"
#include "acx1000_syspld_struct.h"
#include "acx4000_syspld_struct.h"
#include "acx2100_syspld_struct.h"
#include "acx500_syspld_struct.h"

#include <acx_syspld.h>

DECLARE_GLOBAL_DATA_PTR;

static volatile fortius_syspld *syspld_g = (volatile fortius_syspld *)CFG_SYSPLD_BASE;
static volatile fortius_f_syspld *syspld_f = (volatile fortius_f_syspld *)CFG_SYSPLD_BASE;
static volatile fortius_m_syspld *syspld_m = (volatile fortius_m_syspld *)CFG_SYSPLD_BASE;
static volatile fortius_gplus_syspld *syspld_gplus = (volatile fortius_gplus_syspld *)CFG_SYSPLD_BASE;
static volatile kotinos_syspld  *syspld_kotinos = (volatile kotinos_syspld *)CFG_SYSPLD_BASE;

uint8_t
syspld_reg_read(unsigned int reg)
{
    volatile uint8_t *ptr = (volatile uint8_t *) CFG_SYSPLD_BASE;
    uint8_t val = ptr[reg];
    __asm__ __volatile__ ("eieio");
    return val;
}

void
syspld_reg_write(unsigned int reg, uint8_t val)
{
    volatile uint8_t *ptr = (volatile uint8_t *) CFG_SYSPLD_BASE;
    ptr[reg] = val;
    __asm__ __volatile__ ("eieio");
}

#define ACX_VARIANT_SET_BIT(reg, bitpos)	\
do {						\
    switch(gd->board_type) {			\
    case I2C_ID_ACX500_O_POE_DC:                \
    case I2C_ID_ACX500_O_POE_AC:                \
    case I2C_ID_ACX500_O_DC:                    \
    case I2C_ID_ACX500_O_AC:                    \
    case I2C_ID_ACX500_I_DC:                    \
    case I2C_ID_ACX500_I_AC:                    \
	syspld_kotinos->re.reg |= (1 << KOTINOS_##bitpos);\
	break;					\
    case I2C_ID_ACX1000: 			\
    case I2C_ID_ACX1100: 			\
	syspld_f->re.reg |= (1 << FORTIUS_F_##bitpos);\
	break;					\
    case I2C_ID_ACX4000: 			\
	syspld_m->re.reg |= (1 << FORTIUS_M_##bitpos);\
	break;					\
    case I2C_ID_ACX2100:			\
	syspld_gplus->re.reg |= (1 << FORTIUS_GPLUS_##bitpos);  \
	break;					\
    case I2C_ID_ACX2000: 			\
    case I2C_ID_ACX2200: 			\
    default:					\
	syspld_g->re.reg |= (1 << bitpos);	\
	break;					\
    }						\
} while(0)

#define ACX_VARIANT_CLEAR_BIT(reg, bitpos)	\
do {						\
    switch(gd->board_type) {			\
    case I2C_ID_ACX500_O_POE_DC:                \
    case I2C_ID_ACX500_O_POE_AC:                \
    case I2C_ID_ACX500_O_DC:                    \
    case I2C_ID_ACX500_O_AC:                    \
    case I2C_ID_ACX500_I_DC:                    \
    case I2C_ID_ACX500_I_AC:                    \
	syspld_kotinos->re.reg &= ~(1 << KOTINOS_##bitpos);\
	break;					\
    case I2C_ID_ACX1000: 			\
    case I2C_ID_ACX1100: 			\
	syspld_f->re.reg &= ~(1 << FORTIUS_F_##bitpos);\
	break;					\
    case I2C_ID_ACX4000: 			\
	syspld_m->re.reg &= ~(1 << FORTIUS_M_##bitpos);\
	break;					\
    case I2C_ID_ACX2100:                        \
	syspld_gplus->re.reg &= ~(1 << FORTIUS_GPLUS_##bitpos);     \
	break;                                  \
    case I2C_ID_ACX2000: 			\
    case I2C_ID_ACX2200: 			\
    default:					\
	syspld_g->re.reg &= ~(1 << bitpos);	\
	break;					\
    }						\
} while(0)

#define ACX_VARIANT_SET_VAL(reg, val)		\
do {						\
    switch(gd->board_type) {			\
    case I2C_ID_ACX500_O_POE_DC:                \
    case I2C_ID_ACX500_O_POE_AC:                \
    case I2C_ID_ACX500_O_DC:                    \
    case I2C_ID_ACX500_O_AC:                    \
    case I2C_ID_ACX500_I_DC:                    \
    case I2C_ID_ACX500_I_AC:                    \
	syspld_kotinos->re.reg = val;		\
	break;					\
    case I2C_ID_ACX1000: 			\
    case I2C_ID_ACX1100: 			\
	syspld_f->re.reg = val;			\
	break;					\
    case I2C_ID_ACX4000: 			\
	syspld_m->re.reg = val;			\
	break;					\
    case I2C_ID_ACX2100:                        \
	syspld_gplus->re.reg = val;             \
	break;                                  \
    case I2C_ID_ACX2000: 			\
    case I2C_ID_ACX2200: 			\
    default:					\
	syspld_g->re.reg = val;			\
	break;					\
    }						\
} while(0)

#define ACX_VARIANT_BIT(bit)					\
    ((gd->board_type == I2C_ID_ACX1000)? FORTIUS_F_##bit :	\
	((gd->board_type == I2C_ID_ACX2100)? FORTIUS_GPLUS_##bit :  \
	((gd->board_type == I2C_ID_ACX4000)? FORTIUS_M_##bit :	\
	(IS_BOARD_ACX500(gd->board_type)) ? KOTINOS_##bit : \
	 bit)))
	
#define ACX_VARIANT_IS_BIT_SET(val, bit)				\
    ((gd->board_type == I2C_ID_ACX1000)? (val & 1<<FORTIUS_F_##bit) :	\
	((gd->board_type == I2C_ID_ACX2100)? (val & 1<<FORTIUS_GPLUS_##bit) : \
	((gd->board_type == I2C_ID_ACX4000)? (val & 1<<FORTIUS_M_##bit) : \
	(IS_BOARD_ACX500(gd->board_type)) ? (val & 1<<KOTINOS_##bit) : \
	 (val & 1<<bit))))

#define ACX_VARIANT_GET_VAL(reg)				\
    ((gd->board_type == I2C_ID_ACX1000)? syspld_f->re.reg :	\
	((gd->board_type == I2C_ID_ACX2100)? syspld_gplus->re.reg :\
	((gd->board_type == I2C_ID_ACX4000)? syspld_m->re.reg :\
	(IS_BOARD_ACX500(gd->board_type)) ? syspld_kotinos->re.reg :\
	 syspld_g->re.reg)))


uint8_t
syspld_scratch1_read(void)
{
    return ACX_VARIANT_GET_VAL(scratch_1);
}

void
syspld_scratch1_write(uint8_t val)
{
    ACX_VARIANT_SET_VAL(scratch_1, val);
}

void
syspld_scratch2_write(uint8_t val)
{
    /* There IS no scratch2 register in Fortius. Use a dummy write on a readonly register
     * just to drive the desired pattern on the data bus.
     */
    ACX_VARIANT_SET_VAL(version, val);
}

int
syspld_version(void)
{
    return ACX_VARIANT_GET_VAL(version);
}

int
syspld_revision(void)
{
    return ACX_VARIANT_GET_VAL(revision);
}

#define KOTINOS_SET_BIT(reg, bitpos)	\
do {						\
    switch(gd->board_type) {			\
    case I2C_ID_ACX500_O_POE_DC:                \
    case I2C_ID_ACX500_O_POE_AC:                \
    case I2C_ID_ACX500_O_DC:                    \
    case I2C_ID_ACX500_O_AC:                    \
    case I2C_ID_ACX500_I_DC:                    \
    case I2C_ID_ACX500_I_AC:                    \
	syspld_kotinos->re.reg |= (1 << KOTINOS_##bitpos);\
	break;					\
    default:					\
	printf("KOTINOS_SET_BIT " # reg " " # bitpos " : Unsupported Platform.\n");\
	break;					\
    }						\
} while(0)

#define KOTINOS_CLEAR_BIT(reg, bitpos)	\
do {						\
    switch(gd->board_type) {			\
    case I2C_ID_ACX500_O_POE_DC:                \
    case I2C_ID_ACX500_O_POE_AC:                \
    case I2C_ID_ACX500_O_DC:                    \
    case I2C_ID_ACX500_O_AC:                    \
    case I2C_ID_ACX500_I_DC:                    \
    case I2C_ID_ACX500_I_AC:                    \
	syspld_kotinos->re.reg &= ~(1 << KOTINOS_##bitpos);\
	break;					\
    default:					\
	printf("KOTINOS_CLEAR_BIT " # reg " " # bitpos " : Unsupported Platform.\n");\
	break;					\
    }						\
} while(0)

#define KOTINOS_SET_VAL(reg, val)		\
do {						\
    switch(gd->board_type) {			\
    case I2C_ID_ACX500_O_POE_DC:                \
    case I2C_ID_ACX500_O_POE_AC:                \
    case I2C_ID_ACX500_O_DC:                    \
    case I2C_ID_ACX500_O_AC:                    \
    case I2C_ID_ACX500_I_DC:                    \
    case I2C_ID_ACX500_I_AC:                    \
	syspld_kotinos->re.reg = val;		\
	break;					\
    default:					\
	printf("KOTINOS_SET_VAL " # reg " to %d : Unsupported Platform.\n", val);\
	break;					\
    }						\
} while(0)

#define KOTINOS_GET_VAL(reg)	\
	(((gd->board_type >= I2C_ID_ACX500_O_POE_DC) &&		\
	  (gd->board_type <= I2C_ID_ACX500_I_AC)) ?		\
	 syspld_kotinos->re.reg :				\
	 (printf("KOTINOS_GET_VAL " # reg " : Unsupported Platform\n"),0))

#define KOTINOS_IS_BIT_SET(val, bit)				\
	(((gd->board_type >= I2C_ID_ACX500_O_POE_DC) &&		\
	  (gd->board_type <= I2C_ID_ACX500_I_AC)) ?		\
	 (val & 1 << KOTINOS_##bit) :				\
	 (printf("KOTINOS_IS_BIT_SET " # val " " # bit " : Unsupported Platform\n"),0))

/*
 * returns the value of glass safe register
 */
uint8_t
syspld_get_gcr(void)
{
	return KOTINOS_GET_VAL(glass_safe_control);
}

/*
 * sets the glass safe to value gcr
 * returns
 *	value of gcr before set
 */
uint8_t
syspld_set_gcr(uint8_t gcr)
{
	uint8_t old_gcr;

	old_gcr = KOTINOS_GET_VAL(glass_safe_control);

	KOTINOS_SET_VAL(glass_safe_control, gcr);

	return old_gcr;
}

/*
 * returns 1 if glass safe is open (i.e. sticky lock is 0)
 * returns 0 otherwise
 */
int
syspld_is_glass_safe_open(void)
{
	uint8_t	gcr;

	gcr = KOTINOS_GET_VAL(glass_safe_control);

	return ! (KOTINOS_RE_REGS_GLASS_SAFE_CONTROL_STICKEY_LOCK_GET(gcr));
}

/*
 * close the glass safe
 *
 * returns 1 if we could close the glass safe
 * returns 0 if we couldn't close the glass safe
 */
int
syspld_close_glass_safe(void)
{
	uint8_t	gcr;

	gcr = KOTINOS_GET_VAL(glass_safe_control);
	gcr = KOTINOS_RE_REGS_GLASS_SAFE_CONTROL_STICKEY_LOCK_SET(gcr, 1);
	KOTINOS_SET_VAL(glass_safe_control, gcr);

	return ! syspld_is_glass_safe_open();
}

/*
 * set the nor flash write protection bits
 * inputs
 *	xxxx0000	- No write protection
 *	xxxx0001	- Protect Security Parameter Block
 *	xxxx0010	- Protect Loader
 *	xxxx0100	- Protect u-boot & env variables
 *	xxxx1000	- Protect iCRT
 *
 * return
 *	1 - if the write protection succeeded
 *	0 - if the write protection failed
 */
int
syspld_nor_flash_wp_set(uint8_t wp)
{
	uint8_t gcr;

	gcr = KOTINOS_GET_VAL(glass_safe_control);
	gcr = KOTINOS_RE_REGS_GLASS_SAFE_CONTROL_NOR_FLASH_WRITE_PROTECT3_SET(gcr, ((wp & 0xf)  >> 3));
	gcr = KOTINOS_RE_REGS_GLASS_SAFE_CONTROL_NOR_FLASH_WRITE_PROTECT2_0_SET(gcr, wp);
	KOTINOS_SET_VAL(glass_safe_control, gcr);

	gcr = KOTINOS_GET_VAL(glass_safe_control);

	return ( (wp & 0xf) &
		 ( (KOTINOS_RE_REGS_GLASS_SAFE_CONTROL_NOR_FLASH_WRITE_PROTECT3_GET(gcr) <<
		    KOTINOS_RE_REGS_GLASS_SAFE_CONTROL_NOR_FLASH_WRITE_PROTECT2_0_FWIDTH) |
		   (KOTINOS_RE_REGS_GLASS_SAFE_CONTROL_NOR_FLASH_WRITE_PROTECT2_0_GET(gcr)))
	       );
}

/*
 * Set the nvram write protection on or off
 * input
 *	on = 1	- enable write protection of nvram
 *	on = 0  - disable write protection of nvram
 *
 * return
 *	1 - if the requested protection was achieved
 *	0 - if the requested protection was not achieved
 */
int
syspld_nvram_wp(int on)
{
	uint8_t gcr;

	gcr = KOTINOS_GET_VAL(glass_safe_control);
	/* why using an if when a conditional operation could do? */
	/* This helps in single stepping and debugging */
	if (on) {
		gcr = KOTINOS_RE_REGS_GLASS_SAFE_CONTROL_NVRAM_WRITE_PROTECT_SET(gcr, 1);
	} else {
		gcr = KOTINOS_RE_REGS_GLASS_SAFE_CONTROL_NVRAM_WRITE_PROTECT_SET(gcr, 0);
	}
	KOTINOS_SET_VAL(glass_safe_control, gcr);
	gcr = KOTINOS_GET_VAL(glass_safe_control);
	if (on) {
		return KOTINOS_RE_REGS_GLASS_SAFE_CONTROL_NVRAM_WRITE_PROTECT_GET(gcr);
	} else {
		return ! KOTINOS_RE_REGS_GLASS_SAFE_CONTROL_NVRAM_WRITE_PROTECT_GET(gcr);
	}
}

/*
 * Control the TPM PP pin
 *
 * input
 *	0 - Normal operation
 *	1 - Enable some special commands
 *
 * return
 *	1 - If the TPM PP pin was set to the desired value
 *	0 - If not.
 */
int
syspld_tpm_pp_control(int set)
{
	uint8_t gcr;

	gcr = KOTINOS_GET_VAL(glass_safe_control);
	if (set) {
		gcr = KOTINOS_RE_REGS_GLASS_SAFE_CONTROL_TPM_PP_PIN_CONTROL_SET(gcr, 1);
	} else {
		gcr = KOTINOS_RE_REGS_GLASS_SAFE_CONTROL_TPM_PP_PIN_CONTROL_SET(gcr, 0);
	}
	KOTINOS_SET_VAL(glass_safe_control, gcr);
	gcr = KOTINOS_GET_VAL(glass_safe_control);
	if (set) {
		return KOTINOS_RE_REGS_GLASS_SAFE_CONTROL_TPM_PP_PIN_CONTROL_GET(gcr);
	} else {
		return ! KOTINOS_RE_REGS_GLASS_SAFE_CONTROL_TPM_PP_PIN_CONTROL_GET(gcr);
	}
}

/*
 * Enable/Disable the syspld cpld update via GPIO
 *
 * input
 *	1 - Disable the cpld update
 *	0 - Enable the cpld update
 *
 * return
 *	1 - If the required cpld update action was successful
 *	0 - if not.
 */
int
syspld_cpld_update(int disable)
{
	uint8_t gcr;

	gcr = KOTINOS_GET_VAL(glass_safe_control);
	if (disable) {
		gcr = KOTINOS_RE_REGS_GLASS_SAFE_CONTROL_BOOTCPLD_UPDATE_SET(gcr, 1);
	} else {
		gcr = KOTINOS_RE_REGS_GLASS_SAFE_CONTROL_BOOTCPLD_UPDATE_SET(gcr, 0);
	}
	KOTINOS_SET_VAL(glass_safe_control, gcr);
	gcr = KOTINOS_GET_VAL(glass_safe_control);
	if (disable) {
		return KOTINOS_RE_REGS_GLASS_SAFE_CONTROL_TPM_PP_PIN_CONTROL_GET(gcr);
	} else {
		return ! KOTINOS_RE_REGS_GLASS_SAFE_CONTROL_TPM_PP_PIN_CONTROL_GET(gcr);
	}
}

/*
 * Return the state of external push button on kotinos chassis
 *
 * return
 *	1 - if the push button is pressed
 *	0 - if the push button is not pressed
 */
int
syspld_push_button_detect(void)
{
	uint8_t misc;

	misc = KOTINOS_GET_VAL(misc);
	
	return KOTINOS_RE_REGS_MISC_EXT_PUSH_BUTTON_DETECT_GET(misc);
}

/*
 * Return the state of the jumper on the kotinos chassis
 *
 * return
 *	1 - if the jumper is attached
 *	0 - if the jumper is not attached
 */
int
syspld_jumper_detect(void)
{
	uint8_t	misc;

	misc = KOTINOS_GET_VAL(misc);

	return KOTINOS_RE_REGS_MISC_JUMPER_DETECT_GET(misc);
}

/*
 * Asserts the CPU only reset bit in the syspld
 *
 * return
 *	this function should not return
 */
void
syspld_assert_sreset(void)
{
	uint8_t val;

	val = KOTINOS_GET_VAL(cpu_only_reset);
	val = KOTINOS_RE_REGS_CPU_ONLY_RESET_CPU_ONLY_RESET_SET(val, 1);
	/* this should cause a reset and the function will not return */
	KOTINOS_SET_VAL(cpu_only_reset, val);

	/* not going to happen */
	return;
}

/*
 * Get the status of the CPU only reset bit in syspld
 *
 * return
 *	1 - if the bit is set
 *	0 - if the bit is not set
 */
int
syspld_cpu_only_reset(void)
{
	uint8_t val;

	val = KOTINOS_GET_VAL(cpu_only_reset);

	return KOTINOS_RE_REGS_CPU_ONLY_RESET_CPU_ONLY_RESET_GET(val);
}

/*
 * Controls the console/aux UART receive path
 *
 * input
 *	1 - enable RX to the UART
 *	0 - disable RX to the UART
 *
 * return
 *	1 - if the asked operation was successful
 *	0 - if not
 */
int
syspld_uart_rx_control(int enable)
{
	uint8_t uart_ctl;

	uart_ctl = KOTINOS_GET_VAL(uart0_tx_rx_enable);
	if (enable) {
		uart_ctl = KOTINOS_RE_REGS_UART0_TX_RX_ENABLE_UART0_RX_ENABLE_SET(uart_ctl, 1);
	} else {
		uart_ctl = KOTINOS_RE_REGS_UART0_TX_RX_ENABLE_UART0_RX_ENABLE_SET(uart_ctl, 0);
	}
	KOTINOS_SET_VAL(uart0_tx_rx_enable, uart_ctl);
	uart_ctl = KOTINOS_GET_VAL(uart0_tx_rx_enable);
	if (enable) {
		return KOTINOS_RE_REGS_UART0_TX_RX_ENABLE_UART0_RX_ENABLE_GET(uart_ctl);
	} else {
		return ! KOTINOS_RE_REGS_UART0_TX_RX_ENABLE_UART0_RX_ENABLE_GET(uart_ctl);
	}
}

/*
 * Controls the console/aux UART transmit path
 *
 * input
 *	1 - enable TX to the UART
 *	0 - disable TX to the UART
 *
 * return
 *	1 - if the asked operation was successful
 *	0 - if not
 */
int
syspld_uart_tx_control(int enable)
{
	uint8_t uart_ctl;

	uart_ctl = KOTINOS_GET_VAL(uart0_tx_rx_enable);

	if (enable) {
		uart_ctl = KOTINOS_RE_REGS_UART0_TX_RX_ENABLE_UART0_TX_ENABLE_SET(uart_ctl, 1);
	} else {
		uart_ctl = KOTINOS_RE_REGS_UART0_TX_RX_ENABLE_UART0_TX_ENABLE_SET(uart_ctl, 0);
	}
	KOTINOS_SET_VAL(uart0_tx_rx_enable, uart_ctl);
	uart_ctl = KOTINOS_GET_VAL(uart0_tx_rx_enable);
	if (enable) {
		return KOTINOS_RE_REGS_UART0_TX_RX_ENABLE_UART0_TX_ENABLE_GET(uart_ctl);
	} else {
		return ! KOTINOS_RE_REGS_UART0_TX_RX_ENABLE_UART0_TX_ENABLE_GET(uart_ctl);
	}
}

/*
 * Get the status of the UART rx control
 *
 * return
 *	1 - if the RX is enabled
 *	0 - if it is disabled
 */
int
syspld_uart_get_rx_status(void)
{
	uint8_t uart_ctl;

	uart_ctl = KOTINOS_GET_VAL(uart0_tx_rx_enable);
	
	return KOTINOS_RE_REGS_UART0_TX_RX_ENABLE_UART0_RX_ENABLE_GET(uart_ctl);
}

/*
 * Get the status of the UART tx control
 *
 * return
 *	1 - if the TX is enabled
 *	0 - if it is disabled
 */
int
syspld_uart_get_tx_status(void)
{
	uint8_t uart_ctl;

	uart_ctl = KOTINOS_GET_VAL(uart0_tx_rx_enable);
	
	return KOTINOS_RE_REGS_UART0_TX_RX_ENABLE_UART0_TX_ENABLE_GET(uart_ctl);
}


/*
 * Get status of Ready bit in LPC CSR register.
 *
 * if set	- TPM is busy
 * if clear	- TPM is ready
 */
int
syspld_chk_tpm_busy(void)
{
    uint8_t lpc_csr_reg_val;

    lpc_csr_reg_val = KOTINOS_GET_VAL(lpc_ctrl_status);

    return (KOTINOS_RE_REGS_LPC_CTRL_STATUS_READY_GET(lpc_csr_reg_val));
}

/*
 * Set Address[7:0] to transfer to TPM
 *
 */
void
syspld_set_lpc_addr_byte1(uint8_t addr_val)
{
    KOTINOS_SET_VAL(lpc_address_byte1, addr_val);
}

/*
 * Set Address[15:8] to transfer to TPM
 *
 */
void
syspld_set_lpc_addr_byte2(uint8_t addr_val)
{
    KOTINOS_SET_VAL(lpc_address_byte2, addr_val);
}

/*
 * Start a Memory Read Transaction
 *  - Set memory read cycle type - 1 for memory read
 *  - Set start transaction bit
 *
 */
void
syspld_start_lpc_mem_read_trans(void)
{
    uint8_t lpc_csr_reg_val;

    lpc_csr_reg_val = KOTINOS_GET_VAL(lpc_ctrl_status);

    /* Set Memory Read cycle type */
    lpc_csr_reg_val = 
	KOTINOS_RE_REGS_LPC_CTRL_STATUS_LPC_CYCLE_TYPE_SET(lpc_csr_reg_val, 1);

    /* Start transaction */
    lpc_csr_reg_val = 
	KOTINOS_RE_REGS_LPC_CTRL_STATUS_LPC_TRANSFER_START_SET(lpc_csr_reg_val,
							       1);
    KOTINOS_SET_VAL(lpc_ctrl_status, lpc_csr_reg_val);
}

/*
 * Start a Memory Write Transaction
 * - Set memory write cycle type - 0 for memory write
 * - Set start transaction bit
 *
 */
void
syspld_start_lpc_mem_write_trans(void)
{
    uint8_t lpc_csr_reg_val;

    lpc_csr_reg_val = KOTINOS_GET_VAL(lpc_ctrl_status);

    /* Set Memory Write cycle type */
    lpc_csr_reg_val =
	KOTINOS_RE_REGS_LPC_CTRL_STATUS_LPC_CYCLE_TYPE_SET(lpc_csr_reg_val, 0);

    /* Start transaction */
    lpc_csr_reg_val =
	KOTINOS_RE_REGS_LPC_CTRL_STATUS_LPC_TRANSFER_START_SET(lpc_csr_reg_val,
							       1);
    KOTINOS_SET_VAL(lpc_ctrl_status, lpc_csr_reg_val);
}


/*
 * Check LPC transaction result
 *
 *  00 - Success
 *  01 - Additional Wait States by Peripheral
 *  02 - Error
 */
int
syspld_get_trans_result(void)
{
    uint8_t lpc_csr_reg_val;

    lpc_csr_reg_val = KOTINOS_GET_VAL(lpc_ctrl_status);

    return (KOTINOS_RE_REGS_LPC_CTRL_STATUS_LPC_TRANSACTION_RESULT_GET(lpc_csr_reg_val));
}

/*
 * Get LPC Data In register value which is read from
 * peripheral
 *
 */
uint8_t
syspld_get_lpc_din(void)
{
    uint8_t tpm_din_val;

    tpm_din_val = KOTINOS_GET_VAL(lpc_data_in);

    return tpm_din_val;
}

/*
 * Set LPC Data Out register value to transmit to 
 * peripheral
 *
 */
void
syspld_set_lpc_dout(uint8_t data_out)
{
    KOTINOS_SET_VAL(lpc_data_out, data_out);
}



int
syspld_get_logical_bank(void)
{
    int val = ACX_VARIANT_GET_VAL(flash_swizzle_ctrl);
    
    if (ACX_VARIANT_IS_BIT_SET(val, RE_REGS_FLASH_SWIZZLE_CTRL_ADDR_BIT_A23_MSB))
	return 1;
    else
	return 0;
}

void
syspld_set_logical_bank(uint8_t bank)
{
    if (bank)
	ACX_VARIANT_SET_BIT(flash_swizzle_ctrl, RE_REGS_FLASH_SWIZZLE_CTRL_ADDR_BIT_A23_MSB);
    else
	ACX_VARIANT_CLEAR_BIT(flash_swizzle_ctrl, RE_REGS_FLASH_SWIZZLE_CTRL_ADDR_BIT_A23_MSB);
}

void
syspld_self_reset(void)
{
        ACX_VARIANT_CLEAR_BIT(main_reset_cntrl, RE_REGS_MAIN_RESET_CNTRL_CPLD_SELF_RESET_MSB);
}

void
syspld_cpu_reset(void)
{
    ACX_VARIANT_CLEAR_BIT(main_reset_cntrl, RE_REGS_MAIN_RESET_CNTRL_CPLD_CPU_HRESET_MSB);
}

void
syspld_watchdog_disable(void)
{
    ACX_VARIANT_CLEAR_BIT(core_watchdog_timer_cntrl, RE_REGS_CORE_WATCHDOG_TIMER_CNTRL_TIMER_EN_MSB);
}

void
syspld_watchdog_enable(void)
{
    ACX_VARIANT_SET_BIT(core_watchdog_timer_cntrl, RE_REGS_CORE_WATCHDOG_TIMER_CNTRL_TIMER_EN_MSB);
}

void
syspld_watchdog_tickle(void)
{
    ACX_VARIANT_CLEAR_BIT(core_watchdog_timer_cntrl, RE_REGS_CORE_WATCHDOG_TIMER_CNTRL_TIMER_RESTART_MSB);
    ACX_VARIANT_SET_BIT(core_watchdog_timer_cntrl, RE_REGS_CORE_WATCHDOG_TIMER_CNTRL_TIMER_RESTART_MSB);
}

uint8_t
syspld_get_wdt_timeout(void)
{
    return (ACX_VARIANT_GET_VAL(core_watchdog_timer_count) + 2) / 3;
}

void
syspld_set_wdt_timeout(uint8_t secs)
{
    ACX_VARIANT_SET_VAL(core_watchdog_timer_count, secs * 3);
}

void
syspld_swizzle_enable(void)
{
    ACX_VARIANT_SET_BIT(flash_swizzle_ctrl, RE_REGS_FLASH_SWIZZLE_CTRL_FLASH_AUTO_SWIZZLE_ENA_MSB);
}

void
syspld_swizzle_disable(void)
{
    ACX_VARIANT_CLEAR_BIT(flash_swizzle_ctrl, RE_REGS_FLASH_SWIZZLE_CTRL_FLASH_AUTO_SWIZZLE_ENA_MSB);
}

uint8_t
syspld_get_reboot_reason(void)
{
    uint8_t reason = ~ACX_VARIANT_GET_VAL(main_reset_reason); /* register values are active low */
    uint8_t retval;
    
    if (ACX_VARIANT_IS_BIT_SET(reason, RE_REGS_MAIN_RESET_REASON_HRESET_REQ_STATUS_MSB)) {
	retval = RESET_REASON_HW_INITIATED;
    } else if (ACX_VARIANT_IS_BIT_SET(reason, RE_REGS_MAIN_RESET_REASON_CPLD_DRIVEN_HRESET_ON_SWIZZLE_MSB)) {
	retval = RESET_REASON_SWIZZLE;
    } else if (ACX_VARIANT_IS_BIT_SET(reason, RE_REGS_MAIN_RESET_REASON_WATCHDOG_HRESET_MSB)) {
	retval = RESET_REASON_WATCHDOG;
    } else if (ACX_VARIANT_IS_BIT_SET(reason, RE_REGS_MAIN_RESET_REASON_HRESET_CPLDBIT_SET_MSB) ||
	       ACX_VARIANT_IS_BIT_SET(reason, RE_REGS_MAIN_RESET_REASON_CPLD_SELF_RESET_DRIVEN_MSB)) {
	retval = RESET_REASON_SW_INITIATED;
    } else {
	retval = RESET_REASON_POWERCYCLE;
    }

    /*
     * Just read and return reset reason values and
     * don't clear register bits as this is iCRT.
     * U-Boot will use these bits again to know the reset
     * reason and clear the bits.
     */
    return retval;
}

void
syspld_reset_usb_phy(void)
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

static
void syspld_reset_ddr(uint8_t clear_bit)
{
    uint8_t reg_value;
    uint8_t ext_device_reset_cntrl_reg = 0x14;

    /*
     * Only on Fortius F/G and variants of these boards have this bit.
     */
    if ((gd->board_type == I2C_ID_ACX2000) ||
	(gd->board_type == I2C_ID_ACX1000)) { 
	if (clear_bit) {
	    /* Clear Bit */
	    reg_value = syspld_reg_read(ext_device_reset_cntrl_reg);
	    reg_value &= (~(1 << RE_REGS_EXT_DEVICE_RESET_CNTRL_CPU_DDR_RESET_MSB));
	    syspld_reg_write(ext_device_reset_cntrl_reg, reg_value);
	}
	else {
	    /* Set Bit */
	    reg_value = syspld_reg_read(ext_device_reset_cntrl_reg);
	    reg_value |= (1 << RE_REGS_EXT_DEVICE_RESET_CNTRL_CPU_DDR_RESET_MSB);
	    syspld_reg_write(ext_device_reset_cntrl_reg, reg_value);
	}
    }
}

static
void syspld_reset_hw_ic_monitor(uint8_t clear_bit)
{
    uint8_t reg_value;
    uint8_t ext_device_reset_cntrl_reg = 0x14;

    /*
     * Only on Fortius M, this bit control is there.
     */
    if (gd->board_type == I2C_ID_ACX4000) {
	if (clear_bit) {
	    /* Clear Bit */
	    reg_value = syspld_reg_read(ext_device_reset_cntrl_reg);
	    reg_value &= (~(1 << FORTIUS_M_RE_REGS_EXT_DEVICE_RESET_CNTRL_HW_MONITOR_IC_RESET_MSB));
	    syspld_reg_write(ext_device_reset_cntrl_reg, reg_value);
	}
	else {
	    /* Set Bit */
	    reg_value = syspld_reg_read(ext_device_reset_cntrl_reg);
	    reg_value |= (1 << FORTIUS_M_RE_REGS_EXT_DEVICE_RESET_CNTRL_HW_MONITOR_IC_RESET_MSB);
	    syspld_reg_write(ext_device_reset_cntrl_reg, reg_value);
	}
    }
}

void
syspld_init(void)
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

    /*
     * On Fortius M boards, ext_device_reset_cntrl(0x14) register bit 0 is used
     * for resetting the HW_MONITOR_IC
     * (FORTIUS_M_RE_REGS_EXT_DEVICE_RESET_CNTRL_HW_MONITOR_IC_RESET_MSB)
     * whereas on Fortius F and G boards this bit is used for resetting the DDR
     * (RE_REGS_EXT_DEVICE_RESET_CNTRL_CPU_DDR_RESET_MSB)
     *
     * eventually on Fortius F & G boards, this bit will be removed, and either
     * left un-used or used for resetting HW_MONITOR_IC, at that time common
     * macros can be used for setting the same bit.
     *
     */
    syspld_reset_ddr(1);               /* Clear the DDR Reset bit */
    syspld_reset_hw_ic_monitor(1);     /* Clear the HW IC Monitor reset bit */

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

    syspld_reset_ddr(0);           /* Set the DDR Reset bit */
    syspld_reset_hw_ic_monitor(0);  /* Set the HW IC Monitor reset bit */

    ACX_VARIANT_SET_BIT(ext_device_reset_cntrl, 
			  RE_REGS_EXT_DEVICE_RESET_CNTRL_USB_SW_RESET_MSB);
    ACX_VARIANT_SET_BIT(ext_device_reset_cntrl, 
			  RE_REGS_EXT_DEVICE_RESET_CNTRL_USB_FLASH_CTRL_RESET_MSB);

    /* This is enough, since we have a greater delay post USB PHY reset */
    udelay(5000); /* 5 ms */ 
}


void
syspld_reg_dump_all(void)
{
    int i;

    for (i=0; i < sizeof(fortius_syspld); i++)
		printf("syspld[0x%x] = 0x%x\n", i, syspld_reg_read(i));
}

/***************************************************/

