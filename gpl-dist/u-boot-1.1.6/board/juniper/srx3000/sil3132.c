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
 * with the reference on libata and ahci drvier in kernel
 *
 */

/* #define DEBUG   1 */
#include <common.h>

#ifdef CONFIG_SRX3000

#include <command.h>
#include <pci.h>
#include <asm/processor.h>
#include <asm/errno.h>
#include <asm/io.h>
#include <malloc.h>
#include <scsi.h>
#include <ata.h>
#include <linux/ctype.h>
#include "sil3132.h"

#define ATA_OUTL(base, reg, value) \
	writel(value, base + reg)
#define ATA_INL(base, reg) \
	readl(base + reg)

	/* set the SATA resources */
#define ATA_IDX_INL_ATA_SSTATUS(mmio, port) \
	readl(mmio + 0x1f04 + 0x2000 * port)

#define ATA_IDX_INL_ATA_SERROR(mmio, port) \
	readl(mmio + 0x1f08 + 0x2000 * port)

#define ATA_IDX_INL_ATA_SCONTROL(mmio, port) \
	readl(mmio + 0x1f00 + 0x2000 * port)

#define ATA_IDX_INL_ATA_SACTIVE(mmio, port) \
	readl(mmio + 0x1f0c + 0x2000 * port)

#define ATA_IDX_OUTL_ATA_SCONTROL(mmio, port, value) \
	writel(value, mmio + 0x1f00 + 0x2000 * port)

#define ATA_IDX_OUTL_ATA_SACTIVE(mmio, port, value) \
	writel(value, mmio + 0x1f0c + 0x2000 * port)

#define ATA_IDX_OUTL_ATA_SERROR(mmio, port, value) \
	writel(value, mmio + 0x1f08 + 0x2000 * port)

#define le16toh(x) le16_to_cpu(x)
#define htole16(x) cpu_to_le16(x)
#define le32toh(x) le32_to_cpu(x)
#define htole32(x) cpu_to_le32(x)
#define le64toh(x) le64_to_cpu(x)
#define htole64(x) cpu_to_le64(x)

struct sil3132_probe_ent *probe_ent = NULL;
static struct sil3132_probe_ent probe_ent_local;

static hd_driveid_t sata_ataid[SIL3132_MAX_PORTS];

struct sil3132_cmd_slot {
	u8	reserv[16];
	struct ata_siiprb_command	cmd_slot;
};

static struct sil3132_cmd_slot sil3132_cmd_slot[SIL3132_MAX_PORTS];

#define msleep(a) udelay(a * 1000)
#define ssleep(a) msleep(a * 1000)
#define ATA_POLLING_TIMEOUT 2000

#define SIL3132_ISSUE_CMD() \
	do {    \
		asm volatile ("sync"); \
		prb_bus = (u_int64_t)prb; \
		ATA_OUTL(port_mmio, 0x1c00, prb_bus); \
		ATA_OUTL(port_mmio, 0x1c04, prb_bus>>32); \
	} while (0)


static char *
ata_cmd2str (struct ata_siiprb_command *prb)
{
	static char buffer[20];
	switch (prb->fis[2]) {
	case 0x00: return ("NOP");
	case 0x08: return ("DEVICE_RESET");
	case 0x20: return ("READ");
	case 0x24: return ("READ48");
	case 0x25: return ("READ_DMA48");
	case 0x26: return ("READ_DMA_QUEUED48");
	case 0x29: return ("READ_MUL48");
	case 0x30: return ("WRITE");
	case 0x34: return ("WRITE48");
	case 0x35: return ("WRITE_DMA48");
	case 0x36: return ("WRITE_DMA_QUEUED48");
	case 0x39: return ("WRITE_MUL48");
	case 0x70: return ("SEEK");
	case 0xa0: return ("PACKET_CMD");
	case 0xa1: return ("ATAPI_IDENTIFY");
	case 0xa2: return ("SERVICE");
	case 0xc0: return ("CFA ERASE");
	case 0xc4: return ("READ_MUL");
	case 0xc5: return ("WRITE_MUL");
	case 0xc6: return ("SET_MULTI");
	case 0xc7: return ("READ_DMA_QUEUED");
	case 0xc8: return ("READ_DMA");
	case 0xca: return ("WRITE_DMA");
	case 0xcc: return ("WRITE_DMA_QUEUED");
	case 0xe2: return ("STANDBY");
	case 0xe6: return ("SLEEP");
	case 0xe7: return ("FLUSHCACHE");
	case 0xea: return ("FLUSHCACHE48");
	case 0xec: return ("ATA_IDENTIFY");
	case 0xef:
		switch (prb->fis[3]) {
		case 0x03: return ("SETFEATURES SET TRANSFER MODE");
		case 0x02: return ("SETFEATURES ENABLE WCACHE");
		case 0x05: return ("SETFEATURES ENABLE APM");
		case 0x82: return ("SETFEATURES DISABLE WCACHE");
		case 0xaa: return ("SETFEATURES ENABLE RCACHE");
		case 0x55: return ("SETFEATURES DISABLE RCACHE");
		}
		sprintf(buffer, "SETFEATURES 0x%02x", prb->fis[3]);
		return buffer;
	}
	sprintf(buffer, "unknown CMD (0x%02x)", prb->fis[2]);
	return buffer;
}

static int waiting_for_cmd_completed (volatile u8 *mmio,
					 struct ata_siiprb_command *prb,
					 int timeout_msec,
					 u32 sign)
{
	int i;
	u32 status, error;

	status = readl(mmio + 0x1008);
	debug("mmio %08x interrupt statue = %08x\n", mmio, status);
	msleep(1);
	for (i = 0; (!(status && (sign << 16))) && i < timeout_msec; i++)
	{
		msleep(1);
		status = readl(mmio + 0x1008);
	}
	
	debug("mmio %08x interrupt statue = %08x timeout=%dms\n", mmio, status, i);
	msleep(1);
	/* clean interrupt */
	writel(sign, mmio + 0x1008);

	/* any controller errors flagged ? */
	if ((error = ATA_INL(mmio, 0x1024))) {
	debug("ata_transaction %s error=%d\n",
		   ata_cmd2str(prb), error);
	} else {
	debug("ata_transaction %s sucess\n",
		   ata_cmd2str(prb));

	}
	msleep(1);
	return (i < timeout_msec) ? 0 : -1;
}

static int sil3132_host_init (struct sil3132_probe_ent *probe_ent)
{
	volatile u8 *mmio = (volatile u8 *)(probe_ent->gbl_reg_base);
	
	/* reset controller */
	writel(0x80000000, mmio + 0x0040);
	ssleep(1);
	writel(0x0000000f, mmio + 0x0040);

	return 0;
}

static int sil3132_chipinit (pcie_dev_t pdev)
{
	u32 iobase;
	int rc;

	memset((void *)sata_ataid, 0, sizeof(hd_driveid_t) * SIL3132_MAX_PORTS);

	probe_ent = &probe_ent_local;
	pcie_read_config_dword(pdev, SIL3132_PCI_BAR0, &iobase);
	iobase &= ~0xf;
	probe_ent->gbl_reg_base = iobase;

	pcie_read_config_dword(pdev, SIL3132_PCI_BAR2, &iobase);
	iobase &= ~0xf;
	probe_ent->port_reg_base = iobase;

	debug("global = %8x port = %8x \n", 
		probe_ent->gbl_reg_base,
		probe_ent->port_reg_base);
	msleep(1);    
	probe_ent->host_flags = ATA_FLAG_SATA
				| ATA_FLAG_NO_LEGACY
				| ATA_FLAG_MMIO
				| ATA_FLAG_PIO_DMA
				| ATA_FLAG_NO_ATAPI;
	probe_ent->pio_mask = 0x1f;
	probe_ent->udma_mask = 0x7f;	/*Fixme,assume to support UDMA6 */

	/* initialize adapter */
	rc = sil3132_host_init(probe_ent);
	if (rc)
		goto err_out;

	return 0;

err_out:
	return rc;
}


#define MAX_DATA_BYTE_COUNT  (4*1024*1024)

static int sil3132_fill_sg (u8 port, unsigned char *buf, int buf_len)
{
	struct sil3132_ioports *pp = &(probe_ent->port[port]);
	struct ata_siiprb_dma_prdentry *sil3132_sg = pp->cmd_slot->u.ata.prd;
	u32 sg_count;
	int i;

	sg_count = ((buf_len - 1) / MAX_DATA_BYTE_COUNT) + 1;
	if (sg_count > SIL3132_MAX_SGE) {
		printf("Error:Too much sg!\n");
		return -1;
	}
	
	for (i = 0; i < sg_count; i++) {
		sil3132_sg[i].addr =
			cpu_to_le64((u32) buf + i * MAX_DATA_BYTE_COUNT);
		sil3132_sg[i].count= cpu_to_le32((buf_len < MAX_DATA_BYTE_COUNT
					   ? (buf_len)
					   : (MAX_DATA_BYTE_COUNT)));
		sil3132_sg[i].control = htole32(0);
		buf_len -= MAX_DATA_BYTE_COUNT;
	}
	sil3132_sg[i-1].control = htole32(ATA_DMA_EOT);

	return sg_count;
}

static void sil3132_set_feature (u8 port)
{
	struct sil3132_ioports *pp = &(probe_ent->port[port]);
	volatile u8 *port_mmio = (volatile u8 *)pp->port_mmio;
	struct ata_siiprb_command *prb;
	u_int64_t prb_bus;
	u_int8_t *fis;

	prb = pp->cmd_slot;
	fis = prb->fis;

	/*set feature */
	memset(fis, 0, 20);
	fis[0] = 0x27;
	fis[1] = 0x80;
	fis[2] = ATA_CMD_SETF;
	fis[3] = SETFEATURES_XFER;
	fis[12] = ATA_SA150;

	SIL3132_ISSUE_CMD();

	if (waiting_for_cmd_completed(port_mmio, prb, 1500, 0x1)) {
		printf("set feature error!\n");
	}
}


static int
ata_sata_connect (int port)
{
	volatile u8 *port_mmio = (volatile u8 *)probe_ent->port_reg_base;
	u_int32_t status;
	int timeout;

	/* wait up to 1 second for "connect well" */
	for (timeout = 0; timeout < 100 ; timeout++) {
	status = ATA_IDX_INL_ATA_SSTATUS(port_mmio, port);
	if ((status & ATA_SS_CONWELL_MASK) == ATA_SS_CONWELL_GEN1 ||
		(status & ATA_SS_CONWELL_MASK) == ATA_SS_CONWELL_GEN2)
		break;
		msleep(10);
	}
	if (timeout >= 100) {
		printf("SATA connect status=%08x\n", status);
		return 0;
	}

	printf("SATA connect time=%dms\n", timeout * 10);

	/* clear SATA error register */
	ATA_IDX_OUTL_ATA_SERROR(port_mmio, port, ATA_IDX_INL_ATA_SERROR(port_mmio, port));

	return 1;
}


static int
ata_sata_phy_reset (int port)
{
	volatile u8 *port_mmio = (volatile u8 *)probe_ent->port_reg_base;
	int loop, retry;
	u_int32_t spd;

	if (port == 0) {
		spd = ATA_SC_SPD_SPEED_GEN1;
	} else {
		spd = ATA_SC_SPD_NO_SPEED;
	}

	for (retry = 0; retry < 10; retry++) {
		for (loop = 0; loop < 10; loop++) {
			ATA_IDX_OUTL_ATA_SCONTROL(port_mmio, port, ATA_SC_DET_RESET | spd);
			udelay(100);
			if ((ATA_IDX_INL_ATA_SCONTROL(port_mmio, port) &
		 		ATA_SC_DET_MASK) == ATA_SC_DET_RESET)
			break;
		}
		udelay(5000);
		for (loop = 0; loop < 10; loop++) {
			ATA_IDX_OUTL_ATA_SCONTROL(port_mmio, port, ATA_SC_DET_IDLE |
						   ATA_SC_IPM_DIS_PARTIAL |
						   ATA_SC_IPM_DIS_SLUMBER | spd);
			udelay(100);
			if ((ATA_IDX_INL_ATA_SCONTROL(port_mmio, port) & ATA_SC_DET_MASK) == 0)
				return ata_sata_connect(port);
		}
	}
	return 0;
}

static int
ata_sata_start_port (int port)
{
	struct sil3132_ioports *pp = &(probe_ent->port[port]);
	u32 mem;

	debug("Enter start port: %d\n", port);
	msleep(1);
	mem = (u32) (&sil3132_cmd_slot[port]);
	mem = (mem + 0x10) & (~0xf);	/* Aligned to 16-bytes */
	memset((u8 *) mem, 0, sizeof(struct ata_siiprb_command));

	/*
	 * First item in chunk of DMA memory: 32-slot command table,
	 * 32 bytes each in size
	 */
	pp->cmd_slot = (struct ata_siiprb_command  *)mem;
	debug("port = %d - cmd_slot = 0x%x, pp = 0x%x, probe_ent = 0x%x\n", 
		port, pp->cmd_slot, pp, probe_ent);

	debug("Exit start port %d\n", port);
	msleep(1);
	return (0);
}

static int sil3132_port_start (u8 port)
{
	struct sil3132_ioports *pp = &(probe_ent->port[port]);
	volatile u8 *port_mmio = (volatile u8 *)probe_ent->port_reg_base;

	int offset = port * 0x2000;
	int timeout;
	struct ata_siiprb_command *prb;
	u_int64_t prb_bus;
	u_int32_t status, signature;

	port_mmio += offset;

	/* reset channel HW */
	writel(0x00000001, port_mmio + 0x1000);
	ssleep(1);
	ATA_OUTL(port_mmio, 0x1004, 0x00000001);
	ssleep(1);

	/* poll for channel ready */
	for (timeout = 0; timeout < 1000; timeout++) {
		if ((status = ATA_INL(port_mmio, 0x1000)) & 0x00040000)
			break;
		msleep(1);
	}
	
	if (timeout >= 1000) {
		printf("channel HW reset timeout reset failure\n");
		return (-1);
	}

	printf("channel HW reset time=%dms\n", timeout * 1);

	/* reset phy */
	if (!ata_sata_phy_reset(port)) {
		printf("phy reset found no device on port %d\n", port);
		return(-1);
	}

	printf("sata phy reset found device on port %d\n", port);
	ssleep(1);

	ata_sata_start_port(port);
	
	pp->port_mmio = port_mmio;
	
	/* get a piece of the workspace for a soft reset request */
	prb = (struct ata_siiprb_command *)pp->cmd_slot;
	prb_bus = (u_int64_t)prb;

	memset((u8*)prb, 0, sizeof(struct ata_siiprb_command));
	prb->control = htole16(0x0080);

	/* activate the soft reset prb */
	SIL3132_ISSUE_CMD();

	/* poll for channel ready */
	for (timeout = 0; timeout < ATA_POLLING_TIMEOUT; timeout++) {
		msleep(10);
		if ((status = ATA_INL(port_mmio, 0x1008)) & 0x00010000)
			break;
	}
	if (timeout >= ATA_POLLING_TIMEOUT) {
		printf("reset timeout - no device found\n");
		return (-1);
	}

	debug("soft reset exec time=%dms status=%08x\n",
			timeout, status);
	msleep(1);

	prb = (struct ata_siiprb_command *)(port_mmio);
	signature =
		prb->fis[12]|(prb->fis[4]<<8)|(prb->fis[5]<<16)|(prb->fis[6]<<24);
	printf("signature=%08x\n", signature);
	msleep(1);

	switch (signature) {
	case 0xeb140101:
		printf("SATA ATAPI devices not supported yet\n");
		break;
	case 0x96690101:
		printf("Portmultipliers not supported yet\n");
		break;
	case 0x00000101:
		printf("SATA Master is supported\n");
		break;
	default:
		break;
	}
	/* clear interrupt(s) */
	ATA_OUTL(port_mmio, 0x1008, 0x000008ff);

	/* require explicit interrupt ack */
	ATA_OUTL(port_mmio, 0x1000, 0x00000008);

	/* 64bit mode */
	ATA_OUTL(port_mmio, 0x1004, 0x00000400);

	debug("Exit start port %d\n", port);
	msleep(1);

	return 0;
}


static int get_sil3132_device_data (u8 port, u8 *buf, int buf_len)
{

	struct sil3132_ioports *pp = &(probe_ent->port[port]);
	volatile u8 *port_mmio = (volatile u8 *)pp->port_mmio;
	int sg_count;
	u_int64_t prb_bus;
	struct ata_siiprb_command *prb = pp->cmd_slot;

	debug("Enter get_ahci_device_data: for port %d cmd_slot = %08x\n", port, (u32) pp->cmd_slot);
	msleep(1);
	
	if (port > 2) {
		printf("Invaild port number %d\n", port);
		return -1;
	}

	sg_count = sil3132_fill_sg(port, buf, buf_len);

	SIL3132_ISSUE_CMD();
	
	if (waiting_for_cmd_completed(port_mmio, prb, 1500, 0x1)) {
		printf("timeout exit!\n");
		return -1;
	}

	prb = (struct ata_siiprb_command *)port_mmio;
	debug("get_ahci_device_data: %d byte transferred.\n",
		  le32toh(prb->transfer_count));
	msleep(1);

	return 0;
}


static char *ata_id_strcpy (u16 *target, u16 *src, int len)
{
	int i;
	for (i = 0; i < len / 2; i++)
		target[i] = le16_to_cpu(src[i]);
	return (char *)target;
}


static void dump_ataid (hd_driveid_t *ataid)
{
	debug("(47)ataid->max_multsect = 0x%x\n", (ataid->max_multsect << 8 )|ataid->vendor3);
	debug("(49)ataid->capability = 0x%x\n", ataid->capability);
	debug("(53)ataid->field_valid =0x%x\n", ataid->field_valid);
	debug("(59)ataid->multsect = 0x%x\n", (ataid->multsect << 8) | ataid->multsect_valid);
	debug("(63)ataid->dma_mword = 0x%x\n", ataid->dma_mword);
	debug("(64)ataid->eide_pio_modes = 0x%x\n", ataid->eide_pio_modes);
	debug("(75)ataid->queue_depth = 0x%x\n", ataid->queue_depth);
	debug("(80)ataid->major_rev_num = 0x%x\n", ataid->major_rev_num);
	debug("(81)ataid->minor_rev_num = 0x%x\n", ataid->minor_rev_num);
	debug("(82)ataid->command_set_1 = 0x%x\n", ataid->command_set_1);
	debug("(83)ataid->command_set_2 = 0x%x\n", ataid->command_set_2);
	debug("(84)ataid->cfsse = 0x%x\n", ataid->cfsse);
	debug("(85)ataid->cfs_enable_1 = 0x%x\n", ataid->cfs_enable_1);
	debug("(86)ataid->cfs_enable_2 = 0x%x\n", ataid->cfs_enable_2);
	debug("(87)ataid->csf_default = 0x%x\n", ataid->csf_default);
	debug("(88)ataid->dma_ultra = 0x%x\n", ataid->dma_ultra);
	debug("(93)ataid->hw_config = 0x%x\n", ataid->hw_config);
	msleep(10);
}

/*
 * SCSI INQUIRY command operation.
 */
static int ata_sataop_inquiry (ccb *pccb)
{
	int port = pccb->target;
	struct sil3132_ioports *pp = &(probe_ent->port[port]);
	struct ata_siiprb_command *prb = pp->cmd_slot;
	u_int8_t *fis = prb->fis;
	u8 hdr[] = {
		0,
		0,
		0x5,		/* claim SPC-3 version compatibility */
		2,
		95 - 4,
	};
	u8 *tmpid;

	if (pp->cmd_slot == NULL) {
		printf("port %d not started \n", port);
		return -EIO;
	}

	debug("inquiry port = %d\n", port);
	msleep(1);
	
	/* Clean ccb data buffer */
	memset(pccb->pdata, 0, pccb->datalen);
	memset((u8*) prb, 0, sizeof(struct ata_siiprb_command));
	memcpy(pccb->pdata, hdr, sizeof(hdr));

   
	if (pccb->datalen <= 35)
		return 0;

	memset(fis, 0, 20);
	prb->control = 0;

	/* Construct the FIS */
	fis[0] = 0x27;		/* Host to device FIS. */
	fis[1] = 0x80;	/* Command FIS. */
	fis[2] = ATA_CMD_IDENT;	/* Command byte. */

	tmpid = (u8*)&sata_ataid[port];

	if (get_sil3132_device_data(port, tmpid, sizeof(hd_driveid_t))) {
		printf("scsi_ahci: SCSI inquiry command failure.\n");
		return -EIO;
	}


	memcpy(&pccb->pdata[8], "ATA     ", 8);
	ata_id_strcpy((u16 *) &pccb->pdata[16], (u16 *)sata_ataid[port].model, 16);
	ata_id_strcpy((u16 *) &pccb->pdata[32], (u16 *)sata_ataid[port].fw_rev, 4);

	dump_ataid(&sata_ataid[port]);
	return 0;
}


/*
 * SCSI READ10 command operation.
 */
static int ata_sataop_read10 (ccb * pccb)
{
	int port = pccb->target;
	struct sil3132_ioports *pp = &(probe_ent->port[port]);
	struct ata_siiprb_command *prb;
	u_int8_t *fis;
	u64 lba = 0;
	u32 len = 0;

	if (pp->cmd_slot == NULL) {
		printf("port %d not started \n", port);
		return -EIO;
	}

	lba = (((u64) pccb->cmd[2]) << 24) | (((u64) pccb->cmd[3]) << 16)
		| (((u64) pccb->cmd[4]) << 8) | ((u64) pccb->cmd[5]);
	len = (((u32) pccb->cmd[7]) << 8) | ((u32) pccb->cmd[8]);

	/* For 10-byte and 16-byte SCSI R/W commands, transfer
	 * length 0 means transfer 0 block of data.
	 * However, for ATA R/W commands, sector count 0 means
	 * 256 or 65536 sectors, not 0 sectors as in SCSI.
	 *
	 * WARNING: one or two older ATA drives treat 0 as 0...
	 */
	if (!len)
		return 0;

	prb = pp->cmd_slot;

	if ((u8 *)prb != (u8 *)(((u32)&sil3132_cmd_slot[port] + 0x10) & (~0xf))) {
		debug("ata_sataop_read10 error cmd_slot = %08x \n", (u32)prb);
		pp->cmd_slot = (struct ata_siiprb_command *)
			(((u32)&sil3132_cmd_slot[port] + 0x10) & (~0xf));
	} else {
		debug("ata_sataop_read10 cmd_slot = %08x \n", (u32)prb);
	}
	
	fis = prb->fis;
	
	memset((u8*) prb, 0, sizeof(struct ata_siiprb_command));
	
	/* Construct the FIS */
	fis[0] = 0x27;		/* Host to device FIS. */
	fis[1] = 0x80;	/* Command FIS. */
	fis[2] = ATA_CMD_RD_DMA;	/* Command byte. */
	fis[3] = ATA_F_DMA;
		
	/* LBA address, only support LBA28 in this driver */
	fis[4] = pccb->cmd[5];
	fis[5] = pccb->cmd[4];
	fis[6] = pccb->cmd[3];
	fis[7] = (pccb->cmd[2] & 0x0f) | 0xe0;

	/* Sector Count */
	fis[12] = pccb->cmd[8];
	fis[13] = pccb->cmd[7];
	fis[15] = ATA_A_4BIT;

	/* Read from ahci */
	if (get_sil3132_device_data(pccb->target, pccb->pdata, pccb->datalen)) {
		printf("scsi_ahci: SCSI READ10 command failure.\n");
		return -EIO;
	}

	return 0;
}

int ata_sataop_set_mode (ccb *pccb)
{
	int port = pccb->target, mode, i = 3;
	struct sil3132_ioports *pp = &(probe_ent->port[port]);
	volatile u8 *port_mmio = (volatile u8 *)pp->port_mmio;
	struct ata_siiprb_command *prb;
	u_int8_t *fis;
	u_int64_t prb_bus;
	
	prb = pp->cmd_slot;
	fis = prb->fis;
		
	mode = (sata_ataid[port].dma_mword & 0x0700) >> 8;    
	while ((i-- > 0) && !(mode & 0x4) )
		mode <<= 1;  
	
	if (i >= 0) {
		memset((u8*) prb, 0, sizeof(struct ata_siiprb_command));
		memset(fis, 0, 20);
		prb->control = 0;
	
		/* Construct the FIS */
		fis[0] = 0x27;		/* Host to device FIS. */
		fis[1] = 0x80;	/* Command FIS. */
		fis[2] = ATA_CMD_SETF;	/* Command byte. */
		fis[3] = 0x3;	/* Set feature = transfer mode */
		if (port == 0) {
			/* force port 0 to udma4 mode */
			fis[12] = 0x44;
			i = 4;
		} else {
			fis[12] = 0x20 | i;
		}
		
		SIL3132_ISSUE_CMD();
	
		if (waiting_for_cmd_completed(port_mmio, prb, 1500, 0x1)) {
			printf("timeout exit!\n");
			return -1;
		}
	} 
	
	return (i);
}

/*
 * Issue a SMART control command
 */
int 
ata_sataop_enable_smart (ccb *pccb)
{    
	int port = pccb->target;
	struct sil3132_ioports *pp = &(probe_ent->port[port]);
	volatile u8 *port_mmio = (volatile u8 *)pp->port_mmio;
	struct ata_siiprb_command *prb;
	u_int8_t *fis;
	u_int64_t prb_bus;
    
	prb = pp->cmd_slot;
	fis = prb->fis;

	memset((u8*) prb, 0, sizeof(struct ata_siiprb_command));
	memset(fis, 0, 20);

	fis[0] = 0x27;  /* host to device */
	fis[1] = 0x80;  /* command FIS (note PM goes here) */
	fis[2] = WIN_SMART;
	fis[3] = SMART_ENABLE;
	fis[4] = 0;
	fis[5] = 0x4f;
	fis[6] = 0xc2;
	fis[7] = 0;
	fis[8] = 0;
	fis[9] = 0; 
	fis[10] = 0; 
	fis[11] = 0;
	fis[12] = 0;
	fis[13] = 0;
	fis[15] = ATA_A_4BIT;


	SIL3132_ISSUE_CMD();

	if (waiting_for_cmd_completed(port_mmio, prb, 1500, 0x1)) {
		printf("timeout exit!\n");
		return -1;
	}
	
	return 0;
}


/*
 * SCSI READ CAPACITY10 command operation.
 */
static int ata_sataop_read_capacity10 (ccb *pccb)
{
	u8 buf[8];

	memset(buf, 0, 8);

	*(u32 *) buf = le32_to_cpu(sata_ataid[pccb->target].lba_capacity);

	buf[6] = 512 >> 8;
	buf[7] = 512 & 0xff;

	memcpy(pccb->pdata, buf, 8);

	return 0;
}


/*
 * SCSI TEST UNIT READY command operation.
 */
static int ata_sataop_test_unit_ready (ccb *pccb)
{
	return (sata_ataid[pccb->target].lba_capacity) ? 0 : -EPERM;
}


int scsi_exec (ccb *pccb)
{
	int ret;

	switch (pccb->cmd[0]) {
	case SCSI_READ10:
		ret = ata_sataop_read10(pccb);
		break;
	case SCSI_RD_CAPAC:
		ret = ata_sataop_read_capacity10(pccb);
		break;
	case SCSI_TST_U_RDY:
		ret = ata_sataop_test_unit_ready(pccb);
		break;
	case SCSI_INQUIRY:
		ret = ata_sataop_inquiry(pccb);
		break;
	default:
		printf("Unsupport SCSI command 0x%02x\n", pccb->cmd[0]);
		return FALSE;
	}

	if (ret) {
		printf("SCSI command 0x%02x ret errno %d\n", pccb->cmd[0], ret);
		return FALSE;
	}
	return TRUE;

}


void scsi_low_level_init (int busdevfunc)
{
	int i;

	sil3132_chipinit(busdevfunc);

	for (i = 0; i < CFG_SATA_MAX_DEVICE; i++) {
		if (sil3132_port_start((u8) i)) {
			printf("Can not start port %d\n", i);
			continue;
		} else {
			printf("Port %d started \n", i);
		}
		sil3132_set_feature((u8) i);
	}
}


void scsi_bus_reset (void)
{
	/*Not implement*/
}


void scsi_print_error (ccb * pccb)
{
	/*The ahci error info can be read in the ahci driver*/
}
#endif
