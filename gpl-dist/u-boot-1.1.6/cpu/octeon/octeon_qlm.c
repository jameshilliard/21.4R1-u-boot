/***********************license start************************************
 * Copyright (c) 2013 Cavium, Inc. <support@cavium.com>.  All rights
 * reserved.
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
 *     * Neither the name of Cavium Inc. nor the names of
 *       its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 *
 * TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
 * AND WITH ALL FAULTS AND CAVIUM INC. MAKES NO PROMISES, REPRESENTATIONS
 * OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 * RESPECT TO THE SOFTWARE, INCLUDING ITS CONDITION, ITS CONFORMITY TO ANY
 * REPRESENTATION OR DESCRIPTION, OR THE EXISTENCE OF ANY LATENT OR PATENT
 * DEFECTS, AND CAVIUM SPECIFICALLY DISCLAIMS ALL IMPLIED (IF ANY) WARRANTIES
 * OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A PARTICULAR
 * PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET
 * POSSESSION OR CORRESPONDENCE TO DESCRIPTION.  THE ENTIRE RISK ARISING OUT
 * OF USE OR PERFORMANCE OF THE SOFTWARE LIES WITH YOU.
 *
 *
 * For any questions regarding licensing please contact
 * marketing@cavium.com
 *
 ***********************license end**************************************/

#include <common.h>
#include <cvmx.h>
#include <cvmx-gserx-defs.h>
#include <cvmx-mio-defs.h>
#include <lib_octeon_shared.h>

DECLARE_GLOBAL_DATA_PTR;


static enum cvmx_qlm_mode
__cvmx_qlm_get_mode_cn70xx (int qlm)
{
#ifndef CVMX_BUILD_FOR_LINUX_HOST
    union cvmx_gserx_dlmx_phy_reset phy_reset;

    phy_reset.u64 = cvmx_read_csr(CVMX_GSERX_DLMX_PHY_RESET(qlm, 0));
    if (phy_reset.s.phy_reset)
        return CVMX_QLM_MODE_DISABLED;
#endif

	switch(qlm) {
	case 0: /* DLM0/DLM1 - SGMII/QSGMII/RXAUI */
		{
			cvmx_gmxx_inf_mode_t inf_mode0, inf_mode1;

			inf_mode0.u64 = cvmx_read_csr(CVMX_GMXX_INF_MODE(0));
			inf_mode1.u64 = cvmx_read_csr(CVMX_GMXX_INF_MODE(1));

			/* SGMII0 SGMII1 */
			switch (inf_mode0.s.mode) {
			case CVMX_GMX_INF_MODE_SGMII:
				switch (inf_mode1.s.mode) {
				case CVMX_GMX_INF_MODE_SGMII:
					return CVMX_QLM_MODE_SGMII_SGMII;
				case CVMX_GMX_INF_MODE_QSGMII:
					return CVMX_QLM_MODE_SGMII_QSGMII;
				default:
					return CVMX_QLM_MODE_SGMII_DISABLED;
				}
			case CVMX_GMX_INF_MODE_QSGMII:
				switch (inf_mode1.s.mode) {
				case CVMX_GMX_INF_MODE_SGMII:
					return CVMX_QLM_MODE_QSGMII_SGMII;
				case CVMX_GMX_INF_MODE_QSGMII:
					return CVMX_QLM_MODE_QSGMII_QSGMII;
				default:
					return CVMX_QLM_MODE_QSGMII_DISABLED;
				}
			case CVMX_GMX_INF_MODE_RXAUI:
				return CVMX_QLM_MODE_RXAUI_1X2;
			default:
				return CVMX_QLM_MODE_DISABLED;
			}
		}
        case 1:
        case 2:
	default:
		return CVMX_QLM_MODE_DISABLED;
	}

	return CVMX_QLM_MODE_DISABLED;
}

enum cvmx_qlm_mode cvmx_qlm_get_dlm_mode(int qlm, int interface)
{
	enum cvmx_qlm_mode qlm_mode = __cvmx_qlm_get_mode_cn70xx(qlm);

	switch (interface) {
	case 0:
		switch (qlm_mode) {
		case CVMX_QLM_MODE_SGMII_SGMII:
		case CVMX_QLM_MODE_SGMII_DISABLED:
		case CVMX_QLM_MODE_SGMII_QSGMII:
			return CVMX_QLM_MODE_SGMII;
		case CVMX_QLM_MODE_QSGMII_QSGMII:
		case CVMX_QLM_MODE_QSGMII_DISABLED:
		case CVMX_QLM_MODE_QSGMII_SGMII:
			return CVMX_QLM_MODE_QSGMII;
		case CVMX_QLM_MODE_RXAUI_1X2:
			return CVMX_QLM_MODE_RXAUI;
		default:
			return CVMX_QLM_MODE_DISABLED;
		}
	case 1:
		switch (qlm_mode) {
		case CVMX_QLM_MODE_SGMII_SGMII:
		case CVMX_QLM_MODE_DISABLED_SGMII:
		case CVMX_QLM_MODE_QSGMII_SGMII:
			return CVMX_QLM_MODE_SGMII;
		case CVMX_QLM_MODE_QSGMII_QSGMII:
		case CVMX_QLM_MODE_DISABLED_QSGMII:
		case CVMX_QLM_MODE_SGMII_QSGMII:
			return CVMX_QLM_MODE_QSGMII;
		default:
			return CVMX_QLM_MODE_DISABLED;
		}
	default:
		return qlm_mode;
	}
}



/*
 * Read QLM and return mode.
 */
enum cvmx_qlm_mode cvmx_qlm_get_mode(int qlm)
{
	if (OCTEON_IS_MODEL(OCTEON_CN70XX))
		return __cvmx_qlm_get_mode_cn70xx(qlm);

	return CVMX_QLM_MODE_DISABLED;
}



/**
 * This macro spins on a field waiting for it to reach a value. It
 * is common in code to need to wait for a specific field in a CSR
 * to match a specific value. Conceptually this macro expands to:
 *
 * 1) read csr at "address" with a csr typedef of "type"
 * 2) Check if ("type".s."field" "op" "value")
 * 3) If #2 isn't true loop to #1 unless too much time has passed.
 */
#define CVMX_WAIT_FOR_FIELD64(address, type, field, op, value, timeout_usec)\
    ({int result;                                                       \
    do {                                                                \
        uint64_t done = cvmx_clock_get_count(CVMX_CLOCK_CORE) + (uint64_t)timeout_usec * \
                        cvmx_clock_get_rate(CVMX_CLOCK_CORE) / 1000000; \
        type c;                                                         \
        while (1)                                                       \
        {                                                               \
            c.u64 = cvmx_read_csr(address);                             \
            if ((c.s.field) op (value)) {                               \
                result = 0;                                             \
                break;                                                  \
            } else if (cvmx_clock_get_count(CVMX_CLOCK_CORE) > done) {  \
                result = -1;                                            \
                break;                                                  \
            } else                                                      \
                cvmx_wait(100);                                         \
        }                                                               \
    } while (0);                                                        \
    result;})




/**
 * Measure the reference clock of a QLM
 *
 * @param qlm    QLM to measure
 *
 * @return Clock rate in Hz
 *       */
int cvmx_qlm_measure_clock(int qlm)
{
        cvmx_mio_ptp_clock_cfg_t ptp_clock;
        uint64_t count;
        uint64_t start_cycle, stop_cycle;

        if (OCTEON_IS_MODEL(OCTEON_CN3XXX) || OCTEON_IS_MODEL(OCTEON_CN5XXX))
                return -1;

        /* Disable the PTP event counter while we configure it */
        ptp_clock.u64 = cvmx_read_csr(CVMX_MIO_PTP_CLOCK_CFG);  /* For CN63XXp1 errata */
        ptp_clock.s.evcnt_en = 0;
        cvmx_write_csr(CVMX_MIO_PTP_CLOCK_CFG, ptp_clock.u64);
        /* Count on rising edge, Choose which QLM to count */
        ptp_clock.u64 = cvmx_read_csr(CVMX_MIO_PTP_CLOCK_CFG);  /* For CN63XXp1 errata */
        ptp_clock.s.evcnt_edge = 0;
        ptp_clock.s.evcnt_in = 0x10 + qlm;
        cvmx_write_csr(CVMX_MIO_PTP_CLOCK_CFG, ptp_clock.u64);
        /* Clear MIO_PTP_EVT_CNT */
        cvmx_read_csr(CVMX_MIO_PTP_EVT_CNT);    /* For CN63XXp1 errata */
        count = cvmx_read_csr(CVMX_MIO_PTP_EVT_CNT);
        cvmx_write_csr(CVMX_MIO_PTP_EVT_CNT, -count);
        /* Set MIO_PTP_EVT_CNT to 1 billion */
        cvmx_write_csr(CVMX_MIO_PTP_EVT_CNT, 1000000000);
        /* Enable the PTP event counter */
        ptp_clock.u64 = cvmx_read_csr(CVMX_MIO_PTP_CLOCK_CFG);  /* For CN63XXp1 errata */
        ptp_clock.s.evcnt_en = 1;
        cvmx_write_csr(CVMX_MIO_PTP_CLOCK_CFG, ptp_clock.u64);
        start_cycle = cvmx_clock_get_count(CVMX_CLOCK_CORE);
        /* Wait for 50ms */
        cvmx_wait_usec(50000);
        /* Read the counter */
        cvmx_read_csr(CVMX_MIO_PTP_EVT_CNT);    /* For CN63XXp1 errata */
        count = cvmx_read_csr(CVMX_MIO_PTP_EVT_CNT);
        stop_cycle = cvmx_clock_get_count(CVMX_CLOCK_CORE);
        /* Disable the PTP event counter */
        ptp_clock.u64 = cvmx_read_csr(CVMX_MIO_PTP_CLOCK_CFG);  /* For CN63XXp1 errata */
        ptp_clock.s.evcnt_en = 0;
        cvmx_write_csr(CVMX_MIO_PTP_CLOCK_CFG, ptp_clock.u64);
        /* Clock counted down, so reverse it */
        count = 1000000000 - count;
        /* Return the rate */
        return count * cvmx_clock_get_rate(CVMX_CLOCK_CORE) / (stop_cycle - start_cycle);
}

int cvmx_qlm_is_ref_clock(int qlm, int reference_mhz)
{
	int ref_clock = cvmx_qlm_measure_clock(qlm);
	int mhz = ref_clock / 1000000;
	int range = reference_mhz / 10;
	return ((mhz >= reference_mhz - range) &&
		(mhz <= reference_mhz + range));
}

static int __get_qlm_spd(int qlm, int speed)
{
	int qlm_spd = 0xf;

	if (OCTEON_IS_MODEL(OCTEON_CN3XXX) || OCTEON_IS_MODEL(OCTEON_CN5XXX))
		return -1;

	if (cvmx_qlm_is_ref_clock(qlm, 100)) {
		if (speed == 1250)
			qlm_spd = 0x3;
		else if (speed == 2500)
			qlm_spd = 0x2;
		else if (speed == 5000)
			qlm_spd = 0x0;
		else {
			qlm_spd = 0xf;
		}
	} else if (cvmx_qlm_is_ref_clock(qlm, 125)) {
		if (speed == 1250)
			qlm_spd = 0xa;
		else if (speed == 2500)
			qlm_spd = 0x9;
		else if (speed == 3125)
			qlm_spd = 0x8;
		else if (speed == 5000)
			qlm_spd = 0x6;
		else if (speed == 6250)
			qlm_spd = 0x5;
		else {
			qlm_spd = 0xf;
		}
	} else if (cvmx_qlm_is_ref_clock(qlm, 156)) {
		if (speed == 1250)
			qlm_spd = 0x4;
		else if (speed == 2500)
			qlm_spd = 0x7;
		else if (speed == 3125)
			qlm_spd = 0xe;
		else if (speed == 3750)
			qlm_spd = 0xd;
		else if (speed == 5000)
			qlm_spd = 0xb;
		else if (speed == 6250)
			qlm_spd = 0xc;
		else {
			qlm_spd = 0xf;
		}
	}
	return qlm_spd;
}

static int __dlm_set_mult(int qlm, int baud_mhz)
{
	cvmx_gserx_dlmx_ref_clkdiv2_t ref_clkdiv2;
	uint64_t meas_refclock, mult;

	ref_clkdiv2.u64 = cvmx_read_csr(CVMX_GSERX_DLMX_REF_CLKDIV2(qlm, 0));
	if (ref_clkdiv2.s.ref_clkdiv2 == 0) {
		ref_clkdiv2.s.ref_clkdiv2 = 1;
		cvmx_write_csr(CVMX_GSERX_DLMX_REF_CLKDIV2(qlm, 0),
			       ref_clkdiv2.u64);
		cvmx_wait(10000);
	}

	return 0;
}

/* qlm      : DLM to configure
 * baud_mhz : speed of the DLM
 * ref_clk_sel  :  reference clock selection
 * ref_clk_input:  reference clock input
 */
static int __dlm_setup_pll_cn70xx(int qlm, int baud_mhz, int ref_clk_sel,
				  int ref_clk_input)
{
	cvmx_gserx_dlmx_test_powerdown_t dlmx_test_powerdown;
	cvmx_gserx_dlmx_ref_ssp_en_t dlmx_ref_ssp_en;
	cvmx_gserx_dlmx_mpll_en_t dlmx_mpll_en;
	cvmx_gserx_dlmx_phy_reset_t dlmx_phy_reset;
	cvmx_gserx_dlmx_tx_amplitude_t tx_amplitude;
	cvmx_gserx_dlmx_tx_preemph_t tx_preemph;
	cvmx_gserx_dlmx_rx_eq_t rx_eq;
	uint64_t mult = 0;

	/* Hardware defaults are invalid */
	tx_amplitude.u64 = cvmx_read_csr(CVMX_GSERX_DLMX_TX_AMPLITUDE(qlm, 0));
	tx_amplitude.s.tx0_amplitude = 65;
	tx_amplitude.s.tx1_amplitude = 65;
	cvmx_write_csr(CVMX_GSERX_DLMX_TX_AMPLITUDE(qlm, 0), tx_amplitude.u64);

	tx_preemph.u64 = cvmx_read_csr(CVMX_GSERX_DLMX_TX_PREEMPH(qlm, 0));
	tx_preemph.s.tx0_preemph = 22;
	tx_preemph.s.tx1_preemph = 22;
	cvmx_write_csr(CVMX_GSERX_DLMX_TX_PREEMPH(qlm, 0), tx_preemph.u64);

	rx_eq.u64 = cvmx_read_csr(CVMX_GSERX_DLMX_RX_EQ(qlm, 0));
	rx_eq.s.rx0_eq = 0;
	rx_eq.s.rx1_eq = 0;
	cvmx_write_csr(CVMX_GSERX_DLMX_RX_EQ(qlm, 0), rx_eq.u64);

	/* 1. Write GSER0_DLM0_REF_USE_PAD[REF_USE_PAD] = 1 (to select
	 *    reference-clock input)
	 */
	cvmx_write_csr(CVMX_GSERX_DLMX_REF_USE_PAD(0, 0), ref_clk_input);

	/* Reference clock was already chosen before we got here */

	/* 2. Write GSER0_DLM0_REFCLK_SEL[REFCLK_SEL] if required for
	 *    reference-clock selection.
	 */
	cvmx_write_csr(CVMX_GSERX_DLMX_REFCLK_SEL(0, 0), ref_clk_sel);

	/* Reference clock was already chosen before we got here */

	/* 3. If required, write GSER0_DLM0_REF_CLKDIV2[REF_CLKDIV2] (must be
	 *    set if reference clock > 100 MHz)
	 */

	/* 5. Clear GSER0_DLM0_TEST_POWERDOWN[TEST_POWERDOWN] */
	dlmx_test_powerdown.u64 =
			cvmx_read_csr(CVMX_GSERX_DLMX_TEST_POWERDOWN(qlm, 0));
	dlmx_test_powerdown.s.test_powerdown = 0;
	cvmx_write_csr(CVMX_GSERX_DLMX_TEST_POWERDOWN(qlm, 0),
		       dlmx_test_powerdown.u64);

	/* 6. Set GSER0_DLM0_REF_SSP_EN[REF_SSP_EN] = 1 */
	dlmx_ref_ssp_en.u64 = cvmx_read_csr(CVMX_GSERX_DLMX_REF_SSP_EN(qlm, 0));
	dlmx_ref_ssp_en.s.ref_ssp_en = 1;
	cvmx_write_csr(CVMX_GSERX_DLMX_REF_SSP_EN(0, 0), dlmx_ref_ssp_en.u64);

	if (__dlm_set_mult(qlm, baud_mhz))
		return -1;

	/* Set GSER0_DLM0_PHY_RESET[PHY_RESET] = 1 */
	dlmx_phy_reset.u64 = cvmx_read_csr(CVMX_GSERX_DLMX_PHY_RESET(qlm, 0));
	dlmx_phy_reset.s.phy_reset = 1;
	cvmx_write_csr(CVMX_GSERX_DLMX_PHY_RESET(qlm, 0), dlmx_phy_reset.u64);

	/* Set GSER0_DLM0_MPLL_EN[MPLL_EN] = 1 */
	dlmx_mpll_en.u64 = cvmx_read_csr(CVMX_GSERX_DLMX_MPLL_EN(0, 0));
	dlmx_mpll_en.s.mpll_en = 1;
	cvmx_write_csr(CVMX_GSERX_DLMX_MPLL_EN(0, 0), dlmx_mpll_en.u64);

	mult = 0x38;
	cvmx_write_csr(CVMX_GSERX_DLMX_MPLL_MULTIPLIER(qlm, 0), mult);

	/* Clear GSER0_DLM0_PHY_RESET[PHY_RESET] = 1 */
	dlmx_phy_reset.u64 = cvmx_read_csr(CVMX_GSERX_DLMX_PHY_RESET(qlm, 0));
	dlmx_phy_reset.s.phy_reset = 0;
	cvmx_write_csr(CVMX_GSERX_DLMX_PHY_RESET(qlm, 0), dlmx_phy_reset.u64);

	mult = 0x37;
	udelay(10000);

	do {
		cvmx_write_csr(CVMX_GSERX_DLMX_MPLL_MULTIPLIER(qlm, 0), mult);
		udelay(10000);
	} while (mult-- > 0x28);

	/* 9. Poll until the MPLL locks. Wait for
	 *    GSER0_DLM0_MPLL_STATUS[MPLL_STATUS] = 1
	 */
	if (CVMX_WAIT_FOR_FIELD64(CVMX_GSERX_DLMX_MPLL_STATUS(qlm, 0),
				      cvmx_gserx_dlmx_mpll_status_t,
				      mpll_status, ==, 1, 10000)) {
		cvmx_warn("PLL for DLM%d failed to lock\n", qlm);
		return -1;
	}
	return 0;
}

static int __dlm0_setup_tx_cn70xx(void)
{
	int need0, need1;
	cvmx_gmxx_inf_mode_t mode0, mode1;
	cvmx_gserx_dlmx_tx_rate_t rate;
	cvmx_gserx_dlmx_tx_en_t en;
	cvmx_gserx_dlmx_tx_cm_en_t cm_en;
	cvmx_gserx_dlmx_tx_data_en_t data_en;
	cvmx_gserx_dlmx_tx_reset_t tx_reset;

	mode0.u64 = cvmx_read_csr(CVMX_GMXX_INF_MODE(0));
	mode1.u64 = cvmx_read_csr(CVMX_GMXX_INF_MODE(1));

	/* Which lanes do we need? */
	need0 = (mode0.s.mode != CVMX_GMX_INF_MODE_DISABLED);
	need1 = (mode1.s.mode != CVMX_GMX_INF_MODE_DISABLED)
		 || (mode0.s.mode == CVMX_GMX_INF_MODE_RXAUI);

	/* 1. Write GSER0_DLM0_TX_RATE[TXn_RATE] (Set according to required
	      data rate (see Table 21-1). */
	rate.u64 = cvmx_read_csr(CVMX_GSERX_DLMX_TX_RATE(0, 0));
	rate.s.tx0_rate = (mode0.s.mode == CVMX_GMX_INF_MODE_SGMII) ? 2 : 0;
	rate.s.tx1_rate = (mode1.s.mode == CVMX_GMX_INF_MODE_SGMII) ? 2 : 0;
	cvmx_write_csr(CVMX_GSERX_DLMX_TX_RATE(0, 0), rate.u64);

	/* 2. Set GSER0_DLM0_TX_EN[TXn_EN] = 1 */
	en.u64 = cvmx_read_csr(CVMX_GSERX_DLMX_TX_EN(0, 0));
	en.s.tx0_en = need0;
	en.s.tx1_en = need1;
	cvmx_write_csr(CVMX_GSERX_DLMX_TX_EN(0, 0), en.u64);

	/* 3 set GSER0_DLM0_TX_CM_EN[TXn_CM_EN] = 1 */
	cm_en.u64 = cvmx_read_csr(CVMX_GSERX_DLMX_TX_CM_EN(0, 0));
	cm_en.s.tx0_cm_en = need0;
	cm_en.s.tx1_cm_en = need1;
	cvmx_write_csr(CVMX_GSERX_DLMX_TX_CM_EN(0, 0), cm_en.u64);

	/* 4. Set GSER0_DLM0_TX_DATA_EN[TXn_DATA_EN] = 1 */
	data_en.u64 = cvmx_read_csr(CVMX_GSERX_DLMX_TX_DATA_EN(0, 0));
	data_en.s.tx0_data_en = need0;
	data_en.s.tx1_data_en = need1;
	cvmx_write_csr(CVMX_GSERX_DLMX_TX_DATA_EN(0, 0), data_en.u64);

	/* 5. Clear GSER0_DLM0_TX_RESET[TXn_DATA_EN] = 0 */
	tx_reset.u64 = cvmx_read_csr(CVMX_GSERX_DLMX_TX_RESET(0, 0));
	tx_reset.s.tx0_reset = !need0;
	tx_reset.s.tx1_reset = !need1;
	cvmx_write_csr(CVMX_GSERX_DLMX_TX_RESET(0, 0), tx_reset.u64);

	/* 6. Poll GSER0_DLM0_TX_STATUS[TXn_STATUS, TXn_CM_STATUS] until both
	 *    are set to 1. This prevents GMX from transmitting until the DLM
	 *    is ready.
	 */
	if (need0) {
		if (CVMX_WAIT_FOR_FIELD64(CVMX_GSERX_DLMX_TX_STATUS(0, 0),
			cvmx_gserx_dlmx_tx_status_t, tx0_status, ==, 1, 10000)) {
			cvmx_warn("DLM0 TX0 status fail\n");
			return -1;
		}
		if (CVMX_WAIT_FOR_FIELD64(CVMX_GSERX_DLMX_TX_STATUS(0, 0),
			cvmx_gserx_dlmx_tx_status_t, tx0_cm_status, ==, 1, 10000)) {
			cvmx_warn("DLM0 TX0 CM status fail\n");
			return -1;
		}
	}
	if (need1) {
		if (CVMX_WAIT_FOR_FIELD64(CVMX_GSERX_DLMX_TX_STATUS(0, 0),
			cvmx_gserx_dlmx_tx_status_t, tx1_status, ==, 1, 10000)) {
			cvmx_warn("DLM0 TX1 status fail\n");
			return -1;
		}
		if (CVMX_WAIT_FOR_FIELD64(CVMX_GSERX_DLMX_TX_STATUS(0, 0),
			cvmx_gserx_dlmx_tx_status_t, tx1_cm_status, ==, 1, 10000)) {
			cvmx_warn("DLM0 TX1 CM status fail\n");
			return -1;
		}
	}
	return 0;
}

static int __dlm0_setup_rx_cn70xx(void)
{
	int need0, need1;
	cvmx_gmxx_inf_mode_t mode0, mode1;
	cvmx_gserx_dlmx_rx_rate_t rate;
	cvmx_gserx_dlmx_rx_pll_en_t pll_en;
	cvmx_gserx_dlmx_rx_data_en_t data_en;
	cvmx_gserx_dlmx_rx_reset_t rx_reset;

	mode0.u64 = cvmx_read_csr(CVMX_GMXX_INF_MODE(0));
	mode1.u64 = cvmx_read_csr(CVMX_GMXX_INF_MODE(1));

	/* Which lanes do we need? */
	need0 = (mode0.s.mode != CVMX_GMX_INF_MODE_DISABLED);
	need1 = (mode1.s.mode != CVMX_GMX_INF_MODE_DISABLED)
		 || (mode0.s.mode == CVMX_GMX_INF_MODE_RXAUI);

	/* 1. Write GSER0_DLM0_RX_RATE[RXn_RATE] (must match the
	   GER0_DLM0_TX_RATE[TXn_RATE] setting). */
	rate.u64 = cvmx_read_csr(CVMX_GSERX_DLMX_RX_RATE(0, 0));
	rate.s.rx0_rate = (mode0.s.mode == CVMX_GMX_INF_MODE_SGMII) ? 2 : 0;
	rate.s.rx1_rate = (mode1.s.mode == CVMX_GMX_INF_MODE_SGMII) ? 2 : 0;
	cvmx_write_csr(CVMX_GSERX_DLMX_RX_RATE(0, 0), rate.u64);

	/* 2. Set GSER0_DLM0_RX_PLL_EN[RXn_PLL_EN] = 1 */
	pll_en.u64 = cvmx_read_csr(CVMX_GSERX_DLMX_RX_PLL_EN(0, 0));
	pll_en.s.rx0_pll_en = need0;
	pll_en.s.rx1_pll_en = need1;
	cvmx_write_csr(CVMX_GSERX_DLMX_RX_PLL_EN(0, 0), pll_en.u64);

	/* 3. Set GSER0_DLM0_RX_DATA_EN[RXn_DATA_EN] = 1 */
	data_en.u64 = cvmx_read_csr(CVMX_GSERX_DLMX_RX_DATA_EN(0, 0));
	data_en.s.rx0_data_en = need0;
	data_en.s.rx1_data_en = need1;
	cvmx_write_csr(CVMX_GSERX_DLMX_RX_DATA_EN(0, 0), data_en.u64);

	/* 4. Clear GSER0_DLM0_RX_RESET[RXn_DATA_EN] = 0. Now the GMX can be
	   enabled: set GMX(0..1)_INF_MODE[EN] = 1 */
	rx_reset.u64 = cvmx_read_csr(CVMX_GSERX_DLMX_RX_RESET(0, 0));
	rx_reset.s.rx0_reset = !need0;
	rx_reset.s.rx1_reset = !need1;
	cvmx_write_csr(CVMX_GSERX_DLMX_RX_RESET(0, 0), rx_reset.u64);

	return 0;
}


/**
 * Configure dlm speed and mode for cn70xx.
 *
 * @param qlm     The DLM to configure
 * @param speed   The speed the DLM needs to be configured in Mhz.
 * @param mode    The DLM to be configured as SGMII/XAUI/PCIe.
 *                  DLM 0: has 2 interfaces which can be configured as
 *                         SGMII/QSGMII/RXAUI. Need to configure both at the
 *                         same time. These are valid option
 *				CVMX_QLM_MODE_QSGMII,
 *				CVMX_QLM_MODE_SGMII_SGMII,
 *				CVMX_QLM_MODE_SGMII_DISABLED,
 *				CVMX_QLM_MODE_DISABLED_SGMII,
 *				CVMX_QLM_MODE_SGMII_QSGMII,
 *				CVMX_QLM_MODE_QSGMII_QSGMII,
 *				CVMX_QLM_MODE_QSGMII_DISABLED,
 *				CVMX_QLM_MODE_DISABLED_QSGMII,
 *				CVMX_QLM_MODE_QSGMII_SGMII,
 *				CVMX_QLM_MODE_RXAUI_1X2
 *
 *                  DLM 1: PEM0/1 in PCIE_1x4/PCIE_2x1/PCIE_1X1
 *                  DLM 2: PEM0/1/2 in PCIE_1x4/PCIE_1x2/PCIE_2x1/PCIE_1x1
 * @param rc      Only used for PCIe, rc = 1 for root complex mode, 0 for EP mode.
 * @param gen2    Only used for PCIe, gen2 = 1, in GEN2 mode else in GEN1 mode.
 *
 * @param ref_clk_input  The reference-clock input to use to configure QLM
 * @param ref_clk_sel    The reference-clock selection to use to configure QLM
 *
 * @return       Return 0 on success or -1.
 */
static int octeon_configure_qlm_cn70xx(int qlm, int speed, int mode, int rc,
				       int gen2, int ref_clk_sel,
				       int ref_clk_input)
{
	switch (qlm) {
	case 0:
		{
			cvmx_gmxx_inf_mode_t inf_mode0, inf_mode1;

			inf_mode0.u64 = cvmx_read_csr(CVMX_GMXX_INF_MODE(0));
			inf_mode1.u64 = cvmx_read_csr(CVMX_GMXX_INF_MODE(1));
			if (inf_mode0.s.en || inf_mode1.s.en) {
				cvmx_dprintf("DLM0 already configured\n");
				return -1;
			}

			switch (mode) {
			case CVMX_QLM_MODE_SGMII_SGMII:
				inf_mode0.s.mode = CVMX_GMX_INF_MODE_SGMII;
				inf_mode1.s.mode = CVMX_GMX_INF_MODE_SGMII;
				break;
			case CVMX_QLM_MODE_SGMII_QSGMII:
				inf_mode0.s.mode = CVMX_GMX_INF_MODE_SGMII;
				inf_mode1.s.mode = CVMX_GMX_INF_MODE_QSGMII;
				break;
			case CVMX_QLM_MODE_SGMII_DISABLED:
				inf_mode0.s.mode = CVMX_GMX_INF_MODE_SGMII;
				inf_mode1.s.mode = CVMX_GMX_INF_MODE_DISABLED;
				break;
			case CVMX_QLM_MODE_DISABLED_SGMII:
				inf_mode0.s.mode = CVMX_GMX_INF_MODE_DISABLED;
				inf_mode1.s.mode = CVMX_GMX_INF_MODE_SGMII;
				break;
			case CVMX_QLM_MODE_QSGMII_SGMII:
				inf_mode0.s.mode = CVMX_GMX_INF_MODE_QSGMII;
				inf_mode1.s.mode = CVMX_GMX_INF_MODE_SGMII;
				break;
			case CVMX_QLM_MODE_QSGMII_QSGMII:
				inf_mode0.s.mode = CVMX_GMX_INF_MODE_QSGMII;
				inf_mode1.s.mode = CVMX_GMX_INF_MODE_QSGMII;
				break;
			case CVMX_QLM_MODE_QSGMII_DISABLED:
				inf_mode0.s.mode = CVMX_GMX_INF_MODE_QSGMII;
				inf_mode1.s.mode = CVMX_GMX_INF_MODE_DISABLED;
				break;
			case CVMX_QLM_MODE_DISABLED_QSGMII:
				inf_mode0.s.mode = CVMX_GMX_INF_MODE_DISABLED;
				inf_mode1.s.mode = CVMX_GMX_INF_MODE_QSGMII;
				break;
			case CVMX_QLM_MODE_RXAUI:
				inf_mode0.s.mode = CVMX_GMX_INF_MODE_RXAUI;
				inf_mode1.s.mode = CVMX_GMX_INF_MODE_DISABLED;
				break;
			default:
				inf_mode0.s.mode = CVMX_GMX_INF_MODE_DISABLED;
				inf_mode1.s.mode = CVMX_GMX_INF_MODE_DISABLED;
				break;
			}
			cvmx_write_csr(CVMX_GMXX_INF_MODE(0), inf_mode0.u64);
			cvmx_write_csr(CVMX_GMXX_INF_MODE(1), inf_mode1.u64);

			/* Bringup the PLL */
			if (__dlm_setup_pll_cn70xx(qlm, speed, ref_clk_sel,
					 ref_clk_input))
				return -1;

			/* TX Lanes */
			if (__dlm0_setup_tx_cn70xx())
				return -1;

			/* RX Lanes */
			if (__dlm0_setup_rx_cn70xx())
				return -1;

			/* Enable the interface */
			inf_mode0.u64 = cvmx_read_csr(CVMX_GMXX_INF_MODE(0));
			if (inf_mode0.s.mode != CVMX_GMX_INF_MODE_DISABLED)
				inf_mode0.s.en = 1;
			cvmx_write_csr(CVMX_GMXX_INF_MODE(0), inf_mode0.u64);
			inf_mode1.u64 = cvmx_read_csr(CVMX_GMXX_INF_MODE(1));
			if (inf_mode1.s.mode != CVMX_GMX_INF_MODE_DISABLED)
				inf_mode1.s.en = 1;
			cvmx_write_csr(CVMX_GMXX_INF_MODE(1), inf_mode1.u64);
			break;
		}
	case 1:
	case 2:
	default:
			return -1;
	}

	return 0;
}

/**
 * Configure qlm/dlm speed and mode.
 * @param qlm     The QLM or DLM to configure
 * @param speed   The speed the QLM needs to be configured in Mhz.
 * @param mode    The QLM to be configured as SGMII/XAUI/PCIe.
 * @param rc      Only used for PCIe, rc = 1 for root complex mode, 0 for EP
 *		  mode.
 * @param pcie_mode Only used when qlm/dlm are in pcie mode.
 * @param ref_clk_sel Reference clock to use for 70XX
 * @param ref_clk_input	When set selects the external ref_pad_clk_{p,m} inputs
 *			as the reference-clock source.  When deasserted,
 *			ref_alt_clk_{p,m} are selected from an on-chip source of
 *			the reference clock.
 *
 * @return       Return 0 on success or -1.
 */
int octeon_configure_qlm(int qlm, int speed, int mode, int rc, int pcie_mode,
			int ref_clk_sel, int ref_clk_input)
{
	if (OCTEON_IS_MODEL(OCTEON_CN70XX))
		return octeon_configure_qlm_cn70xx(qlm, speed, mode, rc,
						   pcie_mode, ref_clk_sel,
						   ref_clk_input);

        return -1;
}
