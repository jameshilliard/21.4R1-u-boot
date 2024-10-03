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

#define KSEG0_OFFSET		0xffffffff80000000

/************************** 
L2 Loader helper functions
**************************/
	
LEAF(WRITE_L2_LDCBITS)

	move  	s0, a0			//a0=0x86000000/* This is PA of the memory */
	move    v1, ra
	dli 	t0, KSEG0_OFFSET	/* Convert the incoming address to Phyical address */
	sub     s0, t0			//s0=0x06000000
   	li 	s1, 5
	dsrl 	s2, s0, s1		//s2=s0>>5=0x300000  
#bank
    and s2, k0
//	andi 	s2, 7			//s2=s2&7   s2 bank(10.2.1)
					//PRINT("========== LDC LOCK =========\n")
					//PRINTF("Locking L2 Cache Loc   %x \n", s0)
					//PRINTF("Address      %x \n", s0)
					//PRINTF("Bank         %x \n", s2)
#set
    move s1, k1
//	li 	s1, 8         
	dsrl 	s3, s0, s1		//s3=s0>>8=0x60000
	andi 	s3, 0x3ff		//s3=0x60000&&0x3ff=0 s3 set(10.2.1)
					// PRINTF("Set          %x \n", s3)
#tag
	li 	s1, 15
	dsrl 	s4, s0, s1		//s4=s0>>15=0xc00
	li 	s1, 0x01ffffff
	and 	s4, s1			//s4=0xc00&0x1ffffff=0xc00 s4 tag(10.2.1)
					//PRINTF("Tag          %x \n", s4)
	dsll 	s1, s3, 22		//s1=s3<<22  tag="set in cmd register[31:22]"(10.2.3)
					//PRINTF("Set          %x \n", s1)
	move 	a0, s2			/* Bank    */ //
	li   	a1, 4			/* Reg Num */ //l2 cmd register=4(10.2.3)
	move 	a2, s1			/* val     */ //tag(set<<22) is the value to write
	/* Set up set for reading the Valid array */
	la      t0, WRITE_L2_CONFIG_R	//write set, cmd=0 is read valid array
	jalr    t0
					/* read value back -- valid bits will be stored in v0 */
	move	a0, s2			/* Bank */
	li 	a1, 20			/* Tag/Vld Array register 0x14 */
	la      t0, READ_L2_CONFIG_R
	jalr    t0
	move 	s0, v0			//read back tag/valid array register value in s0
					//PRINTF("1L2 Valid array     %x\n" , s0 )
	/* v0 has the valid array */
	/* Iterate through the way to identify the valid bits */
	li	s5, 7
L2ValidLDC:
					//PRINTF("way#     7-%x\n" , s5 )
 	li  	t2, 7			//check from lower way to high way 0-->7
	sub  	t2, s5			//t2=t2-t5 (s5=7,6,5,4,3,2,1,0)(t2=0,1,2,3,4,5,6,7)
	li  	t0, 3
	mul  	t2, t0			//t2=t2*3 t2 is the low way bit loc
	dsrl  	t2, s0, t2		//t2=s0>>t2    /* s0 contains valid array */
	andi  	t2, 7
	bnez  	t2, way_loopLDC		/* jump to next way if clean or dirty bits are set */
					/* AND value should be 3 instaed of 7, check this value */
	nop	
	/* We found a clean way(all 3 bit is 0)and we can lock it with tag  here */
	/* First lock the way and wirte the valid array back */

//2:	/* this part do cmd 1 write lock bit */
	li 	t3, 7
	sub 	t3, s5			//t3 is the way#
	move    t5, s0			/* restore the valid array for setting the lock bit */
	li 	t4, 3
	mul 	t4, t3			//t4=t3*3 point to way low bit loc
					/* dsrl    t5 ,t4 */	/* t5 contains the valid array */ 
	dsrl    t5, t5, t4		//t5=t5>>t4  get way 3bit/* t5 contains the valid array */ 
	andi 	t5, 7
	ori 	t5, 6			/* Set lock bit LDC bit here sliu lock=4*/  //KAZA:
# way bits
					/* dsll 	t3, 3 */
	dsll 	t3, t3, 3		//t3=t3<<3
	or      t5, t3			//t5=way(t3)+3way bit(t5)
# set bits
	move    t7, s3			//s3 is set /* use the set bits from the top */
					/* dsll 	t7 , 6 */
	dsll 	t7, t7, 6		//t7=set<<6
	or   	t5, t7			//t5=set<<6+way<<3+3bits
					/* dsll 	t5, 16  */
	dsll 	t5, t5, 16		//t5=(set<<6+way<<3+3bits)<<16 (10.2.3)
	ori 	t5, 1			//cmd=1(table 10-2) /* Write valid array command value */
	/* Set the paramenters for the subroutine */
	move    a0, s2			/* Bank    */
	li      a1, 4			/* Reg Num */
	move    a2, t5			/* Value for the Writting valid array  command with lock bit set */
	la      t0, WRITE_L2_CONFIG_R
	jalr    t0
	nop
	nop
	/* Set up set for reading the Valid array */
	dsll    s1, s3, 22		//s1=s3(set)<<22
					//PRINTF("Set              %x \n", s1)
	move    a0, s2			/* Bank    */
	li      a1, 4			/* Reg Num */
	move    a2, s1			/* val     */
	la      t0, WRITE_L2_CONFIG_R
	jalr    t0
					/* read value back -- valid bits will be stored in v0 */
	move    a0, s2			/* Bank            */
	li      a1, 20			/* Register number */
	la      t0, READ_L2_CONFIG_R
	jalr    t0
	move    s0, v0
					//PRINTF("way#     7-%x\n" , s5 )
					//PRINTF("2L2 Valid array   %x\n" , s0 )
	/* this part to write tag */
	/* We found a clean way and we can lock it with tag  here */
	dsll 	t3, s3, 22		//t3=set(s3)<<22
	li 	t2, 7
	sub 	t2, s5			//s5 is the 76543210 counter, t2 is way 
					/* dsll 	t2 , 19 */
	dsll 	t2, t2, 19		//way<<19
	or 	t3, t2			//set+way
	ori 	t3, 3			//set+way+cmd3/* cmd3:Write Tag ram value */
	move    a3, t3			//value
	/* First writet the Tag ram to data register at 0x14 */
	/* set the a0, a1 and a2 with proper input paramenters */
//i need to read this part more sliu:	
					/* dsll    s4, 7 */
	dsll    t6, s4, 7		//t6=tag(s4)<<7 (10.2.7 2nd table)
					//tag acts as address bit[31:7]
	move    a0, s2			/* Bank    */
	li      a1, 20			/* Reg Num */
	move    a2, t6			/* Value for the Write Tag ram command */
	la      t0, WRITE_L2_CONFIG_R
	jalr    t0
	/* Now send the command " Write Tag Ram command" */
	/* set the a0, a1 and a2 with proper input paramenters */
	move    a0, s2			/* Bank    */
	li      a1, 4			/* Reg Num */
	move 	a2, a3			/* Value for the Write Tag ram command */
	la      t0, WRITE_L2_CONFIG_R
	jalr    t0
	nop
	/* Read  the tag and display for the testing purpose*/
	b       return_mainLDC
	nop 
way_loopLDC:
	ori     t0, $0, 0		//t0=0?
	addi    s5, -1			//t5 counter --, from 7 to 0
	bge     s5, t0, L2ValidLDC	/* We did not find the clean way so */
					//if s5>0    /* go back and check the next way   */
	nop
					//b       3f
	nop
	/* We have completed the one cycle for one line size */
	/* So return to the calling function */
return_mainLDC:
	move	ra, v1
	jr	ra 
	nop

END(WRITE_L2_LDCBITS)

/******************************/

LEAF(CHECK_LOCK)
	move  	s0, a0			/* s0 has PA of the memory */
	move    v1, ra
	dli 	t0, KSEG0_OFFSET	/* Convert the incoming address to Physical address */
	sub     s0, t0			//s0=s0-0x80000000, phy addr
   	li 	s1, 5
	dsrl 	s2, s0, s1		//s2=s0<<5
#bank
    and s2, k0
//	andi 	s2, 7			//s2=(s0<<5)&7 bank
					//PRINT("============ CHECK_LOCK ===========\n")
					//PRINTF("Checking L2 Cache Loc   %x \n", s0)
					//PRINTF("Address      %x \n", s0)
					//PRINTF("Bank         %x \n", s2)
#set
    move s1, k1
//	li 	s1, 8 
	dsrl 	s3, s0, s1		//s3=s0>>8 set
	andi 	s3, 0x3ff		//s3=(s0>>8)&0x3ff set
					//PRINTF("Set          %x \n", s3)
#tag
	li 	s1, 15           
	dsrl 	s4, s0, s1   
	li 	s1, 0x01ffffff		//s4=(s0>>15)&0x1ffffff tag
	and 	s4, s1
					//PRINTF("Tag          %x \n", s4)
	dsll 	s1, s3, 22		//s1=s3<<22 set<<22
					//PRINTF("Set          %x \n", s1)
	move    a0, s2			/* Bank    */
	li      a1, 4			/* Reg Num */
	move    a2, s1			/* val     */ //cmd=0
	/* Set up set for reading the Valid array */
	la      t0, WRITE_L2_CONFIG_R	//prep read valid array
	jalr    t0
					/* read value back -- valid bits will be stored in v0 */
	move    a0, s2			/* Bank            */
	li      a1, 20			/* Register number */
	la      t0, READ_L2_CONFIG_R    //read valid array
	jalr    t0
	move    s0, v0                  //return array in s0
					//PRINTF("3L2 Valid array     %x\n" , v0 )
	li 	s5, 7			//s5=7 counter from 7->0
WayLoop:
	li      t2, 7     
	sub     t2, s5			//t2=7-s5  way counter from 0->7
	li      t0, 3
	mul     t2, t0
	dsrl    t2, s0, t2		/* s0 contains valid array */ //t2=s0>>(3t2)
	andi    t2, 7			//t2 get way lcd 
	beqz    t2, 1f			/* jump to next way if clean or dirty bits are not set */
					/* AND value should be 3 instaed of 7, check this value */
	nop
//	andi 	t2, 3			//jump to next way if c/d bit not set 
//	beqz    t2, 1f
	li      t2, 7 
	sub  	t2, s5			//t2=7-s5 way counter from 0->7
					/* dsll 	t2 , 19 */
	dsll 	t2, t2 , 19  //t2=t2<<19 way
	or 	t2, s1			/* s1 has the set */ //t2=t2(way<<19)+s1(set<<22)
	ori 	t2, 2			/* Read Tag ram command value *///cmd=2
	move 	a0, s2			/* Bank    */
	li   	a1, 4			/* Reg Num */
	move 	a2, t2			/* val     */
	/* Set up  for reading the Tag Ram */
	la      t0, WRITE_L2_CONFIG_R
	jalr    t0
					/* read value back -- tag ram will be stored in v0 */
	move	a0, s2			/* Bank            */
	li 	a1, 20			/* Register number */

	la      t0, READ_L2_CONFIG_R
	jalr    t0
					//move 	s0, v0
					//PRINTF("Tag Ram      %x\n" , v0 )
	/* read tag and see what can be done */
					//PRINTF("way#     %x\n" , 7-s5 )
	move    t3, v0			/* T3 has the tag ram value */
	li  	t4 , 0x01ffffff
					/* dsrl    t3, 7 */
	dsrl    t3, t3, 7		//t3>>7
	and 	t3, t4			//t3&0x1ffffff
	beq 	t3, s4, 2f		//((t3>>7)&0x1ffffff)==s4(tag), goto label 2
	nop
1:
	ori	t0, $0, 0		//t0=0
	addi 	s5, -1			//s5=s5-1, counter from 7->0
	bge 	s5, t0,			WayLoop	/* We did not find the valid bit so */
					/* go back and check the next way */
					//s5>=0, goto wayloop, check next way
	nop
	PRINTF("*************************************LOCK TAG MISSING %x \n", v0 )
	li 	v0 , 0
	b 	3f
	nop
2:	/* We did find the valid bit  and valid tag ram and so we can lock it the cache */
					//PRINTF("TAG RAM  matches %x\n" , v0 )
	li	v0, 1
3:
	/* We have completed the one cycle for one line size */
	/* So return to the calling function */
	move	ra, v1
	jr	ra 

END(CHECK_LOCK)

/*
 Write L2 Configuration
 Usage: WRITE_L2_CONFIG(regbank, register, regvalue, tempreg1, tempreg2)
*/

LEAF(WRITE_L2_CONFIG_R)
/*
	my $bank = a0
	my $reg  = a1
	my $val  = a2
*/
	move    t0, a0
	move    t1, a1
	move    t2, a2

	move    a0, ra 

	li      t3, BRIDGE_PHNX_IO_RESET_VADDR + PHOENIX_IO_L2_OFFSET
	dsll 	t4, t0, 5 //t4=bank<<5 (10.2.1 l2 config addr form)

	daddu 	t3, t3, t4	            //t3=t3+t4(base+bank)  
	daddu 	t3 ,t3, t1	            //t3=t3+t1(base+bank+reg#)

	sw 	t2, 0(t3)                   //save t2(tag=set<<22) to base+bank+reg#
	nop
	sync


	
	move    ra, a0

	jr 	ra	
    
END(WRITE_L2_CONFIG_R)

/*
 Read L2 Configuration
 Usage: READ_L2_CONFIG(regbank, register, tempreg, retreg)
*/

LEAF(READ_L2_CONFIG_R)

/*
	my $bank = a0
	my $reg  = a1 
*/
	move    t0, a0
	move    t1, a1

	move    a0, ra 

	li      t3, BRIDGE_PHNX_IO_RESET_VADDR + PHOENIX_IO_L2_OFFSET
	
	dsll 	t4, t0, 5 
	
	daddu 	t3, t3, t4 	
	daddu  	t3, t3, t1 
	
	lw  	t6 , 0(t3)		
	nop
	sync
	move 	v0, t6 		/* v0 has the return value */
	
	move    ra, a0
	
	jr 	ra	

END(READ_L2_CONFIG_R)

LEAF(invalidate_l2_cache)

	move	v1, ra
	
	li	s0, 7			/* number of banks */
	li	s1, 0x3ff		/* number of sets  */
	li	s2, 7			/* number of ways  */
	
bank_ldc:
set_ldc:
way_ldc:
	//write reg 4 with set[31:22]+way[21:19]+ldc[18:16]+cmd[3:0]
	dsll	t1, s1, 22
	dsll	t2, s2, 19
	or	t1, t2
	ori	t1, 1			/* cmd=1, wr ldc */
	
	move	a0, s0			/* bank  */
	li	a1, 0x04		/* reg 4 */
	move	a2, t1			/* value wr to reg 4  */
	la	t0, WRITE_L2_CONFIG_R	/* use t0,t1,t2,t3,t4 */
	jalr	t0
	nop
	
	ori	t0, $0, 0
	addi	s2, -1
	bge	s2, t0, way_ldc         /* loop back on ways */
	nop
	li	s2, 7			/* reset way */
	ori	t0, $0, 0
	addi	s1, -1
	bge	s1, t0, set_ldc         /* loop back on sets */
	nop
	li	s1, 0x3ff		/* reset set */
	ori	t0, $0, 0
	addi	s0, -1
	bge	s0, t0, bank_ldc        /* loop back on banks */
	nop
	
	move	ra, v1
	jr	ra
	nop
	
END(invalidate_l2_cache)

