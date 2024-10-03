/*
 * Copyright (c) 2010-2011, Juniper Networks, Inc.
 * All rights reserved.
 *
 * (C) Copyright 2002
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

#include <common.h>

#ifdef CONFIG_POST

#include <configs/ex82xx.h>
#include <command.h>
#include <asm/processor.h>
#include <asm/io.h>
#include <pcie.h>
#include <post.h>
#include <watchdog.h>
#include <ex82xx_devices.h>

#if CONFIG_POST & CFG_POST_PCIE

DECLARE_GLOBAL_DATA_PTR;

typedef struct _pcie_dev_list {
	u8   bus_num;
	u8   dev_num;
	u8   fn_num;
	u16  ven_id;
	u16  dev_id;
	u8   dontcare_devid; 
	char *descr;
} PCIE_DEV_LIST;


static PCIE_DEV_LIST recpu_pcie_list[] = {
	{ 0, 0x0, 0, PCIE_FREESCALE_VID, PCIE_MPC8548_DID, 1,
	  "MPC8548 PCIE Root Complex                       " },
	{ 1, 0x0, 0, PCIE_PLXTECH_VID,   PCIE_PEX8111_DID, 1,
	  "PEX8111/8112 PCIE-PCI Bridge                    " },
	{ 2, 0xB, 0, PCIE_BROADCOM_VID,  PCIE_BCM56307_DID, 0,
	  "BCM56307 24 Port Gigiabit Ethernet Switch       " },
	{ 2, 0xC, 0, PCIE_NXP_VID,       PCIE_NXP1564_OHCI_DID, 0,
	  "ISP1564 Hi-Speed USB PCI Host Controller (OHCI) " },
	{ 2, 0xC, 2, PCIE_NXP_VID,       PCIE_NXP1564_EHCI_DID, 0,
	  "ISP1564 Hi-Speed USB PCI Host Controller (EHCI) " },
	{ 2, 0xD, 0, PCIE_JNPRNET_VID,   PCIE_SCBC_DID, 1,
	  "SCB Contrl FPGA                                 " },
	{ 2, 0xE, 0, PCIE_BROADCOM_VID,  PCIE_BCM56305_DID, 0,
	  "BCM56305 24 Port Gigiabit Ethernet Switch       " },
};


static PCIE_DEV_LIST lc48_pcie_list[] = {
	{ 0, 0x0, 0, PCIE_FREESCALE_VID, PCIE_MPC8548_DID, 1,
	  "MPC8548 PCIE Root Complex    " },
	{ 1, 0x0, 0, PCIE_PLXTECH_VID,   PCIE_PEX8508_DID, 0,
	  "PEX8508 PCIE Switch0         " },
	{ 2, 0x1, 0, PCIE_PLXTECH_VID,   PCIE_PEX8508_DID, 0,
	  "PEX8508 PCIE Switch0 port 0  " },
	{ 2, 0x2, 0, PCIE_PLXTECH_VID,   PCIE_PEX8508_DID, 0,
	  "PEX8508 PCIE Switch0 port 1  " },
	{ 2, 0x3, 0, PCIE_PLXTECH_VID,   PCIE_PEX8508_DID, 0,
	  "PEX8508 PCIE Switch0 port 2  " },
	{ 3, 0x0, 0, PCIE_PLXTECH_VID,   PCIE_PEX8508_DID, 0,
	  "PEX8508 PCIE Switch1         " },
	{ 4, 0x1, 0, PCIE_PLXTECH_VID,   PCIE_PEX8508_DID, 0,
	  "PEX8508 PCIE Switch1 port 0  " },
	{ 4, 0x2, 0, PCIE_PLXTECH_VID,   PCIE_PEX8508_DID, 0,
	  "PEX8508 PCIE Switch1 port 1  " },
	{ 4, 0x3, 0, PCIE_PLXTECH_VID,   PCIE_PEX8508_DID, 0,
	  "PEX8508 PCIE Switch1 port 2  " },
	{ 4, 0x4, 0, PCIE_PLXTECH_VID,   PCIE_PEX8508_DID, 0,
	  "PEX8508 PCIE Switch1 port 3  " },
	{ 5, 0x0, 0, PCIE_MARVELL_VID,   PCIE_PUMAJ_DID, 0,
	  "PUMA0                        " },
	/*{ 5, 0x0, 0, PCIE_MARVELL_VID,   PCIE_48_PUMAJ_DID, 0,
	  "PUMA0                        " },*/
};

static PCIE_DEV_LIST lc8xs_pcie_list[] = {
	{ 0, 0x0, 0, PCIE_FREESCALE_VID, PCIE_MPC8548_DID, 1,
	  "MPC8548 PCIE Root Complex    " },
	{ 1, 0x0, 0, PCIE_PLXTECH_VID, PCIE_PEX8508_DID, 0,
	  "PEX8508 PCIE Switch0         " },
	{ 2, 0x1, 0, PCIE_PLXTECH_VID, PCIE_PEX8508_DID, 0,
	  "PEX8508 PCIE Switch0 port 0  " },
	{ 2, 0x2, 0, PCIE_PLXTECH_VID, PCIE_PEX8508_DID, 0,
	  "PEX8508 PCIE Switch0 port 1  " },
	{ 2, 0x3, 0, PCIE_PLXTECH_VID, PCIE_PEX8508_DID, 0,
	  "PEX8508 PCIE Switch0 port 2  " },
	{ 3, 0x0, 0, PCIE_PLXTECH_VID, PCIE_PEX8508_DID, 0,
	  "PEX8508 PCIE Switch1         " },
	{ 4, 0x1, 0, PCIE_PLXTECH_VID, PCIE_PEX8508_DID, 0,
	  "PEX8508 PCIE Switch1 port 0  " },
	{ 4, 0x2, 0, PCIE_PLXTECH_VID, PCIE_PEX8508_DID, 0,
	  "PEX8508 PCIE Switch1 port 1  " },
	{ 4, 0x3, 0, PCIE_PLXTECH_VID, PCIE_PEX8508_DID, 0,
	  "PEX8508 PCIE Switch1 port 2  " },
	{ 4, 0x4, 0, PCIE_PLXTECH_VID, PCIE_PEX8508_DID, 0,
	  "PEX8508 PCIE Switch1 port 3  " },
	{ 5, 0x0, 0, PCIE_MARVELL_VID, PCIE_PUMAJ_DID, 0,
	  "PUMA0                        " },
	{ 6, 0x0, 0, PCIE_MARVELL_VID, PCIE_PUMAJ_DID, 0,
	  "PUMA1                        " },
	{ 7, 0x0, 0, PCIE_MARVELL_VID, PCIE_PUMAJ_DID, 0,
	  "PUMA0                        " },
	{ 8, 0x0, 0, PCIE_MARVELL_VID, PCIE_PUMAJ_DID, 0,
	  "PUMA1                        " },
	/*{ 5, 0x0, 0, PCIE_MARVELL_VID, PCIE_8XS_PUMAJ_DID, 0,
	  "PUMA0                        " },
	{ 6, 0x0, 0, PCIE_MARVELL_VID, PCIE_8XS_PUMAJ_DID, 0,
	  "PUMA1                        " },
	{ 7, 0x0, 0, PCIE_MARVELL_VID, PCIE_8XS_PUMAJ_DID, 0,
	  "PUMA0                        " },
	{ 8, 0x0, 0, PCIE_MARVELL_VID, PCIE_8XS_PUMAJ_DID, 0,
	  "PUMA1                        " },*/
	{ 9, 0x0, 0, PCIE_PLXTECH_VID, PCIE_PEX8508_DID, 0,
	  "PEX8508 PCIE Switch2         " },
	{ 0xa, 0x1, 0, PCIE_PLXTECH_VID, PCIE_PEX8508_DID, 0,
	  "PEX8508 PCIE Switch2 port 0  " },
	{ 0xa, 0x2, 0, PCIE_PLXTECH_VID, PCIE_PEX8508_DID, 0,
	  "PEX8508 PCIE Switch2 port 1  " },
	{ 0xa, 0x3, 0, PCIE_PLXTECH_VID, PCIE_PEX8508_DID, 0,
	  "PEX8508 PCIE Switch2 port 2  " },
	{ 0xa, 0x4, 0, PCIE_PLXTECH_VID, PCIE_PEX8508_DID, 0,
	  "PEX8508 PCIE Switch2 port 3  " },
};


static PCIE_DEV_LIST lc_2xs_44ge_pcie_list[] = {
	{ 0, 0x0, 0, PCIE_FREESCALE_VID, PCIE_P2020E_DID, 1,
	  "P2020E PCIE Root Complex    " },
	{ 1, 0x0, 0, PCIE_PLXTECH_VID,   PCIE_PEX8509_DID, 0,
	  "PEX8509 PCIE Switch0         " },
	{ 2, 0x1, 0, PCIE_PLXTECH_VID,   PCIE_PEX8509_DID, 0,
	  "PEX8509 PCIE Switch0 port 0  " },
	{ 2, 0x2, 0, PCIE_PLXTECH_VID,   PCIE_PEX8509_DID, 0,
	  "PEX8509 PCIE Switch0 port 1  " },
	{ 2, 0x3, 0, PCIE_PLXTECH_VID,   PCIE_PEX8509_DID, 0,
	  "PEX8509 PCIE Switch0 port 2  " },
	{ 2, 0x4, 0, PCIE_PLXTECH_VID,   PCIE_PEX8509_DID, 0,
	  "PEX8509 PCIE Switch0 port 3  " },
	{ 2, 0x5, 0, PCIE_PLXTECH_VID,   PCIE_PEX8509_DID, 0,
	  "PEX8509 PCIE Switch0 port 4  " },
	{ 2, 0x6, 0, PCIE_PLXTECH_VID,   PCIE_PEX8509_DID, 0,
	  "PEX8509 PCIE Switch0 port 5  " },
	{ 3, 0x0, 0, PCIE_MARVELL_VID,   PCIE_PUMAJ_c341_DID, 0,
	  "PUMAJ0			" },
	{ 4, 0x0, 0, PCIE_MARVELL_VID,   PCIE_PUMAJ_c341_DID, 0,
	  "PUMAJ1			" },
	{ 7, 0x0, 0, PCIE_MARVELL_VID,   PCIE_CHEETAH_DID, 0,
	  "CHEETAHJ0			" },
	{ 8, 0x0, 0, PCIE_MARVELL_VID,   PCIE_CHEETAH_DID, 0,
	  "CHEETAHJ1			" },
};

static PCIE_DEV_LIST lc_48p_pcie_list[] = {
	{ 0, 0x0, 0, PCIE_FREESCALE_VID, PCIE_P2020E_DID, 1,
	  "P2020E PCIE Root Complex    " },
	{ 1, 0x0, 0, PCIE_PLXTECH_VID,   PCIE_PEX8509_DID, 0,
	  "PEX8509 PCIE Switch0         " },
	{ 2, 0x1, 0, PCIE_PLXTECH_VID,   PCIE_PEX8509_DID, 0,
	  "PEX8509 PCIE Switch0 port 0  " },
	{ 2, 0x2, 0, PCIE_PLXTECH_VID,   PCIE_PEX8509_DID, 0,
	  "PEX8509 PCIE Switch0 port 1  " },
	{ 2, 0x3, 0, PCIE_PLXTECH_VID,   PCIE_PEX8509_DID, 0,
	  "PEX8509 PCIE Switch0 port 2  " },
	{ 2, 0x4, 0, PCIE_PLXTECH_VID,   PCIE_PEX8509_DID, 0,
	  "PEX8509 PCIE Switch0 port 3  " },
	{ 2, 0x5, 0, PCIE_PLXTECH_VID,   PCIE_PEX8509_DID, 0,
	  "PEX8509 PCIE Switch0 port 4  " },
	{ 2, 0x6, 0, PCIE_PLXTECH_VID,   PCIE_PEX8509_DID, 0,
	  "PEX8509 PCIE Switch0 port 5  " },
	{ 3, 0x0, 0, PCIE_MARVELL_VID,   PCIE_PUMAJ_c341_DID, 0,
	  "PUMAJ0			" },
	{ 7, 0x0, 0, PCIE_MARVELL_VID,   PCIE_CHEETAH_DID, 0,
	  "CHEETAHJ0			" },
	{ 8, 0x0, 0, PCIE_MARVELL_VID,   PCIE_CHEETAH_DID, 0,
	  "CHEETAHJ1			" },
};

static unsigned int pcie_recpu_dev_list_count = sizeof(recpu_pcie_list) /
												sizeof(PCIE_DEV_LIST);


static unsigned int pcie_lc48_dev_list_count = sizeof(lc48_pcie_list) /
											   sizeof(PCIE_DEV_LIST);
static unsigned int pcie_lc8xs_dev_list_count = sizeof(lc8xs_pcie_list) /
												sizeof(PCIE_DEV_LIST);
static unsigned int pcie_lc_2xs_44ge_dev_list_count = sizeof(lc_2xs_44ge_pcie_list) /
												sizeof(PCIE_DEV_LIST);
static unsigned int pcie_lc_48p_dev_list_count = sizeof(lc_48p_pcie_list) /
												sizeof(PCIE_DEV_LIST);

int pcie_post_test(int flags)
{
	PCIE_DEV_LIST *p_dev_list = 0;
	u32 dev_count = 0;
	u32 i = 0;
	pcie_dev_t dev;
	unsigned short vendor, device;
	int ret = 0, pass = 0;
	u_int16_t assembly_id;

	/* Init PCIE */
	printf("Init PCIE...\n");
	pcie_init();

	post_log("\nVerifying expected devices on PCIe interface ...\n");
	if (EX82XX_RECPU) {
		p_dev_list = recpu_pcie_list;
		dev_count  = pcie_recpu_dev_list_count;
	} else if (EX82XX_LCPU) {
		assembly_id = get_assembly_id(EX82XX_LC_ID_EEPROM_ADDR);
		switch (assembly_id) {
		case EX82XX_LC_48T_BRD_ID:
		case EX82XX_LC_48T_NM_BRD_ID:
		case EX82XX_LC_48T_ES_BRD_ID:
		case EX82XX_LC_48F_BRD_ID:
		case EX82XX_LC_48F_NM_BRD_ID:
		case EX82XX_LC_48F_ES_BRD_ID:
			p_dev_list = lc48_pcie_list;
			dev_count  = pcie_lc48_dev_list_count;
			break;
		case EX82XX_LC_8XS_BRD_ID:
		case EX82XX_LC_8XS_NM_BRD_ID:
		case EX82XX_LC_8XS_ES_BRD_ID:
			p_dev_list = lc8xs_pcie_list;
			dev_count  = pcie_lc8xs_dev_list_count;
			break;
		case EX82XX_LC_48P_BRD_ID: 
		case EX82XX_LC_48TE_BRD_ID:
			p_dev_list = lc_48p_pcie_list;
			dev_count  = pcie_lc_48p_dev_list_count;
			break;
		case EX82XX_LC_2XS_44GE_BRD_ID:
		case EX82XX_LC_2X4F40TE_BRD_ID:
			p_dev_list = lc_2xs_44ge_pcie_list;
			dev_count  = pcie_lc_2xs_44ge_dev_list_count;
			break;
		default:
			printf("Unknown board");
		}
	}

	for (i = 0; i < dev_count; i++) {
		dev = PCIE_BDF(p_dev_list[i].bus_num,
					   p_dev_list[i].dev_num,
					   p_dev_list[i].fn_num);
		pcie_read_config_word(dev, PCIE_VENDOR_ID, &vendor);

		if (vendor != p_dev_list[i].ven_id) {
			post_log("Expected vendor id 0x%04x [got = 0x%04x]\n",
					 p_dev_list[i].ven_id, vendor);
			post_log("Expected %s not found @ bus = %d, device = %d, \
				function = %d \n",
				p_dev_list[i].descr,
				p_dev_list[i].bus_num,
				p_dev_list[i].dev_num,
				p_dev_list[i].fn_num);
			continue;
		}

		pcie_read_config_word(dev, PCIE_DEVICE_ID, &device);

		if (!p_dev_list[i].dontcare_devid) {
			if (device != p_dev_list[i].dev_id) {
				post_log("Expected device id 0x%04x [got = 0x%04x] \n",
						 p_dev_list[i].dev_id, device);
				post_log(
					"Expected %s not found @ bus = %d, device = %d, function = %d \n",
					p_dev_list[i].descr,
					p_dev_list[i].bus_num,
					p_dev_list[i].dev_num,
					p_dev_list[i].fn_num);
				continue;
			}
		}

		pass++;
		post_log("Device - %s found @ bus = %d, device = %d, function = %d \n",
				 p_dev_list[i].descr,
				 p_dev_list[i].bus_num,
				 p_dev_list[i].dev_num,
				 p_dev_list[i].fn_num);


		{
			WATCHDOG_RESET();
 #ifdef CONFIG_SHOW_ACTIVITY
			extern void show_activity(int arg);
			show_activity(1);
 #endif
		}
	}

	if (pass == dev_count) {
		post_log("PCIe Probe verification successful\n");
		ret = 0;
	} else   {
		post_log("PCIe Probe verification failed\n");
		ret = 1;
	}

	return (ret);
}

#endif /* CONFIG_POST & CFG_POST_PCI */
#endif /* CONFIG_POST */
