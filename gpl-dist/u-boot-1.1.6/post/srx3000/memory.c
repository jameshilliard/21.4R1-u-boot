/*
 * Copyright (c) 2009-2011, Juniper Networks, Inc.
 * All rights reserved.
 *
 * (C) Copyright 2002
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>

/* Memory test
 *
 * General observations:
 * o The recommended test sequence is to test the data lines: if they are
 *   broken, nothing else will work properly.  Then test the address
 *   lines.  Finally, test the cells in the memory now that the test
 *   program knows that the address and data lines work properly.
 *   This sequence also helps isolate and identify what is faulty.
 *
 * o For the address line test, it is a good idea to use the base
 *   address of the lowest memory location, which causes a '1' bit to
 *   walk through a field of zeros on the address lines and the highest
 *   memory location, which causes a '0' bit to walk through a field of
 *   '1's on the address line.
 *
 * o Floating buses can fool memory tests if the test routine writes
 *   a value and then reads it back immediately.  The problem is, the
 *   write will charge the residual capacitance on the data bus so the
 *   bus retains its state briefely.  When the test program reads the
 *   value back immediately, the capacitance of the bus can allow it
 *   to read back what was written, even though the memory circuitry
 *   is broken.  To avoid this, the test program should write a test
 *   pattern to the target location, write a different pattern elsewhere
 *   to charge the residual capacitance in a differnt manner, then read
 *   the target location back.
 *
 * o Always read the target location EXACTLY ONCE and save it in a local
 *   variable.  The problem with reading the target location more than
 *   once is that the second and subsequent reads may work properly,
 *   resulting in a failed test that tells the poor technician that
 *   "Memory error at 00000000, wrote aaaaaaaa, read aaaaaaaa" which
 *   doesn't help him one bit and causes puzzled phone calls.  Been there,
 *   done that.
 *
 * Data line test:
 * ---------------
 * This tests data lines for shorts and opens by forcing adjacent data
 * to opposite states. Because the data lines could be routed in an
 * arbitrary manner the must ensure test patterns ensure that every case
 * is tested. By using the following series of binary patterns every
 * combination of adjacent bits is test regardless of routing.
 *
 *     ...101010101010101010101010
 *     ...110011001100110011001100
 *     ...111100001111000011110000
 *     ...111111110000000011111111
 *
 * Carrying this out, gives us six hex patterns as follows:
 *
 *     0xaaaaaaaaaaaaaaaa
 *     0xcccccccccccccccc
 *     0xf0f0f0f0f0f0f0f0
 *     0xff00ff00ff00ff00
 *     0xffff0000ffff0000
 *     0xffffffff00000000
 *
 * To test for short and opens to other signals on our boards, we
 * simply test with the 1's complemnt of the paterns as well, resulting
 * in twelve patterns total.
 *
 * After writing a test pattern. a special pattern 0x0123456789ABCDEF is
 * written to a different address in case the data lines are floating.
 * Thus, if a byte lane fails, you will see part of the special
 * pattern in that byte lane when the test runs.  For example, if the
 * xx__xxxxxxxxxxxx byte line fails, you will see aa23aaaaaaaaaaaa
 * (for the 'a' test pattern).
 *
 * Address line test:
 * ------------------
 *  This function performs a test to verify that all the address lines
 *  hooked up to the RAM work properly.  If there is an address line
 *  fault, it usually shows up as two different locations in the address
 *  map (related by the faulty address line) mapping to one physical
 *  memory storage location.  The artifact that shows up is writing to
 *  the first location "changes" the second location.
 *
 * To test all address lines, we start with the given base address and
 * xor the address with a '1' bit to flip one address line.  For each
 * test, we shift the '1' bit left to test the next address line.
 *
 * In the actual code, we start with address sizeof(ulong) since our
 * test pattern we use is a ulong and thus, if we tried to test lower
 * order address bits, it wouldn't work because our pattern would
 * overwrite itself.
 *
 * Example for a 4 bit address space with the base at 0000:
 *   0000 <- base
 *   0001 <- test 1
 *   0010 <- test 2
 *   0100 <- test 3
 *   1000 <- test 4
 * Example for a 4 bit address space with the base at 0010:
 *   0010 <- base
 *   0011 <- test 1
 *   0000 <- (below the base address, skipped)
 *   0110 <- test 2
 *   1010 <- test 3
 *
 * The test locations are successively tested to make sure that they are
 * not "mirrored" onto the base address due to a faulty address line.
 * Note that the base and each test location are related by one address
 * line flipped.  Note that the base address need not be all zeros.
 *
 * Memory tests 1-4:
 * -----------------
 * These tests verify RAM using sequential writes and reads
 * to/from RAM. There are several test cases that use different patterns to
 * verify RAM. Each test case fills a region of RAM with one pattern and
 * then reads the region back and compares its contents with the pattern.
 * The following patterns are used:
 *
 *  1a) zero pattern (0x00000000)
 *  1b) negative pattern (0xffffffff)
 *  1c) checkerboard pattern (0x55555555)
 *  1d) checkerboard pattern (0xaaaaaaaa)
 *  2)  bit-flip pattern ((1 << (offset % 32))
 *  3)  address pattern (offset)
 *  4)  address pattern (~offset)
 *
 * Being run in normal mode, the test verifies only small 4Kb
 * regions of RAM around each 1Mb boundary. For example, for 64Mb
 * RAM the following areas are verified: 0x00000000-0x00000800,
 * 0x000ff800-0x00100800, 0x001ff800-0x00200800, ..., 0x03fff800-
 * 0x04000000. If the test is run in slow-test mode, it verifies
 * the whole RAM.
 */

#ifdef CONFIG_POST

#include <post.h>
#include <watchdog.h>

#if CONFIG_POST & CFG_POST_MEMORY
	static int memory_diag_flag ;
	static int mem_flag;
	static int mem_tests_flag;
	int boot_flag_post;
	DECLARE_GLOBAL_DATA_PTR;
volatile u_int32_t   *parkes_sc_pad_mem = (u_int32_t *)
	(CFG_PARKES_BASE + 0x04);
#define rom_flag (!(gd->flags & GD_FLG_RELOC))
	int post_result_mem;
#define ZERO_PATTERN        0x00000000
#define NEGATIVE_PATTERN    0xffffffff
#define TWO_KB_LEN          0x800
#define UP_2KB_OFFSET       0xff800
#define TEST_ONE_MB         0x100000
/*
 * Define INJECT_*_ERRORS for testing error detection in the presence of
 * _good_ hardware.
 */
#undef  INJECT_DATA_ERRORS
#undef  INJECT_ADDRESS_ERRORS

#ifdef INJECT_DATA_ERRORS
#warning "Injecting data line errors for testing purposes"
#endif

#ifdef INJECT_ADDRESS_ERRORS
#warning "Injecting address line errors for testing purposes"
#endif

/*
 * This function performs a double word move from the data at
 * the source pointer to the location at the destination pointer.
 * This is helpful for testing memory on processors which have a 64 bit
 * wide data bus.
 *
 * On those PowerPC with FPU, use assembly and a floating point move:
 * this does a 64 bit move.
 *
 * For other processors, let the compiler generate the best code it can.
 */
static void 
move64 (unsigned long long *src, unsigned long long *dest)
{
#if defined(CONFIG_MPC8260) || defined(CONFIG_MPC824X)
	asm ("lfd  0, 0(3)\n\t" /* fpr0	  =  *scr	*/
			"stfd 0, 0(4)"		/* *dest  =  fpr0	*/
			: : : "fr0" );		/* Clobbers fr0		*/
	return;
#else
	*dest = *src;
#endif
}

/*
 * This is 64 bit wide test patterns.  Note that they reside in ROM
 * (which presumably works) and the tests write them to RAM which may
 * not work.
 *
 * The "otherpattern" is written to drive the data bus to values other
 * than the test pattern.  This is for detecting floating bus lines.
 *
 */
const static unsigned long long pattern[] = {
	0xaaaaaaaaaaaaaaaaULL,
	0xccccccccccccccccULL,
	0xf0f0f0f0f0f0f0f0ULL,
	0xff00ff00ff00ff00ULL,
	0xffff0000ffff0000ULL,
	0xffffffff00000000ULL,
	0x00000000ffffffffULL,
	0x0000ffff0000ffffULL,
	0x00ff00ff00ff00ffULL,
	0x0f0f0f0f0f0f0f0fULL,
	0x3333333333333333ULL,
	0x5555555555555555ULL
};
const unsigned long long otherpattern = 0x0123456789abcdefULL;

static int
memory_post_dataline (unsigned long long * pmem)
{
	unsigned long long temp64 = 0;
	int num_patterns          = sizeof(pattern)/ sizeof(pattern[0]);
	int i                     = 0 ;
	unsigned int hi, lo, pathi, patlo;
	int ret = 0;

	for (i = 0; i < num_patterns; i++) {
		move64((unsigned long long *)&(pattern[i]), pmem++);
		/*
		 * Put a different pattern on the data lines: otherwise they
		 * may float long enough to read back what we wrote.
		 */
		move64((unsigned long long *)&otherpattern, pmem--);
		move64(pmem, &temp64);

#ifdef INJECT_DATA_ERRORS
		temp64 ^= 0x00008000;
#endif
		gd->memory_addr = pmem;
		*parkes_sc_pad_mem = pmem;
		if (temp64 != pattern[i]) {
			pathi = (pattern[i]>>32) & 0xffffffff;
			patlo = pattern[i] & 0xffffffff;

			hi = (temp64>>32) & 0xffffffff;
			lo = temp64 & 0xffffffff;
			post_log("\n");
			mem_flag++;
			printf("Memory (date line) error at %08x, "
				"wrote %08x%08x, read %08x%08x !\n",
				pmem, pathi, patlo, hi, lo);
			ret = -1;
		}
	}
	if (ret == -1) {
		mem_tests_flag = 1;
	}
	return ret;
}

static int
memory_post_addrline (ulong *testaddr, ulong *base, ulong size)
{
	ulong *target;
	ulong *end;
	ulong readback;
	ulong xor;
	int   ret = 0;

	end = (ulong *)((ulong)base + size);	/* pointer arith! */
	xor = 0;
	for (xor = sizeof(ulong); xor > 0; xor <<= 1) {
		gd->memory_addr = target;
		*parkes_sc_pad_mem = target;
		target = (ulong *)((ulong)testaddr ^ xor);
		if ((target >= base) && (target < end)) {
			*testaddr = ~*target;
			readback  = *target;

#ifdef INJECT_ADDRESS_ERRORS
			if (xor == 0x00008000) {
				readback = *testaddr;
			}
#endif
			if (readback == *testaddr) {
				post_log("\n");
				mem_flag++;
				printf("Memory (address line) error at %08x<->%08x, "
					"XOR value %08x !\n",
					testaddr, target, xor);
				ret = -1;
			}
		}
	}
	if (ret == -1) {
		mem_tests_flag = 2;
	}
	return ret;
}

static int
memory_post_test1 (unsigned long start, unsigned long size, unsigned long val)
{
	unsigned long i = 0;
	ulong *mem      = (ulong *)start;
	ulong readback  = 0;
	int ret         = 0;

	for (i = 0; i < size / sizeof(ulong); i++) {
		mem[i] = val;
		if (i % 1024 == 0) {
			WATCHDOG_RESET();
		}
	}

	for (i = 0; i < size / sizeof(ulong) && ret == 0; i++) {
		gd->memory_addr = mem + i;
		*parkes_sc_pad_mem = mem + i;
		readback = mem[i];
		if (readback != val) {
			post_log("\n");
			mem_flag++;
			printf("Memory error at %08x, wrote %08x, read %08x !\n",
				mem + i, val, readback);

			ret = -1;
		}
		if (i % 1024 == 0)
			WATCHDOG_RESET();
	}
	if (ret == -1) {
		mem_tests_flag = 3;
	}
	return ret;
}

static int
memory_post_test2 (unsigned long start, unsigned long size)
{
	unsigned long  i = 0;
	ulong   *mem     = (ulong *) start;
	ulong readback   = 0;
	int ret          = 0;

	for (i = 0; i < size / sizeof(ulong); i++) {
		mem[i] = 1 << (i % 32);
		if (i % 1024 == 0) {
			WATCHDOG_RESET();
		}
	}

	for (i = 0; i < size / sizeof(ulong) && ret == 0; i++) {
		gd->memory_addr = mem + i;
		*parkes_sc_pad_mem = mem + i;
		readback = mem[i];
		if (readback != (1 << (i % 32))) {
			post_log("\n");
			mem_flag++;
			printf("Memory error at %08x, "
				"wrote %08x, read %08x !\n",
				mem + i, 1 << (i % 32), readback);

			ret = -1;
		}
		if (i % 1024 == 0) {
			WATCHDOG_RESET();
		}
	}
	if (ret == -1) {
		mem_tests_flag = 4;
	}
	return ret;
}

static int
memory_post_test3 (unsigned long start, unsigned long size)
{
	unsigned long  i = 0;
	ulong *mem       = (ulong *)start;
	ulong readback   = 0;
	int ret = 0;

	for (i = 0; i < size / sizeof(ulong); i++) {
		mem[i] = i;
		if (i % 1024 == 0) {
			WATCHDOG_RESET();
		}
	}

	for (i = 0; i < size / sizeof(ulong) && ret == 0; i++) {
		gd->memory_addr = mem + i;
		*parkes_sc_pad_mem = mem + i;
		readback = mem[i];
		if (readback != i) {
			post_log("\n");
			mem_flag++;
			printf("Memory error at %08x, wrote %08x, read %08x !\n",
				mem + i, i, readback);

			ret = -1;
		}
		if (i % 1024 == 0) {
			WATCHDOG_RESET();
		}
	}
	if (ret == -1) {
		mem_tests_flag = 5;
	}
	return ret;
}

static int
memory_post_test4 (unsigned long start, unsigned long size)
{
	unsigned long i = 0;
	ulong *mem      = (ulong *) start;
	ulong readback  = 0;
	int ret         = 0;

	for (i = 0; i < size / sizeof(ulong); i++) {
		mem[i] = ~i;
		if (i % 1024 == 0) {
			WATCHDOG_RESET();
		}
	}

	for (i = 0; i < size / sizeof(ulong) && ret == 0; i++) {
		gd->memory_addr = mem + i;
		*parkes_sc_pad_mem = mem + i;
		readback = mem[i];
		if (readback != ~i) {
			post_log("\n");
			mem_flag++;
			printf("Memory error at %08x, wrote %08x, read %08x !\n",
				mem + i, ~i, readback);
			ret = -1;
		}
		if (i % 1024 == 0) {
			WATCHDOG_RESET();
		}
	}
	if (ret == -1) {
		mem_tests_flag = 6;
	}
	return ret;
}

#define COUNT_RESET_WDT_VAL 1024
static int
memory_post_tests_addr_value_scan_upper_onemb_flash
(unsigned long start, unsigned long size) {
	int ret = 0;
	unsigned long i = 0;
	ulong *mem      = (ulong *)start;
	unsigned long memend = gd->ram_size-4;
	printf("Memory test running complete 1MB before relocating to RAM\n");
	int k = 0;
	WATCHDOG_RESET();
	for (i = 0; i <= (size / sizeof(ulong)); i++) {
		*parkes_sc_pad_mem = (mem + i);
		mem[i] = (mem + i);
		/* Last memory location tested for 1GB DDR */
		if (mem[i] == memend) {
			printf("Memory at %08lx, wrote %08lx\n",
					mem + i, mem + i);
			break;
		}
		if (k == 2500) {
			printf("Memory at %08lx, wrote %08lx\n",
					mem + i, mem + i);
			k = 0;
		}
		k++;
		if (i % COUNT_RESET_WDT_VAL == 0) {
			WATCHDOG_RESET();
		}
	}

	k = 0;
	for (i = 0; i <= (size / sizeof(ulong)); i++) {
		*parkes_sc_pad_mem = (mem + i);
		if (mem[i] != (mem +i)) {
			post_log("\n");
			printf("Memory at %08lx, wrote %08lx, read %08lx mismatch!\n",
					mem + i, mem + i, mem[i]);
			ret = -1;
			post_log("\n");
		}
		/* Last memory location tested for 1GB DDR */
		if (mem[i] == memend) {
			printf("Memory at %08lx, Read %08lx\n", mem + i, mem[i]);
			*parkes_sc_pad_mem = 0x55;
			break;
		}

		if (k == 2500) {
			printf("Memory at %08lx, Read %08lx\n", mem + i, mem[i]);
			k = 0;
		}
		k++;

		if (i % COUNT_RESET_WDT_VAL == 0) {
			WATCHDOG_RESET();
		}
	}
	return ret;
}
static int
memory_post_tests_addr_value (unsigned long start, unsigned long size) {
	unsigned long i = 0;
	int ret = 0;
	ulong *mem      = (ulong *)start;
	int k = 0;
	unsigned long memend = gd->ram_size-4;
	WATCHDOG_RESET();
	for (i = 0; i <= (size / sizeof(ulong)); i++) {
		gd->memory_addr = (mem + i);
		*parkes_sc_pad_mem = (mem + i);
		mem[i] = (mem + i);
		if (mem[i] == memend) {
			break;
		}
		if (i % COUNT_RESET_WDT_VAL == 0) {
			WATCHDOG_RESET();
		}
	}
	for (i = 0; i <= (size / sizeof(ulong)); i++) {
		gd->memory_addr = (mem + i);
		*parkes_sc_pad_mem = (mem + i);
		if (mem[i] != (mem +i)) {
			post_log("\n");
			printf("Memory at %08lx, wrote %08lx, read %08lx mismatch!\n",
					mem + i, mem + i, mem[i]);
			ret = -1;
			post_log("\n");
		}
		/* Last memory location tested for 1GB DDR */
		if (mem[i] == memend) {
			gd->memory_addr = 0x55;
			*parkes_sc_pad_mem = 0x55;
			break;
		}
		if (i % COUNT_RESET_WDT_VAL == 0) {
			WATCHDOG_RESET();
		}
	}
	return ret;
}

static int
memory_post_tests_allmb_addr_value (unsigned long start, unsigned long size) {
	int ret = 0;
	unsigned long i = 0;
	ulong *mem      = (ulong *)start;
	int k = 0;
	WATCHDOG_RESET();
	printf("The size %d\n",(size / sizeof(ulong)));
	for (i = 0; i <= (size / sizeof(ulong)); i++) {
		*parkes_sc_pad_mem = (mem + i);
		gd->memory_addr = (mem + i);
		mem[i] = (mem + i);
		/* Last memory location tested after the alloc bdinfo */
		if (mem[i] == 0x3FE00000) {
			printf("Memory at %08lx, wrote %08lx\n",
					mem + i, mem + i);
			break;
		}
		if (k == 250000) {
			printf("Memory at %08lx, wrote %08lx\n",
					mem + i, mem + i);
			k = 0;
		}
		k++;
		if (i % COUNT_RESET_WDT_VAL == 0) {
			WATCHDOG_RESET();
		}
		if (ctrlc()) {
			putc('\n');
			break;
		}
	}
	k = 0;
	for (i = 0; i <= (size / sizeof(ulong)); i++) {
		*parkes_sc_pad_mem = (mem + i);
		gd->memory_addr = (mem + i);
		if (mem[i] != (mem +i)) {
			post_log("\n");
			printf("Memory at %08lx, wrote %08lx, read %08lx mismatch!\n",
					mem + i, mem + i, mem[i]);
			ret = -1;
			post_log("\n");
		}
		/* Last memory location tested after the alloc bdinfo */
		if (mem[i] == 0x3FE00000) {
			gd->memory_addr = 0x55;
			*parkes_sc_pad_mem = 0x55;
			printf("Memory at %08lx, Read %08lx\n", mem + i, mem[i]);
			break;
		}
		if (k == 250000) {
			printf("Memory at %08lx, Read %08lx\n", mem + i, mem[i]);
			k = 0;
		}
		k++;
		if (i % COUNT_RESET_WDT_VAL == 0) {
			WATCHDOG_RESET();
		}
		if (ctrlc()) {
			putc('\n');
			break;
		}
	}
	return ret;
}

int
memory_post_tests (unsigned long start, unsigned long size)
{
	int ret = 0;
	ret = memory_post_dataline((unsigned long long *)start);

	WATCHDOG_RESET();
	if (ret == 0) {
		ret = memory_post_addrline((ulong *)start, (ulong *)start, size);
	}
	WATCHDOG_RESET();
	if (ret == 0) {
		ret = memory_post_addrline((ulong *)(start + size - 8),
					(ulong *)start, size);
	}
	WATCHDOG_RESET();
	if (ret == 0) {
		ret = memory_post_test1(start, size, ZERO_PATTERN);
	}
	WATCHDOG_RESET();
	if (ret == 0) {
		ret = memory_post_test1(start, size, NEGATIVE_PATTERN);
	}
	WATCHDOG_RESET();
	if (ret == 0) {
		ret = memory_post_test1(start, size, CHECKERBOARD_PATTERN_55);
	}
	WATCHDOG_RESET();
	if (ret == 0) {
		ret = memory_post_test1(start, size, CHECKERBOARD_PATTERN_AA);
	}
	WATCHDOG_RESET();
	if (ret == 0) {
		ret = memory_post_test2(start, size);
	}
	WATCHDOG_RESET();
	if (ret == 0) {
		ret = memory_post_test3(start, size);
	}
	WATCHDOG_RESET();
	if (ret == 0) {
		ret = memory_post_test4(start, size);
	}
	WATCHDOG_RESET();

	return ret;
}


int memory_post_test_diag (int flags)
{
	int ret          = 0;
	mem_flag         = 0;
	memory_diag_flag = 0;
	mem_tests_flag   = 0;
	boot_flag_post   = 0;
	post_result_mem  = 0;
	unsigned long memsize = gd->ram_size - (1 << 20);
	unsigned long memend = gd->ram_size-4;

	if (flags & POST_DEBUG) {
		memory_diag_flag =1;
	}

	if (flags & BOOT_POST_FLAG) {
		boot_flag_post = 1;
	}
	if (rom_flag == 0) {
		if (!boot_flag_post) {
			post_log(" The size of memory %lu\n",memsize);
		}
	}
	if (flags & POST_SLOWTEST) {
		if (rom_flag == 0) {
			post_log("In slowtest mode\n");
		}
		if (rom_flag) {
			/* last 1M */
			ret = memory_post_tests(memsize, 0x100000);
			if (ret == -1) {
				printf("Failed memory test\n");
			}
		} else {
			ret = memory_post_tests(CFG_SDRAM_BASE, memsize);
		}
	} else {			/* POST_NORMAL */
		unsigned long i = 0;
		if (rom_flag == 0) {
			if (!boot_flag_post) {
				post_log("\n");
				post_log(" This tests the address line,data line");
				post_log(" with different patterns to verfiy the RAM\n");
			}
			for (i = 1; i < (memsize >> 20) && ret == 0; i++) {
				if (ret == 0) {
					ret = memory_post_tests_addr_value(i << 20, TWO_KB_LEN);
				}
				if (ret == 0) {
					ret = memory_post_tests_addr_value(((i << 20) + UP_2KB_OFFSET),
							TWO_KB_LEN);
				}
				if (ret == 0) {
					ret = memory_post_tests(i << 20, TWO_KB_LEN);
				}
				if (ret == 0) {
					ret = memory_post_tests((i << 20) + TEST_ONE_MB, TWO_KB_LEN);
				}
				if ( ret == -1) {
					break;
				}
			}
			if (ret == -1) {
				/* Scan for complete memory */
				ret = memory_post_tests_allmb_addr_value(TEST_ONE_MB, memsize);
			}
			if (ret == 0) {
				gd->memory_addr = 0x00;
				*parkes_sc_pad_mem = 0x00;
			}
		} else {
			printf("Running Memory Test from ROM !\n");
			if (gd->memory_addr == 0xff) {
				ret = -1;
			}
			if (ret == 0) {
				ret = memory_post_tests_addr_value(memsize, TWO_KB_LEN);
			}
			printf(".");
			if (ret == 0) {
				ret = memory_post_tests_addr_value((memsize + UP_2KB_OFFSET),
						TWO_KB_LEN);
			}
			printf(".");
			if ( ret == -1) {
				/* Scan for complete 1mb */
				ret = memory_post_tests_addr_value_scan_upper_onemb_flash
					((memsize), TEST_ONE_MB);
			}
			
			printf(".");
			if (ret == 0) {
				ret = memory_post_tests(memsize, TWO_KB_LEN);
			}
			printf(".");
			if (ret == 0) {
				ret = memory_post_tests((memsize + UP_2KB_OFFSET), TWO_KB_LEN);
			}
			if (ret == 0) {
				gd->memory_addr = 0x00;
				*parkes_sc_pad_mem = 0x00;
			}
			printf(".\n");
		}
	}
	if (rom_flag == 0) {
		if (ret == 0) {
			if (!boot_flag_post) {
				post_log("\n");
				post_log("-------------  The memory diag test passed  -------\n");
			}
			if (boot_flag_post) {
				if (!mem_tests_flag) {
					return 0;
				} else {
					post_result_mem = -1;
					return -1;
				}
			}	
			if (memory_diag_flag) {
				if (mem_tests_flag == 0) {
					post_log("\n");
					post_log(" The address line test passed\n");
					post_log(" The data  line test passed\n");
					post_log(" The zero pattern test passed\n");
					post_log(" The negative pattern test passed\n");
					post_log(" The checkerboard pattern test passed\n");
					post_log(" The bit-flip pattern test passed\n");
					post_log(" The address pattern test passed\n");
				}
			}
		}
	}
	return ret;
}

#endif /* CONFIG_POST & CFG_POST_MEMORY */
#endif /* CONFIG_POST */
