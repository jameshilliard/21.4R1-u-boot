/*
 * $Id$
 *
 * fx_gmac.c
 *
 * Copyright (c) 2009-2013, Juniper Networks, Inc.
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


/*
 *  implemention of RMI GMAC driver
 */

#include <malloc.h>

#include <rmi/rmigmac.h>
#include <rmi/gmac.h>
#include <rmi/on_chip.h>
#include <rmi/xlr_cpu.h>
#include <rmi/shared_structs.h>
#include <rmi/cpu_ipc.h>
#include <soho/soho_cpld.h>
#include <soho/bcm5396.h>
#include <soho/soho_misc.h>
#include <soho/soho_lcd.h>
#include <soho/tor_mgmt_board.h>
#include <command.h>
#include "fx_common.h"
#include "bcm54xxx.h"
#include "bcm5482S.h"


#define MAX_PACKETS                256
#define DEFAULT_SPILL_ENTRIES      (MAX_PACKETS >> 1)
#define MAX_FRIN_SPILL_ENTRIES     DEFAULT_SPILL_ENTRIES
#define MAX_FROUT_SPILL_ENTRIES    DEFAULT_SPILL_ENTRIES
#define MAX_RX_SPILL_ENTRIES       DEFAULT_SPILL_ENTRIES
#define CACHELINE_SIZE             32
#define AUTONEG
#define MII_BUS_0                   0
#define MII_BUS_1                   1
#define SEG_0                       0
#define SEG_1                       1
#define MGMT_PORT4                  4
#define MGMT_PORT5                  5
#define MGMT_PORT6                  6
#define CB_MGMT_GMAC_MASK           ((1 << 1) | (1 << 4))
#define PA_MGMT_GMAC_MASK           (1 << 4 | 1 << 5 | 1 << 6)
#define SOHO_MGMT_GMAC_MASK         (1 << 4)
#define TOR_MGMT_WA_GMAC_MASK       (1 << 5 | 1 << 6)

void gmac_phy_init(gmac_eth_info_t *this_phy);
int gmac_phy_read(gmac_eth_info_t *this_phy, int reg, uint32_t *data);
int gmac_phy_write(gmac_eth_info_t *this_phy, int reg, uint32_t data);
void gmac_clear_phy_intr(gmac_eth_info_t *this_phy);
uint32_t fx_get_gmac_phy_id(unsigned int mac, int type);

unsigned long fx_get_gmac_mmio_addr(unsigned int gmac);
unsigned long fx_get_gmac_mii_addr(unsigned int mac, int type);
unsigned long fx_get_gmac_pcs_addr(unsigned int mac);
unsigned long fx_get_gmac_serdes_addr(unsigned int mac);
BOOLEAN bcm54xxx_is_type_5482S(void);
static BOOLEAN is_mgmt_sfp_presence_change(int, int);
void bcm5482S_init_as_sfp(gmac_eth_info_t *eth_info);
void bcm5482S_init_in_RJ45mode(gmac_eth_info_t *eth_info);
static BOOLEAN fx_gmac_to_bcm5396(uint32_t gmac_id);

struct boot1_info boot_info;
struct rmi_processor_info rmi_processor;
struct rmi_board_info rmi_board;

int gmac_drain_msgs(struct eth_device*dev);
static int max_packets_tx_count;
uint8_t config_phy_flag = 1;
uint8_t fx_dump_pkt_flag = 0;
uint8_t autoneg_flag = 1;
gmac_eth_info_t *eth_info_array[8];
struct eth_device *eth_dev_array[8];
extern unsigned long gmac_mmio[];
int split_mode = 0;
static uint8_t scan_all_gmac = 0;

static void *
cacheline_aligned_malloc (int size)
{
    void *addr = 0;

    /* addr = smp_malloc(size + (CACHELINE_SIZE << 1)); */
    addr = malloc(size + (CACHELINE_SIZE << 1));

    if (!addr) {
        printf("[%s]: unable to allocate memory!\n", __FUNCTION__);
        return 0;
    }

    /* align the data to the next cache line */
    return (void *)((((unsigned long)addr) + CACHELINE_SIZE) &
                    ~(CACHELINE_SIZE - 1));
}


static void *gmac_frin_spill[8];  // for Free In Descriptor FIFO
static void *gmac_frout_spill[8]; // for Free Out Descriptor FIFO
static void *gmac_rx_spill[8];    // for Class FIFO

void
mac_spill_init (void)
{
    int i, size = 0;

    size = sizeof(struct fr_desc) * MAX_FRIN_SPILL_ENTRIES;
    gmac_frin_spill[0] = gmac_frin_spill[1] = cacheline_aligned_malloc(size);
    gmac_frin_spill[2] = gmac_frin_spill[3] = gmac_frin_spill[0];

    for (i = 0; i < 4; i++) {
        gmac_frout_spill[i] = 0;
    }

    if (chip_processor_id_xls()) {
        gmac_frin_spill[4] = gmac_frin_spill[5] = cacheline_aligned_malloc(size);
        gmac_frin_spill[6] = gmac_frin_spill[7] = gmac_frin_spill[4];
        for (i = 4; i < 8; i++) {
            gmac_frout_spill[i] = 0;
        }
    }

    size = sizeof(union rx_tx_desc) * MAX_RX_SPILL_ENTRIES;
    gmac_rx_spill[0] = gmac_rx_spill[1] = cacheline_aligned_malloc(size);
    gmac_rx_spill[2] = gmac_rx_spill[3] = gmac_rx_spill[0];

    if (chip_processor_id_xls()) {
        gmac_rx_spill[4] = gmac_rx_spill[5] = cacheline_aligned_malloc(size);
        gmac_rx_spill[6] = gmac_rx_spill[7] = gmac_rx_spill[4];
    }

    size = sizeof(struct fr_desc) * MAX_FROUT_SPILL_ENTRIES;
    gmac_frout_spill[0] = gmac_frout_spill[1] = cacheline_aligned_malloc(size);
    gmac_frout_spill[2] = gmac_frout_spill[3] = gmac_frout_spill[0];
    if (chip_processor_id_xls()) {
        gmac_frout_spill[4] = gmac_frout_spill[5] = cacheline_aligned_malloc(
                                  size);
        gmac_frout_spill[6] = gmac_frout_spill[7] = gmac_frout_spill[4];
    }
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
        printf("[%s]: Received from station %d\n", __FUNCTION__, stid);
    }

    if ((*len) == 0) {
        *ctrl = CTRL_REG_FREE;
    } else {
        *ctrl = (msg->msg0 >> 63) & 0x01;
    }

    return pbuf;
}



void
print_packet (const char *pkt_buf, int len)
{
    int i = 0, j = 0;
    unsigned short *p = (unsigned short *)pkt_buf;
    unsigned int nWords = len / 2;

    printf("Dumping %d bytes\n", len);
    for (i = 0; i < nWords;) {
        printf("\t0x%04x:  ", 2 * i);
        for (j = 0; j < 8 ; j++) {
            if (i == nWords) {
                break;
            }
            printf("%04x ", *p);
            p++;  i++;
        }
        printf("\n");
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

    b2_debug("Read MAC Address = %02x:%02x.%02x:%02x.%02x:%02x\n",
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


void
rmi_reset_gmac (phoenix_reg_t *mmio)
{
    volatile uint32_t val;

    /* set the bit to 0 */
    val = phoenix_read_reg(mmio, MAC_CONF1);
    val &= ~(1 << 31);
    phoenix_write_reg(mmio, MAC_CONF1, val);
    udelay(200 * 1000); /* 200ms */
 
    /* If the bit is not 1 after 200ms, manually set it to 1 */
    val = phoenix_read_reg(mmio, MAC_CONF1);
    if (!(val & (1 << 31))) {
        val |= 1 << 31;
        phoenix_write_reg(mmio, MAC_CONF1, val);
        udelay(400 * 1000); /* 400ms */
    }

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
        udelay(3 * 1000); /* 3ms */
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
        udelay(3 * 1000); /* 3ms */
    }

    /* Clear the soft reset bit */
    val = phoenix_read_reg(mmio, RxControl);
    val &= ~0x4;
    phoenix_write_reg(mmio, RxControl, val);
}


static void
gmac_config_speed (struct eth_device *dev)
{
    gmac_eth_info_t *eth_info = (gmac_eth_info_t *)dev->priv;
    phoenix_reg_t *mmio = (phoenix_reg_t *)eth_info->mmio_addr;
    int gmac_id = eth_info->port_id;
    int speed = 0;
    int fdx = 0;
    uint32_t value = 0;
    const char *fdx_str = 0;
    uint32_t data;
    uint32_t speed_fdx = 0;


    if (config_phy_flag) {
        if ((fx_is_cb() && gmac_id != 4) ||
            (fx_is_tor() && gmac_id >=4)) {
            if(bcm54xxx_is_type_5482S() && bcm5482S_sfp_linkup(eth_info)) {
                if (bcm5482S_sfp_getduplexmode(eth_info)) {
                    speed_fdx = MAC_SPEED_1000_FULL_DUPLEX;
                } else {
                    speed_fdx = MAC_SPEED_1000_HALF_DUPLEX;
                }
            } else {
                BCM54XXX_PHY_READ(eth_info, 0x19, &value);
                speed_fdx = (value >> 8) & 0x7;
            }

            switch (speed_fdx) {
            case 0:
                speed = MAC_SPEED_1000;
                break;

            case 1:
                speed = MAC_SPEED_10;
                fdx = FDX_HALF;
                break;

            case 2:
                speed = MAC_SPEED_10;
                fdx = FDX_FULL;
                break;

            case 3:
                speed = MAC_SPEED_100;
                fdx = FDX_HALF;
                break;

            case 4:
                speed = MAC_SPEED_100;
                fdx = FDX_FULL;
                break;

            case 5:
                speed = MAC_SPEED_100;
                fdx = FDX_FULL;
                break;

            case 6:
                speed = MAC_SPEED_1000;
                fdx = FDX_HALF;
                break;

            case 7:
                speed = MAC_SPEED_1000;
                fdx = FDX_FULL;
                break;

            default:
                speed = MAC_SPEED_10;
                break;
            }
            b2_debug("%s[%d]: value=0x%x\n", __func__, __LINE__, value);
        } else if (fx_gmac_to_bcm5396(gmac_id)) {
            bcm5396_read(eth_info, 0x28, &data);
            b2_debug("%s[%d]: status=0x%x\n", __func__, __LINE__, (uint32_t)data);
            if ((data >> 1) & 0x1) {
                eth_info->sgmii_linkup = TRUE;
            }

            if ((data >> 2) & 0x1) {
                fdx = FDX_FULL;
            } else {
                fdx = FDX_HALF;
            }

            switch ((data >> 3) & 0x3) {
                case 0:
                    speed = MAC_SPEED_10;
                    break;

                case 1:
                    speed = MAC_SPEED_100;
                    break;

                default:
                    speed = MAC_SPEED_1000;
                    break;
            }

            fdx = FDX_FULL;
            speed = MAC_SPEED_1000;
        } else if ((fx_is_pa() || fx_is_wa()) && 
                   (gmac_id == 2 || gmac_id == 3)) {
            fdx = FDX_FULL;
            speed = MAC_SPEED_1000;
        }
    }

    if (fx_is_pa()) {
        /* connect to accelerator FPGA */
        if (gmac_id == 0 || gmac_id == 1) {
            speed = MAC_SPEED_1000;
            fdx = FDX_FULL;
        }
    }

    if (fx_is_pa()) {
        /* connect to accelerator FPGA */
        if (gmac_id == 0 || gmac_id == 1) {
            speed = MAC_SPEED_1000;
            fdx = FDX_FULL;
        }
    }

    fdx_str = fdx == FDX_FULL ? "full duplex" : "half duplex";

    if ((speed != eth_info->speed) || (fdx != eth_info->fdx)) {
        eth_info->speed = speed;
        eth_info->fdx = fdx;
        /* configure the GMAC Registers */
        if (eth_info->speed == MAC_SPEED_10) {
            /* nibble mode */
            phoenix_write_reg(mmio, MAC_CONF2, (0x7136 | (eth_info->fdx & 0x01)));
            /* 2.5MHz */
            phoenix_write_reg(mmio, CoreControl, 0x02);
            phoenix_write_reg(mmio, SGMII_SPEED, SGMII_SPEED_10);
            b2_debug("configuring %s in nibble mode @2.5MHz (10Mbps):%s mode\n",
                  dev->name, fdx_str);
        } else if (eth_info->speed == MAC_SPEED_100) {
            /* nibble mode */
            phoenix_write_reg(mmio, MAC_CONF2, (0x7136 | (eth_info->fdx & 0x01)));
            /* 25MHz */
            phoenix_write_reg(mmio, CoreControl, 0x01);
            phoenix_write_reg(mmio, SGMII_SPEED, SGMII_SPEED_100);
            b2_debug("configuring %s in nibble mode @25MHz (100Mbps):%s mode\n",
                  dev->name, fdx_str);
        } else {
            /* byte mode */
            if (eth_info->speed != MAC_SPEED_1000) {
                /* unknown speed */
                printf("Unknown mac speed, defaulting to GigE\n");
            }
            phoenix_write_reg(mmio, MAC_CONF2, (0x7236 | (eth_info->fdx & 0x01)));
            phoenix_write_reg(mmio, CoreControl, 0x00);
            phoenix_write_reg(mmio, SGMII_SPEED, SGMII_SPEED_1000);
            b2_debug("configuring %s in byte mode @125MHz (1000Mbps):%s mode\n",
                  dev->name, fdx_str);
        }
    }
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
        } else {
            phoenix_write_reg(mmio, G_TX0_BUCKET_SIZE + (gmac_id & 0x3),
                              xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC1_TX0 +
                                                      (gmac_id & 0x3)]);
            phoenix_write_reg(mmio, G_RFR0_BUCKET_SIZE,
                              xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC1_FR_0]);
            phoenix_write_reg(mmio, G_RFR1_BUCKET_SIZE,
                              xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC1_FR_1]);
        }
    } else {
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


static BOOLEAN
fx_gmac_to_bcm5396(uint32_t gmac_id)
{
    if (fx_is_soho()) {
        if ((gmac_id >= 0 && gmac_id <= 6) && (gmac_id != 4)) {
            return TRUE;
        }
    }
    
    return FALSE;
}

static void
mac_receiver_enable (struct eth_device *dev)
{
    gmac_eth_info_t *eth_info = (gmac_eth_info_t *)dev->priv;
    phoenix_reg_t *mmio = (phoenix_reg_t *)eth_info->mmio_addr;
    uint32_t rx_control = 1;

    phoenix_write_reg(mmio, MAC_CONF1, 0x35);

    if (chip_processor_id_xls()) {
        rx_control = 1;
    }
    /* Enable the Receiver */
    phoenix_write_reg(mmio, RxControl, rx_control);
}


static int
mac_open (struct eth_device *ndev)
{
    gmac_eth_info_t *eth_info = (gmac_eth_info_t *)ndev->priv;
    int gmac_id = eth_info->port_id;
    static uint8_t mac_0_open_done = 0;
    static uint8_t mac_1_open_done = 0;


    int i = 0, stid = 0;
    uchar *pbuf;
    struct msgrng_msg msg;

    if (eth_info->port_id < 4 && mac_0_open_done) {
        return 1;
    }

    if (eth_info->port_id > 3 && eth_info->port_id < 8 && mac_1_open_done) {
        return 1;
    }

    /* Send free descs to the MAC */

#ifdef UBOOT_DELETE_ME
    for (i = 0; i < PKTBUFSRX; i++) {
        pbuf = (txbuffer_t *)NetRxPackets[i];
        eth_info->make_desc_rfr(&msg, gmac_id, virt_to_phys(pbuf), &stid);

#ifdef DUMP_MESSAGES
        if (global_b2_debug) {
            printf
            ("cpu_%d: Sending rfr_%d @ (%lx) to station id %d\n",
             processor_id(), i, virt_to_phys(pbuf), stid);
        }
#endif

        message_send_block(1, MSGRNG_CODE_MAC, stid, &msg);
    }
#endif

    for (i = 0; i < 128; i++) {
        pbuf = (uchar *)cacheline_aligned_malloc(PKTSIZE_ALIGN);
        if (!pbuf) {
            return 0;
        }
        eth_info->make_desc_rfr(&msg, gmac_id, virt_to_phys(pbuf), &stid);
        message_send_block(1, MSGRNG_CODE_MAC, stid, &msg);
    }

    if (eth_info->port_id < 4) {
        mac_0_open_done = 1;
    } else {
        mac_1_open_done = 1;
    }

    return 1;
}


static void
fx_sgmii_ref_clock (void)
{
    uint8_t i;
    uint32_t val;  

    phoenix_reg_t *gmac_mmio;


    phoenix_reg_t *mmio = (phoenix_reg_t *)
                          (DEFAULT_RMI_PROCESSOR_IO_BASE +
                           PHOENIX_IO_GPIO_OFFSET);
    if (fx_is_cb()) {
        for (i = 0; i < RMI_GMAC_TOTAL_PORTS; i++) {
            gmac_mmio = fx_get_gmac_mmio_addr(i);
            val = gmac_mmio[0];
            if (!(val & (1 << 31))) {
                b2_debug("%s: GMAC %d is out of reset\n", __func__, i);
                val |= 1 << 31;
                b2_debug("%s: GMAC %d is put in reset\n", __func__, i);
                gmac_mmio[0] = val;
            }
        }
    }

    val = mmio[0x10];
    val &= ~0x1FF;
    val |= 0xA5;
    mmio[0x10] = val;

#define SGMII_CLOCK_500MHZ  0x84
#define SGMII_CLOCK_1000MHZ 0x85
#define SGMII_CLOCK_1250MHZ 0xA5
#define SGMII_CLOCK_3050MHZ 0xAE

    val = mmio[0x21];
    val &= ~0x1FF;
    val |= SGMII_CLOCK_1250MHZ;
    mmio[0x21] = val;
    udelay(1000 * 10); /* 10ms */
    b2_debug("%s: change sgmii clock to 1250Mhz val=0x%x\n", __func__, val);
}


static void
fx_set_bcm_chip_type (gmac_eth_info_t *eth_info)
{
    static uint8_t set_chip_type_done = 0;

    if (fx_is_cb()) {
        eth_info->phy_type = BCM54640;
    } else if (fx_is_soho()) {
        eth_info->phy_type = BCM5482;
    } else if (fx_is_pa() || fx_is_wa()) {
        if (fx_is_qfx3500_sfp_mgmt_board()) {
            if (GMAC_PORT_IS_TOR_MGMT_BRD_SFP_PORT(eth_info->port_id)) {
                eth_info->phy_type = BCM5482S;
            } else {
                eth_info->phy_type = BCM5482;
            }
        } else {
            eth_info->phy_type = BCM5482;
        }
    }
}

static BOOLEAN 
fx_chk_bcm54xxx_access (gmac_eth_info_t *eth_info)
{
    if (bcm54xxx_access_is_valid(eth_info) == TRUE) {
        b2_debug("%s[%d]: Access to BCM PHY success\n", __func__, __LINE__);
        return TRUE;
    } else {
        printf("%s[%d]: Access to BCM PHY failed\n", __func__, __LINE__);
        return FALSE;
    }
}

static BOOLEAN
fx_chk_bcm5396_access(gmac_eth_info_t *eth_info)
{
    return TRUE;
}

static BOOLEAN 
fx_mdio_access_is_valid (gmac_eth_info_t *eth_info)
{
    uint32_t gmac_id = eth_info->port_id;

    if (fx_is_cb()) {
        if (gmac_id == 1 || gmac_id == 5 || gmac_id == 4) {
            return (fx_chk_bcm54xxx_access(eth_info));
        }
    }

    if (fx_is_tor()) {
        if (gmac_id >= 0 && gmac_id <= 3) {
            return (fx_chk_bcm5396_access(eth_info));
        }

        if (gmac_id >= 4 && gmac_id <= 7) {
            return (fx_chk_bcm54xxx_access(eth_info));
        }
    }

    return FALSE;
}


static BOOLEAN
fx_gmac_has_copper_link (uint32_t gmac_id) 
{
    if (fx_is_cb()) {
        if (gmac_id == 1 || gmac_id == 5) {
            return TRUE;
        }
    }

    if (fx_is_tor()) {
        if (gmac_id >= 4 && gmac_id <= 6) {
            return TRUE;
        } 
    }

    return FALSE;
}

static BOOLEAN
fx_gmac_to_bcm54xxx(uint32_t gmac_id) 
{
    if (fx_is_cb()) {
        if (gmac_id == 1 || gmac_id == 5 || gmac_id == 4) {
            return TRUE;
        }
    }

    if (fx_is_tor()) {
        if (gmac_id >= 4 && gmac_id <= 7) {
            return TRUE;
        } 
    }

    return FALSE;
}

static int 
fx_gmac_is_mgmt_port (uint32_t gmac_id)
{
    char *str_start, *str_end;
    static uint32_t mgmt_gmac_mask = 0x0;

    str_start = getenv("mgmt_gmac_mask");

    if (str_start != NULL) {
        mgmt_gmac_mask = simple_strtoul(str_start, &str_end, 16); 
    } 

    if (fx_is_cb()) {
        if (mgmt_gmac_mask == 0) {
            mgmt_gmac_mask = CB_MGMT_GMAC_MASK;
    }

    } else if (fx_is_pa()) {
        if (mgmt_gmac_mask  == 0) {
           mgmt_gmac_mask = PA_MGMT_GMAC_MASK;
        }
    } else if (fx_is_wa()) {
        if (mgmt_gmac_mask == 0) {
            mgmt_gmac_mask = TOR_MGMT_WA_GMAC_MASK;
        }
    } else if (fx_is_soho()) {
        if (mgmt_gmac_mask  == 0) {
            mgmt_gmac_mask = SOHO_MGMT_GMAC_MASK; 
        }
    } else {
        printf("%s: Unknow board type\n", __func__);
        return -1;
    }

    if ((1 << gmac_id) & mgmt_gmac_mask) {
        return 1;
    } else {
        return 0;
    }
}

static int
fx_gmac_re_init (struct eth_device *dev)
{
    uint32_t i;
    char *str_start, *str_end;
    uint32_t force_gmac = 0;
    gmac_eth_info_t *eth_info = (gmac_eth_info_t *)dev->priv;
    uint32_t gmac_id = eth_info->port_id;

    for (i = 0; i < 8; i++) { 
        gmac_drain_msgs(dev);
    }
    mac_receiver_enable(dev);

    str_start = getenv("force_gmac");

    if (str_start != NULL) {
        force_gmac = simple_strtoul(str_start, &str_end, 16); 
        b2_debug("str_start=%s, force_gmac=%d\n", str_start, force_gmac); 

        if (force_gmac != gmac_id) { 
            return 0;
        } else { 
            return 1;
        }
    }

    b2_debug("[%s]:Init previously done.\n", __FUNCTION__);

    if (!fx_gmac_is_mgmt_port(gmac_id)) {
        return 0;
    }

    if (fx_gmac_has_copper_link(gmac_id) == TRUE) { 
        if (eth_info->copper_linkup == TRUE) {
            b2_debug("Copper and SGMII linkup\n");
            return 1;
        }
        if (eth_info->sfp_linkup == TRUE) {
            b2_debug("SFP and SGMII linkup\n");
            return 1;
        }

        return 0;
    }

    return 0;
}

static int
fx_gmac_set_led (gmac_eth_info_t *eth_info)
{
    bcm54xxx_selector_t selector;

    if (fx_is_cb()) {
        selector.led_0 = 0x3;

        if (eth_info->speed == MAC_SPEED_1000) {
            selector.led_1 = BCM54XXX_LED_FORCE_OFF;
        } else {
            selector.led_1 = BCM54XXX_LED_LINK_SPD_2;
        }

        selector.led_2 = BCM54XXX_LED_LINK_SPD_1;
        selector.led_3 = BCM54XXX_LED_LINK_SPD_1;
    }

    if (fx_is_tor()) {
        if ((eth_info->copper_linkup || eth_info->sfp_linkup)
             && eth_info->sgmii_linkup) {
            selector.led_0 = BCM54XXX_LED_ACTIVITY;
            selector.led_1 = BCM54XXX_LED_FORCE_OFF;
            selector.led_2 = BCM54XXX_LED_FORCE_OFF;
            selector.led_3 = BCM54XXX_LED_FORCE_OFF;
        } else {
            selector.led_0 = BCM54XXX_LED_FORCE_OFF;
            selector.led_1 = BCM54XXX_LED_FORCE_OFF;
            selector.led_2 = BCM54XXX_LED_FORCE_OFF;
            selector.led_3 = BCM54XXX_LED_FORCE_OFF;
        }
    }

    return (bcm54xxx_set_led(eth_info, &selector));
}

static int
fx_cb_pll_fix (struct gmac_eth_info_t *eth_info)
{
    uint32_t i;
    uint32_t value;
    phoenix_reg_t *mmio = (phoenix_reg_t *)
                          (DEFAULT_RMI_PROCESSOR_IO_BASE +
                           PHOENIX_IO_GPIO_OFFSET);


    phoenix_reg_t *gmac_mmio = fx_get_gmac_mmio_addr(4);

    bcm54640_goto_pri_serdes_mode(eth_info);
    for (i = 0; i < 10; i++) {
        BCM54XXX_PHY_WRITE(eth_info, 0x1C, 0x1A << 10);
        BCM54XXX_PHY_READ(eth_info, 0x1C, &value);
        if (value & (1 << 8)) {
            printf("%s: detect RUDI error: %x retry %d\n", 
                    __func__, value, i);

            value = gmac_mmio[0];
            if (!(value & (1 << 31))) {
                b2_debug("%s: GMAC %d is out of reset\n", __func__, i);
                value |= 1 << 31;
                b2_debug("%s: GMAC %d is put in reset\n", __func__, i);
                gmac_mmio[0] = value;
            }

            value = mmio[0x21];
            value &= ~0x1FF;
            value |= 0x04;
            mmio[0x21] = value;
            udelay(1000 * 10);

            value = mmio[0x21];
            value &= ~0x1FF;
            value |= 0xA5;
            mmio[0x21] = value;
            udelay(1000 * 10);
        } else {
            break;
        }
    }
    bcm54640_goto_sec_serdes_mode(eth_info);
	return 0;
}

static int
gmac_init (struct eth_device*dev, bd_t *bd)
{
    gmac_eth_info_t *eth_info = (gmac_eth_info_t *)dev->priv;
    phoenix_reg_t *mmio = (phoenix_reg_t *)eth_info->mmio_addr;
    int gmac_id = eth_info->port_id;
    uint32_t linkstatus = 0, i;
    static BOOLEAN reset_q0_done = FALSE;
    static BOOLEAN reset_q1_done = FALSE;
    static BOOLEAN slow_clock_done = FALSE;
    uint32_t data;
    uint64_t mac_addr;
    uint32_t cc_reg;
    uint32_t size;

    if (!scan_all_gmac && !fx_gmac_is_mgmt_port(gmac_id)) {
        return 0;
    }

    b2_debug("\nGMAC=%d: \n", gmac_id);

    if (gmac_id < 4) {
        if (reset_q0_done == FALSE) {
            b2_debug("%s[%d]: reset gmac quad 0\n", __func__, __LINE__);
            rmi_reset_gmac(mmio);
            reset_q0_done = TRUE;
        }
    } else if (reset_q1_done == FALSE) {
        b2_debug("%s[%d]: reset gmac quad 1\n", __func__, __LINE__);
        rmi_reset_gmac(mmio);
        reset_q1_done = TRUE;
    }

    fx_set_bcm_chip_type(eth_info);

    if (fx_is_tor()) {
        if (fx_gmac_to_bcm5396(gmac_id)) {
            soho_mdio_set_bcm5396();
        } else {
            soho_mdio_set_bcm5482();
        }
    }

    if (slow_clock_done == FALSE) {
        /* lower the mgmt clock freq to minimum possible freq before
        * any MDIO operation */
        phoenix_write_reg((phoenix_reg_t *)eth_info->mii_addr, MIIM_CONF, 0x7);
        phoenix_write_reg((phoenix_reg_t *)eth_info->mii_addr, MAC_CONF1, 0x30);
        gmac_init_serdes_ctrl_regs(gmac_id);

        slow_clock_done = TRUE;
    }

    if (eth_info->phy_reset == FALSE) {
        if (fx_gmac_to_bcm54xxx(gmac_id)) {
            bcm54xxx_reset(eth_info);
            eth_info->phy_reset = TRUE;
        }
    }

    if (!bcm54xxx_all_spd_enable(eth_info)) {
        return 0;
    }

    if (fx_gmac_to_bcm5396(gmac_id)) {
        bcm5396_reset(gmac_id);
    }

    if (eth_info->init_done) {
        return fx_gmac_re_init(dev);
    }

    if (fx_is_sfp_port(gmac_id)) {
        fx_init_gmac_port(gmac_id);
    }

    mac_addr = get_macaddr();
    boot_info.mac_addr = mac_addr;

    cc_reg = 0;

    mac_change_hwaddr(dev, mac_addr + gmac_id);

    mac_init_msgring(dev);

    if (gmac_id == 0) {
        boot_info.mac_addr = mac_addr;
    }

    if (config_phy_flag) {
        if (fx_gmac_has_copper_link(gmac_id) == TRUE) {
            if (bcm54xxx_copper_autoneg(eth_info) == 0) {
                //  return 0;
            }
        }

        if (fx_gmac_to_bcm54xxx(gmac_id)) {
            if (autoneg_flag) {
                bcm54xxx_sgmii_autoneg_ctrl(eth_info, TRUE);
            } else {
                bcm54xxx_sgmii_autoneg_ctrl(eth_info, FALSE);
            }
        }

        if (fx_gmac_to_bcm5396(gmac_id)) {
            bcm5396_read(eth_info, 0x0, &data);
            data |= ((1 << 6) | (1 << 8));
            bcm5396_write(eth_info, 0x0, data);
        }
#if 0
        if (fx_gmac_to_bcm5396(gmac_id)) {
           bcm5396_autoneg(gmac_id);
        }
#endif
    }

    gmac_phy_init(eth_info);

    /* The following is redundant for QFX3500 SFP mgmt board,
       and this configuration is causing link down for QFX3500
       SFP mgmt board. */
    if (!fx_is_qfx3500_sfp_mgmt_board()) {
        is_mgmt_sfp_presence_change(gmac_id, 0);
    }

    gmac_config_speed(dev);
    phoenix_write_reg(mmio, IntReg, 0xffffffff);

    /* configure the GMAC_GLUE Registers */
    phoenix_write_reg(mmio, DmaCr0, 0xffffffff);
    phoenix_write_reg(mmio, DmaCr2, 0xffffffff);
    phoenix_write_reg(mmio, DmaCr3, 0xffffffff);

    /* allocate the spill areas */
    size = sizeof(struct fr_desc) * MAX_FRIN_SPILL_ENTRIES;
    if (!(eth_info->frin_spill_packets = gmac_frin_spill[gmac_id])) {
        size = 0;
    }

    phoenix_write_reg(mmio, RegFrInSpillMemStart0,      // 0x204
                       (virt_to_phys(eth_info->frin_spill_packets) >> 5));
    phoenix_write_reg(mmio, RegFrInSpillMemSize, size); // 0x206

    size = sizeof(struct fr_desc) * MAX_FROUT_SPILL_ENTRIES;
    if (!(eth_info->frout_spill_packets = gmac_frout_spill[gmac_id])) {
        size = 0;
    }

    phoenix_write_reg(mmio, FrOutSpillMemStart0,
                      (virt_to_phys(eth_info->frout_spill_packets) >> 5));

    /* Since all addresses are in KSEG0, no need to configure
     * FrOutSpillMemStart1 */
    phoenix_write_reg(mmio, FrOutSpillMemSize, size);

    size = sizeof(union rx_tx_desc) * MAX_RX_SPILL_ENTRIES;
    if (!(eth_info->rx_spill_packets = gmac_rx_spill[gmac_id])) {
        size = 0;
    }

    phoenix_write_reg(mmio, Class0SpillMemStart0,
                          (virt_to_phys(eth_info->rx_spill_packets) >> 5));
    phoenix_write_reg(mmio, Class0SpillMemSize, size);

    phoenix_write_reg(mmio, TxControl, ((1 << 14) | 512));
    /* Enable transmit of 512 byte packets */
    phoenix_write_reg(mmio, DescPackCtrl,
                      ((MAC_BYTE_OFFSET << 17) | ((2 - 1) << 14) | 1536) );
    /* enable mac ip stats */
    phoenix_write_reg(mmio, StatCtrl, 0x04);
    phoenix_write_reg(mmio, L2AllocCtrl, 2);
    /* configure the classifier - use port based classification by default */
    phoenix_write_reg(mmio, ParserConfig, 0x00000008);

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
        } else {
            phoenix_write_reg(mmio, G_TX0_BUCKET_SIZE + (gmac_id & 0x3),
                              xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC1_TX0 +
                                                      (gmac_id & 0x3)]);
            phoenix_write_reg(mmio, G_RFR0_BUCKET_SIZE,
                              xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC1_FR_0]);
            phoenix_write_reg(mmio, G_RFR1_BUCKET_SIZE,
                              xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC1_FR_1]);
        }
    } else {
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
        } else {
            phoenix_write_reg(mmio, (cc_reg + i),
                              (cc_table_gmac.counters[i >> 3][i & 0x07]));
        }
    }

    /* Enable the MAC but disable TX/RX */
    phoenix_write_reg(mmio, MAC_CONF1, 0x30);

    if (config_phy_flag) {
        if (fx_gmac_has_copper_link(gmac_id) == TRUE) {

            if (bcm54xxx_lineside_linkup(eth_info) == FALSE) {
                eth_info->copper_linkup  = FALSE;
                b2_debug("phy_addr=0x%x copper link is down\n", eth_info->phy_id);
            } else {
                eth_info->copper_linkup  = TRUE;
                b2_debug("phy_addr=0x%x copper link is UP\n", eth_info->phy_id);
            }
        }

        if (fx_gmac_to_bcm54xxx(gmac_id) == TRUE) { 
            if (bcm54xxx_sgmii_linkup(eth_info) == FALSE) {
                eth_info->sgmii_linkup = FALSE;
                b2_debug("phy_addr=0x%x sgmii link is down\n", eth_info->phy_id);
            } else {
                eth_info->sgmii_linkup = TRUE;
                b2_debug("phy_addr=0x%x sgmii link is UP\n",
                          eth_info->phy_id, linkstatus);
            }
        }

        if (fx_is_tor()) {
            if (bcm54xxx_is_type_5482S()) {
                if (bcm5482S_sfp_linkup(eth_info) == TRUE){
                    eth_info->sfp_linkup  = TRUE;
                    b2_debug("phy_addr=0x%x SFP link is UP\n", eth_info->phy_id);
                } else {
                    eth_info->sfp_linkup  = FALSE;
                    b2_debug("phy_addr=0x%x SFP link is FALSE\n", eth_info->phy_id);
                }
             }
        }

        if (fx_gmac_to_bcm5396(gmac_id) == TRUE) {
            /* we keep this part of the code for future
             * SOHO P1 board that has bcm5396 */
            if (bcm5396_sgmii_linkup(gmac_id) == TRUE) {
                eth_info->sgmii_linkup = TRUE;
            } else {
                eth_info->sgmii_linkup = FALSE;
            }
        }
    }

    eth_info->init_done = 1;

    if (fx_gmac_has_copper_link(gmac_id) == TRUE) {
        if (fx_is_cb()) {
            /* bcm 54640 doens't have correct sgmii link status */
            if ((eth_info->copper_linkup == FALSE))  {
                return 0;
            } else {
                if (!fx_gmac_set_led(eth_info)) {
                    printf("%s: set led failed\n", __func__);
                }
            }
        }

        if (fx_is_tor()) {
            if (((eth_info->copper_linkup == FALSE) && 
                (eth_info->sfp_linkup == FALSE)) || 
                (eth_info->sgmii_linkup == FALSE )) {
                b2_debug("%s[%d]: setting gmac %d PHY status led\n", 
                        __func__, __LINE__, gmac_id);
                soho_cpld_select_phy_led(gmac_id, 0x00);
                soho_cpld_set_phy_led(gmac_id, SOHO_PHY_LED_OFF);
                return 0;
            } else {
                if (eth_info->sfp_linkup == TRUE) {
                     soho_cpld_select_phy_led(gmac_id,0x01);
                } else {
                     soho_cpld_select_phy_led(gmac_id,0x00);
                }
                if (eth_info->speed == MAC_SPEED_1000) { 
                     soho_cpld_set_phy_led(gmac_id, SOHO_PHY_LED_GREEN);
                } else {
                     soho_cpld_set_phy_led(gmac_id, SOHO_PHY_LED_AMBER);
                }
            }
            if (!fx_gmac_set_led(eth_info)) {
                printf("%s: set led failed\n", __func__);
            }
        }
    }

    if (fx_is_cb && gmac_id == 4) {
        fx_cb_pll_fix(eth_info);
    }

    if (fx_gmac_is_mgmt_port(gmac_id))  {
        return (mac_open(dev));
    } else {
        mac_open(dev);
        return 0;
    }
} /* ends ... gmac_init() */


static int
gmac_send (struct eth_device*dev, volatile void *pbuf, int len)
{
    gmac_eth_info_t *eth_info = (gmac_eth_info_t *)dev->priv;
    int gmac_id = eth_info->port_id;
    int stid = 0, size = 0;
    struct msgrng_msg msg;

    if (max_packets_tx_count++ > MAX_PACKETS) {
        printf("Reached MAX_PACKETS limit \n");
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

    eth_info->make_desc_tx(&msg,
                           gmac_id,
                           virt_to_phys(pbuf),
                           len,
                           &stid,
                           &size);

    /* Send the packet to MAC */
#ifdef DUMP_MESSAGES
    if (global_b2_debug) {
        printf("[%s]: Sending tx packet to stid %d size %d...\n",
               __FUNCTION__, stid, size);
    }
#endif

    if (fx_dump_pkt_flag) {
        printf("\nSend:\n");
        print_packet(pbuf, len);
        printf("\n\n");
    }
    message_send_block(size, MSGRNG_CODE_MAC, stid, &msg);

#ifdef DUMP_MESSAGES
    if (global_b2_debug) {
        printf("[%s]: ...done\n", __FUNCTION__);
    }
#endif

    /* Let the mac keep the free descriptor */
    return 0;
}


int
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

    /* Wait indefinitely for a msg */
    for (;;) {
        /* printf("cpu_%d: Waiting for a message from %s, netdev=%p\n", id, dev->name, dev); */

        udelay(5000);
        counter = read_32bit_cp0_register(COP_0_COUNT);
        if (message_receive(bucket, &size, &code, &stid, &msg)) {
            break;
        }

        /* Msg received */
        pbuf = (uchar *)eth_info->make_desc_rx(stid, &msg, &ctrl, &port, &len);

        if (ctrl == CTRL_REG_FREE) {
            /* Free back descriptor from mac */
#ifdef DUMP_MESSAGES
            if (global_b2_debug) {
                printf("[%s]: received free desc from mac, %llx\n",
                       __FUNCTION__, msg.msg0);
            }
#endif
            /* free(pbuf); */
            continue;
        }


        /* free the descriptor */
        eth_info->make_desc_rfr(&rxmsg, gmac_id, virt_to_phys(pbuf), &stid);
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
    uchar *pbuf = 0, *lbuf = 0;
    int ctrl = 0;
    uint32_t counter = 0;

    int id = processor_id();
    int bucket = id & 0x03;
    int port = 0;
    int len = 0;

    /* check for changes in mdio of the gmacs */
    {
        phoenix_reg_t *mmio =  (phoenix_reg_t *)eth_info->mii_addr;

        /* Assuming GMAC0 is in RMGII mode */
        if (chip_processor_id_xls()) {
            mmio = (phoenix_reg_t *)eth_info->mii_addr;
        }

        if (phoenix_read_reg(mmio, IntReg) & 0x02) {
            if (chip_processor_id_xls()) {
                gmac_clear_phy_intr(eth_info);
                gmac_phy_init(eth_info);
            } else {
                gmac_clear_phy_intr(eth_info);
            }
            phoenix_write_reg(mmio, IntReg, 0xffffffff);
            gmac_config_speed(dev);
        }
    }

    /* Wait indefinitely for a msg */
    for (;;) {
        /* printf("cpu_%d: Waiting for a message from %s, netdev=%p\n", id, dev->name, dev); */

        counter = read_32bit_cp0_register(COP_0_COUNT);
        if (message_receive(bucket, &size, &code, &stid, &msg)) {
            break;
        }

        /* Msg received */
        pbuf = (uchar *)eth_info->make_desc_rx(stid, &msg, &ctrl, &port, &len);

        if (ctrl == CTRL_REG_FREE) {
            /* Free back descriptor from mac */
#ifdef DUMP_MESSAGES
            if (global_b2_debug) {
                printf("[%s]: received free desc from mac, %llx\n",
                       __FUNCTION__, msg.msg0);
            }
#endif
            /* free(pbuf); */
            max_packets_tx_count--;
            break;
        }

        lbuf = malloc(1536);
        if (!lbuf) {
            printf("** [%s]: unable to malloc.\n", __FUNCTION__);
        }
        if (fx_dump_pkt_flag) {
            printf("\nRecv:\n");
            print_packet(pbuf, len);
            printf("\n\n");
        }
        memcpy((uchar *)lbuf, (uchar *)pbuf, len);

        /* pass the packet up to the protocol layers */
        len = len - 2 - 4;

#ifdef UBOOT_DELETE_ME
        if (gmac_id != 0) {
            printf("received pkt addr %p len %d\n", lbuf, len);
            for (i = 0; i < 64; i++) {
                printf("0x%02x ", (unsigned char)(lbuf[i]));
            }
        }
#endif
        NetReceive( (uchar *)lbuf + 2, len);
        free(lbuf);

        /* free the descriptor */
        eth_info->make_desc_rfr(&rxmsg, gmac_id, virt_to_phys(pbuf), &stid);
        message_send_block(1, MSGRNG_CODE_MAC, stid, &rxmsg);
    }
    return 0;
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


extern u_int8_t *cb_cpld_base;

int
rmigmac_enet_initialize (bd_t *bis)
{
    gmac_eth_info_t *eth_info;
    struct eth_device*dev;
    char *str_start;

    max_packets_tx_count = 0;
    unsigned int num, i, gmac_list;

    if (fx_is_cb()) {
        fx_cpld_write(cb_cpld_base, 0x51, 0);
        udelay(10000); /* 10ms */
        b2_debug("Holding Phy in reset\n");
    }

    fx_sgmii_ref_clock();

    if (fx_is_cb()) {
        fx_cpld_write(cb_cpld_base, 0x51, ((0x1 << 3) | 0x1));
        udelay(10000); /* 10ms */
        b2_debug("Taking Phy out of reset\n");
    }

    struct rmi_board_info *rmib = &rmi_board;

    num = rmib->gmac_num;
    gmac_list = rmib->gmac_list;

    str_start = getenv("scan_all_gmac");

    if (str_start != NULL) { 
        scan_all_gmac = 1;
    }

    i = 0;
    while (i < num) {
        if (!((gmac_list >> i) & 0x01)) {
            continue;
        }

        if ((eth_info = (gmac_eth_info_t *)malloc(sizeof(gmac_eth_info_t)))
            == NULL) {
            printf("initialize: malloc fail\n");
            return 0;
        }

        eth_info_array[i] = eth_info;
        eth_info->init_done = 0;
        eth_info->port_id = i;

        eth_info->phy_id = fx_get_gmac_phy_id(i, eth_info->phy_mode);
        eth_info->mmio_addr = fx_get_gmac_mmio_addr(i);
        eth_info->mii_addr = fx_get_gmac_mii_addr(i, eth_info->phy_mode);

        eth_info->pcs_addr = fx_get_gmac_pcs_addr(i);
        eth_info->serdes_addr = fx_get_gmac_serdes_addr(i);

        if ((dev = (struct eth_device*)malloc(sizeof(*dev))) == NULL) {
            printf("initialize: malloc failed\n");
            return 0;
        }
        memset(dev, 0, sizeof *dev);
        sprintf(dev->name, "gmac%d", i);

        eth_dev_array[i] =  dev;
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

    mac_spill_init();

    return 1;
}

static void
fx_dump_pkt (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    if (argc == 2) {
        fx_dump_pkt_flag = simple_strtoul(argv[1], NULL, 16);
    }
    printf("%s: fx_dump_pkt_flag = 0x%x\n", __func__, fx_dump_pkt_flag);
}

void 
fx_gmac_read_phy (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    gmac_eth_info_t *eth_info;
    uint32_t data, gmac_port, reg_idx;

    if (argc < 3) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    if (argc == 3) {
        gmac_port = simple_strtoul(argv[1], NULL, 16);
        reg_idx = simple_strtoul(argv[2], NULL, 16);
    }

    eth_info = eth_info_array[gmac_port];

    if (fx_is_tor()) {
        if (fx_gmac_to_bcm5396(gmac_port)) {
            bcm5396_read(eth_info, reg_idx, &data);
        } else {
            soho_mdio_set_bcm5482();
            BCM54XXX_PHY_READ(eth_info, reg_idx, &data);
        }
    } else if (fx_is_cb()) {
        BCM54XXX_PHY_READ(eth_info, reg_idx, &data);
    }

    b2_debug("\ngmac_port=0x%x reg_idx=0x%x data=0x%x\n\n", 
           gmac_port, reg_idx, data & 0xFFFF);
}

int
fx_gmac_write_phy (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    gmac_eth_info_t *eth_info;
    uint32_t data, gmac_port, reg_idx;

    if (argc < 3) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return 1;
    }

    if (argc == 4) {
        gmac_port = simple_strtoul(argv[1], NULL, 16);
        reg_idx = simple_strtoul(argv[2], NULL, 16);
        data = simple_strtoul(argv[3], NULL, 16);
    }

    if (gmac_port > 7) {
        printf("gmac_port %d is invalid\n", gmac_port);
        return -1;
    }


    eth_info = eth_info_array[gmac_port];
    if (fx_is_tor()) {
        if (fx_gmac_to_bcm5396(gmac_port)) {
            bcm5396_write(eth_info, reg_idx, data);
        } else {
            soho_mdio_set_bcm5482();
            BCM54XXX_PHY_WRITE(eth_info, reg_idx, data & 0xFFFF);
        }
    } else if (fx_is_cb()) {
        BCM54XXX_PHY_WRITE(eth_info, reg_idx, data);
    }
    b2_debug("write value 0x%x to gmac 0x%x in reg_idx 0x%x\n", data & 0xFFFF, gmac_port, reg_idx);
    return 0;
}

char data_buf[64];

int
fx_gmac_send (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    struct eth_device *dev;
    uint32_t i,gmac_port;
    uint32_t packet_cnt = 0;
    uint16_t l3_proto = 0x0800; /* ipv4 */


    if (argc < 3) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return -1;
    }

    if (argc == 3) {
        gmac_port = simple_strtoul(argv[1], NULL, 16);
        packet_cnt = simple_strtoul(argv[2], NULL, 16);
    }

    dev = eth_dev_array[gmac_port];

    for (i = 0; i < 6; i++) {
        data_buf[i] = 0xFF;
    }

    for (i = 6; i < 12; i++) {
        data_buf[i] = dev->enetaddr[i-6];
    }

    memcpy((char *)&(data_buf[12]), &l3_proto, sizeof(uint16_t));

    for (i = 14; i < sizeof (data_buf); i++) {
        data_buf[i] = i % 0xFF;
    }

    for (i = 0; i < packet_cnt; i++) {
        gmac_send (dev, (void *)data_buf, sizeof(data_buf));
    }

    return 0;
}

static BOOLEAN 
is_mgmt_sfp_presence_change (int gmac_id, int checkForPresenceChange)
{
    uint8_t data = 0;
    uint8_t reg_to_read = 0;
    soho_cpld_status_t status = SOHO_CPLD_ERR;
    gmac_eth_info_t *eth_info = NULL;
    char *override = NULL;

    /*
        By default SFP is the priority.
        If we want to overide RJ45 with SFP set rj45.override.sfp envvar
    */
    override = getenv("rj45.override.sfp");
    if (override != NULL) {
        return FALSE;
    }

    if (gmac_id == MGMT_PORT5) {
        reg_to_read = SOHO_SCPLD_SFP_C1_PRE;
    } else if (gmac_id == MGMT_PORT6) {
        reg_to_read = SOHO_SCPLD_SFP_C0_PRE;
    } else {
        return FALSE;
    }
    soho_cpld_read(reg_to_read, &data);
    if (data == 0xFF) {
        b2_debug("%s[%d]: Register value for %d is 0xFF\n", 
                       __func__, __LINE__, gmac_id);
        return FALSE;
    } 

    if (checkForPresenceChange) {
        if (data & MGMT_SFP_PRESENCE_CHANGE) {
            data |= MGMT_SFP_PRESENCE_CHANGE;
            soho_cpld_write(reg_to_read, data);
        } 
    }

    eth_info = eth_info_array[gmac_id];
    if (data & MGMT_SFP_PRESENCE) {
        if (eth_info) {
            data |= 0x20;
            soho_cpld_write(reg_to_read, data);
            b2_debug("%s[%d]: Initializing =0x%x in SFP Mode\n",
                              __func__, __LINE__, gmac_id);
            bcm5482S_init_as_sfp(eth_info);
            if (bcm54xxx_sgmii_linkup(eth_info) == FALSE) {
                eth_info->sgmii_linkup = FALSE;
                b2_debug("phy_addr=0x%x sgmii link is down\n",
                                eth_info->phy_id);
            } else {
                eth_info->sgmii_linkup = TRUE;
                b2_debug("phy_addr=0x%x sgmii link is UP\n",
                                eth_info->phy_id);
            }
            if (bcm5482S_sfp_linkup(eth_info) == TRUE){
                eth_info->sfp_linkup  = TRUE;
                b2_debug("phy_addr=0x%x SFP link is UP\n",
                                eth_info->phy_id);
            } else {
                eth_info->sfp_linkup  = FALSE;
                b2_debug("phy_addr=0x%x SFP link is FALSE\n",
                                eth_info->phy_id);
            }
            status =  SOHO_CPLD_OK;
        }
    }
    return TRUE;
}

U_BOOT_CMD(
    fx_read_phy_reg,  
    3,   
    1,   
    fx_gmac_read_phy,
    "fx_read_phy_reg - cb read a phy register\n",
    "read a phy register <port> <phy reg id>\n"
);

U_BOOT_CMD(
    fx_write_phy_reg, 
    4, 
    1, 
    fx_gmac_write_phy,
    "fx_write_phy_reg - cb write phy register\n",
    "write_phy_reg write a phy register <port> <phy reg id> <value>\n"
    );


U_BOOT_CMD(
    fx_gmac_send, 
    3,
    1,
    fx_gmac_send,
    "fx_gmac_send - gmac_send packets\n",
    "fx_gmac_send - send <gmac_port> <number of packets>\n"
    );

U_BOOT_CMD(
    fx_dump_pkt, 
    2, 
    1, 
    fx_dump_pkt,
    "fx_dump_pkt- turn on/off dump packet flag\n",
    "fx_dump_pkt- fx_dump_pkt <0 or 1>\n"
    );
