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
 * Interface to PCIe as a host(RC) or target(EP)
 *
 * <hr>$Revision: 1.5.308.1 $<hr>
 */
#include <common.h>

#if defined(CONFIG_PCI)
#include "cvmx.h"
#if !defined(CONFIG_JSRXNLE) && !defined(CONFIG_MAG8600)
#include "cvmx-csr-db.h"
#endif
#include "cvmx-pcie.h"
#include "cvmx-sysinfo.h"

#if defined(CONFIG_JSRXNLE) || defined(CONFIG_MAG8600)

#include "common.h"
#include "command.h"
#ifdef CONFIG_MAG8600
#include "configs/mag8600.h"
#endif
#ifdef CONFIG_JSRXNLE
#include "configs/jsrxnle.h"
#endif
#include <watchdog_cpu.h>
#include <ide.h>
#include <ata.h>

#include <cvmx-pciercx-defs.h>
#include <cvmx-pemx-defs.h>
#include <cvmx-pexp-defs.h>
#include <cvmx-dpi-defs.h>
#include <cvmx-sli-defs.h>

#include <octeon_hal.h>
#endif

extern block_dev_desc_t ide_dev_desc[CFG_IDE_MAXDEVICE];
static int reboots = 0;
static int except;
static int do_bus(int pcie_port, int bus);
static int si3132_reset_done;

static inline uint64_t cvmx_key_read(uint64_t address)
{
    cvmx_addr_t ptr;

    ptr.u64 = 0;
    ptr.sio.mem_region  = CVMX_IO_SEG;
    ptr.sio.is_io       = 1;
    ptr.sio.did         = CVMX_OCT_DID_KEY_RW;
    ptr.sio.offset      = address;

    return cvmx_read_csr(ptr.u64);
}


 /**
 * Write to KEY memory
 *
 * @param address Address (byte) in key memory to write
 *                0 <= address < CVMX_KEY_MEM_SIZE
 * @param value   Value to write to key memory
 */
static inline void cvmx_key_write(uint64_t address, uint64_t value)
{
    cvmx_addr_t ptr;

    ptr.u64 = 0;
    ptr.sio.mem_region  = CVMX_IO_SEG;
    ptr.sio.is_io       = 1;
    ptr.sio.did         = CVMX_OCT_DID_KEY_RW;
    ptr.sio.offset      = address;

    cvmx_write_io(ptr.u64, value);
}




static inline uint16_t swap16(uint16_t x)
{
    return ((uint16_t)((((uint16_t)(x) & (uint16_t)0x00ffU) << 8) |
                       (((uint16_t)(x) & (uint16_t)0xff00U) >> 8) ));
}


static inline uint32_t swap32(uint32_t x)
{
    return ((uint32_t)((((uint32_t)(x) & (uint32_t)0x000000ffUL) << 24) |
                       (((uint32_t)(x) & (uint32_t)0x0000ff00UL) <<  8) |
                       (((uint32_t)(x) & (uint32_t)0x00ff0000UL) >>  8) |
                       (((uint32_t)(x) & (uint32_t)0xff000000UL) >> 24) ));
}


/**
 * Return the Core virtual base address for PCIe IO access. IOs are
 * read/written as an offset from this address.
 *
 * @param pcie_port PCIe port the IO is for
 *
 * @return 64bit Octeon IO base address for read/write
 */
uint64_t cvmx_pcie_get_io_base_address(int pcie_port)
{
    cvmx_pcie_address_t pcie_addr;
    pcie_addr.u64 = 0;
    pcie_addr.io.upper = 0;
    pcie_addr.io.io = 1;
    pcie_addr.io.did = 3;
    pcie_addr.io.subdid = 2;
    pcie_addr.io.es = 1;
    pcie_addr.io.port = pcie_port;
    return pcie_addr.u64;
}


/**
 * Size of the IO address region returned at address
 * cvmx_pcie_get_io_base_address()
 *
 * @param pcie_port PCIe port the IO is for
 *
 * @return Size of the IO window
 */
uint64_t cvmx_pcie_get_io_size(int pcie_port)
{
    return 1ull<<32;
}


/**
 * Return the Core virtual base address for PCIe MEM access. Memory is
 * read/written as an offset from this address.
 *
 * @param pcie_port PCIe port the IO is for
 *
 * @return 64bit Octeon IO base address for read/write
 */
uint64_t cvmx_pcie_get_mem_base_address(int pcie_port)
{
    cvmx_pcie_address_t pcie_addr;
    pcie_addr.u64 = 0;
    pcie_addr.mem.upper = 2;
    pcie_addr.mem.io = 1;
    pcie_addr.mem.did = 3;
    pcie_addr.mem.subdid = 3 + pcie_port;
    return pcie_addr.u64;
}


/**
 * Size of the Mem address region returned at address
 * cvmx_pcie_get_mem_base_address()
 *
 * @param pcie_port PCIe port the IO is for
 *
 * @return Size of the Mem window
 */
uint64_t cvmx_pcie_get_mem_size(int pcie_port)
{
    return 1ull<<36;
}

/**
 * @INTERNAL
 * Initialize the RC config space CSRs
 *
 * @param pcie_port PCIe port to initialize
 */
static void __cvmx_pcie_rc_initialize_config_space(int pcie_port)
{
    DECLARE_GLOBAL_DATA_PTR;
    /* Max Payload Size (PCIE*_CFG030[MPS]) */
    /* Max Read Request Size (PCIE*_CFG030[MRRS]) */
    /* Relaxed-order, no-snoop enables (PCIE*_CFG030[RO_EN,NS_EN] */
    /* Error Message Enables (PCIE*_CFG030[CE_EN,NFE_EN,FE_EN,UR_EN]) */
    {
        cvmx_pciercx_cfg030_t pciercx_cfg030;
        pciercx_cfg030.u32 = cvmx_pcie_cfgx_read(pcie_port, CVMX_PCIERCX_CFG030(pcie_port));
        if (IS_PCIE_GEN2_MODEL(gd->board_desc.board_type)) {
            pciercx_cfg030.s.mps = MPS_CN6XXX;
            pciercx_cfg030.s.mrrs = MRRS_CN6XXX;
        } else {
            pciercx_cfg030.s.mps = 1;
            pciercx_cfg030.s.mrrs = 5;
        }
        pciercx_cfg030.s.ro_en = 1; /* Enable relaxed order processing. This will allow devices to affect read response ordering */
        pciercx_cfg030.s.ns_en = 1; /* Enable no snoop processing. Not used by Octeon */
        pciercx_cfg030.s.ce_en = 1; /* Correctable error reporting enable. */
        pciercx_cfg030.s.nfe_en = 1; /* Non-fatal error reporting enable. */
        pciercx_cfg030.s.fe_en = 1; /* Fatal error reporting enable. */
        pciercx_cfg030.s.ur_en = 1; /* Unsupported request reporting enable. */
        cvmx_pcie_cfgx_write(pcie_port, CVMX_PCIERCX_CFG030(pcie_port), pciercx_cfg030.u32);
    }

    if (IS_PCIE_GEN2_MODEL(gd->board_desc.board_type)) {
           /* Max Payload Size (DPI_SLI_PRTX_CFG[MPS]) must match PCIE*_CFG030[MPS] */
        /* Max Read Request Size (DPI_SLI_PRTX_CFG[MRRS]) must not exceed PCIE*_CFG030[MRRS] */
        cvmx_dpi_sli_prtx_cfg_t prt_cfg;
        cvmx_sli_s2m_portx_ctl_t sli_s2m_portx_ctl;
        prt_cfg.u64 = cvmx_read_csr(CVMX_DPI_SLI_PRTX_CFG(pcie_port));
        prt_cfg.s.mps = MPS_CN6XXX;
        prt_cfg.s.mrrs = MRRS_CN6XXX;
        cvmx_write_csr(CVMX_DPI_SLI_PRTX_CFG(pcie_port), prt_cfg.u64);

        sli_s2m_portx_ctl.u64 = cvmx_read_csr(CVMX_PEXP_SLI_S2M_PORTX_CTL(pcie_port));
        sli_s2m_portx_ctl.s.mrrs = MRRS_CN6XXX;
        cvmx_write_csr(CVMX_PEXP_SLI_S2M_PORTX_CTL(pcie_port), sli_s2m_portx_ctl.u64);
    } else {
        /* Max Payload Size (NPEI_CTL_STATUS2[MPS]) must match PCIE*_CFG030[MPS] */
        /* Max Read Request Size (NPEI_CTL_STATUS2[MRRS]) must not exceed PCIE*_CFG030[MRRS] */
        cvmx_npei_ctl_status2_t npei_ctl_status2;
        npei_ctl_status2.u64 = cvmx_read_csr(CVMX_PEXP_NPEI_CTL_STATUS2);
        npei_ctl_status2.s.mps = 1; /* Max payload size = 128 bytes for best Octeon DMA performance */
        npei_ctl_status2.s.mrrs = 5; /* Max read request size = 128 bytes for best Octeon DMA performance */
        cvmx_write_csr(CVMX_PEXP_NPEI_CTL_STATUS2, npei_ctl_status2.u64);
    }

    /* ECRC Generation (PCIE*_CFG070[GE,CE]) */
    {
        cvmx_pciercx_cfg070_t pciercx_cfg070;
        pciercx_cfg070.u32 = cvmx_pcie_cfgx_read(pcie_port, CVMX_PCIERCX_CFG070(pcie_port));
        pciercx_cfg070.s.ge = 1; /* ECRC generation enable. */
        pciercx_cfg070.s.ce = 1; /* ECRC check enable. */
        cvmx_pcie_cfgx_write(pcie_port, CVMX_PCIERCX_CFG070(pcie_port), pciercx_cfg070.u32);
    }

    /* Access Enables (PCIE*_CFG001[MSAE,ME]) */
        /* ME and MSAE should always be set. */
    /* Interrupt Disable (PCIE*_CFG001[I_DIS]) */
    /* System Error Message Enable (PCIE*_CFG001[SEE]) */
    {
        cvmx_pciercx_cfg001_t pciercx_cfg001;
        pciercx_cfg001.u32 = cvmx_pcie_cfgx_read(pcie_port, CVMX_PCIERCX_CFG001(pcie_port));
        pciercx_cfg001.s.msae = 1; /* Memory space enable. */
        pciercx_cfg001.s.me = 1; /* Bus master enable. */
        pciercx_cfg001.s.i_dis = 1; /* INTx assertion disable. */
        pciercx_cfg001.s.see = 1; /* SERR# enable */
        cvmx_pcie_cfgx_write(pcie_port, CVMX_PCIERCX_CFG001(pcie_port), pciercx_cfg001.u32);
    }


    /* Advanced Error Recovery Message Enables */
    /* (PCIE*_CFG066,PCIE*_CFG067,PCIE*_CFG069) */
    cvmx_pcie_cfgx_write(pcie_port, CVMX_PCIERCX_CFG066(pcie_port), 0);
    /* Use CVMX_PCIERCX_CFG067 hardware default */
    cvmx_pcie_cfgx_write(pcie_port, CVMX_PCIERCX_CFG069(pcie_port), 0);


    /* Active State Power Management (PCIE*_CFG032[ASLPC]) */
    {
        cvmx_pciercx_cfg032_t pciercx_cfg032;
        pciercx_cfg032.u32 = cvmx_pcie_cfgx_read(pcie_port, CVMX_PCIERCX_CFG032(pcie_port));
        pciercx_cfg032.s.aslpc = 0; /* Active state Link PM control. */
        cvmx_pcie_cfgx_write(pcie_port, CVMX_PCIERCX_CFG032(pcie_port), pciercx_cfg032.u32);
    }
     if (IS_PCIE_GEN2_MODEL(gd->board_desc.board_type)) {
    /* Link Width Mode (PCIERCn_CFG452[LME]) - Set during cvmx_pcie_rc_initialize_link() */
    /* Primary Bus Number (PCIERCn_CFG006[PBNUM]) */
         /* We set the primary bus number to 1 so IDT bridges are happy. They don't like zero */
        cvmx_pciercx_cfg006_t pciercx_cfg006;
        pciercx_cfg006.u32 = 0;
        pciercx_cfg006.s.pbnum = 1;
        pciercx_cfg006.s.sbnum = 1;
        pciercx_cfg006.s.subbnum = 1;
        cvmx_pcie_cfgx_write(pcie_port, CVMX_PCIERCX_CFG006(pcie_port), pciercx_cfg006.u32);
    }

    /* Memory-mapped I/O BAR (PCIERCn_CFG008) */
    /* Most applications should disable the memory-mapped I/O BAR by */
    /* setting PCIERCn_CFG008[ML_ADDR] < PCIERCn_CFG008[MB_ADDR] */
    {
        cvmx_pciercx_cfg008_t pciercx_cfg008;
        pciercx_cfg008.u32 = 0;
        switch (gd->board_desc.board_type)
        {
#ifdef CONFIG_JSRXNLE
            CASE_ALL_SRX240_MODELS
                pciercx_cfg008.s.mb_addr = 0;
                pciercx_cfg008.s.ml_addr = 0x100;
                break;
#endif 
                default:
                pciercx_cfg008.s.mb_addr = 0x100;
                pciercx_cfg008.s.ml_addr = 0;
	        break;
        }
        cvmx_pcie_cfgx_write(pcie_port, CVMX_PCIERCX_CFG008(pcie_port), pciercx_cfg008.u32);
    }

    /* Prefetchable BAR (PCIERCn_CFG009,PCIERCn_CFG010,PCIERCn_CFG011) */
    /* Most applications should disable the prefetchable BAR by setting */
    /* PCIERCn_CFG011[UMEM_LIMIT],PCIERCn_CFG009[LMEM_LIMIT] < */
    /* PCIERCn_CFG010[UMEM_BASE],PCIERCn_CFG009[LMEM_BASE] */
    {
        cvmx_pciercx_cfg009_t pciercx_cfg009;
        cvmx_pciercx_cfg010_t pciercx_cfg010;
        cvmx_pciercx_cfg011_t pciercx_cfg011;
        pciercx_cfg009.u32 = cvmx_pcie_cfgx_read(pcie_port, CVMX_PCIERCX_CFG009(pcie_port));
        pciercx_cfg010.u32 = cvmx_pcie_cfgx_read(pcie_port, CVMX_PCIERCX_CFG010(pcie_port));
        pciercx_cfg011.u32 = cvmx_pcie_cfgx_read(pcie_port, CVMX_PCIERCX_CFG011(pcie_port));
        pciercx_cfg009.s.lmem_base = 0x100;
        pciercx_cfg009.s.lmem_limit = 0;
        pciercx_cfg010.s.umem_base = 0x100;
        pciercx_cfg011.s.umem_limit = 0;
        cvmx_pcie_cfgx_write(pcie_port, CVMX_PCIERCX_CFG009(pcie_port), pciercx_cfg009.u32);
        cvmx_pcie_cfgx_write(pcie_port, CVMX_PCIERCX_CFG010(pcie_port), pciercx_cfg010.u32);
        cvmx_pcie_cfgx_write(pcie_port, CVMX_PCIERCX_CFG011(pcie_port), pciercx_cfg011.u32);
    }

    /* System Error Interrupt Enables (PCIERCn_CFG035[SECEE,SEFEE,SENFEE]) */
    /* PME Interrupt Enables (PCIERCn_CFG035[PMEIE]) */
    {
        cvmx_pciercx_cfg035_t pciercx_cfg035;
        pciercx_cfg035.u32 = cvmx_pcie_cfgx_read(pcie_port, CVMX_PCIERCX_CFG035(pcie_port));
        pciercx_cfg035.s.secee = 1; /* System error on correctable error enable. */
        pciercx_cfg035.s.sefee = 1; /* System error on fatal error enable. */
        pciercx_cfg035.s.senfee = 1; /* System error on non-fatal error enable. */
        pciercx_cfg035.s.pmeie = 1; /* PME interrupt enable. */
        cvmx_pcie_cfgx_write(pcie_port, CVMX_PCIERCX_CFG035(pcie_port), pciercx_cfg035.u32);
    }

    /* Advanced Error Recovery Interrupt Enables */
    /* (PCIERCn_CFG075[CERE,NFERE,FERE]) */
    {
        cvmx_pciercx_cfg075_t pciercx_cfg075;
        pciercx_cfg075.u32 = cvmx_pcie_cfgx_read(pcie_port, CVMX_PCIERCX_CFG075(pcie_port));
        pciercx_cfg075.s.cere = 1; /* Correctable error reporting enable. */
        pciercx_cfg075.s.nfere = 1; /* Non-fatal error reporting enable. */
        pciercx_cfg075.s.fere = 1; /* Fatal error reporting enable. */
        cvmx_pcie_cfgx_write(pcie_port, CVMX_PCIERCX_CFG075(pcie_port), pciercx_cfg075.u32);
    }

    /* HP Interrupt Enables (PCIERCn_CFG034[HPINT_EN], */
    /* PCIERCn_CFG034[DLLS_EN,CCINT_EN]) */
    {
        cvmx_pciercx_cfg034_t pciercx_cfg034;
        pciercx_cfg034.u32 = cvmx_pcie_cfgx_read(pcie_port, CVMX_PCIERCX_CFG034(pcie_port));
        pciercx_cfg034.s.hpint_en = 1; /* Hot-plug interrupt enable. */
        pciercx_cfg034.s.dlls_en = 1; /* Data Link Layer state changed enable */
        pciercx_cfg034.s.ccint_en = 1; /* Command completed interrupt enable. */
        cvmx_pcie_cfgx_write(pcie_port, CVMX_PCIERCX_CFG034(pcie_port), pciercx_cfg034.u32);
    }
}

static uint32_t shift_in_data(int bits, uint32_t data)
{
    cvmx_ciu_qlm_jtgd_t jtgd;

    jtgd.u64 = 0;
    jtgd.s.shift = 1;
    jtgd.s.shft_cnt = bits-1;
    jtgd.s.shft_reg = data;

    if (OCTEON_IS_MODEL(OCTEON_CN52XX))
        jtgd.s.select = 1 << 0 /*qlm*/;

    cvmx_write_csr(CVMX_CIU_QLM_JTGD, jtgd.u64);
    do
    {
        jtgd.u64 = cvmx_read_csr(CVMX_CIU_QLM_JTGD);
    } while (jtgd.s.shift);
    return jtgd.s.shft_reg;
}

static void __cvmx_pcie_force_link(void)
{
    DECLARE_GLOBAL_DATA_PTR;
    cvmx_ciu_qlm_jtgc_t jtgc;
    cvmx_ciu_qlm_jtgd_t jtgd;
    int clock_div = 0;
    int divisor;

    switch (gd->board_desc.board_type)
    {
#ifdef CONFIG_JSRXNLE
         CASE_ALL_SRX240_MODELS
		  divisor = (600 * 1000000) / (25 * 1000000);
	         break;
#endif
         default:
		  divisor = cvmx_sysinfo_get()->cpu_clock_hz / (25 * 1000000);
	         break;
    }

    divisor = (divisor-1)>>2;
    while (divisor)
    {
        clock_div++;
        divisor>>=1;
    }

    jtgc.u64 = 0;
    jtgc.s.clk_div = clock_div; /* Clock divider for QLM JTAG operations.  eclk is divided by 2^(CLK_DIV + 2) */
    jtgc.s.mux_sel = 0;         /* Selects which QLM JTAG shift out is shifted into the QLM JTAG shift register: CIU_QLM_JTGD[SHFT_REG] */
    jtgc.s.bypass = 0x1;        /* Selects which QLM JTAG shift chains are bypassed by the QLM JTAG data register (CIU_QLM_JTGD) (one bit per QLM) */
    cvmx_write_csr(CVMX_CIU_QLM_JTGC, jtgc.u64);
    cvmx_read_csr(CVMX_CIU_QLM_JTGC);

    if (OCTEON_IS_MODEL(OCTEON_CN56XX))
    {
        shift_in_data(32, 0x04000000);
        shift_in_data(32, 0x00000000);
        shift_in_data(32, 0x81800000);
        shift_in_data(32, 0x00000000);
        shift_in_data(32, 0x00000000);
        shift_in_data(32, 0x00000000);
        shift_in_data(32, 0x00000000);
        shift_in_data(32, 0x00000000);
        shift_in_data(32, 0x00000000);
        shift_in_data(32, 0x00000040);
        shift_in_data(32, 0x00000000);
        shift_in_data(32, 0x00000818);
        shift_in_data(32, 0x00000000);
        shift_in_data(32, 0x00000000);
        shift_in_data(32, 0x00000000);
        shift_in_data(32, 0x00000000);
        shift_in_data(32, 0x00000000);
        shift_in_data(32, 0x00040000);
        shift_in_data(32, 0x00000000);
        shift_in_data(32, 0x00818000);
        shift_in_data(32, 0x00000000);
        shift_in_data(32, 0x00000000);
        shift_in_data(32, 0x00014000);
        shift_in_data(32, 0x00000000);
        shift_in_data(32, 0x00000000);
        shift_in_data(32, 0x40000000);
        shift_in_data(32, 0x00000000);
        shift_in_data(32, 0x18000000);
        shift_in_data(32, 0x00000008);
        shift_in_data(32, 0x00000000);
        shift_in_data(32, 0x00000000);
        shift_in_data(32, 0x00000000);
        shift_in_data(32, 0x00000000);
        shift_in_data(16, 0x00000000);
    }
    else if (OCTEON_IS_MODEL(OCTEON_CN52XX))
    {
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x2003);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x3000);
        shift_in_data(16, 0x0200);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0300);
        shift_in_data(16, 0x0020);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0030);
        shift_in_data(16, 0x0002);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
        shift_in_data(16, 0x0000);
    }

    /* Update the new data */
    jtgd.u64 = 0;
    jtgd.s.update = 1;
    if (OCTEON_IS_MODEL(OCTEON_CN52XX))
        jtgd.s.select = 1 << 0 /*qlm*/;

    cvmx_write_csr(CVMX_CIU_QLM_JTGD, jtgd.u64);
    do
    {
        jtgd.u64 = cvmx_read_csr(CVMX_CIU_QLM_JTGD);
    } while (jtgd.s.update);
}

/**
 * @INTERNAL
 * Initialize a host mode PCIe link. This function takes a PCIe
 * port from reset to a link up state. Software can then begin
 * configuring the rest of the link.
 *
 * @param pcie_port PCIe port to initialize
 *
 * @return Zero on success
 */
static int __cvmx_pcie_rc_initialize_link(int pcie_port)
{
    DECLARE_GLOBAL_DATA_PTR;
    uint64_t start_cycle;
    cvmx_pescx_ctl_status_t pescx_ctl_status;
    cvmx_pciercx_cfg452_t pciercx_cfg452;
    cvmx_pciercx_cfg032_t pciercx_cfg032;
    cvmx_pciercx_cfg448_t pciercx_cfg448;

    /* Set the lane width */
    pciercx_cfg452.u32 = cvmx_pcie_cfgx_read(pcie_port, CVMX_PCIERCX_CFG452(pcie_port));
    pescx_ctl_status.u64 = cvmx_read_csr(CVMX_PESCX_CTL_STATUS(pcie_port));
    if (pescx_ctl_status.s.qlm_cfg == 0)
    {
        /* We're in 8 lane mode */
        pciercx_cfg452.s.lme = 0xf;
        cvmx_dprintf("We're in 8 lane mode\n");
    }
    else
    {
        /* We're in 4 lane mode */
        pciercx_cfg452.s.lme = 0x7;
        cvmx_dprintf("We're in 4 lane mode\n");
    }
    cvmx_pcie_cfgx_write(pcie_port, CVMX_PCIERCX_CFG452(pcie_port), pciercx_cfg452.u32);

    /* CN52XX pass 1 has an errata where length mismatches on UR responses can
        cause bus errors on 64bit memory reads. Turning off length error
        checking fixes this */

        switch (gd->board_desc.board_type)
        {
#ifdef CONFIG_JSRXNLE
            CASE_ALL_SRX240_MODELS
               {
                    cvmx_pciercx_cfg455_t pciercx_cfg455;

                    pciercx_cfg455.u32 = cvmx_pcie_cfgx_read(pcie_port, CVMX_PCIERCX_CFG455(pcie_port));
                    pciercx_cfg455.s.m_cpl_len_err = 1;
                    cvmx_pcie_cfgx_write(pcie_port, CVMX_PCIERCX_CFG455(pcie_port), pciercx_cfg455.u32);
                }
/* Lane swap is not needed for Vidar */
#ifndef CONFIG_JSRXNLE
                /* Lane swap needs to be manually enabled for CN52XX */
                if (OCTEON_IS_MODEL(OCTEON_CN52XX) && (pcie_port == 1))
               {
                    pescx_ctl_status.s.lane_swp = 1;

                    cvmx_write_csr(CVMX_PESCX_CTL_STATUS(pcie_port),pescx_ctl_status.u64);
                }
#endif
	         break;
#endif
	     default:
                /* Lane swap needs to be manually enabled for CN52XX */
                if (OCTEON_IS_MODEL(OCTEON_CN52XX) && (pcie_port == 1))
                {
                    pescx_ctl_status.s.lane_swp = 1;
                    cvmx_write_csr(CVMX_PESCX_CTL_STATUS(pcie_port),pescx_ctl_status.u64);
                }

	         break;
        }


    /* Bring up the link */
    pescx_ctl_status.u64 = cvmx_read_csr(CVMX_PESCX_CTL_STATUS(pcie_port));
    pescx_ctl_status.s.lnk_enb = 1;
    cvmx_write_csr(CVMX_PESCX_CTL_STATUS(pcie_port), pescx_ctl_status.u64);

    /* Force Link good for CN56XX pass 1 errata. Shouldn't apply to 1.1 */
    if (OCTEON_IS_MODEL(OCTEON_CN56XX_PASS1) || OCTEON_IS_MODEL(OCTEON_CN52XX_PASS1))
        __cvmx_pcie_force_link();

#ifdef CONFIG_JSRXNLE
    switch (gd->board_desc.board_type)
    {
        CASE_ALL_SRX240_MODELS
        CASE_ALL_SRX650_MODELS
            __cvmx_pcie_force_link();
	  break;
    }
#endif

    /* Wait for the link to come up */
    cvmx_dprintf("PCIe: Waiting for port %d link\n", pcie_port);
    start_cycle = cvmx_get_cycle();
    do
    {
        switch (gd->board_desc.board_type)
        {
#ifdef CONFIG_JSRXNLE
            CASE_ALL_SRX240_MODELS
                 if (cvmx_get_cycle() - start_cycle > 2*600*1000000)
                 {
                      cvmx_dprintf("PCIe: Port %d link timeout\n", pcie_port);
                      return -1;
                 }
	          break;
#endif
            default:
                 if (cvmx_get_cycle() - start_cycle > 2*cvmx_sysinfo_get()->cpu_clock_hz)
                 {
                      cvmx_dprintf("PCIe: Port %d link timeout\n", pcie_port);
                      return -1;
                 }
	          break;
        }
        cvmx_wait(10000);
        pciercx_cfg032.u32 = cvmx_pcie_cfgx_read(pcie_port, CVMX_PCIERCX_CFG032(pcie_port));
    } while (pciercx_cfg032.s.dlla == 0);

    /* Display the link status */
    cvmx_dprintf("PCIe: Port %d link active, %d lanes\n", pcie_port, pciercx_cfg032.s.nlw);

    /* Update the Replay Time Limit. Empirically, some PCIe devices take a
        little longer to respond than expected under load. As a workaround for
        this we configure the Replay Time Limit to the value expected for a 512
        byte MPS instead of our actual 256 byte MPS. The numbers below are
        directly from the PCIe spec table 3-4 */
    pciercx_cfg448.u32 = cvmx_pcie_cfgx_read(pcie_port, CVMX_PCIERCX_CFG448(pcie_port));
    switch (pciercx_cfg032.s.nlw)
    {
        case 1: /* 1 lane */
            pciercx_cfg448.s.rtl = 1677;
            break;
        case 2: /* 2 lanes */
            pciercx_cfg448.s.rtl = 867;
            break;
        case 4: /* 4 lanes */
            pciercx_cfg448.s.rtl = 462;
            break;
        case 8: /* 8 lanes */
            pciercx_cfg448.s.rtl = 258;
            break;
    }
    cvmx_pcie_cfgx_write(pcie_port, CVMX_PCIERCX_CFG448(pcie_port), pciercx_cfg448.u32);

    return 0;
}

/**
 * @INTERNAL
 * Initialize a host mode PCIe gen 2 link. This function takes a PCIe
 * port from reset to a link up state. Software can then begin
 * configuring the rest of the link.
 *
 * @param pcie_port PCIe port to initialize
 *
 * @return Zero on success
 */
int __cvmx_pcie_rc_initialize_link_gen2(int pcie_port)
{
    uint64_t start_cycle;
    cvmx_pemx_ctl_status_t pem_ctl_status;
    cvmx_pciercx_cfg032_t pciercx_cfg032;
    cvmx_pciercx_cfg448_t pciercx_cfg448;
    int i; 

    /* Bring up the link */
    pem_ctl_status.u64 = cvmx_read_csr(CVMX_PEMX_CTL_STATUS(pcie_port));
    pem_ctl_status.s.lnk_enb = 1;
    cvmx_write_csr(CVMX_PEMX_CTL_STATUS(pcie_port), pem_ctl_status.u64);

    /* Wait for the link to come up */
    i=0;
    do
    {
        cvmx_wait(10000);
        i++;
        if( i > 5000) 
            return -1;
        pciercx_cfg032.u32 = cvmx_pcie_cfgx_read(pcie_port, CVMX_PCIERCX_CFG032(pcie_port));
    } while (pciercx_cfg032.s.dlla == 0);

    /* Update the Replay Time Limit. Empirically, some PCIe devices take a
        little longer to respond than expected under load. As a workaround for
        this we configure the Replay Time Limit to the value expected for a 512
        byte MPS instead of our actual 256 byte MPS. The numbers below are
        directly from the PCIe spec table 3-4 */
    pciercx_cfg448.u32 = cvmx_pcie_cfgx_read(pcie_port, CVMX_PCIERCX_CFG448(pcie_port));
    switch (pciercx_cfg032.s.nlw)
    {
        case 1: /* 1 lane */
            pciercx_cfg448.s.rtl = 1677;
            break;
        case 2: /* 2 lanes */
            pciercx_cfg448.s.rtl = 867;
            break;
        case 4: /* 4 lanes */
            pciercx_cfg448.s.rtl = 462;
            break;
        case 8: /* 8 lanes */
            pciercx_cfg448.s.rtl = 258;
            break;
    }
    cvmx_pcie_cfgx_write(pcie_port, CVMX_PCIERCX_CFG448(pcie_port), pciercx_cfg448.u32);
    return 0;
}


/**
 * Initialize a PCIe gen 2 port for use in host(RC) mode. It doesn't enumerate
 * the bus.
 *
 * @param pcie_port PCIe port to initialize
 *
 * @return Zero on success
 */
int cvmx_pcie_rc_initialize_gen2(int pcie_port)
{
    int i;
    cvmx_ciu_soft_prst_t ciu_soft_prst;
    cvmx_mio_rst_ctlx_t mio_rst_ctl;
    cvmx_pemx_bar_ctl_t pemx_bar_ctl;
    cvmx_pemx_ctl_status_t pemx_ctl_status;
    cvmx_pemx_bist_status_t pemx_bist_status;
    cvmx_pemx_bist_status2_t pemx_bist_status2;
    cvmx_pciercx_cfg032_t pciercx_cfg032;
    cvmx_pciercx_cfg515_t pciercx_cfg515;
    cvmx_sli_ctl_portx_t sli_ctl_portx;
    cvmx_sli_mem_access_ctl_t sli_mem_access_ctl;
    cvmx_sli_mem_access_subidx_t mem_access_subid;
    cvmx_mio_rst_ctlx_t mio_rst_ctlx;
    cvmx_sriox_status_reg_t sriox_status_reg;
    cvmx_pemx_bar1_indexx_t bar1_index;

    /* Make sure this interface isn't SRIO */
    sriox_status_reg.u64 = cvmx_read_csr(CVMX_SRIOX_STATUS_REG(pcie_port));
    if (sriox_status_reg.s.srio)
    {
        cvmx_dprintf("PCIe: Port %d is SRIO, skipping.\n", pcie_port);
        return -1;
    }

    /* Make sure we aren't trying to setup a target mode interface in host mode */
    mio_rst_ctl.u64 = cvmx_read_csr(CVMX_MIO_RST_CTLX(pcie_port));
    if (!mio_rst_ctl.s.host_mode)
    {
        cvmx_dprintf("PCIe: Port %d in endpoint mode.\n", pcie_port);
        return -1;
    }

    /* Bring the PCIe out of reset */
    if (pcie_port)
        ciu_soft_prst.u64 = cvmx_read_csr(CVMX_CIU_SOFT_PRST1);
    else
        ciu_soft_prst.u64 = cvmx_read_csr(CVMX_CIU_SOFT_PRST);
    /* After a chip reset the PCIe will also be in reset. If it isn't,
        most likely someone is trying to init it again without a proper
        PCIe reset */
    if (ciu_soft_prst.s.soft_prst == 0)
    {
        /* Reset the port */
        ciu_soft_prst.s.soft_prst = 1;
        if (pcie_port)
            cvmx_write_csr(CVMX_CIU_SOFT_PRST1, ciu_soft_prst.u64);
        else
            cvmx_write_csr(CVMX_CIU_SOFT_PRST, ciu_soft_prst.u64);
        /* Wait until pcie resets the ports. */
        cvmx_wait_usec(2000);
    }
    if (pcie_port)
    {
        ciu_soft_prst.u64 = cvmx_read_csr(CVMX_CIU_SOFT_PRST1);
        ciu_soft_prst.s.soft_prst = 0;
        cvmx_write_csr(CVMX_CIU_SOFT_PRST1, ciu_soft_prst.u64);
    }
    else
    {
        ciu_soft_prst.u64 = cvmx_read_csr(CVMX_CIU_SOFT_PRST);
        ciu_soft_prst.s.soft_prst = 0;
        cvmx_write_csr(CVMX_CIU_SOFT_PRST, ciu_soft_prst.u64);
    }

    /* Wait for PCIe reset to complete...10 ms */
    cvmx_wait_usec(10000);

    /* Check and make sure PCIe came out of reset. If it doesn't the board
        probably hasn't wired the clocks up and the interface should be
        skipped */
    mio_rst_ctlx.u64 = cvmx_read_csr(CVMX_MIO_RST_CTLX(pcie_port));
    if (!mio_rst_ctlx.s.rst_done)
    {
        printf("PCIe: Port %d stuck in reset, skipping.\n", pcie_port);
        return -1;
    }

    /* Check BIST status */
    pemx_bist_status.u64 = cvmx_read_csr(CVMX_PEMX_BIST_STATUS(pcie_port));
    if (pemx_bist_status.u64)
        printf("PCIe: BIST FAILED for port %d (0x%016llx)\n", pcie_port, CAST64(pemx_bist_status.u64));
    pemx_bist_status2.u64 = cvmx_read_csr(CVMX_PEMX_BIST_STATUS2(pcie_port));
    if (pemx_bist_status2.u64)
        printf("PCIe: BIST2 FAILED for port %d (0x%016llx)\n", pcie_port, CAST64(pemx_bist_status2.u64));

    /* Initialize the config space CSRs */
    __cvmx_pcie_rc_initialize_config_space(pcie_port);

    /* Enable gen2 speed selection */
    pciercx_cfg515.u32 = cvmx_pcie_cfgx_read(pcie_port, CVMX_PCIERCX_CFG515(pcie_port));
    pciercx_cfg515.s.dsc = 1;
    cvmx_pcie_cfgx_write(pcie_port, CVMX_PCIERCX_CFG515(pcie_port), pciercx_cfg515.u32);

    /* Bring the link up */
    if (__cvmx_pcie_rc_initialize_link_gen2(pcie_port))
    {
        /* Some gen1 devices don't handle the gen 2 training correctly. Disable
            gen2 and try again with only gen1 */
        cvmx_pciercx_cfg031_t pciercx_cfg031;
        pciercx_cfg031.u32 = cvmx_pcie_cfgx_read(pcie_port, CVMX_PCIERCX_CFG031(pcie_port));
        pciercx_cfg031.s.mls = 1;
        cvmx_pcie_cfgx_write(pcie_port, CVMX_PCIERCX_CFG031(pcie_port), pciercx_cfg515.u32);
        if (__cvmx_pcie_rc_initialize_link_gen2(pcie_port))
        {
            printf("PCIe: Link timeout on port %d, probably the slot is empty\n", pcie_port);
            return -1;
        }
    }

    /* Store merge control (SLI_MEM_ACCESS_CTL[TIMER,MAX_WORD]) */
    sli_mem_access_ctl.u64 = cvmx_read_csr(CVMX_PEXP_SLI_MEM_ACCESS_CTL);
    sli_mem_access_ctl.s.max_word = 0;     /* Allow 16 words to combine */
    sli_mem_access_ctl.s.timer = 127;      /* Wait up to 127 cycles for more data */
    cvmx_write_csr(CVMX_PEXP_SLI_MEM_ACCESS_CTL, sli_mem_access_ctl.u64);

    /* Setup Mem access SubDIDs */
    mem_access_subid.u64 = 0;
    mem_access_subid.s.port = pcie_port; /* Port the request is sent to. */
    mem_access_subid.s.nmerge = 0;  /* Allow merging as it works on CN6XXX. */
    mem_access_subid.s.esr = 1;     /* Endian-swap for Reads. */
    mem_access_subid.s.esw = 1;     /* Endian-swap for Writes. */
    mem_access_subid.s.wtype = 0;   /* "No snoop" and "Relaxed ordering" are not set */
    mem_access_subid.s.rtype = 0;   /* "No snoop" and "Relaxed ordering" are not set */
    mem_access_subid.s.ba = 0;      /* PCIe Adddress Bits <63:34>. */

    /* Setup mem access 12-15 for port 0, 16-19 for port 1, supplying 36 bits of address space */
    for (i=12 + pcie_port*4; i<16 + pcie_port*4; i++)
    {
        cvmx_write_csr(CVMX_PEXP_SLI_MEM_ACCESS_SUBIDX(i), mem_access_subid.u64);
        mem_access_subid.s.ba += 1; /* Set each SUBID to extend the addressable range */
    }

    /* Disable the peer to peer forwarding register. This must be setup
        by the OS after it enumerates the bus and assigns addresses to the
        PCIe busses */
    for (i=0; i<4; i++)
    {
        cvmx_write_csr(CVMX_PEMX_P2P_BARX_START(i, pcie_port), -1);
        cvmx_write_csr(CVMX_PEMX_P2P_BARX_END(i, pcie_port), -1);
    }

    /* Set Octeon's BAR0 to decode 0-16KB. It overlaps with Bar2 */
    cvmx_write_csr(CVMX_PEMX_P2N_BAR0_START(pcie_port), 0);

    /* Set Octeon's BAR2 to decode 0-2^41. Bar0 and Bar1 take precedence
        where they overlap. It also overlaps with the device addresses, so
        make sure the peer to peer forwarding is set right */
    cvmx_write_csr(CVMX_PEMX_P2N_BAR2_START(pcie_port), 0);

    /* Setup BAR2 attributes */
    /* Relaxed Ordering (NPEI_CTL_PORTn[PTLP_RO,CTLP_RO, WAIT_COM]) */
    /* PTLP_RO,CTLP_RO should normally be set (except for debug). */
    /* WAIT_COM=0 will likely work for all applications. */
    /* Load completion relaxed ordering (NPEI_CTL_PORTn[WAITL_COM]) */
    pemx_bar_ctl.u64 = cvmx_read_csr(CVMX_PEMX_BAR_CTL(pcie_port));
    pemx_bar_ctl.s.bar1_siz = 3;  /* 256MB BAR1*/
    pemx_bar_ctl.s.bar2_enb = 1;
    pemx_bar_ctl.s.bar2_esx = 1;
    pemx_bar_ctl.s.bar2_cax = 0;
    cvmx_write_csr(CVMX_PEMX_BAR_CTL(pcie_port), pemx_bar_ctl.u64);
    sli_ctl_portx.u64 = cvmx_read_csr(CVMX_PEXP_SLI_CTL_PORTX(pcie_port));
    sli_ctl_portx.s.ptlp_ro = 1;
    sli_ctl_portx.s.ctlp_ro = 1;
    sli_ctl_portx.s.wait_com = 0;
    sli_ctl_portx.s.waitl_com = 0;
    cvmx_write_csr(CVMX_PEXP_SLI_CTL_PORTX(pcie_port), sli_ctl_portx.u64);

    /* BAR1 follows BAR2 */
    cvmx_write_csr(CVMX_PEMX_P2N_BAR1_START(pcie_port), CVMX_PCIE_BAR1_RC_BASE);

    bar1_index.u64 = 0;
    bar1_index.s.addr_idx = (CVMX_PCIE_BAR1_PHYS_BASE >> 22);
    bar1_index.s.ca = 1;       /* Not Cached */
    bar1_index.s.end_swp = 1;  /* Endian Swap mode */
    bar1_index.s.addr_v = 1;   /* Valid entry */

    for (i = 0; i < 16; i++) {
        cvmx_write_csr(CVMX_PEMX_BAR1_INDEXX(i, pcie_port), bar1_index.u64);
        /* 256MB / 16 >> 22 == 4 */
        bar1_index.s.addr_idx += (((1ull << 28) / 16ull) >> 22);
    }

    /* Allow config retries for 250ms. Count is based off the 5Ghz SERDES
       clock */
    pemx_ctl_status.u64 = cvmx_read_csr(CVMX_PEMX_CTL_STATUS(pcie_port));
    pemx_ctl_status.s.cfg_rtry = 250 * 5000000 / 0x10000;
    cvmx_write_csr(CVMX_PEMX_CTL_STATUS(pcie_port), pemx_ctl_status.u64);

    /* Display the link status */
    pciercx_cfg032.u32 = cvmx_pcie_cfgx_read(pcie_port, CVMX_PCIERCX_CFG032(pcie_port));
    printf("PCIe: Port %d link active, %d lanes, speed gen%d\n", pcie_port, pciercx_cfg032.s.nlw, pciercx_cfg032.s.ls);

    return 0;
}

/**
 * Initialize a PCIe port for use in host(RC) mode. It doesn't enumerate the bus.
 *
 * @param pcie_port PCIe port to initialize
 *
 * @return Zero on success
 */
int cvmx_pcie_rc_initialize(int pcie_port)
{
    DECLARE_GLOBAL_DATA_PTR;
    int i;
    cvmx_ciu_soft_prst_t ciu_soft_prst;
    cvmx_pescx_bist_status_t pescx_bist_status;
    cvmx_npei_ctl_status_t npei_ctl_status;
    cvmx_npei_mem_access_ctl_t npei_mem_access_ctl;
    cvmx_npei_mem_access_subidx_t mem_access_subid;
    cvmx_npei_dbg_data_t npei_dbg_data;
    cvmx_pescx_ctl_status2_t pescx_ctl_status2;

    printf("PCIe: Initializing port %u\n", pcie_port);

    /* CN63xx (Magni) has a new gen2 PCIE controller , gen 1 code is not working for 63XX */

    if(IS_PCIE_GEN2_MODEL(gd->board_desc.board_type)) {
        return cvmx_pcie_rc_initialize_gen2(pcie_port);
    }

    /* Make sure we aren't trying to setup a target mode interface in host mode */
    npei_ctl_status.u64 = cvmx_read_csr(CVMX_PEXP_NPEI_CTL_STATUS);
    if ((pcie_port==0) && !npei_ctl_status.s.host_mode)
    {
        printf("PCIe: ERROR: cvmx_pcie_rc_initialize() called on port0, but port0 is not in host mode\n");
        return -1;
    }

    if (OCTEON_IS_MODEL(OCTEON_CN52XX))
    {
        npei_dbg_data.u64 = cvmx_read_csr(CVMX_PEXP_NPEI_DBG_DATA);
        if ((pcie_port==1) && npei_dbg_data.cn52xx.qlm0_link_width)
        {
            printf("PCIe: ERROR: cvmx_pcie_rc_initialize() called on port1, but port1 is disabled\n");
            return -1;
        }
    }

    /* PCIe switch arbitration mode. '0' == fixed priority NPEI, PCIe0, then PCIe1. '1' == round robin. */
    npei_ctl_status.s.arb = 1;
    /* Allow up to 0x20 config retries */
    npei_ctl_status.s.cfg_rtry = 0x20;
    /* CN52XX pass1 has an errata where P0_NTAGS and P1_NTAGS don't reset */
#ifndef CONFIG_JSRXNLE
    if (OCTEON_IS_MODEL(OCTEON_CN52XX_PASS1))
#endif
    {
        npei_ctl_status.s.p0_ntags = 0x20;
        npei_ctl_status.s.p1_ntags = 0x20;
    }
    cvmx_write_csr(CVMX_PEXP_NPEI_CTL_STATUS, npei_ctl_status.u64);

    /* Bring the PCIe out of reset */
#ifndef CONFIG_JSRXNLE
    if (cvmx_sysinfo_get()->board_type == CVMX_BOARD_TYPE_EBH5200)
    {
        /* The EBH5200 board swapped the PCIe reset lines on the board. As a
            workaround for this bug, we bring both PCIe ports out of reset at
            the same time isntead of on seperate calls. So for port 0, we bring
            both out of reset and do nothing on port 1 */
        if (pcie_port == 0)
        {
            ciu_soft_prst.u64 = cvmx_read_csr(CVMX_CIU_SOFT_PRST);
            if (ciu_soft_prst.s.soft_prst == 0)
            {
                printf("PCIe: ERROR: cvmx_pcie_rc_initialize_link() called but port%d is not in reset\n", pcie_port);
                return -1;
            }
            ciu_soft_prst.u64 = cvmx_read_csr(CVMX_CIU_SOFT_PRST1);
            ciu_soft_prst.s.soft_prst = 0;
            cvmx_write_csr(CVMX_CIU_SOFT_PRST1, ciu_soft_prst.u64);
            ciu_soft_prst.u64 = cvmx_read_csr(CVMX_CIU_SOFT_PRST);
            ciu_soft_prst.s.soft_prst = 0;
            cvmx_write_csr(CVMX_CIU_SOFT_PRST, ciu_soft_prst.u64);
        } else {
            ciu_soft_prst.u64 = cvmx_read_csr(CVMX_CIU_SOFT_PRST1);
            if (ciu_soft_prst.s.soft_prst == 0)
            {
                printf("PCIe: ERROR: cvmx_pcie_rc_initialize_link() called but port%d is not in reset\n", pcie_port);
                return -1;
            }
            ciu_soft_prst.u64 = cvmx_read_csr(CVMX_CIU_SOFT_PRST1);
            ciu_soft_prst.s.soft_prst = 0;
            cvmx_write_csr(CVMX_CIU_SOFT_PRST1, ciu_soft_prst.u64);
        }
    }
    else
    {
        /* For 56XX, the PCIe ports are completely separate and can be brought
            out of reset independently */
        if (pcie_port)
            ciu_soft_prst.u64 = cvmx_read_csr(CVMX_CIU_SOFT_PRST1);
        else
            ciu_soft_prst.u64 = cvmx_read_csr(CVMX_CIU_SOFT_PRST);
        if (ciu_soft_prst.s.soft_prst == 0)
        {
            printf("PCIe: ERROR: cvmx_pcie_rc_initialize_link() called but port%d is not in reset\n", pcie_port);
            return -1;
        }
        if (pcie_port)
        {
            ciu_soft_prst.u64 = cvmx_read_csr(CVMX_CIU_SOFT_PRST1);
            ciu_soft_prst.s.soft_prst = 0;
            cvmx_write_csr(CVMX_CIU_SOFT_PRST1, ciu_soft_prst.u64);
        }
        else
        {
            ciu_soft_prst.u64 = cvmx_read_csr(CVMX_CIU_SOFT_PRST);
            ciu_soft_prst.s.soft_prst = 0;
            cvmx_write_csr(CVMX_CIU_SOFT_PRST, ciu_soft_prst.u64);
        }
    }
#else

    if (pcie_port)
        ciu_soft_prst.u64 = cvmx_read_csr(CVMX_CIU_SOFT_PRST1);
    else
        ciu_soft_prst.u64 = cvmx_read_csr(CVMX_CIU_SOFT_PRST);

    if (ciu_soft_prst.s.soft_prst == 0) {
        printf("PCIe: ERROR: cvmx_pcie_rc_initialize_link() called but port%d is not in reset\n", pcie_port);
        return -1;
    }
    if (pcie_port) {
        ciu_soft_prst.u64 = cvmx_read_csr(CVMX_CIU_SOFT_PRST1);
        ciu_soft_prst.s.soft_prst = 0;
        cvmx_write_csr(CVMX_CIU_SOFT_PRST1, ciu_soft_prst.u64);
    } else {
        ciu_soft_prst.u64 = cvmx_read_csr(CVMX_CIU_SOFT_PRST);
        ciu_soft_prst.s.soft_prst = 0;
        cvmx_write_csr(CVMX_CIU_SOFT_PRST, ciu_soft_prst.u64);
    }
#endif
    /* Wait for PCIe reset to complete. Due to errata PCIE-700, we don't poll
       PESCX_CTL_STATUS2[PCIERST], but simply wait a fixed number of cycles */
    cvmx_wait(400000);

    /* Check and make sure PCIe came out of reset. If it doesn't the board
        probably hasn't wired the clocks up and the interface shuold be
        skipped */
    pescx_ctl_status2.u64 = cvmx_read_csr(CVMX_PESCX_CTL_STATUS2(pcie_port));
    if (pescx_ctl_status2.s.pcierst)
    {
        printf("PCIe: Port %d appears to not be clocked, skipping.\n", pcie_port);
        return -1;
    }

    /* Check BIST status */
    pescx_bist_status.u64 = cvmx_read_csr(CVMX_PESCX_BIST_STATUS(pcie_port));
    if (pescx_bist_status.u64)
        printf("PCIe: BIST FAILED for port %d (0x%016llx)\n", pcie_port, CAST64(pescx_bist_status.u64));

    /* Initialize the config space CSRs */
    __cvmx_pcie_rc_initialize_config_space(pcie_port);

    /* Bring the link up */
    if (__cvmx_pcie_rc_initialize_link(pcie_port))
    {
        cvmx_dprintf("PCIe: ERROR: cvmx_pcie_rc_initialize_link() failed\n");
        return -1;
    }

    /* Store merge control (NPEI_MEM_ACCESS_CTL[TIMER,MAX_WORD]) */
/* SAN: FIX the setting */
    switch (gd->board_desc.board_type)
    {
#ifdef CONFIG_JSRXNLE
      CASE_ALL_SRX240_MODELS
          npei_mem_access_ctl.u64 = cvmx_read_csr(CVMX_PEXP_NPEI_MEM_ACCESS_CTL);
          npei_mem_access_ctl.s.max_word = 1;     /* Allow 16 words to combine */
          npei_mem_access_ctl.s.timer = 1;      /* Wait up to 127 cycles for more data */
          cvmx_write_csr(CVMX_PEXP_NPEI_MEM_ACCESS_CTL, npei_mem_access_ctl.u64);

          /* Setup Mem access SubDIDs */
          mem_access_subid.u64 = 0;
          mem_access_subid.s.port = pcie_port; /* Port the request is sent to. */
          mem_access_subid.s.nmerge = 1;  /* Merging is allowed in this window. */
	   break;

#endif
      default:
          npei_mem_access_ctl.u64 = cvmx_read_csr(CVMX_PEXP_NPEI_MEM_ACCESS_CTL);
          npei_mem_access_ctl.s.max_word = 0;     /* Allow 16 words to combine */
          npei_mem_access_ctl.s.timer = 127;      /* Wait up to 127 cycles for more data */
          cvmx_write_csr(CVMX_PEXP_NPEI_MEM_ACCESS_CTL, npei_mem_access_ctl.u64);

          /* Setup Mem access SubDIDs */
          mem_access_subid.u64 = 0;
          mem_access_subid.s.port = pcie_port; /* Port the request is sent to. */
          mem_access_subid.s.nmerge = 0;  /* No Merging is allowed in this window. */
          break;
    }
    mem_access_subid.s.esr = 1;     /* Endian-swap for Reads. */
    mem_access_subid.s.esw = 1;     /* Endian-swap for Writes. */
    mem_access_subid.s.nsr = 1;     /* No Snoop for Reads. */
    mem_access_subid.s.nsw = 1;     /* No Snoop for Writes. */
    mem_access_subid.s.ror = 0;     /* Disable Relaxed Ordering for Reads. */
    mem_access_subid.s.row = 0;     /* Disable Relaxed Ordering for Writes. */
    mem_access_subid.s.ba = 0;      /* PCIe Adddress Bits <63:34>. */

    /* Setup mem access 12-15 for port 0, 16-19 for port 1, supplying 36 bits of address space */
    for (i=12 + pcie_port*4; i<16 + pcie_port*4; i++)
    {
        cvmx_write_csr(CVMX_PEXP_NPEI_MEM_ACCESS_SUBIDX(i), mem_access_subid.u64);
        mem_access_subid.s.ba += (1ull); /* Set each SUBID to extend the addressable range */
    }

    /* Disable the peer to peer forwarding register. This must be setup
        by the OS after it enumerates the bus and assigns addresses to the
        PCIe busses */
    for (i=0; i<4; i++)
    {
        cvmx_write_csr(CVMX_PESCX_P2P_BARX_START(i, pcie_port), -1);
        cvmx_write_csr(CVMX_PESCX_P2P_BARX_END(i, pcie_port), -1);
    }

    /* Set Octeon's BAR0 to decode 0-16KB. It overlaps with Bar2 */
    cvmx_write_csr(CVMX_PESCX_P2N_BAR0_START(pcie_port), 0);

    /* Disable Octeon's BAR1. It isn't needed in RC mode since BAR2
        maps all of memory. BAR2 also maps 256MB-512MB into the 2nd
        256MB of memory */
    cvmx_write_csr(CVMX_PESCX_P2N_BAR1_START(pcie_port), -1);

    /* Set Octeon's BAR2 to decode 0-2^39. Bar0 and Bar1 take precedence
        where they overlap. It also overlaps with the device addresses, so
        make sure the peer to peer forwarding is set right */
    cvmx_write_csr(CVMX_PESCX_P2N_BAR2_START(pcie_port), 0);

    /* Setup BAR2 attributes */
    /* Relaxed Ordering (NPEI_CTL_PORTn[PTLP_RO,CTLP_RO, WAIT_COM]) */
    /* ­ PTLP_RO,CTLP_RO should normally be set (except for debug). */
    /* ­ WAIT_COM=0 will likely work for all applications. */
    /* Load completion relaxed ordering (NPEI_CTL_PORTn[WAITL_COM]) */
    if (pcie_port)
    {
        cvmx_npei_ctl_port1_t npei_ctl_port;
        npei_ctl_port.u64 = cvmx_read_csr(CVMX_PEXP_NPEI_CTL_PORT1);
        npei_ctl_port.s.bar2_enb = 1;
        npei_ctl_port.s.bar2_esx = 1;
        npei_ctl_port.s.bar2_cax = 0;
        npei_ctl_port.s.ptlp_ro = 1;
        npei_ctl_port.s.ctlp_ro = 1;
        npei_ctl_port.s.wait_com = 0;
        npei_ctl_port.s.waitl_com = 0;
        cvmx_write_csr(CVMX_PEXP_NPEI_CTL_PORT1, npei_ctl_port.u64);
    }
    else
    {
        cvmx_npei_ctl_port0_t npei_ctl_port;
        npei_ctl_port.u64 = cvmx_read_csr(CVMX_PEXP_NPEI_CTL_PORT0);
        npei_ctl_port.s.bar2_enb = 1;
        npei_ctl_port.s.bar2_esx = 1;
        npei_ctl_port.s.bar2_cax = 0;
        npei_ctl_port.s.ptlp_ro = 1;
        npei_ctl_port.s.ctlp_ro = 1;
        npei_ctl_port.s.wait_com = 0;
        npei_ctl_port.s.waitl_com = 0;
        cvmx_write_csr(CVMX_PEXP_NPEI_CTL_PORT0, npei_ctl_port.u64);
    }
    return 0;
}


/**
 * Shutdown a PCIe port and put it in reset
 *
 * @param pcie_port PCIe port to shutdown
 *
 * @return Zero on success
 */
int cvmx_pcie_rc_shutdown(int pcie_port)
{
    /* Wait for all pending operations to complete */
    cvmx_pescx_cpl_lut_valid_t pescx_cpl_lut_valid;
    do
    {
        pescx_cpl_lut_valid.u64 = cvmx_read_csr(CVMX_PESCX_CPL_LUT_VALID(pcie_port));
        if (pescx_cpl_lut_valid.s.tag)
            cvmx_wait(10000);
    } while (pescx_cpl_lut_valid.s.tag);

    /* Force reset */
    if (pcie_port)
    {
        cvmx_ciu_soft_prst_t ciu_soft_prst;
        ciu_soft_prst.u64 = cvmx_read_csr(CVMX_CIU_SOFT_PRST1);
        ciu_soft_prst.s.soft_prst = 1;
        cvmx_write_csr(CVMX_CIU_SOFT_PRST1, ciu_soft_prst.u64);
    }
    else
    {
        cvmx_ciu_soft_prst_t ciu_soft_prst;
        ciu_soft_prst.u64 = cvmx_read_csr(CVMX_CIU_SOFT_PRST);
        ciu_soft_prst.s.soft_prst = 1;
        cvmx_write_csr(CVMX_CIU_SOFT_PRST, ciu_soft_prst.u64);
    }
    return 0;
}


/**
 * @INTERNAL
 * Build a PCIe config space request address for a device
 *
 * @param pcie_port PCIe port to access
 * @param bus       Sub bus
 * @param dev       Device ID
 * @param fn        Device sub function
 * @param reg       Register to access
 *
 * @return 64bit Octeon IO address
 */
static inline uint64_t __cvmx_pcie_build_config_addr(int pcie_port, int bus, int dev, int fn, int reg)
{
    cvmx_pcie_address_t pcie_addr;
    cvmx_pciercx_cfg006_t pciercx_cfg006;

    if (OCTEON_IS_MODEL(OCTEON_CN63XX)) {
        pciercx_cfg006.u32 =
            cvmx_pcie_cfgx_read(pcie_port, CVMX_PCIERCX_CFG006(pcie_port));
        if ((bus <= pciercx_cfg006.s.pbnum) && (dev != 0))
            return 0;
        
        pcie_addr.u64 = 0;
        pcie_addr.config.upper = 2;
        pcie_addr.config.io = 1;
        pcie_addr.config.did = 3;
        pcie_addr.config.subdid = 1;
        pcie_addr.config.es = 1;
        pcie_addr.config.port = pcie_port;
        pcie_addr.config.ty = (bus > pciercx_cfg006.s.pbnum);
        pcie_addr.config.bus = bus;
        pcie_addr.config.dev = dev;
        pcie_addr.config.func = fn;
        pcie_addr.config.reg = reg;
        return pcie_addr.u64;
    } else {
        pcie_addr.u64 = 0;
        pcie_addr.config.upper = 2;
        pcie_addr.config.io = 1;
        pcie_addr.config.did = 3;
        pcie_addr.config.subdid = 1;
        pcie_addr.config.es = 1;
        pcie_addr.config.port = pcie_port;
        pcie_addr.config.ty = (bus != 0);
        pcie_addr.config.bus = bus;
        pcie_addr.config.dev = dev;
        pcie_addr.config.func = fn;
        pcie_addr.config.reg = reg;
        return pcie_addr.u64;
    }
}


/**
 * Read 8bits from a Device's config space
 *
 * @param pcie_port PCIe port the device is on
 * @param bus       Sub bus
 * @param dev       Device ID
 * @param fn        Device sub function
 * @param reg       Register to access
 *
 * @return Result of the read
 */
uint8_t cvmx_pcie_config_read8(int pcie_port, int bus, int dev, int fn, int reg)
{
    uint64_t address = __cvmx_pcie_build_config_addr(pcie_port, bus, dev, fn, reg);
    if (address)
        return cvmx_read64_uint8(address);
    else
        return 0xff;
}


/**
 * Read 16bits from a Device's config space
 *
 * @param pcie_port PCIe port the device is on
 * @param bus       Sub bus
 * @param dev       Device ID
 * @param fn        Device sub function
 * @param reg       Register to access
 *
 * @return Result of the read
 */
uint16_t cvmx_pcie_config_read16(int pcie_port, int bus, int dev, int fn, int reg)
{
    uint64_t address = __cvmx_pcie_build_config_addr(pcie_port, bus, dev, fn, reg);
    if (address)
        return cvmx_le16_to_cpu(cvmx_read64_uint16(address));
    else
        return 0xffff;
}

/**
 * Read 32bits from a Device's config space
 *
 * @param pcie_port PCIe port the device is on
 * @param bus       Sub bus
 * @param dev       Device ID
 * @param fn        Device sub function
 * @param reg       Register to access
 *
 * @return Result of the read
 */
uint32_t cvmx_pcie_config_read32(int pcie_port, int bus, int dev, int fn, int reg)
{
    uint64_t address = __cvmx_pcie_build_config_addr(pcie_port, bus, dev, fn, reg);
    if (address)
    {
        return cvmx_le32_to_cpu(cvmx_read64_uint32(address));
    }
    else
        return 0xffffffff;
}

/**
 * Write 8bits to a Device's config space
 *
 * @param pcie_port PCIe port the device is on
 * @param bus       Sub bus
 * @param dev       Device ID
 * @param fn        Device sub function
 * @param reg       Register to access
 * @param val       Value to write
 */
void cvmx_pcie_config_write8(int pcie_port, int bus, int dev, int fn, int reg, uint8_t val)
{
    uint64_t address = __cvmx_pcie_build_config_addr(pcie_port, bus, dev, fn, reg);
    if (address)
        cvmx_write64_uint8(address, val);
}


/**
 * Write 16bits to a Device's config space
 *
 * @param pcie_port PCIe port the device is on
 * @param bus       Sub bus
 * @param dev       Device ID
 * @param fn        Device sub function
 * @param reg       Register to access
 * @param val       Value to write
 */
void cvmx_pcie_config_write16(int pcie_port, int bus, int dev, int fn, int reg, uint16_t val)
{
    uint64_t address = __cvmx_pcie_build_config_addr(pcie_port, bus, dev, fn, reg);
    if (address)
        cvmx_write64_uint16(address, cvmx_cpu_to_le16(val));
}

/**
 * Write 32bits to a Device's config space
 *
 * @param pcie_port PCIe port the device is on
 * @param bus       Sub bus
 * @param dev       Device ID
 * @param fn        Device sub function
 * @param reg       Register to access
 * @param val       Value to write
 */
void cvmx_pcie_config_write32(int pcie_port, int bus, int dev, int fn, int reg, uint32_t val)
{
    cvmx_write64_uint32(__cvmx_pcie_build_config_addr(pcie_port, bus, dev, fn, reg), swap32(val));

    uint64_t address = __cvmx_pcie_build_config_addr(pcie_port, bus, dev, fn, reg);
    if (address)
        cvmx_write64_uint32(address, cvmx_cpu_to_le32(val));
}

/**
 * Read a PCIe config space register indirectly. This is used for
 * registers of the form PCIEEP_CFG??? and PCIERC?_CFG???.
 *
 * @param pcie_port  PCIe port to read from
 * @param cfg_offset Address to read
 *
 * @return Value read
 */
uint32_t cvmx_pcie_cfgx_read(int pcie_port, uint32_t cfg_offset)
{
    DECLARE_GLOBAL_DATA_PTR;

    if(IS_PCIE_GEN2_MODEL(gd->board_desc.board_type)) {
        cvmx_pemx_cfg_rd_t pemx_cfg_rd;
        pemx_cfg_rd.u64 = 0;
        pemx_cfg_rd.s.addr = cfg_offset;
        cvmx_write_csr(CVMX_PEMX_CFG_RD(pcie_port), pemx_cfg_rd.u64);
        pemx_cfg_rd.u64 = cvmx_read_csr(CVMX_PEMX_CFG_RD(pcie_port));
        return pemx_cfg_rd.s.data;
    } else {
        cvmx_pescx_cfg_rd_t pescx_cfg_rd;
        pescx_cfg_rd.u64 = 0;
        pescx_cfg_rd.s.addr = cfg_offset;
        cvmx_write_csr(CVMX_PESCX_CFG_RD(pcie_port), pescx_cfg_rd.u64);
        pescx_cfg_rd.u64 = cvmx_read_csr(CVMX_PESCX_CFG_RD(pcie_port));
        return pescx_cfg_rd.s.data;
    }
}


/**
 * Write a PCIe config space register indirectly. This is used for
 * registers of the form PCIEEP_CFG??? and PCIERC?_CFG???.
 *
 * @param pcie_port  PCIe port to write to
 * @param cfg_offset Address to write
 * @param val        Value to write
 */
void cvmx_pcie_cfgx_write(int pcie_port, uint32_t cfg_offset, uint32_t val)
{
    DECLARE_GLOBAL_DATA_PTR;

    if(IS_PCIE_GEN2_MODEL(gd->board_desc.board_type)) {
        cvmx_pemx_cfg_wr_t pemx_cfg_wr;
        pemx_cfg_wr.u64 = 0;
        pemx_cfg_wr.s.addr = cfg_offset;
        pemx_cfg_wr.s.data = val;
        cvmx_write_csr(CVMX_PEMX_CFG_WR(pcie_port), pemx_cfg_wr.u64);
    } else {
        cvmx_pescx_cfg_wr_t pescx_cfg_wr;
        pescx_cfg_wr.u64 = 0;
        pescx_cfg_wr.s.addr = cfg_offset;
        pescx_cfg_wr.s.data = val;
        cvmx_write_csr(CVMX_PESCX_CFG_WR(pcie_port), pescx_cfg_wr.u64);
    }
}

#if defined(CONFIG_JSRXNLE) || defined(CONFIG_MAG8600)
/**
 * Read 32bits from a Device's Memory space
 *
 * @param pcie_port PCIe port the device is on
 * @param offset       Offset to access (Mem base address + reg offset)
 *
 * @return Result of the read
 */
uint32_t 
cvmx_pcie_mem_read32 (int pcie_port, uint32_t offset)
{
    CVMX_SYNCW;
    return swap32(cvmx_read64_uint32(cvmx_pcie_get_mem_base_address(pcie_port) + offset));
}

/**
 * Write 32bits to a Device's Mem space
 *
 * @param pcie_port PCIe port the device is on
 * @param offset       Offset to access (Mem base address + reg offset)
 * @param val       Value to write
 */
void 
cvmx_pcie_mem_write32 (int pcie_port, uint32_t offset, uint32_t val)
{
    CVMX_SYNCW;
    cvmx_write64_uint32(cvmx_pcie_get_mem_base_address(pcie_port) + offset, swap32(val));
    CVMX_SYNCW;
}

static void 
fix_gen1_gen2_issue(void)
{
    uint32_t data = 0;
    int pcie_port= 1 , bus= 1, slot= 0, func= 0;
    int i = 0;
    int offset = 0xf0000000;

    udelay(1000000);

    data = cvmx_pcie_mem_read32(1, offset + 0x230);
    data = data | 0xfe;
    cvmx_pcie_mem_write32(1, offset +0x230, data);

    data = cvmx_pcie_mem_read32(1, offset +0x230);
    data = cvmx_pcie_mem_read32(1, offset + 0x234);

    data = data | 0xd1;
    cvmx_pcie_mem_write32(1, offset + 0x234, data);

    data = cvmx_pcie_mem_read32(1, offset + 0x234);

    data = cvmx_pcie_config_read32(1, 1, 0, 0, 0x98);
    data = data & 0xfffffffc;
    data = data | 0x1;
    cvmx_pcie_config_write32(1, 1, 0, 0, 0x98, data);
    data = cvmx_pcie_config_read32(1, 1, 0, 0, 0x98);
 
    for (i= 1 ;i< 15; i++) {
        if ( (i == 3) || (i == 11) || (i == 13))
            continue;
        /* Set speed to 2.5 G*/
        data = cvmx_pcie_config_read32(1, 2, i, 0, 0x98);
        data = data & 0xfffffffc;
        data = data | 0x1;
        cvmx_pcie_config_write32(1, 2, i, 0, 0x98, data);
        data = cvmx_pcie_config_read32(1, 2, i, 0, 0x98);
    }


    data = cvmx_pcie_mem_read32(1, offset + 0x230);
    data = data & 0xFFFFFF01;;
    cvmx_pcie_mem_write32(1, offset + 0x230, data);
    data = cvmx_pcie_mem_read32(1, offset + 0x230);

    data = cvmx_pcie_mem_read32(1, offset + 0x234);
    data = data & 0xFFFFFF2E;
    cvmx_pcie_mem_write32(1, offset + 0x234, data);

    data = cvmx_pcie_mem_read32(1, offset + 0x234);

    udelay(1000000);
}

int
octeon_pci_init_bar (int pcie_port, int bus, int slot, int func,
                     int barno)
{
    bus_space_handle_t *allocp;
    bus_space_handle_t addr;
    uint32_t mask, size;
    int reg, width;

    reg = PCIR_BAR(barno);
    cvmx_pcie_config_write32(pcie_port, bus, slot, func, reg, ~0);
    size = cvmx_pcie_config_read32(pcie_port, bus, slot, func, reg);
    width = ((size & 7) == 4) ? 2 : 1;
    /* Unused BARs are hardwired to 0. */
    if (size == 0)
    {
        return (width);
    }

    /*
     * SRX_FIXME - Need to make sure this coexists with
     * octeon_pci_alloc_resource(),
     * or allocation via alternative call is not allowed.
     */
    if (size & 1) {     /* I/O port */
        allocp = &sc_io_poffset;
        size &= ~3;
        if ((size & 0xffff0000) == 0)
            size |= 0xffff0000;
    } else {        /* memory */
        allocp = &sc_mem_poffset;
        size &= ~15;
    }
    mask = ~size;
    size = mask + 1;
    /* Sanity check (must be a power of 2). */
    if (size & mask)
    {
      cvmx_dprintf("    Sanity check failed: size = 0x%lx, mask = 0x%lx\n",size,mask);
        return (width);
    }

    addr = (*allocp & 0xffffffff00000000ull);
    addr |= (((*allocp & 0xffffffff) + mask) & ~mask);
    *allocp = addr + size;
    cvmx_pcie_config_write32(pcie_port, bus, slot, func, reg, addr);
    if (width == 2)
        cvmx_pcie_config_write32(pcie_port, bus, slot, func, reg + 4,0);

    return (width);
}

static void
prescan_setup_bridge (int pcie_port, int bus, int slot, int func,
                     int sbusno)
{
    bus_space_handle_t *allocpm, *allocpi;

    allocpm = &sc_mem_poffset;
    allocpi = &sc_io_poffset;

    /* Program I/O decoder. */
    cvmx_pcie_config_write8(pcie_port, bus, slot, func,
                            PCIR_IOBASEL_1, *allocpi >> 8);
    cvmx_pcie_config_write16(pcie_port, bus, slot, func,
                            PCIR_IOBASEH_1, *allocpi >> 16);

    /* Program  Mem Base of (non-prefetchable) memory decoder. */
    cvmx_pcie_config_write16(pcie_port, bus, slot, func,
                            PCIR_MEMBASE_1, *allocpm >> 16);

    /* Program prefetchable memory decoder.(not used) */
    cvmx_pcie_config_write16(pcie_port, bus, slot, func,
                            PCIR_PMBASEL_1, 0x00f0);
    cvmx_pcie_config_write16(pcie_port, bus, slot, func,
                            PCIR_PMLIMITL_1, 0x000f);
    cvmx_pcie_config_write32(pcie_port, bus, slot, func,
                            PCIR_PMBASEH_1, 0x00000000);
    cvmx_pcie_config_write32(pcie_port, bus, slot, func,
                            PCIR_PMLIMITH_1, 0x00000000);

    /* Program the Bus number registers of the bridge */
    cvmx_pcie_config_write8(pcie_port, bus, slot, func,
                            PCIR_PRIBUS_1, bus);
    cvmx_pcie_config_write8(pcie_port, bus, slot, func,
                            PCIR_SECBUS_1, sbusno);
    cvmx_pcie_config_write8(pcie_port, bus, slot, func,
                            PCIR_SUBBUS_1, 0xff);
    cvmx_pcie_config_read8(pcie_port, bus, slot, func,
              PCIR_SUBBUS_1);

}

static void
postscan_setup_bridge (int pcie_port, int bus, int slot, int func,
                      int sbusno)
{

    cvmx_dprintf ("postscan_setup_bridge  %d:%d:%d:%d\n",pcie_port, bus, slot, func);
    bus_space_handle_t *allocpm, *allocpi;

    allocpm = &sc_mem_poffset;
    allocpi = &sc_io_poffset;
    u_int32_t size= 1 << 20;
    uint16_t vendor, device;
    vendor = cvmx_pcie_config_read16(pcie_port, bus, slot, func,
                                     PCIR_VENDOR);
    device = cvmx_pcie_config_read16(pcie_port, bus, slot, func,
                                     PCIR_DEVICE);

    cvmx_pcie_config_read8(pcie_port, bus, slot, func,
                            PCIR_SUBBUS_1);

    /* Program I/O decoder. */
    cvmx_pcie_config_write8(pcie_port, bus, slot, func,
                            PCIR_IOLIMITL_1, (*allocpi - 1) >> 8);
    cvmx_pcie_config_write16(pcie_port, bus, slot, func,
                            PCIR_IOLIMITH_1, (*allocpi - 1) >> 16);

    /* Program (non-prefetchable) memory decoder. */
    cvmx_pcie_config_write16(pcie_port, bus, slot, func,
                            PCIR_MEMLIMIT_1, (*allocpm - 1) >> 16);

    /*
     * On PLX 8509/8614 , readjust allocpm to start from the next window
     */
    if ((vendor == PEX_VENDOR_ID) &&
        ((device == PEX_8509_DEVICE_ID) || (device == PEX_8614_DEVICE_ID))){
        *allocpm = (*allocpm & 0xffffffff00000000ull) |
            (((*allocpm & 0xffffffff) + (size - 1)) & ~(size - 1));
    }
    /* Program prefetchable memory decoder. */
    cvmx_pcie_config_write8(pcie_port, bus, slot, func,
                            PCIR_SUBBUS_1, sbusno);
    cvmx_pcie_config_read8(pcie_port, bus, slot, func,
                            PCIR_SUBBUS_1);
}



static void
octeon_pcie_scan_configure (int pcie_port, int bus, int dev, int func)
{
    uint32_t bcmValue = 0;
    u_int16_t vendor = 0, device = 0;
    uint8_t cmdstat = 0;
    uint8_t hdrtype;
    int maxfunc = 0;
    int bar, maxbar;

    cvmx_dprintf ("octeon_pcie_scan_configure %d:%d:%d:%d\n",pcie_port, bus, dev, func);
    bcmValue = cvmx_pcie_config_read32(pcie_port, bus,dev,func,CONFIG_VENDOR_DEVICE_ID);
    vendor = cvmx_pcie_config_read16(pcie_port, bus, dev, func, PCIR_VENDOR);
    device = cvmx_pcie_config_read16(pcie_port, bus, dev, func, PCIR_DEVICE);
    cmdstat = cvmx_pcie_config_read8(pcie_port, bus,dev,func, PCIR_COMMAND);
    cmdstat = (cmdstat & ~(PCIM_CMD_PORTEN | PCIM_CMD_MEMEN));
    cvmx_pcie_config_write8(pcie_port, bus,dev,func, PCIR_COMMAND, cmdstat);

    /* PCIE Header type */
    hdrtype = cvmx_pcie_config_read8(pcie_port, bus,dev,func, 0xe);
    if ((hdrtype & PCIM_HDRTYPE) > PCI_MAXHDRTYPE)
        return;

    if (func == 0 && (hdrtype & PCIM_MFDEV))
        maxfunc = PCI_FUNCMAX;

    /* Program the base address registers. */
    maxbar = (hdrtype & PCIM_HDRTYPE) ? 1 : 6;
    bar = 0;
    while (bar < maxbar) {
        bar += octeon_pci_init_bar(pcie_port, bus,dev,func, bar);
    }

    cmdstat |= PCIM_CMD_BUSMASTEREN | PCIM_CMD_MEMEN |
                    PCIM_CMD_PORTEN;
    cvmx_pcie_config_write8(pcie_port, bus,dev,func,
                     PCIR_COMMAND, cmdstat);

    return ;
}

void
cvmx_pcie_config_regex (void)
{
    DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_JSRXNLE
    /* This hook is only for SRX650 platform */
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX650_MODELS
        sc_io_poffset = CVMX_ADD_IO_SEG(cvmx_pcie_get_io_base_address(
                                        SRX650_PCIE_REGEX_PORT));
        sc_mem_poffset = CVMX_ADD_IO_SEG(cvmx_pcie_get_mem_base_address(
                                         SRX650_PCIE_REGEX_PORT));
        cvmx_pcie_rc_initialize(SRX650_PCIE_REGEX_PORT);
        octeon_pcie_scan_configure(SRX650_PCIE_REGEX_PORT, 
                                   SRX650_PCIE_REGEX_BUS,
                                   SRX650_PCIE_REGEX_DEV, 
                                   SRX650_PCIE_REGEX_FUNC);
        break;
    default:
        break;
    }
#endif
    return;
}

static 
int do_device(int pcie_port, int bus, int device, int num_busses)
{
    int func;
    int found_multi = 0;

    for (func=0; func<MAX_NUM_FUNCS; func++)
    {
        cvmx_wait_usec(10000);
        uint32_t v = cvmx_pcie_config_read32(pcie_port, bus, device, func, CONFIG_VENDOR_DEVICE_ID);
        if (!except)
            v = cvmx_pcie_config_read32(pcie_port, bus, device, func, CONFIG_VENDOR_DEVICE_ID);
        /* If an exception occured, then we need to mark this location bad and
         *             redo the complete bus scan */
        if (except)
        {
            cvmx_dprintf("%d:d:d.%d Fail\n", pcie_port, bus, device, func);
            return -1;
        }

        if (func && !found_multi)
            continue;

        /* Nothing here, skip the rest of the functions */
        if (v == 0xffffffff)
            break;

        cvmx_dprintf("Port %d %d:%d:%d v 0x%08x\n", pcie_port, bus, device, func, v);

        uint32_t h = cvmx_pcie_config_read8(pcie_port, bus, device, func, CONFIG_PCI_HEADER_TYPE);
        if (!func)
           found_multi = h & 0x80;

        octeon_pcie_scan_configure(pcie_port, bus, device, func);
        /* Check if this is a bridge */
        uint8_t b = cvmx_pcie_config_read8(pcie_port, bus, device, func, CONFIG_CLASS_CODE_HIGH);
        if (b == CONFIG_CLASS_BRIDGE)
        {
            cvmx_dprintf("%*s%d:%02d:%02d.%d 0x%08x Bridge\n", indent,"", pcie_port, bus, device, func, v);
            /* Setup the bridge to allow us to scan the stuff behind it */

            prescan_setup_bridge(pcie_port, bus, device, func,
                                 bus+num_busses+1);
            /* Scan behind the bridge */
            int r = do_bus(pcie_port, bus+num_busses+1);
            /* We get a -1 if a bus error requires a full rescan */
            if (r == -1)
                return -1;
            postscan_setup_bridge(pcie_port, bus, device, func,
                                  bus+num_busses+r);
            num_busses += r;
        }
        else
            cvmx_dprintf("%*s%d:%02d:%02d.%d 0x%08x\n", indent,"", pcie_port, bus, device, func, v);
    }
    return num_busses;
}


static int do_bus(int pcie_port, int bus)
{
    int device;
    int num_busses = 0;
    for (device=0; device<MAX_NUM_DEVICES; device++)
    {
        /* There will never be more than one device hooked directly to Octeon */
        if ((bus == 1) && (device != 0))
            continue;

        num_busses = do_device(pcie_port, bus, device, num_busses);
        /* We get a -1 if a bus error requires a full rescan */
        if (num_busses == -1)
            return -1;
    }
    return num_busses+1;
}

void 
pcie_init (void)
{
    DECLARE_GLOBAL_DATA_PTR;

    uint8_t i, j, pcie_port;
    uint8_t num_busses = 0;

#ifdef CONFIG_JSRXNLE

    switch (gd->board_desc.board_type)
    {
      CASE_ALL_SRX240_MODELS
        sc_io_poffset = CVMX_ADD_IO_SEG(cvmx_pcie_get_io_base_address(1));
        sc_mem_poffset = CVMX_ADD_IO_SEG(cvmx_pcie_get_mem_base_address(1));

        if (cvmx_pcie_rc_initialize(0) != 0) {
            if ((gd->board_desc.board_type != 
                I2C_ID_JSRXNLE_SRX240_LOWMEM_CHASSIS) && 
                (gd->board_desc.board_type !=
                I2C_ID_JSRXNLE_SRX240B2_CHASSIS)) {
                printf("cvmx_pcie_rc_initialize failed for port 0\n");
            }
        }
        /* SAN: Need delay here ? */
        if (cvmx_pcie_rc_initialize(1) != 0) {
            printf("cvmx_pcie_rc_initialize failed for port 1\n");
            return;
        }

        octeon_pcie_scan_configure (1, 0,0,0);
        prescan_setup_bridge(1,0,0,0,1);

        octeon_pcie_scan_configure (1, 1,7,0);
        postscan_setup_bridge(1,0,0,0,1);
        break;

      CASE_ALL_SRX650_MODELS
        sc_io_poffset = CVMX_ADD_IO_SEG(cvmx_pcie_get_io_base_address(0));
        sc_mem_poffset = CVMX_ADD_IO_SEG(cvmx_pcie_get_mem_base_address(0));

        cvmx_pcie_rc_initialize(0);

        /* 1st PEX 8509 configuration starts here on 0:0:0 */
        /* This PEX has Internal Bus : 1 */
        /* Bus 1 : has 5 ports */
        /*   Port 0 : Connected to CPU complex */
        /*   Port 1 : Connected to BCM 56680 using Bus 2 */
        /*   Port 2 : Connected to connected to SiI3132 using Bus 3 */
        /*   Port 3 : Connected to other PEX8509 using Bus 4 */
        /*   Port 4 : Connected to Backplane using Bus 13 */

        /*****************************************/
        /* Bridge 0:0:0:0: Start */
        octeon_pcie_scan_configure (0, 0,0,0);
        prescan_setup_bridge(0,0,0,0,1);

        /* Bridge 0:1:1:0: Start */
        /* This Bridge is connected to BCM 56680 */
        octeon_pcie_scan_configure (0, 1,1,0);
        prescan_setup_bridge(0,1,1,0,2);
        /* BCM 56680 configuration starts here on 2:0:0 */
        /* Wait for some time ???? */
        udelay(2000000);
        octeon_pcie_scan_configure (0, 2,0,0);
        /* BCM 56680 configuration Ends here */
        postscan_setup_bridge(0,1,1,0,2);
        /* Bridge 0:1:1:0: End */

/* Following code has been commented out as of now.
 * This will be enabled while adding 24 GPIM support 
 */
#if 0
        /* Bridge 0:1:2:0: Start */
        /* This Bridge is connected to SiI3132 */
        octeon_pcie_scan_configure (0, 1,2,0);
        prescan_setup_bridge(0,1,2,0,3);
        /* SiI3132 configuration starts here on 3:0:0 */
        octeon_pcie_scan_configure (0, 3,0,0);
        /* SiI3132 configuration ends here */
        postscan_setup_bridge(0,1,2,0,3);
        /* Bridge 0:1:2:0: End */

        /* Bridge 0:1:3:0: Start */
        /* This Bridge is connected to other PEX8509 bridge */
        octeon_pcie_scan_configure (0, 1,3,0);
        prescan_setup_bridge(0,1,3,0,4);

        /* 2nd PEX 8509 configuration starts here on 4:0:0 */
        /* This PEX has Internal Bus : 5 */
        /* Bus 5 : has 8 ports */
        /*   Port 0 : Connected back to other PEX using bus 4 */
        /*   Port 1 : Connected to backplane using Bus 6 */
        /*   Port 2 : Connected to backplane using Bus 7 */
        /*   Port 3 : Connected to backplane using Bus 8 */
        /*   Port 4 : Connected to backplane using Bus 9 */
        /*   Port 5 : Connected to backplane using Bus 10 */
        /*   Port 6 : Connected to backplane using Bus 11 */
        /*   Port 7 : Connected to backplane using Bus 12 */

        /*****************************************/
        /*****************************************/
        /* Bridge 0:4:0:0: Start */
        octeon_pcie_scan_configure (0, 4,0,0);
        prescan_setup_bridge(0,4,0,0,5);
	  
        /* Bridge 0:5:1:0: Start */
        octeon_pcie_scan_configure (0, 5,1,0);
        prescan_setup_bridge(0,5,1,0,6);
        postscan_setup_bridge(0,5,1,0,6);
        /* Bridge 0:5:1:0: End */

        /* Bridge 0:5:2:0: Start */
        octeon_pcie_scan_configure (0, 5,2,0);
        prescan_setup_bridge(0,5,2,0,7);
        postscan_setup_bridge(0,5,2,0,7);
        /* Bridge 0:5:2:0: End */

        /* Bridge 0:5:3:0: Start */
        octeon_pcie_scan_configure (0, 5,3,0);
        prescan_setup_bridge(0,5,3,0,8);
        postscan_setup_bridge(0,5,3,0,8);
        /* Bridge 0:5:3:0: End */

        /* Bridge 0:5:4:0: Start */
        octeon_pcie_scan_configure (0, 5,4,0);
        prescan_setup_bridge(0,5,4,0,9);
        postscan_setup_bridge(0,5,4,0,9);
        /* Bridge 0:5:4:0: End */

        /* Bridge 0:5:5:0: Start */
        octeon_pcie_scan_configure (0, 5,5,0);
        prescan_setup_bridge(0,5,5,0,10);
        postscan_setup_bridge(0,5,5,0,10);
        /* Bridge 0:5:5:0: End */

        /* Bridge 0:5:6:0: Start */
        octeon_pcie_scan_configure (0, 5,6,0);
        prescan_setup_bridge(0,5,6,0,11);
        postscan_setup_bridge(0,5,6,0,11);
        /* Bridge 0:5:6:0: End */

        /* Bridge 0:5:7:0: Start */
        octeon_pcie_scan_configure (0, 5,7,0);
        prescan_setup_bridge(0,5,7,0,12);
        postscan_setup_bridge(0,5,7,0,12);
        /* Bridge 0:5:7:0: End */

        postscan_setup_bridge(0,4,0,0,12);
        /* Bridge 0:4:0:0: End */
        /*****************************************/
        /*****************************************/
        /* 2nd PEX 8509 configuration Ends here */

        postscan_setup_bridge(0,1,3,0,12);
        /* Bridge 0:1:3:0: End */

        /* Bridge 0:1:4:0: Start */
        octeon_pcie_scan_configure (0, 1,4,0);
        prescan_setup_bridge(0,1,4,0,12);
        postscan_setup_bridge(0,1,4,0,12);
        /* Bridge 0:1:4:0: End */

        postscan_setup_bridge(0,0,0,0,13);
#else
        postscan_setup_bridge(0,0,0,0,2);
#endif
        /* Bridge 0:0:0:0: End */
        /*****************************************/
        /* 1st PEX 8509 configuration Ends here */
        break;
        
    CASE_ALL_SRX550_MODELS
        i=0, j=0;
        while(i < 2) {

            for (pcie_port = 1; pcie_port < MAX_NUM_PCIE; pcie_port++) {
                sc_io_poffset =
                    CVMX_ADD_IO_SEG(cvmx_pcie_get_io_base_address(pcie_port));
                sc_mem_poffset =
                    CVMX_ADD_IO_SEG(cvmx_pcie_get_mem_base_address(pcie_port)) + 0xf0000000;;
               
                if (cvmx_pcie_rc_initialize(pcie_port)) {
		    /* if rc init fails try  couple of times, then break out.
		     * its probably bad h/w
		     */
                    if (j++ > 2) {
			goto _pcie_init_fail;
		    } else {
                        continue;
		    }
                }
                cvmx_wait_usec(1000000);
                do{
                    num_busses = do_bus(pcie_port,1);
                } while (num_busses == -1);
            }
	    /* on proto 1 with CN63XX Rev 1.2 rescan of pci bus is required 
	     * to get the gen1_gen2 fix working. H/w on proto 2 boards
	     * takes care of this.
	     */
 	    if (octeon_is_model(OCTEON_CN63XX_PASS1_X)) {
                if (!i) {
                    fix_gen1_gen2_issue();
                }
                i++;
	    } else {
		break;
	    }
        }
_pcie_init_fail:
        break;

      default:
        break;
    }
#endif
}

static unsigned char cmd_bfr[512];
static unsigned char  data_bfr[512];
static uint32_t mem0, mem1, mem_base;
static ata_prb *prb;

void
swap_prb (void)
{
    int i;
    ata_prb_entry xx;
    unsigned char t;

    for (i = 0; i < 16; i++) {
        xx.wd = prb->entry[i].wd;
        t = xx.u8.uc[3];
        xx.u8.uc[3] = xx.u8.uc[0];
        xx.u8.uc[0] = t;
        t = xx.u8.uc[2];
        xx.u8.uc[2] = xx.u8.uc[1];
        xx.u8.uc[1] = t;
        prb->entry[i].wd = xx.wd;
    }
}

void
sil3132_reset_port (uint8_t pcie_port)
{
    uint32_t data;

    /*
     * issue port reset
     */
    data = cvmx_pcie_mem_read32(pcie_port, mem_base+0x1000);
    data |= 0x01;
    cvmx_pcie_mem_write32(pcie_port, mem_base+0x1000, data);
    udelay(10000);

    /*
     * port reset is not self clearing, write to port control clear
     */
    data = cvmx_pcie_mem_read32(pcie_port, mem_base+0x1004);
    data |= 1;
    cvmx_pcie_mem_write32(pcie_port, mem_base+0x1004, data);

    /*
     * issue device reset
     */
    data = cvmx_pcie_mem_read32(pcie_port, mem_base+0x1000);
    data |= 0x02;
    cvmx_pcie_mem_write32(pcie_port, mem_base+0x1000, data);
    udelay(10000);

    /*
     * issue port initialize
     */
    data = cvmx_pcie_mem_read32(pcie_port, mem_base+0x1000);
    data |= 0x04;
    cvmx_pcie_mem_write32(pcie_port, mem_base+0x1000, data);
    udelay(10000);

    /*
     * clear interrupt status
     */
    data = cvmx_pcie_mem_read32(pcie_port, mem_base+0x1008);
    cvmx_pcie_mem_write32(pcie_port, mem_base+0x1008, data);
}

static int
sil3132_wait_command_completion (uint8_t pcie_port)
{
    uint32_t i, data = 0;

    /*
     * wait for command to complete
     */
    for (i = 0; i < 200000; i++) {
        /*
         * poll on interrupt status register
         */
        data = cvmx_pcie_mem_read32(pcie_port, mem_base+0x1008);
        if (data & 0x00030000) {
            /*
             * if command completion or command error exit
             */
            break;
        }
        udelay(100);
    }

    if (data) {
        /*
         * clear interrupt
         */
        cvmx_pcie_mem_write32(pcie_port, mem_base+0x1008, data);
        if (data & 0x00010000) {
            return 0;
        }
        if (data & 0x00020000) {
            printf("sil3132 command error\n");
            return 1;
        }
    } else {
        printf("sil3132 command timed out\n");
        return 1;
    }
    return 1;
}

static int
sil3132_do_soft_reset (uint8_t pcie_port)
{
    uint32_t data;
    int ret;

    /*
     * do soft reset to device by issue soft reset command
     */
    prb = (ata_prb *)(((uint32_t)&cmd_bfr[0] +0x0f) & ~0x0f);
    memset(prb, 0, sizeof(ata_prb));

    /*
     * soft reset command
     */
    prb->entry[0].u16.us[1] = 0x0080;

    /*
     * sil3132 is little endian machine, do swap
     */
    swap_prb();

    /*
     * mask off the msb bit of the buffer address
     */
    data = (uint32_t)prb & 0x0fffffff;

    /*
     * tel sil3132 where command is located in host memory
     */
    cvmx_pcie_mem_write32(pcie_port, mem_base+0x1c00, data);
    cvmx_pcie_mem_write32(pcie_port, mem_base+0x1c04, 0x0);

    ret = sil3132_wait_command_completion(pcie_port);

    return ret;
}

static int
sil3132_ident (uint8_t pcie_port, block_dev_desc_t *dev_desc)
{
    uint32_t data;
    int ret, i, j;
    u_int8_t *iobuf, tmp;
    hd_driveid_t *iop;
    
    printf("\n  ide %d: ", dev_desc->dev);

    dev_desc->if_type = IF_TYPE_IDE;
    /* 
     * needed for fatls/fatload cmd to work 
     */
    dev_desc->part_type = PART_TYPE_DOS;
    /*
     * do identify drive command
     */
    memset(prb, 0, sizeof(ata_prb));
    memset(&data_bfr[0], 0, sizeof(data_bfr)); /* clear data area */
    prb->entry[2].u8.uc[1] = 0xec;     /* identify drive command */
    prb->entry[2].u8.uc[2] = 0x80;     /* this is command */
    prb->entry[2].u8.uc[3] = 0x27;     /* normal ata command fis type */
    prb->entry[8].wd = ((unsigned int)&data_bfr[0]) & 0x0fffffff; /* bfr address */
    prb->entry[10].wd = 512;            /* length of buffer */
    swap_prb();
    data = (uint32_t)prb & 0x0fffffff;
    cvmx_pcie_mem_write32(pcie_port, mem_base+0x1c00, data);
    cvmx_pcie_mem_write32(pcie_port, mem_base+0x1c04, 0x0);

    ret = sil3132_wait_command_completion(pcie_port);
    if (ret) {
        printf("identify drive command failed\n");
        return ret;
    }

    iobuf = (u_int8_t *)malloc(ATA_BLOCKSIZE);
    
    if (iobuf == NULL) {
        printf(" \n ERROR!!! Cannot allocate buffer for CF info\n");
        return 1;
    }
   
    memset(iobuf,0,sizeof(iobuf));

    for(i=0; i<512;i = i+1) {
        iobuf[i] = data_bfr[i];
    } 
    	
    iop = (hd_driveid_t *)iobuf;
	    
    ident_cpy(dev_desc->revision, iop->fw_rev, sizeof(dev_desc->revision));
    ident_cpy(dev_desc->vendor, iop->model, sizeof(dev_desc->vendor));
    ident_cpy(dev_desc->product, iop->serial_no, sizeof(dev_desc->product));
    
    /*
     * firmware revision and model number have Big Endian Byte
     * order in Word. Convert both to little endian.
     */
    strswab(dev_desc->revision);
    strswab(dev_desc->vendor);
    
    dev_desc->removable = 1;
    
    /*
     * need to do some byte reversal to get lba correctly.
     */
    dev_desc->lba = ((iop->lba_capacity << 24) & 0xff000000) |
        ((iop->lba_capacity << 8) & 0xff0000) |
        ((iop->lba_capacity >> 8) & 0xff00) |
        ((iop->lba_capacity >> 24) & 0xff);
    
    /* assuming HD */
    dev_desc->type=DEV_TYPE_HARDDISK;
    dev_desc->blksz=ATA_BLOCKSIZE;
    dev_desc->lun=0; /* just to fill something in... */
    
    /* we can free iobuf now */
    free(iobuf);
	
	return 0;
}

void 
sil3132_init (uint8_t pcie_port, uint8_t sata_port)
{
    uint32_t data, i;
    int ret;
    block_dev_desc_t *ide_dev_desc_p;

    mem0 = cvmx_pcie_config_read32(pcie_port, 9 , 0, 0 ,0x10) & 0xfffffff0;
    mem1 = cvmx_pcie_config_read32(pcie_port, 9, 0, 0, 0x18) & 0xfffffff0;
    
    mem_base = mem1 + sata_port*0x2000;

    if (!si3132_reset_done) {
        /* do global reset */
        data = cvmx_pcie_mem_read32(pcie_port,mem0+0x40);
        data |= 0x80000000;
        cvmx_pcie_mem_write32(pcie_port, mem0+0x40, data);
        udelay(1000000);
        data &= ~0x80000000;
        cvmx_pcie_mem_write32(pcie_port, mem0+0x40, data);
        udelay(1000000);
        si3132_reset_done = 1;
    } 
    /*
     * check and do port reset
     */
    data = cvmx_pcie_mem_read32(pcie_port, mem_base+0x1000);
    data |= 1;
    cvmx_pcie_mem_write32(pcie_port, mem_base+0x1000, data);
    udelay(100000);

    /*
     * clear port reset
     */
    data = cvmx_pcie_mem_read32(pcie_port, mem_base+0x1004);
    data |= 0x1;
    cvmx_pcie_mem_write32(pcie_port, mem_base+0x1004, data);
    udelay(10000);

    /*
     * do device reset
     */
    data = cvmx_pcie_mem_read32(pcie_port, mem_base+0x1000);
    data |= 0x2;
    cvmx_pcie_mem_write32(pcie_port, mem_base+0x1000, data);
    udelay(100000);

    /*
     * do port initialization
     */
    data = cvmx_pcie_mem_read32(pcie_port, mem_base+0x1000);
    data |= 0x04;
    cvmx_pcie_mem_write32(pcie_port, mem_base+0x1000, data);
    udelay(100000);

    /*
     * disable transitions to both the partial and slumber
     * power management states are disabled
     */
    data = 0x00000300;
    cvmx_pcie_mem_write32(pcie_port, mem_base+0x1f00, data);

    /*
     * clear port interrupt status
     */
    data = cvmx_pcie_mem_read32(pcie_port, mem_base+0x1008);
    cvmx_pcie_mem_write32(pcie_port, mem_base+0x1008, data);

    /*
     * check for device present
     */
    data = cvmx_pcie_mem_read32(pcie_port, mem_base+0x1f04);
    if ((data & 0xf) != 0x03) {
        printf("\nWarning!!!  %s not detected \n", (sata_port == 0) ? "Compact Flash": "SSD");
        return;
    }

    /*
     * wait for port ready bit set
     */
    data = cvmx_pcie_mem_read32(pcie_port, mem_base+0x1000);
    if (!(data & 0x80000000)) {
        printf("\nWarning!!! %s not ready\n", (sata_port == 0) ? "Compact Flash": "SSD");
    }

    /*
     * clear port ready interrupt for port 0
     */
    data = 0x00040000;

    cvmx_pcie_mem_write32(pcie_port, mem_base+0x1008, data);
    udelay(100);

    /*
     * clear error counters
     */
    data = 0x80000000;
    cvmx_pcie_mem_write32(pcie_port, mem_base+0x1040, data);
    cvmx_pcie_mem_write32(pcie_port, mem_base+0x1044, data);
    cvmx_pcie_mem_write32(pcie_port, mem_base+0x1048, data);

    for (i = 1; i < 10; i++) {
        if (sil3132_do_soft_reset(pcie_port)){
            printf("sil3132 soft reset command failed count=%d\n", i);
            sil3132_reset_port(pcie_port);
            udelay(1000000);
        }
    }

    ide_dev_desc_p = &ide_dev_desc[sata_port]; 
    ide_dev_desc_p->type = DEV_TYPE_UNKNOWN;
    ide_dev_desc_p->if_type = IF_TYPE_IDE;
    ide_dev_desc_p->dev = sata_port;
    ide_dev_desc_p->part_type = PART_TYPE_UNKNOWN;
    ide_dev_desc_p->blksz = 0;
    ide_dev_desc_p->lba = 0;
    ide_dev_desc_p->block_read = ide_read;
    /* print CF details if we could get info from device */
    if (!sil3132_ident(pcie_port, ide_dev_desc_p)) {
        dev_print(ide_dev_desc_p);
    }
}



int
sil3132_ide_read (int dev, int lba, int totcnt, unsigned char *buf)
{
    uint32_t data;
    int i, ret, n;
    unsigned char *ptr;
    
    uint8_t pcie_port = 1;

    mem_base = mem1 + dev*0x2000;
    
    /*
     * tickle watchdog here for every read request
     */
    RELOAD_WATCHDOG(PAT_WATCHDOG);
    RELOAD_CPLD_WATCHDOG(PAT_WATCHDOG);

    n = 0;
    while (totcnt > 0) {
        data = lba;
        memset(prb, 0, sizeof(ata_prb));
        memset(&data_bfr[0], 0, sizeof(data_bfr)); /* clear data area */
        prb->entry[2].u8.uc[1] = 0x20;     /* read sector command */
        prb->entry[2].u8.uc[2] = 0x80;     /* this is command */
        prb->entry[2].u8.uc[3] = 0x27;     /* normal ata command fis type */
        prb->entry[3].u8.uc[3] = data&0xff; /* sector number */
        prb->entry[3].u8.uc[2] = (data>>8)&0xff; /* cyl low */
        prb->entry[3].u8.uc[1] = (data>>16)&0xff; /* cyl high */
        prb->entry[3].u8.uc[0] = 0xe0 | ((data >>24)&0x0f);  /* dev/head */
        prb->entry[5].u8.uc[0] = 0x0a;     /* device control */
        prb->entry[5].u8.uc[3] = 0x01;     /* number of sector to read */
        prb->entry[8].wd = ((unsigned int)&data_bfr[0]) & 0x0fffffff;
        prb->entry[10].wd = 512;            /* length of buffer */
        prb->entry[11].wd = 0x80000000;    /* final sgt */
        swap_prb();
        data = (uint32_t)prb & 0x0fffffff;
        cvmx_pcie_mem_write32(pcie_port, mem_base+0x1c00, data);
        cvmx_pcie_mem_write32(pcie_port, mem_base+0x1c04, 0x0);
        ret = sil3132_wait_command_completion(pcie_port);
        if (ret) {
            printf("unable to read sector %d\n", n);
            return n;
        }
        /*
         * read sector completed without error, move data
         */
        ptr = (unsigned char *)&data_bfr[0];
        for (i = 0; i < 512; i++) {
            *buf++ = *ptr++;
        }

        lba += 1;
        n += 1;
        totcnt -= 1;
    }
    return n;
}

int
sil3132_ide_write (int dev, int lba, int totcnt, unsigned char *buf)
{
    uint32_t data;
    int i, ret, n;
    unsigned char *ptr;
    
    uint8_t pcie_port = 1;

    mem_base = mem1 + dev*0x2000;

    /* copy data to static structure */
    ptr = (unsigned char *)&data_bfr[0];
    for (i = 0; i < 512; i++) {
        *ptr++ = *buf++;
    }

    /*
     * tickle watchdog here for every write request
     */
    RELOAD_WATCHDOG(PAT_WATCHDOG);
    RELOAD_CPLD_WATCHDOG(PAT_WATCHDOG);

    n = 0;
    while (totcnt > 0) {
        data = lba;
        memset(prb, 0, sizeof(ata_prb));
        prb->entry[2].u8.uc[1] = 0x30;     /* write sector command */
        prb->entry[2].u8.uc[2] = 0x80;     /* this is command */
        prb->entry[2].u8.uc[3] = 0x27;     /* normal ata command fis type */
        prb->entry[3].u8.uc[3] = data&0xff; /* sector number */
        prb->entry[3].u8.uc[2] = (data>>8)&0xff; /* cyl low */
        prb->entry[3].u8.uc[1] = (data>>16)&0xff; /* cyl high */
        prb->entry[3].u8.uc[0] = 0xe0 | ((data >>24)&0x0f);  /* dev/head */
        prb->entry[5].u8.uc[0] = 0x0a;     /* device control */
        prb->entry[5].u8.uc[3] = 0x01;     /* number of sector to read */
        prb->entry[8].wd = ((unsigned int)&data_bfr[0]) & 0x0fffffff;
        prb->entry[10].wd = 512;            /* length of buffer */
        prb->entry[11].wd = 0x80000000;    /* final sgt */
        swap_prb();
        data = (uint32_t)prb & 0x0fffffff;
        cvmx_pcie_mem_write32(pcie_port, mem_base+0x1c00, data);
        cvmx_pcie_mem_write32(pcie_port, mem_base+0x1c04, 0x0);
        ret = sil3132_wait_command_completion(pcie_port);
        if (ret) {
            printf("unable to write sector %d\n", n);
            return n;
        }

        lba += 1;
        n += 1;
        totcnt -= 1;
    }
    return n;
}

int 
do_pciemd (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    DECLARE_GLOBAL_DATA_PTR;
    uint32_t addr;
    uint32_t length = 4;
    int i,j = 0;
    int pci_value;
    int port;

#ifdef CONFIG_JSRXNLE
    switch (gd->board_desc.board_type)
    {
        CASE_ALL_SRX240_MODELS
            port = 1;
            break;

        CASE_ALL_SRX650_MODELS
            port = 0;
            break;

        default:
            printf("Command not supported on this board\n");
            return 0;
    }

    if ((argc < 2)) 
    {
        printf ("Usage:\n%s\n", cmdtp->usage);
        return 1;
    }

    addr = simple_strtoull(argv[1], NULL, 16);

    if (argc > 2)
      length = simple_strtoul(argv[2], NULL, 16);

    printf("%08lx : ",addr);
    for(i = 0; i < length; i+=4)
    {

        pci_value = cvmx_pcie_mem_read32(port,addr + i);
        printf("%08lx ",pci_value);
        ++j;
        if((j == 4) && ((i + 4) < length))
        {
              printf("\n%08lx : ",addr + i + 4);
              j = 0;
         }
    }
    printf("\n");
    return(0);
#endif
}

int 
do_pciemw (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    DECLARE_GLOBAL_DATA_PTR;
    uint32_t addr;
    int pci_value;
    int port;

#ifdef CONFIG_JSRXNLE
    switch (gd->board_desc.board_type)
    {
        CASE_ALL_SRX240_MODELS
            port = 1;
            break;

        CASE_ALL_SRX650_MODELS
            port = 0;
            break;

        default:
            printf("Command not supported on this board\n");
            return 0;
    }
    if ((argc != 3)) 
    {
        printf ("Usage:\n%s\n", cmdtp->usage);
        return 1;
    }

    addr = simple_strtoull(argv[1], NULL, 16);

    pci_value = simple_strtoul(argv[2], NULL, 16);

    cvmx_pcie_mem_write32(port,addr,pci_value);
#endif
    return(0);
}

int 
pcie_reset (int port)
{
    cvmx_ciu_soft_prst_t ciu_soft_prst;

    if(port)
    {
        ciu_soft_prst.u64 = cvmx_read_csr(CVMX_CIU_SOFT_PRST1);
        ciu_soft_prst.s.soft_prst = 1;
        cvmx_write_csr(CVMX_CIU_SOFT_PRST1, ciu_soft_prst.u64);
    }
    else
    {
        ciu_soft_prst.u64 = cvmx_read_csr(CVMX_CIU_SOFT_PRST);
        ciu_soft_prst.s.soft_prst = 1;
        cvmx_write_csr(CVMX_CIU_SOFT_PRST, ciu_soft_prst.u64);
    }

    return 0;
}
int 
do_pcie_reset (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    DECLARE_GLOBAL_DATA_PTR;
    int     rcode = 0;

#ifdef CONFIG_JSRXNLE
    switch (gd->board_desc.board_type)
    {
        CASE_ALL_SRX240_MODELS
            rcode = pcie_reset(0);
            rcode = pcie_reset(1);
            break;

        CASE_ALL_SRX650_MODELS
            rcode = pcie_reset(0);
            rcode = octeon_network_hw_shutdown();
            break;

        default:
            printf("Command not supported on this board\n");
            return 0;
    }
#endif
    return rcode;
}

U_BOOT_CMD(
      pciemd,     3,     1,      do_pciemd,
      "pciemd      - pcie memory display\n",
      " address [# of objects]\n    - pcie memory display\n"
);

U_BOOT_CMD(
      pciemw,     3,     1,      do_pciemw,
      "pciemw      - pcie memory write\n",
      " address value\n    - pcie memory write\n"
);

U_BOOT_CMD(
      pciereset, 2, 1,	do_pcie_reset,
      "pciereset      - do PCIE reset\n",
      "pciereset      - do PCIE reset\n"
);

#endif
#endif /* CONFIG_PCI */
