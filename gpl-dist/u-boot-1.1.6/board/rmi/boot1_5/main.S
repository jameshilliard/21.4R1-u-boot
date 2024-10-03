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
	
#include "rmi/boardconfig.h"
#include "rmi/bridge.h"
#include "rmi/on_chip.h"
#include "rmi/xlr_cpu.h"
#include "rmi/gpio.h"
#include "rmi/cache.h"
#include "rmi/shared_structs_offsets.h"

#ifdef VERBOSE0
#define DBG(s)
#define DBGF(s,arg)
#else
#define DBG(s) PRINT(s)
#define DBGF(s,arg) PRINTF(s,arg)
#endif	
	.text

	.set push

EXPORT(main)
	/* Enable AERR collection */
	jal aerr_log
	nop
	
	/* Initialize UART */
	la	t0, uart_init
	jalr	t0
	nop
	DBG("\n[UBOOT BOOTING STAGE 1]\n")
    
    /* SW Workaround for the Hardfuse
     * Erratum for the XLS404
     */
    jal xls_404_softfuse
    nop
    
    /* SW Workaround for the Hardfuse
     * Erratum for the XLS408
     */
    jal xls_408_softfuse
    nop

	/* Get the BIST Stats */
	jal	gpio_dump_bist	
	nop

	jal	enable_l1_icache
	nop

	jal	enable_l1_dcache
	nop

	jal     enable_l2_cache_no_ecc
	nop

	jal     enable_cpu_l2_cache
	nop
	sync

#ifndef PHOENIX_SIM
	jal	invalidate_l2_cache
	nop
#endif
	
1:	
	jal	boot1_5
	nop

EXPORT(main_boot2)

	/* enabling ecc for l2 cache */
	jal	enable_l2_cache
	nop
		
	/* initialize and update cpu online map */
	jal	boot2_update_cpu_online_map
	nop

	/* slave cpus will be in reset till this point */
	jal	gpio_wakeup_slave_cpus
	nop
    
	/* enable all threads in this core */
	jal	enable_all_threads
	nop

	jal	boot2_update_psb_info
	nop

#if defined(CFG_BOARD_RMIREF) || defined(CFG_TEMPLATE_MACADDR_OFFSET)
    jal	read_i2c_macaddr
	nop
	la	gp, g_boot1_info
	sd	v1, boot1_info_mac_addr_off(gp)
#endif

#ifdef CFG_BOARD_RMIREF

	/* Major Board version */
	jal	read_i2c_boardver_major
	nop
    
	la	gp, g_boot1_info
	sd	v1, boot1_info_board_major_version_off(gp)

	DBGF("ARIZONA Board Major Version: [%X]\n", v1)
	
	/* Minor Board version */
	jal	read_i2c_boardver_minor
	nop
	la	gp, g_boot1_info
	sd	v1, boot1_info_board_minor_version_off(gp)

	DBGF("ARIZONA Board Minor Version: [%X]\n", v1)

	/* Board manf version char0*/
	jal	read_i2c_boardrev_char0
	nop
        move v0, v1
	la	gp, g_boot1_info
	sb	v1, (boot1_info_board_manf_revision_off+6)(gp)

	/* Board manf version char1*/
	jal	read_i2c_boardrev_char1
	nop
	la	gp, g_boot1_info
	sb	v1, (boot1_info_board_manf_revision_off+7)(gp)

	/* Board serial num hi*/
	jal	read_i2c_board_serialnum_0
	nop
	la	gp, g_boot1_info
	sb	v1, (boot1_info_board_serial_number_off+6)(gp)

	/* Board manf version char1*/
	jal	read_i2c_board_serialnum_1
	nop
	la	gp, g_boot1_info
	sb	v1, (boot1_info_board_serial_number_off+7)(gp)

#endif /* CFG_BOARD_RMIREF */
    
	jal	flush_l1_caches

	nop
	jal	boot2_clear_bev
	nop
	
	la	gp, g_boot1_info
	/* Copy boot1_info here to BOOT1_INFO address */
	li	t0, BOOT1_INFO
	li	t2, boot1_info_size
7: 	ld	t3, 0(gp)
	sd	t3, 0(t0)
	addiu	gp, 8
	addiu	t0, 8
	addi	t2, -8
	bgtz	t2, 7b
	nop


	la	gp, g_boot1_info
	/* DBGF("boot_info @ %x\n", gp) */
	ld	t0, boot1_info_cpu_online_map_off(gp) 
	/* DBGF("cpu_online_map %x\n", t0) */
	
	/* Load boot2 from flash to dram
	 * Assume, boot2 addresses are in the lower 256MB
	 * which has DRAM mapped
	 */
	la	t5, FLASH_BOOT2_START_ADDR
	la	t7, BOOT2_LOADADDR
	la	t6, FLASH_BOOT2_END_ADDR
	DBGF("Copying BOOT2 from 0x%x ",t5)
	DBGF("to 0x%x\n",t7)
	DBGF("Till 0x%x\n",t6)
	subu	s8, t6, t5
2:			
	lw	t8, 0(t5)
	sw	t8, 0(t7)
	addiu	t5, 4
	addiu	t7, 4
	
	andi	t9, t5, 0x7fff
	bnez	t9, 3f
	DBG(".")
3:		
	sltu	t9, t5, t6
        bnez    t9, 2b
        nop

	DBG("Done.\n")
	DBGF("Copied 0x%x Bytes.\n",s8)
#if 0
	DBGF(" 0x%x",t5)	
	DBGF(" 0x%x",t7)
	DBG("\n")
#endif
	la	gp, BOOT1_INFO
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

	/* Jump to boot2 */
	la	s8, BOOT2_ENTRY

	DBGF("\nBOOTING STAGE2 @ [0x%x]\n", s8) 
	jr	s8
	nop
9:	/* Should never get here */
	jal	halt
	nop

EXPORT(nand_boot2)

    jal     boot2_set_bev
    nop

    jal     boot2_clear_bev
    nop

    jal     invalidate_l2_cache
    nop

	/* enabling ecc for l2 cache */
	jal	enable_l2_cache
	nop
		
	/* initialize and update cpu online map */
	jal	boot2_update_cpu_online_map
	nop
	
	/* slave cpus will be in reset till this point */
	jal	gpio_wakeup_slave_cpus
	nop

	/* slave cpus will be in reset till this point */
	/* enable all threads in this core */
	jal	enable_all_threads
	nop
	
	jal	boot2_update_psb_info
	nop

    jal read_i2c_boardver
    nop

    la  gp, g_boot1_info
    sd  v1, boot1_info_board_version_off(gp)

    jal	read_i2c_macaddr
    nop

    la	gp, g_boot1_info
    sd	v1, boot1_info_mac_addr_off(gp)

    /* Major Board version */
    jal	read_i2c_boardver_major
    nop

    la	gp, g_boot1_info
	sd	v1, boot1_info_board_major_version_off(gp)

    DBGF("ARIZONA Board Major Version: [%X]\n", v1)
	
	/* Minor Board version */
	jal	read_i2c_boardver_minor
	nop
	la	gp, g_boot1_info
	sd	v1, boot1_info_board_minor_version_off(gp)

    DBGF("ARIZONA Board Minor Version: [%X]\n", v1)

	/* Board manf version char0*/
	jal	read_i2c_boardrev_char0
	nop
        move v0, v1
	la	gp, g_boot1_info
	sb	v1, (boot1_info_board_manf_revision_off+6)(gp)

	/* Board manf version char1*/
	jal	read_i2c_boardrev_char1
	nop
	la	gp, g_boot1_info
	sb	v1, (boot1_info_board_manf_revision_off+7)(gp)

	/* Board serial num hi*/
	jal	read_i2c_board_serialnum_0
	nop
	la	gp, g_boot1_info
	sb	v1, (boot1_info_board_serial_number_off+6)(gp)

	/* Board manf version char1*/
	jal	read_i2c_board_serialnum_1
	nop
	la	gp, g_boot1_info
	sb	v1, (boot1_info_board_serial_number_off+7)(gp)

	jal	boot2_clear_bev
	nop

	la	gp, g_boot1_info
	/* Copy boot1_info here to BOOT1_INFO address */
	li	t0, BOOT1_INFO
	li	t2, boot1_info_size
7: 	ld	t3, 0(gp)
	sd	t3, 0(t0)
	addiu	gp, 8
	addiu	t0, 8
	addi	t2, -8
	bgtz	t2, 7b
	nop
	
	jal	flush_l1_caches
	nop

	li	gp, BOOT1_INFO
	/* DBGF("boot_info @ %x\n", gp) */
	ld	t0, boot1_info_cpu_online_map_off(gp) 
	/* DBGF("cpu_online_map %x\n", t0) */
	
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


	/* Jump to boot2 */
	la	s8, BOOT2_ENTRY

    DBGF("\nBOOTING STAGE2 @ [0x%x]\n", s8)
	jr	s8
	nop
9:	/* Should never get here */
	jal	halt
	nop


EXPORT(main_boot1_75)
	/* decide that this is an xls with nand chip boot option*/
	mfc0 t0, $15
	srl  t0,8
	andi t0, 0xff
	beq  t0, 0x80, xls_nand_check /*XLS608*/
	nop
	beq  t0, 0x88, xls_nand_check /*XLS408*/
	nop
	beq  t0, 0x8c, xls_nand_check /*XLS404*/
	nop
	beq  t0, 0x8e, xls_nand_check /*XLS208*/
	nop
	beq  t0, 0x8f, xls_nand_check /*XLS204*/
	nop

	beq  t0, 0x40, xls_nand_check /*XLS616 B0*/
	nop
	beq  t0, 0x4a, xls_nand_check /*XLS608 B0*/
	nop
	beq  t0, 0x44, xls_nand_check /*XLS416 B0*/
	nop
	beq  t0, 0x4c, xls_nand_check /*XLS412 B0*/
	nop
	beq  t0, 0x4e, xls_nand_check /*XLS408 B0*/
	nop
	beq  t0, 0x4f, xls_nand_check /*XLS404 B0*/
	nop


	dmfc0	t0, $22, 2
	nop
	nop
	bne	t0, 1, no_nor_backup
	nop
	dmtc0   t0, $22, 3
	b	xls_nand_present
	nop

	j    main_boot2
	nop

xls_nand_check:
	/* DBG("XLS present\n") */
	la   t0, 0xbef18000
	lw   t1, 0x54(t0) 
	srl  t1,14
	andi t1, 1
	beq  t1, 1, xls_nand_present
	nop
	dmfc0	t0, $22, 2
	nop
	nop
	bne	t0, 1, no_nor_backup
	nop
	dmtc0   t0, $22, 3
	b	xls_nand_present
	nop
no_nor_backup:
	j    main_boot2
	nop

xls_nand_present:

	/* enabling ecc for l2 cache */
	jal enable_l2_cache
	nop
	
	/* initialize and update cpu online map */
	jal	boot2_update_cpu_online_map
	nop
	
	jal	boot2_update_psb_info
	nop

#if defined(CFG_BOARD_RMIREF) || defined(CFG_TEMPLATE_MACADDR_OFFSET)

    jal	read_i2c_macaddr
	nop
		
	la	gp, g_boot1_info
	sd	v1, boot1_info_mac_addr_off(gp)

    DBGF("\nMAC Address from EEPROM %X\n", v1)
#endif

#ifdef CFG_BOARD_RMIREF
	
    jal read_i2c_boardver
	nop

    la  gp, g_boot1_info
    sd  v1, boot1_info_board_version_off(gp)

	/* Major Board version */
	jal	read_i2c_boardver_major
	nop
    
	la	gp, g_boot1_info
	sd	v1, boot1_info_board_major_version_off(gp)

    DBGF("ARIZONA Board Major Version: [%X]\n", v1)
	
	/* Minor Board version */
	jal	read_i2c_boardver_minor
	nop
	la	gp, g_boot1_info
	sd	v1, boot1_info_board_minor_version_off(gp)

    DBGF("ARIZONA Board Minor Version: [%X]\n", v1)

	/* Board manf version char0*/
	jal	read_i2c_boardrev_char0
	nop
        move v0, v1
	la	gp, g_boot1_info
	sb	v1, (boot1_info_board_manf_revision_off+6)(gp)

	/* Board manf version char1*/
	jal	read_i2c_boardrev_char1
	nop
	la	gp, g_boot1_info
	sb	v1, (boot1_info_board_manf_revision_off+7)(gp)

	/* Board serial num hi*/
	jal	read_i2c_board_serialnum_0
	nop
	la	gp, g_boot1_info
	sb	v1, (boot1_info_board_serial_number_off+6)(gp)

	/* Board manf version char1*/
	jal	read_i2c_board_serialnum_1
	nop
	la	gp, g_boot1_info
	sb	v1, (boot1_info_board_serial_number_off+7)(gp)

#endif  /* CFG_BOARD_RMIREF */

	jal	boot2_clear_bev
	nop
	
	la	t2, __stack
	move	sp, t2


	li	t0, 0
	la	t1, _fdata
	la	t2, __stack
	move	sp, t2
	/* Lock 24 K of D-cache */
16:
	cache    Fetch_Lock_D, 0(t1)
	addiu	t1, 32
	addiu	t0, 1
	slti	t2, t0, 768 
	bnez    t2, 16b
	nop

    	la      t0, __bss_start
	la      t1, _bss_end
17:      sw      zero, 0(t0)
	addiu   t0, 4
	bne     t0, t1, 17b
	nop


	la      a3, nand_boot2
	dmtc0   a3, $22, 1

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

	/* Lock boot1_75 code in icache as flash will not be accesible linearly
	   untill we return to nand_boot2
	   */
	la      t0, _ntext
	la      t1, _netext
10:     cache   Fetch_Lock_I, 0(t0)
	addiu   t0, 32
	blt     t0, t1, 10b
	nop


	/* Jump to boot1_75 */
	la	s8, boot1_75_start

    DBGF("\nBOOTING STAGE1_75 @ [0x%x]\n", s8)
	jr	s8
	nop
9:	/* Should never get here */
	jal	halt
	nop
	.set	pop

