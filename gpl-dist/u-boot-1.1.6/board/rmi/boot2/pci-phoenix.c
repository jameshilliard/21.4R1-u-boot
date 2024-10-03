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

#include "common.h"
#include <malloc.h>
#include "rmi/cache_error.h"
#include "rmi/rmipci.h"
#include "rmi/pcireg.h"
#include "rmi/pci-phoenix.h"
#include "rmi/on_chip.h"
#include "rmi/byteorder.h"
#include "rmi/gpio.h"
#include "rmi/bridge.h"
#include "pci.h"

#undef PCI_DBG

#ifdef  PCI_DBG
#define pci_dbg(fmt,args...)     printf(fmt,##args)
#else
#define pci_dbg(fmt,args...)
#endif

/* ToDo. This should be removed.*/
int pci_ctrl_mode = 1;

#define config_offset(x) ((x) + (((x) & (0xff<<16)) * MB(16)))

extern int pcie_init_xls_board (void); 

static inline unsigned int read_cfg32(unsigned int config_base, unsigned int addr)
{   
   volatile unsigned int temp = 0;
   unsigned int *p = (unsigned int *)(config_base + (addr & ~3));
   volatile uint64_t cerr_cpu_log = 0;

	disable_and_clear_cache_error();

   temp = PCI_SWAP32(*p);

   cerr_cpu_log = read_64bit_phnx_ctrl_reg(CPU_BLOCKID_LSU, LSU_CERRLOG_REGID);

   if(cerr_cpu_log)
   {
      /* Cache error occured. */
      temp = ~0x0;
   }
	clear_and_enable_cache_error();
   return temp;
}

static inline void write_cfg32(unsigned int config_base, unsigned int addr, unsigned int data)
{
   volatile unsigned int *p = (unsigned int *)(config_base + (addr & ~3));

   *p = PCI_SWAP32(data);
	pci_dbg("[%s]: addr = %x, date = %x\n", __FUNCTION__, (unsigned int)p, data);
}

static int _pci_cfg_r8(unsigned long config_base, int reg, uint8_t * val)
{
	unsigned int data = 0;

	data = read_cfg32((unsigned int)config_base,reg);
	*val = (data >> ((reg & 3) << 3)) & 0xff;

	return 0;
}

static int _pci_cfg_r16(unsigned long config_base, int reg, uint16_t * val)
{
	unsigned int data = 0;
	if (reg & 1)
		return 1;

	data = read_cfg32((unsigned int)config_base,reg);

	*val = (data >> ((reg & 3) << 3)) & 0xffff;

	return 0;
}

static int _pci_cfg_r32(unsigned long config_base, int reg, uint32_t * val)
{
	unsigned int data = 0;

	if (reg & 3)
		return 1;

	data = read_cfg32((unsigned int)config_base,reg);

	*val = data;

	return 0;
}

static int _pci_cfg_w8(unsigned long config_base, int reg, uint8_t val)
{
	unsigned int data = 0;
	data = read_cfg32((unsigned int)config_base,reg);
	data = (data & ~(0xff << ((reg & 3) << 3))) |
	    (val << ((reg & 3) << 3));
	write_cfg32((unsigned int)config_base,reg, data);

	return 0;
}

static int _pci_cfg_w16(unsigned long config_base, int reg, uint16_t val)
{
	unsigned int data = 0;
	if (reg & 1)
		return 1;

	data = read_cfg32((unsigned int)config_base,reg);

	data = (data & ~(0xffff << ((reg & 3) << 3))) |
	    (val << ((reg & 3) << 3));

	write_cfg32((unsigned int)config_base,reg, data);

	return 0;
}

static int _pci_cfg_w32(unsigned long config_base,int reg, uint32_t val)
{
	if (reg & 3)
		return 1;

	write_cfg32((unsigned int)config_base,reg, val);

	return 0;
}

static int rmi_pci_read_config_byte 
	(struct pci_controller *hose, pci_dev_t dev, int reg, u8 * val)
{
	int rc = 0;
	rc = _pci_cfg_r8 ((unsigned long)phys_to_kseg1((unsigned long)RMI_PCIX_CONFIG_BASE),(dev+reg),val);
	return rc;
}
static int rmi_pci_read_config_word 
	(struct pci_controller *hose, pci_dev_t dev, int reg, u16 * val)
{
	int rc = 0;
	rc = _pci_cfg_r16((unsigned long)phys_to_kseg1((unsigned long)RMI_PCIX_CONFIG_BASE),(dev+reg),val);
	return rc;
}
static int rmi_pci_read_config_dword 
	(struct pci_controller *hose, pci_dev_t dev, int reg, u32 * val)
{
	int rc = 0;
	rc = _pci_cfg_r32((unsigned long)phys_to_kseg1((unsigned long)RMI_PCIX_CONFIG_BASE),(dev+reg),val);
	return rc;
}
static int rmi_pci_write_config_byte 
	(struct pci_controller *hose, pci_dev_t dev, int reg, u8 val)
{
	int rc = 0;
	rc = _pci_cfg_w8 ((unsigned long)phys_to_kseg1((unsigned long)RMI_PCIX_CONFIG_BASE),(dev+reg),val);
	return rc;
}
static int rmi_pci_write_config_word 
	(struct pci_controller *hose, pci_dev_t dev, int reg, u16 val)
{
	int rc = 0;
	rc = _pci_cfg_w16((unsigned long)phys_to_kseg1((unsigned long)RMI_PCIX_CONFIG_BASE),(dev+reg),val);
	return rc;
}
static int rmi_pci_write_config_dword 
	(struct pci_controller *hose, pci_dev_t dev, int reg, u32 val)
{
	int rc = 0;
	rc = _pci_cfg_w32((unsigned long)phys_to_kseg1((unsigned long)RMI_PCIX_CONFIG_BASE),(dev+reg),val);
	return rc;
}

struct pci_config_ops pcix_cfg_ops =
{
	._pci_cfg_r8  = _pci_cfg_r8,
	._pci_cfg_r16 = _pci_cfg_r16,
	._pci_cfg_r32 = _pci_cfg_r32,

	._pci_cfg_w8  = _pci_cfg_w8,
	._pci_cfg_w16 = _pci_cfg_w16,
	._pci_cfg_w32 = _pci_cfg_w32
};

/*
 * Initialize module
 */

int pci_init_xlr_board (void)
{
   phoenix_reg_t *bridge_mmio = 
		(phoenix_reg_t *) (DEFAULT_PHOENIX_IO_BASE + PHOENIX_IO_BRIDGE_OFFSET);

	phoenix_reg_t *phoenix_pci_ctrl_mmio = 
		(phoenix_reg_t *) (DEFAULT_PHOENIX_IO_BASE + PHOENIX_IO_PCIX_OFFSET);

   phoenix_reg_t *gpio_mmio  = 
		(phoenix_reg_t *) (DEFAULT_PHOENIX_IO_BASE + PHOENIX_IO_GPIO_OFFSET);

	uint32_t value = 0;
	uint32_t ctrl  = 0;
	int pcix_mode  = 0;

	value = cpu_read_reg(phoenix_pci_ctrl_mmio,PCIX_INIT_DBG0_REG);
	pcix_mode = (value & 0x01) ? PCI_CTRL_PCIX_MODE : PCI_CTRL_PCI_MODE;

	if (pcix_mode == PCI_CTRL_PCIX_MODE) {
		/* DLL Settings for PCIX Mode */
		pci_dbg("PCI BUS is in PCIX mode (DLL settings: 0x%x)\n",
			cpu_read_reg(gpio_mmio,(0xBC >> 2)));
	} 
	else if (pcix_mode == PCI_CTRL_PCI_MODE) {
		/* DLL Settings for PCI Mode */
		cpu_write_reg(gpio_mmio,GPIO_PCIX_PLL_DIS,0x01);	/* Disable */
		cpu_write_reg(gpio_mmio,GPIO_PCIX_DLY,  0x12);	    /* R=2; C=2; */
		cpu_write_reg(gpio_mmio,GPIO_PCIX_PLL_DIS,0x00);	/* Enable */
		pci_dbg("PCI BUS is in PCI mode (DLL settings: 0x%x)\n", 
			cpu_read_reg(gpio_mmio,GPIO_PCIX_DLY));
	}
	ctrl = cpu_read_reg(phoenix_pci_ctrl_mmio,PCIX_HOST_MODE_CTRL_REG);
	pci_ctrl_mode = (ctrl & 0x02) ? PCI_CTRL_HOST_MODE : PCI_CTRL_DEVICE_MODE;

	if(pci_ctrl_mode == PCI_CTRL_HOST_MODE) {

		struct pci_device *pci_dev;
		struct addr_space *available_mem, *available_io;
		unsigned long config_base = 0;
		uint32_t bar = 0;
		uint64_t size = 0;;
		uint64_t mask = 0;
		uint32_t en = 0x01;
		uint32_t cmd_stat = 0;

		printf("PCI is configured in Host mode\n");

		available_mem = (struct addr_space *)malloc(sizeof(*available_mem));
		if(available_mem == NULL)
		{
			printf("%s Not enough memory.\n",__FUNCTION__);
			return -1;
		}

		memset(available_mem,0,sizeof(*available_mem));

		available_io = (struct addr_space *)malloc(sizeof(*available_io));
		if(available_io == NULL)
		{
			printf("%s Not enough memory.\n",__FUNCTION__);
			free(available_mem);
			return -1;
		}

		memset(available_io,0,sizeof(*available_io));

		pci_dev = (struct pci_device *)malloc(sizeof(*pci_dev));
		if(pci_dev == NULL)
		{
			printf("%s Not enough memory.\n");
			free(available_mem);
			free(available_io);
			return -1;
		}

		memset(pci_dev,0,sizeof(*pci_dev));

		/* ToDo: sprintf(pci_dev->controller_name,"pci"); */

		pci_dev->pci_ops = &pcix_cfg_ops;

		mdelay(100);

		/* program the bars before 
		 * pulling PCI out of reset 
		 */
    
        
        if(is_xlr_c_series()){
    		/* Bar 0 & 1 : base and size */
	    	cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_HBAR0_REG  , 0x08);
    		cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_ENBAR0_REG , 0x80000000);

            /*Don't enable bar1-5 on C* series*/
            
            /* Disable Bar 1 */
		    cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_HBAR1_REG  , 0x0);
	    	cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_ENBAR1_REG , 0x0);
            
            /* Disable Bar 2 & 3 */
	    	cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_HBAR2_REG  , 0x0);
		    cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_HBAR3_REG  , 0x0);
    		cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_ENBAR2_REG , 0x0);
	    	cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_ENBAR3_REG , 0x0);

            /* Disable Bar 4 & 5 */
	    	cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_HBAR4_REG  , 0x0);
		    cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_HBAR5_REG  , 0x0);
    		cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_ENBAR4_REG , 0x0);
	    	cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_ENBAR5_REG , 0x0);

            /* Enable Memcoherency for all BARs */
            cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_PHOENIX_CONTROL_REG, 0x00100300);
        }else{
                /* BAR 0 & 1 are combined as 64 bit BAR,
                   2GB at Base address 0x0.             
                   Bar 0 & 1 : base and size */

                cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_HBAR0_REG  , 0x0C);
                cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_HBAR1_REG  , 0x0);
                cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_ENBAR0_REG , 0x80000000);
                cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_ENBAR1_REG , 0xffffffff);

		  /* BAR 4 is 32 bit BAR, 1GB at Base address 0x80000000. */


                cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_HBAR4_REG  , 0x80000008);
                cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_ENBAR4_REG , 0xc0000000);

                /* BAR 5 is 32 bit BAR, 512MB at Base address 0xE0000000. */

                cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_HBAR5_REG  , 0xe0000008);
                cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_ENBAR5_REG , 0xe0000000);

		/* BAR 2 & 3 are combined as 64 bit BAR of size 4GB at 
		   Base address 0x100000000  */


                cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_HBAR2_REG  , 0x0c);
                cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_HBAR3_REG  , 0x00000001);
                cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_ENBAR2_REG , 0x00000001);
                cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_ENBAR3_REG , 0xFFFFFFFF);

            /* Enable Memcoherency for all BARs */
            cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_PHOENIX_CONTROL_REG, 0x00107f00);
        }

	
		/* Take the PCIX controller out of reset */
		cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_HOST_MODE_CTRL_REG, 0x02);

		/* delay for the config scan - for now, wait for the worst case */
		mdelay(1*1000);

		pci_dbg("PCI Host mode Status = %x\n",
			cpu_read_reg(phoenix_pci_ctrl_mmio,PCIX_HOST_MODE_STATUS_REG));

		/* Setup PCI-X Memory bar in Bridge */
		bar = ((uint32_t)RMI_PCIX_MEMORY_BASE >> 24) << 16;
		size = (uint64_t)RMI_PCIX_MEMORY_SIZE; 
       mask = ((size >> 24) - 1) << 1;
		cpu_write_reg(bridge_mmio,PCIXMEM_BAR, (bar | mask | en));

		available_mem->base = (unsigned long) RMI_PCIX_MEMORY_BASE ;
		available_mem->size = size;

		/* Setup PCI-X IO bar in Bridge */
		bar = ((uint32_t)RMI_PCIX_IO_SPACE_BASE >> 26) << 18;
		size = (uint64_t)RMI_PCIX_IO_SPACE_SIZE;
		mask = 0;
		cpu_write_reg(bridge_mmio,PCIXIO_BAR, (bar | mask | en));

		available_io->base = (unsigned long)RMI_PCIX_IO_SPACE_BASE;
		available_io->size = size;

		/* Setup PCI-X Config bar in Bridge */
		config_base = (unsigned long)RMI_PCIX_CONFIG_BASE;
		bar = (((uint32_t) config_base) >> 25) << 17;
		size = (uint64_t)RMI_PCIX_CONFIG_SIZE;
		mask = 0;
		cpu_write_reg(bridge_mmio,BRIDGE_PCIXCFG_BAR,(bar | mask | en));

		config_base = (unsigned long)phys_to_kseg1(config_base);

		if (config_base == (unsigned long)-1) {
			printf("Unable to map PCIX Config space\n");
			free(pci_dev);
			free(available_mem);
			free(available_io);
			return -1;
		}
		
		pci_dev->config_base = config_base;

		pci_cfg_r32(pci_dev,PCI_REG_COMMAND,&cmd_stat);
		cmd_stat = cmd_stat | (PCI_VAL_COMMAND_MEMEN | PCI_VAL_COMMAND_MWINVEN);
		pci_cfg_w32(pci_dev,PCI_REG_COMMAND,cmd_stat);

		return 0;
	}
	if(pci_ctrl_mode == PCI_CTRL_DEVICE_MODE)
	{
		printf("Phoenix is configured in Device mode\n");
		cpu_write_reg(bridge_mmio,BRIDGE_PCIXMEM_BAR,(0x8000fffe | 0x01));	//512G
		/* Device Mapper Configuration for 32MB @ 128MB */
		cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_DEVMODE_TBL_BAR0_REG,0x00080000);
		cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_DEVMODE_TBL_BAR1_REG,0x00088000);
		cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_DEVMODE_TBL_BAR2_REG,0x00090000);
		cpu_write_reg(phoenix_pci_ctrl_mmio,PCIX_DEVMODE_TBL_BAR3_REG,0x00098000);
		
		return 0;
	}
	else
	{
		printf("Phoenix PCI configuration is in unknown mode.\n");
		return -1;
	}
	return 0;
}

uint32_t rmi_link_state[PCI_MAX_PCI_DEVICES];
uint32_t pcie_link_state[PCI_MAX_PCI_DEVICES];
int rmi_change_class = 0;
int rmi_config_bus0 = 1;
extern volatile uint32_t link0, link1;
extern volatile uint32_t link2, link3;

void fixup_rmi_pci(void)
{
	int i;
	for(i=0; i < PCI_MAX_PCI_DEVICES; i++) {
		rmi_link_state[i] = 1;
		/* This variable is used in pci_hose_scan_bus */
		if(chip_processor_id_xls()) {
			if(i < 4) 
				rmi_link_state[i] = pcie_link_state[i];
			else
				rmi_link_state[i] = 0;
		}
	}
	/* For XLS before B0 parts, class_id field in config space
	   needs to be set. This is done in pci_hose_scan_bus();
	   */
	if(chip_processor_id_xls() && !chip_pid_xls_b0())
		rmi_change_class = 1;

	if(!chip_processor_id_xls())
		rmi_config_bus0 = 0;

}

struct pci_controller hose0;

void pci_init_board (void)
{
	struct pci_controller *hose;
	unsigned long skip_scan = 0;
	char * s;

	memset(&hose0, 0, sizeof(hose0));
	hose = &hose0;
	hose->first_busno = 0;
	hose->last_busno = 0xff;

	/* PCI Memory region */
	pci_set_region (hose->regions + 0,
		RMI_PCIX_MEMORY_BASE, RMI_PCIX_MEMORY_BASE, RMI_PCIX_MEMORY_SIZE,
		PCI_REGION_MEM);

	/* PCI I/O space */
	pci_set_region (hose->regions + 1,
		RMI_PCIX_IO_SPACE_BASE, RMI_PCIX_IO_SPACE_BASE, RMI_PCIX_IO_SPACE_SIZE,
		PCI_REGION_IO);

   hose->region_count = 2;
   
   hose->read_byte = rmi_pci_read_config_byte;
   hose->read_word = rmi_pci_read_config_word;
   hose->read_dword = rmi_pci_read_config_dword;
   hose->write_byte = rmi_pci_write_config_byte;
   hose->write_word = rmi_pci_write_config_word;
   hose->write_dword = rmi_pci_write_config_dword;

	/*
	 *  hose->cfg_addr and hose->cfg_data are not set, since we have 
	 *  config read/write functions, which do not need these
	 */

	if(chip_processor_id_xls()) {
		pcie_init_xls_board();
		pcie_link_state[0] = link0;
		pcie_link_state[1] = link1;
		pcie_link_state[2] = link2;
		pcie_link_state[3] = link3;
	}
	else
		pci_init_xlr_board();

   	fixup_rmi_pci();
	pci_register_hose (hose); 
	
	if ((s = getenv ("hw.pci.enable_static_config")) != NULL) {
		skip_scan = simple_strtoul (s, NULL, 16);
	}
	if (!skip_scan) {
		hose->last_busno = pci_hose_scan (hose);
	}
	else {
		debug("skip pci scan because the env variable"
			  " hw.pci.enable_static_config is set.\n");
	}
}

