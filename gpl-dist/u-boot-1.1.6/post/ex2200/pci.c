/*
 * Copyright (c) 2009-2011, Juniper Networks, Inc.
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
#ifdef CONFIG_POST

#include <command.h>
#include <asm/processor.h>
#include <asm/io.h>
#include <pci.h>
#include <post.h>

extern int board_idtype(void);
extern int board_pci(void);
int post_result_pci;

/* Post pci_dev_list struct declartion */
struct pci_dev_list
{
	u16 ven_id;
	u16 dev_id;
	char pass_str[20];
	u16 pass_count;
};

#define MAX_EX2200_PCI_DEV 	2

#define VENDOR_ID_MAX 	0xFFFF
#define VENDOR_ID_MIN 	0x0000
#define PCI_VEN_ID  	0x1957 
#define PCI_DEV_ID  	0x32
#define CON_OHCI_VEN_ID 0x1131
#define CON_OHCI_DEV_ID 0x1561
#define EHCI_VEN_ID		0x1131
#define EHCI_DEV_ID 	0x1562
#define CHEETAH_VEN_ID 	0x11AB
#define CHEETAH_DEV_ID 	0xD91C
#define CHEETAH_J_VEN_ID 	0x11AB
#define CHEETAH_J_DEV_ID 	0xDB00
#define CHEETAH_J_DEV_ID1 	0xDB01
#define CHEETAH_J2_DEV_ID 	0x6281
#define CHEETAH_J2_DEV_ID1 	0xDDB6

struct pci_dev_list pci_expected_list_dev[MAX_EX2200_PCI_DEV] = {
	{CHEETAH_VEN_ID, CHEETAH_J2_DEV_ID, "CHEETAH_J2 Device", 0},
	{CHEETAH_VEN_ID, CHEETAH_J2_DEV_ID1, "CHEETAH_J2 Device", 0}};

int 
pci_post_test (int flags)
{
    int device = 0;
    int function = 0;
    int pass  = 0;
    int known_dev = 0;
    int i =0;
    int total_cheetah_dev =0;
    unsigned char header_type=0;
    unsigned short  vendor_id=0, device_id=0;
    pci_dev_t dev;
    uint ex2200_bd_id =0;
    int boot_flag_post = 0;
    int fail =0;
    int busNum = 0;
    int pci_unknow_dev_flag = 0;
    int pci_flag = 0;
    int count_pci = 0;
    unsigned short pci_exp_list[3] = {0};
    unsigned short pci_unknow_ven_id[255];
    unsigned short pci_unknow_dev_id[255];

    post_result_pci = 0;

    if (board_pci() == 0) /* no pci */
        return 0;
       
    if (flags & POST_DEBUG) {
        pci_flag =1;
    }	
    for (i = 0; i < 3; i++) {
        pci_exp_list[i] = 0;
        pci_expected_list_dev[i].pass_count =0;
    }
    ex2200_bd_id = board_idtype();

    switch (ex2200_bd_id) {
        case I2C_ID_JUNIPER_EX2200_12_T:
            total_cheetah_dev = 1;
            break;
        case I2C_ID_JUNIPER_EX2200_12_P:
            total_cheetah_dev = 1;
            break;
        CASE_ALL_I2C_ID_EX2200_24XX:
            total_cheetah_dev = 1;
            break;
        CASE_ALL_I2C_ID_EX2200_48XX:
            total_cheetah_dev = 2;
            break;
        default:
            total_cheetah_dev = 1;
            break;
    };

    if (flags & BOOT_POST_FLAG) {
        boot_flag_post = 1;
    }

    for (device = 0; device < MAX_EX2200_PCI_DEV; device++) {
        header_type = 0;
        vendor_id = 0;
        for (function = 0; function < PCI_MAX_PCI_FUNCTIONS; function++) {
            /* If this is not a multi-function device, we skip the rest. */
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
                for (i = 0; i < MAX_EX2200_PCI_DEV ; i++) {
                    if ((vendor_id == pci_expected_list_dev[i].ven_id )) { 
                        if ((device_id == pci_expected_list_dev[i].dev_id) &&
                            (vendor_id == pci_expected_list_dev[i].ven_id )) { 
                            known_dev = 1;
                            pci_expected_list_dev[i].pass_count++;
                            pass++;
                            pci_exp_list[count_pci]= i;
                            count_pci++;
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


    /* To display the result over all pass or fail this check code is must. */
    for (i = 0; i < MAX_EX2200_PCI_DEV; i++) {
        if (pci_expected_list_dev[i].pass_count == 0) {
            fail = -1;
        }
    }

    if (boot_flag_post) {
        if (!fail)
            return (0);
        return (-1);
    }
    
    /* This code displays the pci result based on the pci device detection. */
    if (fail == 0) {
        if (!pci_flag){
            post_log("-- PCI test                             PASSED\n");
        }
    }
    else {
        post_result_pci = -1;
        post_log("-- PCI test                             FAILED\n");
    }

    if (pass) {
        if (pci_flag) {
            if (fail == 0) {
                post_log("-- PCI test                             PASSED\n");
            } 
        }
        for (i = 0; i < MAX_EX2200_PCI_DEV; i++) {
            if (pci_flag) {
                if (pci_expected_list_dev[i].pass_count == 1) {
                    post_log(" > Expected PCI: %-9s %-18s "
							 "bus=0x%x "
							 "id=0x%x "
							 "dev=0x%x\n",
							 "Found",
							 pci_expected_list_dev[i].pass_str,
							 busNum, 
							 pci_expected_list_dev[i].ven_id,
							 pci_expected_list_dev[i].dev_id);
                }
            }
        }
    }

    /* Print the missed expected pci devices other then cheetah devices. */
    for (i = 0; i < MAX_EX2200_PCI_DEV; i++) {
        if (pci_expected_list_dev[i].pass_count == 0) {
            post_log(" > Expected PCI: %-9s %-18s "
					 "bus=0x%x "
					 "id=0x%x "
					 "dev=0x%x @\n",
					 "Not Found",
					 pci_expected_list_dev[i].pass_str,
					 busNum, 
					 pci_expected_list_dev[i].ven_id, 
					 pci_expected_list_dev[i].dev_id);
        }
    }

    if (fail == 0) {
        return 0;
    }
    return -1;
}
#endif /* CONFIG_POST */

