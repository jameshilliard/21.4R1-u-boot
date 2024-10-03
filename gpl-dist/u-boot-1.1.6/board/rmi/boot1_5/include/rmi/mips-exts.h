/***********************************************************************
Copyright 2003-2006 Raza Microelectronics, Inc.(RMI). All rights
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
#ifndef __MIPS_EXTS_H__
#define __MIPS_EXTS_H__

#include "rmi/interrupt.h"
#include "rmi/mipscopreg.h"

#define COP_0_OSSCRATCH $22
#define COP_0_EBASE     $15
#define COP_0_PERF_CTR  $25

#ifndef __ASSEMBLY__

#define read_64bit_cp0_register_sel(source, sel)                \
({ unsigned long long __res;                                    \
        __asm__ __volatile__(                                   \
	".set\tpush\n\t"					\
        ".set mips64\n\t"                                       \
        "dmfc0\t%0,"STR(source)", %1\n\t"                        \
	".set\tpop"						\
        : "=r" (__res) : "i" (sel) );                           \
        __res;})

#define write_64bit_cp0_register_sel(reg, value, sel)           \
        __asm__ __volatile__(                                   \
	".set\tpush\n\t"					\
        ".set mips64\n\t"                                       \
        "dmtc0\t%0,"STR(reg)", %1\n\t"                           \
	".set\tpop"						\
        : : "r" (value), "i" (sel) );

#define read_32bit_cp0_register_sel(source, sel)                \
({ unsigned int __res;                                           \
        __asm__ __volatile__(                                   \
	".set\tpush\n\t"					\
        ".set mips32\n\t"                                       \
        "mfc0\t%0,"STR(source)", %1\n\t"                        \
	".set\tpop"						\
        : "=r" (__res) : "i" (sel) );                           \
        __res;})

#define write_32bit_cp0_register_sel(reg, value, sel)           \
        __asm__ __volatile__(                                   \
	".set\tpush\n\t"					\
        ".set mips32\n\t"                                       \
        "mtc0\t%0,"STR(reg)", %1\n\t"                           \
	".set\tpop"						\
        : : "r" (value), "i" (sel) );

#define read_32bit_cp0_register(reg)      read_32bit_cp0_register_sel(reg,0)
#define write_32bit_cp0_register(reg,val) write_32bit_cp0_register_sel(reg,val,0)

#define read_c0_status()	read_32bit_cp0_register(COP_0_STAT)
#define write_c0_status(val)	write_32bit_cp0_register(COP_0_STAT, val)

#define read_mips32_cp0_config1() read_32bit_cp0_register_sel(COP_0_CONFIG,1)

#define read_phnx_eirr() read_64bit_cp0_register_sel(COP_0_COUNT, 6)
#define write_phnx_eirr(value) write_64bit_cp0_register_sel(COP_0_COUNT, value, 6)

#define read_phnx_eimr() read_64bit_cp0_register_sel(COP_0_COUNT, 7)
#define write_phnx_eimr(value) write_64bit_cp0_register_sel(COP_0_COUNT, value, 7)

#define read_phnx_ctrl(addr)                                    \
 ({unsigned long long __res;                                     \
         __asm__ __volatile__ (                                 \
        "mfcr %0, %1\n" : "=r"(__res) : "r"(addr));             \
        __res;})

#define write_phnx_ctrl(addr, value)                            \
        __asm__ __volatile__ (                                  \
        "mtcr %0, %1\n": : "r" (value), "r" (addr));

static __inline__ int processor_id(void)
{
	unsigned int cpu = 0;

	__asm__ volatile (".set push\n"
			  ".set mips64\n"
			  "mfc0 %0, $15, 1\n" ".set pop\n":"=r" (cpu)
	    );
	return (cpu & 0x1f);
}

static __inline__ int phnx_cpu_id(void)
{
	unsigned int cpu = 0;

	__asm__ volatile (".set push\n"
			  ".set mips64\n"
			  "mfc0 %0, $15, 1\n" ".set pop\n":"=r" (cpu)
	    );
	return ((cpu & 0x1c) >> 2);
}

static __inline__ int phnx_thr_id(void)
{
	unsigned int cpu = 0;

	__asm__ volatile (".set push\n"
			  ".set mips64\n"
			  "mfc0 %0, $15, 1\n" ".set pop\n":"=r" (cpu)
	    );
	return (cpu & 0x03);
}

static __inline__ int smp_processor_id(void)
{
	return processor_id();
}

/*******************************************************************/
static __inline__ int ldadd_w(unsigned int value, int *addr)
{
	__asm__ __volatile__(".set push\n"
			     ".set noreorder\n" "move $8, %2\n" "move $9, %3\n"
			     //"ldaddw $8, $9\n"
			     ".word 0x71280010\n"
			     "move %0, $8\n"
			     ".set pop\n":"=&r"(value), "+m"(*addr)
			     :"0"(value), "r"((unsigned long)addr)
			     :"$8", "$9");
	return value;
}

static __inline__ unsigned int ldadd_wu(unsigned int value, unsigned int *addr)
{
	__asm__ __volatile__(".set push\n"
			     ".set noreorder\n" "move $8, %2\n" "move $9, %3\n"
			     //"ldaddwu $8, $9\n"
			     ".word 0x71280011\n"
			     "move %0, $8\n"
			     ".set pop\n":"=&r"(value), "+m"(*addr)
			     :"0"(value), "r"((unsigned long)addr)
			     :"$8", "$9");
	return value;
}

static __inline__ int swap_w(unsigned int value, int *addr)
{
	__asm__ __volatile__(".set push\n"
			     ".set noreorder\n" "move $8, %2\n" "move $9, %3\n"
			     //"swapw $8, $9\n"
			     ".word 0x71280014\n"
			     "move %0, $8\n"
			     ".set pop\n":"=&r"(value), "+m"(*addr)
			     :"0"(value), "r"((unsigned long)addr)
			     :"$8", "$9");
	return value;
}

static __inline__ int test_and_set(int *addr)
{
	int oldval = 0;
	oldval = swap_w(1, addr);
	return oldval == 0 ? 1 /*success */ : 0 /*fail */ ;
}

/*******************************************************************/

#define PERF_CTR_INSTR_FETCHED           0
#define PERF_CTR_ICACHE_MISSES           1
#define PERF_CTR_SLEEP_CYCLES           12
#define PERF_CTR_INSTR_RETIRED          17
#define PERF_CTR_BRJMP_INSTR            20
#define PERF_CTR_BRJMP_FLUSH            21
#define PERF_CTR_REPLAYFLUSH            27
#define PERF_CTR_REPLAYFLUSH_LDUSE      28
#define PERF_CTR_L1_HIT                 38
#define PERF_CTR_L1_REF                 39
#define PERF_CTR_SNOOP_UPGRADE_FAIL     47
#define PERF_CTR_SNOOP_TRANSFERS        48
#define PERF_CTR_SNOOP_HITS             49
#define PERF_CTR_SNOOP_OPS              50
#define PERF_CTR_CYCLES                 63

#define PERF_CTR_EVENT0        0
#define PERF_CTR_EVENT0_VALUE  1
#define PERF_CTR_EVENT1        2
#define PERF_CTR_EVENT1_VALUE  3
#define perf_ctr_start(event, ctr) write_32bit_cp0_register_sel(COP_0_PERF_CTR, (0x80002002|((event)<<5)), ctr)
#define perf_ctr_stop(ctr) write_32bit_cp0_register_sel(COP_0_PERF_CTR, 0x80002000, ctr)
#define perf_ctr_reset(ctr) write_32bit_cp0_register_sel(COP_0_PERF_CTR, 0, ctr)
#define perf_ctr_read(ctr) read_32bit_cp0_register_sel(COP_0_PERF_CTR, ctr)

/* TODO: install the new versions of gcc and newlib */
#ifdef __gnu__elf__xlr__

#if (_MIPS_SIM == _MIPS_SIM_ABI64)
#define PTR2U64(x) (unsigned long)(x)
#define U642PTR(x) (void *)((unsigned long)(x))
#else
// preserve sign-extension
#define PTR2U64(x) (int)(x)
#define U642PTR(x) (void *)((int)(x))
#endif

#else				// Assume 32-bit ABI
// preserve sign-extension
#define PTR2U64(x) (int)(x)
#define U642PTR(x) (void *)((int)(x))
#endif

#else				//__ASSEMBLY__

.macro xlr_cpu_id id, temp0
/*
 * Read the cpu id from cp0 EBASE sel 1 register.
 * cpuid[9:5] -> always 0
 * cpuid[4:2] -> Physcial CPU ID
 * cpuid[1:0] -> thrid
 */
	mfc0	\id, COP_0_EBASE, 1
	srl     \id, \id, 2
	andi    \id, \id, 0x7
        sll     \id, \id, 2
    /* get the thread id */
        mfc0   \temp0, COP_0_EBASE, 1
        andi   \temp0, 0x3
    /* calculate the linear cpu id */
        addu    \id, \id, \temp0
.endm
 
#endif

#endif

