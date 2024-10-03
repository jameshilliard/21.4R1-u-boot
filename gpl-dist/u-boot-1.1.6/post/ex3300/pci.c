/*
 * Copyright (c) 2010, Juniper Networks, Inc.
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
//#include <lcd.h>

#if CONFIG_POST & CFG_POST_PCI

DECLARE_GLOBAL_DATA_PTR;

extern int board_idtype(void);

#define VENDOR_ID_MAX 	0xFFFF
#define VENDOR_ID_MIN 	0x0000
#define MARVELL_VEN_ID 	0x11AB
#define MV78100_DEV_ID 	0x7810
#define CHEETAH_J2_DEV_ID 	0xDDB6

#define DEV_FAIL (-1)
#define DEV_PASS 1
int post_result_pci;

int pci_read_id (int dev, uint16_t *pvenid, uint16_t *pdevid);

typedef int (*p2f) (int, uint16_t *, uint16_t *);

typedef struct pci_dev_struct {
    int pci;  /* 0 - PCI, 1 - PCIe */
    uint32_t bus;
    uint32_t dev;
    uint32_t fun;
    uint16_t venid;
    uint16_t devid;
    p2f dev_op;  /* device function ptr */
    char name[50];
} pci_dev;

/* PCI/PCIe dev list  */
static pci_dev pci_dev_detect[] =
{
    {
        /* PCIe Controller 0 */
        1,  /* PCIe */
        0,  /* bus */
        0,  /* device */
        0,  /* function */
        MARVELL_VEN_ID,  /* vendor ID */
        MV78100_DEV_ID,  /* device ID */
        &pci_read_id,
        "PCIe Controller 0",
    },
    {
        /* Cheetah 0 */
        1,  /* PCIe */
        0,  /* bus */
        1,  /* device */
        0,  /* function */
        MARVELL_VEN_ID,  /* vendor ID */
        CHEETAH_J2_DEV_ID,  /* device ID */
        &pci_read_id,
        "Cheetah 0",
    },
    {
        /* PCIe Controller 1 */
        1,  /* PCIe */
        1,  /* bus */
        0,  /* device */
        0,  /* function */
        MARVELL_VEN_ID,  /* vendor ID */
        MV78100_DEV_ID,  /* device ID */
        &pci_read_id,
        "PCIe Controller 1",
    },
    {
        /* Cheetah 1 */
        1,  /* PCIe */
        1,  /* bus */
        1,  /* device */
        0,  /* function */
        MARVELL_VEN_ID,  /* vendor ID */
        CHEETAH_J2_DEV_ID,  /* device ID */
        &pci_read_id,
        "Cheetah 1",
    },
};


int
pci_read_id (int dev, uint16_t *pvenid, uint16_t *pdevid)
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
    int fail = 0, i;
    int dev, res;
    uint16_t venid[30] = {0}, devid[30] = {0};

    post_result_pci = 0;

    switch (board_idtype())
    {
        default: 
        case I2C_ID_JUNIPER_EX3300_24_T: /* Dragon 24 T */
        case I2C_ID_JUNIPER_EX3300_24_P: /* Dragon 24 P */
        case I2C_ID_JUNIPER_EX3300_24_T_DC: /* Dragon 24 T DC */
            pci_result_size = pci_result_size / 2;
            break;

        case I2C_ID_JUNIPER_EX3300_48_T: /* Dragon 48 T */
        case I2C_ID_JUNIPER_EX3300_48_P: /* Dragon 48 P */
        case I2C_ID_JUNIPER_EX3300_48_T_BF: /* Dragon 48 T BF */
            break;
    }
    
    for (i = 0; i < pci_result_size; i++) {
        dev = ((pci_dev_detect[i].bus) << 16 | 
               (pci_dev_detect[i].dev) << 11 | 
               (pci_dev_detect[i].fun) << 8);
        res = pci_dev_detect[i].dev_op(dev, &venid[i], &devid[i]);
        if (res)
            pci_result_array[i] = DEV_FAIL;
        else {
            if ((venid[i] != pci_dev_detect[i].venid) || 
                (devid[i] != pci_dev_detect[i].devid))
                pci_result_array[i] = DEV_FAIL;
            else
                pci_result_array[i] = DEV_PASS;
        }
        if (pci_result_array[i] == DEV_FAIL)
            fail = 1;
    }
    
    if (flags & BOOT_POST_FLAG) {
        if (fail == 0) {
            return 0;
        }
        else {
            post_result_pci = -1;
            return -1;
        }
    }

    if (fail) {
        post_log("-- PCIe test                            FAILED @\n");
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
						  pci_dev_detect[i].name,
						  pci_dev_detect[i].venid,
						  pci_dev_detect[i].devid,
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
						  pci_dev_detect[i].name,
						  pci_dev_detect[i].venid,
						  pci_dev_detect[i].devid,
						  venid[i],
						  devid[i]);
                }
            }
        }
    }
    else {
        post_log("-- PCIe test                            PASSED\n");
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
						  pci_dev_detect[i].name,
						  pci_dev_detect[i].venid,
						  pci_dev_detect[i].devid,
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
