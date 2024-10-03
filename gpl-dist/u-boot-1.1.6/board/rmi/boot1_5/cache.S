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
#include "rmi/cache.h"
#include "rmi/on_chip.h"
#include "rmi/xlr_cpu.h"
#include "rmi/bridge.h"

#define BRIDGE_ADDR_ERR_LOG

LEAF(invalidate_l1_caches)

	li	t0, 0
	li	t1, 0x80000000
	
1:	cache	Hit_Invalidate_I, 0(t1)
	cache	Hit_Invalidate_D, 0(t1)
	addiu	t1, 32
	addiu	t0, 1
	slti	t2, t0, 1024
	bnez	t2, 1b
	nop
		
	jr	ra
	nop
	
END(invalidate_l1_caches)

LEAF(flush_l1_caches)

	li	t0, 0
	li	t1, 0x80000000
	
1:	cache	Index_Invalidate_I, 0(t1)
	cache	Index_Writeback_Inv_D, 0(t1)
	addiu	t1, 32
	addiu	t0, 1
	slti	t2, t0, 1024
	bnez	t2, 1b
	nop
		
	jr	ra
	nop
	
END(flush_l1_caches)	
	
LEAF(enable_l1_icache)
	
	li	t1, ((CPU_BLOCKID_ICU << 8)|ICU_DEFEATURE_REGID)
	mfcr	t0, t1
	/* enable way0->way7 */
	ori	t0, 0x7f84
	mtcr	t0, t1
	
	jr	ra
	nop
	
END(enable_l1_icache)
	
LEAF(enable_l1_dcache)

	/* Enable Caches - (Cache are initialized to invalid state) */
	li	t1, ((CPU_BLOCKID_LSU << 8)|LSU_CFG0_REGID)
	mfcr	t0, t1
	ori	t0, 0x01
	ori	t0, 0x2e
	
	mtcr	t0, t1
		
	jr	ra
	nop
	
END(enable_l1_dcache)

LEAF(config_l2)

	jr	ra
	nop
	
END(config_l2)

LEAF(enable_cpu_l2_cache)

	/* Enable Caches - (Cache are initialized to invalid state) */
	li	t1, ((CPU_BLOCKID_LSU << 8)|LSU_CFG0_REGID)
	mfcr	t0, t1
	ori	t0, 0x10
	mtcr	t0, t1
	
	jr	ra
	nop
	
END(enable_cpu_l2_cache)
	
LEAF(enable_l2_cache)
	
	li	t0, BRIDGE_PHNX_IO_RESET_VADDR + PHOENIX_IO_L2_OFFSET
	li	t2, 0x400
	
	li	t1, 0x000
	addu	t1, t0, t1
	sw	t2, 0(t1)
	
	li	t1, 0x020
	addu	t1, t0, t1
	sw	t2, 0(t1)

	li	t1, 0x040
	addu	t1, t0, t1
	sw	t2, 0(t1)

	li	t1, 0x060
	addu	t1, t0, t1
	sw	t2, 0(t1)

	li	t1, 0x080
	addu	t1, t0, t1
	sw	t2, 0(t1)

	li	t1, 0x0a0
	addu	t1, t0, t1
	sw	t2, 0(t1)

	li	t1, 0x0c0
	addu	t1, t0, t1
	sw	t2, 0(t1)

	li	t1, 0x0e0
	addu	t1, t0, t1
	sw	t2, 0(t1)

	jr	ra
	nop
		
END(enable_l2_cache)

LEAF(enable_l2_cache_no_ecc)
	
	li	t0, BRIDGE_PHNX_IO_RESET_VADDR + PHOENIX_IO_L2_OFFSET
	li	t2, 0x407
	
	li	t1, 0x000
	addu	t1, t0, t1
	sw	t2, 0(t1)
	
	li	t1, 0x020
	addu	t1, t0, t1
	sw	t2, 0(t1)

	li	t1, 0x040
	addu	t1, t0, t1
	sw	t2, 0(t1)

	li	t1, 0x060
	addu	t1, t0, t1
	sw	t2, 0(t1)

	li	t1, 0x080
	addu	t1, t0, t1
	sw	t2, 0(t1)

	li	t1, 0x0a0
	addu	t1, t0, t1
	sw	t2, 0(t1)

	li	t1, 0x0c0
	addu	t1, t0, t1
	sw	t2, 0(t1)

	li	t1, 0x0e0
	addu	t1, t0, t1
	sw	t2, 0(t1)

	jr	ra
	nop
		
END(enable_l2_cache_no_ecc)
