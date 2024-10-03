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
#include <brgphyreg.h>
#include <led.h>
#include <tsec.h>
#include <ex82xx_devices.h>
#include <ex82xx_i2c.h>

uint8_t global_status_led_st = 0;
extern struct tsec_private *mgmt_priv;
DECLARE_GLOBAL_DATA_PTR;

void set_port_led(uint32_t link_stat_spd)
{
	uint16_t expansion_reg_04h = 0, tmp;
	uint link, link_speed;

	link = link_stat_spd & 0x4;
	link_speed = ((link_stat_spd & 0x0700) >> 8);

	if (!link) { /* LINK DOWN */
		link_speed = SPEED_LED_OFF;		
	}

	switch (link_speed) {
	case MIIM_BCM5466R_PHY_AUX_STATUS_FULLDUPLEX_1000: 	/* LED ON */
	case MIIM_BCM5466R_PHY_AUX_STATUS_HALFDUPLEX_1000:
		expansion_reg_04h = MULTICOLOR_LED_FORCED_ON;
		break;
	case MIIM_BCM5466R_PHY_AUX_STATUS_FULLDUPLEX_100:	/* LED Blink */
	case MIIM_BCM5466R_PHY_AUX_STATUS_HALFDUPLEX_100:
		expansion_reg_04h = MULTICOLOR_LED_ALTERNATING;
		break;
	case MIIM_BCM5466R_PHY_AUX_STATUS_FULLDUPLEX_10:	/* LED OFF */
	case MIIM_BCM5466R_PHY_AUX_STATUS_HALFDUPLEX_10:
	case SPEED_LED_OFF:
	default:
		expansion_reg_04h = MULTICOLOR_LED_FORCED_OFF;
		break;
	}
	/* 0x17 = 0x0F04 - select exp reg 0x4 */
	write_phy_reg(mgmt_priv, BCM_EXPANSION_REG_ACCESS_REG_17, 
	    (BCM_EXPANSION_REG_SEL + BCM_EXP_REG_MULTICOLOR_SEL));

	/* 0x15(exp reg 0x4) = Link speed */
	write_phy_reg(mgmt_priv, BCM_EXPANSION_REG_ACCESS_15, expansion_reg_04h);

	/* 0x17 = 0x0 */
	write_phy_reg(mgmt_priv, BCM_EXPANSION_REG_ACCESS_REG_17, (0x00));
}

void update_led_state(uint8_t led_state)
{
	uint8_t led_state_t;
	uint8_t *ctrl_cpld_led, tmp;
	uint8_t slot, group, device, tdata[4];
	uint8_t offset, i2cs_r45, i2cs_r46, i2cs_r5d, i2cs_r5e;
	int ret;
	uint8_t *a = (uint8_t*)&offset;

	led_state_t = led_state & 0x0f;

	if (EX82XX_RECPU) {
		switch (led_state_t) {
		case LED_GREEN_ON:
			i2cs_r45 = POWER_ON_LED_GREEN | STATUS_LED_GREEN;
			i2cs_r46 = 0x00;
			i2cs_r5d = 0x00;
			break;
		case LED_YELLOW_ON:
			i2cs_r45 = POWER_ON_LED_GREEN;
			i2cs_r46 = STATUS_LED_YELLOW;
			i2cs_r5d = 0x00;
			break;
		case LED_GREEN_BLINK:
			i2cs_r45 = POWER_ON_LED_GREEN | STATUS_LED_GREEN;
			i2cs_r46 = 0x00;
			i2cs_r5d = STATUS_LED_BLINK;
			break;
		case LED_YELLOW_BLINK:
			i2cs_r45 = POWER_ON_LED_GREEN;
			i2cs_r46 = STATUS_LED_YELLOW;
			i2cs_r5d = STATUS_LED_BLINK; 
			break;
		default:
			i2cs_r45 = 0;
			i2cs_r46 = 0;
			i2cs_r5d = 0;
		}

		slot = get_slot_id();
		group = 0x68 + slot;  /*Venti ??*/
		device = SCB_SLAVE_CPLD_ADDR;

		offset = I2CS_SW_LED_CONTROL_REG_5E;
		i2cs_r5e = EN_LED_SW_CONTROL;

		/* Take software control of LEDs */
		tdata[0] = ccb_i2cs_set_odd_parity(offset & 0xFF);
		tdata[1] = ccb_i2cs_set_odd_parity(i2cs_r5e);
		if ((ret = ccb_iic_write (group, device, 2, tdata))) {
			printf ("I2C write to EEROM device 0x%x failed.\n", device);
			goto ret_err;
		}
		 
		/* Prog Green LED Reg */
		offset = I2CS_GREEN_LED_REG_45;
		tdata[0] = ccb_i2cs_set_odd_parity(offset & 0xff);
		tdata[1] = ccb_i2cs_set_odd_parity(i2cs_r45);
		if ((ret = ccb_iic_write (group, device, 2, tdata))) {
			printf ("I2C write to EEROM device 0x%x failed.\n", device);
			goto ret_err;
		}

		/* Prog Yellow LED Reg */
		offset = I2CS_YELLOW_LED_REG_46;
		tdata[0] = ccb_i2cs_set_odd_parity(offset & 0xff);
		tdata[1] = ccb_i2cs_set_odd_parity(i2cs_r46);
		if ((ret = ccb_iic_write (group, device, 2, tdata))) {
			printf ("I2C write to EEROM device 0x%x failed.\n", device);
			goto ret_err;
		}

		/* Prog Blink LED Reg */
		offset = I2CS_LED_BLINK_REG_5D;
		tdata[0] = ccb_i2cs_set_odd_parity(offset & 0xff);
		tdata[1] = ccb_i2cs_set_odd_parity(i2cs_r5d);
		if ((ret = ccb_iic_write (group, device, 2, tdata))) {
			printf ("I2C write to EEROM device 0x%x failed.\n", device);
			goto ret_err;
		}
	} else if(EX82XX_LCPU) {
		ctrl_cpld_led = (CFG_CTRL_CPLD_BASE + STATUS_LED_CONTROL_REG);
		tmp = 0x0;
		switch (led_state_t) {
		case LED_GREEN_ON:
			tmp = (STS_LED_GREEN_ON);
			break;
		case LED_YELLOW_ON:
			tmp = (STS_LED_YELLOW_ON);
			break;
		case LED_GREEN_BLINK:
			tmp = (STS_LED_GREEN_BLINK | STS_LED_GREEN_ON);
			break;
		case LED_YELLOW_BLINK:
			tmp = (STS_LED_YELLOW_BLINK | STS_LED_YELLOW_ON);
			break;
		}
		*ctrl_cpld_led = tmp;
	}
	return;
ret_err:
	printf("LED update error\n");
	return;
}

void set_led_state(uint8_t led_state)
{
	global_status_led_st = led_state;	
}

void set_led_default(void)
{
	/* we do a single byte i2c write here. so tdata never needs 
	more than two bytes(offset + data) */
	uint8_t group, device, offset, tdata[2];
	uint8_t i2cs_r07;
	int ret;

	if (!EX82XX_RECPU) {
		return;
	}

	group = my_i2c_groupid();
	device = SCB_SLAVE_CPLD_ADDR;
	offset = I2CS_LED_DEFAULT_REG_07;

	/* set LEDs to default power ON state */
	i2cs_r07 = RESET_LED_STATE;
	tdata[0] = ccb_i2cs_set_odd_parity(offset & 0xFF);
	tdata[1] = ccb_i2cs_set_odd_parity(i2cs_r07);
	if ((ret = ccb_iic_write (group, device, 2, tdata)) != 0) {
		printf ("I2C write to I2CS CPLD 0x%x failed.\n", device);
		return;
	}

	/* clear the LED default state reg so that software can 
	 control the LEDs */
	i2cs_r07 = 0x00;
	tdata[0] = ccb_i2cs_set_odd_parity(offset & 0xFF);
	tdata[1] = ccb_i2cs_set_odd_parity(i2cs_r07);
	if ((ret = ccb_iic_write (group, device, 2, tdata)) != 0) {
		printf ("I2C write to I2CS CPLD 0x%x failed.\n", device);
		return;
	}
}
