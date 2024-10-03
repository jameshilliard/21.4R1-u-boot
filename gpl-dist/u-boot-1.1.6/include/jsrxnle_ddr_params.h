/*
 * Copyright (c) 2007-2011, Juniper Networks, Inc.
 * All rights reserved.
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

/* Hard coded DRAM configurations */

/* 8x1Gb Qimonda */
#define LMC_MEM_CFG0_8x1Gb_QIMONDA              0x20000865
#define LMC_MEM_CFG1_8x1Gb_QIMONDA              0x2ad2a48c
#define LMC_CTL_8x1Gb_QIMONDA                   0x007c0659
#define LMC_CTL1_8x1Gb_QIMONDA                  0x00000200
#define LMC_DDR2_CTL_8x1Gb_QIMONDA              0xc0e49301
#define LMC_COMP_CTL_8x1Gb_QIMONDA              0xf000f000
#define LMC_WODT_CTL_8x1Gb_QIMONDA              0x00000000
#define LMC_RODT_CTL_8x1Gb_QIMONDA              0x00000000
#define LMC_DELAY_CFG_8x1Gb_QIMONDA             0x00000000
#define LMC_RODT_COMP_CTL_8x1Gb_QIMONDA         0x00010103
#define LMC_PLL_CTL_8x1Gb_QIMONDA               0x0417c110

/* 4x1Gb Qimonda */
#define LMC_MEM_CFG0_4x1Gb_QIMONDA              0x00000845
#define LMC_MEM_CFG1_4x1Gb_QIMONDA              0x28cea48c
#define LMC_CTL_4x1Gb_QIMONDA                   0x007c0659
#define LMC_CTL1_4x1Gb_QIMONDA                  0x00000200
#define LMC_DDR2_CTL_4x1Gb_QIMONDA              0xc0e49301
#define LMC_COMP_CTL_4x1Gb_QIMONDA              0xf000f000
#define LMC_WODT_CTL_4x1Gb_QIMONDA              0x00000000
#define LMC_RODT_CTL_4x1Gb_QIMONDA              0x00000000
#define LMC_DELAY_CFG_4x1Gb_QIMONDA             0x00000000
#define LMC_RODT_COMP_CTL_4x1Gb_QIMONDA         0x00010103
#define LMC_PLL_CTL_4x1Gb_QIMONDA               0x0417c110

/* 8x512Mb Elpida */
#define LMC_MEM_CFG0_8x512Mb_ELPIDA             0x20000841
#define LMC_MEM_CFG1_8x512Mb_ELPIDA             0x2ace848c
#define LMC_CTL_8x512Mb_ELPIDA                  0x007c0659
#define LMC_CTL1_8x512Mb_ELPIDA                 0x00000200
#define LMC_DDR2_CTL_8x512Mb_ELPIDA             0x40e49301
#define LMC_COMP_CTL_8x512Mb_ELPIDA             0xf000f000
#define LMC_WODT_CTL_8x512Mb_ELPIDA             0x00000000
#define LMC_RODT_CTL_8x512Mb_ELPIDA             0x00000000
#define LMC_DELAY_CFG_8x512Mb_ELPIDA            0x00000000
#define LMC_RODT_COMP_CTL_8x512Mb_ELPIDA        0x00010103
#define LMC_PLL_CTL_8x512Mb_EPLIDA              0x0417c110

/* 8x1Gb Qimonda for srx210 proto 4 and above */
#define SRX210_P4_LMC_MEM_CFG0_8x1Gb_QIMONDA       0x20000865
#define SRX210_P4_LMC_MEM_CFG1_8x1Gb_QIMONDA       0x268e6468
#define SRX210_P4_LMC_CTL_8x1Gb_QIMONDA            0x007c0655
#define SRX210_P4_LMC_CTL1_8x1Gb_QIMONDA           0x00000200
#define SRX210_P4_LMC_DDR2_CTL_8x1Gb_QIMONDA       0xd4e47101
#define SRX210_P4_LMC_COMP_CTL_8x1Gb_QIMONDA       0xf000f000
#define SRX210_P4_LMC_WODT_CTL_8x1Gb_QIMONDA       0x00000012
#define SRX210_P4_LMC_RODT_CTL_8x1Gb_QIMONDA       0x00000000
#define SRX210_P4_LMC_DELAY_CFG_8x1Gb_QIMONDA      0x00000000
#define SRX210_P4_LMC_RODT_COMP_CTL_8x1Gb_QIMONDA  0x00010103
#define SRX210_P4_LMC_PLL_CTL_8x1Gb_QIMONDA        0x1417C110

/* 4x1Gb Qimonda for srx210 proto 4 and above */
#define SRX210_P4_LMC_MEM_CFG0_4x1Gb_QIMONDA       0x00000845
#define SRX210_P4_LMC_MEM_CFG1_4x1Gb_QIMONDA       0x268e6468
#define SRX210_P4_LMC_CTL_4x1Gb_QIMONDA            0x007c0655
#define SRX210_P4_LMC_CTL1_4x1Gb_QIMONDA           0x00000200
#define SRX210_P4_LMC_DDR2_CTL_4x1Gb_QIMONDA       0xd4e47101
#define SRX210_P4_LMC_COMP_CTL_4x1Gb_QIMONDA       0xf000f000
#define SRX210_P4_LMC_WODT_CTL_4x1Gb_QIMONDA       0x00000021
#define SRX210_P4_LMC_RODT_CTL_4x1Gb_QIMONDA       0x00000000
#define SRX210_P4_LMC_DELAY_CFG_4x1Gb_QIMONDA      0x00000000
#define SRX210_P4_LMC_RODT_COMP_CTL_4x1Gb_QIMONDA  0x00010103
#define SRX210_P4_LMC_PLL_CTL_4x1Gb_QIMONDA        0x1417C110

/* 4x1Gb Qimonda for SRX100 */
#define SRX100_LMC_MEM_CFG0_4x1Gb_QIMONDA       0x00000845
#define SRX100_LMC_MEM_CFG1_4x1Gb_QIMONDA       0x2892848c
#define SRX100_LMC_CTL_4x1Gb_QIMONDA            0x007c0655
#define SRX100_LMC_CTL1_4x1Gb_QIMONDA           0x00000200
#define SRX100_LMC_DDR2_CTL_4x1Gb_QIMONDA       0xd5249101
#define SRX100_LMC_COMP_CTL_4x1Gb_QIMONDA       0xf000f000
#define SRX100_LMC_WODT_CTL_4x1Gb_QIMONDA       0x00000021
#define SRX100_LMC_RODT_CTL_4x1Gb_QIMONDA       0x00000000
#define SRX100_LMC_DELAY_CFG_4x1Gb_QIMONDA      0x00000000
#define SRX100_LMC_RODT_COMP_CTL_4x1Gb_QIMONDA  0x00010103
#define SRX100_LMC_PLL_CTL_4x1Gb_QIMONDA        0x140fc104

/* 8x512Mb Elpida for SRX100 */
#define SRX100_LMC_MEM_CFG0_8x512Mb_ELPIDA      0x20000841
#define SRX100_LMC_MEM_CFG1_8x512Mb_ELPIDA      0x288e848c
#define SRX100_LMC_CTL_8x512Mb_ELPIDA           0x007c0655
#define SRX100_LMC_CTL1_8x512Mb_ELPIDA          0x00000200
#define SRX100_LMC_DDR2_CTL_8x512Mb_ELPIDA      0x55240101
#define SRX100_LMC_COMP_CTL_8x512Mb_ELPIDA      0xf000f000
#define SRX100_LMC_WODT_CTL_8x512Mb_ELPIDA      0x00000012
#define SRX100_LMC_RODT_CTL_8x512Mb_ELPIDA      0x00000000
#define SRX100_LMC_DELAY_CFG_8x512Mb_ELPIDA     0x00000000
#define SRX100_LMC_RODT_COMP_CTL_8x512Mb_ELPIDA 0x00010103
#define SRX100_LMC_LMC_PLL_CTL_8x1512Mb_ELPIDA  0x140fc104

/* 8x1Gb Qimonda for SRX100 */
#define SRX100_LMC_MEM_CFG0_8x1Gb_QIMONDA       0x20000865
#define SRX100_LMC_MEM_CFG1_8x1Gb_QIMONDA       0x2892848c
#define SRX100_LMC_CTL_8x1Gb_QIMONDA            0x007c0659
#define SRX100_LMC_CTL1_8x1Gb_QIMONDA           0x00000200
#define SRX100_LMC_DDR2_CTL_8x1Gb_QIMONDA       0xd5249101
#define SRX100_LMC_COMP_CTL_8x1Gb_QIMONDA       0xf000f000
#define SRX100_LMC_WODT_CTL_8x1Gb_QIMONDA       0x00000012
#define SRX100_LMC_RODT_CTL_8x1Gb_QIMONDA       0x00000000
#define SRX100_LMC_DELAY_CFG_8x1Gb_QIMONDA      0x00001c07
#define SRX100_LMC_RODT_COMP_CTL_8x1Gb_QIMONDA  0x00010103
#define SRX100_LMC_PLL_CTL_8x1Gb_QIMONDA        0x140fc104

/* 8x1Gb Qimonda for srx210 533Mhz DDR */
#define SRX210_533_LMC_MEM_CFG0_8x1Gb_QIMONDA       0x20000865
#define SRX210_533_LMC_MEM_CFG1_8x1Gb_QIMONDA       0x3892a6ac
#define SRX210_533_LMC_CTL_8x1Gb_QIMONDA            0x007c0655
#define SRX210_533_LMC_CTL1_8x1Gb_QIMONDA           0x00000200
#define SRX210_533_LMC_DDR2_CTL_8x1Gb_QIMONDA       0xd5229101
#define SRX210_533_LMC_COMP_CTL_8x1Gb_QIMONDA       0xf000f000
#define SRX210_533_LMC_WODT_CTL_8x1Gb_QIMONDA       0x00000012
#define SRX210_533_LMC_RODT_CTL_8x1Gb_QIMONDA       0x00000000
#define SRX210_533_LMC_DELAY_CFG_8x1Gb_QIMONDA      0x00000c04
#define SRX210_533_LMC_RODT_COMP_CTL_8x1Gb_QIMONDA  0x00010103
#define SRX210_533_LMC_PLL_CTL_8x1Gb_QIMONDA        0x140fc104

/* 4x1Gb Qimonda for srx210 533MHz DDR */
#define SRX210_533_LMC_MEM_CFG0_4x1Gb_QIMONDA       0x00000845
#define SRX210_533_LMC_MEM_CFG1_4x1Gb_QIMONDA       0x3892a6ac
#define SRX210_533_LMC_CTL_4x1Gb_QIMONDA            0x007c0655
#define SRX210_533_LMC_CTL1_4x1Gb_QIMONDA           0x00000200
#define SRX210_533_LMC_DDR2_CTL_4x1Gb_QIMONDA       0xd5229101
#define SRX210_533_LMC_COMP_CTL_4x1Gb_QIMONDA       0xf000f000
#define SRX210_533_LMC_WODT_CTL_4x1Gb_QIMONDA       0x00000021
#define SRX210_533_LMC_RODT_CTL_4x1Gb_QIMONDA       0x00000000
#define SRX210_533_LMC_DELAY_CFG_4x1Gb_QIMONDA      0x00000c04
#define SRX210_533_LMC_RODT_COMP_CTL_4x1Gb_QIMONDA  0x00010103
#define SRX210_533_LMC_PLL_CTL_4x1Gb_QIMONDA        0x140fc104

/* 8x1Gb Micron for SRX100 */
#define SRX100_LMC_MEM_CFG0_8x1Gb_MICRON        0x20000865
#define SRX100_LMC_MEM_CFG1_8x1Gb_MICRON        0x2892848c
#define SRX100_LMC_CTL_8x1Gb_MICRON             0x007c0655
#define SRX100_LMC_CTL1_8x1Gb_MICRON            0x00000200
#define SRX100_LMC_DDR2_CTL_8x1Gb_MICRON        0xd5249101
#define SRX100_LMC_COMP_CTL_8x1Gb_MICRON        0xf000f000
#define SRX100_LMC_WODT_CTL_8x1Gb_MICRON        0x00000012
#define SRX100_LMC_RODT_CTL_8x1Gb_MICRON        0x00000000
#define SRX100_LMC_DELAY_CFG_8x1Gb_MICRON       0x00001c07
#define SRX100_LMC_RODT_COMP_CTL_8x1Gb_MICRON   0x00010103
#define SRX100_LMC_PLL_CTL_8x1Gb_MICRON         0x140fc104

/* 8x2Gb Qimonda for SRX100 */
#define SRX100_LMC_MEM_CFG0_8x2Gb_QIMONDA       0x20000885
#define SRX100_LMC_MEM_CFG1_8x2Gb_QIMONDA       0x2892848c
#define SRX100_LMC_CTL_8x2Gb_QIMONDA            0x007c0654
#define SRX100_LMC_CTL1_8x2Gb_QIMONDA           0x00000200
#define SRX100_LMC_DDR2_CTL_8x2Gb_QIMONDA       0xd5249501 
#define SRX100_LMC_COMP_CTL_8x2Gb_QIMONDA       0xf000f000
#define SRX100_LMC_WODT_CTL_8x2Gb_QIMONDA       0x00000012
#define SRX100_LMC_RODT_CTL_8x2Gb_QIMONDA       0x00000000
#define SRX100_LMC_DELAY_CFG_8x2Gb_QIMONDA      0x00001405
#define SRX100_LMC_RODT_COMP_CTL_8x2Gb_QIMONDA  0x00010207
#define SRX100_LMC_PLL_CTL_8x2Gb_QIMONDA        0x040fc104

/* 8x2Gb Qimonda for srx210 533Mhz DDR */
#define SRX210_533_LMC_MEM_CFG0_8x2Gb_QIMONDA       0x20000885
#define SRX210_533_LMC_MEM_CFG1_8x2Gb_QIMONDA       0x3892a6ac
#define SRX210_533_LMC_CTL_8x2Gb_QIMONDA            0x007c0655
#define SRX210_533_LMC_CTL1_8x2Gb_QIMONDA           0x00000200
#define SRX210_533_LMC_DDR2_CTL_8x2Gb_QIMONDA       0xd5229101
#define SRX210_533_LMC_COMP_CTL_8x2Gb_QIMONDA       0xf000f000
#define SRX210_533_LMC_WODT_CTL_8x2Gb_QIMONDA       0x00000012
#define SRX210_533_LMC_RODT_CTL_8x2Gb_QIMONDA       0x00000000
#define SRX210_533_LMC_DELAY_CFG_8x2Gb_QIMONDA      0x00000c04
#define SRX210_533_LMC_RODT_COMP_CTL_8x2Gb_QIMONDA  0x00010103
#define SRX210_533_LMC_PLL_CTL_8x2Gb_QIMONDA        0x140fc104

/* 4x2Gb Qimonda for srx210 533MHz DDR */
#define SRX210_533_LMC_MEM_CFG0_4x2Gb_QIMONDA       0x00000865
#define SRX210_533_LMC_MEM_CFG1_4x2Gb_QIMONDA       0x3892a6ac
#define SRX210_533_LMC_CTL_4x2Gb_QIMONDA            0x007c0655
#define SRX210_533_LMC_CTL1_4x2Gb_QIMONDA           0x00000200
#define SRX210_533_LMC_DDR2_CTL_4x2Gb_QIMONDA       0xd5229101
#define SRX210_533_LMC_COMP_CTL_4x2Gb_QIMONDA       0xf000f000
#define SRX210_533_LMC_WODT_CTL_4x2Gb_QIMONDA       0x00000021
#define SRX210_533_LMC_RODT_CTL_4x2Gb_QIMONDA       0x00000000
#define SRX210_533_LMC_DELAY_CFG_4x2Gb_QIMONDA      0x00000c04
#define SRX210_533_LMC_RODT_COMP_CTL_4x2Gb_QIMONDA  0x00010103
#define SRX210_533_LMC_PLL_CTL_4x2Gb_QIMONDA        0x140fc104

#define QIMONDA_8x1Gb           0
#define QIMONDA_4x1Gb           1
#define ELPIDA_8x512Mb          2
#define QIMONDA_8x2Gb           3
#define QIMONDA_4x2Gb           4

#define HIGH_MEM_8x1Gb            0
#define LOW_MEM_4x1Gb             1
#define LOW_MEM_8x512Mb           2
#define HIGH_MEM_8x1Gb_SRX210_P4  3
#define LOW_MEM_4x1Gb_SRX210_P4   4
#define HIGH_MEM_8x1Gb_SRX100     5
#define LOW_MEM_4x1Gb_SRX100      6
#define LOW_MEM_8x512Gb_SRX100    7
#define HIGH_MEM_8x1Gb_SRX210_533 8
#define LOW_MEM_4x1Gb_SRX210_533  9
#define MICRON_8x1Gb_SRX100       10
#define HIGH_MEM_8x2Gb_SRX100     11
#define HIGH_MEM_8x2Gb_SRX210_533 12
#define LOW_MEM_4x2Gb_SRX210_533  13

#define DRAM_TYPE_SPD             0x80

#define LMC_PLL_CTL_FREQ_533    0x040FC104

/* DIMM details start here */

/*
 * SRX110 DRAM related defines.
 */
#define SRX110_DDR_BOARD_DELAY           4200
#define SRX110_LMC_DELAY_CLK             5
#define SRX110_LMC_DELAY_CMD             0
#define SRX110_LMC_DELAY_DQ              4

#define SRX110_UNB_DDR_BOARD_DELAY       2100
#define SRX110_UNB_LMC_DELAY_CLK         13
#define SRX110_UNB_LMC_DELAY_CMD         0
#define SRX110_UNB_LMC_DELAY_DQ          12

/*
 * SRX120 DRAM related defines.
 */
#define SRX120_DDR_BOARD_DELAY           4200
#define SRX120_LMC_DELAY_CLK             5
#define SRX120_LMC_DELAY_CMD             0
#define SRX120_LMC_DELAY_DQ              4

#define SRX120_UNB_DDR_BOARD_DELAY       2100
#define SRX120_UNB_LMC_DELAY_CLK         13
#define SRX120_UNB_LMC_DELAY_CMD         0
#define SRX120_UNB_LMC_DELAY_DQ          12

/*
 * SRX220 DRAM related defines.
 */
#define SRX220_DDR_BOARD_DELAY           4200
#define SRX220_LMC_DELAY_CLK             5
#define SRX220_LMC_DELAY_CMD             0
#define SRX220_LMC_DELAY_DQ              4

#define SRX220_UNB_DDR_BOARD_DELAY       2100
#define SRX220_UNB_LMC_DELAY_CLK         13
#define SRX220_UNB_LMC_DELAY_CMD         0
#define SRX220_UNB_LMC_DELAY_DQ          12


/* Vidar has one DRAM interfaces each with two DIMM sockets. DIMM 0
 * ** must be populated on at least one interface. Each interface used
 * ** must have at least DIMM 0 populated.
 * */
#define SRX240_DIMM_0_SPD_TWSI_ADDR    0x50

#define SRX240_DRAM_SOCKET_CONFIGURATION \
        {SRX240_DIMM_0_SPD_TWSI_ADDR, 0}

#define DRAM_SOCKET_CONFIGURATION       SRX240_DRAM_SOCKET_CONFIGURATION

#define SRX240_DDR_BOARD_DELAY           4200
#define SRX240_LMC_DELAY_CLK             5
#define SRX240_LMC_DELAY_CMD             0
#define SRX240_LMC_DELAY_DQ              4

#define SRX240_UNB_DDR_BOARD_DELAY       4600
#define SRX240_UNB_LMC_DELAY_CLK         15
#define SRX240_UNB_LMC_DELAY_CMD         0
#define SRX240_UNB_LMC_DELAY_DQ          10


/*-----------------------------------------------------------------------
 * DRAM Module Organization for PM3
 * 
 * PM3 can be configured to use two pairs of DIMM's, lower and
 * upper, providing a 128/144-bit interface or one to four DIMM's
 * providing a 64/72-bit interface.  This structure contains the TWSI
 * addresses used to access the DIMM's Serial Presence Detect (SPD)
 * EPROMS and it also implies which DIMM socket organization is used
 * on the board.  Software uses this to detect the presence of DIMM's
 * plugged into the sockets, compute the total memory capacity, and
 * configure DRAM controller.  All DIMM's must be identical.
 */

/* SPD EEPROM addresses on PM3 board */
#define SRX650_DIMM_0_SPD_TWSI_ADDR    0x50
#define SRX650_DIMM_1_SPD_TWSI_ADDR    0x51

#define SRX650_INTERFACE1_DIMM_0_SPD_TWSI_ADDR    0x52
#define SRX650_INTERFACE1_DIMM_1_SPD_TWSI_ADDR    0x53

/*
** CN56xx Registered configuration
*/
#define SRX650_DDR_BOARD_DELAY	4200
#define SRX650_LMC_DELAY_CLK		7
#define SRX650_LMC_DELAY_CMD		0
#define SRX650_LMC_DELAY_DQ		6

/*
** CN56xx Unbuffered configuration
*/
#define SRX650_UNB_DDR_BOARD_DELAY	4600
#define SRX650_UNB_LMC_DELAY_CLK		13
#define SRX650_UNB_LMC_DELAY_CMD  	0
#define SRX650_UNB_LMC_DELAY_DQ		8

typedef struct {
    uint64_t lmc_mem_cfg0;
    uint64_t lmc_mem_cfg1;
    uint64_t lmc_ctl;
    uint64_t lmc_ctl1;
    uint64_t lmc_ddr2_ctl;
    uint64_t lmc_comp_ctl;
    uint64_t lmc_wodt_ctl;
    uint64_t lmc_rodt_ctl;
    uint64_t lmc_delay_cfg;
    uint64_t lmc_rodt_comp_ctl;
    uint64_t lmc_pll_ctl;
} lmc_reg_val_t;

typedef struct {
    int row_bits;
    int col_bits;
    int num_banks;
    int num_ranks;
    int dram_width;
} ddr_config_data_t;

