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
#include "rmi/msgring.h"
#include "rmi/xlr_cpu.h"
#include "rmi/bridge.h"
#include "rmi/on_chip.h"
#include "rmi/gpio.h"		
			
	.section .exception.reset, "ax"

	.set push
	.set mips64
	
EXPORT(reset)
	li  t0, 0xcafe /* RMI signature */
	la	t0, cached_reset
	jr	t0
	nop	
	la	t0, halt
	jalr	t0
	nop	
	
NESTED(nmi_handler, 0, sp)
	/* For now, just trigger a soft reset */
	li	t0, BRIDGE_PHNX_IO_RESET_VADDR + PHOENIX_IO_GPIO_OFFSET
	li	t1, 1
	sw	t1, (GPIO_SWRESET_REG << 2)(t0)
	
	/* shouldn't get here, if it does, well not much we can
	 * do about. just do nothing
	 * TODO: Phoenix NMI is really an interrupt and not an exception
	 * If the OS supports handling NMIs, send it over to it
	 */
	la	t0, halt
	jalr	t0
	nop
END(nmi_handler)
	
	.set pop

	
