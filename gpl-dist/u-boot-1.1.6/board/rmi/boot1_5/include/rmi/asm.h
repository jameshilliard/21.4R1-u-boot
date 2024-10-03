/***********************************************************************
Copyright 2003-2006 Raza Microelectronics, Inc.(RMI).
This is a derived work from software originally provided by the external
entity identified below. The licensing terms and warranties specified in
the header of the original work apply to this derived work.
Contribution by RMI:
   1. Added PRINT and PRINTF
*****************************#RMI_1#**********************************/

/*	$NetBSD: asm.h,v 1.35 2003/08/07 16:28:27 agc Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)machAsmDefs.h	8.1 (Berkeley) 6/10/93
 */

/*
 * machAsmDefs.h --
 *
 *	Macros used when writing assembler programs.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * from: Header: /sprite/src/kernel/mach/ds3100.md/RCS/machAsmDefs.h,
 *	v 1.2 89/08/15 18:28:24 rab Exp  SPRITE (DECWRL)
 */

#ifndef _ASM_H
#define _ASM_H


# define _C_LABEL(x)	x

/*
 * LEAF
 *	A leaf routine does
 *	- call no other function,
 *	- never use any register that callee-saved (S0-S8), and
 *	- not use any local stack storage.
 */
#define LEAF(x)				\
	.globl	_C_LABEL(x);		\
	.ent	_C_LABEL(x), 0;		\
_C_LABEL(x): ;				\
	.frame sp, 0, ra;		


/*
 * STATIC_LEAF
 *	Declare a local leaf function.
 */
#define STATIC_LEAF(x)			\
	.ent	_C_LABEL(x), 0;		\
_C_LABEL(x): ;				\
	.frame sp, 0, ra;		\
	MCOUNT

/*
 * NESTED
 *	A function calls other functions and needs
 *	therefore stack space to save/restore registers.
 */
#define NESTED(x, fsize, retpc)		\
	.globl	_C_LABEL(x);		\
	.ent	_C_LABEL(x), 0; 	\
_C_LABEL(x): ;				\
	.frame	sp, fsize, retpc;	

/*
 * END
 *	Mark end of a procedure.
 */
#define END(x) \
	.end _C_LABEL(x)

/*
 * EXPORT -- export definition of symbol
 */
#define EXPORT(x)			\
	.globl	_C_LABEL(x);		\
_C_LABEL(x):


/*
 * Print formatted string
 */
#define PRINTF(string, arg)                             \
		.set	push;				\
		.set	reorder;                        \
                move    a1, arg;                        \
		la	a0,9f;                          \
		la	t0, asm_printf;			\
		jalr	t0;                     	\
		.set	pop;				\
		MSG(string)

#define PRINT(string)                                   \
		.set	push;				\
		.set	reorder;                        \
		la	a0,9f;                          \
		la      t0, asm_printf;			\
		jalr	t0;	                     	\
		.set	pop;				\
		MSG(string)

#define PRINT_LOG(string, arg)                          \
		.set	push;				\
		.set	reorder;                        \
                move    a1, arg;                         \
		la	a0,9f;                          \
		jal	eeprom_printf;                  \
		.set	pop;				\
	        MSG(string)

/*
 * Macros to panic and printf from assembly language.
 */
#define PANIC(msg)			\
	la	a0, 9f;			\
	jal	_C_LABEL(panic);	\
	nop;				\
	MSG(msg)

#define	MSG(msg)			\
	.rdata;				\
9:	.asciiz	msg;			\
	.text

#if 0
#define	LOG_MSG(msg)			\
	.rdata;				\
10:	.asciiz	msg1;            	\
	.text
#endif

#endif /* _ASM_H */
