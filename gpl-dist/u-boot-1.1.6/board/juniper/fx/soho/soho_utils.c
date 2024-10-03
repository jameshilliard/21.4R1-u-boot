/*
 * Copyright (c) 2010-2011, Juniper Networks, Inc.
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


/******************************************************************
 * Desc : SOHO specific u-boot utilities
 * Author: Bilahari Akkriaju
 *******************************************************************/

#include <common.h>
#include <command.h>
#include <asm/byteorder.h>
#include <common/post.h>

#include <common/fx_gpio.h>
#include <common/i2c.h>
#include <common/fx_common.h>
#include "soho_cpld_map.h"
#include "soho_board.h"
#include "tor_cpld_i2c.h"

extern uint32_t tor_cpld_i2c_debug_flag; 
#ifdef  BW_DEBUG
#define bw_debug(fmt,args...)   printf (fmt ,##args)
#else
#define bw_debug(fmt,args...)
#endif  /* DEBUG */

#define bw_printf(fmt, args...)                          \
    do {                                                \
        if (getenv("bw_verbose")) {                   \
            printf(fmt ,##args);                        \
        }                                               \
    } while (0)

static char *soho_max6697_sensor[SOHO_MAX6697_SENSORS] = 
{   "CPU",
    "TL0",
    "TL1",
    "TQ"
};

static char *soho_tmp435_sensor_name[] = 
{   
    "Left  Exhaust",
    "Right Exhaust",
    "Left  Intake",
    "Right Intake"
};

static char *soho_tmp435_sensor_pos[] = 
{   
    "top",
    "bottom"
};

static uint8_t soho_tmp435_sensor_addr[] = 
{   
    SOHO_TEMP_SENSOR1,
    SOHO_TEMP_SENSOR2,
    SOHO_TEMP_SENSOR3,
    SOHO_TEMP_SENSOR4
};

static char *pwr1220_sensor_name[] = 
{   
    "DDR3           1.5V",
    "CPU            1.2V",
    "DDR2           1.8V",
    "DDR2           3.3V",
    "DDR2           2.5V",
    "Misc           1.5V",
    "DDR2           3.3VB",
    "LATVOL_0       -.-V",
    "PS1 AC OK      -.-V",
    "PS0 AC OK      -.-V",
    "LTC2978 fault  -.-V",
    "              12.0V"
};

/* margin value */
static uint8_t pwr1220_margin[SOHO_PWR1220_MARGIN_DEVICES][SOHO_MARGIN_LEVELS] =
{
    {0x80, 0xD0, 0x50},
    {0x80, 0xD0, 0x50},
    {0x80, 0xD0, 0x50},
    {0x80, 0xD0, 0x50},
    {0x80, 0xD0, 0x50},
    {0x80, 0xD0, 0x50},
    {0x80, 0xD0, 0x50},
    {0x80, 0xD0, 0x50},
};

static char *ltc2978_sensor_name[] = 
{   
    "TL0  0.9V",
    "TL0  1.0V",
    "TL1  0.9V",
    "TL1  1.0V",
    "TQ   0.9V",
    "TQ   1.0V",
    "DDR3 1.5V",
    "CPU  1.2V"
};

/* margin command */
static uint8_t ltc2978_margin[SOHO_MARGIN_LEVELS] =
{ 0x80, 0x98, 0xA8};

static int soho_tmp435_extended_range[4][2] = { 0 };

#define I2C_MAX_ACCESS_PER_WORD     5          
#define I2C_BITS_PER_BYTE           7
#define I2C_DATA_MASK               0x7F
#define I2C_PARITY_MASK             0x80

#define I2C_ADDR_PIO_ADDR0          0
#define I2C_ADDR_PIO_ADDR1          1
#define I2C_ADDR_PIO_ADDR2          2
#define I2C_ADDR_PIO_ADDR3          3
#define I2C_ADDR_PIO_ADDR4          4
#define I2C_ADDR_PIO_CTRL           5
#define I2C_ADDR_PIO_DATA0          6
#define I2C_ADDR_PIO_DATA1          7
#define I2C_ADDR_PIO_DATA2          8
#define I2C_ADDR_PIO_DATA3          9
#define I2C_ADDR_PIO_DATA4          10

#define I2C_ADDR_TOTAL              (I2C_ADDR_PIO_DATA4 + 1)

#define I2C_ADDR_PIO_CTRL_READ      0x1
#define I2C_ADDR_PIO_CTRL_WRITE     0x0
#define I2C_ADDR_PIO_CTRL_PRI       0x0
#define I2C_ADDR_PIO_CTRL_SEC       0x2

#define I2C_CACHE_VALID_MASK        0x8000
#define I2C_CACHE_DATA_MASK         I2C_DATA_MASK

#define EOK                         0
#define EFAIL                       -1

/* For delay between i2c accesses */
/* Considering both i2c & slower SMB devices
 * hooked up on the same i2 controller.
 */
int 
atoh (char* string)
{
    int res = 0;

    while (*string != 0)
    {
        res *= 16;
        if (*string >= '0' && *string <= '9')
            res += *string - '0';
        else if (*string >= 'A' && *string <= 'F')
            res += *string - 'A' + 10;
        else if (*string >= 'a' && *string <= 'f')
            res += *string - 'a' + 10;
        else
            break;
        string++;
    }

    return (res);
}


static void
delay_loop (void)
{
    udelay(2200);
}


/* Swap MSB & LSB */
static void
swap (char *buff)
{
    char temp = buff[0];

    buff[0]   = buff[1];
    buff[1]   = temp;
}


/* Temperature conversion as per PSMI data sheet */

void
print_temp (unsigned short val)
{
    unsigned short exp, mntsa, sign, tmp;

    sign  = (val & TEMP_MSB_MASK) >> 15; /* msb */

    exp   = (val & TEMP_EXP_MASK);       /* the last 6 bits */

    mntsa = (val >> 6) & TEMP_MNTSA_MASK;   /* 9-bits of temperature */

    tmp = ((~mntsa) & TEMP_MNTSA_MASK);

    /* if msb is set - its a negative number's 2's complement */
    if (sign) {
        tmp = tmp + 1; /*2's complement */
        printf("-");
    } else  {          /* A positive number's 2's complement */
        tmp = (TEMP_9_BIT_RES - tmp);
        printf("+");
    }
    printf("%d degrees C", tmp);
}

void
set_ps (int j, int gpio_data)
{
    switch(j) {
        case 1:
            gpio_write_data_set(((gpio_data & ~(0x3f)) | 0x00) | (1 << 24));
            break;
        case 2:
            gpio_write_data_set(((gpio_data & ~(0x3f)) | 0x01) | (1 << 24));
            break;
        default:
            printf("Only 2 power supplies are present on the board." 
                   "Defaulting to first one\n");
            gpio_write_data_set(((gpio_data & ~(0x3f))| 0x00) | (1 << 24));
    }
}

/* Dump the RPM, Temperature values from the PSMIs on SOHO */

void
do_soho_psmi_i2c_get (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int i, j = 1;
    unsigned char buff[3] = { 0 };
    unsigned short ps = PS_I2C_ADDR, *p = NULL;
    uint32_t gpio_cfg = GPIO_CFG_IO_VAL;
    uint32_t gpio_data  = GPIO_WRITE_DATA_VAL;

    gpio_cfg_io_set((gpio_cfg | 0x3F) | (1 << 24));

    if (argc == 2) {
        j = simple_strtoul(argv[1], NULL, 10);
    } else  {
        printf("Using default arguments. Do 'help srps' to get right usage\n");
    }

    set_ps(j,gpio_data);

    printf("Discovery string : ");
    for (i = 0; i < 2; i++) {
        i2c_read_seq(I2C_BUS0, ps, (PSMI_DIS_STR_OFFSET + i), &buff, 2);
        printf("%c%c", buff[0], buff[1]);
        memset(buff, 0, 3);
        delay_loop();
    }

    printf("\nRPMS from PS%d:\n", j);

    for (i = 0; i < 2; i++) {
        i2c_read_seq(I2C_BUS0, ps, (PSMI_RPM_REG_OFFSET + i), &buff, 2);
        swap(buff);
        p = (unsigned short *)buff;
        printf("Fan %d speed : %d RPM\n", i, *p);
        memset(buff, 0, 3);
        delay_loop();
    }

    printf("Temperature sensor values from PS%d:\n", j);
    for (i = 0; i < 2; i++) {
        i2c_read_seq(I2C_BUS0, ps, (0x00 + i), &buff, 2);
        swap(buff);
        p = (unsigned short *)buff;
        print_temp(*p);
        (i == 0 ) ? printf("   Inlet\n") : printf("   Outlet\n");
        memset(buff, 0, 3);
        delay_loop();
    }
}


/* Set the duty cycle of fans inside  power supply */
int
do_soho_psdc_i2c_set (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    unsigned short ps = PS_I2C_ADDR;
    unsigned short d = PSMI_DEFAULT_FAN_RPM;
    unsigned j = 1;
    unsigned char buff[3] = { 0 };
    uint32_t gpio_data  = GPIO_WRITE_DATA_VAL;
    uint32_t gpio_cfg = GPIO_CFG_IO_VAL;

    gpio_cfg_io_set((gpio_cfg | 0x3F) | (1 << 24));

    if (argc == 3) {
        j = (unsigned short)simple_strtoul(argv[1], NULL, 10);
        d = (int)simple_strtoul(argv[2], NULL, 10);
    } else {
        printf("Using default arguments. Do 'help swpsfs' to get right usage\n");
    }

    set_ps(j,gpio_data);

    printf("Selected Power supply PS%d(%x)\n", j, ps);

    buff[0] = 0; buff[1] = 0;
    i2c_read_seq(I2C_BUS0, ps, PSMI_FAN_CFG_REG_OFFSET, buff, 2);
    printf("Setting PS fans in system control mode (Register PSMI_FAN_CFG_REG_OFFSET : 0x%x%x)\n",
           buff[1],
           buff[0]);

    delay_loop();
    buff[0] = 0xcf; buff[1] = 0x00;
    i2c_write_seq(I2C_BUS0, ps, PSMI_FAN_CFG_REG_OFFSET, buff, 2);

    delay_loop();
    buff[0] = 0; buff[1] = 0;
    i2c_read_seq(I2C_BUS0, ps, PSMI_FAN_CFG_REG_OFFSET, buff, 2);
    printf("PS fans now in system control mode (Register PSMI_FAN_CFG_REG_OFFSET : 0x%x%x)\n",
           buff[1],
           buff[0]);

    delay_loop();
    printf("Setting up speed of power supply fans to %d (0x%x) RPM\n", d, d);

    buff[0] = (unsigned char)(d & 0x00ff);
    buff[1] = (unsigned char)((d & 0xff00) >> 8);
    i2c_write_seq(I2C_BUS0, ps, 0x24, buff, 2);

    return 0;
}


/* Set PSMi fan monitoring into auto mode */

int
do_soho_psdc_i2c_auto (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    unsigned short ps, j = 1;
    unsigned char buff[3] = { 0 };
    uint32_t gpio_data  = GPIO_WRITE_DATA_VAL;

    if (argc == 2) {
        j = (unsigned short)simple_strtoul(argv[1], NULL, 10);
    } else {
        printf("Using default arguments. Do 'help twpsfa' to get right usage\n");
    }

    set_ps(j,gpio_data);

    printf("Selected Power supply PS%d(%x)\n", j, ps);

    buff[0] = 0; buff[1] = 0;
    i2c_read_seq(I2C_BUS0, ps, PSMI_FAN_CFG_REG_OFFSET, buff, 2);
    printf("Setting PS fans in auto control mode (Register PSMI_FAN_CFG_REG_OFFSET : 0x%x%x)\n",
           buff[1],
           buff[0]);

    delay_loop();
    buff[0] = 0xc0; buff[1] = 0x00;
    i2c_write_seq(I2C_BUS0, ps, PSMI_FAN_CFG_REG_OFFSET, buff, 2);

    delay_loop();
    buff[0] = 0; buff[1] = 0;
    i2c_read_seq(I2C_BUS0, ps, PSMI_FAN_CFG_REG_OFFSET, buff, 2);
    printf("PS fans now in auto control mode (Register PSMI_FAN_CFG_REG_OFFSET : 0x%x%x)\n",
           buff[1], buff[0]);

    return 0;
}

int
set_tmp435_entended (int chan, int sensor, int enable)
{
    uint32_t gpio_data  = GPIO_WRITE_DATA_VAL;
    uint32_t gpio_cfg = GPIO_CFG_IO_VAL;
    uint8_t temp;
    
    gpio_cfg_io_set(gpio_cfg | 0x3F | (1 << 24));
    gpio_write_data_set((gpio_data & ~(0x3f))| chan | (1 << 24));
    udelay(1000);

    if (i2c_read(I2C_BUS0, 
                 soho_tmp435_sensor_addr[sensor-1], 0x3, &temp, 1))
        return -1;

    if (enable) {
        if ((temp & 0x4) == 0) {
            temp |= 0x4;
            if (i2c_write(I2C_BUS0, 
                          soho_tmp435_sensor_addr[sensor-1], 0x9, &temp, 1))
                return -1;
            soho_tmp435_extended_range[sensor - 1][chan - 4] = 1;
        }
    } else {
        if (temp & 0x4) {
            temp &= ~0x4;
            if (i2c_write(I2C_BUS0, 
                          soho_tmp435_sensor_addr[sensor-1], 0x9, &temp, 1))
                return -1;
            soho_tmp435_extended_range[sensor - 1][chan - 4] = 0;
        }
    }

    return 0;
}

int
set_tmp435_limits (int chan, int sensor, int limit_h, int limit_l, int therm)
{
    uint32_t gpio_data  = GPIO_WRITE_DATA_VAL;
    uint32_t gpio_cfg = GPIO_CFG_IO_VAL;
    uint8_t high_limit, low_limit, therm_value;
    int extend_adj = 0;
    int err = 0, low = 0, high = 191;
    
    gpio_cfg_io_set(gpio_cfg | 0x3F | (1 << 24));
    gpio_write_data_set((gpio_data & ~(0x3f))| chan | (1 << 24));
    udelay(1000);

    if (soho_tmp435_extended_range[sensor - 1][chan - 4]) {
        extend_adj = 0x40;
        low = -64;
    }
    
    if ((limit_h < low) || (limit_h) > high) {
        err++;
        printf("chan %d sensor %d high limit %d out of range %d..%d\n",
                chan, sensor, limit_h, low, high);
    }
    if ((limit_l < low) || (limit_l) > high) {
        err++;
        printf("chan %d sensor %d low limit %d out of range %d..%d\n",
                chan, sensor, limit_l, low, high);
    }
    if ((therm < low) || (therm) > high) {
        err++;
        printf("chan %d sensor %d therm %d out of range %d..%d\n",
                chan, sensor, therm, low, high);
    }
    if (limit_l >= limit_h) {
        err++;
        printf("chan %d sensor %d low limit %d >= high limit %d\n",
                chan, sensor, therm, low, high);
    }
    if (err)
        return -1;
    
    high_limit = (uint8_t)(limit_h + extend_adj);
    low_limit = (uint8_t)(limit_l + extend_adj);
    therm_value = (uint8_t)(therm + extend_adj);

    if (i2c_write(I2C_BUS0, soho_tmp435_sensor_addr[sensor-1],
                  0xb, &high_limit, 1))
        return -1;
    if (i2c_write(I2C_BUS0, soho_tmp435_sensor_addr[sensor-1], 
              0xc, &low_limit, 1))
        return -1;
    if (i2c_write(I2C_BUS0, soho_tmp435_sensor_addr[sensor-1], 
              0x20, &therm_value, 1))
        return -1;

    return 0;
}

int
read_tmp435_temp (int chan, int sensor, int *temp)
{
    uint32_t gpio_data  = GPIO_WRITE_DATA_VAL;
    uint32_t gpio_cfg = GPIO_CFG_IO_VAL;
    uint8_t tmp = 0;
    int res = 0;
    
    gpio_cfg_io_set(gpio_cfg | 0x3F | (1 << 24));
    gpio_write_data_set((gpio_data & ~(0x3f))| chan | (1 << 24));
    udelay(1000);

    res = i2c_read(I2C_BUS0, soho_tmp435_sensor_addr[sensor-1], 0, &tmp, 1);

    if (res == 0) {
        *temp = tmp;
        if (soho_tmp435_extended_range[sensor - 1][chan - 4]) {
            *temp -= 0x40;
        }
    }
    return res;
}

void
read_tmp435_config (int chan, int sensor, 
                    uint8_t *config1, uint8_t *high, 
                    uint8_t *low, uint8_t *therm)
{
    uint32_t gpio_data  = GPIO_WRITE_DATA_VAL;
    uint32_t gpio_cfg = GPIO_CFG_IO_VAL;
    
    gpio_cfg_io_set(gpio_cfg | 0x3F | (1 << 24));
    gpio_write_data_set((gpio_data & ~(0x3f))| chan | (1 << 24));
    udelay(1000);

    i2c_read(I2C_BUS0, soho_tmp435_sensor_addr[sensor-1], 0x3, config1, 1);
    i2c_read(I2C_BUS0, soho_tmp435_sensor_addr[sensor-1], 0x5, high, 1);
    i2c_read(I2C_BUS0, soho_tmp435_sensor_addr[sensor-1], 0x6, low, 1);
    i2c_read(I2C_BUS0, soho_tmp435_sensor_addr[sensor-1], 0x20, therm, 1);
}

int
do_gpio_i2c_temp (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uchar i;
    int chan;
    int temp[4];
    uint8_t temp_b[4];
    int ibegin = 0, iend = 4, adj = 0;

    if (argc >= 2) {
        chan = (uint32_t)simple_strtoul(argv[1], NULL, 10);
    }
    else
        goto usage;

    if ((chan != 4) && (chan != 5 )) {
        printf("No temperature sensors on channel %d on SOHO\n", chan);
        return -1;
    }

    if (argc == 2) {
        printf("TMP435 temperatures: (degree C)\n");
        for (i = 0; i < 4; i++) {
            read_tmp435_temp(chan, i+1, &temp[i]);
            printf("Temp[%d] %-6s %-13s: %4d\n", i+1,
                   soho_tmp435_sensor_pos[chan-4],
                   soho_tmp435_sensor_name[i],
                   temp[i]);
        }
    }

    if (((argc == 3) || (argc == 4)) && (!strncmp(argv[2],"get", 3))) {
        if (argc == 4) {
            iend = (int)simple_strtoul(argv[3], NULL, 10);
            ibegin = iend - 1;
        }
        printf("TMP435 config:\n");
        for (i = ibegin; i < iend; i++) {
            read_tmp435_config(chan, i+1, &temp_b[0], &temp_b[1], 
                               &temp_b[2], &temp_b[3]);
            temp[1] = temp_b[1];
            temp[2] = temp_b[2];
            temp[3] = temp_b[3];

            if (temp_b[0] & 0x4) {
                temp[1] -= 0x40;
                temp[2] -= 0x40;
                temp[3] -= 0x40;
            }
            printf("Temp[%d] %-6s %-13s: high %3d(%02x) low %3d(%02x) therm %3d(%02x)\n",
                   i+1,
                   soho_tmp435_sensor_pos[chan-4],
                   soho_tmp435_sensor_name[i],
                   temp[1], temp_b[1],
                   temp[2], temp_b[2],
                   temp[3], temp_b[3]
                   );
        }
    }

    if (((argc == 4) || (argc == 5)) && (!strncmp(argv[2],"extended", 3))) {
        if (argc == 5) {
            iend = (int)simple_strtoul(argv[4], NULL, 10);
            ibegin = iend - 1;
        }
        temp_b[0] = 0;
        if (!strncmp(argv[3],"enable", 3))
            temp_b[0] = 1;
        for (i = ibegin; i < iend; i++) {
            set_tmp435_entended(chan, i+1, temp_b[0]);
        }
    }

    if ((argc == 7) && (!strncmp(argv[2],"set", 3))) {
        iend = (int)simple_strtoul(argv[3], NULL, 10);
        ibegin = iend - 1;
        temp[1] = (int)simple_strtoul(argv[4], NULL, 10);
        temp[2] = (int)simple_strtoul(argv[5], NULL, 10);
        temp[3] = (int)simple_strtoul(argv[6], NULL, 10);
        for (i = ibegin; i < iend; i++) {
            if (soho_tmp435_extended_range[i][chan-4]) {
                adj = 0x40;
            }
            temp_b[1] = (uint8_t)(temp[1] + adj);
            temp_b[2] = (uint8_t)(temp[2] + adj);
            temp_b[3] = (uint8_t)(temp[3] + adj);
            set_tmp435_limits(chan, i+1, temp_b[1], temp_b[2], temp_b[3]);
        }
    }

    return 0;
    
 usage:
    printf("Usage:\n%s\n", cmdtp->usage);
    return 1;
}


int
read_max6695 (int offset, uint8_t *buf, int size)
{
    uint32_t chan = 4;
    uint32_t gpio_data  = GPIO_WRITE_DATA_VAL;
    uint32_t gpio_cfg = GPIO_CFG_IO_VAL;
    uint32_t bus = I2C_BUS0;
    
    if (fx_has_i2c_cpld()) {
        bus = TOR_CPLD_I2C_CREATE_MUX(I2C_BUS0, 0, 4);
    } else {
        gpio_cfg_io_set(gpio_cfg | 0x3F | (1 << 24));
        gpio_write_data_set((gpio_data & ~(0x3f))| chan | (1 << 24));
    }

    return (i2c_read(bus, SOHO_TEMP_MAX6697, offset, buf, size));
}

int
set_max6695 (int offset, uint8_t *buf, int size)
{
    uint32_t chan = 4;
    uint32_t gpio_data  = GPIO_WRITE_DATA_VAL;
    uint32_t gpio_cfg = GPIO_CFG_IO_VAL;
    uint32_t bus = I2C_BUS0;
    
    if (fx_has_i2c_cpld()) {
        bus = TOR_CPLD_I2C_CREATE_MUX(I2C_BUS0, 0, 4);
    } else {
        gpio_cfg_io_set(gpio_cfg | 0x3F | (1 << 24));
        gpio_write_data_set((gpio_data & ~(0x3f))| chan | (1 << 24));
    }

    return (i2c_write(bus, SOHO_TEMP_MAX6697, offset, buf, size));
}

int
do_itempm (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int i, sensor;
    uint8_t temp[SOHO_MAX6697_SENSORS] = {0, 0, 0, 0};

    switch (argc) {
        case 1:
            if (read_max6695(1, temp, 4)) {
                printf("MAX6697 temperatures read failed.\n");
            } else {
                printf("MAX6697 temperatures: (degree C)\n");
                for (i = 0; i < SOHO_MAX6697_SENSORS; i++) {
                    if (temp[i] == 0xff)
                        printf("%-3s(%d): N/A\n", soho_max6697_sensor[i], i+1);
                    else
                        printf("%-3s(%d): %3d\n", soho_max6697_sensor[i],
                               i+1, temp[i]);
                }
                printf("\n");
            }
            break;
        case 2:
            if (strncmp(argv[1],"alert", 5))
                goto usage;
            
            if (read_max6695(0x11, temp, 4)) {
                printf("MAX6697 alert read failed.\n");
            } else {
                printf("MAX6697 alert: (degree C)\n");
                for (i = 0; i < SOHO_MAX6697_SENSORS; i++) {
                    printf("%-3s(%d): %3d\n", soho_max6697_sensor[i],
                           i+1, temp[i]);
                }
                printf("\n");
            }
            break;
        case 3:
            if (strncmp(argv[1],"alert", 5))
                goto usage;
            
            sensor = (uchar)simple_strtoul(argv[2], NULL, 10);
            
            if ((sensor < 1) || (sensor > 4)) {
                printf("valid sensor range 1 .. 4.\n");
                return 1;
            }

            if (read_max6695(0x11 + sensor - 1, temp, 1)) {
                printf("%-3s alert read failed.\n", 
                       soho_max6697_sensor[sensor - 1]);
            } else {
                printf("%-3s(%d) alert: %3d\n",
                       soho_max6697_sensor[sensor - 1], sensor, temp[0]);
            }
            
            break;
        case 4:
            if (strncmp(argv[1],"alert", 5))
                goto usage;
            
            sensor = (uchar)simple_strtoul(argv[2], NULL, 10);
            
            if ((sensor < 1) || (sensor > 4)) {
                printf("valid sensor range 1 .. 4.\n");
                return 1;
            }

            temp[0] = (uint8_t)simple_strtoul(argv[3], NULL, 10);

            if (set_max6695(0x11 + sensor - 1, temp, 1) == 0) {
                printf("%-3s(%d) set alert %3d succeed.\n",  
                       soho_max6697_sensor[sensor - 1], sensor,temp[0]);
            }
            else {
                printf("%-3s(%d) set alert %3d failed.\n",
                       soho_max6697_sensor[sensor - 1], sensor, temp[0]);
            }
            
            break;
        default:
            goto usage;
            break;
    }
            
    return 0;
    
 usage:
    printf("Usage:\n%s\n", cmdtp->usage);
    return 1;
}

int
do_cpld_led_ctrl (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uchar color = 'g', port = 0, tot_ports = 1;
    int offset, offset1, val = 0, val1 = 0;

    switch (argc) {
        case 1:
            tot_ports = 48;
            port = 0;
            break;
        case 2:
            if (argv[1][0] >= 'a' && argv[1][0] <= 'z') {
                color = argv[1][0];
                tot_ports = 48;
                port = 0;
            } else   {
                port = (uchar)simple_strtoul(argv[1], NULL, 10);
            }
            break;
        case 3:
            port = (uchar)simple_strtoul(argv[1], NULL, 10);
            color = argv[2][0];
            break;
        default:
            printf("led [port#] [y|g|x] (default all 48 ports, green color\n");
            return 0;
    }

    do {
        switch (color) {
            case 'g':
                offset = 0x80 + (0x10 * (port / 8))  + 0x09;
                offset1 = 0x80 + (0x10 * (port / 8))  + 0x08;
                soho_cpld_read(offset, &val);
                soho_cpld_read(offset1, &val1);
                printf("Writing 0x%x to offset 0x%x (port-slave_cpld-index :%d-%d-%d)\n",
                       val,
                       offset,
                       port,
                       (port / 8),
                       (port % 8));
                val &= ~(1 << (port % 8));  /* green = 0 */
                val1 |= (1 << (port % 8));  /* led on = 1 */
                soho_cpld_write(offset1, val1);
                soho_cpld_write(offset, val);
                break;
            case 'y':
                offset = 0x80 + (0x10 * (port / 8))  + 0x09;
                offset1 = 0x80 + (0x10 * (port / 8))  + 0x08;
                soho_cpld_read(offset, &val);
                soho_cpld_read(offset1, &val1);
                printf("Writing 0x%x to offset 0x%x (port-slave_cpld-index :%d-%d-%d)\n",
                       val,
                       offset,
                       port,
                       (port / 8),
                       (port % 8));
                val |= (1 << (port % 8));  /* yellow = 1 */
                val1 |= (1 << (port % 8));  /* led on = 1 */
                soho_cpld_write(offset1, val1);
                soho_cpld_write(offset, val);
                break;
            default:
                offset1 = 0x80 + (0x10 * (port / 8))  + 0x08;
                val1 &= ~(1 << (port % 8));  /* led off = 0 */
                soho_cpld_write(offset1, val1);
        }
        port++;
    } while (--tot_ports);

    return 0;
}

/*voltage monitor command*/
int
do_soho_i2c_vol (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uint32_t gpio_data = GPIO_WRITE_DATA_VAL,chan=5;
    uint32_t gpio_cfg = GPIO_CFG_IO_VAL; 
    unsigned char i,val,hi,low;
    uint16_t adc_v;
    
    /* Expected voltages 
     * 1 - greater than 2 volts
     * 0 - less than 2 volts
     */

    unsigned char expects[ ] = { 1, 1, 1, 0, 
                                 1, 1, 1, 1,
                                 1, 1, 1, 1
                               };

    unsigned char actual[ ][12] = { "1.5 V", "1.2 V", "1.8 V",
                                    "3.3 V", "2.5 V", "1.5 V",
                                    "3.3 V", "xx  V", "xx  V",
                                    "xx  V", "xx  V", "12.0 V"
                                  };

    gpio_cfg_io_set(gpio_cfg | 0x3F | (1 << 24));
    gpio_write_data_set((gpio_data & ~(0x3f))| chan | (1 << 24) );

    printf("Reading Power CPLD voltage monitoring channels \n");
    printf(" Channel     Nominal Voltage      Actual Voltage\n");

    for (i = 0; i < 12; i++ ) {

      /* Read the attenuation reg on the mux in the pwr cpld chip */
      i2c_read(I2C_BUS0, PWR_CPLD_I2C_ADDR, PWR_CPLD_ATTN_REG, &val, 1);
      delay_loop();
      delay_loop();

      if (expects[i]) {
         /* Turn on the attenuation bit */
         val = 0xF0;
      } else {
         /* Clear the attenuation bit */
         val = 0xE0;
      }
    
      val |= i;

      i2c_write(I2C_BUS0, PWR_CPLD_I2C_ADDR, PWR_CPLD_ATTN_REG, &val, 1);
      delay_loop();
      delay_loop();

      /*Read Hi data part of volatage */
      i2c_read(I2C_BUS0, PWR_CPLD_I2C_ADDR, PWR_CPLD_VOL_HI, &hi, 1);
      delay_loop();
      delay_loop();

      /*Read Low data part of voltage */
      i2c_read(I2C_BUS0, PWR_CPLD_I2C_ADDR, PWR_CPLD_VOL_LOW, &low, 1);
      delay_loop();
      delay_loop();

      /* Convert the temperature into Decimal */
      adc_v = (hi << 8) | low;

      /* Shift right 4 times and multiply by resolution of 
       * 2 milli volts.Effectively, this is 'shift right' 3 times. 
       * We convert back to Volts by dividing value by 1000.
       */
      adc_v = adc_v >> 3;

      printf(" %d         %s   <-------------->  %d.%03d V\n",i+1, actual[i], 
             (adc_v / 1000),(adc_v % 1000));
    }
    return 0;
}

static uint16_t 
voltPWR1220Read (uint8_t sensor)
{
    uint8_t tdata, tdata1;
    uint16_t temp = 0;

    if (sensor >= SOHO_PWR1220_SENSORS)
        return (0);
    
    tdata = 0x10 + sensor;
    i2c_write(I2C_BUS0, PWR_CPLD_I2C_ADDR, PWR_CPLD_ATTN_REG, &tdata, 1);

    i2c_read(I2C_BUS0, PWR_CPLD_I2C_ADDR, PWR_CPLD_VOL_LOW, &tdata, 1);
    while ((tdata & 0x1) == 0) {
        udelay(100);
        i2c_read(I2C_BUS0, PWR_CPLD_I2C_ADDR, PWR_CPLD_VOL_HI, &tdata, 1);
    }
    i2c_read(I2C_BUS0, PWR_CPLD_I2C_ADDR, PWR_CPLD_VOL_HI, &tdata1, 1);
    temp = ((tdata1 * 16) + (tdata  / 16)) * 2;

    return (temp);
}

static void 
i2c_read_PWR1220_volt (void)
{
    int i;
    uint16_t volt[SOHO_PWR1220_SENSORS];
    
    for (i = 0; i < SOHO_PWR1220_SENSORS; i++) {
        volt[i] = voltPWR1220Read(i);
    }
    
    printf("\n");
    printf("PWR1220 voltage:\n");
    for (i = 0; i < SOHO_PWR1220_SENSORS; i++) {
        printf("%-20s = %2d.%03d\n", pwr1220_sensor_name[i], volt[i]/1000, 
               volt[i]%1000);
    }    
}

static uint16_t 
voltLTC2978Read (uint8_t sensor)
{
    uint8_t tdata = sensor;
    uint16_t temp = 0;

    if (sensor >= SOHO_LTC2978_SENSORS)
        return (0);
    
    i2c_write(I2C_BUS0, PWR_LTC2978_I2C_ADDR, 0, &tdata, 1);

    i2c_read(I2C_BUS0, PWR_LTC2978_I2C_ADDR, 0x8D, &temp, 2);

    return (temp);
}

static void 
i2c_read_LTC2978_volt (void)
{
    int i;
    uint16_t volt[SOHO_LTC2978_SENSORS];
    
    for (i = 0; i < SOHO_LTC2978_SENSORS; i++) {
        volt[i] = voltLTC2978Read(i);
    }
    
    printf("\n");
    printf("LTC2978 voltage:\n");
    for (i = 0; i < SOHO_LTC2978_SENSORS; i++) {
        printf("%-20s = %2d.%03d\n", ltc2978_sensor_name[i], volt[i]/1000, volt[i]%1000);
    }    
}

int
do_volt (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    unsigned char chan = 5;
    uint8_t trim = 0, regVal = 0;
    int margin_index = 0;
    uint32_t gpio_cfg = GPIO_CFG_IO_VAL;
    uint32_t gpio_data  = GPIO_WRITE_DATA_VAL;

    if (!fx_is_soho()) {
        printf("volt utilily supports SOHO TOR only.");
    }
    if (argc < 2)
        goto usage;

    if (!strncmp(argv[1],"pwr1220", 7)) {
        chan = 5;
    } else if (!strncmp(argv[1],"ltc2978", 7)) {
        chan = 7;
    }
    
    gpio_cfg_io_set(gpio_cfg | 0x3F | (1 << 24));
    gpio_write_data_set((gpio_data & ~(0x3f))| chan | (1 << 24));

    if (argc == 2) {
        if (chan == 5) {
            i2c_read_PWR1220_volt();
        } else if (chan == 7) {
            i2c_read_LTC2978_volt();
        }
    } else if (argc == 5) {
        trim = (uint8_t)simple_strtoul(argv[3], NULL, 16);
        
        if (!strncmp(argv[4],"low", 3)) {
            margin_index = SOHO_MARGIN_LOW;
        } else if (!strncmp(argv[4],"normal", 3)) {
            margin_index = SOHO_MARGIN_NORMAL;
        } else if (!strncmp(argv[4],"high", 3)) {
            margin_index = SOHO_MARGIN_HIGH;
        }
        
        if (chan == 5) {
            regVal = pwr1220_margin[trim][margin_index];
            trim += 0x13;
            
            i2c_write(I2C_BUS0, PWR_CPLD_I2C_ADDR, trim, &regVal, 1);
        } else if (chan == 7) {
            regVal = ltc2978_margin[margin_index];
            /* page */
            i2c_write(I2C_BUS0, PWR_LTC2978_I2C_ADDR, 0, &trim, 1);
            /* margin command */
            i2c_write(I2C_BUS0, PWR_LTC2978_I2C_ADDR, 1, &regVal, 1);
        }
    }
    else
        goto usage;
    
    return 0;
    
usage:
    printf("Usage:\n%s\n", cmdtp->usage);
    return 1;
}

int
do_cpld (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uint8_t *buf;
    uint16_t addr, data, len;
    uint32_t temp;
    uint8_t bus, sseg, pseg, dev;
    int offset, offset_size = 1, i2c_mode = 0, mux;
    int i, j;
    char *datap;
    char tbyte[3] = {0, 0, 0}, temp8;

    if (argc < 4)
        goto usage;
    
    if (!strncmp(argv[1],"i2c", 3)) {
        if (!strncmp(argv[3],"read", 4)) {
            addr = (uint16_t)simple_strtoul(argv[4], NULL, 16);
            len = 1;
            if (argc == 6)
                len = (uint16_t)simple_strtoul(argv[5], NULL, 10);
            for (i = 0; i < len; i++) {
                tor_cpld_i2c_controller_read(addr+i, &temp);
                printf("%04x: %08x\n", addr+i, temp);
            }
        } else if (!strncmp(argv[3],"write", 4)) {
            addr = (uint16_t)simple_strtoul(argv[4], NULL, 16);
            temp = simple_strtoul(argv[5], NULL, 16);
            tor_cpld_i2c_controller_write(addr, temp);
        } else {
            goto usage;
        }
    } else if (!strncmp(argv[1],"imd", 3)) {
        if (argc < 6)
            goto usage;
        
        mux = (uint16_t)simple_strtoul(argv[2], NULL, 16);
        dev = (uint8_t)simple_strtoul(argv[3], NULL, 16);
        offset = (uint16_t)simple_strtoul(argv[4], NULL, 16);
        len = simple_strtoul(argv[5], NULL, 16);
        offset_size = 1;
        for (i = 0; i < 8; i++) {
            if (argv[4][i] == '.') {
                offset_size = argv[4][i+1] - '0';
                if (offset_size > 2) {
                    goto usage;
                }
                break;
            } else if (argv[4][i] == '\0') {
                break;
            }
        }

        i2c_mode = (offset_size == 1) ? TOR_CPLD_I2C_8_OFF : TOR_CPLD_I2C_16_OFF;
        buf = malloc(TOR_CPLD_I2C_MAX_DATA);
        if (tor_cpld_i2c_read(mux, dev, offset, i2c_mode, buf, len)) {
            printf("cpld imd failed at offset %d mux=0x%x\n", offset, mux);
            free(buf);
            return 1;
        }
        for (i = 0; i < len; i++) {
            if ((i != 0) && ((i % 16) == 0)) {
                printf("\n");
            }                
            printf("%02x ", buf[i]);
        }
        free(buf);
        printf("\n");
    } else if (!strncmp(argv[1],"imw", 3)) {
        if (argc < 6)
            goto usage;
        
        mux = (uint16_t)simple_strtoul(argv[2], NULL, 16);
        dev = (uint8_t)simple_strtoul(argv[3], NULL, 16);
        offset = (uint16_t)simple_strtoul(argv[4], NULL, 16);
        offset_size = 1;
        for (i = 0; i < 8; i++) {
            if (argv[4][i] == '.') {
                offset_size = argv[4][i+1] - '0';
                if (offset_size > 2) {
                    goto usage;
                }
                break;
            } else if (argv[4][i] == '\0') {
                break;
            }
        }
        
        i2c_mode = (offset_size == 1) ? TOR_CPLD_I2C_8_OFF : TOR_CPLD_I2C_16_OFF;

        datap = argv [5];
        len = strlen(datap)/2;
        buf = malloc(TOR_CPLD_I2C_MAX_DATA);
        for (i = 0; i < len; i++) {
            tbyte[0] = datap[2*i];
            tbyte[1] = datap[2*i+1];
            temp8 = atoh(tbyte);
            buf[i] = temp8;
        }
        if (tor_cpld_i2c_write(mux, dev, offset, i2c_mode, buf, len)) {
            printf("cpld imw failed at offset %d\n", offset);
        }
        free(buf);
    } else if (!strncmp(argv[1],"iseqr", 5)) {
        if (argc < 6)
            goto usage;
        
        mux = (uint16_t)simple_strtoul(argv[2], NULL, 16);
        dev = (uint8_t)simple_strtoul(argv[3], NULL, 16);
        offset = (uint16_t)simple_strtoul(argv[4], NULL, 16);
        len = (uint16_t)simple_strtoul(argv[5], NULL, 16);;
        i2c_mode = TOR_CPLD_I2C_RSTART | TOR_CPLD_I2C_8_OFF;
        buf = malloc(TOR_CPLD_I2C_MAX_DATA);
        for (i = 0; i < len; ){
            if (i + 1 < len) {
                temp = 2;
            } else {
                temp = 1;
            }
            if (tor_cpld_i2c_read(mux, dev, offset + i, i2c_mode, buf, temp)) {
                printf("cpld iseqr failed at offset %d\n", offset);
                free(buf);
                return 1;
            }
            for (j = 0; j < temp; j++) {
                printf("%02x ", buf[j]);
            }
            if (i && ((i % 16) == 0)) {
                printf("\n");
            }
            i += temp;
        }
        printf("\n");
        free(buf);
    } else if (!strncmp(argv[1],"iseqw", 5)) {
        if (argc < 7)
            goto usage;
        mux = (uint16_t)simple_strtoul(argv[2], NULL, 16);
        dev = (uint8_t)simple_strtoul(argv[3], NULL, 16);
        offset = (uint16_t)simple_strtoul(argv[4], NULL, 16);
        len = (uint16_t)simple_strtoul(argv[5], NULL, 16);;
        data = (uint16_t)simple_strtoul(argv[6], NULL, 16);
        i2c_mode = TOR_CPLD_I2C_RSTART | TOR_CPLD_I2C_8_OFF;
        if (tor_cpld_i2c_write(mux, dev, offset, i2c_mode, &data, len)) {
            printf("cpld iseqw failed at offset %d\n", offset);
        }
    } else {
        goto usage;
    }
        
    return 0;
    
usage:
    printf("Usage:\n%s\n", cmdtp->usage);
    return 1;
}

int
test_cpld (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uint16_t i, loop_time;
    uint8_t test_val, reg_val, reg, cpld_no = 0, fail = 0;

    if (argc < 2) {
        return -1;
    }
    
    loop_time = (uint16_t)simple_strtoul(argv[1], NULL, 16);

    printf("%s: Will run %d times\n", __func__, loop_time);

    for (i = 0; i < loop_time; i++) {
        printf("\nLoop %d:\n", i);

        for (cpld_no = 0; cpld_no < 10; cpld_no++) {
            switch (cpld_no) {
            case 0:
                reg = 0x7D;
                break;

            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
                reg = 0x89 + 0x10 * cpld_no;
                break;

            case 7:
                reg = 0xe0;
                break;

            case 8:
                reg = 0xf2;
                break;

            case 9:
                reg =  0x71;
                break;
                
            default:
                printf("out of bound %d\n", cpld_no);
                break;
            }


            if (!(i % 2)) {
                test_val = 0x5A + cpld_no + i;
            } else {
                test_val = 0xA5 + cpld_no + i;
            }

            soho_cpld_write(reg, test_val);
            soho_cpld_read(reg, &reg_val);

            if (reg_val != test_val) {
                fail = 1;
                printf("Error: loop %d reg=0x%x reg_val=0x%x test_val=0x%x\n", 
                        i, reg, reg_val, test_val);
            } else {
                printf("Success reg=0x%x reg_val=0x%x test_val=0x%x\n", 
                        reg, reg_val, test_val);
            }
        } 
    }

    if (!fail) {
        printf("pass CPLD test\n");
    }
}


U_BOOT_CMD(
        srps, 2,  1,  do_soho_psmi_i2c_get,
        "srps - Read PS fan RPM and Temperature values\n",
        "[ps_unit#] (default: ps_unit : 1)"
        );

U_BOOT_CMD(
        swpsfs, 3,  1,  do_soho_psdc_i2c_set,
        "swpsfs - Write PS internal fan speed (in RPM)\n",
        "[ps_unit#] [RPM] (default: ps_unit: 1, RPM : 4000)"
        );

U_BOOT_CMD(
        swpsfa, 3,  1,  do_soho_psdc_i2c_auto,
        "swpsfa - Write PS internal fans to auto mode\n",
        "[ps_unit#] (default: ps_unit: 1)"
        );

U_BOOT_CMD(
        itemp,
        7,
        1,
        do_gpio_i2c_temp,
        "itemp   - TMP435 temperature sensors\n",
        "\n"
        "itemp [4|5]\n"
        "    - dumps channel [4|5] temp sensor values\n"
        "itemp [4|5] extended [enable|disable] [<1..4>]\n"
        "    - enable/disable extended range\n"
        "itemp [4|5] set <1..4> <high> <low> <therm>\n"
        "    - set high/low limit and therm\n"
        "itemp [4|5] get [<1..4>]\n"
        "    - get limits and therm\n"
        );

U_BOOT_CMD(
        itempm,
        4,
        1,
        do_itempm,
        "itempm  - MAX6697 temperature monitor\n",
        "\n"
        "itempm\n"
        "    - dumps temp sensor values\n"
        "itempm alert [<sensor #>][<limit>]\n"
        "    - get/set sensor 1..4 alert limits\n"
        );

U_BOOT_CMD(
        led,
        3,
        1,
        do_cpld_led_ctrl,
        "led \n",
        "- [port#] [x|y|g] (x- off (default), y- yellow, g - green)\n"
        );


U_BOOT_CMD(
        ivol,
        1,
        1,
        do_soho_i2c_vol,
        "ivol   - dump voltages from power cpld\n",
        "\n"
        );

U_BOOT_CMD(
        volt,
        5,
        1,
        do_volt,
        "ivolt   - voltage monitor and margin\n",
        "\n"
        "volt [pwr1220 | ltc2978]\n"
        "    - dumps voltage values\n"
        "volt [pwr1220 | ltc2978] margin <trim #> [low| normal | high]\n"
        "    - margin pwr1220 or ltc2978 trim 0-7\n"
        );

#if 1
U_BOOT_CMD(
        test_cpld,
        2,
        1,
        test_cpld,
        "test_cpld -- test cpld read/write\n",
        "\n"
        );
#endif

U_BOOT_CMD(
        cpld ,
        8,
        1,
        do_cpld,
        "cpld    - cpld i2c\n",
        "\n"
        "cpld i2c memory read <addr> [<len>]\n"
        "    - read CPLD i2c memory read\n"
        "cpld i2c memory write <addr> <data>\n"
        "    - read CPLD i2c memory write\n"
        "cpld imd <mux> <dev> <offset>[.1, .2] <len>\n"
        "    - CPLD i2c read bytes\n"
        "cpld imw <mux> <dev> <offset>[.1, .2] <data>\n"
        "    - CPLD i2c write bytes\n"
        "cpld iseqr <bus> <sseg> <pseg> <dev> <offset>\n"
        "    - CPLD i2c read word\n"
        "cpld iseqw <bus> <sseg> <pseg> <dev> <offset> <data>\n"
        "    - CPLD i2c write word\n"
        );
