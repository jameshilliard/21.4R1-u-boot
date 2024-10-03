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
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/gpl-2.0.html>.
 */

#ifndef __EX3300_LOCAL__
#define __EX3300_LOCAL__

extern int interrupt_init (void);

extern int i2c_read_norsta(uint8_t chanNum, uchar dev_addr, unsigned int offset, int alen, uchar* data, int len);
extern int i2c_mux(int ctrl, uint8_t mux, uint8_t chan, int ena);
extern void rtc_init(void);
extern void rtc_stop(void);
extern void rtc_start(void);
extern void rtc_set_time(int yy, int mm, int dd, int hh, int mi, int ss);
extern void syspld_watchdog_petting(void);

extern int flash_write_direct (ulong addr, char *src, ulong cnt);

extern void syspld_set_fan (int fan, int speed);

#define SPI_FLASH_SW_PROTECT_START  0x700000
#define SPI_FLASH_SW_PROTECT_END    0x7FFFFF
#define SPI_FLASH_SW_PROTECT_INC    0x10000

#if defined(CONFIG_SPI_SW_PROTECT)
extern int ex_spi_sw_protect_set (uint32_t offset, uint8_t *pdata, uint32_t len);
extern int ex_spi_sw_protect_get (uint32_t offset, uint8_t *pdata, uint32_t len);
#endif

#endif /*__EX3300_LOCAL__*/
