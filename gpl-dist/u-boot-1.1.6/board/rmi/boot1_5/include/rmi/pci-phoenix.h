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
#ifndef __PCIPHOENIX_H__
#define __PCIPHOENIX_H__

/* ToDo

#include "processor.h"
#include "board.h"
#include "device.h"

*/

#define PCI_CTRL_HOST_MODE                  1
#define PCI_CTRL_DEVICE_MODE                0
#define PCI_CTRL_SIM_MODE                   2

#define PCI_CTRL_PCI_MODE                   0
#define PCI_CTRL_PCIX_MODE                  1

#define PCI_CTRL_REG_MODE                   0
#define PCI_CTRL_REG_DEV_MODE_BASE          1
#define PCI_CTRL_REG_DEV_MODE_SIZE          2


/****************************************************************************
* Base Address (Virtual) of the PCI Config address space      
 * For now, choose 256M phys in kseg1 = 0xA0000000 + (1<<28)
 * Config space spans 256 (num of buses) * 256 (num functions) * 256 bytes
 * ie 1<<24 = 16M
 ***************************************************************************/

/* 
   See physaddrspace.c for the following values. TODO, this should be obtained
   dynamically
*/
#define PCI_MEM_START 0xD0000000
#define PCI_MEM_SIZE  ((256 << 20) -1 )

#define PCI_IO_START  (256 << 20)
#define PCI_IO_SIZE   ( (64 << 20) -1 )

/* PCIX extended reg base starts at byte offset 256 */
enum phoenix_pcix 
{
#ifdef PHOENIX_LITTLE_ENDIAN
	PCIX_REG_BASE                      = 64,
#else
	PCIX_REG_BASE                      = (512 + 64),
#endif

	PCIX_HBAR0_REG 				         = (PCIX_REG_BASE + 0),
	PCIX_HBAR1_REG 				         = (PCIX_REG_BASE + 1),
	PCIX_HBAR2_REG 				         = (PCIX_REG_BASE + 2),
	PCIX_HBAR3_REG 				         = (PCIX_REG_BASE + 3),
	PCIX_HBAR4_REG 				         = (PCIX_REG_BASE + 4),
	PCIX_HBAR5_REG 				         = (PCIX_REG_BASE + 5),		

	PCIX_ENBAR0_REG 			         = (PCIX_REG_BASE + 6),
	PCIX_ENBAR1_REG 			         = (PCIX_REG_BASE + 7),
	PCIX_ENBAR2_REG	 			         = (PCIX_REG_BASE + 8),
	PCIX_ENBAR3_REG 			         = (PCIX_REG_BASE + 9),
	PCIX_ENBAR4_REG 			         = (PCIX_REG_BASE + 10),
	PCIX_ENBAR5_REG 			         = (PCIX_REG_BASE + 11),		

	PCIX_MATCHBIT_ADDR_REG 			     = (PCIX_REG_BASE + 12),
	PCIX_MATCHBIT_SIZE_REG 			     = (PCIX_REG_BASE + 13),
	PCIX_PHOENIX_CONTROL_REG 		     = (PCIX_REG_BASE + 14),

	PCIX_INTRPT_CONTROL_REG 		     = (PCIX_REG_BASE + 15),
	PCIX_INTRPT_STATUS_REG 			     = (PCIX_REG_BASE + 16),

	PCIX_ERRSTATUS_REG 			         = (PCIX_REG_BASE + 17),

	PCIX_INT_MSILADDR0_REG			     = (PCIX_REG_BASE + 18),
	PCIX_INT_MSIHADDR0_REG			     = (PCIX_REG_BASE + 19),
	PCIX_INT_MSIDATA0_REG 			     = (PCIX_REG_BASE + 20),

	PCIX_INT_MSILADDR1_REG			     = (PCIX_REG_BASE + 21),
	PCIX_INT_MSIHADDR1_REG			     = (PCIX_REG_BASE + 22),
	PCIX_INT_MSIDATA1_REG 			     = (PCIX_REG_BASE + 23),				

	PCIX_TARGET_DBG0_REG 			     = (PCIX_REG_BASE + 24),
	PCIX_TARGET_DBG1_REG 			     = (PCIX_REG_BASE + 25),

	PCIX_INIT_DBG0_REG 			         = (PCIX_REG_BASE + 26),
	PCIX_INIT_DBG1_REG 			         = (PCIX_REG_BASE + 27),		

	PCIX_EVENT_EN_REG 			         = (PCIX_REG_BASE + 28),
	PCIX_EVENT_CNT_REG 			         = (PCIX_REG_BASE + 29),

	PCIX_HOST_MODE_STATUS_REG 		     = (PCIX_REG_BASE + 30),	
	PCIX_DEFEATURE_MODE_CTLSTATUS_REG 	 = (PCIX_REG_BASE + 31),

	PCIX_EXPROM_ADDR_REG 			     = (PCIX_REG_BASE + 32),
	PCIX_EXPROM_SIZE_REG 			     = (PCIX_REG_BASE + 33),		

	PCIX_DMA_TIMEOUT_REG 			     = (PCIX_REG_BASE + 34),		
	PCIX_HOST_MODE_CTRL_REG 		     = (PCIX_REG_BASE + 35),		

	PCIX_READEX_BAR_REG			         = (PCIX_REG_BASE + 36),
	PCIX_READEX_BAR_SIZE_REG		     = (PCIX_REG_BASE + 37),
	PCIX_DMA_UNALIGNED_LO_REG		     = (PCIX_REG_BASE + 38),
	PCIX_DMA_UNALIGNED_HI_REG		     = (PCIX_REG_BASE + 39),

	PCIX_TX_CALIB_PRESET_REG		     = (PCIX_REG_BASE + 40),
	PCIX_TX_CALIB_PRESET_COUNT_REG	     = (PCIX_REG_BASE + 41),
	PCIX_TX_CALIB_BIAS_REG			     = (PCIX_REG_BASE + 42),
	PCIX_TX_CALIB_CODE_REG			     = (PCIX_REG_BASE + 43),

	PCIX_DEVMODE_TBL_BAR0_REG		     = (PCIX_REG_BASE + 44),
	PCIX_DEVMODE_TBL_BAR1_REG		     = (PCIX_REG_BASE + 45),
	PCIX_DEVMODE_TBL_BAR2_REG		     = (PCIX_REG_BASE + 46),
	PCIX_DEVMODE_TBL_BAR3_REG		     = (PCIX_REG_BASE + 47),

	PCIX_L2_ALLOC_BAR_REG			     = (PCIX_REG_BASE + 48),
	PCIX_L2_ALLOC_BAR_SIZE_REG		     = (PCIX_REG_BASE + 49)

};


/* ToDo

extern int pcix_init_board(struct board *this_board, struct device *this_dev);
extern int pcix_attach(struct board *this_board, struct device *this_dev);

*/

#endif
