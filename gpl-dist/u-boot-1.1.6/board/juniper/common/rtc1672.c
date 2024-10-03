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

#include <common.h>

#ifdef CONFIG_RTC1672
#include <i2c.h>
#ifdef CONFIG_SRX3000
#include <asm/fsl_i2c.h>
#else
#include <fsl_i2c.h>
#endif
#include "rtc1672.h"
#include <rtc.h>

#undef  RTC_DEBUG

DECLARE_GLOBAL_DATA_PTR;

static int rtc_read_byte(u8 dev, uint addr, u8* data)
{
	i2c_controller(CFG_I2C_CTRL_1);
	return i2c_read(dev, addr, 1, data, 1);
}


static int rtc_write_byte(u8 dev, uint addr, u8* data)
{
	i2c_controller(CFG_I2C_CTRL_1);
	return i2c_write(dev, addr, 1, data, 1);
}


/* ------ convert binary time to date format ------ */
void day2date(unsigned long x,
			  uint* p_yrs,
			  uint* p_mon,
			  uint* p_day,
			  uint* p_hrs,
			  uint* p_min,
			  uint* p_sec)
{
	int yrs = 99, mon = 99, day = 99, tmp, jday, hrs, min, sec;
	unsigned long j, n;


	j = x / 60;         /* whole minutes since 1/1/70 */
	sec = x - (60 * j); /* leftover seconds */

	n = j / 60;
	min = j - (60 * n);

	j = n / 24;
	hrs = n - (24 * j);

	j = j + (365 + 366); /* whole days since 1/1/68 */
	day = j / ((4 * 365) + 1);

	tmp = j % ((4 * 365) + 1);
	if (tmp >= (31 + 29)) {         /* if past 2/29 */
		day++;                      /* add a leap day */
	}
	yrs = (j - day) / 365;          /* whole years since 1968 */

	jday = j - (yrs * 365) - day;   /* days since 1/1 of current year */
	if (tmp <= 365 && tmp >= 60) {  /* if past 2/29 and a leap year then */
		jday++;                     /* add a leap day */
	}
	yrs += 1968;                    /* calculate year */


	for (mon = 12; mon > 0; mon--) {
		switch (mon) {
		case 1: tmp = 0; break;
		case 2: tmp = 31; break;
		case 3: tmp = 59; break;
		case 4: tmp = 90; break;
		case 5: tmp = 120; break;
		case 6: tmp = 151; break;
		case 7: tmp = 181; break;
		case 8: tmp = 212; break;
		case 9: tmp = 243; break;
		case 10: tmp = 273; break;

		case 11: tmp = 304; break;
		case 12: tmp = 334; break;
		}
		if ((mon > 2) && !(yrs % 4)) { /* adjust for leap year */
			tmp++;
		}
		if (jday >= tmp) {
			break;
		}
	}

	day = jday - tmp + 1; /* calculate day in month */

	*p_yrs = yrs;
	*p_mon = mon;
	*p_day = day,
	*p_hrs = hrs,
	*p_min = min,
	*p_sec = sec;
}


/* ---- convert date to elapsed days in binary ---- */
unsigned long date2day(uint yr, uint mo, uint da, uint hrs, uint min, uint sec)
{
	unsigned long x;

	/* the following is broken down for clarity */
	x = 365 * (yr - 1970); /* calculate number of days for previous
						    * years */
	x += (yr - 1969) >> 2;              /* add a day for each leap year */
	if ((mo > 2) && (yr % 4 == 0)) {    /* add a day if current year is leap and past Feb 29th */
		x++;
	}

	switch (mo) {
	case 1: x += 0; break;
	case 2: x += 31; break;
	case 3: x += 59; break;
	case 4: x += 90; break;
	case 5: x += 120; break;
	case 6: x += 151; break;
	case 7: x += 181; break;
	case 8: x += 212; break;
	case 9: x += 243; break;
	case 10: x += 273; break;
	case 11: x += 304; break;
	case 12: x += 334; break;
	}
	x += da - 1; /* finally, add the days into the
				  * current month */
	x = x * 86400; /* and calculate the number of seconds in
				    * all those days */
	x += (hrs * 1800); /* add the number of seconds in the hours
						*/
	x += (hrs * 1800); /* add the number of seconds in the hours
						*/
	x += (min * 60);    /* ditto the minutes */
	x += sec;           /* finally, the seconds */
	return x;
}


unsigned long get_rtc1672_count_as_word(u8 dev)
{
	unsigned long word;
	uchar reg1, reg2, reg3, reg4;

	rtc_read_byte(dev, 0x00, &reg1);
	rtc_read_byte(dev, 0x01, &reg2);
	rtc_read_byte(dev, 0x02, &reg3);
	rtc_read_byte(dev, 0x03, &reg4);

	word = reg4;
	word <<= 8;
	word += reg3;
	word <<= 8;
	word += reg2;
	word <<= 8;
	word += reg1;

	return word;
}


void set_rtc1672_word_as_count(u8 dev, unsigned long word)
{
	uchar reg1, reg2, reg3, reg4;

	reg1 = (word & 0xff);
	reg2 = ((word >> 8) & 0xff);
	reg3 = ((word >> 16) & 0xff);
	reg4 = ((word >> 24) & 0xff);

	/* write to the counter registers */
	rtc_write_byte(dev, 0x00, &reg1);
	rtc_write_byte(dev, 0x01, &reg2);
	rtc_write_byte(dev, 0x02, &reg3);
	rtc_write_byte(dev, 0x03, &reg4);

	return;
}


/* show clock*/
void disp_clk_regs(void)
{
	uchar reg1, prv_sec = 99, reg2, reg3, reg4;
	unsigned long z;
	uint yrs, mon, day, hrs, min, sec;

	printf("\n");
	while (1) {   /* Read & Display Clock Registers */
		if (ctrlc()) {
			putc('\n');
			return;
		}

		rtc_read_byte(RTC_I2C_ADDR, 0x00, &reg1);
		rtc_read_byte(RTC_I2C_ADDR, 0x01, &reg2);
		rtc_read_byte(RTC_I2C_ADDR, 0x02, &reg3);
		rtc_read_byte(RTC_I2C_ADDR, 0x03, &reg4);

		if (reg1 != prv_sec) { /* display every time seconds change */
			z = reg4;
			z <<= 8;
			z += reg3;
			z <<= 8;
			z += reg2;
			z <<= 8;
			z += reg1;
			day2date(z, &yrs, &mon, &day, &hrs, &min, &sec);

			printf("\r%04d %02d %02d %02d:%02d:%02d",
				   yrs,
				   mon,
				   day,
				   hrs,
				   min,
				   sec);
		}
		prv_sec = reg1;
	}

	printf("\n");
}

void
rtc_test (void)
{
	uchar reg1, prv_sec = 99, reg2, reg3, reg4 = 0;
	unsigned long z;
	uint yrs, mon, day, hrs, min, sec,temp;
	int rtc_flag_fault = 0;
	printf("\n");
	printf("Testing RTC by adding 6sec delay\n");
	printf("Read the time after 6sec delay the time should match\n");
	while (1) {   /* Read & Display Clock Registers */
		if (ctrlc()) {
			putc('\n');
			return;
		}
		rtc_read_byte(RTC_I2C_ADDR, 0x00, &reg1);
		rtc_read_byte(RTC_I2C_ADDR, 0x01, &reg2);
		rtc_read_byte(RTC_I2C_ADDR, 0x02, &reg3);
		rtc_read_byte(RTC_I2C_ADDR, 0x03, &reg4);

		if (reg1 != prv_sec) { /* display every time seconds change */
			z = reg4;
			z <<= 8;
			z += reg3;
			z <<= 8;
			z += reg2;
			z <<= 8;
			z += reg1;
			day2date(z, &yrs, &mon, &day, &hrs, &min, &sec);
			if (rtc_flag_fault) {
				if (sec < temp) {
					printf("\n\n");
					printf("Fault in RTC\n"); 
					goto stop;
				}
			}
			rtc_flag_fault = 1;
			printf("\r%04d %02d %02d %02d:%02d:%02d",
					yrs,
					mon,
					day,
					hrs,
					min,
					sec);
		}
		prv_sec = reg1;
		temp = sec;
		/* Wait for 6sec */
		udelay(6000000);
		/* Boundary conditions add correct offset to temp*/
		if (sec > 53 && sec <= 59) {
			switch (sec) {
				case 54: temp = 0; break;
				case 55: temp = 1; break;
				case 56: temp = 2; break;
				case 57: temp = 3; break;
				case 58: temp = 4; break;
				case 59: temp = 5; break;
			}
		} else {
			temp += 6;
		}
	}
stop:
	printf("\n");
}


/* ----- enable the trickle-charger ------ */
static void en_rtc_1672_tc(unsigned char dat)
{
	uchar data = dat;

	rtc_write_byte(RTC_I2C_ADDR, 0x05, &data);
}


/* ----- enable the oscillator ------ */
static void en_rtc_1672_osc(unsigned char dat)
{
	uchar data = (~(dat & 0x1)) << 7;

	rtc_write_byte(RTC_I2C_ADDR, 0x04, &data);
}


void en_rtc_1672(void)
{
	en_rtc_1672_tc(0xa6);
	en_rtc_1672_osc(1);
}


void dis_rtc_1672(void)
{
	en_rtc_1672_tc(0);
	en_rtc_1672_osc(0);
}


void rtc_get(struct rtc_time* tmp)
{
	uint year, mon, mday, hour, min, sec;
	unsigned long curr_word;

#ifdef CONFIG_EX82XX
	if (!EX82XX_RECPU) {
		return;
	}
#endif

	curr_word = get_rtc1672_count_as_word(RTC_I2C_ADDR);
	day2date(curr_word, &year, &mon, &mday, &hour, &min, &sec);

	curr_word =  mktime(year, mon, mday, hour, min, sec);
	to_tm(curr_word, tmp);

	tmp->tm_yday = 0;
	tmp->tm_isdst = 0;

#ifdef RTC_DEBUG
	printf("Get DATE: %4d-%02d-%02d (wday=%d)  TIME: %2d:%02d:%02d\n",
		   tmp->tm_year, tmp->tm_mon, tmp->tm_mday, tmp->tm_wday,
		   tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
#endif
}


void rtc_set(struct rtc_time* tmp)
{
	uint year, mon, mday, hour, min, sec;
	unsigned long curr_word;

#ifdef CONFIG_EX82XX
	if (!EX82XX_RECPU) {
		return;
	}
#endif

 #ifdef RTC_DEBUG
	printf("Set DATE: %4d-%02d-%02d (wday=%d)  TIME: %2d:%02d:%02d\n",
		   tmp->tm_year, tmp->tm_mon, tmp->tm_mday, tmp->tm_wday,
		   tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
 #endif

	year = tmp->tm_year;
	mon  = tmp->tm_mon;
	mday = tmp->tm_mday;
	hour = tmp->tm_hour;
	min  = tmp->tm_min;
	sec  = tmp->tm_sec;

	curr_word = date2day(year, mon, mday, hour, min, sec);
	set_rtc1672_word_as_count(RTC_I2C_ADDR, curr_word);
}


void rtc_reset(void)
{
	struct rtc_time tmp;
	uchar ctrl_rg;

#ifdef CONFIG_EX82XX
	if (!EX82XX_RECPU) {
		return;
	}
#endif

	tmp.tm_year = 1970;
	tmp.tm_mon = 1;
	tmp.tm_mday = 1;
	tmp.tm_hour = 0;
	tmp.tm_min = 0;
	tmp.tm_sec = 0;

#ifdef RTC_DEBUG
	printf("RTC:   %4d-%02d-%02d %2d:%02d:%02d UTC\n",
		   tmp.tm_year, tmp.tm_mon, tmp.tm_mday,
		   tmp.tm_hour, tmp.tm_min, tmp.tm_sec);
#endif

	rtc_set(&tmp);
}

#endif /*CONFIG_RTC1672*/
