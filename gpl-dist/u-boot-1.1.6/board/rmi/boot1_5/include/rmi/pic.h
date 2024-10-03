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
#ifndef _PIC_H
#define _PIC_H

#define PIC_IRT_WD_INDEX     0
#define PIC_IRT_TIMER_0_INDEX      1
#define PIC_IRT_TIMER_1_INDEX      2
#define PIC_IRT_TIMER_2_INDEX      3
#define PIC_IRT_TIMER_3_INDEX      4
#define PIC_IRT_TIMER_4_INDEX      5
#define PIC_IRT_TIMER_5_INDEX      6
#define PIC_IRT_TIMER_6_INDEX      7
#define PIC_IRT_TIMER_7_INDEX      8
#define PIC_IRT_UART_0_INDEX       9
#define PIC_IRT_UART_1_INDEX       10
#define PIC_IRT_I2C_0_INDEX       11
#define PIC_IRT_I2C_1_INDEX       12

#define PIC_SYS_TIMER_MAXVAL_0_BASE 0x100
#define PIC_SYS_TIMER_MAXVAL_1_BASE 0x110

#define PIC_SYS_TIMER_0_BASE 0x120
#define PIC_SYS_TIMER_1_BASE 0x130

#define PIC_CTRL    0x00
#define PIC_IPI     0x04
#define PIC_INT_ACK 0x06

#define WD_MAX_VAL_0 0x08
#define WD_MAX_VAL_1 0x09
#define WD_MASK_0    0x0a
#define WD_MASK_1    0x0b
#define WD_HEARBEAT_0 0x0c
#define WD_HEARBEAT_1 0x0d

#define PIC_IRT_0_BASE 0x40
#define PIC_IRT_1_BASE 0x80

#define PIC_IRT_0_WD     (PIC_IRT_0_BASE   + PIC_IRT_WD_INDEX)
#define PIC_IRT_1_WD     (PIC_IRT_1_BASE   + PIC_IRT_WD_INDEX)
#define PIC_IRT_0_TIMER_0     (PIC_IRT_0_BASE   + PIC_IRT_TIMER_0_INDEX)
#define PIC_IRT_1_TIMER_0     (PIC_IRT_1_BASE   + PIC_IRT_TIMER_0_INDEX)
#define PIC_IRT_0_TIMER_1     (PIC_IRT_0_BASE   + PIC_IRT_TIMER_1_INDEX)
#define PIC_IRT_1_TIMER_1     (PIC_IRT_1_BASE   + PIC_IRT_TIMER_1_INDEX)
#define PIC_IRT_0_TIMER_2     (PIC_IRT_0_BASE   + PIC_IRT_TIMER_2_INDEX)
#define PIC_IRT_1_TIMER_2     (PIC_IRT_1_BASE   + PIC_IRT_TIMER_2_INDEX)
#define PIC_IRT_0_TIMER_3     (PIC_IRT_0_BASE   + PIC_IRT_TIMER_3_INDEX)
#define PIC_IRT_1_TIMER_3     (PIC_IRT_1_BASE   + PIC_IRT_TIMER_3_INDEX)
#define PIC_IRT_0_TIMER_4     (PIC_IRT_0_BASE   + PIC_IRT_TIMER_4_INDEX)
#define PIC_IRT_1_TIMER_4     (PIC_IRT_1_BASE   + PIC_IRT_TIMER_4_INDEX)
#define PIC_IRT_0_TIMER_5     (PIC_IRT_0_BASE   + PIC_IRT_TIMER_5_INDEX)
#define PIC_IRT_1_TIMER_5     (PIC_IRT_1_BASE   + PIC_IRT_TIMER_5_INDEX)
#define PIC_IRT_0_TIMER_6     (PIC_IRT_0_BASE   + PIC_IRT_TIMER_6_INDEX)
#define PIC_IRT_1_TIMER_6     (PIC_IRT_1_BASE   + PIC_IRT_TIMER_6_INDEX)
#define PIC_IRT_0_TIMER_7     (PIC_IRT_0_BASE   + PIC_IRT_TIMER_7_INDEX)
#define PIC_IRT_1_TIMER_7     (PIC_IRT_1_BASE   + PIC_IRT_TIMER_7_INDEX)
#define PIC_IRT_0_UART_0 (PIC_IRT_0_BASE + PIC_IRT_UART_0_INDEX)
#define PIC_IRT_1_UART_0 (PIC_IRT_1_BASE + PIC_IRT_UART_0_INDEX)
#define PIC_IRT_0_UART_1 (PIC_IRT_0_BASE + PIC_IRT_UART_1_INDEX)
#define PIC_IRT_1_UART_1 (PIC_IRT_1_BASE + PIC_IRT_UART_1_INDEX)
#define PIC_IRT_0_I2C_0 (PIC_IRT_0_BASE + PIC_IRT_I2C_0_INDEX)
#define PIC_IRT_1_I2C_0 (PIC_IRT_1_BASE + PIC_IRT_I2C_0_INDEX)
#define PIC_IRT_0_I2C_1 (PIC_IRT_0_BASE + PIC_IRT_I2C_1_INDEX)
#define PIC_IRT_1_I2C_1 (PIC_IRT_1_BASE + PIC_IRT_I2C_1_INDEX)

#define PIC_IRQ_BASE      8
#define PIC_IRT_FIRST_IRQ PIC_IRQ_BASE
#define PIC_WD_IRQ      (PIC_IRQ_BASE + PIC_IRT_WD_INDEX)
#define PIC_TIMER_0_IRQ (PIC_IRQ_BASE + PIC_IRT_TIMER_0_INDEX)
#define PIC_TIMER_1_IRQ (PIC_IRQ_BASE + PIC_IRT_TIMER_1_INDEX)
#define PIC_TIMER_2_IRQ (PIC_IRQ_BASE + PIC_IRT_TIMER_2_INDEX)
#define PIC_TIMER_3_IRQ (PIC_IRQ_BASE + PIC_IRT_TIMER_3_INDEX)
#define PIC_TIMER_4_IRQ (PIC_IRQ_BASE + PIC_IRT_TIMER_4_INDEX)
#define PIC_TIMER_5_IRQ (PIC_IRQ_BASE + PIC_IRT_TIMER_5_INDEX)
#define PIC_TIMER_6_IRQ (PIC_IRQ_BASE + PIC_IRT_TIMER_6_INDEX)
#define PIC_TIMER_7_IRQ (PIC_IRQ_BASE + PIC_IRT_TIMER_7_INDEX)
#define PIC_UART_0_IRQ  (PIC_IRQ_BASE + PIC_IRT_UART_0_INDEX)
#define PIC_UART_1_IRQ  (PIC_IRQ_BASE + PIC_IRT_UART_1_INDEX)
#define PIC_I2C_0_IRQ   (PIC_IRQ_BASE + PIC_IRT_I2C_0_INDEX)
#define PIC_I2C_1_IRQ   (PIC_IRQ_BASE + PIC_IRT_I2C_1_INDEX)
#define PIC_IRT_LAST_IRQ (PIC_IRQ_BASE + 23)

#define PIC_IPI_IRQ     (PIC_IRT_LAST_IRQ + 1)

extern void pic_init(void);

#endif
