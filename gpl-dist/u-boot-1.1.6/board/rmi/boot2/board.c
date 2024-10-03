#include "linux/types.h"
#include "asm/addrspace.h"
#include "common.h"
#include "environment.h"
#include "command.h"
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
#include "rmi/mipscopreg.h"
#include "rmi/xlr_cpu.h"
#include "rmi/cpu_ipc.h"
#include <net.h>
#define NUM_DRAM_BARS 7


extern struct boot1_info boot_info;
extern uint64_t rmi_cpu_frequency;
extern unsigned long uboot_end;
extern int usb_stor_curr_dev;


/* global structs for processor and board */
struct rmi_processor_info rmi_processor;
struct rmi_board_info rmi_board;


struct psb_mem_map board_phys_map;


int checkboard(void)
{
	debug("RMI Board: Major %d Minor %d\n", 
		  boot_info.board_major_version, boot_info.board_minor_version);
	printf("CPU Frequency = %dHz\n", rmi_cpu_frequency);
	debug("Online CPU mask 0x%08x\n", boot_info.cpu_online_map);


	return 0;
}

int rmi_add_phys_region(uint64_t start, uint64_t size, uint32_t type)
{
	int i = board_phys_map.nr_map;

	if(i == PSB_MEM_MAP_MAX)
		return -1;
	
	/* add the entry in the end */
	board_phys_map.map[i].addr = start;
	board_phys_map.map[i].size = size;
	board_phys_map.map[i].type = type;

	board_phys_map.nr_map++;

	return 0;
}

static void rmi_get_phys_mem_map(void)
{
	int i;
	uint64_t base, size, mask, bar;
	phoenix_reg_t *mmio = phoenix_io_mmio(PHOENIX_IO_BRIDGE_OFFSET);

	/* Read the Bridge DRAM bars and create the physical memory map
	 * Note: boot1 maps only DRAM. Flash is a fixed mapping at reset
	 */
	for (i=0; i<NUM_DRAM_BARS; i++) {
		bar = phoenix_read_reg(mmio,DRAM_BAR0+i);
		if (!(bar & 0x01))
			continue;
		base = ((bar >> 16) & 0xffff) << 24;
		mask = (((bar >> 4) & 0xfff) << 24) | 0xffffff;
		base = base & ~mask;
		size = mask + 1;
		rmi_add_phys_region(base, size, BOOT_MEM_RAM);
	}


}

static void rmi_sort_merge_map(void)
{
	uint32_t i, j;
	uint64_t temp1, temp2;
	uint32_t map = board_phys_map.nr_map;
	struct psb_mem_map phys_map, temp;


	/* Sort first */
	for(i=0; i < board_phys_map.nr_map; i++) {
		for(j=i+1; j < board_phys_map.nr_map;j++) {
			temp1 = board_phys_map.map[j].addr;
			temp2 =  board_phys_map.map[i].addr;
			if(temp1 > temp2) 
				continue;
			/* Swap the entries */
			temp.map[0] = board_phys_map.map[i];
			board_phys_map.map[i] = board_phys_map.map[j];
			board_phys_map.map[j] = temp.map[0];
		}
	}

	for(i=0; i <  (board_phys_map.nr_map -1); i++) {
		if(board_phys_map.map[i].type != board_phys_map.map[i+1].type)
			continue;

		temp1 = board_phys_map.map[i].addr;
		temp2 = board_phys_map.map[i].size;

		if((temp1 + temp2) == board_phys_map.map[i+1].addr){
			/* Merge i to i+1 */
			board_phys_map.map[i+1].addr = temp1;
			board_phys_map.map[i+1].size += temp2;
			board_phys_map.map[i].type = -1;
		}
	}
	memcpy(&phys_map, &board_phys_map, sizeof(phys_map));
	
	map = board_phys_map.nr_map;
	j = 0;
	for(i=0; i < map; i++){
		/* Copy all valid entries */
		if(phys_map.map[i].type != -1){
			board_phys_map.map[j] = phys_map.map[i];
			j++;		
		}
	}
	board_phys_map.nr_map = j;
}


void rmi_print_board_map(void)
{
	int i;
	uint32_t shi, slo, ahi, alo;
	printf("Available memory on board:\n");
	for(i=0; i < board_phys_map.nr_map; i++) {
		if((board_phys_map.map[i].type != BOOT_MEM_RAM) &&
			(board_phys_map.map[i].type != BOOT_MEM_RESERVED))
			continue;
		shi = (uint32_t)(board_phys_map.map[i].size >> 32);
		slo = (uint32_t)(board_phys_map.map[i].size & 0xffffffff);
		ahi = (uint32_t)(board_phys_map.map[i].addr >> 32);
		alo = (uint32_t)(board_phys_map.map[i].addr & 0xffffffff);
		printf("0x%02x%08x @ 0x%02x%08x\n", shi, slo, ahi, alo);
	}
}

uint64_t top_avail_dram(void) 
{
	phoenix_reg_t *mmio = (phoenix_reg_t *)(DEFAULT_PHOENIX_IO_BASE +
			PHOENIX_IO_BRIDGE_OFFSET);
	uint64_t base, size, mask, bar, tempVal=0;
	int i=0;

	base = size = mask = bar = 0;

	for (i = 0; i < 7 /*NUM_DRAM_BARS*/; i++) {
		bar = cpu_read_reg(mmio,DRAM_BAR0+i);
		if (!(bar & 0x01))
			continue;
		base = ((bar >> 16) & 0xffff) << 24;
		mask = (((bar >> 4) & 0xfff) << 24) | 0xffffff;
		base = base & ~mask;
		size = mask + 1;

		if ((base+size-1) > tempVal)
			tempVal = (base+size-1);
	}
	return tempVal;
}

void rmi_remap_nmi_region(void)
{
	phoenix_reg_t  *my_mmio= NULL;
	uint32_t phnx_dram_bar_data;
	uint32_t phnx_dram_mtr_data;
	uint32_t xleave;
	uint32_t pos_adjust;

	uint64_t base = MB(496);
	uint64_t size = MB(16);

	uint64_t tempVal=0;

	my_mmio = phoenix_io_mmio(PHOENIX_IO_BRIDGE_OFFSET);

	/* Figure out the prevailing interleaving scheme by reading BAR0
	 * Simplifying assumption here is that all BARs have the same
	 * interleaving scheme
	 */
	phnx_dram_bar_data = phoenix_read_reg(my_mmio, BRIDGE_DRAM_0_BAR);
	xleave = (phnx_dram_bar_data & 0xe) >> 1;
	if (xleave < 4){
		pos_adjust = 0;
	}
	else if (xleave < 6){
		pos_adjust = 1;
	}
	else {
		pos_adjust = 2;
	}

	/* Now program BAR7 with the NMI region */
	if ((tempVal = top_avail_dram()) > 0x0fffffffULL) {
		phnx_dram_bar_data = (0x1f<< 16) | (0   << 4) | (xleave << 1) | 0x1;
		phnx_dram_mtr_data = (0x0 << 12) | (0x1 << 8) | (24 - pos_adjust);
		phoenix_write_reg(my_mmio, BRIDGE_DRAM_7_BAR,phnx_dram_bar_data );
		phoenix_write_reg(my_mmio,BRIDGE_DRAM_CHN_0_MTR_7_BAR,
				phnx_dram_mtr_data);
		phoenix_write_reg(my_mmio,BRIDGE_DRAM_CHN_1_MTR_7_BAR,
				phnx_dram_mtr_data);
		rmi_add_phys_region(base, size, BOOT_MEM_RESERVED);
	}
	return;
}

long int initdram (int board_type)
{

	/* return available kseg0 memory */
	/* Return end of u_boot code here - this is to ensure that board_init_f
	 reserves the correct place for malloc, sp etc
	 */
	/* Leave space for malloc region, stack regions used by uboot */
	return ((long int)TEXT_BASE - (long int)KSEG0 - 
			CFG_MALLOC_LEN - (2 << 20));
}


void rmi_init_boot_info(void)
{
	struct boot1_info *boot1_info = &boot_info;

	boot1_info->boot_level = 2;
	boot1_info->io_base = PTR2U64(phoenix_io_base);
	boot1_info->wakeup = PTR2U64(&rmi_wakeup_secondary_cpus);

	boot1_info->magic_dword = 
	((uint64_t) BOOT1_INFO_VERSION << 32) | ((uint64_t) 0x900dbeef);
	boot1_info->size = sizeof(struct boot1_info);
	boot1_info->global_shmem_addr = 0;
	boot1_info->userapp_cpu_map = (uint64_t)boot_info.cpu_online_map;

	boot1_info->wakeup_os = PTR2U64(&rmi_wakeup_secondary_cpus);
	boot1_info->psb_mem_map = PTR2U64(&board_phys_map);
	boot1_info->avail_memory_map = PTR2U64(&board_phys_map);
	boot1_info->psb_io_map = PTR2U64(&board_phys_map);
	boot1_info->cpu_frequency = rmi_cpu_frequency;


	/* Store the boot_info is OS scratch register 0 */
	write_64bit_cp0_register_sel(COP_0_OS_SCTACH, 
			((long)&boot_info), 0);
}

void rmi_late_board_init(void)
{
	rmi_remap_nmi_region();

	rmi_sort_merge_map();

#ifndef CONFIG_FX
	rmi_print_board_map();
#endif

#ifdef CONFIG_USB_STORAGE
#if 0
	/* start USB */
   if (usb_stop() < 0) {
      printf ("usb_stop failed\n");
      return -1;
   }
#endif
   if (usb_init() < 0) {
      printf ("usb_init failed\n");
      return -1;
   }
   /*
    * check whether a storage device is attached (assume that it's
    * a USB memory stick, since nothing else should be attached).
    */
   usb_stor_curr_dev = usb_stor_scan(1);
#endif
#ifdef CONFIG_FX
     fx_board_specific_init();
#endif
}

void rmi_usb_set_mode(void);
int rmi_early_board_init(void) 
{  
	struct rmi_processor_info * rmip = &rmi_processor;
	struct rmi_board_info * rmib = &rmi_board;
	uint32_t *gpio_base = (uint32_t *)
		(DEFAULT_PHOENIX_IO_BASE + PHOENIX_IO_GPIO_OFFSET);

	board_phys_map.nr_map = 0;
	rmi_get_phys_mem_map();

	rmip->processor_id = chip_processor_id();
	rmip->revision = chip_revision_id();
	rmip->io_base  = (unsigned long) DEFAULT_PHOENIX_IO_BASE;

       /*
        *The Major and Minor numbers are Only for the EVAL Boards
        */    
#ifdef RMI_EVAL_BOARD
	rmib->maj = boot_info.board_major_version;
	rmib->min = boot_info.board_minor_version;
#endif
	if (chip_processor_id_xls())  {
		rmib->isXls = 1;
	} else {
		rmib->isXls = 0;
	}

#ifdef CONFIG_FX
        init_board_type(rmib);
#else
	rmi_piobus_setup(rmib);

	/* some boards may require a change in Chip selects */
	rmi_tweak_pio_cs(rmib);

	rmi_init_piobus(rmib);

	rmi_init_piobus_devices(rmib);
#endif

#ifdef RMI_EVAL_BOARD
	/* check valid major & minor numbers */
	if ((rmib->maj < RMI_PHOENIX_BOARD_ARIZONA_I) || 
			(rmib->maj > RMI_PHOENIX_BOARD_ARIZONA_XII)) 
	{
		printf ("Unknown board maj:%d min:%d \n", rmib->maj, rmib->min);
		return 0;
	}

	/* update num of gmacs */
	if ((rmib->maj == RMI_PHOENIX_BOARD_ARIZONA_XI) || 
			(rmib->maj == RMI_PHOENIX_BOARD_ARIZONA_XII))
	{
		/* XLS B0 Boards dont have SGMII ports mounted */
		rmib->gmac_num = 1;
		rmib->gmac_list = 0x01;
	}
	else 
#endif
        if (rmib->isXls)
	    {
		switch (rmip->processor_id)
		{
			case CHIP_PROCESSOR_ID_XLS_608:
			case CHIP_PID_XLS_616_B0:
			case CHIP_PID_XLS_612_B0:
			case CHIP_PID_XLS_608_B0:
			case CHIP_PID_XLS_416_B0:
			case CHIP_PID_XLS_412_B0:
			case CHIP_PID_XLS_408_B0:
			case CHIP_PID_XLS_404_B0: 
				rmib->gmac_num = 8;
				rmib->gmac_list = 0xff;
				break;

			case CHIP_PROCESSOR_ID_XLS_408:  
			case CHIP_PROCESSOR_ID_XLS_404:  
				if(((gpio_base[GPIO_FUSEBANK_REG] & (1<<28)) == 0)  &&
						((gpio_base[GPIO_FUSEBANK_REG] & (1<<29)) == 0))
				{
					/* Below bits are set when ports are disabled.
					   28 - GMAC7
					   29 - GMAC6
					   30 - GMAC5
					   31 - GMAC4
					 */
					rmib->gmac_num = 8;
					rmib->gmac_list = 0xff;
					break;
				}
				else
				{
					rmib->gmac_num = 6;
					rmib->gmac_list = 0x3f;
					break;
				}

			case CHIP_PROCESSOR_ID_XLS_208:
			case CHIP_PROCESSOR_ID_XLS_204:
			case CHIP_PROCESSOR_ID_XLS_108:
			case CHIP_PROCESSOR_ID_XLS_104:  
				rmib->gmac_num = 4;
				rmib->gmac_list = 0x0f;
				break;

			default: printf ("Unknown proc_id:%d\n", rmip->processor_id);
				 return 0;
		}
	} 
	else /* for XLR boards */
	{
#ifdef RMI_EVAL_BOARD
		if ((rmib->maj == RMI_PHOENIX_BOARD_ARIZONA_II) && 
				(rmib->min == MINOR_A))
		{
			rmib->gmac_num = 2;
			rmib->gmac_list = 0xc0; /* gmac2 and gmac3 are present */
		} 
		else if (rmib->maj == RMI_PHOENIX_BOARD_ARIZONA_V)
		{
			rmib->gmac_num = 3;
			rmib->gmac_list = 0x07;
		} 
		else
#endif
		{
			rmib->gmac_num = 4;
			rmib->gmac_list = 0x0f;
		}

	}
	/* Set the USB mode (Host/device). Linux seems to be expecting this */
 	rmi_usb_set_mode();
	return 1;
}

void rmi_do_reset(void *arg /* IGNORED */)
{
	rmi_flush_icache_all();
	rmi_flush_dcache_all();
	drain_messages();
	rmi_flush_tlb_all();
}

int rmi_prelaunch_setup(void)
{
	uint32_t mask = boot_info.cpu_online_map;

	mask &= ~(1 << processor_id());

	rmi_init_boot_info();

	printf("Resetting state for secondary cpus 0x%x....", mask);
	rmi_wakeup_secondary_cpus(rmi_do_reset, 0, mask);
	printf("DONE\n");

	return 0;
}
int rmi_app_launch_setup (void)
{
    extern struct boot1_info boot_info;
    gd_t    *temp_gd;
    bd_t    *bd;
    struct eth_device *dev;

    __asm__ volatile (
                    "move   %0, $26\n\t"
		    :"=r"(temp_gd)
		    ::"$26"
		    );


    bd = temp_gd->bd;
    dev = eth_get_dev();

    if(dev) {
            bd->bi_enetaddr[0] = dev->enetaddr[0];
	    bd->bi_enetaddr[1] = dev->enetaddr[1];
	    bd->bi_enetaddr[2] = dev->enetaddr[2];
	    bd->bi_enetaddr[3] = dev->enetaddr[3];
	    bd->bi_enetaddr[4] = dev->enetaddr[4];
	    bd->bi_enetaddr[5] = dev->enetaddr[5];
    }
    bd->bi_boot_params = &boot_info;
    bd->bi_memstart = 0x20000000;
    bd->bi_memsize = temp_gd->ram_size;
    rmi_prelaunch_setup();
    
    return 0;
}    

