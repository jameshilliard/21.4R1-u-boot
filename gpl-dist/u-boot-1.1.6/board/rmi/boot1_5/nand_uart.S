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
#include "rmi/mipscopreg.h"
#include "rmi/regdef.h"
#include "rmi/asm.h"
#include "rmi/bridge.h"
#include "rmi/on_chip.h"
#include "rmi/uart.h"

.macro	outbyte	c, reg0, reg1
	li	\reg0, BRIDGE_PHNX_IO_RESET_VADDR + PHOENIX_IO_UART_0_OFFSET

	/* check for Tx FIFO empty */
11:	lw	\reg1, (UART_LSR << 2)(\reg0)
	andi	\reg1, 0x20
	beqz	\reg1, 11b
	nop

	li	\reg1, 10
	bne	\c, \reg1, 12f
	nop
	/* print a '\r' */
	li	\reg1, 13
	sw	\reg1, (UART_THR << 2)(\reg0)	
12:			
	sw	\c, (UART_THR << 2)(\reg0)

.endm

	/*            nibble_counter, loop_counter, shift */
.macro print_hex64 val, shift, i, temp0, temp1
	move	\i, zero

23:	li	\temp0, 60
	sll	\shift, \i, 2
	subu	\shift, \temp0, \shift
	dsra	\shift, \val, \shift
	andi	\shift, 0x0f
	slti	\temp0, \shift, 10
	beqz	\temp0, 21f
	nop
	/* nibble < 10 */
	addiu	\shift, 48
	b	22f
	nop
	/* nibble >= 10 */
21:	addiu	\shift, 55
	
22:	outbyte	\shift, \temp0, \temp1

	addiu	\i, 1
	slti	\temp0, \i, 16
	bnez	\temp0, 23b
	nop
	
.endm


	/* This is really a puts */
LEAF(uart_printf)
	/* No prints from boot1_75. Cannot access rodata section from here */
	jr	ra
	nop
	move	t4, a1

1:	lb	t0, 0(a0)
	beqz	t0, 9f
	nop

	/* format marker */
	li	t1, '%'
	bne	t0, t1, 2f
	nop

	/* skip the format char */
	addiu	a0, a0, 1
	lb	t0, 0(a0)
	beqz	t0, 9f
	nop
	/* for now, always print the a1 arg */
	print_hex64 a1, t0, t1, t2, t3
	b	3f
	nop	
2:			
	outbyte	t0, t1, t2
	
3:		
	addiu	a0, a0, 1
	b	1b
	nop
			
9:	jr	ra	
	nop
END(uart_printf)

