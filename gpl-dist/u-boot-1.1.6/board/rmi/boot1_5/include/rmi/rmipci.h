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
#ifndef __PCI_H__
#define __PCI_H__

/*
#include "types.h"
#include "board.h"
#include "device.h"
#include "addrspace.h"
#include "byteorder.h"
#include "pcireg.h"
*/

#include "rmi/types.h"
#include "rmi/addrspace.h"
#include "rmi/byteorder.h"
#include "physaddrspace.h"

#define PCI_SWAP32(x) __swab32(x)

/*
		  #ifdef __MIPSEB__
		  #define PCI_SWAP32(x) __swab32(x)
		  #else
		  #define PCI_SWAP32(x) (x)
		  #endif
*/

#define MAX_PCI_DEVS 32
#define MAX_PCI_FNS 8
#define PCI_NUM_SPACE 12

/*  use the defs in physaddrspace.h, so there is only one unique definition */
#define RMI_PCIX_MEMORY_BASE PCIE_MEM_BAR_BASE
#define RMI_PCIX_MEMORY_SIZE PCIE_MEM_BAR_SIZE
#define RMI_PCIX_IO_SPACE_BASE PCIE_IO_BAR_BASE
#define RMI_PCIX_IO_SPACE_SIZE PCIE_IO_BAR_SIZE
#define RMI_PCIX_CONFIG_BASE PCIE_CONFIG_BAR_BASE
#define RMI_PCIX_CONFIG_SIZE PCIE_CONFIG_BAR_SIZE

#define mdelay(n) ({unsigned long msec=(n); while (msec--) udelay(1000);})

/* 
   In order to scan a PCI bus, we need the following information:
   1. Configuration Space address.
   2. Functions for PCI Configuaration Space read/write.
   3. PCI memory space base and size
   4. PCI I/O space base and size.
   5. Bus number
*/
struct pci_config_ops
{
	int (*_pci_cfg_r8) (unsigned long config_base,int reg, uint8_t *val);
	int (*_pci_cfg_r16)(unsigned long config_base,int reg, uint16_t *val);
	int (*_pci_cfg_r32)(unsigned long config_base,int reg, uint32_t *val);

	int (*_pci_cfg_w8) (unsigned long config_base,int reg, uint8_t val);
	int (*_pci_cfg_w16)(unsigned long config_base,int reg, uint16_t val);
	int (*_pci_cfg_w32)(unsigned long config_base,int reg, uint32_t val);
};

struct pci_device
{
        char name[90];
	char controller_name[10];
	uint32_t bus;
        uint32_t dev;
        uint32_t fn;

        uint16_t vendor;
        uint16_t device;
        uint32_t class;
        uint8_t  hdr_type;

	unsigned long config_base;
	struct pci_config_ops *pci_ops;

	struct addr_space bars[PCI_NUM_SPACE];

	int (*driver)(struct pci_device *this_dev);

	struct pci_device *parent;

	struct pci_device *children;

	struct pci_device *next;
	struct pci_device *prev;
};

#define pci_reg_addr(pci_dev,reg) \
	( ( (((pci_dev)->bus) <<16) | ( ( (pci_dev)->dev) << 11)  | ( ( (pci_dev)->fn) << 8)) + (reg) )

#define pci_cfg_r8(dev,reg,val) \
	(((dev)->pci_ops)->_pci_cfg_r8((dev)->config_base, pci_reg_addr((dev),(reg)), (val)))
#define pci_cfg_r16(dev,reg,val) \
	(((dev)->pci_ops)->_pci_cfg_r16((dev)->config_base, pci_reg_addr((dev),(reg)), (val)))
#define pci_cfg_r32(dev,reg,val) \
	(((dev)->pci_ops)->_pci_cfg_r32((dev)->config_base, pci_reg_addr((dev),(reg)), (val)))

#define pci_cfg_w8(dev,reg,val) \
	(((dev)->pci_ops)->_pci_cfg_w8((dev)->config_base, pci_reg_addr((dev),(reg)), (val)))
#define pci_cfg_w16(dev,reg,val) \
	(((dev)->pci_ops)->_pci_cfg_w16((dev)->config_base, pci_reg_addr((dev),(reg)), (val)))
#define pci_cfg_w32(dev,reg,val) \
	(((dev)->pci_ops)->_pci_cfg_w32((dev)->config_base, pci_reg_addr((dev),(reg)), (val)))

extern void printf (const char *fmt, ...);

/*
	ToDo: check this out

extern int pci_find_cap(struct pci_device *this_dev, int cap);
extern int pci_dev_enable(struct pci_device *this_dev);
extern int enumerate_pci_bus(struct pci_device *parent,
			     struct addr_space *available_mem,
			     struct addr_space *available_io);
extern struct pci_device *pci_find_dev(unsigned short int vendor, 
					  unsigned short int device,
					  struct pci_device *start);
extern struct pci_device *pci_find_dev_bdf(unsigned short int vendor,
        unsigned short int device, struct pci_device *start,
        unsigned int bdf);

extern int detect_pcie_mode(phoenix_reg_t *gpio_mmio);
extern int pcie_reset_controller(phoenix_reg_t *condor_pcie_ctrl_mmio_le);
extern int pcie_setup_bars(phoenix_reg_t *bridge_mmio);
extern int pcie_setup_msi_one(phoenix_reg_t *condor_pcie_ctrl_mmio_le);
extern int pcie_reacivate_links(phoenix_reg_t *condor_pcie_ctrl_mmio_le);
extern int pcie_setup_msi_two(phoenix_reg_t *condor_pcie_ctrl_mmio_le);
extern int pcie_setup_base_limit(phoenix_reg_t *condor_pcie_ctrl_mmio_le);
extern int pcie_device_mode_init(unsigned long config_base, phoenix_reg_t *condor_pcie_ctrl_mmio_le);
extern int pcie_enable_ltssm(phoenix_reg_t *condor_pcie_ctrl_mmio_le);
extern int pcie_get_link_status(phoenix_reg_t *condor_pcie_ctrl_mmio_le,int regNum);
extern int setup_pcie_device_mode(phoenix_reg_t *bridge_mmio,phoenix_reg_t *condor_pcie_ctrl_mmio_le);

*/


#endif
