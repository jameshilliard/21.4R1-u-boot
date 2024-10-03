/*
 * Copyright (c) 2006-2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * (C) Copyright 2000-2004
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#ifndef _PART_H
#define _PART_H
#include <ide.h>

typedef struct block_dev_desc {
	int		if_type;	/* type of the interface */
	int	        dev;	  	/* device number */
	unsigned char	part_type;  	/* partition type */
	unsigned char	target;		/* target SCSI ID */
	unsigned char	lun;		/* target LUN */
	unsigned char	type;		/* device type */
	unsigned char	removable;	/* removable device */
#ifdef CONFIG_LBA48
	unsigned char	lba48;		/* device can use 48bit addr (ATA/ATAPI v7) */
#endif
	lbaint_t		lba;	  	/* number of blocks */
	unsigned long	blksz;		/* block size */
#ifdef CONFIG_SRX3000
	unsigned char	use_multiword_dma;	/* device can use multiword dma mode */
#endif	
	unsigned char	vendor [40+1]; 	/* IDE model, SCSI Vendor */
	unsigned char	product[20+1];	/* IDE Serial no, SCSI product */
	unsigned char	revision[8+1];	/* firmware revision */
	unsigned long	(*block_read)(int dev,
				      unsigned long start,
				      lbaint_t blkcnt,
				      unsigned long *buffer);
#ifdef USB_WRITE_READ
	unsigned long	(*block_write)(int dev,
				      unsigned long start,
				      lbaint_t blkcnt,
				      unsigned long *buffer);
#endif
          /* These are really bus based, but we put them here to ease lookup */
        uint64_t base_addr0;
        uint64_t base_addr1;
        uint64_t base_addr2;
        uint32_t    quirks;  /* bitmask to indicate special handling cases */
        unsigned char dev_on_bus;   /* What position this device is on it's bus */
        unsigned char dma_type;     /* 0 = none, 1 = mdma, 2 = udma */
        unsigned char dma_mode;       /* Which MDMA/UDMA mode in use */
        unsigned char dma_channel;
}block_dev_desc_t;


#define CF_DMA_TYPE_NONE    0
#define CF_DMA_TYPE_MDMA    1
#define CF_DMA_TYPE_UDMA    2

/* Interface types: */
#define IF_TYPE_UNKNOWN		0
#define IF_TYPE_IDE		1
#define IF_TYPE_SCSI		2
#define IF_TYPE_ATAPI		3
#define IF_TYPE_USB		4
#define IF_TYPE_DOC		5
#define IF_TYPE_MMC		6
#define IF_TYPE_NOR		7
#define IF_TYPE_SATA		8

/* Part types */
#define PART_TYPE_UNKNOWN	0x00
#define PART_TYPE_MAC		0x01
#define PART_TYPE_DOS		0x02
#define PART_TYPE_ISO		0x03
#define PART_TYPE_AMIGA		0x04

#define BLOCK_QUIRK_IDE_VIA             (1 << 0)
#define BLOCK_QUIRK_IDE_CF              (1 << 1)
#define BLOCK_QUIRK_IDE_4PORT           (1 << 2)
#define BLOCK_QUIRK_IDE_CF_TRUE_IDE     (1 << 3)
#define BLOCK_QUIRK_IDE_CF_TRUE_IDE_DMA (1 << 4)

/*
 * Type string for U-Boot bootable partitions
 */
#define BOOT_PART_TYPE	"U-Boot"	/* primary boot partition type	*/
#define BOOT_PART_COMP	"PPCBoot"	/* PPCBoot compatibility type	*/

/* device types */
#define DEV_TYPE_UNKNOWN	0xff	/* not connected */
#define DEV_TYPE_HARDDISK	0x00	/* harddisk */
#define DEV_TYPE_TAPE		0x01	/* Tape */
#define DEV_TYPE_CDROM		0x05	/* CD-ROM */
#define DEV_TYPE_OPDISK		0x07	/* optical disk */
#define DEV_TYPE_GENDISK	0x08	/* A generic disk - use when exact type
                                           of the device is not important */

typedef struct disk_partition {
	ulong	start;		/* # of first block in partition	*/
	ulong	size;		/* number of blocks in partition	*/
	ulong	blksz;		/* block size in bytes			*/
	uchar	name[32];	/* partition name			*/
	uchar	type[32];	/* string type description		*/
	uchar   boot_ind;       /* 0x1 - active                 */
} disk_partition_t;

/* disk/part.c */
int get_partition_info (block_dev_desc_t * dev_desc, int part, disk_partition_t *info);
void print_part (block_dev_desc_t *dev_desc);
void  init_part (block_dev_desc_t *dev_desc);
void dev_print(block_dev_desc_t *dev_desc);


#ifdef CONFIG_MAC_PARTITION
/* disk/part_mac.c */
int get_partition_info_mac (block_dev_desc_t * dev_desc, int part, disk_partition_t *info);
void print_part_mac (block_dev_desc_t *dev_desc);
int   test_part_mac (block_dev_desc_t *dev_desc);
#endif

#ifdef CONFIG_DOS_PARTITION
/* disk/part_dos.c */
int get_partition_info_dos (block_dev_desc_t * dev_desc, int part, disk_partition_t *info);
void print_part_dos (block_dev_desc_t *dev_desc);
int   test_part_dos (block_dev_desc_t *dev_desc);
#endif

#ifdef CONFIG_ISO_PARTITION
/* disk/part_iso.c */
int get_partition_info_iso (block_dev_desc_t * dev_desc, int part, disk_partition_t *info);
void print_part_iso (block_dev_desc_t *dev_desc);
int   test_part_iso (block_dev_desc_t *dev_desc);
#endif

#ifdef CONFIG_AMIGA_PARTITION
/* disk/part_amiga.c */
int get_partition_info_amiga (block_dev_desc_t * dev_desc, int part, disk_partition_t *info);
void print_part_amiga (block_dev_desc_t *dev_desc);
int   test_part_amiga (block_dev_desc_t *dev_desc);
#endif

#endif /* _PART_H */
