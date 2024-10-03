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

#ifndef __EEPROM_H_
#define __EEPROM_H_

#define CPU_BOARD_REV(maj, min)	((((maj)&0xff) << 8) | ((min) & 0xff))
#define CPU_BOARD_MAJOR(rev)	(((rev) >> 8) & 0xff)
#define CPU_BOARD_MINOR(rev)	((rev) & 0xff)


/* 
 * Juniper ID EEPROM structure  
 * XXX: jnpr_caffeine - This is used only for Initial debugging. 
 * Will be removed later
*/
typedef struct {
	char jedec_h;		/* OFF 0x00: Juniper JEDEC code, bits 15-8 */
	char jedec_l;		/* OFF 0x01: Juniper JEDEC code, bits 7-0 */
	char id_version;	/* OFF 0x02: ID EEPROM format version */
	char reserved_1;
	char assembly_id_h;	/* OFF 0x04: Assembly ID, bits 15-8 */
	char assembly_id_l;	/* OFF 0x05: Assembly ID, bits 7-0 */
	char major_version;	/* OFF 0x06: Assembly Major version */ 
	char minor_version;	/* OFF 0x07: Assembly Minor version */ 
	char rev_string[12];	/* OFF 0x08 - 0x13: Assembly revision string */
	char part_number[12];	/* OFF 0x14 - 0x1F: Assembly Part number string */
	char serial_number[12];	/* OFF 0x20 - 0x2B: Assembly Serial number string */
	char flags;		/* OFF 0x2C: Assembly flags */
	char mfg_day;		/* OFF 0x2D: Day of Manufacture */ 
	char mfg_month;		/* OFF 0x2E: Month of Manufacture */
	char mfg_year_h;	/* OFF 0x2F: Year of Manufacture, bits 15-8 */
	char mfg_year_l;	/* OFF 0x30: Year of Manufacture, bits 7-0 */
	char reserved_2;
	char reserved_3;
	char reserved_4;
	char board_info[16];	/* OFF 0x34 - 0x43: Board specific Info */
	char mfg_info[48];	/* OFF 0x44 - 0x73: Manufacturing specific Info */	
	char chassis_serial[12];/* OFF 0x74 - 0x7F: Chassis serial number*/
	char reserved_fil[0x80];
} id_eeprom_t;

/*
 * Returns board version register.
 */
unsigned int get_board_version(void);

/*
 * Returns either 33000000 or 66000000 as the SYS_CLK_FREQ.
 */
unsigned long get_clock_freq(void);


/*
 * Returns 1 - 4, as found in the USER CSR[6:7] bits.
 */
unsigned int get_pci_slot(void);


/*
 * Returns PCI DUAL as found in CM_PCI[3].
 */
unsigned int get_pci_dual(void);

unsigned int get_cpu_board_revision(void);

#endif	/* __EEPROM_H_ */
