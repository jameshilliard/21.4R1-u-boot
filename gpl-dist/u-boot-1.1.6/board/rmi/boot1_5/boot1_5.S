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
#include "rmi/mem_ctrl.h"
#include "rmi/cache.h"

#include "rmi/gpio.h"
#include "rmi/shared_structs_offsets.h"

#ifdef VERBOSE0
#define DBG(s)
#define DBGF(s,arg)
#else
#define DBG(s) PRINT(s)
#define DBGF(s,arg) PRINTF(s,arg)
#endif	
				
LEAF(boot1_5)
	.set noreorder

    /* flash the led */
	li    a0, GPIO_LED_OUTPUT_CODE_MAIN
	jal   gpio_led_output
	nop
		
	jal	invalidate_l1_caches
	nop
    
    /* Starting SPD Here... */
	la	gp, reentrypoint
	dmtc0	gp, $22, 1
	la	gp, g_boot1_info

	/* a0:	argc */
	move	a0, zero
	/* a1:	argv */
	move	a1, zero
	/* a2:	envp */
	move	a2, zero
	/* a3:	psb_info */
	move	a3, gp
	/* osscratch:	psb_info */
	dmtc0	gp, $22, 0

	li	t0, 0
	la	t1, _fdata
	la	t2, __stack
	move	sp, t2
	/* Lock 24 K of D-cache */
1:
	cache    Fetch_Lock_D, 0(t1)
	addiu	t1, 32
	addiu	t0, 1
        slti	t2, t0, 768 
        bnez    t2, 1b
        nop 

        li t0,0
        la t1, .text
4:       
        cache  Fetch_Lock_I, 0(t1)
        addiu  t1,32
        addiu  t0,1
        slti   t2,t0,768
        bnez   t2,4b
        nop       
 
        #
        # zero out the bss
        #
    	la      t0, __bss_start
	la      t1, _bss_end
3:      sw      zero, 0(t0)
	addiu   t0, 4
	bne     t0, t1, 3b
	nop

       	/* Jump to dram code */	
	jal	dram_spd_main
	nop
			
2:	/* Should never get here */
	PRINT("FATAL ERROR... HALTING BOOTLOADER...\n")
	jal	halt
	nop

END(boot1_5)


LEAF(boot1_5_reentry)
	
reentrypoint:
	la	t0, 0
	la	t1, _fdata
	/* Lock 24 K of D-cache */
3:
	cache   Hit_Invalidate_D, 0(t1)
	addiu	t1, 32
	addiu	t0, 1
	slti	t2, t0, 768 
        bnez    t2, 3b
        nop
	
	jal dram_post
	nop	
END(boot1_5_reentry)
