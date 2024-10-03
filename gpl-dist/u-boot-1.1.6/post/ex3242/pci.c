/*
 * Copyright (c) 2007-2011, Juniper Networks, Inc.
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
#ifdef CONFIG_EX3242

#ifdef CONFIG_POST

#include <configs/ex3242.h>
#include <command.h>
#include <asm/processor.h>
#include <asm/io.h>
#include <pci.h>
#include <pcie.h>
#include <post.h>
#include <configs/ex3242.h>

#if CONFIG_POST & CFG_POST_PCI

#define ONE_CHEETAH_DEV   1
#define TWO_CHEETAH_DEV   2
#define THREE_CHEETAH_DEV 3
#define OTHER_CHEETAH_DEV 3
#define JAVA_LATTE_48     6
#define MAX_PCI_DEV_JAVA  5
#define MAX_PCIE_CONTROLLER 3
#define PCIE_CTRL_1 1
#define PCIE_CTRL_2 2
#define PCIE_CTRL_3 3

extern uint bd_id;
static  int pci_flag;
extern void pci_header_show(pci_dev_t dev);
extern  void pci_header_show_brief(pci_dev_t dev);
static int pci_unknow_dev_flag;
extern int board_id(void);
static int count_pci;
unsigned short pci_exp_list[10] = {0};
unsigned short pci_unknow_ven_id[255];
unsigned short pci_unknow_dev_id[255];
extern struct pcie_controller pcie_hose[];
int bus_num;
int cheetah_j ;
int boot_flag_post;
static int pcie_ctrl_1;
static int pcie_ctrl_2;
static int pcie_ctrl_3;
static int cheetah_ctrl_1;
static int cheetah_ctrl_2;
static int cheetah_ctrl_3;
int post_result_pci;
extern void lcd_printf_2(const char *fmt, ...);

struct pci_ids
{
	u16 venid;
	u16 devid;
	struct pci_ids *next;
};    
/* Post pci_dev_list struct declartion */
struct pci_dev_list
{
	u16 ven_id;
	u16 dev_id;
	char pass_str[20];
	u16 pass_count;
	struct pci_ids *next_list;
};

#define MAX_PCI_DEV 	5
#define VENDOR_ID_MAX 	0xFFFF
#define VENDOR_ID_MIN 	0x0000
#define PCI_VEN_ID  	0x1957 
#define PCI_DEV_ID  	0x32
#define CON_OHCI_VEN_ID 0x1131
#define CON_OHCI_DEV_ID 0x1561
#define EHCI_VEN_ID		0x1131
#define EHCI_DEV_ID 	0x1562
#define PD7_OHCI_VEN_ID 	0x1033
#define PD7_OHCI_DEV_ID 	0x0035
#define PD7_EHCI_VEN_ID 	0x1033
#define PD7_EHCI_DEV_ID 	0x00E0
#define CHEETAH_VEN_ID 	0x11AB
#define CHEETAH_DEV_ID 	0xD91C
#define CHEETAH_J_VEN_ID 	0x11AB
#define CHEETAH_J_DEV_ID 	0xDB00
#define CHEETAH_J_DEV_ID1 	0xDB01

struct pci_ids ohci_dev1 = {PD7_OHCI_VEN_ID, PD7_OHCI_DEV_ID, NULL};
struct pci_ids ehci_dev1 = {PD7_EHCI_VEN_ID, PD7_EHCI_DEV_ID, NULL};
struct pci_dev_list pci_expected_list_dev[MAX_PCI_DEV] = {
	{PCI_VEN_ID, PCI_DEV_ID, "PCI HOST", 0, NULL},
	{CON_OHCI_VEN_ID, CON_OHCI_DEV_ID, "OHCI Device", 0, &ohci_dev1},
	{EHCI_VEN_ID, EHCI_DEV_ID, "EHCI Device", 0, &ehci_dev1},
	{CHEETAH_VEN_ID, CHEETAH_DEV_ID, "CHEETAH Device", 0, NULL},
	{CHEETAH_J_VEN_ID, CHEETAH_J_DEV_ID, "CHEETAH_J Device", 0, NULL}};


void pcie_scan_rout(int controller,int bus_num)
{
	unsigned char header_type=0;
	unsigned short  vendor_id=0, device_id=0;
	int device;
	int i;
	int pass  = 0;
	int function = 0;
	pci_dev_t dev;

	for (device = 0; device < PCIE_MAX_PCIE_DEVICES; device++) {
		header_type = 0;
		vendor_id = 0;
		for (function = 0; function < PCIE_MAX_PCIE_FUNCTIONS; function++) {
			/* If this is not a multi-function device, we skip the rest. */
			if (function && !(header_type & 0x80))
				break;
			dev = PCIE_BDF(bus_num, device, function);
			pcie_hose_read_config_word(&pcie_hose[controller-1], dev, 
									   PCIE_VENDOR_ID, &vendor_id);
			if ((vendor_id == 0xFFFF) || (vendor_id == 0x0000))
				continue;

			if (!function)
				pcie_hose_read_config_byte(&pcie_hose[controller-1], dev, 
										   PCIE_HEADER_TYPE, &header_type);

			pcie_hose_read_config_word(&pcie_hose[controller-1], dev,
									   PCI_DEVICE_ID, &device_id);
			/* Based on bus_num scan for cheetah-j and pci devices. */

			if ((vendor_id == PCI_VEN_ID) && (device_id == PCI_DEV_ID)) {
				/* found pcie controller */
				if (controller == 1) {
					pcie_ctrl_1 = 1;
				}
				if (controller == 2) {
					pcie_ctrl_2 = 2;
				}
				if (controller == 3) {
					pcie_ctrl_3 = 3;
				}
			} 

			for (i = 4; i < MAX_PCI_DEV_JAVA; i++ ) {
				if ((vendor_id == pci_expected_list_dev[i].ven_id )) { 
					if ((device_id == pci_expected_list_dev[i].dev_id) && 
						(vendor_id == pci_expected_list_dev[i].ven_id )) { 
						pci_expected_list_dev[i].pass_count++;
						pass++;
						pci_exp_list[count_pci]= i;
						count_pci++;
					}
					/* 
					 * latte48i(48GE + 4HGS + 2uplink) can have two
					 * deviceid.Therefore checking two device ids
					 * DB00 is checked above. Below checks for DB01.
					 */
					if ((device_id == CHEETAH_J_DEV_ID1) && 
						(vendor_id == pci_expected_list_dev[i].ven_id )) { 
						pci_expected_list_dev[i].pass_count++;
						pass++;
						pci_exp_list[count_pci]= i;
						count_pci++;
						cheetah_j = 1;
					}
					if (controller == 1) {
						cheetah_ctrl_1 = 1;
					}
					if (controller == 2) {
						cheetah_ctrl_2 = 2;
					}
					if (controller == 3) {
						cheetah_ctrl_3 = 3;
					}
				}
			}
		}
	}
}



int pci_post_test (int flags)
{
	int device = 0;
	int function = 0;
	int pass  = 0;
	int known_dev = 0;
	int i = 0;
	int k = 0;
	int total_cheetah_dev = 0;
	unsigned char header_type = 0;
	unsigned short  vendor_id = 0, device_id = 0;
	int max_pci_dev_list = 0;
	int cheetah = 0;
	pci_dev_t dev;
	uint java_bd_id = 0;
	struct pci_ids * next_id;
	pci_flag = 0;
	pci_unknow_dev_flag = 0;
	uint latte48_deviceid = 0xDB01;
	int fail =0;
	unsigned char link;
	int controller = 0;
	int busNum = 0;
	int bus_num_cheetah_j = 1;
	pcie_ctrl_1 = 0;
	pcie_ctrl_2 = 0;
	pcie_ctrl_3 = 0;
	cheetah_ctrl_1 = 0;
	cheetah_ctrl_2 = 0;
	cheetah_ctrl_3 = 0;
	boot_flag_post = 0;

	cheetah_j = 0;
       count_pci = 0;
       post_result_pci = 0;
       
	if (flags & POST_DEBUG) {
		pci_flag =1;
	}	
	for (i = 0; i < 10; i++) {
              pci_exp_list[i] = 0;
		pci_expected_list_dev[i].pass_count =0;
	}
	if (bd_id == 0) {
		java_bd_id = board_id();
	}
	else {
		java_bd_id = bd_id;
	}

	switch (java_bd_id) {
	  case I2C_ID_JUNIPER_EX3200_24_T:
		  total_cheetah_dev = 1; 
		  max_pci_dev_list  = 4;
		  break;
	  case I2C_ID_JUNIPER_EX3200_24_P:
		  total_cheetah_dev = 1;
		  max_pci_dev_list  = 4;
		  break;
	  case I2C_ID_JUNIPER_EX3200_48_T:
		  total_cheetah_dev = 2;
		  max_pci_dev_list  = 5;
		  break;
	  case I2C_ID_JUNIPER_EX3200_48_P:
		  total_cheetah_dev = 2;
		  max_pci_dev_list  = 5;
		  break;
	  case I2C_ID_JUNIPER_EX4200_24_F:
		  total_cheetah_dev = 2;
		  max_pci_dev_list  = 5;
		  break;
	  case I2C_ID_JUNIPER_EX4200_24_T:
		  total_cheetah_dev = 2;
		  max_pci_dev_list  = 5;
		  break;
	  case I2C_ID_JUNIPER_EX4200_24_P:
	  case I2C_ID_JUNIPER_EX4210_24_P:
		  total_cheetah_dev = 2;
		  max_pci_dev_list  = 5;
		  break;
	  case I2C_ID_JUNIPER_EX4200_48_T:
		  total_cheetah_dev = 3;
		  max_pci_dev_list  = 6;
		  break;
	  case I2C_ID_JUNIPER_EX4200_48_P:
	  case I2C_ID_JUNIPER_EX4210_48_P:
		  total_cheetah_dev = 3;
		  max_pci_dev_list  = 6;
		  break;
	  default:
		  cheetah =1;
		  total_cheetah_dev = 2;
		  max_pci_dev_list  = 5;
		  break;
	};

	if (flags & BOOT_POST_FLAG) {
		boot_flag_post = 1;
	}

	for (device = 0; device < PCI_MAX_PCI_DEVICES; device++) {
		header_type = 0;
		vendor_id = 0;
		for (function = 0; function < PCI_MAX_PCI_FUNCTIONS; function++) {
			/*
			 * If this is not a multi-function device, we skip the rest.
			 */
			if (function && !(header_type & 0x80))
				break;

			dev = PCI_BDF(busNum, device, function);
			pci_read_config_word(dev, PCI_VENDOR_ID, &vendor_id);
			if ((vendor_id == VENDOR_ID_MAX ) || (vendor_id == VENDOR_ID_MIN))
				continue;

			if (!function)
				pci_read_config_byte(dev, PCI_HEADER_TYPE, &header_type);

			pci_read_config_word(dev, PCI_DEVICE_ID, &device_id);
			if (vendor_id != 0 && device_id != 0) {
				known_dev = 0;
				for (i = 0; i < MAX_PCI_DEV_JAVA-1; i++) {
						if ((device_id == pci_expected_list_dev[i].dev_id) && 
							(vendor_id == pci_expected_list_dev[i].ven_id )) { 
							known_dev = 1;
							pci_expected_list_dev[i].pass_count++;
							pass++;
							pci_exp_list[count_pci]= i;
							count_pci++;
						}
						else {
							next_id = pci_expected_list_dev[i].next_list;
							while (next_id != NULL) {
								if ((device_id == next_id->devid)  && 
									(vendor_id == next_id->venid )) {
									known_dev = 1;
									pci_expected_list_dev[i].pass_count++;
									pass++;
									pci_exp_list[count_pci] = i;
									count_pci++;
									break;
								}
								else {
									next_id = next_id->next;
								}
							}
						}    
				}
				if (!known_dev) {
					pci_unknow_dev_id[pci_unknow_dev_flag] = device_id;
					pci_unknow_ven_id[pci_unknow_dev_flag] = vendor_id;
					pci_unknow_dev_flag++;
				}
				vendor_id =0;
				device_id =0;
			}
		}
	}

	/* checking link status for cheetah-j */
	if (( java_bd_id == I2C_ID_JUNIPER_EX3200_24_P) ||
		( java_bd_id == I2C_ID_JUNIPER_EX3200_24_T)) {
		controller =1;
		for (bus_num = 0; bus_num < 1;  bus_num++) {
			for (i = 1;  i <= MAX_PCIE_CONTROLLER;  i++) {
				pcie_scan_rout(i,bus_num);
			}
		}
		for (bus_num = 1; bus_num < 2; bus_num++) {
			for (i = 1;  i <= controller;  i++) {
				pcie_hose_read_config_byte(&pcie_hose[i-1], 0, 0x404, &link);
				/* if link is up scan for cheetah-j devices. */
				if (link != 0x16) {  /* probe bus 1 */
				}
				else {
					pcie_scan_rout(i,bus_num);
				}
			}
		}
	}
	else if (( java_bd_id == I2C_ID_JUNIPER_EX3200_48_T)  ||
			 ( java_bd_id == I2C_ID_JUNIPER_EX3200_48_P)  ||
			 ( java_bd_id == I2C_ID_JUNIPER_EX4200_24_F)  ||
			 ( java_bd_id == I2C_ID_JUNIPER_EX4200_24_T)  ||
			 ( java_bd_id == I2C_ID_JUNIPER_EX4200_24_P)  ||
			 ( java_bd_id == I2C_ID_JUNIPER_EX4210_24_P)) {
		for (bus_num = 0; bus_num < 1;  bus_num++) {
			for (i = 1;  i <= MAX_PCIE_CONTROLLER;  i++) {
				pcie_scan_rout(i,bus_num);
			}
		}
		for (bus_num = 1; bus_num < 2; bus_num++) {
			i = 1;  /* first controller; */
			pcie_hose_read_config_byte(&pcie_hose[i-1], 0, 0x404, &link);
			/* if link is up scan for cheetah-j devices. */
			if (link != 0x16) {  /* probe bus 1 */
			}
			else {
				pcie_scan_rout(i,bus_num);
			}
			i = 3;  /* Third controller; */
			pcie_hose_read_config_byte(&pcie_hose[i-1], 0, 0x404, &link);
			/* if link is up scan for cheetah-j devices. */
			if (link != 0x16) {  /* probe bus 1 */
			}
			else {
				pcie_scan_rout(i,bus_num);
			}
		}
	}
	else if (( java_bd_id == I2C_ID_JUNIPER_EX4200_48_T ) ||
			 ( java_bd_id == I2C_ID_JUNIPER_EX4200_48_P ) ||
			 ( java_bd_id == I2C_ID_JUNIPER_EX4210_48_P )) {
		for (bus_num = 0; bus_num < 1;  bus_num++) {
			for (i = 1;  i <= MAX_PCIE_CONTROLLER;  i++) {
				pcie_scan_rout(i,bus_num);
			}
		}
		for (bus_num = 1; bus_num < 2; bus_num++) {
			i = 1;  /* first controller; */
			pcie_hose_read_config_byte(&pcie_hose[i-1], 0, 0x404, &link);
			/* if link is up scan for cheetah-j devices. */
			if (link != 0x16) {  /* probe bus 1 */
			}
			else {
				pcie_scan_rout(i,bus_num);
			}
			i = 2;  /* 2nd controller; */
			pcie_hose_read_config_byte(&pcie_hose[i-1], 0, 0x404, &link);
			/* if link is up scan for cheetah-j devices. */
			if (link != 0x16) {  /* probe bus 1 */
			}
			else {
				pcie_scan_rout(i,bus_num);
			}
			i = 3;  /* Third controller; */
			pcie_hose_read_config_byte(&pcie_hose[i-1], 0, 0x404, &link);
			/* if link is up scan for cheetah-j devices. */
			if (link != 0x16) {  /* probe bus 1 */
			}
			else {
				pcie_scan_rout(i,bus_num);
			}
		}
	}



	/* To display the result over all pass or fail this check code is must. */
	for (i = 0; i < MAX_PCI_DEV_JAVA - 2; i++) {
		if (pci_expected_list_dev[i].pass_count == 0) {
			fail = -1;
		}
	}

	for (i = 3; i < MAX_PCI_DEV_JAVA - 1; i++) {
		if (cheetah == 1) {
			if (pci_expected_list_dev[i].pass_count == 0) {
				fail = -1;
			}
			if (pci_expected_list_dev[i].pass_count == 1) {
				fail = -1;
			}
		}
		i++; /*  i is incremented to check for cheetah-j  */

		if (( java_bd_id == I2C_ID_JUNIPER_EX3200_24_P) ||
			( java_bd_id == I2C_ID_JUNIPER_EX3200_24_T)) {
			if (pci_expected_list_dev[i].pass_count == 0) {
				fail = -1;
			}
		}

		if (( java_bd_id == I2C_ID_JUNIPER_EX3200_48_T)  ||
			( java_bd_id == I2C_ID_JUNIPER_EX3200_48_P)  ||
			( java_bd_id == I2C_ID_JUNIPER_EX4200_24_F)  ||
			( java_bd_id == I2C_ID_JUNIPER_EX4200_24_T)  ||
			( java_bd_id == I2C_ID_JUNIPER_EX4200_24_P)  ||
			( java_bd_id == I2C_ID_JUNIPER_EX4210_24_P)) {
			for (k = 0; k < 2; k++) {
				if (pci_expected_list_dev[i].pass_count == k) {
					fail = -1;
				}
			}
		}

		if (( java_bd_id == I2C_ID_JUNIPER_EX4200_48_T ) ||
			( java_bd_id == I2C_ID_JUNIPER_EX4200_48_P ) ||
			( java_bd_id == I2C_ID_JUNIPER_EX4200_48_P )) {
			for (k = 0; k < 3; k++) {
				if (pci_expected_list_dev[i].pass_count == k) {
					fail = -1;
				}
			}
		}
		i++; /* end of checking the each result */
	}
       if (boot_flag_post) {
           if (fail == 0) {
               lcd_printf_2("POST PCI pass..");
               return 0;
           }
           else {
               lcd_printf_2("POST PCI fail..");
		 post_result_pci = -1;
               return -1;
           }
       }
	/* This code displays the pci result based on the pci device detection. */
	if (fail == 0) {
		if (!pci_flag){
			post_log("-- PCI/PCIe test                        "
                                "PASSED\n");
		}
	}
	else {
		post_log("-- PCI/PCIe test                        FAILED @\n");
	}

	if (pass) {
		if (pci_flag) {
			if (fail == 0) {
				post_log("-- PCI/PCIe test"
                                        "                        PASSED\n");
			} 
		}
		for (i = 0; i < MAX_PCI_DEV_JAVA - 2; i++) {
			if (pci_flag) {
				if (pci_expected_list_dev[i].pass_count == 1) {
					post_log("Expected Device Found (%s) ---"
							 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
							 " = 0x%x \n",pci_expected_list_dev[i].pass_str,
							 busNum, pci_expected_list_dev[i].ven_id,
							 pci_expected_list_dev[i].dev_id);
				}
			}
			post_log("\n");
		}
		for ( i = 3; i < MAX_PCI_DEV_JAVA -1; i++) {
			if (pci_flag) {
				if (cheetah == 1 ) {
					/* if = 3 display cheetah devices */
					if (pci_expected_list_dev[i].pass_count == 1) {
						post_log("Expected Device Found (%s) ---"
								 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
								 " = 0x%x \n\n",pci_expected_list_dev[i].pass_str,
								 busNum, pci_expected_list_dev[i].ven_id,
								 pci_expected_list_dev[i].dev_id);
					}
					if (pci_expected_list_dev[i].pass_count == 2 ) {
						for (k = 0;  k < 2; k++) {
							post_log("Expected Device Found (%s) ---"
									 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
									 " = 0x%x \n\n",pci_expected_list_dev[i].pass_str,
									 busNum, pci_expected_list_dev[i].ven_id,
									 pci_expected_list_dev[i].dev_id);
						}
					}
				}
				else {
					i++;  /* i is  incremented to display cheetah-j */
					/* if = 4 display cheetah-j devices */
					if (( java_bd_id == I2C_ID_JUNIPER_EX3200_24_P) ||
						( java_bd_id == I2C_ID_JUNIPER_EX3200_24_T))	{
						if (pci_expected_list_dev[i].pass_count == 1) {
							if ( pcie_ctrl_1 == 1) {	
								post_log("Expected Device Found (PCIe %d HOST) ---"
										 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
										 " = 0x%x \n\n",pcie_ctrl_1,busNum,PCI_VEN_ID,PCI_DEV_ID);
							}
							else {
								post_log("Expected Device Not Found (PCIe %d HOST) ---"
										 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
										 " = 0x%x \n\n",PCIE_CTRL_1,busNum,PCI_VEN_ID,PCI_DEV_ID);
							}
							post_log("Expected Device Found (%s) ---"
									 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
									 " = 0x%x \n\n",pci_expected_list_dev[i].pass_str,
									 bus_num_cheetah_j, pci_expected_list_dev[i].ven_id,
									 pci_expected_list_dev[i].dev_id);
							if ( pcie_ctrl_2 == 2) {	
								post_log("Expected Device Found (PCIe %d HOST) ---"
										 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
										 " = 0x%x \n\n",pcie_ctrl_2,busNum,PCI_VEN_ID,PCI_DEV_ID);
							}
							else {
								post_log("Expected Device Not Found (PCIe %d HOST) ---"
										 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
										 " = 0x%x \n\n",PCIE_CTRL_2,busNum,PCI_VEN_ID,PCI_DEV_ID);
							}
							if ( pcie_ctrl_3 == 3) {	
								post_log("Expected Device Found (PCIe %d HOST) ---"
										 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
										 " = 0x%x \n\n",pcie_ctrl_3,busNum,PCI_VEN_ID,PCI_DEV_ID);
							}
							else {
								post_log("Expected Device Not Found (PCIe %d HOST) ---"
										 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
										 " = 0x%x \n\n",PCIE_CTRL_3,busNum,PCI_VEN_ID,PCI_DEV_ID);
							}
						}
					}
					if (( java_bd_id == I2C_ID_JUNIPER_EX3200_48_T)  ||
						( java_bd_id == I2C_ID_JUNIPER_EX3200_48_P)  ||
						( java_bd_id == I2C_ID_JUNIPER_EX4200_24_F)  ||
						( java_bd_id == I2C_ID_JUNIPER_EX4200_24_T)  ||
						( java_bd_id == I2C_ID_JUNIPER_EX4200_24_P)  ||
						( java_bd_id == I2C_ID_JUNIPER_EX4210_24_P))	 {
						if (pci_expected_list_dev[i].pass_count == 1) {
							if ( pcie_ctrl_1 == 1) {	
								post_log("Expected Device Found (PCIe %d HOST) ---"
										 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
										 " = 0x%x \n\n",pcie_ctrl_1,busNum,PCI_VEN_ID,PCI_DEV_ID);
							}
							else {
								post_log("Expected Device Not Found (PCIe %d HOST) ---"
										 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
										 " = 0x%x \n\n",PCIE_CTRL_1,busNum,PCI_VEN_ID,PCI_DEV_ID);

							}
							if (pcie_ctrl_2 == 2) {	
								post_log("Expected Device Found (PCIe %d HOST) ---"
										 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
										 " = 0x%x \n\n",pcie_ctrl_2,busNum,PCI_VEN_ID,PCI_DEV_ID);
							}
							else {
								post_log("Expected Device Not Found (PCIe %d HOST) ---"
										 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
										 " = 0x%x \n\n",PCIE_CTRL_2,busNum,PCI_VEN_ID,PCI_DEV_ID);
							}
							if (pcie_ctrl_3 == 3) {	
								post_log("Expected Device Found (PCIe %d HOST) ---"
										 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
										 " = 0x%x \n\n",pcie_ctrl_3,busNum,PCI_VEN_ID,PCI_DEV_ID);
							}
							else {
								post_log("Expected Device Not Found (PCIe %d HOST) ---"
										 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
										 " = 0x%x \n\n",PCIE_CTRL_3,busNum,PCI_VEN_ID,PCI_DEV_ID);
							}
							post_log("Expected Device Found (%s) ---"
									 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
									 " = 0x%x \n\n",pci_expected_list_dev[i].pass_str,
									 bus_num_cheetah_j, pci_expected_list_dev[i].ven_id,
									 pci_expected_list_dev[i].dev_id);
						}
						if (pci_expected_list_dev[i].pass_count == 2) {
							for (k = 0;  k < 2; k++) {
								if (k == 0) {
									if (pcie_ctrl_1 == 1) {	
										post_log("Expected Device Found (PCIe %d HOST) ---"
												 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
												 " = 0x%x \n\n",pcie_ctrl_1,busNum,PCI_VEN_ID,PCI_DEV_ID);
									}
									else {
										post_log("Expected Device Not Found (PCIe %d HOST) ---"
												 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
												 " = 0x%x \n\n",PCIE_CTRL_1,busNum,PCI_VEN_ID,PCI_DEV_ID);
									}
								}
								if (k == 1) {
									if (pcie_ctrl_2 == 2) {	
										post_log("Expected Device Found (PCIe %d HOST) ---"
												 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
												 " = 0x%x \n\n",pcie_ctrl_2,busNum,PCI_VEN_ID,PCI_DEV_ID);
									}
									else {
										post_log("Expected Device Not Found (PCIe %d HOST) ---"
												 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
												 " = 0x%x \n\n",PCIE_CTRL_2,busNum,PCI_VEN_ID,PCI_DEV_ID);
									}
									if (pcie_ctrl_3 == 3) {	
										post_log("Expected Device Found (PCIe %d HOST) ---"
												 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
												 " = 0x%x \n\n",pcie_ctrl_3,busNum,PCI_VEN_ID,PCI_DEV_ID);
									}
									else {
										post_log("Expected Device Not Found (PCIe %d HOST) ---"
												 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
												 " = 0x%x \n\n",PCIE_CTRL_3,busNum,PCI_VEN_ID,PCI_DEV_ID);
									}
								}
								post_log("Expected Device Found (%s) ---"
										 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
										 " = 0x%x \n\n",pci_expected_list_dev[i].pass_str,
										 bus_num_cheetah_j, pci_expected_list_dev[i].ven_id,
										 pci_expected_list_dev[i].dev_id);
							}
						}
					}
					if (( java_bd_id == I2C_ID_JUNIPER_EX4200_48_T ) ||
						( java_bd_id == I2C_ID_JUNIPER_EX4200_48_P ) ||
						( java_bd_id == I2C_ID_JUNIPER_EX4210_48_P )) {
						if (pci_expected_list_dev[i].pass_count == 3) {
							if (cheetah_j != 1 ) {
								for (k = 0; k < 3; k++ ) {

									if (k == 0) {
										if( pcie_ctrl_1 == 1) {	
											post_log("Expected Device Found (PCIe %d HOST) ---"
													 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
													 " = 0x%x \n\n",pcie_ctrl_1,busNum,PCI_VEN_ID,PCI_DEV_ID);
										}
										else {
											post_log("Expected Device Not Found (PCIe %d HOST) ---"
													 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
													 " = 0x%x \n\n",PCIE_CTRL_1,busNum,PCI_VEN_ID,PCI_DEV_ID);
										}
									}
									if (k == 1) {
										if (pcie_ctrl_2 == 2) {	
											post_log("Expected Device Found (PCIe %d HOST) ---"
													 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
													 " = 0x%x \n\n",pcie_ctrl_2,busNum,PCI_VEN_ID,PCI_DEV_ID);
										}
										else {
											post_log("Expected Device Not Found (PCIe %d HOST) ---"
													 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
													 " = 0x%x \n\n",PCIE_CTRL_2,busNum,PCI_VEN_ID,PCI_DEV_ID);
										}
									}
									if (k == 2) {
										if (pcie_ctrl_3 == 3) {	
											post_log("Expected Device Found (PCIe %d HOST) ---"
													 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
													 " = 0x%x \n\n",pcie_ctrl_3,busNum,PCI_VEN_ID,PCI_DEV_ID);
										}
										else {
											post_log("Expected Device Not Found (PCIe %d HOST) ---"
													 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
													 " = 0x%x \n\n",PCIE_CTRL_3,busNum,PCI_VEN_ID,PCI_DEV_ID);
										}
									}

									post_log("Expected Device Found (%s) ---"
											 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
											 " = 0x%x \n\n",pci_expected_list_dev[i].pass_str,
											 bus_num_cheetah_j, pci_expected_list_dev[i].ven_id,
											 pci_expected_list_dev[i].dev_id);
								}
							}
							else {
								for (k = 0; k < 3; k++) {
									if (k == 0) {
										if (pcie_ctrl_1 == 1) {	
											post_log("Expected Device Found (PCIe %d HOST) ---"
													 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
													 " = 0x%x \n\n",pcie_ctrl_1,busNum,PCI_VEN_ID,PCI_DEV_ID);
										}
										else {
											post_log("Expected Device Not Found (PCIe %d HOST) ---"
													 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
													 " = 0x%x \n\n",PCIE_CTRL_1,busNum,PCI_VEN_ID,PCI_DEV_ID);
										}
									}
									if (k == 1) {
										if (pcie_ctrl_2 == 2) {	
											post_log("Expected Device Found (PCIe %d HOST) ---"
													 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
													 " = 0x%x \n\n",pcie_ctrl_2,busNum,PCI_VEN_ID,PCI_DEV_ID);
										}
										else {
											post_log("Expected Device Not Found (PCIe %d HOST) ---"
													 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
													 " = 0x%x \n\n",PCIE_CTRL_2,busNum,PCI_VEN_ID,PCI_DEV_ID);
										}
									}
									if (k == 2) {
										if (pcie_ctrl_3 == 3) {	
											post_log("Expected Device Found (PCIe %d HOST) ---"
													 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
													 " = 0x%x \n\n",pcie_ctrl_3,busNum,PCI_VEN_ID,PCI_DEV_ID);
										}
										else {
											post_log("Expected Device Not Found (PCIe %d HOST) ---"
													 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
													 " = 0x%x \n\n",PCIE_CTRL_3,busNum,PCI_VEN_ID,PCI_DEV_ID);
										}
									}
									post_log("Expected Device Found (%s) ---"
											 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
											 " = 0x%x \n\n",pci_expected_list_dev[i].pass_str,
											 bus_num_cheetah_j, pci_expected_list_dev[i].ven_id,
											 latte48_deviceid);
								}
							}
						}
						if (pci_expected_list_dev[i].pass_count == 2 ||
						   pci_expected_list_dev[i].pass_count == 1) {

							if (cheetah_ctrl_1 == 1) {
								if (pcie_ctrl_1 == 1) {	
									post_log("Expected Device Found (PCIe %d HOST) ---"
											 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
											 " = 0x%x \n\n",pcie_ctrl_1,busNum,PCI_VEN_ID,PCI_DEV_ID);
								}
								else {
									post_log("Expected Device Not Found (PCIe %d HOST) ---"
											 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
											 " = 0x%x \n\n",PCIE_CTRL_1,busNum,PCI_VEN_ID,PCI_DEV_ID);

								}
								post_log("Expected Device Found (%s) ---"
										 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
										 " = 0x%x \n\n",pci_expected_list_dev[i].pass_str,
										 bus_num_cheetah_j, pci_expected_list_dev[i].ven_id,
										 pci_expected_list_dev[i].dev_id);
							}
							if (cheetah_ctrl_2 == 2 ) {
								if (pcie_ctrl_2 == 2) {	
									post_log("Expected Device Found (PCIe %d HOST) ---"
											 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
											 " = 0x%x \n\n",pcie_ctrl_2,busNum,PCI_VEN_ID,PCI_DEV_ID);
								}
								else {
									post_log("Expected Device Not Found (PCIe %d HOST) ---"
											 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
											 " = 0x%x \n\n",PCIE_CTRL_2,busNum,PCI_VEN_ID,PCI_DEV_ID);
								}
								post_log("Expected Device Found (%s) ---"
										 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
										 " = 0x%x \n\n",pci_expected_list_dev[i].pass_str,
										 bus_num_cheetah_j, pci_expected_list_dev[i].ven_id,
										 pci_expected_list_dev[i].dev_id);
							}
							if (cheetah_ctrl_3 == 3) {
								if (pcie_ctrl_3 == 3) {	
									post_log("Expected Device Found (PCIe %d HOST) ---"
											 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
											 " = 0x%x \n\n",pcie_ctrl_3,busNum,PCI_VEN_ID,PCI_DEV_ID);
								}
								else {
									post_log("Expected Device Not Found (PCIe %d HOST) ---"
											 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
											 " = 0x%x \n\n",PCIE_CTRL_3,busNum,PCI_VEN_ID,PCI_DEV_ID);
								}
								post_log("Expected Device Found (%s) ---"
										 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
										 " = 0x%x \n\n",pci_expected_list_dev[i].pass_str,
										 bus_num_cheetah_j, pci_expected_list_dev[i].ven_id,
										 pci_expected_list_dev[i].dev_id);
							}
						}
					}
				}
			}
			i++;  /* i is  incremented display is completed */
		}
	}

	/* Displays unknown pci devices */
	if (pci_unknow_dev_flag) { 
		for (i = 0; i < pci_unknow_dev_flag; i++) {
			post_log("UNKNOWN device found ---"
					 " busNum = 0x%x --  vendor-id = 0x%x -- device-id" 
					 " = 0x%x \n\n", 
					 busNum,pci_unknow_ven_id[i],
					 pci_unknow_dev_id[i]);
		}
	}

	/* Print the missed expected pci devices other then cheetah devices */
	for (i = 0; i < MAX_PCI_DEV_JAVA - 2; i++) {
		if (pci_expected_list_dev[i].pass_count == 0) {
			post_log("Expected Device Not Found (%s) !!!"
					 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
					 " = 0x%x \n\n",pci_expected_list_dev[i].pass_str,
					 busNum, pci_expected_list_dev[i].ven_id, pci_expected_list_dev[i].dev_id);
		}
	}

	/* Print the missed expected pci devices  cheetah and cheetah-J devices */
	for ( i =3; i < MAX_PCI_DEV_JAVA -1; i++) {
		if (cheetah == 1) {
			/* if = 3 display cheetah devices */
			if (pci_expected_list_dev[i].pass_count == 0) {
				for (k = 0; k < 2; k++) {		
					post_log("Expected Device Not Found (%s) ---"
							 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
							 " = 0x%x \n\n",pci_expected_list_dev[i].pass_str,
							 busNum, pci_expected_list_dev[i].ven_id,
							 pci_expected_list_dev[i].dev_id);
				}
			}
			if (pci_expected_list_dev[i].pass_count == 1) {
				post_log("Expected Device Not Found (%s) ---"
						 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
						 " = 0x%x \n\n",pci_expected_list_dev[i].pass_str,
						 busNum, pci_expected_list_dev[i].ven_id,
						 pci_expected_list_dev[i].dev_id);
			}
		}
		else { 

			i++;  /* i is  incremented to display cheetah-j */
			/* if = 4 display cheetah-j devices */
			if ((java_bd_id == I2C_ID_JUNIPER_EX3200_24_P) ||
				(java_bd_id == I2C_ID_JUNIPER_EX3200_24_T))	{

				if (pci_expected_list_dev[i].pass_count == 0) {
					if (pcie_ctrl_1 == 1) {	
						if (pci_flag) {
							post_log("Expected Device Found (PCIe %d HOST) ---"
									 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
									 " = 0x%x \n\n",pcie_ctrl_1,busNum,PCI_VEN_ID,PCI_DEV_ID);
						}
					}
					else {
						post_log("Expected Device Not Found (PCIe %d HOST) ---"
								 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
								 " = 0x%x \n\n",PCIE_CTRL_1,busNum,PCI_VEN_ID,PCI_DEV_ID);
					}
					post_log("Expected Device Not Found (%s) ---"
							 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
							 " = 0x%x \n\n",pci_expected_list_dev[i].pass_str,
							 bus_num_cheetah_j, pci_expected_list_dev[i].ven_id,
							 pci_expected_list_dev[i].dev_id);
				}
				if (pcie_ctrl_2 == 2) {	
					if (pci_flag) {
						post_log("Expected Device Found (PCIe %d HOST) ---"
								 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
								 " = 0x%x \n\n",pcie_ctrl_2,busNum,PCI_VEN_ID,PCI_DEV_ID);
					}
				}
				else {
					post_log("Expected Device Not Found (PCIe %d HOST) ---"
							 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
							 " = 0x%x \n\n",PCIE_CTRL_2,busNum,PCI_VEN_ID,PCI_DEV_ID);
				}
				if (pcie_ctrl_3 == 3) {	
					if (pci_flag) {
						post_log("Expected Device Found (PCIe %d HOST) ---"
								 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
								 " = 0x%x \n\n",pcie_ctrl_3,busNum,PCI_VEN_ID,PCI_DEV_ID);
					}
				}
				else {
					post_log("Expected Device Not Found (PCIe %d HOST) ---"
							 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
							 " = 0x%x \n\n",PCIE_CTRL_3,busNum,PCI_VEN_ID,PCI_DEV_ID);
				}
			}

			if ((java_bd_id == I2C_ID_JUNIPER_EX3200_48_T)  ||
				(java_bd_id == I2C_ID_JUNIPER_EX3200_48_P)  ||
				(java_bd_id == I2C_ID_JUNIPER_EX4200_24_F)  ||
				(java_bd_id == I2C_ID_JUNIPER_EX4200_24_T)  ||
				(java_bd_id == I2C_ID_JUNIPER_EX4200_24_P)  ||
				(java_bd_id == I2C_ID_JUNIPER_EX4210_24_P))	 {
				if (pci_expected_list_dev[i].pass_count == 0) {
					for (k = 0; k < 2; k++ ) {

						if (k == 0) {
							if (pcie_ctrl_1 == 1) {	
								if (pci_flag) {
									post_log("Expected Device Found (PCIe %d HOST) ---"
											 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
											 " = 0x%x \n\n",pcie_ctrl_1,busNum,PCI_VEN_ID,PCI_DEV_ID);
								}
							}
							else {
								post_log("Expected Device Not Found (PCIe %d HOST) ---"
										 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
										 " = 0x%x \n\n",PCIE_CTRL_1,busNum,PCI_VEN_ID,PCI_DEV_ID);
							}
						}
						if (k == 1) {
							if (pcie_ctrl_2 == 2) {	
								if (pci_flag) {
									post_log("Expected Device Found (PCIe %d HOST) ---"
											 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
											 " = 0x%x \n\n",pcie_ctrl_2,busNum,PCI_VEN_ID,PCI_DEV_ID);
								}
							}
							else {
								post_log("Expected Device Not Found (PCIe %d HOST) ---"
										 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
										 " = 0x%x \n\n",PCIE_CTRL_2,busNum,PCI_VEN_ID,PCI_DEV_ID);
							}
							if (pcie_ctrl_3 == 3) {	
								if (pci_flag) {
									post_log("Expected Device Found (PCIe %d HOST) ---"
											 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
											 " = 0x%x \n\n",pcie_ctrl_3,busNum,PCI_VEN_ID,PCI_DEV_ID);
								}
							}
							else {
								post_log("Expected Device Not Found (PCIe %d HOST) ---"
										 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
										 " = 0x%x \n\n",PCIE_CTRL_3,busNum,PCI_VEN_ID,PCI_DEV_ID);
							}
						}
						post_log("Expected Device Not Found (%s) ---"
								 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
								 " = 0x%x \n\n",pci_expected_list_dev[i].pass_str,
								 bus_num_cheetah_j, pci_expected_list_dev[i].ven_id,
								 pci_expected_list_dev[i].dev_id);
					}
				}
				if (pci_expected_list_dev[i].pass_count == 1) {
					if (pcie_ctrl_1 == 1) {	
						if (pci_flag) {
							post_log("Expected Device Found (PCIe %d HOST) ---"
									 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
									 " = 0x%x \n\n",pcie_ctrl_1,busNum,PCI_VEN_ID,PCI_DEV_ID);
						}
					}
					else {
						post_log("Expected Device Not Found (PCIe %d HOST) ---"
								 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
								 " = 0x%x \n\n",PCIE_CTRL_1,busNum,PCI_VEN_ID,PCI_DEV_ID);

					}
					if (pcie_ctrl_3 == 3) {	
						if (pci_flag) {
							post_log("Expected Device Found (PCIe %d HOST) ---"
									 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
									 " = 0x%x \n\n",pcie_ctrl_3,busNum,PCI_VEN_ID,PCI_DEV_ID);
						}
					}
					else {
						post_log("Expected Device Not Found (PCIe %d HOST) ---"
								 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
								 " = 0x%x \n\n",PCIE_CTRL_3,busNum,PCI_VEN_ID,PCI_DEV_ID);
					}
					post_log("Expected Device Not Found (%s) ---"
							 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
							 " = 0x%x \n\n",pci_expected_list_dev[i].pass_str,
							 bus_num_cheetah_j, pci_expected_list_dev[i].ven_id,
							 pci_expected_list_dev[i].dev_id);
					if (pcie_ctrl_2 == 2) {	
						if (pci_flag) {
							post_log("Expected Device Found (PCIe %d HOST) ---"
									 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
									 " = 0x%x \n\n",pcie_ctrl_2,busNum,PCI_VEN_ID,PCI_DEV_ID);
						}
					}
					else {
						post_log("Expected Device Not Found (PCIe %d HOST) ---"
								 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
								 " = 0x%x \n\n",PCIE_CTRL_2,busNum,PCI_VEN_ID,PCI_DEV_ID);
					}
				}
			}
			if (( java_bd_id == I2C_ID_JUNIPER_EX4200_48_T ) ||
				( java_bd_id == I2C_ID_JUNIPER_EX4200_48_P ) ||
				( java_bd_id == I2C_ID_JUNIPER_EX4210_48_P )) {
				if (pci_expected_list_dev[i].pass_count == 0) {
					for (k = 0; k < 3; k++) {
						if (cheetah_j != 1) {
							if (k == 0 ) {
								if (pcie_ctrl_1 == 1) {	
									if (pci_flag) {
										post_log("Expected Device Found (PCIe %d HOST) ---"
												 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
												 " = 0x%x \n\n",pcie_ctrl_1,busNum,PCI_VEN_ID,PCI_DEV_ID);
									}
								}
								else {
									post_log("Expected Device Not Found (PCIe %d HOST) ---"
											 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
											 " = 0x%x \n\n",PCIE_CTRL_1,busNum,PCI_VEN_ID,PCI_DEV_ID);
								}
							}
							if (k == 1 ) {
								if (pcie_ctrl_2 == 2) {	
									if (pci_flag) {
										post_log("Expected Device Found (PCIe %d HOST) ---"
												 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
												 " = 0x%x \n\n",pcie_ctrl_2,busNum,PCI_VEN_ID,PCI_DEV_ID);
									}
								}
								else {
									post_log("Expected Device Not Found (PCIe %d HOST) ---"
											 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
											 " = 0x%x \n\n",PCIE_CTRL_2,busNum,PCI_VEN_ID,PCI_DEV_ID);
								}
							}
							if (k == 2) {
								if (pcie_ctrl_3 == 3) {	
									if (pci_flag) {
										post_log("Expected Device Found (PCIe %d HOST) ---"
												 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
												 " = 0x%x \n\n",pcie_ctrl_3,busNum,PCI_VEN_ID,PCI_DEV_ID);
									}
								}
								else {
									post_log("Expected Device Not Found (PCIe %d HOST) ---"
											 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
											 " = 0x%x \n\n",PCIE_CTRL_3,busNum,PCI_VEN_ID,PCI_DEV_ID);
								}
							}
							post_log("Expected Device Not Found (%s) ---"
									 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
									 " = 0x%x \n\n",pci_expected_list_dev[i].pass_str,
									 bus_num_cheetah_j, pci_expected_list_dev[i].ven_id,
									 pci_expected_list_dev[i].dev_id);
						}
						else {
							if (k == 0) {
								if (pcie_ctrl_1 == 1) {	
									if (pci_flag) {
										post_log("Expected Device Found (PCIe %d HOST) ---"
												 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
												 " = 0x%x \n\n",pcie_ctrl_1,busNum,PCI_VEN_ID,PCI_DEV_ID);
									}
								}
								else {
									post_log("Expected Device Not Found (PCIe %d HOST) ---"
											 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
											 " = 0x%x \n\n",PCIE_CTRL_1,busNum,PCI_VEN_ID,PCI_DEV_ID);
								}
							}
							if (k == 1) {
								if (pcie_ctrl_2 == 2) {	
									if (pci_flag) {
										post_log("Expected Device Found (PCIe %d HOST) ---"
												 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
												 " = 0x%x \n\n",pcie_ctrl_2,busNum,PCI_VEN_ID,PCI_DEV_ID);
									}
								}
								else {
									post_log("Expected Device Not Found (PCIe %d HOST) ---"
											 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
											 " = 0x%x \n\n",PCIE_CTRL_2,busNum,PCI_VEN_ID,PCI_DEV_ID);
								}
							}
							if (k == 2) {
								if (pcie_ctrl_3 == 3) {	
									if (pci_flag) {
										post_log("Expected Device Found (PCIe %d HOST) ---"
												 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
												 " = 0x%x \n\n",pcie_ctrl_3,busNum,PCI_VEN_ID,PCI_DEV_ID);
									}
								}
								else {
									post_log("Expected Device Not Found (PCIe %d HOST) ---"
											 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
											 " = 0x%x \n\n",PCIE_CTRL_3,busNum,PCI_VEN_ID,PCI_DEV_ID);
								}
							}
							post_log("Expected Device Not Found (%s) ---"
									 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
									 " = 0x%x \n\n",pci_expected_list_dev[i].pass_str,
									 bus_num_cheetah_j, pci_expected_list_dev[i].ven_id,
									 latte48_deviceid);
						}
					}
				}
				if (pci_expected_list_dev[i].pass_count == 2 ||
				   pci_expected_list_dev[i].pass_count == 1 ) {

					if (cheetah_ctrl_1 == 1) {
						if (pcie_ctrl_1 == 1) {	
							if (pci_flag) {
								post_log("Expected Device Found (PCIe %d HOST) ---"
										 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
										 " = 0x%x \n\n",pcie_ctrl_1,busNum,PCI_VEN_ID,PCI_DEV_ID);
							}
						}
						else {
							post_log("Expected Device Not Found (PCIe %d HOST) ---"
									 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
									 " = 0x%x \n\n",PCIE_CTRL_1,busNum,PCI_VEN_ID,PCI_DEV_ID);

						}
						post_log("Expected Device Not Found (%s) ---"
								 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
								 " = 0x%x \n\n",pci_expected_list_dev[i].pass_str,
								 bus_num_cheetah_j, pci_expected_list_dev[i].ven_id,
								 pci_expected_list_dev[i].dev_id);
					}
					if (cheetah_ctrl_2 == 2) {
						if (pcie_ctrl_2 == 2) {	
							if (pci_flag) {
								post_log("Expected Device Found (PCIe %d HOST) ---"
										 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
										 " = 0x%x \n\n",pcie_ctrl_2,busNum,PCI_VEN_ID,PCI_DEV_ID);
							}
						}
						else {
							post_log("Expected Device Not Found (PCIe %d HOST) ---"
									 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
									 " = 0x%x \n\n",PCIE_CTRL_2,busNum,PCI_VEN_ID,PCI_DEV_ID);
						}
						post_log("Expected Device Not Found (%s) ---"
								 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
								 " = 0x%x \n\n",pci_expected_list_dev[i].pass_str,
								 bus_num_cheetah_j, pci_expected_list_dev[i].ven_id,
								 pci_expected_list_dev[i].dev_id);
					}
					if (cheetah_ctrl_3 == 3) {
						if (pcie_ctrl_3 == 3) {	
							if (pci_flag) {
								post_log("Expected Device Found (PCIe %d HOST) ---"
										 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
										 " = 0x%x \n\n",pcie_ctrl_3,busNum,PCI_VEN_ID,PCI_DEV_ID);
							}
						}
						else {
							post_log("Expected Device Not Found (PCIe %d HOST) ---"
									 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
									 " = 0x%x \n\n",PCIE_CTRL_3,busNum,PCI_VEN_ID,PCI_DEV_ID);
						}
						post_log("Expected Device Not Found (%s) ---"
								 "busNum = 0x%x --  vendor-id = 0x%x -- device-id"
								 " = 0x%x \n\n",pci_expected_list_dev[i].pass_str,
								 bus_num_cheetah_j, pci_expected_list_dev[i].ven_id,
								 pci_expected_list_dev[i].dev_id);
					}
				}
			}
			i++;  /* i is  incremented display is completed */
		}
	}

	if (fail == 0) {
		return 0;
	}
	return -1;
}
#endif /* CONFIG_POST & CFG_POST_PCI */
#endif /* CONFIG_POST */
#endif /* CONFIG_EX3242 */
