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

#include <config.h>
#include <common.h>
#include <command.h>
#include <i2c.h>
#include "duart.h"

#ifdef CFG_16C752B

DECLARE_GLOBAL_DATA_PTR; /* for using gd->baudrate */
static int calc_divisor(DUART_16C752B_t port);

static DUART_16C752B_t serial_ports[4] = {
#ifdef CFG_16C752B_COM1
	(DUART_16C752B_t) CFG_16C752B_COM1,
#else
	NULL,
#endif
#ifdef CFG_16C752B_COM2
	(DUART_16C752B_t) CFG_16C752B_COM2,
#else
	NULL,
#endif
#ifdef CFG_16C752B_COM3
	(DUART_16C752B_t) CFG_16C752B_COM3,
#else
	NULL,
#endif
#ifdef CFG_16C752B_COM4
	(DUART_16C752B_t) CFG_16C752B_COM4,
#else
	NULL,
#endif
};

#define PORT      serial_ports[port - 1]

#define MCRVAL    (MCR_DTR | MCR_RTS)                   /* RTS/DTR */
#define FCRVAL    (FCR_FIFO_EN | FCR_RXSR | FCR_TXSR)   /* Clear & enable FIFOs */

/* Initialize duart */
int duart_init(void)
{
	if (!EX82XX_RECPU) {
		return 0;
	}
	int clock_divisor;

#ifdef CFG_16C752B_COM1
	clock_divisor = calc_divisor(serial_ports[0]);
	D16C752B_init(serial_ports[0], clock_divisor);
#endif
#ifdef CFG_16C752B_COM2
	clock_divisor = calc_divisor(serial_ports[1]);
	D16C752B_init(serial_ports[1], clock_divisor);
#endif

	return 0;
}


/* Initialize duart to internal loopback mode */
int duart_init_internal(void)
{
	if (!EX82XX_RECPU) {
		return 0;
	}
	int clock_divisor;

#ifdef CFG_16C752B_COM1
	clock_divisor = calc_divisor(serial_ports[0]);
	D16C752B_init_internal(serial_ports[0], clock_divisor);
#endif
#ifdef CFG_16C752B_COM2
	clock_divisor = calc_divisor(serial_ports[1]);
	D16C752B_init_internal(serial_ports[1], clock_divisor);
#endif

	return 0;
}


/* This function programs the 16C752C DUART registers */
void D16C752B_init(DUART_16C752B_t com_port, int baud_divisor)
{
	com_port->ier = 0x00;
#ifdef CONFIG_OMAP
	com_port->mdr1 = 0x7;   /* mode select reset TL16C750*/
#endif
	com_port->lcr = LCR_BKSE | LCR_8N1;
	com_port->dll = 10;     /* baud_divisor & 0xff */
	com_port->dlh = 0x00;   /* (baud_divisor >> 8) & 0xff */
	com_port->lcr = LCR_8N1;
	com_port->mcr = MCRVAL;
	com_port->fcr = FCRVAL;

#if defined (CONFIG_OMAP)
#if defined (CONFIG_APTIX)
	com_port->mdr1 = 3;     /* /13 mode so Aptix 6MHz can hit 115200 */
#else
	com_port->mdr1 = 0;     /* /16 is proper to hit 115200 with 48MHz */
#endif
#endif
}


/* Initializes the duart in internal loopback mode */
void D16C752B_init_internal(DUART_16C752B_t com_port, int baud_divisor)
{
	com_port->ier = 0x00;
#ifdef CONFIG_OMAP
	com_port->mdr1 = 0x7;   /* mode select reset TL16C750*/
#endif

	com_port->lcr = LCR_BKSE | LCR_8N1;
	com_port->dll = baud_divisor & 0xff;
	com_port->dlh = (baud_divisor >> 8) & 0xff;
	com_port->lcr = LCR_8N1;
	com_port->mcr = MCR_LOOP_INTERNAL | MCRVAL;
	com_port->fcr = FCRVAL;

#if defined (CONFIG_OMAP)
#if defined (CONFIG_APTIX)
	com_port->mdr1 = 3;     /* /13 mode so Aptix 6MHz can hit 115200 */
#else
	com_port->mdr1 = 0;     /* /16 is proper to hit 115200 with 48MHz */
#endif
#endif
}


/* Calculates the clock divisor value for baud. */
static int calc_divisor(DUART_16C752B_t port)
{
	return CFG_NS16550_CLK / MODE_X_DIV / gd->baudrate;
}


void D16C752B_reinit(DUART_16C752B_t com_port, int baud_divisor)
{
	com_port->ier = 0x00;
	com_port->lcr = LCR_BKSE;
	com_port->dll = baud_divisor & 0xff;
	com_port->dlh = (baud_divisor >> 8) & 0xff;
	com_port->lcr = LCR_8N1;
	com_port->mcr = MCRVAL;
	com_port->fcr = FCRVAL;
}


void D16C752B_setbrg(const int port)
{
	int clock_divisor;

	clock_divisor = calc_divisor(PORT);
	D16C752B_reinit(PORT, clock_divisor);
}


/* puts a character on the serial */
void D16C752B_duart_putc(DUART_16C752B_t com_port, char c)
{
	while ((com_port->lsr & LSR_THRE) == 0);
	com_port->thr = c;
}


/* gets a character from the serial */
char D16C752B_duart_getc(DUART_16C752B_t com_port)
{
	while ((com_port->lsr & LSR_DR) == 0);
	return com_port->rhr;
}


int D16C752B_tstc(DUART_16C752B_t com_port)
{
	unsigned char status = ((com_port->lsr & LSR_DR) != 0);

	return status;
}


void duart_putc(unsigned int com_port, char c)
{
	if (c == '\n') {
		D16C752B_duart_putc(serial_ports[com_port - 1], '\r');
	}
	D16C752B_duart_putc(serial_ports[com_port - 1], c);
}


char duart_getc(unsigned int com_port)
{
	return D16C752B_duart_getc(serial_ports[com_port - 1]);
}


void duart_puts(const char* s)
{
	while (*s) {
		duart_putc(CONFIG_CONS_INDEX, *s++);
	}
}


void duart_setbrg(void)
{
	D16C752B_setbrg(CONFIG_CONS_INDEX);
}


int duart_tstc(unsigned int com_port)
{
	return D16C752B_tstc(serial_ports[com_port - 1]);
}


#endif  /* CFG_16C752B */

