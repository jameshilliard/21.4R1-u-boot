/* ==========================================================================
 * $File: //dwh/usb_iip/dev/software/otg_ipmate/linux/drivers/dwc_otg_cil.c $
 * $Revision: 1.9 $
 * $Date: 2006/08/01 18:48:09 $
 * $Change: 541783 $
 *
 * Synopsys HS OTG Linux Software Driver and documentation (hereinafter,
 * "Software") is an Unsupported proprietary work of Synopsys, Inc. unless
 * otherwise expressly agreed to in writing between Synopsys and you.
 *
 * The Software IS NOT an item of Licensed Software or Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Product
 * with Synopsys or any supplement thereto. You are permitted to use and
 * redistribute this Software in source and binary forms, with or without
 * modification, provided that redistributions of source code must retain this
 * notice. You may not view, use, disclose, copy or distribute this file or
 * any information contained herein except pursuant to this license grant from
 * Synopsys. If you do not agree with this notice, including the disclaimer
 * below, then you are not authorized to use the Software.
 *
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * ========================================================================== */

/** @file
 *
 * The Core Interface Layer provides basic services for accessing and
 * managing the DWC_otg hardware. These services are used by both the
 * Host Controller Driver and the Peripheral Controller Driver.
 *
 * The CIL manages the memory map for the core so that the HCD and PCD
 * don't have to do this separately. It also handles basic tasks like
 * reading/writing the registers and data FIFOs in the controller.
 * Some of the data access functions provide encapsulation of several
 * operations required to perform a task, such as writing multiple
 * registers to start a transfer. Finally, the CIL performs basic
 * services that are not specific to either the host or device modes
 * of operation. These services include management of the OTG Host
 * Negotiation Protocol (HNP) and Session Request Protocol (SRP). A
 * Diagnostic API is also provided to allow testing of the controller
 * hardware.
 *
 * The Core Interface Layer has the following requirements:
 * - Provides basic controller operations.
 * - Minimal use of OS services.
 * - The OS services used will be abstracted by using inline functions
 *   or macros.
 *
 */

#include <common.h>

#if defined (CONFIG_USB_DWC_OTG)

#include <malloc.h>

#include "dwc_otg_regs.h"
#include "dwc_otg_driver.h"
#include "dwc_otg_cil.h"

extern dwc_otg_core_params_t dwc_otg_module_params;
extern dwc_otg_core_if_t core_if_struct[DWC_OTG_UNIT_MAX];
extern dwc_otg_host_if_t host_if_struct[DWC_OTG_UNIT_MAX];


/**
 * This function is called to initialize the DWC_otg CSR data
 * structures.  The register addresses in the device and host
 * structures are initialized from the base address supplied by the
 * caller.  The calling function must make the OS calls to get the
 * base address of the DWC_otg controller registers.  The core_params
 * argument holds the parameters that specify how the core should be
 * configured.
 *
 * @param[in] _reg_base_addr Base address of DWC_otg core registers
 * @param[in] _core_params Pointer to the core configuration parameters
 *
 */
/**
 * This function is called to initialize the DWC_otg CSR data
 * structures.   The core_params
 * argument holds the parameters that specify how the core should be
 * configured.
 *
 * @param[in] _core_params Pointer to the core configuration parameters
 *
 */
dwc_otg_core_if_t *dwc_otg_cil_init(int unit, const uint32_t *_reg_base_addr)
{
        dwc_otg_core_params_t *_core_params = &dwc_otg_module_params;
        dwc_otg_core_if_t *core_if;
        dwc_otg_host_if_t *host_if;
        uint8_t *reg_base = (uint8_t *)_reg_base_addr;
        int i = 0;

        dbg("%s: unit %d\n", __FUNCTION__, unit);

        core_if = &core_if_struct[unit];
        host_if = &host_if_struct[unit];

        core_if->core_params = _core_params;
        core_if->unit = unit;

	/*** Initialise register base pointers for core ***/

        core_if->core_global_regs = (dwc_otg_core_global_regs_t *)reg_base;

	/*** Initialise register base pointers for host ***/

        host_if->host_global_regs = (dwc_otg_host_global_regs_t *)
                (reg_base + DWC_OTG_HOST_GLOBAL_REG_OFFSET);
        host_if->hprt0 = (uint32_t*)(reg_base + DWC_OTG_HOST_PORT_REGS_OFFSET);
        for (i=0; i<MAX_EPS_CHANNELS; i++) {
                host_if->hc_regs[i] = (dwc_otg_hc_regs_t *)
                        (reg_base + DWC_OTG_HOST_CHAN_REGS_OFFSET +
                         (i * DWC_OTG_CHAN_REGS_OFFSET));
        }
        host_if->num_host_channels = MAX_EPS_CHANNELS;
        core_if->host_if = host_if;

        for (i=0; i<MAX_EPS_CHANNELS; i++) {
                core_if->data_fifo[i] =
                        (uint32_t *)(reg_base + DWC_OTG_DATA_FIFO_OFFSET +
                                     (i * DWC_OTG_DATA_FIFO_SIZE));
                dbg("data_fifo[%d]=%p\n", i, core_if->data_fifo[i]);
        }

	core_if->pcgcctl = (uint32_t*)(reg_base + DWC_OTG_PCGCCTL_OFFSET);

	/*
	 * Store the contents of the hardware configuration registers here for
	 * easy access later.
	 */
        core_if->hwcfg1.d32 = dwc_read_reg32(&core_if->core_global_regs->ghwcfg1);
        core_if->hwcfg2.d32 = dwc_read_reg32(&core_if->core_global_regs->ghwcfg2);
        core_if->hwcfg3.d32 = dwc_read_reg32(&core_if->core_global_regs->ghwcfg3);
        core_if->hwcfg4.d32 = dwc_read_reg32(&core_if->core_global_regs->ghwcfg4);

	dbg("hwcfg1=%08x\n",core_if->hwcfg1.d32);
	dbg("hwcfg2=%08x\n",core_if->hwcfg2.d32);
	dbg("hwcfg3=%08x\n",core_if->hwcfg3.d32);
	dbg("hwcfg4=%08x\n",core_if->hwcfg4.d32);

	dbg("op_mode=%0x\n",core_if->hwcfg2.b.op_mode);
	dbg("arch=%0x\n",core_if->hwcfg2.b.architecture);
	dbg("num_dev_ep=%d\n",core_if->hwcfg2.b.num_dev_ep);
	dbg("num_host_chan=%d\n",core_if->hwcfg2.b.num_host_chan);
	dbg("nonperio_tx_q_depth=0x%0x\n",core_if->hwcfg2.b.nonperio_tx_q_depth);
	dbg("host_perio_tx_q_depth=0x%0x\n",core_if->hwcfg2.b.host_perio_tx_q_depth);
	dbg("dev_token_q_depth=0x%0x\n",core_if->hwcfg2.b.dev_token_q_depth);
	
	dbg("Total FIFO SZ=%d\n", core_if->hwcfg3.b.dfifo_depth);
	dbg("xfer_size_cntr_width=%0x\n", core_if->hwcfg3.b.xfer_size_cntr_width);

	/*
	 * Set the SRP sucess bit for FS-I2c
	 */
	core_if->srp_success = 0;
	core_if->srp_timer_started = 0;
	
        return core_if;
}

/**
 * This function frees the structures allocated by dwc_otg_cil_init().
 *
 * @param[in] _core_if The core interface pointer returned from
 * dwc_otg_cil_init().
 *
 */
void dwc_otg_cil_remove( dwc_otg_core_if_t *_core_if )
{
        /* Disable all interrupts */
        dwc_modify_reg32( &_core_if->core_global_regs->gahbcfg, 1, 0);
        dwc_write_reg32( &_core_if->core_global_regs->gintmsk, 0);

        if ( _core_if->host_if ) {
                free( _core_if->host_if );
        }
        free( _core_if );
}

/**
 * This function enables the controller's Global Interrupt in the AHB Config
 * register.
 *
 * @param[in] _core_if Programming view of DWC_otg controller.
 */
extern void dwc_otg_enable_global_interrupts( dwc_otg_core_if_t *_core_if )
{
        gahbcfg_data_t ahbcfg = { .d32 = 0};
        ahbcfg.b.glblintrmsk = 1; /* Enable interrupts */
        dwc_modify_reg32(&_core_if->core_global_regs->gahbcfg, 0, ahbcfg.d32);
}

/**
 * This function disables the controller's Global Interrupt in the AHB Config
 * register.
 *
 * @param[in] _core_if Programming view of DWC_otg controller.
 */
extern void dwc_otg_disable_global_interrupts( dwc_otg_core_if_t *_core_if )
{
        gahbcfg_data_t ahbcfg = { .d32 = 0};
        ahbcfg.b.glblintrmsk = 1; /* Enable interrupts */
        dwc_modify_reg32(&_core_if->core_global_regs->gahbcfg, ahbcfg.d32, 0);
}

/**
 * This function initializes the commmon interrupts, used in both
 * device and host modes.
 *
 * @param[in] _core_if Programming view of the DWC_otg controller
 *
 */
static void dwc_otg_enable_common_interrupts(dwc_otg_core_if_t *_core_if)
{
        dwc_otg_core_global_regs_t *global_regs =
                _core_if->core_global_regs;
        gintmsk_data_t intr_mask = { .d32 = 0};
        /* Clear any pending OTG Interrupts */
        dwc_write_reg32( &global_regs->gotgint, 0xFFFFFFFF);
        /* Clear any pending interrupts */
        dwc_write_reg32( &global_regs->gintsts, 0xFFFFFFFF);
        /*
         * Enable the interrupts in the GINTMSK.
         */
        intr_mask.b.modemismatch = 1;
        intr_mask.b.otgintr = 1;
        if (!_core_if->dma_enable) {
                intr_mask.b.rxstsqlvl = 1;
        }
        intr_mask.b.conidstschng = 1;
        intr_mask.b.wkupintr = 1;
        intr_mask.b.disconnect = 1;
        intr_mask.b.usbsuspend = 1;
        intr_mask.b.sessreqintr = 1;
        dwc_write_reg32( &global_regs->gintmsk, intr_mask.d32);
}


/**
 * This function enables the Host mode interrupts.
 *
 * @param _core_if Programming view of DWC_otg controller
 */
void dwc_otg_enable_host_interrupts(dwc_otg_core_if_t *_core_if)
{
        dwc_otg_core_global_regs_t *global_regs = _core_if->core_global_regs;
        gintmsk_data_t intr_mask = {.d32 = 0};

        dbg("%s()\n", __func__);

        /* Disable all interrupts. */
        dwc_write_reg32(&global_regs->gintmsk, 0);

        /* Clear any pending interrupts. */
        dwc_write_reg32(&global_regs->gintsts, 0xFFFFFFFF);

        /* Enable the common interrupts */
        dwc_otg_enable_common_interrupts(_core_if);

        /*
         * Enable host mode interrupts without disturbing common
         * interrupts.
         */
        intr_mask.b.sofintr = 1;
        intr_mask.b.portintr = 1;
        intr_mask.b.hcintr = 1;

        dwc_modify_reg32(&global_regs->gintmsk, intr_mask.d32, intr_mask.d32);
}


/**
 * Initializes the FSLSPClkSel field of the HCFG register depending on the PHY
 * type.
 */
static void init_fslspclksel(dwc_otg_core_if_t *_core_if)
{
        uint32_t        val;
        hcfg_data_t     hcfg;

        if (((_core_if->hwcfg2.b.hs_phy_type == 2) &&
             (_core_if->hwcfg2.b.fs_phy_type == 1) &&
             (_core_if->core_params->ulpi_fs_ls)) ||
            (_core_if->core_params->phy_type == DWC_PHY_TYPE_PARAM_FS))
        {
                /* Full speed PHY */
                val = DWC_HCFG_48_MHZ;
        } else {
                /* High speed PHY running at full speed or high speed */
                val = DWC_HCFG_30_60_MHZ;
        }

        dbg( "Initializing HCFG.FSLSPClkSel to 0x%1x\n", val);
        hcfg.d32 = dwc_read_reg32(&_core_if->host_if->host_global_regs->hcfg);
        hcfg.b.fslspclksel = val;
        dwc_write_reg32(&_core_if->host_if->host_global_regs->hcfg, hcfg.d32);
}


/**
 * This function initializes the DWC_otg controller registers and
 * prepares the core for device mode or host mode operation.
 *
 * @param _core_if Programming view of the DWC_otg controller
 *
 */
void dwc_otg_core_init(dwc_otg_core_if_t *_core_if)
{
        dwc_otg_core_global_regs_t *global_regs =
                _core_if->core_global_regs;
        gahbcfg_data_t ahbcfg = { .d32 = 0};
        gusbcfg_data_t usbcfg = { .d32 = 0 };

        dbg("dwc_otg_core_init(%p)\n",_core_if);

        /* Reset the Controller */
        dwc_otg_core_reset( _core_if );

        /* Initialize parameters from Hardware configuration registers. */

        _core_if->total_fifo_size = _core_if->hwcfg3.b.dfifo_depth;
        _core_if->rx_fifo_size =
                dwc_read_reg32( &global_regs->grxfsiz);
        _core_if->nperio_tx_fifo_size =
                dwc_read_reg32( &global_regs->gnptxfsiz) >> 16;

        dbg( "Total FIFO SZ=%d\n", _core_if->total_fifo_size);
        dbg( "Rx FIFO SZ=%d\n", _core_if->rx_fifo_size);
        dbg( "NP Tx FIFO SZ=%d\n", _core_if->nperio_tx_fifo_size);

	/* This programming sequence needs to happen in FS mode before any other
	 * programming occurs */
	if ((_core_if->core_params->speed == DWC_SPEED_PARAM_FULL) &&
	    (_core_if->core_params->phy_type == DWC_PHY_TYPE_PARAM_FS)) {
		/* If FS mode with FS PHY */

		/* core_init() is now called on every switch so only call the
		 * following for the first time through. */
		if (!_core_if->phy_init_done) {
			_core_if->phy_init_done = 1;
			dbg("FS_PHY detected\n");
			usbcfg.d32 = dwc_read_reg32(&global_regs->gusbcfg);
			usbcfg.b.physel = 1;
			dwc_write_reg32 (&global_regs->gusbcfg, usbcfg.d32);

			/* Reset after a PHY select */
			dwc_otg_core_reset( _core_if );
		}

		/* Program HCFG.FSLSPclkSel to 48Mhz in FS */
		init_fslspclksel(_core_if);

	} /* endif speed == DWC_SPEED_PARAM_FULL */

        /* Program the GAHBCFG Register.*/
        switch (_core_if->hwcfg2.b.architecture){

        case DWC_SLAVE_ONLY_ARCH:
                dbg("Slave Only Mode\n");
                ahbcfg.b.nptxfemplvl = DWC_GAHBCFG_TXFEMPTYLVL_HALFEMPTY;
                ahbcfg.b.ptxfemplvl = DWC_GAHBCFG_TXFEMPTYLVL_HALFEMPTY;
		_core_if->dma_enable = 0;
                break;

        case DWC_EXT_DMA_ARCH:
                dbg("External DMA Mode\n");
                ahbcfg.b.hburstlen = _core_if->core_params->dma_burst_size;
		_core_if->dma_enable = (_core_if->core_params->dma_enable != 0);
                break;

        case DWC_INT_DMA_ARCH:
                dbg("Internal DMA Mode\n");
                ahbcfg.b.hburstlen = DWC_GAHBCFG_INT_DMA_BURST_INCR;
		_core_if->dma_enable = (_core_if->core_params->dma_enable != 0);
                break;

        }
	ahbcfg.b.dmaenable = _core_if->dma_enable;
        dwc_write_reg32(&global_regs->gahbcfg, ahbcfg.d32);

        /*
         * Program the GUSBCFG register.
         */
	usbcfg.d32 = dwc_read_reg32( &global_regs->gusbcfg );

	dbg("HWCFG2 OTGMODE 0x%x\n", _core_if->hwcfg2.b.op_mode);
        switch (_core_if->hwcfg2.b.op_mode) {
        case DWC_MODE_HNP_SRP_CAPABLE:
                usbcfg.b.hnpcap = (_core_if->core_params->otg_cap ==
				   DWC_OTG_CAP_PARAM_HNP_SRP_CAPABLE);
                usbcfg.b.srpcap = (_core_if->core_params->otg_cap !=
				   DWC_OTG_CAP_PARAM_NO_HNP_SRP_CAPABLE);
                break;

        case DWC_MODE_SRP_ONLY_CAPABLE:
		usbcfg.b.hnpcap = 0;
                usbcfg.b.srpcap = (_core_if->core_params->otg_cap !=
				   DWC_OTG_CAP_PARAM_NO_HNP_SRP_CAPABLE);
                break;

        case DWC_MODE_NO_HNP_SRP_CAPABLE:
                usbcfg.b.hnpcap = 0;
		usbcfg.b.srpcap = 0;
                break;

        case DWC_MODE_SRP_CAPABLE_DEVICE:
		usbcfg.b.hnpcap = 0;
                usbcfg.b.srpcap = (_core_if->core_params->otg_cap !=
				   DWC_OTG_CAP_PARAM_NO_HNP_SRP_CAPABLE);
                break;

        case DWC_MODE_NO_SRP_CAPABLE_DEVICE:
		usbcfg.b.hnpcap = 0;
                usbcfg.b.srpcap = 0;
                break;

        case DWC_MODE_SRP_CAPABLE_HOST:
		usbcfg.b.hnpcap = 0;
                usbcfg.b.srpcap = (_core_if->core_params->otg_cap !=
				   DWC_OTG_CAP_PARAM_NO_HNP_SRP_CAPABLE);
                break;

        case DWC_MODE_NO_SRP_CAPABLE_HOST:
		usbcfg.b.hnpcap = 0;
                usbcfg.b.srpcap = 0;
                break;
        }

        dwc_write_reg32( &global_regs->gusbcfg, usbcfg.d32);

        /* Enable common interrupts */
        dwc_otg_enable_common_interrupts( _core_if );

        /* Do host intialization */
	dbg("Host Mode\n" );
	_core_if->op_state = A_HOST;
}


/**
 * This function disables the Host Mode interrupts.
 *
 * @param _core_if Programming view of DWC_otg controller
 */
void dwc_otg_disable_host_interrupts(dwc_otg_core_if_t *_core_if)
{
        dwc_otg_core_global_regs_t *global_regs =
		_core_if->core_global_regs;
	gintmsk_data_t intr_mask = {.d32 = 0};

        dbg("%s()\n", __func__);

	/*
	 * Disable host mode interrupts without disturbing common
	 * interrupts.
	 */
	intr_mask.b.sofintr = 1;
	intr_mask.b.portintr = 1;
	intr_mask.b.hcintr = 1;
        intr_mask.b.ptxfempty = 1;
	intr_mask.b.nptxfempty = 1;

        dwc_modify_reg32(&global_regs->gintmsk, intr_mask.d32, 0);
}

/**
 * The FIFOs are established based on a default percentage of the
 * total FIFO depth. This function converts the percentage into the
 * proper setting.
 *
 */
static inline uint32_t fifo_percentage(uint16_t total_fifo_size, int32_t percentage)
{
    return (((total_fifo_size*percentage)/100) & (-1<<3)); /* 16-byte aligned */
}

void dwc_otg_core_host_init(dwc_otg_core_if_t *_core_if)
{
        dwc_otg_core_global_regs_t *global_regs = _core_if->core_global_regs;
	dwc_otg_host_if_t 	*host_if = _core_if->host_if;
        dwc_otg_core_params_t 	*params = _core_if->core_params;
	hprt0_data_t 		hprt0 = {.d32 = 0};
        fifosize_data_t 	nptxfifosize;
        fifosize_data_t 	ptxfifosize;
	int			i;
	hcchar_data_t		hcchar;
	hcfg_data_t		hcfg;
	dwc_otg_hc_regs_t	*hc_regs;
	int			num_channels;
	gotgctl_data_t  gotgctl = {.d32 = 0};

	dbg("%s(%p)\n", __FUNCTION__, _core_if);

        /* Restart the Phy Clock */
        dwc_write_reg32(_core_if->pcgcctl, 0);

	/* Initialize Host Configuration Register */
	init_fslspclksel(_core_if);
	if (_core_if->core_params->speed == DWC_SPEED_PARAM_FULL) {
		hcfg.d32 = dwc_read_reg32(&host_if->host_global_regs->hcfg);
		hcfg.b.fslssupp = 1;
		dwc_write_reg32(&host_if->host_global_regs->hcfg, hcfg.d32);
	}

	/* Configure data FIFO sizes */
	if (_core_if->hwcfg2.b.dynamic_fifo && params->enable_dynamic_fifo) {
                dbg("Total FIFO Size=%d\n", _core_if->total_fifo_size);
                dbg("Rx FIFO Size=%d\n", params->host_rx_fifo_size);
                dbg("NP Tx FIFO Size=%d\n", params->host_nperio_tx_fifo_size);
                dbg("P Tx FIFO Size=%d\n", params->host_perio_tx_fifo_size);

		/* Rx FIFO */
		dbg("initial grxfsiz=%08x\n", dwc_read_reg32(&global_regs->grxfsiz));
		dwc_write_reg32(&global_regs->grxfsiz,
                                fifo_percentage(_core_if->total_fifo_size,
                                                dwc_param_host_rx_fifo_size_percentage));
		dbg("new grxfsiz=%08x\n", dwc_read_reg32(&global_regs->grxfsiz));

		/* Non-periodic Tx FIFO */
                dbg("initial gnptxfsiz=%08x\n", dwc_read_reg32(&global_regs->gnptxfsiz));
                nptxfifosize.b.depth  = fifo_percentage(_core_if->total_fifo_size,
                                                        dwc_param_host_nperio_tx_fifo_size_percentage);
                nptxfifosize.b.startaddr = dwc_read_reg32(&global_regs->grxfsiz);
                dwc_write_reg32(&global_regs->gnptxfsiz, nptxfifosize.d32);
                dbg("new gnptxfsiz=%08x\n", dwc_read_reg32(&global_regs->gnptxfsiz));
		
		/* Periodic Tx FIFO */
                dbg("initial hptxfsiz=%08x\n", dwc_read_reg32(&global_regs->hptxfsiz));
                ptxfifosize.b.depth  = _core_if->total_fifo_size
                    - dwc_read_reg32(&global_regs->grxfsiz) - nptxfifosize.b.depth;
                ptxfifosize.b.startaddr = nptxfifosize.b.startaddr + nptxfifosize.b.depth;
                dwc_write_reg32(&global_regs->hptxfsiz, ptxfifosize.d32);
                dbg("new hptxfsiz=%08x\n", dwc_read_reg32(&global_regs->hptxfsiz));
	}

        /* Clear Host Set HNP Enable in the OTG Control Register */
        gotgctl.b.hstsethnpen = 1;
        dwc_modify_reg32( &global_regs->gotgctl, gotgctl.d32, 0);

	/* Make sure the FIFOs are flushed. */
	dwc_otg_flush_tx_fifo(_core_if, 0x10 /* all Tx FIFOs */);
	dwc_otg_flush_rx_fifo(_core_if);

	/* Flush out any leftover queued requests. */
	num_channels = _core_if->core_params->host_channels;
	for (i = 0; i < num_channels; i++) {
		dbg("hc_reg[%d]->hcchar=%p\n", i, &_core_if->host_if->hc_regs[i]->hcchar);
		hc_regs = _core_if->host_if->hc_regs[i];
		hcchar.d32 = dwc_read_reg32(&hc_regs->hcchar);
		hcchar.b.chen = 0;
		hcchar.b.chdis = 1;
		hcchar.b.epdir = 0;
		dwc_write_reg32(&hc_regs->hcchar, hcchar.d32);
	}
	
	/* Halt all channels to put them into a known state. */
	for (i = 0; i < num_channels; i++) {
		int count = 0;
		hc_regs = _core_if->host_if->hc_regs[i];
		hcchar.d32 = dwc_read_reg32(&hc_regs->hcchar);
		hcchar.b.chen = 1;
		hcchar.b.chdis = 1;
		hcchar.b.epdir = 0;
		dwc_write_reg32(&hc_regs->hcchar, hcchar.d32);
		dbg("%s: Halt channel %d\n",__FUNCTION__, i);
		do {
			hcchar.d32 = dwc_read_reg32(&hc_regs->hcchar);
			if (++count > 1000) {
				dbg("%s: Unable to clear halt on channel %d\n",
					  __FUNCTION__, i);
				break;
			}
		} while (hcchar.b.chen);
	}

	/* Turn on the vbus power. */
        dbg("Init: Port Power? op_state=%d\n", _core_if->op_state);
        if (_core_if->op_state == A_HOST){
                hprt0.d32 = dwc_otg_read_hprt0(_core_if);
                dbg("Init: Power Port (%d)\n", hprt0.b.prtpwr);
                if (hprt0.b.prtpwr == 0 ) {
                        hprt0.b.prtpwr = 1;
                        dwc_write_reg32(host_if->hprt0, hprt0.d32);
                }
        }

        dwc_otg_enable_host_interrupts( _core_if );
}


/**
 * Prepares a host channel for transferring packets to/from a specific
 * endpoint. The HCCHARn register is set up with the characteristics specified
 * in _hc. Host channel interrupts that may need to be serviced while this
 * transfer is in progress are enabled.
 *
 * @param _core_if Programming view of DWC_otg controller
 * @param _hc Information needed to initialize the host channel
 */
void dwc_otg_hc_init(dwc_otg_core_if_t *_core_if, dwc_hc_t *_hc)
{
	uint32_t intr_enable;
	hcintmsk_data_t hc_intr_mask;
	gintmsk_data_t gintmsk = {.d32 = 0};
	hcchar_data_t hcchar;
        hcsplt_data_t hcsplt;

	uint8_t hc_num = _hc->hc_num;
	dwc_otg_host_if_t *host_if = _core_if->host_if;
	dwc_otg_hc_regs_t *hc_regs = host_if->hc_regs[hc_num];

	/* Clear old interrupt conditions for this host channel. */
	hc_intr_mask.d32 = 0xFFFFFFFF;
	hc_intr_mask.b.reserved = 0;
	dwc_write_reg32(&hc_regs->hcint, hc_intr_mask.d32);

	/* Enable channel interrupts required for this transfer. */
	hc_intr_mask.d32 = 0;
	hc_intr_mask.b.chhltd = 1;
	if (_core_if->dma_enable) {
		hc_intr_mask.b.ahberr = 1;
		if (_hc->error_state && !_hc->do_split &&
		    _hc->ep_type != DWC_OTG_EP_TYPE_ISOC) {
			hc_intr_mask.b.ack = 1;
			if (_hc->ep_is_in) {
				hc_intr_mask.b.datatglerr = 1;
				if (_hc->ep_type != DWC_OTG_EP_TYPE_INTR) {
					hc_intr_mask.b.nak = 1;
				}
			}
		}
	} else {
		switch (_hc->ep_type) {
		case DWC_OTG_EP_TYPE_CONTROL:
		case DWC_OTG_EP_TYPE_BULK:
			hc_intr_mask.b.xfercompl = 1;
			hc_intr_mask.b.stall = 1;
			hc_intr_mask.b.xacterr = 1;
			hc_intr_mask.b.datatglerr = 1;
			if (_hc->ep_is_in) {
				hc_intr_mask.b.bblerr = 1;
			} else {
				hc_intr_mask.b.nak = 1;
				hc_intr_mask.b.nyet = 1;
				if (_hc->do_ping) {
					hc_intr_mask.b.ack = 1;
				}
			}

			if (_hc->do_split) {
				hc_intr_mask.b.nak = 1;
				if (_hc->complete_split) {
					hc_intr_mask.b.nyet = 1;
				}
				else {
					hc_intr_mask.b.ack = 1;
				}
			}

			if (_hc->error_state) {
				hc_intr_mask.b.ack = 1;
			}
			break;
		case DWC_OTG_EP_TYPE_INTR:
                        dbg("%s: Intr EP type not supported\n");
			break;
		case DWC_OTG_EP_TYPE_ISOC:
                        dbg("%s: Isoc EP type not supported\n");
			break;
		}
	}
	dwc_write_reg32(&hc_regs->hcintmsk, hc_intr_mask.d32);

	/* Enable the top level host channel interrupt. */
	intr_enable = (1 << hc_num);
	dwc_modify_reg32(&host_if->host_global_regs->haintmsk, 0, intr_enable);

	/* Make sure host channel interrupts are enabled. */
	gintmsk.b.hcintr = 1;
	dwc_modify_reg32(&_core_if->core_global_regs->gintmsk, 0, gintmsk.d32);
	
	/*
	 * Program the HCCHARn register with the endpoint characteristics for
	 * the current transfer.
	 */
	hcchar.d32 = 0;
	hcchar.b.devaddr = _hc->dev_addr;
	hcchar.b.epnum = _hc->ep_num;
	hcchar.b.epdir = _hc->ep_is_in;
	hcchar.b.lspddev = (_hc->speed == DWC_OTG_EP_SPEED_LOW);
	hcchar.b.eptype = _hc->ep_type;
	hcchar.b.mps = _hc->max_packet;

	dwc_write_reg32(&host_if->hc_regs[hc_num]->hcchar, hcchar.d32);

	info("%s: Channel %d\n", __func__, _hc->hc_num);
	info("  Dev Addr: %d\n", hcchar.b.devaddr);
	info("  Ep Num: %d\n", hcchar.b.epnum);
	info("  Is In: %d\n", hcchar.b.epdir);
	info("  Is Low Speed: %d\n", hcchar.b.lspddev);
	info("  Ep Type: %d\n", hcchar.b.eptype);
	info("  Max Pkt: %d\n", hcchar.b.mps);
	info("  Multi Cnt: %d\n", hcchar.b.multicnt);

        /*
         * Program the HCSPLIT register for SPLITs
         */
        hcsplt.d32 = 0;
        if (_hc->do_split) {
                dbg("Programming HC %d with split --> %s\n", _hc->hc_num,
                           _hc->complete_split ? "CSPLIT" : "SSPLIT");
                hcsplt.b.compsplt = _hc->complete_split;
                hcsplt.b.xactpos = _hc->xact_pos;
                hcsplt.b.hubaddr = _hc->hub_addr;
                hcsplt.b.prtaddr = _hc->port_addr;
                dbg("   comp split %d\n", _hc->complete_split);
                dbg("   xact pos %d\n", _hc->xact_pos);
                dbg("   hub addr %d\n", _hc->hub_addr);
                dbg("   port addr %d\n", _hc->port_addr);
                dbg("   is_in %d\n", _hc->ep_is_in);
                dbg("   Max Pkt: %d\n", hcchar.b.mps);
                dbg("   xferlen: %d\n", _hc->xfer_len);
        }
        dwc_write_reg32(&host_if->hc_regs[hc_num]->hcsplt, hcsplt.d32);
}


/**
 * Attempts to halt a host channel. This function should only be called in
 * Slave mode or to abort a transfer in either Slave mode or DMA mode. Under
 * normal circumstances in DMA mode, the controller halts the channel when the
 * transfer is complete or a condition occurs that requires application
 * intervention.
 *
 * In slave mode, checks for a free request queue entry, then sets the Channel
 * Enable and Channel Disable bits of the Host Channel Characteristics
 * register of the specified channel to intiate the halt. If there is no free
 * request queue entry, sets only the Channel Disable bit of the HCCHARn
 * register to flush requests for this channel. In the latter case, sets a
 * flag to indicate that the host channel needs to be halted when a request
 * queue slot is open.
 *
 * In DMA mode, always sets the Channel Enable and Channel Disable bits of the
 * HCCHARn register. The controller ensures there is space in the request
 * queue before submitting the halt request.
 *
 * Some time may elapse before the core flushes any posted requests for this
 * host channel and halts. The Channel Halted interrupt handler completes the
 * deactivation of the host channel.
 *
 * @param _core_if Controller register interface.
 * @param _hc Host channel to halt.
 * @param _halt_status Reason for halting the channel.
 */
void dwc_otg_hc_halt(dwc_otg_core_if_t *_core_if,
		     dwc_hc_t *_hc,
		     dwc_otg_halt_status_e _halt_status)
{
	gnptxsts_data_t 		nptxsts;
	hptxsts_data_t			hptxsts;
	hcchar_data_t 			hcchar;
	dwc_otg_hc_regs_t 		*hc_regs;
	dwc_otg_core_global_regs_t	*global_regs;
	dwc_otg_host_global_regs_t	*host_global_regs;

	hc_regs = _core_if->host_if->hc_regs[_hc->hc_num];
	global_regs = _core_if->core_global_regs;
	host_global_regs = _core_if->host_if->host_global_regs;

	if (_halt_status == DWC_OTG_HC_XFER_NO_HALT_STATUS) {
	    dbg("%s: Invalid halt status DWC_OTG_HC_XFER_NO_HALT_STATUS\n", __FUNCTION__);
	    return;
	}

	if (_halt_status == DWC_OTG_HC_XFER_URB_DEQUEUE ||
	    _halt_status == DWC_OTG_HC_XFER_AHB_ERR) {
		/*
		 * Disable all channel interrupts except Ch Halted. The QTD
		 * and QH state associated with this transfer has been cleared
		 * (in the case of URB_DEQUEUE), so the channel needs to be
		 * shut down carefully to prevent crashes.
		 */
		hcintmsk_data_t hcintmsk;
		hcintmsk.d32 = 0;
		hcintmsk.b.chhltd = 1;
		dwc_write_reg32(&hc_regs->hcintmsk, hcintmsk.d32);

		/*
		 * Make sure no other interrupts besides halt are currently
		 * pending. Handling another interrupt could cause a crash due
		 * to the QTD and QH state.
		 */
		dwc_write_reg32(&hc_regs->hcint, ~hcintmsk.d32);

		/*
		 * Make sure the halt status is set to URB_DEQUEUE or AHB_ERR
		 * even if the channel was already halted for some other
		 * reason.
		 */
		_hc->halt_status = _halt_status;

		hcchar.d32 = dwc_read_reg32(&hc_regs->hcchar);
		if (hcchar.b.chen == 0) {
			/*
			 * The channel is either already halted or it hasn't
			 * started yet. In DMA mode, the transfer may halt if
			 * it finishes normally or a condition occurs that
			 * requires driver intervention. Don't want to halt
			 * the channel again. In either Slave or DMA mode,
			 * it's possible that the transfer has been assigned
			 * to a channel, but not started yet when an URB is
			 * dequeued. Don't want to halt a channel that hasn't
			 * started yet.
			 */
			return;
		}
	}

	if (_hc->halt_pending) {
		/*
		 * A halt has already been issued for this channel. This might
		 * happen when a transfer is aborted by a higher level in
		 * the stack.
		 */
		dbg("*** %s: Channel %d, _hc->halt_pending already set ***\n",
			__FUNCTION__, _hc->hc_num);

		return;
	}

        hcchar.d32 = dwc_read_reg32(&hc_regs->hcchar);
	hcchar.b.chen = 1;
	hcchar.b.chdis = 1;

	if (!_core_if->dma_enable) {
		/* Check for space in the request queue to issue the halt. */
		if (_hc->ep_type == DWC_OTG_EP_TYPE_CONTROL ||
		    _hc->ep_type == DWC_OTG_EP_TYPE_BULK) {
			nptxsts.d32 = dwc_read_reg32(&global_regs->gnptxsts);
			if (nptxsts.b.nptxqspcavail == 0) {
				hcchar.b.chen = 0;
			}
		} else {
			hptxsts.d32 = dwc_read_reg32(&host_global_regs->hptxsts);
			if ((hptxsts.b.ptxqspcavail == 0) || (_core_if->queuing_high_bandwidth)) {
				hcchar.b.chen = 0;
			}
		}
	}

	dwc_write_reg32(&hc_regs->hcchar, hcchar.d32);

	_hc->halt_status = _halt_status;

	if (hcchar.b.chen) {
		_hc->halt_pending = 1;
		_hc->halt_on_queue = 0;
	} else {
		_hc->halt_on_queue = 1;
	}

	info("%s: Channel %d\n", __func__, _hc->hc_num);
	info("  hcchar: 0x%08x\n", hcchar.d32);
	info("  halt_pending: %d\n", _hc->halt_pending);
	info("  halt_on_queue: %d\n", _hc->halt_on_queue);
	info("  halt_status: %d\n", _hc->halt_status);

	return;
}

/**
 * Clears the transfer state for a host channel. This function is normally
 * called after a transfer is done and the host channel is being released.
 *
 * @param _core_if Programming view of DWC_otg controller.
 * @param _hc Identifies the host channel to clean up.
 */
void dwc_otg_hc_cleanup(dwc_otg_core_if_t *_core_if, dwc_hc_t *_hc)
{
	dwc_otg_hc_regs_t *hc_regs;

	_hc->xfer_started = 0;

	/*
	 * Clear channel interrupt enables and any unhandled channel interrupt
	 * conditions.
	 */
	hc_regs = _core_if->host_if->hc_regs[_hc->hc_num];
	dwc_write_reg32(&hc_regs->hcintmsk, 0);
	dwc_write_reg32(&hc_regs->hcint, 0xFFFFFFFF);
}


/**
 * Sets the channel property that indicates in which frame a periodic transfer
 * should occur. This is always set to the _next_ frame. This function has no
 * effect on non-periodic transfers.
 *
 * @param _core_if Programming view of DWC_otg controller.
 * @param _hc Identifies the host channel to set up and its properties.
 * @param _hcchar Current value of the HCCHAR register for the specified host
 * channel.
 */
static inline void hc_set_even_odd_frame(dwc_otg_core_if_t *_core_if,
					 dwc_hc_t *_hc,
					 hcchar_data_t *_hcchar)
{
	if (_hc->ep_type == DWC_OTG_EP_TYPE_INTR ||
	    _hc->ep_type == DWC_OTG_EP_TYPE_ISOC) {
		hfnum_data_t	hfnum;
		hfnum.d32 = dwc_read_reg32(&_core_if->host_if->host_global_regs->hfnum);
		/* 1 if _next_ frame is odd, 0 if it's even */
		_hcchar->b.oddfrm = (hfnum.b.frnum & 0x1) ? 0 : 1;
	}
}

/*
 * This function does the setup for a data transfer for a host channel and
 * starts the transfer. May be called in either Slave mode or DMA mode. In
 * Slave mode, the caller must ensure that there is sufficient space in the
 * request queue and Tx Data FIFO.
 *
 * For an OUT transfer in Slave mode, it loads a data packet into the
 * appropriate FIFO. If necessary, additional data packets will be loaded in
 * the Host ISR.
 *
 * For an IN transfer in Slave mode, a data packet is requested. The data
 * packets are unloaded from the Rx FIFO in the Host ISR. If necessary,
 * additional data packets are requested in the Host ISR.
 *
 * For a PING transfer in Slave mode, the Do Ping bit is set in the HCTSIZ
 * register along with a packet count of 1 and the channel is enabled. This
 * causes a single PING transaction to occur. Other fields in HCTSIZ are
 * simply set to 0 since no data transfer occurs in this case.
 *
 * For a PING transfer in DMA mode, the HCTSIZ register is initialized with
 * all the information required to perform the subsequent data transfer. In
 * addition, the Do Ping bit is set in the HCTSIZ register. In this case, the
 * controller performs the entire PING protocol, then starts the data
 * transfer.
 *
 * @param _core_if Programming view of DWC_otg controller.
 * @param _hc Information needed to initialize the host channel. The xfer_len
 * value may be reduced to accommodate the max widths of the XferSize and
 * PktCnt fields in the HCTSIZn register. The multi_count value may be changed
 * to reflect the final xfer_len value.
 */
void dwc_otg_hc_start_transfer(dwc_otg_core_if_t *_core_if, dwc_hc_t *_hc)
{
	hcchar_data_t hcchar;
	hctsiz_data_t hctsiz;
	uint16_t num_packets;
	uint32_t max_hc_xfer_size = _core_if->core_params->max_transfer_size;
	uint16_t max_hc_pkt_count = _core_if->core_params->max_packet_count;
	dwc_otg_hc_regs_t *hc_regs = _core_if->host_if->hc_regs[_hc->hc_num];

	hctsiz.d32 = 0;

	if (_hc->do_ping) {
		if (!_core_if->dma_enable) {
			dwc_otg_hc_do_ping(_core_if, _hc);
			_hc->xfer_started = 1;
			return;
		} else {
			hctsiz.b.dopng = 1;
		}
	}

	/*
	 * Ensure that the transfer length and packet count will fit
	 * in the widths allocated for them in the HCTSIZn register.
	 */
        if (_hc->do_split) {
                num_packets = 1;

                if (_hc->complete_split && !_hc->ep_is_in) {
                        /* For CSPLIT OUT Transfer, set the size to 0 so the
                         * core doesn't expect any data written to the FIFO */
                        _hc->xfer_len = 0;
                } else if (_hc->ep_is_in || (_hc->xfer_len > _hc->max_packet)) {
                        _hc->xfer_len = _hc->max_packet;
                } else if (!_hc->ep_is_in && (_hc->xfer_len > 188)) {
                        _hc->xfer_len = 188;
                }

                hctsiz.b.xfersize = _hc->xfer_len;
        } else {
               if (_hc->xfer_len > max_hc_xfer_size) {
		   /* Make sure that xfer_len is a multiple of max packet size. */
		   _hc->xfer_len = max_hc_xfer_size - _hc->max_packet + 1;
	       }
	}

	if (_hc->xfer_len > 0) {
	    num_packets = (_hc->xfer_len + _hc->max_packet - 1) / _hc->max_packet;
	    if (num_packets > max_hc_pkt_count) {
		num_packets = max_hc_pkt_count;
		_hc->xfer_len = num_packets * _hc->max_packet;
	    }
	} else {
	    /* Need 1 packet for transfer length of 0. */
	    num_packets = 1;
	}
	
	if (_hc->ep_is_in) {
	    /* Always program an integral # of max packets for IN transfers. */
	    _hc->xfer_len = num_packets * _hc->max_packet;
	}
	
	
	hctsiz.b.xfersize = _hc->xfer_len;

	_hc->start_pkt_count = num_packets;
	hctsiz.b.pktcnt = num_packets;
	hctsiz.b.pid = _hc->data_pid_start;
	dwc_write_reg32(&hc_regs->hctsiz, hctsiz.d32);

	info("%s: Channel %d\n", __func__, _hc->hc_num);
	info("  Xfer Size: %d\n", hctsiz.b.xfersize);
	info("  Num Pkts: %d\n", hctsiz.b.pktcnt);
	info("  Start PID: %d\n", hctsiz.b.pid);

        /* Start the split */
        if (_hc->do_split) {
                hcsplt_data_t hcsplt;
                hcsplt.d32 = dwc_read_reg32 (&hc_regs->hcsplt);
                hcsplt.b.spltena = 1;
                dwc_write_reg32(&hc_regs->hcsplt, hcsplt.d32);
        }

	hcchar.d32 = dwc_read_reg32(&hc_regs->hcchar);
	hcchar.b.multicnt = _hc->multi_count;
	hc_set_even_odd_frame(_core_if, _hc, &hcchar);

	/* Set host channel enable after all other setup is complete. */
	hcchar.b.chen = 1;
	hcchar.b.chdis = 0;
	dwc_write_reg32(&hc_regs->hcchar, hcchar.d32);

	_hc->xfer_started = 1;
	_hc->requests++;

	if (!_core_if->dma_enable &&
	    !_hc->ep_is_in && _hc->xfer_len > 0)
	{
		/* Load OUT packet into the appropriate Tx FIFO. */
		dwc_otg_hc_write_packet(_core_if, _hc);
	}
}

/**
 * This function continues a data transfer that was started by previous call
 * to <code>dwc_otg_hc_start_transfer</code>. The caller must ensure there is
 * sufficient space in the request queue and Tx Data FIFO. This function
 * should only be called in Slave mode. In DMA mode, the controller acts
 * autonomously to complete transfers programmed to a host channel.
 *
 * For an OUT transfer, a new data packet is loaded into the appropriate FIFO
 * if there is any data remaining to be queued. For an IN transfer, another
 * data packet is always requested. For the SETUP phase of a control transfer,
 * this function does nothing.
 *
 * @return 1 if a new request is queued, 0 if no more requests are required
 * for this transfer.
 */
int dwc_otg_hc_continue_transfer(dwc_otg_core_if_t *_core_if, dwc_hc_t *_hc)
{
	info("%s: Channel %d\n", __func__, _hc->hc_num);

        if (_hc->do_split) {
                /* SPLITs always queue just once per channel */
                return 0;
	} else if (_hc->data_pid_start == DWC_OTG_HC_PID_SETUP) {
		/* SETUPs are queued only once since they can't be NAKed. */
		return 0;
	} else if (_hc->ep_is_in) {
		/*
		 * Always queue another request for other IN transfers. If
		 * back-to-back INs are issued and NAKs are received for both,
		 * the driver may still be processing the first NAK when the
		 * second NAK is received. When the interrupt handler clears
		 * the NAK interrupt for the first NAK, the second NAK will
		 * not be seen. So we can't depend on the NAK interrupt
		 * handler to requeue a NAKed request. Instead, IN requests
		 * are issued each time this function is called. When the
		 * transfer completes, the extra requests for the channel will
		 * be flushed.
		 */
		hcchar_data_t hcchar;
		dwc_otg_hc_regs_t *hc_regs = _core_if->host_if->hc_regs[_hc->hc_num];

		hcchar.d32 = dwc_read_reg32(&hc_regs->hcchar);
		hc_set_even_odd_frame(_core_if, _hc, &hcchar);
		hcchar.b.chen = 1;
		hcchar.b.chdis = 0;
		dbg("  IN xfer: hcchar = 0x%08x\n", hcchar.d32);
		dwc_write_reg32(&hc_regs->hcchar, hcchar.d32);
		_hc->requests++;
		return 1;
	} else {
		/* OUT transfers. */
		if (_hc->xfer_count < _hc->xfer_len) {
			/* Load OUT packet into the appropriate Tx FIFO. */
			dwc_otg_hc_write_packet(_core_if, _hc);
			_hc->requests++;
			return 1;
		} else {
			return 0;
		}
	}
}

/**
 * Starts a PING transfer. This function should only be called in Slave mode.
 * The Do Ping bit is set in the HCTSIZ register, then the channel is enabled.
 */
void dwc_otg_hc_do_ping(dwc_otg_core_if_t *_core_if, dwc_hc_t *_hc)
{
	hcchar_data_t hcchar;
	hctsiz_data_t hctsiz;
	dwc_otg_hc_regs_t *hc_regs = _core_if->host_if->hc_regs[_hc->hc_num];

	info("%s: Channel %d\n", __func__, _hc->hc_num);

	hctsiz.d32 = 0;
	hctsiz.b.dopng = 1;
	hctsiz.b.pktcnt = 1;
	dwc_write_reg32(&hc_regs->hctsiz, hctsiz.d32);

	hcchar.d32 = dwc_read_reg32(&hc_regs->hcchar);
	hcchar.b.chen = 1;
	hcchar.b.chdis = 0;
	dwc_write_reg32(&hc_regs->hcchar, hcchar.d32);
}

/*
 * This function writes a packet into the Tx FIFO associated with the Host
 * Channel. For a channel associated with a non-periodic EP, the non-periodic
 * Tx FIFO is written. For a channel associated with a periodic EP, the
 * periodic Tx FIFO is written. This function should only be called in Slave
 * mode.
 *
 * Upon return the xfer_buff and xfer_count fields in _hc are incremented by
 * then number of bytes written to the Tx FIFO.
 */
void dwc_otg_hc_write_packet(dwc_otg_core_if_t *_core_if, dwc_hc_t *_hc)
{
	uint32_t i;
	uint32_t remaining_count;
	uint32_t byte_count;
	uint32_t dword_count;

	uint32_t *data_buff = (uint32_t *)(_hc->xfer_buff);
	uint32_t *data_fifo = _core_if->data_fifo[_hc->hc_num];

	remaining_count = _hc->xfer_len - _hc->xfer_count;
	if (remaining_count > _hc->max_packet) {
		byte_count = _hc->max_packet;
	} else {
		byte_count = remaining_count;
	}

	dword_count = (byte_count + 3) / 4;

	if ((((unsigned long)data_buff) & 0x3) == 0) {
		/* xfer_buff is DWORD aligned. */
		for (i = 0; i < dword_count; i++, data_buff++) {
			dwc_write_reg32(data_fifo, *data_buff);
		}
	} else {
		/* xfer_buff is not DWORD aligned. */
		for (i = 0; i < dword_count; i++, data_buff++) {
		    printf("JSRXNLE_FIXME: This needs to be fixed\n");
		    //	dwc_write_reg32(data_fifo, get_unaligned(data_buff));
		}
	}

	_hc->xfer_count += byte_count;
	_hc->xfer_buff += byte_count;
}






/**
 * This function reads a packet from the Rx FIFO into the destination
 * buffer.  To read SETUP data use dwc_otg_read_setup_packet.
 *
 * @param _core_if Programming view of DWC_otg controller.
 * @param _dest   Destination buffer for the packet.
 * @param _bytes  Number of bytes to copy to the destination.
 */
void dwc_otg_read_packet(dwc_otg_core_if_t *_core_if,
			 uint8_t *_dest,
			 uint16_t _bytes)
{
	int i;
	int word_count = (_bytes + 3) / 4;

	volatile uint32_t *fifo = _core_if->data_fifo[0];
	uint32_t *data_buff = (uint32_t *)_dest;

#ifdef DWC_OTG_SHOW_INFO
        unsigned char *bufpp;
#endif

	/**
	 * @todo Account for the case where _dest is not dword aligned. This
	 * requires reading data from the FIFO into a uint32_t temp buffer,
	 * then moving it into the data buffer.
	 */

	for (i=0; i<word_count; i++, data_buff++) {
		*data_buff = dwc_read_reg32(fifo);
	}


#ifdef DWC_OTG_SHOW_INFO
        bufpp = (unsigned char *) _dest;
        for (i=0; ((i<64)&&(i<word_count)); i++) {
              info("%02x ", bufpp[i]);
              if ((i+1)%16 == 0) {
                      info("\n");
              }
        }
        info("\n");
#endif

	return;
}



/**
 * Flush a Tx FIFO.
 *
 * @param _core_if Programming view of DWC_otg controller.
 * @param _num Tx FIFO to flush.
 */
extern void dwc_otg_flush_tx_fifo( dwc_otg_core_if_t *_core_if,
                                   const int _num )
{
        dwc_otg_core_global_regs_t *global_regs = _core_if->core_global_regs;
        volatile grstctl_t greset = { .d32 = 0};
        int count = 0;

        info("Flush Tx FIFO %d\n", _num);

        greset.b.txfflsh = 1;
        greset.b.txfnum = _num;
        dwc_write_reg32( &global_regs->grstctl, greset.d32 );

        do {
                greset.d32 = dwc_read_reg32( &global_regs->grstctl);
                if (++count > 10000){
                        printf("%s() HANG! GRSTCTL=%0x GNPTXSTS=0x%08x\n",
			       __func__, greset.d32, dwc_read_reg32( &global_regs->gnptxsts));
                        break;
                }

        } while (greset.b.txfflsh == 1);
        /* Wait for 3 PHY Clocks*/
        udelay(1);
}

/**
 * Flush Rx FIFO.
 *
 * @param _core_if Programming view of DWC_otg controller.
 */
extern void dwc_otg_flush_rx_fifo( dwc_otg_core_if_t *_core_if )
{
        dwc_otg_core_global_regs_t *global_regs = _core_if->core_global_regs;
        volatile grstctl_t greset = { .d32 = 0};
        int count = 0;

        info("%s\n", __func__);
        /*
         *
         */
        greset.b.rxfflsh = 1;
        dwc_write_reg32( &global_regs->grstctl, greset.d32 );

        do {
                greset.d32 = dwc_read_reg32( &global_regs->grstctl);
                if (++count > 10000){
                        printf("%s() HANG! GRSTCTL=%0x\n", __func__, greset.d32);
                        break;
                }
        } while (greset.b.rxfflsh == 1);
        /* Wait for 3 PHY Clocks*/
        udelay(1);
}

/**
 * Do core a soft reset of the core.  Be careful with this because it
 * resets all the internal state machines of the core.
 */
void dwc_otg_core_reset(dwc_otg_core_if_t *_core_if)
{
        dwc_otg_core_global_regs_t *global_regs = _core_if->core_global_regs;
        volatile grstctl_t greset = { .d32 = 0};
        int count = 0;


        dbg("%s\n", __FUNCTION__);
        /* Wait for AHB master IDLE state. */
        do {
                udelay(10);
                greset.d32 = dwc_read_reg32( &global_regs->grstctl);
                if (++count > 100000){
                        printf("%s() HANG! AHB Idle GRSTCTL=%0x\n", __func__,
                                 greset.d32);
                        return;
                }
        } while (greset.b.ahbidle == 0);

        /* Core Soft Reset */
        count = 0;
        greset.b.csftrst = 1;
        dwc_write_reg32( &global_regs->grstctl, greset.d32 );
        do {
                greset.d32 = dwc_read_reg32( &global_regs->grstctl);
                if (++count > 10000000){
                        printf("%s() HANG! Soft Reset GRSTCTL=%0x\n", __func__,
                                 greset.d32);
                        break;
                }
        } while (greset.b.csftrst == 1);
        /* Wait for 3 PHY Clocks*/
        mdelay(100);
}

#endif /* CONFIG_USB_DWC_OTG */
