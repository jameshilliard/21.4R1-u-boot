/*-
 * Copyright (c) 2011, Juniper Networks, Inc.
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
#if defined (CONFIG_USB_EHCI)

#include <usb.h>
#include <usb_ehci.h>

#include <cvmx.h>
#include <cvmx-uctlx-addresses.h>

#ifdef EHCI_DEBUG
#define	DBG(format, arg...)	\
	printf("\nEHCI_DEBUG: %s: " format "\n", __func__, ## arg)
#else
#define	DBG(format, arg...)	\
	do {} while(0)
#endif /* EHCI_DEBUG */

DECLARE_GLOBAL_DATA_PTR;

#define OCTEON_EHCI_REF_CLK   50000000ULL
#define OCTEON_EHCI_VIRT_BASE 0xC0000000U
#define OCTEON_EHCI_REGS_OFS  0x10

#define KSEG0                 0x80000000U
#define KSEG1                 0xa0000000U

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
	0x81,		/* bEndpointAddress: UE_DIR_IN | EHCI_NTR_ENDPT */
	3,		/* bmAttributes: UE_INTERRUPT */
	8, 0,		/* wMaxPacketSize */
	255		/* bInterval */
    }
};

static struct ehci_hccr *hccr;	/* R/O registers, not need for volatile */
static volatile struct ehci_hcor *hcor;
static struct QH qh_list __attribute__((aligned(32)));
static int rootdev;
static uint16_t portreset;

static uint64_t
cvmx_get_sclk_rate (void)
{
    const uint64_t  ref_clk = OCTEON_EHCI_REF_CLK;
    cvmx_mio_rst_boot_t mio_rst_boot;

    mio_rst_boot.u64 = cvmx_read_csr(CVMX_MIO_RST_BOOT);
    return ref_clk * mio_rst_boot.s.pnr_mul;
}

static void
octeon_clock_init (void)
{
    uint8_t i;
    uint64_t div;
    union cvmx_uctlx_if_ena if_ena;
    union cvmx_uctlx_clk_rst_ctl clk_rst_ctl;
    union cvmx_uctlx_uphy_ctl_status uphy_ctl_status;
    union cvmx_uctlx_uphy_portx_ctl_status port_ctl_status;

    /* 
     * Step 1 : Voltages already expected to be stable by now 
     *
     * Step 2 : Enable SCLK of UCTL.
     */
    if_ena.u64 = 0;
    if_ena.s.en = 1;
    cvmx_write_csr(CVMX_UCTLX_IF_ENA(0), if_ena.u64);

    /* Step 3: Configure the reference clock, PHY, and HCLK */
    clk_rst_ctl.u64 = cvmx_read_csr(CVMX_UCTLX_CLK_RST_CTL(0));
    /* 3a */
    clk_rst_ctl.s.p_por = 1;
    clk_rst_ctl.s.hrst = 0;
    clk_rst_ctl.s.p_prst = 0;
    clk_rst_ctl.s.h_clkdiv_rst = 0;
    clk_rst_ctl.s.o_clkdiv_rst = 0;
    clk_rst_ctl.s.h_clkdiv_en = 0;
    clk_rst_ctl.s.o_clkdiv_en = 0;
    cvmx_write_csr(CVMX_UCTLX_CLK_RST_CTL(0), clk_rst_ctl.u64);

    /* 3b */
    /* 12MHz crystal. */
    clk_rst_ctl.s.p_refclk_sel = 0;
    clk_rst_ctl.s.p_refclk_div = 0;
    cvmx_write_csr(CVMX_UCTLX_CLK_RST_CTL(0), clk_rst_ctl.u64);

    /* 3c */
    div = cvmx_get_sclk_rate() / 130000000ull;

    switch (div) {
    case 0:
        div = 1;
        break;
    case 1:
    case 2:
    case 3:
    case 4:
        break;
    case 5:
        div = 4;
        break;
    case 6:
    case 7:
        div = 6;
        break;
    case 8:
    case 9:
    case 10:
    case 11:
        div = 8;
        break;
    default:
        div = 12;
        break;
    }
    clk_rst_ctl.s.h_div = div;
    cvmx_write_csr(CVMX_UCTLX_CLK_RST_CTL(0), clk_rst_ctl.u64);

    /* Read it back, */
    clk_rst_ctl.u64 = cvmx_read_csr(CVMX_UCTLX_CLK_RST_CTL(0));
    clk_rst_ctl.s.h_clkdiv_en = 1;
    cvmx_write_csr(CVMX_UCTLX_CLK_RST_CTL(0), clk_rst_ctl.u64);

    /* 3d */
    clk_rst_ctl.s.h_clkdiv_rst = 1;
    cvmx_write_csr(CVMX_UCTLX_CLK_RST_CTL(0), clk_rst_ctl.u64);

    /* 
     * 3e 
     * Wait for 64 SCLK clock cycles 
     */
    udelay(1000);

    /*
     * Step 4: Program the power-on reset field in the UCTL
     * clock-reset-control register.
     */
    clk_rst_ctl.s.p_por = 0;
    cvmx_write_csr(CVMX_UCTLX_CLK_RST_CTL(0), clk_rst_ctl.u64);

    /* Step 5:    Wait 1 ms for the PHY clock to start. */
    udelay(2000);

    /*
     * Step 6: Program the reset input from automatic test
     * equipment field in the UPHY CSR
     */
    uphy_ctl_status.u64 = cvmx_read_csr(CVMX_UCTLX_UPHY_CTL_STATUS(0));
    uphy_ctl_status.s.ate_reset = 1;
    cvmx_write_csr(CVMX_UCTLX_UPHY_CTL_STATUS(0), uphy_ctl_status.u64);

    /* Step 7: Wait for at least 10ns. */
    udelay(1000);

    /* Step 8: Clear the ATE_RESET field in the UPHY CSR. */
    uphy_ctl_status.s.ate_reset = 0;
    cvmx_write_csr(CVMX_UCTLX_UPHY_CTL_STATUS(0), uphy_ctl_status.u64);

    /*
     * Step 9: Wait for at least 20ns for UPHY to output PHY clock
     * signals and OHCI_CLK48
     */
    udelay(1000);

    /* Step 10: Configure the OHCI_CLK48 and OHCI_CLK12 clocks. */
    /* 10a */
    clk_rst_ctl.s.o_clkdiv_rst = 1;
    cvmx_write_csr(CVMX_UCTLX_CLK_RST_CTL(0), clk_rst_ctl.u64);

    /* 10b */
    clk_rst_ctl.s.o_clkdiv_en = 1;
    cvmx_write_csr(CVMX_UCTLX_CLK_RST_CTL(0), clk_rst_ctl.u64);

    /* 
     * 10c
     * Wait for 64 SCLK clock cycles 
     */
    udelay(1000);

    /* Step 11: Program the PHY reset field: UCTL0_CLK_RST_CTL[P_PRST] = 1 */
    clk_rst_ctl.s.p_prst = 1;
    cvmx_write_csr(CVMX_UCTLX_CLK_RST_CTL(0), clk_rst_ctl.u64);

    /* Step 12: Wait 1 uS. */
    udelay(1000);

    /* Step 13: Program the HRESET_N field: UCTL0_CLK_RST_CTL[HRST] = 1 */
    clk_rst_ctl.s.hrst = 1;
    cvmx_write_csr(CVMX_UCTLX_CLK_RST_CTL(0), clk_rst_ctl.u64);

    switch (gd->board_desc.board_type) {
#if defined CONFIG_JSRXNLE
    /* 
     * The txvreftune parameter needs to be set to 15 to 
     * get a compliant usb eye diagram for srx550.
    */    
    CASE_ALL_SRX550_MODELS
        for (i = 0; i <= 1; i++) {
            port_ctl_status.u64 =
                cvmx_read_csr(CVMX_UCTLX_UPHY_PORTX_CTL_STATUS(i, 0));
            port_ctl_status.s.txvreftune = 0xf;
            cvmx_write_csr(CVMX_UCTLX_UPHY_PORTX_CTL_STATUS(i, 0),
                           port_ctl_status.u64);
        }
        break;
#endif
    default:
        break;
    }
}

static void
octeon_usb_start (void)
{
    union cvmx_uctlx_ehci_ctl ehci_ctl;
    union cvmx_uctlx_ehci_fla ehci_fla;

    octeon_clock_init();

    /*
     * Due to a bug in hardware, the usb frame length is 
     * not the expected 125us, but 123us. This was reported
     * by Cavium as due to the FLA register not being set
     * to it's default value(0x20). Work around this by
     * setting it explicitly in software, to get the 
     * expected frame length.
     */
    ehci_fla.u64 = cvmx_read_csr(CVMX_UCTLX_EHCI_FLA(0));
    ehci_fla.s.fla = 0x20;
    cvmx_write_csr(CVMX_UCTLX_EHCI_FLA(0), ehci_fla.u64);

    ehci_ctl.u64 = cvmx_read_csr(CVMX_UCTLX_EHCI_CTL(0));
    /* Use 64-bit addressing. */
    ehci_ctl.s.ehci_64b_addr_en = 1;
    ehci_ctl.s.l2c_addr_msb = 0;
    ehci_ctl.s.l2c_buff_emod = 0x1; /* Byte swapped. */
    ehci_ctl.s.l2c_desc_emod = 0x1; /* Byte swapped. */
    cvmx_write_csr(CVMX_UCTLX_EHCI_CTL(0), ehci_ctl.u64);
}

static uint32_t
get_physaddr (uint32_t address)
{
    if (address >= KSEG1) {
        return(address - KSEG1);
    } else if (address >= KSEG0) {
        return(address - KSEG0);
    } else {
        return address;
    }
}

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

#endif

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

static __inline int
min3(int a, int b, int c)
{

	if (b < a)
		a = b;
	if (c < a)
		a = c;
	return (a);
}

/*
 * Create the appropriate control structures to manage
 * a new EHCI host controller.
 */
int
ehci_usb_lowlevel_init(void)
{
    uint32_t physaddr;
    uint32_t rval;
    uint64_t base, raddr;

    base = (uint64_t)OCTEON_EHCI_REG_BASE;
    octeon_usb_start();

	/* Reset the device */
    raddr = base + EHCI_REG_CMD;
    rval = cvmx_read64_uint32(raddr);
    rval |= EHCI_CMD_RST;
    cvmx_write64_uint32(raddr, rval);
	udelay(1000);

    do {
        rval = cvmx_read64_uint32(raddr);
        udelay(10);
    } while (rval & EHCI_CMD_RST);

    raddr = base + EHCI_REG_HCSPARAMS;
    rval = cvmx_read64_uint32(raddr);

	descr.hub[2] = rval & 0xf;
	if (rval & 0x10000)	/* Port Indicators */
		descr.hub[3] |= 0x80;
	if (rval & 0x10)		/* Port Power Control */
		descr.hub[3] |= 0x01;

	/* take control over the ports */
    raddr = base + EHCI_REG_CFGFLAG;
    rval = cvmx_read64_uint32(raddr);
    rval |= EHCI_CFG_CF;
    cvmx_write64_uint32(raddr, rval);

	/* Set head of reclaim list */
	memset(&qh_list, 0, sizeof(qh_list));
	qh_list.qh_link = swap_32(physaddr | QH_LINK_TYPE_QH);
	qh_list.qh_endpt1 = swap_32((1 << 15) | (USB_SPEED_HIGH << 12));
	qh_list.qh_curtd = swap_32(QT_NEXT_TERMINATE);
	qh_list.qh_overlay.qt_next = swap_32(QT_NEXT_TERMINATE);
	qh_list.qh_overlay.qt_altnext = swap_32(QT_NEXT_TERMINATE);
	qh_list.qh_overlay.qt_token = swap_32(0x40);

    __asm__ __volatile__("sync");

	/* Set async. queue head pointer. */
    physaddr = get_physaddr((uint32_t)&qh_list);
    raddr = base + EHCI_REG_ASYNC;
    cvmx_write64_uint32(raddr, physaddr);

	/* Start the host controller. */
    raddr = base + EHCI_REG_CMD;
    rval = cvmx_read64_uint32(raddr);
    rval |= EHCI_CMD_RUN;
    cvmx_write64_uint32(raddr, rval);

	rootdev = 0;
	return 0;
}

/*
 * Destroy the appropriate control structures corresponding
 * the the EHCI host controller.
 */
int
ehci_usb_lowlevel_stop(uint32_t port_n)
{
    uint64_t base, raddr;
    uint32_t rval;

    base = (uint64_t)OCTEON_EHCI_REG_BASE;

	if (hcor == NULL)
		return (0);

	if (port_n > 2) {
		port_n = 0;
	}
	if (port_n) {
        raddr = base + EHCI_REG_PORTSC + ((port_n - 1) * 4);
        rval = cvmx_read64_uint32(raddr);
	} else {
		/* Stop the controller. */
        raddr = base + EHCI_REG_CMD;
        rval = cvmx_read64_uint32(raddr);
        rval &= ~EHCI_CMD_RUN;
        cvmx_write64_uint32(raddr, rval);
	}

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
		td->qt_buffer_hi[idx] = 0;
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
wait_for_intr(uint64_t base)
{
    volatile uint32_t rval = 0x0;
    uint64_t raddr;
    volatile uint32_t x = 0;

    raddr = base + EHCI_REG_STS;
    /*Wait for transaction complete interrupt*/
    do {
        rval = cvmx_read64_uint32(raddr);
        udelay(20);
        x++;
        if (x > 0xf0000UL) {
            printf("Transaction timed out !!");
            return -1;
        }
    } while (!((rval & EHCI_STS_EINTR) || (rval & EHCI_STS_INTR)));
    
    raddr = base + EHCI_REG_STS;
    rval = cvmx_read64_uint32(raddr);
    rval |= ((EHCI_STS_IOAA) | (EHCI_STS_EINTR) | (EHCI_STS_INTR));
    cvmx_write64_uint32(raddr, rval);
    
    return 0;
}

int
ehci_submit_async(struct usb_device *dev, unsigned long pipe, void *buffer,
                  int length, struct devrequest *req)
{
    int stat;
	struct QH *qh;
	struct qTD *td;
	volatile struct qTD *vtd;
	unsigned long ts;
	char *s;
	uint32_t *tdp, sts;
	uint32_t endpt, token, usbsts, usbcmd;
	uint32_t c, toggle;
	uint32_t timeout, rval, physaddr;
    uint64_t base = (uint64_t)OCTEON_EHCI_REG_BASE, raddr;

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

    /* Clear any existing IOAA, USBERRINT and USBINT */
    raddr = base + 0x14;
    rval = cvmx_read64_uint32(raddr);
    rval |= ((EHCI_STS_IOAA) | (EHCI_STS_EINTR) | (EHCI_STS_INTR));
    cvmx_write64_uint32(raddr, rval);

	qh = ehci_alloc(sizeof(struct QH), 32);
	if (qh == NULL) {
		DBG("unable to allocate QH");
		return (-1);
	}

    physaddr = get_physaddr((uint32_t)&qh_list);
    qh->qh_link = swap_32(physaddr | QH_LINK_TYPE_QH);

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


    __asm__ __volatile__("sync");

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
        
        __asm__ __volatile__("sync");

		if (ehci_td_buffer(td, get_physaddr(req), sizeof(*req)) != 0) {
			DBG("unable construct SETUP td");
			ehci_free(td, sizeof(*td));
			goto fail;
		}

        physaddr = get_physaddr((uint32_t)td);
		*tdp = swap_32(physaddr);
        __asm__ __volatile__("sync");

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

		if (ehci_td_buffer(td, get_physaddr(buffer), length) != 0) {
			DBG("unable construct DATA td");
			ehci_free(td, sizeof(*td));
			goto fail;
		}
        __asm__ __volatile__("sync");

        physaddr = get_physaddr((u_int32_t)td);
        *tdp = swap_32(physaddr);
        __asm__ __volatile__("sync");

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

        physaddr = get_physaddr((uint32_t)td);
		*tdp = swap_32(physaddr);
        __asm__ __volatile__("sync");

		tdp = &td->qt_next;
	}

    physaddr = get_physaddr(qh);
	qh_list.qh_link = swap_32(physaddr | QH_LINK_TYPE_QH);
    __asm__ __volatile__("sync");

	/* Enable async. schedule. */
    raddr = base + EHCI_REG_CMD;
    rval = cvmx_read64_uint32(raddr);
	rval |= EHCI_CMD_ASE;
    cvmx_write64_uint32(raddr, rval);

    raddr = base + EHCI_REG_STS;
    do {
        rval = cvmx_read64_uint32(raddr);
    } while (!(rval & EHCI_STS_ASE));

	/* Wait for TDs to be processed. */
    stat = wait_for_intr(base);
    if (stat < 0) {
        printf("CTL:TIMEOUT ");
        stat = USB_ST_CRC_ERR;
    }

	/* Disable async schedule. */
    raddr = base + EHCI_REG_CMD;
    rval = cvmx_read64_uint32(raddr);
	rval &= ~EHCI_CMD_ASE;
    cvmx_write64_uint32(raddr, rval);

    raddr = base + EHCI_REG_STS;
    do {
        rval = cvmx_read64_uint32(raddr);
    } while (rval & EHCI_STS_ASE);

    if (stat == USB_ST_CRC_ERR) {
		dev->status = USB_ST_CRC_ERR;
		dev->act_len = 0;
        physaddr = get_physaddr((uint32_t)&qh_list);
        qh_list.qh_link = swap_32(physaddr | QH_LINK_TYPE_QH);
        __asm__ __volatile__("sync");
        goto fail;
    }

    physaddr = get_physaddr((uint32_t)&qh_list);
	qh_list.qh_link = swap_32(physaddr | QH_LINK_TYPE_QH);
    __asm__ __volatile__("sync");

	ts = get_timer(0);
	vtd = td;
    __asm__ __volatile__("sync");
	do {
		token = swap_32(vtd->qt_token);
		if (!(token & 0x80)) {
			break;
        }
	} while (get_timer(ts) < timeout);

    token = swap_32(qh->qh_overlay.qt_token);
	if (!(token & 0x80)) {
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
	uint8_t tmpbuf[4], port;
	void *srcptr;
	int len, srclen;
	uint32_t reg;
    uint64_t base, raddr;

	srclen = 0;
	srcptr = NULL;
    base = (uint64_t)OCTEON_EHCI_REG_BASE;

	DBG("req=%u (%#x), type=%u (%#x), value=%u, index=%u",
	    req->request, req->request,
	    req->requesttype, req->requesttype,
	    swap_16(req->value), swap_16(req->index));

#define C(a,b)	(((b) << 8) | (a))

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
        port = swap_16(req->index) - 1;
        raddr = base + EHCI_REG_PORTSC + (port * 4);
        reg = cvmx_read64_uint32(raddr);
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
        port = swap_16(req->index) - 1;
        raddr = base + EHCI_REG_PORTSC + (port * 4);
        reg = cvmx_read64_uint32(raddr);
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

            port = swap_16(req->index) - 1;
            raddr = base + EHCI_REG_PORTSC + (port * 4);
            cvmx_write64_uint32(raddr, reg);

			/* Wait for reset to complete. */
			udelay(250000);
			/* Terminate reset sequence. */
			reg &= ~EHCI_PS_PR;

            cvmx_write64_uint32(raddr, reg);

			/* Wait for HC to complete reset. */
			udelay(2000);

            reg = cvmx_read64_uint32(raddr);
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

        port = swap_16(req->index) - 1;
        raddr = base + EHCI_REG_PORTSC + (port * 4);
        cvmx_write64_uint32(raddr, reg);
		break;
	case C(USB_REQ_CLEAR_FEATURE, USB_DIR_OUT | USB_RT_PORT):
        port = swap_16(req->index) - 1;
        raddr = base + EHCI_REG_PORTSC + (port * 4);
        reg = cvmx_read64_uint32(raddr);
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
        port = swap_16(req->index) - 1;
        raddr = base + EHCI_REG_PORTSC + (port * 4);
        cvmx_write64_uint32(raddr, reg);
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
ehci_submit_bulk_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
    int length)
{

	if (usb_pipetype(pipe) != PIPE_BULK) {
		DBG("non-bulk pipe (type=%lu)", usb_pipetype(pipe));
		return (-1);
	}
	return (ehci_submit_async(dev, pipe, buffer, length, NULL));
}

int
ehci_submit_control_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
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

	return ehci_submit_async(dev, pipe, buffer, length, setup);
}

int
ehci_submit_int_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
    int length, int interval)
{

	DBG("dev=%p, pipe=%lu, buffer=%p, length=%d, interval=%d", dev, pipe,
	    buffer, length, interval);
	return (-1);
}

#endif /* CONFIG_USB_EHCI */
