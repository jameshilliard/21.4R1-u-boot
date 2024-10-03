/* ==========================================================================
 * $File: //dwh/usb_iip/dev/software/otg_ipmate/linux/drivers/dwc_otg_hcd_queue.c $
 * $Revision: 1.2 $
 * $Date: 2006/07/08 01:55:15 $
 * $Change: 537387 $
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
/**
 * @file
 *
 * This file contains the functions to manage Queue Heads and Queue
 * Transfer Descriptors.
 */

#include <common.h>

#if defined (CONFIG_USB_DWC_OTG)

#include <malloc.h>
#include <usb.h>


#include "dwc_otg_driver.h"
#include "dwc_otg_hcd.h"
#include "dwc_otg_regs.h"
#include <linux/list.h>

static void dwc_otg_hcd_qh_init(dwc_otg_hcd_t *_hcd, dwc_otg_qh_t *_qh, dwc_otg_urb_t *_urb);
static int dwc_otg_hcd_qh_add (dwc_otg_hcd_t *_hcd, dwc_otg_qh_t *_qh);


/**
 * This function allocates and initializes a QH.
 *
 * @param _hcd The HCD state structure for the DWC OTG controller.
 * @param[in] _urb Holds the information about the device/endpoint that we need
 * to initialize the QH.
 *
 * @return Returns pointer to the newly allocated QH, or NULL on error. */
dwc_otg_qh_t *dwc_otg_hcd_qh_create (dwc_otg_hcd_t *_hcd, dwc_otg_urb_t *_urb)
{
	dwc_otg_qh_t *qh;

	/* Allocate memory */
	/** @todo add memflags argument */
	qh = dwc_otg_hcd_qh_alloc ();
	if (qh == NULL) {
		return NULL;
	}

	dwc_otg_hcd_qh_init (_hcd, qh, _urb);
	return qh;
}

/** Free each QTD in the QH's QTD-list then free the QH.  QH should already be
 * removed from a list.  QTD list should already be empty if called from URB
 * Dequeue.
 *
 * @param[in] _qh The QH to free.
 */
void dwc_otg_hcd_qh_free (dwc_otg_qh_t *_qh)
{
	dwc_otg_qtd_t *qtd;
	struct list_head *pos;

	/* Free each QTD in the QTD list */
	for (pos = _qh->qtd_list.next;
	     pos != &_qh->qtd_list;
	     pos = _qh->qtd_list.next)
	{
		list_del (pos);
		qtd = dwc_list_to_qtd (pos);
		dwc_otg_hcd_qtd_free (qtd);
	}

	free (_qh);
	return;
}

/** Initializes a QH structure.
 *
 * @param[in] _hcd The HCD state structure for the DWC OTG controller.
 * @param[in] _qh The QH to init.
 * @param[in] _urb Holds the information about the device/endpoint that we need
 * to initialize the QH. */
static void dwc_otg_hcd_qh_init(dwc_otg_hcd_t *_hcd, dwc_otg_qh_t *_qh, dwc_otg_urb_t *_urb)
{
	memset (_qh, 0, sizeof (dwc_otg_qh_t));

	/* Initialize QH */
	switch (usb_pipetype(_urb->pipe)) {
	case PIPE_CONTROL:
		_qh->ep_type = USB_ENDPOINT_XFER_CONTROL; 
		break;
	case PIPE_BULK:
		_qh->ep_type = USB_ENDPOINT_XFER_BULK;
		break;
	default:
                 printf("%s: Unsupported pipe type\n", __FUNCTION__);
                 return;
	}

	_qh->ep_is_in = usb_pipein(_urb->pipe) ? 1 : 0;

	_qh->data_toggle = DWC_OTG_HC_PID_DATA0;

	_qh->maxp = usb_maxpacket(_urb->dev, _urb->pipe);
	INIT_LIST_HEAD(&_qh->qtd_list);
	INIT_LIST_HEAD(&_qh->qh_list_entry);
	_qh->channel = NULL;


        /* FS/LS Enpoint on HS Hub
         * NOT virtual root hub */
        _qh->do_split = 0;
	// JSRXNLE_FIXME
	//        if (((_urb->dev->speed == USB_SPEED_LOW) ||
	//             (_urb->dev->speed == USB_SPEED_FULL)) &&
	//            (_urb->dev->tt) && (_urb->dev->tt->hub->devnum != 1))
	//	if (_urb->dev->devnum != 0)
	//        {
	    //                printf("QH init: EP %d: TT found at hub addr %d, for port %d\n",
	    //		       usb_pipeendpoint(_urb->pipe), _urb->dev->tt->hub->devnum,
	    //		       _urb->dev->ttport);
	//	    printf("%s: Setting do split\n", __FUNCTION__);
	//	    _qh->do_split = 1;
	    //        }


	info("DWC OTG HCD QH Initialized\n");
	info("DWC OTG HCD QH  - qh = %p\n", _qh);
	info("DWC OTG HCD QH  - Device Address = %d\n",
		    _urb->dev->devnum);
	info("DWC OTG HCD QH  - Endpoint %d, %s\n",
		    usb_pipeendpoint(_urb->pipe),
		    usb_pipein(_urb->pipe) ? "IN" : "OUT");
	info("DWC OTG HCD QH  - Type = %s\n",
		    ({ char *type; switch (_qh->ep_type) {
		    case USB_ENDPOINT_XFER_ISOC: type = "isochronous";	break;
		    case USB_ENDPOINT_XFER_INT: type = "interrupt";	break;
		    case USB_ENDPOINT_XFER_CONTROL: type = "control";	break;
		    case USB_ENDPOINT_XFER_BULK: type = "bulk";	break;
		    default: type = "?";	break;
		    }; type;}));
	return;
}


/**
 * This function adds a QH
 *
 * @return 0 if successful, negative error code otherwise.
 */
int dwc_otg_hcd_qh_add (dwc_otg_hcd_t *_hcd, dwc_otg_qh_t *_qh)
{
	int status = 0;

	if (!list_empty(&_qh->qh_list_entry)) {
		/* QH already in a schedule. */
		goto done;
	}

	/* Add the new QH to the appropriate schedule */
	if (dwc_qh_is_non_per(_qh)) {
		/* Always start in the inactive schedule. */
		list_add_tail(&_qh->qh_list_entry, &_hcd->non_periodic_sched_inactive);
	}

 done:
	return status;
}

/**
 * Removes a QH.  Memory is
 * not freed.
 *
 * @param[in] _hcd The HCD state structure.
 * @param[in] _qh QH to remove from schedule. */
void dwc_otg_hcd_qh_remove (dwc_otg_hcd_t *_hcd, dwc_otg_qh_t *_qh)
{
	if (list_empty(&_qh->qh_list_entry)) {
		/* QH is not in a schedule. */
		goto done;
	}

	if (dwc_qh_is_non_per(_qh)) {
		if (_hcd->non_periodic_qh_ptr == &_qh->qh_list_entry) {
			_hcd->non_periodic_qh_ptr = _hcd->non_periodic_qh_ptr->next;
		}
		list_del_init(&_qh->qh_list_entry);
	} 

 done:
	;
}

/**
 * Deactivates a QH. Removes the QH from the active
 * non-periodic schedule. The QH is added to the inactive non-periodic
 * schedule if any QTDs are still attached to the QH.
*
 */
void dwc_otg_hcd_qh_deactivate(dwc_otg_hcd_t *_hcd, dwc_otg_qh_t *_qh, int sched_next_periodic_split)
{
	if (dwc_qh_is_non_per(_qh)) {
		dwc_otg_hcd_qh_remove(_hcd, _qh);
		if (!list_empty(&_qh->qtd_list)) {
			/* Add back to inactive non-periodic schedule. */
			dwc_otg_hcd_qh_add(_hcd, _qh);
		}
	}
}

/**
 * This function allocates and initializes a QTD.
 *
 * @param[in] _urb The URB to create a QTD from.  Each URB-QTD pair will end up
 * pointing to each other so each pair should have a unique correlation.
 *
 * @return Returns pointer to the newly allocated QTD, or NULL on error. */
dwc_otg_qtd_t *dwc_otg_hcd_qtd_create (dwc_otg_urb_t *_urb)
{
	dwc_otg_qtd_t *qtd;

	qtd = dwc_otg_hcd_qtd_alloc ();
	if (qtd == NULL) {
		return NULL;
	}

	dwc_otg_hcd_qtd_init (qtd, _urb);
	return qtd;
}

/**
 * Initializes a QTD structure.
 *
 * @param[in] _qtd The QTD to initialize.
 * @param[in] _urb The URB to use for initialization.  */
void dwc_otg_hcd_qtd_init (dwc_otg_qtd_t *_qtd, dwc_otg_urb_t *_urb)
{
	memset (_qtd, 0, sizeof (dwc_otg_qtd_t));
	_qtd->urb = _urb;
	if (usb_pipecontrol(_urb->pipe)) {
		/*
		 * The only time the QTD data toggle is used is on the data
		 * phase of control transfers. This phase always starts with
		 * DATA1.
		 */
		_qtd->data_toggle = DWC_OTG_HC_PID_DATA1;
		_qtd->control_phase = DWC_OTG_CONTROL_SETUP;
	}

       /* start split */
        _qtd->complete_split = 0;
        _qtd->isoc_split_pos = DWC_HCSPLIT_XACTPOS_ALL;
        _qtd->isoc_split_offset = 0;

	/* Store the qtd ptr in the urb to reference what QTD. */
	_urb->hcpriv = _qtd;
	return;
}

/**
 * This function adds a QTD to the QTD-list of a QH.  It will find the correct
 * QH to place the QTD into.  If it does not find a QH, then it will create a
 * new QH. If the QH to which the QTD is added is not currently scheduled, it
 * is placed into the proper schedule based on its EP type.
 *
 * @param[in] _qtd The QTD to add
 * @param[in] _dwc_otg_hcd The DWC HCD structure
 *
 * @return 0 if successful, negative error code otherwise.
 */
int dwc_otg_hcd_qtd_add (dwc_otg_qtd_t *_qtd,
			 dwc_otg_hcd_t *_dwc_otg_hcd)
{
	struct usb_host_endpoint *ep;
	dwc_otg_qh_t *qh;
	int retval = 0;

	dwc_otg_urb_t *urb = _qtd->urb;

	/*
	 * Get the QH which holds the QTD-list to insert to. Create QH if it
	 * doesn't exist.
	 */
	ep = dwc_urb_to_endpoint(urb);
	qh = (dwc_otg_qh_t *)ep->hcpriv;
	if (qh == NULL) {
		qh = dwc_otg_hcd_qh_create (_dwc_otg_hcd, urb);
		if (qh == NULL) {
			goto done;
		}
		ep->hcpriv = qh;
	}

	retval = dwc_otg_hcd_qh_add(_dwc_otg_hcd, qh);
	if (retval == 0) {
		list_add_tail(&_qtd->qtd_list_entry, &qh->qtd_list);
	}

 done:
	return retval;
}

#endif /* CONFIG_USB_DWC_OTG */
