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

#ifndef __DUART_16C752B__
#define __DUART_16C752B__

struct DUART_16C752B {
	volatile unsigned char rhr;     /* A[2]=0, A[1]=0, A[0]=0 */
	volatile unsigned char ier;     /* A[2]=0, A[1]=0, A[0]=1 */
	volatile unsigned char fcr;     /* A[2]=0, A[1]=2, A[0]=0 */
	volatile unsigned char lcr;     /* A[2]=0, A[1]=1, A[0]=1 */
	volatile unsigned char mcr;     /* A[2]=1, A[1]=0, A[0]=0 */
	volatile unsigned char lsr;     /* A[2]=1, A[1]=0, A[0]=1 */
	volatile unsigned char msr;     /* A[2]=1, A[1]=1, A[0]=0 */
	volatile unsigned char scr;     /* A[2]=1, A[1]=1, A[0]=1 */
} __attribute__ ((packed));

#ifdef  thr
#undef  thr
#endif

#ifdef  iir
#undef  iir
#endif

#ifdef  dll
#undef  dll
#endif

#ifdef  dlh
#undef  dlh
#endif

#define thr rhr
#define iir fcr
#define dll rhr
#define dlh ier



typedef volatile struct DUART_16C752B*DUART_16C752B_t;

#define FCR_FIFO_EN     0x01            /* Fifo enable */
#define FCR_RXSR        0x02            /* Receiver soft reset */
#define FCR_TXSR        0x04            /* Transmitter soft reset */

#define MCR_DTR         0x01
#define MCR_RTS         0x02
#define MCR_DMA_EN      0x04
#define MCR_TX_DFR      0x08
#define MCR_LOOP_INTERNAL 0x10          /* UART internal loopback */
#define LCR_PEN         0x08            /* Parity eneble */
#define LCR_EPS         0x10            /* Even Parity Select */
#define LCR_STKP        0x20            /* Stick Parity */
#define LCR_SBRK        0x40            /* Set Break */
#define LCR_BKSE        0x80            /* Bank select enable */
#define LCR_8N1         0x03            /* 8 bit data, 1 stop, no parity */

#define LSR_DR          0x01            /* Data ready */
#define LSR_OE          0x02            /* Overrun */
#define LSR_PE          0x04            /* Parity error */
#define LSR_FE          0x08            /* Framing error */
#define LSR_BI          0x10            /* Break */
#define LSR_THRE        0x20            /* Xmit holding register empty */
#define LSR_TEMT        0x40            /* Xmitter empty */
#define LSR_ERR         0x80            /* Error */

#define MODE_X_DIV  16

/* prototypes */
int  duart_init (void);
int  duart_init_internal (void);
void D16C752B_duart_putc (DUART_16C752B_t com_port, char c);
char D16C752B_duart_getc (DUART_16C752B_t com_port);
void duart_putc (unsigned int com_port, char c);
char duart_getc (unsigned int com_port);
void D16C752B_init (DUART_16C752B_t com_port, int baud_divisor);
void D16C752B_init_internal (DUART_16C752B_t com_port, int baud_divisor);
void duart_puts (const char* s);
void duart_setbrg (void);
void D16C752B_setbrg (const int port);
void D16C752B_reinit (DUART_16C752B_t com_port, int baud_divisor);
int duart_tstc (unsigned int com_port);

#endif /*__DUART_16C752B__ */
