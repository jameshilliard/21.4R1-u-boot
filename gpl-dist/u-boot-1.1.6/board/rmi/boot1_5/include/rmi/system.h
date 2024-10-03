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
#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#define PAGE_SIZE (1<<12)

#define XKPHYS        0x8000000000000000
#define CCA_UNCACHED  2
#define CCA_CACHED    3

#ifndef __ASSEMBLY__
#include "rmi/types.h"
#include "rmi/interrupt.h"
#include "rmi/byteorder.h"
#include "asm/addrspace.h"

static inline unsigned long get_gp(void)
{
	unsigned long gp = 0;
	__asm__ volatile (".set push           \n"
			  ".set noreorder      \n"
			  " move %0, $28       \n"
			  ".set pop            \n":"=r" (gp)
	    );
	return gp;
}

static inline unsigned long get_sp(void)
{
	unsigned long sp = 0;
	__asm__ volatile (".set push           \n"
			  ".set noreorder      \n"
			  " move %0, $29       \n"
			  ".set pop            \n":"=r" (sp)
	    );
	return sp;
}

static inline void cpu_halt(void)
{
	cpu_disable_int();
	for (;;) 
	{
		__asm__ volatile ("wait\n");
	}
}

static inline unsigned long virt_to_phys(volatile void *address)
{
	unsigned long addr = (unsigned long)address;
	/* For now assume all addresses are in KSEG0 or KSEG1 */
	if (addr >= (unsigned long)KSEG2)
		return 0;
	if (addr >= (unsigned long)KSEG1)
		return addr - (unsigned long)KSEG1;
	return addr - (unsigned long)KSEG0;
}

static inline void *phys_to_virt(unsigned long address)
{
	/* For now assume all addresses are in KSEG0 */
	return (void *)((address & 0x1fffffff) + (unsigned long)KSEG0);
}

static inline void *phys_to_kseg0(unsigned long address)
{
	/* For now assume all addresses are in KSEG0 */
	return (void *)((address & 0x1fffffff) + (unsigned long)KSEG0);
}

static inline void *phys_to_kseg1(unsigned long address)
{
	/* For now assume all addresses are in KSEG0 */
	return (void *)((address & 0x1fffffff) + (unsigned long)KSEG1);
}

static inline unsigned short phoenix_inw(unsigned long port)
{
	return (*(volatile unsigned short *)((port)));

}
static inline void phoenix_outw(unsigned long port, unsigned short value)
{
	*(volatile unsigned short *)((port)) = (value);
}
static inline unsigned char phoenix_inb(unsigned long port)
{
	return (*(volatile unsigned char *)(port));
}
static inline void phoenix_outb(unsigned long port, unsigned char value)
{
	*(volatile unsigned char *)((port)) = (value);

}

#ifndef __STR
#define __STR(x) #x
#endif
#ifndef STR
#define STR(x) __STR(x)
#endif


static __inline__ uint64_t ld_40bit_phys(uint64_t phys, int cca)
{
	uint64_t value = 0;

	__asm__ __volatile__(".set push\n"
			     ".set noreorder\n"
			     ".set mips64\n"
			     "dli   $8, " STR(XKPHYS) "\n"
			     "or    $8, $8, %2\n"
			     "daddu $8, $8, %1\n"
			     "ld    %0, 0($8) \n"
			     ".set pop\n"
			     :"=r"(value)
			     :"r"(phys & 0xfffffffff8ULL),"r"((uint64_t) cca << 59)
			     :"$8");

	return value;
}

static __inline__ uint32_t lw_40bit_phys(uint64_t phys, int cca)
{
	uint32_t value = 0;

	__asm__ __volatile__(".set push\n"
			     ".set noreorder\n"
			     ".set mips64\n"
			     "dli   $8, " STR(XKPHYS) "\n"
			     "or    $8, $8, %2\n"
			     "daddu $8, $8, %1\n"
			     "lw    %0, 0($8) \n"
			     ".set pop\n"
			     :"=r"(value)
			     :"r"(phys & 0xfffffffffcULL),"r"((uint64_t) cca << 59)
			     :"$8");

	return value;
}

static __inline__ uint16_t lh_40bit_phys(uint64_t phys, int cca)
{
	uint16_t value = 0;

	__asm__ __volatile__(".set push\n"
			     ".set noreorder\n"
			     ".set mips64\n"
			     "dli   $8, " STR(XKPHYS) "\n"
			     "or    $8, $8, %2\n"
			     "daddu $8, $8, %1\n"
			     "lhu    %0, 0($8) \n"
			     ".set pop\n"
			     :"=r"(value)
			     :"r"(phys & 0xfffffffffeULL),"r"((uint64_t) cca << 59)
			     :"$8");

	return value;
}

static __inline__ uint8_t lb_40bit_phys(uint64_t phys, int cca)
{
	uint8_t value = 0;

	__asm__ __volatile__(".set push\n"
			     ".set noreorder\n"
			     ".set mips64\n"
			     "dli   $8, " STR(XKPHYS) "\n"
			     "or    $8, $8, %2\n"
			     "daddu $8, $8, %1\n"
			     "lb    %0, 0($8) \n" ".set pop\n":"=r"(value)
			     :"r"(phys & 0xffffffffffULL),
			     "r"((uint64_t) cca << 59)
			     :"$8");

	return value;
}

static __inline__ void sd_40bit_phys(uint64_t phys, int cca, uint64_t value)
{
	__asm__ __volatile__(".set push\n"
			     ".set noreorder\n"
			     ".set mips64\n"
			     "dli   $8, " STR(XKPHYS) "\n"
			     "or    $8, $8, %2\n"
			     "daddu $8, $8, %1\n"
			     "sd    %0, 0($8) \n"
			     ".set pop\n"
			     :
			     :"r"(value), "r"(phys & 0xfffffffff8ULL), "r"((uint64_t) cca << 59)
			     :"$8"
		);
}

static __inline__ void sw_40bit_phys(uint64_t phys, int cca, uint32_t value)
{
	__asm__ __volatile__(".set push\n"
			     ".set noreorder\n"
			     ".set mips64\n"
			     "dli   $8, " STR(XKPHYS) "\n"
			     "or    $8, $8, %2\n"
			     "daddu $8, $8, %1\n"
			     "sw    %0, 0($8) \n"
			     ".set pop\n"
			     :
			     :"r"(value), "r"(phys & 0xfffffffffcULL), "r"((uint64_t) cca << 59)
			     :"$8"
		);
}

static __inline__ void sh_40bit_phys(uint64_t phys, int cca, uint32_t value)
{
  __asm__ __volatile__(".set push\n"
                       ".set noreorder\n"
                       ".set mips64\n"
                       "dli   $8, "STR(XKPHYS)"\n"
                       "or    $8, $8, %2\n"
                       "daddu $8, $8, %1\n"
                       "sh    %0, 0($8) \n"
                       ".set pop\n"
                       :
                       : "r"(value), "r"(phys & 0xfffffffffeULL), "r"((uint64_t)cca << 59)
                       : "$8"
	  );
}

static __inline__ void sb_40bit_phys(uint64_t phys, int cca, uint8_t value)
{
  __asm__ __volatile__(".set push\n"
                       ".set noreorder\n"
                       ".set mips64\n"
                       "dli   $8, "STR(XKPHYS)"\n"
                       "or    $8, $8, %2\n"
                       "daddu $8, $8, %1\n"
                       "sb    %0, 0($8) \n"
                       ".set pop\n"
                       :
                       : "r"(value), "r"(phys & 0xffffffffffULL), "r"((uint64_t)cca << 59)
                       : "$8"
	  );
}

static __inline__ uint64_t ld_40bit_phys_uncached(uint64_t phys)
{
	return ld_40bit_phys(phys, CCA_UNCACHED);
}
static __inline__ uint64_t ld_40bit_phys_cached(uint64_t phys)
{
	return ld_40bit_phys(phys, CCA_CACHED);
}

static __inline__ uint32_t lw_40bit_phys_uncached(uint64_t phys)
{
	return lw_40bit_phys(phys, CCA_UNCACHED);
}
static __inline__ uint32_t lw_40bit_phys_cached(uint64_t phys)
{
	return lw_40bit_phys(phys, CCA_CACHED);
}

static __inline__ uint16_t lh_40bit_phys_uncached(uint64_t phys)
{
	return lh_40bit_phys(phys, CCA_UNCACHED);
}
static __inline__ uint16_t lh_40bit_phys_cached(uint64_t phys)
{
	return lh_40bit_phys(phys, CCA_CACHED);
}

static __inline__ uint8_t lb_40bit_phys_uncached(uint64_t phys)
{
	return lb_40bit_phys(phys, CCA_UNCACHED);
}
static __inline__ uint8_t lb_40bit_phys_cached(uint64_t phys)
{
	return lb_40bit_phys(phys, CCA_CACHED);
}

static __inline__ void sd_40bit_phys_uncached(uint64_t phys, uint64_t value)
{
	sd_40bit_phys(phys, CCA_UNCACHED, value);
}
static __inline__ void sd_40bit_phys_cached(uint64_t phys, uint64_t value)
{
	sd_40bit_phys(phys, CCA_CACHED, value);
}

static __inline__ void sw_40bit_phys_uncached(uint64_t phys, uint32_t value)
{
	sw_40bit_phys(phys, CCA_UNCACHED, value);
}
static __inline__ void sw_40bit_phys_cached(uint64_t phys, uint32_t value)
{
	sw_40bit_phys(phys, CCA_CACHED, value);
}

static __inline__ void sh_40bit_phys_uncached(uint64_t phys, uint16_t value)
{
      sh_40bit_phys(phys, CCA_UNCACHED, value);
}
static __inline__ void sh_40bit_phys_cached(uint64_t phys, uint16_t value)
{
      sh_40bit_phys(phys, CCA_CACHED, value);
}

static __inline__ void sb_40bit_phys_uncached(uint64_t phys, uint8_t value)
{
      sb_40bit_phys(phys, CCA_UNCACHED, value);
}
static __inline__ void sb_40bit_phys_cached(uint64_t phys, uint8_t value)
{
      sb_40bit_phys(phys, CCA_CACHED, value);
}

/* Low level READ/WRITE Routines */
static inline void OUTL(uint32_t iobase, int value, unsigned long offset) 
{
    sw_40bit_phys_uncached((iobase + offset), __swab32(value));
}

static inline void OUTW(uint32_t iobase, int value, unsigned long offset) 
{
    sh_40bit_phys_uncached((iobase + offset), __swab16(value));
}

static inline void OUTB(uint32_t iobase, int value, unsigned long offset) 
{
    sb_40bit_phys_uncached((iobase + offset), value); 
}

static volatile inline unsigned int INL(uint32_t iobase, unsigned long offset) 
{
    volatile unsigned int retVal =
        lw_40bit_phys_uncached(iobase + offset);
    return (__swab32(retVal));
}
static volatile inline unsigned short int INW(uint32_t iobase, unsigned long offset) 
{
    volatile unsigned short int retVal = 
        lh_40bit_phys_uncached(iobase + offset);
    return (__swab16(retVal));
}

static volatile inline unsigned char INB(uint32_t iobase, unsigned long offset) 
{
    volatile unsigned short retVal = lb_40bit_phys_uncached(iobase + offset);
    return (retVal);
}

static volatile inline unsigned int INR(uint32_t iobase, unsigned long offset) 
{
    volatile unsigned int retVal =
        lw_40bit_phys_uncached(iobase + offset);
    return (retVal);
}

#endif

#endif
