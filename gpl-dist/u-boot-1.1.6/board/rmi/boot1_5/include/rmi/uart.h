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
#ifndef _UART_H
#define _UART_H

#include <config.h>

#define UART_RHR 0
#define UART_THR 0
#define UART_IER 1
#define UART_IIR 2
#define UART_FCR 2
#define UART_LCR 3
#define UART_MCR 4
#define UART_LSR 5
#define UART_MSR 6

#define UART_DLB_1 0
#define UART_DLB_2 1

#define UART_DEBUG_1 8
#define UART_DEBUG_2 9

// baud rate divisors
#define UART_BR9600_DLB1 0xad
#define UART_BR9600_DLB2 0x01

#define UART_BR38400_DLB1 0x6b
#define UART_BR38400_DLB2 0x00

#define UART_BR115200_DLB1 0x23
#define UART_BR115200_DLB2 0x00

#ifdef CONFIG_FX
#define UART_BR_DLB1 UART_BR9600_DLB1
#define UART_BR_DLB2 UART_BR9600_DLB2
#else
#ifndef PHOENIX_SIM
#define UART_BR_DLB1 UART_BR38400_DLB1
#define UART_BR_DLB2 UART_BR38400_DLB2
#else
#define UART_BR_DLB1 UART_BR115200_DLB1
#define UART_BR_DLB2 UART_BR115200_DLB2
#endif
#endif


#ifndef __ASSEMBLY__
extern int outbyte(char c);
extern char inbyte(void);
void uart_init(void);
extern void uart1_init(void);
void console_init(void);
void dmesg_off(void);
void dmesg_on(void);

#endif

#endif
