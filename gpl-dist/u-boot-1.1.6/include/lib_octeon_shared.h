/***********************license start************************************
 * Copyright (c) 2004-2007  Cavium Networks (support@cavium.com). 
 * All rights reserved.
 * 
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 * 
 *     * Neither the name of Cavium Networks nor the names of
 *       its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.  
 * 
 * This Software, including technical data, may be subject to U.S.  export 
 * control laws, including the U.S.  Export Administration Act and its 
 * associated regulations, and may be subject to export or import regulations 
 * in other countries.  You warrant that You will comply strictly in all 
 * respects with all such regulations and acknowledge that you have the 
 * responsibility to obtain licenses to export, re-export or import the 
 * Software.  
 * 
 * TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS" 
 * AND WITH ALL FAULTS AND CAVIUM NETWORKS MAKES NO PROMISES, REPRESENTATIONS 
 * OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH 
 * RESPECT TO THE SOFTWARE, INCLUDING ITS CONDITION, ITS CONFORMITY TO ANY 
 * REPRESENTATION OR DESCRIPTION, OR THE EXISTENCE OF ANY LATENT OR PATENT 
 * DEFECTS, AND CAVIUM SPECIFICALLY DISCLAIMS ALL IMPLIED (IF ANY) WARRANTIES 
 * OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A PARTICULAR 
 * PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET 
 * POSSESSION OR CORRESPONDENCE TO DESCRIPTION.  THE ENTIRE RISK ARISING OUT 
 * OF USE OR PERFORMANCE OF THE SOFTWARE LIES WITH YOU.
 * 
 ***********************license end**************************************/


#ifndef __LIB_OCTEON_SHARED_H__
#define __LIB_OCTEON_SHARED_H__

#define CALC_SCALER 10000

/* Structure for bootloader pci IO buffers */
typedef struct
{
    uint32_t owner;
    uint32_t len;
    char data[0];
} octeon_pci_io_buf_t;
#define BOOTLOADER_PCI_READ_BUFFER_STR_LEN  (BOOTLOADER_PCI_READ_BUFFER_SIZE - 8)
#define BOOTLOADER_PCI_WRITE_BUFFER_STR_LEN  (BOOTLOADER_PCI_WRITE_BUFFER_SIZE - 8)

#define BOOTLOADER_PCI_READ_BUFFER_OWNER_ADDR   (BOOTLOADER_PCI_READ_BUFFER_BASE + 0)
#define BOOTLOADER_PCI_READ_BUFFER_LEN_ADDR   (BOOTLOADER_PCI_READ_BUFFER_BASE + 4)
#define BOOTLOADER_PCI_READ_BUFFER_DATA_ADDR   (BOOTLOADER_PCI_READ_BUFFER_BASE + 8)

#define OCTEON_TWSI_BUS_IN_ADDR_BIT       12
#define OCTEON_TWSI_BUS_IN_ADDR_MASK      (1 << OCTEON_TWSI_BUS_IN_ADDR_BIT)

enum octeon_pci_io_buf_owner
{
    OCTEON_PCI_IO_BUF_OWNER_INVALID = 0,  /* Must be zero, set when memory cleared */
    OCTEON_PCI_IO_BUF_OWNER_OCTEON = 1,
    OCTEON_PCI_IO_BUF_OWNER_HOST = 2,
};

/* data field addresses in the DDR2 SPD eeprom */
typedef enum ddr2_spd_addrs {
    DDR2_SPD_BYTES_PROGRAMMED	= 0,
    DDR2_SPD_TOTAL_BYTES	= 1,
    DDR2_SPD_MEM_TYPE		= 2,
    DDR2_SPD_NUM_ROW_BITS	= 3,
    DDR2_SPD_NUM_COL_BITS	= 4,
    DDR2_SPD_NUM_RANKS		= 5,
    DDR2_SPD_CYCLE_CLX		= 9,
    DDR2_SPD_CONFIG_TYPE	= 11,
    DDR2_SPD_REFRESH		= 12,
    DDR2_SPD_SDRAM_WIDTH	= 13,
    DDR2_SPD_BURST_LENGTH	= 16,
    DDR2_SPD_NUM_BANKS		= 17,
    DDR2_SPD_CAS_LATENCY	= 18,
    DDR2_SPD_DIMM_TYPE		= 20,
    DDR2_SPD_CYCLE_CLX1		= 23,
    DDR2_SPD_CYCLE_CLX2		= 25,
    DDR2_SPD_TRP		= 27,
    DDR2_SPD_TRRD 		= 28,
    DDR2_SPD_TRCD 		= 29,
    DDR2_SPD_TRAS 		= 30,
    DDR2_SPD_TWR 		= 36,
    DDR2_SPD_TWTR 		= 37,
    DDR2_SPD_TRFC_EXT		= 40,
    DDR2_SPD_TRFC 		= 42,
    DDR2_SPD_CHECKSUM		= 63,
    DDR2_SPD_MFR_ID		= 64
} ddr2_spd_addr_t;


/* data field addresses in the DDR2 SPD eeprom */
typedef enum ddr3_spd_addrs {
    DDR3_SPD_BYTES_PROGRAMMED                           =  0,
    DDR3_SPD_REVISION                                   =  1,
    DDR3_SPD_KEY_BYTE_DEVICE_TYPE                       =  2,
    DDR3_SPD_KEY_BYTE_MODULE_TYPE                       =  3,
    DDR3_SPD_DENSITY_BANKS                              =  4,
    DDR3_SPD_ADDRESSING_ROW_COL_BITS                    =  5,
    DDR3_SPD_NOMINAL_VOLTAGE                            =  6,
    DDR3_SPD_MODULE_ORGANIZATION                        =  7,
    DDR3_SPD_MEMORY_BUS_WIDTH                           =  8,
    DDR3_SPD_FINE_TIMEBASE_DIVIDEND_DIVISOR             =  9,
    DDR3_SPD_MEDIUM_TIMEBASE_DIVIDEND                   = 10,
    DDR3_SPD_MEDIUM_TIMEBASE_DIVISOR                    = 11,
    DDR3_SPD_MINIMUM_CYCLE_TIME_TCKMIN                  = 12,
    DDR3_SPD_CAS_LATENCIES_LSB                          = 14,
    DDR3_SPD_CAS_LATENCIES_MSB                          = 15,
    DDR3_SPD_MIN_CAS_LATENCY_TAAMIN                     = 16,
    DDR3_SPD_MIN_WRITE_RECOVERY_TWRMIN                  = 17,
    DDR3_SPD_MIN_RAS_CAS_DELAY_TRCDMIN                  = 18,
    DDR3_SPD_MIN_ROW_ACTIVE_DELAY_TRRDMIN               = 19,
    DDR3_SPD_MIN_ROW_PRECHARGE_DELAY_TRPMIN             = 20,
    DDR3_SPD_UPPER_NIBBLES_TRAS_TRC                     = 21,
    DDR3_SPD_MIN_ACTIVE_PRECHARGE_LSB_TRASMIN           = 22,
    DDR3_SPD_MIN_ACTIVE_REFRESH_LSB_TRCMIN              = 23,
    DDR3_SPD_MIN_REFRESH_RECOVERY_LSB_TRFCMIN           = 24,
    DDR3_SPD_MIN_REFRESH_RECOVERY_MSB_TRFCMIN           = 25,
    DDR3_SPD_MIN_INTERNAL_WRITE_READ_CMD_TWTRMIN        = 26,
    DDR3_SPD_MIN_INTERNAL_READ_PRECHARGE_CMD_TRTPMIN    = 27,
    DDR3_SPD_UPPER_NIBBLE_TFAW                          = 28,
    DDR3_SPD_MIN_FOUR_ACTIVE_WINDOW_TFAWMIN             = 29,
    DDR3_SPD_ADDRESS_MAPPING                            = 63,
    DDR3_SPD_CYCLICAL_REDUNDANCY_CODE_LOWER_NIBBLE      = 126,
    DDR3_SPD_CYCLICAL_REDUNDANCY_CODE_UPPER_NIBBLE      = 127
} ddr3_spd_addr_t;

/*
** DRAM Module Organization
**
** Octeon:
** Octeon can be configured to use two pairs of DIMM's, lower and
** upper, providing a 128/144-bit interface or one to four DIMM's
** providing a 64/72-bit interface.  This structure contains the TWSI
** addresses used to access the DIMM's Serial Presence Detect (SPD)
** EPROMS and it also implies which DIMM socket organization is used
** on the board.  Software uses this to detect the presence of DIMM's
** plugged into the sockets, compute the total memory capacity, and
** configure DRAM controller.  All DIMM's must be identical.
**
** CN31XX:
** Octeon CN31XX can be configured to use one to four DIMM's providing
** a 64/72-bit interface.  This structure contains the TWSI addresses
** used to access the DIMM's Serial Presence Detect (SPD) EPROMS and
** it also implies which DIMM socket organization is used on the
** board.  Software uses this to detect the presence of DIMM's plugged
** into the sockets, compute the total memory capacity, and configure
** DRAM controller.  All DIMM's must be identical.
*/

/* Structure that provides DIMM information, either in the form of an SPD TWSI address,
** or a pointer to an array that contains SPD data.  One of the two fields must be valid. */
typedef struct {
    uint8_t spd_addrs[2];    /* TWSI address of SPD, 0 if not used */
    uint8_t *spd_ptrs[2];   /* pointer to SPD data array, NULL if not used */
} dimm_config_t;


/* dimm_config_t value that terminates list */
#define DIMM_CONFIG_TERMINATOR  {{0,0}, {NULL, NULL}}

typedef struct {
    uint8_t odt_ena;
    uint64_t odt_mask;
    union {
        uint64_t u64;
        struct cvmx_lmcx_modereg_params1_s cn63xx;
    }  odt_mask1;
    uint8_t qs_dic;
    uint64_t rodt_ctl;
    uint8_t dic;
} dimm_odt_config_t;

typedef struct {
    uint32_t ddr_board_delay;
    uint8_t lmc_delay_clk;
    uint8_t lmc_delay_cmd;
    uint8_t lmc_delay_dq;
} ddr_delay_config_t;

#define DDR_CFG_T_MAX_DIMMS     2 

/*
 * Taken from cavium sdk-3.1 to make it
 * compatible for octeon3.
 */
typedef struct {
    uint8_t min_rtt_nom_idx;
    uint8_t max_rtt_nom_idx;
    uint8_t min_rodt_ctl;
    uint8_t max_rodt_ctl;
    uint8_t dqx_ctl;
    uint8_t ck_ctl;
    uint8_t cmd_ctl;
    uint8_t ctl_ctl;
    uint8_t min_cas_latency;
    uint8_t offset_en;
    uint8_t offset_udimm;
    uint8_t offset_rdimm;
    uint8_t rlevel_compute;
    uint8_t ddr_rtt_nom_auto;
    uint8_t ddr_rodt_ctl_auto;
    uint8_t rlevel_comp_offset;
    uint8_t rlevel_average_loops;
    uint8_t ddr2t_udimm;
    uint8_t ddr2t_rdimm;
    uint8_t disable_sequential_delay_check;
    uint8_t maximum_adjacent_rlevel_delay_increment;
    uint8_t parity;
    uint8_t fprch2;
    uint8_t mode32b;
    int8_t dll_write_offset;
    int8_t dll_read_offset;
} ddr3_custom_config_t;

typedef struct {
    dimm_config_t dimm_config_table[DDR_CFG_T_MAX_DIMMS];
    dimm_odt_config_t odt_1rank_config[4];
    dimm_odt_config_t odt_2rank_config[4];
    dimm_odt_config_t odt_4rank_config[4];
    ddr_delay_config_t unbuffered;
    ddr_delay_config_t registered;
    ddr3_custom_config_t custom_lmc_config;
} ddr_configuration_t;

#define MAX_DDR_CONFIGS_PER_BOARD    2
typedef struct
{
    uint32_t chip_model;  /* OCTEON model that this config is for */
    /* Maximum board revision level that this configuration supports. */
    uint8_t max_maj_rev; /* 0 means all revs */
    uint8_t max_min_rev;
    ddr_configuration_t ddr_config[2];  /* One config for each interface */
} ddr_config_table_t;

/* Entry for the board table.  This table has an entry for each board,
** and allows the configuration to be looked up from the table.  The table
** is populated with static initializers, and is shared between u-boot and
** oct-remote-boot.
** Some fields are of interest to only u-boot or oct-remote-boot.
*/
typedef struct
{
    int board_type;
    int board_has_spd;
    ddr_config_table_t chip_ddr_config[MAX_DDR_CONFIGS_PER_BOARD];
    int default_ddr_clock_hz;
    int default_ddr_ref_hz;
    int default_cpu_ref_hz;
    int eeprom_addr;
    char bootloader_name[100];   /* Only used if default needs to be overridden */
} board_table_entry_t;

extern const dimm_odt_config_t disable_odt_config[];
extern const dimm_odt_config_t single_rank_odt_config[];
extern const dimm_odt_config_t dual_rank_odt_config[];
extern const board_table_entry_t octeon_board_ddr_config_table[];

int validate_dimm(dimm_config_t dimm_config, int dimm_index);
int init_octeon_dram_interface(uint32_t cpu_id,
                               ddr_configuration_t *ddr_configuration,
                               uint32_t ddr_hertz,
                               uint32_t cpu_hertz,
                               uint32_t ddr_ref_hertz,
                               int board_type,
                               int board_rev_maj,
                               int board_rev_min,
                               int ddr_interface_num,
                               uint32_t ddr_interface_mask);

int initialize_ddr_clock(uint32_t cpu_id,
                         const ddr_configuration_t *ddr_configuration,
                         uint32_t cpu_hertz,
                         uint32_t ddr_hertz,
                         uint32_t ddr_ref_hertz,
                         int ddr_interface_num,
                         uint32_t ddr_interface_mask);

uint32_t measure_octeon_ddr_clock(uint32_t cpu_id,
                                  const ddr_configuration_t *ddr_configuration,
                                  uint32_t cpu_hertz,
                                  uint32_t ddr_hertz,
                                  uint32_t ddr_ref_hertz,
                                  int ddr_interface_num,
                                  uint32_t ddr_interface_mask);

int twsii_mcu_read(uint8_t twsii_addr);
int octeon_twsi_read(uint8_t dev_addr, uint8_t twsii_addr);

int twsii_mcu_read(uint8_t twsii_addr);
int octeon_twsi_read16(uint8_t dev_addr, uint16_t addr);
int octeon_twsi_write16(uint8_t dev_addr, uint16_t addr, uint16_t data);
int octeon_twsi_read16_cur_addr(uint8_t dev_addr);
int octeon_twsi_read8(uint8_t dev_addr, uint16_t addr);
int octeon_twsi_write8(uint8_t dev_addr, uint8_t addr, uint8_t data);
int octeon_twsi_read8_cur_addr(uint8_t dev_addr);
int octeon_twsi_set_addr16(uint8_t dev_addr, uint16_t addr);

int octeon_tlv_get_tuple_addr(uint8_t dev_addr, uint16_t type, uint8_t major_version, uint8_t *eeprom_buf, uint32_t buf_len);

int  octeon_tlv_eeprom_get_next_tuple(uint8_t dev_addr, uint16_t addr, uint8_t *buf_ptr, uint32_t buf_len);

#ifdef ENABLE_BOARD_DEBUG
void gpio_status(int data);
#endif

#define CN31XX_DRAM_ODT_1RANK_CONFIGURATION \
    /* DIMMS   ODT_ENA  WODT_CTL0     WODT_CTL1   QS_DIC RODT_CTL DIC */ \
    /* =====   ======= ============ ============= ====== ======== === */ \
    /*   1 */ {   0,    0x00000100,   0x00000000,    1,   0x0000,  0  },  \
    /*   2 */ {   0,    0x01000400,   0x00000000,    1,   0x0000,  0  },  \
    /*   3 */ {   0,    0x01000400,   0x00000400,    2,   0x0000,  0  },  \
    /*   4 */ {   0,    0x01000400,   0x04000400,    2,   0x0000,  0  }

#define CN31XX_DRAM_ODT_2RANK_CONFIGURATION \
    /* DIMMS   ODT_ENA  WODT_CTL0     WODT_CTL1   QS_DIC RODT_CTL DIC */ \
    /* =====   ======= ============ ============= ====== ======== === */ \
    /*   1 */ {   0,    0x00000101,   0x00000000,    1,   0x0000,  0  },  \
    /*   2 */ {   0,    0x01010404,   0x00000000,    1,   0x0000,  0  },  \
    /*   3 */ {   0,    0x01010404,   0x00000404,    2,   0x0000,  0  },  \
    /*   4 */ {   0,    0x01010404,   0x04040404,    2,   0x0000,  0  }

/* CN30xx is the same as CN31xx */
#define CN30XX_DRAM_ODT_1RANK_CONFIGURATION CN31XX_DRAM_ODT_1RANK_CONFIGURATION
#define CN30XX_DRAM_ODT_2RANK_CONFIGURATION CN31XX_DRAM_ODT_2RANK_CONFIGURATION

#define CN38XX_DRAM_ODT_1RANK_CONFIGURATION \
    /* DIMMS   ODT_ENA LMC_ODT_CTL  reserved  QS_DIC RODT_CTL DIC */ \
    /* =====   ======= ============ ========= ====== ======== === */ \
    /*   1 */ {   0,    0x00000001,   0x0000,    1,   0x0000,  0  },  \
    /*   2 */ {   0,    0x00010001,   0x0000,    2,   0x0000,  0  },  \
    /*   3 */ {   0,    0x01040104,   0x0000,    2,   0x0000,  0  },  \
    /*   4 */ {   0,    0x01040104,   0x0000,    2,   0x0000,  0  }

#define CN38XX_DRAM_ODT_2RANK_CONFIGURATION \
    /* DIMMS   ODT_ENA LMC_ODT_CTL  reserved  QS_DIC RODT_CTL DIC */ \
    /* =====   ======= ============ ========= ====== ======== === */ \
    /*   1 */ {   0,    0x00000011,   0x0000,    1,   0x0000,  0  },  \
    /*   2 */ {   0,    0x00110011,   0x0000,    2,   0x0000,  0  },  \
    /*   3 */ {   0,    0x11441144,   0x0000,    3,   0x0000,  0  },  \
    /*   4 */ {   0,    0x11441144,   0x0000,    3,   0x0000,  0  }

/* Note: CN58XX RODT_ENA 0 = disabled, 1 = Weak Read ODT, 2 = Strong Read ODT */
#define CN58XX_DRAM_ODT_1RANK_CONFIGURATION \
    /* DIMMS   RODT_ENA LMC_ODT_CTL  reserved  QS_DIC RODT_CTL DIC */ \
    /* =====   ======== ============ ========= ====== ======== === */ \
    /*   1 */ {   2,    0x00000001,   0x0000,    1,   0x0000,  0  },  \
    /*   2 */ {   2,    0x00010001,   0x0000,    2,   0x0000,  0  },  \
    /*   3 */ {   2,    0x01040104,   0x0000,    2,   0x0000,  0  },  \
    /*   4 */ {   2,    0x01040104,   0x0000,    2,   0x0000,  0  }

/* Note: CN58XX RODT_ENA 0 = disabled, 1 = Weak Read ODT, 2 = Strong Read ODT */
#define CN58XX_DRAM_ODT_2RANK_CONFIGURATION \
    /* DIMMS   RODT_ENA LMC_ODT_CTL  reserved  QS_DIC RODT_CTL DIC */ \
    /* =====   ======== ============ ========= ====== ======== === */ \
    /*   1 */ {   2,    0x00000011,   0x0000,    1,   0x0000,  0  },  \
    /*   2 */ {   2,    0x00110011,   0x0000,    2,   0x0000,  0  },  \
    /*   3 */ {   2,    0x11441144,   0x0000,    3,   0x0000,  0  },  \
    /*   4 */ {   2,    0x11441144,   0x0000,    3,   0x0000,  0  }

/* Note: CN50XX RODT_ENA 0 = disabled, 1 = Weak Read ODT, 2 = Strong Read ODT */
#define CN50XX_DRAM_ODT_1RANK_CONFIGURATION \
    /* DIMMS   RODT_ENA LMC_ODT_CTL  reserved  QS_DIC RODT_CTL DIC */ \
    /* =====   ======== ============ ========= ====== ======== === */ \
    /*   1 */ {   2,    0x00000001,   0x0000,    1,   0x0000,  0  },  \
    /*   2 */ {   2,    0x00010001,   0x0000,    2,   0x0000,  0  },  \
    /*   3 */ {   2,    0x01040104,   0x0000,    2,   0x0000,  0  },  \
    /*   4 */ {   2,    0x01040104,   0x0000,    2,   0x0000,  0  }

/* Note: CN50XX RODT_ENA 0 = disabled, 1 = Weak Read ODT, 2 = Strong Read ODT */
#define CN50XX_DRAM_ODT_2RANK_CONFIGURATION \
    /* DIMMS   RODT_ENA LMC_ODT_CTL  reserved  QS_DIC RODT_CTL DIC */ \
    /* =====   ======== ============ ========= ====== ======== === */ \
    /*   1 */ {   2,    0x00000011,   0x0000,    1,   0x0000,  0  },  \
    /*   2 */ {   2,    0x00110011,   0x0000,    2,   0x0000,  0  },  \
    /*   3 */ {   2,    0x11441144,   0x0000,    3,   0x0000,  0  },  \
    /*   4 */ {   2,    0x11441144,   0x0000,    3,   0x0000,  0  }

/* Note: CN56XX RODT_ENA 0 = disabled, 1 = Weak Read ODT, 2 = Strong Read ODT */
#define CN56XX_DRAM_ODT_1RANK_CONFIGURATION \
    /* DIMMS   RODT_ENA  WODT_CTL0     WODT_CTL1   QS_DIC RODT_CTL DIC */ \
    /* =====   ======== ============ ============= ====== ======== === */ \
    /*   1 */ {   2,     0x00000100,   0x00000000,    1,   0x0000,  0  },  \
    /*   2 */ {   2,     0x01000400,   0x00000000,    1,   0x0000,  0  },  \
    /*   3 */ {   2,     0x01000400,   0x00000400,    2,   0x0000,  0  },  \
    /*   4 */ {   2,     0x01000400,   0x04000400,    2,   0x0000,  0  }

/* Note: CN56XX RODT_ENA 0 = disabled, 1 = Weak Read ODT, 2 = Strong Read ODT */
#define CN56XX_DRAM_ODT_2RANK_CONFIGURATION \
    /* DIMMS   RODT_ENA  WODT_CTL0     WODT_CTL1   QS_DIC RODT_CTL DIC */ \
    /* =====   ======== ============ ============= ====== ======== === */ \
    /*   1 */ {   2,     0x00000101,   0x00000000,    1,   0x0000,  0  },  \
    /*   2 */ {   2,     0x01010404,   0x00000000,    1,   0x0000,  0  },  \
    /*   3 */ {   2,     0x01010404,   0x00000404,    2,   0x0000,  0  },  \
    /*   4 */ {   2,     0x01010404,   0x04040404,    2,   0x0000,  0  }

/* Note: CN52XX RODT_ENA 0 = disabled, 1 = Weak Read ODT, 2 = Strong Read ODT */
#define CN52XX_DRAM_ODT_1RANK_CONFIGURATION \
    /* DIMMS   RODT_ENA  WODT_CTL0     WODT_CTL1   QS_DIC RODT_CTL DIC */ \
    /* =====   ======== ============ ============= ====== ======== === */ \
    /*   1 */ {   2,     0x00000100,   0x00000000,    1,   0x0000,  0  },  \
    /*   2 */ {   2,     0x01000400,   0x00000000,    1,   0x0000,  0  },  \
    /*   3 */ {   2,     0x01000400,   0x00000400,    2,   0x0000,  0  },  \
    /*   4 */ {   2,     0x01000400,   0x04000400,    2,   0x0000,  0  }

/* Note: CN52XX RODT_ENA 0 = disabled, 1 = Weak Read ODT, 2 = Strong Read ODT */
#define CN52XX_DRAM_ODT_2RANK_CONFIGURATION \
    /* DIMMS   RODT_ENA  WODT_CTL0     WODT_CTL1   QS_DIC RODT_CTL DIC */ \
    /* =====   ======== ============ ============= ====== ======== === */ \
    /*   1 */ {   2,     0x00000101,   0x00000000,    1,   0x0000,  0  },  \
    /*   2 */ {   2,     0x01010404,   0x00000000,    1,   0x0000,  0  },  \
    /*   3 */ {   2,     0x01010404,   0x00000404,    2,   0x0000,  0  },  \
    /*   4 */ {   2,     0x01010404,   0x04040404,    2,   0x0000,  0  }



#define CN63XX_DRAM_ODT_1RANK_CONFIGURATION \
    /* DIMMS   reserved  WODT_MASK     LMCX_MODEREG_PARAMS1         RODT_CTL    RODT_MASK    reserved */ \
    /* =====   ======== ============== ============================ ========= ============== ======== */ \
    /*   1 */ {   0,    0x00000001ULL, MODEREG_PARAMS1_1RANK_1SLOT,    4,     0x00000000ULL,  0  },      \
    /*   2 */ {   0,    0x00050005ULL, MODEREG_PARAMS1_1RANK_2SLOT,    4,     0x00010004ULL,  0  }

#define CN63XX_DRAM_ODT_2RANK_CONFIGURATION \
    /* DIMMS   reserved  WODT_MASK     LMCX_MODEREG_PARAMS1         RODT_CTL    RODT_MASK    reserved */ \
    /* =====   ======== ============== ============================ ========= ============== ======== */ \
    /*   1 */ {   0,    0x00000101ULL, MODEREG_PARAMS1_2RANK_1SLOT,    4,     0x00000000ULL,    0  },    \
    /*   2 */ {   0,    0x06060909ULL, MODEREG_PARAMS1_2RANK_2SLOT,    4,     0x02020808ULL,    0  }

#define TWSI_VALID_BIT  0x8000000000000000ull
#define TWSI_STATUS_BIT 0x0100000000000000ull
#define TWSI_TIMEOUT_CYCLES 1000000
#define TWSI_TIMEOUT_COUNT  1000
#define MIO_TWS_SW_TWSI_REG_OPS_SHIFT         57
#define MIO_TWS_SW_TWSI_REG_SOVR_SHIFT        55
#define MIO_TWS_SW_TWSI_REG_SIZE_SHIFT        52
#define MIO_TWS_SW_TWSI_REG_DEV_ADDR_SHIFT    40
#define MIO_TWS_SW_TWSI_REG_READ_OR_RES_SHIFT 56
#define TWSI_NO_OFFSET 0xff


/**
 * Enumeration of different Clocks in Octeon.
 */
typedef enum{
    CVMX_CLOCK_RCLK,        /**< Clock used by cores, coherent bus and L2 cache. */
    CVMX_CLOCK_SCLK,        /**< Clock used by IO blocks. */
    CVMX_CLOCK_DDR,         /**< Clock used by DRAM */
    CVMX_CLOCK_CORE,        /**< Alias for CVMX_CLOCK_RCLK */
    CVMX_CLOCK_TIM,         /**< Alias for CVMX_CLOCK_SCLK */
    CVMX_CLOCK_IPD,         /**< Alias for CVMX_CLOCK_SCLK */
} cvmx_clock_t;

/**
 * Get cycle count based on the clock type.
 *
 * @param clock - Enumeration of the clock type.
 * @return      - Get the number of cycles executed so far.
 */
static inline uint64_t cvmx_clock_get_count(cvmx_clock_t clock)
{
    switch(clock)
    {
        case CVMX_CLOCK_RCLK:
        case CVMX_CLOCK_CORE:
        {
#ifndef __mips__
            return cvmx_read_csr(CVMX_IPD_CLK_COUNT);
#elif defined(CVMX_ABI_O32)
            uint32_t tmp_low, tmp_hi;

            asm volatile (
               "   .set push                    \n"
               "   .set mips64r2                \n"
               "   .set noreorder               \n"
               "   rdhwr %[tmpl], $31           \n"
               "   dsrl  %[tmph], %[tmpl], 32   \n"
               "   sll   %[tmpl], 0             \n"
               "   sll   %[tmph], 0             \n"
               "   .set pop                 \n"
                  : [tmpl] "=&r" (tmp_low), [tmph] "=&r" (tmp_hi) : );

            return(((uint64_t)tmp_hi << 32) + tmp_low);
#else
            uint64_t cycle;
            CVMX_RDHWR(cycle, 31);
            return(cycle);
#endif
        }

        case CVMX_CLOCK_SCLK:
        case CVMX_CLOCK_TIM:
        case CVMX_CLOCK_IPD:
            return cvmx_read_csr(CVMX_IPD_CLK_COUNT);

        case CVMX_CLOCK_DDR:
            if (OCTEON_IS_MODEL(OCTEON_CN6XXX))
                return cvmx_read_csr(CVMX_LMCX_DCLK_CNT(0));
            else
                return ((cvmx_read_csr(CVMX_LMCX_DCLK_CNT_HI(0)) << 32) | cvmx_read_csr(CVMX_LMCX_DCLK_CNT_LO(0)));
    }

    cvmx_dprintf("cvmx_clock_get_count: Unknown clock type\n");
    return 0;
}

#define rttnom_none   0         /* Rtt_Nom disabled */
#define rttnom_60ohm  1         /* RZQ/4  = 240/4  =  60 ohms */
#define rttnom_120ohm 2         /* RZQ/2  = 240/2  = 120 ohms */
#define rttnom_40ohm  3         /* RZQ/6  = 240/6  =  40 ohms */
#define rttnom_20ohm  4         /* RZQ/12 = 240/12 =  20 ohms */
#define rttnom_30ohm  5         /* RZQ/8  = 240/8  =  30 ohms */
#define rttnom_rsrv1  6         /* Reserved */
#define rttnom_rsrv2  7         /* Reserved */

#define rttwr_none    0         /* Dynamic ODT off */
#define rttwr_60ohm   1         /* RZQ/4  = 240/4  =  60 ohms */
#define rttwr_120ohm  2         /* RZQ/2  = 240/2  = 120 ohms */
#define rttwr_rsrv1   3         /* Reserved */


/* LMC0_MODEREG_PARAMS1 */
#define MODEREG_PARAMS1_1RANK_1SLOT             \
    { .cn63xx = { .pasr_00      = 0,            \
                  .asr_00       = 0,            \
                  .srt_00       = 0,            \
                  .rtt_wr_00    = 0,            \
                  .dic_00       = 0,            \
                  .rtt_nom_00   = rttnom_40ohm, \
                  .pasr_01      = 0,            \
                  .asr_01       = 0,            \
                  .srt_01       = 0,            \
                  .rtt_wr_01    = 0,            \
                  .dic_01       = 0,            \
                  .rtt_nom_01   = 0,            \
                  .pasr_10      = 0,            \
                  .asr_10       = 0,            \
                  .srt_10       = 0,            \
                  .rtt_wr_10    = 0,            \
                  .dic_10       = 0,            \
                  .rtt_nom_10   = 0,            \
                  .pasr_11      = 0,            \
                  .asr_11       = 0,            \
                  .srt_11       = 0,            \
                  .rtt_wr_11    = 0,            \
                  .dic_11       = 0,            \
                  .rtt_nom_11   = 0,            \
        }                                       \
    }

#define MODEREG_PARAMS1_1RANK_2SLOT             \
    { .cn63xx = { .pasr_00      = 0,            \
                  .asr_00       = 0,            \
                  .srt_00       = 0,            \
                  .rtt_wr_00    = rttwr_120ohm, \
                  .dic_00       = 0,            \
                  .rtt_nom_00   = rttnom_40ohm, \
                  .pasr_01      = 0,            \
                  .asr_01       = 0,            \
                  .srt_01       = 0,            \
                  .rtt_wr_01    = 0,            \
                  .dic_01       = 0,            \
                  .rtt_nom_01   = 0,            \
                  .pasr_10      = 0,            \
                  .asr_10       = 0,            \
                  .srt_10       = 0,            \
                  .rtt_wr_10    = rttwr_120ohm, \
                  .dic_10       = 0,            \
                  .rtt_nom_10   = rttnom_40ohm, \
                  .pasr_11      = 0,            \
                  .asr_11       = 0,            \
                  .srt_11       = 0,            \
                  .rtt_wr_11    = 0,            \
                  .dic_11       = 0,            \
                  .rtt_nom_11   = 0             \
        }                                       \
    }

#define MODEREG_PARAMS1_2RANK_1SLOT             \
    { .cn63xx = { .pasr_00      = 0,            \
                  .asr_00       = 0,            \
                  .srt_00       = 0,            \
                  .rtt_wr_00    = 0,            \
                  .dic_00       = 0,            \
                  .rtt_nom_00   = rttnom_40ohm, \
                  .pasr_01      = 0,            \
                  .asr_01       = 0,            \
                  .srt_01       = 0,            \
                  .rtt_wr_01    = 0,            \
                  .dic_01       = 0,            \
                  .rtt_nom_01   = 0,            \
                  .pasr_10      = 0,            \
                  .asr_10       = 0,            \
                  .srt_10       = 0,            \
                  .rtt_wr_10    = 0,            \
                  .dic_10       = 0,            \
                  .rtt_nom_10   = 0,            \
                  .pasr_11      = 0,            \
                  .asr_11       = 0,            \
                  .srt_11       = 0,            \
                  .rtt_wr_11    = 0,            \
                  .dic_11       = 0,            \
                  .rtt_nom_11   = 0,            \
        }                                       \
    }

#define MODEREG_PARAMS1_2RANK_2SLOT                     \
    { .cn63xx = { .pasr_00      = 0,                    \
                  .asr_00       = 0,                    \
                  .srt_00       = 0,                    \
                  .rtt_wr_00    = 0,                    \
                  .dic_00       = 0,                    \
                  .rtt_nom_00   = rttnom_120ohm,        \
                  .pasr_01      = 0,                    \
                  .asr_01       = 0,                    \
                  .srt_01       = 0,                    \
                  .rtt_wr_01    = 0,                    \
                  .dic_01       = 0,                    \
                  .rtt_nom_01   = rttnom_40ohm,         \
                  .pasr_10      = 0,                    \
                  .asr_10       = 0,                    \
                  .srt_10       = 0,                    \
                  .rtt_wr_10    = 0,                    \
                  .dic_10       = 0,                    \
                  .rtt_nom_10   = rttnom_120ohm,        \
                  .pasr_11      = 0,                    \
                  .asr_11       = 0,                    \
                  .srt_11       = 0,                    \
                  .rtt_wr_11    = 0,                    \
                  .dic_11       = 0,                    \
                  .rtt_nom_11   = rttnom_40ohm,         \
        }                                               \
    }


#define SRXSME_DRAM_SOCKET_CONFIGURATION0 \
    {{0x50, 0x0}, {NULL, NULL}},{{0x51, 0x0}, {NULL, NULL}}

#define JSRXNLE_DRAM_SOCKET_CONFIGURATION0 \
        {{0x50, 0x0}, {NULL, NULL}}

#define JSRXNLE_NO_SPD_DRAM_SOCKET_CONFIGURATION0 \
        {{0x0, 0x0}, {NULL, NULL}}

/* Board delay in picoseconds */
#define SRX550_DDR_BOARD_DELAY          3815   /* First guess was 3815 */

#define SRX550_DDR_CONFIGURATION                                        \
    /* Interface 0 */                                                   \
    {                                                                   \
        .dimm_config_table = {SRXSME_DRAM_SOCKET_CONFIGURATION0, DIMM_CONFIG_TERMINATOR}, \
            .unbuffered = {                                             \
             .ddr_board_delay = 0,                                      \
             .lmc_delay_clk = 0,                                        \
             .lmc_delay_cmd = 0,                                        \
             .lmc_delay_dq = 0,                                         \
         },                                                             \
                 .registered = {                                        \
             .ddr_board_delay = 0,                                      \
             .lmc_delay_clk = 0,                                        \
             .lmc_delay_cmd = 0,                                        \
             .lmc_delay_dq = 0,                                         \
         },                                                             \
                      .odt_1rank_config = {                             \
             CN63XX_DRAM_ODT_1RANK_CONFIGURATION,                       \
         },                                                             \
                           .odt_2rank_config = {                        \
             CN63XX_DRAM_ODT_2RANK_CONFIGURATION,                       \
         },                                                             \
   }

#define SRX650_DDR_CONFIGURATION                                        \
    /* Interface 0 */                                                   \
    {                                                                   \
        .dimm_config_table = {SRXSME_DRAM_SOCKET_CONFIGURATION0, DIMM_CONFIG_TERMINATOR}, \
            .unbuffered = {                                             \
             .ddr_board_delay = SRX650_UNB_DDR_BOARD_DELAY,             \
             .lmc_delay_clk = SRX650_UNB_LMC_DELAY_CLK,                 \
             .lmc_delay_cmd = SRX650_UNB_LMC_DELAY_CMD,                 \
             .lmc_delay_dq = SRX650_UNB_LMC_DELAY_DQ,                   \
         },                                                             \
                 .registered = {                                        \
             .ddr_board_delay = SRX650_DDR_BOARD_DELAY,                 \
             .lmc_delay_clk = SRX650_LMC_DELAY_CLK,                     \
             .lmc_delay_cmd = SRX650_LMC_DELAY_CMD,                     \
             .lmc_delay_dq = SRX650_LMC_DELAY_DQ,                       \
         },                                                             \
                      .odt_1rank_config = {                             \
             CN56XX_DRAM_ODT_1RANK_CONFIGURATION,                       \
         },                                                             \
                           .odt_2rank_config = {                        \
             CN56XX_DRAM_ODT_2RANK_CONFIGURATION,                       \
         },                                                             \
   }

#define SRX220_DDR_CONFIGURATION                                        \
    /* Interface 0 */                                                   \
    {                                                                   \
        .dimm_config_table = {JSRXNLE_DRAM_SOCKET_CONFIGURATION0, DIMM_CONFIG_TERMINATOR}, \
            .unbuffered = {                                             \
             .ddr_board_delay = SRX220_UNB_DDR_BOARD_DELAY,             \
             .lmc_delay_clk = SRX220_UNB_LMC_DELAY_CLK,                 \
             .lmc_delay_cmd = SRX220_UNB_LMC_DELAY_CMD,                 \
             .lmc_delay_dq = SRX220_UNB_LMC_DELAY_DQ,                   \
         },                                                             \
                 .registered = {                                        \
             .ddr_board_delay = SRX220_DDR_BOARD_DELAY,                 \
             .lmc_delay_clk = SRX220_LMC_DELAY_CLK,                     \
             .lmc_delay_cmd = SRX220_LMC_DELAY_CMD,                     \
             .lmc_delay_dq = SRX220_LMC_DELAY_DQ,                       \
         },                                                             \
                      .odt_1rank_config = {                             \
             CN50XX_DRAM_ODT_1RANK_CONFIGURATION,                       \
         },                                                             \
                           .odt_2rank_config = {                        \
             CN50XX_DRAM_ODT_2RANK_CONFIGURATION,                       \
         },                                                             \
   }

#define SRX240_DDR_CONFIGURATION                                        \
    /* Interface 0 */                                                   \
    {                                                                   \
        .dimm_config_table = {JSRXNLE_DRAM_SOCKET_CONFIGURATION0, DIMM_CONFIG_TERMINATOR}, \
            .unbuffered = {                                             \
             .ddr_board_delay = SRX240_UNB_DDR_BOARD_DELAY,             \
             .lmc_delay_clk = SRX240_UNB_LMC_DELAY_CLK,                 \
             .lmc_delay_cmd = SRX240_UNB_LMC_DELAY_CMD,                 \
             .lmc_delay_dq = SRX240_UNB_LMC_DELAY_DQ,                   \
         },                                                             \
                 .registered = {                                        \
             .ddr_board_delay = SRX240_DDR_BOARD_DELAY,                 \
             .lmc_delay_clk = SRX240_LMC_DELAY_CLK,                     \
             .lmc_delay_cmd = SRX240_LMC_DELAY_CMD,                     \
             .lmc_delay_dq = SRX240_LMC_DELAY_DQ,                       \
         },                                                             \
                      .odt_1rank_config = {                             \
             CN52XX_DRAM_ODT_1RANK_CONFIGURATION,                       \
         },                                                             \
                           .odt_2rank_config = {                        \
             CN52XX_DRAM_ODT_2RANK_CONFIGURATION,                       \
         },                                                             \
   }

#define SRX210_DDR_CONFIGURATION                                        \
    /* Interface 0 */                                                   \
    {                                                                   \
        .dimm_config_table = {JSRXNLE_NO_SPD_DRAM_SOCKET_CONFIGURATION0, DIMM_CONFIG_TERMINATOR}, \
            .unbuffered = {                                             \
             .ddr_board_delay = 0,                                      \
             .lmc_delay_clk = 0,                                        \
             .lmc_delay_cmd = 0,                                        \
             .lmc_delay_dq = 0,                                         \
         },                                                             \
                 .registered = {                                        \
             .ddr_board_delay = 0,                                      \
             .lmc_delay_clk = 0,                                        \
             .lmc_delay_cmd = 0,                                        \
             .lmc_delay_dq = 0,                                         \
         },                                                             \
                      .odt_1rank_config = {                             \
             CN50XX_DRAM_ODT_1RANK_CONFIGURATION,                       \
         },                                                             \
                           .odt_2rank_config = {                        \
             CN50XX_DRAM_ODT_2RANK_CONFIGURATION,                       \
         },                                                             \
    }

#define SRX110_DDR_CONFIGURATION                                        \
    /* Interface 0 */                                                   \
    {                                                                   \
        .dimm_config_table = {JSRXNLE_DRAM_SOCKET_CONFIGURATION0, DIMM_CONFIG_TERMINATOR}, \
            .unbuffered = {                                             \
             .ddr_board_delay = SRX110_UNB_DDR_BOARD_DELAY,             \
             .lmc_delay_clk = SRX110_UNB_LMC_DELAY_CLK,                 \
             .lmc_delay_cmd = SRX110_UNB_LMC_DELAY_CMD,                 \
             .lmc_delay_dq = SRX110_UNB_LMC_DELAY_DQ,                   \
         },                                                             \
                 .registered = {                                        \
             .ddr_board_delay = SRX110_DDR_BOARD_DELAY,                 \
             .lmc_delay_clk = SRX110_LMC_DELAY_CLK,                     \
             .lmc_delay_cmd = SRX110_LMC_DELAY_CMD,                     \
             .lmc_delay_dq = SRX110_LMC_DELAY_DQ,                       \
         },                                                             \
                      .odt_1rank_config = {                             \
             CN50XX_DRAM_ODT_1RANK_CONFIGURATION,                       \
         },                                                             \
                           .odt_2rank_config = {                        \
             CN50XX_DRAM_ODT_2RANK_CONFIGURATION,                       \
         },                                                             \
    }
#if defined(CONFIG_ACX500_SVCS)
#define ACX500_SVCS_SPD_VALUES                                               \
0x92,0x11,0x0b,0x06,0x03,0x19,0x00,0x02,0x0b,0x11,0x01,0x08,0x0f,0x00,0x1e,0x00,\
0xa0,0x78,0x69,0x5a,0x69,0x11,0x2c,0x95,0x60,0x09,0x3c,0x3c,0x01,0x95,0x83,0x05,\
0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x11,0x1f,0x00,\
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
0x00,0x00,0x00,0x00,0x00,0x80,0x2c,0x08,0x13,0x09,0xdc,0x95,0xd6,0x2f,0xbb,0x60

#define ACX500_SVCS_DRAM_SOCKET_CONFIGURATION0                   \
	{{0x0, 0x0}, {acx500_svcs_spd_values, NULL}}

/* Using drive strength of 34ohm based on SI input */
#define ACX500_SVCS_MODEREG_PARAMS1_1RANK_1SLOT	\
{							\
	.cn63xx = {					\
		.pasr_00	= 0,			\
		.asr_00		= 0,			\
		.srt_00		= 0,			\
		.rtt_wr_00	= 0,			\
        .dic_00		= 1,			\
		.rtt_nom_00	= rttnom_60ohm,		\
		.pasr_01	= 0,			\
		.asr_01		= 0,			\
		.srt_01		= 0,			\
		.rtt_wr_01	= 0,			\
		.dic_01		= 0,			\
		.rtt_nom_01	= 0,			\
		.pasr_10	= 0,			\
		.asr_10		= 0,			\
		.srt_10		= 0,			\
		.rtt_wr_10	= 0,			\
		.dic_10		= 0,			\
		.rtt_nom_10	= 0,			\
		.pasr_11	= 0,			\
		.asr_11		= 0,			\
		.srt_11		= 0,			\
		.rtt_wr_11	= 0,			\
		.dic_11		= 0,			\
		.rtt_nom_11	= 0			\
	}						\
}

#define ACX500_SVCS_MODEREG_PARAMS1_2RANK_1SLOT	\
{							\
	.cn63xx = {					\
		.pasr_00	= 0,			\
		.asr_00		= 0,			\
		.srt_00		= 0,			\
		.rtt_wr_00	= 0,			\
		.dic_00		= 0,			\
		.rtt_nom_00	= rttnom_40ohm,		\
		.pasr_01	= 0,			\
		.asr_01		= 0,			\
		.srt_01		= 0,			\
		.rtt_wr_01	= 0,			\
		.dic_01		= 0,			\
		.rtt_nom_01	= 0,			\
		.pasr_10	= 0,			\
		.asr_10		= 0,			\
		.srt_10		= 0,			\
		.rtt_wr_10	= 0,			\
		.dic_10		= 0,			\
		.rtt_nom_10	= 0,			\
		.pasr_11	= 0,			\
		.asr_11		= 0,			\
		.srt_11		= 0,			\
		.rtt_wr_11	= 0,			\
		.dic_11		= 0,			\
		.rtt_nom_11	= 0			\
	}						\
}

#define ACX500_SVCS_DRAM_ODT_1RANK_CONFIGURATION \
	/* DIMMS   reserved  WODT_MASK                LMCX_MODEREG_PARAMS1            RODT_CTL    RODT_MASK    reserved */	\
	/* =====   ======== ============== ========================================== ========= ============== ======== */	\
	/*   1 */ {   0,    0x00000001ULL, ACX500_SVCS_MODEREG_PARAMS1_1RANK_1SLOT,    3,     0x00000000ULL,  0  }	\

#define ACX500_SVCS_DRAM_ODT_2RANK_CONFIGURATION \
	/* DIMMS   reserved  WODT_MASK                LMCX_MODEREG_PARAMS1            RODT_CTL    RODT_MASK    reserved */	\
	/* =====   ======== ============== ========================================== ========= ============== ======== */	\
	/*   1 */ {   0,    0x00000101ULL, ACX500_SVCS_MODEREG_PARAMS1_2RANK_1SLOT,    3,     0x00000000ULL,    0  }    \


#define ACX500_SVCS_DDR_CONFIGURATION {                                      \
	/* Interface 0 */                                                   \
	.custom_lmc_config = {                                              \
		.min_rtt_nom_idx	= 1,                                        \
		.max_rtt_nom_idx	= 5,                                        \
		.min_rodt_ctl		= 1,                                        \
		.max_rodt_ctl		= 5,                                        \
		.dqx_ctl		= 1,                                            \
		.ck_ctl			= 1,                                            \
		.cmd_ctl		= 1,                                            \
		.ctl_ctl		= 1,                                            \
		.min_cas_latency	= 0,                                        \
		.offset_en 		= 0,                                            \
		.offset_udimm		= 0,                                        \
		.offset_rdimm		= 0,                                        \
		.ddr_rtt_nom_auto	= 0,                                        \
		.ddr_rodt_ctl_auto	= 0,                                        \
		.rlevel_comp_offset	= 2,                                        \
		.rlevel_compute 	= 0,                                        \
		.ddr2t_udimm 		= 1,                                        \
		.ddr2t_rdimm 		= 1,                                        \
		.fprch2			= 0,                                    \
		.parity			= 0,                                            \
		.mode32b		= 1,                                            \
	},                                                                  \
        .dimm_config_table = {                                          \
		ACX500_SVCS_DRAM_SOCKET_CONFIGURATION0,                              \
		DIMM_CONFIG_TERMINATOR,                                         \
	},                                                                  \
	.unbuffered = {                                                     \
		.ddr_board_delay = 0,                                           \
		.lmc_delay_clk = 0,                                             \
		.lmc_delay_cmd = 0,                                             \
		.lmc_delay_dq = 0,                                              \
	},                                                                  \
	.registered = {                                                     \
		.ddr_board_delay = 0,                                           \
		.lmc_delay_clk = 0,                                             \
		.lmc_delay_cmd = 0,                                             \
		.lmc_delay_dq = 0,                                              \
	},                                                                  \
	.odt_1rank_config = {                                               \
		ACX500_SVCS_DRAM_ODT_1RANK_CONFIGURATION,                            \
	},                                                                  \
	.odt_2rank_config = {                                               \
		ACX500_SVCS_DRAM_ODT_2RANK_CONFIGURATION,                            \
	},                                                                  \
}
#endif /* CONFIG_ACX500_SVCS */

uint64_t cvmx_clock_get_rate(cvmx_clock_t clock);

#endif  /*  __LIB_OCTEON_SHARED_H__  */
