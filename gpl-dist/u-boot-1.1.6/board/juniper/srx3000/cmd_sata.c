/*
 * Copyright (c) 2007-2010, Juniper Networks, Inc.
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
 *
 *
 *
 */

/*
 * SATA support.
 */
/* #define DEBUG 1 */

#include <common.h>
#include <command.h>
#include <asm/processor.h>
#include <scsi.h>
#include <image.h>
#include <pci.h>
#include <pcie.h>

#if (CONFIG_COMMANDS & CFG_CMD_BSP)

#ifdef CONFIG_SATA_SIL3132

#define SATA_VEND_ID 0x1095
#define SATA_DEV_ID  0x3132

#else
#error no satadevice defined
#endif

static ccb tempccb;	/* temporary sata command buffer */
static unsigned char tempbuff[512]; /* temporary data buffer */
static int sata_max_devs; /* number of highest available sata device */
static int sata_curr_dev; /* current device */
static block_dev_desc_t sata_dev_desc[CFG_SATA_MAX_DEVICE];

/********************************************************************************
 *  forward declerations of some Setup Routines
 */
void sata_setup_test_unit_ready (ccb * pccb);
void sata_setup_read_capacity (ccb * pccb);
void sata_setup_read6 (ccb * pccb, unsigned long start, unsigned short blocks);
void sata_setup_read_ext (ccb * pccb, unsigned long start, unsigned short blocks);
void sata_setup_inquiry (ccb * pccb);
void sata_ident_cpy (unsigned char *dest, unsigned char *src, unsigned int len);
ulong sata_read (int device, ulong blknr, ulong blkcnt, ulong *buffer);

extern int ata_sataop_set_mode (ccb *pccb);
extern int ata_sataop_enable_smart (ccb *pccb);

/*********************************************************************************
 * (re)-scan the sata bus and reports sata device info
 * to the user if mode = 1
 */
void sata_scan (int mode)
{
	unsigned char i, perq, modi, lun = 0;
	unsigned long capacity, blksz;
	int m;
	
	ccb* pccb=(ccb *)&tempccb;

	if(mode==1) {
		printf("scanning bus for devices...\n");
	}
	for(i=0;i<CFG_SATA_MAX_DEVICE;i++) {
		sata_dev_desc[i].target=0xff;
		sata_dev_desc[i].lun=0xff;
		sata_dev_desc[i].lba=0;
		sata_dev_desc[i].blksz=0;
		sata_dev_desc[i].type=DEV_TYPE_UNKNOWN;
		sata_dev_desc[i].vendor[0]=0;
		sata_dev_desc[i].product[0]=0;
		sata_dev_desc[i].revision[0]=0;
		sata_dev_desc[i].removable=FALSE;
		sata_dev_desc[i].if_type=IF_TYPE_SCSI;
		sata_dev_desc[i].dev=i;
		sata_dev_desc[i].part_type=PART_TYPE_UNKNOWN;
		sata_dev_desc[i].block_read=sata_read;
	}
	
	sata_max_devs=0;
	for(i=0;i<CFG_SATA_MAX_DEVICE;i++) {
		pccb->target=i;
		pccb->lun=lun;
		pccb->pdata=(unsigned char *)&tempbuff;
		pccb->datalen=512;
		sata_setup_inquiry(pccb);
		if(scsi_exec(pccb)!=TRUE) {
			if(pccb->contr_stat==SCSI_SEL_TIME_OUT) {
				debug ("Selection timeout ID %d\n",pccb->target);
				continue; /* selection timeout => assuming no device present */
			}
			scsi_print_error(pccb);
			continue;
		}
		perq=tempbuff[0];
		modi=tempbuff[1];
		if((perq & 0x1f)==0x1f) {
			continue; /* skip unknown devices */
		}
		if((modi&0x80)==0x80) /* drive is removable */
			sata_dev_desc[sata_max_devs].removable=TRUE;
		/* get info for this device */
		sata_ident_cpy(&sata_dev_desc[sata_max_devs].vendor[0],&tempbuff[8],8);
		sata_ident_cpy(&sata_dev_desc[sata_max_devs].product[0],&tempbuff[16],16);
		sata_ident_cpy(&sata_dev_desc[sata_max_devs].revision[0],&tempbuff[32],4);
		sata_dev_desc[sata_max_devs].target=pccb->target;
		sata_dev_desc[sata_max_devs].lun=pccb->lun;

		pccb->datalen=0;
		sata_setup_test_unit_ready(pccb);
		if(scsi_exec(pccb)!=TRUE) {
			if(sata_dev_desc[sata_max_devs].removable==TRUE) {
				sata_dev_desc[sata_max_devs].type=perq;
				goto removable;
			}
			scsi_print_error(pccb);
			continue;
		}
		pccb->datalen=8;
		sata_setup_read_capacity(pccb);
		if(scsi_exec(pccb)!=TRUE) {
			scsi_print_error(pccb);
			continue;
		}
		capacity=((unsigned long)tempbuff[0]<<24)|((unsigned long)tempbuff[1]<<16)|
				((unsigned long)tempbuff[2]<<8)|((unsigned long)tempbuff[3]);
		blksz=((unsigned long)tempbuff[4]<<24)|((unsigned long)tempbuff[5]<<16)|
			((unsigned long)tempbuff[6]<<8)|((unsigned long)tempbuff[7]);
		sata_dev_desc[sata_max_devs].lba=capacity;
		sata_dev_desc[sata_max_devs].blksz=blksz;
		sata_dev_desc[sata_max_devs].type=perq;
		init_part(&sata_dev_desc[sata_max_devs]);
removable:
		if(mode==1) {
			printf ("  Device %d: ", sata_max_devs);
			dev_print(&sata_dev_desc[sata_max_devs]);
		} /* if mode */
		
		if ((m = ata_sataop_set_mode(pccb)) < 0) 
		{
		    sata_dev_desc[sata_max_devs].use_multiword_dma = 0;					
		}			
		else
		{
		    sata_dev_desc[sata_max_devs].use_multiword_dma = 1;
		    printf("            Set multiword DMA mode %d\n", m);
		}

#if 0
		if (pccb->target == 0) {
		    /* enable smart mode for hard disk */
		    if (ata_sataop_enable_smart(pccb) == 0) {
		        printf("            Enable S.M.A.R.T Mode\n");
		    }
		}
#endif

		sata_max_devs++;
	}
	if(sata_max_devs>0)
		sata_curr_dev=0;
	else
		sata_curr_dev=-1;
}

void sata_init (void)
{
	int busdevfunc, i;

	busdevfunc=pcie_find_device(SATA_VEND_ID,SATA_DEV_ID,0); /* get PCI Device ID */
	if(busdevfunc==-1) {
		printf("Error sata Controller (%04X,%04X) not found\n",SATA_VEND_ID,SATA_DEV_ID);
		return;
	}
#ifdef DEBUG
	else {
		printf("sata Controller (%04X,%04X) found (%d:%d:%d)\n",SATA_VEND_ID,SATA_DEV_ID,(busdevfunc>>16)&0xFF,(busdevfunc>>11)&0x1F,(busdevfunc>>8)&0x7);
	}
#endif
	scsi_low_level_init(busdevfunc);
	for(i=0;i<CFG_SATA_MAX_DEVICE;i++) {
	    memset(&sata_dev_desc[i], 0, sizeof(block_dev_desc_t));
    }
	sata_scan(1);
}

block_dev_desc_t * sata_get_dev (int dev)
{
	return((block_dev_desc_t *)&sata_dev_desc[dev]);
}

/******************************************************************************
 * sata boot command intepreter. Derived from diskboot
 */
int do_sataboot (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char *boot_device = NULL;
	char *ep;
	int dev, part = 0;
	ulong addr, cnt, checksum;
	disk_partition_t info;
	image_header_t *hdr;
	int rcode = 0;

	switch (argc) {
	case 1:
		addr = CFG_LOAD_ADDR;
		boot_device = getenv ("bootdevice");
		break;
	case 2:
		addr = simple_strtoul(argv[1], NULL, 16);
		boot_device = getenv ("bootdevice");
		break;
	case 3:
		addr = simple_strtoul(argv[1], NULL, 16);
		boot_device = argv[2];
		break;
	default:
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	if (!boot_device) {
		puts ("\n** No boot device **\n");
		return 1;
	}

	dev = simple_strtoul(boot_device, &ep, 16);
	printf("booting from dev %d\n",dev);
	if (sata_dev_desc[dev].type == DEV_TYPE_UNKNOWN) {
		printf ("\n** Device %d not available\n", dev);
		return 1;
	}

	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid boot device, use `dev[:part]' **\n");
			return 1;
		}
		part = simple_strtoul(++ep, NULL, 16);
	}
	if (get_partition_info (sata_dev_desc, part, &info)) {
		printf("error reading partinfo\n");
		return 1;
	}
	if ((strncmp((char *)(info.type), BOOT_PART_TYPE, sizeof(info.type)) != 0) &&
	    (strncmp((char *)(info.type), BOOT_PART_COMP, sizeof(info.type)) != 0)) {
		printf ("\n** Invalid partition type \"%.32s\""
			" (expect \"" BOOT_PART_TYPE "\")\n",
			info.type);
		return 1;
	}

	printf ("\nLoading from sata device %d, partition %d: "
		"Name: %.32s  Type: %.32s\n",
		dev, part, info.name, info.type);

	debug ("First Block: %ld,  # of blocks: %ld, Block Size: %ld\n",
		info.start, info.size, info.blksz);

	if (sata_read (dev, info.start, 1, (ulong *)addr) != 1) {
		printf ("** Read error on %d:%d\n", dev, part);
		return 1;
	}

	hdr = (image_header_t *)addr;

	if (ntohl(hdr->ih_magic) == IH_MAGIC) {
		printf("\n** Bad Magic Number **\n");
		return 1;
	}

	checksum = ntohl(hdr->ih_hcrc);
	hdr->ih_hcrc = 0;

	if (crc32 (0, (uchar *)hdr, sizeof(image_header_t)) != checksum) {
		puts ("\n** Bad Header Checksum **\n");
		return 1;
	}
	hdr->ih_hcrc = htonl(checksum);	/* restore checksum for later use */

	print_image_hdr (hdr);
	cnt = (ntohl(hdr->ih_size) + sizeof(image_header_t));
	cnt += info.blksz - 1;
	cnt /= info.blksz;
	cnt -= 1;

	if (sata_read (dev, info.start+1, cnt,
		      (ulong *)(addr+info.blksz)) != cnt) {
		printf ("** Read error on %d:%d\n", dev, part);
		return 1;
	}
	/* Loading ok, update default load address */
	load_addr = addr;

	flush_cache (addr, (cnt+1)*info.blksz);

	/* Check if we should attempt an auto-start */
	if (((ep = getenv("autostart")) != NULL) && (strcmp(ep,"yes") == 0)) {
		char *local_args[2];
		extern int do_bootm (cmd_tbl_t *, int, int, char *[]);
		local_args[0] = argv[0];
		local_args[1] = NULL;
		printf ("Automatic boot of image at addr 0x%08lX ...\n", addr);
		rcode = do_bootm (cmdtp, 0, 1, local_args);
	}
	return rcode;
}

/*********************************************************************************
 * sata command intepreter
 */
int do_sata (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	switch (argc) {
    case 0:
    case 1:	printf ("Usage:\n%s\n", cmdtp->usage);	return 1;
    case 2:
		if (strncmp(argv[1],"res",3) == 0) {
			printf("\nReset sata\n");
			scsi_bus_reset();
			sata_scan(1);
			return 0;
		}

		if (strncmp(argv[1],"init",4) == 0) {
        	sata_init();
			return 0;
		}

		if (strncmp(argv[1],"inf",3) == 0) {
			int i;
			for (i=0; i<CFG_SATA_MAX_DEVICE; ++i) {
				if(sata_dev_desc[i].type==DEV_TYPE_UNKNOWN)
					continue; /* list only known devices */
				printf ("sata dev. %d:  ", i);
				dev_print(&sata_dev_desc[i]);
			}
			return 0;
		}
		if (strncmp(argv[1],"dev",3) == 0) {
			if ((sata_curr_dev < 0) || (sata_curr_dev >= CFG_SATA_MAX_DEVICE)) {
				printf("\nno sata devices available\n");
				return 1;
			}
			printf ("\n    Device %d: ", sata_curr_dev);
			dev_print(&sata_dev_desc[sata_curr_dev]);
			return 0;
		}
		if (strncmp(argv[1],"scan",4) == 0) {
			sata_scan(1);
			return 0;
		}
		if (strncmp(argv[1],"part",4) == 0) {
			int dev, ok;
			for (ok=0, dev=0; dev<CFG_SATA_MAX_DEVICE; ++dev) {
				if (sata_dev_desc[dev].type!=DEV_TYPE_UNKNOWN) {
					ok++;
					if (dev)
						printf("\n");
					debug ("print_part of %x\n",dev);
					print_part(&sata_dev_desc[dev]);
				}
			}
			if (!ok)
				printf("\nno sata devices available\n");
			return 1;
		}
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
  	case 3:
		if (strncmp(argv[1],"dev",3) == 0) {
			int dev = (int)simple_strtoul(argv[2], NULL, 10);
			printf ("\nsata device %d: ", dev);
			if (dev >= CFG_SATA_MAX_DEVICE) {
				printf("unknown device\n");
				return 1;
			}
			printf ("\n    Device %d: ", dev);
			dev_print(&sata_dev_desc[dev]);
			if(sata_dev_desc[dev].type == DEV_TYPE_UNKNOWN) {
				return 1;
			}
			sata_curr_dev = dev;
			printf("... is now current device\n");
			return 0;
		}
		if (strncmp(argv[1],"part",4) == 0) {
			int dev = (int)simple_strtoul(argv[2], NULL, 10);
			if(sata_dev_desc[dev].type != DEV_TYPE_UNKNOWN) {
				print_part(&sata_dev_desc[dev]);
			}
			else {
				printf ("\nsata device %d not available\n", dev);
			}
			return 1;
		}
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
    default:
		/* at least 4 args */
		if (strcmp(argv[1],"read") == 0) {
			ulong addr = simple_strtoul(argv[2], NULL, 16);
			ulong blk  = simple_strtoul(argv[3], NULL, 16);
			ulong cnt  = simple_strtoul(argv[4], NULL, 16);
			ulong n;
			printf ("\nsata read: device %d block # %ld, count %ld ... ",
					sata_curr_dev, blk, cnt);
			n = sata_read(sata_curr_dev, blk, cnt, (ulong *)addr);
			printf ("%ld blocks read: %s\n",n,(n==cnt) ? "OK" : "ERROR");
			return 0;
		}
	} /* switch */
	printf ("Usage:\n%s\n", cmdtp->usage);
	return 1;
}

/****************************************************************************************
 * sata_read
 */

#define sata_MAX_READ_BLK 0xFFFF /* almost the maximum amount of the sata_ext command.. */

ulong sata_read (int device, ulong blknr, ulong blkcnt, ulong *buffer)
{
	ulong start, blks, buf_addr;
	unsigned short smallblks;
	ccb* pccb=(ccb *)&tempccb;
	device&=0xff;
	/* Setup  device
	 */
	pccb->target=sata_dev_desc[device].target;
	pccb->lun=sata_dev_desc[device].lun;
	buf_addr=(unsigned long)buffer;
	start=blknr;
	blks=blkcnt;
	debug ("\nsata_read: dev %d startblk %lx, blccnt %lx buffer %lx\n",device,start,blks,(unsigned long)buffer);
	do {
		pccb->pdata=(unsigned char *)buf_addr;
		if(blks>sata_MAX_READ_BLK) {
			pccb->datalen=sata_dev_desc[device].blksz * sata_MAX_READ_BLK;
			smallblks=sata_MAX_READ_BLK;
			sata_setup_read_ext(pccb,start,smallblks);
			start+=sata_MAX_READ_BLK;
			blks-=sata_MAX_READ_BLK;
		}
		else {
			pccb->datalen=sata_dev_desc[device].blksz * blks;
			smallblks=(unsigned short) blks;
			sata_setup_read_ext(pccb,start,smallblks);
			start+=blks;
			blks=0;
		}
		debug ("sata_read_ext: startblk %lx, blccnt %x buffer %lx\n",start,smallblks,buf_addr);
		if(scsi_exec(pccb)!=TRUE) {
			scsi_print_error(pccb);
			blkcnt-=blks;
			break;
		}
		buf_addr+=pccb->datalen;
	} while(blks!=0);
	debug ("sata_read_ext: end startblk %lx, blccnt %x buffer %lx\n",start,smallblks,buf_addr);
	return(blkcnt);
}

/* copy src to dest, skipping leading and trailing blanks
 * and null terminate the string
 */
void sata_ident_cpy (unsigned char *dest, unsigned char *src, unsigned int len)
{
	int start,end;

	start=0;
	while(start<len) {
		if(src[start]!=' ')
			break;
		start++;
	}
	end=len-1;
	while(end>start) {
		if(src[end]!=' ')
			break;
		end--;
	}
	for( ; start<=end; start++) {
		*dest++=src[start];
	}
	*dest='\0';
}


/* Trim trailing blanks, and NUL-terminate string
 */
void sata_trim_trail (unsigned char *str, unsigned int len)
{
	unsigned char *p = str + len - 1;

	while (len-- > 0) {
		*p-- = '\0';
		if (*p != ' ') {
			return;
		}
	}
}


/************************************************************************************
 * Some setup (fill-in) routines
 */
void sata_setup_test_unit_ready (ccb * pccb)
{
	pccb->cmd[0]=SCSI_TST_U_RDY;
	pccb->cmd[1]=pccb->lun<<5;
	pccb->cmd[2]=0;
	pccb->cmd[3]=0;
	pccb->cmd[4]=0;
	pccb->cmd[5]=0;
	pccb->cmdlen=6;
	pccb->msgout[0]=SCSI_IDENTIFY; /* NOT USED */
}

void sata_setup_read_capacity (ccb * pccb)
{
	pccb->cmd[0]=SCSI_RD_CAPAC;
	pccb->cmd[1]=pccb->lun<<5;
	pccb->cmd[2]=0;
	pccb->cmd[3]=0;
	pccb->cmd[4]=0;
	pccb->cmd[5]=0;
	pccb->cmd[6]=0;
	pccb->cmd[7]=0;
	pccb->cmd[8]=0;
	pccb->cmd[9]=0;
	pccb->cmdlen=10;
	pccb->msgout[0]=SCSI_IDENTIFY; /* NOT USED */

}

void sata_setup_read_ext (ccb * pccb, unsigned long start, unsigned short blocks)
{
	pccb->cmd[0]=SCSI_READ10;
	pccb->cmd[1]=pccb->lun<<5;
	pccb->cmd[2]=((unsigned char) (start>>24))&0xff;
	pccb->cmd[3]=((unsigned char) (start>>16))&0xff;
	pccb->cmd[4]=((unsigned char) (start>>8))&0xff;
	pccb->cmd[5]=((unsigned char) (start))&0xff;
	pccb->cmd[6]=0;
	pccb->cmd[7]=((unsigned char) (blocks>>8))&0xff;
	pccb->cmd[8]=(unsigned char) blocks & 0xff;
	pccb->cmd[6]=0;
	pccb->cmdlen=10;
	pccb->msgout[0]=SCSI_IDENTIFY; /* NOT USED */
	debug ("sata_setup_read_ext: cmd: %02X %02X startblk %02X%02X%02X%02X blccnt %02X%02X\n",
		pccb->cmd[0],pccb->cmd[1],
		pccb->cmd[2],pccb->cmd[3],pccb->cmd[4],pccb->cmd[5],
		pccb->cmd[7],pccb->cmd[8]);
}

void sata_setup_read6 (ccb * pccb, unsigned long start, unsigned short blocks)
{
	pccb->cmd[0]=SCSI_READ6;
	pccb->cmd[1]=pccb->lun<<5 | (((unsigned char)(start>>16))&0x1f);
	pccb->cmd[2]=((unsigned char) (start>>8))&0xff;
	pccb->cmd[3]=((unsigned char) (start))&0xff;
	pccb->cmd[4]=(unsigned char) blocks & 0xff;
	pccb->cmd[5]=0;
	pccb->cmdlen=6;
	pccb->msgout[0]=SCSI_IDENTIFY; /* NOT USED */
	debug ("sata_setup_read6: cmd: %02X %02X startblk %02X%02X blccnt %02X\n",
		pccb->cmd[0],pccb->cmd[1],
		pccb->cmd[2],pccb->cmd[3],pccb->cmd[4]);
}


void sata_setup_inquiry (ccb * pccb)
{
	pccb->cmd[0]=SCSI_INQUIRY;
	pccb->cmd[1]=pccb->lun<<5;
	pccb->cmd[2]=0;
	pccb->cmd[3]=0;
	if(pccb->datalen>255)
		pccb->cmd[4]=255;
	else
		pccb->cmd[4]=(unsigned char)pccb->datalen;
	pccb->cmd[5]=0;
	pccb->cmdlen=6;
	pccb->msgout[0]=SCSI_IDENTIFY; /* NOT USED */
}

U_BOOT_CMD (
	sata, 5, 1, do_sata,
	"sata    - sata sub-system\n",
	"reset - reset sata controller\n"
	"sata init  - show available sata devices\n"
	"sata info  - show available sata devices\n"
	"sata scan  - (re-)scan sata bus\n"
	"sata device [dev] - show or set current device\n"
	"sata part [dev] - print partition table of one or all sata devices\n"
	"sata read addr blk# cnt - read `cnt' blocks starting at block `blk#'\n"
	"     to memory address `addr'\n"
);

U_BOOT_CMD (
	sataboot, 3, 1, do_sataboot,
	"sataboot- boot from sata device\n",
	"loadAddr dev:part\n"
);

#endif /* #if (CONFIG_COMMANDS & CFG_CMD_sata) */
