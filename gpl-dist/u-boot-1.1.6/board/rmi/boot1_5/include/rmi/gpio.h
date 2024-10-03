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
#ifndef __GPIO_H__
#define __GPIO_H__

#include "rmi/types.h"

#define GPIO_INT_EN_REG 0
#define GPIO_INPUT_INVERSION_REG 1
#define GPIO_IO_DIR_REG 2
#define GPIO_IO_DATA_WR_REG 3
#define GPIO_IO_DATA_RD_REG 4

#define GPIO_SWRESET_REG 8

#define GPIO_DRAM1_CNTRL_REG 9
#define GPIO_DRAM1_RATIO_REG 10
#define GPIO_DRAM1_RESET_REG 11
#define GPIO_DRAM1_STATUS_REG 12

#define GPIO_DRAM2_CNTRL_REG 13
#define GPIO_DRAM2_RATIO_REG 14
#define GPIO_DRAM2_RESET_REG 15
#define GPIO_DRAM2_STATUS_REG 16

#define GPIO_QDR_PLL_CNTRL_REG 17
#define GPIO_QDR_PLL_RATIO_REG 18
#define GPIO_QDR_PLL_RESET_REG 19
#define GPIO_QDR_PLL_STATUS_REG 20

#define GPIO_PWRON_RESET_CFG_REG 21

#define GPIO_BIST_ALL_GO_STATUS_REG 24
#define GPIO_BIST_CPU_GO_STATUS_REG 25
#define GPIO_BIST_DEV_GO_STATUS_REG 26

#define GPIO_FUSEBANK_REG 35

#define GPIO_CPU_RESET_REG 40

#define PWRON_RESET_PCMCIA_BOOT 17

#define GPIO_LED_BITMAP 0x1700000
#define GPIO_LED_0_SHIFT 20
#define GPIO_LED_1_SHIFT 24

#define GPIO_LED_OUTPUT_CODE_RESET 0x01
#define GPIO_LED_OUTPUT_CODE_HARD_RESET 0x02
#define GPIO_LED_OUTPUT_CODE_SOFT_RESET 0x03
#define GPIO_LED_OUTPUT_CODE_MAIN 0x04

#define RUN_BIST_BIT 0x00100000

#define GPIO_IO_REG  0xbef1800C
#define GPIO_CFG_REG 0xbef18008

#ifndef __ASSEMBLY__
enum processor_gpio
{
	GPIO_INT_EN     = 0,
	GPIO_IN_INV     = 1,
	GPIO_DIR        = 2,
	GPIO_WRITE      = 3,
	GPIO_READ       = 4,
	GPIO_INT_CLR    = 5,
	GPIO_INT_STATUS = 6,
	GPIO_INT_TYPE   = 7,
	GPIO_RESET      = 8,
	GPIO_DRAMAB_PLL_EN      = 9,
	GPIO_DRAMAB_PLL_RATIO   = 10,
	GPIO_DRAMAB_PLL_RESET   = 11,
	GPIO_DRAMAB_PLL_STATUS  = 12,
	GPIO_DRAMCD_PLL_EN      = 13,
	GPIO_DRAMCD_PLL_RATIO   = 14,
	GPIO_DRAMCD_PLL_RESET   = 15,
	GPIO_DRAMCD_PLL_STATUS  = 16,
	GPIO_QDR_PLL_EN         = 17,
	GPIO_QDR_PLL_RATIO      = 18,
	GPIO_QDR_PLL_RESET      = 19,
	GPIO_QDR_PLL_STATUS     = 20,
	GPIO_RESET_CFG          = 21,
	GPIO_THERMAL_FSM_EN     = 22,
	GPIO_SHIFT_PATTERN      = 23,
	GPIO_BIST_STATUS_0      = 24,
	GPIO_BIST_STATUS_1      = 25,
	GPIO_BIST_STATUS_2      = 26,
	GPIO_L1DATA_SENSE_AMP   = 27,
	GPIO_L1TAG_SENSE_AMP    = 28,
	GPIO_L2DATA_SENSE_AMP   = 29,
	GPIO_L2TAG_SENSE_AMP    = 30,
	GPIO_L2U_3CYCLE = 31,
	GPIO_CLOCK_DLYS_0       = 32,
	GPIO_CLOCK_DLYS_1       = 33,
	GPIO_CLK_DLYS_2 = 34,
	RESERVED_0      = 35,
	GPIO_HT_CLK_EN  = 36,
	GPIO_INT_MAP    = 37,
	GPIO_EXT_INT    = 38,
	RESERVED_1      = 39,
	GPIO_CPU_RST    = 40,
	LOW_PWR_DIS     = 41,
	GPIO_SLOW_CLOCK = 42,
	GPIO_RANDOM     = 43,
	GPIO_HT_CLK_DIV = 44,
	GPIO_CPU_CLK_DIS  = 45,
	GPIO_PCIX_PLL_DIS = 46,
	GPIO_PCIX_DLY     = 47
};
#endif
#endif
