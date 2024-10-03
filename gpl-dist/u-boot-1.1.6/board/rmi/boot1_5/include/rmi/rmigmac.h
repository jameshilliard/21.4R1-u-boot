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


#ifndef _RMIGMAC_H_
#define _RMIGMAC_H_

#include <common.h>
#include <net.h>
#include "rmi/types.h"
#include "rmi/msgring.h"

#define MAC_BYTE_OFFSET 2
#define DEFAULT_RMI_PROCESSOR_IO_BASE 0xbef00000

typedef uint8_t   BOOLEAN;
typedef uchar txbuffer_t[PKTSIZE_ALIGN];

typedef struct
{
   unsigned char dev_addr[20];
	int init_done;
   int speed;
   int fdx;
   int port_id;
   uint32_t phy_id;            /* phy id */
   uint32_t phy_type;          /* BCM phy type */
   int phy_mode;               /* sgmii or rgmii */
   uint32_t mmio_addr;         /* config address */
   uint32_t mii_addr;          /* mdio addr */
   uint32_t pcs_addr;          /* only for sgmii ports */
   uint32_t serdes_addr;       /* only for sgmii ports */
	uint32_t pcs_phyid;         /* phy id for pcs */
	uint32_t serdes_phyid;      /* phy id for serdes */
   BOOLEAN sgmii_linkup;
   BOOLEAN copper_linkup;
   BOOLEAN sfp_linkup;
   BOOLEAN phy_reset;
   void *frin_spill_packets;
   void *frout_spill_packets;
   void *rx_spill_packets;
   void (*make_desc_tx) (struct msgrng_msg * msg,
                         int id,
                         unsigned long addr,
                         int len,
                         int *stid,
                         int *size);
   void (*make_desc_rfr) (struct msgrng_msg * msg,
                          int id,
                          unsigned long addr,
                          int *stid);
   uchar *(*make_desc_rx) (int stid,
                                struct msgrng_msg * msg,
                                int *ctrl,
                                int *port,
                                int *len);
} gmac_eth_info_t;

enum rmi_gmac_modes {RMI_GMAC_IN_SGMII = 0, RMI_GMAC_IN_RGMII = 1};

/* for spill areas */

struct size_1_desc {
   uint64_t entry0;
}; 
   
struct size_2_desc {
   uint64_t entry0;
   uint64_t entry1;
}; 

struct size_3_desc {
   uint64_t entry0;
   uint64_t entry1;
   uint64_t entry2;
};

struct size_4_desc {
   uint64_t entry0;
   uint64_t entry1;
   uint64_t entry2;
   uint64_t entry3;
};     
   
struct fr_desc {
   struct size_1_desc d1;
}; 
      
union rx_tx_desc {
   struct size_2_desc d2;
   struct size_3_desc d3;
   struct size_4_desc d4;
};

enum ctrl {
   CTRL_RES0,
   CTRL_RES1, 
   CTRL_REG_FREE,
   CTRL_JUMBO_FREE,
   CTRL_CONT,
   CTRL_EOP, 
   CTRL_START,
   CTRL_SNGL,
};

enum ctrl_b0 {
   CTRL_B0_NOT_EOP,
   CTRL_B0_EOP,
};

enum phoenix_gmac_regs {
   MAC_CONF1 = (0x000),
   MAC_CONF2 = (0x001),

   MIIM_CONF = (0x008),
   MIIM_CMD  = (0x009),
   MIIM_ADDR = (0x00A),
   MIIM_CTRL = (0x00B),
   MIIM_STAT = (0x00C),
   MIIM_IND  = (0x00D),
   SGMII_SPEED = (0x0E),

   MAC_ADDR0_LO = (0x050),
   MAC_ADDR0_HI = (0x051),

        MAC_ADDR_MASK0_LO = (0x058),
        MAC_ADDR_MASK0_HI = (0x059),
        MAC_ADDR_MASK1_LO = (0x05A),
        MAC_ADDR_MASK1_HI = (0x05B),
        MAC_FILTER_CONFIG = (0x05C),

   TxControl    = (0x0A0),
   RxControl    = (0x0A1),
   DescPackCtrl = (0x0A2),
   StatCtrl     = (0x0A3),
   L2AllocCtrl  = (0x0A4),
   IntMask      = (0x0A5),
   IntReg       = (0x0A6),


   CoreControl =      (0x0A8),
   ByteOffsets0 =     (0x0A9),
   ByteOffsets1 =     (0x0AA),
   PauseFrame =       (0x0AB),
   FIFOModeCtrl =     (0x0AF),

        L2Type =                         (0x0F0),
        ParserConfig =                   (0x100),
        ParseDepth =                     (0x101),
        ExtractStringHashMask0 =         (0x102),
        ExtractStringHashMask1 =         (0x103),
        ExtractStringHashMask2 =         (0x104),
        ExtractStringHashMask3 =         (0x105),
        L3_L4_PROTO_MASK =               (0x106),

        L3CTable =                       (0x140),

        L4CTable =                       (0x160),

   TranslateTable_0 =             (0x1A0),
   TranslateTable_1 =             (0x1A1),

        L3CTableExtraKeys =              (0x1E0),

        L4_CTABLE_EXTRA_KEYS =           (0x1E4),

        HASH7_BASE_MASK =                (0x1E8),

        HASH7_BASE_MASK_CAM_KEYS =       (0x1EC),


        DmaCr0 =                         (0x200),
        DmaCr1 =                         (0x201),
        DmaCr2 =                         (0x202),
        DmaCr3 =                         (0x203),
        RegFrInSpillMemStart0 =                  (0x204),
        RegFrInSpillMemStart1 =                  (0x205),
        RegFrInSpillMemSize =                    (0x206),
        FrOutSpillMemStart0 =                    (0x207),
        FrOutSpillMemStart1 =                    (0x208),
        FrOutSpillMemSize =                      (0x209),

        Class0SpillMemStart0 =                   (0x20A),
        Class0SpillMemStart1 =                   (0x20B),
        Class0SpillMemSize =                     (0x20C),

        Class1SpillMemStart0 =                   (0x210),
        Class1SpillMemStart1 =                   (0x211),
        Class1SpillMemSize =                     (0x212),

        Class2SpillMemStart0 =                   (0x213),
        Class2SpillMemStart1 =                   (0x214),
        Class2SpillMemSize =                     (0x215),

        Class3SpillMemStart0 =                   (0x216),
        Class3SpillMemStart1 =                   (0x217),
        Class3SpillMemSize =                     (0x218),

        RegFrIn1SpillMemStart0 =                 (0x219),
        RegFrIn1SpillMemStart1 =                 (0x21A),
        RegFrIn1SpillMemSize =                   (0x21B),

   FreeQCarve =                             (0x233),
        XLR_REG_XGS_DBG_DESC_WORD_CNT = (0x23c),
        XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL = (0x23d),
   

        ClassWatermarks =                        (0x244),
        RxWatermarks0 =                  (0x245),
        RxWatermarks1 =                  (0x246),
        RxWatermarks2 =                  (0x247),
        RxWatermarks3 =                  (0x248),
        FreeWatermarks =                 (0x249),

        PDE_CLASS0_0 =                   (0x300),
        PDE_CLASS0_1 =                   (0x301),
        PDE_CLASS1_0 =                   (0x302),
        PDE_CLASS1_1 =                   (0x303),
        PDE_CLASS2_0 =                   (0x304),
        PDE_CLASS2_1 =                   (0x305),
        PDE_CLASS3_0 =                   (0x306),
        PDE_CLASS3_1 =                   (0x307),


   G_RFR0_BUCKET_SIZE = (0x321),
   G_TX0_BUCKET_SIZE = (0x322),
   G_TX1_BUCKET_SIZE = (0x323),
   G_TX2_BUCKET_SIZE = (0x324),
   G_TX3_BUCKET_SIZE = (0x325),

   G_RFR1_BUCKET_SIZE = (0x327),


   CC_CPU0_0 = (0x380),
   CC_CPU1_0 = (0x388),
   CC_CPU2_0 = (0x390),
   CC_CPU3_0 = (0x398),
   CC_CPU4_0 = (0x3a0),
   CC_CPU5_0 = (0x3a8),
   CC_CPU6_0 = (0x3b0),
   CC_CPU7_0 = (0x3b8)
};

enum {
   MAC_SPEED_10 = 0x00,
   MAC_SPEED_100,
   MAC_SPEED_1000,
};

enum {
   SGMII_SPEED_10   = 0x00000000,
   SGMII_SPEED_100  = 0x02000000,
   SGMII_SPEED_1000 = 0x04000000,
};

enum {
   FDX_HALF = 0x00,
   FDX_FULL,
};

enum mac_glue_config {
   TX_CONTROL = 0xA0,
   RX_CONTROL,
   DESC_PACK_CTRL,
   STAT_CTRL,
   L2_ALLOC_CTRL,
   INT_MASK,
   INT_REG,

   CORE_CONTROL = 0xA8,

   PAUSE_FRAME = 0xAB,
   BAD_DESC_0,
   BAD_DESC_1,
   TX_VLAN_SRC_ADDR,
   FIFO_MODE_CTRL,

   L2_TYPE = 0xF0,

   PARSER_CONFIG = 0x100,
   L3_CTABLE = 0x140,
   /* L3 Cam table is 16 entries deep and each entry is 2 regs wide */
   L4_CTABLE = 0x160,
   /* L3 Cam table is 8 entries deep and each entry is 2 regs wide */
   CAM_4X_128_TABLE = 0x172,
   CAM_4X_128_KEY = 0x180,
   TRANSLATE_TABLE = 0x1A0,
   /* Translate Table is 128 entries deep and each entry is 16-bit wide
    * (2 entries per reg) */

   DMA_CR_0 = 0x200,
   DMA_CR_1,
   DMA_CR_2,
   DMA_CR_3,
   REG_FRIN_SPILL_MEM_START_0,
   REG_FRIN_SPILL_MEM_START_1,
   REG_FRIN_SPILL_MEM_SIZE,
   FROUT_SPILL_MEM_START_0, // 0x207
   FROUT_SPILL_MEM_START_1, // 0x208
   FROUT_SPILL_MEM_SIZE,    // 0x209
   CLASS0_SPILL_MEM_START_0, // 0x20A
   CLASS0_SPILL_MEM_START_1, // 0x20B
   CLASS0_SPILL_MEM_SIZE,  // 0x20C
   JUM_FRIN_SPILL_MEM_START_0, // 0x20D
   JUM_FRIN_SPILL_MEM_START_1, // 0x20E
   JUM_FRIN_SPILL_MEM_SIZE,  // 0x20F
   CLASS1_SPILL_MEM_START_0, // 0x210
   CLASS1_SPILL_MEM_START_1, // 0x211
   CLASS1_SPILL_MEM_SIZE,    // 0x212
   CLASS2_SPILL_MEM_START_0, // 0x213
   CLASS2_SPILL_MEM_START_1, // 0x214
   CLASS2_SPILL_MEM_SIZE,    // 0x215
   CLASS3_SPILL_MEM_START_0, // 0x216
   CLASS3_SPILL_MEM_START_1, // 0x217
   CLASS3_SPILL_MEM_SIZE,   // 0x218
   SPI_HNGY_0,
   SPI_HNGY_1,
   SPI_HNGY_2,
   SPI_HNGY_3,
   SPI_STRV_0,
   SPI_STRV_1,
   SPI_STRV_2,
   SPI_STRV_3,

   REG_FRIN_1_SPILL_MEM_START_0 = 0x219,
   REG_FRIN_1_SPILL_MEM_START_1,
   REG_FRIN_1_SPILL_MEM_SIZE,
   JUM_FRIN_1_SPILL_MEM_START_0,
   JUM_FRIN_1_SPILL_MEM_START_1,
   JUM_FRIN_1_SPILL_MEM_SIZE,
   FREEQ_CARVE = 0x233,

   ROUND_ROBIN_TABLE,
   PDE_CLASS_0 = 0x300,
   PDE_CLASS_1 = 0x302,
   PDE_CLASS_2 = 0x304,
   PDE_CLASS_3 = 0x306,
   
   G_JFR0_BUCKET_SIZE = 0x320,
   G_JFR1_BUCKET_SIZE = 0x326, 
   
   XGS_TX0_BUCKET_SIZE = 0x320,
   XGS_TX1_BUCKET_SIZE,
   XGS_TX2_BUCKET_SIZE,
   XGS_TX3_BUCKET_SIZE,
   XGS_TX4_BUCKET_SIZE,
   XGS_TX5_BUCKET_SIZE,
   XGS_TX6_BUCKET_SIZE,
   XGS_TX7_BUCKET_SIZE,
   XGS_TX8_BUCKET_SIZE,
   XGS_TX9_BUCKET_SIZE,
   XGS_TX10_BUCKET_SIZE,
   XGS_TX11_BUCKET_SIZE,
   XGS_TX12_BUCKET_SIZE,
   XGS_TX13_BUCKET_SIZE,
   XGS_TX14_BUCKET_SIZE,
   XGS_TX15_BUCKET_SIZE,
   XGS_JFR_BUCKET_SIZE,
   XGS_RFR_BUCKET_SIZE,
   
#if 0
   CC_CPU0_0 = 0x380,
   CC_CPU1_0 = 0x388,
   CC_CPU2_0 = 0x390,
   CC_CPU3_0 = 0x398,
   CC_CPU4_0 = 0x3a0,
   CC_CPU5_0 = 0x3a8,
   CC_CPU6_0 = 0x3b0,
   CC_CPU7_0 = 0x3b8
#endif
}; 

#endif
