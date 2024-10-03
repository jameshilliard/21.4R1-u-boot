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
#include "rmi/on_chip.h"
#include "rmi/msgring.h"
#include "rmi/types.h"
#include "rmi/printk.h"
#include "rmi/bridge.h"
#include "rmi/mips-exts.h"
#include "rmi/gpio.h"
#include "rmi/xlr_cpu.h"
#include "rmi/cpu_ipc.h"

#define MSGRNG_CC_INIT_CPU_DEST(dest,cpu) \
do { \
     msgrng_write_cc(MSGRNG_CC_##dest##_REG, cc_table_cpu_##cpu.counters[dest][0], 0 ); \
     msgrng_write_cc(MSGRNG_CC_##dest##_REG, cc_table_cpu_##cpu.counters[dest][1], 1 ); \
     msgrng_write_cc(MSGRNG_CC_##dest##_REG, cc_table_cpu_##cpu.counters[dest][2], 2 ); \
     msgrng_write_cc(MSGRNG_CC_##dest##_REG, cc_table_cpu_##cpu.counters[dest][3], 3 ); \
     msgrng_write_cc(MSGRNG_CC_##dest##_REG, cc_table_cpu_##cpu.counters[dest][4], 4 ); \
     msgrng_write_cc(MSGRNG_CC_##dest##_REG, cc_table_cpu_##cpu.counters[dest][5], 5 ); \
     msgrng_write_cc(MSGRNG_CC_##dest##_REG, cc_table_cpu_##cpu.counters[dest][6], 6 ); \
     msgrng_write_cc(MSGRNG_CC_##dest##_REG, cc_table_cpu_##cpu.counters[dest][7], 7 ); \
} while(0)

/* Initialized CC for cpu 0 to send to all buckets at 0-7 cpus */
#define MSGRNG_CC_INIT_CPU(cpu) \
do { \
  MSGRNG_CC_INIT_CPU_DEST(0,cpu); \
  MSGRNG_CC_INIT_CPU_DEST(1,cpu); \
  MSGRNG_CC_INIT_CPU_DEST(2,cpu); \
  MSGRNG_CC_INIT_CPU_DEST(3,cpu); \
  MSGRNG_CC_INIT_CPU_DEST(4,cpu); \
  MSGRNG_CC_INIT_CPU_DEST(5,cpu); \
  MSGRNG_CC_INIT_CPU_DEST(6,cpu); \
  MSGRNG_CC_INIT_CPU_DEST(7,cpu); \
  MSGRNG_CC_INIT_CPU_DEST(8,cpu); \
  MSGRNG_CC_INIT_CPU_DEST(9,cpu); \
  MSGRNG_CC_INIT_CPU_DEST(10,cpu); \
  MSGRNG_CC_INIT_CPU_DEST(11,cpu); \
  MSGRNG_CC_INIT_CPU_DEST(12,cpu); \
  MSGRNG_CC_INIT_CPU_DEST(13,cpu); \
  MSGRNG_CC_INIT_CPU_DEST(14,cpu); \
  MSGRNG_CC_INIT_CPU_DEST(15,cpu); \
} while (0)

#define MSGRNG_BUCKETSIZE_INIT_CPU(base) \
do { \
  msgrng_write_bucksize(0, bucket_sizes.bucket[base+0]); \
  msgrng_write_bucksize(1, bucket_sizes.bucket[base+1]); \
  msgrng_write_bucksize(2, bucket_sizes.bucket[base+2]); \
  msgrng_write_bucksize(3, bucket_sizes.bucket[base+3]); \
  msgrng_write_bucksize(4, bucket_sizes.bucket[base+4]); \
  msgrng_write_bucksize(5, bucket_sizes.bucket[base+5]); \
  msgrng_write_bucksize(6, bucket_sizes.bucket[base+6]); \
  msgrng_write_bucksize(7, bucket_sizes.bucket[base+7]); \
} while(0);

#define XLS_MSGRNG_CC_INIT_CPU_DEST(dest,cpu) \
do { \
     msgrng_write_cc(MSGRNG_CC_##dest##_REG, xls_cc_table_cpu_##cpu.counters[dest][0], 0 ); \
     msgrng_write_cc(MSGRNG_CC_##dest##_REG, xls_cc_table_cpu_##cpu.counters[dest][1], 1 ); \
     msgrng_write_cc(MSGRNG_CC_##dest##_REG, xls_cc_table_cpu_##cpu.counters[dest][2], 2 ); \
     msgrng_write_cc(MSGRNG_CC_##dest##_REG, xls_cc_table_cpu_##cpu.counters[dest][3], 3 ); \
     msgrng_write_cc(MSGRNG_CC_##dest##_REG, xls_cc_table_cpu_##cpu.counters[dest][4], 4 ); \
     msgrng_write_cc(MSGRNG_CC_##dest##_REG, xls_cc_table_cpu_##cpu.counters[dest][5], 5 ); \
     msgrng_write_cc(MSGRNG_CC_##dest##_REG, xls_cc_table_cpu_##cpu.counters[dest][6], 6 ); \
     msgrng_write_cc(MSGRNG_CC_##dest##_REG, xls_cc_table_cpu_##cpu.counters[dest][7], 7 ); \
} while(0)


/* Initialized CC for cpu 0 to send to all buckets at 0-7 cpus */
#define XLS_MSGRNG_CC_INIT_CPU(cpu) \
do { \
  XLS_MSGRNG_CC_INIT_CPU_DEST(0,cpu); \
  XLS_MSGRNG_CC_INIT_CPU_DEST(1,cpu); \
  XLS_MSGRNG_CC_INIT_CPU_DEST(2,cpu); \
  XLS_MSGRNG_CC_INIT_CPU_DEST(3,cpu); \
  XLS_MSGRNG_CC_INIT_CPU_DEST(4,cpu); \
  XLS_MSGRNG_CC_INIT_CPU_DEST(5,cpu); \
  XLS_MSGRNG_CC_INIT_CPU_DEST(6,cpu); \
  XLS_MSGRNG_CC_INIT_CPU_DEST(7,cpu); \
  XLS_MSGRNG_CC_INIT_CPU_DEST(8,cpu); \
  XLS_MSGRNG_CC_INIT_CPU_DEST(9,cpu); \
  XLS_MSGRNG_CC_INIT_CPU_DEST(10,cpu); \
  XLS_MSGRNG_CC_INIT_CPU_DEST(11,cpu); \
  XLS_MSGRNG_CC_INIT_CPU_DEST(12,cpu); \
  XLS_MSGRNG_CC_INIT_CPU_DEST(13,cpu); \
  XLS_MSGRNG_CC_INIT_CPU_DEST(14,cpu); \
  XLS_MSGRNG_CC_INIT_CPU_DEST(15,cpu); \
} while (0)

#define XLS_MSGRNG_BUCKETSIZE_INIT_CPU(base) \
do { \
  msgrng_write_bucksize(0, xls_bucket_sizes.bucket[base+0]); \
  msgrng_write_bucksize(1, xls_bucket_sizes.bucket[base+1]); \
  msgrng_write_bucksize(2, xls_bucket_sizes.bucket[base+2]); \
  msgrng_write_bucksize(3, xls_bucket_sizes.bucket[base+3]); \
  msgrng_write_bucksize(4, xls_bucket_sizes.bucket[base+4]); \
  msgrng_write_bucksize(5, xls_bucket_sizes.bucket[base+5]); \
  msgrng_write_bucksize(6, xls_bucket_sizes.bucket[base+6]); \
  msgrng_write_bucksize(7, xls_bucket_sizes.bucket[base+7]); \
} while(0);

void xlr_message_ring_cpu_init(void)
{
	int cpu = phnx_cpu_id();

	//printf("Initializing message ring for cpu_%d\n", cpu);

	/* update config register - interrupts disabled */
	write_32bit_cp2_register(MSGRNG_MSG_CONFIG_REG, 0);

	/* Message Stations are shared among all threads in a cpu core
	 * Assume, thread 0 on all cores are always active when more than
	 * 1 thread is active in a core
	 */
	switch (cpu) {
	case 0:{
			MSGRNG_BUCKETSIZE_INIT_CPU(0);
			MSGRNG_CC_INIT_CPU(0);
		}
		break;
	case 1:{
			MSGRNG_BUCKETSIZE_INIT_CPU(8);
			MSGRNG_CC_INIT_CPU(1);
		}
		break;
	case 2:{
			MSGRNG_BUCKETSIZE_INIT_CPU(16);
			MSGRNG_CC_INIT_CPU(2);
		}
		break;
	case 3:{
			MSGRNG_BUCKETSIZE_INIT_CPU(24);
			MSGRNG_CC_INIT_CPU(3);
		}
		break;
	case 4:{
			MSGRNG_BUCKETSIZE_INIT_CPU(32);
			MSGRNG_CC_INIT_CPU(4);
		}
		break;
	case 5:{
			MSGRNG_BUCKETSIZE_INIT_CPU(40);
			MSGRNG_CC_INIT_CPU(5);
		}
		break;
	case 6:{
			MSGRNG_BUCKETSIZE_INIT_CPU(48);
			MSGRNG_CC_INIT_CPU(6);
		}
		break;
	case 7:{
			MSGRNG_BUCKETSIZE_INIT_CPU(56);
			MSGRNG_CC_INIT_CPU(7);
		}
		break;

	default:
		break;
	}
}

void xls_message_ring_cpu_init(void)
{
	int cpu = phnx_cpu_id();


	//printf("Initializing message ring for cpu_%d\n", cpu);
	/* update config register - interrupts disabled */
	write_32bit_cp2_register(MSGRNG_MSG_CONFIG_REG, 0);

	/* Message Stations are shared among all threads in a cpu core
	 * Assume, thread 0 on all cores are always active when more than
	 * 1 thread is active in a core
	 */
	switch (cpu) {
		case 0:
			XLS_MSGRNG_BUCKETSIZE_INIT_CPU(0);
			XLS_MSGRNG_CC_INIT_CPU(0);
			break;
		case 1:
			XLS_MSGRNG_BUCKETSIZE_INIT_CPU(8);
			XLS_MSGRNG_CC_INIT_CPU(1);
			break;
		case 2:
			XLS_MSGRNG_BUCKETSIZE_INIT_CPU(16);
			XLS_MSGRNG_CC_INIT_CPU(2);
			break;
		case 3:
			XLS_MSGRNG_BUCKETSIZE_INIT_CPU(24);
			XLS_MSGRNG_CC_INIT_CPU(3);
			break;

		default:
		       //printf("[%s]: unexpected cpu=%d initialization!\n", __FUNCTION__, cpu);
			break;
	}
	return;

}

void message_ring_cpu_init(void)
{
    int pr_id = chip_processor_id_xls();

    /* Message Stations are shared among all threads in a cpu core
     * Assume, thread 0 on all cores are always active when more than
     * 1 thread is active in a core
     */
    /* XLR chips do not have a consistent pr_ids. So assuming all to 
       have pr_ids of 0 for now
     */

    if (pr_id != 1) {
        pr_id = 0;   
    }  
    
    if (pr_id==1) {
        /* XLS */
        xls_message_ring_cpu_init();
    }
    else {
        /* XLR */
        xlr_message_ring_cpu_init();
    }
}

void drain_messages(void)
{
	int i = 0, j = 0;
	const int num_retries = 8;
	int bucket = phnx_thr_id();
	struct msgrng_msg msg;
	int size = 0, code = 0, stid = 0;

	/* wait and drain all messages */
	/* for now just drain everything */
	for (i = 0; i < num_retries; i++) {
		/* drain all messages */
		while (!message_receive(bucket, &size, &code, &stid, &msg)) ;
		for (j = 0; j < 10000; j++) ;
	}
}


