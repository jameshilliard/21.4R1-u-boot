#include <config.h>
#include <version.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include "rmi/shared_structs_offsets.h"
#include "rmi/asm.h"


	.set noreorder

	.globl _start
	.text
	.set noreorder
	/* Here are the assumptions in this code:
	   1. We are already running in RAM and memory is initialized
	   2. CPU and caches are already initialized 
	   */
_start:
	lui	t0, 0x5000
   ori   t0, 0x80
	mtc0	t0, $12
	nop
	nop
	nop
	la	t0, __bss_start
	la	t1, __bss_end
1:
	sw	zero, 0(t0)
	addiu	t0, 4
	bne	t0, t1, 1b
	nop

	/* Set up a temporary stack */
	la	sp, __stack_start
	addiu	sp, 4096 - 32

	/* Boot stage 1 passes boot_info structure pointer in OS scratch 0 */
	dmfc0	gp, $22, 0
	move	t1, gp
	/* copy boot1_info here - address is in gp*/
	la	t0, boot_info
	li	t2, boot1_info_size
2:
	ld	t3, 0(t1)
	sd	t3, 0(t0)
	addiu	t0, 8
	addiu	t1, 8
	addiu	t2, -8
	bnez	t2, 2b
	nop

	jal 	rmi_get_cpu_clk
	nop

	/* we return to relocate_code */
	la	t9, board_init_f
	jr	t9
	nop


/*
 * void relocate_code (addr_sp, gd, addr_moni)
 *
 * This function does not do any relocation. It is just a stub
 *
 * a0 = addr_sp
 * a1 = gd
 * a2 = destination address
 */
	.globl  relocate_code
	.ent    relocate_code
relocate_code:
	/* call rmi_board_init fn. */
	move s0, a0
	move s1, a1
	jal rmi_early_init
	nop
	move a0, s0
	move a1, s1

	/* change to the new sp */
	move	sp, a0
	move	a0, a1 /*  pointer to gd_t */
	li	a1, CFG_MONITOR_BASE /* dest_addr arg for board_init_r */

	la	t9, board_init_r
	j	t9
	nop
	.end relocate_code



