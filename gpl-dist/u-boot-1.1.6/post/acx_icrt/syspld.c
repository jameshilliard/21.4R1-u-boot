/*
 * $Id$
 *
 * syspld.c -- Diagnostics for the syspld in iCRT
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
#if defined(CONFIG_ACX) && defined (CONFIG_POST)

#include <command.h>
#include <post.h>
#include "acx_icrt_post.h"
#include <acx_syspld.h>

#if CONFIG_POST & CFG_POST_SYSPLD

extern int get_bits(char *buf, unsigned long long val);

int post_result_syspld;

typedef uint8_t (* readfn)(void);
typedef void (* writefn)(uint8_t);

static int 
syspld_scratch_register_test (readfn read_reg, writefn write_reg, writefn write_altreg,
							  int flags)
{
	uint8_t pattern = 0, antipattern = 0;
	uint8_t readval = 0, err = 0, mismatch = 0;
	char bits[20];

	/* Walking 1(s) */
	POST_LOG(POST_LOG_INFO, "\t[Walking 1s Test] starting...\n");
	for (pattern = 1; pattern; pattern <<= 1) {
		antipattern = ~pattern;

		POST_LOG(POST_LOG_DEBUG, "\t\tTest pattern = %02X\n",  pattern);

		write_reg(antipattern);
		write_reg(pattern);
		/*
		 * Put a different pattern on the data lines: otherwise they
		 * may float long enough to read back what we wrote.
		 */
		write_altreg(antipattern);
		readval = read_reg();

#ifdef INJECT_DATA_ERRORS
		readval ^= 0x8;
#endif
		POST_LOG(POST_LOG_DEBUG, "\t\tReadback pattern = %02X\n",  readval);

		mismatch = readval ^ pattern;
		if (mismatch) {
			get_bits(bits, mismatch);
			POST_LOG(POST_LOG_ERR, "\t\t[SYSPLD Walking Bit Test]\n");
			POST_LOG(POST_LOG_ERR, "\t\t[Walking 1s] FAILURE! Anomaly at Syspld data PIN(s) %s\n", bits);
			get_bits(bits, pattern);
			POST_LOG(POST_LOG_ERR, "\t\t            (either stuck high, or shorted to PIN %s)\n", bits); 
			POST_LOG(POST_LOG_ERR, "\t\t            (I wrote 0x%02X, read back 0x%02X)\n", pattern, readval);
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

		POST_LOG(POST_LOG_DEBUG, "\t\tTest pattern = %02X\n",  pattern);

		write_reg(antipattern);
		write_reg(pattern);
		/*
		 * Put a different pattern on the data lines: otherwise they
		 * may float long enough to read back what we wrote.
		 */
		write_altreg(antipattern);
		readval = read_reg();

#ifdef INJECT_DATA_ERRORS
		readval ^= 0x4;
#endif
		POST_LOG(POST_LOG_DEBUG, "\t\tReadback pattern = %02X\n",  readval);

		mismatch = readval ^ pattern;
		if (mismatch) {
			get_bits(bits, mismatch);
			POST_LOG(POST_LOG_ERR, "\t\t[Dataline Walking Bit Test]\n");
			POST_LOG(POST_LOG_ERR, "\t\t[Walking 0s] FAILURE! Anomaly at Memory data PIN(s) %s\n", bits);
			get_bits(bits, antipattern);
			POST_LOG(POST_LOG_ERR, "\t\t            (either stuck low, or shorted to PIN %s)\n", bits); 
			POST_LOG(POST_LOG_ERR, "\t\t            (I wrote 0x%02X, read back 0x%02X)\n", pattern, readval);
			err |= 2;
		}
	}
	if (err & 2)
		POST_LOG(POST_LOG_ERR,  "\t[Walking 0s Test] ... FAILURE\n");
	else
		POST_LOG(POST_LOG_INFO, "\t[Walking 0s Test] ... SUCCESS\n");

	return err;
}

int syspld_post_test(int flags)
{
	unsigned int scratch1_val;

	post_result_syspld = 1;

	POST_LOG(POST_LOG_IMP, "===================SYSPLD TEST START===================\n");
	POST_LOG(POST_LOG_INFO, "SYSPLD Version: %02X Revision: %02X\n", syspld_version(), syspld_revision());
	POST_LOG(POST_LOG_INFO, "The test writes values into scratch registers & reads them back to verify that\n");
	POST_LOG(POST_LOG_INFO, "sysPLD registers can be read / written correctly. Multiple values are tried, and\n");
	POST_LOG(POST_LOG_INFO,	"the values are chosen in such a way that ensures that each data pin can be set\n");
	POST_LOG(POST_LOG_INFO,	"to 0 or 1 independent of the values on the other data pins. This is essentially\n");
	POST_LOG(POST_LOG_INFO,	"the Walking Bit Test (Walking 0s / Waling 1s) on SysPLD register. Both the\n");
	POST_LOG(POST_LOG_INFO,	"scratch registers are tested. This test can detect any stuck high or stuck\n");
	POST_LOG(POST_LOG_INFO,	"low pins, or any shorted pins, or even the floating lines, or inaccessible SysPLD\n");
	POST_LOG(POST_LOG_INFO,	"* Walking 1s: Walks a 1 from pin0 -> pin31 against a backdrop of 0s on all pins\n");
	POST_LOG(POST_LOG_INFO,	"* Walking 0s: Walks a 0 from pin0 -> pin31 against a backdrop of 1s on all pins\n");

	/*
	 * Save the current SYSPLD scratch register values, for restoring later
	 */
	scratch1_val = syspld_scratch1_read();

	/*
	 * Write a value into scratch register, read it and then compare.
	 * The read value should match with the value written earlier. The
	 * values are actually walking bit values.
	 */

	POST_LOG(POST_LOG_INFO, "[Test #1] Testing with SCRATCH1 register\n");
	if (syspld_scratch_register_test(syspld_scratch1_read, syspld_scratch1_write,
	                                   syspld_scratch2_write, flags)) {
		POST_LOG(POST_LOG_ERR, "[Test #1] SCRATCH1 register test FAILED\n");
		post_result_syspld = -1;
	} else {
		POST_LOG(POST_LOG_INFO, "[Test #1] SCRATCH1 register test SUCCESS\n");
	}

	/*
	 * Restore the value
	 */
	syspld_scratch1_write(scratch1_val);

	POST_LOG(POST_LOG_IMP, "===================SYSPLD TEST END=====================\n");

	if (post_result_syspld == -1)
		return -1;
	else
		return 0;
}

#endif /* CONFIG_POST & CFG_POST_SYSPLD */
#endif
