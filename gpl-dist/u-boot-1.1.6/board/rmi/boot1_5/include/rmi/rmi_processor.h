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
#ifndef __PROCESSOR_H__
#define __PROCESSOR_H__

#include "rmi/types.h"
#include "rmi/components.h"
#include "rmi/board.h"
#include "rmi/physaddrspace.h"
#include "rmi/byteorder.h"

#define cache_op(op, base) __asm__ __volatile__ ("cache %0, 0(%1)\n" : : "i"(op), "r"(base))

/* Data structures defining a processor chip */
struct processor;
struct processor_rev;
struct cpu;

struct processor_rev {
        int revision;
        const char *code_name;
};

struct cpu 
{
	/* per_cpu_arch_init() */
	void (*cpu_init) (void *cpu_data);
	/* Nothing for now */
	void (*cpu_reset) (void *cpu_data);

	void *cpu_data;
};

struct processor 
{
        const char *name;
        int processor_id;
        struct processor_rev *processor_rev;

	struct cpu *cpu;
	uint64_t cpu_frequency;

	int nr_cores;
        int nr_cpus;

	unsigned long io_base;
	unsigned long io_size;
	struct component_base *component_base;

	/* For XLR, this is message_ring_cpu_init */
	void (*cpu_ipc_init)(struct processor *this_chip);

	void (*park_cpu)(void *cpu_data);
	void (*wakeup_cpu) (int cpu_id, void *stack);

	void (*processor_init)(struct processor *this_processor);
	void (*processor_reset)(struct processor *this_processor);

	struct board *this_board;
};
struct bist_info
{
	unsigned int mask;
	char message[80];
};
struct processor *detect_processor(void);
struct processor *get_processor_info(void);
static inline struct cpu *get_cpu_info(void)
{
	struct processor *this_processor = get_processor_info();
	return(this_processor->cpu);
}

#define get_component_address(this_processor,component)                            \
(                                                                                  \
        (((struct processor *)(this_processor))->io_base)                       +  \
        (((struct processor *)(this_processor))->component_base->component)        \
)

#define component_available(this_processor,component) \
        (((struct processor *)(this_processor))->component_base->component)

#define get_component_physaddr(this_processor,component) \
	virt_to_phys((void *)get_component_address(this_processor,component))

#define cpu_read_reg(base, offset) (__be32_to_cpu((base)[(offset)]))
#define cpu_write_reg(base, offset, value) ((base)[(offset)] = __cpu_to_be32((value)))

extern void (*_check_bist)(struct processor *this_processor);
#define check_bist(this_processor) _check_bist(this_processor)

extern void (*_setup_tlb)(unsigned long virt, unsigned long long phys, int size);
#define setup_tlb(virt,phys,size) _setup_tlb(virt,phys,size)

extern void (*_setup_tlb_uncached)(unsigned long virt, unsigned long long phys, int size);
#define setup_tlb_uncached(virt,phys,size) _setup_tlb_uncached(virt,phys,size)

extern void (*_flush_tlb_all)(void);
#define flush_tlb_all _flush_tlb_all

extern void (*_flush_dcache_all)(void);
#define flush_dcache_all _flush_dcache_all

extern void (*_flush_icache_all)(void);
#define flush_icache_all _flush_icache_all

#endif

