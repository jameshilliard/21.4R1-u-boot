/*
 * Copyright (c) 2009-2011, Juniper Networks, Inc.
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

#include <rmi/types.h>
#include <common/post.h>
#include <common/i2c.h>
#include <cb/cb_post.h>

#include <cb/cb_board.h>

static int slave_dev_map[MAX_I2C_BUSES][MAX_I2C_ADDR] = { 0 };

int
diag_post_i2c_init (void)
{
    uchar ch, bus, dev, chan;

    post_printf(POST_VERB_DBG,"In i2c init\n");

    for (bus = 0; bus < MAX_I2C_BUSES; bus++) {
        /* discover and record first level of devices */
        for (dev = 1; dev < MAX_I2C_ADDR; dev++) {
            if (i2c_probe(bus, dev) == 0) {
                slave_dev_map[bus][dev] = 1;
            }
        }
    }

    return 0;
}


int
diag_post_i2c_run ()
{
    uchar bus, dev, byte = 0x1, i;

    uchar *i2c_dev_list, tot_i2c_devs = 0;

    int ret = 0;

    /* check if discovered device is a board device */
    for (bus = 0; bus < MAX_I2C_BUSES; bus++) {

        if (fx_is_cb()) {
            i2c_dev_list = cb_board_i2c_devs[bus];
            tot_i2c_devs = cb_num_i2c_devices[bus];
        } else {
            post_printf(POST_VERB_DBG,"Board type couldnt be determined" 
                                      "to successfully probe i2c bus %d\n",bus);
            post_test_status_set(POST_I2C_PROBE_ID, POST_UNKNOWN_BOARD_ERROR);
                            
            return -1; /* No point in proceeding */
        } 

        for (dev = 0; dev < tot_i2c_devs; dev++) {
            /*   
             *   1 -- additional devices (not in list)
             *   0 -- matched devices
             *  -1 -- missing devices (not able to detect)
             */
            slave_dev_map[bus][i2c_dev_list[dev]]--;
        }
    }


    for (bus = 0; bus < MAX_I2C_BUSES; bus++) {
        /* dev addr 0 is a generic address to which all i2c devices respond, skip it */
        for (dev = 1; dev < MAX_I2C_ADDR; dev++) {
            if (slave_dev_map[bus][dev] == 1) {
                post_printf(POST_VERB_DBG,"\nAdditional i2c slave dev at addr 0x%x on bus %d", dev, bus);
                post_test_status_set(POST_I2C_PROBE_ID, POST_I2C_PROBE_AD_ERROR);
                ret = -1;
            } else if (slave_dev_map[bus][dev] == -1) {
                post_printf(POST_VERB_DBG,
                            "\nMissing i2c slave dev at addr 0x%x on bus %d",
                            dev, bus);
                post_test_status_set(POST_I2C_PROBE_ID, 
                                     POST_I2C_PROBE_AD_ERROR);
                ret = -1;
            }
        }
    }

    return ret;
}


int
diag_post_i2c_clean ()
{
    return 0;
}


