/***********************license start***************
 * Copyright (c) 2003-2013  Cavium Inc. (support@cavium.com). All rights
 * reserved.
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.

 *   * Neither the name of Cavium Inc. nor the names of
 *     its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written
 *     permission.

 * This Software, including technical data, may be subject to U.S. export  control
 * laws, including the U.S. Export Administration Act and its  associated
 * regulations, and may be subject to export or import  regulations in other
 * countries.

 * TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
 * AND WITH ALL FAULTS AND CAVIUM INC. MAKES NO PROMISES, REPRESENTATIONS OR
 * WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH RESPECT TO
 * THE SOFTWARE, INCLUDING ITS CONDITION, ITS CONFORMITY TO ANY REPRESENTATION OR
 * DESCRIPTION, OR THE EXISTENCE OF ANY LATENT OR PATENT DEFECTS, AND CAVIUM
 * SPECIFICALLY DISCLAIMS ALL IMPLIED (IF ANY) WARRANTIES OF TITLE,
 * MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF
 * VIRUSES, ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 * CORRESPONDENCE TO DESCRIPTION. THE ENTIRE  RISK ARISING OUT OF USE OR
 * PERFORMANCE OF THE SOFTWARE LIES WITH YOU.
 ***********************license end**************************************/


/**
 * cvmx-mio-defs.h
 *
 * Configuration and status register (CSR) type definitions for
 * Octeon mio.
 *
 * This file is auto generated. Do not edit.
 *
 * <hr>$Revision$<hr>
 *
 */
#ifndef __CVMX_MIO_DEFS_H__
#define __CVMX_MIO_DEFS_H__

/**
 * cvmx_mio_ptp_clock_cfg
 *
 * This register configures the timestamp architecture. See MIO_PTP Registers for address
 *
 */
union cvmx_mio_ptp_clock_cfg {
	uint64_t u64;
	struct cvmx_mio_ptp_clock_cfg_s {
#ifdef __BIG_ENDIAN_BITFIELD
	uint64_t reserved_42_63               : 22;
	uint64_t pps                          : 1;  /**< PTP PPS Output
                                                         reflects ptp__pps after PPS_INV inverter */
	uint64_t ckout                        : 1;  /**< PTP Clock Output
                                                         reflects ptp__ckout after CKOUT_INV inverter */
	uint64_t ext_clk_edge                 : 2;  /**< External Clock input edge
                                                         00 = rising edge
                                                         01 = falling edge
                                                         10 = both rising & falling edge
                                                         11 = reserved */
	uint64_t ckout_out4                   : 1;  /**< Destination for PTP Clock Out output
                                                         See CKOUT_OUT */
	uint64_t pps_out                      : 5;  /**< Destination for PTP PPS output to GPIO
                                                         0-19 : GPIO[PPS_OUT[4:0]]
                                                         - 20:30: Reserved
                                                         31   : Disabled
                                                         This should be different from CKOUT_OUT */
	uint64_t pps_inv                      : 1;  /**< Invert PTP PPS
                                                         0 = don't invert
                                                         1 = invert */
	uint64_t pps_en                       : 1;  /**< Enable PTP PPS */
	uint64_t ckout_out                    : 4;  /**< Destination for PTP Clock Out output to GPIO
                                                         0-19 : GPIO[[CKOUT_OUT4,CKOUT_OUT[3:0]]]
                                                         - 20:30: Reserved
                                                         31   : Disabled
                                                         This should be different from PPS_OUT */
	uint64_t ckout_inv                    : 1;  /**< Invert PTP Clock Out
                                                         0 = don't invert
                                                         1 = invert */
	uint64_t ckout_en                     : 1;  /**< Enable PTP Clock Out */
	uint64_t evcnt_in                     : 6;  /**< Source for event counter input
                                                         0x00-0x0f : GPIO[EVCNT_IN[3:0]]
                                                         0x20      : GPIO[16]
                                                         0x21      : GPIO[17]
                                                         0x22      : GPIO[18]
                                                         0x23      : GPIO[19]
                                                         0x10      : QLM0_REF_CLK
                                                         0x11      : QLM1_REF_CLK
                                                         0x18      : RF_MCLK (PHY pin)
                                                         0x12-0x17 : Reserved
                                                         0x19-0x1f : Reserved
                                                         0x24-0x3f : Reserved */
	uint64_t evcnt_edge                   : 1;  /**< Event counter input edge
                                                         0 = falling edge
                                                         1 = rising edge */
	uint64_t evcnt_en                     : 1;  /**< Enable event counter */
	uint64_t tstmp_in                     : 6;  /**< Source for timestamp input
                                                         0x00-0x0f : GPIO[TSTMP_IN[3:0]]
                                                         0x20      : GPIO[16]
                                                         0x21      : GPIO[17]
                                                         0x22      : GPIO[18]
                                                         0x23      : GPIO[19]
                                                         0x10      : QLM0_REF_CLK
                                                         0x11      : QLM1_REF_CLK
                                                         0x18      : RF_MCLK (PHY pin)
                                                         0x12-0x17 : Reserved
                                                         0x19-0x1f : Reserved
                                                         0x24-0x3f : Reserved */
	uint64_t tstmp_edge                   : 1;  /**< External timestamp input edge
                                                         0 = falling edge
                                                         1 = rising edge */
	uint64_t tstmp_en                     : 1;  /**< Enable external timestamp */
	uint64_t ext_clk_in                   : 6;  /**< Source for external clock
                                                         0x00-0x0f : GPIO[EXT_CLK_IN[3:0]]
                                                         0x20      : GPIO[16]
                                                         0x21      : GPIO[17]
                                                         0x22      : GPIO[18]
                                                         0x23      : GPIO[19]
                                                         0x10      : QLM0_REF_CLK
                                                         0x11      : QLM1_REF_CLK
                                                         0x18      : RF_MCLK (PHY pin)
                                                         0x12-0x17 : Reserved
                                                         0x19-0x1f : Reserved
                                                         0x24-0x3f : Reserved */
	uint64_t ext_clk_en                   : 1;  /**< Use external clock */
	uint64_t ptp_en                       : 1;  /**< Enable PTP Module */
#else
	uint64_t ptp_en                       : 1;
	uint64_t ext_clk_en                   : 1;
	uint64_t ext_clk_in                   : 6;
	uint64_t tstmp_en                     : 1;
	uint64_t tstmp_edge                   : 1;
	uint64_t tstmp_in                     : 6;
	uint64_t evcnt_en                     : 1;
	uint64_t evcnt_edge                   : 1;
	uint64_t evcnt_in                     : 6;
	uint64_t ckout_en                     : 1;
	uint64_t ckout_inv                    : 1;
	uint64_t ckout_out                    : 4;
	uint64_t pps_en                       : 1;
	uint64_t pps_inv                      : 1;
	uint64_t pps_out                      : 5;
	uint64_t ckout_out4                   : 1;
	uint64_t ext_clk_edge                 : 2;
	uint64_t ckout                        : 1;
	uint64_t pps                          : 1;
	uint64_t reserved_42_63               : 22;
#endif
	} s;
	struct cvmx_mio_ptp_clock_cfg_s       cn61xx;
	struct cvmx_mio_ptp_clock_cfg_cn63xx {
#ifdef __BIG_ENDIAN_BITFIELD
	uint64_t reserved_24_63               : 40;
	uint64_t evcnt_in                     : 6;  /**< Source for event counter input
                                                         0x00-0x0f : GPIO[EVCNT_IN[3:0]]
                                                         0x10      : QLM0_REF_CLK
                                                         0x11      : QLM1_REF_CLK
                                                         0x12      : QLM2_REF_CLK
                                                         0x13-0x3f : Reserved */
	uint64_t evcnt_edge                   : 1;  /**< Event counter input edge
                                                         0 = falling edge
                                                         1 = rising edge */
	uint64_t evcnt_en                     : 1;  /**< Enable event counter */
	uint64_t tstmp_in                     : 6;  /**< Source for timestamp input
                                                         0x00-0x0f : GPIO[TSTMP_IN[3:0]]
                                                         0x10      : QLM0_REF_CLK
                                                         0x11      : QLM1_REF_CLK
                                                         0x12      : QLM2_REF_CLK
                                                         0x13-0x3f : Reserved */
	uint64_t tstmp_edge                   : 1;  /**< External timestamp input edge
                                                         0 = falling edge
                                                         1 = rising edge */
	uint64_t tstmp_en                     : 1;  /**< Enable external timestamp */
	uint64_t ext_clk_in                   : 6;  /**< Source for external clock
                                                         0x00-0x0f : GPIO[EXT_CLK_IN[3:0]]
                                                         0x10      : QLM0_REF_CLK
                                                         0x11      : QLM1_REF_CLK
                                                         0x12      : QLM2_REF_CLK
                                                         0x13-0x3f : Reserved */
	uint64_t ext_clk_en                   : 1;  /**< Use positive edge of external clock */
	uint64_t ptp_en                       : 1;  /**< Enable PTP Module */
#else
	uint64_t ptp_en                       : 1;
	uint64_t ext_clk_en                   : 1;
	uint64_t ext_clk_in                   : 6;
	uint64_t tstmp_en                     : 1;
	uint64_t tstmp_edge                   : 1;
	uint64_t tstmp_in                     : 6;
	uint64_t evcnt_en                     : 1;
	uint64_t evcnt_edge                   : 1;
	uint64_t evcnt_in                     : 6;
	uint64_t reserved_24_63               : 40;
#endif
	} cn63xx;
	struct cvmx_mio_ptp_clock_cfg_cn63xx  cn63xxp1;
	struct cvmx_mio_ptp_clock_cfg_cn66xx {
#ifdef __BIG_ENDIAN_BITFIELD
	uint64_t reserved_40_63               : 24;
	uint64_t ext_clk_edge                 : 2;  /**< External Clock input edge
                                                         00 = rising edge
                                                         01 = falling edge
                                                         10 = both rising & falling edge
                                                         11 = reserved */
	uint64_t ckout_out4                   : 1;  /**< Destination for PTP Clock Out output
                                                         0-19 : GPIO[[CKOUT_OUT4,CKOUT_OUT[3:0]]]
                                                         This should be different from PPS_OUT */
	uint64_t pps_out                      : 5;  /**< Destination for PTP PPS output
                                                         0-19 : GPIO[PPS_OUT[4:0]]
                                                         This should be different from CKOUT_OUT */
	uint64_t pps_inv                      : 1;  /**< Invert PTP PPS
                                                         0 = don't invert
                                                         1 = invert */
	uint64_t pps_en                       : 1;  /**< Enable PTP PPS */
	uint64_t ckout_out                    : 4;  /**< Destination for PTP Clock Out output
                                                         0-19 : GPIO[[CKOUT_OUT4,CKOUT_OUT[3:0]]]
                                                         This should be different from PPS_OUT */
	uint64_t ckout_inv                    : 1;  /**< Invert PTP Clock Out
                                                         0 = don't invert
                                                         1 = invert */
	uint64_t ckout_en                     : 1;  /**< Enable PTP Clock Out */
	uint64_t evcnt_in                     : 6;  /**< Source for event counter input
                                                         0x00-0x0f : GPIO[EVCNT_IN[3:0]]
                                                         0x20      : GPIO[16]
                                                         0x21      : GPIO[17]
                                                         0x22      : GPIO[18]
                                                         0x23      : GPIO[19]
                                                         0x10      : QLM0_REF_CLK
                                                         0x11      : QLM1_REF_CLK
                                                         0x12      : QLM2_REF_CLK
                                                         0x13-0x1f : Reserved
                                                         0x24-0x3f : Reserved */
	uint64_t evcnt_edge                   : 1;  /**< Event counter input edge
                                                         0 = falling edge
                                                         1 = rising edge */
	uint64_t evcnt_en                     : 1;  /**< Enable event counter */
	uint64_t tstmp_in                     : 6;  /**< Source for timestamp input
                                                         0x00-0x0f : GPIO[TSTMP_IN[3:0]]
                                                         0x20      : GPIO[16]
                                                         0x21      : GPIO[17]
                                                         0x22      : GPIO[18]
                                                         0x23      : GPIO[19]
                                                         0x10      : QLM0_REF_CLK
                                                         0x11      : QLM1_REF_CLK
                                                         0x12      : QLM2_REF_CLK
                                                         0x13-0x1f : Reserved
                                                         0x24-0x3f : Reserved */
	uint64_t tstmp_edge                   : 1;  /**< External timestamp input edge
                                                         0 = falling edge
                                                         1 = rising edge */
	uint64_t tstmp_en                     : 1;  /**< Enable external timestamp */
	uint64_t ext_clk_in                   : 6;  /**< Source for external clock
                                                         0x00-0x0f : GPIO[EXT_CLK_IN[3:0]]
                                                         0x20      : GPIO[16]
                                                         0x21      : GPIO[17]
                                                         0x22      : GPIO[18]
                                                         0x23      : GPIO[19]
                                                         0x10      : QLM0_REF_CLK
                                                         0x11      : QLM1_REF_CLK
                                                         0x12      : QLM2_REF_CLK
                                                         0x13-0x1f : Reserved
                                                         0x24-0x3f : Reserved */
	uint64_t ext_clk_en                   : 1;  /**< Use external clock */
	uint64_t ptp_en                       : 1;  /**< Enable PTP Module */
#else
	uint64_t ptp_en                       : 1;
	uint64_t ext_clk_en                   : 1;
	uint64_t ext_clk_in                   : 6;
	uint64_t tstmp_en                     : 1;
	uint64_t tstmp_edge                   : 1;
	uint64_t tstmp_in                     : 6;
	uint64_t evcnt_en                     : 1;
	uint64_t evcnt_edge                   : 1;
	uint64_t evcnt_in                     : 6;
	uint64_t ckout_en                     : 1;
	uint64_t ckout_inv                    : 1;
	uint64_t ckout_out                    : 4;
	uint64_t pps_en                       : 1;
	uint64_t pps_inv                      : 1;
	uint64_t pps_out                      : 5;
	uint64_t ckout_out4                   : 1;
	uint64_t ext_clk_edge                 : 2;
	uint64_t reserved_40_63               : 24;
#endif
	} cn66xx;
	struct cvmx_mio_ptp_clock_cfg_s       cn68xx;
	struct cvmx_mio_ptp_clock_cfg_cn63xx  cn68xxp1;
	struct cvmx_mio_ptp_clock_cfg_cn70xx {
#ifdef __BIG_ENDIAN_BITFIELD
	uint64_t reserved_42_63               : 22;
	uint64_t pps                          : 1;  /**< PTP PPS Output
                                                         reflects ptp__pps after PPS_INV inverter */
	uint64_t ckout                        : 1;  /**< PTP Clock Output
                                                         reflects ptp__ckout after CKOUT_INV inverter */
	uint64_t ext_clk_edge                 : 2;  /**< External Clock input edge
                                                         00 = rising edge
                                                         01 = falling edge
                                                         10 = both rising & falling edge
                                                         11 = reserved */
	uint64_t reserved_32_37               : 6;
	uint64_t pps_inv                      : 1;  /**< Invert PTP PPS
                                                         0 = don't invert
                                                         1 = invert */
	uint64_t pps_en                       : 1;  /**< Enable PTP PPS */
	uint64_t reserved_26_29               : 4;
	uint64_t ckout_inv                    : 1;  /**< Invert PTP Clock Out
                                                         0 = don't invert
                                                         1 = invert */
	uint64_t ckout_en                     : 1;  /**< Enable PTP Clock Out */
	uint64_t evcnt_in                     : 6;  /**< Source for event counter input
                                                         0x00-0x0f : GPIO[EVCNT_IN[3:0]]
                                                         0x20      : GPIO[16]
                                                         0x21      : GPIO[17]
                                                         0x22      : GPIO[18]
                                                         0x23      : GPIO[19]
                                                         0x10      : QLM0_REF_CLK
                                                         0x11      : QLM1_REF_CLK
                                                         0x12      : QLM2_REF_CLK
                                                         0x13-0x1f : Reserved
                                                         0x24-0x3f : Reserved */
	uint64_t evcnt_edge                   : 1;  /**< Event counter input edge
                                                         0 = falling edge
                                                         1 = rising edge */
	uint64_t evcnt_en                     : 1;  /**< Enable event counter */
	uint64_t tstmp_in                     : 6;  /**< Source for timestamp input
                                                         0x00-0x0f : GPIO[TSTMP_IN[3:0]]
                                                         0x20      : GPIO[16]
                                                         0x21      : GPIO[17]
                                                         0x22      : GPIO[18]
                                                         0x23      : GPIO[19]
                                                         0x10      : QLM0_REF_CLK
                                                         0x11      : QLM1_REF_CLK
                                                         0x12      : QLM2_REF_CLK
                                                         0x13-0x1f : Reserved
                                                         0x24-0x3f : Reserved */
	uint64_t tstmp_edge                   : 1;  /**< External timestamp input edge
                                                         0 = falling edge
                                                         1 = rising edge */
	uint64_t tstmp_en                     : 1;  /**< Enable external timestamp */
	uint64_t ext_clk_in                   : 6;  /**< Source for external clock
                                                         0x00-0x0f : GPIO[EXT_CLK_IN[3:0]]
                                                         0x20      : GPIO[16]
                                                         0x21      : GPIO[17]
                                                         0x22      : GPIO[18]
                                                         0x23      : GPIO[19]
                                                         0x10      : QLM0_EF_CLK
                                                         0x11      : QLM1_REF_CLK
                                                         0x12      : QLM2_REF_CLK
                                                         0x13-0x1f : Reserved
                                                         0x24-0x3f : Reserved */
	uint64_t ext_clk_en                   : 1;  /**< Use external clock */
	uint64_t ptp_en                       : 1;  /**< Enable PTP Module */
#else
	uint64_t ptp_en                       : 1;
	uint64_t ext_clk_en                   : 1;
	uint64_t ext_clk_in                   : 6;
	uint64_t tstmp_en                     : 1;
	uint64_t tstmp_edge                   : 1;
	uint64_t tstmp_in                     : 6;
	uint64_t evcnt_en                     : 1;
	uint64_t evcnt_edge                   : 1;
	uint64_t evcnt_in                     : 6;
	uint64_t ckout_en                     : 1;
	uint64_t ckout_inv                    : 1;
	uint64_t reserved_26_29               : 4;
	uint64_t pps_en                       : 1;
	uint64_t pps_inv                      : 1;
	uint64_t reserved_32_37               : 6;
	uint64_t ext_clk_edge                 : 2;
	uint64_t ckout                        : 1;
	uint64_t pps                          : 1;
	uint64_t reserved_42_63               : 22;
#endif
	} cn70xx;
	struct cvmx_mio_ptp_clock_cfg_cn70xx  cn78xx;
	struct cvmx_mio_ptp_clock_cfg_s       cnf71xx;
};
typedef union cvmx_mio_ptp_clock_cfg cvmx_mio_ptp_clock_cfg_t;

#endif
