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
#ifndef _BOARD_I2C_H_
#define _BOARD_I2C_H_

#include "boardconfig.h"

#ifdef __ASSEMBLY__
#define I2C_LW lwu
#define I2C_SW sw
#else
#define I2C_REGSIZE volatile unsigned int
extern I2C_REGSIZE *get_i2c_base(unsigned short bus);
#endif

/* Real time clock */
#define ARIZONA_RTC_BUS 1
#define I2C_RTC_BUS                         ARIZONA_RTC_BUS
#define ARIZONA_RTC_ADDR                    0x68
#define I2C_RTC_ADDR                        ARIZONA_RTC_ADDR
#define ARIZONA_RTC_BYTE0_OFFSET            0x0
#define ARIZONA_RTC_BYTE1_OFFSET            0x1
#define ARIZONA_RTC_BYTE2_OFFSET            0x2
#define ARIZONA_RTC_BYTE3_OFFSET            0x3
#define ARIZONA_TEMPSENSOR_BUS              1
#define ARIZONA_TEMPSENSOR_ADDR             0x4C

#define I2C_ADDR_TEMPSENSOR                 ARIZONA_TEMPSENSOR_ADDR
#define I2C_BUS_TEMPSENSOR                  ARIZONA_TEMPSENSOR_BUS

/* registers inside the temp sensor */
#define ARIZONA_TEMPSENSOR_INT_TEMP         0
#define INT_TEMP_OFFSET                     ARIZONA_TEMPSENSOR_INT_TEMP

#define ARIZONA_TEMPSENSOR_EXT_TEMP         1
#define EXT_TEMP_OFFSET                     ARIZONA_TEMPSENSOR_EXT_TEMP

#define ARIZONA_TEMPSENSOR_RDSTATUS         2
#define ARIZONA_TEMPSENSOR_RDCONFIG         3
#define ARIZONA_TEMPSENSOR_RDCONVRATE       4
#define ARIZONA_TEMPSENSOR_RDINTHI          5
#define ARIZONA_TEMPSENSOR_RDINTLO          6
#define ARIZONA_TEMPSENSOR_RDEXTHI          7
#define ARIZONA_TEMPSENSOR_RDEXTLO          8
#define ARIZONA_TEMPSENSOR_WRCONFIG         9
#define ARIZONA_TEMPSENSOR_WRCONVRATE       10
#define ARIZONA_TEMPSENSOR_WRINTHI          11
#define ARIZONA_TEMPSENSOR_WRINTLO          12
#define ARIZONA_TEMPSENSOR_WREXTHI          13
#define ARIZONA_TEMPSENSOR_WREXTLO          14
#define ARIZONA_TEMPSENSOR_ONESHOT          15
#define ARIZONA_TEMPSENSOR_RDEXTNDEXT       16
#define ARIZONA_TEMPSENSOR_RDEXTNDINT       17

#define ARIZONA_EEPROM_BUS                  1

#define I2C_EEPROM_BUS                      ARIZONA_EEPROM_BUS

#define ARIZONA_EEPROM_ADDR                 0x50
#define I2C_EEPROM_ADDR                     ARIZONA_EEPROM_ADDR

#define ARIZONA_EEPROM_SIGNATURE_OFFSET     0
#define ARIZONA_EEPROM_CKSUM_OFFSET         4
#define ARIZONA_EEPROM_BNAME_OFFSET         0x10
#define ARIZONA_EEPROM_MAJOR_OFFSET         0x18
#define MAJOR_OFFSET                        ARIZONA_EEPROM_MAJOR_OFFSET
#define ARIZONA_EEPROM_MINOR_OFFSET         0x19
#define MINOR_OFFSET                        ARIZONA_EEPROM_MINOR_OFFSET
#define ARIZONA_EEPROM_BOARDREV_OFFSET      0x1A
#define ARIZONA_EEPROM_BOARD_ID_OFFSET0     0x1C
#define ARIZONA_EEPROM_BOARD_ID_OFFSET1     0x1D
#define ARIZONA_EEPROM_AVAILABLE_OFFSET     0x1E
#define ARIZONA_EEPROM_ETH_MAC_OFFSET       0x20
#define ARIZONA_EEPROM_QDR2_OFFSET          0x26
#define ARIZONA_EEPROM_QDR2_SIZE_OFFSET     0x27
#define ARIZONA_EEPROM_RLD2A_OFFSET         0x28
#define ARIZONA_EEPROM_RLD2A_SIZE_OFFSET    0x29
#define ARIZONA_EEPROM_RLD2B_OFFSET         0x2a
#define ARIZONA_EEPROM_RLD2B_SIZE_OFFSET    0x2b

#ifdef RMI_EVAL_BOARD
#define SPD_BUS                     0x00

#define SPD_EEPROM0                 0x54
#define SPD_EEPROM1                 0x55
#define SPD_EEPROM2                 0x56
#define SPD_EEPROM3                 0x57

#elif defined CONFIG_FX
#define SPD_BUS                     0x00

#define SPD_EEPROM0                 0x50
#define SPD_EEPROM1                 0x52
#define SPD_EEPROM1_WA              0x54
#define SPD_EEPROM2                 0x54
#define SPD_EEPROM3                 0x56

#else

#define SPD_BUS                     0x00

#define SPD_EEPROM0                 0x00
#define SPD_EEPROM1                 0x00
#define SPD_EEPROM2                 0x00
#define SPD_EEPROM3                 0x00
#endif

// DDR config table
#define ARIZONA_SPD_DDR_NUM_BYTES                   0
#define ARIZONA_SPD_DDR_TOTAL_BYTES                 1
#define ARIZONA_SPD_MEM_TYPE                        2
#define ARIZONA_SPD_DDR_NUM_ROWS                    3
#define ARIZONA_SPD_DDR_NUM_COLS                    4
#define ARIZONA_SPD_DDR_NUM_PHYS_BANKS              5
#define ARIZONA_SPD_DDR_MODULE_WIDTH                6
#define ARIZONA_SPD_DDR_MODULE_WIDTH_EXT            7
#define ARIZONA_SPD_DDR_VOLTAGE_INTF                8
#define ARIZONA_SPD_DDR_MAX_CYCLE_TIME              9
#define ARIZONA_SPD_DDR_DRAM_tAC                    10
#define ARIZONA_SPD_DDR_DIMM_ECC_PARITY             11
#define ARIZONA_SPD_DDR_DIMM_REFRESH_RATE           12
#define ARIZONA_SPD_DDR_PRIMARY_WIDTH               13
#define ARIZONA_SPD_DDR_ERRORCK_WIDTH               14
#define ARIZONA_SPD_DDR_tCCD_MIN                    15
#define ARIZONA_SPD_DDR_BL_SUPPORTED                16
#define ARIZONA_SPD_DDR_NUM_BANKS                   17
#define ARIZONA_SPD_DDR_CASLAT                      18
#define ARIZONA_SPD_DDR_RESERVED19                  19
#define ARIZONA_SPD_DDR_DIMM_TYPE                   20
#define ARIZONA_SPD_DDR_BUF_REG                     21
#define ARIZONA_SPD_DDR_DEV_ATTR                    22
#define ARIZONA_SPD_DDR_DERATE_FREQ_CL_DERATED_HALF 23
#define ARIZONA_SPD_DDR_DERATE_tAC_CL_DERATED_HALF  24
#define ARIZONA_SPD_DDR_DERATE_FREQ_CL_DERATED_ONE  25
#define ARIZONA_SPD_DDR_DERATE_tAC_CL_DERATED_ONE   26
#define ARIZONA_SPD_DDR_tRP                         27
#define ARIZONA_SPD_DDR_tRRD                        28
#define ARIZONA_SPD_DDR_tRCD                        29
#define ARIZONA_SPD_DDR_tRAS                        30
#define ARIZONA_SPD_DDR_DENSITY                     31
#define ARIZONA_SPD_DDR_tIS                         32
#define ARIZONA_SPD_DDR_tIH                         33
#define ARIZONA_SPD_DDR_tDS                         34
#define ARIZONA_SPD_DDR_tDH                         35
#define ARIZONA_SPD_DDR_tRC                         41
#define ARIZONA_SPD_DDR_tRFC                        42
#define ARIZONA_SPD_DDR_tCK_MAX                     43
#define ARIZONA_SPD_DDR_tDQSQ                       44
#define ARIZONA_SPD_DDR_tQHS                        45
#define ARIZONA_SPD_DDR_PLL_RELOCK                  46

#endif
