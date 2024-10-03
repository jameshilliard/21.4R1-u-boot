#include "configs/rmi_boards.h"
#include "rmi/types.h"
#include "rmi/shared_structs.h"
#include "rmi/on_chip.h"
#include "rmi/gpio.h"
#include "rmi/xlr_cpu.h"
#include "command.h"
#include "asm/addrspace.h"
#include "common.h"
#include "rmi/mips-exts.h"
#include "rmi/mipscopreg.h"
#include "rmi/cache.h"
#include "rmi/interrupt.h"
#include "rmi/bridge.h"
#include "rmi/cpu_ipc.h"
#include "rmi/smp.h"
#include "rmi/board_dep.h"

unsigned char __stack_start[4096];

struct boot1_info boot_info __attribute__((aligned(8))) ;

uint64_t rmi_cpu_frequency;
static unsigned int config1;

static unsigned int icache_size;
static unsigned int dcache_size;

static unsigned int icache_line_size;
static unsigned int dcache_line_size;

static unsigned int icache_index_mask;

static unsigned int icache_assoc;
static unsigned int dcache_assoc;

static unsigned int icache_sets;
static unsigned int dcache_sets;


unsigned long phoenix_io_base = (unsigned long)DEFAULT_PHOENIX_IO_BASE;
extern void rmi_nmi_handler(void);

void cpu_wait(void)
{
	__asm__(
			".set push\n\t"
			".set\tmips3\n\t"
			".set noreorder\n\t"
			"c0 0x60\n\t"
			".set pop\n\t"
	       );
}


int do_reset (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	phoenix_reg_t *mmio = phoenix_io_mmio(PHOENIX_IO_GPIO_OFFSET);

	/* trigger a chip reset */
#ifdef CONFIG_FX
        /*
	 * For FX Platforms all the reboots should be through the
	 * CPLD in order to be able to detect it correctly as a
	 * Software Reset
	 */
        fx_cpld_issue_sw_reset();
        printf("\nFATAL: CPLD could not issue a RESET!!!!\n");
#endif
	phoenix_write_reg(mmio, GPIO_SWRESET_REG, 1);
	for(;;) cpu_wait();

}

void enable_interrupts(void)
{
}

int disable_interrupts(void)
{
	return 0;
}

static uint32_t xls_get_cpu_clk(uint32_t cfg)
{
	int adivq, adivf;
	uint32_t ref = 6667; /* 66.67 actually */
	uint32_t res;

	/* REF/2 * DIVF / DIVQ = PLLOUT */
	adivf = (cfg & 0xff) + 1;
	adivq = (cfg >> 8) & 0x7 ;
	adivq = (1 << adivq); /* adivq = 2 ^ divq */

	//res = (((ref / 2)/100) * (adivf/adivq));
	res = (((ref)) * (adivf/adivq))/200;
	return (uint32_t)res;
}


/* NOTE: DON'T DO PRINTF HERE */
void rmi_get_cpu_clk(void)
{
	phoenix_reg_t *mmio = phoenix_io_mmio(PHOENIX_IO_GPIO_OFFSET);
	uint32_t freq;
	uint64_t cfg;


	cfg = phoenix_read_reg(mmio, GPIO_RESET_CFG);

	if(chip_processor_id_xls())
		freq = xls_get_cpu_clk(cfg);
	else {
		freq = ((((cfg >> 2) & 0x7f) + 1) * 16667ULL) / 1000;

		if (cfg & 0x200)
			freq = freq >> 1;
	}
	if (!freq) {
		rmi_cpu_frequency = 1000 * 1000 * 1000;
	} else {
		rmi_cpu_frequency = freq * 1000 * 1000;
	}
}

void flush_cache (ulong start_addr, ulong size)
{

}
static void cpu_init(void)
{
	phoenix_reg_t *mmio;
	mmio = phoenix_io_mmio(PHOENIX_IO_BRIDGE_OFFSET);
	debug("Bridge Device Mask: [0x%x]\n", cpu_read_reg(mmio,DEVICE_MASK));
	mmio = phoenix_io_mmio(PHOENIX_IO_GPIO_OFFSET);
	debug("GPIO POR Config: [0x%x]\n", cpu_read_reg(mmio,GPIO_RESET_CFG));
}


void rmi_early_init(void)
{

	cpu_init();
	rmi_cpu_arch_init();

	/* Initialize message ring for CPU 0 here */
	message_ring_cpu_init();

	/* Wakeup other CPUs and get them to uboot code
	NOTE: This must be done before remapping the flash.
	 */
	rmi_smp_init();

	/* Call the board specific early init hook */
	rmi_early_board_init();
	
}

void rmi_late_board_init(void);
int misc_init_r(void)
{
	volatile void *ptr = (void *)(long)0xbfc00000;

	rmi_late_board_init();
	/* Copy the NMI handler here 
	 * Assume that flash is remapped by this time
	 */
	memcpy((void *)ptr, &rmi_nmi_handler, 0x80);

	return 0;
}



static void per_core_init(void)
{
	uint64_t value = 0;
	if(phnx_thr_id() == 0)
	{
		/* Enable weak ordering: TODO use macros for the value */
		value = read_phnx_ctrl(0x300);
		write_phnx_ctrl(0x300, (value & ~0x0f00));
		
		/* Read the cache configuration */
		config1 = read_mips32_cp0_config1();

		icache_line_size = 1 << (((config1 >> 19) & 0x7) + 1);
		dcache_line_size = 1 << (((config1 >> 10) & 0x7) + 1);

		icache_sets = 1 << (((config1 >> 22) & 0x7) + 6);
		dcache_sets = 1 << (((config1 >> 13) & 0x7) + 6);
		
		icache_assoc = ((config1 >> 16) & 0x7) + 1;
		dcache_assoc = ((config1 >> 7) & 0x7) + 1;
		
		icache_size = icache_line_size * icache_sets * icache_assoc;
		dcache_size = dcache_line_size * dcache_sets * dcache_assoc;
		
		icache_index_mask = (icache_sets - 1) * icache_line_size;
	}
}

static void per_cpu_init(void)
{
	int entry = 0;
	long addr = 0;
	long inc = 1 << 24;	/* 16MB */
	unsigned int config1 = 0;
	int tlbsize;

	/* TLB registers */
	write_32bit_cp0_register(COP_0_PAGEMASK, 0);
	write_32bit_cp0_register(COP_0_WIRED, 0);

	/* Status & Cause */
	write_32bit_cp0_register(COP_0_STATUS, 
			STAT_0_KX | STAT_0_CU0 | STAT_0_CU2);
	write_32bit_cp0_register(COP_0_CAUSE, 0);

	/* Count Compare */
	//write_32bit_cp0_register(COP_0_COUNT, 1);
	write_32bit_cp0_register(COP_0_COMPARE, 0);

	config1 = read_mips32_cp0_config1();
	tlbsize = ((config1 >> 25) & 0x3f) + 1;

	/* Save old context and create impossible VPN2 value */
	set_cop_entrylo_0_reg(0);
	set_cop_entrylo_1_reg(0);
	for (entry = 0; entry < tlbsize; entry++) {
		do {
			addr += inc;
			set_cop_entryhi_reg(addr);
			mips_tlb_probe();
		} while ((int)(get_cop_index_reg()) >= 0);
		set_cop_index_reg(entry);
		mips_tlb_write_indexed();
	}

	write_32bit_cp0_register(COP_0_WIRED, 0);
}

void rmi_cpu_arch_init(void)
{
	per_cpu_init();
	per_core_init();
}

void rmi_flush_icache_all(void)
{
	int i = 0;
	int icache_lines = icache_sets * icache_assoc;
	unsigned long base = (unsigned long)KSEG0;

	for (i = 0; i < icache_lines; i++) {
		cache_op(Index_Invalidate_I, base);
		base += icache_line_size;
	}
}

void rmi_flush_dcache_all(void)
{
	int i = 0;
	int dcache_lines = dcache_sets * dcache_assoc;
	unsigned long base = (unsigned long)KSEG0;

	for (i = 0; i < dcache_lines; i++) {
		cache_op(Index_Writeback_Inv_D, base);
		base += dcache_line_size;
	}
}


void rmi_flush_tlb_all(void)
{
#ifndef PHOENIX_SIM
	unsigned long flags = 0;
	unsigned long old_ctx = 0;
	unsigned int entry = 0;
	unsigned int tlbsize=10;

	unsigned int config1=0;

	config1 = read_mips32_cp0_config1();
	tlbsize = ((config1 >> 25) & 0x3f) + 1;

	cpu_save_and_disable_int(flags);

	/* Save old context and create impossible VPN2 value */
	old_ctx = (get_cop_entryhi_reg() & 0xff);

	set_cop_entrylo_0_reg(0);
	set_cop_entrylo_1_reg(0);
	for (entry = 0; entry < tlbsize; entry++)
	{
		set_cop_entryhi_reg((unsigned long)KSEG0 + (PAGE_SIZE << 1) * entry);
		set_cop_index_reg(entry);
		mips_tlb_write_indexed();
	}
	set_cop_entryhi_reg(old_ctx);

	cpu_restore_int(flags);
#endif
}

int dcache_status (void)
{
    return 0;
}

void dcache_disable (void)
{
    return;
}
