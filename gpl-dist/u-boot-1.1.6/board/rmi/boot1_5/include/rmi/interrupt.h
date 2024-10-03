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
#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

/* Defines for the IRQ numbers */

#define IRQ_UART                 2
#define IRQ_IPI_SMP_FUNCTION     3
#define IRQ_IPI_SMP_RESCHEDULE   4
#define IRQ_REMOTE_DEBUG         5
#define IRQ_MAC                  6
#define IRQ_TIMER                7

#ifndef __ASSEMBLY__
/* The following will generate ssnop. From 'See MIPS Run 2nd Edition', pp 207*/
__asm__(".macro\tsnops\n\t"
	".set\tpush\n\t"
	".set\treorder\n\t"
	".set\tnoat\n\t"
	"sll\t$0, $0, 1\t\n"
	"sll\t$0, $0, 1\t\n"
	"sll\t$0, $0, 1\t\n"
	".set\tpop\n\t" ".endm");


static inline void cpu_disable_int(void)
{
	__asm__ volatile (".set push             \n"
			  ".set noreorder        \n"
			  ".set noat             \n"
			  "mfc0 $1, $12          \n"
			  "ori  $1, 0x1          \n"
			  "xori $1, 0x1          \n"
			  "mtc0 $1, $12          \n"
			  "snops                 \n"
			  ".set pop              \n"
		);
}

static inline void cpu_enable_int(void)
{
	__asm__ volatile (".set push             \n"
			  ".set noreorder        \n"
			  ".set noat             \n"
			  "mfc0 $1, $12          \n"
			  "ori  $1, 0x1f         \n"
			  "xori $1, 0x1e         \n"
			  "mtc0 $1, $12          \n"
			  "snops\t               \n"
			  ".set pop              \n"
		);
}

static inline void cpu_save_and_disable_int(int flags)
{
	__asm__ volatile (".set push             \n"
			  ".set noreorder        \n"
			  ".set noat             \n"
			  "mfc0 %0, $12          \n"
			  "ori  $1, %0, 0x1      \n"
			  "xori $1, 0x1          \n"
			  "mtc0 $1, $12          \n"
			  "snops                 \n"
			  ".set pop              \n"
			  : "=r" (flags)
			  :
			  : "memory"
		);
}

static inline void cpu_restore_int(unsigned long flags)
{
	unsigned long tmp;
	__asm__ volatile (".set push            \n"
			  ".set noreorder       \n"
			  ".set noat            \n"
			  "mfc0 $1,  $12        \n"
			  "ori  $1,  0x1        \n"
			  "xori $1,  0x1        \n"
			  "andi %0,    1        \n"
			  "or   %0,   $1        \n"
			  "mtc0 %0,  $12        \n"
			  "snops                \n"
			  ".set pop             \n"
			  : "=r" (tmp) 
			  : "0" (flags) 
			  : "memory"
		);
}

#endif

#endif
