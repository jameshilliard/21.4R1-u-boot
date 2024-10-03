/*
 * Copyright (c) 2006-2012, Juniper Networks, Inc.
 * All rights reserved.
 *
 * (C) Copyright 2001-2004
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
#include <command.h>
#include <net.h>
#include <miiphy.h>

DECLARE_GLOBAL_DATA_PTR;
#if (CONFIG_COMMANDS & CFG_CMD_NET) && defined(CONFIG_NET_MULTI)

#ifdef CFG_GT_6426x
extern int gt6426x_eth_initialize(bd_t *bis);
#endif

extern int au1x00_enet_initialize(bd_t*);
extern int dc21x4x_initialize(bd_t*);
extern int e1000_initialize(bd_t*);
extern int eepro100_initialize(bd_t*);
extern int eth_3com_initialize(bd_t*);
extern int fec_initialize(bd_t*);
extern int inca_switch_initialize(bd_t*);
extern int mpc5xxx_fec_initialize(bd_t*);
extern int mpc8220_fec_initialize(bd_t*);
extern int mv6436x_eth_initialize(bd_t *);
extern int mv6446x_eth_initialize(bd_t *);
extern int natsemi_initialize(bd_t*);
extern int ns8382x_initialize(bd_t*);
extern int pcnet_initialize(bd_t*);
extern int plb2800_eth_initialize(bd_t*);
extern int ppc_4xx_eth_initialize(bd_t *);
extern int rtl8139_initialize(bd_t*);
extern int rtl8169_initialize(bd_t*);
extern int scc_initialize(bd_t*);
extern int skge_initialize(bd_t*);
extern int tsec_initialize(bd_t*, int, char *);
extern int mv_eth_initialize(bd_t *);
extern int mv_prestera_initialize(bd_t *);
extern int npe_initialize(bd_t *);
extern int octeon_eth_initialize(bd_t *bis);
extern void (*push_packet)(volatile void *, int);
extern int rmigmac_enet_initialize (bd_t *);
extern int rmisky2_initialize (bd_t * bis);

#if defined( CONFIG_EX3242) || \
    defined(CONFIG_EX82XX) || \
    defined(CONFIG_EX45XX) || \
    defined(CONFIG_RPS200) || \
    defined(CONFIG_EX2200) || \
    defined(CONFIG_EX3300)
static int verbose = 0;
#else
static int verbose = 1;
#endif

static struct {
	uchar data[PKTSIZE];
	int length;
} eth_rcv_bufs[PKTBUFSRX];

static struct eth_device *eth_devices, *eth_current;
static unsigned int eth_rcv_current = 0, eth_rcv_last = 0;

struct eth_device *eth_get_dev(void)
{
	return eth_current;
}

struct eth_device *eth_get_dev_by_name(char *devname)
{
	struct eth_device *dev, *target_dev;

	if (!eth_devices)
		return NULL;

	dev = eth_devices;
	target_dev = NULL;
	do {
		if (strcmp(devname, dev->name) == 0) {
			target_dev = dev;
			break;
		}
		dev = dev->next;
	} while (dev != eth_devices);

	return target_dev;
}

int eth_get_dev_index (void)
{
	struct eth_device *dev;
	int num = 0;

	if (!eth_devices) {
		return (-1);
	}

	for (dev = eth_devices; dev; dev = dev->next) {
		if (dev == eth_current)
			break;
		++num;
	}

	if (dev) {
		return (num);
	}

	return (0);
}

int eth_register(struct eth_device* dev)
{
	struct eth_device *d;

	if (!eth_devices) {
		eth_current = eth_devices = dev;
#ifdef CONFIG_NET_MULTI
		/* update current ethernet name */
		{
			char *act = getenv("ethact");
#ifdef CONFIG_JSRXNLE
			switch (gd->board_desc.board_type) {
			CASE_ALL_SRX650_MODELS
			    /* In SRX650 set ethact only in case it not set */
			    if (act == NULL)
				setenv("ethact", eth_current->name);
			    break;
			default:
			    if (act == NULL || strcmp(act, eth_current->name) != 0)
				setenv("ethact", eth_current->name);
			    break;
			}
#else
#ifdef CONFIG_MAG8600
			if (act == NULL)
				setenv("ethact","octeth1");	
#elif defined(CONFIG_FX)
			if (act == NULL)
			    setenv("ethact", eth_current->name);
#else
			if (act == NULL || strcmp(act, eth_current->name) != 0)
				setenv("ethact", eth_current->name);
#endif
#endif
		}
#endif
	} else {
		for (d=eth_devices; d->next!=eth_devices; d=d->next);
		d->next = dev;
	}

	dev->state = ETH_STATE_INIT;
	dev->next  = eth_devices;

	return 0;
}

int eth_initialize(bd_t *bis)
{
	char enetvar[32];
	unsigned char env_enetaddr[6];
	int i, eth_number = 0;
	char *tmp, *end;

	eth_devices = NULL;
	eth_current = NULL;

#ifdef  CONFIG_MARVELL
#if defined(MV_INCLUDE_GIG_ETH) || defined(MV_INCLUDE_UNM_ETH)
	/* move to the begining so in case we have a PCI NIC it will
        read the env mac addresses correctlly. */
        mv_eth_initialize(bis);
#endif
#if defined(MV_PRESTERA_SWITCH)
	mv_prestera_initialize(bis);
#endif
#endif
#if defined(CONFIG_MII) || (CONFIG_COMMANDS & CFG_CMD_MII)
	miiphy_init();
#endif

#ifdef CONFIG_DB64360
	mv6436x_eth_initialize(bis);
#endif
#ifdef CONFIG_CPCI750
	mv6436x_eth_initialize(bis);
#endif
#ifdef CONFIG_DB64460
	mv6446x_eth_initialize(bis);
#endif
#if defined(CONFIG_4xx) && !defined(CONFIG_IOP480) && !defined(CONFIG_AP1000)
	ppc_4xx_eth_initialize(bis);
#endif
#ifdef CONFIG_INCA_IP_SWITCH
	inca_switch_initialize(bis);
#endif
#ifdef CONFIG_PLB2800_ETHER
	plb2800_eth_initialize(bis);
#endif
#ifdef SCC_ENET
	scc_initialize(bis);
#endif
#if defined(CONFIG_MPC5xxx_FEC)
	mpc5xxx_fec_initialize(bis);
#endif
#if defined(CONFIG_MPC8220_FEC)
	mpc8220_fec_initialize(bis);
#endif
#if defined(CONFIG_SK98)
	skge_initialize(bis);
#endif
#if defined(CONFIG_MPC85XX_TSEC1)
	tsec_initialize(bis, 0, CONFIG_MPC85XX_TSEC1_NAME);
#elif defined(CONFIG_MPC83XX_TSEC1)
	tsec_initialize(bis, 0, CONFIG_MPC83XX_TSEC1_NAME);
#endif

#if defined(CONFIG_MPC85XX_TSEC2) 
	tsec_initialize(bis, 1, CONFIG_MPC85XX_TSEC2_NAME);
#elif defined(CONFIG_MPC83XX_TSEC2)
	tsec_initialize(bis, 1, CONFIG_MPC83XX_TSEC2_NAME);
#endif

#if defined(CONFIG_MPC85XX_FEC)
	tsec_initialize(bis, 2, CONFIG_MPC85XX_FEC_NAME);
#else
#if defined(CONFIG_MPC85XX_TSEC3)
	tsec_initialize(bis, 2, CONFIG_MPC85XX_TSEC3_NAME);
#elif defined(CONFIG_MPC83XX_TSEC3)
	tsec_initialize(bis, 2, CONFIG_MPC83XX_TSEC3_NAME);
#endif

#if defined(CONFIG_MPC85XX_TSEC4)
	tsec_initialize(bis, 3, CONFIG_MPC85XX_TSEC4_NAME);
#elif defined(CONFIG_MPC83XX_TSEC4)
	tsec_initialize(bis, 3, CONFIG_MPC83XX_TSEC4_NAME);
#endif
#endif

#if defined(CONFIG_MPC86XX_TSEC1)
       tsec_initialize(bis, 0, CONFIG_MPC86XX_TSEC1_NAME);
#endif

#if defined(CONFIG_MPC86XX_TSEC2)
       tsec_initialize(bis, 1, CONFIG_MPC86XX_TSEC2_NAME);
#endif

#if defined(CONFIG_MPC86XX_TSEC3)
       tsec_initialize(bis, 2, CONFIG_MPC86XX_TSEC3_NAME);
#endif

#if defined(CONFIG_MPC86XX_TSEC4)
       tsec_initialize(bis, 3, CONFIG_MPC86XX_TSEC4_NAME);
#endif

#if defined(FEC_ENET) || defined(CONFIG_ETHER_ON_FCC)
	fec_initialize(bis);
#endif
#if defined(CONFIG_AU1X00)
	au1x00_enet_initialize(bis);
#endif
#if defined(CONFIG_IXP4XX_NPE)
	npe_initialize(bis);
#endif
#ifdef CONFIG_E1000
	e1000_initialize(bis);
#endif
#ifdef CONFIG_EEPRO100
	eepro100_initialize(bis);
#endif
#ifdef CONFIG_TULIP
	dc21x4x_initialize(bis);
#endif
#ifdef CONFIG_3COM
	eth_3com_initialize(bis);
#endif
#ifdef CONFIG_PCNET
	pcnet_initialize(bis);
#endif
#ifdef CFG_GT_6426x
	gt6426x_eth_initialize(bis);
#endif
#ifdef CONFIG_NATSEMI
	natsemi_initialize(bis);
#endif
#ifdef CONFIG_NS8382X
	ns8382x_initialize(bis);
#endif
#if defined(CONFIG_RTL8139)
	rtl8139_initialize(bis);
#endif
#if defined(CONFIG_RTL8169)
	rtl8169_initialize(bis);
#endif
#if defined(OCTEON_RGMII_ENET)
	octeon_eth_initialize(bis);
#endif
#if defined (CONFIG_GMAC_RMIBOARD)
        rmigmac_enet_initialize (bis);
#endif
#if defined (CONFIG_RMI_PCI_SKY2)
        rmisky2_initialize(bis);
#endif

	if (!eth_devices) {
		puts ("No ethernet found.\n");
	} else {
		struct eth_device *dev = eth_devices;
		char *ethprime = getenv ("ethprime");

		do {
			if (verbose) {
				if (eth_number)
					puts (", ");

				printf("%s", dev->name);
			}

			if (ethprime && strcmp (dev->name, ethprime) == 0) {
				eth_current = dev;
				if (verbose) {
					puts (" [PRIME]");
				}
			}

			sprintf(enetvar, eth_number ? "eth%daddr" : "ethaddr", eth_number);
			tmp = getenv (enetvar);

			for (i=0; i<6; i++) {
				env_enetaddr[i] = tmp ? simple_strtoul(tmp, &end, 16) : 0;
				if (tmp)
					tmp = (*end) ? end+1 : end;
			}

			if (memcmp(env_enetaddr, "\0\0\0\0\0\0", 6)) {
				if (memcmp(dev->enetaddr, "\0\0\0\0\0\0", 6) &&
				    memcmp(dev->enetaddr, env_enetaddr, 6))
				{
					printf ("\nWarning: %s MAC addresses don't match:\n",
						dev->name);
					printf ("Address in SROM is         "
					       "%02X:%02X:%02X:%02X:%02X:%02X\n",
					       dev->enetaddr[0], dev->enetaddr[1],
					       dev->enetaddr[2], dev->enetaddr[3],
					       dev->enetaddr[4], dev->enetaddr[5]);
					printf ("Address in environment is  "
					       "%02X:%02X:%02X:%02X:%02X:%02X\n",
					       env_enetaddr[0], env_enetaddr[1],
					       env_enetaddr[2], env_enetaddr[3],
					       env_enetaddr[4], env_enetaddr[5]);
				}

				memcpy(dev->enetaddr, env_enetaddr, 6);
			}

			eth_number++;
			dev = dev->next;
		} while(dev != eth_devices);

#ifdef CONFIG_NET_MULTI
		/* update current ethernet name */
		if (eth_current) {
			char *act = getenv("ethact");
#ifdef CONFIG_JSRXNLE
			switch (gd->board_desc.board_type) {
			CASE_ALL_SRX650_MODELS
			    /* In SRX650 set ethact only in case it not set */
			    if (act == NULL)
				setenv("ethact", eth_current->name);
			    break;
			default:
			    if (act == NULL || strcmp(act, eth_current->name) != 0)
				setenv("ethact", eth_current->name);
			    break;
			}
#else
#ifdef CONFIG_MAG8600
			if (act == NULL)
				setenv("ethact","octeth1");	
#elif defined(CONFIG_FX)
			if (act == NULL)
				setenv("ethact", eth_current->name);
#else
			if (act == NULL || strcmp(act, eth_current->name) != 0)
				setenv("ethact", eth_current->name);
#endif
#endif
		} else
			setenv("ethact", NULL);
#endif

		putc ('\n');
	}

	return eth_number;
}

void eth_set_enetaddr(int num, char *addr) {
	struct eth_device *dev;
	unsigned char enetaddr[6];
	char *end;
	int i;

	debug ("eth_set_enetaddr(num=%d, addr=%s)\n", num, addr);

	if (!eth_devices)
		return;

	for (i=0; i<6; i++) {
		enetaddr[i] = addr ? simple_strtoul(addr, &end, 16) : 0;
		if (addr)
			addr = (*end) ? end+1 : end;
	}

	dev = eth_devices;
	while(num-- > 0) {
		dev = dev->next;

		if (dev == eth_devices)
			return;
	}

	debug ( "Setting new HW address on %s\n"
		"New Address is             %02X:%02X:%02X:%02X:%02X:%02X\n",
		dev->name,
		enetaddr[0], enetaddr[1],
		enetaddr[2], enetaddr[3],
		enetaddr[4], enetaddr[5]);

	memcpy(dev->enetaddr, enetaddr, 6);
}

int eth_init(bd_t *bis)
{
	struct eth_device* old_current;
#ifdef CONFIG_FX
	struct eth_device *good_eth = NULL;
#endif


	if (!eth_current)
		return 0;

	eth_rcv_current = eth_rcv_last = 0;

	old_current = eth_current;
#if !defined(CONFIG_EX45XX) && \
    !defined(CONFIG_EX2200)  && \
    !defined(CONFIG_RPS200) && \
    !defined(CONFIG_EX3300)
	do {
#endif
		debug ("Trying %s\n", eth_current->name);

		if (eth_current->init(eth_current, bis)) {
			eth_current->state = ETH_STATE_ACTIVE;

#ifdef CONFIG_FX
		if (good_eth == NULL) { 
		    good_eth = eth_current; 
		} 
#else
			return 1;

			debug  ("FAIL\n");
#endif
		}

#if !defined(CONFIG_EX45XX) && \
    !defined(CONFIG_EX2200)  && \
    !defined(CONFIG_RPS200) && \
    !defined(CONFIG_EX3300)
		eth_try_another(0);
	} while (old_current != eth_current);
#ifdef CONFIG_FX
	if (good_eth != NULL) {
	    eth_current = good_eth;
	    if (!getenv("ethact")) {
		setenv("ethact", eth_current->name);
	    }
	    return 1;
	} else {
	    return 0;
	}
#endif
#endif

	return 0;
}

void eth_halt(void)
{
	if (!eth_current)
		return;

	eth_current->halt(eth_current);

	eth_current->state = ETH_STATE_PASSIVE;
}

int eth_send(volatile void *packet, int length)
{
	if (!eth_current)
		return -1;

	return eth_current->send(eth_current, packet, length);
}

int eth_rx(void)
{
	if (!eth_current)
		return -1;

	return eth_current->recv(eth_current);
}

static void eth_save_packet(volatile void *packet, int length)
{
	volatile char *p = packet;
	int i;

	if ((eth_rcv_last + 1) % PKTBUFSRX == eth_rcv_current)
		return;

	if (PKTSIZE < length)
		return;

	for (i = 0; i < length; i++)
		eth_rcv_bufs[eth_rcv_last].data[i] = p[i];

	eth_rcv_bufs[eth_rcv_last].length = length;
	eth_rcv_last = (eth_rcv_last + 1) % PKTBUFSRX;
}

int eth_receive(volatile void *packet, int length)
{
	void *pp;
	int idx, res;

	pp = push_packet;
	push_packet = eth_save_packet;

	while (1) {
		if (eth_rcv_current == eth_rcv_last) {
			eth_rx();
			if (eth_rcv_current == eth_rcv_last) {
				res = -1;
				break;
			}
		}
		idx = eth_rcv_current;
		eth_rcv_current = (eth_rcv_current + 1) % PKTBUFSRX;
		res = eth_rcv_bufs[idx].length;
		if (length >= res) {
			memcpy((void*)packet, eth_rcv_bufs[idx].data, res);
			break;
		}
	}

	push_packet = pp;
	return res;
}

void eth_try_another(int first_restart)
{
	static struct eth_device *first_failed = NULL;

	if (!eth_current)
		return;

	if (first_restart) {
		first_failed = eth_current;
	}

#if !defined(CONFIG_EX45XX) && \
    !defined(CONFIG_EX2200) && \
    !defined(CONFIG_RPS200) && \
    !defined(CONFIG_EX3300)
	eth_current = eth_current->next;
#endif

#ifdef CONFIG_NET_MULTI
	/* update current ethernet name */
	{
		char *act = getenv("ethact");
#if defined(CONFIG_FX)
		if (act == NULL)
		    setenv("ethact", eth_current->name);
#else
		if (act == NULL || strcmp(act, eth_current->name) != 0)
			setenv("ethact", eth_current->name);
#endif
	}
#endif

	if (first_failed == eth_current) {
		NetRestartWrap = 1;
	}
}

#ifdef CONFIG_NET_MULTI
void eth_set_current(void)
{
	char *act;
	struct eth_device* old_current;

	if (!eth_current)	/* XXX no current */
		return;

	act = getenv("ethact");
	if (act != NULL) {
		old_current = eth_current;
		do {
			if (strcmp(eth_current->name, act) == 0)
				return;
			eth_current = eth_current->next;
		} while (old_current != eth_current);
	}

	setenv("ethact", eth_current->name);
}
#endif

char *eth_get_name (void)
{
	return (eth_current ? eth_current->name : "unknown");
}
#elif (CONFIG_COMMANDS & CFG_CMD_NET) && !defined(CONFIG_NET_MULTI)

extern int at91rm9200_miiphy_initialize(bd_t *bis);
extern int emac4xx_miiphy_initialize(bd_t *bis);
extern int mcf52x2_miiphy_initialize(bd_t *bis);
extern int ns7520_miiphy_initialize(bd_t *bis);

int eth_initialize(bd_t *bis)
{
#if defined(CONFIG_MII) || (CONFIG_COMMANDS & CFG_CMD_MII)
	miiphy_init();
#endif

#if defined(CONFIG_AT91RM9200)
	at91rm9200_miiphy_initialize(bis);
#endif
#if defined(CONFIG_4xx) && !defined(CONFIG_IOP480) \
	&& !defined(CONFIG_AP1000) && !defined(CONFIG_405)
	emac4xx_miiphy_initialize(bis);
#endif
#if defined(CONFIG_MCF52x2)
	mcf52x2_miiphy_initialize(bis);
#endif
#if defined(CONFIG_NETARM)
	ns7520_miiphy_initialize(bis);
#endif
	return 0;
}
#endif
