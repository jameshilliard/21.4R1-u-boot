/*
 * Copyright (c) 2009-2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * (C) Copyright 2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

/*
 * UART test
 *
 * The UART listed in ctlr_list array below are tested in
 * the loopback mode.
 * The UART controllers are configured accordingly using uart serial init
 * function and test characters are inputed and compared
 * with the predefined characters in the function test_ctlr.
 * If the inputed characters and predefined characters does not match
 * in the test_ctlr function will return fail.Otherwise will return pass.
 */

#include <common.h>

#ifdef CONFIG_POST

#include <configs/mx80.h>
#include <post.h>
#if CONFIG_POST & CFG_POST_UART
#include <asm-ppc/immap_85xx.h>
#include <ns16550.h>
#include <serial.h>


DECLARE_GLOBAL_DATA_PTR;

#define LCRVAL LCR_8N1					/* 8 data, 1 stop, no parity */
#define MCRVAL (MCR_DTR | MCR_RTS)			/* RTS/DTR */
#define MCR_LOOP_INTERNAL 0x10

#define CTLR_UART             1
#define MODE_X_DIV            16
#define CTRL_LIST_SIZE (sizeof(ctlr_list) / sizeof(ctlr_list[0]))
#define MAX_CHAR_DISP         25
#define MAX_LOOP_DISP_RES     1
#define MAX_LOOP_CHAR_CNT     1200
#define MAX_CTLR              2

static int uart_flag = 0;
static int uart_serial_init(void);
static int  uart_serial_getc_diag(void);
static char ch1[MAX_CHAR_DISP];
static int ctlr_list[][MAX_CTLR] = {{CTLR_UART, 0}};
static int int_loop_flag;

static struct {
	int (*init) (void);
	void (*putc) (char index);
	int (*getc) (void);
} ctlr_proc[2];

#define FCRVAL (FCR_FIFO_EN | FCR_RXSR | FCR_TXSR)	/* Clear & enable FIFOs */

void NS16550_init_internal (NS16550_t com_port, int baud_divisor);
char	NS16550_getc_diag  (NS16550_t com_port);
void	NS16550_init_flush_rx_tx (NS16550_t com_port);

#define PORT	serial_ports[port-1]
static NS16550_t serial_ports[4] = {
#ifdef CFG_NS16550_COM1
	(NS16550_t)CFG_NS16550_COM1,
#else
	NULL,
#endif
#ifdef CFG_NS16550_COM2
	(NS16550_t)CFG_NS16550_COM2,
#else
	NULL,
#endif
#ifdef CFG_NS16550_COM3
	(NS16550_t)CFG_NS16550_COM3,
#else
	NULL,
#endif
#ifdef CFG_NS16550_COM4
	(NS16550_t)CFG_NS16550_COM4
#else
		NULL
#endif
};

static int calc_divisor(NS16550_t port)
{
	return (CFG_NS16550_CLK / MODE_X_DIV / gd->baudrate);
}

static int uart_serial_init_internal(void)
{
	int clock_divisor;
#ifdef CFG_NS16550_COM1
	clock_divisor = calc_divisor(serial_ports[0]);
	NS16550_init_internal(serial_ports[0], clock_divisor);
#endif
#ifdef CFG_NS16550_COM2
	clock_divisor = calc_divisor(serial_ports[1]);
	NS16550_init_internal(serial_ports[1], clock_divisor);
#endif
#ifdef CFG_NS16550_COM3
	clock_divisor = calc_divisor(serial_ports[2]);
	NS16550_init_internal(serial_ports[2], clock_divisor);
#endif
#ifdef CFG_NS16550_COM4
	clock_divisor = calc_divisor(serial_ports[3]);
	NS16550_init_internal(serial_ports[3], clock_divisor);
#endif
	return (0);
}

static int uart_serial_init(void)
{
	int clock_divisor;
#ifdef CFG_NS16550_COM1
	clock_divisor = calc_divisor(serial_ports[0]);
	NS16550_init(serial_ports[0], clock_divisor);
#endif
#ifdef CFG_NS16550_COM2
	clock_divisor = calc_divisor(serial_ports[1]);
	NS16550_init(serial_ports[1], clock_divisor);
#endif
#ifdef CFG_NS16550_COM3
	clock_divisor = calc_divisor(serial_ports[2]);
	NS16550_init(serial_ports[2], clock_divisor);
#endif
#ifdef CFG_NS16550_COM4
	clock_divisor = calc_divisor(serial_ports[3]);
	NS16550_init(serial_ports[3], clock_divisor);
#endif
	return (0);
}

static void x_serial_putc(const char c,const int port)
{
	if (c == '\n') {
		NS16550_putc(PORT,'\r');
		return;
	}
	NS16550_putc(PORT, c);
}

void uart_serial_putc(const char c)
{
	x_serial_putc(c,CONFIG_CONS_INDEX);
}

int x_serial_getc(int port)
{
	return NS16550_getc(PORT);
}

int x_serial_getc_diag(int port)
{
	return NS16550_getc_diag(PORT);
}

int uart_serial_getc(void)
{
	return x_serial_getc(CONFIG_CONS_INDEX);
}

int uart_serial_getc_diag(void)
{
	return x_serial_getc_diag(CONFIG_CONS_INDEX);
}

int uart_post_test(int flags)
{
	int int_res = 0;
	int i       = 0;
	uart_flag   = 0;
	char test_str[]  = "*** UART Test String ***\r";
	char obtained_str[100];

	uart_serial_init_internal();
	/*
	 * Junk character is received before test chararcter
	 * is put on the target board in case of
	 * internal loop back so adding extra getc
	 */
	uart_serial_putc('a');
	uart_serial_getc();
	uart_serial_getc();
	for (i = 0; i < sizeof(test_str) - 1; i++) {
	     uart_serial_putc(test_str[i]);
	     obtained_str[i] = uart_serial_getc();
	     if (test_str[i] == '\n') {
		 if (obtained_str[i] != '\r')
			int_res = -1;
	     } else if (obtained_str[i] != test_str[i]) {
			int_res = -1;
			break;
		}
	}

	uart_serial_init ();
	obtained_str[i+1]='\0';

	udelay(500000);
	post_log("\n\nNS16550 UART has been Initialized in loopback mode\n");
	post_log("Sent string = %s\n", test_str);
	post_log("Received string = %s\n", obtained_str);
	if (int_res == 0) {
		post_log(" ------ Strings matched ------ \n");
		post_log("UART test completed, 1 pass, 0 errors\n");
	} else {
		post_log(" ------ Strings did not match ------ \n");
		post_log("UART test failed, 0 pass, 1 errors\n");
 	}

	udelay(500000);
	NS16550_init_flush_rx_tx(serial_ports[0]);
	return 0;
}

void NS16550_init_internal(NS16550_t com_port, int baud_divisor)
{
	com_port->ier = 0x00;
	com_port->lcr = LCR_BKSE | LCRVAL;
	com_port->dll = baud_divisor & 0xff;
	com_port->dlm = (baud_divisor >> 8) & 0xff;
	com_port->lcr = LCRVAL;
	com_port->mcr = MCR_LOOP_INTERNAL | MCRVAL;
	com_port->fcr = FCRVAL;
}

void NS16550_init_flush_rx_tx(NS16550_t com_port)
{
	com_port->fcr = FCRVAL;
}

char NS16550_getc_diag(NS16550_t com_port)
{
    int i =0;
	for( i =0;   i < 20000;  i++) {
		if ((com_port->lsr & LSR_DR) == 0 ) {
			break;
		}
	}
	return (com_port->rbr);
}
#endif /* CONFIG_POST & CFG_POST_UART */
#endif /* CONFIG_POST */
