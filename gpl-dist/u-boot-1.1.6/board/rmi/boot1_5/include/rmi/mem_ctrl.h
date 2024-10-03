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
#ifndef _MEM_CTRL_H
#define _MEM_CTRL_H

#define DDR2_CONFIG_REG_PARAMS_1                 0
#define DDR2_CONFIG_REG_PARAMS_2                 1
#define DDR2_CONFIG_REG_PARAMS_3                 2
#define DDR2_CONFIG_REG_PARAMS_4                 3
#define DDR2_CONFIG_REG_PARAMS_5                 4
#define DDR2_CONFIG_REG_BNK_CTL                  5
#define DDR2_CONFIG_REG_ADDRESS_PARAMS           6
#define DDR2_CONFIG_REG_GLB_PARAMS               7
#define DDR2_CONFIG_REG_CLK_CALIBRATE_PARAMS     8
#define DDR2_CONFIG_REG_ADDRESS_PARAMS1 DDR2_CONFIG_REG_ADDRESS_PARAMS
#define DDR2_CONFIG_REG_CLK_CALIBRATE_REQUEST    9

#define DDR2_CONFIG_REG_CLK_CALIBRATION_RES_CMD 19
#define DDR2_CONFIG_REG_MRS                     20
#define DDR2_CONFIG_REG_RDMRS                   21
#define DDR2_CONFIG_REG_EMRS1                   22
#define DDR2_CONFIG_REG_EMRS2                   23
#define DDR2_CONFIG_REG_EMRS3                   24
#define DDR2_CONFIG_REG_RESET_TIMERS            25
#define DDR2_CONFIG_REG_PARAMS_6                26
#define DDR2_CONFIG_REG_ADDRESS_PARAMS2         27
#define DDR2_CONFIG_REG_RESET_CMD0              28
#define DDR2_CONFIG_REG_RESET_CMD1              29
#define DDR2_CONFIG_REG_RESET_CMD2              30
#define DDR2_CONFIG_REG_RESET_CMD3              31
#define DDR2_CONFIG_REG_RESET_CMD4              32
#define DDR2_CONFIG_REG_RESET_CMD5              33
#define DDR2_CONFIG_REG_RESET_CMD6              34
#define DDR2_CONFIG_REG_RMW_ECC_LOG_HI          35
#define DDR2_CONFIG_REG_ECC_POISON_MASK         36
#define DDR2_CONFIG_REG_POISON_MASK_0           37
#define DDR2_CONFIG_REG_POISON_MASK_1           38
#define DDR2_CONFIG_REG_POISON_MASK_2           39
#define DDR2_CONFIG_REG_POISON_MASK_3           40
#define DDR2_CONFIG_REG_PARAMS_7                41
#define DDR2_CONFIG_REG_WRITE_ODT_MASK          42
#define DDR2_CONFIG_REG_READ_ODT_MASK           43
#define DDR2_CONFIG_REG_RMW_ECC_LOG             44
#define DDR2_COUNTER_CONFIG                     45
#define DDR2_PRF_COUNTER0                       46
#define DDR2_PRF_COUNTER1                       47
#define DDR2_CONFIG_REG_DYN_DATA_PAD_CTRL       48

#define RLD2_BASE_ADDRESS_REGISTER0             64
#define RLD2_BASE_ADDRESS_REGISTER1             65
#define MASTER_CONFIG_REGISTER0                 66
#define MASTER_CONFIG_REGISTER1                 67
#define DAR_REGISTER                            68
#define INITIALIZE_REGISTER                     69

#define DDR2_DQS0_RX_NEW_DELAY_STAGE_COUNT (0x400 >> 2)
#define DDR2_DQS1_RX_NEW_DELAY_STAGE_COUNT (0x410 >> 2)
#define DDR2_DQS2_RX_NEW_DELAY_STAGE_COUNT (0x420 >> 2)
#define DDR2_DQS3_RX_NEW_DELAY_STAGE_COUNT (0x430 >> 2)
#define DDR2_DQS4_RX_NEW_DELAY_STAGE_COUNT (0x440 >> 2)
#define DDR2_DQS5_RX_NEW_DELAY_STAGE_COUNT (0x450 >> 2)
#define DDR2_DQS6_RX_NEW_DELAY_STAGE_COUNT (0x460 >> 2)
#define DDR2_DQS7_RX_NEW_DELAY_STAGE_COUNT (0x470 >> 2)
#define DDR2_DQS8_RX_NEW_DELAY_STAGE_COUNT (0x480 >> 2)
#define DDR2_DQS9_RX_NEW_DELAY_STAGE_COUNT (0x490 >> 2)
#define DDR2_DQS9_RX_NEW_DELAY_STAGE_COUNT_36_RD_ADDR (0x470 >> 2)
#define DDR2_DQS9_RX_NEW_DELAY_STAGE_COUNT_36_WR_ADDR (0x480 >> 2)

#define DDR2_DQS0_TX_NEW_DELAY_STAGE_COUNT (0x500 >> 2)
#define DDR2_DQS1_TX_NEW_DELAY_STAGE_COUNT (0x510 >> 2)
#define DDR2_DQS2_TX_NEW_DELAY_STAGE_COUNT (0x520 >> 2)
#define DDR2_DQS3_TX_NEW_DELAY_STAGE_COUNT (0x530 >> 2)
#define DDR2_DQS4_TX_NEW_DELAY_STAGE_COUNT (0x540 >> 2)
#define DDR2_DQS5_TX_NEW_DELAY_STAGE_COUNT (0x550 >> 2)
#define DDR2_DQS6_TX_NEW_DELAY_STAGE_COUNT (0x560 >> 2)
#define DDR2_DQS7_TX_NEW_DELAY_STAGE_COUNT (0x570 >> 2)
#define DDR2_DQS8_TX_NEW_DELAY_STAGE_COUNT (0x580 >> 2)
#define DDR2_DQS9_TX_NEW_DELAY_STAGE_COUNT (0x590 >> 2)
#define DDR2_DQS9_TX_NEW_DELAY_STAGE_COUNT_36_RD_ADDR (0x570 >> 2)
#define DDR2_DQS9_TX_NEW_DELAY_STAGE_COUNT_36_WR_ADDR (0x580 >> 2)

#define DDR2_CMD_NEW_DELAY_STAGE_COUNT     (0x4F0 >> 2)
#define DDR2_SLV_CMD_NEW_DELAY_STAGE_COUNT (0x4E0 >> 2)

#define DDR2_DQS0_RX_DELAY_STAGE_COUNT (0x404 >> 2)
#define DDR2_DQS1_RX_DELAY_STAGE_COUNT (0x414 >> 2)
#define DDR2_DQS2_RX_DELAY_STAGE_COUNT (0x424 >> 2)
#define DDR2_DQS3_RX_DELAY_STAGE_COUNT (0x434 >> 2)
#define DDR2_DQS4_RX_DELAY_STAGE_COUNT (0x444 >> 2)
#define DDR2_DQS5_RX_DELAY_STAGE_COUNT (0x454 >> 2)
#define DDR2_DQS6_RX_DELAY_STAGE_COUNT (0x464 >> 2)
#define DDR2_DQS7_RX_DELAY_STAGE_COUNT (0x474 >> 2)
#define DDR2_DQS8_RX_DELAY_STAGE_COUNT (0x484 >> 2)
#define DDR2_DQS9_RX_DELAY_STAGE_COUNT (0x494 >> 2)
#define DDR2_DQS9_RX_DELAY_STAGE_COUNT_36_RD_ADDR (0x474 >> 2)
#define DDR2_DQS9_RX_DELAY_STAGE_COUNT_36_WR_ADDR (0x484 >> 2)

#define DDR2_DQS0_TX_DELAY_STAGE_COUNT (0x504 >> 2)
#define DDR2_DQS1_TX_DELAY_STAGE_COUNT (0x514 >> 2)
#define DDR2_DQS2_TX_DELAY_STAGE_COUNT (0x524 >> 2)
#define DDR2_DQS3_TX_DELAY_STAGE_COUNT (0x534 >> 2)
#define DDR2_DQS4_TX_DELAY_STAGE_COUNT (0x544 >> 2)
#define DDR2_DQS5_TX_DELAY_STAGE_COUNT (0x554 >> 2)
#define DDR2_DQS6_TX_DELAY_STAGE_COUNT (0x564 >> 2)
#define DDR2_DQS7_TX_DELAY_STAGE_COUNT (0x574 >> 2)
#define DDR2_DQS8_TX_DELAY_STAGE_COUNT (0x584 >> 2)
#define DDR2_DQS9_TX_DELAY_STAGE_COUNT (0x594 >> 2)
#define DDR2_DQS9_TX_DELAY_STAGE_COUNT_36_RD_ADDR (0x574 >> 2)
#define DDR2_DQS9_TX_DELAY_STAGE_COUNT_36_WR_ADDR (0x584 >> 2)

#define DDR2_CMD_DELAY_STAGE_COUNT      (0x4F4 >> 2)
#define DDR2_SLV_CMD_DELAY_STAGE_COUNT  (0x4E4 >> 2)

#define DDR2_DQS0_RX_CYCLE_MEASURE_COUNT (0x408 >> 2)
#define DDR2_DQS1_RX_CYCLE_MEASURE_COUNT (0x418 >> 2)
#define DDR2_DQS2_RX_CYCLE_MEASURE_COUNT (0x428 >> 2)
#define DDR2_DQS3_RX_CYCLE_MEASURE_COUNT (0x438 >> 2)
#define DDR2_DQS4_RX_CYCLE_MEASURE_COUNT (0x448 >> 2)
#define DDR2_DQS5_RX_CYCLE_MEASURE_COUNT (0x458 >> 2)
#define DDR2_DQS6_RX_CYCLE_MEASURE_COUNT (0x468 >> 2)
#define DDR2_DQS7_RX_CYCLE_MEASURE_COUNT (0x478 >> 2)
#define DDR2_DQS8_RX_CYCLE_MEASURE_COUNT (0x488 >> 2)
#define DDR2_DQS9_RX_CYCLE_MEASURE_COUNT (0x498 >> 2)
#define DDR2_DQS9_RX_CYCLE_MEASURE_COUNT_36_RD_ADDR (0x478 >> 2)
#define DDR2_DQS9_RX_CYCLE_MEASURE_COUNT_36_WR_ADDR (0x488 >> 2)

#define DDR2_DQS0_TX_CYCLE_MEASURE_COUNT (0x508 >> 2)
#define DDR2_DQS1_TX_CYCLE_MEASURE_COUNT (0x518 >> 2)
#define DDR2_DQS2_TX_CYCLE_MEASURE_COUNT (0x528 >> 2)
#define DDR2_DQS3_TX_CYCLE_MEASURE_COUNT (0x538 >> 2)
#define DDR2_DQS4_TX_CYCLE_MEASURE_COUNT (0x548 >> 2)
#define DDR2_DQS5_TX_CYCLE_MEASURE_COUNT (0x558 >> 2)
#define DDR2_DQS6_TX_CYCLE_MEASURE_COUNT (0x568 >> 2)
#define DDR2_DQS7_TX_CYCLE_MEASURE_COUNT (0x578 >> 2)
#define DDR2_DQS8_TX_CYCLE_MEASURE_COUNT (0x588 >> 2)
#define DDR2_DQS9_TX_CYCLE_MEASURE_COUNT (0x598 >> 2)
#define DDR2_DQS9_TX_CYCLE_MEASURE_COUNT_36_RD_ADDR (0x578 >> 2)
#define DDR2_DQS9_TX_CYCLE_MEASURE_COUNT_36_WR_ADDR (0x588 >> 2)

#define DDR2_CMD_CYCLE_MEASURE_COUNT      (0x4F8 >> 2)
#define DDR2_SLV_CMD_CYCLE_MEASURE_COUNT  (0x4E8 >> 2)

#define DDR2_DQS0_RX_DELAY_STAGE_CONFIG (0x40c >> 2)
#define DDR2_DQS1_RX_DELAY_STAGE_CONFIG (0x41c >> 2)
#define DDR2_DQS2_RX_DELAY_STAGE_CONFIG (0x42c >> 2)
#define DDR2_DQS3_RX_DELAY_STAGE_CONFIG (0x43c >> 2)
#define DDR2_DQS4_RX_DELAY_STAGE_CONFIG (0x44c >> 2)
#define DDR2_DQS5_RX_DELAY_STAGE_CONFIG (0x45c >> 2)
#define DDR2_DQS6_RX_DELAY_STAGE_CONFIG (0x46c >> 2)
#define DDR2_DQS7_RX_DELAY_STAGE_CONFIG (0x47c >> 2)
#define DDR2_DQS8_RX_DELAY_STAGE_CONFIG (0x48c >> 2)
#define DDR2_DQS9_RX_DELAY_STAGE_CONFIG (0x49c >> 2)
#define DDR2_DQS9_RX_DELAY_STAGE_CONFIG_36_RD_ADDR (0x47c >> 2)
#define DDR2_DQS9_RX_DELAY_STAGE_CONFIG_36_WR_ADDR (0x48c >> 2)

#define DDR2_DQS0_TX_DELAY_STAGE_CONFIG (0x50c >> 2)
#define DDR2_DQS1_TX_DELAY_STAGE_CONFIG (0x51c >> 2)
#define DDR2_DQS2_TX_DELAY_STAGE_CONFIG (0x52c >> 2)
#define DDR2_DQS3_TX_DELAY_STAGE_CONFIG (0x53c >> 2)
#define DDR2_DQS4_TX_DELAY_STAGE_CONFIG (0x54c >> 2)
#define DDR2_DQS5_TX_DELAY_STAGE_CONFIG (0x55c >> 2)
#define DDR2_DQS6_TX_DELAY_STAGE_CONFIG (0x56c >> 2)
#define DDR2_DQS7_TX_DELAY_STAGE_CONFIG (0x57c >> 2)
#define DDR2_DQS8_TX_DELAY_STAGE_CONFIG (0x58c >> 2)
#define DDR2_DQS9_TX_DELAY_STAGE_CONFIG (0x59c >> 2)
#define DDR2_DQS9_TX_DELAY_STAGE_CONFIG_36_RD_ADDR (0x57c >> 2)
#define DDR2_DQS9_TX_DELAY_STAGE_CONFIG_36_WR_ADDR (0x58c >> 2)

#define DDR2_CMD_DELAY_STAGE_CONFIG      (0x4FC >> 2)
#define DDR2_SLV_CMD_DELAY_STAGE_CONFIG  (0x4EC >> 2)

#define DDR2_RX0_THERM_CTRL_WR_ADDR (0x4d0 >> 2)
#define DDR2_RX1_THERM_CTRL_WR_ADDR (0x4d4 >> 2)
#define DDR2_TX0_THERM_CTRL_WR_ADDR (0x4d8 >> 2)
#define DDR2_TX1_THERM_CTRL_WR_ADDR (0x4dc >> 2)
#define DDR2_RX0_THERM_CTRL_RD_ADDR (0x4c0 >> 2)
#define DDR2_RX1_THERM_CTRL_RD_ADDR (0x4c4 >> 2)
#define DDR2_TX0_THERM_CTRL_RD_ADDR (0x4c8 >> 2)
#define DDR2_TX1_THERM_CTRL_RD_ADDR (0x4cc >> 2)


#define PHOENIX_SILICON_VERSION_MAJOR 'B'
#define PHOENIX_SILICON_VERSION_MINOR '0'

/* SPD byte addr definitions */
#define SPD_DATA_NUM_BYTES 64
#define ARIZONA_SPD_DDR_tWR 36
#define ARIZONA_SPD_DDR_tWTR 37
#define ARIZONA_SPD_DDR_tRTP 38
#define ARIZONA_SPD_DDR_tRFC_tRC_EXTENSION 40
#define SPD_DATA_CHECKSUM_BYTE 63

#define SPD_DATA_ENC_MEMTYPE_DDR1 7
#define SPD_DATA_ENC_MEMTYPE_DDR2 8

#define SPD_DATA_DDR1_tWR 15 /* 15 ns */
#define SPD_DATA_DDR1_tWTR 2 /* 2 clock cycles */

#define PHOENIX_DRAM_CLK_PERIOD_NUMERATOR 30
#define PHOENIX_DRAM_CLK_PERIOD_DENOM_100  3 /* 30/ 3 = 10.00 */
#define PHOENIX_DRAM_CLK_PERIOD_DENOM_133  4 /* 30/ 4 =  7.50 */
#define PHOENIX_DRAM_CLK_PERIOD_DENOM_166  5 /* 30/ 5 =  6.00 */
#define PHOENIX_DRAM_CLK_PERIOD_DENOM_200  6 /* 30/ 6 =  5.00 */
#define PHOENIX_DRAM_CLK_PERIOD_DENOM_233  7 /* 30/ 7 =  4.29 */
#define PHOENIX_DRAM_CLK_PERIOD_DENOM_266  8 /* 30/ 8 =  3.75 */
#define PHOENIX_DRAM_CLK_PERIOD_DENOM_300  9 /* 30/ 9 =  3.33 */
#define PHOENIX_DRAM_CLK_PERIOD_DENOM_333 10 /* 30/10 =  3.00 */ 
#define PHOENIX_DRAM_CLK_PERIOD_DENOM_366 11 /* 30/11 =  2.73 */
#define PHOENIX_DRAM_CLK_PERIOD_DENOM_400 12 /* 30/12 =  2.50 */


#define PHOENIX_DRAM_PLL_RATIO_100 0x76	/* out = 3, feedback = 11, in = 0 */
#define PHOENIX_DRAM_PLL_RATIO_133 0x2f	/* out = 1, feedback =  7, in = 1 */
#define PHOENIX_DRAM_PLL_RATIO_166 0x33	/* out = 1, feedback =  9, in = 1 */
#define PHOENIX_DRAM_PLL_RATIO_200 0x37	/* out = 1, feedback = 11, in = 1 */
#define PHOENIX_DRAM_PLL_RATIO_233 0x0d	/* out = 0, feedback =  6, in = 1 */
#define PHOENIX_DRAM_PLL_RATIO_266 0x0f	/* out = 0, feedback =  7, in = 1 */
#define PHOENIX_DRAM_PLL_RATIO_300 0x11	/* out = 0, feedback =  8, in = 1 */
#define PHOENIX_DRAM_PLL_RATIO_333 0x13	/* out = 0, feedback =  9, in = 1 */
#define PHOENIX_DRAM_PLL_RATIO_366 0x15	/* out = 0, feedback = 10, in = 1 */
#define PHOENIX_DRAM_PLL_RATIO_400 0x17	/* out = 0, feedback = 11, in = 1 */

#define XLS_DRAM_PLL_RATIO_100 0x42f /* DIVQ = 16; DIVF = 48 */
#define XLS_DRAM_PLL_RATIO_133 0x43f /* DIVQ = 16; DIVF = 64 */
#define XLS_DRAM_PLL_RATIO_166 0x44f /* DIVQ = 16; DIVF = 80 */
#define XLS_DRAM_PLL_RATIO_200 0x32f /* DIVQ =  8; DIVF = 48 */
#define XLS_DRAM_PLL_RATIO_233 0x337 /* DIVQ =  8; DIVF = 56 */
#define XLS_DRAM_PLL_RATIO_266 0x33f /* DIVQ =  8; DIVF = 64 */
#define XLS_DRAM_PLL_RATIO_300 0x347 /* DIVQ =  8; DIVF = 72 */
#define XLS_DRAM_PLL_RATIO_333 0x34f /* DIVQ =  8; DIVF = 80 */
#define XLS_DRAM_PLL_RATIO_366 0x357 /* DIVQ =  8; DIVF = 88 */
#define XLS_DRAM_PLL_RATIO_400 0x22f /* DIVQ =  4; DIVF = 48 */


#define NO_OVERRIDE_ADD_LAT_MAGIC_NUM 99

/* 72-bit mode. Address spaces 0x1000, 0x2000, 0x3000, 0x4000 enabled. 
   Interleave on PA[5] or PA[6:5]. Change interleave bit here. */
#define MEM_CTL_DDR72_INITIAL_BRIDGE_CFG 0x0011 

/* 36-bit mode. Address spaces 0x1000, 0x2000, 0x3000, 0x4000 enabled. 
   Interleave on PA[5] or PA[6:5]. Change interleave bit here.*/
#define MEM_CTL_DDR36_INITIAL_BRIDGE_CFG 0x0001 

/* 1x72 mode. Address space 0x1000 enabled. No interleaving. */
#define MEM_CTL_XLS2XX_DDR72_INITIAL_BRIDGE_CFG 0x0011    

/* 1x36 mode. Address space 0x1000 enabled. No interleaving. */
#define MEM_CTL_XLS2XX_DDR36_INITIAL_BRIDGE_CFG 0x0001    

/* 2x36 mode. Address spaces 0x1000, 0x2000, 0x3000, 0x4000 enabled. 
   Channels A and B populated on XLS6xx or XLS4xx. Interleave on PA[5] */
#define MEM_CTL_XLS4XX_XLS6XX_DDR36_AB_BRIDGE_CFG 0x1001

#define MEM_CTL_RLD2_INITIAL_BRIDGE_CFG  0x012

/* Hardcoded field values (i.e. not calculated from SPD, and not provided as an input parameter to the user */
#define MEM_CTL_GLB_PARAMS_ORWR                 0
#define MEM_CTL_GLB_PARAMS_SES                  0
#define MEM_CTL_GLB_PARAMS_CLK_SCM              1
#define MEM_CTL_GLB_PARAMS_OPP                  0
#define MEM_CTL_GLB_PARAMS_CPO                  1
#define MEM_CTL_GLB_PARAMS_ODT2_MUX_EN_L        1

#define MEM_CTL_CMD_DELAY_STAGE_CONFIG_VALUE    0x8
#define MEM_CTL_RX_DELAY_STAGE_CONFIG_VALUE     0x6
#define MEM_CTL_TX_DELAY_STAGE_CONFIG_VALUE     0x6

/* The next four defines assume that on this board, for this channel:
    -XLR's DDR[A/B/C/D]_CS0_L pin (if connected) is connected to rank0 of the DIMM whose SPD EEPROM address was supplied in the dimm0_spd_addr parameter.
    -XLR's DDR[A/B/C/D]_CS1_L pin (if connected) is connected to rank1 of the DIMM whose SPD EEPROM address was supplied in the dimm0_spd_addr parameter.
    -XLR's DDR[A/B/C/D]_CS2_L pin (if connected) is connected to rank0 of the DIMM whose SPD EEPROM address was supplied in the dimm1_spd_addr parameter.
    -XLR's DDR[A/B/C/D]_CS3_L pin (if connected) is connected to rank1 of the DIMM whose SPD EEPROM address was supplied in the dimm1_spd_addr parameter.
   If this software is running on a board where that is not true, these defines will have to be adjusted accordingly. */
#define MEM_CTL_72_DEFAULT_SPD0_RANK0_CS_PIN 0
#define MEM_CTL_72_DEFAULT_SPD0_RANK1_CS_PIN 1
#define MEM_CTL_72_DEFAULT_SPD1_RANK0_CS_PIN 2
#define MEM_CTL_72_DEFAULT_SPD1_RANK1_CS_PIN 3

#define MEM_CTL_36AC_DEFAULT_SPD0_RANK0_CS_PIN 0
#define MEM_CTL_36AC_DEFAULT_SPD0_RANK1_CS_PIN 1
#define MEM_CTL_36AC_DEFAULT_SPD1_RANK0_CS_PIN 2
#define MEM_CTL_36AC_DEFAULT_SPD1_RANK1_CS_PIN 3

#define MEM_CTL_36BD_DEFAULT_SPD0_RANK0_CS_PIN 2
#define MEM_CTL_36BD_DEFAULT_SPD0_RANK1_CS_PIN 3
#define MEM_CTL_36BD_DEFAULT_SPD1_RANK0_CS_PIN 0
#define MEM_CTL_36BD_DEFAULT_SPD1_RANK1_CS_PIN 1

/* The next four defines assume that on this board, for this channel, in 72-bit mode:
    -XLR's DDR[A/C]_ODT0 pin (if connected) is connected to rank0 of the DIMM whose SPD EEPROM address was supplied in the dimm0_spd_addr parameter.
    -XLR's DDR[A/C]_ODT1 pin (if connected) is connected to rank1 of the DIMM whose SPD EEPROM address was supplied in the dimm0_spd_addr parameter.
    -XLR's DDR[B/D]_ODT0 pin (if connected) is connected to rank0 of the DIMM whose SPD EEPROM address was supplied in the dimm1_spd_addr parameter.
    -XLR's DDR[B/D]_ODT1 pin (if connected) is connected to rank1 of the DIMM whose SPD EEPROM address was supplied in the dimm1_spd_addr parameter.
   If this software is running on a board where that is not true, these defines will have to be adjusted accordingly. */
#define MEM_CTL_72_DEFAULT_SPD0_RANK0_ODT_PIN 0
#define MEM_CTL_72_DEFAULT_SPD0_RANK1_ODT_PIN 1
#define MEM_CTL_72_DEFAULT_SPD1_RANK0_ODT_PIN 2
#define MEM_CTL_72_DEFAULT_SPD1_RANK1_ODT_PIN 3

#define MEM_CTL_36AC_DEFAULT_SPD0_RANK0_ODT_PIN 0
#define MEM_CTL_36AC_DEFAULT_SPD0_RANK1_ODT_PIN 1
#define MEM_CTL_36AC_DEFAULT_SPD1_RANK0_ODT_PIN 2
#define MEM_CTL_36AC_DEFAULT_SPD1_RANK1_ODT_PIN 3

#define MEM_CTL_36BD_DEFAULT_SPD0_RANK0_ODT_PIN 2
#define MEM_CTL_36BD_DEFAULT_SPD0_RANK1_ODT_PIN 3
#define MEM_CTL_36BD_DEFAULT_SPD1_RANK0_ODT_PIN 0
#define MEM_CTL_36BD_DEFAULT_SPD1_RANK1_ODT_PIN 1

#define MEM_CTL_ADDR_PARAMS_2_AP_LOC            10

#define MEM_CTL_PARAMS_1_tCCD_DDR2               2
#define MEM_CTL_PARAMS_1_tMRD                    2
#define MEM_CTL_PARAMS_1_tRRDwindow_NUM_CMDS     4

#define MEM_CTL_A0_PARAMS_3_HALF_EARLY            1
#define MEM_CTL_A0_PARAMS_3_HALF_LATE             0

#define MEM_CTL_DQS0_RX_DELAY_STAGE_CONFIG_HALF_EARLY          1
#define MEM_CTL_DQS0_RX_DELAY_STAGE_CONFIG_NOMINAL             1
#define MEM_CTL_DQS0_RX_DELAY_STAGE_CONFIG_HALF_LATE           0
#define MEM_CTL_DQS0_RX_DELAY_STAGE_CONFIG_CYC_LATE            0

#define MEM_CTL_DQS1_RX_DELAY_STAGE_CONFIG_HALF_EARLY          1
#define MEM_CTL_DQS1_RX_DELAY_STAGE_CONFIG_NOMINAL             1
#define MEM_CTL_DQS1_RX_DELAY_STAGE_CONFIG_HALF_LATE           0
#define MEM_CTL_DQS1_RX_DELAY_STAGE_CONFIG_CYC_LATE            0

#define MEM_CTL_DQS2_RX_DELAY_STAGE_CONFIG_HALF_EARLY          1
#define MEM_CTL_DQS2_RX_DELAY_STAGE_CONFIG_NOMINAL             1
#define MEM_CTL_DQS2_RX_DELAY_STAGE_CONFIG_HALF_LATE           0
#define MEM_CTL_DQS2_RX_DELAY_STAGE_CONFIG_CYC_LATE            0

#define MEM_CTL_DQS3_RX_DELAY_STAGE_CONFIG_HALF_EARLY          1
#define MEM_CTL_DQS3_RX_DELAY_STAGE_CONFIG_NOMINAL             1
#define MEM_CTL_DQS3_RX_DELAY_STAGE_CONFIG_HALF_LATE           0
#define MEM_CTL_DQS3_RX_DELAY_STAGE_CONFIG_CYC_LATE            0

#define MEM_CTL_DQS4_RX_DELAY_STAGE_CONFIG_HALF_EARLY          1
#define MEM_CTL_DQS4_RX_DELAY_STAGE_CONFIG_NOMINAL             1
#define MEM_CTL_DQS4_RX_DELAY_STAGE_CONFIG_HALF_LATE           0
#define MEM_CTL_DQS4_RX_DELAY_STAGE_CONFIG_CYC_LATE            0

#define MEM_CTL_DQS5_RX_DELAY_STAGE_CONFIG_HALF_EARLY          1
#define MEM_CTL_DQS5_RX_DELAY_STAGE_CONFIG_NOMINAL             1
#define MEM_CTL_DQS5_RX_DELAY_STAGE_CONFIG_HALF_LATE           0
#define MEM_CTL_DQS5_RX_DELAY_STAGE_CONFIG_CYC_LATE            0

#define MEM_CTL_DQS6_RX_DELAY_STAGE_CONFIG_HALF_EARLY          1
#define MEM_CTL_DQS6_RX_DELAY_STAGE_CONFIG_NOMINAL             1
#define MEM_CTL_DQS6_RX_DELAY_STAGE_CONFIG_HALF_LATE           0
#define MEM_CTL_DQS6_RX_DELAY_STAGE_CONFIG_CYC_LATE            0

#define MEM_CTL_DQS7_RX_DELAY_STAGE_CONFIG_HALF_EARLY          1
#define MEM_CTL_DQS7_RX_DELAY_STAGE_CONFIG_NOMINAL             1
#define MEM_CTL_DQS7_RX_DELAY_STAGE_CONFIG_HALF_LATE           0
#define MEM_CTL_DQS7_RX_DELAY_STAGE_CONFIG_CYC_LATE            0

#define MEM_CTL_DQS8_RX_DELAY_STAGE_CONFIG_HALF_EARLY          1
#define MEM_CTL_DQS8_RX_DELAY_STAGE_CONFIG_NOMINAL             1
#define MEM_CTL_DQS8_RX_DELAY_STAGE_CONFIG_HALF_LATE           0
#define MEM_CTL_DQS8_RX_DELAY_STAGE_CONFIG_CYC_LATE            0

#define MEM_CTL_DQS9_RX_DELAY_STAGE_CONFIG_HALF_EARLY          1
#define MEM_CTL_DQS9_RX_DELAY_STAGE_CONFIG_NOMINAL             1
#define MEM_CTL_DQS9_RX_DELAY_STAGE_CONFIG_HALF_LATE           0
#define MEM_CTL_DQS9_RX_DELAY_STAGE_CONFIG_CYC_LATE            0

#define MEM_CTL_PARAMS_4_DDR2_ODT_EN               1
#define MEM_CTL_PARAMS_4_tAOND                     2
#define MEM_CTL_PARAMS_4_CONTROLLER_TERM_EN        1
#define MEM_CTL_PARAMS_4_CONTROLLER_TERM_ALWAYS_EN 0

#define MEM_CTL_PARAMS_5_tXSRD                   200
#define MEM_CTL_PARAMS_5_REF_CMD_ISS_CNT         1

#define MEM_CTL_PARAMS_6_tRASMAX                 70000	/*NOTE: unit for this is ns, not cycles */
#define MEM_CTL_PARAMS_6_tXPRD                   6
#define MEM_CTL_PARAMS_6_tXARDS                  6
#define MEM_CTL_PARAMS_6_tCKE_PW_MIN             3

#define MEM_CTL_PARAMS_7_ALT_PARAM_RANK_MASK     0x7bde

#define MEM_CTL_MRS_BURST_TYPE                   0	/* Sequential */
#define MEM_CTL_MRS_TEST_MODE                    0
#define MEM_CTL_MRS_DLL_RESET                    0
#define MEM_CTL_MRS_PWRDWN_EXIT_MODE             0	/* Fast exit */

#define MEM_CTL_RDMRS_BURST_TYPE                 0	/* Sequential */
#define MEM_CTL_RDMRS_TEST_MODE                  0
#define MEM_CTL_RDMRS_DLL_RESET                  1
#define MEM_CTL_RDMRS_PWRDWN_EXIT_MODE           0	/* Fast exit */

#define MEM_CTL_EMRS1_DLL_EN                     0	/* Enable */
#define MEM_CTL_EMRS1_OUT_DRIVE_STRENGTH         0	/* 100% */
#define MEM_CTL_EMRS1_OCD_OPERATION              0	/* OCD exit */
#define MEM_CTL_EMRS1_RDQS_EN                    0	/* Not enabled */
#define MEM_CTL_EMRS1_OUT_EN                     0	/* Enabled */

#define MEM_CTL_TX0_THERM_CTRL_EN_CNT            1
#define MEM_CTL_TX0_THERM_CTRL_PRESET_VAL        0
#define MEM_CTL_TX0_THERM_CTRL_PRESET_N          0
#define MEM_CTL_TX0_THERM_CTRL_PRESET_P          0

#define MEM_CTL_RX0_THERM_CTRL_EN_CNT            1
#define MEM_CTL_RX0_THERM_CTRL_PRESET_VAL        0
#define MEM_CTL_RX0_THERM_CTRL_PRESET_N          0
#define MEM_CTL_RX0_THERM_CTRL_PRESET_P          0

#define MEM_CTL_DDR1_RESET_SEQ_CMD0              0x2f	/* PWR_DN_EX */
#define MEM_CTL_DDR1_RESET_SEQ_CMD0_CNT          0x0
#define MEM_CTL_DDR1_RESET_SEQ_CMD1              0x25	/* PRECHARGE_ALL */
#define MEM_CTL_DDR1_RESET_SEQ_CMD1_CNT          0x0

#define MEM_CTL_DDR1_RESET_SEQ_CMD2              0x01	/* ENABLE_DLL */
#define MEM_CTL_DDR1_RESET_SEQ_CMD2_CNT          0x0
#define MEM_CTL_DDR1_RESET_SEQ_CMD3              0x27	/* RESET_DLL */
#define MEM_CTL_DDR1_RESET_SEQ_CMD3_CNT          0x0

#define MEM_CTL_DDR1_RESET_SEQ_CMD4              0x2e	/* NOP */
#define MEM_CTL_DDR1_RESET_SEQ_CMD4_CNT          0xc8	/*...repeated for 200 clock cycles */
#define MEM_CTL_DDR1_RESET_SEQ_CMD5              0x25	/* PRECHARGE_ALL */
#define MEM_CTL_DDR1_RESET_SEQ_CMD5_CNT          0x0

#define MEM_CTL_DDR1_RESET_SEQ_CMD6              0x32	/* AUTO_REFRESH */
#define MEM_CTL_DDR1_RESET_SEQ_CMD6_CNT          0x0
#define MEM_CTL_DDR1_RESET_SEQ_CMD7              0x32	/* AUTO_REFRESH */
#define MEM_CTL_DDR1_RESET_SEQ_CMD7_CNT          0x0

#define MEM_CTL_DDR1_RESET_SEQ_LENGTH            8

#define MEM_CTL_DDR2_RESET_SEQ_CMD0              0x2f	/* PWR_DN_EX */
#define MEM_CTL_DDR2_RESET_SEQ_CMD0_CNT          0x0
#define MEM_CTL_DDR2_RESET_SEQ_CMD1              0x2e	/* NOP */
#define MEM_CTL_DDR2_RESET_SEQ_CMD1_CNT          0xa0	/* 400 ns wait, @DDR2-800 */

#define MEM_CTL_DDR2_RESET_SEQ_CMD2              0x25	/* PRECHARGE_ALL */
#define MEM_CTL_DDR2_RESET_SEQ_CMD2_CNT          0x0
#define MEM_CTL_DDR2_RESET_SEQ_CMD3              0x22	/* EMRS2 */
#define MEM_CTL_DDR2_RESET_SEQ_CMD3_CNT          0x0

#define MEM_CTL_DDR2_RESET_SEQ_CMD4              0x23	/* EMRS3 */
#define MEM_CTL_DDR2_RESET_SEQ_CMD4_CNT          0x0
#define MEM_CTL_DDR2_RESET_SEQ_CMD5              0x01	/* ENABLE_DLL */
#define MEM_CTL_DDR2_RESET_SEQ_CMD5_CNT          0x0

#define MEM_CTL_DDR2_RESET_SEQ_CMD6              0x27	/* RESET_DLL */
#define MEM_CTL_DDR2_RESET_SEQ_CMD6_CNT          0x0
#define MEM_CTL_DDR2_RESET_SEQ_CMD7              0x25	/* PRECHARGE_ALL */
#define MEM_CTL_DDR2_RESET_SEQ_CMD7_CNT          0x0

#define MEM_CTL_DDR2_RESET_SEQ_CMD8              0x32	/* AUTO_REFRESH */
#define MEM_CTL_DDR2_RESET_SEQ_CMD8_CNT          0x1
#define MEM_CTL_DDR2_RESET_SEQ_CMD9              0x20	/* MRS */
#define MEM_CTL_DDR2_RESET_SEQ_CMD9_CNT          0x0

#define MEM_CTL_DDR2_RESET_SEQ_CMD10             0x2e	/* NOP */
#define MEM_CTL_DDR2_RESET_SEQ_CMD10_CNT         0xc8	/* 200 cycles */
#define MEM_CTL_DDR2_RESET_SEQ_CMD11             0x03	/* DEFAULT_OCD */
#define MEM_CTL_DDR2_RESET_SEQ_CMD11_CNT         0x0

#define MEM_CTL_DDR2_RESET_SEQ_CMD12             0x21	/* EMRS1 */
#define MEM_CTL_DDR2_RESET_SEQ_CMD12_CNT         0x0
#define MEM_CTL_DDR2_RESET_SEQ_CMD13             0x2e	/* NOP */
#define MEM_CTL_DDR2_RESET_SEQ_CMD13_CNT         0x0

#define MEM_CTL_DDR2_RESET_SEQ_LENGTH            13
#define MEM_CTL_INT_RESET_DURATION_DIV_BY_16     20	/* Memory controller internal reset assertion  duration, in units of 16 clock cycles */

#define DDR2_SODIMM_DEF_TYPE                     0     /* unbuffered */
#define DDR2_MICRODIMM_DEF_TYPE                  0     /* unbuffered */

#define TX_MAX     64 /* Defines value range during TX strobe centering */
#define TX_STEP     1 /* Defines granularity during TX strobe centering */
#define RX_MAX     64 /* Defines value range during RX strobe centering */
#define ASCT_SIZE 170 /* Defines size of ASCT table */  
#define C_MAX     128


#endif
