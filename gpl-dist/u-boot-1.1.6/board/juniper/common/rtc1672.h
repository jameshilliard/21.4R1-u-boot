/*
 * Copyright (c) 2008-2010, Juniper Networks, Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/gpl-2.0.html>.
 */

#ifndef __RTC1672_H__
#define __RTC1672_H__

extern void day2date(unsigned long x,uint* p_yrs, uint* p_mon,uint* p_day,uint* p_hrs, uint* p_min, uint* p_sec);
extern unsigned long date2day(uint yr, uint mo, uint da, uint hrs, uint min, uint sec);
extern unsigned long get_rtc1672_count_as_word (u8 dev);
extern void set_rtc1672_word_as_count (u8 dev, unsigned long word);
extern void disp_clk_regs(void);
extern void en_rtc_1672(void);
extern void dis_rtc_1672(void);
extern void rtc_test(void);

#endif
