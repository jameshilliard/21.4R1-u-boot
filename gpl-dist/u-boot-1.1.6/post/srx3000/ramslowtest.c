/*
 * Copyright (c) 2009-2010, Juniper Networks, Inc.
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
 *   pattern1 to the target location, write a different pattern1 elsewhere
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
 * arbitrary manner the must ensure test pattern1s ensure that every case
 * is tested. By using the following series of binary pattern1s every
 * combination of adjacent bits is test regardless of routing.
 *
 *     ...101010101010101010101010
 *     ...110011001100110011001100
 *     ...111100001111000011110000
 *     ...111111110000000011111111
 *
 * Carrying this out, gives us six hex pattern1s as follows:
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
 * in twelve pattern1s total.
 *
 * After writing a test pattern1. a special pattern1 0x0123456789ABCDEF is
 * written to a different address in case the data lines are floating.
 * Thus, if a byte lane fails, you will see part of the special
 * pattern1 in that byte lane when the test runs.  For example, if the
 * xx__xxxxxxxxxxxx byte line fails, you will see aa23aaaaaaaaaaaa
 * (for the 'a' test pattern1).
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
 * test pattern1 we use is a ulong and thus, if we tried to test lower
 * order address bits, it wouldn't work because our pattern1 would
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
 * to/from RAM. There are several test cases that use different pattern1s to
 * verify RAM. Each test case fills a region of RAM with one pattern1 and
 * then reads the region back and compares its contents with the pattern1.
 * The following pattern1s are used:
 *
 *  1a) zero pattern1 (0x00000000)
 *  1b) negative pattern1 (0xffffffff)
 *  1c) checkerboard pattern1 (0x55555555)
 *  1d) checkerboard pattern1 (0xaaaaaaaa)
 *  2)  bit-flip pattern1 ((1 << (offset % 32))
 *  3)  address pattern1 (offset)
 *  4)  address pattern1 (~offset)
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

#if CONFIG_POST & CFG_POST_MEMORY_SLOW_TEST_RAM
#define ZERO_pattern1        0x00000000
#define NEGATIVE_pattern1    0xffffffff
#define CHECKBOARD_pattern11 0x55555555
#define CHECKBOARD_pattern12 0xaaaaaaaa
#define DISP_MEM_COUNT_10MB 2500000
#define COUNT_RESET_WDT_VAL 1024
static int mem_fail_flag;
DECLARE_GLOBAL_DATA_PTR;

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
 * This is 64 bit wide test pattern1s.  Note that they reside in ROM
 * (which presumably works) and the tests write them to RAM which may
 * not work.
 *
 * The "otherpattern1" is written to drive the data bus to values other
 * than the test pattern1.  This is for detecting floating bus lines.
 *
 */
const static unsigned long long pattern1[] = {
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
const unsigned long long otherpattern1 = 0x0123456789abcdefULL;

static void
memory_post_dataline (unsigned long long *pmem)
{
	unsigned long long temp64 = 0;
	int num_pattern1s          = sizeof(pattern1)/ sizeof(pattern1[0]);
	int i                     = 0;
	unsigned int hi, lo, pathi, patlo;

	for (i = 0; i < num_pattern1s; i++) {
		move64((unsigned long long *)&(pattern1[i]), pmem++);
		/*
		 * Put a different pattern1 on the data lines: otherwise they
		 * may float long enough to read back what we wrote.
		 */
		move64((unsigned long long *)&otherpattern1, pmem--);
		move64(pmem, &temp64);

		if (temp64 != pattern1[i]) {
			pathi = (pattern1[i]>>32) & 0xffffffff;
			patlo = pattern1[i] & 0xffffffff;

			hi = (temp64>>32) & 0xffffffff;
			lo = temp64 & 0xffffffff;
			post_log("\n");
			mem_fail_flag++;
			printf("Memory (date line) error at %08x, "
                        "wrote %08x%08x, read %08x%08x !\n",
                         pmem, pathi, patlo, hi, lo);
		}
	}
}

static void
memory_post_addrline (ulong *testaddr, ulong *base, ulong size)
{
	ulong *target;
	ulong *end;
	ulong readback;
	ulong xor;

	end = (ulong *)((ulong)base + size); /* pointer arith! */
	xor = 0;
	for (xor = sizeof(ulong); xor > 0; xor <<= 1) {
		target = (ulong *)((ulong)testaddr ^ xor);
		if ((target >= base) && (target < end)) {
			*testaddr = ~*target;
			readback  = *target;

			if (readback == *testaddr) {
				post_log("\n");
				mem_fail_flag++;
				printf("Memory (address line) error at %08x<->%08x, "
                       "XOR value %08x !\n", testaddr, target, xor);
			}
		}
	}
}

static void
memory_post_test1 (unsigned long start, unsigned long size, unsigned long val)
{
	unsigned long i = 0;
	unsigned long k = 0;
	ulong *mem      = (ulong *)start;
	ulong readback  = 0;

	for (i = 0; i < (size / sizeof(ulong)); i++) {
		if (ctrlc()) {
			putc('\n');
			break;
		}
		mem[i] = val;
		k++;
		if (k == DISP_MEM_COUNT_10MB) {
			printf("Memory at %08x, wrote %08x\n", mem + i, mem[i]);
			k = 0;
		}
		if (size == (ulong)(mem +i)) {
			printf("Memory at %08x, wrote %08x\n", mem + i, mem[i]);
			break;
		}
		if (i % COUNT_RESET_WDT_VAL == 0) {
			WATCHDOG_RESET();
		}
	}

	k = 0;
	for (i = 0; i < (size / sizeof(ulong)); i++) {
		if (ctrlc()) {
			putc('\n');
			break;
		}
		readback = mem[i];
		k++;
		if (k == DISP_MEM_COUNT_10MB) {
			printf("Memory at %08x, Read %08x\n", mem + i, mem[i]);
			k = 0;
		}     
		if (size == (ulong)(mem +i)) {
			printf("Memory at %08x, Read %08x\n", mem + i, mem[i]);
			if (readback != val) {
				post_log("\n");
				mem_fail_flag++;
				printf("Memory error at %08x, wrote %08x !\n",
                               mem + i, mem[i]);
			}
			break;
		}
		if (readback != val) {
			post_log("\n");
			mem_fail_flag++;
			printf("Memory error at %08x, wrote %08x, read %08x !\n",
                           mem + i, val, readback);
		}
		if (i % COUNT_RESET_WDT_VAL == 0) {
			WATCHDOG_RESET();
		}
	}
}

static void
memory_post_test2 (unsigned long start, unsigned long size)
{
	unsigned long  i = 0;
	unsigned long  k = 0;
	ulong   *mem     = (ulong *)start;
	ulong readback   = 0;
	int ret          = 0;

	for (i = 0; i < size / sizeof(ulong); i++) {
		if (ctrlc()) {
			putc('\n');
			break;
		}
		mem[i] = 1 << (i % 32);
		k++;
		if (k == DISP_MEM_COUNT_10MB) {
			printf("Memory at %08x, wrote %08x\n", mem + i, mem[i]);
			k = 0;
		}
		if (size == (ulong)(mem +i)) {
			printf("Memory at %08x, wrote %08x\n", mem + i, mem[i]);
			break;
		}
		if (i % COUNT_RESET_WDT_VAL == 0) {
			WATCHDOG_RESET();
		}
	}

	k = 0;
	for (i = 0; i < size / sizeof(ulong) && ret == 0; i++) {
		if (ctrlc()) {
			putc('\n');
			break;
		}
		readback = mem[i];
		k++;
		if (k == DISP_MEM_COUNT_10MB) {
			printf("Memory at %08x, Read %08x\n", mem + i, mem[i]);
			k = 0;
		}     
		if (size == (ulong)(mem +i)) {
			printf("Memory at %08x, Read %08x\n", mem + i, mem[i]);
			if (readback != (1 << (i % 32))) {
				post_log("\n");
				mem_fail_flag++;
				printf("Memory error at %08x, wrote %08x !\n",
                               mem + i, 1 << (i % 32), readback);

				post_log("\n");
			}
			break;
		}
		if (readback != (1 << (i % 32))) {
			post_log("\n");
			mem_fail_flag++;
			printf("Memory error at %08x, wrote %08x, read %08x !\n",
                           mem + i, 1 << (i % 32), readback);
		}
		if (i % COUNT_RESET_WDT_VAL == 0) {
			WATCHDOG_RESET();
		}
	}
}

static void
memory_post_test3 (unsigned long start, unsigned long size)
{
	unsigned long  i = 0;
	unsigned long  k = 0;
	ulong *mem       = (ulong *)start;
	ulong readback   = 0;

	for (i = 0; i < (size / sizeof(ulong)); i++) {
		if (ctrlc()) {
			putc('\n');
			break;
		}
		mem[i] = i;
		k++;
		if (k == DISP_MEM_COUNT_10MB) {
			printf("Memory at %08x, wrote %08x\n", mem + i, mem[i]);
			k = 0;
		}
		if (size == (ulong)(mem +i)) {
			printf("Memory at %08x, wrote %08x\n", mem + i, mem[i]);
			break;
		}
		if (i % COUNT_RESET_WDT_VAL == 0) {
			WATCHDOG_RESET();
		}
	}

	k = 0;
	for (i = 0; i < (size / sizeof(ulong)); i++) {

		if (ctrlc()) {
			putc('\n');
			break;
		}
		readback = mem[i];
		k++;
		if (k == DISP_MEM_COUNT_10MB) {
			printf("Memory at %08x, Read %08x\n",mem + i, mem[i]);
			k = 0;
		}     
		if (size == (ulong)(mem +i)) {
			printf("Memory at %08x, Read %08x\n",mem + i, mem[i]);
			if (readback != i) {
				post_log("\n");
				mem_fail_flag++;
				printf("Memory error at %08x, wrote %08x, read %08x !\n",
                               mem + i, i, readback);
				post_log("\n");
			}
			break;
		}
		if (readback != i) {
			post_log("\n");
			mem_fail_flag++;
			printf("Memory error at %08x, wrote %08x, read %08x !\n",
                           mem + i, i, readback);
		}
		if (i % COUNT_RESET_WDT_VAL == 0) {
			WATCHDOG_RESET();
		}
	}
}

static void
memory_post_test4 (unsigned long start, unsigned long size)
{
	unsigned long i = 0;
	unsigned long k = 0;
	ulong *mem      = (ulong *)start;
	ulong readback  = 0;

	for (i = 0; i < (size / sizeof(ulong)); i++) {
		if (ctrlc()) {
			putc('\n');
			break;
		}
		mem[i] = ~i;
		k++;
		if (k == DISP_MEM_COUNT_10MB) {
			printf("Memory at %08x, wrote %08x\n", mem + i, mem[i]);
			k = 0;
		}
		if (size == (ulong)(mem +i)) {
			printf("Memory at %08x, wrote %08x\n", mem + i, mem[i]);
			break;
		}
		if (i % COUNT_RESET_WDT_VAL == 0) {
			WATCHDOG_RESET();
		}
	}

	k = 0;
	for (i = 0; i < (size / sizeof(ulong)); i++) {
		if (ctrlc()) {
			putc('\n');
			break;
		}
		readback = mem[i];
		k++;
		if (k == DISP_MEM_COUNT_10MB) {
			printf("Memory at %08x, Read %08x\n", mem + i, mem[i]);
			k = 0;
		}     
		if (size == (ulong)(mem +i)) {
			printf("Memory at %08x, Read %08x\n", mem + i, mem[i]);
			if (readback != ~i) {
				post_log("\n");
				mem_fail_flag++;
				printf("Memory error at %08x, wrote %08x, read %08x !\n",
                        mem + i, ~i, readback);
				post_log("\n");
			}
			break;
		}
		if (readback != ~i) {
			post_log("\n");
			mem_fail_flag++;
			printf("Memory error at %08x, wrote %08x, read %08x !\n",
                    mem + i, ~i, readback);
		}
		if (i % COUNT_RESET_WDT_VAL == 0) {
			WATCHDOG_RESET();
		}
	}
}

static void
memory_post_slow_ram_tests (unsigned long start, unsigned long size)
{
	memory_post_dataline((unsigned long long *)start);

	WATCHDOG_RESET();
	memory_post_addrline((ulong *)start, (ulong *)start, size);

	WATCHDOG_RESET();
	memory_post_addrline((ulong *)(start + size - 8), (ulong *)start, size);

	WATCHDOG_RESET();
	memory_post_test1(start, size, ZERO_pattern1);

	WATCHDOG_RESET();
	memory_post_test1(start, size, NEGATIVE_pattern1);

	WATCHDOG_RESET();
	memory_post_test1(start, size, CHECKBOARD_pattern11);

	WATCHDOG_RESET();
	memory_post_test1(start, size, CHECKBOARD_pattern12);

	WATCHDOG_RESET();
	memory_post_test2(start, size);

	WATCHDOG_RESET();
	memory_post_test3(start, size);

	WATCHDOG_RESET();
	memory_post_test4(start, size);

	WATCHDOG_RESET();
}


int 
memory_post_slow_ram_test_diag (int flags)
{
	int ret          = 0;
	mem_fail_flag    = 0;
	bd_t *bd = gd->bd;

	post_log("\n\n");
	post_log(" Testing memory from 0x100000 to 0x3fe00000 with "
              "different patterns\n");
	post_log(" The size of memory %lu\n\n",(ulong)bd->bi_memsize);

	memory_post_slow_ram_tests(0x100000, bd->bi_memsize);

	post_log("\n\n");
	if (mem_fail_flag) {
		printf(" ------------ MEMORY TEST: FAILED  ------------\n");
		ret = -1;
	} else {
		printf(" ------------ MEMORY TEST: PASSED   ------------\n");
	}

	return ret;
}

#endif /* CONFIG_POST & CFG_POST_MEMORY_SLOW_TEST_RAM */
#endif /* CONFIG_POST */
