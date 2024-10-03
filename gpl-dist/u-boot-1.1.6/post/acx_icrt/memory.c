/*
 * $Id$
 *
 * memory.c -- memory diagnostics for iCRT
 *
 * Copyright (c) 2011-2014, Juniper Networks, Inc.
 * All rights reserved.
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
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/gpl-2.0.html>.
 */

#include <common.h>
#include <asm/processor.h>
#include <asm/mmu.h>

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
 */

#ifdef CONFIG_POST

#include <post.h>
#include <acx_syspld.h>
#include "acx_icrt_post.h"

/*
 * Can't call via the hw_watchdog_reset because in case not relocated, the 
 * ticke override is still in flash and value is -1.
 */
#define WATCHDOG_RESET syspld_watchdog_tickle

#if CONFIG_POST & CFG_POST_MEMORY

int post_result_mem;
DECLARE_GLOBAL_DATA_PTR;

#define rom_flag (!(gd->flags & GD_FLG_RELOC))
#define SIZE_8KB  0x2000
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


#define DATALINE_WALKING_BIT      1
#define DATALINE_ADJ_COUPLING     2
#define ADDRLINE_INCR_UNIQUE_VAL  3
#define ADDRLINE_WALKING_BIT      4
#define MEMORY_CHECKERBOARD       5
#define MEMORY_MARCH_C            6
#define MEMORY_MARCH_C_MINUS      7
#define MEMORY_MARCH_B            8
#define MEMORY_MARCH_LR           9
#define ADDRESS_DECODER          10

static char *testnames[] = {
    "No Test", /* 0th location is intentionally left blank */
    "Dataline Walking Bit Test",
    "Dataline Adjacency Coupling Test",
    "Addressline Incremental Unique Values",
    "Addressline Walking Bit Test",
    "Memory Patterns / Checkerboard Test",
    "Memory March C Algorithm",
    "Memory March C Minus Algorithm",
    "Memory March B Algorithm",
    "Memory March LR Algorithm",
    "Memory Address Decoder Algorithm",
};

/*
 * The RAM area used by u-boot including stack, bdinfo, gd_t, malloc area etc
 * IN addition, u-boot also uses about 8K of memory @ 0x00000000 for vectors.
 */
#define UBOOT_MEMSTART 0x0FE00000
#define UBOOT_MEMSIZE  0x00200000

/*
 * Parses a value and prints the bit locations that are set (have "1")
 * The caller needs to ensure enough space in the buf (80 bytes are enough).
 */
int get_bits(char *buf, unsigned long long val)
{
    char *ptr = buf;
    unsigned int bits_set = 0;

    int i = 0;
    while (val) {
	/*
	 * Skip 0s
	 */
	while (!(val & 1)) {
	    val >>= 1;
	    i++;
	}
	/*
	 * Found a 1
	 */
	bits_set++;
	ptr += sprintf(ptr, " %d", i);
	val >>= 1;
	i++;
    }
    ptr += sprintf(ptr, " ");
    return bits_set;
}


/*
 * Test #1: Dataline Walking Bit Test (Walking 0s / Waling 1s)
 * This test ensures that each data pin can be set to 0 or 1 independent of the
 * values on the other data pins. This test can detect any stuck high or stuck
 * low pins, or any shorted pins, or even the floating lines.
 * Walking 1s: Walks a 1 from pin0 -> pin31 against a backdrop of 0s on all pins
 * Walking 0s: Walks a 0 from pin0 -> pin31 against a backdrop of 1s on all pins
 */
static int dataline_walking_bit(volatile unsigned long long * pmem, int flags)
{
    uint64_t temp64 = 0;
    uint64_t pattern = 0, antipattern = 0, mismatch = 0;
    char bits[80];
    unsigned int hi, lo, pathi, patlo, err = 0;

    POST_LOG(POST_LOG_INFO, "[Test #1] \"Dataline Walking Bit Test\" starting..."
	     "\n");
    POST_LOG(POST_LOG_DEBUG, "\tAddress Used: %08X\n", pmem);

    /* Walking 1(s) */
    POST_LOG(POST_LOG_INFO, "\t[Walking 1s Test] starting...\n");
    for (pattern = 1; pattern; pattern <<= 1) {
	antipattern = ~pattern;

	pathi = (pattern >> 32) & 0xffffffff;
	patlo = pattern & 0xffffffff;
	POST_LOG(POST_LOG_DEBUG, "\t\tTest pattern = %08X%08X\n",  pathi, patlo);

	*pmem = antipattern;
	*pmem = pattern;
	/*
	 * Put a different pattern on the data lines: otherwise they
	 * may float long enough to read back what we wrote.
	 */
	*(pmem + 1) = antipattern;
	temp64 = *pmem;

#ifdef INJECT_DATA_ERRORS
	temp64 ^= 0x00008000;
#endif
	mismatch = temp64 ^ pattern;
	if (mismatch) {

	    hi = (temp64 >> 32) & 0xffffffff;
	    lo = temp64 & 0xffffffff;

	    get_bits(bits, mismatch);
	    POST_LOG(POST_LOG_ERR, "\t\t[Dataline Walking Bit Test]\n");
	    POST_LOG(POST_LOG_ERR, "\t\t[Walking 1s] FAILURE! Anomaly at "
		     "Memory data PIN(s) %s\n", bits);
	    get_bits(bits, pattern);
	    POST_LOG(POST_LOG_ERR, "\t\t            (either stuck high, or "
		     "shorted to PIN %s)\n", bits); 
	    POST_LOG(POST_LOG_ERR, "\t\t            (I wrote 0x%08X%08X, read "
		     "back 0x%08X%08X)\n", pathi, patlo, hi, lo);
	    err |= 1;
	}
    }
    if (err & 1)
	POST_LOG(POST_LOG_ERR,  "\t[Walking 1s Test] ... FAILURE\n");
    else
	POST_LOG(POST_LOG_INFO, "\t[Walking 1s Test] ... SUCCESS\n");

    /* Walking 0(s) */
    POST_LOG(POST_LOG_INFO, "\t[Walking 0s Test] starting...\n");
    for (antipattern = 1; antipattern; antipattern <<= 1) {
	pattern = ~antipattern;

	pathi = (pattern >> 32) & 0xffffffff;
	patlo = pattern & 0xffffffff;
	POST_LOG(POST_LOG_DEBUG, "\t\tTest pattern = %08X%08X\n",  pathi, patlo);

	*pmem = antipattern;
	*pmem = pattern;
	/*
	 * Put a different pattern on the data lines: otherwise they
	 * may float long enough to read back what we wrote.
	 */
	*(pmem + 1) = antipattern;
	temp64 = *pmem;

#ifdef INJECT_DATA_ERRORS
	temp64 ^= 0x00006000;
#endif
	mismatch = temp64 ^ pattern;
	if (mismatch) {

	    hi = (temp64>>32) & 0xffffffff;
	    lo = temp64 & 0xffffffff;

	    get_bits(bits, mismatch);
	    POST_LOG(POST_LOG_ERR, "\t\t[Dataline Walking Bit Test]\n");
	    POST_LOG(POST_LOG_ERR, "\t\t[Walking 0s] FAILURE! Anomaly at "
		     "Memory data PIN(s) %s\n", bits);
	    get_bits(bits, antipattern);
	    POST_LOG(POST_LOG_ERR, "\t\t            (either stuck low, or "
		     "shorted to PIN %s)\n", bits); 
	    POST_LOG(POST_LOG_ERR, "\t\t            (I wrote 0x%08X%08X, read "
		     "back 0x%08X%08X)\n", pathi, patlo, hi, lo);
	    err |= 2;
	}
    }
    if (err & 2)
	POST_LOG(POST_LOG_ERR,  "\t[Walking 0s Test] ... FAILURE\n");
    else
	POST_LOG(POST_LOG_INFO, "\t[Walking 0s Test] ... SUCCESS\n");


    if (err) {
	POST_LOG(POST_LOG_ERR, "[Test #1] \"Dataline Walking Bit Test\" FAILURE"
		 "\n");
	return 1;
    } else {
	POST_LOG(POST_LOG_INFO, "[Test #1] \"Dataline Walking Bit Test\" SUCCESS"
		 "\n");
	return 0;
    }
}

/*
 * This is 64 bit wide test patterns.  Note that they reside in ROM
 * (which presumably works) and the tests write them to RAM which may
 * not work.
 *
 * The "antipattern" is written to drive the data bus to values other
 * than the test pattern.  This is for detecting floating bus lines.
 *
 */
const static unsigned long long patterns[] = {
    0x0000000000000000ULL,
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
    0x5555555555555555ULL,
    0xffffffffffffffffULL,
};
/*
 * Test #2: Dataline Adjacency Coupling Test
 * Use diferent patterns that group sets of 1s & 0s together in different 
 * denominations on the data line. Verify if a bunch of 1s cannot cause an
 * adjacent 0 bit line to become 1, and vice versa. This test can detect any
 * "coupling" effect where a bunch of lines can "induce" a value on nearby
 * pins.
 */
static int dataline_adj_coupling(volatile unsigned long long * pmem, int flags)
{
    unsigned long long temp64 = 0;
    unsigned long long antipattern = 0;
    unsigned long long mismatch = 0;
    int num_patterns          = sizeof(patterns)/ sizeof(patterns[0]);
    int i                     = 0;
    char bits[80];
    unsigned int hi, lo, pathi, patlo;
    int err = 0;

    POST_LOG(POST_LOG_INFO, "[Test #2] \"Dataline Adjacency Coupling Test\" "
	     "starting...\n");
    POST_LOG(POST_LOG_INFO, "\tThis test the data lines for shorts and opens "
	     "by forcing adjacent\n"  "\tto opposite states. The following "
	     "patterns are used\n");
    POST_LOG(POST_LOG_INFO, "\t0xaaaaaaaaaaaaaaaaULL, 0xccccccccccccccccULL, "
	     "0xf0f0f0f0f0f0f0f0ULL\n");
    POST_LOG(POST_LOG_INFO, "\t0xff00ff00ff00ff00ULL, 0xffff0000ffff0000ULL, "
	     "0xffffffff00000000ULL\n");
    POST_LOG(POST_LOG_INFO, "\t0x00000000ffffffffULL, 0x0000ffff0000ffffULL, "
	     "0x00ff00ff00ff00ffULL\n");
    POST_LOG(POST_LOG_INFO, "\t0x0f0f0f0f0f0f0f0fULL, 0x3333333333333333ULL, "
	     "0x5555555555555555ULL\n");
    POST_LOG(POST_LOG_DEBUG, "\tAddress Used: %08X\n", pmem);

    for (i = 0; i < num_patterns; i++) {

	pathi = (patterns[i] >> 32) & 0xffffffff;
	patlo = patterns[i] & 0xffffffff;
	POST_LOG(POST_LOG_DEBUG, "\tTest pattern = %08X%08X\n",  pathi, patlo);

	*pmem = patterns[i];
	/*
	 * Put a different pattern on the data lines: otherwise they
	 * may float long enough to read back what we wrote.
	 */
	antipattern = ~patterns[i];
	*(pmem + 1) = antipattern;
	temp64 = *pmem;

#ifdef INJECT_DATA_ERRORS
	temp64 ^= 0x00008000;
#endif
	mismatch = temp64 ^ patterns[i];
	if (mismatch) {

	    hi = (temp64 >> 32) & 0xffffffff;
	    lo = temp64 & 0xffffffff;

	    get_bits(bits, mismatch);
	    POST_LOG(POST_LOG_ERR, "\t\t[Dataline Adjacency Coupling Test]\n");
	    POST_LOG(POST_LOG_ERR, "\tFAILURE! Anomaly at Memory data PIN(s) "
		     "%s\n", bits);
	    POST_LOG(POST_LOG_ERR, "\t(I wrote 0x%08X%08X, read back 0x%08X%08X)"
		     "\n", pathi, patlo, hi, lo);
	    err = 1;
	}
    }
    if (err)
	POST_LOG(POST_LOG_ERR, "[Test #2] \"Dataline Adjacency Coupling Test\" "
		 "FAILURE!\n");
    else
	POST_LOG(POST_LOG_INFO, "[Test #2] \"Dataline Adjacency Coupling Test\" "
		 "SUCCESS!\n");
    return err;
}

/*
 * Test #3: Addressline Incremental Unique Values Test
 * The Test: Store a unique value at all location in a loop, and then verify
 * that each location still has the SAME unique value it was supposed to have.
 * This ensures that there are no overlapping regions (writing to a location
 * does not cause ANOTHER location to be written as a side effect). This test
 * verifies that the address lines are not shoted, or stuck high or stuck low.
 * Being the paranoid we are, run this twice - once going up & then coming down
 * Note: Both mem & size are assumed to be SIZE_8KB aligned.
 */
static int addrline_incr_unique_val(unsigned long mem, unsigned long size,
				    int flags)
{
    volatile ulong *ptr;
    ulong readval;
    int i, err = 0;
    char bits[80];

    /* 
     * In P2020, there are only 16 address lines from processor to DRAM. The 
     * row / col addrs are time multiplexed over these lines. The ACX has a
     * memory organization of (14x10x3) or (15x10x3). Since the row & col
     * addresses are multiplexed over the same addr lines, it suffices to check
     * all the row addresses (If we've covered all combination of row addresses,
     * all addr lines have been tested). There are 10 col bits & 3 unimportant
     * bits while calculating addr pin values from phys addr, Thus we increment
     * by SIZE_8KB (13 bits) every time.
     */
    POST_LOG(POST_LOG_INFO, "[Test #3] \"Addressline Incremental Unique Values "
	     "Test\" starting...\n");
    POST_LOG(POST_LOG_INFO, "\tThis test stores a unique value at all location "
	     "in a loop, and then verifies\n""\tthat each location still has "
	     "the SAME unique value it was supposed to have.\n");
    POST_LOG(POST_LOG_INFO, "\tThis ensures that there are no overlapping "
	     "regions (writing to a location\n");
    POST_LOG(POST_LOG_INFO, "\tdoes not cause ANOTHER location to be written as "
	     "a side effect). This test\n");
    POST_LOG(POST_LOG_INFO, "\tverifies that the address lines are not shoted, "
	     "or stuck high or stuck low.\n");
    POST_LOG(POST_LOG_INFO, "\tBeing the paranoid we are, run this twice - once "
	     "going up & then coming down\n");
    POST_LOG(POST_LOG_INFO, "\tPhase1 (Incremental Loop) starting...\n");

    for (i = 0; i < size; i += SIZE_8KB) {
	ptr = (ulong *)(mem + i);
	POST_LOG(POST_LOG_DEBUG, "\t\tWriting @addr [%08X]...\n", ptr);
	*ptr = (ulong)ptr + 1;
    }
    for (i = 0; i < size; i += SIZE_8KB) {
	ptr = (ulong *)(mem + i);
	readval = *ptr;

#ifdef INJECT_ADDRESS_ERRORS
if (i==16*SIZE_8KB)
    readval ^= 0x8000;
#endif
	if (readval != ((ulong)ptr + 1)) {
	    ulong mismatch = (ulong)ptr ^ (readval - 1);
	    err |= 1;
	    POST_LOG(POST_LOG_ERR, "\t\t[Addressline Incremental Unique Values "
		     "Test]\n");
	    POST_LOG(POST_LOG_ERR, "\t\tPhase1 FAILURE! Mismatch at address "
		     "%08X\n", ptr);
	    POST_LOG(POST_LOG_ERR, "\t\t[%08X] was written with %08X, but it "
		     "automatically became %08X\n", 
		     ptr, (ulong)ptr + 1, readval);
	    POST_LOG(POST_LOG_ERR, "\t\tA write to [%08X] seems to be affecting "
		     "contents of [%08X]\n", readval - 1, ptr);
	    POST_LOG(POST_LOG_ERR, "\t\t[Heuristic Analysis: (analysis may not "
		     "be correct - but error is certain)]\n");
	    get_bits(bits, mismatch);
	    POST_LOG(POST_LOG_ERR, "\t\tAddress Bits %s seem to be affected\n", 
		     bits);
	    POST_LOG(POST_LOG_ERR, "\t\tThe above bits may be stuck high / low "
		     "or may be shorted to one of the other bits\n");
	    POST_LOG(POST_LOG_ERR, "\t\tThe above holds true for physical "
		     "address bits and will need to be mapped to A0 - A15 pins\n");
	    POST_LOG(POST_LOG_ERR, "\t\ton the baord (depends upon mem "
		     "controller initialization, RAM organization in rows / cols)\n");
	}
    }
    if (err & 1)
	POST_LOG(POST_LOG_ERR, "\tPhase1 (Incremental Loop) ...FAILURE\n");
    else
	POST_LOG(POST_LOG_INFO, "\tPhase1 (Incremental Loop) ...SUCCESS\n");
 
    POST_LOG(POST_LOG_INFO, "\tPhase2 (Decremental Loop) starting...\n");
    for (i = size-SIZE_8KB; i >= 0; i -= SIZE_8KB) {
	ptr = (ulong *)(mem + i);
	POST_LOG(POST_LOG_DEBUG, "\t\tWriting @addr [%08X]...\n", ptr);
	*ptr = (ulong)ptr + 1;
    }
    for (i = size-SIZE_8KB; i >= 0; i -= SIZE_8KB) {
	ptr = (ulong *)(mem + i);
	readval = *ptr;
#ifdef INJECT_ADDRESS_ERRORS
if (i==16*SIZE_8KB)
    readval ^= 0x80000;
#endif
	if (readval != ((ulong)ptr + 1)) {
	    ulong mismatch = (ulong)ptr ^ (readval - 1);
	    err |= 2;
	    POST_LOG(POST_LOG_ERR, "\t\t[Addressline Incremental Unique Values "
		     "Test]\n");
	    POST_LOG(POST_LOG_ERR, "\t\tPhase1 FAILURE! Mismatch at address "
		     "%08X\n", ptr);
	    POST_LOG(POST_LOG_ERR, "\t\t[%08X] was written with %08X, but it "
		     "automatically became %08X\n", ptr, (ulong)ptr + 1, readval);
	    POST_LOG(POST_LOG_ERR, "\t\tA write to [%08X] seems to be affecting "
		     "contents of [%08X]\n", readval - 1, ptr);
	    POST_LOG(POST_LOG_ERR, "\t\t[Heuristic Analysis: (analysis may not "
		     "be correct - but error is certain)]\n");
	    get_bits(bits, mismatch);
	    POST_LOG(POST_LOG_ERR, "\t\tAddress Bits %s seem to be affected\n",
		     bits);
	    POST_LOG(POST_LOG_ERR, "\t\tThe above bits may be stuck high / low "
		     "or may be shorted to one of the other bits\n");
	    POST_LOG(POST_LOG_ERR, "\t\tThe above holds true for physical "
		     "address bits and will need to be mapped to A0 - A15 pins\n");
	    POST_LOG(POST_LOG_ERR, "\t\ton the baord (depends upon mem "
		     "controller initialization, RAM organization in rows / cols)\n");
	}
    }
    if (err & 2)
	POST_LOG(POST_LOG_ERR, "\tPhase2 (Decremental Loop) ...FAILURE\n");
    else
	POST_LOG(POST_LOG_INFO, "\tPhase2 (Decremental Loop) ...SUCCESS\n");

    if (err)
	POST_LOG(POST_LOG_ERR, "[Test #3] \"Addressline Incremental Unique "
		 "Values Test\" FAILURE!\n");
    else
	POST_LOG(POST_LOG_INFO, "[Test #3] \"Addressline Incremental Unique "
		 "Values Test\" SUCCESS!\n");
    return err;
}

/*
 * Test #4: Addressline Walking Bit Test
 * This Test assumes memory starts from zero to a given memory size, thus can
 * be executed at pre relocation time only. The test can detect if an address
 * line is stuck high / low / open or is shorted to one of its adjacent lines.
 * It can detect upto 3 adjacent shorted lines. The test involves driving a 1
 * bit on each of the address bits With other addressbits being 0. It then 
 * checks if any adjacent bits (left or right or both) have become one. This is
 * then repeated by walking a 0 bit on a backdrop of 1s on all other address bits.
 * memsize is assumed to be a power of 2.
 */
static int addrline_walking_bit(unsigned long memsize, int flags)
{
    unsigned char readval;
    int i, j, err = 0;
    char bits[80];
    unsigned long testaddr, verifyaddr[4], mismatch;
    int num_verifyaddr;
    int maxbit = __ilog2(memsize - 1);

#define PATTERN 0x55
#define ANTIPATTERN 0xAA

    POST_LOG(POST_LOG_INFO, "[Test #4] \"Addressline Walking Bit Test\" "
	     "starting...\n");
    POST_LOG(POST_LOG_INFO, "\tThe test can detect if an address\n");
    POST_LOG(POST_LOG_INFO, "\tline is stuck high / low / open or is shorted to "
	     "one of its adjacent lines.\n");
    POST_LOG(POST_LOG_INFO, "\tIt can detect upto 3 adjacent shorted lines. The "
	     "test involves driving a 1\n");
    POST_LOG(POST_LOG_INFO, "\tbit on each of the address bits With other "
	     "addressbits being 0. It then\n");
    POST_LOG(POST_LOG_INFO, "\tchecks if any adjacent bits (left or right or "
	     "both) have become one. This is\n");
    POST_LOG(POST_LOG_INFO, "\tthen repeated by walking a 0 bit on a backdrop "
	     "of 1s on all other address bits.\n");
    POST_LOG(POST_LOG_INFO, "\tPhysical address bits to be tested = 0 - %d\n",
	     maxbit);
    POST_LOG(POST_LOG_INFO, "\tPhase1 (Walking 1s) starting...\n");
    verifyaddr[0] = 0;
    for (i = 0; i <= maxbit; i++) {
	testaddr = 1 << i;
	verifyaddr[1] = (testaddr << 1) | testaddr;
	verifyaddr[2] = (testaddr >> 1) | testaddr;
	verifyaddr[3] = (testaddr << 1) | testaddr | (testaddr >> 1);

	if (i == 0) {
	    num_verifyaddr = 2;
	} else if (i == maxbit) {
	    num_verifyaddr = 2;
	    verifyaddr[1] = verifyaddr[2];
	} else {
	    num_verifyaddr = 4;
	}

	POST_LOG(POST_LOG_DEBUG, "\t\ttestaddr   = [%08X]...\n\t\tverifyaddr =",
		 testaddr);
	for (j = 0; j < num_verifyaddr; j++) {
	    POST_LOG(POST_LOG_DEBUG, " [%08X] ", verifyaddr[j]);
	}
	POST_LOG(POST_LOG_DEBUG, "\n");

	*((volatile unsigned char *) testaddr) = ANTIPATTERN;
	for (j = 0; j < num_verifyaddr; j++) {
	    *((volatile unsigned char *) verifyaddr[j]) = ANTIPATTERN;
	}

	*((volatile unsigned char *) testaddr) = PATTERN;
	for (j = 0; j < num_verifyaddr; j++) {
	    readval = *((volatile unsigned char *) verifyaddr[j]);

#ifdef INJECT_ADDRESS_ERRORS
readval ^= 0x8;
#endif

	    if (readval != ANTIPATTERN) {
		err |= 1;
		POST_LOG(POST_LOG_ERR, "\t\t[Addressline Walking Bit Test]\n");
		POST_LOG(POST_LOG_ERR, "\t\tPhase1 FAILURE![%08X] was written "
			 "with %02X, but it automatically became %02X\n",
			 verifyaddr[j], ANTIPATTERN, readval);
		POST_LOG(POST_LOG_ERR, "\t\tA write to [%08X] seems to be "
			 "affecting contents of [%08X]\n", 
			 testaddr, verifyaddr[j]);
		if (verifyaddr[j] == 0) {
		    POST_LOG(POST_LOG_ERR, "\t\tAddress Bit %d seem to be "
			     "either stuck high / low or is floating\n", i);
		} else {
		    mismatch = testaddr ^ verifyaddr[j];
		    get_bits(bits, mismatch);
		    POST_LOG(POST_LOG_ERR, "\t\tA 1 driven on Address Bit %d "
			     "seems to be causing a 1 to appear on bits %s\n", 
			     i, bits);
		}
	    }
	}
	readval = *((volatile unsigned char *) testaddr);
	if (readval != PATTERN) {
	    err |= 1;
	    POST_LOG(POST_LOG_ERR, "\t\t[Addressline Walking Bit Test]\n");
	    POST_LOG(POST_LOG_ERR, "\t\tPhase1 FAILURE![%08X] was written with "
		     "%02X, but it automatically became %02X\n",
		     testaddr, PATTERN, readval);
	    POST_LOG(POST_LOG_ERR, "\t\tAddress Bit %d seems to be problematic "
		     "(either stuck high / low or is floating / shorted)\n", i);
	}
    }

    if (err & 1) {
	POST_LOG(POST_LOG_ERR, "\t\tThe above holds true for physical address "
		 "bits and will need to be mapped to A0 - A15 pins\n");
	POST_LOG(POST_LOG_ERR, "\t\ton the baord (depends upon mem controller "
		 "initialization, RAM organization in rows / cols)\n");
	POST_LOG(POST_LOG_ERR, "\tPhase1 (Walking 1s) ...FAILURE\n");
    } else {
	POST_LOG(POST_LOG_INFO, "\tPhase1 (Walking 1s) ...SUCCESS\n");
    }
 
    POST_LOG(POST_LOG_INFO, "\tPhase2 (Walking 0s) starting...\n");
    verifyaddr[0] = memsize - 1;
    for (i = 0; i <= maxbit; i++) {
	testaddr = 1 << i;
	verifyaddr[1] = ~((testaddr << 1) | testaddr) & (memsize - 1);
	verifyaddr[2] = ~((testaddr >> 1) | testaddr) & (memsize - 1);
	verifyaddr[3] = ~((testaddr << 1) | testaddr | (testaddr >> 1)) 
			& (memsize - 1);
	testaddr = ~testaddr & (memsize - 1);

	if (i == 0) {
	    num_verifyaddr = 2;
	} else if (i == maxbit) {
	    num_verifyaddr = 2;
	    verifyaddr[1] = verifyaddr[2];
	} else {
	    num_verifyaddr = 4;
	}

	POST_LOG(POST_LOG_DEBUG, "\t\ttestaddr   = [%08X]...\n\t\tverifyaddr =",
		 testaddr);
	for (j = 0; j < num_verifyaddr; j++) {
	    POST_LOG(POST_LOG_DEBUG, " [%08X] ", verifyaddr[j]);
	}
	POST_LOG(POST_LOG_DEBUG, "\n");

	*((volatile unsigned char *) testaddr) = ANTIPATTERN;
	for (j = 0; j < num_verifyaddr; j++) {
	    *((volatile unsigned char *) verifyaddr[j]) = ANTIPATTERN;
	}

	*((volatile unsigned char *) testaddr) = PATTERN;
	for (j = 0; j < num_verifyaddr; j++) {
	    readval = *((volatile unsigned char *) verifyaddr[j]);
#ifdef INJECT_ADDRESS_ERRORS
readval ^= 0x8;
#endif
	    if (readval != ANTIPATTERN) {
		err |= 2;    
		POST_LOG(POST_LOG_ERR, "\t\t[Addressline Walking Bit Test]\n");
		POST_LOG(POST_LOG_ERR, "\t\tPhase2 FAILURE![%08X] was written "
			 "with %02X, but it automatically became %02X\n", 
			 verifyaddr[j], ANTIPATTERN, readval);
		POST_LOG(POST_LOG_ERR, "\t\tA write to [%08X] seems to be "
			 "affecting contents of [%08X]\n",
			 testaddr, verifyaddr[j]);
		if (verifyaddr[j] == 0) {
		    POST_LOG(POST_LOG_ERR, "\t\tAddress Bit %d seems to be "
			     "either stuck high / low or is floating\n", i);
		} else {
		    mismatch = testaddr ^ verifyaddr[j];
		    get_bits(bits, mismatch);
		    POST_LOG(POST_LOG_ERR, "\t\tA 0 driven on Address Bit %d "
			     "seems to be causing a 0 to appear on bits %s\n",
			     i, bits);
		}
	    }
	}
	readval = *((volatile unsigned char *) testaddr);
	if (readval != PATTERN) {
	    err |= 2;
	    POST_LOG(POST_LOG_ERR, "\t\t[Addressline Walking Bit Test]\n");
	    POST_LOG(POST_LOG_ERR, "\t\tPhase2 FAILURE![%08X] was written with "
		     "%02X, but it automatically became %02X\n", testaddr,
		     PATTERN, readval);
	    POST_LOG(POST_LOG_ERR, "\t\tAddress Bit %d seems to be problematic "
		     "(either stuck high / low or is floating / shorted)\n", i);
	}
    }

    if (err & 2) {
	POST_LOG(POST_LOG_ERR, "\t\tThe above holds true for physical address "
		 "bits and will need to be mapped to A0 - A15 pins\n");
	POST_LOG(POST_LOG_ERR, "\t\ton the baord (depends upon mem controller "
		 "initialization, RAM organization in rows / cols)\n");
	POST_LOG(POST_LOG_ERR, "\tPhase2 (Walking 0s) ...FAILURE\n");
    } else {
	POST_LOG(POST_LOG_INFO, "\tPhase2 (Walking 0s) ...SUCCESS\n");
    }

    if (err)
	POST_LOG(POST_LOG_ERR, "[Test #4] \"Addressline Walking Bit Test\" "
		 "FAILURE!\n");
    else
	POST_LOG(POST_LOG_INFO, "[Test #4] \"Addressline Walking Bit Test\" "
		 "SUCCESS!\n");

    return err;
}

static int memory_pattern_test(unsigned long start, unsigned long size,
			       uint64_t val, int flags)
{
    unsigned long i = 0;
    volatile uint64_t *mem      = (void*) start;
    uint64_t readback  = 0;
    int ret         = 0;
    ulong pathi, patlo, readhi, readlo;

    pathi = val >> 32;
    patlo = val & 0xFFFFFFFF;

    POST_LOG(POST_LOG_INFO, "\t\"Memory Pattern Test\" [%08X%08X] starting...\n",
	     pathi, patlo);
    POST_LOG(POST_LOG_INFO, "\tTesting %08X - %08X memory\n", start, 
	     start + size - 1);

    POST_LOG(POST_LOG_INFO, "\tWriting...\n");
    for (i = 0; i < size / sizeof(uint64_t); i++) {
	mem[i] = val;
	if (i % 1024 == 0) {
	    WATCHDOG_RESET();
	}
	if (i % (1024*1024) == 0)
	    POST_LOG(POST_LOG_INFO, ".");
    }

    POST_LOG(POST_LOG_INFO, "\n\tVerifying...\n");
    for (i = 0; i < size / sizeof(uint64_t) && ret == 0; i++) {
	readback = mem[i];
	readhi = readback >> 32;
	readlo = readback & 0xFFFFFFFF;
	if (readback != val) {
	    POST_LOG(POST_LOG_ERR, "\t\t[Memory Pattern Test]\n");
	    POST_LOG(POST_LOG_ERR, "\t\tMemory error at [%08x], wrote %08X%08X, "
		     "read %08X%08X!\n",
		mem + i, pathi, patlo, readhi, readlo);
	    ret = -1;
	    break;
	}
	if (i % 1024 == 0)
	    WATCHDOG_RESET();
	if (i % (1024*1024) == 0)
	    POST_LOG(POST_LOG_INFO, ".");
    }
    if (ret)
	POST_LOG(POST_LOG_ERR, "\n\t\"Memory Pattern Test\" [%08X%08X] "
		 "FAILURE!\n", pathi, patlo);
    else
	POST_LOG(POST_LOG_INFO, "\n\t\"Memory Pattern Test\" [%08X%08X] "
		 "SUCCESS!\n", pathi, patlo);

    return ret;
}

static int memory_checkerboard_test(unsigned long start, unsigned long size, 
				    int flags) 
{
    int i, ret = 0;
    int num_patterns = sizeof(patterns)/ sizeof(patterns[0]);

    POST_LOG(POST_LOG_INFO, "[Test #5] \"Memory Checkerboard Test\" starting...\n");
    for (i = 0; i < num_patterns; i++) {

	WATCHDOG_RESET();
	if (ret == 0) {
	    ret = memory_pattern_test(start, size, patterns[i], flags);
	}
    }
    if (ret)
	POST_LOG(POST_LOG_ERR, "[Test #5] \"Memory Checkerboard Test\" FAILURE\n");
    else
	POST_LOG(POST_LOG_INFO, "[Test #5] \"Memory Checkerboard Test\" SUCCESS\n");

    return ret;
}

#define SETBIT(addr, bit)       \
do {                            \
    *(addr) |= (1 << (bit));    \
} while(0)

#define CLEARBIT(addr, bit)     \
do {                            \
    *(addr) &= ~(1 << (bit));   \
} while(0)

#define GETBIT(addr, bit) ((*(addr) >> (bit)) & 1)


/*
 * Test #6: Memory March C
 * Algorithm:
 *  1) (up)w0; 
 *  2) (up)(r0, w1); 
 *  3) (up)(r1, w0);
 *  4) (down)(r0, w1); 
 *  5) (down)(r1, w0); 
 *  6) (down)(r0);
 */
static int memory_march_c(unsigned long start, unsigned long size, int flags)
{
    int i, j, n;
    volatile ulong *mem      = (ulong *)start;
    
    n = size / sizeof(ulong);

    POST_LOG(POST_LOG_INFO, "[Test #6] \"Memory March C Test\" starting...\n");
    POST_LOG(POST_LOG_INFO, "\tAlgorithm:\n");
    POST_LOG(POST_LOG_INFO, "\t\t1) (up)w0;\n");
    POST_LOG(POST_LOG_INFO, "\t\t2) (up)(r0, w1);\n");
    POST_LOG(POST_LOG_INFO, "\t\t3) (up)(r1, w0);\n");
    POST_LOG(POST_LOG_INFO, "\t\t4) (down)(r0, w1);\n");
    POST_LOG(POST_LOG_INFO, "\t\t5) (down)(r1, w0);\n");
    POST_LOG(POST_LOG_INFO, "\t\t6) (down)(r0);\n");
    POST_LOG(POST_LOG_INFO, "\tTesting %08X - %08X memory\n", start,
	     start + size - 1);

    POST_LOG(POST_LOG_INFO, "\tStep1 [(up)w0]...\n");
    for (i = 0; i < n; i++) {
	for (j = 0; j < 32; j++) {
	    CLEARBIT(mem + i, j);
	}
	if (i % (1024*1024) == 0) {
	    WATCHDOG_RESET();
	    POST_LOG(POST_LOG_INFO, ".");
	}
    }

    POST_LOG(POST_LOG_INFO, "\n\tStep2 [(up)(r0, w1)]...\n");
    for (i = 0; i < n; i++) {
	for (j = 0; j < 32; j++) {
	    if (GETBIT(mem + i, j) == 0)
		SETBIT(mem + i, j);
	    else {
		POST_LOG(POST_LOG_ERR, "\n\t\t March C test failed at Step2. "
			 "The memory chip is faulty (possibly cannot save "
			 "1s & 0s on all bits)...");
		return -1;
	    }
	}
	if (i % (1024*1024) == 0) {
	    WATCHDOG_RESET();
	    POST_LOG(POST_LOG_INFO, ".");
	}
    }

    POST_LOG(POST_LOG_INFO, "\n\tStep3 [(up)(r1, w0)]...\n");
    for (i = 0; i < n; i++) {
	for (j = 0; j < 32; j++) {
	    if (GETBIT(mem + i, j) == 1)
		CLEARBIT(mem + i, j);
	    else {
		POST_LOG(POST_LOG_ERR, "\n\t\t March C test failed at Step3. The "
			 "memory chip is faulty (possibly cannot save 1s & 0s on "
			 "all bits)...");
		return -1;
	    }
	}
	if (i % (1024*1024) == 0) {
	    WATCHDOG_RESET();
	    POST_LOG(POST_LOG_INFO, ".");
	}
    }

    POST_LOG(POST_LOG_INFO, "\n\tStep4 [(down)(r0, w1)]...\n");
    for (i = n - 1; i >= 0; i--) {
	for (j = 31; j >= 0; j--) {
	    if (GETBIT(mem + i, j) == 0)
		SETBIT(mem + i, j);
	    else {
		POST_LOG(POST_LOG_ERR, "\n\t\t March C test failed at Step4. "
			 "The memory chip is faulty (possibly cannot save 1s "
			 "& 0s on all bits)...");
		return -1;
	    }
	}
	if (i % (1024*1024) == 0) {
	    WATCHDOG_RESET();
	    POST_LOG(POST_LOG_INFO, ".");
	}
    }

    POST_LOG(POST_LOG_INFO, "\n\tStep5 [(down)(r1, w0)]...\n");
    for (i = n - 1; i >= 0; i--) {
	for (j = 31; j >= 0; j--) {
	    if (GETBIT(mem + i, j) == 1)
		CLEARBIT(mem + i, j);
	    else {
		POST_LOG(POST_LOG_ERR, "\n\t\t March C test failed at Step5. "
			 "The memory chip is faulty (possibly cannot save 1s "
			 "& 0s on all bits)...");
		return -1;
	    }
	}
	if (i % (1024*1024) == 0) {
	    WATCHDOG_RESET();
	    POST_LOG(POST_LOG_INFO, ".");
	}
    }

    POST_LOG(POST_LOG_INFO, "\n\tStep6 [(down)(r0)]...\n");
    for (i = n - 1; i >= 0; i--) {
	for (j = 31; j >= 0; j--) {
	    if (GETBIT(mem + i, j) == 0)
		/* Do nothing */;
	    else {
		POST_LOG(POST_LOG_ERR, "\n\t\t March C test failed at Step6. "
			 "The memory chip is faulty (possibly cannot save 1s "
			 "& 0s on all bits)...");
		return -1;
	    }
	}
	if (i % (1024*1024) == 0) {
	    WATCHDOG_RESET();
	    POST_LOG(POST_LOG_INFO, ".");
	}
    }
    POST_LOG(POST_LOG_INFO, "\n[Test #6] \"Memory March C Test\" SUCCESSFULL\n");

    return 0;
}

/*
 * Test #7: Memory March C Minus
 * Algorithm:
 *  1) (up)wAAAA; 
 *  2) (up)(rAAAA, w5555); 
 *  3) (up)(r5555, wAAAA);
 *  4) (down)(rAAAA, w5555); 
 *  5) (down)(r5555, wAAAA); 
 *  6) (down)(rAAAA);
 */
static int memory_march_c_minus(unsigned long start, unsigned long size,
				int flags)
{
    int i, n;
    volatile ulong *mem      = (ulong *)start;
    int failed = 0;
    
    n = size / sizeof(ulong);

    POST_LOG(POST_LOG_INFO, "[Test #7] \"Memory March C Minus Test\" starting...\n");
    POST_LOG(POST_LOG_INFO, "\tAlgorithm:\n");
    POST_LOG(POST_LOG_INFO, "\t\t1) (up)wAAAA;\n");
    POST_LOG(POST_LOG_INFO, "\t\t2) (up)(rAAAA, w5555);\n");
    POST_LOG(POST_LOG_INFO, "\t\t3) (up)(r5555, wAAAA);\n");
    POST_LOG(POST_LOG_INFO, "\t\t4) (down)(rAAAA, w5555);\n");
    POST_LOG(POST_LOG_INFO, "\t\t5) (down)(r5555, wAAAA);\n");
    POST_LOG(POST_LOG_INFO, "\t\t6) (up)(rAAAA);\n");
    POST_LOG(POST_LOG_INFO, "\tTesting %08X - %08X memory\n", start,
	     start + size - 1);

    POST_LOG(POST_LOG_INFO, "\tStep1 [(up)wAAAA]...\n");
    for (i = 0; !failed && (i < n); i++) {
	mem[i] = 0xAAAAAAAA;
	if (i % (1024*1024) == 0) {
	    WATCHDOG_RESET();
	    POST_LOG(POST_LOG_INFO, ".");
	}
    }

    POST_LOG(POST_LOG_INFO, "\n\tStep2 [(up)(rAAAA, w5555)]...\n");
    for (i = 0; !failed && (i < n); i++) {
	if (mem[i] == 0xAAAAAAAA)
	    mem[i] = 0x55555555;
	else 
	    failed = 2;
	if (i % (1024*1024) == 0) {
	    WATCHDOG_RESET();
	    POST_LOG(POST_LOG_INFO, ".");
	}
    }

    POST_LOG(POST_LOG_INFO, "\n\tStep3 [(up)(r5555, wAAAA)]...\n");
    for (i = 0; !failed && (i < n); i++) {
	if (mem[i] == 0x55555555)
	    mem[i] = 0xAAAAAAAA;
	else
	    failed = 3;
	if (i % (1024*1024) == 0) {
	    WATCHDOG_RESET();
	    POST_LOG(POST_LOG_INFO, ".");
	}
    }

    POST_LOG(POST_LOG_INFO, "\n\tStep4 [(down)(rAAAA, w5555)]...\n");
    for (i = n - 1; !failed && (i >= 0); i--) {
	if (mem[i] == 0xAAAAAAAA)
	    mem[i] = 0x55555555;
	else
	    failed = 4;
	if (i % (1024*1024) == 0) {
	    WATCHDOG_RESET();
	    POST_LOG(POST_LOG_INFO, ".");
	}
    }

    POST_LOG(POST_LOG_INFO, "\n\tStep5 [(down)(r5555, wAAAA)]...\n");
    for (i = n - 1; !failed && (i >= 0); i--) {

#ifdef INJECT_DATA_ERRORS
	if (i == 24)
	    mem[i] = 0x34;
#endif

	if (mem[i] == 0x55555555)
	    mem[i] = 0xAAAAAAAA;
	else
	    failed = 5;
	if (i % (1024*1024) == 0) {
	    WATCHDOG_RESET();
	    POST_LOG(POST_LOG_INFO, ".");
	}
    }

    POST_LOG(POST_LOG_INFO, "\n\tStep6 [(down)(rAAAA)]...\n");
    for (i = n - 1; !failed && (i >= 0); i--) {
	if (mem[i] == 0xAAAAAAAA)
	    /* Do nothing */;
	else
	    failed = 6;
	if (i % (1024*1024) == 0) {
	    WATCHDOG_RESET();
	    POST_LOG(POST_LOG_INFO, ".");
	}
    }
    if (failed) {
	POST_LOG(POST_LOG_ERR, "\n\t\t March C Minus test failed at Step%d. The "
		 "memory chip is faulty (possibly cannot save 1s & 0s on all "
		 "bits)...", failed);
	return -1;
    } else {
	POST_LOG(POST_LOG_INFO, "\n[Test #7] \"Memory March C Minus Test\" "
		 "SUCCESSFULL\n");
	return 0;
    }
}

/*
 * Test #8: Memory March B
 * Algorithm:
 *  1) (up)w0; 
 *  2) (up)(r0, w1, r1, w0, r0, w1); 
 *  3) (up)(r1, w0, w1);
 *  4) (down)(r1, w0, w1, w0);
 *  5) (down)(r0, w1, w0);
 *  6) (down)(r0);
 */
static int memory_march_b(unsigned long start, unsigned long size, int flags)
{
    int i, j, n, failed = 0;
    volatile ulong *mem      = (ulong *)start;
    
    n = size / sizeof(ulong);

    POST_LOG(POST_LOG_INFO, "[Test #8] \"Memory March B Test\" starting...\n");
    POST_LOG(POST_LOG_INFO, "\tAlgorithm:\n");
    POST_LOG(POST_LOG_INFO, "\t\t1) (up)w0;\n");
    POST_LOG(POST_LOG_INFO, "\t\t2) (up)(r0, w1, r1, w0, r0, w1);\n");
    POST_LOG(POST_LOG_INFO, "\t\t3) (up)(r1, w0, w1);\n");
    POST_LOG(POST_LOG_INFO, "\t\t4) (down)(r1, w0, w1, w0);\n");
    POST_LOG(POST_LOG_INFO, "\t\t5) (down)(r0, w1, w0);\n");
    POST_LOG(POST_LOG_INFO, "\t\t6) (up)(r0);\n");
    POST_LOG(POST_LOG_INFO, "\tTesting %08X - %08X memory\n", start,
	     start + size - 1);

    POST_LOG(POST_LOG_INFO, "\tStep1 [(up)w0]...\n");
    for (i = 0; i < n; i++) {
	for (j = 0; j < 32; j++) {
	    CLEARBIT(mem + i, j);
	}
	if (i % (1024*1024) == 0) {
	    WATCHDOG_RESET();
	    POST_LOG(POST_LOG_INFO, ".");
	}
    }

    POST_LOG(POST_LOG_INFO, "\n\tStep2 [(up)(r0, w1, r1, w0, r0, w1)]...\n");
    for (i = 0; i < n; i++) {
	for (j = 0; j < 32; j++) {
	    if (GETBIT(mem + i, j) == 0)
		SETBIT(mem + i, j);
	    else {
		failed = 2;
		goto out;
	    }
	    if (GETBIT(mem + i, j) == 1)
		CLEARBIT(mem + i, j);
	    else {
		failed = 2;
		goto out;
	    }
	    if (GETBIT(mem + i, j) == 0)
		SETBIT(mem + i, j);
	    else {
		failed = 2;
		goto out;
	    }
	}
	if (i % (1024*1024) == 0) {
	    WATCHDOG_RESET();
	    POST_LOG(POST_LOG_INFO, ".");
	}
    }

    POST_LOG(POST_LOG_INFO, "\n\tStep3 [(up)(r1, w0, w1)]...\n");
    for (i = 0; i < n; i++) {
	for (j = 0; j < 32; j++) {
	    if (GETBIT(mem + i, j) == 1) {
		CLEARBIT(mem + i, j);
		SETBIT(mem + i, j);
	    } else {
		failed = 3;
		goto out;
	    }
	}
	if (i % (1024*1024) == 0) {
	    WATCHDOG_RESET();
	    POST_LOG(POST_LOG_INFO, ".");
	}
    }

    POST_LOG(POST_LOG_INFO, "\n\tStep4 [(down)(r1, w0, w1, w0)]...\n");
    for (i = n - 1; i >= 0; i--) {
	for (j = 31; j >= 0; j--) {
	    if (GETBIT(mem + i, j) == 1) {
		CLEARBIT(mem + i, j);
		SETBIT(mem + i, j);
		CLEARBIT(mem + i, j);
	    } else {
		failed = 4;
		goto out;
	    }
	}
	if (i % (1024*1024) == 0) {
	    WATCHDOG_RESET();
	    POST_LOG(POST_LOG_INFO, ".");
	}
    }

    POST_LOG(POST_LOG_INFO, "\n\tStep5 [(down)(r0, w1, w0)]...\n");
    for (i = n - 1; i >= 0; i--) {
	for (j = 31; j >= 0; j--) {
	    if (GETBIT(mem + i, j) == 0) {
		SETBIT(mem + i, j);
		CLEARBIT(mem + i, j);
	    } else {
		failed = 5;
		goto out;
	    }
	}
	if (i % (1024*1024) == 0) {
	    WATCHDOG_RESET();
	    POST_LOG(POST_LOG_INFO, ".");
	}
    }

    POST_LOG(POST_LOG_INFO, "\n\tStep6 [(down)(r0)]...\n");
    for (i = n - 1; i >= 0; i--) {
	for (j = 31; j >= 0; j--) {
	    if (GETBIT(mem + i, j) == 0)
		/*Do nothing */;
	    else {
		failed = 6;
		goto out;
	    }
	}
	if (i % (1024*1024) == 0) {
	    WATCHDOG_RESET();
	    POST_LOG(POST_LOG_INFO, ".");
	}
    }

out:
    if (failed) {
	POST_LOG(POST_LOG_INFO, "\n[Test #8] \"Memory March B Test\" FAILED at "
		 "step%d\n", failed);
	return -1;
    } else {
	POST_LOG(POST_LOG_INFO, "\n[Test #8] \"Memory March B Test\" "
		 "SUCCESSFULL\n");
	return 0;
    }

    return 0;
}

/*
 * Test #9: Memory March LR
 * Algorithm:
 *  1) (up)w0; 
 *  2) (down)(r0, w1)
 *  3) (up)(r1, w0, r0, r0, w1); 
 *  4) (up)(r1, w0);
 *  5) (up)(r0, w1, r1, r1, w0);
 *  6) (up)(r0);
 */
static int memory_march_lr(unsigned long start, unsigned long size, int flags)
{
    int i, j, n, failed = 0;
    volatile ulong *mem      = (ulong *)start;
    
    n = size / sizeof(ulong);

    POST_LOG(POST_LOG_INFO, "[Test #9] \"Memory March LR Test\" starting...\n");
    POST_LOG(POST_LOG_INFO, "\tAlgorithm:\n");
    POST_LOG(POST_LOG_INFO, "\t\t1) (up)w0;\n");
    POST_LOG(POST_LOG_INFO, "\t\t2) (down)(r0, w1);\n");
    POST_LOG(POST_LOG_INFO, "\t\t3) (up)(r1, w0, r0, r0, w1);\n");
    POST_LOG(POST_LOG_INFO, "\t\t4) (up)(r1, w0);\n");
    POST_LOG(POST_LOG_INFO, "\t\t5) (up)(r0, w1, r1, r1, w0));\n");
    POST_LOG(POST_LOG_INFO, "\t\t6) (up)(r0);\n");
    POST_LOG(POST_LOG_INFO, "\tTesting %08X - %08X memory\n", start,
	     start + size - 1);

    POST_LOG(POST_LOG_INFO, "\tStep1 [(up)w0]...\n");
    for (i = 0; i < n; i++) {
	for (j = 0; j < 32; j++) {
	    CLEARBIT(mem + i, j);
	}
	if (i % (1024*1024) == 0) {
	    WATCHDOG_RESET();
	    POST_LOG(POST_LOG_INFO, ".");
	}
    }

    POST_LOG(POST_LOG_INFO, "\n\tStep2 [(down)(r0, w1)]...\n");
    for (i = n - 1; i >= 0; i--) {
	for (j = 31; j >= 0; j--) {
	    if (GETBIT(mem + i, j) == 0)
		SETBIT(mem + i, j);
	    else {
		failed = 2;
		goto out;
	    }
	}
	if (i % (1024*1024) == 0) {
	    WATCHDOG_RESET();
	    POST_LOG(POST_LOG_INFO, ".");
	}
    }

    POST_LOG(POST_LOG_INFO, "\n\tStep3 [(up)(r1, w0, r0, r0, w1)]...\n");
    for (i = 0; i < n; i++) {
	for (j = 0; j < 32; j++) {
	    if (GETBIT(mem + i, j) == 1) {
		CLEARBIT(mem + i, j);
	    } else {
		failed = 3;
		goto out;
	    }
	    if (GETBIT(mem + i, j) == 0) {
		/* Do Nothing */;
	    } else {
		failed = 3;
		goto out;
	    }
	    if (GETBIT(mem + i, j) == 0) {
		SETBIT(mem + i, j);
	    } else {
		failed = 3;
		goto out;
	    }
	}
	if (i % (1024*1024) == 0) {
	    WATCHDOG_RESET();
	    POST_LOG(POST_LOG_INFO, ".");
	}
    }

    POST_LOG(POST_LOG_INFO, "\n\tStep4 [(up)(r1, w0)]...\n");
    for (i = 0; i < n; i++) {
	for (j = 0; j < 32; j++) {
	    if (GETBIT(mem + i, j) == 1) {
		CLEARBIT(mem + i, j);
	    } else {
		failed = 4;
		goto out;
	    }
	}
	if (i % (1024*1024) == 0) {
	    WATCHDOG_RESET();
	    POST_LOG(POST_LOG_INFO, ".");
	}
    }

    POST_LOG(POST_LOG_INFO, "\n\tStep5 [(up)(r0, w1, r1, r1, w0)]...\n");
    for (i = 0; i < n; i++) {
	for (j = 0; j < 32; j++) {
	    if (GETBIT(mem + i, j) == 0) {
		SETBIT(mem + i, j);
	    } else {
		failed = 5;
		goto out;
	    }
	    if (GETBIT(mem + i, j) == 1) {
		/* Do nothing */;
	    } else {
		failed = 5;
		goto out;
	    }
	    if (GETBIT(mem + i, j) == 1) {
		CLEARBIT(mem + i, j);
	    } else {
		failed = 5;
		goto out;
	    }
	}
	if (i % (1024*1024) == 0) {
	    WATCHDOG_RESET();
	    POST_LOG(POST_LOG_INFO, ".");
	}
    }

    POST_LOG(POST_LOG_INFO, "\n\tStep6 [(up)(r0)]...\n");
    for (i = 0; i < n; i++) {
	for (j = 0; j < 32; j++) {
	    if (GETBIT(mem + i, j) == 0)
		/*Do nothing */;
	    else {
		failed = 6;
		goto out;
	    }
	}
	if (i % (1024*1024) == 0) {
	    WATCHDOG_RESET();
	    POST_LOG(POST_LOG_INFO, ".");
	}
    }

out:
    if (failed) {
	POST_LOG(POST_LOG_ERR, "\n[Test #9] \"Memory March LR Test\" FAILED at "
		 "step%d\n", failed);
	return -1;
    } else {
	POST_LOG(POST_LOG_INFO, "\n[Test #9] \"Memory March LR Test\" "
		 "SUCCESSFULL\n");
	return 0;
    }
}

/* 
 * Memory Address Decoder Test: tests whether the address decoder is fast enough
 * to handle back to back flipping of ALL bits on the address line. To do this,
 * it does back-to-back write and read to addresses that are 1s complement of 
 * each other, thus causing all the bits to flip on every transaction.
 *
 * IMP NOTE: This test can destroy contents unintended because it writes to
 * complement of given memory location. Thus it should be run from ROM only.
 * Also, address should be passed such that its complement lies within the range
 * that needs to be tested.
 */

static int memory_address_decoder_test(unsigned long addr1, int num_sig_bits,
				       int flags)
{
    char readval1, readval2;
    int ret         = 0;
    unsigned long mask = (1 << num_sig_bits) - 1;
    unsigned long addr2 = addr1 ^ mask;

    POST_LOG(POST_LOG_INFO, "[Test #10] \"Memory Address Decoder Test\" starting...\n");
    POST_LOG(POST_LOG_INFO, "\tTesting using addresses: [%08X] [%08X]\n",
	     addr1, addr2);

    *((volatile char *) addr1) = 0;
    *((volatile char *) addr2) = 0;
    readval1 = *((volatile char *) addr1);
    readval2 = *((volatile char *) addr2);

    if (readval1 != 0) {
	POST_LOG(POST_LOG_ERR, "\tMemory Address Decoder Failure!\n");
	POST_LOG(POST_LOG_ERR, "\t[%08X] was written with %02X but readback "
		 "%02X\n", addr1, 0, readval1);
	POST_LOG(POST_LOG_ERR, "\t[%08X] and [%08X] were written / read back "
		 "to back, and seemingly address "
		 "decoder cannot decode so fast (all bits flipped)\n", 
		 addr1, addr2);
	ret = -1;
    }
    if (readval2 != 0) {
	POST_LOG(POST_LOG_ERR, "\tMemory Address Decoder Failure!\n");
	POST_LOG(POST_LOG_ERR, "\t[%08X] was written with %02X but readback "
		 "%02X\n", addr2, 0, readval2);
	POST_LOG(POST_LOG_ERR, "\t[%08X] and [%08X] were written / read back "
		 "to back, and seemingly address decoder cannot decode so fast "
		 "(all bits flipped)\n", addr1, addr2);
	ret = -1;
    }
    *((volatile char *) addr1) = 0xFF;
    *((volatile char *) addr2) = 0xFF;
    readval1 = *((volatile char *) addr1);
    readval2 = *((volatile char *) addr2);

    if (readval1 != 0xFF) {
	POST_LOG(POST_LOG_ERR, "\tMemory Address Decoder Failure!\n");
	POST_LOG(POST_LOG_ERR, "\t[%08X] was written with %02X but readback "
		 "%02X\n", addr1, 0xFF, readval1);
	POST_LOG(POST_LOG_ERR, "\t[%08X] and [%08X] were written / read back "
		 "to back, and seemingly address decoder cannot decode so fast ("
		 "all bits flipped)\n", addr1, addr2);
	ret = -1;
    }
    if (readval2 != 0xFF) {
	POST_LOG(POST_LOG_ERR, "\tMemory Address Decoder Failure!\n");
	POST_LOG(POST_LOG_ERR, "\t[%08X] was written with %02X but readback "
		 "%02X\n", addr2, 0xFF, readval2);
	POST_LOG(POST_LOG_ERR, "\t[%08X] and [%08X] were written / read back "
		 "to back, and seemingly address decoder cannot decode so fast "
		 "(all bits flipped)\n", addr1, addr2);
	ret = -1;
    }

    if (ret)
	POST_LOG(POST_LOG_ERR, "[Test #10] \"Memory Address Decoder Test\" "
		 "FAILED.\n");
    else
	POST_LOG(POST_LOG_INFO, "[Test #10] \"Memory Address Decoder Test\" "
		 "SUCCESSFUL.\n");
    return ret;
}

void memory_post_tests(unsigned long start, unsigned long size, int flags,
		       unsigned long *run, unsigned long *failed)
{
    int ret = 0;

    POST_LOG(POST_LOG_IMP,  "Testing %d MB of memory at %08X\n",
	     size >> 20, start);

    WATCHDOG_RESET();
    *run |= 1 << DATALINE_WALKING_BIT;
    ret = dataline_walking_bit((unsigned long long *)start, flags);
    if (ret) {
	*failed |= 1 << DATALINE_WALKING_BIT;
	POST_LOG(POST_LOG_ERR, "IMPORTANT WARNING: \"Dataline Walking Bit "
		 "Test\" Failed. The result of subsequent tests may not be "
		 "correct!\n");
    }

    WATCHDOG_RESET();
    *run |= 1 << DATALINE_ADJ_COUPLING;
    ret = dataline_adj_coupling((unsigned long long *)start, flags);
    if (ret) {
	*failed |= 1 << DATALINE_ADJ_COUPLING;
	POST_LOG(POST_LOG_ERR, "IMPORTANT WARNING: \"Dataline Adjacency "
		 "Coupling Test\" Failed. The result of subsequent tests may "
		 "not be correct!\n");
    }

    WATCHDOG_RESET();
    *run |= 1 << ADDRLINE_INCR_UNIQUE_VAL;
    ret = addrline_incr_unique_val(start, size, flags);
    if (ret) {
	*failed |= 1 << ADDRLINE_INCR_UNIQUE_VAL;
	POST_LOG(POST_LOG_ERR, "IMPORTANT WARNING: \"Addressline Incremental "
		 "Unique Values Test\" Failed. The result of subsequent tests "
		 "may not be correct!\n");
    }

    WATCHDOG_RESET();
    if (rom_flag) {
	/* This test can affect memory OUTSIDE of specified range and hence "
	 * run only from ROM */
	*run |= 1 << ADDRLINE_WALKING_BIT;
	ret = addrline_walking_bit(size, flags);
	if (ret) {
	    *failed |= 1 << ADDRLINE_WALKING_BIT;
	    POST_LOG(POST_LOG_ERR, "IMPORTANT WARNING: \"Addressline Walking "
		     "Bit Test\" Failed. The result of subsequent tests may "
		     "not be correct!\n");
	}
    }

    if (flags & (POST_MANUAL | POST_SLOWTEST)) {
	/*
	 * Run All tests on Entire memory range
	 */
	WATCHDOG_RESET();
	*run |= 1 << MEMORY_CHECKERBOARD;
	ret = memory_checkerboard_test(start, size, flags);
	if (ret)
	    *failed |= 1 << MEMORY_CHECKERBOARD;

	WATCHDOG_RESET();
	*run |= 1 << MEMORY_MARCH_C;
	ret = memory_march_c(start, size, flags);
	if (ret)
	    *failed |= 1 << MEMORY_MARCH_C;

	WATCHDOG_RESET();
	*run |= 1 << MEMORY_MARCH_C_MINUS;
	ret = memory_march_c_minus(start, size, flags);
	if (ret)
	    *failed |= 1 << MEMORY_MARCH_C_MINUS;

	WATCHDOG_RESET();
	*run |= 1 << MEMORY_MARCH_B;
	ret = memory_march_b(start, size, flags);
	if (ret)
	    *failed |= 1 << MEMORY_MARCH_B;

	WATCHDOG_RESET();
	*run |= 1 << MEMORY_MARCH_LR;
	ret = memory_march_lr(start, size, flags);
	if (ret)
	    *failed |= 1 << MEMORY_MARCH_LR;

    } else if (flags & POST_POWERON) {

	/* 
	 * First power-on of the board (probably at MFG)
	 * Run atleast the checkerboard test on all memory
	 */

	WATCHDOG_RESET();
	*run |= 1 << MEMORY_CHECKERBOARD;
	ret = memory_checkerboard_test(start, size, flags);
	if (ret)
	    *failed |= 1 << MEMORY_CHECKERBOARD;

	/*
	 * These tests take a while to execute, but atleast run on the memory 
	 * u-boot will use for relocation, stack, global data, malloc area + 8K
	 * for exception vecs
	 */
	WATCHDOG_RESET();
	*run |= 1 << MEMORY_MARCH_C;
	ret = memory_march_c(UBOOT_MEMSTART, UBOOT_MEMSIZE, flags) |
	      memory_march_c(0, SIZE_8KB, flags);
	if (ret)
	    *failed |= 1 << MEMORY_MARCH_C;

	WATCHDOG_RESET();
	*run |= 1 << MEMORY_MARCH_C_MINUS;
	ret = memory_march_c_minus(UBOOT_MEMSTART, UBOOT_MEMSIZE, flags) |
	      memory_march_c_minus(0, SIZE_8KB, flags);
	if (ret)
	    *failed |= 1 << MEMORY_MARCH_C_MINUS;

	WATCHDOG_RESET();
	*run |= 1 << MEMORY_MARCH_B;
	ret = memory_march_b(UBOOT_MEMSTART, UBOOT_MEMSIZE, flags) |
	      memory_march_b(0, SIZE_8KB, flags);
	if (ret)
	    *failed |= 1 << MEMORY_MARCH_B;

	WATCHDOG_RESET();
	*run |= 1 << MEMORY_MARCH_LR;
	ret = memory_march_lr(UBOOT_MEMSTART, UBOOT_MEMSIZE, flags) |
	      memory_march_lr(0, SIZE_8KB, flags);
	if (ret)
	    *failed |= 1 << MEMORY_MARCH_LR;

    } else {
	/*
	 * flags & POST_NORMAL => every boot in the field
	 * 
	 * Atleast run some tests on the memory that is to be used by u-boot
	 * for relocation, stack, global data, malloc area + 8K
	 * for exception vecs
	 */
	WATCHDOG_RESET();
	*run |= 1 << MEMORY_CHECKERBOARD;
	ret = memory_checkerboard_test(UBOOT_MEMSTART, UBOOT_MEMSIZE, flags) |
	      memory_checkerboard_test(0, SIZE_8KB, flags);
	if (ret)
	    *failed |= 1 << MEMORY_CHECKERBOARD;

    }

    if (rom_flag) {
	/* This test can affect memory OUTSIDE of specified range and hence run
	 * only from ROM */
	WATCHDOG_RESET();
	*run |= 1 << ADDRESS_DECODER;
	ret = memory_address_decoder_test(start, __ilog2(CFG_SDRAM_SIZE), flags);
	if (ret)
	    *failed |= 1 << ADDRESS_DECODER;
    }
}

void print_report_card (int level, unsigned long run_tests,
			unsigned long failed_tests, int flags)
{
    int i;

    POST_LOG(level, "------------------------------------------------------------\n");
    POST_LOG(level, "           MEMORY TEST                                RESULT\n");
    POST_LOG(level, "------------------------------------------------------------\n");

    for (i = DATALINE_WALKING_BIT; i <= ADDRESS_DECODER; i++) {
	if (run_tests & (1 << i)) {
	    POST_LOG(level, "[Test #%02d] %-40s = %s\n", i, testnames[i],
	    (failed_tests & (1 << i))? "FAILED" : "PASSED");
	}
    }
    POST_LOG(level, "           FINAL RESULT                             = %s\n",
	(failed_tests)? "FAILED" : "PASSED");
    POST_LOG(level, "------------------------------------------------------------\n");
	
}

static void memory_remap_noncacheable(void)
{
    /*
     * RAM is mapped from TLB index 8 upto a max of 8 entries
     */
    unsigned int ram_tlb_index = 8;
    unsigned int more_tlbs = 1;
    unsigned long mas0, mas1, mas2, mas3;

    while (more_tlbs && ram_tlb_index < 16) {
	mtspr(MAS0, TLB1_MAS0(1, ram_tlb_index, 0));
	asm volatile("tlbre;isync;msync");
	mas0 = mfspr(MAS0);
	mas1 = mfspr(MAS1);
	mas2 = mfspr(MAS2);
	mas3 = mfspr(MAS3);

	/*
	 * If mapping is valid, mark it as cache inhibited
	 */
	if (mas1 & MAS1_VALID) {
	    mas2 |= MAS2_I;
	    mtspr(MAS2,mas2);
	    asm volatile("isync;msync;tlbwe;isync");
	} else {
	    more_tlbs = 0;
	}
	ram_tlb_index++;
    }
}

static void memory_remap_cacheable(void)
{
    /*
     * RAM is mapped from TLB index 8 upto a max of 8 entries
     */
    unsigned int ram_tlb_index = 8;
    unsigned int more_tlbs = 1;
    unsigned long mas0, mas1, mas2, mas3;

    while (more_tlbs && ram_tlb_index < 16) {
	mtspr(MAS0, TLB1_MAS0(1, ram_tlb_index, 0));
	asm volatile("tlbre;isync;msync");
	mas0 = mfspr(MAS0);
	mas1 = mfspr(MAS1);
	mas2 = mfspr(MAS2);
	mas3 = mfspr(MAS3);

	/*
	 * If mapping is valid, mark it as cacheacble
	 */
	if (mas1 & MAS1_VALID) {
	    mas2 &= ~MAS2_I;
	    mtspr(MAS2,mas2);
	    asm volatile("isync;msync;tlbwe;isync");
	} else {
	    more_tlbs = 0;
	}
	ram_tlb_index++;
    }
}

int memory_post_test_diag(int flags)
{
    unsigned long run_tests = 0, failed_tests = 0;

    unsigned long mstart = UBOOT_MEMSTART;
    unsigned long mend   = UBOOT_MEMSTART + UBOOT_MEMSIZE - 1;

    if ((flags & POST_RAM) && !(flags & POST_MANUAL)) {
	/*
	 * only manually running memory test from u-boot prompt will result in
	 * this test to actually get running from RAM
	 */
	return 0;
    }

    /* 
     * Disable the memory caching before running memory test
     */
    memory_remap_noncacheable();

    POST_LOG(POST_LOG_IMP, "===================MEMORY TEST START==============="
	     "====\n");
    POST_LOG(POST_LOG_INFO, "Flags               : %08X\n", flags);

    if (rom_flag) {
	POST_LOG(POST_LOG_INFO, "Test running out of : ROM\n");
	POST_LOG(POST_LOG_IMP,  "Memory to be tested : %08X - %08X (%d MB)\n", 
		 CFG_SDRAM_BASE, CFG_SDRAM_BASE + CFG_SDRAM_SIZE - 1,
		 CFG_SDRAM_SIZE >> 20);
	/*
	 * Test all memory
	 */
	memory_post_tests(CFG_SDRAM_BASE, CFG_SDRAM_SIZE, flags,
			  &run_tests, &failed_tests);
    } else {
	POST_LOG(POST_LOG_INFO, "Test running out of : RAM\n");
	POST_LOG(POST_LOG_IMP,  "Memory to be tested : %08X - %08X (~%d MB)\n",
		 CFG_SDRAM_BASE + SIZE_8KB, CFG_SDRAM_BASE + mstart - 1, 
		 (mstart - CFG_SDRAM_BASE - SIZE_8KB) >> 20);
	POST_LOG(POST_LOG_IMP,  "Memory to be tested : %08X - %08X (~%d MB)\n",
		 mend +1, CFG_SDRAM_SIZE - 1,
		 (CFG_SDRAM_SIZE - (mend + 1)) >> 20);

	/*
	 * Test all memory except:
	 * - 8K for vectors @ [0x00000000]
	 * - Memory used by u-boot for relocation
	 */
	memory_post_tests(CFG_SDRAM_BASE + SIZE_8KB, 
			  mstart - CFG_SDRAM_BASE - SIZE_8KB, 
			  flags, &run_tests, &failed_tests);
	memory_post_tests(mend + 1, 
			  CFG_SDRAM_SIZE - (mend + 1), 
			  flags, &run_tests, &failed_tests);
    }

    if (failed_tests) {
	POST_LOG(POST_LOG_ERR, "Final Memory test Result: FAILURE!\n");
	print_report_card(POST_LOG_ERR, run_tests, failed_tests, flags);
	printf("Memory Test... FAILED!\n");
    } else {
	POST_LOG(POST_LOG_IMP, "Final memory test Result: SUCCESS!\n");
	print_report_card(POST_LOG_INFO, run_tests, failed_tests, flags);
	printf("Memory Test... SUCCESSFUL!\n");
    }

    POST_LOG(POST_LOG_IMP, "===================MEMORY TEST END================="
	     "====\n");
    /* 
     * Enable the memory caching after running memory test
     */
    memory_remap_cacheable();

    if (failed_tests)
	return -1;
    else
	return 0;
}


#endif /* CONFIG_POST & CFG_POST_MEMORY */
#endif /* CONFIG_POST */
