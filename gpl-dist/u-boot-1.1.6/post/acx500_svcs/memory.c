/*
 * (C) Copyright 2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * Copyright (c) 2014, Juniper Networks, Inc.
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

/*
 * NOTE: Source of this file is SB/vendor/u-boot/post/memory.c
 * This has been suitably modified for octeon.
 */

#include <common.h>
#include <watchdog_cpu.h>

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

DECLARE_GLOBAL_DATA_PTR;

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


int
move64 (unsigned long long *src, unsigned long long *dest)
{
    if ((unsigned long)src & (0x8UL - 1)
        || (unsigned long)dest & (0x8UL - 1)) {
        printf("Source or destination addresses are\n"
               "not 8-byte aligned!!!\n");
        return -1;
    }

    __asm__ __volatile__ (".set mips64      \n\t"
                          "ld   $9, 0(%0)   \n\t"
                          "nop              \n\t"
                          "nop              \n\t"
                          "nop              \n\t"
                          "sd   $9, 0(%1)   \n\t"
                          ".set mips0       \n\t"
                          ::"g"(src), "g"(dest));

    return 0;
}

void
memory_barrier (void)
{
    __asm__ __volatile__ ("syncw\n\t"
                          :::"memory");
}


static int 
memory_post_tests (unsigned long start, unsigned long size)
{
    int ret = 0;

    printf("Starting Memory POST... \n");

    printf("Checking datalines... ");

    ret = memory_post_dataline ((unsigned long long *)start);

    if (ret == 0) {
        printf("OK\n");
        printf("Checking address lines... ");
        ret = memory_post_addrline((ulong *)start, (ulong *)start, size);
        if (ret == 0) {
            ret = memory_post_addrline((ulong *)(start + size - 8), (ulong *)start, size);
        }
    }

    if (ret == 0) {
        printf("OK\n");
        printf("Checking 1MB memory for U-Boot... ");
        ret = memory_post_test1 (start, size, 0xAAAAAAAA);
    }

    if (ret) {
        printf("Failed!\n");
    } else {
        printf("OK.\n");
    }

    return ret;
}

int
memory_post_test (int flags)
{
    int ret = 0;

    bd_t *bd = gd->bd;

    /* test 1 MB */
    ret = memory_post_tests (CFG_SDRAM_BASE, (1 * 1024 * 1024));

    return ret;
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

static uint32_t
oct_cache_evict_address (uint32_t src_addr, uint32_t increment)
{
    uint32_t    evict_addr, l2cSet, mid, high;

    l2cSet = ((src_addr >> 16) & 0x1FFUL) ^ ((src_addr >> 7) & 0x1FFUL);

    evict_addr = src_addr + increment;  /* One cache block more */

    mid = (evict_addr >> 7) & 0x1FFUL;
    high = mid ^ l2cSet;
    evict_addr = (evict_addr & 0xFC00FFFFUL) | (high << 16);

    return evict_addr;
}

void
flush_cache_line (unsigned long *src_addr)
{
    volatile uint32_t *evict_address;

    evict_address = oct_cache_evict_address((uint32_t)src_addr, 0x80);

    *evict_address = *src_addr;
}

int
memory_post_dataline(unsigned long long * pmem)
{
    unsigned long long temp64 = 0;
    int                num_patterns = sizeof(pattern)/ sizeof(pattern[0]);
    int                i;
    unsigned int       hi, lo, pathi, patlo;
    int                ret = 0;

    for ( i = 0; i < num_patterns; i++) {
        move64((unsigned long long *)&(pattern[i]), pmem);

        /* flush the cache line if required. */
        flush_cache_line(pmem);

        pmem++;

        /*
         * Put a different pattern on the data lines: otherwise they
         * may float long enough to read back what we wrote.
         */
        move64((unsigned long long *)&otherpattern, pmem);

        /* flush cache line */
        flush_cache_line(pmem);

        pmem--;

        move64(pmem, &temp64);

#ifdef INJECT_DATA_ERRORS
        temp64 ^= 0x00008000;
#endif

        if (temp64 != pattern[i]) {
            pathi = (pattern[i]>>32) & 0xffffffff;
            patlo = pattern[i] & 0xffffffff;

            hi = (temp64>>32) & 0xffffffff;
            lo = temp64 & 0xffffffff;

            post_log ("Memory (date line) error at %08x, "
                      "wrote %08x%08x, read %08x%08x !\n",
                      pmem, pathi, patlo, hi, lo);
            ret = -1;
        }
    }
    return ret;
}

int
memory_post_addrline(ulong *testaddr, ulong *base, ulong size)
{
    ulong *target;
    ulong *end;
    ulong  readback;
    ulong  xor;
    int    ret = 0;

    end = (ulong *)((ulong)base + size);	/* pointer arith! */
    xor = 0;
    for (xor = sizeof(ulong); xor > 0; xor <<= 1) {
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
                post_log ("Memory (address line) error at %08x<->%08x, "
                          "XOR value %08x !\n",
                          testaddr, target, xor);
                ret = -1;
            }
        }
    }
    return ret;
}

int 
memory_post_test1(unsigned long start, unsigned long size,
	          unsigned long val)
{
    unsigned long       i;
    ulong              *mem = (ulong *) start;
    ulong               readback, inc;
    ulong               rb1, rb2, rb3;
    int                 ret = 0;
    unsigned long long  start_ticks, end_ticks;

    start_ticks = get_ticks();
    inc = 4;

    for (i = 0; i < size / sizeof (ulong); i+=inc) {
        mem[i] = val;
        mem[i+1] = val;
        mem[i+2] = val;
        mem[i+3] = val;
    }

    for (i = 0; i < size / sizeof (ulong) && ret == 0; i+=inc) {
        readback = mem[i];
        rb1 = mem[i+1];
        rb2 = mem[i+2];
        rb3 = mem[i+3];

        if (readback != val) {
            printf("Memory error at %08x, "
                   "wrote %08x, read %08x !\n",
                   mem + i, val, readback);

            ret = -1;
            break;
        }
        if (rb1 != val) {
            printf("Memory error at %08x, "
                   "wrote %08x, read %08x !\n",
                   (mem + i + 1), val, rb1);

            ret = -1;
            break;
        }
        if (rb2 != val) {
            printf("Memory error at %08x, "
                   "wrote %08x, read %08x !\n",
                   (mem + i + 2), val, rb2);

            ret = -1;
            break;
        }
        if (rb3 != val) {
            printf("Memory error at %08x, "
                   "wrote %08x, read %08x !\n",
                   (mem + i + 3), val, rb3);

            ret = -1;
            break;
        }
    }

    return ret;
}

#endif /* CONFIG_POST & CFG_POST_MEMORY */
#endif /* CONFIG_POST */
