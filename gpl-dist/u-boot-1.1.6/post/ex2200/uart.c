/*
 * Copyright (c) 2008-2010, Juniper Networks, Inc.
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

#include <common.h>
#include <post.h>
#if CONFIG_POST & CFG_POST_UART
#include "uart/mvUart.h"


DECLARE_GLOBAL_DATA_PTR;

#define CTLR_UART   0
#define CTRL_LIST_SIZE (sizeof(ctlr_list) / sizeof(ctlr_list[0]))

static int uart_serial_init (int port);
static int  uart_serial_getc_diag(int port);
static	char ch1[25];
static int ctlr_list[][2] = {{CTLR_UART, 0} };

static struct {
    int (*init) (int);
    void (*putc) (char, int);
    int (*getc) (int);
} ctlr_proc[2];

static MV_UART_PORT *serial_ports[MV_UART_MAX_CHAN] = {
    mvUartBase(0),
    mvUartBase(1),
};

int 
test_ctlr (int ctlr, int port, int int_loop)
{
    int res = -1;
    char test_str[]  = "*** UART Test String ***\r";
    int i=0;
    char ch;
    int time;
    
    for (i = 0; i < 24; i++) {
        ch1[i] = 0;
    }
    ctlr_proc[ctlr].init(port);

    if (!int_loop) {
        for (time = 0; time < 1200; time++) {
            ctlr_proc[ctlr].putc ('.', port);
            ch = uart_serial_getc_diag(port);
            if (ch == '.') {
                break;
            }
            udelay(50000);
        }
        if (time == 1200) {
            return (-1);
        }
        /* Dot  '.' is displayed , to eliminate this one
           dot extra getc_diag function is added. */
        uart_serial_getc_diag(port); 
								   
    }

    for (i = 0; i < sizeof(test_str) - 1; i++) {
        ctlr_proc[ctlr].putc (test_str[i], port);
        ch = ctlr_proc[ctlr].getc(port);
        ch1[i] = ch;
        if (test_str[i] == '\n') {
            if (ch != '\r') {
                goto Done;
            }
        }
        else if (ch != test_str[i]){
			goto Done;
        }

    }
    res = 0;
Done:
    return (res);
}

void 
uart_flush_rx_tx (MV_UART_PORT *com_port)
{
    com_port->fcr = (FCR_FIFO_EN | FCR_RXSR | FCR_TXSR);
}

char 
uart_getc_diag (MV_UART_PORT *com_port)
{
    int i = 0;

    for (i = 0; i < 20000;  i++) {
        if ((com_port->lsr & LSR_DR) == 0) {
            break;
        }
    }
    return (com_port->rbr);
}

static uint32_t 
calc_divisor (void)
{
    return ((CFG_TCLK / 16) / gd->baudrate);
}

static int 
uart_serial_init_internal (int port)
{
    uint32_t clock_divisor;
    
    clock_divisor = calc_divisor();
    serial_ports[port]->ier = 0x00;
    serial_ports[port]->lcr = LCR_DIVL_EN;           /* Access baud rate */
    serial_ports[port]->dll = clock_divisor & 0xff;    /* baud */
    serial_ports[port]->dlm = (clock_divisor >> 8) & 0xff;
    serial_ports[port]->lcr = LCR_8N1;         /* 8 data, 1 stop, no parity */
    serial_ports[port]->mcr = 0x10;
    
    /* Clear & enable FIFOs */
    serial_ports[port]->fcr = FCR_FIFO_EN | FCR_RXSR | FCR_TXSR;
    return (0);
}

static int 
uart_serial_init_normal (int port)
{
    uint32_t clock_divisor;
    
    clock_divisor = calc_divisor();
    serial_ports[port]->ier = 0x00;
    serial_ports[port]->lcr = LCR_DIVL_EN;           /* Access baud rate */
    serial_ports[port]->dll = clock_divisor & 0xff;    /* baud */
    serial_ports[port]->dlm = (clock_divisor >> 8) & 0xff;
    serial_ports[port]->lcr = LCR_8N1;        /* 8 data, 1 stop, no parity */
    serial_ports[port]->mcr = 0x0;
    
    /* Clear & enable FIFOs */
    serial_ports[port]->fcr = FCR_FIFO_EN | FCR_RXSR | FCR_TXSR;
    return (0);
}

void 
uart_serial_putc (const char c, int port)
{
    if (c == '\n') {
        mvUartPutc(port, '\r');
        return;
    }
    mvUartPutc(port, c);
}

int  
uart_serial_getc (int port)
{
    return mvUartGetc(port);
}

int  
uart_serial_getc_diag (int port)
{
    return uart_getc_diag(serial_ports[port]);
}

int 
uart_post_test (int flags)
{
    int int_res = 0;
    int ext_res = 0;
    int i=0;
    int uart_flag =0;

    ctlr_proc[CTLR_UART].init = uart_serial_init_internal;
    ctlr_proc[CTLR_UART].putc = uart_serial_putc;
    ctlr_proc[CTLR_UART].getc = uart_serial_getc;

    if (flags & POST_DEBUG) {
        uart_flag = 1;
    }

    for (i = 0; i < CTRL_LIST_SIZE; i++) {
        if (test_ctlr (ctlr_list[i][0], ctlr_list[i][1], 1) != 0) {
            int_res = -1;
        }
        uart_serial_init_normal(ctlr_list[i][1]);
    }
    
    post_log("\n\n");	

    if (uart_flag) {
        post_log(" Prints the internal loopback result \n");
        for (i = 0;i < 25; i++) {
            post_log(" i = %d, ch1 = %c ,  ch1 = 0x%x   \n\n",i, ch1[i],ch1[i]); 
        }
    }
    post_log(" Please disconnect the console cable and");
    post_log(" insert a loopback cable\n");
    post_log(" After approximately 5 secs, remove the loopback cable and\n");
    post_log(" reinsert the console cable.\n");

    ctlr_proc[CTLR_UART].init = uart_serial_init_normal;
    ctlr_proc[CTLR_UART].putc = uart_serial_putc;
    ctlr_proc[CTLR_UART].getc = uart_serial_getc;
    post_log("\n\n");

    for (i = 0; i < CTRL_LIST_SIZE; i++) {
        if (test_ctlr (ctlr_list[i][0], ctlr_list[i][1], 0) != 0) {
            ext_res = -1;
        }
    }
    post_log("\n\n");
    if (uart_flag) {
        for (i = 0;i < 25; i++){
            post_log(" Prints the external loopback result \n");
            post_log(" i = %d, ch1 = %c ,  ch1 = 0x%x   \n",i, ch1[i],ch1[i]); 
            post_log("\n\n\n");
        }
    }

    if (int_res  || ext_res ) {
        post_log("-- Uart loopback test                   FAILED @\n");
    }
    else {
        post_log("-- Uart loopback test                    PASSED\n");
    }

    for (i = 0;  i < 50; i++)
    {
        if (int_res == -1) {
            post_log(" > Internal loopback                    FAILED @\n");
        }
        else {
            post_log(" > Internal loopback                    PASSED\n");
        }
        udelay(20000);
        if (ext_res == -1) {
            post_log(" > External loopback                    FAILED @\n");
        }
        else {
            post_log(" > External loopback                    PASSED\n");
        }
        udelay(500000);
    }
    udelay(500000);
    post_log("\n\n");
    uart_flush_rx_tx(serial_ports[0]);
    if (int_res == 0 && ext_res == 0) {
        return (0);
    }
    return (-1);
}
#endif /* CONFIG_POST & CFG_POST_UART */
#endif /* CONFIG_POST */
