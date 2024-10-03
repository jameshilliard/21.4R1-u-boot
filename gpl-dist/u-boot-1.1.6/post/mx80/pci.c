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
#ifdef CONFIG_MX80

#ifdef CONFIG_POST

#include <configs/mx80.h>
#include <command.h>
#include <asm/processor.h>
#include <asm/io.h>
#include <pci.h>
#include <pcie.h>
#include <post.h>
#if CONFIG_POST & CFG_POST_PCI

#define MAX_PCI_VENDOR_ID       0xFFFF
#define MIN_PCI_VENDOR_ID       0X0000

#define MAX_PCI_DEVICE              4
#define MAX_PCI_ENTRY               3
#define MAX_PCI_DEVICE_IDS          3


static  int pci_fail;
int boot_flag_post;
int post_result_pci;
extern struct pcie_controller pcie_hose;
static int pcie_result_store[MAX_PCI_DEVICE][MAX_PCI_ENTRY];

typedef struct _pci_vend_dev_list {
	unsigned int vendor;
	unsigned int device_ids[MAX_PCI_DEVICE_IDS];
	char *name;
} pci_ven_dev_list;

static pci_ven_dev_list  pci_expected_dev_list[] = {
	{0x1957, {0x40, 0x41}, "MPC8572E PCIE Controller"},
	{0x10b5, {0x8112}, "PEX8112 PCIe-to-PCI Bridge"},
	{0x1033, {0x0035}, "uPD720101/2 USB(OHCI) Controller"},
	{0x1033, {0x00e0}, "uPD720101/2 USB(EHCI) Controller"},
	{0, 0, ""},
};

int pci_post_test(int flags)
{
	int bus_num     = 0;
	int dev         = 0, i, j;
	pci_fail        = 0;
	boot_flag_post  = 0;
	post_result_pci = 0;

	if (flags & BOOT_POST_FLAG) {
		boot_flag_post = 1;
	}

	if (!boot_flag_post) {
		printf("Each expected device is being probed for\n");
	}

	for (i = 0; i < MAX_PCI_DEVICE; i++) {
		for (j = 0; j < MAX_PCI_DEVICE_IDS &&
			pci_expected_dev_list[i].device_ids[j] != 0; j++) {
			dev = pcie_find_device(pci_expected_dev_list[i].vendor,
				pci_expected_dev_list[i].device_ids[j],0);
			if (dev != -1) {
				pcie_result_store[i][0] = 1;
				break;
			}
		}
	}

	printf("\n\n");

	/* Display the pci result for bus number 0 & 1 */
	for (bus_num = 0; bus_num < MAX_PCI_DEVICE; bus_num++) {
		if (pcie_result_store[bus_num][0]) {
			if (!boot_flag_post) {
				printf(" Expected PCIE device %s Found\n",
						pci_expected_dev_list[bus_num].name);
			}
		} else {
			if (!boot_flag_post) {
				printf(" Expected PCIE Device %s Not Found\n",
							pci_expected_dev_list[bus_num].name);
			}
			pci_fail = -1;
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
		printf(" ------ All Expected devices found ------ \n");
		printf("PCI test completed, 1 pass, 0 errors\n");
		return 0;
	} else {
		printf( "\n" );
		printf(" ------ All Expected devices not found ------ \n");
		printf("PCI test failed, 0 pass, 1 errors\n");
		return -1;
	}
}

#endif /* CONFIG_POST & CFG_POST_PCI */
#endif /* CONFIG_POST */
#endif /* CONFIG_MX80 */
