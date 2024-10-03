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

#include "rmi/boardconfig.h"

	// Real Time Clock on Aizona board
	
LEAF(read_rtc)
	// really a nested function, but we don't have a stack available
	// we save the return address in t7
	move	t7,	ra
	move	t6,	zero
	
	
	/*read byte 3 of the time */
	li	a0,	ARIZONA_RTC_ADDR
	li	a2,	ARIZONA_RTC_BUS
	li	a1,	ARIZONA_RTC_BYTE3_OFFSET

	jal	i2c_rx
	nop

	bnez	v0,	1f
	sll	v1,	24
	or	t6,	v1

	/*read byte 2 of the time */
	li	a0,	ARIZONA_RTC_ADDR
	li	a2,	ARIZONA_RTC_BUS
	li	a1,	ARIZONA_RTC_BYTE2_OFFSET

	jal	i2c_rx
	nop

	bnez	v0,	1f
	sll	v1,	16
	or	t6,	v1

	/*read byte 1 of the time */
	li	a0,	ARIZONA_RTC_ADDR
	li	a2,	ARIZONA_RTC_BUS
	li	a1,	ARIZONA_RTC_BYTE1_OFFSET

	jal	i2c_rx
	nop

	bnez	v0,	1f
	sll	v1,	8
	or	t6,	v1

	/*read byte 0 of the time */
	li	a0,	ARIZONA_RTC_ADDR
	li	a2,	ARIZONA_RTC_BUS
	li	a1,	ARIZONA_RTC_BYTE0_OFFSET

	jal	i2c_rx
	nop

	bnez	v0,	1f
	or	v1,	t6
	// original return addr in t7
	jr	t7	
	nop

1:	
	jr	t7
	nop
			
END(read_rtc)


	// READ the Board EEPROM

LEAF(read_i2c_eeprom)
	//	byte offset in a0, read data in v0
	move	t7,	ra
	move	a1,	a0
	li	a0,	ARIZONA_EEPROM_ADDR
	li	a2,	ARIZONA_EEPROM_BUS
	jal	i2c_rx
	nop
	jr	t7
	nop
END(read_i2c_eeprom)


	// Store a byte in a1 to the eeprom at address a0
LEAF(write_i2c_eeprom)
	// save return address in t7
	move	t7,	ra
	move	a2,	a1
	move	a1,	a0
	li	a0,	ARIZONA_EEPROM_ADDR
	li	a3,	ARIZONA_EEPROM_BUS
	jal	i2c_tx
	nop
	jr	t7
	nop
END(write_i2c_eeprom)

LEAF(read_i2c_boardver)
	move	t7,	ra
	move	t6,	zero
	li	t5,	-1
	li	a0,	ARIZONA_EEPROM_ADDR
	li	a1,	ARIZONA_EEPROM_BNAME_OFFSET+7
	li	a2,	ARIZONA_EEPROM_BUS
	jal	i2c_rx
	nop
	beq	v0,	t5,	1f
	andi	v1,	0xff
	addi	v1,	-0x30
	andi	v1,	0xff
	jr	t7
	nop
1:
	li	v1,	-1
	jr	t7
	nop
END(read_i2c_boardver)
		
LEAF(read_i2c_boardver_major)
	move	t7,	ra
	move	t6,	zero
	li	t5,	-1
	li	a0,	ARIZONA_EEPROM_ADDR
	li	a1,	ARIZONA_EEPROM_MAJOR_OFFSET
	li	a2,	ARIZONA_EEPROM_BUS
	jal	i2c_rx
	nop
	beq	v0,	t5,	1f
	andi	v1,	0xff
	addi	v1,	-0x30
	/* Hack for boards with Major > 9 */
	li      a0, 0x9
	ble     v1, a0, 2f
	nop
	addi    v1, -0x7
2:
	andi	v1,	0xff
	jr	t7
	nop
1:
	li	v1,	-1
	jr	t7
	nop
END(read_i2c_boardver_major)
		
LEAF(read_i2c_boardver_minor)
	move	t7,	ra
	move	t6,	zero
	li	t5,	-1
	li	a0,	ARIZONA_EEPROM_ADDR
	li	a1,	ARIZONA_EEPROM_MINOR_OFFSET
	li	a2,	ARIZONA_EEPROM_BUS
	jal	i2c_rx
	nop
	beq	v0,	t5,	1f
	andi	v1,	0xff
	addi	v1,	-0x30
	andi	v1,	0xff
	jr	t7
	nop
1:
	li	v1,	-1
	jr	t7
	nop
END(read_i2c_boardver_minor)
		
LEAF(read_i2c_boardrev_char0)
	move	t7,	ra
	move	t6,	zero
	li	t5,	-1
	li	a0,	ARIZONA_EEPROM_ADDR
	li	a1,	ARIZONA_EEPROM_BOARDREV_OFFSET
	li	a2,	ARIZONA_EEPROM_BUS
	jal	i2c_rx
	nop
	beq	v0,	t5,	1f
	andi	v1,	0xff
//	addi	v1,	-0x30
//	andi	v1,	0xff
	jr	t7
	nop
1:
	li	v1,	-1
	jr	t7
	nop
END(read_i2c_boardrev_char0)
		
LEAF(read_i2c_boardrev_char1)
	move	t7,	ra
	move	t6,	zero
	li	t5,	-1
	li	a0,	ARIZONA_EEPROM_ADDR
	li	a1,	ARIZONA_EEPROM_BOARDREV_OFFSET+1
	li	a2,	ARIZONA_EEPROM_BUS
	jal	i2c_rx
	nop
	beq	v0,	t5,	1f
	andi	v1,	0xff
//	addi	v1,	-0x30
//	andi	v1,	0xff
	jr	t7
	nop
1:
	li	v1,	-1
	jr	t7
	nop
END(read_i2c_boardrev_char1)
		
LEAF(read_i2c_board_serialnum_0)
	move	t7,	ra
	move	t6,	zero
	li	t5,	-1
	li	a0,	ARIZONA_EEPROM_ADDR
	li	a1,	ARIZONA_EEPROM_BOARD_ID_OFFSET0
	li	a2,	ARIZONA_EEPROM_BUS
	jal	i2c_rx
	nop
	beq	v0,	t5,	1f
	andi	v1,	0xff
//	addi	v1,	-0x30
//	andi	v1,	0xff
	jr	t7
	nop
1:
	li	v1,	-1
	jr	t7
	nop
END(read_i2c_board_serialnum_0)
		
LEAF(read_i2c_board_serialnum_1)
	move	t7,	ra
	move	t6,	zero
	li	t5,	-1
	li	a0,	ARIZONA_EEPROM_ADDR
	li	a1,	ARIZONA_EEPROM_BOARD_ID_OFFSET1
	li	a2,	ARIZONA_EEPROM_BUS
	jal	i2c_rx
	nop
	beq	v0,	t5,	1f
	andi	v1,	0xff
//	addi	v1,	-0x30
//	andi	v1,	0xff
	jr	t7
	nop
1:
	li	v1,	-1
	jr	t7
	nop
END(read_i2c_board_serialnum_1)
		
		
/* Read the MAC address */
LEAF(read_i2c_macaddr)
	move	t7,	ra
	move	t6,	zero
	li	t5,	-1	
    
#if defined(CFG_BOARD_RMIREF)
	li	a0,	ARIZONA_EEPROM_ADDR
	li	a1,	ARIZONA_EEPROM_ETH_MAC_OFFSET
#elif defined(CFG_TEMPLATE_MACADDR_OFFSET)
	li	a0,	CFG_TEMPLATE_I2CADDR_EEPROM
	li	a1,	CFG_TEMPLATE_MACADDR_OFFSET
#endif

	/* byte 5 */
	jal	i2c_rx
	nop
	beq	t5,	v0,	1f
	andi	v1,	0xff
	dsll32	v1,	8
	or	t6,	v1

	/* byte 4 */
	addi	a1,	1
	jal	i2c_rx
	nop
	beq	t5,	v0,	1f
	andi	v1,	0xff
	dsll32	v1,	0
	or	t6,	v1

	/* byte 3 */
	addi	a1,	1
	jal	i2c_rx
	nop
	beq	t5,	v0,	1f
	andi	v1,	0xff
	dsll	v1,	24
	or	t6,	v1

	/* byte 2*/
	addi	a1,	1
	jal	i2c_rx
	nop
	beq	t5,	v0,	1f
	andi	v1,	0xff
	dsll	v1,	16
	or	t6,	v1

	/* byte 1 */
	addi	a1,	1
	jal	i2c_rx
	nop
	beq	t5,	v0,	1f
	andi	v1,	0xff
	dsll	v1,	8
	or	t6,	v1

	/* byte 0 */
	addi	a1,	1
	jal	i2c_rx
	nop
	beq	t5,	v0,	1f
	andi	v1,	0xff
	or	v1,	t6
	move	v0,	zero
	jr	t7
	nop
1:
	li	v1,	-1
	jr	t7	
	nop
END(read_i2c_macaddr)
	
LEAF(write_i2c_macaddr)
	// a0 has the macaddress in little endian form (Register form)
	move	t6,	ra
	move	t5,	a0
	li	t4,	ARIZONA_EEPROM_ETH_MAC_OFFSET
	// convert big endian to little endian

	//byte 5 goes to address 0
	srl	t3,	t5,	5
	andi	t3,	0xff
	addi	a0,	t3,	0
	jal	write_i2c_eeprom
	nop

	//byte 4 goes to address 1
	srl	t3,	t5,	4
	andi	t3,	0xff
	addi	a0,	t3,	1
	jal	write_i2c_eeprom
	nop

	//byte 3 goes to address 2
	srl	t3,	t5,	3
	andi	t3,	0xff
	addi	a0,	t3,	2
	jal	write_i2c_eeprom
	nop

	//byte 2 goes to address 3
	srl	t3,	t5,	4
	andi	t3,	0xff
	addi	a0,	t3,	3
	jal	write_i2c_eeprom
	nop

	//byte 1 goes to address 4
	srl	t3,	t5,	1
	andi	t3,	0xff
	addi	a0,	t3,	4
	jal	write_i2c_eeprom
	nop

	//byte 0 goes to address 5
	andi	t3,	0xff
	addi	a0,	t3,	5
	jal	write_i2c_eeprom
	nop

	jr	t6
	nop
END(write_i2c_macaddr)

	// READ the DRAM SPD EEPROM

LEAF(read_spd_eeprom)
	// Whch eeprom to read is in a0
	// which parameter to read is in a1
	move	t7,	ra
	li	a2,	SPD_BUS
	jal	i2c_rx
	nop
	jr	t7
	nop
END(read_spd_eeprom)

LEAF(read_full_spd)
	// eeprom to read is in a0
	move	s1,	ra
	move	s6,	a0
	li	s5,	0	// parameter
	li	s4,	0x50	// size
1:		
	move	a0,	s6
	move	a1,	s5
	jal	read_spd_eeprom
	nop
	move	s2,	v1
	// Print parameter value
	PRINTF("SPD Parameter %x = ", s5)
	nop
	PRINTF("%x\n", s2)
	nop
	addi	s5,	1
	addi	s4,	-1
	bnez	s4,	1b
	nop
	jr	s1
	nop
END(read_full_spd)		
LEAF(read_spd_size)
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_NUM_BYTES
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_spd_size)	
	
LEAF(read_spd_total_size)
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_TOTAL_BYTES
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_spd_total_size)
	
LEAF(read_spd_mem_type)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_MEM_TYPE
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_spd_mem_type)

	
LEAF(read_SPD_DDR_NUM_ROWS)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_NUM_ROWS
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_NUM_ROWS)

LEAF(read_SPD_DDR_NUM_COLS)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_NUM_COLS
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_NUM_COLS)
	
LEAF(read_SPD_DDR_NUM_PHYS_BANKS)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_NUM_PHYS_BANKS
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_NUM_PHYS_BANKS)
	
LEAF(read_SPD_DDR_MODULE_WIDTH)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_MODULE_WIDTH
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_MODULE_WIDTH)
	
LEAF(read_SPD_DDR_MODULE_WIDTH_EXT)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_MODULE_WIDTH_EXT
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_MODULE_WIDTH_EXT)
	
LEAF(read_SPD_DDR_VOLTAGE_INTF)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_VOLTAGE_INTF
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_VOLTAGE_INTF)
	
LEAF(read_SPD_DDR_MAX_CYCLE_TIME)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_MAX_CYCLE_TIME
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_MAX_CYCLE_TIME)
	
LEAF(read_SPD_DDR_DRAM_tAC)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_DRAM_tAC
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_DRAM_tAC)
	
LEAF(read_SPD_DDR_DIMM_ECC_PARITY)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_DIMM_ECC_PARITY
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_DIMM_ECC_PARITY)
	
LEAF(read_SPD_DDR_DIMM_REFRESH_RATE)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_DIMM_REFRESH_RATE
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_DIMM_REFRESH_RATE)
	
LEAF(read_SPD_DDR_PRIMARY_WIDTH)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_PRIMARY_WIDTH
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_PRIMARY_WIDTH)
	
LEAF(read_SPD_DDR_ERRORCK_WIDTH)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_ERRORCK_WIDTH
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_ERRORCK_WIDTH)
	
LEAF(read_SPD_DDR_tCCD_MIN)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_tCCD_MIN
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_tCCD_MIN)
	
LEAF(read_SPD_DDR_BL_SUPPORTED)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_BL_SUPPORTED
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_BL_SUPPORTED)
	
LEAF(read_SPD_DDR_NUM_BANKS)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_NUM_BANKS
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_NUM_BANKS)
	
LEAF(read_SPD_DDR_CASLAT)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_CASLAT
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_CASLAT)
		
LEAF(read_SPD_DDR_DIMM_TYPE)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_DIMM_TYPE
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_DIMM_TYPE)
	
LEAF(read_SPD_DDR_BUF_REG)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_BUF_REG
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_BUF_REG)
	
LEAF(read_SPD_DDR_DEV_ATTR)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_DEV_ATTR
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_DEV_ATTR)
	
LEAF(read_SPD_DDR_DERATE_FREQ_CL_DERATED_HALF)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_DERATE_FREQ_CL_DERATED_HALF
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_DERATE_FREQ_CL_DERATED_HALF)
	
LEAF(read_SPD_DDR_DERATE_tAC_CL_DERATED_HALF)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_DERATE_tAC_CL_DERATED_HALF
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_DERATE_tAC_CL_DERATED_HALF)
	
LEAF(read_SPD_DDR_DERATE_FREQ_CL_DERATED_ONE)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_DERATE_FREQ_CL_DERATED_ONE
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_DERATE_FREQ_CL_DERATED_ONE)
	
LEAF(read_SPD_DDR_DERATE_tAC_CL_DERATED_ONE)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_DERATE_tAC_CL_DERATED_ONE
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_DERATE_tAC_CL_DERATED_ONE)
	
LEAF(read_SPD_DDR_tRP)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_tRP
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_tRP)
	
LEAF(read_SPD_DDR_tRRD)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_tRRD
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_tRRD)
	
LEAF(read_SPD_DDR_tRCD)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_tRCD
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_tRCD)
	
LEAF(read_SPD_DDR_tRAS)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_tRAS
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_tRAS)
	
LEAF(read_SPD_DDR_DENSITY)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_DENSITY
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_DENSITY)
	
LEAF(read_SPD_DDR_tIS)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_tIS
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_tIS)
	
LEAF(read_SPD_DDR_tIH)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_tIH
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_tIH)
	
LEAF(read_SPD_DDR_tDS)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_tDS
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_tDS)
	
LEAF(read_SPD_DDR_tDH)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_tDH
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_tDH)
	
LEAF(read_SPD_DDR_tRC)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_tRC
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_tRC)
	
LEAF(read_SPD_DDR_tRFC)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_tRFC
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_tRFC)
	
LEAF(read_SPD_DDR_tCK_MAX)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_tCK_MAX
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_tCK_MAX)
	
LEAF(read_SPD_DDR_tDQSQ)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_tDQSQ
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_tDQSQ)
	
LEAF(read_SPD_DDR_tQHS)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_tQHS
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_tQHS)
	
LEAF(read_SPD_DDR_PLL_RELOCK)
	// which eeprom to read is in a0, retval in v1, -1 on failure
	move	t6,	ra
	li	a1,	ARIZONA_SPD_DDR_PLL_RELOCK
	jal	read_spd_eeprom
	nop
	jr	t6
	nop
END(read_SPD_DDR_PLL_RELOCK)
	
	// Functions to access the thermal sensor on the board

	// subroutine called by individual parameter reads
LEAF(read_thermal_sensor)
	// which parameter to read is in a1
	move	t7,	ra
	li	a0,	ARIZONA_TEMPSENSOR_ADDR
//	li	a2,	ARIZONA_TEMPSENSOR_BUS
	jal	i2c_rx
	nop
	jr	t7
	nop
END(read_thermal_sensor)

LEAF(write_thermal_sensor)
	// parameter to write is in a1
	// value to write is in a2
	move	t7,	ra
	li	a0,	ARIZONA_TEMPSENSOR_ADDR
	li	a3,	ARIZONA_TEMPSENSOR_BUS
	jal	i2c_tx
	nop
	jr	t7
	nop
END(write_thermal_sensor)
	
LEAF(read_i2cts_INT_TEMP)
	move	t6,	ra
	li	a1,	ARIZONA_TEMPSENSOR_INT_TEMP
	jal	read_thermal_sensor
	nop
	andi	v1,	0x7f
	jr	t6
	nop
END(read_i2cts_INT_TEMP)
	
LEAF(read_i2cts_EXT_TEMP)
	move	t6,	ra
	li	a1,	ARIZONA_TEMPSENSOR_EXT_TEMP
	jal	read_thermal_sensor
	nop
	andi	v1,	0x7f
	jr	t6
	nop
END(read_i2cts_EXT_TEMP)
	
LEAF(read_i2cts_RDSTATUS)
	move	t6,	ra
	li	a1,	ARIZONA_TEMPSENSOR_RDSTATUS
	jal	read_thermal_sensor
	nop
	jr	t6
	nop
END(read_i2cts_RDSTATUS)
	
LEAF(read_i2cts_RDCONFIG)
	move	t6,	ra
	li	a1,	ARIZONA_TEMPSENSOR_RDCONFIG
	jal	read_thermal_sensor
	nop
	jr	t6
	nop
END(read_i2cts_RDCONFIG)
	
LEAF(read_i2cts_RDCONVRATE)
	move	t6,	ra
	li	a1,	ARIZONA_TEMPSENSOR_RDCONVRATE
	jal	read_thermal_sensor
	nop
	jr	t6
	nop
END(read_i2cts_RDCONVRATE)
	
LEAF(read_i2cts_RDINTHI)
	move	t6,	ra
	li	a1,	ARIZONA_TEMPSENSOR_RDINTHI
	jal	read_thermal_sensor
	nop
	jr	t6
	nop
END(read_i2cts_RDINTHI)
	
LEAF(read_i2cts_RDINTLO)
	move	t6,	ra
	li	a1,	ARIZONA_TEMPSENSOR_RDINTLO
	jal	read_thermal_sensor
	nop
	jr	t6
	nop
END(read_i2cts_RDINTLO)
	
LEAF(read_i2cts_RDEXTHI)
	move	t6,	ra
	li	a1,	ARIZONA_TEMPSENSOR_RDEXTHI
	jal	read_thermal_sensor
	nop
	jr	t6
	nop
END(read_i2cts_RDEXTHI)
	
LEAF(read_i2cts_RDEXTLO)
	move	t6,	ra
	li	a1,	ARIZONA_TEMPSENSOR_RDEXTLO
	jal	read_thermal_sensor
	nop
	jr	t6
	nop
END(read_i2cts_RDEXTLO)

	// value to write is in a0
LEAF(write_i2cts_WRCONFIG)
	move	t6,	ra
	move	a2,	a0
	li	a1,	ARIZONA_TEMPSENSOR_WRCONFIG
	jal	write_thermal_sensor
	nop
	jr	t6
	nop
END(write_i2cts_WRCONFIG)
	
LEAF(write_i2cts_WRCONVRATE)
	move	t6,	ra
	move	a2,	a0
	li	a1,	ARIZONA_TEMPSENSOR_WRCONVRATE
	jal	write_thermal_sensor
	nop
	jr	t6
	nop
END(write_i2cts_WRCONVRATE)
	
LEAF(write_i2cts_WRINTHI)
	move	t6,	ra
	move	a2,	a0
	li	a1,	ARIZONA_TEMPSENSOR_WRINTHI
	jal	write_thermal_sensor
	nop
	jr	t6
	nop
END(write_i2cts_WRINTHI)
	
LEAF(write_i2cts_WRINTLO)
	move	t6,	ra
	move	a2,	a0
	li	a1,	ARIZONA_TEMPSENSOR_WRINTLO
	jal	write_thermal_sensor
	nop
	jr	t6
	nop
END(write_i2cts_WRINTLO)
	
LEAF(write_i2cts_WREXTHI)
	move	t6,	ra
	move	a2,	a0
	li	a1,	ARIZONA_TEMPSENSOR_WREXTHI
	jal	write_thermal_sensor
	nop
	jr	t6
	nop
END(write_i2cts_WREXTHI)
	
LEAF(write_i2cts_WREXTLO)
	move	t6,	ra
	move	a2,	a0
	li	a1,	ARIZONA_TEMPSENSOR_WREXTLO
	jal	write_thermal_sensor
	nop
	jr	t6
	nop
END(write_i2cts_WREXTLO)
	
LEAF(read_i2cts_ONESHOT)
	move	t6,	ra
	li	a1,	ARIZONA_TEMPSENSOR_ONESHOT
	jal	read_thermal_sensor
	nop
	jr	t6
	nop
END(read_i2cts_ONESHOT)
	
LEAF(read_i2cts_RDEXTNDEXT)
	move	t6,	ra
	li	a1,	ARIZONA_TEMPSENSOR_RDEXTNDEXT
	jal	read_thermal_sensor
	nop
	jr	t6
	nop
END(read_i2cts_RDEXTNDEXT)
	
LEAF(read_i2cts_RDEXTNDINT)
	move	t6,	ra
	li	a1,	ARIZONA_TEMPSENSOR_RDEXTNDINT
	jal	read_thermal_sensor
	nop
	jr	t6
	nop
END(read_i2cts_RDEXTNDINT)
	
LEAF(arizona_i2c_devtest)
	move	s7,	ra
	li	s6,	ARIZONA_RTC_ADDR
	PRINTF("Accessing the real time clock at %x ...\n",s6)
	/* I2C tests */	
	jal	read_rtc
	nop
	PRINTF("RTC read returns %d\n", v1)

	jal	read_i2cts_EXT_TEMP
	nop
	PRINTF("Thermal sensor indicates XLR Processor diode temperature is 0x%X Celsius\n",v1)
	nop
	
	PRINT("Probing all 4 memory channels to see if dimm exists\n")
	li	s6,	SPD_EEPROM0
//	PRINTF("Probing at address %x\n", s6)	
	li	a0,	SPD_EEPROM0
	jal	read_spd_size
	nop
	bnez	v0,	1f
	nop
	PRINTF("DIMM0:	 Size of SPD EEPROM is %X\n", v1)
	li	a0,	SPD_EEPROM0
//	jal	read_full_spd
	nop
	j	2f
1:	
	PRINT("DIMM0:	No dimm detected\n") ;
2:		
	li	s6,	SPD_EEPROM1
//	PRINTF("Probing at address %x\n", s6)	
	li	a0,	SPD_EEPROM1
	jal	read_spd_size
	nop
	bnez	v0,	1f
	
	PRINTF("DIMM1:	 Size of SPD EEPROM is %X\n", v1)
	li	a0,	SPD_EEPROM1
//	jal	read_full_spd
	nop
	j	2f
1:	
	PRINT("DIMM1:	No dimm detected\n") ;
2:		
	li	s6,	SPD_EEPROM2
//	PRINTF("Probing at address %x\n", s6)	
	li	a0,	SPD_EEPROM2
	jal	read_spd_size
	nop
	bnez	v0,	1f
	PRINTF("DIMM2:	 Size of SPD EEPROM is %X\n", v1)
	li	a0,	SPD_EEPROM2
//	jal	read_full_spd
	nop
	j	2f
1:	
	PRINT("DIMM2:	No dimm detected\n") ;
2:		
	li	s6,	SPD_EEPROM3
//	PRINTF("Probing at address %x\n", s6)	
	li	a0,	SPD_EEPROM3
	jal	read_spd_size
	nop
	bnez	v0,	1f
	PRINTF("DIMM3:	 Size of SPD EEPROM is %X\n", v1)
	li	a0,	SPD_EEPROM3
//	jal	read_full_spd
	nop
	j	2f
1:	
	PRINT("DIMM3:	No dimm detected\n") ;
2:		
	jr	s7
	nop
END(arizona_i2c_devtest)
					
