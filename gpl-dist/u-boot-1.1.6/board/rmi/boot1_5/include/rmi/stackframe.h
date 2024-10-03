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
#ifndef __STACKFRAME_H__
#define __STACKFRAME_H__

#include "rmi/regdef.h"
#include "rmi/asm.h"
#include "rmi/types.h"

#define STACK_OFF_R0  		    48
#define STACK_OFF_R1     	    56
#define STACK_OFF_R2     	    64
#define STACK_OFF_R3     	    72
#define STACK_OFF_R4     	    80
#define STACK_OFF_R5     	    88
#define STACK_OFF_R6     	    96
#define STACK_OFF_R7     	   104
#define STACK_OFF_R8     	   112
#define STACK_OFF_R9     	   120
#define STACK_OFF_R10    	   128
#define STACK_OFF_R11    	   136
#define STACK_OFF_R12    	   144
#define STACK_OFF_R13    	   152
#define STACK_OFF_R14    	   160
#define STACK_OFF_R15    	   168
#define STACK_OFF_R16    	   176
#define STACK_OFF_R17    	   184
#define STACK_OFF_R18    	   192
#define STACK_OFF_R19    	   200
#define STACK_OFF_R20    	   208
#define STACK_OFF_R21    	   216
#define STACK_OFF_R22    	   224
#define STACK_OFF_R23    	   232
#define STACK_OFF_R24    	   240
#define STACK_OFF_R25    	   248
#define STACK_OFF_R26    	   256
#define STACK_OFF_R27    	   264
#define STACK_OFF_R28    	   272
#define STACK_OFF_R29    	   280
#define STACK_OFF_R30    	   288
#define STACK_OFF_R31    	   296

#define STACK_OFF_STATUS 	   304
#define STACK_OFF_HI     	   312
#define STACK_OFF_LO     	   320

#define STACK_OFF_BVADDR 	   328
#define STACK_OFF_CAUSE  	   336
#define STACK_OFF_EPC    	   344

#define K_STACK_SIZE   	           352

#define _PAGE_SIZE  0x1000

#define save_stack_frame           \
	.set push;                 \
	.set noat;                 \
	.set reorder;              \
	move	k1, sp;            \
        move	k0, sp;            \
	dsubu	sp, k1, K_STACK_SIZE;     \
	sd	k0, STACK_OFF_R29(sp);	  \
        sd	$3, STACK_OFF_R3(sp);	  \
	sd	$0, STACK_OFF_R0(sp);	  \
	dmfc0	v1, COP_0_STATUS;         \
	sd	$2, STACK_OFF_R2(sp);	  \
	sd	v1, STACK_OFF_STATUS(sp); \
	sd	$4, STACK_OFF_R4(sp);	  \
	dmfc0	v1, COP_0_CAUSE;          \
	sd	$5, STACK_OFF_R5(sp);	  \
	sd	v1, STACK_OFF_CAUSE(sp);  \
	sd	$6, STACK_OFF_R6(sp);	  \
	dmfc0	v1, COP_0_EPC;            \
	sd	$7, STACK_OFF_R7(sp);	  \
	sd	v1, STACK_OFF_EPC(sp);	  \
	sd	$25, STACK_OFF_R25(sp);   \
	sd	$28, STACK_OFF_R28(sp);   \
	sd	$31, STACK_OFF_R31(sp);   \
	ori	$28, sp, 0x1fff;          \
	xori	$28, 0x1fff;              \
	sd	$1, STACK_OFF_R1(sp);	  \
	mfhi	v1;		          \
	sd	$8, STACK_OFF_R8(sp);	  \
	sd	$9, STACK_OFF_R9(sp);	  \
	sd	v1, STACK_OFF_HI(sp);	  \
	mflo	v1;		          \
	sd	$10,STACK_OFF_R10(sp);	  \
	sd	$11, STACK_OFF_R11(sp);   \
	sd	v1,  STACK_OFF_LO(sp);	  \
	sd	$12, STACK_OFF_R12(sp);   \
	sd	$13, STACK_OFF_R13(sp);   \
	sd	$14, STACK_OFF_R14(sp);   \
	sd	$15, STACK_OFF_R15(sp);   \
	sd	$24, STACK_OFF_R24(sp);   \
	sd	$16, STACK_OFF_R16(sp);   \
	sd	$17, STACK_OFF_R17(sp);   \
	sd	$18, STACK_OFF_R18(sp);   \
	sd	$19, STACK_OFF_R19(sp);   \
	sd	$20, STACK_OFF_R20(sp);   \
	sd	$21, STACK_OFF_R21(sp);   \
	sd	$22, STACK_OFF_R22(sp);   \
	sd	$23, STACK_OFF_R23(sp);   \
	sd	$30, STACK_OFF_R30(sp);   \
.set pop;			   

#define restore_stack_frame                  \
	.set	push;		             \
	.set    noat;                        \
	.set	reorder;	             \
	mfc0	t0, COP_0_STATUS;            \
	ori	t0, 0x1f;	             \
	xori	t0, 0x1f;	             \
	mtc0	t0, COP_0_STATUS;            \
	li	v1, 0xff00;	             \
	and	t0, v1;		             \
	ld	v0, STACK_OFF_STATUS(sp);    \
	nor	v1, $0, v1;	             \
	and	v0, v1;		             \
	or	v0, t0;		             \
	mtc0	v0, COP_0_STATUS;            \
	ld	v1, STACK_OFF_EPC(sp);	     \
	mtc0	v1, COP_0_EPC;	             \
	ld	$31, STACK_OFF_R31(sp);      \
	ld	$28, STACK_OFF_R28(sp);      \
	ld	$25, STACK_OFF_R25(sp);      \
	ld	$7,  STACK_OFF_R7(sp);	     \
	ld	$6,  STACK_OFF_R6(sp);	     \
	ld	$5,  STACK_OFF_R5(sp);	     \
	ld	$4,  STACK_OFF_R4(sp);	     \
	ld	$3,  STACK_OFF_R3(sp);	     \
	ld	$2,  STACK_OFF_R2(sp);	     \
	ld	$1,  STACK_OFF_R1(sp);       \
	ld	$24, STACK_OFF_LO(sp);	     \
	ld	$8, STACK_OFF_R8(sp);	     \
	ld	$9, STACK_OFF_R9(sp);	     \
	mtlo	$24;		             \
	ld	$24, STACK_OFF_HI(sp);	     \
	ld	$10,STACK_OFF_R10(sp);	     \
	ld	$11, STACK_OFF_R11(sp);      \
	mthi	$24;		             \
	ld	$12, STACK_OFF_R12(sp);      \
	ld	$13, STACK_OFF_R13(sp);      \
	ld	$14, STACK_OFF_R14(sp);      \
	ld	$15, STACK_OFF_R15(sp);      \
	ld	$24, STACK_OFF_R24(sp);      \
	ld	$16, STACK_OFF_R16(sp);      \
	ld	$17, STACK_OFF_R17(sp);      \
	ld	$18, STACK_OFF_R18(sp);      \
	ld	$19, STACK_OFF_R19(sp);      \
	ld	$20, STACK_OFF_R20(sp);      \
	ld	$21, STACK_OFF_R21(sp);      \
	ld	$22, STACK_OFF_R22(sp);      \
	ld	$23, STACK_OFF_R23(sp);      \
	ld	$30, STACK_OFF_R30(sp);      \
	ld	sp,  STACK_OFF_R29(sp);      \
.set pop;			   

#ifndef __ASSEMBLY__
struct pt_regs {
	uint64_t pad0[6];

	uint64_t regs[32];

	uint64_t cp0_status;
	uint64_t hi;
	uint64_t lo;

	/*
	 * saved cp0 registers
	 */
	uint64_t cp0_badvaddr;
	uint64_t cp0_cause;
	uint64_t cp0_epc;
};
#endif
#endif
