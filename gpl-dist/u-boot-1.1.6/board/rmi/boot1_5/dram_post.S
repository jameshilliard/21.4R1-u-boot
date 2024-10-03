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

#include "rmi/board_i2c.h"

#ifdef VERBOSE0
#define DBG(s)
#define DBGF(s,arg)
#else
#define DBG(s) PRINT(s)
#define DBGF(s,arg) PRINTF(s,arg)
#endif



LEAF(dram_post)
        
       /* Setup DRAM BARs */
	jal	dram_setup_bar
	nop

	jal	boot2_set_bev
	nop
	jal	flush_l1_caches
	nop
	jal	boot2_clear_bev
	nop
	jal	invalidate_l2_cache
	nop

       /* Size-based DRAM scrub prior to ECC enabling */
	la	t0, dram_scrub
	jalr	t0
	nop
	la	t0, dram_ecc_enable
	jalr	t0
	nop

    la      t0, execute_shell_temp
    jalr    t0
    nop

    ## soft init of eeprom

    jal  eeprom_init
	nop

    ## make eeprom ready for logging

    jal eeprom_log_init
	nop

	la      t0, check_swizzle
	jalr    t0
	nop

    /* Enable 64-bit addressing to peek/poke all of the 
       memory from the mini shell that runs from the
       cache
    */

    mfc0    t0, COP_0_STATUS
    ori     t0, 0x80
    mtc0    t0, COP_0_STATUS
    nop
        

#ifdef EEPROM_POST_LOG
#ifdef CONFIG_FX || CFG_BOARD_RMIREF 
       /* Read the eeprom mem test flag over I2C */
       li   a0,  I2C_EEPROM_ADDR
       li   a1,  MEM_TEST_FLAG_EEPROM_OFFSET
       li   a2,  I2C_EEPROM_BUS
       la   t0,  i2c_rx
       jalr t0
       nop
 
       /*  And the returned value of i2c_rx with 0x01 to decide
        *  on Min or Max memory test
        */
        andi t1,v1,0x01  /* Check if Max flag is set */
        bnez t1, 2f      /* If set go for max test */   
        nop        
#else
       /* If it post log has to go to some other device
          other than eeprom on i2c */
#endif 
#endif
 
1:
        /* Testing Dram with dram_mats */
        ## 4MB from where u-boot gets copied to RAM from flash 
        #li a1, 0x00400000 
        #DBGF("\nRunning  minimum DRAM MATS on %x bytes",a1);
        #li a1, 0x8c100000
        #DBGF("from start address %x\n",a0);
	
        li a1, 0x00400000 
        li a0, 0x8c100000
        jal dram_mats

        nop
        j 3f
        nop
          
2:       
        #jal dram_bar_mats
        #nop
        #b 2b
        #nop
3:
        la	t0, erase_dram_bar7
        jalr    t0
        nop

        jal dram_scrub
        nop
      
	jal	dram_setup_bar
	nop
        j	main_boot1_75
        nop
END(dram_post)	



LEAF(dram_scrub)

/* t0 - BAR address */
/* t1 - xkphys base virtual address */
/* t2 - contents of the BAR */
/* t3 - temporary register */
/* t4 - extracted mask bits, number of cache lines */		
/* t5 - extracted virtual address */
/* t6 - BAR read loop counter, BAR offset */
	
	/* Initialize variables */
	dli     t1, 0x9800000000000000	/* CONSTANT - move this later */
	li	t6, 0x20	        /* Start with BAR7 */

	/* Enable 64-bit addressing by activating KX */
	mfc0    t0, COP_0_STATUS
	ori     t0, 0x80
	mtc0    t0, COP_0_STATUS
		
1:	/* BAR read loop begin */
	addi	t6, -4          /* Point to the next BAR */
	bltz	t6, 3f          /* If no more BARs left, exit the loop */
	nop

        li      t0, (BRIDGE_PHNX_IO_REMAP_VADDR + PHOENIX_IO_BRIDGE_OFFSET)
	addu	t0, t0, t6	/* Calculate the address of the BAR */
	
        lw      t2, 0(t0)      /* Read the BAR */
	#dli     t3, 0x0000000000000001
	#and     t3, t2, t3     /* Extract the BAR enable bit */
	#beqz	t3, 1b	       /* If BAR is not enabled, skip to the next one */
	#nop
	dli	t3, 0x00000000ffff0000	
	and	t3, t2, t3     /* Extract the BAR base address field */
	dli	t4, 0x000000000000fff0	
	and	t4, t2, t4     /* Extract the BAR mask bits */
	
	dsll    t5, t4, 20     /* Make shifted copy of mask bits, aligned from 35 to 24 */
	nor	t5, t5, zero   /* Invert the shifted mask */
	dsll    t3, t3, 8      /* Move base address to bits 39 to 24 */	
	and     t5, t5, t3     /* Mask out unwanted base address bits */
	or	t5, t5, t1     /* Convert address to xkphys cached virtual address */
	
	dsrl	t4, t4, 4      /* Move mask to bits 11 to 0 */
	daddiu  t4, t4, 0x1    /* Add 1 */
	dsll    t4, t4, 24     /* Move results to bits 36 to 24 */ 
	dsrl    t4, t4, 5      /* Divide by 32 to get number of cache lines */

2:	/* Scrub loop begin */
	beqz	t4, 1b         /* If # of unscrubbed cache lines = 0, skip the scrubbing process */
	nop
	pref	Pref_Prep_For_Store, 0(t5)
	
	sd	zero, 0(t5)
	sd	zero, 8(t5)
	sd	zero, 16(t5)
	sd	zero, 24(t5)

	daddiu	t5, 32
	addiu	t4, -1
	b	2b
	nop
	/* Scrub loop end */
        /* BAR read loop end */
	
3:	/* Disable 64-bit addressing by clearing KX */
	li	t1, 0xffffff7f
	mfc0    t0, COP_0_STATUS
	and     t0, t0, t1
	mtc0    t0, COP_0_STATUS

	jr ra
	nop
END(dram_scrub)




/* Used to implement Max memory test running from 
   cache. Uses 64-bit addressing mode and XKPHYS.
*/
LEAF(dram_bar_mats)
/* t0 - used to enable/disable 64-bit addressing */
/* t1 - xkphys base virtual address */
/* t2 - contents of the BAR */
/* t3 - temporary register */
/* t4 - extracted mask bits, number of cache lines */		
/* t5 - extracted virtual address */
/* t6 - BAR read loop counter, BAR offset */
	
        /* Save the return address */
        move t7,ra
        
        #move   t6, a0
        li      t6, 0x20                /* Start with BAR7 */

	/* Enable 64-bit addressing by activating KX */
	mfc0    t0, COP_0_STATUS
	ori     t0, 0x80
	mtc0    t0, COP_0_STATUS
		
1:	/* BAR read loop begin */
        
	/* Initialize t1 to XKPHY base as t1 would have got modified during
           function calls */
	dli     t1, 0x9800000000000000	/* CONSTANT - move this later */
        
        addi    t6, -4          /* Point to the next BAR */
        bltz    t6, 3f          /* If no more BARs left, exit the loop */
        nop
	#move	t6,a0           /* Point to the BAR indicated by offset in a0 */

        li      t0, (BRIDGE_PHNX_IO_REMAP_VADDR + PHOENIX_IO_BRIDGE_OFFSET)
	addu	t0, t0, t6	/* Calculate the address of the BAR */
	
        lw      t2, 0(t0)      /* Read the BAR */
	dli     t3, 0x0000000000000001
	and     t3, t2, t3     /* Extract the BAR enable bit */
	beqz	t3, 1b	       /* If BAR is not enabled, skip to next bar */
	nop
	dli	t3, 0x00000000ffff0000	
	and	t3, t2, t3     /* Extract the BAR base address field */

	dli	t4, 0x000000000000fff0	
	and	t4, t2, t4     /* Extract the BAR mask bits */

	dsll    t5, t4, 20     /* Make shifted copy of mask bits, aligned from 35 to 24 */
	nor	t5, t5, zero   /* Invert the shifted mask */
	dsll    t3, t3, 8      /* Move base address to bits 39 to 24 */	
	and     t5, t5, t3     /* Mask out unwanted base address bits */
	or	t5, t5, t1     /* Convert address to xkphys cached virtual address */

	dsrl	t4, t4, 4      /* Move mask to bits 11 to 0 */
	daddiu  t4, t4, 0x1    /* Add 1 */
	dsll    t4, t4, 24     /* Move results to bits 36 to 24 */ 
	dsrl    t4, t4, 5      /* Divide by 32 to get number of cache lines */


        move a0, t5   # physical base address 
        move a1, t4   # size
        dsll a1,a1,5  # multiply by 32

        
        jal dram_mats
        nop
	
        b 1b /* Jump back for next bar */
        nop
 
3:	/* Disable 64-bit addressing by clearing KX */
	li	t1, 0xffffff7f
	mfc0    t0, COP_0_STATUS
	and     t0, t0, t1
	mtc0    t0, COP_0_STATUS

        move ra,t7
	 
        jr ra
	nop
	
END(dram_bar_mats)



LEAF(dram_ecc_enable)

	/* enable ecc for dram channels */
	/* ------------------------------------------------------------- */
        /* Note: this function uses the bridge scratch register 1 as an  */
	/* input and assumes that it has been programmed by the SPD code */ 
	/* (not to be confused with OS scratch register 0 )              */
	/* Format: [0]: Set if channel 0 supports ECC                    */
	/*         [1]: Set if channel 1 supports ECC                    */
	/*         [2]: Set if channel 2 supports ECC                    */
	/*         [3]: Set if channel 3 supports ECC                    */
        /* ------------------------------------------------------------- */
		
    /* Activating ECC0 */
    li      t0, (BRIDGE_PHNX_IO_REMAP_VADDR + PHOENIX_IO_BRIDGE_OFFSET)
    lw      t1, (BRIDGE_SCRATCH1 << 2)(t0)
	andi    t1, t1, 0x1
	beqz    t1, 1f      /* skip this channel if it does not support ECC */
	nop
	li	t0, (BRIDGE_PHNX_IO_REMAP_VADDR + PHOENIX_IO_DDR2_CHN0_OFFSET)
	lw	t1, (DDR2_CONFIG_REG_GLB_PARAMS <<2)(t0)
	ori	t1, 0x820
	sw	t1, (DDR2_CONFIG_REG_GLB_PARAMS <<2)(t0)

1:		
	/* Activating ECC1 */
        li      t0, (BRIDGE_PHNX_IO_REMAP_VADDR + PHOENIX_IO_BRIDGE_OFFSET)
        lw      t1, (BRIDGE_SCRATCH1 << 2)(t0)
	andi    t1, t1, 0x2
	beqz    t1, 2f      /* skip this channel if it does not support ECC */
	nop
	li	t0, (BRIDGE_PHNX_IO_REMAP_VADDR + PHOENIX_IO_DDR2_CHN1_OFFSET)
	lw	t1, (DDR2_CONFIG_REG_GLB_PARAMS <<2)(t0)
	ori	t1, 0x820
	sw	t1, (DDR2_CONFIG_REG_GLB_PARAMS <<2)(t0)

2:		
    li      t0, (BRIDGE_PHNX_IO_REMAP_VADDR + PHOENIX_IO_BRIDGE_OFFSET)
    lw      t1, (BRIDGE_SCRATCH1 << 2)(t0)
	andi    t1, t1, 0x4
	beqz    t1, 3f      /* skip this channel if it does not support ECC */
	nop
	li	t0, (BRIDGE_PHNX_IO_REMAP_VADDR + PHOENIX_IO_DDR2_CHN2_OFFSET)
	lw	t1, (DDR2_CONFIG_REG_GLB_PARAMS <<2)(t0)
	ori	t1, 0x820
	sw	t1, (DDR2_CONFIG_REG_GLB_PARAMS <<2)(t0)

3:		
    li      t0, (BRIDGE_PHNX_IO_REMAP_VADDR + PHOENIX_IO_BRIDGE_OFFSET)
    lw      t1, (BRIDGE_SCRATCH1 << 2)(t0)
    andi    t1, t1, 0x8
	beqz    t1, 4f      /* skip this channel if it does not support ECC */
	nop
	li	t0, (BRIDGE_PHNX_IO_REMAP_VADDR + PHOENIX_IO_DDR2_CHN3_OFFSET)
	lw	t1, (DDR2_CONFIG_REG_GLB_PARAMS <<2)(t0)
	ori	t1, 0x820
	sw	t1, (DDR2_CONFIG_REG_GLB_PARAMS <<2)(t0)	
	
4:	jr ra
	nop
	
END(dram_ecc_enable)

LEAF(dram_setup_bar)

	/* ------------------------------------------------------------- */
        /* Note: this function uses the bridge scratch register 1 as an  */
	/* input and assumes that it has been programmed by the SPD code */ 
	/* (not to be confused with OS scratch register 1 )              */
	/* Format: [4]: Set if DRAM BAR 0 should be enabled              */
	/*         [5]:	Set if DRAM BAR 1 should be enabled              */
	/*         [6]:	Set if DRAM BAR 2 should be enabled              */
	/*         [7]:	Set if DRAM BAR 3 should be enabled              */
	/*         [8]:	Set if DRAM BAR 4 should be enabled              */
	/*         [9]:	Set if DRAM BAR 5 should be enabled              */
	/*        [10]:	Set if DRAM BAR 6 should be enabled              */
	/*        [11]:	Set if DRAM BAR 7 should be enabled              */ 
        /* ------------------------------------------------------------- */

        li      t0, (BRIDGE_PHNX_IO_REMAP_VADDR + PHOENIX_IO_BRIDGE_OFFSET)
	
	/* Enable DRAM BAR 0, if required */
        lw      t1, (BRIDGE_SCRATCH1 << 2)(t0)
	andi    t1, t1, 0x10
	beqz    t1, 1f      /* skip this BAR if it does not need to be activated */
	nop
        lw      t1, (BRIDGE_DRAM_0_BAR << 2)(t0)
	ori	t1, 0x1
        sw      t1, (BRIDGE_DRAM_0_BAR << 2)(t0)
	
1:	/* Enable DRAM BAR 1, if required */
        lw      t1, (BRIDGE_SCRATCH1 << 2)(t0)
	andi    t1, t1, 0x20
	beqz    t1, 2f      /* skip this BAR if it does not need to be activated */
	nop
        lw      t1, (BRIDGE_DRAM_1_BAR << 2)(t0)
	ori	t1, 0x1
        sw      t1, (BRIDGE_DRAM_1_BAR << 2)(t0)
	
2:	/* Enable DRAM BAR 2, if required */
        lw      t1, (BRIDGE_SCRATCH1 << 2)(t0)
	andi    t1, t1, 0x40
	beqz    t1, 3f      /* skip this BAR if it does not need to be activated */
	nop
        lw      t1, (BRIDGE_DRAM_2_BAR << 2)(t0)
	ori	t1, 0x1
        sw      t1, (BRIDGE_DRAM_2_BAR << 2)(t0)

3:	/* Enable DRAM BAR 3, if required */
        lw      t1, (BRIDGE_SCRATCH1 << 2)(t0)
	andi    t1, t1, 0x80
	beqz    t1, 4f      /* skip this BAR if it does not need to be activated */
	nop
        lw      t1, (BRIDGE_DRAM_3_BAR << 2)(t0)
	ori	t1, 0x1
        sw      t1, (BRIDGE_DRAM_3_BAR << 2)(t0)

4:	/* Enable DRAM BAR 4, if required */
        lw      t1, (BRIDGE_SCRATCH1 << 2)(t0)
	andi    t1, t1, 0x100
	beqz    t1, 5f      /* skip this BAR if it does not need to be activated */
	nop
        lw      t1, (BRIDGE_DRAM_4_BAR << 2)(t0)
	ori	t1, 0x1
        sw      t1, (BRIDGE_DRAM_4_BAR << 2)(t0)

5:	/* Enable DRAM BAR 5, if required */
        lw      t1, (BRIDGE_SCRATCH1 << 2)(t0)
	andi    t1, t1, 0x200
	beqz    t1, 6f      /* skip this BAR if it does not need to be activated */
	nop
        lw      t1, (BRIDGE_DRAM_5_BAR << 2)(t0)
	ori	t1, 0x1
        sw      t1, (BRIDGE_DRAM_5_BAR << 2)(t0)

6:	/* Enable DRAM BAR 6, if required */
        lw      t1, (BRIDGE_SCRATCH1 << 2)(t0)
	andi    t1, t1, 0x400
	beqz    t1, 7f      /* skip this BAR if it does not need to be activated */
	nop
        lw      t1, (BRIDGE_DRAM_6_BAR << 2)(t0)
	ori	t1, 0x1
        sw      t1, (BRIDGE_DRAM_6_BAR << 2)(t0)
	
7:	/* Enable DRAM BAR 7, if required */
        lw      t1, (BRIDGE_SCRATCH1 << 2)(t0)
	andi    t1, t1, 0x800
	beqz    t1, 8f      /* skip this BAR if it does not need to be activated */
	nop
        lw      t1, (BRIDGE_DRAM_7_BAR << 2)(t0)
	ori	t1, 0x1
        sw      t1, (BRIDGE_DRAM_7_BAR << 2)(t0)

8:	jr ra
	nop
	
END(dram_setup_bar)

LEAF(erase_dram_bar7)

        li      t0, (BRIDGE_PHNX_IO_REMAP_VADDR + PHOENIX_IO_BRIDGE_OFFSET)
        li      t1, 0x0
        sw      t1, (BRIDGE_DRAM_7_BAR << 2)(t0)

	jr ra
	nop
	
END(erase_dram_bar7)

	
