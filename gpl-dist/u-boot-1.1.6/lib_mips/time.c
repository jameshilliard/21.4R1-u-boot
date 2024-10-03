/*
 * (C) Copyright 2003
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

#ifdef CONFIG_RMI_XLS
#define RMI_XLS_IO_DEV_PIC 0xBEF08000
#define RMI_XLS_TIMER_CHUNK0_OFFSET(timer_num) ((0x120 + timer_num) * 4)
#define RMI_XLS_TIMER_CHUNK1_OFFSET(timer_num) ((0x130 + timer_num) * 4)
#define RMI_XLS_TIMER_CHUNK0_MAX_VALUE_OFFSET(timer_num) ((0x100 + timer_num) * 4)
#define RMI_XLS_TIMER_CHUNK1_MAX_VALUE_OFFSET(timer_num) ((0x110 + timer_num) * 4)
#define RMI_PIC_TIMER_FREQ (66666667)
#define CLK_ADJUST_RATIO ((CFG_HZ)/(RMI_PIC_TIMER_FREQ))
static uint64_t max_timer_value = ~(0);
static uint64_t max_value = 0;
static uint32_t max_value_chunk0;
static uint32_t max_value_chunk1;

extern uint64_t rmi_cpu_frequency;
#endif

static inline void mips_compare_set(u32 v)
{
	asm volatile ("mtc0 %0, $11" : : "r" (v));
}

static inline void mips_count_set(u32 v)
{
	asm volatile ("mtc0 %0, $9" : : "r" (v));
}

#ifdef CONFIG_OCTEON
uint64_t octeon_get_cycles(void);
static inline uint64_t mips_count_get(void)
{
    return(octeon_get_cycles());
}
#elif defined (CONFIG_RMI_XLS)
   /* The 32 bit count register that MIPS provides overflows very frequently.
    * Hence introducing a 66.66 MHZ clock and scaling it to the actual clock.
    * The PIC registers are 64 bit and are stored as two 32bit chunks. Chunk 0 
    * is the LSB and the chunk1 is the MSB.*/
static inline uint64_t get_64bit_value (uint32_t *low_addr, uint32_t *highaddr)
{
        volatile uint64_t count;
	volatile uint32_t count_high0 = 0, count_high1 = 0, count_low = 0;
	do {
	        count_high0 = (uint32_t) *highaddr;
		count_low = (uint32_t) *low_addr;
		count_high1 = (uint32_t) *highaddr;
	} while (count_high0 != count_high1);
	count = count_high0;
	count = ((count << 32) | count_low);
	return count;
}

    /* The system timer 7 is used for the 66.67MHZ clock */ 	

static inline uint64_t mips_count_get(void)
{
    volatile uint64_t count;
    volatile uint32_t *chunk0_addr = (volatile uint32_t *)(RMI_XLS_IO_DEV_PIC + RMI_XLS_TIMER_CHUNK0_OFFSET(7));
    volatile uint32_t *chunk1_addr = (volatile uint32_t *)(RMI_XLS_IO_DEV_PIC + RMI_XLS_TIMER_CHUNK1_OFFSET(7));
    count = get_64bit_value (chunk0_addr, chunk1_addr);
    return ((max_value - count) * CLK_ADJUST_RATIO);
}

#else
static inline u32 mips_count_get(void)
{
	u32 count;

	asm volatile ("mfc0 %0, $9" : "=r" (count) :);
	return count;
}
#endif

/*
 * timer without interrupts
 */

int timer_init(void)
{
#ifdef CONFIG_RMI_XLS
        max_value = (max_timer_value) / (CLK_ADJUST_RATIO);
	max_value_chunk0 = (max_value & 0x00000000ffffffff);
	max_value_chunk1 = (max_value >> 32);
	*((volatile uint32_t *)(RMI_XLS_IO_DEV_PIC + RMI_XLS_TIMER_CHUNK0_MAX_VALUE_OFFSET(7))) = max_value_chunk0;
	*((volatile uint32_t *)(RMI_XLS_IO_DEV_PIC + RMI_XLS_TIMER_CHUNK1_MAX_VALUE_OFFSET(7))) = max_value_chunk1;
	*((volatile uint32_t *)RMI_XLS_IO_DEV_PIC) = 0x8006;
#else	
	mips_compare_set(0);
	mips_count_set(0);
#endif
	return 0;
}

void reset_timer(void)
{
	mips_count_set(0);
}

#if defined (CONFIG_OCTEON) || defined (CONFIG_RMI_XLS)
uint64_t get_timer(uint64_t base)
{
    return mips_count_get() - base;
}
#else
ulong get_timer(ulong base)
{
	return mips_count_get() - base;
}
#endif

void set_timer(ulong t)
{
	mips_count_set(t);
}

void udelay (unsigned long usec)
{
	ulong tmo;
	ulong start = get_timer(0);

	tmo = usec * (CFG_HZ / 1000000);
	while ((ulong)((mips_count_get() - start)) < tmo)
		/*NOP*/;
}

/*
 * This function is derived from PowerPC code (read timebase as long long).
 * On MIPS it just returns the timer value.
 */
unsigned long long get_ticks(void)
{
	return mips_count_get();
}

/*
 * This function is derived from PowerPC code (timebase clock frequency).
 * On MIPS it returns the number of timer ticks per second.
 */
ulong get_tbclk(void)
{
	return CFG_HZ;
}

/*
 * This function exported via jumptable is used by secondary
 * loader for autoboot on JSRXNLE platform instead of get_timer,
 * as get_timer returns current timer ticks value as 64 bits, but
 * the function in junos loader which uses get_time expects return
 * value to be 32 bit. Also, a millisecond timer is expected in the
 * secondary loader.
 */
uint32_t
get_msec_timer (uint32_t base)
{
    uint64_t ticks = 0;
    ticks = get_timer((uint64_t)base);
    /*
     * get_timer return ticks from boot up time
     * convert the ticks to milliseconds
     */
    ticks = (ticks / (CFG_HZ / 1000));
    return ((uint32_t)ticks);
}

