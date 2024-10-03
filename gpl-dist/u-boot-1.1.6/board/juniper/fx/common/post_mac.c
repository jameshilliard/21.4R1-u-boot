/*
 * Copyright (c) 2009-2011, Juniper Networks, Inc.
 * All rights reserved.
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
 * You should have received a copy of the GNU General Public
 * License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <watchdog.h>
#include <command.h>
#include <net.h>


#include <malloc.h>

#include <rmi/rmigmac.h>
#include <rmi/gmac.h>
#include <rmi/on_chip.h>
#include <rmi/xlr_cpu.h>
#include <rmi/shared_structs.h>
#include <rmi/cpu_ipc.h>

#include "post_mac.h"

#include <common/post.h>

static int port_loopback_state;
static int port_loopback_mask = 0xFF;

static struct eth_device*diag_eth_current;
static struct eth_device*diag_eth_devices;

int diag_compare_data(uchar *pbuf, int len);
static int *original_data = 0;



static int malloc_address_data_base[200 * 8];
static int index_total_malloc = 0;

extern struct boot1_info boot_info;

extern struct rmi_processor_info rmi_processor;
extern struct rmi_board_info rmi_board;

static int max_packets_tx_count;

static void *
cacheline_aligned_malloc (int size)
{
    void *addr = 0;

    /* addr = smp_malloc(size + (CACHELINE_SIZE << 1)); */

    debug(POST_VERB_STD,"%s: \n", __FUNCTION__);

    addr = malloc(size + (CACHELINE_SIZE << 1));

    if (!addr) {
        post_printf(POST_VERB_STD,"[%s]: unable to allocate memory!\n", __FUNCTION__);
        return 0;
    }


    malloc_address_data_base[index_total_malloc++] = (int)addr;
    if (index_total_malloc > 200 * 8) {
        post_printf(POST_VERB_DBG,
            "Warning: Oveloaded With sMmemory Allocation, Resseting the index to 0 \n");
        index_total_malloc = 0;
    }

// align the data to the next cache line twy/
    return (void *)((((unsigned long)addr) + CACHELINE_SIZE) &
                    ~(CACHELINE_SIZE - 1));
}


static void
mac_make_desc_b0_rfr (struct msgrng_msg *msg, int id,
                      unsigned long addr, int *pstid)
{
    int stid = 0;

    stid = msgrng_gmac_stid_rfr(id);
    msg->msg0 = (uint64_t)addr & 0xffffffffe0ULL;
    *pstid = stid;
}


static void
mac_make_desc_xls_rfr (struct msgrng_msg *msg, int id,
                       unsigned long addr, int *pstid)
{
    int stid = 0;

    if (id < 4) {
        stid = msgrng_gmac0_stid_rfr(id);
    } else {
        stid = msgrng_gmac1_stid_rfr(id - 4);
    }

    msg->msg0 = (uint64_t)addr & 0xffffffffe0ULL;
    *pstid = stid;
}


static void
mac_make_desc_b0_tx (struct msgrng_msg *msg,
                     int id,
                     unsigned long addr,
                     int len,
                     int *stid,
                     int *size)
{
    int tx_stid = 0;
    int fr_stid = 0;

    tx_stid = msgrng_gmac_stid_tx(id);
    fr_stid = msgrng_cpu_to_bucket(processor_id());

    msg->msg0 = (((uint64_t)CTRL_B0_EOP << 63) |
                 ((uint64_t)fr_stid << 54) |
                 ((uint64_t)len << 40) |
                 ((uint64_t)addr & 0xffffffffffULL)
                 );

    *stid = tx_stid;
    *size = 1;
}


static void
mac_make_desc_xls_tx (struct msgrng_msg *msg,
                      int id,
                      unsigned long addr,
                      int len,
                      int *stid,
                      int *size)
{
    int tx_stid = 0;
    int fr_stid = 0;

    if (id < 4) {
        tx_stid = msgrng_gmac0_stid_tx(id);
    } else {
        tx_stid = msgrng_gmac1_stid_tx(id);
    }

    fr_stid = msgrng_cpu_to_bucket(processor_id());

    msg->msg0 = (((uint64_t)CTRL_B0_EOP << 63) |
                 ((uint64_t)fr_stid << 54) |
                 ((uint64_t)len << 40) |
                 ((uint64_t)addr & 0xffffffffffULL)
                 );

    *stid = tx_stid;
    *size = 1;
}


static uchar *
mac_make_desc_b0_rx (int stid, struct msgrng_msg *msg,
                     int *ctrl, int *port, int *len)
{
    uchar *pbuf = 0;

    pbuf = ((uchar *)phys_to_virt((msg->msg0 & 0xffffffffe0ULL)));
    *len = ((uint16_t)(msg->msg0 >> 40)) & 0x3fff;
    *port = msg->msg0 & 0x0f;

    if ((*len) == 0) {
        *ctrl = CTRL_REG_FREE;
    } else {
        *ctrl = (msg->msg0 >> 63) & 0x01;
    }

    return pbuf;
}


static uchar  *
mac_make_desc_xls_rx
(int stid, struct msgrng_msg *msg, int *ctrl, int *port, int *len)
{
    uchar *pbuf = 0;

    pbuf = ((uchar *)phys_to_virt((msg->msg0 & 0xffffffffe0ULL)));
    *len = ((uint16_t)(msg->msg0 >> 40)) & 0x3fff;

    if (stid == MSGRNG_STNID_GMAC0) {
        *port = msg->msg0 & 0x0f;
    } else if (stid == MSGRNG_STNID_GMAC1) {
        *port = 4 + (msg->msg0 & 0x0f);
    } else {
        post_printf(POST_VERB_DBG,"[%s]: Received from station %d\n", __FUNCTION__, stid);
    }


    if ((*len) == 0) {
        *ctrl = CTRL_REG_FREE;
    } else {
        *ctrl = (msg->msg0 >> 63) & 0x01;
    }

    return pbuf;
}


static unsigned int pbuf_debug = 0;

static void
print_packet (const char *pkt_buf, int len)
{
    int i = 0, j = 0;
    unsigned short *p = (unsigned short *)pkt_buf;
    unsigned int nWords = len / 2;

    if (pbuf_debug) {
        post_printf(POST_VERB_DBG,"Dumping %d bytes\n", len);
        for (i = 0; i < nWords;) {
            post_printf(POST_VERB_DBG,"\t0x%04x:  ", 2 * i);
            for (j = 0; j < 8 ; j++) {
                if (i == nWords) {
                    break;
                }
                post_printf(POST_VERB_DBG,"%04x ", *p);
                p++;  i++;
            }
            post_printf(POST_VERB_DBG,"\n");
        }
    }
}


static void
mac_change_hwaddr (struct eth_device *dev, uint64_t mac_addr)
{
    gmac_eth_info_t *eth_info = (gmac_eth_info_t *)dev->priv;
    phoenix_reg_t *mmio = (phoenix_reg_t *)eth_info->mmio_addr;

    eth_info->dev_addr[0] = (mac_addr >> 40) & 0xff;
    eth_info->dev_addr[1] = (mac_addr >> 32) & 0xff;
    eth_info->dev_addr[2] = (mac_addr >> 24) & 0xff;
    eth_info->dev_addr[3] = (mac_addr >> 16) & 0xff;
    eth_info->dev_addr[4] = (mac_addr >> 8) & 0xff;
    eth_info->dev_addr[5] = (mac_addr >> 0) & 0xff;

    dev->enetaddr[0] = eth_info->dev_addr[0];
    dev->enetaddr[1] = eth_info->dev_addr[1];
    dev->enetaddr[2] = eth_info->dev_addr[2];
    dev->enetaddr[3] = eth_info->dev_addr[3];
    dev->enetaddr[4] = eth_info->dev_addr[4];
    dev->enetaddr[5] = eth_info->dev_addr[5];

    debug("Read MAC Address = %02x:%02x.%02x:%02x.%02x:%02x\n",
           dev->enetaddr[0], dev->enetaddr[1],
           dev->enetaddr[2], dev->enetaddr[3],
           dev->enetaddr[4], dev->enetaddr[5]);

    phoenix_write_reg(mmio, MAC_ADDR0_LO,
                      (eth_info->dev_addr[5] << 24) |
                      (eth_info->dev_addr[4] << 16) |
                      (eth_info->dev_addr[3] << 8)   |
                      (eth_info->dev_addr[2]) );

    phoenix_write_reg(mmio, MAC_ADDR0_HI,
                      (eth_info->dev_addr[1] << 24) |
                      (eth_info->dev_addr[0] << 16));

    phoenix_write_reg(mmio, MAC_ADDR_MASK0_LO, 0xffffffff);
    phoenix_write_reg(mmio, MAC_ADDR_MASK0_HI, 0xffffffff);
    phoenix_write_reg(mmio, MAC_ADDR_MASK1_LO, 0xffffffff);
    phoenix_write_reg(mmio, MAC_ADDR_MASK1_HI, 0xffffffff);
    phoenix_write_reg(mmio, MAC_FILTER_CONFIG, 0x401);
}


static void
rmi_reset_gmac (phoenix_reg_t *mmio)
{
    volatile uint32_t val;

    /* Disable MAC RX */
    val = phoenix_read_reg(mmio, MAC_CONF1);
    val &= ~0x4;
    phoenix_write_reg(mmio, MAC_CONF1, val);

    /* Disable Core RX */
    val = phoenix_read_reg(mmio, RxControl);
    val &= ~0x1;
    phoenix_write_reg(mmio, RxControl, val);

    /* wait for rx to halt */
    while (1) {
        val = phoenix_read_reg(mmio, RxControl);
        if (val & 0x2) {
            break;
        }
        udelay(1 * 1000);
        udelay(1 * 1000);
        udelay(1 * 1000);
    }

    /* Issue a soft reset */
    val = phoenix_read_reg(mmio, RxControl);
    val |= 0x4;
    phoenix_write_reg(mmio, RxControl, val);

    /* wait for reset to complete */
    while (1) {
        val = phoenix_read_reg(mmio, RxControl);
        if (val & 0x8) {
            break;
        }
        udelay(1 * 1000);
        udelay(1 * 1000);
        udelay(1 * 1000);
    }

    /* Clear the soft reset bit */
    val = phoenix_read_reg(mmio, RxControl);
    val &= ~0x4;
    phoenix_write_reg(mmio, RxControl, val);
}


static void mac_init_msgring(struct eth_device *ndev) __attribute__ ((unused));
static void
mac_init_msgring (struct eth_device *ndev)
{
    gmac_eth_info_t *eth_info = (gmac_eth_info_t *)ndev->priv;
    phoenix_reg_t *mmio = (phoenix_reg_t *)eth_info->mmio_addr;
    int gmac_id = eth_info->port_id;
    uint32_t cc_reg = CC_CPU0_0;
    int i = 0;

    if (chip_processor_id_xls()) {
        if (gmac_id < 4) {
            phoenix_write_reg(mmio, G_TX0_BUCKET_SIZE + gmac_id,
                              xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC0_TX0 +
                                                      gmac_id]);
            phoenix_write_reg(mmio, G_RFR0_BUCKET_SIZE,
                              xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC0_FR_0]);
            phoenix_write_reg(mmio, G_RFR1_BUCKET_SIZE,
                              xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC0_FR_1]);
        } else   {
            phoenix_write_reg(mmio, G_TX0_BUCKET_SIZE + (gmac_id & 0x3),
                              xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC1_TX0 +
                                                      (gmac_id & 0x3)]);
            phoenix_write_reg(mmio, G_RFR0_BUCKET_SIZE,
                              xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC1_FR_0]);
            phoenix_write_reg(mmio, G_RFR1_BUCKET_SIZE,
                              xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC1_FR_1]);
        }
    } else  {
        phoenix_write_reg(mmio,  (G_TX0_BUCKET_SIZE + gmac_id),
                          bucket_sizes.bucket[MSGRNG_STNID_GMACTX0 + gmac_id]);
        phoenix_write_reg(mmio, G_RFR0_BUCKET_SIZE,
                          bucket_sizes.bucket[MSGRNG_STNID_GMACRFR_0]);
        phoenix_write_reg(mmio, G_RFR1_BUCKET_SIZE,
                          bucket_sizes.bucket[MSGRNG_STNID_GMACRFR_1]);
    }
    phoenix_write_reg(mmio, G_JFR0_BUCKET_SIZE, 0);
    phoenix_write_reg(mmio, G_JFR1_BUCKET_SIZE, 0);

    for (i = 0; i < 128; i++) {
        if (chip_processor_id_xls()) {
            if (gmac_id < 4) {
                phoenix_write_reg(mmio, (cc_reg + i),
                                  xls_cc_table_gmac0.counters[i >> 3][i & 0x07]);
            } else {
                phoenix_write_reg(mmio, (cc_reg + i),
                                  xls_cc_table_gmac1.counters[i >> 3][i & 0x07]);
            }
        } else {
            phoenix_write_reg(mmio, (cc_reg + i),
                              cc_table_gmac.counters[i >> 3][i & 0x07]);
        }
    }
}


static void
mac_receiver_enable (struct eth_device *dev)
{
    gmac_eth_info_t *eth_info = (gmac_eth_info_t *)dev->priv;
    phoenix_reg_t *mmio = (phoenix_reg_t *)eth_info->mmio_addr;
    uint32_t rx_control = 1;
    int gmac_id = eth_info->port_id;

    if (chip_processor_id_xls()) {
        if ((eth_info->phy_mode == RMI_GMAC_IN_RGMII) && (gmac_id == 0)) {
            rx_control = (1 << 10) | 1;

            debug("configuring gmac%d in RGMII mode...\n", gmac_id);
        } else   {
            rx_control = 1;
            debug("configuring gmac%d in SGMII mode...\n", gmac_id);
        }
    }
    /* Enable the Receiver */
    phoenix_write_reg(mmio, RxControl, rx_control);

    debug("[%s@%d]: gmac_%d: RX_CONTROL = 0x%08x\n", __FUNCTION__,
          __LINE__, gmac_id, phoenix_read_reg(mmio, RX_CONTROL));
}


static int
mac_open (struct eth_device *ndev)
{
    gmac_eth_info_t *eth_info = (gmac_eth_info_t *)ndev->priv;
    int gmac_id = eth_info->port_id;

    int i = 0, stid = 0;
    uchar *pbuf;
    struct msgrng_msg msg;

    /* Send free descs to the MAC */

#ifdef UBOOT_DELETE_ME
    for (i = 0; i < PKTBUFSRX; i++) {
        pbuf = (txbuffer_t *)NetRxPackets[i];
        eth_info->make_desc_rfr(&msg, gmac_id, virt_to_phys(pbuf), &stid);

        post_printf(POST_VERB_DBG,
                    "cpu_%d: Sending rfr_%d @ (%lx) to station id %d\n",
                    processor_id(), i, virt_to_phys(pbuf), stid);

        message_send_block(1, MSGRNG_CODE_MAC, stid, &msg);
    }
#endif

    for (i = 0; i < 16; i++) {
        pbuf = (uchar *)cacheline_aligned_malloc(PKTSIZE_ALIGN);
        if (!pbuf) {
            return 0;
        }
        mac_make_desc_xls_rfr(&msg, gmac_id, virt_to_phys(pbuf), &stid);
        message_send_block(1, MSGRNG_CODE_MAC, stid, &msg);
    }
    mac_receiver_enable(ndev);
    return 1;
}


static void
qfx_sgmii_ref_clock (void)
{
    phoenix_reg_t *mmio = (phoenix_reg_t *)
                          (DEFAULT_RMI_PROCESSOR_IO_BASE +
                           PHOENIX_IO_GPIO_OFFSET);
    u_int32_t val = mmio[0x10];

    val &= ~0x1FF;
    val |= 0xA5;
    mmio[0x10] = val;
}


extern int gmac_phy_mdio_write(phoenix_reg_t *mmio,
                               int phy_id,
                               int reg,
                               uint32_t data);
extern int gmac_phy_mdio_read(phoenix_reg_t *mmio,
                              int phy_id,
                              int reg,
                              uint32_t *data);


static int gmac_drain_msgs(struct eth_device*dev);
static int
gmac_init (struct eth_device*dev, bd_t *bd)
{
    gmac_eth_info_t *eth_info = (gmac_eth_info_t *)dev->priv;
    phoenix_reg_t *mmio = (phoenix_reg_t *)eth_info->mmio_addr;
    int gmac_id = eth_info->port_id;
    uint32_t linkstatus = 0, i;

#if 0
    u_int32_t phy_id = 0;
    gmac_phy_read(eth_info, 0x3, &phy_id);
    post_printf(POST_VERB_STD,"%s[%d]: phy_addr=0x%x phy_id=0x%x model=0x%x\n",
           __func__, __LINE__, eth_info->phy_id, phy_id, (phy_id >> 4) & 0x3F);
#endif

    if (eth_info->init_done) {
        for (i = 0; i < 8; i++) {
            gmac_drain_msgs(dev);
        }
        mac_receiver_enable(dev);

        if ((linkstatus >> 2) & 0x01) {
         
            debug("[%s]:Init previously done and link is Up.\n", __FUNCTION__);

            return 1;
        } else   {
            return 0;
        }
    }

    uint64_t mac_addr = get_macaddr();
    boot_info.mac_addr = mac_addr;

    uint32_t cc_reg = 0;

    i = 0;
    /* Reset the MAC */
    rmi_reset_gmac(mmio);

    qfx_sgmii_ref_clock();

    mac_change_hwaddr(dev, mac_addr);

#ifdef UBOOT_DELETE_ME
    mac_init_msgring(dev);
#endif

    if (gmac_id == 0) {
        boot_info.mac_addr = mac_addr;
    }


    phoenix_write_reg(mmio, IntReg, 0xffffffff);

    /* configure the GMAC_GLUE Registers */
    phoenix_write_reg(mmio, DmaCr0, 0xffffffff);
    phoenix_write_reg(mmio, DmaCr2, 0xffffffff);
    phoenix_write_reg(mmio, DmaCr3, 0xffffffff);



    // Enable Tx
    phoenix_write_reg(mmio, TxControl, ((1 << 14) | 512));
    /* Enable transmit of 512 byte packets */
    phoenix_write_reg(mmio, DescPackCtrl,
// PRE_PAD          ((MAC_BYTE_OFFSET << 17) | ((2 - 1) << 14) | 1536) );
                      ((MAC_BYTE_OFFSET << 17) | ((2 - 1) << 14) | 1536) );
    /* enable mac ip stats */
    phoenix_write_reg(mmio, StatCtrl, 0x04);
    phoenix_write_reg(mmio, L2AllocCtrl, 2);
    /* configure the classifier - use port based classification by default */
    phoenix_write_reg(mmio, ParserConfig, 0x00000008);

/*
 * phoenix_write_reg(mmio,TranslateTable_0,0x00800000);
 * phoenix_write_reg(mmio,TranslateTable_1,0x01800100);
 */


    phoenix_write_reg(mmio, TranslateTable_0, 0x00);
    phoenix_write_reg(mmio, TranslateTable_1, 0x00);

    /* configure PDE - all gmacs send packets to cpu0/bucket0, by default */
    phoenix_write_reg(mmio, PDE_CLASS0_0, 0x01);
    phoenix_write_reg(mmio, PDE_CLASS0_1, 0x00);
    phoenix_write_reg(mmio, PDE_CLASS1_0, 0x01);
    phoenix_write_reg(mmio, PDE_CLASS1_1, 0x00);
    phoenix_write_reg(mmio, PDE_CLASS2_0, 0x01);
    phoenix_write_reg(mmio, PDE_CLASS2_1, 0x00);
    phoenix_write_reg(mmio, PDE_CLASS3_0, 0x01);
    phoenix_write_reg(mmio, PDE_CLASS3_1, 0x00);

    /* Configure the Parser/Classifier Layer 2 information */
    phoenix_write_reg(mmio, L2Type, (0x01 << 0));

    if (chip_processor_id_xls()) {
        if (gmac_id < 4) {
            phoenix_write_reg(mmio, G_TX0_BUCKET_SIZE + gmac_id,
                              xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC0_TX0 +
                                                      gmac_id]);
            phoenix_write_reg(mmio, G_RFR0_BUCKET_SIZE,
                              xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC0_FR_0]);
            phoenix_write_reg(mmio, G_RFR1_BUCKET_SIZE,
                              xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC0_FR_1]);
        } else  {
            phoenix_write_reg(mmio, G_TX0_BUCKET_SIZE + (gmac_id & 0x3),
                              xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC1_TX0 +
                                                      (gmac_id & 0x3)]);
            phoenix_write_reg(mmio, G_RFR0_BUCKET_SIZE,
                              xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC1_FR_0]);
            phoenix_write_reg(mmio, G_RFR1_BUCKET_SIZE,
                              xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC1_FR_1]);
        }
    } else  {
        phoenix_write_reg(mmio, G_TX0_BUCKET_SIZE + gmac_id,
                          bucket_sizes.bucket[MSGRNG_STNID_GMACTX0 + gmac_id]);
        phoenix_write_reg(mmio, G_RFR0_BUCKET_SIZE,
                          bucket_sizes.bucket[MSGRNG_STNID_GMACRFR_0]);
        phoenix_write_reg(mmio, G_RFR1_BUCKET_SIZE,
                          bucket_sizes.bucket[MSGRNG_STNID_GMACRFR_1]);
    }
    phoenix_write_reg(mmio, G_JFR0_BUCKET_SIZE, 0);
    phoenix_write_reg(mmio, G_JFR1_BUCKET_SIZE, 0);

    cc_reg = CC_CPU0_0;

    for (i = 0; i < 128; i++) {
        if (chip_processor_id_xls()) {
            if (gmac_id < 4) {
                phoenix_write_reg(mmio, (cc_reg + i),
                                  xls_cc_table_gmac0.counters[i >> 3][i & 0x07]);
            } else {
                phoenix_write_reg(mmio, (cc_reg + i),
                                  xls_cc_table_gmac1.counters[i >> 3][i & 0x07]);
            }
        } else  {
            phoenix_write_reg(mmio, (cc_reg + i),
                              (cc_table_gmac.counters[i >> 3][i & 0x07]));
        }
    }

    phoenix_write_reg(mmio, MAC_CONF1, 0x135);
    /* mask off all interrupts and clear the existing interupt conditions */
    phoenix_write_reg(mmio, IntMask, 0x0);


    phoenix_write_reg(mmio, MAC_CONF2, (0x7236 | (eth_info->fdx & 0x01)));
    phoenix_write_reg(mmio, CoreControl, 0x00);
    phoenix_write_reg(mmio, SGMII_SPEED, SGMII_SPEED_1000);

    eth_info->init_done = 1;


    return mac_open(dev);
} /* ends ... gmac_init() */


static int
gmac_send (struct eth_device*dev, volatile void *pbuf, int len)
{
    gmac_eth_info_t *eth_info = (gmac_eth_info_t *)dev->priv;
    int gmac_id = eth_info->port_id;
    int stid = 0, size = 0;
    struct msgrng_msg msg;

    debug("%s: addr=0x%08x , len=%d\n", __FUNCTION__, pbuf, len);

    if (max_packets_tx_count++ > MAX_PACKETS) {
        post_printf(POST_VERB_STD,"Reached MAX_PACKETS limit \n");
        return -1;
    }

/*
 * Use application provided buffer, instead of malloc
 * volatile void * pkt_buf = malloc(sizeof(txbuffer_t));
 * if (pkt_buf == NULL) {
 *    printf ("gmac_send: unable to malloc  \n");
 *    return 0;
 * }
 * memcpy((uchar *)pkt_buf, (uchar *) pbuf, len);
 */

    mac_make_desc_xls_tx(&msg,
                         gmac_id,
                         virt_to_phys(pbuf),
                         len,
                         &stid,
                         &size);


    message_send_block(size, MSGRNG_CODE_MAC, stid, &msg);


    /* Let the mac keep the free descriptor */
    return 0;
}


static int
gmac_drain_msgs (struct eth_device*dev)
{
    gmac_eth_info_t *eth_info = (gmac_eth_info_t *)dev->priv;
    int size = 0;
    int code = 0;
    int stid = 0;
    int gmac_id = eth_info->port_id;
    struct msgrng_msg msg, rxmsg;
    uchar *pbuf = 0;
    int ctrl = 0;
    uint32_t counter = 0;

    int id = processor_id();
    int bucket = id & 0x03;
    int port = 0;
    int len = 0;

    post_printf(POST_VERB_DBG,"%s: \n", __FUNCTION__);
    /* Wait indefinitely for a msg */
    for (;;) {
        /* post_printf(POST_VERB_STD,"cpu_%d: Waiting for a message from %s, netdev=%p\n", id, dev->name, dev); */

        udelay(5000);
        counter = read_32bit_cp0_register(COP_0_COUNT);
        if (message_receive(bucket, &size, &code, &stid, &msg)) {
            break;
        }

        /* Msg received */
#if 0  // FIXMe
        pbuf = (uchar *)eth_info->make_desc_rx(stid, &msg, &ctrl, &port, &len);
#endif
        pbuf = (uchar *)mac_make_desc_xls_rx(stid, &msg, &ctrl, &port, &len);

        if (ctrl == CTRL_REG_FREE) {
            /* Free back descriptor from mac */
            post_printf(POST_VERB_DBG,
                        "[%s]: received free desc from mac, %llx\n",
                         __FUNCTION__, msg.msg0);

            /* free(pbuf); */
            continue;
        }


        /* free the descriptor */
#if 0
        eth_info->make_desc_rfr(&rxmsg, gmac_id, virt_to_phys(pbuf), &stid);
#endif
        mac_make_desc_xls_rfr(&rxmsg, gmac_id, virt_to_phys(pbuf), &stid);
        message_send_block(1, MSGRNG_CODE_MAC, stid, &rxmsg);
    }
    return 0;
} /* ends ... gmac_recv() */


static int
gmac_recv (struct eth_device*dev)
{
    gmac_eth_info_t *eth_info = (gmac_eth_info_t *)dev->priv;
    int size = 0;
    int code = 0;
    int stid = 0;
    int gmac_id = eth_info->port_id;
    struct msgrng_msg msg, rxmsg;
    volatile uchar *pbuf = 0, *lbuf = 0;
    int ctrl = 0;
    uint32_t counter = 0;

    int id = processor_id();
    int bucket = id & 0x03;
    int port = 0;
    int len = 0;
    int status = 0;



    /* Wait indefinitely for a msg */
    for (;;) {
retry:

        debug("cpu_%d: Waiting for a message from %s, netdev=%p\n",
               id,
               dev->name,
               dev);

        counter = read_32bit_cp0_register(COP_0_COUNT);

        if (message_receive(bucket, &size, &code, &stid, &msg)) {
            break;
        }


        /* Msg received */
        pbuf = (uchar *)mac_make_desc_xls_rx(stid, &msg, &ctrl, &port, &len);

// If the packets it is not for us ot it is not Broadcast , just drop the packets
        if (ctrl == CTRL_REG_FREE) {
            /* Free back descriptor from mac */
            debug("[%s]: received free desc from mac, %llx\n",
                  __FUNCTION__, msg.msg0);
            max_packets_tx_count--;
            goto retry;
        }

        lbuf = malloc(1536);
        if (!lbuf) {
            post_printf(POST_VERB_STD,"** [%s]: unable to malloc.\n", __FUNCTION__);
        }
        memcpy((uchar *)lbuf, (uchar *)pbuf, len);

        /* pass the packet up to the protocol layers */

#ifdef FX_COMMON_DEBUG
        {
            int i;

            debug("received pkt addr %p len %d\n", pbuf, len);
            for (i = 0; i < len; i++) {
                debug(POST_VERB_STD,"0x%02x ", (unsigned char)(pbuf[i]));
            }


            debug("received pkt addr %p len %d\n", lbuf, len);
            for (i = 0; i < len; i++) {
                debug("0x%02x ", (unsigned char)(lbuf[i]));
            }
        }
#endif


        status = diag_compare_data((uchar *)lbuf + 2, len);

        free((void*)lbuf);
        mac_make_desc_xls_rfr(&rxmsg, gmac_id, virt_to_phys(pbuf), &stid);
        message_send_block(1, MSGRNG_CODE_MAC, stid, &rxmsg);
    }

    return status;
} /* ends ... gmac_recv() */


static void
mac_close (struct eth_device *dev)
{
    gmac_eth_info_t *eth_info = (gmac_eth_info_t *)dev->priv;
    phoenix_reg_t *mmio = (phoenix_reg_t *)eth_info->mmio_addr;

    /* Disable the Receiver */
    phoenix_write_reg(mmio, RxControl, 0);
}


static void
gmac_halt (struct eth_device*dev)
{
    int i;

    mac_close(dev);
    for (i = 0; i < 8; i++) {
        gmac_drain_msgs(dev);
    }
}


int
diag_rmigmac_enet_initialize (bd_t *bis)
{
    gmac_eth_info_t *eth_info;
    struct eth_device*dev;

    max_packets_tx_count = 0;
    unsigned int num, i, gmac_list;
    char *mode = getenv("gmac0mode");
    int gmac0_sgmii = 0;

    post_printf(POST_VERB_DBG,"%s: \n", __FUNCTION__);
    /* By default gmac0 is assumed to be in RGMII mode on XLS boards if the
     * board has one
     */
    if (mode) {
        if ((strcmp(mode, "sgmii")) == 0) {
            gmac0_sgmii = 1;
            post_printf(POST_VERB_DBG,"Configuring GMAC0 in SGMII mode\n");
        }
    }

#if 1
    /* CNG force to sgmii */
    gmac0_sgmii = 1;
#endif

    struct rmi_board_info *rmib = &rmi_board;

    num = rmib->gmac_num;
    gmac_list = rmib->gmac_list;

    i = 0;
    while (i < num) {
        if (!((gmac_list >> i) & 0x01)) {
            continue;
        }

        if ((eth_info = (gmac_eth_info_t *)malloc(sizeof(gmac_eth_info_t)))
            == NULL) {
            post_printf(POST_VERB_STD,"initialize: malloc fail\n");
            return 0;
        }

        eth_info->init_done = 0;
        eth_info->port_id = i;

        /* assume RGMII mode for XLS-gmac0 and XLR */
        if (chip_processor_id_xls()) {
            eth_info->phy_mode = RMI_GMAC_IN_SGMII;
        } else   {
            eth_info->phy_mode = RMI_GMAC_IN_RGMII;
        }

        if ((i == 0) &&
            (chip_processor_id_xls()) &&
            (rmib->maj != RMI_PHOENIX_BOARD_ARIZONA_VIII)
            && (gmac0_sgmii == 0)) {
            eth_info->phy_mode = RMI_GMAC_IN_RGMII;
        }

        eth_info->phy_id = fx_get_gmac_phy_id(i, eth_info->phy_mode);
        eth_info->mmio_addr = fx_get_gmac_mmio_addr(i);
        eth_info->mii_addr = fx_get_gmac_mii_addr(i);

        if (eth_info->phy_mode == RMI_GMAC_IN_SGMII) {
            eth_info->pcs_addr = fx_get_gmac_pcs_addr(i);
            eth_info->serdes_addr = fx_get_gmac_serdes_addr(i);
        } else {
            eth_info->pcs_addr = 0;
            eth_info->serdes_addr = 0;
        }

        if ((dev = (struct eth_device*)malloc(sizeof(*dev))) == NULL) {
            post_printf(POST_VERB_DBG,"initialize: malloc failed\n");
            return 0;
        }
        memset(dev, 0, sizeof *dev);
        post_printf(POST_VERB_DBG,dev->name, "gmac%d", i);

        dev->iobase = eth_info->mmio_addr;
        dev->priv   = (void *)eth_info;
        dev->init   = gmac_init;
        dev->halt   = gmac_halt;
        dev->send   = gmac_send;
        dev->recv   = gmac_recv;

        if (chip_processor_id_xls()) {
            eth_info->make_desc_tx = mac_make_desc_xls_tx;
            eth_info->make_desc_rfr = mac_make_desc_xls_rfr;
            eth_info->make_desc_rx = mac_make_desc_xls_rx;
        } else {
            eth_info->make_desc_tx = mac_make_desc_b0_tx;
            eth_info->make_desc_rfr = mac_make_desc_b0_rfr;
            eth_info->make_desc_rx = mac_make_desc_b0_rx;
        }
        eth_register(dev);
        i++;
    }

//   mac_spill_init();

    return 1;
}


int diag_post_gmac_init(void);

int
diag_post_gmac_init ()
{
    gmac_eth_info_t *eth_info;
    int status;

    // Init all MAC interfaces 0-7
    status = 1;


    debug("%s: \n", __FUNCTION__);

    port_loopback_state = 0;
    diag_eth_devices = 0;
    diag_eth_current = eth_get_dev();
    while (diag_eth_devices != diag_eth_current) {
        if (diag_eth_devices == 0) {
            diag_eth_devices = diag_eth_current;
        }
        eth_info = diag_eth_current->priv;
        eth_info->init_done = 0;

        debug("Initialization device %d \n", eth_info->port_id);

        if (gmac_init(diag_eth_current, 0)) {
            debug("Net Mac Device Port %d has been init fine \n",
                  eth_info->port_id);
        } else {
            post_printf(POST_VERB_STD,"Error: Failed to init MAC interface %d \n",
                   eth_info->port_id);
            port_loopback_state |= 0x1 << eth_info->port_id;
            // Also write the error TLV to eeprom
            status = 0;
        }
        diag_eth_current = diag_eth_current->next;
    }
    return status;
}


#if 1
void
diag_rmi_dump_stats (struct eth_device *dev)
{
    gmac_eth_info_t *eth_info = (gmac_eth_info_t *)dev->priv;
    phoenix_reg_t *mmio = (phoenix_reg_t *)eth_info->mmio_addr;
    int i;



    post_printf(POST_VERB_DBG," GMAC Register \n");
    for (i = 0; i <= 0x6F; i++) {
        post_printf(POST_VERB_DBG,"Mac offset=0x%08x data = 0x%08x \n", i,
               phoenix_read_reg(mmio, (i)));
    }

    post_printf(POST_VERB_DBG," NA register \n");

    for (i = 0xA0; i <= 0x307; i++) {
        post_printf(POST_VERB_DBG,"Mac offset=0x%08x data = 0x%08x \n", i,
               phoenix_read_reg(mmio, (i)));
    }

    post_printf(POST_VERB_DBG,"bucket and cc \n");

    for (i = 0x320; i <= 0x3FF; i++) {
        post_printf(POST_VERB_DBG,"Mac offset=0x%08x data = 0x%08x \n", i,
               phoenix_read_reg(mmio, (i)));
    }
}


#endif


int
diag_compare_data (uchar *pbuf, int len)
{
    int i;

    int *compare_data;
    int *data = original_data;
    int status = 0;

    compare_data = (int*)(pbuf);
    len -= 6;

    for (i = 0; i < len / 4; i++) {
        if (*compare_data != *data) {
            post_printf(POST_VERB_STD,
                "Error: Mac Pattern Failed at Location=%d Expected=%08x Actual=%08x \n",
                i,
                *data,
                *compare_data);
            status = 1;
        }
        compare_data++;
        data++;
    }
    return status;
}


extern uchar NetBcastAddr[6];
extern int
NetSetEther(volatile uchar *xet, uchar *addr, uint prot);

extern IPaddr_t NetArpWaitPacketIP;
extern IPaddr_t NetArpWaitReplyIP;

int diag_post_gmac_run(void);

int
diag_post_gmac_run ()
{
//    bd_t *bd = gd->bd;
    volatile int *pbuf;
    volatile unsigned char *pbuf_pttern;
    gmac_eth_info_t *eth_info;
    int status = 0;
    int i;
    volatile uchar *pkt;
    ARP_t *arp;

    debug("%s: \n", __FUNCTION__);

    diag_eth_devices = 0;
    diag_eth_current = eth_get_dev();

    pbuf = malloc(2000);   // allocate buffer
    pbuf_pttern = (unsigned char *)pbuf;

    // Fll pattern

    for (i = 0; i < 100; i++) {
        *pbuf_pttern++ = i;
    }



    original_data =  (int *)pbuf;
    pkt = (volatile uchar *)pbuf;
    pbuf_pttern = (volatile unsigned char *)pbuf;



    pkt += NetSetEther(pkt, NetBcastAddr, PROT_ARP);

    arp = (ARP_t *)pkt;

    arp->ar_hrd = htons(ARP_ETHER);
    arp->ar_pro = htons(PROT_IP);
    arp->ar_hln = 6;
    arp->ar_pln = 4;
    arp->ar_op = htons(ARPOP_REQUEST);

//        memcpy (&arp->ar_data[0], NetOurEther, 6);
    pbuf_pttern[6 + 0] = 0xDE;
    pbuf_pttern[6 + 1] = 0xAD;
    pbuf_pttern[6 + 2] = 0xBE;
    pbuf_pttern[6 + 3] = 0xAF;
    pbuf_pttern[6 + 4] = 0xD0;
    pbuf_pttern[6 + 5] = 0xD0;

    NetWriteIP((uchar *)&arp->ar_data[16], NetArpWaitReplyIP);

    pbuf_pttern = (volatile unsigned char *)pbuf;

    print_packet((char *)pbuf_pttern, 64);
    // Walk thru all port to send/recv

    while (diag_eth_devices != diag_eth_current) {
        if (diag_eth_devices == 0) {
            diag_eth_devices = diag_eth_current;
        }

        eth_info = diag_eth_current->priv;

        debug("Send Mac loopback test port %d \n", eth_info->port_id);
        debug("The buffer address for original is 0x%08x \n", pbuf);
        debug("Send Packet Thru Port %d \n", eth_info->port_id);

        if (gmac_send(diag_eth_current, pbuf, 64) != 0) {
            post_printf(POST_VERB_STD,"Error: Send Mac  Loopback Test Failed on Port %d \n",
                   eth_info->port_id);
           
            post_test_status_set(POST_GMAC_LPBK_ID, POST_GMAC_TX_ERROR);

            port_loopback_state |= 0x1 << eth_info->port_id;
            status = -1;
        }

        debug("Receive Packet Thru Port %d \n", eth_info->port_id);

        if (gmac_recv(diag_eth_current) != 0) {
            post_printf(POST_VERB_STD,"Error: Rx Mac Pattern Mis-match port %d \n",
                   eth_info->port_id);

            post_test_status_set(POST_GMAC_LPBK_ID, POST_GMAC_RX_ERROR);

            port_loopback_state |= 0x1 << eth_info->port_id;
            status = -1;
        }

        if (status == 0) {
            post_printf(POST_VERB_DBG,"Pass Mac Loopback Test Port %d \n",
                        eth_info->port_id);
        }

#if 1 // FIXME loop port 0 for now
//        diag_rmi_dump_stats(diag_eth_current);
//break;
#endif // FIXME

        diag_eth_current = diag_eth_current->next;
    }

    free((void*)pbuf);
    return status;
}


int diag_post_gmac_clean(void);

int
diag_post_gmac_clean (void)
{
    gmac_eth_info_t *eth_info;


    debug("%s: \n", __FUNCTION__);

    diag_eth_current = diag_eth_devices = eth_get_dev();


    // free malloc
    index_total_malloc--;

    post_printf(POST_VERB_DBG,"Releasing total malloc's memories %d \n", index_total_malloc);

    while (index_total_malloc) {
        if (malloc_address_data_base[index_total_malloc]) {
            free((void*)malloc_address_data_base[index_total_malloc]);
            post_printf(POST_VERB_DBG,".");
        }
        index_total_malloc--;
    }

    if (malloc_address_data_base[index_total_malloc]) {
        free((void*)malloc_address_data_base[index_total_malloc]);
        post_printf(POST_VERB_DBG,"$.");
    }

    post_printf(POST_VERB_DBG,"Current index malloc %d \n", index_total_malloc);
    diag_eth_current = 0;

    diag_eth_devices = eth_get_dev();
    while (diag_eth_devices != diag_eth_current) {
        if (diag_eth_current == 0) {
            diag_eth_current = diag_eth_devices;
        }
        eth_info = diag_eth_devices->priv;
        post_printf(POST_VERB_DBG,"Reset the Done flag Port=%d \n", eth_info->port_id);
        eth_info->init_done = 0;
        diag_eth_devices = diag_eth_devices->next;
    }

    post_printf(POST_VERB_DBG,"Mac State 0x%08x \n", port_loopback_state);
    if ((port_loopback_state & port_loopback_mask)) {
        post_printf(POST_VERB_STD,"Error: Mac Error Halt 0x%08x\n", port_loopback_state);
    }

    return 1;
}


typedef struct DIAG_MAC_T {
    char *test_name;
    int (*init)(void);
    int (*run)(void);
    int (*clean)(void);
    int  continue_on_error;
} diag_mac_t;


diag_mac_t diag_post_mac = {
    "Mac Loopback Test\n",
    diag_post_gmac_init,
    diag_post_gmac_run,
    diag_post_gmac_clean,
    0
};




int
diag_mac_post_main (void)
{
    int status = 0;

    // 1 is ok

    post_printf(POST_VERB_DBG,"%s: \n", __FUNCTION__);
    post_printf(POST_VERB_STD,"Test Start: %s \n", diag_post_mac.test_name);

    status = diag_post_mac.init();

    if (status == 0) {
        goto diag_mac_post_main_exit;
    }

    status = diag_post_mac.run();

    if (status == 0) {
        goto diag_mac_post_main_exit;
    }




diag_mac_post_main_exit:

    diag_post_mac.clean();
    post_printf(POST_VERB_DBG,"Mac State 0x%08x \n", port_loopback_state);
    if ((port_loopback_state & port_loopback_mask) ) {
        post_printf(POST_VERB_STD,"Error: Mac Error Halt 0x%08x\n", port_loopback_state);
    }
    post_printf(POST_VERB_STD,"%s %s \n",
           diag_post_mac.test_name,
           status == 1 ? "PASS" : "FAILURE");
    post_printf(POST_VERB_STD,"Test End: %s \n", diag_post_mac.test_name);
    return status;
}


int
diag_post_main (void)
{
    int status = 1;

    status = diag_mac_post_main();
    return status;
}


