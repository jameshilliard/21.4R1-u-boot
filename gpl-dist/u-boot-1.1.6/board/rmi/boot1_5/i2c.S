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

#include "rmi/on_chip.h"
#include "rmi/bridge.h"
#include "rmi/mem_ctrl.h"
#include "rmi/i2c_spd.h"
#include "rmi/board_i2c.h"
	
 /*******************************************************************************
 * Ackerr:	All writes should wait for sdoempty
 *		All reads should wait for datardy
 *		All transactions should wait for idle bit
 *		All startxfr commands should retry on lostarb and starterr
 *		- The above ensures that ACKERR is only due to a non-existant device
 *		On ACKERR, return error
 *	
 *******************************************************************************/

	
LEAF(i2c_init)

	/* Bus 1 */
	li	t1, I2C_CLOCK_DIVISOR
	li	t0, BRIDGE_PHNX_IO_REMAP_VADDR + PHOENIX_IO_I2C_1_OFFSET	
	I2C_SW	t1, (I2C_CLKDIV << 2)(t0)

	li	t1, I2C_HDSTA_DELAY
	I2C_SW	t1, (I2C_HDSTATIM << 2)(t0)

	/* Bus 0 */
	li	t1, I2C_CLOCK_DIVISOR
	li	t0, BRIDGE_PHNX_IO_REMAP_VADDR + PHOENIX_IO_I2C_0_OFFSET
	I2C_SW	t1, (I2C_CLKDIV << 2)(t0)
	
	li	t1, I2C_HDSTA_DELAY
	I2C_SW	t1, (I2C_HDSTATIM << 2)(t0)

	jr	ra
	nop
	
END(i2c_init)



	
	/************************************************************
	*	I2C Write Single Byte				    *
	************************************************************/
	
	/* a0 = slave address, a1 = mem loc, a2 = data, a3 = i2c0 or 1,v0 = result */
LEAF(i2c_tx)

	li	t0, BRIDGE_PHNX_IO_REMAP_VADDR + PHOENIX_IO_I2C_1_OFFSET

	bnez	a3,	1f
	nop
	
	li	t0, BRIDGE_PHNX_IO_REMAP_VADDR + PHOENIX_IO_I2C_0_OFFSET

	/* Wait for any previous transactions to complete */
1:	I2C_LW	t1, (I2C_STATUS << 2)(t0)
	andi	t1, t1, 0x01
	bnez	t1, 1b

	/* Interrupts off, 
		8 bit internal-device address, 
		7 bit i2c-compliant device address
		Both addresses enabled
	*/
	li	t1, 0xf8 
	I2C_SW	t1, (I2C_CONFIG << 2)(t0)

	I2C_SW	zero, (I2C_BYTECNT << 2)(t0)			
	
	I2C_SW	a0, (I2C_DEVADDR << 2)(t0)

	I2C_SW	a1, (I2C_ADDR << 2)(t0)

	I2C_SW	a2, (I2C_DATAOUT << 2)(t0)

	/* Write transaction */
1:	I2C_SW	zero, (I2C_STARTXFR << 2)(t0)
	
2:	I2C_LW	t1, (I2C_STATUS << 2)(t0)

	/* ackerr ? abort */
	andi	t2, t1, 0x08
	bnez	t2, 3f

	/* lostarb || starterr ? retry startxfr */
	andi	t2, t1, 0x30
	bnez	t2, 1b 

	/* !sdoempty ? poll on status */
	andi	t2, t1, 0x02
	beqz	t2, 2b
		
	move	v0, zero	
	jr	ra
	
3:	li	v0, -1
	jr	ra
	
END(i2c_tx)


	/************************************************************/
	/* I2C READ Single byte					    */
	/************************************************************/
	
	/* a0 = slave address, a1 = mem loc, a2=i2c0 or 1, v0 = result, v1 = data */
LEAF(i2c_rx)

	li	t0, BRIDGE_PHNX_IO_REMAP_VADDR + PHOENIX_IO_I2C_1_OFFSET
	
	bnez	a2,	1f
	nop
	
	li	t0, BRIDGE_PHNX_IO_REMAP_VADDR + PHOENIX_IO_I2C_0_OFFSET
1:	

	/* Interrupts off
		8 bit internal address
		7 bit i2c device address followed by direction bit
		both addresses enabled
	*/
	
	li	t1, 0xf8 
	I2C_SW	t1, (I2C_CONFIG << 2)(t0)

	I2C_SW	zero, (I2C_BYTECNT << 2)(t0)
	I2C_SW	a0, (I2C_DEVADDR << 2)(t0)
	I2C_SW	a1, (I2C_ADDR << 2)(t0)

	/* startxfer with NODATA */
1:	li	t1, I2C_TND_XFR 
	I2C_SW	t1, (I2C_STARTXFR << 2)(t0)

2:	
	I2C_LW	t1, (I2C_STATUS << 2)(t0)
	/* ackerr ? abort */
	andi	t2, t1, 0x08
	bnez	t2, 3f

	/* lostarb || starterr ? retry startxfr */
	andi	t2, t1, 0x30
	bnez	t2, 1b

	/* busy ? poll on status */
	andi	t2, t1, 0x01
	bnez	t2, 2b

		
	/* Interrupts off
		8 bit internal address, not transmitted
		7 bit device address, transmitted
	*/

	li	t1, 0xfa 
	I2C_SW	t1, (I2C_CONFIG << 2)(t0)

	I2C_SW	zero, (I2C_BYTECNT << 2)(t0)			

1:	li	t1, I2C_READ_XFR 
	I2C_SW	t1, (I2C_STARTXFR << 2)(t0)

2:	
	I2C_LW	t1, (I2C_STATUS << 2)(t0)
	/* ackerr ? abort */
	andi	t2, t1, 0x08
	bnez	t2, 3f

	/* lostarb || starterr ? retry startxfr */
	andi	t2, t1, 0x30
	bnez	t2, 1b

	/* !datardy? poll on status */
	andi	t2, t1, 0x04
	beqz	t2, 2b

	I2C_LW	t2, (I2C_DATAIN << 2)(t0)

	move	v1,	t2
	move	v0,	zero
	jr	ra
	nop
	
3:
	li	v0,	-1
	li	v1,	-1
	jr	ra
	nop	
END(i2c_rx)
	

	
