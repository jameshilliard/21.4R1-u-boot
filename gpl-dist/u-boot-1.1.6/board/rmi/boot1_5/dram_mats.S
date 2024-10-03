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



//#define VERBOSED

#ifdef VERBOSE0
#define DBG(s)
#define DBGF(s,arg)
#else
#define DBG(s) PRINT(s)
#define DBGF(s,arg) PRINTF(s,arg)
#endif	


//#define POST_LOG

/*******************************************************
   s1 <- return address
   s7 <- size of the bar
   s0 <- 8 byte aligned address
   s2 <- final 8 byte aligned address
   s5 <- start address copy
   s6 <- End address copy

********************************************************/
	
LEAF(dram_mats)
	
        ### save return address
	move	s1,	ra
	move	s7,	a1
	## mask out lower three bits of start address
	dsrl	s0,	a0,	3
	dsll	s0,	3
	
	daddu	s2,	s0,	a1	#final address+1
	daddi	s2,	-1
	dsrl	s2,	3
	dsll	s2,	3

	## copies of address
	move	s5,	s0	
	move	s6,	s2
	
	nor	s3,	zero,	zero    #all 1s
    
/* debug statements */
#if 0 
        move a1, ra 
	DBGF("\nRAM BIST Called with start_addr %x\n", s5)
	DBGF("RAM BIST Called with size %x\n", s7)
	DBGF("RAM BIST Final address %x\n", s6)
        move ra,a1
#endif
        
	# ^W0 (write0, incrementing address)
___W0loop:	
	sd	zero, 0(s0)
        cache	0x15, 0(s0)
	daddiu	s0,	8
	ble	s0,	s6,	___W0loop
	nop
	# ^(R0,W1) (read0, write 1, incrementing address)
	move	s0,	s5      
___r0w1loop:	
	ld	s4,	0(s0)
	bnez	s4,	___r0w1fail
	nop
	sd	s3,	0(s0)    # s3 is all 1s, so you are witing 1s
	cache	0x15,	0(s0)
	daddiu	s0,	8
	ble	s0,	s6,	___r0w1loop
	nop
	# v(R1,W0) (read1, write0 to scrub, decrementing address)
	move	s0,	s5
___r1w0loop:	
	ld	s4,	0(s2)
	bne	s4,	s3,	___r1w0fail
	nop
	sd	zero,	0(s2)
	cache	0x15,	0(s2)
	daddi	s2,	-8
	bge	s2,	s0,	___r1w0loop
	nop

	# ^R0 (read0) to confirm no sa1 (stuck-at 1 ) faults  
___r0loop:	
	ld	s4,	0(s0)
	bnez	s4,	___r0fail
	nop
	daddiu	s0,	8
	ble	s0,	s6,	___r0loop
	nop
	move	s0,	s5
___nR01W1loop:	
	ld	s4,	0(s0)
	bnez	s4,	___nR01W1fail
	nop
	
	daddiu	s0,	8
	## Should we do a write?
	dsrl	s4,	s0,	5
	andi	s4,	1
	beqz	s4,	___nR01W1loop
	nop
	sd	s4,	0(s0)
	cache	0x15,	0(s0)
	bge	s0,	s6,	___nR01W1loop
	nop
	li	s2,	512
___strider:	
	move	s0,	s5
	dsll	s2,	1	
	
___KR01W1loop:	
	ld	s4,	0(s0)
	beqz	s4,	___readzero
	nop
	bne	s4,	s3,	___KR01W1fail
	nop
	
___readzero:		
	sd	s4,	0(s0)
	cache	0x15,	0(s0)
	daddu	s0,	s2
	ble	s0,	s6,	___KR01W1loop

	nop
	ble	s2,	s7,	___strider
	

	## Checkerboard
	# ^WX (write 5555, incrementing address)
	dli	s3,	0x5555555555555555
	move	s0,	s5
	
___WXloop:	
	sd	s3,	0(s0)
	cache	0x15,	0(s0)
	daddiu	s0,	8
	ble	s0,	s6,	___WXloop
	nop
	# ^(RX,Wxbar) (read5555, write aaaa, incrementing address)
	move	s0,	s5      
___rXwXbarloop:	
	ld	s4,	0(s0)
	bne	s4,	s3,	___rXwXbarfail
	nop
	## create aaaa for store
	nor	s3,	s3,	s3
	sd	s3,	0(s0)
	cache	0x15,	0(s0)

	## re-create 5555 for compare
	nor	s3,	s3,	s3
	daddiu	s0,	8
	ble	s0,	s6,	___rXwXbarloop
	nop
	# v(RXbar,WX) (readaaaa, write5555 to scrub, decrementing address)
	move	s0,	s5
	move	s2,	s6

	## create aaaa for compare
	nor	s3,	s3,	s3
___rXbarwXloop:	
	ld	s4,	0(s2)
	bne	s4,	s3,	___rXbarwXfail
	nop
	## create 5555 for store
	nor	s3,	s3,	s3
	sd	s3,	0(s2)
	cache	0x15,	0(s2)
	## re-create aaaa for compare
	nor	s3,	s3,	s3
	
	daddi	s2,	-8
	bge	s2,	s0,	___rXbarwXloop
	nop

	# ^RX (read5555) to confirm no sa1 faults
	nor	s3,	s3,	s3
___rXloop:	
	ld	s4,	0(s0)
	bne	s4,	s3,	___rXfail
	nop
	daddiu	s0,	8
	ble	s0,	s6,	___rXloop
	nop
___MATSPass:	
	PRINT("DRAM MATS Passed\n")
       	li	v0,	1
	jr	s1
	nop
	
___MATSFail:
	PRINT("DRAM MATS Failed! Halting...\n")
	li	v0,	0
	j	halt
	nop
						
	
	
___r0w1fail:	
	PRINTF("RAM_TEST_Step1:	R0W1:	address %x failed\n", s0)
	PRINTF("RAM_TEST_Step1:	R0W1:	expected 0, got %x\n", s4)

#ifdef POST_LOG
	PRINT_LOG("RAM_TEST_Step1:	R0W1:	address %x failed\n", s0)
	PRINT_LOG("RAM_TEST_Step1:	R0W1:	expected 0, got %x\n", s4)
#endif

	j	___MATSFail
	nop
			
___r1w0fail:
	PRINTF("RAM_TEST_Step2:	R1W0:	address %x failed\n", s2)
	PRINTF("RAM_TEST_Step2:	R1W0:	expected 1, got %x\n", s4)
 	
#ifdef POST_LOG
        PRINT_LOG("RAM_TEST_Step2:	R1W0:	address %x failed\n", s2)
	PRINT_LOG("RAM_TEST_Step2:	R1W0:	expected 1, got %x\n", s4)
#endif
	j	___MATSFail
	nop

___r0fail:	
	PRINTF("RAM_TEST_Step3:	R0:	address %x failed\n", s0)
	PRINTF("RAM_TEST_Step3:	R0:	expected 0, got %x\n", s4)

#ifdef POST_LOG
	PRINTF("RAM_TEST_Step3:	R0:	address %x failed\n", s0)
	PRINTF("RAM_TEST_Step3:	R0:	expected 0, got %x\n", s4)
#endif
	j	___MATSFail
	nop

___nR01W1fail:	
	PRINTF("RAM_TEST_Step4:	nR[0]W1:	address %x failed\n", s0)
	PRINTF("RAM_TEST_Step4:	nR[0]W1:	expected either 0, got %x\n", s4)

#ifdef POST_LOG 
	PRINT_LOG("RAM_TEST_Step4:	nR[0]W1:	address %x failed\n", s0)
	PRINT_LOG("RAM_TEST_Step4:	nR[0]W1:	expected either 0, got %x\n", s4)
#endif
	j	___MATSFail
	nop

___KR01W1fail:	
	PRINTF("RAM_TEST_Step5:	K[R0W1]:	address %x failed\n", s0)
	PRINTF("RAM_TEST_Step5:	K[R0W1]:	expected either 0, got %x\n", s4)
	PRINTF("RAM_TEST_Step5:	K[R0W1]:	Stride[k] = %x",s2)

#ifdef POST_LOG
	PRINT_LOG("RAM_TEST_Step5:	K[R0W1]:	address %x failed\n", s0)
	PRINT_LOG("RAM_TEST_Step5:	K[R0W1]:	expected either 0, got %x\n", s4)
	PRINT_LOG("RAM_TEST_Step5:	K[R0W1]:	Stride[k] = %x",s2)
#endif
	j	___MATSFail
	nop

___rXwXbarfail:	

	PRINTF("RAM_TEST_Step6:	R0W1:	address %x failed\n", s0)
	PRINTF("RAM_TEST_Step6:	R0W1:	expected 5555, got %x\n", s4)
	
#ifdef POST_LOG
	PRINT_LOG("RAM_TEST_Step6:	R0W1:	address %x failed\n", s0)
	PRINT_LOG("RAM_TEST_Step6:	R0W1:	expected 5555, got %x\n", s4)
#endif
	j	___MATSFail
	nop
			
___rXbarwXfail:	
	PRINTF("RAM_TEST_Step7:	R1W0:	address %x failed\n", s2)
	PRINTF("RAM_TEST_Step7:	R1W0:	expected AAAA, got %x\n", s4)

#ifdef POST_LOG
	PRINT_LOG("RAM_TEST_Step7:	R1W0:	address %x failed\n", s2)
	PRINT_LOG("RAM_TEST_Step7:	R1W0:	expected AAAA, got %x\n", s4)
#endif
	j	___MATSFail
	nop

___rXfail:	
	PRINTF("RAM_TEST_Step8:	R0:	address %x failed\n", s0)
	PRINTF("RAM_TEST_Step8:	R0:	expected 5555, got %x\n", s4)

#ifdef POST_LOG
	PRINT_LOG("RAM_TEST_Step8:	R0:	address %x failed\n", s0)
	PRINT_LOG("RAM_TEST_Step8:	R0:	expected 5555, got %x\n", s4)
#endif

	j	___MATSFail
	nop

	
END(dram_mats)

