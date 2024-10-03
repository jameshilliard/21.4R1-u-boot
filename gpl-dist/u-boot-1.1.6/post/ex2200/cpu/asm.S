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

/* void cpu_post_exec_12 (ulong *code, ulong *res, ulong op1, ulong op2); */
/* res ulong in [r1] */
	.global	cpu_post_exec_12
cpu_post_exec_12:
	stmfd	sp!, {r11, r14}
	mov	r11, lr
	mov	lr, pc
	bx	r0
	ldmfd	sp!, {r11, pc}
	
/* void cpu_post_exec_11 (ulong *code, ulong *res, ulong op1); */
/* res ulong in [r1] */
	.global	cpu_post_exec_11
cpu_post_exec_11:
	stmfd	sp!, {r11, r14}
	mov	r11, lr
	mov	lr, pc
	bx	r0
	ldmfd	sp!, {r11, pc}

/* ulong cpu_post_load_return (ulong *code, ulong *res); */
/* return ulong in r0 */
	.global	cpu_post_load_return
cpu_post_load_return:
	stmfd	sp!, {r11, r14}
	mov	r11, lr
	mov	lr, pc
	bx	r0
	ldmfd	sp!, {r11, pc}

