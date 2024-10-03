/***********************license start************************************
 * Copyright (c) 2003-2007  Cavium Networks (support@cavium.com). 
 * All rights reserved.
 * 
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 * 
 *     * Neither the name of Cavium Networks nor the names of
 *       its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.  
 * 
 * This Software, including technical data, may be subject to U.S.  export 
 * control laws, including the U.S.  Export Administration Act and its 
 * associated regulations, and may be subject to export or import regulations 
 * in other countries.  You warrant that You will comply strictly in all 
 * respects with all such regulations and acknowledge that you have the 
 * responsibility to obtain licenses to export, re-export or import the 
 * Software.  
 * 
 * TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS" 
 * AND WITH ALL FAULTS AND CAVIUM NETWORKS MAKES NO PROMISES, REPRESENTATIONS 
 * OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH 
 * RESPECT TO THE SOFTWARE, INCLUDING ITS CONDITION, ITS CONFORMITY TO ANY 
 * REPRESENTATION OR DESCRIPTION, OR THE EXISTENCE OF ANY LATENT OR PATENT 
 * DEFECTS, AND CAVIUM SPECIFICALLY DISCLAIMS ALL IMPLIED (IF ANY) WARRANTIES 
 * OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A PARTICULAR 
 * PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET 
 * POSSESSION OR CORRESPONDENCE TO DESCRIPTION.  THE ENTIRE RISK ARISING OUT 
 * OF USE OR PERFORMANCE OF THE SOFTWARE LIES WITH YOU.
 * 
 ***********************license end**************************************/

#include <config.h>

#ifdef CONFIG_OCTEON

#include <common.h>
#include <asm/au1x00.h>
#include "octeon_boot.h"
#include "octeon_serial.h"
#include "octeon-pci-console.h"

/******************************************************************************
*
* serial_init - initialize a channel
*
* This routine initializes the number of data bits, parity
* and set the selected baud rate. Interrupts are disabled.
* Set the modem control signals if the option is selected.
*
* RETURNS: N/A
*/


uint16_t compute_divisor(uint32_t eclock, uint32_t baud)
{
    uint16_t divisor;
#if CONFIG_OCTEON_SIM
    divisor = 1; /* We're simulating, go fast! */
#else
    divisor = ((unsigned long)(eclock + 8 * baud) / (unsigned long)(16 * baud));
#endif

    return(divisor);
}


#define LOG_REGS    \
    asm volatile ("\
        .set push\n\
        .set mips64\n\
        dadd $1, $0, $2\n\
        dadd $1, $0, $3\n\
        dadd $1, $0, $4\n\
        dadd $1, $0, $5\n\
        dadd $1, $0, $6\n\
        dadd $1, $0, $7\n\
        dadd $1, $0, $8\n\
        dadd $1, $0, $9\n\
        dadd $1, $0, $10\n\
        dadd $1, $0, $11\n\
        dadd $1, $0, $12\n\
        dadd $1, $0, $13\n\
        dadd $1, $0, $14\n\
        dadd $1, $0, $15\n\
        dadd $1, $0, $16\n\
        dadd $1, $0, $17\n\
        dadd $1, $0, $18\n\
        dadd $1, $0, $19\n\
        dadd $1, $0, $20\n\
        dadd $1, $0, $21\n\
        dadd $1, $0, $22\n\
        dadd $1, $0, $23\n\
        dadd $1, $0, $24\n\
        dadd $1, $0, $25\n\
        dadd $1, $0, $26\n\
        dadd $1, $0, $27\n\
        dadd $1, $0, $28\n\
        dadd $1, $0, $29\n\
        dadd $1, $0, $30\n\
        dadd $1, $0, $31\n\
        .set pop\n\
        " : : );
void simprintf(const char *format, ...){
    OCTEON_SYNC;
    asm volatile ("\
	.set push \n\
        .set mips64 \n\
        dadd $25, $0, 6\n\
	lui  $15, 0x8000\n\
	ori  $15, $15,0xf\n\
	dsll $15, $15, 0x10\n\
	ori  $15, $15, 0xeffe\n\
	dsll $15, $15, 0x10\n\
        dadd $24, $31, $0\n\
        jalr $15\n\
        dadd $31, $24, $0\n\
        .set pop \n\
        " : : );
}



/////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Setup a uart for use
 *
 * @param uart_index Uart to setup (0 or 1)
 * @return Zero on success
 */
int octeon_uart_setup(int uart_index)
{
    DECLARE_GLOBAL_DATA_PTR;
    uint16_t divisor;
    cvmx_uart_fcr_t fcrval;
    cvmx_uart_mcr_t mcrval;
    cvmx_uart_lcr_t lcrval;

    fcrval.u64 = 0;
    fcrval.s.en = 1;    /* enable the FIFO's */
    fcrval.s.rxfr = 1;  /* reset the RX fifo */
    fcrval.s.txfr = 1;  /* reset the TX fifo */

    /*
     * FIXME MERGE FROM TOT: This needs to be revisited as code from TOT
     * needs to be understood. Retaining the code from the MAGNI branch for
     * now as TOT change will break MAGNI u-boot.
     */
    divisor = compute_divisor(gd->sys_clock_mhz*1000000, gd->baudrate);
#if 0 /* FIXME: Code from TOT.*/
    if (FPC_UART_PORT == uart_index)
        divisor = compute_divisor(gd->cpu_clock_mhz*1000000, CONFIG_FPC_CONSOLE_BAUD);
    else
        divisor = compute_divisor(gd->cpu_clock_mhz*1000000, gd->baudrate);
#endif
    cvmx_write_csr(OCTEON_MIO_UARTX_FCR(uart_index), fcrval.u64);

    mcrval.u64 = 0;
#if CONFIG_OCTEON_SIM
    if (uart_index == 1)
      mcrval.s.afce = 1;  /* enable auto flow control for simulator. Needed for  gdb regression callfuncs.exp. */
    else
      mcrval.s.afce = 0;  /* disable auto flow control so board can power on without serial port connected */
#else
    mcrval.s.afce = 0;  /* disable auto flow control so board can power on without serial port connected */
#endif
    mcrval.s.rts = 1;   /* looks like this must be set for auto flow control to work */

    cvmx_read_csr(OCTEON_MIO_UARTX_LSR(uart_index));

    lcrval.u64 = 0;
    lcrval.s.cls = CVMX_UART_BITS8;
    lcrval.s.stop = 0;  /* stop bit included? */
    lcrval.s.pen = 0;   /* no parity? */
    lcrval.s.eps = 1;   /* even parity? */
    lcrval.s.dlab = 1;  /* temporary to program the divisor */
    cvmx_write_csr(OCTEON_MIO_UARTX_LCR(uart_index), lcrval.u64);

    cvmx_write_csr(OCTEON_MIO_UARTX_DLL(uart_index), divisor & 0xff);
    cvmx_write_csr(OCTEON_MIO_UARTX_DLH(uart_index), (divisor>>8) & 0xff);

    lcrval.s.dlab = 0;  /* divisor is programmed now, set this back to normal */
    cvmx_write_csr(OCTEON_MIO_UARTX_LCR(uart_index), lcrval.u64);

    /* spec says need to wait after you program the divisor */
    uint64_t read_cycle = octeon_get_cycles() + (2 * divisor * 16) + 10000;
    while (octeon_get_cycles() < read_cycle)
    {
        /* Spin */
    }

    /* Don't enable flow control until after baud rate is configured. - we don't want
    ** to allow characters in until after the baud rate is fully configured */
    cvmx_write_csr(OCTEON_MIO_UARTX_MCR(uart_index), mcrval.u64);
    return 0;
}

void octeon_set_baud(uint32_t uart_index, uint32_t baud)
{
    DECLARE_GLOBAL_DATA_PTR;
    uint16_t divisor;
    divisor = compute_divisor(gd->cpu_clock_mhz*1000000, baud);

    cvmx_uart_lcr_t lcrval;
    lcrval.u64 = cvmx_read_csr(OCTEON_MIO_UARTX_LCR(uart_index));

    cvmx_wait((2 * divisor * 16) + 10000);

    lcrval.s.dlab = 1;  /* temporary to program the divisor */
    cvmx_write_csr(OCTEON_MIO_UARTX_LCR(uart_index), lcrval.u64);

    cvmx_write_csr(OCTEON_MIO_UARTX_DLL(uart_index), divisor & 0xff);
    cvmx_write_csr(OCTEON_MIO_UARTX_DLH(uart_index), (divisor>>8) & 0xff);

    lcrval.s.dlab = 0;  /* divisor is programmed now, set this back to normal */
    cvmx_write_csr(OCTEON_MIO_UARTX_LCR(uart_index), lcrval.u64);

    /* spec says need to wait after you program the divisor */
    cvmx_wait((2 * divisor * 16) + 10000);
    /* Wait a little longer..... */
    udelay(500000);

}


/**
 * Put a single byte to uart port.
 *
 * @param uart_index Uart to write to (0 or 1)
 * @param ch         Byte to write
 */
static inline void uart_write_byte(int uart_index, uint8_t ch)
{
    cvmx_uart_lsr_t lsrval;

    /* Spin until there is room */
    do
    {
        lsrval.u64 = cvmx_read_csr(OCTEON_MIO_UARTX_LSR(uart_index));
    }
    while (lsrval.s.thre == 0);

    /* Write the byte */
    cvmx_write_csr(OCTEON_MIO_UARTX_THR(uart_index), ch);
}


/**
 * Get a single byte from serial port.
 *
 * @param uart_index Uart to read from (0 or 1)
 * @return The byte read
 */
static inline uint8_t uart_read_byte(int uart_index)
{
    cvmx_uart_lsr_t lsrval;

    /* Spin until data is available */
    do
    {
        lsrval.u64 = cvmx_read_csr(OCTEON_MIO_UARTX_LSR(uart_index));
    } while (!lsrval.s.dr);

    /* Read and return the data */
    return cvmx_read_csr(OCTEON_MIO_UARTX_RBR(uart_index));
}

/** non-blocking read for fpc console support in hardware diags */
static inline uint8_t uart_no_block_read_byte(int uart_index)
{
    cvmx_uart_lsr_t lsrval;
    lsrval.u64 = cvmx_read_csr(OCTEON_MIO_UARTX_LSR(uart_index));
    if (!lsrval.s.dr)
        return 0xFF;
    /* Read and return the data */
    return cvmx_read_csr(OCTEON_MIO_UARTX_RBR(uart_index));
}





// modified for Octane.....





int serial_init (void)
{
    DECLARE_GLOBAL_DATA_PTR;
    char *env_str;

    gd->console_uart = 0;
    if ((env_str = getenv("console_uart")))
    {
        if (1 == simple_strtoul (env_str, NULL, 10))
            gd->console_uart = 1;
    }

    octeon_uart_setup(gd->console_uart);
	return 0;
}

/*FPC port initialisation as uboot doesn't configure it by default*/
int uart_init(void *uart_info)
{
    DECLARE_GLOBAL_DATA_PTR;
    octeon_uart_setup(*(int *)uart_info);
    return 0;
}

void serial_setbrg (void)
{
    DECLARE_GLOBAL_DATA_PTR;
    octeon_set_baud(gd->console_uart, gd->baudrate);
}

void serial_putc (const char c)
{
    DECLARE_GLOBAL_DATA_PTR;
#if !CONFIG_OCTEON_SIM
    if (c == '\n') uart_write_byte(gd->console_uart, '\r');
#endif

    uart_write_byte(gd->console_uart, c);
}

void serial_puts (const char *s)
{
	while (*s)
	{
		serial_putc (*s++);
	}
}

int serial_getc (void)
{
    DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_ACX500_SVCS)
    int c;

    /*
     * Once u-boot is up, we need to continuously monitor the
     * cpld to check if there is any image to boot. That cannot
     * be done if getc() blocks.
     */
    if ((c = uart_no_block_read_byte(gd->console_uart)) == 0xff) {
        return 0;
    } else {
        return c;
    }
#endif

    return(uart_read_byte(gd->console_uart));
}

int serial_tstc (void)
{
    DECLARE_GLOBAL_DATA_PTR;
    cvmx_uart_lsr_t lsrval;

    lsrval.u64 = cvmx_read_csr(OCTEON_MIO_UARTX_LSR(gd->console_uart));
    return(lsrval.s.dr);
}

/*Added for fpc  functionality support  in hardware diags*/
void uart_putc(const char c, int uart_index)
{
    DECLARE_GLOBAL_DATA_PTR;
    uart_write_byte(uart_index, c);
}

int uart_getc(int uart_idx) 
{
    DECLARE_GLOBAL_DATA_PTR;
    return(uart_no_block_read_byte(uart_idx));
}


#ifdef CFG_PCI_CONSOLE
/* PCI Console functions.  The main implementation is in octeon-pci-console.[ch] in
** the executive. */
#include "octeon-pci-console.h"
extern uint64_t pci_cons_desc_addr;
void pci_putc (const char c)
{
    char r = '\r';
    if (pci_cons_desc_addr)
    {
        if (c == '\n') octeon_pci_console_write(pci_cons_desc_addr, 0,  &r, 1, 0);
        octeon_pci_console_write(pci_cons_desc_addr, 0,  (char *)&c, 1, 0);
    }
}

void pci_puts (const char *s)
{
	while (*s)
	{
		pci_putc (*s++);
	}
}

int pci_getc (void)
{
    char c;
    octeon_pci_console_read(pci_cons_desc_addr, 0,  &c, 1, 0);
    return c;
}

int pci_tstc (void)
{
    return (0 < octeon_pci_console_read_avail(pci_cons_desc_addr, 0));
}

#endif
#endif /* CONFIG_OCTEON */
