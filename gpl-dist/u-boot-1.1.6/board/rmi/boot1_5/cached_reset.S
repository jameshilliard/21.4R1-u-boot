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

#include "rmi/regdef.h"
#include "rmi/asm.h"
#include "rmi/mipscopreg.h"
#include "rmi/mips-exts.h"
#include "rmi/msgring.h"
#include "rmi/xlr_cpu.h"
#include "rmi/bridge.h"
#include "rmi/on_chip.h"
#include "rmi/gpio.h"		
#include "rmi/shared_structs_offsets.h"
				
	.text

	.set push
	
EXPORT(cached_reset)

	/* Read CPU ID */
	xlr_cpu_id a2, a0
	
	/* Initialize Status register */
	mfc0	t0, COP_0_STATUS
	li	t1, 0xffa7ffff
	nor	t1, t1, t1
	and	t0, t0, t1	
	li	t1, 0x500000c0
	or	t0, t0, t1
	mtc0	t0, COP_0_STATUS
	
	/* If NMI, jump to the NMI handler */
	li	t1, STAT_0_NMI
	and	t1, t0, t1
	bnez	t1, nmi
	nop

	/* If not master cpu, wait for a message */
	beqz	a2, master_cpu
	nop

slave_cpu:
	jal	enable_l1_icache
	nop
	jal	enable_l1_dcache
	nop
	jal	enable_cpu_l2_cache
	nop
	sync
	jal	enable_all_threads
	nop
	
	/* update the cpu online map */
	jal	boot2_update_cpu_online_map
	nop	

	/* wait for a message in bucket# == thrid */
	mfc0	a0, COP_0_EBASE,1
	andi	a0, a0, 0x3
	
	li	t1, 1
	sllv	t1, t1, a0
1:	msgwait	t1
	msgld	a0
	/* wait for pending to clear */
2:	mfc2	t2, MSGRNG_MSG_STATUS_REG, 0
	andi	t3, t2, 0x08
	bnez	t3, 2b
	nop
	/* check for empty bucket and pending fail */
	andi	t3, t2, 0x30
	bnez	t3, 1b
	nop
	/* Check for SW code */
	srl	t3, t2, 8
	andi	t3, 0xff
	li	t4, MSGRNG_CODE_BOOT_WAKEUP
	bne	t3, t4, 1b
	nop
	/* check for size */
	srl	t3, t2, 6
	andi	t3, 0x03
	li	t4, 3
	bne	t3, t4, 1b
	nop
	/* msg0=sp, msg1=gp, msg2=fn msg3=args */
	dmfc2	sp, MSGRNG_RX_BUF_REG, 0
	dmfc2	gp, MSGRNG_RX_BUF_REG, 1
	dmfc2	a0, MSGRNG_RX_BUF_REG, 2
	dmfc2	a1, MSGRNG_RX_BUF_REG, 3

 
        /* Clear BEV */
        mfc0    t0, COP_0_STATUS
        li      t1, 0x00400000
        nor     t1, t1, t1
        and     t0, t0, t1
        mtc0    t0, COP_0_STATUS
 
	
	jr	a0
	nop
	/* should never come here */
	la	t0, halt
	jalr	t0
	nop
	
master_cpu:			

	li	a0, GPIO_LED_OUTPUT_CODE_RESET
	jal	gpio_led_output
	nop
		
        mfc0    t0, COP_0_STATUS
	
	li	t1, STAT_0_SR
	and	t1, t0, t1
	bnez	t1, soft_reset
	nop	

	/* Hard reset fall thru */

hard_reset:

	li	a0, GPIO_LED_OUTPUT_CODE_HARD_RESET
	jal	gpio_led_output
	nop
	/* POST and other stuff */
	
		
soft_reset:
	li	a0, GPIO_LED_OUTPUT_CODE_SOFT_RESET
	jal	gpio_led_output
	nop
	
	la	t0, main
	jr	t0
	nop

nmi:	la	t0, g_boot1_info
	beqz	t0, 2f
	nop	
	ld	t0, boot1_info_nmi_handler_off(t0)
	beqz	t0, 2f
	nop
	jalr	t0
	nop
	
2:	
	la	t0, nmi_handler
	jalr	t0
	nop

LEAF(halt)		
	xlr_cpu_id k0, k1
	
	PRINTF("CPU_%x HALTED\n", k0)	
1:	wait
	b       1b
	nop
END(halt)
	
LEAF(dump_ex_info)
	move	k1, ra
	
	mfc0	k0, COP_0_EPC
	PRINTF("epc = %x, ", k0)
	mfc0	k0, COP_0_CAUSE
	PRINTF(" cause = %x, ", k0)	
	mfc0	k0, COP_0_STATUS
	PRINTF(" status = %x \n", k0)
	PRINTF(" sp = %x \n", sp)
	mfc0	k0, COP_0_BADVADDR
	PRINTF(" badva = %x \n", k0)

	move	ra, k1
	jr	ra
	nop
END(dump_ex_info)
	
	.set pop
