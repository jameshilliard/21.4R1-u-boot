/* ==========================================================================
 * $File: //dwh/usb_iip/dev/software/otg_ipmate/linux/drivers/dwc_otg_hcd.h $
 * $Revision$
 * $Date$
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
#ifndef __DWC_HCD_H__
#define __DWC_HCD_H__

#include <linux/list.h>

struct dwc_otg_device;

#include "dwc_otg_cil.h"

#define DWC_OTG_ENODEV       1
#define DWC_OTG_ENOMEM       2
#define DWC_OTG_EINVAL       3
#define DWC_OTG_EPROTO       4
#define DWC_OTG_EINPROGRESS  5
#define DWC_OTG_EPIPE        6
#define DWC_OTG_EOVERFLOW    7
#define DWC_OTG_EIO          8
#define DWC_OTG_EREMOTEIO    9

/**
 * @file
 *
 * This file contains the structures, constants, and interfaces for
 * the Host Contoller Driver (HCD).
 *
 * The Host Controller Driver (HCD) is responsible for translating requests
 * from the USB Driver into the appropriate actions on the DWC_otg controller.
 * It isolates the USBD from the specifics of the controller by providing an
 * API to the USBD.
 */

/**
 * Phases for control transfers.
 */
typedef enum dwc_otg_control_phase {
	DWC_OTG_CONTROL_SETUP,
	DWC_OTG_CONTROL_DATA,
	DWC_OTG_CONTROL_STATUS
} dwc_otg_control_phase_e;

/** Transaction types. */
typedef enum dwc_otg_transaction_type {
	DWC_OTG_TRANSACTION_NONE,
	DWC_OTG_TRANSACTION_PERIODIC,
	DWC_OTG_TRANSACTION_NON_PERIODIC,
	DWC_OTG_TRANSACTION_ALL
} dwc_otg_transaction_type_e;


/*
 * urb->transfer_flags:
 */
#define URB_SHORT_NOT_OK        0x0001  /* report short reads as errors */
#define URB_ISO_ASAP            0x0002  /* iso-only, urb->start_frame
                                         * ignored */
#define URB_NO_TRANSFER_DMA_MAP 0x0004  /* urb->transfer_dma valid on submit */
#define URB_NO_SETUP_DMA_MAP    0x0008  /* urb->setup_dma valid on submit */
#define URB_NO_FSBR             0x0020  /* UHCI-specific */
#define URB_ZERO_PACKET         0x0040  /* Finish bulk OUT with short packet */
#define URB_NO_INTERRUPT        0x0080  /* HINT: no non-error interrupt
                                         * needed */

typedef struct dwc_otg_urb
{
        /* private: usb core and host controller only fields in the urb */
        void *hcpriv;                   /* private data for host controller */

        struct list_head urb_list;      /* list head for use by the urb's
                                         * current owner */
        struct usb_device *dev;         /* (in) pointer to associated device */
        unsigned int pipe;              /* (in) pipe information */
        int status;                     /* (return) non-ISO status */
        unsigned int transfer_flags;    /* (in) URB_SHORT_NOT_OK | ...*/
        void *transfer_buffer;          /* (in) associated data buffer */
        int transfer_buffer_length;     /* (in) data buffer length */
        int actual_length;              /* (return) actual transfer length */
        unsigned char *setup_packet;    /* (in) setup packet (control only) */
        int start_frame;                /* (modify) start frame (ISO) */
        int number_of_packets;          /* (in) number of ISO packets */
        int interval;                   /* (modify) transfer interval
                                         * (INT/ISO) */
        int error_count;                /* (return) number of ISO errors */
} dwc_otg_urb_t;


#define DWC_OTG_SOF_INTR_OK             (1 << 0)
#define DWC_OTG_RX_Q_INTR_OK            (1 << 1)
#define DWC_OTG_TX_FIFO_EMPTY_INTR_OK   (1 << 2)

/**
 * A Queue Transfer Descriptor (QTD) holds the state of a bulk, control,
 * interrupt, or isochronous transfer. A single QTD is created for each URB
 * (of one of these types) submitted to the HCD. The transfer associated with
 * a QTD may require one or multiple transactions.
 *
 * A QTD is linked to a Queue Head, which is entered in either the
 * non-periodic or periodic schedule for execution. When a QTD is chosen for
 * execution, some or all of its transactions may be executed. After
 * execution, the state of the QTD is updated. The QTD may be retired if all
 * its transactions are complete or if an error occurred. Otherwise, it
 * remains in the schedule so more transactions can be executed later.
 */
typedef struct dwc_otg_qtd {
	/**
	 * Determines the PID of the next data packet for the data phase of
	 * control transfers. Ignored for other transfer types.<br>
	 * One of the following values:
	 *	- DWC_OTG_HC_PID_DATA0
	 *	- DWC_OTG_HC_PID_DATA1
	 */
	uint8_t			data_toggle;

	/** Current phase for control transfers (Setup, Data, or Status). */
	dwc_otg_control_phase_e	control_phase;

        /** Keep track of the current split type
         * for FS/LS endpoints on a HS Hub */
        uint8_t                 complete_split;

        /** How many bytes transferred during SSPLIT OUT */
        uint32_t                ssplit_out_xfer_count;

        /** Position of the ISOC split on full/low speed */
        uint8_t                 isoc_split_pos;

        /** Position of the ISOC split in the buffer for the current frame */
        uint16_t                isoc_split_offset;

	/**
	 * Holds the number of bus errors that have occurred for a transaction
	 * within this transfer.
	 */
	uint8_t 		error_count;

	/** URB for this transfer */
	dwc_otg_urb_t 		*urb;

	/** This list of QTDs */
	struct list_head  	qtd_list_entry;

} dwc_otg_qtd_t;

/**
 * A Queue Head (QH) holds the static characteristics of an endpoint and
 * maintains a list of transfers (QTDs) for that endpoint. A QH structure may
 * be entered in either the non-periodic or periodic schedule.
 */
typedef struct dwc_otg_qh {
	/**
	 * Endpoint type.
	 * One of the following values:
	 * 	- USB_ENDPOINT_XFER_CONTROL
	 *	- USB_ENDPOINT_XFER_ISOC
	 *	- USB_ENDPOINT_XFER_BULK
	 *	- USB_ENDPOINT_XFER_INT
	 */
	uint8_t 		ep_type;
	uint8_t 		ep_is_in;

	/** wMaxPacketSize Field of Endpoint Descriptor. */
	uint16_t		maxp;

	/**
	 * Determines the PID of the next data packet for non-control
	 * transfers. Ignored for control transfers.<br>
	 * One of the following values:
	 *	- DWC_OTG_HC_PID_DATA0
	 * 	- DWC_OTG_HC_PID_DATA1
	 */
	uint8_t			data_toggle;

	/** Ping state if 1. */
	uint8_t 		ping_state;

	/**
	 * List of QTDs for this QH.
	 */
	struct list_head 	qtd_list;

	/** Host channel currently processing transfers for this QH. */
	dwc_hc_t		*channel;

	/** QTD currently assigned to a host channel for this QH. */
	dwc_otg_qtd_t		*qtd_in_process;

        /** Full/low speed endpoint on high-speed hub requires split. */
        uint8_t                 do_split;

	/** @name Periodic schedule information */
	/** @{ */

	/** Bandwidth in microseconds per (micro)frame. */
	uint8_t			usecs;

	/** Entry for QH in either the periodic or non-periodic schedule. */
	struct list_head        qh_list_entry;
} dwc_otg_qh_t;


/* Virtual Root HUB */
struct virt_root_hub {
        int devnum; /* Address of Root Hub endpoint */
        void *dev;  
};

/* USB HUB CONSTANTS (not OHCI-specific; see hub.h) */

/* destination of request */
#define RH_INTERFACE               0x01
#define RH_ENDPOINT                0x02
#define RH_OTHER                   0x03

#define RH_CLASS                   0x20
#define RH_VENDOR                  0x40

/* Requests: bRequest << 8 | bmRequestType */
#define RH_GET_STATUS           0x0080
#define RH_CLEAR_FEATURE        0x0100
#define RH_SET_FEATURE          0x0300
#define RH_SET_ADDRESS          0x0500
#define RH_GET_DESCRIPTOR       0x0680
#define RH_SET_DESCRIPTOR       0x0700
#define RH_GET_CONFIGURATION    0x0880
#define RH_SET_CONFIGURATION    0x0900
#define RH_GET_STATE            0x0280
#define RH_GET_INTERFACE        0x0A80
#define RH_SET_INTERFACE        0x0B00
#define RH_SYNC_FRAME           0x0C80
/* Our Vendor Specific Request */
#define RH_SET_EP               0x2000


/* Hub port features */
#define RH_PORT_CONNECTION         0x00
#define RH_PORT_ENABLE             0x01
#define RH_PORT_SUSPEND            0x02
#define RH_PORT_OVER_CURRENT       0x03
#define RH_PORT_RESET              0x04
#define RH_PORT_POWER              0x08
#define RH_PORT_LOW_SPEED          0x09

#define RH_C_PORT_CONNECTION       0x10
#define RH_C_PORT_ENABLE           0x11
#define RH_C_PORT_SUSPEND          0x12
#define RH_C_PORT_OVER_CURRENT     0x13
#define RH_C_PORT_RESET            0x14

/* Hub features */
#define RH_C_HUB_LOCAL_POWER       0x00
#define RH_C_HUB_OVER_CURRENT      0x01

#define RH_DEVICE_REMOTE_WAKEUP    0x00
#define RH_ENDPOINT_STALL          0x01

#define RH_ACK                     0x01
#define RH_REQ_ERR                 -1
#define RH_NACK                    0x00


#define USB_REQUEST_GET_STATUS              0x00
#define USB_REQUEST_CLEAR_FEATURE           0x01
#define USB_REQUEST_SET_FEATURE             0x03
#define USB_REQUEST_SET_ADDRESS             0x05
#define USB_REQUEST_GET_DESCRIPTOR          0x06
#define USB_REQUEST_SET_DESCRIPTOR          0x07
#define USB_REQUEST_GET_CONFIGURATION       0x08
#define USB_REQUEST_SET_CONFIGURATION       0x09
#define USB_REQUEST_GET_INTERFACE           0x0A
#define USB_REQUEST_SET_INTERFACE           0x0B
#define USB_REQUEST_SYNCH_FRAME             0x0C


#define ClearHubFeature         (0x2000 | USB_REQUEST_CLEAR_FEATURE)
#define ClearPortFeature        (0x2300 | USB_REQUEST_CLEAR_FEATURE)
#define GetHubDescriptor        (0xa000 | USB_REQUEST_GET_DESCRIPTOR)
#define GetHubStatus            (0xa000 | USB_REQUEST_GET_STATUS)
#define GetPortStatus           (0xa300 | USB_REQUEST_GET_STATUS)
#define SetHubFeature           (0x2000 | USB_REQUEST_SET_FEATURE)
#define SetPortFeature          (0x2300 | USB_REQUEST_SET_FEATURE)


#define USB_PORT_FEATURE_CONNECTION        0
#define USB_PORT_FEATURE_ENABLE            1
#define USB_PORT_FEATURE_SUSPEND           2
#define USB_PORT_FEATURE_OVER_CURRENT      3
#define USB_PORT_FEATURE_RESET             4
#define USB_PORT_FEATURE_POWER             8
#define USB_PORT_FEATURE_LOWSPEED          9
#define USB_PORT_FEATURE_HIGHSPEED         10
#define USB_PORT_FEATURE_C_CONNECTION      16
#define USB_PORT_FEATURE_C_ENABLE          17
#define USB_PORT_FEATURE_C_SUSPEND         18
#define USB_PORT_FEATURE_C_OVER_CURRENT    19
#define USB_PORT_FEATURE_C_RESET           20
#define USB_PORT_FEATURE_TEST              21
#define USB_PORT_FEATURE_INDICATOR         22




/**
 * This structure holds the state of the HCD, including the non-periodic and
 * periodic schedules.
 */
typedef struct dwc_otg_hcd {

	/** DWC OTG Core Interface Layer */
	dwc_otg_core_if_t       *core_if;

	/** Internal DWC HCD Flags */	
	volatile union dwc_otg_hcd_internal_flags {
		uint32_t d32;
		struct {
#ifdef __BIG_ENDIAN_BITFIELD
			unsigned reserved : 26;
			unsigned port_over_current_change : 1;
			unsigned port_suspend_change : 1;
			unsigned port_enable_change : 1;
			unsigned port_reset_change : 1;
			unsigned port_connect_status : 1;
			unsigned port_connect_status_change : 1;
#else
			unsigned port_connect_status_change : 1;
			unsigned port_connect_status : 1;
			unsigned port_reset_change : 1;
			unsigned port_enable_change : 1;
			unsigned port_suspend_change : 1;
			unsigned port_over_current_change : 1;
			unsigned reserved : 26;
#endif
		} b;
	} flags;

        /* Virtual root hub */
        struct virt_root_hub rh;

	/**
	 * Inactive items in the non-periodic schedule. This is a list of
	 * Queue Heads. Transfers associated with these Queue Heads are not
	 * currently assigned to a host channel.
	 */
	struct list_head 	non_periodic_sched_inactive;

	/**
	 * Active items in the non-periodic schedule. This is a list of
	 * Queue Heads. Transfers associated with these Queue Heads are
	 * currently assigned to a host channel.
	 */
	struct list_head 	non_periodic_sched_active;

	/**
	 * Pointer to the next Queue Head to process in the active
	 * non-periodic schedule.
	 */
	struct list_head 	*non_periodic_qh_ptr;

	/**
	 * Frame number read from the core at SOF. The value ranges from 0 to
	 * DWC_HFNUM_MAX_FRNUM.
	 */
	uint16_t		frame_number;

	/**
	 * Free host channels in the controller. This is a list of
	 * dwc_hc_t items.
	 */
	struct list_head 	free_hc_list;

	/**
	 * Number of host channels assigned to non-periodic transfers.
	 */
	int			non_periodic_channels;

	/**
	 * Array of pointers to the host channel descriptors. Allows accessing
	 * a host channel descriptor given the host channel number. This is
	 * useful in interrupt handlers.
	 */
	dwc_hc_t		*hc_ptr_array[MAX_EPS_CHANNELS];

	/**
	 * Buffer to use for any data received during the status phase of a
	 * control transfer. Normally no data is transferred during the status
	 * phase. This buffer is used as a bit bucket.
	 */
	uint8_t			*status_buf;

	/**
	 * DMA address for status_buf.
	 */
	dma_addr_t		status_buf_dma;
#define DWC_OTG_HCD_STATUS_BUF_SIZE 64	

} dwc_otg_hcd_t;



/** @name Transaction Execution Functions */
/** @{ */
extern dwc_otg_transaction_type_e dwc_otg_hcd_select_transactions(dwc_otg_hcd_t *_hcd);
extern void dwc_otg_hcd_queue_transactions(dwc_otg_hcd_t *_hcd,
					   dwc_otg_transaction_type_e _tr_type);
extern void dwc_otg_hcd_complete_urb(dwc_otg_hcd_t *_hcd, dwc_otg_urb_t *_urb,
				     int _status);
/** @} */

/** @name Interrupt Handler Functions */
/** @{ */
extern int32_t dwc_otg_hcd_handle_intr (void);
extern int32_t dwc_otg_hcd_handle_sof_intr (dwc_otg_hcd_t *_dwc_otg_hcd);
extern int32_t dwc_otg_hcd_handle_rx_status_q_level_intr (dwc_otg_hcd_t *_dwc_otg_hcd);
extern int32_t dwc_otg_hcd_handle_np_tx_fifo_empty_intr (dwc_otg_hcd_t *_dwc_otg_hcd);
extern int32_t dwc_otg_hcd_handle_perio_tx_fifo_empty_intr (dwc_otg_hcd_t *_dwc_otg_hcd);
extern int32_t dwc_otg_hcd_handle_incomplete_periodic_intr(dwc_otg_hcd_t *_dwc_otg_hcd);
extern int32_t dwc_otg_hcd_handle_port_intr (dwc_otg_hcd_t *_dwc_otg_hcd);
extern int32_t dwc_otg_hcd_handle_conn_id_status_change_intr (dwc_otg_hcd_t *_dwc_otg_hcd);
extern int32_t dwc_otg_hcd_handle_disconnect_intr (dwc_otg_hcd_t *_dwc_otg_hcd);
extern int32_t dwc_otg_hcd_handle_hc_intr (dwc_otg_hcd_t *_dwc_otg_hcd);
extern int32_t dwc_otg_hcd_handle_hc_n_intr (dwc_otg_hcd_t *_dwc_otg_hcd, uint32_t _num);
/** @} */


/** @name Schedule Queue Functions */
/** @{ */

/* Implemented in dwc_otg_hcd_queue.c */
extern void dwc_otg_hcd_qh_free (dwc_otg_qh_t *_qh);
extern void dwc_otg_hcd_qh_remove (dwc_otg_hcd_t *_hcd, dwc_otg_qh_t *_qh);
extern void dwc_otg_hcd_qh_deactivate (dwc_otg_hcd_t *_hcd, dwc_otg_qh_t *_qh, int sched_csplit);

/** Remove and free a QH */
static inline void dwc_otg_hcd_qh_remove_and_free (dwc_otg_hcd_t *_hcd,
						   dwc_otg_qh_t *_qh)
{
	dwc_otg_hcd_qh_remove (_hcd, _qh);
	dwc_otg_hcd_qh_free (_qh);
}

/** Allocates memory for a QH structure.
 * @return Returns the memory allocate or NULL on error. */
static inline dwc_otg_qh_t *dwc_otg_hcd_qh_alloc (void)
{
        return (dwc_otg_qh_t *) malloc (sizeof(dwc_otg_qh_t));
}

extern dwc_otg_qtd_t *dwc_otg_hcd_qtd_create (dwc_otg_urb_t *urb);
extern void dwc_otg_hcd_qtd_init (dwc_otg_qtd_t *qtd, dwc_otg_urb_t *urb);
extern int dwc_otg_hcd_qtd_add (dwc_otg_qtd_t *qtd, dwc_otg_hcd_t *dwc_otg_hcd);

/** Allocates memory for a QTD structure.
 * @return Returns the memory allocate or NULL on error. */
static inline dwc_otg_qtd_t *dwc_otg_hcd_qtd_alloc (void)
{
	return (dwc_otg_qtd_t *) malloc (sizeof(dwc_otg_qtd_t));
}

/** Frees the memory for a QTD structure.  QTD should already be removed from
 * list.
 * @param[in] _qtd QTD to free.*/
static inline void dwc_otg_hcd_qtd_free (dwc_otg_qtd_t *_qtd)
{
	free (_qtd);
}

/** Removes a QTD from list.
 * @param[in] _qtd QTD to remove from list. */
static inline void dwc_otg_hcd_qtd_remove (dwc_otg_qtd_t *_qtd)
{
	list_del (&_qtd->qtd_list_entry);
}

/** Remove and free a QTD */
static inline void dwc_otg_hcd_qtd_remove_and_free (dwc_otg_qtd_t *_qtd)
{
	dwc_otg_hcd_qtd_remove (_qtd);
	dwc_otg_hcd_qtd_free (_qtd);
}

/** @} */

#define DWC_OTG_DEVNUM_INVALID    -1
#define DWC_OTG_UNIT0              0
#define DWC_OTG_UNIT1              1

extern int usb_multi_root_hub;
extern int usb_num_root_hub;
extern int usb_root_hub_devnum[USB_MAX_ROOT_HUB];
extern dwc_otg_hcd_t dwc_otg_hcd_struct[DWC_OTG_UNIT_MAX];
 
/* 
 * dwc_otg_hcd_get_unit
 *
 * Get USB unit (root hub) based on devnum
 *
 * This func handles only max 2 usb units as of now, as that is 
 * the MAX on any Octeon in forseeable future
 */
static inline int dwc_otg_hcd_get_unit(int devnum)
{
	/* If not multi root hub then always return unit 0 */
	if (!usb_multi_root_hub)
		return DWC_OTG_UNIT0;

	/*
	 * A special case where devnum is 0
	 *
	 * If there is no devnum it means that a get descriptor is being
	 * requested from the hub for a device which is not yet 
	 * assigned address. We assume that this will happen only during
	 * initial enumeration as we do not support PnP. Hence, if we 
	 * have not yet reached hub 1 we assume that device belongs to hub 0
	 */
	if (devnum == 0) {
		if (usb_root_hub_devnum[DWC_OTG_UNIT1] != 
		    DWC_OTG_DEVNUM_INVALID) {
			return DWC_OTG_UNIT1;
		} else {
			return DWC_OTG_UNIT0;
		}
	}

	/* 
	 * if root hub 1's devnum is not valid or root hub 1's devnum 
	 * is valid and greater than devnum (input arg), 
	 * then devnum belongs to unit 0 else unit 1
	 */
	if ((usb_root_hub_devnum[DWC_OTG_UNIT1] == DWC_OTG_DEVNUM_INVALID)
	    || (devnum < usb_root_hub_devnum[DWC_OTG_UNIT1])) {
		return DWC_OTG_UNIT0;
	} else {
		return DWC_OTG_UNIT1;
	}
}

extern dwc_otg_device_t dwc_otg_device_struct[DWC_OTG_UNIT_MAX];

/** Gets the usb_host_endpoint associated with an URB. */
static inline struct usb_host_endpoint *dwc_urb_to_endpoint(dwc_otg_urb_t *_urb)
{
	dwc_otg_device_t *dev;
	usb_host_dev_info_t *dinfo;
	int ep_num = usb_pipeendpoint(_urb->pipe);

	dev = &dwc_otg_device_struct[dwc_otg_hcd_get_unit(_urb->dev->devnum)];
	dinfo = &dev->dev_info[_urb->dev->devnum];

	if (usb_pipein(_urb->pipe))
		return &dinfo->ep_in[ep_num];
	else
		return &dinfo->ep_out[ep_num];
}

/**
 * Gets the endpoint number from a _bEndpointAddress argument. The endpoint is
 * qualified with its direction (possible 32 endpoints per device).
 */
#define dwc_ep_addr_to_endpoint(_bEndpointAddress_) ((_bEndpointAddress_ & USB_ENDPOINT_NUMBER_MASK) | \
                                                     ((_bEndpointAddress_ & USB_DIR_IN) != 0) << 4)

/** Gets the QH that contains the list_head */
#define dwc_list_to_qh(_list_head_ptr_) (container_of(_list_head_ptr_,dwc_otg_qh_t,qh_list_entry))

/** Gets the QTD that contains the list_head */
#define dwc_list_to_qtd(_list_head_ptr_) (container_of(_list_head_ptr_,dwc_otg_qtd_t,qtd_list_entry))

/** Check if QH is non-periodic  */
#define dwc_qh_is_non_per(_qh_ptr_) ((_qh_ptr_->ep_type == USB_ENDPOINT_XFER_BULK) || \
                                     (_qh_ptr_->ep_type == USB_ENDPOINT_XFER_CONTROL))

/** High bandwidth multiplier as encoded in highspeed endpoint descriptors */
#define dwc_hb_mult(wMaxPacketSize) (1 + (((wMaxPacketSize) >> 11) & 0x03))

/** Packet size for any kind of endpoint descriptor */
#define dwc_max_packet(wMaxPacketSize) ((wMaxPacketSize) & 0x07ff)

#endif
