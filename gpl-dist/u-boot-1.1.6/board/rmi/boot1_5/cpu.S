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

#include "rmi/xlr_cpu.h"

#if defined(XRTOS_BOOT_HACKS) && !defined(TLB_GLOBAL_MODE)
#define TLB_GLOBAL_MODE
#endif
	
#define THREADS_PER_CPU 4
	
LEAF(enable_all_threads)

	/* all threads are enabled in this reg, but write it anyway */
	li	t1, ((CPU_BLOCKID_IFU << 8) | IFU_THR_EN_REGID)
	li	t0, 0x0f
	mtcr	t0, t1

	/* setup MMU thread register */
	li	t1, ((CPU_BLOCKID_MMU << 8) | MMU_SETUP_REGID)
	mfcr	t0, t1
	
#ifdef TLB_GLOBAL_MODE
	ori	t0, 0x01
#endif
	
#if (THREADS_PER_CPU == 4)
	ori	t0, 0x06
#elif (THREADS_PER_CPU == 2)
	ori	t0, 0x04
#else
	// no threads
#endif
	mtcr	t0, t1

#ifdef DEBUG_MODE_WATCH_EXCEPTION
	li	t1, ((CPU_BLOCKID_IEU << 8) | IEU_DEFEATURE_REGID)
	mfcr	t0, t1
	ori	t0, 0x80
	mtcr	t0, t1
#endif	
		
	jr	ra
	nop
	
END(enable_all_threads)
