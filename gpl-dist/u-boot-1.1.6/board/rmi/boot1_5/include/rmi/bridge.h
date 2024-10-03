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
#ifndef __BRIDGE_H__
#define __BRIDGE_H__

#include "rmi/types.h"

#define BRIDGE_DRAM_0_BAR   0
#define BRIDGE_DRAM_1_BAR   1
#define BRIDGE_DRAM_2_BAR   2
#define BRIDGE_DRAM_3_BAR   3
#define BRIDGE_DRAM_4_BAR   4
#define BRIDGE_DRAM_5_BAR   5
#define BRIDGE_DRAM_6_BAR   6
#define BRIDGE_DRAM_7_BAR   7
#define BRIDGE_DRAM_CHN_0_MTR_0_BAR  8
#define BRIDGE_DRAM_CHN_0_MTR_1_BAR  9
#define BRIDGE_DRAM_CHN_0_MTR_2_BAR 10
#define BRIDGE_DRAM_CHN_0_MTR_3_BAR  11
#define BRIDGE_DRAM_CHN_0_MTR_4_BAR  12
#define BRIDGE_DRAM_CHN_0_MTR_5_BAR  13
#define BRIDGE_DRAM_CHN_0_MTR_6_BAR  14
#define BRIDGE_DRAM_CHN_0_MTR_7_BAR  15
#define BRIDGE_DRAM_CHN_1_MTR_0_BAR  16
#define BRIDGE_DRAM_CHN_1_MTR_1_BAR  17
#define BRIDGE_DRAM_CHN_1_MTR_2_BAR  18
#define BRIDGE_DRAM_CHN_1_MTR_3_BAR  19
#define BRIDGE_DRAM_CHN_1_MTR_4_BAR  20
#define BRIDGE_DRAM_CHN_1_MTR_5_BAR  21
#define BRIDGE_DRAM_CHN_1_MTR_6_BAR  22
#define BRIDGE_DRAM_CHN_1_MTR_7_BAR  23
#define BRIDGE_CFG_BAR          24
#define BRIDGE_PHNX_IO_BAR      25
#define BRIDGE_FLASH_BAR        26
#define BRIDGE_SRAM_BAR         27
#define BRIDGE_HTMEM_BAR        28
#define BRIDGE_HTINT_BAR        29
#define BRIDGE_HTPIC_BAR        30
#define BRIDGE_HTSM_BAR         31
#define BRIDGE_HTIO_BAR         32
#define BRIDGE_HTCFG_BAR        33
#define BRIDGE_PCIXCFG_BAR     34
#define BRIDGE_PCIXMEM_BAR     35
#define BRIDGE_PCIXIO_BAR      36
#define BRIDGE_DEVICE_MASK     37
#define BRIDGE_AERR_INTR_LOG1  38
#define BRIDGE_AERR_INTR_LOG2  39
#define BRIDGE_AERR_INTR_LOG3  40
#define BRIDGE_AERR_DEV_STAT   41
#define BRIDGE_AERR1_LOG1      42
#define BRIDGE_AERR1_LOG2      43
#define BRIDGE_AERR1_LOG3      44
#define BRIDGE_AERR1_DEV_STAT  45
#define BRIDGE_AERR_INTR_EN    46
#define BRIDGE_AERR_UPG        47
#define BRIDGE_AERR_CLEAR      48
#define BRIDGE_AERR1_CLEAR     49
#define BRIDGE_SBE_COUNTS      50
#define BRIDGE_DBE_COUNTS      51
#define BRIDGE_BITERR_INT_EN   52

#define BRIDGE_SYS2IO_CREDITS  53
#define BRIDGE_EVNT_CNT_CTRL1  54
#define BRIDGE_EVNT_COUNTER1   55
#define BRIDGE_EVNT_CNT_CTRL2  56
#define BRIDGE_EVNT_COUNTER2   57
#define BRIDGE_RESERVED1       58

#define BRIDGE_DEFEATURE       59
#define BRIDGE_SCRATCH0        60
#define BRIDGE_SCRATCH1        61
#define BRIDGE_SCRATCH2        62
#define BRIDGE_SCRATCH3        63

#define BRIDGE_PHNX_IO_RESET_VADDR 0xbef00000
#define BRIDGE_PHNX_IO_REMAP_VADDR BRIDGE_PHNX_IO_RESET_VADDR
#define BRIDGE_PHNX_IO_REMAP_VAL (495<<20)

#ifndef __ASSEMBLY__

enum processor_bridge
{
	DRAM_BAR0 		= 0,
	DRAM_BAR1 		= 1,
	DRAM_BAR2 		= 2,
	DRAM_BAR3 		= 3,
	DRAM_BAR4 		= 4,
	DRAM_BAR5 		= 5,
	DRAM_BAR6 		= 6,
	DRAM_BAR7 		= 7,
	DRAM_CHNAC_DTR0 	= 8,
	DRAM_CHNAC_DTR1 	= 9,
	DRAM_CHNAC_DTR2 	= 10,
	DRAM_CHNAC_DTR3 	= 11,
	DRAM_CHNAC_DTR4 	= 12,
	DRAM_CHNAC_DTR5 	= 13,
	DRAM_CHNAC_DTR6 	= 14,
	DRAM_CHNAC_DTR7 	= 15,
	DRAM_CHNBD_DTR0 	= 16,
	DRAM_CHNBD_DTR1 	= 17,
	DRAM_CHNBD_DTR2 	= 18,
	DRAM_CHNBD_DTR3 	= 19,
	DRAM_CHNBD_DTR4 	= 20,
	DRAM_CHNBD_DTR5 	= 21,
	DRAM_CHNBD_DTR6 	= 22,
	DRAM_CHNBD_DTR7 	= 23,
	BRIDGE_CFG 		= 24,
	XLR_IO_BAR 		= 25,
	FLASH_BAR 		= 26,
	SRAM_BAR 		= 27,
	HTMEM_BAR 		= 28,
	HTINT_BAR 		= 29,
	HTPIC_BAR 		= 30,
	HTSM_BAR 		= 31,
	HTIO_BAR 		= 32,
	HTCFG_BAR 		= 33,
	PCIXCFG_BAR 		= 34,
	PCIXMEM_BAR 		= 35,
	PCIXIO_BAR 		= 36,
	DEVICE_MASK 		= 37,
	AERR0_LOG1 		= 38,
	AERR0_LOG2 		= 39,
	AERR0_LOG3 		= 40,
	AERR0_DEVSTAT 		= 41,
	AERR1_LOG1 		= 42,
	AERR1_LOG2 		= 43,
	AERR1_LOG3 		= 44,
	AERR1_DEVSTAT 		= 45,
	AERR0_EN 		= 46,
	AERR0_UPG 		= 47,
	AERR0_CLEAR 		= 48,
	AERR1_CLEAR 		= 49,
	SBE_COUNTS 		= 50,
	DBE_COUNTS 		= 51,
	BITERR_INT_EN 		= 52,
	SYS2IO_CREDITS 		= 53,
	EVENT_CNT_CNTRL1 	= 54,
	EVENT_CNTR1 		= 55,
	EVENT_CNT_CNTRL2 	= 56,
	EVENT_CNTR2 		= 57,
	RESERVED 		= 58,
	MISC 		        = 59,
	SCRATCH0 		= 60,
	SCRATCH1 		= 61,
	SCRATCH2 		= 62,
	SCRATCH3 		= 63
};

#endif
#endif
