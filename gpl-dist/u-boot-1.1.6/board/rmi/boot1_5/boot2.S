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
#include "rmi/mips-exts.h"
#include "rmi/bridge.h"
#include "rmi/shared_structs_offsets.h"
#inlcude "cache.h"
		

	
	/* a0:	cpu_id */
LEAF(boot2_update_cpu_online_map)
	
	la	t7, g_boot1_info
	xlr_cpu_id a0, t0
	bnez	a0, 1f
	nop

#ifndef PHOENIX_SIM	
	/* initialize the online map */
	sd	zero, boot1_info_cpu_online_map_off(t7)
#endif
	
	/* slave cpus update the map here */
1:	li	t1, 1
	sll	t1, t1, a0
	
2:	lld	t0, boot1_info_cpu_online_map_off(t7)
	or	t0, t0, t1
	scd	t0, boot1_info_cpu_online_map_off(t7)
	beqz	t0, 2b
	nop		
	
	jr	ra
	nop
		
END(boot2_update_cpu_online_map)

LEAF(boot2_update_psb_info)
	
	/* Store information for use by boot2? */
	la	t0, g_boot1_info
	
	li	t1, BRIDGE_PHNX_IO_RESET_VADDR
	sd	t1, boot1_info_io_base_off(t0)
	
	la	t1, asm_printf
	sd	t1, boot1_info_uart_print_off(t0)
	
	li	t1, 1
	sd	t1, boot1_info_boot_level_off(t0)

	li	t1, 1	
	sd	t1, boot1_info_output_device_off(t0)
	
	/* magic dword:	[63: 32] -> version (major.minor):
	 *		[31: 0] -> goodbeef
	 */
	li	t1, 0x900dbeef
	dsll32  t1, t1, 0
	dsrl32	t1, t1, 0
	li	t2, boot1_info_version
	dsll32  t2, t2, 0
	or	t1, t2, t1
	sd	t1, boot1_info_magic_dword_off(t0)
	
	li	t1, boot1_info_size
	sd	t1, boot1_info_size_off(t0)
	
	la	t1, uart_putchar
	sd	t1, boot1_info_uart_putchar_off(t0)

	la	t1, nmi_handler
	sd	t1, boot1_info_nmi_handler_off(t0)

	li	t1, PSB_COMPILE_TIME
	dsll32	t1, t1, 0
	li	t2, PSB_MAJOR_VER
	andi	t2, t2, 0xff
	sll	t2, t2, 8
	or	t1, t1, t2
	li	t2, PSB_MINOR_VER
	andi	t2, t2, 0xff
	or	t1, t1, t2
	sd	t1, boot1_info_psb_version_off(t0)
	
	/* for now terminate if used any of the below serives */
	sd	zero, boot1_info_master_reentry_sp_off(t0)
	sd	zero, boot1_info_master_reentry_gp_off(t0)
	
	la	t1, halt
	sd	t1, boot1_info_led_output_off(t0)
	sd	t1, boot1_info_init_off(t0)
	sd	t1, boot1_info_exit_off(t0)
	sd	t1, boot1_info_warm_reset_off(t0)
	sd	t1, boot1_info_wakeup_off(t0)
	sd	t1, boot1_info_master_reentry_fn_off(t0)
	sd	t1, boot1_info_slave_reentry_fn_off(t0)
	sd	t1, boot1_info_uart_getchar_off(t0)

	jr	ra
	nop
	
END(boot2_update_psb_info)

LEAF(boot2_clear_bev)
	/* Initialize Status register */
	mfc0	t0, COP_0_STATUS
	li	t1, 0x00400000
	nor	t1, t1, t1
	and	t0, t0, t1
	mtc0	t0, COP_0_STATUS

	jr	ra
	nop
END(boot2_clear_bev)

LEAF(boot2_set_bev)
	/* Initialize Status register */
	mfc0	t0, COP_0_STATUS
	li	t1, 0x00400000
	or	t0, t0, t1
	mtc0	t0, COP_0_STATUS
	
	jr      ra
	nop
END(boot2_set_bev)

LEAF(boot1_5_l2_cache_load)

	/* Load boot1_5 into memory    
	 *  0		:	elf entry
	 *  8		:	Num program headers
	 * 16,24	:	addr, dwords
	 * 32,40	:	addr, dwords
	 * 48,56	:	addr, dwords
	 */
	
    move t0, a0
	/* binary data of the program headers - t5 */
	li	t5, 256
	addu	t5, t0, t5

	/* load the number of loadable entries - t1 */
	ld	t1, 8(t0)

	/* point to the next table entry */
1:	addiu	t0, 16

	/* if no more table entries, exit the loop */
	beqz	t1, 3f
	nop
	addiu	t1, -1
	
	/* load the address, size(in dwords) for the current index */
 
	ld	t2, 0(t0)
	ld	t3, 8(t0)
	move 	v1, t3		/* size */

	/* if size (num dwords) of the entry is zero, goto the next table entry */
2:	
	beqz	t3, 1b
	nop
	addiu	t3, -1
	
	/* load the next dword and store to the virt address */
	ld	t4, 0(t5)
	sync
	sd  t4, 0(t2)
	sync
	nop

/*****************************/
    cache   0x15, 0(t2)
/*****************************/
    nop
    nop
    nop
    nop

	addiu	t5, 8
	addi	t2, 8
		
	b	2b
	nop
	
3:	/* load the start address from the table and jump */
    move t0, a0
	ld	v0, 0(t0)

	jr	ra
	nop

END(boot1_5_l2_cache_load)
