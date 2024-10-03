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
#include "micron_nand.h"

#include "rmi/mipscopreg.h"
#include "rmi/mips-exts.h"
#include "rmi/rmi_processor.h"
#include "rmi/cache.h"
#include "rmi/nand_bad_info.h"

#define BOOT2_LOAD_ADDRESS 0xac100000
#define BOOT_LOAD_SUCCESSFUL 5
#define BOOT_BLOCK_LOADED    6
#define BOOT_MAGIC_NOT_FOUND 7
#define BOOT_PAGE_READ_FAILED -1
#define BOOT_MAGIC_INCORRECT  -2
#define BOOT_ECC_INCORRECT    -3
#define BOOT_BLOCKS_OUT_OF_ORDER -4

/* 3 bytes of ecc per 256 bytes block of data */
#define ECC_BLOCK_SIZE 256
#define ECC_PER_BLOCK 3

/* Note: Use only one arg after the fmt.
 * This is a primitive printf 
 */
extern int uart_printf(char *fmt, ...);

#ifdef VERBOSE0
#define debug_printf(fmt, ...)
#else
#define debug_printf uart_printf
#endif


#define COP_0_OSSCRATCH $22
#define read_64bit_cp0_register_sel(source, sel)                \
	({ unsigned long long __res;                                    \
	 __asm__ __volatile__(                                   \
		 ".set\tpush\n\t"					\
		 ".set mips64\n\t"                                       \
		 "dmfc0\t%0,"STR(source)", %1\n\t"                        \
		 ".set\tpop"						\
		 : "=r" (__res) : "i" (sel) );                           \
	 __res;})


static int nand_flash_load_block(unsigned long start_addr, 
		unsigned long curr_block, unsigned long *expected_block);
static int nand_flash_page_read(unsigned long pagenum, unsigned char *dst, unsigned long nBytes);
static int load_boot2(unsigned long start_addr, int start_block, int num_blocks);
static void nand_flash_init(void);
int nand_reset(void);
int nand_calculate_ecc(const unsigned char *dat, unsigned char *ecc_code);
int nand_correct_data(unsigned char *dat, unsigned char *read_ecc, unsigned char *calc_ecc);


void *memcpy(void *dest, const void *src, size_t n)
{
	volatile unsigned char *d,*s;
	int i;
	d = (unsigned char *)dest;
	s = (unsigned char *)src;
	for (i = 0; i < n; i++) {
		d[i] = s[i];
	}
	return dest;
}

int my_delay(int i)
{
	int ret =0,j;

	for (j=0; j < i; j++)
		ret += j;
	return ret;
}

int nor_flash_present(void)
{
	int is_nor;
	is_nor = (int)read_64bit_cp0_register_sel(COP_0_OSSCRATCH, 3);
	if (is_nor == 1)
		return 1;
	else 
		return 0;
}

int load_nor_backup(void)
{
	unsigned long r1, r2;
	int allfs;
	int size = 0x300000;
	int i, j, k;

	debug_printf("Loading boot2 from NOR flash backup...\n");

	r1 = *(volatile unsigned long *)(0xbef19000); /* cs0 base */
	r2 = *(volatile unsigned long *)(0xbef19040); /* cs0 mask */

	*(volatile unsigned long *)(0xbef19000) = 0; /* cs0 base */
	*(volatile unsigned long *)(0xbef19040) = 0xff; /* cs0 mask */

	unsigned char *src = (unsigned char *)0xbf440000;
	unsigned char *dst = (unsigned char *)0xac100000;

	memcpy(dst, src, size);
	/* copied 3 mb. now check for all F's */
	allfs = 0;
	for (i = 0; i < size ; i  += 0x10000) {
		allfs = 0;
		for (j = 0; j < 0x10000; j++)
			if (dst[i + j] == 0xff)
				allfs++;
		if (allfs == 0x10000) {
			for (k = i; k < 0x300000; k++)
				dst[k] = 0;
			break;
		}
	}

	*(volatile unsigned long *)(0xbef19000) = r1; /* cs0 base */
	*(volatile unsigned long *)(0xbef19040) = r2; /* cs0 mask */
	return 0;
}

int boot1_75_start(void)
{

	int retval = 0;
	void (*go_to_boot2_fn)(void) = NULL;
	unsigned long r1, r2, r3;
	int boot_from_backup;

	go_to_boot2_fn = (void (*)(void))(unsigned long)
		read_64bit_cp0_register_sel(COP_0_OSSCRATCH, 1);

	if (nor_flash_present()) {
		load_nor_backup();
		debug_printf("Boot2 loaded from NOR flash backup.\n");
		go_to_boot2_fn();
		return 0;
	}

	boot_from_backup = (unsigned int)
		read_64bit_cp0_register_sel(COP_0_OSSCRATCH, 2);

	r1 = *(volatile unsigned long *)(0xbef19080);
	r2 = *(volatile unsigned long *)(0xbef190c0);
	r3 = *(volatile unsigned long *)(0xbef19100);

	write_32bit_cp0_register(COP_0_STATUS, STAT_0_CU0 | STAT_0_CU2);
	write_32bit_cp0_register(COP_0_CAUSE, 0);
	debug_printf("Loading boot2 from nand flash...\n");
	nand_flash_init();
	nand_reset();
	retval = my_delay(10000000);

	nand_flash_page_read(80*64, (char *)BOOT2_LOAD_ADDRESS, NAND_PAGE_SIZE_BYTE);
	nand_flash_page_read(80*64, (char *)BOOT2_LOAD_ADDRESS, NAND_PAGE_SIZE_BYTE);
	nand_flash_page_read(1 * 64, (char *)BOOT2_LOAD_ADDRESS, NAND_PAGE_SIZE_BYTE);
	nand_flash_page_read(1 * 64, (char *)BOOT2_LOAD_ADDRESS, NAND_PAGE_SIZE_BYTE);
	nand_flash_page_read(0, (char *)BOOT2_LOAD_ADDRESS, NAND_PAGE_SIZE_BYTE);

	if (boot_from_backup == 1) {
		retval = load_boot2(BOOT2_LOAD_ADDRESS, 33,24);
		if (retval == BOOT_LOAD_SUCCESSFUL) {
			debug_printf("Boot2 loaded from backup bootloader\n");
		} else {
			uart_printf("Backup bootloader possibly corrupted. \n");
		}
		goto this_out;
	}

	retval = load_boot2(BOOT2_LOAD_ADDRESS, 1,24);
	if (retval != BOOT_LOAD_SUCCESSFUL) {
		/* fall back to backup bootloader */
		retval = load_boot2(BOOT2_LOAD_ADDRESS, 33,24);
		if (retval == BOOT_LOAD_SUCCESSFUL) {
			debug_printf("Boot2 loaded from backup bootloader\n");
		}
		else  {
			uart_printf("Bootloader possibly corrupted. \n");
			uart_printf("Please burn correct bootloader for nand flash\n");
		}
	} else {
		debug_printf("Boot2 loaded from primary bootloader\n");
	}

this_out:

	*(volatile unsigned long *)(0xbef19080) = r1;
	*(volatile unsigned long *)(0xbef190c0) = r2;
	*(volatile unsigned long *)(0xbef19100) = r3; 

	go_to_boot2_fn();
	return 0;
}

static int load_boot2(unsigned long start_addr, int start_block, int num_blocks)
{
	int i;
	int retval=0;
	unsigned long expected_block = 0;
	int out_of_order_retry = 0;

	for (i = start_block; i < start_block + num_blocks; i++) {
		retval = nand_flash_load_block(start_addr, i, &expected_block);
		if (retval == BOOT_LOAD_SUCCESSFUL)
			return retval;;
		if (retval == BOOT_BLOCK_LOADED)
			continue;
		if (retval == BOOT_PAGE_READ_FAILED) /* block might be bad */
			continue;
		if (retval == BOOT_MAGIC_NOT_FOUND) /* block might not be used
						       because of error */
			continue;

		if (retval == BOOT_MAGIC_INCORRECT) /* block is not loaded
						       completely */
			continue;
		if (retval == BOOT_ECC_INCORRECT) /* ecc for a page is
						     incorrect... block is not
						     loaded completely */
			continue;
		if (retval == BOOT_BLOCKS_OUT_OF_ORDER) {/* blocks are out of
							   order... cannot
							   continue */
			if (out_of_order_retry== 0) {
				i = start_block - 1;
				out_of_order_retry = 1;
				expected_block = 0;
				continue;
			} else
			return retval;
		}

	}
	return retval;
}

/* return 0 for success, -1 for failure */
static int nand_flash_load_block(unsigned long start_addr, 
		unsigned long curr_block, unsigned long *expected_block)
{
	char dst[2200];
	int i=0,last_block = 0;
	struct nand_extra_bytes *extra;
	int retval;

	extra = (struct nand_extra_bytes *)&dst[2048];

	/* this is the first page of the block */
	retval = nand_flash_page_read(curr_block * NAND_PAGE_COUNT_PER_BLOCK, dst, NAND_PAGE_SIZE_BYTE);
	if (retval == 1 || retval  == 2) {
		uart_printf("[NAND]: Could not read Page %d, ",i);
		uart_printf("Block %d\n", curr_block);
		return BOOT_PAGE_READ_FAILED;
	}
	if (retval == -1) {
		debug_printf("[NAND]: Unrecoverable ECC Error.\n");
        debug_printf("\tPage %d, Block %d\n", i, curr_block);
		return BOOT_ECC_INCORRECT;
	}
#if 0
	debug_printf("boot sig for block %d is ",(int)curr_block);
	debug_printf("%x ",(unsigned int)extra->boot_sig);
	debug_printf("block = %d\n",(int)extra->block_num);
#endif
	if (extra->boot_sig == BOOT_NAND_MAGIC) {

		if (extra->block_num & BOOT_LAST_BLOCK) {
			last_block = 1;
			extra->block_num &= ~(BOOT_LAST_BLOCK);
		}

		if (extra->block_num != *expected_block) {
			debug_printf("[NAND]: Incorrect expected block# [%d]\n", extra->block_num);
			debug_printf("[NAND]: Expected block# [%d]\n", *expected_block);
			return BOOT_BLOCKS_OUT_OF_ORDER;
		} else {
			*expected_block = extra->block_num + 1;
		}

		/* get the position of the block */
		start_addr += (extra->block_num) 
			* NAND_PAGE_COUNT_PER_BLOCK * 2048;
		memcpy((unsigned char *)start_addr, dst, 2048);
		start_addr += 2048;
	} else {
		uart_printf("[NAND]: Incorrect boot sig for block %d. Returning.\n",(int)curr_block);
		return BOOT_MAGIC_NOT_FOUND;
	}

	for(i =  1; i < NAND_PAGE_COUNT_PER_BLOCK; i++) {

		retval = nand_flash_page_read(i + curr_block * NAND_PAGE_COUNT_PER_BLOCK, dst, NAND_PAGE_SIZE_BYTE);
		if (retval == 1 || retval  == 2) {
			uart_printf("[NAND]: Could not read Page %d, ",i);
			uart_printf("Block %d\n", curr_block);
			*expected_block--;
			return BOOT_PAGE_READ_FAILED;
		}
		if (retval == -1) {
			debug_printf("[NAND]: Unrecoverable ECC Error. Page %d, ",i);
			debug_printf("Block %d\n", curr_block);
			*expected_block--;
			return BOOT_ECC_INCORRECT;
		}
		if (extra->boot_sig == BOOT_NAND_MAGIC) {
			memcpy((unsigned char *)start_addr, dst, 2048);
			start_addr += 2048;
		} else {
			*expected_block--;
			return BOOT_MAGIC_INCORRECT;
		}
	}

	if (last_block) 
		return BOOT_LOAD_SUCCESSFUL;
	else 
		return BOOT_BLOCK_LOADED;
}

static int nand_flash_page_read(unsigned long pagenum, unsigned char *dst, unsigned long nBytes)
{
	unsigned char ecc_buf[ECC_PER_BLOCK];
	int i;
	int retval;
	struct nand_extra_bytes *extra;

	extra = (struct nand_extra_bytes *)&dst[2048];

	retval = nand_read_page(pagenum, 0, nBytes, dst);
	if (retval)
		return retval;
	for ( i = 0; i < 8; i++) {
		nand_calculate_ecc(&dst[i*ECC_BLOCK_SIZE], ecc_buf);
		retval = nand_correct_data(&dst[i*ECC_BLOCK_SIZE],
				&(extra->ecc[i*ECC_PER_BLOCK]),ecc_buf);
		if (retval == -1)
			return retval; /* -1: unrecoverable error */
		/* 0:no error 1:error corrected */
	}
	return 0;
}


static void nand_flash_init(void)
{
	/* 
	 * See chapter: Flash/PCMCIA Memory and the peripherals interface of XLS PRM
	 */
	int retval;
	char id_buf[4];

	/*dev param chip 0*/
	*(volatile unsigned long *)(0xbef19080) = 0x000041e0;
	/*time paramA chip 0*/
	*(volatile unsigned long *)(0xbef190c0) = 0x4f400e22;
	/*time paramB chip 0*/
	*(volatile unsigned long *)(0xbef19100) = 0x000083cf; 

	retval = nand_readid(id_buf);
	/* debug_printf("nand flash read id value is %d \n",(int)id_buf[0]); */

}
