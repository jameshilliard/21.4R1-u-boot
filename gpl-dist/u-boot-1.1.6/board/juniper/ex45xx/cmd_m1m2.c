/*
 * Copyright (c) 2010, Juniper Networks, Inc.
 * All rights reserved.
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

#if !defined(CONFIG_PRODUCTION) 
#include <ns16550.h>
#include "m1m2.h"
#include <i2c.h>
#include <tsec.h>
#include <ether.h>
#include "../ex3242/fsl_i2c.h"
#include "../ex3242/cmd_jmem.h"
#include <pcie.h>
#include "epld.h"
#include "cmd_ex45xx.h"
#include "ex45xx_local.h"

DECLARE_GLOBAL_DATA_PTR;

extern int i2c_manual;
extern struct pcie_controller pcie_hose[];

/* M1M2 test circuit */

static char slot_mac_name [4][6] = 
 { "TSEC3",   /* ??? */
    "TSEC1",  /* M1-40 slot */
    "TSEC0",  /* M1-80 slot */
    "TSEC2"   /* M2 slot */
};

static int m_slot = 0; /* = card slot + 1 */
static int m_module = 0;
static uint8_t echo = 0;
static uint8_t m1m2_flag = 0;
static NS16550_t uart2 = (NS16550_t)CFG_NS16550_COM2;
static uint8_t m_mac[] = {0x0, 0xE0, 0xC, 0x0, 0x0, 0xFD};

char  *m1m2_slot_str[] = 
    {
        "m1-40",
        "m1-80",
        "m2",
    };

uint8_t my_mac[6];

static uint8_t *test_string = "0123456789abcdefghijklmnopqrstuvwxy";
int m1m2_reloc = 0;

static int 
calc_divisor (NS16550_t port, int baudrate)
{
    return (CFG_NS16550_CLK / 16 / baudrate);
}

static void 
update_tx (uint8_t *tdata, int len)
{
    int i;
    uint16_t cksum = 0;

    tdata[1] = echo;
    echo++;
    
    /* update length fields */
    tdata[2] = ((len - 4) & 0xff00) >> 8;
    tdata[3] = (len - 4) & 0xff;

    /* checksum */
    if (len > 8) {
         for (i = 8; i < len; i++) {
            cksum += tdata[i];
         }
         tdata[6] = (cksum & 0xff00) >> 8;
         tdata[7] = cksum & 0xff;
    }
    else {
         tdata[6] = 0;
         tdata[7] = 0;
    }
}

static void 
build_tx_hdr (uint8_t *tdata, uint8_t msg_type, uint8_t cmd_byte)
{
    tdata[0] = msg_type;
    tdata[1] = echo;
    tdata[4] = cmd_byte;
}

static void 
build_tx_data_hex (uint8_t *tdata, uint8_t *data, int offset, int len)
{
    char tbyte[3];
    int i;
    uint16_t temp;

    tbyte[2] = 0;
    for (i = 0; i < len; i++) {
        tbyte[0] = data[2*i];
        tbyte[1] = data[2*i+1];
        temp = atoh(tbyte);
        tdata[offset + i] = temp;
    }
}

static void 
build_tx_data_ascii (uint8_t *tdata, uint8_t *data, int offset, int len)
{
    memcpy(&tdata[offset], data, len);
}

static int
m1m2_tx (uint8_t *tdata, int tlen, uint8_t *rdata, int rlen, int wait)
{
    int i, ret = 0;
    
    if (m1m2_flag & M1M2_VERBOSE_ON) {
        printf ("Tx - \n");
        for (i = 0; i < tlen; i++)
            printf("%02x ", tdata[i]);
        printf("\n");
    }
        
    if ((ret = i2c_write_noaddr(0x60, tdata, tlen)) != 0) {
        if (m1m2_flag & M1M2_DEBUG_ON)
            printf ("I2C write to device 0x60 failed.\n");
    }
    else {
        udelay(wait);
        if ((ret = i2c_read_noaddr(0x60, rdata, rlen)) != 0) {
            if (m1m2_flag & M1M2_DEBUG_ON)
                printf ("I2C read from device 0x60 failed.\n");
        }
        else {
            if (m1m2_flag & M1M2_VERBOSE_ON) {
                printf ("Rx - \n");
                for (i = 0; i < rlen; i++)
                    printf("%02x ", rdata[i]);
                printf("\n");
            }

            /* check data match */
            for (i = I2C_DATA_START; i < tlen; i++) {
                if (tdata[i] != rdata[i]) {
                    ret = -1;
                    break;
                }
            }
        }
    }
    return (ret);
}

static int
m1m2_tx_ascii (uint8_t *tdata, int tlen, uint8_t *rdata, int rlen, int wait)
{
    int i, ret = 0;
    
    if (m1m2_flag & M1M2_VERBOSE_ON) {
        printf ("Tx - \n");
        for (i = I2C_DATA_START; i < tlen; i++)
            printf("%c", tdata[i]);
        printf("\n");
    }
        
    if ((ret = i2c_write_noaddr(0x60, tdata, tlen)) != 0) {
        if (m1m2_flag & M1M2_DEBUG_ON)
            printf ("I2C write to device 0x60 failed.\n");
    }
    else {
        udelay(wait);
        if ((ret = i2c_read_noaddr(0x60, rdata, rlen)) != 0) {
            if (m1m2_flag & M1M2_DEBUG_ON)
                printf ("I2C read from device 0x60 failed.\n");
        }
        else {
            if (m1m2_flag & M1M2_VERBOSE_ON) {
                printf ("Rx - \n");
                for (i = I2C_DATA_START; i < rlen; i++)
                    printf("%c", rdata[i]);
                printf("\n");
            }

            /* check data match */
            if (tlen != rlen) {
                ret = -1;
            }
            else {
                for (i = I2C_DATA_START; i < rlen; i++) {
                    if (tdata[i] != rdata[i]) {
                        ret = -1;
                        break;
                    }
                }
            }
        }
    }
    return (ret);
}

/*
 * Test routines
 */
#if 1
static void 
packet_fill (char *packet, int length)
{
    Eth_t *et = (Eth_t *)packet;
    char c = (char) length;
    int i;

    memcpy (et->et_dest, m_mac, 6);
    memcpy (et->et_src, my_mac, 6);
    et->et_protlen = 0x0800;

    for (i = ETH_HDR_SIZE; i < length; i++) {
        packet[i] = c++;
    }
}
#else
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
#endif

static int 
packets_check (char *pkt1, char *pkt2, int length)
{
    int i, j;
    int res = 0;

    if (length <= ETHER_HDR_SIZE)
        return (-1);
    
    for (i = ETHER_HDR_SIZE; i < length; i++) {
        if (pkt1[i] != pkt2[i]) {
            res = -1;
            break;
        }
    }
    if (m1m2_flag & M1M2_VERBOSE_ON) {
        printf("%s tx:\n",  slot_mac_name[m_slot]);
        for (j = 0; j < length; j++) {
            if ((j != 0) && ((j % 16) == 0))
                printf("\n");
            printf("%02x ", pkt1[j]);
        }
        printf("\n");
        printf("%s rx:\n", slot_mac_name[m_slot]);
        for (j = 0; j < length; j++) {
            if ((j != 0) && ((j % 16) == 0))
                printf("\n");
            printf("%02x ", pkt2[j]);
        }
        printf("\n");
    }
    return (res);
}

static void 
phy_1145_force_1000 (struct eth_device *dev, int enable, int *link)
{
    struct tsec_private *priv = (struct tsec_private *)dev->priv;
    volatile tsec_t *regs;
    volatile uint16_t mii_reg;
    int i = 0;

    if (priv->index == 3) {
        return;  /* not for mgmt port */
    }
    regs = priv->phyregs;
    
    mdio_mode_set(0, 1);  /* PHY 1145 */
    write_phy_reg(priv, 22, 0);
    if (enable) {
        /* disable auto-negotiation */
        write_phy_reg(priv, 0, 0x8140);
        udelay(30000);
        
        *link = 1;
        mii_reg = read_phy_reg(priv, MIIM_88E1011_PHY_STATUS);
        if (!((mii_reg & MIIM_88E1011_PHYSTAT_SPDDONE) &&
	        (mii_reg & MIIM_88E1011_PHYSTAT_LINK))) {
            udelay(1000);
        
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

                udelay(1000);	/* 1 ms */
                write_phy_reg(priv, 22, 0);
                mii_reg = read_phy_reg(priv, MIIM_88E1011_PHY_STATUS);
            }
        }
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

static int
set_baudrate_m1m2 (int baudrate, int tdelay)
{
    int ret;
    int clock_divisor;
    uint32_t *ptmp32;
    uint8_t tdata[20], rdata[20];
    
    build_tx_hdr(tdata, MSG_COMMAND, CMD_BAUD);
    tdata[8] = (baudrate & 0xff000000) >> 24;
    tdata[9] = (baudrate & 0xff0000) >> 16;
    tdata[10] = (baudrate & 0xff00) >> 8;
    tdata[11] = (baudrate & 0xff);
                    
    /* update length fields and checksum */
    update_tx(tdata, 12);
                    
    if ((ret = m1m2_tx(tdata, 12, rdata, 12, tdelay)) == 0) {
        ptmp32 = (uint32_t *)&(rdata[8]);
        if (baudrate == *ptmp32) {
            clock_divisor = calc_divisor(uart2, baudrate);
            NS16550_reinit(uart2, clock_divisor);
        }
    }

    return (ret);
}

static int
tx_uart_m1m2 (char *data, int len, int tdelay)
{
    int ret;
    int i, j;
    uint8_t tdata[20], rdata[20];
    char data_1[20];

    build_tx_hdr(tdata, MSG_REQUEST, REQ_UART);
    tdata[8] = (len & 0xff00) >> 8;
    tdata[9] = len & 0xff;
    tdata[10] = (len & 0xff00) >> 8;
    tdata[11] = len & 0xff;
                    
    /* update length fields and checksum */
    update_tx(tdata, 12);
                    
    if ((ret = m1m2_tx(tdata, 12, rdata, 12, tdelay)) == 0) {
        if (m1m2_flag & M1M2_VERBOSE_ON) {
            printf("Tx - \n");
            for (i = 0; i < len; i++) {
                NS16550_putc(uart2, data[i]);
                printf("%c", data[i]);
            }
            printf("\n");
            udelay(tdelay);
            printf("Rx - \n");
            j = 0;
            for (i = 0; i < 2 * len; i++) {
                if (NS16550_tstc(uart2)) {
                    data_1[j] = NS16550_getc(uart2);
                    j++;
                }
                udelay(1000);
            }
            for (i = 0; i < j; i++) {
                printf("%c", data_1[i]);
            }
            printf("\n");
        }
        else {
            for (i = 0; i < len; i++)
                NS16550_putc(uart2, data[i]);
            udelay(tdelay);
            j = 0;
            for (i = 0; i < 2 * len; i++) {
                if (NS16550_tstc(uart2)) {
                    data_1[j] = NS16550_getc(uart2);
                    j++;
                }
                udelay(1000);
            }
        }

        for (i = 0; i < len; i++) {
            if (data[i] != data_1[i]) {
                ret = -1;
                break;
            }
        }
    }

    return (ret);
}

static int
tx_eth_m1m2 (int slot, int len, int tdelay)
{
    int link, len1;
    int ret;
    struct eth_device *dev;
    uint8_t tdata[20], rdata[20];
    char packet_send[MAX_PACKET_LENGTH];
    char packet_recv[MAX_PACKET_LENGTH];

    build_tx_hdr(tdata, MSG_REQUEST, REQ_ETH);
    tdata[8] = (len & 0xff00) >> 8;
    tdata[9] = len & 0xff;
    tdata[10] = tdata[8];
    tdata[11] = tdata[9];
                        
    /* update length fields and checksum */
    update_tx(tdata, 12);
                    
    if ((ret = m1m2_tx(tdata, 12, rdata, 12, tdelay)) == 0) {
        dev = eth_get_dev_by_name(slot_mac_name[slot+1]);
        init_tsec(dev);
        start_tsec(dev);
        phy_1145_force_1000(dev, 1, &link);
        if (link) {
            packet_fill(packet_send, len);
            ret = ether_etsec_send(dev, packet_send, len);
            if (m1m2_flag & M1M2_DEBUG_ON)
                printf("Ethernet tx to slot %d, length = %d, ret = %x\n",
                       slot, len, ret);
            udelay(tdelay);
            len1 = ether_etsec_recv(dev, packet_recv, len);
            if (m1m2_flag & M1M2_DEBUG_ON)
                printf("Ethernet rx from slot %d, length = %d\n", slot, len1);
            ret = packets_check(packet_send, packet_recv, len1);
        }
        phy_1145_force_1000(dev, 0, &link);
        stop_tsec(dev);
    }

    return (ret);
}

static int
get_uptime_m1m2 (int tdelay, uint32_t *uptime)
{
    uint8_t tdata[20], rdata[20];
    int ret;

    build_tx_hdr(tdata, MSG_REQUEST, REQ_UPTIME);
    update_tx(tdata, 8);
    if ((ret = m1m2_tx(tdata, 8, rdata, 12, tdelay)) == 0) {
        *uptime = *((uint32_t *)&(rdata[8]));
    }

    return (ret);
}

static int
get_gpio_m1m2 (int tdelay, uint16_t *gpio)
{
    uint8_t tdata[20], rdata[20];
    int ret;

    build_tx_hdr(tdata, MSG_REQUEST, REQ_GET_GPIO);
    update_tx(tdata, 8);
    if ((ret = m1m2_tx(tdata, 8, rdata, 12, tdelay)) == 0) {
        *gpio = *((uint16_t *)&(rdata[10]));
    }

    return (ret);
}

static int
set_gpio_m1m2 (int tdelay, uint16_t gpio, uint16_t mask)
{
    uint8_t tdata[20], rdata[20];
    uint16_t *temp;
    int ret;

    build_tx_hdr(tdata, MSG_REQUEST, REQ_SET_GPIO);
    temp = (uint16_t *)&tdata[8];
    *temp = gpio;
    temp = (uint16_t *)&tdata[10];
    *temp = mask;
    update_tx(tdata, 12);
    ret = m1m2_tx(tdata, 12, rdata, 12, tdelay);

    return (ret);
}

static uint 
read_phyid_reg(struct tsec_private *priv, uint phyid, uint regnum)
{
	uint value;
	volatile tsec_t *regbase = priv->phyregs;

	/* Put the address of the phy, and the register
	 * number into MIIMADD */
	regbase->miimadd = (phyid << 8) | regnum;

	/* Clear the command register, and wait */
	regbase->miimcom = 0;
	asm("sync");

	/* Initiate a read command, and wait */
	regbase->miimcom = MIIM_READ_COMMAND;
	asm("sync");

	/* Wait for the the indication that the read is done */
	while ((regbase->miimind & (MIIMIND_NOTVALID | MIIMIND_BUSY))) ;

	/* Grab the value read from the PHY */
	value = regbase->miimstat;

	return value;
}

static void 
phy_read_m1m2 (int slot, int reg, uint32_t *regVal)
{
    struct eth_device *dev;
    struct tsec_private *priv;
    volatile tsec_t *regs;
    int i;
    uint32_t temp;

    dev = eth_get_dev_by_name(slot_mac_name[slot+1]);
    priv = (struct tsec_private *)dev->priv;
    if (priv->index == 3) {
        return;  /* not for mgmt port */
    }
    regs = priv->phyregs;

    switch (slot) {
        case 0:
            mdio_mode_set(2, 1);  /* M2 */
            break;
            
            mdio_mode_set(2, 1);  /* M1-40 */
            break;
            
            mdio_mode_set(3, 1);  /* M1-80 */
            break;
    }

    if (m1m2_flag & M1M2_DEBUG_ON) {
        for (i = 0; i < 0xff; i++) {
            temp = read_phyid_reg(priv, i, reg);
            printf("phyid = %02x, reg %02x = %04x\n", i, reg, temp);
        }
    }
    
    *regVal = read_phyid_reg(priv, 0, reg);
    
    mdio_mode_set(0, 0);
}

static void 
m1m2_reset(int slot, int reset)
{
    volatile immap_t *immap = (immap_t *) CFG_CCSRBAR;
    volatile ccsr_gur_t *gur = &immap->im_gur;
    uint32_t temp;

    temp = 1 << (7-slot);
    
    if (reset) { /* active low */
        gur->gpoutdr &= ~temp;
    }
    else { 
        gur->gpoutdr |= temp;
        udelay(3000000);
        pcie_init();
    }
}

/*
 *  slot 0 -- M1-40
 *  slot 1 -- M1-80
 *  slot 2 -- M2
 */
static int
m1m2_card (int slot, int enable)
{
    static unsigned char ch, ch1, mux1;
    int ret;
 
    if (slot >= MAX_CARD_SLOTS)
       return (-1);

    switch (slot) {
        case 0: 
            ch = CFG_I2C_C2_9546SW2_M1_40_CHAN;
            mux1 = CFG_I2C_M1_SFPP_9548SW1;
            ch1 = CFG_I2C_C2_9546SW2_M1_CPU_CHAN;
            memcpy (my_mac, gd->bd->bi_enet1addr, 6);
            break;
                
        case 1: 
            ch = CFG_I2C_C2_9546SW2_M1_80_CHAN;
            mux1 = CFG_I2C_M1_SFPP_9548SW1;
            ch1 = CFG_I2C_C2_9546SW2_M1_CPU_CHAN;
            memcpy (my_mac, gd->bd->bi_enet3addr, 6);
            break;
                
        case 2: 
            ch = CFG_I2C_C2_9546SW2_M2_CHAN;
            mux1 = CFG_I2C_M2_LBK_9548SW1;
            ch1 = CFG_I2C_C2_9546SW2_M2_CPU_CHAN;
            memcpy (my_mac, gd->bd->bi_enet2addr, 6);
            break;
    }

    if (enable) {
        m_slot = slot + 1;
        m_module = slot;
        i2c_controller(CFG_I2C_CTRL_2);
        if ((ret = i2c_mux(1, CFG_I2C_C2_9546SW2, ch, 1)) != 0) {
            if (m1m2_flag & M1M2_DEBUG_ON)
                printf ("I2C mux 0x%02x enable channel %d failed.\n", 
                        CFG_I2C_C2_9546SW2, ch);
            ch1 = ch = 0;
            i2c_mux(1, CFG_I2C_C2_9546SW2, ch, 0);
            i2c_controller(CFG_I2C_CTRL_1);
            m_slot = 0;
            m_module = 0;
            return (-1);
        }
        if (ret == 0) {
            if ((ret = i2c_mux(1, mux1, ch1, 1)) != 0) {
                if (m1m2_flag & M1M2_DEBUG_ON)
                    printf ("I2C mux 0x%02x enable channel %d failed.\n", 
                            mux1, ch1);
                ch1 = ch = 0;
                i2c_mux(1, mux1, ch, 0);
                i2c_controller(CFG_I2C_CTRL_1);
                m_slot = 0;
                m_module = 0;
                return (-1);
            }
        }
    }
    else {
        m_slot = 0;
        m_module = 0;
        ch1 = ch = 0;
        i2c_mux(1, mux1, ch, 0);
        i2c_mux(1, CFG_I2C_C2_9546SW2, ch, 0);
        i2c_controller(CFG_I2C_CTRL_1);
    }
    return (0);
}

/* m1m2 message
 *
 * Syntax:
 */
int 
do_m1m2 (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char cmd1, cmd1_1, cmd1_2, cmd2;
    int i, len, len1, ret, reg, tmp, tdelay = 100000;
    char tbyte[3];
    uint8_t tdata[256], rdata[256];
    char *data;
    uint16_t temp, mask;
    uint32_t baudrate = 0;
    uint32_t uptime;
    uint32_t regVal;
    uint8_t tsync[8] = { 1, 0, 0, 4, 0, 0, 0, 0};

    if (argc < 2)
        goto usage;

    cmd1 = argv[1][0];
    cmd1_1 = argv[1][1];
    cmd1_2 = argv[1][2];

    switch (cmd1) {
        case 'e':       /* enable */
        case 'E':
            if (argc <= 2)
                goto usage;
            
            i2c_manual = 1;
            if (!strcmp(argv[2], "m1-40")) { 
                return (m1m2_card (0, 1));
            }
            else if (!strcmp(argv[2], "m1-80")) {
                return (m1m2_card (1, 1));
            }
            else if (!strcmp(argv[2], "m2")) {
                return (m1m2_card (2, 1));
            }
            else {
                i2c_manual = 0;
                goto usage;
            }

            break;
            
        case 'd':       /* disable/dw */
        case 'D':
            
            switch (cmd1_1) {
                case 'i':    /* disable */
                case 'I':
                    return (m1m2_card (m_slot-1, 0));
                    i2c_manual = 0;
                    break;
                    
                case 'w':    /* dw */
                case 'W':
                    if (argc < 4)
                        goto usage;
            
                    len1 = simple_strtoul(argv[3], NULL, 10);
                    if (argc >= 5)
                        tdelay = simple_strtoul(argv[4], NULL, 10);
            
                    data = argv [2];
                    len = strlen(data)/2;
                    tbyte[2] = 0;
                    for (i = 0; i < len; i++) {
                        tbyte[0] = data[2*i];
                        tbyte[1] = data[2*i+1];
                        temp = atoh(tbyte);
                        tdata[i] = temp;
                    }
            
                    m1m2_flag = M1M2_VERBOSE_ON;

                    /* transmit sync */
                    for (i = 0; i < 3; i++) {
                        /* update length fields and checksum */
                        update_tx(tsync, len);

                        if ((ret = m1m2_tx(tsync, 8, rdata, 8, tdelay)) == 0)
                            break;
                
                        udelay(tdelay);
                    }

                    if (ret == 0) {
                        udelay(tdelay);
        
                        /* update length fields and checksum */
                        update_tx(tdata, len);
                
                        ret = m1m2_tx(tdata, len, rdata, len1, tdelay);
                    }
                    m1m2_flag = 0;
                    break;                    
            }
            break;
            
        case 'c':       /* command */
        case 'C':
            cmd2 = argv[2][0];
            
            switch (cmd2) {
                case 's':       /* sync */
                case 'S':
                    if (argc >= 4)
                        tdelay = simple_strtoul(argv[3], NULL, 10);

                    m1m2_flag = M1M2_VERBOSE_ON;
                    
                    for (i = 0; i < 3; i++) {
                        /* update length fields and checksum */
                        update_tx(tsync, 8);

                        if ((ret = m1m2_tx(tsync, 8, rdata, 8, tdelay)) == 0)
                            break;
                
                        udelay(tdelay);
                    }
                    m1m2_flag = 0;
                    break;
                    
                case 'b':       /* uart baudrate */
                case 'B':
                    baudrate = simple_strtoul(argv[3], NULL, 10);
                    
                    if (argc >= 5)
                        tdelay = simple_strtoul(argv[4], NULL, 10);

                    m1m2_flag = M1M2_VERBOSE_ON;
                    set_baudrate_m1m2(baudrate, tdelay);
                    m1m2_flag = 0;

                    break;
             }
            break;
            
        case 'r':       /* request/read */
        case 'R':
            if (((cmd1_1 == 'e') || (cmd1_1 == 'E')) &&
                ((cmd1_2 == 'q') || (cmd1_2 == 'Q'))) { /* request */
                cmd2 = argv[2][0];
            
                switch (cmd2) {
                    case 'e':       /* echo/eth */
                    case 'E':
                        if ((argv[2][1] == 'c') || (argv[2][1] == 'C')) {
                            if (argc >= 5)
                                tdelay = simple_strtoul(argv[4], NULL, 10);
                    
                            data = argv[3];
                            len = strlen(data)/2;
                    
                            build_tx_hdr(tdata, MSG_REQUEST, REQ_ECHO);
                            build_tx_data_hex(tdata, data, 8, len);
                            update_tx(tdata, len + 8);
                            m1m2_flag = M1M2_VERBOSE_ON;
                            m1m2_tx(tdata, len + 8, rdata, len + 8, tdelay);
                            m1m2_flag = 0;
                            break;
                        }
                        else if ((argv[2][1] == 't') || (argv[2][1] == 'T')) {
                            if (m_slot == 0)
                                return (0);
                        
                            if (argc >= 5)
                                tdelay = simple_strtoul(argv[4], NULL, 10);

                            len = simple_strtoul(argv[3], NULL, 10);
                            
                            m1m2_flag = M1M2_VERBOSE_ON + M1M2_DEBUG_ON;
                            tx_eth_m1m2(m_slot-1, len, tdelay);
                            m1m2_flag = 0;
                        }
                        break;
                    
                    case 'g':       /* gpio */
                    case 'G':
                        if (argc < 4)
                            goto usage;

                        if ((argv[3][0] == 'g') || (argv[3][0] == 'G')) {
                            if (argc >= 5)
                                tdelay = simple_strtoul(argv[4], NULL, 10);
                            m1m2_flag = M1M2_VERBOSE_ON + M1M2_DEBUG_ON;
                            if (get_gpio_m1m2(tdelay, &temp) == 0)
                                printf("%s GPIO = %04x\n", 
                                       m1m2_slot_str[m_slot-1], temp);
                            m1m2_flag = 0;
                        }
                        else if ((argv[3][0] == 's') || (argv[3][0] == 'S')) {
                            if (argc >= 7)
                                tdelay = simple_strtoul(argv[6], NULL, 10);
                            temp = simple_strtoul(argv[4], NULL, 16);
                            mask = simple_strtoul(argv[5], NULL, 16);
                            m1m2_flag = M1M2_VERBOSE_ON + M1M2_DEBUG_ON;
                            set_gpio_m1m2(tdelay, temp, mask);
                            m1m2_flag = 0;
                        }
                        else
                            goto usage;

                        break;
                    
                    case 'r':       /* run */
                    case 'R':
                        if (argc < 4)
                            goto usage;

                        if (argc >= 5)
                            tdelay = simple_strtoul(argv[4], NULL, 10);
                        
                        build_tx_hdr(tdata, MSG_REQUEST, REQ_RUN);
                        len = strlen(argv[3]);

                        if (len) {
                            strcpy(&tdata[8], argv[3]);
                            if ((ret = m1m2_tx(tdata, len + 8, rdata, len + 8,
                                tdelay)) == 0) {
                            }
                        }
                        break;
                    
                    case 'u':       /* uart string/uptime */
                    case 'U':
                        if ((argv[2][1] == 'p') || (argv[2][1] == 'P')) { 
                            if (argc >= 4)
                                tdelay = simple_strtoul(argv[3], NULL, 10);

                            if ((ret = get_uptime_m1m2(tdelay, &uptime)) == 0){
                                printf("Uptime: %d hr %2d min %2d sec\n",
                                       uptime / 3600,
                                       (uptime % 3600) / 60,
                                       uptime % 60);
                            }
                            return (1);
                        }

                        /* uart string */
                        if (argc >= 5)
                            tdelay = simple_strtoul(argv[4], NULL, 10);
                    
                        data = argv[3];
                        len = strlen(data);

                        m1m2_flag = M1M2_VERBOSE_ON;
                        tx_uart_m1m2 (data, len, tdelay);
                        m1m2_flag = 0;

                        break;
                    
                    case 'v':       /* version */
                    case 'V':
                        
                        if (argc >= 4)
                            tdelay = simple_strtoul(argv[3], NULL, 10);
                        
                        build_tx_hdr(tdata, MSG_REQUEST, REQ_VERSION);
                        update_tx(tdata, 8);
                        m1m2_flag = M1M2_VERBOSE_ON + M1M2_DEBUG_ON;
                        if ((ret = m1m2_tx(tdata, 8, rdata, 48, tdelay)) 
                            == 0) {
                            data = (char *)&(rdata[8]);
                            printf("Version: %s\n", data);
                        }
                        m1m2_flag = 0;
                        break;
                }
            }
            else if (((cmd1_1 == 'e') || (cmd1_1 == 'E')) &&
                ((cmd1_2 == 'a') || (cmd1_2 == 'A'))) { /* read */
                
                cmd2 = argv[2][0];
                switch (cmd2) {
                    case 'p':       /* phy */
                    case 'P':
                        reg = simple_strtoul(argv[3], NULL, 16);
                        phy_read_m1m2(m_slot-1, reg, &regVal);
                        printf("Phy reg %02x = %04x\n", reg, regVal);
                        break;
                        
                    case 'h':       /* hotplug-switch */
                    case 'H':
                        if (argc < 4)
                            goto usage;
                        
                        epld_reg_read(EPLD_INT_STATUS1, &temp);
                    
                        if (!strcmp(argv[3], "m1-40")) { 
                            printf("M1-40 hotplug-switch %s.\n",
                                   (temp & 0x0800) ? "off" : "on");
                        }
                        else if (!strcmp(argv[3], "m1-80")) {
                            printf("M1-80 hotplug-switch %s.\n",
                                   (temp & 0x0400) ? "off" : "on");
                        }
                        else if (!strcmp(argv[3], "m2")) {
                            printf("M2    hotplug-switch %s.\n",
                                   (temp & 0x1000) ? "off" : "on");
                        }
                        else if (!strcmp(argv[3], "all")) {
                            printf("M1-40 hotplug-switch %s.\n",
                                   (temp & 0x0800) ? "off" : "on");
                            printf("M1-80 hotplug-switch %s.\n",
                                   (temp & 0x0400) ? "off" : "on");
                            printf("M2    hotplug-switch %s.\n",
                                   (temp & 0x1000) ? "off" : "on");
                        }
                        else {
                            goto usage;
                        }
                        break;
                        
                    default:
                        goto usage;
                        break;
                }
            }
            
            break;
        case 's':       /* set */
        case 'S':
            cmd2 = argv[2][0];

            switch(cmd2) {
                case 'h':       /* hotplug-led */
                case 'H':
                    if (argc < 5)
                        goto usage;
                    
                    epld_reg_read(EPLD_MISC_CONTROL_0, &temp);
                    
                    if (!strcmp(argv[4], "on")) 
                        tmp = 1;
                    else 
                        tmp = 0;
                    
                    if (!strcmp(argv[3], "m1-40")) { 
                        if (tmp)
                            temp |= 0x10;
                        else
                            temp &= ~0x10;
                    }
                    else if (!strcmp(argv[3], "m1-80")) {
                        if (tmp)
                            temp |= 0x20;
                        else
                            temp &= ~0x20;
                    }
                    else if (!strcmp(argv[3], "m2")) {
                        if (tmp)
                            temp |= 0x8;
                        else
                            temp &= ~0x8;
                    }
                    else if (!strcmp(argv[3], "all")) {
                        if (tmp)
                            temp |= 0x38;
                        else
                            temp &= ~0x38;
                    }
                    else {
                        goto usage;
                    }
                    
                    epld_reg_write(EPLD_MISC_CONTROL_0, temp);
                    break;
                    
                default:
                    goto usage;
                    break;
            }
            break;
            
        default:
            goto usage;
            break;
    }

    return (1);
    
 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return (1);
}

static int gpio_reset_m1m2 (int slot, int testn);
int i2c_reset_m1m2 (int slot, int testn);
int i2c_temp_m1m2 (int slot, int testn);
int i2c_slave_m1m2 (int slot, int testn);
int i2c_uart_9600_m1m2 (int slot, int testn);
int i2c_uart_19200_m1m2 (int slot, int testn);
int i2c_uart_38400_m1m2 (int slot, int testn);
int i2c_uart_115200_m1m2 (int slot, int testn);
int i2c_eth_m1m2 (int slot, int testn);
int cpu_mdio (int slot, int testn);
int pcie_m1m2 (int slot, int testn);
int epld_led_timing (int slot, int testn);
int epld_switch (int slot, int testn);
int epld_int (int slot, int testn);
int epld_spare_int (int slot, int testn);

int m1m2_pcie_pdf[] = {0x90000, 0xc0000, 0x40000};

uint32_t m1m2_pcie_bar[] = {0, 0, 0};
uint32_t m1m2_id[] = {0, 0, 0};

char  *m1m2_card_str[] = 
    {
        "M1-SFP+",
        "M1-SFP+",
        "M2-LBK",
    };

char  *m1m2_state_str[] = 
    {
        "Present, test version",  /* state 0 */
        "Present, test version, unknown ID",  /* state 1 */
        "Present, test version, wrong U-boot",  /* state 2 */
        "Present, test version, unknown ID, wrong U-boot",  /* state 3 */
        "Present, non-test version",  /* state 4 */
        "Not present",  /* state 5 */
    };

int m1m2_slot_state[MAX_CARD_SLOTS] = {5, 5, 5};

struct m1m2_test m1_40_list[] =
{
    {
        "I2C Reset",
        "i2c-reset",
        "This test verifies the M_I2C_RST_L to m1-40",
        "setup => -l, default 1",
        &i2c_reset_m1m2,
    },
    {
        "I2C Temperature",
        "i2c-temp",
        "This test verifies the temperature sensor read from m1-40",
        "setup => -l, default 1",
        &i2c_temp_m1m2,
    },
    {
        "I2C Slave",
        "i2c-slave",
        "This test verifies the I2C communication to m1-40",
        "setup => -l, default 1; -s <0..36>, default 10",
        &i2c_slave_m1m2,
    },
    {
        "Ethernet",
        "eth",
        "This test verifies the Ethernet communication to m1-40",
        "setup => -l, default 1; -s <64..1518>, default 64",
        &i2c_eth_m1m2,
    },
    {
        "PCIe Memory",
        "pcie",
        "This test verifies the PCIe memory to m1-40",
        "setup => -l, default 1; -a, <0x300000 - 0x7FFFFF>; -s ",
        &pcie_m1m2,
    },
    {
        "Hotplug Switch",
        "hotplug-switch",
        "This test verifies the hotplug switch state of m1-40",
        "setup => -l, default 1",
        &epld_switch,
    },
    {
        "Interrupt",
        "int",
        "This test verifies the EPLD interrupt from m1-40",
        "setup => -l, default 1",
        &epld_int,
    },
    {
        "Spare Interrupt",
        "spare-int",
        "This test verifies the EPLD spare interrupt from m1-40",
        "setup => -l, default 1",
        &epld_spare_int,
    },
};

struct m1m2_test m1_80_list[] =
{
    {
        "I2C reset",
        "i2c-reset",
        "This test verifies the M_I2C_RST_L to m1-80",
        "setup => -l, default 1",
        &i2c_reset_m1m2,
    },
    {
        "I2C Temperature",
        "i2c-temp",
        "This test verifies the temperature sensor read from m1-80",
        "setup => -l, default 1",
        &i2c_temp_m1m2,
    },
    {
        "I2C Slave",
        "i2c-slave",
        "This test verifies the I2C communication to m1-80",
        "setup => -l, default 1; -s <0..36>, default 10",
        &i2c_slave_m1m2,
    },
    {
        "Ethernet",
        "eth",
        "This test verifies the Ethernet communication to m1-80",
        "setup => -l, default 1; -s <64..1518>, default 64",
        &i2c_eth_m1m2,
    },
    {
        "PCIe Memory",
        "pcie",
        "This test verifies the PCIe memory to m1-80",
        "setup => -l, default 1; -a, <0x300000 - 0x7FFFFF>; -s ",
        &pcie_m1m2,
    },
    {
        "LED Timing",
        "led-timing",
        "This test verifies the EPLD LED timing to m1-80",
        "setup => -l, default 1",
        &epld_led_timing,
    },
    {
        "Interrupt",
        "int",
        "This test verifies the EPLD interrupt from m1-80",
        "setup => -l, default 1",
        &epld_int,
    },
    {
        "Spare Interrupt",
        "spare-int",
        "This test verifies the EPLD spare interrupt from m1-80",
        "setup => -l, default 1",
        &epld_spare_int,
    },
};

struct m1m2_test m2_list[] =
{
    {
        "I2C reset",
        "i2c-reset",
        "This test verifies the M2_I2C_RST_L to m2",
        "setup => -l, default 1",
        &i2c_reset_m1m2,
    },
    {
        "I2C Temperature",
        "i2c-temp",
        "This test verifies the temperature sensor read from m2",
        "setup => -l, default 1",
        &i2c_temp_m1m2,
    },
    {
        "I2C Slave",
        "i2c-slave",
        "This test verifies the I2C communication to m2",
        "setup => -l, default 1; -s <0..36>, default 10",
        &i2c_slave_m1m2,
    },
    {
        "UART 9600",
        "uart-9600",
        "This test verifies the UART communication to m2",
        "setup => -l, default 1; -s <0..16>, default 10",
        &i2c_uart_9600_m1m2,
    },
    {
        "UART 19200",
        "uart-19200",
        "This test verifies the UART communication to m2",
        "setup => -l, default 1; -s <0..16>, default 10",
        &i2c_uart_19200_m1m2,
    },
    {
        "UART 38400",
        "uart-38400",
        "This test verifies the UART communication to m2",
        "setup => -l, default 1; -s <0..16>, default 10",
        &i2c_uart_38400_m1m2,
    },
    {
        "UART 115200",
        "uart-115200",
        "This test verifies the UART communication to m2",
        "setup => -l, default 1; -s <0..16>, default 10",
        &i2c_uart_115200_m1m2,
    },
    {
        "Ethernet",
        "eth",
        "This test verifies the Ethernet communication to m2",
        "setup => -l, default 1; -s <64..1518>, default 64",
        &i2c_eth_m1m2,
    },
    {
        "PCIe Memory",
        "pcie",
        "This test verifies the PCIe memory to m2",
        "setup => -l, default 1; -a, <0x300000 - 0x7FFFFF>; -s ",
        &pcie_m1m2,
    },
    {
        "LED Timing",
        "led-timing",
        "This test verifies the EPLD LED timing to m2",
        "setup => -l, default 1",
        &epld_led_timing,
    },
    {
        "Hotplug Switch",
        "hotplug-switch",
        "This test verifies the hotplug switch state of m2",
        "setup => -l, default 1",
        &epld_switch,
    },
    {
        "Interrupt",
        "int",
        "This test verifies the EPLD interrupt from m2",
        "setup => -l, default 1",
        &epld_int,
    },
    {
        "Spare Interrupt",
        "spare-int",
        "This test verifies the EPLD spare interrupt from m2",
        "setup => -l, default 1",
        &epld_spare_int,
    },
};

struct m1m2_test *m1m2_test_list[MAX_CARD_SLOTS] =
{
    m1_40_list,
    m1_80_list,
    m2_list,
};

int m1m2_test_size[MAX_CARD_SLOTS] =
{
    sizeof(m1_40_list) / sizeof(struct m1m2_test),
    sizeof(m1_80_list) / sizeof(struct m1m2_test),
    sizeof(m2_list) / sizeof(struct m1m2_test),
};

struct m1m2_setup m1_40_default[] =
{
    { 1, 1, 0, 0},  /* i2c-reset */
    { 1, 1, 0, 0},  /* i2c-temp */
    { 1, 1, 10, 0},  /* i2c-slave */
    { 1, 1, 64, 0},  /* eth */
    { 1, 1, 0x100000, 0x400000},  /* pcie */
    { 1, 1, 0, 0},  /* hotplug-switch */
    { 1, 1, 0, 0},  /* int */
    { 1, 1, 0, 0},  /* spare-int */
};

struct m1m2_setup m1_40_setup[] =
{
    { 1, 1, 0, 0},  /* i2c-reset */
    { 1, 1, 0, 0},  /* i2c-temp */
    { 1, 1, 10, 0},  /* i2c-slave */
    { 1, 1, 64, 0},  /* eth */
    { 1, 1, 0x100000, 0x400000},  /* pcie */
    { 1, 1, 0, 0},  /* hotplug-switch */
    { 1, 1, 0, 0},  /* int */
    { 1, 1, 0, 0},  /* spare-int */
};

struct m1m2_setup m1_80_default[] =
{
    { 1, 1, 0, 0},  /* i2c-reset */
    { 1, 1, 0, 0},  /* i2c-temp */
    { 1, 1, 10, 0},  /* i2c-slave */
    { 1, 1, 64, 0},  /* eth */
    { 1, 1, 0x100000, 0x400000},  /* pcie */
    { 1, 1, 0, 0},  /* led-timing */
    { 1, 1, 0, 0},  /* hotplug-switch */
    { 1, 1, 0, 0},  /* int */
    { 1, 1, 0, 0},  /* spare-int */
};

struct m1m2_setup m1_80_setup[] =
{
    { 1, 1, 0, 0},  /* i2c-reset */
    { 1, 1, 0, 0},  /* i2c-temp */
    { 1, 1, 10, 0},  /* i2c-slave */
    { 1, 1, 64, 0},  /* eth */
    { 1, 1, 0x100000, 0x400000},  /* pcie */
    { 1, 1, 0, 0},  /* led-timing */
    { 1, 1, 0, 0},  /* hotplug-switch */
    { 1, 1, 0, 0},  /* int */
    { 1, 1, 0, 0},  /* spare-int */
};

struct m1m2_setup m2_default[] =
{
    { 1, 1, 0, 0},  /* i2c-reset */
    { 1, 1, 0, 0},  /* i2c-temp */
    { 1, 1, 10, 0},  /* i2c-slave */
    { 1, 1, 10, 0},  /* uart-9600 */
    { 1, 1, 10, 0},  /* uart-19200 */
    { 1, 1, 10, 0},  /* uart-38400 */
    { 1, 1, 10, 0},  /* uart-115200 */
    { 1, 1, 64, 0},  /* eth */
    { 1, 1, 0x100000, 0x400000},  /* pcie */
    { 1, 1, 0, 0},  /* led-timing */
    { 1, 1, 0, 0},  /* int */
    { 1, 1, 0, 0},  /* spare-int */
};

struct m1m2_setup m2_setup[] =
{
    { 1, 1, 0, 0},  /* i2c-reset */
    { 1, 1, 0, 0},  /* i2c-temp */
    { 1, 1, 10, 0},  /* i2c-slave */
    { 1, 1, 10, 0},  /* uart-9600 */
    { 1, 1, 10, 0},  /* uart-19200 */
    { 1, 1, 10, 0},  /* uart-38400 */
    { 1, 1, 10, 0},  /* uart-115200 */
    { 1, 1, 64, 0},  /* eth */
    { 1, 1, 0x100000, 0x400000},  /* pcie */
    { 1, 1, 0, 0},  /* led-timing */
    { 1, 1, 0, 0},  /* int */
    { 1, 1, 0, 0},  /* spare-int */
};

struct m1m2_setup *m1m2_default_setup[MAX_CARD_SLOTS] = 
{
    m1_40_default,
    m1_80_default,
    m2_default,
};

struct m1m2_setup *m1m2_test_setup[MAX_CARD_SLOTS] = {0, 0, 0};

int
slot_index (char *name)
{
    int i;

    for (i = 0; i < MAX_CARD_SLOTS; i++) {
        if (!strcmp(name, m1m2_slot_str[i]))
            break;
    }
    return (i);
}

int
test_index (int slot, char *name)
{
    int i;
    struct m1m2_test *test_list;

    if (slot >= MAX_CARD_SLOTS)
        return (m1m2_test_size[slot]);

    test_list = m1m2_test_list[slot];

    for (i = 0; i < m1m2_test_size[slot]; i++) {
        if (!strcmp(name, test_list[i].cmd))
            break;
    }
    return (i);
}

static void
m1m2_check (int flag)
{
    uint16_t mid = 0;
    int rev;
    int i, j;
    uint8_t addr;
    unsigned short device;
    unsigned int bar;
    uint32_t uptime = 0;
    struct m1m2_test *tlist;

    if (m1m2_reloc == 0) {
        m1m2_test_setup[0] = m1_40_setup;
        m1m2_test_setup[1] = m1_80_setup;
        m1m2_test_setup[2] = m2_setup;
        m1m2_test_setup[0] = m1_40_setup;
        m1m2_test_size[0] = sizeof(m1_40_list) / sizeof(struct m1m2_test);
        m1m2_test_size[1] = sizeof(m1_80_list) / sizeof(struct m1m2_test);
        m1m2_test_size[2] = sizeof(m2_list) / sizeof(struct m1m2_test);
        m1m2_test_list[0] = m1_40_list;
        m1m2_test_list[1] = m1_80_list;
        m1m2_test_list[2] = m2_list;

        for (i = 0; i < MAX_CARD_SLOTS; i++) {
            tlist = m1m2_test_list[i];
            for (j = 0; j < m1m2_test_size[i]; j++) {
                tlist[j].test += gd->reloc_off;
            }
        }
        m1m2_reloc = 1;
    }

    if (!m1_40_present() && !m1_80_present() && !m2_present())
    {
        if (flag & M1M2_VERBOSE_ON)
            printf("No M1/M2 card present.\n");
        return;
    }
    
    /* always do pcie re-init */
    pcie_init();

    for (i = 0; i < MAX_CARD_SLOTS; i++) {
        m1m2_pcie_bar[i] = 0;
    }

    if (m1_40_present()) {
        m1_40_id(&rev, &mid, &addr);
        device = 0;
        m1m2_id[M1_40_SLOT] = mid;
        pcie_hose_read_config_word(&pcie_hose[0], 
                                   m1m2_pcie_pdf[M1_40_SLOT], 
                                   PCIE_DEVICE_ID, &device);
        if (device == 0x6082) { /* orion CPU */
            pcie_hose_read_config_dword(&pcie_hose[0], 
                                        m1m2_pcie_pdf[M1_40_SLOT],
                                        PCIE_BASE_ADDRESS_3, &bar);
            if (bar == 0) {
                if (mid == I2C_ID_TSUNAMI_UPLINK_SFPPLUS_4PORT_PIC)
                    m1m2_slot_state[M1_40_SLOT] = 0;
                else
                    m1m2_slot_state[M1_40_SLOT] = 1;
                pcie_hose_read_config_dword(&pcie_hose[0], 
                                            m1m2_pcie_pdf[M1_40_SLOT], 
                                            PCIE_BASE_ADDRESS_2, &bar);
                m1m2_pcie_bar[M1_40_SLOT] = bar & 0xfffffff0;
            }
            else {
                if (mid == I2C_ID_TSUNAMI_UPLINK_SFPPLUS_4PORT_PIC)
                    m1m2_slot_state[M1_40_SLOT] = 2;
                else
                    m1m2_slot_state[M1_40_SLOT] = 3;
            }
        }
        else {
            i2c_manual = 1;
            m1m2_card(M1_40_SLOT, 1);
            if ((get_uptime_m1m2(100000, &uptime)) == 0) {
                if (mid == I2C_ID_TSUNAMI_UPLINK_SFPPLUS_4PORT_PIC)
                    m1m2_slot_state[M1_40_SLOT] = 0;
                else
                    m1m2_slot_state[M1_40_SLOT] = 1;
            }
            else
                m1m2_slot_state[M1_40_SLOT] = 4;
            m1m2_card(M1_40_SLOT, 0);
            i2c_manual = 0;
        }
    }
    else {
        m1m2_slot_state[M1_40_SLOT] = 5;
        m1m2_id[M1_40_SLOT] = 0;
    }

    if (m1_80_present()) {
        m1_80_id(&rev, &mid, &addr);
        device = 0;
        m1m2_id[M1_80_SLOT] = mid;
        pcie_hose_read_config_word(&pcie_hose[0], 
                                   m1m2_pcie_pdf[M1_80_SLOT],
                                   PCIE_DEVICE_ID, &device);
        if (device == 0x6082) { /* orion CPU */
            pcie_hose_read_config_dword(&pcie_hose[0], 
                                        m1m2_pcie_pdf[M1_80_SLOT],
                                        PCIE_BASE_ADDRESS_3, &bar);
            if (bar == 0) {
                if (mid == I2C_ID_TSUNAMI_UPLINK_SFPPLUS_4PORT_PIC)
                    m1m2_slot_state[M1_80_SLOT] = 0;
                else
                    m1m2_slot_state[M1_80_SLOT] = 1;
                pcie_hose_read_config_dword(&pcie_hose[0], 
                                            m1m2_pcie_pdf[M1_80_SLOT],
                                            PCIE_BASE_ADDRESS_2, &bar);
                m1m2_pcie_bar[M1_80_SLOT] = bar & 0xfffffff0;
            }
            else {
                if (mid == I2C_ID_TSUNAMI_UPLINK_SFPPLUS_4PORT_PIC)
                    m1m2_slot_state[M1_80_SLOT] = 2;
                else
                    m1m2_slot_state[M1_80_SLOT] = 3;
            }
        }
        else {
            i2c_manual = 1;
            m1m2_card(M1_80_SLOT, 1);
            if ((get_uptime_m1m2(100000, &uptime)) == 0) {
                if (mid == I2C_ID_TSUNAMI_UPLINK_SFPPLUS_4PORT_PIC)
                    m1m2_slot_state[M1_80_SLOT] = 0;
                else
                    m1m2_slot_state[M1_80_SLOT] = 1;
            }
            else
                m1m2_slot_state[M1_80_SLOT] = 4;
            m1m2_card(M1_80_SLOT, 0);
            i2c_manual = 0;
        }
    }
    else {
        m1m2_slot_state[M1_80_SLOT] = 5;
        m1m2_id[M1_80_SLOT] = mid;
    }

    if (m2_present()) {
        m2_id(&rev, &mid, &addr);
        device = 0;
        m1m2_id[M2_SLOT] = mid;
        pcie_hose_read_config_word(&pcie_hose[0], 
                                   m1m2_pcie_pdf[M2_SLOT], 
                                   PCIE_DEVICE_ID, &device);
        if (device == 0x6082) { /* orion CPU */
            pcie_hose_read_config_dword(&pcie_hose[0], 
                                        m1m2_pcie_pdf[M2_SLOT], 
                                        PCIE_BASE_ADDRESS_3, &bar);
            if (bar == 0) {
                if (mid == I2C_ID_TSUNAMI_M2_LOOPBACK_PIC)
                    m1m2_slot_state[M2_SLOT] = 0;
                else
                    m1m2_slot_state[M2_SLOT] = 1;
                pcie_hose_read_config_dword(&pcie_hose[0], 
                                            m1m2_pcie_pdf[M2_SLOT], 
                                            PCIE_BASE_ADDRESS_2, &bar);
                m1m2_pcie_bar[M2_SLOT] = bar & 0xfffffff0;
            }
            else {
                if (mid == I2C_ID_TSUNAMI_M2_LOOPBACK_PIC)
                    m1m2_slot_state[M2_SLOT] = 2;
                else
                    m1m2_slot_state[M2_SLOT] = 3;
            }
        }
        else {
            i2c_manual = 1;
            m1m2_card(M2_SLOT, 1);
            if ((get_uptime_m1m2(100000, &uptime)) == 0) {
                if (mid == I2C_ID_TSUNAMI_M2_LOOPBACK_PIC)
                    m1m2_slot_state[M2_SLOT] = 0;
                else
                    m1m2_slot_state[M2_SLOT] = 1;
            }
            else
                m1m2_slot_state[M2_SLOT] = 4;
            m1m2_card(M2_SLOT, 0);
            i2c_manual = 0;
        }
    }
    else {
        m1m2_slot_state[M2_SLOT] = 5;
        m1m2_id[M2_SLOT] = mid;
    }

    if (flag & M1M2_VERBOSE_ON) {
        printf(" M1-40 slot: M1-SFP+  %-50s\n", 
               m1m2_state_str[m1m2_slot_state[M1_40_SLOT]]);
        printf(" M1-80 slot: M1-SFP+  %-50s\n", 
               m1m2_state_str[m1m2_slot_state[M1_80_SLOT]]);
        printf(" M2    slot: M2-LBK   %-50s\n", 
               m1m2_state_str[m1m2_slot_state[M2_SLOT]]);
    }

}

static void
test_info (void)
{
    int i;

    if (!m1_40_present() && !m1_80_present() && !m2_present())
    {
        printf("No M1/M2 card present.\n");
    }
    
    if (m1m2_slot_state[M1_40_SLOT] <= 1) {
        printf("M1-40 available tests:\n");
        for (i = 0; i < sizeof(m1_40_list) / sizeof(struct m1m2_test); i++) {
            printf ("  %-15s - %s\n", m1_40_list[i].cmd, m1_40_list[i].name);
        }
        printf("\n");
    }

    if (m1m2_slot_state[M1_80_SLOT] <= 1) {
        printf("M1-80 available tests:\n");
        for (i = 0; i < sizeof(m1_80_list) / sizeof(struct m1m2_test); i++) {
            printf ("  %-15s - %s\n", m1_80_list[i].cmd, m1_80_list[i].name);
        }
        printf("\n");
    }

    if (m1m2_slot_state[M2_SLOT] <= 1) {
        printf("M2 available tests:\n");
        for (i = 0; i < sizeof(m2_list) / sizeof(struct m1m2_test); i++) {
            printf ("  %-15s - %s\n", m2_list[i].cmd, m2_list[i].name);
        }
        printf("\n");
    }

}

static void
test_show_all (void)
{
    int i, j;
    struct m1m2_test *tlist;
    struct m1m2_setup *slist;

    if (!m1_40_present() && !m1_80_present() && !m2_present())
    {
        printf("No M1/M2 card present.\n");
    }

    for (i = 0; i < MAX_CARD_SLOTS; i ++) {
        if (m1m2_slot_state[i] > 1)
            continue;

        tlist = m1m2_test_list[i];
        slist = m1m2_test_setup[i];
        
        printf("%-5s:        enable      loop      length      address\n", 
               m1m2_slot_str[i]);
        for (j = 0; j < m1m2_test_size[i]; j++) {
            printf (" %-15s", tlist[j].cmd);
            printf ("     %-3s", slist[j].enable ? "yes" : "no");
            printf ("%10d", slist[j].loop);
            if (slist[j].length)
                printf (" %10d", slist[j].length);
            else
                printf ("         na");
            if (slist[j].address)
                printf ("   0x%08x\n", slist[j].address);
            else
                printf ("           na\n");
        }
        printf("\n");
    }

}

static void
test_show_one (struct m1m2_test *tlist, struct m1m2_setup *slist, 
               int lsize, char *test)
{
    int i;

    printf("              enable      loop      length      address\n");
    for (i = 0; i < lsize; i++) {
        if ((!strcmp(test, tlist[i].cmd)) || (test == NULL)) {
            printf (" %-15s", tlist[i].cmd);
            printf ("     %-3s", slist[i].enable ? "yes" : "no");
            printf ("%10d", slist[i].loop);
            if (slist[i].length)
                printf (" %10d", slist[i].length);
            else
                printf ("         na");
            if (slist[i].address)
                printf ("   0x%08x\n", slist[i].address);
            else
                printf ("           na\n");
            
            if (test != NULL)
                break;
        }
    }
    
}

static void
test_info_detail (struct m1m2_test *tlist, int lsize, char *test)
{
    int i;

    for (i = 0; i < lsize; i++) {
        if ((!strcmp(test, tlist[i].cmd)) || (test == NULL)) {
            printf ("name:   %s\n", tlist[i].name);
            printf ("cmd:    %s\n", tlist[i].cmd);
            printf ("desc:   %s\n", tlist[i].desc);
            printf ("setup:  %s\n", tlist[i].limit);
            printf ("test:   %08x\n", tlist[i].test);
            printf ("\n");
            
            if (test != NULL)
                break;
        }
    }
}

static void
test_enable_m1m2 (struct m1m2_test *tlist, struct m1m2_setup *slist, 
               int lsize, char *test, int ena)
{
    int i;

    for (i = 0; i < lsize; i++) {
        if ((!strcmp(test, tlist[i].cmd)) || (test == NULL)) {
            
            if (test != NULL) {
                slist[i].enable = ena;
                break;
            }
            else 
                slist[i].enable = ena;
        }
    }
    
}

static void
test_default_setup (struct m1m2_setup *dlist, struct m1m2_setup *slist, 
               int lsize, int atloc)
{
    int i;

    if (atloc) {
        dlist[atloc].enable = slist[atloc].enable;
        dlist[atloc].loop = slist[atloc].loop;
        dlist[atloc].length= slist[atloc].length;
        dlist[atloc].address= slist[atloc].address;
        return;
    }

    for (i = 0; i < lsize; i++) {
        dlist[i].enable = slist[i].enable;
        dlist[i].loop = slist[i].loop;
        dlist[i].length= slist[i].length;
        dlist[i].address= slist[i].address;
    }
    
}

static int 
gpio_reset_m1m2 (int slot, int testn)
{
    uint32_t uptime = 0;
    int ret;

    m1m2_reset(slot, 1);  /* reset card */
    udelay(100000);
    m1m2_reset(slot, 0);  /* enable card */
    m1m2_card(slot, 1);

    if ((ret = get_uptime_m1m2(100000, &uptime)) == 0) {
        if (m1m2_flag & M1M2_VERBOSE_ON) {
            printf("Uptime: %d hr %2d min %2d sec\n",
                   uptime / 3600,
                   (uptime % 3600) / 60,
                   uptime % 60);
        }
        if (uptime > 3)
            ret = -1;
    }

    return (ret);       
}

int 
i2c_reset_m1m2 (int slot, int testn)
{
    int result = 0;
    uint8_t data8, data8_1;
    struct m1m2_setup *test_setup;

    test_setup = m1m2_test_setup[slot];
        
    switch(slot) {
        case 0:
            i2c_mux(1, CFG_I2C_C2_9546SW2, 3, 1); 
            i2c_mux(1, CFG_I2C_M1_SFPP_9548SW1, 0, 1); 
            i2c_read(CFG_I2C_M1_SFPP_9548SW1, 0, 0, &data8_1, 1);
            if (m1m2_flag & M1M2_VERBOSE_ON) {
                printf("I2C read addr %02x = %02x\n", 
                       CFG_I2C_M1_SFPP_9548SW1, data8_1);
            }
            i2c_read(CFG_I2C_C2_9506EXP1, 0xc, 1, &data8, 1);
            data8 &= ~0x10;
            i2c_write(CFG_I2C_C2_9506EXP1, 0xc, 1, &data8, 1);
            udelay(10000);
            data8 |= 0x10;
            i2c_write(CFG_I2C_C2_9506EXP1, 0xc, 1, &data8, 1);
            i2c_read(CFG_I2C_M1_SFPP_9548SW1, 0, 0, &data8, 1);
            if (m1m2_flag & M1M2_VERBOSE_ON) {
                printf("I2C read addr %02x = %02x\n", 
                       CFG_I2C_M1_SFPP_9548SW1, data8);
            }
            break;
        case 1:
            i2c_mux(1, CFG_I2C_C2_9546SW2, 2, 1); 
            i2c_mux(1, CFG_I2C_M1_SFPP_9548SW1, 0, 1);
            i2c_read(CFG_I2C_M1_SFPP_9548SW1, 0, 0, &data8_1, 1);
            if (m1m2_flag & M1M2_VERBOSE_ON) {
                printf("I2C read addr %02x = %02x\n", 
                       CFG_I2C_M1_SFPP_9548SW1, data8_1);
            }
            i2c_read(CFG_I2C_C2_9506EXP1, 0xc, 1, &data8, 1);
            data8 &= ~0x20;
            i2c_write(CFG_I2C_C2_9506EXP1, 0xc, 1, &data8, 1);
            udelay(10000);
            data8 |= 0x20;
            i2c_write(CFG_I2C_C2_9506EXP1, 0xc, 1, &data8, 1);
            i2c_read(CFG_I2C_M1_SFPP_9548SW1, 0, 0, &data8, 1);
            if (m1m2_flag & M1M2_VERBOSE_ON) {
                printf("I2C read addr %02x = %02x\n", 
                       CFG_I2C_M1_SFPP_9548SW1, data8);
            }
            break;
        case 2:
            i2c_mux(1, CFG_I2C_C2_9546SW2, 1, 1);
            i2c_mux(1, CFG_I2C_M2_LBK_9548SW1, 0, 1);
            i2c_read(CFG_I2C_M1_SFPP_9548SW1, 0, 0, &data8_1, 1);
            if (m1m2_flag & M1M2_VERBOSE_ON) {
                printf("I2C read addr %02x = %02x\n", 
                       CFG_I2C_M1_SFPP_9548SW1, data8_1);
            }
            i2c_read(CFG_I2C_C2_9506EXP1, 0xc, 1, &data8, 1);
            data8 &= ~0x40;
            i2c_write(CFG_I2C_C2_9506EXP1, 0xc, 1, &data8, 1);
            udelay(10000);
            data8 |= 0x40;
            i2c_write(CFG_I2C_C2_9506EXP1, 0xc, 1, &data8, 1);
            i2c_read(CFG_I2C_M1_SFPP_9548SW1, 0, 0, &data8, 1);
            if (m1m2_flag & M1M2_VERBOSE_ON) {
                printf("I2C read addr %02x = %02x\n", 
                       CFG_I2C_M1_SFPP_9548SW1, data8);
            }
            break;                    
    }

    if (data8 == data8_1) {
        result = -1;
    }

    /* i2c reset also reset 6082, wait 2s for out of reset */
    udelay(2000000);
    pcie_init();
        
    return (result);
}

int 
i2c_temp_m1m2 (int slot, int testn)
{
    int result = 0;
    int temp;
    uint16_t id;
    struct m1m2_setup *test_setup;

    test_setup = m1m2_test_setup[slot];
        
    temp = 1000;
    switch(slot) {
        case 0:
            temp = i2c_read_temp_m1_40(&id);
            break;
        case 1:
            temp = i2c_read_temp_m1_80(&id);
            break;
        case 2:
            temp = i2c_read_temp_m2(&id);
            break;  
    }

    if (m1m2_flag & M1M2_VERBOSE_ON) {
        printf("I2C read temperature = %d\n", temp);
    }

    if (temp == 1000) {
        result = -1;
    }

    return (result);
}

int 
i2c_slave_m1m2 (int slot, int testn)
{
    int len, result = 0;
    uint8_t tdata[50], rdata[50];
    struct m1m2_setup *test_setup;

    test_setup = m1m2_test_setup[slot];
        
    len = test_setup[testn].length;
    m1m2_card(slot, 1);
    build_tx_hdr(tdata, MSG_REQUEST, REQ_ECHO);
    build_tx_data_ascii(tdata, test_string, 8, len);
    update_tx(tdata, len + 8);
    result = m1m2_tx_ascii(tdata, len+8, rdata, len+8, 100000);
    m1m2_card(slot, 0);

    return (result);
}

int 
i2c_uart_m1m2 (int slot, int testn, int baud)
{
    int result = 0;
    struct m1m2_setup *test_setup;

    test_setup = m1m2_test_setup[slot];
        
    m1m2_card(slot, 1);

    if ((result = set_baudrate_m1m2(baud, 100000)) == 0) {
        result = tx_uart_m1m2 (test_string, test_setup[testn].length, 100000);
        m1m2_card(slot, 0);
    }

    return (result);
}

int 
i2c_uart_9600_m1m2 (int slot, int testn)
{
    return (i2c_uart_m1m2(slot, testn, 9600));
}

int 
i2c_uart_19200_m1m2 (int slot, int testn)
{
    return (i2c_uart_m1m2(slot, testn, 19200));
}

int 
i2c_uart_38400_m1m2 (int slot, int testn)
{
    return (i2c_uart_m1m2(slot, testn, 38400));
}

int 
i2c_uart_115200_m1m2 (int slot, int testn)
{
    return (i2c_uart_m1m2(slot, testn, 115200));
}

int 
i2c_eth_m1m2 (int slot, int testn)
{
    int result = 0;
    struct m1m2_setup *test_setup;

    test_setup = m1m2_test_setup[slot];
        
    m1m2_card(slot, 1);
    result = tx_eth_m1m2 (slot, test_setup[testn].length, 100000);
    m1m2_card(slot, 0);

    return (result);
}

int 
cpu_mdio (int slot, int testn)
{
    uint32_t regVal;
    int result = 0;
    struct m1m2_setup *test_setup;

    test_setup = m1m2_test_setup[slot];
    phy_read_m1m2(slot, test_setup[testn].address, &regVal);
    if (regVal == 0xffff)
        result = -1;
    if (m1m2_flag & M1M2_VERBOSE_ON) {
        printf("PHY reg %02x = %04x\n", 
            test_setup[testn].address, regVal);
    }

    return (result);
}

int 
pcie_m1m2 (int slot, int testn)
{
    int result = 0;
    uint32_t addr;
    struct m1m2_setup *test_setup;

    test_setup = m1m2_test_setup[slot];
    
    addr = test_setup[testn].address + m1m2_pcie_bar[slot];
    
    if (memtest(addr, (addr + test_setup[testn].length - 1), 1, 0))
        result = -1;

    return (result);
}

int 
epld_led_timing (int slot, int testn)
{
    uint16_t temp = 0, temp1;
    int ret = 0;

    switch(slot) {
        /* not applicable for slot 0 M1-40 */
        case 1:
            /* set timing bit */
            epld_reg_read(EPLD_LED_MISC_CTRL, &temp1);
            /* enable M1-80 LED port 4-7 parallel mode */
            epld_reg_write(EPLD_LED_MISC_CTRL, 
                           (temp1 | EPLD_LED_M1_80_PMODE) & 0xFFFB);
            epld_reg_write(EPLD_LED_AB_PORT_37_40, 0x7777);
            m1m2_card(slot, 1);
            ret = get_gpio_m1m2(100000, &temp);
            m1m2_card(slot, 0);
            /* clear timing bit */
            epld_reg_write(EPLD_LED_AB_PORT_37_40, 0x0);
            /* disable M1-80 LED port 4-7 parallel mode */
            epld_reg_write(EPLD_LED_MISC_CTRL, (temp1 | EPLD_LED_M1_80_PMODE));
            if ((ret == 0) && (m1m2_flag & M1M2_VERBOSE_ON)) {
                printf("GPIN = %04x\n", temp);
            }
            if ((ret == 0) && ((temp & 0x0F00) != 0x0F00)) {
                if (m1m2_flag & M1M2_VERBOSE_ON) {
                    /* reg 0x3e set 0x0700 */
                    if ((temp & 0x0100) == 0x0100)
                        printf("Timing A passed\n");
                    else
                        printf("Timing A failed\n");
                    
                    /* reg 0x3e set 0x7000 */
                    if ((temp & 0x0200) == 0x0200)
                        printf("Timing B passed\n");
                    else
                        printf("Timing B failed\n");
                    
                    /* reg 0x3e set 0x0007 */
                    if ((temp & 0x0400) == 0x0400)
                        printf("Timing C passed\n");
                    else
                        printf("Timing C failed\n");
                    
                    /* reg 0x3e set 0x0070 */
                    if ((temp & 0x0800) == 0x0800)
                        printf("Timing D passed\n");
                    else
                        printf("Timing D failed\n");
                }
                ret = -1;
            }
            break;  
            
        case 2:
            /* set timing bit */
            epld_reg_write(EPLD_LED_AB_PORT_45_48, 0x7777);
            m1m2_card(slot, 1);
            ret = get_gpio_m1m2(100000, &temp);
            m1m2_card(slot, 0);
            /* clear timing bit */
            epld_reg_write(EPLD_LED_AB_PORT_45_48, 0x0);
            if ((ret == 0) && (m1m2_flag & M1M2_VERBOSE_ON)) {
                printf("GPIN = %04x\n", temp);
            }
            if ((ret == 0) && ((temp & 0x0F00) != 0x0F00)) {
                if (m1m2_flag & M1M2_VERBOSE_ON) {
                    /* reg 0x3c set 0x0700 */
                    if ((temp & 0x0100) == 0x0100)
                        printf("Timing A passed\n");
                    else
                        printf("Timing A failed\n");
                    
                    /* reg 0x3c set 0x7000 */
                    if ((temp & 0x0200) == 0x0200)
                        printf("Timing B passed\n");
                    else
                        printf("Timing B failed\n");
                    
                    /* reg 0x3c set 0x0007 */
                    if ((temp & 0x0400) == 0x0400)
                        printf("Timing C passed\n");
                    else
                        printf("Timing C failed\n");
                    
                    /* reg 0x3c set 0x0070 */
                    if ((temp & 0x0800) == 0x0800)
                        printf("Timing D passed\n");
                    else
                        printf("Timing D failed\n");
                }
                ret = -1;
            }
            break;  
    }

    return (ret);
}

int 
epld_switch (int slot, int testn)
{
    uint16_t temp = 0;
    int ret = 0;

    epld_reg_read(EPLD_INT_STATUS1, &temp);
    
    if (m1m2_flag & M1M2_VERBOSE_ON) {
        printf("EPLD reg 0x14 = %04x\n", temp);
    }
                    
    switch(slot) {
        case 0:
            if ((temp & 0x0800) == 0)
                ret = -1;

            if (m1m2_flag & M1M2_VERBOSE_ON) {
                printf("%s hotplug switch %s.\n",
                    m1m2_slot_str[slot],
                    (temp & 0x0800) ? "off" : "on");
            }
            break;  
            
        case 1:
            if ((temp & 0x0400) == 0)
                ret = -1;
            
            if (m1m2_flag & M1M2_VERBOSE_ON) {
                printf("%s hotplug switch %s.\n",
                    m1m2_slot_str[slot],
                    (temp & 0x0400) ? "off" : "on");
            }
            break;  
            
        case 2:
            if ((temp & 0x1000) == 0)
                ret = -1;
            
            if (m1m2_flag & M1M2_VERBOSE_ON) {
                printf("%s hotplug switch %s.\n",
                    m1m2_slot_str[slot],
                    (temp & 0x1000) ? "off" : "on");
            }
            break;  
    }

    return (ret);
}

int 
epld_int (int slot, int testn)
{
    uint8_t data8[2];
    uint16_t temp = 0, temp_1, mask = 0;
    int ret = 0;

    epld_reg_read(EPLD_INT_STATUS1, &temp);
    if (m1m2_flag & M1M2_VERBOSE_ON) {
        printf("EPLD reg 0x14 = %04x\n", temp);
    }

    switch(slot) {
        case 0:
            mask = 0x0100;
            i2c_mux(1, CFG_I2C_C2_9546SW2, 3, 1);
            i2c_mux(1, CFG_I2C_M1_SFPP_9548SW1, 5, 1);
            i2c_read(CFG_I2C_M1_SFPP_9548SW1_P5_LM75,
                     0x0, 1, data8, 2);
            data8[1] = 0;
            data8[0] -= 5;
            i2c_write(CFG_I2C_M1_SFPP_9548SW1_P5_LM75,
                      0x2, 1, data8, 2);  /* Thyst */
            data8[0] += 2;
            i2c_write(CFG_I2C_M1_SFPP_9548SW1_P5_LM75,
                      0x3, 1, data8, 2);  /* Tos */
            udelay(300000);
            epld_reg_read(EPLD_INT_STATUS1, &temp_1);
            data8[0] = 0x50;  /* default */
            i2c_write(CFG_I2C_M1_SFPP_9548SW1_P5_LM75,
                      0x3, 1, data8, 2);  /* Tos */
            data8[0] = 0x4b;  /* default */
            i2c_write(CFG_I2C_M1_SFPP_9548SW1_P5_LM75,
                      0x2, 1, data8, 2);  /* Thyst */
            i2c_mux(1, CFG_I2C_M1_SFPP_9548SW1, 5, 0);
            i2c_mux(1, CFG_I2C_C2_9546SW2, 3, 0);
            break;
            
        case 1:
            mask = 0x0080;
            i2c_mux(1, CFG_I2C_C2_9546SW2, 2, 1); 
            i2c_mux(1, CFG_I2C_M1_SFPP_9548SW1, 5, 1);
            i2c_read(CFG_I2C_M1_SFPP_9548SW1_P5_LM75,
                     0x0, 1, data8, 2);
            data8[1] = 0;
            data8[0] -= 5;
            i2c_write(CFG_I2C_M1_SFPP_9548SW1_P5_LM75,
                      0x2, 1, data8, 2);  /* Thyst */
            data8[0] += 2;
            i2c_write(CFG_I2C_M1_SFPP_9548SW1_P5_LM75,
                      0x3, 1, data8, 2);  /* Tos */
            udelay(300000);
            epld_reg_read(EPLD_INT_STATUS1, &temp_1);
            data8[0] = 0x50;  /* default */
            i2c_write(CFG_I2C_M1_SFPP_9548SW1_P5_LM75,
                      0x3, 1, data8, 2);  /* Tos */
            data8[0] = 0x4b;  /* default */
            i2c_write(CFG_I2C_M1_SFPP_9548SW1_P5_LM75,
                      0x2, 1, data8, 2);  /* Thyst */
            i2c_mux(1, CFG_I2C_M1_SFPP_9548SW1, 5, 0);
            i2c_mux(1, CFG_I2C_C2_9546SW2, 2, 0);
            break;
            
        case 2:
            mask = 0x0200;
            i2c_mux(1, CFG_I2C_C2_9546SW2, 1, 1);
            i2c_mux(1, CFG_I2C_M2_LBK_9548SW1, 2, 1);
            i2c_read(CFG_I2C_M2_LBK_9548SW1_P2_LM75,
                     0x0, 1, data8, 2);
            data8[1] = 0;
            data8[0] -= 5;
            i2c_write(CFG_I2C_M1_SFPP_9548SW1_P5_LM75,
                      0x2, 1, data8, 2);  /* Thyst */
            data8[0] += 2;
            i2c_write(CFG_I2C_M1_SFPP_9548SW1_P5_LM75,
                      0x3, 1, data8, 2);  /* Tos */
            udelay(300000);
            epld_reg_read(EPLD_INT_STATUS1, &temp_1);
            data8[0] = 0x50;  /* default */
            i2c_write(CFG_I2C_M1_SFPP_9548SW1_P5_LM75,
                      0x3, 1, data8, 2);  /* Tos */
            data8[0] = 0x4b;  /* default */
            i2c_write(CFG_I2C_M1_SFPP_9548SW1_P5_LM75,
                      0x2, 1, data8, 2);  /* Thyst */
            i2c_mux(1, CFG_I2C_M2_LBK_9548SW1, 2, 0);
            i2c_mux(1, CFG_I2C_C2_9546SW2, 1, 0);
            break;
    }
    
    if (m1m2_flag & M1M2_VERBOSE_ON) {
        printf("EPLD reg 0x14 = %04x\n", temp_1);
    }

    if ((ret == 0) && ((temp & mask) == 0)) {
        ret = -1;
    }
    if ((ret == 0) && (temp_1 & mask)) {
        ret = -1;
    }
    return (ret);
}

int 
epld_spare_int (int slot, int testn)
{
    uint16_t temp = 0;
    int ret = 0;

    m1m2_card(slot, 1);
    /* set 6084 GPOUT bit 6 */
    if ((ret = set_gpio_m1m2(100000, 0x40, 0x40)) == 0) {
        epld_reg_read(EPLD_INT_STATUS1, &temp);
        /* clear 6084 GPOUT bit 6 */
        set_gpio_m1m2(100000, 0, 0x40);
    }
    m1m2_card(slot, 0);

    if (ret == 0) {
        if (m1m2_flag & M1M2_VERBOSE_ON) {
            printf("EPLD reg 0x14 = %04x\n", temp);
        }
        
        switch(slot) {
            case 0:
                if ((temp & 0x4000) == 0)
                    ret = -1;
                break;
            case 1:
                if ((temp & 0x2000) == 0)
                    ret = -1;
                break;
            case 2:
                if ((temp & 0x8000) == 0)
                    ret = -1;
                break;
        }
    }

    return (ret);
}

static void
test_exec_m1m2 (int slot, char *test)
{
    int i, j, lsize;
    int result[MAX_M1M2_TEST];
    int count[MAX_M1M2_TEST];
    int testinc[MAX_M1M2_TEST];
    int fail = 0;
    struct m1m2_test *tlist;
    struct m1m2_setup*setup;

    memset(result, 0x0, sizeof(result));
    memset(count, 0x0, sizeof(count));
    memset(testinc, 0x0, sizeof(testinc));
    
    tlist = m1m2_test_list[slot];
    lsize = m1m2_test_size[slot];
    setup = m1m2_test_setup[slot];

    i2c_manual = 1;
    for (i = 0; i < lsize; i++) {
        if ((!strcmp(test, tlist[i].cmd)) || (test == NULL)) {
            if (setup[i].enable) {
                for (j = 0; j < setup[i].loop; j++) {
                    if (m1m2_flag & M1M2_DEBUG_ON) {
                        printf("* run test %s\n", tlist[i].cmd);
                    }
                    result[i] = tlist[i].test(slot, i);
                    if (result[i]) {
                        fail = -1;
                        break;
                    }
                    count[i] = j;
                }
            }
            if (test != NULL) {
                testinc[i] = 1;
                break;
            }
            else {
                testinc[i] = 1;
            }
        }
    }
    i2c_manual = 0;
    
    if (test == NULL) {
        if (fail)
            printf("-- %-10s                                               "
                   "FAILED   @\n", 
                   id_to_name(m1m2_id[slot]));
        else
            printf("-- %-10s                                               "
                   "PASSED\n", 
                   id_to_name(m1m2_id[slot]));
    }
    
    for (i = 0; i < lsize; i++) {
        if (testinc[i]) {
            if (setup[i].enable) {
                if (result[i])
                    printf (" > %-15s: %-20s       %4d/%4d    FAILED   @\n", 
                            tlist[i].cmd, tlist[i].name, count[i], setup[i].loop);
                else
                    printf (" > %-15s: %-20s       %4d/%4d    PASSED\n", 
                            tlist[i].cmd, tlist[i].name, count[i]+1, setup[i].loop);
            }
            else
                printf (" > %-15s: %-20s                    DISABLED\n", 
                        tlist[i].cmd, tlist[i].name);
        }
    }
    printf("\n");
}

int 
do_testc (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
    char cmd1, cmd1_1, cmd1_2;
    int sindex, tindex;
    int i;
    struct m1m2_setup *psetup;

    if (argc == 1) {
        /* List test info */
        m1m2_check(0);
        test_info();
        return 1;
    }

    cmd1 = argv[1][0];
    cmd1_1 = argv[1][1];
    cmd1_2 = argv[1][2];
    
    switch(cmd1){
        case 'i':       /* init */
        case 'I':
            if ((cmd1_2 == 'i') || (cmd1_2 == 'I'))  /* init */
                m1m2_check(M1M2_VERBOSE_ON);
            else if ((cmd1_2 == 'f') || (cmd1_2 == 'F')) { /* info*/
                if (argc == 2) {
                    for (i = 0; i < MAX_CARD_SLOTS; i++) {
                        printf("%s:\n", m1m2_slot_str[i]);
                        test_info_detail(m1m2_test_list[i],
                                         m1m2_test_size[i],
                                         NULL
                                         );
                    }
                }
                else if (argc == 3) {
                    if ((sindex = slot_index(argv[2])) < MAX_CARD_SLOTS) {
                        printf("%s:\n", m1m2_slot_str[sindex]);
                        test_info_detail(m1m2_test_list[sindex],
                                         m1m2_test_size[sindex],
                                         NULL
                                         );
                    }
                }
                else if (argc == 4) {
                    if ((sindex = slot_index(argv[2])) < MAX_CARD_SLOTS) {
                        test_info_detail(m1m2_test_list[sindex],
                                         m1m2_test_size[sindex],
                                         argv[3]
                                         );
                    }
                }
                else
                    goto usage;
            }
            else
                goto usage;
                
            break;
            
        case 'e':       /* enable */
        case 'E':
            if (argc <= 2)
                goto usage;

            if (argc == 2) {
                for (i = 0; i < MAX_CARD_SLOTS; i++) {
                    test_enable_m1m2(m1m2_test_list[i], 
                                     m1m2_test_setup[i], 
                                     m1m2_test_size[i],
                                     NULL,
                                     1);
                }
            }
            else if (argc == 3) {
                if ((sindex = slot_index(argv[2])) < MAX_CARD_SLOTS) {
                    if (m1m2_slot_state[sindex] <= 1)
                        test_enable_m1m2(m1m2_test_list[sindex], 
                                         m1m2_test_setup[sindex], 
                                         m1m2_test_size[sindex],
                                         NULL,
                                         1);
                }
                else
                    goto usage;
            }
            else if (argc == 4) {
                if ((sindex = slot_index(argv[2])) < MAX_CARD_SLOTS) {
                    if (m1m2_slot_state[sindex] <= 1)
                        test_enable_m1m2(m1m2_test_list[sindex], 
                                         m1m2_test_setup[sindex], 
                                         m1m2_test_size[sindex],
                                         argv[3],
                                         1);
                }
                else
                    goto usage;
            }
            else
               goto usage;
                
            break;
            
        case 'd':       /* disable/default */
        case 'D':
            if ((cmd1_1 == 'i') || (cmd1_1 == 'I')) { /* disable */
                if (argc == 2) {
                    for (i = 0; i < MAX_CARD_SLOTS; i++) {
                        test_enable_m1m2(m1m2_test_list[i], 
                                         m1m2_test_setup[i], 
                                         m1m2_test_size[i],
                                         NULL,
                                         0);
                    }
                }
                else if (argc == 3) {
                    if ((sindex = slot_index(argv[2])) < MAX_CARD_SLOTS) {
                        if (m1m2_slot_state[sindex] <= 1)
                            test_enable_m1m2(m1m2_test_list[sindex], 
                                             m1m2_test_setup[sindex], 
                                             m1m2_test_size[sindex],
                                             NULL,
                                             0);
                    }
                    else
                        goto usage;
                }
                else if (argc == 4) {
                    if ((sindex = slot_index(argv[2])) < MAX_CARD_SLOTS) {
                        if (m1m2_slot_state[sindex] <= 1)
                            test_enable_m1m2(m1m2_test_list[sindex], 
                                             m1m2_test_setup[sindex], 
                                             m1m2_test_size[sindex],
                                             argv[3],
                                             0);
                    }
                    else
                        goto usage;
                }
                else
                    goto usage;
            }
            else if ((cmd1_1 == 'e') || (cmd1_1 == 'E')) {
                /* default */
                if (argc == 2) {
                    for (i = 0; i < MAX_CARD_SLOTS; i++)
                        test_default_setup(m1m2_test_setup[i],
                                           m1m2_default_setup[i],
                                           m1m2_test_size[i],
                                           0
                                           );
                }
                else if (argc == 3) {
                    if ((sindex = slot_index(argv[2])) < MAX_CARD_SLOTS) {
                        test_default_setup(m1m2_test_setup[sindex],
                                           m1m2_default_setup[sindex],
                                           m1m2_test_size[sindex],
                                           0
                                           );
                    }
                    else
                        goto usage;
                }
                else if (argc == 4) {
                    if ((sindex = slot_index(argv[2])) >= MAX_CARD_SLOTS)
                        goto usage;
                    if ((tindex = test_index(sindex, argv[3])) >= 
                        MAX_CARD_SLOTS)
                        goto usage;
                    
                    test_default_setup(m1m2_test_setup[sindex],
                                       m1m2_default_setup[sindex],
                                       m1m2_test_size[sindex],
                                       tindex
                                       );
                }
                else
                    goto usage;
            }
            else
                goto usage;
                
            break;
            
        case 's':       /* show/set */
        case 'S':
            if ((cmd1_1 == 'h') || (cmd1_1 == 'H')) {  /* show */
                if (argc == 2)
                    test_show_all ();
                else if (argc == 3) {
                    if ((sindex = slot_index(argv[2])) < MAX_CARD_SLOTS) {
                        if (m1m2_slot_state[sindex] <= 1)
                            test_show_one(m1m2_test_list[sindex], 
                                          m1m2_test_setup[sindex], 
                                          m1m2_test_size[sindex],
                                          NULL);
                    }
                    else
                        goto usage;
                }
                else if (argc == 4) {
                    if ((sindex = slot_index(argv[2])) < MAX_CARD_SLOTS) {
                        if (m1m2_slot_state[sindex] <= 1)
                            test_show_one(m1m2_test_list[sindex], 
                                          m1m2_test_setup[sindex], 
                                          m1m2_test_size[sindex],
                                          argv[3]);
                    }
                    else
                        goto usage;
                }
                else
                    goto usage;
            }
            else if ((cmd1_1 == 'e') || (cmd1_1 == 'E')) { /* set */
                if (argc < 6)
                    goto usage;
                
                if ((sindex = slot_index(argv[2])) >= MAX_CARD_SLOTS) 
                    goto usage;
                    
                if ((tindex = test_index(sindex, argv[3])) >=
                    m1m2_test_size[sindex])
                    goto usage;

                psetup = m1m2_test_setup[sindex];

                if ((argc == 6) || (argc == 8) || (argc == 10)) {
                    do {
                        if (!strcmp(argv[argc-2], "-a"))
                            psetup[tindex].address = 
                                simple_strtoul(argv[argc-1], NULL, 16);
                        else if (!strcmp(argv[argc-2], "-s"))
                            psetup[tindex].length = 
                                simple_strtoul(argv[argc-1], NULL, 10);
                        else if (!strcmp(argv[argc-2], "-l"))
                            psetup[tindex].loop = 
                                simple_strtoul(argv[argc-1], NULL, 10);
                        else
                            goto usage;
                        
                        argc -= 2;
                    } while (argc > 4);
                }
                else
                    goto usage;
            }
            else
                goto usage;
            
            break;
            
        case 'r':       /* run */
        case 'R':
            
            if (!strcmp(argv[argc-1], "-d")) {
                m1m2_flag |= M1M2_DEBUG_ON;
                argc--;
                if (!strcmp(argv[argc-1], "-v")) {
                    m1m2_flag |= M1M2_VERBOSE_ON;
                    argc--;
                }
            }
            else if (!strcmp(argv[argc-1], "-v")) {
                m1m2_flag |= M1M2_VERBOSE_ON;
                argc--;
                if (!strcmp(argv[argc-1], "-d")) {
                    m1m2_flag |= M1M2_DEBUG_ON;
                    argc--;
                }
            }
            
            if (argc == 2) {
                for (i = 0; i < MAX_CARD_SLOTS; i++) {
                    if (m1m2_slot_state[i] <= 1) {
                        printf("%s:\n", m1m2_slot_str[i]);
                        test_exec_m1m2(i, NULL);
                    }
                    else {
                        printf("%s: Skip (%s)\n\n", 
                               m1m2_slot_str[i],
                               m1m2_state_str[m1m2_slot_state[i]]);
                    }
                }
                m1m2_flag = 0;
            }
            else if (argc == 3) {
                if ((sindex = slot_index(argv[2])) < MAX_CARD_SLOTS) {
                    if (m1m2_slot_state[sindex] <= 1) {
                        printf("%s:\n", m1m2_slot_str[sindex]);
                        test_exec_m1m2(sindex, NULL);
                    }
                    else {
                        printf("%s: Skip (%s)\n\n", 
                               m1m2_slot_str[sindex],
                               m1m2_state_str[m1m2_slot_state[sindex]]);
                    }
                }
                m1m2_flag = 0;
            }
            else if (argc == 4) {
                if ((sindex = slot_index(argv[2])) < MAX_CARD_SLOTS) {
                    if (m1m2_slot_state[sindex] <= 1) {
                        test_exec_m1m2(sindex, argv[3]);
                    }
                    else {
                        printf("%s: Skip (%s)\n\n", 
                               m1m2_slot_str[sindex],
                               m1m2_state_str[m1m2_slot_state[sindex]]);
                    }
                }
                m1m2_flag = 0;
            }
            else
                goto  usage;
            
            break;
            
        default:
            goto usage;
            break;
    }

    return (0);
    
 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return (1);
}

/***************************************************/

U_BOOT_CMD(
    m1m2,    7,  1,  do_m1m2,
    "m1m2    - I2C m1m2 message\n",
    "\n"
    "m1m2 [enable [m1-40|m1-80|m2] | disable] \n"
    "    - enable/diable i2c channel to card\n"
    "m1m2 dw <tx-data> <rx-len> [<delay>]\n"
    "    - transmit/receive message to card\n"
    "m1m2 cmd sync [<delay>]\n"
    "    - command sync to card\n"
    "m1m2 cmd baudrate [9600|19200|38400|115200] [<delay>]\n"
    "    - command baudrate to card\n"
    "m1m2 req echo <data> [<delay>]\n"
    "    - request i2c echo from card\n"
    "m1m2 req uart <string> [<delay>]\n"
    "    - request uart echo string from card\n"
    "m1m2 req eth <length> [<delay>]\n"
    "    - request Ethernet echo packet from card\n"
    "m1m2 req gpio get [<delay>]\n"
    "    - request gpio setting from card\n"
    "m1m2 req gpio set  <data> <mask> [<delay>]\n"
    "    - request set gpio to card\n"
    "m1m2 req run <command> [<delay>]\n"
    "    - request run command thru i2c to card\n"
    "m1m2 req uptime [<delay>]\n"
    "    - request uptime thru i2c from card\n"
    "m1m2 req version [<delay>]\n"
    "    - request version thru i2c from card\n"
    "m1m2 read phy <reg>\n"
    "    - read cpu MDC/MDIO\n"
    "m1m2 read hotplug-switch [m1-40|m1-80|m2|all]\n"
    "    - read M1-40/M1-80/M2 switches' status\n"
    "m1m2 set hotplug-led [m1-40|m1-80|m2|all][on|off]\n"
    "    - set M1-40/M1-80/M2 hotplug led\n"
);

U_BOOT_CMD(
    testc,	10,	0,	do_testc,
    "testc   - perform M1/M2 test circuit test\n",
    "    - print list of available tests\n"
    "testc init\n"
    "         - check card status\n"
    "testc info [m1-40|m1-80|m2] <test>\n"
    "         - detail test information\n"
    "testc default [m1-40|m1-80|m2] <test>\n"
    "         - set default about specified test\n"
    "testc show [m1-40|m1-80|m2] <test>\n"
    "         - print setup information about specified test\n"
    "testc set [m1-40|m1-80|m2] <test>[-l <loop>][-s <len>][-a <addr>]\n"
    "         - edit setup about specified test\n"
    "testc [enable|disable] [m1-40|m1-80|m2] <test>\n"
    "         - edit enable/disable about specified test\n"
    "testc run [-v] [-d]\n"
    "         - run all available tests\n"
    "testc run [m1-40|m1-80|m2] <test> [-v] [-d]\n"
    "         - run specified test\n"
);
#endif
