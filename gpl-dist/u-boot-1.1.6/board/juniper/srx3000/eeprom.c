/*
 * Copyright (c) 2007-2010, Juniper Networks, Inc.
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
#include <i2c.h>
#include <watchdog.h>

#include "fsl_i2c.h"
#include "eeprom.h"

DECLARE_GLOBAL_DATA_PTR;

#ifdef CFG_JAUS_ID_EEPROM
/*
 * This function validates the EEPROM for 
 * valid Juniper IDEEPROM data.
 */
uint eeprom_valid (uchar i2c_addr)
{
	uchar buf[2];    
	buf[0] = buf[1] = 0x00;
	
	i2c_read(i2c_addr, 0, 1, (uchar*)buf, sizeof(buf));
	if((buf[0] == 0x7F) && (buf[1] == 0xB0)) {
		return 1;	/*Valid Juniper Format*/
	} else {
		return 0;	/*Invalid Data*/
	}	    
}

/*
 * This function dumps the IDEEPROM data.
 * XXX: jnpr_caffeine - This function is used only for debug and 
 * will be removed later.
 */
void dump_ideeprom_data ()
{
	uchar i2c_addr, *buf;
	uint cnt;
	id_eeprom_t id_eeprom_data;

	buf = (uchar*)&id_eeprom_data;

	i2c_controller(CFG_I2C_CTRL_1);
	i2c_addr = ID_EEPROM_ADDR;
	i2c_read(i2c_addr, 0x00, 1,
		(uchar*)&id_eeprom_data, sizeof(id_eeprom_data)); 
	printf("\nIDEEPROM fields:\n");
	printf("IDEEPROM format: 0x%X\n",id_eeprom_data.id_version);
	printf("Assembly ID: 0x%X,0x%X\n",id_eeprom_data.assembly_id_h, id_eeprom_data.assembly_id_l);
	printf("Version No: 0x%X,0x%X\n",id_eeprom_data.major_version, id_eeprom_data.minor_version);
	printf("Rev: %s\n", id_eeprom_data.rev_string);
	printf("Part No: %s\n",id_eeprom_data.part_number);
	printf("Serial No: %s\n",id_eeprom_data.serial_number);
	printf("Mfg date(DD/MM/YYYY): %X/%X/%X%X\n",id_eeprom_data.mfg_day, 
		id_eeprom_data.mfg_month, id_eeprom_data.mfg_year_h, id_eeprom_data.mfg_year_l);
	printf("\nIDEEPROM raw data:\n");
	for(cnt = 1;cnt <= sizeof(id_eeprom_data);cnt++) {
		printf("%02X ", buf[cnt]);
		if(!(cnt % 0x10))
			printf("\n");
	}
}

unsigned int
get_board_version (void)
{
	volatile cadmus_reg_t *cadmus = (cadmus_reg_t *)CFG_CADMUS_BASE_REG;

	return cadmus->cm_ver;
}


unsigned long
get_clock_freq (void)
{
	volatile cadmus_reg_t *cadmus = (cadmus_reg_t *)CFG_CADMUS_BASE_REG;

	uint pci1_speed = (cadmus->cm_pci >> 2) & 0x3; /* PSPEED in [4:5] */

	if (pci1_speed == 0) {
		return 33000000;
	} else if (pci1_speed == 1) {
		return 66000000;
	} else {
		/* Really, unknown. Be safe? */
		return 33000000;
	}
}


unsigned int
get_pci_slot (void)
{
	volatile cadmus_reg_t *cadmus = (cadmus_reg_t *)CFG_CADMUS_BASE_REG;

	/*
	 * PCI slot in USER bits CSR[6:7] by convention.
	 */
	return ((cadmus->cm_csr >> 6) & 0x3) + 1;
}

unsigned int
get_pci_dual (void)
{
	volatile cadmus_reg_t *cadmus = (cadmus_reg_t *)CFG_CADMUS_BASE_REG;

	/*
	 * PCI DUAL in CM_PCI[3]
	 */
	return cadmus->cm_pci & 0x10;
}

unsigned int
get_cpu_board_revision (void)
{
	uint major = 0;
	uint minor = 0;

	id_eeprom_t id_eeprom;

	i2c_read(CFG_I2C_EEPROM_ADDR, 0, 2,
		 (uchar *) &id_eeprom, sizeof(id_eeprom));

	major = id_eeprom.idee_major;
	minor = id_eeprom.idee_minor;

	if (major == 0xff && minor == 0xff) {
		major = minor = 0;
	}

	return MPC85XX_CPU_BOARD_REV(major,minor);
}


#else
unsigned int
get_board_version (void)
{
	return (1);
}


unsigned long
get_clock_freq (void)
{
	return (66666667);
}


unsigned int
get_pci_slot (void)
{
	return (1);
}


unsigned int
get_pci_dual (void)
{
	return (0);
}

unsigned int
get_cpu_board_revision (void)
{

	return (0x0000);
}

#endif /*CFG_JAUS_ID_EEPROM*/
