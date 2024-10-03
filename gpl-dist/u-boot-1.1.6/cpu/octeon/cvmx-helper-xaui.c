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
 * Functions for XAUI initialization, configuration,
 * and monitoring.
 *
 * <hr>$Revision: 1.2 $<hr>
 */
#include "executive-config.h"
#include "cvmx-config.h"
#ifdef CVMX_ENABLE_PKO_FUNCTIONS

#include "cvmx.h"
#include "cvmx-helper.h"
#include <command.h>

/**
 * cvmx_ciu_qlm2
 *
 * Notes:
 * This register is only reset by cold reset.
 *
 */
union cvmx_ciu_qlm2
{
        uint64_t u64;
        struct cvmx_ciu_qlm2_s
        {
#if __BYTE_ORDER == __BIG_ENDIAN
        uint64_t reserved_32_63               : 32;
        uint64_t txbypass                     : 1;  /**< QLM2 transmitter bypass enable */
        uint64_t reserved_21_30               : 10;
        uint64_t txdeemph                     : 5;  /**< QLM2 transmitter bypass de-emphasis value */
        uint64_t reserved_13_15               : 3;
        uint64_t txmargin                     : 5;  /**< QLM2 transmitter bypass margin (amplitude) value */
        uint64_t reserved_4_7                 : 4;
        uint64_t lane_en                      : 4;  /**< QLM2 lane enable mask */
#else
        uint64_t lane_en                      : 4;
        uint64_t reserved_4_7                 : 4;
        uint64_t txmargin                     : 5;
        uint64_t reserved_13_15               : 3;
        uint64_t txdeemph                     : 5;
        uint64_t reserved_21_30               : 10;
        uint64_t txbypass                     : 1;
        uint64_t reserved_32_63               : 32;
#endif
        } s;
        struct cvmx_ciu_qlm2_s                cn63xx;
        struct cvmx_ciu_qlm2_cn63xxp1
        {
#if __BYTE_ORDER == __BIG_ENDIAN
        uint64_t reserved_32_63               : 32;
        uint64_t txbypass                     : 1;  /**< QLM2 transmitter bypass enable */
        uint64_t reserved_20_30               : 11;
        uint64_t txdeemph                     : 4;  /**< QLM2 transmitter bypass de-emphasis value */
        uint64_t reserved_13_15               : 3;
        uint64_t txmargin                     : 5;  /**< QLM2 transmitter bypass margin (amplitude) value */
        uint64_t reserved_4_7                 : 4;
        uint64_t lane_en                      : 4;  /**< QLM2 lane enable mask */
#else
        uint64_t lane_en                      : 4;
        uint64_t reserved_4_7                 : 4;
        uint64_t txmargin                     : 5;
        uint64_t reserved_13_15               : 3;
        uint64_t txdeemph                     : 4;
        uint64_t reserved_20_30               : 11;
        uint64_t txbypass                     : 1;
        uint64_t reserved_32_63               : 32;
#endif
        } cn63xxp1;
};
typedef union cvmx_ciu_qlm2 cvmx_ciu_qlm2_t;

#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
#define CVMX_CIU_QLM2 CVMX_CIU_QLM2_FUNC()
static inline uint64_t CVMX_CIU_QLM2_FUNC(void)
{
        if (!(OCTEON_IS_MODEL(OCTEON_CN63XX)))
                cvmx_warn("CVMX_CIU_QLM2 not supported on this chip\n");
        return CVMX_ADD_IO_SEG(0x0001070000000790ull);
}
#else
#define CVMX_CIU_QLM2 (CVMX_ADD_IO_SEG(0x0001070000000790ull))
#endif

/**
 * This macro spins on a field waiting for it to reach a value. It
 * is common in code to need to wait for a specific field in a CSR
 * to match a specific value. Conceptually this macro expands to:
 *
 * 1) read csr at "address" with a csr typedef of "type"
 * 2) Check if ("type".s."field" "op" "value")
 * 3) If #2 isn't true loop to #1 unless too much time has passed.
 */
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
        uint64_t done = cvmx_get_cycle() + (uint64_t)timeout_usec *     \
                           cvmx_sysinfo_get()->cpu_clock_hz / 1000000;  \
        type c;                                                         \
        while (1)                                                       \
        {                                                               \
            c.u64 = cvmx_read_csr(address);                             \
            if ((c.s.field) op (value)) {                               \
                result = 0;                                             \
                break;                                                  \
            } else if (cvmx_get_cycle() > done) {                       \
                result = -1;                                            \
                break;                                                  \
            } else                                                      \
                cvmx_wait(100);                                         \
        }                                                               \
    } while (0);                                                        \
    result;})




/**
 * @INTERNAL
 * Probe a XAUI interface and determine the number of ports
 * connected to it. The XAUI interface should still be down
 * after this call.
 *
 * @param interface Interface to probe
 *
 * @return Number of ports on the interface. Zero to disable.
 */
int __cvmx_helper_xaui_probe(int interface)
{
    int i;
    cvmx_gmxx_hg2_control_t gmx_hg2_control;
    cvmx_gmxx_inf_mode_t mode;

    /* CN63XX Pass 1.0 errata G-14395 requires the QLM De-emphasis be programmed */
    if (OCTEON_IS_MODEL(OCTEON_CN63XX_PASS1_0))
    {
        cvmx_ciu_qlm2_t ciu_qlm;
        ciu_qlm.u64 = cvmx_read_csr(CVMX_CIU_QLM2);
        ciu_qlm.s.txbypass = 1;
        ciu_qlm.s.txdeemph = 0x5;
        ciu_qlm.s.txmargin = 0x1a;
        cvmx_write_csr(CVMX_CIU_QLM2, ciu_qlm.u64);
    }

    /* Due to errata GMX-700 on CN56XXp1.x and CN52XXp1.x, the interface
        needs to be enabled before IPD otherwise per port backpressure
        may not work properly */
    mode.u64 = cvmx_read_csr(CVMX_GMXX_INF_MODE(interface));
    mode.s.en = 1;
    cvmx_write_csr(CVMX_GMXX_INF_MODE(interface), mode.u64);

    __cvmx_helper_setup_gmx(interface, 1);

    /* Setup PKO to support 16 ports for HiGig2 virtual ports. We're pointing
        all of the PKO packet ports for this interface to the XAUI. This allows
        us to use HiGig2 backpressure per port */
    for (i=0; i<16; i++)
    {
        cvmx_pko_mem_port_ptrs_t pko_mem_port_ptrs;
        pko_mem_port_ptrs.u64 = 0;
        /* We set each PKO port to have equal priority in a round robin
            fashion */
        pko_mem_port_ptrs.s.static_p = 0;
        pko_mem_port_ptrs.s.qos_mask = 0xff;
        /* All PKO ports map to the same XAUI hardware port */
        pko_mem_port_ptrs.s.eid = interface*4;
        pko_mem_port_ptrs.s.pid = interface*16 + i;
        cvmx_write_csr(CVMX_PKO_MEM_PORT_PTRS, pko_mem_port_ptrs.u64);
    }

    /* If HiGig2 is enabled return 16 ports, otherwise return 1 port */
    gmx_hg2_control.u64 = cvmx_read_csr(CVMX_GMXX_HG2_CONTROL(interface));
    if (gmx_hg2_control.s.hg2tx_en)
        return 16;
    else
        return 1;
}

/**
 * @INTERNAL
 * Bringup and enable a XAUI interface. After this call packet
 * I/O should be fully functional. This is called with IPD
 * enabled but PKO disabled.
 *
 * @param interface Interface to bring up
 *
 * @return Zero on success, negative on failure
 */
int __cvmx_helper_xaui_enable(int interface)
{
    cvmx_gmxx_prtx_cfg_t          gmx_cfg;
    cvmx_pcsxx_control1_reg_t     xauiCtl;
    cvmx_pcsxx_misc_ctl_reg_t     xauiMiscCtl;
    cvmx_gmxx_tx_xaui_ctl_t       gmxXauiTxCtl;
    cvmx_helper_link_info_t       link_info;

    /* (1) Interface has already been enabled. */

    /* (2) Disable GMX. */
    xauiMiscCtl.u64 = cvmx_read_csr(CVMX_PCSXX_MISC_CTL_REG(interface));
    xauiMiscCtl.s.gmxeno = 1;
    cvmx_write_csr (CVMX_PCSXX_MISC_CTL_REG(interface), xauiMiscCtl.u64);

    /* (3) Disable GMX and PCSX interrupts. */
    cvmx_write_csr(CVMX_GMXX_RXX_INT_EN(0,interface), 0x0);
    cvmx_write_csr(CVMX_GMXX_TX_INT_EN(interface), 0x0);
    cvmx_write_csr(CVMX_PCSXX_INT_EN_REG(interface), 0x0);

    /* (4) Bring up the PCSX and GMX reconciliation layer. */
    /* (4)a Set polarity and lane swapping. */
    /* (4)b */
    gmxXauiTxCtl.u64 = cvmx_read_csr (CVMX_GMXX_TX_XAUI_CTL(interface));
    gmxXauiTxCtl.s.dic_en = 1; /* Enable better IFG packing and improves performance */
    gmxXauiTxCtl.s.uni_en = 0;
    cvmx_write_csr (CVMX_GMXX_TX_XAUI_CTL(interface), gmxXauiTxCtl.u64);

    /* (4)c Aply reset sequence */
    xauiCtl.u64 = cvmx_read_csr (CVMX_PCSXX_CONTROL1_REG(interface));
    if (OCTEON_IS_MODEL(OCTEON_CN63XX)) {
        /*
         * This is a hardware workaround for QLM CDR reset Errata on
         * Octeon 63XX processor.
         */
        xauiCtl.s.lo_pwr = 1;
        xauiCtl.s.reset  = 1;
    } else {
        xauiCtl.s.lo_pwr = 0;
        xauiCtl.s.reset  = 1;
    }
    
    cvmx_write_csr (CVMX_PCSXX_CONTROL1_REG(interface), xauiCtl.u64);

    if (OCTEON_IS_MODEL(OCTEON_CN63XX)) {
        udelay(2000);
        xauiCtl.s.lo_pwr = 0;
        xauiCtl.s.reset  = 0;
        cvmx_write_csr (CVMX_PCSXX_CONTROL1_REG(interface), xauiCtl.u64);
    }

    /* Wait for PCS to come out of reset */
    if (CVMX_WAIT_FOR_FIELD64(CVMX_PCSXX_CONTROL1_REG(interface), cvmx_pcsxx_control1_reg_t, reset, ==, 0, 10000))
    {
        printf("Error Wait for PCS to come out of reset\n");
        return -1;
    }
    /* Wait for PCS to be aligned */
    if (CVMX_WAIT_FOR_FIELD64(CVMX_PCSXX_10GBX_STATUS_REG(interface), cvmx_pcsxx_10gbx_status_reg_t, alignd, ==, 1, 10000))
    {
        printf("Error Wait for PCS to be aligned\n");
        return -1;
    }
    /* Wait for RX to be ready */
    if (CVMX_WAIT_FOR_FIELD64(CVMX_GMXX_RX_XAUI_CTL(interface), cvmx_gmxx_rx_xaui_ctl_t, status, ==, 0, 10000))
    {
        printf("Error Wait for RX to be ready \n");
        return -1;
    }

    /* (6) Configure GMX */
    gmx_cfg.u64 = cvmx_read_csr(CVMX_GMXX_PRTX_CFG(0, interface));
    gmx_cfg.s.en = 0;
    cvmx_write_csr(CVMX_GMXX_PRTX_CFG(0, interface), gmx_cfg.u64);

    /* Wait for GMX RX to be idle */
    if (CVMX_WAIT_FOR_FIELD64(CVMX_GMXX_PRTX_CFG(0, interface), cvmx_gmxx_prtx_cfg_t, rx_idle, ==, 1, 10000))
    {
        printf("Error Wait for GMX RX to be idle\n");
        return -1;
    }
    /* Wait for GMX TX to be idle */
    if (CVMX_WAIT_FOR_FIELD64(CVMX_GMXX_PRTX_CFG(0, interface), cvmx_gmxx_prtx_cfg_t, tx_idle, ==, 1, 10000))
    {
        printf("Error Wait for GMX TX to be idle\n");
        return -1;
    }
    /* GMX configure */
    gmx_cfg.u64 = cvmx_read_csr(CVMX_GMXX_PRTX_CFG(0, interface));
    gmx_cfg.s.speed = 1;
    gmx_cfg.s.speed_msb = 0;
    gmx_cfg.s.slottime = 1;
    cvmx_write_csr(CVMX_GMXX_TX_PRTS(interface), 1);
    cvmx_write_csr(CVMX_GMXX_TXX_SLOT(0, interface), 512);
    cvmx_write_csr(CVMX_GMXX_TXX_BURST(0, interface), 8192);
    cvmx_write_csr(CVMX_GMXX_PRTX_CFG(0, interface), gmx_cfg.u64);

    /* Wait for receive link */
    if (CVMX_WAIT_FOR_FIELD64(CVMX_PCSXX_STATUS1_REG(interface), cvmx_pcsxx_status1_reg_t, rcv_lnk, ==, 1, 10000))
    {
        printf("Error Wait for  rcv_lnk\n");
        return -1;
    }
    if (CVMX_WAIT_FOR_FIELD64(CVMX_PCSXX_STATUS2_REG(interface), cvmx_pcsxx_status2_reg_t, xmtflt, ==, 0, 10000))
    {
        printf("Error Wait for  xmtflt \n");
        return -1;
    }
    if (CVMX_WAIT_FOR_FIELD64(CVMX_PCSXX_STATUS2_REG(interface), cvmx_pcsxx_status2_reg_t, rcvflt, ==, 0, 10000))
    {
        printf("Error Wait for  rcvflt\n");
        return -1;
    }
    /* (8) Enable packet reception */
    xauiMiscCtl.s.gmxeno = 0;
    cvmx_write_csr (CVMX_PCSXX_MISC_CTL_REG(interface), xauiMiscCtl.u64);

    gmx_cfg.u64 = cvmx_read_csr(CVMX_GMXX_PRTX_CFG(0, interface));
    gmx_cfg.s.en = 1;
    cvmx_write_csr(CVMX_GMXX_PRTX_CFG(0, interface), gmx_cfg.u64);

    link_info = cvmx_helper_link_autoconf(cvmx_helper_get_ipd_port(interface, 0));
    if (!link_info.s.link_up)
    {
        printf("Error Link not up\n");
        return -1;
    }

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
cvmx_helper_link_info_t __cvmx_helper_xaui_link_get(int ipd_port)
{
    int interface = cvmx_helper_get_interface_num(ipd_port);
    cvmx_gmxx_tx_xaui_ctl_t gmxx_tx_xaui_ctl;
    cvmx_gmxx_rx_xaui_ctl_t gmxx_rx_xaui_ctl;
    cvmx_pcsxx_status1_reg_t pcsxx_status1_reg;
    cvmx_helper_link_info_t result;
    static int link_get_count = 0;


    if (link_get_count == 0) {
        cvmx_dprintf("!!!!!!!!!!! ipd_port port from link get is %d \n",ipd_port);
        link_get_count++;
    }
    gmxx_tx_xaui_ctl.u64 = cvmx_read_csr(CVMX_GMXX_TX_XAUI_CTL(interface));
    gmxx_rx_xaui_ctl.u64 = cvmx_read_csr(CVMX_GMXX_RX_XAUI_CTL(interface));
    pcsxx_status1_reg.u64 = cvmx_read_csr(CVMX_PCSXX_STATUS1_REG(interface));
    result.u64 = 0;

    /* Only return a link if both RX and TX are happy */
    if ((gmxx_tx_xaui_ctl.s.ls == 0) && (gmxx_rx_xaui_ctl.s.status == 0) &&
        (pcsxx_status1_reg.s.rcv_lnk == 1))
    {
        result.s.link_up = 1;
        result.s.full_duplex = 1;
        result.s.speed = 10000;
    }
    else
    {
        /* Disable GMX and PCSX interrupts. */
        cvmx_write_csr (CVMX_GMXX_RXX_INT_EN(0,interface), 0x0);
        cvmx_write_csr (CVMX_GMXX_TX_INT_EN(interface), 0x0);
        cvmx_write_csr (CVMX_PCSXX_INT_EN_REG(interface), 0x0);
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
int __cvmx_helper_xaui_link_set(int ipd_port, cvmx_helper_link_info_t link_info)
{
    int interface = cvmx_helper_get_interface_num(ipd_port);
    cvmx_gmxx_tx_xaui_ctl_t gmxx_tx_xaui_ctl;
    cvmx_gmxx_rx_xaui_ctl_t gmxx_rx_xaui_ctl;
    static int link_set_count = 0;

    if (link_set_count == 0) {
        link_set_count++;
    }
    gmxx_tx_xaui_ctl.u64 = cvmx_read_csr(CVMX_GMXX_TX_XAUI_CTL(interface));
    gmxx_rx_xaui_ctl.u64 = cvmx_read_csr(CVMX_GMXX_RX_XAUI_CTL(interface));

    /* If the link shouldn't be up, then just return */
    if (!link_info.s.link_up)
        return 0;

    /* Do nothing if both RX and TX are happy */
    if ((gmxx_tx_xaui_ctl.s.ls == 0) && (gmxx_rx_xaui_ctl.s.status == 0))
        return 0;

    /* Bring the link up */
    return __cvmx_helper_xaui_enable(interface);

}

extern int __cvmx_helper_xaui_configure_loopback(int ipd_port, int enable_internal, int enable_external)
{
    int interface = cvmx_helper_get_interface_num(ipd_port);
    cvmx_pcsxx_control1_reg_t pcsxx_control1_reg;
    cvmx_gmxx_xaui_ext_loopback_t gmxx_xaui_ext_loopback;

    /* Set the internal loop */
    pcsxx_control1_reg.u64 = cvmx_read_csr(CVMX_PCSXX_CONTROL1_REG(interface));
    pcsxx_control1_reg.s.loopbck1 = enable_internal;
    cvmx_write_csr(CVMX_PCSXX_CONTROL1_REG(interface), pcsxx_control1_reg.u64);

    /* Set the external loop */
    gmxx_xaui_ext_loopback.u64 = cvmx_read_csr(CVMX_GMXX_XAUI_EXT_LOOPBACK(interface));
    gmxx_xaui_ext_loopback.s.en = enable_external;
    cvmx_write_csr(CVMX_GMXX_XAUI_EXT_LOOPBACK(interface), gmxx_xaui_ext_loopback.u64);

    /* Take the link through a reset */
    return __cvmx_helper_xaui_enable(interface);
}

#endif /* CVMX_ENABLE_PKO_FUNCTIONS */

