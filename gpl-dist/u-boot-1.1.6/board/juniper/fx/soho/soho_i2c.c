/*
 * Copyright (c) 2010-2012, Juniper Networks, Inc.
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

/* Author : Bilahari Akkriaju */
/* SOHO board specific i2c */

#include <command.h>
#include <common.h>
#include <common/post.h>
#include "soho_board.h"
#include "soho_cpld.h"
#include "tor_cpld_i2c.h"

#include <common/fx_gpio.h>
#include <common/i2c.h>

void
probe_i2c (uint32_t mux, unsigned char chan)
{
    unsigned char addr;
    for (addr = 1; addr <= I2C_MAX_ADDR; addr++) {
        if (i2c_probe(mux, addr) == 0) {
            printf("Successfully probed device address 0x%x on channel %d\n",
                   addr, chan);
        }
    }
}

void 
wa_i2c_probe (uint32_t mux, unsigned char chan, uint32_t gpio_data)
{
    unsigned char chan1;
    for (chan1 = 0; chan1 < WA_MAX_CHANNEL; chan1++) {
        gpio_write_data_set((gpio_data & CHAN_MASK)  | 
                                        chan         | 
                                        (chan1 << 3) | 
                                        (1 << 24));
        probe_i2c(mux,chan);
    }
}

void 
soho_i2c_probe (uint32_t mux, unsigned char chan)
{
    probe_i2c(mux,chan);
}

void
probe_i2c_tree (void)
{
    unsigned char chan, chan1, j, addr;
    uint32_t gpio_cfg = GPIO_CFG_IO_VAL;
    uint32_t gpio_data  = GPIO_WRITE_DATA_VAL;
    uint32_t max_channel, channel_mask; 
    volatile uint32_t mux;

    gpio_cfg_io_set((gpio_cfg | 0x3F) | (1 << 24));
    mux = I2C_BUS0;
    /* Select one channel at a  time */
    for (chan = 0; chan < MAX_CHANNEL; chan++) {
        /* 
         * channel 6 has sec rom device , to probe it
         * you first have to bring it out of reset 
         * by writing all 1's(0xff) followed by all 0's(0x00) 
         * to cpld thats managing resets
         */
        if (chan == 6) {
            printf("Bringing out security ROM out of reset\n");
            soho_cpld_write(MCPLD_RST_REG_OFFSET, 0xff);
            udelay(SOHO_DELAY);
            soho_cpld_write(MCPLD_RST_REG_OFFSET, 0x00);
            udelay(SOHO_DELAY);
        }
        if (fx_has_i2c_cpld()) {
            mux = TOR_CPLD_I2C_CREATE_MUX(I2C_BUS0, 0, chan);
        } else {
            gpio_write_data_set((gpio_data & CHAN_MASK) | chan | (1 << 24));
        }
        if ((chan == 0x03) && (fx_is_wa()) && (wa_is_rev3_or_above())) {
            wa_i2c_probe(mux, chan, gpio_data);
        } else {
            soho_i2c_probe(mux, chan);
        }
    }

    printf("\n\n");

    if (fx_is_wa()){
           fx_cpld_qsfp_enable();
           max_channel = WA_MAX_CHANNEL;
           channel_mask = WA_CHAN_MASK;
    } else {
           max_channel = MAX_CHANNEL;
           channel_mask = CHAN_MASK;
    }
    mux = I2C_BUS1;
    /* Select one channel at a  time */
    for (chan = 0; chan < max_channel; chan++) {
        for (chan1 = 0; chan1 < max_channel; chan1++) {
            if (fx_has_i2c_cpld()) {
                mux = TOR_CPLD_I2C_CREATE_MUX(I2C_BUS1, chan, chan1);
            } else {
                gpio_write_data_set((gpio_data & channel_mask) | 
                                    (chan1 << 3)            | 
                                    (chan)                  | 
                                    (1 << 24));
            }
            for (addr = 1; addr <= I2C_MAX_ADDR; addr++) {
                if (i2c_probe(mux, addr) == 0) {
                    printf("Successfully probed device address 0x%x on (%d,%d) \n",
                           addr, chan, chan1);
                }
            }
            printf("\n");
        }
        printf("\n");
    }
}


/* Scan the whole i2c bus */
int
do_gpio_i2c_tree_scan (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    probe_i2c_tree();
}


/* Probe a particular 7-bit i2c address  */
int
do_gpio_i2c_probe (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    unsigned char bus  = 0;
    unsigned char addr = 0;

    if (argc == 3) {
        bus  = (unsigned char)simple_strtoul(argv[1], NULL, 16);
        addr = (unsigned char)simple_strtoul(argv[2], NULL, 16);
    } else {
        printf("Defaulting to bus 0, dev_addr 0\n");
    }

    if (i2c_probe(bus, addr) != 0) {
        printf("Ack error on bus %d addr 0x%x\n", bus, addr);
        return -1;
    } else {
        printf("Ack from addr 0x%x on bus %d \n", addr, bus);
    }

    return 0;
}


/* Select a particular channel on the i2c bus over GPIO */
int
do_gpio_i2c_chan_sel (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    unsigned char bus = 0, chan = 0, chan1 = 0;
    uint32_t gpio_cfg = GPIO_CFG_IO_VAL;
    uint32_t gpio_data  = GPIO_WRITE_DATA_VAL;

    gpio_cfg_io_set((gpio_cfg | 0x3F) | (1 << 24));

    printf("cfg :: 0x%x   io :: 0x%x\n", gpio_cfg, gpio_data);


    bus = simple_strtoul(argv[1], NULL, 16);

    if (bus == 1) {
        if (argc != 4) {
            return -1;
        }
        chan = simple_strtoul(argv[2], NULL, 16);
        chan1 =  simple_strtoul(argv[3], NULL, 16);
        gpio_write_data_set(((gpio_data & CHAN_MASK)   |
                            (chan1 << 3)               | 
                            chan                       | 
                            (1 << 24)));
        printf("Selected gpio channel 0x%x\n", GPIO_WRITE_DATA_VAL);
    } else {
        chan = simple_strtoul(argv[2], NULL, 16);
        if ((chan == 0x03) && (fx_is_wa()) && (wa_is_rev3_or_above())) {
            chan1 =  simple_strtoul(argv[3], NULL, 16);
            gpio_write_data_set(((gpio_data & WA_CHAN_MASK)    |
                                       (chan1 << 3)            | 
                                       chan                    | 
                                       (1 << 24)));

        } else {
            gpio_write_data_set(((gpio_data & CHAN_MASK)    | 
                                  chan                      | 
                                  (1 << 24)));
        }
        printf("Selected gpio channel 0x%x\n", GPIO_WRITE_DATA_VAL);
    }
    return 0;
}

U_BOOT_CMD(
        itree,
        1,
        1,
        do_gpio_i2c_tree_scan,
        "itree \n",
        "\n"
        );

U_BOOT_CMD(
        isel,
        4,
        1,
        do_gpio_i2c_chan_sel,
        "isel   - select an i2c channel\n",
        "bus# chan#  \n"
        );

U_BOOT_CMD(
        ipb,
        3,
        1,
        do_gpio_i2c_probe,
        "ipb    - probe a particular 7-bit i2c addr\n",
        "[<7-bit i2c addr>]\n"
        );


