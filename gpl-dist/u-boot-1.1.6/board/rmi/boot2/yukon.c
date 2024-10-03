/***********************************************************************
  Copyright 2003-2006 Raza Microelectronics, Inc.(RMI).
  This is a derived work from software originally provided by the external
  entity identified below. The licensing terms and warranties specified in
  the header of the original work apply to this derived work.
  Contribution by RMI: Adapted to XLS Bootloader.
  *****************************#RMI_1#**********************************/

/*
 * New driver for Marvell Yukon 2 chipset.
 * Based on earlier sk98lin, and skge driver.
 *
 * This driver intentionally does not support all the features
 * of the original driver such as link fail-over and link management because
 * those should be done at higher levels.
 *
 * Copyright (C) 2005 Stephen Hemminger <shemminger@osdl.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <common.h>
#include <malloc.h>
#include <net.h>
#include <asm/io.h>
#include <pci.h>
#include "rmi/yukon.h"
#include "rmi/system.h"
#include "rmi/cache.h"
#include "rmi/packet.h"

extern struct packet *alloc_pbuf(void);
extern void free_pbuf(struct packet *pbuf);

#define RX_LE_SIZE              512
#define RX_LE_BYTES             (RX_LE_SIZE*sizeof(struct sky2_rx_le))
#define RX_MAX_PENDING          (RX_LE_SIZE/6 - 2)
#define RX_DEF_PENDING          RX_MAX_PENDING
#define RX_SKB_ALIGN            8
#define RX_BUF_WRITE            16

#define TX_RING_SIZE            512
#define TX_DEF_PENDING          (TX_RING_SIZE - 1)
#define TX_MIN_PENDING          64
#define MAX_SKB_TX_LE           (4 + (sizeof(dma_addr_t)/sizeof(uint32_t))*MAX_SKB_FRAGS)

#define STATUS_RING_SIZE        2048    /* 2 ports * (TX + 2*RX) */
#define STATUS_LE_BYTES         (STATUS_RING_SIZE*sizeof(struct sky2_status_le))
#define TX_WATCHDOG             (5 * HZ)
#define PHY_RETRIES             1000

#define RING_NEXT(x,s)  (((x)+1) & ((s)-1))

#define ETH_HLEN        14              /* Total octets in header.       */

/* Yukon II needs 4K aligned address for tx/rx/status le rings */
#define SKY2_LE_ALIGN		(0x1 << 12)  /* 4 KB */

/* #define SKY2_DEBUG */

#ifdef SKY2_DEBUG
#define dprint(fmt, args...) printf(fmt, ##args)
#else
#define dprint(fmt, args...)
#endif

#define DENTER dprint("sky2_debug: %s\n", __FUNCTION__)

int sky2_open(struct eth_device *dev);

/* Avoid conditionals by using array */
static const unsigned txqaddr[] = { Q_XA1, Q_XA2 };
static const unsigned rxqaddr[] = { Q_R1, Q_R2 };
static const uint32_t portirq_msk[] = { Y2_IS_PORT_1, Y2_IS_PORT_2 };


static inline void mips_sync(void)
{
        __asm__ __volatile__ (                          
                        ".set push\n"                   
                        ".set noreorder\n"              
                        "sync\n"                        
                        ".set pop\n"                    
                        : : : "memory");
}

static void * aligned_malloc(size_t how_much)
{
	size_t rnd_sz;
	uint32_t round_off = SKY2_LE_ALIGN;
	void *ptr;
	uint32_t addr, org_addr;

	rnd_sz = how_much  + sizeof(void *) + round_off;
	addr = (uint32_t)(malloc(rnd_sz));
	org_addr = addr;
	addr += sizeof(void *);

	addr = (addr + round_off) & ~(round_off -1);

	ptr = (void *) addr;

	addr -= sizeof(void *);
	*((uint32_t *)addr) = org_addr;

	return ptr;
}

static void aligned_free(void *p)
{
	uint32_t *org_p;

	org_p = p;
	org_p--;

	free((void *) (*org_p));
}


static int sky2_init(struct sky2_hw *hw)
{
	uint8_t t8;
	sky2_write8(hw, B0_CTST, CS_RST_CLR);

	hw->chip_id = sky2_read8(hw, B2_CHIP_ID);
	if (hw->chip_id < CHIP_ID_YUKON_XL || hw->chip_id > CHIP_ID_YUKON_FE) {
		printf("Unsupported yukon chipid %#x\n", hw->chip_id);
		return -1;
	}

	/* Enable all clocks */
	if (hw->chip_id == CHIP_ID_YUKON_EX || hw->chip_id == CHIP_ID_YUKON_EC_U)
		sky2_pci_write32(hw, PCI_DEV_REG3, 0);

	hw->chip_rev = (sky2_read8(hw, B2_MAC_CFG) & CFG_CHIP_R_MSK) >> 4;

	hw->pmd_type = sky2_read8(hw, B2_PMD_TYP);

	/* Support only one port NIC */
	hw->ports = 1;
	t8 = sky2_read8(hw, B2_Y2_HW_RES);
	if ((t8 & CFG_DUAL_MAC_MSK) == CFG_DUAL_MAC_MSK) {
		if (!(sky2_read8(hw, B2_Y2_CLK_GATE) & Y2_STATUS_LNK2_INAC))
			/* ++hw->ports; */
			printf("More than one port on this NIC \n");
	}

	return 0;
}

static void sky2_power_on(struct sky2_hw *hw)
{
        /* switch power to VCC (WA for VAUX problem) */
        sky2_write8(hw, B0_POWER_CTRL,
                    PC_VAUX_ENA | PC_VCC_ENA | PC_VAUX_OFF | PC_VCC_ON);

        /* disable Core Clock Division, */
        sky2_write32(hw, B2_Y2_CLK_CTRL, Y2_CLK_DIV_DIS);

        if (hw->chip_id == CHIP_ID_YUKON_XL && hw->chip_rev > 1)
                /* enable bits are inverted */
                sky2_write8(hw, B2_Y2_CLK_GATE,
                            Y2_PCI_CLK_LNK1_DIS | Y2_COR_CLK_LNK1_DIS |
                            Y2_CLK_GAT_LNK1_DIS | Y2_PCI_CLK_LNK2_DIS |
                            Y2_COR_CLK_LNK2_DIS | Y2_CLK_GAT_LNK2_DIS);
        else
                sky2_write8(hw, B2_Y2_CLK_GATE, 0);

        if (hw->chip_id == CHIP_ID_YUKON_EC_U || hw->chip_id == CHIP_ID_YUKON_EX) {
                uint32_t reg1;

                sky2_pci_write32(hw, PCI_DEV_REG3, 0);
                reg1 = sky2_pci_read32(hw, PCI_DEV_REG4);
                reg1 &= P_ASPM_CONTROL_MSK;
                sky2_pci_write32(hw, PCI_DEV_REG4, reg1);
                sky2_pci_write32(hw, PCI_DEV_REG5, 0);
        }
}

/* Access to external PHY */
static int gm_phy_write(struct sky2_hw *hw, unsigned port, uint16_t reg, uint16_t val)
{
        int i;

        gma_write16(hw, port, GM_SMI_DATA, val);
        gma_write16(hw, port, GM_SMI_CTRL,
                    GM_SMI_CT_PHY_AD(PHY_ADDR_MARV) | GM_SMI_CT_REG_AD(reg));

        for (i = 0; i < PHY_RETRIES; i++) {
                if (!(gma_read16(hw, port, GM_SMI_CTRL) & GM_SMI_CT_BUSY))
                        return 0;
                udelay(1);
        }

        printf("%s: phy write timeout\n", hw->dev[port]->name);
        return -1;
}

static int __gm_phy_read(struct sky2_hw *hw, unsigned port, u16 reg, u16 *val)
{
	int i;

	gma_write16(hw, port, GM_SMI_CTRL, GM_SMI_CT_PHY_AD(PHY_ADDR_MARV)
		    | GM_SMI_CT_REG_AD(reg) | GM_SMI_CT_OP_RD);

	for (i = 0; i < PHY_RETRIES; i++) {
		if (gma_read16(hw, port, GM_SMI_CTRL) & GM_SMI_CT_RD_VAL) {
			*val = gma_read16(hw, port, GM_SMI_DATA);
			return 0;
		}

		udelay(1);
	}

	return -1;
}

static u16 gm_phy_read(struct sky2_hw *hw, unsigned port, u16 reg)
{
	u16 v;

	if (__gm_phy_read(hw, port, reg, &v) != 0)
		printf("%s: phy read timeout\n", hw->dev[port]->name);
	return v;
}


static void sky2_gmac_reset(struct sky2_hw *hw, unsigned port)
{
        uint16_t reg;

        /* disable all GMAC IRQ's */
        sky2_write8(hw, SK_REG(port, GMAC_IRQ_MSK), 0);
        /* disable PHY IRQs */
        gm_phy_write(hw, port, PHY_MARV_INT_MASK, 0);

        gma_write16(hw, port, GM_MC_ADDR_H1, 0);        /* clear MC hash */
        gma_write16(hw, port, GM_MC_ADDR_H2, 0);
        gma_write16(hw, port, GM_MC_ADDR_H3, 0);
        gma_write16(hw, port, GM_MC_ADDR_H4, 0);

        reg = gma_read16(hw, port, GM_RX_CTRL);
        reg |= GM_RXCR_UCF_ENA | GM_RXCR_MCF_ENA;
        gma_write16(hw, port, GM_RX_CTRL, reg);
}


/* Chip internal frequency for clock calculations */
static inline uint32_t sky2_mhz(const struct sky2_hw *hw)
{
        switch (hw->chip_id) {  
        case CHIP_ID_YUKON_EC:  
        case CHIP_ID_YUKON_EC_U:
                return 125;     /* 125 Mhz */
        case CHIP_ID_YUKON_FE:  
                return 100;     /* 100 Mhz */
        default:                /* YUKON_XL */
                return 156;     /* 156 Mhz */
        }
}

static inline uint32_t sky2_us2clk(const struct sky2_hw *hw, uint32_t us)
{
        return sky2_mhz(hw) * us;
}
 
static inline uint32_t sky2_clk2us(const struct sky2_hw *hw, uint32_t clk)
{
        return clk / sky2_mhz(hw);
}   


static void _sky2_reset(struct sky2_hw *hw)
{
	uint16_t status;
	int i;

	/* disable ASF */
	if (hw->chip_id <= CHIP_ID_YUKON_EC) {
                if (hw->chip_id == CHIP_ID_YUKON_EX) {
                        status = sky2_read16(hw, HCU_CCSR);
                        status &= ~(HCU_CCSR_AHB_RST | HCU_CCSR_CPU_RST_MODE |
                                    HCU_CCSR_UC_STATE_MSK);
                        sky2_write16(hw, HCU_CCSR, status);
                } else
                        sky2_write8(hw, B28_Y2_ASF_STAT_CMD, Y2_ASF_RESET);
                sky2_write16(hw, B0_CTST, Y2_ASF_DISABLE);
        }

        /* do a SW reset */
        sky2_write8(hw, B0_CTST, CS_RST_SET);
        sky2_write8(hw, B0_CTST, CS_RST_CLR);

        /* clear PCI errors, if any */
        status = sky2_pci_read16(hw, PCI_STATUS);

        sky2_write8(hw, B2_TST_CTRL1, TST_CFG_WRITE_ON);
        sky2_pci_write16(hw, PCI_STATUS, status | PCI_STATUS_ERROR_BITS);

        sky2_write8(hw, B0_CTST, CS_MRST_CLR);

	/* clean PEX errors */
	sky2_pci_write32(hw, PEX_UNC_ERR_STAT, 0xffffffffUL);


	sky2_power_on(hw);

	for (i = 0; i < hw->ports; i++) { /* only one port as of now */
                sky2_write8(hw, SK_REG(i, GMAC_LINK_CTRL), GMLC_RST_SET);
                sky2_write8(hw, SK_REG(i, GMAC_LINK_CTRL), GMLC_RST_CLR);
        }

	sky2_write8(hw, B2_TST_CTRL1, TST_CFG_WRITE_OFF);

        /* Clear I2C IRQ noise */
        sky2_write32(hw, B2_I2C_IRQ, 1);

        /* turn off hardware timer (unused) */
        sky2_write8(hw, B2_TI_CTRL, TIM_STOP);
        sky2_write8(hw, B2_TI_CTRL, TIM_CLR_IRQ);

        sky2_write8(hw, B0_Y2LED, LED_STAT_ON);

        /* Turn off descriptor polling */
        sky2_write32(hw, B28_DPT_CTRL, DPT_STOP);

        /* Turn off receive timestamp */
        sky2_write8(hw, GMAC_TI_ST_CTRL, GMT_ST_STOP);
        sky2_write8(hw, GMAC_TI_ST_CTRL, GMT_ST_CLR_IRQ);


        /* enable the Tx Arbiters */
        for (i = 0; i < hw->ports; i++)
                sky2_write8(hw, SK_REG(i, TXA_CTRL), TXA_ENA_ARB);

        /* Initialize ram interface */
        for (i = 0; i < hw->ports; i++) {
                sky2_write8(hw, RAM_BUFFER(i, B3_RI_CTRL), RI_RST_CLR);

                sky2_write8(hw, RAM_BUFFER(i, B3_RI_WTO_R1), SK_RI_TO_53);
                sky2_write8(hw, RAM_BUFFER(i, B3_RI_WTO_XA1), SK_RI_TO_53);
                sky2_write8(hw, RAM_BUFFER(i, B3_RI_WTO_XS1), SK_RI_TO_53);
                sky2_write8(hw, RAM_BUFFER(i, B3_RI_RTO_R1), SK_RI_TO_53);
                sky2_write8(hw, RAM_BUFFER(i, B3_RI_RTO_XA1), SK_RI_TO_53);
                sky2_write8(hw, RAM_BUFFER(i, B3_RI_RTO_XS1), SK_RI_TO_53);
                sky2_write8(hw, RAM_BUFFER(i, B3_RI_WTO_R2), SK_RI_TO_53);
                sky2_write8(hw, RAM_BUFFER(i, B3_RI_WTO_XA2), SK_RI_TO_53);
                sky2_write8(hw, RAM_BUFFER(i, B3_RI_WTO_XS2), SK_RI_TO_53);
                sky2_write8(hw, RAM_BUFFER(i, B3_RI_RTO_R2), SK_RI_TO_53);
                sky2_write8(hw, RAM_BUFFER(i, B3_RI_RTO_XA2), SK_RI_TO_53);
                sky2_write8(hw, RAM_BUFFER(i, B3_RI_RTO_XS2), SK_RI_TO_53);
        }

        sky2_write32(hw, B0_HWE_IMSK, Y2_HWE_ALL_MASK);

        for (i = 0; i < hw->ports; i++)
                sky2_gmac_reset(hw, i);

        memset(hw->st_le, 0, STATUS_LE_BYTES);
        hw->st_idx = 0;

        sky2_write32(hw, SKY2_STAT_CTRL, SC_STAT_RST_SET);
        sky2_write32(hw, SKY2_STAT_CTRL, SC_STAT_RST_CLR);

        sky2_write32(hw, STAT_LIST_ADDR_LO, hw->st_dma);
        //sky2_write32(hw, STAT_LIST_ADDR_HI, (u64) hw->st_dma >> 32);
        sky2_write32(hw, STAT_LIST_ADDR_HI, 0);

        /* Set the list last index */
        sky2_write16(hw, STAT_LAST_IDX, STATUS_RING_SIZE - 1);
#if 1
        sky2_write16(hw, STAT_TX_IDX_TH, 10);
        sky2_write8(hw, STAT_FIFO_WM, 16);
#else
        sky2_write16(hw, STAT_TX_IDX_TH, 1);
        sky2_write8(hw, STAT_FIFO_WM, 1);
#endif

        /* set Status-FIFO ISR watermark */
        if (hw->chip_id == CHIP_ID_YUKON_XL && hw->chip_rev == 0)
                sky2_write8(hw, STAT_FIFO_ISR_WM, 4);
        else
                sky2_write8(hw, STAT_FIFO_ISR_WM, 16);

        sky2_write32(hw, STAT_TX_TIMER_INI, sky2_us2clk(hw, 1000));
        sky2_write32(hw, STAT_ISR_TIMER_INI, sky2_us2clk(hw, 20));
        sky2_write32(hw, STAT_LEV_TIMER_INI, sky2_us2clk(hw, 100));

        /* enable status unit */
        sky2_write32(hw, SKY2_STAT_CTRL, SC_STAT_OP_ON);

        sky2_write8(hw, STAT_TX_TIMER_CTRL, TIM_START);
        sky2_write8(hw, STAT_LEV_TIMER_CTRL, TIM_START);
        sky2_write8(hw, STAT_ISR_TIMER_CTRL, TIM_START);
}

static void memcopy_fromio (void *dst, const volatile void *src, size_t n)
{
	char *d = dst;
	int i = 0;

	while (n--) {
		char tmp = INB((unsigned int)src, i);
		*d++ = tmp;
		i++;
	}

}

static uint32_t sky2_supported_modes(const struct sky2_hw *hw)
{
        if (sky2_is_copper(hw)) {
                uint32_t modes = SUPPORTED_10baseT_Half
                        | SUPPORTED_10baseT_Full
                        | SUPPORTED_100baseT_Half
                        | SUPPORTED_100baseT_Full
                        | SUPPORTED_Autoneg | SUPPORTED_TP;

                if (hw->chip_id != CHIP_ID_YUKON_FE)
                        modes |= SUPPORTED_1000baseT_Half
                                | SUPPORTED_1000baseT_Full;
                return modes;
        } else
                return  SUPPORTED_1000baseT_Half
                        | SUPPORTED_1000baseT_Full
                        | SUPPORTED_Autoneg
                        | SUPPORTED_FIBRE;
}


static void sky2_init_netdev(struct eth_device * dev, struct sky2_hw *hw)
{
	struct sky2_port *sky2;
	int port = 0;
	struct sky2_priv_data *pr;

	pr = (struct sky2_priv_data *) (dev->priv);

	sky2 = malloc(sizeof(*sky2));
	if (!sky2) {
		printf("Malloc failed \n");
		return;
	}

	sky2->netdev = dev;
	sky2->hw = hw;

	/* Auto speed and flow control */
	sky2->autoneg = AUTONEG_ENABLE; 
	sky2->flow_mode = FC_BOTH;
	sky2->duplex = -1;
	sky2->speed = -1;
	sky2->advertising =  sky2_supported_modes(hw);
	/* Disable checksum offloading */
	sky2->rx_csum = 0;
	sky2->wol = 0; /* Unsupported Wake On LAN */
	sky2->tx_pending = TX_DEF_PENDING;
	sky2->rx_pending = RX_DEF_PENDING;
	hw->dev[port] = dev; /* Only one port */
	sky2->port = port; 

	pr->driver_data = sky2;

	/* read MAC Address */
	memcopy_fromio(dev->enetaddr, 
			hw->regs + B2_MAC_1 + port * 8, 6);

	printf("Read MAC Address = %02x:%02x.%02x:%02x.%02x:%02x\n",
			dev->enetaddr[0], dev->enetaddr[1], 
			dev->enetaddr[2], dev->enetaddr[3], 
			dev->enetaddr[4], dev->enetaddr[5]);

}
	
int sky2_initialize (struct eth_device *dev, bd_t * bis)
{
	struct sky2_priv_data * pri;
	struct sky2_port *sky2;
   struct sky2_hw *hw;
	uint16_t phystat, gpcontrol;
	unsigned port;

	pri = (struct sky2_priv_data *) dev->priv;

	/* uint16_t cmd; */
	int err;

   if (!dev) {
      printf("Null eth_device passed to %s\n", __FUNCTION__);        
      return -1;
   }

	if (pri->init_done == 1) {
   	sky2 = pri->driver_data;
		port = sky2->port;
   	hw = sky2->hw;

		/* get link status */
		phystat = gm_phy_read(hw, port, PHY_MARV_PHY_STAT);
		if (phystat & PHY_M_PS_LINK_UP) {
			/* printf ("link is Up.\n"); */
		}
		else {
			printf ("link is Down.\n");
			return 0;
		}

		/* enable receiver if link is up*/
		gpcontrol = gma_read16(hw, port, GM_GP_CTRL);
		gpcontrol |= GM_GPCR_RX_ENA;
		gma_write16(hw, port, GM_GP_CTRL, gpcontrol);

	   /* printf("[%s]:Init previously done and link is Up.\n", __FUNCTION__); */

		return 1;
	}

	hw = malloc(sizeof(*hw));
	if (!hw) {
		printf("malloc failed at %s:%d\n", __FILE__, __LINE__);
		return -1;
	}

	memset(hw, 0, sizeof(*hw));

	hw->regs = (volatile void *) dev->iobase;

	/* Software byte swapping - hardware byte swapping not used */
	{
		uint32_t reg;
		reg = sky2_pci_read32(hw, PCI_DEV_REG2);
		reg &= ~PCI_REV_DESC;
		sky2_pci_write32(hw, PCI_DEV_REG2, reg);
	}

	hw->st_le = aligned_malloc(STATUS_LE_BYTES);
	if (hw->st_le == NULL) {
		dprint("Malloc failed for %d bytes\n", STATUS_LE_BYTES);
		free(hw);
		return -1;
	}
	hw->st_dma = virt_to_phys(hw->st_le);


	sky2->link_up_done = 0;
	err = sky2_init(hw);
	if (err) {
		aligned_free(hw->st_le);
		free(hw);
		return -1; 
	}

	dprint("Chip Id = %x\n", hw->chip_id);

	_sky2_reset(hw);
	sky2_init_netdev(dev, hw);

	sky2_write32(hw, B0_IMSK, Y2_IS_BASE);
	
	sky2_open (dev);

	pri->init_done = 1;
	
	udelay (1000 * 1000 * 10); /* 10 seconds*/

   sky2 = pri->driver_data;
	port = sky2->port;
	phystat = gm_phy_read(hw, port, PHY_MARV_PHY_STAT);
	if (phystat & PHY_M_PS_LINK_UP) {
		printf ("link is Up.\n");
	}
	else {
		printf ("link is Down.\n");
		return 0;
	}

	return 1;
}

static void sky2_phy_power(struct sky2_hw *hw, unsigned port, int onoff)
{
	uint32_t reg1;
	static const uint32_t phy_power[]
		= { PCI_Y2_PHY1_POWD, PCI_Y2_PHY2_POWD };

	/* looks like this XL is back asswards .. */
	if (hw->chip_id == CHIP_ID_YUKON_XL && hw->chip_rev > 1)
		onoff = !onoff;

	sky2_write8(hw, B2_TST_CTRL1, TST_CFG_WRITE_ON);
	reg1 = sky2_pci_read32(hw, PCI_DEV_REG1);
	if (onoff)
		/* Turn off phy power saving */
		reg1 &= ~phy_power[port];
	else
		reg1 |= phy_power[port];

	sky2_pci_write32(hw, PCI_DEV_REG1, reg1);
	sky2_pci_read32(hw, PCI_DEV_REG1);
	sky2_write8(hw, B2_TST_CTRL1, TST_CFG_WRITE_OFF);
	udelay(100);
}

/* Assign Ram Buffer allocation to queue */
static void sky2_ramset(struct sky2_hw *hw, uint16_t q, uint32_t start, uint32_t space)
{
	uint32_t end;

	/* convert from K bytes to qwords used for hw register */
	start *= 1024/8;
	space *= 1024/8;
	end = start + space - 1;

	sky2_write8(hw, RB_ADDR(q, RB_CTRL), RB_RST_CLR);
	sky2_write32(hw, RB_ADDR(q, RB_START), start);
	sky2_write32(hw, RB_ADDR(q, RB_END), end);
	sky2_write32(hw, RB_ADDR(q, RB_WP), start);
	sky2_write32(hw, RB_ADDR(q, RB_RP), start);

	if (q == Q_R1 || q == Q_R2) {
		uint32_t tp = space - space/4;

		/* On receive queue's set the thresholds
		 * 		 * give receiver priority when > 3/4 full
		 * 		 		 * send pause when down to 2K
		 * 		 		 		 */
		sky2_write32(hw, RB_ADDR(q, RB_RX_UTHP), tp);
		sky2_write32(hw, RB_ADDR(q, RB_RX_LTHP), space/2);

		tp = space - 2048/8;
		sky2_write32(hw, RB_ADDR(q, RB_RX_UTPP), tp);
		sky2_write32(hw, RB_ADDR(q, RB_RX_LTPP), space/4);
	} else {
		/* Enable store & forward on Tx queue's because
		 * 		 * Tx FIFO is only 1K on Yukon
		 * 		 		 */
		sky2_write8(hw, RB_ADDR(q, RB_CTRL), RB_ENA_STFWD);
	}

	sky2_write8(hw, RB_ADDR(q, RB_CTRL), RB_ENA_OP_MD);
	sky2_read8(hw, RB_ADDR(q, RB_CTRL));
}

/* Setup Bus Memory Interface */
static void sky2_qset(struct sky2_hw *hw, uint16_t q)
{
	sky2_write32(hw, Q_ADDR(q, Q_CSR), BMU_CLR_RESET);
	sky2_write32(hw, Q_ADDR(q, Q_CSR), BMU_OPER_INIT);
	sky2_write32(hw, Q_ADDR(q, Q_CSR), BMU_FIFO_OP_ON);
	sky2_write32(hw, Q_ADDR(q, Q_WM),  BMU_WM_DEFAULT);
}


/* Setup prefetch unit registers. This is the interface between
 *  * hardware and driver list elements
 *   */
static void sky2_prefetch_init(struct sky2_hw *hw, uint32_t qaddr,
				      u64 addr, uint32_t last)
{
	dprint("sky2_prefetch_init: addr = %#x\n", (uint32_t) addr);
	sky2_write32(hw, Y2_QADDR(qaddr, PREF_UNIT_CTRL), PREF_UNIT_RST_SET);
	sky2_write32(hw, Y2_QADDR(qaddr, PREF_UNIT_CTRL), PREF_UNIT_RST_CLR);

	sky2_write32(hw, Y2_QADDR(qaddr, PREF_UNIT_ADDR_HI), 0);
	sky2_write32(hw, Y2_QADDR(qaddr, PREF_UNIT_ADDR_LO), (uint32_t) addr);
	sky2_write16(hw, Y2_QADDR(qaddr, PREF_UNIT_LAST_IDX), last);
	sky2_write32(hw, Y2_QADDR(qaddr, PREF_UNIT_CTRL), PREF_UNIT_OP_ON);

	sky2_read32(hw, Y2_QADDR(qaddr, PREF_UNIT_CTRL));
}

static inline struct sky2_rx_le *sky2_next_rx(struct sky2_port *sky2)
{
	struct sky2_rx_le *le = sky2->rx_le + sky2->rx_put;
	sky2->rx_put = RING_NEXT(sky2->rx_put, RX_LE_SIZE);
	le->ctrl = 0;
	return le;
}


/* Tell chip where to start receive checksum.
 * Actually has two checksums, but set both same to avoid possible byte
 * order problems.
 */
static void rx_set_checksum(struct sky2_port *sky2)
{
	struct sky2_rx_le *le;

	le = sky2_next_rx(sky2);
	le->addr = cpu_to_le32((ETH_HLEN << 16) | ETH_HLEN);
	le->ctrl = 0;
	le->opcode = OP_TCPSTART | HW_OWNER;

	sky2_write32(sky2->hw,
		     Q_ADDR(rxqaddr[sky2->port], Q_CSR),
		     sky2->rx_csum ? BMU_ENA_RX_CHKSUM : BMU_DIS_RX_CHKSUM);

}

/*
 * Allocate an skb for receiving. If the MTU is large enough
 * make the skb non-linear with a fragment list of pages.
 *
 * It appears the hardware has a bug in the FIFO logic that
 * cause it to hang if the FIFO gets overrun and the receive buffer
 * is not 64 byte aligned. The buffer returned from netdev_alloc_skb is
 * aligned except if slab debugging is enabled.
 */
static struct packet *sky2_rx_alloc(struct sky2_port *sky2)
{
	struct packet *skb;
	skb = alloc_pbuf();
	return skb;
}

/* Build description to hardware for one receive segment */
static void sky2_rx_add(struct sky2_port *sky2,  uint8_t op,
			uint32_t map, unsigned len)
{
	struct sky2_rx_le *le;
	uint32_t hi = 0;

	if (sky2->rx_addr64 != hi) {
		le = sky2_next_rx(sky2);
		le->addr = cpu_to_le32(hi);
		le->opcode = OP_ADDR64 | HW_OWNER;
		sky2->rx_addr64 = 0;
	}

	le = sky2_next_rx(sky2);

	le->addr = cpu_to_le32((uint32_t) map); 
	le->length = cpu_to_le16(len);
	le->opcode = op | HW_OWNER;
}


/* Build description to hardware for one possibly fragmented skb */
static void sky2_rx_submit(struct sky2_port *sky2,
			   const struct rx_ring_info *re)
{
	sky2_rx_add(sky2, OP_PACKET, re->data_addr, sky2->rx_data_size);
}

/* Clean out receive buffer area, assumes receiver hardware stopped */
static void sky2_rx_clean(struct sky2_port *sky2)
{
	unsigned i;

	memset(sky2->rx_le, 0, RX_LE_BYTES);
	for (i = 0; i < sky2->rx_pending; i++) {
		struct rx_ring_info *re = sky2->rx_ring + i;

		if (re->skb) {
			free_pbuf(re->skb);
			re->skb = NULL;
		}
	}
}

static void sky2_rx_map_skb(struct pci_dev *pdev, struct rx_ring_info *re,
			    unsigned size)
{
	struct packet *skb = re->skb;

	re->data_addr = virt_to_phys(skb->data);
	re->data_size = size;

}


/*
 * Allocate and setup receiver buffer pool.
 * Normal case this ends up creating one list element for skb
 * in the receive ring. Worst case if using large MTU and each
 * allocation falls on a different 64 bit region, that results
 * in 6 list elements per ring entry.
 * One element is used for checksum enable/disable, and one
 * extra to avoid wrap.
 */
static int sky2_rx_start(struct sky2_port *sky2)
{
	struct sky2_hw *hw = sky2->hw;
	struct rx_ring_info *re;
	unsigned rxq = rxqaddr[sky2->port];
	unsigned i, size, space, thresh;

	sky2->rx_put = sky2->rx_next = 0;
	sky2_qset(hw, rxq);

	/* On PCI express lowering the watermark gives better performance */
	sky2_write32(hw, Q_ADDR(rxq, Q_WM), BMU_WM_PEX); 

	/* These chips have no ram buffer?
	 * MAC Rx RAM Read is controlled by hardware */
	if (hw->chip_id == CHIP_ID_YUKON_EC_U &&
	    (hw->chip_rev == CHIP_REV_YU_EC_U_A1
	     || hw->chip_rev == CHIP_REV_YU_EC_U_B0))
		sky2_write32(hw, Q_ADDR(rxq, Q_F), F_M_RX_RAM_DIS);

	sky2_prefetch_init(hw, rxq, sky2->rx_le_map, RX_LE_SIZE - 1);

	rx_set_checksum(sky2);

	/* Space needed for frame data + headers rounded up */
	size = ALIGN(PKT_DATA_LEN, 8) + 8;

	/* Stopping point for hardware truncation */
	thresh = (size - 8) / sizeof(uint32_t);

	/* Account for overhead of skb - to avoid order > 0 allocation */
	/* space = SKB_DATA_ALIGN(size) + 16
		+ sizeof(struct skb_shared_info);
	*/
	space = size;
	sky2->rx_data_size = size;

	/* Fill Rx ring */
	for (i = 0; i < sky2->rx_pending; i++) {
		re = sky2->rx_ring + i;

		re->skb = sky2_rx_alloc(sky2);
		if (!re->skb)
			goto nomem;

		sky2_rx_map_skb(hw->pdev, re, sky2->rx_data_size); 
		sky2_rx_submit(sky2, re);
	}

	/*
	 * The receiver hangs if it receives frames larger than the
	 * packet buffer. As a workaround, truncate oversize frames, but
	 * the register is limited to 9 bits, so if you do frames > 2052
	 * you better get the MTU right!
	 */
	if (thresh > 0x1ff)
		sky2_write32(hw, SK_REG(sky2->port, RX_GMF_CTRL_T), RX_TRUNC_OFF);
	else {
		sky2_write16(hw, SK_REG(sky2->port, RX_GMF_TR_THR), thresh);
		sky2_write32(hw, SK_REG(sky2->port, RX_GMF_CTRL_T), RX_TRUNC_ON);
	}

	/* Tell chip about available buffers */
	sky2_write16(hw, Y2_QADDR(rxq, PREF_UNIT_PUT_IDX), sky2->rx_put);
	return 0;
nomem:
	sky2_rx_clean(sky2);
	return -1;
}

/* flow control to advertise bits */
static const uint16_t copper_fc_adv[] = {
	[FC_NONE]	= 0,
	[FC_TX]		= PHY_M_AN_ASP,
	[FC_RX]		= PHY_M_AN_PC,
	[FC_BOTH]	= PHY_M_AN_PC | PHY_M_AN_ASP,
};

/* flow control to advertise bits when using 1000BaseX */
static const uint16_t fiber_fc_adv[] = {
	[FC_BOTH] = PHY_M_P_BOTH_MD_X,
	[FC_TX]   = PHY_M_P_ASYM_MD_X,
	[FC_RX]	  = PHY_M_P_SYM_MD_X,
	[FC_NONE] = PHY_M_P_NO_PAUSE_X,
};

/* flow control to GMA disable bits */
static const uint16_t gm_fc_disable[] = {
	[FC_NONE] = GM_GPCR_FC_RX_DIS | GM_GPCR_FC_TX_DIS,
	[FC_TX]	  = GM_GPCR_FC_RX_DIS,
	[FC_RX]	  = GM_GPCR_FC_TX_DIS,
	[FC_BOTH] = 0,
};

static void sky2_phy_init(struct sky2_hw *hw, unsigned port)
{
	struct sky2_priv_data * pri;
	pri = (struct sky2_priv_data *) hw->dev[port]->priv;

	struct sky2_port *sky2 = pri->driver_data;
	uint16_t ctrl, ct1000, adv, pg, ledctrl, ledover, reg;

	if (sky2->autoneg == AUTONEG_ENABLE
	    && !(hw->chip_id == CHIP_ID_YUKON_XL
		 || hw->chip_id == CHIP_ID_YUKON_EC_U
		 || hw->chip_id == CHIP_ID_YUKON_EX)) {
		uint16_t ectrl = gm_phy_read(hw, port, PHY_MARV_EXT_CTRL);

		ectrl &= ~(PHY_M_EC_M_DSC_MSK | PHY_M_EC_S_DSC_MSK |
			   PHY_M_EC_MAC_S_MSK);
		ectrl |= PHY_M_EC_MAC_S(MAC_TX_CLK_25_MHZ);

		if (hw->chip_id == CHIP_ID_YUKON_EC)
			ectrl |= PHY_M_EC_DSC_2(2) | PHY_M_EC_DOWN_S_ENA;
		else
			ectrl |= PHY_M_EC_M_DSC(2) | PHY_M_EC_S_DSC(3);

		gm_phy_write(hw, port, PHY_MARV_EXT_CTRL, ectrl);
	}

	ctrl = gm_phy_read(hw, port, PHY_MARV_PHY_CTRL);
	if (sky2_is_copper(hw)) {
		if (hw->chip_id == CHIP_ID_YUKON_FE) {
			/* enable automatic crossover */
			ctrl |= PHY_M_PC_MDI_XMODE(PHY_M_PC_ENA_AUTO) >> 1;
		} else {
			/* disable energy detect */
			ctrl &= ~PHY_M_PC_EN_DET_MSK;

			/* enable automatic crossover */
			ctrl |= PHY_M_PC_MDI_XMODE(PHY_M_PC_ENA_AUTO);

			if (sky2->autoneg == AUTONEG_ENABLE
			    && (hw->chip_id == CHIP_ID_YUKON_XL
				|| hw->chip_id == CHIP_ID_YUKON_EC_U
				|| hw->chip_id == CHIP_ID_YUKON_EX)) {
				ctrl &= ~PHY_M_PC_DSC_MSK;
				ctrl |= PHY_M_PC_DSC(2) | PHY_M_PC_DOWN_S_ENA;
			}
		}
	} else {
		/* workaround for deviation #4.88 (CRC errors) */
		/* disable Automatic Crossover */

		ctrl &= ~PHY_M_PC_MDIX_MSK;
	}

	gm_phy_write(hw, port, PHY_MARV_PHY_CTRL, ctrl);

	/* special setup for PHY 88E1112 Fiber */
	if (hw->chip_id == CHIP_ID_YUKON_XL && !sky2_is_copper(hw)) {
		pg = gm_phy_read(hw, port, PHY_MARV_EXT_ADR);

		/* Fiber: select 1000BASE-X only mode MAC Specific Ctrl Reg. */
		gm_phy_write(hw, port, PHY_MARV_EXT_ADR, 2);
		ctrl = gm_phy_read(hw, port, PHY_MARV_PHY_CTRL);
		ctrl &= ~PHY_M_MAC_MD_MSK;
		ctrl |= PHY_M_MAC_MODE_SEL(PHY_M_MAC_MD_1000BX);
		gm_phy_write(hw, port, PHY_MARV_PHY_CTRL, ctrl);

		if (hw->pmd_type  == 'P') {
			/* select page 1 to access Fiber registers */
			gm_phy_write(hw, port, PHY_MARV_EXT_ADR, 1);

			/* for SFP-module set SIGDET polarity to low */
			ctrl = gm_phy_read(hw, port, PHY_MARV_PHY_CTRL);
			ctrl |= PHY_M_FIB_SIGD_POL;
			gm_phy_write(hw, port, PHY_MARV_CTRL, ctrl);
		}

		gm_phy_write(hw, port, PHY_MARV_EXT_ADR, pg);
	}

	ctrl = PHY_CT_RESET;
	ct1000 = 0;
	adv = PHY_AN_CSMA;
	reg = 0;

	if (sky2->autoneg == AUTONEG_ENABLE) {
		if (sky2_is_copper(hw)) {
			if (sky2->advertising & ADVERTISED_1000baseT_Full)
				ct1000 |= PHY_M_1000C_AFD;
			if (sky2->advertising & ADVERTISED_1000baseT_Half)
				ct1000 |= PHY_M_1000C_AHD;
			if (sky2->advertising & ADVERTISED_100baseT_Full)
				adv |= PHY_M_AN_100_FD;
			if (sky2->advertising & ADVERTISED_100baseT_Half)
				adv |= PHY_M_AN_100_HD;
			if (sky2->advertising & ADVERTISED_10baseT_Full)
				adv |= PHY_M_AN_10_FD;
			if (sky2->advertising & ADVERTISED_10baseT_Half)
				adv |= PHY_M_AN_10_HD;

			adv |= copper_fc_adv[sky2->flow_mode];
		} else {	/* special defines for FIBER (88E1040S only) */
			if (sky2->advertising & ADVERTISED_1000baseT_Full)
				adv |= PHY_M_AN_1000X_AFD;
			if (sky2->advertising & ADVERTISED_1000baseT_Half)
				adv |= PHY_M_AN_1000X_AHD;

			adv |= fiber_fc_adv[sky2->flow_mode];
		}

		/* Restart Auto-negotiation */
		ctrl |= PHY_CT_ANE | PHY_CT_RE_CFG;
	} else {
		/* forced speed/duplex settings */
		ct1000 = PHY_M_1000C_MSE;

		/* Disable auto update for duplex flow control and speed */
		reg |= GM_GPCR_AU_ALL_DIS;

		switch (sky2->speed) {
		case SPEED_1000:
			ctrl |= PHY_CT_SP1000;
			reg |= GM_GPCR_SPEED_1000;
			break;
		case SPEED_100:
			ctrl |= PHY_CT_SP100;
			reg |= GM_GPCR_SPEED_100;
			break;
		}

		if (sky2->duplex == DUPLEX_FULL) {
			reg |= GM_GPCR_DUP_FULL;
			ctrl |= PHY_CT_DUP_MD;
		} else if (sky2->speed < SPEED_1000)
			sky2->flow_mode = FC_NONE;


 		reg |= gm_fc_disable[sky2->flow_mode];

		/* Forward pause packets to GMAC? */
		if (sky2->flow_mode & FC_RX)
			sky2_write8(hw, SK_REG(port, GMAC_CTRL), GMC_PAUSE_ON);
		else
			sky2_write8(hw, SK_REG(port, GMAC_CTRL), GMC_PAUSE_OFF);
	}

	gma_write16(hw, port, GM_GP_CTRL, reg);

	if (hw->chip_id != CHIP_ID_YUKON_FE)
		gm_phy_write(hw, port, PHY_MARV_1000T_CTRL, ct1000);

	gm_phy_write(hw, port, PHY_MARV_AUNE_ADV, adv);
	gm_phy_write(hw, port, PHY_MARV_CTRL, ctrl);

	/* Setup Phy LED's */
	ledctrl = PHY_M_LED_PULS_DUR(PULS_170MS);
	ledover = 0;

	switch (hw->chip_id) {
	case CHIP_ID_YUKON_FE:
		/* on 88E3082 these bits are at 11..9 (shifted left) */
		ledctrl |= PHY_M_LED_BLINK_RT(BLINK_84MS) << 1;

		ctrl = gm_phy_read(hw, port, PHY_MARV_FE_LED_PAR);

		/* delete ACT LED control bits */
		ctrl &= ~PHY_M_FELP_LED1_MSK;
		/* change ACT LED control to blink mode */
		ctrl |= PHY_M_FELP_LED1_CTRL(LED_PAR_CTRL_ACT_BL);
		gm_phy_write(hw, port, PHY_MARV_FE_LED_PAR, ctrl);
		break;

	case CHIP_ID_YUKON_XL:
		pg = gm_phy_read(hw, port, PHY_MARV_EXT_ADR);

		/* select page 3 to access LED control register */
		gm_phy_write(hw, port, PHY_MARV_EXT_ADR, 3);

		/* set LED Function Control register */
		gm_phy_write(hw, port, PHY_MARV_PHY_CTRL,
			     (PHY_M_LEDC_LOS_CTRL(1) |	/* LINK/ACT */
			      PHY_M_LEDC_INIT_CTRL(7) |	/* 10 Mbps */
			      PHY_M_LEDC_STA1_CTRL(7) |	/* 100 Mbps */
			      PHY_M_LEDC_STA0_CTRL(7)));	/* 1000 Mbps */

		/* set Polarity Control register */
		gm_phy_write(hw, port, PHY_MARV_PHY_STAT,
			     (PHY_M_POLC_LS1_P_MIX(4) |
			      PHY_M_POLC_IS0_P_MIX(4) |
			      PHY_M_POLC_LOS_CTRL(2) |
			      PHY_M_POLC_INIT_CTRL(2) |
			      PHY_M_POLC_STA1_CTRL(2) |
			      PHY_M_POLC_STA0_CTRL(2)));

		/* restore page register */
		gm_phy_write(hw, port, PHY_MARV_EXT_ADR, pg);
		break;

	case CHIP_ID_YUKON_EC_U:
	case CHIP_ID_YUKON_EX:
		pg = gm_phy_read(hw, port, PHY_MARV_EXT_ADR);

		/* select page 3 to access LED control register */
		gm_phy_write(hw, port, PHY_MARV_EXT_ADR, 3);

		/* set LED Function Control register */
		gm_phy_write(hw, port, PHY_MARV_PHY_CTRL,
			     (PHY_M_LEDC_LOS_CTRL(1) |	/* LINK/ACT */
			      PHY_M_LEDC_INIT_CTRL(8) |	/* 10 Mbps */
			      PHY_M_LEDC_STA1_CTRL(7) |	/* 100 Mbps */
			      PHY_M_LEDC_STA0_CTRL(7)));/* 1000 Mbps */

		/* set Blink Rate in LED Timer Control Register */
		gm_phy_write(hw, port, PHY_MARV_INT_MASK,
			     ledctrl | PHY_M_LED_BLINK_RT(BLINK_84MS));
		/* restore page register */
		gm_phy_write(hw, port, PHY_MARV_EXT_ADR, pg);
		break;

	default:
		/* set Tx LED (LED_TX) to blink mode on Rx OR Tx activity */
		ledctrl |= PHY_M_LED_BLINK_RT(BLINK_84MS) | PHY_M_LEDC_TX_CTRL;
		/* turn off the Rx LED (LED_RX) */
		ledover &= ~PHY_M_LED_MO_RX;
	}

	if (hw->chip_id == CHIP_ID_YUKON_EC_U &&
	    hw->chip_rev == CHIP_REV_YU_EC_U_A1) {
		/* apply fixes in PHY AFE */
		gm_phy_write(hw, port, PHY_MARV_EXT_ADR, 255);

		/* increase differential signal amplitude in 10BASE-T */
		gm_phy_write(hw, port, 0x18, 0xaa99);
		gm_phy_write(hw, port, 0x17, 0x2011);

		/* fix for IEEE A/B Symmetry failure in 1000BASE-T */
		gm_phy_write(hw, port, 0x18, 0xa204);
		gm_phy_write(hw, port, 0x17, 0x2002);

		/* set page register to 0 */
		gm_phy_write(hw, port, PHY_MARV_EXT_ADR, 0);
	} else if (hw->chip_id != CHIP_ID_YUKON_EX) {
		gm_phy_write(hw, port, PHY_MARV_LED_CTRL, ledctrl);

		if (sky2->autoneg == AUTONEG_DISABLE || sky2->speed == SPEED_100) {
			/* turn on 100 Mbps LED (LED_LINK100) */
			ledover |= PHY_M_LED_MO_100;
		}

		if (ledover)
			gm_phy_write(hw, port, PHY_MARV_LED_OVER, ledover);

	}

	/* Enable phy interrupt on auto-negotiation complete (or link up) */
	if (sky2->autoneg == AUTONEG_ENABLE)
		gm_phy_write(hw, port, PHY_MARV_INT_MASK, PHY_M_IS_AN_COMPL);
	else
		gm_phy_write(hw, port, PHY_MARV_INT_MASK, PHY_M_DEF_MSK);
}


static void sky2_mac_init(struct sky2_hw *hw, unsigned port)
{
	/* struct sky2_port *sky2 = hw->dev[port]->priv->driver_data; */
	uint16_t reg;
	int i;
	const uint8_t *addr = hw->dev[port]->enetaddr;

	sky2_write32(hw, SK_REG(port, GPHY_CTRL), GPC_RST_SET);
	sky2_write32(hw, SK_REG(port, GPHY_CTRL), GPC_RST_CLR|GPC_ENA_PAUSE);

	sky2_write8(hw, SK_REG(port, GMAC_CTRL), GMC_RST_CLR);

	if (hw->chip_id == CHIP_ID_YUKON_XL && hw->chip_rev == 0 && port == 1) {
		/* WA DEV_472 -- looks like crossed wires on port 2 */
		/* clear GMAC 1 Control reset */
		sky2_write8(hw, SK_REG(0, GMAC_CTRL), GMC_RST_CLR);
		do {
			sky2_write8(hw, SK_REG(1, GMAC_CTRL), GMC_RST_SET);
			sky2_write8(hw, SK_REG(1, GMAC_CTRL), GMC_RST_CLR);
		} while (gm_phy_read(hw, 1, PHY_MARV_ID0) != PHY_MARV_ID0_VAL ||
			 gm_phy_read(hw, 1, PHY_MARV_ID1) != PHY_MARV_ID1_Y2 ||
			 gm_phy_read(hw, 1, PHY_MARV_INT_MASK) != 0);
	}

	sky2_read16(hw, SK_REG(port, GMAC_IRQ_SRC));

	/* Enable Transmit FIFO Underrun */
	sky2_write8(hw, SK_REG(port, GMAC_IRQ_MSK), GMAC_DEF_MSK);

	sky2_phy_init(hw, port);

	/* MIB clear */
	reg = gma_read16(hw, port, GM_PHY_ADDR);
	gma_write16(hw, port, GM_PHY_ADDR, reg | GM_PAR_MIB_CLR);

	for (i = GM_MIB_CNT_BASE; i <= GM_MIB_CNT_END; i += 4)
		gma_read16(hw, port, i);
	gma_write16(hw, port, GM_PHY_ADDR, reg);

	/* transmit control */
	gma_write16(hw, port, GM_TX_CTRL, TX_COL_THR(TX_COL_DEF));

	/* receive control reg: unicast + multicast + no FCS  */
	gma_write16(hw, port, GM_RX_CTRL,
		    GM_RXCR_UCF_ENA | GM_RXCR_CRC_DIS | GM_RXCR_MCF_ENA);

	/* transmit flow control */
	gma_write16(hw, port, GM_TX_FLOW_CTRL, 0xffff);

	/* transmit parameter */
	gma_write16(hw, port, GM_TX_PARAM,
		    TX_JAM_LEN_VAL(TX_JAM_LEN_DEF) |
		    TX_JAM_IPG_VAL(TX_JAM_IPG_DEF) |
		    TX_IPG_JAM_DATA(TX_IPG_JAM_DEF) |
		    TX_BACK_OFF_LIM(TX_BOF_LIM_DEF));

	/* serial mode register */
	reg = DATA_BLIND_VAL(DATA_BLIND_DEF) |
		GM_SMOD_VLAN_ENA | IPG_DATA_VAL(IPG_DATA_DEF);

	gma_write16(hw, port, GM_SERIAL_MODE, reg);

	/*  virtual address for data */
	gma_set_addr(hw, port, GM_SRC_ADDR_2L, addr);

	/* physical address: used for pause frames */
	gma_set_addr(hw, port, GM_SRC_ADDR_1L, addr);

	/* ignore counter overflows */
	gma_write16(hw, port, GM_TX_IRQ_MSK, 0);
	gma_write16(hw, port, GM_RX_IRQ_MSK, 0);
	gma_write16(hw, port, GM_TR_IRQ_MSK, 0);

	/* Configure Rx MAC FIFO */
	sky2_write8(hw, SK_REG(port, RX_GMF_CTRL_T), GMF_RST_CLR);
	sky2_write32(hw, SK_REG(port, RX_GMF_CTRL_T),
		     GMF_OPER_ON | GMF_RX_F_FL_ON);

	/* Flush Rx MAC FIFO on any flow control or error */
	sky2_write16(hw, SK_REG(port, RX_GMF_FL_MSK), GMR_FS_ANY_ERR);

	/* Set threshold to 0xa (64 bytes) + 1 to workaround pause bug  */
	sky2_write16(hw, SK_REG(port, RX_GMF_FL_THR), RX_GMF_FL_THR_DEF+1);

	/* Configure Tx MAC FIFO */
	sky2_write8(hw, SK_REG(port, TX_GMF_CTRL_T), GMF_RST_CLR);
	sky2_write16(hw, SK_REG(port, TX_GMF_CTRL_T), GMF_OPER_ON);

	if (hw->chip_id == CHIP_ID_YUKON_EC_U || hw->chip_id == CHIP_ID_YUKON_EX) {
		sky2_write8(hw, SK_REG(port, RX_GMF_LP_THR), 768/8);
		sky2_write8(hw, SK_REG(port, RX_GMF_UP_THR), 1024/8);
	}
}


/* Bring up network interface. */
static int sky2_up(struct eth_device *dev)
{
	struct sky2_priv_data * pri;
	pri = (struct sky2_priv_data *) dev->priv;

	struct sky2_port *sky2 = pri->driver_data;
	struct sky2_hw *hw = sky2->hw;
	unsigned port = sky2->port;
	uint32_t ramsize;
	int err = -1;

	dprint("%s: enabling interface\n", dev->name);

	/* Must be a power of 2 */
	sky2->tx_le = aligned_malloc(TX_RING_SIZE * sizeof(struct sky2_tx_le));
	sky2->tx_le_map = virt_to_phys(sky2->tx_le);

	if (!sky2->tx_le)
		goto err_out;

	sky2->tx_ring = aligned_malloc(TX_RING_SIZE * sizeof(struct tx_ring_info));
	if (!sky2->tx_ring)
		goto err_out;
	memset(sky2->tx_ring, 0, TX_RING_SIZE * sizeof(struct tx_ring_info));

	sky2->tx_prod = sky2->tx_cons = 0;

	sky2->rx_le = aligned_malloc(RX_LE_BYTES);
	sky2->rx_le_map = virt_to_phys(sky2->rx_le);

	if (!sky2->rx_le)
		goto err_out;
	memset(sky2->rx_le, 0, RX_LE_BYTES);

	sky2->rx_ring = aligned_malloc(sky2->rx_pending * sizeof(struct rx_ring_info));
	if (!sky2->rx_ring)
		goto err_out;
	memset(sky2->rx_ring, 0, sky2->rx_pending * sizeof(struct rx_ring_info));

	sky2_phy_power(hw, port, 1);
	sky2_mac_init(hw, port);

	/* Register is number of 4K blocks on internal RAM buffer. */
	ramsize = sky2_read8(hw, B2_E_0) * 4;
	dprint("%s: ram buffer %dK\n", dev->name, ramsize);

	if (ramsize > 0) {
		uint32_t rxspace;

		if (ramsize < 16)
			rxspace = ramsize / 2;
		else
			rxspace = 8 + (2*(ramsize - 16))/3;

		sky2_ramset(hw, rxqaddr[port], 0, rxspace);
		sky2_ramset(hw, txqaddr[port], rxspace, ramsize - rxspace);

		/* Make sure SyncQ is disabled */
		sky2_write8(hw, RB_ADDR(port == 0 ? Q_XS1 : Q_XS2, RB_CTRL),
			    RB_RST_SET);
	}

	sky2_qset(hw, txqaddr[port]);

	/* Set almost empty threshold */
	if (hw->chip_id == CHIP_ID_YUKON_EC_U
	    && hw->chip_rev == CHIP_REV_YU_EC_U_A0)
		sky2_write16(hw, Q_ADDR(txqaddr[port], Q_AL), 0x1a0);

	sky2_prefetch_init(hw, txqaddr[port], sky2->tx_le_map,
			   TX_RING_SIZE - 1);

	err = sky2_rx_start(sky2);
	if (err)
		goto err_out;
	return 0;

err_out:
	if (sky2->rx_le) {
		aligned_free(sky2->rx_le);
		sky2->rx_le = NULL;
	}
	if (sky2->tx_le) {
		aligned_free(sky2->tx_le);
		sky2->tx_le = NULL;
	}

	if(sky2->tx_ring)
		aligned_free(sky2->tx_ring);
	if(sky2->rx_ring)
		aligned_free(sky2->rx_ring);

	sky2->tx_ring = NULL;
	sky2->rx_ring = NULL;
	return err;
}

static void sky2_rx_stop(struct sky2_port *sky2)
{
	struct sky2_hw *hw = sky2->hw;
	unsigned rxq = rxqaddr[sky2->port];
	int i;

	/* disable the RAM Buffer receive queue */
	sky2_write8(hw, RB_ADDR(rxq, RB_CTRL), RB_DIS_OP_MD);

	for (i = 0; i < 0xffff; i++)
		if (sky2_read8(hw, RB_ADDR(rxq, Q_RSL))
		    == sky2_read8(hw, RB_ADDR(rxq, Q_RL)))
			goto stopped;

	printf("%s: receiver stop failed\n",
	       sky2->netdev->name);
stopped:
	sky2_write32(hw, Q_ADDR(rxq, Q_CSR), BMU_RST_SET | BMU_FIFO_RST);

	/* reset the Rx prefetch unit */
	sky2_write32(hw, Y2_QADDR(rxq, PREF_UNIT_CTRL), PREF_UNIT_RST_SET);
}

/* Bring down the interface */
static int sky2_down(struct eth_device *dev) __attribute__ ((unused));
static int sky2_down(struct eth_device *dev)
{
	struct sky2_priv_data *pri;
	pri = (struct sky2_priv_data *) dev->priv;

	struct sky2_port *sky2 = pri->driver_data;
	struct sky2_hw *hw = sky2->hw;
	unsigned port = sky2->port;
	u32 imask;
	if (!sky2->tx_le)
		return 0;
	
	/* Disable port IRQ */
	imask = sky2_read32(hw, B0_IMSK);
	imask &= ~portirq_msk[port];
	sky2_write32(hw, B0_IMSK, imask);
	
	sky2_rx_stop(sky2);

	/*Clean the rx buffers.*/
	sky2_rx_clean(sky2);

	if (sky2->rx_le) {
		aligned_free(sky2->rx_le);
		sky2->rx_le = NULL;
	}
	if (sky2->tx_le) {
		aligned_free(sky2->tx_le);
		sky2->tx_le = NULL;
	}

	if(sky2->tx_ring)
		aligned_free(sky2->tx_ring);
	if(sky2->rx_ring)
		aligned_free(sky2->rx_ring);

	sky2->tx_ring = NULL;
	sky2->rx_ring = NULL;
	
	return 0;

}


/* Modular subtraction in ring */
static inline int tx_dist(unsigned tail, unsigned head)
{
		return (head - tail) & (TX_RING_SIZE - 1);
}

/* Number of list elements available for next tx */
static inline int tx_avail(const struct sky2_port *sky2)
{
	return sky2->tx_pending - tx_dist(sky2->tx_cons, sky2->tx_prod);
}

static inline struct sky2_tx_le *get_tx_le(struct sky2_port *sky2)
{
	struct sky2_tx_le *le = sky2->tx_le + sky2->tx_prod;

	sky2->tx_prod = RING_NEXT(sky2->tx_prod, TX_RING_SIZE);
	le->ctrl = 0;
	return le;
}

/* Update chip's next pointer */
static inline void sky2_put_idx(struct sky2_hw *hw, unsigned q, u16 idx)
{
	q = Y2_QADDR(q, PREF_UNIT_PUT_IDX);

	mips_sync();
	sky2_write16(hw, q, idx);
	sky2_read16(hw, q);
}

static inline struct tx_ring_info *tx_le_re(struct sky2_port *sky2,
					    struct sky2_tx_le *le)
{
	return sky2->tx_ring + (le - sky2->tx_le);
}


/*
 * Put one packet in ring for transmit.
 * A single packet can generate multiple list elements, and
 * the number of ring elements will probably be less than the number
 * of list elements used.
 */
static int sky2_xmit_frame(void *skb, struct eth_device *dev, unsigned int len)
{
	struct sky2_priv_data * pri;
	pri = (struct sky2_priv_data *) dev->priv;

	struct sky2_port *sky2 = pri->driver_data;
	struct sky2_hw *hw = sky2->hw;
	struct sky2_tx_le *le = NULL;
	struct tx_ring_info *re;
	uint32_t addr;
	uint8_t ctrl = 0;

	/* Assume only one List Element is required */
 	if (tx_avail(sky2) < 1) {
		printf("No Tx list element available\n");
  		return 0;
	}

	/* OpenTCP stack offsets actual data by 2 bytes */
	/* addr = virt_to_phys(skb->data + 2); */
	addr = virt_to_phys(skb);

	le = get_tx_le(sky2);
	le->addr = cpu_to_le32(addr);
	le->length = cpu_to_le16((uint16_t)len);
	le->ctrl = ctrl;
	le->opcode = OP_PACKET | HW_OWNER;

	re = tx_le_re(sky2, le);
	re->skb = skb;
	
	le->ctrl |= EOP;

	sky2_put_idx(hw, txqaddr[sky2->port], sky2->tx_prod);	
	dprint("tx_frame: le->addr = %#x, le->length = %#x,  le->ctrl = %#x, "
	       "le->opcode = %#x\n",
			(uint32_t)addr, le->length, le->ctrl, le->opcode);
	dprint("sky2->tx_prod = %#x\n", sky2->tx_prod);


	return 1;
}


/* Access Routines */
void dummy_msgring_handle(struct eth_device *dev)
{
	/* Do nothing */
}


int sky2_send(struct eth_device *dev, volatile void *packet, int length)
{
	int status;

/* Not required, since we use u-boot's NetTxPacket to send
	if (!sky2->link_up_done) {
		free_pbuf(pbuf);	
		return 0;
	}
*/

	status = sky2_xmit_frame((void *)packet, dev, length);

	if (!status) {
		printf("sky2_send: Error sending packet\n");
		return -1;
	}
	return 0;
}

static void sky2_hw_error(struct sky2_hw *hw, unsigned port, uint32_t status)
{
	struct eth_device *dev = hw->dev[port];

		printf("%s: hw error interrupt status 0x%x\n",
		       dev->name, status);

	if (status & Y2_IS_PAR_RD1) {
		printf("%s: ram data read parity error\n",
				dev->name);
		/* Clear IRQ */
		sky2_write16(hw, RAM_BUFFER(port, B3_RI_CTRL), RI_CLR_RD_PERR);
	}

	if (status & Y2_IS_PAR_WR1) {
		printf("%s: ram data write parity error\n", dev->name);

		sky2_write16(hw, RAM_BUFFER(port, B3_RI_CTRL), RI_CLR_WR_PERR);
	}

	if (status & Y2_IS_PAR_MAC1) {
		printf("%s: MAC parity error\n", dev->name);
		sky2_write8(hw, SK_REG(port, TX_GMF_CTRL_T), GMF_CLI_TX_PE);
	}

	if (status & Y2_IS_PAR_RX1) {
		printf("%s: RX parity error\n", dev->name);
		sky2_write32(hw, Q_ADDR(rxqaddr[port], Q_CSR), BMU_CLR_IRQ_PAR);
	}

	if (status & Y2_IS_TCP_TXA1) {
		printf("%s: TCP segmentation error\n", dev->name);
		sky2_write32(hw, Q_ADDR(txqaddr[port], Q_CSR), BMU_CLR_IRQ_TCP);
	}
}

static void sky2_hw_intr(struct sky2_hw *hw)
{
	uint32_t status = sky2_read32(hw, B0_HWE_ISRC);

	if (status & Y2_IS_TIST_OV)
		sky2_write8(hw, GMAC_TI_ST_CTRL, GMT_ST_CLR_IRQ);

	if (status & (Y2_IS_MST_ERR | Y2_IS_IRQ_STAT)) {
		uint16_t pci_err;

		pci_err = sky2_pci_read16(hw, PCI_STATUS);
		printf("PCI hardware error (0x%x)\n", pci_err);

		sky2_write8(hw, B2_TST_CTRL1, TST_CFG_WRITE_ON);
		sky2_pci_write16(hw, PCI_STATUS,
				 pci_err | PCI_STATUS_ERROR_BITS);
		sky2_write8(hw, B2_TST_CTRL1, TST_CFG_WRITE_OFF);
	}

	if (status & Y2_IS_PCI_EXP) {
		/* PCI-Express uncorrectable Error occurred */
		uint32_t pex_err;

		pex_err = sky2_pci_read32(hw, PEX_UNC_ERR_STAT);

		printf("PCI Express error (0x%x)\n", pex_err);

		/* clear the interrupt */
		sky2_write32(hw, B2_TST_CTRL1, TST_CFG_WRITE_ON);
		sky2_pci_write32(hw, PEX_UNC_ERR_STAT,
				       0xffffffffUL);
		sky2_write32(hw, B2_TST_CTRL1, TST_CFG_WRITE_OFF);

		if (pex_err & PEX_FATAL_ERRORS) {
			uint32_t hwmsk = sky2_read32(hw, B0_HWE_IMSK);
			hwmsk &= ~Y2_IS_PCI_EXP;
			sky2_write32(hw, B0_HWE_IMSK, hwmsk);
		}
	}

	if (status & Y2_HWE_L1_MASK)
		sky2_hw_error(hw, 0, status);
	/* We have only one port 
		status >>= 8;
		if (status & Y2_HWE_L1_MASK)
			sky2_hw_error(hw, 1, status);
	*/
}

static void sky2_mac_intr(struct sky2_hw *hw, unsigned port)
{
	struct eth_device *dev = hw->dev[port];
	uint8_t status = sky2_read8(hw, SK_REG(port, GMAC_IRQ_SRC));

	printf("%s: mac interrupt status 0x%x\n", dev->name, status);

	if (status & GM_IS_RX_FF_OR) {
		sky2_write8(hw, SK_REG(port, RX_GMF_CTRL_T), GMF_CLI_RX_FO);
	}

	if (status & GM_IS_TX_FF_UR) {
		sky2_write8(hw, SK_REG(port, TX_GMF_CTRL_T), GMF_CLI_TX_FU);
	}
}

static void sky2_link_down(struct sky2_port *sky2)
{
	struct sky2_hw *hw = sky2->hw;
	unsigned port = sky2->port;
	uint16_t reg;

	gm_phy_write(hw, port, PHY_MARV_INT_MASK, 0);

	reg = gma_read16(hw, port, GM_GP_CTRL);
	reg &= ~(GM_GPCR_RX_ENA | GM_GPCR_TX_ENA);
	gma_write16(hw, port, GM_GP_CTRL, reg);

	/* Turn on link LED */
	sky2_write8(hw, SK_REG(port, LNK_LED_REG), LINKLED_OFF);

	printf("%s: Link is down.\n", sky2->netdev->name);

	sky2_phy_init(hw, port);
}

/* This should never happen it is a fatal situation */
static void sky2_descriptor_error(struct sky2_hw *hw, unsigned port,
				  const char *rxtx, uint32_t mask)
{
	struct eth_device *dev = hw->dev[port];
	struct sky2_priv_data * pri;

	pri = (struct sky2_priv_data *) dev->priv;
	struct sky2_port *sky2 = pri->driver_data;
	uint32_t imask;

	printf("%s: %s descriptor error (hardware problem)\n",
	       dev ? dev->name : "<not registered>", rxtx); 

	imask = sky2_read32(hw, B0_IMSK);
	imask &= ~mask;
	sky2_write32(hw, B0_IMSK, imask);

	if (dev) {
		sky2_link_down(sky2);
	}
}

static struct packet * sky2_receive(struct eth_device *dev, 
		uint16_t length, uint32_t status)
{
	struct sky2_priv_data * pri;
	pri = (struct sky2_priv_data *)dev->priv;

	struct sky2_port *sky2 = pri->driver_data;
	struct rx_ring_info *re = sky2->rx_ring + sky2->rx_next;
	struct packet *skb = NULL, *rx_skb = NULL;

	dprint("%s: rx slot %u status 0x%x len %d\n", 
			dev->name, sky2->rx_next, status, length);

	sky2->rx_next = (sky2->rx_next + 1) % sky2->rx_pending;

	if (status & GMR_FS_ANY_ERR)
		goto error;

	if (!(status & GMR_FS_RX_OK))
		goto resubmit;

	skb = alloc_pbuf();
	if (!skb) {
		printf("Out of pbufs\n");
		return NULL;
	}

	rx_skb = re->skb;
	
	/* OpenTCP needs half world aligned ethernet pkt */
	memmove(rx_skb->data + 2, rx_skb->data, length);

	re->skb = skb; 
	re->data_addr = virt_to_phys(skb->data);

resubmit:
	sky2_rx_submit(sky2, re);

	return rx_skb;

error:
	if (status & GMR_FS_RX_FF_OV) {
		goto resubmit;
	}

	dprint("%s: rx error, status 0x%x length %d\n",
			dev->name, status, length);

	goto resubmit;
}

/*
 * Free ring elements from starting at tx_cons until "done"
 *
 * NB: the hardware will tell us about partial completion of multi-part
 *     buffers so make sure not to free skb to early.
 */
static void sky2_tx_complete(struct sky2_port *sky2, u16 done)
{
	unsigned idx;

	if (done >= TX_RING_SIZE)
		printf("sky2_tx_complete: Error done >= TX_RING_SIZE\n");

	for (idx = sky2->tx_cons; idx != done;
	     idx = RING_NEXT(idx, TX_RING_SIZE)) {
		struct sky2_tx_le *le = sky2->tx_le + idx;

		switch(le->opcode & ~HW_OWNER) {
		case OP_LARGESEND:
		case OP_PACKET:
		case OP_BUFFER:
			dprint("sky2_tx_complete \n");
			break;
		}
		/* free_pbuf(re->skb); not required, since we use TxPacket */
		le->opcode = 0;	/* paranoia */
	}
	sky2->tx_cons = idx;
}

/* Transmit complete */
static inline void sky2_tx_done(struct eth_device *dev, u16 last)
{
	struct sky2_priv_data * pri;
	pri = (struct sky2_priv_data *) dev->priv;

	struct sky2_port *sky2 = pri->driver_data;

		sky2_tx_complete(sky2, last);
}

static struct packet * sky2_status_intr(struct sky2_hw *hw, int *len)
{
	struct sky2_port *sky2;
	unsigned buf_write[2] = { 0, 0 };
	uint16_t hwidx = sky2_read16(hw, STAT_PUT_IDX);
	struct packet *skb = NULL;

	mips_sync();


	while (hw->st_idx != hwidx) {
		struct sky2_status_le *le  = hw->st_le + hw->st_idx;
		struct eth_device *dev;
		uint32_t status;
		uint16_t length;

		dprint("sky2_status_intr: hw->st_idx = %d hwidx = %d\n",
				hw->st_idx, hwidx);

		hw->st_idx = RING_NEXT(hw->st_idx, STATUS_RING_SIZE);

		if (le->link >= 2) {
			printf("ERROR: le->link > 2\n");
		}
		dev = hw->dev[le->link];

		sky2 = ((struct sky2_priv_data*)dev->priv)->driver_data;
		length = le16_to_cpu(le->length);
		*len = length;
		status = le32_to_cpu(le->status);

		switch (le->opcode & ~HW_OWNER) {
		case OP_RXSTAT:
			skb = sky2_receive(dev, length, status);
			if (!skb)
				goto force_update;
			/* Update receiver after 16 frames */
			if (++buf_write[le->link] == RX_BUF_WRITE) {
force_update:
				sky2_put_idx(hw, rxqaddr[le->link], sky2->rx_put);
				buf_write[le->link] = 0;
			}
			
			/* Exit if 1 pkt is received */
			goto exit_loop;
			break;

		case OP_TXINDEXLE:
			/* TX index reports status for both ports */
			sky2_tx_done(hw->dev[0], status & 0xfff); 
			break;

		case OP_RXVLAN:
		case OP_RXCHKSVLAN:
		case OP_RXCHKS:
			printf("Unhandled status opcode %#x\n", le->opcode);
			break;
	
		default:
			printf("Error: Unknown status opcode %#x\n", le->opcode);
			goto exit_loop;
		}
	}

	/* Fully processed status ring so clear irq */
	sky2_write32(hw, SKY2_STAT_CTRL, SC_STAT_CLR_IRQ);

exit_loop:
	if (buf_write[0]) {
		sky2 = ((struct sky2_priv_data*) hw->dev[0]->priv)->driver_data;
		sky2_put_idx(hw, Q_R1, sky2->rx_put);
	}

	return skb;
}

static u16 sky2_phy_speed(const struct sky2_hw *hw, u16 aux)
{
	if (!sky2_is_copper(hw))
		return SPEED_1000;

	if (hw->chip_id == CHIP_ID_YUKON_FE)
		return (aux & PHY_M_PS_SPEED_100) ? SPEED_100 : SPEED_10;

	switch (aux & PHY_M_PS_SPEED_MSK) {
	case PHY_M_PS_SPEED_1000:
		return SPEED_1000;
	case PHY_M_PS_SPEED_100:
		return SPEED_100;
	default:
		return SPEED_10;
	}
}


static void sky2_link_up(struct sky2_port *sky2)
{
	struct sky2_hw *hw = sky2->hw;
	unsigned port = sky2->port;
	uint16_t reg;
	static const char *fc_name[] = {
		[FC_NONE]	= "none",
		[FC_TX]		= "tx",
		[FC_RX]		= "rx",
		[FC_BOTH]	= "both",
	};

	/* enable Rx/Tx */
	reg = gma_read16(hw, port, GM_GP_CTRL);
	reg |= GM_GPCR_RX_ENA | GM_GPCR_TX_ENA;
	gma_write16(hw, port, GM_GP_CTRL, reg);

	gm_phy_write(hw, port, PHY_MARV_INT_MASK, PHY_M_DEF_MSK);

	/* Turn on link LED */
	sky2_write8(hw, SK_REG(port, LNK_LED_REG),
		    LINKLED_ON | LINKLED_BLINK_OFF | LINKLED_LINKSYNC_OFF);

	if (hw->chip_id == CHIP_ID_YUKON_XL
	    || hw->chip_id == CHIP_ID_YUKON_EC_U
	    || hw->chip_id == CHIP_ID_YUKON_EX) {
		u16 pg = gm_phy_read(hw, port, PHY_MARV_EXT_ADR);
		u16 led = PHY_M_LEDC_LOS_CTRL(1);	/* link active */

		switch(sky2->speed) {
		case SPEED_10:
			led |= PHY_M_LEDC_INIT_CTRL(7);
			break;

		case SPEED_100:
			led |= PHY_M_LEDC_STA1_CTRL(7);
			break;

		case SPEED_1000:
			led |= PHY_M_LEDC_STA0_CTRL(7);
			break;
		}

		gm_phy_write(hw, port, PHY_MARV_EXT_ADR, 3);
		gm_phy_write(hw, port, PHY_MARV_PHY_CTRL, led);
		gm_phy_write(hw, port, PHY_MARV_EXT_ADR, pg);
	}

	dprint("sky2_link_up: %#x %#x %#x\n", 
			gm_phy_read(hw, port, PHY_MARV_EXT_ADR),
			gm_phy_read(hw, port, PHY_MARV_PHY_CTRL),
			gm_phy_read(hw, port, PHY_MARV_EXT_ADR));


	printf("%s: Link is up at %d Mbps, %s duplex, flow control %s\n",
			sky2->netdev->name, sky2->speed,
			sky2->duplex == DUPLEX_FULL ? "full" : "half",
			fc_name[sky2->flow_status]);
	sky2->link_up_done = 1;
}

static int sky2_autoneg_done(struct sky2_port *sky2, u16 aux)
{
	struct sky2_hw *hw = sky2->hw;
	unsigned port = sky2->port;
	u16 advert, lpa;

	advert = gm_phy_read(hw, port, PHY_MARV_AUNE_ADV);
	lpa = gm_phy_read(hw, port, PHY_MARV_AUNE_LP);
	if (lpa & PHY_M_AN_RF) {
		printf("%s: remote fault", sky2->netdev->name);
		return -1;
	}

	if (!(aux & PHY_M_PS_SPDUP_RES)) {
		printf("%s: speed/duplex mismatch",
		       sky2->netdev->name);
		return -1;
	}

	sky2->speed = sky2_phy_speed(hw, aux);
	sky2->duplex = (aux & PHY_M_PS_FULL_DUP) ? DUPLEX_FULL : DUPLEX_HALF;

	/* Since the pause result bits seem to in different positions on
	 * different chips. look at registers.
	 */
	if (!sky2_is_copper(hw)) {
		/* Shift for bits in fiber PHY */
		advert &= ~(ADVERTISE_PAUSE_CAP|ADVERTISE_PAUSE_ASYM);
		lpa &= ~(LPA_PAUSE_CAP|LPA_PAUSE_ASYM);

		if (advert & ADVERTISE_1000XPAUSE)
			advert |= ADVERTISE_PAUSE_CAP;
		if (advert & ADVERTISE_1000XPSE_ASYM)
			advert |= ADVERTISE_PAUSE_ASYM;
		if (lpa & LPA_1000XPAUSE)
			lpa |= LPA_PAUSE_CAP;
		if (lpa & LPA_1000XPAUSE_ASYM)
			lpa |= LPA_PAUSE_ASYM;
	}

	sky2->flow_status = FC_NONE;
	if (advert & ADVERTISE_PAUSE_CAP) {
		if (lpa & LPA_PAUSE_CAP)
			sky2->flow_status = FC_BOTH;
		else if (advert & ADVERTISE_PAUSE_ASYM)
			sky2->flow_status = FC_RX;
	} else if (advert & ADVERTISE_PAUSE_ASYM) {
		if ((lpa & LPA_PAUSE_CAP) && (lpa & LPA_PAUSE_ASYM))
			sky2->flow_status = FC_TX;
	}

	if (sky2->duplex == DUPLEX_HALF && sky2->speed < SPEED_1000
	    && !(hw->chip_id == CHIP_ID_YUKON_EC_U || hw->chip_id == CHIP_ID_YUKON_EX))
		sky2->flow_status = FC_NONE;

	if (sky2->flow_status & FC_TX)
		sky2_write8(hw, SK_REG(port, GMAC_CTRL), GMC_PAUSE_ON);
	else
		sky2_write8(hw, SK_REG(port, GMAC_CTRL), GMC_PAUSE_OFF);

	return 0;
}


/* Interrupt from PHY */
static void sky2_phy_intr(struct sky2_hw *hw, unsigned port)
{
	struct eth_device *dev = hw->dev[port];
	struct sky2_port *sky2 = ((struct sky2_priv_data *)dev->priv)->driver_data;
	uint16_t istatus, phystat;

	istatus = gm_phy_read(hw, port, PHY_MARV_INT_STAT);
	phystat = gm_phy_read(hw, port, PHY_MARV_PHY_STAT);

	if (sky2->autoneg == AUTONEG_ENABLE && (istatus & PHY_M_IS_AN_COMPL)) {
		if (sky2_autoneg_done(sky2, phystat) == 0)
			sky2_link_up(sky2);
		return;
	}

	if (istatus & PHY_M_IS_LSP_CHANGE)
		sky2->speed = sky2_phy_speed(hw, phystat);

	if (istatus & PHY_M_IS_DUP_CHANGE)
		sky2->duplex =
		    (phystat & PHY_M_PS_FULL_DUP) ? DUPLEX_FULL : DUPLEX_HALF;
	if (istatus & PHY_M_IS_LST_CHANGE) {
		if (phystat & PHY_M_PS_LINK_UP)
			sky2_link_up(sky2);
		else
			sky2_link_down(sky2);
	}
}

int sky2_poll(struct eth_device *dev)
{
	unsigned int len = 0;

	struct packet *pbuf = NULL;
	struct sky2_port *sky2 = ((struct sky2_priv_data *)dev->priv)->driver_data;
   struct sky2_hw *hw = sky2->hw;

	uint32_t status = sky2_read32(hw, B0_Y2_SP_EISR);
	int tx, rx;

	if (status)
		dprint("sky2_poll: status = %#x\n", status);

	tx = sky2_read32(hw, RB_ADDR(txqaddr[0], RB_PC));
	rx = sky2_read32(hw, RB_ADDR(txqaddr[0], RB_PC));

	if (status & Y2_IS_IRQ_PHY1)
		sky2_phy_intr(hw, 0);

	if (status & Y2_IS_IRQ_MAC1)
		sky2_mac_intr(hw, 0);

	if (!sky2->link_up_done)
		return -1;

	if (status & Y2_IS_HW_ERR)
		sky2_hw_intr(hw);


	if (status & Y2_IS_CHK_RX1)
		sky2_descriptor_error(hw, 0, "receive", Y2_IS_CHK_RX1);

	if (status & Y2_IS_CHK_TXA1)
		sky2_descriptor_error(hw, 0, "transmit", Y2_IS_CHK_TXA1);

	/* receive one pkt if available */
	pbuf = sky2_status_intr(hw, &len);
	if (pbuf) {
		dprint("sky2_poll: pbuf = %p\n", (void *) pbuf);
		dprint("pbuf len = %d\n", len);
		/* print_packet_pci(&pbuf->data[2],len); */

		/* give packet to higher level routine */
		NetReceive((pbuf->data)+2, len);
		free_pbuf(pbuf);
	}

	return 1;
}

int sky2_tx_free(struct eth_device *dev)
{
/*
	Not required, since we use u-boot's NetTxPacket to send.

	struct packet *pbuf __unused = ((sky2_tx_free_args_t *)arg)->pbuf;
	struct eth_device *dev __unused = ((sky2_tx_free_args_t *)arg)->dev;

	printf("\n sky2_tx_free: pbuf = %p\n", (void *)pbuf);
	comment: Send the packet to MAC
	free_pbuf(pbuf);
*/

	return 1;
}

int sky2_reset(struct eth_device *dev)
{
	struct sky2_priv_data * pri;
	pri = (struct sky2_priv_data *)dev->priv;

	struct sky2_port *sky2 = pri->driver_data;

	_sky2_reset(sky2->hw);

	return 1;

}

static void sky2_dump_buff_info (struct sky2_port * sky2)
{
#ifdef SKY2_DEBUG
	struct sky2_hw *hw = sky2->hw;
#endif

	dprint("sky2_hw\n");
	dprint("sky2_hw->regs = %p\n", (void *)hw->regs);
	dprint("sky2_hw->pdev = %p\n", (void *)hw->pdev);
	dprint("sky2_hw->eth_device[0,1] = %p, %p\n",
			(void *)hw->dev[0], (void *)hw->dev[1]);
	dprint("sky_hw->[chip_id, chip_rev, pmd_type, ports] = "
			"%d, %d, %d, %d\n",
			hw->chip_id, hw->chip_rev, hw->pmd_type, hw->ports);
	dprint("sky2_hw->st_le = %p, st_idx = %#x, st_dma = %#x \n", 
			(void *) (hw->st_le), hw->st_idx, hw->st_dma);
	dprint("sky2_hw->msi = %#x\n", hw->msi);

	dprint("sky2_port \n");

	dprint("sky2_port->[port, msg_enable] = %d, %d\n", sky2->port,
			sky2->msg_enable);
	dprint("sky2_port->[tx_ring, tx_le, tx_cons, tx_prod] ="
			" %p, %p, %#x, %#x\n",
			(void *)sky2->tx_ring, (void *)sky2->tx_le,
			sky2->tx_cons, sky2->tx_prod);
	dprint("sky2_port->[tx_addr64, tx_pending, tx_last_mss, tx_tcpsum] = "
			"%d, %d, %d, %d\n",
			sky2->tx_addr64, sky2->tx_pending, sky2->tx_last_mss,
			sky2->tx_tcpsum);

	dprint("sky2_port->rx_ring = %p, sky2_port->rx_le = %p\n",
			(void *)sky2->rx_ring, (void *)sky2->rx_le);

	dprint("sky2_port->[rx_addr64, rx_next, rx_put, rx_pending,  "
			"rx_data_size, rx_nfrags = %d, %d, %d, %d, %d, %d\n",
			sky2->rx_addr64, sky2->rx_next, sky2->rx_put,
			sky2->rx_pending, sky2->rx_data_size, sky2->rx_nfrags);

	dprint("sky2->[rx_le_map = %#x, tx_le_map = %#x]\n",
			sky2->rx_le_map, sky2->tx_le_map);
}


int sky2_open(struct eth_device *dev)
{
	struct sky2_port *sky2 = ((struct sky2_priv_data *)dev->priv)->driver_data;

	DENTER;

	sky2_up(dev);
	sky2_dump_buff_info(sky2);
	return 1;
}

void sky2_close(struct eth_device *dev)
{
	/* disable the receiver */

	struct sky2_priv_data * pri;
	struct sky2_port *sky2;
	struct sky2_hw *hw;
	uint16_t gpcontrol;
   unsigned port;
 
   pri = (struct sky2_priv_data *) dev->priv; 
   sky2 = pri->driver_data;
   port = sky2->port;
   hw = sky2->hw;

	gpcontrol = gma_read16(hw, port, GM_GP_CTRL);
	gpcontrol &= ~GM_GPCR_RX_ENA;
	gma_write16(hw, port, GM_GP_CTRL, gpcontrol);

	return;
}
	
#define PCI_VENDOR_ID_D_LINK 0x1186
#define PCI_DEVICE_ID_DL_4B00 0X4b00

static struct pci_device_id supported[] = {
   {PCI_VENDOR_ID_D_LINK, PCI_DEVICE_ID_DL_4B00},
   {}
}; 

int rmisky2_initialize (bd_t * bis)
{
   pci_dev_t devno;
	uint32_t iobase, status;
	int idx = 0;
	static int card_number = 0;
	struct eth_device *dev;
	struct sky2_priv_data *priv;

	while (1)
	{
		if ((devno = pci_find_devices(supported, idx++)) < 0)
			break;

		pci_read_config_dword(devno, PCI_BASE_ADDRESS_0, &iobase);
		iobase &= ~0x7;  /* ignore last 3 bits */

		pci_read_config_dword (devno, PCI_COMMAND, &status);
		status |= PCI_COMMAND_MASTER | PCI_COMMAND_IO | PCI_COMMAND_MEMORY;		
      pci_write_config_dword(devno, PCI_COMMAND, status);

      /* Check if I/O accesses and Bus Mastering are enabled. */
      pci_read_config_dword(devno, PCI_COMMAND, &status);
      if (!(status & PCI_COMMAND_MEMORY)) { 
         printf("Error: Can not enable MEM access.\n");
         continue;
      } else if (!(status & PCI_COMMAND_MASTER)) {
         printf("Error: Can not enable Bus Mastering.\n");
         continue;
      }

      dev = (struct eth_device *) malloc(sizeof *dev);
		priv = (struct sky2_priv_data *) malloc(sizeof *priv);

		if ((!dev) || (!priv)) {
			printf ("** [%s]: Unable to malloc!\n", __FUNCTION__);
			return -1;
		}

      sprintf(dev->name, "dlink#%d", card_number);
      card_number++;

      dev->iobase = iobase;
      printf("Dlink @ %#x\n", dev->iobase);
      dev->init = sky2_initialize;
      dev->halt = sky2_close;
      dev->send = sky2_send;
      dev->recv = sky2_poll;
		dev->state = 0;

		priv->devno = devno;
		priv->init_done = 0;

      dev->priv = (void *) priv;

      eth_register(dev);
	}
	return 1;
}
