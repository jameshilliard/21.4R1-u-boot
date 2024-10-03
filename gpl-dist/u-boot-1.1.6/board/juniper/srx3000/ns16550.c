/*
 * Copyright (c) 2009-2010, Juniper Networks, Inc.
 * All rights reserved.
 * COM1 NS16550 support
 * originally from linux source (arch/ppc/boot/ns16550.c)
 * modified to use CFG_ISA_MEM and new defines
 */

#include <config.h>
#include <common.h>

#ifdef CFG_NS16550

#include <ns16550.h>

#define LCRVAL LCR_8N1					/* 8 data, 1 stop, no parity */
#define MCRVAL (MCR_DTR | MCR_RTS)			/* RTS/DTR */
#define FCRVAL (FCR_FIFO_EN | FCR_RXSR | FCR_TXSR)	/* Clear & enable FIFOs */

DECLARE_GLOBAL_DATA_PTR;

void
ns16550_regdump (NS16550_t com_port)
{
	if (gd->serial_reg_flag) {
		printf("Receiver buffer register     0x%x\n", com_port->rbr);
		printf("Interrupt enable register    0x%x\n", com_port->ier);
		printf("Line control register        0x%x\n", com_port->lcr);
		printf("Line status register         0x%x\n", com_port->lsr);
		printf("Device latch low register    0x%x\n", com_port->dll);
		printf("Device latch high register   0x%x\n", com_port->dlm);
		printf("Modem control reister        0x%x\n", com_port->mcr);
		printf("Modem status reister         0x%x\n", com_port->msr);
		printf("FIFO control register        0x%x\n", com_port->fcr);
		printf("Scratchpad register          0x%x\n", com_port->scr);
		return;
	}
}


void
NS16550_init (NS16550_t com_port, int baud_divisor)
{
	ns16550_regdump (com_port);
	com_port->ier = 0x00;
#ifdef CONFIG_OMAP
	com_port->mdr1 = 0x7;	/* mode select reset TL16C750*/
#endif
	com_port->lcr = LCR_BKSE | LCRVAL;
	com_port->dll = baud_divisor & 0xff;
	com_port->dlm = (baud_divisor >> 8) & 0xff;
	com_port->lcr = LCRVAL;
	com_port->mcr = MCRVAL;
	com_port->fcr = FCRVAL;
#if defined(CONFIG_OMAP)
#if defined(CONFIG_APTIX)
	com_port->mdr1 = 3;	/* /13 mode so Aptix 6MHz can hit 115200 */
#else
	com_port->mdr1 = 0;	/* /16 is proper to hit 115200 with 48MHz */
#endif
#endif
	/* Write and read the scratch pad reg with AA and 55 pattern */
	unsigned char readback;
	com_port->scr = TEST_AA_PAT;
	readback = com_port->scr;
	if (readback == TEST_AA_PAT) {
		com_port->scr = TEST_55_PAT;
		readback = com_port->scr;
		if (readback == TEST_55_PAT) {
			gd->serial_console_flag = 1;
		}
	}
}

void
NS16550_reinit (NS16550_t com_port, int baud_divisor)
{
	com_port->ier = 0x00;
	com_port->lcr = LCR_BKSE;
	com_port->dll = baud_divisor & 0xff;
	com_port->dlm = (baud_divisor >> 8) & 0xff;
	com_port->lcr = LCRVAL;
	com_port->mcr = MCRVAL;
	com_port->fcr = FCRVAL;
}

void
NS16550_putc (NS16550_t com_port, char c)
{
	while ((com_port->lsr & LSR_THRE) == 0);
	com_port->thr = c;
}

char
NS16550_getc (NS16550_t com_port)
{
	while ((com_port->lsr & LSR_DR) == 0) {
#ifdef CONFIG_USB_TTY
		extern void usbtty_poll(void);
		usbtty_poll();
#endif
	}
	return (com_port->rbr);
}

int
NS16550_tstc (NS16550_t com_port)
{
	return ((com_port->lsr & LSR_DR) != 0);
}

#endif
