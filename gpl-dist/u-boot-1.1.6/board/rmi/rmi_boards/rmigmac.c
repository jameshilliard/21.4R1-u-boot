/***********************************************************************
Copyright 2003-2006 Raza Microelectronics, Inc.(RMI). All rights
reserved.
Use of this software shall be governed in all respects by the terms and
conditions of the RMI software license agreement ("SLA") that was
accepted by the user as a condition to opening the attached files.
Without limiting the foregoing, use of this software in source and
binary code forms, with or without modification, and subject to all
other SLA terms and conditions, is permitted.
Any transfer or redistribution of the source code, with or without
modification, IS PROHIBITED,unless specifically allowed by the SLA.
Any transfer or redistribution of the binary code, with or without
modification, is permitted, provided that the following condition is
met:
Redistributions in binary form must reproduce the above copyright
notice, the SLA, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution:
THIS SOFTWARE IS PROVIDED BY Raza Microelectronics, Inc. `AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL RMI OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*****************************#RMI_3#***********************************/

/* 
	implemention of RMI GMAC driver
*/

#include <malloc.h>

#include "rmi/rmigmac.h"
#include "rmi/gmac.h"
#include "rmi/on_chip.h"
#include "rmi/xlr_cpu.h"
#include "rmi/shared_structs.h"
#include "rmi/cpu_ipc.h"

#define MAX_PACKETS 256
#define DEFAULT_SPILL_ENTRIES (MAX_PACKETS>>1)
#define MAX_FRIN_SPILL_ENTRIES DEFAULT_SPILL_ENTRIES
#define MAX_FROUT_SPILL_ENTRIES DEFAULT_SPILL_ENTRIES
#define MAX_RX_SPILL_ENTRIES DEFAULT_SPILL_ENTRIES
#define CACHELINE_SIZE 32

extern void gmac_phy_init(gmac_eth_info_t *this_phy);
extern int gmac_phy_read(gmac_eth_info_t *this_phy, int reg, uint32_t *data);
extern int gmac_phy_write(gmac_eth_info_t *this_phy, int reg, uint32_t data);
extern void gmac_clear_phy_intr(gmac_eth_info_t *this_phy);
extern uint32_t rmi_get_gmac_phy_id(unsigned int mac, int type);

extern unsigned long rmi_get_gmac_mmio_addr(unsigned int gmac);
extern unsigned long rmi_get_gmac_mii_addr(unsigned int mac, int type);
extern unsigned long rmi_get_gmac_pcs_addr(unsigned int mac);
extern unsigned long rmi_get_gmac_serdes_addr(unsigned int mac);

extern struct boot1_info boot_info;

extern struct rmi_processor_info rmi_processor;
extern struct rmi_board_info rmi_board;

static int max_packets_tx_count;
int split_mode = 0; /* disabled */

static void *cacheline_aligned_malloc(int size)
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

static void *gmac_frin_spill[8]; // for Free In Descriptor FIFO
static void *gmac_frout_spill[8]; // for Free Out Descriptor FIFO
static void *gmac_rx_spill[8];  // for Class FIFO

void mac_spill_init(void)
{
    int i, size = 0;

    size = sizeof(struct fr_desc) * MAX_FRIN_SPILL_ENTRIES;
    gmac_frin_spill[0] = gmac_frin_spill[1] = cacheline_aligned_malloc(size);
	 if (split_mode) {
        gmac_frin_spill[2] = gmac_frin_spill[3] = cacheline_aligned_malloc(size);
	 } 
	 else {
        gmac_frin_spill[2] = gmac_frin_spill[3] = gmac_frin_spill[0];
	 }

    for (i = 0; i < 4; i++)
        gmac_frout_spill[i] = 0;

    if (chip_processor_id_xls()) {
        gmac_frin_spill[4] = gmac_frin_spill[5] = cacheline_aligned_malloc(size);
        if (split_mode) {
            gmac_frin_spill[6] = 
					gmac_frin_spill[7] = cacheline_aligned_malloc(size);
        }
        else {
            gmac_frin_spill[6] = gmac_frin_spill[7] = gmac_frin_spill[4];
        }
        for (i = 4; i < 8; i++)
            gmac_frout_spill[i] = 0;
    }

    size = sizeof(union rx_tx_desc) * MAX_RX_SPILL_ENTRIES;
    gmac_rx_spill[0] = gmac_rx_spill[1] = cacheline_aligned_malloc(size);
    if (split_mode) {
        gmac_rx_spill[2] = gmac_rx_spill[3] = cacheline_aligned_malloc(size);
    }
    else {
        gmac_rx_spill[2] = gmac_rx_spill[3] = gmac_rx_spill[0];
    }

    if (chip_processor_id_xls()) {
        gmac_rx_spill[4] = gmac_rx_spill[5] = cacheline_aligned_malloc(size);
        if (split_mode) {
            gmac_rx_spill[6] = gmac_rx_spill[7] = cacheline_aligned_malloc(size);
        }
        else {
            gmac_rx_spill[6] = gmac_rx_spill[7] = gmac_rx_spill[4];
        }
    }

    size = sizeof(struct fr_desc) * MAX_FROUT_SPILL_ENTRIES;
    gmac_frout_spill[0] = gmac_frout_spill[1] = cacheline_aligned_malloc(size);
    if (split_mode) {
        gmac_frout_spill[2]= gmac_frout_spill[3]= cacheline_aligned_malloc(size);
    }
    else {
        gmac_frout_spill[2] = gmac_frout_spill[3] = gmac_frout_spill[0];
    }
    if (chip_processor_id_xls()) {
        gmac_frout_spill[4]= gmac_frout_spill[5]= cacheline_aligned_malloc(size);
        if (split_mode) {
            gmac_frout_spill[6] = gmac_frout_spill[7] = 
					cacheline_aligned_malloc(size);
        }
        else {
            gmac_frout_spill[6] = gmac_frout_spill[7] = gmac_frout_spill[4];
        }
    }
}


static void mac_make_desc_b0_rfr(struct msgrng_msg *msg, int id,
                                 unsigned long addr, int *pstid)
{
   int stid = 0;

   stid = msgrng_gmac_stid_rfr(id);
   msg->msg0 = (uint64_t) addr & 0xffffffffe0ULL;
   *pstid = stid;
}

static void mac_make_desc_xls_rfr(struct msgrng_msg *msg, int id,
                                 unsigned long addr, int *pstid)
{
   int stid = 0;

   if (id < 4)
      stid = msgrng_gmac0_stid_rfr(id);
   else
      stid = msgrng_gmac1_stid_rfr(id-4);

   msg->msg0 = (uint64_t) addr & 0xffffffffe0ULL;
   *pstid = stid;
}


static void mac_make_desc_b0_tx(struct msgrng_msg *msg,
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

   msg->msg0 = (((uint64_t) CTRL_B0_EOP << 63) |
           ((uint64_t) fr_stid << 54) |
           ((uint64_t) len << 40) |
           ((uint64_t) addr & 0xffffffffffULL)
       );

   *stid = tx_stid;
   *size = 1;
}

static void mac_make_desc_xls_tx(struct msgrng_msg *msg,
                                int id,
                                unsigned long addr,
                                int len,
                                int *stid,
                                int *size)
{
   int tx_stid = 0;
   int fr_stid = 0;

   if (id < 4)
         tx_stid = msgrng_gmac0_stid_tx(id);
   else
      tx_stid = msgrng_gmac1_stid_tx(id);

   fr_stid = msgrng_cpu_to_bucket(processor_id());

   msg->msg0 = (((uint64_t) CTRL_B0_EOP << 63) |
                ((uint64_t) fr_stid << 54) |
                ((uint64_t) len << 40) |
                ((uint64_t) addr & 0xffffffffffULL)
               );

   *stid = tx_stid;
   *size = 1;
}


static uchar *mac_make_desc_b0_rx(int stid, struct msgrng_msg *msg,
         int *ctrl, int *port, int * len)
{
   uchar * pbuf = 0;

   pbuf = ((uchar *) phys_to_virt((msg->msg0 & 0xffffffffe0ULL)));
   *len = ((uint16_t) (msg->msg0 >> 40)) & 0x3fff;
   *port = msg->msg0 & 0x0f;

   if ((*len) == 0)
      *ctrl = CTRL_REG_FREE;
   else
      *ctrl = (msg->msg0 >> 63) & 0x01;

   return pbuf;
}

static uchar  *mac_make_desc_xls_rx
   (int stid, struct msgrng_msg *msg, int *ctrl, int *port, int *len)
{
   uchar  *pbuf = 0;

   pbuf = ((uchar *) phys_to_virt((msg->msg0 & 0xffffffffe0ULL)));
   *len = ((uint16_t) (msg->msg0 >> 40)) & 0x3fff;

   if (stid == MSGRNG_STNID_GMAC0)
         *port = msg->msg0 & 0x0f;
   else if (stid == MSGRNG_STNID_GMAC1)
      *port = 4 + (msg->msg0 & 0x0f);
   else
      printf("[%s]: Received from station %d\n", __FUNCTION__, stid);

   if ((*len) == 0)
      *ctrl = CTRL_REG_FREE;
   else
      *ctrl = (msg->msg0 >> 63) & 0x01;

   return pbuf;
}

static unsigned int pbuf_debug = 0;

void print_packet(const char *pkt_buf, int len)
{
   int i = 0, j = 0;
   unsigned short *p = (unsigned short *)pkt_buf;
   unsigned int   nWords = len/2;
   if(pbuf_debug)
   {
      printf("Dumping %d bytes\n",len);
      for (i = 0; i < nWords; ) {
         printf("\t0x%04x:  ",2*i);
         for(j = 0; j <8 ; j++)
         {
            if(i==nWords) break;
            printf("%04x ", *p);
            p++;  i++;
         }
      printf("\n");
      }
   }
}


static void mac_change_hwaddr(struct eth_device *dev , uint64_t mac_addr)
{
   gmac_eth_info_t * eth_info = (gmac_eth_info_t *) dev->priv;
   phoenix_reg_t *mmio = (phoenix_reg_t *)eth_info->mmio_addr;

   //printf("[%s]: Programming mac address to 0x%016llx in the hardware\n",
         //__FUNCTION__, mac_addr);

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

   printf("Read MAC Address = %02x:%02x.%02x:%02x.%02x:%02x\n",
		   dev->enetaddr[0], dev->enetaddr[1], 
		   dev->enetaddr[2], dev->enetaddr[3], 
		   dev->enetaddr[4], dev->enetaddr[5]);

   phoenix_write_reg(mmio, MAC_ADDR0_LO,
           (eth_info->dev_addr[5] << 24) |
           (eth_info->dev_addr[4] << 16) |
           (eth_info->dev_addr[3]<< 8)   |
           (eth_info->dev_addr[2]) );

   phoenix_write_reg(mmio, MAC_ADDR0_HI,
           (eth_info->dev_addr[1] << 24) |
           (eth_info->dev_addr[0] << 16));

   phoenix_write_reg(mmio, MAC_ADDR_MASK0_LO , 0xffffffff);
   phoenix_write_reg(mmio, MAC_ADDR_MASK0_HI , 0xffffffff);
   phoenix_write_reg(mmio, MAC_ADDR_MASK1_LO , 0xffffffff);
   phoenix_write_reg(mmio, MAC_ADDR_MASK1_HI , 0xffffffff);
   phoenix_write_reg(mmio, MAC_FILTER_CONFIG , 0x401);
}

void rmi_reset_gmac(phoenix_reg_t *mmio)
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
   while(1) {
      val = phoenix_read_reg(mmio, RxControl);
      if(val & 0x2)
      	break;
      udelay(1*1000);
      udelay(1*1000);
      udelay(1*1000);
   }

   /* Issue a soft reset */
   val = phoenix_read_reg(mmio, RxControl);
   val |= 0x4;
   phoenix_write_reg(mmio, RxControl, val);

   /* wait for reset to complete */
   while(1) {
      val = phoenix_read_reg(mmio, RxControl);
      if(val & 0x8)
      	break;
      udelay(1*1000);
      udelay(1*1000);
      udelay(1*1000);
   }

   /* Clear the soft reset bit */
   val = phoenix_read_reg(mmio, RxControl);
   val &= ~0x4;
   phoenix_write_reg(mmio, RxControl, val);
}

static void gmac_config_speed(struct eth_device *dev)
{
   gmac_eth_info_t * eth_info = (gmac_eth_info_t *) dev->priv;
   phoenix_reg_t *mmio = (phoenix_reg_t *)eth_info->mmio_addr;
   int speed =0;
   int fdx = 0;
   uint32_t value = 0;
   const char *fdx_str = 0;

	if (!gmac_phy_read(eth_info, 28, &value))
   {
      speed = MAC_SPEED_10;
      fdx = FDX_FULL;
   }
   else
   {
      if (!(value & 0x8000)) {
         speed = MAC_SPEED_10;
         fdx = FDX_FULL;
      }
      else
      {
         speed = (value >> 3) & 0x03;
         fdx = (value >> 5) & 0x01;
      }
   }
   fdx_str = fdx == FDX_FULL ? "full duplex" : "half duplex";

   if((speed != eth_info->speed) || (fdx != eth_info->fdx))
   {
      eth_info->speed = speed;
      eth_info->fdx = fdx;
      /* configure the GMAC Registers */
      if (eth_info->speed == MAC_SPEED_10) 
		{
         /* nibble mode */
         phoenix_write_reg(mmio, MAC_CONF2, (0x7136 | (eth_info->fdx & 0x01)));
         /* 2.5MHz */
         phoenix_write_reg(mmio, CoreControl , 0x02);
         phoenix_write_reg(mmio, SGMII_SPEED, SGMII_SPEED_10);
         printf("configuring %s in nibble mode @2.5MHz (10Mbps):%s mode\n",
                dev->name, fdx_str);
      }
      else if (eth_info->speed == MAC_SPEED_100)
      {
         /* nibble mode */
         phoenix_write_reg(mmio, MAC_CONF2 , (0x7136 | (eth_info->fdx & 0x01)));
         /* 25MHz */
         phoenix_write_reg(mmio, CoreControl , 0x01);
         phoenix_write_reg(mmio, SGMII_SPEED, SGMII_SPEED_100);
         printf("configuring %s in nibble mode @25MHz (100Mbps):%s mode\n",
                dev->name, fdx_str);
      }
      else
      {
         /* byte mode */
         if (eth_info->speed != MAC_SPEED_1000)
         {
            /* unknown speed */
            printf("Unknown mac speed, defaulting to GigE\n");
         }
         phoenix_write_reg(mmio, MAC_CONF2 , (0x7236 | (eth_info->fdx & 0x01)));
         phoenix_write_reg(mmio, CoreControl , 0x00);
         phoenix_write_reg(mmio, SGMII_SPEED, SGMII_SPEED_1000);
         printf("configuring %s in byte mode @125MHz (1000Mbps):%s mode\n",
                dev->name, fdx_str);
      }
   }
}


static void mac_init_msgring(struct eth_device *ndev) __attribute__ ((unused));
static void mac_init_msgring(struct eth_device *ndev)
{
   gmac_eth_info_t * eth_info = (gmac_eth_info_t *)ndev->priv;
   phoenix_reg_t *mmio = (phoenix_reg_t *)eth_info->mmio_addr;
   int gmac_id = eth_info->port_id;
   uint32_t cc_reg = CC_CPU0_0;
   int i = 0;

   if(chip_processor_id_xls())
	{
		if(gmac_id < 4) 
		{
         phoenix_write_reg(mmio,G_TX0_BUCKET_SIZE + gmac_id,
            xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC0_TX0 + gmac_id]);
         phoenix_write_reg(mmio,G_RFR0_BUCKET_SIZE,
            xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC0_FR_0]);
         phoenix_write_reg(mmio,G_RFR1_BUCKET_SIZE,
            xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC0_FR_1]);
		}	
		else 
		{
         phoenix_write_reg(mmio,G_TX0_BUCKET_SIZE + (gmac_id & 0x3),
            xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC1_TX0 + (gmac_id & 0x3)]);
         phoenix_write_reg(mmio,G_RFR0_BUCKET_SIZE,
            xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC1_FR_0]);
         phoenix_write_reg(mmio,G_RFR1_BUCKET_SIZE,
            xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC1_FR_1]);
		}
   }
	else 
	{
      phoenix_write_reg(mmio,  (G_TX0_BUCKET_SIZE + gmac_id),
         bucket_sizes.bucket[MSGRNG_STNID_GMACTX0 + gmac_id]);
      phoenix_write_reg(mmio,G_RFR0_BUCKET_SIZE,
         bucket_sizes.bucket[MSGRNG_STNID_GMACRFR_0]);
      phoenix_write_reg(mmio,G_RFR1_BUCKET_SIZE,
         bucket_sizes.bucket[MSGRNG_STNID_GMACRFR_1]);
   }
   phoenix_write_reg(mmio,G_JFR0_BUCKET_SIZE, 0);
   phoenix_write_reg(mmio,G_JFR1_BUCKET_SIZE, 0);

   for (i = 0; i < 128; i++) 
	{
      if(chip_processor_id_xls()) 
		{
         if (gmac_id < 4)
            phoenix_write_reg(mmio,(cc_reg + i),
            	xls_cc_table_gmac0.counters[i >> 3][i & 0x07]);
         else
            phoenix_write_reg(mmio,(cc_reg + i),
            	xls_cc_table_gmac1.counters[i >> 3][i & 0x07]);
      } 
		else
		{
         phoenix_write_reg(mmio,(cc_reg + i),
              cc_table_gmac.counters[i >> 3][i & 0x07]);
		}
   }
}

static void mac_receiver_enable (struct eth_device *dev)
{
   gmac_eth_info_t * eth_info = (gmac_eth_info_t *)dev->priv;
   phoenix_reg_t *mmio = (phoenix_reg_t *)eth_info->mmio_addr;
   uint32_t rx_control = 1;
   int gmac_id = eth_info->port_id;

   if(chip_processor_id_xls()) {
      if ((eth_info->phy_mode == RMI_GMAC_IN_RGMII) && (gmac_id == 0)) {
         rx_control = (1<<10) | 1;
         /* printf("configuring gmac%d in RGMII mode...\n", gmac_id); */
      }
      else  {
         rx_control = 1;
         /* printf("configuring gmac%d in SGMII mode...\n", gmac_id); */
      }
   }
   /* Enable the Receiver */
   phoenix_write_reg(mmio,RxControl,rx_control);
   //printf("[%s@%d]: gmac_%d: RX_CONTROL = 0x%08x\n", __FUNCTION__,
         //__LINE__, gmac_id, phoenix_read_reg(mmio, RX_CONTROL));

}

static int mac_open(struct eth_device *ndev)
{
   gmac_eth_info_t * eth_info = (gmac_eth_info_t *)ndev->priv;
   int gmac_id = eth_info->port_id;

   int i = 0, stid = 0;
   uchar * pbuf;
   struct msgrng_msg msg;

   /* Send free descs to the MAC */

#ifdef UBOOT_DELETE_ME
   for (i = 0; i < PKTBUFSRX; i++) {
      pbuf = (txbuffer_t *) NetRxPackets[i];
      eth_info->make_desc_rfr(&msg, gmac_id, virt_to_phys(pbuf), &stid);

#ifdef DUMP_MESSAGES
      if (global_debug) {
         printf
             ("cpu_%d: Sending rfr_%d @ (%lx) to station id %d\n",
              processor_id(), i, virt_to_phys(pbuf), stid);
      }
#endif

      message_send_block(1, MSGRNG_CODE_MAC, stid, &msg);
   }
#endif

	for (i=0; i<128; i++)
	{
		pbuf = (uchar *) cacheline_aligned_malloc(PKTSIZE_ALIGN);
		if (!pbuf)
			return 0;
      eth_info->make_desc_rfr(&msg, gmac_id, virt_to_phys(pbuf), &stid);
      message_send_block(1, MSGRNG_CODE_MAC, stid, &msg);
	}
	mac_receiver_enable (ndev);
   return 1;
}


int gmac_drain_msgs(struct eth_device* dev);
static int gmac_init(struct eth_device* dev, bd_t * bd)
{
   gmac_eth_info_t * eth_info = (gmac_eth_info_t *)dev->priv;
   phoenix_reg_t *mmio = (phoenix_reg_t *)eth_info->mmio_addr;
   int gmac_id = eth_info->port_id;
	uint32_t linkstatus = 0, i;

   if (! (gmac_phy_read(eth_info, 01, &linkstatus))) {
		printf ("Unable to read status, assuming interface is disabled\n");
		return 0;
	}

	if (eth_info->init_done) {
		for(i=0; i < 8; i++)
			gmac_drain_msgs(dev);
		mac_receiver_enable (dev);

	 	if ((linkstatus>>2) & 0x01) {
#ifdef DEBUG
			printf("[%s]:Init previously done and link is Up.\n", __FUNCTION__);
#endif
			return 1;
		}
		else {
			return 0;
		}
	}

   uint64_t mac_addr = boot_info.mac_addr + gmac_id;

   int phy_value;
   uint32_t cc_reg = 0;
   int size;

   i = 0;
   /* Reset the MAC */
   rmi_reset_gmac(mmio);

   mac_change_hwaddr(dev, mac_addr);

#ifdef UBOOT_DELETE_ME
	mac_init_msgring(dev);
#endif

   if (gmac_id == 0)
      boot_info.mac_addr = mac_addr;

	gmac_phy_init(eth_info);
   gmac_config_speed(dev);
   phoenix_write_reg(mmio,IntReg,0xffffffff);
	gmac_phy_read(eth_info,26, &phy_value);

   /* Enable MDIO interrupts in the PHY RX_ER bit seems to be getting set
      about every 1sec in GigE, ignore for now...
   */
	gmac_phy_write(eth_info, 25, 0xfffffffe);

	if (chip_processor_id_xls())
	{
      /* Assume GMAC0 is always in RGMII mode at startup*/
      if((gmac_id == 0) && (eth_info->phy_mode == RMI_GMAC_IN_RGMII)) {
         /* Enable RGMII Tx/Rx clk skew for 1.5ns */
         gmac_phy_read(eth_info, 23, &phy_value);
         phy_value = phy_value & ~0xf00;
         phy_value |= (0x5 << 8);
         gmac_phy_write(eth_info, 23, phy_value);
      }
   }

   /* configure the GMAC_GLUE Registers */
   phoenix_write_reg(mmio,DmaCr0,0xffffffff);
   phoenix_write_reg(mmio,DmaCr2,0xffffffff);
   phoenix_write_reg(mmio,DmaCr3,0xffffffff);

	if (split_mode) {
   	phoenix_write_reg(mmio,FreeQCarve,0x60);
	}

   /* allocate the spill areas */
   size = sizeof(struct fr_desc) * MAX_FRIN_SPILL_ENTRIES;
   if (!(eth_info->frin_spill_packets = gmac_frin_spill[gmac_id]))
      size = 0;

	if (split_mode) {
   	if(gmac_id & 0x2) {
      	/* Since all addresses are in KSEG0, no need to configure
         RegFrIn1SpillMemStart1*/
      	phoenix_write_reg(mmio,RegFrIn1SpillMemStart0, // 0x219
              (virt_to_phys(eth_info->frin_spill_packets) >> 5));
      	phoenix_write_reg(mmio,RegFrIn1SpillMemSize,size); // 0x21B
   	}
   	else {
      	/* Since all addresses are in KSEG0, no need to configure
         RegFrInSpillMemStart1*/
      	phoenix_write_reg(mmio,RegFrInSpillMemStart0, // 0x204
              (virt_to_phys(eth_info->frin_spill_packets) >> 5));
      	phoenix_write_reg(mmio,RegFrInSpillMemSize,size); // 0x206
      }
	}
	else {
   	/* Since all addresses are in KSEG0, no need to configure
      RegFrInSpillMemStart1*/
   	phoenix_write_reg(mmio,RegFrInSpillMemStart0, // 0x204
           (virt_to_phys(eth_info->frin_spill_packets) >> 5));
   	phoenix_write_reg(mmio,RegFrInSpillMemSize,size); // 0x206
	}

   size = sizeof(struct fr_desc) * MAX_FROUT_SPILL_ENTRIES;
   if (!(eth_info->frout_spill_packets = gmac_frout_spill[gmac_id]))
      size = 0;

   phoenix_write_reg(mmio,FrOutSpillMemStart0,
           (virt_to_phys(eth_info->frout_spill_packets) >> 5));

   /* Since all addresses are in KSEG0, no need to configure
      FrOutSpillMemStart1 */
   phoenix_write_reg(mmio,FrOutSpillMemSize,size);

   size = sizeof(union rx_tx_desc) * MAX_RX_SPILL_ENTRIES;
   if (!(eth_info->rx_spill_packets = gmac_rx_spill[gmac_id]))
      size = 0;

	if (split_mode) {
   	if(gmac_id & 0x2) {
      	phoenix_write_reg(mmio,Class2SpillMemStart0,
              (virt_to_phys(eth_info->rx_spill_packets) >> 5));
      	phoenix_write_reg(mmio,Class2SpillMemSize,size);
   	}
   	else {
      	phoenix_write_reg(mmio,Class0SpillMemStart0,
              (virt_to_phys(eth_info->rx_spill_packets) >> 5));
      	phoenix_write_reg(mmio,Class0SpillMemSize,size);
   	}
	}
	else {
   	phoenix_write_reg(mmio,Class0SpillMemStart0,
              (virt_to_phys(eth_info->rx_spill_packets) >> 5));
   	phoenix_write_reg(mmio,Class0SpillMemSize,size);
	}

   phoenix_write_reg(mmio,TxControl,((1 << 14) | 512));
   /* Enable transmit of 512 byte packets */
   phoenix_write_reg(mmio,DescPackCtrl,
           ((MAC_BYTE_OFFSET << 17) | ((2 - 1) << 14) | 1536) );
   /* enable mac ip stats */
   phoenix_write_reg(mmio,StatCtrl, 0x04);
   phoenix_write_reg(mmio,L2AllocCtrl,2);
   /* configure the classifier - use port based classification by default */
   phoenix_write_reg(mmio,ParserConfig,0x00000008);
/*
   phoenix_write_reg(mmio,TranslateTable_0,0x00800000);
   phoenix_write_reg(mmio,TranslateTable_1,0x01800100);
*/

   phoenix_write_reg(mmio,TranslateTable_0,0x00);
   phoenix_write_reg(mmio,TranslateTable_1,0x00);

   /* configure PDE - all gmacs send packets to cpu0/bucket0, by default */
   phoenix_write_reg(mmio,PDE_CLASS0_0, 0x01);
   phoenix_write_reg(mmio,PDE_CLASS0_1, 0x00);
   phoenix_write_reg(mmio,PDE_CLASS1_0, 0x01);
   phoenix_write_reg(mmio,PDE_CLASS1_1, 0x00);
   phoenix_write_reg(mmio,PDE_CLASS2_0, 0x01);
   phoenix_write_reg(mmio,PDE_CLASS2_1, 0x00);
   phoenix_write_reg(mmio,PDE_CLASS3_0, 0x01);
   phoenix_write_reg(mmio,PDE_CLASS3_1, 0x00);

   /* Configure the Parser/Classifier Layer 2 information */
   phoenix_write_reg(mmio,L2Type, (0x01 << 0));

   if(chip_processor_id_xls()) {
      if(gmac_id < 4) {
         phoenix_write_reg(mmio,G_TX0_BUCKET_SIZE + gmac_id,
            xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC0_TX0 + gmac_id]);
         phoenix_write_reg(mmio,G_RFR0_BUCKET_SIZE,
            xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC0_FR_0]);
         phoenix_write_reg(mmio,G_RFR1_BUCKET_SIZE,
            xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC0_FR_1]);
      }else {
         phoenix_write_reg(mmio,G_TX0_BUCKET_SIZE + (gmac_id & 0x3),
            xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC1_TX0 + (gmac_id & 0x3)]);
         phoenix_write_reg(mmio,G_RFR0_BUCKET_SIZE,
            xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC1_FR_0]);
         phoenix_write_reg(mmio,G_RFR1_BUCKET_SIZE,
            xls_bucket_sizes.bucket[MSGRNG_STNID_GMAC1_FR_1]);
      }
   }else {
      phoenix_write_reg(mmio,G_TX0_BUCKET_SIZE + gmac_id,
         bucket_sizes.bucket[MSGRNG_STNID_GMACTX0 + gmac_id]);
      phoenix_write_reg(mmio,G_RFR0_BUCKET_SIZE,
         bucket_sizes.bucket[MSGRNG_STNID_GMACRFR_0]);
      phoenix_write_reg(mmio,G_RFR1_BUCKET_SIZE,
         bucket_sizes.bucket[MSGRNG_STNID_GMACRFR_1]);
   }
   phoenix_write_reg(mmio,G_JFR0_BUCKET_SIZE, 0);
   phoenix_write_reg(mmio,G_JFR1_BUCKET_SIZE, 0);

   cc_reg = CC_CPU0_0;

   for (i = 0; i < 128; i++) {
      if(chip_processor_id_xls()) {
         if (gmac_id < 4)
            phoenix_write_reg(mmio,(cc_reg + i),
            xls_cc_table_gmac0.counters[i >> 3][i & 0x07]);
         else
            phoenix_write_reg(mmio,(cc_reg + i),
            xls_cc_table_gmac1.counters[i >> 3][i & 0x07]);
      }else {
         phoenix_write_reg(mmio,(cc_reg + i),
          (cc_table_gmac.counters[i >> 3][i & 0x07]));
      }
   }

   /* Enable the MAC */
   phoenix_write_reg(mmio,MAC_CONF1,0x35);
   /* mask off all interrupts and clear the existing interupt conditions */
   phoenix_write_reg(mmio,IntMask,0x0);

	/* check link status */
	linkstatus = 0;
   if (! (gmac_phy_read(eth_info, 01, &linkstatus))) {
		printf ("Unable to read status, assuming interface is disabled\n");
		return 0;
	}

	if (!((linkstatus>>2) & 0x01)) {
		printf ("link is down ");
		return 0;
	}
	/* printf ("link is UP\n"); */

	eth_info->init_done = 1;

   return (mac_open(dev));
} /* ends ... gmac_init() */


static int gmac_send(struct eth_device* dev, volatile void *pbuf, int len)
{
   gmac_eth_info_t * eth_info = (gmac_eth_info_t *)dev->priv; 
   int gmac_id = eth_info->port_id;
   int stid = 0, size = 0;
   struct msgrng_msg msg;

   if (max_packets_tx_count++ > MAX_PACKETS) {
      printf ("Reached MAX_PACKETS limit \n");
      return -1;
   }

/*
Use application provided buffer, instead of malloc 
   volatile void * pkt_buf = malloc(sizeof(txbuffer_t));
   if (pkt_buf == NULL) {
      printf ("gmac_send: unable to malloc  \n");
      return 0;
   }
   memcpy((uchar *)pkt_buf, (uchar *) pbuf, len);
*/

   eth_info->make_desc_tx(&msg,
           gmac_id,
           virt_to_phys(pbuf), 
           len,
           &stid,
           &size);

   /* Send the packet to MAC */
#ifdef DUMP_MESSAGES
   if (global_debug) {
      printf("[%s]: Sending tx packet to stid %d size %d...\n",
             __FUNCTION__, stid, size);
   }
#endif

   message_send_block(size, MSGRNG_CODE_MAC, stid, &msg);

#ifdef DUMP_MESSAGES
   if (global_debug) {
      printf("[%s]: ...done\n", __FUNCTION__);
   }
#endif

   /* Let the mac keep the free descriptor */
   return 0;
}

int gmac_drain_msgs(struct eth_device* dev)
{
   gmac_eth_info_t * eth_info = (gmac_eth_info_t *)dev->priv;
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
      if (message_receive(bucket, &size, &code, &stid, &msg))
         break;

      /* Msg received */
      pbuf = (uchar *) eth_info->make_desc_rx(stid, &msg, &ctrl, &port, &len);

      if (ctrl == CTRL_REG_FREE) {
         /* Free back descriptor from mac */
#ifdef DUMP_MESSAGES
         if (global_debug) {
            printf
                ("[%s]: received free desc from mac, %llx\n",
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
static int gmac_recv(struct eth_device* dev)
{
   gmac_eth_info_t * eth_info = (gmac_eth_info_t *)dev->priv;
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
      if(chip_processor_id_xls())
         mmio = (phoenix_reg_t *)eth_info->mii_addr;

      if (phoenix_read_reg(mmio,IntReg) & 0x02)
      {
         if (chip_processor_id_xls()) {
            gmac_clear_phy_intr(eth_info);
            gmac_phy_init(eth_info);
         }
         else {
            gmac_clear_phy_intr(eth_info);
         }
         phoenix_write_reg(mmio,IntReg,0xffffffff);
         gmac_config_speed(dev);
      }
   }

   /* Wait indefinitely for a msg */
   for (;;) {

      /* printf("cpu_%d: Waiting for a message from %s, netdev=%p\n", id, dev->name, dev); */

      counter = read_32bit_cp0_register(COP_0_COUNT);
      if (message_receive(bucket, &size, &code, &stid, &msg))
         break;

      /* Msg received */
      pbuf = (uchar *) eth_info->make_desc_rx(stid, &msg, &ctrl, &port, &len);

      if (ctrl == CTRL_REG_FREE) {
         /* Free back descriptor from mac */
#ifdef DUMP_MESSAGES
         if (global_debug) {
            printf
                ("[%s]: received free desc from mac, %llx\n",
                 __FUNCTION__, msg.msg0);
         }
#endif
         /* free(pbuf); */
         max_packets_tx_count--;
         break;
      }

		lbuf = malloc(1536);
		if (!lbuf)
			printf ("** [%s]: unable to malloc.\n", __FUNCTION__);
		memcpy((uchar *)lbuf, (uchar *) pbuf, len);

      /* pass the packet up to the protocol layers */
		len = len - 2 - 4;

#ifdef UBOOT_DELETE_ME
		if(gmac_id != 0) {
				  printf("received pkt addr %p len %d\n", lbuf, len);
				  for(i=0; i < 64; i++) {
							 printf("0x%02x ", (unsigned char)(lbuf[i]));
				  }
		}
#endif
      NetReceive ( (uchar *)lbuf+2, len);
		free (lbuf);

      /* free the descriptor */
      eth_info->make_desc_rfr(&rxmsg, gmac_id, virt_to_phys(pbuf), &stid);
      message_send_block(1, MSGRNG_CODE_MAC, stid, &rxmsg);
   }
   return 0;
} /* ends ... gmac_recv() */


static void mac_close(struct eth_device *dev)
{
	gmac_eth_info_t * eth_info = (gmac_eth_info_t *)dev->priv;
   phoenix_reg_t *mmio = (phoenix_reg_t *)eth_info->mmio_addr;

   /* Disable the Receiver */
   phoenix_write_reg(mmio,RxControl,0);
}


static void gmac_halt(struct eth_device* dev)
{
	int i;
	mac_close (dev);
	for(i=0; i < 8; i++)
		gmac_drain_msgs(dev);
}


int rmigmac_enet_initialize (bd_t *bis)
{
   gmac_eth_info_t * eth_info;
   struct eth_device* dev;
   max_packets_tx_count = 0;
   unsigned int num, i, gmac_list;
   char *mode = getenv("gmac0mode");
   int gmac0_sgmii = 0;

   /* By default gmac0 is assumed to be in RGMII mode on XLS boards if the 
      board has one
      */
   if(mode) {
	   if((strcmp(mode, "sgmii")) == 0) {
		   gmac0_sgmii = 1;
		   printf("Configuring GMAC0 in SGMII mode\n");
	   }
   }

	struct rmi_board_info * rmib = &rmi_board;

	num = rmib->gmac_num;
	gmac_list = rmib->gmac_list;

	i = 0;
   while (i<num)
   {
		if (!((gmac_list>>i) & 0x01)) {
			continue;
		}

      if ((eth_info = (gmac_eth_info_t *) malloc(sizeof(gmac_eth_info_t)))
            == NULL)
      {
         printf ("initialize: malloc fail\n");
         return 0;
      }

		eth_info->init_done = 0;
		eth_info->port_id = i;

		/* assume RGMII mode for XLS-gmac0 and XLR */
		if (chip_processor_id_xls()) {
			eth_info->phy_mode = RMI_GMAC_IN_SGMII;
		}
		else {
			eth_info->phy_mode = RMI_GMAC_IN_RGMII;
		}

		if ((i == 0) &&
			 (chip_processor_id_xls()) &&
			 (rmib->maj != RMI_PHOENIX_BOARD_ARIZONA_VIII)
			 && (gmac0_sgmii == 0)) {
			eth_info->phy_mode = RMI_GMAC_IN_RGMII;
		}

		eth_info->phy_id = rmi_get_gmac_phy_id(i, eth_info->phy_mode);
		eth_info->mmio_addr = rmi_get_gmac_mmio_addr(i);
		eth_info->mii_addr = rmi_get_gmac_mii_addr(i, eth_info->phy_mode);

		if (eth_info->phy_mode == RMI_GMAC_IN_SGMII) {
			eth_info->pcs_addr = rmi_get_gmac_pcs_addr(i);
			eth_info->serdes_addr = rmi_get_gmac_serdes_addr(i);
		} else {
			eth_info->pcs_addr = 0;
			eth_info->serdes_addr = 0;
		}

      if ((dev = (struct eth_device*)malloc(sizeof (*dev))) == NULL)
      {
         printf ("initialize: malloc failed\n");
         return 0;
      }
      memset(dev, 0, sizeof *dev);
      sprintf(dev->name, "gmac%d",i);

      dev->iobase = eth_info->mmio_addr;
      dev->priv   = (void *)eth_info;
      dev->init   = gmac_init;
      dev->halt   = gmac_halt;
      dev->send   = gmac_send;
      dev->recv   = gmac_recv;

		if(chip_processor_id_xls()) { 
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

