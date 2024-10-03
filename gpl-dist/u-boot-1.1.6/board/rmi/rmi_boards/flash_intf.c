#include "rmi/types.h"
#include "configs/rmi_boards.h"
#include "common.h"
#include "flash.h"
#include "environment.h"
#include "rmi/shared_structs.h"
#include "rmi/board.h"
#include "rmi/physaddrspace.h"
#include "rmi/bridge.h"
#include "rmi/pioctrl.h"
#include "rmi/cpld.h"
#include "rmi/flashdev.h"
#include "rmi/cfiflash.h"
#include "rmi/gpio.h"
#include "asm/global_data.h"


extern struct flash_device cfi_flash;
extern struct rmi_board_info rmi_board;
extern struct flash_device nand_flash;

/* UBOOT Interfce Functions are implemented here */

flash_info_t flash_info[CFG_MAX_FLASH_BANKS];   /* info for FLASH chips */
extern struct cfiflash rmi_cfiflash;
void env_crc_update (void);

void flash_print_info(flash_info_t *info)
{
	int i;
	int valid=0;

	if(info->flash_id == rmi_cfiflash.id) {
		cfi_flash.iprint(&cfi_flash);
		valid = 1;
	}else if (info->flash_id == nand_flash.flash_id) {
		nand_flash.iprint(&nand_flash);
		valid = 1;
	}
	if(valid) {
		puts ("  Size: ");
		if ((info->size >> 20) > 0)
		{
			printf ("%ld MiB",info->size >> 20);
		}
		else
		{
			printf ("%ld KiB",info->size >> 10);
		}
		printf (" in %d Sectors\n", info->sector_count);

		printf ("  Sector Start Addresses:");
		for (i = 0; i < info->sector_count; i++) {
			if ((i % 4) == 0) {
				printf ("\n    ");
			}
			printf ("%02d: %08lX%s  ", i,info->start[i],
					info->protect[i] ? " P" : "  ");
		}
		printf ("\n\n");
	} else {
		printf("Unknown Flash or Flash does not exist\n");
		return;
	}
}
int flash_read(flash_info_t *info, int s_first, int s_last, char *buf)
{
	unsigned long iobase;
	uint32_t sector_size;
	int i, err = ERR_INVAL;

	if ((s_first < 0) || (s_first > s_last) || 
			s_last >= info->sector_count)
	{
		printf("Invalid sectors start %d and end %d\n", 
				s_first, s_last);
		return ERR_INVAL;
	}
	sector_size = (info->size / info->sector_count);
	if(info->flash_id == rmi_cfiflash.id) {
		for(i=s_first; i <= s_last; i++) {
			iobase = rmi_cfiflash.iobase + (i * sector_size);
			err = cfi_flash.read(&cfi_flash, iobase, buf,
					sector_size);
			return err;

		}
	} else if(info->flash_id == nand_flash.flash_id) {
		for(i=s_first; i <= s_last; i++) {
			iobase = nand_flash.iobase + (i * sector_size);
			err = nand_flash.read(&nand_flash, iobase, buf,
					sector_size);
			return err;
		}

	} else {
		return ERR_INVAL;
	}
	return ERR_INVAL;
}

int flash_erase(flash_info_t *info, int s_first, int s_last)
{
	unsigned long iobase;
	uint32_t sector_size;
	int i, err = ERR_INVAL;

	if ((s_first < 0) || (s_first > s_last) || 
			s_last >= info->sector_count)
	{
		printf("Invalid sectors start %d and end %d\n", 
				s_first, s_last);
		return ERR_INVAL;
	}
	sector_size = (info->size / info->sector_count);
	if(info->flash_id == rmi_cfiflash.id) {
		for(i=s_first; i <= s_last; i++) {
			iobase = rmi_cfiflash.iobase + (i * sector_size);
			if (info->protect[i])
				putc('P');
			else {
				puts("'E\b");
				err = cfi_flash.erase(&cfi_flash, iobase, 
						sector_size);
				return err;

			}
		}
	} else if(info->flash_id == nand_flash.flash_id) {
		for(i=s_first; i <= s_last; i++) {
			iobase = nand_flash.iobase + (i * sector_size);
			if (info->protect[i])
				putc('P');
			else {
				puts("'E\b");
				err = nand_flash.erase(&nand_flash, iobase, 
								sector_size);
				return err;
			}
		}

	} else {
		return ERR_INVAL;
	}
	return ERR_INVAL;
}

int write_buff (flash_info_t * info, uchar * src, ulong addr, ulong cnt)
{
	unsigned int bs;

	bs = info->size / info->sector_count;

	if(info->flash_id == rmi_cfiflash.id) {
		cfi_flash.erase(&cfi_flash, addr, cnt);
		return cfi_flash.write(&cfi_flash, addr, src, cnt);
	} else if(info->flash_id == nand_flash.flash_id) {
		/* if first block is also being written then do this
		   write in two steps to take care of the logical 
		   block numbers.
		   */
		if(addr == nand_flash.iobase) {
			nand_flash.erase(&nand_flash, addr, bs);
			nand_flash.write(&nand_flash, addr, src, bs);
			addr += bs;
			src += bs;
			cnt -= bs;
		}
		/* erase 1 extra block */
		nand_flash.erase(&nand_flash, addr, cnt+bs);
		return nand_flash.write(&nand_flash, addr, src, cnt);
	} else {
		return ERR_INVAL;
	}
}
static unsigned long fill_cfi_flash_info(flash_info_t *info)
{
	int sector_size, sector_count;
	int i;

	info->size = rmi_cfiflash.size;
	info->sector_count = rmi_cfiflash.nr_blocks;
	info->flash_id = rmi_cfiflash.id;
	sector_size = (info->size / info->sector_count);
	sector_count = (info->sector_count > CFG_MAX_FLASH_SECT) ?
		CFG_MAX_FLASH_SECT : info->sector_count;
#ifdef RMI_DEBUG
	printf("Flash size 0x%x sectors %d flash_id 0x%x\n", 
			info->size, info->sector_count, info->flash_id);
#endif
	for(i=0; i < sector_count; i++) {
		info->start[i] = cfi_flash.iobase + (i * sector_size);
		info->protect[i] = 0;
	}
	return (info->size);
}

static unsigned long fill_nand_flash_info(flash_info_t *info)
{
	int sector_size, sector_count;
	int i;

#ifdef RMI_DEBUG
	printf("Actual flash size = %x\n", nand_flash.size);
#endif
	info->size = nand_flash.size;
	info->sector_count = nand_flash.nr_blocks;
	sector_size = (info->size / info->sector_count);
	info->size = (4 << 20);
	info->sector_count = (info->size / sector_size);

	info->flash_id = nand_flash.flash_id;
	sector_count = (info->sector_count > CFG_MAX_FLASH_SECT) ?
		CFG_MAX_FLASH_SECT : info->sector_count;
#ifdef RMI_DEBUG
	printf("Flash size 0x%x sectors %d flash_id 0x%x\n", 
			info->size, info->sector_count, info->flash_id);
#endif
	for(i=0; i < sector_count; i++) {
		info->start[i] = nand_flash.iobase + (i * sector_size);
		info->protect[i] = 0;
	}
	return (info->size);
}

unsigned long flash_init (void)
{
	int i;
	unsigned long size, c_size=0, n_size=0;

	/* Init the global flash info struture first */
	for(i=0; i < CFG_MAX_FLASH_BANKS; i++)
		flash_info[i].flash_id = FLASH_UNKNOWN;

	if(rmi_board.bfinfo.primary_flash_type == FLASH_TYPE_NOR) 
		c_size =  fill_cfi_flash_info(&flash_info[0]);
	else if(rmi_board.bfinfo.secondary_flash_type == FLASH_TYPE_NOR) 
		size =  fill_cfi_flash_info(&flash_info[1]);
	if(rmi_board.bfinfo.primary_flash_type == FLASH_TYPE_NAND)
		n_size =  fill_nand_flash_info(&flash_info[0]);
	else if(rmi_board.bfinfo.secondary_flash_type == FLASH_TYPE_NAND)
		size =  fill_nand_flash_info(&flash_info[1]);

	if(rmi_board.bfinfo.primary_flash_type == FLASH_TYPE_NOR)
		return c_size;
	else
		return n_size;

}


int flash_probe(env_t **env, env_t **flash_addr)
{
	int s_start, s_end;
	flash_info_t *info = &flash_info[0];
	uint32_t sector_size;
	unsigned long   crc;
	gd_t	*temp_gd;

	__asm__ volatile (
			"move	%0, $26\n\t"
			:"=r"(temp_gd)
			::"$26"
			);


	/* This function primarily changes the global env flash addresses 
	   depending upon the primary flash available on the board
	   */
	*flash_addr = (env_t *)(info->start[0] + CFG_ENV_OFFSET);

	sector_size = (info->size / info->sector_count);
	s_start = ((uint32_t)(*flash_addr) - info->start[0]) / sector_size;
	s_end = ((uint32_t)(*flash_addr) - info->start[0] + 
				CFG_ENV_SIZE) / sector_size;

	s_end -= 1;

#if 0
	printf("Using 0x%x as the actual start address for env \n", *flash_addr);
	printf("reading sector %d to %d\n", s_start, s_end);
#endif
	if(flash_read(&flash_info[0], s_start, s_end, (char *)*env)) {
		printf("flash_read failed\n");
		return -1;
	} else {
		crc = crc32(0, (*env)->data, ENV_SIZE);
		if(crc != (*env)->crc) {
			printf("CRC for env is not valid.\n");
			printf("May be, saveenv is not done\n");
			memset((*env)->data, 0, ENV_SIZE);
			env_crc_update ();
		}
		temp_gd->env_addr = (*env)->data;
		temp_gd->env_valid = 1;
		return 0;
	}

	return -1;
}
