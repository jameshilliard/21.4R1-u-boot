/***********************litcense start************************************
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

#ifdef __U_BOOT__
#include <common.h>
#include <octeon_boot.h>
#else
/* Building on host for PCI boot
** Always use the pass 2 definitions for this module.
** Assume that pass 2 is a superset of pass 1 and
** determine actual chip revision at run time. */
#undef  OCTEON_MODEL
#define OCTEON_MODEL OCTEON_CN38XX_PASS2

#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <cvmx.h>


#define simple_strtoul strtoul
#endif

#include "lib_octeon_shared.h"
#include "octeon_eeprom_types.h"

#ifdef CONFIG_MAG8600
#include "configs/mag8600.h"
#endif
#ifdef CONFIG_JSRXNLE
#include <jsrxnle_ddr_params.h>
#endif
int32_t is_proto_id_supported(void);
#define error_print printf

#define CONFIG_SOFTWARE_LEVELING 0
#if CONFIG_SOFTWARE_LEVELING
#define SAVE_RESTORE_DDR_CONFIG  1
#define CONFIG_SOFTWARE_LEVELING 1
#else
#define SAVE_RESTORE_DDR_CONFIG  0
#define CONFIG_SOFTWARE_LEVELING 0
#endif

#define RLEVEL_BITMASK_INVALID_BITS_ERROR       1
#define RLEVEL_BITMASK_BUBBLE_BITS_ERROR        4
#define RLEVEL_BITMASK_NARROW_ERROR             8
#define RLEVEL_BITMASK_BLANK_ERROR              9
#define RLEVEL_NONSEQUENTIAL_DELAY_ERROR       10
#define RLEVEL_ADJACENT_DELAY_ERROR             5
#define RLEVEL_UNDEFINED_ERROR            9999999

#define READ_LEVEL_ENABLE                       1

static int init_octeon_ddr2_interface(uint32_t cpu_id,
                               const ddr_configuration_t *ddr_configuration,
                               uint32_t ddr_hertz,
                               uint32_t cpu_hertz,
                               uint32_t ddr_ref_hertz,
                               int board_type,
                               int board_rev_maj,
                               int board_rev_min,
                               int ddr_interface_num,
                               uint32_t ddr_interface_mask
                               );
static int init_octeon_ddr3_interface(uint32_t cpu_id,
                               const ddr_configuration_t *ddr_configuration,
                               uint32_t ddr_hertz,
                               uint32_t cpu_hertz,
                               uint32_t ddr_ref_hertz,
                               int board_type,
                               int board_rev_maj,
                               int board_rev_min,
                               int ddr_interface_num,
                               uint32_t ddr_interface_mask
                               );

#if defined(CONFIG_OCTEON3)
static int init_octeon3_ddr3_interface(uint32_t cpu_id,
                               const ddr_configuration_t *ddr_configuration,
                               uint32_t ddr_hertz,
                               uint32_t cpu_hertz,
                               uint32_t ddr_ref_hertz,
                               int board_type,
                               int board_rev_maj,
                               int board_rev_min,
                               int ddr_interface_num,
                               uint32_t ddr_interface_mask
                               );
#endif

static char *getenv_wrapper(char *name)
{    
    DECLARE_GLOBAL_DATA_PTR;
    int i = 0;

    /* This buffer must be in scratch, as DRAM is not initialized yet. */
    char *input = (char *)(gd->tmp_str);
    memset(input, 0, GD_TMP_STR_SIZE);

    if (getenv("ddr_prompt"))
    {
        printf("Please enter value for %s DDR param(enter for default):", name);
        while (i < GD_TMP_STR_SIZE)
        {
            input[i] = getc();

            switch (input[i])
            {
                /* Enter ends the input */
                case '\r':
                case '\n':
                    input[i] = '\0';
                    goto gw_done;
                    break;

                    /* Handle backspace for interactive use */
                case 0x08:
                case 0x7F:
                    putc(0x08);
                    putc(' ');
                    putc(0x08);
                    if (i > 0)
                        i--;
                    continue;
                    break;
            }
            putc(input[i]);
            i++;
        }
gw_done:
        if (i == 0)
        {
            printf("\n");
            printf("No value entered, using default value for parameter %s\n", name);
            return(NULL);
        }
        else
        {
            printf("\n");
            printf("Using value %s for parameter %s\n", input, name);
            return(input);
        }



    }
    else
        return getenv(name);
}


static inline char *ddr_getenv_debug(char *name)
{
    DECLARE_GLOBAL_DATA_PTR;

    /* Require ddr_verbose to enable ddr_getenv_debug */
    if (gd->flags & GD_FLG_DDR_VERBOSE)
        return getenv(name);

    return ((char *) 0);
}

static char * lookup_env_parameter(char *label)
{
    char *s;
    unsigned long value;
    if ((s = ddr_getenv_debug(label)) != NULL) {
        value = simple_strtoul(s, NULL, 0);
        error_print("Parameter found in environment. %s = 0x%lx (%ld)\n", label, value, value);
    }
    return s;
}

static char * lookup_env_parameter_ull(char *label)
{
    char *s;
    unsigned long long value;
    if ((s = ddr_getenv_debug(label)) != NULL) {
        value = simple_strtoull(s, NULL, 0);
        error_print("Parameter found in environment. %s = 0x%016llx\n", label, value);
    }
    return s;
}

int cvmx_l2c_get_set_bits(void)
{
    int l2_set_bits;
    if (OCTEON_IS_MODEL(OCTEON_CN56XX) ||
        OCTEON_IS_MODEL(OCTEON_CN58XX))
        l2_set_bits =  11; /* 2048 sets */
    else if (OCTEON_IS_MODEL(OCTEON_CN38XX) || OCTEON_IS_MODEL(OCTEON_CN63XX))
        l2_set_bits =  10; /* 1024 sets */
    else if (OCTEON_IS_MODEL(OCTEON_CN31XX) || OCTEON_IS_MODEL(OCTEON_CN52XX))
        l2_set_bits =  9; /* 512 sets */
    else if (OCTEON_IS_MODEL(OCTEON_CN30XX))
        l2_set_bits =  8; /* 256 sets */
    else if (OCTEON_IS_MODEL(OCTEON_CN50XX))
        l2_set_bits =  7; /* 128 sets */
    else
    {
        cvmx_dprintf("Unsupported OCTEON Model in %s\n", __FUNCTION__);
        l2_set_bits =  11; /* 2048 sets */
    }
    return(l2_set_bits);

}

/* Return the number of sets in the L2 Cache */
int cvmx_l2c_get_num_sets(void)
{
    return (1 << cvmx_l2c_get_set_bits());
}

/* turn the variable name into a string */
#define CVMX_TMP_STR(x) CVMX_TMP_STR2(x)
#define CVMX_TMP_STR2(x) #x

#define CVMX_L2C_IDX_ADDR_SHIFT 7  /* based on 128 byte cache line size */
#define CVMX_L2C_IDX_MASK       (cvmx_l2c_get_num_sets() - 1)

/* Defines for index aliasing computations */
#define CVMX_L2C_TAG_ADDR_ALIAS_SHIFT (CVMX_L2C_IDX_ADDR_SHIFT + cvmx_l2c_get_set_bits())
#define CVMX_L2C_ALIAS_MASK (CVMX_L2C_IDX_MASK << CVMX_L2C_TAG_ADDR_ALIAS_SHIFT)
#define CVMX_L2C_MEMBANK_SELECT_SIZE  4096

#define CVMX_CACHE(op, address, offset) asm volatile ( "cache " CVMX_TMP_STR(op) ", " CVMX_TMP_STR(offset) "(%[rbase])" : : [rbase] "d" (address) )
#define CVMX_CACHE_WBIL2I(address, offset) CVMX_CACHE(3, address, offset)

static int64_t _sign(int64_t v)
{
    return (v < 0);
}

#define cvmx_wait_usec(x) octeon_delay_cycles((x)*800)

#define ddr_getenv_debug(...) ((char*)0)

/* Divide and round results to the nearest integer. */
uint64_t divide_nint(uint64_t dividend, uint64_t divisor)
{
    uint64_t quotent, remainder;
    quotent   = dividend / divisor;
    remainder = dividend % divisor;
    return (quotent + ((remainder*2) >= divisor));
}

static uint64_t divide_roundup(uint64_t dividend, uint64_t divisor)
{
    return ((dividend+divisor-1) / divisor);
}

static int64_t _abs(int64_t v)
{
    return ((v < 0) ? -v : v);
}

#ifdef CONFIG_JSRXNLE

const lmc_reg_val_t lmc_reg_val[] = {
    /* 8x1Gb Qimonda */
    {
        LMC_MEM_CFG0_8x1Gb_QIMONDA, LMC_MEM_CFG1_8x1Gb_QIMONDA,
        LMC_CTL_8x1Gb_QIMONDA, LMC_CTL1_8x1Gb_QIMONDA,
        LMC_DDR2_CTL_8x1Gb_QIMONDA, LMC_COMP_CTL_8x1Gb_QIMONDA,
        LMC_WODT_CTL_8x1Gb_QIMONDA, LMC_RODT_CTL_8x1Gb_QIMONDA,
        LMC_DELAY_CFG_8x1Gb_QIMONDA, LMC_RODT_COMP_CTL_8x1Gb_QIMONDA,
        LMC_PLL_CTL_8x1Gb_QIMONDA
    },
    /* 4x1Gb Qimonda */
    {
        LMC_MEM_CFG0_4x1Gb_QIMONDA, LMC_MEM_CFG1_4x1Gb_QIMONDA,
        LMC_CTL_4x1Gb_QIMONDA, LMC_CTL1_4x1Gb_QIMONDA,
        LMC_DDR2_CTL_4x1Gb_QIMONDA, LMC_COMP_CTL_4x1Gb_QIMONDA,
        LMC_WODT_CTL_4x1Gb_QIMONDA, LMC_RODT_CTL_4x1Gb_QIMONDA,
        LMC_DELAY_CFG_4x1Gb_QIMONDA, LMC_RODT_COMP_CTL_4x1Gb_QIMONDA,
        LMC_PLL_CTL_4x1Gb_QIMONDA
    },
    /* 8x512Mb Elpida */
    {
        LMC_MEM_CFG0_8x512Mb_ELPIDA, LMC_MEM_CFG1_8x512Mb_ELPIDA,
        LMC_CTL_8x512Mb_ELPIDA, LMC_DDR2_CTL_8x512Mb_ELPIDA,
        LMC_DDR2_CTL_8x512Mb_ELPIDA, LMC_COMP_CTL_8x512Mb_ELPIDA,
        LMC_WODT_CTL_8x512Mb_ELPIDA, LMC_RODT_CTL_8x512Mb_ELPIDA,
        LMC_DELAY_CFG_8x512Mb_ELPIDA, LMC_RODT_COMP_CTL_8x512Mb_ELPIDA,
        LMC_PLL_CTL_8x512Mb_EPLIDA
    },
    /* 8x1Gb Qimonda for srx210 proto4 and above*/
    {
        SRX210_P4_LMC_MEM_CFG0_8x1Gb_QIMONDA,
        SRX210_P4_LMC_MEM_CFG1_8x1Gb_QIMONDA,
        SRX210_P4_LMC_CTL_8x1Gb_QIMONDA,
        SRX210_P4_LMC_CTL1_8x1Gb_QIMONDA,
        SRX210_P4_LMC_DDR2_CTL_8x1Gb_QIMONDA,
        SRX210_P4_LMC_COMP_CTL_8x1Gb_QIMONDA,
        SRX210_P4_LMC_WODT_CTL_8x1Gb_QIMONDA,
        SRX210_P4_LMC_RODT_CTL_8x1Gb_QIMONDA,
        SRX210_P4_LMC_DELAY_CFG_8x1Gb_QIMONDA,
        SRX210_P4_LMC_RODT_COMP_CTL_8x1Gb_QIMONDA,
        SRX210_P4_LMC_PLL_CTL_8x1Gb_QIMONDA
    },
    /* 4x1Gb Qimonda for srx210 proto4 and above */
    {
        SRX210_P4_LMC_MEM_CFG0_4x1Gb_QIMONDA,
        SRX210_P4_LMC_MEM_CFG1_4x1Gb_QIMONDA,
        SRX210_P4_LMC_CTL_4x1Gb_QIMONDA,
        SRX210_P4_LMC_CTL1_4x1Gb_QIMONDA,
        SRX210_P4_LMC_DDR2_CTL_4x1Gb_QIMONDA,
        SRX210_P4_LMC_COMP_CTL_4x1Gb_QIMONDA,
        SRX210_P4_LMC_WODT_CTL_4x1Gb_QIMONDA,
        SRX210_P4_LMC_RODT_CTL_4x1Gb_QIMONDA,
        SRX210_P4_LMC_DELAY_CFG_4x1Gb_QIMONDA,
        SRX210_P4_LMC_RODT_COMP_CTL_4x1Gb_QIMONDA,
        SRX210_P4_LMC_PLL_CTL_4x1Gb_QIMONDA
    },
    /* 8x1Gb Qimonda for SRX100 */
    {
        SRX100_LMC_MEM_CFG0_8x1Gb_QIMONDA,
        SRX100_LMC_MEM_CFG1_8x1Gb_QIMONDA,
        SRX100_LMC_CTL_8x1Gb_QIMONDA,
        SRX100_LMC_CTL1_8x1Gb_QIMONDA,
        SRX100_LMC_DDR2_CTL_8x1Gb_QIMONDA,
        SRX100_LMC_COMP_CTL_8x1Gb_QIMONDA,
        SRX100_LMC_WODT_CTL_8x1Gb_QIMONDA,
        SRX100_LMC_RODT_CTL_8x1Gb_QIMONDA,
        SRX100_LMC_DELAY_CFG_8x1Gb_QIMONDA,
        SRX100_LMC_RODT_COMP_CTL_8x1Gb_QIMONDA,
        SRX100_LMC_PLL_CTL_8x1Gb_QIMONDA
    },
    /* 4x1Gb Qimonda for SRX100 */
    {
        SRX100_LMC_MEM_CFG0_4x1Gb_QIMONDA,
        SRX100_LMC_MEM_CFG1_4x1Gb_QIMONDA,
        SRX100_LMC_CTL_4x1Gb_QIMONDA,
        SRX100_LMC_CTL1_4x1Gb_QIMONDA,
        SRX100_LMC_DDR2_CTL_4x1Gb_QIMONDA,
        SRX100_LMC_COMP_CTL_4x1Gb_QIMONDA,
        SRX100_LMC_WODT_CTL_4x1Gb_QIMONDA,
        SRX100_LMC_RODT_CTL_4x1Gb_QIMONDA,
        SRX100_LMC_DELAY_CFG_4x1Gb_QIMONDA,
        SRX100_LMC_RODT_COMP_CTL_4x1Gb_QIMONDA,
        SRX100_LMC_PLL_CTL_4x1Gb_QIMONDA
    },
    /* 8x512Mb Elpida for SRX100 */
    {
        SRX100_LMC_MEM_CFG0_8x512Mb_ELPIDA,
        SRX100_LMC_MEM_CFG1_8x512Mb_ELPIDA,
        SRX100_LMC_CTL_8x512Mb_ELPIDA,
        SRX100_LMC_CTL1_8x512Mb_ELPIDA,
        SRX100_LMC_DDR2_CTL_8x512Mb_ELPIDA,
        SRX100_LMC_COMP_CTL_8x512Mb_ELPIDA,
        SRX100_LMC_WODT_CTL_8x512Mb_ELPIDA,
        SRX100_LMC_RODT_CTL_8x512Mb_ELPIDA,
        SRX100_LMC_DELAY_CFG_8x512Mb_ELPIDA,
        SRX100_LMC_RODT_COMP_CTL_8x512Mb_ELPIDA,
        SRX100_LMC_LMC_PLL_CTL_8x1512Mb_ELPIDA 
    },
    /* 8x1Gb Qimonda for srx210 533Mhz DDR */
    {
        SRX210_533_LMC_MEM_CFG0_8x1Gb_QIMONDA,
        SRX210_533_LMC_MEM_CFG1_8x1Gb_QIMONDA,
        SRX210_533_LMC_CTL_8x1Gb_QIMONDA,
        SRX210_533_LMC_CTL1_8x1Gb_QIMONDA,
        SRX210_533_LMC_DDR2_CTL_8x1Gb_QIMONDA,
        SRX210_533_LMC_COMP_CTL_8x1Gb_QIMONDA,
        SRX210_533_LMC_WODT_CTL_8x1Gb_QIMONDA,
        SRX210_533_LMC_RODT_CTL_8x1Gb_QIMONDA,
        SRX210_533_LMC_DELAY_CFG_8x1Gb_QIMONDA,
        SRX210_533_LMC_RODT_COMP_CTL_8x1Gb_QIMONDA,
        SRX210_533_LMC_PLL_CTL_8x1Gb_QIMONDA
    },
    /* 4x1Gb Qimonda for srx210 533Mhz DDR */
    {
        SRX210_533_LMC_MEM_CFG0_4x1Gb_QIMONDA,
        SRX210_533_LMC_MEM_CFG1_4x1Gb_QIMONDA,
        SRX210_533_LMC_CTL_4x1Gb_QIMONDA,
        SRX210_533_LMC_CTL1_4x1Gb_QIMONDA,
        SRX210_533_LMC_DDR2_CTL_4x1Gb_QIMONDA,
        SRX210_533_LMC_COMP_CTL_4x1Gb_QIMONDA,
        SRX210_533_LMC_WODT_CTL_4x1Gb_QIMONDA,
        SRX210_533_LMC_RODT_CTL_4x1Gb_QIMONDA,
        SRX210_533_LMC_DELAY_CFG_4x1Gb_QIMONDA,
        SRX210_533_LMC_RODT_COMP_CTL_4x1Gb_QIMONDA,
        SRX210_533_LMC_PLL_CTL_4x1Gb_QIMONDA
    },
    /* 8x1Gb Micron */
    {
        SRX100_LMC_MEM_CFG0_8x1Gb_MICRON,
        SRX100_LMC_MEM_CFG1_8x1Gb_MICRON,
        SRX100_LMC_CTL_8x1Gb_MICRON,
        SRX100_LMC_CTL1_8x1Gb_MICRON,
        SRX100_LMC_DDR2_CTL_8x1Gb_MICRON,
        SRX100_LMC_COMP_CTL_8x1Gb_MICRON,
        SRX100_LMC_WODT_CTL_8x1Gb_MICRON,
        SRX100_LMC_RODT_CTL_8x1Gb_MICRON,
        SRX100_LMC_DELAY_CFG_8x1Gb_MICRON,
        SRX100_LMC_RODT_COMP_CTL_8x1Gb_MICRON,
        SRX100_LMC_PLL_CTL_8x1Gb_MICRON 
    },
    /* 8x2Gb Qimonda for SRX100 */
    {
        SRX100_LMC_MEM_CFG0_8x2Gb_QIMONDA,
        SRX100_LMC_MEM_CFG1_8x2Gb_QIMONDA,
        SRX100_LMC_CTL_8x2Gb_QIMONDA,
        SRX100_LMC_CTL1_8x2Gb_QIMONDA,
        SRX100_LMC_DDR2_CTL_8x2Gb_QIMONDA,
        SRX100_LMC_COMP_CTL_8x2Gb_QIMONDA,
        SRX100_LMC_WODT_CTL_8x2Gb_QIMONDA,
        SRX100_LMC_RODT_CTL_8x2Gb_QIMONDA,
        SRX100_LMC_DELAY_CFG_8x2Gb_QIMONDA,
        SRX100_LMC_RODT_COMP_CTL_8x2Gb_QIMONDA,
        SRX100_LMC_PLL_CTL_8x2Gb_QIMONDA
    },
     /* 8x2Gb Qimonda for srx210 533Mhz DDR */
    {
        SRX210_533_LMC_MEM_CFG0_8x2Gb_QIMONDA,
        SRX210_533_LMC_MEM_CFG1_8x2Gb_QIMONDA,
        SRX210_533_LMC_CTL_8x2Gb_QIMONDA,
        SRX210_533_LMC_CTL1_8x2Gb_QIMONDA,
        SRX210_533_LMC_DDR2_CTL_8x2Gb_QIMONDA,
        SRX210_533_LMC_COMP_CTL_8x2Gb_QIMONDA,
        SRX210_533_LMC_WODT_CTL_8x2Gb_QIMONDA,
        SRX210_533_LMC_RODT_CTL_8x2Gb_QIMONDA,
        SRX210_533_LMC_DELAY_CFG_8x2Gb_QIMONDA,
        SRX210_533_LMC_RODT_COMP_CTL_8x2Gb_QIMONDA,
        SRX210_533_LMC_PLL_CTL_8x2Gb_QIMONDA
    },
    /* 4x2Gb Qimonda for srx210 533Mhz DDR */
    {
        SRX210_533_LMC_MEM_CFG0_4x2Gb_QIMONDA,
        SRX210_533_LMC_MEM_CFG1_4x2Gb_QIMONDA,
        SRX210_533_LMC_CTL_4x2Gb_QIMONDA,
        SRX210_533_LMC_CTL1_4x2Gb_QIMONDA,
        SRX210_533_LMC_DDR2_CTL_4x2Gb_QIMONDA,
        SRX210_533_LMC_COMP_CTL_4x2Gb_QIMONDA,
        SRX210_533_LMC_WODT_CTL_4x2Gb_QIMONDA,
        SRX210_533_LMC_RODT_CTL_4x2Gb_QIMONDA,
        SRX210_533_LMC_DELAY_CFG_4x2Gb_QIMONDA,
        SRX210_533_LMC_RODT_COMP_CTL_4x2Gb_QIMONDA,
        SRX210_533_LMC_PLL_CTL_4x2Gb_QIMONDA
    }
};

const ddr_config_data_t ddr_config_data[] = {
    /* high mem, 8x1Gbits */
    { 14, 10, 8, 2, 8},
    /* low mem, 4x1Gbits */
    { 14, 10, 8, 1, 8},
    /* low mem, 8x512Mbits */
    { 14, 10, 4, 2},
    /* high mem, 8x2Gbits */
    { 15, 10, 8, 2, 8},
    /* low mem, 4x2Gbits */
    { 15, 10, 8, 1, 8}
};
#endif
/* Make a define similar to OCTEON_IS_MODEL that will work in this context.  This is runtime only.
** We can't do the normal runtime here since this code can run on the host.
** All functions using this must have a cpu_id variable set to the correct value */
#define octeon_is_cpuid(x)   (__OCTEON_IS_MODEL_COMPILE__(x, cpu_id))

#if (!CONFIG_OCTEON_SIM)

#ifdef DEBUG
#define DEBUG_DEFINED (1)
const char *ddr2_spd_strings[];
#define debug_print 
#else
#define DEBUG_DEFINED (0)
#define debug_print(...)
#endif

#ifdef __U_BOOT__
static inline int ddr_verbose(void)
{
    DECLARE_GLOBAL_DATA_PTR;
    return (!!(gd->flags & GD_FLG_DDR_VERBOSE));
}
#else
static int ddr_verbose(void)
{
    return(getenv("ddr_verbose") != NULL);
}
#endif

static void ddr_print(const char *format, ...)
{
    if (ddr_verbose() || (DEBUG_DEFINED))
    {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}

extern uint64_t cvmx_read_csr(uint64_t address);
extern void cvmx_write_csr(uint64_t address, uint64_t val);
extern void octeon_delay_cycles(uint64_t cycles);

/* Return the revision field from the NPI_CTL_STATUS register.  This
** is used to distinguish pass1/pass2.
** Compile time checks should only be used as a last resort.
** pass1 == 0
** pass2 == 1
*/
static inline uint32_t octeon_get_chip_rev(void)
{
    cvmx_npi_ctl_status_t npi_ctl_status;
    npi_ctl_status.u64 = cvmx_read_csr(CVMX_NPI_CTL_STATUS);
    return(npi_ctl_status.s.chip_rev);
}

#undef min
#define min(X, Y)				\
	({ typeof (X) __x = (X), __y = (Y);	\
		(__x < __y) ? __x : __y; })

#undef max
#define max(X, Y)				\
	({ typeof (X) __x = (X), __y = (Y);	\
		(__x > __y) ? __x : __y; })

#undef round_divide
#define round_divide(X, Y)                              /* ***  Example  *** */ \
        ({   typeof (X) __x = (X), __y = (Y), __z;      /* 8/5 = 1.6 ==> 2   */ \
             __z = __x * 10 / __y;                      /* 8 * 10 / 5 = 16   */ \
             __x = __z / 10;                            /* 16 / 10 = 1       */ \
             __z = __z - (__x * 10);                    /* 16 - (1 * 10) = 6 */ \
             (__x + (typeof (__x))(__z >= 5));})        /* 1 + 1 = 2         */


/*
**
** +---------+----+----------------------+---------------+--------+------+-----+
** |  Dimm   |Rank|         Row          |      Col      |  Bank  |  Col | Bus |
** +---------+----+----------------------+---------------+--------+------+-----+
**          |   |                       |                    |
**          |   |                       |                    |
**          |  1 bit           LMC_MEM_CFG0[ROW_LSB]         |
**          |                                                |
**   LMC_MEM_CFG0[PBANK_LSB]                       (2 + LMC_DDR2_CTL[BANK8]) bits
**
**    Bus     = Selects the byte on the 72/144-bit DDR2 bus
**         CN38XX: Bus = (3 + LMC_CTL[MODE128b]) bits
**         CN31XX: Bus = (3 - LMC_CTL[MODE32b]) bits
**         CN30XX: Bus = (1 + LMC_CTL[MODE32b]) bits
**         CN58XX: Bus = 
**         CN56XX: Bus = 
**         CN50XX: Bus = 
**    Col     = Column Address for the DDR2 part
**    Bank    = Bank Address for the DDR2 part
**    Row     = Row Address for the DDR2 part
**    Rank    = Optional Rank Address for dual-rank DIMM's
**              (present when LMC_CTL[BUNK_ENABLE] is set)
**    DIMM    = Optional DIMM address - 0, 1,or 2 bits
*/





#ifdef STATIC_DRAM_CONFIG_TABLE
extern const unsigned char STATIC_DRAM_CONFIG_TABLE [];
#endif

int oct_print64 (uint64_t csr_addr)
{
	uint64_t val = cvmx_read_csr(csr_addr);
	printf("0x%08x%08x: 0x%08x%08x\n", (uint32_t)((csr_addr>>32)&0xffffffff), (uint32_t)(csr_addr&0xffffffff),
                               (uint32_t)((val>>32)&0xffffffff), (uint32_t)(val&0xffffffff));
    return 0;
}

int validate_spd_checksum_ddr2(int twsi_addr, int silent)
{
    int i;
    int rv;
    uint16_t sum = 0;
    uint8_t csum;
    int ff_cnt = 0;
    for (i = 0;i < 63;i++)
    {
        rv = octeon_twsi_read(twsi_addr, i);
        if (rv < 0)
            return 0;   /* TWSI read error */
        sum  += (uint8_t)rv;

        /* Sometimes the tuple eeprom gets a correct SPD checksum, even though
        ** most of it reads back 0xff due to different IA size.  */
        if (rv == 0xff && ++ff_cnt > 20)
            return(0);
        /* Bail out early if it looks blank, as this is slow over ejtag.
        ** If first 5 values are all 0xff, it is not a valid ddr2 SPD. */
        if (sum == (5*0xff) && i == 4)
            return 0;
    }
    csum = octeon_twsi_read(twsi_addr, 63);
    if (sum == 0xff*63)
    {
        if (!silent)
            printf("ERROR: DIMM SPD at addr 0x%x is all 0xff..., looks like SPD is blank\n", twsi_addr);
        return 0;
    }
    if (csum != (sum & 0xff))
    {
        /* Special handling for dimms that have byte 0 value of 0 instead of 0x80 */
        if (octeon_twsi_read(twsi_addr, 0) == 0 && ((sum + 0x80)&0xFF) == csum)
        {
            ddr_print("WARNING: DIMM SPD at addr 0x%x has incorrect value at address 0\n", twsi_addr);
            return 1;  /* Checksum is bad, but we know why */
        }

        if (!silent)
            printf("ERROR: DIMM SPD at addr 0x%x: checksum mismatch(read: 0x%x, computed: 0x%x)\n", twsi_addr, csum, sum & 0xFF);
        return 0;
    }
    if (sum == 0)
    {
        if (!silent)
            printf("ERROR: DIMM SPD at addr 0x%x is all zeros....\n", twsi_addr);
        return 0;
    }
    return 1;

}

int cvmx_l2c_get_core_way_partition(uint32_t core)
{
    uint32_t    field;

    /* Validate the core number */
    if (core >= cvmx_octeon_num_cores())
        return -1;

    if (OCTEON_IS_MODEL(OCTEON_CN63XX))
        return (cvmx_read_csr(CVMX_L2C_WPAR_PPX(core)) & 0xffff);

    /* Use the lower two bits of the coreNumber to determine the bit offset
     * of the UMSK[] field in the L2C_SPAR register.
     */
    field = (core & 0x3) * 8;

    /* Return the UMSK[] field from the appropriate L2C_SPAR register based
     * on the coreNumber.
     */

    switch (core & 0xC)
    {
        case 0x0:
            return((cvmx_read_csr(CVMX_L2C_SPAR0) & (0xFF << field)) >> field);
        case 0x4:
            return((cvmx_read_csr(CVMX_L2C_SPAR1) & (0xFF << field)) >> field);
        case 0x8:
            return((cvmx_read_csr(CVMX_L2C_SPAR2) & (0xFF << field)) >> field);
        case 0xC:
            return((cvmx_read_csr(CVMX_L2C_SPAR3) & (0xFF << field)) >> field);
    }
    return(0);
}



int cvmx_l2c_set_core_way_partition(uint32_t core, uint32_t mask)
{
    uint32_t    field;
    uint32_t    valid_mask;

    valid_mask = (0x1 << cvmx_l2c_get_num_assoc()) - 1;

    mask &= valid_mask;

    /* A UMSK setting which blocks all L2C Ways is an error on some chips */
    if (mask == valid_mask &&
        !(OCTEON_IS_MODEL(OCTEON_CN63XX) || OCTEON_IS_MODEL(OCTEON_CN71XX)))
        return -1;

    /* Validate the core number */
    if (core >= cvmx_octeon_num_cores())
        return -1;

    if (OCTEON_IS_MODEL(OCTEON_CN63XX) || OCTEON_IS_MODEL(OCTEON_CN71XX))
    {
       cvmx_write_csr(CVMX_L2C_WPAR_PPX(core), mask);
       return 0;
    }

    /* Use the lower two bits of core to determine the bit offset of the
     * UMSK[] field in the L2C_SPAR register.
     */
    field = (core & 0x3) * 8;

    /* Assign the new mask setting to the UMSK[] field in the appropriate
     * L2C_SPAR register based on the core_num.
     *
     */
    switch (core & 0xC)
    {
        case 0x0:
            cvmx_write_csr(CVMX_L2C_SPAR0,
                           (cvmx_read_csr(CVMX_L2C_SPAR0) & ~(0xFF << field)) |
                           mask << field);
            break;
        case 0x4:
            cvmx_write_csr(CVMX_L2C_SPAR1,
                           (cvmx_read_csr(CVMX_L2C_SPAR1) & ~(0xFF << field)) |
                           mask << field);
            break;
        case 0x8:
            cvmx_write_csr(CVMX_L2C_SPAR2,
                           (cvmx_read_csr(CVMX_L2C_SPAR2) & ~(0xFF << field)) |
                           mask << field);
            break;
        case 0xC:
            cvmx_write_csr(CVMX_L2C_SPAR3,
                           (cvmx_read_csr(CVMX_L2C_SPAR3) & ~(0xFF << field)) |
                           mask << field);
            break;
    }
    return 0;
}


int cvmx_l2c_set_hw_way_partition(uint32_t mask)
{
    uint32_t valid_mask;

    valid_mask = (0x1 << cvmx_l2c_get_num_assoc()) - 1;
    mask &= valid_mask;

    /* A UMSK setting which blocks all L2C Ways is an error on some chips */
    if (mask == valid_mask  &&
        !(OCTEON_IS_MODEL(OCTEON_CN63XX) || OCTEON_IS_MODEL(OCTEON_CN71XX)))
        return -1;

    if (OCTEON_IS_MODEL(OCTEON_CN63XX) || OCTEON_IS_MODEL(OCTEON_CN71XX))
        cvmx_write_csr(CVMX_L2C_WPAR_IOBX(0), mask);
    else
        cvmx_write_csr(CVMX_L2C_SPAR4, (cvmx_read_csr(CVMX_L2C_SPAR4) & ~0xFF) | mask);
    return 0;
}

int cvmx_l2c_get_hw_way_partition(void)
{
    if (OCTEON_IS_MODEL(OCTEON_CN63XX) || OCTEON_IS_MODEL(OCTEON_CN71XX))
        return(cvmx_read_csr(CVMX_L2C_WPAR_IOBX(0)) & 0xffff);
    else
        return(cvmx_read_csr(CVMX_L2C_SPAR4) & (0xFF));
}

/* Return the number of associations in the L2 Cache */
int cvmx_l2c_get_num_assoc(void)
{
    int l2_assoc;
    if (OCTEON_IS_MODEL(OCTEON_CN56XX) ||
        OCTEON_IS_MODEL(OCTEON_CN52XX) ||
        OCTEON_IS_MODEL(OCTEON_CN58XX) ||
        OCTEON_IS_MODEL(OCTEON_CN50XX) ||
        OCTEON_IS_MODEL(OCTEON_CN38XX))
        l2_assoc =  8;
    else if (OCTEON_IS_MODEL(OCTEON_CN63XX))
        l2_assoc =  16;
    else if (OCTEON_IS_MODEL(OCTEON_CN31XX) ||
             OCTEON_IS_MODEL(OCTEON_CN30XX))
        l2_assoc =  4;
    else if (OCTEON_IS_MODEL(OCTEON_CN71XX))
    {
        union cvmx_mio_fus_dat3 mio_fus_dat3;

        mio_fus_dat3.u64 = cvmx_read_csr(CVMX_MIO_FUS_DAT3);

        switch (mio_fus_dat3.s.l2c_crip)
        {
        case 3:  /* 1/4 size */
            l2_assoc = 1;
            break;
        case 2:  /* 1/2 size */
            l2_assoc = 2;
            break; 
        case 1:  /* 3/4 size */
            l2_assoc = 3;
            break;
        default: /* Full size */
            l2_assoc = 4;
            break;
        }
        return l2_assoc;
    }
    else
    {
        cvmx_dprintf("Unsupported OCTEON Model in %s\n", __FUNCTION__);
        l2_assoc =  8;
    }

    /* Check to see if part of the cache is disabled */
    if (OCTEON_IS_MODEL(OCTEON_CN63XX))
    {
        cvmx_mio_fus_dat3_t mio_fus_dat3;

        mio_fus_dat3.u64 = cvmx_read_csr(CVMX_MIO_FUS_DAT3);
        /* cvmx_mio_fus_dat3.s.l2c_crip fuses map as follows
           <2> will be not used for 63xx
           <1> disables 1/2 ways
           <0> disables 1/4 ways
           They are cumulative, so for 63xx:
           <1> <0>
           0 0 16-way 2MB cache
           0 1 12-way 1.5MB cache
           1 0 8-way 1MB cache
           1 1 4-way 512KB cache */

        if (mio_fus_dat3.s.l2c_crip == 3)
            l2_assoc = 4;
        else if (mio_fus_dat3.s.l2c_crip == 2)
            l2_assoc = 8;
        else if (mio_fus_dat3.s.l2c_crip == 1)
            l2_assoc = 12;
    }
    else
    {
        cvmx_l2d_fus3_t val;
        val.u64 = cvmx_read_csr(CVMX_L2D_FUS3);
        /* Using shifts here, as bit position names are different for
           each model but they all mean the same. */
        if ((val.u64 >> 35) & 0x1)
            l2_assoc = l2_assoc >> 2;
        else if ((val.u64 >> 34) & 0x1)
            l2_assoc = l2_assoc >> 1;
    }
    return(l2_assoc);
}

void cvmx_l2c_flush_line(uint32_t assoc, uint32_t index)
{
    /* Check the range of the index. */
    if (index > (uint32_t)cvmx_l2c_get_num_sets())
    {
        cvmx_dprintf("ERROR: cvmx_l2c_flush_line index out of range.\n");
        return;
    }

    /* Check the range of association. */
    if (assoc > (uint32_t)cvmx_l2c_get_num_assoc())
    {
        cvmx_dprintf("ERROR: cvmx_l2c_flush_line association out of range.\n");
        return;
    }

    if (OCTEON_IS_MODEL(OCTEON_CN63XX))
    {
        uint64_t address;
        /* Create the address based on index and association.
           Bits<20:17> select the way of the cache block involved in
                       the operation
           Bits<16:7> of the effect address select the index */
        address = CVMX_ADD_SEG(CVMX_MIPS_SPACE_XKPHYS,
                               (assoc << CVMX_L2C_TAG_ADDR_ALIAS_SHIFT) |
                               (index << CVMX_L2C_IDX_ADDR_SHIFT));
        CVMX_CACHE_WBIL2I(address, 0);
    }
    else
    {
        cvmx_l2c_dbg_t l2cdbg;

        l2cdbg.u64 = 0;
        if (!OCTEON_IS_MODEL(OCTEON_CN30XX))
            l2cdbg.s.ppnum = cvmx_get_core_num();
        l2cdbg.s.finv = 1;

        l2cdbg.s.set = assoc;
        /* Enter debug mode, and make sure all other writes complete before we
        ** enter debug mode */
        CVMX_SYNC;
        cvmx_write_csr(CVMX_L2C_DBG, l2cdbg.u64);
        cvmx_read_csr(CVMX_L2C_DBG);

        CVMX_PREPARE_FOR_STORE (CVMX_ADD_SEG(CVMX_MIPS_SPACE_XKPHYS, index*CVMX_CACHE_LINE_SIZE), 0);
        /* Exit debug mode */
        CVMX_SYNC;
        cvmx_write_csr(CVMX_L2C_DBG, 0);
        cvmx_read_csr(CVMX_L2C_DBG);
    }
}


void cvmx_l2c_flush(void)
{
    uint64_t assoc, set;
    uint64_t n_assoc, n_set;

    n_set = cvmx_l2c_get_num_sets();
    n_assoc = cvmx_l2c_get_num_assoc();

    if (OCTEON_IS_MODEL(OCTEON_CN6XXX))
    {
        uint64_t address;
        /* These may look like constants, but they aren't... */
        int assoc_shift = CVMX_L2C_TAG_ADDR_ALIAS_SHIFT;
        int set_shift = CVMX_L2C_IDX_ADDR_SHIFT;
        for (set=0; set < n_set; set++)
        {
            for(assoc=0; assoc < n_assoc; assoc++)
            {
                address = CVMX_ADD_SEG(CVMX_MIPS_SPACE_XKPHYS,
                                       (assoc << assoc_shift) |
                                       (set << set_shift));
                CVMX_CACHE_WBIL2I(address, 0);
            }
        }
    }
    else
    {
        for (set=0; set < n_set; set++)
            for(assoc=0; assoc < n_assoc; assoc++)
                cvmx_l2c_flush_line(assoc, set);
    }
}

static uint16_t ddr3_crc16 (uint8_t *ptr, int count)
{
    /* From DDR3 spd specification */
    int crc, i;
    crc = 0;
    while (--count >= 0) {
        crc = crc ^ (int)*ptr++ << 8;
        for (i = 0; i < 8; ++i)
            if (crc & 0x8000)
                crc = crc << 1 ^ 0x1021;
            else
                crc = crc << 1;
    }
    return (crc & 0xFFFF);
}
static int validate_spd_checksum_ddr3(int twsi_addr, int silent)
{

    int i;
    int rv;
    uint16_t crc_comp;
    uint8_t spd_data[128];
    int crc_bytes = 126;

    for (i = 0;i < 128;i++)
    {
        rv = octeon_twsi_read(twsi_addr, i);
        if (rv < 0)
            return 0;   /* TWSI read error */
        spd_data[i]  = (uint8_t)rv;
    }

    /* Check byte 0 to see how many bytes checksum is over */
    if (spd_data[0] & 0x80)
        crc_bytes = 117;

    crc_comp = ddr3_crc16(spd_data, crc_bytes);

    if (spd_data[126] == (crc_comp & 0xff) && spd_data[127] == (crc_comp >> 8))
    {
        return(1);
    }
    else
    {
        if (!silent)
            printf("DDR3 SPD CRC error, spd addr: 0x%x\n", twsi_addr);
        return(0);
    }
}

int validate_spd_checksum(int twsi_addr, int silent)
{
    int rv;
    
    /* Look up module type to determine if DDR2 or DDR3 */
    rv = octeon_twsi_read(twsi_addr, 2);
    if (rv < 0)
        return 0;   /* TWSI read error */
    if (rv >= 0x8 && rv <= 0xA)
        return(validate_spd_checksum_ddr2(twsi_addr, silent));
    if (rv >= 0xB && rv <= 0xB)
        return(validate_spd_checksum_ddr3(twsi_addr, silent));

    if (!silent)
        printf("Unrecognized DIMM type: 0x%x at spd address: 0x%x\n", rv, twsi_addr);
    return(0);

}

/* Read an DIMM SPD value, either using TWSI to read it from the DIMM, or
** from a provided array. */
int read_spd(dimm_config_t dimm_config, int dimm_index, int spd_field)
{
    dimm_index = !!dimm_index;
        
    /* If pointer to data is provided, use it, otherwise read from SPD over twsi */
    if (dimm_config.spd_ptrs[dimm_index])
    {
        return dimm_config.spd_ptrs[dimm_index][spd_field];
    }
    else if (dimm_config.spd_addrs[dimm_index])
    {
        return octeon_twsi_read(dimm_config.spd_addrs[dimm_index], spd_field);
    }
    else 
        return -1;
}

int validate_dimm(dimm_config_t dimm_config, int dimm_index)
{
    dimm_index = !!dimm_index;  /* Normalize to 0/1 */
    int spd_addr = dimm_config.spd_addrs[dimm_index];

    /* Only validate 'real' dimms, assume compiled in values are OK */
    
    if (!dimm_config.spd_ptrs[dimm_index])
    {
        int val0, val1;
        int dimm_type = read_spd(dimm_config, dimm_index, DDR2_SPD_MEM_TYPE) & 0xff;

        switch(dimm_type)
        {
        case 0x08:              /* DDR2 */
            val0 = read_spd(dimm_config, dimm_index, DDR2_SPD_NUM_ROW_BITS);
            val1 = read_spd(dimm_config, dimm_index, DDR2_SPD_NUM_COL_BITS);
            if (val0 < 0 && val1 < 0)
                return(0); /* Failed to read dimm */
            if (val0 == 0xff && val1 == 0xff)
                return(0); /* Blank SPD or otherwise unreadable device */

            /* Don't treat bad checksums as fatal. */
            validate_spd_checksum(spd_addr, 0);
            break;

        case 0x0B:              /* DDR3 */
            val0 = read_spd(dimm_config, dimm_index, DDR3_SPD_DENSITY_BANKS);
            val1 = read_spd(dimm_config, dimm_index, DDR3_SPD_ADDRESSING_ROW_COL_BITS);
            if (val0 < 0 && val1 < 0)
                return(0); /* Failed to read dimm */
            if (val0 == 0xff && val1 == 0xff)
                return(0); /* Blank SPD or otherwise unreadable device */

            /* Don't treat bad checksums as fatal. */
            validate_spd_checksum(spd_addr, 0);
            break;

        default:
                return(0);      /* Failed to read dimm */
        }
    }
    return 1;
}

static void report_ddr2_dimm(dimm_config_t dimm_config, int upper_dimm, int dimm)
{
    upper_dimm = !!upper_dimm;  /* Normalize to 0/1 */
    int spd_ecc;
    int spd_rdimm; 
    int i;
    int strlen;
    uint32_t serial_number;
    char part_number[19] = {0}; /* 18 bytes plus string terminator */

    spd_ecc   = !!(read_spd(dimm_config, upper_dimm, DDR2_SPD_CONFIG_TYPE) & 2);
    spd_rdimm = !!(read_spd(dimm_config, upper_dimm, DDR2_SPD_DIMM_TYPE) & 0x11);

    printf("DIMM %d: DDR2 %s, %s", dimm,
              (spd_rdimm ? "Registered" : "Unbuffered"), (spd_ecc ? "ECC" : "non-ECC"));

    strlen = 0;
    for (i = 0; i < 18; ++i) {
        part_number[i] = (read_spd(dimm_config, upper_dimm, i+73) & 0xff);
        if (part_number[i])
            ++strlen;
        //debug_print("spd[%d]: 0x%02x\n", i+128, part_number[i]);
    }
    if (strlen)
        printf("  %s", part_number);

    serial_number = 0;
    for (i = 0; i<4; ++i) {
        serial_number |= ((read_spd(dimm_config, upper_dimm, 95+i) & 0xff) << ((3-i)*8));
    }
    if ((serial_number!=0) && (serial_number!=0xffffffff))
        printf("  s/n %u", serial_number);
    else {
        int spd_chksum;
        spd_chksum  =   0xff & read_spd(dimm_config, upper_dimm, DDR2_SPD_CHECKSUM);
        printf("  chksum: %u (0x%02x)", spd_chksum, spd_chksum);
    }

    printf("\n");
}

static void report_ddr3_dimm(dimm_config_t dimm_config, int upper_dimm, int dimm)
{
    upper_dimm = !!upper_dimm;  /* Normalize to 0/1 */
    int spd_ecc;
    unsigned spd_module_type;
    int i;
    int strlen;
    uint32_t serial_number;
    static const char *dimm_types[] = {
	/* 0000	*/ "Undefined",
	/* 0001	*/ "RDIMM",
	/* 0010	*/ "UDIMM",
	/* 0011	*/ "SO-DIMM",
	/* 0100	*/ "Micro-DIMM",
	/* 0101	*/ "Mini-RDIMM",
	/* 0110	*/ "Mini-UDIMM",
	/* 0111	*/ "Mini-CDIMM",
	/* 1000	*/ "72b-SO-UDIMM",
	/* 1001	*/ "72b-SO-RDIMM",
	/* 1010	*/ "72b-SO-CDIMM"
    };

    char part_number[19] = {0}; /* 18 bytes plus string terminator */

    spd_module_type = read_spd(dimm_config, upper_dimm, DDR3_SPD_KEY_BYTE_MODULE_TYPE);

    /* Validate dimm type */
    spd_module_type = (spd_module_type > (sizeof(dimm_types)/sizeof(char*))) ? 0 : spd_module_type;

    spd_ecc         = !!(read_spd(dimm_config, upper_dimm, DDR3_SPD_MEMORY_BUS_WIDTH) & 8);

    printf("DIMM %d: DDR3 %s, %s", dimm,
           dimm_types[spd_module_type],
           (spd_ecc ? "ECC" : "non-ECC"));

    strlen = 0;
    for (i = 0; i < 18; ++i) {
        part_number[i] = (read_spd(dimm_config, upper_dimm, i+128) & 0xff);
        if (part_number[i])
            ++strlen;
        debug_print("spd[%d]: 0x%02x\n", i+128, part_number[i]);
    }
    if (strlen)
        printf("  %s", part_number);

    serial_number = 0;
    for (i = 0 ; i < 4; ++i) {
        serial_number |= ((read_spd(dimm_config, upper_dimm, 122+i) & 0xff) << ((3-i)*8));
    }
    if ((serial_number!=0) && (serial_number!=0xffffffff))
        printf("  s/n %u", serial_number);
    else {
        unsigned spd_chksum;
        spd_chksum  =   0xff & read_spd(dimm_config, upper_dimm, DDR3_SPD_CYCLICAL_REDUNDANCY_CODE_LOWER_NIBBLE);
        spd_chksum |= ((0xff & read_spd(dimm_config, upper_dimm, DDR3_SPD_CYCLICAL_REDUNDANCY_CODE_UPPER_NIBBLE)) << 8);
        printf("  chksum: %u (0x%04x)", spd_chksum, spd_chksum);
    }

    printf("\n");
}

static int lookup_cycle_time_psec (int spd_byte)
{
    static const char _subfield_b[] = { 00, 10, 20, 30, 40, 50, 60, 70,
                                        80, 90, 25, 33, 66, 75, 00, 00 };

    return (((spd_byte>>4)&0xf) * 1000) + _subfield_b[spd_byte&0xf] * 10;
}



static int lookup_delay_psec (int spd_byte)
{
    static const char _subfield_b[] =  { 0, 25, 50, 75 };

    return (((spd_byte>>2)&0x3f) * 1000) + _subfield_b[spd_byte&0x3] * 10;
}



static int lookup_refresh_rate_nsec (int spd_byte)
{
#define BASE_REFRESH 15625
    static const int _refresh[] =
        {
            (BASE_REFRESH),     /* 15.625 us */
            (BASE_REFRESH/4),   /* Reduced (.25x)..3.9us */
            (BASE_REFRESH/2),   /* Reduced (.5x)...7.8us */
            (BASE_REFRESH*2),   /* Extended (2x)...31.3us */
            (BASE_REFRESH*4),   /* Extended (4x)...62.5us */
            (BASE_REFRESH*8),   /* Extended (8x)...125us */
        };

    if ((spd_byte&0x7f) > 5) {
        error_print("Unsupported refresh rate: %#04x\n", spd_byte);
        return (-1);
    }

    return (_refresh[spd_byte&0x7f]);
}



static int lookup_rfc_psec (int spd_trfc, int spd_trfc_ext)
{
    static const char _subfield_b[] =  { 0, 25, 33, 50, 66, 75 };
    int trfc = (spd_trfc * 1000) + _subfield_b[(spd_trfc_ext>>1)&0x7] * 10;
    return ((spd_trfc_ext&1) ? (trfc + (256*1000)) : trfc);
}



static int select_cas_latency (ulong tclk_psecs, int spd_cas_latency, unsigned *cycle_clx)
{
    int i;
    int max_cl = 7;

    while ( (((spd_cas_latency>>max_cl)&1) == 0) && (max_cl > 0) )
        --max_cl;

    if (max_cl == 0) {
        error_print("CAS Latency was not specified by SPD: %#04x\n", spd_cas_latency);
        return (-1);
    }

    /* Pick that fastest CL that is within spec. */
    for (i = 2; i >= 0; --i) {
        if ((spd_cas_latency>>(max_cl-i))&1) {
            ddr_print("CL%d Minimum Clock Rate                        : %6d ps\n", max_cl-i, cycle_clx[i]);

            if (tclk_psecs >= cycle_clx[i])
                return max_cl-i;
        }
    }

    error_print("WARNING!!!!!!: DDR Clock Rate (tCLK) exceeds DIMM specifications!!!!!!!!\n");
    return max_cl;
}


/*
** Calculate the board delay in quarter-cycles.  That value is used as
** an index into the delay parameter table.
*/
static ulong compute_delay_params(ulong board_skew, ulong tclk_psecs, int silo_qc_unsupported)
{
    int idx;
    static const ulong _params[] = {
        /* idx   board delay (cycles) tskw  silo_hc  silo_qc */
        /* ====  ==================== ====  =======  ======= */
        /* [ 0]     0.00 - 0.25 */     0 |   1<<8 |   0<<16,
        /* [ 1]     0.25 - 0.50 */     0 |   1<<8 |   1<<16,
        /* [ 2]     0.50 - 0.75 */     1 |   0<<8 |   0<<16,
        /* [ 3]     0.75 - 1.00 */     1 |   0<<8 |   1<<16,
        /* [ 4]     1.00 - 1.25 */     1 |   1<<8 |   0<<16,
        /* [ 5]     1.25 - 1.50 */     1 |   1<<8 |   1<<16,
        /* [ 6]     1.50 - 1.75 */     2 |   0<<8 |   0<<16,
        /* [ 7]     1.75 - 2.00 */     2 |   0<<8 |   1<<16,
        /* [ 8]     2.00 - 2.25 */     2 |   1<<8 |   0<<16,
        /* [ 9]     2.25 - 2.50 */     2 |   1<<8 |   1<<16,
        /* [10]     2.50 - 2.75 */     3 |   0<<8 |   0<<16,
        /* [11]     2.75 - 3.00 */     3 |   0<<8 |   1<<16,
        /* [12]     3.00 - 3.25 */     3 |   1<<8 |   0<<16,
        /* [13]     3.25 - 3.50 */     3 |   1<<8 |   1<<16,
        /* [14]     3.50 - 3.75 */     4 |   0<<8 |   0<<16,
    };


    /*
    ** Octeon pass 1 did not support the quarter-cycle delay setting.
    ** In that case round to the settings for the nearest half-cycle.
    ** This is effectively done by rounding up to the next
    ** quarter-cycle setting and ignoring the silo_qc bit when the
    ** quarter-cycle setting is not supported.
    */

    idx = ((board_skew*4) + (tclk_psecs*silo_qc_unsupported)) / tclk_psecs;
    return _params[min(idx, 14)];
}


static int encode_row_lsb(uint32_t cpu_id, int row_lsb, int ddr_interface_wide)
{
    int encoded_row_lsb;
    static const int _params[] = {
        /* 110: row_lsb = mem_adr[12] */ 6,
        /* 111: row_lsb = mem_adr[13] */ 7,
        /* 000: row_lsb = mem_adr[14] */ 0,
        /* 001: row_lsb = mem_adr[15] */ 1,
        /* 010: row_lsb = mem_adr[16] */ 2,
        /* 011: row_lsb = mem_adr[17] */ 3,
        /* 100: row_lsb = mem_adr[18] */ 4
        /* 101: RESERVED              */
    };

    int row_lsb_start = 12;
    if (octeon_is_cpuid(OCTEON_CN30XX))
        row_lsb_start = 12;
    else if (octeon_is_cpuid(OCTEON_CN31XX))
        row_lsb_start = 12;
    else if (octeon_is_cpuid(OCTEON_CN38XX))
        row_lsb_start = 14;
    else if (octeon_is_cpuid(OCTEON_CN58XX))
        row_lsb_start = 14;
    else if (octeon_is_cpuid(OCTEON_CN56XX))
        row_lsb_start = 12;
    else if (octeon_is_cpuid(OCTEON_CN52XX))
        row_lsb_start = 12;
    else if (octeon_is_cpuid(OCTEON_CN50XX))
        row_lsb_start = 12;
    else 
        printf("ERROR: Unsupported Octeon model: 0x%x\n", cpu_id);

    if (octeon_is_cpuid(OCTEON_CN30XX) || octeon_is_cpuid(OCTEON_CN31XX) || octeon_is_cpuid(OCTEON_CN56XX)
        || octeon_is_cpuid(OCTEON_CN50XX) || octeon_is_cpuid(OCTEON_CN52XX)
        )
    {
        encoded_row_lsb      = _params[row_lsb  + ddr_interface_wide - row_lsb_start];
        debug_print("_params[%d] = %d\n", (row_lsb  + ddr_interface_wide - row_lsb_start), encoded_row_lsb);
    }
    else
    {
        encoded_row_lsb      = row_lsb + ddr_interface_wide - row_lsb_start ;
    }
    return encoded_row_lsb;
}


static int encode_pbank_lsb(uint32_t cpu_id, int pbank_lsb, int ddr_interface_wide)
{
    int encoded_pbank_lsb;

    static const int _params_cn31xx[] = {
        14, /* 1110:pbank[1:0] = mem_adr[26:25]    / rank = mem_adr[24] (if bunk_ena) */
        15, /* 1111:pbank[1:0] = mem_adr[27:26]    / rank = mem_adr[25] (if bunk_ena) */
         0, /* 0000:pbank[1:0] = mem_adr[28:27]    / rank = mem_adr[26] (if bunk_ena) */
         1, /* 0001:pbank[1:0] = mem_adr[29:28]    / rank = mem_adr[27]      "        */
         2, /* 0010:pbank[1:0] = mem_adr[30:29]    / rank = mem_adr[28]      "        */
         3, /* 0011:pbank[1:0] = mem_adr[31:30]    / rank = mem_adr[29]      "        */
         4, /* 0100:pbank[1:0] = mem_adr[32:31]    / rank = mem_adr[30]      "        */
         5, /* 0101:pbank[1:0] = mem_adr[33:32]    / rank = mem_adr[31]      "        */
         6, /* 0110:pbank[1:0] ={1'b0,mem_adr[33]} / rank = mem_adr[32]      "        */
         7, /* 0111:pbank[1:0] ={2'b0}             / rank = mem_adr[33]      "        */
            /* 1000-1101: RESERVED                                                    */
    };

    int pbank_lsb_start = 0;
    if (octeon_is_cpuid(OCTEON_CN30XX))
        pbank_lsb_start = 25;
    else if (octeon_is_cpuid(OCTEON_CN31XX))
        pbank_lsb_start = 25;
    else if (octeon_is_cpuid(OCTEON_CN38XX))
        pbank_lsb_start = 27;
    else if (octeon_is_cpuid(OCTEON_CN58XX))
        pbank_lsb_start = 27;
    else if (octeon_is_cpuid(OCTEON_CN56XX))
        pbank_lsb_start = 25;
    else if (octeon_is_cpuid(OCTEON_CN52XX))
        pbank_lsb_start = 25;
    else if (octeon_is_cpuid(OCTEON_CN50XX))
        pbank_lsb_start = 25;
    else 
        printf("ERROR: Unsupported Octeon model: 0x%x\n", cpu_id);
    if (octeon_is_cpuid(OCTEON_CN30XX) || octeon_is_cpuid(OCTEON_CN31XX) || octeon_is_cpuid(OCTEON_CN56XX)
        || octeon_is_cpuid(OCTEON_CN50XX) || octeon_is_cpuid(OCTEON_CN52XX)
        )
    {
        encoded_pbank_lsb      = _params_cn31xx[pbank_lsb + ddr_interface_wide - pbank_lsb_start];
        debug_print("_params[%d] = %d\n", (pbank_lsb + ddr_interface_wide - pbank_lsb_start), encoded_pbank_lsb);
    }
    else
    {
        encoded_pbank_lsb      = pbank_lsb + ddr_interface_wide - pbank_lsb_start;
    }
    return encoded_pbank_lsb;
}

static int encode_row_lsb_ddr3(uint32_t cpu_id, int row_lsb, int ddr_interface_wide)
{
    int encoded_row_lsb;
    int row_lsb_start = 14;

    /*  Decoding for row_lsb             */
    /*       000: row_lsb = mem_adr[14]  */
    /*       001: row_lsb = mem_adr[15]  */
    /*       010: row_lsb = mem_adr[16]  */
    /*       011: row_lsb = mem_adr[17]  */
    /*       100: row_lsb = mem_adr[18]  */
    /*       101: row_lsb = mem_adr[19]  */
    /*       110: row_lsb = mem_adr[20]  */
    /*       111: RESERVED               */

    if (octeon_is_cpuid(OCTEON_CN63XX) || octeon_is_cpuid(OCTEON_CN71XX))
        row_lsb_start = 14;
    else
        printf("ERROR: Unsupported Octeon model: 0x%x\n", cpu_id);

    encoded_row_lsb      = row_lsb + ddr_interface_wide - row_lsb_start ;

    return encoded_row_lsb;
}

static int encode_pbank_lsb_ddr3(uint32_t cpu_id, int pbank_lsb, int ddr_interface_wide)
{
    int encoded_pbank_lsb;

    /*  Decoding for pbank_lsb                                             */
    /*       0000:DIMM = mem_adr[28]    / rank = mem_adr[27] (if RANK_ENA) */
    /*       0001:DIMM = mem_adr[29]    / rank = mem_adr[28]      "        */
    /*       0010:DIMM = mem_adr[30]    / rank = mem_adr[29]      "        */
    /*       0011:DIMM = mem_adr[31]    / rank = mem_adr[30]      "        */
    /*       0100:DIMM = mem_adr[32]    / rank = mem_adr[31]      "        */
    /*       0101:DIMM = mem_adr[33]    / rank = mem_adr[32]      "        */
    /*       0110:DIMM = mem_adr[34]    / rank = mem_adr[33]      "        */
    /*       0111:DIMM = 0              / rank = mem_adr[34]      "        */
    /*       1000-1111: RESERVED                                           */

    int pbank_lsb_start = 0;
    if (octeon_is_cpuid(OCTEON_CN63XX) || octeon_is_cpuid(OCTEON_CN71XX))
        pbank_lsb_start = 28;
    else
        printf("ERROR: Unsupported Octeon model: 0x%x\n", cpu_id);

    encoded_pbank_lsb      = pbank_lsb + ddr_interface_wide - pbank_lsb_start;

    return encoded_pbank_lsb;
}

#ifdef PRINT_LMC_REGS
void print_addr_name(uint64_t addr, char *name)
{
    printf("%34s(0x%qx): 0x%qx\n", name, (unsigned long long)addr, (unsigned long long)cvmx_read_csr(addr));
}
#define PRINT_REG(X)     print_addr_name(X,#X)
static void octeon_print_lmc_regs(uint32_t cpu_id, int int_num)
{
    PRINT_REG(CVMX_LMCX_CTL(int_num));
    PRINT_REG(CVMX_LMCX_CTL1(int_num));
    PRINT_REG(CVMX_LMCX_DDR2_CTL(int_num));
    PRINT_REG(CVMX_LMC_RODT_CTL);

    if (octeon_is_cpuid(OCTEON_CN31XX) || octeon_is_cpuid(OCTEON_CN30XX))
    {
        PRINT_REG(CVMX_LMCX_WODT_CTL0(int_num));
        PRINT_REG(CVMX_LMCX_WODT_CTL1(int_num));
    }
    else
    {
        PRINT_REG(CVMX_LMC_WODT_CTL);
    }
    PRINT_REG(CVMX_LMCX_DELAY_CFG(int_num));
    PRINT_REG(CVMX_LMCX_MEM_CFG0(int_num));
    PRINT_REG(CVMX_LMCX_MEM_CFG1(int_num));
    if (octeon_is_cpuid(OCTEON_CN58XX) || octeon_is_cpuid(OCTEON_CN56XX)
        || octeon_is_cpuid(OCTEON_CN50XX) || octeon_is_cpuid(OCTEON_CN52XX)
        )
    {
        PRINT_REG(CVMX_LMCX_RODT_COMP_CTL(int_num));
        PRINT_REG(CVMX_LMC_PLL_CTL);
    }
}
#endif

int64_t octeon_read_lmcx_read_level_dbg(int ddr_interface_num, int idx)
{
    cvmx_lmcx_read_level_dbg_t read_level_dbg;
    read_level_dbg.u64 = 0;
    read_level_dbg.s.byte = idx;
    cvmx_write_csr(CVMX_LMCX_READ_LEVEL_DBG(ddr_interface_num), read_level_dbg.u64);
    read_level_dbg.u64 = cvmx_read_csr(CVMX_LMCX_READ_LEVEL_DBG(ddr_interface_num));
    return read_level_dbg.s.bitmask;
}

static uint64_t octeon_read_lmcx_ddr3_rlevel_dbg(int ddr_interface_num, int idx)
{
    cvmx_lmcx_rlevel_dbg_t rlevel_dbg;
    cvmx_lmcx_rlevel_ctl_t rlevel_ctl;

    rlevel_ctl.u64 = cvmx_read_csr(CVMX_LMCX_RLEVEL_CTL(ddr_interface_num));
    rlevel_ctl.s.byte = idx;

    cvmx_write_csr(CVMX_LMCX_RLEVEL_CTL(ddr_interface_num), rlevel_ctl.u64);
    cvmx_read_csr(CVMX_LMCX_RLEVEL_CTL(ddr_interface_num));

    rlevel_dbg.u64 = cvmx_read_csr(CVMX_LMCX_RLEVEL_DBG(ddr_interface_num));
    return rlevel_dbg.s.bitmask;
}

static uint64_t octeon_read_lmcx_ddr3_wlevel_dbg(int ddr_interface_num, int idx)
{
    cvmx_lmcx_wlevel_dbg_t wlevel_dbg;

    wlevel_dbg.u64 = 0;
    wlevel_dbg.s.byte = idx;

    cvmx_write_csr(CVMX_LMCX_WLEVEL_DBG(ddr_interface_num), wlevel_dbg.u64);
    cvmx_read_csr(CVMX_LMCX_WLEVEL_DBG(ddr_interface_num));

    wlevel_dbg.u64 = cvmx_read_csr(CVMX_LMCX_WLEVEL_DBG(ddr_interface_num));
    return wlevel_dbg.s.bitmask;
}


#define DEBUG_VALIDATE_BITMASK 0
#if DEBUG_VALIDATE_BITMASK
#define debug_bitmask_print printf
#else
#define debug_bitmask_print(...)
#endif

static int validate_ddr3_rlevel_bitmask(uint64_t bitmask, ulong tclk_psecs)
{
    int mask =0x3f;
    int shifted_bitmask;
    int i, leader, width;
    int errors  = 0;
    int bubble  = 0;
    int blank   = 0;
    int narrow  = 0;
    int invalid = 0;

    if (bitmask == 0) {
        blank += RLEVEL_BITMASK_BLANK_ERROR;
    } else {
        for (leader=0; leader<64; ++leader) {
            shifted_bitmask = (bitmask>>leader);

            /* Look for at leaset 2 consecutive */
            if ((shifted_bitmask&3) == 3)
                break;

            /* Detect leading bubbles in the bitmask */
            if (shifted_bitmask&1) {
                bubble += RLEVEL_BITMASK_BUBBLE_BITS_ERROR;
            }
        }

        if (leader == 64) {
            for (leader=0; leader<64; ++leader) {
                shifted_bitmask = (bitmask>>leader);

                /* Resort to single bit if necessary */
                if ((shifted_bitmask&3) == 1)
                    break;
            }
        }

        shifted_bitmask &= mask;

        width=0;
        while ((shifted_bitmask>>width)&1) ++width;

        /* Detect if bitmask is too narrow. */
        if (width < 4)
            narrow = (4 - width) / 2;
        if (width == 1)
            narrow += RLEVEL_BITMASK_NARROW_ERROR; /* Excessively narrow */

        /* Detect extra invalid bits at the trailing end of the bitmask. */
        for (i=width+leader; i<64; ++i) {
            if ((bitmask>>i) & 1)
                invalid += RLEVEL_BITMASK_INVALID_BITS_ERROR;
        }
    }

    errors = bubble + blank + narrow + invalid;

    debug_bitmask_print("bitmask:%05llx shifted:%05x leader:%2d",
                        (unsigned long long) bitmask, shifted_bitmask, leader);

    debug_bitmask_print(" errors: %d (%2d,%2d,%2d,%2d)", errors, bubble, blank, narrow, invalid);

    if (errors) {
        debug_bitmask_print(" => invalid");
    }

    debug_bitmask_print("\n");

    return errors;
}

static int compute_ddr3_rlevel_delay (uint64_t bitmask, cvmx_lmcx_rlevel_ctl_t rlevel_ctl)
{
    int mask = 0x3f;
    int shifted_bitmask;
    int leader, width;
    int delay;

    if (bitmask == 0)
        return (0);

    for (leader=0; leader<64; ++leader) {
        shifted_bitmask = (bitmask>>leader);

        /* Look for at leaset 2 consecutive */
        if ((shifted_bitmask&3) == 3)
            break;
    }

    if (leader == 64) {
        for (leader=0; leader<64; ++leader) {
            shifted_bitmask = (bitmask>>leader);

            /* Resort to single bit if necessary */
            if ((shifted_bitmask&3) == 1)
                break;
        }
    }

    shifted_bitmask &= mask;

    width=0;
    while ((shifted_bitmask>>width)&1) ++width;

    if (rlevel_ctl.s.offset_en) {
        delay = max(leader, leader + width - 1 - rlevel_ctl.s.offset);
    } else {
        if (rlevel_ctl.s.offset) {
            delay = max(leader + rlevel_ctl.s.offset, leader + 1);
            /* Insure that the offset delay falls within the bitmask */
            delay = min(delay, leader + width-1);
        } else {
            delay = (width-1)/2 + leader; /* Round down */
            //delay = (width/2) + leader;   /* Round up */
        }
    }

    return (delay);
}


/* Sometimes the pass/fail results for all possible delay settings
 * determined by the read-leveling sequence is too forgiving.  This
 * usually occurs for DCLK speeds below 300 MHz. As a result the
 * passing range is exaggerated. This function accepts the bitmask
 * results from the sequence and truncates the passing range to a
 * reasonable range and recomputes the proper deskew setting.
 */
int adjust_read_level_byte(int bitmask)
{
    int mask;
    int shift;
    int leader;
    int adjusted = 0;
    int adjusted_byte_delay = 0;

    for (mask=0xf, leader=3; mask>0; mask=mask>>1, --leader) {
        for (shift=15-leader; shift>=0; --shift) {

#if DEBUG_DEFINED
            ddr_print("bitmask:%04x, mask:%x, shift:%d, leader:%d\n",
                      bitmask, mask, shift, leader);
#endif

            if (((bitmask>>shift) & mask) == mask) {
                adjusted = 1;
                break;
            }
        }

        if (adjusted)
            break;
    }

    if (adjusted)
        adjusted_byte_delay = shift + leader/2;

    return adjusted_byte_delay;
}


/* Default ODT config must disable ODT */
/* Must be const (read only) so that the structure is in flash */
const dimm_odt_config_t disable_odt_config[] = {
    /* DIMMS   ODT_ENA ODT_MASK ODT_MASK1 QS_DIC RODT_CTL DIC */ \
    /* =====   ======= ======== ========= ====== ======== === */ \
    /*   1 */ {   0,    0x0000,   0x0000,    0,   0x0000,  0  },  \
    /*   2 */ {   0,    0x0000,   0x0000,    0,   0x0000,  0  },  \
    /*   3 */ {   0,    0x0000,   0x0000,    0,   0x0000,  0  },  \
    /*   4 */ {   0,    0x0000,   0x0000,    0,   0x0000,  0  },  \
};

/* Memory controller setup function */
int init_octeon_dram_interface(uint32_t cpu_id,
                               ddr_configuration_t *ddr_configuration,
                               uint32_t ddr_hertz,
                               uint32_t cpu_hertz,
                               uint32_t ddr_ref_hertz,
                               int board_type,
                               int board_rev_maj,
                               int board_rev_min,
                               int ddr_interface_num,
                               uint32_t ddr_interface_mask
                               )
{


     uint32_t mem_size_mbytes;
     uint64_t addr;
     
     printf("Initializing memory this may take some time...\n");
#ifdef CONFIG_MAG8600
     mem_size_mbytes = init_octeon_ddr2_interface(cpu_id,
                                                     ddr_configuration,
                                                     ddr_hertz,
                                                     cpu_hertz,
                                                     ddr_ref_hertz,
                                                     board_type,
                                                     board_rev_maj,
                                                     board_rev_min,
                                                     ddr_interface_num,
                                                     ddr_interface_mask
                                                     );

#elif defined(CONFIG_OCTEON3)
     mem_size_mbytes = init_octeon3_ddr3_interface(cpu_id,
                                                   ddr_configuration,
                                                   ddr_hertz,
                                                   cpu_hertz,
                                                   ddr_ref_hertz,
                                                   board_type,
                                                   board_rev_maj,
                                                   board_rev_min,
                                                   ddr_interface_num,
                                                   ddr_interface_mask
                                                   );
#else
     if (IS_DDR3_MODEL(board_type)) {
        mem_size_mbytes = init_octeon_ddr3_interface(cpu_id,
                                                     ddr_configuration,
                                                     ddr_hertz,
                                                     cpu_hertz,
                                                     ddr_ref_hertz,
                                                     board_type,
                                                     board_rev_maj,
                                                     board_rev_min,
                                                     ddr_interface_num,
                                                     ddr_interface_mask
                                                     );
     } else {
        mem_size_mbytes = init_octeon_ddr2_interface(cpu_id,
                                                     ddr_configuration,
                                                     ddr_hertz,
                                                     cpu_hertz,
                                                     ddr_ref_hertz,
                                                     board_type,
                                                     board_rev_maj,
                                                     board_rev_min,
                                                     ddr_interface_num,
                                                     ddr_interface_mask
                                                     );
     }
#endif
     return (mem_size_mbytes);
}

#ifdef CONFIG_JSRXNLE
static int init_non_spd_ddr2_interface(uint32_t cpu_id,
                                       const ddr_configuration_t *ddr_configuration,
                                       uint32_t ddr_hertz,
                                       uint32_t cpu_hertz,
                                       uint32_t ddr_ref_hertz,
                                       uint32_t board_type,
                                       int board_rev_maj,
                                       int board_rev_min,
                                       int ddr_interface_num,
                                       uint32_t ddr_interface_mask
                                       )
{
    DECLARE_GLOBAL_DATA_PTR;
    uint8_t ddr_cfg_type;
    uint8_t ddr_make;
    int bank_bits;
    int pbank_lsb;
    int row_lsb;
    int column_bits_start;
    int bunk_enable;
    int ddr_interface_wide = 1;
    int dimm_count = 1;
    const ddr_config_data_t *ddr_cfg = NULL;
    const lmc_reg_val_t *lmc_reg = NULL;
    uint32_t mem_size_mbytes = 0;
    
    /*
     * ddr_make here will be set to default
     * type of memory which is applicable for
     * proto 1 2 3 of srx210 and srx100
     */ 
    ddr_make = gd->board_desc.board_dram;
    board_type = gd->board_desc.board_type;

    switch (ddr_make) {
    case QIMONDA_8x2Gb:
        ddr_cfg_type = QIMONDA_8x2Gb;
#ifdef CONFIG_JSRXNLE
        if (IS_PLATFORM_SRX100(board_type)) {
             ddr_make = HIGH_MEM_8x2Gb_SRX100;
        }
        if (is_proto_id_supported() && IS_PLATFORM_SRX210(board_type)) {
            ddr_make = HIGH_MEM_8x2Gb_SRX210_533;
        }
#endif
        break;
    case QIMONDA_4x2Gb:
        ddr_cfg_type = QIMONDA_4x2Gb;
#ifdef CONFIG_JSRXNLE
        if (is_proto_id_supported() && IS_PLATFORM_SRX210(board_type)) {
            ddr_make = LOW_MEM_4x2Gb_SRX210_533;
        }
#endif
        break;
    case QIMONDA_8x1Gb:
        ddr_cfg_type = HIGH_MEM_8x1Gb;
        /* 
         * ddr_make which is an index to array of lmc_reg_val
         * will changes for proto 4 and above for srx210
         * as proto4 and will have differnt set of ddr
         * values.
         */
#ifdef CONFIG_JSRXNLE
         /*
          * For all proto's of SRX100 of higm mem, DDR values
          * will be picked from lmc_reg_val using index
          * HIGH_MEM_8x1Gb_SRX100
          */
         if (IS_PLATFORM_SRX100(board_type)) {
             ddr_make =
                 jsrxnle_read_eeprom(JSRXNLE_EEPROM_MAJOR_REV_OFFSET) ==
                 SRX100_MICRON_REV ? MICRON_8x1Gb_SRX100 : HIGH_MEM_8x1Gb_SRX100;
         }
         if (is_proto_id_supported() && IS_PLATFORM_SRX210(board_type)) {
             ddr_make = IS_PLATFORM_SRX210_533(board_type) ?
                 HIGH_MEM_8x1Gb_SRX210_533 : HIGH_MEM_8x1Gb_SRX210_P4;
         }
#endif
        break;
    case QIMONDA_4x1Gb:
        ddr_cfg_type = LOW_MEM_4x1Gb;
        /* 
         * ddr_make which is an index to array of lmc_reg_val
         * will changes for proto 4 and above for srx210
         * as proto4 and will have differnt set of ddr
         * values.
         */
#ifdef CONFIG_JSRXNLE
        /*
         * For all proto's of SRX100 of low mem, DDR values
         * will be picked from lmc_reg_val using index
         * LOW_MEM_4x1Gb_SRX100 and will be used by default
         */
        if (IS_PLATFORM_SRX100(board_type)) {
            ddr_make = LOW_MEM_4x1Gb_SRX100;
        }

        /*
         * if SRX210 supports proto_id, dram values
         * will be picked from lmc_reg_val using
         * LOW_MEM_4x1Gb_SRX210_P4.
         * if the proto is SRX100 and supports proto_id 
         * dram values will be picked using 
         * HIGH_MEM_8x1Gb_SRX100 and entire 1GB of ram 
         * will be initialised.
         */
         if (is_proto_id_supported()) {
            if (IS_PLATFORM_SRX210(board_type)) {
                ddr_make = IS_PLATFORM_SRX210_533(board_type) ? 
                    LOW_MEM_4x1Gb_SRX210_533 : LOW_MEM_4x1Gb_SRX210_P4;
            } else if (IS_PLATFORM_SRX100(board_type)) {
                ddr_make =
                    jsrxnle_read_eeprom(JSRXNLE_EEPROM_MAJOR_REV_OFFSET) ==
                    SRX100_MICRON_REV ? MICRON_8x1Gb_SRX100 : 
                    HIGH_MEM_8x1Gb_SRX100;
            }
       }
        break;
    case ELPIDA_8x512Mb:
        ddr_cfg_type = LOW_MEM_8x512Mb;
        /*
         * For all proto's of SRX100 of low mem with 8*512Mb, 
         * DDR values will be picked from lmc_reg_val using index
         * LOW_MEM_4x1Gb_SRX100
         */
        if (IS_PLATFORM_SRX100(board_type)) {
            ddr_make = LOW_MEM_8x512Gb_SRX100;
        }
        break;
#endif
    default:
        ddr_make = 0; 
        ddr_cfg_type = 0;
        break;
    }

    ddr_cfg = &ddr_config_data[ddr_cfg_type];
    lmc_reg = &lmc_reg_val[ddr_make];

    if (ddr_cfg->num_banks == 8)
        bank_bits = 3;
    else if (ddr_cfg->num_banks == 4)
        bank_bits = 2;

    if (octeon_is_cpuid(OCTEON_CN30XX) | octeon_is_cpuid(OCTEON_CN50XX))
        column_bits_start = 1;
    else if (octeon_is_cpuid(OCTEON_CN31XX))
        column_bits_start = 2;
    else if (octeon_is_cpuid(OCTEON_CN38XX))
        column_bits_start = 3;
    else if (octeon_is_cpuid(OCTEON_CN58XX))
        column_bits_start = 3;
    else
        printf("ERROR: Unsupported Octeon model: 0x%x\n", cpu_id);

    bunk_enable = (ddr_cfg->num_ranks > 1);
    row_lsb = column_bits_start + ddr_cfg->col_bits + bank_bits;
    pbank_lsb = row_lsb + ddr_cfg->row_bits + bunk_enable;

    if (octeon_is_cpuid(OCTEON_CN31XX) || octeon_is_cpuid(OCTEON_CN30XX) || octeon_is_cpuid(OCTEON_CN50XX)) {
        mem_size_mbytes =  dimm_count * ((1ull << (pbank_lsb+ddr_interface_wide)) >> 20);
    } else {
        mem_size_mbytes =  dimm_count * ((1ull << pbank_lsb) >> 20);
    }

    cvmx_write_csr(CVMX_LMC_CTL, lmc_reg->lmc_ctl);

    /* 2. Read L2D_BST0 and wait for the result. */
    cvmx_read_csr(CVMX_L2D_BST0); /* Read CVMX_L2D_BST0 */

    /* 3. Wait 10 us (LMC_CTL[PLL_BYPASS] and LMC_CTL[PLL_DIV2] must not
     * transition after this) */
    octeon_delay_cycles(6000);  /* Wait 10 us */

    /* 4. Write LMC_DDR2_CTL[QDLL_ENA] = 1. */ /* Is it OK to write 0 first?
                                                */
    cvmx_write_csr(CVMX_LMC_DDR2_CTL, 0);
    cvmx_read_csr(CVMX_LMC_DDR2_CTL);

    cvmx_write_csr(CVMX_LMC_DDR2_CTL, lmc_reg->lmc_ddr2_ctl);
    cvmx_read_csr(CVMX_LMC_DDR2_CTL);
    octeon_delay_cycles(2000);   // must be 200 dclocks

    /* 5. Read L2D_BST0 and wait for the result. */
    cvmx_read_csr(CVMX_L2D_BST0); /* Read CVMX_L2D_BST0 */

    /* 6. Wait 10 us (LMC_DDR2_CTL[QDLL_ENA] must not transition after this) */
    octeon_delay_cycles(2000);  /* Wait 10 us */

    /*
     * 7. Write LMC_CTL[DRESET] = 0 (at this point, the DCLK is running and
     * the memory controller is out of reset)
     */
    cvmx_write_csr(CVMX_LMC_CTL, lmc_reg->lmc_ctl);

    /*
     * Next, boot software must re-initialize the LMC_MEM_CFG1, LMC_CTL, and
     * LMC_DDR2_CTL CSRs, and also the LMC_WODT_CTL and LMC_RODT_CTL
     * CSRs. Refer to Sections 2.3.4, 2.3.5, and 2.3.7 regarding these CSRs
     * (and LMC_MEM_CFG0).
     */
    if (octeon_is_cpuid(OCTEON_CN31XX) || octeon_is_cpuid(OCTEON_CN30XX)) {
        /* JSRXNLE_FIXME, need parameters for 3020 */
    } else {
        cvmx_write_csr(CVMX_LMC_WODT_CTL, lmc_reg->lmc_wodt_ctl);
    }

    cvmx_write_csr(CVMX_LMC_RODT_CTL, lmc_reg->lmc_rodt_ctl);
    cvmx_write_csr(CVMX_LMC_DDR2_CTL, lmc_reg->lmc_ddr2_ctl);
    cvmx_write_csr(CVMX_LMC_DELAY_CFG, lmc_reg->lmc_delay_cfg);
    cvmx_write_csr(CVMX_LMC_MEM_CFG1, lmc_reg->lmc_mem_cfg1);
    cvmx_write_csr(CVMX_LMC_COMP_CTL, lmc_reg->lmc_comp_ctl);

    if (octeon_is_cpuid(OCTEON_CN58XX) || octeon_is_cpuid(OCTEON_CN50XX)) {
        cvmx_write_csr(CVMX_LMC_RODT_COMP_CTL, lmc_reg->lmc_rodt_comp_ctl);
    }
    /* JSRXNLE_FIXME: check this for different memory config */
    cvmx_write_csr(CVMX_LMC_MEM_CFG0, 0x7fe00864);
    cvmx_write_csr(CVMX_LMC_MEM_CFG0, lmc_reg->lmc_mem_cfg0);
    cvmx_read_csr(CVMX_LMC_MEM_CFG0);

    return mem_size_mbytes;
}
#endif

int init_octeon_ddr2_interface(uint32_t cpu_id,
                               const ddr_configuration_t *ddr_configuration,
                               uint32_t ddr_hertz,
                               uint32_t cpu_hertz,
                               uint32_t ddr_ref_hertz,
                               int board_type,
                               int board_rev_maj,
                               int board_rev_min,
                               int ddr_interface_num,
                               uint32_t ddr_interface_mask
                               )
{
#if defined(__U_BOOT__) || defined(unix)
    char *s;
#endif

    uint32_t board_delay;

    DECLARE_GLOBAL_DATA_PTR;
    
#ifdef CONFIG_JSRXNLE
    if (gd->board_desc.board_dram != DRAM_TYPE_SPD) {
        return (init_non_spd_ddr2_interface(cpu_id,
                                            ddr_configuration,
                                            ddr_hertz,
                                            cpu_hertz,
                                            ddr_ref_hertz,
                                            board_type,
                                            board_rev_maj,
                                            board_rev_min,
                                            ddr_interface_num,
                                            ddr_interface_mask));
    }
#endif
#ifdef CONFIG_MAG8600
    if (gd->board_desc.board_dram != DRAM_TYPE_SPD) {
        return 0; 
    }
#endif
    const dimm_odt_config_t *odt_1rank_config = ddr_configuration->odt_1rank_config;
    const dimm_odt_config_t *odt_2rank_config = ddr_configuration->odt_2rank_config;
    const dimm_config_t *dimm_config_table = ddr_configuration->dimm_config_table;
    

    /*
    ** Compute clock rates in picoseconds.
    */
    ulong tclk_psecs = (ulong) divide_nint((1000*1000*1000), (ddr_hertz/1000)); /* Clock in psecs */
    ulong eclk_psecs = (ulong) divide_nint((1000*1000*1000), (cpu_hertz/1000)); /* Clock in psecs */

    cvmx_lmc_ctl_t lmc_ctl;
    cvmx_lmc_ddr2_ctl_t ddr2_ctl;
    cvmx_lmc_mem_cfg0_t mem_cfg0;
    cvmx_lmc_mem_cfg1_t mem_cfg1;
    cvmx_lmc_delay_cfg_t lmc_delay;
    cvmx_lmc_rodt_comp_ctl_t lmc_rodt_comp_ctl;

    int row_bits, col_bits, num_banks, num_ranks, dram_width;
    int fatal_error = 0;        /* Accumulate and report all the errors before giving up */

    int safe_ddr_flag = 0; /* Flag that indicates safe DDR settings should be used */
    int dimm_count = 0;
    int ddr_interface_wide = 0;
    int ddr_interface_width;
    uint32_t mem_size_mbytes = 0;
    unsigned int didx;
    int bank_bits = 0;
    int bunk_enable;
    int column_bits_start = 1;
    int row_lsb;
    int pbank_lsb;
    int use_ecc = 1;
    int spd_cycle_clx;
    int spd_refresh;
    int spd_cas_latency;
    int spd_cycle_clx1;
    int spd_cycle_clx2;
    int spd_trp;
    int spd_trrd;
    int spd_trcd;
    int spd_tras;
    int spd_twr;
    int spd_twtr;
    int spd_trfc_ext;
    int spd_trfc;
    int spd_ecc;
    int spd_rdimm;
    int spd_burst_lngth;
    int cas_latency;
    int refresh;
    int trp;
    int trrd;
    int trcd;
    int tras;
    int twr;
    int twtr;
    int trfc;
    unsigned cycle_clx[3];
    int ctl_silo_hc = 0, ctl_silo_qc = 0, ctl_tskw = 0, ctl_fprch2;
    int ctl_qs_dic, ctl_dic, ctl_odt_ena;
    unsigned int ctl_odt_mask, ctl_rodt_ctl;
    unsigned int ctl_odt_mask1 = 0;
    unsigned long delay_params;
    int odt_idx;
    const dimm_odt_config_t *odt_config;
    int ctl_ddr_eof;
    int ctl_twr;
    int cfg1_tras;
    int cfg1_trcd;
    int cfg1_twtr;
    int cfg1_trp; 
    int cfg1_trfc;
    int cfg1_tmrd;
    int cfg1_trrd;
    int ref_int;
    int cfg0_tcl;


    ddr_print("\nInitializing DDR interface %d, DDR Clock %d, DDR Reference Clock %d, CPUID 0x%08x\n",
              ddr_interface_num, ddr_hertz, ddr_ref_hertz, cpu_id);

    if (dimm_config_table[0].spd_addrs[0] == 0 && !dimm_config_table[0].spd_ptrs[0]) {
        error_print("ERROR: No dimms specified in the dimm_config_table.\n");
        return (-1);
    }
    
    if (ddr_verbose()) {
        printf("DDR SPD Table:");
        for (didx = 0; didx < DDR_CFG_T_MAX_DIMMS; ++didx) {
            if (dimm_config_table[didx].spd_addrs[0] == 0) break;
            printf(" --ddr%dspd=0x%02x", ddr_interface_num, dimm_config_table[didx].spd_addrs[0]);
            if (dimm_config_table[didx].spd_addrs[1] != 0)
                printf(",0x%02x", dimm_config_table[didx].spd_addrs[1]);
        }
        printf("\n");
    }

    /*
    ** Walk the DRAM Socket Configuration Table to see what is installed.
    */
#if defined(CONFIG_JSRXNLE) || defined(CONFIG_MAG8600)
    for (didx = 0; didx < 2; ++didx)
#else
    for (didx = 0; didx < DDR_CFG_T_MAX_DIMMS; ++didx)
#endif
    {
        /* Check for lower DIMM socket populated */
        if (validate_dimm(dimm_config_table[didx], 0)) {
            if (ddr_verbose())
                report_ddr2_dimm(dimm_config_table[didx], 0, dimm_count);
            ++dimm_count;
        } else { break; }       /* Finished when there is no lower DIMM */

        if (octeon_is_cpuid(OCTEON_CN31XX) || octeon_is_cpuid(OCTEON_CN30XX) || octeon_is_cpuid(OCTEON_CN56XX)
            || octeon_is_cpuid(OCTEON_CN50XX) || octeon_is_cpuid(OCTEON_CN52XX)
            )
        {
            ddr_interface_wide = 1;
            continue;               /* Wide means 1 DIMM width for O2P */
        }
        else
        {
            /* Check for upper DIMM socket populated */
            if (validate_dimm(dimm_config_table[didx], 1)) {
                ddr_interface_wide = 1;
                if (ddr_verbose())
                    report_ddr2_dimm(dimm_config_table[didx], 1, dimm_count);
                ++dimm_count;
            }

            /* Check for odd number of DIMMs when 128-bit expected */
            if ((ddr_interface_wide)  && (dimm_count&1)) {
                error_print("ERROR: Install DIMMs in pairs for 128-bit interface\n");
                ++fatal_error;
            }
        }
    }


    initialize_ddr_clock(cpu_id, ddr_configuration, cpu_hertz, ddr_hertz,
                         ddr_ref_hertz, ddr_interface_num, ddr_interface_mask);

    if (!odt_1rank_config)
        odt_1rank_config = disable_odt_config;
    if (!odt_2rank_config)
        odt_2rank_config = disable_odt_config;

    if (((s = getenv("ddr_safe")) != NULL) && (strcmp(s,"yes") == 0)) {
        safe_ddr_flag = 1;
        error_print("Parameter found in environment. ddr_safe = %d\n", safe_ddr_flag);
    }


    if (dimm_count == 0) {
        error_print("ERROR: DIMM 0 not detected.\n");
        return(-1);
    }

    /* Force narrow DDR for chips that do not support wide */
    if (octeon_is_cpuid(OCTEON_CN3020) || octeon_is_cpuid(OCTEON_CN3005))
        ddr_interface_wide = 0;

    /* Temporary workaround.   Some sample 3020 and 3010 parts are mismarked as 3100. */
    if (octeon_is_cpuid(OCTEON_CN31XX)
        && ((board_type == CVMX_BOARD_TYPE_CN3020_EVB_HS5) || (board_type == CVMX_BOARD_TYPE_CN3010_EVB_HS5))) {
        ddr_interface_wide = 0;
    }

    if (ddr_interface_wide && ((s = getenv("ddr_narrow")) != NULL) && (strcmp(s,"yes") == 0)) {
        error_print("Parameter found in environment: ddr_narrow, forcing narrow ddr interface\n");
        ddr_interface_wide = 0;
    }


    row_bits = read_spd(dimm_config_table[0], 0, DDR2_SPD_NUM_ROW_BITS);
    col_bits = read_spd(dimm_config_table[0], 0, DDR2_SPD_NUM_COL_BITS);
    num_banks = read_spd(dimm_config_table[0], 0, DDR2_SPD_NUM_BANKS);
    num_ranks = 1 + (0x7 & read_spd(dimm_config_table[0], 0, DDR2_SPD_NUM_RANKS));
    dram_width = read_spd(dimm_config_table[0], 0, DDR2_SPD_SDRAM_WIDTH);



    /*
    ** Check that values are within some theoretical limits.
    ** col_bits(min) = row_lsb(min) - bank_bits(max) - bus_bits(max) = 14 - 3 - 4 = 7
    ** col_bits(max) = row_lsb(max) - bank_bits(min) - bus_bits(min) = 18 - 2 - 3 = 13
    */
    if ((col_bits > 13) || (col_bits < 7)) {
        error_print("Unsupported number of Col Bits: %d\n", col_bits);
        ++fatal_error;
    }

    /*
    ** Check that values are within some theoretical limits.
    ** row_bits(min) = pbank_lsb(min) - row_lsb(max) - rank_bits = 26 - 18 - 1 = 7
    ** row_bits(max) = pbank_lsb(max) - row_lsb(min) - rank_bits = 33 - 14 - 1 = 18
    */
    if ((row_bits > 18) || (row_bits < 7)) {
        error_print("Unsupported number of Row Bits: %d\n", row_bits);
        ++fatal_error;
    }

    if (num_banks == 8)
        bank_bits = 3;
    else if (num_banks == 4)
        bank_bits = 2;


    bunk_enable = (num_ranks > 1);

#if defined(__U_BOOT__)
    if (octeon_is_cpuid(OCTEON_CN31XX))
    {
        /*
        ** For EBH3100 pass 1 we have screwy chip select mappings.  As a
        ** result only the first rank of each DIMM is selectable.
        ** Furthermore, the first rank of the second DIMM appears as the
        ** second rank of the first DIMM.  Therefore, when dual-rank DIMMs
        ** are present report half the memory size.  When single-rank
        ** DIMMs are present report them as one dual-rank DIMM.  The ODT
        ** masks will be adjusted appropriately.
        */
        {
            DECLARE_GLOBAL_DATA_PTR;
            if (gd->board_desc.board_type == CVMX_BOARD_TYPE_EBH3100
                && gd->board_desc.rev_major == 1
                && gd->board_desc.rev_minor == 0)
            {
                bunk_enable = (dimm_count > 1) || (num_ranks > 1);
            }
        }
    }
#endif
    if (octeon_is_cpuid(OCTEON_CN30XX))
        column_bits_start = 1;
    else if (octeon_is_cpuid(OCTEON_CN31XX))
        column_bits_start = 2;
    else if (octeon_is_cpuid(OCTEON_CN38XX))
        column_bits_start = 3;
    else if (octeon_is_cpuid(OCTEON_CN58XX))
        column_bits_start = 3;
    else if (octeon_is_cpuid(OCTEON_CN56XX))
        column_bits_start = 2;
    else if (octeon_is_cpuid(OCTEON_CN52XX))
        column_bits_start = 2;
    else if (octeon_is_cpuid(OCTEON_CN50XX))
        column_bits_start = 1;
    else 
        printf("ERROR: Unsupported Octeon model: 0x%x\n", cpu_id);

    row_lsb = column_bits_start + col_bits + bank_bits;
    debug_print("row_lsb = column_bits_start + col_bits + bank_bits = %d\n", row_lsb);

    pbank_lsb = row_lsb + row_bits + bunk_enable;
    debug_print("pbank_lsb = row_lsb + row_bits + bunk_enable = %d\n", pbank_lsb);


    if (octeon_is_cpuid(OCTEON_CN31XX) || octeon_is_cpuid(OCTEON_CN30XX) || octeon_is_cpuid(OCTEON_CN56XX)
        || octeon_is_cpuid(OCTEON_CN50XX) || octeon_is_cpuid(OCTEON_CN52XX)
        )
    {
        /* Interface width is not a function of dimm_count so add it in
           for this calculation. */
        mem_size_mbytes =  dimm_count * ((1ull << (pbank_lsb+ddr_interface_wide)) >> 20);
#if defined(__U_BOOT__)
        {
            DECLARE_GLOBAL_DATA_PTR;
            if (gd->board_desc.board_type == CVMX_BOARD_TYPE_EBH3100
                && gd->board_desc.rev_major == 1
                && gd->board_desc.rev_minor == 0
                && bunk_enable)
            {
                /* Reduce memory size by half on rev 1.0 ebh3100s */
                printf("NOTICE: Memory size reduced by half on 2 rank memory configs.\n");
                mem_size_mbytes /= 2;
            }
        }
#endif
    }
    else
        mem_size_mbytes =  dimm_count * ((1ull << pbank_lsb) >> 20);


    ddr_print("row bits: %d, col bits: %d, banks: %d, ranks: %d, dram width: %d, size: %d MB\n",
              row_bits, col_bits, num_banks, num_ranks, dram_width, mem_size_mbytes);

#if defined(__U_BOOT__)
{
    /* If we are booting from RAM, the DRAM controller is already set up.  Just return the
    ** memory size */
    DECLARE_GLOBAL_DATA_PTR;
    if (gd->flags & GD_FLG_RAM_RESIDENT)
        return mem_size_mbytes;
}
#endif


    spd_cycle_clx   = 0xff & read_spd(dimm_config_table[0], 0, DDR2_SPD_CYCLE_CLX);
    spd_refresh     = 0xff & read_spd(dimm_config_table[0], 0, DDR2_SPD_REFRESH);
    spd_cas_latency = 0xff & read_spd(dimm_config_table[0], 0, DDR2_SPD_CAS_LATENCY);
    spd_cycle_clx1  = 0xff & read_spd(dimm_config_table[0], 0, DDR2_SPD_CYCLE_CLX1);
    spd_cycle_clx2  = 0xff & read_spd(dimm_config_table[0], 0, DDR2_SPD_CYCLE_CLX2);
    spd_trp         = 0xff & read_spd(dimm_config_table[0], 0, DDR2_SPD_TRP);
    spd_trrd        = 0xff & read_spd(dimm_config_table[0], 0, DDR2_SPD_TRRD);
    spd_trcd        = 0xff & read_spd(dimm_config_table[0], 0, DDR2_SPD_TRCD);
    spd_tras        = 0xff & read_spd(dimm_config_table[0], 0, DDR2_SPD_TRAS);
    spd_twr         = 0xff & read_spd(dimm_config_table[0], 0, DDR2_SPD_TWR);
    spd_twtr        = 0xff & read_spd(dimm_config_table[0], 0, DDR2_SPD_TWTR);
    spd_trfc_ext    = 0xff & read_spd(dimm_config_table[0], 0, DDR2_SPD_TRFC_EXT);
    spd_trfc        = 0xff & read_spd(dimm_config_table[0], 0, DDR2_SPD_TRFC);
    spd_ecc         = !!(read_spd(dimm_config_table[0], 0, DDR2_SPD_CONFIG_TYPE) & 2);
    spd_rdimm       = !!(read_spd(dimm_config_table[0], 0, DDR2_SPD_DIMM_TYPE) & 0x11);
    spd_burst_lngth = 0xff & read_spd(dimm_config_table[0], 0, DDR2_SPD_BURST_LENGTH);

    /* Determine board delay based on registered or unbuffered dimms */
    board_delay = spd_rdimm ? ddr_configuration->registered.ddr_board_delay : ddr_configuration->unbuffered.ddr_board_delay;

    if (octeon_is_cpuid(OCTEON_CN3020) || octeon_is_cpuid(OCTEON_CN3005))
        use_ecc = 0;

    /* Temporary to handle mismarked 3020 parts */
    if (board_type == CVMX_BOARD_TYPE_CN3020_EVB_HS5)
        use_ecc = 0;

    use_ecc = use_ecc && spd_ecc;

    if (octeon_is_cpuid(OCTEON_CN31XX))
        ddr_interface_width = ddr_interface_wide ? 64 : 32;
    else if (octeon_is_cpuid(OCTEON_CN30XX))
        ddr_interface_width = ddr_interface_wide ? 32 : 16;
    else if (octeon_is_cpuid(OCTEON_CN58XX))
        ddr_interface_width = ddr_interface_wide ? 128 : 64;
    else if (octeon_is_cpuid(OCTEON_CN56XX))
        ddr_interface_width = ddr_interface_wide ? 64 : 32;
    else if (octeon_is_cpuid(OCTEON_CN52XX))
        ddr_interface_width = ddr_interface_wide ? 64 : 32;
    else if (octeon_is_cpuid(OCTEON_CN50XX))
        ddr_interface_width = ddr_interface_wide ? 32 : 16;
    else
        ddr_interface_width = ddr_interface_wide ? 128 : 64;

    ddr_print("DRAM Interface width: %d bits %s\n", ddr_interface_width, use_ecc ? "+ECC" : "");



    debug_print("spd_cycle_clx   : %#04x\n", spd_cycle_clx);
    debug_print("spd_refresh     : %#04x\n", spd_refresh);
    debug_print("spd_cas_latency : %#04x\n", spd_cas_latency);
    debug_print("spd_cycle_clx1  : %#04x\n", spd_cycle_clx1);
    debug_print("spd_cycle_clx2  : %#04x\n", spd_cycle_clx2);
    debug_print("spd_trp         : %#04x\n", spd_trp);
    debug_print("spd_trrd        : %#04x\n", spd_trrd);
    debug_print("spd_trcd        : %#04x\n", spd_trcd);
    debug_print("spd_tras        : %#04x\n", spd_tras);
    debug_print("spd_twr         : %#04x\n", spd_twr);
    debug_print("spd_twtr        : %#04x\n", spd_twtr);
    debug_print("spd_trfc_ext    : %#04x\n", spd_trfc_ext);
    debug_print("spd_trfc        : %#04x\n", spd_trfc);

    cycle_clx[0] = lookup_cycle_time_psec (spd_cycle_clx);
    cycle_clx[1] =  lookup_cycle_time_psec (spd_cycle_clx1);
    cycle_clx[2] =  lookup_cycle_time_psec (spd_cycle_clx2);

    cas_latency     = select_cas_latency (tclk_psecs, spd_cas_latency, cycle_clx);
    refresh         = lookup_refresh_rate_nsec (spd_refresh);
    trp             = lookup_delay_psec (spd_trp);
    trrd            = lookup_delay_psec (spd_trrd);
    trcd            = lookup_delay_psec (spd_trcd);
    tras            = spd_tras * 1000;
    twr             = lookup_delay_psec (spd_twr);
    twtr            = lookup_delay_psec (spd_twtr);
    trfc            = lookup_rfc_psec (spd_trfc, spd_trfc_ext);

    ddr_print("DDR Clock Rate (tCLK)                         : %6d ps\n", tclk_psecs);
    ddr_print("Core Clock Rate (eCLK)                        : %6d ps\n", eclk_psecs);
    ddr_print("CAS Latency                                   : %6d\n",    cas_latency);
    ddr_print("Refresh Rate (tREFI)                          : %6d ns\n", refresh);
    ddr_print("Minimum Row Precharge Time (tRP)              : %6d ps\n", trp);
    ddr_print("Minimum Row Active to Row Active delay (tRRD) : %6d ps\n", trrd);
    ddr_print("Minimum RAS to CAS delay (tRCD)               : %6d ps\n", trcd);
    ddr_print("Minimum Active to Precharge Time (tRAS)       : %6d ps\n", tras);
    ddr_print("Write Recovery Time (tWR)                     : %6d ps\n", twr);
    ddr_print("Internal write to read command delay (tWTR)   : %6d ps\n", twtr);
    ddr_print("Device Min Auto-refresh Active/Command (tRFC) : %6d ps\n", trfc);


    if ((num_banks != 4) && (num_banks != 8))
    {
        error_print("Unsupported number of banks %d. Must be 4 or 8.\n", num_banks);
        ++fatal_error;
    }

    if ((num_ranks < 1) || (num_ranks > 2))
    {
        error_print("Unsupported number of ranks: %d\n", num_ranks);
        ++fatal_error;
    }

    if ((dram_width != 8) && (dram_width != 16))
    {
        error_print("Unsupported SDRAM Width, %d.  Must be 8 or 16.\n", dram_width);
        ++fatal_error;
    }





    if ((s = getenv_wrapper("ddr_board_delay")) != NULL) {
        unsigned env_delay;
        env_delay = (unsigned) simple_strtoul(s, NULL, 0);
        if ((board_delay) && (board_delay != env_delay)) {
            error_print("Overriding internal board delay (%d ps).\n", board_delay);
            error_print("Parameter found in environment. ddr_board_delay = %d ps\n",
                        env_delay);
            board_delay = env_delay;
        }
    }

    ddr_print("Board delay                                   : %6d ps\n", board_delay);
    if (board_delay == 0 && (!octeon_is_cpuid(OCTEON_CN56XX)) && (!octeon_is_cpuid(OCTEON_CN52XX))) {
        /* Board delay of 0 uses read leveling for cn52xx/cn56xx */
        error_print("Error!!!  Board delay was not specified!!!\n");
        ++fatal_error;
    }

    /*
    ** Bail out here if things are not copasetic.
    */
    if (fatal_error)
        return(-1);


    if ((!octeon_is_cpuid(OCTEON_CN56XX)) && (!octeon_is_cpuid(OCTEON_CN52XX))) {
        delay_params = compute_delay_params(board_delay, tclk_psecs, 0);

        ctl_tskw    = (delay_params>>0) & 0xff;
        ctl_silo_hc = (delay_params>>8) & 1;
        ctl_silo_qc = (delay_params>>16) & 1;


        if (ddr_verbose())
        {
            if ((s = getenv_wrapper("ddr_tskw")) != NULL) {
                ctl_tskw    = simple_strtoul(s, NULL, 0);
                error_print("Parameter found in environment. ddr_tskw = %d\n", ctl_tskw);
            }

            if ((s = getenv_wrapper("ddr_silo_hc")) != NULL) {
                ctl_silo_hc    = simple_strtoul(s, NULL, 0);
                error_print("Parameter found in environment. ddr_silo_hc = %d\n", ctl_silo_hc);
            }

            if ((s = getenv_wrapper("ddr_silo_qc")) != NULL) {
                ctl_silo_qc    = simple_strtoul(s, NULL, 0);
                error_print("Parameter found in environment. ddr_silo_qc = %d\n", ctl_silo_qc);
            }
        }
    }


    ctl_fprch2  = 1;

    odt_idx = min(dimm_count - 1, 3);
    odt_config = bunk_enable ? odt_2rank_config : odt_1rank_config;

    ctl_qs_dic		= odt_config[odt_idx].qs_dic;
    /* Note: We don't use OCTEON_IS_MODEL here since we must use the cpu_id variable passed us - we
    ** may be running on a PCI host and not be able to ready the CPU ID directly */
    if ((octeon_is_cpuid(OCTEON_CN31XX) || (octeon_is_cpuid(OCTEON_CN30XX)))
        && (tclk_psecs <= 3750)
        && (num_ranks > 1)) {
        /* O2P requires 50 ohm ODT for dual-rank 533 MHz and higher  */
        ctl_qs_dic  = 3;
    }

    ctl_dic		= odt_config[odt_idx].dic;
    ctl_odt_ena		= odt_config[odt_idx].odt_ena;
    ctl_odt_mask	= odt_config[odt_idx].odt_mask;
    if (octeon_is_cpuid(OCTEON_CN31XX) || octeon_is_cpuid(OCTEON_CN30XX) || octeon_is_cpuid(OCTEON_CN56XX)
        || octeon_is_cpuid(OCTEON_CN50XX) || octeon_is_cpuid(OCTEON_CN52XX)
        )
        ctl_odt_mask1	= odt_config[odt_idx].odt_mask1.u64;
    ctl_rodt_ctl	= odt_config[odt_idx].rodt_ctl;

    if ((s = getenv_wrapper("ddr_dic")) != NULL) {
        ctl_dic    = simple_strtoul(s, NULL, 0);
        error_print("Parameter found in environment. ddr_dic = %d\n", ctl_dic);
    }

    if ((s = getenv_wrapper("ddr_qs_dic")) != NULL) {
        ctl_qs_dic    = simple_strtoul(s, NULL, 0);
        error_print("Parameter found in environment. ddr_qs_dic = %d\n", ctl_qs_dic);
    }

    if ((s = getenv_wrapper("ddr_odt_ena")) != NULL) {
        ctl_odt_ena    = simple_strtoul(s, NULL, 0);
        error_print("Parameter found in environment. ddr_odt_ena = %d\n", ctl_odt_ena);
    }

    if ((s = getenv_wrapper("ddr_odt_mask")) != NULL) {
        ctl_odt_mask    = simple_strtoul(s, NULL, 0);
        error_print("Parameter found in environment. ddr_odt_mask = 0x%x\n", ctl_odt_mask);
    }

    if (octeon_is_cpuid(OCTEON_CN31XX) || octeon_is_cpuid(OCTEON_CN30XX) || octeon_is_cpuid(OCTEON_CN56XX)
        || octeon_is_cpuid(OCTEON_CN50XX) || octeon_is_cpuid(OCTEON_CN52XX))
    {
        if ((s = getenv_wrapper("ddr_odt_mask1")) != NULL) {
            ctl_odt_mask1    = simple_strtoul(s, NULL, 0);
            error_print("Parameter found in environment. ddr_odt_mask1 = 0x%x\n", ctl_odt_mask1);
        }
    }

        if ((s = getenv_wrapper("ddr_rodt_ctl")) != NULL) {
            ctl_rodt_ctl    = simple_strtoul(s, NULL, 0);
            error_print("Parameter found in environment. ddr_rodt_ctl = 0x%x\n", ctl_rodt_ctl);
        }

    /* Work around RODT disable errata.  In order to disable remote read ODT, both odt_ena and rodt_ctl must be 0 */
    if ((ctl_odt_ena == 0) || (ctl_rodt_ctl == 0)) {
	ctl_odt_ena = 0;
	ctl_rodt_ctl = 0;
    }


    if (board_type == CVMX_BOARD_TYPE_EBT3000 && board_rev_maj == 1)
    {
        /* Hack to support old rev 1 ebt3000 boards.  These boards
        ** are not supported by the pci utils */
        ctl_fprch2  = 0;
        ctl_qs_dic  = 2;  /* 0 may also work */
    }
    /* ----------------------------------------------------------------------------- */

    /*
     * DRAM Controller Initialization
     * The reference-clock inputs to the LMC (DDR2_REF_CLK_*) should be stable
     * when DCOK asserts (refer to Section 26.3). DDR_CK_* should be stable from that
     * point, which is more than 200 us before software can bring up the main memory
     * DRAM interface. The generated DDR_CK_* frequency is four times the
     * DDR2_REF_CLK_* frequency.
     * To initialize the main memory and controller, software must perform the following
     * steps in this order:
     *
     * 1. Write LMC_CTL with [DRESET] = 1, [PLL_BYPASS] = user_value, and
     * [PLL_DIV2] = user_value.
     */

    lmc_ctl.u64               = 0;

    lmc_ctl.s.dic               = ctl_dic;
    lmc_ctl.s.qs_dic            = ctl_qs_dic;
    if ((!octeon_is_cpuid(OCTEON_CN56XX)) && (!octeon_is_cpuid(OCTEON_CN52XX))) {
        lmc_ctl.s.tskw              = ctl_tskw;
        lmc_ctl.s.sil_lat           = safe_ddr_flag ? 2 : 1;
    }
    lmc_ctl.s.bprch             = ctl_silo_hc & ctl_silo_qc;
    lmc_ctl.s.fprch2            = ctl_fprch2;
    if (octeon_is_cpuid(OCTEON_CN31XX))
        lmc_ctl.cn31xx.mode32b           = ! ddr_interface_wide; /* 32-bit == Not Wide */
    else if (octeon_is_cpuid(OCTEON_CN30XX))
        lmc_ctl.cn30xx.mode32b           = ddr_interface_wide; /* 32-bit == Wide */
    else if (octeon_is_cpuid(OCTEON_CN56XX))
        lmc_ctl.cn56xx.mode32b           = ! ddr_interface_wide; /* 32-bit == Not Wide */
    else if (octeon_is_cpuid(OCTEON_CN52XX))
        lmc_ctl.cn52xx.mode32b           = ! ddr_interface_wide; /* 32-bit == Not Wide */
    else if (octeon_is_cpuid(OCTEON_CN50XX))
        lmc_ctl.cn50xx.mode32b           = ddr_interface_wide; /* 32-bit == Wide */
    else
        lmc_ctl.cn38xx.mode128b          = ddr_interface_wide; /* 128-bit == Wide */
    lmc_ctl.s.inorder_mrf       = 0;
    lmc_ctl.s.inorder_mwf       = 0;
    lmc_ctl.s.r2r_slot          = safe_ddr_flag ? 1 : 0;
    lmc_ctl.s.rdimm_ena         = spd_rdimm;
    lmc_ctl.s.max_write_batch   = 0xf;
    lmc_ctl.s.xor_bank          = 1;
    lmc_ctl.s.ddr__pctl         = 0;
    lmc_ctl.s.ddr__nctl         = 0;
    if (octeon_is_cpuid(OCTEON_CN31XX) || octeon_is_cpuid(OCTEON_CN30XX))
    {
#if CONFIG_OCTEON_HIKARI
        lmc_ctl.cn31xx.pll_div2          = 1;
#endif
        lmc_ctl.cn31xx.dreset            = 1;
    }



    cvmx_write_csr(CVMX_LMCX_CTL(ddr_interface_num), lmc_ctl.u64);



    /* 2. Read L2D_BST0 and wait for the result. */
    cvmx_read_csr(CVMX_L2D_BST0); /* Read CVMX_L2D_BST0 */

    /* 3. Wait 10 us (LMC_CTL[PLL_BYPASS] and LMC_CTL[PLL_DIV2] must not
     * transition after this) */
    octeon_delay_cycles(6000);  /* Wait 10 us */


    /* ----------------------------------------------------------------------------- */

    ctl_twr = divide_roundup(twr, tclk_psecs) - 1;
    ctl_twr = min(ctl_twr, 5);
    ctl_twr = max(ctl_twr, 1);

    if ((octeon_is_cpuid(OCTEON_CN58XX)) || (octeon_is_cpuid(OCTEON_CN56XX))
        || (octeon_is_cpuid(OCTEON_CN50XX)) || (octeon_is_cpuid(OCTEON_CN52XX))
        ) {
        int ctl_ddr_eof_ratio;
        ctl_ddr_eof_ratio = tclk_psecs*10/eclk_psecs;
            ctl_ddr_eof = 3;
        if (ctl_ddr_eof_ratio >= 15)
            ctl_ddr_eof = 2;
        if (ctl_ddr_eof_ratio >= 20)
            ctl_ddr_eof = 1;
        if (ctl_ddr_eof_ratio >= 30)
            ctl_ddr_eof = 0;
    }
    else {
        ctl_ddr_eof = min( 3, max( 1, divide_roundup(tclk_psecs, eclk_psecs)));
    }

    ddr2_ctl.u64 = 0;
    ddr2_ctl.s.ddr2       = 1;
    if (octeon_is_cpuid(OCTEON_CN38XX))
        ddr2_ctl.cn38xx.rdqs       = 0;
    if (octeon_is_cpuid(OCTEON_CN58XX))
        ddr2_ctl.cn58xx.rdqs       = 0;
    if (octeon_is_cpuid(OCTEON_CN56XX))
        ddr2_ctl.cn56xx.rdqs       = 0;
    if (octeon_is_cpuid(OCTEON_CN52XX))
        ddr2_ctl.cn52xx.rdqs       = 0;
    if (octeon_is_cpuid(OCTEON_CN50XX))
        ddr2_ctl.cn50xx.rdqs       = 0;
    if ((!octeon_is_cpuid(OCTEON_CN56XX)) && (!octeon_is_cpuid(OCTEON_CN52XX))) {
        ddr2_ctl.s.dll90_byp  = 0;
        ddr2_ctl.s.dll90_vlu  = 0;
        ddr2_ctl.s.qdll_ena   = 0;
    }
    ddr2_ctl.s.crip_mode  = 0;

    if (cpu_id != OCTEON_CN38XX_PASS1) { /* Octeon Pass 1 chip? */
        switch (gd->board_desc.board_type) {
#ifdef CONFIG_JSRXNLE
        CASE_ALL_SRX220_MODELS 
            ddr2_ctl.s.ddr2t = 0;
            break;
#endif
        default:
            ddr2_ctl.s.ddr2t  = safe_ddr_flag ? 1 : (spd_rdimm == 0);
            break;
        }
    }

    if (ddr_verbose() && ((s = getenv("ddr_2t")) != NULL)) {
        ddr2_ctl.s.ddr2t  = simple_strtoul(s, NULL, 0);
        error_print("Parameter found in environment. ddr_2t = %d\n", ddr2_ctl.s.ddr2t);
    }


    ddr2_ctl.s.ddr_eof    = ctl_ddr_eof;
    if ((!octeon_is_cpuid(OCTEON_CN56XX)) && (!octeon_is_cpuid(OCTEON_CN52XX))) {
        ddr2_ctl.s.silo_hc    = ctl_silo_hc;
    }
    ddr2_ctl.s.twr        = ctl_twr;
    ddr2_ctl.s.bwcnt      = 0;
    ddr2_ctl.s.pocas      = 0;
    ddr2_ctl.s.addlat     = 0;
    if (cpu_id == OCTEON_CN38XX_PASS2) {
        ddr2_ctl.s.odt_ena    = 0; /* Signal aliased to cripple_mode. */
    }
    else {
        ddr2_ctl.s.odt_ena    = (ctl_odt_ena != 0);
    }
    ddr2_ctl.s.burst8     = (spd_burst_lngth & 8) ? 1 : 0;


    if (ddr_verbose() && (s = getenv("ddr_burst8")) != NULL) {
        ddr2_ctl.s.burst8  = simple_strtoul(s, NULL, 0);
        error_print("Parameter found in environment. ddr_burst8 = %d\n", ddr2_ctl.s.burst8);
    }


    ddr2_ctl.s.bank8      = (num_banks == 8);

    if (num_banks == 8) {
        int dram_page_size;
        int tfaw;

        /*
        ** Page size is the number of bytes of data delivered from the
        ** array to the internal sense amplifiers when an ACTIVE command
        ** is registered. Page size is per bank, calculated as follows:
        ** 
        ** page size = 2 COLBITS x ORG ÷ 8
        ** 
        ** where:
        ** COLBITS = the number of column address bits
        ** ORG = the number of I/O (DQ) bits
        */

        dram_page_size = (1 << col_bits) * dram_width / 8;

        /*
        **                       tFAW ns (JESD79-2C)
        **              -----------------------------------
        **   page size  DDR2-800 DDR2-667 DDR2-533 DDR2-400
        **   ---------  -------- -------- -------- --------
        **      1KB       35       37.5     37.5     37.5
        **      2KB       45       50       50       50
        */

        if (dram_page_size < 2048)
            tfaw = (tclk_psecs > 2500) ? 37500 : 35000; /* 1KB Page Size */
        else
            tfaw = (tclk_psecs > 2500) ? 50000 : 45000; /* 2KB Page Size */

        /*
        **  tFAW - Cycles = RNDUP[tFAW(ns)/tcyc(ns)] - 1
        **  Four Access Window time. Only relevent to 8-bank parts.
        */

        ddr2_ctl.s.tfaw = max(0, divide_roundup(tfaw, tclk_psecs) - 1);

        ddr_print("Four Activate Window/Page Size (Tfaw=%5d ps): %d/%dK\n",
                  tfaw, ddr2_ctl.s.tfaw, dram_page_size/1024);
    }


    /* 4. Write LMC_DDR2_CTL[QDLL_ENA] = 1. */ /* Is it OK to write 0 first? */
    if ((!octeon_is_cpuid(OCTEON_CN56XX)) && (!octeon_is_cpuid(OCTEON_CN52XX))) {
        cvmx_read_csr(CVMX_LMCX_DDR2_CTL(ddr_interface_num));
        ddr2_ctl.s.qdll_ena   = 1;
        cvmx_write_csr(CVMX_LMCX_DDR2_CTL(ddr_interface_num), ddr2_ctl.u64);
        cvmx_read_csr(CVMX_LMCX_DDR2_CTL(ddr_interface_num));

        octeon_delay_cycles(2000);   // must be 200 dclocks

        /* 5. Read L2D_BST0 and wait for the result. */
        cvmx_read_csr(CVMX_L2D_BST0); /* Read CVMX_L2D_BST0 */

        /* 6. Wait 10 us (LMC_DDR2_CTL[QDLL_ENA] must not transition after this) */
        octeon_delay_cycles(2000);  /* Wait 10 us */
    }


    /* ----------------------------------------------------------------------------- */

    /*
     * 7. Write LMC_CTL[DRESET] = 0 (at this point, the DCLK is running and the
     * memory controller is out of reset)
     */

    if (octeon_is_cpuid(OCTEON_CN31XX) || octeon_is_cpuid(OCTEON_CN30XX))
    {
        lmc_ctl.cn31xx.dreset            = 0;
        cvmx_write_csr(CVMX_LMCX_CTL(ddr_interface_num), lmc_ctl.u64);
        octeon_delay_cycles(2000);   // must be 200 dclocks
    }

    /* ----------------------------------------------------------------------------- */
    /*
     * Next, boot software must re-initialize the LMC_MEM_CFG1, LMC_CTL, and
     * LMC_DDR2_CTL CSRs, and also the LMC_WODT_CTL and LMC_RODT_CTL
     * CSRs. Refer to Sections 2.3.4, 2.3.5, and 2.3.7 regarding these CSRs (and
     * LMC_MEM_CFG0).
     */

    /* Configure ODT */
    if (octeon_is_cpuid(OCTEON_CN31XX) || octeon_is_cpuid(OCTEON_CN30XX) || octeon_is_cpuid(OCTEON_CN56XX)
        || octeon_is_cpuid(OCTEON_CN50XX) || octeon_is_cpuid(OCTEON_CN52XX)
        )
    {
        cvmx_write_csr(CVMX_LMCX_WODT_CTL0(ddr_interface_num), ctl_odt_mask);
    }
    else
        cvmx_write_csr(CVMX_LMC_WODT_CTL, ctl_odt_mask);

    if (octeon_is_cpuid(OCTEON_CN31XX) || octeon_is_cpuid(OCTEON_CN30XX) || octeon_is_cpuid(OCTEON_CN56XX)
        || octeon_is_cpuid(OCTEON_CN52XX)
        )
    {
        cvmx_write_csr(CVMX_LMCX_WODT_CTL1(ddr_interface_num), ctl_odt_mask1);
    }

    cvmx_write_csr(CVMX_LMCX_RODT_CTL(ddr_interface_num), ctl_rodt_ctl);

    cvmx_write_csr(CVMX_LMCX_DDR2_CTL(ddr_interface_num), ddr2_ctl.u64);

    /* ----------------------------------------------------------------------------- */

    if ((octeon_is_cpuid(OCTEON_CN30XX))
        || (cpu_id == OCTEON_CN38XX_PASS3)
        || (octeon_is_cpuid(OCTEON_CN58XX))
        || (octeon_is_cpuid(OCTEON_CN56XX))
        || (octeon_is_cpuid(OCTEON_CN52XX))
        || (octeon_is_cpuid(OCTEON_CN50XX)))
    {
        lmc_delay.u64 = 0;

        if (spd_rdimm == 1) {
            lmc_delay.s.clk  = ddr_configuration->registered.lmc_delay_clk;
            lmc_delay.s.cmd  = ddr_configuration->registered.lmc_delay_cmd;
            lmc_delay.s.dq   = ddr_configuration->registered.lmc_delay_dq;
        } else {
            lmc_delay.s.clk  = ddr_configuration->unbuffered.lmc_delay_clk;
            lmc_delay.s.cmd  = ddr_configuration->unbuffered.lmc_delay_cmd;
            lmc_delay.s.dq   = ddr_configuration->unbuffered.lmc_delay_dq;
        }

        if (ddr_verbose())
        {
            if ((s = getenv_wrapper("ddr_delay_clk")) != NULL) {
                lmc_delay.s.clk  = simple_strtoul(s, NULL, 0);
                error_print("Parameter found in environment. ddr_delay_clk = %d\n", lmc_delay.s.clk);
            }

            if ((s = getenv_wrapper("ddr_delay_cmd")) != NULL) {
                lmc_delay.s.cmd  = simple_strtoul(s, NULL, 0);
                error_print("Parameter found in environment. ddr_delay_cmd = %d\n", lmc_delay.s.cmd);
            }

            if ((s = getenv_wrapper("ddr_delay_dq")) != NULL) {
                lmc_delay.s.dq  = simple_strtoul(s, NULL, 0);
                error_print("Parameter found in environment. ddr_delay_dq = %d\n", lmc_delay.s.dq);
            }
        }

        cvmx_write_csr(CVMX_LMCX_DELAY_CFG(ddr_interface_num), lmc_delay.u64);

        ddr_print("delay_clk                                     : %6d\n", lmc_delay.s.clk);
        ddr_print("delay_cmd                                     : %6d\n", lmc_delay.s.cmd);
        ddr_print("delay_dq                                      : %6d\n", lmc_delay.s.dq);
    }

    /* ----------------------------------------------------------------------------- */

    if (ddr_verbose() && (octeon_is_cpuid(OCTEON_CN58XX) || octeon_is_cpuid(OCTEON_CN56XX)
                          || octeon_is_cpuid(OCTEON_CN50XX) || octeon_is_cpuid(OCTEON_CN52XX)
                          ))
    {
        cvmx_lmc_ctl1_t lmc_ctl1;
        cvmx_lmc_pll_ctl_t pll_ctl;
        uint64_t clkf, clkr, pll_MHz, calculated_ddr_hertz;
        int _en = 0;

        lmc_ctl1.u64 = cvmx_read_csr(CVMX_LMCX_CTL1(ddr_interface_num));
        ddr_print("sil_mode                                      : %6d\n", lmc_ctl1.s.sil_mode);

        pll_ctl.u64 = cvmx_read_csr(CVMX_LMC_PLL_CTL);  /* Only one LMC_PLL_CTL */
        clkf = pll_ctl.s.clkf;
        clkr = pll_ctl.s.clkr;

        ddr_print("DDR Fixed Reference Clock Hertz               : %8d\n", ddr_ref_hertz);
        ddr_print("clkf                                          : %6d\n", clkf);
        ddr_print("clkr                                          : %6d\n", clkr);

        if (pll_ctl.s.en2 == 1) {
            ddr_print("EN2                                           : %6d\n", 1); _en = 2;
        }
        if (pll_ctl.s.en4 == 1) {
            ddr_print("EN4                                           : %6d\n", 1); _en = 4;
        }
        if (pll_ctl.s.en6 == 1) {
            ddr_print("EN6                                           : %6d\n", 1); _en = 6;
        }
        if (pll_ctl.s.en8 == 1) {
            ddr_print("EN8                                           : %6d\n", 1); _en = 8;
        }
        if (pll_ctl.s.en12 == 1) {
            ddr_print("EN12                                          : %6d\n", 1); _en = 12;
        }
        if (pll_ctl.s.en16 == 1) {
            ddr_print("EN16                                          : %6d\n", 1); _en = 16;
        }

        pll_MHz = ddr_ref_hertz * (clkf+1) / (clkr+1) / 1000000;

        ddr_print("LMC PLL Frequency                             : %6d MHz\n", pll_MHz);

        calculated_ddr_hertz = ddr_ref_hertz * (clkf + 1) / ((clkr + 1) * _en);
        ddr_print("Calculated DClk Frequency                     : %8d Hz\n", calculated_ddr_hertz);
    }

    /* ----------------------------------------------------------------------------- */

    cfg1_tras =  divide_roundup(tras, tclk_psecs);

    cfg1_trcd = divide_roundup(trcd, tclk_psecs);

    cfg1_twtr = divide_roundup(twtr, tclk_psecs);

    /* Plus 1 for 8-bank parts */
    cfg1_trp = divide_roundup(trp, tclk_psecs) + (num_banks == 8);

    cfg1_trfc =  divide_roundup(trfc, (4*tclk_psecs));

    cfg1_tmrd = 3; /* Always at least 2. 3 supports higher speeds */

    cfg1_trrd = divide_roundup(trrd, tclk_psecs);



    mem_cfg1.u64 = 0;
    mem_cfg1.s.tras     = cfg1_tras;
    /* In 2T mode, make this register TRCD-1, not going below 2. */
    mem_cfg1.s.trcd     = max(2, (ddr2_ctl.s.ddr2t ? (cfg1_trcd - 1) : cfg1_trcd));
    mem_cfg1.s.twtr     = cfg1_twtr;
    mem_cfg1.s.trp      = cfg1_trp;
    mem_cfg1.s.trfc     = cfg1_trfc;
    mem_cfg1.s.tmrd     = cfg1_tmrd;
    mem_cfg1.s.caslat   = cas_latency;
    mem_cfg1.s.trrd     = cfg1_trrd;



    cvmx_write_csr(CVMX_LMCX_MEM_CFG1(ddr_interface_num), mem_cfg1.u64);

    /* ----------------------------------------------------------------------------- */


    {
        cvmx_lmc_comp_ctl_t lmc_comp_ctl;

        lmc_comp_ctl.u64 = 0;
        lmc_comp_ctl.s.pctl_clk = 0x0;
        lmc_comp_ctl.s.pctl_csr = 0xf;
        lmc_comp_ctl.s.nctl_dat = 0x0;
        lmc_comp_ctl.s.nctl_clk = 0x0;
        lmc_comp_ctl.s.nctl_csr = 0xf;


#if 0
        if (octeon_is_cpuid(OCTEON_CN52XX)) {
            cvmx_lmcx_pll_status_t tmp_pll_status;

            /*
             * Override compensation circuit with hardcoded settings.
             */

            tmp_pll_status.u64 = cvmx_read_csr(CVMX_LMCX_PLL_STATUS(ddr_interface_num));

#if 0
#define CN52XX_P1_NCTL_DAT_STRENGTH       11                      /* 15 ohm Output Impedance (Strength) setting */
#define CN52XX_P1_PCTL_DAT_STRENGTH       31                      /* 15 ohm Output Impedance (Strength) setting */
#endif
#if 1
#define CN52XX_P1_NCTL_DAT_STRENGTH        3                      /* 50 ohm Output Impedance (Strength) setting */
#define CN52XX_P1_PCTL_DAT_STRENGTH       10                      /* 50 ohm Output Impedance (Strength) setting */
#endif
#if 0
#define CN52XX_P1_NCTL_DAT_STRENGTH        2                      /* 65 ohm Output Impedance (Strength) setting */
#define CN52XX_P1_PCTL_DAT_STRENGTH        7                      /* 65 ohm Output Impedance (Strength) setting */
#endif


            lmc_comp_ctl.cn52xx.pctl_dat =  (((CN52XX_P1_PCTL_DAT_STRENGTH) + 32) - tmp_pll_status.cn52xx.ddr__pctl) & 0x1f;
            lmc_comp_ctl.cn52xx.nctl_dat =  (((CN52XX_P1_NCTL_DAT_STRENGTH) + 32) - tmp_pll_status.cn52xx.ddr__nctl) & 0xf;
            ddr_print("DDR PMOS dat                                  : %6d\n", lmc_comp_ctl.u64 & 0x1full);
            ddr_print("DDR NMOS dat                                  : %6d\n", lmc_comp_ctl.cn52xx.nctl_dat);
        }
#endif

        if (board_type == CVMX_BOARD_TYPE_EBT3000 && board_rev_maj == 1)
        {
            /* Hack to support old rev 1 ebt3000 boards.  These boards
            ** are not supported by the pci utils */
            cvmx_lmc_ctl_t tmp_ctl;

#define NCTL_CMD_STRENGTH       14                      /* Strength setting */
#define PCTL_CMD_STRENGTH       14                      /* Strength setting */
#define NCTL_CLK_STRENGTH       2                       /* Strength setting */
#define PCTL_CLK_STRENGTH       1                       /* Strength setting */

            tmp_ctl.u64 = cvmx_read_csr(CVMX_LMCX_CTL(ddr_interface_num));

            lmc_comp_ctl.cn38xx.pctl_cmd =  (((PCTL_CMD_STRENGTH) + 16) - tmp_ctl.s.ddr__pctl) & 0xf;
            lmc_comp_ctl.s.nctl_cmd =  (((NCTL_CMD_STRENGTH) + 16) - tmp_ctl.s.ddr__nctl) & 0xf;

            if (ddr_hertz > 250000000)
            {
                lmc_comp_ctl.s.pctl_clk =  (((PCTL_CLK_STRENGTH) + 16) - tmp_ctl.s.ddr__pctl) & 0xf;
                lmc_comp_ctl.s.nctl_clk =  (((NCTL_CLK_STRENGTH) + 16) - tmp_ctl.s.ddr__nctl) & 0xf;
            }
        }


        if (cpu_id == OCTEON_CN56XX_PASS1) {
            cvmx_lmcx_pll_status_t tmp_pll_status;

            /*
             * CN56XX Compensation circuit is not working on pass 1.
             * Override with hardcoded settings.
             */

            tmp_pll_status.u64 = cvmx_read_csr(CVMX_LMCX_PLL_STATUS(ddr_interface_num));

#define CN56XX_P1_NCTL_DAT_STRENGTH        6                      /* 25 ohm Output Impedance (Strength) setting */
#define CN56XX_P1_PCTL_DAT_STRENGTH       19                      /* 25 ohm Output Impedance (Strength) setting */

            lmc_comp_ctl.cn56xx.pctl_dat =  (((CN56XX_P1_PCTL_DAT_STRENGTH) + 32) - tmp_pll_status.cn56xx.ddr__pctl) & 0x1f;
            lmc_comp_ctl.cn56xx.nctl_dat =  (((CN56XX_P1_NCTL_DAT_STRENGTH) + 32) - tmp_pll_status.cn56xx.ddr__nctl) & 0xf;
            ddr_print("DDR PMOS dat                                  : %6d\n", lmc_comp_ctl.u64 & 0x1full);
            ddr_print("DDR NMOS dat                                  : %6d\n", lmc_comp_ctl.cn56xx.nctl_dat);
        }

        cvmx_write_csr(CVMX_LMCX_COMP_CTL(ddr_interface_num), lmc_comp_ctl.u64);
    }

    /* ----------------------------------------------------------------------------- */

    /*
     * Finally, software must write the LMC_MEM_CFG0 register with
     * LMC_MEM_CFG0[INIT_START] = 1. At that point, CN31XX hardware initiates
     * the standard DDR2 initialization sequence shown in Figure 2.
     */

    ref_int = (refresh*1000) / (tclk_psecs*512);

    cfg0_tcl = cas_latency;

    mem_cfg0.u64 = 0;


    mem_cfg0.s.init_start   = 0;
    cvmx_write_csr(CVMX_LMCX_MEM_CFG0(ddr_interface_num), mem_cfg0.u64); /* Reset one shot */

    mem_cfg0.s.ecc_ena      = use_ecc && spd_ecc;
    mem_cfg0.s.row_lsb      = encode_row_lsb(cpu_id, row_lsb, ddr_interface_wide);
    mem_cfg0.s.bunk_ena     = bunk_enable;

    if ((!octeon_is_cpuid(OCTEON_CN56XX)) && (!octeon_is_cpuid(OCTEON_CN52XX))) {
        mem_cfg0.s.silo_qc  = ctl_silo_qc;
    }

    mem_cfg0.s.pbank_lsb    = encode_pbank_lsb(cpu_id, pbank_lsb, ddr_interface_wide);
    mem_cfg0.s.ref_int      = ref_int;
    mem_cfg0.s.tcl          = 0; /* Has no effect on the controller's behavior */
    mem_cfg0.s.intr_sec_ena = 0;
    mem_cfg0.s.intr_ded_ena = 0;
    mem_cfg0.s.sec_err      = ~0;
    mem_cfg0.s.ded_err      = ~0;
    mem_cfg0.s.reset        = 0;



    ddr_print("bunk_enable                                   : %6d\n", mem_cfg0.s.bunk_ena);
    ddr_print("burst8                                        : %6d\n", ddr2_ctl.s.burst8);
    ddr_print("ddr2t                                         : %6d\n", ddr2_ctl.s.ddr2t);

    if ((!octeon_is_cpuid(OCTEON_CN56XX)) && (!octeon_is_cpuid(OCTEON_CN52XX))) {
        ddr_print("tskw                                          : %6d\n", lmc_ctl.s.tskw);
        ddr_print("silo_hc                                       : %6d\n", ddr2_ctl.s.silo_hc);

        ddr_print("silo_qc                                       : %6d\n", mem_cfg0.s.silo_qc);

        ddr_print("sil_lat                                       : %6d\n", lmc_ctl.s.sil_lat);
    }

    ddr_print("r2r_slot                                      : %6d\n", lmc_ctl.s.r2r_slot);

    ddr_print("odt_ena                                       : %6d\n", ddr2_ctl.s.odt_ena);
    ddr_print("qs_dic                                        : %6d\n", lmc_ctl.s.qs_dic);
    ddr_print("dic                                           : %6d\n", lmc_ctl.s.dic);
    ddr_print("ctl_odt_mask                                  : %08x\n", ctl_odt_mask);
    if (octeon_is_cpuid(OCTEON_CN31XX) || octeon_is_cpuid(OCTEON_CN30XX) || octeon_is_cpuid(OCTEON_CN56XX)
        || octeon_is_cpuid(OCTEON_CN50XX) || octeon_is_cpuid(OCTEON_CN52XX))
        ddr_print("ctl_odt_mask1                                 : %08x\n", ctl_odt_mask1);
    ddr_print("ctl_rodt_ctl                                  : %08x\n", ctl_rodt_ctl);

    if (octeon_is_cpuid(OCTEON_CN58XX) || octeon_is_cpuid(OCTEON_CN56XX)
        || octeon_is_cpuid(OCTEON_CN50XX) || octeon_is_cpuid(OCTEON_CN52XX)
        ) {
        cvmx_lmc_ctl_t tmp_ctl;
        lmc_rodt_comp_ctl.u64 = 0;
        uint32_t rodt_ena = odt_config[odt_idx].odt_ena;

        if ((s = getenv("ddr_rodt_ena")) != NULL) {
            rodt_ena  = simple_strtoul(s, NULL, 0);
            error_print("Parameter found in environment. ddr_rodt_ena = %d\n", rodt_ena);
        }

        if (rodt_ena == 1) { /* Weak Read ODT */
            lmc_rodt_comp_ctl.s.enable = 1;
            lmc_rodt_comp_ctl.s.pctl = 3;
            lmc_rodt_comp_ctl.s.nctl = 1;
        }

        if (rodt_ena == 2) { /* Strong Read ODT */
            lmc_rodt_comp_ctl.s.enable = 1;
            lmc_rodt_comp_ctl.s.pctl = 7;
            lmc_rodt_comp_ctl.s.nctl = 2;
        }

        if (rodt_ena == 3) { /* Dynamic Read ODT */
            cvmx_lmcx_pll_status_t pll_status;
            pll_status.u64 = cvmx_read_csr(CVMX_LMCX_PLL_STATUS(ddr_interface_num));

            lmc_rodt_comp_ctl.s.enable = 1;
            lmc_rodt_comp_ctl.s.pctl = pll_status.s.ddr__pctl;
            lmc_rodt_comp_ctl.s.nctl = pll_status.s.ddr__nctl;
        }


        cvmx_write_csr(CVMX_LMCX_RODT_COMP_CTL(ddr_interface_num), lmc_rodt_comp_ctl.u64);

        ddr_print("RODT enable                                   : %6d\n", lmc_rodt_comp_ctl.s.enable);
        ddr_print("RODT pctl                                     : %6d\n", lmc_rodt_comp_ctl.s.pctl);
        ddr_print("RODT nctl                                     : %6d\n", lmc_rodt_comp_ctl.s.nctl);

        if (octeon_is_cpuid(OCTEON_CN56XX) || octeon_is_cpuid(OCTEON_CN50XX) || octeon_is_cpuid(OCTEON_CN52XX)) {
            /* CN56XX has a 5-bit copy of the compensation values */
            cvmx_lmcx_pll_status_t pll_status;
            pll_status.u64 = cvmx_read_csr(CVMX_LMCX_PLL_STATUS(ddr_interface_num));
            ddr_print("DDR PMOS control                              : %6d\n", pll_status.s.ddr__pctl);
            ddr_print("DDR NMOS control                              : %6d\n", pll_status.s.ddr__nctl);
        } else {
            tmp_ctl.u64 = cvmx_read_csr(CVMX_LMCX_CTL(ddr_interface_num));
            ddr_print("DDR PMOS control                              : %6d\n", tmp_ctl.s.ddr__pctl);
            ddr_print("DDR NMOS control                              : %6d\n", tmp_ctl.s.ddr__nctl);
        }
    }

    cvmx_write_csr(CVMX_LMCX_MEM_CFG0(ddr_interface_num), mem_cfg0.u64);
    mem_cfg0.u64 = cvmx_read_csr(CVMX_LMCX_MEM_CFG0(ddr_interface_num));

    mem_cfg0.s.init_start   = 1;
    cvmx_write_csr(CVMX_LMCX_MEM_CFG0(ddr_interface_num), mem_cfg0.u64);
    cvmx_read_csr(CVMX_LMCX_MEM_CFG0(ddr_interface_num));
    /* ----------------------------------------------------------------------------- */


    if ((octeon_is_cpuid(OCTEON_CN56XX)) || (octeon_is_cpuid(OCTEON_CN52XX))) {
        cvmx_lmcx_read_level_ctl_t read_level_ctl;
        cvmx_lmcx_read_level_rankx_t read_level_rankx;
        cvmx_lmc_ctl1_t lmc_ctl1;
        uint32_t rankmask;
        int sequence = READ_LEVEL_ENABLE; /* Default to using read leveling to establish timing */
        int global_delay;
        int idx, rankidx;


        /* Use read levelling if sequence is 0 */
        if (board_delay == 0)
            sequence = READ_LEVEL_ENABLE;

        if ((s = getenv_wrapper("ddr_read_level")) != NULL) {
            sequence = (simple_strtoul(s, NULL, 0) == 0) ? 0 : 1;
            error_print("Parameter found in environment. ddr_read_level = %d\n", sequence);
        }

        read_level_ctl.u64 = cvmx_read_csr(CVMX_LMCX_READ_LEVEL_CTL(ddr_interface_num));
        lmc_ctl1.u64 = cvmx_read_csr(CVMX_LMCX_CTL1(ddr_interface_num));
        read_level_rankx.u64 = 0;

        rankmask = (num_ranks == 1) ? 0x1 : 0x3; /* mask for ranks present */
        /* Replicate dimm0 mask for the number of dimms present */
        for (idx=1; idx<dimm_count; ++idx) {
            rankmask |= (rankmask & 0x3) << (idx*2);
        }

        for (rankidx = 0; rankidx < 4; ++rankidx) {
            if ((rankmask >> rankidx) & 1) {
                if (sequence == 1) {
                    read_level_ctl.s.rankmask = 1 << rankidx;
                    cvmx_write_csr(CVMX_LMCX_READ_LEVEL_CTL(ddr_interface_num), read_level_ctl.u64);
                    //ddr_print("Read Level Rankmask                           : %#6x\n", read_level_ctl.s.rankmask);

                    lmc_ctl1.s.sequence = 1;
                    cvmx_write_csr(CVMX_LMCX_CTL1(ddr_interface_num), lmc_ctl1.u64);
                    cvmx_read_csr(CVMX_LMCX_CTL1(ddr_interface_num));

                    /* Trigger init_start to allow read leveling to take effect */
                    mem_cfg0.u64 = cvmx_read_csr(CVMX_LMCX_MEM_CFG0(ddr_interface_num));
                    mem_cfg0.s.init_start   = 0; /* Insure that one shot is armed */
                    cvmx_write_csr(CVMX_LMCX_MEM_CFG0(ddr_interface_num), mem_cfg0.u64);
                    cvmx_read_csr(CVMX_LMCX_MEM_CFG0(ddr_interface_num));
                    mem_cfg0.s.init_start   = 1;
                    cvmx_write_csr(CVMX_LMCX_MEM_CFG0(ddr_interface_num), mem_cfg0.u64);
                    cvmx_read_csr(CVMX_LMCX_MEM_CFG0(ddr_interface_num));
                }

                if (sequence == 0) {
                    global_delay = board_delay * 4 / tclk_psecs;
                    read_level_rankx.s.byte0 = global_delay;
                    read_level_rankx.s.byte1 = global_delay;
                    read_level_rankx.s.byte2 = global_delay;
                    read_level_rankx.s.byte3 = global_delay;
                    read_level_rankx.s.byte4 = global_delay;
                    read_level_rankx.s.byte5 = global_delay;
                    read_level_rankx.s.byte6 = global_delay;
                    read_level_rankx.s.byte7 = global_delay;
                    read_level_rankx.s.byte8 = global_delay;

                    cvmx_write_csr(CVMX_LMCX_READ_LEVEL_RANKX(rankidx, ddr_interface_num), read_level_rankx.u64);
                }

                if (sequence == 1) {
                    int read_level_bitmask[9];
                    int byte_idx;

                    /*
                     * A given read of LMC0/1_READ_LEVEL_DBG returns
                     * the read-leveling pass/fail results for all
                     * possible delay settings (i.e. the BITMASK) for
                     * only one byte in the last rank that the
                     * hardware ran read-leveling
                     * on. LMC0/1_READ_LEVEL_DBG[BYTE] selects the
                     * particular byte. To get these pass/fail results
                     * for a different rank, you must run the hardware
                     * read-leveling again. For example, it is
                     * possible to get the BITMASK results for every
                     * byte of every rank if you run read-leveling
                     * separately for each rank, probing
                     * LMC0/1_READ_LEVEL_DBG between each
                     * read-leveling.
                     */

                    for (byte_idx=0; byte_idx<9; ++byte_idx)
                        read_level_bitmask[byte_idx] = octeon_read_lmcx_read_level_dbg(ddr_interface_num, byte_idx);

                    ddr_print("Rank(%d) Read Level Debug Test Results 8:0     : %04x %04x %04x %04x %04x %04x %04x %04x %04x\n",
                              rankidx,
                              read_level_bitmask[8],
                              read_level_bitmask[7],
                              read_level_bitmask[6],
                              read_level_bitmask[5],
                              read_level_bitmask[4],
                              read_level_bitmask[3],
                              read_level_bitmask[2],
                              read_level_bitmask[1],
                              read_level_bitmask[0] 
                              );
                    
                    read_level_rankx.s.byte8 = adjust_read_level_byte(read_level_bitmask[8]);
                    read_level_rankx.s.byte7 = adjust_read_level_byte(read_level_bitmask[7]);
                    read_level_rankx.s.byte6 = adjust_read_level_byte(read_level_bitmask[6]);
                    read_level_rankx.s.byte5 = adjust_read_level_byte(read_level_bitmask[5]);
                    read_level_rankx.s.byte4 = adjust_read_level_byte(read_level_bitmask[4]);
                    read_level_rankx.s.byte3 = adjust_read_level_byte(read_level_bitmask[3]);
                    read_level_rankx.s.byte2 = adjust_read_level_byte(read_level_bitmask[2]);
                    read_level_rankx.s.byte1 = adjust_read_level_byte(read_level_bitmask[1]);
                    read_level_rankx.s.byte0 = adjust_read_level_byte(read_level_bitmask[0]);
                    cvmx_write_csr(CVMX_LMCX_READ_LEVEL_RANKX(rankidx, ddr_interface_num), read_level_rankx.u64);
                }

                read_level_rankx.u64 = cvmx_read_csr(CVMX_LMCX_READ_LEVEL_RANKX(rankidx, ddr_interface_num));
                ddr_print("Rank(%d) Status / Deskew Bytes 8:0      %#6x : %4d %4d %4d %4d %4d %4d %4d %4d %4d\n",
                          rankidx,
                          read_level_rankx.s.status,
                          read_level_rankx.s.byte8,
                          read_level_rankx.s.byte7,
                          read_level_rankx.s.byte6,
                          read_level_rankx.s.byte5,
                          read_level_rankx.s.byte4,
                          read_level_rankx.s.byte3,
                          read_level_rankx.s.byte2,
                          read_level_rankx.s.byte1,
                          read_level_rankx.s.byte0
                          );
                if ((sequence == 1) && DEBUG_DEFINED) {
                    uint32_t avg_board_delay;
                    avg_board_delay = (( read_level_rankx.s.byte8 + read_level_rankx.s.byte7 + read_level_rankx.s.byte6 +
                                         read_level_rankx.s.byte5 + read_level_rankx.s.byte4 + read_level_rankx.s.byte3 +
                                         read_level_rankx.s.byte2 + read_level_rankx.s.byte1 + read_level_rankx.s.byte0 ) / 9 ) * tclk_psecs / 4;
                    ddr_print("Equivalent Board Delay                        : %4d:%4d ps\n", avg_board_delay, avg_board_delay + tclk_psecs / 4);
                }
            }
        }
    }

    if ((octeon_is_cpuid(OCTEON_CN56XX)) || (octeon_is_cpuid(OCTEON_CN52XX)) || (octeon_is_cpuid(OCTEON_CN58XX))) {
        cvmx_lmcx_nxm_t lmc_nxm;
        lmc_nxm.u64 = 0;
        static const unsigned char _cs_mask_interface_wide  [] = {(unsigned char)~0x33, (unsigned char)~0xFF};
        static const unsigned char _cs_mask_interface_narrow[] = {(unsigned char)~0x03, (unsigned char)~0x0F, (unsigned char)~0x3F, (unsigned char)~0xFF};

        /*
        **  128 bit interface
        **  dimm 0,1 cs_0,cs_4 Rank 0,0 mask 0x11
        **  dimm 0,1 cs_1,cs_5 Rank 1,1 mask 0x22
        **
        **  dimm 2,3 cs_2,cs_6 Rank 0,0 mask 0x44
        **  dimm 2,3 cs_3,cs_7 Rank 1,1 mask 0x88
        **
        **  64 bit interface
        **  dimm 0   cs_0      Rank 0,0 mask 0x01
        **  dimm 0   cs_1      Rank 1,1 mask 0x02
        **
        **  dimm 1   cs_2      Rank 0,0 mask 0x04
        **  dimm 1   cs_3      Rank 1,1 mask 0x08
        **
        **  dimm 2   cs_4      Rank 0,0 mask 0x10
        **  dimm 2   cs_5      Rank 1,1 mask 0x20
        **
        **  dimm 3   cs_6      Rank 0,0 mask 0x40
        **  dimm 3   cs_7      Rank 1,1 mask 0x80
        */

        if (ddr_interface_width > 64)
            lmc_nxm.s.cs_mask = _cs_mask_interface_wide  [(dimm_count/2)-1]; /* Get mask from table */
        else
            lmc_nxm.s.cs_mask = _cs_mask_interface_narrow[dimm_count-1]; /* Get mask from table */

        ddr_print("lmc_nxm                                       :     %02x\n", lmc_nxm.u64);
        cvmx_write_csr(CVMX_LMCX_NXM(ddr_interface_num), lmc_nxm.u64);
    }


#ifdef PRINT_LMC_REGS
    printf("##################################################################\n");
    printf("LMC register dump for interface %d\n",ddr_interface_num);
    octeon_print_lmc_regs(cpu_id, ddr_interface_num);
    printf("##################################################################\n");
#endif

    return(mem_size_mbytes);
}

int limit_l2_ways(int ways, int verbose)
{
    uint32_t valid_mask = (0x1 << cvmx_l2c_get_num_assoc()) - 1;
    int ways_max = cvmx_l2c_get_num_assoc();
    int ways_min;
    uint32_t mask = (valid_mask << ways) & valid_mask;
    int errors = 0;

    if (OCTEON_IS_MODEL(OCTEON_CN63XX) || OCTEON_IS_MODEL(OCTEON_CN71XX))
        ways_min = 0;
    else
        ways_min = 1;

    if (ways >= ways_min && ways <= ways_max)
    {
        int i;
        if (verbose)
            printf("Limiting L2 to %d ways\n", ways);
        for (i = 0; i < (int)cvmx_octeon_num_cores(); i++)
            errors += cvmx_l2c_set_core_way_partition(i, mask);
        errors += cvmx_l2c_set_hw_way_partition(mask);
    }
    else
    {
        errors++;
        printf("ERROR: invalid limit_l2_ways, must be between %d and %d\n", ways_min, ways_max);
    }
    if (errors)
    {
        printf("ERROR limiting L2 cache ways\n");
    }
    return errors;
}

static int limit_l2_mask(uint32_t mask)
{
    uint32_t valid_mask = (0x1 << cvmx_l2c_get_num_assoc()) - 1;
    int errors = 0;
                  
    if ((mask&valid_mask) == mask) {
        int i;
        printf("Limiting L2 mask 0x%x\n", mask);
        for (i = 0; i < (int)cvmx_octeon_num_cores(); i++)
            errors += cvmx_l2c_set_core_way_partition(i, mask);
        errors += cvmx_l2c_set_hw_way_partition(mask);
    }
    else
    {
        errors++;
        printf("ERROR: invalid limit_l2_mask 0x%x, valid mask = 0x%x\n", mask, valid_mask);
    }
    if (errors)
    {
        printf("ERROR limiting L2 cache ways\n");
    }
    return errors;
}


static int test_dram_byte(uint64_t p, int count, int stride, int byte)
{
    uint64_t p1, p2, d1, d2;
    int errors = 0;
    int i;
    uint64_t v;
    p |= 1ull<<63;

    /* Add offset to both test regions to not clobber u-boot stuff when running from L2 for L2_ONLY mode
    ** or NAND boot. */
    p += 0x800000;

    uint64_t p2base = ((((p+count*8)+0x100000)>>20)<<20); /* Round up to next MB boundary */

    p1 = p;
    p2 = p2base;
    for (i=0; i<count; ++i) {
        v = (~((uint64_t)i)&0xff)<<(8*byte);
        cvmx_write64_uint64(p1, v);
        cvmx_write64_uint64(p2, ~v);
        p1 += stride;
        p2 += stride;
    }

    p1 = p;
    p2 = p2base;
    for (i=0; i<count; ++i) {
        v = (~((uint64_t)i)&0xff)<<(8*byte);
        d1 = cvmx_read64_uint64(p1) & (0xFFULL << 8*byte);
        d2 = ~cvmx_read64_uint64(p2) & (0xFFULL << 8*byte);

        if (d1 != v) {
            ++errors;
            debug_print("%d: [0x%016llX] 0x%016llX expected 0x%016llX xor %016llX\n",
                        errors, p1, d1, v, (d1 ^ v));
            return errors;      /* Quit on first error */
        }
        if (d2 != v) {
            ++errors;            
            debug_print("%d: [0x%016llX] 0x%016llX  expected 0x%016llX xor %016llX\n",
                        errors, p2, d2, v, (d2 ^ v));
            return errors;      /* Quit on first error */
        }
        p1 += stride;
        p2 += stride;
    }
    return errors;
}

static void update_wlevel_rank_struct(cvmx_lmcx_wlevel_rankx_t *lmc_wlevel_rank, int byte, int delay)
{
    switch(byte) {
    case 0:
        lmc_wlevel_rank->cn63xx.byte0 = delay;
        break;
    case 1:
        lmc_wlevel_rank->cn63xx.byte1 = delay;
        break;
    case 2:
        lmc_wlevel_rank->cn63xx.byte2 = delay;
        break;
    case 3:
        lmc_wlevel_rank->cn63xx.byte3 = delay;
        break;
    case 4:
        lmc_wlevel_rank->cn63xx.byte4 = delay;
        break;
    case 5:
        lmc_wlevel_rank->cn63xx.byte5 = delay;
        break;
    case 6:
        lmc_wlevel_rank->cn63xx.byte6 = delay;
        break;
    case 7:
        lmc_wlevel_rank->cn63xx.byte7 = delay;
        break;
    case 8:
        lmc_wlevel_rank->cn63xx.byte8 = delay;
        break;
    }
}

static int  get_wlevel_rank_struct(cvmx_lmcx_wlevel_rankx_t *lmc_wlevel_rank, int byte)
{
    int delay;
    switch(byte) {
    case 0:
        delay = lmc_wlevel_rank->cn63xx.byte0;
        break;
    case 1:
        delay = lmc_wlevel_rank->cn63xx.byte1;
        break;
    case 2:
        delay = lmc_wlevel_rank->cn63xx.byte2;
        break;
    case 3:
        delay = lmc_wlevel_rank->cn63xx.byte3;
        break;
    case 4:
        delay = lmc_wlevel_rank->cn63xx.byte4;
        break;
    case 5:
        delay = lmc_wlevel_rank->cn63xx.byte5;
        break;
    case 6:
        delay = lmc_wlevel_rank->cn63xx.byte6;
        break;
    case 7:
        delay = lmc_wlevel_rank->cn63xx.byte7;
        break;
    case 8:
        delay = lmc_wlevel_rank->cn63xx.byte8;
        break;
    }
    return delay;
}

static void update_rlevel_rank_struct(cvmx_lmcx_rlevel_rankx_t *lmc_rlevel_rank, int byte, int delay)
{
    switch(byte) {
    case 0:
        lmc_rlevel_rank->cn63xx.byte0 = delay;
        break;
    case 1:
        lmc_rlevel_rank->cn63xx.byte1 = delay;
        break;
    case 2:
        lmc_rlevel_rank->cn63xx.byte2 = delay;
        break;
    case 3:
        lmc_rlevel_rank->cn63xx.byte3 = delay;
        break;
    case 4:
        lmc_rlevel_rank->cn63xx.byte4 = delay;
        break;
    case 5:
        lmc_rlevel_rank->cn63xx.byte5 = delay;
        break;
    case 6:
        lmc_rlevel_rank->cn63xx.byte6 = delay;
        break;
    case 7:
        lmc_rlevel_rank->cn63xx.byte7 = delay;
        break;
    case 8:
        lmc_rlevel_rank->cn63xx.byte8 = delay;
        break;
    }
}

static int  get_rlevel_rank_struct(cvmx_lmcx_rlevel_rankx_t *lmc_rlevel_rank, int byte)
{
    int delay;
    switch(byte) {
    case 0:
        delay = lmc_rlevel_rank->cn63xx.byte0;
        break;
    case 1:
        delay = lmc_rlevel_rank->cn63xx.byte1;
        break;
    case 2:
        delay = lmc_rlevel_rank->cn63xx.byte2;
        break;
    case 3:
        delay = lmc_rlevel_rank->cn63xx.byte3;
        break;
    case 4:
        delay = lmc_rlevel_rank->cn63xx.byte4;
        break;
    case 5:
        delay = lmc_rlevel_rank->cn63xx.byte5;
        break;
    case 6:
        delay = lmc_rlevel_rank->cn63xx.byte6;
        break;
    case 7:
        delay = lmc_rlevel_rank->cn63xx.byte7;
        break;
    case 8:
        delay = lmc_rlevel_rank->cn63xx.byte8;
        break;
    }
    return delay;
}

static void rlevel_to_wlevel(cvmx_lmcx_rlevel_rankx_t *lmc_rlevel_rank, cvmx_lmcx_wlevel_rankx_t *lmc_wlevel_rank, int byte)
{
    int byte_delay = get_rlevel_rank_struct(lmc_rlevel_rank, byte);

    debug_print("Estimating Wlevel delay byte %d: ", byte);
    debug_print("Rlevel=%d => ", byte_delay);
    byte_delay = divide_roundup(byte_delay,2) & 0x1e;
    debug_print("Wlevel=%d\n", byte_delay);
    update_wlevel_rank_struct(lmc_wlevel_rank, byte, byte_delay);
}

static int nonsequential_delays(int delay0, int delay1, int delay2)
{
    int error = 0;
    int near_diff = _abs (delay1 - delay0);
    int far_diff  = _abs (delay2 - delay0);
    int near_sign = _sign(delay1 - delay0);
    int far_sign  = _sign(delay2 - delay0);
    int sign_miss = ((near_sign != far_sign)&&(near_diff != 0));

    debug_bitmask_print("delay0: %2d, delay1: %2d, delay2: %2d", delay0, delay1, delay2);

    if ((near_diff > far_diff) || sign_miss) {
        error = 1;
        debug_bitmask_print(" => invalid (nonsequential)");
    }
    debug_bitmask_print("\n");
    return error;
}

static int roundup_ddr3_wlevel_bitmask(int bitmask)
{
    int shifted_bitmask;
    int leader;
    int delay;

    for (leader=0; leader<8; ++leader) {
        shifted_bitmask = (bitmask>>leader);
        if ((shifted_bitmask&1) == 0)
            break;
    }

    for (leader=leader; leader<16; ++leader) {
        shifted_bitmask = (bitmask>>(leader%8));
        if (shifted_bitmask&1)
            break;
    }

    delay = (leader&1) ? leader+1 : leader;
    delay = delay % 8;

    return delay;
}

static void perform_ddr3_init_sequence(uint32_t cpu_id, int rank_mask, int ddr_interface_num)
{
#if defined(__U_BOOT__)
    DECLARE_GLOBAL_DATA_PTR;
#endif
#if defined(__U_BOOT__) || defined(unix)
    char *s;
#endif
    int ddr_init_loops = 1;
    static const char *sequence_str[] = {
        "power-up/init",
        "read-leveling",
        "self-refresh entry",
        "self-refresh exit",
        "precharge power-down entry",
        "precharge power-down exit",
        "write-leveling",
        "illegal"
    };

    if ((s = lookup_env_parameter("ddr_init_loops")) != NULL) {
        ddr_init_loops = simple_strtoul(s, NULL, 0);
    }

    while (ddr_init_loops--) {
        cvmx_lmcx_config_t lmc_config;
        cvmx_lmcx_control_t lmc_control;
        cvmx_lmcx_reset_ctl_t lmc_reset_ctl;
        int save_ddr2t;

        lmc_control.u64 = cvmx_read_csr(CVMX_LMCX_CONTROL(ddr_interface_num));
        save_ddr2t                    = lmc_control.s.ddr2t;

        if ((save_ddr2t == 0) && octeon_is_cpuid(OCTEON_CN63XX_PASS1_X)) {
            /* Some register parts (IDT and TI included) do not like
               the sequence that LMC generates for an MRS register
               write in 1T mode. In this case, the register part does
               not properly forward the MRS register write to the DRAM
               parts.  See errata (LMC-14548) Issues with registered
               DIMMs. */
            ddr_print("Forcing DDR 2T during init seq. Re: Pass 1 LMC-14548\n");
            lmc_control.s.ddr2t           = 1;
        }

        if ((s = lookup_env_parameter("ddr_init_2t")) != NULL) {
            lmc_control.s.ddr2t = simple_strtoul(s, NULL, 0);
        }

        cvmx_write_csr(CVMX_LMCX_CONTROL(ddr_interface_num), lmc_control.u64);

        lmc_config.u64 = cvmx_read_csr(CVMX_LMCX_CONFIG(ddr_interface_num));
        lmc_config.s.init_start      = 1;

        lmc_reset_ctl.u64 = cvmx_read_csr(CVMX_LMCX_RESET_CTL(ddr_interface_num));
        if (lmc_reset_ctl.cn63xx.ddr3psv) {
            /* Contents are being preserved */
            ddr_print("Exiting Self-Refresh\n");
            lmc_config.s.sequence        = 3; /* self-refresh exit */
#if defined(__U_BOOT__)
            gd->flags |= GD_FLG_MEMORY_PRESERVED;
#endif
            /* Re-initialize flags */
            lmc_reset_ctl.cn63xx.ddr3pwarm = 0;
            lmc_reset_ctl.cn63xx.ddr3psoft = 0;
            lmc_reset_ctl.cn63xx.ddr3psv   = 0;
            cvmx_write_csr(CVMX_LMCX_RESET_CTL(ddr_interface_num), lmc_reset_ctl.u64);
        } else {
            /* Contents are not being preserved */
            lmc_config.s.sequence        = 0; /* power-up/init */
#if defined(__U_BOOT__)
            gd->flags &= ~GD_FLG_MEMORY_PRESERVED;
#endif
        }

        lmc_config.s.rankmask        = rank_mask;
        lmc_config.s.init_status     = rank_mask;

        cvmx_write_csr(CVMX_LMCX_CONFIG(ddr_interface_num), lmc_config.u64);
        cvmx_read_csr(CVMX_LMCX_CONFIG(ddr_interface_num));
        cvmx_wait_usec(600); /* Wait a while */

        if ((s = lookup_env_parameter("ddr_sequence1")) != NULL) {
            int sequence1;
            sequence1 = simple_strtoul(s, NULL, 0);

            lmc_config.s.sequence        = sequence1;
            lmc_config.s.init_start      = 1;

            error_print("Parameter found in environment. ddr_sequence1 = %d (%s)\n",
                        sequence1, sequence_str[lmc_config.s.sequence]);

            cvmx_write_csr(CVMX_LMCX_CONFIG(ddr_interface_num), lmc_config.u64);
            cvmx_read_csr(CVMX_LMCX_CONFIG(ddr_interface_num));
            cvmx_wait_usec(600); /* Wait a while */
        }

        if ((s = lookup_env_parameter("ddr_sequence2")) != NULL) {
            int sequence2;
            sequence2 = simple_strtoul(s, NULL, 0);

            lmc_config.s.sequence        = sequence2;
            lmc_config.s.init_start      = 1;

            error_print("Parameter found in environment. ddr_sequence2 = %d (%s)\n",
                        sequence2, sequence_str[lmc_config.s.sequence]);

            cvmx_write_csr(CVMX_LMCX_CONFIG(ddr_interface_num), lmc_config.u64);
            cvmx_read_csr(CVMX_LMCX_CONFIG(ddr_interface_num));
            cvmx_wait_usec(600); /* Wait a while */
        }

        lmc_control.s.ddr2t           = save_ddr2t;
        cvmx_write_csr(CVMX_LMCX_CONTROL(ddr_interface_num), lmc_control.u64);
        cvmx_read_csr(CVMX_LMCX_CONTROL(ddr_interface_num));
    }
}

static int init_octeon_ddr3_interface(uint32_t cpu_id,
                                      const ddr_configuration_t *ddr_configuration,
                                      uint32_t ddr_hertz,
                                      uint32_t cpu_hertz,
                                      uint32_t ddr_ref_hertz,
                                      int board_type,
                                      int board_rev_maj,
                                      int board_rev_min,
                                      int ddr_interface_num,
                                      uint32_t ddr_interface_mask
                                      )
{
#if defined(__U_BOOT__) || defined(unix)
    char *s;
#endif

    const dimm_odt_config_t *odt_1rank_config = ddr_configuration->odt_1rank_config;
    const dimm_odt_config_t *odt_2rank_config = ddr_configuration->odt_2rank_config;
    const dimm_config_t *dimm_config_table = ddr_configuration->dimm_config_table;
    const dimm_odt_config_t *odt_config;
    const ddr3_custom_config_t *custom_lmc_config = &ddr_configuration->custom_lmc_config;
    int odt_idx;

    /*
    ** Compute clock rates to the nearest picosecond.
    */
    ulong tclk_psecs = divide_nint((uint64_t) 1000*1000*1000*1000, ddr_hertz); /* Clock in psecs */
    ulong eclk_psecs = divide_nint((uint64_t) 1000*1000*1000*1000, cpu_hertz); /* Clock in psecs */

    int row_bits, col_bits, num_banks, num_ranks, dram_width, real_num_ranks;
    int dimm_count = 0;
    int fatal_error = 0;        /* Accumulate and report all the errors before giving up */

    int safe_ddr_flag = 0; /* Flag that indicates safe DDR settings should be used */
    int ddr_interface_wide = 0;
    int ddr_interface_width;
    uint32_t mem_size_mbytes = 0;
    unsigned int didx;
    int bank_bits = 0;
    int bunk_enable;
    int column_bits_start = 1;
    int row_lsb;
    int pbank_lsb;
    int use_ecc = 1;
    int mtb_psec;
    int tAAmin;
    int tCKmin;
    int CL, min_cas_latency = 0, max_cas_latency = 0, override_cas_latency = 0;
    int ddr_rtt_nom_auto, ddr_rodt_ctl_auto;
    int i;

    int spd_addr;
    int spd_org;
    int spd_rdimm;
    int spd_dimm_type;
    int spd_ecc;
    int spd_cas_latency;
    int spd_mtb_dividend;
    int spd_mtb_divisor;
    int spd_tck_min;
    int spd_taa_min;
    int spd_twr;
    int spd_trcd;
    int spd_trrd;
    int spd_trp;
    int spd_tras;
    int spd_trc;
    int spd_trfc;
    int spd_twtr;
    int spd_trtp;
    int spd_tfaw;
    int spd_addr_mirror;

    int twr;
    int trcd;
    int trrd;
    int trp;
    int tras;
    int trc;
    int trfc;
    int twtr;
    int trtp;
    int tfaw;

#if CONFIG_SOFTWARE_LEVELING
    int software_leveling = 0;
#endif

#if SAVE_RESTORE_DDR_CONFIG
    cvmx_lmcx_wlevel_rankx_t save_lmc_wlevel_rank[4];
    cvmx_lmcx_rlevel_rankx_t save_lmc_rlevel_rank[4];
    cvmx_lmcx_comp_ctl2_t    save_lmc_comp_ctl2;
    int rankx;
#endif

    int wlevel_bitmask_errors = 0;
    int wlevel_loops = 0;


    ddr_print("\nInitializing DDR interface %d, DDR Clock %d, DDR Reference Clock %d, CPUID 0x%08x\n",
              ddr_interface_num, ddr_hertz, ddr_ref_hertz, cpu_id);

    if (dimm_config_table[0].spd_addrs[0] == 0 && !dimm_config_table[0].spd_ptrs[0]) {
        error_print("ERROR: No dimms specified in the dimm_config_table.\n");
        return (-1);
    }
    
    if (ddr_verbose()) {
        printf("DDR SPD Table:");
        for (didx = 0; didx < DDR_CFG_T_MAX_DIMMS; ++didx) {
            if (dimm_config_table[didx].spd_addrs[0] == 0) break;
            printf(" --ddr%dspd=0x%02x", ddr_interface_num, dimm_config_table[didx].spd_addrs[0]);
            if (dimm_config_table[didx].spd_addrs[1] != 0)
                printf(",0x%02x", dimm_config_table[didx].spd_addrs[1]);
        }
        printf("\n");
    }

    /*
    ** Walk the DRAM Socket Configuration Table to see what is installed.
    */
    for (didx = 0; didx < DDR_CFG_T_MAX_DIMMS; ++didx)
    {
        /* Check for lower DIMM socket populated */
        if (validate_dimm(dimm_config_table[didx], 0)) {
            if (ddr_verbose())
                report_ddr3_dimm(dimm_config_table[didx], 0, dimm_count);
            ++dimm_count;
        } else { break; }       /* Finished when there is no lower DIMM */

        if (octeon_is_cpuid(OCTEON_CN63XX))
        {
            ddr_interface_wide = 0; /* Only support 64bit interface width */
            continue;
        }
    }


    initialize_ddr_clock(cpu_id, ddr_configuration, cpu_hertz, ddr_hertz,
                         ddr_ref_hertz, ddr_interface_num, ddr_interface_mask);

    if (!odt_1rank_config)
        odt_1rank_config = disable_odt_config;
    if (!odt_2rank_config)
        odt_2rank_config = disable_odt_config;

#if CONFIG_SOFTWARE_LEVELING
    if ((s = lookup_env_parameter("ddr_software_leveling")) != NULL) {
        software_leveling = simple_strtoul(s, NULL, 0);
    }
#endif

    if ((s = getenv("ddr_safe")) != NULL) {
        safe_ddr_flag = simple_strtoul(s, NULL, 0);
        error_print("Parameter found in environment. ddr_safe = %d\n", safe_ddr_flag);
    }


    if (dimm_count == 0) {
        error_print("ERROR: DIMM 0 not detected.\n");
        return(-1);
    }

    if (ddr_interface_wide && ((s = lookup_env_parameter("ddr_narrow")) != NULL) && (strcmp(s,"yes") == 0)) {
        error_print("Parameter found in environment: ddr_narrow, forcing narrow ddr interface\n");
        ddr_interface_wide = 0;
    }


    spd_addr = read_spd(dimm_config_table[0], 0, DDR3_SPD_ADDRESSING_ROW_COL_BITS);
    debug_print("spd_addr        : %#06x\n", spd_addr );
    row_bits = 12 + ((spd_addr >> 3) & 0x7);
    col_bits =  9 + ((spd_addr >> 0) & 0x7);

    num_banks = 1 << (3+((read_spd(dimm_config_table[0], 0, DDR3_SPD_DENSITY_BANKS) >> 4) & 0x7));

    spd_org = read_spd(dimm_config_table[0], 0, DDR3_SPD_MODULE_ORGANIZATION);
    debug_print("spd_org         : %#06x\n", spd_org );
    num_ranks =  1 + ((spd_org >> 3) & 0x7);
    dram_width = 4 << ((spd_org >> 0) & 0x7);



    /* FIX
    ** Check that values are within some theoretical limits.
    ** col_bits(min) = row_lsb(min) - bank_bits(max) - bus_bits(max) = 14 - 3 - 4 = 7
    ** col_bits(max) = row_lsb(max) - bank_bits(min) - bus_bits(min) = 18 - 2 - 3 = 13
    */
    if ((col_bits > 13) || (col_bits < 7)) {
        error_print("Unsupported number of Col Bits: %d\n", col_bits);
        ++fatal_error;
    }

    /* FIX
    ** Check that values are within some theoretical limits.
    ** row_bits(min) = pbank_lsb(min) - row_lsb(max) - rank_bits = 26 - 18 - 1 = 7
    ** row_bits(max) = pbank_lsb(max) - row_lsb(min) - rank_bits = 33 - 14 - 1 = 18
    */
    if ((row_bits > 18) || (row_bits < 7)) {
        error_print("Unsupported number of Row Bits: %d\n", row_bits);
        ++fatal_error;
    }

    if (num_banks == 8)
        bank_bits = 3;
    else if (num_banks == 4)
        bank_bits = 2;



    spd_dimm_type   = read_spd(dimm_config_table[0], 0, DDR3_SPD_KEY_BYTE_MODULE_TYPE);
    spd_rdimm       = (spd_dimm_type == 1) || (spd_dimm_type == 5) || (spd_dimm_type == 9);

    if ((s = lookup_env_parameter("ddr_rdimm_ena")) != NULL) {
        spd_rdimm = simple_strtoul(s, NULL, 0);
    }

    /*  Avoid using h/w write-leveling for pass 1.0 and 1.1 silicon.
        This bug is partially fixed in pass 1.2-1.x. (See LMC-14415B) */
    if (octeon_is_cpuid(OCTEON_CN63XX_PASS1_0) || octeon_is_cpuid(OCTEON_CN63XX_PASS1_1)) {
        ddr_print("Disabling H/W Write-leveling. Re: Pass 1.0-1.1 LMC-14415B\n");
        wlevel_loops = 0;
    } else
        wlevel_loops = 1;

    /* Avoid using h/w write-leveling for pass 1.X silicon with dual
       ranked, registered dimms.  See errata (LMC-14548) Issues with
       registered DIMMs. */
    if (spd_rdimm && (num_ranks==2) && octeon_is_cpuid(OCTEON_CN63XX_PASS1_X))
        wlevel_loops = 0;

    if ((s = getenv("ddr_wlevel_loops")) != NULL) {
        wlevel_loops = simple_strtoul(s, NULL, 0);
        error_print("Parameter found in environment. ddr_wlevel_loops = %d\n", wlevel_loops);
    }

    real_num_ranks = num_ranks;

    if (octeon_is_cpuid(OCTEON_CN63XX_PASS1_X)) {
        /* NOTICE: Ignoring second rank of registered DIMMs if using
           h/w write-leveling for pass 1. See errata (LMC-14548)
           Issues with registered DIMMs. */
        num_ranks = (spd_rdimm && wlevel_loops) ? 1 : real_num_ranks;
    }

    if ((s = lookup_env_parameter("ddr_ranks")) != NULL) {
        num_ranks = simple_strtoul(s, NULL, 0);
    }

    if (octeon_is_cpuid(OCTEON_CN63XX_PASS1_X)) {
        /* NOTICE: Ignoring second rank of registered DIMMs if using
           h/w write-leveling for pass 1. See errata (LMC-14548)
           Issues with registered DIMMs. */
        if ((spd_rdimm) && (num_ranks != real_num_ranks))  {
            ddr_print("Ignoring second rank of Registered DIMMs. Re: Pass 1 LMC-14548\n");
        }
    }

    bunk_enable = (num_ranks > 1);

    if (octeon_is_cpuid(OCTEON_CN63XX))
        column_bits_start = 3;
    else 
        printf("ERROR: Unsupported Octeon model: 0x%x\n", cpu_id);

    row_lsb = column_bits_start + col_bits + bank_bits;
    debug_print("row_lsb = column_bits_start + col_bits + bank_bits = %d\n", row_lsb);

    pbank_lsb = row_lsb + row_bits + bunk_enable;
    debug_print("pbank_lsb = row_lsb + row_bits + bunk_enable = %d\n", pbank_lsb);


    mem_size_mbytes =  dimm_count * ((1ull << pbank_lsb) >> 20);


    /* Mask with 1 bits set for for each active rank, allowing 2 bits per dimm.
    ** This makes later calculations simpler, as a variety of CSRs use this layout.
    ** This init needs to be updated for dual configs (ie non-identical DIMMs).
    ** Bit 0 = dimm0, rank 0
    ** Bit 1 = dimm0, rank 1
    ** Bit 2 = dimm1, rank 0
    ** Bit 3 = dimm1, rank 1
    ** ...
    */
    int rank_mask = 1 | ((num_ranks > 1) << 1);

    for (i = 1; i < dimm_count; i++)
        rank_mask |= ((rank_mask & 0x3) << (2*i));

    ddr_print("row bits: %d, col bits: %d, banks: %d, ranks: %d, dram width: %d, size: %d MB\n",
              row_bits, col_bits, num_banks, num_ranks, dram_width, mem_size_mbytes);

#if defined(__U_BOOT__)
    {
        /* If we are booting from RAM, the DRAM controller is already set up.  Just return the
        ** memory size */
        DECLARE_GLOBAL_DATA_PTR;
        if (gd->flags & GD_FLG_RAM_RESIDENT)
            return mem_size_mbytes;
    }
#endif


    spd_ecc         = !!(read_spd(dimm_config_table[0], 0, DDR3_SPD_MEMORY_BUS_WIDTH) & 8);

    spd_cas_latency  = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_CAS_LATENCIES_LSB);
    spd_cas_latency |= ((0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_CAS_LATENCIES_MSB)) << 8);

    spd_mtb_dividend = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MEDIUM_TIMEBASE_DIVIDEND);
    spd_mtb_divisor  = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MEDIUM_TIMEBASE_DIVISOR);
    spd_tck_min      = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MINIMUM_CYCLE_TIME_TCKMIN);
    spd_taa_min      = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MIN_CAS_LATENCY_TAAMIN);

    spd_twr          = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MIN_WRITE_RECOVERY_TWRMIN);
    spd_trcd         = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MIN_RAS_CAS_DELAY_TRCDMIN);
    spd_trrd         = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MIN_ROW_ACTIVE_DELAY_TRRDMIN);
    spd_trp          = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MIN_ROW_PRECHARGE_DELAY_TRPMIN);
    spd_tras         = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MIN_ACTIVE_PRECHARGE_LSB_TRASMIN);
    spd_tras        |= ((0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_UPPER_NIBBLES_TRAS_TRC)&0xf) << 8);
    spd_trc          = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MIN_ACTIVE_REFRESH_LSB_TRCMIN);
    spd_trc         |= ((0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_UPPER_NIBBLES_TRAS_TRC)&0xf0) << 4);
    spd_trfc         = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MIN_REFRESH_RECOVERY_LSB_TRFCMIN);
    spd_trfc        |= ((0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MIN_REFRESH_RECOVERY_MSB_TRFCMIN)) << 8);
    spd_twtr         = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MIN_INTERNAL_WRITE_READ_CMD_TWTRMIN);
    spd_trtp         = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MIN_INTERNAL_READ_PRECHARGE_CMD_TRTPMIN);
    spd_tfaw         = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MIN_FOUR_ACTIVE_WINDOW_TFAWMIN);
    spd_tfaw        |= ((0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_UPPER_NIBBLE_TFAW)&0xf) << 8);
    spd_addr_mirror  = 0xff & read_spd(dimm_config_table[0], 0,DDR3_SPD_ADDRESS_MAPPING) & 0x1;
    spd_addr_mirror  = spd_addr_mirror && !spd_rdimm; /* Only address mirror unbuffered dimms.  */

    if ((s = getenv("ddr_use_ecc")) != NULL) {
        use_ecc = simple_strtoul(s, NULL, 0);
        error_print("Parameter found in environment. ddr_use_ecc = %d\n", use_ecc);
    }
    use_ecc = use_ecc && spd_ecc;

    ddr_interface_width = 64;

    ddr_print("DRAM Interface width: %d bits %s\n", ddr_interface_width, use_ecc ? "+ECC" : "");


    debug_print("spd_cas_latency : %#06x\n", spd_cas_latency );
    debug_print("spd_twr         : %#06x\n", spd_twr );
    debug_print("spd_trcd        : %#06x\n", spd_trcd);
    debug_print("spd_trrd        : %#06x\n", spd_trrd);
    debug_print("spd_trp         : %#06x\n", spd_trp );
    debug_print("spd_tras        : %#06x\n", spd_tras);
    debug_print("spd_trc         : %#06x\n", spd_trc );
    debug_print("spd_trfc        : %#06x\n", spd_trfc);
    debug_print("spd_twtr        : %#06x\n", spd_twtr);
    debug_print("spd_trtp        : %#06x\n", spd_trtp);
    debug_print("spd_tfaw        : %#06x\n", spd_tfaw);
    debug_print("spd_addr_mirror : %#06x\n", spd_addr_mirror);


    mtb_psec        = spd_mtb_dividend * 1000 / spd_mtb_divisor;
    tAAmin          = mtb_psec * spd_taa_min;
    tCKmin          = mtb_psec * spd_tck_min;

    CL              = divide_roundup(tAAmin, tclk_psecs);

    ddr_print("Desired CAS Latency                           : %6d\n", CL);

    min_cas_latency = custom_lmc_config->min_cas_latency;


    if ((s = lookup_env_parameter("ddr_min_cas_latency")) != NULL) {
        min_cas_latency = simple_strtoul(s, NULL, 0);
    }

    ddr_print("CAS Latencies supported in DIMM               :");
    for (i=0; i<16; ++i) {
        if ((spd_cas_latency >> i) & 1) {
            ddr_print(" %d", i+4);
            max_cas_latency = i+4;
            if (min_cas_latency == 0)
                min_cas_latency = i+4;
        }
    }
    ddr_print("\n");


    /* Use relaxed timing when running slower than the minimum
     supported speed.  Adjust timing to match the smallest supported
     CAS Latency. */
    if (CL < min_cas_latency) {
        ulong adjusted_tclk = tAAmin / min_cas_latency;
        CL = min_cas_latency;
        ddr_print("Slow clock speed. Adjusting timing: tClk = %d, Adjusted tClk = %d\n",
                  tclk_psecs, adjusted_tclk);
        tclk_psecs = adjusted_tclk;
    }

    if ((s = getenv("ddr_cas_latency")) != NULL) {
        override_cas_latency = simple_strtoul(s, NULL, 0);
        error_print("Parameter found in environment. ddr_cas_latency = %d\n", override_cas_latency);
    }

    /* Make sure that the selected cas latency is legal */
    for (i=(CL-4); i<16; ++i) {
        if ((spd_cas_latency >> i) & 1) {
            CL = i+4;
            break;
        }
    }

    if (CL > max_cas_latency)
        CL = max_cas_latency;

    if (override_cas_latency != 0) {
        CL = override_cas_latency;
    }

    ddr_print("CAS Latency                                   : %6d\n", CL);

    if ((CL * tCKmin) > 20000)
    {
        ddr_print("(CLactual * tCKmin) = %d exceeds 20 ns\n", (CL * tCKmin));
    }

    if (tclk_psecs < (ulong)tCKmin)
        error_print("WARNING!!!!!!: DDR3 Clock Rate (tCLK) exceeds DIMM specifications!!!!!!!!\n");

    twr             = spd_twr  * mtb_psec;
    trcd            = spd_trcd * mtb_psec;
    trrd            = spd_trrd * mtb_psec;
    trp             = spd_trp  * mtb_psec;
    tras            = spd_tras * mtb_psec;
    trc             = spd_trc  * mtb_psec;
    trfc            = spd_trfc * mtb_psec;
    twtr            = spd_twtr * mtb_psec;
    trtp            = spd_trtp * mtb_psec;
    tfaw            = spd_tfaw * mtb_psec;


    ddr_print("DDR Clock Rate (tCLK)                         : %6d ps\n", tclk_psecs);
    ddr_print("Core Clock Rate (eCLK)                        : %6d ps\n", eclk_psecs);
    ddr_print("Medium Timebase (MTB)                         : %6d ps\n", mtb_psec);
    ddr_print("Minimum Cycle Time (tCKmin)                   : %6d ps\n", tCKmin);
    ddr_print("Minimum CAS Latency Time (tAAmin)             : %6d ps\n", tAAmin);
    ddr_print("Write Recovery Time (tWR)                     : %6d ps\n", twr);
    ddr_print("Minimum RAS to CAS delay (tRCD)               : %6d ps\n", trcd);
    ddr_print("Minimum Row Active to Row Active delay (tRRD) : %6d ps\n", trrd);
    ddr_print("Minimum Row Precharge Delay (tRP)             : %6d ps\n", trp);
    ddr_print("Minimum Active to Precharge (tRAS)            : %6d ps\n", tras);
    ddr_print("Minimum Active to Active/Refresh Delay (tRC)  : %6d ps\n", trc);
    ddr_print("Minimum Refresh Recovery Delay (tRFC)         : %6d ps\n", trfc);
    ddr_print("Internal write to read command delay (tWTR)   : %6d ps\n", twtr);
    ddr_print("Min Internal Rd to Precharge Cmd Delay (tRTP) : %6d ps\n", trtp);
    ddr_print("Minimum Four Activate Window Delay (tFAW)     : %6d ps\n", tfaw);


    if ((num_banks != 4) && (num_banks != 8))
    {
        error_print("Unsupported number of banks %d. Must be 4 or 8.\n", num_banks);
        ++fatal_error;
    }

    if ((num_ranks < 1) || (num_ranks > 2))
    {
        error_print("Unsupported number of ranks: %d\n", num_ranks);
        ++fatal_error;
    }

    if ((dram_width != 8) && (dram_width != 16))
    {
        error_print("Unsupported SDRAM Width, %d.  Must be 8 or 16.\n", dram_width);
        ++fatal_error;
    }


    /*
    ** Bail out here if things are not copasetic.
    */
    if (fatal_error)
        return(-1);

    odt_idx = min(dimm_count - 1, 3);
    odt_config = bunk_enable ? odt_2rank_config : odt_1rank_config;


    /* Parameters from DDR3 Specifications */
#define DDR3_tREFI         7800000    /* 7.8 us */
#define DDR3_ZQCS          80000      /* 80 ns */
#define DDR3_ZQCS_Interval 1280000000 /* 128ms/100 */
#define DDR3_tCKE          5000       /* 5 ns */
#define DDR3_tMRD          4          /* 4 nCK */
#define DDR3_tDLLK         512        /* 512 nCK */
#define DDR3_tMPRR         1          /* 1 nCK */
#define DDR3_tWLMRD        40         /* 40 nCK */
#define DDR3_tWLDQSEN      25         /* 25 nCK */

    /*
     * 4.8.5 Early LMC Initialization
     *
     * All of DDR PLL, LMC CK, and LMC DRESET initializations must
     * be completed prior to starting this LMC initialization
     * sequence.
     *
     * 1. Software must ensure there are no pending DRAM transactions.
     *
     * 2. Write LMC0_CONFIG, LMC0_CONTROL, LMC0_TIMING_PARAMS0,
     *    LMC0_TIMING_PARAMS1, LMC0_MODEREG_PARAMS0,
     *    LMC0_MODEREG_PARAMS1, LMC0_DUAL_MEMCFG, LMC0_NXM,
     *    LMC0_WODT_MASK, LMC0_RODT_MASK, LMC0_COMP_CTL2,
     *    LMC0_PHY_CTL, LMC0_DIMM0/1_PARAMS, and LMC0_DIMM_CTL
     *    with appropriate values. All sections in this chapter
     *    may be used to derive proper register settings.
     */

    /* LMC0_CONFIG */
    {
        cvmx_lmcx_config_t lmc_config;
        lmc_config.u64 = cvmx_read_csr(CVMX_LMCX_CONFIG(ddr_interface_num));
        lmc_config.s.init_start      = 0;
        lmc_config.s.ecc_ena         = use_ecc && spd_ecc;
        lmc_config.s.row_lsb         = encode_row_lsb_ddr3(cpu_id, row_lsb, ddr_interface_wide);
        lmc_config.s.pbank_lsb       = encode_pbank_lsb_ddr3(cpu_id, pbank_lsb, ddr_interface_wide);

        lmc_config.s.idlepower       = 0; /* Disabled */

        if ((s = lookup_env_parameter("ddr_idlepower")) != NULL) {
            lmc_config.s.idlepower = simple_strtoul(s, NULL, 0);
        }

        lmc_config.s.forcewrite      = 0; /* Disabled */
        lmc_config.s.ecc_adr         = 1; /* Include memory reference address in the ECC */

        if ((s = lookup_env_parameter("ddr_ecc_adr")) != NULL) {
            lmc_config.s.ecc_adr = simple_strtoul(s, NULL, 0);
        }

        lmc_config.s.reset           = 0;

        /*
         *  Program LMC0_CONFIG[24:18], ref_zqcs_int(6:0) to
         *  RND-DN(tREFI/clkPeriod/512) Program LMC0_CONFIG[36:25],
         *  ref_zqcs_int(18:7) to
         *  RND-DN(ZQCS_Interval/clkPeriod/(512*64)). Note that this
         *  value should always be greater than 32, to account for
         *  resistor calibration delays.
         */
        lmc_config.s.ref_zqcs_int     = ((DDR3_tREFI/tclk_psecs/512) & 0x7f);
        lmc_config.s.ref_zqcs_int    |= ((max(33, (DDR3_ZQCS_Interval/(tclk_psecs/100)/(512*64))) & 0xfff) << 7);

        lmc_config.s.sequence        = 0; /* Set later */
        lmc_config.s.early_dqx       = 0; /* disabled */

        if ((s = lookup_env_parameter("ddr_early_dqx")) != NULL) {
            lmc_config.s.early_dqx = simple_strtoul(s, NULL, 0);
        }

        lmc_config.s.sref_with_dll   = 1; /* enabled for sref */
        lmc_config.s.rank_ena        = bunk_enable;
        lmc_config.s.rankmask        = rank_mask; /* Set later */
        lmc_config.s.mirrmask        = (spd_addr_mirror << 1 | spd_addr_mirror << 3) & rank_mask;
        lmc_config.s.init_status     = 0; /* Set later */


        if ((s = lookup_env_parameter_ull("ddr_config")) != NULL) {
            lmc_config.u64    = simple_strtoull(s, NULL, 0);
        }
        ddr_print("LMC_CONFIG                                    : 0x%016llx\n", lmc_config.u64);
        cvmx_write_csr(CVMX_LMCX_CONFIG(ddr_interface_num), lmc_config.u64);
    }

    /* LMC0_CONTROL */
    {
        cvmx_lmcx_control_t lmc_control;
        lmc_control.u64 = cvmx_read_csr(CVMX_LMCX_CONTROL(ddr_interface_num));
        lmc_control.s.rdimm_ena       = spd_rdimm;
        lmc_control.s.bwcnt           = 0; /* Clear counter later */
        lmc_control.s.ddr2t           = (safe_ddr_flag ? 1 : 0);
        lmc_control.s.pocas           = 0;
        lmc_control.s.fprch2          = 1;
        lmc_control.s.throttle_rd     = safe_ddr_flag ? 1 : 0;
        lmc_control.s.throttle_wr     = safe_ddr_flag ? 1 : 0;
        lmc_control.s.inorder_rd      = safe_ddr_flag ? 1 : 0;
        lmc_control.s.inorder_wr      = safe_ddr_flag ? 1 : 0;
        lmc_control.s.elev_prio_dis   = safe_ddr_flag ? 1 : 0;
        lmc_control.s.nxm_write_en    = 0; /* discards writes to
                                            addresses that don't exist
                                            in the DRAM */
        lmc_control.s.max_write_batch = 8;
        lmc_control.s.xor_bank        = 1;
        lmc_control.s.auto_dclkdis    = 1;
        lmc_control.s.int_zqcs_dis    = 1;
        lmc_control.s.ext_zqcs_dis    = 0;
        lmc_control.s.bprch           = 0;
        lmc_control.s.wodt_bprch      = 0;
        lmc_control.s.rodt_bprch      = 0;

        if ((s = lookup_env_parameter("ddr_xor_bank")) != NULL) {
            lmc_control.s.xor_bank = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_2t")) != NULL) {
            lmc_control.s.ddr2t = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_fprch2")) != NULL) {
            lmc_control.s.fprch2 = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_bprch")) != NULL) {
            lmc_control.s.bprch = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_wodt_bprch")) != NULL) {
            lmc_control.s.wodt_bprch = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_rodt_bprch")) != NULL) {
            lmc_control.s.rodt_bprch = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter_ull("ddr_control")) != NULL) {
            lmc_control.u64    = simple_strtoull(s, NULL, 0);
        }
        ddr_print("LMC_CONTROL                                   : 0x%016llx\n", lmc_control.u64);
        cvmx_write_csr(CVMX_LMCX_CONTROL(ddr_interface_num), lmc_control.u64);
    }

    /* LMC0_TIMING_PARAMS0 */
    {
        unsigned trp_value;
        cvmx_lmcx_timing_params0_t lmc_timing_params0;
        lmc_timing_params0.u64 = cvmx_read_csr(CVMX_LMCX_TIMING_PARAMS0(ddr_interface_num));

        trp_value = divide_roundup(trp, tclk_psecs) 
            + divide_roundup(max(4*tclk_psecs, 7500), tclk_psecs) - 5;

        if (octeon_is_cpuid(OCTEON_CN63XX_PASS1_X)) {
            /* Reserved. Should be written to zero. */
            lmc_timing_params0.cn63xxp1.tckeon   = 0;
        }
        lmc_timing_params0.s.tzqcs    = divide_roundup(max(64*tclk_psecs, DDR3_ZQCS), (16*tclk_psecs));
        lmc_timing_params0.s.tcke     = divide_roundup(DDR3_tCKE, tclk_psecs) - 1;
        lmc_timing_params0.s.txpr     = divide_roundup(max(5*tclk_psecs, trfc+10000), 16*tclk_psecs);
        lmc_timing_params0.s.tmrd     = divide_roundup((DDR3_tMRD*tclk_psecs), tclk_psecs) - 1;
        lmc_timing_params0.s.tmod     = divide_roundup(max(12*tclk_psecs, 15000), tclk_psecs) - 1;
        lmc_timing_params0.s.tdllk    = divide_roundup(DDR3_tDLLK, 256);
        lmc_timing_params0.s.tzqinit  = divide_roundup(max(512*tclk_psecs, 640000), (256*tclk_psecs));
        lmc_timing_params0.s.trp      = trp_value & 0xf;
        lmc_timing_params0.s.tcksre   = divide_roundup(max(5*tclk_psecs, 10000), tclk_psecs) - 1;

        if (!octeon_is_cpuid(OCTEON_CN63XX_PASS1_X)) {
                lmc_timing_params0.s.trp_ext  = (trp_value >> 4) & 1;
        }

        if ((s = lookup_env_parameter_ull("ddr_timing_params0")) != NULL) {
            lmc_timing_params0.u64    = simple_strtoull(s, NULL, 0);
        }
        ddr_print("TIMING_PARAMS0                                : 0x%016llx\n", lmc_timing_params0.u64);
        cvmx_write_csr(CVMX_LMCX_TIMING_PARAMS0(ddr_interface_num), lmc_timing_params0.u64);
    }

    /* LMC0_TIMING_PARAMS1 */
    {
        int txp;
        cvmx_lmcx_timing_params1_t lmc_timing_params1;
        lmc_timing_params1.u64 = cvmx_read_csr(CVMX_LMCX_TIMING_PARAMS1(ddr_interface_num));
        lmc_timing_params1.s.tmprr    = divide_roundup(DDR3_tMPRR*tclk_psecs, tclk_psecs);

        lmc_timing_params1.s.tras     = divide_roundup(tras, tclk_psecs) - 1;
        lmc_timing_params1.s.trcd     = divide_roundup(trcd, tclk_psecs);
        lmc_timing_params1.s.twtr     = divide_roundup(twtr, tclk_psecs) - 1;
        lmc_timing_params1.s.trfc     = divide_roundup(trfc, 8*tclk_psecs);
        lmc_timing_params1.s.trrd     = divide_roundup(trrd, tclk_psecs) - 2;

        /*
        ** tXP = max( 3nCK, 7.5 ns)     DDR3-800   tCLK = 2500 psec
        ** tXP = max( 3nCK, 7.5 ns)     DDR3-1066  tCLK = 1875 psec
        ** tXP = max( 3nCK, 6.0 ns)     DDR3-1333  tCLK = 1500 psec
        ** tXP = max( 3nCK, 6.0 ns)     DDR3-1600  tCLK = 1250 psec
        ** tXP = max( 3nCK, 6.0 ns)     DDR3-1866  tCLK = 1071 psec
        ** tXP = max( 3nCK, 6.0 ns)     DDR3-2133  tCLK =  937 psec
        */
        txp = (tclk_psecs < 1875) ? 6000 : 7500;
        lmc_timing_params1.s.txp      = divide_roundup(max(3*tclk_psecs, txp), tclk_psecs) - 1;

        lmc_timing_params1.s.twlmrd   = divide_roundup(DDR3_tWLMRD*tclk_psecs, 4*tclk_psecs);
        lmc_timing_params1.s.twldqsen = divide_roundup(DDR3_tWLDQSEN*tclk_psecs, 4*tclk_psecs);
        lmc_timing_params1.s.tfaw     = divide_roundup(tfaw, 4*tclk_psecs);
        lmc_timing_params1.s.txpdll   = divide_roundup(max(10*tclk_psecs, 24000), tclk_psecs) - 1;
        if (!octeon_is_cpuid(OCTEON_CN63XX_PASS1_X)) {
                lmc_timing_params1.s.tras_ext = (tras >> 4) & 1;
        }

        if ((s = lookup_env_parameter_ull("ddr_timing_params1")) != NULL) {
            lmc_timing_params1.u64    = simple_strtoull(s, NULL, 0);
        }
        ddr_print("TIMING_PARAMS1                                : 0x%016llx\n", lmc_timing_params1.u64);
        cvmx_write_csr(CVMX_LMCX_TIMING_PARAMS1(ddr_interface_num), lmc_timing_params1.u64);
    }

    /* LMC0_MODEREG_PARAMS0 */
    {
        cvmx_lmcx_modereg_params0_t lmc_modereg_params0;
        int param;

        lmc_modereg_params0.u64 = cvmx_read_csr(CVMX_LMCX_MODEREG_PARAMS0(ddr_interface_num));


        /*
        ** CSR   CWL         CAS write Latency
        ** ===   ===   =================================
        **  0      5   (           tCK(avg) >=   2.5 ns)
        **  1      6   (2.5 ns   > tCK(avg) >= 1.875 ns)
        **  2      7   (1.875 ns > tCK(avg) >= 1.5   ns)
        **  3      8   (1.5 ns   > tCK(avg) >= 1.25  ns)
        **  4      9   (1.25 ns  > tCK(avg) >= 1.07  ns)
        **  5     10   (1.07 ns  > tCK(avg) >= 0.935 ns)
        **  6     11   (0.935 ns > tCK(avg) >= 0.833 ns)
        **  7     12   (0.833 ns > tCK(avg) >= 0.75  ns)
        */

        lmc_modereg_params0.s.cwl     = 0;
        if (tclk_psecs < 2500)
            lmc_modereg_params0.s.cwl = 1;
        if (tclk_psecs < 1875)
            lmc_modereg_params0.s.cwl = 2;
        if (tclk_psecs < 1500)
            lmc_modereg_params0.s.cwl = 3;
        if (tclk_psecs < 1250)
            lmc_modereg_params0.s.cwl = 4;
        if (tclk_psecs < 1070)
            lmc_modereg_params0.s.cwl = 5;
        if (tclk_psecs <  935)
            lmc_modereg_params0.s.cwl = 6;
        if (tclk_psecs <  833)
            lmc_modereg_params0.s.cwl = 7;

        if ((s = lookup_env_parameter("ddr_cwl")) != NULL) {
            lmc_modereg_params0.s.cwl = simple_strtoul(s, NULL, 0) - 5;
        }

        ddr_print("CAS Write Latency                             : %6d\n", lmc_modereg_params0.s.cwl + 5);

        lmc_modereg_params0.s.mprloc  = 0;
        lmc_modereg_params0.s.mpr     = 0;
        lmc_modereg_params0.s.dll     = 0;
        lmc_modereg_params0.s.al      = 0;
        lmc_modereg_params0.s.wlev    = 0; /* Read Only */
        lmc_modereg_params0.s.tdqs    = 0;
        lmc_modereg_params0.s.qoff    = 0;
        lmc_modereg_params0.s.bl      = 0; /* Read Only */

        if ((s = lookup_env_parameter("ddr_cl")) != NULL) {
            CL = simple_strtoul(s, NULL, 0);
            ddr_print("CAS Latency                                   : %6d\n", CL);
        }

        lmc_modereg_params0.s.cl      = 0x2;
        if (CL > 5)
            lmc_modereg_params0.s.cl  = 0x4;
        if (CL > 6)
            lmc_modereg_params0.s.cl  = 0x6;
        if (CL > 7)
            lmc_modereg_params0.s.cl  = 0x8;
        if (CL > 8)
            lmc_modereg_params0.s.cl  = 0xA;
        if (CL > 9)
            lmc_modereg_params0.s.cl  = 0xC;
        if (CL > 10)
            lmc_modereg_params0.s.cl  = 0xE;
        if (CL > 11)
            lmc_modereg_params0.s.cl  = 0x1;
        if (CL > 12)
            lmc_modereg_params0.s.cl  = 0x3;
        if (CL > 13)
            lmc_modereg_params0.s.cl  = 0x5;
        if (CL > 14)
            lmc_modereg_params0.s.cl  = 0x7;
        if (CL > 15)
            lmc_modereg_params0.s.cl  = 0x9;

        lmc_modereg_params0.s.rbt     = 0; /* Read Only. */
        lmc_modereg_params0.s.tm      = 0;
        lmc_modereg_params0.s.dllr    = 0;

        param = divide_roundup(twr, tclk_psecs);
        lmc_modereg_params0.s.wrp     = 1;
        if (param > 5)
            lmc_modereg_params0.s.wrp = 2;
        if (param > 6)
            lmc_modereg_params0.s.wrp = 3;
        if (param > 7)
            lmc_modereg_params0.s.wrp = 4;
        if (param > 8)
            lmc_modereg_params0.s.wrp = 5;
        if (param > 10)
            lmc_modereg_params0.s.wrp = 6;
        if (param > 12)
            lmc_modereg_params0.s.wrp = 7;

        lmc_modereg_params0.s.ppd     = 0;

        if ((s = lookup_env_parameter("ddr_wrp")) != NULL) {
            lmc_modereg_params0.s.wrp = simple_strtoul(s, NULL, 0);
        }

        ddr_print("Write recovery for auto precharge (WRP)       : %6d\n", lmc_modereg_params0.s.wrp);

        if ((s = lookup_env_parameter_ull("ddr_modereg_params0")) != NULL) {
            lmc_modereg_params0.u64    = simple_strtoull(s, NULL, 0);
        }
        ddr_print("MODEREG_PARAMS0                               : 0x%016llx\n", lmc_modereg_params0.u64);
        cvmx_write_csr(CVMX_LMCX_MODEREG_PARAMS0(ddr_interface_num), lmc_modereg_params0.u64);
    }

    /* LMC0_MODEREG_PARAMS1 */
    {
        cvmx_lmcx_modereg_params1_t lmc_modereg_params1;
        lmc_modereg_params1.u64 = odt_config[odt_idx].odt_mask1.u64;
        static const unsigned char rtt_nom_ohms[] = { -1, 60, 120, 40, 20, 30};
        static const unsigned char rtt_wr_ohms[]  = { -1, 60, 120};
        static const unsigned char dic_ohms[]     = { 40, 34 };

        for (i=0; i<4; ++i) {
            char buf[10] = {0};
            uint64_t value;
            sprintf(buf, "ddr_rtt_nom_%1d%1d", !!(i&2), !!(i&1));
            if ((s = lookup_env_parameter(buf)) != NULL) {
                value = simple_strtoul(s, NULL, 0);
                lmc_modereg_params1.u64 &= ~((uint64_t)0x7  << (i*12+9));
                lmc_modereg_params1.u64 |=  ( (value & 0x7) << (i*12+9));
                ddr_rtt_nom_auto = 0;
            }
        }

        if ((s = lookup_env_parameter("ddr_rtt_nom")) != NULL) {
            uint64_t value;
            value = simple_strtoul(s, NULL, 0);
            if (rank_mask == 0xf) { /* 2-Rank, 2-Slot */
                lmc_modereg_params1.s.rtt_nom_01 = value;
                lmc_modereg_params1.s.rtt_nom_11 = value;
            }
            if (rank_mask == 0x5) { /* 1-Rank, 2-Slot */
                lmc_modereg_params1.s.rtt_nom_00 = value;
                lmc_modereg_params1.s.rtt_nom_10 = value;
            }
            if (rank_mask == 0x3) { /* 2-Rank, 1-Slot */
                lmc_modereg_params1.s.rtt_nom_00 = value;
            }
            if (rank_mask == 0x1) { /* 1-Rank, 1-Slot */
                lmc_modereg_params1.s.rtt_nom_00 = value;
            }
            ddr_rtt_nom_auto = 0;
        }

        for (i=0; i<4; ++i) {
            char buf[10] = {0};
            uint64_t value;
            sprintf(buf, "ddr_rtt_wr_%1d%1d", !!(i&2), !!(i&1));
            if ((s = lookup_env_parameter(buf)) != NULL) {
                value = simple_strtoul(s, NULL, 0);
                lmc_modereg_params1.u64 &= ~((uint64_t)0x3  << (i*12+5));
                lmc_modereg_params1.u64 |=  ( (value & 0x3) << (i*12+5));
            }
        }

        for (i=0; i<4; ++i) {
            char buf[10] = {0};
            uint64_t value;
            sprintf(buf, "ddr_dic_%1d%1d", !!(i&2), !!(i&1));
            if ((s = lookup_env_parameter(buf)) != NULL) {
                value = simple_strtoul(s, NULL, 0);
                lmc_modereg_params1.u64 &= ~((uint64_t)0x3  << (i*12+7));
                lmc_modereg_params1.u64 |=  ( (value & 0x3) << (i*12+7));
            }
        }

        if ((s = lookup_env_parameter_ull("ddr_modereg_params1")) != NULL) {
            lmc_modereg_params1.u64    = simple_strtoull(s, NULL, 0);
        }

        ddr_print("RTT_NOM     %3d, %3d, %3d, %3d ohms           :  %x,%x,%x,%x\n",
                  (lmc_modereg_params1.s.rtt_nom_11 == 0) ? 0 : rtt_nom_ohms[lmc_modereg_params1.s.rtt_nom_11],
                  (lmc_modereg_params1.s.rtt_nom_10 == 0) ? 0 : rtt_nom_ohms[lmc_modereg_params1.s.rtt_nom_10],
                  (lmc_modereg_params1.s.rtt_nom_01 == 0) ? 0 : rtt_nom_ohms[lmc_modereg_params1.s.rtt_nom_01],
                  (lmc_modereg_params1.s.rtt_nom_00 == 0) ? 0 : rtt_nom_ohms[lmc_modereg_params1.s.rtt_nom_00],
                  lmc_modereg_params1.s.rtt_nom_11,
                  lmc_modereg_params1.s.rtt_nom_10,
                  lmc_modereg_params1.s.rtt_nom_01,
                  lmc_modereg_params1.s.rtt_nom_00);

        ddr_print("RTT_WR      %3d, %3d, %3d, %3d ohms           :  %x,%x,%x,%x\n",
                  (lmc_modereg_params1.s.rtt_wr_11 == 0) ? 0 : rtt_wr_ohms[lmc_modereg_params1.s.rtt_wr_11],
                  (lmc_modereg_params1.s.rtt_wr_10 == 0) ? 0 : rtt_wr_ohms[lmc_modereg_params1.s.rtt_wr_10],
                  (lmc_modereg_params1.s.rtt_wr_01 == 0) ? 0 : rtt_wr_ohms[lmc_modereg_params1.s.rtt_wr_01],
                  (lmc_modereg_params1.s.rtt_wr_00 == 0) ? 0 : rtt_wr_ohms[lmc_modereg_params1.s.rtt_wr_00],
                  lmc_modereg_params1.s.rtt_wr_11,
                  lmc_modereg_params1.s.rtt_wr_10,
                  lmc_modereg_params1.s.rtt_wr_01,
                  lmc_modereg_params1.s.rtt_wr_00);

        ddr_print("DIC         %3d, %3d, %3d, %3d ohms           :  %x,%x,%x,%x\n",
                  dic_ohms[lmc_modereg_params1.s.dic_11],
                  dic_ohms[lmc_modereg_params1.s.dic_10],
                  dic_ohms[lmc_modereg_params1.s.dic_01],
                  dic_ohms[lmc_modereg_params1.s.dic_00],
                  lmc_modereg_params1.s.dic_11,
                  lmc_modereg_params1.s.dic_10,
                  lmc_modereg_params1.s.dic_01,
                  lmc_modereg_params1.s.dic_00);

        ddr_print("MODEREG_PARAMS1                               : 0x%016llx\n", lmc_modereg_params1.u64);
        cvmx_write_csr(CVMX_LMCX_MODEREG_PARAMS1(ddr_interface_num), lmc_modereg_params1.u64);
    }


#if 0                           /* Deal with dual mem config later */
    /* LMC0_DUAL_MEMCFG */
    {
        cvmx_lmcx_dual_memcfg_t lmc_dual_memcfg;
        lmc_dual_memcfg.u64 = cvmx_read_csr(CVMX_LMCX_DUAL_MEMCFG(ddr_interface_num));
        lmc_dual_memcfg.s.cs_mask           =
        lmc_dual_memcfg.s.row_lsb           =
            if (!octeon_is_cpuid(OCTEON_CN63XX_PASS1_X)) {
                lmc_dual_memcfg.s.bank8             =
        }
        cvmx_write_csr(CVMX_LMCX_DUAL_MEMCFG(ddr_interface_num), lmc_dual_memcfg.u64);
    }
#endif

    /* LMC0_NXM */
    {
        cvmx_lmcx_nxm_t lmc_nxm;
        lmc_nxm.u64 = cvmx_read_csr(CVMX_LMCX_NXM(ddr_interface_num));
        if (rank_mask & 0x1)
            lmc_nxm.cn63xx.mem_msb_d0_r0 = row_lsb + row_bits - 26;
        if (rank_mask & 0x2)
            lmc_nxm.cn63xx.mem_msb_d0_r1 = row_lsb + row_bits - 26;
        if (rank_mask & 0x4)
            lmc_nxm.cn63xx.mem_msb_d1_r0 = row_lsb + row_bits - 26;
        if (rank_mask & 0x8)
            lmc_nxm.cn63xx.mem_msb_d1_r1 = row_lsb + row_bits - 26;
        if ((s = lookup_env_parameter_ull("ddr_nxm")) != NULL) {
            lmc_nxm.u64    = simple_strtoull(s, NULL, 0);
        }
        ddr_print("LMC_NXM                                       : 0x%016llx\n", lmc_nxm.u64);
        cvmx_write_csr(CVMX_LMCX_NXM(ddr_interface_num), lmc_nxm.u64);
    }

    /* LMC0_WODT_MASK */
    {
        cvmx_lmcx_wodt_mask_t lmc_wodt_mask;
        lmc_wodt_mask.u64 = odt_config[odt_idx].odt_mask;

        if ((s = lookup_env_parameter_ull("ddr_wodt_mask")) != NULL) {
            lmc_wodt_mask.u64    = simple_strtoull(s, NULL, 0);
        }

        ddr_print("WODT_MASK                                     : 0x%016llx\n", lmc_wodt_mask.u64);
        cvmx_write_csr(CVMX_LMCX_WODT_MASK(ddr_interface_num), lmc_wodt_mask.u64);
    }

    /* LMC0_RODT_MASK */
    {
        cvmx_lmcx_rodt_mask_t lmc_rodt_mask;
        lmc_rodt_mask.u64 = odt_config[odt_idx].rodt_ctl;

        if ((s = lookup_env_parameter_ull("ddr_rodt_mask")) != NULL) {
            lmc_rodt_mask.u64    = simple_strtoull(s, NULL, 0);
        }

        ddr_print("RODT_MASK                                     : 0x%016llx\n", lmc_rodt_mask.u64);
        cvmx_write_csr(CVMX_LMCX_RODT_MASK(ddr_interface_num), lmc_rodt_mask.u64);
    }

    /* LMC0_COMP_CTL2 */
    {
        cvmx_lmcx_comp_ctl2_t lmc_comp_ctl2;
        static const char *strength_str[] = {"none","24","26.67","30","34.3","40","48","60"};
        lmc_comp_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num));
        lmc_comp_ctl2.s.ntune              = 0;
        lmc_comp_ctl2.s.ptune              = 0;
        lmc_comp_ctl2.s.byp                = 0;
        lmc_comp_ctl2.s.m180               = 0;

        if ((s = lookup_env_parameter_ull("ddr_comp_ctl2")) != NULL) {
            lmc_comp_ctl2.u64    = simple_strtoull(s, NULL, 0);
        }

        ddr_print("COMP_CTL2                                     : 0x%016llx\n", lmc_comp_ctl2.u64);

        ddr_print("CMD_CTL                                       : 0x%x (%s ohms)\n",
                  lmc_comp_ctl2.s.cmd_ctl, strength_str[lmc_comp_ctl2.s.cmd_ctl]);
        ddr_print("CK_CTL                                        : 0x%x (%s ohms)\n",
                  lmc_comp_ctl2.s.ck_ctl, strength_str[lmc_comp_ctl2.s.ck_ctl]);
        ddr_print("DQX_CTL                                       : 0x%x (%s ohms)\n",
                  lmc_comp_ctl2.s.dqx_ctl, strength_str[lmc_comp_ctl2.s.dqx_ctl]);
        ddr_print("DDR__PTUNE/DDR__PTUNE                         : %d/%d\n",
                  lmc_comp_ctl2.s.ddr__ptune, lmc_comp_ctl2.s.ddr__ntune);


        cvmx_write_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num), lmc_comp_ctl2.u64);
    }

    /* LMC0_PHY_CTL */
    {
        cvmx_lmcx_phy_ctl_t lmc_phy_ctl;
        lmc_phy_ctl.u64 = cvmx_read_csr(CVMX_LMCX_PHY_CTL(ddr_interface_num));
        lmc_phy_ctl.s.ts_stagger           = 0;

        if ((s = lookup_env_parameter_ull("ddr_phy_ctl")) != NULL) {
            lmc_phy_ctl.u64    = simple_strtoull(s, NULL, 0);
        }

        ddr_print("PHY_CTL                                       : 0x%016llx\n", lmc_phy_ctl.u64);
        cvmx_write_csr(CVMX_LMCX_PHY_CTL(ddr_interface_num), lmc_phy_ctl.u64);
    }

    /* LMC0_DIMM0_PARAMS */
    if (spd_rdimm) {
        for (didx=0; didx<(unsigned)dimm_count; ++didx) {
        cvmx_lmcx_dimmx_params_t lmc_dimmx_params;
        cvmx_lmcx_dimm_ctl_t lmc_dimm_ctl;
        int dimm = didx;
        lmc_dimmx_params.u64 = cvmx_read_csr(CVMX_LMCX_DIMMX_PARAMS(dimm, ddr_interface_num));
        int rc;

        rc = read_spd(dimm_config_table[didx], 0, 69);
        lmc_dimmx_params.s.rc0         = (rc >> 0) & 0xf;
        lmc_dimmx_params.s.rc1         = (rc >> 4) & 0xf;

        rc = read_spd(dimm_config_table[didx], 0, 70);
        lmc_dimmx_params.s.rc2         = (rc >> 0) & 0xf;
        lmc_dimmx_params.s.rc3         = (rc >> 4) & 0xf;

        rc = read_spd(dimm_config_table[didx], 0, 71);
        lmc_dimmx_params.s.rc4         = (rc >> 0) & 0xf;
        lmc_dimmx_params.s.rc5         = (rc >> 4) & 0xf;

        rc = read_spd(dimm_config_table[didx], 0, 72);
        lmc_dimmx_params.s.rc6         = (rc >> 0) & 0xf;
        lmc_dimmx_params.s.rc7         = (rc >> 4) & 0xf;

        rc = read_spd(dimm_config_table[didx], 0, 73);
        lmc_dimmx_params.s.rc8         = (rc >> 0) & 0xf;
        lmc_dimmx_params.s.rc9         = (rc >> 4) & 0xf;

        rc = read_spd(dimm_config_table[didx], 0, 74);
        lmc_dimmx_params.s.rc10        = (rc >> 0) & 0xf;
        lmc_dimmx_params.s.rc11        = (rc >> 4) & 0xf;

        rc = read_spd(dimm_config_table[didx], 0, 75);
        lmc_dimmx_params.s.rc12        = (rc >> 0) & 0xf;
        lmc_dimmx_params.s.rc13        = (rc >> 4) & 0xf;

        rc = read_spd(dimm_config_table[didx], 0, 76);
        lmc_dimmx_params.s.rc14        = (rc >> 0) & 0xf;
        lmc_dimmx_params.s.rc15        = (rc >> 4) & 0xf;


        if ((s = ddr_getenv_debug("ddr_clk_drive")) != NULL) {
            if (strcmp(s,"light") == 0) {
                lmc_dimmx_params.s.rc5         = 0x0; /* Light Drive */
            }
            if (strcmp(s,"moderate") == 0) {
                lmc_dimmx_params.s.rc5         = 0x5; /* Moderate Drive */
            }
            if (strcmp(s,"strong") == 0) {
                lmc_dimmx_params.s.rc5         = 0xA; /* Strong Drive */
            }
            error_print("Parameter found in environment. ddr_clk_drive = %s\n", s);
        }

        if ((s = ddr_getenv_debug("ddr_cmd_drive")) != NULL) {
            if (strcmp(s,"light") == 0) {
                lmc_dimmx_params.s.rc3         = 0x0; /* Light Drive */
            }
            if (strcmp(s,"moderate") == 0) {
                lmc_dimmx_params.s.rc3         = 0x5; /* Moderate Drive */
            }
            if (strcmp(s,"strong") == 0) {
                lmc_dimmx_params.s.rc3         = 0xA; /* Strong Drive */
            }
            error_print("Parameter found in environment. ddr_cmd_drive = %s\n", s);
        }

        if ((s = ddr_getenv_debug("ddr_ctl_drive")) != NULL) {
            if (strcmp(s,"light") == 0) {
                lmc_dimmx_params.s.rc4         = 0x0; /* Light Drive */
            }
            if (strcmp(s,"moderate") == 0) {
                lmc_dimmx_params.s.rc4         = 0x5; /* Moderate Drive */
            }
            error_print("Parameter found in environment. ddr_ctl_drive = %s\n", s);
        }


        /*
        ** rc10               RDIMM Operating Speed
        ** ====   =========================================================
        **  0                 tclk_psecs > 2500 psec DDR3/DDR3L-800 (default)
        **  1     2500 psec > tclk_psecs > 1875 psec DDR3/DDR3L-1066
        **  2     1875 psec > tclk_psecs > 1500 psec DDR3/DDR3L-1333
        **  3     1500 psec > tclk_psecs > 1250 psec DDR3/DDR3L-1600
        */
        lmc_dimmx_params.s.rc10        = 3;
        if (tclk_psecs >= 1250)
            lmc_dimmx_params.s.rc10    = 3;
        if (tclk_psecs >= 1500)
            lmc_dimmx_params.s.rc10    = 2;
        if (tclk_psecs >= 1875)
            lmc_dimmx_params.s.rc10    = 1;
        if (tclk_psecs >= 2500)
            lmc_dimmx_params.s.rc10    = 0;

        for (i=0; i<16; ++i) {
            char buf[10] = {0};
            uint64_t value;
            sprintf(buf, "ddr_rc%d", i);
            if ((s = lookup_env_parameter(buf)) != NULL) {
                value = simple_strtoul(s, NULL, 0);
                lmc_dimmx_params.u64 &= ~((uint64_t)0xf << (i*4));
                lmc_dimmx_params.u64 |=           (  value << (i*4));
            }
        }

        cvmx_write_csr(CVMX_LMCX_DIMMX_PARAMS(dimm, ddr_interface_num), lmc_dimmx_params.u64);

        ddr_print("DIMM%d Register Control Words         RC15:RC0 : %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",
                  dimm,
                  lmc_dimmx_params.s.rc15,
                  lmc_dimmx_params.s.rc14,
                  lmc_dimmx_params.s.rc13,
                  lmc_dimmx_params.s.rc12,
                  lmc_dimmx_params.s.rc11,
                  lmc_dimmx_params.s.rc10,
                  lmc_dimmx_params.s.rc9 ,
                  lmc_dimmx_params.s.rc8 ,
                  lmc_dimmx_params.s.rc7 ,
                  lmc_dimmx_params.s.rc6 ,
                  lmc_dimmx_params.s.rc5 ,
                  lmc_dimmx_params.s.rc4 ,
                  lmc_dimmx_params.s.rc3 ,
                  lmc_dimmx_params.s.rc2 ,
                  lmc_dimmx_params.s.rc1 ,
                  lmc_dimmx_params.s.rc0 );


        /* LMC0_DIMM_CTL */
        lmc_dimm_ctl.u64 = cvmx_read_csr(CVMX_LMCX_DIMM_CTL(ddr_interface_num));
        lmc_dimm_ctl.s.dimm0_wmask         = 0xffff;
        lmc_dimm_ctl.s.dimm1_wmask         = 0xffff;
        lmc_dimm_ctl.s.tcws                = 0x4e0;
        lmc_dimm_ctl.s.parity              = 0;

        if ((s = lookup_env_parameter("ddr_dimm_wmask")) != NULL) {
            lmc_dimm_ctl.s.dimm0_wmask    = simple_strtoul(s, NULL, 0);
            lmc_dimm_ctl.s.dimm1_wmask    = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_dimm_ctl_parity")) != NULL) {
            lmc_dimm_ctl.s.parity = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_dimm_ctl_tcws")) != NULL) {
            lmc_dimm_ctl.s.tcws = simple_strtoul(s, NULL, 0);
        }

        ddr_print("DIMM%d DIMM_CTL                                : 0x%016llx\n", dimm, lmc_dimm_ctl.u64);
        cvmx_write_csr(CVMX_LMCX_DIMM_CTL(ddr_interface_num), lmc_dimm_ctl.u64);
    }
    } else {
        /* Disable register control writes for unbuffered */
        cvmx_lmcx_dimm_ctl_t lmc_dimm_ctl;
        lmc_dimm_ctl.u64 = cvmx_read_csr(CVMX_LMCX_DIMM_CTL(ddr_interface_num));
        lmc_dimm_ctl.s.dimm0_wmask         = 0;
        lmc_dimm_ctl.s.dimm1_wmask         = 0;
        cvmx_write_csr(CVMX_LMCX_DIMM_CTL(ddr_interface_num), lmc_dimm_ctl.u64);
    }


    /*
     * 3. Without changing any other fields in LMC0_CONFIG, write
     *    LMC0_CONFIG[RANKMASK,INIT_STATUS,SEQUENCE] while writing
     *    LMC0_CONFIG[INIT_START] = 1, all with a single CSR
     *    write. Both LMC0_CONFIG[RANKMASK,INIT_STATUS] bits
     *    should be set to the same mask value indicating the
     *    ranks that will participate in the sequence.
     *
     *    The SEQUENCE value should select power-up/init or
     *    self-refresh exit, depending on whether the DRAM parts
     *    are in self-refresh and whether their contents should be
     *    preserved. While LMC performs these sequences, it will
     *    not perform any other DDR3 transactions.
     *
     *    If power-up/init is selected immediately following a
     *    DRESET assertion, LMC executes the sequence described in
     *    the Reset and Initialization Procedure section of the
     *    JEDEC DDR3 specification. This includes activating CKE,
     *    writing all four DDR3 mode registers on all selected
     *    ranks, and issuing the required ZQCL command. The
     *    RANKMASK value should select all ranks with attached
     *    DRAM in this case. If LMC0_CONTROL[RDIMM_ENA]=1, LMC
     *    writes the JEDEC standard SSTE32882 control words
     *    selected by LMC0_DIMM_CTL [DIMM*_WMASK] between DDR_CKE*
     *    signal assertion and the first DDR3 mode register write.
     */

    perform_ddr3_init_sequence(cpu_id, rank_mask, ddr_interface_num);

    /*
     *    If self-refresh exit is selected, LMC executes the
     *    required SRX command followed by a refresh and ZQ
     *    calibration. Section 4.5 describes behavior of a REF +
     *    ZQCS.  LMC does not write the DDR3 mode registers as
     *    part of this sequence, and the mode register parameters
     *    must match at self-refresh entry and exit times. The
     *    self-refresh exit sequence does not use LMC0_CONFIG
     *    [RANKMASK]. If the self-refresh exit is preceded by a
     *    self-refresh exit SEQUENCE without an intervening DRESET
     *    sequence, then LMC0_CONFIG [INIT_STATUS] will already
     *    indicate the ranks that should participate, and software
     *    need not assert any LMC0_CONFIG[INIT_STATUS]
     *    bits. Otherwise, the LMC0_CONFIG CSR write that
     *    initiates the self-refresh exit must also set
     *    LMC0_CONFIG[INIT_STATUS] bits for all ranks with
     *    attached DRAM.
     *
     * 4. Read LMC0_CONFIG and wait for the result.
     */

        cvmx_read_csr(CVMX_LMCX_CONFIG(ddr_interface_num)); /* Read CVMX_LMCX_CONFIG */
        cvmx_wait_usec(1000);   /* Wait a while. */

    /*
     * 4.8.6 LMC Write Leveling
     *
     * LMC supports an automatic write-leveling like that
     * described in the JEDEC DDR3 specifications separately per
     * byte-lane.
     *
     * All of DDR PLL, LMC CK, and LMC DRESET, and early LMC
     * initializations must be completed prior to starting this
     * LMC write-leveling sequence.
     *
     * There are many possible procedures that will write-level
     * all the attached DDR3 DRAM parts. One possibility is for
     * software to simply write the desired values into
     * LMC0_WLEVEL_RANK(0..3). This section describes one possible
     * sequence that uses LMCs auto-write-leveling capabilities.
     */

    {
        cvmx_lmcx_wlevel_ctl_t wlevel_ctl;
        cvmx_lmcx_wlevel_rankx_t lmc_wlevel_rank;
        cvmx_lmcx_config_t lmc_config;
        int rankx = 0;
        int wlevel_bitmask[9];
        int byte_idx;
        int passx;
        int ecc_ena;
        int ddr_wlevel_roundup = 0;

        if (wlevel_loops)
            ddr_print("Performing Write-Leveling\n");
        else
            wlevel_bitmask_errors = 1; /* Force software write-leveling to run */

        while(wlevel_loops--) {
        if ((s = lookup_env_parameter("ddr_wlevel_roundup")) != NULL) {
            ddr_wlevel_roundup = simple_strtoul(s, NULL, 0);
        }
        for (rankx = 0; rankx < dimm_count * 2;rankx++) {
            if (!(rank_mask & (1 << rankx)))
                continue;

            wlevel_ctl.u64 = cvmx_read_csr(CVMX_LMCX_WLEVEL_CTL(ddr_interface_num));
            lmc_config.u64 = cvmx_read_csr(CVMX_LMCX_CONFIG(ddr_interface_num));
            ecc_ena = lmc_config.s.ecc_ena;
          
            if (!octeon_is_cpuid(OCTEON_CN63XX_PASS1_X)) {
                if ((s = lookup_env_parameter("ddr_wlevel_rtt_nom")) != NULL) {
                    wlevel_ctl.s.rtt_nom   = simple_strtoul(s, NULL, 0);
                }
            }

            cvmx_write_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num), 0); /* Clear write-level delays */
    
            wlevel_bitmask_errors = 0; /* Reset error counter */

            for (byte_idx=0; byte_idx<(8+ecc_ena); ++byte_idx) {
                wlevel_bitmask[byte_idx] = 0; /* Reset bitmasks */
            }

            /* Make two passes to accomodate some x16 parts that
               require it.  Others don't care. */
            for (passx=0; passx<2; ++passx) {
                wlevel_ctl.s.lanemask = 0x155 >> passx; /* lanemask = 0x155, 0x0AA */
    
                cvmx_write_csr(CVMX_LMCX_WLEVEL_CTL(ddr_interface_num), wlevel_ctl.u64);
    
                /* Read and write values back in order to update the
                   status field. This insurs that we read the updated
                   values after write-leveling has completed. */
                cvmx_write_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num),
                               cvmx_read_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num)));
    
                lmc_config.s.sequence        = 6; /* write-leveling */
                lmc_config.s.rankmask        = 1 << rankx; /* FIXME: hardwire 1 rank for now */
                lmc_config.s.init_start      = 1;

                cvmx_write_csr(CVMX_LMCX_CONFIG(ddr_interface_num), lmc_config.u64);
    
                do {
                    lmc_wlevel_rank.u64 = cvmx_read_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num));
                } while (lmc_wlevel_rank.cn63xx.status != 3);
    
                for (byte_idx=passx; byte_idx<(8+ecc_ena); byte_idx+=2) {
                    wlevel_bitmask[byte_idx] = octeon_read_lmcx_ddr3_wlevel_dbg(ddr_interface_num, byte_idx);
                    if (wlevel_bitmask[byte_idx] == 0)
                        ++wlevel_bitmask_errors;
                }
    
                ddr_print("Rank(%d) Wlevel Debug Results   (lanemask %03X) : %05x %05x %05x %05x %05x %05x %05x %05x %05x\n",
                          rankx,
                          wlevel_ctl.s.lanemask,
                          wlevel_bitmask[8],
                          wlevel_bitmask[7],
                          wlevel_bitmask[6],
                          wlevel_bitmask[5],
                          wlevel_bitmask[4],
                          wlevel_bitmask[3],
                          wlevel_bitmask[2],
                          wlevel_bitmask[1],
                          wlevel_bitmask[0]
                          );
    
                ddr_print("Rank(%d) Wlevel Rank %#5x, 0x%016llX : %5d %5d %5d %5d %5d %5d %5d %5d %5d\n",
                          rankx,
                          lmc_wlevel_rank.cn63xx.status,
                          lmc_wlevel_rank.u64,
                          lmc_wlevel_rank.cn63xx.byte8,
                          lmc_wlevel_rank.cn63xx.byte7,
                          lmc_wlevel_rank.cn63xx.byte6,
                          lmc_wlevel_rank.cn63xx.byte5,
                          lmc_wlevel_rank.cn63xx.byte4,
                          lmc_wlevel_rank.cn63xx.byte3,
                          lmc_wlevel_rank.cn63xx.byte2,
                          lmc_wlevel_rank.cn63xx.byte1,
                          lmc_wlevel_rank.cn63xx.byte0
                          );
            }
    
            if (ddr_wlevel_roundup) { /* Round up odd bitmask delays */
                for (byte_idx=0; byte_idx<(8+ecc_ena); ++byte_idx) {
                    update_wlevel_rank_struct(&lmc_wlevel_rank,
                                           byte_idx,
                                           roundup_ddr3_wlevel_bitmask(wlevel_bitmask[byte_idx]));
                }
                cvmx_write_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num), lmc_wlevel_rank.u64);
                ddr_print("Rank(%d) Wlevel Rank %#5x, 0x%016llX : %5d %5d %5d %5d %5d %5d %5d %5d %5d\n",
                          rankx,
                          lmc_wlevel_rank.cn63xx.status,
                          lmc_wlevel_rank.u64,
                          lmc_wlevel_rank.cn63xx.byte8,
                          lmc_wlevel_rank.cn63xx.byte7,
                          lmc_wlevel_rank.cn63xx.byte6,
                          lmc_wlevel_rank.cn63xx.byte5,
                          lmc_wlevel_rank.cn63xx.byte4,
                          lmc_wlevel_rank.cn63xx.byte3,
                          lmc_wlevel_rank.cn63xx.byte2,
                          lmc_wlevel_rank.cn63xx.byte1,
                          lmc_wlevel_rank.cn63xx.byte0
                          );
            }

            if (wlevel_bitmask_errors != 0) {
                ddr_print("Rank(%d) Write-Leveling Failed: %d Bitmask errors\n", rankx, wlevel_bitmask_errors);
            }
        }
        } /* while(wlevel_loops--) */
    }

    /*
     * 1. If the DQS/DQ delays on the board may be more than the
     *    ADD/CMD delays, then ensure that LMC0_CONFIG[EARLY_DQX]
     *    is set at this point.
     *
     * Do the remaining steps 2-7 separately for each rank i with
     * attached DRAM.
     *
     * 2. Write LMC0_WLEVEL_RANKi = 0.
     *
     * 3. For 8 parts:
     *
     *    Without changing any other fields in LMC0_WLEVEL_CTL,
     *    write LMC0_WLEVEL_CTL[LANEMASK] to select all byte lanes
     *    with attached DRAM.
     *
     *    For 16 parts:
     *
     *    Without changing any other fields in LMC0_WLEVEL_CTL,
     *    write LMC0_WLEVEL_CTL[LANEMASK] to select all even byte
     *    lanes with attached DRAM.
     *
     * 4. Without changing any other fields in LMC0_CONFIG,
     *
     *    o write LMC0_CONFIG[SEQUENCE] to select write-leveling, and
     *
     *    o write LMC0_CONFIG[RANKMASK] = (1 << i), and
     *
     *    o write LMC0_CONFIG[INIT_START] = 1
     *
     *    LMC will initiate write-leveling at this point. Assuming
     *    LMC0_WLEVEL_CTL [SSET] = 0, LMC first enables
     *    write-leveling on the selected DRAM rank via a DDR3 MR1
     *    write, then sequences through and accumulates
     *    write-leveling results for 8 different delay settings
     *    twice, starting at a delay of zero in this case since
     *    LMC0_WLEVEL_RANKi[BYTE*<4:3>] = 0, increasing by 1/8 CK
     *    each setting, covering a total distance of one CK, then
     *    disables the write-leveling via another DDR3 MR1 write.
     *
     *    After the sequence through 16 delay settings is complete:
     *
     *    o LMC sets LMC0_WLEVEL_RANKi[STATUS]=3
     *
     *    o LMC sets LMC0_WLEVEL_RANKi[BYTE*<2:0>] (for all ranks
     *      selected by LANEMASK) to indicate the first write
     *      leveling result of 1 that followed result of 0 during
     *      the sequence, except that LMC always writes
     *      LMC0_WLEVEL_RANKi[BYTE*<0>]=0.
     *
     *    o Software can read the 8 write-leveling results from
     *      the first pass through the delay settings by reading
     *      LMC0_WLEVEL_DBG[BITMASK] (after writing
     *      LMC0_WLEVEL_DBG[BYTE]). (LMC does not retain the
     *      write-leveling results from the second pass through
     *      the 8 delay settings. They should often be identical
     *      to the LMC0_WLEVEL_DBG[BITMASK] results, though.)
     *
     * 5. Wait until LMC0_WLEVEL_RANKi[STATUS] != 2.
     *
     *    LMC will have updated LMC0_WLEVEL_RANKi[BYTE*<2:0>] for
     *    all byte lanes selected by LMC0_WLEVEL_CTL[LANEMASK] at
     *    this point.  LMC0_WLEVEL_RANKi[BYTE*<4:3>] will still be
     *    the value that software wrote in step 2 above, which is
     *    zero.
     *
     * 6. For 16 parts:
     *
     *    Without changing any other fields in LMC0_WLEVEL_CTL,
     *    write LMC0_WLEVEL_CTL[LANEMASK] to select all odd byte
     *    lanes with attached DRAM.
     *
     *    Repeat steps 4 and 5 with this new
     *    LMC0_WLEVEL_CTL[LANEMASK] setting. Skip to step 7 if
     *    this has already been done.
     *
     *    For 8 parts:
     *
     *    Skip this step. Go to step 7.
     *
     * 7. Calculate LMC0_WLEVEL_RANKi[BYTE*<4:3>] settings for all
     *    byte lanes on all ranks with attached DRAM.
     *
     *    At this point, all byte lanes on rank i with attached
     *    DRAM should have been write-leveled, and
     *    LMC0_WLEVEL_RANKi[BYTE*<2:0>] has the result for each
     *    byte lane.
     *
     *    But note that the DDR3 write-leveling sequence will only
     *    determine the delay modulo the CK cycle time, and cannot
     *    determine how many additional CKs of delay are
     *    present. Software must calculate the number of CKs, or
     *    equivalently, the LMC0_WLEVEL_RANKi[BYTE*<4:3>]
     *    settings.
     *
     *    This BYTE*<4:3> calculation is system/board specific.
     *
     *    Many techniques may be used to calculate write-leveling
     *    BYTE*<4:3> values, including:
     *
     *    o Known values for some byte lanes.
     *
     *    o Relative values for some byte lanes relative to others.
     *
     *    For example, suppose lane X is likely to require a
     *    larger write-leveling delay than lane Y. A BYTEX<2:0>
     *    value that is much smaller than the BYTEY<2:0> value may
     *    then indicate that the required lane X delay wrapped
     *    into the next CK, so BYTEX<4:3> should be set to
     *    BYTEY<4:3>+1.
     *
     * 8. Initialize LMC0_WLEVEL_RANK* values for all unused ranks.
     *
     *    Let rank i be a rank with attached DRAM.
     *
     *    For all ranks j that do not have attached DRAM, set
     *    LMC0_WLEVEL_RANKj = LMC0_WLEVEL_RANKi.
     *
     * 4.8.7 LMC Read Leveling
     *
     * LMC supports an automatic read-leveling separately per
     * byte-lane using the DDR3 multi-purpose register pre-defined
     * pattern for system calibration defined in the JEDEC DDR3
     * specifications.
     *
     * All of DDR PLL, LMC CK, and LMC DRESET, and early LMC
     * initializations must be completed prior to starting this
     * LMC read-leveling sequence.
     *
     * Software could simply write the desired read-leveling
     * values into LMC0_RLEVEL_RANK(0..3). This section describes
     * a sequence that uses LMCs auto-read-leveling capabilities.
     *
     * When LMC does the read-leveling sequence for a rank, it
     * first enables the DDR3 multi-purpose register predefined
     * pattern for system calibration on the selected DRAM rank
     * via a DDR3 MR3 write, then executes 64 RD operations at
     * different internal delay settings, then disables the
     * predefined pattern via another DDR3 MR3 write. LMC
     * determines the pass or fail of each of the 64 settings
     * independently for each byte lane, then writes appropriate
     * LMC0_RLEVEL_RANK(0..3)[BYTE*] values for the rank.
     *
     * After read-leveling for a rank, software can read the 64
     * pass/fail indications for one byte lane via
     * LMC0_RLEVEL_DBG[BITMASK]. Software can observe all
     * pass/fail results for all byte lanes in a rank via separate
     * read-leveling sequences on the rank with different
     * LMC0_RLEVEL_CTL[BYTE] values.
     *
     * The 64 pass/fail results will typically have failures for
     * the low delays, followed by a run of some passing settings,
     * followed by more failures in the remaining high delays.
     * LMC sets LMC0_RLEVEL_RANK(0..3)[BYTE*] to one of the
     * passing settings. First, LMC selects the longest run of
     * successes in the 64 results. (In the unlikely event that
     * there is more than one longest run, LMC selects the first
     * one.) Then if LMC0_RLEVEL_CTL[OFFSET_EN] = 1 and the
     * selected run has more than LMC0_RLEVEL_CTL[OFFSET]
     * successes, LMC selects the last passing setting in the run
     * minus LMC0_RLEVEL_CTL[OFFSET]. Otherwise LMC selects the
     * middle setting in the run (rounding earlier when
     * necessary). We expect the read-leveling sequence to produce
     * good results with the reset values LMC0_RLEVEL_CTL
     * [OFFSET_EN]=1, LMC0_RLEVEL_CTL[OFFSET] = 2.
     *
     * The read-leveling sequence:
     *
     * 1. Select desired LMC0_RLEVEL_CTL[OFFSET_EN,OFFSET,BYTE]
     *    settings.  Do the remaining steps 2V4 separately for
     *    each rank i with attached DRAM.
     */

    {
        cvmx_lmcx_rlevel_rankx_t lmc_rlevel_rank;
        cvmx_lmcx_config_t lmc_config;
        cvmx_lmcx_comp_ctl2_t lmc_comp_ctl2;
        cvmx_lmcx_rlevel_ctl_t rlevel_ctl;
        cvmx_lmcx_control_t lmc_control;
        cvmx_lmcx_modereg_params1_t lmc_modereg_params1;
        unsigned char rodt_ctl;
        unsigned char rankx = 0;
        int rlevel_rodt_errors = 0;
        unsigned char ecc_ena;
        unsigned char rtt_nom;
        unsigned char rtt_idx;
        unsigned char default_rtt_nom_00=0, default_rtt_nom_01=0, default_rtt_nom_10=0, default_rtt_nom_11=0;
        int min_rtt_nom_idx;
        int max_rtt_nom_idx;
        int min_rodt_ctl;
        int max_rodt_ctl;
        static const unsigned char rodt_ohms[] = { -1, 20, 30, 40, 60, 120 };
        static const unsigned char rtt_nom_ohms[] = { -1, 60, 120, 40, 20, 30 };
        static const unsigned char rtt_nom_table[] = { -1, 2, 1, 3, 5, 4 };
        static const unsigned char rtt_wr_ohms[]  = { -1, 60, 120 };
        static const unsigned char dic_ohms[]     = { 40, 34 };
        int rlevel_loops = 1;
        int default_rodt_ctl;
        unsigned char save_ddr2t;
        int rlevel_avg_loops = 1;
        int ddr_rlevel_compute = 1;
        int saved_ddr__ptune, saved_ddr__ntune, rlevel_comp_offset;
        struct {
            uint64_t setting;
            int      score;
        } rlevel_scoreboard[sizeof(rtt_nom_ohms)][sizeof(rodt_ohms)][4];

        default_rodt_ctl = odt_config[odt_idx].qs_dic;

        lmc_control.u64 = cvmx_read_csr(CVMX_LMCX_CONTROL(ddr_interface_num));
        save_ddr2t                    = lmc_control.s.ddr2t;

        if ((save_ddr2t == 1) && octeon_is_cpuid(OCTEON_CN63XX_PASS1_X)) {
        /* During the read-leveling sequence with registered DIMMs
           when 2T mode is enabled (i.e. LMC0_CONTROL[DDR2T]=1), some
           register parts do not like the sequence that LMC generates
           for an MRS register write. (14656) */
            ddr_print("Disabling DDR 2T during read-leveling seq. Re: Pass 1 LMC-14656\n");
            lmc_control.s.ddr2t           = 0;
        }

        if ((s = lookup_env_parameter("ddr_rlevel_2t")) != NULL) {
            lmc_control.s.ddr2t = simple_strtoul(s, NULL, 0);
        }

        cvmx_write_csr(CVMX_LMCX_CONTROL(ddr_interface_num), lmc_control.u64);

        ddr_print("Performing Read-Leveling\n");

        rlevel_ctl.u64 = cvmx_read_csr(CVMX_LMCX_RLEVEL_CTL(ddr_interface_num));


        rlevel_ctl.s.offset_en = custom_lmc_config->offset_en;

        if (rlevel_ctl.s.offset_en)
            rlevel_ctl.s.offset    = 2 - spd_rdimm; /* Needed for some registered dimms when offset enabled. */

        rlevel_ctl.s.offset = spd_rdimm ?
            custom_lmc_config->offset_rdimm : custom_lmc_config->offset_udimm;
        ddr_rlevel_compute = custom_lmc_config->rlevel_compute;

        ddr_rtt_nom_auto = custom_lmc_config->ddr_rtt_nom_auto;
        ddr_rodt_ctl_auto = custom_lmc_config->ddr_rodt_ctl_auto;
        rlevel_comp_offset = (custom_lmc_config->rlevel_comp_offset == 0) ? 2 : custom_lmc_config->rlevel_comp_offset;

        if ((s = lookup_env_parameter("ddr_rlevel_offset")) != NULL) {
            rlevel_ctl.s.offset   = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_rlevel_offset_en")) != NULL) {
            rlevel_ctl.s.offset_en   = simple_strtoul(s, NULL, 0);
        }
        cvmx_write_csr(CVMX_LMCX_RLEVEL_CTL(ddr_interface_num), rlevel_ctl.u64);

        if ((s = lookup_env_parameter("ddr_rlevel_loops")) != NULL) {
            rlevel_loops = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_rtt_nom_auto")) != NULL) {
            ddr_rtt_nom_auto = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_rodt_ctl")) != NULL) {
            default_rodt_ctl    = simple_strtoul(s, NULL, 0);
            ddr_rodt_ctl_auto = 0;
        }

        if ((s = lookup_env_parameter("ddr_rodt_ctl_auto")) != NULL) {
            ddr_rodt_ctl_auto = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_rlevel_average")) != NULL) {
            rlevel_avg_loops = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_rlevel_compute")) != NULL) {
            ddr_rlevel_compute = simple_strtoul(s, NULL, 0);
        }

        ddr_print("RLEVEL_CTL                                    : 0x%016llx\n", rlevel_ctl.u64);
        ddr_print("RLEVEL_OFFSET                                 : %6d\n", rlevel_ctl.s.offset);
        ddr_print("RLEVEL_OFFSET_EN                              : %6d\n", rlevel_ctl.s.offset_en);
        ddr_print("RLEVEL_COMPUTE                                : %6d\n", ddr_rlevel_compute);

        /* The purpose for the indexed table is to sort the settings
        ** by the ohm value to simplify the testing when incrementing
        ** through the settings.  (index => ohms) 1=120, 2=60, 3=40,
        ** 4=30, 5=20 */
        min_rtt_nom_idx = (custom_lmc_config->min_rtt_nom_idx == 0) ? 1 : custom_lmc_config->min_rtt_nom_idx;
        max_rtt_nom_idx = (custom_lmc_config->max_rtt_nom_idx == 0) ? 5 : custom_lmc_config->max_rtt_nom_idx;

        min_rodt_ctl = (custom_lmc_config->min_rodt_ctl == 0) ? 1 : custom_lmc_config->min_rodt_ctl;
        max_rodt_ctl = (custom_lmc_config->max_rodt_ctl == 0) ? 5 : custom_lmc_config->max_rodt_ctl;

        while(rlevel_loops--) {
        /* Initialize the error scoreboard */
        for ( rtt_idx=1; rtt_idx<6; ++rtt_idx) {
            rtt_nom = rtt_nom_table[rtt_idx];
        for (rodt_ctl=5; rodt_ctl>0; --rodt_ctl) {
            for (rankx = 0; rankx < dimm_count*2; rankx++) {
                if (!(rank_mask & (1 << rankx)))
                    continue;
                rlevel_scoreboard[rtt_nom][rodt_ctl][rankx].score   = RLEVEL_UNDEFINED_ERROR;
                rlevel_scoreboard[rtt_nom][rodt_ctl][rankx].setting = 0;
            }
        }
        }

        if ((s = lookup_env_parameter("ddr_rlevel_comp_offset")) != NULL) {
            rlevel_comp_offset = simple_strtoul(s, NULL, 0);
        }

        lmc_comp_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num));
        saved_ddr__ptune = lmc_comp_ctl2.s.ddr__ptune;
        saved_ddr__ntune = lmc_comp_ctl2.s.ddr__ntune;

        /* Disable dynamic compensation settings */
        if (rlevel_comp_offset != 0) {
            lmc_comp_ctl2.s.ptune = saved_ddr__ptune;
            lmc_comp_ctl2.s.ntune = saved_ddr__ntune;

            lmc_comp_ctl2.s.ptune += rlevel_comp_offset;

            lmc_comp_ctl2.s.byp = 1; /* Enable bypass mode */
            cvmx_write_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num), lmc_comp_ctl2.u64);
            cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num));
        }

        lmc_modereg_params1.u64 = cvmx_read_csr(CVMX_LMCX_MODEREG_PARAMS1(ddr_interface_num));

        /* Save the original settings before sweeping through settings. */
        default_rtt_nom_00 = lmc_modereg_params1.s.rtt_nom_00;
        default_rtt_nom_01 = lmc_modereg_params1.s.rtt_nom_01;
        default_rtt_nom_10 = lmc_modereg_params1.s.rtt_nom_10;
        default_rtt_nom_11 = lmc_modereg_params1.s.rtt_nom_11;

        for (rtt_idx=min_rtt_nom_idx; rtt_idx<=max_rtt_nom_idx; ++rtt_idx) {
            rtt_nom = rtt_nom_table[rtt_idx];

            if (rank_mask == 0xf) { /* 2-Rank, 2-Slot */
                lmc_modereg_params1.s.rtt_nom_01 = rtt_nom;
                lmc_modereg_params1.s.rtt_nom_11 = rtt_nom;
            }
            if (rank_mask == 0x5) { /* 1-Rank, 2-Slot */
                lmc_modereg_params1.s.rtt_nom_00 = rtt_nom;
                lmc_modereg_params1.s.rtt_nom_10 = rtt_nom;
            }
            if (rank_mask == 0x3) { /* 2-Rank, 1-Slot */
                lmc_modereg_params1.s.rtt_nom_00 = rtt_nom;
            }
            if (rank_mask == 0x1) { /* 1-Rank, 1-Slot */
                lmc_modereg_params1.s.rtt_nom_00 = rtt_nom;
            }

            cvmx_write_csr(CVMX_LMCX_MODEREG_PARAMS1(ddr_interface_num), lmc_modereg_params1.u64);
            ddr_print("\n");
            ddr_print("RTT_NOM     %3d, %3d, %3d, %3d ohms           :  %x,%x,%x,%x\n",
                      (lmc_modereg_params1.s.rtt_nom_11 == 0) ? 0 : rtt_nom_ohms[lmc_modereg_params1.s.rtt_nom_11],
                      (lmc_modereg_params1.s.rtt_nom_10 == 0) ? 0 : rtt_nom_ohms[lmc_modereg_params1.s.rtt_nom_10],
                      (lmc_modereg_params1.s.rtt_nom_01 == 0) ? 0 : rtt_nom_ohms[lmc_modereg_params1.s.rtt_nom_01],
                      (lmc_modereg_params1.s.rtt_nom_00 == 0) ? 0 : rtt_nom_ohms[lmc_modereg_params1.s.rtt_nom_00],
                      lmc_modereg_params1.s.rtt_nom_11,
                      lmc_modereg_params1.s.rtt_nom_10,
                      lmc_modereg_params1.s.rtt_nom_01,
                      lmc_modereg_params1.s.rtt_nom_00);

            perform_ddr3_init_sequence(cpu_id, rank_mask, ddr_interface_num);

        for (rodt_ctl=max_rodt_ctl; rodt_ctl>=min_rodt_ctl; --rodt_ctl) {
            rlevel_rodt_errors = 0;
            lmc_comp_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num));
            lmc_comp_ctl2.s.rodt_ctl = rodt_ctl;
            cvmx_write_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num), lmc_comp_ctl2.u64);
            lmc_comp_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num));
            ddr_print("Read ODT_CTL                                  : 0x%x (%d ohms)\n",
                      lmc_comp_ctl2.s.rodt_ctl, rodt_ohms[lmc_comp_ctl2.s.rodt_ctl]);
    
            for (rankx = 0; rankx < dimm_count*2; rankx++) {
                int rlevel_byte[9] = {0};
                int byte_idx;
                int rlevel_rank_errors;
                if (!(rank_mask & (1 << rankx)))
                    continue;

                lmc_config.u64 = cvmx_read_csr(CVMX_LMCX_CONFIG(ddr_interface_num));
                ecc_ena = lmc_config.s.ecc_ena;
    
                int average_loops = rlevel_avg_loops;
                while (average_loops--) {
                    rlevel_rank_errors = 0;

                    /* Clear read-level delays */
                    cvmx_write_csr(CVMX_LMCX_RLEVEL_RANKX(rankx, ddr_interface_num), 0);
    
                    lmc_config.s.sequence        = 1; /* read-leveling */
                    lmc_config.s.rankmask        = 1 << rankx;
                    lmc_config.s.init_start      = 1;

                    cvmx_write_csr(CVMX_LMCX_CONFIG(ddr_interface_num), lmc_config.u64);
    
                    do {
                        lmc_rlevel_rank.u64 = cvmx_read_csr(CVMX_LMCX_RLEVEL_RANKX(rankx, ddr_interface_num));
                    } while (lmc_rlevel_rank.cn63xx.status != 3);
    
                    {
                        uint64_t rlevel_bitmask[9];
                        int byte_idx;
    
                        for (byte_idx=0; byte_idx<(8+ecc_ena); ++byte_idx) {
                            rlevel_bitmask[byte_idx] = octeon_read_lmcx_ddr3_rlevel_dbg(ddr_interface_num, byte_idx);
                            rlevel_rank_errors += validate_ddr3_rlevel_bitmask(rlevel_bitmask[byte_idx], tclk_psecs);
                        }

                        ddr_print("Rank(%d) Rlevel Debug Test Results  8:0        : %05llx %05llx %05llx %05llx %05llx %05llx %05llx %05llx %05llx\n",
                                  rankx,
                                  rlevel_bitmask[8],
                                  rlevel_bitmask[7],
                                  rlevel_bitmask[6],
                                  rlevel_bitmask[5],
                                  rlevel_bitmask[4],
                                  rlevel_bitmask[3],
                                  rlevel_bitmask[2],
                                  rlevel_bitmask[1],
                                  rlevel_bitmask[0] 
                                  );

                        ddr_print("Rank(%d) Rlevel Rank %#5x, 0x%016llX : %5d %5d %5d %5d %5d %5d %5d %5d %5d (%d)\n",
                                  rankx,
                                  lmc_rlevel_rank.cn63xx.status,
                                  lmc_rlevel_rank.u64,
                                  lmc_rlevel_rank.cn63xx.byte8,
                                  lmc_rlevel_rank.cn63xx.byte7,
                                  lmc_rlevel_rank.cn63xx.byte6,
                                  lmc_rlevel_rank.cn63xx.byte5,
                                  lmc_rlevel_rank.cn63xx.byte4,
                                  lmc_rlevel_rank.cn63xx.byte3,
                                  lmc_rlevel_rank.cn63xx.byte2,
                                  lmc_rlevel_rank.cn63xx.byte1,
                                  lmc_rlevel_rank.cn63xx.byte0,
                                  rlevel_rank_errors
                                  );
    
                        if (ddr_rlevel_compute) {
                            /* Recompute the delays based on the bitmask */
                            for (byte_idx=0; byte_idx<(8+ecc_ena); ++byte_idx) {
                                update_rlevel_rank_struct(&lmc_rlevel_rank, byte_idx,
                                                          compute_ddr3_rlevel_delay(rlevel_bitmask[byte_idx],
                                                                                    rlevel_ctl));
                            }

                            ddr_print("Rank(%d) Computed delays                       : %5d %5d %5d %5d %5d %5d %5d %5d %5d\n",
                                      rankx,
                                      lmc_rlevel_rank.cn63xx.byte8,
                                      lmc_rlevel_rank.cn63xx.byte7,
                                      lmc_rlevel_rank.cn63xx.byte6,
                                      lmc_rlevel_rank.cn63xx.byte5,
                                      lmc_rlevel_rank.cn63xx.byte4,
                                      lmc_rlevel_rank.cn63xx.byte3,
                                      lmc_rlevel_rank.cn63xx.byte2,
                                      lmc_rlevel_rank.cn63xx.byte1,
                                      lmc_rlevel_rank.cn63xx.byte0
                                      );
                        }
                    }

                    if (ecc_ena) {
                        rlevel_byte[8] += lmc_rlevel_rank.cn63xx.byte7;
                        rlevel_byte[7] += lmc_rlevel_rank.cn63xx.byte6;
                        rlevel_byte[6] += lmc_rlevel_rank.cn63xx.byte5;
                        rlevel_byte[5] += lmc_rlevel_rank.cn63xx.byte4;
                        rlevel_byte[4] += lmc_rlevel_rank.cn63xx.byte8; /* ECC */
                    } else {
                        rlevel_byte[7] += lmc_rlevel_rank.cn63xx.byte7;
                        rlevel_byte[6] += lmc_rlevel_rank.cn63xx.byte6;
                        rlevel_byte[5] += lmc_rlevel_rank.cn63xx.byte5;
                        rlevel_byte[4] += lmc_rlevel_rank.cn63xx.byte4;
                    }
                    rlevel_byte[3] += lmc_rlevel_rank.cn63xx.byte3;
                    rlevel_byte[2] += lmc_rlevel_rank.cn63xx.byte2;
                    rlevel_byte[1] += lmc_rlevel_rank.cn63xx.byte1;
                    rlevel_byte[0] += lmc_rlevel_rank.cn63xx.byte0;

                    /* Save the score for this configuration */
                    rlevel_scoreboard[rtt_nom][rodt_ctl][rankx].score += rlevel_rank_errors;
                    debug_print("rlevel_scoreboard[rtt_nom=%d][rodt_ctl=%d][rankx=%d].score:%d\n",
                                rtt_nom, rodt_ctl, rankx,
                                rlevel_scoreboard[rtt_nom][rodt_ctl][rankx].score);

                    rlevel_rodt_errors += rlevel_rank_errors;
                } /* while (rlevel_avg_loops--) */
                /* Compute the average */
                rlevel_scoreboard[rtt_nom][rodt_ctl][rankx].score -= RLEVEL_UNDEFINED_ERROR;
                rlevel_scoreboard[rtt_nom][rodt_ctl][rankx].score =
                    divide_nint(rlevel_scoreboard[rtt_nom][rodt_ctl][rankx].score, rlevel_avg_loops);

                rlevel_byte[8] = divide_nint(rlevel_byte[8], rlevel_avg_loops);
                rlevel_byte[7] = divide_nint(rlevel_byte[7], rlevel_avg_loops);
                rlevel_byte[6] = divide_nint(rlevel_byte[6], rlevel_avg_loops);
                rlevel_byte[5] = divide_nint(rlevel_byte[5], rlevel_avg_loops);
                rlevel_byte[4] = divide_nint(rlevel_byte[4], rlevel_avg_loops);
                rlevel_byte[3] = divide_nint(rlevel_byte[3], rlevel_avg_loops);
                rlevel_byte[2] = divide_nint(rlevel_byte[2], rlevel_avg_loops);
                rlevel_byte[1] = divide_nint(rlevel_byte[1], rlevel_avg_loops);
                rlevel_byte[0] = divide_nint(rlevel_byte[0], rlevel_avg_loops);

                lmc_rlevel_rank.u64 = cvmx_read_csr(CVMX_LMCX_RLEVEL_RANKX(rankx, ddr_interface_num));

                if (ecc_ena) {
                    lmc_rlevel_rank.cn63xx.byte7 = rlevel_byte[8];
                    lmc_rlevel_rank.cn63xx.byte6 = rlevel_byte[7];
                    lmc_rlevel_rank.cn63xx.byte5 = rlevel_byte[6];
                    lmc_rlevel_rank.cn63xx.byte4 = rlevel_byte[5];
                    lmc_rlevel_rank.cn63xx.byte8 = rlevel_byte[4]; /* ECC */
                } else {
                    lmc_rlevel_rank.cn63xx.byte7 = rlevel_byte[7];
                    lmc_rlevel_rank.cn63xx.byte6 = rlevel_byte[6];
                    lmc_rlevel_rank.cn63xx.byte5 = rlevel_byte[5];
                    lmc_rlevel_rank.cn63xx.byte4 = rlevel_byte[4];
                }
                lmc_rlevel_rank.cn63xx.byte3 = rlevel_byte[3];
                lmc_rlevel_rank.cn63xx.byte2 = rlevel_byte[2];
                lmc_rlevel_rank.cn63xx.byte1 = rlevel_byte[1];
                lmc_rlevel_rank.cn63xx.byte0 = rlevel_byte[0];

                /* Evaluate delay sequence with a sliding window of three
                   byte delays across the range of bytes. */
                if ((spd_dimm_type == 1) || (spd_dimm_type == 5)) { /* 1=RDIMM, 5=Mini-RDIMM */
                    /* Registerd dimm topology routes from the center. */
                    for (byte_idx=0; byte_idx<(2+ecc_ena); ++byte_idx) {
                        rlevel_rank_errors += (nonsequential_delays(rlevel_byte[byte_idx+0],
                                                                    rlevel_byte[byte_idx+1],
                                                                    rlevel_byte[byte_idx+2])
                                               ? RLEVEL_NONSEQUENTIAL_DELAY_ERROR : 0);
                    }
                    for (byte_idx=4; byte_idx<(6+ecc_ena); ++byte_idx) {
                        rlevel_rank_errors += (nonsequential_delays(rlevel_byte[byte_idx+0],
                                                                    rlevel_byte[byte_idx+1],
                                                                    rlevel_byte[byte_idx+2])
                                               ? RLEVEL_NONSEQUENTIAL_DELAY_ERROR : 0);
                    }
                }
                if ((spd_dimm_type == 2) || (spd_dimm_type == 6)) { /* 1=UDIMM, 5=Mini-UDIMM */
                    /* Unbuffered dimm topology routes from end to end. */
                    for (byte_idx=0; byte_idx<(6+ecc_ena); ++byte_idx) {
                        rlevel_rank_errors += (nonsequential_delays(rlevel_byte[byte_idx+0],
                                                                    rlevel_byte[byte_idx+1],
                                                                    rlevel_byte[byte_idx+2])
                                               ? RLEVEL_NONSEQUENTIAL_DELAY_ERROR : 0);
                    }
                }

                if (rlevel_avg_loops > 1) {
                    ddr_print("Rank(%d) Rlevel Rank %#5x, 0x%016llX : %5d %5d %5d %5d %5d %5d %5d %5d %5d (%d) Average\n\n",
                              rankx,
                              lmc_rlevel_rank.cn63xx.status,
                              lmc_rlevel_rank.u64,
                              lmc_rlevel_rank.cn63xx.byte8,
                              lmc_rlevel_rank.cn63xx.byte7,
                              lmc_rlevel_rank.cn63xx.byte6,
                              lmc_rlevel_rank.cn63xx.byte5,
                              lmc_rlevel_rank.cn63xx.byte4,
                              lmc_rlevel_rank.cn63xx.byte3,
                              lmc_rlevel_rank.cn63xx.byte2,
                              lmc_rlevel_rank.cn63xx.byte1,
                              lmc_rlevel_rank.cn63xx.byte0,
                              rlevel_scoreboard[rtt_nom][rodt_ctl][rankx].score
                              );
                }

                rlevel_scoreboard[rtt_nom][rodt_ctl][rankx].setting = lmc_rlevel_rank.u64;
            } /* for (rankx = 0; rankx < dimm_count*2; rankx++) */
        } /* for (rodt_ctl=odt_config[odt_idx].qs_dic; rodt_ctl>0; --rodt_ctl) */
        } /*  for ( rtt_idx=1; rtt_idx<6; ++rtt_idx) */


        /* Re-enable dynamic compensation settings. */
        if (rlevel_comp_offset != 0) {
            lmc_comp_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num));

            lmc_comp_ctl2.s.ptune = 0;
            lmc_comp_ctl2.s.ntune = 0;
            lmc_comp_ctl2.s.byp = 0; /* Disable bypass mode */
            cvmx_write_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num), lmc_comp_ctl2.u64);
            cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num)); /* Read once */

            lmc_comp_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num)); /* Read again */
            ddr_print("DDR__PTUNE/DDR__PTUNE                         : %d/%d\n",
                      lmc_comp_ctl2.s.ddr__ptune, lmc_comp_ctl2.s.ddr__ntune);
        }
        } /* while(rlevel_loops--) */

        lmc_control.s.ddr2t           = save_ddr2t;
        cvmx_write_csr(CVMX_LMCX_CONTROL(ddr_interface_num), lmc_control.u64);
        lmc_control.u64 = cvmx_read_csr(CVMX_LMCX_CONTROL(ddr_interface_num));
        ddr_print("DDR2T                                         : %6d\n", lmc_control.s.ddr2t); /* Display final 2T value */


        {
            ddr_print("Evaluating Read-Leveling Scoreboard.\n");
            uint64_t best_setting[4] = {0};
            int      best_rodt_score = RLEVEL_UNDEFINED_ERROR;
            int      auto_rodt_ctl = 0;
            int      auto_rtt_nom  = 0;
            int rodt_score;

            for (rtt_idx=min_rtt_nom_idx; rtt_idx<=max_rtt_nom_idx; ++rtt_idx) {
            rtt_nom = rtt_nom_table[rtt_idx];

            for (rodt_ctl=max_rodt_ctl; rodt_ctl>=min_rodt_ctl; --rodt_ctl) {
                rodt_score = 0;
                for (rankx = 0; rankx < dimm_count * 2;rankx++) {
                    if (!(rank_mask & (1 << rankx)))
                        continue;
                    debug_print("rlevel_scoreboard[rtt_nom=%d][rodt_ctl=%d][rankx=%d].score:%d\n",
                                rtt_nom, rodt_ctl, rankx, rlevel_scoreboard[rtt_nom][rodt_ctl][rankx].score);
                    rodt_score += rlevel_scoreboard[rtt_nom][rodt_ctl][rankx].score;
                }
                debug_print("rodt_score:%d, best_rodt_score:%d\n", rodt_score, best_rodt_score);
                if (rodt_score < best_rodt_score) {
                    best_rodt_score = rodt_score;
                    auto_rodt_ctl   = rodt_ctl;
                    auto_rtt_nom    = rtt_nom;

                    for (rankx = 0; rankx < dimm_count * 2;rankx++) {
                        if (!(rank_mask & (1 << rankx)))
                            continue;
                        best_setting[rankx]  = rlevel_scoreboard[rtt_nom][rodt_ctl][rankx].setting;
                    }
                }
            }
            }

            lmc_modereg_params1.u64 = cvmx_read_csr(CVMX_LMCX_MODEREG_PARAMS1(ddr_interface_num));

            if (ddr_rtt_nom_auto) {
                if (rank_mask == 0xf) { /* 2-Rank, 2-Slot */
                    lmc_modereg_params1.s.rtt_nom_01 = auto_rtt_nom;
                    lmc_modereg_params1.s.rtt_nom_11 = auto_rtt_nom;
                }
                if (rank_mask == 0x5) { /* 1-Rank, 2-Slot */
                    lmc_modereg_params1.s.rtt_nom_00 = auto_rtt_nom;
                    lmc_modereg_params1.s.rtt_nom_10 = auto_rtt_nom;
                }
                if (rank_mask == 0x3) { /* 2-Rank, 1-Slot */
                    lmc_modereg_params1.s.rtt_nom_00 = auto_rtt_nom;
                }
                if (rank_mask == 0x1) { /* 1-Rank, 1-Slot */
                    lmc_modereg_params1.s.rtt_nom_00 = auto_rtt_nom;
                }
            } else {
                /* restore the manual settings to the register */
                lmc_modereg_params1.s.rtt_nom_00 = default_rtt_nom_00;
                lmc_modereg_params1.s.rtt_nom_01 = default_rtt_nom_01;
                lmc_modereg_params1.s.rtt_nom_10 = default_rtt_nom_10;
                lmc_modereg_params1.s.rtt_nom_11 = default_rtt_nom_11;
            }

            cvmx_write_csr(CVMX_LMCX_MODEREG_PARAMS1(ddr_interface_num), lmc_modereg_params1.u64);
            ddr_print("RTT_NOM     %3d, %3d, %3d, %3d ohms           :  %x,%x,%x,%x\n",
                      (lmc_modereg_params1.s.rtt_nom_11 == 0) ? 0 : rtt_nom_ohms[lmc_modereg_params1.s.rtt_nom_11],
                      (lmc_modereg_params1.s.rtt_nom_10 == 0) ? 0 : rtt_nom_ohms[lmc_modereg_params1.s.rtt_nom_10],
                      (lmc_modereg_params1.s.rtt_nom_01 == 0) ? 0 : rtt_nom_ohms[lmc_modereg_params1.s.rtt_nom_01],
                      (lmc_modereg_params1.s.rtt_nom_00 == 0) ? 0 : rtt_nom_ohms[lmc_modereg_params1.s.rtt_nom_00],
                      lmc_modereg_params1.s.rtt_nom_11,
                      lmc_modereg_params1.s.rtt_nom_10,
                      lmc_modereg_params1.s.rtt_nom_01,
                      lmc_modereg_params1.s.rtt_nom_00);

            ddr_print("RTT_WR      %3d, %3d, %3d, %3d ohms           :  %x,%x,%x,%x\n",
                      (lmc_modereg_params1.s.rtt_wr_11 == 0) ? 0 : rtt_wr_ohms[lmc_modereg_params1.s.rtt_wr_11],
                      (lmc_modereg_params1.s.rtt_wr_10 == 0) ? 0 : rtt_wr_ohms[lmc_modereg_params1.s.rtt_wr_10],
                      (lmc_modereg_params1.s.rtt_wr_01 == 0) ? 0 : rtt_wr_ohms[lmc_modereg_params1.s.rtt_wr_01],
                      (lmc_modereg_params1.s.rtt_wr_00 == 0) ? 0 : rtt_wr_ohms[lmc_modereg_params1.s.rtt_wr_00],
                      lmc_modereg_params1.s.rtt_wr_11,
                      lmc_modereg_params1.s.rtt_wr_10,
                      lmc_modereg_params1.s.rtt_wr_01,
                      lmc_modereg_params1.s.rtt_wr_00);

            ddr_print("DIC         %3d, %3d, %3d, %3d ohms           :  %x,%x,%x,%x\n",
                      dic_ohms[lmc_modereg_params1.s.dic_11],
                      dic_ohms[lmc_modereg_params1.s.dic_10],
                      dic_ohms[lmc_modereg_params1.s.dic_01],
                      dic_ohms[lmc_modereg_params1.s.dic_00],
                      lmc_modereg_params1.s.dic_11,
                      lmc_modereg_params1.s.dic_10,
                      lmc_modereg_params1.s.dic_01,
                      lmc_modereg_params1.s.dic_00);

            lmc_comp_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num));
            if (ddr_rodt_ctl_auto)
                lmc_comp_ctl2.s.rodt_ctl = auto_rodt_ctl;
            else
                lmc_comp_ctl2.s.rodt_ctl = default_rodt_ctl;
            cvmx_write_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num), lmc_comp_ctl2.u64);
            lmc_comp_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num));
            ddr_print("Read ODT_CTL                                  : 0x%x (%d ohms)\n",
                      lmc_comp_ctl2.s.rodt_ctl, rodt_ohms[lmc_comp_ctl2.s.rodt_ctl]);

            for (rankx = 0; rankx < dimm_count * 2;rankx++) {
                if (!(rank_mask & (1 << rankx)))
                    continue;
                cvmx_write_csr(CVMX_LMCX_RLEVEL_RANKX(rankx, ddr_interface_num), rlevel_scoreboard[auto_rtt_nom][auto_rodt_ctl][rankx].setting);
                lmc_rlevel_rank.u64 = cvmx_read_csr(CVMX_LMCX_RLEVEL_RANKX(rankx, ddr_interface_num));
                ddr_print("Rank(%d) Rlevel Rank %#5x, 0x%016llX : %5d %5d %5d %5d %5d %5d %5d %5d %5d (%d)\n",
                          rankx,
                          lmc_rlevel_rank.cn63xx.status,
                          lmc_rlevel_rank.u64,
                          lmc_rlevel_rank.cn63xx.byte8,
                          lmc_rlevel_rank.cn63xx.byte7,
                          lmc_rlevel_rank.cn63xx.byte6,
                          lmc_rlevel_rank.cn63xx.byte5,
                          lmc_rlevel_rank.cn63xx.byte4,
                          lmc_rlevel_rank.cn63xx.byte3,
                          lmc_rlevel_rank.cn63xx.byte2,
                          lmc_rlevel_rank.cn63xx.byte1,
                          lmc_rlevel_rank.cn63xx.byte0,
                          rlevel_scoreboard[auto_rtt_nom][auto_rodt_ctl][rankx].score
                          );
            }
        }

        perform_ddr3_init_sequence(cpu_id, rank_mask, ddr_interface_num);

        for (rankx = 0; rankx < dimm_count * 2;rankx++) {
            char buf[20]  = {0};
            uint64_t value;
            int parameter_set = 0;
            if (!(rank_mask & (1 << rankx)))
                continue;

            lmc_rlevel_rank.u64 = cvmx_read_csr(CVMX_LMCX_RLEVEL_RANKX(rankx, ddr_interface_num));

            for (i=0; i<9; ++i) {
                sprintf(buf, "ddr_rlevel_rank%d_byte%d", rankx, i);
                if ((s = lookup_env_parameter(buf)) != NULL) {
                    parameter_set |= 1;
                    value = simple_strtoul(s, NULL, 0);

                    update_rlevel_rank_struct(&lmc_rlevel_rank, i, value);
                }
            }

            sprintf(buf, "ddr_rlevel_rank%d", rankx);
            if ((s = lookup_env_parameter_ull(buf)) != NULL) {
                parameter_set |= 1;
                value = simple_strtoull(s, NULL, 0);
                lmc_rlevel_rank.u64 = value;
            }

            if (parameter_set) {
                cvmx_write_csr(CVMX_LMCX_RLEVEL_RANKX(rankx, ddr_interface_num), lmc_rlevel_rank.u64);
                lmc_rlevel_rank.u64 = cvmx_read_csr(CVMX_LMCX_RLEVEL_RANKX(rankx, ddr_interface_num));
    
                ddr_print("Rank(%d) Rlevel Rank %#5x, 0x%016llX : %5d %5d %5d %5d %5d %5d %5d %5d %5d\n",
                          rankx,
                          lmc_rlevel_rank.cn63xx.status,
                          lmc_rlevel_rank.u64,
                          lmc_rlevel_rank.cn63xx.byte8,
                          lmc_rlevel_rank.cn63xx.byte7,
                          lmc_rlevel_rank.cn63xx.byte6,
                          lmc_rlevel_rank.cn63xx.byte5,
                          lmc_rlevel_rank.cn63xx.byte4,
                          lmc_rlevel_rank.cn63xx.byte3,
                          lmc_rlevel_rank.cn63xx.byte2,
                          lmc_rlevel_rank.cn63xx.byte1,
                          lmc_rlevel_rank.cn63xx.byte0
                          );
            }
        }
    }

    /*
     * 2. Without changing any other fields in LMC0_CONFIG,
     *
     *    o write LMC0_CONFIG[SEQUENCE] to select read-leveling, and
     *
     *    o write LMC0_CONFIG[RANKMASK] = (1 << i), and
     *
     *    o write LMC0_CONFIG[INIT_START] = 1
     *
     *    This initiates the previously-described read-leveling.
     *
     * 3. Wait until LMC0_RLEVEL_RANKi[STATUS] != 2
     *
     *    LMC will have updated LMC0_RLEVEL_RANKi[BYTE*] for all
     *    byte lanes at this point.
     *
     * 4. If desired, consult LMC0_RLEVEL_DBG[BITMASK] and compare
     *    to LMC0_RLEVEL_RANKi[BYTE*] for the lane selected by
     *    LMC0_RLEVEL_CTL[BYTE]. If desired, modify
     *    LMC0_RLEVEL_CTL[BYTE] to a new value and repeat so that
     *    all BITMASKs can be observed.
     *
     * 5. Initialize LMC0_RLEVEL_RANK* values for all unused ranks.
     *
     *    Let rank i be a rank with attached DRAM.
     *
     *    For all ranks j that do not have attached DRAM, set
     *    LMC0_RLEVEL_RANKj = LMC0_RLEVEL_RANKi.
     */

    {
        /* Try to determine/optimize write-level delays experimentally. */
        cvmx_lmcx_wlevel_rankx_t lmc_wlevel_rank;
        cvmx_lmcx_rlevel_rankx_t lmc_rlevel_rank;
        cvmx_lmcx_config_t lmc_config;
        cvmx_lmcx_modereg_params0_t lmc_modereg_params0;
        int byte;
        int delay;
        char *eptr;
        int rankx = 0;
        int save_ecc_ena;
        int active_rank;
        int sw_wlevel_offset = 1;
        int sw_wlevel_enable = 1;
        typedef enum {
            WL_ESTIMATED = 0,   /* HW/SW wleveling failed. Results
                                   estimated. */
            WL_HARDWARE  = 1,   /* H/W wleveling succeeded */
            WL_SOFTWARE  = 2,   /* S/W wleveling passed 2 contiguous
                                   settings. */
            WL_SOFTWARE1 = 3,   /* S/W wleveling passed 1 marginal
                                   setting. */
        } sw_wl_status_t;

        static const char *wl_status_strings[] = {
            "(e)",
            "   ",
            "   ",
            "(1)"
        };

        if ((s = lookup_env_parameter("ddr_software_wlevel")) != NULL) {
            sw_wlevel_enable = simple_strtoul(s, NULL, 0);
        }

        if (sw_wlevel_enable)
            ddr_print("Performing software Write-Leveling\n");
            
        /* Disable ECC for DRAM tests */
        lmc_config.u64 = cvmx_read_csr(CVMX_LMCX_CONFIG(ddr_interface_num));
        save_ecc_ena = lmc_config.s.ecc_ena;
        lmc_config.s.ecc_ena = 0;
        cvmx_write_csr(CVMX_LMCX_CONFIG(ddr_interface_num), lmc_config.u64);
        limit_l2_ways(0, 0);       /* Disable l2 sets for DRAM testing */

        lmc_modereg_params0.u64 = cvmx_read_csr(CVMX_LMCX_MODEREG_PARAMS0(ddr_interface_num));

        /* We need to track absolute rank number, as well as how many
        ** active ranks we have.  Two single rank DIMMs show up as
        ** ranks 0 and 2, but only 2 ranks are active. */
        active_rank = 0;

        for (rankx = 0; rankx < dimm_count * 2;rankx++) {
            sw_wl_status_t byte_test_status[9] = {WL_ESTIMATED};
            sw_wl_status_t sw_wl_rank_status = WL_HARDWARE;
            int bytes_failed = 0;

            if (!sw_wlevel_enable)
                break;

            if (!(rank_mask & (1 << rankx)))
                continue;
    

            lmc_wlevel_rank.u64 = cvmx_read_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num));
            if (wlevel_bitmask_errors == 0) {
                for (byte=0; byte<8; ++byte) {
                    /* If h/w write-leveling had no errors then use
                    ** s/w write-leveling to fixup only the upper bits
                    ** of the delays. */
                    for (delay=get_wlevel_rank_struct(&lmc_wlevel_rank, byte); delay<32; delay+=8) {
                        update_wlevel_rank_struct(&lmc_wlevel_rank, byte, delay);
                        debug_print("Testing byte %d delay %2d\n", byte, delay);
                        cvmx_write_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num), lmc_wlevel_rank.u64);
                        lmc_wlevel_rank.u64 = cvmx_read_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num));

                        /* Determine address of DRAM to test for software write leveling.  Adjust
                        ** address for boot bus hole in memory map. */
                        uint64_t rank_addr = active_rank * ((1ull << pbank_lsb)/num_ranks);
                        if (rank_addr > 0x10000000)
                            rank_addr += 0x10000000;
                        if (test_dram_byte(rank_addr, 2048, 8, byte) == 0) {
                            debug_print("        byte %d(0x%x) delay %2d Passed\n", byte, rank_addr, delay);
                            byte_test_status[byte] = WL_HARDWARE;
                            break;
                        } else {
                            debug_print("        byte %d delay %2d Errors\n", byte, delay);
                        }
                    }
                }

                if (save_ecc_ena) {
                        int save_byte8 = lmc_wlevel_rank.cn63xx.byte8;
                        byte_test_status[8] = WL_HARDWARE; /* H/W delay value */
                        if (spd_rdimm) {
                            while (nonsequential_delays(lmc_wlevel_rank.cn63xx.byte8,
                                                        lmc_wlevel_rank.cn63xx.byte4,
                                                        lmc_wlevel_rank.cn63xx.byte5) ||
                                   nonsequential_delays(lmc_wlevel_rank.cn63xx.byte2,
                                                        lmc_wlevel_rank.cn63xx.byte3,
                                                        lmc_wlevel_rank.cn63xx.byte8)) {
                                lmc_wlevel_rank.cn63xx.byte8 += 8;
                                if (lmc_wlevel_rank.cn63xx.byte8 == save_byte8) {
                                    byte_test_status[8] = WL_ESTIMATED; /* Estimated delay */
                                    break; /* give up if it wraps back to where it started */
                                }
                            }
                        } else {
                            while (nonsequential_delays(lmc_wlevel_rank.cn63xx.byte3,
                                                        lmc_wlevel_rank.cn63xx.byte8,
                                                        lmc_wlevel_rank.cn63xx.byte4)) {
                                lmc_wlevel_rank.cn63xx.byte8 += 8;
                                if (lmc_wlevel_rank.cn63xx.byte8 == save_byte8) {
                                    byte_test_status[8] = WL_ESTIMATED; /* Estimated delay */
                                    break; /* give up if it wraps back to where it started */
                                }
                            }
                        }
                } else {
                    lmc_wlevel_rank.cn63xx.byte8 = lmc_wlevel_rank.cn63xx.byte0; /* ECC is not used */
                }
            }

            for (byte=0; byte<8; ++byte) {
                bytes_failed += (byte_test_status[byte]==WL_ESTIMATED);
            }

            if (bytes_failed) {
                sw_wl_rank_status = WL_SOFTWARE;
                /* If s/w fixups failed then retry using s/w write-leveling. */
                if (wlevel_bitmask_errors == 0) {
                    /* h/w succeeded but s/w fixups failed. So retry s/w. */
                    debug_print("Rank(%d) Retrying software Write-Leveling.\n", rankx);
                }
                for (byte=0; byte<8; ++byte) {
                    int passed = 0;
                    int wl_offset;
                    for (wl_offset = sw_wlevel_offset; wl_offset >= 0; --wl_offset) {
                    //for (delay=30; delay>=0; delay-=2) { /* Top-Down */
                    for (delay=0; delay<32; delay+=2) {  /* Bottom-UP */
                        update_wlevel_rank_struct(&lmc_wlevel_rank, byte, delay);
                        debug_print("Testing byte %d delay %2d\n", byte, delay);
                        cvmx_write_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num), lmc_wlevel_rank.u64);
                        lmc_wlevel_rank.u64 = cvmx_read_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num));

                        /* Determine address of DRAM to test for software write leveling.  Adjust
                        ** address for boot bus hole in memory map. */
                        uint64_t rank_addr = active_rank * ((1ull << pbank_lsb)/num_ranks);
                        if (rank_addr > 0x10000000)
                            rank_addr += 0x10000000;
                        if (test_dram_byte(rank_addr, 2048, 8, byte) == 0) {
                            ++passed;
                            if (passed == (1+wl_offset)) { /* Look for consecutive working settings */
                                debug_print("        byte %d(0x%x) delay %2d Passed\n", byte, rank_addr, delay);
                                if (wl_offset == 1) {
                                    byte_test_status[byte] = WL_SOFTWARE;
                                } else if (wl_offset == 0) {
                                    byte_test_status[byte] = WL_SOFTWARE1;
                                }
                                break;
                            }
                        } else {
                            debug_print("        byte %d delay %2d Errors\n", byte, delay);
                            passed = 0;
                        }
                    }
                    if (passed == (1+wl_offset)) { /* Look for consecutive working settings */
                        break;
                    }
                    }
                    if (passed == 0) {
                        /* Last resort. Use Rlevel settings to estimate
                           Wlevel if software write-leveling fails */
                        lmc_rlevel_rank.u64 = cvmx_read_csr(CVMX_LMCX_RLEVEL_RANKX(rankx, ddr_interface_num));
                        rlevel_to_wlevel(&lmc_rlevel_rank, &lmc_wlevel_rank, byte);
                    } 
                }

                if (save_ecc_ena) {
                    /* ECC byte has to be estimated. Take the average of the two surrounding bytes. */
                    lmc_wlevel_rank.cn63xx.byte8 = ((lmc_wlevel_rank.cn63xx.byte3+lmc_wlevel_rank.cn63xx.byte4)/2) & 0x1e;
                    byte_test_status[8] = WL_ESTIMATED; /* Estimated delay */
                } else {
                    lmc_wlevel_rank.cn63xx.byte8 = lmc_wlevel_rank.cn63xx.byte0; /* ECC is not used */
                }
            }
    
            cvmx_write_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num), lmc_wlevel_rank.u64);
            lmc_wlevel_rank.u64 = cvmx_read_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num));
    
            ddr_print("Rank(%d) Wlevel Rank %#5x, 0x%016llX : %5d%3s %2d%3s %2d%3s %2d%3s %2d%3s %2d%3s %2d%3s %2d%3s %2d%3s %s\n",
                      rankx,
                      lmc_wlevel_rank.cn63xx.status,
                      lmc_wlevel_rank.u64,
                      lmc_wlevel_rank.cn63xx.byte8, wl_status_strings[byte_test_status[8]],
                      lmc_wlevel_rank.cn63xx.byte7, wl_status_strings[byte_test_status[7]],
                      lmc_wlevel_rank.cn63xx.byte6, wl_status_strings[byte_test_status[6]],
                      lmc_wlevel_rank.cn63xx.byte5, wl_status_strings[byte_test_status[5]],
                      lmc_wlevel_rank.cn63xx.byte4, wl_status_strings[byte_test_status[4]],
                      lmc_wlevel_rank.cn63xx.byte3, wl_status_strings[byte_test_status[3]],
                      lmc_wlevel_rank.cn63xx.byte2, wl_status_strings[byte_test_status[2]],
                      lmc_wlevel_rank.cn63xx.byte1, wl_status_strings[byte_test_status[1]],
                      lmc_wlevel_rank.cn63xx.byte0, wl_status_strings[byte_test_status[0]],
                      (sw_wl_rank_status == WL_HARDWARE) ? "" : "(s)"
                      );

            active_rank++;
        }


        for (rankx = 0; rankx < dimm_count * 2;rankx++) {
            char buf[20]  = {0};
            uint64_t value;
            int parameter_set = 0;
            if (!(rank_mask & (1 << rankx)))
                continue;

            lmc_wlevel_rank.u64 = cvmx_read_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num));

            for (i=0; i<9; ++i) {
                sprintf(buf, "ddr_wlevel_rank%d_byte%d", rankx, i);
                if ((s = lookup_env_parameter(buf)) != NULL) {
                    parameter_set |= 1;
                    value = simple_strtoul(s, NULL, 0);

                    update_wlevel_rank_struct(&lmc_wlevel_rank, i, value);
                }
            }

            sprintf(buf, "ddr_wlevel_rank%d", rankx);
            if ((s = lookup_env_parameter_ull(buf)) != NULL) {
                parameter_set |= 1;
                value = simple_strtoull(s, NULL, 0);
                lmc_wlevel_rank.u64 = value;
            }

            if (parameter_set) {
                cvmx_write_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num), lmc_wlevel_rank.u64);
                lmc_wlevel_rank.u64 = cvmx_read_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num));
    
                ddr_print("Rank(%d) Wlevel Rank %#5x, 0x%016llX : %5d %5d %5d %5d %5d %5d %5d %5d %5d\n",
                          rankx,
                          lmc_wlevel_rank.cn63xx.status,
                          lmc_wlevel_rank.u64,
                          lmc_wlevel_rank.cn63xx.byte8,
                          lmc_wlevel_rank.cn63xx.byte7,
                          lmc_wlevel_rank.cn63xx.byte6,
                          lmc_wlevel_rank.cn63xx.byte5,
                          lmc_wlevel_rank.cn63xx.byte4,
                          lmc_wlevel_rank.cn63xx.byte3,
                          lmc_wlevel_rank.cn63xx.byte2,
                          lmc_wlevel_rank.cn63xx.byte1,
                          lmc_wlevel_rank.cn63xx.byte0
                          );
            }
        }

        /* Restore the ECC configuration */
        lmc_config.s.ecc_ena = save_ecc_ena;
        cvmx_write_csr(CVMX_LMCX_CONFIG(ddr_interface_num), lmc_config.u64);
        
        /* Restore the l2 set configuration */
        eptr = getenv("limit_l2_ways");
        if (eptr) {
            int ways = simple_strtoul(eptr, NULL, 10);
            limit_l2_ways(ways, 1);
        } else {
            limit_l2_ways(cvmx_l2c_get_num_assoc(), 0);
        }
    
        eptr = getenv("limit_l2_mask");
        if (eptr) {
            int mask = simple_strtoul(eptr, NULL, 10);
            limit_l2_mask(mask);
        }
    }

#if SAVE_RESTORE_DDR_CONFIG
    if (software_leveling) {
        save_lmc_comp_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num));
        for (rankx = 0; rankx < dimm_count * 2;rankx++) {
            if (!(rank_mask & (1 << rankx)))
                continue;
    
            save_lmc_wlevel_rank[rankx].u64 = cvmx_read_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num));
            save_lmc_rlevel_rank[rankx].u64 = cvmx_read_csr(CVMX_LMCX_RLEVEL_RANKX(rankx, ddr_interface_num));
        }
    }
#endif

#if CONFIG_SOFTWARE_LEVELING
#define DEBUG_SOFTWARE_LEVELING

    if ((software_leveling) && (wlevel_bitmask_errors != 0)) {
        /* Try to determine write-level delays experimentally. */
        cvmx_lmcx_wlevel_rankx_t lmc_wlevel_rank;
        cvmx_lmcx_rlevel_rankx_t lmc_rlevel_rank;
        cvmx_lmcx_config_t lmc_config;
        cvmx_lmcx_comp_ctl2_t lmc_comp_ctl2;

        int byte;
        int rlevel_delay;
        int wlevel_delay;
        int byte_test_status[9] = {0};
        char *eptr;
        int rankx = 0;
        int save_ecc_ena;
        int rodt_ctl;
        static const int rodt_ohms[] = { -1, 20, 30, 40, 60, 120 };
        int i;

        ddr_print("Performing software READ/Write Leveling\n");
            
        /* Disable ECC for DRAM tests */
        lmc_config.u64 = cvmx_read_csr(CVMX_LMCX_CONFIG(ddr_interface_num));
        save_ecc_ena = lmc_config.s.ecc_ena;
        lmc_config.s.ecc_ena = 0;
        cvmx_write_csr(CVMX_LMCX_CONFIG(ddr_interface_num), lmc_config.u64);
        limit_l2_ways(0, 0);       /* Disable l2 sets for DRAM testing */

#define RLEVEL_SWEEP_SETTINGS  24

        for (rodt_ctl=5; rodt_ctl>0; --rodt_ctl) {
            /* We need to track absolute rank number, as well as how many active ranks we have.
            ** Two single rank DIMMs show up as ranks 0 and 2, but only 2 ranks are active. */
            int active_rank = 0;

            lmc_comp_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num));
            lmc_comp_ctl2.s.rodt_ctl = rodt_ctl;
            cvmx_write_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num), lmc_comp_ctl2.u64);
            lmc_comp_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num));
            printf("Read ODT_CTL                                  : 0x%x (%d ohms)\n",
                      lmc_comp_ctl2.s.rodt_ctl, rodt_ohms[rodt_ctl]);

            for (rankx = 0; rankx < dimm_count * 2;rankx++) {
                if (!(rank_mask & (1 << rankx)))
                    continue;

                lmc_rlevel_rank.u64 = cvmx_read_csr(CVMX_LMCX_RLEVEL_RANKX(rankx, ddr_interface_num));
                for (byte=0; byte<8; ++byte) {
                    uint32_t test_results[RLEVEL_SWEEP_SETTINGS] = {0};
                    for (rlevel_delay=RLEVEL_SWEEP_SETTINGS-1; rlevel_delay>=0; --rlevel_delay) {
                        update_rlevel_rank_struct(&lmc_rlevel_rank, byte, rlevel_delay);

                        /* Hardware read-level sequence resets internal counters */
                        lmc_config.u64 = cvmx_read_csr(CVMX_LMCX_CONFIG(ddr_interface_num));
                        lmc_config.s.sequence        = 1; /* read-leveling */
                        lmc_config.s.rankmask        = 1 << rankx;
                        lmc_config.s.init_start      = 1;

                        cvmx_write_csr(CVMX_LMCX_CONFIG(ddr_interface_num), lmc_config.u64);

                        cvmx_write_csr(CVMX_LMCX_RLEVEL_RANKX(rankx, ddr_interface_num), lmc_rlevel_rank.u64);
                        lmc_rlevel_rank.u64 = cvmx_read_csr(CVMX_LMCX_RLEVEL_RANKX(rankx, ddr_interface_num));

                        lmc_wlevel_rank.u64 = cvmx_read_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num));
                        for (wlevel_delay=30; wlevel_delay>=0; wlevel_delay-=2) {
                            update_wlevel_rank_struct(&lmc_wlevel_rank, byte, wlevel_delay);

                            cvmx_write_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num), lmc_wlevel_rank.u64);
                            lmc_wlevel_rank.u64 = cvmx_read_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num));

#define TEST_DRAM_STRIDE  32
#define TEST_DRAM_RANGE  (((2048*8)+TEST_DRAM_STRIDE-1)/TEST_DRAM_STRIDE)

                            /* Determine address of DRAM to test for software write leveling.  Adjust
                            ** address for boot bus hole in memory map. */
                            uint64_t rank_addr = active_rank * ((1ull << pbank_lsb)/num_ranks);
                            if (rank_addr > 0x10000000)
                                rank_addr += 0x10000000;
                            if (test_dram_byte(rank_addr, TEST_DRAM_RANGE, TEST_DRAM_STRIDE, byte) == 0) {
                                test_results[rlevel_delay] |= (uint32_t)(1<<(wlevel_delay/2));
                                debug_print("        byte %d(0x%08x) rlevel_delay %2d, wlevel_delay %2d Passed\n",
                                            byte, rank_addr, rlevel_delay, wlevel_delay);
                                byte_test_status[byte] = 1;
                            } else {
                                debug_print("        byte %d(0x%08x) rlevel_delay %2d, wlevel_delay %2d Failed!!!!\n",
                                            byte, rank_addr, rlevel_delay, wlevel_delay);
                            }
                        }
                    }

#ifdef DEBUG_SOFTWARE_LEVELING
                    printf("Rank %d Byte %d:", rankx, byte);
                    for (i=0; i<RLEVEL_SWEEP_SETTINGS; ++i) {
                        if (((i%16)==0)&&(i!=0)) printf("\n              ");
                        printf(" %04x", test_results[i]);
                    }
                    printf("\n\n");
#endif
                }
                active_rank++;
            }
        }

        /* Restore the ECC configuration */
        lmc_config.s.ecc_ena = save_ecc_ena;
        cvmx_write_csr(CVMX_LMCX_CONFIG(ddr_interface_num), lmc_config.u64);
        
        /* Restore the l2 set configuration */
        eptr = getenv("limit_l2_ways");
        if (eptr) {
            int ways = simple_strtoul(eptr, NULL, 10);
            limit_l2_ways(ways, 0);
        } else {
            limit_l2_ways(cvmx_l2c_get_num_assoc(), 0);
        }
    
        eptr = getenv("limit_l2_mask");
        if (eptr) {
            int mask = simple_strtoul(eptr, NULL, 10);
            limit_l2_mask(mask);
        }


        /* Clear any residual ECC errors */
        cvmx_write_csr(CVMX_LMCX_INT(ddr_interface_num), -1ULL);
    }
#endif  /* CONFIG_SOFTWARE_LEVELING */

#if SAVE_RESTORE_DDR_CONFIG
    if (software_leveling) {
        static const int rodt_ohms[] = { -1, 20, 30, 40, 60, 120 };
        cvmx_lmcx_config_t lmc_config;

        cvmx_write_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num), save_lmc_comp_ctl2.u64);
        save_lmc_comp_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num));
        ddr_print("Read ODT_CTL                                  : 0x%x (%d ohms)\n",
                  save_lmc_comp_ctl2.s.rodt_ctl, rodt_ohms[save_lmc_comp_ctl2.s.rodt_ctl]);

        for (rankx = 0; rankx < dimm_count * 2;rankx++) {
            if (!(rank_mask & (1 << rankx)))
                continue;
    
            /* Hardware read-level sequence resets internal counters */
            lmc_config.u64 = cvmx_read_csr(CVMX_LMCX_CONFIG(ddr_interface_num));
            lmc_config.s.sequence        = 1; /* read-leveling */
            lmc_config.s.rankmask        = 1 << rankx;
            lmc_config.s.init_start      = 1;

            cvmx_write_csr(CVMX_LMCX_CONFIG(ddr_interface_num), lmc_config.u64);

            cvmx_write_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num), save_lmc_wlevel_rank[rankx].u64);
            save_lmc_wlevel_rank[rankx].u64 = cvmx_read_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num));
            ddr_print("Rank(%d) Wlevel Rank %#5x, 0x%016llX : %5d %5d %5d %5d %5d %5d %5d %5d %5d\n",
                      rankx,
                      save_lmc_wlevel_rank[rankx].cn63xx.status,
                      save_lmc_wlevel_rank[rankx].u64,
                      save_lmc_wlevel_rank[rankx].cn63xx.byte8,
                      save_lmc_wlevel_rank[rankx].cn63xx.byte7,
                      save_lmc_wlevel_rank[rankx].cn63xx.byte6,
                      save_lmc_wlevel_rank[rankx].cn63xx.byte5,
                      save_lmc_wlevel_rank[rankx].cn63xx.byte4,
                      save_lmc_wlevel_rank[rankx].cn63xx.byte3,
                      save_lmc_wlevel_rank[rankx].cn63xx.byte2,
                      save_lmc_wlevel_rank[rankx].cn63xx.byte1,
                      save_lmc_wlevel_rank[rankx].cn63xx.byte0
                      );
        }
        for (rankx = 0; rankx < dimm_count * 2;rankx++) {
            if (!(rank_mask & (1 << rankx)))
                continue;
    
            cvmx_write_csr(CVMX_LMCX_RLEVEL_RANKX(rankx, ddr_interface_num), save_lmc_rlevel_rank[rankx].u64);
            save_lmc_rlevel_rank[rankx].u64 = cvmx_read_csr(CVMX_LMCX_RLEVEL_RANKX(rankx, ddr_interface_num));
            ddr_print("Rank(%d) Rlevel Rank %#5x, 0x%016llX : %5d %5d %5d %5d %5d %5d %5d %5d %5d\n",
                      rankx,
                      save_lmc_rlevel_rank[rankx].cn63xx.status,
                      save_lmc_rlevel_rank[rankx].u64,
                      save_lmc_rlevel_rank[rankx].cn63xx.byte8,
                      save_lmc_rlevel_rank[rankx].cn63xx.byte7,
                      save_lmc_rlevel_rank[rankx].cn63xx.byte6,
                      save_lmc_rlevel_rank[rankx].cn63xx.byte5,
                      save_lmc_rlevel_rank[rankx].cn63xx.byte4,
                      save_lmc_rlevel_rank[rankx].cn63xx.byte3,
                      save_lmc_rlevel_rank[rankx].cn63xx.byte2,
                      save_lmc_rlevel_rank[rankx].cn63xx.byte1,
                      save_lmc_rlevel_rank[rankx].cn63xx.byte0
                      );
        }
    }
#endif  /* SAVE_RESTORE_DDR_CONFIG */

    /*
     * 4.8.8 Final LMC Initialization
     *
     * Early LMC initialization, LMC write-leveling, and LMC
     * read-leveling must be completed prior to starting this
     * final LMC initialization.
     *
     * LMC hardware updates the LMC0_SLOT_CTL0, LMC0_SLOT_CTL1,
     * LMC0_SLOT_CTL2 CSRs with minimum values based on the
     * selected read-leveling and write-leveling
     * settings. Software should not write the final
     * LMC0_SLOT_CTL0, LMC0_SLOT_CTL1, and LMC0_SLOT_CTL2 values
     * until after the final readleveling and write-leveling
     * settings are written.
     *
     * Software must ensure the LMC0_SLOT_CTL0, LMC0_SLOT_CTL1,
     * and LMC0_SLOT_CTL2 CSR values are appropriate for this
     * step. These CSRs select the minimum gaps between reads and
     * writes of various types.
     *
     * Software must not reduce the values in these CSR fields
     * below the values previously selected by the LMC hardware
     * (during write-leveling and read-leveling steps above).
     *
     * All sections in this chapter may be used to derive proper
     * settings for these registers.
     */

    if (safe_ddr_flag) {
        cvmx_lmcx_slot_ctl0_t lmc_slot_ctl0;
        cvmx_lmcx_slot_ctl1_t lmc_slot_ctl1;
        cvmx_lmcx_slot_ctl2_t lmc_slot_ctl2;

        lmc_slot_ctl0.u64 = 0;
        lmc_slot_ctl0.cn63xx.r2r_init = 0x3f;
        lmc_slot_ctl0.cn63xx.r2w_init = 0x3f;
        lmc_slot_ctl0.cn63xx.w2r_init = 0x3f;
        lmc_slot_ctl0.cn63xx.w2w_init = 0x3f;
        cvmx_write_csr(CVMX_LMCX_SLOT_CTL0(ddr_interface_num), lmc_slot_ctl0.u64);

        lmc_slot_ctl1.u64 = 0;
        lmc_slot_ctl1.cn63xx.r2r_xrank_init = 0x3f;
        lmc_slot_ctl1.cn63xx.r2w_xrank_init = 0x3f;
        lmc_slot_ctl1.cn63xx.w2r_xrank_init = 0x3f;
        lmc_slot_ctl1.cn63xx.w2w_xrank_init = 0x3f;
        cvmx_write_csr(CVMX_LMCX_SLOT_CTL1(ddr_interface_num), lmc_slot_ctl1.u64);

        lmc_slot_ctl2.u64 = 0;
        lmc_slot_ctl2.cn63xx.r2r_xdimm_init = 0x3f;
        lmc_slot_ctl2.cn63xx.r2w_xdimm_init = 0x3f;
        lmc_slot_ctl2.cn63xx.w2r_xdimm_init = 0x3f;
        lmc_slot_ctl2.cn63xx.w2w_xdimm_init = 0x3f;
        cvmx_write_csr(CVMX_LMCX_SLOT_CTL2(ddr_interface_num), lmc_slot_ctl2.u64);
    }

    /* Clear any residual ECC errors */
    cvmx_write_csr(CVMX_LMCX_INT(ddr_interface_num), -1ULL);
    if (!octeon_is_cpuid(OCTEON_CN63XX_PASS1_X)) {
        /* Initialize "Disabled LMC Error" status */
        cvmx_write_csr(CVMX_L2C_TADX_INT(0), cvmx_read_csr(CVMX_L2C_TADX_INT(0))); /* (not present in pass 1 O63) */
    }

    return(mem_size_mbytes);
}

#if defined(CONFIG_OCTEON3)

static int64_t
calc_delay_trend(int64_t v)
{
    if (v == 0)
        return (0);
    if (v < 0)
        return (-1);
    return 1;
}

typedef struct {
    int delay;
    int loop_total;
    int loop_count;
} rlevel_byte_data_t;

static void
perform_octeon3_ddr3_sequence(uint32_t cpu_id, int rank_mask, int ddr_interface_num, int sequence)
{
    /*
     * 3. Without changing any other fields in LMC(0)_CONFIG, write
     *    LMC(0)_CONFIG[RANKMASK] then write both
     *    LMC(0)_SEQ_CTL[SEQ_SEL,INIT_START] = 1 with a single CSR write
     *    operation. LMC(0)_CONFIG[RANKMASK] bits should be set to indicate
     *    the ranks that will participate in the sequence.
     * 
     *    The LMC(0)_SEQ_CTL[SEQ_SEL] value should select power-up/init or
     *    selfrefresh exit, depending on whether the DRAM parts are in
     *    self-refresh and whether their contents should be preserved. While
     *    LMC performs these sequences, it will not perform any other DDR3
     *    transactions. When the sequence is complete, hardware sets the
     *    LMC(0)_CONFIG[INIT_STATUS] bits for the ranks that have been
     *    initialized.
     * 
     *    If power-up/init is selected immediately following a DRESET
     *    assertion, LMC executes the sequence described in the "Reset and
     *    Initialization Procedure" section of the JEDEC DDR3
     *    specification. This includes activating CKE, writing all four DDR3
     *    mode registers on all selected ranks, and issuing the required ZQCL
     *    command. The LMC(0)_CONFIG[RANKMASK] value should select all ranks
     *    with attached DRAM in this case. If LMC(0)_CONTROL[RDIMM_ENA] = 1,
     *    LMC writes the JEDEC standard SSTE32882 control words selected by
     *    LMC(0)_DIMM_CTL[DIMM*_WMASK] between DDR_CKE* signal assertion and
     *    the first DDR3 mode register write operation.
     *    LMC(0)_DIMM_CTL[DIMM*_WMASK] should be cleared to 0 if the
     *    corresponding DIMM is not present.
     * 
     *    If self-refresh exit is selected, LMC executes the required SRX
     *    command followed by a refresh and ZQ calibration. Section 4.5
     *    describes behavior of a REF + ZQCS.  LMC does not write the DDR3
     *    mode registers as part of this sequence, and the mode register
     *    parameters must match at self-refresh entry and exit times.
     * 
     * 4. Read LMC(0)_SEQ_CTL and wait for LMC(0)_SEQ_CTL[SEQ_COMPLETE] to be
     *    set.
     * 
     * 5. Read LMC(0)_CONFIG[INIT_STATUS] and confirm that all ranks have
     *    been initialized.
     */

#if defined(__U_BOOT__) || defined(unix)
    char *s;
#endif

#undef DEBUG_PERFORM_DDR3_SEQUENCE
#ifdef DEBUG_PERFORM_DDR3_SEQUENCE
    static const char *sequence_str[] = {
            "Power-up/init",
            "Read-leveling",
            "Self-refresh entry",
            "Self-refresh exit",
            "Illegal",
            "Illegal",
            "Write-leveling",
            "Init Register Control Words",
            "Mode Register Write",
            "MPR Register Access",
            "Vref internal training",
            "Offset Training"
    };
#endif

    cvmx_lmcx_seq_ctl_t seq_ctl;
    cvmx_lmcx_config_t  lmc_config;

    lmc_config.u64 = cvmx_read_csr(CVMX_LMCX_CONFIG(ddr_interface_num));
    lmc_config.s.rankmask     = rank_mask;
    cvmx_write_csr(CVMX_LMCX_CONFIG(ddr_interface_num), lmc_config.u64);

    seq_ctl.u64    = 0;

    seq_ctl.s.init_start  = 1;
    seq_ctl.s.seq_sel    = sequence;

#ifdef DEBUG_PERFORM_DDR3_SEQUENCE
    ddr_print("Performing LMC sequence: rank_mask=0x%02x, sequence=%d, %s\n",
              rank_mask, sequence, sequence_str[sequence]);
#endif

    cvmx_write_csr(CVMX_LMCX_SEQ_CTL(ddr_interface_num), seq_ctl.u64);
    cvmx_read_csr(CVMX_LMCX_SEQ_CTL(ddr_interface_num));

    do {
        cvmx_wait_usec(10); /* Wait a while */
        seq_ctl.u64 = cvmx_read_csr(CVMX_LMCX_SEQ_CTL(ddr_interface_num));
    } while (seq_ctl.s.seq_complete != 1);
}

static void
perform_octeon3_ddr3_init_sequence(uint32_t cpu_id, int rank_mask,
                                   int ddr_interface_num)
{
    DECLARE_GLOBAL_DATA_PTR;
#if defined(__U_BOOT__) || defined(unix)
    char *s;
#endif
    int ddr_init_loops = 1;

    while (ddr_init_loops--) {
        cvmx_lmcx_reset_ctl_t lmc_reset_ctl;
        lmc_reset_ctl.u64 = cvmx_read_csr(CVMX_LMCX_RESET_CTL(ddr_interface_num));
        if (lmc_reset_ctl.s.ddr3psv) {
            /* Contents are being preserved */
            /* self-refresh exit */
            perform_octeon3_ddr3_sequence(cpu_id, rank_mask,
                                          ddr_interface_num, 3);

#if defined(__U_BOOT__)
            gd->flags |= GD_FLG_MEMORY_PRESERVED;
#endif
            /* Re-initialize flags */
            lmc_reset_ctl.s.ddr3pwarm = 0;
            lmc_reset_ctl.s.ddr3psoft = 0;
            lmc_reset_ctl.s.ddr3psv   = 0;
            cvmx_write_csr(CVMX_LMCX_RESET_CTL(ddr_interface_num),
                           lmc_reset_ctl.u64);
        } else {
            /* Contents are not being preserved */
            /* power-up/init */
            perform_octeon3_ddr3_sequence(cpu_id, rank_mask,
                                          ddr_interface_num, 0);
        }

        cvmx_wait_usec(1000);   /* Wait a while. */

        if ((s = lookup_env_parameter("ddr_sequence1")) != NULL) {
            int sequence1;
            sequence1 = simple_strtoul(s, NULL, 0);

            perform_octeon3_ddr3_sequence(cpu_id, rank_mask,
                                          ddr_interface_num, sequence1);
        }

        if ((s = lookup_env_parameter("ddr_sequence2")) != NULL) {
            int sequence2;
            sequence2 = simple_strtoul(s, NULL, 0);

            perform_octeon3_ddr3_sequence(cpu_id, rank_mask,
                                          ddr_interface_num, sequence2);
        }
    }
}

static int
octeon3_nonsequential_delays(rlevel_byte_data_t *rlevel_byte,
				int start, int end, int max_adj_delay_inc)
{
    int error = 0;
    int delay_trend, prev_trend = 0;
    int byte_idx;
    int delay_inc;

    for (byte_idx=start; byte_idx<end; ++byte_idx) {
        delay_trend = calc_delay_trend(rlevel_byte[byte_idx+1].delay -
                                       rlevel_byte[byte_idx].delay);
        debug_bitmask_print("Byte %d: %2d, delay_trend: %2d, prev_trend: %2d",
                            byte_idx+1, rlevel_byte[byte_idx+1].delay,
                            delay_trend, prev_trend);
        if ((prev_trend != 0) && (delay_trend != 0) &&
            (prev_trend != delay_trend)) {
            /* Increment error each time the trend changes. */
            error += RLEVEL_NONSEQUENTIAL_DELAY_ERROR;
            prev_trend = delay_trend;
            debug_bitmask_print(" => Nonsequential byte delay");
        }

        delay_inc = _abs(rlevel_byte[byte_idx+1].delay
                         - rlevel_byte[byte_idx].delay);
        if ((max_adj_delay_inc != 0) && (delay_inc > max_adj_delay_inc)) {
            error += (delay_inc - max_adj_delay_inc) * RLEVEL_ADJACENT_DELAY_ERROR;
            debug_bitmask_print(" => Adjacent delay error");
        }

        debug_bitmask_print("\n");
        if (delay_trend != 0)
            prev_trend = delay_trend;
    }
    return error;
}

static int
init_octeon3_ddr3_interface(uint32_t cpu_id,
                            const ddr_configuration_t *ddr_configuration,
                            uint32_t ddr_hertz,
                            uint32_t cpu_hertz,
                            uint32_t ddr_ref_hertz,
                            int board_type,
                            int board_rev_maj,
                            int board_rev_min,
                            int ddr_interface_num,
                            uint32_t ddr_interface_mask
                            )
{
    DECLARE_GLOBAL_DATA_PTR;
#if defined(__U_BOOT__) || defined(unix)
    char *s;
#endif

    const dimm_odt_config_t *odt_1rank_config = ddr_configuration->odt_1rank_config;
    const dimm_odt_config_t *odt_2rank_config = ddr_configuration->odt_2rank_config;
    const dimm_odt_config_t *odt_4rank_config = ddr_configuration->odt_4rank_config;
    const dimm_config_t *dimm_config_table = ddr_configuration->dimm_config_table;
    const dimm_odt_config_t *odt_config;
    const ddr3_custom_config_t *custom_lmc_config = &ddr_configuration->custom_lmc_config;
    int odt_idx;

    /*
    ** Compute clock rates to the nearest picosecond.
    */
    ulong tclk_psecs = divide_nint((uint64_t) 1000*1000*1000*1000, ddr_hertz); /* Clock in psecs */
    ulong eclk_psecs = divide_nint((uint64_t) 1000*1000*1000*1000, cpu_hertz); /* Clock in psecs */

    int row_bits, col_bits, num_banks, num_ranks, dram_width, real_num_ranks;
    int dimm_count = 0;
    int fatal_error = 0;        /* Accumulate and report all the errors before giving up */

    int safe_ddr_flag = 0; /* Flag that indicates safe DDR settings should be used */
    int ddr_interface_64b = 1;  /* Octeon II Default: 64bit interface width */
    int ddr_interface_bytemask;
    uint32_t mem_size_mbytes = 0;
    unsigned int didx;
    int bank_bits = 0;
    int bunk_enable;
    int column_bits_start = 1;
    int row_lsb;
    int pbank_lsb;
    int use_ecc = 1;
    int mtb_psec;
    int tAAmin;
    int tCKmin;
    int CL, min_cas_latency = 0, max_cas_latency = 0, override_cas_latency = 0;
    int ddr_rtt_nom_auto, ddr_rodt_ctl_auto;
    int i;

    int spd_addr;
    int spd_org;
    int spd_rdimm;
    int spd_dimm_type;
    int spd_ecc;
    int spd_cas_latency;
    int spd_mtb_dividend;
    int spd_mtb_divisor;
    int spd_tck_min;
    int spd_taa_min;
    int spd_twr;
    int spd_trcd;
    int spd_trrd;
    int spd_trp;
    int spd_tras;
    int spd_trc;
    int spd_trfc;
    int spd_twtr;
    int spd_trtp;
    int spd_tfaw;
    int spd_addr_mirror;

    int twr;
    int trcd;
    int trrd;
    int trp;
    int tras;
    int trc;
    int trfc;
    int twtr;
    int trtp;
    int tfaw;

    int wlevel_bitmask_errors = 0;
    int wlevel_loops = 0;
    int default_rtt_nom[4];
    int dyn_rtt_nom_mask;


    ddr_print("\nInitializing DDR interface %d, DDR Clock %d, DDR Reference Clock %d, CPUID 0x%08x\n",
              ddr_interface_num, ddr_hertz, ddr_ref_hertz, cpu_id);

    if (dimm_config_table[0].spd_addrs[0] == 0 && !dimm_config_table[0].spd_ptrs[0]) {
        error_print("ERROR: No dimms specified in the dimm_config_table.\n");
        return (-1);
    }

    if (ddr_verbose()) {
        printf("DDR SPD Table:");
        for (didx = 0; didx < DDR_CFG_T_MAX_DIMMS; ++didx) {
            if (dimm_config_table[didx].spd_addrs[0] == 0) break;
            printf(" --ddr%dspd=0x%02x", ddr_interface_num, dimm_config_table[didx].spd_addrs[0]);
            if (dimm_config_table[didx].spd_addrs[1] != 0)
                printf(",0x%02x", dimm_config_table[didx].spd_addrs[1]);
        }
        printf("\n");
    }

    /*
     * Walk the DRAM Socket Configuration Table to see what is installed.
     */
    for (didx = 0; didx < DDR_CFG_T_MAX_DIMMS; ++didx)
    {
        /* Check for lower DIMM socket populated */
        if (validate_dimm(dimm_config_table[didx], 0)) {
            if (ddr_verbose())
                report_ddr3_dimm(dimm_config_table[didx], 0, dimm_count);
            ++dimm_count;
        } else { break; }       /* Finished when there is no lower DIMM */
    }

    if (!odt_1rank_config)
        odt_1rank_config = disable_odt_config;
    if (!odt_2rank_config)
        odt_2rank_config = disable_odt_config;
    if (!odt_4rank_config)
        odt_4rank_config = disable_odt_config;

    if ((s = getenv("ddr_safe")) != NULL) {
        safe_ddr_flag = simple_strtoul(s, NULL, 0);
        error_print("Parameter found in environment. ddr_safe = %d\n", safe_ddr_flag);
    }


    if (dimm_count == 0) {
        error_print("ERROR: DIMM 0 not detected.\n");
        return(-1);
    }

    if (custom_lmc_config->mode32b)
        ddr_interface_64b = 0;

    if ((s = lookup_env_parameter("ddr_interface_64b")) != NULL) {
        ddr_interface_64b = simple_strtoul(s, NULL, 0);
    }

    if (ddr_interface_64b == 0) {
        if ( !(octeon_is_cpuid(OCTEON_CN71XX))) {
            error_print("32-bit interface width is not supported for this Octeon model\n");
            ++fatal_error;
        }
    }

    if (ddr_interface_64b == 1) {
        if (octeon_is_cpuid(OCTEON_CN71XX)) {
            error_print("64-bit interface width is not supported for this Octeon model\n");
            ++fatal_error;
        }
    }

    spd_addr = read_spd(dimm_config_table[0], 0, DDR3_SPD_ADDRESSING_ROW_COL_BITS);
    debug_print("spd_addr        : %#06x\n", spd_addr );
    row_bits = 12 + ((spd_addr >> 3) & 0x7);
    col_bits =  9 + ((spd_addr >> 0) & 0x7);

    num_banks = 1 << (3+((read_spd(dimm_config_table[0], 0, DDR3_SPD_DENSITY_BANKS) >> 4) & 0x7));

    spd_org = read_spd(dimm_config_table[0], 0, DDR3_SPD_MODULE_ORGANIZATION);
    debug_print("spd_org         : %#06x\n", spd_org );
    num_ranks =  1 + ((spd_org >> 3) & 0x7);
    dram_width = 4 << ((spd_org >> 0) & 0x7);



    /* FIX
    ** Check that values are within some theoretical limits.
    ** col_bits(min) = row_lsb(min) - bank_bits(max) - bus_bits(max) = 14 - 3 - 4 = 7
    ** col_bits(max) = row_lsb(max) - bank_bits(min) - bus_bits(min) = 18 - 2 - 3 = 13
    */
    if ((col_bits > 13) || (col_bits < 7)) {
        error_print("Unsupported number of Col Bits: %d\n", col_bits);
        ++fatal_error;
    }

    /* FIX
    ** Check that values are within some theoretical limits.
    ** row_bits(min) = pbank_lsb(min) - row_lsb(max) - rank_bits = 26 - 18 - 1 = 7
    ** row_bits(max) = pbank_lsb(max) - row_lsb(min) - rank_bits = 33 - 14 - 1 = 18
    */
    if ((row_bits > 18) || (row_bits < 7)) {
        error_print("Unsupported number of Row Bits: %d\n", row_bits);
        ++fatal_error;
    }

    if (num_banks == 8)
        bank_bits = 3;
    else if (num_banks == 4)
        bank_bits = 2;



    spd_dimm_type   = read_spd(dimm_config_table[0], 0, DDR3_SPD_KEY_BYTE_MODULE_TYPE);
    spd_rdimm       = (spd_dimm_type == 1) || (spd_dimm_type == 5) || (spd_dimm_type == 9);

    if ((s = lookup_env_parameter("ddr_rdimm_ena")) != NULL) {
        spd_rdimm = simple_strtoul(s, NULL, 0);
    }

    /*  Avoid using h/w write-leveling for pass 1.0 and 1.1 silicon.
        This bug is partially fixed in pass 1.2-1.x. (See LMC-14415B) */
    if (octeon_is_cpuid(OCTEON_CN63XX_PASS1_0) || octeon_is_cpuid(OCTEON_CN63XX_PASS1_1)) {
        ddr_print("Disabling H/W Write-leveling. Re: Pass 1.0-1.1 LMC-14415B\n");
        wlevel_loops = 0;
    } else
        wlevel_loops = 1;

    /* Avoid using h/w write-leveling for pass 1.X silicon with dual
       ranked, registered dimms.  See errata (LMC-14548) Issues with
       registered DIMMs. */
    if (spd_rdimm && (num_ranks==2) && octeon_is_cpuid(OCTEON_CN63XX_PASS1_X))
        wlevel_loops = 0;

    real_num_ranks = num_ranks;

    if (octeon_is_cpuid(OCTEON_CN63XX_PASS1_X)) {
        /* NOTICE: Ignoring second rank of registered DIMMs if using
           h/w write-leveling for pass 1. See errata (LMC-14548)
           Issues with registered DIMMs. */
        num_ranks = (spd_rdimm && wlevel_loops) ? 1 : real_num_ranks;
    }

    if ((s = lookup_env_parameter("ddr_ranks")) != NULL) {
        num_ranks = simple_strtoul(s, NULL, 0);
    }

    if (octeon_is_cpuid(OCTEON_CN63XX_PASS1_X)) {
        /* NOTICE: Ignoring second rank of registered DIMMs if using
           h/w write-leveling for pass 1. See errata (LMC-14548)
           Issues with registered DIMMs. */
        if ((spd_rdimm) && (num_ranks != real_num_ranks))  {
            ddr_print("Ignoring second rank of Registered DIMMs. Re: Pass 1 LMC-14548\n");
        }
    }

    bunk_enable = (num_ranks > 1);

    if (octeon_is_cpuid(OCTEON_CN71XX))
        column_bits_start = 3;
    else
        printf("ERROR: Unsupported Octeon model: 0x%x\n", cpu_id);

    row_lsb = column_bits_start + col_bits + bank_bits - (! ddr_interface_64b);
    debug_print("row_lsb = column_bits_start + col_bits + bank_bits = %d\n", row_lsb);

    pbank_lsb = row_lsb + row_bits + bunk_enable;
    debug_print("pbank_lsb = row_lsb + row_bits + bunk_enable = %d\n", pbank_lsb);


    mem_size_mbytes =  dimm_count * ((1ull << pbank_lsb) >> 20);
    if (num_ranks == 4) {
        /* Quad rank dimm capacity is equivalent to two dual-rank dimms. */
        mem_size_mbytes *= 2;
    }

    /* Mask with 1 bits set for for each active rank, allowing 2 bits per dimm.
    ** This makes later calculations simpler, as a variety of CSRs use this layout.
    ** This init needs to be updated for dual configs (ie non-identical DIMMs).
    ** Bit 0 = dimm0, rank 0
    ** Bit 1 = dimm0, rank 1
    ** Bit 2 = dimm1, rank 0
    ** Bit 3 = dimm1, rank 1
    ** ...
    */
    int rank_mask = 0x1;
    if (num_ranks > 1)
        rank_mask = 0x3;
    if (num_ranks > 2)
        rank_mask = 0xf;

    for (i = 1; i < dimm_count; i++)
        rank_mask |= ((rank_mask & 0x3) << (2*i));



    ddr_print("row bits: %d, col bits: %d, banks: %d, ranks: %d, dram width: %d, size: %d MB\n",
              row_bits, col_bits, num_banks, num_ranks, dram_width, mem_size_mbytes);

#if defined(__U_BOOT__)
    {
        /* If we are booting from RAM, the DRAM controller is already set up.  Just return the
        ** memory size */
        if (gd->flags & GD_FLG_RAM_RESIDENT) {
            ddr_print("Ram Boot: Skipping LMC config\n");
            return mem_size_mbytes;
        }
    }
#endif


    spd_ecc         = !!(read_spd(dimm_config_table[0], 0, DDR3_SPD_MEMORY_BUS_WIDTH) & 8);

    spd_cas_latency  = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_CAS_LATENCIES_LSB);
    spd_cas_latency |= ((0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_CAS_LATENCIES_MSB)) << 8);

    spd_mtb_dividend = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MEDIUM_TIMEBASE_DIVIDEND);
    spd_mtb_divisor  = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MEDIUM_TIMEBASE_DIVISOR);
    spd_tck_min      = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MINIMUM_CYCLE_TIME_TCKMIN);
    spd_taa_min      = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MIN_CAS_LATENCY_TAAMIN);

    spd_twr          = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MIN_WRITE_RECOVERY_TWRMIN);
    spd_trcd         = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MIN_RAS_CAS_DELAY_TRCDMIN);
    spd_trrd         = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MIN_ROW_ACTIVE_DELAY_TRRDMIN);
    spd_trp          = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MIN_ROW_PRECHARGE_DELAY_TRPMIN);
    spd_tras         = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MIN_ACTIVE_PRECHARGE_LSB_TRASMIN);
    spd_tras        |= ((0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_UPPER_NIBBLES_TRAS_TRC)&0xf) << 8);
    spd_trc          = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MIN_ACTIVE_REFRESH_LSB_TRCMIN);
    spd_trc         |= ((0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_UPPER_NIBBLES_TRAS_TRC)&0xf0) << 4);
    spd_trfc         = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MIN_REFRESH_RECOVERY_LSB_TRFCMIN);
    spd_trfc        |= ((0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MIN_REFRESH_RECOVERY_MSB_TRFCMIN)) << 8);
    spd_twtr         = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MIN_INTERNAL_WRITE_READ_CMD_TWTRMIN);
    spd_trtp         = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MIN_INTERNAL_READ_PRECHARGE_CMD_TRTPMIN);
    spd_tfaw         = 0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_MIN_FOUR_ACTIVE_WINDOW_TFAWMIN);
    spd_tfaw        |= ((0xff & read_spd(dimm_config_table[0], 0, DDR3_SPD_UPPER_NIBBLE_TFAW)&0xf) << 8);
    spd_addr_mirror  = 0xff & read_spd(dimm_config_table[0], 0,DDR3_SPD_ADDRESS_MAPPING) & 0x1;
    spd_addr_mirror  = spd_addr_mirror && !spd_rdimm; /* Only address mirror unbuffered dimms.  */

    if ((s = getenv("ddr_use_ecc")) != NULL) {
        use_ecc = simple_strtoul(s, NULL, 0);
        error_print("Parameter found in environment. ddr_use_ecc = %d\n", use_ecc);
    }
    use_ecc = use_ecc && spd_ecc;

    ddr_interface_bytemask = ddr_interface_64b
        ? (use_ecc ? 0x1ff : 0xff)
        : (use_ecc ? 0x01f : 0x0f);

    ddr_print("DRAM Interface width: %d bits %s\n",
              ddr_interface_64b ? 64 : 32, use_ecc ? "+ECC" : "");

    debug_print("spd_cas_latency : %#06x\n", spd_cas_latency );
    debug_print("spd_twr         : %#06x\n", spd_twr );
    debug_print("spd_trcd        : %#06x\n", spd_trcd);
    debug_print("spd_trrd        : %#06x\n", spd_trrd);
    debug_print("spd_trp         : %#06x\n", spd_trp );
    debug_print("spd_tras        : %#06x\n", spd_tras);
    debug_print("spd_trc         : %#06x\n", spd_trc );
    debug_print("spd_trfc        : %#06x\n", spd_trfc);
    debug_print("spd_twtr        : %#06x\n", spd_twtr);
    debug_print("spd_trtp        : %#06x\n", spd_trtp);
    debug_print("spd_tfaw        : %#06x\n", spd_tfaw);
    debug_print("spd_addr_mirror : %#06x\n", spd_addr_mirror);

    ddr_print("\n------ Board Custom Configuration Settings ------\n");
    ddr_print("%-45s : %d\n", "MIN_RTT_NOM_IDX   ", custom_lmc_config->min_rtt_nom_idx);
    ddr_print("%-45s : %d\n", "MAX_RTT_NOM_IDX   ", custom_lmc_config->max_rtt_nom_idx);
    ddr_print("%-45s : %d\n", "MIN_RODT_CTL      ", custom_lmc_config->min_rodt_ctl);
    ddr_print("%-45s : %d\n", "MAX_RODT_CTL      ", custom_lmc_config->max_rodt_ctl);
    ddr_print("%-45s : %d\n", "DQX_CTL           ", custom_lmc_config->dqx_ctl);
    ddr_print("%-45s : %d\n", "CK_CTL            ", custom_lmc_config->ck_ctl);
    ddr_print("%-45s : %d\n", "CMD_CTL           ", custom_lmc_config->cmd_ctl);
    ddr_print("%-45s : %d\n", "MIN_CAS_LATENCY   ", custom_lmc_config->min_cas_latency);
    ddr_print("%-45s : %d\n", "OFFSET_EN         ", custom_lmc_config->offset_en);
    ddr_print("%-45s : %d\n", "OFFSET_UDIMM      ", custom_lmc_config->offset_udimm);
    ddr_print("%-45s : %d\n", "OFFSET_RDIMM      ", custom_lmc_config->offset_rdimm);
    ddr_print("%-45s : %d\n", "DDR_RTT_NOM_AUTO  ", custom_lmc_config->ddr_rtt_nom_auto);
    ddr_print("%-45s : %d\n", "DDR_RODT_CTL_AUTO ", custom_lmc_config->ddr_rodt_ctl_auto);
    ddr_print("%-45s : %d\n", "RLEVEL_COMP_OFFSET", custom_lmc_config->rlevel_comp_offset);
    ddr_print("%-45s : %d\n", "RLEVEL_COMPUTE    ", custom_lmc_config->rlevel_compute);
    ddr_print("%-45s : %d\n", "DDR2T_UDIMM       ", custom_lmc_config->ddr2t_udimm);
    ddr_print("%-45s : %d\n", "DDR2T_RDIMM       ", custom_lmc_config->ddr2t_rdimm);
    ddr_print("%-45s : %d\n", "FPRCH2            ", custom_lmc_config->fprch2);
    ddr_print("-------------------------------------------------\n");

    mtb_psec        = spd_mtb_dividend * 1000 / spd_mtb_divisor;
    tAAmin          = mtb_psec * spd_taa_min;
    tCKmin          = mtb_psec * spd_tck_min;

    CL              = divide_roundup(tAAmin, tclk_psecs);

    ddr_print("Desired CAS Latency                           : %6d\n", CL);

    min_cas_latency = custom_lmc_config->min_cas_latency;


    if ((s = lookup_env_parameter("ddr_min_cas_latency")) != NULL) {
        min_cas_latency = simple_strtoul(s, NULL, 0);
    }

    ddr_print("CAS Latencies supported in DIMM               :");
    for (i=0; i<16; ++i) {
        if ((spd_cas_latency >> i) & 1) {
            ddr_print(" %d", i+4);
            max_cas_latency = i+4;
            if (min_cas_latency == 0)
                min_cas_latency = i+4;
        }
    }
    ddr_print("\n");


    /* Use relaxed timing when running slower than the minimum
       supported speed.  Adjust timing to match the smallest supported
       CAS Latency. */
    if (CL < min_cas_latency) {
        ulong adjusted_tclk = tAAmin / min_cas_latency;
        CL = min_cas_latency;
        ddr_print("Slow clock speed. Adjusting timing: tClk = %d, Adjusted tClk = %d\n",
                  tclk_psecs, adjusted_tclk);
        tclk_psecs = adjusted_tclk;
    }

    if ((s = getenv("ddr_cas_latency")) != NULL) {
        override_cas_latency = simple_strtoul(s, NULL, 0);
        error_print("Parameter found in environment. ddr_cas_latency = %d\n", override_cas_latency);
    }

    /* Make sure that the selected cas latency is legal */
    for (i=(CL-4); i<16; ++i) {
        if ((spd_cas_latency >> i) & 1) {
            CL = i+4;
            break;
        }
    }

    if (CL > max_cas_latency)
        CL = max_cas_latency;

    if (override_cas_latency != 0) {
        CL = override_cas_latency;
    }

    ddr_print("CAS Latency                                   : %6d\n", CL);

    if ((CL * tCKmin) > 20000)
    {
        ddr_print("(CLactual * tCKmin) = %d exceeds 20 ns\n", (CL * tCKmin));
    }

    if (tclk_psecs < (ulong)tCKmin)
        error_print("WARNING!!!!!!: DDR3 Clock Rate (tCLK: %ld) exceeds DIMM specifications (tCKmin:%ld)!!!!!!!!\n",
                    tclk_psecs, (ulong)tCKmin);

    twr             = spd_twr  * mtb_psec;
    trcd            = spd_trcd * mtb_psec;
    trrd            = spd_trrd * mtb_psec;
    trp             = spd_trp  * mtb_psec;
    tras            = spd_tras * mtb_psec;
    trc             = spd_trc  * mtb_psec;
    trfc            = spd_trfc * mtb_psec;
    twtr            = spd_twtr * mtb_psec;
    trtp            = spd_trtp * mtb_psec;
    tfaw            = spd_tfaw * mtb_psec;


    ddr_print("DDR Clock Rate (tCLK)                         : %6d ps\n", tclk_psecs);
    ddr_print("Core Clock Rate (eCLK)                        : %6d ps\n", eclk_psecs);
    ddr_print("Medium Timebase (MTB)                         : %6d ps\n", mtb_psec);
    ddr_print("Minimum Cycle Time (tCKmin)                   : %6d ps\n", tCKmin);
    ddr_print("Minimum CAS Latency Time (tAAmin)             : %6d ps\n", tAAmin);
    ddr_print("Write Recovery Time (tWR)                     : %6d ps\n", twr);
    ddr_print("Minimum RAS to CAS delay (tRCD)               : %6d ps\n", trcd);
    ddr_print("Minimum Row Active to Row Active delay (tRRD) : %6d ps\n", trrd);
    ddr_print("Minimum Row Precharge Delay (tRP)             : %6d ps\n", trp);
    ddr_print("Minimum Active to Precharge (tRAS)            : %6d ps\n", tras);
    ddr_print("Minimum Active to Active/Refresh Delay (tRC)  : %6d ps\n", trc);
    ddr_print("Minimum Refresh Recovery Delay (tRFC)         : %6d ps\n", trfc);
    ddr_print("Internal write to read command delay (tWTR)   : %6d ps\n", twtr);
    ddr_print("Min Internal Rd to Precharge Cmd Delay (tRTP) : %6d ps\n", trtp);
    ddr_print("Minimum Four Activate Window Delay (tFAW)     : %6d ps\n", tfaw);


    if ((num_banks != 4) && (num_banks != 8))
    {
        error_print("Unsupported number of banks %d. Must be 4 or 8.\n", num_banks);
        ++fatal_error;
    }

    if ((num_ranks != 1) && (num_ranks != 2) && (num_ranks != 4))
    {
        error_print("Unsupported number of ranks: %d\n", num_ranks);
        ++fatal_error;
    }

    if ((dram_width != 8) && (dram_width != 16))
    {
        error_print("Unsupported SDRAM Width, %d.  Must be 8 or 16.\n", dram_width);
        ++fatal_error;
    }


    /*
    ** Bail out here if things are not copasetic.
    */
    if (fatal_error)
        return(-1);

    odt_idx = min(dimm_count - 1, 3);

    switch (num_ranks) {
    case 1:
        odt_config = odt_1rank_config;
        break;
    case 2:
        odt_config = odt_2rank_config;
        break;
    case 4:
        odt_config = odt_4rank_config;
        break;
    default:
        odt_config = disable_odt_config;
        error_print("Unsupported number of ranks: %d\n", num_ranks);
        ++fatal_error;
    }

#undef  DDR3_tREFI
#undef  DDR3_ZQCS
#undef  DDR3_ZQCS_Interval
#undef  DDR3_tCKE
#undef  DDR3_tMRD
#undef  DDR3_tDLLK
#undef  DDR3_tMPRR
#undef  DDR3_tWLMRD
#undef  DDR3_tWLDQSEN

    /* Parameters from DDR3 Specifications */
#define DDR3_tREFI         7800000    /* 7.8 us */
#define DDR3_ZQCS          80000ull   /* 80 ns */
#define DDR3_ZQCS_Interval 1280000000 /* 128ms/100 */
#define DDR3_tCKE          5625       /* 5 ns (changed to 5625 for proto) */
#define DDR3_tMRD          4          /* 4 nCK */
#define DDR3_tDLLK         512        /* 512 nCK */
#define DDR3_tMPRR         1          /* 1 nCK */
#define DDR3_tWLMRD        40         /* 40 nCK */
#define DDR3_tWLDQSEN      25         /* 25 nCK */

    /*
     * 4.8.5 Early LMC Initialization
     * 
     * All of DDR PLL, LMC CK, and LMC DRESET initializations must be
     * completed prior to starting this LMC initialization sequence.
     * 
     * Perform the following five substeps for early LMC initialization:
     * 
     * 1. Software must ensure there are no pending DRAM transactions.
     * 
     * 2. Write LMC(0)_CONFIG, LMC(0)_CONTROL, LMC(0)_TIMING_PARAMS0,
     *    LMC(0)_TIMING_PARAMS1, LMC(0)_MODEREG_PARAMS0,
     *    LMC(0)_MODEREG_PARAMS1, LMC(0)_DUAL_MEMCFG, LMC(0)_NXM,
     *    LMC(0)_WODT_MASK, LMC(0)_RODT_MASK, LMC(0)_COMP_CTL2,
     *    LMC(0)_PHY_CTL, LMC(0)_DIMM0/1_PARAMS, and LMC(0)_DIMM_CTL with
     *    appropriate values. All sections in this chapter can be used to
     *    derive proper register settings.
     */

    /* LMC(0)_CONFIG */
    {
        /* .cn70xx. */
        cvmx_lmcx_config_t lmcx_config;

        lmcx_config.u64 = 0;

        lmcx_config.cn70xx.ecc_ena         = use_ecc;
        lmcx_config.cn70xx.row_lsb         = encode_row_lsb_ddr3(cpu_id, row_lsb, ddr_interface_64b);
        lmcx_config.cn70xx.pbank_lsb       = encode_pbank_lsb_ddr3(cpu_id, pbank_lsb, ddr_interface_64b);

        lmcx_config.cn70xx.idlepower       = 0; /* Disabled */

        if ((s = lookup_env_parameter("ddr_idlepower")) != NULL) {
            lmcx_config.cn70xx.idlepower = simple_strtoul(s, NULL, 0);
        }

        lmcx_config.cn70xx.forcewrite      = 0; /* Disabled */
        lmcx_config.cn70xx.ecc_adr         = 1; /* Include memory reference address in the ECC */

        if ((s = lookup_env_parameter("ddr_ecc_adr")) != NULL) {
            lmcx_config.cn70xx.ecc_adr = simple_strtoul(s, NULL, 0);
        }

        lmcx_config.cn70xx.reset           = 0;

        /*
         *  Program LMC0_CONFIG[24:18], ref_zqcs_int(6:0) to
         *  RND-DN(tREFI/clkPeriod/512) Program LMC0_CONFIG[36:25],
         *  ref_zqcs_int(18:7) to
         *  RND-DN(ZQCS_Interval/clkPeriod/(512*128)). Note that this
         *  value should always be greater than 32, to account for
         *  resistor calibration delays.
         */

        lmcx_config.cn70xx.ref_zqcs_int     = ((DDR3_tREFI/tclk_psecs/512) & 0x7f);
        lmcx_config.cn70xx.ref_zqcs_int    |= ((max(33ull, (DDR3_ZQCS_Interval/(tclk_psecs/100)/(512*128))) & 0xfff) << 7);


        lmcx_config.cn70xx.early_dqx       = 0; /* disabled */
        lmcx_config.cn70xx.sref_with_dll        = 0;

        lmcx_config.cn70xx.rank_ena        = bunk_enable;
        lmcx_config.cn70xx.rankmask        = rank_mask; /* Set later */
        lmcx_config.cn70xx.mirrmask        = (spd_addr_mirror << 1 | spd_addr_mirror << 3) & rank_mask;
        lmcx_config.cn70xx.init_status     = rank_mask; /* Set once and don't change it. */
        lmcx_config.cn70xx.early_unload_d0_r0   = 0;
        lmcx_config.cn70xx.early_unload_d0_r1   = 0;
        lmcx_config.cn70xx.early_unload_d1_r0   = 0;
        lmcx_config.cn70xx.early_unload_d1_r1   = 0;
        lmcx_config.cn70xx.scrz                 = 0;
        lmcx_config.cn70xx.mode32b              = 1; /* Read-only. Always 1. */
        lmcx_config.cn70xx.mode_x4dev           = 0;
        lmcx_config.cn70xx.bg2_enable           = 0;

        if ((s = lookup_env_parameter_ull("ddr_config")) != NULL) {
            lmcx_config.u64    = simple_strtoull(s, NULL, 0);
        }
        ddr_print("LMC_CONFIG                                    : 0x%016llx\n", lmcx_config.u64);
        cvmx_write_csr(CVMX_LMCX_CONFIG(ddr_interface_num), lmcx_config.u64);
    }

    /* LMC(0)_CONTROL */
    {
        /* .s. */
        cvmx_lmcx_control_t lmc_control;
        lmc_control.u64 = cvmx_read_csr(CVMX_LMCX_CONTROL(ddr_interface_num));
        lmc_control.s.rdimm_ena       = spd_rdimm;
        lmc_control.s.bwcnt           = 0; /* Clear counter later */
        if (spd_rdimm)
            lmc_control.s.ddr2t       = (safe_ddr_flag ? 1 : custom_lmc_config->ddr2t_rdimm );
        else
            lmc_control.s.ddr2t       = (safe_ddr_flag ? 1 : custom_lmc_config->ddr2t_udimm );
        lmc_control.s.pocas           = 0;
        lmc_control.s.fprch2          = (safe_ddr_flag ? 2 : custom_lmc_config->fprch2 );
        lmc_control.s.throttle_rd     = safe_ddr_flag ? 1 : 0;
        lmc_control.s.throttle_wr     = safe_ddr_flag ? 1 : 0;
        lmc_control.s.inorder_rd      = safe_ddr_flag ? 1 : 0;
        lmc_control.s.inorder_wr      = safe_ddr_flag ? 1 : 0;
        lmc_control.s.elev_prio_dis   = safe_ddr_flag ? 1 : 0;
        lmc_control.s.nxm_write_en    = 0; /* discards writes to
                                            addresses that don't exist
                                            in the DRAM */
        lmc_control.s.max_write_batch = 8;
        lmc_control.s.xor_bank        = 1;
        lmc_control.s.auto_dclkdis    = 1;
        lmc_control.s.int_zqcs_dis    = 0;
        lmc_control.s.ext_zqcs_dis    = 0;
        lmc_control.s.bprch           = 0;
        lmc_control.s.wodt_bprch      = 0;
        lmc_control.s.rodt_bprch      = 0;

        if ((s = lookup_env_parameter("ddr_xor_bank")) != NULL) {
            lmc_control.s.xor_bank = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_2t")) != NULL) {
            lmc_control.s.ddr2t = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_fprch2")) != NULL) {
            lmc_control.s.fprch2 = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_bprch")) != NULL) {
            lmc_control.s.bprch = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_wodt_bprch")) != NULL) {
            lmc_control.s.wodt_bprch = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_rodt_bprch")) != NULL) {
            lmc_control.s.rodt_bprch = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_int_zqcs_dis")) != NULL) {
            lmc_control.s.int_zqcs_dis = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_ext_zqcs_dis")) != NULL) {
            lmc_control.s.ext_zqcs_dis = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter_ull("ddr_control")) != NULL) {
            lmc_control.u64    = simple_strtoull(s, NULL, 0);
        }
        ddr_print("LMC_CONTROL                                   : 0x%016llx\n", lmc_control.u64);
        cvmx_write_csr(CVMX_LMCX_CONTROL(ddr_interface_num), lmc_control.u64);
    }

    /* LMC(0)_TIMING_PARAMS0 */
    {
        unsigned trp_value;
        cvmx_lmcx_timing_params0_t lmc_timing_params0;
        lmc_timing_params0.u64 = cvmx_read_csr(CVMX_LMCX_TIMING_PARAMS0(ddr_interface_num));

        trp_value = divide_roundup(trp, tclk_psecs)
            + divide_roundup(max(4*tclk_psecs, 7500ull), tclk_psecs) - 5;

        /* .cn70xx. */
        lmc_timing_params0.cn70xx.tzqcs    = divide_roundup(max(64*tclk_psecs, DDR3_ZQCS), (16*tclk_psecs));
        lmc_timing_params0.cn70xx.tcke     = divide_roundup(DDR3_tCKE, tclk_psecs) - 1;
        lmc_timing_params0.cn70xx.txpr     = divide_roundup(max(5*tclk_psecs, trfc+10000ull), 16*tclk_psecs);
        lmc_timing_params0.cn70xx.tmrd     = divide_roundup((DDR3_tMRD*tclk_psecs), tclk_psecs) - 1;
        lmc_timing_params0.cn70xx.tmod     = divide_roundup(max(12*tclk_psecs, 15000ull), tclk_psecs) - 1;
        lmc_timing_params0.cn70xx.tdllk    = divide_roundup(DDR3_tDLLK, 256);
        lmc_timing_params0.cn70xx.tzqinit  = divide_roundup(max(512*tclk_psecs, 640000ull), (256*tclk_psecs));
        lmc_timing_params0.cn70xx.trp      = trp_value & 0x1f;
        lmc_timing_params0.cn70xx.tcksre   = divide_roundup(max(5*tclk_psecs, 10000ull), tclk_psecs) - 1;

        if ((s = lookup_env_parameter_ull("ddr_timing_params0")) != NULL) {
            lmc_timing_params0.u64    = simple_strtoull(s, NULL, 0);
        }
        ddr_print("TIMING_PARAMS0                                : 0x%016llx\n", lmc_timing_params0.u64);
        cvmx_write_csr(CVMX_LMCX_TIMING_PARAMS0(ddr_interface_num), lmc_timing_params0.u64);
    }

    /* LMC(0)_TIMING_PARAMS1 */
    {
        int txp;
        cvmx_lmcx_timing_params1_t lmc_timing_params1;
        lmc_timing_params1.u64 = cvmx_read_csr(CVMX_LMCX_TIMING_PARAMS1(ddr_interface_num));


        /* .cn70xx. */
        lmc_timing_params1.s.tmprr    = divide_roundup(DDR3_tMPRR*tclk_psecs, tclk_psecs);

        lmc_timing_params1.cn70xx.tras     = divide_roundup(tras, tclk_psecs) - 1;
        lmc_timing_params1.cn70xx.trcd     = divide_roundup(trcd, tclk_psecs);
        lmc_timing_params1.cn70xx.twtr     = divide_roundup(twtr, tclk_psecs) - 1;
        lmc_timing_params1.cn70xx.trfc     = divide_roundup(trfc, 8*tclk_psecs);
        lmc_timing_params1.cn70xx.trrd     = divide_roundup(trrd, tclk_psecs) - 2;

        /*
        ** tXP = max( 3nCK, 7.5 ns)     DDR3-800   tCLK = 2500 psec
        ** tXP = max( 3nCK, 7.5 ns)     DDR3-1066  tCLK = 1875 psec
        ** tXP = max( 3nCK, 6.0 ns)     DDR3-1333  tCLK = 1500 psec
        ** tXP = max( 3nCK, 6.0 ns)     DDR3-1600  tCLK = 1250 psec
        ** tXP = max( 3nCK, 6.0 ns)     DDR3-1866  tCLK = 1071 psec
        ** tXP = max( 3nCK, 6.0 ns)     DDR3-2133  tCLK =  937 psec
        */
        txp = (tclk_psecs < 1875) ? 6000 : 7500;
        lmc_timing_params1.cn70xx.txp      = divide_roundup(max(3*tclk_psecs, (unsigned)txp), tclk_psecs) - 1;

        lmc_timing_params1.cn70xx.twlmrd   = divide_roundup(DDR3_tWLMRD*tclk_psecs, 4*tclk_psecs);
        lmc_timing_params1.cn70xx.twldqsen = divide_roundup(DDR3_tWLDQSEN*tclk_psecs, 4*tclk_psecs);
        lmc_timing_params1.cn70xx.tfaw     = divide_roundup(tfaw, 4*tclk_psecs);
        lmc_timing_params1.cn70xx.txpdll   = divide_roundup(max(10*tclk_psecs, 24000ull), tclk_psecs) - 1;

        if ((s = lookup_env_parameter_ull("ddr_timing_params1")) != NULL) {
            lmc_timing_params1.u64    = simple_strtoull(s, NULL, 0);
        }
        ddr_print("TIMING_PARAMS1                                : 0x%016llx\n", lmc_timing_params1.u64);
        cvmx_write_csr(CVMX_LMCX_TIMING_PARAMS1(ddr_interface_num), lmc_timing_params1.u64);
    }

    /* LMC(0)_MODEREG_PARAMS0 */
    {
        /* .s. */
        cvmx_lmcx_modereg_params0_t lmc_modereg_params0;
        int param;

        lmc_modereg_params0.u64 = cvmx_read_csr(CVMX_LMCX_MODEREG_PARAMS0(ddr_interface_num));


        /*
        ** CSR   CWL         CAS write Latency
        ** ===   ===   =================================
        **  0      5   (           tCK(avg) >=   2.5 ns)
        **  1      6   (2.5 ns   > tCK(avg) >= 1.875 ns)
        **  2      7   (1.875 ns > tCK(avg) >= 1.5   ns)
        **  3      8   (1.5 ns   > tCK(avg) >= 1.25  ns)
        **  4      9   (1.25 ns  > tCK(avg) >= 1.07  ns)
        **  5     10   (1.07 ns  > tCK(avg) >= 0.935 ns)
        **  6     11   (0.935 ns > tCK(avg) >= 0.833 ns)
        **  7     12   (0.833 ns > tCK(avg) >= 0.75  ns)
        */

        lmc_modereg_params0.s.cwl     = 0;
        if (tclk_psecs < 2500)
            lmc_modereg_params0.s.cwl = 1;
        if (tclk_psecs < 1875)
            lmc_modereg_params0.s.cwl = 2;
        if (tclk_psecs < 1500)
            lmc_modereg_params0.s.cwl = 3;
        if (tclk_psecs < 1250)
            lmc_modereg_params0.s.cwl = 4;
        if (tclk_psecs < 1070)
            lmc_modereg_params0.s.cwl = 5;
        if (tclk_psecs <  935)
            lmc_modereg_params0.s.cwl = 6;
        if (tclk_psecs <  833)
            lmc_modereg_params0.s.cwl = 7;

        if ((s = lookup_env_parameter("ddr_cwl")) != NULL) {
            lmc_modereg_params0.s.cwl = simple_strtoul(s, NULL, 0) - 5;
        }

        ddr_print("CAS Write Latency                             : %6d\n", lmc_modereg_params0.s.cwl + 5);

        lmc_modereg_params0.s.mprloc  = 0;
        lmc_modereg_params0.s.mpr     = 0;
        lmc_modereg_params0.s.dll     = 0;
        lmc_modereg_params0.s.al      = 0;
        lmc_modereg_params0.s.wlev    = 0; /* Read Only */
        lmc_modereg_params0.s.tdqs    = 0;
        lmc_modereg_params0.s.qoff    = 0;
        lmc_modereg_params0.s.bl      = 0; /* Read Only */

        if ((s = lookup_env_parameter("ddr_cl")) != NULL) {
            CL = simple_strtoul(s, NULL, 0);
            ddr_print("CAS Latency                                   : %6d\n", CL);
        }

        lmc_modereg_params0.s.cl      = 0x2;
        if (CL > 5)
            lmc_modereg_params0.s.cl  = 0x4;
        if (CL > 6)
            lmc_modereg_params0.s.cl  = 0x6;
        if (CL > 7)
            lmc_modereg_params0.s.cl  = 0x8;
        if (CL > 8)
            lmc_modereg_params0.s.cl  = 0xA;
        if (CL > 9)
            lmc_modereg_params0.s.cl  = 0xC;
        if (CL > 10)
            lmc_modereg_params0.s.cl  = 0xE;
        if (CL > 11)
            lmc_modereg_params0.s.cl  = 0x1;
        if (CL > 12)
            lmc_modereg_params0.s.cl  = 0x3;
        if (CL > 13)
            lmc_modereg_params0.s.cl  = 0x5;
        if (CL > 14)
            lmc_modereg_params0.s.cl  = 0x7;
        if (CL > 15)
            lmc_modereg_params0.s.cl  = 0x9;

        lmc_modereg_params0.s.rbt     = 0; /* Read Only. */
        lmc_modereg_params0.s.tm      = 0;
        lmc_modereg_params0.s.dllr    = 0;

        param = divide_roundup(twr, tclk_psecs);
        lmc_modereg_params0.s.wrp     = 1;
        if (param > 5)
            lmc_modereg_params0.s.wrp = 2;
        if (param > 6)
            lmc_modereg_params0.s.wrp = 3;
        if (param > 7)
            lmc_modereg_params0.s.wrp = 4;
        if (param > 8)
            lmc_modereg_params0.s.wrp = 5;
        if (param > 10)
            lmc_modereg_params0.s.wrp = 6;
        if (param > 12)
            lmc_modereg_params0.s.wrp = 7;

        lmc_modereg_params0.s.ppd     = 0;

        if ((s = lookup_env_parameter("ddr_wrp")) != NULL) {
            lmc_modereg_params0.s.wrp = simple_strtoul(s, NULL, 0);
        }

        ddr_print("Write recovery for auto precharge (WRP)       : %6d\n", lmc_modereg_params0.s.wrp);

        if ((s = lookup_env_parameter_ull("ddr_modereg_params0")) != NULL) {
            lmc_modereg_params0.u64    = simple_strtoull(s, NULL, 0);
        }
        ddr_print("MODEREG_PARAMS0                               : 0x%016llx\n", lmc_modereg_params0.u64);
        cvmx_write_csr(CVMX_LMCX_MODEREG_PARAMS0(ddr_interface_num), lmc_modereg_params0.u64);
    }

    /* LMC(0)_MODEREG_PARAMS1 */
    {
        /* .s. */
        cvmx_lmcx_modereg_params1_t lmc_modereg_params1;
        lmc_modereg_params1.u64 = odt_config[odt_idx].odt_mask1.u64;
        static const unsigned char rtt_nom_ohms[] = { 0, 60, 120, 40, 20, 30, 0, 0};
        static const unsigned char rtt_wr_ohms[]  = { 0, 60, 120, 0};
        static const unsigned char dic_ohms[]     = { 40, 34, 0, 0 };


        if ((s = lookup_env_parameter("ddr_rtt_nom_mask")) != NULL) {
            dyn_rtt_nom_mask    = simple_strtoul(s, NULL, 0);
        }


        /* Save the original rtt_nom settings before sweeping through settings. */
        default_rtt_nom[0] = lmc_modereg_params1.s.rtt_nom_00;
        default_rtt_nom[1] = lmc_modereg_params1.s.rtt_nom_01;
        default_rtt_nom[2] = lmc_modereg_params1.s.rtt_nom_10;
        default_rtt_nom[3] = lmc_modereg_params1.s.rtt_nom_11;

        ddr_rtt_nom_auto = custom_lmc_config->ddr_rtt_nom_auto;


        ddr_print("RTT_NOM     %3d, %3d, %3d, %3d ohms           :  %x,%x,%x,%x\n",
                  rtt_nom_ohms[lmc_modereg_params1.s.rtt_nom_11],
                  rtt_nom_ohms[lmc_modereg_params1.s.rtt_nom_10],
                  rtt_nom_ohms[lmc_modereg_params1.s.rtt_nom_01],
                  rtt_nom_ohms[lmc_modereg_params1.s.rtt_nom_00],
                  lmc_modereg_params1.s.rtt_nom_11,
                  lmc_modereg_params1.s.rtt_nom_10,
                  lmc_modereg_params1.s.rtt_nom_01,
                  lmc_modereg_params1.s.rtt_nom_00);

        ddr_print("RTT_WR      %3d, %3d, %3d, %3d ohms           :  %x,%x,%x,%x\n",
                  rtt_wr_ohms[lmc_modereg_params1.s.rtt_wr_11],
                  rtt_wr_ohms[lmc_modereg_params1.s.rtt_wr_10],
                  rtt_wr_ohms[lmc_modereg_params1.s.rtt_wr_01],
                  rtt_wr_ohms[lmc_modereg_params1.s.rtt_wr_00],
                  lmc_modereg_params1.s.rtt_wr_11,
                  lmc_modereg_params1.s.rtt_wr_10,
                  lmc_modereg_params1.s.rtt_wr_01,
                  lmc_modereg_params1.s.rtt_wr_00);

        ddr_print("DIC         %3d, %3d, %3d, %3d ohms           :  %x,%x,%x,%x\n",
                  dic_ohms[lmc_modereg_params1.s.dic_11],
                  dic_ohms[lmc_modereg_params1.s.dic_10],
                  dic_ohms[lmc_modereg_params1.s.dic_01],
                  dic_ohms[lmc_modereg_params1.s.dic_00],
                  lmc_modereg_params1.s.dic_11,
                  lmc_modereg_params1.s.dic_10,
                  lmc_modereg_params1.s.dic_01,
                  lmc_modereg_params1.s.dic_00);

        ddr_print("MODEREG_PARAMS1                               : 0x%016llx\n", lmc_modereg_params1.u64);
        cvmx_write_csr(CVMX_LMCX_MODEREG_PARAMS1(ddr_interface_num), lmc_modereg_params1.u64);
    }

    /* LMC(0)_NXM */
    {
        cvmx_lmcx_nxm_t lmc_nxm;
        lmc_nxm.u64 = cvmx_read_csr(CVMX_LMCX_NXM(ddr_interface_num));

        /* .cn70xx. */
        if (rank_mask & 0x1)
            lmc_nxm.cn70xx.mem_msb_d0_r0 = row_lsb + row_bits - 26;
        if (rank_mask & 0x2)
            lmc_nxm.cn70xx.mem_msb_d0_r1 = row_lsb + row_bits - 26;
        if (rank_mask & 0x4)
            lmc_nxm.cn70xx.mem_msb_d1_r0 = row_lsb + row_bits - 26;
        if (rank_mask & 0x8)
            lmc_nxm.cn70xx.mem_msb_d1_r1 = row_lsb + row_bits - 26;

        lmc_nxm.cn70xx.cs_mask = ~rank_mask & 0xff; /* Set the mask for non-existant ranks. */

       if ((s = lookup_env_parameter_ull("ddr_nxm")) != NULL) {
            lmc_nxm.u64    = simple_strtoull(s, NULL, 0);
        }
        ddr_print("LMC_NXM                                       : 0x%016llx\n", lmc_nxm.u64);
        cvmx_write_csr(CVMX_LMCX_NXM(ddr_interface_num), lmc_nxm.u64);
    }

    /* LMC(0)_WODT_MASK */
    {
        cvmx_lmcx_wodt_mask_t lmc_wodt_mask;
        lmc_wodt_mask.u64 = odt_config[odt_idx].odt_mask;

        if ((s = lookup_env_parameter_ull("ddr_wodt_mask")) != NULL) {
            lmc_wodt_mask.u64    = simple_strtoull(s, NULL, 0);
        }

        ddr_print("WODT_MASK                                     : 0x%016llx\n", lmc_wodt_mask.u64);
        cvmx_write_csr(CVMX_LMCX_WODT_MASK(ddr_interface_num), lmc_wodt_mask.u64);
    }

    /* LMC(0)_RODT_MASK */
    {
        int rankx;
        cvmx_lmcx_rodt_mask_t lmc_rodt_mask;
        lmc_rodt_mask.u64 = odt_config[odt_idx].rodt_ctl;

        if ((s = lookup_env_parameter_ull("ddr_rodt_mask")) != NULL) {
            lmc_rodt_mask.u64    = simple_strtoull(s, NULL, 0);
        }

        ddr_print("%-45s : 0x%016llx\n", "RODT_MASK", lmc_rodt_mask.u64);
       cvmx_write_csr(CVMX_LMCX_RODT_MASK(ddr_interface_num), lmc_rodt_mask.u64);

        dyn_rtt_nom_mask = 0;
        for (rankx = 0; rankx < dimm_count * 4;rankx++) {
            if (!(rank_mask & (1 << rankx)))
                continue;
            dyn_rtt_nom_mask |= ((lmc_rodt_mask.u64 >> (8*rankx)) & 0xff);
        }
        if (num_ranks == 4) {
            /* Normally ODT1 is wired to rank 1. For quad-ranked DIMMs
               ODT1 is wired to the third rank (rank 2).  The mask,
               dyn_rtt_nom_mask, is used to indicate for which ranks
               to sweep RTT_NOM during read-leveling. Shift the bit
               from the ODT1 position over to the "ODT2" position so
               that the read-leveling analysis comes out right. */
            int odt1_bit = dyn_rtt_nom_mask & 2;
            dyn_rtt_nom_mask &= ~2;
            dyn_rtt_nom_mask |= odt1_bit<<1;
        }
        ddr_print("%-45s : 0x%02x\n", "DYN_RTT_NOM_MASK", dyn_rtt_nom_mask);
    }

    /* LMC(0)_COMP_CTL2 */
    {
        /* .cn70xx. */

        cvmx_lmcx_comp_ctl2_t comp_ctl2;
        const ddr3_custom_config_t *custom_lmc_config = &ddr_configuration->custom_lmc_config;

        comp_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num));

        comp_ctl2.cn70xx.dqx_ctl	= (custom_lmc_config->dqx_ctl == 0) ? 4 : custom_lmc_config->dqx_ctl; /* Default 4=34.3 ohm */
        comp_ctl2.cn70xx.ck_ctl		= (custom_lmc_config->ck_ctl  == 0) ? 4 : custom_lmc_config->ck_ctl;  /* Default 4=34.3 ohm */
        comp_ctl2.cn70xx.cmd_ctl	= (custom_lmc_config->cmd_ctl == 0) ? 4 : custom_lmc_config->cmd_ctl; /* Default 4=34.3 ohm */
        comp_ctl2.cn70xx.control_ctl	= (custom_lmc_config->ctl_ctl == 0) ? 4 : custom_lmc_config->ctl_ctl; /* Default 4=34.3 ohm */

        comp_ctl2.cn70xx.ntune_offset    = 0;
        comp_ctl2.cn70xx.ptune_offset    = 0;

        comp_ctl2.cn70xx.rodt_ctl           = 0x4; /* 60 ohm */

        if ((s = lookup_env_parameter("ddr_clk_ctl")) != NULL) {
            comp_ctl2.cn70xx.ck_ctl  = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_ck_ctl")) != NULL) {
            comp_ctl2.cn70xx.ck_ctl  = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_cmd_ctl")) != NULL) {
            comp_ctl2.cn70xx.cmd_ctl  = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_dqx_ctl")) != NULL) {
            comp_ctl2.cn70xx.dqx_ctl  = simple_strtoul(s, NULL, 0);
        }

        cvmx_write_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num), comp_ctl2.u64);
    }

    /* LMC(0)_PHY_CTL */
    {
        /* .s. */
        cvmx_lmcx_phy_ctl_t lmc_phy_ctl;
        lmc_phy_ctl.u64 = cvmx_read_csr(CVMX_LMCX_PHY_CTL(ddr_interface_num));
        lmc_phy_ctl.s.ts_stagger           = 0;

        if ((s = lookup_env_parameter_ull("ddr_phy_ctl")) != NULL) {
            lmc_phy_ctl.u64    = simple_strtoull(s, NULL, 0);
        }

        ddr_print("PHY_CTL                                       : 0x%016llx\n", lmc_phy_ctl.u64);
        cvmx_write_csr(CVMX_LMCX_PHY_CTL(ddr_interface_num), lmc_phy_ctl.u64);
    }

    /* LMC(0)_DIMM0/1_PARAMS */
    if (spd_rdimm) {
        for (didx=0; didx<(unsigned)dimm_count; ++didx) {
        cvmx_lmcx_dimmx_params_t lmc_dimmx_params;
        cvmx_lmcx_dimm_ctl_t lmc_dimm_ctl;
        int dimm = didx;
        lmc_dimmx_params.u64 = cvmx_read_csr(CVMX_LMCX_DIMMX_PARAMS(dimm, ddr_interface_num));
        int rc;

        rc = read_spd(dimm_config_table[didx], 0, 69);
        lmc_dimmx_params.s.rc0         = (rc >> 0) & 0xf;
        lmc_dimmx_params.s.rc1         = (rc >> 4) & 0xf;

        rc = read_spd(dimm_config_table[didx], 0, 70);
        lmc_dimmx_params.s.rc2         = (rc >> 0) & 0xf;
        lmc_dimmx_params.s.rc3         = (rc >> 4) & 0xf;

        rc = read_spd(dimm_config_table[didx], 0, 71);
        lmc_dimmx_params.s.rc4         = (rc >> 0) & 0xf;
        lmc_dimmx_params.s.rc5         = (rc >> 4) & 0xf;

        rc = read_spd(dimm_config_table[didx], 0, 72);
        lmc_dimmx_params.s.rc6         = (rc >> 0) & 0xf;
        lmc_dimmx_params.s.rc7         = (rc >> 4) & 0xf;

        rc = read_spd(dimm_config_table[didx], 0, 73);
        lmc_dimmx_params.s.rc8         = (rc >> 0) & 0xf;
        lmc_dimmx_params.s.rc9         = (rc >> 4) & 0xf;

        rc = read_spd(dimm_config_table[didx], 0, 74);
        lmc_dimmx_params.s.rc10        = (rc >> 0) & 0xf;
        lmc_dimmx_params.s.rc11        = (rc >> 4) & 0xf;

        rc = read_spd(dimm_config_table[didx], 0, 75);
        lmc_dimmx_params.s.rc12        = (rc >> 0) & 0xf;
        lmc_dimmx_params.s.rc13        = (rc >> 4) & 0xf;

        rc = read_spd(dimm_config_table[didx], 0, 76);
        lmc_dimmx_params.s.rc14        = (rc >> 0) & 0xf;
        lmc_dimmx_params.s.rc15        = (rc >> 4) & 0xf;


        if ((s = ddr_getenv_debug("ddr_clk_drive")) != NULL) {
            if (strcmp(s,"light") == 0) {
                lmc_dimmx_params.s.rc5         = 0x0; /* Light Drive */
            }
            if (strcmp(s,"moderate") == 0) {
                lmc_dimmx_params.s.rc5         = 0x5; /* Moderate Drive */
            }
            if (strcmp(s,"strong") == 0) {
                lmc_dimmx_params.s.rc5         = 0xA; /* Strong Drive */
            }
            error_print("Parameter found in environment. ddr_clk_drive = %s\n", s);
        }

        if ((s = ddr_getenv_debug("ddr_cmd_drive")) != NULL) {
            if (strcmp(s,"light") == 0) {
                lmc_dimmx_params.s.rc3         = 0x0; /* Light Drive */
            }
            if (strcmp(s,"moderate") == 0) {
                lmc_dimmx_params.s.rc3         = 0x5; /* Moderate Drive */
            }
            if (strcmp(s,"strong") == 0) {
                lmc_dimmx_params.s.rc3         = 0xA; /* Strong Drive */
            }
            error_print("Parameter found in environment. ddr_cmd_drive = %s\n", s);
        }

        if ((s = ddr_getenv_debug("ddr_ctl_drive")) != NULL) {
            if (strcmp(s,"light") == 0) {
                lmc_dimmx_params.s.rc4         = 0x0; /* Light Drive */
            }
            if (strcmp(s,"moderate") == 0) {
                lmc_dimmx_params.s.rc4         = 0x5; /* Moderate Drive */
            }
            error_print("Parameter found in environment. ddr_ctl_drive = %s\n", s);
        }


        /*
        ** rc10               RDIMM Operating Speed
        ** ====   =========================================================
        **  0                 tclk_psecs > 2500 psec DDR3/DDR3L-800 (default)
        **  1     2500 psec > tclk_psecs > 1875 psec DDR3/DDR3L-1066
        **  2     1875 psec > tclk_psecs > 1500 psec DDR3/DDR3L-1333
        **  3     1500 psec > tclk_psecs > 1250 psec DDR3/DDR3L-1600
        */
        lmc_dimmx_params.s.rc10        = 3;
        if (tclk_psecs >= 1250)
            lmc_dimmx_params.s.rc10    = 3;
        if (tclk_psecs >= 1500)
            lmc_dimmx_params.s.rc10    = 2;
        if (tclk_psecs >= 1875)
            lmc_dimmx_params.s.rc10    = 1;
        if (tclk_psecs >= 2500)
            lmc_dimmx_params.s.rc10    = 0;

        cvmx_write_csr(CVMX_LMCX_DIMMX_PARAMS(dimm, ddr_interface_num), lmc_dimmx_params.u64);

        ddr_print("DIMM%d Register Control Words         RC15:RC0 : %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",
                  dimm,
                  lmc_dimmx_params.s.rc15,
                  lmc_dimmx_params.s.rc14,
                  lmc_dimmx_params.s.rc13,
                  lmc_dimmx_params.s.rc12,
                  lmc_dimmx_params.s.rc11,
                  lmc_dimmx_params.s.rc10,
                  lmc_dimmx_params.s.rc9 ,
                  lmc_dimmx_params.s.rc8 ,
                  lmc_dimmx_params.s.rc7 ,
                  lmc_dimmx_params.s.rc6 ,
                  lmc_dimmx_params.s.rc5 ,
                  lmc_dimmx_params.s.rc4 ,
                  lmc_dimmx_params.s.rc3 ,
                  lmc_dimmx_params.s.rc2 ,
                  lmc_dimmx_params.s.rc1 ,
                  lmc_dimmx_params.s.rc0 );


        /* LMC0_DIMM_CTL */
        lmc_dimm_ctl.u64 = cvmx_read_csr(CVMX_LMCX_DIMM_CTL(ddr_interface_num));
        lmc_dimm_ctl.s.dimm0_wmask         = 0xffff;
        lmc_dimm_ctl.s.dimm1_wmask         = (dimm_count > 1) ? 0xffff : 0x0000;
        lmc_dimm_ctl.s.tcws                = 0x4e0;
        lmc_dimm_ctl.s.parity              = custom_lmc_config->parity;

        if ((s = lookup_env_parameter("ddr_dimm0_wmask")) != NULL) {
            lmc_dimm_ctl.s.dimm0_wmask    = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_dimm1_wmask")) != NULL) {
            lmc_dimm_ctl.s.dimm1_wmask    = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_dimm_ctl_parity")) != NULL) {
            lmc_dimm_ctl.s.parity = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_dimm_ctl_tcws")) != NULL) {
            lmc_dimm_ctl.s.tcws = simple_strtoul(s, NULL, 0);
        }

        ddr_print("DIMM%d DIMM_CTL                                : 0x%016llx\n", dimm, lmc_dimm_ctl.u64);
        cvmx_write_csr(CVMX_LMCX_DIMM_CTL(ddr_interface_num), lmc_dimm_ctl.u64);
    }
    } else {
        /* Disable register control writes for unbuffered */
        cvmx_lmcx_dimm_ctl_t lmc_dimm_ctl;
        lmc_dimm_ctl.u64 = cvmx_read_csr(CVMX_LMCX_DIMM_CTL(ddr_interface_num));
        lmc_dimm_ctl.s.dimm0_wmask         = 0;
        lmc_dimm_ctl.s.dimm1_wmask         = 0;
        cvmx_write_csr(CVMX_LMCX_DIMM_CTL(ddr_interface_num), lmc_dimm_ctl.u64);
    }

    /*
     * Comments (steps 3 through 5) continue in perform_octeon3_ddr3_sequence()
     */

    perform_octeon3_ddr3_init_sequence(cpu_id, rank_mask, ddr_interface_num);

    /*
     * 4.8.6 LMC Offset Training
     * 
     * LMC requires input-receiver offset training.
     * 
     * 1. Write LMC(0)_PHY_CTL[DAC_ON] = 1
     */

    {
        cvmx_lmcx_phy_ctl_t lmc_phy_ctl;
        lmc_phy_ctl.u64 = cvmx_read_csr(CVMX_LMCX_PHY_CTL(ddr_interface_num));
        lmc_phy_ctl.cn70xx.dac_on = 1;
        ddr_print("PHY_CTL                                       : 0x%016llx\n", lmc_phy_ctl.u64);
        cvmx_write_csr(CVMX_LMCX_PHY_CTL(ddr_interface_num), lmc_phy_ctl.u64);
    }

    /*
     * 2. Write LMC(0)_SEQ_CTL[SEQ_SEL] = 0x0B and
     *    LMC(0)_SEQ_CTL[INIT_START] = 1.
     * 
     * 3. Wait for LMC(0)_SEQ_CTL[SEQ_COMPLETE] to be set to 1.
     */

    perform_octeon3_ddr3_sequence(cpu_id, rank_mask, ddr_interface_num, 0x0B); /* Offset training sequence */

    /*
     * 4.8.7 LMC Internal Vref Training
     * 
     * LMC requires input-reference-voltage training.
     * 
     * 1. Write LMC(0)_EXT_CONFIG[VREFINT_SEQ_DESKEW] = 0.
     */

    {
        cvmx_lmcx_ext_config_t ext_config;
        ext_config.u64 = cvmx_read_csr(CVMX_LMCX_EXT_CONFIG(ddr_interface_num));
        ext_config.s.vrefint_seq_deskew = 0;
        cvmx_write_csr(CVMX_LMCX_EXT_CONFIG(ddr_interface_num), ext_config.u64);
    }

    /*
     * 2. Write LMC(0)_SEQ_CTL[SEQ_SEL] = 0x0a and
     *    LMC(0)_SEQ_CTL[INIT_START] = 1.
     * 
     * 3. Wait for LMC(0)_SEQ_CTL[SEQ_COMPLETE] to be set to 1.
     */

    perform_octeon3_ddr3_sequence(cpu_id, rank_mask, ddr_interface_num, 0x0A); /* Vref internal training sequence */

    /*
     * 4.8.8 LMC Deskew Training
     * 
     * LMC requires input-read-data deskew training.
     * 
     * 1. Write LMC(0)_EXT_CONFIG[VREFINT_SEQ_DESKEW] = 1.
     */

    {
        cvmx_lmcx_ext_config_t ext_config;
        ext_config.u64 = cvmx_read_csr(CVMX_LMCX_EXT_CONFIG(ddr_interface_num));
        ext_config.s.vrefint_seq_deskew = 1;
        cvmx_write_csr(CVMX_LMCX_EXT_CONFIG(ddr_interface_num), ext_config.u64);
    }

    /*
     * 2. Write LMC(0)_SEQ_CTL[SEQ_SEL] = 0x0A and
     *    LMC(0)_SEQ_CTL[INIT_START] = 1.
     * 
     * 3. Wait for LMC(0)_SEQ_CTL[SEQ_COMPLETE] to be set to 1.
     */

    perform_octeon3_ddr3_sequence(cpu_id, rank_mask, ddr_interface_num, 0x0A); /* Vref internal training sequence */

    /*
     * 4.8.9 LMC Write Leveling
     * 
     * LMC supports an automatic write leveling like that described in the
     * JEDEC DDR3 specifications separately per byte-lane.
     * 
     * All of DDR PLL, LMC CK, LMC DRESET, and early LMC initializations must
     * be completed prior to starting this LMC write-leveling sequence.
     * 
     * There are many possible procedures that will write-level all the
     * attached DDR3 DRAM parts. One possibility is for software to simply
     * write the desired values into LMC(0)_WLEVEL_RANK(0..3). This section
     * describes one possible sequence that uses LMC's autowrite-leveling
     * capabilities.
     * 
     * 1. If the DQS/DQ delays on the board may be more than the ADD/CMD
     *    delays, then ensure that LMC(0)_CONFIG[EARLY_DQX] is set at this
     *    point.
     * 
     * Do the remaining steps 2-7 separately for each rank i with attached
     * DRAM.
     * 
     * 2. Write LMC(0)_WLEVEL_RANKi = 0.
     * 
     * 3. For ×8 parts:
     * 
     *    Without changing any other fields in LMC(0)_WLEVEL_CTL, write
     *    LMC(0)_WLEVEL_CTL[LANEMASK] to select all byte lanes with attached
     *    DRAM.
     * 
     *    For ×16 parts:
     * 
     *    Without changing any other fields in LMC(0)_WLEVEL_CTL, write
     *    LMC(0)_WLEVEL_CTL[LANEMASK] to select all even byte lanes with
     *    attached DRAM.
     * 
     * 4. Without changing any other fields in LMC(0)_CONFIG,
     * 
     *    o write LMC(0)_SEQ_CTL[SEQ_SEL] to select write-leveling
     * 
     *    o write LMC(0)_CONFIG[RANKMASK] = (1 << i)
     * 
     *    o write LMC(0)_SEQ_CTL[INIT_START] = 1
     * 
     *    LMC will initiate write-leveling at this point. Assuming
     *    LMC(0)_WLEVEL_CTL [SSET] = 0, LMC first enables write-leveling on
     *    the selected DRAM rank via a DDR3 MR1 write, then sequences through
     *    and accumulates write-leveling results for eight different delay
     *    settings twice, starting at a delay of zero in this case since
     *    LMC(0)_WLEVEL_RANKi[BYTE*<4:3>] = 0, increasing by 1/8 CK each
     *    setting, covering a total distance of one CK, then disables the
     *    write-leveling via another DDR3 MR1 write.
     * 
     *    After the sequence through 16 delay settings is complete:
     * 
     *    o LMC sets LMC(0)_WLEVEL_RANKi[STATUS] = 3
     * 
     *    o LMC sets LMC(0)_WLEVEL_RANKi[BYTE*<2:0>] (for all ranks selected
     *      by LMC(0)_WLEVEL_CTL[LANEMASK]) to indicate the first write
     *      leveling result of 1 that followed result of 0 during the
     *      sequence, except that the LMC always writes
     *      LMC(0)_WLEVEL_RANKi[BYTE*<0>]=0.
     * 
     *    o Software can read the eight write-leveling results from the first
     *      pass through the delay settings by reading
     *      LMC(0)_WLEVEL_DBG[BITMASK] (after writing
     *      LMC(0)_WLEVEL_DBG[BYTE]). (LMC does not retain the writeleveling
     *      results from the second pass through the eight delay
     *      settings. They should often be identical to the
     *      LMC(0)_WLEVEL_DBG[BITMASK] results, though.)
     * 
     * 5. Wait until LMC(0)_WLEVEL_RANKi[STATUS] != 2.
     * 
     *    LMC will have updated LMC(0)_WLEVEL_RANKi[BYTE*<2:0>] for all byte
     *    lanes selected by LMC(0)_WLEVEL_CTL[LANEMASK] at this point.
     *    LMC(0)_WLEVEL_RANKi[BYTE*<4:3>] will still be the value that
     *    software wrote in substep 2 above, which is 0.
     * 
     * 6. For ×16 parts:
     * 
     *    Without changing any other fields in LMC(0)_WLEVEL_CTL, write
     *    LMC(0)_WLEVEL_CTL[LANEMASK] to select all odd byte lanes with
     *    attached DRAM.
     * 
     *    Repeat substeps 4 and 5 with this new LMC(0)_WLEVEL_CTL[LANEMASK]
     *    setting. Skip to substep 7 if this has already been done.
     * 
     *    For ×8 parts:
     * 
     *    Skip this substep. Go to substep 7.
     * 
     * 7. Calculate LMC(0)_WLEVEL_RANKi[BYTE*<4:3>] settings for all byte
     *    lanes on all ranks with attached DRAM.
     * 
     *    At this point, all byte lanes on rank i with attached DRAM should
     *    have been write-leveled, and LMC(0)_WLEVEL_RANKi[BYTE*<2:0>] has
     *    the result for each byte lane.
     * 
     *    But note that the DDR3 write-leveling sequence will only determine
     *    the delay modulo the CK cycle time, and cannot determine how many
     *    additional CK cycles of delay are present. Software must calculate
     *    the number of CK cycles, or equivalently, the
     *    LMC(0)_WLEVEL_RANKi[BYTE*<4:3>] settings.
     * 
     *    This BYTE*<4:3> calculation is system/board specific.
     * 
     * Many techniques can be used to calculate write-leveling BYTE*<4:3> values,
     * including:
     * 
     *    o Known values for some byte lanes.
     * 
     *    o Relative values for some byte lanes relative to others.
     * 
     *    For example, suppose lane X is likely to require a larger
     *    write-leveling delay than lane Y. A BYTEX<2:0> value that is much
     *    smaller than the BYTEY<2:0> value may then indicate that the
     *    required lane X delay wrapped into the next CK, so BYTEX<4:3>
     *    should be set to BYTEY<4:3>+1.
     * 
     *    When ECC DRAM is not present (i.e. when DRAM is not attached to the
     *    DDR_CBS_0_* and DDR_CB<7:0> chip signals, or the DDR_DQS_<4>_* and
     *    DDR_DQ<35:32> chip signals), write LMC(0)_WLEVEL_RANK*[BYTE8] =
     *    LMC(0)_WLEVEL_RANK*[BYTE0], using the final calculated BYTE0 value.
     *    Write LMC(0)_WLEVEL_RANK*[BYTE4] = LMC(0)_WLEVEL_RANK*[BYTE0],
     *    using the final calculated BYTE0 value.
     * 
     * 8. Initialize LMC(0)_WLEVEL_RANK* values for all unused ranks.
     * 
     *    Let rank i be a rank with attached DRAM.
     * 
     *    For all ranks j that do not have attached DRAM, set
     *    LMC(0)_WLEVEL_RANKj = LMC(0)_WLEVEL_RANKi.
     */
    {
        cvmx_lmcx_wlevel_ctl_t wlevel_ctl;
        cvmx_lmcx_wlevel_rankx_t lmc_wlevel_rank;
        cvmx_lmcx_config_t lmc_config;
        int rankx = 0;
        int wlevel_bitmask[9];
        int byte_idx;
        int passx;
        int ecc_ena;
        int ddr_wlevel_roundup = 0;
        int save_mode32b;

        if (wlevel_loops)
            ddr_print("Performing Write-Leveling\n");
        else
            wlevel_bitmask_errors = 1; /* Force software write-leveling to run */

        lmc_config.u64 = cvmx_read_csr(CVMX_LMCX_CONFIG(ddr_interface_num));
        save_mode32b = lmc_config.cn70xx.mode32b;
        lmc_config.cn70xx.mode32b         = (! ddr_interface_64b);
        cvmx_write_csr(CVMX_LMCX_CONFIG(ddr_interface_num), lmc_config.u64);
        ddr_print("%-45s : %d\n", "MODE32B", lmc_config.cn70xx.mode32b);

        while(wlevel_loops--) {
            if ((s = lookup_env_parameter("ddr_wlevel_roundup")) != NULL) {
                ddr_wlevel_roundup = simple_strtoul(s, NULL, 0);
            }
            for (rankx = 0; rankx < dimm_count * 4;rankx++) {
                if (!(rank_mask & (1 << rankx)))
                    continue;

                wlevel_ctl.u64 = cvmx_read_csr(CVMX_LMCX_WLEVEL_CTL(ddr_interface_num));
                lmc_config.u64 = cvmx_read_csr(CVMX_LMCX_CONFIG(ddr_interface_num));
                ecc_ena = lmc_config.cn70xx.ecc_ena;

                if (!octeon_is_cpuid(OCTEON_CN63XX_PASS1_X)) {
                    if ((s = lookup_env_parameter("ddr_wlevel_rtt_nom")) != NULL) {
                        wlevel_ctl.cn70xx.rtt_nom   = simple_strtoul(s, NULL, 0);
                    }
                }
                cvmx_write_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num), 0); /* Clear write-level delays */

                wlevel_bitmask_errors = 0; /* Reset error counter */

                for (byte_idx=0; byte_idx<9; ++byte_idx) {
                    wlevel_bitmask[byte_idx] = 0; /* Reset bitmasks */
                }

                /* Make separate passes for each byte to reduce power. */
                for (passx=0; passx<(8+ecc_ena); ++passx) {

                    if (!(ddr_interface_bytemask&(1<<passx)))
                        continue;

                    wlevel_ctl.cn70xx.lanemask = (1<<passx);

                    cvmx_write_csr(CVMX_LMCX_WLEVEL_CTL(ddr_interface_num), wlevel_ctl.u64);

                    /* Read and write values back in order to update the
                       status field. This insurs that we read the updated
                       values after write-leveling has completed. */
                    cvmx_write_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num),
                                   cvmx_read_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num)));
                    perform_octeon3_ddr3_sequence(cpu_id, 1 << rankx, ddr_interface_num, 6); /* write-leveling */
                    do {
                        lmc_wlevel_rank.u64 = cvmx_read_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num));
                    } while (lmc_wlevel_rank.cn70xx.status != 3);

                    wlevel_bitmask[passx] = octeon_read_lmcx_ddr3_wlevel_dbg(ddr_interface_num, passx);
                    if (wlevel_bitmask[passx] == 0)
                        ++wlevel_bitmask_errors;
                } /* for (passx=0; passx<(8+ecc_ena); ++passx) */

                ddr_print("Rank(%d) Wlevel Debug Results                  : %05x %05x %05x %05x %05x %05x %05x %05x %05x\n",
                          rankx,
                          wlevel_bitmask[8],
                          wlevel_bitmask[7],
                          wlevel_bitmask[6],
                          wlevel_bitmask[5],
                          wlevel_bitmask[4],
                          wlevel_bitmask[3],
                          wlevel_bitmask[2],
                          wlevel_bitmask[1],
                          wlevel_bitmask[0]
                    );

                ddr_print("Rank(%d) Wlevel Rank %#5x, 0x%016llX : %5d %5d %5d %5d %5d %5d %5d %5d %5d\n",
                          rankx,
                          lmc_wlevel_rank.cn70xx.status,
                          lmc_wlevel_rank.u64,
                          lmc_wlevel_rank.cn70xx.byte8,
                          lmc_wlevel_rank.cn70xx.byte7,
                          lmc_wlevel_rank.cn70xx.byte6,
                          lmc_wlevel_rank.cn70xx.byte5,
                          lmc_wlevel_rank.cn70xx.byte4,
                          lmc_wlevel_rank.cn70xx.byte3,
                          lmc_wlevel_rank.cn70xx.byte2,
                          lmc_wlevel_rank.cn70xx.byte1,
                          lmc_wlevel_rank.cn70xx.byte0
                    );

                if (ddr_wlevel_roundup) { /* Round up odd bitmask delays */
                    for (byte_idx=0; byte_idx<(8+ecc_ena); ++byte_idx) {
                        if (!(ddr_interface_bytemask&(1<<byte_idx)))
                            continue;
                        update_wlevel_rank_struct(&lmc_wlevel_rank,
                                                  byte_idx,
                                                  roundup_ddr3_wlevel_bitmask(wlevel_bitmask[byte_idx]));
                    }
                    cvmx_write_csr(CVMX_LMCX_WLEVEL_RANKX(rankx, ddr_interface_num), lmc_wlevel_rank.u64);
                    ddr_print("Rank(%d) Wlevel Rank %#5x, 0x%016llX : %5d %5d %5d %5d %5d %5d %5d %5d %5d\n",
                              rankx,
                              lmc_wlevel_rank.cn70xx.status,
                              lmc_wlevel_rank.u64,
                              lmc_wlevel_rank.cn70xx.byte8,
                              lmc_wlevel_rank.cn70xx.byte7,
                              lmc_wlevel_rank.cn70xx.byte6,
                              lmc_wlevel_rank.cn70xx.byte5,
                              lmc_wlevel_rank.cn70xx.byte4,
                              lmc_wlevel_rank.cn70xx.byte3,
                              lmc_wlevel_rank.cn70xx.byte2,
                              lmc_wlevel_rank.cn70xx.byte1,
                              lmc_wlevel_rank.cn70xx.byte0
                        );
                }

                if (wlevel_bitmask_errors != 0) {
                    ddr_print("Rank(%d) Write-Leveling Failed: %d Bitmask errors\n", rankx, wlevel_bitmask_errors);
                }
            } /* for (rankx = 0; rankx < dimm_count * 4;rankx++) */
        } /* while(wlevel_loops--) */

        lmc_config.u64 = cvmx_read_csr(CVMX_LMCX_CONFIG(ddr_interface_num));
        lmc_config.cn70xx.mode32b         = save_mode32b;
        cvmx_write_csr(CVMX_LMCX_CONFIG(ddr_interface_num), lmc_config.u64);
        ddr_print("%-45s : %d\n", "MODE32B", lmc_config.cn70xx.mode32b);
    }

    /*
     * 4.8.10 LMC Read Leveling
     * 
     * LMC supports an automatic read-leveling separately per byte-lane using
     * the DDR3 multipurpose register predefined pattern for system
     * calibration defined in the JEDEC DDR3 specifications.
     * 
     * All of DDR PLL, LMC CK, and LMC DRESET, and early LMC initializations
     * must be completed prior to starting this LMC read-leveling sequence.
     * 
     * Software could simply write the desired read-leveling values into
     * LMC(0)_RLEVEL_RANK(0..3). This section describes a sequence that uses
     * LMC's autoread-leveling capabilities.
     * 
     * When LMC does the read-leveling sequence for a rank, it first enables
     * the DDR3 multipurpose register predefined pattern for system
     * calibration on the selected DRAM rank via a DDR3 MR3 write, then
     * executes 64 RD operations at different internal delay settings, then
     * disables the predefined pattern via another DDR3 MR3 write
     * operation. LMC determines the pass or fail of each of the 64 settings
     * independently for each byte lane, then writes appropriate
     * LMC(0)_RLEVEL_RANK(0..3)[BYTE*] values for the rank.
     * 
     * After read-leveling for a rank, software can read the 64 pass/fail
     * indications for one byte lane via LMC(0)_RLEVEL_DBG[BITMASK]. Software
     * can observe all pass/fail results for all byte lanes in a rank via
     * separate read-leveling sequences on the rank with different
     * LMC(0)_RLEVEL_CTL[BYTE] values.
     * 
     * The 64 pass/fail results will typically have failures for the low
     * delays, followed by a run of some passing settings, followed by more
     * failures in the remaining high delays.  LMC sets
     * LMC(0)_RLEVEL_RANK(0..3)[BYTE*] to one of the passing settings.
     * First, LMC selects the longest run of successes in the 64 results. (In
     * the unlikely event that there is more than one longest run, LMC
     * selects the first one.) Then if LMC(0)_RLEVEL_CTL[OFFSET_EN] = 1 and
     * the selected run has more than LMC(0)_RLEVEL_CTL[OFFSET] successes,
     * LMC selects the last passing setting in the run minus
     * LMC(0)_RLEVEL_CTL[OFFSET]. Otherwise LMC selects the middle setting in
     * the run (rounding earlier when necessary). We expect the read-leveling
     * sequence to produce good results with the reset values
     * LMC(0)_RLEVEL_CTL [OFFSET_EN]=1, LMC(0)_RLEVEL_CTL[OFFSET] = 2.
     * 
     * The read-leveling sequence has the following steps:
     * 
     * 1. Select desired LMC(0)_RLEVEL_CTL[OFFSET_EN,OFFSET,BYTE] settings.
     *    Do the remaining substeps 2-4 separately for each rank i with
     *    attached DRAM.
     * 
     * 2. Without changing any other fields in LMC(0)_CONFIG,
     * 
     *    o write LMC(0)_SEQ_CTL[SEQ_SEL] to select read-leveling
     * 
     *    o write LMC(0)_CONFIG[RANKMASK] = (1 << i)
     * 
     *    o write LMC(0)_SEQ_CTL[INIT_START] = 1
     * 
     *    This initiates the previously-described read-leveling.
     * 
     * 3. Wait until LMC(0)_RLEVEL_RANKi[STATUS] != 2
     * 
     *    LMC will have updated LMC(0)_RLEVEL_RANKi[BYTE*] for all byte lanes
     *    at this point.
     * 
     *    If ECC DRAM is not present (i.e. when DRAM is not attached to the
     *    DDR_CBS_0_* and DDR_CB<7:0> chip signals, or the DDR_DQS_<4>_* and
     *    DDR_DQ<35:32> chip signals), write LMC(0)_RLEVEL_RANK*[BYTE8] =
     *    LMC(0)_RLEVEL_RANK*[BYTE0]. Write LMC(0)_RLEVEL_RANK*[BYTE4] =
     *    LMC(0)_RLEVEL_RANK*[BYTE0].
     * 
     * 4. If desired, consult LMC(0)_RLEVEL_DBG[BITMASK] and compare to
     *    LMC(0)_RLEVEL_RANKi[BYTE*] for the lane selected by
     *    LMC(0)_RLEVEL_CTL[BYTE]. If desired, modify LMC(0)_RLEVEL_CTL[BYTE]
     *    to a new value and repeat so that all BITMASKs can be observed.
     * 
     * 5. Initialize LMC(0)_RLEVEL_RANK* values for all unused ranks.
     * 
     *    Let rank i be a rank with attached DRAM.
     * 
     *    For all ranks j that do not have attached DRAM, set
     *    LMC(0)_RLEVEL_RANKj = LMC(0)_RLEVEL_RANKi.
     * 
     * This read-leveling sequence can help select the proper CN70XX ODT
     * resistance value (LMC(0)_COMP_CTL2[RODT_CTL]). A hardware-generated
     * LMC(0)_RLEVEL_RANKi[BYTEj] value (for a used byte lane j) that is
     * drastically different from a neighboring LMC(0)_RLEVEL_RANKi[BYTEk]
     * (for a used byte lane k) can indicate that the CN70XX ODT value is
     * bad. It is possible to simultaneously optimize both
     * LMC(0)_COMP_CTL2[RODT_CTL] and LMC(0)_RLEVEL_RANKn[BYTE*] values by
     * performing this read-leveling sequence for several
     * LMC(0)_COMP_CTL2[RODT_CTL] values and selecting the one with the best
     * LMC(0)_RLEVEL_RANKn[BYTE*] profile for the ranks.
     */

    {
#pragma pack(push,4)
        cvmx_lmcx_rlevel_rankx_t lmc_rlevel_rank;
        cvmx_lmcx_config_t lmc_config;
        cvmx_lmcx_comp_ctl2_t lmc_comp_ctl2;
        cvmx_lmcx_rlevel_ctl_t rlevel_ctl;
        cvmx_lmcx_control_t lmc_control;
        cvmx_lmcx_modereg_params1_t lmc_modereg_params1;
        unsigned char rodt_ctl;
        unsigned char rankx = 0;
        int rlevel_rodt_errors = 0;
        unsigned char ecc_ena;
        unsigned char rtt_nom;
        unsigned char rtt_idx;
        int min_rtt_nom_idx;
        int max_rtt_nom_idx;
        int min_rodt_ctl;
        int max_rodt_ctl;
        static const unsigned char rodt_ohms[] = { 0, 20, 30, 40, 60, 120 };
        static const unsigned char rtt_nom_ohms[] = { 0, 60, 120, 40, 20, 30 };
        static const unsigned char rtt_nom_table[] = { 0, 2, 1, 3, 5, 4 };
        static const unsigned char rtt_wr_ohms[]  = { 0, 60, 120 };
        static const unsigned char dic_ohms[]     = { 40, 34 };
        int rlevel_debug_loops = 1;
        int default_rodt_ctl;
        unsigned char save_ddr2t;
        int rlevel_avg_loops;
        int ddr_rlevel_compute;
        int saved_ddr__ptune, saved_ddr__ntune, rlevel_comp_offset;
        int saved_int_zqcs_dis = 0;
        int disable_sequential_delay_check = 0;
        int maximum_adjacent_rlevel_delay_increment = 0;
        struct {
            uint64_t setting;
            int      score;
        } rlevel_scoreboard[sizeof(rtt_nom_ohms)][sizeof(rodt_ohms)][4];
#pragma pack(pop)

        default_rodt_ctl = odt_config[odt_idx].qs_dic;

        lmc_control.u64 = cvmx_read_csr(CVMX_LMCX_CONTROL(ddr_interface_num));
        save_ddr2t                    = lmc_control.cn70xx.ddr2t;

        lmc_config.u64 = cvmx_read_csr(CVMX_LMCX_CONFIG(ddr_interface_num));
        ecc_ena = lmc_config.cn70xx.ecc_ena;

        {
            int save_ref_zqcs_int;
            uint64_t temp_delay_usecs;

            /* Temporarily select the minimum ZQCS interval and wait
               long enough for a few ZQCS calibrations to occur.  This
               should ensure that the calibration circuitry is
               stabilized before read-leveling occurs. */
            if (OCTEON_IS_MODEL(OCTEON_CN71XX)) {
                save_ref_zqcs_int         = lmc_config.cn70xx.ref_zqcs_int;
                lmc_config.cn70xx.ref_zqcs_int = 1 | (32<<7); /* set smallest interval */
            } else {
                save_ref_zqcs_int         = lmc_config.cn63xx.ref_zqcs_int;
                lmc_config.cn63xx.ref_zqcs_int = 1 | (32<<7); /* set smallest interval */
            }
            cvmx_write_csr(CVMX_LMCX_CONFIG(ddr_interface_num), lmc_config.u64);
            cvmx_read_csr(CVMX_LMCX_CONFIG(ddr_interface_num));

            /* Compute an appropriate delay based on the current ZQCS
               interval. The delay should be long enough for the
               current ZQCS delay counter to expire plus ten of the
               minimum intarvals to ensure that some calibrations
               occur. */
            temp_delay_usecs = (((uint64_t)save_ref_zqcs_int >> 7)
                                * tclk_psecs * 100 * 512 * 128) / (10000*10000)
                + 10 * ((uint64_t)32 * tclk_psecs * 100 * 512 * 128) / (10000*10000);

            ddr_print ("Waiting %d usecs for ZQCS calibrations to start\n",
                         temp_delay_usecs);
            cvmx_wait_usec(temp_delay_usecs);

            if (OCTEON_IS_MODEL(OCTEON_CN71XX))
                lmc_config.cn70xx.ref_zqcs_int = save_ref_zqcs_int; /* Restore computed interval */
            else
                lmc_config.cn63xx.ref_zqcs_int = save_ref_zqcs_int; /* Restore computed interval */
            cvmx_write_csr(CVMX_LMCX_CONFIG(ddr_interface_num), lmc_config.u64);
            cvmx_read_csr(CVMX_LMCX_CONFIG(ddr_interface_num));
        }

        if ((save_ddr2t == 1) && octeon_is_cpuid(OCTEON_CN63XX_PASS1_X)) {
        /* During the read-leveling sequence with registered DIMMs
           when 2T mode is enabled (i.e. LMC0_CONTROL[DDR2T]=1), some
           register parts do not like the sequence that LMC generates
           for an MRS register write. (14656) */
            ddr_print("Disabling DDR 2T during read-leveling seq. Re: Pass 1 LMC-14656\n");
            lmc_control.cn70xx.ddr2t           = 0;
        }

        if ((s = lookup_env_parameter("ddr_rlevel_2t")) != NULL) {
            lmc_control.cn70xx.ddr2t = simple_strtoul(s, NULL, 0);
        }

        cvmx_write_csr(CVMX_LMCX_CONTROL(ddr_interface_num), lmc_control.u64);

        ddr_print("Performing Read-Leveling\n");

        rlevel_ctl.u64 = cvmx_read_csr(CVMX_LMCX_RLEVEL_CTL(ddr_interface_num));

        rlevel_avg_loops = custom_lmc_config->rlevel_average_loops;
        if (rlevel_avg_loops == 0) rlevel_avg_loops = 1;

        ddr_rlevel_compute = custom_lmc_config->rlevel_compute;
        rlevel_ctl.cn70xx.offset_en = custom_lmc_config->offset_en;
        rlevel_ctl.cn70xx.offset    = spd_rdimm
            ? custom_lmc_config->offset_rdimm
            : custom_lmc_config->offset_udimm;

        if (! octeon_is_cpuid(OCTEON_CN63XX_PASS1_X)) {
            rlevel_ctl.cn70xx.delay_unload_0 = 1; /* should normally be set */
            rlevel_ctl.cn70xx.delay_unload_1 = 1; /* should normally be set */
            rlevel_ctl.cn70xx.delay_unload_2 = 1; /* should normally be set */
            rlevel_ctl.cn70xx.delay_unload_3 = 1; /* should normally be set */
        }

        int byte_bitmask = 0xff;

        /* If we will be switching to 32bit mode level based on only
           four bits because there are only 4 ECC bits. */
        if (! ddr_interface_64b)
            byte_bitmask = 0x0f;

        rlevel_ctl.cn70xx.or_dis   = (! ddr_interface_64b);
        rlevel_ctl.cn70xx.bitmask  = byte_bitmask;

        ddr_rodt_ctl_auto = custom_lmc_config->ddr_rodt_ctl_auto;
        rlevel_comp_offset = (custom_lmc_config->rlevel_comp_offset == 0) ? 2 : custom_lmc_config->rlevel_comp_offset;

        if ((s = lookup_env_parameter("ddr_rlevel_offset")) != NULL) {
            rlevel_ctl.cn70xx.offset   = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_rlevel_offset_en")) != NULL) {
            rlevel_ctl.cn70xx.offset_en   = simple_strtoul(s, NULL, 0);
        }
        if ((s = lookup_env_parameter("ddr_rlevel_ctl")) != NULL) {
            rlevel_ctl.u64   = simple_strtoul(s, NULL, 0);
        }

        cvmx_write_csr(CVMX_LMCX_RLEVEL_CTL(ddr_interface_num), rlevel_ctl.u64);

        if ((s = lookup_env_parameter("ddr_rtt_nom_auto")) != NULL) {
            ddr_rtt_nom_auto = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_rodt_ctl_auto")) != NULL) {
            ddr_rodt_ctl_auto = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_rlevel_average")) != NULL) {
            rlevel_avg_loops = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_rlevel_compute")) != NULL) {
            ddr_rlevel_compute = simple_strtoul(s, NULL, 0);
        }

        ddr_print("RLEVEL_CTL                                    : 0x%016llx\n", rlevel_ctl.u64);
        ddr_print("RLEVEL_OFFSET                                 : %6d\n", rlevel_ctl.cn70xx.offset);
        ddr_print("RLEVEL_OFFSET_EN                              : %6d\n", rlevel_ctl.cn70xx.offset_en);

        /* The purpose for the indexed table is to sort the settings
        ** by the ohm value to simplify the testing when incrementing
        ** through the settings.  (index => ohms) 1=120, 2=60, 3=40,
        ** 4=30, 5=20 */
        min_rtt_nom_idx = (custom_lmc_config->min_rtt_nom_idx == 0) ? 1 : custom_lmc_config->min_rtt_nom_idx;
        max_rtt_nom_idx = (custom_lmc_config->max_rtt_nom_idx == 0) ? 5 : custom_lmc_config->max_rtt_nom_idx;

        min_rodt_ctl = (custom_lmc_config->min_rodt_ctl == 0) ? 1 : custom_lmc_config->min_rodt_ctl;
        max_rodt_ctl = (custom_lmc_config->max_rodt_ctl == 0) ? 5 : custom_lmc_config->max_rodt_ctl;

        if ((s = lookup_env_parameter("ddr_min_rodt_ctl")) != NULL) {
            min_rodt_ctl = simple_strtoul(s, NULL, 0);
        }
        if ((s = lookup_env_parameter("ddr_max_rodt_ctl")) != NULL) {
            max_rodt_ctl = simple_strtoul(s, NULL, 0);
        }
        if ((s = lookup_env_parameter("ddr_min_rtt_nom_idx")) != NULL) {
            min_rtt_nom_idx = simple_strtoul(s, NULL, 0);
        }
        if ((s = lookup_env_parameter("ddr_max_rtt_nom_idx")) != NULL) {
            max_rtt_nom_idx = simple_strtoul(s, NULL, 0);
        }

        while(rlevel_debug_loops--) {
        /* Initialize the error scoreboard */
        for ( rtt_idx=1; rtt_idx<6; ++rtt_idx) {
            rtt_nom = rtt_nom_table[rtt_idx];
        for (rodt_ctl=5; rodt_ctl>0; --rodt_ctl) {
            for (rankx = 0; rankx < dimm_count*4; rankx++) {
                if (!(rank_mask & (1 << rankx)))
                    continue;
                rlevel_scoreboard[rtt_nom][rodt_ctl][rankx].score   = 0;
                rlevel_scoreboard[rtt_nom][rodt_ctl][rankx].setting = 0;
            }
        }
        }

        if ((s = lookup_env_parameter("ddr_rlevel_comp_offset")) != NULL) {
            rlevel_comp_offset = simple_strtoul(s, NULL, 0);
        }

        disable_sequential_delay_check = custom_lmc_config->disable_sequential_delay_check;

        if ((s = lookup_env_parameter("ddr_disable_sequential_delay_check")) != NULL) {
            disable_sequential_delay_check = simple_strtoul(s, NULL, 0);
        }

        maximum_adjacent_rlevel_delay_increment = custom_lmc_config->maximum_adjacent_rlevel_delay_increment;

        if ((s = lookup_env_parameter("ddr_maximum_adjacent_rlevel_delay_increment")) != NULL) {
            maximum_adjacent_rlevel_delay_increment = simple_strtoul(s, NULL, 0);
        }

        lmc_comp_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num));
        saved_ddr__ptune = lmc_comp_ctl2.cn70xx.ddr__ptune;
        saved_ddr__ntune = lmc_comp_ctl2.cn70xx.ddr__ntune;

        /* Disable dynamic compensation settings */
        if (rlevel_comp_offset != 0) {
            lmc_comp_ctl2.cn70xx.ptune = saved_ddr__ptune;
            lmc_comp_ctl2.cn70xx.ntune = saved_ddr__ntune;

            lmc_comp_ctl2.cn70xx.ptune += rlevel_comp_offset;

            lmc_control.u64 = cvmx_read_csr(CVMX_LMCX_CONTROL(ddr_interface_num));
            saved_int_zqcs_dis = lmc_control.cn70xx.int_zqcs_dis;
            lmc_control.cn70xx.int_zqcs_dis    = 1; /* Disable ZQCS while in bypass. */
            cvmx_write_csr(CVMX_LMCX_CONTROL(ddr_interface_num), lmc_control.u64);

            lmc_comp_ctl2.cn70xx.byp = 1; /* Enable bypass mode */
            cvmx_write_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num), lmc_comp_ctl2.u64);
            cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num));
        }

        lmc_modereg_params1.u64 = cvmx_read_csr(CVMX_LMCX_MODEREG_PARAMS1(ddr_interface_num));

        for (rtt_idx=min_rtt_nom_idx; rtt_idx<=max_rtt_nom_idx; ++rtt_idx) {
            rtt_nom = rtt_nom_table[rtt_idx];

            /* When the read ODT mask is zero the dyn_rtt_nom_mask is
               zero than RTT_NOM will not be changing during
               read-leveling.  Since the value is fixed we only need
               to test it once. */
            if ((dyn_rtt_nom_mask == 0) && (rtt_idx != min_rtt_nom_idx))
                continue;

            if (dyn_rtt_nom_mask & 1) lmc_modereg_params1.cn70xx.rtt_nom_00 = rtt_nom;
            if (dyn_rtt_nom_mask & 2) lmc_modereg_params1.cn70xx.rtt_nom_01 = rtt_nom;
            if (dyn_rtt_nom_mask & 4) lmc_modereg_params1.cn70xx.rtt_nom_10 = rtt_nom;
            if (dyn_rtt_nom_mask & 8) lmc_modereg_params1.cn70xx.rtt_nom_11 = rtt_nom;

            cvmx_write_csr(CVMX_LMCX_MODEREG_PARAMS1(ddr_interface_num), lmc_modereg_params1.u64);
            ddr_print("\n");
            ddr_print("RTT_NOM     %3d, %3d, %3d, %3d ohms           :  %x,%x,%x,%x\n",
                      rtt_nom_ohms[lmc_modereg_params1.cn70xx.rtt_nom_11],
                      rtt_nom_ohms[lmc_modereg_params1.cn70xx.rtt_nom_10],
                      rtt_nom_ohms[lmc_modereg_params1.cn70xx.rtt_nom_01],
                      rtt_nom_ohms[lmc_modereg_params1.cn70xx.rtt_nom_00],
                      lmc_modereg_params1.cn70xx.rtt_nom_11,
                      lmc_modereg_params1.cn70xx.rtt_nom_10,
                      lmc_modereg_params1.cn70xx.rtt_nom_01,
                      lmc_modereg_params1.cn70xx.rtt_nom_00);

            perform_octeon3_ddr3_init_sequence(cpu_id, rank_mask, ddr_interface_num);

        for (rodt_ctl=max_rodt_ctl; rodt_ctl>=min_rodt_ctl; --rodt_ctl) {
            rlevel_rodt_errors = 0;
            lmc_comp_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num));
            lmc_comp_ctl2.cn70xx.rodt_ctl = rodt_ctl;
            cvmx_write_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num), lmc_comp_ctl2.u64);
            lmc_comp_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num));
            ddr_print("Read ODT_CTL                                  : 0x%x (%d ohms)\n",
                      lmc_comp_ctl2.cn70xx.rodt_ctl, rodt_ohms[lmc_comp_ctl2.cn70xx.rodt_ctl]);

            for (rankx = 0; rankx < dimm_count*4; rankx++) {
                int byte_idx;
                rlevel_byte_data_t rlevel_byte[9] = {{0, 0, 0}};

                int rlevel_rank_errors;
                if (!(rank_mask & (1 << rankx)))
                    continue;

                int average_loops = rlevel_avg_loops;
                while (average_loops--) {
                    rlevel_rank_errors = 0;

                    /* Clear read-level delays */
                    cvmx_write_csr(CVMX_LMCX_RLEVEL_RANKX(rankx, ddr_interface_num), 0);

                    perform_octeon3_ddr3_sequence(cpu_id, 1 << rankx, ddr_interface_num, 1); /* read-leveling */

                    do {
                        lmc_rlevel_rank.u64 = cvmx_read_csr(CVMX_LMCX_RLEVEL_RANKX(rankx, ddr_interface_num));
                    } while (lmc_rlevel_rank.cn70xx.status != 3);

                    {
                        struct {
                            uint64_t bm;
                            uint8_t mstart;
                            uint8_t width;
                        } rlevel_bitmask[9]={{0, 0, 0}};

                        /* Evaluate the quality of the read-leveling
                           delays. Also save off a software computed
                           read-leveling mask that may be used later
                           to qualify the delay results from Octeon. */
                        for (byte_idx=0; byte_idx<(8+ecc_ena); ++byte_idx) {
                            if (!(ddr_interface_bytemask&(1<<byte_idx)))
                                continue;
                            rlevel_bitmask[byte_idx].bm = octeon_read_lmcx_ddr3_rlevel_dbg(ddr_interface_num, byte_idx);
                            rlevel_rank_errors += validate_ddr3_rlevel_bitmask(rlevel_bitmask[byte_idx].bm, tclk_psecs);
                        }

                        /* Set delays for unused bytes to match byte 0. */
                        for (byte_idx=0; byte_idx<9; ++byte_idx) {
                            if (ddr_interface_bytemask&(1<<byte_idx))
                                continue;
                            update_rlevel_rank_struct(&lmc_rlevel_rank, byte_idx, lmc_rlevel_rank.cn70xx.byte0);
                        }

                        /* Save a copy of the byte delays in physical
                           order for sequential evaluation. */
                        if (ecc_ena) {
                            rlevel_byte[8].delay = lmc_rlevel_rank.cn70xx.byte7;
                            rlevel_byte[7].delay = lmc_rlevel_rank.cn70xx.byte6;
                            rlevel_byte[6].delay = lmc_rlevel_rank.cn70xx.byte5;
                            rlevel_byte[5].delay = lmc_rlevel_rank.cn70xx.byte4;
                            rlevel_byte[4].delay = lmc_rlevel_rank.cn70xx.byte8; /* ECC */
                        } else {
                            rlevel_byte[7].delay = lmc_rlevel_rank.cn70xx.byte7;
                            rlevel_byte[6].delay = lmc_rlevel_rank.cn70xx.byte6;
                            rlevel_byte[5].delay = lmc_rlevel_rank.cn70xx.byte5;
                            rlevel_byte[4].delay = lmc_rlevel_rank.cn70xx.byte4;
                        }
                        rlevel_byte[3].delay = lmc_rlevel_rank.cn70xx.byte3;
                        rlevel_byte[2].delay = lmc_rlevel_rank.cn70xx.byte2;
                        rlevel_byte[1].delay = lmc_rlevel_rank.cn70xx.byte1;
                        rlevel_byte[0].delay = lmc_rlevel_rank.cn70xx.byte0;

                        if (! disable_sequential_delay_check) {
                            if ((ddr_interface_bytemask&0xff) == 0xff) {
                                /* Evaluate delay sequence across the whole range of bytes for stantard dimms. */
                                if ((spd_dimm_type == 1) || (spd_dimm_type == 5)) { /* 1=RDIMM, 5=Mini-RDIMM */
                                    /* Registerd dimm topology routes from the center. */
                                    rlevel_rank_errors += octeon3_nonsequential_delays(rlevel_byte, 0, 3+ecc_ena, maximum_adjacent_rlevel_delay_increment);
                                    rlevel_rank_errors += octeon3_nonsequential_delays(rlevel_byte, 4, 7+ecc_ena, maximum_adjacent_rlevel_delay_increment);
                                }
                                if ((spd_dimm_type == 2) || (spd_dimm_type == 6)) { /* 1=UDIMM, 5=Mini-UDIMM */
                                    /* Unbuffered dimm topology routes from end to end. */
                                    rlevel_rank_errors += octeon3_nonsequential_delays(rlevel_byte, 0, 7+ecc_ena, maximum_adjacent_rlevel_delay_increment);
                                }
                            } else {
                                rlevel_rank_errors += octeon3_nonsequential_delays(rlevel_byte, 0, 3+ecc_ena, maximum_adjacent_rlevel_delay_increment);
                            }
                        }

                        ddr_print("Rank(%d) Rlevel Debug Test Results  8:0        : %05llx %05llx %05llx %05llx %05llx %05llx %05llx %05llx %05llx\n",
                                  rankx,
                                  rlevel_bitmask[8].bm,
                                  rlevel_bitmask[7].bm,
                                  rlevel_bitmask[6].bm,
                                  rlevel_bitmask[5].bm,
                                  rlevel_bitmask[4].bm,
                                  rlevel_bitmask[3].bm,
                                  rlevel_bitmask[2].bm,
                                  rlevel_bitmask[1].bm,
                                  rlevel_bitmask[0].bm
                                  );

                        ddr_print("Rank(%d) Rlevel Rank %#5x, 0x%016llX : %5d %5d %5d %5d %5d %5d %5d %5d %5d (%d)\n",
                                  rankx,
                                  lmc_rlevel_rank.cn70xx.status,
                                  lmc_rlevel_rank.u64,
                                  lmc_rlevel_rank.cn70xx.byte8,
                                  lmc_rlevel_rank.cn70xx.byte7,
                                  lmc_rlevel_rank.cn70xx.byte6,
                                  lmc_rlevel_rank.cn70xx.byte5,
                                  lmc_rlevel_rank.cn70xx.byte4,
                                  lmc_rlevel_rank.cn70xx.byte3,
                                  lmc_rlevel_rank.cn70xx.byte2,
                                  lmc_rlevel_rank.cn70xx.byte1,
                                  lmc_rlevel_rank.cn70xx.byte0,
                                  rlevel_rank_errors
                                  );

                        if (ddr_rlevel_compute) {
                            /* Recompute the delays based on the bitmask */
                            for (byte_idx=0; byte_idx<(8+ecc_ena); ++byte_idx) {
                                if (!(ddr_interface_bytemask&(1<<byte_idx)))
                                    continue;

                                update_rlevel_rank_struct(&lmc_rlevel_rank, byte_idx,
                                                          compute_ddr3_rlevel_delay(rlevel_bitmask[byte_idx].bm,
                                                                                    rlevel_ctl));
                            }

                            /* Override the copy of byte delays with the computed results. */
                            if (ecc_ena) {
                                rlevel_byte[8].delay = lmc_rlevel_rank.cn70xx.byte7;
                                rlevel_byte[7].delay = lmc_rlevel_rank.cn70xx.byte6;
                                rlevel_byte[6].delay = lmc_rlevel_rank.cn70xx.byte5;
                                rlevel_byte[5].delay = lmc_rlevel_rank.cn70xx.byte4;
                                rlevel_byte[4].delay = lmc_rlevel_rank.cn70xx.byte8; /* ECC */
                            } else {
                                rlevel_byte[7].delay = lmc_rlevel_rank.cn70xx.byte7;
                                rlevel_byte[6].delay = lmc_rlevel_rank.cn70xx.byte6;
                                rlevel_byte[5].delay = lmc_rlevel_rank.cn70xx.byte5;
                                rlevel_byte[4].delay = lmc_rlevel_rank.cn70xx.byte4;
                            }
                            rlevel_byte[3].delay = lmc_rlevel_rank.cn70xx.byte3;
                            rlevel_byte[2].delay = lmc_rlevel_rank.cn70xx.byte2;
                            rlevel_byte[1].delay = lmc_rlevel_rank.cn70xx.byte1;
                            rlevel_byte[0].delay = lmc_rlevel_rank.cn70xx.byte0;

                            ddr_print("Rank(%d) Computed delays                       : %5d %5d %5d %5d %5d %5d %5d %5d %5d\n",
                                      rankx,
                                      lmc_rlevel_rank.cn70xx.byte8,
                                      lmc_rlevel_rank.cn70xx.byte7,
                                      lmc_rlevel_rank.cn70xx.byte6,
                                      lmc_rlevel_rank.cn70xx.byte5,
                                      lmc_rlevel_rank.cn70xx.byte4,
                                      lmc_rlevel_rank.cn70xx.byte3,
                                      lmc_rlevel_rank.cn70xx.byte2,
                                      lmc_rlevel_rank.cn70xx.byte1,
                                      lmc_rlevel_rank.cn70xx.byte0
                                      );
                        }
                    }

                    /* Accumulate the total score across averaging loops for this setting */
                    rlevel_scoreboard[rtt_nom][rodt_ctl][rankx].score += rlevel_rank_errors;
                    debug_print("rlevel_scoreboard[rtt_nom=%d][rodt_ctl=%d][rankx=%d].score:%d\n",
                                rtt_nom, rodt_ctl, rankx,
                                rlevel_scoreboard[rtt_nom][rodt_ctl][rankx].score);

                    /* Accumulate the delay totals and loop counts
                       necessary to compute average delay results */
                    for (byte_idx=0; byte_idx<9; ++byte_idx) {
                        rlevel_byte[byte_idx].loop_total += rlevel_byte[byte_idx].delay;
                        if (rlevel_byte[byte_idx].delay != 0) /* Don't include delay=0 in the average */
                            ++rlevel_byte[byte_idx].loop_count;
                    }

                    rlevel_rodt_errors += rlevel_rank_errors;
                } /* while (rlevel_avg_loops--) */

                /* Compute the average score across averaging loops */
                rlevel_scoreboard[rtt_nom][rodt_ctl][rankx].score =
                    divide_nint(rlevel_scoreboard[rtt_nom][rodt_ctl][rankx].score, rlevel_avg_loops);

                /* Compute the average delay results */
                for (byte_idx=0; byte_idx<9; ++byte_idx) {
                    if (rlevel_byte[byte_idx].loop_count == 0)
                        rlevel_byte[byte_idx].loop_count = 1;
                    rlevel_byte[byte_idx].delay = divide_nint(rlevel_byte[byte_idx].loop_total,
                                                              rlevel_byte[byte_idx].loop_count);
                }

                lmc_rlevel_rank.u64 = cvmx_read_csr(CVMX_LMCX_RLEVEL_RANKX(rankx, ddr_interface_num));

                if (ecc_ena) {
                    lmc_rlevel_rank.cn70xx.byte7 = rlevel_byte[8].delay;
                    lmc_rlevel_rank.cn70xx.byte6 = rlevel_byte[7].delay;
                    lmc_rlevel_rank.cn70xx.byte5 = rlevel_byte[6].delay;
                    lmc_rlevel_rank.cn70xx.byte4 = rlevel_byte[5].delay;
                    lmc_rlevel_rank.cn70xx.byte8 = rlevel_byte[4].delay; /* ECC */
                } else {
                    lmc_rlevel_rank.cn70xx.byte7 = rlevel_byte[7].delay;
                    lmc_rlevel_rank.cn70xx.byte6 = rlevel_byte[6].delay;
                    lmc_rlevel_rank.cn70xx.byte5 = rlevel_byte[5].delay;
                    lmc_rlevel_rank.cn70xx.byte4 = rlevel_byte[4].delay;
                }
                lmc_rlevel_rank.cn70xx.byte3 = rlevel_byte[3].delay;
                lmc_rlevel_rank.cn70xx.byte2 = rlevel_byte[2].delay;
                lmc_rlevel_rank.cn70xx.byte1 = rlevel_byte[1].delay;
                lmc_rlevel_rank.cn70xx.byte0 = rlevel_byte[0].delay;

                if (rlevel_avg_loops > 1) {
                    ddr_print("Rank(%d) Rlevel Rank %#5x, 0x%016llX : %5d %5d %5d %5d %5d %5d %5d %5d %5d (%d) Average\n\n",
                              rankx,
                              lmc_rlevel_rank.cn70xx.status,
                              lmc_rlevel_rank.u64,
                              lmc_rlevel_rank.cn70xx.byte8,
                              lmc_rlevel_rank.cn70xx.byte7,
                              lmc_rlevel_rank.cn70xx.byte6,
                              lmc_rlevel_rank.cn70xx.byte5,
                              lmc_rlevel_rank.cn70xx.byte4,
                              lmc_rlevel_rank.cn70xx.byte3,
                              lmc_rlevel_rank.cn70xx.byte2,
                              lmc_rlevel_rank.cn70xx.byte1,
                              lmc_rlevel_rank.cn70xx.byte0,
                              rlevel_scoreboard[rtt_nom][rodt_ctl][rankx].score
                              );
                }

                rlevel_scoreboard[rtt_nom][rodt_ctl][rankx].setting = lmc_rlevel_rank.u64;
            } /* for (rankx = 0; rankx < dimm_count*4; rankx++) */
        } /* for (rodt_ctl=odt_config[odt_idx].qs_dic; rodt_ctl>0; --rodt_ctl) */
        } /*  for ( rtt_idx=1; rtt_idx<6; ++rtt_idx) */


        /* Re-enable dynamic compensation settings. */
        if (rlevel_comp_offset != 0) {
            lmc_comp_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num));

            lmc_comp_ctl2.cn70xx.ptune = 0;
            lmc_comp_ctl2.cn70xx.ntune = 0;
            lmc_comp_ctl2.cn70xx.byp = 0; /* Disable bypass mode */
            cvmx_write_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num), lmc_comp_ctl2.u64);
            cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num)); /* Read once */

            lmc_comp_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num)); /* Read again */
            ddr_print("DDR__PTUNE/DDR__NTUNE                         : %d/%d\n",
                      lmc_comp_ctl2.cn70xx.ddr__ptune, lmc_comp_ctl2.cn70xx.ddr__ntune);

            lmc_control.u64 = cvmx_read_csr(CVMX_LMCX_CONTROL(ddr_interface_num));
            lmc_control.cn70xx.int_zqcs_dis    = saved_int_zqcs_dis; /* Restore original setting */
            cvmx_write_csr(CVMX_LMCX_CONTROL(ddr_interface_num), lmc_control.u64);

        }


        int override_compensation = 0;
        if ((s = lookup_env_parameter("ddr__ptune")) != NULL) {
            saved_ddr__ptune = simple_strtoul(s, NULL, 0);
            override_compensation = 1;
        }
        if ((s = lookup_env_parameter("ddr__ntune")) != NULL) {
            saved_ddr__ntune = simple_strtoul(s, NULL, 0);
            override_compensation = 1;
        }
        if (override_compensation) {
            lmc_comp_ctl2.cn70xx.ptune = saved_ddr__ptune;
            lmc_comp_ctl2.cn70xx.ntune = saved_ddr__ntune;

            lmc_control.u64 = cvmx_read_csr(CVMX_LMCX_CONTROL(ddr_interface_num));
            saved_int_zqcs_dis = lmc_control.cn70xx.int_zqcs_dis;
            lmc_control.cn70xx.int_zqcs_dis    = 1; /* Disable ZQCS while in bypass. */
            cvmx_write_csr(CVMX_LMCX_CONTROL(ddr_interface_num), lmc_control.u64);

            lmc_comp_ctl2.cn70xx.byp = 1; /* Enable bypass mode */
            cvmx_write_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num), lmc_comp_ctl2.u64);
            cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num));

            ddr_print("DDR__PTUNE/DDR__NTUNE                         : %d/%d\n",
                      lmc_comp_ctl2.cn70xx.ptune, lmc_comp_ctl2.cn70xx.ntune);
        }

        {
            uint64_t best_setting[4] = {0};
            int      best_rodt_score = 9999999; /* Start with an arbitrarily high score */
            int      auto_rodt_ctl = 0;
            int      auto_rtt_nom  = 0;
            int rodt_score;

            ddr_print("Evaluating Read-Leveling Scoreboard.\n");
            for (rtt_idx=min_rtt_nom_idx; rtt_idx<=max_rtt_nom_idx; ++rtt_idx) {
                rtt_nom = rtt_nom_table[rtt_idx];

                /* When the read ODT mask is zero the dyn_rtt_nom_mask is
                   zero than RTT_NOM will not be changing during
                   read-leveling.  Since the value is fixed we only need
                   to test it once. */
                if ((dyn_rtt_nom_mask == 0) && (rtt_idx != min_rtt_nom_idx))
                    continue;

                for (rodt_ctl=max_rodt_ctl; rodt_ctl>=min_rodt_ctl; --rodt_ctl) {
                    rodt_score = 0;
                    for (rankx = 0; rankx < dimm_count * 4;rankx++) {
                        if (!(rank_mask & (1 << rankx)))
                            continue;
                        debug_print("rlevel_scoreboard[rtt_nom=%d][rodt_ctl=%d][rankx=%d].score:%d\n",
                                    rtt_nom, rodt_ctl, rankx, rlevel_scoreboard[rtt_nom][rodt_ctl][rankx].score);
                        rodt_score += rlevel_scoreboard[rtt_nom][rodt_ctl][rankx].score;
                    }
                    debug_print("rodt_score:%d, best_rodt_score:%d\n", rodt_score, best_rodt_score);
                    if (rodt_score < best_rodt_score) {
                        best_rodt_score = rodt_score;
                        auto_rodt_ctl   = rodt_ctl;
                        auto_rtt_nom    = rtt_nom;

                        for (rankx = 0; rankx < dimm_count * 4;rankx++) {
                            if (!(rank_mask & (1 << rankx)))
                                continue;
                            best_setting[rankx]  = rlevel_scoreboard[rtt_nom][rodt_ctl][rankx].setting;
                        }
                    }
                }
            }

            lmc_modereg_params1.u64 = cvmx_read_csr(CVMX_LMCX_MODEREG_PARAMS1(ddr_interface_num));

            if (ddr_rtt_nom_auto) {
                /* Store the automatically set RTT_NOM value */
                if (dyn_rtt_nom_mask & 1) lmc_modereg_params1.cn70xx.rtt_nom_00 = auto_rtt_nom;
                if (dyn_rtt_nom_mask & 2) lmc_modereg_params1.cn70xx.rtt_nom_01 = auto_rtt_nom;
                if (dyn_rtt_nom_mask & 4) lmc_modereg_params1.cn70xx.rtt_nom_10 = auto_rtt_nom;
                if (dyn_rtt_nom_mask & 8) lmc_modereg_params1.cn70xx.rtt_nom_11 = auto_rtt_nom;
            } else {
                /* restore the manual settings to the register */
                lmc_modereg_params1.cn70xx.rtt_nom_00 = default_rtt_nom[0];
                lmc_modereg_params1.cn70xx.rtt_nom_01 = default_rtt_nom[1];
                lmc_modereg_params1.cn70xx.rtt_nom_10 = default_rtt_nom[2];
                lmc_modereg_params1.cn70xx.rtt_nom_11 = default_rtt_nom[3];
            }

            cvmx_write_csr(CVMX_LMCX_MODEREG_PARAMS1(ddr_interface_num), lmc_modereg_params1.u64);
            ddr_print("RTT_NOM     %3d, %3d, %3d, %3d ohms           :  %x,%x,%x,%x\n",
                      rtt_nom_ohms[lmc_modereg_params1.cn70xx.rtt_nom_11],
                      rtt_nom_ohms[lmc_modereg_params1.cn70xx.rtt_nom_10],
                      rtt_nom_ohms[lmc_modereg_params1.cn70xx.rtt_nom_01],
                      rtt_nom_ohms[lmc_modereg_params1.cn70xx.rtt_nom_00],
                      lmc_modereg_params1.cn70xx.rtt_nom_11,
                      lmc_modereg_params1.cn70xx.rtt_nom_10,
                      lmc_modereg_params1.cn70xx.rtt_nom_01,
                      lmc_modereg_params1.cn70xx.rtt_nom_00);

            ddr_print("RTT_WR      %3d, %3d, %3d, %3d ohms           :  %x,%x,%x,%x\n",
                      rtt_wr_ohms[lmc_modereg_params1.cn70xx.rtt_wr_11],
                      rtt_wr_ohms[lmc_modereg_params1.cn70xx.rtt_wr_10],
                      rtt_wr_ohms[lmc_modereg_params1.cn70xx.rtt_wr_01],
                      rtt_wr_ohms[lmc_modereg_params1.cn70xx.rtt_wr_00],
                      lmc_modereg_params1.cn70xx.rtt_wr_11,
                      lmc_modereg_params1.cn70xx.rtt_wr_10,
                      lmc_modereg_params1.cn70xx.rtt_wr_01,
                      lmc_modereg_params1.cn70xx.rtt_wr_00);

            ddr_print("DIC         %3d, %3d, %3d, %3d ohms           :  %x,%x,%x,%x\n",
                      dic_ohms[lmc_modereg_params1.cn70xx.dic_11],
                      dic_ohms[lmc_modereg_params1.cn70xx.dic_10],
                      dic_ohms[lmc_modereg_params1.cn70xx.dic_01],
                      dic_ohms[lmc_modereg_params1.cn70xx.dic_00],
                      lmc_modereg_params1.cn70xx.dic_11,
                      lmc_modereg_params1.cn70xx.dic_10,
                      lmc_modereg_params1.cn70xx.dic_01,
                      lmc_modereg_params1.cn70xx.dic_00);

            lmc_comp_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num));
            if (ddr_rodt_ctl_auto)
                lmc_comp_ctl2.cn70xx.rodt_ctl = auto_rodt_ctl;
            else
                lmc_comp_ctl2.cn70xx.rodt_ctl = default_rodt_ctl;
            cvmx_write_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num), lmc_comp_ctl2.u64);
            lmc_comp_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num));
            ddr_print("Read ODT_CTL                                  : 0x%x (%d ohms)\n",
                      lmc_comp_ctl2.cn70xx.rodt_ctl, rodt_ohms[lmc_comp_ctl2.cn70xx.rodt_ctl]);

            for (rankx = 0; rankx < dimm_count * 4;rankx++) {
                if (!(rank_mask & (1 << rankx)))
                    continue;
                lmc_rlevel_rank.u64 = rlevel_scoreboard[auto_rtt_nom][auto_rodt_ctl][rankx].setting;
                if (!ecc_ena){
                    lmc_rlevel_rank.cn70xx.byte8 = lmc_rlevel_rank.cn70xx.byte0; /* ECC is not used */
                }
                cvmx_write_csr(CVMX_LMCX_RLEVEL_RANKX(rankx, ddr_interface_num), lmc_rlevel_rank.u64);
                lmc_rlevel_rank.u64 = cvmx_read_csr(CVMX_LMCX_RLEVEL_RANKX(rankx, ddr_interface_num));
                ddr_print("Rank(%d) Rlevel Rank %#5x, 0x%016llX : %5d %5d %5d %5d %5d %5d %5d %5d %5d (%d)\n",
                          rankx,
                          lmc_rlevel_rank.cn70xx.status,
                          lmc_rlevel_rank.u64,
                          lmc_rlevel_rank.cn70xx.byte8,
                          lmc_rlevel_rank.cn70xx.byte7,
                          lmc_rlevel_rank.cn70xx.byte6,
                          lmc_rlevel_rank.cn70xx.byte5,
                          lmc_rlevel_rank.cn70xx.byte4,
                          lmc_rlevel_rank.cn70xx.byte3,
                          lmc_rlevel_rank.cn70xx.byte2,
                          lmc_rlevel_rank.cn70xx.byte1,
                          lmc_rlevel_rank.cn70xx.byte0,
                          rlevel_scoreboard[auto_rtt_nom][auto_rodt_ctl][rankx].score
                          );
            }
        }
        } /* while(rlevel_debug_loops--) */

        lmc_control.cn70xx.ddr2t           = save_ddr2t;
        cvmx_write_csr(CVMX_LMCX_CONTROL(ddr_interface_num), lmc_control.u64);
        lmc_control.u64 = cvmx_read_csr(CVMX_LMCX_CONTROL(ddr_interface_num));
        ddr_print("DDR2T                                         : %6d\n", lmc_control.cn70xx.ddr2t); /* Display final 2T value */



        perform_octeon3_ddr3_init_sequence(cpu_id, rank_mask, ddr_interface_num);

        for (rankx = 0; rankx < dimm_count * 4;rankx++) {
            uint64_t value;
            int parameter_set = 0;
            if (!(rank_mask & (1 << rankx)))
                continue;

            lmc_rlevel_rank.u64 = cvmx_read_csr(CVMX_LMCX_RLEVEL_RANKX(rankx, ddr_interface_num));
            cvmx_write_csr(CVMX_LMCX_RLEVEL_RANKX(rankx, ddr_interface_num), 0x00430c30c50c30c3);

            if (parameter_set) {
                cvmx_write_csr(CVMX_LMCX_RLEVEL_RANKX(rankx, ddr_interface_num), lmc_rlevel_rank.u64);
                lmc_rlevel_rank.u64 = cvmx_read_csr(CVMX_LMCX_RLEVEL_RANKX(rankx, ddr_interface_num));

                ddr_print("Rank(%d) Rlevel Rank %#5x, 0x%016llX : %5d %5d %5d %5d %5d %5d %5d %5d %5d\n",
                          rankx,
                          lmc_rlevel_rank.cn70xx.status,
                          lmc_rlevel_rank.u64,
                          lmc_rlevel_rank.cn70xx.byte8,
                          lmc_rlevel_rank.cn70xx.byte7,
                          lmc_rlevel_rank.cn70xx.byte6,
                          lmc_rlevel_rank.cn70xx.byte5,
                          lmc_rlevel_rank.cn70xx.byte4,
                          lmc_rlevel_rank.cn70xx.byte3,
                          lmc_rlevel_rank.cn70xx.byte2,
                          lmc_rlevel_rank.cn70xx.byte1,
                          lmc_rlevel_rank.cn70xx.byte0
                          );
            }
        }
    }


    /*
     * 4.8.11 Final LMC Initialization
     * 
     * Early LMC initialization, LMC write-leveling, and LMC read-leveling
     * must be completed prior to starting this final LMC initialization.
     * 
     * LMC hardware updates the LMC(0)_SLOT_CTL0, LMC(0)_SLOT_CTL1,
     * LMC(0)_SLOT_CTL2 CSRs with minimum values based on the selected
     * readleveling and write-leveling settings. Software should not write
     * the final LMC(0)_SLOT_CTL0, LMC(0)_SLOT_CTL1, and LMC(0)_SLOT_CTL2
     * values until after the final read-leveling and write-leveling settings
     * are written.
     * 
     * Software must ensure the LMC(0)_SLOT_CTL0, LMC(0)_SLOT_CTL1, and
     * LMC(0)_SLOT_CTL2 CSR values are appropriate for this step. These CSRs
     * select the minimum gaps between read operations and write operations
     * of various types.
     * 
     * Software must not reduce the values in these CSR fields below the
     * values previously selected by the LMC hardware (during write-leveling
     * and read-leveling steps above).
     * 
     * All sections in this chapter may be used to derive proper settings for
     * these registers.
     * 
     * For minimal read latency, L2C_CTL[EF_ENA,EF_CNT] should be programmed
     * properly. This should be done prior to the first read.
     */

    /* Clear any residual ECC errors */
    cvmx_write_csr(CVMX_LMCX_INT(ddr_interface_num), -1ULL);
    int num_tads = 1;
    int tad;
    for (tad=0; tad<num_tads; tad++)
        cvmx_write_csr(CVMX_L2C_TADX_INT(tad), cvmx_read_csr(CVMX_L2C_TADX_INT(tad)));

    return(mem_size_mbytes);
}
#endif /* CONFIG_OCTEON3 */

#endif

/* Single byte address twsi read, used for reading MCU, DRAM eeproms */

int octeon_twsi_read(uint8_t dev_addr, uint8_t twsii_addr)
{
    /* 8 bit internal address */


    uint64_t val;
    cvmx_write_csr(CVMX_MIO_TWS_SW_TWSI,0x8000000000000000ull | (((uint64_t)dev_addr) << 40) | twsii_addr); //write the address we want to read
    while (cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI)&0x8000000000000000ull);
    val = cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI);
    if (!(val & 0x0100000000000000ull)) {
        //      printf("twsii write failed\n");
        return -1;
    }
    cvmx_write_csr(CVMX_MIO_TWS_SW_TWSI,0x8100000000000000ull | (((uint64_t)dev_addr) << 40)); // tell twsii to do the read
    while (cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI)&0x8000000000000000ull);
    val = cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI);
    if (!(val & 0x0100000000000000ull)) {
        //      printf("twsii read failed\n");
        return -1;
    }
    else {
        //    printf("twsii read address %d  val = %d\n",twsii_addr,(uint8_t)(val & 0xFF));

        return (uint8_t)(val & 0xFF);
    }
}

int octeon_ddr_initialize(uint32_t cpu_id,
                          uint32_t cpu_hertz,
                          uint32_t ddr_hertz,
                          uint32_t ddr_ref_hertz,
                          uint32_t ddr_interface_mask,
                          const ddr_configuration_t *ddr_config_array,
                          uint32_t *measured_ddr_hertz,
                          int board_type,
                          int board_rev_maj,
                          int board_rev_min)
{
    
    DECLARE_GLOBAL_DATA_PTR;
    uint32_t ddr_config_valid_mask = 0;
    int memsize_mbytes = 0;
    char *eptr;
    int retval;
    
    /* Enable L2 ECC */

    eptr = getenv("disable_l2_ecc");
    if (eptr)
    {
        printf("Disabling L2 ECC based on disable_l2_ecc environment variable\n");
        if (octeon_is_cpuid(OCTEON_CN6XXX) || octeon_is_cpuid(OCTEON_CN71XX))
        {
            cvmx_l2c_ctl_t l2c_val;
            l2c_val.u64 = cvmx_read_csr(CVMX_L2C_CTL);
            l2c_val.s.disecc = 1;
            cvmx_write_csr(CVMX_L2C_CTL, l2c_val.u64);
        }
        else
        {
            cvmx_write_csr(CVMX_L2D_ERR, 0);
            cvmx_write_csr(CVMX_L2T_ERR, 0);
        }
    }
    else
    {
        if (octeon_is_cpuid(OCTEON_CN6XXX) || octeon_is_cpuid(OCTEON_CN71XX))
        {
            cvmx_l2c_ctl_t l2c_val;
            l2c_val.u64 = cvmx_read_csr(CVMX_L2C_CTL);
            l2c_val.s.disecc = 0;
            cvmx_write_csr(CVMX_L2C_CTL, l2c_val.u64);
        }
        else
        {
            cvmx_write_csr(CVMX_L2D_ERR, 1);
            cvmx_write_csr(CVMX_L2T_ERR, 1);
        }
    }

#if !(CONFIG_OCTEON_NAND_STAGE2)
    {
        /* Init the L2C, must be done before DRAM access so that we know L2 is empty */
#if defined(__U_BOOT__)
        DECLARE_GLOBAL_DATA_PTR;
        if (!(gd->flags & GD_FLG_RAM_RESIDENT))
#endif
        {
            eptr = getenv("disable_l2_index_aliasing");
            if (eptr)
            {
                printf("L2 index aliasing disabled.\n");
                if (octeon_is_cpuid(OCTEON_CN6XXX) || octeon_is_cpuid(OCTEON_CN71XX))
                {
                    cvmx_l2c_ctl_t l2c_val;
                    l2c_val.u64 = cvmx_read_csr(CVMX_L2C_CTL);
                    l2c_val.s.disidxalias = 1;
                    cvmx_write_csr(CVMX_L2C_CTL, l2c_val.u64);
                }
                else
                {
                    cvmx_l2c_cfg_t l2c_cfg;
                    l2c_cfg.u64 = cvmx_read_csr(CVMX_L2C_CFG);
                    l2c_cfg.s.idxalias = 0;
                    cvmx_write_csr(CVMX_L2C_CFG,  l2c_cfg.u64);
                }
            }
            else
            {
                /* Enable L2C index aliasing */
                if(octeon_is_cpuid(OCTEON_CN6XXX) || octeon_is_cpuid(OCTEON_CN71XX))
                {
                    cvmx_l2c_ctl_t l2c_val;
                    l2c_val.u64 = cvmx_read_csr(CVMX_L2C_CTL);
                    l2c_val.s.disidxalias = 0;
                    cvmx_write_csr(CVMX_L2C_CTL, l2c_val.u64);
                }
                else
                {
                    cvmx_l2c_cfg_t l2c_cfg;
                    l2c_cfg.u64 = cvmx_read_csr(CVMX_L2C_CFG);
                    l2c_cfg.s.idxalias = 1;
                    cvmx_write_csr(CVMX_L2C_CFG,  l2c_cfg.u64);
                }
            }
        }
    }
#endif

    /* Check to see if we should limit the number of L2 ways. */
    eptr = getenv("limit_l2_ways");
    if (eptr) {
        int ways = simple_strtoul(eptr, NULL, 10);
        limit_l2_ways(ways, 1);
    }

    /* Check for lower DIMM socket populated */
    if ((ddr_interface_mask & 0x1) && validate_dimm(ddr_config_array[0].dimm_config_table[0], 0))
        ddr_config_valid_mask = 1;
    if ((ddr_interface_mask & 0x2) && validate_dimm(ddr_config_array[1].dimm_config_table[0], 0))
        ddr_config_valid_mask |= (1<<1);

    if (!ddr_config_valid_mask)
    {
        if (gd->board_desc.board_dram == DRAM_TYPE_SPD) {
            printf("ERROR: No valid DIMMs detected on any DDR interface.!!!\n");
            return(-1);
        } else {
            /* 
             * for non-spd memory models we fake DIMM presence here...its OK though..
             * as the meomory is soldered on the board, it will always be present 
             */
            ddr_config_valid_mask = 1;
        }
    }

    /* Except for Octeon pass1 we can count DDR clocks, and we use
    ** this to correct the DDR clock that we are passed. For Octeon
    ** pass1 the measured value will always be returned as 0. */
    uint32_t calc_ddr_hertz = 0;

    if (ddr_config_valid_mask & 1){
        calc_ddr_hertz = measure_octeon_ddr_clock(cpu_id, ddr_config_array,
                                                  cpu_hertz, ddr_hertz,
                                                  ddr_ref_hertz, 0,
                                                  ddr_config_valid_mask);
    }
    if (ddr_config_valid_mask & 2){
        uint32_t tmp_hertz = measure_octeon_ddr_clock(cpu_id, ddr_config_array,
                                                      cpu_hertz, ddr_hertz,
                                                      ddr_ref_hertz, 1,
                                                      ddr_config_valid_mask);
        if (tmp_hertz > 0)
            calc_ddr_hertz = tmp_hertz;
    }

    if (measured_ddr_hertz)
        *measured_ddr_hertz = calc_ddr_hertz;

    memsize_mbytes = 0;
    if (ddr_config_valid_mask & 1){
        retval = init_octeon_dram_interface(cpu_id,
                                            ddr_config_array,
                                            ddr_hertz, /* FIXME: This should be qualified by the measured value? */
                                            cpu_hertz,
                                            ddr_ref_hertz,
                                            board_type,
                                            board_rev_maj,
                                            board_rev_min,
                                            0,
                                            ddr_config_valid_mask
                                            );
        if (retval > 0)
            memsize_mbytes = retval;
    }


    if (ddr_config_valid_mask & 2){
        retval = init_octeon_dram_interface(cpu_id,
                                              ddr_config_array + 1,
                                              ddr_hertz, /* FIXME: This should be qualified by the measured value? */
                                              cpu_hertz,
                                              ddr_ref_hertz,
                                              board_type,
                                              board_rev_maj,
                                              board_rev_min,
                                              1,
                                              ddr_config_valid_mask
                                              );
        if (retval > 0)
            memsize_mbytes += retval;
    }
    if (memsize_mbytes == 0)
        return(-1);     /* Both interfaces failed to initialize, so return error */

    eptr = getenv("limit_dram_mbytes");
    if (eptr)
    {
        unsigned int mbytes = simple_strtoul(eptr, NULL, 10);
        if (mbytes > 0)
        {
            memsize_mbytes = mbytes;
            printf("Limiting DRAM size to %d MBytes based on limit_dram_mbytes env. variable\n", mbytes);
        }
    }

    return(memsize_mbytes);
}

int twsii_mcu_read(uint8_t twsii_addr)
{
#ifdef BOARD_MCU_TWSI_ADDR
    return(octeon_twsi_read(BOARD_MCU_TWSI_ADDR, twsii_addr));
#else
    return(0);
#endif /* BOARD_MCU_TWSI_ADDR */
}

#if defined(CONFIG_JSRXNLE) || defined(CONFIG_MAG8600)

int octeon_twsi_read_cur_addr16(uint8_t dev_addr)
{

    uint64_t val;

    cvmx_write_csr(CVMX_MIO_TWS_SW_TWSI,0x8100000000000000ull | ( 0x0ull << 57) | ( 0x1ull << 55) | ( 0x1ull << 52) | (((uint64_t)dev_addr) << 40) );
    while (cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI)&0x8000000000000000ull);
    val = cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI);
    if (!(val & 0x0100000000000000ull))
    {
        printf("octeon_twsi_read8_cur_addr: Failed to read\n");
        return -1;
    }
    else
    {
        return(uint16_t)(val & 0xFFFF);
    }
}

int octeon_twsi_write_reg16(uint8_t dev_addr, uint8_t addr, uint16_t data)
{

    uint64_t val, val_1;

    val_1= 0x8000000000000000ull | ( 0x0ull << 57) | ( 0x1ull << 55) | ( 0x2ull << 52) | (((uint64_t)dev_addr) << 40) | (((uint64_t)addr) << 16) | data ;
    cvmx_write_csr(CVMX_MIO_TWS_SW_TWSI, val_1 ); // tell twsii to do the read
    while (cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI)&0x8000000000000000ull);
    val = cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI);
    if (!(val & 0x0100000000000000ull)) {
        printf("twsii write (3byte) failed\n");
        return -1;
    }

    /* or poll for read to not fail */
    //octeon_delay_cycles(40*1000000);
    return(0);
}

int octeon_twsi_write_n(uint8_t dev_addr, uint16_t addr, void *data, int len)
{
    uint64_t val, val_1, mask, nbyte;

    if ((len <= 0) || ( len > 3)) {
        printf("octeon_twsi_read_cur_addr:: invalid length \n");
        return 1;
    }

    nbyte = len;

    mask = (1 << (8 * len)) - 1;
    val = *(uint64_t *)data;
    printf("octeon_twsi_write_n: value to write: 0x%x \n", val);
    val_1= 0x8000000000000000ull | ( 0x0ull << 57) | ( 0x1ull << 55) | ( nbyte << 52 ) | (((uint64_t)dev_addr) << 40) |
        (((uint64_t)addr) << (8 * len)) | (val & mask) ;
    cvmx_write_csr(CVMX_MIO_TWS_SW_TWSI, val_1 ); // tell twsii to do the read
    while (cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI)&0x8000000000000000ull);
    val = cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI);
    if (!(val & 0x0100000000000000ull)) {
        printf("twsii write (2byte) failed\n");
        return -1;
    }

    /* or poll for read to not fail */
    //octeon_delay_cycles(40*1000000);
    return(0);

}

/*
 * octeon_twsi_write_max function is used to write data to twsi
 * devices which doesn't have internal offset addresses.
 */
int32_t
octeon_twsi_write_max (uint8_t dev_addr, uint16_t addr, uint8_t *data,
                      int32_t len)
{
    uint64_t val = 0, val_1 = 0, nbyte;
    uint64_t tmp = 0, index;
    uint32_t loop_count = 0;

    if ((len <= 0) || (len > 8)) {
        printf("octeon_twsi_read_cur_addr:: invalid length \n");
        return -1;
    }

    for (index = 0; index < len; index++) {
        tmp = ((tmp << 8) | data[index]);
    }

    nbyte = len - 1;
    if (addr == TWSI_NO_OFFSET) {
        /* tell twsi to send data */
        val = (TWSI_VALID_BIT | (0x0ull << MIO_TWS_SW_TWSI_REG_OPS_SHIFT) |
              (0x1ull << MIO_TWS_SW_TWSI_REG_SOVR_SHIFT) |
              (nbyte << MIO_TWS_SW_TWSI_REG_SIZE_SHIFT) |
              (((uint64_t)dev_addr) << MIO_TWS_SW_TWSI_REG_DEV_ADDR_SHIFT));
        val |= (uint64_t)(tmp & 0xffffffff);
        val_1 = (uint32_t)(tmp >> 32);
        cvmx_write_csr(CVMX_MIO_TWS_SW_TWSI_EXT, val_1);
        cvmx_write_csr(CVMX_MIO_TWS_SW_TWSI, val);
        /* polls till twsi master mode op complete's or twsi timeout */
        while (cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI) & TWSI_VALID_BIT) {
            /* delay of 2ms */
            octeon_delay_cycles(TWSI_TIMEOUT_CYCLES);
            if (loop_count > TWSI_TIMEOUT_COUNT) {
                printf("octeon_twsi_write_max:failed\n");
                return -1;
            }
            loop_count++;
        }
        /* check for opertation status */
        val = cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI);
        if (!(val & TWSI_STATUS_BIT)) {
            printf("twsi write max: failed\n");
            return -1;
        }
    }
    octeon_delay_cycles((40 * TWSI_TIMEOUT_CYCLES));
    return 0;
}

/*
 * octeon_twsi_read_max function is used to read data from twsi
 * devices which doesn't have internal offset addresses.
 */
int32_t
octeon_twsi_read_max (uint8_t dev_addr, uint16_t addr, uint8_t *data,
                     int32_t len)
{
    uint64_t val = 0, val_1 = 0, nbyte;
    uint64_t tmp64;
    int32_t index = 0;
    int32_t loop_count = 0;

    if ((len <= 0) || (len > 8)) {
        printf("octeon_twsi_read_cur_addr:: invalid length \n");
        return -1;
    }

    nbyte = len - 1;
    if (addr == TWSI_NO_OFFSET) {
       /* tell twsi to read */
       val = (TWSI_VALID_BIT | (0x0ull << MIO_TWS_SW_TWSI_REG_OPS_SHIFT) |
             (0x1ull << MIO_TWS_SW_TWSI_REG_SOVR_SHIFT) |
             (nbyte << MIO_TWS_SW_TWSI_REG_SIZE_SHIFT) |
             (((uint64_t)dev_addr) << MIO_TWS_SW_TWSI_REG_DEV_ADDR_SHIFT) |
             (0x1ull << MIO_TWS_SW_TWSI_REG_READ_OR_RES_SHIFT));
       cvmx_write_csr(CVMX_MIO_TWS_SW_TWSI, val);
       while (cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI) & TWSI_VALID_BIT) {
           /* delay of 2ms */
           octeon_delay_cycles(TWSI_TIMEOUT_CYCLES);
           if (loop_count > TWSI_TIMEOUT_COUNT) {
               printf("octeon_twsi_read_max:failed\n");
               return -1;
           }
           loop_count++;
       }
       /* check for opertaion status */
       val = cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI);
       if (!(val & TWSI_STATUS_BIT)) {
           printf("twsi read max: failed\n");
           return -1;
       }
    }
    /*
     * read lower and higher 32 bit data from
     * SW_TWSI and SW_TWSI_EXT register's
     */
    val = cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI);
    val_1 =  cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI_EXT);
    tmp64 = ((uint64_t) val_1 << 32) | (uint32_t)val;

    for (index = (len - 1); index >= 0; index--) {
        data[index] = (uint8_t)tmp64;
        tmp64 >>= 8;
    }

    return 0;
}

#endif

static void set_ddr_clock_initialized(int ddr_interface, int inited_flag)
{
#ifdef __U_BOOT__
    DECLARE_GLOBAL_DATA_PTR;
    if (inited_flag)
        gd->flags |= (GD_FLG_DDR0_CLK_INITIALIZED << ddr_interface);
    else
        gd->flags &= ~(GD_FLG_DDR0_CLK_INITIALIZED << ddr_interface);
#else
    if (inited_flag)
        global_ddr_clock_initialized |= (0x1 << ddr_interface);
    else
        global_ddr_clock_initialized &= ~(0x1 << ddr_interface);
#endif

}
static int ddr_clock_initialized(int ddr_interface)
{
#ifdef __U_BOOT__
    DECLARE_GLOBAL_DATA_PTR;
    return (!!(gd->flags & (GD_FLG_DDR0_CLK_INITIALIZED << ddr_interface)));
#else
    return (!!(global_ddr_clock_initialized & (0x1 << ddr_interface)));
#endif
}

static int
initialize_octeon3_ddr3_clock(uint32_t cpu_id,
                              const ddr_configuration_t *ddr_configuration,
                              uint32_t cpu_hertz,
                              uint32_t ddr_hertz,
                              uint32_t ddr_ref_hertz,
                              int ddr_interface_num,
                              uint32_t ddr_interface_mask)
{
#if defined (__U_BOOT__) || defined (unix)
    char *s;
#endif

    cvmx_lmcx_ddr_pll_ctl_t ddr_pll_ctl;
    cvmx_lmcx_dll_ctl2_t	dll_ctl2;
    cvmx_lmcx_reset_ctl_t lmc_reset_ctl;

    /*
     * 4.8 LMC Initialization Sequence
     *
     * There are eleven parts to the LMC initialization procedure:
     *
     * 1. DDR PLL initialization
     *
     * 2. LMC CK initialization
     *
     * 3. LMC DRESET initialization
     *
     * 4. LMC RESET initialization
     *
     * 5. Early LMC initialization
     *
     * 6. LMC offset training
     *
     * 7. LMC internal Vref training
     *
     * 8. LMC deskew training
     *
     * 9. LMC write leveling
     *
     * 10. LMC read leveling
     *
     * 11. Final LMC initialization
     *
     * During a cold reset, all eight initializations should be executed in
     * sequence. The DDR PLL and LMC CK initializations need only be
     * performed once if the SDRAM-clock speed and parameters do not
     * change. Subsequently, it is possible to execute only DRESET and
     * subsequent steps, or to execute only the final three steps.
     *
     * The remainder of this section covers these initialization steps in
     * sequence.
     *
     * 4.8.1 DDR PLL Initialization
     *
     * Perform the following eight substeps to initialize the DDR PLL:
     *
     * 1. If not done already, write all fields in LMC(0)_DDR_PLL_CTL and
     *    LMC(0)_DLL_CTL2 to their reset values, including:
     *
     *    - LMC(0)_DDR_PLL_CTL[DDR_DIV_RESET] = 1
     *
     *    - LMC(0)_DLL_CTL2[DRESET] = 1
     *
     *    This substep is not necessary after a chip cold reset/power up.
     */

    ddr_pll_ctl.u64	= 0;

    ddr_pll_ctl.cn70xx.clkf              = 0x30;
    ddr_pll_ctl.cn70xx.reset_n           = 0;
    ddr_pll_ctl.cn70xx.ddr_ps_en         = 2;
    ddr_pll_ctl.cn70xx.ddr_div_reset     = 1;
    ddr_pll_ctl.cn70xx.jtg_test_mode     = 0;
    ddr_pll_ctl.cn70xx.clkr              = 0;
    ddr_pll_ctl.cn70xx.pll_rfslip        = 0;
    ddr_pll_ctl.cn70xx.pll_lock          = 0;
    ddr_pll_ctl.cn70xx.pll_fbslip        = 0;
    ddr_pll_ctl.cn70xx.ddr4_mode         = 0;
    ddr_pll_ctl.cn70xx.phy_dcok          = 0;

    cvmx_write_csr(CVMX_LMCX_DDR_PLL_CTL(0), ddr_pll_ctl.u64);


    dll_ctl2.u64 = 0;

    dll_ctl2.cn70xx.byp_setting          = 0;
    dll_ctl2.cn70xx.byp_sel              = 0;
    dll_ctl2.cn70xx.quad_dll_ena         = 0;
    dll_ctl2.cn70xx.dreset               = 1;
    dll_ctl2.cn70xx.dll_bringup          = 0;
    dll_ctl2.cn70xx.intf_en              = 0;

    cvmx_write_csr(CVMX_LMCX_DLL_CTL2(0), dll_ctl2.u64);

    /*
     * 2. If the current DRAM contents are not preserved (see
     *    LMC(0)_RESET_CTL[DDR3PSV]), this is also an appropriate time to
     *    assert the RESET# pin of the DDR3 DRAM parts. If desired, write
     *    LMC(0)_RESET_CTL[DDR3RST] = 0 without modifying any other
     *    LMC(0)_RESET_CTL fields to assert the DDR_RESET_L pin. No action is
     *    required here to assert DDR_RESET_L following a chip cold
     *    reset/power up.  Refer to Section 4.8.4.
     */

    {
        cvmx_lmcx_reset_ctl_t reset_ctl;
        reset_ctl.u64 = cvmx_read_csr(CVMX_LMCX_RESET_CTL(ddr_interface_num));
        if (reset_ctl.cn70xx.ddr3psv == 0) {
            ddr_print("Asserting DDR_RESET_L\n");
            reset_ctl.cn70xx.ddr3rst = 0; /* Reset asserted */
            cvmx_write_csr(CVMX_LMCX_RESET_CTL(ddr_interface_num), reset_ctl.u64);
            cvmx_read_csr(CVMX_LMCX_RESET_CTL(ddr_interface_num));
        }
    }

    /*
     * 3. Without changing any other LMC(0)_DDR_PLL_CTL values, write
     *    LMC(0)_DDR_PLL_CTL[CLKF] with a value that gives a desired DDR PLL
     *    speed. The LMC(0)_DDR_PLL_CTL[CLKF] value should be selected in
     *    conjunction with the post-scalar divider values for LMC
     *    (LMC(0)_DDR_PLL_CTL[DDR_PS_EN]) so that the desired LMC CK speeds
     *    are is produced. Section 4.13 describes LMC(0)_DDR_PLL_CTL[CLKF]
     *    and LMC(0)_DDR_PLL_CTL[DDR_PS_EN] programmings that produce the
     *    desired LMC CK speed. Section 4.8.2 describes LMC CK
     *    initialization, which can be done separately from the DDR PLL
     *    initialization described in this section.
     *
     *    The LMC(0)_DDR_PLL_CTL[CLKF] value must not change after this point
     *    without restarting this SDRAM PLL initialization sequence.
     */

    {
        /* CLKF = (DCLK * (CLKR+1) * EN(1, 2, 3, 4, 5, 6, 7, 8, 10, 12))/DREF - 1 */
        int en_idx, save_en_idx, best_en_idx=0;
        uint64_t clkf, clkr, max_clkf = 64;
        uint64_t best_clkf=0, best_clkr=0;
        uint64_t best_pll_MHz = 0, pll_MHz, min_pll_MHz = 700, max_pll_MHz = 3500;
        uint64_t error = ddr_hertz, best_error = ddr_hertz; /* Init to max error */;
        uint64_t lowest_clkf = max_clkf;
        uint64_t best_calculated_ddr_hertz = 0, calculated_ddr_hertz = 0;
        uint64_t orig_ddr_hertz = ddr_hertz;
        static const int _en[] = {1, 2, 3, 4, 5, 6, 7, 8, 10, 12};

        ddr_pll_ctl.u64 = 0;

        while (best_error == ddr_hertz)
        {

            for (clkr = 0; clkr < 4; ++clkr) {
                for (en_idx=sizeof(_en)/sizeof(int)-1; en_idx>=0; --en_idx) {
                    save_en_idx = en_idx;
                    clkf = ((ddr_hertz) * (clkr+1) * (_en[save_en_idx]));
                    clkf = divide_nint(clkf, ddr_ref_hertz) - 1;
                    pll_MHz = ddr_ref_hertz * (clkf+1) / (clkr+1) / 1000000;
                    calculated_ddr_hertz = ddr_ref_hertz * (clkf + 1) / ((clkr + 1) * (_en[save_en_idx]));
                    error = ddr_hertz - calculated_ddr_hertz;

                    if ((pll_MHz < min_pll_MHz) || (pll_MHz > max_pll_MHz)) continue;
                    if (clkf > max_clkf) continue; /* PLL requires clkf to be limited */
                    if (_abs(error) > _abs(best_error)) continue;

#if defined(__U_BOOT__)
                    ddr_print("clkr: %2qu, en: %2d, clkf: %4qu, pll_MHz: %4qu, ddr_hertz: %8qu, error: %8qd\n",
                              clkr, _en[save_en_idx], clkf, pll_MHz, calculated_ddr_hertz, error);
#else
                    ddr_print("clkr: %2llu, en: %2d, clkf: %4llu, pll_MHz: %4llu, ddr_hertz: %8llu, error: %8lld\n",
                              clkr, _en[save_en_idx], clkf, pll_MHz, calculated_ddr_hertz, error);
#endif

                    if ((_abs(error) < _abs(best_error)) || (clkf < lowest_clkf)) {
                        lowest_clkf = clkf;
                        best_pll_MHz = pll_MHz;
                        best_calculated_ddr_hertz = calculated_ddr_hertz;
                        best_error = error;
                        best_clkr = clkr;
                        best_clkf = clkf;
                        best_en_idx = save_en_idx;
                    }
                }
            }

#if defined(__U_BOOT__)
            ddr_print("clkr: %2qu, en: %2d, clkf: %4qu, pll_MHz: %4qu, ddr_hertz: %8qu, error: %8qd <==\n",
                      best_clkr, _en[best_en_idx], best_clkf, best_pll_MHz, best_calculated_ddr_hertz, best_error);
#else
            ddr_print("clkr: %2llu, en: %2d, clkf: %4llu, pll_MHz: %4llu, ddr_hertz: %8llu, error: %8lld <==\n",
                      best_clkr, _en[best_en_idx], best_clkf, best_pll_MHz, best_calculated_ddr_hertz, best_error);
#endif

            /* Try lowering the frequency if we can't get a working configuration */
            if (best_error == ddr_hertz) {
                if (ddr_hertz < orig_ddr_hertz - 10000000)
                    break;
                ddr_hertz -= 1000000;
                best_error = ddr_hertz;
            }

        }


        if (best_error == ddr_hertz) {
            error_print("ERROR: Can not compute a legal DDR clock speed configuration.\n");
            return(-1);
        }

        ddr_pll_ctl.cn70xx.ddr_ps_en = best_en_idx;
        ddr_pll_ctl.cn70xx.clkf = best_clkf;
        ddr_pll_ctl.cn70xx.clkr = best_clkr;
        ddr_pll_ctl.cn70xx.reset_n = 0;

        cvmx_write_csr(CVMX_LMCX_DDR_PLL_CTL(0), ddr_pll_ctl.u64);
    }


    /*
     * 4. Read LMC(0)_DDR_PLL_CTL and wait for the result.
     */

    cvmx_read_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num));

    /*
     * 5. Wait a minimum of 3 us.
     */

    cvmx_wait_usec(3);          /* Wait 3 us */

    /*
     * 6. Write LMC(0)_DDR_PLL_CTL[RESET_N] = 1 without changing any other
     *    LMC(0)_DDR_PLL_CTL values.
     */

    ddr_pll_ctl.u64 = cvmx_read_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num));
    ddr_pll_ctl.cn70xx.reset_n = 1;
    cvmx_write_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num), ddr_pll_ctl.u64);

    /*
     * 7. Read LMC(0)_DDR_PLL_CTL and wait for the result.
     */

    cvmx_read_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num));

    /*
     * 8. Wait a minimum of 25 us.
     */

    cvmx_wait_usec(25);          /* Wait 25 us */

    /*
     * 4.8.2 LMC CK Initialization
     *
     * DDR PLL initialization must be completed prior to starting LMC CK
     * initialization.
     *
     * Perform the following seven substeps to initialize the LMC CK:
     *
     * 1. Without changing any other LMC(0)_DDR_PLL_CTL values, write
     *    LMC(0)_DDR_PLL_CTL[DDR_DIV_RESET] = 1 and
     *    LMC(0)_DDR_PLL_CTL[DDR_PS_EN] with the appropriate value to get the
     *    desired LMC CK speed. Section Section 4.13 discusses CLKF and
     *    DDR_PS_EN programmings.
     *
     *    The LMC(0)_DDR_PLL_CTL[DDR_PS_EN] must not change after this point
     *    without restarting this LMC CK initialization sequence.
     */

    ddr_pll_ctl.u64 = cvmx_read_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num));
    ddr_pll_ctl.cn70xx.ddr_div_reset = 1;
    cvmx_write_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num), ddr_pll_ctl.u64);

    /*
     * 2. Without changing any other fields in LMC(0)_DDR_PLL_CTL, write
     *    LMC(0)_DDR_PLL_CTL[DDR4_MODE] = 0.
     */

    ddr_pll_ctl.u64 = cvmx_read_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num));
    ddr_pll_ctl.cn70xx.ddr4_mode = 0;
    cvmx_write_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num), ddr_pll_ctl.u64);

    /*
     * 3. Read LMC(0)_DDR_PLL_CTL and wait for the result.
     */

    cvmx_read_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num));

    /*
     * 4. Wait a minimum of 1 us.
     */

    cvmx_wait_usec(1);          /* Wait 1 us */

    /*
     * 5. Without changing any other fields in LMC(0)_DDR_PLL_CTL, write
     *    LMC(0)_DDR_PLL_CTL[PHY_DCOK] = 1.
     */

    ddr_pll_ctl.u64 = cvmx_read_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num));
    ddr_pll_ctl.cn70xx.phy_dcok = 1;
    cvmx_write_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num), ddr_pll_ctl.u64);

    /*
     * 6. Read LMC(0)_DDR_PLL_CTL and wait for the result.
     */

    cvmx_read_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num));

    /*
     * 7. Wait a minimum of 20 us.
     */

    cvmx_wait_usec(20);          /* Wait 20 us */

    /*
     * 8. Without changing any other LMC(0)_COMP_CTL2 values, write
     *    LMC(0)_COMP_CTL2[CK_CTL] to the desired DDR_CK_*_P/
     *    DDR_DIMM*_CS*_L/DDR_DIMM*_ODT_* drive strength.
     */

    {
        cvmx_lmcx_comp_ctl2_t comp_ctl2;
	    const ddr3_custom_config_t *custom_lmc_config = &ddr_configuration->custom_lmc_config;

        comp_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num));

	    comp_ctl2.cn70xx.dqx_ctl  =
            (custom_lmc_config->dqx_ctl == 0) ? 4 : custom_lmc_config->dqx_ctl; /* Default 4=34.3 ohm */
	    comp_ctl2.cn70xx.ck_ctl   =
            (custom_lmc_config->ck_ctl  == 0) ? 4 : custom_lmc_config->ck_ctl;  /* Default 4=34.3 ohm */
	    comp_ctl2.cn70xx.cmd_ctl  =
            (custom_lmc_config->cmd_ctl == 0) ? 4 : custom_lmc_config->cmd_ctl; /* Default 4=34.3 ohm */

        comp_ctl2.cn70xx.rodt_ctl           = 0x4; /* 60 ohm */

        if ((s = lookup_env_parameter("ddr_clk_ctl")) != NULL) {
            comp_ctl2.cn70xx.ck_ctl  = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_ck_ctl")) != NULL) {
            comp_ctl2.cn70xx.ck_ctl  = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_cmd_ctl")) != NULL) {
            comp_ctl2.cn70xx.cmd_ctl  = simple_strtoul(s, NULL, 0);
        }

        if ((s = lookup_env_parameter("ddr_dqx_ctl")) != NULL) {
            comp_ctl2.cn70xx.dqx_ctl  = simple_strtoul(s, NULL, 0);
        }

        cvmx_write_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num), comp_ctl2.u64);
    }

    /*
     * 9. Read LMC(0)_DDR_PLL_CTL and wait for the result.
     */

    cvmx_read_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num));

    /*
     * 10. Wait a minimum of 200 ns.
     */

    cvmx_wait_usec(1);          /* Wait 1 us */

    /*
     * 11. Without changing any other LMC(0)_DDR_PLL_CTL values, write
     *     LMC(0)_DDR_PLL_CTL[DDR_DIV_RESET] = 0.
     */

    ddr_pll_ctl.u64 = cvmx_read_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num));
    ddr_pll_ctl.cn70xx.ddr_div_reset = 0;
    cvmx_write_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num), ddr_pll_ctl.u64);

    /*
     * 12. Read LMC(0)_DDR_PLL_CTL and wait for the result.
     */

    cvmx_read_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num));

    /*
     * 13. Wait a minimum of 200 ns.
     */

    cvmx_wait_usec(1);          /* Wait 1 us */

    /*
     * 4.8.3 LMC DRESET Initialization
     *
     * Both the DDR PLL and LMC CK initializations must be completed prior to
     * starting this LMC DRESET initialization.
     *
     * 1. If not done already, write LMC(0)_DLL_CTL2 to its reset value,
     *    including LMC(0)_DLL_CTL2[DRESET] = 1.
     */

    /* Already done. */

    /*
     * 2. Without changing any other LMC(0)_DLL_CTL2 fields, write
     *    LMC(0)_DLL_CTL2 [DLL_BRINGUP] = 1.
     */


    dll_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_DLL_CTL2(ddr_interface_num));
    dll_ctl2.cn70xx.dll_bringup = 1;
    cvmx_write_csr(CVMX_LMCX_DLL_CTL2(ddr_interface_num), dll_ctl2.u64);

    /*
     * 3. Read LMC(0)_DLL_CTL2 and wait for the result.
     */

    cvmx_read_csr(CVMX_LMCX_DLL_CTL2(ddr_interface_num));

    /*
     * 4. Wait for a minimum of 10 LMC CK cycles.
     */

    cvmx_wait_usec(1);

    /*
     * 5. Without changing any other fields in LMC(0)_DLL_CTL2, write
     *    LMC(0)_DLL_CTL2[QUAD_DLL_ENA] = 1.  LMC(0)_DLL_CTL2[QUAD_DLL_ENA]
     *    must not change after this point without restarting the LMC DRESET
     *    initialization sequence.
     */

    dll_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_DLL_CTL2(ddr_interface_num));
    dll_ctl2.cn70xx.quad_dll_ena = 1;
    cvmx_write_csr(CVMX_LMCX_DLL_CTL2(ddr_interface_num), dll_ctl2.u64);

    /*
     * 6. Read LMC(0)_DLL_CTL2 and wait for the result.
     */

    cvmx_read_csr(CVMX_LMCX_DLL_CTL2(ddr_interface_num));

    /*
     * 7. Wait a minimum of 10 us.
     */

    cvmx_wait_usec(10);

    /*
     * 8. Without changing any other fields in LMC(0)_DLL_CTL2, write
     *    LMC(0)_DLL_CTL2[DLL_BRINGUP] = 0.  LMC(0)_DLL_CTL2[DLL_BRINGUP]
     *    must not change after this point without restarting the LMC DRESET
     *    initialization sequence.
     */

    dll_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_DLL_CTL2(ddr_interface_num));
    dll_ctl2.cn70xx.dll_bringup = 0;
    cvmx_write_csr(CVMX_LMCX_DLL_CTL2(ddr_interface_num), dll_ctl2.u64);

    /*
     * 9. Read LMC(0)_DLL_CTL2 and wait for the result.
     */

    cvmx_read_csr(CVMX_LMCX_DLL_CTL2(ddr_interface_num));

    /*
     * 10. Without changing any other fields in LMC(0)_DLL_CTL2, write
     *     LMC(0)_DLL_CTL2[DRESET] = 0.
     *
     *     LMC(0)_DLL_CTL2[DRESET] must not change after this point without
     *     restarting the LMC DRESET initialization sequence.
     *
     * After completing LMC DRESET initialization, all LMC CSRs may be
     * accessed. Prior to completing LMC DRESET initialization, only
     * LMC(0)_DDR_PLL_CTL, LMC(0)_DLL_CTL2, LMC(0)_RESET_CTL, and
     * LMC(0)_COMP_CTL2 LMC CSRs can be accessed.
     */

    dll_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_DLL_CTL2(ddr_interface_num));
    dll_ctl2.cn70xx.dreset= 0;
    cvmx_write_csr(CVMX_LMCX_DLL_CTL2(ddr_interface_num), dll_ctl2.u64);

    /*
     * 4.8.4 LMC RESET Initialization
     *
     * The purpose of this step is to assert/deassert the RESET# pin at the
     * DDR3 parts.
     *
     * It may be appropriate to skip this step if the DDR3 DRAM parts are in
     * self refresh and are currently preserving their contents. (Software
     * can determine this via LMC(0)_RESET_CTL[DDR3PSV] in some
     * circumstances.) The remainder of this section assumes that the DRAM
     * contents need not be preserved.
     *
     * The remainder of this section assumes that the CN70XX DDR_RESET_L pin
     * is attached to the RESET# pin of the attached DDR3 parts, as will be
     * appropriate in many systems.
     *
     * (In other systems, such as ones that can preserve DDR3 part contents
     * while CN70XX is powered down, it will not be appropriate to directly
     * attach the CN70XX DDR_RESET_L pin to DRESET# of the DDR3 parts, and
     * this section may not apply.)
     *
     * Perform the following six substeps for LMC reset initialization:
     *
     * 1. If not done already, assert DDR_RESET_L pin by writing
     *    LMC(0)_RESET_CTL[DDR3RST] = 0 without modifying any other
     *    LMC(0)_RESET_CTL fields.
     */

    lmc_reset_ctl.u64 = cvmx_read_csr(CVMX_LMCX_RESET_CTL(ddr_interface_num));
    if (lmc_reset_ctl.cn70xx.ddr3psv == 0) {
        /*
         * 2. Read LMC(0)_RESET_CTL and wait for the result.
         */

        cvmx_read_csr(CVMX_LMCX_RESET_CTL(ddr_interface_num));

        /*
         * 3. Wait until RESET# assertion-time requirement from JEDEC DDR3
         *    specification is satisfied (200 us during a power-on ramp, 100ns
         *    when power is already stable).
         */

        cvmx_wait_usec(200);

        /*
         * 4. Deassert DDR_RESET_L pin by writing LMC(0)_RESET_CTL[DDR3RST] = 1
         *    without modifying any other LMC(0)_RESET_CTL fields.
         */

        lmc_reset_ctl.u64 = cvmx_read_csr(CVMX_LMCX_RESET_CTL(ddr_interface_num));
        ddr_print("De-asserting DDR_RESET_L\n");
        lmc_reset_ctl.cn70xx.ddr3rst = 1;
        cvmx_write_csr(CVMX_LMCX_RESET_CTL(ddr_interface_num), lmc_reset_ctl.u64);

        /*
         * 5. Read LMC(0)_RESET_CTL and wait for the result.
         */

        cvmx_read_csr(CVMX_LMCX_RESET_CTL(ddr_interface_num));

        /*
         * 6. Wait a minimum of 500us. This guarantees the necessary T = 500us
         *    delay between DDR_RESET_L deassertion and DDR_CKE* assertion.
         */

        cvmx_wait_usec(500);
    }

    return 0;
}

int initialize_ddr_clock(uint32_t cpu_id,
                         const ddr_configuration_t *ddr_configuration,
                         uint32_t cpu_hertz,
                         uint32_t ddr_hertz,
                         uint32_t ddr_ref_hertz,
                         int ddr_interface_num,
                         uint32_t ddr_interface_mask
                         )
{
#if defined(__U_BOOT__) || defined(unix)
    char *s;
#endif
    cvmx_lmc_mem_cfg0_t mem_cfg0;
    DECLARE_GLOBAL_DATA_PTR;
#ifdef CONFIG_JSRXNLE
    const lmc_reg_val_t *lmc_reg = NULL;

#ifdef __U_BOOT__
    {
        DECLARE_GLOBAL_DATA_PTR;

#if CONFIG_L2_ONLY
        /* L2 only can be useful for the initial bringup of the DDR
           interface. Verbose mode is necessary for that situation. */
        gd->flags |= GD_FLG_DDR_VERBOSE;
#endif
    }
#endif
    if (ddr_clock_initialized(ddr_interface_num))
        return 0;


    lmc_reg = &lmc_reg_val[gd->board_desc.board_dram];

    switch (gd->board_desc.board_dram) {
    case QIMONDA_8x2Gb:
#ifdef CONFIG_JSRXNLE
         if (is_proto_id_supported()) {
            lmc_reg = IS_PLATFORM_SRX210_533(gd->board_desc.board_type) ?
                &lmc_reg_val[HIGH_MEM_8x2Gb_SRX210_533] : 
                &lmc_reg_val[HIGH_MEM_8x2Gb_SRX100];
        }
#endif
        break;
    case QIMONDA_8x1Gb:
#ifdef CONFIG_JSRXNLE
        if (is_proto_id_supported()) {
            lmc_reg = IS_PLATFORM_SRX210_533(gd->board_desc.board_type) ?
                &lmc_reg_val[HIGH_MEM_8x1Gb_SRX210_533] : 
                &lmc_reg_val[HIGH_MEM_8x1Gb_SRX210_P4];
        }
#endif
        break;
    case QIMONDA_4x1Gb:
#ifdef CONFIG_JSRXNLE
        if (is_proto_id_supported()) {
            lmc_reg = IS_PLATFORM_SRX210_533(gd->board_desc.board_type) ? 
                &lmc_reg_val[LOW_MEM_4x1Gb_SRX210_533] :
                &lmc_reg_val[LOW_MEM_4x1Gb_SRX210_P4];
        }
#endif
        break;
    case QIMONDA_4x2Gb:
#ifdef CONFIG_JSRXNLE
            lmc_reg = &lmc_reg_val[LOW_MEM_4x2Gb_SRX210_533];
#endif
        break;
    }
#endif
    if ((octeon_is_cpuid(OCTEON_CN31XX)) || (octeon_is_cpuid(OCTEON_CN30XX))) {
        cvmx_lmc_ctl_t lmc_ctl;
        cvmx_lmc_ddr2_ctl_t ddr2_ctl;

        /*
         * DRAM Controller Initialization
         * The reference-clock inputs to the LMC (DDR2_REF_CLK_*) should be stable
         * when DCOK asserts (refer to Section 26.3). DDR_CK_* should be stable from that
         * point, which is more than 200 us before software can bring up the main memory
         * DRAM interface. The generated DDR_CK_* frequency is four times the
         * DDR2_REF_CLK_* frequency.
         * To initialize the main memory and controller, software must perform the following
         * steps in this order:
         *
         * 1. Write LMC_CTL with [DRESET] = 1, [PLL_BYPASS] = user_value, and
         * [PLL_DIV2] = user_value.
         */

        lmc_ctl.u64                 = cvmx_read_csr(CVMX_LMC_CTL);

        lmc_ctl.s.pll_bypass        = 0;
#if CONFIG_OCTEON_HIKARI
        lmc_ctl.s.pll_div2          = 1;
#else
        lmc_ctl.s.pll_div2          = 0;
#endif
        lmc_ctl.cn31xx.dreset            = 1;
        cvmx_write_csr(CVMX_LMC_CTL, lmc_ctl.u64);

        /* 2. Read L2D_BST0 and wait for the result. */
        cvmx_read_csr(CVMX_L2D_BST0); /* Read CVMX_L2D_BST0 */

        /* 3. Wait 10 us (LMC_CTL[PLL_BYPASS] and LMC_CTL[PLL_DIV2] must not
         * transition after this) */
        octeon_delay_cycles(6000);  /* Wait 10 us */

        /* 4. Write LMC_DDR2_CTL[QDLL_ENA] = 1. */
        ddr2_ctl.u64 = cvmx_read_csr(CVMX_LMC_DDR2_CTL);
        ddr2_ctl.s.qdll_ena   = 1;
        cvmx_write_csr(CVMX_LMC_DDR2_CTL, ddr2_ctl.u64);
        cvmx_read_csr(CVMX_LMC_DDR2_CTL);

        octeon_delay_cycles(2000);   // must be 200 dclocks

        /* 5. Read L2D_BST0 and wait for the result. */
        cvmx_read_csr(CVMX_L2D_BST0); /* Read CVMX_L2D_BST0 */

        /* 6. Wait 10 us (LMC_DDR2_CTL[QDLL_ENA] must not transition after this) */
        octeon_delay_cycles(6000);  /* Wait 10 us */

	/* 7. Write LMC_CTL[DRESET] = 0 (at this point, the DCLK is running and the
	 * memory controller is out of reset)
	 */
	lmc_ctl.cn31xx.dreset = 0;
	cvmx_write_csr(CVMX_LMC_CTL, lmc_ctl.u64);
	octeon_delay_cycles(2000);	// must be 200 dclocks
    }

    if (octeon_is_cpuid(OCTEON_CN58XX) || octeon_is_cpuid(OCTEON_CN50XX)) {
        cvmx_lmc_ctl_t lmc_ctl;
        cvmx_lmc_ddr2_ctl_t ddr2_ctl;
        cvmx_lmc_pll_ctl_t pll_ctl;
        cvmx_lmc_ctl1_t lmc_ctl1;

        if (ddr_ref_hertz == 0) {
            error_print("ERROR: DDR Reference Clock not specified.\n");
            return(-1);
        }

        /*
         * DCLK Initialization Sequence
         * 
         * When the reference-clock inputs to the LMC (DDR2_REF_CLK_P/N) are
         * stable, perform the following steps to initialize the DCLK.
         * 
         * 1. Write LMC_CTL[DRESET]=1, LMC_DDR2_CTL[QDLL_ENA]=0.
         */

        lmc_ctl.u64                 = cvmx_read_csr(CVMX_LMC_CTL);
        lmc_ctl.cn50xx.dreset       = 1;
        cvmx_write_csr(CVMX_LMC_CTL, lmc_ctl.u64);

        ddr2_ctl.u64                 = cvmx_read_csr(CVMX_LMC_DDR2_CTL);
        ddr2_ctl.s.qdll_ena     = 0;
        cvmx_write_csr(CVMX_LMC_DDR2_CTL, ddr2_ctl.u64);

        /*
         * 2. Write LMC_PLL_CTL[CLKR, CLKF, EN*] with the appropriate values,
         *    while writing LMC_PLL_CTL[RESET_N] = 0, LMC_PLL_CTL[DIV_RESET] = 1.
         *    LMC_PLL_CTL[CLKR, CLKF, EN*] values must not change after this
         *    point without restarting the DCLK initialization sequence.
         */

        /* CLKF = (DCLK * (CLKR+1) * EN(2, 4, 6, 8, 12, 16))/DREF - 1 */
        {
            int en_idx, save_en_idx, best_en_idx=0;
            uint64_t clkf, clkr, max_clkf = 127;
            uint64_t best_clkf=0, best_clkr=0;
            uint64_t best_pll_MHz = 0, pll_MHz, min_pll_MHz = 1200, max_pll_MHz = 2500;
            uint64_t error = ddr_hertz, best_error = ddr_hertz; /* Init to max error */;
            uint64_t lowest_clkf = max_clkf;
            uint64_t best_calculated_ddr_hertz = 0, calculated_ddr_hertz = 0;
            uint64_t orig_ddr_hertz = ddr_hertz;
            static int _en[] = {2, 4, 6, 8, 12, 16};

            pll_ctl.u64 = 0;

            while (best_error == ddr_hertz)
            {

                for (clkr = 0; clkr < 64; ++clkr) {
                    for (en_idx=sizeof(_en)/sizeof(int)-1; en_idx>=0; --en_idx) {
                        save_en_idx = en_idx;
                        clkf = ((ddr_hertz) * (clkr+1) * _en[save_en_idx]);
                        clkf = divide_nint(clkf, ddr_ref_hertz) - 1;
                        pll_MHz = ddr_ref_hertz * (clkf+1) / (clkr+1) / 1000000;
                        calculated_ddr_hertz = ddr_ref_hertz * (clkf + 1) / ((clkr + 1) * _en[save_en_idx]);
                        error = ddr_hertz - calculated_ddr_hertz;

                        if ((pll_MHz < min_pll_MHz) || (pll_MHz > max_pll_MHz)) continue;
                        if (clkf > max_clkf) continue; /* PLL requires clkf to be limited values less than 128 */
                        if (_abs(error) > _abs(best_error)) continue;

#if defined(__U_BOOT__)
                        ddr_print("clkr: %2lu, en: %2d, clkf: %4lu, pll_MHz: %4lu, ddr_hertz: %8lu, error: %8d\n",
                                  clkr, _en[save_en_idx], clkf, pll_MHz, calculated_ddr_hertz, error);
#else
                        ddr_print("clkr: %2llu, en: %2d, clkf: %4llu, pll_MHz: %4llu, ddr_hertz: %8llu, error: %8lld\n",
                                  clkr, _en[save_en_idx], clkf, pll_MHz, calculated_ddr_hertz, error);
#endif
    
                        if ((_abs(error) < _abs(best_error)) || (clkf < lowest_clkf)) {
                            lowest_clkf = clkf;
                            best_pll_MHz = pll_MHz;
                            best_calculated_ddr_hertz = calculated_ddr_hertz;
                            best_error = error;
                            best_clkr = clkr;
                            best_clkf = clkf;
                            best_en_idx = save_en_idx;
                        }
                    }
                }

#if defined(__U_BOOT__)
                ddr_print("clkr: %2lu, en: %2d, clkf: %4lu, pll_MHz: %4lu, ddr_hertz: %8lu, error: %8d <==\n",
                          best_clkr, _en[best_en_idx], best_clkf, best_pll_MHz, best_calculated_ddr_hertz, best_error);
#else
                ddr_print("clkr: %2llu, en: %2d, clkf: %4llu, pll_MHz: %4llu, ddr_hertz: %8llu, error: %8lld <==\n",
                          best_clkr, _en[best_en_idx], best_clkf, best_pll_MHz, best_calculated_ddr_hertz, best_error);
#endif

                /* Try lowering the frequency if we can't get a working configuration */
                if (best_error == ddr_hertz) {
                    if (ddr_hertz < orig_ddr_hertz - 10000000)
                        break;
                    ddr_hertz -= 1000000;
                    best_error = ddr_hertz;
                }

            }
            if (best_error == ddr_hertz) {
                error_print("ERROR: Can not compute a legal DDR clock speed configuration.\n");
                return(-1);
            }
            if (gd->board_desc.board_dram == DRAM_TYPE_SPD) {
                pll_ctl.cn50xx.en2  = (best_en_idx == 0);
                pll_ctl.cn50xx.en4  = (best_en_idx == 1);
                pll_ctl.cn50xx.en6  = (best_en_idx == 2);
                pll_ctl.cn50xx.en8  = (best_en_idx == 3);
                pll_ctl.cn50xx.en12 = (best_en_idx == 4);
                pll_ctl.cn50xx.en16 = (best_en_idx == 5);

                pll_ctl.cn50xx.clkf = best_clkf;
                pll_ctl.cn50xx.clkr = best_clkr;
                pll_ctl.cn50xx.reset_n = 0;
                pll_ctl.cn50xx.fasten_n = 1;
                pll_ctl.cn50xx.div_reset = 1;

                cvmx_write_csr(CVMX_LMC_PLL_CTL, pll_ctl.u64);
            } 
            else {
#ifdef CONFIG_JSRXNLE
                switch (gd->board_desc.board_type) {
                CASE_ALL_SRX100_MODELS 
                    cvmx_write_csr(CVMX_LMC_PLL_CTL, LMC_PLL_CTL_FREQ_533);
                    break;
                default:
                    cvmx_write_csr(CVMX_LMC_PLL_CTL, lmc_reg->lmc_pll_ctl);
                    break;

                }
#else
/* TODO */
#endif
            }
        }


        /*
         * 3. Read L2D_BST0 and wait for the result.
         */

        cvmx_read_csr(CVMX_L2D_BST0); /* Read CVMX_L2D_BST0 */

        /* 
         * 4. Wait 5 usec.
         */

        octeon_delay_cycles(10000);  /* Wait 10 us */


        /*
         * 5. Write LMC_PLL_CTL[RESET_N] = 1 while keeping LMC_PLL_CTL[DIV_RESET]
         *    = 1. LMC_PLL_CTL[RESET_N] must not change after this point without
         *    restarting the DCLK initialization sequence.
         */

        pll_ctl.u64 = cvmx_read_csr(CVMX_LMC_PLL_CTL);
        pll_ctl.cn50xx.reset_n = 1;
        cvmx_write_csr(CVMX_LMC_PLL_CTL, pll_ctl.u64);

        /*
         * 6. Read L2D_BST0 and wait for the result.
         */

        cvmx_read_csr(CVMX_L2D_BST0); /* Read CVMX_L2D_BST0 */

        /*
         * 7. Wait 500  (LMC_PLL_CTL[CLKR] + 1) reference-clock cycles.
         */

        octeon_delay_cycles(1000000); /* FIX: Wait 1 ms */

        /*
         * 8. Write LMC_PLL_CTL[DIV_RESET] = 0. LMC_PLL_CTL[DIV_RESET] must not
         *    change after this point without restarting the DCLK initialization
         *    sequence.
         */

        pll_ctl.u64 = cvmx_read_csr(CVMX_LMC_PLL_CTL);
        pll_ctl.cn50xx.div_reset = 0;
        cvmx_write_csr(CVMX_LMC_PLL_CTL, pll_ctl.u64);

        /*
         * 9. Read L2D_BST0 and wait for the result.
         * 
         * The DDR address clock frequency (DDR_CK_<5:0>_P/N) should be stable at
         * that point. Section 2.3.9 describes the DDR_CK frequencies resulting
         * from different reference-clock values and programmings.
         */

        cvmx_read_csr(CVMX_L2D_BST0); /* Read CVMX_L2D_BST0 */


        /*
         * DRESET Initialization Sequence
         * 
         * The DRESET initialization sequence cannot start unless DCLK is stable
         * due to a prior DCLK initialization sequence. Perform the following
         * steps to initialize DRESET.
         * 
         * 1. Write LMC_CTL[DRESET] = 1 and LMC_DDR2_CTL[QDLL_ENA] = 0.
         */
 
        lmc_ctl.u64                 = cvmx_read_csr(CVMX_LMC_CTL);
        lmc_ctl.cn50xx.dreset       = 1;
        cvmx_write_csr(CVMX_LMC_CTL, lmc_ctl.u64);

        ddr2_ctl.u64                 = cvmx_read_csr(CVMX_LMC_DDR2_CTL);
        ddr2_ctl.s.qdll_ena     = 0;
        cvmx_write_csr(CVMX_LMC_DDR2_CTL, ddr2_ctl.u64);

        /*
         * 2. Write LMC_DDR2_CTL[QDLL_ENA] = 1. LMC_DDR2_CTL[QDLL_ENA] must not
         *    change after this point without restarting the LMC and/or DRESET
         *    initialization sequence.
         */
 
        ddr2_ctl.u64 = cvmx_read_csr(CVMX_LMC_DDR2_CTL);
        ddr2_ctl.s.qdll_ena   = 1;
        cvmx_write_csr(CVMX_LMC_DDR2_CTL, ddr2_ctl.u64);
        cvmx_read_csr(CVMX_LMC_DDR2_CTL);

        /*
         * 3. Read L2D_BST0 and wait for the result.
         */

        cvmx_read_csr(CVMX_L2D_BST0); /* Read CVMX_L2D_BST0 */
 
        /*
         * 4. Wait 10 usec.
         */

        octeon_delay_cycles(10000);  /* Wait 10 us */

 
        /*
         * 5. Write LMC_CTL[DRESET] = 0. LMC_CTL[DRESET] must not change after
         *    this point without restarting the DRAM-controller and/or DRESET
         *    initialization sequence.
         */

        lmc_ctl.u64                 = cvmx_read_csr(CVMX_LMC_CTL);
        lmc_ctl.cn50xx.dreset       = 0;
        cvmx_write_csr(CVMX_LMC_CTL, lmc_ctl.u64);


        /*
         * 6. Read L2D_BST0 and wait for the result.
         */

        cvmx_read_csr(CVMX_L2D_BST0); /* Read CVMX_L2D_BST0 */

 

        /* 
         * LMC Initialization Sequence
         * 
         * The LMC initialization sequence must be preceded by a DCLK and DRESET
         * initialization sequence.
         * 
         * 1. Software must ensure there are no pending DRAM transactions.
         */

        /*
         * 2. Write LMC_CTL, LMC_CTL1, LMC_MEM_CFG1, LMC_DDR2_CTL,
         * LMC_RODT_CTL, LMC_DUAL_MEMCFG, and LMC_WODT_CTL with appropriate
         * values, if necessary. Refer to Sections 2.3.4, 2.3.5, and 2.3.7 regarding
         * these registers (and LMC_MEM_CFG0).
         */
        if (gd->board_desc.board_dram == DRAM_TYPE_SPD) {
      	    lmc_ctl1.u64 = cvmx_read_csr(CVMX_LMC_CTL1);
       	    lmc_ctl1.cn50xx.sil_mode = 1;

            if (ddr_verbose() && (s = getenv("ddr_sil_mode")) != NULL) {
                lmc_ctl1.cn50xx.sil_mode = simple_strtoul(s, NULL, 0);
                error_print("Parameter found in environment. ddr_sil_mode = %d\n", lmc_ctl1.cn50xx.sil_mode);
            }

        cvmx_write_csr(CVMX_LMC_CTL1, lmc_ctl1.u64);
        } 
#ifdef CONFIG_JSRXNLE
        else {
            cvmx_write_csr(CVMX_LMC_CTL1, lmc_reg->lmc_ctl1);
        }
#endif
        /*
         * 3. Write LMC_MEM_CFG0 with appropriate values and
         * LMC_MEM_CFG0[INIT_START] = 0.
         */

        /*
         * 4. Write LMC_MEM_CFG0 with appropriate values and
         *    LMC_MEM_CFG0[INIT_START] = 1. At that point, CN58XX hardware
         *    initiates the standard DDR2 init sequence shown in Figure 29.
         * 
         *    CN58XX activates DDR_CKE (if it has not already been
         *    activated). DDR_CKE remains activated from that point until a
         *    subsequent DRESET.
         * 
         *    CN58XX then follows with the standard DDR2 initialization sequence,
         *    not using OCD. While CN58XX performs the initialization sequence,
         *    it cannot perform other DDR2 transactions.
         * 
         *    Note that if there is not a DRESET between two LMC initialization
         *    sequences, DDR_CKE remains asserted through the second
         *    initialization sequence. The hardware initiates the same DDR2
         *    initialization sequence as the first, except that DDR_CKE does not
         *    deactivate. If DDR_CKE deactivation and reactivation is desired for
         *    a second controller reset, a DRESET sequence is required.
         */

        /*
         * 5. Read L2D_BST0 and wait for the result.
         * 
         * After this point, the LMC is fully functional.
         * LMC_MEM_CFG0[INIT_START] must not transition from 0.1 during normal
         * operation.
         */ 

    }


    if ((octeon_is_cpuid(OCTEON_CN56XX)) || (octeon_is_cpuid(OCTEON_CN52XX))) {
        cvmx_l2c_cfg_t l2c_cfg;
        cvmx_lmc_pll_ctl_t pll_ctl;
        cvmx_lmcx_dll_ctl_t lmc_dll_ctl;
        cvmx_lmc_bist_ctl_t lmc_bist_ctl;
        cvmx_lmc_bist_result_t lmc_bist_result;
        cvmx_lmc_ctl1_t lmc_ctl1;
        cvmx_lmc_dclk_ctl_t lmc_dclk_ctl;
        int ddr_interface_enabled = 0;

        ddr_ref_hertz = 50000000;

        /*
         * 2.3.8 DRAM Controller Initialization
         * 
         *       There are five parts to the LMC-initialization procedure:
         * 
         *       1. Global DCLK initialization
         *       2. LMC0 DRESET initialization
         *       3. LMC1 DCLK offsetting
         *       4. LMC1 DRESET initialization
         *       5. LMC0/LMC1 initialization
         * 
         *       During a cold reset, all five initializations should be executed
         *       in sequence. The global DCLK and LMC1 DCLK initialization
         *       (i.e. steps 1 and 3) need only be performed once if the DCLK
         *       speed and parameters do not change. Subsequently, it is possible
         *       to execute only DRESET and LMC initializations, or to execute
         *       only the LMC0/LMC1 initialization.
         * 
         * 
         * 2.3.8.1 Global DCLK Initialization Sequence
         * 
         *       Perform the following steps to initialize the DCLK.
         * 
         *       1.  First, L2C_CFG[DPRES0,DPRES1] must be set to enable the LMC
         *           controllers that have DRAM attached. L2C_CFG[DPRES0,DPRES1]
         *           must not change after this point without restarting the
         *           global DCLK initialization sequence. Refer to Section 2.2.5.
         */

	if (octeon_is_cpuid(OCTEON_CN56XX)) {
	    l2c_cfg.u64 = cvmx_read_csr(CVMX_L2C_CFG);

	    if ((ddr_interface_num == 0) && (ddr_interface_mask & 1)) {
		l2c_cfg.cn56xx.dpres0 = 1; /* Enable Interface */
		ddr_interface_enabled = 1;
	    }

	    if ((ddr_interface_num == 1) && (ddr_interface_mask & 2)) {
		l2c_cfg.cn56xx.dpres1 = 1; /* Enable Interface */
		ddr_interface_enabled = 1;
	    }

	    cvmx_write_csr(CVMX_L2C_CFG, l2c_cfg.u64);
	} else if (octeon_is_cpuid(OCTEON_CN52XX)) {
            /* Unlike the bits for the CN56XX, this bit does not do anything.
            ** We set it here to indicate to the host the the LMC is setup up, and this
            ** allows the same host code to work for the CN52XX CN56XX */
	    l2c_cfg.u64 = cvmx_read_csr(CVMX_L2C_CFG);
            l2c_cfg.cn56xx.dpres0 = 1; /* Use spare bit to indiate interface is enabled */
	    cvmx_write_csr(CVMX_L2C_CFG, l2c_cfg.u64);
            ddr_interface_enabled = 1;
        } else {
	    ddr_interface_enabled = 1; /* Interface is always enabled */
	}

        ddr_print("DDR Interface %d: %s,  Mask 0x%x\n",
                  ddr_interface_num,
                  ddr_interface_enabled ? "Enabled" : "Disabled",
                  ddr_interface_mask);

        /*       2. If not done already, set LMC0/1_DLL_CTL=0, except that LMC0/
         *          1_DLL_CTL[DRESET]=1, for each LMC controller that is enabled.
         */


        lmc_dll_ctl.u64 = 0;
        lmc_dll_ctl.cn56xx.dreset = 1;

        cvmx_write_csr(CVMX_LMCX_DLL_CTL(ddr_interface_num), lmc_dll_ctl.u64);
        

        /*       3. Write LMC1_DCLK_CTL=0. */

	if (octeon_is_cpuid(OCTEON_CN56XX)) {
            cvmx_write_csr(CVMX_LMCX_DCLK_CTL(1), 0);
        }


        /*       Note that the remaining global DCLK initialization steps
         *       reference only LMC0 CSRs, not LMC1. This is because there is
         *       only one global DCLK PLL, and it is attached only to LMC0.
         */

        /* Since there is only one global DCLK PLL this only needs to
         * be performed once.  Therefore, if LMC0_PLL_CTL[RESET_N] = 1
         * then this sequence has already been performed. */

        pll_ctl.u64 = cvmx_read_csr(CVMX_LMCX_PLL_CTL(0));
        if (pll_ctl.cn56xx.reset_n == 0) {
            ddr_print("DDR Interface %d: %s, Target DCLK: %d\n", ddr_interface_num,
                      "Enabling DCLK PLL", ddr_hertz);
            /*       4.  Write LMC0_PLL_CTL[CLKF,CLKR,EN*] with the appropriate values, while
             *           writing LMC0_PLL_CTL[RESET_N] = 0, LMC0_PLL_CTL[DIV_RESET] = 1.
             *           LMC0_PLL_CTL[CLKF,CLKR,EN*] values must not change after
             *           this point without restarting the DCLK initialization
             *           sequence.
             */

            /* CLKF = (DCLK * (CLKR+1) * EN(2, 4, 6, 8, 12, 16))/DREF - 1 */
            int en_idx, save_en_idx, best_en_idx=0;
            uint64_t clkf, clkr, max_clkf = 127;
            uint64_t best_clkf=0, best_clkr=0;
            uint64_t best_pll_MHz = 0, pll_MHz, min_pll_MHz = 1200, max_pll_MHz = 2500;
            uint64_t error = ddr_hertz, best_error = ddr_hertz; /* Init to max error */;
            uint64_t lowest_clkf = max_clkf;
            uint64_t best_calculated_ddr_hertz = 0, calculated_ddr_hertz = 0;
            uint64_t orig_ddr_hertz = ddr_hertz;
            static int _en[] = {2, 4, 6, 8, 12, 16};

            pll_ctl.u64 = 0;

            while (best_error == ddr_hertz)
            {

                for (clkr = 0; clkr < 64; ++clkr) {
                    for (en_idx=sizeof(_en)/sizeof(int)-1; en_idx>=0; --en_idx) {
                        save_en_idx = en_idx;
                        clkf = ((ddr_hertz) * (clkr+1) * _en[save_en_idx]);
                        clkf = divide_nint(clkf, ddr_ref_hertz) - 1;
                        pll_MHz = ddr_ref_hertz * (clkf+1) / (clkr+1) / 1000000;
                        calculated_ddr_hertz = ddr_ref_hertz * (clkf + 1) / ((clkr + 1) * _en[save_en_idx]);
                        error = ddr_hertz - calculated_ddr_hertz;

                        if ((pll_MHz < min_pll_MHz) || (pll_MHz > max_pll_MHz)) continue;
                        if (clkf > max_clkf) continue; /* PLL requires clkf to be limited values less than 128 */
                        if (_abs(error) > _abs(best_error)) continue;

#if defined(__U_BOOT__)
                        ddr_print("clkr: %2lu, en: %2d, clkf: %4lu, pll_MHz: %4lu, ddr_hertz: %8lu, error: %8d\n",
                                  clkr, _en[save_en_idx], clkf, pll_MHz, calculated_ddr_hertz, error);
#else
                        ddr_print("clkr: %2llu, en: %2d, clkf: %4llu, pll_MHz: %4llu, ddr_hertz: %8llu, error: %8lld\n",
                                  clkr, _en[save_en_idx], clkf, pll_MHz, calculated_ddr_hertz, error);
#endif
    
                        if ((_abs(error) < _abs(best_error)) || (clkf < lowest_clkf)) {
                            lowest_clkf = clkf;
                            best_pll_MHz = pll_MHz;
                            best_calculated_ddr_hertz = calculated_ddr_hertz;
                            best_error = error;
                            best_clkr = clkr;
                            best_clkf = clkf;
                            best_en_idx = save_en_idx;
                        }
                    }
                }

#if defined(__U_BOOT__)
                ddr_print("clkr: %2lu, en: %2d, clkf: %4lu, pll_MHz: %4lu, ddr_hertz: %8lu, error: %8d <==\n",
                          best_clkr, _en[best_en_idx], best_clkf, best_pll_MHz, best_calculated_ddr_hertz, best_error);
#else
                ddr_print("clkr: %2llu, en: %2d, clkf: %4llu, pll_MHz: %4llu, ddr_hertz: %8llu, error: %8lld <==\n",
                          best_clkr, _en[best_en_idx], best_clkf, best_pll_MHz, best_calculated_ddr_hertz, best_error);
#endif
    
                /* Try lowering the frequency if we can't get a working configuration */
                if (best_error == ddr_hertz) {
                    if (ddr_hertz < orig_ddr_hertz - 10000000)
                        break;
                    ddr_hertz -= 1000000;
                    best_error = ddr_hertz;
                }

            }


            if (best_error == ddr_hertz) {
                error_print("ERROR: Can not compute a legal DDR clock speed configuration.\n");
                return(-1);
            }

            pll_ctl.cn56xx.en2  = (best_en_idx == 0);
            pll_ctl.cn56xx.en4  = (best_en_idx == 1);
            pll_ctl.cn56xx.en6  = (best_en_idx == 2);
            pll_ctl.cn56xx.en8  = (best_en_idx == 3);
            pll_ctl.cn56xx.en12 = (best_en_idx == 4);
            pll_ctl.cn56xx.en16 = (best_en_idx == 5);

            pll_ctl.cn56xx.clkf = best_clkf;
            pll_ctl.cn56xx.clkr = best_clkr;
            pll_ctl.cn56xx.reset_n = 0;
            pll_ctl.cn56xx.fasten_n = 1;
            pll_ctl.cn56xx.div_reset = 1;
            pll_ctl.cn56xx.fasten_n = 1; /* Must always be set due to G-800 */

            cvmx_write_csr(CVMX_LMCX_PLL_CTL(0), pll_ctl.u64);


            /*       5.  Read L2D_BST0 and wait for the result. */
            cvmx_read_csr(CVMX_L2D_BST0); /* Read CVMX_L2D_BST0 */

            /*       6.  Wait 5 Êsec. */

            octeon_delay_cycles(2000);  /* Wait 10 us */


            /*       7.  Write LMC0_PLL_CTL[RESET_N] = 1 while keeping
             *           LMC0_PLL_CTL[DIV_RESET] = 1. LMC0_PLL_CTL[RESET_N] must not
             *           change after this point without restarting the DCLK
             *           initialization sequence.
             */

            pll_ctl.u64 = cvmx_read_csr(CVMX_LMCX_PLL_CTL(0));
            pll_ctl.cn56xx.reset_n = 1;
            cvmx_write_csr(CVMX_LMCX_PLL_CTL(0), pll_ctl.u64);

            /*       8.  Read L2D_BST0 and wait for the result. */
            cvmx_read_csr(CVMX_L2D_BST0); /* Read CVMX_L2D_BST0 */


            /*       9.  Wait 500 ~ (LMC0_PLL_CTL[CLKR] + 1) reference-clock cycles. */

            octeon_delay_cycles(1000000); /* FIX: Wait 1 ms */


            /*       10. Write LMC0_PLL_CTL[DIV_RESET] = 0. LMC0_PLL_CTL[DIV_RESET]
             *           must not change after this point without restarting the DCLK
             *           initialization sequence.
             */

            pll_ctl.u64 = cvmx_read_csr(CVMX_LMCX_PLL_CTL(0));
            pll_ctl.cn56xx.div_reset = 0;
            cvmx_write_csr(CVMX_LMCX_PLL_CTL(0), pll_ctl.u64);

            /*       11. Read L2D_BST0 and wait for the result.
             * 
             *       The DDR address clock frequency (DDR_CK_<5:0>_P/N) should be
             *       stable at that point. Section 2.3.9 describes the DDR_CK
             *       frequencies resulting from different reference-clock values and
             *       programmings.
             */

            cvmx_read_csr(CVMX_L2D_BST0); /* Read CVMX_L2D_BST0 */
        }


        /* 
         * Only perform the LMC1 DCLK Offsetting Sequence if both
         * interfaces are enabled.  This can only happen on the second
         * pass of this procedure.
         */
        if (octeon_is_cpuid(OCTEON_CN56XX) && (l2c_cfg.cn56xx.dpres0 && l2c_cfg.cn56xx.dpres1)) {
            lmc_dclk_ctl.u64 = cvmx_read_csr(CVMX_LMCX_DCLK_CTL(1));
            if (lmc_dclk_ctl.cn56xx.off90_ena == 0) {
                ddr_print("DDR Interface %d: %s\n", ddr_interface_num,
                          "Performing LMC1 DCLK Offsetting Sequence");
                /* 2.3.8.3 LMC1 DCLK Offsetting Sequence
                 * 
                 *       The LMC1 DCLK Offsetting sequence offsets the LMC1 DCLK relative
                 *       to the LMC0 DCLK (by approximately 90 degrees). Note that DCLK
                 *       offsetting only applies to LMC1, so the sequence only references
                 *       LMC1 CSRs.
                 * 
                 *       The LMC1 DCLK offsetting sequence should be skipped if either
                 *       LMC0 or LMC1 are not being used (i.e. if either
                 *       L2C_CFG[DPRES0]=0 or L2C_CFG[DPRES1]=0).
                 * 
                 *       1.  Write LMC1_DCLK_CTL[DCLK90_LD]=1 to cause the LMC1
                 *           offsetting DLL to sample the delay setting from
                 *           LMC0. (LMC1_DCLK_CTL should be zero prior to this.)
                 */

                lmc_dclk_ctl.u64 = cvmx_read_csr(CVMX_LMCX_DCLK_CTL(1));
                lmc_dclk_ctl.cn56xx.dclk90_ld = 1;
                cvmx_write_csr(CVMX_LMCX_DCLK_CTL(1), lmc_dclk_ctl.u64);

                /*       2.  Write LMC1_DCLK_CTL[DCLK90_LD]=0 and
                 *           LMC1_DCLK_CTL[OFF90_ENA]=1 to enable the offsetting DLL.
                 *           LMC1_DCLK_CTL must not change after this point without
                 *           restarting the LMC1 DCLK offsetting initialization sequence
                 *           and redoing LMC1 DRESET sequence.
                 */

                lmc_dclk_ctl.u64 = cvmx_read_csr(CVMX_LMCX_DCLK_CTL(1));
                lmc_dclk_ctl.cn56xx.dclk90_ld = 0;
                lmc_dclk_ctl.cn56xx.off90_ena = 1;
                cvmx_write_csr(CVMX_LMCX_DCLK_CTL(1), lmc_dclk_ctl.u64);

                /*       3.  Read L2D_BST0 and wait for the result. */

                cvmx_read_csr(CVMX_L2D_BST0); /* Read CVMX_L2D_BST0 */
            }
        }


        /* 
         *  This section handles the DRESET Initialization Sequence
         *  for both interfaces.  The comments taken from the HRM
         *  describe the sequence for interface 0.  Therefore, this
         *  same section is used on a subsequent call to perform the
         *  sequence described in section "2.3.8.4 LMC1 DRESET
         *  Initialization Sequence".
         */

        if (ddr_interface_enabled) {
            ddr_print("DDR Interface %d: %s\n", ddr_interface_num,
                      "Performing DRESET Initialization Sequence");
            /* 
             * 
             * 2.3.8.2 LMC0 DRESET Initialization Sequence
             * 
             *       The LMC0 DRESET initialization should be skipped if LMC0 is not
             *       being used (i.e. if L2C_CFG[DPRES0]=0).
             * 
             *       The LMC0 DRESET initialization sequence cannot start unless DCLK
             *       is stable due to a prior DCLK initialization sequence. Perform
             *       the following steps to initialize LMC0 DRESET.
             * 
             * 2.3.8.3 LMC1 DCLK Offsetting Sequence (See above)
             *
             * 2.3.8.4 LMC1 DRESET Initialization Sequence
             * 
             *       The LMC1 DRESET initialization should be skipped if LMC1 is not being used (i.e. if
             *       L2C_CFG[DPRES1]=0).
             * 
             *       The LMC1 DRESET initialization sequence cannot start unless
             *       LMC0's DCLK is stable due to a prior global DCLK initialization
             *       sequence and, if necessary, a prior LMC1 DCLK
             *       offsetting. Perform the following steps to initialize DRESET.
             * 
             *       The LMC1 DRESET sequence steps are identical to the LMC0 DRESET
             *       steps described in Section 2.3.8.2 above, except that LMC1 CSRs
             *       are used rather than LMC0.
             */


            /*       1.  If not done already, set LMC0_DLL_CTL=0, except that
             *           LMC0_DLL_CTL[DRESET]=1.
             */
#if 1
            /* Already done during step 2 of 2.3.8.1 Global DCLK Initialization Sequence.
               Why is it prescribed again? */
            lmc_dll_ctl.u64 = 0;
            lmc_dll_ctl.cn56xx.dreset = 1;
            cvmx_write_csr(CVMX_LMCX_DLL_CTL(ddr_interface_num), lmc_dll_ctl.u64);
#endif


            /*       2.  Write LMC0_DLL_CTL[DLL90_ENA] = 1.
             *           LMC0_DLL_CTL[DLL90_ENA,DLL90_BYP] must not change after this
             *           point without restarting the LMC and/or DRESET
             *           initialization sequence.
             */

            lmc_dll_ctl.u64 = cvmx_read_csr(CVMX_LMCX_DLL_CTL(ddr_interface_num));
            lmc_dll_ctl.cn56xx.dll90_ena = 1;
            cvmx_write_csr(CVMX_LMCX_DLL_CTL(ddr_interface_num), lmc_dll_ctl.u64);


            /*       3.  Read L2D_BST0 and wait for the result. */

            cvmx_read_csr(CVMX_L2D_BST0); /* Read CVMX_L2D_BST0 */

            /*       4.  Wait 10 Êsec. */

            octeon_delay_cycles(2000);  /* Wait 10 us */


            /*       5.  Write LMC0_BIST_CTL[START]=1 and back to zero to start BIST
             *           of the DCLK memories in LMC. (LMC0_BIST_CTL[START] should
             *           have previously been zero.)
             */

            lmc_bist_ctl.u64 = cvmx_read_csr(CVMX_LMCX_BIST_CTL(ddr_interface_num));
            lmc_bist_ctl.cn56xx.start = 0; /* Incase it wasn't already 0 */
            cvmx_write_csr(CVMX_LMCX_BIST_CTL(ddr_interface_num), lmc_bist_ctl.u64);
            lmc_bist_ctl.cn56xx.start = 1; /* Start BIST */
            cvmx_write_csr(CVMX_LMCX_BIST_CTL(ddr_interface_num), lmc_bist_ctl.u64);
            lmc_bist_ctl.cn56xx.start = 0; /* Leave it zeroed */
            cvmx_write_csr(CVMX_LMCX_BIST_CTL(ddr_interface_num), lmc_bist_ctl.u64);


            /*       6.  Read L2D_BST0 and wait for the result. */

            cvmx_read_csr(CVMX_L2D_BST0); /* Read CVMX_L2D_BST0 */

            /*       7.  Wait 3000 core-clock cycles + 3000 memory-clock cycles for
             *           BIST to complete. (25 Ês should cover the longest possible
             *           case.)
             */

            octeon_delay_cycles(1000000); /* FIX: Wait 1 ms */


            /*       8.  Read LMC0_BIST_RESULT. If LMC0_BIST_RESULT is not zero, it
             *           is a fatal error.
             */

            lmc_bist_result.u64 = cvmx_read_csr(CVMX_LMCX_BIST_RESULT(ddr_interface_num));
            if (lmc_bist_result.u64 != 0) {
                error_print("BIST ERROR: 0x%llx\n", (unsigned long long)lmc_bist_result.u64);
                error_print("CSRD2E = 0x%x\n", lmc_bist_result.cn56xx.csrd2e);
                error_print("CSRE2D = 0x%x\n", lmc_bist_result.cn56xx.csre2d);
                error_print("MWF    = 0x%x\n", lmc_bist_result.cn56xx.mwf);
                error_print("MWD    = 0x%x\n", lmc_bist_result.cn56xx.mwd);  
                error_print("MWC    = 0x%x\n", lmc_bist_result.cn56xx.mwc);  
                error_print("MRF    = 0x%x\n", lmc_bist_result.cn56xx.mrf);  
                error_print("MRD    = 0x%x\n", lmc_bist_result.cn56xx.mrd);  
                return(-1);
            }

            /*       9.  Write LMC0_DLL_CTL[DRESET] = 0. LMC0_DLL_CTL[DRESET] must
             *           not change after this point without restarting the
             *           DRAM-controller and/or DRESET initialization sequence.
             */

            lmc_dll_ctl.u64 = cvmx_read_csr(CVMX_LMCX_DLL_CTL(ddr_interface_num));
            lmc_dll_ctl.cn56xx.dreset = 0;
            cvmx_write_csr(CVMX_LMCX_DLL_CTL(ddr_interface_num), lmc_dll_ctl.u64);

            /*       10. Read L2D_BST0 and wait for the result. */

            cvmx_read_csr(CVMX_L2D_BST0); /* Read CVMX_L2D_BST0 */
        }


        /* 
         * 2.3.8.5 LMC0/LMC1 Initialization Sequence
         * 
         *       The LMC initialization sequence must be preceded by a DCLK and DRESET
         *       initialization sequence.
         * 
         *       Perform these steps for all enabled LMCs (i.e. perform these
         *       steps for LMC0 if L2C_CFG[DPRES0]=1, and separately perform
         *       these steps for LMC1 if L2C_CFG[DPRES0]=1).
         */

        if (ddr_interface_enabled) {
            ddr_print("DDR Interface %d: %s\n", ddr_interface_num,
                      "Performing LMCX Initialization Sequence");
            /* 
             *       1.  Software must ensure there are no pending DRAM transactions.
             *
             *       2.  Write LMC0/1_CTL, LMC0/1_CTL1, LMC0/1_MEM_CFG1, LMC0/
             *           1_DDR2_CTL, LMC0/1_RODT_COMP_CTL, LMC0/1_DUAL_MEMCFG, LMC0/
             *           1_WODT_CTL, LMC0/1_READ_LEVEL_RANKn, and LMC0/
             *           1_READ_LEVEL_CTL with appropriate values, if
             *           necessary. Refer to Sections 2.3.4, 2.3.5, and 2.3.7
             *           regarding these registers (and LMC0/1_MEM_CFG0).
             */

            lmc_ctl1.u64 = cvmx_read_csr(CVMX_LMCX_CTL1(ddr_interface_num));

            lmc_ctl1.cn56xx.sil_mode = 1;
            lmc_ctl1.cn56xx.ecc_adr = 1; /* Include the address in ECC to pickup incorrect line loads */
            lmc_ctl1.cn56xx.sequence = 0; /* Required at startup */

            if (ddr_verbose() && (s = getenv("ddr_sil_mode")) != NULL) {
                lmc_ctl1.cn56xx.sil_mode = simple_strtoul(s, NULL, 0);
                error_print("Parameter found in environment. ddr_sil_mode = %d\n", lmc_ctl1.cn56xx.sil_mode);
            }

            cvmx_write_csr(CVMX_LMCX_CTL1(ddr_interface_num), lmc_ctl1.u64);


            /*       3.  Write LMC0/1_MEM_CFG0 with appropriate values and
             *           LMC_MEM_CFG0[INIT_START] = 0.
             */


            /*       4.  If not done already, set LMC0/1_CTL1[SEQUENCE] to the
             *           desired initialization sequence.  This will normally be
             *           LMC0/1_CTL1[SEQUENCE]=0, selecting a power-up/init.
             */

            /*       5.  Write LMC0/1_MEM_CFG0 with appropriate values and
             *           LMC0/1_MEM_CFG0[INIT_START] = 1. At that point, CN56/57XX
             *           hardware initiates the initialization sequence selected by
             *           LMC0/1_CTL1[SEQUENCE].
             * 
             *           Figure 2]12 shows the power-up/init sequence. CN56/57XX
             *           activates DDR_CKE as part of the power-up/init sequence (if
             *           it has not already been activated).  DDR_CKE remains
             *           activated from that point until a subsequent DRESET, except
             *           during power-down (refer to Section 2.3.11 below) or
             *           self-refresh entry.
             * 
             *           After DDR_CKE asserts to start the power-up/init sequence,
             *           CN56/57XX then follows with the standard DDR2 initialization
             *           sequence, not using OCD. While CN56/57XX performs this
             *           initialization sequence, it cannot perform other DDR2
             *           transactions.
             * 
             *           Note that if there is not a DRESET between two LMC
             *           power-up/init sequences, DDR_CKE remains asserted through
             *           the second sequence. The hardware initiates the same DDR2
             *           initialization sequence as the first, except that DDR_CKE
             *           does not deactivate. If DDR_CKE deactivation and
             *           reactivation is desired for a second controller reset, a
             *           DRESET sequence is required.
             * 
             *       6.  Read L2D_BST0 and wait for the result.
             * 
             *       After this point, the LMC is fully
             *       functional. LMC0/1_MEM_CFG0[INIT_START] should not transition
             *       from 0¨1 during normal operation.
             */
        } else {
            /* If this interface is not enabled don't try start it. */
            return(-1);
        }
    }

    if (octeon_is_cpuid(OCTEON_CN63XX)) {
        /* Initialization Sequence
         *
         * There are seven parts to the LMC initialization procedure:
         *
         * 1. DDR PLL initialization
         *
         * 2. LMC CK initialization
         *
         * 3. LMC DRESET initialization
         *
         * 4. LMC RESET initialization
         *
         * 5. Early LMC initialization
         *
         * 6. LMC write leveling
         *
         * 7. LMC read leveling
         *
         * 8. Final LMC initialization
         *
         * During a cold reset, all seven initializations should be
         * executed in sequence. The DDR PLL and LMC CK
         * initializations need only be performed once if the
         * SDRAMclock speed and parameters do not
         * change. Subsequently, it is possible to execute only DRESET
         * and subsequent steps, or to execute only the final 3 steps.
         *
         * The remainder of this section covers these parts in
         * sequence.
         */

        cvmx_lmcx_ddr_pll_ctl_t ddr_pll_ctl;
        cvmx_lmcx_dll_ctl2_t dll_ctl2;
        cvmx_lmcx_reset_ctl_t lmc_reset_ctl;
        cvmx_dfm_dll_ctl2_t dfm_dll_ctl2;

        /*
         * 4.8.1 DDR PLL Initialization
         *
         * The same PLL is used by both LMC and the HFA memory
         * controller (HMC), so its initialization affects
         * both. Chapter 11 describes more about HMC.
         *
         * Perform the following steps to initialize the DDR PLL:
         *
         * 1. If not done already, write all fields in
         *    LMC0_DDR_PLL_CTL, LMC0_DLL_CTL2 to their reset value,
         *    including LMC0_DDR_PLL_CTL[DDR_DIV_RESET, DFM_DIV_RESET]
         *    = 1 and LMC0_DLL_CTL2[DRESET] = 1.
         */

        ddr_pll_ctl.u64 = 0;
        ddr_pll_ctl.cn63xx.dfm_div_reset = 1;
        ddr_pll_ctl.cn63xx.dfm_ps_en = 2;
        ddr_pll_ctl.cn63xx.ddr_div_reset = 1;
        ddr_pll_ctl.cn63xx.ddr_ps_en = 2;
        ddr_pll_ctl.cn63xx.clkf = 0x30;
        cvmx_write_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num), ddr_pll_ctl.u64);

        dll_ctl2.u64 = 0;
        dll_ctl2.cn63xx.dreset = 1;
        cvmx_write_csr(CVMX_LMCX_DLL_CTL2(ddr_interface_num), dll_ctl2.u64);

        /*
         *    If the HMC is being used, also write all fields in
         *    DFM_DLL_CTL2 to their reset value, including
         *    DFM_DLL_CTL2[DRESET] = 1.
         */

        dfm_dll_ctl2.u64 = 0;
        dfm_dll_ctl2.cn63xx.dreset = 1;
        cvmx_write_csr(CVMX_DFM_DLL_CTL2, dfm_dll_ctl2.u64);

        /*
         *    The above actions of this step are not necessary after a
         *    chip cold reset / power up.
         *
         *    If the current DRAM contents are not preserved (See
         *    LMC0_RESET_CTL [DDR3PSV]), this is also an appropriate
         *    time to assert the RESET# pin of the DDR3 DRAM parts. If
         *    desired, write LMC0_RESET_CTL[DDR3RST]=0 without
         *    modifying any other LMC0_RESET_CTL fields to assert the
         *    DDR_RESET_L pin.  No action is required here to assert
         *    DDR_RESET_L following a chip cold reset/ power up. Refer
         *    to Section 4.8.4.
         */

        {
            cvmx_lmcx_reset_ctl_t reset_ctl;
            reset_ctl.u64 = cvmx_read_csr(CVMX_LMCX_RESET_CTL(ddr_interface_num));
            if (reset_ctl.cn63xx.ddr3psv == 0) {
                reset_ctl.cn63xx.ddr3rst = 0; /* Reset asserted */
                cvmx_write_csr(CVMX_LMCX_RESET_CTL(ddr_interface_num), reset_ctl.u64);
                cvmx_read_csr(CVMX_LMCX_RESET_CTL(ddr_interface_num));
                cvmx_wait_usec(400);          /* Wait 400 us */
                reset_ctl.cn63xx.ddr3rst = 1; /* Reset de-asserted */
                cvmx_write_csr(CVMX_LMCX_RESET_CTL(ddr_interface_num), reset_ctl.u64);
            }
        }

        /*
         * 2. Without changing any other LMC0_DDR_PLL_CTL values,
         *    write LMC0_DDR_PLL_CTL[CLKF] with a value that gives a
         *    desired DDR PLL speed.  The LMC0_DDR_PLL_CTL[CLKF] value
         *    should be selected in conjunction with both post-scalar
         *    divider values for LMC (LMC0_DDR_PLL_CTL[DDR_PS_EN]) and
         *    HMC (LMC0_DDR_PLL_CTL[DFM_PS_EN]) so that the desired
         *    LMC and HMC CK speeds are produced. Section 4.13
         *    describes LMC0_DDR_PLL_CTL[CLKF] and
         *    LMC0_DDR_PLL_CTL[DDR_PS_EN] programmings that produce
         *    the desired LMC CK speed. Section 4.8.2 describes LMC CK
         *    initialization, which can be done separately from the
         *    DDR PLL initialization described in this section.
         *
         *    The LMC0_DDR_PLL_CTL[CLKF] value must not change after
         *    this point without restarting this SDRAM PLL
         *    initialization sequence.
         */

        /* CLKF = (LMC CLK SPEED(MHz) * DDR_PS_EN(1, 2, 3, 4, 6, 8, 12)) / 50 (Ref CLK MHz) */
        {
            unsigned en_idx, save_en_idx, best_en_idx=0;
            uint64_t clkf, min_clkf = 32, max_clkf = 64;
            uint64_t best_clkf=0;
            uint64_t best_pll_MHz = 0, pll_MHz, highest_pll_MHz = 0;
            uint64_t min_pll_MHz = 1600, max_pll_MHz = 3000;
            int64_t  error = ddr_hertz, best_error = ddr_hertz; /* Init to max error */
            uint64_t best_calculated_ddr_hertz = 0, calculated_ddr_hertz = 0;
            uint64_t orig_ddr_hertz = ddr_hertz;
            static const unsigned _en[] = {1, 2, 3, 4, 6, 8, 12};

            ddr_pll_ctl.u64 = cvmx_read_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num));

            while (best_error == ddr_hertz)
            {
                for (en_idx=0; en_idx < sizeof(_en)/sizeof(int); ++en_idx) {
                    save_en_idx = en_idx;
                    clkf = ((ddr_hertz) * _en[save_en_idx]);
                    clkf = divide_nint(clkf, 50 * 1000000);
                    pll_MHz = clkf * 50;
                    calculated_ddr_hertz = (pll_MHz * 1000000) / _en[save_en_idx];
                    error = ddr_hertz - calculated_ddr_hertz;

#if defined(__U_BOOT__)
                    ddr_print("en: %2d, clkf: %4lu, pll_MHz: %4lu, ddr_hertz: %9lu, error: %10d, best_error: %8d\n",
                              _en[save_en_idx], clkf, pll_MHz, calculated_ddr_hertz, error, best_error);
#else
                    ddr_print("en: %2d, clkf: %4llu, pll_MHz: %4llu, ddr_hertz: %9llu, error: %10lld, best_error: %8d\n",
                              _en[save_en_idx], clkf, pll_MHz, calculated_ddr_hertz, error, best_error);
#endif


                    if ((pll_MHz < min_pll_MHz) || (pll_MHz > max_pll_MHz)) continue;
                    if ((clkf < min_clkf) || (clkf > max_clkf)) continue;
                    if (_abs(error) > _abs(best_error)) continue;

                    if ((_abs(error) < _abs(best_error)) || (pll_MHz > highest_pll_MHz)) {
                        highest_pll_MHz = pll_MHz;
                        best_pll_MHz = pll_MHz;
                        best_calculated_ddr_hertz = calculated_ddr_hertz;
                        best_error = error;
                        best_clkf = clkf;
                        best_en_idx = save_en_idx;
                    }
                }

#if defined(__U_BOOT__)
                ddr_print("en: %2d, clkf: %4lu, pll_MHz: %4lu, ddr_hertz: %8lu, error: %10d <==\n",
                          _en[best_en_idx], best_clkf, best_pll_MHz, best_calculated_ddr_hertz, best_error);
#else
                ddr_print("en: %2d, clkf: %4llu, pll_MHz: %4llu, ddr_hertz: %8llu, error: %10lld <==\n",
                          _en[best_en_idx], best_clkf, best_pll_MHz, best_calculated_ddr_hertz, best_error);
#endif

                /* Try lowering the frequency if we can't get a working configuration */
                if (best_error == ddr_hertz) {
                    if (ddr_hertz < orig_ddr_hertz - 10000000)
                        break;
                    ddr_hertz -= 1000000;
                    best_error = ddr_hertz;
                }
            }
            if (best_error == ddr_hertz) {
                error_print("ERROR: Can not compute a legal DDR clock speed configuration.\n");
                return(-1);
            }

            ddr_pll_ctl.cn63xx.ddr_ps_en = best_en_idx;
            ddr_pll_ctl.cn63xx.dfm_ps_en = best_en_idx; /* Set DFM to the DDR speed. */
            ddr_pll_ctl.cn63xx.clkf = best_clkf;

            cvmx_write_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num), ddr_pll_ctl.u64);
        }

        /*
         * 3. Read LMC0_DDR_PLL_CTL and wait for the result.
         */

        cvmx_read_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num));

        /*
         * 4. Wait 3 usec.
         */

        cvmx_wait_usec(3);          /* Wait 3 us */

        /*
         * 5. Write LMC0_DDR_PLL_CTL[RESET_N] = 1 without changing any
         *    other LMC0_DDR_PLL_CTL values.
         */

        ddr_pll_ctl.u64 = cvmx_read_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num));
        ddr_pll_ctl.cn63xx.reset_n = 1;
        cvmx_write_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num), ddr_pll_ctl.u64);

        /*
         * 6. Read LMC0_DDR_PLL_CTL and wait for the result.
         */

        cvmx_read_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num));

        /*
         * 7. Wait 25 usec.
         */

        cvmx_wait_usec(25);          /* Wait 25 us */

        /*
         * 4.8.2 LMC CK Initialization
         *
         * DDR PLL initialization must be completed prior to starting
         * LMC CK initialization.
         *
         * 1. Without changing any other LMC0_DDR_PLL_CTL values,
         *    write LMC0_DDR_PLL_CTL[DDR_DIV_RESET] = 1 and
         *    LMC0_DDR_PLL_CTL [DDR_PS_EN] with the appropriate value
         *    to get the desired LMC CK speed. Section Section 4.13
         *    discusses CLKF and DDR_PS_EN programmings.
         *
         *    The LMC0_DDR_PLL_CTL[DDR_PS_EN] must not change after
         *    this point without restarting this LMC CK initialization
         *    sequence.
         */

        ddr_pll_ctl.u64 = cvmx_read_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num));
        ddr_pll_ctl.cn63xx.ddr_div_reset = 1;
        cvmx_write_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num), ddr_pll_ctl.u64);


        /*
         * 2. Without changing any other LMC0_COMP_CTL2 values, write
         *    LMC0_COMP_CTL2[CK_CTL] to the desired DDR_CK_*_P/
         *    DDR_DIMM*_CS*_L/DDR_DIMM*_ODT_* drive strength.
         */

        {
            cvmx_lmcx_comp_ctl2_t comp_ctl2;
            comp_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num));
            comp_ctl2.s.dqx_ctl            = 0x4; /* 34.3 ohm */
            comp_ctl2.s.ck_ctl             = 0x4; /* 34.3 ohm */
            comp_ctl2.s.cmd_ctl            = 0x4; /* 34.3 ohm */
            comp_ctl2.s.rodt_ctl           = 0x4; /* 60 ohm */

            if (((s = ddr_getenv_debug("ddr_clk_ctl")) != NULL) || ((s = ddr_getenv_debug("ddr_ck_ctl")) != NULL)) {
                comp_ctl2.s.ck_ctl  = simple_strtoul(s, NULL, 0);
                error_print("Parameter found in environment. ddr_clk_ctl = 0x%x\n", comp_ctl2.s.ck_ctl);
            }

            if ((s = ddr_getenv_debug("ddr_cmd_ctl")) != NULL) {
                comp_ctl2.s.cmd_ctl  = simple_strtoul(s, NULL, 0);
                error_print("Parameter found in environment. ddr_cmd_ctl = 0x%x\n", comp_ctl2.s.cmd_ctl);
            }

            if ((s = ddr_getenv_debug("ddr_dqx_ctl")) != NULL) {
                comp_ctl2.s.dqx_ctl  = simple_strtoul(s, NULL, 0);
                error_print("Parameter found in environment. ddr_dqx_ctl = 0x%x\n", comp_ctl2.s.dqx_ctl);
            }

            cvmx_write_csr(CVMX_LMCX_COMP_CTL2(ddr_interface_num), comp_ctl2.u64);
        }

        /*
         * 3. Read LMC0_DDR_PLL_CTL and wait for the result.
         */

        cvmx_read_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num));

        /*
         * 4. Wait 200 ns.
         */

        cvmx_wait_usec(200000);          /* Wait 200 ns */

        /*
         * 5. Without changing any other LMC0_DDR_PLL_CTL values,
         *    write LMC0_DDR_PLL_CTL[DDR_DIV_RESET] = 0.
         */

        ddr_pll_ctl.cn63xx.ddr_div_reset = 0;

        cvmx_write_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num), ddr_pll_ctl.u64);

        /*
         * 6. Read LMC0_DDR_PLL_CTL and wait for the result.
         */

        cvmx_read_csr(CVMX_LMCX_DDR_PLL_CTL(ddr_interface_num));

        /*
         * 7. Wait 200 ns.
         */

        cvmx_wait_usec(1);      /* Wait 200 ns */

        /*
         * 4.8.3 LMC DRESET Initialization
         *
         * Both DDR PLL and LMC CK initializations must be completed
         * prior to starting this LMC DRESET initialization.
         *
         * 1. If not done already, write LMC0_DLL_CTL2 to its reset
         *    value, including LMC0_DLL_CTL2[DRESET] = 1.
         */

        /* Already done. */

        /*
         * 2. Without changing any other LMC0_DLL_CTL2 fields, write
         *    LMC0_DLL_CTL2 [DLL_BRINGUP] = 1.
         */

        dll_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_DLL_CTL2(ddr_interface_num));
        dll_ctl2.cn63xx.dll_bringup = 1;
        cvmx_write_csr(CVMX_LMCX_DLL_CTL2(ddr_interface_num), dll_ctl2.u64);

        /*
         * 3. Read LMC0_DLL_CTL2 and wait for the result.
         */

        cvmx_read_csr(CVMX_LMCX_DLL_CTL2(ddr_interface_num));

        /*
         * 4. Wait for 10 LMC CK cycles.
         */

        cvmx_wait_usec(1);

        /*
         * 5. Without changing any other fields in LMC0_DLL_CTL2, write
         *
         *    LMC0_DLL_CTL2[QUAD_DLL_ENA] = 1.
         *
         *    LMC0_DLL_CTL2[QUAD_DLL_ENA] must not change after this
         *    point without restarting the LMC DRESET initialization
         *    sequence.
         */

        dll_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_DLL_CTL2(ddr_interface_num));
        dll_ctl2.cn63xx.quad_dll_ena = 1;
        cvmx_write_csr(CVMX_LMCX_DLL_CTL2(ddr_interface_num), dll_ctl2.u64);

        /*
         * 6. Read LMC0_DLL_CTL2 and wait for the result.
         */

        cvmx_read_csr(CVMX_LMCX_DLL_CTL2(ddr_interface_num));

        /*
         * 7. Wait 10 usec.
         */

        cvmx_wait_usec(10);

        /*
         * 8. Without changing any other fields in LMC0_DLL_CTL2, write
         *
         *    LMC0_DLL_CTL2[DLL_BRINGUP] = 0.
         *
         *    LMC0_DLL_CTL2[DLL_BRINGUP] must not change after this
         *    point without restarting the LMC DRESET initialization
         *    sequence.
         */

        dll_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_DLL_CTL2(ddr_interface_num));
        dll_ctl2.cn63xx.dll_bringup = 0;
        cvmx_write_csr(CVMX_LMCX_DLL_CTL2(ddr_interface_num), dll_ctl2.u64);

        /*
         * 9. Read LMC0_DLL_CTL2 and wait for the result.
         */

        cvmx_read_csr(CVMX_LMCX_DLL_CTL2(ddr_interface_num));

        /*
         * 10. Without changing any other fields in LMC0_DLL_CTL2, write
         *
         *     LMC0_DLL_CTL2[DRESET] = 0.
         *
         *     LMC0_DLL_CTL2[DRESET] must not change after this point
         *     without restarting the LMC DRESET initialization
         *     sequence.
         *
         *     After completing LMC DRESET initialization, all LMC
         *     CSRs may be accessed. Prior to completing LMC DRESET
         *     initialization, only LMC0_DDR_PLL_CTL, LMC0_DLL_CTL2,
         *     LMC0_RESET_CTL, and LMC0_COMP_CTL2 LMC CSRs can be
         *     accessed.
         */

        dll_ctl2.u64 = cvmx_read_csr(CVMX_LMCX_DLL_CTL2(ddr_interface_num));
        dll_ctl2.cn63xx.dreset = 0;
        cvmx_write_csr(CVMX_LMCX_DLL_CTL2(ddr_interface_num), dll_ctl2.u64);

        /*
         * 4.8.4 LMC RESET Initialization
         *
         * The purpose of this step is to assert/deassert the RESET#
         * pin at the DDR3 parts.
         *
         * It may be appropriate to skip this step if the DDR3 DRAM
         * parts are in self refresh and are currently preserving
         * their contents. (Software can determine this via
         * LMC0_RESET_CTL[DDR3PSV] in some circumstances.)  The
         * remainder of this section assumes that the DRAM contents
         * need not be preserved.
         *
         * The remainder of this section assumes that the CN63XX
         * DDR_RESET_L pin is attached to the RESET# pin of the
         * attached DDR3 parts, as will be appropriate in many
         * systems.
         *
         * (In other systems, such as ones that can preserve DDR3 part
         * contents while CN63XX is powered down, it will not be
         * appropriate to directly attach the CN63XX DDR_RESET_L pin
         * to DRESET# of the DDR3 parts, and this section may not
         * apply.)
         *
         * DRAM Controller (LMC): LMC Initialization Sequence
         *
         * 1. If not done already, assert DDR_RESET_L pin by writing
         *    LMC0_RESET_CTL[DDR3RST] = 0 without modifying any other
         *    LMC0_RESET_CTL fields.
         */

        lmc_reset_ctl.u64 = cvmx_read_csr(CVMX_LMCX_RESET_CTL(ddr_interface_num));
        if (lmc_reset_ctl.cn63xx.ddr3psv == 0) {
            /* Contents are not being preserved */
            lmc_reset_ctl.cn63xx.ddr3rst = 0;
            cvmx_write_csr(CVMX_LMCX_RESET_CTL(ddr_interface_num), lmc_reset_ctl.u64);

            /*
             * 2. Read LMC0_RESET_CTL and wait for the result.
             */

            cvmx_read_csr(CVMX_LMCX_RESET_CTL(ddr_interface_num));

            /*
             * 3. Wait until RESET# assertion time requirement from JEDEC
             *    DDR3 specification is satisfied (200 usec during a
             *    power-on ramp, 100ns when power is already stable).
             */

            cvmx_wait_usec(200);

            /*
             * 4. Deassert DDR_RESET_L pin by writing
             *    LMC0_RESET_CTL[DDR3RST] = 1 without modifying any other
             *    LMC0_RESET_CTL fields.
             */

            lmc_reset_ctl.u64 = cvmx_read_csr(CVMX_LMCX_RESET_CTL(ddr_interface_num));
            lmc_reset_ctl.cn63xx.ddr3rst = 1;
            cvmx_write_csr(CVMX_LMCX_RESET_CTL(ddr_interface_num), lmc_reset_ctl.u64);

            /*
             * 5. Read LMC0_RESET_CTL and wait for the result.
             */

            cvmx_read_csr(CVMX_LMCX_RESET_CTL(ddr_interface_num));
        }

        /*
         * (Subsequent RESET# deassertion to CKE assertion requirement
         * can be satisfied during LMC initialization via
         * LMC0_TIMING_PARAMS0[TCKEON] timing.)
         */

    }

    if (octeon_is_cpuid(OCTEON_CN71XX)) {
        if (initialize_octeon3_ddr3_clock(cpu_id, ddr_configuration,
                                      cpu_hertz, ddr_hertz, ddr_ref_hertz,
                                          ddr_interface_num, ddr_interface_mask) < 0)
            return -1;
    }


    /* Count DDR clocks, and we use this to correct the DDR clock that
    ** we are passed.  We must enable the memory controller to count
    ** DDR clocks. */
    if (!(octeon_is_cpuid(OCTEON_CN63XX) || octeon_is_cpuid(OCTEON_CN71XX))) {
        mem_cfg0.u64 = 0;
        mem_cfg0.s.init_start   = 1;
        cvmx_write_csr(CVMX_LMCX_MEM_CFG0(ddr_interface_num), mem_cfg0.u64);
        cvmx_read_csr(CVMX_LMCX_MEM_CFG0(ddr_interface_num));
    }

    set_ddr_clock_initialized(ddr_interface_num, 1);
    return(0);
}

static void octeon_ipd_delay_cycles(uint64_t cycles)
{
    uint64_t start = cvmx_read_csr((uint64_t)0x80014F0000000338ULL);
    while (start + cycles > cvmx_read_csr((uint64_t)0x80014F0000000338ULL))
        ;
}

uint32_t measure_octeon_ddr_clock(uint32_t cpu_id,
                                  const ddr_configuration_t *ddr_configuration,
                                  uint32_t cpu_hertz,
                                  uint32_t ddr_hertz,
                                  uint32_t ddr_ref_hertz,
                                  int ddr_interface_num,
                                  uint32_t ddr_interface_mask
                                  )
{
    uint64_t core_clocks;
    uint64_t ddr_clocks;
    uint64_t calc_ddr_hertz;
    
    DECLARE_GLOBAL_DATA_PTR;

    if (initialize_ddr_clock(cpu_id, ddr_configuration, cpu_hertz,
                             ddr_hertz, ddr_ref_hertz, ddr_interface_num,
                             ddr_interface_mask) != 0)
        return (0);
#ifdef CONFIG_JSRXNLE
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX550_MODELS //FIXME : need to make this work for Magni. For now hardcoding DDR speed.
        return 533333333;
    default:
        break;
    }
#endif

    /* Dynamically determine the DDR clock speed */
    if (octeon_is_cpuid(OCTEON_CN63XX) || octeon_is_cpuid(OCTEON_CN71XX)) {
        core_clocks = cvmx_read_csr(CVMX_IPD_CLK_COUNT);
        ddr_clocks = cvmx_read_csr(CVMX_LMCX_DCLK_CNT(ddr_interface_num));
        octeon_ipd_delay_cycles(100000000); /* How many cpu cycles to measure over */
        core_clocks = cvmx_read_csr(CVMX_IPD_CLK_COUNT) - core_clocks;
        ddr_clocks = cvmx_read_csr(CVMX_LMCX_DCLK_CNT(ddr_interface_num)) - ddr_clocks;
        calc_ddr_hertz = ddr_clocks * gd->sys_clock_mhz * 1000000 / core_clocks;
    } else {
        core_clocks = cvmx_read_csr(CVMX_IPD_CLK_COUNT);
        ddr_clocks = cvmx_read_csr(CVMX_LMCX_DCLK_CNT_LO(ddr_interface_num));  /* ignore overflow, starts counting when we enable the controller */
        octeon_ipd_delay_cycles(100000000); /* How many cpu cycles to measure over */
        core_clocks = cvmx_read_csr(CVMX_IPD_CLK_COUNT) - core_clocks;
        ddr_clocks = cvmx_read_csr(CVMX_LMCX_DCLK_CNT_LO(ddr_interface_num)) - ddr_clocks;
        calc_ddr_hertz = ddr_clocks * cpu_hertz / core_clocks;
    }
    ddr_print("....Measured DDR clock %qu\n", calc_ddr_hertz);
   /* THU */
#ifdef CONFIG_MAG8600
    //calc_ddr_hertz = 395*1000*1000; 
#ifdef CONFIG_RAM_RESIDENT
    calc_ddr_hertz = 400*1000*1000; 
#endif
#endif

    /* Check for unreasonable settings. */
    if (calc_ddr_hertz < 10000) {
        error_print("DDR clock misconfigured. Resetting...\n");
    }

    /* Check for unreasonable settings. */
    if (calc_ddr_hertz == 0) {
        error_print("DDR clock misconfigured. Exiting.\n");
    }
    return (calc_ddr_hertz);
}

int octeon_twsi_set_addr16(uint8_t dev_addr, uint16_t addr)
{

    /* 16 bit internal address ONLY */
    uint64_t val;
    cvmx_write_csr(CVMX_MIO_TWS_SW_TWSI,0x8000000000000000ull | ( 0x1ull << 57) | (((uint64_t)dev_addr) << 40) | ((((uint64_t)addr) >> 8) << 32) | (addr & 0xff)); // tell twsii to do the read
    while (cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI)&0x8000000000000000ull);
    val = cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI);
    if (!(val & 0x0100000000000000ull)) {
        return -1;
    }

    return(0);
}

int octeon_twsi_set_addr8(uint8_t dev_addr, uint16_t addr)
{

    /* 16 bit internal address ONLY */
    uint64_t val;
    cvmx_write_csr(CVMX_MIO_TWS_SW_TWSI,0x8000000000000000ull | ( 0x0ull << 57) | (((uint64_t)dev_addr) << 40) | (addr & 0xff)); // tell twsii to do the read
    while (cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI)&0x8000000000000000ull);
    val = cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI);
    if (!(val & 0x0100000000000000ull)) {
        return -1;
    }

    return(0);
}

int octeon_twsi_read8_cur_addr(uint8_t dev_addr)
{

    uint64_t val;
    cvmx_write_csr(CVMX_MIO_TWS_SW_TWSI,0x8100000000000000ull | ( 0x0ull << 57) | (((uint64_t)dev_addr) << 40) ); // tell twsii to do the read
    while (cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI)&0x8000000000000000ull);
    val = cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI);
    if (!(val & 0x0100000000000000ull))
    {
        return -1;
    }
    else
    {
        return(uint8_t)(val & 0xFF);
    }
}

int octeon_twsi_write8(uint8_t dev_addr, uint8_t addr, uint8_t data)
{

    uint64_t val, val_1;

    val_1= 0x8000000000000000ull | ( 0x0ull << 57) | ( 0x1ull << 55) | ( 0x1ull << 52) | (((uint64_t)dev_addr) << 40) | (((uint64_t)addr) << 8) | data ;
    cvmx_write_csr(CVMX_MIO_TWS_SW_TWSI, val_1 ); // tell twsii to do the read
    while (cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI)&0x8000000000000000ull);
    val = cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI);
    if (!(val & 0x0100000000000000ull)) {
	printf("twsii write (2byte) failed\n");
        return -1;
    }

    /* or poll for read to not fail */
    //octeon_delay_cycles(40*1000000);
    return(0);
}

int octeon_twsi_read8(uint8_t dev_addr, uint16_t addr)
{
    if (octeon_twsi_set_addr16(dev_addr, addr))
        return(-1);

    return(octeon_twsi_read8_cur_addr(dev_addr));
}

int octeon_twsi_read16_cur_addr(uint8_t dev_addr)
{
    int tmp;
    uint16_t val;
    tmp = octeon_twsi_read8_cur_addr(dev_addr);
    if (tmp < 0)
        return(-1);
    val = (tmp & 0xff) << 8;
    tmp = octeon_twsi_read8_cur_addr(dev_addr);
    if (tmp < 0)
        return(-1);
    val |= (tmp & 0xff);

    return((int)val);

}

int octeon_twsi_read16(uint8_t dev_addr, uint16_t addr)
{
    if (octeon_twsi_set_addr16(dev_addr, addr))
        return(-1);

    return(octeon_twsi_read16_cur_addr(dev_addr));
}

int octeon_twsi_write16(uint8_t dev_addr, uint16_t addr, uint16_t data)
{

    uint64_t val;
    cvmx_write_csr(CVMX_MIO_TWS_SW_TWSI,0x8000000000000000ull | ( 0x8ull << 57) | (((uint64_t)dev_addr) << 40) | (addr << 16) | data ); // tell twsii to do the read
    while (cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI)&0x8000000000000000ull);
    val = cvmx_read_csr(CVMX_MIO_TWS_SW_TWSI);
    if (!(val & 0x0100000000000000ull)) {
        //      printf("twsii write (4byte) failed\n");
        return -1;
    }

    /* or poll for read to not fail */
    octeon_delay_cycles(40*1000000);
    return(0);
}



#define tlv_debug_print(...) /* Careful... This could be too early to print */

/***************** EEPROM TLV support functions ******************/

int  octeon_tlv_eeprom_get_next_tuple(uint8_t dev_addr, uint16_t addr, uint8_t *buf_ptr, uint32_t buf_len)
{
    octeon_eeprom_header_t *tlv_hdr_ptr = (void *)buf_ptr;
    uint16_t checksum = 0;
    unsigned int i;


    if (buf_len < sizeof(octeon_eeprom_header_t))
    {
        tlv_debug_print("ERROR: buf_len too small: %d, must be at least %d\n", buf_len, sizeof(octeon_eeprom_header_t));
        return(-1);
    }
    if (octeon_twsi_set_addr16(dev_addr,addr))
    {
        tlv_debug_print("octeon_tlv_eeprom_get_next_tuple: unable to set address: 0x%x, device: 0x%x\n", addr, dev_addr);
        return(-1);
    }
    for (i = 0; i < sizeof(octeon_eeprom_header_t); i++)
    {
        *buf_ptr = (uint8_t)octeon_twsi_read8_cur_addr(dev_addr);
        tlv_debug_print("Read: 0x%x\n", *buf_ptr);
        checksum += *buf_ptr++;
    }

    /* Fix endian issues - data structure in EEPROM is big endian */
    tlv_hdr_ptr->length = ntohs(tlv_hdr_ptr->length);
    tlv_hdr_ptr->type = ntohs(tlv_hdr_ptr->type);
    tlv_hdr_ptr->checksum = ntohs(tlv_hdr_ptr->checksum);

    if (tlv_hdr_ptr->type == EEPROM_END_TYPE)
        return -1;

    tlv_debug_print("TLV header at addr 0x%x: type: 0x%x, len: 0x%x, maj_version: %d, min_version: %d, checksum: 0x%x\n",
           addr,tlv_hdr_ptr->type, tlv_hdr_ptr->length, tlv_hdr_ptr->major_version, tlv_hdr_ptr->minor_version, tlv_hdr_ptr->checksum);

    /* Do basic header check to see if we should continue */

    if (tlv_hdr_ptr->length > OCTEON_EEPROM_MAX_TUPLE_LENGTH
        || tlv_hdr_ptr->length < sizeof(octeon_eeprom_header_t))
    {
        tlv_debug_print("Invalid tuple type/length: type: 0x%x, len: 0x%x\n", tlv_hdr_ptr->type, tlv_hdr_ptr->length);
        return(-1);
    }
    if (buf_len < tlv_hdr_ptr->length)
    {
        tlv_debug_print("Error: buffer length too small.\n");
        return(-1);
    }

    /* Read rest of tuple into buffer */
    for (i = 0; i < (int)tlv_hdr_ptr->length - sizeof(octeon_eeprom_header_t); i++)
    {
        *buf_ptr = (uint8_t)octeon_twsi_read8_cur_addr(dev_addr);
        checksum += *buf_ptr++;
    }

    checksum -= (tlv_hdr_ptr->checksum & 0xff) + (tlv_hdr_ptr->checksum >> 8);

    if (checksum != tlv_hdr_ptr->checksum)
    {
        tlv_debug_print("Checksum mismatch: computed 0x%x, found 0x%x\n", checksum, tlv_hdr_ptr->checksum);
        return(-1);
    }

    return(tlv_hdr_ptr->length);
}

/* find tuple based on type and major version.  if not found, returns next available address with bit 31 set (negative).
** If positive value is returned the tuple was found
** if supplied version is zero, matches any version
**
*/
int octeon_tlv_get_tuple_addr(uint8_t dev_addr, uint16_t type, uint8_t major_version, uint8_t *eeprom_buf, uint32_t buf_len)
{

    octeon_eeprom_header_t *tlv_hdr_ptr = (void *)eeprom_buf;
    int cur_addr = 0;
#ifdef CONFIG_EEPROM_TLV_BASE_ADDRESS
    cur_addr = CONFIG_EEPROM_TLV_BASE_ADDRESS; /* Skip to the beginning of the TLV region. */
#endif

    int len;


    while ((len = octeon_tlv_eeprom_get_next_tuple(dev_addr, cur_addr, eeprom_buf, buf_len)) > 0)
    {
        /* Checksum already validated here, endian swapping of header done in  octeon_tlv_eeprom_get_next_tuple*/
        /* Check to see if we found matching */
#if !defined(CONFIG_JSRXNLE) && !defined(CONFIG_MAG8600) && !defined(CONFIG_ACX500_SVCS)
        if (type == tlv_hdr_ptr->type && (!major_version || (major_version == tlv_hdr_ptr->major_version)))
        {
            return(cur_addr);
        }
#endif
        cur_addr += len;
    }
    /* Return next available address with bit 31 set */
    cur_addr |= (1 << 31);

    return(cur_addr);

}
#ifdef CONFIG_JSRXNLE
/* Table where all the DDR configurations are defined.  This is shared between
** u-boot and oct-remote-boot.  oct-remote-boot has all boards compiled in, while
** u-boot only includes the board for which it is built.
*/
const board_table_entry_t octeon_board_ddr_config_table[] =
{
    {
        .board_type = I2C_ID_SRX550_CHASSIS,
        .board_has_spd = TRUE,
        .default_ddr_clock_hz = 533333333,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX550_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_SRX550M_CHASSIS,
        .board_has_spd = TRUE,
        .default_ddr_clock_hz = 533333333,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX550_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_JSRXNLE_SRX650_CHASSIS,
        .board_has_spd = TRUE,
        .default_ddr_clock_hz = 333333333,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX650_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_JSRXNLE_SRX240_LOWMEM_CHASSIS,
        .board_has_spd = TRUE,
        .default_ddr_clock_hz = 333333333,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX240_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_JSRXNLE_SRX240_HIGHMEM_CHASSIS,
        .board_has_spd = TRUE,
        .default_ddr_clock_hz = 333333333,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX240_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_JSRXNLE_SRX240_POE_CHASSIS,
        .board_has_spd = TRUE,
        .default_ddr_clock_hz = 333333333,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX240_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_JSRXNLE_SRX240H_DC_CHASSIS,
        .board_has_spd = TRUE,
        .default_ddr_clock_hz = 333333333,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX240_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_JSRXNLE_SRX240B2_CHASSIS,
        .board_has_spd = TRUE,
        .default_ddr_clock_hz = 333333333,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX240_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_JSRXNLE_SRX240H2_CHASSIS,
        .board_has_spd = TRUE,
        .default_ddr_clock_hz = 333333333,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX240_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_JSRXNLE_SRX240H2_POE_CHASSIS,
        .board_has_spd = TRUE,
        .default_ddr_clock_hz = 333333333,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX240_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_JSRXNLE_SRX240H2_DC_CHASSIS,
        .board_has_spd = TRUE,
        .default_ddr_clock_hz = 333333333,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX240_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_JSRXNLE_SRX220_HIGHMEM_CHASSIS,
        .board_has_spd = TRUE,
        .default_ddr_clock_hz = 266000000,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX220_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_JSRXNLE_SRX220_POE_CHASSIS,
        .board_has_spd = TRUE,
        .default_ddr_clock_hz = 266000000,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX220_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_SRXSME_SRX220H2_CHASSIS,
        .board_has_spd = TRUE,
        .default_ddr_clock_hz = 266000000,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX220_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_SRXSME_SRX220H2_POE_CHASSIS,
        .board_has_spd = TRUE,
        .default_ddr_clock_hz = 266000000,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX220_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_JSRXNLE_SRX210_LOWMEM_CHASSIS,
        .board_has_spd = FALSE,
        .default_ddr_clock_hz = 200000000,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX210_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_JSRXNLE_SRX210_HIGHMEM_CHASSIS,
        .board_has_spd = FALSE,
        .default_ddr_clock_hz = 200000000,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX210_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_JSRXNLE_SRX210_POE_CHASSIS,
        .board_has_spd = FALSE,
        .default_ddr_clock_hz = 200000000,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX210_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_SRXSME_SRX210BE_CHASSIS,
        .board_has_spd = FALSE,
        .default_ddr_clock_hz = 266000000,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX210_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_SRXSME_SRX210HE_CHASSIS,
        .board_has_spd = FALSE,
        .default_ddr_clock_hz = 266000000,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX210_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_SRXSME_SRX210HE_POE_CHASSIS,
        .board_has_spd = FALSE,
        .default_ddr_clock_hz = 266000000,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX210_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_SRXSME_SRX210HE2_CHASSIS,
        .board_has_spd = FALSE,
        .default_ddr_clock_hz = 266000000,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX210_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_SRXSME_SRX210HE2_POE_CHASSIS,
        .board_has_spd = FALSE,
        .default_ddr_clock_hz = 266000000,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX210_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_JSRXNLE_SRX100_LOWMEM_CHASSIS,
        .board_has_spd = FALSE,
        .default_ddr_clock_hz = 266000000,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX210_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_JSRXNLE_SRX100_HIGHMEM_CHASSIS,
        .board_has_spd = FALSE,
        .default_ddr_clock_hz = 266000000,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX210_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_SRXSME_SRX100H2_CHASSIS,
        .board_has_spd = FALSE,
        .default_ddr_clock_hz = 266000000,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX210_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = I2C_ID_SRXSME_SRX110B_VA_CHASSIS,
        .board_has_spd = TRUE,
        .default_ddr_clock_hz = 266000000,
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    SRX110_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = 0,  /* Mark the end of the board list */
    },

};
#endif

#ifdef CONFIG_MAG8600
#define MAG8600_DRAM_SOCKET_CONFIGURATION \
        {{MSC_DIMM_0_SPD_TWSI_ADDR, 0x0}, {NULL, NULL}},                \
        {{MSC_DIMM_1_SPD_TWSI_ADDR ,0x0}, {NULL, NULL}}

#define MAG8600_DDR_CONFIGURATION                                       \
    {                                                                   \
        .dimm_config_table = {MAG8600_DRAM_SOCKET_CONFIGURATION,        \
             DIMM_CONFIG_TERMINATOR},                                   \
            .unbuffered = {                                             \
             .ddr_board_delay = OCTEON_CN56XX_MSC_DDR_BOARD_DELAY,      \
             .lmc_delay_clk = OCTEON_CN56XX_MSC_UNB_LMC_DELAY_CLK,      \
             .lmc_delay_cmd = OCTEON_CN56XX_MSC_UNB_LMC_DELAY_CMD ,     \
             .lmc_delay_dq = OCTEON_CN56XX_MSC_UNB_LMC_DELAY_DQ,        \
         },                                                             \
                 .registered = {                                        \
             .ddr_board_delay = OCTEON_CN56XX_MSC_DDR_BOARD_DELAY,      \
             .lmc_delay_clk = OCTEON_CN56XX_MSC_LMC_DELAY_CLK ,         \
             .lmc_delay_cmd = OCTEON_CN56XX_MSC_LMC_DELAY_CMD,          \
             .lmc_delay_dq =  OCTEON_CN56XX_MSC_LMC_DELAY_DQ,           \
         },                                                             \
                      .odt_1rank_config = {                             \
             CN56XX_DRAM_ODT_1RANK_CONFIGURATION,                       \
         },                                                             \
                           .odt_2rank_config = {                        \
             CN56XX_DRAM_ODT_2RANK_CONFIGURATION,                       \
         },                                                             \
   }
const board_table_entry_t octeon_board_ddr_config_table[] =
{
    {
        .board_type = MAG8600_MSC,
        .board_has_spd = TRUE,
        .default_ddr_clock_hz = (MSC_DEF_DRAM_FREQ*1000000),
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    MAG8600_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = 0,  /* Mark the end of the board list */
    },
};
#endif

#if defined (CONFIG_ACX500_SVCS)
static uint8_t acx500_svcs_spd_values[128] =
					{ ACX500_SVCS_SPD_VALUES };

const board_table_entry_t octeon_board_ddr_config_table[] =
{
    {
        .board_type = BOARD_TYPE_ACX500_SVCS,
        .board_has_spd = FALSE,
        .default_ddr_clock_hz = (ACX500_SVCS_DEF_DRAM_FREQ*1000000),
        .eeprom_addr = 0,
        .chip_ddr_config =
        {
            {
                .ddr_config = {
                    ACX500_SVCS_DDR_CONFIGURATION
                }
            }
        }
    },
    {
        .board_type = 0,  /* Mark the end of the board list */
    },
};
#endif

#ifdef DEBUG
/* Serial Presence Detect (SPD) for DDR2 SDRAM - JEDEC Standard No. 21-C */
/* ===================================================================== */

const char *ddr2_spd_strings[] = {
/* Byte                                                                   */
/* Number  Function                                                       */
/* ======  ============================================================== */
/*  0     */ "Number of Serial PD Bytes written during module production",
/*  1     */ "Total number of Bytes in Serial PD device",
/*  2     */ "Fundamental Memory Type (FPM, EDO, SDRAM, DDR, DDR2)",
/*  3     */ "Number of Row Addresses on this assembly",
/*  4     */ "Number of Column Addresses on this assembly",
/*  5     */ "Number of DIMM Ranks",
/*  6     */ "Data Width of this assembly",
/*  7     */ "Reserved",
/*  8     */ "Voltage Interface Level of this assembly",
/*  9     */ "SDRAM Cycle time at Maximum Supported CAS Latency (CL), CL=X",
/* 10     */ "SDRAM Access from Clock (tAC)",
/* 11     */ "DIMM configuration type (Non-parity, Parity or ECC)",
/* 12     */ "Refresh Rate/Type (tREFI)",
/* 13     */ "Primary SDRAM Width",
/* 14     */ "Error Checking SDRAM Width",
/* 15     */ "Reserved",
/* 16     */ "SDRAM Device Attributes: Burst Lengths Supported",
/* 17     */ "SDRAM Device Attributes: Number of Banks on SDRAM Device",
/* 18     */ "SDRAM Device Attributes: CAS Latency",
/* 19     */ "Reserved",
/* 20     */ "DIMM Type Information",
/* 21     */ "SDRAM Module Attributes",
/* 22     */ "SDRAM Device Attributes: General",
/* 23     */ "Minimum Clock Cycle at CLX-1",
/* 24     */ "Maximum Data Access Time (tAC) from Clock at CLX-1",
/* 25     */ "Minimum Clock Cycle at CLX-2",
/* 26     */ "Maximum Data Access Time (tAC) from Clock at CLX-2",
/* 27     */ "Minimum Row Precharge Time (tRP)",
/* 28     */ "Minimum Row Active to Row Active delay (tRRD)",
/* 29     */ "Minimum RAS to CAS delay (tRCD)",
/* 30     */ "Minimum Active to Precharge Time (tRAS)",
/* 31     */ "Module Rank Density",
/* 32     */ "Address and Command Input Setup Time Before Clock (tIS)",
/* 33     */ "Address and Command Input Hold Time After Clock (tIH)",
/* 34     */ "Data Input Setup Time Before Clock (tDS)",
/* 35     */ "Data Input Hold Time After Clock (tDH)",
/* 36     */ "Write recovery time (tWR)",
/* 37     */ "Internal write to read command delay (tWTR)",
/* 38     */ "Internal read to precharge command delay (tRTP)",
/* 39     */ "Memory Analysis Probe Characteristics",
/* 40     */ "Extension of Byte 41 tRC and Byte 42 tRFC",
/* 41     */ "SDRAM Device Minimum Active to Active/Auto Refresh Time (tRC)",
/* 42     */ "SDRAM Min Auto-Ref to Active/Auto-Ref Command Period (tRFC)",
/* 43     */ "SDRAM Device Maximum device cycle time (tCKmax)",
/* 44     */ "SDRAM Device maximum skew between DQS and DQ signals (tDQSQ)",
/* 45     */ "SDRAM Device Maximum Read DataHold Skew Factor (tQHS)",
/* 46     */ "PLL Relock Time",
/* 47-61  */ "IDD in SPD - To be defined",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/* 62     */ "SPD Revision",
/* 63     */ "Checksum for Bytes 0-62",
/* 64-71  */ "Manufacturers JEDEC ID Code",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/* 72     */ "Module Manufacturing Location",
/* 73-90  */ "Module Part Number",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/* 91-92  */ "Module Revision Code",
/*        */ "",
/* 93-94  */ "Module Manufacturing Date",
/*        */ "",
/* 95-98  */ "Module Serial Number",
/*        */ "",
/*        */ "",
/*        */ "",
/* 99-127 */ "Manufacturers Specific Data",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
/*        */ "",
};
#endif /* DEBUG */

const ddr_configuration_t *lookup_ddr_config_structure(uint32_t cpu_id, int board_type, int board_rev_maj, int board_rev_min, uint32_t *interface_mask)
{

    /* Look up board type and get ddr_configuration from table */
    int i,j;
    if (!interface_mask)
        return NULL;
    *interface_mask = 0;

    for (i = 0; octeon_board_ddr_config_table[i].board_type;i++)
    {
        if (octeon_board_ddr_config_table[i].board_type == board_type)
        {

            /* Now look up the configuration matching the current chip type */
            for (j = 0; j < MAX_DDR_CONFIGS_PER_BOARD; j++)
            {
                if ((!octeon_board_ddr_config_table[i].chip_ddr_config[j].chip_model
                    || octeon_is_cpuid(octeon_board_ddr_config_table[i].chip_ddr_config[j].chip_model))
                    && (!octeon_board_ddr_config_table[i].chip_ddr_config[j].max_maj_rev
                        || (board_rev_maj <= octeon_board_ddr_config_table[i].chip_ddr_config[j].max_maj_rev
                            && board_rev_min <= octeon_board_ddr_config_table[i].chip_ddr_config[j].max_min_rev)))
                {
                    if (octeon_board_ddr_config_table[i].chip_ddr_config[j].ddr_config[0].dimm_config_table[0].spd_addrs[0]
                        || octeon_board_ddr_config_table[i].chip_ddr_config[j].ddr_config[0].dimm_config_table[0].spd_ptrs[0])
                        *interface_mask |= 1;
                    if (octeon_board_ddr_config_table[i].chip_ddr_config[j].ddr_config[1].dimm_config_table[0].spd_addrs[0]
                        || octeon_board_ddr_config_table[i].chip_ddr_config[j].ddr_config[1].dimm_config_table[0].spd_ptrs[0])
                        *interface_mask |= 1 << 1;

                    return(octeon_board_ddr_config_table[i].chip_ddr_config[j].ddr_config);
                    }
            }
        }
    }
    return(NULL);
}




/* Look up the board table entry for a board, given a board type or a board name */
const board_table_entry_t *lookup_board_table_entry(int board_type, char *board_name)
{

    int i;
    for (i = 0; octeon_board_ddr_config_table[i].board_type;i++)
    {
        if (octeon_board_ddr_config_table[i].board_type == board_type)
            return(&octeon_board_ddr_config_table[i]);
#if !defined(__U_BOOT__)
        if (board_name && !strcasecmp(board_name,cvmx_board_type_to_string(octeon_board_ddr_config_table[i].board_type)))
            return(&octeon_board_ddr_config_table[i]);
#endif
    }

    return(NULL);
}
