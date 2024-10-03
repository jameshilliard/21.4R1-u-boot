#include <common.h>
#include <pcie.h>
#include "pex/mvPex.h"
#include "pci-if/mvPciIf.h"

static void pcie_auto_prescan_setup_bridge(struct pcie_controller *,
	                                         pcie_dev_t, int);
static void pcie_auto_postscan_setup_bridge(struct pcie_controller *,
	                                         pcie_dev_t, int);

#undef DEBUG	
#ifdef DEBUG
#define DEBUGF(x...) printf(x)
#else
#define DEBUGF(x...)
#endif /* DEBUG */


void pcie_auto_region_init(struct pcie_region* res)
{
	res->bus_lower = res->bus_start;
}
void pcie_auto_region_align(struct pcie_region *res, unsigned  long size)
{
	res->bus_lower = ((res->bus_lower - 1) | (size - 1)) + 1;
}

int pcie_auto_region_allocate(struct pcie_region* res, unsigned int size, unsigned long long *bar)
{
// need to be re-written

	unsigned long long addr;

	if (!res) {
		DEBUGF("No resource");
		goto error;
	}

	DEBUGF("size=0x%08X \n", size);

	addr = ((res->bus_lower - 1) | (size - 1)) + 1;

	if (addr - res->bus_start + size > res->size) {
		DEBUGF("No room in resource");
		goto error;
	}

	res->bus_lower = addr + size;

	DEBUGF("address=0x%08X \n", addr);
	DEBUGF("bus_lower=0x%08X \n", res->bus_lower);

	*bar = addr;
	return 0;

 error:
	*bar = 0xffffffff;
	return -1;

}




/********************************************************************************
 * Assign hose Memory and IO region. And initialize. The region pointer is kept	*
 * in hose->pcie_mem and hose->pcie_io structure					*
 *******************************************************************************/
void pcie_auto_config_init(struct pcie_controller *hose)
{
	int i;

	hose->pcie_io = hose->pcie_mem = NULL;

	/* Assign the global IO and memory region */
	for (i=0; i<hose->region_count; i++)
	{
		switch(hose->regions[i].flags)
		{
		case PCIE_REGION_IO:
			/* If the current hose->region is IO assign that as the hose pci IO region */
			if (!hose->pcie_io ||
			    hose->pcie_io->size < hose->regions[i].size)
				hose->pcie_io = hose->regions + i;
			break;
		case PCIE_REGION_MEM:
			/* If the current hose->region is Mem assign that as the hose pci Mem region */
			if (!hose->pcie_mem ||
			    hose->pcie_mem->size < hose->regions[i].size)
				hose->pcie_mem = hose->regions + i;
			break;
		}
	}

	/* Initialize the memory region */
	if (hose->pcie_mem)
	{
		/* Initialize the Memory region pointer to the region start address */
		pcie_auto_region_init(hose->pcie_mem);
		DEBUGF("PCI Express Autoconfig: Memory region: [0x%08x-0x%08x]\n",
		    hose->pcie_mem->bus_start,
		    hose->pcie_mem->bus_start + hose->pcie_mem->size - 1);
	}
	
	/* Initialize the IO region */
	if (hose->pcie_io) 
	{
		/* Initialize the Memory region pointer to the region start address */
		pcie_auto_region_init(hose->pcie_io);

		DEBUGF("PCI Express Autoconfig: I/O region: [%llx-%llx]\n",
		    hose->pcie_io->bus_start,
		    hose->pcie_io->bus_start + hose->pcie_io->size - 1);
	}
}

/* HJF: Changed this to return int. I think this is required
 * to get the correct result when scanning bridges
 */
int pcie_auto_config_device(struct pcie_controller *hose, pcie_dev_t dev)
{
	unsigned int sub_bus = PCIE_BUS(dev);
	unsigned short class;
	unsigned char prg_iface;
	int n = 0;
 
	int pcie_mode = PCI_IF_MODE_HOST;
	unsigned int link;


	/* EX2200 bus 0 dev 0/1 */
	if ((sub_bus > 0) || (PCIE_DEV(dev) > mvCtrlPciIfMaxIfGet()))
		return sub_bus;

	pcie_hose_read_config_word(hose, dev, PCIE_CLASS_DEVICE, &class);
	DEBUGF("PCIe class=0x%04X \n", class);

	switch(class) {
	case 0xb20:/*<< MPC8548 specific data to detect on chip PCI Express bridge>>*/
	case PCIE_CLASS_MEMORY_OTHER:  /* Cheetah-J2 */
	case PCIE_CLASS_BRIDGE_PCI:
   	case PCIE_CLASS_BRIDGE_OTHER:

	mvPexModeGet(mvPciRealIfNumGet(0),&pcie_mode);
    
	if(pcie_mode == PCI_IF_MODE_HOST) //Host(RC)
	{
		
		hose->current_busno++;
		
		/* Dont do this setup for 8548 */
		if (class != 0xb20)
		{
			pcie_auto_setup_device(hose, dev, 2, hose->pcie_mem, hose->pcie_io);
		}
		else
		{
			pcie_auto_setup_device(hose, dev, 1, hose->pcie_mem, hose->pcie_io);
		}

		DEBUGF("PCI Express Autoconfig: Found P2P bridge, device %d\n", PCIE_DEV(dev));

		/* Passing in current_busno allows for sibling P2P bridges */
		pcie_auto_prescan_setup_bridge(hose, dev, hose->current_busno);

		link = MV_REG_READ(PEX_STATUS_REG(0)) & 0x1;
     	       /*
		 * need to figure out if this is a subordinate bridge on the bus
		 * to be able to properly set the pri/sec/sub bridge registers.
		 */
		if (link == 0x0)
		    n = pcie_hose_scan_bus(hose, hose->current_busno);
		
   		
		/* figure out the deepest we've gone for this leg */
		sub_bus = max(n, sub_bus);
		pcie_auto_postscan_setup_bridge(hose, dev, sub_bus);
 
	}
 
		sub_bus = hose->current_busno;
		break;



	case PCIE_CLASS_STORAGE_IDE:
		pcie_hose_read_config_byte(hose, dev, PCIE_CLASS_PROG, &prg_iface);
#if 0		
	if (!(prg_iface & PCIAUTO_IDE_MODE_MASK)) {
			DEBUGF("PCI Autoconfig: Skipping legacy mode IDE controller\n");
			return sub_bus;
		}
#endif
		pcie_auto_setup_device(hose, dev, 6, hose->pcie_mem, hose->pcie_io);
		break;

	case PCIE_CLASS_BRIDGE_CARDBUS:
		/* just do a minimal setup of the bridge, let the OS take care of the rest */
		pcie_auto_setup_device(hose, dev, 0, hose->pcie_mem, hose->pcie_io);

		DEBUGF("PCI Express Autoconfig: Found P2CardBus bridge, device %d\n", PCIE_DEV(dev));

		hose->current_busno++;
		break;


	default:

		pcie_auto_setup_device(hose, dev, 6, hose->pcie_mem, hose->pcie_io);
		
		break;
	}

	return sub_bus;
}



void pcie_auto_setup_device(struct pcie_controller *hose,
			  pcie_dev_t dev, int bars_num,
			  struct pcie_region *mem,
			  struct pcie_region *io)
{
	unsigned int  bar_response, bar_size;
	unsigned long long bar_value;
	unsigned int cmdstat = 0;
	struct pcie_region *bar_res;
	int bar, bar_nr = 0;
	int found_mem64 = 0;
	unsigned int temp_unit;

	pcie_hose_read_config_dword(hose, dev, PCIE_COMMAND, &cmdstat);
	cmdstat = (cmdstat & ~(PCIE_COMMAND_IO | PCIE_COMMAND_MEMORY)) | PCIE_COMMAND_MASTER;

	for (bar = PCIE_BASE_ADDRESS_0; bar <= PCIE_BASE_ADDRESS_0 + (bars_num*4); bar += 4) {
		/* Tickle the BAR and get the response */
		pcie_hose_write_config_dword(hose, dev, bar, 0xffffffff);
		pcie_hose_read_config_dword(hose, dev, bar, &bar_response);
		/* If BAR is not implemented go to the next BAR */
		if (!bar_response)
			continue;

		found_mem64 = 0;

		/* Check the BAR type and set our address mask */
		if (bar_response & PCIE_BASE_ADDRESS_SPACE) {
			bar_size = ~(bar_response & PCIE_BASE_ADDRESS_IO_MASK) + 1;
			bar_res = io;

			DEBUGF("PCI Express Autoconfig: BAR %d, I/O, size=0x%x, ", bar_nr, bar_size);
		} else {
			if ( (bar_response & PCIE_BASE_ADDRESS_MEM_TYPE_MASK) ==
			     PCIE_BASE_ADDRESS_MEM_TYPE_64)
				found_mem64 = 1;

			bar_size = ~(bar_response & PCIE_BASE_ADDRESS_MEM_MASK) + 1;
			bar_res = mem;

			DEBUGF("PCI Express Autoconfig: BAR %d, Mem, size=0x%x, ", bar_nr, bar_size);
		}

		if (pcie_auto_region_allocate(bar_res, bar_size, &bar_value) == 0) {
			/* Write it out and update our limit */
			temp_unit = (unsigned int)(bar_value & 0xffffffff);
			pcie_hose_write_config_dword(hose, dev, bar, temp_unit);
			//DEBUGF("offest %x at: addr %x\r\n",bar,temp_unit);

			/*
			 * If we are a 64-bit decoder then increment to the
			 * upper 32 bits of the bar and force it to locate
			 * in the lower 4GB of memory.
			 */
			if (found_mem64) {
				bar += 4;
				temp_unit = (unsigned int)((bar_value & 0xffffffff00000000ULL)>>32); 
				pcie_hose_write_config_dword(hose, dev, bar, temp_unit);
			
			//	DEBUGF("offest %x at: addr %x\r\n",bar,temp_unit);
			}

			cmdstat |= (bar_response & PCIE_BASE_ADDRESS_SPACE) ?
				PCIE_COMMAND_IO : PCIE_COMMAND_MEMORY;
		}

		DEBUGF("\n");

		bar_nr++;
	}

	pcie_hose_write_config_dword(hose, dev, PCIE_COMMAND, cmdstat);
	pcie_hose_write_config_byte_via_dword(hose, dev, PCIE_CACHE_LINE_SIZE, 0x08);
	pcie_hose_write_config_byte_via_dword(hose, dev, PCIE_LATENCY_TIMER, 0x32);
}



static void pcie_auto_prescan_setup_bridge(struct pcie_controller *hose,
					 pcie_dev_t dev, int sub_bus)
{
	struct pcie_region *pcie_mem = hose->pcie_mem;
	struct pcie_region *pcie_io = hose->pcie_io;
	unsigned int cmdstat;
	unsigned int temp_int;	

	pcie_hose_read_config_dword(hose, dev, PCIE_COMMAND, &cmdstat);

	/* Configure bus number registers */
	pcie_hose_write_config_byte_via_dword(hose, dev, PCIE_PRIMARY_BUS, PCIE_BUS(dev));
	pcie_hose_write_config_byte_via_dword(hose, dev, PCIE_SECONDARY_BUS, sub_bus);
	pcie_hose_write_config_byte_via_dword(hose, dev, PCIE_SUBORDINATE_BUS, 0x7F);
	
 
	pcie_hose_read_config_dword(hose, dev, PCIE_PRIMARY_BUS, &temp_int);
	//printf("combine is %x\r\n",temp_int);
	if (pcie_mem) {
        
#if 0
		/* Round memory allocator to 1MB boundary */
		pcie_auto_region_align(pcie_mem, 0x100000);
#endif

		/* Set up memory and I/O filter limits, assume 32-bit I/O space */
		pcie_hose_write_config_word_via_dword(hose, dev, PCIE_MEMORY_BASE,
					(pcie_mem->bus_lower & 0xfff00000) >> 16);

		cmdstat |= PCIE_COMMAND_MEMORY;
	}

	if (pcie_io) {
#if 0
		/* Round I/O allocator to 4KB boundary */
		pcie_auto_region_align(pcie_io, 0x1000);

		pcie_hose_write_config_byte_via_dword(hose, dev, PCIE_IO_BASE,
					(pcie_io->bus_lower & 0x0000f000) >> 8);
		pcie_hose_write_config_word_via_dword(hose, dev, PCIE_IO_BASE_UPPER16,
					(pcie_io->bus_lower & 0xffff0000) >> 16);

#endif
		cmdstat |= PCIE_COMMAND_IO;
	}

#if 0
	/* We don't support prefetchable memory for now, so disable */
	pcie_hose_write_config_word_via_dword(hose, dev, PCIE_PREF_MEMORY_BASE, 0x1000);
	pcie_hose_write_config_word_via_dword(hose, dev, PCIE_PREF_MEMORY_LIMIT, 0x1000);
#endif
	
       /* Enable memory and I/O accesses, enable bus master */
	pcie_hose_write_config_dword(hose, dev, PCIE_COMMAND, cmdstat | PCIE_COMMAND_MASTER);
}

static void pcie_auto_postscan_setup_bridge(struct pcie_controller *hose,
					  pcie_dev_t dev, int sub_bus)
{
	struct pcie_region *pcie_mem = hose->pcie_mem;
 
	/* Configure bus number registers */

	pcie_hose_write_config_byte_via_dword(hose, dev, PCIE_SUBORDINATE_BUS, sub_bus);


		if (pcie_mem)
		{

#if 0
			/* Round memory allocator to 1MB boundary */
			pciauto_region_align(pcie_mem, 0x100000);
#endif
			pcie_hose_write_config_word_via_dword(hose, dev, PCIE_MEMORY_LIMIT,
												  (pcie_mem->bus_lower-1) >> 16);
		}

#if 0
		if (pcie_io)
		{
			/* Round I/O allocator to 4KB boundary */
			pciauto_region_align(pcie_io, 0x1000);

			pcie_hose_write_config_byte_via_dword(hose, dev, PCIE_IO_LIMIT,
												  ((pcie_io->bus_lower-1) & 0x0000f000) >> 8);
			pcie_hose_write_config_word_via_dword(hose, dev, PCIE_IO_LIMIT_UPPER16,
												  ((pcie_io->bus_lower-1) & 0xffff0000) >> 16);
		}
#endif

}

