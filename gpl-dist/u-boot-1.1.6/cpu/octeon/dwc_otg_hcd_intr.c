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

#include <common.h>

#if defined (CONFIG_USB_DWC_OTG)

#include <malloc.h>
#include <usb.h>

#include "dwc_otg_driver.h"
#include "dwc_otg_hcd.h"
#include "dwc_otg_regs.h"
#include <linux/list.h>


extern dwc_otg_hcd_t dwc_otg_hcd_struct[];

/** This interrupt occurs when the non-periodic Tx FIFO is half-empty. More
 * data packets may be written to the FIFO for OUT transfers. More requests
 * may be written to the non-periodic request queue for IN transfers. This
 * interrupt is enabled only in Slave mode. */
int32_t dwc_otg_hcd_handle_np_tx_fifo_empty_intr (dwc_otg_hcd_t *_dwc_otg_hcd)
{
        info("--Non-Periodic TxFIFO Empty Interrupt--\n");
	dwc_otg_hcd_queue_transactions(_dwc_otg_hcd,
				       DWC_OTG_TRANSACTION_NON_PERIODIC);
	return 1;
}

/**
 * Handles the start-of-frame interrupt in host mode. Non-periodic
 * transactions may be queued to the DWC_otg controller for the current
 * (micro)frame. Periodic transactions may be queued to the controller for the
 * next (micro)frame.
 */
int32_t dwc_otg_hcd_handle_sof_intr (dwc_otg_hcd_t *_hcd)
{
	hfnum_data_t		hfnum;
	dwc_otg_transaction_type_e tr_type;
	gintsts_data_t gintsts = {.d32 = 0};

	info("--Start of Frame Interrupt--\n");

	hfnum.d32 = dwc_read_reg32(&_hcd->core_if->host_if->host_global_regs->hfnum);

	_hcd->frame_number = hfnum.b.frnum;

	tr_type = dwc_otg_hcd_select_transactions(_hcd);
	if (tr_type != DWC_OTG_TRANSACTION_NONE) {
		dwc_otg_hcd_queue_transactions(_hcd, tr_type);
	}

	/* Clear interrupt */
	gintsts.b.sofintr = 1;
	dwc_write_reg32(&_hcd->core_if->core_global_regs->gintsts, gintsts.d32);

	return 1;
}


/** Handles the Rx Status Queue Level Interrupt, which indicates that there is at
 * least one packet in the Rx FIFO.  The packets are moved from the FIFO to
 * memory if the DWC_otg controller is operating in Slave mode. */
int32_t dwc_otg_hcd_handle_rx_status_q_level_intr (dwc_otg_hcd_t *_dwc_otg_hcd)
{
        host_grxsts_data_t grxsts;
        dwc_hc_t *hc = NULL;

	info("--RxStsQ Level Interrupt--\n");

        grxsts.d32 = dwc_read_reg32(&_dwc_otg_hcd->core_if->core_global_regs->grxstsp);

        hc = _dwc_otg_hcd->hc_ptr_array[grxsts.b.chnum];

        /* Packet Status */
	info("    Ch num = %d\n", grxsts.b.chnum);
        info("    Count = %d\n", grxsts.b.bcnt);
        info("    DPID = %d, hc.dpid = %d\n", grxsts.b.dpid, hc->data_pid_start);
        info("    PStatus = %d\n", grxsts.b.pktsts);

        switch (grxsts.b.pktsts) {
        case DWC_GRXSTS_PKTSTS_IN:
                /* Read the data into the host buffer. */
                if (grxsts.b.bcnt > 0) {
                        dwc_otg_read_packet(_dwc_otg_hcd->core_if,
                                            hc->xfer_buff,
                                            grxsts.b.bcnt);

                        /* Update the HC fields for the next packet received. */
                        hc->xfer_count += grxsts.b.bcnt;
                        hc->xfer_buff += grxsts.b.bcnt;
                }

        case DWC_GRXSTS_PKTSTS_IN_XFER_COMP:
        case DWC_GRXSTS_PKTSTS_DATA_TOGGLE_ERR:
        case DWC_GRXSTS_PKTSTS_CH_HALTED:
                /* Handled in interrupt, just ignore data */
                break;
        default:
                printf("RX_STS_Q Interrupt: Unknown status %d\n", grxsts.b.pktsts);
                break;
        }

        return 1;
}


/** There are multiple conditions that can cause a port interrupt. This function
 * determines which interrupt conditions have occurred and handles them
 * appropriately. */
int32_t dwc_otg_hcd_handle_port_intr (dwc_otg_hcd_t *_dwc_otg_hcd)
{
	int retval = 0;
	hprt0_data_t hprt0;
	hprt0_data_t hprt0_modify;

	hprt0.d32 = dwc_read_reg32(_dwc_otg_hcd->core_if->host_if->hprt0);
	hprt0_modify.d32 = dwc_read_reg32(_dwc_otg_hcd->core_if->host_if->hprt0);

	/* Clear appropriate bits in HPRT0 to clear the interrupt bit in
	 * GINTSTS */

	hprt0_modify.b.prtena = 0;
	hprt0_modify.b.prtconndet = 0;
	hprt0_modify.b.prtenchng = 0;
	hprt0_modify.b.prtovrcurrchng = 0;

	/* Port Connect Detected
	 * Set flag and clear if detected */
	if (hprt0.b.prtconndet) {
		info("--Port Interrupt HPRT0=0x%08x "
			    "Port Connect Detected--\n", hprt0.d32);
		_dwc_otg_hcd->flags.b.port_connect_status_change = 1;
		_dwc_otg_hcd->flags.b.port_connect_status = 1;
		hprt0_modify.b.prtconndet = 1;

		/* The Hub driver asserts a reset when it sees port connect
		 * status change flag */
		retval |= 1;
	}

	/* Port Enable Changed
	 * Clear if detected - Set internal flag if disabled */
	if (hprt0.b.prtenchng) {
		info("  --Port Interrupt HPRT0=0x%08x "
			    "Port Enable Changed--\n", hprt0.d32);
		hprt0_modify.b.prtenchng = 1;
		if (hprt0.b.prtena == 1) {
			int do_reset = 0;
			dwc_otg_core_params_t *params = _dwc_otg_hcd->core_if->core_params;
			dwc_otg_core_global_regs_t *global_regs = _dwc_otg_hcd->core_if->core_global_regs;
			dwc_otg_host_if_t *host_if = _dwc_otg_hcd->core_if->host_if;

			/* Check if we need to adjust the PHY clock speed for
			 * low power and adjust it */
			if (params->host_support_fs_ls_low_power)
			{
				gusbcfg_data_t usbcfg;

				usbcfg.d32 = dwc_read_reg32 (&global_regs->gusbcfg);

				if ((hprt0.b.prtspd == DWC_HPRT0_PRTSPD_LOW_SPEED) ||
				    (hprt0.b.prtspd == DWC_HPRT0_PRTSPD_FULL_SPEED))
				{
					/*
					 * Low power
					 */
					hcfg_data_t hcfg;
					if (usbcfg.b.phylpwrclksel == 0) {
						/* Set PHY low power clock select for FS/LS devices */
						usbcfg.b.phylpwrclksel = 1;
						dwc_write_reg32(&global_regs->gusbcfg, usbcfg.d32);
						do_reset = 1;
					}

					hcfg.d32 = dwc_read_reg32(&host_if->host_global_regs->hcfg);

					if ((hprt0.b.prtspd == DWC_HPRT0_PRTSPD_LOW_SPEED) &&
					    (params->host_ls_low_power_phy_clk ==
					     DWC_HOST_LS_LOW_POWER_PHY_CLK_PARAM_6MHZ))
					{
						/* 6 MHZ */
						info("FS_PHY programming HCFG to 6 MHz (Low Power)\n");
						if (hcfg.b.fslspclksel != DWC_HCFG_6_MHZ) {
							hcfg.b.fslspclksel = DWC_HCFG_6_MHZ;
							dwc_write_reg32(&host_if->host_global_regs->hcfg,
									hcfg.d32);
							do_reset = 1;
						}
					}
					else {
						/* 48 MHZ */
						info("FS_PHY programming HCFG to 48 MHz ()\n");
						if (hcfg.b.fslspclksel != DWC_HCFG_48_MHZ) {
							hcfg.b.fslspclksel = DWC_HCFG_48_MHZ;
							dwc_write_reg32(&host_if->host_global_regs->hcfg,
									hcfg.d32);
							do_reset = 1;
						}
					}
				}
				else {
					/*
					 * Not low power
					 */
					if (usbcfg.b.phylpwrclksel == 1) {
						usbcfg.b.phylpwrclksel = 0;
						dwc_write_reg32(&global_regs->gusbcfg, usbcfg.d32);
						do_reset = 1;
					}
				}

			}
			
			if (!do_reset) {
				/* Port has been enabled set the reset change flag */
				_dwc_otg_hcd->flags.b.port_reset_change = 1;
			}

		} else {
			_dwc_otg_hcd->flags.b.port_enable_change = 1;
		}
		retval |= 1;
	}

	/** Overcurrent Change Interrupt */
	if (hprt0.b.prtovrcurrchng) {
	        info("  --Port Interrupt HPRT0=0x%08x "
		       "Port Overcurrent Changed--\n", hprt0.d32);
		_dwc_otg_hcd->flags.b.port_over_current_change = 1;
		hprt0_modify.b.prtovrcurrchng = 1;
		retval |= 1;
	}

	/* Clear Port Interrupts */
	dwc_write_reg32(_dwc_otg_hcd->core_if->host_if->hprt0, hprt0_modify.d32);

	return retval;
}

/**
 * Frees the first QTD in the QH's list if free_qtd is 1. For non-periodic
 * QHs, removes the QH from the active non-periodic schedule. If any QTDs are
 * still linked to the QH, the QH is added to the end of the inactive
 * non-periodic schedule. For periodic QHs, removes the QH from the periodic
 * schedule if no more QTDs are linked to the QH.
 */
static void deactivate_qh(dwc_otg_hcd_t *_hcd,
			  dwc_otg_qh_t *_qh,
			  int free_qtd)
{
	int continue_split = 0;
	dwc_otg_qtd_t *qtd;

	info("  %s(%p,%p,%d)\n", __FUNCTION__, _hcd, _qh, free_qtd);

	qtd = list_entry(_qh->qtd_list.next, dwc_otg_qtd_t, qtd_list_entry);

	if (qtd->complete_split) {
		continue_split = 1;
	}

	if (free_qtd) {
		dwc_otg_hcd_qtd_remove_and_free(qtd);
		continue_split = 0;
	}

	_qh->channel = NULL;
	_qh->qtd_in_process = NULL;
	dwc_otg_hcd_qh_deactivate(_hcd, _qh, continue_split);
}

/**
 * Releases a host channel for use by other transfers. Attempts to select and
 * queue more transactions since at least one host channel is available.
 *
 * @param _hcd The HCD state structure.
 * @param _hc The host channel to release.
 * @param _qtd The QTD associated with the host channel. This QTD may be freed
 * if the transfer is complete or an error has occurred.
 * @param _halt_status Reason the channel is being released. This status
 * determines the actions taken by this function.
 */
static void release_channel(dwc_otg_hcd_t *_hcd,
			    dwc_hc_t *_hc,
			    dwc_otg_qtd_t *_qtd,
			    dwc_otg_halt_status_e _halt_status)
{
	dwc_otg_transaction_type_e tr_type;
	int free_qtd;

	info("  %s: channel %d, halt_status %d\n",
	     __FUNCTION__, _hc->hc_num, _halt_status);

	switch (_halt_status) {
	case DWC_OTG_HC_XFER_URB_COMPLETE:
		free_qtd = 1;
		break;
	case DWC_OTG_HC_XFER_AHB_ERR:
	case DWC_OTG_HC_XFER_STALL:
	case DWC_OTG_HC_XFER_BABBLE_ERR:
		free_qtd = 1;
		break;
	case DWC_OTG_HC_XFER_XACT_ERR:
		if (_qtd->error_count >= 3) {
			info("  Complete URB with transaction error\n");
			free_qtd = 1;
			_qtd->urb->status = -DWC_OTG_EPROTO;
			dwc_otg_hcd_complete_urb(_hcd, _qtd->urb, -DWC_OTG_EPROTO);
		} else {
			free_qtd = 0;
		}
		break;
	case DWC_OTG_HC_XFER_URB_DEQUEUE:
		/*
		 * The QTD has already been removed and the QH has been
		 * deactivated. Don't want to do anything except release the
		 * host channel and try to queue more transfers.
		 */
		goto cleanup;
	case DWC_OTG_HC_XFER_NO_HALT_STATUS:
		info("%s: No halt_status, channel %d\n", __FUNCTION__, _hc->hc_num);
		free_qtd = 0;
		break;
	default:
		free_qtd = 0;
		break;
	}


	deactivate_qh(_hcd, _hc->qh, free_qtd);

 cleanup:
	/*
	 * Release the host channel for use by other transfers. The cleanup
	 * function clears the channel interrupt enables and conditions, so
	 * there's no need to clear the Channel Halted interrupt separately.
	 */
	dwc_otg_hc_cleanup(_hcd->core_if, _hc);
	list_add_tail(&_hc->hc_list_entry, &_hcd->free_hc_list);

	switch (_hc->ep_type) {
	case DWC_OTG_EP_TYPE_CONTROL:
	case DWC_OTG_EP_TYPE_BULK:
		_hcd->non_periodic_channels--;
		break;

	default:
		/*
		 * Don't release reservations for periodic channels here.
		 * That's done when a periodic transfer is descheduled (i.e.
		 * when the QH is removed from the periodic schedule).
		 */
		break;
	}

	/* Try to queue more transfers now that there's a free channel. */
	tr_type = dwc_otg_hcd_select_transactions(_hcd);
	if (tr_type != DWC_OTG_TRANSACTION_NONE) {
		dwc_otg_hcd_queue_transactions(_hcd, tr_type);
	}
}

/**
 * Halts a host channel. If the channel cannot be halted immediately because
 * the request queue is full, this function ensures that the FIFO empty
 * interrupt for the appropriate queue is enabled so that the halt request can
 * be queued when there is space in the request queue.
 *
 * This function may also be called in DMA mode. In that case, the channel is
 * simply released since the core always halts the channel automatically in
 * DMA mode.
 */
static void halt_channel(dwc_otg_hcd_t *_hcd,
			 dwc_hc_t *_hc,
			 dwc_otg_qtd_t *_qtd,
			 dwc_otg_halt_status_e _halt_status)
{
	if (_hcd->core_if->dma_enable) {
		release_channel(_hcd, _hc, _qtd, _halt_status);
		return;
	}
	info("  %s: channel %d, halt_status %d\n",
	     __FUNCTION__, _hc->hc_num, _halt_status);

	/* Slave mode processing... */
	dwc_otg_hc_halt(_hcd->core_if, _hc, _halt_status);

	if (_hc->halt_on_queue) {
		gintmsk_data_t gintmsk = {.d32 = 0};
		dwc_otg_core_global_regs_t *global_regs;
		global_regs = _hcd->core_if->core_global_regs;

		if (_hc->ep_type == DWC_OTG_EP_TYPE_CONTROL ||
		    _hc->ep_type == DWC_OTG_EP_TYPE_BULK) {
			/*
			 * Make sure the Non-periodic Tx FIFO empty interrupt
			 * is enabled so that the non-periodic schedule will
			 * be processed.
			 */
			gintmsk.b.nptxfempty = 1;
			dwc_modify_reg32(&global_regs->gintmsk, 0, gintmsk.d32);
		}
	}
}


/* Macro used to clear one channel interrupt */
#define clear_hc_int(_hc_regs_,_intr_) \
do { \
	hcint_data_t hcint_clear = {.d32 = 0}; \
	hcint_clear.b._intr_ = 1; \
	dwc_write_reg32(&((_hc_regs_)->hcint), hcint_clear.d32); \
} while (0)

/*
 * Macro used to disable one channel interrupt. Channel interrupts are
 * disabled when the channel is halted or released by the interrupt handler.
 * There is no need to handle further interrupts of that type until the
 * channel is re-assigned. In fact, subsequent handling may cause crashes
 * because the channel structures are cleaned up when the channel is released.
 */
#define disable_hc_int(_hc_regs_,_intr_) \
do { \
	hcintmsk_data_t hcintmsk = {.d32 = 0}; \
	hcintmsk.b._intr_ = 1; \
	dwc_modify_reg32(&((_hc_regs_)->hcintmsk), hcintmsk.d32, 0); \
} while (0)


/**
 * Gets the actual length of a transfer after the transfer halts. _halt_status
 * holds the reason for the halt.
 *
 * For IN transfers where _halt_status is DWC_OTG_HC_XFER_COMPLETE,
 * *_short_read is set to 1 upon return if less than the requested
 * number of bytes were transferred. Otherwise, *_short_read is set to 0 upon
 * return. _short_read may also be NULL on entry, in which case it remains
 * unchanged.
 */
static uint32_t get_actual_xfer_length(dwc_hc_t *_hc,
				       dwc_otg_hc_regs_t *_hc_regs,
				       dwc_otg_qtd_t *_qtd,
				       dwc_otg_halt_status_e _halt_status,
				       int *_short_read)
{
	hctsiz_data_t 	hctsiz;
	uint32_t 	length;

	if (_short_read != NULL) {
		*_short_read = 0;
	}
	hctsiz.d32 = dwc_read_reg32(&_hc_regs->hctsiz);

	if (_halt_status == DWC_OTG_HC_XFER_COMPLETE) {
		if (_hc->ep_is_in) {
			length = _hc->xfer_len - hctsiz.b.xfersize;
			if (_short_read != NULL) {
				*_short_read = (hctsiz.b.xfersize != 0);
			}
		} else if (_hc->qh->do_split) {
			length = _qtd->ssplit_out_xfer_count;
		} else {
			length = _hc->xfer_len;
		}
	} else {
		/*
		 * Must use the hctsiz.pktcnt field to determine how much data
		 * has been transferred. This field reflects the number of
		 * packets that have been transferred via the USB. This is
		 * always an integral number of packets if the transfer was
		 * halted before its normal completion. (Can't use the
		 * hctsiz.xfersize field because that reflects the number of
		 * bytes transferred via the AHB, not the USB).
		 */
		length = (_hc->start_pkt_count - hctsiz.b.pktcnt) * _hc->max_packet;
	}

	return length;
}

/**
 * Updates the state of the URB after a Transfer Complete interrupt on the
 * host channel. Updates the actual_length field of the URB based on the
 * number of bytes transferred via the host channel. Sets the URB status
 * if the data transfer is finished.
 *
 * @return 1 if the data transfer specified by the URB is completely finished,
 * 0 otherwise.
 */
static int update_urb_state_xfer_comp(dwc_hc_t *_hc,
				      dwc_otg_hc_regs_t *_hc_regs,
				      dwc_otg_urb_t *_urb,
				      dwc_otg_qtd_t *_qtd)
{
	int 		xfer_done = 0;
	int 		short_read = 0;

	_urb->actual_length += get_actual_xfer_length(_hc, _hc_regs, _qtd,
						      DWC_OTG_HC_XFER_COMPLETE,
						      &short_read);

	if (short_read || (_urb->actual_length == _urb->transfer_buffer_length)) {
		xfer_done = 1;
		if (short_read && (_urb->transfer_flags & URB_SHORT_NOT_OK)) {
			_urb->status = -DWC_OTG_EREMOTEIO;
		}
		else {
			_urb->status = 0;
		}
	}

#ifdef DWC_OTG_SHOW_INFO
	{
		hctsiz_data_t 	hctsiz;
		hctsiz.d32 = dwc_read_reg32(&_hc_regs->hctsiz);
		info("DWC_otg: %s: %s, channel %d\n",
			    __func__, (_hc->ep_is_in ? "IN" : "OUT"), _hc->hc_num);
		info("  hc->xfer_len %d\n", _hc->xfer_len);
		info("  hctsiz.xfersize %d\n", hctsiz.b.xfersize);
		info("  urb->transfer_buffer_length %d\n",
			    _urb->transfer_buffer_length);
		info("  urb->actual_length %d\n", _urb->actual_length);
		info("  short_read %d, xfer_done %d\n",
			    short_read, xfer_done);
	}
#endif

	return xfer_done;
}

/*
 * Save the starting data toggle for the next transfer. The data toggle is
 * saved in the QH for non-control transfers and it's saved in the QTD for
 * control transfers.
 */
static void save_data_toggle(dwc_hc_t *_hc,
			     dwc_otg_hc_regs_t *_hc_regs,
			     dwc_otg_qtd_t *_qtd)
{
	hctsiz_data_t hctsiz;
	hctsiz.d32 = dwc_read_reg32(&_hc_regs->hctsiz);

	if (_hc->ep_type != DWC_OTG_EP_TYPE_CONTROL) {
		dwc_otg_qh_t *qh = _hc->qh;
		if (hctsiz.b.pid == DWC_HCTSIZ_DATA0) {
			qh->data_toggle = DWC_OTG_HC_PID_DATA0;
		} else {
			qh->data_toggle = DWC_OTG_HC_PID_DATA1;
		}
	} else {
		if (hctsiz.b.pid == DWC_HCTSIZ_DATA0) {
			_qtd->data_toggle = DWC_OTG_HC_PID_DATA0;
		} else {
			_qtd->data_toggle = DWC_OTG_HC_PID_DATA1;
		}
	}
}


/**
 * Performs common cleanup for non-periodic transfers after a Transfer
 * Complete interrupt. This function should be called after any endpoint type
 * specific handling is finished to release the host channel.
 */
static void complete_non_periodic_xfer(dwc_otg_hcd_t *_hcd,
				       dwc_hc_t *_hc,
				       dwc_otg_hc_regs_t *_hc_regs,
				       dwc_otg_qtd_t *_qtd,
				       dwc_otg_halt_status_e _halt_status)
{
	hcint_data_t hcint;

	_qtd->error_count = 0;

	hcint.d32 = dwc_read_reg32(&_hc_regs->hcint);
	if (hcint.b.nyet) {
		/*
		 * Got a NYET on the last transaction of the transfer. This
		 * means that the endpoint should be in the PING state at the
		 * beginning of the next transfer.
		 */
		_hc->qh->ping_state = 1;
		clear_hc_int(_hc_regs,nyet);
	}

	/*
	 * Always halt and release the host channel to make it available for
	 * more transfers. There may still be more phases for a control
	 * transfer or more data packets for a bulk transfer at this point,
	 * but the host channel is still halted. A channel will be reassigned
	 * to the transfer when the non-periodic schedule is processed after
	 * the channel is released. This allows transactions to be queued
	 * properly via dwc_otg_hcd_queue_transactions, which also enables the
	 * Tx FIFO Empty interrupt if necessary.
	 */
	if (_hc->ep_is_in) {
		/*
		 * IN transfers in Slave mode require an explicit disable to
		 * halt the channel. (In DMA mode, this call simply releases
		 * the channel.)
		 */
		halt_channel(_hcd, _hc, _qtd, _halt_status);
	} else {
		/*
		 * The channel is automatically disabled by the core for OUT
		 * transfers in Slave mode.
		 */
		release_channel(_hcd, _hc, _qtd, _halt_status);
	}
}

/**
 * Handles a host channel Transfer Complete interrupt. This handler may be
 * called in either DMA mode or Slave mode.
 */
static int32_t handle_hc_xfercomp_intr(dwc_otg_hcd_t *_hcd,
				       dwc_hc_t *_hc,
				       dwc_otg_hc_regs_t *_hc_regs,
				       dwc_otg_qtd_t *_qtd)
{
	int 			urb_xfer_done;
	dwc_otg_halt_status_e 	halt_status = DWC_OTG_HC_XFER_COMPLETE;
	dwc_otg_urb_t 		*urb = _qtd->urb;
	int 			pipe_type = usb_pipetype(urb->pipe);

	info("--Host Channel %d Interrupt: "
		    "Transfer Complete--\n", _hc->hc_num);

     	/*
	 * Handle xfer complete on CSPLIT.
	 */
	if (_hc->qh->do_split) {
		_qtd->complete_split = 0;
	}

	/* Update the QTD and URB states. */
	switch (pipe_type) {
	case PIPE_CONTROL:
		switch (_qtd->control_phase) {
		case DWC_OTG_CONTROL_SETUP:
			if (urb->transfer_buffer_length > 0) {
				_qtd->control_phase = DWC_OTG_CONTROL_DATA;
			} else {
				_qtd->control_phase = DWC_OTG_CONTROL_STATUS;
			}
			info("  Control setup transaction done\n");
			halt_status = DWC_OTG_HC_XFER_COMPLETE;
			break;
		case DWC_OTG_CONTROL_DATA: {
			urb_xfer_done = update_urb_state_xfer_comp(_hc, _hc_regs, urb, _qtd);
			if (urb_xfer_done) {
				_qtd->control_phase = DWC_OTG_CONTROL_STATUS;
				info("  Control data transfer done\n");
			} else {
				save_data_toggle(_hc, _hc_regs, _qtd);
			}
			halt_status = DWC_OTG_HC_XFER_COMPLETE;
			break;
		}
		case DWC_OTG_CONTROL_STATUS:
			info("  Control transfer complete\n");
			if (urb->status == -DWC_OTG_EINPROGRESS) {
				urb->status = 0;
			}
			dwc_otg_hcd_complete_urb(_hcd, urb, urb->status);
			halt_status = DWC_OTG_HC_XFER_URB_COMPLETE;
			break;
		}

		complete_non_periodic_xfer(_hcd, _hc, _hc_regs, _qtd, halt_status);
		break;
	case PIPE_BULK:
		info("  Bulk transfer complete\n");
		urb_xfer_done = update_urb_state_xfer_comp(_hc, _hc_regs, urb, _qtd);
		if (urb_xfer_done) {
			dwc_otg_hcd_complete_urb(_hcd, urb, urb->status);
			halt_status = DWC_OTG_HC_XFER_URB_COMPLETE;
		} else {
			halt_status = DWC_OTG_HC_XFER_COMPLETE;
		}
			
		save_data_toggle(_hc, _hc_regs, _qtd);
		complete_non_periodic_xfer(_hcd, _hc, _hc_regs, _qtd, halt_status);
		break;
	}

        disable_hc_int(_hc_regs,xfercompl);

	return 1;
}

/**
 * Handles a host channel STALL interrupt. This handler may be called in
 * either DMA mode or Slave mode.
 */
static int32_t handle_hc_stall_intr(dwc_otg_hcd_t *_hcd,
				    dwc_hc_t *_hc,
				    dwc_otg_hc_regs_t *_hc_regs,
				    dwc_otg_qtd_t *_qtd)
{
	dwc_otg_urb_t *urb = _qtd->urb;
	int pipe_type = usb_pipetype(urb->pipe);

	info("--Host Channel %d Interrupt: "
		    "STALL Received--\n", _hc->hc_num);

	if (pipe_type == PIPE_CONTROL) {
		dwc_otg_hcd_complete_urb(_hcd, _qtd->urb, -DWC_OTG_EPIPE);
	}

	if (pipe_type == PIPE_BULK || pipe_type == PIPE_INTERRUPT) {
		dwc_otg_hcd_complete_urb(_hcd, _qtd->urb, -DWC_OTG_EPIPE);
		/*
		 * USB protocol requires resetting the data toggle for bulk
		 * and interrupt endpoints when a CLEAR_FEATURE(ENDPOINT_HALT)
		 * setup command is issued to the endpoint. Anticipate the
		 * CLEAR_FEATURE command since a STALL has occurred and reset
		 * the data toggle now.
		 */
		_hc->qh->data_toggle = 0;
	}

	halt_channel(_hcd, _hc, _qtd, DWC_OTG_HC_XFER_STALL);

	disable_hc_int(_hc_regs,stall);

	return 1;
}

/*
 * Updates the state of the URB when a transfer has been stopped due to an
 * abnormal condition before the transfer completes. Modifies the
 * actual_length field of the URB to reflect the number of bytes that have
 * actually been transferred via the host channel.
 */
static void update_urb_state_xfer_intr(dwc_hc_t *_hc,
				       dwc_otg_hc_regs_t *_hc_regs,
				       dwc_otg_urb_t *_urb,
				       dwc_otg_qtd_t *_qtd,
				       dwc_otg_halt_status_e _halt_status)
{
	uint32_t bytes_transferred = get_actual_xfer_length(_hc, _hc_regs, _qtd,
							    _halt_status, NULL);
	_urb->actual_length += bytes_transferred;

#ifdef DWC_OTG_SHOW_INFO
	{
		hctsiz_data_t 	hctsiz;
		hctsiz.d32 = dwc_read_reg32(&_hc_regs->hctsiz);
		info("DWC_otg: %s: %s, channel %d\n",
			    __FUNCTION__, (_hc->ep_is_in ? "IN" : "OUT"), _hc->hc_num);
		info("  _hc->start_pkt_count %d\n", _hc->start_pkt_count);
		info("  hctsiz.pktcnt %d\n", hctsiz.b.pktcnt);
		info("  _hc->max_packet %d\n", _hc->max_packet);
		info("  bytes_transferred %d\n", bytes_transferred);
		info("  _urb->actual_length %d\n", _urb->actual_length);
		info("  _urb->transfer_buffer_length %d\n",
		_urb->transfer_buffer_length);
	}
#endif
}

/**
 * Handles a host channel NAK interrupt. This handler may be called in either
 * DMA mode or Slave mode.
 */
static int32_t handle_hc_nak_intr(dwc_otg_hcd_t *_hcd,
				  dwc_hc_t *_hc,
				  dwc_otg_hc_regs_t *_hc_regs,
				  dwc_otg_qtd_t *_qtd)
{
	info("--Host Channel %d Interrupt: "
		    "NAK Received--\n", _hc->hc_num);

	/*
	 * Handle NAK for IN/OUT SSPLIT/CSPLIT transfers, bulk, control, and
	 * interrupt.  Re-start the SSPLIT transfer.
	 */
	if (_hc->do_split) {
		if (_hc->complete_split) {
			_qtd->error_count = 0;
		}
		_qtd->complete_split = 0;
		halt_channel(_hcd, _hc, _qtd, DWC_OTG_HC_XFER_NAK);
		goto handle_nak_done;
	}

	switch (usb_pipetype(_qtd->urb->pipe)) {
	case PIPE_CONTROL:
	case PIPE_BULK:
		if (_hcd->core_if->dma_enable && _hc->ep_is_in) {
			/*
			 * NAK interrupts are enabled on bulk/control IN
			 * transfers in DMA mode for the sole purpose of
			 * resetting the error count after a transaction error
			 * occurs. The core will continue transferring data.
			 */
			_qtd->error_count = 0;
			goto handle_nak_done;
		}

		/*
		 * NAK interrupts normally occur during OUT transfers in DMA
		 * or Slave mode. For IN transfers, more requests will be
		 * queued as request queue space is available.
		 */
		_qtd->error_count = 0;

		if (!_hc->qh->ping_state) {
			update_urb_state_xfer_intr(_hc, _hc_regs, _qtd->urb,
						   _qtd, DWC_OTG_HC_XFER_NAK);
			save_data_toggle(_hc, _hc_regs, _qtd);
			/* ASGARD_FIXME: Fix this */
			//			if (_qtd->urb->dev->speed == USB_SPEED_HIGH) {
			//				_hc->qh->ping_state = 1;
			//			}
		}

		/*
		 * Halt the channel so the transfer can be re-started from
		 * the appropriate point or the PING protocol will
		 * start/continue.
		 */
		halt_channel(_hcd, _hc, _qtd, DWC_OTG_HC_XFER_NAK);
		break;
	}

 handle_nak_done:
	disable_hc_int(_hc_regs,nak);

	return 1;
}

/**
 * Handles a host channel ACK interrupt. This interrupt is enabled when
 * performing the PING protocol in Slave mode, when errors occur during
 * either Slave mode or DMA mode, and during Start Split transactions.
 */
static int32_t handle_hc_ack_intr(dwc_otg_hcd_t *_hcd,
				  dwc_hc_t *_hc,
				  dwc_otg_hc_regs_t *_hc_regs,
				  dwc_otg_qtd_t *_qtd)
{
	info("--Host Channel %d Interrupt: "
		    "ACK Received--\n", _hc->hc_num);

	if (_hc->do_split) {
	       /*
                * Handle ACK on SSPLIT.
        	* ACK should not occur in CSPLIT.
	        */
		if ((!_hc->ep_is_in) && (_hc->data_pid_start != DWC_OTG_HC_PID_SETUP)) {
		    dbg("%s: not epin and dpid start not setup\n", __FUNCTION__);
			_qtd->ssplit_out_xfer_count = _hc->xfer_len;
		}
                if (!(_hc->ep_type == DWC_OTG_EP_TYPE_ISOC && !_hc->ep_is_in)) {
                        /* Don't need complete for isochronous out transfers. */
		    dbg("%s: setting complete split\n", __FUNCTION__);
                        _qtd->complete_split = 1;
                }
                else {
		    dbg("%s: halting channel\n", __FUNCTION__);
                        halt_channel(_hcd, _hc, _qtd, DWC_OTG_HC_XFER_ACK);
                }
	} else {
		    dbg("%s: no do split\n", __FUNCTION__);
		_qtd->error_count = 0;

		if (_hc->qh->ping_state) {
			_hc->qh->ping_state = 0;
			/*
			 * Halt the channel so the transfer can be re-started
			 * from the appropriate point. This only happens in
			 * Slave mode. In DMA mode, the ping_state is cleared
			 * when the transfer is started because the core
			 * automatically executes the PING, then the transfer.
			 */
			halt_channel(_hcd, _hc, _qtd, DWC_OTG_HC_XFER_ACK);
		}
	}

	/*
	 * If the ACK occurred when _not_ in the PING state, let the channel
	 * continue transferring data after clearing the error count.
	 */

	disable_hc_int(_hc_regs,ack);

	return 1;
}

/**
 * Handles a host channel NYET interrupt. This interrupt should only occur on
 * Bulk and Control OUT endpoints and for complete split transactions. If a
 * NYET occurs at the same time as a Transfer Complete interrupt, it is
 * handled in the xfercomp interrupt handler, not here. This handler may be
 * called in either DMA mode or Slave mode.
 */
static int32_t handle_hc_nyet_intr(dwc_otg_hcd_t *_hcd,
				   dwc_hc_t *_hc,
				   dwc_otg_hc_regs_t *_hc_regs,
				   dwc_otg_qtd_t *_qtd)
{
	printf("--Host Channel %d Interrupt: "
		    "NYET Received--\n", _hc->hc_num);

	/*
	 * NYET on CSPLIT
	 * re-do the CSPLIT immediately on non-periodic
	 */
	if ((_hc->do_split) && (_hc->complete_split)) {
		halt_channel(_hcd, _hc, _qtd, DWC_OTG_HC_XFER_NYET);
		goto handle_nyet_done;
	}

	_hc->qh->ping_state = 1;
	_qtd->error_count = 0;

	update_urb_state_xfer_intr(_hc, _hc_regs, _qtd->urb, _qtd,
				   DWC_OTG_HC_XFER_NYET);
	save_data_toggle(_hc, _hc_regs, _qtd);

	/*
	 * Halt the channel and re-start the transfer so the PING
	 * protocol will start.
	 */
	halt_channel(_hcd, _hc, _qtd, DWC_OTG_HC_XFER_NYET);

handle_nyet_done:
	disable_hc_int(_hc_regs,nyet);
	return 1;
}

/**
 * Handles a host channel babble interrupt. This handler may be called in
 * either DMA mode or Slave mode.
 */
static int32_t handle_hc_babble_intr(dwc_otg_hcd_t *_hcd,
				     dwc_hc_t *_hc,
				     dwc_otg_hc_regs_t *_hc_regs,
				     dwc_otg_qtd_t *_qtd)
{
	printf("--Host Channel %d Interrupt: "
	       "Babble Error--\n", _hc->hc_num);

	dwc_otg_hcd_complete_urb(_hcd, _qtd->urb, -DWC_OTG_EOVERFLOW);
	halt_channel(_hcd, _hc, _qtd, DWC_OTG_HC_XFER_BABBLE_ERR);
	disable_hc_int(_hc_regs,bblerr);
	return 1;
}

/**
 * Handles a host channel AHB error interrupt. This handler is only called in
 * DMA mode.
 */
static int32_t handle_hc_ahberr_intr(dwc_otg_hcd_t *_hcd,
				     dwc_hc_t *_hc,
				     dwc_otg_hc_regs_t *_hc_regs,
				     dwc_otg_qtd_t *_qtd)
{
	hcchar_data_t 	hcchar;
	hcsplt_data_t	hcsplt;
	hctsiz_data_t 	hctsiz;
	uint32_t	hcdma;
	dwc_otg_urb_t	*urb = _qtd->urb;

	printf("--Host Channel %d Interrupt: "
		    "AHB Error--\n", _hc->hc_num);

	hcchar.d32 = dwc_read_reg32(&_hc_regs->hcchar);
	hcsplt.d32 = dwc_read_reg32(&_hc_regs->hcsplt);
	hctsiz.d32 = dwc_read_reg32(&_hc_regs->hctsiz);
	hcdma = dwc_read_reg32(&_hc_regs->hcdma);

	printf("AHB ERROR, Channel %d\n", _hc->hc_num);
	printf("  hcchar 0x%08x, hcsplt 0x%08x\n", hcchar.d32, hcsplt.d32);
	printf("  hctsiz 0x%08x, hcdma 0x%08x\n", hctsiz.d32, hcdma);
		printf("DWC OTG HCD URB Enqueue\n");
	printf("  Device address: %d\n", usb_pipedevice(urb->pipe));
	printf("  Endpoint: %d, %s\n", usb_pipeendpoint(urb->pipe),
		    (usb_pipein(urb->pipe) ? "IN" : "OUT"));
	printf("  Endpoint type: %s\n",
		    ({char *pipetype;
		    switch (usb_pipetype(urb->pipe)) {
		    case PIPE_CONTROL: pipetype = "CONTROL"; break;
		    case PIPE_BULK: pipetype = "BULK"; break;
		    case PIPE_INTERRUPT: pipetype = "INTERRUPT"; break;
		    case PIPE_ISOCHRONOUS: pipetype = "ISOCHRONOUS"; break;
		    default: pipetype = "UNKNOWN"; break;
		    }; pipetype;}));
	/*  ASGARD_FIXME: 
	            printf("  Speed: %s\n",
		    ({char *speed;
		    switch (urb->dev->speed) {
		    case USB_SPEED_HIGH: speed = "HIGH"; break;
		    case USB_SPEED_FULL: speed = "FULL"; break;
		    case USB_SPEED_LOW: speed = "LOW"; break;
		    default: speed = "UNKNOWN"; break;
		    }; speed;}));*/
	printf("  Max packet size: %d\n",
		    usb_maxpacket(urb->dev, urb->pipe));
	printf("  Data buffer length: %d\n", urb->transfer_buffer_length);
	printf("  Transfer buffer: %p Setup buffer: %p\n",
		    urb->transfer_buffer, urb->setup_packet);
	printf("  Interval: %d\n", urb->interval);

	dwc_otg_hcd_complete_urb(_hcd, urb, -DWC_OTG_EIO);

	/*
	 * Force a channel halt. Don't call halt_channel because that won't
	 * write to the HCCHARn register in DMA mode to force the halt.
	 */
	dwc_otg_hc_halt(_hcd->core_if, _hc, DWC_OTG_HC_XFER_AHB_ERR);

	disable_hc_int(_hc_regs,ahberr);
	return 1;
}

/**
 * Handles a host channel transaction error interrupt. This handler may be
 * called in either DMA mode or Slave mode.
 */
static int32_t handle_hc_xacterr_intr(dwc_otg_hcd_t *_hcd,
				      dwc_hc_t *_hc,
				      dwc_otg_hc_regs_t *_hc_regs,
				      dwc_otg_qtd_t *_qtd)
{
	printf("--Host Channel %d Interrupt: "
		    "Transaction Error--\n", _hc->hc_num);

	switch (usb_pipetype(_qtd->urb->pipe)) {
	case PIPE_CONTROL:
	case PIPE_BULK:
		_qtd->error_count++;
		if (!_hc->qh->ping_state) {
			update_urb_state_xfer_intr(_hc, _hc_regs, _qtd->urb,
						   _qtd, DWC_OTG_HC_XFER_XACT_ERR);
			save_data_toggle(_hc, _hc_regs, _qtd);
			/* ASGARD_FIXME: */
			//			if (!_hc->ep_is_in && _qtd->urb->dev->speed == USB_SPEED_HIGH) {
			//				_hc->qh->ping_state = 1;
			//			}
		}

		/*
		 * Halt the channel so the transfer can be re-started from
		 * the appropriate point or the PING protocol will start.
		 */
		halt_channel(_hcd, _hc, _qtd, DWC_OTG_HC_XFER_XACT_ERR);
		break;
	}
		

	disable_hc_int(_hc_regs,xacterr);

	return 1;
}

/**
 * Handles a host channel frame overrun interrupt. This handler may be called
 * in either DMA mode or Slave mode.
 */
static int32_t handle_hc_frmovrun_intr(dwc_otg_hcd_t *_hcd,
				       dwc_hc_t *_hc,
				       dwc_otg_hc_regs_t *_hc_regs,
				       dwc_otg_qtd_t *_qtd)
{
	printf("--Host Channel %d Interrupt: "
		    "Frame Overrun--\n", _hc->hc_num);

	switch (usb_pipetype(_qtd->urb->pipe)) {
	case PIPE_CONTROL:
	case PIPE_BULK:
		break;
	}

	disable_hc_int(_hc_regs,frmovrun);

	return 1;
}

/**
 * Handles a host channel data toggle error interrupt. This handler may be
 * called in either DMA mode or Slave mode.
 */
static int32_t handle_hc_datatglerr_intr(dwc_otg_hcd_t *_hcd,
					 dwc_hc_t *_hc,
					 dwc_otg_hc_regs_t *_hc_regs,
					 dwc_otg_qtd_t *_qtd)
{
	printf("--Host Channel %d Interrupt: "
		    "Data Toggle Error--\n", _hc->hc_num);

	if (_hc->ep_is_in) {
		_qtd->error_count = 0;
	} else {
		printf("Data Toggle Error on OUT transfer,"
			  "channel %d\n", _hc->hc_num);
	}

	disable_hc_int(_hc_regs,datatglerr);

	return 1;
}

#ifdef DWC_OTG_SHOW_INFO
/**
 * This function is for debug only. It checks that a valid halt status is set
 * and that HCCHARn.chdis is clear. If there's a problem, corrective action is
 * taken and a warning is issued.
 * @return 1 if halt status is ok, 0 otherwise.
 */
static inline int halt_status_ok(dwc_otg_hcd_t *_hcd,
				 dwc_hc_t *_hc,
				 dwc_otg_hc_regs_t *_hc_regs,
				 dwc_otg_qtd_t *_qtd)
{
	hcchar_data_t hcchar;
	hctsiz_data_t hctsiz;
	hcint_data_t hcint;
	hcintmsk_data_t hcintmsk;
	hcsplt_data_t hcsplt;

	if (_hc->halt_status == DWC_OTG_HC_XFER_NO_HALT_STATUS) {
		/*
		 * This code is here only as a check. This condition should
		 * never happen. Ignore the halt if it does occur.
		 */
		hcchar.d32 = dwc_read_reg32(&_hc_regs->hcchar);
		hctsiz.d32 = dwc_read_reg32(&_hc_regs->hctsiz);
		hcint.d32 = dwc_read_reg32(&_hc_regs->hcint);
		hcintmsk.d32 = dwc_read_reg32(&_hc_regs->hcintmsk);
		hcsplt.d32 = dwc_read_reg32(&_hc_regs->hcsplt);
		info("%s: _hc->halt_status == DWC_OTG_HC_XFER_NO_HALT_STATUS, "
			 "channel %d, hcchar 0x%08x, hctsiz 0x%08x, "
			 "hcint 0x%08x, hcintmsk 0x%08x, "
			 "hcsplt 0x%08x, qtd->complete_split %d\n",
			 __FUNCTION__, _hc->hc_num, hcchar.d32, hctsiz.d32,
			 hcint.d32, hcintmsk.d32,
			 hcsplt.d32, _qtd->complete_split);

		info("%s: no halt status, channel %d, ignoring interrupt\n",
			 __FUNCTION__, _hc->hc_num);
		info("\n");
		clear_hc_int(_hc_regs,chhltd);
		return 0;
	}

	/*
	 * This code is here only as a check. hcchar.chdis should
	 * never be set when the halt interrupt occurs. Halt the
	 * channel again if it does occur.
	 */
	hcchar.d32 = dwc_read_reg32(&_hc_regs->hcchar);
	if (hcchar.b.chdis) {
		info("%s: hcchar.chdis set unexpectedly, "
			 "hcchar 0x%08x, trying to halt again\n",
			 __FUNCTION__, hcchar.d32);
		clear_hc_int(_hc_regs,chhltd);
		_hc->halt_pending = 0;
		halt_channel(_hcd, _hc, _qtd, _hc->halt_status);
		return 0;
	}

	return 1;
}
#endif

/**
 * Handles a host channel Channel Halted interrupt.
 *
 * In slave mode, this handler is called only when the driver specifically
 * requests a halt. This occurs during handling other host channel interrupts
 * (e.g. nak, xacterr, stall, nyet, etc.).
 *
 * In DMA mode, this is the interrupt that occurs when the core has finished
 * processing a transfer on a channel. Other host channel interrupts (except
 * ahberr) are disabled in DMA mode.
 */
static int32_t handle_hc_chhltd_intr(dwc_otg_hcd_t *_hcd,
				     dwc_hc_t *_hc,
				     dwc_otg_hc_regs_t *_hc_regs,
				     dwc_otg_qtd_t *_qtd)
{
	info("--Host Channel %d Interrupt: "
		    "Channel Halted--\n", _hc->hc_num);

#ifdef DWC_OTG_SHOW_INFO
	if (!halt_status_ok(_hcd, _hc, _hc_regs, _qtd)) {
	    return 1;
	}
#endif	
	release_channel(_hcd, _hc, _qtd, _hc->halt_status);

	return 1;
}



/** Handles interrupt for a specific Host Channel */
int32_t dwc_otg_hcd_handle_hc_n_intr (dwc_otg_hcd_t *_dwc_otg_hcd, uint32_t _num)
{
	int retval = 0;
	hcint_data_t hcint;
	hcintmsk_data_t hcintmsk;
	dwc_hc_t *hc;
	dwc_otg_hc_regs_t *hc_regs;
	dwc_otg_qtd_t *qtd;
	
	info("--Host Channel Interrupt--, Channel %d\n", _num);

	hc = _dwc_otg_hcd->hc_ptr_array[_num];
	hc_regs = _dwc_otg_hcd->core_if->host_if->hc_regs[_num];
	qtd = list_entry(hc->qh->qtd_list.next, dwc_otg_qtd_t, qtd_list_entry);

	hcint.d32 = dwc_read_reg32(&hc_regs->hcint);
	hcintmsk.d32 = dwc_read_reg32(&hc_regs->hcintmsk);
	info("  hcint 0x%08x, hcintmsk 0x%08x, hcint&hcintmsk 0x%08x\n",
		    hcint.d32, hcintmsk.d32, (hcint.d32 & hcintmsk.d32));
	hcint.d32 = hcint.d32 & hcintmsk.d32;

	if (!_dwc_otg_hcd->core_if->dma_enable) {
		if ((hcint.b.chhltd) && (hcint.d32 != 0x2)) {
			hcint.b.chhltd = 0;
		}
	}

	if (hcint.b.xfercomp) {
		retval |= handle_hc_xfercomp_intr(_dwc_otg_hcd, hc, hc_regs, qtd);
		/*
		 * If NYET occurred at same time as Xfer Complete, the NYET is
		 * handled by the Xfer Complete interrupt handler. Don't want
		 * to call the NYET interrupt handler in this case.
		 */
		hcint.b.nyet = 0;
	}
	if (hcint.b.chhltd) {
		retval |= handle_hc_chhltd_intr(_dwc_otg_hcd, hc, hc_regs, qtd);
	}
	if (hcint.b.ahberr) {
		retval |= handle_hc_ahberr_intr(_dwc_otg_hcd, hc, hc_regs, qtd);
	}
	if (hcint.b.stall) {
		retval |= handle_hc_stall_intr(_dwc_otg_hcd, hc, hc_regs, qtd);
	}
	if (hcint.b.nak) {
		retval |= handle_hc_nak_intr(_dwc_otg_hcd, hc, hc_regs, qtd);
	}
	if (hcint.b.ack) {
		retval |= handle_hc_ack_intr(_dwc_otg_hcd, hc, hc_regs, qtd);
	}
	if (hcint.b.nyet) {
		retval |= handle_hc_nyet_intr(_dwc_otg_hcd, hc, hc_regs, qtd);
	}
	if (hcint.b.xacterr) {
		retval |= handle_hc_xacterr_intr(_dwc_otg_hcd, hc, hc_regs, qtd);
	}
	if (hcint.b.bblerr) {
		retval |= handle_hc_babble_intr(_dwc_otg_hcd, hc, hc_regs, qtd);
	}
	if (hcint.b.frmovrun) {
		retval |= handle_hc_frmovrun_intr(_dwc_otg_hcd, hc, hc_regs, qtd);
	}
	if (hcint.b.datatglerr) {
		retval |= handle_hc_datatglerr_intr(_dwc_otg_hcd, hc, hc_regs, qtd);
	}

	return retval;
}


/** This interrupt indicates that one or more host channels has a pending
 * interrupt. There are multiple conditions that can cause each host channel
 * interrupt. This function determines which conditions have occurred for each
 * host channel interrupt and handles them appropriately. */
int32_t dwc_otg_hcd_handle_hc_intr (dwc_otg_hcd_t *_dwc_otg_hcd)
{
	int i;
	int retval = 0;
	haint_data_t haint;

	/* Clear appropriate bits in HCINTn to clear the interrupt bit in
	 * GINTSTS */

	haint.d32 = dwc_otg_read_host_all_channels_intr(_dwc_otg_hcd->core_if);

	for (i=0; i<_dwc_otg_hcd->core_if->core_params->host_channels; i++) {
		if (haint.b2.chint & (1 << i)) {
			retval |= dwc_otg_hcd_handle_hc_n_intr (_dwc_otg_hcd, i);
		}
	}

	return retval;
}

extern int usb_num_root_hub;

/** This function handles interrupts for the HCD. */
int32_t dwc_otg_hcd_handle_intr (void)
{
        int retval = 0;
        dwc_otg_hcd_t *_dwc_otg_hcd;
        dwc_otg_core_if_t *core_if;
        gintsts_data_t gintsts;
	int unit;
#ifdef DWC_OTG_SHOW_INFO
        dwc_otg_core_global_regs_t *global_regs;
#endif

	for (unit = 0; unit < usb_num_root_hub; unit++) {

	    _dwc_otg_hcd = &dwc_otg_hcd_struct[unit];
	    core_if = _dwc_otg_hcd->core_if;
#ifdef DWC_OTG_SHOW_INFO
            global_regs = core_if->core_global_regs;
#endif

            gintsts.d32 = dwc_otg_read_core_intr(core_if);
            if (!gintsts.d32) {
                return 0;
            }

            info("In %s: unit %d gintsts is 0x%x\n", __FUNCTION__, unit, gintsts.d32);

            if (gintsts.b.sofintr) {
                dwc_otg_hcd_handle_sof_intr(_dwc_otg_hcd);
            }
            if (gintsts.b.rxstsqlvl) {
                dwc_otg_hcd_handle_rx_status_q_level_intr(_dwc_otg_hcd);
            }
            if (gintsts.b.nptxfempty) {
                dwc_otg_hcd_handle_np_tx_fifo_empty_intr(_dwc_otg_hcd);
            }
            if (gintsts.b.portintr) {
                dwc_otg_hcd_handle_port_intr(_dwc_otg_hcd);
            }
            if (gintsts.b.hcintr) {
                dwc_otg_hcd_handle_hc_intr(_dwc_otg_hcd);
            }

#ifdef DWC_OTG_SHOW_INFO
            info("DWC OTG HCD Finished Servicing Interrupts unit %d\n", unit);
            info("DWC OTG HCD gintsts=0x%08x\n", dwc_read_reg32(&global_regs->gintsts));
            info("DWC OTG HCD gintmsk=0x%08x\n", dwc_read_reg32(&global_regs->gintmsk));
#endif
    
        }

        return retval;
}

#endif /* CONFIG_USB_DWC_OTG */
