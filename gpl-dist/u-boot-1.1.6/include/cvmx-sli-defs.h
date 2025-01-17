/***********************license start***************
 * Copyright (c) 2003-2010  Cavium Networks (support@cavium.com). All rights
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

 *   * Neither the name of Cavium Networks nor the names of
 *     its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written
 *     permission.

 * This Software, including technical data, may be subject to U.S. export  control
 * laws, including the U.S. Export Administration Act and its  associated
 * regulations, and may be subject to export or import  regulations in other
 * countries.

 * TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
 * AND WITH ALL FAULTS AND CAVIUM  NETWORKS MAKES NO PROMISES, REPRESENTATIONS OR
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
 * cvmx-sli-defs.h
 *
 * Configuration and status register (CSR) type definitions for
 * Octeon sli.
 *
 * This file is auto generated. Do not edit.
 *
 * <hr>$Revision$<hr>
 *
 */

#ifndef __CVMX_SLI_TYPEDEFS_H__
#define __CVMX_SLI_TYPEDEFS_H__

#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_BIST_STATUS CVMX_SLI_BIST_STATUS_FUNC()
static inline uint64_t CVMX_SLI_BIST_STATUS_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX) || OCTEON_IS_MODEL(OCTEON_CN71XX)))
		cvmx_warn("CVMX_SLI_BIST_STATUS not supported on this chip\n");
	return 0x0000000000000580ull;
}
#else
#define CVMX_SLI_BIST_STATUS (0x0000000000000580ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
static inline uint64_t CVMX_SLI_CTL_PORTX(unsigned long offset)
{
	if (!(
	      (OCTEON_IS_MODEL(OCTEON_CN63XX) && ((offset <= 1)))))
		cvmx_warn("CVMX_SLI_CTL_PORTX(%lu) is invalid on this chip\n", offset);
	return 0x0000000000000050ull + ((offset) & 1) * 16;
}
#else
#define CVMX_SLI_CTL_PORTX(offset) (0x0000000000000050ull + ((offset) & 1) * 16)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_CTL_STATUS CVMX_SLI_CTL_STATUS_FUNC()
static inline uint64_t CVMX_SLI_CTL_STATUS_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_CTL_STATUS not supported on this chip\n");
	return 0x0000000000000570ull;
}
#else
#define CVMX_SLI_CTL_STATUS (0x0000000000000570ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_DATA_OUT_CNT CVMX_SLI_DATA_OUT_CNT_FUNC()
static inline uint64_t CVMX_SLI_DATA_OUT_CNT_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_DATA_OUT_CNT not supported on this chip\n");
	return 0x00000000000005F0ull;
}
#else
#define CVMX_SLI_DATA_OUT_CNT (0x00000000000005F0ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_DBG_DATA CVMX_SLI_DBG_DATA_FUNC()
static inline uint64_t CVMX_SLI_DBG_DATA_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_DBG_DATA not supported on this chip\n");
	return 0x0000000000000310ull;
}
#else
#define CVMX_SLI_DBG_DATA (0x0000000000000310ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_DBG_SELECT CVMX_SLI_DBG_SELECT_FUNC()
static inline uint64_t CVMX_SLI_DBG_SELECT_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_DBG_SELECT not supported on this chip\n");
	return 0x0000000000000300ull;
}
#else
#define CVMX_SLI_DBG_SELECT (0x0000000000000300ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
static inline uint64_t CVMX_SLI_DMAX_CNT(unsigned long offset)
{
	if (!(
	      (OCTEON_IS_MODEL(OCTEON_CN63XX) && ((offset <= 1)))))
		cvmx_warn("CVMX_SLI_DMAX_CNT(%lu) is invalid on this chip\n", offset);
	return 0x0000000000000400ull + ((offset) & 1) * 16;
}
#else
#define CVMX_SLI_DMAX_CNT(offset) (0x0000000000000400ull + ((offset) & 1) * 16)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
static inline uint64_t CVMX_SLI_DMAX_INT_LEVEL(unsigned long offset)
{
	if (!(
	      (OCTEON_IS_MODEL(OCTEON_CN63XX) && ((offset <= 1)))))
		cvmx_warn("CVMX_SLI_DMAX_INT_LEVEL(%lu) is invalid on this chip\n", offset);
	return 0x00000000000003E0ull + ((offset) & 1) * 16;
}
#else
#define CVMX_SLI_DMAX_INT_LEVEL(offset) (0x00000000000003E0ull + ((offset) & 1) * 16)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
static inline uint64_t CVMX_SLI_DMAX_TIM(unsigned long offset)
{
	if (!(
	      (OCTEON_IS_MODEL(OCTEON_CN63XX) && ((offset <= 1)))))
		cvmx_warn("CVMX_SLI_DMAX_TIM(%lu) is invalid on this chip\n", offset);
	return 0x0000000000000420ull + ((offset) & 1) * 16;
}
#else
#define CVMX_SLI_DMAX_TIM(offset) (0x0000000000000420ull + ((offset) & 1) * 16)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_INT_ENB_CIU CVMX_SLI_INT_ENB_CIU_FUNC()
static inline uint64_t CVMX_SLI_INT_ENB_CIU_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_INT_ENB_CIU not supported on this chip\n");
	return 0x0000000000003CD0ull;
}
#else
#define CVMX_SLI_INT_ENB_CIU (0x0000000000003CD0ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
static inline uint64_t CVMX_SLI_INT_ENB_PORTX(unsigned long offset)
{
	if (!(
	      (OCTEON_IS_MODEL(OCTEON_CN63XX) && ((offset <= 1)))))
		cvmx_warn("CVMX_SLI_INT_ENB_PORTX(%lu) is invalid on this chip\n", offset);
	return 0x0000000000000340ull + ((offset) & 1) * 16;
}
#else
#define CVMX_SLI_INT_ENB_PORTX(offset) (0x0000000000000340ull + ((offset) & 1) * 16)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_INT_SUM CVMX_SLI_INT_SUM_FUNC()
static inline uint64_t CVMX_SLI_INT_SUM_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_INT_SUM not supported on this chip\n");
	return 0x0000000000000330ull;
}
#else
#define CVMX_SLI_INT_SUM (0x0000000000000330ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_LAST_WIN_RDATA0 CVMX_SLI_LAST_WIN_RDATA0_FUNC()
static inline uint64_t CVMX_SLI_LAST_WIN_RDATA0_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_LAST_WIN_RDATA0 not supported on this chip\n");
	return 0x0000000000000600ull;
}
#else
#define CVMX_SLI_LAST_WIN_RDATA0 (0x0000000000000600ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_LAST_WIN_RDATA1 CVMX_SLI_LAST_WIN_RDATA1_FUNC()
static inline uint64_t CVMX_SLI_LAST_WIN_RDATA1_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_LAST_WIN_RDATA1 not supported on this chip\n");
	return 0x0000000000000610ull;
}
#else
#define CVMX_SLI_LAST_WIN_RDATA1 (0x0000000000000610ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_MAC_CREDIT_CNT CVMX_SLI_MAC_CREDIT_CNT_FUNC()
static inline uint64_t CVMX_SLI_MAC_CREDIT_CNT_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_MAC_CREDIT_CNT not supported on this chip\n");
	return 0x0000000000003D70ull;
}
#else
#define CVMX_SLI_MAC_CREDIT_CNT (0x0000000000003D70ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_MAC_NUMBER CVMX_SLI_MAC_NUMBER_FUNC()
static inline uint64_t CVMX_SLI_MAC_NUMBER_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_MAC_NUMBER not supported on this chip\n");
	return 0x0000000000003E00ull;
}
#else
#define CVMX_SLI_MAC_NUMBER (0x0000000000003E00ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_MEM_ACCESS_CTL CVMX_SLI_MEM_ACCESS_CTL_FUNC()
static inline uint64_t CVMX_SLI_MEM_ACCESS_CTL_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_MEM_ACCESS_CTL not supported on this chip\n");
	return 0x00000000000002F0ull;
}
#else
#define CVMX_SLI_MEM_ACCESS_CTL (0x00000000000002F0ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
static inline uint64_t CVMX_SLI_MEM_ACCESS_SUBIDX(unsigned long offset)
{
	if (!(
	      (OCTEON_IS_MODEL(OCTEON_CN63XX) && (((offset >= 12) && (offset <= 27))))))
		cvmx_warn("CVMX_SLI_MEM_ACCESS_SUBIDX(%lu) is invalid on this chip\n", offset);
	return 0x00000000000001A0ull + ((offset) & 31) * 16 - 16*12;
}
#else
#define CVMX_SLI_MEM_ACCESS_SUBIDX(offset) (0x00000000000001A0ull + ((offset) & 31) * 16 - 16*12)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_MSI_ENB0 CVMX_SLI_MSI_ENB0_FUNC()
static inline uint64_t CVMX_SLI_MSI_ENB0_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_MSI_ENB0 not supported on this chip\n");
	return 0x0000000000003C50ull;
}
#else
#define CVMX_SLI_MSI_ENB0 (0x0000000000003C50ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_MSI_ENB1 CVMX_SLI_MSI_ENB1_FUNC()
static inline uint64_t CVMX_SLI_MSI_ENB1_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_MSI_ENB1 not supported on this chip\n");
	return 0x0000000000003C60ull;
}
#else
#define CVMX_SLI_MSI_ENB1 (0x0000000000003C60ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_MSI_ENB2 CVMX_SLI_MSI_ENB2_FUNC()
static inline uint64_t CVMX_SLI_MSI_ENB2_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_MSI_ENB2 not supported on this chip\n");
	return 0x0000000000003C70ull;
}
#else
#define CVMX_SLI_MSI_ENB2 (0x0000000000003C70ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_MSI_ENB3 CVMX_SLI_MSI_ENB3_FUNC()
static inline uint64_t CVMX_SLI_MSI_ENB3_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_MSI_ENB3 not supported on this chip\n");
	return 0x0000000000003C80ull;
}
#else
#define CVMX_SLI_MSI_ENB3 (0x0000000000003C80ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_MSI_RCV0 CVMX_SLI_MSI_RCV0_FUNC()
static inline uint64_t CVMX_SLI_MSI_RCV0_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_MSI_RCV0 not supported on this chip\n");
	return 0x0000000000003C10ull;
}
#else
#define CVMX_SLI_MSI_RCV0 (0x0000000000003C10ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_MSI_RCV1 CVMX_SLI_MSI_RCV1_FUNC()
static inline uint64_t CVMX_SLI_MSI_RCV1_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_MSI_RCV1 not supported on this chip\n");
	return 0x0000000000003C20ull;
}
#else
#define CVMX_SLI_MSI_RCV1 (0x0000000000003C20ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_MSI_RCV2 CVMX_SLI_MSI_RCV2_FUNC()
static inline uint64_t CVMX_SLI_MSI_RCV2_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_MSI_RCV2 not supported on this chip\n");
	return 0x0000000000003C30ull;
}
#else
#define CVMX_SLI_MSI_RCV2 (0x0000000000003C30ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_MSI_RCV3 CVMX_SLI_MSI_RCV3_FUNC()
static inline uint64_t CVMX_SLI_MSI_RCV3_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_MSI_RCV3 not supported on this chip\n");
	return 0x0000000000003C40ull;
}
#else
#define CVMX_SLI_MSI_RCV3 (0x0000000000003C40ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_MSI_RD_MAP CVMX_SLI_MSI_RD_MAP_FUNC()
static inline uint64_t CVMX_SLI_MSI_RD_MAP_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_MSI_RD_MAP not supported on this chip\n");
	return 0x0000000000003CA0ull;
}
#else
#define CVMX_SLI_MSI_RD_MAP (0x0000000000003CA0ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_MSI_W1C_ENB0 CVMX_SLI_MSI_W1C_ENB0_FUNC()
static inline uint64_t CVMX_SLI_MSI_W1C_ENB0_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_MSI_W1C_ENB0 not supported on this chip\n");
	return 0x0000000000003CF0ull;
}
#else
#define CVMX_SLI_MSI_W1C_ENB0 (0x0000000000003CF0ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_MSI_W1C_ENB1 CVMX_SLI_MSI_W1C_ENB1_FUNC()
static inline uint64_t CVMX_SLI_MSI_W1C_ENB1_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_MSI_W1C_ENB1 not supported on this chip\n");
	return 0x0000000000003D00ull;
}
#else
#define CVMX_SLI_MSI_W1C_ENB1 (0x0000000000003D00ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_MSI_W1C_ENB2 CVMX_SLI_MSI_W1C_ENB2_FUNC()
static inline uint64_t CVMX_SLI_MSI_W1C_ENB2_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_MSI_W1C_ENB2 not supported on this chip\n");
	return 0x0000000000003D10ull;
}
#else
#define CVMX_SLI_MSI_W1C_ENB2 (0x0000000000003D10ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_MSI_W1C_ENB3 CVMX_SLI_MSI_W1C_ENB3_FUNC()
static inline uint64_t CVMX_SLI_MSI_W1C_ENB3_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_MSI_W1C_ENB3 not supported on this chip\n");
	return 0x0000000000003D20ull;
}
#else
#define CVMX_SLI_MSI_W1C_ENB3 (0x0000000000003D20ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_MSI_W1S_ENB0 CVMX_SLI_MSI_W1S_ENB0_FUNC()
static inline uint64_t CVMX_SLI_MSI_W1S_ENB0_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_MSI_W1S_ENB0 not supported on this chip\n");
	return 0x0000000000003D30ull;
}
#else
#define CVMX_SLI_MSI_W1S_ENB0 (0x0000000000003D30ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_MSI_W1S_ENB1 CVMX_SLI_MSI_W1S_ENB1_FUNC()
static inline uint64_t CVMX_SLI_MSI_W1S_ENB1_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_MSI_W1S_ENB1 not supported on this chip\n");
	return 0x0000000000003D40ull;
}
#else
#define CVMX_SLI_MSI_W1S_ENB1 (0x0000000000003D40ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_MSI_W1S_ENB2 CVMX_SLI_MSI_W1S_ENB2_FUNC()
static inline uint64_t CVMX_SLI_MSI_W1S_ENB2_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_MSI_W1S_ENB2 not supported on this chip\n");
	return 0x0000000000003D50ull;
}
#else
#define CVMX_SLI_MSI_W1S_ENB2 (0x0000000000003D50ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_MSI_W1S_ENB3 CVMX_SLI_MSI_W1S_ENB3_FUNC()
static inline uint64_t CVMX_SLI_MSI_W1S_ENB3_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_MSI_W1S_ENB3 not supported on this chip\n");
	return 0x0000000000003D60ull;
}
#else
#define CVMX_SLI_MSI_W1S_ENB3 (0x0000000000003D60ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_MSI_WR_MAP CVMX_SLI_MSI_WR_MAP_FUNC()
static inline uint64_t CVMX_SLI_MSI_WR_MAP_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_MSI_WR_MAP not supported on this chip\n");
	return 0x0000000000003C90ull;
}
#else
#define CVMX_SLI_MSI_WR_MAP (0x0000000000003C90ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PCIE_MSI_RCV CVMX_SLI_PCIE_MSI_RCV_FUNC()
static inline uint64_t CVMX_SLI_PCIE_MSI_RCV_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PCIE_MSI_RCV not supported on this chip\n");
	return 0x0000000000003CB0ull;
}
#else
#define CVMX_SLI_PCIE_MSI_RCV (0x0000000000003CB0ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PCIE_MSI_RCV_B1 CVMX_SLI_PCIE_MSI_RCV_B1_FUNC()
static inline uint64_t CVMX_SLI_PCIE_MSI_RCV_B1_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PCIE_MSI_RCV_B1 not supported on this chip\n");
	return 0x0000000000000650ull;
}
#else
#define CVMX_SLI_PCIE_MSI_RCV_B1 (0x0000000000000650ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PCIE_MSI_RCV_B2 CVMX_SLI_PCIE_MSI_RCV_B2_FUNC()
static inline uint64_t CVMX_SLI_PCIE_MSI_RCV_B2_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PCIE_MSI_RCV_B2 not supported on this chip\n");
	return 0x0000000000000660ull;
}
#else
#define CVMX_SLI_PCIE_MSI_RCV_B2 (0x0000000000000660ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PCIE_MSI_RCV_B3 CVMX_SLI_PCIE_MSI_RCV_B3_FUNC()
static inline uint64_t CVMX_SLI_PCIE_MSI_RCV_B3_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PCIE_MSI_RCV_B3 not supported on this chip\n");
	return 0x0000000000000670ull;
}
#else
#define CVMX_SLI_PCIE_MSI_RCV_B3 (0x0000000000000670ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
static inline uint64_t CVMX_SLI_PKTX_CNTS(unsigned long offset)
{
	if (!(
	      (OCTEON_IS_MODEL(OCTEON_CN63XX) && ((offset <= 31)))))
		cvmx_warn("CVMX_SLI_PKTX_CNTS(%lu) is invalid on this chip\n", offset);
	return 0x0000000000002400ull + ((offset) & 31) * 16;
}
#else
#define CVMX_SLI_PKTX_CNTS(offset) (0x0000000000002400ull + ((offset) & 31) * 16)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
static inline uint64_t CVMX_SLI_PKTX_INSTR_BADDR(unsigned long offset)
{
	if (!(
	      (OCTEON_IS_MODEL(OCTEON_CN63XX) && ((offset <= 31)))))
		cvmx_warn("CVMX_SLI_PKTX_INSTR_BADDR(%lu) is invalid on this chip\n", offset);
	return 0x0000000000002800ull + ((offset) & 31) * 16;
}
#else
#define CVMX_SLI_PKTX_INSTR_BADDR(offset) (0x0000000000002800ull + ((offset) & 31) * 16)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
static inline uint64_t CVMX_SLI_PKTX_INSTR_BAOFF_DBELL(unsigned long offset)
{
	if (!(
	      (OCTEON_IS_MODEL(OCTEON_CN63XX) && ((offset <= 31)))))
		cvmx_warn("CVMX_SLI_PKTX_INSTR_BAOFF_DBELL(%lu) is invalid on this chip\n", offset);
	return 0x0000000000002C00ull + ((offset) & 31) * 16;
}
#else
#define CVMX_SLI_PKTX_INSTR_BAOFF_DBELL(offset) (0x0000000000002C00ull + ((offset) & 31) * 16)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
static inline uint64_t CVMX_SLI_PKTX_INSTR_FIFO_RSIZE(unsigned long offset)
{
	if (!(
	      (OCTEON_IS_MODEL(OCTEON_CN63XX) && ((offset <= 31)))))
		cvmx_warn("CVMX_SLI_PKTX_INSTR_FIFO_RSIZE(%lu) is invalid on this chip\n", offset);
	return 0x0000000000003000ull + ((offset) & 31) * 16;
}
#else
#define CVMX_SLI_PKTX_INSTR_FIFO_RSIZE(offset) (0x0000000000003000ull + ((offset) & 31) * 16)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
static inline uint64_t CVMX_SLI_PKTX_INSTR_HEADER(unsigned long offset)
{
	if (!(
	      (OCTEON_IS_MODEL(OCTEON_CN63XX) && ((offset <= 31)))))
		cvmx_warn("CVMX_SLI_PKTX_INSTR_HEADER(%lu) is invalid on this chip\n", offset);
	return 0x0000000000003400ull + ((offset) & 31) * 16;
}
#else
#define CVMX_SLI_PKTX_INSTR_HEADER(offset) (0x0000000000003400ull + ((offset) & 31) * 16)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
static inline uint64_t CVMX_SLI_PKTX_IN_BP(unsigned long offset)
{
	if (!(
	      (OCTEON_IS_MODEL(OCTEON_CN63XX) && ((offset <= 31)))))
		cvmx_warn("CVMX_SLI_PKTX_IN_BP(%lu) is invalid on this chip\n", offset);
	return 0x0000000000003800ull + ((offset) & 31) * 16;
}
#else
#define CVMX_SLI_PKTX_IN_BP(offset) (0x0000000000003800ull + ((offset) & 31) * 16)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
static inline uint64_t CVMX_SLI_PKTX_OUT_SIZE(unsigned long offset)
{
	if (!(
	      (OCTEON_IS_MODEL(OCTEON_CN63XX) && ((offset <= 31)))))
		cvmx_warn("CVMX_SLI_PKTX_OUT_SIZE(%lu) is invalid on this chip\n", offset);
	return 0x0000000000000C00ull + ((offset) & 31) * 16;
}
#else
#define CVMX_SLI_PKTX_OUT_SIZE(offset) (0x0000000000000C00ull + ((offset) & 31) * 16)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
static inline uint64_t CVMX_SLI_PKTX_SLIST_BADDR(unsigned long offset)
{
	if (!(
	      (OCTEON_IS_MODEL(OCTEON_CN63XX) && ((offset <= 31)))))
		cvmx_warn("CVMX_SLI_PKTX_SLIST_BADDR(%lu) is invalid on this chip\n", offset);
	return 0x0000000000001400ull + ((offset) & 31) * 16;
}
#else
#define CVMX_SLI_PKTX_SLIST_BADDR(offset) (0x0000000000001400ull + ((offset) & 31) * 16)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
static inline uint64_t CVMX_SLI_PKTX_SLIST_BAOFF_DBELL(unsigned long offset)
{
	if (!(
	      (OCTEON_IS_MODEL(OCTEON_CN63XX) && ((offset <= 31)))))
		cvmx_warn("CVMX_SLI_PKTX_SLIST_BAOFF_DBELL(%lu) is invalid on this chip\n", offset);
	return 0x0000000000001800ull + ((offset) & 31) * 16;
}
#else
#define CVMX_SLI_PKTX_SLIST_BAOFF_DBELL(offset) (0x0000000000001800ull + ((offset) & 31) * 16)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
static inline uint64_t CVMX_SLI_PKTX_SLIST_FIFO_RSIZE(unsigned long offset)
{
	if (!(
	      (OCTEON_IS_MODEL(OCTEON_CN63XX) && ((offset <= 31)))))
		cvmx_warn("CVMX_SLI_PKTX_SLIST_FIFO_RSIZE(%lu) is invalid on this chip\n", offset);
	return 0x0000000000001C00ull + ((offset) & 31) * 16;
}
#else
#define CVMX_SLI_PKTX_SLIST_FIFO_RSIZE(offset) (0x0000000000001C00ull + ((offset) & 31) * 16)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_CNT_INT CVMX_SLI_PKT_CNT_INT_FUNC()
static inline uint64_t CVMX_SLI_PKT_CNT_INT_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_CNT_INT not supported on this chip\n");
	return 0x0000000000001130ull;
}
#else
#define CVMX_SLI_PKT_CNT_INT (0x0000000000001130ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_CNT_INT_ENB CVMX_SLI_PKT_CNT_INT_ENB_FUNC()
static inline uint64_t CVMX_SLI_PKT_CNT_INT_ENB_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_CNT_INT_ENB not supported on this chip\n");
	return 0x0000000000001150ull;
}
#else
#define CVMX_SLI_PKT_CNT_INT_ENB (0x0000000000001150ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_CTL CVMX_SLI_PKT_CTL_FUNC()
static inline uint64_t CVMX_SLI_PKT_CTL_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_CTL not supported on this chip\n");
	return 0x0000000000001220ull;
}
#else
#define CVMX_SLI_PKT_CTL (0x0000000000001220ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_DATA_OUT_ES CVMX_SLI_PKT_DATA_OUT_ES_FUNC()
static inline uint64_t CVMX_SLI_PKT_DATA_OUT_ES_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_DATA_OUT_ES not supported on this chip\n");
	return 0x00000000000010B0ull;
}
#else
#define CVMX_SLI_PKT_DATA_OUT_ES (0x00000000000010B0ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_DATA_OUT_NS CVMX_SLI_PKT_DATA_OUT_NS_FUNC()
static inline uint64_t CVMX_SLI_PKT_DATA_OUT_NS_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_DATA_OUT_NS not supported on this chip\n");
	return 0x00000000000010A0ull;
}
#else
#define CVMX_SLI_PKT_DATA_OUT_NS (0x00000000000010A0ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_DATA_OUT_ROR CVMX_SLI_PKT_DATA_OUT_ROR_FUNC()
static inline uint64_t CVMX_SLI_PKT_DATA_OUT_ROR_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_DATA_OUT_ROR not supported on this chip\n");
	return 0x0000000000001090ull;
}
#else
#define CVMX_SLI_PKT_DATA_OUT_ROR (0x0000000000001090ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_DPADDR CVMX_SLI_PKT_DPADDR_FUNC()
static inline uint64_t CVMX_SLI_PKT_DPADDR_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_DPADDR not supported on this chip\n");
	return 0x0000000000001080ull;
}
#else
#define CVMX_SLI_PKT_DPADDR (0x0000000000001080ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_INPUT_CONTROL CVMX_SLI_PKT_INPUT_CONTROL_FUNC()
static inline uint64_t CVMX_SLI_PKT_INPUT_CONTROL_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_INPUT_CONTROL not supported on this chip\n");
	return 0x0000000000001170ull;
}
#else
#define CVMX_SLI_PKT_INPUT_CONTROL (0x0000000000001170ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_INSTR_ENB CVMX_SLI_PKT_INSTR_ENB_FUNC()
static inline uint64_t CVMX_SLI_PKT_INSTR_ENB_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_INSTR_ENB not supported on this chip\n");
	return 0x0000000000001000ull;
}
#else
#define CVMX_SLI_PKT_INSTR_ENB (0x0000000000001000ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_INSTR_RD_SIZE CVMX_SLI_PKT_INSTR_RD_SIZE_FUNC()
static inline uint64_t CVMX_SLI_PKT_INSTR_RD_SIZE_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_INSTR_RD_SIZE not supported on this chip\n");
	return 0x00000000000011A0ull;
}
#else
#define CVMX_SLI_PKT_INSTR_RD_SIZE (0x00000000000011A0ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_INSTR_SIZE CVMX_SLI_PKT_INSTR_SIZE_FUNC()
static inline uint64_t CVMX_SLI_PKT_INSTR_SIZE_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_INSTR_SIZE not supported on this chip\n");
	return 0x0000000000001020ull;
}
#else
#define CVMX_SLI_PKT_INSTR_SIZE (0x0000000000001020ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_INT_LEVELS CVMX_SLI_PKT_INT_LEVELS_FUNC()
static inline uint64_t CVMX_SLI_PKT_INT_LEVELS_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_INT_LEVELS not supported on this chip\n");
	return 0x0000000000001120ull;
}
#else
#define CVMX_SLI_PKT_INT_LEVELS (0x0000000000001120ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_IN_BP CVMX_SLI_PKT_IN_BP_FUNC()
static inline uint64_t CVMX_SLI_PKT_IN_BP_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_IN_BP not supported on this chip\n");
	return 0x0000000000001210ull;
}
#else
#define CVMX_SLI_PKT_IN_BP (0x0000000000001210ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
static inline uint64_t CVMX_SLI_PKT_IN_DONEX_CNTS(unsigned long offset)
{
	if (!(
	      (OCTEON_IS_MODEL(OCTEON_CN63XX) && ((offset <= 31)))))
		cvmx_warn("CVMX_SLI_PKT_IN_DONEX_CNTS(%lu) is invalid on this chip\n", offset);
	return 0x0000000000002000ull + ((offset) & 31) * 16;
}
#else
#define CVMX_SLI_PKT_IN_DONEX_CNTS(offset) (0x0000000000002000ull + ((offset) & 31) * 16)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_IN_INSTR_COUNTS CVMX_SLI_PKT_IN_INSTR_COUNTS_FUNC()
static inline uint64_t CVMX_SLI_PKT_IN_INSTR_COUNTS_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_IN_INSTR_COUNTS not supported on this chip\n");
	return 0x0000000000001200ull;
}
#else
#define CVMX_SLI_PKT_IN_INSTR_COUNTS (0x0000000000001200ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_IN_PCIE_PORT CVMX_SLI_PKT_IN_PCIE_PORT_FUNC()
static inline uint64_t CVMX_SLI_PKT_IN_PCIE_PORT_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_IN_PCIE_PORT not supported on this chip\n");
	return 0x00000000000011B0ull;
}
#else
#define CVMX_SLI_PKT_IN_PCIE_PORT (0x00000000000011B0ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_IPTR CVMX_SLI_PKT_IPTR_FUNC()
static inline uint64_t CVMX_SLI_PKT_IPTR_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_IPTR not supported on this chip\n");
	return 0x0000000000001070ull;
}
#else
#define CVMX_SLI_PKT_IPTR (0x0000000000001070ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_OUTPUT_WMARK CVMX_SLI_PKT_OUTPUT_WMARK_FUNC()
static inline uint64_t CVMX_SLI_PKT_OUTPUT_WMARK_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_OUTPUT_WMARK not supported on this chip\n");
	return 0x0000000000001180ull;
}
#else
#define CVMX_SLI_PKT_OUTPUT_WMARK (0x0000000000001180ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_OUT_BMODE CVMX_SLI_PKT_OUT_BMODE_FUNC()
static inline uint64_t CVMX_SLI_PKT_OUT_BMODE_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_OUT_BMODE not supported on this chip\n");
	return 0x00000000000010D0ull;
}
#else
#define CVMX_SLI_PKT_OUT_BMODE (0x00000000000010D0ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_OUT_ENB CVMX_SLI_PKT_OUT_ENB_FUNC()
static inline uint64_t CVMX_SLI_PKT_OUT_ENB_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_OUT_ENB not supported on this chip\n");
	return 0x0000000000001010ull;
}
#else
#define CVMX_SLI_PKT_OUT_ENB (0x0000000000001010ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_PCIE_PORT CVMX_SLI_PKT_PCIE_PORT_FUNC()
static inline uint64_t CVMX_SLI_PKT_PCIE_PORT_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_PCIE_PORT not supported on this chip\n");
	return 0x00000000000010E0ull;
}
#else
#define CVMX_SLI_PKT_PCIE_PORT (0x00000000000010E0ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_PORT_IN_RST CVMX_SLI_PKT_PORT_IN_RST_FUNC()
static inline uint64_t CVMX_SLI_PKT_PORT_IN_RST_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_PORT_IN_RST not supported on this chip\n");
	return 0x00000000000011F0ull;
}
#else
#define CVMX_SLI_PKT_PORT_IN_RST (0x00000000000011F0ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_SLIST_ES CVMX_SLI_PKT_SLIST_ES_FUNC()
static inline uint64_t CVMX_SLI_PKT_SLIST_ES_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_SLIST_ES not supported on this chip\n");
	return 0x0000000000001050ull;
}
#else
#define CVMX_SLI_PKT_SLIST_ES (0x0000000000001050ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_SLIST_NS CVMX_SLI_PKT_SLIST_NS_FUNC()
static inline uint64_t CVMX_SLI_PKT_SLIST_NS_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_SLIST_NS not supported on this chip\n");
	return 0x0000000000001040ull;
}
#else
#define CVMX_SLI_PKT_SLIST_NS (0x0000000000001040ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_SLIST_ROR CVMX_SLI_PKT_SLIST_ROR_FUNC()
static inline uint64_t CVMX_SLI_PKT_SLIST_ROR_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_SLIST_ROR not supported on this chip\n");
	return 0x0000000000001030ull;
}
#else
#define CVMX_SLI_PKT_SLIST_ROR (0x0000000000001030ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_TIME_INT CVMX_SLI_PKT_TIME_INT_FUNC()
static inline uint64_t CVMX_SLI_PKT_TIME_INT_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_TIME_INT not supported on this chip\n");
	return 0x0000000000001140ull;
}
#else
#define CVMX_SLI_PKT_TIME_INT (0x0000000000001140ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_PKT_TIME_INT_ENB CVMX_SLI_PKT_TIME_INT_ENB_FUNC()
static inline uint64_t CVMX_SLI_PKT_TIME_INT_ENB_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_PKT_TIME_INT_ENB not supported on this chip\n");
	return 0x0000000000001160ull;
}
#else
#define CVMX_SLI_PKT_TIME_INT_ENB (0x0000000000001160ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
static inline uint64_t CVMX_SLI_S2M_PORTX_CTL(unsigned long offset)
{
	if (!(
	      (OCTEON_IS_MODEL(OCTEON_CN63XX) && ((offset <= 1)))))
		cvmx_warn("CVMX_SLI_S2M_PORTX_CTL(%lu) is invalid on this chip\n", offset);
	return 0x0000000000003D80ull + ((offset) & 1) * 16;
}
#else
#define CVMX_SLI_S2M_PORTX_CTL(offset) (0x0000000000003D80ull + ((offset) & 1) * 16)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_SCRATCH_1 CVMX_SLI_SCRATCH_1_FUNC()
static inline uint64_t CVMX_SLI_SCRATCH_1_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_SCRATCH_1 not supported on this chip\n");
	return 0x00000000000003C0ull;
}
#else
#define CVMX_SLI_SCRATCH_1 (0x00000000000003C0ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_SCRATCH_2 CVMX_SLI_SCRATCH_2_FUNC()
static inline uint64_t CVMX_SLI_SCRATCH_2_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_SCRATCH_2 not supported on this chip\n");
	return 0x00000000000003D0ull;
}
#else
#define CVMX_SLI_SCRATCH_2 (0x00000000000003D0ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_STATE1 CVMX_SLI_STATE1_FUNC()
static inline uint64_t CVMX_SLI_STATE1_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_STATE1 not supported on this chip\n");
	return 0x0000000000000620ull;
}
#else
#define CVMX_SLI_STATE1 (0x0000000000000620ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_STATE2 CVMX_SLI_STATE2_FUNC()
static inline uint64_t CVMX_SLI_STATE2_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_STATE2 not supported on this chip\n");
	return 0x0000000000000630ull;
}
#else
#define CVMX_SLI_STATE2 (0x0000000000000630ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_STATE3 CVMX_SLI_STATE3_FUNC()
static inline uint64_t CVMX_SLI_STATE3_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_STATE3 not supported on this chip\n");
	return 0x0000000000000640ull;
}
#else
#define CVMX_SLI_STATE3 (0x0000000000000640ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_WINDOW_CTL CVMX_SLI_WINDOW_CTL_FUNC()
static inline uint64_t CVMX_SLI_WINDOW_CTL_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_WINDOW_CTL not supported on this chip\n");
	return 0x00000000000002E0ull;
}
#else
#define CVMX_SLI_WINDOW_CTL (0x00000000000002E0ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_WIN_RD_ADDR CVMX_SLI_WIN_RD_ADDR_FUNC()
static inline uint64_t CVMX_SLI_WIN_RD_ADDR_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_WIN_RD_ADDR not supported on this chip\n");
	return 0x0000000000000010ull;
}
#else
#define CVMX_SLI_WIN_RD_ADDR (0x0000000000000010ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_WIN_RD_DATA CVMX_SLI_WIN_RD_DATA_FUNC()
static inline uint64_t CVMX_SLI_WIN_RD_DATA_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_WIN_RD_DATA not supported on this chip\n");
	return 0x0000000000000040ull;
}
#else
#define CVMX_SLI_WIN_RD_DATA (0x0000000000000040ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_WIN_WR_ADDR CVMX_SLI_WIN_WR_ADDR_FUNC()
static inline uint64_t CVMX_SLI_WIN_WR_ADDR_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_WIN_WR_ADDR not supported on this chip\n");
	return 0x0000000000000000ull;
}
#else
#define CVMX_SLI_WIN_WR_ADDR (0x0000000000000000ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_WIN_WR_DATA CVMX_SLI_WIN_WR_DATA_FUNC()
static inline uint64_t CVMX_SLI_WIN_WR_DATA_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_WIN_WR_DATA not supported on this chip\n");
	return 0x0000000000000020ull;
}
#else
#define CVMX_SLI_WIN_WR_DATA (0x0000000000000020ull)
#endif
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_SLI_WIN_WR_MASK CVMX_SLI_WIN_WR_MASK_FUNC()
static inline uint64_t CVMX_SLI_WIN_WR_MASK_FUNC(void)
{
	if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
		cvmx_warn("CVMX_SLI_WIN_WR_MASK not supported on this chip\n");
	return 0x0000000000000030ull;
}
#else
#define CVMX_SLI_WIN_WR_MASK (0x0000000000000030ull)
#endif

/**
 * cvmx_sli_bist_status
 *
 * SLI_BIST_STATUS = SLI's BIST Status Register
 *
 * Results from BIST runs of SLI's memories.
 */
union cvmx_sli_bist_status
{
	uint64_t u64;
	struct cvmx_sli_bist_status_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_31_63               : 33;
	uint64_t n2p0_c                       : 1;  /**< BIST Status for N2P Port0 Cmd */
	uint64_t n2p0_o                       : 1;  /**< BIST Status for N2P Port0 Data */
	uint64_t n2p1_c                       : 1;  /**< BIST Status for N2P Port1 Cmd */
	uint64_t n2p1_o                       : 1;  /**< BIST Status for N2P Port1 Data */
	uint64_t cpl_p0                       : 1;  /**< BIST Status for CPL Port 0 */
	uint64_t cpl_p1                       : 1;  /**< BIST Status for CPL Port 1 */
	uint64_t reserved_19_24               : 6;
	uint64_t p2n0_c0                      : 1;  /**< BIST Status for P2N Port0 C0 */
	uint64_t p2n0_c1                      : 1;  /**< BIST Status for P2N Port0 C1 */
	uint64_t p2n0_n                       : 1;  /**< BIST Status for P2N Port0 N */
	uint64_t p2n0_p0                      : 1;  /**< BIST Status for P2N Port0 P0 */
	uint64_t p2n0_p1                      : 1;  /**< BIST Status for P2N Port0 P1 */
	uint64_t p2n1_c0                      : 1;  /**< BIST Status for P2N Port1 C0 */
	uint64_t p2n1_c1                      : 1;  /**< BIST Status for P2N Port1 C1 */
	uint64_t p2n1_n                       : 1;  /**< BIST Status for P2N Port1 N */
	uint64_t p2n1_p0                      : 1;  /**< BIST Status for P2N Port1 P0 */
	uint64_t p2n1_p1                      : 1;  /**< BIST Status for P2N Port1 P1 */
	uint64_t reserved_6_8                 : 3;
	uint64_t dsi1_1                       : 1;  /**< BIST Status for DSI1 Memory 1 */
	uint64_t dsi1_0                       : 1;  /**< BIST Status for DSI1 Memory 0 */
	uint64_t dsi0_1                       : 1;  /**< BIST Status for DSI0 Memory 1 */
	uint64_t dsi0_0                       : 1;  /**< BIST Status for DSI0 Memory 0 */
	uint64_t msi                          : 1;  /**< BIST Status for MSI Memory Map */
	uint64_t ncb_cmd                      : 1;  /**< BIST Status for NCB Outbound Commands */
#else
	uint64_t ncb_cmd                      : 1;
	uint64_t msi                          : 1;
	uint64_t dsi0_0                       : 1;
	uint64_t dsi0_1                       : 1;
	uint64_t dsi1_0                       : 1;
	uint64_t dsi1_1                       : 1;
	uint64_t reserved_6_8                 : 3;
	uint64_t p2n1_p1                      : 1;
	uint64_t p2n1_p0                      : 1;
	uint64_t p2n1_n                       : 1;
	uint64_t p2n1_c1                      : 1;
	uint64_t p2n1_c0                      : 1;
	uint64_t p2n0_p1                      : 1;
	uint64_t p2n0_p0                      : 1;
	uint64_t p2n0_n                       : 1;
	uint64_t p2n0_c1                      : 1;
	uint64_t p2n0_c0                      : 1;
	uint64_t reserved_19_24               : 6;
	uint64_t cpl_p1                       : 1;
	uint64_t cpl_p0                       : 1;
	uint64_t n2p1_o                       : 1;
	uint64_t n2p1_c                       : 1;
	uint64_t n2p0_o                       : 1;
	uint64_t n2p0_c                       : 1;
	uint64_t reserved_31_63               : 33;
#endif
	} s;
	struct cvmx_sli_bist_status_s         cn63xx;
	struct cvmx_sli_bist_status_s         cn63xxp1;
};
typedef union cvmx_sli_bist_status cvmx_sli_bist_status_t;

/**
 * cvmx_sli_ctl_port#
 *
 * SLI_CTL_PORTX = SLI's Control Port X
 *
 * Contains control for access for Port0
 */
union cvmx_sli_ctl_portx
{
	uint64_t u64;
	struct cvmx_sli_ctl_portx_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_22_63               : 42;
	uint64_t intd                         : 1;  /**< When '0' Intd wire asserted. Before mapping. */
	uint64_t intc                         : 1;  /**< When '0' Intc wire asserted. Before mapping. */
	uint64_t intb                         : 1;  /**< When '0' Intb wire asserted. Before mapping. */
	uint64_t inta                         : 1;  /**< When '0' Inta wire asserted. Before mapping. */
	uint64_t dis_port                     : 1;  /**< When set the output to the MAC is disabled. This
                                                         occurs when the MAC reset line transitions from
                                                         de-asserted to asserted. Writing a '1' to this
                                                         location will clear this condition when the MAC is
                                                         no longer in reset and the output to the MAC is at
                                                         the begining of a transfer. */
	uint64_t waitl_com                    : 1;  /**< When set '1' casues the SLI to wait for a commit
                                                         from the L2C before sending additional completions
                                                         to the L2C from a MAC.
                                                         Set this for more conservative behavior. Clear
                                                         this for more aggressive, higher-performance
                                                         behavior */
	uint64_t intd_map                     : 2;  /**< Maps INTD to INTA(00), INTB(01), INTC(10) or
                                                         INTD (11). */
	uint64_t intc_map                     : 2;  /**< Maps INTC to INTA(00), INTB(01), INTC(10) or
                                                         INTD (11). */
	uint64_t intb_map                     : 2;  /**< Maps INTB to INTA(00), INTB(01), INTC(10) or
                                                         INTD (11). */
	uint64_t inta_map                     : 2;  /**< Maps INTA to INTA(00), INTB(01), INTC(10) or
                                                         INTD (11). */
	uint64_t ctlp_ro                      : 1;  /**< Relaxed ordering enable for Completion TLPS. */
	uint64_t reserved_6_6                 : 1;
	uint64_t ptlp_ro                      : 1;  /**< Relaxed ordering enable for Posted TLPS. */
	uint64_t reserved_1_4                 : 4;
	uint64_t wait_com                     : 1;  /**< When set '1' casues the SLI to wait for a commit
                                                         from the L2C before sending additional stores to
                                                         the L2C from a MAC.
                                                         The SLI will request a commit on the last store
                                                         if more than one STORE operation is required on
                                                         the NCB.
                                                         Most applications will not notice a difference, so
                                                         should not set this bit. Setting the bit is more
                                                         conservative on ordering, lower performance */
#else
	uint64_t wait_com                     : 1;
	uint64_t reserved_1_4                 : 4;
	uint64_t ptlp_ro                      : 1;
	uint64_t reserved_6_6                 : 1;
	uint64_t ctlp_ro                      : 1;
	uint64_t inta_map                     : 2;
	uint64_t intb_map                     : 2;
	uint64_t intc_map                     : 2;
	uint64_t intd_map                     : 2;
	uint64_t waitl_com                    : 1;
	uint64_t dis_port                     : 1;
	uint64_t inta                         : 1;
	uint64_t intb                         : 1;
	uint64_t intc                         : 1;
	uint64_t intd                         : 1;
	uint64_t reserved_22_63               : 42;
#endif
	} s;
	struct cvmx_sli_ctl_portx_s           cn63xx;
	struct cvmx_sli_ctl_portx_s           cn63xxp1;
};
typedef union cvmx_sli_ctl_portx cvmx_sli_ctl_portx_t;

/**
 * cvmx_sli_ctl_status
 *
 * SLI_CTL_STATUS = SLI Control Status Register
 *
 * Contains control and status for SLI. Writes to this register are not ordered with writes/reads to the MAC Memory space.
 * To ensure that a write has completed the user must read the register before making an access(i.e. MAC memory space)
 * that requires the value of this register to be updated.
 */
union cvmx_sli_ctl_status
{
	uint64_t u64;
	struct cvmx_sli_ctl_status_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_20_63               : 44;
	uint64_t p1_ntags                     : 6;  /**< Number of tags available for MAC Port1.
                                                         In RC mode 1 tag is needed for each outbound TLP
                                                         that requires a CPL TLP. In Endpoint mode the
                                                         number of tags required for a TLP request is
                                                         1 per 64-bytes of CPL data + 1.
                                                         This field should only be written as part of
                                                         reset sequence, before issuing any reads, CFGs, or
                                                         IO transactions from the core(s). */
	uint64_t p0_ntags                     : 6;  /**< Number of tags available for MAC Port0.
                                                         In RC mode 1 tag is needed for each outbound TLP
                                                         that requires a CPL TLP. In Endpoint mode the
                                                         number of tags required for a TLP request is
                                                         1 per 64-bytes of CPL data + 1.
                                                         This field should only be written as part of
                                                         reset sequence, before issuing any reads, CFGs, or
                                                         IO transactions from the core(s). */
	uint64_t chip_rev                     : 8;  /**< The chip revision. */
#else
	uint64_t chip_rev                     : 8;
	uint64_t p0_ntags                     : 6;
	uint64_t p1_ntags                     : 6;
	uint64_t reserved_20_63               : 44;
#endif
	} s;
	struct cvmx_sli_ctl_status_s          cn63xx;
	struct cvmx_sli_ctl_status_s          cn63xxp1;
};
typedef union cvmx_sli_ctl_status cvmx_sli_ctl_status_t;

/**
 * cvmx_sli_data_out_cnt
 *
 * SLI_DATA_OUT_CNT = SLI DATA OUT COUNT
 *
 * The EXEC data out fifo-count and the data unload counter.
 */
union cvmx_sli_data_out_cnt
{
	uint64_t u64;
	struct cvmx_sli_data_out_cnt_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_44_63               : 20;
	uint64_t p1_ucnt                      : 16; /**< MAC Port1 Fifo Unload Count. This counter is
                                                         incremented by '1' every time a word is removed
                                                         from the Data Out FIFO, whose count is shown in
                                                         P0_FCNT. */
	uint64_t p1_fcnt                      : 6;  /**< MAC Port1 Data Out Fifo Count. Number of address
                                                         data words to be sent out the MAC port presently
                                                         buffered in the FIFO. */
	uint64_t p0_ucnt                      : 16; /**< MAC Port0 Fifo Unload Count. This counter is
                                                         incremented by '1' every time a word is removed
                                                         from the Data Out FIFO, whose count is shown in
                                                         P0_FCNT. */
	uint64_t p0_fcnt                      : 6;  /**< MAC Port0 Data Out Fifo Count. Number of address
                                                         data words to be sent out the MAC port presently
                                                         buffered in the FIFO. */
#else
	uint64_t p0_fcnt                      : 6;
	uint64_t p0_ucnt                      : 16;
	uint64_t p1_fcnt                      : 6;
	uint64_t p1_ucnt                      : 16;
	uint64_t reserved_44_63               : 20;
#endif
	} s;
	struct cvmx_sli_data_out_cnt_s        cn63xx;
	struct cvmx_sli_data_out_cnt_s        cn63xxp1;
};
typedef union cvmx_sli_data_out_cnt cvmx_sli_data_out_cnt_t;

/**
 * cvmx_sli_dbg_data
 *
 * SLI_DBG_DATA = SLI Debug Data Register
 *
 * Value returned on the debug-data lines from the RSLs
 */
union cvmx_sli_dbg_data
{
	uint64_t u64;
	struct cvmx_sli_dbg_data_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_18_63               : 46;
	uint64_t dsel_ext                     : 1;  /**< Allows changes in the external pins to set the
                                                         debug select value. */
	uint64_t data                         : 17; /**< Value on the debug data lines. */
#else
	uint64_t data                         : 17;
	uint64_t dsel_ext                     : 1;
	uint64_t reserved_18_63               : 46;
#endif
	} s;
	struct cvmx_sli_dbg_data_s            cn63xx;
	struct cvmx_sli_dbg_data_s            cn63xxp1;
};
typedef union cvmx_sli_dbg_data cvmx_sli_dbg_data_t;

/**
 * cvmx_sli_dbg_select
 *
 * SLI_DBG_SELECT = Debug Select Register
 *
 * Contains the debug select value last written to the RSLs.
 */
union cvmx_sli_dbg_select
{
	uint64_t u64;
	struct cvmx_sli_dbg_select_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_33_63               : 31;
	uint64_t adbg_sel                     : 1;  /**< When set '1' the SLI_DBG_DATA[DATA] will only be
                                                         loaded when SLI_DBG_DATA[DATA] bit [16] is a '1'.
                                                         When the debug data comes from an Async-RSL bit
                                                         16 is used to tell that the data present is valid. */
	uint64_t dbg_sel                      : 32; /**< When this register is written the RML will write
                                                         all "F"s to the previous RTL to disable it from
                                                         sending Debug-Data. The RML will then send a write
                                                         to the new RSL with the supplied Debug-Select
                                                         value. Because it takes time for the new Debug
                                                         Select value to take effect and the requested
                                                         Debug-Data to return, time is needed to the new
                                                         Debug-Data to arrive.  The inititator of the Debug
                                                         Select should issue a read to a CSR before reading
                                                         the Debug Data (this read could also be to the
                                                         SLI_DBG_DATA but the returned value for the first
                                                         read will return NS data. */
#else
	uint64_t dbg_sel                      : 32;
	uint64_t adbg_sel                     : 1;
	uint64_t reserved_33_63               : 31;
#endif
	} s;
	struct cvmx_sli_dbg_select_s          cn63xx;
	struct cvmx_sli_dbg_select_s          cn63xxp1;
};
typedef union cvmx_sli_dbg_select cvmx_sli_dbg_select_t;

/**
 * cvmx_sli_dma#_cnt
 *
 * SLI_DMAx_CNT = SLI DMA Count
 *
 * The DMA Count value.
 */
union cvmx_sli_dmax_cnt
{
	uint64_t u64;
	struct cvmx_sli_dmax_cnt_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_32_63               : 32;
	uint64_t cnt                          : 32; /**< The DMA counter.
                                                         Writing this field will cause the written value
                                                         to be subtracted from DMA. HW will optionally
                                                         increment this field after it completes an
                                                         OUTBOUND or EXTERNAL-ONLY DMA instruction. These
                                                         increments may cause interrupts. Refer to
                                                         SLI_DMAx_INT_LEVEL and SLI_INT_SUM[DCNT,DTIME]. */
#else
	uint64_t cnt                          : 32;
	uint64_t reserved_32_63               : 32;
#endif
	} s;
	struct cvmx_sli_dmax_cnt_s            cn63xx;
	struct cvmx_sli_dmax_cnt_s            cn63xxp1;
};
typedef union cvmx_sli_dmax_cnt cvmx_sli_dmax_cnt_t;

/**
 * cvmx_sli_dma#_int_level
 *
 * SLI_DMAx_INT_LEVEL = SLI DMAx Interrupt Level
 *
 * Thresholds for DMA count and timer interrupts.
 */
union cvmx_sli_dmax_int_level
{
	uint64_t u64;
	struct cvmx_sli_dmax_int_level_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t time                         : 32; /**< Whenever the SLI_DMAx_TIM[TIM] timer exceeds
                                                         this value, SLI_INT_SUM[DTIME<x>] is set.
                                                         The SLI_DMAx_TIM[TIM] timer increments every SLI
                                                         clock whenever SLI_DMAx_CNT[CNT]!=0, and is
                                                         cleared when SLI_INT_SUM[DTIME<x>] is written with
                                                         one. */
	uint64_t cnt                          : 32; /**< Whenever SLI_DMAx_CNT[CNT] exceeds this value,
                                                         SLI_INT_SUM[DCNT<x>] is set. */
#else
	uint64_t cnt                          : 32;
	uint64_t time                         : 32;
#endif
	} s;
	struct cvmx_sli_dmax_int_level_s      cn63xx;
	struct cvmx_sli_dmax_int_level_s      cn63xxp1;
};
typedef union cvmx_sli_dmax_int_level cvmx_sli_dmax_int_level_t;

/**
 * cvmx_sli_dma#_tim
 *
 * SLI_DMAx_TIM = SLI DMA Timer
 *
 * The DMA Timer value.
 */
union cvmx_sli_dmax_tim
{
	uint64_t u64;
	struct cvmx_sli_dmax_tim_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_32_63               : 32;
	uint64_t tim                          : 32; /**< The DMA timer value.
                                                         The timer will increment when SLI_DMAx_CNT[CNT]!=0
                                                         and will clear when SLI_DMAx_CNT[CNT]==0 */
#else
	uint64_t tim                          : 32;
	uint64_t reserved_32_63               : 32;
#endif
	} s;
	struct cvmx_sli_dmax_tim_s            cn63xx;
	struct cvmx_sli_dmax_tim_s            cn63xxp1;
};
typedef union cvmx_sli_dmax_tim cvmx_sli_dmax_tim_t;

/**
 * cvmx_sli_int_enb_ciu
 *
 * SLI_INT_ENB_CIU = SLI's Interrupt Enable CIU Register
 *
 * Used to enable the various interrupting conditions of SLI
 */
union cvmx_sli_int_enb_ciu
{
	uint64_t u64;
	struct cvmx_sli_int_enb_ciu_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_61_63               : 3;
	uint64_t ill_pad                      : 1;  /**< Illegal packet csr address. */
	uint64_t reserved_58_59               : 2;
	uint64_t sprt1_err                    : 1;  /**< Error Response received on SLI port 1. */
	uint64_t sprt0_err                    : 1;  /**< Error Response received on SLI port 0. */
	uint64_t pins_err                     : 1;  /**< Read Error during packet instruction fetch. */
	uint64_t pop_err                      : 1;  /**< Read Error during packet scatter pointer fetch. */
	uint64_t pdi_err                      : 1;  /**< Read Error during packet data fetch. */
	uint64_t pgl_err                      : 1;  /**< Read Error during gather list fetch. */
	uint64_t pin_bp                       : 1;  /**< Packet Input Count exceeded WMARK. */
	uint64_t pout_err                     : 1;  /**< Packet Out Interrupt, Error From PKO. */
	uint64_t psldbof                      : 1;  /**< Packet Scatterlist Doorbell Count Overflow. */
	uint64_t pidbof                       : 1;  /**< Packet Instruction Doorbell Count Overflow. */
	uint64_t reserved_38_47               : 10;
	uint64_t dtime                        : 2;  /**< DMA Timer Interrupts */
	uint64_t dcnt                         : 2;  /**< DMA Count Interrupts */
	uint64_t dmafi                        : 2;  /**< DMA set Forced Interrupts */
	uint64_t reserved_18_31               : 14;
	uint64_t mio_int1                     : 1;  /**< Enables SLI_INT_SUM[17] to generate an
                                                         interrupt on the RSL.
                                                         THIS SHOULD NEVER BE SET */
	uint64_t mio_int0                     : 1;  /**< Enables SLI_INT_SUM[16] to generate an
                                                         interrupt on the RSL.
                                                         THIS SHOULD NEVER BE SET */
	uint64_t m1_un_wi                     : 1;  /**< Enables SLI_INT_SUM[15] to generate an
                                                         interrupt on the RSL. */
	uint64_t m1_un_b0                     : 1;  /**< Enables SLI_INT_SUM[14] to generate an
                                                         interrupt on the RSL. */
	uint64_t m1_up_wi                     : 1;  /**< Enables SLI_INT_SUM[13] to generate an
                                                         interrupt on the RSL. */
	uint64_t m1_up_b0                     : 1;  /**< Enables SLI_INT_SUM[12] to generate an
                                                         interrupt on the RSL. */
	uint64_t m0_un_wi                     : 1;  /**< Enables SLI_INT_SUM[11] to generate an
                                                         interrupt on the RSL. */
	uint64_t m0_un_b0                     : 1;  /**< Enables SLI_INT_SUM[10] to generate an
                                                         interrupt on the RSL. */
	uint64_t m0_up_wi                     : 1;  /**< Enables SLI_INT_SUM[9] to generate an
                                                         interrupt on the RSL. */
	uint64_t m0_up_b0                     : 1;  /**< Enables SLI_INT_SUM[8] to generate an
                                                         interrupt on the RSL. */
	uint64_t reserved_6_7                 : 2;
	uint64_t ptime                        : 1;  /**< Enables SLI_INT_SUM[5] to generate an
                                                         interrupt on the RSL. */
	uint64_t pcnt                         : 1;  /**< Enables SLI_INT_SUM[4] to generate an
                                                         interrupt on the RSL. */
	uint64_t iob2big                      : 1;  /**< Enables SLI_INT_SUM[3] to generate an
                                                         interrupt on the RSL. */
	uint64_t bar0_to                      : 1;  /**< Enables SLI_INT_SUM[2] to generate an
                                                         interrupt on the RSL. */
	uint64_t reserved_1_1                 : 1;
	uint64_t rml_to                       : 1;  /**< Enables SLI_INT_SUM[0] to generate an
                                                         interrupt on the RSL. */
#else
	uint64_t rml_to                       : 1;
	uint64_t reserved_1_1                 : 1;
	uint64_t bar0_to                      : 1;
	uint64_t iob2big                      : 1;
	uint64_t pcnt                         : 1;
	uint64_t ptime                        : 1;
	uint64_t reserved_6_7                 : 2;
	uint64_t m0_up_b0                     : 1;
	uint64_t m0_up_wi                     : 1;
	uint64_t m0_un_b0                     : 1;
	uint64_t m0_un_wi                     : 1;
	uint64_t m1_up_b0                     : 1;
	uint64_t m1_up_wi                     : 1;
	uint64_t m1_un_b0                     : 1;
	uint64_t m1_un_wi                     : 1;
	uint64_t mio_int0                     : 1;
	uint64_t mio_int1                     : 1;
	uint64_t reserved_18_31               : 14;
	uint64_t dmafi                        : 2;
	uint64_t dcnt                         : 2;
	uint64_t dtime                        : 2;
	uint64_t reserved_38_47               : 10;
	uint64_t pidbof                       : 1;
	uint64_t psldbof                      : 1;
	uint64_t pout_err                     : 1;
	uint64_t pin_bp                       : 1;
	uint64_t pgl_err                      : 1;
	uint64_t pdi_err                      : 1;
	uint64_t pop_err                      : 1;
	uint64_t pins_err                     : 1;
	uint64_t sprt0_err                    : 1;
	uint64_t sprt1_err                    : 1;
	uint64_t reserved_58_59               : 2;
	uint64_t ill_pad                      : 1;
	uint64_t reserved_61_63               : 3;
#endif
	} s;
	struct cvmx_sli_int_enb_ciu_s         cn63xx;
	struct cvmx_sli_int_enb_ciu_s         cn63xxp1;
};
typedef union cvmx_sli_int_enb_ciu cvmx_sli_int_enb_ciu_t;

/**
 * cvmx_sli_int_enb_port#
 *
 * SLI_INT_ENB_PORTX = SLI's Interrupt Enable Register per mac port
 *
 * Used to allow the generation of interrupts (MSI/INTA) to the PORT X
 *
 * Notes:
 * This CSR is not used when the corresponding MAC is sRIO.
 *
 */
union cvmx_sli_int_enb_portx
{
	uint64_t u64;
	struct cvmx_sli_int_enb_portx_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_61_63               : 3;
	uint64_t ill_pad                      : 1;  /**< Illegal packet csr address. */
	uint64_t reserved_58_59               : 2;
	uint64_t sprt1_err                    : 1;  /**< Error Response received on SLI port 1. */
	uint64_t sprt0_err                    : 1;  /**< Error Response received on SLI port 0. */
	uint64_t pins_err                     : 1;  /**< Read Error during packet instruction fetch. */
	uint64_t pop_err                      : 1;  /**< Read Error during packet scatter pointer fetch. */
	uint64_t pdi_err                      : 1;  /**< Read Error during packet data fetch. */
	uint64_t pgl_err                      : 1;  /**< Read Error during gather list fetch. */
	uint64_t pin_bp                       : 1;  /**< Packet Input Count exceeded WMARK. */
	uint64_t pout_err                     : 1;  /**< Packet Out Interrupt, Error From PKO. */
	uint64_t psldbof                      : 1;  /**< Packet Scatterlist Doorbell Count Overflow. */
	uint64_t pidbof                       : 1;  /**< Packet Instruction Doorbell Count Overflow. */
	uint64_t reserved_38_47               : 10;
	uint64_t dtime                        : 2;  /**< DMA Timer Interrupts */
	uint64_t dcnt                         : 2;  /**< DMA Count Interrupts */
	uint64_t dmafi                        : 2;  /**< DMA set Forced Interrupts */
	uint64_t reserved_20_31               : 12;
	uint64_t mac1_int                     : 1;  /**< Enables SLI_INT_SUM[19] to generate an
                                                         interrupt to the PCIE-Port1 for MSI/inta.
                                                         The valuse of this bit has NO effect on PCIE Port0.
                                                         SLI_INT_ENB_PORT0[MAC1_INT] sould NEVER be set. */
	uint64_t mac0_int                     : 1;  /**< Enables SLI_INT_SUM[18] to generate an
                                                         interrupt to the PCIE-Port0 for MSI/inta.
                                                         The valus of this bit has NO effect on PCIE Port1.
                                                         SLI_INT_ENB_PORT1[MAC0_INT] sould NEVER be set. */
	uint64_t mio_int1                     : 1;  /**< Enables SLI_INT_SUM[17] to generate an
                                                         interrupt to the PCIE core for MSI/inta.
                                                         SLI_INT_ENB_PORT0[MIO_INT1] should NEVER be set. */
	uint64_t mio_int0                     : 1;  /**< Enables SLI_INT_SUM[16] to generate an
                                                         interrupt to the PCIE core for MSI/inta.
                                                         SLI_INT_ENB_PORT1[MIO_INT0] should NEVER be set. */
	uint64_t m1_un_wi                     : 1;  /**< Enables SLI_INT_SUM[15] to generate an
                                                         interrupt to the PCIE core for MSI/inta. */
	uint64_t m1_un_b0                     : 1;  /**< Enables SLI_INT_SUM[14] to generate an
                                                         interrupt to the PCIE core for MSI/inta. */
	uint64_t m1_up_wi                     : 1;  /**< Enables SLI_INT_SUM[13] to generate an
                                                         interrupt to the PCIE core for MSI/inta. */
	uint64_t m1_up_b0                     : 1;  /**< Enables SLI_INT_SUM[12] to generate an
                                                         interrupt to the PCIE core for MSI/inta. */
	uint64_t m0_un_wi                     : 1;  /**< Enables SLI_INT_SUM[11] to generate an
                                                         interrupt to the PCIE core for MSI/inta. */
	uint64_t m0_un_b0                     : 1;  /**< Enables SLI_INT_SUM[10] to generate an
                                                         interrupt to the PCIE core for MSI/inta. */
	uint64_t m0_up_wi                     : 1;  /**< Enables SLI_INT_SUM[9] to generate an
                                                         interrupt to the PCIE core for MSI/inta. */
	uint64_t m0_up_b0                     : 1;  /**< Enables SLI_INT_SUM[8] to generate an
                                                         interrupt to the PCIE core for MSI/inta. */
	uint64_t reserved_6_7                 : 2;
	uint64_t ptime                        : 1;  /**< Enables SLI_INT_SUM[5] to generate an
                                                         interrupt to the PCIE core for MSI/inta. */
	uint64_t pcnt                         : 1;  /**< Enables SLI_INT_SUM[4] to generate an
                                                         interrupt to the PCIE core for MSI/inta. */
	uint64_t iob2big                      : 1;  /**< Enables SLI_INT_SUM[3] to generate an
                                                         interrupt to the PCIE core for MSI/inta. */
	uint64_t bar0_to                      : 1;  /**< Enables SLI_INT_SUM[2] to generate an
                                                         interrupt to the PCIE core for MSI/inta. */
	uint64_t reserved_1_1                 : 1;
	uint64_t rml_to                       : 1;  /**< Enables SLI_INT_SUM[0] to generate an
                                                         interrupt to the PCIE core for MSI/inta. */
#else
	uint64_t rml_to                       : 1;
	uint64_t reserved_1_1                 : 1;
	uint64_t bar0_to                      : 1;
	uint64_t iob2big                      : 1;
	uint64_t pcnt                         : 1;
	uint64_t ptime                        : 1;
	uint64_t reserved_6_7                 : 2;
	uint64_t m0_up_b0                     : 1;
	uint64_t m0_up_wi                     : 1;
	uint64_t m0_un_b0                     : 1;
	uint64_t m0_un_wi                     : 1;
	uint64_t m1_up_b0                     : 1;
	uint64_t m1_up_wi                     : 1;
	uint64_t m1_un_b0                     : 1;
	uint64_t m1_un_wi                     : 1;
	uint64_t mio_int0                     : 1;
	uint64_t mio_int1                     : 1;
	uint64_t mac0_int                     : 1;
	uint64_t mac1_int                     : 1;
	uint64_t reserved_20_31               : 12;
	uint64_t dmafi                        : 2;
	uint64_t dcnt                         : 2;
	uint64_t dtime                        : 2;
	uint64_t reserved_38_47               : 10;
	uint64_t pidbof                       : 1;
	uint64_t psldbof                      : 1;
	uint64_t pout_err                     : 1;
	uint64_t pin_bp                       : 1;
	uint64_t pgl_err                      : 1;
	uint64_t pdi_err                      : 1;
	uint64_t pop_err                      : 1;
	uint64_t pins_err                     : 1;
	uint64_t sprt0_err                    : 1;
	uint64_t sprt1_err                    : 1;
	uint64_t reserved_58_59               : 2;
	uint64_t ill_pad                      : 1;
	uint64_t reserved_61_63               : 3;
#endif
	} s;
	struct cvmx_sli_int_enb_portx_s       cn63xx;
	struct cvmx_sli_int_enb_portx_s       cn63xxp1;
};
typedef union cvmx_sli_int_enb_portx cvmx_sli_int_enb_portx_t;

/**
 * cvmx_sli_int_sum
 *
 * SLI_INT_SUM = SLI Interrupt Summary Register
 *
 * Set when an interrupt condition occurs, write '1' to clear.
 */
union cvmx_sli_int_sum
{
	uint64_t u64;
	struct cvmx_sli_int_sum_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_61_63               : 3;
	uint64_t ill_pad                      : 1;  /**< Set when a BAR0 address R/W falls into theaddress
                                                         range of the Packet-CSR, but for an unused
                                                         address. */
	uint64_t reserved_58_59               : 2;
	uint64_t sprt1_err                    : 1;  /**< When an error response received on SLI port 1
                                                         this bit is set. */
	uint64_t sprt0_err                    : 1;  /**< When an error response received on SLI port 0
                                                         this bit is set. */
	uint64_t pins_err                     : 1;  /**< When a read error occurs on a packet instruction
                                                         this bit is set. */
	uint64_t pop_err                      : 1;  /**< When a read error occurs on a packet scatter
                                                         pointer pair this bit is set. */
	uint64_t pdi_err                      : 1;  /**< When a read error occurs on a packet data read
                                                         this bit is set. */
	uint64_t pgl_err                      : 1;  /**< When a read error occurs on a packet gather list
                                                         read this bit is set. */
	uint64_t pin_bp                       : 1;  /**< Packet input count has exceeded the WMARK.
                                                         See SLI_PKT_IN_BP */
	uint64_t pout_err                     : 1;  /**< Set when PKO sends packet data with the error bit
                                                         set. */
	uint64_t psldbof                      : 1;  /**< Packet Scatterlist Doorbell count overflowed. Which
                                                         doorbell can be found in DPI_PINT_INFO[PSLDBOF] */
	uint64_t pidbof                       : 1;  /**< Packet Instruction Doorbell count overflowed. Which
                                                         doorbell can be found in DPI_PINT_INFO[PIDBOF] */
	uint64_t reserved_38_47               : 10;
	uint64_t dtime                        : 2;  /**< Whenever SLI_DMAx_CNT[CNT] is not 0, the
                                                         SLI_DMAx_TIM[TIM] timer increments every SLI
                                                         clock.
                                                         DTIME[x] is set whenever SLI_DMAx_TIM[TIM] >
                                                         SLI_DMAx_INT_LEVEL[TIME].
                                                         DTIME[x] is normally cleared by clearing
                                                         SLI_DMAx_CNT[CNT] (which also clears
                                                         SLI_DMAx_TIM[TIM]). */
	uint64_t dcnt                         : 2;  /**< DCNT[x] is set whenever SLI_DMAx_CNT[CNT] >
                                                         SLI_DMAx_INT_LEVEL[CNT].
                                                         DCNT[x] is normally cleared by decreasing
                                                         SLI_DMAx_CNT[CNT]. */
	uint64_t dmafi                        : 2;  /**< DMA set Forced Interrupts. */
	uint64_t reserved_20_31               : 12;
	uint64_t mac1_int                     : 1;  /**< Interrupt from MAC1.
                                                         See PEM1_INT_SUM (enabled by PEM1_INT_ENB_INT) */
	uint64_t mac0_int                     : 1;  /**< Interrupt from MAC0.
                                                         See PEM0_INT_SUM (enabled by PEM0_INT_ENB_INT) */
	uint64_t mio_int1                     : 1;  /**< Interrupt from MIO for PORT 1.
                                                         See CIU_INT33_SUM0, CIU_INT_SUM1
                                                         (enabled by CIU_INT33_EN0, CIU_INT33_EN1) */
	uint64_t mio_int0                     : 1;  /**< Interrupt from MIO for PORT 0.
                                                         See CIU_INT32_SUM0, CIU_INT_SUM1
                                                         (enabled by CIU_INT32_EN0, CIU_INT32_EN1) */
	uint64_t m1_un_wi                     : 1;  /**< Received Unsupported N-TLP for Window Register
                                                         from MAC 1. This occurs when the window registers
                                                         are disabeld and a window register access occurs. */
	uint64_t m1_un_b0                     : 1;  /**< Received Unsupported N-TLP for Bar0 from MAC 1.
                                                         This occurs when the BAR 0 address space is
                                                         disabeled. */
	uint64_t m1_up_wi                     : 1;  /**< Received Unsupported P-TLP for Window Register
                                                         from MAC 1. This occurs when the window registers
                                                         are disabeld and a window register access occurs. */
	uint64_t m1_up_b0                     : 1;  /**< Received Unsupported P-TLP for Bar0 from MAC 1.
                                                         This occurs when the BAR 0 address space is
                                                         disabeled. */
	uint64_t m0_un_wi                     : 1;  /**< Received Unsupported N-TLP for Window Register
                                                         from MAC 0. This occurs when the window registers
                                                         are disabeld and a window register access occurs. */
	uint64_t m0_un_b0                     : 1;  /**< Received Unsupported N-TLP for Bar0 from MAC 0.
                                                         This occurs when the BAR 0 address space is
                                                         disabeled. */
	uint64_t m0_up_wi                     : 1;  /**< Received Unsupported P-TLP for Window Register
                                                         from MAC 0. This occurs when the window registers
                                                         are disabeld and a window register access occurs. */
	uint64_t m0_up_b0                     : 1;  /**< Received Unsupported P-TLP for Bar0 from MAC 0.
                                                         This occurs when the BAR 0 address space is
                                                         disabeled. */
	uint64_t reserved_6_7                 : 2;
	uint64_t ptime                        : 1;  /**< Packet Timer has an interrupt. Which rings can
                                                         be found in SLI_PKT_TIME_INT. */
	uint64_t pcnt                         : 1;  /**< Packet Counter has an interrupt. Which rings can
                                                         be found in SLI_PKT_CNT_INT. */
	uint64_t iob2big                      : 1;  /**< A requested IOBDMA is to large. */
	uint64_t bar0_to                      : 1;  /**< BAR0 R/W to a NCB device did not receive
                                                         read-data/commit in 0xffff core clocks. */
	uint64_t reserved_1_1                 : 1;
	uint64_t rml_to                       : 1;  /**< A read or write transfer did not complete
                                                         within 0xffff core clocks. */
#else
	uint64_t rml_to                       : 1;
	uint64_t reserved_1_1                 : 1;
	uint64_t bar0_to                      : 1;
	uint64_t iob2big                      : 1;
	uint64_t pcnt                         : 1;
	uint64_t ptime                        : 1;
	uint64_t reserved_6_7                 : 2;
	uint64_t m0_up_b0                     : 1;
	uint64_t m0_up_wi                     : 1;
	uint64_t m0_un_b0                     : 1;
	uint64_t m0_un_wi                     : 1;
	uint64_t m1_up_b0                     : 1;
	uint64_t m1_up_wi                     : 1;
	uint64_t m1_un_b0                     : 1;
	uint64_t m1_un_wi                     : 1;
	uint64_t mio_int0                     : 1;
	uint64_t mio_int1                     : 1;
	uint64_t mac0_int                     : 1;
	uint64_t mac1_int                     : 1;
	uint64_t reserved_20_31               : 12;
	uint64_t dmafi                        : 2;
	uint64_t dcnt                         : 2;
	uint64_t dtime                        : 2;
	uint64_t reserved_38_47               : 10;
	uint64_t pidbof                       : 1;
	uint64_t psldbof                      : 1;
	uint64_t pout_err                     : 1;
	uint64_t pin_bp                       : 1;
	uint64_t pgl_err                      : 1;
	uint64_t pdi_err                      : 1;
	uint64_t pop_err                      : 1;
	uint64_t pins_err                     : 1;
	uint64_t sprt0_err                    : 1;
	uint64_t sprt1_err                    : 1;
	uint64_t reserved_58_59               : 2;
	uint64_t ill_pad                      : 1;
	uint64_t reserved_61_63               : 3;
#endif
	} s;
	struct cvmx_sli_int_sum_s             cn63xx;
	struct cvmx_sli_int_sum_s             cn63xxp1;
};
typedef union cvmx_sli_int_sum cvmx_sli_int_sum_t;

/**
 * cvmx_sli_last_win_rdata0
 *
 * SLI_LAST_WIN_RDATA0 = SLI Last Window Read Data Port0
 *
 * The data from the last initiated window read.
 */
union cvmx_sli_last_win_rdata0
{
	uint64_t u64;
	struct cvmx_sli_last_win_rdata0_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t data                         : 64; /**< Last window read data. */
#else
	uint64_t data                         : 64;
#endif
	} s;
	struct cvmx_sli_last_win_rdata0_s     cn63xx;
	struct cvmx_sli_last_win_rdata0_s     cn63xxp1;
};
typedef union cvmx_sli_last_win_rdata0 cvmx_sli_last_win_rdata0_t;

/**
 * cvmx_sli_last_win_rdata1
 *
 * SLI_LAST_WIN_RDATA1 = SLI Last Window Read Data Port1
 *
 * The data from the last initiated window read.
 */
union cvmx_sli_last_win_rdata1
{
	uint64_t u64;
	struct cvmx_sli_last_win_rdata1_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t data                         : 64; /**< Last window read data. */
#else
	uint64_t data                         : 64;
#endif
	} s;
	struct cvmx_sli_last_win_rdata1_s     cn63xx;
	struct cvmx_sli_last_win_rdata1_s     cn63xxp1;
};
typedef union cvmx_sli_last_win_rdata1 cvmx_sli_last_win_rdata1_t;

/**
 * cvmx_sli_mac_credit_cnt
 *
 * SLI_MAC_CREDIT_CNT = SLI MAC Credit Count
 *
 * Contains the number of credits for the MAC port FIFOs used by the SLI. This value needs to be set BEFORE S2M traffic
 * flow starts. A write to this register will cause the credit counts in the SLI for the MAC ports to be reset to the value
 * in this register.
 */
union cvmx_sli_mac_credit_cnt
{
	uint64_t u64;
	struct cvmx_sli_mac_credit_cnt_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_54_63               : 10;
	uint64_t p1_c_d                       : 1;  /**< When set does not allow writing of P1_CCNT. */
	uint64_t p1_n_d                       : 1;  /**< When set does not allow writing of P1_NCNT. */
	uint64_t p1_p_d                       : 1;  /**< When set does not allow writing of P1_PCNT. */
	uint64_t p0_c_d                       : 1;  /**< When set does not allow writing of P0_CCNT. */
	uint64_t p0_n_d                       : 1;  /**< When set does not allow writing of P0_NCNT. */
	uint64_t p0_p_d                       : 1;  /**< When set does not allow writing of P0_PCNT. */
	uint64_t p1_ccnt                      : 8;  /**< Port1 C-TLP FIFO Credits.
                                                         Legal values are 0x25 to 0x80. */
	uint64_t p1_ncnt                      : 8;  /**< Port1 N-TLP FIFO Credits.
                                                         Legal values are 0x5 to 0x10. */
	uint64_t p1_pcnt                      : 8;  /**< Port1 P-TLP FIFO Credits.
                                                         Legal values are 0x25 to 0x80. */
	uint64_t p0_ccnt                      : 8;  /**< Port0 C-TLP FIFO Credits.
                                                         Legal values are 0x25 to 0x80. */
	uint64_t p0_ncnt                      : 8;  /**< Port0 N-TLP FIFO Credits.
                                                         Legal values are 0x5 to 0x10. */
	uint64_t p0_pcnt                      : 8;  /**< Port0 P-TLP FIFO Credits.
                                                         Legal values are 0x25 to 0x80. */
#else
	uint64_t p0_pcnt                      : 8;
	uint64_t p0_ncnt                      : 8;
	uint64_t p0_ccnt                      : 8;
	uint64_t p1_pcnt                      : 8;
	uint64_t p1_ncnt                      : 8;
	uint64_t p1_ccnt                      : 8;
	uint64_t p0_p_d                       : 1;
	uint64_t p0_n_d                       : 1;
	uint64_t p0_c_d                       : 1;
	uint64_t p1_p_d                       : 1;
	uint64_t p1_n_d                       : 1;
	uint64_t p1_c_d                       : 1;
	uint64_t reserved_54_63               : 10;
#endif
	} s;
	struct cvmx_sli_mac_credit_cnt_s      cn63xx;
	struct cvmx_sli_mac_credit_cnt_cn63xxp1
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_48_63               : 16;
	uint64_t p1_ccnt                      : 8;  /**< Port1 C-TLP FIFO Credits.
                                                         Legal values are 0x25 to 0x80. */
	uint64_t p1_ncnt                      : 8;  /**< Port1 N-TLP FIFO Credits.
                                                         Legal values are 0x5 to 0x10. */
	uint64_t p1_pcnt                      : 8;  /**< Port1 P-TLP FIFO Credits.
                                                         Legal values are 0x25 to 0x80. */
	uint64_t p0_ccnt                      : 8;  /**< Port0 C-TLP FIFO Credits.
                                                         Legal values are 0x25 to 0x80. */
	uint64_t p0_ncnt                      : 8;  /**< Port0 N-TLP FIFO Credits.
                                                         Legal values are 0x5 to 0x10. */
	uint64_t p0_pcnt                      : 8;  /**< Port0 P-TLP FIFO Credits.
                                                         Legal values are 0x25 to 0x80. */
#else
	uint64_t p0_pcnt                      : 8;
	uint64_t p0_ncnt                      : 8;
	uint64_t p0_ccnt                      : 8;
	uint64_t p1_pcnt                      : 8;
	uint64_t p1_ncnt                      : 8;
	uint64_t p1_ccnt                      : 8;
	uint64_t reserved_48_63               : 16;
#endif
	} cn63xxp1;
};
typedef union cvmx_sli_mac_credit_cnt cvmx_sli_mac_credit_cnt_t;

/**
 * cvmx_sli_mac_number
 *
 * 0x13DA0 - 0x13DF0 reserved for ports 2 - 7
 *
 *                  SLI_MAC_NUMBER = SLI MAC Number
 *
 * When read from a MAC port it returns the MAC's port number.
 * register.
 */
union cvmx_sli_mac_number
{
	uint64_t u64;
	struct cvmx_sli_mac_number_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_8_63                : 56;
	uint64_t num                          : 8;  /**< The mac number. */
#else
	uint64_t num                          : 8;
	uint64_t reserved_8_63                : 56;
#endif
	} s;
	struct cvmx_sli_mac_number_s          cn63xx;
};
typedef union cvmx_sli_mac_number cvmx_sli_mac_number_t;

/**
 * cvmx_sli_mem_access_ctl
 *
 * SLI_MEM_ACCESS_CTL = SLI's Memory Access Control
 *
 * Contains control for access to the MAC address space.
 */
union cvmx_sli_mem_access_ctl
{
	uint64_t u64;
	struct cvmx_sli_mem_access_ctl_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_14_63               : 50;
	uint64_t max_word                     : 4;  /**< The maximum number of words to merge into a single
                                                         write operation from the PPs to the MAC. Legal
                                                         values are 1 to 16, where a '0' is treated as 16. */
	uint64_t timer                        : 10; /**< When the SLI starts a PP to MAC write it waits
                                                         no longer than the value of TIMER in eclks to
                                                         merge additional writes from the PPs into 1
                                                         large write. The values for this field is 1 to
                                                         1024 where a value of '0' is treated as 1024. */
#else
	uint64_t timer                        : 10;
	uint64_t max_word                     : 4;
	uint64_t reserved_14_63               : 50;
#endif
	} s;
	struct cvmx_sli_mem_access_ctl_s      cn63xx;
	struct cvmx_sli_mem_access_ctl_s      cn63xxp1;
};
typedef union cvmx_sli_mem_access_ctl cvmx_sli_mem_access_ctl_t;

/**
 * cvmx_sli_mem_access_subid#
 *
 * // *
 * // * 8070 - 80C0 saved for ports 2 through 7
 * // *
 * // *
 * // * 0x80d0 free
 * // *
 *
 *                   SLI_MEM_ACCESS_SUBIDX = SLI Memory Access SubidX Register
 *
 *  Contains address index and control bits for access to memory from Core PPs.
 */
union cvmx_sli_mem_access_subidx
{
	uint64_t u64;
	struct cvmx_sli_mem_access_subidx_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_43_63               : 21;
	uint64_t zero                         : 1;  /**< Causes all byte reads to be zero length reads.
                                                         Returns to the EXEC a zero for all read data.
                                                         This must be zero for sRIO ports. */
	uint64_t port                         : 3;  /**< Physical MAC Port that reads/writes to
                                                         this subid are sent to. Must be <= 1, as there are
                                                         only two ports present. */
	uint64_t nmerge                       : 1;  /**< When set, no merging is allowed in this window. */
	uint64_t esr                          : 2;  /**< ES<1:0> for reads to this subid.
                                                         ES<1:0> is the endian-swap attribute for these MAC
                                                         memory space reads. */
	uint64_t esw                          : 2;  /**< ES<1:0> for writes to this subid.
                                                         ES<1:0> is the endian-swap attribute for these MAC
                                                         memory space writes. */
	uint64_t wtype                        : 2;  /**< ADDRTYPE<1:0> for writes to this subid
                                                         For PCIe:
                                                         - ADDRTYPE<0> is the relaxed-order attribute
                                                         - ADDRTYPE<1> is the no-snoop attribute
                                                         For sRIO:
                                                         - ADDRTYPE<1:0> help select an SRIO*_S2M_TYPE*
                                                           entry */
	uint64_t rtype                        : 2;  /**< ADDRTYPE<1:0> for reads to this subid
                                                         For PCIe:
                                                         - ADDRTYPE<0> is the relaxed-order attribute
                                                         - ADDRTYPE<1> is the no-snoop attribute
                                                         For sRIO:
                                                         - ADDRTYPE<1:0> help select an SRIO*_S2M_TYPE*
                                                           entry */
	uint64_t ba                           : 30; /**< Address Bits <63:34> for reads/writes that use
                                                         this subid. */
#else
	uint64_t ba                           : 30;
	uint64_t rtype                        : 2;
	uint64_t wtype                        : 2;
	uint64_t esw                          : 2;
	uint64_t esr                          : 2;
	uint64_t nmerge                       : 1;
	uint64_t port                         : 3;
	uint64_t zero                         : 1;
	uint64_t reserved_43_63               : 21;
#endif
	} s;
	struct cvmx_sli_mem_access_subidx_s   cn63xx;
	struct cvmx_sli_mem_access_subidx_s   cn63xxp1;
};
typedef union cvmx_sli_mem_access_subidx cvmx_sli_mem_access_subidx_t;

/**
 * cvmx_sli_msi_enb0
 *
 * SLI_MSI_ENB0 = SLI MSI Enable0
 *
 * Used to enable the interrupt generation for the bits in the SLI_MSI_RCV0.
 */
union cvmx_sli_msi_enb0
{
	uint64_t u64;
	struct cvmx_sli_msi_enb0_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t enb                          : 64; /**< Enables bit [63:0] of SLI_MSI_RCV0. */
#else
	uint64_t enb                          : 64;
#endif
	} s;
	struct cvmx_sli_msi_enb0_s            cn63xx;
	struct cvmx_sli_msi_enb0_s            cn63xxp1;
};
typedef union cvmx_sli_msi_enb0 cvmx_sli_msi_enb0_t;

/**
 * cvmx_sli_msi_enb1
 *
 * SLI_MSI_ENB1 = SLI MSI Enable1
 *
 * Used to enable the interrupt generation for the bits in the SLI_MSI_RCV1.
 */
union cvmx_sli_msi_enb1
{
	uint64_t u64;
	struct cvmx_sli_msi_enb1_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t enb                          : 64; /**< Enables bit [63:0] of SLI_MSI_RCV1. */
#else
	uint64_t enb                          : 64;
#endif
	} s;
	struct cvmx_sli_msi_enb1_s            cn63xx;
	struct cvmx_sli_msi_enb1_s            cn63xxp1;
};
typedef union cvmx_sli_msi_enb1 cvmx_sli_msi_enb1_t;

/**
 * cvmx_sli_msi_enb2
 *
 * SLI_MSI_ENB2 = SLI MSI Enable2
 *
 * Used to enable the interrupt generation for the bits in the SLI_MSI_RCV2.
 */
union cvmx_sli_msi_enb2
{
	uint64_t u64;
	struct cvmx_sli_msi_enb2_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t enb                          : 64; /**< Enables bit [63:0] of SLI_MSI_RCV2. */
#else
	uint64_t enb                          : 64;
#endif
	} s;
	struct cvmx_sli_msi_enb2_s            cn63xx;
	struct cvmx_sli_msi_enb2_s            cn63xxp1;
};
typedef union cvmx_sli_msi_enb2 cvmx_sli_msi_enb2_t;

/**
 * cvmx_sli_msi_enb3
 *
 * SLI_MSI_ENB3 = SLI MSI Enable3
 *
 * Used to enable the interrupt generation for the bits in the SLI_MSI_RCV3.
 */
union cvmx_sli_msi_enb3
{
	uint64_t u64;
	struct cvmx_sli_msi_enb3_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t enb                          : 64; /**< Enables bit [63:0] of SLI_MSI_RCV3. */
#else
	uint64_t enb                          : 64;
#endif
	} s;
	struct cvmx_sli_msi_enb3_s            cn63xx;
	struct cvmx_sli_msi_enb3_s            cn63xxp1;
};
typedef union cvmx_sli_msi_enb3 cvmx_sli_msi_enb3_t;

/**
 * cvmx_sli_msi_rcv0
 *
 * SLI_MSI_RCV0 = SLI MSI Receive0
 *
 * Contains bits [63:0] of the 256 bits of MSI interrupts.
 */
union cvmx_sli_msi_rcv0
{
	uint64_t u64;
	struct cvmx_sli_msi_rcv0_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t intr                         : 64; /**< Bits 63-0 of the 256 bits of MSI interrupt. */
#else
	uint64_t intr                         : 64;
#endif
	} s;
	struct cvmx_sli_msi_rcv0_s            cn63xx;
	struct cvmx_sli_msi_rcv0_s            cn63xxp1;
};
typedef union cvmx_sli_msi_rcv0 cvmx_sli_msi_rcv0_t;

/**
 * cvmx_sli_msi_rcv1
 *
 * SLI_MSI_RCV1 = SLI MSI Receive1
 *
 * Contains bits [127:64] of the 256 bits of MSI interrupts.
 */
union cvmx_sli_msi_rcv1
{
	uint64_t u64;
	struct cvmx_sli_msi_rcv1_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t intr                         : 64; /**< Bits 127-64 of the 256 bits of MSI interrupt. */
#else
	uint64_t intr                         : 64;
#endif
	} s;
	struct cvmx_sli_msi_rcv1_s            cn63xx;
	struct cvmx_sli_msi_rcv1_s            cn63xxp1;
};
typedef union cvmx_sli_msi_rcv1 cvmx_sli_msi_rcv1_t;

/**
 * cvmx_sli_msi_rcv2
 *
 * SLI_MSI_RCV2 = SLI MSI Receive2
 *
 * Contains bits [191:128] of the 256 bits of MSI interrupts.
 */
union cvmx_sli_msi_rcv2
{
	uint64_t u64;
	struct cvmx_sli_msi_rcv2_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t intr                         : 64; /**< Bits 191-128 of the 256 bits of MSI interrupt. */
#else
	uint64_t intr                         : 64;
#endif
	} s;
	struct cvmx_sli_msi_rcv2_s            cn63xx;
	struct cvmx_sli_msi_rcv2_s            cn63xxp1;
};
typedef union cvmx_sli_msi_rcv2 cvmx_sli_msi_rcv2_t;

/**
 * cvmx_sli_msi_rcv3
 *
 * SLI_MSI_RCV3 = SLI MSI Receive3
 *
 * Contains bits [255:192] of the 256 bits of MSI interrupts.
 */
union cvmx_sli_msi_rcv3
{
	uint64_t u64;
	struct cvmx_sli_msi_rcv3_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t intr                         : 64; /**< Bits 255-192 of the 256 bits of MSI interrupt. */
#else
	uint64_t intr                         : 64;
#endif
	} s;
	struct cvmx_sli_msi_rcv3_s            cn63xx;
	struct cvmx_sli_msi_rcv3_s            cn63xxp1;
};
typedef union cvmx_sli_msi_rcv3 cvmx_sli_msi_rcv3_t;

/**
 * cvmx_sli_msi_rd_map
 *
 * SLI_MSI_RD_MAP = SLI MSI Read MAP
 *
 * Used to read the mapping function of the SLI_PCIE_MSI_RCV to SLI_MSI_RCV registers.
 */
union cvmx_sli_msi_rd_map
{
	uint64_t u64;
	struct cvmx_sli_msi_rd_map_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_16_63               : 48;
	uint64_t rd_int                       : 8;  /**< The value of the map at the location PREVIOUSLY
                                                         written to the MSI_INT field of this register. */
	uint64_t msi_int                      : 8;  /**< Selects the value that would be received when the
                                                         SLI_PCIE_MSI_RCV register is written. */
#else
	uint64_t msi_int                      : 8;
	uint64_t rd_int                       : 8;
	uint64_t reserved_16_63               : 48;
#endif
	} s;
	struct cvmx_sli_msi_rd_map_s          cn63xx;
	struct cvmx_sli_msi_rd_map_s          cn63xxp1;
};
typedef union cvmx_sli_msi_rd_map cvmx_sli_msi_rd_map_t;

/**
 * cvmx_sli_msi_w1c_enb0
 *
 * SLI_MSI_W1C_ENB0 = SLI MSI Write 1 To Clear Enable0
 *
 * Used to clear bits in SLI_MSI_ENB0.
 */
union cvmx_sli_msi_w1c_enb0
{
	uint64_t u64;
	struct cvmx_sli_msi_w1c_enb0_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t clr                          : 64; /**< A write of '1' to a vector will clear the
                                                         cooresponding bit in SLI_MSI_ENB0.
                                                         A read to this address will return 0. */
#else
	uint64_t clr                          : 64;
#endif
	} s;
	struct cvmx_sli_msi_w1c_enb0_s        cn63xx;
	struct cvmx_sli_msi_w1c_enb0_s        cn63xxp1;
};
typedef union cvmx_sli_msi_w1c_enb0 cvmx_sli_msi_w1c_enb0_t;

/**
 * cvmx_sli_msi_w1c_enb1
 *
 * SLI_MSI_W1C_ENB1 = SLI MSI Write 1 To Clear Enable1
 *
 * Used to clear bits in SLI_MSI_ENB1.
 */
union cvmx_sli_msi_w1c_enb1
{
	uint64_t u64;
	struct cvmx_sli_msi_w1c_enb1_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t clr                          : 64; /**< A write of '1' to a vector will clear the
                                                         cooresponding bit in SLI_MSI_ENB1.
                                                         A read to this address will return 0. */
#else
	uint64_t clr                          : 64;
#endif
	} s;
	struct cvmx_sli_msi_w1c_enb1_s        cn63xx;
	struct cvmx_sli_msi_w1c_enb1_s        cn63xxp1;
};
typedef union cvmx_sli_msi_w1c_enb1 cvmx_sli_msi_w1c_enb1_t;

/**
 * cvmx_sli_msi_w1c_enb2
 *
 * SLI_MSI_W1C_ENB2 = SLI MSI Write 1 To Clear Enable2
 *
 * Used to clear bits in SLI_MSI_ENB2.
 */
union cvmx_sli_msi_w1c_enb2
{
	uint64_t u64;
	struct cvmx_sli_msi_w1c_enb2_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t clr                          : 64; /**< A write of '1' to a vector will clear the
                                                         cooresponding bit in SLI_MSI_ENB2.
                                                         A read to this address will return 0. */
#else
	uint64_t clr                          : 64;
#endif
	} s;
	struct cvmx_sli_msi_w1c_enb2_s        cn63xx;
	struct cvmx_sli_msi_w1c_enb2_s        cn63xxp1;
};
typedef union cvmx_sli_msi_w1c_enb2 cvmx_sli_msi_w1c_enb2_t;

/**
 * cvmx_sli_msi_w1c_enb3
 *
 * SLI_MSI_W1C_ENB3 = SLI MSI Write 1 To Clear Enable3
 *
 * Used to clear bits in SLI_MSI_ENB3.
 */
union cvmx_sli_msi_w1c_enb3
{
	uint64_t u64;
	struct cvmx_sli_msi_w1c_enb3_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t clr                          : 64; /**< A write of '1' to a vector will clear the
                                                         cooresponding bit in SLI_MSI_ENB3.
                                                         A read to this address will return 0. */
#else
	uint64_t clr                          : 64;
#endif
	} s;
	struct cvmx_sli_msi_w1c_enb3_s        cn63xx;
	struct cvmx_sli_msi_w1c_enb3_s        cn63xxp1;
};
typedef union cvmx_sli_msi_w1c_enb3 cvmx_sli_msi_w1c_enb3_t;

/**
 * cvmx_sli_msi_w1s_enb0
 *
 * SLI_MSI_W1S_ENB0 = SLI MSI Write 1 To Set Enable0
 *
 * Used to set bits in SLI_MSI_ENB0.
 */
union cvmx_sli_msi_w1s_enb0
{
	uint64_t u64;
	struct cvmx_sli_msi_w1s_enb0_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t set                          : 64; /**< A write of '1' to a vector will set the
                                                         cooresponding bit in SLI_MSI_ENB0.
                                                         A read to this address will return 0. */
#else
	uint64_t set                          : 64;
#endif
	} s;
	struct cvmx_sli_msi_w1s_enb0_s        cn63xx;
	struct cvmx_sli_msi_w1s_enb0_s        cn63xxp1;
};
typedef union cvmx_sli_msi_w1s_enb0 cvmx_sli_msi_w1s_enb0_t;

/**
 * cvmx_sli_msi_w1s_enb1
 *
 * SLI_MSI_W1S_ENB0 = SLI MSI Write 1 To Set Enable1
 *
 * Used to set bits in SLI_MSI_ENB1.
 */
union cvmx_sli_msi_w1s_enb1
{
	uint64_t u64;
	struct cvmx_sli_msi_w1s_enb1_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t set                          : 64; /**< A write of '1' to a vector will set the
                                                         cooresponding bit in SLI_MSI_ENB1.
                                                         A read to this address will return 0. */
#else
	uint64_t set                          : 64;
#endif
	} s;
	struct cvmx_sli_msi_w1s_enb1_s        cn63xx;
	struct cvmx_sli_msi_w1s_enb1_s        cn63xxp1;
};
typedef union cvmx_sli_msi_w1s_enb1 cvmx_sli_msi_w1s_enb1_t;

/**
 * cvmx_sli_msi_w1s_enb2
 *
 * SLI_MSI_W1S_ENB2 = SLI MSI Write 1 To Set Enable2
 *
 * Used to set bits in SLI_MSI_ENB2.
 */
union cvmx_sli_msi_w1s_enb2
{
	uint64_t u64;
	struct cvmx_sli_msi_w1s_enb2_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t set                          : 64; /**< A write of '1' to a vector will set the
                                                         cooresponding bit in SLI_MSI_ENB2.
                                                         A read to this address will return 0. */
#else
	uint64_t set                          : 64;
#endif
	} s;
	struct cvmx_sli_msi_w1s_enb2_s        cn63xx;
	struct cvmx_sli_msi_w1s_enb2_s        cn63xxp1;
};
typedef union cvmx_sli_msi_w1s_enb2 cvmx_sli_msi_w1s_enb2_t;

/**
 * cvmx_sli_msi_w1s_enb3
 *
 * SLI_MSI_W1S_ENB3 = SLI MSI Write 1 To Set Enable3
 *
 * Used to set bits in SLI_MSI_ENB3.
 */
union cvmx_sli_msi_w1s_enb3
{
	uint64_t u64;
	struct cvmx_sli_msi_w1s_enb3_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t set                          : 64; /**< A write of '1' to a vector will set the
                                                         cooresponding bit in SLI_MSI_ENB3.
                                                         A read to this address will return 0. */
#else
	uint64_t set                          : 64;
#endif
	} s;
	struct cvmx_sli_msi_w1s_enb3_s        cn63xx;
	struct cvmx_sli_msi_w1s_enb3_s        cn63xxp1;
};
typedef union cvmx_sli_msi_w1s_enb3 cvmx_sli_msi_w1s_enb3_t;

/**
 * cvmx_sli_msi_wr_map
 *
 * SLI_MSI_WR_MAP = SLI MSI Write MAP
 *
 * Used to write the mapping function of the SLI_PCIE_MSI_RCV to SLI_MSI_RCV registers.
 */
union cvmx_sli_msi_wr_map
{
	uint64_t u64;
	struct cvmx_sli_msi_wr_map_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_16_63               : 48;
	uint64_t ciu_int                      : 8;  /**< Selects which bit in the SLI_MSI_RCV# (0-255)
                                                         will be set when the value specified in the
                                                         MSI_INT of this register is recevied during a
                                                         write to the SLI_PCIE_MSI_RCV register. */
	uint64_t msi_int                      : 8;  /**< Selects the value that would be received when the
                                                         SLI_PCIE_MSI_RCV register is written. */
#else
	uint64_t msi_int                      : 8;
	uint64_t ciu_int                      : 8;
	uint64_t reserved_16_63               : 48;
#endif
	} s;
	struct cvmx_sli_msi_wr_map_s          cn63xx;
	struct cvmx_sli_msi_wr_map_s          cn63xxp1;
};
typedef union cvmx_sli_msi_wr_map cvmx_sli_msi_wr_map_t;

/**
 * cvmx_sli_pcie_msi_rcv
 *
 * SLI_PCIE_MSI_RCV = SLI MAC MSI Receive
 *
 * Register where MSI writes are directed from the MAC.
 */
union cvmx_sli_pcie_msi_rcv
{
	uint64_t u64;
	struct cvmx_sli_pcie_msi_rcv_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_8_63                : 56;
	uint64_t intr                         : 8;  /**< A write to this register will result in a bit in
                                                         one of the SLI_MSI_RCV# registers being set.
                                                         Which bit is set is dependent on the previously
                                                         written using the SLI_MSI_WR_MAP register or if
                                                         not previously written the reset value of the MAP. */
#else
	uint64_t intr                         : 8;
	uint64_t reserved_8_63                : 56;
#endif
	} s;
	struct cvmx_sli_pcie_msi_rcv_s        cn63xx;
	struct cvmx_sli_pcie_msi_rcv_s        cn63xxp1;
};
typedef union cvmx_sli_pcie_msi_rcv cvmx_sli_pcie_msi_rcv_t;

/**
 * cvmx_sli_pcie_msi_rcv_b1
 *
 * SLI_PCIE_MSI_RCV_B1 = SLI MAC MSI Receive Byte 1
 *
 * Register where MSI writes are directed from the MAC.
 *
 * Notes:
 * This CSR can be used by PCIe and sRIO MACs.
 *
 */
union cvmx_sli_pcie_msi_rcv_b1
{
	uint64_t u64;
	struct cvmx_sli_pcie_msi_rcv_b1_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_16_63               : 48;
	uint64_t intr                         : 8;  /**< A write to this register will result in a bit in
                                                         one of the SLI_MSI_RCV# registers being set.
                                                         Which bit is set is dependent on the previously
                                                         written using the SLI_MSI_WR_MAP register or if
                                                         not previously written the reset value of the MAP. */
	uint64_t reserved_0_7                 : 8;
#else
	uint64_t reserved_0_7                 : 8;
	uint64_t intr                         : 8;
	uint64_t reserved_16_63               : 48;
#endif
	} s;
	struct cvmx_sli_pcie_msi_rcv_b1_s     cn63xx;
	struct cvmx_sli_pcie_msi_rcv_b1_s     cn63xxp1;
};
typedef union cvmx_sli_pcie_msi_rcv_b1 cvmx_sli_pcie_msi_rcv_b1_t;

/**
 * cvmx_sli_pcie_msi_rcv_b2
 *
 * SLI_PCIE_MSI_RCV_B2 = SLI MAC MSI Receive Byte 2
 *
 * Register where MSI writes are directed from the MAC.
 *
 * Notes:
 * This CSR can be used by PCIe and sRIO MACs.
 *
 */
union cvmx_sli_pcie_msi_rcv_b2
{
	uint64_t u64;
	struct cvmx_sli_pcie_msi_rcv_b2_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_24_63               : 40;
	uint64_t intr                         : 8;  /**< A write to this register will result in a bit in
                                                         one of the SLI_MSI_RCV# registers being set.
                                                         Which bit is set is dependent on the previously
                                                         written using the SLI_MSI_WR_MAP register or if
                                                         not previously written the reset value of the MAP. */
	uint64_t reserved_0_15                : 16;
#else
	uint64_t reserved_0_15                : 16;
	uint64_t intr                         : 8;
	uint64_t reserved_24_63               : 40;
#endif
	} s;
	struct cvmx_sli_pcie_msi_rcv_b2_s     cn63xx;
	struct cvmx_sli_pcie_msi_rcv_b2_s     cn63xxp1;
};
typedef union cvmx_sli_pcie_msi_rcv_b2 cvmx_sli_pcie_msi_rcv_b2_t;

/**
 * cvmx_sli_pcie_msi_rcv_b3
 *
 * SLI_PCIE_MSI_RCV_B3 = SLI MAC MSI Receive Byte 3
 *
 * Register where MSI writes are directed from the MAC.
 *
 * Notes:
 * This CSR can be used by PCIe and sRIO MACs.
 *
 */
union cvmx_sli_pcie_msi_rcv_b3
{
	uint64_t u64;
	struct cvmx_sli_pcie_msi_rcv_b3_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_32_63               : 32;
	uint64_t intr                         : 8;  /**< A write to this register will result in a bit in
                                                         one of the SLI_MSI_RCV# registers being set.
                                                         Which bit is set is dependent on the previously
                                                         written using the SLI_MSI_WR_MAP register or if
                                                         not previously written the reset value of the MAP. */
	uint64_t reserved_0_23                : 24;
#else
	uint64_t reserved_0_23                : 24;
	uint64_t intr                         : 8;
	uint64_t reserved_32_63               : 32;
#endif
	} s;
	struct cvmx_sli_pcie_msi_rcv_b3_s     cn63xx;
	struct cvmx_sli_pcie_msi_rcv_b3_s     cn63xxp1;
};
typedef union cvmx_sli_pcie_msi_rcv_b3 cvmx_sli_pcie_msi_rcv_b3_t;

/**
 * cvmx_sli_pkt#_cnts
 *
 * SLI_PKT[0..31]_CNTS = SLI Packet ring# Counts
 *
 * The counters for output rings.
 */
union cvmx_sli_pktx_cnts
{
	uint64_t u64;
	struct cvmx_sli_pktx_cnts_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_54_63               : 10;
	uint64_t timer                        : 22; /**< Timer incremented every 1024 core clocks
                                                         when SLI_PKTS#_CNTS[CNT] is non zero. Field
                                                         cleared when SLI_PKTS#_CNTS[CNT] goes to 0.
                                                         Field is also cleared when SLI_PKT_TIME_INT is
                                                         cleared.
                                                         The first increment of this count can occur
                                                         between 0 to 1023 core clocks. */
	uint64_t cnt                          : 32; /**< ring counter. This field is incremented as
                                                         packets are sent out and decremented in response to
                                                         writes to this field.
                                                         When SLI_PKT_OUT_BMODE is '0' a value of 1 is
                                                         added to the register for each packet, when '1'
                                                         and the info-pointer is NOT used the length of the
                                                         packet plus 8 is added, when '1' and info-pointer
                                                         mode IS used the packet length is added to this
                                                         field. */
#else
	uint64_t cnt                          : 32;
	uint64_t timer                        : 22;
	uint64_t reserved_54_63               : 10;
#endif
	} s;
	struct cvmx_sli_pktx_cnts_s           cn63xx;
	struct cvmx_sli_pktx_cnts_s           cn63xxp1;
};
typedef union cvmx_sli_pktx_cnts cvmx_sli_pktx_cnts_t;

/**
 * cvmx_sli_pkt#_in_bp
 *
 * SLI_PKT[0..31]_IN_BP = SLI Packet ring# Input Backpressure
 *
 * The counters and thresholds for input packets to apply backpressure to processing of the packets.
 */
union cvmx_sli_pktx_in_bp
{
	uint64_t u64;
	struct cvmx_sli_pktx_in_bp_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t wmark                        : 32; /**< When CNT is greater than this threshold no more
                                                         packets will be processed for this ring.
                                                         When writing this field of the SLI_PKT#_IN_BP
                                                         register, use a 4-byte write so as to not write
                                                         any other field of this register. */
	uint64_t cnt                          : 32; /**< ring counter. This field is incremented by one
                                                         whenever OCTEON receives, buffers, and creates a
                                                         work queue entry for a packet that arrives by the
                                                         cooresponding input ring. A write to this field
                                                         will be subtracted from the field value.
                                                         When writing this field of the SLI_PKT#_IN_BP
                                                         register, use a 4-byte write so as to not write
                                                         any other field of this register. */
#else
	uint64_t cnt                          : 32;
	uint64_t wmark                        : 32;
#endif
	} s;
	struct cvmx_sli_pktx_in_bp_s          cn63xx;
	struct cvmx_sli_pktx_in_bp_s          cn63xxp1;
};
typedef union cvmx_sli_pktx_in_bp cvmx_sli_pktx_in_bp_t;

/**
 * cvmx_sli_pkt#_instr_baddr
 *
 * SLI_PKT[0..31]_INSTR_BADDR = SLI Packet ring# Instruction Base Address
 *
 * Start of Instruction for input packets.
 */
union cvmx_sli_pktx_instr_baddr
{
	uint64_t u64;
	struct cvmx_sli_pktx_instr_baddr_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t addr                         : 61; /**< Base address for Instructions. */
	uint64_t reserved_0_2                 : 3;
#else
	uint64_t reserved_0_2                 : 3;
	uint64_t addr                         : 61;
#endif
	} s;
	struct cvmx_sli_pktx_instr_baddr_s    cn63xx;
	struct cvmx_sli_pktx_instr_baddr_s    cn63xxp1;
};
typedef union cvmx_sli_pktx_instr_baddr cvmx_sli_pktx_instr_baddr_t;

/**
 * cvmx_sli_pkt#_instr_baoff_dbell
 *
 * SLI_PKT[0..31]_INSTR_BAOFF_DBELL = SLI Packet ring# Instruction Base Address Offset and Doorbell
 *
 * The doorbell and base address offset for next read.
 */
union cvmx_sli_pktx_instr_baoff_dbell
{
	uint64_t u64;
	struct cvmx_sli_pktx_instr_baoff_dbell_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t aoff                         : 32; /**< The offset from the SLI_PKT[0..31]_INSTR_BADDR
                                                         where the next instruction will be read. */
	uint64_t dbell                        : 32; /**< Instruction doorbell count. Writes to this field
                                                         will increment the value here. Reads will return
                                                         present value. A write of 0xffffffff will set the
                                                         DBELL and AOFF fields to '0'. */
#else
	uint64_t dbell                        : 32;
	uint64_t aoff                         : 32;
#endif
	} s;
	struct cvmx_sli_pktx_instr_baoff_dbell_s cn63xx;
	struct cvmx_sli_pktx_instr_baoff_dbell_s cn63xxp1;
};
typedef union cvmx_sli_pktx_instr_baoff_dbell cvmx_sli_pktx_instr_baoff_dbell_t;

/**
 * cvmx_sli_pkt#_instr_fifo_rsize
 *
 * SLI_PKT[0..31]_INSTR_FIFO_RSIZE = SLI Packet ring# Instruction FIFO and Ring Size.
 *
 * Fifo field and ring size for Instructions.
 */
union cvmx_sli_pktx_instr_fifo_rsize
{
	uint64_t u64;
	struct cvmx_sli_pktx_instr_fifo_rsize_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t max                          : 9;  /**< Max Fifo Size. */
	uint64_t rrp                          : 9;  /**< Fifo read pointer. */
	uint64_t wrp                          : 9;  /**< Fifo write pointer. */
	uint64_t fcnt                         : 5;  /**< Fifo count. */
	uint64_t rsize                        : 32; /**< Instruction ring size. */
#else
	uint64_t rsize                        : 32;
	uint64_t fcnt                         : 5;
	uint64_t wrp                          : 9;
	uint64_t rrp                          : 9;
	uint64_t max                          : 9;
#endif
	} s;
	struct cvmx_sli_pktx_instr_fifo_rsize_s cn63xx;
	struct cvmx_sli_pktx_instr_fifo_rsize_s cn63xxp1;
};
typedef union cvmx_sli_pktx_instr_fifo_rsize cvmx_sli_pktx_instr_fifo_rsize_t;

/**
 * cvmx_sli_pkt#_instr_header
 *
 * SLI_PKT[0..31]_INSTR_HEADER = SLI Packet ring# Instruction Header.
 *
 * VAlues used to build input packet header.
 */
union cvmx_sli_pktx_instr_header
{
	uint64_t u64;
	struct cvmx_sli_pktx_instr_header_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_44_63               : 20;
	uint64_t pbp                          : 1;  /**< Enable Packet-by-packet mode. */
	uint64_t reserved_38_42               : 5;
	uint64_t rparmode                     : 2;  /**< Parse Mode. Used when packet is raw and PBP==0. */
	uint64_t reserved_35_35               : 1;
	uint64_t rskp_len                     : 7;  /**< Skip Length. Used when packet is raw and PBP==0. */
	uint64_t reserved_26_27               : 2;
	uint64_t rnqos                        : 1;  /**< RNQOS. Used when packet is raw and PBP==0. */
	uint64_t rngrp                        : 1;  /**< RNGRP. Used when packet is raw and PBP==0. */
	uint64_t rntt                         : 1;  /**< RNTT. Used when packet is raw and PBP==0. */
	uint64_t rntag                        : 1;  /**< RNTAG. Used when packet is raw and PBP==0. */
	uint64_t use_ihdr                     : 1;  /**< When set '1' the instruction header will be sent
                                                         as part of the packet data, regardless of the
                                                         value of bit [63] of the instruction header.
                                                         USE_IHDR must be set whenever PBP is set. */
	uint64_t reserved_16_20               : 5;
	uint64_t par_mode                     : 2;  /**< Parse Mode. Used when USE_IHDR is set and packet
                                                         is not raw and PBP is not set. */
	uint64_t reserved_13_13               : 1;
	uint64_t skp_len                      : 7;  /**< Skip Length. Used when USE_IHDR is set and packet
                                                         is not raw and PBP is not set. */
	uint64_t reserved_4_5                 : 2;
	uint64_t nqos                         : 1;  /**< NQOS. Used when packet is raw and PBP==0. */
	uint64_t ngrp                         : 1;  /**< NGRP. Used when packet is raw and PBP==0. */
	uint64_t ntt                          : 1;  /**< NTT. Used when packet is raw and PBP==0. */
	uint64_t ntag                         : 1;  /**< NTAG. Used when packet is raw and PBP==0. */
#else
	uint64_t ntag                         : 1;
	uint64_t ntt                          : 1;
	uint64_t ngrp                         : 1;
	uint64_t nqos                         : 1;
	uint64_t reserved_4_5                 : 2;
	uint64_t skp_len                      : 7;
	uint64_t reserved_13_13               : 1;
	uint64_t par_mode                     : 2;
	uint64_t reserved_16_20               : 5;
	uint64_t use_ihdr                     : 1;
	uint64_t rntag                        : 1;
	uint64_t rntt                         : 1;
	uint64_t rngrp                        : 1;
	uint64_t rnqos                        : 1;
	uint64_t reserved_26_27               : 2;
	uint64_t rskp_len                     : 7;
	uint64_t reserved_35_35               : 1;
	uint64_t rparmode                     : 2;
	uint64_t reserved_38_42               : 5;
	uint64_t pbp                          : 1;
	uint64_t reserved_44_63               : 20;
#endif
	} s;
	struct cvmx_sli_pktx_instr_header_s   cn63xx;
	struct cvmx_sli_pktx_instr_header_s   cn63xxp1;
};
typedef union cvmx_sli_pktx_instr_header cvmx_sli_pktx_instr_header_t;

/**
 * cvmx_sli_pkt#_out_size
 *
 * SLI_PKT[0..31]_OUT_SIZE = SLI Packet Out Size
 *
 * Contains the BSIZE and ISIZE for output packet ports.
 */
union cvmx_sli_pktx_out_size
{
	uint64_t u64;
	struct cvmx_sli_pktx_out_size_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_23_63               : 41;
	uint64_t isize                        : 7;  /**< INFO BYTES size (bytes) for ring X. Legal sizes
                                                         are 0 to 120. Not used in buffer-pointer-only mode. */
	uint64_t bsize                        : 16; /**< BUFFER SIZE (bytes) for ring X. */
#else
	uint64_t bsize                        : 16;
	uint64_t isize                        : 7;
	uint64_t reserved_23_63               : 41;
#endif
	} s;
	struct cvmx_sli_pktx_out_size_s       cn63xx;
	struct cvmx_sli_pktx_out_size_s       cn63xxp1;
};
typedef union cvmx_sli_pktx_out_size cvmx_sli_pktx_out_size_t;

/**
 * cvmx_sli_pkt#_slist_baddr
 *
 * SLI_PKT[0..31]_SLIST_BADDR = SLI Packet ring# Scatter List Base Address
 *
 * Start of Scatter List for output packet pointers - MUST be 16 byte alligned
 */
union cvmx_sli_pktx_slist_baddr
{
	uint64_t u64;
	struct cvmx_sli_pktx_slist_baddr_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t addr                         : 60; /**< Base address for scatter list pointers. */
	uint64_t reserved_0_3                 : 4;
#else
	uint64_t reserved_0_3                 : 4;
	uint64_t addr                         : 60;
#endif
	} s;
	struct cvmx_sli_pktx_slist_baddr_s    cn63xx;
	struct cvmx_sli_pktx_slist_baddr_s    cn63xxp1;
};
typedef union cvmx_sli_pktx_slist_baddr cvmx_sli_pktx_slist_baddr_t;

/**
 * cvmx_sli_pkt#_slist_baoff_dbell
 *
 * SLI_PKT[0..31]_SLIST_BAOFF_DBELL = SLI Packet ring# Scatter List Base Address Offset and Doorbell
 *
 * The doorbell and base address offset for next read.
 */
union cvmx_sli_pktx_slist_baoff_dbell
{
	uint64_t u64;
	struct cvmx_sli_pktx_slist_baoff_dbell_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t aoff                         : 32; /**< The offset from the SLI_PKT[0..31]_SLIST_BADDR
                                                         where the next SList pointer will be read.
                                                         A write of 0xFFFFFFFF to the DBELL field will
                                                         clear DBELL and AOFF */
	uint64_t dbell                        : 32; /**< Scatter list doorbell count. Writes to this field
                                                         will increment the value here. Reads will return
                                                         present value. The value of this field is
                                                         decremented as read operations are ISSUED for
                                                         scatter pointers.
                                                         A write of 0xFFFFFFFF will clear DBELL and AOFF */
#else
	uint64_t dbell                        : 32;
	uint64_t aoff                         : 32;
#endif
	} s;
	struct cvmx_sli_pktx_slist_baoff_dbell_s cn63xx;
	struct cvmx_sli_pktx_slist_baoff_dbell_s cn63xxp1;
};
typedef union cvmx_sli_pktx_slist_baoff_dbell cvmx_sli_pktx_slist_baoff_dbell_t;

/**
 * cvmx_sli_pkt#_slist_fifo_rsize
 *
 * SLI_PKT[0..31]_SLIST_FIFO_RSIZE = SLI Packet ring# Scatter List FIFO and Ring Size.
 *
 * The number of scatter pointer pairs in the scatter list.
 */
union cvmx_sli_pktx_slist_fifo_rsize
{
	uint64_t u64;
	struct cvmx_sli_pktx_slist_fifo_rsize_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_32_63               : 32;
	uint64_t rsize                        : 32; /**< The number of scatter pointer pairs contained in
                                                         the scatter list ring. */
#else
	uint64_t rsize                        : 32;
	uint64_t reserved_32_63               : 32;
#endif
	} s;
	struct cvmx_sli_pktx_slist_fifo_rsize_s cn63xx;
	struct cvmx_sli_pktx_slist_fifo_rsize_s cn63xxp1;
};
typedef union cvmx_sli_pktx_slist_fifo_rsize cvmx_sli_pktx_slist_fifo_rsize_t;

/**
 * cvmx_sli_pkt_cnt_int
 *
 * SLI_PKT_CNT_INT = SLI Packet Counter Interrupt
 *
 * The packets rings that are interrupting because of Packet Counters.
 */
union cvmx_sli_pkt_cnt_int
{
	uint64_t u64;
	struct cvmx_sli_pkt_cnt_int_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_32_63               : 32;
	uint64_t port                         : 32; /**< Output ring packet counter interrupt bits
                                                         SLI sets PORT<i> whenever
                                                         SLI_PKTi_CNTS[CNT] > SLI_PKT_INT_LEVELS[CNT].
                                                         SLI_PKT_CNT_INT_ENB[PORT<i>] is the corresponding
                                                         enable. */
#else
	uint64_t port                         : 32;
	uint64_t reserved_32_63               : 32;
#endif
	} s;
	struct cvmx_sli_pkt_cnt_int_s         cn63xx;
	struct cvmx_sli_pkt_cnt_int_s         cn63xxp1;
};
typedef union cvmx_sli_pkt_cnt_int cvmx_sli_pkt_cnt_int_t;

/**
 * cvmx_sli_pkt_cnt_int_enb
 *
 * SLI_PKT_CNT_INT_ENB = SLI Packet Counter Interrupt Enable
 *
 * Enable for the packets rings that are interrupting because of Packet Counters.
 */
union cvmx_sli_pkt_cnt_int_enb
{
	uint64_t u64;
	struct cvmx_sli_pkt_cnt_int_enb_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_32_63               : 32;
	uint64_t port                         : 32; /**< Output ring packet counter interrupt enables
                                                         When both PORT<i> and corresponding
                                                         SLI_PKT_CNT_INT[PORT<i>] are set, for any i,
                                                         then SLI_INT_SUM[PCNT] is set, which can cause
                                                         an interrupt. */
#else
	uint64_t port                         : 32;
	uint64_t reserved_32_63               : 32;
#endif
	} s;
	struct cvmx_sli_pkt_cnt_int_enb_s     cn63xx;
	struct cvmx_sli_pkt_cnt_int_enb_s     cn63xxp1;
};
typedef union cvmx_sli_pkt_cnt_int_enb cvmx_sli_pkt_cnt_int_enb_t;

/**
 * cvmx_sli_pkt_ctl
 *
 * SLI_PKT_CTL = SLI Packet Control
 *
 * Control for packets.
 */
union cvmx_sli_pkt_ctl
{
	uint64_t u64;
	struct cvmx_sli_pkt_ctl_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_5_63                : 59;
	uint64_t ring_en                      : 1;  /**< When '0' forces "relative Q position" received
                                                         from PKO to be zero, and replicates the back-
                                                         pressure indication for the first ring attached
                                                         to a PKO port across all the rings attached to a
                                                         PKO port. When '1' backpressure is on a per
                                                         port/ring. */
	uint64_t pkt_bp                       : 4;  /**< When set '1' enable the port level backpressure for
                                                         PKO ports associated with the bit. */
#else
	uint64_t pkt_bp                       : 4;
	uint64_t ring_en                      : 1;
	uint64_t reserved_5_63                : 59;
#endif
	} s;
	struct cvmx_sli_pkt_ctl_s             cn63xx;
	struct cvmx_sli_pkt_ctl_s             cn63xxp1;
};
typedef union cvmx_sli_pkt_ctl cvmx_sli_pkt_ctl_t;

/**
 * cvmx_sli_pkt_data_out_es
 *
 * SLI_PKT_DATA_OUT_ES = SLI's Packet Data Out Endian Swap
 *
 * The Endian Swap for writing Data Out.
 */
union cvmx_sli_pkt_data_out_es
{
	uint64_t u64;
	struct cvmx_sli_pkt_data_out_es_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t es                           : 64; /**< ES<1:0> or MACADD<63:62> for buffer/info writes.
                                                         ES<2i+1:2i> becomes either ES<1:0> or
                                                         MACADD<63:62> for writes to buffer/info pair
                                                         MAC memory space addresses fetched from packet
                                                         output ring i. ES<1:0> if SLI_PKT_DPADDR[DPTR<i>]=1
                                                         , else MACADD<63:62>.
                                                         In the latter case, ES<1:0> comes from DPTR<63:62>.
                                                         ES<1:0> is the endian-swap attribute for these MAC
                                                         memory space writes. */
#else
	uint64_t es                           : 64;
#endif
	} s;
	struct cvmx_sli_pkt_data_out_es_s     cn63xx;
	struct cvmx_sli_pkt_data_out_es_s     cn63xxp1;
};
typedef union cvmx_sli_pkt_data_out_es cvmx_sli_pkt_data_out_es_t;

/**
 * cvmx_sli_pkt_data_out_ns
 *
 * SLI_PKT_DATA_OUT_NS = SLI's Packet Data Out No Snoop
 *
 * The NS field for the TLP when writing packet data.
 */
union cvmx_sli_pkt_data_out_ns
{
	uint64_t u64;
	struct cvmx_sli_pkt_data_out_ns_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_32_63               : 32;
	uint64_t nsr                          : 32; /**< ADDRTYPE<1> or MACADD<61> for buffer/info writes.
                                                         NSR<i> becomes either ADDRTYPE<1> or MACADD<61>
                                                         for writes to buffer/info pair MAC memory space
                                                         addresses fetched from packet output ring i.
                                                         ADDRTYPE<1> if SLI_PKT_DPADDR[DPTR<i>]=1, else
                                                         MACADD<61>.
                                                         In the latter case,ADDRTYPE<1> comes from DPTR<61>.
                                                         ADDRTYPE<1> is the no-snoop attribute for PCIe
                                                         , helps select an SRIO*_S2M_TYPE* entry with sRIO. */
#else
	uint64_t nsr                          : 32;
	uint64_t reserved_32_63               : 32;
#endif
	} s;
	struct cvmx_sli_pkt_data_out_ns_s     cn63xx;
	struct cvmx_sli_pkt_data_out_ns_s     cn63xxp1;
};
typedef union cvmx_sli_pkt_data_out_ns cvmx_sli_pkt_data_out_ns_t;

/**
 * cvmx_sli_pkt_data_out_ror
 *
 * SLI_PKT_DATA_OUT_ROR = SLI's Packet Data Out Relaxed Ordering
 *
 * The ROR field for the TLP when writing Packet Data.
 */
union cvmx_sli_pkt_data_out_ror
{
	uint64_t u64;
	struct cvmx_sli_pkt_data_out_ror_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_32_63               : 32;
	uint64_t ror                          : 32; /**< ADDRTYPE<0> or MACADD<60> for buffer/info writes.
                                                         ROR<i> becomes either ADDRTYPE<0> or MACADD<60>
                                                         for writes to buffer/info pair MAC memory space
                                                         addresses fetched from packet output ring i.
                                                         ADDRTYPE<0> if SLI_PKT_DPADDR[DPTR<i>]=1, else
                                                         MACADD<60>.
                                                         In the latter case,ADDRTYPE<0> comes from DPTR<60>.
                                                         ADDRTYPE<0> is the relaxed-order attribute for PCIe
                                                         , helps select an SRIO*_S2M_TYPE* entry with sRIO. */
#else
	uint64_t ror                          : 32;
	uint64_t reserved_32_63               : 32;
#endif
	} s;
	struct cvmx_sli_pkt_data_out_ror_s    cn63xx;
	struct cvmx_sli_pkt_data_out_ror_s    cn63xxp1;
};
typedef union cvmx_sli_pkt_data_out_ror cvmx_sli_pkt_data_out_ror_t;

/**
 * cvmx_sli_pkt_dpaddr
 *
 * SLI_PKT_DPADDR = SLI's Packet Data Pointer Addr
 *
 * Used to detemine address and attributes for packet data writes.
 */
union cvmx_sli_pkt_dpaddr
{
	uint64_t u64;
	struct cvmx_sli_pkt_dpaddr_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_32_63               : 32;
	uint64_t dptr                         : 32; /**< Determines whether buffer/info pointers are
                                                         DPTR format 0 or DPTR format 1.
                                                         When DPTR<i>=1, the buffer/info pointers fetched
                                                         from packet output ring i are DPTR format 0.
                                                         When DPTR<i>=0, the buffer/info pointers fetched
                                                         from packet output ring i are DPTR format 1.
                                                         (Replace SLI_PKT_INPUT_CONTROL[D_ESR,D_NSR,D_ROR]
                                                         in the HRM descriptions of DPTR format 0/1 with
                                                         SLI_PKT_DATA_OUT_ES[ES<2i+1:2i>],
                                                         SLI_PKT_DATA_OUT_NS[NSR<i>], and
                                                         SLI_PKT_DATA_OUT_ROR[ROR<i>], respectively,
                                                         though.) */
#else
	uint64_t dptr                         : 32;
	uint64_t reserved_32_63               : 32;
#endif
	} s;
	struct cvmx_sli_pkt_dpaddr_s          cn63xx;
	struct cvmx_sli_pkt_dpaddr_s          cn63xxp1;
};
typedef union cvmx_sli_pkt_dpaddr cvmx_sli_pkt_dpaddr_t;

/**
 * cvmx_sli_pkt_in_bp
 *
 * SLI_PKT_IN_BP = SLI Packet Input Backpressure
 *
 * Which input rings have backpressure applied.
 */
union cvmx_sli_pkt_in_bp
{
	uint64_t u64;
	struct cvmx_sli_pkt_in_bp_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_32_63               : 32;
	uint64_t bp                           : 32; /**< A packet input  ring that has its count greater
                                                         than its WMARK will have backpressure applied.
                                                         Each of the 32 bits coorespond to an input ring.
                                                         When '1' that ring has backpressure applied an
                                                         will fetch no more instructions, but will process
                                                         any previously fetched instructions. */
#else
	uint64_t bp                           : 32;
	uint64_t reserved_32_63               : 32;
#endif
	} s;
	struct cvmx_sli_pkt_in_bp_s           cn63xx;
	struct cvmx_sli_pkt_in_bp_s           cn63xxp1;
};
typedef union cvmx_sli_pkt_in_bp cvmx_sli_pkt_in_bp_t;

/**
 * cvmx_sli_pkt_in_done#_cnts
 *
 * SLI_PKT_IN_DONE[0..31]_CNTS = SLI Instruction Done ring# Counts
 *
 * Counters for instructions completed on Input rings.
 */
union cvmx_sli_pkt_in_donex_cnts
{
	uint64_t u64;
	struct cvmx_sli_pkt_in_donex_cnts_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_32_63               : 32;
	uint64_t cnt                          : 32; /**< This field is incrmented by '1' when an instruction
                                                         is completed. This field is incremented as the
                                                         last of the data is read from the MAC. */
#else
	uint64_t cnt                          : 32;
	uint64_t reserved_32_63               : 32;
#endif
	} s;
	struct cvmx_sli_pkt_in_donex_cnts_s   cn63xx;
	struct cvmx_sli_pkt_in_donex_cnts_s   cn63xxp1;
};
typedef union cvmx_sli_pkt_in_donex_cnts cvmx_sli_pkt_in_donex_cnts_t;

/**
 * cvmx_sli_pkt_in_instr_counts
 *
 * SLI_PKT_IN_INSTR_COUNTS = SLI Packet Input Instrutction Counts
 *
 * Keeps track of the number of instructions read into the FIFO and Packets sent to IPD.
 */
union cvmx_sli_pkt_in_instr_counts
{
	uint64_t u64;
	struct cvmx_sli_pkt_in_instr_counts_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t wr_cnt                       : 32; /**< Shows the number of packets sent to the IPD. */
	uint64_t rd_cnt                       : 32; /**< Shows the value of instructions that have had reads
                                                         issued for them.
                                                         to the Packet-ring is in reset. */
#else
	uint64_t rd_cnt                       : 32;
	uint64_t wr_cnt                       : 32;
#endif
	} s;
	struct cvmx_sli_pkt_in_instr_counts_s cn63xx;
	struct cvmx_sli_pkt_in_instr_counts_s cn63xxp1;
};
typedef union cvmx_sli_pkt_in_instr_counts cvmx_sli_pkt_in_instr_counts_t;

/**
 * cvmx_sli_pkt_in_pcie_port
 *
 * SLI_PKT_IN_PCIE_PORT = SLI's Packet In To MAC Port Assignment
 *
 * Assigns Packet Input rings to MAC ports.
 */
union cvmx_sli_pkt_in_pcie_port
{
	uint64_t u64;
	struct cvmx_sli_pkt_in_pcie_port_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t pp                           : 64; /**< The MAC port that the Packet ring number is
                                                         assigned. Two bits are used per ring (i.e. ring 0
                                                         [1:0], ring 1 [3:2], ....). A value of '0 means
                                                         that the Packetring is assign to MAC Port 0, a '1'
                                                         MAC Port 1, '2' and '3' are reserved. */
#else
	uint64_t pp                           : 64;
#endif
	} s;
	struct cvmx_sli_pkt_in_pcie_port_s    cn63xx;
	struct cvmx_sli_pkt_in_pcie_port_s    cn63xxp1;
};
typedef union cvmx_sli_pkt_in_pcie_port cvmx_sli_pkt_in_pcie_port_t;

/**
 * cvmx_sli_pkt_input_control
 *
 * SLI_PKT_INPUT_CONTROL = SLI's Packet Input Control
 *
 * Control for reads for gather list and instructions.
 */
union cvmx_sli_pkt_input_control
{
	uint64_t u64;
	struct cvmx_sli_pkt_input_control_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_23_63               : 41;
	uint64_t pkt_rr                       : 1;  /**< When set '1' the input packet selection will be
                                                         made with a Round Robin arbitration. When '0'
                                                         the input packet ring is fixed in priority,
                                                         where the lower ring number has higher priority. */
	uint64_t pbp_dhi                      : 13; /**< PBP_DHI replaces address bits that are used
                                                         for parse mode and skip-length when
                                                         SLI_PKTi_INSTR_HEADER[PBP]=1.
                                                         PBP_DHI becomes either MACADD<63:55> or MACADD<59:51>
                                                         for the instruction DPTR reads in this case.
                                                         The instruction DPTR reads are called
                                                         "First Direct" or "First Indirect" in the HRM.
                                                         When PBP=1, if "First Direct" and USE_CSR=0, PBP_DHI
                                                         becomes MACADD<59:51>, else MACADD<63:55>. */
	uint64_t d_nsr                        : 1;  /**< ADDRTYPE<1> or MACADD<61> for packet input data
                                                         reads.
                                                         D_NSR becomes either ADDRTYPE<1> or MACADD<61>
                                                         for MAC memory space reads of packet input data
                                                         fetched for any packet input ring.
                                                         ADDRTYPE<1> if USE_CSR=1, else MACADD<61>.
                                                         In the latter case, ADDRTYPE<1> comes from DPTR<61>.
                                                         ADDRTYPE<1> is the no-snoop attribute for PCIe
                                                         , helps select an SRIO*_S2M_TYPE* entry with sRIO. */
	uint64_t d_esr                        : 2;  /**< ES<1:0> or MACADD<63:62> for packet input data
                                                         reads.
                                                         D_ESR becomes either ES<1:0> or MACADD<63:62>
                                                         for MAC memory space reads of packet input data
                                                         fetched for any packet input ring.
                                                         ES<1:0> if USE_CSR=1, else MACADD<63:62>.
                                                         In the latter case, ES<1:0> comes from DPTR<63:62>.
                                                         ES<1:0> is the endian-swap attribute for these MAC
                                                         memory space reads. */
	uint64_t d_ror                        : 1;  /**< ADDRTYPE<0> or MACADD<60> for packet input data
                                                         reads.
                                                         D_ROR becomes either ADDRTYPE<0> or MACADD<60>
                                                         for MAC memory space reads of packet input data
                                                         fetched for any packet input ring.
                                                         ADDRTYPE<0> if USE_CSR=1, else MACADD<60>.
                                                         In the latter case, ADDRTYPE<0> comes from DPTR<60>.
                                                         ADDRTYPE<0> is the relaxed-order attribute for PCIe
                                                         , helps select an SRIO*_S2M_TYPE* entry with sRIO. */
	uint64_t use_csr                      : 1;  /**< When set '1' the csr value will be used for
                                                         ROR, ESR, and NSR. When clear '0' the value in
                                                         DPTR will be used. In turn the bits not used for
                                                         ROR, ESR, and NSR, will be used for bits [63:60]
                                                         of the address used to fetch packet data. */
	uint64_t nsr                          : 1;  /**< ADDRTYPE<1> for packet input instruction reads and
                                                         gather list (i.e. DPI component) reads from MAC
                                                         memory space.
                                                         ADDRTYPE<1> is the no-snoop attribute for PCIe
                                                         , helps select an SRIO*_S2M_TYPE* entry with sRIO. */
	uint64_t esr                          : 2;  /**< ES<1:0> for packet input instruction reads and
                                                         gather list (i.e. DPI component) reads from MAC
                                                         memory space.
                                                         ES<1:0> is the endian-swap attribute for these MAC
                                                         memory space reads. */
	uint64_t ror                          : 1;  /**< ADDRTYPE<0> for packet input instruction reads and
                                                         gather list (i.e. DPI component) reads from MAC
                                                         memory space.
                                                         ADDRTYPE<0> is the relaxed-order attribute for PCIe
                                                         , helps select an SRIO*_S2M_TYPE* entry with sRIO. */
#else
	uint64_t ror                          : 1;
	uint64_t esr                          : 2;
	uint64_t nsr                          : 1;
	uint64_t use_csr                      : 1;
	uint64_t d_ror                        : 1;
	uint64_t d_esr                        : 2;
	uint64_t d_nsr                        : 1;
	uint64_t pbp_dhi                      : 13;
	uint64_t pkt_rr                       : 1;
	uint64_t reserved_23_63               : 41;
#endif
	} s;
	struct cvmx_sli_pkt_input_control_s   cn63xx;
	struct cvmx_sli_pkt_input_control_s   cn63xxp1;
};
typedef union cvmx_sli_pkt_input_control cvmx_sli_pkt_input_control_t;

/**
 * cvmx_sli_pkt_instr_enb
 *
 * SLI_PKT_INSTR_ENB = SLI's Packet Instruction Enable
 *
 * Enables the instruction fetch for a Packet-ring.
 */
union cvmx_sli_pkt_instr_enb
{
	uint64_t u64;
	struct cvmx_sli_pkt_instr_enb_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_32_63               : 32;
	uint64_t enb                          : 32; /**< When ENB<i>=1, instruction input ring i is enabled. */
#else
	uint64_t enb                          : 32;
	uint64_t reserved_32_63               : 32;
#endif
	} s;
	struct cvmx_sli_pkt_instr_enb_s       cn63xx;
	struct cvmx_sli_pkt_instr_enb_s       cn63xxp1;
};
typedef union cvmx_sli_pkt_instr_enb cvmx_sli_pkt_instr_enb_t;

/**
 * cvmx_sli_pkt_instr_rd_size
 *
 * SLI_PKT_INSTR_RD_SIZE = SLI Instruction Read Size
 *
 * The number of instruction allowed to be read at one time.
 */
union cvmx_sli_pkt_instr_rd_size
{
	uint64_t u64;
	struct cvmx_sli_pkt_instr_rd_size_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t rdsize                       : 64; /**< Number of instructions to be read in one MAC read
                                                         request for the 4 ports - 8 rings. Every two bits
                                                         (i.e. 1:0, 3:2, 5:4..) are assign to the port/ring
                                                         combinations.
                                                         - 15:0  PKIPort0,Ring 7..0  31:16 PKIPort1,Ring 7..0
                                                         - 47:32 PKIPort2,Ring 7..0  63:48 PKIPort3,Ring 7..0
                                                         Two bit value are:
                                                         0 - 1 Instruction
                                                         1 - 2 Instructions
                                                         2 - 3 Instructions
                                                         3 - 4 Instructions */
#else
	uint64_t rdsize                       : 64;
#endif
	} s;
	struct cvmx_sli_pkt_instr_rd_size_s   cn63xx;
	struct cvmx_sli_pkt_instr_rd_size_s   cn63xxp1;
};
typedef union cvmx_sli_pkt_instr_rd_size cvmx_sli_pkt_instr_rd_size_t;

/**
 * cvmx_sli_pkt_instr_size
 *
 * SLI_PKT_INSTR_SIZE = SLI's Packet Instruction Size
 *
 * Determines if instructions are 64 or 32 byte in size for a Packet-ring.
 */
union cvmx_sli_pkt_instr_size
{
	uint64_t u64;
	struct cvmx_sli_pkt_instr_size_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_32_63               : 32;
	uint64_t is_64b                       : 32; /**< When IS_64B<i>=1, instruction input ring i uses 64B
                                                         instructions, else 32B instructions. */
#else
	uint64_t is_64b                       : 32;
	uint64_t reserved_32_63               : 32;
#endif
	} s;
	struct cvmx_sli_pkt_instr_size_s      cn63xx;
	struct cvmx_sli_pkt_instr_size_s      cn63xxp1;
};
typedef union cvmx_sli_pkt_instr_size cvmx_sli_pkt_instr_size_t;

/**
 * cvmx_sli_pkt_int_levels
 *
 * 0x90F0 reserved SLI_PKT_PCIE_PORT2
 *
 *
 *                  SLI_PKT_INT_LEVELS = SLI's Packet Interrupt Levels
 *
 * Output packet interrupt levels.
 */
union cvmx_sli_pkt_int_levels
{
	uint64_t u64;
	struct cvmx_sli_pkt_int_levels_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_54_63               : 10;
	uint64_t time                         : 22; /**< Output ring counter time interrupt threshold
                                                         SLI sets SLI_PKT_TIME_INT[PORT<i>] whenever
                                                         SLI_PKTi_CNTS[TIMER] > TIME */
	uint64_t cnt                          : 32; /**< Output ring counter interrupt threshold
                                                         SLI sets SLI_PKT_CNT_INT[PORT<i>] whenever
                                                         SLI_PKTi_CNTS[CNT] > CNT */
#else
	uint64_t cnt                          : 32;
	uint64_t time                         : 22;
	uint64_t reserved_54_63               : 10;
#endif
	} s;
	struct cvmx_sli_pkt_int_levels_s      cn63xx;
	struct cvmx_sli_pkt_int_levels_s      cn63xxp1;
};
typedef union cvmx_sli_pkt_int_levels cvmx_sli_pkt_int_levels_t;

/**
 * cvmx_sli_pkt_iptr
 *
 * SLI_PKT_IPTR = SLI's Packet Info Poitner
 *
 * Controls using the Info-Pointer to store length and data.
 */
union cvmx_sli_pkt_iptr
{
	uint64_t u64;
	struct cvmx_sli_pkt_iptr_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_32_63               : 32;
	uint64_t iptr                         : 32; /**< When IPTR<i>=1, packet output ring i is in info-
                                                         pointer mode, else buffer-pointer-only mode. */
#else
	uint64_t iptr                         : 32;
	uint64_t reserved_32_63               : 32;
#endif
	} s;
	struct cvmx_sli_pkt_iptr_s            cn63xx;
	struct cvmx_sli_pkt_iptr_s            cn63xxp1;
};
typedef union cvmx_sli_pkt_iptr cvmx_sli_pkt_iptr_t;

/**
 * cvmx_sli_pkt_out_bmode
 *
 * SLI_PKT_OUT_BMODE = SLI's Packet Out Byte Mode
 *
 * Control the updating of the SLI_PKT#_CNT register.
 */
union cvmx_sli_pkt_out_bmode
{
	uint64_t u64;
	struct cvmx_sli_pkt_out_bmode_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_32_63               : 32;
	uint64_t bmode                        : 32; /**< Determines whether SLI_PKTi_CNTS[CNT] is a byte or
                                                         packet counter.
                                                         When BMODE<i>=1, SLI_PKTi_CNTS[CNT] is a byte
                                                         counter, else SLI_PKTi_CNTS[CNT] is a packet
                                                         counter. */
#else
	uint64_t bmode                        : 32;
	uint64_t reserved_32_63               : 32;
#endif
	} s;
	struct cvmx_sli_pkt_out_bmode_s       cn63xx;
	struct cvmx_sli_pkt_out_bmode_s       cn63xxp1;
};
typedef union cvmx_sli_pkt_out_bmode cvmx_sli_pkt_out_bmode_t;

/**
 * cvmx_sli_pkt_out_enb
 *
 * SLI_PKT_OUT_ENB = SLI's Packet Output Enable
 *
 * Enables the output packet engines.
 */
union cvmx_sli_pkt_out_enb
{
	uint64_t u64;
	struct cvmx_sli_pkt_out_enb_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_32_63               : 32;
	uint64_t enb                          : 32; /**< When ENB<i>=1, packet output ring i is enabled.
                                                         If an error occurs on reading pointers for an
                                                         output ring, the ring will be disabled by clearing
                                                         the bit associated with the ring to '0'. */
#else
	uint64_t enb                          : 32;
	uint64_t reserved_32_63               : 32;
#endif
	} s;
	struct cvmx_sli_pkt_out_enb_s         cn63xx;
	struct cvmx_sli_pkt_out_enb_s         cn63xxp1;
};
typedef union cvmx_sli_pkt_out_enb cvmx_sli_pkt_out_enb_t;

/**
 * cvmx_sli_pkt_output_wmark
 *
 * SLI_PKT_OUTPUT_WMARK = SLI's Packet Output Water Mark
 *
 * Value that when the SLI_PKT#_SLIST_BAOFF_DBELL[DBELL] value is less then that backpressure for the rings will be applied.
 */
union cvmx_sli_pkt_output_wmark
{
	uint64_t u64;
	struct cvmx_sli_pkt_output_wmark_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_32_63               : 32;
	uint64_t wmark                        : 32; /**< Value when DBELL count drops below backpressure
                                                         for the ring will be applied to the PKO. */
#else
	uint64_t wmark                        : 32;
	uint64_t reserved_32_63               : 32;
#endif
	} s;
	struct cvmx_sli_pkt_output_wmark_s    cn63xx;
	struct cvmx_sli_pkt_output_wmark_s    cn63xxp1;
};
typedef union cvmx_sli_pkt_output_wmark cvmx_sli_pkt_output_wmark_t;

/**
 * cvmx_sli_pkt_pcie_port
 *
 * SLI_PKT_PCIE_PORT = SLI's Packet To MAC Port Assignment
 *
 * Assigns Packet Ports to MAC ports.
 */
union cvmx_sli_pkt_pcie_port
{
	uint64_t u64;
	struct cvmx_sli_pkt_pcie_port_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t pp                           : 64; /**< The physical MAC  port that the output ring uses.
                                                         Two bits are used per ring (i.e. ring 0 [1:0],
                                                         ring 1 [3:2], ....). A value of '0 means
                                                         that the Packetring is assign to MAC Port 0, a '1'
                                                         MAC Port 1, '2' and '3' are reserved. */
#else
	uint64_t pp                           : 64;
#endif
	} s;
	struct cvmx_sli_pkt_pcie_port_s       cn63xx;
	struct cvmx_sli_pkt_pcie_port_s       cn63xxp1;
};
typedef union cvmx_sli_pkt_pcie_port cvmx_sli_pkt_pcie_port_t;

/**
 * cvmx_sli_pkt_port_in_rst
 *
 * 91c0 reserved
 * 91d0 reserved
 * 91e0 reserved
 *
 *
 *                   SLI_PKT_PORT_IN_RST = SLI Packet Port In Reset
 *
 *  Vector bits related to ring-port for ones that are reset.
 */
union cvmx_sli_pkt_port_in_rst
{
	uint64_t u64;
	struct cvmx_sli_pkt_port_in_rst_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t in_rst                       : 32; /**< When asserted '1' the vector bit cooresponding
                                                         to the inbound Packet-ring is in reset. */
	uint64_t out_rst                      : 32; /**< When asserted '1' the vector bit cooresponding
                                                         to the outbound Packet-ring is in reset. */
#else
	uint64_t out_rst                      : 32;
	uint64_t in_rst                       : 32;
#endif
	} s;
	struct cvmx_sli_pkt_port_in_rst_s     cn63xx;
	struct cvmx_sli_pkt_port_in_rst_s     cn63xxp1;
};
typedef union cvmx_sli_pkt_port_in_rst cvmx_sli_pkt_port_in_rst_t;

/**
 * cvmx_sli_pkt_slist_es
 *
 * SLI_PKT_SLIST_ES = SLI's Packet Scatter List Endian Swap
 *
 * The Endian Swap for Scatter List Read.
 */
union cvmx_sli_pkt_slist_es
{
	uint64_t u64;
	struct cvmx_sli_pkt_slist_es_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t es                           : 64; /**< ES<1:0> for the packet output ring reads that
                                                         fetch buffer/info pointer pairs.
                                                         ES<2i+1:2i> becomes ES<1:0> in DPI/SLI reads that
                                                         fetch buffer/info pairs from packet output ring i
                                                         (from address SLI_PKTi_SLIST_BADDR+ in MAC memory
                                                         space.)
                                                         ES<1:0> is the endian-swap attribute for these MAC
                                                         memory space reads. */
#else
	uint64_t es                           : 64;
#endif
	} s;
	struct cvmx_sli_pkt_slist_es_s        cn63xx;
	struct cvmx_sli_pkt_slist_es_s        cn63xxp1;
};
typedef union cvmx_sli_pkt_slist_es cvmx_sli_pkt_slist_es_t;

/**
 * cvmx_sli_pkt_slist_ns
 *
 * SLI_PKT_SLIST_NS = SLI's Packet Scatter List No Snoop
 *
 * The NS field for the TLP when fetching Scatter List.
 */
union cvmx_sli_pkt_slist_ns
{
	uint64_t u64;
	struct cvmx_sli_pkt_slist_ns_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_32_63               : 32;
	uint64_t nsr                          : 32; /**< ADDRTYPE<1> for the packet output ring reads that
                                                         fetch buffer/info pointer pairs.
                                                         NSR<i> becomes ADDRTYPE<1> in DPI/SLI reads that
                                                         fetch buffer/info pairs from packet output ring i
                                                         (from address SLI_PKTi_SLIST_BADDR+ in MAC memory
                                                         space.)
                                                         ADDRTYPE<1> is the relaxed-order attribute for PCIe
                                                         , helps select an SRIO*_S2M_TYPE* entry with sRIO. */
#else
	uint64_t nsr                          : 32;
	uint64_t reserved_32_63               : 32;
#endif
	} s;
	struct cvmx_sli_pkt_slist_ns_s        cn63xx;
	struct cvmx_sli_pkt_slist_ns_s        cn63xxp1;
};
typedef union cvmx_sli_pkt_slist_ns cvmx_sli_pkt_slist_ns_t;

/**
 * cvmx_sli_pkt_slist_ror
 *
 * SLI_PKT_SLIST_ROR = SLI's Packet Scatter List Relaxed Ordering
 *
 * The ROR field for the TLP when fetching Scatter List.
 */
union cvmx_sli_pkt_slist_ror
{
	uint64_t u64;
	struct cvmx_sli_pkt_slist_ror_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_32_63               : 32;
	uint64_t ror                          : 32; /**< ADDRTYPE<0> for the packet output ring reads that
                                                         fetch buffer/info pointer pairs.
                                                         ROR<i> becomes ADDRTYPE<0> in DPI/SLI reads that
                                                         fetch buffer/info pairs from packet output ring i
                                                         (from address SLI_PKTi_SLIST_BADDR+ in MAC memory
                                                         space.)
                                                         ADDRTYPE<0> is the relaxed-order attribute for PCIe
                                                         , helps select an SRIO*_S2M_TYPE* entry with sRIO. */
#else
	uint64_t ror                          : 32;
	uint64_t reserved_32_63               : 32;
#endif
	} s;
	struct cvmx_sli_pkt_slist_ror_s       cn63xx;
	struct cvmx_sli_pkt_slist_ror_s       cn63xxp1;
};
typedef union cvmx_sli_pkt_slist_ror cvmx_sli_pkt_slist_ror_t;

/**
 * cvmx_sli_pkt_time_int
 *
 * SLI_PKT_TIME_INT = SLI Packet Timer Interrupt
 *
 * The packets rings that are interrupting because of Packet Timers.
 */
union cvmx_sli_pkt_time_int
{
	uint64_t u64;
	struct cvmx_sli_pkt_time_int_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_32_63               : 32;
	uint64_t port                         : 32; /**< Output ring packet timer interrupt bits
                                                         SLI sets PORT<i> whenever
                                                         SLI_PKTi_CNTS[TIMER] > SLI_PKT_INT_LEVELS[TIME].
                                                         SLI_PKT_TIME_INT_ENB[PORT<i>] is the corresponding
                                                         enable. */
#else
	uint64_t port                         : 32;
	uint64_t reserved_32_63               : 32;
#endif
	} s;
	struct cvmx_sli_pkt_time_int_s        cn63xx;
	struct cvmx_sli_pkt_time_int_s        cn63xxp1;
};
typedef union cvmx_sli_pkt_time_int cvmx_sli_pkt_time_int_t;

/**
 * cvmx_sli_pkt_time_int_enb
 *
 * SLI_PKT_TIME_INT_ENB = SLI Packet Timer Interrupt Enable
 *
 * The packets rings that are interrupting because of Packet Timers.
 */
union cvmx_sli_pkt_time_int_enb
{
	uint64_t u64;
	struct cvmx_sli_pkt_time_int_enb_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_32_63               : 32;
	uint64_t port                         : 32; /**< Output ring packet timer interrupt enables
                                                         When both PORT<i> and corresponding
                                                         SLI_PKT_TIME_INT[PORT<i>] are set, for any i,
                                                         then SLI_INT_SUM[PTIME] is set, which can cause
                                                         an interrupt. */
#else
	uint64_t port                         : 32;
	uint64_t reserved_32_63               : 32;
#endif
	} s;
	struct cvmx_sli_pkt_time_int_enb_s    cn63xx;
	struct cvmx_sli_pkt_time_int_enb_s    cn63xxp1;
};
typedef union cvmx_sli_pkt_time_int_enb cvmx_sli_pkt_time_int_enb_t;

/**
 * cvmx_sli_s2m_port#_ctl
 *
 * SLI_S2M_PORTX_CTL = SLI's S2M Port 0 Control
 *
 * Contains control for access from SLI to a MAC port.
 * Writes to this register are not ordered with writes/reads to the MAC Memory space.
 * To ensure that a write has completed the user must read the register before
 * making an access(i.e. MAC memory space) that requires the value of this register to be updated.
 */
union cvmx_sli_s2m_portx_ctl
{
	uint64_t u64;
	struct cvmx_sli_s2m_portx_ctl_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_5_63                : 59;
	uint64_t wind_d                       : 1;  /**< When set '1' disables access to the Window
                                                         Registers from the MAC-Port. */
	uint64_t bar0_d                       : 1;  /**< When set '1' disables access from MAC to
                                                         BAR-0 address offsets: Less Than 0x330,
                                                         0x3CD0, and greater than 0x3D70. */
	uint64_t mrrs                         : 3;  /**< Max Read Request Size
                                                                 0 = 128B
                                                                 1 = 256B
                                                                 2 = 512B
                                                                 3 = 1024B
                                                                 4 = 2048B
                                                                 5-7 = Reserved
                                                         This field should not exceed the desired
                                                               max read request size. This field is used to
                                                               determine if an IOBDMA is too large.
                                                         For a PCIe MAC, this field should not exceed
                                                               PCIE*_CFG030[MRRS].
                                                         For a sRIO MAC, this field should indicate a size
                                                               of 256B or smaller. */
#else
	uint64_t mrrs                         : 3;
	uint64_t bar0_d                       : 1;
	uint64_t wind_d                       : 1;
	uint64_t reserved_5_63                : 59;
#endif
	} s;
	struct cvmx_sli_s2m_portx_ctl_s       cn63xx;
	struct cvmx_sli_s2m_portx_ctl_s       cn63xxp1;
};
typedef union cvmx_sli_s2m_portx_ctl cvmx_sli_s2m_portx_ctl_t;

/**
 * cvmx_sli_scratch_1
 *
 * SLI_SCRATCH_1 = SLI's Scratch 1
 *
 * A general purpose 64 bit register for SW use.
 */
union cvmx_sli_scratch_1
{
	uint64_t u64;
	struct cvmx_sli_scratch_1_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t data                         : 64; /**< The value in this register is totaly SW dependent. */
#else
	uint64_t data                         : 64;
#endif
	} s;
	struct cvmx_sli_scratch_1_s           cn63xx;
	struct cvmx_sli_scratch_1_s           cn63xxp1;
};
typedef union cvmx_sli_scratch_1 cvmx_sli_scratch_1_t;

/**
 * cvmx_sli_scratch_2
 *
 * SLI_SCRATCH_2 = SLI's Scratch 2
 *
 * A general purpose 64 bit register for SW use.
 */
union cvmx_sli_scratch_2
{
	uint64_t u64;
	struct cvmx_sli_scratch_2_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t data                         : 64; /**< The value in this register is totaly SW dependent. */
#else
	uint64_t data                         : 64;
#endif
	} s;
	struct cvmx_sli_scratch_2_s           cn63xx;
	struct cvmx_sli_scratch_2_s           cn63xxp1;
};
typedef union cvmx_sli_scratch_2 cvmx_sli_scratch_2_t;

/**
 * cvmx_sli_state1
 *
 * SLI_STATE1 = SLI State 1
 *
 * State machines in SLI. For debug.
 */
union cvmx_sli_state1
{
	uint64_t u64;
	struct cvmx_sli_state1_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t cpl1                         : 12; /**< CPL1 State */
	uint64_t cpl0                         : 12; /**< CPL0 State */
	uint64_t arb                          : 1;  /**< ARB State */
	uint64_t csr                          : 39; /**< CSR State */
#else
	uint64_t csr                          : 39;
	uint64_t arb                          : 1;
	uint64_t cpl0                         : 12;
	uint64_t cpl1                         : 12;
#endif
	} s;
	struct cvmx_sli_state1_s              cn63xx;
	struct cvmx_sli_state1_s              cn63xxp1;
};
typedef union cvmx_sli_state1 cvmx_sli_state1_t;

/**
 * cvmx_sli_state2
 *
 * SLI_STATE2 = SLI State 2
 *
 * State machines in SLI. For debug.
 */
union cvmx_sli_state2
{
	uint64_t u64;
	struct cvmx_sli_state2_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_56_63               : 8;
	uint64_t nnp1                         : 8;  /**< NNP1 State */
	uint64_t reserved_47_47               : 1;
	uint64_t rac                          : 1;  /**< RAC State */
	uint64_t csm1                         : 15; /**< CSM1 State */
	uint64_t csm0                         : 15; /**< CSM0 State */
	uint64_t nnp0                         : 8;  /**< NNP0 State */
	uint64_t nnd                          : 8;  /**< NND State */
#else
	uint64_t nnd                          : 8;
	uint64_t nnp0                         : 8;
	uint64_t csm0                         : 15;
	uint64_t csm1                         : 15;
	uint64_t rac                          : 1;
	uint64_t reserved_47_47               : 1;
	uint64_t nnp1                         : 8;
	uint64_t reserved_56_63               : 8;
#endif
	} s;
	struct cvmx_sli_state2_s              cn63xx;
	struct cvmx_sli_state2_s              cn63xxp1;
};
typedef union cvmx_sli_state2 cvmx_sli_state2_t;

/**
 * cvmx_sli_state3
 *
 * SLI_STATE3 = SLI State 3
 *
 * State machines in SLI. For debug.
 */
union cvmx_sli_state3
{
	uint64_t u64;
	struct cvmx_sli_state3_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_56_63               : 8;
	uint64_t psm1                         : 15; /**< PSM1 State */
	uint64_t psm0                         : 15; /**< PSM0 State */
	uint64_t nsm1                         : 13; /**< NSM1 State */
	uint64_t nsm0                         : 13; /**< NSM0 State */
#else
	uint64_t nsm0                         : 13;
	uint64_t nsm1                         : 13;
	uint64_t psm0                         : 15;
	uint64_t psm1                         : 15;
	uint64_t reserved_56_63               : 8;
#endif
	} s;
	struct cvmx_sli_state3_s              cn63xx;
	struct cvmx_sli_state3_s              cn63xxp1;
};
typedef union cvmx_sli_state3 cvmx_sli_state3_t;

/**
 * cvmx_sli_win_rd_addr
 *
 * SLI_WIN_RD_ADDR = SLI Window Read Address Register
 *
 * The address to be read when the SLI_WIN_RD_DATA register is read.
 * This register should NOT be used to read SLI_* registers.
 */
union cvmx_sli_win_rd_addr
{
	uint64_t u64;
	struct cvmx_sli_win_rd_addr_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_51_63               : 13;
	uint64_t ld_cmd                       : 2;  /**< The load command sent wit hthe read.
                                                         0x3 == Load 8-bytes, 0x2 == Load 4-bytes,
                                                         0x1 == Load 2-bytes, 0x0 == Load 1-bytes, */
	uint64_t iobit                        : 1;  /**< A 1 or 0 can be written here but will not be used
                                                         in address generation. */
	uint64_t rd_addr                      : 48; /**< The address to be read from.
                                                         [47:40] = NCB_ID
                                                         [39:0]  = Address
                                                         When [47:43] == SLI & [42:40] == 0 bits [39:0] are:
                                                              [39:32] == x, Not Used
                                                              [31:24] == RSL_ID
                                                              [23:0]  == RSL Register Offset */
#else
	uint64_t rd_addr                      : 48;
	uint64_t iobit                        : 1;
	uint64_t ld_cmd                       : 2;
	uint64_t reserved_51_63               : 13;
#endif
	} s;
	struct cvmx_sli_win_rd_addr_s         cn63xx;
	struct cvmx_sli_win_rd_addr_s         cn63xxp1;
};
typedef union cvmx_sli_win_rd_addr cvmx_sli_win_rd_addr_t;

/**
 * cvmx_sli_win_rd_data
 *
 * SLI_WIN_RD_DATA = SLI Window Read Data Register
 *
 * Reading this register causes a window read operation to take place. Address read is that contained in the SLI_WIN_RD_ADDR
 * register.
 */
union cvmx_sli_win_rd_data
{
	uint64_t u64;
	struct cvmx_sli_win_rd_data_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t rd_data                      : 64; /**< The read data. */
#else
	uint64_t rd_data                      : 64;
#endif
	} s;
	struct cvmx_sli_win_rd_data_s         cn63xx;
	struct cvmx_sli_win_rd_data_s         cn63xxp1;
};
typedef union cvmx_sli_win_rd_data cvmx_sli_win_rd_data_t;

/**
 * cvmx_sli_win_wr_addr
 *
 * Add Lock Register (Set on Read, Clear on write), SW uses to control access to BAR0 space.
 *
 * Total Address is 16Kb; 0x0000 - 0x3fff, 0x000 - 0x7fe(Reg, every other 8B)
 *
 * General  5kb; 0x0000 - 0x13ff, 0x000 - 0x27e(Reg-General)
 * PktMem  10Kb; 0x1400 - 0x3bff, 0x280 - 0x77e(Reg-General-Packet)
 * Rsvd     1Kb; 0x3c00 - 0x3fff, 0x780 - 0x7fe(Reg-NCB Only Mode)
 *
 *                  SLI_WIN_WR_ADDR = SLI Window Write Address Register
 *
 * Contains the address to be writen to when a write operation is started by writing the
 * SLI_WIN_WR_DATA register (see below).
 *
 * This register should NOT be used to write SLI_* registers.
 */
union cvmx_sli_win_wr_addr
{
	uint64_t u64;
	struct cvmx_sli_win_wr_addr_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_49_63               : 15;
	uint64_t iobit                        : 1;  /**< A 1 or 0 can be written here but this will always
                                                         read as '0'. */
	uint64_t wr_addr                      : 45; /**< The address that will be written to when the
                                                         SLI_WIN_WR_DATA register is written.
                                                         [47:40] = NCB_ID
                                                         [39:3]  = Address
                                                         When [47:43] == SLI & [42:40] == 0 bits [39:0] are:
                                                              [39:32] == x, Not Used
                                                              [31:24] == RSL_ID
                                                              [23:3]  == RSL Register Offset */
	uint64_t reserved_0_2                 : 3;
#else
	uint64_t reserved_0_2                 : 3;
	uint64_t wr_addr                      : 45;
	uint64_t iobit                        : 1;
	uint64_t reserved_49_63               : 15;
#endif
	} s;
	struct cvmx_sli_win_wr_addr_s         cn63xx;
	struct cvmx_sli_win_wr_addr_s         cn63xxp1;
};
typedef union cvmx_sli_win_wr_addr cvmx_sli_win_wr_addr_t;

/**
 * cvmx_sli_win_wr_data
 *
 * SLI_WIN_WR_DATA = SLI Window Write Data Register
 *
 * Contains the data to write to the address located in the SLI_WIN_WR_ADDR Register.
 * Writing the least-significant-byte of this register will cause a write operation to take place.
 */
union cvmx_sli_win_wr_data
{
	uint64_t u64;
	struct cvmx_sli_win_wr_data_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t wr_data                      : 64; /**< The data to be written. Whenever the LSB of this
                                                         register is written, the Window Write will take
                                                         place. */
#else
	uint64_t wr_data                      : 64;
#endif
	} s;
	struct cvmx_sli_win_wr_data_s         cn63xx;
	struct cvmx_sli_win_wr_data_s         cn63xxp1;
};
typedef union cvmx_sli_win_wr_data cvmx_sli_win_wr_data_t;

/**
 * cvmx_sli_win_wr_mask
 *
 * SLI_WIN_WR_MASK = SLI Window Write Mask Register
 *
 * Contains the mask for the data in the SLI_WIN_WR_DATA Register.
 */
union cvmx_sli_win_wr_mask
{
	uint64_t u64;
	struct cvmx_sli_win_wr_mask_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_8_63                : 56;
	uint64_t wr_mask                      : 8;  /**< The data to be written. When a bit is '1'
                                                         the corresponding byte will be written. The values
                                                         of this field must be contiguos and for 1, 2, 4, or
                                                         8 byte operations and aligned to operation size.
                                                         A Value of 0 will produce unpredictable results */
#else
	uint64_t wr_mask                      : 8;
	uint64_t reserved_8_63                : 56;
#endif
	} s;
	struct cvmx_sli_win_wr_mask_s         cn63xx;
	struct cvmx_sli_win_wr_mask_s         cn63xxp1;
};
typedef union cvmx_sli_win_wr_mask cvmx_sli_win_wr_mask_t;

/**
 * cvmx_sli_window_ctl
 *
 * // *
 * // * 81e0 - 82d0 Reserved for future subids
 * // *
 *
 *                   SLI_WINDOW_CTL = SLI's Window Control
 *
 *  Access to register space on the NCB (caused by Window Reads/Writes) will wait for a period of time specified
 *  by this register before timeing out. Because a Window Access can access the RML, which has a fixed timeout of 0xFFFF
 *  core clocks, the value of this register should be set to a minimum of 0x200000 to ensure that a timeout to an RML register
 *  occurs on the RML 0xFFFF timer before the timeout for a BAR0 access from the MAC.
 */
union cvmx_sli_window_ctl
{
	uint64_t u64;
	struct cvmx_sli_window_ctl_s
	{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint64_t reserved_32_63               : 32;
	uint64_t time                         : 32; /**< Time to wait in core clocks for a
                                                         BAR0 access to completeon the NCB
                                                         before timing out. A value of 0 will cause no
                                                         timeouts. A minimum value of 0x200000 should be
                                                         used when this register is not set to 0x0. */
#else
	uint64_t time                         : 32;
	uint64_t reserved_32_63               : 32;
#endif
	} s;
	struct cvmx_sli_window_ctl_s          cn63xx;
	struct cvmx_sli_window_ctl_s          cn63xxp1;
};
typedef union cvmx_sli_window_ctl cvmx_sli_window_ctl_t;

#endif

