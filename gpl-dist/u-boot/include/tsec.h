/*
 *  tsec.h
 *
 *  Driver for the Motorola Triple Speed Ethernet Controller
 *
 *  This software may be used and distributed according to the
 *  terms of the GNU Public License, Version 2, incorporated
 *  herein by reference.
 *
 * Copyright 2004, 2007, 2009, 2011  Freescale Semiconductor, Inc.
 * (C) Copyright 2003, Motorola, Inc.
 * maintained by Xianghua Xiao (x.xiao@motorola.com)
 * author Andy Fleming
 *
 */

#ifndef __TSEC_H
#define __TSEC_H

#include <net.h>
#include <config.h>
#include <phy.h>
#include <asm/fsl_enet.h>

#define TSEC_SIZE 		0x01000
#define TSEC_MDIO_OFFSET	0x01000

#define CONFIG_SYS_MDIO_BASE_ADDR (MDIO_BASE_ADDR + 0x520)

#define DEFAULT_MII_NAME "FSL_MDIO"

#define STD_TSEC_INFO(num) \
{			\
	.regs = (tsec_t *)(TSEC_BASE_ADDR + ((num - 1) * TSEC_SIZE)), \
	.miiregs_sgmii = (struct tsec_mii_mng *)(CONFIG_SYS_MDIO_BASE_ADDR \
					 + (num - 1) * TSEC_MDIO_OFFSET), \
	.devname = CONFIG_TSEC##num##_NAME, \
	.phyaddr = TSEC##num##_PHY_ADDR, \
	.flags = TSEC##num##_FLAGS, \
	.mii_devname = DEFAULT_MII_NAME \
}

/* JUNOS add x.hw_unit = num; */
#define SET_STD_TSEC_INFO(x, num) \
{                       \
	x.regs = (tsec_t *)(TSEC_BASE_ADDR + ((num - 1) * TSEC_SIZE)); \
	x.miiregs_sgmii = (struct tsec_mii_mng *)(CONFIG_SYS_MDIO_BASE_ADDR \
					+ (num - 1) * TSEC_MDIO_OFFSET); \
	x.devname = CONFIG_TSEC##num##_NAME; \
	x.phyaddr = TSEC##num##_PHY_ADDR; \
	x.flags = TSEC##num##_FLAGS;\
	x.mii_devname = DEFAULT_MII_NAME;\
	x.hw_unit = num; \
}

/* JUNOS Start */
#define SET_STD_TSECV2_INFO(x, num) \
{			\
	x.regs = (tsec_t *)(TSECV2_BASE_ADDR + ((num - 1) * TSEC_SIZE)); \
	x.miiregs_sgmii = (struct tsec_mii_mng *)(CONFIG_SYS_MDIO_BASE_ADDR \
					+ (num - 1) * TSEC_MDIO_OFFSET); \
	x.devname = CONFIG_TSEC##num##_NAME; \
	x.phyaddr = TSEC##num##_PHY_ADDR; \
	x.flags = TSEC##num##_FLAGS;\
	x.mii_devname = DEFAULT_MII_NAME;\
	x.hw_unit = num; \
}
/* JUNOS End */

#define MAC_ADDR_LEN 6

/* #define TSEC_TIMEOUT	1000000 */
#define TSEC_TIMEOUT 1000
#define TOUT_LOOP	1000000

#define PHY_AUTONEGOTIATE_TIMEOUT       5000 /* in ms */

/* TBI register addresses */
#define TBI_CR			0x00
#define TBI_SR			0x01
#define TBI_ANA			0x04
#define TBI_ANLPBPA		0x05
#define TBI_ANEX		0x06
#define TBI_TBICON		0x11

/* TBI MDIO register bit fields*/
#define TBICON_CLK_SELECT	0x0020
#define TBIANA_ASYMMETRIC_PAUSE 0x0100
#define TBIANA_SYMMETRIC_PAUSE  0x0080
#define TBIANA_HALF_DUPLEX	0x0040
#define TBIANA_FULL_DUPLEX	0x0020
#define TBICR_PHY_RESET		0x8000
#define TBICR_ANEG_ENABLE	0x1000
#define TBICR_RESTART_ANEG	0x0200
#define TBICR_FULL_DUPLEX	0x0100
#define TBICR_SPEED1_SET	0x0040

/* 88E1011 PHY Status Register */
#define MIIM_88E1011_PHY_STATUS		0x11
#define MIIM_88E1011_PHYSTAT_SPEED	0xc000
#define MIIM_88E1011_PHYSTAT_GBIT	0x8000
#define MIIM_88E1011_PHYSTAT_100	0x4000
#define MIIM_88E1011_PHYSTAT_DUPLEX	0x2000
#define MIIM_88E1011_PHYSTAT_SPDDONE	0x0800
#define MIIM_88E1011_PHYSTAT_LINK	0x0400

#define MIIMCFG_INIT_VALUE	0x00000003
#define MIIMCFG_RESET		0x80000000

#define MIIMIND_BUSY		0x00000001
#define MIIMIND_NOTVALID	0x00000004

#define MIIM_CR_INIT		0x00001000

/* MAC register bits */
#define MACCFG1_SOFT_RESET	0x80000000
#define MACCFG1_RESET_RX_MC	0x00080000
#define MACCFG1_RESET_TX_MC	0x00040000
#define MACCFG1_RESET_RX_FUN	0x00020000
#define	MACCFG1_RESET_TX_FUN	0x00010000
#define MACCFG1_LOOPBACK	0x00000100
#define MACCFG1_RX_FLOW		0x00000020
#define MACCFG1_TX_FLOW		0x00000010
#define MACCFG1_SYNCD_RX_EN	0x00000008
#define MACCFG1_RX_EN		0x00000004
#define MACCFG1_SYNCD_TX_EN	0x00000002
#define MACCFG1_TX_EN		0x00000001

#define MACCFG2_INIT_SETTINGS	0x00007205
#define MACCFG2_FULL_DUPLEX	0x00000001
#define MACCFG2_IF		0x00000300
#define MACCFG2_GMII		0x00000200
#define MACCFG2_MII		0x00000100

#define ECNTRL_INIT_SETTINGS	0x00001000
#define ECNTRL_TBI_MODE		0x00000020
#define ECNTRL_REDUCED_MODE	0x00000010
#define ECNTRL_R100		0x00000008
#define ECNTRL_REDUCED_MII_MODE	0x00000004
#define ECNTRL_SGMII_MODE	0x00000002

#ifndef CONFIG_SYS_TBIPA_VALUE
    #define CONFIG_SYS_TBIPA_VALUE	0x1f
#endif

#define MRBLR_INIT_SETTINGS	PKTSIZE_ALIGN

#define MINFLR_INIT_SETTINGS	0x00000040

#define DMACTRL_INIT_SETTINGS	0x000000c3
#define DMACTRL_GRS		0x00000010
#define DMACTRL_GTS		0x00000008

#define TSTAT_CLEAR_THALT	0x80000000
#define RSTAT_CLEAR_RHALT	0x00800000


#define IEVENT_INIT_CLEAR	0xffffffff
#define IEVENT_BABR		0x80000000
#define IEVENT_RXC		0x40000000
#define IEVENT_BSY		0x20000000
#define IEVENT_EBERR		0x10000000
#define IEVENT_MSRO		0x04000000
#define IEVENT_GTSC		0x02000000
#define IEVENT_BABT		0x01000000
#define IEVENT_TXC		0x00800000
#define IEVENT_TXE		0x00400000
#define IEVENT_TXB		0x00200000
#define IEVENT_TXF		0x00100000
#define IEVENT_IE		0x00080000
#define IEVENT_LC		0x00040000
#define IEVENT_CRL		0x00020000
#define IEVENT_XFUN		0x00010000
#define IEVENT_RXB0		0x00008000
#define IEVENT_GRSC		0x00000100
#define IEVENT_RXF0		0x00000080

#define IMASK_INIT_CLEAR	0x00000000
#define IMASK_TXEEN		0x00400000
#define IMASK_TXBEN		0x00200000
#define IMASK_TXFEN		0x00100000
#define IMASK_RXFEN0		0x00000080


/* Default Attribute fields */
#define ATTR_INIT_SETTINGS     0x000000c0
#define ATTRELI_INIT_SETTINGS  0x00000000


/* TxBD status field bits */
#define TXBD_READY		0x8000
#define TXBD_PADCRC		0x4000
#define TXBD_WRAP		0x2000
#define TXBD_INTERRUPT		0x1000
#define TXBD_LAST		0x0800
#define TXBD_CRC		0x0400
#define TXBD_DEF		0x0200
#define TXBD_HUGEFRAME		0x0080
#define TXBD_LATECOLLISION	0x0080
#define TXBD_RETRYLIMIT		0x0040
#define	TXBD_RETRYCOUNTMASK	0x003c
#define TXBD_UNDERRUN		0x0002
#define TXBD_STATS		0x03ff

/* RxBD status field bits */
#define RXBD_EMPTY		0x8000
#define RXBD_RO1		0x4000
#define RXBD_WRAP		0x2000
#define RXBD_INTERRUPT		0x1000
#define RXBD_LAST		0x0800
#define RXBD_FIRST		0x0400
#define RXBD_MISS		0x0100
#define RXBD_BROADCAST		0x0080
#define RXBD_MULTICAST		0x0040
#define RXBD_LARGE		0x0020
#define RXBD_NONOCTET		0x0010
#define RXBD_SHORT		0x0008
#define RXBD_CRCERR		0x0004
#define RXBD_OVERRUN		0x0002
#define RXBD_TRUNCATED		0x0001
#define RXBD_STATS		0x003f

#define MIIM_BCM54xx_AUXSTATUS			0x19
#define MIIM_BCM54XX_SHD			0x1c
#define MIIM_BCM54XX_MODE_CTRL			0x7c00
#define BRGPHY_1C_MCR_INTFSEL_RGMII_FIBER	0x0002
#define BRGPHY_1C_MCR_1000BASEX_REG_EN		0x0001
#define MIIM_STATUS				0x1
#define MIIM_STATUS_LINK			0x0004
#define MIIM_BCM54XX_SHD_WRITE			0x8000

/* Broadcom 5466R registers for */
#define MIIM_BCM5466R_PHY_AUX_STATUS			0x19
#define MIIM_BCM5466R_PHY_AUX_STATUS_FULLDUPLEX_1000	0x0007
#define MIIM_BCM5466R_PHY_AUX_STATUS_FULLDUPLEX_100	0x0005
#define MIIM_BCM5466R_PHY_AUX_STATUS_HALFDUPLEX_1000	0x0006
#define MIIM_BCM5466R_PHY_AUX_STATUS_HALFDUPLEX_100	0x0003
#define MIIM_BCM5466R_PHY_AUX_STATUS_FULLDUPLEX_10	0x0002
#define MIIM_BCM5466R_PHY_AUX_STATUS_HALFDUPLEX_10	0x0001
#define MIIM_BCM5466R_PHY_AUX_STATUS_ANGDONE		0x8000
#define MIIM_BCM5466R_PHY_AUX_STATUS_LINKUP		0x0004

/* LED settings */
#define BCM_EXP_REG_MULTICOLOR_SEL	0x0004

#define MULTICOLOR_LED_FORCED_OFF	0x04
#define MULTICOLOR_LED_FORCED_ON	0x05
#define MULTICOLOR_LED_ALTERNATING	0x06

#define SPEED_10M_OFF			0x0000
#define SPEED_100M_BLINK		0x2000
#define SPEED_1000M_ON			0x0040
#define SPEED_LED_OFF			0x2040

#define MIIM_BCM54XX_EXP_DATA		0x15    /* Expansion register data */
#define MIIM_BCM54XX_EXP_SEL		0x17    /* Expansion register select */
#define MIIM_BCM54XX_EXP_SEL_SSD	0x0e00  /* Secondary SerDes select */
#define MIIM_BCM54XX_EXP_SEL_ER		0x0f00  /* Expansion register select */

typedef struct txbd8
{
	ushort	     status;	     /* Status Fields */
	ushort	     length;	     /* Buffer length */
	uint	     bufPtr;	     /* Buffer Pointer */
} txbd8_t;

typedef struct rxbd8
{
	ushort	     status;	     /* Status Fields */
	ushort	     length;	     /* Buffer Length */
	uint	     bufPtr;	     /* Buffer Pointer */
} rxbd8_t;

typedef struct rmon_mib
{
	/* Transmit and Receive Counters */
	uint	tr64;		/* Transmit and Receive 64-byte Frame Counter */
	uint	tr127;		/* Transmit and Receive 65-127 byte Frame Counter */
	uint	tr255;		/* Transmit and Receive 128-255 byte Frame Counter */
	uint	tr511;		/* Transmit and Receive 256-511 byte Frame Counter */
	uint	tr1k;		/* Transmit and Receive 512-1023 byte Frame Counter */
	uint	trmax;		/* Transmit and Receive 1024-1518 byte Frame Counter */
	uint	trmgv;		/* Transmit and Receive 1519-1522 byte Good VLAN Frame */
	/* Receive Counters */
	uint	rbyt;		/* Receive Byte Counter */
	uint	rpkt;		/* Receive Packet Counter */
	uint	rfcs;		/* Receive FCS Error Counter */
	uint	rmca;		/* Receive Multicast Packet (Counter) */
	uint	rbca;		/* Receive Broadcast Packet */
	uint	rxcf;		/* Receive Control Frame Packet */
	uint	rxpf;		/* Receive Pause Frame Packet */
	uint	rxuo;		/* Receive Unknown OP Code */
	uint	raln;		/* Receive Alignment Error */
	uint	rflr;		/* Receive Frame Length Error */
	uint	rcde;		/* Receive Code Error */
	uint	rcse;		/* Receive Carrier Sense Error */
	uint	rund;		/* Receive Undersize Packet */
	uint	rovr;		/* Receive Oversize Packet */
	uint	rfrg;		/* Receive Fragments */
	uint	rjbr;		/* Receive Jabber */
	uint	rdrp;		/* Receive Drop */
	/* Transmit Counters */
	uint	tbyt;		/* Transmit Byte Counter */
	uint	tpkt;		/* Transmit Packet */
	uint	tmca;		/* Transmit Multicast Packet */
	uint	tbca;		/* Transmit Broadcast Packet */
	uint	txpf;		/* Transmit Pause Control Frame */
	uint	tdfr;		/* Transmit Deferral Packet */
	uint	tedf;		/* Transmit Excessive Deferral Packet */
	uint	tscl;		/* Transmit Single Collision Packet */
	/* (0x2_n700) */
	uint	tmcl;		/* Transmit Multiple Collision Packet */
	uint	tlcl;		/* Transmit Late Collision Packet */
	uint	txcl;		/* Transmit Excessive Collision Packet */
	uint	tncl;		/* Transmit Total Collision */

	uint	res2;

	uint	tdrp;		/* Transmit Drop Frame */
	uint	tjbr;		/* Transmit Jabber Frame */
	uint	tfcs;		/* Transmit FCS Error */
	uint	txcf;		/* Transmit Control Frame */
	uint	tovr;		/* Transmit Oversize Frame */
	uint	tund;		/* Transmit Undersize Frame */
	uint	tfrg;		/* Transmit Fragments Frame */
	/* General Registers */
	uint	car1;		/* Carry Register One */
	uint	car2;		/* Carry Register Two */
	uint	cam1;		/* Carry Register One Mask */
	uint	cam2;		/* Carry Register Two Mask */
} rmon_mib_t;

typedef struct tsec_hash_regs
{
	uint	iaddr0;		/* Individual Address Register 0 */
	uint	iaddr1;		/* Individual Address Register 1 */
	uint	iaddr2;		/* Individual Address Register 2 */
	uint	iaddr3;		/* Individual Address Register 3 */
	uint	iaddr4;		/* Individual Address Register 4 */
	uint	iaddr5;		/* Individual Address Register 5 */
	uint	iaddr6;		/* Individual Address Register 6 */
	uint	iaddr7;		/* Individual Address Register 7 */
	uint	res1[24];
	uint	gaddr0;		/* Group Address Register 0 */
	uint	gaddr1;		/* Group Address Register 1 */
	uint	gaddr2;		/* Group Address Register 2 */
	uint	gaddr3;		/* Group Address Register 3 */
	uint	gaddr4;		/* Group Address Register 4 */
	uint	gaddr5;		/* Group Address Register 5 */
	uint	gaddr6;		/* Group Address Register 6 */
	uint	gaddr7;		/* Group Address Register 7 */
	uint	res2[24];
} tsec_hash_t;

typedef struct tsec_mdio {
	uint    res1[4];
	uint    ieventm;
	uint    imaskm;
	uint    res2;
	uint    emapm;
	uint    res3[320];
	uint    miimcfg;        /* MII Management: Configuration */
	uint    miimcom;        /* MII Management: Command */
	uint    miimadd;        /* MII Management: Address */
uint    miimcon;        /* MII Management: Control */
uint    miimstat;       /* MII Management: Status */
uint    miimind;        /* MII Management: Indicators */
uint    res4[690];
} tsec_mdio_t;

typedef struct tsec
{
	/* General Control and Status Registers (0x2_n000) */
	uint	res000[4];

	uint	ievent;		/* Interrupt Event */
	uint	imask;		/* Interrupt Mask */
	uint	edis;		/* Error Disabled */
	uint	res01c;
	uint	ecntrl;		/* Ethernet Control */
	uint	minflr;		/* Minimum Frame Length */
	uint	ptv;		/* Pause Time Value */
	uint	dmactrl;	/* DMA Control */
	uint	tbipa;		/* TBI PHY Address */

	uint	res034[3];
	uint	res040[48];

	/* Transmit Control and Status Registers (0x2_n100) */
	uint	tctrl;		/* Transmit Control */
	uint	tstat;		/* Transmit Status */
	uint	res108;
	uint	tbdlen;		/* Tx BD Data Length */
	uint	res110[5];
	uint	ctbptr;		/* Current TxBD Pointer */
	uint	res128[23];
	uint	tbptr;		/* TxBD Pointer */
	uint	res188[30];
	/* (0x2_n200) */
	uint	res200;
	uint	tbase;		/* TxBD Base Address */
	uint	res208[42];
	uint	ostbd;		/* Out of Sequence TxBD */
	uint	ostbdp;		/* Out of Sequence Tx Data Buffer Pointer */
	uint	res2b8[18];

	/* Receive Control and Status Registers (0x2_n300) */
	uint	rctrl;		/* Receive Control */
	uint	rstat;		/* Receive Status */
	uint	res308;
	uint	rbdlen;		/* RxBD Data Length */
	uint	res310[4];
	uint	res320;
	uint	crbptr;	/* Current Receive Buffer Pointer */
	uint	res328[6];
	uint	mrblr;	/* Maximum Receive Buffer Length */
	uint	res344[16];
	uint	rbptr;	/* RxBD Pointer */
	uint	res388[30];
	/* (0x2_n400) */
	uint	res400;
	uint	rbase;	/* RxBD Base Address */
	uint	res408[62];

	/* MAC Registers (0x2_n500) */
	uint	maccfg1;	/* MAC Configuration #1 */
	uint	maccfg2;	/* MAC Configuration #2 */
	uint	ipgifg;		/* Inter Packet Gap/Inter Frame Gap */
	uint	hafdup;		/* Half-duplex */
	uint	maxfrm;		/* Maximum Frame */
	uint	res514;
	uint	res518;

	uint	res51c;

	uint	resmdio[6];

	uint	res538;

	uint	ifstat;		/* Interface Status */
	uint	macstnaddr1;	/* Station Address, part 1 */
	uint	macstnaddr2;	/* Station Address, part 2 */
	uint	res548[46];

	/* (0x2_n600) */
	uint	res600[32];

	/* RMON MIB Registers (0x2_n680-0x2_n73c) */
	rmon_mib_t	rmon;
	uint	res740[48];

	/* Hash Function Registers (0x2_n800) */
	tsec_hash_t	hash;

	uint	res900[128];

	/* Pattern Registers (0x2_nb00) */
	uint	resb00[62];
	uint	attr;	   /* Default Attribute Register */
	uint	attreli;	   /* Default Attribute Extract Length and Index */

	/* TSEC Future Expansion Space (0x2_nc00-0x2_nffc) */
	uint	resc00[256];
} tsec_t;

#define TSEC_GIGABIT (1 << 0)

/* These flags currently only have meaning if we're using the eTSEC */
#define TSEC_REDUCED	(1 << 1)	/* MAC-PHY interface uses RGMII */
#define TSEC_SGMII	(1 << 2)	/* MAC-PHY interface uses SGMII */

struct tsec_private {
	tsec_t *regs;
	struct tsec_mii_mng *phyregs_sgmii;
	struct phy_device *phydev;
	phy_interface_t interface;
	struct mii_dev *bus;
	uint phyaddr;
	char mii_devname[16];
	u32 flags;
	/* JUNOS Start */
#ifdef CONFIG_EX45XX
	uint index;
#endif
	/* JUNOS End */
};

struct tsec_info_struct {
	tsec_t *regs;
	struct tsec_mii_mng *miiregs_sgmii;
	char *devname;
	char *mii_devname;
	phy_interface_t interface;
	unsigned int phyaddr;
	u32 flags;
	u32 hw_unit;	/* JUNOS */
};

int tsec_standard_init(bd_t *bis);
int tsec_eth_init(bd_t *bis, struct tsec_info_struct *tsec_info, int num);

#endif /* __TSEC_H */
