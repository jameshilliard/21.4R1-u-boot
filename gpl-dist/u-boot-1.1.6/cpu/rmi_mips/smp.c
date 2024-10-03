#include "linux/types.h"
#include "asm/addrspace.h"
#include "common.h"
#include "environment.h"
#include "rmi/shared_structs.h"
#include "configs/rmi_boards.h"
//#include "flash.h"
#include <malloc.h>
#include "rmi/board.h"
#include "rmi/physaddrspace.h"
#include "rmi/bridge.h"
#include "rmi/pioctrl.h"
#include "rmi/cpld.h"
#include "rmi/flashdev.h"
#include "rmi/cfiflash.h"
#include "rmi/gpio.h"
#include "rmi/mips-exts.h"
#include "rmi/smp.h"
#include "rmi/xlr_cpu.h"
#include "rmi/threads.h"
#include "rmi/msgring.h"
#include "rmi/utils.h"
#include "rmi/pic.h"
#include "rmi/cpu_ipc.h"

extern struct boot1_info boot_info;

uint32_t cpu_online_map;
unsigned int num_cpus_online = 0;
unsigned int num_cores_online = 0;


static uint32_t cpu_online_map_temp;

static spinlock_t cpu_online_map_lock;
static atomic_t smp_init_done;
static atomic_t message_ring_config_init;
static atomic_t message_ring_config_done;
struct thread_info threads_info[NR_CPUS];

struct rmi_cpu_wakeup_info cpu_wakeup_info[NR_CPUS];

#define WAKEUP_IPI 52

void park(int thr_id)
{
	int cpu;
	smp_call func;
	void *arg0;
	struct rmi_cpu_wakeup_info *info;

	while(1) {
		write_phnx_eimr((1ULL << WAKEUP_IPI));
		cpu_wait();
		cpu = processor_id();
		info = &cpu_wakeup_info[cpu];
		if(cpu_wakeup_info[cpu].valid == 0)
			continue;
		/* Run(or invoke) the function specified */
		arg0 = info->arg0;
		func = info->func;

		__asm__ volatile(
				".set noreorder\n"
				"move $4, %0\n"
				"move $8, %1\n"
				"jalr	$8 \n"
				"nop\n"
				"nop\n"
				".set reorder\n"
				::"r"(arg0), "r"(func)
				:"$4","$8"
				);
	}
}

void rmi_send_nmi(int cpu)
{
	uint32_t pid, tid;
	uint32_t val;
	int ipi = WAKEUP_IPI;

	phoenix_reg_t *mmio = phoenix_io_mmio(PHOENIX_IO_PIC_OFFSET);
	pid = cpu >> 2;
	tid = cpu & 0x3;
	val = (pid << 20) | (tid << 16) | (1 << 8) | ipi;
	phoenix_write_reg(mmio, PIC_IPI, val);
}


void rmi_wakeup_secondary_cpus(smp_call func, void *arg, uint32_t mask)
{
	uint32_t i;
	struct rmi_cpu_wakeup_info *info;

	for(i=0; i < NR_CPUS; i++) {
		if(((1 << i) & mask) == 0)
			continue;

		info = &cpu_wakeup_info[i];
		info->func = func; 
		info->arg0 = arg; 
		info->valid = 1;

		__asm__ __volatile__("sync": : :"memory");
		rmi_send_nmi(i);
	}
}

void wakeup_cpu_msgring(int cpu, unsigned long fn, unsigned long args)
{
	struct msgrng_msg msg;

	msg.msg0 = PTR2U64((char *)&threads_info[cpu] + THREAD_SIZE - 32);
	msg.msg1 = PTR2U64(&threads_info[cpu]);
	msg.msg2 = PTR2U64(fn);
	msg.msg3 = PTR2U64(args);

	__asm__ __volatile__("sync": : :"memory");
	//printf("[%s]: cpu=%d msg0=%llx, msg1=%llx, msg2=%llx, msg3=%llx\n", __FUNCTION__, cpu
	// msg.msg0, msg.msg1, msg.msg2, msg.msg3);
	message_send_block(4, MSGRNG_CODE_BOOT_WAKEUP,
			((cpu >> 2) << 3) | (cpu & 0x03), &msg);

}


static void smp_init_secondary(void)
{
	int id = processor_id();
	int thr = phnx_thr_id();
	uint32_t lsu=0,tmp=0;
	

	/* tell the master cpu that this cpu is online */
	smp_lock(&cpu_online_map_lock);
	cpu_online_map_temp |= (1 << id);
	smp_unlock(&cpu_online_map_lock);

	rmi_cpu_arch_init();

	if (thr == 0) {
		while (readAtomic(&message_ring_config_init) == 0) ;
		message_ring_cpu_init();

		if(is_xlr_c_series() || chip_processor_id_xls()){
			/*We found C series or XLS  - Enable Unaligned access*/
			__asm__ volatile(
					".set push\n"
					"li  %0, 0x300\n"
					"mfcr    %1, %0\n"
					"ori %1,%1,0x4000\n"
					"mtcr %1,%0\n"
					".set pop\n"
					:"=r"(lsu),"=r"(tmp)/*O/P*/
					);
		}

		addAtomic(1, &message_ring_config_done);        
	}

	/* Wait till master says yes */
	while (readAtomic(&smp_init_done) == 0) ;

	park(thr);
}
void smp_callee_function(void (*fn) (void *, void *), void *args)
{
	int thr = phnx_thr_id();

	/* validation test suite sends gp reg as an arguments and expects
	 * gp in a1. oh well,...
	 */
	fn(args, args);

	park(thr);
}


void rmi_smp_init(void)
{
	int cpu = 0;
	int i = 0, j = 0;
	uint32_t lsu=0,tmp=0;

#ifdef PHOENIX_SIM
	int max_wait_time = 1000;
#else
	int max_wait_time = 0x10000000;
#endif

	uint32_t boot1_cpu_online_map = (uint32_t) boot_info.cpu_online_map;
	int num_cpus = num_ones(boot1_cpu_online_map);

	cpu_online_map_temp = 0x01;
	smp_lock_init(&cpu_online_map_lock);
	setAtomic(&smp_init_done, 0);
	setAtomic(&message_ring_config_init, 0);
	setAtomic(&message_ring_config_done, 1);

	__asm__ __volatile__("sync": : :"memory");

	if(is_xlr_c_series() || chip_processor_id_xls()){
		debug("Enabling Unaligned Access on %s revisions\n",
				is_xlr_c_series()? "C*": "XLS");
		__asm__ volatile(
				".set push\n"
				"li   %0, 0x300\n"
				"mfcr %1, %0\n"
				"ori  %1,%1,0x4000\n"
				"mtcr %1,%0\n"
				"nop\n"
				".set pop\n"
				:"=r"(lsu),"=r"(tmp)/*O/P*/
				);
	}

	for (cpu = 0; cpu < NR_CPUS ; cpu++) {
		if (processor_id() != cpu
				&& (boot1_cpu_online_map & (1 << cpu))) {
			wakeup_cpu_msgring(cpu, 
					(unsigned long)&smp_init_secondary, 0);
		}
	}

	/* Wait for some time for the last cpu to get msg.
	 * Around ~20 core clock cycles for a message to get 
	 * to it's destination
	 */
	for (i = 0; i < 2000; i++);

	/* Wait for Slave cpu(s) to boot */
	for (j = 0; j < 8; j++) {
		for (i = 0; i < max_wait_time; i++);
		cpu_online_map = cpu_online_map_temp;
		num_cpus_online = 0;
		num_cores_online = 0;

		for (i = 0; i < NR_CPUS; i++) {
			if (cpu_online_map & (1 << i)) {
				num_cpus_online++;
				if ((i & 0x03) == 0)
					num_cores_online++;
			}
		}
		if (num_cpus_online == num_cpus)
			break;
		if (num_cpus_online != num_cpus) {
#ifndef CONFIG_FX
			printf("Only %d cpus out of %d came online...\n",
					num_cpus_online, num_cpus);
#endif
		}
	}

	if (num_cpus_online != num_cpus)
#ifndef CONFIG_FX
		printf("Only %d cpu(s) out of %d came online...\n",
				num_cpus_online, num_cpus);
#endif

	debug("Detected %d online CPU(s), map = 0x%x\n", num_cpus_online,
		  cpu_online_map);

	/* Let the cpus configure the message ring */
	setAtomic(&message_ring_config_init, 1);
	__asm__ __volatile__("sync": : :"memory");

	/* wait for message ring config to be done */
	while (readAtomic(&message_ring_config_done) < num_cores_online) ;

	/* Announce SMP boot done */
	setAtomic(&smp_init_done, 1);
	debug("All cpus successfully started\n");
}
