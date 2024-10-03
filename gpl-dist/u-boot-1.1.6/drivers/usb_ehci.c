/*-
 * Copyright (c) 2007-2012, Juniper Networks, Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#if defined(CONFIG_USB_EHCI)
#if !defined(CONFIG_MARVELL)
#ifdef CONFIG_USB_PCIE
#include <pcie.h>
#endif
#include <pci.h>
#endif
#include <usb.h>
#include <usb_ehci.h>

#if defined(CONFIG_EX2200) || defined(CONFIG_EX3300) 
extern uint32_t mvOsCacheFlush( void* osHandle, void* p, int size );
extern uint32_t mvOsCacheInvalidate( void* osHandle, void* p, int size );
#define DCACHE_FLUSH(addr, size)     mvOsCacheFlush(NULL, addr, size)
#define DCACHE_INVALIDATE(addr, size)     mvOsCacheInvalidate(NULL, addr, size)
#else
#define DCACHE_FLUSH(addr, size) 
#define DCACHE_INVALIDATE(addr, size) 
#endif

#if !defined(CONFIG_MARVELL)
#ifdef CONFIG_USB_PCIE
#define PCI_DEV_T		pcie_dev_t
#define PCI_FIND_DEVICE		pcie_find_device
#define PCI_FIND_DEVICES		pcie_find_devices
#define PCI_READ_CONFIG_DWORD	pcie_read_config_dword
#else
#define PCI_DEV_T		pci_dev_t
#define PCI_FIND_DEVICE		pci_find_device
#define PCI_FIND_DEVICES		pci_find_devices
#define PCI_READ_CONFIG_DWORD	pci_read_config_dword
#endif
#endif

#ifdef EHCI_DEBUG
#define	DBG(format, arg...)	\
	printf("\nEHCI_DEBUG: %s: " format "\n", __func__, ## arg)
#else
#define	DBG(format, arg...)	\
	do {} while(0)
#endif /* EHCI_DEBUG */
extern int board_booting;
void (*board_usb_err_recover)(void) = NULL;

#if defined(CONFIG_MARVELL)
#define rdl(off)	(*((vu_long *)(off)))
#define wrl(off, val)	(*((vu_long *)(off)) = ((vu_long)val))

#define USB_CMD			0x140
#define USB_MODE		0x1a8
#define USB_CAUSE		0x310
#define USB_MASK		0x314
#define USB_WINDOW_CTRL(i)	(0x320 + ((i) << 4))
#define USB_WINDOW_BASE(i)	(0x324 + ((i) << 4))
#define USB_IPG			0x360
#define USB_PHY_PWR_CTRL	0x400
#define USB_PHY_TX_CTRL		0x420
#define USB_PHY_RX_CTRL		0x430
#define USB_PHY_IVREF_CTRL	0x440
#define USB_PHY_TST_GRP_CTRL	0x450
#endif

#if !defined(CONFIG_MARVELL) && !defined(CONFIG_P2020)
static struct pci_device_id ehci_pci_ids[] = {
	{0x1131, 0x1562},
	{0x1033, 0x00E0},
	{0, 0}
};
#endif
static struct {
	uint8_t hub[8];
	uint8_t device[18];
	uint8_t config[9];
	uint8_t interface[9];
	uint8_t endpoint[7];
} descr = {
    {	/* HUB */
	sizeof(descr.hub),	/* bDescLength */
	0x29,		/* bDescriptorType: hub descriptor */
	2,		/* bNrPorts -- runtime modified */
	0, 0,		/* wHubCharacteristics -- runtime modified */
	0xff,		/* bPwrOn2PwrGood */
	0,		/* bHubCntrCurrent */
	0		/* DeviceRemovable XXX at most 7 ports! XXX */
    },
    {	/* DEVICE */
	sizeof(descr.device),	/* bLength */
	1,		/* bDescriptorType: UDESC_DEVICE */
	0x00, 0x02,	/* bcdUSB: v2.0 */
	9,		/* bDeviceClass: UDCLASS_HUB */
	0,		/* bDeviceSubClass: UDSUBCLASS_HUB */
	1,		/* bDeviceProtocol: UDPROTO_HSHUBSTT */
	64,		/* bMaxPacketSize: 64 bytes */
	0x00, 0x00,	/* idVendor */
	0x00, 0x00,	/* idProduct */
	0x00, 0x01,	/* bcdDevice */
	1,		/* iManufacturer */
	2,		/* iProduct */
	0,		/* iSerialNumber */
	1		/* bNumConfigurations: 1 */
    },
    {	/* CONFIG */
	sizeof(descr.config),	/* bLength */
	2,		/* bDescriptorType: UDESC_CONFIG */
	sizeof(descr.config)+sizeof(descr.interface)+sizeof(descr.endpoint), 0,
			/* wTotalLength */
	1,		/* bNumInterface */
	1,		/* bConfigurationValue */
	0,		/* iConfiguration */
	0x40,		/* bmAttributes: UC_SELF_POWERED */
	0		/* bMaxPower */
    },
    {	/* INTERFACE */
	sizeof(descr.interface),	/* bLength */
	4,		/* bDescriptorType: UDESC_INTERFACE */
	0,		/* bInterfaceNumber */
	0,		/* bAlternateSetting */
	1,		/* bNumEndpoints */
	9,		/* bInterfaceClass: UICLASS_HUB */
	0,		/* bInterfaceSubClass: UISUBCLASS_HUB */
	0,		/* bInterfaceProtocol: UIPROTO_HSHUBSTT */
	0		/* iInterface */
    },
    {	/* ENDPOINT */
	sizeof(descr.endpoint),		/* bLength */
	5,		/* bDescriptorType: UDESC_ENDPOINT */
	0x81,		/* bEndpointAddress: UE_DIR_IN | EHCI_INTR_ENDPT */
	3,		/* bmAttributes: UE_INTERRUPT */
	8, 0,		/* wMaxPacketSize */
	255		/* bInterval */
    }
};

#if defined(CONFIG_MARVELL) /* Avoid an "unused valiable" warning */
static uint32_t base;
#endif
static struct ehci_hccr *hccr;	/* R/O registers, not need for volatile */
static volatile struct ehci_hcor *hcor;

#if defined(USB_MULTI_CONTROLLER)
int curr_controller = -1;
uint32_t ehci_base[USB_MAX_ROOT_HUB];
static struct ehci_hccr *ehci_hccr[USB_MAX_ROOT_HUB];
static volatile struct ehci_hcor *ehci_hcor[USB_MAX_ROOT_HUB];
static int rootdev_inited[USB_MAX_ROOT_HUB];
#endif
static struct QH qh_list __attribute__((aligned(32)));
static int rootdev;
static uint16_t portreset;

#ifdef EHCI_DEBUG
static void
dump_data(char *buffer, unsigned int length)
{
	unsigned int l;

	l = 0;
	while (l < length) {
		printf(" %02x", buffer[l]);
		l++;
		if ((l & 15) == 0)
			printf("\n");
	}
	if (l & 15)
		printf("\n");
}

#if !defined(CONFIG_MARVELL)
static void
dump_pci_reg(PCI_DEV_T dev, int ofs)
{
	uint32_t reg;

	PCI_READ_CONFIG_DWORD(dev, ofs, &reg);
	printf("\t0x%02x: %08x\n", ofs, reg);
}

static void
dump_pci(int enh, PCI_DEV_T dev)
{
	int ofs;

	DBG("\n%s", (enh) ? "EHCI" : "OHCI");
	for (ofs = 0; ofs < 0x44; ofs += 4)
		dump_pci_reg(dev, ofs);
	if (enh)
		dump_pci_reg(dev, 0x60);
	dump_pci_reg(dev, 0xdc);
	dump_pci_reg(dev, 0xe0);
	if (enh) {
		dump_pci_reg(dev, 0xe4);
		dump_pci_reg(dev, 0xe8);
	}
}
#endif

static void
dump_regs(void)
{

	DBG("usbcmd=%#x, usbsts=%#x, usbintr=%#x,\n\tfrindex=%#x, "
	    "ctrldssegment=%#x, periodiclistbase=%#x,\n\tasynclistaddr=%#x, "
	    "configflag=%#x,\n\tportsc[1]=%#x, portsc[2]=%#x, systune=%#x",
	    swap_32(hcor->or_usbcmd), swap_32(hcor->or_usbsts),
	    swap_32(hcor->or_usbintr), swap_32(hcor->or_frindex),
	    swap_32(hcor->or_ctrldssegment),
	    swap_32(hcor->or_periodiclistbase),
	    swap_32(hcor->or_asynclistaddr), swap_32(hcor->or_configflag),
	    swap_32(hcor->or_portsc[0]), swap_32(hcor->or_portsc[1]),
	    swap_32(hcor->or_systune));
}
#endif

#ifdef EHCI_DEBUG
static void
dump_TD(struct qTD *td)
{

	printf("%p: qt_next=%#x, qt_altnext=%#x, qt_token=%#x, "
	    "qt_buffer={%#x,%#x,%#x,%#x,%#x}", td, swap_32(td->qt_next),
	    swap_32(td->qt_altnext), swap_32(td->qt_token),
	    swap_32(td->qt_buffer[0]), swap_32(td->qt_buffer[1]),
	    swap_32(td->qt_buffer[2]), swap_32(td->qt_buffer[3]),
	    swap_32(td->qt_buffer[4]));
}

static void
dump_QH(struct QH *qh)
{

	printf("%p: qh_link=%#x, qh_endpt1=%#x, qh_endpt2=%#x, qh_curtd=%#x",
	    qh, swap_32(qh->qh_link), swap_32(qh->qh_endpt1),
	    swap_32(qh->qh_endpt2), swap_32(qh->qh_curtd));
	dump_TD(&qh->qh_overlay);
}
#endif

#if defined(CONFIG_EX3242)  || defined(CONFIG_EX45XX)
#if defined(CONFIG_EX3242)
#define	GPIO_ISP	0x10000000
#define	GPIO_ST		0x08000000
#endif
#if defined(CONFIG_EX45XX)
#define	GPIO_ISP	0x00000010
#define	GPIO_ST		0x00000008
#endif

static void
gpio_clear(uint32_t bits)
{
	immap_t *immap = (immap_t *)CFG_CCSRBAR;
	volatile ccsr_gur_t *gur = &immap->im_gur;
	uint32_t new_v, old_v;

	old_v = gur->gpoutdr;
	new_v = old_v & ~bits;
	gur->gpoutdr = new_v;
}

static void
gpio_set(uint32_t bits)
{
	immap_t *immap = (immap_t *)CFG_CCSRBAR;
	volatile ccsr_gur_t *gur = &immap->im_gur;
	uint32_t new_v, old_v;

	old_v = gur->gpoutdr;
	new_v = old_v | bits;
	gur->gpoutdr = new_v;
}
#endif
static __inline int
min3(int a, int b, int c)
{

	if (b < a)
		a = b;
	if (c < a)
		a = c;
	return (a);
}

#ifdef CONFIG_ACX

#include <asm/io.h>

#define P2020_USBMODE	(CFG_USB_ADDR + 0x1A8)
	#define P2020_OTG_HOSTMODE		0x3

#define P2020_SNOOP1 	(CFG_USB_ADDR + 0x400)
#define P2020_SNOOP2 	(CFG_USB_ADDR + 0x404)
	#define P2020_SNOOP_SIZE_2GB 	0x1E

#define P2020_CONTROL	(CFG_USB_ADDR + 0x500)
	#define P2020_USB_EN      		(1 << 2)

#define P2020_PRICTRL	(CFG_USB_ADDR + 0x40C)

#define P2020_AGE_CNT_THRESH (CFG_USB_ADDR + 0x408)

#define P2020_SI_CTRL  	     (CFG_USB_ADDR + 0x410)


int acx_ehci_init(void)
{
	hccr = (struct ehci_hccr *)(CFG_USB_ADDR + 0x100);
	hcor = (volatile struct ehci_hcor *)((uint32_t)hccr + 
		                                 hccr->cr_caplength);
	/* 
	 * Put the OTG controller to host mode
	 */
	out_le32((void*)P2020_USBMODE, 
		     in_le32((void*)P2020_USBMODE) | P2020_OTG_HOSTMODE);

	out_be32((void*)P2020_SNOOP1, P2020_SNOOP_SIZE_2GB);
	out_be32((void*)P2020_SNOOP2, 0x80000000 | P2020_SNOOP_SIZE_2GB);

	out_le32(&(hcor->or_portsc[0]), EHCI_PS_PTS_ULPI);

	out_be32((void*)P2020_CONTROL,
		     in_be32((void*)P2020_CONTROL) | P2020_USB_EN);

	out_be32((void*)P2020_PRICTRL, 0);

	out_be32((void*)P2020_AGE_CNT_THRESH, 0x40);
	out_be32((void*)P2020_SI_CTRL, 0x1);

	in_le32((void*)P2020_USBMODE);

	return 0;
}

#endif

/*
 * Create the appropriate control structures to manage
 * a new EHCI host controller.
 */
int
usb_lowlevel_init(void)
{
#if defined(CONFIG_MARVELL)
	uint32_t reg;

#if !defined(USB_MULTI_CONTROLLER)
	base = CFG_USB_EHCI_BASE;
	hccr = (void *)CFG_USB_EHCI_HCCR_BASE;
	hcor = (void *)CFG_USB_EHCI_HCOR_BASE;
#endif

	/*
	 * Clear interrupt cause and mask
	 */
	wrl(base|USB_CAUSE, 0);
	wrl(base|USB_MASK, 0);

	/*
	 * Reset controller
	 */
	wrl(base|USB_CMD, rdl(base|USB_CMD) | 0x2);
	while (rdl(base|USB_CMD) & 0x2);

	/*
	 * GL# USB-10: Set IPG for non start of frame packets
	 * Bits[14:8]=0xc
	 */
	wrl(base|USB_IPG, (rdl(base|USB_IPG) & ~0x7f00) | 0xc00);

	/*
	 * GL# USB-9: USB 2.0 Power Control
	 * BG_VSEL[7:6]=0x1
	 */
	wrl(base|USB_PHY_PWR_CTRL, (rdl(base|USB_PHY_PWR_CTRL) & ~0xc0)| 0x40);

	/*
	 * GL# USB-1: USB PHY Tx Control - force calibration to '8'
	 * TXDATA_BLOCK_EN[21]=0x1, EXT_RCAL_EN[13]=0x1, IMP_CAL[6:3]=0x8
	 */
	wrl(base|USB_PHY_TX_CTRL, (rdl(base|USB_PHY_TX_CTRL) & ~0x78) | 0x202040);

	/*
	 * GL# USB-3 GL# USB-9: USB PHY Rx Control
	 * RXDATA_BLOCK_LENGHT[31:30]=0x3, EDGE_DET_SEL[27:26]=0,
	 * CDR_FASTLOCK_EN[21]=0, DISCON_THRESHOLD[9:8]=0, SQ_THRESH[7:4]=0x1
	 */
	wrl(base|USB_PHY_RX_CTRL, (rdl(base|USB_PHY_RX_CTRL) & ~0xc2003f0) | 0xc0000010);

	/*
	 * GL# USB-3 GL# USB-9: USB PHY IVREF Control
	 * PLLVDD12[1:0]=0x2, RXVDD[5:4]=0x3, Reserved[19]=0
	 */
	wrl(base|USB_PHY_IVREF_CTRL, (rdl(base|USB_PHY_IVREF_CTRL) & ~0x80003 ) | 0x32);

	/*
	 * GL# USB-3 GL# USB-9: USB PHY Test Group Control
	 * REG_FIFO_SQ_RST[15]=0
	 */
	wrl(base|USB_PHY_TST_GRP_CTRL, rdl(base|USB_PHY_TST_GRP_CTRL) & ~0x8000);

	/*
	 * Stop and reset controller
	 */
	wrl(base|USB_CMD, rdl(base|USB_CMD) & ~0x1);
	wrl(base|USB_CMD, rdl(base|USB_CMD) | 0x2);
	while (rdl(base|USB_CMD) & 0x2);

#if defined(CONFIG_EX3300)	 
	/*
	 * GL# USB-5 Streaming disable REG_USB_MODE[4]=1
	 * TBD: This need to be done after each reset!
	 * GL# USB-4 Setup USB Host mode
	 * Errata for MV78100/78200 A0/A1
	 */
	wrl(base|USB_MODE, 0x13);
#else
	wrl(base|USB_MODE, 0x3);
#endif

#elif defined(CONFIG_ACX)
	uint32_t reg;
	acx_ehci_init();
#else
	PCI_DEV_T dev;
	uint32_t addr, reg;

   
	dev = PCI_FIND_DEVICES(ehci_pci_ids, 0);
	if (dev == -1) {
		printf("EHCI host controller not found\n");
		return (-1);
	}
	else {
		volatile uint32_t *hcreg;

		PCI_READ_CONFIG_DWORD(dev, PCI_BASE_ADDRESS_0, &addr);
		hcreg = (uint32_t *)(addr + 8);
		*hcreg = swap_32(1);
		udelay(1000);
	}
	hccr = (void *)addr;

	addr += hccr->cr_caplength;
	hcor = (void *)addr;
#endif

	/* Reset the device */
	hcor->or_usbcmd |= swap_32(2);
	udelay(1000);
	while (hcor->or_usbcmd & swap_32(2))
		udelay(1000);

#if defined(CONFIG_MARVELL)
	wrl(base|USB_MODE, 0x3);
#endif

#ifdef CONFIG_ACX
	acx_ehci_init();
#endif

	reg = swap_32(hccr->cr_hcsparams);
	descr.hub[2] = reg & 0xf;
	if (reg & 0x10000)	/* Port Indicators */
		descr.hub[3] |= 0x80;
	if (reg & 0x10)		/* Port Power Control */
		descr.hub[3] |= 0x01;

#if !defined(CONFIG_MARVELL)
	/* Latte/Espresso USB hardware bug workaround */
	hcor->or_systune |= swap_32(3);
#endif

	/* take control over the ports */
	hcor->or_configflag |= swap_32(1);

	/* Set head of reclaim list */
	memset(&qh_list, 0, sizeof(qh_list));
	qh_list.qh_link = swap_32((uint32_t)&qh_list | QH_LINK_TYPE_QH);
	qh_list.qh_endpt1 = swap_32((1 << 15) | (USB_SPEED_HIGH << 12));
	qh_list.qh_curtd = swap_32(QT_NEXT_TERMINATE);
	qh_list.qh_overlay.qt_next = swap_32(QT_NEXT_TERMINATE);
	qh_list.qh_overlay.qt_altnext = swap_32(QT_NEXT_TERMINATE);
	qh_list.qh_overlay.qt_token = swap_32(0x40);

	DCACHE_FLUSH(&qh_list, sizeof(qh_list));

	/* Set async. queue head pointer. */
#if defined(CONFIG_MARVELL)
	hcor->or_asynclistaddr = swap_32((uint32_t)&qh_list) | 0x1;
#else
	hcor->or_asynclistaddr = swap_32((uint32_t)&qh_list);
#endif

	/* Start the host controller. */
	hcor->or_usbcmd |= swap_32(1);

	rootdev = 0;

	return (0);
}

/*
 * Destroy the appropriate control structures corresponding
 * the the EHCI host controller.
 */
int
usb_lowlevel_stop(uint32_t port_n)
{
#if defined(CONFIG_EX3242)  || defined(CONFIG_EX45XX)
	/* Assert ST72681 hard reset. */
	gpio_clear(GPIO_ST);
	udelay(100);
#else
	if (hcor == NULL)
		return (0);

	if (port_n > 2) {
		port_n = 0;
	}
	if (port_n) {
		hcor->or_portsc[port_n - 1] &= swap_32(~(EHCI_PS_PE));
	} else {
		/* Stop the controller. */
		hcor->or_usbcmd &= swap_32(~1);
	}
#endif

	return (0);
}

static void *
ehci_alloc(size_t sz, size_t align)
{
	static struct QH qh __attribute__((aligned(32)));
	static struct qTD td[3] __attribute__((aligned(32)));
	static int ntds = 0;
	void *p;

	switch (sz) {
	case sizeof(struct QH):
		p = &qh;
		ntds = 0;
		break;
	case sizeof(struct qTD):
		if (ntds == 3) {
			DBG("out of TDs");
			return (NULL);
		}
		p = &td[ntds];
		ntds++;
		break;
	default:
		DBG("unknown allocation size");
		return (NULL);
	}

	memset(p, 0, sz);
	return (p);
}

static void
ehci_free(void *p, size_t sz)
{
}

static int
ehci_td_buffer(struct qTD *td, void *buf, size_t sz)
{
	uint32_t addr, delta, next;
	int idx;

	addr = (uint32_t)buf;
	idx = 0;
	while (idx < 5) {
		td->qt_buffer[idx] = swap_32(addr);
		next = (addr + 4096) & ~4095;
		delta = next - addr;
		if (delta >= sz)
			break;
		sz -= delta;
		addr = next;
		idx++;
	}

	if (idx == 5) {
		DBG("out of buffer pointers (%u bytes left)", sz);
		return (-1);
	}

	return (0);
}

static int
ehci_submit_async(struct usb_device *dev, unsigned long pipe, void *buffer,
    int length, struct devrequest *req)
{
	struct QH *qh;
	struct qTD *td;
	volatile struct qTD *vtd;
	unsigned long ts;
	char *s;
	uint32_t *tdp;
	uint32_t endpt, token, usbsts;
	uint32_t c, toggle;
	uint32_t timeout;

	timeout = 5;
	s = getenv("usb_timeout");
	if (s != NULL)
		timeout = simple_strtoul(s, NULL, 10);
	timeout *= CFG_HZ;

	DBG("dev=%u, pipe=%lx, buffer=%p, length=%d, req=%p", dev->devnum,
	    pipe, buffer, length, req);
#ifdef EHCI_DEBUG
	if (req != NULL)
		printf("req=%u (%#x), type=%u (%#x), val=%u (%#x), index=%u\n",
		    req->request, req->request,
		    req->requesttype, req->requesttype,
		    swap_16(req->value), swap_16(req->value),
		    swap_16(req->index), swap_16(req->index));
	else if (usb_pipeout(pipe))
		dump_data(buffer, length);
#endif

#if defined(USB_MULTI_CONTROLLER)
	if (dev->controller != curr_controller) {
		curr_controller = dev->controller;
		hcor = ehci_hcor[dev->controller];
		DCACHE_INVALIDATE(hcor, 4);
	}
#endif

	qh = ehci_alloc(sizeof(struct QH), 32);
	if (qh == NULL) {
		DBG("unable to allocate QH");
		return (-1);
	}
	qh->qh_link = swap_32((uint32_t)&qh_list | QH_LINK_TYPE_QH);
	c = (usb_pipespeed(pipe) != USB_SPEED_HIGH &&
	    usb_pipeendpoint(pipe) == 0) ? 1 : 0;
	endpt = (8 << 28) |
	    (c << 27) |
	    (usb_maxpacket(dev, pipe) << 16) |
	    (0 << 15) |
	    (1 << 14) |
	    (usb_pipespeed(pipe) << 12) |
	    (usb_pipeendpoint(pipe) << 8) |
	    (0 << 7) |
	    (usb_pipedevice(pipe) << 0);
	qh->qh_endpt1 = swap_32(endpt);
	endpt = (1 << 30) |
	    (dev->portnr << 23) |
	    (dev->parent->devnum << 16) |
	    (0 << 8) |
	    (0 << 0);
	qh->qh_endpt2 = swap_32(endpt);
	qh->qh_overlay.qt_next = swap_32(QT_NEXT_TERMINATE);
	qh->qh_overlay.qt_altnext = swap_32(QT_NEXT_TERMINATE);

	DCACHE_FLUSH(qh, sizeof(*qh));

	td = NULL;
	tdp = &qh->qh_overlay.qt_next;

	toggle = usb_gettoggle(dev, usb_pipeendpoint(pipe), usb_pipeout(pipe));

	if (req != NULL) {
		td = ehci_alloc(sizeof(struct qTD), 32);
		if (td == NULL) {
			DBG("unable to allocate SETUP td");
			goto fail;
		}
		td->qt_next = swap_32(QT_NEXT_TERMINATE);
		td->qt_altnext = swap_32(QT_NEXT_TERMINATE);
		token = (0 << 31) |
		    (sizeof(*req) << 16) |
		    (0 << 15) |
		    (0 << 12) |
		    (3 << 10) |
		    (2 << 8) |
		    (0x80 << 0);
		td->qt_token = swap_32(token);
        
		DCACHE_FLUSH(req, sizeof(*req));
		if (ehci_td_buffer(td, req, sizeof(*req)) != 0) {
			DBG("unable construct SETUP td");
			ehci_free(td, sizeof(*td));
			goto fail;
		}
		*tdp = swap_32((uint32_t)td);

		DCACHE_FLUSH(tdp, sizeof(*tdp));
		DCACHE_FLUSH(td, sizeof(*td));

		tdp = &td->qt_next;
		toggle = 1;
	}

	if (length > 0 || req == NULL) {
		td = ehci_alloc(sizeof(struct qTD), 32);
		if (td == NULL) {
			DBG("unable to allocate DATA td");
			goto fail;
		}
		td->qt_next = swap_32(QT_NEXT_TERMINATE);
		td->qt_altnext = swap_32(QT_NEXT_TERMINATE);
		token = (toggle << 31) |
		    (length << 16) |
		    ((req == NULL ? 1 : 0) << 15) |
		    (0 << 12) |
		    (3 << 10) |
		    ((usb_pipein(pipe) ? 1 : 0) << 8) |
		    (0x80 << 0);
		td->qt_token = swap_32(token);
		if (ehci_td_buffer(td, buffer, length) != 0) {
			DBG("unable construct DATA td");
			ehci_free(td, sizeof(*td));
			goto fail;
		}
		DCACHE_INVALIDATE(buffer, length);
		*tdp = swap_32((uint32_t)td);
        
		DCACHE_FLUSH(tdp, sizeof(*tdp));
		DCACHE_FLUSH(td, sizeof(*td));

		tdp = &td->qt_next;
	}

	if (req != NULL) {
		td = ehci_alloc(sizeof(struct qTD), 32);
		if (td == NULL) {
			DBG("unable to allocate ACK td");
			goto fail;
		}
		td->qt_next = swap_32(QT_NEXT_TERMINATE);
		td->qt_altnext = swap_32(QT_NEXT_TERMINATE);
		token = (toggle << 31) |
		    (0 << 16) |
		    (1 << 15) |
		    (0 << 12) |
		    (3 << 10) |
		    ((usb_pipein(pipe) ? 0 : 1) << 8) |
		    (0x80 << 0);
		td->qt_token = swap_32(token);
		*tdp = swap_32((uint32_t)td);
        
		DCACHE_FLUSH(tdp, sizeof(*tdp));
		DCACHE_FLUSH(td, sizeof(*td));

		tdp = &td->qt_next;
	}

	qh_list.qh_link = swap_32((uint32_t)qh | QH_LINK_TYPE_QH);

	DCACHE_FLUSH(&qh_list.qh_link, sizeof(qh_list.qh_link));

	usbsts = swap_32(hcor->or_usbsts);
	hcor->or_usbsts = swap_32(usbsts & 0x3f);

	/* Enable async. schedule. */
	hcor->or_usbcmd |= swap_32(0x20);
	while ((hcor->or_usbsts & swap_32(0x8000)) == 0)
		udelay(1);

	/* Wait for TDs to be processed. */
	ts = get_timer(0);
	vtd = td;
	do {
		DCACHE_INVALIDATE(vtd, sizeof(*td));
		token = swap_32(vtd->qt_token);
		if (!(token & 0x80))
			break;
	} while (get_timer(ts) < timeout);

	/* Disable async schedule. */
	hcor->or_usbcmd &= ~swap_32(0x20);
	while ((hcor->or_usbsts & swap_32(0x8000)) != 0)
		udelay(1);

	qh_list.qh_link = swap_32((uint32_t)&qh_list | QH_LINK_TYPE_QH);

	DCACHE_FLUSH(&qh_list.qh_link, sizeof(qh_list.qh_link));

	token = swap_32(qh->qh_overlay.qt_token);
	if (!(token & 0x80)) {
		// DBG("TOKEN=%#x", token);
		switch (token & 0xfc) {
		case 0:
			toggle = token >> 31;
			usb_settoggle(dev, usb_pipeendpoint(pipe),
			    usb_pipeout(pipe), toggle);
			dev->status = 0;
			break;
		case 0x40:
			dev->status = USB_ST_STALLED;
			break;
		case 0xa0:
		case 0x20:
			dev->status = USB_ST_BUF_ERR;
			break;
		case 0x50:
		case 0x10:
			dev->status = USB_ST_BABBLE_DET;
			break;
		default:
			dev->status = USB_ST_CRC_ERR;
			break;
		}
		dev->act_len = length - ((token >> 16) & 0x7fff);
	} else {
		printf("T ");
		dev->status = USB_ST_TIMEOUT;
		dev->act_len = 0;
	}

#ifdef EHCI_DEBUG
	if (dev->status != 0) {
		s = getenv("usb_dumps");
		if (s != NULL) {
			printf("\n*** Dump of QH:\n");
			dump_QH(qh);
			printf("\n*** Dump of all TDs:\n");
			td = (void *)swap_32(qh->qh_overlay.qt_next);
			while (td != (void *)QT_NEXT_TERMINATE) {
				qh->qh_overlay.qt_next = td->qt_next;
				dump_TD(td);
				td = (void *)swap_32(qh->qh_overlay.qt_next);
			}
		}
	}
#endif

	DBG("STATUS %#x: dev=%u, usbcmd=%#x, usbsts=%#x, p[1]=%#x, p[2]=%#x",
	    dev->status, dev->devnum, swap_32(hcor->or_usbcmd),
	    swap_32(hcor->or_usbsts), swap_32(hcor->or_portsc[0]),
	    swap_32(hcor->or_portsc[1]));
#ifdef EHCI_DEBUG
	if (usb_pipein(pipe))
		dump_data(buffer, length);
#endif

	return ((dev->status != USB_ST_NOT_PROC) ? 0 : -1);

 fail:
	td = (void *)swap_32(qh->qh_overlay.qt_next);
	while (td != (void *)QT_NEXT_TERMINATE) {
		qh->qh_overlay.qt_next = td->qt_next;
		ehci_free(td, sizeof(*td));
		td = (void *)swap_32(qh->qh_overlay.qt_next);
	}
	ehci_free(qh, sizeof(*qh));
	return (-1);
}

static int
ehci_submit_root(struct usb_device *dev, unsigned long pipe, void *buffer,
    int length, struct devrequest *req)
{
	uint8_t tmpbuf[4];
	void *srcptr;
	int len, srclen;
	uint32_t reg;

	srclen = 0;
	srcptr = NULL;

	DBG("req=%u (%#x), type=%u (%#x), value=%u, index=%u",
	    req->request, req->request,
	    req->requesttype, req->requesttype,
	    swap_16(req->value), swap_16(req->index));

#define C(a,b)	(((b) << 8) | (a))

#if defined(USB_MULTI_CONTROLLER)
	if (dev->controller != curr_controller) {
		curr_controller = dev->controller;
		hcor = ehci_hcor[dev->controller];
		DCACHE_INVALIDATE(hcor, 4);
	}
#endif

	switch (C(req->request, req->requesttype)) {
	case C(USB_REQ_GET_DESCRIPTOR, USB_DIR_IN | USB_RECIP_DEVICE):
		switch(swap_16(req->value) >> 8) {
		case USB_DT_DEVICE:
			srcptr = descr.device;
			srclen = sizeof(descr.device);
			break;
		case USB_DT_CONFIG:
			srcptr = descr.config;
			srclen = sizeof(descr.config) +
			    sizeof(descr.interface) + sizeof(descr.endpoint);
			break;
		case USB_DT_STRING:
			switch (swap_16(req->value) & 0xff) {
			case 0:		/* Language */
				srcptr = "\4\3\1\0";
				srclen = 4;
				break;
			case 1:		/* Vendor */
				srcptr = "\20\3P\0h\0i\0l\0i\0p\0s\0";
				srclen = 16;
				break;
			case 2:		/* Product */
				srcptr = "\12\3E\0H\0C\0I\0";
				srclen = 10;
				break;
			default:
				goto unknown;
			}
			break;
		default:
			DBG("unknown value %x", swap_16(req->value));
			goto unknown;
		}
		break;
	case C(USB_REQ_GET_DESCRIPTOR, USB_DIR_IN | USB_RT_HUB):
		switch (swap_16(req->value) >> 8) {
		case USB_DT_HUB:
			srcptr = descr.hub;
			srclen = sizeof(descr.hub);
			break;
		default:
			DBG("unknown value %x", swap_16(req->value));
			goto unknown;
		}
		break;
	case C(USB_REQ_SET_ADDRESS, USB_RECIP_DEVICE):
		rootdev = swap_16(req->value);
		break;
	case C(USB_REQ_SET_CONFIGURATION, USB_RECIP_DEVICE):
		/* Nothing to do */
		break;
	case C(USB_REQ_GET_STATUS, USB_DIR_IN | USB_RT_HUB):
		tmpbuf[0] = 1; /* USB_STATUS_SELFPOWERED */
		tmpbuf[1] = 0;
		srcptr = tmpbuf;
		srclen = 2;
		break;
	case C(USB_REQ_GET_STATUS, USB_DIR_IN | USB_RT_PORT):
		memset(tmpbuf, 0, 4);
		reg = swap_32(hcor->or_portsc[swap_16(req->index) - 1]);
		if (reg & EHCI_PS_CS)
			tmpbuf[0] |= USB_PORT_STAT_CONNECTION;
		if (reg & EHCI_PS_PE)
			tmpbuf[0] |= USB_PORT_STAT_ENABLE;
		if (reg & EHCI_PS_SUSP)
			tmpbuf[0] |= USB_PORT_STAT_SUSPEND;
		if (reg & EHCI_PS_OCA)
			tmpbuf[0] |= USB_PORT_STAT_OVERCURRENT;
		if (reg & EHCI_PS_PR)
			tmpbuf[0] |= USB_PORT_STAT_RESET;
		if (reg & EHCI_PS_PP)
			tmpbuf[1] |= USB_PORT_STAT_POWER >> 8;
		tmpbuf[1] |= USB_PORT_STAT_HIGH_SPEED >> 8;

		if (reg & EHCI_PS_CSC)
			tmpbuf[2] |= USB_PORT_STAT_C_CONNECTION;
		if (reg & EHCI_PS_PEC)
			tmpbuf[2] |= USB_PORT_STAT_C_ENABLE;
		if (reg & EHCI_PS_OCC)
			tmpbuf[2] |= USB_PORT_STAT_C_OVERCURRENT;
		if (portreset & (1 << swap_16(req->index)))
			tmpbuf[2] |= USB_PORT_STAT_C_RESET;
		srcptr = tmpbuf;
		srclen = 4;
		break;
	case C(USB_REQ_SET_FEATURE, USB_DIR_OUT | USB_RT_PORT):
		reg = swap_32(hcor->or_portsc[swap_16(req->index) - 1]);
		reg &= ~EHCI_PS_CLEAR;
		switch (swap_16(req->value)) {
		case USB_PORT_FEAT_POWER:
#if defined(CONFIG_EX3242)  || defined(CONFIG_EX45XX)
			if (swap_16(req->index) == 2)
				gpio_set(GPIO_ST);
#endif
			reg |= EHCI_PS_PP;
			break;
		case USB_PORT_FEAT_RESET:
			if (EHCI_PS_IS_LOWSPEED(reg)) {
				/* Low speed device, give up ownership. */
				reg |= EHCI_PS_PO;
				break;
			}
			/* Start reset sequence. */
			reg &= ~EHCI_PS_PE;
			reg |= EHCI_PS_PR;
			hcor->or_portsc[swap_16(req->index) - 1] = swap_32(reg);
			/* Wait for reset to complete. */
			udelay(250000);

			/*
			 * Reset Termination not needed for certain chips
			 */
#if !defined(CONFIG_MARVELL) && !defined(CONFIG_P2020) 
			/* Terminate reset sequence. */
			reg &= ~EHCI_PS_PR;
			hcor->or_portsc[swap_16(req->index) - 1] = swap_32(reg);
#endif
			/* Wait for HC to complete reset. */
			udelay(2000);

			reg = swap_32(hcor->or_portsc[swap_16(req->index) - 1]);
			reg &= ~EHCI_PS_CLEAR;
			if ((reg & EHCI_PS_PE) == 0) {
				/* Not a high speed device, give up ownership.*/
				reg |= EHCI_PS_PO;
				break;
			}
			portreset |= 1 << swap_16(req->index);
			break;
		default:
			DBG("unknown feature %x", swap_16(req->value));
			goto unknown;
		}
		hcor->or_portsc[swap_16(req->index) - 1] = swap_32(reg);
		break;
	case C(USB_REQ_CLEAR_FEATURE, USB_DIR_OUT | USB_RT_PORT):
		reg = swap_32(hcor->or_portsc[swap_16(req->index) - 1]);
		reg &= ~EHCI_PS_CLEAR;
		switch (swap_16(req->value)) {
		case USB_PORT_FEAT_ENABLE:
			reg &= ~EHCI_PS_PE;
			break;
		case USB_PORT_FEAT_C_CONNECTION:
			reg |= EHCI_PS_CSC;
			break;
		case USB_PORT_FEAT_C_RESET:
			portreset &= ~(1 << swap_16(req->index));
			break;
		default:
			DBG("unknown feature %x", swap_16(req->value));
			goto unknown;
		}
		hcor->or_portsc[swap_16(req->index) - 1] = swap_32(reg);
		break;
	default:
		DBG("Unknown request %x", C(req->request, req->requesttype));
		goto unknown;
	}

#undef C

	len = min3(srclen, swap_16(req->length), length);
	if (srcptr != NULL && len > 0)
		memcpy(buffer, srcptr, len);
	dev->act_len = len;
	dev->status = 0;
	return (0);

 unknown:
	DBG("requesttype=%x, request=%x, value=%x, index=%x, length=%x",
	    req->requesttype, req->request, swap_16(req->value),
	    swap_16(req->index), swap_16(req->length));

	dev->act_len = 0;
	dev->status = USB_ST_STALLED;
	return (-1);
}

int
submit_bulk_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
    int length)
{

	if (usb_pipetype(pipe) != PIPE_BULK) {
		DBG("non-bulk pipe (type=%lu)", usb_pipetype(pipe));
		return (-1);
	}
	return (ehci_submit_async(dev, pipe, buffer, length, NULL));
}

int
submit_control_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
    int length, struct devrequest *setup)
{
	int res;

	if (usb_pipetype(pipe) != PIPE_CONTROL) {
		DBG("non-control pipe (type=%lu)", usb_pipetype(pipe));
		return (-1);
	}

	if (usb_pipedevice(pipe) == rootdev) {
		if (rootdev == 0)
			dev->speed = USB_SPEED_HIGH;
		return (ehci_submit_root(dev, pipe, buffer, length, setup));
	}

	res = ehci_submit_async(dev, pipe, buffer, length, setup);
	if (res == -1)
		return (res);

	/* USB error recovery */
	if (board_booting) {
		if (dev->status != 0)
			if (board_usb_err_recover) {
				board_usb_err_recover();
			}
	}

	return (res);
}

int
submit_int_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
    int length, int interval)
{

	DBG("dev=%p, pipe=%lu, buffer=%p, length=%d, interval=%d", dev, pipe,
	    buffer, length, interval);
	return (-1);
}

#if defined(USB_MULTI_CONTROLLER)
int 
usb_lowlevel_init_n(int controller)
{
    base = ehci_base[controller] = getUsbBase(controller);
    hccr = ehci_hccr[controller] = (void *)getUsbHccrBase(controller);
    hcor = ehci_hcor[controller] = (void *)getUsbHcorBase(controller);
    curr_controller = controller;
    
    return usb_lowlevel_init();
}

int 
usb_lowlevel_stop_n(int controller, uint32_t port_n)
{
    base = ehci_base[controller];
    hccr = ehci_hccr[controller];
    hcor = ehci_hcor[controller];
    curr_controller = controller;

    return usb_lowlevel_stop(port_n);
}

void
usb_reset_rootdev(void)
{
    rootdev = 0;
}
#endif /* USB_MULTI_CONTROLLER */

#endif /*CONFIG_USB_EHCI*/
