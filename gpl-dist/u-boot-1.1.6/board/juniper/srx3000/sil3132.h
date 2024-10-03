/*
 * Copyright (C) Freescale Semiconductor, Inc. 2006. All rights reserved.
 * Author: Jason Jin<Jason.jin@freescale.com>
 *         Zhang Wei<wei.zhang@freescale.com>
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
 *
 */
#ifndef _SIL3131_H_
#define _SIL3131_H_

#include "pcie.h"

#define SIL3132_PCI_BAR0		0x10
#define SIL3132_PCI_BAR2		0x18
#define AHCI_MAX_SG		56 /* hardware max is 64K */
#define AHCI_CMD_SLOT_SZ	32
#define AHCI_RX_FIS_SZ		256
#define AHCI_CMD_TBL_HDR	0x80
#define AHCI_CMD_TBL_CDB	0x40
#define AHCI_CMD_TBL_SZ		AHCI_CMD_TBL_HDR + (AHCI_MAX_SG * 16)
#define AHCI_PORT_PRIV_DMA_SZ	AHCI_CMD_SLOT_SZ + AHCI_CMD_TBL_SZ	\
								+ AHCI_RX_FIS_SZ
#define AHCI_CMD_ATAPI		(1 << 5)
#define AHCI_CMD_WRITE		(1 << 6)
#define AHCI_CMD_PREFETCH	(1 << 7)
#define AHCI_CMD_RESET		(1 << 8)
#define AHCI_CMD_CLR_BUSY	(1 << 10)

#define RX_FIS_D2H_REG		0x40	/* offset of D2H Register FIS data */

/* Global controller registers */
#define HOST_CAP		0x00 /* host capabilities */
#define HOST_CTL		0x04 /* global host control */
#define HOST_IRQ_STAT		0x08 /* interrupt status */
#define HOST_PORTS_IMPL		0x0c /* bitmap of implemented ports */
#define HOST_VERSION		0x10 /* AHCI spec. version compliancy */

/* HOST_CTL bits */
#define HOST_RESET		(1 << 0)  /* reset controller; self-clear */
#define HOST_IRQ_EN		(1 << 1)  /* global IRQ enable */
#define HOST_AHCI_EN		(1 << 31) /* AHCI enabled */

/* Registers for each SATA port */
#define PORT_LST_ADDR		0x00 /* command list DMA addr */
#define PORT_LST_ADDR_HI	0x04 /* command list DMA addr hi */
#define PORT_FIS_ADDR		0x08 /* FIS rx buf addr */
#define PORT_FIS_ADDR_HI	0x0c /* FIS rx buf addr hi */
#define PORT_IRQ_STAT		0x10 /* interrupt status */
#define PORT_IRQ_MASK		0x14 /* interrupt enable/disable mask */
#define PORT_CMD		0x18 /* port command */
#define PORT_TFDATA		0x20 /* taskfile data */
#define PORT_SIG		0x24 /* device TF signature */
#define PORT_CMD_ISSUE		0x38 /* command issue */
#define PORT_SCR		0x28 /* SATA phy register block */
#define PORT_SCR_STAT		0x28 /* SATA phy register: SStatus */
#define PORT_SCR_CTL		0x2c /* SATA phy register: SControl */
#define PORT_SCR_ERR		0x30 /* SATA phy register: SError */
#define PORT_SCR_ACT		0x34 /* SATA phy register: SActive */

/* PORT_IRQ_{STAT,MASK} bits */
#define PORT_IRQ_COLD_PRES	(1 << 31) /* cold presence detect */
#define PORT_IRQ_TF_ERR		(1 << 30) /* task file error */
#define PORT_IRQ_HBUS_ERR	(1 << 29) /* host bus fatal error */
#define PORT_IRQ_HBUS_DATA_ERR	(1 << 28) /* host bus data error */
#define PORT_IRQ_IF_ERR		(1 << 27) /* interface fatal error */
#define PORT_IRQ_IF_NONFATAL	(1 << 26) /* interface non-fatal error */
#define PORT_IRQ_OVERFLOW	(1 << 24) /* xfer exhausted available S/G */
#define PORT_IRQ_BAD_PMP	(1 << 23) /* incorrect port multiplier */

#define PORT_IRQ_PHYRDY		(1 << 22) /* PhyRdy changed */
#define PORT_IRQ_DEV_ILCK	(1 << 7) /* device interlock */
#define PORT_IRQ_CONNECT	(1 << 6) /* port connect change status */
#define PORT_IRQ_SG_DONE	(1 << 5) /* descriptor processed */
#define PORT_IRQ_UNK_FIS	(1 << 4) /* unknown FIS rx'd */
#define PORT_IRQ_SDB_FIS	(1 << 3) /* Set Device Bits FIS rx'd */
#define PORT_IRQ_DMAS_FIS	(1 << 2) /* DMA Setup FIS rx'd */
#define PORT_IRQ_PIOS_FIS	(1 << 1) /* PIO Setup FIS rx'd */
#define PORT_IRQ_D2H_REG_FIS	(1 << 0) /* D2H Register FIS rx'd */

#define PORT_IRQ_FATAL		PORT_IRQ_TF_ERR | PORT_IRQ_HBUS_ERR 	\
				| PORT_IRQ_HBUS_DATA_ERR | PORT_IRQ_IF_ERR

#define DEF_PORT_IRQ		PORT_IRQ_FATAL | PORT_IRQ_PHYRDY 	\
				| PORT_IRQ_CONNECT | PORT_IRQ_SG_DONE 	\
				| PORT_IRQ_UNK_FIS | PORT_IRQ_SDB_FIS 	\
				| PORT_IRQ_DMAS_FIS | PORT_IRQ_PIOS_FIS	\
				| PORT_IRQ_D2H_REG_FIS

/* PORT_CMD bits */
#define PORT_CMD_ATAPI		(1 << 24) /* Device is ATAPI */
#define PORT_CMD_LIST_ON	(1 << 15) /* cmd list DMA engine running */
#define PORT_CMD_FIS_ON		(1 << 14) /* FIS DMA engine running */
#define PORT_CMD_FIS_RX		(1 << 4) /* Enable FIS receive DMA engine */
#define PORT_CMD_CLO		(1 << 3) /* Command list override */
#define PORT_CMD_POWER_ON	(1 << 2) /* Power up device */
#define PORT_CMD_SPIN_UP	(1 << 1) /* Spin up device */
#define PORT_CMD_START		(1 << 0) /* Enable port DMA engine */

#define PORT_CMD_ICC_ACTIVE	(0x1 << 28) /* Put i/f in active state */
#define PORT_CMD_ICC_PARTIAL	(0x2 << 28) /* Put i/f in partial state */
#define PORT_CMD_ICC_SLUMBER	(0x6 << 28) /* Put i/f in slumber state */

/* SETFEATURES stuff */
#define SETFEATURES_XFER	0x03
#define XFER_UDMA_7		0x47
#define XFER_UDMA_6		0x46
#define XFER_UDMA_5		0x45
#define XFER_UDMA_4		0x44
#define XFER_UDMA_3		0x43
#define XFER_UDMA_2		0x42
#define XFER_UDMA_1		0x41
#define XFER_UDMA_0		0x40
#define XFER_MW_DMA_2		0x22
#define XFER_MW_DMA_1		0x21
#define XFER_MW_DMA_0		0x20
#define XFER_SW_DMA_2		0x12
#define XFER_SW_DMA_1		0x11
#define XFER_SW_DMA_0		0x10
#define XFER_PIO_4		0x0C
#define XFER_PIO_3		0x0B
#define XFER_PIO_2		0x0A
#define XFER_PIO_1		0x09
#define XFER_PIO_0		0x08
#define XFER_PIO_SLOW		0x00

#define ATA_FLAG_SATA		(1 << 3)
#define ATA_FLAG_NO_LEGACY	(1 << 4) /* no legacy mode check */
#define ATA_FLAG_MMIO		(1 << 6) /* use MMIO, not PIO */
#define ATA_FLAG_SATA_RESET	(1 << 7) /* (obsolete) use COMRESET */
#define ATA_FLAG_PIO_DMA	(1 << 8) /* PIO cmds via DMA */
#define ATA_FLAG_NO_ATAPI	(1 << 11) /* No ATAPI support */


/* SATA register defines */
#define         ATA_SS_DET_MASK         0x0000000f
#define         ATA_SS_DET_NO_DEVICE    0x00000000
#define         ATA_SS_DET_DEV_PRESENT  0x00000001
#define         ATA_SS_DET_PHY_ONLINE   0x00000003
#define         ATA_SS_DET_PHY_OFFLINE  0x00000004

#define         ATA_SS_SPD_MASK         0x000000f0
#define         ATA_SS_SPD_NO_SPEED     0x00000000
#define         ATA_SS_SPD_GEN1         0x00000010
#define         ATA_SS_SPD_GEN2         0x00000020

#define         ATA_SS_IPM_MASK         0x00000f00
#define         ATA_SS_IPM_NO_DEVICE    0x00000000
#define         ATA_SS_IPM_ACTIVE       0x00000100
#define         ATA_SS_IPM_PARTIAL      0x00000200
#define         ATA_SS_IPM_SLUMBER      0x00000600

#define         ATA_SS_CONWELL_MASK \
			(ATA_SS_DET_MASK|ATA_SS_SPD_MASK|ATA_SS_IPM_MASK)
#define         ATA_SS_CONWELL_GEN1 \
			(ATA_SS_DET_PHY_ONLINE|ATA_SS_SPD_GEN1|ATA_SS_IPM_ACTIVE)
#define         ATA_SS_CONWELL_GEN2 \
			(ATA_SS_DET_PHY_ONLINE|ATA_SS_SPD_GEN2|ATA_SS_IPM_ACTIVE)

#define         ATA_SE_DATA_CORRECTED   0x00000001
#define         ATA_SE_COMM_CORRECTED   0x00000002
#define         ATA_SE_DATA_ERR         0x00000100
#define         ATA_SE_COMM_ERR         0x00000200
#define         ATA_SE_PROT_ERR         0x00000400
#define         ATA_SE_HOST_ERR         0x00000800
#define         ATA_SE_PHY_CHANGED      0x00010000
#define         ATA_SE_PHY_IERROR       0x00020000
#define         ATA_SE_COMM_WAKE        0x00040000
#define         ATA_SE_DECODE_ERR       0x00080000
#define         ATA_SE_PARITY_ERR       0x00100000
#define         ATA_SE_CRC_ERR          0x00200000
#define         ATA_SE_HANDSHAKE_ERR    0x00400000
#define         ATA_SE_LINKSEQ_ERR      0x00800000
#define         ATA_SE_TRANSPORT_ERR    0x01000000
#define         ATA_SE_UNKNOWN_FIS      0x02000000

#define         ATA_SC_DET_MASK         0x0000000f
#define         ATA_SC_DET_IDLE         0x00000000
#define         ATA_SC_DET_RESET        0x00000001
#define         ATA_SC_DET_DISABLE      0x00000004

#define         ATA_SC_SPD_MASK         0x000000f0
#define         ATA_SC_SPD_NO_SPEED     0x00000000
#define         ATA_SC_SPD_SPEED_GEN1   0x00000010
#define         ATA_SC_SPD_SPEED_GEN2   0x00000020

#define         ATA_SC_IPM_MASK         0x00000f00
#define         ATA_SC_IPM_NONE         0x00000000
#define         ATA_SC_IPM_DIS_PARTIAL  0x00000100
#define         ATA_SC_IPM_DIS_SLUMBER  0x00000200

#define         SIL3132_PORT_PRIV_DMA_SZ    0x2048
/* DMA register defines */
#define ATA_DMA_ENTRIES                 256
#define ATA_DMA_EOT                     0x80000000
#define ATA_DMA_LNK                     0x40000000
#define ATA_SA150                       0x47
#define ATA_A_4BIT                      0x08    /* 4 head bits */
#define ATA_F_DMA                       0x01    /* enable DMA */
#define SIL3132_MAX_PORTS		        2
#define SIL3132_MAX_SGE		            2

#define WIN_SMART                       0xB0 /* self-monitoring and reporting */

/* WIN_SMART sub-commands */
#define SMART_READ_VALUES               0xD0
#define SMART_READ_THRESHOLDS           0xD1
#define SMART_AUTOSAVE                  0xD2
#define SMART_SAVE                      0xD3
#define SMART_IMMEDIATE_OFFLINE         0xD4
#define SMART_READ_LOG_SECTOR           0xD5
#define SMART_WRITE_LOG_SECTOR          0xD6
#define SMART_WRITE_THRESHOLDS          0xD7
#define SMART_ENABLE                    0xD8
#define SMART_DISABLE                   0xD9
#define SMART_STATUS                    0xDA
#define SMART_AUTO_OFFLINE              0xDB

#define __packed    __attribute__ ((packed))

struct ata_siiprb_dma_prdentry {
	u_int64_t addr;
	u_int32_t count;
	u_int32_t control;
} __packed;

struct ata_siiprb_ata_command {
	u_int32_t reserved0;
	struct ata_siiprb_dma_prdentry prd[SIL3132_MAX_SGE];
} __packed;

struct ata_siiprb_atapi_command {
	u_int8_t cdb[16];
	struct ata_siiprb_dma_prdentry prd[SIL3132_MAX_SGE-1];
} __packed;

struct ata_siiprb_command {
	u_int16_t control;
	u_int16_t protocol_override;
	u_int32_t transfer_count;
	u_int8_t fis[20];
	union {
	struct ata_siiprb_ata_command ata;
	struct ata_siiprb_atapi_command atapi;
	} u;
} __packed;

struct sil3132_ioports {
	u32	cmd_addr;
	u32	scr_addr;
	u32	port_mmio;
	struct ata_siiprb_command	*cmd_slot;
};

struct sil3132_probe_ent {
	pcie_dev_t 	dev;
	struct sil3132_ioports	port[SIL3132_MAX_PORTS];
	u32	n_ports;
	u32	hard_port_no;
	u32	host_flags;
	u32	host_set_flags;
	u32	gbl_reg_base;
	u32	port_reg_base;
	u32     pio_mask;
	u32	udma_mask;
	u32	flags;
	u32	cap;	/* cache of HOST_CAP register */
	u32	port_map; /* cache of HOST_PORTS_IMPL reg */
	u32	link_port_map; /*linkup port map*/
};

#endif /* _SIL3131_H_ */
