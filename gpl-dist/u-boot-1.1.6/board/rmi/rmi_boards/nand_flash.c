/***********************************************************************
Copyright 2003-2008 Raza Microelectronics, Inc.(RMI). All rights
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
#include "rmi/types.h"
#include "common.h"
#include "rmi/byteorder.h"
#include "rmi/flashdev.h"
#include "rmi/cpld.h"
#include "micron_nand.h"
#include "rmi/nand_bad_info.h"

/* 3 bytes of ecc per 256 bytes block of data */
#define ECC_BLOCK_SIZE 256
#define ECC_PER_BLOCK 3

struct nand_flash;
int nand_chip_select;

extern int nand_calculate_ecc(const unsigned char *dat, unsigned char *ecc_code);
extern int nand_correct_data(unsigned char *dat, unsigned char *read_ecc, unsigned char *calc_ecc);

struct algorithm
{
	char *name;
	int (*reset) (struct nand_flash *this_nand);
	int (*check_status) (struct nand_flash *this_nand);

	int (*lock_block) (struct nand_flash *this_nand, unsigned long blocknum,
			unsigned char lock);
	int (*unlock_block) (struct nand_flash *this_nand, unsigned long
			blocknum, unsigned char lock);
	int (*block_erase)     (struct nand_flash *this_nand, unsigned long
			blocknum);
	int (*block_write)     (struct nand_flash *this_nand, unsigned long
			blocknum, unsigned char *val,unsigned long
			logical_blocknum);
	int (*page_read)     (struct nand_flash *this_nand, unsigned long
			pagenum, unsigned char *val);
};

struct nand_flash
{
	char name[20];
	int id;
	unsigned long iobase;
	unsigned int bwidth;     // Width of the Bus
	unsigned int cwidth;     // No of Address pins on chip
	unsigned long nr_blocks; // No of Blocks/Sectors
	unsigned long size;
	unsigned long wbs;       // Write Buffer size (in Bytes)
	unsigned char if_size; // interface size
	unsigned long page_size; /* page size without spare area */
	unsigned long page_spare_area_size; /* spare area in bytes per page */
	unsigned long page_size_with_spare_area; /* total page size */
	unsigned long nr_pages; /* no of pages per block */
	unsigned long block_size_with_spare_area; /* total block size in bytes */
	unsigned long block_size; /* block size without spare area */
	struct algorithm *algo;  // Algorithm
	void *priv;
};

struct nand_flash micron_nand_flash;

/* Start micron algo wrapper */

int micron_reset(struct nand_flash *this_nand)
{
	return nand_reset();
}

int micron_check_status(struct nand_flash *this_nand)
{
	return nand_read_status();
}

static int micron_lock_block(struct nand_flash *this_nand, 
		unsigned long block_num,unsigned char lock)
{
	return -1;
}

static int micron_block_erase(struct nand_flash *this_nand, 
		unsigned long block_num)
{
	int result;
	result = nand_erase_block(block_num);
	if (result) {
		printf("return val after erasing block %d is %x\n",block_num, 
				result);
	}
	return result;

}

static int micron_page_verify(struct nand_flash *this_nand, 
		unsigned long page_num,unsigned char *val)
{
	unsigned char ecc_buf[3];
	unsigned char tbuf[2200];
	int retval,i;
	struct nand_extra_bytes *extra;

	extra = (struct nand_extra_bytes *)&tbuf[2048];

	retval = nand_read_page(page_num, 0,
			this_nand->page_size_with_spare_area, tbuf);
	if (retval)
		return retval;

	for ( i = this_nand->page_size; 
			i < this_nand->page_size_with_spare_area; i++) {

		if (tbuf[i] != val[i]) {
			printf("extra bytes read back incorrect\n");
			return -1;
		}
	}
	for (i = 0; i < this_nand->page_size/ECC_BLOCK_SIZE; i++) {
		nand_calculate_ecc(&tbuf[i*ECC_BLOCK_SIZE], ecc_buf);
		retval = nand_correct_data(&tbuf[i*ECC_BLOCK_SIZE],&(extra->ecc[i*ECC_PER_BLOCK]),ecc_buf);
		if (retval == -1)
			return retval; /* -1: unrecoverable error */
		/* 0:no error 1:error corrected */
	}
	return 0;

}


static int micron_page_read(struct nand_flash *this_nand, 
		unsigned long page_num,unsigned char *val)
{
	unsigned char ecc_buf[ECC_PER_BLOCK];
	unsigned char tbuf[2200];
	int retval,i;
	struct nand_extra_bytes *extra;

	extra = (struct nand_extra_bytes *)&tbuf[this_nand->page_size];

	retval = nand_read_page(page_num, 0,
			this_nand->page_size_with_spare_area, tbuf);
	if (retval)
		return retval;
	for (i = 0; i < this_nand->page_size/ECC_BLOCK_SIZE; i++) {
		nand_calculate_ecc(&tbuf[i*ECC_BLOCK_SIZE], ecc_buf);
		retval = nand_correct_data(&tbuf[i*ECC_BLOCK_SIZE],
				&(extra->ecc[i*ECC_PER_BLOCK]),ecc_buf);
		if (retval == -1)
			return retval; /* -1: unrecoverable error */
		/* 0:no error 1:error corrected */
	}
	memcpy(val,tbuf,this_nand->page_size);
	return 0;

}

static int micron_block_write(struct nand_flash *this_nand, 
		unsigned long block_num,unsigned char *val,unsigned long
		logical_blocknum)
{
	int i,j,result=0;
	unsigned char tempbuf[2200];
	struct nand_extra_bytes *extra;
	unsigned char ecc_buf[3];

	extra = (struct nand_extra_bytes *)&tempbuf[this_nand->page_size];

	for (i = 0; i < this_nand->nr_pages; i++) {
		memcpy(tempbuf,val + i * this_nand->page_size,this_nand->page_size);
		memset(&tempbuf[this_nand->page_size],0xff,this_nand->page_spare_area_size);

		for (j = 0; j < this_nand->page_size/ECC_BLOCK_SIZE; j++) {
			nand_calculate_ecc(&tempbuf[j*ECC_BLOCK_SIZE], ecc_buf);
			/* 3 bytes of ecc per 256 bytes block of data */
			memcpy(&(extra->ecc[j*ECC_PER_BLOCK]),ecc_buf,ECC_PER_BLOCK); 
		}
		extra->boot_sig = BOOT_NAND_MAGIC;
		extra->block_num = logical_blocknum;
		result = nand_program_page(block_num * this_nand->nr_pages + i,
				0, this_nand->page_size_with_spare_area,
				tempbuf);
		if (result) {
			printf("return val after writing page %d is %x\n",i,
					result);
			return result;
		}

		result = micron_page_verify(this_nand,
				block_num * this_nand->nr_pages + i ,tempbuf);
		if (result) {
			printf("return val after verifying page %d is %x\n",i,
					result);
			return result;
		}
	}
	return result;
}

/* End of Micron Algorithm wrapper
 */

#define MICRON_ALGO_ID 0
static struct algorithm nand_algos[] = 
{
	{
		.name              = "Micron Nand",
		.reset             = micron_reset,
		.check_status      = micron_check_status,
		.lock_block        = micron_lock_block, //not yet implemented
		.block_erase       = micron_block_erase,
		.block_write       = micron_block_write,
		.page_read         = micron_page_read,
	},
	{}
};

static int nand_flash_reset(struct flash_device *this_flash)
{
	struct nand_flash *this_nand = (struct nand_flash *)this_flash->priv;
	struct algorithm *algo = this_nand->algo;
	algo->reset(this_nand);
	return 0;
}

static int nand_flash_erase(struct flash_device *this_flash, unsigned long address, 
		unsigned long nBytes)
{
	struct nand_flash *this_nand = (struct nand_flash *)this_flash->priv;
	unsigned long block_size = (this_nand->block_size);
	unsigned long i;
	struct algorithm *algo = this_nand->algo;
	unsigned long nblocks=(nBytes/block_size)+((nBytes%block_size)>0 ? 1:0);
	unsigned long start_block = (address - this_flash->iobase) / block_size;

	if((address - this_flash->iobase) % block_size)
	{
		printf("%s Address 0x%08x is not block aligned. "
				"Cannot continue...\n",
				__FUNCTION__,address);
		return -1;
	}

	for (i = start_block; i < nblocks + start_block; i++,
			address+=block_size) {
		printf("Erasing 0x%08x... \n",address);
		algo->block_erase(this_nand, i);
	}

	algo->reset(this_nand);
	printf("Done.\n");
	return 0;

}

static int nand_flash_write(struct flash_device *this_flash, unsigned long 
		address, unsigned char *src, unsigned long nBytes)
{
	struct nand_flash *this_nand = (struct nand_flash *)this_flash->priv;
	struct algorithm *algo = this_nand->algo;
	unsigned long i,logical_blocknum;
	unsigned long block_size = (this_nand->block_size);
	unsigned long nblocks=(nBytes/block_size)+((nBytes%block_size)>0 ? 1:0);
	unsigned long start_block = (address - this_flash->iobase) / block_size;
	unsigned long additional_blocks,actual_logical_blocknum;

	if((address - this_flash->iobase) % block_size)
	{
		printf("%s Address 0x%08x is not block aligned. "
				"Cannot continue...\n",
				__FUNCTION__,address);
		return -1;
	}

	for (i = start_block,logical_blocknum=0,additional_blocks=0; logical_blocknum < nblocks; 
			i++) {
		if (logical_blocknum + 1 == nblocks) 
			actual_logical_blocknum = logical_blocknum | BOOT_LAST_BLOCK;
		else 
			actual_logical_blocknum = logical_blocknum;

		printf("Copying 0x%08x...logical blk num %x\n",
				address, actual_logical_blocknum);
		if (!algo->block_write(this_nand, i, src, actual_logical_blocknum)) {
			logical_blocknum++;
			address += block_size;
			src += block_size;
		} else {
			//erase this block
			algo->block_erase(this_nand, i);
			//erase one more block...
			algo->block_erase(this_nand, start_block+nblocks+additional_blocks);
			additional_blocks++;
			if (additional_blocks > nblocks) {
				printf("too many errors in block write. Aborting.\n");
				return -1;
			}
		}
	}

	printf("Done.\n");
	return 0;

}

static int nand_flash_read(struct flash_device *this_flash, unsigned long 
		address, unsigned char *dst, unsigned long nBytes)
{
	struct nand_flash *this_nand = (struct nand_flash *)this_flash->priv;
	struct algorithm *algo = this_nand->algo;
	unsigned long i,logical_page,additional_pages;
	unsigned long page_size = (this_nand->page_size);
	unsigned long npages = nBytes/page_size + ((nBytes%page_size)>0 ? 1:0);
	unsigned long start_page = (address - this_flash->iobase) / 
		this_nand->page_size;

	if((address - this_flash->iobase) % (this_nand->page_size *
				this_nand->nr_pages))
	{
		printf("%s Address 0x%08x is not block aligned. "
				"Cannot continue...\n",
				__FUNCTION__,address);
		return -1;
	}

	for (i = start_page,logical_page=0,additional_pages=0; logical_page < npages; i++) {
		if (!algo->page_read(this_nand, i, dst)) {
			logical_page++;
			dst+=this_nand->page_size;
		} else {
			additional_pages++;
			if (additional_pages > npages) {
				printf("too many errors while reading nand "
						"flash\n");
				return -1;
			}
		}
	}

	return 0;

}

static void print_nand_flash_info(struct flash_device *this_flash)
{
	struct nand_flash *this_nand = (struct nand_flash *)this_flash->priv;
	printf("Nand Flash Info: ID%d, Algo:%s, Blocks:0x%x, BlockSize:0x%x\n",
			this_nand->id,
			this_nand->algo->name,
			this_nand->nr_blocks,
			this_nand->block_size);
}

/* Unfortunately, there is no std way to extract nand flash info. 
 * So, values are hardcoded. Need to look at ONFI spec in future revisions 
 */
static int fill_micron_nand_info(struct nand_flash *this_nand, 
		unsigned char *id_buf)
{
	sprintf(this_nand->name,"Micron");

	switch (id_buf[1]) {
		case 0xAA:
		case 0xBA:
		case 0xCA:
		case 0xDA:
			this_nand->size = 0x10000000; /* 2Gb = 256 MB */
			this_nand->nr_blocks = 2048;
			break;
		case 0xDC:
			this_nand->size = 0x20000000; /* 4Gb = 512 MB */
			this_nand->nr_blocks = 2048 * 2;
			break;
		default:
			printf("incorrect id info field 2 for micron"
					" nand flash\n");
			return -1;
	}

	switch (id_buf[3]) {
		case 0x15: this_nand->if_size = 8;
			   break;
		case 0x55:
			   this_nand->if_size = 16;
			   break;
		default:
			   printf("incorrect interface size info for micron"
					   " nand flash\n");
			   return -1;
	}

	this_nand->page_size = 2048; /* without spare area */
	this_nand->page_spare_area_size = 64;
	this_nand->page_size_with_spare_area = this_nand->page_size +
		this_nand->page_spare_area_size; 
	this_nand->nr_pages = 64;

	this_nand->block_size_with_spare_area = this_nand->nr_pages *
		( this_nand->page_size + this_nand->page_spare_area_size);
	/* without spare area */
	this_nand->block_size = this_nand->nr_pages * this_nand->page_size; 

	this_nand->algo = &nand_algos[MICRON_ALGO_ID]; /* micron_algo */
	return 0;

}

int nand_flash_detect(struct flash_device *this_flash)
{
	struct nand_flash *nand = &micron_nand_flash;
	unsigned long iobase = this_flash->iobase;
	unsigned char id_buf[4];
	int cs=0;

	cs = this_flash->chip_select;
	nand_chip_select = cs;

	/* 
	 * See chapter: Flash/PCMCIA Memory and the peripherals interface of XLS PRM
	 */

	/*dev param chip 0*/
	*(volatile unsigned long *)(0xbef19080 + cs*4) = 0x000041e0;
	/*time paramA chip 0*/
	*(volatile unsigned long *)(0xbef190c0 + cs*4) = 0x4f400e22;
	/*time paramB chip 0*/
	*(volatile unsigned long *)(0xbef19100 + cs*4) = 0x000083cf; 

	/* See chapter: Memory and I/O Access of XLS PRM */
	/* flash bar */
//	*(volatile unsigned long *)(0xbef00068) = 0x001c0021; 

	nand_readid(id_buf);

	if (id_buf[0] == NAND_ID_MICRON) {/* micron nand flash */

		memset(nand,0,sizeof(*nand));

		nand->iobase = iobase;
		nand->priv = (void *)this_flash;

		nand->id = id_buf[0];

		fill_micron_nand_info(nand, id_buf);

		sprintf(this_flash->name,"%s",nand->name);
		this_flash->iobase = iobase;
		this_flash->flash_type = NAND_FLASH;
		this_flash->size   = nand->size;
		this_flash->reset = nand_flash_reset;
		this_flash->erase = nand_flash_erase;
		this_flash->write = nand_flash_write;
		this_flash->read = nand_flash_read;
		this_flash->iprint = print_nand_flash_info;
		this_flash->flash_id = nand->id;
		this_flash->nr_blocks = nand->nr_blocks;
		this_flash->nr_pages = nand->nr_pages;
		this_flash->priv  = (void *)nand;

		cpld_enable_nand_write(1);
		this_flash->iprint(this_flash);
		return 0;
	}
	printf("this device is currently not supported by Nand driver \n");
	return -1;
}
