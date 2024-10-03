/*
 * Copyright (c) 2009-2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * (C) Copyright 2001 Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Andreas Heppel <aheppel@sysgo.de>
 *
 * (C) Copyright 2002, 2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * PCI routines
 */

#include <common.h>

#include <command.h>
#include <asm/processor.h>
#include <asm/io.h>
#include <pcie.h>
#include "recb.h"

#define PCIE_HOSE_OP(rw, size, type)					\
int pcie_hose_##rw##_config_##size(struct pcie_controller *hose, 		\
				  pcie_dev_t dev, 			\
				  int offset, type value)		\
{									\
	return hose->rw##_##size(hose, dev, offset, value);		\
}

PCIE_HOSE_OP(read, byte, unsigned char *)
PCIE_HOSE_OP(read, word, unsigned short *)
PCIE_HOSE_OP(read, dword, unsigned int *)
PCIE_HOSE_OP(write, byte, unsigned char)
PCIE_HOSE_OP(write, word, unsigned short)
PCIE_HOSE_OP(write, dword, unsigned int)

#define PCIE_OP(rw, size, type, error_code)				\
int pcie_##rw##_config_##size(pcie_dev_t dev, int offset, type value)	\
{									\
	struct pcie_controller *hose = pcie_bus_to_hose(PCIE_BUS(dev));	\
									\
	if (!hose)							\
	{								\
		error_code;						\
		return -1;						\
	}								\
									\
	return pcie_hose_##rw##_config_##size(hose, dev, offset, value);	\
}

PCIE_OP(read, byte, unsigned char *, *value = 0xff)
PCIE_OP(read, word, unsigned short *, *value = 0xffff)
PCIE_OP(read, dword, unsigned int *, *value = 0xffffffff)
PCIE_OP(write, byte, unsigned char, )
PCIE_OP(write, word, unsigned short, )
PCIE_OP(write, dword, unsigned int, )

#define PCIE_READ_VIA_DWORD_OP(size, type, off_mask)			\
int pcie_hose_read_config_##size##_via_dword(struct pcie_controller *hose,\
					pcie_dev_t dev, 			\
					int offset, type val)		\
{									\
	unsigned int val32;							\
									\
	if (pcie_hose_read_config_dword(hose, dev, offset & 0xffc, &val32) < 0)\
		return -1;						\
									\
	*val = (val32 >> ((offset & (int)off_mask) * 8));		\
									\
	return 0;							\
}

#define PCIE_WRITE_VIA_DWORD_OP(size, type, off_mask, val_mask)		\
int pcie_hose_write_config_##size##_via_dword(struct pcie_controller *hose,\
						 pcie_dev_t dev, 		\
						 int offset, type val)	\
{									\
	unsigned int val32, mask, ldata, shift;					\
									\
	if (pcie_hose_read_config_dword(hose, dev, offset & 0xffc, &val32) < 0)\
		return -1;						\
									\
	shift = ((offset & (int)off_mask) * 8);				\
	ldata = (((unsigned long)val) & val_mask) << shift;		\
	mask = val_mask << shift;					\
	val32 = (val32 & ~mask) | ldata;				\
									\
	if (pcie_hose_write_config_dword(hose, dev, offset & 0xffc, val32) < 0)\
		return -1;						\
									\
	return 0;							\
}

PCIE_READ_VIA_DWORD_OP(byte, unsigned char *, 0x03)
PCIE_READ_VIA_DWORD_OP(word, unsigned short *, 0x02)
PCIE_WRITE_VIA_DWORD_OP(byte, unsigned char, 0x03, 0x000000ff)
PCIE_WRITE_VIA_DWORD_OP(word, unsigned short, 0x02, 0x0000ffff)

/*
 *
 */

static struct pcie_controller* pcie_hose_head = NULL;
struct pcie_controller pcie_hose;


void pcie_register_hose (struct pcie_controller* hose)
{
	struct pcie_controller **phose = &pcie_hose_head;

	while(*phose)
		phose = &(*phose)->next;

	hose->next = NULL;

	*phose = hose;
}

struct pcie_controller *pcie_bus_to_hose (int bus)
{
	struct pcie_controller *hose;

	for (hose = pcie_hose_head; hose; hose = hose->next)
		if (bus >= hose->first_busno && bus <= hose->last_busno)
			return hose;

	printf ("pcie_bus_to_hose() failed for bus%d\n", bus);
	return NULL;
}

pcie_dev_t pcie_find_devices (struct pcie_device_id *ids, int index)
{
	struct pcie_controller * hose;
	unsigned int bus,function,dev;
	unsigned short vendor = 0, device = 0;
	pcie_dev_t bdf;
	int i ;
	u8 header_type;
 
	for (hose = pcie_hose_head; hose; hose = hose->next)
	{
		for (bus = hose->first_busno; bus <= hose->last_busno; bus++)
		{
					
			for (dev = 0; dev < PCIE_MAX_PCIE_DEVICES; dev++) {

				header_type = 0;
				vendor = 0;

				for (function = 0; function < PCIE_MAX_PCIE_FUNCTIONS; function++) {

					/* If this is not a multi-function device, we skip the rest. */
					if (function && !(header_type & 0x80))
						break;

					bdf = PCIE_BDF(bus, dev, function);
					pcie_read_config_word(bdf,PCIE_VENDOR_ID,&vendor);
					if ((vendor == 0xFFFF) || (vendor == 0x0000))
						continue;
	
					pcie_read_config_word(bdf, PCIE_DEVICE_ID, &device);
					pcie_read_config_byte(bdf, PCIE_HEADER_TYPE, &header_type);

					for (i=0; ids[i].vendor != 0; i++)
					{
						if (vendor == ids[i].vendor &&
							device == ids[i].device)
						{
							if (index <= 0)
								return bdf;
							index--;
						}
					} /* for i loop	*/
				} /* for function */
			} /* for device */
		} /* for bus loop */
	} /* for hose loop */

	return (-1);
}

pcie_dev_t pcie_find_device (unsigned int vendor, unsigned int device, int index)
{
	static struct pcie_device_id ids[2] = {{}, {0, 0}};

	ids[0].vendor = vendor;
	ids[0].device = device;

	return pcie_find_devices(ids, index);
}

typedef struct _pci_vend_dev_name_entry {
	unsigned int vendor;
	unsigned int device;
	char *name;
} pci_vend_dev_name_entry;

static pci_vend_dev_name_entry pci_vendor_list[] = {
	{0x1957, 0x12, "MPC8548E PCIE Controller"},
	{0x10b5, 0x8508, "PLX PEX8508 PCIE Switch"},
	{0x1095, 0x3132, "Silicon Image Sil3132 SATA"},
	{0, 0, ""},	
};
#define PCIE_VEN_ID 0x1957
#define PCIE_DEV_ID 0x12
int pcie_flag;
pci_vend_dev_name_entry pci_vendor_list_ver_2_1_mpc8548[] = {
	{0x1957, 0x13, "MPC8548E PCIE Controller"},
	{0, 0, ""},	
};

static char *get_pcie_devname (unsigned int vendor, unsigned int device)
{
	uint major, minor;
	get_processor_ver(&major, &minor);
	pci_vend_dev_name_entry *p = NULL;
	if (major == 2 && minor != 0) {
		if (vendor == PCIE_VEN_ID && device == (PCIE_DEV_ID + 1)) {
			p = &pci_vendor_list_ver_2_1_mpc8548[0];
		} else {
			p = &pci_vendor_list[0];
		}
	} else {
		p = &pci_vendor_list[0];
	}
	while (p->vendor != 0) {
		if (p->vendor == vendor && p->device == device)
			return (p->name);
		p++;
	}
	return ("Unknown device");
}

int pcie_hose_scan_bus (struct pcie_controller *hose, int bus)
{
	unsigned int sub_bus;
	unsigned short vendor, device, class, device_2_1_id;
	pcie_dev_t dev;
	int n;	
	u8 header_type = 0 ;
	uint major, minor;;

	/*
	 * MPC8548 Product Family Device Migration from ver 2.0 to 2.1.
	 */
	get_processor_ver(&major, &minor);

	sub_bus = bus;

	dev =  PCIE_BDF(bus,0,0);
	
	for (dev =  PCIE_BDF(bus,0,0);
			dev <  PCIE_BDF(bus,PCIE_MAX_PCIE_DEVICES-1,PCIE_MAX_PCIE_FUNCTIONS-1);
			dev += PCIE_BDF(0,0,1))
	{
	
		/* If this is not a multi-function device, we skip the rest. */
		if (PCIE_FUNC(dev) && !(header_type & 0x80))
			continue;
   
		pcie_hose_read_config_word(hose, dev, PCIE_VENDOR_ID, &vendor);

		if (vendor != 0xffff && vendor != 0x0000) {
			pcie_hose_read_config_word(hose, dev, PCIE_DEVICE_ID, &device);
			pcie_hose_read_config_word(hose, dev, PCIE_CLASS_DEVICE, &class);
			pcie_read_config_byte(dev, PCIE_HEADER_TYPE, &header_type);

			if (((vendor == 0x10b5) && 
				((device == 0x8508) || (device == 0x8509))) &&
				((PCIE_BUS(dev) == 1) && (PCIE_DEV(dev) != 0)))
				continue;

			if (((vendor == 0x1095) && 
				(device == 0x3132)) &&
				((PCIE_BUS(dev) == 1) && (PCIE_DEV(dev) != 0)))
				continue;

			if (pcie_flag) {
				/* Check for mpc8548 version 2.1 or 2.0 */
				if (major == 2 && minor != 0) {
					device_2_1_id = device;
					if (vendor == PCIE_VEN_ID && device_2_1_id == (PCIE_DEV_ID + 1)) { 
						goto no_disp;
					}
				} else { 
					if (vendor == PCIE_VEN_ID && device == PCIE_DEV_ID) {
						goto no_disp;
					}
				}
			}

			printf ("Found %s, (%d, %d, %d), (%x, %x)\n",
					get_pcie_devname(vendor, device), PCIE_BUS(dev),
					PCIE_DEV(dev), PCIE_FUNC(dev), vendor, device);
no_disp:
			if (pcie_flag) {
			n = pcie_auto_config_device(hose, dev);			
			sub_bus = max(sub_bus, n);
			}
		}
	}
	
	return sub_bus;
}


/*****************************************************************************
* Scan for the PCIE device attached to the selected hose 		 *
*****************************************************************************/
int pcie_hose_scan (struct pcie_controller *hose)
{
	uint8_t link_stat;
	pcie_dev_t bdf;
	volatile uint *pcie_af00 = (uint *) (CFG_CCSRBAR + 0xAf00);
	uint8_t link_stat_reg;
	uint8_t lane_mode;
	pcie_flag = 0;

	/* Initialize the PCIE address translation space */
	pcie_auto_config_init(hose);
	
	/* Scan for the devices attached to the pcie BUS */
	printf("Scaning PCIE bus: \r\n");

	/* Display pcie vendor-id and device-id */
	 pcie_hose_scan_bus(hose, hose->first_busno);

	/* Check the PCIE link training status before 
	 doing a PCIE scan. This will avoid CPU hang if
	 link is not up */

	bdf = PCIE_BDF(hose->first_busno, 0, 0);
	pcie_hose_read_config_byte(hose, 0, PCIE_LTSSM, &link_stat);
	printf("Link Training Status State Machine reg  = 0x%02x\n", link_stat);
	pcie_hose_read_config_byte(&pcie_hose, 0, PCIE_LTSR, &link_stat_reg);
	printf("The Link Satus reg =  0x%0x\n", link_stat_reg);
	lane_mode = (link_stat_reg >> 4);
	if (lane_mode == 4) {
		printf("Correct link width trained :x%x\n", lane_mode);
	} else {
		printf("Link width trained :x%x (Expected :x4)\n", lane_mode);
	}
	if(link_stat < PCIE_LTSSM_L0) {
		printf("no link on pcie; status = 0x%02x\n", link_stat);
		printf(" Applying PCIe workaround \n");
		*pcie_af00 |= 0x08000000;
		udelay(5000);
		*pcie_af00 &= 0xF7FFFFFF;
		udelay(1000);
		pcie_hose_read_config_byte(hose, 0, PCIE_LTSSM, &link_stat);
		printf("Link Training Status State Machine reg  = 0x%02x\n", link_stat);
		pcie_hose_read_config_byte(&pcie_hose, 0, PCIE_LTSR, &link_stat_reg);
		printf("The Link Satus reg = 0x%0x\n", link_stat_reg);
		lane_mode = (link_stat_reg >> 4);
		if (lane_mode == 4) {
			printf("Correct link width trained x%x\n", lane_mode);
		} else {
			printf("Link width trained :x%x (Expected :x4)\n", lane_mode);
		}
		if (link_stat < PCIE_LTSSM_L0) {
			printf("FATAL Error - no link on pcie after workaround; status = 0x%02x\n", link_stat);
			return (0);
		}
	}
	pcie_flag = 1;
	return pcie_hose_scan_bus(hose, hose->first_busno);
}



int pcie_init ( void )
{
PARAM_PCIE pcie_info;

#ifdef CONFIG_MPC8548
	/* skip pcie for HDRE */
	if (cb_is_hdre()) {
		return 0;
	}

	pcie_info.pcie_mem_base = CFG_PCIE_MEM_BASE;
	pcie_info.pcie_mem_size = CFG_PCIE_MEM_SIZE;

	printf("\n");
	pcie_mpc85xx_init(&pcie_hose, &pcie_info);
	printf("\n");
#endif

	return 0;
}


