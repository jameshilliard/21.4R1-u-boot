/*
 * $Id: cmd_i2c.c,v 1.3 2009-03-06 22:32:12 vishalg Exp $
 * 
 * cmd_i2c.c - generic i2c read/write cmd support for NG-SSG Octeon boards
 * 
 * Nitin Srivastava
 * 
 * Copyright (c) 2008-2011, Juniper Networks, Inc.
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
#include <i2c.h>
#include <configs/jsrxnle.h>

typedef struct srxsme_i2c_sw_map_ {
    u_int8_t twsi_bus;
    u_int8_t address;
    u_int8_t channel;
} srxsme_i2c_sw_map_t;

extern int i2c_switch_programming(uint8_t i2c_switch_addr, uint8_t value);

static const srxsme_i2c_sw_map_t srxsme_mux_settings[] = {
              { 0x0, 0x00, 0x0  }, /* group 0 */
              { 0x1, 0x71, 0x1  }, /* group 1 */
              { 0x1, 0x71, 0x2  }, /* group 2 */
              { 0x1, 0x71, 0x4  }, /* group 3 */
              { 0x1, 0x71, 0x8  }, /* group 4 */
              { 0x1, 0x71, 0x10 }, /* group 5 */
              { 0x1, 0x71, 0x20 }, /* group 6 */
              { 0x1, 0x71, 0x40 }, /* group 7 */
              { 0x1, 0x71, 0x80 }, /* group 8 */
              { 0x1, 0x72, 0x1  }, /* group 9 */
              { 0x1, 0x72, 0x2  }, /* group 10 */
              { 0x1, 0x72, 0x4  }, /* group 11 */
              { 0x1, 0x72, 0x8  }, /* group 12 */
              { 0x1, 0x72, 0x10 }, /* group 13 */
              { 0x1, 0x72, 0x20 }, /* group 14 */
              { 0x1, 0x72, 0x40 }, /* group 15 */
              { 0x1, 0x72, 0x80 }, /* group 16 */
              { 0x1, 0x73, 0x1  }, /* group 17 */
              { 0x1, 0x73, 0x2  }, /* group 18 */
              { 0x1, 0x73, 0x4  }, /* group 19 */
              { 0x1, 0x74, 0x1  }, /* group 20 */
              { 0x1, 0x74, 0x2  }, /* group 21 */
              { 0x1, 0x74, 0x4  }, /* group 22 */
              { 0x1, 0x74, 0x8  }, /* group 23 */
              { 0x1, 0x74, 0x10 }, /* group 24 */
              { 0x1, 0x74, 0x20 }, /* group 25 */
              { 0x1, 0x74, 0x40 }, /* group 26 */
              { 0x1, 0x74, 0x80 }, /* group 27 */
};

unsigned char
read_i2c (unsigned char dev_addr, unsigned char offset,
          unsigned char *val, unsigned char group)
{
    DECLARE_GLOBAL_DATA_PTR;

    switch (gd->board_desc.board_type) {
    CASE_ALL_SRXLE_MODELS
        if(IS_PLATFORM_SRX550(gd->board_desc.board_type)) {
            cpld_write(SRX550_CPLD_I2C_MUX_SEL_REG, group);
            if (i2c_read8_generic(dev_addr, offset, val, !!group)) {
                printf("ERR: Failed to read the I2C addr 0x%x \n", offset);
                return JSRXNLE_I2C_ERR;
            }
        } else {
            if (0 != group) {
                printf(" invalid group for this platform \n" );
                return JSRXNLE_I2C_ERR;
            }
            if (i2c_read8_generic(dev_addr, offset, val, !!group)) {
                printf("ERR: Failed to read the I2C addr 0x%x \n", offset);
                return JSRXNLE_I2C_ERR;
            }
        }
        break;
    CASE_ALL_SRX650_MODELS
        if (!i2c_switch_programming(srxsme_mux_settings[group].address,
                                    srxsme_mux_settings[group].channel)) { 
            if (i2c_read8_generic(dev_addr, offset, 
                                  val, srxsme_mux_settings[group].twsi_bus)) {
                printf("ERR: Failed to read the I2C addr 0x%x \n", offset);
                return JSRXNLE_I2C_ERR;
            }
        } else {
            printf("i2c switch programming failed\n" );
            return JSRXNLE_I2C_ERR;
        }
        break;

    default: 
        printf( " unknown board type\n" );
        return JSRXNLE_I2C_ERR;
    }
    return JSRXNLE_I2C_SUCCESS;
}

unsigned char
write_i2c (unsigned char dev_addr, unsigned char offset,
           unsigned char val, unsigned char group)
{
    
    DECLARE_GLOBAL_DATA_PTR;

    switch (gd->board_desc.board_type) {
    CASE_ALL_SRXLE_MODELS
        if(IS_PLATFORM_SRX550(gd->board_desc.board_type)) {
            cpld_write(SRX550_CPLD_I2C_MUX_SEL_REG, group);
            if (i2c_write8_generic(dev_addr, offset, val, !!group)) {
                printf("ERR: Failed to write the I2C addr 0x%x \n", offset);
                return JSRXNLE_I2C_ERR;
            }
        } else {
            if (0 != group) {
                printf(" invalid group for this platform \n" );
                return JSRXNLE_I2C_ERR;
            }
            if (i2c_write8_generic(dev_addr, offset, 
                                  val, !!group)) {
                printf("ERR: Failed to write the I2C addr 0x%x \n", offset);
                return JSRXNLE_I2C_ERR;
            }
        }
        break;
    CASE_ALL_SRX650_MODELS
        if (!i2c_switch_programming(srxsme_mux_settings[group].address,
                                    srxsme_mux_settings[group].channel)) { 
            if (i2c_write8_generic(dev_addr, offset, 
                                   val, srxsme_mux_settings[group].twsi_bus)) {
                printf("ERR: Failed to write the I2C addr 0x%x \n", offset);
                return JSRXNLE_I2C_ERR;
            }
        } else {
            printf("i2c switch programming failed\n" );
            return JSRXNLE_I2C_ERR;
        }
        break;

    default: 
        printf( " unknown board type\n" );
        return JSRXNLE_I2C_ERR;
    }
    return JSRXNLE_I2C_SUCCESS;
}

static int
do_i2c (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int     rcode;
    uint8_t val;
    uint8_t group;
    uint8_t dev_addr;
    uint8_t offset;

    switch (argc) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
        printf ("Usage:\n%s\n", cmdtp->usage);
        rcode = 1;
        break;
    case 5:
        if (strcmp(argv[1],"read") == 0) {
            group = (uint8_t)simple_strtoul(argv[2], NULL, 16);
            dev_addr = (uint8_t)simple_strtoul(argv[3], NULL, 16);
            offset = (uint8_t)simple_strtoul(argv[4], NULL, 16);

            rcode = read_i2c(dev_addr, offset, &val, group);
            if (JSRXNLE_I2C_ERR == rcode) {
                printf(" I2c read error\n");
            } else {
                printf(" Dev addr : 0x%x, offset 0x%x, value 0x%x\n",
                       dev_addr, offset, val);
            }
            break;
        } else {
            printf ("Usage:\n%s\n", cmdtp->usage);
            rcode = 1;
            break;
        }

    case 6:
        if (strcmp(argv[1],"write") == 0) {
            group = (uint8_t)simple_strtoul(argv[2], NULL, 16);
            dev_addr = (uint8_t)simple_strtoul(argv[3], NULL, 16);
            offset = (uint8_t)simple_strtoul(argv[4], NULL, 16);
            val = (uint8_t)simple_strtoul(argv[5], NULL, 16);

            rcode = write_i2c(dev_addr, offset, val, group);
            if (JSRXNLE_I2C_ERR == rcode) {
                printf( " I2c write error\n" );
            } else {
                printf(" Dev addr : 0x%x, offset 0x%x, written 0x%x\n",
                       dev_addr, offset, val);
            }
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
	i2c, 10, 1, do_i2c,
	"i2c - read/write on i2c bus\n",
	"i2c read <i2c group> <dev_addr> <offset> \n"
	"    - read the specified offset from dev_addr which belongs "
	"to specified i2c group\n"
	"i2c write <i2c group> <dev_addr> <offset> <val>\n"
	"    - write val to the specified offset from dev_addr which belongs "
	"to specified i2c group\n"
);
