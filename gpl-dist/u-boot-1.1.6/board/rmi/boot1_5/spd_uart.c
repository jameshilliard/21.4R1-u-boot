/***********************************************************************
Copyright 2003-2006 Raza Microelectronics, Inc.(RMI). All rights
reserved.
Use of this software shall be governed in all respects by the terms and
conditions of the RMI software license agreement ("SLA") that was
accepted by the user as a condition to opening the attached files.
Without limiting the foregoing, use of this software in source and
binary code forms, with or without modification, and subject to all
other SLA terms and conditions, is permitted.
Any transfer or redistribution of the source code, with or without
modification, IS PROHIBITED,unless specifically allowed by the SLA.
Any transfer or redistribution of the binary code, with or without
modification, is permitted, provided that the following condition is
met:
Redistributions in binary form must reproduce the above copyright
notice, the SLA, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution:
THIS SOFTWARE IS PROVIDED BY Raza Microelectronics, Inc. `AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL RMI OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*****************************#RMI_3#***********************************/
#include "rmi/types.h"
#include "rmi/bridge.h"
#include "rmi/on_chip.h"
#include "rmi/uart.h"
#include "rmi/printk.h"
#include "rmi/stackframe.h"
#include "rmi/fifo.h"
#include "rmi/byteorder.h" /* for cpu_to_be32() and be32_to_cpu() */

static struct fifo tx_fifo;
static struct fifo rx_fifo;

#define TX_BURST_SIZE 1

#define DMESG_BUFSIZE 0x1000
static unsigned char dmesg_buf[DMESG_BUFSIZE];
static int dmesg_buf_index;
static int dmesg_trace = 0;


static void uart_outbyte(phoenix_reg_t * mmio, int data)
{
	if (!dmesg_trace) {
		mmio[UART_THR] = cpu_to_be32(data);
	} else {
		if (dmesg_buf_index < DMESG_BUFSIZE)
			dmesg_buf[dmesg_buf_index++] = (unsigned char)data;
	}
}

void uart_flush_tx_buf(void)
{
	int data = 0;
	int lsr = 0;
	volatile uint32_t *mmio = phoenix_io_mmio(PHOENIX_IO_UART_0_OFFSET);

	for (;;) {

		lsr = be32_to_cpu(mmio[UART_LSR]);

		if (lsr & 0x20) {

			if (!fifo_dequeue(&tx_fifo, &data))
				break;

			/* Tx available, try to send more characters */
			uart_outbyte(mmio, data);
		}
	}
}

int tstbyte(void)
{
	int data = 0;
	if (fifo_dequeue(&rx_fifo, &data)) {
		/* FIFO not empty */
		return 1;
	}
	return 0;
}

char inbyte_nowait(void)
{
    int data = 0;
    int lsr = 0;
    volatile uint32_t *mmio = phoenix_io_mmio(PHOENIX_IO_UART_0_OFFSET);

    if (fifo_dequeue(&rx_fifo, &data)) {
        /* characters to be read already in fifo */
        return (char)data;
    }

    do {
        lsr = be32_to_cpu(mmio[UART_LSR]);
        data = 0;
        if (lsr & 0x80) {
            /* parity/frame/break - push a 0! */
            break;
        }
        if (lsr & 0x01) {
            /* Rx Data */
            data = be32_to_cpu(mmio[UART_RHR]);
            break;
        }
    } while (0);

    return (char)data;
}

char inbyte(void)
{
	int data;
    int lsr;
    volatile uint32_t *mmio = phoenix_io_mmio(PHOENIX_IO_UART_0_OFFSET);

    if (fifo_dequeue(&rx_fifo, &data)) {
        return data;
    }

    data = 0;
    do {    
        lsr = be32_to_cpu(mmio[UART_LSR]);
        if (lsr & 0x80) { /* parity/frame/break - push a 0! */ 
            return 0;
        }
        if (lsr & 0x01) { /* Rx Data */ 
            return be32_to_cpu(mmio[UART_RHR]);
        }
    } while (1);

	return data;
}

int outbyte(char c)
{
    int lsr, _stack[2], _top;
    volatile uint32_t *mmio = phoenix_io_mmio(PHOENIX_IO_UART_0_OFFSET);

    lsr = _top = 0; 

    _stack[_top] = c;
    if (c == '\n') { 
        _top++;
        _stack[_top] = '\r'; 
    }

    for ( ; _top >= 0; _top--) {
        do {
            lsr = be32_to_cpu(mmio[UART_LSR]);
            if (lsr & 0x80) { 
                fifo_enqueue(&rx_fifo, 0);
            }
            if (lsr & 0x01) { 
                fifo_enqueue(&rx_fifo, be32_to_cpu(mmio[UART_RHR]));
            }
        } while (!(lsr & 0x20)); 

        mmio[UART_THR] = cpu_to_be32(_stack[_top]);
    }

    return 0;
}

void uart_int_handler(int irq, struct pt_regs *regs)
{
}

int putch(int ch){
    return outbyte(ch);
}

void rmi_uart_fifo_init(void)
{
	fifo_init(&rx_fifo);
	fifo_init(&tx_fifo);
}

void rmi_uart_init(void)
{
	volatile uint32_t *mmio = phoenix_io_mmio(PHOENIX_IO_UART_0_OFFSET);

	fifo_init(&rx_fifo);
	fifo_init(&tx_fifo);

	/* Set up the baud rate */
	mmio[UART_LCR] = cpu_to_be32(be32_to_cpu(mmio[UART_LCR]) | (1 << 7));
	mmio[UART_DLB_1] = cpu_to_be32(UART_BR_DLB1);
	mmio[UART_DLB_2] = cpu_to_be32(UART_BR_DLB2);
	mmio[UART_LCR] = cpu_to_be32(be32_to_cpu(mmio[UART_LCR]) & ~(1 << 7));

}

void rmi_uart1_init(void)
{
        volatile uint32_t *mmio = phoenix_io_mmio(PHOENIX_IO_UART_1_OFFSET);

        /* Set up the baud rate */
        mmio[UART_LCR] = cpu_to_be32(be32_to_cpu(mmio[UART_LCR]) | (1 << 7));
        mmio[UART_DLB_1] = cpu_to_be32(UART_BR_DLB1);
        mmio[UART_DLB_2] = cpu_to_be32(UART_BR_DLB2);
        mmio[UART_LCR] = cpu_to_be32(be32_to_cpu(mmio[UART_LCR]) & ~(1 << 7));

}

#if 0
void console_init(void)
{
	printk_init();
}
#endif
