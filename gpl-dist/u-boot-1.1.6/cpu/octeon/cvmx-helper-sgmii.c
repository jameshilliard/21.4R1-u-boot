/***********************license start***************
 * Copyright (c) 2003-2008  Cavium Networks (support@cavium.com). All rights
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
 *
 * For any questions regarding licensing please contact marketing@caviumnetworks.com
 *
 ***********************license end**************************************/






/**
 * @file
 *
 * Functions for SGMII initialization, configuration,
 * and monitoring.
 *
 * <hr>$Revision: 1.3.38.1 $<hr>
 */
#include "executive-config.h"
#include "cvmx-config.h"
#ifdef CVMX_ENABLE_PKO_FUNCTIONS

#include "cvmx.h"
#include "cvmx-sysinfo.h"
#include "cvmx-mdio.h"
#include "cvmx-helper.h"
#include "cvmx-helper-board.h"
#include "cvmx-helper-sgmii.h"
#include "octeon_hal.h"
#ifdef CONFIG_JSRXNLE
#include "configs/jsrxnle.h"
#endif
#ifdef CONFIG_MAG8600
#include "configs/mag8600.h"
#endif
#ifdef CONFIG_ACX500_SVCS
#include "lib_octeon_shared.h"
#endif


/**
 * @INTERNAL
 * Perform initialization required only once for an SGMII port.
 *
 * @param interface Interface to init
 * @param index     Index of prot on the interface
 *
 * @return Zero on success, negative on failure
 */
#define OCTEON_CLOCK_DEFAULT (700 * 1000 * 1000)

static int __cvmx_helper_sgmii_hardware_init_one_time(int interface, int index)
{
#ifdef CONFIG_JSRXNLE
    DECLARE_GLOBAL_DATA_PTR;
#endif
    uint64_t clock_mhz = cvmx_sysinfo_get()->cpu_clock_hz / 1000000;
    cvmx_gmxx_inf_mode_t mode;
    cvmx_pcsx_linkx_timer_count_reg_t pcsx_linkx_timer_count_reg;
    cvmx_gmxx_prtx_cfg_t gmxx_prtx_cfg;

#if defined(CONFIG_ACX500_SVCS)
    clock_mhz = cvmx_clock_get_rate(CVMX_CLOCK_SCLK) / 1000000;
#endif
#ifdef CONFIG_JSRXNLE
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX240_MODELS
        clock_mhz = OCTEON_CLOCK_DEFAULT / 1000000;
        break;
    default:
        break;
    }
#endif
    /* Disable GMX */
    gmxx_prtx_cfg.u64 = cvmx_read_csr(CVMX_GMXX_PRTX_CFG(index, interface));
    gmxx_prtx_cfg.s.en = 0;
    cvmx_write_csr(CVMX_GMXX_PRTX_CFG(index, interface), gmxx_prtx_cfg.u64);

    /* Write PCS*_LINK*_TIMER_COUNT_REG[COUNT] with the appropriate
        value. 1000BASE-X specifies a 10ms interval. SGMII specifies a 1.6ms
        interval. */
    mode.u64 = cvmx_read_csr(CVMX_GMXX_INF_MODE(interface));
    pcsx_linkx_timer_count_reg.u64 = cvmx_read_csr(CVMX_PCSX_LINKX_TIMER_COUNT_REG(index, interface));
    if (mode.cn56xx.type)
    {
        /* 1000BASE-X */
        pcsx_linkx_timer_count_reg.s.count = (10000ull * clock_mhz) >> 10;
    }
    else
    {
        /* SGMII */
        pcsx_linkx_timer_count_reg.s.count = (1600ull * clock_mhz) >> 10;
    }
    cvmx_write_csr(CVMX_PCSX_LINKX_TIMER_COUNT_REG(index, interface), pcsx_linkx_timer_count_reg.u64);

    /* Write the advertisement register to be used as the
        tx_Config_Reg<D15:D0> of the autonegotiation.
        In 1000BASE-X mode, tx_Config_Reg<D15:D0> is PCS*_AN*_ADV_REG.
        In SGMII PHY mode, tx_Config_Reg<D15:D0> is PCS*_SGM*_AN_ADV_REG.
        In SGMII MAC mode, tx_Config_Reg<D15:D0> is the fixed value 0x4001, so
        this step can be skipped. */
    if (mode.cn56xx.type)
    {
        /* 1000BASE-X */
        cvmx_pcsx_anx_adv_reg_t pcsx_anx_adv_reg;
        pcsx_anx_adv_reg.u64 = cvmx_read_csr(CVMX_PCSX_ANX_ADV_REG(index, interface));
        pcsx_anx_adv_reg.s.rem_flt = 0;
        pcsx_anx_adv_reg.s.pause = 3;
        pcsx_anx_adv_reg.s.hfd = 1;
        pcsx_anx_adv_reg.s.fd = 1;
        cvmx_write_csr(CVMX_PCSX_ANX_ADV_REG(index, interface), pcsx_anx_adv_reg.u64);
    }
    else
    {
        cvmx_pcsx_miscx_ctl_reg_t pcsx_miscx_ctl_reg;
        pcsx_miscx_ctl_reg.u64 = cvmx_read_csr(CVMX_PCSX_MISCX_CTL_REG(index, interface));

#ifdef CONFIG_JSRXNLE
        switch (gd->board_desc.board_type)
        {
            CASE_ALL_SRX650_MODELS
                pcsx_miscx_ctl_reg.s.mode = 1; /* 1000 BASE-X */
                pcsx_miscx_ctl_reg.s.mac_phy = 1; /* PHY */
                cvmx_write_csr(CVMX_PCSX_MISCX_CTL_REG(index, interface), pcsx_miscx_ctl_reg.u64);
                return 0;
            default:
                break;
        }
#endif
		
        if (pcsx_miscx_ctl_reg.s.mac_phy)
        {
            /* PHY Mode */
            cvmx_pcsx_sgmx_an_adv_reg_t pcsx_sgmx_an_adv_reg;
            pcsx_sgmx_an_adv_reg.u64 = cvmx_read_csr(CVMX_PCSX_SGMX_AN_ADV_REG(index, interface));
            pcsx_sgmx_an_adv_reg.s.link = 1;
            pcsx_sgmx_an_adv_reg.s.dup = 1;
            pcsx_sgmx_an_adv_reg.s.speed= 2;
            cvmx_write_csr(CVMX_PCSX_SGMX_AN_ADV_REG(index, interface), pcsx_sgmx_an_adv_reg.u64);
        }
        else
        {
            /* MAC Mode - Nothing to do */
        }
    }
    return 0;
}


/**
 * @INTERNAL
 * Initialize the SERTES link for the first time or after a loss
 * of link.
 *
 * @param interface Interface to init
 * @param index     Index of prot on the interface
 *
 * @return Zero on success, negative on failure
 */
static int __cvmx_helper_sgmii_hardware_init_link(int interface, int index)
{
#ifdef CONFIG_JSRXNLE
    DECLARE_GLOBAL_DATA_PTR;
    int count = 0;
    int autoneg = 1;
    cvmx_pcsx_mrx_status_reg_t status_reg;
#endif
    cvmx_pcsx_mrx_control_reg_t control_reg;

    /* Take PCS through a reset sequence.
        PCS*_MR*_CONTROL_REG[PWR_DN] should be cleared to zero.
        Write PCS*_MR*_CONTROL_REG[RESET]=1 (while not changing the value of
            the other PCS*_MR*_CONTROL_REG bits).
        Read PCS*_MR*_CONTROL_REG[RESET] until it changes value to zero. */
#ifdef CONFIG_JSRXNLE
    switch (gd->board_desc.board_type)
    {
        CASE_ALL_SRX240_MODELS
        CASE_ALL_SRX650_MODELS
            autoneg  = 0;
            break;
        default:
            break;
    }
    if (!autoneg) {
        /* Disable autonegotiation and use manual mode instead */
        control_reg.u64 = cvmx_read_csr(CVMX_PCSX_MRX_CONTROL_REG(index, interface)); 
        control_reg.s.an_en = 0;
        control_reg.s.dup = 1;
        control_reg.s.spdmsb = 1;
        control_reg.s.spdlsb = 0;
        control_reg.s.pwr_dn = 0;
        cvmx_write_csr(CVMX_PCSX_MRX_CONTROL_REG(index, interface), control_reg.u64);
    }
#endif
    control_reg.u64 = cvmx_read_csr(CVMX_PCSX_MRX_CONTROL_REG(index, interface));
    if (cvmx_sysinfo_get()->board_type != CVMX_BOARD_TYPE_SIM)
    {
        control_reg.s.reset = 1;
        cvmx_write_csr(CVMX_PCSX_MRX_CONTROL_REG(index, interface), control_reg.u64);
        do
        {
            cvmx_wait(100);
            control_reg.u64 = cvmx_read_csr(CVMX_PCSX_MRX_CONTROL_REG(index, interface));
        } while (control_reg.s.reset);
    }

#ifdef CONFIG_JSRXNLE
    switch (gd->board_desc.board_type)
    {
        CASE_ALL_SRX240_MODELS
        CASE_ALL_SRX650_MODELS
            count = 0xffff;
            do {
                cvmx_wait(100);
                status_reg.u64 = cvmx_read_csr(CVMX_PCSX_MRX_STATUS_REG(index, interface));
            } while (!status_reg.s.lnk_st && --count);
            return 0;
        default:
            break;
    }
#endif

#if defined(CONFIG_ACX500_SVCS)
     /* Disable autonegotiation and use manual mode instead */       
     control_reg.u64 = cvmx_read_csr(CVMX_PCSX_MRX_CONTROL_REG(index, interface)); 
     control_reg.s.an_en = 0;
     control_reg.s.dup = 1;
     control_reg.s.spdmsb = 1;
     control_reg.s.spdlsb = 0;
     control_reg.s.pwr_dn = 0;
     cvmx_write_csr(CVMX_PCSX_MRX_CONTROL_REG(index, interface), control_reg.u64);
     return 0;
#else
    /* Write PCS*_MR*_CONTROL_REG[RST_AN]=1 to ensure a fresh sgmii negotiation starts. */
    control_reg.s.rst_an = 1;
    control_reg.s.an_en = 1;
    control_reg.s.pwr_dn = 0;
    cvmx_write_csr(CVMX_PCSX_MRX_CONTROL_REG(index, interface), control_reg.u64);
#endif

    /* Wait for PCS*_MR*_STATUS_REG[AN_CPT] to be set, indicating that
        sgmii autonegotiation is complete. In MAC mode this isn't an ethernet
        link, but a link between Octeon and the PHY */
    if (cvmx_sysinfo_get()->board_type != CVMX_BOARD_TYPE_SIM)
    {
        uint64_t start_cycle = cvmx_get_cycle();
        cvmx_pcsx_mrx_status_reg_t pcsx_mrx_status_reg;
        do
        {
            if (cvmx_get_cycle() - start_cycle > cvmx_sysinfo_get()->cpu_clock_hz/100)
            {
                //cvmx_dprintf("SGMII%d: Port %d link timeout\n", interface, index);
                return -1;
            }
            cvmx_wait(100);
            pcsx_mrx_status_reg.u64 = cvmx_read_csr(CVMX_PCSX_MRX_STATUS_REG(index, interface));
        } while (!pcsx_mrx_status_reg.s.an_cpt);
    }
    return 0;
}


/**
 * @INTERNAL
 * Configure an SGMII link to the specified speed after the SERTES
 * link is up.
 *
 * @param interface Interface to init
 * @param index     Index of prot on the interface
 * @param link_info Link state to configure
 *
 * @return Zero on success, negative on failure
 */
static int __cvmx_helper_sgmii_hardware_init_link_speed(int interface, int index, cvmx_helper_link_info_t link_info)
{
#ifdef CONFIG_JSRXNLE
    DECLARE_GLOBAL_DATA_PTR;
#endif
    int is_enabled;
    cvmx_gmxx_prtx_cfg_t gmxx_prtx_cfg;
    cvmx_pcsx_miscx_ctl_reg_t pcsx_miscx_ctl_reg;

    /* Disable GMX before we make any changes. Remember the enable state */
    gmxx_prtx_cfg.u64 = cvmx_read_csr(CVMX_GMXX_PRTX_CFG(index, interface));
    is_enabled = gmxx_prtx_cfg.s.en;
    gmxx_prtx_cfg.s.en = 0;
    cvmx_write_csr(CVMX_GMXX_PRTX_CFG(index, interface), gmxx_prtx_cfg.u64);

    /* Read GMX CFG again to make sure the disable completed */
    gmxx_prtx_cfg.u64 = cvmx_read_csr(CVMX_GMXX_PRTX_CFG(index, interface));

    /* Wait for GMX to be idle */
    if (!gmxx_prtx_cfg.s.rx_idle || !gmxx_prtx_cfg.s.tx_idle)
    {
        cvmx_dprintf("SGMII%d: Waiting for port %d to be idle\n", interface, index);
        do
        {
            cvmx_wait(1000);
            gmxx_prtx_cfg.u64 = cvmx_read_csr(CVMX_GMXX_PRTX_CFG(index, interface));
        } while (!gmxx_prtx_cfg.s.rx_idle || !gmxx_prtx_cfg.s.tx_idle);
    }

    /* Read GMX CFG again to make sure the disable completed */
    gmxx_prtx_cfg.u64 = cvmx_read_csr(CVMX_GMXX_PRTX_CFG(index, interface));

    /* Get the misc control for PCS. We will need to set the duplication amount */
    pcsx_miscx_ctl_reg.u64 = cvmx_read_csr(CVMX_PCSX_MISCX_CTL_REG(index, interface));

#ifdef CONFIG_JSRXNLE
    switch (gd->board_desc.board_type)
    {
        CASE_ALL_SRX240_MODELS
        CASE_ALL_SRX650_MODELS
            /* Link should be up always */
            pcsx_miscx_ctl_reg.s.gmxeno = 0;
            break;
        default:
            break;
    }
#else
    /* Use GMXENO to force the link down if the status we get says it should be down */
    pcsx_miscx_ctl_reg.s.gmxeno = !link_info.s.link_up;
#endif

#if defined(CONFIG_ACX500_SVCS)
    pcsx_miscx_ctl_reg.s.gmxeno = 0;
#endif

    /* Only change the duplex setting if the link is up */
    if (link_info.s.link_up)
        gmxx_prtx_cfg.s.duplex = link_info.s.full_duplex;

    /* Do speed based setting for GMX */
    switch (link_info.s.speed)
    {
        case 10:
            gmxx_prtx_cfg.s.speed = 0;
            gmxx_prtx_cfg.s.speed_msb = 1;
            gmxx_prtx_cfg.s.slottime = 0;
            pcsx_miscx_ctl_reg.s.samp_pt = 25; /* Setting from GMX-603 */
            cvmx_write_csr(CVMX_GMXX_TXX_SLOT(index, interface), 64);
            cvmx_write_csr(CVMX_GMXX_TXX_BURST(index, interface), 0);
            break;
        case 100:
            gmxx_prtx_cfg.s.speed = 0;
            gmxx_prtx_cfg.s.speed_msb = 0;
            gmxx_prtx_cfg.s.slottime = 0;
            pcsx_miscx_ctl_reg.s.samp_pt = 0x5;
            cvmx_write_csr(CVMX_GMXX_TXX_SLOT(index, interface), 64);
            cvmx_write_csr(CVMX_GMXX_TXX_BURST(index, interface), 0);
            break;
        case 1000:
            gmxx_prtx_cfg.s.speed = 1;
            gmxx_prtx_cfg.s.speed_msb = 0;
            gmxx_prtx_cfg.s.slottime = 1;
            pcsx_miscx_ctl_reg.s.samp_pt = 1;
            cvmx_write_csr(CVMX_GMXX_TXX_SLOT(index, interface), 512);
            cvmx_write_csr(CVMX_GMXX_TXX_BURST(index, interface), 8192);
            break;
        default:
            break;
    }

    /* Write the new misc control for PCS */
    cvmx_write_csr(CVMX_PCSX_MISCX_CTL_REG(index, interface), pcsx_miscx_ctl_reg.u64);

    /* Write the new GMX settings with the port still disabled */
    cvmx_write_csr(CVMX_GMXX_PRTX_CFG(index, interface), gmxx_prtx_cfg.u64);

    /* Read GMX CFG again to make sure the config completed */
    gmxx_prtx_cfg.u64 = cvmx_read_csr(CVMX_GMXX_PRTX_CFG(index, interface));

    /* Restore the enabled / disabled state */
    gmxx_prtx_cfg.s.en = is_enabled;
    cvmx_write_csr(CVMX_GMXX_PRTX_CFG(index, interface), gmxx_prtx_cfg.u64);

    return 0;
}


/**
 * @INTERNAL
 * Bring up the SGMII interface to be ready for packet I/O but
 * leave I/O disabled using the GMX override. This function
 * follows the bringup documented in 10.6.3 of the manual.
 *
 * @param interface Interface to bringup
 * @param num_ports Number of ports on the interface
 *
 * @return Zero on success, negative on failure
 */
static int __cvmx_helper_sgmii_hardware_init(int interface, int num_ports)
{
    int index;
    cvmx_gmxx_inf_mode_t mode;

    __cvmx_helper_setup_gmx(interface, num_ports);

#ifdef CONFIG_MAG8600
    if (num_ports == 1)
    {
        int index = 0;
        /* for MSC the SGMII port is 0 */
        int ipd_port = cvmx_helper_get_ipd_port(interface, index);
        __cvmx_helper_sgmii_hardware_init_one_time(interface, index);
        __cvmx_helper_sgmii_link_set(ipd_port, __cvmx_helper_sgmii_link_get(ipd_port));
    }
    else
    {
        for (index=0; index<num_ports; index++)
        {
            int ipd_port = cvmx_helper_get_ipd_port(interface, index);
            __cvmx_helper_sgmii_hardware_init_one_time(interface, index);
            __cvmx_helper_sgmii_link_set(ipd_port, __cvmx_helper_sgmii_link_get(ipd_port));

        }
    }
#else    
    /* Enable the interface. Set GMX0/1_INF_MODE[EN]=1 if not already set. */
    mode.u64 = cvmx_read_csr(CVMX_GMXX_INF_MODE(interface));
    if (!mode.s.en)
    {
        mode.s.en = 1;
        cvmx_write_csr(CVMX_GMXX_INF_MODE(interface), mode.u64);
    }

    for (index=0; index<num_ports; index++)
    {
        int ipd_port = cvmx_helper_get_ipd_port(interface, index);
        __cvmx_helper_sgmii_hardware_init_one_time(interface, index);
        __cvmx_helper_sgmii_link_set(ipd_port, __cvmx_helper_sgmii_link_get(ipd_port));

    }
#endif

    return 0;
}


/**
 * @INTERNAL
 * Probe a SGMII interface and determine the number of ports
 * connected to it. The SGMII interface should still be down after
 * this call.
 *
 * @param interface Interface to probe
 *
 * @return Number of ports on the interface. Zero to disable.
 */
int __cvmx_helper_sgmii_probe(int interface)
{
#ifdef CONFIG_MAG8600
    cvmx_gmxx_inf_mode_t mode;

    mode.u64 = cvmx_read_csr(CVMX_GMXX_INF_MODE(interface));
    mode.s.en = 1;
    cvmx_write_csr(CVMX_GMXX_INF_MODE(interface), mode.u64);

    return 1;
#endif

#if defined(CONFIG_ACX500_SVCS)
    if (OCTEON_IS_MODEL(OCTEON_CN70XX)) {
        enum cvmx_qlm_mode qlm_mode = cvmx_qlm_get_dlm_mode(0, interface);

        if (qlm_mode == CVMX_QLM_MODE_SGMII)
            return 1;
        else if (qlm_mode == CVMX_QLM_MODE_QSGMII)
            return 4;
    }
    return 0;
#endif

    return 4;
}


/**
 * @INTERNAL
 * Bringup and enable a SGMII interface. After this call packet
 * I/O should be fully functional. This is called with IPD
 * enabled but PKO disabled.
 *
 * @param interface Interface to bring up
 *
 * @return Zero on success, negative on failure
 */
int __cvmx_helper_sgmii_enable(int interface)
{
    int num_ports = cvmx_helper_ports_on_interface(interface);
    int index;

    __cvmx_helper_sgmii_hardware_init(interface, num_ports);
#ifdef CONFIG_MAG8600
    /* Special case here for MSC */
    if (num_ports == 1) {
        int index = 0;
        cvmx_gmxx_prtx_cfg_t gmxx_prtx_cfg;
        gmxx_prtx_cfg.u64 = cvmx_read_csr(CVMX_GMXX_PRTX_CFG(index, interface));
        gmxx_prtx_cfg.s.en = 1;
        cvmx_write_csr(CVMX_GMXX_PRTX_CFG(index, interface), gmxx_prtx_cfg.u64);
    }
    else {
        for (index=0; index<num_ports; index++) {
            cvmx_gmxx_prtx_cfg_t gmxx_prtx_cfg;
            gmxx_prtx_cfg.u64 = cvmx_read_csr(CVMX_GMXX_PRTX_CFG(index, interface));
            gmxx_prtx_cfg.s.en = 1;
            cvmx_write_csr(CVMX_GMXX_PRTX_CFG(index, interface), gmxx_prtx_cfg.u64);
        }
    }
#else

    for (index=0; index<num_ports; index++)
    {
        cvmx_gmxx_prtx_cfg_t gmxx_prtx_cfg;
        gmxx_prtx_cfg.u64 = cvmx_read_csr(CVMX_GMXX_PRTX_CFG(index, interface));
        gmxx_prtx_cfg.s.en = 1;
        cvmx_write_csr(CVMX_GMXX_PRTX_CFG(index, interface), gmxx_prtx_cfg.u64);
    }
#endif
    return 0;
}


/**
 * @INTERNAL
 * Return the link state of an IPD/PKO port as returned by
 * auto negotiation. The result of this function may not match
 * Octeon's link config if auto negotiation has changed since
 * the last call to cvmx_helper_link_set().
 *
 * @param ipd_port IPD/PKO port to query
 *
 * @return Link state
 */
cvmx_helper_link_info_t __cvmx_helper_sgmii_link_get(int ipd_port)
{
#ifdef CONFIG_JSRXNLE
    DECLARE_GLOBAL_DATA_PTR;
#endif
    cvmx_helper_link_info_t result;
    cvmx_gmxx_inf_mode_t mode;
    int interface = cvmx_helper_get_interface_num(ipd_port);
    int index = cvmx_helper_get_interface_index_num(ipd_port);

    result.u64 = 0;

    if (cvmx_sysinfo_get()->board_type == CVMX_BOARD_TYPE_SIM)
    {
        /* The simulator gives you a simulated 1Gbps full duplex link */
        result.s.link_up = 1;
        result.s.full_duplex = 1;
        result.s.speed = 1000;
        return result;
    }

    mode.u64 = cvmx_read_csr(CVMX_GMXX_INF_MODE(interface));
    if (mode.cn56xx.type)
    {
        /* 1000BASE-X */
        // FIXME
    }
    else
    {
        cvmx_pcsx_miscx_ctl_reg_t pcsx_miscx_ctl_reg;
        pcsx_miscx_ctl_reg.u64 = cvmx_read_csr(CVMX_PCSX_MISCX_CTL_REG(index, interface));
        if (pcsx_miscx_ctl_reg.s.mac_phy)
        {
            /* PHY Mode */
            cvmx_pcsx_mrx_status_reg_t pcsx_mrx_status_reg;
            cvmx_pcsx_anx_results_reg_t pcsx_anx_results_reg;

            /* Don't bother continuing if the SERTES low level link is down */
            pcsx_mrx_status_reg.u64 = cvmx_read_csr(CVMX_PCSX_MRX_STATUS_REG(index, interface));
            if (pcsx_mrx_status_reg.s.lnk_st == 0)
            {
                if (__cvmx_helper_sgmii_hardware_init_link(interface, index) != 0)
                    return result;
            }
#ifdef CONFIG_JSRXNLE
            switch (gd->board_desc.board_type) {
            CASE_ALL_SRX650_MODELS
                pcsx_mrx_status_reg.u64 = 
                  cvmx_read_csr(CVMX_PCSX_MRX_STATUS_REG(index, interface));
                result.s.speed = 1000;
                result.s.full_duplex = 1;
                result.s.link_up = pcsx_mrx_status_reg.s.lnk_st;
                return result;
            default:
                break;
            }
#endif

            /* Read the autoneg results */
            pcsx_anx_results_reg.u64 = cvmx_read_csr(CVMX_PCSX_ANX_RESULTS_REG(index, interface));
            if (pcsx_anx_results_reg.s.an_cpt)
            {
                /* Auto negotiation is complete. Set status accordingly */
                result.s.full_duplex = pcsx_anx_results_reg.s.dup;
                result.s.link_up = pcsx_anx_results_reg.s.link_ok;
                switch (pcsx_anx_results_reg.s.spd)
                {
                    case 0:
                        result.s.speed = 10;
                        break;
                    case 1:
                        result.s.speed = 100;
                        break;
                    case 2:
                        result.s.speed = 1000;
                        break;
                    default:
                        result.s.speed = 0;
                        result.s.link_up = 0;
                        break;
                }
            }
            else
            {
                /* Auto negotiation isn't complete. Return link down */
                result.s.speed = 0;
                result.s.link_up = 0;
            }
        }
        else /* MAC Mode */
        {
            result = __cvmx_helper_board_link_get(ipd_port);
        }
    }
    return result;
}


/**
 * @INTERNAL
 * Configure an IPD/PKO port for the specified link state. This
 * function does not influence auto negotiation at the PHY level.
 * The passed link state must always match the link state returned
 * by cvmx_helper_link_get(). It is normally best to use
 * cvmx_helper_link_autoconf() instead.
 *
 * @param ipd_port  IPD/PKO port to configure
 * @param link_info The new link state
 *
 * @return Zero on success, negative on failure
 */
int __cvmx_helper_sgmii_link_set(int ipd_port, cvmx_helper_link_info_t link_info)
{
    int interface = cvmx_helper_get_interface_num(ipd_port);
    int index = cvmx_helper_get_interface_index_num(ipd_port);
    __cvmx_helper_sgmii_hardware_init_link(interface, index);
    return __cvmx_helper_sgmii_hardware_init_link_speed(interface, index, link_info);
}

int cvmx_helper_sgmii_configure_loopback(int ipd_port, int enable_internal,
					   int enable_external)
{
	int interface = cvmx_helper_get_interface_num(ipd_port);
	int index = cvmx_helper_get_interface_index_num(ipd_port);
	cvmx_pcsx_mrx_control_reg_t pcsx_mrx_control_reg;
	cvmx_pcsx_miscx_ctl_reg_t pcsx_miscx_ctl_reg;

	pcsx_mrx_control_reg.u64 = cvmx_read_csr(CVMX_PCSX_MRX_CONTROL_REG(index, interface));
	pcsx_mrx_control_reg.s.loopbck1 = enable_internal;
	cvmx_write_csr(CVMX_PCSX_MRX_CONTROL_REG(index, interface),
		       pcsx_mrx_control_reg.u64);

	pcsx_miscx_ctl_reg.u64 = cvmx_read_csr(CVMX_PCSX_MISCX_CTL_REG(index, interface));
	pcsx_miscx_ctl_reg.s.loopbck2 = enable_external;
	cvmx_write_csr(CVMX_PCSX_MISCX_CTL_REG(index, interface),
		       pcsx_miscx_ctl_reg.u64);

	__cvmx_helper_sgmii_hardware_init_link(interface, index);
	return 0;
}

#ifdef CONFIG_JSRXNLE
/**
 * @fn     cvmx_get_iface_speed (int interface)
 *
 * @brief  Returns interface speed given the interface
 *
 * @param  interface The interface for which this is required
 *
 * @return int
 *
 */
int
cvmx_get_iface_speed (int interface)
{
    cvmx_gmxx_inf_mode_t iface_mode;

    iface_mode.u64 = cvmx_read_csr(CVMX_GMXX_INF_MODE(interface));

    if (octeon_is_model(OCTEON_CN63XX)) {
        return CVMX_INTERFACE_SPEED_XAUI;
    }

    if (octeon_is_model(OCTEON_CN56XX) || octeon_is_model(OCTEON_CN52XX)) {
        if (iface_mode.cn56xx.type) {
            return CVMX_INTERFACE_SPEED_XAUI;
        }

        switch (iface_mode.cn56xx.speed) {
            case 0:  return CVMX_INTERFACE_SPEED_XGMII_1250M;
            case 1:  return CVMX_INTERFACE_SPEED_XGMII_2500M;
            case 2:  return CVMX_INTERFACE_SPEED_XGMII_3125M;
            case 3:  return CVMX_INTERFACE_SPEED_XGMII_3750M;
            default: return CVMX_INTERFACE_SPEED_NONE;
        }
    } else {
        if (iface_mode.s.type) {
            if (octeon_is_model(OCTEON_CN38XX) || 
                octeon_is_model(OCTEON_CN58XX)) {
                return CVMX_INTERFACE_SPEED_XAUI;
            } else {
                return CVMX_INTERFACE_SPEED_XGMII_1250M;
            }
        } else {
            return CVMX_INTERFACE_SPEED_XGMII_1250M;
        }
    }
}
#endif

#endif /* CVMX_ENABLE_PKO_FUNCTIONS */
