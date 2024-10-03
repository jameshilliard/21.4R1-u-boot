/*
 * Copyright (c) 2009-2011, Juniper Networks, Inc.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>

#include "eth-phy/mvEthPhy.h"
#include "ethfp/gbe/mvEthRegs.h"
#include "prestera/hwIf/mvHwIf.h"
#include "ctrlEnv/sys/mvCpuIfRegs.h"
#include "pex/mvPexRegs.h"
#include "pci.h"
#include <pcie.h>
#include "post.h"

#if !defined(CONFIG_PRODUCTION)
#include "twsi/mvTwsi.h"
#include "gpp/mvGpp.h"
#include "mvCtrlEnvLib.h"
#include "cntmr/mvCntmr.h"
#include "syspld.h"
#include "led.h"
#include "hush.h"
#include <net.h>
#include <i2c.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

extern struct pcie_controller pcie_hose[];

extern int board_idtype(void);
extern int board_pci(void);

extern int eth_receive(volatile void *packet, int length);
extern int cmd_get_data_size(char* arg, int default_size);

void pcieinfo(int, int , int );
void pciedump(int, int );
static char *pcie_classes_str(unsigned char );
void pcie_header_show_brief(int, pcie_dev_t );
void pcie_header_show(int, pcie_dev_t );
int do_pcie_probe(cmd_tbl_t *, int , int , char *[]);

int ledUpdateEna = 1;

#if !defined(CONFIG_PRODUCTION)
extern int i2c_read_norsta(uint8_t chanNum, uchar dev_addr, 
                       unsigned int offset, int alen, MV_U8* data, int len);
#endif
int i2c_mux_busy = 0;

uint32_t 
switchClkGet (void)
{
    uint32_t regVal;
    
    /* get it only on first time */
    if (gd->sw_clk == 0) {
        mvSwitchReadReg(0, 0x2c, &regVal);
        regVal = (regVal & 0x1c) >> 2;
        switch (regVal)
        {
            case 0:
                gd->sw_clk = 250000000;
                break;
            case 1:
                gd->sw_clk = 222000000;
                break;
            case 2:
                gd->sw_clk = 200000000;
                break;
            case 3:
                gd->sw_clk = 182000000;
                break;
            case 4:
                gd->sw_clk = 167000000;
                break;
            case 5:
                gd->sw_clk = 154000000;
                break;
            case 6:
                gd->sw_clk = 143000000;
                break;
            case 7:
                gd->sw_clk = 133000000;
                break;
        }
            
    }

    return gd->bus_clk;
}

int 
i2c_mux (uint8_t mux, uint8_t chan, int ena)
{
    uint8_t ch;
    int ret = 0;

    if (ena) {
        i2c_mux_busy = 1;
        ch = 1 << chan;
        ret = i2c_write(0, mux, 0, 0, &ch, 1);
    }
    else {
        ch = 0;
        ret = i2c_write(0, mux, 0, 0, &ch, 1);
        i2c_mux_busy = 0;
    }

    return ret;
}


#if !defined(CONFIG_PRODUCTION)
static int 
twos_complement (uint8_t temp)
{
    return (int8_t)temp;
}

static int 
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

    return res;
}

static int 
atod (char* string)
{
    int res = 0;

    while (*string != 0)
    {
        res *= 10;
        if (*string >= '0' && *string <= '9')
            res += *string - '0';
        else
            break;
        string++;
    }

    return res;
}

#define  MAX_BUF_LEN        256
#define BASIC_HDR_LEN       4 
#define SEC_IC_NO_OFFSET    -1

/* secure eeprom commands */
#define  GET_M2_VERSION            0xC0
#define  WRITE_EEPROM_DATA         0xC1
#define  HW_AUTHENTICATE           0xC2
#define  SW_AUTHENTICATE           0xC3
#define  GET_HW_PUB_INFO           0xC4
#define  GET_HW_PUB_SIGN           0xC5
#define  GET_HW_UID                0xC6 
#define  GET_CHIP_STATUS           0xC7

/* error commands */
#define  STATUS_SUCCESS       0xE0
#define  STATUS_ERROR         0xE1
#define  STATUS_NOT_AVAILABLE 0xE2   /* secure eeprom is busy */ 
#define  STATUS_WRITE_ERROR   0xE3
#define  STATUS_READ_ERROR    0xE4 

/* i2c command structures */
typedef struct _hw_cmd_ {
    unsigned char i2c_cmd;
    unsigned char flags;
    unsigned short len;
} hw_cmd_t;

typedef struct _eeprom_cmd_ {
   hw_cmd_t cmd;
   unsigned short eeprom_addr;
   unsigned short eeprom_len;
   unsigned char buf[MAX_BUF_LEN];
} eeprom_cmd_t;

int
secure_eeprom_write (unsigned i2c_addr, int addr, void *in_buf, int len )
{
    int            status = 0;
    eeprom_cmd_t  eeprom_cmd;
    unsigned short length;

        if (len > 128) {
            printf("secure eeprom size > 128 bytes.\n");
            return (-1);
        }
            
        if (len >= MAX_BUF_LEN) {
            length = MAX_BUF_LEN;
        } else {
            length = len;
        }
        eeprom_cmd.eeprom_len = swap_ushort(length);
        eeprom_cmd.eeprom_addr = swap_ushort(addr);

        eeprom_cmd.cmd.i2c_cmd = WRITE_EEPROM_DATA;
        eeprom_cmd.cmd.flags = 0;
        eeprom_cmd.cmd.len = swap_ushort(sizeof(hw_cmd_t) + 4 + length);

        memcpy((void *)eeprom_cmd.buf, in_buf, length);

        status = i2c_write (0, i2c_addr, 0, 0,
                                              (uint8_t *) &eeprom_cmd, eeprom_cmd.cmd.len);
        if (status) {
            printf("secure eeprom write failed status = %d.\n", status);
            return status;
        }

        udelay (50000);  /* ~500 bytes */

    return status;
}

int
secure_eeprom_read (unsigned dev_addr, unsigned offset, 
       uchar *buffer, unsigned cnt)
{
    int alen = (offset > 255) ? 2 : 1;
    
    return i2c_read_norsta(0, dev_addr, offset, alen, buffer, cnt);
}

uint8_t 
tempRead (int32_t sensor)
{
    uint8_t tdata[2] = {0, 0};

    if (sensor >= 5)
        return (0);
    
    i2c_mux(I2C_MAIN_MUX_ADDR, 2, 1);  /* enable mux chan 2 */
    switch(sensor) {
        case 0:
            i2c_read(0, I2C_WINBOND_ADDR, 0x27, 1, tdata, 1);
            break;
            
        case 1:
            i2c_read(0, I2C_WINBOND_2_ADDR, 0, 1, tdata, 2);
            break;
            
        case 2:
            i2c_read(0, I2C_WINBOND_1_ADDR, 0, 1, tdata, 2);
            break;
            
        case 3:
            i2c_read(0, I2C_TEMP_1_ADDR, 0, 1, tdata, 1);
            break;
            
        case 4:
            i2c_read(0, I2C_TEMP_1_ADDR, 1, 1, tdata, 1);
            break;
            
        default:
            break;       
    }

    i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);  /* disable mux */

    if ((sensor <= 2) && (tdata[0] == 0xff))  /* read failure */
        tdata[0] = 0;
    
    return tdata[0];
}

int 
i2c_read_temp (uint32_t threshold)
{
    uint8_t temp1[5] = {0, 0, 0, 0, 0};
    int16_t temp2[5] = {0, 0, 0, 0, 0};
    int i;
    int res = 0;

    for (i = 0; i < 5; i++) {
        temp1[i] = tempRead(i);
    }

    temp2[0] = twos_complement(temp1[0]);
    temp2[1] = twos_complement(temp1[1]);
    temp2[2] = twos_complement(temp1[2]);
    temp2[3] = twos_complement(temp1[3]);
    temp2[4] = twos_complement(temp1[4]);

    /* adjustment */
    temp2[1] = 1.3 * temp2[1] + 11;
    temp2[2] = 1.3 * temp2[2] + 11;

    printf("Temperature (deg C): Winbond (TEMPn), NE1617A (Ln/Rn)\n");
    printf(" TEMP1    TEMP2    TEMP3   L1/R1\n");
    printf("%3d(%02x)  %3d(%02x)  %3d(%02x)  %2d/%2d\n",
                temp2[0], temp1[0], temp2[1], temp1[1], temp2[2], temp1[2], 
                temp2[3], temp2[4]);
    printf("\n");

    for (i = 0; i < 5; i++) {
        if (temp2[i] > threshold) {
            res = 1;
            break;
        }
    }
    
    return res;
}

uint16_t 
voltRead (int32_t sensor)
{
    uint8_t tdata;
    uint16_t temp = 0;

    if (sensor >= 5)
        return (0);
    
    i2c_mux(I2C_MAIN_MUX_ADDR, 2, 1);  /* enable mux chan 2 */
    switch(sensor) {
        case 0:
            i2c_read(0, I2C_WINBOND_ADDR, 0x20, 1, &tdata, 1);
            temp = tdata * 16;
            break;
            
        case 1:
            i2c_read(0, I2C_WINBOND_ADDR, 0x21, 1, &tdata, 1);
            temp = tdata * 16;
            break;
            
        case 2:
            i2c_read(0, I2C_WINBOND_ADDR, 0x22, 1, &tdata, 1);
            temp = tdata * 16;
            break;
            
        case 3:
            i2c_read(0, I2C_WINBOND_ADDR, 0x23, 1, &tdata, 1);
            temp = tdata * 2688 / 100;
            break;
            
        case 4:
            i2c_read(0, I2C_WINBOND_ADDR, 0x24, 1, &tdata, 1);
            temp = tdata * 608 / 10;
            break;
                       
        default:
            break;
            
    }
    i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);  /* disable mux */
    
    return temp;
}

void 
i2c_read_volt (void)
{
    int i;
    uint16_t volt[5];
    
    for (i = 0; i < 5; i++) {
        volt[i] = voltRead(i);
    }    
    printf("\n");
    for (i = 0; i < 5; i++) {
        printf("volt%d = %2d.%03d\n", i, volt[i]/1000, volt[i]%1000);
    }    
}

void 
i2c_set_fan (int fan, int duty)
{
    uint8_t tdata[2];

    tdata[0] = duty * 0xFF / 100;
    i2c_mux(I2C_MAIN_MUX_ADDR, 2, 1);  /* enable mux chan 2 */

    switch (fan) {
        case 1:
            i2c_write(0, I2C_WINBOND_ADDR, 0x5b, 1, tdata, 1); /* fan 1 TAC */
            break;
                        
        case 2:
            i2c_write(0, I2C_WINBOND_ADDR, 0x5e, 1, tdata, 1); /* fan 2 TAC */
            break;
                        
        case 3:
            i2c_write(0, I2C_WINBOND_ADDR, 0x5f, 1, tdata, 1); /* fan 3 TAC */
            break;
    }
    i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);  /* disable mux */
}


void 
i2c_reset (void)
{
	MV_TWSI_ADDR slave;
    
	slave.type = ADDR7_BIT;
	slave.address = 0;
	mvTwsiInit(0, CFG_I2C_SPEED, CFG_TCLK, &slave, 0);
}

/* I2C commands
 *
 * Syntax:
 */
int 
do_i2c (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char cmd1;
    ulong mux = 0, device = 0, channel = 0, value;
    unsigned char bvalue;
    uint regnum = 0;
    int i = 0, ret, reg = 0, temp, len = 0, offset = 0, secure = 0, nbytes;
    char tbyte[3];
    uint8_t tdata[1024];
    char *data;
    uint16_t cksum = 0;
    ulong mloop = 1, threshold = 255;
    uint8_t count1, count2, count3;
    uint32_t duty = 0;
    int32_t rpm1, rpm2, rpm3;
    uint8_t temp1, temp2, temp3, temp4;
    extern char console_buffer[];

    if (argc < 2)
        goto usage;
    
    cmd1 = argv[1][0];
    
    switch (cmd1) {
        case 'r':          /* read/reset */
        case 'R':

            i2c_reset();
            break;
            
        case 'b':   /* byte read/write */
        case 'B':
            device = simple_strtoul(argv[2], NULL, 16);
            if ((argv[1][1] == 'r') ||(argv[1][1] == 'R')) {
                bvalue = 0xff;
                if ((ret = i2c_read(0, device, 0, 0, &bvalue, 1)) != 0) {
                    printf("i2c failed to read from device 0x%x.\n", device);
                    return (0);
                }
                printf("i2c read addr=0x%x value=0x%x\n", device, bvalue);
            }
            else {
                if (argc <= 3)
                    goto usage;
                
                bvalue = simple_strtoul(argv[3], NULL, 16);
                if ((ret = i2c_write(0, device, 0, 0, &bvalue, 1)) != 0) {
                    printf("i2c failed to write to address 0x%x value 0x%x.\n", 
                           device, bvalue);
                    return (0);
                }
            }
            
            break;
            
        case 'm':   
        case 'M':
            if ((argv[1][1] == 'u') ||(argv[1][1] == 'U')) {  /* mux channel */
                if (argc <= 3)
                    goto usage;
            
                mux = simple_strtoul(argv[3], NULL, 16);
                channel = simple_strtoul(argv[4], NULL, 10);
            
                if ((argv[2][0] == 'e') ||(argv[2][0] == 'E')) {
                    if ((ret = i2c_mux(mux, channel, 1)) != 0) {
                        printf("i2c failed to enable mux 0x%x channel %d.\n", 
                               mux, channel);
                    }
                    else
                        printf("i2c enabled mux 0x%x channel %d.\n", 
                               mux, channel);
                }
                else {
                    if ((ret = i2c_mux(mux, 0, 0)) != 0) {
                        printf("i2c failed to disable mux 0x%x channel %d.\n", 
                               mux, channel);
                    }
                    else
                        printf("i2c disabled mux 0x%x channel %d.\n", 
                               mux, channel);
                }
            }
            break;
            
        case 'd':   /* Data */
        case 'D':
            if (argc <= 4)
                goto usage;
            
            device = simple_strtoul(argv[2], NULL, 16);
            regnum = simple_strtoul(argv[3], NULL, 16);
            reg = 1;
            if (reg == 0xff) {
                reg = 0;
                regnum = 0;
            }
            if ((argv[1][1] == 'r') ||(argv[1][1] == 'R')) {  /* read */
                len = simple_strtoul(argv[4], NULL, 10);
                if (len) {   
                    if ((ret = i2c_read(0, device, regnum, reg, 
                                        tdata, len)) != 0) {
                        printf ("I2C read from device 0x%x failed.\n", device);
                    }
                    else {
                        printf ("The data read - \n");
                        for (i = 0; i < len; i++)
                            printf("%02x ", tdata[i]);
                        printf("\n");
                    }
                }
            }
            else {  /* write */
                data = argv [4];
                len = strlen(data)/2;
                tbyte[2] = 0;
                for (i = 0; i < len; i++) {
                    tbyte[0] = data[2*i];
                    tbyte[1] = data[2*i+1];
                    temp = atoh(tbyte);
                    tdata[i] = temp;
                }
                if ((argv[5][0] == 'c') ||(argv[5][0] == 'C')) {
                    for (i = 0; i < len; i++) {
                       cksum += tdata[i];
                    }
                    tdata[len] = (cksum & 0xff00) >> 8;
                    tdata[len+1] = cksum & 0xff;
                    len = len + 2;
                }
                if ((ret = i2c_write(0, device, regnum, reg, tdata, len)) != 0)
                    printf ("I2C write to device 0x%x failed.\n", device);
            }
                        
            break;
            
        case 'e':   /* EEPROM */
        case 'E':
            if (argc <= 4)
                goto usage;
            
            device = simple_strtoul(argv[2], NULL, 16);
            offset = simple_strtoul(argv[3], NULL, 10);
            if ((argv[1][1] == 'r') ||(argv[1][1] == 'R')) {  /* read */
                len = simple_strtoul(argv[4], NULL, 10);
                if (len) {
                    if ((argv[1][2] == 's') ||(argv[1][2] == 'S')) {
                        if ((ret = secure_eeprom_read(device, offset, 
                                    tdata, len)) != 0) {
                            printf ("I2C read from secure EEPROM device 0x%x "
                                    "failed.\n", device);
                            return (-1);
                        }
                    }
                    else {
                        if ((ret = eeprom_read(device, offset, 
                                tdata, len)) != 0) {
                            printf ("I2C read from EEPROM device 0x%x "
                                "failed.\n", device);
                            return (-1);
                        }
                    }
                    printf ("The data read from offset - \n", offset);
                    for (i = 0; i < len; i++)
                        printf("%02x ", tdata[i]);
                    printf("\n");
                }
            }
            else {  /* write offset auto increment */
                 if ((argv[1][2] == 'o') ||(argv[1][2] == 'O')) {
                    if ((argv[4][0] == 's') ||(argv[4][0] == 'S'))
                        secure = 1;

                    /* Print the address, followed by value.  Then accept input
                     *  for the next value.  A non-converted value exits.
                     */
                    do {
                        if (secure) {
                            if ((ret = secure_eeprom_read(device, offset, 
                                    tdata, 1)) != 0) {
                                printf ("I2C read from EEPROM device 0x%x "
                                    "at 0x%x failed.\n", device, offset);
                                return (1);
                            }
                        }
                        else {
                            if ((ret = eeprom_read(device, offset, 
                                    tdata, 1)) != 0) {
                                printf ("I2C read from EEPROM device 0x%x "
                                    "at 0x%x failed.\n", device, offset);
                                return (1);
                            }
                        }
                        printf("%4d: %02x", offset, tdata[0]);

                        nbytes = readline (" ? ");
                        if (nbytes == 0 || 
                            (nbytes == 1 && console_buffer[0] == '-')) {
                            /* <CR> pressed as only input, don't modify current
                             * location and move to next. "-" pressed will go 
                             * back.
                             */
                            offset += nbytes ? -1 : 1;
                            nbytes = 1;
                        }
                        else {
                            char *endp;
                            value = simple_strtoul(console_buffer, &endp, 16);
                            nbytes = endp - console_buffer;
                            if (nbytes) {
                                tdata[0] = value;
                                if (secure) {
                                    if ((ret = secure_eeprom_write(device, 
                                            offset, tdata, 1)) != 0) {
                                        printf ("I2C write to EEPROM device 0x%x "
                                            "at 0x%x failed.\n", device, offset);
                                        return (1);
                                    }
                                }
                                else {
                                    if ((ret = eeprom_write(device, offset,
                                            tdata, 1)) != 0) {
                                        printf ("I2C write to EEPROM device 0x%x "
                                            "at 0x%x failed.\n", device, offset);
                                        return (1);
                                    }
                                }
                            }
                            offset += 1;
                        }
                    } while (nbytes);
                    return (1);
                 }

                data = argv [4];
                if ((argv[5][0] == 'h') || (argv[5][0] == 'H')) {
                    len = strlen(data)/2;
                    tbyte[2] = 0;
                    for (i = 0; i < len; i++) {
                        tbyte[0] = data[2*i];
                        tbyte[1] = data[2*i+1];
                        temp = atoh(tbyte);
                        tdata[i] = temp;
                    }
                }
                else {
                    len = strlen(data);
                    for (i = 0; i < len; i++) {
                        tdata[i] = data[i];
                    }
                }
                if ((argv[1][2] == 's') ||(argv[1][2] == 'S')) {
                    if ((ret = secure_eeprom_write(device, offset, 
                            tdata, len)) != 0) {
                        printf ("I2C write to EEPROM device 0x%x failed.\n", 
                                device);
                    }
                }
                else {
                    if ((ret = eeprom_write(device, offset, 
                            tdata, len)) != 0) {
                        printf ("I2C write to EEPROM device 0x%x failed.\n", 
                                device);
                    }
                }
            }
                        
            break;
            
        case 't':          /* I2C temperature */
        case 'T':
            value = 1000000;
            if (argc > 2)
                value = simple_strtoul(argv[2], NULL, 10);
            if (argc > 3)
                mloop = simple_strtoul(argv[3], NULL, 10);
            if (argc > 4)
                threshold = simple_strtoul(argv[4], NULL, 10);

            if (mloop) {
                for (i = 0; i < mloop; i++) {
                    if (i2c_read_temp(threshold))
                        break;
                    if (ctrlc()) {
                        break;
                    }
                    udelay(value);
                }
            }
            else {
                for (;;) {
                    mloop++;
                    printf("loop %6d\n", mloop);
                    
                    if (i2c_read_temp(threshold))
                        break;
                    if (ctrlc()) {
                        break;
                    }
                    udelay(value);
                }
            }
            break;
            
        case 'v':          /* I2C voltage */
        case 'V':
            i2c_read_volt();
            break;
            
        case 'f':          /* I2C fan */
        case 'F':
            if (argc < 2)
                goto usage;

            value = 1000000;
            if (argc == 3)
                value = simple_strtoul(argv[2], NULL, 10) * 1000;
            if (argc == 5) {
                if ((argv[3][0] == 'd') ||(argv[3][0] == 'D')) {
                    value = simple_strtoul(argv[2], NULL, 10);
                    if ((value <= 0) || (value >3)) {
                        printf("Fan number %d out of range 1 - 3.\n", value);
                    }
                    duty = simple_strtoul(argv[4], NULL, 10);
                    if ((duty < 0) || (duty >100)) {
                        printf("Fan duty %d out of range 0 - 100.\n", duty);
                    }
                }
                else 
                    goto usage;
            }

            if (argc == 5) {
                i2c_set_fan(value, duty);
                udelay(3000000);
                count1 = 0xff;
                i2c_mux(I2C_MAIN_MUX_ADDR, 2, 1);  /* enable mux chan 2 */
                switch (value) {
                    case 1:
                        i2c_read(0, I2C_WINBOND_ADDR, 0x28, 1, &count1, 1);
                        break;
                        
                    case 2:
                        i2c_read(0, I2C_WINBOND_ADDR, 0x29, 1, &count1, 1);
                        break;
                        
                    case 3:
                        i2c_read(0, I2C_WINBOND_ADDR, 0x2a, 1, &count1, 1);
                        break;
                }

                i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);  /* disable mux */
                
                rpm1 = 0;
                if (count1 != 0xff)
                    rpm1 = 1350000  / (8 * count1);
                printf("Fan %d duty %d percent RPM = %d.\n", value, duty, 
                        rpm1);
                return (1);
            }

            i2c_mux(I2C_MAIN_MUX_ADDR, 2, 1);  /* enable mux chan 2 */
            tdata[0] = 0x20;
            i2c_write(0, I2C_WINBOND_ADDR, 0x5b, 1, tdata, 1); /* fan 1 TAC */
            i2c_write(0, I2C_WINBOND_ADDR, 0x5e, 1, tdata, 1); /* fan 2 TAC */
            i2c_write(0, I2C_WINBOND_ADDR, 0x5f, 1, tdata, 1); /* fan 3 TAC */
            udelay(3000000);

            printf("              Fan 1      Fan 2      Fan 3\n");
            for (i = 0; i <= 0xf; i++) {
                count1 = count2 =count3 = 0xff;
                tdata[0] = i * 0x10 + 0xf;
                i2c_write(0, I2C_WINBOND_ADDR, 0x5b, 1, tdata, 1); /* fan 1 */
                i2c_write(0, I2C_WINBOND_ADDR, 0x5e, 1, tdata, 1); /* fan 2 */
                i2c_write(0, I2C_WINBOND_ADDR, 0x5f, 1, tdata, 1); /* fan 3 */
                udelay(value);
                i2c_read(0, I2C_WINBOND_ADDR, 0x28, 1, &count1, 1); /* 1 */
                i2c_read(0, I2C_WINBOND_ADDR, 0x29, 1, &count2, 1); /* 2 */
                i2c_read(0, I2C_WINBOND_ADDR, 0x2a, 1, &count3, 1); /* 3 */
                duty = tdata[0] * 100 / 0xff;
                rpm1 = rpm2 = rpm3 = 0;
                if ((count1 != 0xff) && (count1 != 0x0))
                    rpm1 = 1350000  / (8 * count1);
                if ((count2 != 0xff) && (count2 != 0x0))
                    rpm2 = 1350000  / (8 * count2);
                if ((count3 != 0xff) && (count3 != 0x0))
                    rpm3 = 1350000  / (8 * count3);
                
                printf("Duty %3d\%", duty);
                printf("    %4dRPM    %4dRPM    %4dRPM\n", rpm1, rpm2, rpm3);
            }
            i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);  /* disable mux */
            break;
            
        case 's':   /* uplink SFP*/
        case 'S':
            if (argc <= 2)
                goto usage;
            
            if (argc == 3) {
                if ((argv[2][0] == 't') ||(argv[2][0] == 'T')) { /* temp */
                    temp1 = temp2 = temp3 = temp4 = 0;
                    i2c_mux(I2C_MAIN_MUX_ADDR, 4, 1);  /* enable mux chan 4 */
                    i2c_read(0, 0x51, 0x60, 1, &temp1, 1);
                    i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);  /* disable mux */
                    i2c_mux(I2C_MAIN_MUX_ADDR, 5, 1);  /* enable mux chan 5 */
                    i2c_read(0, 0x51, 0x60, 1, &temp2, 1);
                    i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);  /* disable mux */
                    i2c_mux(I2C_MAIN_MUX_ADDR, 6, 1);  /* enable mux chan 6 */
                    i2c_read(0, 0x51, 0x60, 1, &temp3, 1);
                    i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);  /* disable mux */
                    i2c_mux(I2C_MAIN_MUX_ADDR, 7, 1);  /* enable mux chan 7 */
                    i2c_read(0, 0x51, 0x60, 1, &temp4, 1);
                    i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);  /* disable mux */

                    printf("Temperature (deg C): SFP (TEMPn)\n");
                    printf("TEMP1   TEMP2   TEMP3   TEMP4\n");
                    printf("%3d     %3d     %3d     %3d\n",
                            twos_complement(temp1), 
                            twos_complement(temp2), 
                            twos_complement(temp3), 
                            twos_complement(temp4));
                    printf("\n");
                }
            }
            else if (argc == 6) {
                if ((argv[2][0] == 'e') ||(argv[2][0] == 'E')) { /* EEPROM */
                    device = simple_strtoul(argv[3], NULL, 10);
                    offset = simple_strtoul(argv[4], NULL, 10);
                    len= simple_strtoul(argv[5], NULL, 10);
                    if ((device >= 0) && (device <= 3)) {
                        if (len) {
                            i2c_mux(I2C_MAIN_MUX_ADDR, 4+device, 1);
                            if ((ret = eeprom_read(0x50, offset, tdata, len)) != 0) {
                                printf ("I2C read SFP %d EEPROM failed.\n", 
                                        device);
                                i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);
                                return (-1);
                            }
                            i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);
                       }
                        printf ("The SFP %d EEPROM offset- \n", device);
                        for (i = 0; i < len; i++) {
                            if (i % 16 == 0) {
                                printf("%04x  ", offset+i);
                            }
                                
                            printf("%02x ", tdata[i]);
                            if (i % 16 == 0xf)
                                printf("\n");
                        }
                        printf("\n");
                    }
                    else
                        printf("SFP module %d out of range 0-3.\n", device);
                }
            }
            else
                goto usage;

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

void 
syspld_dump (void)
{
    int i;
    uint8_t temp[100];

    if (syspld_read(0, temp, SYSPLD_LAST_REG+1))
        return;

    for (i = 0; i <= SYSPLD_LAST_REG; i++) {
        printf("SYSPLD addr 0x%02x = 0x%02x.\n", i, temp[i]);
    }
    printf("\n");
}

int 
do_syspld (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int addr = 0;
    uint8_t value = 0;
    int enable = 1, index = 0, shift = 0, rassert = 0;
    int port = 0, duplex = 0, speed = 0, link = 0;
    int led = 0, color = 0, mode = 0;
    char cmd1_0, cmd1_1, cmd1_2, cmd2;

    if (argc <= 2)
        goto usage;
    
    cmd1_0 = argv[1][0];
    cmd1_1 = argv[1][1];
    cmd1_2 = argv[1][2];
    cmd2 = argv[2][0];
    
    switch (cmd1_0) {
        case 'r':       /* Register/Reset */
        case 'R':
            if ((cmd1_1 != 'e') && (cmd1_1 != 'E'))
                goto usage;
            
            if ((cmd1_2 == 's') || (cmd1_2 == 'S')) { /* Reset */
                if (argc <= 3)
                    goto usage;

                if ((cmd2=='a') || (cmd2=='A')) {
                    rassert = 1;
                }
                else if ((cmd2=='c') || (cmd2=='C')) {
                    rassert = 0;
                }
                else
                    goto usage;

                index = simple_strtoul(argv[3], NULL, 10);

                switch (index) {
                    case 1:
                        shift = 7;
                        break;

                    case 2:
                        shift = 6;
                        break;

                    case 3:
                        shift = 5;
                        break;
                        
                    case 4:
                        shift = 4;
                        break;
                        
                    case 5:
                        shift = 2;
                        break;
                        
                    case 6:
                        shift = 1;
                        break;
                        
                    case 7:
                        shift = 0;
                        break;

                    default:
                        printf("???");
                        goto usage;
                }
            
                addr = SYSPLD_RESET_REG_1;

                if (syspld_reg_read(addr, &value))
                    printf("SYSPLD addr 0x%02x read failed.\n", addr);
                else {
                    if (rassert)
                        value |= 1<<shift;
                    else
                        value &= ~(1<<shift);

                    if (syspld_reg_write(addr, value))
                        printf("SYSPLD write addr 0x%02x value 0x%02x "
                                "failed.\n", addr, value);
                    else {
                        if (syspld_reg_read(addr, &value))
                            printf("SYSPLD addr 0x%02x read back failed.\n", 
                                    addr);
                        else
                            printf("SYSPLD read back addr 0x%02x = 0x%02x.\n",
                                    addr, value);
                    }
                }
                return (1);
            }
            else if ((cmd1_2 == 'g') || (cmd1_2 == 'G')) { /* Register */
            }
            else
                goto usage;
            
            switch (cmd2) {
                case 'd':   /* dump */
                case 'D':
                    syspld_dump();
                    break;
                    
                case 'r':   /* read */
                case 'R':
                    if (argc <= 3)
                        goto usage;
                    
                    addr = simple_strtoul(argv[3], NULL, 16);
                    if (syspld_reg_read(addr, &value))
                        printf("SYSPLD addr 0x%02x read failed.\n", addr);
                    else
                        printf("SYSPLD addr 0x%02x = 0x%02x.\n", addr, value);
                    break;
                    
                case 'w':   /* write */
                case 'W':
                    if (argc <= 4)
                        goto usage;
                    
                    addr = simple_strtoul(argv[3], NULL, 16);
                    value = simple_strtoul(argv[4], NULL, 16);
                    if (syspld_reg_write(addr, value))
                        printf("SYSPLD write addr 0x%02x value 0x%02x "
                                "failed.\n", addr, value);
                    else {
                        if (syspld_reg_read(addr, &value))
                            printf("SYSPLD addr 0x%02x read back failed.\n",
                                    addr);
                        else
                            printf("SYSPLD read back addr 0x%02x = 0x%02x.\n",
                                    addr, value);
                    }
                    break;
                    
                default:
                    printf("???");
                    break;
            }
            
            break;
            
        case 'd':       /* Device */
        case 'D':
            if (argc <= 3)
                goto usage;
            
            if ((cmd2=='d') || (cmd2=='D')) {
                enable = 0;
            }
            else if ((cmd2=='e') || (cmd2=='E')) {
                enable = 1;
            }
            else
               goto usage;

            index = simple_strtoul(argv[3], NULL, 10);

            if (index > 6) {
                printf("???");
                goto usage;
            }

            shift = 6 - index;
            
            addr = SYSPLD_RESET_REG_2;

            if (syspld_reg_read(addr, &value))
                printf("SYSPLD addr 0x%02x read failed.\n", addr);
            else {
                if (enable) {
                    if (index > 4) 
                        value |= 1<<shift;
                    else
                        value &= ~(1<<shift);
                }
                else {
                    if (index > 4) 
                        value &= ~(1<<shift);
                    else
                        value |= 1<<shift;
                }

                if (syspld_reg_write(addr, value))
                    printf("SYSPLD write addr 0x%02x value 0x%02x failed.\n",
                            addr, value);
                else {
                    if (syspld_reg_read(addr, &value))
                        printf("SYSPLD addr 0x%02x read back failed.\n", addr);
                    else
                        printf("SYSPLD read back addr 0x%02x = 0x%02x.\n",
                                addr, value);
                }
            }
            
            break;

        case 'l':       /* LED */
        case 'L':
            
            if ((cmd2 == 'e') || (cmd2 == 'E')) {/* enasys/alm LED */
                ledUpdateEna = 1;
                break;
            }
            else if ((cmd2 == 'd') || (cmd2 == 'D')) {/* dissys/alm LED */
                ledUpdateEna = 0;
                break;
            }
            else if ((cmd2 == 'u') || (cmd2 == 'U')) {/* update sys/alm LED */
                update_led();
                return (1);
            }
            
            if (argc <= 5)
                goto usage;

            if ((cmd2 == 'p') || (cmd2 == 'P')) { /* Port */
                port = simple_strtoul(argv[3], NULL, 10);
                if ((argv[4][0] == 'u') ||(argv[4][0] == 'U'))
                    link = LINK_UP;
                else if ((argv[4][0] == 'd') ||(argv[4][0] == 'D'))
                    link = LINK_DOWN;
                else
                    goto usage;
                
                if ((argv[5][0] == 'h') ||(argv[5][0] == 'H'))
                    duplex = HALF_DUPLEX;
                else if ((argv[5][0] == 'f') ||(argv[5][0] == 'F'))
                    duplex = FULL_DUPLEX;
                else
                    goto usage;
                
                if (argv[6][0] != '1')
                    goto usage;
                
                if ((argv[6][1] == 'g') ||(argv[6][1] == 'G'))
                    speed = SPEED_1G;
                else if (argv[6][1] == '0') {
                    if ((argv[6][2] == 'm') ||(argv[6][2] == 'M'))
                        speed = SPEED_10M;
                    else if ((argv[6][2] == '0') && ((argv[6][3] == 'm') ||
                                (argv[6][3] == 'M')))
                        speed = SPEED_100M;
                    else
                        goto usage;
                }
                else
                    goto usage;

                set_port_led(port, link, duplex, speed);
            }
            else if ((cmd2 == 'm') || (cmd2 == 'M')) { /* Mgmt */
                if ((argv[3][0] == 'u') ||(argv[3][0] == 'U'))
                    link = LINK_UP;
                else if ((argv[3][0] == 'd') ||(argv[3][0] == 'D'))
                    link = LINK_DOWN;
                else
                    goto usage;
                
                if ((argv[4][0] == 'h') ||(argv[4][0] == 'H'))
                    duplex = HALF_DUPLEX;
                else if ((argv[4][0] == 'f') ||(argv[4][0] == 'F'))
                    duplex = FULL_DUPLEX;
                else
                    goto usage;
                
                if (argv[5][0] != '1')
                    goto usage;
                
                if (argv[5][1] == '0') {
                    if ((argv[5][2] == 'm') ||(argv[5][2] == 'M'))
                        speed = SPEED_10M;
                    else if ((argv[5][2] == '0') && ((argv[5][3] == 'm') ||
                                (argv[6][3] == 'M')))
                        speed = SPEED_100M;
                    else
                        goto usage;
                }
                else
                    goto usage;

                set_mgmt_led(link, duplex, speed);
            }
            else if ((cmd2 == 's') || (cmd2 == 'S')) { /* set */
                if ((argv[3][0] == 's') ||(argv[3][0] == 'S'))
                    led = SYS_LED;
                else if ((argv[3][0] == 'a') ||(argv[3][0] == 'A'))
                    led = ALM_LED;
                else
                    goto usage;
                
                if ((argv[4][0] == 'o') ||(argv[4][0] == 'O'))
                    color = LED_OFF;
                else if ((argv[4][0] == 'r') ||(argv[4][0] == 'R'))
                    color = LED_RED;
                else if ((argv[4][0] == 'g') ||(argv[4][0] == 'G'))
                    color = LED_GREEN;
                else
                    goto usage;
                
                if ((argv[5][0] == 's') ||(argv[5][0] == 'S'))
                    if ((argv[5][1] == 't') ||(argv[5][1] == 'T'))
                        mode = LED_STEADY;
                    else if ((argv[5][1] == 'l') ||(argv[5][1] == 'L'))
                        mode = LED_SLOW_BLINKING;
                    else
                        goto usage;
                else if ((argv[5][0] == 'f') ||(argv[5][0] == 'F'))
                    mode = LED_FAST_BLINKING;
                else
                    goto usage;
                
                set_led(led, color, mode);
            }
            else 
                goto usage;

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
#endif

#define CPU_TIMER_CONTROL			0x20300
#define WATCHDOG_ENABLE_BIT        BIT4
#define CPU_WATCHDOG_RELOAD			0x20320
#define CPU_WATCHDOG_TIMER			0x20324
void 
cpu_watchdog_init (uint32_t ms)
{
    uint32_t temp = 0;

    temp = (mvTclkGet() / 1000) * ms;

    MV_REG_WRITE(CPU_WATCHDOG_RELOAD, temp);
    MV_REG_BIT_RESET( CPU_TIMER_CONTROL, WATCHDOG_ENABLE_BIT);
    MV_REG_BIT_RESET( CPU_AHB_MBUS_CAUSE_INT_REG, BIT3); /* no WD interrupt */
}

void 
cpu_watchdog_disable (void)
{
    MV_REG_BIT_RESET( CPU_RSTOUTN_MASK_REG, BIT1);
    MV_REG_BIT_RESET( CPU_TIMER_CONTROL, WATCHDOG_ENABLE_BIT);
}

void 
cpu_watchdog_enable (void)
{
    MV_REG_WRITE(CPU_WATCHDOG_TIMER, MV_REG_READ(CPU_WATCHDOG_RELOAD));
    MV_REG_BIT_SET( CPU_RSTOUTN_MASK_REG, BIT1);
    MV_REG_BIT_SET( CPU_TIMER_CONTROL, WATCHDOG_ENABLE_BIT);
}

void 
cpu_watchdog_reset (void)
{
    MV_REG_WRITE(CPU_WATCHDOG_TIMER, MV_REG_READ(CPU_WATCHDOG_RELOAD));
}

#if !defined(CONFIG_PRODUCTION)
void 
syspld_watchdog_enable (void)
{
    uint8_t temp = 0;

    if (syspld_reg_read(SYSPLD_WATCHDOG_ENABLE, &temp) == 0) {
        temp |= SYSPLD_WDOG_ENABLE;
        syspld_reg_write(SYSPLD_WATCHDOG_ENABLE, temp);
    }
}

void 
syspld_watchdog_disable (void)
{
    uint8_t temp = 0;

    if (syspld_reg_read(SYSPLD_WATCHDOG_ENABLE, &temp) == 0) {
        temp &= ~SYSPLD_WDOG_ENABLE;
        syspld_reg_write(SYSPLD_WATCHDOG_ENABLE, temp);
    }
}

void 
syspld_watchdog_petting (void)
{
    /* RGPP4 - offset 30 (clk), RGPP3 - offset 27 (data) */

    /* 1, 1 */
    MV_REG_BIT_SET( GPP_DATA_OUT_REG(0) ,(1 << 27));
    MV_REG_BIT_SET( GPP_DATA_OUT_REG(0) ,(1 << 30));
    mvGppTypeSet(0, 0xFFFFFFFF, ~0x480C20A0);
    /* 0, 0 */
    MV_REG_BIT_RESET( GPP_DATA_OUT_REG(0) ,(1 << 27));
    MV_REG_BIT_RESET( GPP_DATA_OUT_REG(0) ,(1 << 30));
    mvGppTypeSet(0, 0xFFFFFFFF, ~0x480C20A0);
    /* 1, 0 */
    MV_REG_BIT_SET( GPP_DATA_OUT_REG(0) ,(1 << 30));
    mvGppTypeSet(0, 0xFFFFFFFF, ~0x480C20A0);
    /* 0, 1 */
    MV_REG_BIT_RESET( GPP_DATA_OUT_REG(0) ,(1 << 30));
    MV_REG_BIT_SET( GPP_DATA_OUT_REG(0) ,(1 << 27));
    mvGppTypeSet(0, 0xFFFFFFFF, ~0x480C20A0);
    /* 1, 1 */
    MV_REG_BIT_SET( GPP_DATA_OUT_REG(0) ,(1 << 30));
    mvGppTypeSet(0, 0xFFFFFFFF, ~0x480C20A0);
    /* 0, 0 */
    MV_REG_BIT_RESET( GPP_DATA_OUT_REG(0) ,(1 << 27));
    MV_REG_BIT_RESET( GPP_DATA_OUT_REG(0) ,(1 << 30));
    mvGppTypeSet(0, 0xFFFFFFFF, ~0x480C20A0);
    /* 1, 0 */
    MV_REG_BIT_SET( GPP_DATA_OUT_REG(0) ,(1 << 30));
    mvGppTypeSet(0, 0xFFFFFFFF, ~0x480C20A0);
    /* 0, 0 */
    MV_REG_BIT_RESET( GPP_DATA_OUT_REG(0) ,(1 << 30));
    mvGppTypeSet(0, 0xFFFFFFFF, ~0x480C20A0);
    /* 1, 0 */
    MV_REG_BIT_SET( GPP_DATA_OUT_REG(0) ,(1 << 30));
    mvGppTypeSet(0, 0xFFFFFFFF, ~0x480C20A0);
    /* 0, 0 */
    MV_REG_BIT_RESET( GPP_DATA_OUT_REG(0) ,(1 << 30));
    mvGppTypeSet(0, 0xFFFFFFFF, ~0x480C20A0);
    /* 1, 0 */
    MV_REG_BIT_SET( GPP_DATA_OUT_REG(0) ,(1 << 30));
    mvGppTypeSet(0, 0xFFFFFFFF, ~0x480C20A0);
    /* 0, 1 */
    MV_REG_BIT_RESET( GPP_DATA_OUT_REG(0) ,(1 << 30));
    MV_REG_BIT_SET( GPP_DATA_OUT_REG(0) ,(1 << 27));
    mvGppTypeSet(0, 0xFFFFFFFF, ~0x480C20A0);
    /* 1, 1 */
    MV_REG_BIT_SET( GPP_DATA_OUT_REG(0) ,(1 << 30));
    mvGppTypeSet(0, 0xFFFFFFFF, ~0x480C20A0);
    /* 0, 0 */

    return;
}

int 
do_wd (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uint32_t value = 0;
    char cmd1, cmd2;
    char *cmd;

    if (argc < 3)
        goto usage;

    cmd = argv[1];

    if (!strncmp(cmd,"syspld", 3)) {
        cmd = argv[2];
        if (!strncmp(cmd,"enable", 2)) {
            syspld_watchdog_enable();
            return (1);
        }
        else if (!strncmp(cmd,"disable", 2)) {
            syspld_watchdog_disable();
            return (1);
        }
        else if (!strncmp(cmd,"toggle", 2)) {
            syspld_watchdog_petting();
            
            return (1);
        }
        else
            goto usage;
    }

    cmd1 = argv[1][0];
    cmd2 = argv[2][0];

    if ((argv[1][0] != 'c') && (argv[1][0] != 'C'))
        goto usage;

    if ((argv[1][2] == 'u') || (argv[1][2] == 'U')) {
        switch (cmd2) {
            case 'i':       /* init */
            case 'I':
                if (argc < 4)
                    goto usage;

                value = simple_strtoul(argv[3], NULL, 10);
                cpu_watchdog_init(value);

                break;
            
            case 'e':   /* enable */
            case 'E':
                cpu_watchdog_enable();
            
                break;
            
            case 'd':   /* disable */
            case 'D':
                cpu_watchdog_disable();
            
                break;
            
            case 'r':   /* reset */
            case 'R':
                cpu_watchdog_reset();
            
                break;
            
            default:
                printf("???");
                break;
        }
    }

    return (1);
 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return 1;
}

void 
mpp_dump (void)
{
    int i;

    printf("MPP:\n"); 
    for (i = 0; i < 6; i++) {
        printf("MP_Control[%d] = 0x%08x\n", 
            i, MV_REG_READ(mvCtrlMppRegGet(i)));
    }
}

void 
gpi_dump (void)
{
    int i;

    printf("GPI:\n"); 
    for (i = 0; i < 2; i++) {
        printf("GPI[%d] = 0x%08x\n", 
            i, MV_REG_READ(GPP_DATA_IN_REG(i)));
        printf("GPI_pol[%d] = 0x%08x\n", 
            i, MV_REG_READ(GPP_DATA_IN_POL_REG(i)));
    }
}

void 
gpo_dump (void)
{
    int i;

    printf("GPO:\n"); 
    for (i = 0; i < 2; i++) {
        printf("GPO[%d] = 0x%08x\n", 
            i, MV_REG_READ(GPP_DATA_OUT_REG(i)));
        printf("GPO_EN[%d] = 0x%08x\n", 
            i, MV_REG_READ(GPP_DATA_OUT_EN_REG(i)));
    }
}

int 
do_si (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int index = 0, reg_offset, bit_offset, bit_val;
    ulong mstart, mend, size, pattern[2];
    ulong i, j, mloop = 0;
    int pattern_read = 1, pattern_write = 1;
    volatile ulong *mem;
    uint32_t temp;
    char *cmd = "gpio";  /* default command */

    if (argc < 2)
        goto usage;

    cmd = argv[1];
    
    if (!strncmp(cmd,"mpp", 3)) { 
        mpp_dump(); 
    }
    else if (!strncmp(cmd,"gpi", 3)) { 
        gpi_dump(); 
    }
    else if (!strncmp(cmd,"gpo", 3)) {
        if (argc == 2) {
            gpo_dump();
        }
        else if (argc == 5) {
            cmd = argv[2];
            if (strncmp(cmd,"device", 2))
                goto usage;
            
            index = simple_strtoul(argv[4], NULL, 10);
            if ((index <= 0) || (index > 4))
                goto usage;

            reg_offset = 0;
            if (index == 1) {
                bit_offset = 5;
            }
            else if (index == 2) {
                bit_offset = 13;
            }
            else if (index == 3) {
                bit_offset = 18;
            }
            else {
                reg_offset = 1;
                bit_offset = 0;
            }

            cmd = argv[3];
            if (!strncmp(cmd,"reset", 2)) {
                bit_val = 0;
                if (index == 1)
                    bit_val = 1;
            }
            else if (!strncmp(cmd,"clear", 2)) {
                bit_val = 1;
                if (index == 1)
                    bit_val = 0;
            }
            else
                goto usage;

            if (bit_val) {
                MV_REG_BIT_SET(GPP_DATA_OUT_REG(reg_offset), 
                                (1 << bit_offset));
            }
            else {
                MV_REG_BIT_RESET(GPP_DATA_OUT_REG(reg_offset), 
                                (1 << bit_offset));
            }
            if (reg_offset == 0)
                mvGppTypeSet(0, 0xFFFFFFFF, ~0x480C20A0);
            else
                mvGppTypeSet(1, 0xFFFFFFFF, ~0x00000001);

        }
        else
            goto usage;
    }
    else if ((!strncmp(cmd,"pattern", 2)) ||
        (!strncmp(cmd,"pwrite", 2)) ||
        (!strncmp(cmd,"pread", 2))) {
            if (argc <= 5)
                goto usage;
    
            pattern[0] = simple_strtoul(argv[2], NULL, 16);
            pattern[1] = simple_strtoul(argv[3], NULL, 16);
            mstart = simple_strtoul(argv[4], NULL, 16);
            mend = simple_strtoul(argv[5], NULL, 16);
            mem = (ulong *) mstart;
            size = (mend - mstart + 1)  / sizeof (ulong);
            if (argc >= 7)
                mloop = simple_strtoul(argv[6], NULL, 10);

            if ((argv[1][1] == 'r') ||(argv[1][1] == 'R')) {
                printf("Pattern read: start = 0x%x, end = 0x%x, "
                        "pattern = 0x%x/0x%x, ", mstart, mend, pattern[0], 
                        pattern[1]);
                pattern_read = 1;
                pattern_write = 0;
            } 
            else if ((argv[1][1] == 'w') ||(argv[1][1] == 'W')) {
                printf("Pattern write: start = 0x%x, end = 0x%x, "
                        "pattern = 0x%x/0x%x, ", mstart, mend, pattern[0], 
                        pattern[1]);
                pattern_read = 0;
                pattern_write = 1;
            } 
            else {
                printf("Pattern read/write: start = 0x%x, end = 0x%x, "
                        "pattern = 0x%x/0x%x, ", mstart, mend, pattern[0],
                        pattern[1]);
                pattern_read = 1;
                pattern_write = 1;
            }
            
            if (mloop) {
                printf("loop = %d\n", mloop);
                for (i = 0; i < mloop; i++) {
                    for (j = 0; j < size; j = j+8) {
                        if (pattern_write) {
                            mem[j] = pattern[0];
                            mem[j+1] = pattern[0];
                            mem[j+2] = pattern[1];
                            mem[j+3] = pattern[1];
                        }
                        if (pattern_read) {
                            temp = mem[j];
                            temp = mem[j+1];
                            temp = mem[j+2];
                            temp = mem[j+3];
                        }
                    }
                    /* swap pattern */
                    if (pattern_write && pattern_read) {
                        temp = pattern[0];
                        pattern[0] = pattern[1];
                        pattern[1] = temp;
                    }
                }
            }
            else {
                printf("loop forever\n");
                for (;;) {
                    for (i = 0; i < size; i = i+8) {
                        if (pattern_write) {
                            mem[i] = pattern[0];
                            mem[i+1] = pattern[0];
                            mem[i+2] = pattern[1];
                            mem[i+3] = pattern[1];
                        }
                        if (pattern_read) {
                            temp = mem[i];
                            temp = mem[i+1];
                            temp = mem[i+2];
                            temp = mem[i+3];
                        }
                    }
                    if (pattern_write && pattern_read) {
                        /* swap pattern */
                        temp = pattern[0];
                        pattern[0] = pattern[1];
                        pattern[1] = temp;
                    }
                }
            }
        
            printf("Pattern test done.\n");
    }
    else 
        goto usage;
    
    return (1);
 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return (1);
}

#endif

void 
rtc_init (void)
{
    uint8_t temp[16];
    uint8_t i2cAddr = I2C_RTC_ADDR;

    if (i2c_mux(I2C_MAIN_MUX_ADDR, 2, 1)) {
        printf("i2c failed to enable mux 0x%x channel 2.\n", 
                I2C_MAIN_MUX_ADDR);
    }
    else {
        if (i2c_read(0, i2cAddr, 0, 1, temp, 16)) {
            printf ("I2C read from device 0x%x failed.\n", i2cAddr);
            return;
        }

        if (temp[2] & 0x80) {
            temp[2] &= (~0x80);
                i2c_write(0, i2cAddr, 2, 1, &temp[2], 1);
        }
        temp[0] = 0x20;
        temp[1] = 0;
        temp[9] = temp[10] = temp[11] = temp[12] = 0x80;
        temp[13] = 0;
        temp[14] = 0;
        i2c_write(0, i2cAddr, 0, 1, temp, 16);
        temp[0] = 0;
        i2c_write(0, i2cAddr, 0, 1, temp, 1);
    }
    i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);  /* disable mux */
}

void 
rtc_start (void)
{
    uint8_t temp[2];
    uint8_t i2cAddr = I2C_RTC_ADDR;

    if (i2c_mux(I2C_MAIN_MUX_ADDR, 2, 1)) {
        printf("i2c failed to enable mux 0x%x channel 2.\n", 
                I2C_MAIN_MUX_ADDR);
    }
    else {
        temp[0] = 0;
        i2c_write(0, i2cAddr, 0, 1, temp, 1);
    }
    i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);  /* disable mux */
}

void 
rtc_stop (void)
{
    uint8_t temp[2];
    uint8_t i2cAddr = I2C_RTC_ADDR;

    if (i2c_mux(I2C_MAIN_MUX_ADDR, 2, 1)) {
        printf("i2c failed to enable mux 0x%x channel 2.\n", 
                I2C_MAIN_MUX_ADDR);
    }
    else {
        temp[0] = 0x20;
        i2c_write(0, i2cAddr, 0, 1, temp, 1);
    }
    i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);  /* disable mux */
}

void 
rtc_reg_dump (void)
{
    uint8_t temp[20];
    uint8_t i2cAddr = I2C_RTC_ADDR;
    int i;

    if (i2c_mux(I2C_MAIN_MUX_ADDR, 2, 1)) {
        printf("i2c failed to enable mux 0x%x channel 2.\n", 
                I2C_MAIN_MUX_ADDR);
    }
    else {
            if (i2c_read(0, i2cAddr, 0, 1, temp, 16)) {
                printf ("I2C read from device 0x%x failed.\n", i2cAddr);
            }
            else {
                printf("RTC register dump:\n");

                for (i = 0; i < 16; i++)
                    printf("%02x ", temp[i]);
                printf("\n");
        }
    }
    i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);  /* disable mux */
}

void 
rtc_get_status (void)
{
    uint8_t temp[20];
    uint8_t i2cAddr = I2C_RTC_ADDR;

    if (i2c_mux(I2C_MAIN_MUX_ADDR, 2, 1)) {
        printf("i2c failed to enable mux 0x%x channel 2.\n", 
                I2C_MAIN_MUX_ADDR);
    }
    else {
            if (i2c_read(0, i2cAddr, 0, 1, temp, 16)) {
                printf ("I2C read from device 0x%x failed.\n", i2cAddr);
            }
            else {
                printf("RTC status:\n");
                printf("Reg 0x0 (0x%02x) STOP = %d\n", 
                        temp[0], (temp[0] & 0x20) ? 1 : 0);
                printf("Reg 0x1 (0x%02x) TI/TP = %d, AF = %d, TF = %d, "
                        "AIE = %d, TIE = %d\n", 
                    temp[1], 
                    (temp[1] & 0x10) ? 1 : 0, 
                    (temp[1] & 0x08) ? 1 : 0,
                    (temp[1] & 0x04) ? 1 : 0,
                    (temp[1] & 0x02) ? 1 : 0,
                    (temp[1] & 0x01) ? 1 : 0
                    );
                printf("Reg 0x2 (0x%02x) VL = %d\n", 
                    temp[2], 
                    (temp[2] & 0x80) ? 1 : 0
                    );
                printf("Reg 0x7 (0x%02x) C = %d\n", 
                    temp[7], 
                    (temp[7] & 0x80) ? 1 : 0
                    );
                printf("Reg 0x9 (0x%02x) AE = %d\n", 
                    temp[9], 
                    (temp[9] & 0x80) ? 1 : 0
                    );
                printf("Reg 0xA (0x%02x) AE = %d\n", 
                    temp[0xA], 
                    (temp[0xA] & 0x80) ? 1 : 0
                    );
                printf("Reg 0xB (0x%02x) AE = %d\n", 
                    temp[0xB], 
                    (temp[0xB] & 0x80) ? 1 : 0
                    );
                printf("Reg 0xC (0x%02x) AE = %d\n", 
                    temp[0xC], 
                    (temp[0xC] & 0x80) ? 1 : 0
                    );
                printf("Reg 0xD (0x%02x) FE = %d, FD1 = %d, FD0 = %d\n", 
                    temp[0xD], 
                    (temp[0xD] & 0x80) ? 1 : 0, 
                    (temp[0xD] & 0x02) ? 1 : 0, 
                    (temp[0xD] & 0x01) ? 1 : 0
                    );
                printf("Reg 0xE (0x%02x) TE = %d, TD1 = %d, TD0 = %d\n", 
                    temp[0xE], 
                    (temp[0xE] & 0x80) ? 1 : 0, 
                    (temp[0xE] & 0x02) ? 1 : 0, 
                    (temp[0xE] & 0x01) ? 1 : 0
                    );

        }
    }
    i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);  /* disable mux */
}

void 
rtc_get_time (void)
{
    uint8_t temp[9];
    uint8_t i2cAddr = I2C_RTC_ADDR;

    if (i2c_mux(I2C_MAIN_MUX_ADDR, 2, 1)) {
        printf("i2c failed to enable mux 0x%x channel 2.\n", 
                I2C_MAIN_MUX_ADDR);
    }
    else {
        if (i2c_read(0, i2cAddr, 0, 1, temp, 9)) {
            printf ("I2C read from device 0x%x failed.\n", i2cAddr);
        }
        else {
            printf("RTC time:  ");
            printf("%d%d/", (temp[8]&0xf0) >> 4, temp[8]&0xf);
            printf("%d%d/", (temp[7]&0x10) >> 4, temp[7]&0xf);
            printf("%d%d ", (temp[5]&0x30) >> 4, temp[5]&0xf);
            printf("%d%d:", (temp[4] & 0x30) >> 4, temp[4]&0xf);
            printf("%d%d:", (temp[3] & 0x70) >> 4, temp[3]&0xf);
            printf("%d%d\n", (temp[2] & 0x70) >> 4, temp[2]&0xf);
        }    
    }
    i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);  /* disable mux */
}

void 
rtc_set_time (int yy, int mm, int dd, int hh, int mi, int ss)
{
    uint8_t temp[9];
    uint8_t i2cAddr = I2C_RTC_ADDR;

    if (i2c_mux(I2C_MAIN_MUX_ADDR, 2, 1)) {
        printf("i2c failed to enable mux 0x%x channel 2.\n", 
                I2C_MAIN_MUX_ADDR);
    }
    else {
        i2c_read(0, i2cAddr, 0, 1, temp, 9);
        temp[8] = ((yy/10) << 4) + (yy%10);
        temp[7] &= 0x80;
        temp[7] = ((mm/10) << 4) + (mm%10);
        temp[5] = ((dd/10) << 4) + (dd%10);
        temp[4] = ((hh/10) << 4) + (hh%10);
        temp[3] = ((mi/10) << 4) + (mi%10);
        temp[2] = ((ss/10) << 4) + (ss%10);
        temp[6] = 0; /* weekday? */
        i2c_write(0, i2cAddr, 0, 1, temp, 9);
    }
    i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);  /* disable mux */
}

#if !defined(CONFIG_PRODUCTION)
int 
do_rtc (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char *data;
    char tbyte[3];
    int i, len, temp, time1[3], time2[3];
    char *cmd = "status";  /* default command */

    if (argc == 1) {
        if (!strncmp(argv[0],"rtc", 3)) {
            rtc_get_time();
        }
        else
            goto usage;
    }
    else if (argc == 2) {
        cmd = argv[1];
        if (!strncmp(cmd,"reset", 3)) {
            rtc_set_time(0, 0, 0, 0, 0, 0);
        }
        else if (!strncmp(cmd,"init", 4)) {
            rtc_init();
        }
        else if (!strncmp(cmd,"start", 4)) {
            rtc_start();
        }
        else if (!strncmp(cmd,"stop", 3)) {
            rtc_stop();
        }
        else if (!strncmp(cmd,"status", 4)) {
            rtc_get_status();
        }
        else if (!strncmp(cmd,"dump", 4)) {
            rtc_reg_dump();
        }
        else
            goto usage;
    }
    else if (argc == 4) {
        cmd = argv[1];
        if (!strncmp(cmd,"set", 3)) {
            data = argv [2];
            len = strlen(data)/2;
            tbyte[2] = 0;
            for (i = 0; i < len; i++) {
                tbyte[0] = data[2*i];
                tbyte[1] = data[2*i+1];
                temp = atod(tbyte);
                time1[i] = temp;
            }
            data = argv [3];
            len = strlen(data)/2;
            for (i = 0; i < len; i++) {
                tbyte[0] = data[2*i];
                tbyte[1] = data[2*i+1];
                temp = atod(tbyte);
                time2[i] = temp;
            }
            rtc_set_time(time1[0], time1[1], time1[2], time2[0], 
                            time2[1], time2[2]);
        }
        else
            goto usage;
    }
    else 
        goto usage;
    
    return (1);
 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return (1);
}
#endif

#define MAX_PACKET_LENGTH 1518

int eth0_debug_flag = 1;
char packet_send[MAX_PACKET_LENGTH];
char packet_recv[MAX_PACKET_LENGTH];

typedef struct _phy_mii_t {
    uint8_t reg;
    uint16_t page;
    char * name;
} phy_mii_t;

phy_mii_t phy_mii_table[] = {
    { 0, 0, "control" },
    { 1, 0, "status" },
    { 2, 0, "PHY ID 1" },
    { 3, 0, "PHY ID 2" },
    { 4, 0, "auto adv base" },
    { 5, 0, "auto partner ab" },
    { 6, 0, "auto adv ex" },
    { 7, 0, "auto next" },
    { 8, 0, "auto partner next" },
    { 16, 0, "specific control" },
    { 17, 0, "specific status" },
    { 18, 0, "int enable" },
    { 19, 0, "int status" },
    { 20, 0, "int port summary" },
    { 21, 0, "rx err counter" },
    { 22, 0, "LED para sel" },
    { 24, 0, "LED control" },
    { 25, 0, "LED override" },
    { 26, 0, "VCT MDIP/n[0]" },
    { 27, 0, "VCT MDIP/n[1]" },
    { 28, 0, "specific control 2" },
    { 29, 0, "test mode sel" },
    { 30, 9, "CRC status" },
    { 30, 10, "MAC imp override" },
    { 30, 11, "MAC imp target" }
};

void 
eth0_phy_dump (void)
{
    int i;
    uint16_t temp, temp1;

    printf("Mgmt ethernet phy register dump:\n");
    for (i = 0; i < sizeof(phy_mii_table)/sizeof(phy_mii_table[0]); i++) {
        if (phy_mii_table[i].reg == 30) {
            mvEthPhyRegRead(1, 29, &temp1);
            temp1 &= 0xffe0;
            temp1 |= phy_mii_table[i].page;
            mvEthPhyRegWrite(1, 29, temp1);
            mvEthPhyRegRead(1, 30, &temp);
            printf("%2d_%02d %-20s  %04x\n", phy_mii_table[i].reg, 
                phy_mii_table[i].page, phy_mii_table[i].name, temp);
        }
        else {
            mvEthPhyRegRead(1, phy_mii_table[i].reg, &temp);
            printf("%2d    %-20s  %04x\n", phy_mii_table[i].reg, 
                    phy_mii_table[i].name, temp);
        }
    }    
}

static void 
packet_fill (char *packet, int length)
{
	char c = (char) length;
	int i;

	packet[0] = 0xFF;
	packet[1] = 0xFF;
	packet[2] = 0xFF;
	packet[3] = 0xFF;
	packet[4] = 0xFF;
	packet[5] = 0xFF;

	for (i = 6; i < length; i++) {
		packet[i] = c++;
	}
}

static int 
packet_check (char *packet, int length)
{
    char c = (char) length-2; /* do not include checksum */
    int i;
    
    for (i = 6; i < length-2; i++) { 
        if (packet[i] != c++) {
            if (eth0_debug_flag) {
                printf("Packet check contents are...\n");
                for (i = 0 ; i < length; i++) {
                    if ((i % 16) == 0) printf("\n");
                    printf("%02x ", packet[i]);
                }
                printf("\n");
            }
            return (-1);
        }
    }
    return (0);
}

void 
eth0_phy_loopback (int enable)
{
    
    if (enable) {
        mvEthPhyRegWrite(1, 0, 0xa100);
        mvEthPhyRegWrite(1, 0, 0x6100);
        mvEthPhyRegWrite(1, 29, 0x2);
        mvEthPhyRegWrite(1, 30, 0x876f);
    }
    else {
        mvEthPhyRegWrite(1, 29, 0x2);
        mvEthPhyRegWrite(1, 30, 0);
        mvEthPhyRegWrite(1, 0, 0xb100);
    }
}

void 
eth0_ext_loopback (int enable, int sp100)
{
    uint16_t regVal = 0;
    
    if (enable) {
        if (sp100) {
            regVal = 0x2100;
        }
        else {
            regVal = 0x100;
        }
    }
    else
        regVal = 0x3100;
    
    mvEthPhyRegWrite(1, 0, 0x8000 | regVal);
}

void 
eth0_mac_loopback (int enable)
{
    uint32_t regVal;

    regVal = MV_REG_READ(ETH_PORT_SERIAL_CTRL_1_REG(0));
    if (enable) {
        eth0_ext_loopback(1, 1);
        regVal |= 0x1;
    }
    else {
        eth0_ext_loopback(0, 1);
        regVal &= ~0x1;
    }
    MV_REG_WRITE(ETH_PORT_SERIAL_CTRL_1_REG(0), regVal);
}


int 
eth0_mac_link (void)
{
    uint32_t regVal;
    int i, res = -1;

    for (i = 0; i < 100; i++) {
        regVal = MV_REG_READ(ETH_PORT_STATUS_REG(0));
        if (regVal & 0x2) {
            res = 0;
            break;
        }
        udelay(1000);
    }
    return res;
}

int 
eth0_ext_link (void)
{
    uint16_t regVal;
    int i, res = -1;

    for (i = 0; i < 150; i++) {
        mvEthPhyRegRead(1, 1, &regVal);
        if (regVal & 0x4) {
            res = 0;
            break;
        }
        udelay(10000);
    }
    return res;
}

void 
dump_loopback_setup (void)
{
    uint32_t mac43c, mac444, mac44c;
    uint16_t phy0, phy17;

    mac43c = MV_REG_READ(ETH_PORT_SERIAL_CTRL_REG(0));
    mac444 = MV_REG_READ(ETH_PORT_STATUS_REG(0));
    mac44c = MV_REG_READ(ETH_PORT_SERIAL_CTRL_1_REG(0));

    mvEthPhyRegRead(1, 0, &phy0);
    mvEthPhyRegRead(1, 17, &phy17);

    printf("\nPHY_0 = 0x%04x, PHY_17 = 0x%04x\n", phy0, phy17);
    printf("MAC_43C = 0x%08x, MAC_444 = 0x%08x, MAC_44C = 0x%08x\n", 
            mac43c, mac444, mac44c);
}

int 
eth0_loopback (int loop_type, int len)
{
    int res = -1, len_rx, init_done = 0;
    bd_t *bd = gd->bd;

    if (len > MAX_PACKET_LENGTH)
        len = MAX_PACKET_LENGTH;

    if (loop_type == 0) { eth0_mac_loopback(1);}
    else if (loop_type == 1) { eth0_phy_loopback(1); }
    else if (loop_type == 2) { eth0_ext_loopback(1, 0); }
    else if (loop_type == 3) { eth0_ext_loopback(1, 1); }
    else
        return res;

    packet_fill (packet_send, len);
    eth_halt();
    eth_set_current();
    init_done = eth_init(bd);
    
    if (init_done) {
        if (loop_type == 0) { }
        else if (loop_type == 1) {  res = eth0_mac_link(); }
        else if ((loop_type == 2) ||(loop_type == 3)) { 
            res = eth0_ext_link(); 
        }
    }

    if ((init_done) && (!res)) {
        eth_send(packet_send, len);
        udelay(5000);
        if ((len_rx = eth_receive(packet_recv, MAX_PACKET_LENGTH)) != -1) {
            res = packet_check(packet_recv, len_rx);
        }
        else
            res = len_rx;
    }
    else
        res = -1;
    eth_halt();
    if (loop_type == 0) { eth0_mac_loopback(0); }
    else if (loop_type == 1) { eth0_phy_loopback(0); }
    else if (loop_type == 2) { eth0_ext_loopback(0, 0); }
    else if (loop_type == 3) { eth0_ext_loopback(0, 1); }

    return res;
}

/* 
 * ehto commands
 */
int 
do_eth0 (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char cmd1, cmd2;
    uint16_t regno, regvalue;
    int i, count, size, speed = 0, looptype = 0, regindex = -1;
    
    if (argc <= 2)
        goto usage;
    
    cmd1 = argv[1][0];
    cmd2 = argv[2][0];

    if ((cmd1 == 'p') || (cmd1 == 'P')) {
        switch (cmd2) {
            case 'd':
            case 'D':  /* dump */
                eth0_phy_dump();
                break;
                
            case 'r':
            case 'R':  /* read */
                if (argc <= 3)
                    goto usage;
                
                regno = simple_strtoul(argv[3], NULL, 10);
                for (i = 0; i < sizeof(phy_mii_table) / sizeof(phy_mii_table[0]);
                        i++) {
                    if (regno == phy_mii_table[i].reg)
                        regindex = i;
                }
                if (regindex < 0) {
                    printf("phy register %d not valid.\n", regno);
                    return (1);
                }
                mvEthPhyRegRead(1, regno, &regvalue);
                printf("    %d  %-10s 0x%x\n", regno, 
                        phy_mii_table[regindex].name, regvalue);
                break;
                
            case 'w':
            case 'W':  /* write */
                if (argc <= 4)
                    goto usage;
                
                regno = simple_strtoul(argv[3], NULL, 10);
                regvalue = simple_strtoul(argv[4], NULL, 16);
                for (i = 0; i < sizeof(phy_mii_table) / sizeof(phy_mii_table[0]);
                        i++) {
                    if (regno == phy_mii_table[i].reg)
                        regindex = i;
                }
                if (regindex < 0) {
                    printf("phy register %d not valid.\n", regno);
                    return (1);
                }
                mvEthPhyRegWrite(1, regno, regvalue);
                break;
                
            default:
                break;
        }
    }
    else if ((cmd1 == 'l') || (cmd1 == 'L')) {
        if ((cmd2 == 'm') || (cmd2 == 'M')) { /* MAC */
            if (argc < 4)
                goto usage;
            size = simple_strtoul(argv[3], NULL, 10);
            count = simple_strtoul(argv[4], NULL, 10);
            looptype = 0;
        }
        else if ((cmd2 == 'p') || (cmd2 == 'P')) { /* PHY */
            if (argc < 4)
                goto usage;
            size = simple_strtoul(argv[3], NULL, 10);
            count = simple_strtoul(argv[4], NULL, 10);
            looptype = 1;
        }
        else if ((cmd2 == 'e') || (cmd2 == 'E')) { /* EXT */
            if (argc < 5)
                goto usage;
            speed = simple_strtoul(argv[3], NULL, 10);
            size = simple_strtoul(argv[4], NULL, 10);
            count = simple_strtoul(argv[5], NULL, 10);

            if (speed == 10) {
                looptype = 2;
            }
            else if (speed == 100) {
                looptype = 3;
            }
            else
                goto usage;
        }
        else
            goto usage;

        for (i = 0; ((i < count) || (count == 0)); i++) {
            if (ctrlc()) {
                putc ('\n');
                return (0);
            }

            if ((looptype == 2) || (looptype == 3))
                printf("eth0 %s %s loopback: %d   ", argv[2], argv[3], i+1);
            else
                printf("eth0 %s loopback: %d   ", argv[2], i+1);

            if (eth0_loopback(looptype, size) == 0) {
                printf(" passed.");
            }
            else {
                printf(" failed.");
            }
            printf("\n");
        }
    }
    else
        goto usage;

    return (1);
 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return (1);
}

/*---------------------------------------------*/

/* Show information about devices on PCIe bus. */

void 
pcieinfo (int controller, int busnum, int short_listing)
{
    unsigned int device = 0, function = 0;
    unsigned char header_type;
    unsigned short vendor_id;
    pcie_dev_t dev;
    
    printf("\nScanning PCIE[%d] devices on bus %d\n", controller, busnum);
    
    if (short_listing) {
    printf("BusDevFun  VendorId   DeviceId   Device Class     Sub-Class\n");
    printf("-----------------------------------------------------------\n");    
    }

    for (device = 0; device < PCIE_MAX_PCIE_DEVICES; device++) {

        header_type = 0;
        vendor_id = 0;

        for (function = 0; function < PCIE_MAX_PCIE_FUNCTIONS; function++) {
          
        /* If this is not a multi-function device, we skip the rest. */
              if (function && !(header_type & 0x80))
                   break;

           dev = PCIE_BDF(busnum, device, function);
    
               pcie_hose_read_config_word(&pcie_hose[controller-1], dev, 
                        PCIE_VENDOR_ID, &vendor_id);
        if ((vendor_id == 0xFFFF) || (vendor_id == 0x0000))
        continue;
    
        pcie_hose_read_config_byte(&pcie_hose[controller-1], dev, 
                    PCIE_HEADER_TYPE, &header_type);
        
        if (short_listing) {
            printf("%02x.%02x.%02x   ",busnum, device, function);
            pcie_header_show_brief(controller, dev);
        }
        else {
            printf("\nFound PCIE[%d] device %02x.%02x.%02x:\n",
                    controller, busnum, device,function); 
            pcie_header_show(controller, dev);
        }
       }
    }
}

void 
pciedump (int controller, int busnum)
{
    pcie_dev_t dev;
    unsigned char buff[256],temp[20];
    int addr,offset = 0;
    int linebytes;
    int nbytes = 256;
    int i, j = 0;

    printf("\nScanning PCIE[%d] devices on bus %d\n", controller, busnum);

    dev = PCIE_BDF(busnum, 0, 0);
    memset(buff,0xff,256);

    for (addr = 0 ; addr<265 ; addr++) 
    pcie_hose_read_config_byte(&pcie_hose[controller-1], dev, addr,
            (buff+addr));

    do {
    printf("0x%02x :",offset);
    linebytes = (nbytes < 16) ? nbytes : 16;

    for (i = 0; i < linebytes; i++) {
        if ((i == 4) || (i == 8)|| (i == 12))
            printf(" ");

        temp[i] = buff[j];
        printf(" %02x",buff[j]);
        j++;
    }
    printf("  ");

    for (i = 0; i < linebytes; i++) {
        if ((i == 4) || (i == 8)|| (i == 12))
        printf(" ");
        
        if ((temp[i] < 0x20) || (temp[i] > 0x7e)) 
        putc('.');
        else
        putc(temp[i]);
    }
    printf("\n");

    offset = offset + 16;
    nbytes = nbytes - linebytes;
    }while (nbytes);

    printf("\n");
}

static char *
pcie_classes_str (u8 class)
{
    switch (class) {
    case PCIE_CLASS_NOT_DEFINED:
        return "Build before PCIE Rev2.0";
        break;
    case PCIE_BASE_CLASS_STORAGE:
        return "Mass storage controller";
        break;
    case PCIE_BASE_CLASS_NETWORK:
        return "Network controller";
        break;
    case PCIE_BASE_CLASS_DISPLAY:
        return "Display controller";
        break;
    case PCIE_BASE_CLASS_MULTIMEDIA:
        return "Multimedia device";
        break;
    case PCIE_BASE_CLASS_MEMORY:
        return "Memory controller";
        break;
    case PCIE_BASE_CLASS_BRIDGE:
        return "Bridge device";
        break;
    case PCIE_BASE_CLASS_COMMUNICATION:
        return "Simple comm. controller";
        break;
    case PCIE_BASE_CLASS_SYSTEM:
        return "Base system peripheral";
        break;
    case PCIE_BASE_CLASS_INPUT:
        return "Input device";
        break;
    case PCIE_BASE_CLASS_DOCKING:
        return "Docking station";
        break;
    case PCIE_BASE_CLASS_PROCESSOR:
        return "Processor";
        break;
    case PCIE_BASE_CLASS_SERIAL:
        return "Serial bus controller";
        break;
    case PCIE_BASE_CLASS_INTELLIGENT:
        return "Intelligent controller";
        break;
    case PCIE_BASE_CLASS_SATELLITE:
        return "Satellite controller";
        break;
    case PCIE_BASE_CLASS_CRYPT:
        return "Cryptographic device";
        break;
    case PCIE_BASE_CLASS_SIGNAL_PROCESSING:
        return "DSP";
        break;
    case PCIE_CLASS_OTHERS:
        return "Does not fit any class";
        break;
    default:
    return  "???";
        break;
    };
}


/* Reads and prints the header of the specified PCIe device in short form 
 * Inputs: dev  bus+device+function number
 */
    
void 
pcie_header_show_brief (int controller, pcie_dev_t dev)
{
    unsigned short vendor, device;
    unsigned char class, subclass;

    pcie_hose_read_config_word(&pcie_hose[controller-1], dev, 
                PCIE_VENDOR_ID, &vendor);
    pcie_hose_read_config_word(&pcie_hose[controller-1], dev, 
                PCIE_VENDOR_ID, &vendor);
    pcie_hose_read_config_word(&pcie_hose[controller-1], dev, 
                PCIE_DEVICE_ID, &device);
    pcie_hose_read_config_byte(&pcie_hose[controller-1], dev, 
                PCIE_CLASS_CODE, &class);
    pcie_hose_read_config_byte(&pcie_hose[controller-1], dev, 
                PCIE_CLASS_SUB_CODE, &subclass);
                
    printf("0x%.4x     0x%.4x     %-23s 0x%.2x\n", 
       vendor, device, pcie_classes_str(class), subclass);
}

/* Reads the header of the specified PCIe device
 * Inputs: dev  bus+device+function number
 */

void 
pcie_header_show (int controller, pcie_dev_t dev)
{
    unsigned char _byte, header_type;
    unsigned short _word;
    unsigned int _dword;
        
#define PRINT(msg, type, reg) \
        pcie_hose_read_config_##type(&pcie_hose[controller-1], dev, reg, &_##type); \
        printf(msg, _##type)

    pcie_hose_read_config_byte(&pcie_hose[controller-1], dev, 
            PCIE_HEADER_TYPE, &header_type);
    PRINT ("vendor ID = 0x%.4x\n", word, PCIE_VENDOR_ID);
    PRINT ("device ID = 0x%.4x\n", word, PCIE_DEVICE_ID);
    PRINT ("command register = 0x%.4x\n", word, PCIE_COMMAND);
    PRINT ("status register = 0x%.4x\n", word, PCIE_STATUS);
    PRINT ("revision ID = 0x%.2x\n", byte, PCIE_REVISION_ID);
    PRINT ("sub class code = 0x%.2x\n", byte, PCIE_CLASS_SUB_CODE);
    PRINT ("programming interface = 0x%.2x\n", byte, PCIE_CLASS_PROG);
    PRINT ("cache line = 0x%.2x\n", byte, PCIE_CACHE_LINE_SIZE);
    PRINT ("header type = 0x%.2x\n", byte, PCIE_HEADER_TYPE);
    PRINT ("base address 0 = 0x%.8x\n", dword, PCIE_BASE_ADDRESS_0);

    switch (header_type & 0x03)
    {
    case PCIE_HEADER_TYPE_NORMAL:   /* "normal" PCIE device */
        PRINT ("base address 1 = 0x%.8x\n", dword, PCIE_BASE_ADDRESS_1);
        PRINT ("base address 2 = 0x%.8x\n", dword, PCIE_BASE_ADDRESS_2);
        PRINT ("base address 3 = 0x%.8x\n", dword, PCIE_BASE_ADDRESS_3);
        PRINT ("base address 4 = 0x%.8x\n", dword, PCIE_BASE_ADDRESS_4);
        PRINT ("base address 5 = 0x%.8x\n", dword, PCIE_BASE_ADDRESS_5);
        PRINT ("sub system vendor ID = 0x%.4x\n", word, 
                    PCIE_SUBSYSTEM_VENDOR_ID);
        PRINT ("sub system ID = 0x%.4x\n", word, PCIE_SUBSYSTEM_ID);
        PRINT ("interrupt line = 0x%.2x\n", byte, PCIE_INTERRUPT_LINE);
        PRINT ("interrupt pin = 0x%.2x\n", byte, PCIE_INTERRUPT_PIN);
        break;

    case PCIE_HEADER_TYPE_BRIDGE:   /* PCIE-to-PCIE bridge */
        PRINT ("base address 1 = 0x%.8x\n", dword, PCIE_BASE_ADDRESS_1);
        PRINT ("primary bus number = 0x%.2x\n", byte, PCIE_PRIMARY_BUS);
        PRINT ("secondary bus number = 0x%.2x\n", byte,PCIE_SECONDARY_BUS);
        PRINT ("subordinate bus number = 0x%.2x\n", byte, 
                    PCIE_SUBORDINATE_BUS);
        PRINT ("IO base = 0x%.2x\n", byte, PCIE_IO_BASE);
        PRINT ("IO limit = 0x%.2x\n", byte, PCIE_IO_LIMIT);
        PRINT ("memory base = 0x%.4x\n", word, PCIE_MEMORY_BASE);
        PRINT ("memory limit = 0x%.4x\n", word, PCIE_MEMORY_LIMIT);
        PRINT ("prefetch memory base = 0x%.4x\n", word, 
                    PCIE_PREF_MEMORY_BASE);
        PRINT ("prefetch memory limit = 0x%.4x\n", word, 
                    PCIE_PREF_MEMORY_LIMIT);
        PRINT ("interrupt line = 0x%.2x\n", byte, PCIE_INTERRUPT_LINE);
        PRINT ("interrupt pin =  0x%.2x\n", byte, PCIE_INTERRUPT_PIN);
        PRINT ("bridge control = 0x%.4x\n", word, PCIE_BRIDGE_CONTROL);
        break;
    
    default:
        printf("unknown header\n");
        break;
    }

#undef PRINT
}

static unsigned char 
pcieshortinfo (int controller, int busnum)
{
    pcie_dev_t     dev;
    unsigned short vendor_id;
    unsigned short device_id;
    unsigned char  header_type;
    unsigned char  class_code, subclass;
    unsigned char  linkstat;
    
    dev = PCIE_BDF(busnum, 0, 0);
    pcie_hose_read_config_word(&pcie_hose[controller-1], dev, 
                PCIE_VENDOR_ID     , &vendor_id  );
    pcie_hose_read_config_byte(&pcie_hose[controller-1], dev, 
                PCIE_HEADER_TYPE   , &header_type);
    pcie_hose_read_config_word(&pcie_hose[controller-1], dev, 
                PCIE_DEVICE_ID     , &device_id  );
    pcie_hose_read_config_byte(&pcie_hose[controller-1], dev, 
                PCIE_CLASS_CODE    , &class_code );
    pcie_hose_read_config_byte(&pcie_hose[controller-1], dev, 
                PCIE_CLASS_SUB_CODE, &subclass   );
    linkstat = MV_REG_READ(PEX_STATUS_REG(0)) & 0x1;
        
    printf ("%d\t" "%d\t" "0\t" "0\t" "0x%.4x\t" "0x%.4x\t", 
            controller, busnum, vendor_id, device_id);
    printf ("%-23s\t0x%.2x\t", pcie_classes_str(class_code), subclass);
    printf ("0x%2X", linkstat);
    printf ("\n");
    return linkstat;
}

static void 
do_pcie_status (void)

{   unsigned char ls1;
    printf("Ctrl\t" "Bus\t" "Dev\t" "Func\t" "Vendor\t" "Device\t" "%-23s\t" 
        "SubCls\t" "Link\n", "Class");
    ls1 = pcieshortinfo (1, 0);
    if (ls1 == 0x0) pcieshortinfo (1, 1);
    printf ("-\n");
}

static int 
stress_pcie_up (int controller, int retries, int CheetahNo)

{   int i;
    unsigned char ls;
    unsigned short vendor_id;
    unsigned short device_id;
    
    printf ("PCIE stresstest ctrl %d - Cheetah %d up   (%d retries) ... ", 
            controller, CheetahNo, retries);
    pcie_hose_read_config_word(&pcie_hose[controller-1], 
            PCIE_BDF(1, 0, 0), PCIE_VENDOR_ID     , &vendor_id  );
    pcie_hose_read_config_word(&pcie_hose[controller-1], 
            PCIE_BDF(1, 0, 0), PCIE_DEVICE_ID     , &device_id  );
    if (vendor_id != 0x11AB )
    {
        printf ("Invalid Vendor ID detected : %04X\n",vendor_id);
        return (1);
    }
    if (device_id != 0x6281 && device_id != 0xDDB6)
    {
        printf ("No CheetahJ detected - device id=%04X "
                "(expected 0x6281/0xDDB6)\n", device_id);
        return (1);
    }
    for (i = 0; i < retries; i++) 
    {
        ls = MV_REG_READ(PEX_STATUS_REG(0)) & 0x1;	
        if (ls != 0)
        {
            printf ("LinkStatus: 0x%02X after %5d retries, expected 0x0\n",
                    ls,i);
            return (1);
        }
    }
    printf ("OK\n");
    return (0);
}

static int 
stress_pcie_down (int controller, int retries)

{   int i;
    unsigned char ls;

    printf ("PCIE stresstest ctrl %d expect Link down (%d retries) ... ", 
            controller, retries);
    for (i = 0; i < retries; i++) 
    {
        ls = MV_REG_READ(PEX_STATUS_REG(0)) & 0x1;	
        if (ls != 0x00)
        {
            printf ("LinkStatus: 0x%02X after %5d retries (expected 0x00)\n",
                    ls,i);
            return (1);
        }
    }
    printf ("OK\n");
    return (0);
}

static void 
do_pcie_stress (void)

{   int err     = 0;
    int retries = 5000000;

    switch (board_idtype())
    {
        default: 
            printf ("Unsupported board type - can not stress test PCI-E bus!");
            err = 1;
            break;

        case I2C_ID_JUNIPER_EX2200_12_T: /* Leo 12 T */
        case I2C_ID_JUNIPER_EX2200_12_P: /* Leo 12 P */
	    err += stress_pcie_down (1,retries);
            break;

        CASE_ALL_I2C_ID_EX2200_24XX:
            err += stress_pcie_down (1,retries);
            break;

        CASE_ALL_I2C_ID_EX2200_48XX:
            err += stress_pcie_up   (1,retries, 1);
            break;

    }
    if (err)
        printf ("FAILED - PCIE stresstest with %d PCI errors\n", err);
    else
        printf ("PASSED - PCIE stresstest\n");
}

/* This routine services the "pcieprobe" command.
 * Syntax - pcieprobe [-v] [-d]
 *            -v for verbose, -d for dump.
 */

int 
do_pcie_probe (cmd_tbl_t *cmd, int flag, int argc, char *argv[])
{
    int controller = 1;  /* default PCIe 1 */
    unsigned char link;

    if (board_pci() == 0) {
        printf("No PCIE supported.\n");
        return (1);
    }

    if (argc <= 1)
    {   
        do_pcie_status ();
        return (0);
    }
    
    if (argv[1][0] == '1')
        ;
    else if (argv[1][0] == 's')
    {   
        do_pcie_stress ();
        return (0);
    }
    else
    {   
        do_pcie_status ();
        return (0);
    }
       
    controller = simple_strtoul(argv[1], NULL, 10);
    if (controller != 1)
        printf("PCIE controller %d out of range 1.\n",controller);

    link = MV_REG_READ(PEX_STATUS_REG(0)) & 0x1;
    if (argc == 3) {
    if (strcmp(argv[2], "-v") == 0) {
        pcieinfo(controller, 0,0); /* 0 - bus0 0 - verbose list */
           if (link != 0)
               return (0);
        pcieinfo(controller, 1,0); /* 1 - bus1 0 - verbose list */
    }
    else if (strcmp(argv[2],"-d") == 0) {
        pciedump(controller, 0);  /* 0 - bus0 */
           if (link != 0)
               return (0);
        pciedump(controller, 1);  /* 1 - bus1 */
    }
    }else {
    pcieinfo(controller, 0,1); /* 0 - bus0 1 - short list */
       if (link != 0)
           return (0);
    pcieinfo(controller, 1,1); /* 1 - bus1 1 - short list */
    }
    
    return (0);
 usage:
    printf ("Usage:\n%s\n", cmd->usage);
    return (1);
}


/* Convert the "bus.device.function" identifier into a number.
 */
static pcie_dev_t 
get_pcie_dev (char* name)
{
    char cnum[12];
    int len, i, iold, n;
    int bdfs[3] = {0,0,0};

    len = strlen(name);
    if (len > 8)
        return (-1);
    for (i = 0, iold = 0, n = 0; i < len; i++) {
        if (name[i] == '.') {
            memcpy(cnum, &name[iold], i - iold);
            cnum[i - iold] = '\0';
            bdfs[n++] = simple_strtoul(cnum, NULL, 16);
            iold = i + 1;
        }
    }
    strcpy(cnum, &name[iold]);
    if (n == 0)
        n = 1;
    bdfs[n] = simple_strtoul(cnum, NULL, 16);
    return PCIE_BDF(bdfs[0], bdfs[1], bdfs[2]);
}

static int 
pcie_cfg_display (int controller, pcie_dev_t bdf, ulong addr, ulong size,
                        ulong length)
{
#define DISP_LINE_LEN   16
    ulong i, nbytes, linebytes;
    int rc = 0;

    if (length == 0)
        length = 0x40 / size; /* Standard PCI configuration space */

    /* Print the lines.
     * once, and all accesses are with the specified bus width.
     */
    nbytes = length * size;
    do {
        uint    val4;
        ushort  val2;
        u_char  val1;

        printf("%08lx:", addr);
        linebytes = (nbytes>DISP_LINE_LEN)?DISP_LINE_LEN:nbytes;
        for (i = 0; i < linebytes; i+= size) {
            if (size == 4) {
                pcie_hose_read_config_dword(&pcie_hose[controller-1], 
                            bdf, addr, &val4);
                printf(" %08x", val4);
            } else if (size == 2) {
                pcie_hose_read_config_word(&pcie_hose[controller-1], 
                            bdf, addr, &val2);
                printf(" %04x", val2);
            } else {
                pcie_hose_read_config_byte(&pcie_hose[controller-1], 
                            bdf, addr, &val1);
                printf(" %02x", val1);
            }
            addr += size;
        }
        printf("\n");
        nbytes -= linebytes;
        if (ctrlc()) {
            rc = 1;
            break;
        }
    } while (nbytes > 0);

    return (rc);
}

static int 
pcie_cfg_write (int controller, pcie_dev_t bdf, ulong addr, ulong size, 
                ulong value)
{
    if (size == 4) {
        pcie_write_config_dword(bdf, addr, value);
    }
    else if (size == 2) {
        ushort val = value & 0xffff;
        pcie_hose_write_config_word(&pcie_hose[controller-1], bdf, addr, val);
    }
    else {
        u_char val = value & 0xff;
        pcie_hose_write_config_byte(&pcie_hose[controller-1], bdf, addr, val);
    }
    return (0);
}

static int
pcie_cfg_modify (int controller, pcie_dev_t bdf, ulong addr, ulong size, 
                ulong value, int incrflag)
{
    ulong   i;
    int nbytes;
    extern char console_buffer[];
    uint    val4;
    ushort  val2;
    u_char  val1;

    /* Print the address, followed by value.  Then accept input for
     * the next value.  A non-converted value exits.
     */
    do {
        printf("%08lx:", addr);
        if (size == 4) {
            pcie_hose_read_config_dword(&pcie_hose[controller-1], 
                        bdf, addr, &val4);
            printf(" %08x", val4);
        }
        else if (size == 2) {
            pcie_hose_read_config_word(&pcie_hose[controller-1], 
                        bdf, addr, &val2);
            printf(" %04x", val2);
        }
        else {
            pcie_hose_read_config_byte(&pcie_hose[controller-1], 
                        bdf, addr, &val1);
            printf(" %02x", val1);
        }

        nbytes = readline (" ? ");
        if (nbytes == 0 || (nbytes == 1 && console_buffer[0] == '-')) {
            /* <CR> pressed as only input, don't modify current
             * location and move to next. "-" pressed will go back.
             */
            if (incrflag)
                addr += nbytes ? -size : size;
            nbytes = 1;
#ifdef CONFIG_BOOT_RETRY_TIME
            reset_cmd_timeout(); /* good enough to not time out */
#endif
        }
#ifdef CONFIG_BOOT_RETRY_TIME
        else if (nbytes == -2) {
            break;  /* timed out, exit the command  */
        }
#endif
        else {
            char *endp;
            i = simple_strtoul(console_buffer, &endp, 16);
            nbytes = endp - console_buffer;
            if (nbytes) {
#ifdef CONFIG_BOOT_RETRY_TIME
                /* good enough to not time out
                 */
                reset_cmd_timeout();
#endif
                pcie_cfg_write (controller, bdf, addr, size, i);
                if (incrflag)
                    addr += size;
            }
        }
    } while (nbytes);

    return (0);
}


/* PCI Configuration Space access commands
 *
 * Syntax:
 *  pci display[.b, .w, .l] bus.device.function} [addr] [len]
 *  pci next[.b, .w, .l] bus.device.function [addr]
 *      pci modify[.b, .w, .l] bus.device.function [addr]
 *      pci write[.b, .w, .l] bus.device.function addr value
 */
int 
do_pcie (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    ulong addr = 0, value = 0, size = 0;
    pcie_dev_t bdf = 0;
    char cmd = 's';
    int controller = 1;  /* default PCIe 1 */
    unsigned char link;

    if (board_pci() == 0) {
        printf("No PCIE supported.\n");
        return (1);
    }

    if (argc < 2)
            goto usage;
    
    if (argc == 2) {
        if (!strcmp(argv[1], "init"))
            pcie_init();
        return (1);
    }

    if (argc > 2)
        cmd = argv[2][0];
    
    if (argv[1][0] == '1')
        ;
    else
        goto usage;
       
    controller = simple_strtoul(argv[1], NULL, 10);
    if (controller != 1)
        printf("PCIe controller %d out of range 1.\n",controller);

    link = MV_REG_READ(PEX_STATUS_REG(0)) & 0x1;

    switch (cmd) {
    case 'd':       /* display */
    case 'n':       /* next */
    case 'm':       /* modify */
    case 'w':       /* write */
        /* Check for a size specification. */
        size = cmd_get_data_size(argv[2], 4);
        if (argc > 4)
            addr = simple_strtoul(argv[4], NULL, 16);
        if (argc > 5)
            value = simple_strtoul(argv[5], NULL, 16);
    case 'h':       /* header */
        if (argc < 4)
            goto usage;
        if ((bdf = get_pcie_dev(argv[3])) == -1)
            return (1);
        break;
    default:        /* scan bus */
        value = 1; /* short listing */
        bdf = 0;   /* bus number  */
        if (argc > 2) {
            if (argv[argc-1][0] == 'l') {
                value = 0;
                argc--;
            }
            if (argc > 2)
                bdf = simple_strtoul(argv[2], NULL, 16);
        }
              if ((bdf >= 1) && (link != 0))
                  return (0);
        pcieinfo(controller, bdf, value);
        return (0);
    }

       if ((bdf >= 1) && (link != 0))
           return (0);
       
    switch (argv[2][0]) {
    case 'h':       /* header */
        pcie_header_show(controller, bdf);
        return (0);
    case 'd':       /* display */
        return pcie_cfg_display(controller, bdf, addr, size, value);
    case 'n':       /* next */
        if (argc < 5)
            goto usage;
        return pcie_cfg_modify(controller, bdf, addr, size, value, 0);
    case 'm':       /* modify */
        if (argc < 5)
            goto usage;
        return pcie_cfg_modify(controller, bdf, addr, size, value, 1);
    case 'w':       /* write */
        if (argc < 6)
            goto usage;
        return pcie_cfg_write(controller, bdf, addr, size, value);
    }

    return (1);
 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return (1);
}

#define EX2200_DIAG_NUM 7
static char *ex2200_diag[EX2200_DIAG_NUM]=
        {"cpu", "ram", "pci", "i2c", "ethernet", "usb", "uart"};
	
int 
do_diag (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
    int num_test, res = 0;
    int cmd_flag = POST_RAM | POST_MANUAL;
    unsigned int i;
    unsigned int j;

    if ((argc >= 3) && (!strcmp(argv[argc-1], "-v"))) {
        cmd_flag = POST_RAM | POST_MANUAL | POST_DEBUG;
        argc = argc - 1;
    }
    
    if (argc == 1) {
        /* List test info */
        printf("Available hardware tests:\n");
        post_info(NULL);
        printf("Use 'diag [<test1> [<test2> ...]]'"
               " to get more info.\n");
        printf("Use 'diag run [<test1> [<test2> ...]] [-v]'"
               " to run tests.\n");
    }
    else if (argc == 2) {
        if (!strcmp(argv[1],"run")) {
            for (i = 0; i < EX2200_DIAG_NUM; i++) {
                if (post_run(ex2200_diag[i], cmd_flag) != 0)
                    res = -1;
            }
            if (res)
                printf("\n- EX2200 test                           FAILED @\n");
            else
                printf("\n- EX2200 test                           PASSED\n");
            }
        else
            puts(" Please specify the test name \n");
    }
    else {
        if ((argc == 5) && (!strcmp(argv[3], "test"))) {
            num_test = simple_strtoul(argv[4], NULL, 10);
            printf("The total number of test %d\n",num_test);
            for (j = 0; j <= num_test; j++) {
                printf("The current test number : %d\n",j);
                for (i = 2; i < argc - 2; i++) {
                    post_run(argv[i], cmd_flag);
                }
            }
        }
        else {
            for (i = 2; i < argc; i++) {
                post_run (argv[i], cmd_flag);
            }
        }
    }
    return (0);
    
}

void 
u_sleep (ulong delay)
{
    ulong count = delay * (mvTclkGet()/1000000);
    ulong old_count = mvCntmrRead(UBOOT_CNTR);  /* count down counter */
    ulong new_count, delta;

    while(1) {
        new_count = mvCntmrRead(UBOOT_CNTR);
        delta = (new_count <= old_count) ? (old_count - new_count) :
                (0xFFFFFFFF - new_count + old_count);
        if (count > delta)
            count -= delta;
        else
            break;
        old_count = new_count;
    }
}

#if !defined(CONFIG_PRODUCTION)
static uint8_t echo = 1; 
void 
read_poe_status (void)
{
    uint8_t rdata[16];
    int i = 0, ret;

    if ((ret = i2c_read(0, I2C_POE_ADDR, 0, 0, rdata, 15)) != 0) {
       printf("i2c failed to read poe status[%d] from address 0x%x.\n",
                i, I2C_POE_ADDR);
       return;
    }
    
    printf ("POE status   - \n");
    for (i = 0; i < 15; i++)
       printf("%02x ", rdata[i]);
    printf("\n");

}

void 
send_poe_command (uint8_t *tdata)
{
    int i = 0, ret;

    i2c_mux(I2C_MAIN_MUX_ADDR, 1, 1);  /* enable chan 1 */
    if ((ret = i2c_write(0, I2C_POE_ADDR, 0, 0, tdata, 15)) != 0) {
        printf("i2c failed to write poe data to address 0x%x.\n", 
                I2C_POE_ADDR);
    }
    else {
        printf ("POE command - \n");
        for (i = 0; i < 15; i++)
            printf("%02x ", tdata[i]);
        printf("\n");
        udelay(400000);
        read_poe_status();
        echo++;
        if (echo > 0x7f)
           echo = 1;
    }

    i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);  /* disable mux */
}

/* poe commands
 *
 * Syntax:
 *    poe [enable|disable] <port>
 */
int 
do_poe (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{

    char cmd1;
    int i, j, dtemp, len;
    uint8_t port;
    uint8_t tdata[15]; 
    uint8_t rdata[15], temp[4]; 
    uint16_t cksum = 0, limit;
    uint8_t value = 0, value1 = 0; 
    uint32_t wait  = 0; 
    char tbyte[3];
    char *data;
    
    if (argc <= 1)
        goto usage;
    
    memset(tdata, 0xFE, 15);
    cmd1 = argv[1][0];
    
    switch (cmd1) {
        case 'o':       /* On/Off */
        case 'O':
            syspld_reg_read(0xB, &value);
            syspld_reg_read(0x1B, &value1);
            if ((argv[1][1]=='n') || (argv[1][1]=='N')) {
                value &= ~0x20;  /* enable poe controller */
                syspld_reg_write(0xB, value);
                value1 &= ~0x2;  /* enable poe ports */
                syspld_reg_write(0x1B, value1);
                udelay(3000000);
                i2c_mux(I2C_MAIN_MUX_ADDR, 1, 1);  /* ena mux chan 1*/
                read_poe_status();

                i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);  /* disable mux */
            }
            else {
                value |= 0x20;  /* disable poe controller */
                syspld_reg_write(0xB, value);
                value1 |= 0x2;  /* disable poe ports */
                syspld_reg_write(0x1B, value1);
            }
                
            break;
            
        case 'e':       /* enable */
        case 'E':
        case 'd':       /* disable */
        case 'D':  
            if (argc <= 2)
                goto usage;
            
            if ((argv[2][0] == 'L') || (argv[2][0] == 'l')) { /* legacy mode */
                tdata[0] = 0x00;  /* command */
                tdata[1] = echo;
                tdata[2] = 0x07;  /* global */
                tdata[3] = 0x2B;  /* maskz */
                if ((cmd1 == 'e') || (cmd1 == 'E'))
                   tdata[4] = 0x7;   /* legacy mode on */
                else
                   tdata[4] = 0x5;   /* legacy mode off */
                            
                for (i = 0; i < 13; i++) {
                    cksum += tdata[i];
                }
                tdata[13] = (cksum & 0xff00) >> 8;
                tdata[14] = cksum & 0xff;
                
                send_poe_command(tdata);                            
                return (1);
            }
            
            if ((argv[2][0] == 'D') || (argv[2][0] == 'D')) { /* DC disconnect */
                tdata[0] = 0x00;  /* command */
                tdata[1] = echo;
                tdata[2] = 0x07;  /* global */
                tdata[3] = 0x56;  /* mask */
                tdata[4] = 0x09;  /* mask key number */
                if ((cmd1 == 'e') || (cmd1 == 'E'))
                   tdata[5] = 0x0;   /* DC disconnect enable */
                else
                   tdata[5] = 0x1;   /* DC disconnect disable */
                            
                for (i = 0; i < 13; i++) {
                    cksum += tdata[i];
                }
                tdata[13] = (cksum & 0xff00) >> 8;
                tdata[14] = cksum & 0xff;
                
                send_poe_command(tdata);                            
                return (1);
            }
            
            if ((argv[2][0] == 'a') || (argv[2][0] == 'A')) {
                port = 128;  /* all ports */
            }
            else {
                port = simple_strtoul(argv[2], NULL, 10);
                if (port > 47) {
                    printf("POE port %d out of range 0-47.\n", port);
                }
            }
            tdata[0] = 0x00;  /* command */
            tdata[1] = echo;
            tdata[2] = 0x05;  /* channel */
            tdata[3] = 0x0C;  /* EnDis */
            tdata[4] = port;   /* port */
            if ((cmd1 == 'e') || (cmd1 == 'E'))
                tdata[5] = 1;   /* enable */
            else
                tdata[5] = 0;   /* disable */
            
            for (i = 0; i < 13; i++) {
                cksum += tdata[i];
            }
            tdata[13] = (cksum & 0xff00) >> 8;
            tdata[14] = cksum & 0xff;

            send_poe_command(tdata);                            
            break;

        case 's':       /* status */
        case 'S':
#if 0
            if (argc == 2) {  
                tdata[0] = 0x02;  /* command */
                tdata[1] = echo;
                tdata[2] = 0x07;  /* global */
                tdata[3] = 0x3d;   /* system status */
                for (i = 0; i < 13; i++) {
                    cksum += tdata[i];
                }
                tdata[13] = (cksum & 0xff00) >> 8;
                tdata[14] = cksum & 0xff;
                
                send_poe_command(tdata);                            
                break;
            }
            else if (argc == 4) {
                tdata[0] = 0x02;  /* command */
                tdata[1] = echo;
                tdata[2] = 0x07;  /* global */
                tdata[3] = 0x87;   /* device status */
                dev = simple_strtoul(argv[3], NULL, 10);
                tdata[4] = dev;
                for (i = 0; i < 13; i++) {
                    cksum += tdata[i];
                }
                tdata[13] = (cksum & 0xff00) >> 8;
                tdata[14] = cksum & 0xff;
                
                send_poe_command(tdata);                            
                break;
            }
#endif

        case 'l':       /* power limit */
        case 'L':
        case 'p':   /* priority/PM/param */
        case 'P':
        case 'm':   /* electrical parameters */ /* mask */
        case 'M':
            if (argc < 2)
                goto usage;
            
            port = simple_strtoul(argv[2], NULL, 10);
            tdata[0] = 0x02;  /* command */
            tdata[1] = echo;
            tdata[2] = 0x05;  /* channel */
            tdata[4] = port;   /* port */

            if ((cmd1 == 's') || (cmd1 == 'S'))
                if ((argv[1][1] == 't') || (argv[1][1] == 'T')) { /* status */
                    if (argc == 2) { 
                        tdata[2] = 0x07;  /* global */
                        tdata[3] = 0x3d;   /* system status */
                        tdata[4] = 0xFE;
                    }
                    else if (argc == 3) {
                        tdata[2] = 0x05;  /* port */
                        tdata[3] = 0x0E;   /* port status */
                    }
                    else
                        goto usage;
                }
                else if ((argv[1][1] == 'e') || (argv[1][1] == 'E')) {
                     tdata[0] = 0x01;  /* command */
                    if ((argv[2][0] == 's') || (argv[2][0] == 'S')) {
                        tdata[2] = 0x06;  /* E2 */
                        tdata[3] = 0x0F;  /* save config */
                        tdata[4] = 0xFE; 
                    }
                    else  if ((argv[2][0] == 'd') || (argv[2][0] == 'D')) {
                        tdata[2] = 0x2D;  /* restore factory default */
                        tdata[3] = 0xFE;
                        tdata[4] = 0xFE; 
                    }
                    else
                        goto usage;
                }
                else
                    goto usage;
            else if ((cmd1 == 'l') || (cmd1 == 'L')) {
                tdata[3] = 0x0B;  /* port power limit */
                if (argc == 4) {
                    tdata[0] = 0x0;  /* set command */
                    limit = simple_strtoul(argv[3], NULL, 16);
                    tdata[5] = (limit & 0xFF00) >> 8;
                    tdata[6] = limit & 0xFF;
                }
            }
            else if ((cmd1 == 'p') || (cmd1 == 'P')) {
                if ((argv[1][1] == 'm') || (argv[1][1] == 'M')) { /* PM method */
                    if (argc == 2) {
                        tdata[2] = 0x07;  /* global */
                        tdata[3] = 0x0B;  /* supply */
                        tdata[4] = 0x5F;  /* power manage mode */
                    }
                    else if (argc == 5) {
                        tdata[0] = 0x00;  /* set */
                        tdata[2] = 0x07;  /* global */
                        tdata[3] = 0x0B;  /* supply */
                        tdata[4] = 0x5F;  /* power manage mode */
                        tdata[5] = simple_strtoul(argv[2], NULL, 16);
                        tdata[6] = simple_strtoul(argv[3], NULL, 16);
                        tdata[7] = simple_strtoul(argv[4], NULL, 16);
                    }
                    else
                        goto usage;
                }
                else if ((argv[1][1] == 'r') || (argv[1][1] == 'R')) {
                    tdata[3] = 0x0A;  /* port priority */
                    if (argc == 4) {
                        tdata[0] = 0x0;  /* set command */
                        tdata[5] = simple_strtoul(argv[3], NULL, 16);
                    }
                }
                else if ((argv[1][1] == 'a') || (argv[1][1] == 'A')) { 
                    if (argc == 3) {
                        tdata[2] = 0x07;  /* global */
                        tdata[3] = 0x87;  /* device param */
                        tdata[4] = simple_strtoul(argv[2], NULL, 16);
                        tdata[5] = 0;
                        tdata[6] = 0;
                        tdata[7] = 0;
                        tdata[8] = 0;
                        tdata[9] = 0;
                        tdata[10] = 0;
                        tdata[11] = 0;
                        tdata[12] = 0;
                    }
                    else if (argc == 6) {
                        tdata[0] = 0x00;  /* set */
                        tdata[2] = 0x07;  /* global */
                        tdata[3] = 0x87;  /* device param */
                        tdata[4] = simple_strtoul(argv[2], NULL, 16);/* dev */
                        tdata[5] = simple_strtoul(argv[3], NULL, 16);/* IE */
                        tdata[6] = simple_strtoul(argv[4], NULL, 16);/* TSH */
                        if ((argv[5][0] == 'a') || (argv[5][0] == 'A')) {  
                            tdata[7] = 0; /* AF */
                        }
                        else {  /* MIP-AT */
                            tdata[7] = 1;
                        }
                    }
                    else
                        goto usage;
                }
                else
                    goto usage;
            }
            else if ((cmd1 == 'm') || (cmd1 == 'M')) {
                if ((argv[1][1] == 'a') || (argv[1][1] == 'A')) {  /* mask */
                    if (argc == 3) {
                        tdata[2] = 0x07;  /* global */
                        tdata[3] = 0x56;  /* individual mask */
                        tdata[4] = simple_strtoul(argv[2], NULL, 16); /* key */
                    }
                    else {
                        tdata[0] = 0x00;  /* set */
                        tdata[2] = 0x07;  /* global */
                        tdata[3] = 0x56;  /* individual mask */
                        tdata[4] = simple_strtoul(argv[2], NULL, 16);
                        tdata[5] = simple_strtoul(argv[3], NULL, 16);
                    }
                }
                else {
                    tdata[3] = 0x25;  /* port electrial parameters */
                }
            }
            
            for (i = 0; i < 13; i++) {
                cksum += tdata[i];
            }
            tdata[13] = (cksum & 0xff00) >> 8;
            tdata[14] = cksum & 0xff;

            send_poe_command(tdata);                            
            break;
            
        case 'r':       /* reset */
        case 'R':

            if ((argv[1][2]=='a') || (argv[1][2]=='A')) {
                i2c_mux(I2C_MAIN_MUX_ADDR, 1, 1);  /* ena channel 1 */
                read_poe_status();
                i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);  /* disable mux */
                break;
            }

            /* Reset command  */                
            tdata[0] = 0x00;
            tdata[1] = 0x00;
            tdata[2] = 0x07;  /* Global */
            tdata[3] = 0x55;  /* Reset */
            tdata[4] = 0x00;
            tdata[5] = 0x55;  /* Reset */
            tdata[6] = 0x00;
            tdata[7] = 0x55;  /* Reset */
     
            for (i = 0; i < 13; i++) {
                cksum += tdata[i];
            }
            tdata[13] = (cksum & 0xff00) >> 8;
            tdata[14] = cksum & 0xff;

            send_poe_command(tdata);                            
            break;
            
        case 'c':       /* command */
        case 'C':

            if (argc < 2)
                goto usage;
            
            memset(tdata, 0x4E, 15);

            data = argv [2];
            len = strlen(data)/2;
            tbyte[2] = 0;
            for (i = 0; i < len; i++) {
                tbyte[0] = data[2*i];
                tbyte[1] = data[2*i+1];
                dtemp = atoh(tbyte);
                tdata[i] = dtemp;
            }
            for (i = 0; i < 13; i++) {
                cksum += tdata[i];
            }
            tdata[13] = (cksum & 0xff00) >> 8;
            tdata[14] = cksum & 0xff;

            i2c_mux(I2C_MAIN_MUX_ADDR, 1, 1);  /* enable chan 1 */
            if ((i2c_write(0, I2C_POE_ADDR, 0, 0, tdata, 15)) != 0) {
                printf("i2c failed to write poe data to address 0x%x.\n", 
                        I2C_POE_ADDR);
            }
            else {
                printf ("POE command - \n");
                for (i = 0; i < 15; i++)
                    printf("%02x ", tdata[i]);
                printf("\n");
                udelay(400000);
                read_poe_status();
            }

            i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0);  /* disable mux */
            
            break;
            
        case 't':       /* temperature */
        case 'T':

            if (argc > 2)
                wait = simple_strtoul(argv[2], NULL, 10);
            else
                wait = 400000;
            
            tdata[0] = 0x02;  /* command */
            tdata[2] = 0x07;  /* global */
            tdata[3] = 0x87;   /* device status */
            
            i2c_mux(I2C_MAIN_MUX_ADDR, 1, 1);  /* ena channel 1 */

            for (j = 0; j < 4; j++) {
                temp[j] = 0;
                cksum = 0;
                
                tdata[1] = echo;
                tdata[4] = j;
                for (i = 0; i < 13; i++) {
                    cksum += tdata[i];
                }
                tdata[13] = (cksum & 0xff00) >> 8;
                tdata[14] = cksum & 0xff;
                
                if (i2c_write(0, I2C_POE_ADDR, 0, 0, tdata, 15) != 0) {
                    printf("i2c failed to write poe data to address 0x%x.\n", 
                            I2C_POE_ADDR);
                    i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0); /* disable mux channel */
                    return (0);
                }
                
                udelay(wait);
                if (i2c_read(0, I2C_POE_ADDR, 0, 0, rdata, 15) != 0) {
                    printf("i2c failed to read poe status[%d] from "
                            "address 0x%x.\n", i, I2C_POE_ADDR);
                    i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0); /* disable mux channel */
                    return (0);
                }
                
                if ((rdata[0] == 0x3) && (rdata[1] == echo) && 
                    (rdata[9] != 0xff))
                    temp[j] = rdata[9];
                
                echo++;
                if (echo > 0x7f)
                    echo = 1;
            }
            i2c_mux(I2C_MAIN_MUX_ADDR, 0, 0); /* disable mux channel */
            
            printf("Temperature (deg C): PD64012 (TEMPn)\n");
            printf("TEMP1   TEMP2   TEMP3   TEMP4\n");
            printf("%3d     %3d     %3d     %3d\n",
                temp[0], temp[1], temp[2], temp[3]);
            printf("\n");
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

int 
do_runloop (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	int i, j = 0;

	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return (1);
	}
       for (;;) {
		if (ctrlc()) {
			putc ('\n');
			return (0);
		}
		printf("\n\nLoop:  %d\n", j);
		j++;
		for (i = 1; i < argc; ++i) {
			char *arg;

			if ((arg = getenv (argv[i])) == NULL) {
				printf ("## Error: \"%s\" not defined\n", argv[i]);
				return (1);
			}
#ifndef CFG_HUSH_PARSER
			if (run_command (arg, flag) == -1)
				return (1);
#else
			if (parse_string_outer(arg,
		    	FLAG_PARSE_SEMICOLON | FLAG_EXIT_FROM_LOOP) != 0)
				return (1);
#endif
		}
       }
	return (0);
}

/* do_mfg_config commands
 *
 * Syntax:
 */

extern char console_buffer[];

typedef struct 
    {
        char *name; 
        ushort id; 
        char *clei; 
        ushort macblk;
    } assm_id_type;

assm_id_type assm_id_list_board [] = {
 { "EX2200-48P-4G"     , I2C_ID_JUNIPER_EX2200_48_P, "N/A       " },
 { "EX2200-48T-4G"     , I2C_ID_JUNIPER_EX2200_48_T, "N/A       " },
 { "EX2200-24P-4G"     , I2C_ID_JUNIPER_EX2200_24_P, "N/A       " },
 { "EX2200-24T-4G"     , I2C_ID_JUNIPER_EX2200_24_T, "N/A       " },
 { "EX2200-C-12P-2G"   , I2C_ID_JUNIPER_EX2200_12_P, "N/A       " },
 { "EX2200-C-12T-2G"   , I2C_ID_JUNIPER_EX2200_12_T, "N/A       " },
 { "EX2200-24T-DC-4G"  , I2C_ID_JUNIPER_EX2200_24_T_DC, "N/A       " },
 {  NULL            , 0                          }
};

assm_id_type assm_id_list_system [] = {
 { "EX2200-48P-4G"     , I2C_ID_JUNIPER_EX2200_48_P, "N/A       " ,0x40},
 { "EX2200-48T-4G"     , I2C_ID_JUNIPER_EX2200_48_T, "N/A       " ,0x40},
 { "EX2200-24P-4G"     , I2C_ID_JUNIPER_EX2200_24_P, "N/A       " ,0x40},
 { "EX2200-24T-4G"     , I2C_ID_JUNIPER_EX2200_24_T, "N/A       " ,0x40},
 { "EX2200-C-12P-2G"   , I2C_ID_JUNIPER_EX2200_12_P, "N/A       " ,0x40},
 { "EX2200-C-12T-2G"   , I2C_ID_JUNIPER_EX2200_12_T, "N/A       " ,0x40},
 { "EX2200-24T-DC-4G"  , I2C_ID_JUNIPER_EX2200_24_T_DC, "N/A       ", 0x40 },
 {  NULL             , 0                          }
};

int 
isxdigit (int c)
{
    if ('0' <= c && c <= '9') return (1);
    if ('A' <= c && c <= 'Z') return (1);
    if ('a' <= c && c <= 'z') return (1);
    return (0);
}

int 
do_mfgcfg (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    assm_id_type *assm_id_list = assm_id_list_system;
    ulong    mux        = I2C_MAIN_MUX_ADDR, 
                device     = I2C_MAIN_EEPROM_ADDR, 
                channel    = 3;

    int         ret, 
                assm_id    = 0;

    int         i,j,nbytes;

    unsigned    int     start_address = 0;

    uint8_t     tdata[0x80];
    char        tbyte[3];
    
    if (argc < 2)
    {   printf ("Usage:\n%s\n", cmdtp->usage);
        return (1);
    }
    
    if (strcmp(argv[1],"system")) 
    {   printf ("Usage:\n%s\n", cmdtp->usage);
        return (1);
    }

    if ((ret = i2c_mux(mux, channel, 1)) != 0) {
        printf("i2c failed to enable mux 0x%x channel 0x%x.\n", mux, channel);
        i2c_mux(mux, 0, 0);
        return (1);
    }

    if (secure_eeprom_read (device, start_address, tdata, 0x80)) {
        printf ("ERROR reading eeprom at device 0x%x\n", device);
        i2c_mux(mux, 0, 0);
        return (1);
    }
    
    for (i = 0; i < 0x80; i+=8)
    {
        printf("%02X : ",i + start_address);
        for (j = 0; j < 8; j++)
           printf("%02X ",tdata[i+j]);
        printf("\n");
    }   
    
    printf ("Start address (Hex)= 0x%04X\n", start_address);    
    assm_id   = (tdata[4]<<8) + tdata[5];
    printf ("Assembly Id (Hex)= 0x%04X\n", assm_id);
    for (i = 0; assm_id_list[i].name; i++)
        if (assm_id_list[i].id == assm_id)
            break;
            
    if (assm_id_list[i].name)
        printf ("Assembly name = %s\n", assm_id_list[i].name);
    else
        printf ("Invalid / Undefined Assembly Id\n");

    if (argc >= 3) 
    {
        if (!strcmp(argv[2],"update"))
        {
            /* constants for all eeproms*/    
            memcpy (tdata      ,"\x7F\xB0\x02\xFF", 4);
            memcpy (tdata+0x08 ,"REV "            , 4);         
            memcpy (tdata+0x31 ,"\xFF\xFF\xFF"    , 3);
            tdata[0x2c] = 0;
            tdata[0x44] = 1;
            memset (tdata+0X74, 0x00, 12);

            printf("List of assemblies:\n");
            for (i = 0; assm_id_list[i].name; i++)
                printf ("%2d = %s\n", i+1, assm_id_list[i].name);
            
            nbytes = readline ("\nSelect assembly:");
            if (nbytes) 
            {   i = simple_strtoul(console_buffer, NULL, 10) - 1;
                assm_id = assm_id_list[i].id;
                printf ("new Assm Id (Hex)= 0x%04X\n", assm_id);
                tdata[4] = assm_id >>   8;
                tdata[5] = assm_id & 0xFF;
            } 
            else 
            {
                for (i = 0; assm_id_list[i].name; i++)
                    if (assm_id_list[i].id == assm_id)
                        break;
            }   
            printf("\nHW Version : '%.3d'\n "   ,tdata[0x6]); 
            nbytes = readline ("Enter HW version: ");      
            if (nbytes) 
                tdata[0x06] = simple_strtoul (console_buffer, NULL, 10);
           
            printf("\n"
                   "Assembly Part Number  : '%.12s'\n "   ,tdata+0x14); 
            printf("Assembly Rev          : '%.8s'\n"     ,tdata+0x0C); 
            nbytes = readline ("Enter Assembly Part Number: ");
            if (nbytes >= 10) 
            {
                strncpy (tdata+0x14, console_buffer,10);
                tdata [0x14+10] = 0;
              
                if (nbytes >= 16 && (!strncmp(console_buffer+10, " REV ", 5)
                    || !strncmp(console_buffer+10, " Rev ", 5) ))
                {
                    strncpy (tdata+0x0C, console_buffer+15,8);
                    tdata[0x07] = simple_strtoul (console_buffer+15, NULL, 10);
                }
                else
                {   /* enter Rev separate */
                    nbytes = readline ("Enter Rev: ");
                    strncpy (tdata+0x0C, console_buffer,8);
                    tdata[0x07] = simple_strtoul (console_buffer, NULL, 10);
                }
            }

            /* only ask for Model number/rev for FRU */
            if (assm_id_list[i].name && strncmp(assm_id_list[i].clei, "N/A", 10))
            {
                printf("\n"
                       "Assembly Model Name   : '%.23s'\n " ,tdata+0x4f); 
                printf("Assembly Model Rev    : '%.3s'\n"   ,tdata+0x66); 
                nbytes = readline ("Enter Assembly Model Name: ");
                if (nbytes >= 13) 
                {
                    strncpy (tdata+0x4f, console_buffer,13);
                    tdata [0x4f+13] = 0;
                  
                    if (nbytes >= 19 && (!strncmp(console_buffer+13, " REV ",5)
                        || !strncmp(console_buffer+13, " Rev ", 5) ))
                        strncpy (tdata+0x66, console_buffer+18,3);
                    else
                    {    /* enter Rev separate */
                        nbytes = readline ("Enter Rev: ");
                        strncpy (tdata+0x66, console_buffer,3);
                    }
                }
            } 
            else 
                tdata[0x44] = 0;     /* not a FRU */
            
            printf("\nAssembly Serial Number: '%.12s'\n"   ,tdata+0x20); 
            nbytes = readline ("Enter Assembly Serial Number: ");       
            if (nbytes) 
                strncpy (tdata+0x20, console_buffer,12);
            

            if (!strcmp(argv[1],"system")) 
            {  
                /* constants for system eeprom */   
                memcpy (tdata+0x34,"\xAD\x01\x00",3);

                retry_mac:
                printf("\nBase MAC: ");  
                for (j = 0; j < 6; j++)
                    printf("%02X",tdata[0x38+j]);
                
                nbytes = readline ("\nEnter Base MAC: ");
                for (i = j = 0; console_buffer[i]; i++)
                    if (isxdigit(console_buffer[i]))
                        console_buffer[j++] = console_buffer[i];
                console_buffer[j] = 0;
                nbytes = j;
                
                if (nbytes)
                {   
                    if (nbytes==12)
                    {
                        tbyte[2] = 0;
                        for (i = 0; i < 6; i++)
                        {
                            tbyte[0]      = console_buffer[2*i];
                            tbyte[1]      = console_buffer[2*i+1];
                            tdata[0x38+i] = atoh(tbyte);
                        }
                    }
                    else 
                    {
                        printf ("Mac must be 12 characters! "
                                "No update performed\n");
                        goto retry_mac;
                    }
                } 

                assm_id   = (tdata[4]<<8) + tdata[5];
                for (i = 0; assm_id_list[i].name; i++)
                    if (assm_id_list[i].id == assm_id)
                        break;
                        
                if (assm_id_list[i].name)
                {
                    tdata[0x37] = assm_id_list[i].macblk;
                }   
            }


            printf("\nDeviation Info        : '%.10s'\n"   ,tdata+0x69); 
            nbytes = readline ("Enter Deviation Info: ");       
            if (nbytes) 
               strncpy (tdata+0x69, console_buffer,10);

             /* prog clei */
            for (i = 0; assm_id_list[i].name; i++)
                if (assm_id_list[i].id == assm_id)
                    break;
            if (assm_id_list[i].name)
               strncpy(tdata+0x45, assm_id_list[i].clei,10);

            /* calculate checksum */
            for (j = 0, i = 0x44; i <= 0x72; i++)
               j += tdata[i];
            tdata[0x73] = j;
            secure_eeprom_write (device, start_address, tdata, 0x80);
        }
        else if (!strcmp(argv[2],"clear"))
        {
            memset (tdata, 0xFF, 0x80);
            secure_eeprom_write (device, start_address, tdata, 0x80);
        }       

        /* always reread */
        secure_eeprom_read (device, start_address, tdata, 0x80);
        for (i = 0; i < 0x80; i+=8)
        {
            printf("%02X : ",i + start_address);
            for (j = 0; j < 8; j++)
               printf("%02X ",tdata[i+j]);
            printf("\n");
        }   
    } 
    i2c_mux(mux, 0, 0);

    assm_id   = (tdata[4]<<8) + tdata[5];
    for (i = 0; assm_id_list[i].name; i++)
        if (assm_id_list[i].id == assm_id)
            break;
            
    printf    ("Assembly Id (Hex)     : '0x%04X'\n"  , assm_id);
    if (assm_id_list[i].name)
    {
        printf("HW Version            : '%.3d'\n"    , tdata[0x06]); 
        printf("Assembly Model Name   : '%.23s'\n"   , tdata+0x4F);
        printf("Assembly Model rev    : '%.3s'\n"    , tdata+0x66);
        printf("Assembly Clei         : '%.10s'\n"   , tdata+0x45);
        printf("Assembly Part Number  : '%.12s'\n"   , tdata+0x14); 
        printf("Assembly Rev          : '%.8s'\n"    , tdata+0x0C); 
        printf("Assembly Serial Number: '%.12s'\n"   , tdata+0x20); 
        if (!strcmp(argv[1],"system")) 
        {  
            printf("Base MAC              : ");  
            for (j = 0; j < 6; j++)
                printf("%02X",tdata[0x38+j]);
        }
        printf("\n"
               "Deviation Info        : '%.10s'\n"   ,tdata+0x69); 
    }

    return (1);
}

int 
do_msleep (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    ulong start_time = get_timer(0);
    ulong delay, end_time;

    if (argc != 2) {
        printf ("Usage:\n%s\n", cmdtp->usage);
        return (1);
    }

    delay = simple_strtoul(argv[1], NULL, 10);
    while(1) {
        if ((end_time = get_timer(start_time)) >= delay)
            break;
    }
    printf("delay %d, start 0x%08x, end 0x%08x/0x%08x\n",
        delay, start_time, end_time, get_timer(0));
    return (0);
}

int 
do_usleep (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    ulong delay;

    if (argc != 2) {
        printf ("Usage:\n%s\n", cmdtp->usage);
        return (1);
    }

    delay = simple_strtoul(argv[1], NULL, 10);
    u_sleep(delay);
    return (0);
}

extern unsigned long read_p15_c1 (void);
extern void icache_enable (void);
extern void icache_disable (void);
extern void dcache_enable (void);
extern void dcache_disable (void);
extern int dcache_status (void);
extern int icache_status (void);

static int 
on_off (const char *s)
{
    if (strcmp(s, "on") == 0) {
        return (1);
    } else if (strcmp(s, "off") == 0) {
        return (0);
    }
    return (-1);
}

int 
do_icache ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    switch (argc) {
        case 2:			/* on / off	*/
            switch (on_off(argv[1])) {
                case 0:	
                    icache_disable();
                    break;
                case 1:	
                    icache_enable ();
                    break;
            }
        /* FALL TROUGH */
        case 1:			/* get status */
            printf ("Instruction Cache (%08x) is %s\n", read_p15_c1(),
                    icache_status() ? "ON" : "OFF");
            return (0);
        default:
            printf ("Usage:\n%s\n", cmdtp->usage);
            return (1);
    }
    return (0);
}

int 
do_dcache ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    switch (argc) {
        case 2:			/* on / off	*/
            switch (on_off(argv[1])) {
                case 0:	
                    dcache_disable();
                    break;
                case 1:	
                    dcache_enable ();
                    break;
            }
        /* FALL TROUGH */
        case 1:			/* get status */
            printf ("Data Cache (%08x) is %s\n",  read_p15_c1(),
                    dcache_status() ? "ON" : "OFF");
            return (0);
        default:
            printf ("Usage:\n%s\n", cmdtp->usage);
            return (1);
    }
    return (0);
}
#endif

extern flash_info_t flash_info[];	/* info for FLASH chips */
extern int mv_flash_real_protect(flash_info_t *info, long sector, int prot);
extern int mv_flash_erase (flash_info_t *info, int s_first, int s_last);
extern int mv_write_buff (flash_info_t *info, uchar *src, ulong addr, 
            ulong cnt);

/* simple flash 16 bits write, do not accross sector */
int 
flash_write_direct (ulong addr, char *src, ulong cnt)
{
    flash_info_t *info;
    long sector;

    info = &flash_info[0];  /* 1 bank for EX2200 */
    sector = (addr - CFG_FLASH_BASE) / (64 * 1024);
    /* unprotect */
    mv_flash_real_protect(info, sector , 0);
    /* erase */
    mv_flash_erase(info, sector, sector);
    /* write */
    mv_write_buff(info, src, addr, cnt);
    /* protect */
    mv_flash_real_protect(info, sector , 1);

    return (0);
}

/* upgrade commands
 *
 * Syntax:
 *    upgrade get
 *    upgrade set <state>
 */
int 
do_upgrade (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char cmd1;
    int state;
    volatile int *ustate = (int *) (CFG_FLASH_BASE + 
            CFG_UPGRADE_BOOT_STATE_OFFSET);

    if (argc < 2)
        goto usage;
    
    cmd1 = argv[1][0];
    
    switch (cmd1) {
        case 'g':       /* get state */
        case 'G':
            printf("upgrade state = 0x%x\n", *ustate);
            break;
            
        case 's':                 /* set state */
        case 'S':
            state = simple_strtoul(argv[2], NULL, 16);
            if (state == *ustate)
                return (1);
            
            if (flash_write_direct(CFG_FLASH_BASE + 
                    CFG_UPGRADE_BOOT_STATE_OFFSET,
                (char *)&state, sizeof(int)) != 0)
                printf("upgrade set state 0x%x failed.\n", state);
                return (0);
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

#if defined(CONFIG_SPI_SW_PROTECT)
extern int ex_mvStatusRegGet (uint8_t * pStatReg);

int
ex_spi_sw_protect_set (uint32_t offset, uint8_t *pdata, uint32_t len)
{
    int ret;
	uint8_t cmd[4];
    uint8_t stat;
    int i;

    /* write enable */
    cmd[0] = 0x6;  /* WREN */
	if ((ret = mvSpiWriteThenRead(cmd, 1, NULL, 0, 0)) != 0)
		return ret;
    
    /* set write lock */
    cmd[0] = 0xE5;  /* WRLR */
	cmd[1] = ((offset >> 16) & 0xFF);
	cmd[2] = ((offset >> 8) & 0xFF);
	cmd[3] = (offset & 0xFF);

	if ((ret = mvSpiWriteThenWrite(cmd, 4, pdata, len)) != 0)
		return ret;

    /* wait write in progress clear */
	for (i = 0; i < 1000000; i++)
	{
        if ((ret = ex_mvStatusRegGet(&stat)) != 0)
            return ret;

		if ((stat & 0x1) == 0)
			return (0);
	}

	return (-1);
}

int
ex_spi_sw_protect_get (uint32_t offset, uint8_t *pdata, uint32_t len)
{
	uint8_t cmd[4];

    /* get write lock */
    cmd[0] = 0xE8;  /* RDLR */
	cmd[1] = ((offset >> 16) & 0xFF);
	cmd[2] = ((offset >> 8) & 0xFF);
	cmd[3] = (offset & 0xFF);

	return mvSpiWriteThenRead(cmd, 4, pdata, len, 0);
}

#if !defined(CONFIG_PRODUCTION)
/* sprotect commands
 *
 * Syntax:
 *    sprotect get <start> <end>
 *    sprotect on/off <start> <end>
 */
int 
do_sprotect (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char cmd;
    uint32_t saddr, eaddr, offset, i;
    uint8_t data;

    if (argc < 4)
        goto usage;
    
    cmd = argv[1][0];
    saddr = simple_strtoul(argv[2], NULL, 16);
    eaddr = simple_strtoul(argv[3], NULL, 16);
    saddr &= 0x7FFFFF;  /* SPI offset */
    eaddr &= 0x7FFFFF;  /* SPI offset */
    
    if (saddr >=  eaddr)
        goto usage;
    
    switch (cmd) {
        case 'g':       /* get RDLR */
        case 'G':
            for (i = saddr; i < eaddr; i += 0x10000) {
                offset = i & 0x7F0000;
                if (ex_spi_sw_protect_get(offset, &data, 1) != 0)
                    break;
                printf("SPI offset %06x RDLR = %02x\n", offset, data);
            }
            break;
            
        case 'o':                 /* set state */
        case 'O':
            switch (on_off(argv[1])) {
                case 0:	/* off */
                    data = 0x0;
                    for (i = saddr; i < eaddr; i += 0x10000) {
                        offset = i & 0x7F0000;
                        if (ex_spi_sw_protect_set(offset, &data, 1) != 0)
                            break;
                    }
                    break;
                case 1:	/* on */
                    data = 0x3;
                    for (i = saddr; i < eaddr; i += 0x10000) {
                        offset = i & 0x7F0000;
                        if (ex_spi_sw_protect_set(offset, &data, 1) != 0)
                            break;
                    }
                    break;
            }
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
#endif
#endif

/***************************************************/

U_BOOT_CMD(
    eth0,   6,  1,  do_eth0,
    "eth0    - management ethernet utility commands\n",
    "\n"
    "eth0 phy dump\n"
    "    - dump phy registers\n"
    "eth0 phy read <reg>\n"
    "    - read phy register\n"
    "eth0 phy write <reg> <data>\n"
    "    - write phy register <reg> with value\n"
    "eth0 loopback [mac | phy | ext [10 | 100]] <size> <count>\n"
    "    - loopback traffic test\n"
);

U_BOOT_CMD(
    pcie,   6,  1,  do_pcie,
    "pcie    - list and access PCIE Configuration Space\n",
    "<controller> [bus] [long]\n"
    "    - short or long list of PCIE devices on bus 'bus'\n"
    "pcie <controller> header b.d.f\n"
    "    - show header of PCIE device 'bus.device.function'\n"
    "pcie <controller> display[.b, .w, .l] b.d.f [address] [# of objects]\n"
    "    - display PCIE configuration space (CFG)\n"
    "pcie <controller> next[.b, .w, .l] b.d.f address\n"
    "    - modify, read and keep CFG address\n"
    "pcie <controller> modify[.b, .w, .l] b.d.f address\n"
    "    -  modify, auto increment CFG address\n"
    "pcie <controller> write[.b, .w, .l] b.d.f address value\n"
    "    - write to CFG address\n"
    "pcie init\n"
    "    - pcie init\n"
);

U_BOOT_CMD(
    pcieprobe,     3,      1,       do_pcie_probe,
    "pcieprobe  - List the PCI-E devices found on the bus\n",
    "<controller> [-v]/[-d](v - verbose d - dump)\n"
);

U_BOOT_CMD(
    diag,	CFG_MAXARGS,	0,	do_diag,
    "diag    - perform board diagnostics\n",
    "    - print list of available tests\n"
    "diag [test1 [test2]]\n"
    "         - print information about specified tests\n"
    "diag run - run all available tests\n"
    "diag run [test1 [test2]]\n"
    "         - run specified tests\n"
);

U_BOOT_CMD(
    upgrade, 3, 1, do_upgrade,
    "upgrade - upgrade state\n",
    "\n"
    "upgrade get\n"
    "    - get upgrade state\n"
    "upgrade set <state>\n"
    "    - set upgrade state\n"
);

#if !defined(CONFIG_PRODUCTION)
U_BOOT_CMD(
    i2c,    8,  1,  do_i2c,
    "i2c     - I2C test commands\n",
    "\n"
    "i2c reset\n"
    "    - reset cpu i2c\n"
    "i2c bw <address> <value>\n"
    "    - write byte to i2c address\n"
    "i2c br <address>\n"
    "    - read byte from i2c address\n"
    "i2c dw <address> <reg> <data> [checksum]\n"
    "    - write data to i2c address\n"
    "i2c dr <address> <reg> <len>\n"
    "    - read data from i2c address\n"
    "i2c ew <address> <offset> <data> [hex | char]\n"
    "    - write data to EEPROM at i2c address\n"
    "i2c er <address> <offset> <len>\n"
    "    - read data from EEPROM at i2c address\n"
    "i2c ews <address> <offset> <data> [hex | char]\n"
    "    - write data to secure EEPROM at i2c address\n"
    "i2c ers <address> <offset> <len>\n"
    "    - read data from secure EEPROM at i2c address\n"
    "i2c ewo <address> <offset> [secure]\n"
    "    - write byte to offset (auto-incrementing) of the device EEPROM\n"
    "i2c mux [enable|disable] <mux addr> <channel>\n"
    "    - enable/disable mux channel\n"
    "i2c temperature [<delay>] [<loop>] [<threshold>]\n"
    "    - i2c temperature reading delay #us (100000) for Winbond read temp\n"
    "i2c voltage\n"
    "    - i2c voltage reading for Winbond\n"
    "i2c fan [<delay> |<fan> duty <percent>]\n"
    "    - i2c fan test interval delay #ms (1000ms) or individual fan duty\n"
    "i2c sfp temp\n"
    "    - uplink SFP temperature\n"
    "i2c sfp eeprom <module> <offset> <len>\n"
    "    - uplink SFP eeprom read\n"
);

U_BOOT_CMD(
    syspld,   7,  1,  do_syspld,
    "syspld    - SYSPLD test commands\n",
    "\n"
    "syspld register read <address>\n"
    "    - read SYSPLD register\n"
    "syspld register write <address> <value>\n"
    "    - write SYSPLD register\n"
    "syspld register dump\n"
    "    - dump SYSPLD registers\n"
    "syspld reset [asert | clear] <number>\n"
    "    - syspld reset [asert | clear]\n"  
    "      1. CHJ2_CPU_2\n"
    "      2. CHJ2_SW_2\n"
    "      3. CHJ2_CPU_1\n"
    "      4. CHJ2_SW_1\n"
    "      5. PLD_OT\n"
    "      6. SYS_RESET_HOLD\n"
    "      7. SYS_RESET\n"
    "syspld device [enable | disable] <number>\n"
    "    - syspld device [enable | disable]\n"  
    "      1. POE_CONTROLLER\n"
    "      2. MGMT_PHY\n"
    "      3. USB_HUB\n"
    "      4. NAND_FLASH_CONTROLLER\n"
    "      5. USB_HUB_VCC\n"
    "      6. MAC_EEPROM_VCC\n"
    "syspld led port <num> <link> <duplex> <speed>\n"
    "    - syspld port (0..) [up | down] [full | half] [10M | 100M | 1G]\n"
    "syspld led mgmt <link> <duplex> <speed>\n"
    "    - syspld port [up | down] [full | half] [10M | 100M]\n"
    "syspld led set <led> <color> <state>\n"
    "    - syspld led set [sys|alm] [off|red|green] [steady|slow|fast]\n"
    "syspld led [enable|disable|update]\n"
    "    - disable/enable/update sys and alm leds\n"
);

U_BOOT_CMD(
    rtc,	4,	1,	do_rtc,
    "rtc     - get/set/reset date & time\n",
    "\n"
    "rtc\n"
    "    - rtc get time\n"  
    "rtc reset\n"
    "    - rtc reset time\n"  
    "rtc [init | start |stop]\n"
    "    - rtc start/stop\n"  
    "rtc set YYMMDD hhmmss\n"
    "    - rtc set time YY/MM/DD hh:mm:ss\n"  
    "rtc status\n"
    "    - rtc status\n"  
    "rtc dump\n"
    "    - rtc register dump\n"  
);

U_BOOT_CMD(
    wd,   4,  1,  do_wd,
    "wd      - watchdog commands\n",
    "\n"
    "wd cpu init <value>\n"
    "    - cpu watchdog init with reload value in milliseconds\n"
    "wd cpu [enable | disable | reset]\n"
    "    - cpu watch enable/disable/reset\n"
    "wd syspld [enable | disable | toggle]\n"
    "    - syspld watch enable/disable/toggle\n"
);

U_BOOT_CMD(
    poe,    6,  1,  do_poe,
    "poe     - poe test commands\n",
    "\n"
    "poe [on|off]\n"
    "    - poe on/off\n"
    "poe [enable|disable] legacy\n"
    "    - poe enable/disable legacy mode\n"
    "poe [enable|disable] DCdisconnect\n"
    "    - poe enable/disable DC disconnect\n"
    "poe [enable|disable] [<port>|all]\n"
    "    - poe enable/disable port 0-47\n"
    "poe PM [<PM1><PM2><PM3>]\n"
    "    - poe PM method\n"
    "poe status [<port>]\n"
    "    - poe single port status or system status\n"
    "poe param <device> [<IC><TSH>[MIP-AT|AF]]\n"
    "    - poe device 0-7 status status\n"
    "poe limit <port> [<limit>]\n"
    "    - poe single port power limit\n"
    "poe priority <port> [<priority>]\n"
    "    - poe single port priority, 1-critical, 2-high, 3-low(default)\n"
    "poe measurement <port>\n"
    "    - poe single port momentary electrical parameters\n"
    "poe mask <mask-key> [<mask>]\n"
    "    - poe individual mask\n"
    "poe setting [save|default]\n"
    "    - poe system setting command\n"
    "poe reset\n"
    "    - poe reset command\n"
    "poe read\n"
    "    - poe read status\n"
    "poe command <data>\n"
    "    - poe command\n"
    "poe temperature [<delay>]\n"
    "    - poe device temperature\n"
);

U_BOOT_CMD(
    si,    7,    1,     do_si,
    "si      - signal integrate\n",
    "\n"
    "si mpp\n"
    "    - mpp register setting\n"
    "si gpi\n"
    "    - gpio register setting\n"
    "si gpo [device [reset | clear] <number>]\n"
    "    - gpo [reset | clear] or setting\n"  
    "      1. W38782G_H\n"
    "      2. I2C_L\n"
    "      3. SYSPLD_I2C_L\n"
    "      4. EEPROM_L\n"
    "si pattern <pattern 1> <pattern 2> <start addr> <end addr> [<count>]\n"
    "    - ram 32 bits pattern burst write/read\n"
    "si pwrite <pattern 1> <pattern 2> <start addr> <end addr> [<count>]\n"
    "    - ram 32 bits pattern burst write\n"
    "si pread <pattern 1> <pattern 2> <start addr> <end addr> [<count>]\n"
    "    - ram 32 bits pattern burst read\n"
);

U_BOOT_CMD(
	runloop,	CFG_MAXARGS,	1,	do_runloop,
	"runloop - run commands in an environment variable\n",
	"var [...]\n"
	"    - run the commands in the env variable(s) 'var' infinite loop\n"
);

U_BOOT_CMD(
    mfgcfg,    3,    0,     do_mfgcfg,
    "mfgcfg  - manufacturing config of EEPROMS\n",
    "\nmfgcfg system [read|update|clear]\n"
);

U_BOOT_CMD(
	msleep ,    2,    2,     do_msleep,
	"msleep  - delay execution for msec\n",
	"\n"
	"    - delay execution for N milli seconds (N is _decimal_ !!!)\n"
);

U_BOOT_CMD(
	usleep ,    2,    2,     do_usleep,
	"usleep  - delay execution for usec\n",
	"\n"
	"    - delay execution for N micro seconds (N is _decimal_ !!!)\n"
);

U_BOOT_CMD(
	icache,   2,   1,     do_icache,
	"icache  - enable or disable instruction cache\n",
	"icache[on|off]\n"
	"    - enable or disable instruction cache\n"
);

U_BOOT_CMD(
	dcache,   2,   1,     do_dcache,
	"dcache  - enable or disable data cache\n",
	"dcache [on|off]\n"
	"    - enable or disable data (writethrough) cache\n"
);

#if defined(CONFIG_SPI_SW_PROTECT)
U_BOOT_CMD(
    sprotect, 4, 1, do_sprotect,
    "sprotect - spi flash protect\n",
    "\n"
    "sprotect [on|off|get] <start> <end>\n"
    "    - get or set spi protect address <start> to <end>\n"
);
#endif
#endif
