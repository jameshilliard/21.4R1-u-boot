/*
 * Copyright (c) 2011-2013, Juniper Networks, Inc.
 * All rights reserved.
 *
 * (C) Copyright 2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>

/************  tsec for EX45XX ***************/

/*
 * Ethernet test
 *
 * The eTSEC controller  listed in ctlr_list array below
 * are tested in the internal and external loopback  mode.
 * The controllers are configured accordingly and several packets
 * are transmitted and recevied back both in internal and external loopback.
 * The configurable test parameters are:
 * MIN_PACKET_LENGTH - minimum size of packet to transmit
 * MAX_PACKET_LENGTH - maximum size of packet to transmit
 * TEST_NUM - number of tests
 */
#include <miiphy.h>
#include <malloc.h>
#include <net.h>
#include <command.h>
#include <fsl_mdio.h>
#include <asm/fsl_enet.h>
#include <post.h>
#include <tsec.h>
#include "lcd.h"
#include <ex45xx_local.h>
#include "ether.h"


DECLARE_GLOBAL_DATA_PTR;

struct tsec_device {
    char *name;
    u32 capability;
};

#define write_phy_reg(priv, regnum, value) \
	tsec_local_mdio_write(priv->regs, priv->phyaddr, 0, regnum, value)

#define read_phy_reg(priv,regnum) \
	tsec_local_mdio_read(priv->regs, priv->phyaddr, 0, regnum)

/* JUNOS Start */
#ifdef CONFIG_EX45XX
void write_phy_reg_private(struct tsec_private *priv, uint reg, uint val)
{
	write_phy_reg(priv, reg, val);
}

uint read_phy_reg_private(struct tsec_private *priv, uint regnum)
{
	uint val;
	val = read_phy_reg(priv,regnum);
	return val;
}
#endif
/* JUNOS End */

struct tsec_device etsec_dev[] =
    {
     {CONFIG_TSEC1_NAME, 1<<LB_MAC | 1<<LB_PHY1145},
     {CONFIG_TSEC2_NAME, 1<<LB_MAC | 1<<LB_PHY1145},
     {CONFIG_TSEC3_NAME, 1<<LB_MAC | 1<<LB_PHY1145},
/* JUNOS Start */
#ifdef CONFIG_EX4500
     {CONFIG_TSEC4_NAME, 1<<LB_MAC | 1<<LB_PHY1145 |
        1<<LB_PHY1112 |1<<LB_EXT_10 |1<<LB_EXT_100 | 1<<LB_EXT_1000},
#endif
/* JUNOS End */
     {NULL , 0}
    };

/* JUNOS Start */
#ifdef CONFIG_EX4500
struct tsec_device post_etsec_dev[] =
    {{CONFIG_TSEC4_NAME, LB_MAC | 1<<LB_PHY1145 |1<<LB_PHY1112},
     {NULL , 0}
    };
#endif
/* JUNOS End */

/* Ethernet Transmit and Receive Buffers */
#define TX_BUF_CNT 2

typedef struct _loopback_status_field {
    uint32_t loopType : 4;  /* loopback type */
    uint32_t pktSize  :12;  /* packet size */
    uint32_t TxErr    : 8;  /* Tx Error */
    uint32_t RxErr    : 8;  /* Rx Error */
} loopback_status_field;

typedef union _loopback_status {
    loopback_status_field field;
    uint32_t status;
} loopback_status;

static uint rxIdx;		/* index of the current RX buffer */
static uint txIdx;		/* index of the current TX buffer */

/* JUNOS Start */
/*
 * There is an errata for the etsec version which is present on EX4500.
 * The errata is not required on EX4510.
 */ 
#ifdef CONFIG_EX4500
static int toggle_rx_errata = 1;
#else
static int toggle_rx_errata = 0;
#endif
/* JUNOS End */
static int ether_debug_flag;
static int boot_flag_post;
int post_result_ether;
loopback_status lb_status;
/* JUNOS Start */
#ifdef CONFIG_EX4500
extern struct tsec_private *privlist1[];
#endif
/* JUNOS End */

/*
 * TSEC Ethernet Tx and Rx buffer descriptors allocated at the
 *  immr->udata_bd address on Dual-Port RAM
 * Provide for Double Buffering
 */
typedef volatile struct rtxbd {
	txbd8_t txbd[TX_BUF_CNT];
	rxbd8_t rxbd[PKTBUFSRX];
}  RTXBD;

#ifdef __GNUC__
static RTXBD rtx __attribute__ ((aligned(8)));
#else
#error "rtx must be 64-bit aligned"
#endif
char mode_test_string[MAX_LB_TYPES][40] = 
     {{"Mac Loopback"},
	{"Phy 1145 Loopback"},
	{"Phy 1112 Loopback"},
	{"Ext Loopback 10   Mbps Full Duplex"},
	{"Ext Loopback 100  Mbps Full Duplex"},
	{"Ext Loopback 1000 Mbps Full Duplex"}
     };
int ether_etsec_send(struct eth_device *dev,
							volatile void *packet, int length);
int ether_etsec_recv(struct eth_device *dev, void *packet, int
							len);
int post_log (char *format, ...)
{
	va_list args;
	uint i;
	char printbuffer[CONFIG_SYS_PBSIZE];

	va_start (args, format);

	/* For this to work, printbuffer must be larger than
	 * anything we ever want to print.
	 */
	i = vsprintf (printbuffer, format, args);
	va_end (args);

#ifdef CONFIG_LOGBUFFER
	/* Send to the logbuffer */
	logbuff_log (printbuffer);
#else
	/* Send to the stdout file */
	puts (printbuffer);
#endif

	return 0;
}

static char *
rx_error_msg (uint16_t error_code)
{
    static char errorlist[100];	/* returned as result */
    struct rxbd_bit_items_s {
        uint16_t rxbd_bit_mask;
        const char *item;
    } rxbd_bit_list[] = { { RXBD_TRUNCATED, "truncated, " },
                          { RXBD_OVERRUN,   "over run, "  },
                          { RXBD_CRCERR,    "crc, "       },
                          { RXBD_SHORT,     "short, "     },
                          { RXBD_NONOCTET,  "non-octet, " },
                          { RXBD_LARGE,     "large, "     } };
    int entries = sizeof(rxbd_bit_list) / sizeof(struct rxbd_bit_items_s);
    char *cptr = &errorlist[0], *lastp = &errorlist[100];
    int i;

    *cptr = '\0';		/* reset for this pass */

    for (i = 0; i < entries; i++) {
        if (error_code & rxbd_bit_list[i].rxbd_bit_mask) {
	    size_t len = strlen(rxbd_bit_list[i].item);

	    /* Ensure we have enough room */
	    if ((&cptr[len]) > lastp) {
		cptr -= 2;	/* remove ", " */
		*cptr = '\0';
		return (errorlist);
	    }
	    strcpy(cptr, rxbd_bit_list[i].item);
	    cptr += len;
        }
    }

    if (cptr != errorlist) {
	cptr -= 2;	/* remove ", " */
	*cptr = '\0';
    }
    return (errorlist);
}

static void 
toggle_tsec_rx(struct eth_device *dev)
{
    struct tsec_private *priv = (struct tsec_private *)dev->priv;
    volatile tsec_t *regs = priv->regs;

    regs->maccfg1 &= ~MACCFG1_RX_EN;
    regs->maccfg1 |= MACCFG1_RX_EN;
}

/*
 * etsec transmit routine
 */
int 
ether_etsec_send (struct eth_device *dev, volatile void *packet, int length)
{
    int i;
    int result = 0;
    struct tsec_private *priv = (struct tsec_private *)dev->priv;
    volatile tsec_t *regs = priv->regs;

    /* Find an empty buffer descriptor */
    for (i = 0; rtx.txbd[txIdx].status & TXBD_READY; i++) {
        if (i >= TOUT_LOOP) {
            post_log("%s: tx (buffers full)\n", dev->name);
            lb_status.field.TxErr |= 0x1;
            return (result);
        }
    }

    rtx.txbd[txIdx].bufPtr = (uint) packet;
    rtx.txbd[txIdx].length = length;
    rtx.txbd[txIdx].status |=
        (TXBD_READY | TXBD_LAST | TXBD_CRC | TXBD_INTERRUPT);

    /* Tell the DMA to go */
    regs->tstat = TSTAT_CLEAR_THALT;

    /* Wait for buffer to be transmitted */
    for (i = 0; rtx.txbd[txIdx].status & TXBD_READY; i++) {
        if (i >= TOUT_LOOP) {
            post_log("%s: tx (timeout)\n", dev->name);
            lb_status.field.TxErr |= 0x2;
            return (result);
        }
    }

    txIdx = (txIdx + 1) % TX_BUF_CNT;
    result = rtx.txbd[txIdx].status & TXBD_STATS;

    return (result);
}

/*
 * etsec recv routine
 */
int 
ether_etsec_recv(struct eth_device *dev, void *recv_pack, int max_len)
{
    int length = 0;
    struct tsec_private *priv = (struct tsec_private *)dev->priv;
    volatile tsec_t *regs = priv->regs;

    udelay(1000);	
    while(!(rtx.rxbd[rxIdx].status & RXBD_EMPTY)) {

        length = rtx.rxbd[rxIdx].length + length;

        /* Send the packet up if there were no errors */
        if (!(rtx.rxbd[rxIdx].status & RXBD_STATS)) {
            length = length - 4;

            memcpy(recv_pack, (void*) NetRxPackets[rxIdx], length < max_len
                   ? length : max_len); 

        } 
        else {
            post_log("%s: rx error (%s)\n", dev->name,
                     rx_error_msg(rtx.rxbd[rxIdx].status & RXBD_STATS));
            lb_status.field.RxErr = rtx.rxbd[rxIdx].status & RXBD_STATS;
        }

        rtx.rxbd[rxIdx].length = 0;

        /* Set the wrap bit if this is the last element in the list */
        rtx.rxbd[rxIdx].status = 
                RXBD_EMPTY | (((rxIdx + 1) == PKTBUFSRX) ? RXBD_WRAP : 0);

        rxIdx = (rxIdx + 1) % PKTBUFSRX;
    }
    if (regs->ievent & IEVENT_BSY) {
        regs->ievent = IEVENT_BSY;
        regs->rstat = RSTAT_CLEAR_RHALT;
    }
    return (length);
}

/* Initializes data structures and registers for the controller,
 * and brings the interface up.	 Returns the link status, meaning
 * that it returns success if the link is up, failure otherwise.
 * This allows u-boot to find the first active controller.
 */
int 
init_tsec (struct eth_device *dev)
{
    uint tempval;
    char tmpbuf[MAC_ADDR_LEN];
    int i;
    struct tsec_private *priv = (struct tsec_private *)dev->priv;
    volatile tsec_t *regs = priv->regs;
    
    /* Init MACCFG2.  Defaults to GMII */
    regs->maccfg2 = MACCFG2_INIT_SETTINGS;

    /* Init ECNTRL */
    regs->ecntrl = ECNTRL_INIT_SETTINGS;

    /* Copy the station address into the address registers.
     * Backwards, because little endian MACS are dumb 
     */
     for (i = 0; i < MAC_ADDR_LEN; i++) {
        tmpbuf[MAC_ADDR_LEN - 1 - i] = dev->enetaddr[i];
     };
     regs->macstnaddr1 = *((uint *) (tmpbuf));

     tempval = *((uint *) (tmpbuf + 4));

     regs->macstnaddr2 = tempval;

     /* reset the indices to zero */
     rxIdx = 0;
     txIdx = 0;

     /* Clear out (for the most part) the other registers */
     init_registers_private(regs);

     return (1);
}

/* Set up the buffers and their descriptors, and bring up the
 * interface
 */
void 
start_tsec (struct eth_device *dev)
{
    int i;
    struct tsec_private *priv = (struct tsec_private *)dev->priv;
    volatile tsec_t *regs = priv->regs;

    /* Point to the buffer descriptors */
    regs->tbase = (unsigned int)(&rtx.txbd[txIdx]);
    regs->rbase = (unsigned int)(&rtx.rxbd[rxIdx]);

    /* Initialize the Rx Buffer descriptors */
    for (i = 0; i < PKTBUFSRX; i++) {
        rtx.rxbd[i].status = RXBD_EMPTY;
        rtx.rxbd[i].length = 0;
        rtx.rxbd[i].bufPtr = (uint) NetRxPackets[i];
    }
    rtx.rxbd[PKTBUFSRX - 1].status |= RXBD_WRAP;

    /* Initialize the TX Buffer Descriptors */
    for (i = 0; i < TX_BUF_CNT; i++) {
        rtx.txbd[i].status = 0;
        rtx.txbd[i].length = 0;
        rtx.txbd[i].bufPtr = 0;
    }
    rtx.txbd[TX_BUF_CNT - 1].status |= TXBD_WRAP;

    /* Enable Transmit and Receive */
    regs->maccfg1 |= (MACCFG1_RX_EN | MACCFG1_TX_EN);

    /* Tell the DMA it is clear to go */
    regs->dmactrl |= DMACTRL_INIT_SETTINGS;
    regs->tstat = TSTAT_CLEAR_THALT;
    regs->dmactrl &= ~(DMACTRL_GRS | DMACTRL_GTS);
}

/* Stop the interface */
void 
stop_tsec (struct eth_device *dev)
{
    struct tsec_private *priv = (struct tsec_private *)dev->priv;
    volatile tsec_t *regs = priv->regs;

    regs->dmactrl &= ~(DMACTRL_GRS | DMACTRL_GTS);
    regs->dmactrl |= (DMACTRL_GRS | DMACTRL_GTS);

    while (!(regs->ievent & (IEVENT_GRSC | IEVENT_GTSC))) ;

    regs->maccfg1 &= ~(MACCFG1_TX_EN | MACCFG1_RX_EN | MACCFG1_LOOPBACK);
}

/*
 * Test routines
 */
static void 
packet_fill (char *packet, int length)
{
    char c = (char) length;
    int i;

    packet[0] = 0xFF;
    packet[1] = 0xFF;
    packet[2] = 0xFF;
    packet[3] = 0xFF;
    packet[4] = 0xFF;
    packet[5] = 0xFF;

    for (i = 6; i < length; i++) {
        packet[i] = c++;
    }
}

static int 
packet_check (char *packet, int length)
{
    char c = (char) length;
    int i;
    
    for (i = 6; i < length; i++) {
        if (packet[i] != c++) {
            if (ether_debug_flag) {
                post_log("\nPacket size %d,"
                         " data received/expected at location %d ="
                         " %02x/%02x\n",
                         length, i, packet[i], c-1);
                post_log("\nPacket check contents are...\n");
                for (i = 0; i < length; i++) {
                    if ((i != 0) && ((i % 16) == 0))
                        post_log("\n");
                    post_log("%02x ", packet[i]);
                }
                post_log("\n");
            }
            return (-1);
        }
    }
    return (0);
}

static void 
mac_loopback (struct eth_device *dev, int enable, int *link)
{
    struct tsec_private *priv = (struct tsec_private *)dev->priv;
    volatile tsec_t *regs = priv->regs;
    uint32_t value;
    
    if (enable) {
        value = regs->maccfg1;
        value |= MACCFG1_LOOPBACK;
        regs->maccfg1 = value;
        *link = 1;
    }
    else {
        value = regs->maccfg1;
        value &= ~MACCFG1_LOOPBACK;
        regs->maccfg1 = value;
        *link = 0;
    }
}

static void 
phy_1112_loopback (struct eth_device *dev, int enable, int *link)
{
    struct tsec_private *priv = (struct tsec_private *)dev->priv;
    struct tsec_private *priv1;
    volatile struct tsec_mii_mng *regs = priv->phyregs_sgmii;

/* JUNOS Start */
#ifdef CONFIG_EX4500
    if (priv->index != 3)
        return;
   
    priv1 = privlist1[priv->index];  /* PHY 1145 */
    
    write_phy_reg(priv1, 22, 0);
    if (enable) {
        mdio_mode_set(0, 1);  /* PHY 1145 */
        /* PHY 1145 disable auto-negotiation */
        write_phy_reg(priv1, 0, 0x8140);
        udelay(30000);

        mdio_mode_set(2, 1);  /* PHY 1112 */
        write_phy_reg(priv, 22, 2);
        /* PHY 1112 disable SGMII auto-negotiation */
        write_phy_reg(priv, 0, 0x8040);
        
        write_phy_reg(priv, 22, 0);
        /* PHY 1112 disable auto-negotiation */
        write_phy_reg(priv, 0, 0x8140);
        udelay(30000);
        /* set phy 1112 loop back mode */
        write_phy_reg(priv, 0, 0x4140);
        *link = 1;
    }
    else {
        mdio_mode_set(0, 1);  /* PHY 1145 */
        /* PHY 1145 enable auto-negotiation */
        write_phy_reg(priv1, 0, 0x9140);
        udelay(30000);

        mdio_mode_set(2, 1);  /* PHY 1112 */
        write_phy_reg(priv, 22, 2);
        /* PHY 1112 enable SGMII auto-negotiation */
        write_phy_reg(priv, 0, 0x9040);
        
        write_phy_reg(priv, 22, 0);
        /* clear phy 1112 loop back mode */
        write_phy_reg(priv, 0, 0x9140);
        udelay(30000);
        *link = 0;
    }

    priv->phyregs_sgmii->miimcfg = MIIMCFG_INIT_VALUE;
    while (priv->phyregs_sgmii->miimind & MIIMIND_BUSY) ;

    mdio_mode_set(0, 0);  /* PHY 1145 */

#endif /* CONFIG_EX4500 */

#ifdef CONFIG_EX4510
	if (enable) {
		/* disable auto-negotiation */
		write_phy_reg(priv, 0, 0x8140);
		udelay(30000); /* 30 ms */
		/*  Set phy loop back mode */
		write_phy_reg(priv, 0, 0x4140);
		*link = 1;
	} else {
		write_phy_reg(priv, 0, 0x9140);
		udelay(30000); /* 30 ms */
		*link = 0;
	}

	regs->miimcfg = MIIMCFG_INIT_VALUE;
	while (regs->miimind & MIIMIND_BUSY);
#endif /* CONFIG_EX4510 */
}

#if CONFIG_EX4500
static void 
phy_1145_loopback (struct eth_device *dev, int enable, int *link)
{
    struct tsec_private *priv = (struct tsec_private *)dev->priv;
    volatile struct tsec_mii_mng *regs;

    if (priv->index == 3) {
        priv = privlist1[priv->index];
    }
    regs = priv->phyregs_sgmii;
    
    mdio_mode_set(0, 1);  /* PHY 1145 */
    write_phy_reg(priv, 22, 0);
    if (enable) {
        /* disable auto-negotiation */
        write_phy_reg(priv, 0, 0x8140);
        udelay(30000);
        /*  Set phy loop back mode */
        write_phy_reg(priv, 0, 0x4140);
        *link = 1;
    }
    else {
        write_phy_reg(priv, 0, 0x9140);
        udelay(30000);
        *link = 0;
    }

    regs->miimcfg = MIIMCFG_INIT_VALUE;
    while (regs->miimind & MIIMIND_BUSY);

    mdio_mode_set(0, 0);  /* PHY 1145 */
}
#endif
/* JUNOS End */

static void 
ext_loopback (struct eth_device *dev, int enable, int speed, int *link)
{
    volatile uint16_t regVal;
    volatile uint16_t mii_reg;
    struct tsec_private *priv = (struct tsec_private *)dev->priv;
    volatile tsec_t *regs = priv->regs;
/* JUNOS Start */
#ifdef CONFIG_EX4500
    struct tsec_private *priv1 = NULL;

    priv1 = privlist1[priv->index];  /* PHY 1145 */

    mdio_mode_set(2, 1);  /* PHY 1112 */
    write_phy_reg(priv, 22, 0);
    if (enable) {
        if (speed == 0) {  /* 10M */
            write_phy_reg(priv, 22, 0);
            write_phy_reg(priv, 0, 0x8100);
        }
        else if (speed == 1) {  /* 100M */
            write_phy_reg(priv, 22, 0);
            write_phy_reg(priv, 0, 0xa100);
        }
        else if (speed == 2) {  /* 1G */
            /* Full duplex 1 G-bps setting in reg 0 */
            write_phy_reg(priv, 0, 0x8140);
            write_phy_reg(priv, 22, 6);
            regVal = read_phy_reg(priv, 16);
            regVal |= 0x20;
            write_phy_reg(priv, 16, regVal);
            write_phy_reg(priv, 22, 0);
        }

        *link = 1;
        /* check PHY 1112 link */
        write_phy_reg(priv, 22, 0);
        mii_reg = read_phy_reg(priv, 0x11);

        priv->phyregs_sgmii->miimcfg = MIIMCFG_INIT_VALUE;
        while (priv->phyregs_sgmii->miimind & MIIMIND_BUSY);

        if (!((mii_reg & MIIM_88E1011_PHYSTAT_SPDDONE) &&
             (mii_reg & MIIM_88E1011_PHYSTAT_LINK))) {
            int i = 0;

            udelay(30000);

            while (!((mii_reg & MIIM_88E1011_PHYSTAT_SPDDONE) &&
                    (mii_reg & MIIM_88E1011_PHYSTAT_LINK))) {
                /*
                 * Timeout reached ?
                 */
                if (i > PHY_AUTONEGOTIATE_TIMEOUT) {
                    *link = 0;
                    break;
                }
                i++;

                udelay(1000);  /* 1 ms */
                write_phy_reg(priv, 22, 0);
                mii_reg = read_phy_reg(priv, MIIM_88E1011_PHY_STATUS);
            }
        }

        mdio_mode_set(0, 1);  /* PHY 1145 */
        if (*link == 1) {
            write_phy_reg(priv1, 22, 0);
            if (speed == 0) {  /* 10M */
                regs->maccfg2 = ((regs->maccfg2 & ~(MACCFG2_IF)) |MACCFG2_MII);
                regs->ecntrl &= ~(ECNTRL_R100);
            }
            else if (speed == 1) {  /* 100M */
                regs->maccfg2 = ((regs->maccfg2 & ~(MACCFG2_IF)) |MACCFG2_MII);
                regs->ecntrl |= ECNTRL_R100;
            }
            else if (speed == 2) {  /* 1G */
                regs->maccfg2 = ((regs->maccfg2 & ~(MACCFG2_IF))|MACCFG2_GMII);
            }
            udelay(500000);
        }
        else {
            mdio_mode_set(0, 0);  /* PHY 1145 */
            return;
        }
    }
    else{
        mdio_mode_set(2, 1);  /* PHY 1112 */
        write_phy_reg(priv, 22, 0);
        write_phy_reg(priv,0,0x9140);	
        if (speed == 2) {
            write_phy_reg(priv, 22, 6);
            regVal = read_phy_reg(priv, 16);
            regVal &= ~0x20;
            write_phy_reg(priv, 16, regVal);	
            write_phy_reg(priv, 22, 0);
        }
        mdio_mode_set(0, 1);  /* PHY 1145 */
        write_phy_reg(priv1, 22, 0);
        write_phy_reg(priv1, 0, 0x9140);
        regs->maccfg1 = 0;
        regs->maccfg2 = MACCFG2_INIT_SETTINGS;
        regs->ecntrl = ECNTRL_INIT_SETTINGS;
        *link = 0;
    }

    udelay(30000);

    priv->phyregs_sgmii->miimcfg = MIIMCFG_INIT_VALUE;
    while (priv->phyregs_sgmii->miimind & MIIMIND_BUSY);

    mdio_mode_set(0, 0);  /* PHY 1145 */
#endif /* CONFIG_EX4500 */

#ifdef CONFIG_EX4510
	if (enable) {
		if (speed == 0) { /* 10M */
			write_phy_reg(priv, 0x0, 0x0100);
			write_phy_reg(priv, 0x18,0x8400);
		} else if (speed == 1) {  /* 100M */
			write_phy_reg(priv, 0x0, 0x2100);
			write_phy_reg(priv, 0x18,0x8400);
			write_phy_reg(priv, 0x18,0x0014);
		} else if (speed == 2) {  /* 1G */
			/* Full duplex 1 G-bps setting in reg 0 */
			write_phy_reg(priv, 0x9, 0x1800);
			write_phy_reg(priv, 0x0, 0x0040);
			write_phy_reg(priv, 0x18, 0x8400);
			write_phy_reg(priv, 0x18, 0x0014);
		}

		*link = 1;
		mii_reg = read_phy_reg(priv, 0x19);

		priv->phyregs_sgmii->miimcfg = MIIMCFG_INIT_VALUE;
		while (priv->phyregs_sgmii->miimind & MIIMIND_BUSY);
		
		udelay(500000); /* 500 ms */

		if (!((mii_reg & MIIM_88E1011_PHYSTAT_SPDDONE) &&
			 (mii_reg & MIIM_88E1011_PHYSTAT_LINK))) {
			int i = 0;

			udelay(30000); /* 30 ms */

			while (!((mii_reg & MIIM_88E1011_PHYSTAT_SPDDONE) &&
				(mii_reg & MIIM_88E1011_PHYSTAT_LINK))) {

				/* Timeout reached ? */
				if (i > PHY_AUTONEGOTIATE_TIMEOUT) {
					*link = 0;
					break;
				}
				i++;

				udelay(1000);  /* 1 ms */
				write_phy_reg(priv, 22, 0);
				mii_reg = read_phy_reg(priv, MIIM_88E1011_PHY_STATUS);
			}
		}

		if (*link == 1) {
			if (speed == 0) {  /* 10M */
				regs->maccfg2 = ((regs->maccfg2 & ~(MACCFG2_IF)) |MACCFG2_MII);
				//regs->ecntrl &= ~(ECNTRL_R100);
				regs->ecntrl = 0x1022;
			} else if (speed == 1) {  /* 100M */
				regs->maccfg2 = ((regs->maccfg2 & ~(MACCFG2_IF)) |MACCFG2_MII);
				regs->ecntrl = 0x102a;
			} else if (speed == 2) {  /* 1G */
				//regs->maccfg2 = ((regs->maccfg2 & ~(MACCFG2_IF))|MACCFG2_GMII);
				regs->maccfg2 = 0x7205;
				regs->ecntrl = 0x1022;
			}
			udelay(500000); /* 50 ms */
		} else {
			return;
		}
	} else {
		write_phy_reg(priv, 0, 0x9140);
		regs->maccfg1 = 0;
		regs->maccfg2 = MACCFG2_INIT_SETTINGS;
		regs->ecntrl = ECNTRL_INIT_SETTINGS;
		*link = 0;
	}
	udelay(30000); /* 30 ms*/

	priv->phyregs_sgmii->miimcfg = MIIMCFG_INIT_VALUE;
	while (priv->phyregs_sgmii->miimind & MIIMIND_BUSY);
#endif /* CONFIG_EX4510 */
/* JUNOS End */
}

/*
 *This routine checks for mac,phy and external loop back mode
 */
int 
ether_loopback (struct eth_device *dev, int loopback,
               int max_pkt_len, int debug_flag, int reg_dump) 
{
    int res = -1;
    char packet_send[MAX_PACKET_LENGTH];
    char packet_recv[MAX_PACKET_LENGTH];
    int length = 0;
    int i = 0;
    int link;

    if (max_pkt_len > MAX_PACKET_LENGTH)
        max_pkt_len = MAX_PACKET_LENGTH;

    lb_status.status = 0;
    lb_status.field.loopType = loopback;
    lb_status.field.pktSize = 0;
    lb_status.field.TxErr = 0;
    lb_status.field.RxErr = 0;

    init_tsec(dev);

    /* setup loopback */
    if (loopback == LB_MAC) {
        mac_loopback(dev, 1, &link);
    }
    else if (loopback == LB_PHY1145) {
/* JUNOS Start */
#ifdef CONFIG_EX4500
        phy_1145_loopback(dev, 1, &link);
#endif
/* JUNOS End */
    }
    else if (loopback == LB_PHY1112) {
        phy_1112_loopback(dev, 1, &link);
    }
    else if (loopback == LB_EXT_10) {
        ext_loopback(dev, 1, 0, &link);
    }
    else if (loopback == LB_EXT_100) {
        ext_loopback(dev, 1, 1, &link);
    }
    else if (loopback == LB_EXT_1000) {
        ext_loopback(dev, 1, 2, &link);
    }

    if (link == 0)
        goto Done;
    
    start_tsec(dev);
       
    /* 8544 Rev 1.0 errata eTSEC 27: rx_en synchronization may cause 
     * unrecoverable RX error at startup.  Workaround is to test one loopback
     * packet.  If failed, toggle rx_enbit.
     */
    if (toggle_rx_errata) {
        for (i = 0; i < 5; i++) {
            packet_fill (packet_send, 64);
            ether_etsec_send(dev,packet_send, 64);
            length = ether_etsec_recv(dev, packet_recv, 64);
            if (length == 0) {
                toggle_tsec_rx(dev);
            }
            else {
                break;
            }
        }
    }
       
    for (i = MIN_PACKET_LENGTH; i <= max_pkt_len; i++) {
        lb_status.field.pktSize = i;
        packet_fill (packet_send, i);
        ether_etsec_send (dev,packet_send, i);
        length = ether_etsec_recv(dev, packet_recv, i);
        if (length != i || packet_check (packet_recv, length) < 0) {
            if (reg_dump) {
                tsec_phy_dump(dev->name);
                tsec_reg_dump(dev->name);
                mac_reg_dump(dev->name);
            }
            goto Done;
        }
    }
    res = 0;
    
Done:
    
    stop_tsec(dev);

    /* clear loopback */
    if (loopback == LB_MAC) {
        mac_loopback(dev, 0, &link);
    }
    else if (loopback == LB_PHY1145) {
/* JUNOS Start */
#ifdef CONFIG_EX4500
        phy_1145_loopback(dev, 0, &link);
#endif
/* JUNOS End */
    }
    else if (loopback == LB_PHY1112) {
        phy_1112_loopback(dev, 0, &link);
    }
    else if (loopback == LB_EXT_10) {
        ext_loopback(dev, 0, 0, &link);
    }
    else if (loopback == LB_EXT_100) {
        ext_loopback(dev, 0, 1, &link);
    }
    else if (loopback == LB_EXT_1000) {
        ext_loopback(dev, 0, 2, &link);
    }

    return (res);
}


/*
 * API to send cont pkts on the given i/f
 */
int tsec_cont_send_pkts(char *dev)
{
	struct eth_device *eth_dev = NULL;
	int res = -1;
	char packet_send[MAX_PACKET_LENGTH];
	char packet_recv[MAX_PACKET_LENGTH];
	int length = 0, max_pkt_len = MAX_PACKET_LENGTH;
	int i = 0;
	int link;
	int tot_dropped = 0;

	eth_dev = eth_get_dev_by_name(dev);
	lb_status.status = 0;
	lb_status.field.loopType = 0;
	lb_status.field.pktSize = 0;
	lb_status.field.TxErr = 0;
	lb_status.field.RxErr = 0;

	init_tsec(eth_dev);

	start_tsec(eth_dev);

	for (i = MIN_PACKET_LENGTH; i <= 1024; i++) {
	    lb_status.field.pktSize = i;
	    packet_fill(packet_send, i);
	    res = ether_etsec_send(eth_dev,packet_send, i);
	}

	stop_tsec(eth_dev);

	return 0;
}


/*
 * Test ctlr routine for etsec0 and etsec1
 */
static int 
test_loopback (struct tsec_device* eth_dev)
{
    struct eth_device *dev;
    int ether_result[NUM_TSEC_DEVICE][MAX_LB_TYPES];
    int i = 0;
    int j = 0;
    int fail = 0;
    loopback_status test_status[NUM_TSEC_DEVICE][MAX_LB_TYPES];

    memset(ether_result, 0, NUM_TSEC_DEVICE * MAX_LB_TYPES * sizeof(int));
    memset(ether_result, 0, NUM_TSEC_DEVICE * MAX_LB_TYPES * sizeof(loopback_status));

    for (i = 0; eth_dev[i].name; i++) {
        dev = eth_get_dev_by_name(eth_dev[i].name);
        for (j = 0; j < MAX_LB_TYPES; j++) {
            if (eth_dev[i].capability & (1 << j)) {
                if ((ether_result[i][j] = 
                    ether_loopback(dev, j, POST_MAX_PACKET_LENGTH,
                    ether_debug_flag, 0)) == -1) {
                    fail = 1;
                    test_status[i][j].status = lb_status.status;
                    test_status[i][j].field.loopType = lb_status.field.loopType;
                    test_status[i][j].field.pktSize = lb_status.field.pktSize;
                    test_status[i][j].field.TxErr = lb_status.field.TxErr;
                    test_status[i][j].field.RxErr = lb_status.field.RxErr;
                }
            }
        }
    }

    if (boot_flag_post ) {
        if (!fail) {
            lcd_printf_2("POST Eth pass..");
            return (0);
        }
        else {
            lcd_printf_2("POST Eth fail..");
            post_result_ether = -1;
            return (-1);
        }
    }

    /* tsec diag result printing */
    if (!fail) {
        post_log("-- TSEC test                            PASSED\n");
        if (ether_debug_flag) {
            for (i = 0; eth_dev[i].name; i++) {
                for (j = 0; j < MAX_LB_TYPES; j++) {
                     if (eth_dev[i].capability & (1 << j))
                        post_log(" > %s: %-35s %13s Passed\n",
                                 eth_dev[i].name, mode_test_string[j], "");
                }
            }
        }
    }
    else {
        post_log("-- TSEC test                            FAILED @\n");
        if (ether_debug_flag) {
            for (i = 0; eth_dev[i].name; i++) {
                for (j = 0; j < MAX_LB_TYPES; j++) {
                    if (eth_dev[i].capability & (1 << j)) {
                        if (ether_result[i][j] == -1) {
                            if (test_status[i][j].field.TxErr || (test_status[i][j].field.RxErr)) {
                                post_log(" > %s: %-35s (%04x, %02x/%02x) Failed @\n",
                                         eth_dev[i].name, mode_test_string[j],
                                         test_status[i][j].field.pktSize,
                                         test_status[i][j].field.TxErr,
                                         test_status[i][j].field.RxErr);
                            }
                            else {
                                post_log(" > %s: %-35s (no link)     Failed @\n",
                                         eth_dev[i].name, mode_test_string[j]);
                            }
                        }
                        else {
                            if (ether_debug_flag) {
                                post_log(" > %s: %-35s %13s Passed\n",
                                         eth_dev[i].name, mode_test_string[j], "");
                            }
                        }
                    }
                }
            }
        }
        else {
            for (i = 0; eth_dev[i].name; i++) {
                for (j = 0; j < MAX_LB_TYPES; j++) {
                    if (eth_dev[i].capability & (1 << j)) {
                        if (ether_result[i][j] == -1) {
                            if (test_status[i][j].field.TxErr || (test_status[i][j].field.RxErr)) {
                                post_log(" > %s: %-35s (%04x, %02x/%02x) Failed @\n",
                                         eth_dev[i].name, mode_test_string[j],
                                         test_status[i][j].field.pktSize,
                                         test_status[i][j].field.TxErr,
                                         test_status[i][j].field.RxErr);
                            }
                            else {
                                post_log(" > %s: %-35s (no link)     Failed @\n",
                                         eth_dev[i].name, mode_test_string[j]);
                            }
                        }
                    }
                }
            }
        }
    }

    if (!fail)
        return (0); 
    
    return (-1);
}	

/* JUNOS Start */
#ifdef CONFIG_EX4500
int 
ether_post_test (int flags)
{
    int res = 0;
    uint svr;
    boot_flag_post = 0;
    ether_debug_flag = 0;
    post_result_ether = 0;
	
    ether_debug_flag = 1;
    boot_flag_post = 1;
    
    /* 8544 Rev 1.0 errata eTSEC 27: rx_en synchronization may cause unrecoverable
     *  RX error at startup.  Workaround is to test one loopback packet.  If failed, toggle rx_en
     *  bit.  This problem will be fix in Rev 1.1.
     */
    svr = get_svr();
    if ((svr == 0x80390020) || (svr == 0x80310020) ||
        (svr == 0x80390021) || (svr == 0x80310021)) {
        toggle_rx_errata = 1;
    }

    if (!boot_flag_post) {
        if (ether_debug_flag) {
            post_log("This tests the functionality of eTSEC in MAC,PHY\n");
            post_log(" and external loopBack. \n");
            post_log(" Please insert a loopBack cable in the eTSEC0 port. \n\n");
        }
        res = test_loopback(etsec_dev);
    }
    else {
        res = test_loopback(post_etsec_dev);
    }

    return (res);
}

uint32_t 
diag_ether_loopback (int loopback)
{
    int i, result = 0;
    int loop_ctrl = LB_PHY1112 + 1;
    struct eth_device *dev = NULL;
    
    if (loopback)
        loop_ctrl = MAX_LB_TYPES;

    dev = eth_get_dev_by_name(post_etsec_dev[0].name);
    for (i = 0; i < loop_ctrl; i++) {
        if (ether_loopback(dev, i, MAX_PACKET_LENGTH, 0, 0)) {
            result = lb_status.status;
            break;
        }
    }

    return (result);
}
#endif
/* JUNOS End */
