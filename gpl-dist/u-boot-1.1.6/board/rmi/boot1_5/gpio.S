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
#include "rmi/on_chip.h"
#include "rmi/bridge.h"
#include "rmi/gpio.h"

#define CPU_BITMAP 0x00
	
#ifdef VERBOSE0
#define DBG(s)
#define DBGF(s,arg)
#else
#define DBG(s) PRINT(s)
#define DBGF(s,arg) PRINTF(s,arg)
#endif
	
LEAF(gpio_wakeup_slave_cpus)
	.set	push
	.set	noreorder

	li	t0, BRIDGE_PHNX_IO_RESET_VADDR + PHOENIX_IO_GPIO_OFFSET

	/* pull all the slave cpus out of reset */
#ifdef CPU_BITMAP
	li	t1, CPU_BITMAP
	li	t2, 0x01
	nor	t2, t2, t2
	and	t1, t1, t2
#else
	li	t1, 0xfe
#endif
	sw	t1, (GPIO_CPU_RESET_REG << 2)(t0)
	
	jr	ra
	nop
			
	.set pop	
END(gpio_wakeup_slave_cpus)

	/* a0:	value to be output */
LEAF(gpio_led_output)
	.set push
	.set noreorder
    
	li	t0, BRIDGE_PHNX_IO_RESET_VADDR + PHOENIX_IO_GPIO_OFFSET

	/* set the io direction */
	lw	t1, (GPIO_IO_DIR_REG << 2)(t0)
	li	t2, GPIO_LED_BITMAP
	or      t1, t1, t2
	sw	t1, (GPIO_IO_DIR_REG << 2)(t0)

	andi	t2, a0, 0x07
	sll	t2, t2, GPIO_LED_0_SHIFT
	andi	t1, a0, 0x08
	sll	t1, t1, GPIO_LED_1_SHIFT
	or	t2, t1, t2
	lw	t1, (GPIO_IO_DATA_WR_REG << 2)(t0)
	or	t1, t1, t2
	sw	t1, (GPIO_IO_DATA_WR_REG << 2)(t0)

	/* TODO: wait for some time */
	jr	ra
	nop
	
	.set pop
END(gpio_led_output)

LEAF(xls_404_softfuse)

	.set push
	.set noreorder
    move    s8, ra

    /* Workaround for XLS404 Advisory */
    mfc0 t0, $15
    srl  t0, 8
    andi t0, 0xff
    bne  t0, 0x8c, 5f   /* XLS 404 CPU ID */
    nop
    
	DBG("SoftFusing XLS404...")
    li  t0, BRIDGE_PHNX_IO_RESET_VADDR + PHOENIX_IO_GPIO_OFFSET
    li  t1, 0x00
    sw  t1, (13 << 2)(t0)
    li  t1, 0x20      
    sw  t1, (41 << 2)(t0)                     
    li  t1, 0x02
    sw  t1, (45 << 2)(t0)                         
	DBG("Done\n")
5:
	move	ra, s8
    jr  ra
    nop
    
    .set pop
    
END(xls_404_softfuse)

LEAF(xls_408_softfuse)

	.set push
	.set noreorder
    move    s8, ra

    mfc0 t0, $15
    srl  t0, 8
    andi t0, 0xff
    bne  t0, 0x88, 6f   /* XLS 408 CPU ID */
    nop
    
	DBG("SoftFusing XLS408...")
    li  t0, BRIDGE_PHNX_IO_RESET_VADDR + PHOENIX_IO_GPIO_OFFSET
    li  t1, 0x00
    sw  t1, (13 << 2)(t0)
    li  t1, 0x20      
    sw  t1, (41 << 2)(t0)                     
	DBG("Done\n")
6:
	move	ra, s8
    jr  ra
    nop
    
    .set pop

END(xls_408_softfuse)
    
    
	/* 
	Dump the BIST information provided the RUN_BIST Bit is set in
	the GPIO Power On Reset Register. The CPU will not be halted
	here, instead, we merely print out the BIST values for people
	to look at. Later, during board initialization, the BIST value
	will be checked again, if enabled for the particular board.
	For e.g. ATX-IV boards used by OPS has the BIST checking enabled.
	In this case, if any of the BIST fails, a detailed message will
	be printed on the screen and the CPU will halt.
	*/
LEAF(gpio_dump_bist)
	.set push
	.set noreorder

	move	s8, ra

	li	t0, BRIDGE_PHNX_IO_RESET_VADDR + PHOENIX_IO_GPIO_OFFSET	
	lw	t1, (GPIO_PWRON_RESET_CFG_REG << 2)(t0)
	DBGF("PowerOn RST Config [0x%x]\n", t1)

	li	t0, BRIDGE_PHNX_IO_RESET_VADDR + PHOENIX_IO_GPIO_OFFSET	
	lw	t1, (GPIO_PWRON_RESET_CFG_REG << 2)(t0)
	
	li	t2, RUN_BIST_BIT

	and	t1,t1,t2
	
	beqz	t1, 1f
	nop
			
	li	t0, BRIDGE_PHNX_IO_RESET_VADDR + PHOENIX_IO_GPIO_OFFSET	
	lw	t1, (GPIO_BIST_ALL_GO_STATUS_REG << 2)(t0)
	DBGF("BIST All Go Status = 0x%x\n", t1)

		
	li	t0, BRIDGE_PHNX_IO_RESET_VADDR + PHOENIX_IO_GPIO_OFFSET	
	lw	t1, (GPIO_BIST_CPU_GO_STATUS_REG << 2)(t0)
	DBGF("BIST CPU Go Status = 0x%x\n", t1)
	
	li	t0, BRIDGE_PHNX_IO_RESET_VADDR + PHOENIX_IO_GPIO_OFFSET	
	lw	t1, (GPIO_BIST_DEV_GO_STATUS_REG << 2)(t0)
	DBGF("BIST DEVICE Go Status = 0x%x\n", t1)
	
	li	t0, BRIDGE_PHNX_IO_RESET_VADDR + PHOENIX_IO_GPIO_OFFSET	
	lw	t1, (GPIO_PWRON_RESET_CFG_REG << 2)(t0)

1:		
	move	ra, s8
	jr	ra
	nop
	
	.set pop	
END(gpio_dump_bist)

	
LEAF(gpio_get_procid)
    .set push
    .set noreorder
    .set mips64

#ifndef SPD_TEST
	li	t0, BRIDGE_PHNX_IO_RESET_VADDR + PHOENIX_IO_GPIO_OFFSET	
	lw	t1, (GPIO_FUSEBANK_REG << 2)(t0)

    srl t1, 21

    li t2, 0x0f
    and t2, t1, t2
    
    move t3, t2

    beqz t2, 2f
    
    andi t2, 0x09
    
    addi t2, -9
    beqz t2, 2f
    nop
    
    move t2, t3

    addi t2, -7
    beqz t2, 3f
    nop


    move t2, t3
    addi t2, -6
    beqz t2, 3f
    nop
	
    li k1, 7
    li k0, 3
    
    b 1f
    nop

3:  li k1, 6
    li k0, 1
    b 1f
    nop
#endif
    
2:
    li k1, 8
    li k0, 7

1:
	jr	ra
	nop
    .set pop
    END(gpio_get_procid)
