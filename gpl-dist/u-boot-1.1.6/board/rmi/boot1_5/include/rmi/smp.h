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
#ifndef __SMP_H__
#define __SMP_H__

#include "rmi/types.h"
#define NR_CPUS 32

#ifndef __ASSEMBLY__
#include "rmi/rmi_processor.h"

typedef volatile int atomic_t;
typedef	volatile unsigned int spinlock_t;

typedef void (*smp_call) (void *);

struct rmi_cpu_wakeup_info {
	int            valid;
	smp_call	func;
	void		*arg0;
};


#define readAtomic(v)	(*(v))
#define setAtomic(v,i)	(*(v) = (i))

static __inline__ void addAtomic(int i, atomic_t * v)
{
	unsigned long temp;

	__asm__ __volatile__(
		"1:   ll      %0, %1            \n"
		"     addu    %0, %2            \n"
		"     sc      %0, %1            \n"
		"     beqz    %0, 1b            \n"
		"     nop                       \n"
		"     sync                       \n"
		:"=&r" (temp), "=m"(*v)
		:"Ir"(i), "m"(*v));
}

static __inline__ void subAtomic(int i, atomic_t * v)
{
	unsigned long temp;

	__asm__ __volatile__(
		"1:   ll      %0, %1           \n"
		"     subu    %0, %2           \n"
		"     sc      %0, %1           \n"
		"     beqz    %0, 1b           \n"
		"     nop                      \n"
		"     sync                      \n"
		:"=&r"(temp), "=m"(*v)
		:"Ir"(i), "m"(*v));
}


static __inline__ void smp_lock_init(spinlock_t * lp)
{
	__asm__ __volatile__(".set noreorder    \n"
			     "    sync          \n"
			     "    sw $0, %0     \n"
			     ".set\treorder     \n"
			     :"=m"(*lp)
			     :"m"(*lp)
			     :"memory");
}

static __inline__ void smp_lock(spinlock_t * lp)
{
	unsigned int tmp;

	__asm__ __volatile__(".set noreorder    \n"
			     "1: ll    %1, %2   \n"
			     "   bnez  %1, 1b   \n"
			     "   nop            \n"
			     "   li    %1, 1    \n"
			     "   sc    %1, %0   \n"
			     "   beqz  %1, 1b   \n"
			     "   nop            \n"
			     "   sync           \n"
			     ".set reorder      \n"
			     :"=m"(*lp), "=&r"(tmp)
			     :"m"(*lp)
			     :"memory");
}
#define smp_unlock(x) smp_lock_init(x)

extern uint32_t num_cpus_online;
extern uint32_t cpu_online_map;

void rmi_wakeup_secondary_cpus(smp_call fn, void *args, uint32_t cpu_mask);
void rmi_smp_init(void);

#endif

#endif
