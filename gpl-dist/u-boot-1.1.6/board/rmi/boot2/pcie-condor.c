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

#define Message(a,b...) //printf("\nFUNC [%s], LINE [%d]"a"\n",__FUNCTION__,__LINE__,##b)
#undef PCI_DBG

#ifdef  PCI_DBG
#define pci_dbg(fmt,args...)     printf(fmt,##args)
#else
#define pci_dbg(fmt,args...)
#endif

#define config_offset(x) ((x) + (((x) & (0xff<<16)) * MB(16)))

/* Globals, for link status, to 
 * determine enumeration for XLS
 */
volatile uint32_t link0 = 0, link1 = 0;
volatile uint32_t link2 = 0, link3 = 0;

extern struct pci_config_ops pcix_cfg_ops;
extern struct rmi_board_info rmi_board;


/* We are going to assume Root Complex Mode */

static int flag_ep = 0, flag_2x2 = 0;
int detect_pcie_mode(phoenix_reg_t *gpio_mmio)
{
	 unsigned int pci_ctrl_mode   = (cpu_read_reg(gpio_mmio,21) >>20) & 0x03;

	 if( (pci_ctrl_mode & 0x1) == 0) {
		  flag_ep = (1<<0);
	 }

	 if(chip_processor_id_xls2xx() || (pci_ctrl_mode & 0x2)) {
		  flag_2x2 = (1<<1);
	 }
	 return (flag_ep|flag_2x2);
}
int pcie_reset_controller(phoenix_reg_t *condor_pcie_ctrl_mmio_le) 
{
	 /* The RMI-Specific PCIE Block */
	 cpu_write_reg(condor_pcie_ctrl_mmio_le, 0, 0x00000000);
	 cpu_write_reg(condor_pcie_ctrl_mmio_le, 1, 0x00000000);
    
	 if(chip_processor_id_xls2xx() || chip_pid_xls_b0()) {
		  cpu_write_reg(condor_pcie_ctrl_mmio_le, 8, 0x00000000);
		  cpu_write_reg(condor_pcie_ctrl_mmio_le, 9, 0x00000000);
	 }

	 return 0;
}

void pcie_setup_bars(phoenix_reg_t *bridge_mmio)
{
	uint32_t bar = 0;
	uint64_t size = 0;;
	uint64_t mask = 0;
	uint32_t en = 0x01;
	
	debug("pcie bars: mem 0x%x, io 0x%x, conf %x\n", 
		  PCIE_MEM_BAR_BASE, PCIE_IO_BAR_BASE, PCIE_CONFIG_BAR_BASE);
	
	/* Setup PCIE Memory bar in Bridge */
	bar     = (PCIE_MEM_BAR_BASE >> 24) << 16;
	size    = PCIE_MEM_BAR_SIZE;
	mask    = (((size >> 24) - 1) << 1);
	
	cpu_write_reg(bridge_mmio, 0x42, (bar | mask | en));
	
	/* Setup PCIE IO bar in Bridge */
	bar = (PCIE_IO_BAR_BASE >> 26) << 18;
	size = PCIE_IO_BAR_SIZE;
	mask = (((size >> 26) - 1) << 1);
	cpu_write_reg(bridge_mmio, 0x43, (bar | mask | en));
	
	/* Setup PCIE Config bar in Bridge */
	bar = (PCIE_CONFIG_BAR_BASE >> 25) << 17;
	size = PCIE_CONFIG_BAR_SIZE;
	mask = 0;
	cpu_write_reg(bridge_mmio, 0x40, (bar | mask | en));
	
	return;
}
int pcie_setup_msi_one(phoenix_reg_t *condor_pcie_ctrl_mmio_le)
{
        /* MSI Section.
         * ------------------------------------------------------
         * 1. Enable all LINK0 and LINK1 MSI Interrupts using the 
         *    PCIE_LINK0_MSI_ENABLE & PCIE_LINK1_MSI_ENABLE Regs.
         * 2. Currently, not enabling the PHY-related LINK0/LINK1 
         *    interrupts.
         * ------------------------------------------------------
         */
        cpu_write_reg(condor_pcie_ctrl_mmio_le, 38, 0xffffffff);
        cpu_write_reg(condor_pcie_ctrl_mmio_le, 39, 0xffffffff);

        if(chip_processor_id_xls2xx() || chip_pid_xls_b0()){
		  cpu_write_reg(condor_pcie_ctrl_mmio_le, 102, 0xffffffff);
		  cpu_write_reg(condor_pcie_ctrl_mmio_le, 103, 0xffffffff);
        }

        /* Enable Legacy Interrupts INT A/B/C/D on Link0 & Link1 */
        cpu_write_reg(condor_pcie_ctrl_mmio_le, 44, 0x0000000f);
        cpu_write_reg(condor_pcie_ctrl_mmio_le, 45, 0x0000000f);
        
        if(chip_processor_id_xls2xx() || chip_pid_xls_b0()){
		  cpu_write_reg(condor_pcie_ctrl_mmio_le, 108, 0x0000000f);
		  cpu_write_reg(condor_pcie_ctrl_mmio_le, 109, 0x0000000f);
        }

	 return 0;
}
int  pcie_reacivate_links(phoenix_reg_t *condor_pcie_ctrl_mmio_le)
{
	 /* set Link0/Link1 SYS_AUX_PWR_DET to 1'b1 in PCIE_CTRL_1 */
	 cpu_write_reg(condor_pcie_ctrl_mmio_le, 0, 0x01000100);

	 if(chip_processor_id_xls2xx() || chip_pid_xls_b0()){
		  cpu_write_reg(condor_pcie_ctrl_mmio_le, 8, 0x01000100);
	 }
        
	 /* set the LINK0_RST_N and LINK1_RST_N to 1'b1 (bring it out of reset) */
	 cpu_write_reg(condor_pcie_ctrl_mmio_le, 0, 0x01010101);
        
	 if(chip_processor_id_xls2xx() || chip_pid_xls_b0()){
		  cpu_write_reg(condor_pcie_ctrl_mmio_le, 8, 0x01010101);
	 }

	 /* set PHY_TX_LVL ,PHY_LOS_LVL and PHY_ACJT_LVL in PCIE_CTRL3 */
	 cpu_write_reg(condor_pcie_ctrl_mmio_le, 2, 0x0c0c1000);
		  
	 return 0;
}
int pcie_condor_2x2_workaround(phoenix_reg_t *bridge_mmio)
{	 
        volatile uint32_t i = 0;
	 cpu_write_reg(bridge_mmio, 65, 0x00C00001);
	 sw_40bit_phys_uncached(0xC00007A8ULL, (uint32_t)0xD9001900);
	 for (i=0;i<0x10000;i++);
	 sw_40bit_phys_uncached(0xC0000748ULL, (uint32_t)0x36802140);
	 return 0;
}
int pcie_setup_msi_two(phoenix_reg_t *condor_pcie_ctrl_mmio_le)
{
        /* Set the MSI ADDRESS Register : Intel's 0xFEE0_0000 */
        sw_40bit_phys_uncached(0x18000054, 0x0000e0fe); 
        sw_40bit_phys_uncached(0x18000854, 0x0000e0fe);
        sw_40bit_phys_uncached(0x18001054, 0x0000e0fe);
        sw_40bit_phys_uncached(0x18001854, 0x0000e0fe);

        /* Enable MSI */
        sw_40bit_phys_uncached(0x18000050, 0x05708100);
        sw_40bit_phys_uncached(0x18000850, 0x05708100);
        sw_40bit_phys_uncached(0x18001050, 0x05708100);
        sw_40bit_phys_uncached(0x18001850, 0x05708100);
	 return 0;
        
}
int pcie_setup_base_limit(phoenix_reg_t *condor_pcie_ctrl_mmio_le) 
{
	 cpu_write_reg(condor_pcie_ctrl_mmio_le, 20, 0x00000000);
	 cpu_write_reg(condor_pcie_ctrl_mmio_le, 21, 0xffffffff);
	 return 0;
}

extern void cpld_check_lte_routing(void);

int pcie_enable_ltssm(phoenix_reg_t *condor_pcie_ctrl_mmio_le)
{
        /* Enable LTSSM */
        cpu_write_reg(condor_pcie_ctrl_mmio_le, 0, 0x01030103);

        if(chip_processor_id_xls2xx() || chip_pid_xls_b0()){
		  cpu_write_reg(condor_pcie_ctrl_mmio_le, 8, 0x01030103);
        }
	 return 0;
}
int pcie_get_link_status(phoenix_reg_t *condor_pcie_ctrl_mmio_le,int regNum)
{
	 volatile uint32_t link = cpu_read_reg(condor_pcie_ctrl_mmio_le, regNum);
	 volatile uint32_t i = 0;
	 /* Wait for Link UP */                      

	 while (!(link & 0x4000)) { 
		  /* Bit 14 Should become 1 */
		  link = cpu_read_reg(condor_pcie_ctrl_mmio_le, regNum);
		  if (i == 0x50000) {
			   break;
		  }
		  i++;
	 }
	 if (link & 0x4000) {
		  link = 1;
	 }
	 else {
		  link = 0;
	 }
	 
	 return link;
}
int pcie_device_mode_init(unsigned long config_base, phoenix_reg_t *condor_pcie_ctrl_mmio_le) 
{
	 volatile unsigned long temp = 0;
	 /* Sanity Check */
	 /* Controller's own config space located at Bus:0, Dev:3, Fn:0 */
	 temp = lw_40bit_phys_uncached((config_base | 0x1800));
	 printf("Processor [Vendor/Device ID]: [0x%08x]\n", PCI_SWAP32(temp));

	 /* Change BAR 0 Mask for 32 MB */
	 temp = cpu_read_reg(condor_pcie_ctrl_mmio_le, 4);
	 cpu_write_reg(condor_pcie_ctrl_mmio_le, 4, temp|0x10);

	 sw_40bit_phys_uncached( (config_base | 0x1810), PCI_SWAP32( (MB(32) - 1) ));
	 sw_40bit_phys_uncached( (config_base | 0x1814), 0x0);
	 cpu_write_reg(condor_pcie_ctrl_mmio_le, 4, temp & ~0x10);
	 return 0;
}
int setup_pcie_device_mode(phoenix_reg_t *bridge_mmio,phoenix_reg_t *condor_pcie_ctrl_mmio_le)
{
	 volatile unsigned long temp = 0;

	 /* 
	    Program DEV Mapper Registers 
	    PCIE_EP_ADDR_MAP_ENTRY0 - PCIE_EP_ADDR_MAP_ENTRY3
	 */
	 cpu_write_reg(condor_pcie_ctrl_mmio_le, 28, 0x00080000);
	 cpu_write_reg(condor_pcie_ctrl_mmio_le, 29, 0x00088000);
	 cpu_write_reg(condor_pcie_ctrl_mmio_le, 30, 0x00090000);
	 cpu_write_reg(condor_pcie_ctrl_mmio_le, 31, 0x00098000);

	 temp = cpu_read_reg(condor_pcie_ctrl_mmio_le, 4);
	 cpu_write_reg(condor_pcie_ctrl_mmio_le, 4, temp|0x08);

	 cpu_write_reg(condor_pcie_ctrl_mmio_le, 26, 0x0);
	 cpu_write_reg(condor_pcie_ctrl_mmio_le, 27, 0xffffffff);

	 printf("PCIe: Mapped Device Memory to 32MB @ 128MB\n");

	 /*Set Defeature bit*/
	 temp = cpu_read_reg(bridge_mmio, 59);
	 temp |= 0x02;
	 cpu_write_reg(bridge_mmio, 59, temp);


	 return 0;

}


int pcie_init_xls_board (void)
{
	 /*
	  * bridge_mmio           = 0x1ef00000 
	  * condor_pcie_ctrl_mmio = 0x1ef00000 + 0x1e000 [BE]
	  * condor_pcie_ctrl_mmio = 0x1ef00000 + 0x1f000 [LE]
	  */

	int pci_dev_mode;

	phoenix_reg_t *bridge_mmio = 
		(phoenix_reg_t *) (DEFAULT_PHOENIX_IO_BASE + PHOENIX_IO_BRIDGE_OFFSET);	

	phoenix_reg_t *gpio_mmio = 
		(phoenix_reg_t *) (DEFAULT_PHOENIX_IO_BASE + PHOENIX_IO_GPIO_OFFSET);

	phoenix_reg_t *condor_pcie_ctrl_mmio_le = 
		(phoenix_reg_t *) (DEFAULT_PHOENIX_IO_BASE + PHOENIX_IO_PCIE_1_OFFSET);

	struct rmi_board_info * rmib = &rmi_board;
	pci_dev_mode = detect_pcie_mode(gpio_mmio);

	printf("PCIE: ");
	if(chip_processor_id_xls() && 
			(!chip_processor_id_xls2xx()) &&
			(!chip_pid_xls_b0())) {
		if(flag_2x2) {
			if(flag_ep) {
				    printf("[1EPx2/1RCx2]");
			} 
			else {
				printf("[2RCx2]");
			}
		} 
		else {
			if(flag_ep) {
				printf("[1EPx4]");
			}
			else {
				printf("[1RCx4]");
			}
		}
	} 
	else if(chip_processor_id_xls2xx() || chip_pid_xls_b0()){
         if(flag_ep) {
            if(flag_2x2) {
               printf("[1EPx1/3RCx1]");
            }
            else {
               printf("[1EPx4]");
            }
         }
         else {
            if(flag_2x2) {
            	printf("[4RCx1]");
             }
             else {
             	printf("[1RCx4]");
             }
         }
	}

	printf(" Mode Detected\n");

	pcie_reset_controller(condor_pcie_ctrl_mmio_le);

	/* Provided our accesses are ok, this is the
	 * sequence we need to follow...
	 * -------------------------------------------------
	 * 1. Program Bridge's PCIE BARs
	 *      -- CFG = 0x18000000
	 *      -- MEM = 0xd0000000
	 *      -- IO  = 0x10000000
	 * 2. Bring PCIE Core out of Reset  (PCIE Block)
	 * 3. Program XLS's PCIE Config Space 
	 * 4. Enable LTSSM State Machine    (PCIE Block)
	 * 5. Wait for Link to Come up      (PCIE Block)
	 * -------------------------------------------------
	 */

	pcie_setup_bars(bridge_mmio);
	pcie_setup_msi_one(condor_pcie_ctrl_mmio_le);
	pcie_reacivate_links(condor_pcie_ctrl_mmio_le);
	/* Temporary Workaround for XLS 2x2 Mode */
	if (flag_2x2 && !chip_processor_id_xls2xx() && !chip_pid_xls_b0()) 
	{
		pcie_condor_2x2_workaround(bridge_mmio);
	}

   cpu_write_reg(bridge_mmio, 65, 0x00C00000);

	/* Sanity Check */
	pci_dbg("Condor [XLS] Vendor/Device ID: 0x%lx\n", 
		lw_40bit_phys_uncached(0x18000000));

	/* Program the XLS MSI Capability Structure here... */
	pcie_setup_msi_two(condor_pcie_ctrl_mmio_le);
	/* Also, program the Base/Limit Regs in the PCIE Block
	 * to sane reset values. 
	 */
	pcie_setup_base_limit(condor_pcie_ctrl_mmio_le);

	pci_dbg("Programmed TGT_COHERENT_MEM_BASE/Limit Values.\n");

	/* On the LTE Reference Board, the onboard CPLD 
	 * routes PCIE/USB Signals to mini-PCIE and/or 
	 * ExpressCard Slots, based on Jumper Settings.
	 * Detect the configuration here...
	 */

	if ((rmib->maj) == 8) {
		printf("LiTE Board: Checking PCIE Lane Logic...\n");
		cpld_check_lte_routing();
	}

	pcie_enable_ltssm(condor_pcie_ctrl_mmio_le);

	mdelay(2000);

	if(!flag_ep) {
		link0 = pcie_get_link_status(condor_pcie_ctrl_mmio_le,32);
		if(link0)
			debug("\nPCIE0 Link Up.\n");
	}

	if(flag_2x2) {
		link1 = pcie_get_link_status(condor_pcie_ctrl_mmio_le,33);
		if(link1)
			debug("\nPCIE1 Link Up.\n");
	}

	if(chip_processor_id_xls2xx() || chip_pid_xls_b0()) {
		link2 = pcie_get_link_status(condor_pcie_ctrl_mmio_le,96);
		if(link2)
			debug("\nPCIE2 Link Up.\n");
		  
		link3 = pcie_get_link_status(condor_pcie_ctrl_mmio_le,97);
		if(link3)
			debug("\nPCIE3 Link Up.\n");
	}


	 return 0;
}
