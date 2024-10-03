
/*
 * $Id$
 *
 * i2c_cdev.c -- i2c driver specific  for the Juniper ACX Product Family
 *
 * Mathew K A, Jan 2012
 *
 * Copyright (c) 2012-2014, Juniper Networks, Inc.
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
#include <i2c.h>
#include "i2c_dev_acx.h"

DECLARE_GLOBAL_DATA_PTR;

#define DS1721_CMD_READ 0xAA
#define DS1271_RESPONSE_LEN 2
#define MAX_6581_ENABLE_EXTENDED 0x02	/* Enable bit for extended range */
#define MAX_6581_CONFIG_REG_ADDR 0x41	/* MAX 6851 physical address	 */
#define MAX_6581_EXTD_TEMP_MIN -64	/* Minimum supported vale -64 C  */
#define DELAY_IN_SECS 420		/* Loop time during low temp	 */
#define TEMP_SENSOR_NUM 6		/* Index for MAX6851 sensor	 */
#define TEMP_SENSOR_CHANL_START 1	/* Starting channel number	 */
#define TEMP_SENSOR_CHANL_MAX 9		/* Ending channel number	 */ 

#define ACX1000_TEMP_SENSE_CHAN_NEAR_FLASH 4
#define ACX2000_TEMP_SENSE_CHAN_NEAR_FLASH 6 
#define ACX4000_TEMP_SENSE_CHAN_NEAR_FLASH 3 
#define ACX500_TEMP_SENSE_CHAN_NEAR_FLASH  5

#define TEMP_SENSOR_LOW_VALUE -26	/* Low temperature to check below */

extern void syspld_swizzle_enable(void);
extern void syspld_swizzle_disable(void);


static i2c_device_t *
i2c_get_device_details (unsigned long sc_addr)
{
    unsigned int i;
    i2c_device_t *i2c_device_list = i2c_acx_device_list;

    for (i = 0; i2c_device_list[i].logical_addr != I2C_DEV_INVALID; i++) {
	if (i2c_device_list[i].logical_addr == sc_addr)
	    return &i2c_device_list[i];
    }
    return NULL;
}

/*
 * This function will open up muxes and read the i2c device
 * to read the temperature value
 */
int
temp_sens_read (unsigned char sensor_addr, unsigned int channel, 
		signed char *temp)
{
    u_char multiplexer_control_reg;
    int error = 0;
    int i = 0;
    unsigned char data = 0;
    i2c_device_t *i2c_dev;
    unsigned char buf[DS1271_RESPONSE_LEN];

    /*
     * get physical address and other details of the device
     * from logical address
     */
    i2c_dev = i2c_get_device_details(sensor_addr);
    if (!i2c_dev) {
	printf("Failed to get dev list \n");
	return -1;
    }
    if (i2c_dev->num_multiplexer > I2C_MULTIPLEXER_MAX_DEPTH) {
	printf("Invalid no of multiplexers\n");
	return -1;
        
    }
    /*
     * Take care of multiple muxes that may be present.
     */
    if (i2c_dev->has_multiplexer & MULTIPLEXER_PRESENT) {
	for (i = 0; i < i2c_dev->num_multiplexer; i++) {
	    /* Do a read, so that we don't disturb other bits */
	    error = i2c_read(i2c_dev->multiplexer_addr[i], 0, 1,
			     &multiplexer_control_reg, 1);
	    if (i2c_dev->channel_index[i] != -1) {
		multiplexer_control_reg |= (1 << i2c_dev->channel_index[i]);
	    }
	    else {
		multiplexer_control_reg = 0;
	    }
	    error = i2c_write(i2c_dev->multiplexer_addr[i], 0, 1,
			      &multiplexer_control_reg, 1);
	    if (error) {
		printf("Failed to open multiplexer \n");
		return  error;
	    }
	}
    }


    /*
     * Enable extended range for the temperature sensors
     * on MAX6581
     *
     */
    if (i2c_dev->logical_addr == ACX_RE_MAX6581_1) {
        error = i2c_read(i2c_dev->physical_addr, MAX_6581_CONFIG_REG_ADDR, 1,
			 &data, 1);
        if (error) {
            printf("Failed to read MAX6581 configuration register\n");
            return  error;
        }

        data = data & MAX_6581_ENABLE_EXTENDED;

        /* If not in extended temperature mode already */
        if (!data) {
            data = data | MAX_6581_ENABLE_EXTENDED;
            error = i2c_write(i2c_dev->physical_addr, MAX_6581_CONFIG_REG_ADDR,
                              1, &data, 1);
            if (error) {
                printf("\nEnabling MAX6581 extended range temperature failed!");
                return  error;
            }
            else {
                printf("\nEnabled extended range temperature for MAX6581");
            }

            /*
             * Wait for 1.5 seconds for propogation delay 
             * as provided by ACCTON (MAXIM provided data)
             *
             */
            udelay(1500000);

        }
    }

    /*
     * Do the read.
     * For DS1271, send command to read the temperature and
     * ignore the fraction part.
     *
     */
    if ((i2c_dev->logical_addr == ACX_RE_PSU0_DS1271) ||
        (i2c_dev->logical_addr == ACX_RE_PSU1_DS1271)) {
        error = i2c_read(i2c_dev->physical_addr, DS1721_CMD_READ, 1, buf,
                         DS1271_RESPONSE_LEN);
        data = buf[0];
    }
    else {
        error = i2c_read(i2c_dev->physical_addr, channel, 1, &data, 1);
    }
    if (error) {
        printf("Failed to read\n");
        return  error;
    }

    multiplexer_control_reg = 0;
    i--;
    /*
     * Traverse the multiplexer tree reversly and disable
     * each channel
     */

    for (; i >= 0; i--) {
        error = i2c_read(i2c_dev->multiplexer_addr[i], 0, 1,
                         &multiplexer_control_reg, 1);
        multiplexer_control_reg &= ~(1 << i2c_dev->channel_index[i]);
        error = i2c_write(i2c_dev->multiplexer_addr[i],
                          0, 1, &multiplexer_control_reg, 1);
        if (error) {
            printf("Failed to write :%d\n", i);
            return  error;
        }
    }

    /*
     * For MAX6581 device, convert to extended range
     * of -64 Degree C to +191 Degree C
     *
     * For DS1721, check sign bit and return negative
     * values
     *
     */
    if (i2c_dev->logical_addr == ACX_RE_MAX6581_1) {
        *temp = (data + MAX_6581_EXTD_TEMP_MIN);
    }
    else if ((i2c_dev->logical_addr == ACX_RE_PSU0_DS1271) ||
             (i2c_dev->logical_addr == ACX_RE_PSU1_DS1271)) {
             if (data & 0x80) {
                 *temp = (data - 0xFF);
             }
        else {
                *temp = data;
             }
    }
    else {
        *temp = data;
    }
    return 0;
}

void 
check_temperature_and_wait (void)
{
    signed char temp_sense_val = 0;
    unsigned int temp_chn;
    unsigned int i;


    /*
     * Read temperature from sensor near to NAND FLASH
     * and if it is in low temperature condition,
     * need to give enough time for NAND Flash chip
     * to warm up.
     *
     */
    switch (gd->board_type) {
        case I2C_ID_ACX500_O_POE_DC:
        case I2C_ID_ACX500_O_POE_AC:
        case I2C_ID_ACX500_O_DC:
        case I2C_ID_ACX500_O_AC:
        case I2C_ID_ACX500_I_DC:
        case I2C_ID_ACX500_I_AC:
            temp_chn = ACX500_TEMP_SENSE_CHAN_NEAR_FLASH;
            break;
        case I2C_ID_ACX1000:
        case I2C_ID_ACX1100:
            temp_chn = ACX1000_TEMP_SENSE_CHAN_NEAR_FLASH;
            break;
        case I2C_ID_ACX2000:
        case I2C_ID_ACX2100:
        case I2C_ID_ACX2200:
            temp_chn = ACX2000_TEMP_SENSE_CHAN_NEAR_FLASH;
            break;
        case I2C_ID_ACX4000:
            temp_chn = ACX4000_TEMP_SENSE_CHAN_NEAR_FLASH;
            break;
        default:
            /* If any other ACX board types */
            return;
    }

    /*
     * Loop for delay in multiples of 20 seconds to check for low
     * temperature condition, if low temperature wait for 20 seconds
     * before reading temperature again, any time temperature is above
     * low value, exit the loop. Also reset the swizzle counter each
     * time to give us enough time before next flash swizzle happens.
     *
     */
    for (i = 0; i < (DELAY_IN_SECS/20); i++) {

        /* Do the read */
        /* If error reading sensor, then exit */
        if ((temp_sens_read(TEMP_SENSOR_NUM, temp_chn, &temp_sense_val)) 
            == -1) {
            puts("Error in reading temperature");
            break;
        }

        printf("\nSensor: %d Channel: %d Temperature is %d degrees C\n",
                TEMP_SENSOR_NUM, temp_chn, temp_sense_val);

        /*
         * If under low temperature condition, wait for specified time as found
         * in factory testing in multiples of loop above seconds.
         *
         */
        if (temp_sense_val < TEMP_SENSOR_LOW_VALUE) {
            puts ("Low temperature condition detected, waiting...\n");

            /*
             * Re-load the swizzle counter.
             *
             */
            syspld_swizzle_disable();
            syspld_swizzle_enable();

            /*
             * Wait for 20 seconds
             *
             */
            udelay(20 * 1000000);

            printf("Total waited time - %d seconds\n",(i+1) * 20);
        }
        else {
            /* Exit, not under low temperature condition */
            break;
        }
    }
}
