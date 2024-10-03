/*
 * Copyright (c) 2009-2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#ifndef _CPU_ASM_H
#define _CPU_ASM_H

/* flag N, Z, C, V */
#define COND_SH				28
#define COND_EQ				0x0  /* Z = 1 */
#define COND_NE				0x1  /* Z = 0 */
#define COND_CS				0x2  /* C = 1 */
#define COND_CC				0x3  /* C = 0 */
#define COND_MI				0x4  /* N = 1 */
#define COND_PL				0x5  /* N = 0 */
#define COND_VS				0x6  /* V = 1 */
#define COND_VC				0x7  /* V = 0 */
#define COND_HI				0x8  /* C =1, Z = 0 */
#define COND_LS				0x9  /* C = 0, Z = 1 */
#define COND_GE				0xA  /* N = V */
#define COND_LT				0xB  /* N != V */
#define COND_GT				0xC  /* Z = 0, N = V */
#define COND_LE				0xD  /* Z = 1, N != V */
#define COND_AL				0xE  /* always */
#define COND_NV				0xF  /* never */

/* option fields */
#define OP_I_SH				25
#define OP_I				1 /* operand 2, 0 = register, 1 = immediate value */
#define OP_S_SH				20
#define OP_S				1 /* condition code, 0 = not set, 1 = set */
#define OP_A_SH				21
#define OP_A				1 /* 0 = multiply, 1 = multiply and accumulate */
#define OP_P_SH				24
#define OP_P				1 /* 0 = post, 1 = pre */
#define OP_U_SH				23
#define OP_U				1 /* 0 = down, 1 = up */
#define OP_B_SH				22
#define OP_B				1 /* transfer quanty, 0 = word, 1 = byte */
#define OP_W_SH				21
#define OP_W				1 /* write back, 0 = no, 1 = yes */
#define OP_L_SH				20
#define OP_L				1 /* memory, 0 = store to, 1 = load from */

/* register */
#define OP_RLIST_MASK		0xFFFF
#define OP_RLIST_SH			0
#define OP_R_MASK			0xF
#define OP_RN_SH			16
#define OP_RD_SH			12
#define OP_RS_SH			8
#define OP_RM_SH			0
#define OP_R0				0x0
#define OP_R1				0x1
#define OP_R2				0x2
#define OP_R3				0x3
#define OP_R4				0x4
#define OP_R5				0x5
#define OP_R6				0x6
#define OP_R7				0x7
#define OP_R8				0x8
#define OP_R9				0x9
#define OP_R10				0xA
#define OP_R11				0xB
#define OP_R12				0xC
#define OP_R13				0xD
#define OP_R14				0xE
#define OP_R15				0xF

/* others */
#define OP_SN_MASK			0x1F
#define OP_SN_SH			7
#define OP_SH_MASK			0x3
#define OP_SH_SH			5
#define OP_RT_MASK			0xF
#define OP_RT_SH			8
#define OP_IM_8_MASK		0xFF
#define OP_IM_12_MASK		0xFFF
#define OP_IM_SH			0

/* branch */
#define BIT_B_L_SH			24
#define BIT_B_L				1  /* link option */

#define OP_BR_SH			25
#define OP_BR				0x5  /* branch */

#define OP_BX_SH			20
#define OP_BX				0x12  /* bx */
#define OP_BX_1_SH			4
#define OP_BX_1				0x1  
#define OP_BX_2_SH			8
#define OP_BX_2				0xFFF  

/* data processing, option fields -I, S */
#define OP_D_SH				21
#define OP_D_AND			0x0  /* and */
#define OP_D_EOR			0x1  /* eor */
#define OP_D_SUB			0x2  /* sub */
#define OP_D_RSB			0x3  /* rsb */
#define OP_D_ADD			0x4  /* add */
#define OP_D_ADC			0x5  /* adc */
#define OP_D_SBC			0x6  /* sbc */
#define OP_D_RSC			0x7  /* adc */
#define OP_D_TST			0x8  /* tst */
#define OP_D_TEQ			0x9  /* teq */
#define OP_D_CMP			0xA  /* cmp */
#define OP_D_CMN			0xB  /* cmn */
#define OP_D_ORR			0xC  /* orr */
#define OP_D_MOV			0xD  /* mov */
#define OP_D_BIC			0xE  /* bic */
#define OP_D_MVN			0xF  /* mvn */

/* PSR transfer, option fileds - I */
#define OP_PSR_FLAG_SH		22
#define OP_PSR_FLAG			1 /* PSR, 0 = CPSR, 1 = SPSR */
/* MRS */
#define OP_MRS_SH			16
#define OP_MRS				0x10F /* PSR to register */
/* MSR */
#define OP_MSR_SH			12
#define OP_MSR				0x129F /* to PSR */
#define OP_MSR_F			0x128F /* to PSR flag */

/* multiply, option fileds - A, S */
#define OP_M_SH				22
#define OP_M				0x0 /* multiply */
#define OP_M_1_SH			4
#define OP_M_1				0x9

/* single data transfer, option fields - I, P, U, B, W, L */
#define OP_ST_SH			26
#define OP_ST				0x1 /* load/store */

/* block data transfer, option fields - P, U, B, W, L */
#define OP_BT_SH			25
#define OP_BT				0x4 /* load/store */

/* single data swap, option fields - B */
#define OP_DW_SH			22
#define OP_DW				0x2 /* swap */
#define OP_DW_1_SH			4
#define OP_DW_1				0x9

/*------------------- ARM instructions ----------------------*/
/* branch */
#define ASM_B(cond, link, offset)    \
                            ((cond << COND_SH) +    \
                            (OP_BR << OP_BR_SH) +   \
                            (link ? (BIT_B_L << BIT_B_L_SH) : 0) +  \
                            (offset >> 2))

#define ASM_BX(cond, rm)    \
                            ((cond << COND_SH) +    \
                            (OP_BX << OP_BX_SH) +   \
                            (OP_BX_1 << OP_BX_1_SH) +   \
                            (OP_BX_2 << OP_BX_2_SH) +   \
                            (rm & OP_R_MASK) << OP_RM_SH)

#define ASM_BLR    ASM_BX(COND_AL, 14)

/* data processing */
#define ASM_DP(cond, opcode, I, S, rn, rd, shift, rm, rotate, im)    \
                            ((cond << COND_SH) +    \
                            (opcode << OP_D_SH) +   \
                            (S ? (OP_S << OP_S_SH) : 0) +  \
                            ((rn & OP_R_MASK) << OP_RN_SH) +   \
                            ((rd & OP_R_MASK) << OP_RD_SH) +   \
                            (I ? (((rotate & OP_RT_MASK) << OP_RT_SH) + \
                            (im & OP_IM_8_MASK) << OP_IM_SH) : \
                            (((shift & OP_SH_MASK) << OP_SH_SH) + \
                            (rm & OP_R_MASK) << OP_RM_SH)))

#define ASM_AND(cond, I, S, rn, rd, shift, rm, rotate, im)    \
                            ASM_DP(cond, OP_D_AND, I, S, rn, rd, shift, rm, rotate, im)

#define ASM_EOR(cond, I, S, rn, rd, shift, rm, rotate, im)    \
                            ASM_DP(cond, OP_D_EOR, I, S, rn, rd, shift, rm, rotate, im)

#define ASM_SUB(cond, I, S, rn, rd, shift, rm, rotate, im)    \
                            ASM_DP(cond, OP_D_SUB, I, S, rn, rd, shift, rm, rotate, im)

#define ASM_RSB(cond, I, S, rn, rd, shift, rm, rotate, im)    \
                            ASM_DP(cond, OP_D_RSB, I, S, rn, rd, shift, rm, rotate, im)

#define ASM_ADD(cond, I, S, rn, rd, shift, rm, rotate, im)    \
                            ASM_DP(cond, OP_D_ADD, I, S, rn, rd, shift, rm, rotate, im)

#define ASM_ADC(cond, I, S, rn, rd, shift, rm, rotate, im)    \
                            ASM_DP(cond, OP_D_ADC, I, S, rn, rd, shift, rm, rotate, im)

#define ASM_SBC(cond, I, S, rn, rd, shift, rm, rotate, im)    \
                            ASM_DP(cond, OP_D_SBC, I, S, rn, rd, shift, rm, rotate, im)

#define ASM_RSC(cond, I, S, rn, rd, shift, rm, rotate, im)    \
                            ASM_DP(cond, OP_D_RSC, I, S, rn, rd, shift, rm, rotate, im)

#define ASM_TST(cond, I, S, rn, rd, shift, rm, rotate, im)    \
                            ASM_DP(cond, OP_D_TST, I, S, rn, rd, shift, rm, rotate, im)

#define ASM_TEQ(cond, I, S, rn, rd, shift, rm, rotate, im)    \
                            ASM_DP(cond, OP_D_TEQ, I, S, rn, rd, shift, rm, rotate, im)

#define ASM_CMP(cond, I, rn, rd, shift, rm, rotate, im)    \
                            ASM_DP(cond, OP_D_CMP, I, 1, rn, rd, shift, rm, rotate, im)

#define ASM_CMN(cond, I, rn, rd, shift, rm, rotate, im)    \
                            ASM_DP(cond, OP_D_CMN, I, 1, rn, rd, shift, rm, rotate, im)

#define ASM_ORR(cond, I, S, rn, rd, shift, rm, rotate, im)    \
                            ASM_DP(cond, OP_D_ORR, I, S, rn, rd, shift, rm, rotate, im)

#define ASM_MOV(cond, I, S, rn, rd, shift, rm, rotate, im)    \
                            ASM_DP(cond, OP_D_MOV, I, S, rn, rd, shift, rm, rotate, im)

#define ASM_BIC(cond, I, S, rn, rd, shift, rm, rotate, im)    \
                            ASM_DP(cond, OP_D_BIC, I, S, rn, rd, shift, rm, rotate, im)

#define ASM_MVN(cond, I, S, rn, rd, shift, rm, rotate, im)    \
                            ASM_DP(cond, OP_D_MVN, I, S, rn, rd, shift, rm, rotate, im)

/* PSR transfer, option fields - I */
#define ASM_MRS(cond, psr, rd)    \
                            ((cond << COND_SH) +    \
                            (OP_MRS << OP_MRS_SH) +   \
                            ((psr & 0x1) << OP_PSR_FLAG_SH) +   \
                            ((rd & OP_R_MASK) << OP_RD_SH))

#define ASM_MSR(cond, psr, rm)    \
                            ((cond << COND_SH) +    \
                            (OP_MSR << OP_MSR_SH) +   \
                            ((psr & 0x1) << OP_PSR_FLAG_SH) +   \
                            ((rm & OP_R_MASK) << OP_RM_SH))

#define ASM_MSR_F(cond, I, psr, shift, rm, rotate, im)    \
                            ((cond << COND_SH) +    \
                            (OP_MSR_F << OP_MSR_SH) +   \
                            ((psr & 0x1) << OP_PSR_FLAG_SH) +   \
                            (I ? (((rotate & OP_RT_MASK) << OP_RT_SH) + \
                            (im & OP_IM_8_MASK) << OP_IM_SH) : \
                            ((rm & OP_R_MASK) << OP_RM_SH)))

/* multiply, option fileds - A, S */
#define ASM_MULT(cond, A, S, rs, rn, rm)    \
                            ((cond << COND_SH) +    \
                            (OP_M << OP_M_SH) +   \
                            (A ? (OP_A << OP_A_SH) : 0) +  \
                            (S ? (OP_S << OP_S_SH) : 0) +  \
                            (OP_M_1 << OP_M_1_SH) +   \
                            ((rs & OP_R_MASK) << OP_RS_SH) +   \
                            ((rn & OP_R_MASK) << OP_RN_SH) +   \
                            ((rm & OP_R_MASK) << OP_RM_SH))

#define ASM_MUL(cond, S, rs, rn, rm)    \
                            ASM_MULT(cond, 0, S, rs, rn, rm)
                            
#define ASM_MLA(cond, S, rn, rd, rm)    \
                            ASM_MULT(cond, 1, S, rn, rd, rm)

/* single data transfer, option fields - I, P, U, B, W, L */
#define ASM_ST(cond, I, P, U, B, W, L, rn, rd, shift, rm, im)    \
                            ((cond << COND_SH) +    \
                            (OP_ST << OP_ST_SH) +   \
                            (P ? (OP_P << OP_P_SH) : 0) +  \
                            (U ? (OP_U << OP_U_SH) : 0) +  \
                            (B ? (OP_B << OP_B_SH) : 0) +  \
                            (W ? (OP_W << OP_W_SH) : 0) +  \
                            (L ? (OP_L << OP_L_SH) : 0) +  \
                            ((rn & OP_R_MASK) << OP_RN_SH) +   \
                            ((rd & OP_R_MASK) << OP_RD_SH) +   \
                            (I ? (((shift & OP_SH_MASK) << OP_SH_SH) + \
                            (rm & OP_R_MASK) << OP_RM_SH) : \
                            ((im & OP_IM_12_MASK) << OP_IM_SH)))

#define ASM_LDR(cond, I, P, U, B, W, rn, rd, shift, rm, im)    \
                            ASM_ST(cond, I, P, U, B, W, 1, rn, rd, shift, rm, im)
                            
#define ASM_LDRW(cond, P, U, B, W, rn, rd, am1, am2)    \
                            ((cond << COND_SH) +    \
                            (0x0 << OP_ST_SH) +   \
                            (P ? (OP_P << OP_P_SH) : 0) +  \
                            (U ? (OP_U << OP_U_SH) : 0) +  \
                            (B ? (OP_B << OP_B_SH) : 0) +  \
                            (W ? (OP_W << OP_W_SH) : 0) +  \
                            (1 << OP_L_SH) +  \
                            ((rn & OP_R_MASK) << OP_RN_SH) +   \
                            ((rd & OP_R_MASK) << OP_RD_SH) +   \
                            (am1 << 8) + \
                            (am2) + \
                            (0xB << 4))
                            
#define ASM_STR(cond, I, P, U, B, W, rn, rd, shift, rm, im)    \
                            ASM_ST(cond, I, P, U, B, W, 0, rn, rd, shift, rm, im)
                            
/* block data transfer, option fields - P, U, B, W, L */
#define OP_BT_SH			25
#define OP_BT				0x4 /* load/store */
#define ASM_BT(cond, P, U, B, W, L, rn, rlist)    \
                            ((cond << COND_SH) +    \
                            (OP_BT << OP_BT_SH) +   \
                            (P ? (OP_P << OP_P_SH) : 0) +  \
                            (U ? (OP_U << OP_U_SH) : 0) +  \
                            (B ? (OP_B << OP_B_SH) : 0) +  \
                            (W ? (OP_W << OP_W_SH) : 0) +  \
                            (L ? (OP_L << OP_L_SH) : 0) +  \
                            ((rn & OP_R_MASK) << OP_RN_SH) +   \
                            ((rlist & OP_RLIST_MASK) << OP_RLIST_SH))

#define ASM_LDM(cond, P, U, B, W, L, rn, rlist)    \
                            ASM_BT(cond, P, U, B, W, 1, rn, rlist)
                            
#define ASM_STM(cond, P, U, B, W, L, rn, rlist)    \
                            ASM_BT(cond, P, U, B, W, 0, rn, rlist)

/* single data swap, option fields - B */
#define ASM_SWP(cond, B, rn, rd, rm)    \
                            ((cond << COND_SH) +    \
                            (OP_DW << OP_DW_SH) +   \
                            (OP_DW_1 << OP_DW_1_SH) +   \
                            (B ? (OP_B << OP_B_SH) : 0) +  \
                            ((rn & OP_R_MASK) << OP_RN_SH) +   \
                            ((rd & OP_R_MASK) << OP_RD_SH) +   \
                            ((rm & OP_R_MASK) << OP_RM_SH))


//extern  void flush_page_to_ram(void *page);
//extern int cpu_dbg;

#endif /* _CPU_ASM_H */
