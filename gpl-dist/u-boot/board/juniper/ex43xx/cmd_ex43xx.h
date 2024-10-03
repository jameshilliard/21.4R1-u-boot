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
 */

#ifndef __CMD_EX43XX__
#define __CMD_EX43XX__

#define MAX_BUF_LEN		256
#define BASIC_HDR_LEN		4
#define SEC_IC_NO_OFFSET	-1

/* eeprom commands */
#define  GET_M2_VERSION		0xC0
#define  WRITE_EEPROM_DATA	0xC1
#define  HW_AUTHENTICATE	0xC2
#define  SW_AUTHENTICATE	0xC3
#define  GET_HW_PUB_INFO	0xC4
#define  GET_HW_PUB_SIGN	0xC5
#define  GET_HW_UID		0xC6 
#define  GET_CHIP_STATUS	0xC7

/* eeprom error message */
#define  STATUS_SUCCESS		0xE0
#define  STATUS_ERROR		0xE1
#define  STATUS_NOT_AVAILABLE	0xE2   /* secure eeprom is busy */ 
#define  STATUS_WRITE_ERROR	0xE3
#define  STATUS_READ_ERROR	0xE4 

/* PCIe */
#define DISP_LINE_LEN		16

/* flash */
#define FLASH_CMD_PROTECT_SET	0x01
#define FLASH_CMD_PROTECT_CLEAR	0xD0
#define FLASH_CMD_CLEAR_STATUS	0x50
#define FLASH_CMD_PROTECT	0x60

#define AMD_CMD_RESET		0xF0
#define AMD_CMD_WRITE		0xA0
#define AMD_CMD_ERASE_START	0x80
#define AMD_CMD_ERASE_SECTOR	0x30
#define AMD_CMD_UNLOCK_START	0xAA
#define AMD_CMD_UNLOCK_ACK	0x55

#define AMD_STATUS_TOGGLE	0x40

extern int flash_write_direct(ulong addr, char *src, ulong cnt);

extern void rtc_init(void);
extern void rtc_stop(void);
extern void rtc_start(void);
extern void rtc_set_time(int yy, int mm, int dd, int hh, int mi, int ss);

extern char *slot_to_name(uint16_t id, int slot);

#if !defined(CONFIG_PRODUCTION)
extern int atoh(char* string);
extern int  i2c_mux(int ctlr, uint8_t mux, uint8_t chan, int ena);
extern int m1_40_present(void);
extern int m1_80_present(void);
extern int m2_present(void);
extern int m1_40_id(int *rev, uint16_t *id, uint8_t *addr);
extern int m1_80_id(int *rev, uint16_t *id, uint8_t *addr);
extern int m2_id(int *rev, uint16_t *id, uint8_t *addr);
extern int i2c_read_temp_m1_40(uint16_t *id);
extern int i2c_read_temp_m1_80(uint16_t *id);
extern int i2c_read_temp_m2(uint16_t *id);
#endif /* CONFIG_PRODUCTION */

#endif /*__CMD_EX43XX__*/
