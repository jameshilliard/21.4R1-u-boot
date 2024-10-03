/*
 * $Id: cmd_gpio.c,v 1.1.144.2 2009-04-16 05:53:08 kdickman Exp $
 * 
 * cmd_gpio.c -  generic gpio read/write/config support for Valhalla boards
 * 
 * Nitin Srivastava
 * 
 * Copyright (c) 2008-2010, Juniper Networks, Inc.
 * All rights reserved.
 */

#include <common.h>
#include <config.h>
#include <watchdog.h>
#include <command.h>
#include <image.h>
#include <asm/byteorder.h>
#include <octeon_boot.h>
#include <octeon_hal.h>
#include <octeon_boot.h>

#define IS_VALID_GPIO_PIN(_pin) \
    (_pin >= 0 && _pin <= 0xF)

static int
do_gpio (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int      rcode;
    int      val;
    uint8_t  pin;
    uint32_t iter;

    switch (argc) {
    case 0:
    case 1:
    case 2:
        printf ("Usage:\n%s\n", cmdtp->usage);
        rcode = 1;
        break;

    case 3:
        if (strcmp(argv[1],"read") == 0) {
            pin = (uint8_t)simple_strtoul(argv[2], NULL, 16);
            if (!IS_VALID_GPIO_PIN(pin)) {
                printf( " Not a valid pin number\n" );
                break;
            }
            val = octeon_gpio_value(pin);
            printf(" gpio pin 0x%x val %d\n",
                   pin, val);
        } else if (strcmp(argv[1],"set") == 0) {
            pin = (uint8_t)simple_strtoul(argv[2], NULL, 16);
            if (!IS_VALID_GPIO_PIN(pin)) {
                printf(" Not a valid pin number\n");
                break;
            }
            octeon_gpio_set(pin);
            printf(" gpio pin 0x%x set \n",
                    pin);
        } else if (strcmp(argv[1],"clear") == 0) {
            pin = (uint8_t)simple_strtoul(argv[2], NULL, 16);
            if (!IS_VALID_GPIO_PIN(pin)) { 
                printf(" Not a valid pin number\n");
                break;
            }
            octeon_gpio_clr(pin);
            printf(" gpio pin 0x%x cleared\n",
                   pin);
        } else {
            printf ("Usage:\n%s\n", cmdtp->usage);
            rcode = 1;
        }
        break;

    case 4:
        if (strcmp(argv[1],"config") == 0) {
            pin = (uint8_t)simple_strtoul(argv[2], NULL, 16);
            if (!IS_VALID_GPIO_PIN(pin)) {
                printf(" Not a valid pin number\n");
                break;
            }
            if (strcmp(argv[3],"i") == 0) {
                octeon_gpio_cfg_input(pin);
                printf(" gpio pin 0x%x configured as input pin\n", pin);
            } else if (strcmp(argv[3],"o" ) == 0) {
                octeon_gpio_cfg_output(pin);
                printf(" gpio pin 0x%x configured as output pin\n", pin);
            } else {
                printf( " wrong config param\n" );
            }
        } else {
            printf ("Usage:\n%s\n", cmdtp->usage);
            rcode = 1;
        }
        break;
    default:
        printf ("Usage:\n%s\n", cmdtp->usage);
        rcode = 1;
        break;
    }
    return rcode;
}

U_BOOT_CMD(
	gpio, 10, 1, do_gpio,
	"gpio - read/write on gpio pins\n",
	"gpio config <pin_no> <i/o>\n"
	"     - set the specified gpio pin as input/output pin\n"
	"gpio read <pin no>\n"
	"     - read the gpio pin.\n"
	"gpio set <pin no>\n"
	"     - set the gpio pin.\n"
	"gpio clear <pin no>\n"
	"     - clear the gpio pin.\n"
);
