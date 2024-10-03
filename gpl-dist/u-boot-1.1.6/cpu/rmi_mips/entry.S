#include "rmi/asm.h"
#include "rmi/regdef.h"
#include "rmi/stackframe.h"
#include "rmi/mipscopreg.h"
#include "rmi/threads.h"
#include "rmi/msgring.h"
#include "rmi/asm-macros.h"


/* void park(int thr_id) */
LEAF(old_park)
	.set push
	li      t1, 1
	sllv    t1, t1, a0
	1:      msgwait t1
	msgld   a0
	/* wait for pending to clear */
	2:      mfc2    t2, MSGRNG_MSG_STATUS_REG, 0
	andi    t3, t2, 0x08
	bnez    t3, 2b
	nop
	/* check for empty bucket and pending fail */
	andi    t3, t2, 0x30
	bnez    t3, 1b
	nop
	/* Check for SW code */
	srl     t3, t2, 8
	andi    t3, 0xff
	li      t4, MSGRNG_CODE_BOOT_WAKEUP
	bne     t3, t4, 1b
	nop
	/* check for size */
	srl     t3, t2, 6
	andi    t3, 0x03
	li      t4, 3
	bne     t3, t4, 1b
	nop
	/* msg0=sp, msg1=gp, msg2=fn msg3=args */
	dmfc2   sp, MSGRNG_RX_BUF_REG, 0
	dmfc2   gp, MSGRNG_RX_BUF_REG, 1
	dmfc2   a0, MSGRNG_RX_BUF_REG, 2
	dmfc2   a1, MSGRNG_RX_BUF_REG, 3

	la      t1, smp_callee_function
	jr      t1
	nop

	.set pop
END(old_park)


	.text
LEAF(rmi_nmi_handler)
	.set push
	.set noat

	/* Save K0 and K1 to scratch regs sel 6 and 7 */

	mtc0	k0, $22, 6
	mtc0	k1, $22, 7

	/* Clear NMI and BEV bits */
	mfc0	k0, COP_0_STATUS,0
	li 	k1, 0xffb7ffff
	and 	k0, k0, k1
	mtc0 	k0, COP_0_STATUS,0

	/* restore K0 and K1 registers */
	mfc0	k0, $22, 6
	mfc0	k1, $22, 7

	eret
	nop
	nop
	.set at
	.set pop
END(rmi_nmi_handler)





