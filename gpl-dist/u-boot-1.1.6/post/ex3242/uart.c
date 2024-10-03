/*
 * Copyright (c) 2007-2010, Juniper Networks, Inc.
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


#include <common.h>

/*
 * UART test
 *
 * The UART listed in ctlr_list array below are tested in
 * the loopback mode.
 * The UART controllers are configured accordingly using uart serial init
 * function and test characters are inputed and compared with the 
 * with the predefined characters in the function test_ctlr.
 * If the inputed characters and predefined characters does not match 
 * in the test_ctlr function will return fail.Otherwise will return pass.
 */
#include <common.h>

#ifdef CONFIG_POST

#include <post.h>
#if CONFIG_POST & CFG_POST_UART
#include <asm-ppc/immap_85xx.h>
#include <ns16550.h>
#include <serial.h>


DECLARE_GLOBAL_DATA_PTR;

#define LCRVAL LCR_8N1					/* 8 data, 1 stop, no parity */
#define MCRVAL (MCR_DTR | MCR_RTS)			/* RTS/DTR */
#define MCR_LOOP_INTERNAL 0x10

#define CTLR_UART   0
#define MODE_X_DIV 16
#define LCD_LINE_LENGTH 16
#define CTRL_LIST_SIZE (sizeof(ctlr_list) / sizeof(ctlr_list[0]))

static int uart_flag =0;
static int uart_serial_init (void);
static int  uart_serial_getc_diag(void);
static	char ch1[25];
static int ctlr_list[][2] = {{CTLR_UART, 0} };
static int int_loop_flag;

#define FCRVAL (FCR_FIFO_EN | FCR_RXSR | FCR_TXSR)	/* Clear & enable FIFOs */

void NS16550_init_internal (NS16550_t com_port, int baud_divisor);
char	NS16550_getc_diag  (NS16550_t com_port);
void	NS16550_init_flush_rx_tx (NS16550_t com_port);

static struct {
	int (*init) (void);
	void (*putc) (char index);
	int (*getc) (void);
} ctlr_proc[2];

static int test_ctlr (int ctlr, int index)
{
	int res = -1;
	char test_str[]  = "*** UART Test String ***\r";
	int i=0;
	char ch;
	int time;
	for(i=0; i<24; i++) {
		ch1[i] = 0;
	}
	ctlr_proc[ctlr].init ();
	if(int_loop_flag)
	ctlr_proc[ctlr].getc();  /* Junk character is received before test chararcter
								is put on the target board in case of
								internal loop back so adding one extra getc*/

	if(int_loop_flag == 0)
	{

		for(time =0; time < 1200; time++) {
			ctlr_proc[ctlr].putc ('.');
			ch = uart_serial_getc_diag();
			if(ch == '.')
			{
				break;
			}
			udelay(50000);
			udelay(10000);
		}
		if(time == 1200)
		{
			return -1;
		}
	}

	if(int_loop_flag == 0)
		uart_serial_getc_diag(); /** Dot  '.' is displayed , to  eliminate this one
								   dot  extra getc_diag function  is added  **/
	for (i = 0; i < sizeof (test_str) - 1; i++) {
		ctlr_proc[ctlr].putc (test_str[i]);
		ch = ctlr_proc[ctlr].getc();
		ch1[i] = ch;
		if(test_str[i] == '\n') {
			if(ch != '\r')
			{
				goto Done;
			}
		}
		else if (ch != test_str[i]){
			goto Done;
		}

	}
	res = 0;
Done:
	return res;
}

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

static int calc_divisor (NS16550_t port)
{
	return (CFG_NS16550_CLK / MODE_X_DIV / gd->baudrate);
}

static int uart_serial_init_internal (void)
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

static int uart_serial_init (void)
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
int  uart_serial_getc(void)
{
	return x_serial_getc(CONFIG_CONS_INDEX);
}
int  uart_serial_getc_diag(void)
{
	return x_serial_getc_diag(CONFIG_CONS_INDEX);
}

int uart_post_test (int flags)
{
	int int_res = 0;
	int ext_res = 0;
	int i=0;

	uart_flag = 0;

	ctlr_proc[CTLR_UART].init = uart_serial_init_internal;
	ctlr_proc[CTLR_UART].putc = uart_serial_putc;
	ctlr_proc[CTLR_UART].getc = uart_serial_getc;

	if(flags & POST_DEBUG) {
		uart_flag = 1;
	}

	int_loop_flag = 1;
	for (i = 0; i < CTRL_LIST_SIZE; i++) {
		if (test_ctlr (ctlr_list[i][0], ctlr_list[i][1]) != 0) {
			int_res = -1;
		}
	}
	uart_serial_init ();
	ctlr_proc[CTLR_UART].init = uart_serial_init;
	post_log("\n\n");	

	if(uart_flag) {
		post_log(" Prints the internal loopback result \n");
		for(i = 0;i < 25; i++){
			post_log(" i = %d, ch1 = %c ,  ch1 = 0x%x   \n\n",i, ch1[i],ch1[i]); 
		}
	}
	int_loop_flag = 0;
	post_log(" Please disconnect the console cable and insert a loopback cable  \n");
	post_log(" After approximately 5 secs, remove the loopback cable and\n");
	post_log(" reinsert the console cable.\n");

	ctlr_proc[CTLR_UART].init = uart_serial_init;
	ctlr_proc[CTLR_UART].putc = uart_serial_putc;
	ctlr_proc[CTLR_UART].getc = uart_serial_getc;
	post_log("\n\n");

	for (i = 0; i < CTRL_LIST_SIZE; i++) {
		if (test_ctlr (ctlr_list[i][0], ctlr_list[i][1]) != 0) {
			ext_res = -1;
		}
	}
	post_log("\n\n");
	if(uart_flag) {
		for(i = 0;i < 25; i++){
			post_log(" Prints the external loopback result \n");
			post_log(" i = %d, ch1 = %c ,  ch1 = 0x%x   \n",i, ch1[i],ch1[i]); 
			post_log("\n\n\n");
		}
	}
	for(i =0;  i< 50; i++)
	{
		if (int_res == -1) {
			post_log(" Internal loopback failed\n");
		}
		else {
			post_log(" Internal loopback passed \n");
		}
		udelay(20000);
		if(ext_res == -1) {
			post_log(" External loopback failed \n");
		}
		else {
			post_log(" External loopback passed \n");
		}
		udelay(500000);
	}
	udelay(500000);
	if(int_res == 0 && ext_res == 0) {
		post_log("\n\n");
		NS16550_init_flush_rx_tx(serial_ports[0]);
		return 0;
	}
	post_log("\n\n");
	NS16550_init_flush_rx_tx(serial_ports[0]);
	return -1;
}


void NS16550_init_internal (NS16550_t com_port, int baud_divisor)
{
	com_port->ier = 0x00;
	com_port->lcr = LCR_BKSE | LCRVAL;
	com_port->dll = baud_divisor & 0xff;
	com_port->dlm = (baud_divisor >> 8) & 0xff;
	com_port->lcr = LCRVAL;
	com_port->mcr = MCR_LOOP_INTERNAL | MCRVAL;
	com_port->fcr = FCRVAL;
}

void NS16550_init_flush_rx_tx (NS16550_t com_port)
{
	com_port->fcr = FCRVAL;
}

char NS16550_getc_diag (NS16550_t com_port)
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
