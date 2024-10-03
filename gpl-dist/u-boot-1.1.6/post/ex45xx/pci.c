/*
 * Copyright (c) 2010-2011, Juniper Networks, Inc.
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
#include <pcie.h>
#include <post.h>
#include <i2c.h>
#include <fsl_i2c.h>
#include <lcd.h>

#if CONFIG_POST & CFG_POST_PCI

DECLARE_GLOBAL_DATA_PTR;

#define DEV_FAIL (-1)
#define DEV_PASS 1
int post_result_pci;
int pci_relocate = 0;

extern struct pci_controller hose;
extern struct pcie_controller pcie_hose[];

int pcie_read_id (int ctrl, int dev, uint16_t *pvenid, uint16_t *pdevid);
int pci_read_id (int ctrl, int dev, uint16_t *pvenid, uint16_t *pdevid);

typedef int (*p2f) (int, int, uint16_t *, uint16_t *);
typedef int (*p2p) (void);

typedef struct pci_devices_struct {
    uint16_t venid;
    uint16_t devid;
    char name[50];
    struct  pci_devices_struct *next;   
} pci_devices;    

typedef struct pci_dev_struct {
    int pci;  /* 0 - PCI, 1 - PCIe */
    int ctrl;
    uint32_t bus;
    uint32_t dev;
    uint32_t fun;
    uint16_t venid;
    uint16_t devid;
    int chk_present;   /* 0 - no check, 1 - check present */
    int present_i2c_ctrl;   /* 0 - controller 1, 1 - controller 2 */
    uint8_t io_addr;
    uint8_t io_reg;
    uint8_t io_bit;
    uint8_t io_data; /* 0 - normal, 1 - inverted */
    p2f dev_op;  /* device function ptr */
    p2p present_op;  /* present function ptr */
    char name[50];
    pci_devices *next_dev;
} pci_dev;

static pci_devices ohci_dev={ 0x1033, 0x0035, "PD720102 OHCI", NULL};
static pci_devices ehci_dev={ 0x1033, 0x00E0, "PD720102 EHCI", NULL};


/* PCI/PCIe dev list  */
static pci_dev pci_dev_detect[] =
{
    {
        /* MPC8548 PCI Controller */
        0,  /* PCI */
        0,  /* PCI controller*/
        0,  /* bus */
        0,  /* device */
        0,  /* function */
        PCI_MPC8548_VENDOR_ID,  /* vendor ID */
        PCI_MPC8548_DEVICE_ID,  /* device ID */
        0,  /* present check */
        0,  /* present check i2c controller */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &pci_read_id,
        NULL,
        "MPC8548 PCI Controller",
        NULL,
    },
    {
        /* PCI ISP1568 OHCI */
        0,  /* PCI */
        0,  /* PCI controller*/
        0,  /* bus */
        0x12,  /* device */
        0,  /* function */
        USB_OHCI_VEND_ID,  /* vendor ID */
        USB_OHCI_DEV_ID,  /* device ID */
        0,  /* present check */
        0,  /* present check i2c controller */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &pci_read_id,
        NULL,
        "PCI ISP1568 OHCI",
        &ohci_dev,
    },
    {
        /* PCI ISP1568 EHCI */
        0,  /* PCI */
        0,  /* PCI controller*/
        0,  /* bus */
        0x12,  /* device */
        1,  /* function */
        USB_OHCI_VEND_ID,  /* vendor ID */
        USB_EHCI_DEV_ID,  /* device ID */
        0,  /* present check */
        0,  /* present check i2c controller */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &pci_read_id,
        NULL,
        "PCI ISP1568 EHCI",
        &ehci_dev,
    },
    {
        /* MPC8548 PCIe Controller */
        1,  /* PCIe */
        0,  /* PCIe controller*/
        0,  /* bus */
        0,  /* device */
        0,  /* function */
        PCIE_MPC8548_VENDOR_ID,  /* vendor ID */
        PCIE_MPC8548_DEVICE_ID,  /* device ID */
        0,  /* present check */
        0,  /* present check i2c controller */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &pcie_read_id,
        NULL,
        "MPC8548 PCIe Controller",
        NULL,
    },
    {
        /* PCIe PEX8618 Lane 0-3 */
        1,  /* PCIe */
        0,  /* PCIe controller*/
        1,  /* bus */
        0,  /* device */
        0,  /* function */
        PCIE_PEX8618_VENDOR_ID,  /* vendor ID */
        PCIE_PEX8618_DEVICE_ID,  /* device ID */
        0,  /* present check */
        0,  /* present check i2c controller */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &pcie_read_id,
        NULL,
        "PCIe PEX8618 Lane 0-3",
        NULL,
    },
    {
        /* PCIe PEX8618 Lane 8 */
        1,  /* PCIe */
        0,  /* PCIe controller*/
        2,  /* bus */
        1,  /* device */
        0,  /* function */
        PCIE_PEX8618_VENDOR_ID,  /* vendor ID */
        PCIE_PEX8618_DEVICE_ID,  /* device ID */
        0,  /* present check */
        0,  /* present check i2c controller */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &pcie_read_id,
        NULL,
        "PCIe PEX8618 Lane 8",
        NULL,
    },
    {
        /* PCIe PEX8618 Lane 4 */
        1,  /* PCIe */
        0,  /* PCIe controller*/
        2,  /* bus */
        2,  /* device */
        0,  /* function */
        PCIE_PEX8618_VENDOR_ID,  /* vendor ID */
        PCIE_PEX8618_DEVICE_ID,  /* device ID */
        0,  /* present check */
        0,  /* present check i2c controller */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &pcie_read_id,
        NULL,
        "PCIe PEX8618 Lane 4",
        NULL,
    },
    {
        /* PCIe PEX8618 Lane 12 */
        1,  /* PCIe */
        0,  /* PCIe controller*/
        2,  /* bus */
        3,  /* device */
        0,  /* function */
        PCIE_PEX8618_VENDOR_ID,  /* vendor ID */
        PCIE_PEX8618_DEVICE_ID,  /* device ID */
        0,  /* present check */
        0,  /* present check i2c controller */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &pcie_read_id,
        NULL,
        "PCIe PEX8618 Lane 12",
        NULL,
    },
    {
        /* PCIe PEX8618 Lane 9 */
        1,  /* PCIe */
        0,  /* PCIe controller*/
        2,  /* bus */
        5,  /* device */
        0,  /* function */
        PCIE_PEX8618_VENDOR_ID,  /* vendor ID */
        PCIE_PEX8618_DEVICE_ID,  /* device ID */
        0,  /* present check */
        0,  /* present check i2c controller */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &pcie_read_id,
        NULL,
        "PCIe PEX8618 Lane 9",
        NULL,
    },
    {
        /* PCIe PEX8618 Lane 10 */
        1,  /* PCIe */
        0,  /* PCIe controller*/
        2,  /* bus */
        7,  /* device */
        0,  /* function */
        PCIE_PEX8618_VENDOR_ID,  /* vendor ID */
        PCIE_PEX8618_DEVICE_ID,  /* device ID */
        0,  /* present check */
        0,  /* present check i2c controller */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &pcie_read_id,
        NULL,
        "PCIe PEX8618 Lane 10",
        NULL,
    },
    {
        /* PCIe PEX8618 Lane 11 */
        1,  /* PCIe */
        0,  /* PCIe controller*/
        2,  /* bus */
        9,  /* device */
        0,  /* function */
        PCIE_PEX8618_VENDOR_ID,  /* vendor ID */
        PCIE_PEX8618_DEVICE_ID,  /* device ID */
        0,  /* present check */
        0,  /* present check i2c controller */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &pcie_read_id,
        NULL,
        "PCIe PEX8618 Lane 11",
        NULL,
    },
    {
        /* PCIe PEX8618 Lane 5 */
        1,  /* PCIe */
        0,  /* PCIe controller*/
        2,  /* bus */
        0xa,  /* device */
        0,  /* function */
        PCIE_PEX8618_VENDOR_ID,  /* vendor ID */
        PCIE_PEX8618_DEVICE_ID,  /* device ID */
        0,  /* present check */
        0,  /* present check i2c controller */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &pcie_read_id,
        NULL,
        "PCIe PEX8618 Lane 5",
        NULL,
    },
    {
        /* PCIe PEX8618 Lane 13 */
        1,  /* PCIe */
        0,  /* PCIe controller*/
        2,  /* bus */
        0xb,  /* device */
        0,  /* function */
        PCIE_PEX8618_VENDOR_ID,  /* vendor ID */
        PCIE_PEX8618_DEVICE_ID,  /* device ID */
        0,  /* present check */
        0,  /* present check i2c controller */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &pcie_read_id,
        NULL,
        "PCIe PEX8618 Lane 13",
        NULL,
    },
    {
        1,  /* PCIe */
        /* PCIe PEX8618 Lane 6 */
        0,  /* PCIe controller*/
        2,  /* bus */
        0xc,  /* device */
        0,  /* function */
        PCIE_PEX8618_VENDOR_ID,  /* vendor ID */
        PCIE_PEX8618_DEVICE_ID,  /* device ID */
        0,  /* present check */
        0,  /* present check i2c controller */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &pcie_read_id,
        NULL,
        "PCIe PEX8618 Lane 6",
        NULL,
    },
    {
        /* PCIe PEX8618 Lane 14 */
        1,  /* PCIe */
        0,  /* PCIe controller*/
        2,  /* bus */
        0xd,  /* device */
        0,  /* function */
        PCIE_PEX8618_VENDOR_ID,  /* vendor ID */
        PCIE_PEX8618_DEVICE_ID,  /* device ID */
        0,  /* present check */
        0,  /* present check i2c controller */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &pcie_read_id,
        NULL,
        "PCIe PEX8618 Lane 14",
        NULL,
    },
    {
        /* PCIe PEX8618 Lane 7 */
        1,  /* PCIe */
        0,  /* PCIe controller*/
        2,  /* bus */
        0xe,  /* device */
        0,  /* function */
        PCIE_PEX8618_VENDOR_ID,  /* vendor ID */
        PCIE_PEX8618_DEVICE_ID,  /* device ID */
        0,  /* present check */
        0,  /* present check i2c controller */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &pcie_read_id,
        NULL,
        "PCIe PEX8618 Lane 7",
        NULL,
    },
    {
        /* PCIe PEX8618 Lane 15 */
        1,  /* PCIe */
        0,  /* PCIe controller*/
        2,  /* bus */
        0xf,  /* device */
        0,  /* function */
        PCIE_PEX8618_VENDOR_ID,  /* vendor ID */
        PCIE_PEX8618_DEVICE_ID,  /* device ID */
        0,  /* present check */
        0,  /* present check i2c controller */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &pcie_read_id,
        NULL,
        "PCIe PEX8618 Lane 15",
        NULL,
    },
    {
        /* PCIe Lion 1 PCIe 2 */
        1,  /* PCIe */
        0,  /* PCIe controller*/
        3,  /* bus */
        0,  /* device */
        0,  /* function */
        PCIE_LION_VENDOR_ID,  /* vendor ID */
        PCIE_LION_DEVICE_ID,  /* device ID */
        0,  /* present check */
        0,  /* present check i2c controller */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &pcie_read_id,
        NULL,
        "PCIe Lion 1 PCIe 2",
        NULL,
    },
    {
        /* PCIe Lion 2 PCIe 2 */
        1,  /* PCIe */
        0,  /* PCIe controller*/
        5,  /* bus */
        0,  /* device */
        0,  /* function */
        PCIE_LION_VENDOR_ID,  /* vendor ID */
        PCIE_LION_DEVICE_ID,  /* device ID */
        0,  /* present check */
        0,  /* present check i2c controller */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &pcie_read_id,
        NULL,
        "PCIe Lion 2 PCIe 2",
        NULL,
    },
    {
        /* PCIe Lion 1 PCIe 3 */
        1,  /* PCIe */
        0,  /* PCIe controller*/
        6,  /* bus */
        0,  /* device */
        0,  /* function */
        PCIE_LION_VENDOR_ID,  /* vendor ID */
        PCIE_LION_DEVICE_ID,  /* device ID */
        0,  /* present check */
        0,  /* present check i2c controller */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &pcie_read_id,
        NULL,
        "PCIe Lion 1 PCIe 3",
        NULL,
    },
    {
        /* PCIe Lion 2 PCIe 0 */
        1,  /* PCIe */
        0,  /* PCIe controller*/
        7,  /* bus */
        0,  /* device */
        0,  /* function */
        PCIE_LION_VENDOR_ID,  /* vendor ID */
        PCIE_LION_DEVICE_ID,  /* device ID */
        0,  /* present check */
        0,  /* present check i2c controller */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &pcie_read_id,
        NULL,
        "PCIe Lion 2 PCIe 0",
        NULL,
    },
    {
        /* PCIe Lion 2 PCIe 1 */
        1,  /* PCIe */
        0,  /* PCIe controller*/
        8,  /* bus */
        0,  /* device */
        0,  /* function */
        PCIE_LION_VENDOR_ID,  /* vendor ID */
        PCIE_LION_DEVICE_ID,  /* device ID */
        0,  /* present check */
        0,  /* present check i2c controller */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &pcie_read_id,
        NULL,
        "PCIe Lion 2 PCIe 1",
        NULL,
    },
    {
        /* PCIe Lion 2 PCIe 3 */
        1,  /* PCIe */
        0,  /* PCIe controller*/
        0xa,  /* bus */
        0,  /* device */
        0,  /* function */
        PCIE_LION_VENDOR_ID,  /* vendor ID */
        PCIE_LION_DEVICE_ID,  /* device ID */
        0,  /* present check */
        0,  /* present check i2c controller */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &pcie_read_id,
        NULL,
        "PCIe Lion 2 PCIe 3",
        NULL,
    },
    {
        /* PCIe Lion 1 PCIe 0 */
        1,  /* PCIe */
        0,  /* PCIe controller*/
        0xb,  /* bus */
        0,  /* device */
        0,  /* function */
        PCIE_LION_VENDOR_ID,  /* vendor ID */
        PCIE_LION_DEVICE_ID,  /* device ID */
        0,  /* present check */
        0,  /* present check i2c controller */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &pcie_read_id,
        NULL,
        "PCIe Lion 1 PCIe 0",
        NULL,
    },
    {
        /* PCIe Lion 1 PCIe 1 */
        1,  /* PCIe */
        0,  /* PCIe controller*/
        0xd,  /* bus */
        0,  /* device */
        0,  /* function */
        PCIE_LION_VENDOR_ID,  /* vendor ID */
        PCIE_LION_DEVICE_ID,  /* device ID */
        0,  /* present check */
        0,  /* present check i2c controller */
        0,  /* io expander addr */
        0,  /* read register */
        0,  /* bit offset */
        0,  /* normal/inverted */
        &pcie_read_id,
        NULL,
        "PCIe Lion 1 PCIe 1",
        NULL,
    },
};

int
pcie_read_id (int ctrl, int dev, uint16_t *pvenid, uint16_t *pdevid)
{
    int res1, res2;
    
    res1 = pcie_hose_read_config_word
        (&pcie_hose[ctrl], dev, PCIE_VENDOR_ID, pvenid);
    res2 = pcie_hose_read_config_word
        (&pcie_hose[ctrl], dev, PCIE_DEVICE_ID, pdevid);

    return (res1 | res2);
}

int
pci_read_id (int ctrl, int dev, uint16_t *pvenid, uint16_t *pdevid)
{
    int res1, res2;
    
    res1 = pci_read_config_word
        (dev, PCI_VENDOR_ID, pvenid);
    res2 = pci_read_config_word
        (dev, PCI_DEVICE_ID, pdevid);

    return (res1 | res2);
}

int 
pci_post_test (int flags)
{
    int pci_result_size = sizeof(pci_dev_detect) / sizeof(pci_dev_detect[0]);
    int pci_result_array[30] = {0};
    int pci_verbose = flags & POST_DEBUG;
    int present = 0, fail = 0, cur_ctrl, i;
    int dev, res;
    uint16_t venid[30] = {0}, devid[30] = {0};
    uint8_t temp;
    uint16_t disp_venid[30], disp_devid[30];
    char disp_name[30][50];
    pci_devices *pci_next_dev;

    post_result_pci = 0;

    if (pci_relocate == 0) {
        /* relocate function pointers */
        for (i = 0; i < pci_result_size; i++) {
            if (pci_dev_detect[i].dev_op)
                pci_dev_detect[i].dev_op = pci_dev_detect[i].dev_op + gd->reloc_off;
            if (pci_dev_detect[i].present_op)
                pci_dev_detect[i].present_op = pci_dev_detect[i].present_op + gd->reloc_off;
        }
        pci_relocate = 1;
    }

    cur_ctrl = current_i2c_controller();

    for (i = 0; i < pci_result_size; i++) {
        if (pci_dev_detect[i].chk_present) {
            if (pci_dev_detect[i].present_op) {
                present = pci_dev_detect[i].present_op();
            }
            else {
                present = 0;
                i2c_controller(pci_dev_detect[i].present_i2c_ctrl);
                i2c_read(pci_dev_detect[i].io_addr, pci_dev_detect[i].io_reg, 1, &temp, 1);
                if (pci_dev_detect[i].io_data) {
                    temp = ~temp;
                }
                present = temp & (1 << pci_dev_detect[i].io_bit);
            }
        }
        else
            present = 1;

        if (present) {
            dev = ((pci_dev_detect[i].bus) << 16 | 
                (pci_dev_detect[i].dev) << 11 | 
                (pci_dev_detect[i].fun) << 8);
            res = pci_dev_detect[i].dev_op(pci_dev_detect[i].ctrl, 
                dev, &venid[i], &devid[i]);
            disp_venid[i] = pci_dev_detect[i].venid;
            disp_devid[i] = pci_dev_detect[i].devid;
            strncpy(disp_name[i],pci_dev_detect[i].name,
                sizeof(pci_dev_detect[i].name));
            if (res)
                pci_result_array[i] = DEV_FAIL;
            else {
                if ((venid[i] != pci_dev_detect[i].venid) || 
                    (devid[i] != pci_dev_detect[i].devid)) {
                    pci_result_array[i] = DEV_FAIL;
                    pci_next_dev =  pci_dev_detect[i].next_dev;
                    while (pci_next_dev != NULL) {
                        if ((venid[i] == pci_next_dev->venid) &&
                            (devid[i] == pci_next_dev->devid)) {
                            pci_result_array[i] = DEV_PASS;
                            disp_venid[i] = pci_next_dev->venid;
                            disp_devid[i] = pci_next_dev->devid;
                            strncpy(disp_name[i],pci_next_dev->name,
                                sizeof(pci_next_dev->name));
                            break;
                        }    
                        else {
                            pci_next_dev = pci_next_dev->next;
                        }    
                    } 
                }    
                else {
                    pci_result_array[i] = DEV_PASS;
                }    
            }
            if (pci_result_array[i] == DEV_FAIL)
                fail = 1;
        }
    }
    i2c_controller(cur_ctrl);

    if (flags & BOOT_POST_FLAG) {
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

    if (fail) {
        post_log("-- PCI/PCIe test                        FAILED @\n");
        for (i = 0; i < pci_result_size; i++) {
            if (pci_result_array[i] == DEV_FAIL) {
                post_log (" > %s: %02x:%02x:%02x %-11s %-23s "
						  "%04x/%04x got %04x/%04x @\n",
						  (pci_dev_detect[i].pci) ?
						  "PCIe" : "PCI ",
						  pci_dev_detect[i].bus,
						  pci_dev_detect[i].dev,
						  pci_dev_detect[i].fun,
						  "Not Match",
						  disp_name[i],
						  disp_venid[i],
						  disp_devid[i],
						  venid[i],
						  devid[i]);
             }
            else {
                if (pci_verbose) {
                post_log (" > %s: %02x:%02x:%02x %-11s %-23s "
						  "%04x/%04x got %04x/%04x\n",
						  (pci_dev_detect[i].pci) ?
						  "PCIe" : "PCI ",
						  pci_dev_detect[i].bus,
						  pci_dev_detect[i].dev,
						  pci_dev_detect[i].fun,
						  (pci_result_array[i] == DEV_PASS) ? 
						  "Found" : "Not Present",
						  disp_name[i],
						  disp_venid[i],
						  disp_devid[i],
						  venid[i],
						  devid[i]);
                }
            }
        }
    }
    else {
        post_log("-- PCI/PCIe test                        PASSED\n");
        if (pci_verbose) {
            for (i = 0; i < pci_result_size; i++) {
                post_log (" > %s: %02x:%02x:%02x %-11s %-23s "
						  "%04x/%04x got %04x/%04x\n",
						  (pci_dev_detect[i].pci) ?
						  "PCIe" : "PCI ",
						  pci_dev_detect[i].bus,
						  pci_dev_detect[i].dev,
						  pci_dev_detect[i].fun,
						  (pci_result_array[i] == DEV_PASS) ? 
						  "Found" : "Not Present",
						  disp_name[i],
						  disp_venid[i],
						  disp_devid[i],
						  venid[i],
						  devid[i]);
            }
        }
    }

    if (fail)
        return (-1);
    return 0;

}
#endif /* CONFIG_POST & CFG_POST_PCI */
#endif /* CONFIG_POST */
