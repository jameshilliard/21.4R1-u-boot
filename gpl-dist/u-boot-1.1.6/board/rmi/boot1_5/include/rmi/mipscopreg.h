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

#ifndef __MIPSCOPREG_H__
#define __MIPSCOPREG_H__

#include "rmi/system.h"

#define COP_0_INDEX        $0
#define COP_0_RANDOM       $1
#define COP_0_ENTRYLO0     $2
#define COP_0_ENTRYLO1     $3
#define COP_0_CONF         $3
#define COP_0_CONTEXT      $4
#define COP_0_PAGEMASK     $5
#define COP_0_WIRED        $6
#define COP_0_INFO         $7
#define COP_0_BADVADDR     $8
#define COP_0_COUNT        $9
#define COP_0_ENTRYHI     $10
#define COP_0_COMPARE     $11
#define COP_0_STATUS      $12
#define COP_0_CAUSE       $13
#define COP_0_EPC         $14
#define COP_0_PRID        $15
#define COP_0_CONFIG      $16
#define COP_0_LLADDR      $17
#define COP_0_WATCHLO     $18
#define COP_0_WATCHHI     $19
#define COP_0_XCONTEXT    $20
#define COP_0_FRAMEMASK   $21
#define COP_0_DIAGNOSTIC  $22
#define COP_0_OS_SCTACH   $22
#define COP_0_DEBUG       $23
#define COP_0_DEPC        $24
#define COP_0_PERFORMANCE $25
#define COP_0_ECC         $26
#define COP_0_CACHEERR    $27
#define COP_0_TAGLO       $28
#define COP_0_TAGHI       $29
#define COP_0_ERROREPC    $30
#define COP_0_DESAVE      $31

/* Register COP_0_STATUS bit definitions. 
   Only relevant bits are given */
#define STAT_0_KX                  0x00000080
#define STAT_0_NMI                 0x00080000
#define STAT_0_SR                  0x00100000
#define STAT_0_CU0                 0x10000000
#define STAT_0_CU2                 0x40000000


#ifndef __ASSEMBLY__

static inline void set_cop_entrylo_0_reg(unsigned long long val)
{
	__asm__ __volatile__(".set mips3\n"
			     "dsll  %L0, %L0, 32 \n"
			     "dsrl  %L0, %L0, 32 \n"
			     "dsll  %M0, %M0, 32 \n"
			     "or    %L0, %L0, %M0\n"
			     "dmtc0 %L0, $2      \n" 
			     ".set mips0         \n"
			     :
			     :"r"(val));
}

static inline void set_cop_entrylo_1_reg(unsigned long long val)
{
	
	__asm__ __volatile__(".set mips3\n"
			     "dsll  %L0, %L0, 32 \n"
			     "dsrl  %L0, %L0, 32 \n"
			     "dsll  %M0, %M0, 32 \n"
			     "or    %L0, %L0, %M0\n"
			     "dmtc0 %L0, $3      \n"
			     ".set mips0         \n"
			     :
			     :"r"(val));
}

static inline void set_cop_entryhi_reg(unsigned long val)
{
	__asm__ __volatile__(".set push    \n"
			     ".set reorder \n"
			     "mtc0 %z0, $10\n"
			     ".set pop     \n"
			     :
			     :"Jr"(val));
}

static inline unsigned long get_cop_entryhi_reg(void)
{
	unsigned long val;

	__asm__ __volatile__(".set push     \n"
			     ".set reorder  \n"
			     "mfc0 %0, $10  \n"
			     ".set pop      \n"
			     :"=r"(val));

	return val;
}

static inline void set_cop_index_reg(unsigned long val)
{
	__asm__ __volatile__(".set push   \n"
			     ".set reorder\n"
			     "mtc0 %z0, $0\n"
			     ".set pop    \n"
			     :
			     :"Jr"(val));
}

static inline unsigned long get_cop_index_reg(void)
{
	unsigned long val;

	__asm__ __volatile__(".set push   \n"
			     ".set reorder\n"
			     "mfc0 %0, $0 \n"
			     ".set pop    \n"
			     :"=r"(val));
	return val;
}
static inline void mips_tlb_probe(void)
{
	__asm__ __volatile__(".set push   \n"
			     ".set reorder\n"
			     "tlbp        \n"
			     ".set pop");
}

static inline void mips_tlb_write_indexed(void)
{
	__asm__ __volatile__(".set push   \n"
			     ".set reorder\n"
			     "tlbwi        \n"
			     ".set pop");
}

#endif

#endif
