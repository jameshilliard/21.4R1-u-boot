/*
 * URB OHCI HCD (Host Controller Driver) for USB on the AU1x00.
 *
 * (C) Copyright 2003
 * Gary Jennejohn, DENX Software Engineering <gj@denx.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * Note: Part of this code has been derived from linux
 *
 */

#include <common.h>

#if defined (CONFIG_USB_DWC_OTG)

#include <malloc.h>

#include <usb.h>
#include "dwc_otg_driver.h"
#include "dwc_otg_hcd.h"
#include "dwc_otg_regs.h"
#include <linux/list.h>
#include <octeon_boot.h>


#define m16_swap(x) swap_16(x)
#define m32_swap(x) swap_32(x)

#define min_t(type,x,y) ({ type __x = (x); type __y = (y); __x < __y ? __x: __y; })

dwc_otg_core_params_t dwc_otg_module_params[2] = {
    {
        .opt = -1,
        .otg_cap = -1,
        .dma_enable = -1,
	.dma_burst_size = -1,
	.speed = -1,
	.host_support_fs_ls_low_power = -1,
	.host_ls_low_power_phy_clk = -1,
	.enable_dynamic_fifo = -1,
	.data_fifo_size = -1,
	.dev_rx_fifo_size = -1,
	.dev_nperio_tx_fifo_size = -1,
	.dev_perio_tx_fifo_size = {-1,   /* dev_perio_tx_fifo_size_1 */
				   -1,
				   -1,
				   -1,
				   -1,
				   -1,
				   -1,
				   -1,
				   -1,
				   -1,
				   -1,
				   -1,
				   -1,
				   -1,
				   -1},  /* 15 */
	.host_rx_fifo_size = -1,
	.host_nperio_tx_fifo_size = -1,
	.host_perio_tx_fifo_size = -1,
	.max_transfer_size = -1,
	.max_packet_count = -1,
	.host_channels = -1,
	.dev_endpoints = -1,
	.phy_type = -1,
        .phy_utmi_width = -1,
        .phy_ulpi_ddr = -1,
        .phy_ulpi_ext_vbus = -1,
	.i2c_enable = -1,
	.ulpi_fs_ls = -1,
	.ts_dline = -1
    }, 
    {
        .opt = -1,
        .otg_cap = -1,
        .dma_enable = -1,
	.dma_burst_size = -1,
	.speed = -1,
	.host_support_fs_ls_low_power = -1,
	.host_ls_low_power_phy_clk = -1,
	.enable_dynamic_fifo = -1,
	.data_fifo_size = -1,
	.dev_rx_fifo_size = -1,
	.dev_nperio_tx_fifo_size = -1,
	.dev_perio_tx_fifo_size = {-1,   /* dev_perio_tx_fifo_size_1 */
				   -1,
				   -1,
				   -1,
				   -1,
				   -1,
				   -1,
				   -1,
				   -1,
				   -1,
				   -1,
				   -1,
				   -1,
				   -1,
				   -1},  /* 15 */
	.host_rx_fifo_size = -1,
	.host_nperio_tx_fifo_size = -1,
	.host_perio_tx_fifo_size = -1,
	.max_transfer_size = -1,
	.max_packet_count = -1,
	.host_channels = -1,
	.dev_endpoints = -1,
	.phy_type = -1,
        .phy_utmi_width = -1,
        .phy_ulpi_ddr = -1,
        .phy_ulpi_ext_vbus = -1,
	.i2c_enable = -1,
	.ulpi_fs_ls = -1,
	.ts_dline = -1
    }
};

dwc_otg_core_if_t core_if_struct[DWC_OTG_UNIT_MAX];
dwc_otg_host_if_t host_if_struct[DWC_OTG_UNIT_MAX];
dwc_otg_device_t dwc_otg_device_struct[DWC_OTG_UNIT_MAX];
dwc_otg_hcd_t dwc_otg_hcd_struct[DWC_OTG_UNIT_MAX];
dwc_otg_urb_t urb_priv;

int urb_finished;


/*-------------------------------------------------------------------------*
 * Interface functions (URB)
 *-------------------------------------------------------------------------*/

static void dump_urb_info(dwc_otg_urb_t *_urb, char* _fn_name)
{
        info("%s, urb %p pipe 0x%x unit %d\n", _fn_name, _urb, _urb->pipe, 
             dwc_otg_hcd_get_unit(_urb->dev->devnum));
        info("  Device address: %d\n", usb_pipedevice(_urb->pipe));
        info("  Endpoint: %d, %s\n", usb_pipeendpoint(_urb->pipe),
                  (usb_pipein(_urb->pipe) ? "IN" : "OUT"));
        info("  Endpoint type: %s\n",
                  ({char *pipetype;
                  switch (usb_pipetype(_urb->pipe)) {
                  case PIPE_CONTROL: pipetype = "CONTROL"; break;
                  case PIPE_BULK: pipetype = "BULK"; break;
                  case PIPE_INTERRUPT: pipetype = "INTERRUPT"; break;
                  case PIPE_ISOCHRONOUS: pipetype = "ISOCHRONOUS"; break;
                  default: pipetype = "UNKNOWN"; break;
                  }; pipetype;}));
	// JSRXNLE_FIXME:
	//        info("  Speed: %s\n",
	//                  ({char *speed;
	//                  switch (_urb->dev->speed) {
	//                  case USB_SPEED_HIGH: speed = "HIGH"; break;
	//                  case USB_SPEED_FULL: speed = "FULL"; break;
	//                  case USB_SPEED_LOW: speed = "LOW"; break;
	//                  default: speed = "UNKNOWN"; break;
	//                  }; speed;}));
        info("  Max packet size: %d\n",
                  usb_maxpacket(_urb->dev, _urb->pipe));
        info("  Data buffer length: %d\n", _urb->transfer_buffer_length);
        info("  Transfer buffer: %p\n",
                  _urb->transfer_buffer);
        info("  Setup buffer: %p\n",
                  _urb->setup_packet);
}


/** Starts processing a USB transfer request specified by a USB Request Block
 * (URB). mem_flags indicates the type of memory allocation to use while
 * processing this URB. */
int dwc_otg_hcd_urb_enqueue(dwc_otg_urb_t *_urb)
{
	int retval = 0;
	dwc_otg_hcd_t *dwc_otg_hcd;
	dwc_otg_qtd_t *qtd;
	
	dwc_otg_transaction_type_e tr_type;
	struct devrequest *setup_pkt;

#ifdef DWC_OTG_SHOW_INFO
        char *Xbuf;
	int i;
#endif

	dwc_otg_hcd = &dwc_otg_hcd_struct[dwc_otg_hcd_get_unit(_urb->dev->devnum)];

	dump_urb_info(_urb, "dwc_otg_hcd_urb_enqueue");

#ifdef DWC_OTG_SHOW_INFO
	Xbuf = (char *) _urb->transfer_buffer;
	if (_urb->transfer_buffer_length >= 8) {
	    info("Xbuf:\n");
	    for (i=0; i<_urb->transfer_buffer_length ; i++) {
		printf("%02x ", Xbuf[i]);
		if ((i+1)%8==0 && i!=0)
		    printf("\n");
	    }
	}
	printf("\n");
#endif

	setup_pkt = (struct devrequest *) _urb->setup_packet;
	if (setup_pkt)
	    info("Setup pkt: 0x%02x 0x%02x 0x%x 0x%x 0x%x\n",  
		   setup_pkt->requesttype, setup_pkt->request, setup_pkt->value, 
		   setup_pkt->index, setup_pkt->length);

	if (!dwc_otg_hcd->flags.b.port_connect_status) {
		/* No longer connected. */
		printf("DWC OTG HCD URB Enqueue port not connected\n");
		return -DWC_OTG_ENODEV;
	}


	qtd = dwc_otg_hcd_qtd_create (_urb);
	if (qtd == NULL) {
		printf("DWC OTG HCD URB Enqueue failed creating QTD\n");
		return -DWC_OTG_ENOMEM;
	}

	retval = dwc_otg_hcd_qtd_add (qtd, dwc_otg_hcd);
	if (retval < 0) {
		printf("DWC OTG HCD URB Enqueue failed adding QTD. "
			  "Error status %d\n", retval);
		dwc_otg_hcd_qtd_free(qtd);
	}

	tr_type = dwc_otg_hcd_select_transactions(dwc_otg_hcd);
	if (tr_type != DWC_OTG_TRANSACTION_NONE) {
	    dwc_otg_hcd_queue_transactions(dwc_otg_hcd, tr_type);
	}

	return retval;
}

/**
 * Assigns transactions from a QTD to a free host channel and initializes the
 * host channel to perform the transactions. The host channel is removed from
 * the free list.
 *
 * @param _hcd The HCD state structure.
 * @param _qh Transactions from the first QTD for this QH are selected and
 * assigned to a free host channel.
 */
static void assign_and_init_hc(dwc_otg_hcd_t *_hcd, dwc_otg_qh_t *_qh)
{
	dwc_hc_t	*hc;
	dwc_otg_qtd_t	*qtd;
	dwc_otg_urb_t	*urb;

	info("%s(%p,%p)\n", __FUNCTION__, _hcd, _qh);

	hc = list_entry(_hcd->free_hc_list.next, dwc_hc_t, hc_list_entry);

	/* Remove the host channel from the free list. */
	list_del_init(&hc->hc_list_entry);

	qtd = list_entry(_qh->qtd_list.next, dwc_otg_qtd_t, qtd_list_entry);
	urb = qtd->urb;
	_qh->channel = hc;
	_qh->qtd_in_process = qtd;

	/*
	 * Use usb_pipedevice to determine device address. This address is
	 * 0 before the SET_ADDRESS command and the correct address afterward.
	 */
	hc->dev_addr = usb_pipedevice(urb->pipe);
	hc->ep_num = usb_pipeendpoint(urb->pipe);

	//	if (urb->dev->speed == USB_SPEED_LOW) {
	//		hc->speed = DWC_OTG_EP_SPEED_LOW;
		//	} else if (urb->dev->speed == USB_SPEED_FULL) {
	/* JSRXNLE_FIXME: Check if this is correct */
	//	hc->speed = DWC_OTG_EP_SPEED_FULL;
		//	} else {
	hc->speed = DWC_OTG_EP_SPEED_HIGH;
		//	}

	hc->max_packet = dwc_max_packet(_qh->maxp);

	hc->xfer_started = 0;
	hc->halt_status = DWC_OTG_HC_XFER_NO_HALT_STATUS;
	hc->error_state = (qtd->error_count > 0);
	hc->halt_on_queue = 0;
	hc->halt_pending = 0;
	hc->requests = 0;

	/*
	 * The following values may be modified in the transfer type section
	 * below. The xfer_len value may be reduced when the transfer is
	 * started to accommodate the max widths of the XferSize and PktCnt
	 * fields in the HCTSIZn register.
	 */
	hc->do_ping = _qh->ping_state;
	hc->ep_is_in = (usb_pipein(urb->pipe) != 0);
	hc->data_pid_start = _qh->data_toggle;
	hc->multi_count = 1;

	if (!_hcd->core_if->dma_enable) {
		hc->xfer_buff = (uint8_t *)urb->transfer_buffer + urb->actual_length;
	}
	hc->xfer_len = urb->transfer_buffer_length - urb->actual_length;
	hc->xfer_count = 0;

       /*
         * Set the split attributes
         */
        hc->do_split = 0;
        if (_qh->do_split) {
                hc->do_split = 1;
                hc->xact_pos = qtd->isoc_split_pos;
                hc->complete_split = qtd->complete_split;
		/* JSRXNLE_FIXME: Fix these values */
                hc->hub_addr = 1; //urb->dev->tt->hub->devnum;
                hc->port_addr = 1; //urb->dev->ttport;
        }

	switch (usb_pipetype(urb->pipe)) {
	case PIPE_CONTROL:
		hc->ep_type = DWC_OTG_EP_TYPE_CONTROL;
		switch (qtd->control_phase) {
		case DWC_OTG_CONTROL_SETUP:
			info("  Control setup transaction\n");
			hc->do_ping = 0;
			hc->ep_is_in = 0;
			hc->data_pid_start = DWC_OTG_HC_PID_SETUP;
			if (!_hcd->core_if->dma_enable) {
			    hc->xfer_buff = (uint8_t *)urb->setup_packet;
			}
			hc->xfer_len = 8;
			break;
		case DWC_OTG_CONTROL_DATA:
			info("  Control data transaction\n");
			hc->data_pid_start = qtd->data_toggle;
			break;
		case DWC_OTG_CONTROL_STATUS:
			/*
			 * Direction is opposite of data direction or IN if no
			 * data.
			 */
			info("  Control status transaction\n");
			if (urb->transfer_buffer_length == 0) {
				hc->ep_is_in = 1;
			} else {
				/* We want opposite of data direction */
				hc->ep_is_in = usb_pipein(urb->pipe) ? 0 : 1;
			}
			if (hc->ep_is_in) {
				hc->do_ping = 0;
			}
			hc->data_pid_start = DWC_OTG_HC_PID_DATA1;
			hc->xfer_len = 0;
			if (_hcd->core_if->dma_enable) {
				hc->xfer_buff = (uint8_t *)_hcd->status_buf_dma;
			} else {
				hc->xfer_buff = (uint8_t *)_hcd->status_buf;
			}
			break;
		}
		break;
	case PIPE_BULK:
		hc->ep_type = DWC_OTG_EP_TYPE_BULK;
		break;
	case PIPE_INTERRUPT:
                printf("%s: Interrupt mode not supported\n");
		break;
	case PIPE_ISOCHRONOUS:
                printf("%s: Isochronous not supported\n");
		break;
	}

	dwc_otg_hc_init(_hcd->core_if, hc);
	hc->qh = _qh;
}

/**
 * This function selects transactions from the HCD transfer schedule and
 * assigns them to available host channels. It is called from HCD interrupt
 * handler functions.
 *
 * @param _hcd The HCD state structure.
 *
 * @return The types of new transactions that were assigned to host channels.
 */
dwc_otg_transaction_type_e dwc_otg_hcd_select_transactions(dwc_otg_hcd_t *_hcd)
{
	struct list_head 		*qh_ptr;
	dwc_otg_qh_t 			*qh;
	int				num_channels;
	dwc_otg_transaction_type_e	ret_val = DWC_OTG_TRANSACTION_NONE;

	/*
	 * Process entries in the inactive portion of the non-periodic
	 * schedule. Some free host channels may not be used if they are
	 * reserved for periodic transfers.
	 */
	qh_ptr = _hcd->non_periodic_sched_inactive.next;
	num_channels = _hcd->core_if->core_params->host_channels;
	while (qh_ptr != &_hcd->non_periodic_sched_inactive &&
	       (_hcd->non_periodic_channels < num_channels) &&
	       !list_empty(&_hcd->free_hc_list)) {

		qh = list_entry(qh_ptr, dwc_otg_qh_t, qh_list_entry);
		assign_and_init_hc(_hcd, qh);

		/*
		 * Move the QH from the non-periodic inactive schedule to the
		 * non-periodic active schedule.
		 */
		qh_ptr = qh_ptr->next;
		list_move(&qh->qh_list_entry, &_hcd->non_periodic_sched_active);

		if (ret_val == DWC_OTG_TRANSACTION_NONE) {
			ret_val = DWC_OTG_TRANSACTION_NON_PERIODIC;
		} else {
			ret_val = DWC_OTG_TRANSACTION_ALL;
		}

		_hcd->non_periodic_channels++;
	}

	return ret_val;
}

/**
 * Attempts to queue a single transaction request for a host channel
 * associated with either a periodic or non-periodic transfer. This function
 * assumes that there is space available in the appropriate request queue. For
 * an OUT transfer or SETUP transaction in Slave mode, it checks whether space
 * is available in the appropriate Tx FIFO.
 *
 * @param _hcd The HCD state structure.
 * @param _hc Host channel descriptor associated with either a periodic or
 * non-periodic transfer.
 * @param _fifo_dwords_avail Number of DWORDs available in the periodic Tx
 * FIFO for periodic transfers or the non-periodic Tx FIFO for non-periodic
 * transfers.
 *
 * @return 1 if a request is queued and more requests may be needed to
 * complete the transfer, 0 if no more requests are required for this
 * transfer, -1 if there is insufficient space in the Tx FIFO.
 */
static int queue_transaction(dwc_otg_hcd_t *_hcd,
			     dwc_hc_t *_hc,
			     uint16_t _fifo_dwords_avail)
{
	int retval;

	if (_hcd->core_if->dma_enable) {
		if (!_hc->xfer_started) {
			dwc_otg_hc_start_transfer(_hcd->core_if, _hc);
			_hc->qh->ping_state = 0;
		}
		retval = 0;
	} else 	if (_hc->halt_pending) {
		/* Don't queue a request if the channel has been halted. */
		retval = 0;
	} else if (_hc->halt_on_queue) {
	    /* JSRXNLE_FIXME */
	    //		dwc_otg_hc_halt(_hcd->core_if, _hc, _hc->halt_status);
		retval = 0;
	} else if (_hc->do_ping) {
		if (!_hc->xfer_started) {
			dwc_otg_hc_start_transfer(_hcd->core_if, _hc);
		}
		retval = 0;
	} else if (!_hc->ep_is_in ||
		   _hc->data_pid_start == DWC_OTG_HC_PID_SETUP) {
		if ((_fifo_dwords_avail * 4) >= _hc->max_packet) {
			if (!_hc->xfer_started) {
				dwc_otg_hc_start_transfer(_hcd->core_if, _hc);
				retval = 1;
			} else {
				retval = dwc_otg_hc_continue_transfer(_hcd->core_if, _hc);
			}
		} else {
			retval = -1;
		}
	} else {		
		if (!_hc->xfer_started) {
			dwc_otg_hc_start_transfer(_hcd->core_if, _hc);
			retval = 1;
		} else {
			retval = dwc_otg_hc_continue_transfer(_hcd->core_if, _hc);
		}
	}

	return retval;
}

/**
 * Processes active non-periodic channels and queues transactions for these
 * channels to the DWC_otg controller. After queueing transactions, the NP Tx
 * FIFO Empty interrupt is enabled if there are more transactions to queue as
 * NP Tx FIFO or request queue space becomes available. Otherwise, the NP Tx
 * FIFO Empty interrupt is disabled.
 */
static void process_non_periodic_channels(dwc_otg_hcd_t *_hcd)
{
	gnptxsts_data_t		tx_status;
	struct list_head	*orig_qh_ptr;
	dwc_otg_qh_t		*qh;
	int			status;
	int			no_queue_space = 0;
	int			no_fifo_space = 0;
	int			more_to_do = 0;

	dwc_otg_core_global_regs_t *global_regs = _hcd->core_if->core_global_regs;

#ifdef DWC_OTG_SHOW_INFO	
	tx_status.d32 = dwc_read_reg32(&global_regs->gnptxsts);
	info("  NP Tx Req Queue Space Avail (before queue): %d\n",
		    tx_status.b.nptxqspcavail);
	info("  NP Tx FIFO Space Avail (before queue): %d\n",
		    tx_status.b.nptxfspcavail);
#endif
	/*
	 * Keep track of the starting point. Skip over the start-of-list
	 * entry.
	 */
	if (_hcd->non_periodic_qh_ptr == &_hcd->non_periodic_sched_active) {
		_hcd->non_periodic_qh_ptr = _hcd->non_periodic_qh_ptr->next;
	}
	orig_qh_ptr = _hcd->non_periodic_qh_ptr;

	/*
	 * Process once through the active list or until no more space is
	 * available in the request queue or the Tx FIFO.
	 */
	do {
		tx_status.d32 = dwc_read_reg32(&global_regs->gnptxsts);
		if (!_hcd->core_if->dma_enable && tx_status.b.nptxqspcavail == 0) {
			no_queue_space = 1;
			break;
		}

		qh = list_entry(_hcd->non_periodic_qh_ptr, dwc_otg_qh_t, qh_list_entry);
		status = queue_transaction(_hcd, qh->channel, tx_status.b.nptxfspcavail);

		if (status > 0) {
			more_to_do = 1;
		} else if (status < 0) {
			no_fifo_space = 1;
			break;
		}

		/* Advance to next QH, skipping start-of-list entry. */
		_hcd->non_periodic_qh_ptr = _hcd->non_periodic_qh_ptr->next;
		if (_hcd->non_periodic_qh_ptr == &_hcd->non_periodic_sched_active) {
			_hcd->non_periodic_qh_ptr = _hcd->non_periodic_qh_ptr->next;
		}

	} while (_hcd->non_periodic_qh_ptr != orig_qh_ptr);
		
	if (!_hcd->core_if->dma_enable) {
		gintmsk_data_t intr_mask = {.d32 = 0};
		intr_mask.b.nptxfempty = 1;

#ifdef DWC_OTG_SHOW_INFO	
		tx_status.d32 = dwc_read_reg32(&global_regs->gnptxsts);
		info("  NP Tx Req Queue Space Avail (after queue): %d\n",
			    tx_status.b.nptxqspcavail);
		info("  NP Tx FIFO Space Avail (after queue): %d\n",
			    tx_status.b.nptxfspcavail);
#endif

		if (more_to_do || no_queue_space || no_fifo_space) {
			/*
			 * May need to queue more transactions as the request
			 * queue or Tx FIFO empties. Enable the non-periodic
			 * Tx FIFO empty interrupt. (Always use the half-empty
			 * level to ensure that new requests are loaded as
			 * soon as possible.)
			 */
			dwc_modify_reg32(&global_regs->gintmsk, 0, intr_mask.d32);
		} else {
			/*
			 * Disable the Tx FIFO empty interrupt since there are
			 * no more transactions that need to be queued right
			 * now. This function is called from interrupt
			 * handlers to queue more transactions as transfer
			 * states change.
			 */
			dwc_modify_reg32(&global_regs->gintmsk, intr_mask.d32, 0);
		}
	}
}

/**
 * This function processes the currently active host channels and queues
 * transactions for these channels to the DWC_otg controller. It is called
 * from HCD interrupt handler functions.
 *
 * @param _hcd The HCD state structure.
 * @param _tr_type The type(s) of transactions to queue (non-periodic,
 * periodic, or both).
 */
void dwc_otg_hcd_queue_transactions(dwc_otg_hcd_t *_hcd,
				    dwc_otg_transaction_type_e _tr_type)
{
	/* Process host channels associated with non-periodic transfers. */
	if ((_tr_type == DWC_OTG_TRANSACTION_NON_PERIODIC ||
	     _tr_type == DWC_OTG_TRANSACTION_ALL)) {
		if (!list_empty(&_hcd->non_periodic_sched_active)) {
			process_non_periodic_channels(_hcd);
		} else {
			/*
			 * Ensure NP Tx FIFO empty interrupt is disabled when
			 * there are no non-periodic transfers to process.
			 */
		        info("Disabling nptxfempty\n");
			gintmsk_data_t gintmsk = {.d32 = 0};
			gintmsk.b.nptxfempty = 1;
			dwc_modify_reg32(&_hcd->core_if->core_global_regs->gintmsk,
					 gintmsk.d32, 0);
		}
	}
}

/**
 * Sets the final status of an URB and returns it to the device driver. Any
 * required cleanup of the URB is performed.
 */
void dwc_otg_hcd_complete_urb(dwc_otg_hcd_t *_hcd, dwc_otg_urb_t *_urb, int _status)
{

	_urb->status = _status;
	_urb->hcpriv = NULL;

	dbg("%s:\n", __FUNCTION__);
	urb_finished = 1;
}



static int dwc_otg_submit_job (struct usb_device *dev, unsigned long pipe, void *buffer,
				int transfer_len, struct devrequest *setup )
{
	dwc_otg_urb_t *urb;

	urb = &urb_priv;
        urb->dev = dev;
	urb->pipe = pipe;
	urb->transfer_buffer = buffer;
	urb->transfer_buffer_length = transfer_len;
	urb->setup_packet = (unsigned char *) setup;
	urb->actual_length = 0;

	if (dwc_otg_hcd_urb_enqueue(urb) < 0) {
	    printf("%s: Error enqueueing URB\n", __FUNCTION__);
	    return -1;
	}

	return 0;
}

/*-------------------------------------------------------------------------*
 * Virtual Root Hub
 *-------------------------------------------------------------------------*/

/* Device descriptor */
static __u8 root_hub_dev_des[] =
{
	0x12,	    /*	__u8  bLength; */
	0x01,	    /*	__u8  bDescriptorType; Device */
	0x10,	    /*	__u16 bcdUSB; v1.1 */
	0x01,
	0x09,	    /*	__u8  bDeviceClass; HUB_CLASSCODE */
	0x00,	    /*	__u8  bDeviceSubClass; */
	0x00,	    /*	__u8  bDeviceProtocol; */
	0x08,	    /*	__u8  bMaxPacketSize0; 8 Bytes */
	0x00,	    /*	__u16 idVendor; */
	0x00,
	0x00,	    /*	__u16 idProduct; */
	0x00,
	0x00,	    /*	__u16 bcdDevice; */
	0x00,
	0x00,	    /*	__u8  iManufacturer; */
	0x01,	    /*	__u8  iProduct; */
	0x00,	    /*	__u8  iSerialNumber; */
	0x01	    /*	__u8  bNumConfigurations; */
};


/* Configuration descriptor */
static __u8 root_hub_config_des[] =
{
	0x09,	    /*	__u8  bLength; */
	0x02,	    /*	__u8  bDescriptorType; Configuration */
	0x19,	    /*	__u16 wTotalLength; */
	0x00,
	0x01,	    /*	__u8  bNumInterfaces; */
	0x01,	    /*	__u8  bConfigurationValue; */
	0x00,	    /*	__u8  iConfiguration; */
	0x40,	    /*	__u8  bmAttributes;
		 Bit 7: Bus-powered, 6: Self-powered, 5 Remote-wakwup, 4..0: resvd */
	0x00,	    /*	__u8  MaxPower; */

	/* interface */
	0x09,	    /*	__u8  if_bLength; */
	0x04,	    /*	__u8  if_bDescriptorType; Interface */
	0x00,	    /*	__u8  if_bInterfaceNumber; */
	0x00,	    /*	__u8  if_bAlternateSetting; */
	0x01,	    /*	__u8  if_bNumEndpoints; */
	0x09,	    /*	__u8  if_bInterfaceClass; HUB_CLASSCODE */
	0x00,	    /*	__u8  if_bInterfaceSubClass; */
	0x00,	    /*	__u8  if_bInterfaceProtocol; */
	0x00,	    /*	__u8  if_iInterface; */

	/* endpoint */
	0x07,	    /*	__u8  ep_bLength; */
	0x05,	    /*	__u8  ep_bDescriptorType; Endpoint */
	0x81,	    /*	__u8  ep_bEndpointAddress; IN Endpoint 1 */
	0x03,	    /*	__u8  ep_bmAttributes; Interrupt */
	0x02,	    /*	__u16 ep_wMaxPacketSize; ((MAX_ROOT_PORTS + 1) / 8 */
	0x00,
	0xff	    /*	__u8  ep_bInterval; 255 ms */
};

static unsigned char root_hub_str_index0[] =
{
	0x04,			/*  __u8  bLength; */
	0x03,			/*  __u8  bDescriptorType; String-descriptor */
	0x09,			/*  __u8  lang ID */
	0x04,			/*  __u8  lang ID */
};

static unsigned char root_hub_str_index1[] =
{
	28,			/*  __u8  bLength; */
	0x03,			/*  __u8  bDescriptorType; String-descriptor */
	'D',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'O',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'T',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'G',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	' ',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'R',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'o',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'o',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	't',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	' ',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'H',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'u',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'b',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
};


#define OK(x)			len = (x); break



/** Handles hub class-specific requests.*/
int dwc_otg_hcd_hub_control(int hubnum, 
			    uint16_t _typeReq,
			    uint16_t _wValue,
			    uint16_t _wIndex,
			    char *_buf,
			    uint16_t _wLength)
{
	int retval = 0;

        dwc_otg_hcd_t *dwc_otg_hcd;
        dwc_otg_core_if_t *core_if;
	struct usb_hub_descriptor *desc;
	hprt0_data_t hprt0 = {.d32 = 0};

	uint32_t port_status;

	dbg("%s: unit %d\n", __FUNCTION__, hubnum);

        dwc_otg_hcd = &dwc_otg_hcd_struct[hubnum];
        core_if = dwc_otg_hcd->core_if;

	dbg("    ");

	switch (_typeReq) {
	case ClearHubFeature:
		info("DWC OTG HCD HUB CONTROL - "
			    "ClearHubFeature 0x%x\n", _wValue);
		switch (_wValue) {
		case C_HUB_LOCAL_POWER:
		case C_HUB_OVER_CURRENT:
			/* Nothing required here */
			break;
		default:
			retval = USB_ST_STALLED;
			printf("DWC OTG HCD - "
				   "ClearHubFeature request %xh unknown\n", _wValue);
		}
		break;
	case ClearPortFeature:
		if (!_wIndex || _wIndex > 1)
			goto error;

		switch (_wValue) {
		case USB_PORT_FEATURE_ENABLE:
			info("DWC OTG HCD HUB CONTROL - "
			       "ClearPortFeature USB_PORT_FEATURE_ENABLE\n");
			hprt0.d32 = dwc_otg_read_hprt0 (core_if);
			hprt0.b.prtena = 1;
			dwc_write_reg32(core_if->host_if->hprt0, hprt0.d32);
			break;
		case USB_PORT_FEATURE_SUSPEND:
			info("DWC OTG HCD HUB CONTROL - "
				     "ClearPortFeature USB_PORT_FEATURE_SUSPEND\n");
			hprt0.d32 = dwc_otg_read_hprt0 (core_if);
			hprt0.b.prtres = 1;
			dwc_write_reg32(core_if->host_if->hprt0, hprt0.d32);
			/* Clear Resume bit */
			mdelay (100);
			hprt0.b.prtres = 0;
			dwc_write_reg32(core_if->host_if->hprt0, hprt0.d32);
			break;
		case USB_PORT_FEATURE_POWER:
			info("DWC OTG HCD HUB CONTROL - "
				     "ClearPortFeature USB_PORT_FEATURE_POWER\n");
			hprt0.d32 = dwc_otg_read_hprt0 (core_if);
			hprt0.b.prtpwr = 0;
			dwc_write_reg32(core_if->host_if->hprt0, hprt0.d32);
			break;
		case USB_PORT_FEATURE_INDICATOR:
			info("DWC OTG HCD HUB CONTROL - "
				     "ClearPortFeature USB_PORT_FEATURE_INDICATOR\n");
			/* Port inidicator not supported */
			break;
		case USB_PORT_FEATURE_C_CONNECTION:
			/* Clears drivers internal connect status change
			 * flag */
			info("DWC OTG HCD HUB CONTROL - "
				     "ClearPortFeature USB_PORT_FEATURE_C_CONNECTION\n");
			dwc_otg_hcd->flags.b.port_connect_status_change = 0;
			break;
		case USB_PORT_FEATURE_C_RESET:
			/* Clears the driver's internal Port Reset Change
			 * flag */
			info("DWC OTG HCD HUB CONTROL - "
				     "ClearPortFeature USB_PORT_FEATURE_C_RESET\n");
			dwc_otg_hcd->flags.b.port_reset_change = 0;
			break;
		case USB_PORT_FEATURE_C_ENABLE:
			/* Clears the driver's internal Port
			 * Enable/Disable Change flag */
			info("DWC OTG HCD HUB CONTROL - "
				     "ClearPortFeature USB_PORT_FEATURE_C_ENABLE\n");
			dwc_otg_hcd->flags.b.port_enable_change = 0;
			break;
		case USB_PORT_FEATURE_C_SUSPEND:
			/* Clears the driver's internal Port Suspend
			 * Change flag, which is set when resume signaling on
			 * the host port is complete */
			info("DWC OTG HCD HUB CONTROL - "
				     "ClearPortFeature USB_PORT_FEATURE_C_SUSPEND\n");
			dwc_otg_hcd->flags.b.port_suspend_change = 0;
			break;
		case USB_PORT_FEATURE_C_OVER_CURRENT:
			info("DWC OTG HCD HUB CONTROL - "
				     "ClearPortFeature USB_PORT_FEATURE_C_OVER_CURRENT\n");
			dwc_otg_hcd->flags.b.port_over_current_change = 0;
			break;
		default:
			retval = USB_ST_STALLED;
			printf("DWC OTG HCD - "
                                   "ClearPortFeature request %xh "
                                   "unknown or unsupported\n", _wValue);
		}
		break;
	case GetHubDescriptor:
		info("DWC OTG HCD HUB CONTROL - GetHubDescriptor\n");
		desc = (struct usb_hub_descriptor *)_buf;
		desc->bLength = 9;
		desc->bDescriptorType = 0x29;
		desc->bNbrPorts = 1;
		desc->wHubCharacteristics = 0x08;
		desc->bPwrOn2PwrGood = 1;
		desc->bHubContrCurrent = 0;
#define bitmap  DeviceRemovable
		desc->bitmap[0] = 0;
		desc->bitmap[1] = 0xff;
		break;
	case GetHubStatus:
		info("DWC OTG HCD HUB CONTROL - GetHubStatus\n");
		memset (_buf, 0, 4);
		break;
	case GetPortStatus:
		info("DWC OTG HCD HUB CONTROL - GetPortStatus\n");

		if (!_wIndex || _wIndex > 1)
			goto error;

		port_status = 0;

		if (dwc_otg_hcd->flags.b.port_connect_status_change)
			port_status |= (1 << USB_PORT_FEATURE_C_CONNECTION);

		if (dwc_otg_hcd->flags.b.port_enable_change)
			port_status |= (1 << USB_PORT_FEATURE_C_ENABLE);

		if (dwc_otg_hcd->flags.b.port_suspend_change)
			port_status |= (1 << USB_PORT_FEATURE_C_SUSPEND);

		if (dwc_otg_hcd->flags.b.port_reset_change)
			port_status |= (1 << USB_PORT_FEATURE_C_RESET);

		if (dwc_otg_hcd->flags.b.port_over_current_change) {
			printf("Device Not Supported\n");
			port_status |= (1 << USB_PORT_FEATURE_C_OVER_CURRENT);
		}

		info("port_status %x port_connect_status %x\n", port_status, 
		     dwc_otg_hcd->flags.b.port_connect_status);

		if (!dwc_otg_hcd->flags.b.port_connect_status) {
			/*
			 * The port is disconnected, which means the core is
			 * either in device mode or it soon will be. Just
			 * return 0's for the remainder of the port status
			 * since the port register can't be read if the core
			 * is in device mode.
			 */
			*((volatile uint32_t *) _buf) = cpu_to_le32(port_status);
			break;
		}

		hprt0.d32 = dwc_read_reg32(core_if->host_if->hprt0);
		info("  HPRT0: 0x%08x\n", hprt0.d32);

		if (hprt0.b.prtconnsts)
			port_status |= (1 << USB_PORT_FEATURE_CONNECTION);

		if (hprt0.b.prtena)
			port_status |= (1 << USB_PORT_FEATURE_ENABLE);

		if (hprt0.b.prtsusp)
			port_status |= (1 << USB_PORT_FEATURE_SUSPEND);

		if (hprt0.b.prtovrcurract)
			port_status |= (1 << USB_PORT_FEATURE_OVER_CURRENT);

		if (hprt0.b.prtrst)
			port_status |= (1 << USB_PORT_FEATURE_RESET);

		if (hprt0.b.prtpwr)
			port_status |= (1 << USB_PORT_FEATURE_POWER);

		if (hprt0.b.prtspd == DWC_HPRT0_PRTSPD_HIGH_SPEED)
			port_status |= (1 << USB_PORT_FEATURE_HIGHSPEED);
		else if (hprt0.b.prtspd == DWC_HPRT0_PRTSPD_LOW_SPEED)
			port_status |= (1 << USB_PORT_FEATURE_LOWSPEED);

		if (hprt0.b.prttstctl)
			port_status |= (1 << USB_PORT_FEATURE_TEST);

		/* USB_PORT_FEATURE_INDICATOR unsupported always 0 */

		*((volatile uint32_t *) _buf) = cpu_to_le32(port_status);

		break;
	case SetHubFeature:
		info("DWC OTG HCD HUB CONTROL - SetHubFeature\n");
		/* No HUB features supported */
		break;
	case SetPortFeature:
		if (_wValue != USB_PORT_FEATURE_TEST && (!_wIndex || _wIndex > 1))
			goto error;

		if (!dwc_otg_hcd->flags.b.port_connect_status) {
			/*
			 * The port is disconnected, which means the core is
			 * either in device mode or it soon will be. Just
			 * return without doing anything since the port
			 * register can't be written if the core is in device
			 * mode.
			 */
			break;
		}

		switch (_wValue) {
		case USB_PORT_FEATURE_SUSPEND:
			info("DWC OTG HCD HUB CONTROL - "
				     "SetPortFeature - USB_PORT_FEATURE_SUSPEND\n");
                        hprt0.d32 = dwc_otg_read_hprt0 (core_if);
                        hprt0.b.prtsusp = 1;
			dwc_write_reg32(core_if->host_if->hprt0, hprt0.d32);
                        info( "SUSPEND: HPRT0=%0x\n", hprt0.d32);
                        /* Suspend the Phy Clock */
                        {
                                pcgcctl_data_t pcgcctl = {.d32=0};
                                pcgcctl.b.stoppclk = 1;
                                dwc_write_reg32(core_if->pcgcctl, pcgcctl.d32);
                        }

                        mdelay(200);
			break;
		case USB_PORT_FEATURE_POWER:
			info("DWC OTG HCD HUB CONTROL - "
				     "SetPortFeature - USB_PORT_FEATURE_POWER\n");
			hprt0.d32 = dwc_otg_read_hprt0 (core_if);
			hprt0.b.prtpwr = 1;
			dwc_write_reg32(core_if->host_if->hprt0, hprt0.d32);
			break;
		case USB_PORT_FEATURE_RESET:
			info("DWC OTG HCD HUB CONTROL - "
				     "SetPortFeature - USB_PORT_FEATURE_RESET\n");
			hprt0.d32 = dwc_otg_read_hprt0 (core_if);
                        /* When B-Host the Port reset bit is set in
                         * the Start HCD Callback function, so that
                         * the reset is started within 1ms of the HNP
                         * success interrupt. */
			hprt0.b.prtrst = 1;
			dwc_write_reg32(core_if->host_if->hprt0, hprt0.d32);
			/* Clear reset bit in 10ms (FS/LS) or 50ms (HS) */
			mdelay (60);
			hprt0.b.prtrst = 0;
			dwc_write_reg32(core_if->host_if->hprt0, hprt0.d32);
			break;


		case USB_PORT_FEATURE_INDICATOR:
			info("DWC OTG HCD HUB CONTROL - "
				     "SetPortFeature - USB_PORT_FEATURE_INDICATOR\n");
			/* Not supported */
			break;
		default:
			retval = USB_ST_STALLED;
			printf("DWC OTG HCD - "
				   "SetPortFeature request %xh "
				   "unknown or unsupported\n", _wValue);
			break;
		}
		break;
	default:
	error:
		retval = USB_ST_STALLED;
		printf("DWC OTG HCD - "
		       "Unknown hub control request type or invalid"
		       " typeReq: %xh wIndex: %xh wValue: %xh\n",
		       _typeReq, _wIndex, _wValue);
		break;
	}

	return retval;
}


static int dwc_otg_submit_rh_msg(struct usb_device *dev, unsigned long pipe,
		void *buffer, int transfer_len, struct devrequest *cmd)
{
	void * data = buffer;
	int leni = transfer_len;
	int len = 0;
	int stat = 0;
	uint32_t datab[4];
        uint8_t  *data_buf = (__u8 *)datab;
	uint16_t bmRType_bReq;
	uint16_t typeReq;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
	int hubnum=0;

	wait_ms(1);

	bmRType_bReq  = cmd->requesttype | (cmd->request << 8);
	wValue	      = m16_swap (cmd->value);
	wIndex	      = m16_swap (cmd->index);
	wLength	      = m16_swap (cmd->length);

	info("Root-Hub: adr: %2x cmd(%1x): %08x %04x %04x %04x\n",
	     dev->devnum, 8, bmRType_bReq, wValue, wIndex, wLength);

	typeReq  = (cmd->requesttype << 8) | cmd->request;

	if (usb_multi_root_hub)
		hubnum = dwc_otg_hcd_get_unit(dev->devnum); 

	dbg("%s: unit %d\n", __FUNCTION__, hubnum);

	switch (bmRType_bReq) {
	/* Request Destination:
	   without flags: Device,
	   RH_INTERFACE: interface,
	   RH_ENDPOINT: endpoint,
	   RH_CLASS means HUB here,
	   RH_OTHER | RH_CLASS	almost ever means HUB_PORT here
	*/

	case RH_GET_STATUS:
			*(__u16 *) data_buf = m16_swap (1); OK (2);
	case RH_GET_STATUS | RH_INTERFACE:
			*(__u16 *) data_buf = m16_swap (0); OK (2);
	case RH_GET_STATUS | RH_ENDPOINT:
			*(__u16 *) data_buf = m16_swap (0); OK (2);
	case RH_GET_STATUS | RH_CLASS:
	    stat = dwc_otg_hcd_hub_control(hubnum, typeReq, wValue,wIndex, data_buf, wLength); OK (4);
	case RH_GET_STATUS | RH_OTHER | RH_CLASS:
	    stat = dwc_otg_hcd_hub_control(hubnum, typeReq, wValue,wIndex, data_buf, wLength); OK (4);
	case RH_CLEAR_FEATURE | RH_ENDPOINT:
		switch (wValue) {
			case (RH_ENDPOINT_STALL): OK (0);
		}
		break;

	case RH_CLEAR_FEATURE | RH_CLASS:
	    stat = dwc_otg_hcd_hub_control(hubnum, typeReq, wValue,wIndex, data_buf, wLength); OK (0);

	case RH_CLEAR_FEATURE | RH_OTHER | RH_CLASS:
	    stat = dwc_otg_hcd_hub_control(hubnum, typeReq, wValue,wIndex, data_buf, wLength); OK (0);
	case RH_SET_FEATURE | RH_OTHER | RH_CLASS: 
	    stat = dwc_otg_hcd_hub_control(hubnum, typeReq, wValue,wIndex, data_buf, wLength); OK (0);
	case RH_SET_ADDRESS: 
	    dbg("%s: hubnum %d devnum %d pipe_devnum %d wValue %d\n", 
		   __FUNCTION__, hubnum, dev->devnum, ((pipe >> 8) & 0x7f), wValue); 
	    dwc_otg_hcd_struct[hubnum].rh.devnum = wValue; OK(0);
	case RH_GET_DESCRIPTOR:
		switch ((wValue & 0xff00) >> 8) {
			case (0x01): /* device descriptor */
				len = min_t(unsigned int,
					  leni,
					  min_t(unsigned int,
					      sizeof (root_hub_dev_des),
					      wLength));
				data_buf = root_hub_dev_des; OK(len);
			case (0x02): /* configuration descriptor */
				len = min_t(unsigned int,
					  leni,
					  min_t(unsigned int,
					      sizeof (root_hub_config_des),
					      wLength));
				data_buf = root_hub_config_des; OK(len);
			case (0x03): /* string descriptors */
				if(wValue==0x0300) {
					len = min_t(unsigned int,
						  leni,
						  min_t(unsigned int,
						      sizeof (root_hub_str_index0),
						      wLength));
					data_buf = root_hub_str_index0;
					OK(len);
				}
				if(wValue==0x0301) {
					len = min_t(unsigned int,
						  leni,
						  min_t(unsigned int,
						      sizeof (root_hub_str_index1),
						      wLength));
					data_buf = root_hub_str_index1;
					OK(len);
			}
			default:
				stat = USB_ST_STALLED;
		}
		break;

	case RH_GET_DESCRIPTOR | RH_CLASS: 
	    stat = dwc_otg_hcd_hub_control(hubnum, typeReq, wValue,wIndex, data_buf, wLength); 
            OK (sizeof(struct usb_hub_descriptor));
	case RH_GET_CONFIGURATION:	*(__u8 *) data_buf = 0x01; OK (1);
	case RH_SET_CONFIGURATION:      OK (0);

	default:
		printf("unsupported root hub command");
		stat = USB_ST_STALLED;
	}

	wait_ms(1);

	len = min_t(int, len, leni);
	if (data != data_buf)
	    memcpy (data, data_buf, len);
	dev->act_len = len;
	dev->status = stat;

	wait_ms(1);

	return stat;
}


int dwc_submit_common_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
		int transfer_len, struct devrequest *setup)
{
	int stat = 0, result = 0;
	int maxsize = usb_maxpacket(dev, pipe);
	int timeout;

 	udelay(10);
	if (!maxsize) {
		err("submit_common_message: pipesize for pipe %lx is zero",
			pipe);
		return -1;
	}

	if (dwc_otg_submit_job(dev, pipe, buffer, transfer_len, setup) < 0) {
		err("dwc_otg_submit_job failed");
		return -1;
	}

 	udelay(100);

	/*
	 * allow more time for a BULK device to react - some are slow. 
	 * allowing max time out of 2 sec for any transfer. 
	 * 
	 * Transfer complete will be polled at interval of 50 micro 
	 * secs.
	 */ 
	timeout = MAX_USB_TRANSFER_TIMEOUT / USB_TRANSFER_POLL_INTERVAL;
	urb_finished = 0;
	/* wait for it to complete */
	for (;;) {
		/* check whether the controller is done */
                result = dwc_otg_hcd_handle_intr();
		if (urb_finished) {
		    break;
		}

		if (--timeout) {
		    	udelay(USB_TRANSFER_POLL_INTERVAL); 
		} else {
			err("CTL:TIMEOUT ");
			stat = USB_ST_CRC_ERR;
			break;
		}
	}

	dev->status = stat;
	dev->act_len = transfer_len;

	udelay(10);

	return 0;
}

/* submit routines called from usb.c */
int dwc_submit_bulk_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
		int transfer_len)
{
	info("submit_bulk_msg");
	return dwc_submit_common_msg(dev, pipe, buffer, transfer_len, NULL);
}

int dwc_submit_control_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
		int transfer_len, struct devrequest *setup)
{
	int maxsize = usb_maxpacket(dev, pipe);
	int hub_num = -1;
	int is_rh_msg = 0;

	info("submit_control_msg");
	wait_ms(1);
	if (!maxsize) {
		err("submit_control_message: pipesize for pipe %lx is zero",
			pipe);
		return -1;
	}

	if (usb_multi_root_hub) {
	    if (dev->is_root_hub) {
		is_rh_msg = 1;

		/* devnum 1 always is for root hub 0. 
		 * If pipe devnum is 0, it means address has not been set for
		 * that root hub. If in this case, root hub 0 ha already got the
		 * address set, it is for root hub 1 else the msg is for 
		 * root hub 0 
		 */
		if ((((pipe >> 8) & 0x7f) == 1) || 
		    (dwc_otg_hcd_struct[0].rh.devnum == DWC_OTG_DEVNUM_INVALID)) {
		    hub_num = 0;
		} else {
		    hub_num = 1;
		}
	    }
	} else {
	        if (((pipe >> 8) & 0x7f) == dwc_otg_hcd_struct[0].rh.devnum) {
			is_rh_msg = 1; 
		}
	}

	if (is_rh_msg) {
	        dwc_otg_hcd_handle_intr();

		if (usb_multi_root_hub) {
		    if (hub_num != -1) 
			dwc_otg_hcd_struct[hub_num].rh.dev = dev;
		} else {
			dwc_otg_hcd_struct[0].rh.dev = dev;
		}
                /* root hub - redirect */
                return dwc_otg_submit_rh_msg(dev, pipe, buffer, transfer_len,
                        setup);
        }


	return dwc_submit_common_msg(dev, pipe, buffer, transfer_len, setup);
}

int dwc_submit_int_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
		int transfer_len, int interval)
{
	info("submit_int_msg");
	return -1;
}



/**
 * Initializes the HCD. This function allocates memory for and initializes the
 * static parts of the usb_hcd and dwc_otg_hcd structures. It also registers the
 * USB bus with the core and calls the hc_driver->start() function. It returns
 * a negative error on failure.
 */
int dwc_otg_hcd_init(dwc_otg_core_if_t *_core_if)
{
	int 		num_channels;
	int 		i;
	dwc_hc_t	*channel;
	dwc_otg_hcd_t   *dwc_otg_hcd;
        struct list_head  *item;

	int retval = 0;

	dbg("%s: unit %d\n", __FUNCTION__, _core_if->unit);

	dwc_otg_hcd = &dwc_otg_hcd_struct[_core_if->unit];

	if (usb_multi_root_hub) {
		/* Initialise rh.devnum to an invalid value */
		dwc_otg_hcd->rh.devnum = DWC_OTG_DEVNUM_INVALID;
	}

	/*
	 * Allocate memory for the base HCD plus the DWC OTG HCD.
	 * Initialize the base HCD.
	 */
	dwc_otg_hcd->core_if = _core_if;

	/* Initialize the non-periodic schedule. */
	INIT_LIST_HEAD(&dwc_otg_hcd->non_periodic_sched_inactive);
	INIT_LIST_HEAD(&dwc_otg_hcd->non_periodic_sched_active);

	/*
	 * Create a host channel descriptor for each host channel implemented
	 * in the controller. Initialize the channel descriptor array.
	 */
	INIT_LIST_HEAD(&dwc_otg_hcd->free_hc_list);
	num_channels = dwc_otg_hcd->core_if->core_params->host_channels;
	for (i = 0; i < num_channels; i++) {
		channel = malloc(sizeof(dwc_hc_t));
		if (channel == NULL) {
			retval = -DWC_OTG_ENOMEM;
			printf("%s: host channel allocation failed\n", __func__);
			goto error2;
		}
		memset(channel, 0, sizeof(dwc_hc_t));
		channel->hc_num = i;
		dwc_otg_hcd->hc_ptr_array[i] = channel;
		debug("HCD Added channel #%d, hc=%p\n", i, channel);
	}

	dwc_otg_hcd->status_buf = malloc(DWC_OTG_HCD_STATUS_BUF_SIZE);
        if (dwc_otg_hcd->status_buf == NULL) {
                retval = -DWC_OTG_ENOMEM;
                printf("%s: status_buf allocation failed\n", __func__);
                goto error3;
        }

	/* Taken from hcd_reinit called from dwc_otg_hcd_start */
        dwc_otg_hcd->flags.d32 = 0;

        dwc_otg_hcd->non_periodic_qh_ptr = &dwc_otg_hcd->non_periodic_sched_active;
        dwc_otg_hcd->non_periodic_channels = 0;

	/* 
	 * Put all channels in the free channel list and clean up channel
	 * states.
	 */
	item = dwc_otg_hcd->free_hc_list.next;
	while (item != &dwc_otg_hcd->free_hc_list) {
		list_del(item);
		item = dwc_otg_hcd->free_hc_list.next;
	}
	num_channels = dwc_otg_hcd->core_if->core_params->host_channels;
	for (i = 0; i < num_channels; i++) {
		channel = dwc_otg_hcd->hc_ptr_array[i];
		list_add_tail(&channel->hc_list_entry, &dwc_otg_hcd->free_hc_list);
		dwc_otg_hc_cleanup(dwc_otg_hcd->core_if, channel);
	}
	dwc_otg_core_host_init(_core_if);


	dbg("DWC OTG HCD Initialized HCD\n");

	return 0;

	/* Error conditions */
 error3:
	/* JSRXNLE_FIXME */
	//	usb_remove_hcd(hcd);
 error2:
	/* JSRXNLE_FIXME */
	//	dwc_otg_hcd_free(dwc_otg_hcd);
	//	usb_put_hcd(hcd);
	// error1:
	return retval;
}




/**
 * This function is called during module intialization to verify that
 * the module parameters are in a valid state.
 */
static int check_parameters(int unit, dwc_otg_core_if_t *core_if)
{
	int i;
	int retval = 0;

/* Checks if the parameter is outside of its valid range of values */
#define DWC_OTG_PARAM_TEST(_param_,_low_,_high_) \
	((dwc_otg_module_params[unit]._param_ < (_low_)) || \
         (dwc_otg_module_params[unit]._param_ > (_high_)))

/* If the parameter has been set by the user, check that the parameter value is
 * within the value range of values.  If not, report a module error. */
#define DWC_OTG_PARAM_ERR(_param_,_low_,_high_,_string_) \
        do { \
		if (dwc_otg_module_params[unit]._param_ != -1) { \
                        dbg("      %s = %d\n", _string_, dwc_otg_module_params[unit]._param_); \
			if (DWC_OTG_PARAM_TEST(_param_,(_low_),(_high_))) { \
				dbg("`%d' invalid for parameter `%s'\n", \
				       dwc_otg_module_params[unit]._param_, _string_); \
				dwc_otg_module_params[unit]._param_ = dwc_param_##_param_##_default; \
				retval ++; \
			} \
		} \
	} while (0)

	DWC_OTG_PARAM_ERR(opt,0,1,"opt");
	DWC_OTG_PARAM_ERR(otg_cap,0,2,"otg_cap");
        DWC_OTG_PARAM_ERR(dma_enable,0,1,"dma_enable");
	DWC_OTG_PARAM_ERR(speed,0,1,"speed");
	DWC_OTG_PARAM_ERR(host_support_fs_ls_low_power,0,1,"host_support_fs_ls_low_power");
	DWC_OTG_PARAM_ERR(host_ls_low_power_phy_clk,0,1,"host_ls_low_power_phy_clk");
	DWC_OTG_PARAM_ERR(enable_dynamic_fifo,0,1,"enable_dynamic_fifo");
	DWC_OTG_PARAM_ERR(data_fifo_size,32,32768,"data_fifo_size");
	DWC_OTG_PARAM_ERR(dev_rx_fifo_size,16,32768,"dev_rx_fifo_size");
	DWC_OTG_PARAM_ERR(dev_nperio_tx_fifo_size,16,32768,"dev_nperio_tx_fifo_size");
	DWC_OTG_PARAM_ERR(host_rx_fifo_size,16,32768,"host_rx_fifo_size");
	DWC_OTG_PARAM_ERR(host_nperio_tx_fifo_size,16,32768,"host_nperio_tx_fifo_size");
	DWC_OTG_PARAM_ERR(host_perio_tx_fifo_size,16,32768,"host_perio_tx_fifo_size");
	DWC_OTG_PARAM_ERR(max_transfer_size,2047,524288,"max_transfer_size");
	DWC_OTG_PARAM_ERR(max_packet_count,15,511,"max_packet_count");
	DWC_OTG_PARAM_ERR(host_channels,1,16,"host_channels");
	DWC_OTG_PARAM_ERR(dev_endpoints,1,15,"dev_endpoints");
	DWC_OTG_PARAM_ERR(phy_type,0,2,"phy_type");
        DWC_OTG_PARAM_ERR(phy_ulpi_ddr,0,1,"phy_ulpi_ddr");
        DWC_OTG_PARAM_ERR(phy_ulpi_ext_vbus,0,1,"phy_ulpi_ext_vbus");
	DWC_OTG_PARAM_ERR(i2c_enable,0,1,"i2c_enable");
	DWC_OTG_PARAM_ERR(ulpi_fs_ls,0,1,"ulpi_fs_ls");
	DWC_OTG_PARAM_ERR(ts_dline,0,1,"ts_dline");

	dbg("      dma_burst_size = %d\n", dwc_otg_module_params[unit].dma_burst_size);
	if (dwc_otg_module_params[unit].dma_burst_size != -1) {
		if (DWC_OTG_PARAM_TEST(dma_burst_size,1,1) &&
		    DWC_OTG_PARAM_TEST(dma_burst_size,4,4) &&
		    DWC_OTG_PARAM_TEST(dma_burst_size,8,8) &&
		    DWC_OTG_PARAM_TEST(dma_burst_size,16,16) &&
		    DWC_OTG_PARAM_TEST(dma_burst_size,32,32) &&
		    DWC_OTG_PARAM_TEST(dma_burst_size,64,64) &&
		    DWC_OTG_PARAM_TEST(dma_burst_size,128,128) &&
		    DWC_OTG_PARAM_TEST(dma_burst_size,256,256))
		{
			printf("`%d' invalid for parameter `dma_burst_size'\n",
				  dwc_otg_module_params[unit].dma_burst_size);
			dwc_otg_module_params[unit].dma_burst_size = 32;
			retval ++;
		}
	}

	dbg("      phy_utmi_width = %d\n", dwc_otg_module_params[unit].phy_utmi_width);
	if (dwc_otg_module_params[unit].phy_utmi_width != -1) {
		if (DWC_OTG_PARAM_TEST(phy_utmi_width,8,8) &&
		    DWC_OTG_PARAM_TEST(phy_utmi_width,16,16))
		{
			printf("`%d' invalid for parameter `phy_utmi_width'\n",
				  dwc_otg_module_params[unit].phy_utmi_width);
			dwc_otg_module_params[unit].phy_utmi_width = 16;
			retval ++;
		}
	}

	for (i=0; i<15; i++) {
		/** @todo should be like above */
		if (dwc_otg_module_params[unit].dev_perio_tx_fifo_size[i] != (unsigned)-1) {
		        printf("      dev_perio_tx_fifo_size[%d] = %d\n", i, dwc_otg_module_params[unit].dev_perio_tx_fifo_size[i]);
			if (DWC_OTG_PARAM_TEST(dev_perio_tx_fifo_size[i],4,768)) {
				printf("`%d' invalid for parameter `%s_%d'\n",
					  dwc_otg_module_params[unit].dev_perio_tx_fifo_size[i], "dev_perio_tx_fifo_size", i);
				dwc_otg_module_params[unit].dev_perio_tx_fifo_size[i] = dwc_param_dev_perio_tx_fifo_size_default;
				retval ++;
			}
		}
	}

	dbg("\n");

	/* At this point, all module parameters that have been set by the user
	 * are valid, and those that have not are left unset.  Now set their
	 * default values and/or check the parameters against the hardware
	 * configurations of the OTG core. */



/* This sets the parameter to the default value if it has not been set by the
 * user */
#define DWC_OTG_PARAM_SET_DEFAULT(_param_, _string_) \
	({ \
		int changed = 1; \
		if (dwc_otg_module_params[unit]._param_ == -1) { \
			changed = 0; \
			dwc_otg_module_params[unit]._param_ = dwc_param_##_param_##_default; \
		} \
                dbg("       def %s = %d\n", _string_, dwc_otg_module_params[unit]._param_); \
		changed; \
	})

/* This checks the macro agains the hardware configuration to see if it is
 * valid.  It is possible that the default value could be invalid.  In this
 * case, it will report a module error if the user touched the parameter.
 * Otherwise it will adjust the value without any error. */
#define DWC_OTG_PARAM_CHECK_VALID(_param_,_str_,_is_valid_,_set_valid_) \
	({ \
	        int changed = DWC_OTG_PARAM_SET_DEFAULT(_param_, _str_); \
		int error = 0; \
		if (!(_is_valid_)) { \
			if (changed) { \
				printf("`%d' invalid for parameter `%s'.  Check HW configuration.\n", dwc_otg_module_params[unit]._param_,_str_); \
				error = 1; \
			} \
			dwc_otg_module_params[unit]._param_ = (_set_valid_); \
		} \
                dbg("     valid %s = %d\n", _str_, dwc_otg_module_params[unit]._param_); \
		error; \
	})

	/* OTG Cap */
	retval += DWC_OTG_PARAM_CHECK_VALID(otg_cap,"otg_cap",
                  ({
			  int valid;
			  valid = 1;
			  switch (dwc_otg_module_params[unit].otg_cap) {
			  case DWC_OTG_CAP_PARAM_HNP_SRP_CAPABLE:
				  if (core_if->hwcfg2.b.op_mode != DWC_HWCFG2_OP_MODE_HNP_SRP_CAPABLE_OTG) valid = 0;
				  break;
			  case DWC_OTG_CAP_PARAM_SRP_ONLY_CAPABLE:
				  if ((core_if->hwcfg2.b.op_mode != DWC_HWCFG2_OP_MODE_HNP_SRP_CAPABLE_OTG) &&
				      (core_if->hwcfg2.b.op_mode != DWC_HWCFG2_OP_MODE_SRP_ONLY_CAPABLE_OTG) &&
				      (core_if->hwcfg2.b.op_mode != DWC_HWCFG2_OP_MODE_SRP_CAPABLE_DEVICE) &&
				      (core_if->hwcfg2.b.op_mode != DWC_HWCFG2_OP_MODE_SRP_CAPABLE_HOST))
				  {
					  valid = 0;
				  }
				  break;
			  case DWC_OTG_CAP_PARAM_NO_HNP_SRP_CAPABLE:
				  /* always valid */
				  break;
			  }
			  valid;
		  }),
                  (((core_if->hwcfg2.b.op_mode == DWC_HWCFG2_OP_MODE_HNP_SRP_CAPABLE_OTG) ||
		    (core_if->hwcfg2.b.op_mode == DWC_HWCFG2_OP_MODE_SRP_ONLY_CAPABLE_OTG) ||
		    (core_if->hwcfg2.b.op_mode == DWC_HWCFG2_OP_MODE_SRP_CAPABLE_DEVICE) ||
		    (core_if->hwcfg2.b.op_mode == DWC_HWCFG2_OP_MODE_SRP_CAPABLE_HOST)) ?
		   DWC_OTG_CAP_PARAM_SRP_ONLY_CAPABLE :
		   DWC_OTG_CAP_PARAM_NO_HNP_SRP_CAPABLE));
	
	retval += DWC_OTG_PARAM_CHECK_VALID(dma_enable,"dma_enable",
					    ((dwc_otg_module_params[unit].dma_enable == 1) && (core_if->hwcfg2.b.architecture == 0)) ? 0 : 1,
					    0);

	retval += DWC_OTG_PARAM_CHECK_VALID(opt,"opt",
					    1,
					    0);

	DWC_OTG_PARAM_SET_DEFAULT(dma_burst_size, "dma_burst_size");

	retval += DWC_OTG_PARAM_CHECK_VALID(host_support_fs_ls_low_power,
					    "host_support_fs_ls_low_power",
					    1, 0);

	retval += DWC_OTG_PARAM_CHECK_VALID(enable_dynamic_fifo,
				  "enable_dynamic_fifo",
				  ((dwc_otg_module_params[unit].enable_dynamic_fifo == 0) ||
				   (core_if->hwcfg2.b.dynamic_fifo == 1)), 0);


	retval += DWC_OTG_PARAM_CHECK_VALID(data_fifo_size,
				  "data_fifo_size",
				  (dwc_otg_module_params[unit].data_fifo_size <= core_if->hwcfg3.b.dfifo_depth),
				  core_if->hwcfg3.b.dfifo_depth);

	retval += DWC_OTG_PARAM_CHECK_VALID(dev_rx_fifo_size,
				  "dev_rx_fifo_size",
				  (dwc_otg_module_params[unit].dev_rx_fifo_size <= dwc_read_reg32(&core_if->core_global_regs->grxfsiz)),
				  dwc_read_reg32(&core_if->core_global_regs->grxfsiz));

	retval += DWC_OTG_PARAM_CHECK_VALID(dev_nperio_tx_fifo_size,
				  "dev_nperio_tx_fifo_size",
				  (dwc_otg_module_params[unit].dev_nperio_tx_fifo_size <= (dwc_read_reg32(&core_if->core_global_regs->gnptxfsiz) >> 16)),
				  (dwc_read_reg32(&core_if->core_global_regs->gnptxfsiz) >> 16));

	retval += DWC_OTG_PARAM_CHECK_VALID(host_rx_fifo_size,
					    "host_rx_fifo_size",
					    (dwc_otg_module_params[unit].host_rx_fifo_size <= dwc_read_reg32(&core_if->core_global_regs->grxfsiz)),
					    dwc_read_reg32(&core_if->core_global_regs->grxfsiz));


	retval += DWC_OTG_PARAM_CHECK_VALID(host_nperio_tx_fifo_size,
				  "host_nperio_tx_fifo_size",
				  (dwc_otg_module_params[unit].host_nperio_tx_fifo_size <= (dwc_read_reg32(&core_if->core_global_regs->gnptxfsiz) >> 16)),
				  (dwc_read_reg32(&core_if->core_global_regs->gnptxfsiz) >> 16));

	retval += DWC_OTG_PARAM_CHECK_VALID(host_perio_tx_fifo_size,
					    "host_perio_tx_fifo_size",
					    (dwc_otg_module_params[unit].host_perio_tx_fifo_size <= ((dwc_read_reg32(&core_if->core_global_regs->hptxfsiz) >> 16))),
					    ((dwc_read_reg32(&core_if->core_global_regs->hptxfsiz) >> 16)));

	retval += DWC_OTG_PARAM_CHECK_VALID(max_transfer_size,
				  "max_transfer_size",
				  (dwc_otg_module_params[unit].max_transfer_size < (1 << (core_if->hwcfg3.b.xfer_size_cntr_width + 11))),
				  ((1 << (core_if->hwcfg3.b.xfer_size_cntr_width + 11)) - 1));

	retval += DWC_OTG_PARAM_CHECK_VALID(max_packet_count,
				  "max_packet_count",
				  (dwc_otg_module_params[unit].max_packet_count < (1 << (core_if->hwcfg3.b.packet_size_cntr_width + 4))),
				  ((1 << (core_if->hwcfg3.b.packet_size_cntr_width + 4)) - 1));

	retval += DWC_OTG_PARAM_CHECK_VALID(host_channels,
				  "host_channels",
				  (dwc_otg_module_params[unit].host_channels <= (core_if->hwcfg2.b.num_host_chan + 1)),
				  (core_if->hwcfg2.b.num_host_chan + 1));

	retval += DWC_OTG_PARAM_CHECK_VALID(dev_endpoints,
				  "dev_endpoints",
				  (dwc_otg_module_params[unit].dev_endpoints <= (core_if->hwcfg2.b.num_dev_ep)),
				  core_if->hwcfg2.b.num_dev_ep);

/*
 * Define the following to disable the FS PHY Hardware checking.  This is for
 * internal testing only.
 *
 * #define NO_FS_PHY_HW_CHECKS
 */

#ifdef NO_FS_PHY_HW_CHECKS
	retval += DWC_OTG_PARAM_CHECK_VALID(phy_type,
					    "phy_type", 1, 0);
#else
	retval += DWC_OTG_PARAM_CHECK_VALID(phy_type,
					    "phy_type",
					    ({
						    int valid = 0;
						    if ((dwc_otg_module_params[unit].phy_type == DWC_PHY_TYPE_PARAM_UTMI) &&
							((core_if->hwcfg2.b.hs_phy_type == 1) ||
							 (core_if->hwcfg2.b.hs_phy_type == 3)))
						    {
							    valid = 1;
						    }
						    else if ((dwc_otg_module_params[unit].phy_type == DWC_PHY_TYPE_PARAM_ULPI) &&
							     ((core_if->hwcfg2.b.hs_phy_type == 2) ||
							      (core_if->hwcfg2.b.hs_phy_type == 3)))
						    {
							    valid = 1;
						    }
						    else if ((dwc_otg_module_params[unit].phy_type == DWC_PHY_TYPE_PARAM_FS) &&
							     (core_if->hwcfg2.b.fs_phy_type == 1))
						    {
							    valid = 1;
						    }
						    valid;
					    }),
					    ({
						    int set = DWC_PHY_TYPE_PARAM_FS;
						    if (core_if->hwcfg2.b.hs_phy_type) {
							    if ((core_if->hwcfg2.b.hs_phy_type == 3) ||
								(core_if->hwcfg2.b.hs_phy_type == 1)) {
								    set = DWC_PHY_TYPE_PARAM_UTMI;
							    }
							    else {
								    set = DWC_PHY_TYPE_PARAM_ULPI;
							    }
						    }
						    set;
					    }));
#endif

	retval += DWC_OTG_PARAM_CHECK_VALID(speed,"speed",
					    (dwc_otg_module_params[unit].speed == 0) && (dwc_otg_module_params[unit].phy_type == DWC_PHY_TYPE_PARAM_FS) ? 0 : 1,
					    dwc_otg_module_params[unit].phy_type == DWC_PHY_TYPE_PARAM_FS ? 1 : 0);

	retval += DWC_OTG_PARAM_CHECK_VALID(host_ls_low_power_phy_clk,
					    "host_ls_low_power_phy_clk",
					    ((dwc_otg_module_params[unit].host_ls_low_power_phy_clk == DWC_HOST_LS_LOW_POWER_PHY_CLK_PARAM_48MHZ) && (dwc_otg_module_params[unit].phy_type == DWC_PHY_TYPE_PARAM_FS) ? 0 : 1),
					    ((dwc_otg_module_params[unit].phy_type == DWC_PHY_TYPE_PARAM_FS) ? DWC_HOST_LS_LOW_POWER_PHY_CLK_PARAM_6MHZ : DWC_HOST_LS_LOW_POWER_PHY_CLK_PARAM_48MHZ));

        DWC_OTG_PARAM_SET_DEFAULT(phy_ulpi_ddr, "phy_ulpi_ddr");
        DWC_OTG_PARAM_SET_DEFAULT(phy_ulpi_ext_vbus, "phy_ulpi_ext_vbus");
        DWC_OTG_PARAM_SET_DEFAULT(phy_utmi_width, "phy_utmi_width");
        DWC_OTG_PARAM_SET_DEFAULT(ulpi_fs_ls, "ulpi_fs_ls");
        DWC_OTG_PARAM_SET_DEFAULT(ts_dline, "ts_dline");

#ifdef NO_FS_PHY_HW_CHECKS
	retval += DWC_OTG_PARAM_CHECK_VALID(i2c_enable,
					    "i2c_enable", 1, 0);
#else
	retval += DWC_OTG_PARAM_CHECK_VALID(i2c_enable,
					    "i2c_enable",
					    (dwc_otg_module_params[unit].i2c_enable == 1) && (core_if->hwcfg3.b.i2c == 0) ? 0 : 1,
					    0);
#endif

	for (i=0; i<16; i++) {

		int changed = 1;
		int error = 0;

		if (dwc_otg_module_params[unit].dev_perio_tx_fifo_size[i] == -1) {
			changed = 0;
			dwc_otg_module_params[unit].dev_perio_tx_fifo_size[i] = dwc_param_dev_perio_tx_fifo_size_default;
		}
		if (!(dwc_otg_module_params[unit].dev_perio_tx_fifo_size[i] <= (dwc_read_reg32(&core_if->core_global_regs->dptxfsiz[i])))) {
			if (changed) {
				printf("`%d' invalid for parameter `dev_perio_fifo_size_%d'.  Check HW configuration.\n", dwc_otg_module_params[unit].dev_perio_tx_fifo_size[i],i);
				error = 1;
			}
			dwc_otg_module_params[unit].dev_perio_tx_fifo_size[i] = dwc_read_reg32(&core_if->core_global_regs->dptxfsiz[i]);
		}
		retval += error;
	}

	return retval;
}


/**
 * This function is called when an device is bound to a
 * dwc_otg_driver. It creates the driver components required to
 * control the device (CIL, HCD, and PCD) and it initializes the
 * device. The driver components are stored in a dwc_otg_device
 * structure. A reference to the dwc_otg_device is saved in the
 * device. This allows the driver to access the dwc_otg_device
 * structure on subsequent calls to driver methods for this device.
 *
 * @param[in] dev  device definition
 */
static int dwc_otg_driver_probe(int unit)
{
	int retval = 0;
	dwc_otg_device_t *dwc_otg_device;
	int32_t	snpsid;

	dbg("%s unit %d\n", __FUNCTION__, unit);

	dwc_otg_device = &dwc_otg_device_struct[unit];
	dwc_otg_device->reg_offset = 0xFFFFFFFF;  

	/*
	 * Map the DWC_otg Core memory into virtual address space.
	 */
	if (unit == 0)
		dwc_otg_device->base = (void *) OCTEON_USB_UNIT0_VIRT_BASE;
	else
		dwc_otg_device->base = (void *) OCTEON_USB_UNIT1_VIRT_BASE;

	/*
	 * Attempt to ensure this device is really a DWC_otg Controller.
	 * Read and verify the SNPSID register contents. The value should be
	 * 0x45F42XXX, which corresponds to "OT2", as in "OTG version 2.XX".
	 */
	snpsid = dwc_read_reg32((uint32_t *)((uint8_t *)dwc_otg_device->base + 0x40));
	dbg("### unit %d base %p snpsid = 0x%x\n", unit, dwc_otg_device->base, snpsid);
	if ((snpsid & 0xFFFFF000) != 0x4F542000) {
		printf("!!! Bad value for SNPSID: 0x%08x !!!\n", snpsid);
		retval = -DWC_OTG_EINVAL;
		goto fail;
	}

	/*
	 * Initialize driver data to point to the global DWC_otg
	 * Device structure.
	 */
	dwc_otg_device->core_if = dwc_otg_cil_init(unit, dwc_otg_device->base);
	if (dwc_otg_device->core_if == 0) {
		printf("CIL initialization failed!\n");
		retval = -DWC_OTG_ENOMEM;
		goto fail;
	}
	
	/*
	 * Validate parameter values.
	 */
	if (check_parameters(unit,dwc_otg_device->core_if) != 0) {
		retval = -DWC_OTG_EINVAL;
		goto fail;
	}

       dwc_otg_disable_global_interrupts( dwc_otg_device->core_if );

	/*
	 * Initialize the DWC_otg core.
	 */
	dwc_otg_core_init( dwc_otg_device->core_if );

	/*
	 * Initialize the HCD
	 */
	retval = dwc_otg_hcd_init(dwc_otg_device->core_if);
	if (retval != 0) {
		printf("dwc_otg_hcd_init failed\n");
		dwc_otg_device->hcd = NULL;
		goto fail;
	}

	/*
	 * Enable the global interrupt after all the interrupt
	 * handlers are installed.
	 */
	dwc_otg_enable_global_interrupts( dwc_otg_device->core_if );

	return 0;

fail:
	/* JSRXNLE_FIXME: Fill this later */
	//	dwc_otg_driver_remove(dev);
	return retval;
}

/*
 * low level initalisation routine, called from usb.c
 */
static char hcd_inited = 0;

static inline uint32_t dwc_get_freq(void) {
        DECLARE_GLOBAL_DATA_PTR;

        return (gd->cpu_clock_mhz * 1000 * 1000);
}

static int dwc_otg_driver_init(int unit)
{

        union
        {
            uint64_t u64;
            struct
            {
                uint64_t reserved_20_63          : 44;
                uint64_t divide2                 : 2;
                uint64_t hclk_rst                : 1;
                uint64_t reserved_16_16          : 1;
                uint64_t p_rtype                 : 2;
                uint64_t p_com_on                : 1;
                uint64_t p_c_sel                 : 2;
                uint64_t cdiv_byp                : 1;
                uint64_t sd_mode                 : 2;
                uint64_t s_bist                  : 1;
                uint64_t por                     : 1;
                uint64_t enable                  : 1;
                uint64_t prst                    : 1;
                uint64_t hrst                    : 1;
                uint64_t divide                  : 3;
            } s;
        } usbn_clk_ctl;
        uint64_t OCTEON_USBN_CLK_CTL;
        int divisor;
        cvmx_usbnx_usbp_ctl_status_t usbn_usbp_ctl_status; 

	if (unit == 0) {
		OCTEON_USBN_CLK_CTL = 0x8001180068000010ull;
	} else {
		OCTEON_USBN_CLK_CTL = 0x8001180078000010ull;
	}

        /*
         * Though core was configured for external dma override that
         * with slave mode only for CN31XX.
         */
        dwc_otg_module_params[unit].dma_enable = 0;

	/* Divide the core clock down such that USB is as close as possible to
	   125Mhz */
	divisor = (dwc_get_freq()+125000000-1) / 125000000;
	if (divisor < 4)  /* Lower than 4 doesn't seem to work properly */
	    divisor = 4;

        // Fetch the value of the Register, and de-assert POR
        usbn_clk_ctl.u64 = cvmx_read_csr(OCTEON_USBN_CLK_CTL);


#ifdef CONFIG_MAG8600
        
        usbn_clk_ctl.s.p_c_sel = 0;
        usbn_clk_ctl.s.por = 0;
        usbn_clk_ctl.s.p_rtype = 0; 

        usbn_clk_ctl.s.divide = divisor;
        usbn_clk_ctl.s.divide2 = 0;
        usbn_clk_ctl.s.por = 1;
        usbn_clk_ctl.s.hrst = 0;
        usbn_clk_ctl.s.prst = 0;
        usbn_clk_ctl.s.hclk_rst = 0;
        usbn_clk_ctl.s.enable = 0;
        cvmx_write_csr(OCTEON_USBN_CLK_CTL, usbn_clk_ctl.u64);

        // Wait for POR
        udelay(850);
        usbn_clk_ctl.s.hclk_rst = 1;
        cvmx_write_csr(OCTEON_USBN_CLK_CTL, usbn_clk_ctl.u64);
        udelay(1);

        usbn_clk_ctl.s.por = 0;
        cvmx_write_csr(OCTEON_USBN_CLK_CTL, usbn_clk_ctl.u64);
        udelay(10);
     
        usbn_usbp_ctl_status.u64 = cvmx_read_csr(OCTEON_USBN_CLK_CTL+8);
        usbn_usbp_ctl_status.s.ate_reset = 1;
        cvmx_write_csr(OCTEON_USBN_CLK_CTL + 8,usbn_usbp_ctl_status.u64);

        udelay(1);
        usbn_usbp_ctl_status.s.ate_reset = 0;
        cvmx_write_csr(OCTEON_USBN_CLK_CTL + 8,usbn_usbp_ctl_status.u64);

        usbn_clk_ctl.s.prst = 1;
        cvmx_write_csr(OCTEON_USBN_CLK_CTL, usbn_clk_ctl.u64);

        udelay(1);
        usbn_usbp_ctl_status.s.hst_mode = 0;
        cvmx_write_csr(OCTEON_USBN_CLK_CTL + 8,usbn_usbp_ctl_status.u64);
         
        udelay(1);
        usbn_clk_ctl.s.hrst = 1;
        cvmx_write_csr(OCTEON_USBN_CLK_CTL, usbn_clk_ctl.u64);
        usbn_clk_ctl.s.enable = 1;
        cvmx_write_csr(OCTEON_USBN_CLK_CTL, usbn_clk_ctl.u64);
        udelay(1);
#else
        usbn_clk_ctl.s.p_c_sel = 0;
        usbn_clk_ctl.s.por = 0;
        usbn_clk_ctl.s.p_rtype = 0; 

        usbn_clk_ctl.s.divide = divisor;
        usbn_clk_ctl.s.divide2 = 0;

        cvmx_write_csr(OCTEON_USBN_CLK_CTL, usbn_clk_ctl.u64);

        // Wait for POR
        udelay(850);

        usbn_clk_ctl.u64 = cvmx_read_csr(OCTEON_USBN_CLK_CTL);
        usbn_clk_ctl.s.por = 0;
        usbn_clk_ctl.s.p_rtype = 0;
        usbn_clk_ctl.s.prst = 1;
        cvmx_write_csr(OCTEON_USBN_CLK_CTL, usbn_clk_ctl.u64);

        udelay(1);

        usbn_clk_ctl.s.hrst = 1;
        cvmx_write_csr(OCTEON_USBN_CLK_CTL, usbn_clk_ctl.u64);

        udelay(1);

#endif
	dwc_otg_driver_probe(unit);
}

#define octeon_is_cpuid(x)   (__OCTEON_IS_MODEL_COMPILE__(x, cpu_id))

int dwc_usb_lowlevel_init(void)
{
	int i;
	uint32_t cpu_id = cvmx_get_proc_id();

	if (octeon_is_cpuid(OCTEON_CN52XX)) {
	    usb_multi_root_hub = 1;
	    usb_num_root_hub = 2;
	} else {
	    usb_num_root_hub = 1;
	}

	for (i=0; i<usb_num_root_hub; i++) {
	       dwc_otg_driver_init(i);
	}

	hcd_inited = 1;
	return 0;
}

int dwc_usb_lowlevel_stop(uint32_t port_n)
{
	/* this gets called really early - before the controller has */
	/* even been initialized! */
	if (!hcd_inited)
		return 0;

	/* JSRXNLE_FIXME: Fill this part */
	//	hc_reset (&gohci);

	return 0;
}

#endif /* CONFIG_USB_DWC_OTG */
