/*
 * Copyright (c) 2011-2013, Juniper Networks, Inc.
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
 *
 * ex45xx_local
 *
 */

#ifndef __EX45XX_LOCAL__
#define __EX45XX_LOCAL__

extern void mdio_mode_set (uint8_t mode, int inuse);
extern uint8_t mdio_mode_get (void);
extern int mdio_get_inuse(void);
extern int secure_eeprom_read (unsigned dev_addr, unsigned offset, 
                               uchar *buffer, unsigned cnt);

/* misc */
extern int valid_elf_image (unsigned long addr);
extern void i2c_sfp_temp (void);
extern int cmd_get_data_size (char* arg, int default_size);

#endif /*__EX45XX_LOCAL__*/
