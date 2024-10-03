/*
 * Copyright (c) 2009-2010, Juniper Networks, Inc.
 * All rights reserved.
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
 * You should have received a copy of the GNU General Public
 * License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __EEPROM_FX_H__
#define __EEPROM_FX_H__

#include "rmi/types.h"

#define EEPROM_LOG_INFO_OFFSET    0x50
#define EEPROM_LOG_AREA_START     0x60
#define EEPROM_LOG_AREA_SIZE      1024
#define EEPROM_LOG_AREA_END       (EEPROM_LOG_AREA_START + EEPROM_LOG_AREA_SIZE)

#define ERASE_LOG                 0x00000001

typedef struct {
    int  nfb_offset;  // next free byte offset
    int  nom;         // number of msgs
    char resrvd[8];   // Reserved for future use
} tlogInfo_ST;


typedef struct eeprom_dev_api {
    int  (*read)(int index, int offset, char *, int nbytes);
    int (*write)(int index, int offset, char *, int nbytes);
    int (*erase)(int index, int start,  int nbytes);
} tEEPROM_i2c_API_ST;


typedef struct eeprom_on_i2c {
    uint     i2c_bus;
    uint16_t i2c_addr;
    uint     flag;
    void     *eeprom_api;
} tEEPROM_i2c_DEV_ST;


extern tEEPROM_i2c_DEV_ST eeprom[] ;
extern tEEPROM_i2c_API_ST api;

extern int i2c_eeprom_read(uint index, int offset, char *get_buffer, int nbytes);
extern int i2c_eeprom_write(uint index,
                            int offset,
                            char *put_buffer,
                            int nbytes);
extern int i2c_eeprom_erase(uint index, int start, int nbytes);

#endif



