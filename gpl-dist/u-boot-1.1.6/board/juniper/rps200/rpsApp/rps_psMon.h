/*
 * Copyright (c) 2008-2012, Juniper Networks, Inc.
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

#ifndef __RPS_PSMON_H
#define __RPS_PSMON_H

#define DAC_0_ADDR      0xC
#define DAC_1_ADDR      0xD

#define I2C_ID_EX32X42X_PWR_320         0x041F  /* Java 320W Power Module*/
#define I2C_ID_EX32X42X_PWR_600         0x0420  /* Java 600W Power Module */
#define I2C_ID_EX32X42X_PWR_930         0x0421  /* Java 930W Power Module */
#define I2C_ID_EX32X42X_PWR_190_DC  0x042c  /* Java 190W DC Power Module */

#define PS_1 0
#define PS_2 1
#define PS_3 2

#define PS_1_MASK 0x40
#define PS_3_MASK 0x80
#define PS_MODEL_SIZE       23
#define PS_MODEL_OFFSET     0x4F

extern void initPSMon(void);
extern void startPSMon(int32_t start_time, int32_t cycle_time);
extern void stopPSMon(void);
extern void psUpdate(uint8_t ps, uint8_t fan);
extern void psPower(uint16_t *walts_12V, uint16_t *walts_48V);
extern uint16_t getPSID(int32_t slot);
extern uint16_t getPSStatus(void);
extern uint8_t getPSNumber(void);
extern void setPSEnable(int ps, int ena);

#endif	/* __RPS_PSMON_H */
