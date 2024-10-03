/*
 * Copyright (c) 2009-2010, Juniper Networks, Inc.
 * All rights reserved.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#ifdef CONFIG_SRX3000

#ifdef CONFIG_POST

#include <configs/srx3000.h>
#include <command.h>
#include <asm/processor.h>
#include <asm/io.h>
#include <pci.h>
#include <pcie.h>
#include <post.h>
#if CONFIG_POST & CFG_POST_PCI

#define MAX_PCI_VENDOR_ID       0xFFFF
#define MIN_PCI_VENDOR_ID       0X0000
#define BUS_PCIE_MPC8548_CTLR   0
#define BUS1_PCIE_PEX8508_SWT   1
#define BUS2_PCIE_PEX8508_SWT   2
#define BUS_PCIE_SATA_CTLR      12
#define MAX_DEV_PEX8508_SWT     4

#define MAX_PCIE_MPC8548_BUS        1
#define MAX_PCIE_PEX8508_SWT1_BUS   2
#define MAX_PCIE_PEX8508_SWT2_BUS   3
#define MAX_PCIE_SATA_CTRL_BUS      13
#define MAX_PCI_DEVICE              12
#define MAX_PCI_ENTRY               6


static  int pci_fail;
static  int pci_flag;
int boot_flag_post;
int post_result_pci;
extern struct pcie_controller pcie_hose;
static int pcie_result_store[MAX_PCI_DEVICE][MAX_PCI_ENTRY];
static int pcie_link_status(int bus_numm, int pci_flag);

typedef struct _pci_vend_dev_list {
	unsigned int vendor;
	unsigned int device;
	char *name;
} pci_ven_dev_list;

static pci_ven_dev_list  pci_expected_dev_list[] = {
	{0x1957, 0x12, "MPC8548E PCIE Controller"},
	{0x10b5, 0x8508, "PLX PEX8508 PCIE Switch"},
	{0x1095, 0x3132, "Silicon Image Sil3132 SATA"},
	{0, 0, ""},
};

static pci_ven_dev_list  pci_expected_dev_list_ver_2_1[] = {
	{0x1957, 0x13, "MPC8548E PCIE Controller"},
};


/* Check the PCIE link training status before
 * doing a PCIE scan. This will avoid CPU hang if
 * link is not up
 */
static int
pcie_link_status (int bus_num, int pci_flag) {
	int ret = 1;
	uint8_t link_stat;
	uint8_t lane_mode;
	uint8_t link_stat_reg;
	pcie_dev_t bdf;
	volatile uint *pcie_af00 = (uint *) (CFG_CCSRBAR + 0xAf00);
	/* Initialize the PCIE address translation space */
	pcie_auto_config_init(&pcie_hose);
	bdf = PCIE_BDF(bus_num, 0, 0);
	pcie_hose.first_busno = bus_num;
	pcie_hose_read_config_byte(&pcie_hose, 0, PCIE_LTSSM, &link_stat);
	pcie_hose_read_config_byte(&pcie_hose, 0, PCIE_LTSR, &link_stat_reg);
	/* check for lane 4 mode */
	lane_mode = (link_stat_reg >> 4);
	/* Store the result of link_stat and lane_mode */
	pcie_result_store[bus_num][5] = link_stat;
	pcie_result_store[bus_num][6] = lane_mode;
	if (link_stat < PCIE_LTSSM_L0) {
		if (pci_flag) {
			printf("no link on pcie; status = 0x%02x\n", link_stat);
			printf(" Applying PCIe workaround\n");
		}
		*pcie_af00 |= 0x08000000;
		udelay(5000);
		*pcie_af00 &= 0xF7FFFFFF;
		udelay(1000);
		pcie_hose_read_config_byte(&pcie_hose, 0, PCIE_LTSSM, &link_stat);
		pcie_hose_read_config_byte(&pcie_hose, 0, PCIE_LTSR, &link_stat_reg);
		/* check for lane 4 mode */
		lane_mode = (link_stat_reg >> 4);
		/* Store the result of link_stat and lane_mode */
		pcie_result_store[bus_num][5] = link_stat;
		pcie_result_store[bus_num][6] = lane_mode;
		if (link_stat < PCIE_LTSSM_L0) {
			if (pci_flag) {
				printf("FATAL Error - no link on pcie after workaround; status"
						" = 0x%02x\n", link_stat);
			}
			pci_fail = -1;
			ret = 0;
		}
	}
	return ret;
}


void
pcie_scan_rout (int bus_num)
{
	unsigned char header_type = 0;
	unsigned short  vendor_id = 0;
	unsigned short  device_id = 0;
	int device                = 0;
	int function              = 0;
	pci_dev_t dev;
	uint major, minor;
    
	get_processor_ver(&major, &minor);

	for (device = 0; device < PCIE_MAX_PCIE_DEVICES; device++) {
		header_type = 0;
		vendor_id = 0;
		device_id = 0;
		for (function = 0; function < PCIE_MAX_PCIE_FUNCTIONS; function++) {
			/* If this is not a multi-function device, we skip the rest. */
			if (function && !(header_type & 0x80)) {
				continue;
			}
			dev = PCIE_BDF(bus_num, device, function);
			pcie_hose_read_config_word(&pcie_hose, dev,
					PCIE_VENDOR_ID, &vendor_id);
			if ((vendor_id != MAX_PCI_VENDOR_ID)
					&& (vendor_id != MIN_PCI_VENDOR_ID)) {
				pcie_hose_read_config_word(&pcie_hose, dev,
						PCI_DEVICE_ID, &device_id);

				pcie_hose_read_config_byte(&pcie_hose, dev, 
						PCIE_HEADER_TYPE, &header_type);

				if (bus_num == BUS_PCIE_MPC8548_CTLR) {
					if ( major == 2 && minor != 0) {
						if (vendor_id == pci_expected_dev_list_ver_2_1[bus_num].vendor
								&& device_id == pci_expected_dev_list_ver_2_1[bus_num].device) {
							pcie_result_store[bus_num][0] = 1;
						} 
					} else {	
						if (vendor_id == pci_expected_dev_list[bus_num].vendor
								&& device_id == pci_expected_dev_list[bus_num].device) {
							pcie_result_store[bus_num][0] = 1;
						} 
					}
				} else if (bus_num == BUS1_PCIE_PEX8508_SWT) {
					if (vendor_id == pci_expected_dev_list[bus_num].vendor
							&& device_id == pci_expected_dev_list[bus_num].device) {
						pcie_result_store[bus_num][0] = 1;
					} 
				} else if (bus_num == BUS2_PCIE_PEX8508_SWT) {
					if (vendor_id == pci_expected_dev_list[bus_num - 1].vendor
							&& device_id == pci_expected_dev_list[bus_num - 1].device) {
						pcie_result_store[bus_num][PCIE_DEV(dev)] = 1;
					}
				} else if (bus_num == BUS_PCIE_SATA_CTLR) {
					if (vendor_id == pci_expected_dev_list[bus_num - 10].vendor
							&& device_id == pci_expected_dev_list[bus_num - 10].device) {
						pcie_result_store[bus_num][0] = 1;
					} 
				} else {
					printf(" Not Valid Buss Number\n");
				}
			}
		}
	}
}
int pci_post_test (int flags)
{
	int bus_num     = 0;
	int dev         = 0;
	pci_fail        = 0;
	boot_flag_post  = 0;
	post_result_pci = 0;
	int i           = 0;
	int j           = 0;
	uint major, minor;
	
    /*
	 * MPC8548 Product Family Device Migration from Rev 2.0.1 to 2.1.2.
	 */
	get_processor_ver(&major, &minor);

	for (i = 0; i <= MAX_PCI_DEVICE; i++) {
		for (j = 0; j <= MAX_PCI_ENTRY; j++) {
			pcie_result_store[i][j] = 0x00;
		}
	}

	if (flags & POST_DEBUG) {
		pci_flag = 1;
	}	

	if (flags & BOOT_POST_FLAG) {
		boot_flag_post = 1;
	}

	for (bus_num = 0; bus_num < MAX_PCIE_MPC8548_BUS;  bus_num++) {
			pcie_scan_rout(bus_num);
	}

	for (bus_num = 1; bus_num < MAX_PCIE_PEX8508_SWT1_BUS; bus_num++) {
		if (pcie_link_status(bus_num, pci_flag)) {
			pcie_scan_rout(bus_num);
		}
	}
	for (bus_num = 2; bus_num < MAX_PCIE_PEX8508_SWT2_BUS; bus_num++) {
		if (pcie_link_status(bus_num, pci_flag)) {
			pcie_scan_rout(bus_num);
		}
	}
	for (bus_num = 12; bus_num < MAX_PCIE_SATA_CTRL_BUS; bus_num++) {
		if (pcie_link_status(bus_num, pci_flag)) {
			pcie_scan_rout(bus_num);
		}
	}

	printf("\n\n");

	/* Display the pci result for bus number 0 & 1 */
	for (bus_num = 0; bus_num <= MAX_PCIE_MPC8548_BUS; bus_num++) {
		if (bus_num == 1) {
			if (!boot_flag_post) {
				printf("Link Training Status State Machine reg 0x%02x\n",
						pcie_result_store[bus_num][5]);
				if (pcie_result_store[bus_num][6] == 4) {
					printf("Correct link width trained :x%x\n",
							pcie_result_store[bus_num][6]);
				} else {
					printf("Link width trained :x%x (Expected :x4)\n",
							pcie_result_store[bus_num][6]);
				}
			}
		}

		if (major == 2 && minor != 0 && bus_num == 0) {
			if (pcie_result_store[bus_num][0]) {
				if (!boot_flag_post) {
					printf(" Expected PCIE device %s Found Bus_Num = %d,"
							" Device = %d,"
							" Vendor_id  = %x, device_id = %x\n",
							pci_expected_dev_list_ver_2_1[bus_num].name,
							bus_num, dev,
							pci_expected_dev_list_ver_2_1[bus_num].vendor,
							pci_expected_dev_list_ver_2_1[bus_num].device);
				}
			} else {
				if (!boot_flag_post) {
					if (pci_flag) {
						printf(" Expected PCIE Device %s Not Found Bus_Num = %d,"
								" Device = %d,"
								" vendor_id = %x, device_id = %x\n",
								pci_expected_dev_list_ver_2_1[bus_num].name,
								bus_num, dev,
								pci_expected_dev_list_ver_2_1[bus_num].vendor,
								pci_expected_dev_list_ver_2_1[bus_num].device);
						pci_fail = -1;
					}
				} else {
					pci_fail = -1;
				}
			}
		} else {
			if (pcie_result_store[bus_num][0]) {
				if (!boot_flag_post) {
					printf(" Expected PCIE device %s Found Bus_Num = %d,"
							" Device = %d,"
							" Vendor_id  = %x, device_id = %x\n",
							pci_expected_dev_list[bus_num].name,
							bus_num, dev,
							pci_expected_dev_list[bus_num].vendor,
							pci_expected_dev_list[bus_num].device);
				}
			} else {
				if (!boot_flag_post) {
					if (pci_flag) {
						printf(" Expected PCIE Device %s Not Found Bus_Num = %d,"
								" Device = %d,"
								" vendor_id = %x, device_id = %x\n",
								pci_expected_dev_list[bus_num].name,
								bus_num, dev, pci_expected_dev_list[bus_num].vendor,
								pci_expected_dev_list[bus_num].device);
						pci_fail = -1;
					}
				} else {
					pci_fail = -1;
				}
			}
		}
	}

	/* Display the pci result for bus number 2 */  
	for (bus_num = 2; bus_num < MAX_PCIE_PEX8508_SWT2_BUS; bus_num++) {
		if (!boot_flag_post) {
			printf("Link Training Status State Machine reg 0x%02x\n",
					pcie_result_store[bus_num][5]);
			if (pcie_result_store[bus_num][6] == 4) {
				printf("Correct link width trained :x%x\n",
						pcie_result_store[bus_num][6]);
			} else {
				printf("Link width trained :x%x (Expected :x4)\n",
						pcie_result_store[bus_num][6]);
			}
		}
		for (dev = 1; dev <= MAX_DEV_PEX8508_SWT; dev++) {
			if (pcie_result_store[bus_num][dev]) {
				if (!boot_flag_post) {
					printf(" Expected PCIE device %s Found Bus_Num = %d,"
						    " Device = %d,"
							" Vendor_id  = %x, device_id = %x\n",
							pci_expected_dev_list[bus_num - 1].name,
							bus_num, dev,
						   	pci_expected_dev_list[bus_num - 1].vendor,
							pci_expected_dev_list[bus_num - 1].device);
				}
			} else {
				if (!boot_flag_post) {
					if (pci_flag) {
						printf(" Expected PCEI Device %s Not Found Bus_Num = %d,"
								" Device = %d, vendor_id = %x, device_id = %x\n",
								pci_expected_dev_list[bus_num - 1].name,
								bus_num, dev,
								pci_expected_dev_list[bus_num - 1].vendor,
								pci_expected_dev_list[bus_num - 1].device);
						pci_fail = -1;
					} else {
						pci_fail = -1;
					}
				}
			}
		}
	}

	/* Display the pci result for bus number 12 */
	for (bus_num = 12; bus_num < MAX_PCIE_SATA_CTRL_BUS; bus_num++) {
		if (!boot_flag_post) {
			printf("Link Training Status State Machine reg 0x%02x\n",
					pcie_result_store[bus_num][5]);
			if (pcie_result_store[bus_num][6] == 4) {
				printf("Correct link width trained :x%x\n",
						pcie_result_store[bus_num][6]);
			} else {
				printf("Link width trained :x%x (Expected :x4)\n",
						pcie_result_store[bus_num][6]);
			}
		}
		dev = 0;
		if (pcie_result_store[bus_num][0]) {
			if (!boot_flag_post) {
				printf(" Expected PCIE device %s Found Bus_Num = %d,"
						" Device = %d,"
						" Vendor_id  = %x, device_id = %x\n",
						pci_expected_dev_list[bus_num - 10].name,
						bus_num, dev,
						pci_expected_dev_list[bus_num - 10].vendor,
						pci_expected_dev_list[bus_num - 10].device);
			}
		} else {
			if (!boot_flag_post) {
				if (pci_flag) {
					printf(" Expected PCIE Device %s Not Found Bus_Num = %d,"
							" Device = %d,"
							" vendor_id = %x, device_id = %x\n",
							pci_expected_dev_list[bus_num - 10].name,
							bus_num, dev,
							pci_expected_dev_list[bus_num - 10].vendor,
							pci_expected_dev_list[bus_num - 10].device);

					pci_fail = -1;
				} else {
					pci_fail = -1;
				}
			}
		}
	}

	if (boot_flag_post) {
		if (pci_fail == -1) {
			post_result_pci = -1;
			return -1;
		}
		return 0;
	}

	if (!pci_fail) {
		printf( "\n" );
		printf(" --------------  PCI DIAG TEST PASSED ----------------\n");
		return 0;
	} else {
		printf( "\n" );
		printf(" --------------  PCI DIAG TEST FAILED ----------------\n");
		return -1;
	}
}

#endif /* CONFIG_POST & CFG_POST_PCI */
#endif /* CONFIG_POST */
#endif /* CONFIG_SRX3000 */
