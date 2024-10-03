/*
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

struct pcie_controller pcie_hose[1];  /* only 1 PEX for EX2200 */

void pcie_register_hose(struct pcie_controller* hose)
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

	printf ("pcie_bus_to_hose() failed\n");
	return NULL;
}

pcie_dev_t pcie_find_devices(struct pcie_device_id *ids, int index)
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
	
					pcie_read_config_word(bdf,PCIE_DEVICE_ID,&device);
 
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
					}// for i loop
		
				 } // for function

				}// for device 

		}// for bus loop	

	}// for hose loop

	return (-1);
}

pcie_dev_t pcie_find_device(unsigned int vendor, unsigned int device, int index)
{
	static struct pcie_device_id ids[2] = {{}, {0, 0}};

	ids[0].vendor = vendor;
	ids[0].device = device;

	return pcie_find_devices(ids, index);
}



int pcie_hose_scan_bus(struct pcie_controller *hose, int bus)
{
	unsigned int sub_bus;
	unsigned short vendor, device, class;
	pcie_dev_t dev;
	u8 header_type = 0;

	sub_bus = bus;

	//printf("Scanning PCI Express Bus .. for bus %d\r\n",bus);
        //printf("\n        BUS DEV VEN_ID DEV_ID CLASS INT_LINE\n");

	dev =  PCIE_BDF(bus,0,0);
	
	for (dev =  PCIE_BDF(bus,0,0);
		   dev <  PCIE_BDF(bus,PCIE_MAX_PCIE_DEVICES-1,PCIE_MAX_PCIE_FUNCTIONS-1);
		   dev += PCIE_BDF(0,0,1))
	{
	
		/* If this is not a multi-function device, we skip the rest. */
		if (PCIE_FUNC(dev) && !(header_type & 0x80))
		continue;
        
		vendor = 0;
		header_type = 0;
   
		pcie_hose_read_config_word(hose, dev, PCIE_VENDOR_ID, &vendor);
		 
		if (vendor != 0xffff && vendor != 0x0000)
		{

//			printf ("PCI Express Scan: Found Bus %d, Device %d, Function %d : vendor is %x\n",
//				PCIE_BUS(dev), PCIE_DEV(dev), PCIE_FUNC(dev),vendor );

			pcie_hose_read_config_word(hose, dev, PCIE_DEVICE_ID, &device);
			pcie_hose_read_config_word(hose, dev, PCIE_CLASS_DEVICE, &class);
			pcie_read_config_byte(dev, PCIE_HEADER_TYPE, &header_type);
			int n = pcie_auto_config_device(hose, dev);

			sub_bus = max(sub_bus, n);
#if 0
			if (hose->fixup_irq)
				hose->fixup_irq(hose, dev);
#endif


		}
	}
	

	return sub_bus;
}


/*****************************************************************************
* Scan for the PCIE device attached to the selected hose 		 *
*****************************************************************************/
int pcie_hose_scan(struct pcie_controller *hose)
{

	/* Initialize the PCIE address translation space */
	pcie_auto_config_init(hose);
	
	/* Scan for the devices attached to the pcie BUS */
//	printf("scaning for PCIE bus %d\r\n",hose->first_busno);
	return pcie_hose_scan_bus(hose, hose->first_busno);
}


int pcie_init( void )
{
    PARAM_PCIE pcie_info;
    extern void pcie_arm_init(struct pcie_controller *hose, PARAM_PCIE *pcie_info, uint pex);

    pcie_info.pcie_mem_base = PEX0_MEM_BASE;
    pcie_info.pcie_mem_size = PEX0_MEM_SIZE;
    pcie_arm_init(&pcie_hose[0], &pcie_info, 0);  /* pex0 */

    return 0;
}


