/***********************************************************************
Copyright 2003-2006 Raza Microelectronics, Inc.(RMI).
This is a derived work from software originally provided by the external
entity identified below. The licensing terms and warranties specified in
the header of the original work apply to this derived work.
Contribution by RMI: Adapted to XLS Bootloader. 
*****************************#RMI_1#**********************************/

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
/*
 * IMPORTANT NOTES
 * 1 - you MUST define LITTLEENDIAN in the configuration file for the
 *     board or this driver will NOT work!
 * 2 - this driver is intended for use with USB Mass Storage Devices
 *     (BBB) ONLY. There is NO support for Interrupt or Isochronous pipes!
 */

#define CONFIG_USB_EHCI
#define ETIMEDOUT 2
#include <common.h>
#include <part.h>

#ifdef CONFIG_USB_EHCI

#include <malloc.h>
#include <usb.h>
#include "rmi/types.h"
#include "rmi/rmi_processor.h"
#include "rmi/on_chip.h"
#include "rmi/xlr_cpu.h"
#include "usb_ehci.h"

#undef XLS_USB_DEBUG 
//#define XLS_USB_DEBUG
#ifdef XLS_USB_DEBUG
#define Message(a,b...) printf("\n[%s]@[%d]" a"\n",__FUNCTION__,__LINE__,##b)
#else
#define Message(a,b...) 
#endif

#define ErrorMsg(a,b...) printf("\nERROR - [%s]@[%d] "a"\n",__FUNCTION__,__LINE__,##b)

#define USB_GLUE_START 0xbef25000
#define USB_EHCI_BASE 0xbef24000

/* magic numbers that can affect system performance */
#define EHCI_TUNE_CERR      3   /* 0-3 qtd retries; 0 == don't stop */
#define EHCI_TUNE_RL_HS     4   /* nak throttle; see 4.9 */
#define EHCI_TUNE_RL_TT     0
#define EHCI_TUNE_MULT_HS   1   /* 1-3 transactions/uframe; 4.10.3 */
#define EHCI_TUNE_MULT_TT   1
#define EHCI_TUNE_FLS       2   /* (small) 256 frame schedule */

#define mdelay(n) ({unsigned long msec=(n); while (msec--) udelay(1000);})

/*These are filled up in "attach".*/
extern unsigned long usb_glue_start;
extern unsigned long usb_ehci_start;
extern unsigned long usb_ohci0_start;
extern unsigned long usb_ohci1_start;
#define ___my_swab32(x) \
({ \
     unsigned int __x = (x); \
         ((unsigned int)( \
                           (((unsigned int)(__x) & (unsigned int)0x000000ffUL) << 24) | \
                                   (((unsigned int)(__x) & (unsigned int)0x0000ff00UL) <<  8)\
                                   | \
                                    (((unsigned int)(__x) & (unsigned int)0x00ff0000UL)\
                                            >>  8) | \
                                                   (((unsigned int)(__x) &\
                                                     (unsigned int)0xff000000UL) >>24)\
                                                   )); \
         })


static inline u32 xls_readl(void *addr)
{
	return cpu_to_le32((*(volatile u32 *)addr));
}

void static inline xls_writel(u32 val, void *reg)
{
	*(volatile u32 *)(reg) = ___my_swab32(val);
}

static int handshake (void *ptr, unsigned int mask, unsigned int done, int
        usec);
/*Lang. ID*/
static unsigned char root_hub_str_index0[] =
{
	0x04,			/*  __u8  bLength; */
	0x03,			/*  __u8  bDescriptorType; String-descriptor */
	0x09,			/*  __u8  lang ID */
	0x04,			/*  __u8  lang ID */
};

/*Serial Number.*/
static unsigned char root_hub_str_index1[] =
{
	36,			/*  __u8  bLength; */
	0x03,			/*  __u8  bDescriptorType; String-descriptor */
	'2',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'.',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'0',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	' ',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'H',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'I',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'G',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'H',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'S',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'P',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'E',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'E',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'D',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	' ',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'H',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'U',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'B',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
};

/*Product Desc*/
static unsigned char root_hub_str_index2[] =
{
	28,			/*  __u8  bLength; */
	0x03,			/*  __u8  bDescriptorType; String-descriptor */
	'E',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'H',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'C',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'I',			/*  __u8  Unicode */
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

/*Manufacture */
static unsigned char root_hub_str_index3[] =
{
	8,			/*  __u8  bLength; */
	0x03,			/*  __u8  bDescriptorType; String-descriptor */
	'R',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'M',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'I',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
};


/* usb 2.0 root hub device descriptor */
static u8 usb2_rh_dev_descriptor [18] = {
	0x12,       /*  __u8  bLength; */
	0x01,       /*  __u8  bDescriptorType; Device */
	0x00, 0x02, /*  __le16 bcdUSB; v2.0 */

	0x09,	    /*  __u8  bDeviceClass; HUB_CLASSCODE */
	0x00,	    /*  __u8  bDeviceSubClass; */
	0x01,       /*  __u8  bDeviceProtocol; [ usb 2.0 single TT ]*/
	0x40,       /*  __u8  bMaxPacketSize0; 64 Bytes */

	0x00, 0x00, /*  __le16 idVendor; */
 	0x00, 0x00, /*  __le16 idProduct; */
	0x18, 0x2e, /*  __le16 bcdDevice */

	0x03,       /*  __u8  iManufacturer; */
	0x02,       /*  __u8  iProduct; */
	0x01,       /*  __u8  iSerialNumber; */
	0x01        /*  __u8  bNumConfigurations; */
};

static u8 hs_rh_config_descriptor [] = {

	/* one configuration */
	0x09,       /*  __u8  bLength; */
	0x02,       /*  __u8  bDescriptorType; Configuration */
	0x19, 0x00, /*  __le16 wTotalLength; */
	0x01,       /*  __u8  bNumInterfaces; (1) */
	0x01,       /*  __u8  bConfigurationValue; */
	0x00,       /*  __u8  iConfiguration; */
	0xc0,       /*  __u8  bmAttributes; 
				 Bit 7: must be set,
				     6: Self-powered,
				     5: Remote wakeup,
				     4..0: resvd */
	0x00,       /*  __u8  MaxPower; */
      
	/* USB 1.1:
	 * USB 2.0, single TT organization (mandatory):
	 *	one interface, protocol 0
	 *
	 * USB 2.0, multiple TT organization (optional):
	 *	two interfaces, protocols 1 (like single TT)
	 *	and 2 (multiple TT mode) ... config is
	 *	sometimes settable
	 *	NOT IMPLEMENTED
	 */

	/* one interface */
	0x09,       /*  __u8  if_bLength; */
	0x04,       /*  __u8  if_bDescriptorType; Interface */
	0x00,       /*  __u8  if_bInterfaceNumber; */
	0x00,       /*  __u8  if_bAlternateSetting; */
	0x01,       /*  __u8  if_bNumEndpoints; */
	0x09,       /*  __u8  if_bInterfaceClass; HUB_CLASSCODE */
	0x00,       /*  __u8  if_bInterfaceSubClass; */
	0x00,       /*  __u8  if_bInterfaceProtocol; [usb1.1 or single tt] */
	0x00,       /*  __u8  if_iInterface; */
     
	/* one endpoint (status change endpoint) */
	0x07,       /*  __u8  ep_bLength; */
	0x05,       /*  __u8  ep_bDescriptorType; Endpoint */
	0x81,       /*  __u8  ep_bEndpointAddress; IN Endpoint 1 */
 	0x03,       /*  __u8  ep_bmAttributes; Interrupt */
 	0x02, 0x00, /*  __le16 ep_wMaxPacketSize; 1 + (MAX_ROOT_PORTS / 8) */
	0x0c        /*  __u8  ep_bInterval; (256ms -- usb 2.0 spec) */
};

#define OK(x)			len = (x); break
#if 0
#define wmb() __asm__ volatile\
                (\
                ".set noreorder\n"\
                "sync\n"\
                ".set reorder\n")
#endif

#define HALT_BIT __constant_cpu_to_le32(QTD_STS_HALT)

/* global ehci_t */
ehci_t gehci;

static struct ehci_qtd *qtd_being_used[10];
static struct ehci_qh *debug_qh;
static int submitted_qtd_counter=0;

/*Forward declaration goes here.*/
void ehci_qh_free(ehci_t *ehci, struct ehci_qh *qh);
void qh_unlink_async_disable(struct usb_device *dev, struct ehci_qh *qh);
int alloc_usb_pool(ehci_t *ehci);
int ehci_mem_init(ehci_t *ehci);
void ehci_qtd_free(ehci_t *ehci, struct ehci_qtd *qtd);
struct ehci_qtd * ehci_qtd_alloc(ehci_t *ehci);
int usb_maxpacket(struct usb_device *dev,unsigned long pipe);
struct list_head *qh_urb_transaction(struct usb_device *,ehci_t *,struct devrequest *,void *,int ,struct list_head *,int);
struct ehci_qh * ehci_qh_alloc(ehci_t *ehci);
void ehci_qtd_init (struct ehci_qtd *qtd, unsigned long dma);
int submit_common_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
                int transfer_len, struct devrequest *setup, int interval);

int submit_bulk_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
		int transfer_len)
{
    Message("\nSubmit BULK MSG Called.\n");
    return submit_common_msg(dev, pipe, buffer, transfer_len, NULL, 0);
}

/* Looks like OSes (linux) expect the USB mode to be set */
void rmi_usb_set_mode(void)
{
#if 0
	/* supported only on XLS boards */
	if (!chip_processor_id_xls()) {
		printf("Supported on XLS boards only.\n");
		return;
	}

	phoenix_reg_t *condor_usb_ctrl_mmio = (phoenix_reg_t *)USB_GLUE_START;
	{
		phoenix_reg_t *gpio_mmio = phoenix_io_mmio(0x18000);
		volatile unsigned int value = gpio_mmio[21];
		if ((value >> 22) & 0x01) {
			printf("Detected USB Host mode..\n");
			cpu_write_reg(condor_usb_ctrl_mmio,  0, 0x02000000);
		}
		else {
			printf("Detected USB Device mode..\n");
			cpu_write_reg(condor_usb_ctrl_mmio,  0, 0x01000000);
	//		flag_usb_dev_mode = 0x01;
		}
	}
#endif

}

static void ehci_hub_descriptor (struct ehci_hcd *ehci,
                struct usb_hub_descriptor *desc)
{
	int		ports = HCS_N_PORTS (ehci->hcs_params);
	u16		temp;

	desc->bDescriptorType = 0x29;
	desc->bPwrOn2PwrGood = 10;	/* ehci 1.0, 2.3.9 says 20ms max */
	desc->bHubContrCurrent = 0;

	desc->bNbrPorts = ports;
    Message("\n%d number of ports present on ehci-hub",ports);
	temp = 1 + (ports / 8);
	desc->bLength = 7 + 2 * temp;
    Message("Total Desc Length %d",desc->bLength);

#define bitmap DeviceRemovable
	/* two bitmaps:  ports removable, and usb 1.0 legacy PortPwrCtrlMask */
	memset (&desc->bitmap[0], 0, temp);
	memset (&desc->bitmap[temp], 0xff, temp);
#undef bitmap
	temp = 0x0008;			/* per-port overcurrent reporting */
	if (HCS_PPC (ehci->hcs_params))
		temp |= 0x0001;		/* per-port power control */
	else
		temp |= 0x0002;		/* no power switching */
	desc->wHubCharacteristics = (unsigned short)cpu_to_le16 (temp);
}


/*Fill the QTD*/
static int
qtd_fill (struct ehci_qtd *qtd, unsigned long buf, size_t len,
		int token, int maxpacket)
{
	int	i, count;
	u64	addr = buf;

    qtd_being_used[submitted_qtd_counter] = qtd;
    submitted_qtd_counter++;
    
	/* one buffer entry per 4K ... first might be short or unaligned */
	qtd->hw_buf [0] = cpu_to_le32 ((u32)addr);
	qtd->hw_buf_hi [0] = cpu_to_le32 ((u32)(addr >> 32));
	count = 0x1000 - (buf & 0x0fff);	/* rest of that page */
	if ( (len < count))		/* ... iff needed */
		count = len;
	else {
        /*Request crosses the page boundry*/
		buf +=  0x1000;
		buf &= ~0x0fff;

		/* per-qtd limit: from 16K to 20K (best alignment) */
		for (i = 1; count < len && i < 5; i++) {
			addr = buf;
			qtd->hw_buf [i] = cpu_to_le32 ((u32)addr);
			qtd->hw_buf_hi [i] = cpu_to_le32 ((u32)(addr >> 32));
			buf += 0x1000;
			if ((count + 0x1000) < len)
				count += 0x1000;
			else
				count = len;
		}

		/* short packets may only terminate transfers */
		if (count != len){
			count -= (count % maxpacket);
            if(count%maxpacket){
                ErrorMsg("Unknown condition evalute to true.");
            }            
        }
	}
	qtd->hw_token = cpu_to_le32 ((count << 16) | token);
	qtd->length = count;
	return count;
}

static void qtd_list_free (struct ehci_hcd  *ehci, struct list_head *qtd_list) 
{
    struct list_head    *entry, *temp;
    list_for_each_safe (entry, temp, qtd_list) {
        struct ehci_qtd *qtd;
        qtd = list_entry (entry, struct ehci_qtd, qtd_list);
        list_del (&qtd->qtd_list);
        ehci_qtd_free (ehci, qtd);
    }
}


/*
 * create a list of filled qtds for this Request; won't link into qh.
 */

struct list_head *qh_urb_transaction(struct usb_device *dev, 
        ehci_t *ehci, struct devrequest *setup, void *buffer, int transfer_len, 
        struct list_head *head, int pipe)
{
    struct ehci_qtd *qtd = NULL;
    struct ehci_qtd *qtd_prev = NULL;
    int         len, maxpacket;
    int         is_input;
    u32         token;
    unsigned long buf = (unsigned long)buffer;

    maxpacket = usb_maxpacket(dev,pipe);
   
    if(!maxpacket){
        ErrorMsg("Can not transmit on this endpoint - Maxsize %d",maxpacket);
        return NULL;
    }
     
    /*Request map to sequences of QTDs : One logical transaction*/
    qtd = ehci_qtd_alloc(ehci);
    if(!qtd){
        ErrorMsg("1st Qtd Allocation Failed.");
        return NULL;
    }
    list_add_tail (&qtd->qtd_list, head);
    token = QTD_STS_ACTIVE;
    token |= (EHCI_TUNE_CERR << 10);
    len = transfer_len;
    is_input = usb_pipein (pipe);

    if(usb_pipecontrol(pipe)){
        /*SETUP pid*/
        qtd_fill (qtd, virt_to_phys(setup), sizeof (struct devrequest),
                token | (2 /* "setup" */ << 8), 8);
        /* ... and always at least one more pid */
        token ^= QTD_TOGGLE;    /*Dont know the usage*/
        qtd_prev = qtd;
        qtd = ehci_qtd_alloc (ehci);
        if(!qtd){
            ehci_qtd_free(ehci,qtd_prev);
            ErrorMsg("2nd Qtd Allocation Failed.");
            return NULL;
        }
        qtd_prev->hw_next = QTD_NEXT (qtd->qtd_dma);
        list_add_tail (&qtd->qtd_list, head);

        /* for zero length DATA stages, STATUS is always IN */
        if (len == 0)
            token |= (1 /* "in" */ << 8);
    }
    if(is_input)
        token |= (1 /* "in" */ << 8);
    /* else it's already initted to "out" pid (0 << 8) */
    
    /*
	 * buffer gets wrapped in one or more qtds;
	 * last one may be "short" (including zero len)
	 * and may serve as a control status ack
	 */
	for (;;) {
		int this_qtd_len;

		this_qtd_len = qtd_fill (qtd, buf, len, token, maxpacket);
		len -= this_qtd_len;
		buf += this_qtd_len;

        /*In case of short xfr halt this endpoint*/
		if (is_input)   
			qtd->hw_alt_next = EHCI_LIST_END; // NOT ehci->async->hw_alt_next;

		/* qh makes control packets use qtd toggle; maybe switch it */
		if ((maxpacket & (this_qtd_len + (maxpacket - 1))) == 0)
			token ^= QTD_TOGGLE;    /*Dont know the usage*/

		if (len <= 0)
			break;

		qtd_prev = qtd;
		qtd = ehci_qtd_alloc (ehci);
		if (!qtd)
            goto cleanup; 
		qtd_prev->hw_next = QTD_NEXT (qtd->qtd_dma);
		list_add_tail (&qtd->qtd_list, head);
	}

    /*Setup qtd for Status Phase in case of Control X'fr only which has data
     * phase for other control transaction zero len data xfr will act as a
     * status phase*/
    if (transfer_len != 0 && usb_pipecontrol(pipe)) {
        token ^= 0x0100;    /* "in" <--> "out" */
        token |= QTD_TOGGLE; /* force * DATA1 */ 
        qtd_prev = qtd;
        qtd = ehci_qtd_alloc (ehci);
        if (!qtd)
            goto cleanup;
        qtd_prev->hw_next = QTD_NEXT (qtd->qtd_dma);
        list_add_tail (&qtd->qtd_list, head);
        /* never any data in such packets */
        qtd_fill (qtd, 0, 0, token, 0);
    }
    qtd->hw_token |= __constant_cpu_to_le32 (QTD_IOC);
    return head;

cleanup:
    qtd_list_free(ehci,head);
    return NULL;
}

#define	QH_ADDR_MASK	__constant_cpu_to_le32(0x7f)

static inline void
qh_update (struct ehci_hcd *ehci, struct ehci_qh *qh, struct ehci_qtd *qtd)
{
	/* writes to an active overlay are unsafe */
//	BUG_ON(qh->qh_state != QH_STATE_IDLE);

	qh->hw_qtd_next = QTD_NEXT (qtd->qtd_dma);
	qh->hw_alt_next = EHCI_LIST_END;
	/* Except for control endpoints, we make hardware maintain data
	 * toggle (like OHCI) ... here (re)initialize the toggle in the QH,
	 * and set the pseudo-toggle in udev. Only usb_clear_halt() will
	 * ever clear it.
	 */
	if (!(qh->hw_info1 & cpu_to_le32(1 << 14))) {
		unsigned	is_out, epnum;

		is_out = !(qtd->hw_token & cpu_to_le32(1 << 8));
		epnum = (le32_to_cpup(&qh->hw_info1) >> 8) & 0x0f;
		if ( (!usb_gettoggle (qh->dev, epnum, is_out))) {
			qh->hw_token &= ~__constant_cpu_to_le32 (QTD_TOGGLE);
			usb_settoggle (qh->dev, epnum, is_out, 1);
		}
	}

	/* HC must see latest qtd and qh data before we clear ACTIVE+HALT */
	wmb ();
	qh->hw_token &= __constant_cpu_to_le32 (QTD_TOGGLE | QTD_STS_PING);
}

/* if it weren't for a common silicon quirk (writing the dummy into the qh
 * overlay, so qh->hw_token wrongly becomes inactive/halted), only fault
 * recovery (including urb dequeue) would need software changes to a QH...
 */
static void
qh_refresh (struct ehci_hcd *ehci, struct ehci_qh *qh)
{
	struct ehci_qtd *qtd;
	if (list_empty (&qh->qtd_list))
		qtd = qh->dummy;
	else {
		qtd = list_entry (qh->qtd_list.next,
				struct ehci_qtd, qtd_list);
		/* first qtd may already be partially processed */
		if (cpu_to_le32 (qtd->qtd_dma) == qh->hw_current)
			qtd = NULL;
	}

	if (qtd)
		qh_update (ehci, qh, qtd);
}

/*
 * Each QH holds a qtd list; a QH is used for everything except iso.
 *
 * For interrupt urbs, the scheduler must set the microframe scheduling
 * mask(s) each time the QH gets scheduled.  For highspeed, that's
 * just one microframe in the s-mask.  For split interrupt transactions
 * there are additional complications: c-mask, maybe FSTNs.
 */
static struct ehci_qh * qh_make (struct ehci_hcd *ehci,struct usb_device *dev, 
        int pipe) 
{
    
	struct ehci_qh		*qh = ehci_qh_alloc (ehci);
	u32			info1 = 0, info2 = 0;
	int			is_input, type;

	if (!qh)
		return qh;
	/*
	 * init endpoint/device data for this QH
	 */
	info1 |= usb_pipeendpoint (pipe) << 8;
	info1 |= usb_pipedevice (pipe) << 0;
	info2 |= (dev->portnr << 23) |(dev->parent->devnum << 16);

	is_input = usb_pipein (pipe);
	type = usb_pipetype (pipe);

    info1 |= (2 << 12);	/* EPS "high" */
    if (type == PIPE_CONTROL) {
        info1 |= (EHCI_TUNE_RL_HS << 28);
        info1 |= 64 << 16;	/* usb2 fixed maxpacket */
        info1 |= 1 << 14;	/* toggle from qtd */
        info2 |= (EHCI_TUNE_MULT_HS << 30);
    } else if (type == PIPE_BULK) {
        info1 |= (EHCI_TUNE_RL_HS << 28);
        info1 |= 512 << 16;	/* usb2 fixed maxpacket */
        info2 |= (EHCI_TUNE_MULT_HS << 30);
    }

	qh->qh_state = QH_STATE_IDLE;
	qh->hw_info1 = cpu_to_le32 (info1);
	qh->hw_info2 = cpu_to_le32 (info2);
    qh->dev = dev;
	usb_settoggle (dev, usb_pipeendpoint (pipe), !is_input, 1);
	qh_refresh (ehci, qh);
	return qh;
}

/*
 * For control/bulk/interrupt, return QH with these TDs appended.
 * Allocates and initializes the QH if necessary.
 * Returns null if it can't allocate a QH it needs to.
 * If the QH has TDs (urbs) already, that's great.
 */
static struct ehci_qh *qh_append_tds (ehci_t *ehci, struct usb_device *dev, 
        struct list_head *qtd_list, int pipe)
{
	struct ehci_qh		*qh = NULL;
    uint32_t tmp; 


    if(!usb_pipedevice(pipe)){
        qh = ehci->qh[0];    /*Same QH for endpoint 0 IN/OUT*/
        Message("\nQH should be at index 0, QH Address %#lx\n",
                (unsigned long)qh);
    }else{
        if(usb_pipeendpoint(pipe) == 0){
            /*Guessing set address is already done, so changing qh[0]'s 
              address to the new assigned address */ 
            tmp = le32_to_cpu(ehci->qh[0]->hw_info1);
            tmp = tmp|(usb_pipedevice(pipe));
            ehci->qh[0]->hw_info1 = cpu_to_le32(tmp);
            qh = ehci->qh[0];
            Message("\nQH should be at index %ld, QH ADDR %#lx\n",
               (unsigned long)0, (unsigned long)qh);
        }else if(usb_pipeendpoint(pipe) != 0){
           	qh = ehci->qh[((usb_pipeendpoint(pipe)<<1) | (usb_pipeout(pipe))) + (dev->portnr - 1)];
            Message("\nQH should be at index %ld, QH ADDR %#lx\n",
               (unsigned long)((usb_pipeendpoint(pipe)<<1)|(usb_pipeout(pipe))+(dev->portnr - 1)),\
               (unsigned long)qh); 
        }
    }
	if (qh == NULL){
		/* can't sleep here, we have ehci->lock... */
        Message("Allocating a New QH");
   		qh = qh_make (ehci, dev, pipe);
        if(!qh)
            return NULL;

        if(usb_pipeendpoint(pipe) == 0){
            ehci->qh[1] = ehci->qh[0] = qh;
        }else{
            ehci->qh[((usb_pipeendpoint(pipe)<<1) | (usb_pipeout(pipe))) + (dev->portnr - 1)] = qh;
        }

        Message("\nGot the NEW QH @ %#lx\n",(unsigned long)qh);
	}
    debug_qh = qh;
	if ((qh != NULL)) {
		struct ehci_qtd	*qtd;

		if (list_empty (qtd_list))
			qtd = NULL;
		else
			qtd = list_entry (qtd_list->next, struct ehci_qtd, qtd_list);
        
		/* control qh may need patching ... */
		if ((usb_pipeendpoint(pipe)== 0)) {
                        /* usb_reset_device() briefly reverts to address 0 */
            if (usb_pipedevice (pipe) == 0)
                qh->hw_info1 &= ~QH_ADDR_MASK;
		}

		/* just one way to queue requests: swap with the dummy qtd.
		 * only hc or qh_refresh() ever modify the overlay.
		 */
		if ((qtd != NULL)) {
			struct ehci_qtd		*dummy;
			unsigned long dma;
			unsigned int			token;

			/* to avoid racing the HC, use the dummy td instead of
			 * the first td of our list (becomes new dummy).  both
			 * tds stay deactivated until we're done, when the
			 * HC is allowed to fetch the old dummy (4.10.2).
			 */
			token = qtd->hw_token;
			qtd->hw_token = HALT_BIT;
			wmb ();
			dummy = qh->dummy;

			dma = dummy->qtd_dma;
			*dummy = *qtd;
			dummy->qtd_dma = dma;

            qtd_being_used[0] = dummy;

			list_del (&qtd->qtd_list);
			list_add (&dummy->qtd_list, qtd_list);
            Message("\nSubmiitin Request @ current dummy %#lx\n", (unsigned long)dummy); 

#ifdef XLS_USB_DEBUG
            /*Dump QTDs*/
            /*TODO*/
//           dump_qtd_list(qtd_list);
#endif

            /*Adding new request QTD list to this qh->qtd_list. This will be
             * useful at the time of completion.*/
			__list_splice (qtd_list, qh->qtd_list.prev);

			ehci_qtd_init (qtd, qtd->qtd_dma);
			qh->dummy = qtd;

			/* hc must see the new dummy at list end */
			dma = qtd->qtd_dma;
			qtd = list_entry (qh->qtd_list.prev,
					struct ehci_qtd, qtd_list);
			qtd->hw_next = QTD_NEXT (dma);

			/* let the hc process these next qtds */
			wmb ();
			dummy->hw_token = token;
            Message("\nNew DUMMY @ %#lx\n",(unsigned long)qh->dummy);
		}else{
            ErrorMsg("\nNO QTDS ???\n");
        }
	}else{
        ErrorMsg("\nQH IS NULL\n");
    }
	return qh;
}
/* move qh (and its qtds) onto async queue; maybe enable queue.  */

static void qh_link_async (ehci_t *ehci, struct ehci_qh *qh)
{
	unsigned int		dma = QH_NEXT (qh->qh_dma);
	struct ehci_qh	*head;

	/* (re)start the async schedule? */
	head = ehci->async;

	if (!head->qh_next.qh) {
		u32	cmd = xls_readl (&ehci->regs->command);

		if (!(cmd & CMD_ASE)) {
			/* in case a clear of CMD_ASE didn't take yet */
			(void) handshake (&ehci->regs->status, STS_ASS, 0, 150);
			cmd |= CMD_ASE | CMD_RUN;
			xls_writel (cmd, &ehci->regs->command);
			/* posted write need not be known to HC yet ... */
		}
	}

	/* clear halt and/or toggle; and maybe recover from silicon quirk */
	if (qh->qh_state == QH_STATE_IDLE)
		qh_refresh (ehci, qh);

	/* splice right after start */
	qh->qh_next = head->qh_next;
	qh->hw_next = head->hw_next;
	wmb ();

	head->qh_next.qh = qh;
	head->hw_next = dma;

	qh->qh_state = QH_STATE_LINKED;
	/* qtd completions reported later by interrupt */
}


static int
submit_async (struct ehci_hcd *ehci, struct list_head *qtd_list, struct usb_device *dev, int pipe)
{
	struct ehci_qtd		*qtd;
	struct ehci_qh		*qh = NULL;
	int			rc = 0;

	Message("Submit Async Called...");
	qtd = list_entry (qtd_list->next, struct ehci_qtd, qtd_list);

#ifdef XLS_USB_DEBUG
    if(xls_readl(&ehci->regs->status) & (STS_INT | STS_ERR)){
        ErrorMsg("INT/ERR BIT IS ALREADY SET !!!\n");
        while(1);
    }
#endif

	qh = qh_append_tds (ehci, dev, qtd_list, pipe);
	if (qh == NULL) {
		rc = -1;
		goto done;
	}

	/* Control/bulk operations through TTs don't need scheduling,
	 * the HC and TT handle it when the TT has a buffer ready.
	 */
	if (qh->qh_state == QH_STATE_IDLE)
		qh_link_async (ehci, qh);
 done:
	return rc;
}

void dump_qh(struct ehci_qh *qh)
{
    printf("HW NEXT %#x\n",le32_to_cpu(qh->hw_next));
    printf("HW_INFO1 %#x",le32_to_cpu(qh->hw_info1));
    printf("\nHW INFO 2 %#x",le32_to_cpu(qh->hw_info2));
    printf("\nHW CURRENT %#x",le32_to_cpu(qh->hw_current));
    printf("\nHW QTD NEXT %#x",le32_to_cpu(qh->hw_qtd_next));
    printf("\nHW ALT NEXT %#x",le32_to_cpu(qh->hw_alt_next));
    printf("\nHW TOKEN %#x",le32_to_cpu(qh->hw_token));
    printf("\nHW BUF0 [%#x]",le32_to_cpu(qh->hw_buf[0]));
    printf("\nHW BUF1 [%#x]",le32_to_cpu(qh->hw_buf[1]));
    printf("\nHWBUF2 [%#x]",le32_to_cpu(qh->hw_buf[2]));
    printf("\nHWBUF3 [%#x]",le32_to_cpu(qh->hw_buf[3]));
    printf("\nHWBUF4 [%#x]",le32_to_cpu(qh->hw_buf[4]));
    return;
}

void dump_qtd(struct ehci_qtd *qtd)
{
    printf("\nQTD @ %#lx\n",(unsigned long)qtd);
    printf("\nQTD HW NEXT %#x", le32_to_cpu(qtd->hw_next));
    printf("\nHW ALT NEXT %#x", cpu_to_le32(qtd->hw_alt_next));
    printf("\nHW TOKEN %#x", cpu_to_le32(qtd->hw_token));
    printf("\nHW BUF0 %#x",cpu_to_le32(qtd->hw_buf[0]));
    printf("\nHW BUF1 %#x",cpu_to_le32(qtd->hw_buf[1]));
    printf("\nHW BUF2 %#x",cpu_to_le32(qtd->hw_buf[2]));
    printf("\nHW BUF3 %#x",cpu_to_le32(qtd->hw_buf[3]));
    printf("\nHW BUF4 %#x",cpu_to_le32(qtd->hw_buf[4]));
}

void dump_qh_qtd(struct ehci_qh *qh)
{
    struct ehci_qtd *qtd;
    dump_qh(qh);
    printf("\nNEXT QTD\n");
    qtd = phys_to_virt(((unsigned long)le32_to_cpu(qh->hw_qtd_next) &
                        ~(0x1f)));
    dump_qtd(qtd);
    printf("\nCURRENT QTD\n");
    qtd = phys_to_virt(((unsigned long)le32_to_cpu(qh->hw_current) &
                        ~(0x1f)));
    dump_qtd(qtd);
}

static int wait_for_intr(ehci_t *ehci)
{
    volatile int status=0;
    volatile unsigned long x=0;
    /*Wait for transaction complete interrupt*/

    while(1){
        status = xls_readl (&ehci->regs->status);
        if(status & STS_ERR){
            //Should be ErrorMsg
            Message("Transaction Completed With an Error.");
            xls_writel(STS_ERR|STS_INT,&ehci->regs->status);
            return -1;
        }else if(status & STS_INT){
            Message("Transaction Completed Successfully");
            xls_writel(STS_INT|STS_ERR,&ehci->regs->status);
            return 0;
        }
        x++;
        /*Few devices are damn slow.. give them enough time*/
        if(x > 0xf000000UL){
            printf("Transaction timed out !!");
            for( x =0; x<submitted_qtd_counter; x++){
                printf("\nDUMPIN QTD %ld\n",x);
                dump_qtd(qtd_being_used[x]);
            }
            dump_qh_qtd(ehci->async->qh_next.qh);
            return -ETIMEDOUT;
        }
    }
    return 0;
}

int hc_interrupt(ehci_t *ehci, struct usb_device *dev)
{
    struct list_head    *entry, *tmp;
    struct ehci_qh *qh = NULL;
    struct ehci_qtd *qtd =  NULL; 
    volatile unsigned int token = 0;
    int ret = 0;
    unsigned int xfr_remaining=0;
    int sanity_flag = 0;
    volatile int wait = 0;
    volatile int cnt=0;
    volatile int xfr_status;

    xfr_status = wait_for_intr(ehci);
    if(xfr_status == -ETIMEDOUT){
        /*TODO: ERROR_CASE_1
         Handle this special case, where device has not served the request,
         Active bit of this QTD's will be set.
         So, Do the following,
            -Disable Async Transactions.
            -Delete All QTD's associated with this QH and Free the QH
            -Write ehci->async value in ASYNCADDR register
            -Enable Async Transactions.
         */
        return -1;
    }else if(xfr_status == -1){
        /*Xfr Completed with an Error*/
    }else if(xfr_status == 0){
        /*Xfr Completed without an Error*/
    }

    /*If timeout or any error happens, release this QH and all QTD's linked
      with this QH head. It will be reallocated if new request comes for the 
      same device and for same endpoint.
      -Return Error -ENODEV or something which would force
       usbcore to check the port change status.
      -If port connection change is detected than call appropriate callback
       of application driver to notify it and free the resources.
      -Also call ehci free routine for this device to free the remaining
       qh/qtds for this device.
      -If connection change is not detected than just return error to the 
       application driver. Let it do the necessary stuffs.
     */ 
    
    /*Ok.. Now traverse all QH and free all processed QTDs*/
    qh = ehci->async->qh_next.qh;

    if(qh!=NULL){
        do{
            if (!list_empty (&qh->qtd_list)){
                if(sanity_flag){
                    ErrorMsg("Something went wrong");
                    while(1);
                }
                sanity_flag = 1;
                /*Loop over all qtds*/  
                list_for_each_safe (entry, tmp, &qh->qtd_list){

                    qtd = list_entry (entry, struct ehci_qtd, qtd_list);
try_again:
                    token = cpu_to_le32(qtd->hw_token);
                    /*Check the status*/
                    if(token & QTD_STS_ACTIVE){
                        /*If "wait" is set, than wait for some time
                          for transaction to get complete. We might have come 
                          here due to short pkt, not because of IOC.
                         */
                        if(wait){
                           if(cnt<=10000){
                               mdelay(5);
                               cnt++;
                               goto try_again;
                           } else{
                               wait = 0;
                               goto try_again;
                           }
                        }else{
                            ErrorMsg("Restart USB subsystem.\n");  
                            ErrorMsg("Unknown state.TOKEN %#x\n",token); 
                            dump_qh(qh);
                            dump_qtd(qtd);
                            ErrorMsg("QH NO %d, Manufact %s",
                                    qh->index,qh->dev->mf);
                            ret = -1;
                            qh_unlink_async_disable(dev,qh);
                            goto exit;                     
                            /*TODO:
                              Same as ERROR_CASE_1
                             */
                        }
                    }

                    if(!(token & QTD_STS_ACTIVE)){
                        if(token & QTD_STS_HALT){
                            if(token & QTD_STS_BABBLE){
                                ErrorMsg("Babble Error Detected.");
                            }
                            if(QTD_CERR(token) == 0){
                                //Should be ErrorMsg
                                Message("Halt bit is set because CERR counter\
                                            decremented to zero.");
                            }
                            ret = -1;
                        }
                        if(token & QTD_STS_XACT){
                            //Should be ErrorMsg
                            Message("Transaction Error Bit is Set.");
                            ret = -1;
                        }
                        xfr_remaining = QTD_LENGTH(token);
                        if(xfr_remaining!= 0){
                            if(QTD_PID(token) == 1){
                                /*In Token*/
                                 Message("Couldnt rcv %d bytes out of %d\
                                         bytes",xfr_remaining,qtd->length);
                                if((token & QTD_STS_HALT) || (token &
                                            QTD_STS_XACT)){
                                    wait = 0;
                                }else{
                                    wait = 1;
                                }
                            }else{
                                /*Out/Setup Token*/
                                //Should be ErrorMsg
                                Message("Couldnt xmit %d bytes out of %d\
                                        bytes",xfr_remaining,qtd->length);
                                if((token & QTD_STS_HALT) || (token &
                                            QTD_STS_XACT)){
                                    wait = 0;
                                }else {
                                    wait = 1;
                                }
                            }
                        }else{
                            wait = 0;
                            cnt = 0;
                        }
                        list_del(&qtd->qtd_list);
                        ehci_qtd_free(ehci,qtd);
                        if(ret == -1)
                            break;
                    }
                }
            }
            if(ret == -1){
                /*Just go on and free all QTDs, Assume that device
                 *is removed so deactive all remaining qtds and free them.
                 */
                qh_unlink_async_disable(dev,qh);
                break;
            }
            if(!list_empty(&qh->qtd_list)){
                ErrorMsg("@#@#@#@#@#@#@#");
                ErrorMsg("QH SHD HV BEEN EMPTY BY NOW");
                ErrorMsg("@#@#@#@#@#@#@#");
                ret = -1;
                goto exit;
            }
            qh = qh->qh_next.qh;
        }while(qh != NULL);
    }
exit:
    /*Ack the interrupt, Earlier ACK could be due to short pkt rcv*/
    xls_writel(STS_INT|STS_ERR,&ehci->regs->status);
    Message("Returnin");
    return ret;
}

/* common code for handling submit messages - used for all but root hub */
/* accesses. */
int submit_common_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
                int transfer_len, struct devrequest *setup, int interval)
{
    struct list_head    qtd_list;
    ehci_t *ehci = &gehci;
    int ret = 0;
    int stat = 0;
    int timeout;
#ifdef XLS_USB_DEBUG
    if(xls_readl(&ehci->regs->status) & (STS_INT | STS_ERR)){
        ErrorMsg("INT/ERR BIT IS ALREADY SET");
        while(1);
    }
#endif
    submitted_qtd_counter = 0;
    INIT_LIST_HEAD (&qtd_list);
    //dev->request_len = transfer_len;
    switch (usb_pipetype (pipe)) {
        case PIPE_CONTROL:
            Message("\nGot the %s request for device\
                    %ld\n",usb_pipetype(pipe)==PIPE_CONTROL?"Control":"Bulk",\
                    usb_pipedevice(pipe));
            if (!qh_urb_transaction (dev, ehci, setup, 
                        (void *)virt_to_phys(buffer), transfer_len, 
                        &qtd_list,pipe))
                return -1;
            Message("ALL QTD FORMED");
#ifdef XLS_USB_DEBUG
            if(xls_readl(&ehci->regs->status) & (STS_INT | STS_ERR)){
                ErrorMsg("INT/ERR BIT IS ALREADY SET");
                while(1);
            }
#endif
            ret = submit_async (ehci, &qtd_list, dev, pipe);
            Message("\nASYNC REQUEST SUBMITTED !!!\n");
            break;
        case PIPE_BULK:
            Message("\nGot the BULK request for device %ld\n", \
                        usb_pipedevice(pipe));
            if (!qh_urb_transaction (dev, ehci, setup, 
                        (void *)virt_to_phys(buffer), transfer_len, 
                        &qtd_list,pipe))
                return -1;
            Message("ALL QTD FORMED");
#ifdef XLS_USB_DEBUG
            if(xls_readl(&ehci->regs->status) & (STS_INT | STS_ERR)){
                ErrorMsg("INT/ERR BIT IS ALREADY SET");
                while(1);
            }
#endif
            ret = submit_async (ehci, &qtd_list, dev, pipe);
            Message("\nASYNC REQUEST SUBMITTED !!!\n");
            break;
        case PIPE_ISOCHRONOUS:
            ErrorMsg("\nIsochronous Request Not Supported\n");
            return -1;
        case PIPE_INTERRUPT:
            ErrorMsg("\nInterrupt Request Not Supported\n");
            return -1;
    }
    /*Ok.. We have submitted the request now wait for the response. And free 
     *the qtds. */

    /* allow more time for a BULK device to react - some are slow */
#define BULK_TO  5000   /* timeout in milliseconds */
    if (usb_pipetype (pipe) == PIPE_BULK)
        timeout = BULK_TO;
    else
        timeout = 100;
    /*
     * PR 56074,728570:
     * Depending on the memory speed, we need to wait more time
     * for USB commands to complete.
     * WA is running DDR @ 266 MHz, which is the slowest amongst
     * DCF TORs. Hence fine tuning the timeout value such that
     * it works for WA, which essentially makes it work with 
     * all DCF TORs.
     */
    timeout *=6;

    /* wait for it to complete */
	for (;;) {
		/* check whether the controller is done */
		ret = stat = hc_interrupt(ehci,dev);
		if (stat < 0) {
			stat = USB_ST_CRC_ERR;
			break;
		}
		if (stat >= 0 && stat != 0xff) {
			/* 0xff is returned for an SF-interrupt */
			break;
		}
		if (--timeout) {
			udelay(250); /* wait_ms(1); */
		} else {
			printf("CTL:TIMEOUT ");
			stat = USB_ST_CRC_ERR;
			break;
		}
	}
    dev->status = stat;
    dev->act_len = transfer_len;
    return ret;
}

static int ehci_submit_rh_msg(struct usb_device *dev, unsigned long pipe,
		void *buffer, int transfer_len, struct devrequest *cmd)
{
	void * data = buffer;
	int leni = transfer_len;
	int len = 0;
	int stat = 0;
    int retval;
    __u32 temp;
	__u32 datab[4];
	__u8 *data_buf = (__u8 *)datab;
	__u16 bmRType_bReq;
	__u16 wValue;
	__u16 wIndex;
	__u16 wLength;
    int ports;
    ehci_t *ehci = &gehci;
    uint32_t status;

#ifdef DEBUG
urb_priv.actual_length = 0;
pkt_printf(dev, pipe, buffer, transfer_len, cmd, "SUB(rh)", usb_pipein(pipe));
#else
	wait_ms(1);
#endif
    Message("Pipe %#lx",pipe);
	if ((usb_pipetype(pipe) & PIPE_INTERRUPT) == PIPE_INTERRUPT) {
		Message("Root-Hub submit IRQ: NOT implemented");
		return 0;
	}

	bmRType_bReq  = cmd->requesttype | (cmd->request << 8);
	wValue	      = swap_16 (cmd->value);
	wIndex	      = swap_16 (cmd->index);
	wLength	      = swap_16 (cmd->length);
    ports         = HCS_N_PORTS (ehci->hcs_params);

	Message("Root-Hub: adr: %2x cmd(%1x): %08x %04x %04x %04x",
		dev->devnum, 8, bmRType_bReq, wValue, wIndex, wLength);

	switch (bmRType_bReq) {
	/* Request Destination:
	   without flags: Device,
	   RH_INTERFACE: interface,
	   RH_ENDPOINT: endpoint,
	   RH_CLASS means HUB here,
	   RH_OTHER | RH_CLASS	almost ever means HUB_PORT here
	*/
	case RH_GET_STATUS | RH_CLASS:  
            /*Get the HUB Status*/
            memset(data_buf,0,4);
			OK (4);
    case RH_GET_STATUS | RH_OTHER | RH_CLASS:   
            /*Get the Specified Port Status of this HUB.*/
            if (!wIndex || wIndex > ports){
                ErrorMsg("Invalid Port");
                OK(0);
            }
		    wIndex--;
    		status = 0;
	    	temp = xls_readl(&ehci->regs->port_status [wIndex]);

            if(temp & 0x1){
                Message("Port Connected");
            }
		    // wPortChange bits
    		if (temp & PORT_CSC){
	    		status |= 1 << USB_PORT_FEAT_C_CONNECTION;
            }
		    if (temp & PORT_PEC){
			    status |= 1 << USB_PORT_FEAT_C_ENABLE;
            }
    		if (temp & PORT_OCC){
	    		status |= 1 << USB_PORT_FEAT_C_OVER_CURRENT;
            }

            /* whoever resumes must GetPortStatus to complete it!! */
            if ((temp & PORT_RESUME)){
                mdelay(50);
                status |= 1 << USB_PORT_FEAT_C_SUSPEND;

                /* stop resume signaling */
                temp = xls_readl (&ehci->regs->port_status [wIndex]);
                xls_writel (temp & ~(PORT_RWC_BITS | PORT_RESUME),
                        &ehci->regs->port_status [wIndex]);
                retval = handshake (
                        &ehci->regs->port_status [wIndex],
                        PORT_RESUME, 0, 2000 /* 2msec */);
                if (retval != 0) {
                    ErrorMsg("port %d resume error %d\n",
                            wIndex + 1, retval);
                    OK(0);
                }
                temp &= ~(PORT_SUSPEND|PORT_RESUME|(3<<10));
            }

            /* whoever resets must GetPortStatus to complete it!! */
            if ((temp & PORT_RESET)){
                mdelay(50);
                status |= 1 << USB_PORT_FEAT_C_RESET;

                /* force reset to complete */
                xls_writel (temp & ~(PORT_RWC_BITS | PORT_RESET),
                        &ehci->regs->port_status [wIndex]);
                /* REVISIT:  some hardware needs 550+ usec to clear
                 * this bit; seems too long to spin routinely...
                 */
                retval = handshake (
                        &ehci->regs->port_status [wIndex],
                        PORT_RESET, 0, 750);
                if (retval != 0) {
                    ErrorMsg("port %d reset error %d\n",
                            wIndex + 1, retval);
                    OK(0);
                }

                /* see what we found out */
                mdelay(50);
                temp = xls_readl (&ehci->regs->port_status [wIndex]);
            }

		// don't show wPortStatus if it's owned by a companion hc
		if (!(temp & PORT_OWNER)) {
			if (temp & PORT_CONNECT) {
				status |= 1 << USB_PORT_FEAT_CONNECTION;
				// status may be from integrated TT
				status |= ehci_port_speed(ehci, temp);
			}
			if (temp & PORT_PE)
				status |= 1 << USB_PORT_FEAT_ENABLE;
			if (temp & (PORT_SUSPEND|PORT_RESUME))
				status |= 1 << USB_PORT_FEAT_SUSPEND;
			if (temp & PORT_OC)
				status |= 1 << USB_PORT_FEAT_OVER_CURRENT;
			if (temp & PORT_RESET)
				status |= 1 << USB_PORT_FEAT_RESET;
			if (temp & PORT_POWER)
				status |= 1 << USB_PORT_FEAT_POWER;
		}

		// we "know" this alignment is good, caller used kmalloc()...
		*((unsigned int *) data_buf) = cpu_to_le32 (status);
        OK (4);
#if 0	
	case RH_CLEAR_FEATURE | RH_ENDPOINT:
		switch (wValue) {
			case (RH_ENDPOINT_STALL): OK (0);
		}
		break;

	case RH_CLEAR_FEATURE | RH_CLASS:
		switch (wValue) {
			case RH_C_HUB_LOCAL_POWER:
				OK(0);
			case (RH_C_HUB_OVER_CURRENT):
					WR_RH_STAT(RH_HS_OCIC); OK (0);
		}
		break;
#endif

	case RH_CLEAR_FEATURE | RH_OTHER | RH_CLASS:
        /*Port Clear Feature Requests*/
        if (!wIndex || wIndex > ports){
            ErrorMsg("Invalid Port ID %d",wIndex);
            break;
        }
        wIndex--;
        temp = xls_readl (&ehci->regs->port_status [wIndex]);
        if (temp & PORT_OWNER)
            break;
		switch (wValue) {
			case (RH_PORT_ENABLE):
                    xls_writel(temp & ~PORT_PE, &ehci->regs->port_status[wIndex]);
                    OK (0);
#if 0
			case (RH_PORT_SUSPEND):
					WR_RH_PORTSTAT (RH_PS_POCI); 
                    OK (0);
			case (RH_PORT_POWER):
					WR_RH_PORTSTAT (RH_PS_LSDA); 
                    OK (0);
#endif
			case (RH_C_PORT_CONNECTION):
                    /*Clear the PortConnection Change Bit*/
//					WR_RH_PORTSTAT (RH_PS_CSC ); OK (0);
                    xls_writel((temp & ~PORT_RWC_BITS) | PORT_CSC,
                            &ehci->regs->port_status[wIndex]);
                    OK(0);
#if 0
			case (RH_C_PORT_ENABLE):
					WR_RH_PORTSTAT (RH_PS_PESC); 
                    OK (0);
			case (RH_C_PORT_SUSPEND):
					WR_RH_PORTSTAT (RH_PS_PSSC); 
                    OK (0);
			case (RH_C_PORT_OVER_CURRENT):
					WR_RH_PORTSTAT (RH_PS_OCIC); 
                    OK (0);
#endif
			case (RH_C_PORT_RESET):
//					WR_RH_PORTSTAT (RH_PS_PRSC); 
                    /* GetPortStatus clears reset */
                    OK (0);
            default:
                    ErrorMsg("unsupported ClearFeature Requested %#x",wValue);
                    break;
		}
		break;

	case RH_SET_FEATURE | RH_OTHER | RH_CLASS:
        /*Set the Featur of the wIndex-1 port. This is hub class specific
         * request.*/
        if (!wIndex || wIndex > ports){
            ErrorMsg("Wrong wIndex %d",wIndex);
            break;
        }
        wIndex--;
        temp = xls_readl(&ehci->regs->port_status[wIndex]);

        if (temp & PORT_OWNER)
            break;
        temp &= ~PORT_RWC_BITS;

		switch (wValue) {
            /*Parsee the Feature*/
			case (RH_PORT_SUSPEND): 
                /*USB_PORT_FEAT_SUSPEND*/
                if ((temp & PORT_PE) == 0
                        || (temp & PORT_RESET) != 0){
                    ErrorMsg("Error in RH_PORT_SUSPEND");
                    break;
                }
                xls_writel (temp | PORT_SUSPEND,&ehci->regs->port_status[wIndex]);
                OK (0);

			case (RH_PORT_RESET): /* BUG IN HUP CODE *********/
                /*USB_PORT_FEAT_RESET:*/
    			if (temp & PORT_RESUME){
                    ErrorMsg("Port Resume is Set.");
                    break;
                }
	    		/* line status bits may report this as low speed,
	    		 * which can be fine if this root hub has a
	    		 * transaction translator built in.
	    		 */
        		Message("ehci port %d reset\n", wIndex + 1);
    			temp |= PORT_RESET;
    			temp &= ~PORT_PE;
    			/*
    			 * caller must wait, then call GetPortStatus
    			 * usb 2.0 spec says 50 ms resets on root
    			 */
	    		xls_writel(temp, &ehci->regs->port_status [wIndex]);
                mdelay(50);
				OK (0);
			case (RH_PORT_POWER):   
                /*USB_PORT_FEAT_POWER*/
                if (HCS_PPC (ehci->hcs_params))
                   xls_writel (temp | PORT_POWER, &ehci->regs->port_status[wIndex]);
                   OK (0);
			case (RH_PORT_ENABLE): /* BUG IN HUP CODE *********/
                    /*USB_PORT_FEAT_ENABLE*/
					OK (0);
		}
        Message("HERE .. with wValue %d, wIndex %d",wValue, wIndex);

		break;

	case RH_SET_ADDRESS: gehci.rh.devnum = wValue; OK(0);

	case RH_GET_DESCRIPTOR:
		switch ((wValue & 0xff00) >> 8) {
			case (0x01): /* device descriptor */
                Message("Get The Device Descriptor");
				len = min_t(unsigned int,
					  leni,
					  min_t(unsigned int,
					      sizeof (usb2_rh_dev_descriptor),
					      wLength));
				data_buf = usb2_rh_dev_descriptor; OK(len);
			case (0x02): /* configuration descriptor */
                Message("Get The Config Descriptor");
				len = min_t(unsigned int,
					  leni,
					  min_t(unsigned int,
					      sizeof (hs_rh_config_descriptor),
					      wLength));
				data_buf = hs_rh_config_descriptor; OK(len);
			case (0x03): /* string descriptors */
                Message("Get The String Descriptor, wvalue %#x",wValue);

                if((wValue&0xff) == 0){
                    len = min_t(unsigned int,
						  leni,
						  min_t(unsigned int,
						      sizeof (root_hub_str_index0),
						      wLength));
					data_buf = root_hub_str_index0;
					OK(len);
                }else if((wValue&0xff) == 1){
                    len = min_t(unsigned int, leni, min_t(unsigned int,
                                sizeof (root_hub_str_index1),
                                wLength));
					data_buf = root_hub_str_index1;
					OK(len);
                }else if((wValue&0xff) == 2){
                 len = min_t(unsigned int,
						  leni,
						  min_t(unsigned int,
						      sizeof (root_hub_str_index2),
						      wLength));
					data_buf = root_hub_str_index2;
					OK(len);
                }else if((wValue&0xff) == 3){
                    len = min_t(unsigned int,
						  leni,
						  min_t(unsigned int,
						      sizeof (root_hub_str_index3),
						      wLength));
					data_buf = root_hub_str_index3;
					OK(len);
                }
				OK(len);
			default:
				stat = USB_ST_STALLED;
		}
		break;
	case RH_GET_DESCRIPTOR | RH_CLASS:
	    {
            ehci_hub_descriptor(&gehci,(struct usb_hub_descriptor *)data_buf);
		    len = min_t(unsigned int, leni,
			      min_t(unsigned int, data_buf [0], wLength));
		    OK (len);
		}
	case RH_SET_CONFIGURATION:	OK (0);
	default:
		ErrorMsg("unsupported root hub command");
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

int submit_control_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
		int transfer_len, struct devrequest *setup)
{
    Message("Device ID %#lx",((pipe>>8) & 0x7f));

    /*Check if this is for root hub*/
    if (((pipe >> 8) & 0x7f) == gehci.rh.devnum) {
        gehci.rh.dev = dev;
        /* root hub - redirect */
        Message("Ok... This is for root hub");
        return ehci_submit_rh_msg(dev, pipe, buffer, transfer_len, setup);
    }
    
    Message("Msg for device ..");
    return submit_common_msg(dev, pipe, buffer, transfer_len, setup, 0);
}

int submit_int_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
		int transfer_len, int interval)
{
    ErrorMsg("Interrupt Transactions Not Supported Yet.");
	return -1;
}


/*
 * handshake - spin reading hc until handshake completes or fails
 * @ptr: address of hc register to be read
 * @mask: bits to look at in result of read
 * @done: value of those bits when handshake succeeds
 * @usec: timeout in microseconds
 *
 * Returns negative errno, or zero on success
 *
 * Success happens when the "mask" bits have the specified value (hardware
 * handshake done).  There are two failure modes:  "usec" have passed (major
 * hardware flakeout), or the register reads as all-ones (hardware removed).
 *
 * That last failure should_only happen in cases like physical cardbus eject
 * before driver shutdown. But it also seems to be caused by bugs in cardbus
 * bridge shutdown:  shutting down the bridge before the devices using it.
 */
static int handshake (void *ptr, unsigned int mask, unsigned int done, int usec)
{
	unsigned int	result;

	do {
		result = xls_readl (ptr);
		if (result == ~(unsigned int)0)		/* card removed */
			return -2;
		result &= mask;
		if (result == done)
			return 0;
		udelay (1);
		usec--;
	} while (usec > 0);
	return -1;
}

/* force HC to halt state from unknown (EHCI spec section 2.3) */
static int ehci_halt (ehci_t *ehci)
{
    unsigned int temp = xls_readl (&ehci->regs->status);

	Message("Status Value %#x, Addr %#lx",temp,(unsigned long)&ehci->regs->status);
    /* disable any irqs left enabled by previous code */
    xls_writel (0, &ehci->regs->intr_enable);

	Message("value of intr_enable %#x",xls_readl(&ehci->regs->intr_enable));

    if ((temp & STS_HALT) != 0){
		Message("EHCI is already in halt state.");
		temp = xls_readl(&ehci->regs->command);
		Message("Current value of cmd %#x",temp);
        return 0;
	}
    temp = xls_readl(&ehci->regs->command);
    temp &= ~CMD_RUN;
    xls_writel (temp, &ehci->regs->command);
	Message("value of regs_command %#x",xls_readl(&ehci->regs->command));
    return handshake (&ehci->regs->status, STS_HALT, STS_HALT, 16 * 125);
}



int ehci_init(ehci_t *ehci)
{   
    uint32_t hcc_params;
    /*Allocate QH and QTD Pool*/
    if(ehci_mem_init(ehci)){
        ErrorMsg("USB EHCI MEM INIT FAILED.");
        return -1;
    }
    /* controllers may cache some of the periodic schedule ... */
    hcc_params = xls_readl(&ehci->caps->hcc_params);

    ehci->reclaim = NULL;
    ehci->reclaim_ready = 0;

    /*
     * dedicate a qh for the async ring head, since we couldn't
     * unlink
     * a 'real' qh without stopping the async schedule [4.8].
     * use it
     * as the 'reclamation list head' too.
     * its dummy is used in hw_alt_next of many tds,
     * to prevent the qh
     * from automatically advancing to the next
     * td after short reads.
     */
    ehci->async->qh_next.qh = NULL;

    /*Point to Self. We are alone in this bloody QH world.*/
    ehci->async->hw_next = QH_NEXT(ehci->async->qh_dma); 
    ehci->async->hw_info1 = cpu_to_le32(QH_HEAD);           

    /*Setting Qh->Qtd->hw_token in halt state - We are not going to add any
     * transactions in this QH.*/
    ehci->async->hw_token = cpu_to_le32(QTD_STS_HALT); 
    
    /*No QTD List*/
    ehci->async->hw_qtd_next = EHCI_LIST_END; 
    
    /*Point to dummy QTD*/
    ehci->async->hw_alt_next = QTD_NEXT(ehci->async->dummy->qtd_dma); 

    /*Ok... mark that we are ready to become visible to HC*/
    ehci->async->qh_state = QH_STATE_LINKED;               

    /*Setting the interrupt rate to 1 microframe.*/
    ehci->command = 1 << 16;
        
    return 0;
}

/* reset a non-running (STS_HALT == 1) controller */
static int ehci_reset (struct ehci_hcd *ehci)
{
    int retval;
    unsigned int command = xls_readl (&ehci->regs->command);

    command |= CMD_RESET;
    xls_writel (command, &ehci->regs->command);
    retval = handshake (&ehci->regs->command,
            CMD_RESET, 0, 250 * 1000);

    return retval;
}


int ehci_run(ehci_t *ehci)
{
    uint32_t hcc_params;
    uint32_t temp;

    if(ehci_reset(ehci)){
        ErrorMsg("Ehci Reset Failed.");
       // return -1;
    }

    /*Lets make asyn Q visible to HC, though we have not enabled async schedule
     * in COMMAND register*/
    
    xls_writel((unsigned int)ehci->async->qh_dma, &ehci->regs->async_next);
    hcc_params = xls_readl(&ehci->caps->hcc_params);

    if (HCC_64BIT_ADDR(hcc_params)) {
        xls_writel(0, &ehci->regs->segment);
        printf(" Oh !!! We Support 64bit Addr ?? No more ... hmmm...");
    }

    // Philips, Intel, and maybe others need CMD_RUN before the
    // root hub will detect new devices (why?); NEC doesn't
    ehci->command &= ~(CMD_LRESET|CMD_IAAD|CMD_PSE|CMD_ASE|CMD_RESET);
    ehci->command |= CMD_RUN;
    xls_writel(ehci->command, &ehci->regs->command);

    /*Let EHCI be the Owner of all ports*/
    xls_writel (FLAG_CF, &ehci->regs->configured_flag);
    xls_readl (&ehci->regs->command);   /* unblock posted writes */
    
    temp = HC_VERSION(xls_readl (&ehci->caps->hc_capbase));
    Message("USB %x.%x started, EHCI %x.%02x\n", 2, 0, temp >> 8,
             temp & 0xff);
    
    /* Turn On Interrupts, though it is still disabled in pic. */
    xls_writel (INTR_MASK, &ehci->regs->intr_enable); 

    return 0;
}



int ehci_setup(ehci_t *ehci)
{
	ehci->caps = (struct ehci_caps *)ehci->rsrc_start;
	Message("Readin @ %#lx",(unsigned long)&ehci->caps->hc_capbase);
	ehci->regs = (struct ehci_regs *)(ehci->rsrc_start + HC_LENGTH(xls_readl(&ehci->caps->hc_capbase)));
	Message("gehci.caps @ %#lx",(unsigned long)ehci->caps);
	Message("gehci.regs @ %#lx",(unsigned long)ehci->regs);

	/* cache this readonly data; minimize chip reads */
	ehci->hcs_params = xls_readl(&ehci->caps->hcs_params);

    /*We dont know ehci state, put it in halt state.*/
	if(ehci_halt(ehci)){
		ErrorMsg("couldnt halt ehci");
		return -1;
	}

	if(ehci_init(ehci)){
		ErrorMsg("ehci_init failed.");
		return -1;
	}

	return 0;
}

/* Default to Host Mode */
int flag_usb_dev_mode = 0;

int usb_lowlevel_init(void)
{
    /*Glue Logic*/
    /*0xbef00000 + 0x25000*/
	phoenix_reg_t *condor_usb_ctrl_mmio = (phoenix_reg_t *)USB_GLUE_START;

	Message("USB Board-specific initialization.");

	/* The RMI-Specific USB Block */
	cpu_write_reg(condor_usb_ctrl_mmio, 49, 0x10000000);    /* Clear Rogue Phy INTs */
	cpu_write_reg(condor_usb_ctrl_mmio, 50, 0x1f000000);
	cpu_write_reg(condor_usb_ctrl_mmio,  1, 0x07000500);

    {
        phoenix_reg_t *gpio_mmio = phoenix_io_mmio(0x18000);
        volatile unsigned int value = gpio_mmio[21];
        if ((value >> 22) & 0x01) {
            Message("Detected USB Host mode..\n");
            cpu_write_reg(condor_usb_ctrl_mmio,  0, 0x02000000);
        }
        else {
            Message("Detected USB Device mode..\n");
            cpu_write_reg(condor_usb_ctrl_mmio,  0, 0x01000000);
            flag_usb_dev_mode = 0x01;
        }
    }

	Message("Programmed USB IntrEnable/PhyReset/Reset Registers.");

	cpu_write_reg(condor_usb_ctrl_mmio, 16, 0x00000000);
	cpu_write_reg(condor_usb_ctrl_mmio, 17, 0xffffffff);
	Message("Programmed USB COHERENT_MEM_BASE/Limit Values.");

    if (flag_usb_dev_mode == 0x01)
        return 0;

	/*Initialize global ehci data structure.*/
	memset(&gehci, 0, sizeof(ehci_t));

	/*Initialize all other global variables here. 
	* We may come here more than once.
	*/
    gehci.rsrc_start = (unsigned long)USB_EHCI_BASE; 
    
	/* hc_reset - Reset EHCI host controller.*/
	ehci_setup(&gehci);
	Message("EHCI Controller Reset Done");	

	/*
	* hc_start - Start EHCI
	*	Initalize EHCI registers, Keep interrupt disable for now.
	*/	
    ehci_run(&gehci);

	/*Debug:
		-Dump EHCI Capability and Controller Operational Registers
	*/
	return 0;
}

int usb_lowlevel_stop(uint32_t port_n)
{
    /*HALT EHCI*/
    printf("%s[%d]\n", __func__, __LINE__);
    ehci_halt(&gehci);
    printf("%s[%d]\n", __func__, __LINE__);
    /*Reset usb bus and put in halt state*/
    ehci_reset(&gehci);
    printf("%s[%d]\n", __func__, __LINE__);
	return 0;
}

int disable_async(ehci_t *ehci)
{
    uint32_t cmd;
    uint32_t status; 
    int ret ;
    status = xls_readl(&ehci->regs->status);
    
    if(!(status & STS_ASS)){
        printf("\nAsync is already disabled.\n");
        return 0;
    }
    cmd = xls_readl (&ehci->regs->command);
    cmd = cmd & ~CMD_ASE;
    /*Disable Async Transactions*/
	xls_writel (cmd, &ehci->regs->command);
    wmb();
    /*Wait till async doesnt stop*/ 
    ret = handshake (&ehci->regs->status, STS_ASS, 0, 10000);
    if(ret){
        ErrorMsg("Couldnt Stop Async Transactions");
    }
    return ret;
}

int enable_async(ehci_t *ehci)
{
    volatile uint32_t cmd;
    volatile uint32_t status;
    volatile uint32_t ret; 

    status = xls_readl(&ehci->regs->status);
    if(status & STS_ASS){
        printf("\nAsync is already enabled\n");
        return 0;
    }
    /*Lets start from the begining.*/
    xls_writel((unsigned int)ehci->async->qh_dma, &ehci->regs->async_next);
    wmb();
    
    /*Enable Async Transactions*/
    cmd = xls_readl (&ehci->regs->command);
    cmd = cmd | CMD_ASE;
	xls_writel (cmd, &ehci->regs->command);
    wmb();
    cmd = xls_readl (&ehci->regs->command);

    ret = handshake (&ehci->regs->status, STS_ASS, STS_ASS, 50000);
    if(ret){
        ErrorMsg("\nCouldnt Enable Async Transactions\n");
    }
    return ret;
}

void qh_unlink_async_disable(struct usb_device *dev, struct ehci_qh *qh)
{
    struct ehci_qh *prev = gehci.async;
    struct ehci_qtd *qtd;
    struct list_head    *entry, *tmp;

    if(disable_async(&gehci)){
        ErrorMsg("Couldnt Disable ASYNC Transaction");
    }
    
    while(prev->qh_next.qh != qh)
        prev = prev->qh_next.qh;

    if(prev->qh_next.qh != qh) 
       /*If not found*/
       return;

    prev->hw_next = qh->hw_next;
    prev->qh_next = qh->qh_next;
    
    if(enable_async(&gehci)){
        ErrorMsg("Couldnt Enable Async Transactions");
    }

    /*Ok.. Now free the pending QTDs of this QH*/
    if (!list_empty (&qh->qtd_list)){
        list_for_each_safe (entry, tmp, &qh->qtd_list){
            qtd = list_entry (entry, struct ehci_qtd, qtd_list);
            list_del(&qtd->qtd_list);
            ehci_qtd_free(&gehci,qtd);
        }
    }
    qh->qh_state = QH_STATE_IDLE;
}

void qh_unlink_async(struct usb_device *dev, struct ehci_qh *qh)
{
    struct ehci_qh *prev = gehci.async;
     
    while(prev->qh_next.qh != qh)
        prev = prev->qh_next.qh;

    prev->hw_next = qh->hw_next;
    prev->qh_next = qh->qh_next;

}
#if 0
void hcd_free_resource_dev(struct usb_device *dev)
{
    int i=0;
    struct ehci_qh *qh;

    /*traverse through ehci->async queue and free all qh and qtds
      linked with this device. Always start with '1' as '0' and
      '1' both points to the same QH.
     */
    for(i=1; i<32; i++){
        qh = dev->qh[i];
        if(qh){
            if(qh->qh_state == QH_STATE_LINKED){
                /*Unlink This QH and Free all QTDs*/
                qh_unlink_async_disable(dev,qh);
            }
            /*Free This QH */
            ehci_qh_free(&gehci,qh);
            dev->qh[i] = NULL;
        }
    }
}
#endif
#endif /* CONFIG_USB_EHCI */
