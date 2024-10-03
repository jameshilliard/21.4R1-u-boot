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

#include "rps.h"
#include <exports.h>
#include "rps_command.h"
#include "rps_cpld.h"
#include "rps_led.h"

#define RPS_LED_SHIFT   0
#define RPS_LED_MASK 0xf0
#define FANFAIL_LED_SHIFT   4
#define FANFAIL_LED_MASK 0x0f

#define PORT_LED_BITS 4

int32_t 
set_rps_led (LedColor color)
{
    uint8_t temp;
    uint8_t tempc = (uint8_t)color << RPS_LED_SHIFT;

    if (cpld_reg_read(RPS_FANFAIL_LED, &temp))
        return (-1);

    temp = (temp & RPS_LED_MASK) | tempc;

    if (cpld_reg_write(RPS_FANFAIL_LED, temp))
        return (-1);
    
    return (0);
}

int32_t 
set_fanfail_led (LedColor color)
{
    uint8_t temp;
    uint8_t tempc = (uint8_t)color << FANFAIL_LED_SHIFT;

    if (cpld_reg_read(RPS_FANFAIL_LED, &temp))
        return (-1);

    temp = (temp & FANFAIL_LED_MASK) | tempc;

    if (cpld_reg_write(RPS_FANFAIL_LED, temp))
        return (-1);
    
    return (0);
}

int32_t 
set_port_led (int32_t port, LedColor color)
{
    uint8_t offset = cpld_get_reg_offset(PORT_LED_BITS, port);
    uint8_t tempc = (uint8_t)color << cpld_get_reg_shift(PORT_LED_BITS, port);
    uint8_t mask = ~(cpld_get_reg_mask(PORT_LED_BITS, port));
    uint8_t temp;

    if (port >= CPLD_MAX_PORT)
        return (-1);

    if (cpld_reg_read(CPLD_FIRST_PORT_LED_REG+offset, &temp))
        return (-1);

    temp = (temp & mask) | tempc;

    if (cpld_reg_write(CPLD_FIRST_PORT_LED_REG+offset, temp))
        return (-1);
    
    return (0);
}

static char *decode_led_state[9] =
{
    "OFF",
    "AMBER BLINK",
    "AMBER",
    " ",
    "GREEN BLINK",
    " ",
    " ",
    " ",
    "GREEN"
};

LedColor 
get_rps_led (void)
{
    uint8_t temp;
    
    cpld_reg_read(RPS_FANFAIL_LED, &temp);
    return (LedColor) (temp & 0xf);
}

char * 
get_rps_led_state (void)
{
    return (decode_led_state[get_rps_led()]);
}

LedColor 
get_fanfail_led (void)
{
    uint8_t temp;
    
    cpld_reg_read(RPS_FANFAIL_LED, &temp);
    return (LedColor) ((temp & 0xf0) >> 4);
}

char * 
get_fanfail_led_state (void)
{
    return (decode_led_state[get_fanfail_led()]);
}

LedColor 
get_port_led (int32_t port)
{
    uint8_t temp;
    
    cpld_reg_read(CPLD_FIRST_PORT_LED_REG + (port/2), &temp);
    if (port % 2)
        return (LedColor) ((temp & 0xf0) >> 4);
    else
        return (LedColor) (temp & 0xf);
}

char * 
get_port_led_state (int32_t port)
{
    return (decode_led_state[get_port_led(port)]);
}

int 
do_led (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    LedColor color = LED_OFF;
    char cmd1, cmd2, cmd3;
    int port;

    if (argc <= 2)
        goto usage;
    
    cmd1 = argv[1][0];
    cmd2 = argv[2][0];
    
    switch (cmd1) {
        case 'r':       /* rps */
        case 'R':
        case 'f':       /* fan fail */
        case 'F':
            if ((argc == 4) && ((argv[3][0] == 'b') || (argv[3][0] == 'B'))) {
                if ((cmd2 == 'o') || (cmd2 == 'O')) {
                    color = LED_OFF;
                }
                else if ((cmd2 == 'g') || (cmd2 == 'G')) {
                    color = LED_GREEN_BLINKING;
                }
                else if ((cmd2 == 'a') || (cmd2 == 'A')) {
                    color = LED_AMBER_BLINKING;
                }
                else
                    goto usage;
            }
            else {
                if ((cmd2 == 'o') || (cmd2 == 'O')) {
                    color = LED_OFF;
                }
                else if ((cmd2 == 'g') || (cmd2 == 'G')) {
                    color = LED_GREEN;
                }
                else if ((cmd2 == 'a') || (cmd2 == 'A')) {
                    color = LED_AMBER;
                }
                else
                    goto usage;
            }
            if ((cmd1 == 'r') || (cmd1 == 'R'))
                set_rps_led(color);
            else
                set_fanfail_led(color);
                    
            break;
            
        case 'p':       /* port */
        case 'P':
            if (argc <= 3)
                goto usage;
            
            cmd3 = argv[3][0];
            port = simple_strtoul(argv[2], NULL, 16);
            if ((argc == 5) && ((argv[4][0] == 'b') || (argv[4][0] == 'B'))) {
                if ((cmd3 == 'o') || (cmd3 == 'O')) {
                    color = LED_OFF;
                }
                else if ((cmd3 == 'g') || (cmd3 == 'G')) {
                    color = LED_GREEN_BLINKING;
                }
                else if ((cmd3 == 'a') || (cmd3 == 'A')) {
                    color = LED_AMBER_BLINKING;
                }
                else
                    goto usage;
            }
            else {
                if ((cmd3 == 'o') || (cmd3 == 'O')) {
                    color = LED_OFF;
                }
                else if ((cmd3 == 'g') || (cmd3 == 'G')) {
                    color = LED_GREEN;
                }
                else if ((cmd3 == 'a') || (cmd3 == 'A')) {
                    color = LED_AMBER;
                }
                else
                    goto usage;
            }
            set_port_led(port, color);
                    
            break;
            
        default:
            printf("???");
            break;
    }

    return (1);
 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return (1);
}

U_BOOT_CMD(
    led,   5,  1,  do_led,
    "led     - led test commands\n",
    "\n"
    "led [rps|fan] [off|green|amber] [blinking]\n"
    "    - rps/fan fail led\n"
    "led port <number> [off|green|amber] [blinking]\n"
    "    - port 0-5 led\n"
);

