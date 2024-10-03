/*
 * Copyright (c) 2006-2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * (C) Copyright 2000
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

/*
 * Cache support: switch on or off, get status
 */
#include <common.h>
#include <command.h>

#if (CONFIG_COMMANDS & CFG_CMD_CACHE)

#define DEBUG_L1CACHE

#if defined(DEBUG_L1CACHE)
__inline__ void enable_icache (void)
{
#if 0
	unsigned long l1csr1;
	asm volatile("mfspr %0, 1011" : "=r" (l1csr1) :);
	l1csr1 |= 0x00000002;  /* invalidate icache */
	asm volatile("mtspr 1011, %0" : : "r" (l1csr1));
	asm ("isync");
	asm volatile("mfspr %0, 1011" : "=r" (l1csr1) :);
	l1csr1 |= 0x00000001;
	asm volatile("mtspr 1011, %0" : : "r" (l1csr1));
	asm ("isync");
#endif
	unsigned long l1csr1;
	asm volatile("mfspr %0, 1011" : "=r" (l1csr1) :);
	l1csr1 |= 0x00000003;  /* invalidate icache */
	asm volatile("mtspr 1011, %0" : : "r" (l1csr1));
	asm ("isync");
	asm ("isync");
	l1csr1 &= 0xFFFFFFFD;
	asm volatile("mtspr 1011, %0" : : "r" (l1csr1));
	asm ("isync");
	asm ("isync");
}

__inline__ void disable_icache (void)
{
#if 0
	unsigned long l1csr1;
	asm volatile("mfspr %0, 1011" : "=r" (l1csr1) :);
	l1csr1 &= 0xFFFFFFFE;
	asm volatile("mtspr 1011, %0" : : "r" (l1csr1));
	asm ("isync");
#endif
	unsigned long l1csr1;
	asm volatile("mfspr %0, 1011" : "=r" (l1csr1) :);
	l1csr1 &= 0xFFFFFFFC;
	asm volatile("mtspr 1011, %0" : : "r" (l1csr1));
	asm ("isync");
       asm ("isync");
       asm ("isync");
}

static __inline__ int status_icache (void)
{
	unsigned long l1csr1;
	asm volatile("mfspr %0, 1011" : "=r" (l1csr1) :);
	return (l1csr1 & 0x1);
}

static __inline__ void enable_dcache (void)
{
#if 0
	unsigned long l1csr0;
	asm volatile("mfspr %0, 1010" : "=r" (l1csr0) :);
	l1csr0 |= 0x00000002;  /* invalidate dcache */
	asm ("msync; isync");
	asm volatile("mtspr 1010, %0" : : "r" (l1csr0));
	asm ("isync");
	asm volatile("mfspr %0, 1010" : "=r" (l1csr0) :);
	l1csr0 |= 0x00000001;
	asm ("msync; isync");
	asm volatile("mtspr 1010, %0" : : "r" (l1csr0));
	asm ("isync");
#endif
	unsigned long l1csr0;
	asm volatile("mfspr %0, 1010" : "=r" (l1csr0) :);
	l1csr0 |= 0x00000003;  /* invalidate dcache */
	asm ("msync; isync");
	asm volatile("mtspr 1010, %0" : : "r" (l1csr0));
	asm ("isync");
	asm ("isync");
	l1csr0 &= 0xFFFFFFFD;
	asm ("msync; isync");
	asm volatile("mtspr 1010, %0" : : "r" (l1csr0));
	asm ("isync");
	asm ("isync");
}

static __inline__ void disable_dcache (void)
{
#if 0
	unsigned long l1csr0;
	asm volatile("mfspr %0, 1010" : "=r" (l1csr0) :);
	l1csr0 &= 0xFFFFFFFE;
	asm ("msync; isync");
	asm volatile("mtspr 1010, %0" : : "r" (l1csr0));
	asm ("isync");
#endif
	unsigned long l1csr0;
	asm volatile("mfspr %0, 1010" : "=r" (l1csr0) :);
	l1csr0 &= 0xFFFFFFFC;
	asm ("msync; isync");
	asm volatile("mtspr 1010, %0" : : "r" (l1csr0));
	asm ("isync");
       asm ("isync");
}

static __inline__ int status_dcache (void)
{
	unsigned long l1csr0;
	asm volatile("mfspr %0, 1010" : "=r" (l1csr0) :);
	return (l1csr0 & 0x1);
}

#endif

static int on_off (const char *);

int do_icache ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	switch (argc) {
	case 2:			/* on / off	*/
		switch (on_off(argv[1])) {
#if 0	/* prevented by varargs handling; FALLTROUGH is harmless, too */
		default: printf ("Usage:\n%s\n", cmdtp->usage);
			return;
#endif
		case 0:	
#if defined(DEBUG_L1CACHE)
                     disable_icache();
#else
                     icache_disable();
#endif
			break;
		case 1:	
#if defined(DEBUG_L1CACHE)
                     enable_icache();
#else
                     icache_enable ();
#endif
			break;
		}
		/* FALL TROUGH */
	case 1:			/* get status */
		printf ("Instruction Cache is %s\n",
#if defined(DEBUG_L1CACHE)
                     status_icache()
#else
			icache_status()
#endif
			? "ON" : "OFF");
		return 0;
	default:
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
	return 0;
}

int do_dcache ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	switch (argc) {
	case 2:			/* on / off	*/
		switch (on_off(argv[1])) {
#if 0	/* prevented by varargs handling; FALLTROUGH is harmless, too */
		default: printf ("Usage:\n%s\n", cmdtp->usage);
			return;
#endif
		case 0:	
#if defined(DEBUG_L1CACHE)
                     disable_dcache();
#else
                     dcache_disable();
#endif
			break;
		case 1:	
#if defined(DEBUG_L1CACHE)
                     enable_dcache();
#else
                     dcache_enable ();
#endif
			break;
		}
		/* FALL TROUGH */
	case 1:			/* get status */
		printf ("Data (writethrough) Cache is %s\n",
#if defined(DEBUG_L1CACHE)
                     status_dcache()
#else
			dcache_status()
#endif
			? "ON" : "OFF");
		return 0;
	default:
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
	return 0;

}

static int on_off (const char *s)
{
	if (strcmp(s, "on") == 0) {
		return (1);
	} else if (strcmp(s, "off") == 0) {
		return (0);
	}
	return (-1);
}


U_BOOT_CMD(
	icache,   2,   1,     do_icache,
	"icache  - enable or disable instruction cache\n",
	"[on, off]\n"
	"    - enable or disable instruction cache\n"
);

U_BOOT_CMD(
	dcache,   2,   1,     do_dcache,
	"dcache  - enable or disable data cache\n",
	"[on, off]\n"
	"    - enable or disable data (writethrough) cache\n"
);

#endif	/* CFG_CMD_CACHE */
