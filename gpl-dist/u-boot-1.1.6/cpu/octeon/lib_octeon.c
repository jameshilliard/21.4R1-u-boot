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
/**
 *
 * $Id$
 *
 */


#include <common.h>
#include <command.h>
#include <exports.h>
#include <linux/ctype.h>
#include "octeon_boot.h"
#include "octeon_hal.h"
#include <octeon_eeprom_types.h>
#include <lib_octeon_shared.h>
#include <configs/jsrxnle.h>
#include <cvmx-dpi-defs.h>
#include <cvmx-pemx-defs.h>
#include <cvmx-uctlx-addresses.h>
#include <cvmx-pexp-defs.h>
#include <cvmx-sli-defs.h>

#ifdef CONFIG_JSRXNLE
void srx650_send_booted_signal(void);
void srx650_wait_for_master(void);

/*
 * Boot bus chip select pins
 */
typedef enum {
    BOOT_CE_0_L = 0,
    BOOT_CE_1_L,
    BOOT_CE_2_L,
    BOOT_CE_3_L,
    BOOT_CE_4_L,
    BOOT_CE_5_L,
    BOOT_CE_6_L,
    BOOT_CE_7_L,
    BOOT_CE_LAST
} boot_ce_pin_t;

/*
 * SRX240/210 Boot bus config options for Mini PIMs. 
 */
typedef struct jsrnxle_mpim_boot_reg_cfg_ {
    boot_ce_pin_t boot_ce_pin;
    uint64_t     boot_ce_val;
    uint64_t     boot_reg_tim_val;
} jsrxnle_mpim_boot_reg_cfg_t;

static jsrxnle_mpim_boot_reg_cfg_t mpim_boot_reg_cfg_srx210[] = {
    /* Chip select 5 for MiniPIM 4 */
    { BOOT_CE_2_L,
      MIO_BOOT_REG_CFG_EN     |      /* Enable bit */
      MIO_BOOT_REG_CFG_WIDTH  |      /* Width - 8 bit */
      MIO_BOOT_REG_CFG_SIZE   |      /* Size */
      MINPIM_SRX210_BASE,            /* Base Address */
      MIO_BOOT_REG_TIM_RESET_VAL     /* TIM Reset Value */
    },
    { BOOT_CE_LAST, 0x00, 0x00 }
};

static jsrxnle_mpim_boot_reg_cfg_t mpim_boot_reg_cfg_srx220[] = {
    /* Chip select 2 for MiniPIM 1 */
    { BOOT_CE_2_L,
      MIO_BOOT_REG_CFG_EN     |      /* Enable bit */
      MIO_BOOT_REG_CFG_WIDTH  |      /* Width - 8 bit */
      MIO_BOOT_REG_CFG_SIZE_64K   |      /* Size */       
      SRX220_MPIM1_BASE,             /* Base Address */
      MIO_BOOT_REG_TIM_RESET_VAL     /* TIM Reset Value */
    },
    /* Chip select 3 for MiniPIM 2 */
    { BOOT_CE_3_L,
      MIO_BOOT_REG_CFG_EN     |      /* Enable bit */
      MIO_BOOT_REG_CFG_WIDTH  |      /* Width - 8 bit */
      MIO_BOOT_REG_CFG_SIZE_64K   |      /* Size */       
      SRX220_MPIM2_BASE,             /* Base Address */
      MIO_BOOT_REG_TIM_RESET_VAL     /* TIM Reset Value */
    },
     /* Chip select 4 for voice card */
    { BOOT_CE_4_L,
      MIO_BOOT_REG_CFG_EN     |      /* Enable bit */
      MIO_BOOT_REG_CFG_WIDTH  |      /* Width - 8 bit */
      MIO_BOOT_REG_CFG_SIZE   |      /* Size */
      SRX220_VOICE_BASE,             /* Base Address */
      MIO_BOOT_REG_TIM_RESET_VAL     /* TIM Reset Value */
    },
    { BOOT_CE_LAST, 0x00, 0x00 }
};

static jsrxnle_mpim_boot_reg_cfg_t mpim_boot_reg_cfg_srx240[] = {
    /* Chip select 2 for MiniPIM 1 */
    { BOOT_CE_2_L,
      MIO_BOOT_REG_CFG_EN     |      /* Enable bit */
      MIO_BOOT_REG_CFG_WIDTH  |      /* Width - 8 bit */
      MIO_BOOT_REG_CFG_SIZE   |      /* Size */
      SRX240_MPIM1_BASE,             /* Base Address */
      MIO_BOOT_REG_TIM_RESET_VAL     /* TIM Reset Value */
    },
    /* Chip select 3 for MiniPIM 2 */
    { BOOT_CE_3_L,
      MIO_BOOT_REG_CFG_EN     |      /* Enable bit */
      MIO_BOOT_REG_CFG_WIDTH  |      /* Width - 8 bit */
      MIO_BOOT_REG_CFG_SIZE   |      /* Size */
      SRX240_MPIM2_BASE,             /* Base Address */
      MIO_BOOT_REG_TIM_RESET_VAL     /* TIM Reset Value */
    },
    /* Chip select 4 for MiniPIM 3 */
    { BOOT_CE_4_L,
      MIO_BOOT_REG_CFG_EN     |      /* Enable bit */
      MIO_BOOT_REG_CFG_WIDTH  |      /* Width - 8 bit */
      MIO_BOOT_REG_CFG_SIZE   |      /* Size */
      SRX240_MPIM3_BASE,             /* Base Address */
      MIO_BOOT_REG_TIM_RESET_VAL     /* TIM Reset Value */
    },
    /* Chip select 5 for MiniPIM 4 */
    { BOOT_CE_5_L,
      MIO_BOOT_REG_CFG_EN     |      /* Enable bit */
      MIO_BOOT_REG_CFG_WIDTH  |      /* Width - 8 bit */
      MIO_BOOT_REG_CFG_SIZE   |      /* Size */
      SRX240_MPIM4_BASE,             /* Base Address */
      MIO_BOOT_REG_TIM_RESET_VAL     /* TIM Reset Value */
    },
    { BOOT_CE_LAST, 0x00, 0x00 }
};

static jsrxnle_mpim_boot_reg_cfg_t mpim_boot_reg_cfg_srx550[] = {
    /* Chip select 2 for MiniPIM 1 */
    { BOOT_CE_2_L,
      MIO_BOOT_REG_CFG_EN     |      /* Enable bit */
      MIO_BOOT_REG_CFG_WIDTH  |      /* Width - 8 bit */
      MIO_BOOT_REG_CFG_SIZE_64K   |      /* Size */       
      SRX220_MPIM1_BASE,             /* Base Address */
      MIO_BOOT_REG_TIM_RESET_VAL     /* TIM Reset Value */
    },
    /* Chip select 3 for MiniPIM 2 */
    { BOOT_CE_3_L,
      MIO_BOOT_REG_CFG_EN     |      /* Enable bit */
      MIO_BOOT_REG_CFG_WIDTH  |      /* Width - 8 bit */
      MIO_BOOT_REG_CFG_SIZE_64K   |      /* Size */       
      SRX220_MPIM2_BASE,             /* Base Address */
      MIO_BOOT_REG_TIM_RESET_VAL     /* TIM Reset Value */
    },
    { BOOT_CE_LAST, 0x00, 0x00 }
};
#endif

#ifndef CONFIG_OCTEON_SIM
/******************  Begin u-boot eeprom hooks ******************************/
/* support for u-boot i2c functions is limited to the standard serial eeprom on the board.
** The do not support reading either the MCU or the DIMM eeproms
*/


/**
 * Reads bytes from eeprom and copies to DRAM.
 * Only supports address size of 2 (16 bit internal address.)
 * 
 * @param chip   chip address
 * @param addr   internal address (only 16 bit addresses supported)
 * @param alen   address length, must be 2
 * @param buffer memory buffer pointer
 * @param len    number of bytes to read
 * 
 * @return 0 on Success
 *         1 on Failure
 */
int  i2c_read(uchar chip, uint addr, int alen, uchar *buffer, int len)
{


    if (alen != 2 || !buffer || !len)
        return(1);
    if (octeon_twsi_set_addr16(chip, addr) < 0)
        return(1);

    while (len--)
    {
        int tmp;
        tmp = octeon_twsi_read8_cur_addr(chip);
        if (tmp < 0)
            return(1);
        *buffer++ = (uchar)(tmp & 0xff);
    }

    return(0);

}



/**
 * Reads bytes from memory and copies to eeprom.
 * Only supports address size of 2 (16 bit internal address.)
 * 
 * We can only write two bytes at a time to the eeprom, so in some cases
 * we need to to a read/modify/write.
 * 
 * Note: can't do single byte write to last address in EEPROM
 * 
 * @param chip   chip address
 * @param addr   internal address (only 16 bit addresses supported)
 * @param alen   address length, must be 2
 * @param buffer memory buffer pointer
 * @param len    number of bytes to write
 * 
 * @return 0 on Success
 *         1 on Failure
 */
int  i2c_write(uchar chip, uint addr, int alen, uchar *buffer, int len)
{

    if (alen != 2 || !buffer || !len)
        return(1);

    while (len >= 2)
    {
        if (octeon_twsi_write16(chip, addr, *((uint16_t *)buffer)) < 0)
            return(1);
        len -= 2;
        addr +=2;
        buffer += 2;
    }



    /* Handle single (or last) byte case */
    if (len == 1)
    {
        int tmp;

        /* Read 16 bits at addr */
        tmp = octeon_twsi_read16(chip, addr);
        if (tmp < 0)
            return(1);

        tmp &= 0xff; 
        tmp |= (*buffer) << 8;

        if (octeon_twsi_write16(chip, addr, (uint16_t)tmp) < 0)
            return(1);
    }

    return(0);

}
/*-----------------------------------------------------------------------
 * Probe to see if a chip is present using single byte read.  Also good for checking for the
 * completion of EEPROM writes since the chip stops responding until
 * the write completes (typically 10mSec).
 */
int i2c_probe(uchar addr)
{
    if (octeon_twsi_read8(addr, 0) < 0)
        return(1);
    else
        return(0); /* probed OK */
}

/******************  End u-boot eeprom hooks ******************************/
#endif


#if defined(CONFIG_JSRXNLE) || defined(CONFIG_MAG8600)

int  i2c_read8(uchar chip, uint addr, uchar *buffer)
{
    if (octeon_twsi_set_addr8(chip, addr) < 0)
        return(1);

    int tmp;
    tmp = octeon_twsi_read8_cur_addr(chip);
    if (tmp < 0)
        return(1);
    *buffer = (uchar)(tmp & 0xff);
    return(0);
}

int  i2c_read16(uchar chip, uint addr, uchar *buffer)
{
    int tmp;

    if (!buffer)
        return(-1);

    if (octeon_twsi_set_addr8(chip, addr) < 0) {
        printf("i2c_read16:: Failed to set Addr \n");
        return -1;
    }

    tmp = octeon_twsi_read_cur_addr16(chip);
    if (tmp < 0) {
        printf("i2c_read16:: Failed to read reg \n");
        return -1;
    }
    buffer[1] = (uchar)(tmp & 0xff);
    buffer[0] = (uchar)(tmp >> 8);
    return(0);
}

int  i2c_write8(uchar chip, uint addr, uchar *buffer)
{
    if (octeon_twsi_write8(chip, addr, (uint8_t)buffer[0]) < 0)
        return(1);

    return (0);
}

int  i2c_write16(uchar chip, uint addr, uchar *buffer)
{
    if (octeon_twsi_write_reg16(chip, addr,  *((uint16_t *)buffer)) < 0)
        return(1);

    return (0);
}

#endif



#if (CONFIG_OCTEON_EBT3000 || CONFIG_OCTEON_EBT5800)
int octeon_ebt3000_get_board_major_rev(void)
{
    DECLARE_GLOBAL_DATA_PTR;
    return(gd->board_desc.rev_major);
}
int octeon_ebt3000_get_board_minor_rev(void)
{
    DECLARE_GLOBAL_DATA_PTR;
    return(gd->board_desc.rev_minor);
}
#endif

int octeon_show_info(void)
{
    return (!!(octeon_read_gpio() & OCTEON_GPIO_SHOW_FREQ));
}


#ifdef OCTEON_CHAR_LED_BASE_ADDR
void octeon_led_str_write_std(const char *str)
{
    DECLARE_GLOBAL_DATA_PTR;
    if ((gd->board_desc.board_type == CVMX_BOARD_TYPE_EBT3000) && (gd->board_desc.rev_major == 1))
    {
        char *ptr = (char *)(OCTEON_CHAR_LED_BASE_ADDR + 4);
        int i;
        for (i=0; i<4; i++)
        {
            if (*str)
                ptr[3 - i] = *str++;
            else
                ptr[3 - i] = ' ';
        }
    }
    else
    {
        /* rev 2 ebt3000 or ebt5800 or kodama board */
        char *ptr = (char *)(OCTEON_CHAR_LED_BASE_ADDR | 0xf8);
        int i;
        for (i=0; i<8; i++)
        {
            if (*str)
                ptr[i] = *str++;
            else
                ptr[i] = ' ';
        }
    }
}
#else
void octeon_led_str_write_std(const char *str)
{
    /* empty function for when there is no LED */
}
#endif


static int bist_failures = 0;

int displayErrorReg_1(const char *reg_name, uint64_t addr, uint64_t expected, uint64_t mask)
{
    uint64_t bist_val;
    bist_val = cvmx_read_csr(addr);
    bist_val = (bist_val & mask) ^ expected;
    if (bist_val)
    {
        bist_failures++;
        printf("BIST FAILURE: %s, error bits ((bist_val & mask) ^ expected): 0x%qx\n", reg_name, bist_val);
        return 1;
    }
    return 0;
}

#define COP0_CVMMEMCTL_REG $11,7
int displayErrorRegC0_cvmmem(const char *reg_name, uint64_t expected, uint64_t mask)
{
    uint64_t bist_val;

    {
        uint32_t tmp_low, tmp_hi;
    
        asm volatile (
                   "   .set push                    \n"
                   "   .set mips64                  \n"
                   "   .set noreorder               \n"
                   "   dmfc0 %[tmpl], $11, 7         \n"
                   "   dadd   %[tmph], %[tmpl], $0   \n"
                   "   dsrl  %[tmph], 32            \n"
                   "   dsll  %[tmpl], 32            \n"
                   "   dsrl  %[tmpl], 32            \n"
                   "   .set pop                 \n"
                      : [tmpl] "=&r" (tmp_low), [tmph] "=&r" (tmp_hi) : );
    
        bist_val = (((uint64_t)tmp_hi << 32) | tmp_low);
    }
    bist_val = (bist_val & mask) ^ expected;
    if (bist_val)
    {
        bist_failures++;
        printf("BIST FAILURE: %s, error bits: 0x%qx\n", reg_name, bist_val);
        return 1;
    }
    return 0;
}

int displayErrorRegC0_Icache(const char *reg_name, uint64_t expected, uint64_t mask)
{
    uint64_t bist_val;
    {
        uint32_t tmp_low, tmp_hi;

        asm volatile (
                   "   .set push                    \n"
                   "   .set mips64                  \n"
                   "   .set noreorder               \n"
                   "   dmfc0 %[tmpl], $27, 0         \n"
                   "   dadd   %[tmph], %[tmpl], $0   \n"
                   "   dsrl  %[tmph], 32            \n"
                   "   dsll  %[tmpl], 32            \n"
                   "   dsrl  %[tmpl], 32            \n"
                   "   .set pop                     \n"
                      : [tmpl] "=&r" (tmp_low), [tmph] "=&r" (tmp_hi) : );

        bist_val = (((uint64_t)tmp_hi << 32) | tmp_low);
    }
    bist_val = (bist_val & mask) ^ expected;
    if (bist_val)
    {
        printf("BIST FAILURE: %s, error bits: 0x%llx\n", reg_name, bist_val);
        return(1);
    }
    return(0);
}

int displayErrorRegC0_cache(const char *reg_name, uint64_t expected, uint64_t mask)
{
    uint64_t bist_val;
    {
        uint32_t tmp_low, tmp_hi;
    
        asm volatile (
                   "   .set push                    \n"
                   "   .set mips64                  \n"
                   "   .set noreorder               \n"
                   "   dmfc0 %[tmpl], $27, 0         \n"
                   "   dadd   %[tmph], %[tmpl], $0   \n"
                   "   dsrl  %[tmph], 32            \n"
                   "   dsll  %[tmpl], 32            \n"
                   "   dsrl  %[tmpl], 32            \n"
                   "   .set pop                     \n"
                      : [tmpl] "=&r" (tmp_low), [tmph] "=&r" (tmp_hi) : );
    
        bist_val = (((uint64_t)tmp_hi << 32) | tmp_low);
    }
    bist_val = (bist_val & mask) ^ expected;
    if (bist_val)
    {
        bist_failures++;
        printf("BIST FAILURE: %s, error bits: 0x%qx\n", reg_name, bist_val);
        return 1;
    }
    return 0;
}

/* We want a mask that is all 1's in bit positions not covered
 * by the coremask once it is shifted.
 */
static inline uint64_t make_cm_mask(uint64_t cm, int shift)
{
    uint64_t cm_override_mask;
    if (OCTEON_IS_MODEL(OCTEON_CN63XX))
        cm_override_mask = 0x3f;
    else
    {
        printf("ERROR: unsupported OCTEON model\n");
        return ~0ull;
    }

    return ((~0ull & ~(cm_override_mask << shift)) | (cm << shift));
}

static int octeon_bist_7XXX(void)
{
	int bist_failures = 0;
	int i;

	bist_failures +=
	    displayErrorRegC0_cvmmem("COP0_CVMMEMCTL_REG", 0ull,
				     0xff00000000000000ull);

	bist_failures +=
	    displayErrorRegC0_Icache("COP0_ICACHEERR_REG",
				     0,
				     0x35ull << 32);

	bist_failures +=
	    displayErrorReg_1("CVMX_AGL_GMX_BIST", CVMX_AGL_GMX_BIST, 0, ~0ull);
	bist_failures +=
	    displayErrorReg_1("CVMX_BCH_BIST_RESULT", CVMX_BCH_BIST_RESULT,
			      0, ~0ull);
	bist_failures +=
	    displayErrorReg_1("CVMX_CIU_BIST", CVMX_CIU_BIST, 0, ~0ull);
	bist_failures +=
	    displayErrorReg_1("CVMX_DFA_BIST0", CVMX_DFA_BIST0, 0, ~0ull);
	bist_failures +=
	    displayErrorReg_1("CVMX_DFA_BIST1", CVMX_DFA_BIST1, 0, ~0ull);
	bist_failures +=
	    displayErrorReg_1("CVMX_DPI_BIST_STATUS", CVMX_DPI_BIST_STATUS,
			      0, ~0ull);
	bist_failures +=
	    displayErrorReg_1("CVMX_FPA_BIST_STATUS", CVMX_FPA_BIST_STATUS, 0,
			      ~0ull);
	bist_failures +=
	    displayErrorReg_1("CVMX_IPD_BIST_STATUS", CVMX_IPD_BIST_STATUS, 0,
			      ~0ull);
	for (i = 0; i < 2; i++) {
		bist_failures +=
	 		displayErrorReg_1("CVMX_GMXX_BIST(i)", CVMX_GMXX_BIST(i),
					0, ~0ull);
		bist_failures +=
			displayErrorReg_1("CVMX_PCSXX_BIST_STATUS_REG(i)",
					CVMX_PCSXX_BIST_STATUS_REG(i), 0, ~0ull);
	}
	bist_failures +=
	    displayErrorReg_1("CVMX_IOB_BIST_STATUS", CVMX_IOB_BIST_STATUS, 0,
			      ~0ull);
	bist_failures +=
	    displayErrorReg_1("CVMX_IPD_BIST_STATUS", CVMX_IPD_BIST_STATUS, 0,
			      ~0ull);
	bist_failures +=
	    displayErrorReg_1("CVMX_KEY_BIST_REG", CVMX_KEY_BIST_REG, 0, ~0ull);
	bist_failures +=
	    displayErrorReg_1("CVMX_L2C_CBCX_BIST_STATUS(0)",
				CVMX_L2C_CBCX_BIST_STATUS(0), 0, ~0ull);
	bist_failures +=
	    displayErrorReg_1("CVMX_L2C_TBFX_BIST_STATUS(0)",
				CVMX_L2C_TBFX_BIST_STATUS(0), 0, ~0ull);
	bist_failures +=
	    displayErrorReg_1("CVMX_L2C_TDTX_BIST_STATUS(0)",
				CVMX_L2C_TDTX_BIST_STATUS(0), 0, ~0ull);
	bist_failures +=
	    displayErrorReg_1("CVMX_L2C_TTGX_BIST_STATUS(0)",
				CVMX_L2C_TTGX_BIST_STATUS(0), 0, ~0ull);
	bist_failures +=
	    displayErrorReg_1("CVMX_MIO_BOOT_BIST_STAT",
				CVMX_MIO_BOOT_BIST_STAT, 0, ~0ull);
	for (i = 0; i < 3; i++) {
		if (OCTEON_IS_MODEL(OCTEON_CN70XX_PASS1_X))
			break;
		bist_failures +=
	 		displayErrorReg_1("CVMX_PEMX_BIST_STATUS(i)",
				CVMX_PEMX_BIST_STATUS(i), 0, ~0ull);
		bist_failures +=
	 		displayErrorReg_1("CVMX_PEMX_BIST_STATUS2(i)",
				CVMX_PEMX_BIST_STATUS2(i), 0, ~0ull);
	}
	bist_failures +=
	    displayErrorReg_1("CVMX_PIP_BIST_STATUS", CVMX_PIP_BIST_STATUS, 0,
			      ~0ull);
	bist_failures +=
	    displayErrorReg_1("CVMX_PKO_REG_BIST_RESULT",
				CVMX_PKO_REG_BIST_RESULT, 0, ~0ull);
	bist_failures +=
	    displayErrorReg_1("CVMX_RAD_REG_BIST_RESULT",
				CVMX_RAD_REG_BIST_RESULT, 0, ~0ull);
	bist_failures +=
	    displayErrorReg_1("CVMX_RNM_BIST_STATUS", CVMX_RNM_BIST_STATUS, 0,
			      ~0ull);

	bist_failures +=
	    displayErrorReg_1("CVMX_POW_BIST_STAT", CVMX_POW_BIST_STAT, 0,
			      ~0ull);
	return 0;
}

int octeon_bist_63XX(void)
{
    /* Mask POW BIST_STAT with coremask so we don't report errors on known bad cores  */
    char *cm_str = getenv("coremask_override");
    uint64_t cm_override = ~0ull;
    int bist_failures = 0;

    bist_failures += displayErrorRegC0_cvmmem( "COP0_CVMMEMCTL_REG",       0ull,                        0xfc00000000000000ull );

    bist_failures += displayErrorRegC0_Icache( "COP0_ICACHEERR_REG",     0x1ffull << 40 | 0x1ffull << 52,       0x1ffull << 40 | 0x1ffull << 52);
    if (cm_str)
        cm_override = simple_strtoul(cm_str, NULL, 0);

    bist_failures += displayErrorReg_1  (    "POW_BIST_STAT",           CVMX_POW_BIST_STAT,        0ull,                        make_cm_mask(cm_override, 16));


    bist_failures += displayErrorReg_1  ( "CVMX_AGL_GMX_BIST",     CVMX_AGL_GMX_BIST,     0,                    ~0ull );
    bist_failures += displayErrorReg_1  ( "CVMX_CIU_BIST",     CVMX_CIU_BIST,     0,                    ~0ull );
    bist_failures += displayErrorReg_1  ( "CVMX_DFA_BIST0",     CVMX_DFA_BIST0,     0,                  ~0ull );
    bist_failures += displayErrorReg_1  ( "CVMX_DFA_BIST1",     CVMX_DFA_BIST1,     0,                  ~0ull );
    bist_failures += displayErrorReg_1  ( "CVMX_DPI_BIST_STATUS",     CVMX_DPI_BIST_STATUS,     0,                      ~0ull );
    bist_failures += displayErrorReg_1  ( "CVMX_FPA_BIST_STATUS",     CVMX_FPA_BIST_STATUS,     0,                      ~0ull );
    bist_failures += displayErrorReg_1  ( "CVMX_GMXX_BIST0",     CVMX_GMXX_BIST(0),     0,                      ~0ull );
    bist_failures += displayErrorReg_1  ( "CVMX_IOB_BIST_STATUS",     CVMX_IOB_BIST_STATUS,     0,                      ~0ull );
    bist_failures += displayErrorReg_1  ( "CVMX_IPD_BIST_STATUS",     CVMX_IPD_BIST_STATUS,     0,                      ~0ull );
    bist_failures += displayErrorReg_1  ( "CVMX_KEY_BIST_REG",     CVMX_KEY_BIST_REG,     0,                    ~0ull );
    bist_failures += displayErrorReg_1  ( "CVMX_MIO_BOOT_BIST_STAT",     CVMX_MIO_BOOT_BIST_STAT,     0,                        ~0ull );
    bist_failures += displayErrorReg_1  ( "CVMX_MIXX_BIST0",     CVMX_MIXX_BIST(0),     0,                      ~0ull );
    bist_failures += displayErrorReg_1  ( "CVMX_MIXX_BIST1",     CVMX_MIXX_BIST(1),     0,                      ~0ull );
    
    bist_failures += displayErrorReg_1  ( "CVMX_PEXP_SLI_BIST_STATUS",     CVMX_PEXP_SLI_BIST_STATUS,     0,                    ~0ull );
    bist_failures += displayErrorReg_1  ( "CVMX_PIP_BIST_STATUS",     CVMX_PIP_BIST_STATUS,     0,                      ~0ull );
    bist_failures += displayErrorReg_1  ( "CVMX_PKO_REG_BIST_RESULT",     CVMX_PKO_REG_BIST_RESULT,     0,                      ~0ull );
    bist_failures += displayErrorReg_1  ( "CVMX_RAD_REG_BIST_RESULT",     CVMX_RAD_REG_BIST_RESULT,     0,                      ~0ull );
    bist_failures += displayErrorReg_1  ( "CVMX_RNM_BIST_STATUS",     CVMX_RNM_BIST_STATUS,     0,                      ~0ull );
    bist_failures += displayErrorReg_1  ( "CVMX_TIM_REG_BIST_RESULT",     CVMX_TIM_REG_BIST_RESULT,     0,                      ~0ull );
    bist_failures += displayErrorReg_1  ( "CVMX_TRA_BIST_STATUS",     CVMX_TRA_BIST_STATUS,     0,                      ~0ull );
    bist_failures += displayErrorReg_1  ( "CVMX_ZIP_CMD_BIST_RESULT",     CVMX_ZIP_CMD_BIST_RESULT,     0,                      ~0ull );


    bist_failures += displayErrorReg_1  ( "CVMX_L2C_BST",     CVMX_L2C_BST,     0,                      make_cm_mask(cm_override, 32));
    bist_failures += displayErrorReg_1  ( "CVMX_L2C_BST_MEMX(0)",     CVMX_L2C_BST_MEMX(0),     0,                      0x1f );
    bist_failures += displayErrorReg_1  ( "CVMX_L2C_BST_TDTX(0)",     CVMX_L2C_BST_TDTX(0),     0,                      ~0ull );
    bist_failures += displayErrorReg_1  ( "CVMX_L2C_BST_TTGX(0)",     CVMX_L2C_BST_TTGX(0),     0,                      ~0ull );

    if (bist_failures)
    {
        printf("BIST Errors found (%d).\n", bist_failures);
        printf("'1' bits above indicate unexpected BIST status.\n");
    }
    else
    {
        printf("BIST check passed.\n");
    }
    return 0;
}

int octeon_bist()
{

    if (octeon_is_model(OCTEON_CN63XX))
        return(octeon_bist_63XX());

    if (octeon_is_model(OCTEON_CN71XX))
        return octeon_bist_7XXX();

    displayErrorRegC0_cvmmem( "COP0_CVMMEMCTL_REG",       0ull,			0xfc00000000000000ull );
    displayErrorRegC0_cache( "COP0_CACHEERR_REG",      0x0ull,	0x1ull <<32 | 0x1ull <<33 );
    /*                    NAME                   REGISTER              EXPECTED            MASK   */
    displayErrorReg_1  ( "GMX0_BIST",           CVMX_GMXX_BIST(0),        0ull,			0xffffffffffffffffull );
    if (!octeon_is_model(OCTEON_CN31XX) && !octeon_is_model(OCTEON_CN30XX) && !octeon_is_model(OCTEON_CN50XX)
        && !octeon_is_model(OCTEON_CN52XX)
        )
        displayErrorReg_1  ( "GMX1_BIST",           CVMX_GMXX_BIST(1),        0ull,			0xffffffffffffffffull );
    displayErrorReg_1  ( "IPD_BIST_STATUS",     CVMX_IPD_BIST_STATUS,     0ull,			0xffffffffffffffffull );
    if (!octeon_is_model(OCTEON_CN31XX) && !octeon_is_model(OCTEON_CN30XX) && !octeon_is_model(OCTEON_CN50XX) && !octeon_is_model(OCTEON_CN52XX))
        displayErrorReg_1  ( "KEY_BIST_REG",        CVMX_KEY_BIST_REG,        0ull,			0xffffffffffffffffull );
    displayErrorReg_1  ( "L2D_BST0",            CVMX_L2D_BST0,            0ull,		    1ull<<34 );
    displayErrorReg_1  ( "L2C_BST0",            CVMX_L2C_BST0,            0,			0x1full );
    displayErrorReg_1  ( "L2C_BST1",            CVMX_L2C_BST1,            0,			0xffffffffffffffffull );
    displayErrorReg_1  ( "L2C_BST2",            CVMX_L2C_BST2,            0,			0xffffffffffffffffull );
    displayErrorReg_1  ( "CIU_BIST",            CVMX_CIU_BIST,            0,			0xffffffffffffffffull );
    displayErrorReg_1  ( "PKO_REG_BIST_RESULT", CVMX_PKO_REG_BIST_RESULT, 0,			0xffffffffffffffffull );
    if (!octeon_is_model(OCTEON_CN56XX)) {
        displayErrorReg_1  ( "NPI_BIST_STATUS",     CVMX_NPI_BIST_STATUS,     0,			0xffffffffffffffffull );
    }
    if ((!octeon_is_model(OCTEON_CN56XX)) && (!octeon_is_model(OCTEON_CN56XX))) {
        displayErrorReg_1  ( "PIP_BIST_STATUS",     CVMX_PIP_BIST_STATUS,     0,			0xffffffffffffffffull );
    }
    /* Mask POW BIST_STAT with coremask so we don't report errors on known bad cores  */
    uint32_t val = (uint32_t)cvmx_read_csr(CVMX_POW_BIST_STAT);
    char *cm_str = getenv("coremask_override");
    if (cm_str)
    {
        uint32_t cm_override = simple_strtoul(cm_str, NULL, 0);
        /* Shift coremask override to match up with core BIST bits in register */
        cm_override = cm_override << 16;
        val &= cm_override;
    }
    if (val)
    {
        printf("BIST FAILURE: POW_BIST_STAT: 0x%08x\n", val);
        bist_failures++;
    }

    displayErrorReg_1  ( "RNM_BIST_STATUS",     CVMX_RNM_BIST_STATUS,     0,			0xffffffffffffffffull );
    if (!octeon_is_model(OCTEON_CN31XX) && !octeon_is_model(OCTEON_CN30XX) && !octeon_is_model(OCTEON_CN56XX)
        && !octeon_is_model(OCTEON_CN50XX) && !octeon_is_model(OCTEON_CN52XX)
        )
    {
        displayErrorReg_1  ( "SPX0_BIST_STAT",      CVMX_SPXX_BIST_STAT(0),   0,			0xffffffffffffffffull );
        displayErrorReg_1  ( "SPX1_BIST_STAT",      CVMX_SPXX_BIST_STAT(1),   0,			0xffffffffffffffffull );
    }
    displayErrorReg_1  ( "TIM_REG_BIST_RESULT", CVMX_TIM_REG_BIST_RESULT, 0,			0xffffffffffffffffull );
    if (!octeon_is_model(OCTEON_CN30XX) && !octeon_is_model(OCTEON_CN50XX))
        displayErrorReg_1  ( "TRA_BIST_STATUS",     CVMX_TRA_BIST_STATUS,     0,			0xffffffffffffffffull );
    displayErrorReg_1  ( "MIO_BOOT_BIST_STAT",  CVMX_MIO_BOOT_BIST_STAT,  0,			0xffffffffffffffffull );
    displayErrorReg_1  ( "IOB_BIST_STATUS",     CVMX_IOB_BIST_STATUS,     0,			0xffffffffffffffffull );
    if (!octeon_is_model(OCTEON_CN30XX) && !octeon_is_model(OCTEON_CN56XX)
        && !octeon_is_model(OCTEON_CN50XX) && !octeon_is_model(OCTEON_CN52XX)
        )
    {
        displayErrorReg_1  ( "DFA_BST0",            CVMX_DFA_BST0,            0,			0xffffffffffffffffull );
        displayErrorReg_1  ( "DFA_BST1",            CVMX_DFA_BST1,            0,			0xffffffffffffffffull );
    }
    displayErrorReg_1  ( "FPA_BIST_STATUS",     CVMX_FPA_BIST_STATUS,     0,			0xffffffffffffffffull );
    if (!octeon_is_model(OCTEON_CN30XX) && !octeon_is_model(OCTEON_CN50XX)&& !octeon_is_model(OCTEON_CN52XX))
        displayErrorReg_1  ( "ZIP_CMD_BIST_RESULT", CVMX_ZIP_CMD_BIST_RESULT, 0,			0xffffffffffffffffull );

    if (!octeon_is_model(OCTEON_CN38XX) && !octeon_is_model(OCTEON_CN58XX) && !octeon_is_model(OCTEON_CN52XX))
    {
        displayErrorReg_1("USBNX_BIST_STATUS(0)", CVMX_USBNX_BIST_STATUS(0), 0 , ~0ull);
    }
    if ((octeon_is_model(OCTEON_CN56XX)) || (octeon_is_model(OCTEON_CN52XX)))
    {
        displayErrorReg_1("AGL_GMX_BIST", CVMX_AGL_GMX_BIST, 0 , ~0ull);
        displayErrorReg_1("IOB_BIST_STATUS", CVMX_IOB_BIST_STATUS, 0 , ~0ull);
        displayErrorReg_1("MIXX_BIST(0)", CVMX_MIXX_BIST(0), 0 , ~0ull);
        displayErrorReg_1("PEXP_NPEI_BIST_STATUS", CVMX_PEXP_NPEI_BIST_STATUS, 0 , ~0ull);
        displayErrorReg_1("RAD_REG_BIST_RESULT", CVMX_RAD_REG_BIST_RESULT, 0 , ~0ull);
        displayErrorReg_1("RNM_BIST_STATUS", CVMX_RNM_BIST_STATUS, 0 , ~0ull);
    }
    if (octeon_is_model(OCTEON_CN56XX))
    {
        displayErrorReg_1("PCSX0_BIST_STATUS_REG(0)", CVMX_PCSXX_BIST_STATUS_REG(0), 0 , ~0ull);
        displayErrorReg_1("PCSX1_BIST_STATUS_REG(1)", CVMX_PCSXX_BIST_STATUS_REG(1), 0 , ~0ull);
    }
    if (bist_failures)
    {
        printf("BIST Errors found.\n");
        printf("'1' bits above indicate unexpected BIST status.\n");
    }
    else
    {
        printf("BIST check passed.\n");
    }
    return(0);
}

void octeon_flush_l2_cache(void)
{
    uint64_t assoc, set;
    cvmx_l2c_dbg_t l2cdbg;

    int l2_sets = 0;
    int l2_assoc = 0;
    if (octeon_is_model(OCTEON_CN30XX))
    {
        l2_sets  = 256;
        l2_assoc = 4;
    }
    else if (octeon_is_model(OCTEON_CN31XX))
    {
        l2_sets  = 512;
        l2_assoc = 4;
    }
    else if (octeon_is_model(OCTEON_CN38XX))
    {
        l2_sets  = 1024;
        l2_assoc = 8;
    }
    else if (octeon_is_model(OCTEON_CN58XX))
    {
        l2_sets  = 2048;
        l2_assoc = 8;
    }
    else if (octeon_is_model(OCTEON_CN56XX))
    {
        l2_sets  = 2048;
        l2_assoc = 8;
    }
    else if (octeon_is_model(OCTEON_CN52XX))
    {
        l2_sets  = 512;
        l2_assoc = 8;
    }
    else if (octeon_is_model(OCTEON_CN50XX))
    {
        l2_sets  = 128;
        l2_assoc = 8;
    }
    else
    {
        printf("ERROR: unsupported Octeon model\n");
    }


    l2cdbg.u64 = 0;
    l2cdbg.s.ppnum = get_core_num();
    l2cdbg.s.finv = 1;
    for(set=0; set < l2_sets; set++)
    {
        for(assoc = 0; assoc < l2_assoc; assoc++)
        {
            l2cdbg.s.set = assoc;
            /* Enter debug mode, and make sure all other writes complete before we
            ** enter debug mode */
            OCTEON_SYNC;
            cvmx_write_csr(CVMX_L2C_DBG, l2cdbg.u64);
            cvmx_read_csr(CVMX_L2C_DBG);

            CVMX_PREPARE_FOR_STORE (1ULL << 63 | set*CVMX_CACHE_LINE_SIZE, 0);
            /* Exit debug mode */
            OCTEON_SYNC;
            cvmx_write_csr(CVMX_L2C_DBG, 0);
            cvmx_read_csr(CVMX_L2C_DBG);
        }
    }

}


int octeon_check_mem_errors(void)
{
    /* Check for DDR/L2C memory errors */
    uint64_t val;
    int errors = 0;
    int ddr_ecc_errors = 0;

    val = cvmx_read_csr(CVMX_L2D_ERR);
    if (val & 0x18ull)
    {
        printf("WARNING: L2D ECC errors detected!\n");
        cvmx_write_csr(CVMX_L2D_ERR, val);
        errors++;
    }
    val = cvmx_read_csr(CVMX_L2T_ERR);
    if (val & 0x18ull)
    {
        printf("WARNING: L2T ECC errors detected!\n");
        cvmx_write_csr(CVMX_L2T_ERR, val);
        errors++;
    }

    if (octeon_is_model(OCTEON_CN56XX)) {
	/* The cn56xx has multiple ddr interfaces that can be enabled
	** individually. Confirm that the interface is enabled before
	** checking for ECC errors.  Otherwise the read will timeout
	** with a bus error. */
        cvmx_l2c_cfg_t l2c_cfg;
        l2c_cfg.u64 = cvmx_read_csr(CVMX_L2C_CFG);
        if (l2c_cfg.cn56xx.dpres0) {
	    val = cvmx_read_csr(CVMX_LMCX_MEM_CFG0(0));
	    if (val & (0xFFull << 21)) {
		cvmx_write_csr(CVMX_LMCX_MEM_CFG0(0), val);
		errors++;
		++ddr_ecc_errors;
	    }
        }
        if (l2c_cfg.cn56xx.dpres1) {
	    val = cvmx_read_csr(CVMX_LMCX_MEM_CFG0(1));
	    if (val & (0xFFull << 21)) {
            cvmx_write_csr(CVMX_LMCX_MEM_CFG0(1), val);
		errors++;
		++ddr_ecc_errors;
	    }
        }
    } else {
	val = cvmx_read_csr(CVMX_LMC_MEM_CFG0);
	if (val & (0xFFull << 21))
	{
	    cvmx_write_csr(CVMX_LMC_MEM_CFG0, val);
	    errors++;
	    ++ddr_ecc_errors;
	}
    }

    if (ddr_ecc_errors) {
	printf("WARNING: DDR ECC errors detected!\n");
    }
    return(errors);
}

static void
octeon_srx220_bootreg_timings (void)
{
    cvmx_mio_boot_reg_timx_t __attribute__((unused)) reg_tim;
    
    reg_tim.u64 = 0;
    reg_tim.s.pagem = 0;
    reg_tim.s.waitm = 0;
    reg_tim.s.pages = 0;
    reg_tim.s.ale = 0x04;
    reg_tim.s.page = 0x3F;
    reg_tim.s.wait = 0x30;
    reg_tim.s.pause = 0x1A;
    reg_tim.s.wr_hld = 0x01;
    reg_tim.s.rd_hld = 0x01;
    reg_tim.s.we = 0x2A;
    reg_tim.s.oe = 0x2A;
    reg_tim.s.ce = 0x01;
    reg_tim.s.adr = 0x01;
    
    /* Set timing values for CE1/CPLD. */
    cvmx_write_csr(CVMX_MIO_BOOT_REG_TIMX(1), reg_tim.u64);

    reg_tim.u64 = 0;
    reg_tim.s.pagem = 0;
    reg_tim.s.waitm = 0;
    reg_tim.s.pages = 0;
    reg_tim.s.ale = 0x04;
    reg_tim.s.page = 0x3F;
    reg_tim.s.wait = 0x30;
    reg_tim.s.pause = 0x1A;
    reg_tim.s.wr_hld = 0x10;
    reg_tim.s.rd_hld = 0x10;
    reg_tim.s.we = 0x2A;
    reg_tim.s.oe = 0x2A;
    reg_tim.s.ce = 0x1A;
    reg_tim.s.adr = 0x01;
    
    /* Set timing values for MPIM1 */
    cvmx_write_csr(CVMX_MIO_BOOT_REG_TIMX(2), reg_tim.u64);

    /* Set timing values for MPIM1 */
    cvmx_write_csr(CVMX_MIO_BOOT_REG_TIMX(3), reg_tim.u64);

    /* Set timing values for VOICE */
    cvmx_write_csr(CVMX_MIO_BOOT_REG_TIMX(4), reg_tim.u64);
}

static void
octeon_srx550_bootreg_timings (void)
{
    cvmx_mio_boot_reg_timx_t __attribute__((unused)) reg_tim;
    
    reg_tim.u64 = 0;
    reg_tim.s.pagem = 0;
    reg_tim.s.waitm = 0;
    reg_tim.s.pages = 0;
    reg_tim.s.ale = 0x04;
    reg_tim.s.page = 0x3F;
    reg_tim.s.wait = 0x30;
    reg_tim.s.pause = 0x18;
    reg_tim.s.wr_hld = 0x02;
    reg_tim.s.rd_hld = 0x02;
    reg_tim.s.we = 0x26;
    reg_tim.s.oe = 0x26;
    reg_tim.s.ce = 0x2;
    reg_tim.s.adr = 0x02;
    
    /* Set timing values for CE1/CPLD. */
    cvmx_write_csr(CVMX_MIO_BOOT_REG_TIMX(1), reg_tim.u64);
}

#ifndef CONFIG_OCTEON_SIM
/* Boot bus init for flash and peripheral access */
#define FLASH_RoundUP(_Dividend, _Divisor) (((_Dividend)+(_Divisor))/(_Divisor))
#undef ECLK_PERIOD
#ifndef CONFIG_MAG8600
#define ECLK_PERIOD	(1000)	/* eclk period (psecs), support speeds up to 1GHz */
#endif
flash_info_t flash_info[CFG_MAX_FLASH_BANKS];	/* info for FLASH chips */

int octeon_boot_bus_init(void)
{
    cvmx_mio_boot_reg_cfgx_t reg_cfg;
    cvmx_mio_boot_reg_timx_t __attribute__((unused)) reg_tim;
    uint64_t eclk_mhz;
    uint64_t eclk_period;
    uint8_t i = 0;
#ifdef CONFIG_MAG8600
    uint64_t ECLK_PERIOD;
#endif   
    DECLARE_GLOBAL_DATA_PTR;

    uint16_t jdec_code;
    uint8_t  ideeprom_format_version;
    uint16_t default_board_type;
    u_int64_t data;
    u_int16_t cpu_50xx_freq_multiplier;
    u_int8_t cpu_ref = 50;

    if (octeon_is_model(OCTEON_CN50XX)) {
        data = cvmx_read_csr((uint64_t)CN5020_DBG_DATA_REG);
        data = data >> 18;
        data &= 0x1f;
        cpu_50xx_freq_multiplier = data;
        data *= cpu_ref;
        data += 10;   /* Give us some margin  */
        eclk_mhz = data;
        eclk_period = 1000000 / eclk_mhz; /* eclk period (psecs) */
    } else if (octeon_is_model(OCTEON_CN63XX)) {
        data = cvmx_read_csr((uint64_t)CN6300_DBG_DATA_REG);
        data = ((data >> 24) & 0x3f);
        data *= cpu_ref; 
        data += 10;   /* Give us some margin  */
        eclk_mhz = data;
        eclk_period = 1000000 / eclk_mhz; /* eclk period (psecs) */
    } else if (octeon_is_model(OCTEON_CN71XX)) {
        cvmx_rst_boot_t rst_boot;
        rst_boot.u64 = cvmx_read_csr(CVMX_RST_BOOT);
        eclk_mhz = cpu_ref * rst_boot.s.pnr_mul;
    } else {
        data = cvmx_read_csr((uint64_t)CN5600_DBG_DATA_REG);
        data = data >> 18ull;
        data &= 0x1f;
        data *= cpu_ref; 
        eclk_mhz = data;
        eclk_period = 1000000 / eclk_mhz; /* eclk period (psecs) */
    }
#ifdef CONFIG_MAG8600
    ECLK_PERIOD = eclk_period;
#endif
    /*
     * Set the TWSI clock to a conservative 100KHz.  Compute the
     * clocks M divider based on the core clock.
     *
     * TWSI freq = (core freq) / (20 x (M+1) x (thp+1) x 2^N)
     * M = ((core freq) / (20 x (TWSI freq) x (thp+1) x 2^N)) - 1
     */

#ifndef TWSI_BUS_FREQ
#define TWSI_BUS_FREQ   (100000) /* 100KHz */
#endif
#ifndef TWSI_THP
#define TWSI_THP            (24) /* TCLK half period (default 24) */
#endif

    int M_divider, N_divider;
    for (N_divider = 0; N_divider < 8; ++N_divider) {
        M_divider = ((eclk_mhz * 1000000) / (20 * TWSI_BUS_FREQ * (TWSI_THP+1) * (1<<N_divider))) - 1;
        if (M_divider < 16) break;
    }

    cvmx_mio_tws_sw_twsi_t sw_twsi;
    sw_twsi.u64 = 0;
    sw_twsi.s.v = 1;
    sw_twsi.s.op = 0x6;
    sw_twsi.s.eop_ia = 0x3;
    sw_twsi.s.d = ((M_divider&0xf) << 3) | ((N_divider&0x7) << 0);
#if defined(CONFIG_ACX500_SVCS)
    cvmx_write_csr(CVMX_MIO_TWSX_SW_TWSI(1), sw_twsi.u64);
#else
    cvmx_write_csr(CVMX_MIO_TWSX_SW_TWSI(0), sw_twsi.u64);
#endif
       
#ifdef CFG_FLASH_SIZE
    /* Remap flash part so that it is all addressable on boot bus, with alias
    ** at 0x1fc00000 so that the data mapped at the default location (0x1fc00000) is
    ** still available at the same address */
    reg_cfg.u64 = 0;
    reg_cfg.s.en = 1;
#if CONFIG_OCTEON_HIKARI
    reg_cfg.s.ale = 1;
#endif

    memset((void *)&(gd->board_desc), 0x0, sizeof(octeon_eeprom_board_desc_t));

#ifdef CONFIG_JSRXNLE

    /* we don't know yet if we are an asgard or valhalla board */
    /* unfortunately ID EEPROM has different address on asgard and valhalla */
    /* so first we look at the CPU id and assume a default board type.*/
    /* Based on cpu type  we read the ID EEPROM for exact board type.*/
    /* God Bless Hardware guys for this..!!!! */
 
    if( octeon_is_model(OCTEON_CN56XX) ){
        default_board_type = I2C_ID_JSRXNLE_SRX650_CHASSIS;
        /* Send booted signal to PLD */
        srx650_send_booted_signal();
        /* Wait for 200ms which is more that the 128ms time PLD wait
         * before it decides the mastership. And before mastership is 
         * decided EEPROM cannot be accessed.
         */
        udelay(200000);
        srx650_wait_for_master();
    } 
    else if(octeon_is_model(OCTEON_CN63XX)) {
        default_board_type = I2C_ID_SRX550_CHASSIS;
    }
    else if(octeon_is_model(OCTEON_CN52XX)) {
        default_board_type = I2C_ID_JSRXNLE_SRX240_LOWMEM_CHASSIS;
    } else {
           if (cpu_50xx_freq_multiplier == SRX220_CPU_CORE_FREQ_MULTIPLIER) {
               default_board_type = I2C_ID_JSRXNLE_SRX220_HIGHMEM_CHASSIS;
           } else {
               default_board_type = I2C_ID_JSRXNLE_SRX210_LOWMEM_CHASSIS;
           }
    }
    
    gd->board_desc.board_type = default_board_type;

    jdec_code = 
        ((uint16_t)jsrxnle_read_eeprom(JSRXNLE_EEPROM_JDEC_OFFSET) << 8)
        | jsrxnle_read_eeprom(JSRXNLE_EEPROM_JDEC_OFFSET + 1);
    ideeprom_format_version = 
        jsrxnle_read_eeprom(JSRXNLE_EEPROM_FORMAT_VERSION_OFFSET);
    
    if (jdec_code == JUNIPER_JDEC_CODE 
        && ((ideeprom_format_version == JUNIPER_EEPROM_FORMAT_V1)
            || (ideeprom_format_version == JUNIPER_EEPROM_FORMAT_V2))) {
        gd->board_desc.board_type = 
         ((uint16_t)jsrxnle_read_eeprom(JSRXNLE_EEPROM_I2CID_OFFSET) << 8)
         | jsrxnle_read_eeprom(JSRXNLE_EEPROM_I2CID_OFFSET + 1);

        /*
         * This is for handling the case where the ID EEPROM values 
         * are wrong. we do our best to come up so that we can write 
         * the correct EEPROM values once u-boot is up
         */
        if ((!IS_PLATFORM_SRX650(gd->board_desc.board_type) && 
             IS_PLATFORM_SRX650(default_board_type)) ||
            (!IS_PLATFORM_SRX550(gd->board_desc.board_type) && 
             IS_PLATFORM_SRX550(default_board_type)) ||
            (!IS_PLATFORM_SRX240(gd->board_desc.board_type) &&
             IS_PLATFORM_SRX240(default_board_type)) ||
            (!IS_PLATFORM_SRX100(gd->board_desc.board_type) &&
             !IS_PLATFORM_SRX110(gd->board_desc.board_type) &&
             !IS_PLATFORM_SRX210(gd->board_desc.board_type) &&
             !IS_PLATFORM_SRX220(gd->board_desc.board_type) &&
             IS_PLATFORM_SRX210(default_board_type))) {
            gd->flags |= GD_FLG_BOARD_EEPROM_READ_FAILED;
            gd->board_desc.rev_minor = 0;
            gd->board_desc.board_type = default_board_type;
            gd->board_desc.rev_major = 1;
        } else {
            gd->board_desc.rev_major = 
                jsrxnle_read_eeprom(JSRXNLE_EEPROM_MAJOR_REV_OFFSET);
            gd->board_desc.rev_minor = 
                jsrxnle_read_eeprom(JSRXNLE_EEPROM_MINOR_REV_OFFSET);
        }
    }
    else {
        gd->flags |= GD_FLG_BOARD_EEPROM_READ_FAILED;
        gd->board_desc.rev_minor = 0;
        gd->board_desc.board_type = default_board_type;
        gd->board_desc.rev_major = 1;
    }

    switch (gd->board_desc.board_type) {
    CASE_ALL_SRXSME_4MB_FLASH_MODELS
        reg_cfg.s.size = ((CFG_FLASH_SIZE) >> 16) - 1;  /* In 64k blocks */
        reg_cfg.s.base = ((CFG_FLASH_BASE >> 16) & 0x1fff);
        break;
    CASE_ALL_SRXSME_8MB_FLASH_MODELS
        reg_cfg.s.size = ((CFG_8MB_FLASH_SIZE + 0x400000) >> 16) - 1;  /* In 64k blocks, + 4MByte alias of low 4Mbytes of flash */
        reg_cfg.s.base = ((CFG_8MB_FLASH_BASE >> 16) & 0x1fff);  /* Mask to put physical address in boot bus window */
        /* lower frequency of bus 1 also to 100 Khz */
        cvmx_write_csr(CVMX_MIO_TWSX_SW_TWSI(1),sw_twsi.u64);
        break;
    default:
        reg_cfg.s.size = ((CFG_FLASH_SIZE) >> 16) - 1;  /* In 64k blocks */
        reg_cfg.s.base = ((CFG_FLASH_BASE >> 16) & 0x1fff);
        break;
    }
#else
    reg_cfg.s.size = ((CFG_FLASH_SIZE + 0x400000) >> 16) - 1;  /* In 64k blocks, + 4MByte alias of low 4Mbytes of flash */
    reg_cfg.s.base = ((CFG_FLASH_BASE >> 16) & 0x1fff);
#endif
#ifdef CONFIG_MAG8600
    gd->board_desc.board_type = MAG8600_MSC;
    reg_cfg.s.size = ((CFG_FLASH_SIZE + 0x400000) >> 16) - 1;  /* In 64k blocks, + 4MByte alias of low 4Mbytes of flash */
    reg_cfg.s.base = ((CFG_FLASH_BASE >> 16) & 0x1fff);
    reg_cfg.s.we_ext = 3;
    reg_cfg.s.oe_ext = 3;
    reg_cfg.s.tim_mult = 0;
    reg_cfg.s.rd_dly = 0;

#endif
#if defined(CONFIG_ACX500_SVCS)
    reg_cfg.s.dmack = 0;
    reg_cfg.s.rd_dly = 0;
    reg_cfg.s.sam = 0;
    reg_cfg.s.we_ext = 0;
    reg_cfg.s.oe_ext = 0;
    reg_cfg.s.tim_mult = 0;
    reg_cfg.s.size = ((CFG_FLASH_SIZE) >> 16) - 1;  /* In 64k blocks, + 4MByte alias of low 4Mbytes of flash */
    reg_cfg.s.base = ((CFG_FLASH_BASE >> 16) & 0x1fff);
#endif

    cvmx_write_csr(CVMX_MIO_BOOT_REG_CFG0, reg_cfg.u64);

#ifdef CONFIG_JSRXNLE
    /* 
     * Initialize mpim boot bus windows if defined
     * for this board.
     */
    jsrxnle_mpim_boot_reg_cfg_t *mpim_reg_cfg = NULL;

    switch(gd->board_desc.board_type) {
    CASE_ALL_SRX210_MODELS
        mpim_reg_cfg = mpim_boot_reg_cfg_srx210;
        break;
    
    CASE_ALL_SRX220_MODELS
        mpim_reg_cfg = mpim_boot_reg_cfg_srx220;
        /* enabling gpio pin4 as chip select 4 for voice daughter card*/
        cvmx_write_csr((uint64_t)0x80010700000008a8ull, 0x100);
        break;

    CASE_ALL_SRX240_MODELS
        mpim_reg_cfg = mpim_boot_reg_cfg_srx240;
        break;
        
    CASE_ALL_SRX550_MODELS
        mpim_reg_cfg = mpim_boot_reg_cfg_srx550;
        break;
    }

    if (mpim_reg_cfg) {
        /*
         * Initialize boot bus mpim region
         */
        while (mpim_reg_cfg && mpim_reg_cfg->boot_ce_pin != BOOT_CE_LAST) {
            uint64_t boot_reg_cfg = mpim_reg_cfg->boot_ce_val;
            cvmx_write64(CVMX_MIO_BOOT_REG_CFGX(mpim_reg_cfg->boot_ce_pin),
                                                boot_reg_cfg);
            mpim_reg_cfg++;
        }
    }
#endif

/* Leave flash slow for Trantor and ACX500_SVCS */
#if !defined(CONFIG_ACX500_SVCS) && !defined(CONFIG_OCTEON_TRANTOR)
    /* Set timing to be valid for all CPU clocks up to 600 Mhz */
    reg_tim.u64 = 0;
    reg_tim.s.pagem = 0;
    reg_tim.s.wait = 0x3f;
    reg_tim.s.adr = FLASH_RoundUP((FLASH_RoundUP(10000ULL, ECLK_PERIOD) - 1), 4);
    reg_tim.s.pause = 0;
    reg_tim.s.ce = FLASH_RoundUP((FLASH_RoundUP(50000ULL, ECLK_PERIOD) - 1), 4);;
    if (octeon_is_model(OCTEON_CN31XX))
        reg_tim.s.ale = 4; /* Leave ALE at 34 nS */
    reg_tim.s.oe = FLASH_RoundUP((FLASH_RoundUP(50000ULL, ECLK_PERIOD) - 1), 4);
    reg_tim.s.rd_hld = FLASH_RoundUP((FLASH_RoundUP(25000ULL, ECLK_PERIOD) - 1), 4);
    reg_tim.s.wr_hld = FLASH_RoundUP((FLASH_RoundUP(35000ULL, ECLK_PERIOD) - 1), 4);
    reg_tim.s.we = FLASH_RoundUP((FLASH_RoundUP(35000ULL, ECLK_PERIOD) - 1), 4);
    reg_tim.s.page = FLASH_RoundUP((FLASH_RoundUP(25000ULL, ECLK_PERIOD) - 1), 4);
    cvmx_write_csr(CVMX_MIO_BOOT_REG_TIM0, reg_tim.u64);
#endif
   /* Set up regions for CompactFlash */
    /* Attribute memory region */
#ifdef OCTEON_CF_ATTRIB_CHIP_SEL
    reg_cfg.u64 = 0;
    reg_cfg.s.en = 1;
#ifdef OCTEON_CF_16_BIT_BUS
    reg_cfg.s.width = 1;
#endif
    reg_cfg.s.base = ((OCTEON_CF_ATTRIB_BASE_ADDR >> 16) & 0x1fff);  /* Mask to put physical address in boot bus window */
    cvmx_write_csr(CVMX_MIO_BOOT_REG_CFGX(OCTEON_CF_ATTRIB_CHIP_SEL), reg_cfg.u64);

    reg_tim.u64 = 0;
    reg_tim.s.wait = 0x3f;
    reg_tim.s.page = 0x3f;
    reg_tim.s.wr_hld = FLASH_RoundUP((FLASH_RoundUP(70000ULL, ECLK_PERIOD) - 2), 4);
    reg_tim.s.rd_hld = FLASH_RoundUP((FLASH_RoundUP(100000ULL, ECLK_PERIOD) - 1), 4);
    reg_tim.s.we = FLASH_RoundUP((FLASH_RoundUP(150000ULL, ECLK_PERIOD) - 1), 4);
    reg_tim.s.oe = FLASH_RoundUP((FLASH_RoundUP(270000ULL, ECLK_PERIOD) - 1), 4);
    reg_tim.s.ce = FLASH_RoundUP((FLASH_RoundUP(30000ULL, ECLK_PERIOD) - 2), 4);
    cvmx_write_csr(CVMX_MIO_BOOT_REG_TIMX(OCTEON_CF_ATTRIB_CHIP_SEL), reg_tim.u64);
#endif



#ifdef OCTEON_CF_COMMON_CHIP_SEL
    /* Common memory region */
    reg_cfg.u64 = 0;
    reg_cfg.s.en = 1;
#ifdef OCTEON_CF_16_BIT_BUS
    reg_cfg.s.width = 1;
#endif
    reg_cfg.s.base = ((OCTEON_CF_COMMON_BASE_ADDR >> 16) & 0x1fff);  /* Mask to put physical address in boot bus window */
    cvmx_write_csr(CVMX_MIO_BOOT_REG_CFGX(OCTEON_CF_COMMON_CHIP_SEL), reg_cfg.u64);

    reg_tim.u64 = 0;
    reg_tim.s.wait = (FLASH_RoundUP(30000ULL, ECLK_PERIOD) - 1);
    reg_tim.s.waitm = 1;
    reg_tim.s.page = 0x3f;
    reg_tim.s.wr_hld = FLASH_RoundUP((FLASH_RoundUP(30000ULL, ECLK_PERIOD) - 1), 4);
    reg_tim.s.rd_hld = FLASH_RoundUP((FLASH_RoundUP(100000ULL, ECLK_PERIOD) - 1), 4);
    reg_tim.s.we = FLASH_RoundUP((FLASH_RoundUP(150000ULL, ECLK_PERIOD) - 1), 4);
    reg_tim.s.oe = FLASH_RoundUP((FLASH_RoundUP(125000ULL, ECLK_PERIOD) - 1), 4);
    reg_tim.s.ce = FLASH_RoundUP((FLASH_RoundUP(30000ULL, ECLK_PERIOD) - 2), 4);
    cvmx_write_csr(CVMX_MIO_BOOT_REG_TIMX(OCTEON_CF_COMMON_CHIP_SEL), reg_tim.u64);
#endif


#ifdef OCTEON_CF_TRUE_IDE_CS0_CHIP_SEL
    /* 
     * Initialize CS0 for compact flash only if
     * board type is SRX650; otherwise, already
     * initialized 4th mpim boot bus settings for
     * SRX240 will be overwritten.
     */
#ifdef CONFIG_JSRXNLE
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX650_MODELS
        reg_cfg.u64 = 0;
        reg_cfg.s.en = 1;
        reg_cfg.s.width = 1;
        reg_cfg.s.base = ((OCTEON_CF_TRUE_IDE_CS0_ADDR >> 16) & 0x1fff);  /* Mask to put physical address in boot bus window */
        cvmx_write_csr(CVMX_MIO_BOOT_REG_CFGX(OCTEON_CF_TRUE_IDE_CS0_CHIP_SEL), reg_cfg.u64);

        reg_tim.u64 = 0;
        reg_tim.s.wait = (FLASH_RoundUP(300000ULL, ECLK_PERIOD) - 1);
        reg_tim.s.page = 0x3f;
        reg_tim.s.wr_hld = FLASH_RoundUP((FLASH_RoundUP(300000ULL, ECLK_PERIOD) - 1), 4);
        reg_tim.s.rd_hld = FLASH_RoundUP((FLASH_RoundUP(1000000ULL, ECLK_PERIOD) - 1), 4);
        reg_tim.s.we = FLASH_RoundUP((FLASH_RoundUP(1500000ULL, ECLK_PERIOD) - 1), 4);
        reg_tim.s.oe = FLASH_RoundUP((FLASH_RoundUP(1250000ULL, ECLK_PERIOD) - 1), 4);
        reg_tim.s.ce = FLASH_RoundUP((FLASH_RoundUP(300000ULL, ECLK_PERIOD) - 2), 4);
        cvmx_write_csr(CVMX_MIO_BOOT_REG_TIMX(OCTEON_CF_TRUE_IDE_CS0_CHIP_SEL), reg_tim.u64);
        break;
    }
#else
        reg_cfg.u64 = 0;
        reg_cfg.s.en = 1;
        reg_cfg.s.width = 1;
        reg_cfg.s.base = ((OCTEON_CF_TRUE_IDE_CS0_ADDR >> 16) & 0x1fff);  /* Mask to put physical address in boot bus window */
        cvmx_write_csr(CVMX_MIO_BOOT_REG_CFGX(OCTEON_CF_TRUE_IDE_CS0_CHIP_SEL), reg_cfg.u64);

        reg_tim.u64 = 0;
        reg_tim.s.wait = (FLASH_RoundUP(300000ULL, ECLK_PERIOD) - 1);
        reg_tim.s.page = 0x3f;
        reg_tim.s.wr_hld = FLASH_RoundUP((FLASH_RoundUP(300000ULL, ECLK_PERIOD) - 1), 4);
        reg_tim.s.rd_hld = FLASH_RoundUP((FLASH_RoundUP(1000000ULL, ECLK_PERIOD) - 1), 4);
        reg_tim.s.we = FLASH_RoundUP((FLASH_RoundUP(1500000ULL, ECLK_PERIOD) - 1), 4);
        reg_tim.s.oe = FLASH_RoundUP((FLASH_RoundUP(1250000ULL, ECLK_PERIOD) - 1), 4);
        reg_tim.s.ce = FLASH_RoundUP((FLASH_RoundUP(300000ULL, ECLK_PERIOD) - 2), 4);
        cvmx_write_csr(CVMX_MIO_BOOT_REG_TIMX(OCTEON_CF_TRUE_IDE_CS0_CHIP_SEL), reg_tim.u64);
#endif
#endif

#ifdef CONFIG_JSRXNLE
    /*
     * Set the boot bus timings for SRX220.
     */
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX220_MODELS
        octeon_srx220_bootreg_timings();
        break;
    CASE_ALL_SRX550_MODELS
        octeon_srx550_bootreg_timings();
        break;
    default:
        break;
    }
#endif
#ifdef OCTEON_CF_TRUE_IDE_CS1_CHIP_SEL
    /* 
     * Initialize CS1 for compact flash only if
     * board type is SRX650; otherwise, already
     * initialized 4th mpim boot bus settings for
     * SRX240 will be overwritten.
     */
#ifdef CONFIG_JSRXNLE
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX650_MODELS
        reg_cfg.u64 = 0;
        reg_cfg.s.en = 1;
        reg_cfg.s.width = 1;
        reg_cfg.s.base = ((OCTEON_CF_TRUE_IDE_CS1_ADDR >> 16) & 0x1fff);  /* Mask to put physical address in boot bus window */
        cvmx_write_csr(CVMX_MIO_BOOT_REG_CFGX(OCTEON_CF_TRUE_IDE_CS1_CHIP_SEL), reg_cfg.u64);

        reg_tim.u64 = 0;
        reg_tim.s.wait = (FLASH_RoundUP(30000ULL, ECLK_PERIOD) - 1);
        reg_tim.s.page = 0x3f;
        reg_tim.s.wr_hld = FLASH_RoundUP((FLASH_RoundUP(30000ULL, ECLK_PERIOD) - 1), 4);
        reg_tim.s.rd_hld = FLASH_RoundUP((FLASH_RoundUP(100000ULL, ECLK_PERIOD) - 1), 4);
        reg_tim.s.we = FLASH_RoundUP((FLASH_RoundUP(150000ULL, ECLK_PERIOD) - 1), 4);
        reg_tim.s.oe = FLASH_RoundUP((FLASH_RoundUP(125000ULL, ECLK_PERIOD) - 1), 4);
        reg_tim.s.ce = FLASH_RoundUP((FLASH_RoundUP(30000ULL, ECLK_PERIOD) - 2), 4);
        cvmx_write_csr(CVMX_MIO_BOOT_REG_TIMX(OCTEON_CF_TRUE_IDE_CS1_CHIP_SEL), reg_tim.u64);
        break;
    }
#else
        reg_cfg.u64 = 0;
        reg_cfg.s.en = 1;
        reg_cfg.s.width = 1;
        reg_cfg.s.base = ((OCTEON_CF_TRUE_IDE_CS1_ADDR >> 16) & 0x1fff);  /* Mask to put physical address in boot bus window */
        cvmx_write_csr(CVMX_MIO_BOOT_REG_CFGX(OCTEON_CF_TRUE_IDE_CS1_CHIP_SEL), reg_cfg.u64);

        reg_tim.u64 = 0;
        reg_tim.s.wait = (FLASH_RoundUP(30000ULL, ECLK_PERIOD) - 1);
        reg_tim.s.page = 0x3f;
        reg_tim.s.wr_hld = FLASH_RoundUP((FLASH_RoundUP(30000ULL, ECLK_PERIOD) - 1), 4);
        reg_tim.s.rd_hld = FLASH_RoundUP((FLASH_RoundUP(100000ULL, ECLK_PERIOD) - 1), 4);
        reg_tim.s.we = FLASH_RoundUP((FLASH_RoundUP(150000ULL, ECLK_PERIOD) - 1), 4);
        reg_tim.s.oe = FLASH_RoundUP((FLASH_RoundUP(125000ULL, ECLK_PERIOD) - 1), 4);
        reg_tim.s.ce = FLASH_RoundUP((FLASH_RoundUP(30000ULL, ECLK_PERIOD) - 2), 4);
        cvmx_write_csr(CVMX_MIO_BOOT_REG_TIMX(OCTEON_CF_TRUE_IDE_CS1_CHIP_SEL), reg_tim.u64);

#endif
#endif

#ifdef OCTEON_CPLD_CHIP_SEL
    /* Setup region for CPLD */
    reg_cfg.u64 = 0;
    reg_cfg.s.en = 1;
#ifdef CONFIG_JSRXNLE
    reg_cfg.s.ale = 1;

    /*
     * For SRX220 and 550, we allocate 64K for CS. For other platforms, it is 128K.
     */
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX220_MODELS
    CASE_ALL_SRX550_MODELS
        reg_cfg.s.ale = 0;
        /* Mask to put physical address in boot bus window */
        reg_cfg.s.base = ((OCTEON_SRX220_CPLD_BASE_ADDR >> 16) & 0x1fff);
        break;
    default:
        /* Mask to put physical address in boot bus window */
        reg_cfg.s.base = ((OCTEON_CPLD_BASE_ADDR >> 16) & 0x1fff);
        break;
    }
#endif
#ifdef CONFIG_MAG8600
    reg_cfg.s.tim_mult = 0;
    reg_cfg.s.rd_dly = 0;
    reg_cfg.s.we_ext = 3;
    reg_cfg.s.oe_ext = 3;
    reg_cfg.s.width = 0;        // 8bit
    reg_cfg.s.size = 1;  /* In 64k blocks */
    reg_cfg.s.ale = 0;
    reg_cfg.s.base = ((OCTEON_CPLD_BASE_ADDR >> 16) & 0x1fff);
#endif
    cvmx_write_csr(CVMX_MIO_BOOT_REG_CFGX(1), reg_cfg.u64);
#endif
    
#ifdef OCTEON_CHAR_LED_CHIP_SEL
    /* Setup region for 4 char LED display */
    reg_cfg.u64 = 0;
    reg_cfg.s.en = 1;
    reg_cfg.s.base = ((OCTEON_CHAR_LED_BASE_ADDR >> 16) & 0x1fff);  /* Mask to put physical address in boot bus window */
    cvmx_write_csr(CVMX_MIO_BOOT_REG_CFGX(4), reg_cfg.u64);
#endif

#ifdef OCTEON_PAL_CHIP_SEL
    /* Setup region for PAL access */
    reg_cfg.u64 = 0;
    reg_cfg.s.en = 1;
    reg_cfg.s.base = ((OCTEON_PAL_BASE_ADDR >> 16) & 0x1fff);  /* Mask to put physical address in boot bus window */
    cvmx_write_csr(CVMX_MIO_BOOT_REG_CFGX(OCTEON_PAL_CHIP_SEL), reg_cfg.u64);
#endif
#endif
#if CONFIG_RAM_RESIDENT
    /* Install the 2nd moveable region to the debug exception main entry
        point. This way the debugger will work even if there isn't any
        flash. */
    extern void debugHandler_entrypoint(void);
    const uint64_t *handler_code = (const uint64_t *)debugHandler_entrypoint;
    int count;
    cvmx_write_csr(CVMX_MIO_BOOT_LOC_CFGX(1), (1<<31) | (0x1fc00480>>7<<3));
    cvmx_write_csr(CVMX_MIO_BOOT_LOC_ADR, 0x80);
    for (count=0; count<128/8; count++)
        cvmx_write_csr(CVMX_MIO_BOOT_LOC_DAT, *handler_code++);
#endif

    return(0);
}
#endif
