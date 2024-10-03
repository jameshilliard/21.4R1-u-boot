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
/* XLR CPU specific */

#ifndef _CPU_H
#define _CPU_H

#include "rmi/mips-exts.h"
#include "rmi/on_chip.h"

#define CPU_BLOCKID_IFU      0
#define CPU_BLOCKID_ICU      1
#define CPU_BLOCKID_IEU      2
#define CPU_BLOCKID_LSU      3
#define CPU_BLOCKID_MMU      4
#define CPU_BLOCKID_PRF      5

#define IFU_THR_EN_REGID             0
#define IFU_THR_SLEEP_REGID          1
#define IFU_THR_SCHED_POLICY_REGID   2
#define IFU_THR_SCHED_COUNTER_REGID  3
#define IFU_BHR_PROG_MASK_REGID      4
#define IFU_DISABLE_SB_SLEEP_REGID   5
#define IFU_DEFEATURE_REGID          6
#define IFU_SLEEP_STATE_REGID        16
#define IFU_BHR_REGID                17
#define IFU_IPG_0_REGID              20
#define IFU_IPG_1_REGID              21
#define IFU_IPG_2_REGID              22
#define IFU_IPG_3_REGID              23
#define IFU_RAS_0_REGID              24
#define IFU_RAS_1_REGID              25
#define IFU_RAS_2_REGID              26
#define IFU_RAS_3_REGID              27
#define IFU_RAS_4_REGID              28
#define IFU_RAS_5_REGID              29
#define IFU_RAS_6_REGID              30
#define IFU_RAS_7_REGID              31
#define IFU_BHT_0_31_REGID           64
#define IFU_BHT_2016_2047_REGID      127

#define LSU_CFG0_REGID       0
#define LSU_CFG1_REGID       1
#define LSU_CFG2_REGID       2
#define LSU_CFG3_REGID       3
#define LSU_CFG4_REGID       4
#define LSU_STAT_REGID       5
#define LSU_DEF_REGID        6
#define LSU_DBG0_REGID       7
#define LSU_DBG1_REGID       8
#define LSU_CERRLOG_REGID    9
#define LSU_CERROVF_REGID    10
#define LSU_CERRINT_REGID    11

#define IEU_DEFEATURE_REGID 0

#define ICU_DEFEATURE_REGID 0

#define MMU_SETUP_REGID 0

#define CHIP_PROCESSOR_ID_XLR       0x00
#define CHIP_PROCESSOR_ID_XLS_608   0x80  
#define CHIP_PROCESSOR_ID_XLS_408   0x88
#define CHIP_PROCESSOR_ID_XLS_404   0x8c
#define CHIP_PROCESSOR_ID_XLS_208   0x8e
#define CHIP_PROCESSOR_ID_XLS_204   0x8f
#define CHIP_PROCESSOR_ID_XLS_108   0xce
#define CHIP_PROCESSOR_ID_XLS_104   0xcf

/* Defines for 'XLS B0' */
#define CHIP_PID_XLS_616_B0   0x40
#define CHIP_PID_XLS_608_B0   0x4a
#define CHIP_PID_XLS_612_B0   0x48
#define CHIP_PID_XLS_416_B0   0x44
#define CHIP_PID_XLS_412_B0   0x4c
#define CHIP_PID_XLS_408_B0   0x4e
#define CHIP_PID_XLS_404_B0   0x4f

#ifndef __ASSEMBLY__

#define read_64bit_phnx_ctrl_reg(block, reg)                    \
({ unsigned long long __res;                                    \
        __asm__ __volatile__(                                   \
	".set\tpush\n\t"					\
	".set\tnoreorder\n\t"					\
        "mfcr\t%0, %1\n\t"                                      \
	".set\tpop"						\
        : "=r" (__res) : "r"((block<<8)|reg));                  \
        __res;})

#define write_64bit_phnx_ctrl_reg(block, reg, value)            \
        __asm__ __volatile__(                                   \
        "mtcr\t%0, %1\n\t"				        \
        : : "r" (value), "r"((block<<8)|reg)  );

static inline void cpu_enable_l2_cache(void)
{
	uint64_t value = 0;

	value = read_phnx_ctrl((CPU_BLOCKID_LSU << 8) | LSU_CFG0_REGID);
	write_phnx_ctrl(((CPU_BLOCKID_LSU << 8) | LSU_CFG0_REGID),
			(value | 0x10));
}

static inline void cpu_disable_l2_cache(void)
{
	uint64_t value = 0;

	value = read_phnx_ctrl((CPU_BLOCKID_LSU << 8) | LSU_CFG0_REGID);
	write_phnx_ctrl(((CPU_BLOCKID_LSU << 8) | LSU_CFG0_REGID),
			(value & ~0x10));
}



static __inline__ int chip_id(void)
{
	unsigned int chip = 0;

	__asm__ volatile (".set push\n"
			  ".set mips64\n"
			  "mfc0 %0, $15\n" ".set pop\n":"=r" (chip)
	    );
	return (chip);
}

static inline unsigned int xlr_revision(void)
{
    return chip_id() & 0xff00ff;
}

static __inline__ int chip_company_id(void)
{
	return ((chip_id()>>15) & 0xff );
}

static __inline__ int chip_processor_id(void)
{
    return ((chip_id()>>8) & 0xff);
}

/* Returns '1' if the processor id belongs
 * to any one of the family of XLS Processors;
 * viz. 608/408/404/208/204, else '0'.
 */
static __inline__ int chip_processor_id_xls(void) {

    int processor_id = ((chip_id()>>8) & 0xff);
    switch (processor_id) {

        case CHIP_PROCESSOR_ID_XLS_608:
        case CHIP_PROCESSOR_ID_XLS_408:
        case CHIP_PROCESSOR_ID_XLS_404:
        case CHIP_PROCESSOR_ID_XLS_208:
        case CHIP_PROCESSOR_ID_XLS_204:
        case CHIP_PROCESSOR_ID_XLS_108:
        case CHIP_PROCESSOR_ID_XLS_104:
        case CHIP_PID_XLS_616_B0:
        case CHIP_PID_XLS_608_B0:
        case CHIP_PID_XLS_612_B0:
        case CHIP_PID_XLS_416_B0:
        case CHIP_PID_XLS_412_B0:
        case CHIP_PID_XLS_408_B0:
        case CHIP_PID_XLS_404_B0:
            return 1;
        default:
            return 0;
    }
}

static __inline__ int chip_processor_id_xls4xx(void) {

    int processor_id = ((chip_id()>>8) & 0xff);
    switch (processor_id) {
        case CHIP_PROCESSOR_ID_XLS_408:
        case CHIP_PROCESSOR_ID_XLS_404:
            return 1;
        default:
            return 0;
    }
}

static __inline__ int chip_processor_id_xls2xx(void) {

    int processor_id = ((chip_id()>>8) & 0xff);
    switch (processor_id) {
        case CHIP_PROCESSOR_ID_XLS_208:
        case CHIP_PROCESSOR_ID_XLS_204:
            return 1;
        default:
            return 0;
    }
}

static __inline__ int chip_processor_id_xls1xx(void) {

    int processor_id = ((chip_id()>>8) & 0xff);
    switch (processor_id) {
        case CHIP_PROCESSOR_ID_XLS_108:
        case CHIP_PROCESSOR_ID_XLS_104:
            return 1;
        default:
            return 0;
    }
}

/* The B0 Stepping of the XLS family... */
static __inline__ int chip_pid_xls_b0(void) {

    int processor_id = ((chip_id()>>8) & 0xff);
    switch (processor_id) {
        case CHIP_PID_XLS_616_B0:
        case CHIP_PID_XLS_608_B0:
        case CHIP_PID_XLS_612_B0:  
        case CHIP_PID_XLS_416_B0:
        case CHIP_PID_XLS_412_B0:
        case CHIP_PID_XLS_408_B0:
        case CHIP_PID_XLS_404_B0:
            return 1;
        default:
            return 0;
    }
}
        


static __inline__ int chip_revision_id(void)
{
	return (chip_id() & 0xff );
}

static __inline__ int is_xlr_c_series(void)
{
    int processor_id = chip_processor_id();
    int revision_id  = chip_revision_id();
    int retval=0;

    switch (processor_id) {
        /* These are the relevant PIDs for XLR
         * steppings (hawk and above). For these,
         * PIDs, Rev-Ids of [5-9] indicate 'C'.
         */
        case 0x00: 
        case 0x02:
        case 0x07: 
        case 0x08:
        case 0x0A: 
        case 0x0B:
            {
                if (revision_id>=5 && revision_id<=9) {
                    retval=1;
                }
                else {
                    retval=0;
                }
            }
            break;
        default:
            retval=0;
    }
    return retval;
}
void rmi_cpu_arch_init(void);
void rmi_flush_icache_all(void);
void rmi_flush_dcache_all(void);
void rmi_flush_tlb_all(void);
void cpu_wait(void);


#endif				// __ASSEMBLY__
#endif
