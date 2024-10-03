/*
 * Copyright (c) 2007-2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * most of this is porting from the file board\cogent\lcd.c
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

#include <config.h>
#include <common.h>
#include <part.h>
#include <fat.h>

#if (CONFIG_COMMANDS & CFG_CMD_FAT)
extern int fat_register_device(block_dev_desc_t *dev_desc, int part_no);

block_dev_desc_t nor_dev;

block_dev_desc_t * nor_get_dev(int dev)
{
	return ((block_dev_desc_t *)&nor_dev);
}

ulong nor_bread(int dev_num, ulong blknr, ulong blkcnt, ulong *dst)
{
	int nor_block_size = FS_BLOCK_SIZE;
	ulong src = blknr * nor_block_size + CFG_FLASH_BASE;

	memcpy((void *)dst, (void *)src, blkcnt*nor_block_size);
	return blkcnt;
}

int nor_init(int verbose)
{

    /* fill in device description */
    nor_dev.if_type = IF_TYPE_NOR;
    nor_dev.part_type = PART_TYPE_DOS;
    nor_dev.dev = 0;
    nor_dev.lun = 112;  /* start from 64K offset */
    nor_dev.type = 0;
    nor_dev.blksz = 512;
    nor_dev.lba = 12176;  /* 12176 * 512 = 5.94MByte */
    sprintf(nor_dev.vendor,"Man %s", "Spansion");
    sprintf(nor_dev.product,"%s","NOR Flash");
    sprintf(nor_dev.revision,"%x %x",0, 0);
    nor_dev.removable = 0;
    nor_dev.block_read = nor_bread;

    if (fat_register_device(&nor_dev,1)) { /* partitions start counting with 1 */
        if (verbose)
		printf ("** Failed to register NOR FAT device %d **\n", nor_dev.dev);
    }

    return 1;
}
#endif

