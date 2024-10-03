/*
 * Copyright (c) 2008-2011, Juniper Networks, Inc.
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

#ifndef __RPS_CPLD__
#define __RPS_CPLD__


enum {
    I2C_ADDRESS = 0x00,
    I2C_STATUS = 0x01,
    PORT_1_2_PRIORITY = 0x02,
    PORT_3_4_PRIORITY = 0x03,
    PORT_5_6_PRIORITY = 0x04,
    PORT_1_2_3_STATE = 0x05,
    PORT_4_5_6_STATE = 0x06,
    PORT_1_2_3_DEBUG = 0x07,
    PORT_4_5_6_DEBUG = 0x08,
    RPS_FANFAIL_LED = 0x09,
    PORT_1_2_LED = 0x0A,
    PORT_3_4_LED = 0x0B,
    PORT_5_6_LED = 0x0C,
    WRITE_PROTECT = 0x0D,
    MISC_ENABLE = 0x0E,
    UNIT_PRESENT = 0x0F,
    PORT_1_2_VOLTAGE_TRIP_POINT = 0x10,
    PORT_3_4_VOLTAGE_TRIP_POINT = 0x11,
    PORT_5_6_VOLTAGE_TRIP_POINT = 0x12,
    PS_WORKING = 0x13,
    PS_FAN_FAILURE = 0x14,
    MAX_ACTIVE = 0x15,
    SWITCH_I2C_ADDRESS = 0x16,
    SWITCH_I2C_RESET = 0x17,
    SWITCH_I2C_TIMEOUT = 0x18,
    CPLD_DUMMY_1 = 0x19,
    CPLD_DUMMY_2 = 0x1A,
    CPLD_DUMMY_3 = 0x1B,
    CPLD_DUMMY_4 = 0x1C,
    CPLD_DUMMY_5 = 0x1D,
    CPLD_DUMMY_6 = 0x1E,
    CPLD_52V_CFG = 0x1F,
    PORT_1_IN_MAILBOX = 0x20,
    PORT_1_OUT_MAILBOX = 0x21,
    PORT_2_IN_MAILBOX = 0x22,
    PORT_2_OUT_MAILBOX = 0x23,
    PORT_3_IN_MAILBOX = 0x24,
    PORT_3_OUT_MAILBOX = 0x25,
    PORT_4_IN_MAILBOX = 0x26,
    PORT_4_OUT_MAILBOX = 0x27,
    PORT_5_IN_MAILBOX = 0x28,
    PORT_5_OUT_MAILBOX = 0x29,
    PORT_6_IN_MAILBOX = 0x2A,
    PORT_6_OUT_MAILBOX = 0x2B,
    CPLD_DUMMY_9 = 0x2C,
    CPLD_DUMMY_10 = 0x2D,
    CPLD_DUMMY_11 = 0x2E,
    CPLD_DUMMY_12 = 0x2F,
    CPLD_REVISION = 0x30,
    SCRATCH = 0x31,
    CPLD_CREATION_DAY = 0x32,
    CPLD_CREATION_MONTH = 0x33,
    CPLD_CREATION_CENTURY = 0x34,
    CPLD_CREATION_YEAR = 0x35,
    CPLD_LAST = 0x36,
};

#define CPLD_LAST_REG                           (CPLD_CREATION_YEAR)
#define CPLD_FIRST_PRIORITY_REG       (PORT_1_2_PRIORITY)
#define CPLD_FIRST_STATE_REG       (PORT_1_2_3_STATE)
#define CPLD_FIRST_PORT_LED_REG       (PORT_1_2_LED)
#define CPLD_FIRST_IN_MAILBOX_REG       (PORT_1_IN_MAILBOX)
#define CPLD_FIRST_OUT_MAILBOX_REG       (PORT_1_OUT_MAILBOX)
#define CPLD_FIRST_TP_REG       (PORT_1_2_VOLTAGE_TRIP_POINT)

#define CPLD_MAX_PORT 6
#define CPLD_MAX_PS 3

/* PS_WORKING */
#define PS1_PRESENT         (1<<0)
#define PS1_POWER_GOOD (1<<1)
#define PS2_PRESENT         (1<<2)
#define PS2_POWER_GOOD (1<<3)
#define PS3_PRESENT         (1<<4)
#define PS3_POWER_GOOD (1<<5)

/* PS_FAN_FAILURE */
#define PS1_FAN_FAILURE         (1<<0)
#define PS2_FAN_FAILURE         (1<<1)
#define PS3_FAN_FAILURE         (1<<2)

struct cpld_t {
	union {
		uint8_t data[CPLD_LAST];
		struct {
			uint8_t i2c_addr;
			uint8_t i2c_status;
			uint8_t port_priority[3];
			uint8_t port_state[2];
			uint8_t port_debug[2];
			uint8_t rps_fan_led;
			uint8_t port_led[3];
			uint8_t write_protect;
			uint8_t misc_en;
			uint8_t unit_present;
			uint8_t port_volt_trip_point[3];
			uint8_t ps_working;
			uint8_t ps_fan_fail;
			uint8_t max_active;
			uint8_t switch_i2c_addr;
			uint8_t i2c_sw_reset;
			uint8_t i2c_port_timeout;
			uint8_t dummy[7];
			uint8_t port_inbox[6];
			uint8_t port_outbox[6];
			uint8_t dummy_1[4];
			uint8_t id;
			uint8_t scratch;
			uint8_t day;
			uint8_t month;
			uint8_t century;
			uint8_t year;
		} cpld;
	} u;
};

extern struct cpld_t rps_pld;

extern int cpld_init(void);
extern int cpld_reg_read(int32_t reg, uint8_t *val);
extern int cpld_reg_write(int32_t reg, uint8_t val);
extern int cpld_read(int32_t offset, uint8_t *data, int32_t length);
extern int cpld_write(int32_t offset, uint8_t *data, int32_t length);
extern int cpld_reg_read_direct(int32_t reg, uint8_t *val);
extern int cpld_read_direct(int32_t offset, uint8_t *data, int32_t length);

extern uint8_t cpld_get_reg_offset(int32_t bits, int32_t port);
extern uint8_t cpld_get_reg_shift(int32_t bits, int32_t port);
extern uint8_t cpld_get_reg_mask(int32_t bits, int32_t port);

#endif /*__RPS_CPLD__*/
