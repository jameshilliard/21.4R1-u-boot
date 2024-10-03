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
#ifndef __GMAC_H__
#define __GMAC_H__

struct gmac {
	uint32_t MAC_CONF1 ;	/*  Configuration #1 0x8000_0000 (0x00) */
	uint32_t MAC_CONF2 ;	/*  Configuration #2 0x0000_7000 (0x01) */
	uint32_t IPG_IFG ;	/*  / IFG 0x4060_5060 (0x02) */
	uint32_t HLF_DUP ;	/*  0x00A1_F037 (0x03) */
	uint32_t MAX_FRM ;	/*  Frame 0x0000_0600 (0x04) */


	uint32_t TEST ;		/*  Register 0x0000_0000 (0x07) */
	uint32_t MIIM_CONF ;	/*  Mgmt: Configuration 0x0000_0000 (0x08) */
	uint32_t MIIM_CMD ;	/*  Mgmt: Command 0x0000_0000 (0x09) */
	uint32_t MIIM_ADDR ;	/*  Mgmt: Address 0x0000_0000 (0x0A) */
	uint32_t MIIM_CTRL ;	/*  Mgmt: Control 0x0000_0000 (0x0B) */
	uint32_t MIIM_STAT ;	/*  Mgmt: Status 0x0000_0000 (0x0C) */
	uint32_t MIIM_IND ;	/*  Mgmt: Indicators 0x0000_0000 (0x0D) */
	uint32_t IO_CTRL ;	/*  Control 0x0000_0000 (0x0E) */
	uint32_t IO_STAT ;	/*  Status 0x0000_0000 (0x0F) */


	uint32_t TR64 ;		/*  Transmit and Receive 64 Byte Frame Counter R/W (0x20) */
	uint32_t TR127 ;	/*  Transmit and Receive 65 to 127 Byte Frame Counter R/W (0x21) */
	uint32_t TR255 ;	/*  Transmit and Receive 128 to 255 Byte Frame Counter R/W (0x22) */
	uint32_t TR511 ;	/*  Transmit and Receive 256 to 511 Byte Frame Counter R/W (0x23) */
	uint32_t TR1K ;		/*  Transmit and Receive 512 to 1023 Byte Frame Counter R/W (0x24) */
	uint32_t TRMAX ;	/*  Transmit and Receive 1024 to 1518 Byte Frame Counter R/W (0x25) */
	uint32_t TRMGV ;	/*  Transmit and Receive 1519 to 1522 Byte Good VLAN Frame Cnt R/W (0x26) */

	uint32_t RBYT ;		/* Receive   Byte Counter R/W (0x27) */
	uint32_t RPKT ;		/* Receive   Packet Counter R/W (0x28) */
	uint32_t RFCS ;		/* Receive   FCS Error Counter R/W (0x29) */
	uint32_t RMCA ;		/* Receive   Multicast Packet Counter R/W (0x2A) */
	uint32_t RBCA ;		/* Receive   Broadcast Packet Counter R/W (0x2B) */
	uint32_t RXCF ;		/* Receive   Control Frame Packet Counter R/W (0x2C) */
	uint32_t RXPF ;		/* Receive   PAUSE Frame Packet Counter R/W (0x2D) */
	uint32_t RXUO ;		/* Receive   Unknown OP code Counter R/W (0x2E) */
	uint32_t RALN ;		/* Receive   Alignment Error Counter R/W (0x2F) */
	uint32_t RFLR ;		/* Receive   Frame Length Error Counter R/W (0x30) */
	uint32_t RCDE ;		/* Receive   Code Error Counter R/W (0x31) */
	uint32_t RCSE ;		/* Receive   Carrier Sense Error Counter R/W (0x32) */
	uint32_t RUND ;		/* Receive   Undersize Packet Counter R/W (0x33) */
	uint32_t ROVR ;		/* Receive   Oversize Packet Counter R/W (0x34) */
	uint32_t RFRG ;		/* Receive   Fragments Counter R/W (0x35) */
	uint32_t RJBR ;		/* Receive   Jabber Counter R/W (0x36) */
	uint32_t RDRP ;		/* Receive   Drop R/W (0x37) */

	uint32_t TBYT ;		/*  Transmit  Byte Counter R/W (0x38) */
	uint32_t TPKT ;		/*  Transmit  Packet Counter R/W (0x39) */
	uint32_t TMCA ;		/*  Transmit  Multicast Packet Counter R/W (0x3A) */
	uint32_t TBCA ;		/*  Transmit  Broadcast Packet Counter R/W (0x3B) */
	uint32_t TXPF ;		/*  Transmit  PAUSE Control Frame Counter R/W (0x3C) */
	uint32_t TDFR ;		/*  Transmit  Deferral Packet Counter R/W (0x3D) */
	uint32_t TEDF ;		/*  Transmit  Excessive Deferral Packet Counter R/W (0x3E) */
	uint32_t TSCL ;		/*  Transmit  Single Collision Packet Counter R/W (0x3F) */
	uint32_t TMCL ;		/*  Transmit  Multiple Collision Packet Counter R/W (0x40) */
	uint32_t TLCL ;		/*  Transmit  Late Collision Packet Counter R/W (0x41) */
	uint32_t TXCL ;		/*  Transmit  Excessive Collision Packet Counter R/W (0x42) */
	uint32_t TNCL ;		/*  Transmit  Total Collision Counter R/W (0x43) */
	uint32_t TPFH ;		/*  Transmit  PAUSE Frames Honored Counter R/W (0x44) */
	uint32_t TDRP ;		/*  Transmit  Drop Frame Counter R/W (0x45) */
	uint32_t TJBR ;		/*  Transmit  Jabber Frame Counter R/W (0x46) */
	uint32_t TFCS ;		/*  Transmit  FCS Error Counter R/W (0x47) */
	uint32_t TXCF ;		/*  Transmit  Control Frame Counter R/W (0x48) */
	uint32_t TOVR ;		/*  Transmit  Oversize Frame Counter R/W (0x49) */
	uint32_t TUND ;		/*  Transmit  Undersize Frame Counter R/W (0x4A) */
	uint32_t TFRG ;		/*  Transmit  Fragments Frame Counter R/W (0x4B) */

	uint32_t CAR1 ;		/*  Carry Register One Register* RO (0x4C) */
	uint32_t CAR2 ;		/*  Carry Register Two Register* RO (0x4D) */
	uint32_t CAM1 ;		/*  Carry Register One Mask Register R/W (0x4E) */
	uint32_t CAM2 ;		/*  Carry Register Two Mask Register R/W (0x4F) */

	uint32_t MAC_ADDR0_LO ;		/*  low word 0x0000_0000 (0x50) */
	uint32_t MAC_ADDR0_HI ;		/*  high word 0x0000_0000 (0x51) */
	uint32_t MAC_ADDR1_LO ;		/*  low word 0x0000_0000 (0x52) */
	uint32_t MAC_ADDR1_HI ;		/*  high word 0x0000_0000 (0x53) */
	uint32_t MAC_ADDR2_LO ;		/*  low word 0x0000_0000 (0x54) */
	uint32_t MAC_ADDR2_HI ;		/*  high word 0x0000_0000 (0x55) */
	uint32_t MAC_ADDR3_LO ;		/*  low word 0x0000_0000 (0x56) */
	uint32_t MAC_ADDR3_HI ;		/*  high word 0x0000_0000 (0x57) */
	uint32_t MAC_ADDR_MASK0_LO ;		/*  low word 0x0000_0000 (0x58) */
	uint32_t MAC_ADDR_MASK0_HI ;		/*  high word 0x0000_0000 (0x59) */
	uint32_t MAC_ADDR_MASK1_LO ;		/*  low word 0x0000_0000 (0x5A) */
	uint32_t MAC_ADDR_MASK1_HI ;		/*  high word 0x0000_0000 (0x5B) */
	uint32_t MAC_FILTER_CONFIG ;		/*  Filter Configuration 0x0000_0000 (0x5C) */
	uint32_t HASH_TABLE_VECTOR_B31_0 ;		/*  TABLE VECTOR bits 31 to 0 (0x60) */
	uint32_t HASH_TABLE_VECTOR_B63_32 ;		/*  TABLE VECTOR bits 63 to 32 (0x61) */
	uint32_t HASH_TABLE_VECTOR_B95_64 ;		/*  TABLE VECTOR bits 95 to 64 (0x62) */
	uint32_t HASH_TABLE_VECTOR_B127_96 ;		/*  TABLE VECTOR bits 127 to 96 (0x63) */
	uint32_t HASH_TABLE_VECTOR_B159_128 ;		/*  TABLE VECTOR bits 159 to 128 (0x64) */
	uint32_t HASH_TABLE_VECTOR_B191_160 ;		/*  TABLE VECTOR bits 191 to 160 (0x65) */
	uint32_t HASH_TABLE_VECTOR_B223_192 ;		/*  TABLE VECTOR bits 223 to 192 (0x66) */
	uint32_t HASH_TABLE_VECTOR_B255_224 ;		/*  TABLE VECTOR bits 255 to 224 (0x67) */
	uint32_t HASH_TABLE_VECTOR_B287_256 ;		/*  TABLE VECTOR bits 287 to 256 (0x68) */
	uint32_t HASH_TABLE_VECTOR_B319_288 ;		/*  TABLE VECTOR bits 319 to 288 (0x69) */
	uint32_t HASH_TABLE_VECTOR_B351_320 ;		/*  TABLE VECTOR bits 351 to 320 (0x6A) */
	uint32_t HASH_TABLE_VECTOR_B383_352 ;		/*  TABLE VECTOR bits 383 to 352 (0x6B) */
	uint32_t HASH_TABLE_VECTOR_B415_384 ;		/*  TABLE VECTOR bits 415 to 384 (0x6C) */
	uint32_t HASH_TABLE_VECTOR_B447_416 ;		/*  TABLE VECTOR bits 447 to 416 (0x6D) */
	uint32_t HASH_TABLE_VECTOR_B479_448 ;		/*  TABLE VECTOR bits 479 to 448 (0x6E) */
	uint32_t HASH_TABLE_VECTOR_B511_480 ;		/*  TABLE VECTOR bits 511 to 480 (0x6F) */


	uint32_t TxControl;      /*  0x0A0  */
	uint32_t RxControl;      /*  0x0A1  */
	uint32_t DescPackCtrl;    /*  0x0A2  */
	uint32_t StatCtrl;        /*  0x0A3  */
	uint32_t L2AllocCtrl;     /*  0x0A4  */
	uint32_t IntMask;         /*  0x0A5  */
	uint32_t IntReg;          /*  0x0A6  */

	uint32_t CoreControl;     /*  0x0A8  */
	uint32_t ByteOffsets0;    /*  0x0A9  */
	uint32_t ByteOffsets1;    /*  0x0AA  */
	uint32_t PauseFrame;      /*  0x0AB  */
	uint32_t FIFOModeCtrl;    /*  0x0AF  */

        uint32_t L2Type;                        /*0x0F0*/


	/* Shared Address Space */

        uint32_t ParserConfig;                  /*0x100*/
        uint32_t ParseDepth;                    /*0x101*/
        uint32_t ExtractStringHashMask0;        /*0x102*/
        uint32_t ExtractStringHashMask1;        /*0x103*/
        uint32_t ExtractStringHashMask2;        /*0x104*/
        uint32_t ExtractStringHashMask3;        /*0x105*/
        uint32_t L3_L4_PROTO_MASK;              /*0x106*/

        uint32_t L3CTable;                      /*0x140*/

        uint32_t L4CTable;                      /*0x160*/

	uint32_t TranslateTable_0;            /*0x1A0*/
	uint32_t TranslateTable_1;            /*0x1A1*/

        uint32_t L3CTableExtraKeys;             /*0x1E0*/

        uint32_t L4_CTABLE_EXTRA_KEYS;          /*0x1E4*/

        uint32_t HASH7_BASE_MASK;               /*0x1E8*/

        uint32_t HASH7_BASE_MASK_CAM_KEYS;      /*0x1EC*/


        uint32_t DmaCr0;                        /*0x200*/
        uint32_t DmaCr1;                        /*0x201*/
        uint32_t DmaCr2;                        /*0x202*/
        uint32_t DmaCr3;                        /*0x203*/
        uint32_t RegFrInSpillMemStart0;                 /*0x204*/
        uint32_t RegFrInSpillMemStart1;                 /*0x205*/
        uint32_t RegFrInSpillMemSize;                   /*0x206*/
        uint32_t FrOutSpillMemStart0;                   /*0x207*/
        uint32_t FrOutSpillMemStart1;                   /*0x208*/
        uint32_t FrOutSpillMemSize;                     /*0x209*/
        uint32_t Class0SpillMemStart0;                  /*0x20A*/
        uint32_t Class0SpillMemStart1;                  /*0x20B*/
        uint32_t Class0SpillMemSize;                    /*0x20C*/
        uint32_t Class1SpillMemStart0;                  /*0x210*/
        uint32_t Class1SpillMemStart1;                  /*0x211*/
        uint32_t Class1SpillMemSize;                    /*0x212*/
        uint32_t Class2SpillMemStart0;                  /*0x213*/
        uint32_t Class2SpillMemStart1;                  /*0x214*/

        uint32_t Class2SpillMemSize;                    /*0x215*/
        uint32_t Class3SpillMemStart0;                  /*0x216*/
        uint32_t Class3SpillMemStart1;                  /*0x217*/
        uint32_t Class3SpillMemSize;                    /*0x218*/
        uint32_t RegFrIn1SpillMemStart0;                /*0x219*/
        uint32_t RegFrIn1SpillMemStart1;                /*0x21A*/
        uint32_t RegFrIn1SpillMemSize;                  /*0x21B*/

	uint32_t FreeQCarve;                            /*0x233*/

        uint32_t XLR_REG_XGS_DBG_DESC_WORD_CNT;          /* 0x23C */
        uint32_t XLR_REG_XGS_DBG_DESC_WORD_CNT_SEL;         /* 0x23D */
        uint32_t ClassWatermarks;                       /*0x244*/
        uint32_t RxWatermarks0;                 /*0x245*/
        uint32_t RxWatermarks1;                 /*0x246*/
        uint32_t RxWatermarks2;                 /*0x247*/
        uint32_t RxWatermarks3;                 /*0x248*/
        uint32_t FreeWatermarks;                /*0x249*/

        uint32_t PDE_CLASS0_0;                  /*0x300*/
        uint32_t PDE_CLASS0_1;                  /*0x301*/
        uint32_t PDE_CLASS1_0;                  /*0x302*/
        uint32_t PDE_CLASS1_1;                  /*0x303*/
        uint32_t PDE_CLASS2_0;                  /*0x304*/
        uint32_t PDE_CLASS2_1;                  /*0x305*/
        uint32_t PDE_CLASS3_0;                  /*0x306*/
        uint32_t PDE_CLASS3_1;                  /*0x307*/

	uint32_t G_RFR0_BUCKET_SIZE; 		/*0x321*/  
	uint32_t G_TX0_BUCKET_SIZE; 		/*0x322*/ 
	uint32_t G_TX1_BUCKET_SIZE; 		/*0x323*/
	uint32_t G_TX2_BUCKET_SIZE; 		/*0x324*/
	uint32_t G_TX3_BUCKET_SIZE; 		/*0x325*/

	uint32_t G_RFR1_BUCKET_SIZE; 		/*0x327*/


	uint32_t CC_CPU0_0; 		/*0x380*/
	uint32_t CC_CPU1_0; 		/*0x388*/
	uint32_t CC_CPU2_0; 		/*0x390*/
	uint32_t CC_CPU3_0; 		/*0x398*/
	uint32_t CC_CPU4_0; 		/*0x3a0*/
	uint32_t CC_CPU5_0; 		/*0x3a8*/
	uint32_t CC_CPU6_0; 		/*0x3b0*/
	uint32_t CC_CPU7_0; 		/*0x3b8*/


};

#endif
