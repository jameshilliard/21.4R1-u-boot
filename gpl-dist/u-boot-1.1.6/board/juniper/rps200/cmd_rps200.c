/*
 * Copyright (c) 2008-2012, Juniper Networks, Inc.
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

#include <i2c.h>
extern int i2c_slave_write_wait(uint8_t slave, uint8_t *pBlock, 
                        uint32_t blockSize, uint32_t *pSize, uint32_t iwait);
extern int i2c_slave_read_wait(uint8_t slave, uint8_t *pBlock, 
                               uint32_t blockSize, uint32_t iwait);
extern void i2c_cpld_reset(void);
extern void watchdog_init(uint32_t ms);
extern void watchdog_disable(void);
extern void watchdog_enable(void);
extern void watchdog_reset(void);

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

    return (res);
}

static int 
twos_complement (uint8_t temp)
{
    return (int8_t)temp;
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

int 
i2c_mux (uint8_t mux, uint8_t chan, int ena)
{
    uint8_t ch;
    int ret = 0;

    if (ena) {
        ch = 1 << chan;
        ret = i2c_write(0, mux, 0, 0, &ch, 1);
    }
    else {
        ch = 0;
        ret = i2c_write(0, mux, 0, 0, &ch, 1);
    }

    return ret;
}

/* I2C commands
 *
 * Syntax:
 */
int 
do_i2c (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char cmd1;
    ulong mux = 0, device = 0, channel = 0;
    unsigned char ch, bvalue;
    uint regnum = 0;
    int i=0, ret, reg = 0, temp, len = 0, offset = 0;
    char tbyte[3];
    uint8_t tdata[2048];
    unsigned int length = 0, iwait = 0;
    char *data;
    uint16_t cksum = 0;
    uint8_t slave;

    if (argc < 2)
        goto usage;
    
    cmd1 = argv[1][0];
    
    switch (cmd1) {
        case 'b':   /* byte read/write */
        case 'B':
            device = simple_strtoul(argv[2], NULL, 16);
            if ((argv[1][1] == 'r') ||(argv[1][1] == 'R')) {
                bvalue = 0xff;
                if ((ret = i2c_read(0, device, 0, 0, &bvalue, 1)) != 0) {
                    printf("i2c failed to read from device 0x%x.\n", device);
                    return 0;
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
                    return 0;
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
                    ch = 1<<channel;
                    if ((ret = i2c_write(0, mux, 0, 0, &ch, 1)) != 0) {
                        printf("i2c failed to enable mux 0x%x channel %d.\n",
                               mux, channel);
                    }
                    else
                        printf("i2c enabled mux 0x%x channel %d.\n",
                        mux, channel);
                }
                else {
                    ch = 0;
                    if ((ret = i2c_write(0, mux, 0, 0, &ch, 1)) != 0) {
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
                    if ((ret = i2c_read
                        (0, device, regnum, reg, tdata, len)) != 0) {
                        printf ("I2C read from device 0x%x failed.\n", device);
                    }
                    else {
                        printf ("The data read - \n");
                        for (i=0; i< len; i++)
                            printf("%02x ", tdata[i]);
                        printf("\n");
                    }
                }
            }
            else {  /* write */
                data = argv [4];
                len = strlen(data)/2;
                tbyte[2] = 0;
                for (i=0; i<len; i++) {
                    tbyte[0] = data[2*i];
                    tbyte[1] = data[2*i+1];
                    temp = atoh(tbyte);
                    tdata[i] = temp;
                }
                if ((argv[5][0] == 'c') ||(argv[5][0] == 'C')) {
                    for (i=0; i<len; i++) {
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
                    if ((ret = eeprom_read(device, offset, tdata, len)) != 0) {
                        printf ("I2C read from EEPROM device 0x%x failed.\n",
                                device);
                    }
                    else {
                        printf ("The data read from offset - \n", offset);
                        for (i=0; i< len; i++)
                            printf("%02x ", tdata[i]);
                        printf("\n");
                    }
                }
            }
            else {  /* write */
                data = argv [4];
                if ((argv[5][0] == 'h') || (argv[5][0] == 'H')) {
                    len = strlen(data)/2;
                    tbyte[2] = 0;
                    for (i=0; i<len; i++) {
                        tbyte[0] = data[2*i];
                        tbyte[1] = data[2*i+1];
                        temp = atoh(tbyte);
                        tdata[i] = temp;
                    }
                }
                else {
                    len = strlen(data);
                    for (i=0; i<len; i++) {
                        tdata[i] = data[i];
                    }
                }
                if ((ret = eeprom_write(device, offset, tdata, len)) != 0)
                    printf ("I2C write to EEPROM device 0x%x failed.\n",
                            device);
            }
                        
            break;
            
        case 's':   /* slave */
        case 'S':
            if (argc <= 2)
                goto usage;

            slave = simple_strtoul(argv[2], NULL, 16);
            iwait = simple_strtoul(argv[3], NULL, 10);
            if ((argv[1][1] == 'w') ||(argv[1][1] == 'W')) {  /* read */
                if ((ret = i2c_slave_write_wait
                    (slave, tdata, 2048, &length, iwait)) == 0) {
                    if (length) {
                        printf ("I2c slave received  - \n");
                        for (i=0; i< length; i++)
                            printf("%02x ", tdata[i]);
                        printf("\n");
                    }
                }
                else
                    printf ("I2C slave receive failed [0x%x].\n", ret);
            }
            else {  /* read */
                data = argv [4];
                length = strlen(data)/2;
                tbyte[2] = 0;
                for (i=0; i<length; i++) {
                    tbyte[0] = data[2*i];
                    tbyte[1] = data[2*i+1];
                    temp = atoh(tbyte);
                    tdata[i] = temp;
                }
                if ((ret = i2c_slave_read_wait(slave, tdata, length, iwait)) != 0)
                    printf ("I2C slave send reply failed [0x%x].\n", ret);
            }
                        
            break;
            
        case 'r':          /* read/reset */
        case 'R':
            if (argc <= 2)
                goto usage;

            if ((argv[2][0] == 'c') || (argv[2][0] == 'C'))
                i2c_reset();
            else
                i2c_cpld_reset();
            break;
            
        default:
            printf("???");
            break;
    }

    return 1;
 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return 1;
}


int 
cpld_reg_read (int reg, uint8_t *val)
{
    uint8_t temp;
    uint8_t ch = 1 << 6;

    i2c_write(0, CFG_I2C_PCA9548_ADDR, 0, 0, &ch, 1);
    if (i2c_read(0, CFG_I2C_SYSPLD_ADDR, reg, 1, &temp, 1)) {
        ch = 0;
        i2c_write(0, CFG_I2C_PCA9548_ADDR, 0, 0, &ch, 1);
        return -1;
    }
    
    ch = 0;
    i2c_write(0, CFG_I2C_PCA9548_ADDR, 0, 0, &ch, 1);

    *val = temp;
    return 0;
}

int 
cpld_reg_write (int reg, uint8_t val)
{
    uint8_t temp = val;
    uint8_t ch = 1 << 6;
        
    i2c_write(0, CFG_I2C_PCA9548_ADDR, 0, 0, &ch, 1);
    if (i2c_write(0, CFG_I2C_SYSPLD_ADDR, reg, 1, &temp, 1)) {
        ch = 0;
        i2c_write(0, CFG_I2C_PCA9548_ADDR, 0, 0, &ch, 1);
        return -1;
    }
    
    ch = 0;
    i2c_write(0, CFG_I2C_PCA9548_ADDR, 0, 0, &ch, 1);
    
    return 0;
}


int 
cpld_read (int offset, uint8_t *data, int length)
{
    uint8_t ch = 1 << 6;
    
    i2c_write(0, CFG_I2C_PCA9548_ADDR, 0, 0, &ch, 1);
    if (i2c_read(0, CFG_I2C_SYSPLD_ADDR, offset, 1, data, length)) {
        ch = 0;
        i2c_write(0, CFG_I2C_PCA9548_ADDR, 0, 0, &ch, 1);
        return -1;
    }

    ch = 0;
    i2c_write(0, CFG_I2C_PCA9548_ADDR, 0, 0, &ch, 1);
    
    return 0;
}

void 
cpld_dump (void)
{
    int i;
    uint8_t temp[100];

    cpld_read(0, temp, 0x36);

    for (i=0; i<0x36; i++) {
        printf("CPLD addr 0x%02x = 0x%02x.\n", i, temp[i]);
    }
    printf("\n");
}

int 
cpld_nand_write (int enable)
{
    uint8_t data;

    cpld_reg_read(0xd, &data);
    data = enable ? (data | 0x1) : (data & (0xFE));
    cpld_reg_write(0xd, data);
    
    return 0;
}

int
do_cpld (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int addr = 0;
    uint8_t value = 0;
    char cmd1, cmd2;

    if (argc <= 2)
        goto usage;
    
    cmd1 = argv[1][0];
    cmd2 = argv[2][0];
    
    switch (cmd1) {
        case 'r':       /* Register */
        case 'R':
            switch (cmd2) {
                case 'd':   /* dump */
                case 'D':
                    cpld_dump();
                    break;
                    
                case 'r':   /* read */
                case 'R':
                    if (argc <= 3)
                        goto usage;
                    
                    addr = simple_strtoul(argv[3], NULL, 16);
                    if (cpld_reg_read(addr, &value))
                        printf("CPLD addr 0x%02x read failed.\n", addr);
                    else
                        printf("CPLD addr 0x%02x = 0x%02x.\n", addr, value);
                    break;
                    
                case 'w':   /* write */
                case 'W':
                    if (argc <= 4)
                        goto usage;
                    
                    addr = simple_strtoul(argv[3], NULL, 16);
                    value = simple_strtoul(argv[4], NULL, 16);
                    if (cpld_reg_write(addr, value))
                        printf("CPLD write addr 0x%02x value 0x%02x failed.\n",
                                addr, value);
                    else {
                        if (cpld_reg_read(addr, &value))
                            printf("CPLD addr 0x%02x read back failed.\n",
                                   addr);
                        else
                            printf("CPLD read back addr 0x%02x = 0x%02x.\n",
                                   addr, value);
                    }
                    break;
                    
                default:
                    printf("???");
                    break;
            }
            
            break;
            
        case 'n':   /* nand write protect */
        case 'N':
            if (argc <= 3)
                goto usage;

            if ((cmd2=='w') || (cmd2=='W'))
                ;
            else
                goto usage;
            
            if ((argv[3][0]=='e') || (argv[3][0]=='E')) {
                cpld_nand_write(1);
            }
            else if ((argv[3][0]=='d') || (argv[3][0]=='D')) {
                cpld_nand_write(0);
            }
            else
                goto usage;

            break;
           
        default:
            printf("???");
            break;
    }

    return 1;
 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return 1;
}


int 
do_wd (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uint32_t value = 0;
    char cmd1, cmd2;

    if (argc < 2)
        goto usage;
    
    cmd1 = argv[1][0];
    cmd2 = argv[2][0];
    
    switch (cmd1) {
        case 'i':       /* init */
        case 'I':
            if (argc < 3)
                goto usage;

            value = simple_strtoul(argv[2], NULL, 10);
            watchdog_init(value);
            
            break;
            
        case 'e':   /* enable */
        case 'E':
            watchdog_enable();
            
            break;
            
        case 'd':   /* disable */
        case 'D':
            watchdog_disable();
            
            break;
            
        case 'r':   /* reset */
        case 'R':
            watchdog_reset();
            
            break;
            
        default:
            printf("???");
            break;
    }

    return 1;
 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return 1;
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
 { "EX-RPS-PWR"     , I2C_ID_EX200_RPS, "N/A       " },
 {  NULL            , 0                          }
};

assm_id_type assm_id_list_system [] = {
 { "EX-RPS-PWR"     , I2C_ID_EX200_RPS, "N/A       " ,0x40},
 {  NULL             , 0                          }
};

int 
isxdigit (int c)
{
    if ('0' <= c && c <= '9') return 1;
    if ('A' <= c && c <= 'Z') return 1;
    if ('a' <= c && c <= 'z') return 1;
    return 0;
}

int valid_days [] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

int
is_leap(int year)
{
    if (year<1 || year>9999) {
        return -1;
    }

    /* leap adjusted for years 1-1700 */
    return ( year%4 == 0 && (year%100 != 0 || year<=1700) || year%400 == 0 );
}

int 
do_mfgcfg (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    assm_id_type *assm_id_list = assm_id_list_system;
    ulong    mux        = CFG_I2C_PCA9548_ADDR, 
                device     = CFG_I2C_ID_EEPROM_ADDR, 
                channel    = CFG_I2C_ID_EEPROM_CHAN;

    int         ret, 
                assm_id    = 0;

    int         i,j,k,nbytes;

    unsigned    int     start_address = 0;

    uint8_t     tdata[0x80];
    char        tbyte[3];
    int day, mon, year;
    char temp[3][5];

    if (argc < 2)
    {   printf ("Usage:\n%s\n", cmdtp->usage);
        return 1;
    }
    
    if (strcmp(argv[1],"system")) 
    {   printf ("Usage:\n%s\n", cmdtp->usage);
        return 1;
    }

    if ((ret = i2c_mux(mux, channel, 1)) != 0) {
        printf("i2c failed to enable mux 0x%x channel 0x%x.\n", mux, channel);
        i2c_mux(mux, 0, 0);
        return 1;
    }

    if (eeprom_read (device, start_address, tdata, 0x80)) {
        printf ("ERROR reading eeprom at device 0x%x\n", device);
        i2c_mux(mux, 0, 0);
        return 1;
    }
    
    for (i=0; i < 0x80; i+=8)
    {
        printf("%02X : ",i + start_address);
        for (j=0; j<8; j++)
           printf("%02X ",tdata[i+j]);
        printf("\n");
    }   
    
    printf ("Start address (Hex)= 0x%04X\n", start_address);    
    assm_id   = (tdata[4]<<8) + tdata[5];
    printf ("Assembly Id (Hex)= 0x%04X\n", assm_id);
    for (i=0; assm_id_list[i].name; i++)
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
//            tdata[0x06] = 1;
            tdata[0x2c] = 0;
            tdata[0x44] = 1;
            memset (tdata+0X74, 0x00, 12);

            printf("List of assemblies:\n");
            for (i=0; assm_id_list[i].name; i++)
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
                for (i=0; assm_id_list[i].name; i++)
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
                {   // enter Rev separate
                    nbytes = readline ("Enter Rev: ");
                    strncpy (tdata+0x0C, console_buffer,8);
                    tdata[0x07] = simple_strtoul (console_buffer, NULL, 10);
                }
            }

            // only ask for Model number/rev for FRU 
            if (assm_id_list[i].name && strncmp(assm_id_list[i].clei, "N/A", 10))
            {
                int model_name_size;
                printf("\n"
                       "Assembly Model Name   : '%.23s'\n " ,tdata+0x4f); 
                printf("Assembly Model Rev    : '%.3s'\n"   ,tdata+0x66); 
                model_name_size = strlen (assm_id_list[i].name);
                nbytes = readline ("Enter Assembly Model Name: ");
                if (((nbytes >= model_name_size) && nbytes)) 
                {
                    strncpy (tdata+0x4f, console_buffer,model_name_size);
                    tdata [0x4f+model_name_size] = 0;
                  
                    if (nbytes >= (model_name_size + 5) && (!strncmp(console_buffer + model_name_size, " REV ",5)
                        || !strncmp(console_buffer + model_name_size, " Rev ", 5) ))
                        strncpy (tdata+0x66, console_buffer + model_name_size + 5, 3);
                    else
                    {    // enter Rev separate
                        nbytes = readline ("Enter Rev: ");
                        strncpy (tdata+0x66, console_buffer,3);
                    }
                }
            } 
            else 
                tdata[0x44] = 0;     //not a FRU 
            
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
                for (j=0; j<6; j++)
                    printf("%02X",tdata[0x38+j]);
                
                nbytes = readline ("\nEnter Base MAC: ");
                for (i=j=0; console_buffer[i]; i++)
                    if (isxdigit(console_buffer[i]))
                        console_buffer[j++] = console_buffer[i];
                console_buffer[j] = 0;
                nbytes = j;
                
                if (nbytes)
                {   
                    if (nbytes==12)
                    {
                        tbyte[2] = 0;
                        for (i=0; i<6; i++)
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
                for (i=0; assm_id_list[i].name; i++)
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

retry_date:
            year = *(tdata+0x2f) << 8;
            year = year | (*(tdata+0x30));
            printf("\nAssembly Date: '%d/%d/%d'\n", *(tdata+0x2e), *(tdata+0x2d), year);
            nbytes = readline ("Enter Date (MM/DD/YYYY): ");       
            if (nbytes) {
                for (i = 0; i < 3; i++) {
                    for (j = 0; j < 5; j++)
                        temp[i][j] = 0;
                }
                j = k = 0;
                for (i = 0; i < nbytes; i++) {
                    if (console_buffer[i] == '/') {
                        j++;
                        k = 0;
                        continue;
                    }
                    temp[j][k] = console_buffer[i];
                    k++;
                }
                mon = atod(temp[0]);
                day = atod(temp[1]);
                year = atod(temp[2]);
                if (mon>12 || mon < 1) {
                    printf("Invalid month in date\n");

                    goto retry_date;
                }

                if (day > (valid_days[mon-1]
                            + (is_leap(year) > 0 )) || day < 1) {
                    printf("Invalid day in date\n");

                    goto retry_date;
                }

                if (year > 2099 || year < 2007) {
                    printf("Invalid year in date\n");

                    goto retry_date;
                }

                tdata[0x2d] = day;
                tdata[0x2e] = mon;
                tdata[0x30] = (char)(year & 0xff);
                tdata[0x2f] = (char)((year >> 8)& 0xff) ;
            }
            
             // prog clei
            for (i=0; assm_id_list[i].name; i++)
                if (assm_id_list[i].id == assm_id)
                    break;
                
            if (strncmp(assm_id_list[i].clei, "N/A       ", 10)) {
                strncpy(tdata+0x45, assm_id_list[i].clei, 10);
            } else {
                printf("\nCLEI code: '%.10s'\n ", tdata+0x45);
                nbytes = readline ("Enter CLEI code: ");       
                // Some OEM SKU's may not require CLEI - so allow N/A
                if ((nbytes == 10) || !strncmp(console_buffer, "N/A", 3)) {
                    strncpy (tdata+0x45, console_buffer, 10);
                } else {
                    strncpy(tdata+0x45, assm_id_list[i].clei, 10);
                }
            }

            // calculate checksum
            for (j=0,i=0x44; i<=0x72; i++)
               j += tdata[i];
            tdata[0x73] = j;
            for (i = 0; i < 0x80; i += 8) {
                eeprom_write (device, start_address + i, &tdata[i], 8);
                udelay(5000);
            }
        }
        else if (!strcmp(argv[2],"clear"))
        {
            memset (tdata, 0xFF, 0x80);
            for (i = 0; i < 0x80; i += 8) {
                eeprom_write (device, start_address + i, &tdata[i], 8);
                udelay(5000);
            }
        }       

        // always reread
        eeprom_read (device, start_address, tdata, 0x80);
        for (i=0; i < 0x80; i+=8)
        {
            printf("%02X : ",i + start_address);
            for (j=0; j<8; j++)
               printf("%02X ",tdata[i+j]);
            printf("\n");
        }   
    } 
    i2c_mux(mux, 0, 0);

    assm_id   = (tdata[4]<<8) + tdata[5];
    for (i=0; assm_id_list[i].name; i++)
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
        year = *(tdata+0x2f) << 8;
        year = year | (*(tdata+0x30));
        printf("Assembly Date         : '%d/%d/%d'\n", *(tdata+0x2e), *(tdata+0x2d), year);
        if (!strcmp(argv[1],"system")) 
        {  
            printf("Base MAC              : ");  
            for (j=0; j<6; j++)
                printf("%02X",tdata[0x38+j]);
        }
        printf("\n"
               "Deviation Info        : '%.10s'\n"   ,tdata+0x69); 
    }

    return 1;
}

/***************************************************/


U_BOOT_CMD(
    i2c,    8,  1,  do_i2c,
    "i2c     - I2C test commands\n",
    "\n"
    "i2c reset [cpu | other]\n"
    "    - reset i2c mux and I/O expander\n"
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
    "i2c sw <slave> <wait>\n"
    "    - slave wait and read data from i2c address\n"
    "i2c sr <slave> <wait> <data>\n"
    "    - slave wait and send/reply data to i2c address\n"
    "i2c mux [enable|disable] <mux addr> <channel>\n"
    "    - enable/disable mux channel\n"
);

U_BOOT_CMD(
    cpld,   5,  1,  do_cpld,
    "cpld    - CPLD test commands\n",
    "\n"
    "cpld register read <address>\n"
    "    - read CPLD register\n"
    "cpld register write <address> <value>\n"
    "    - write CPLD register\n"
    "cpld register dump\n"
    "    - dump CPLD registers\n"
    "cpld nand write [enable | disable]\n"
    "    - CPLD NAND write enable/disable\n"
);

U_BOOT_CMD(
    wd,   3,  1,  do_wd,
    "wd      - watchdog commands\n",
    "\n"
    "wd init <value>\n"
    "    - watchdog init with reload value in milliseconds\n"
    "wd [enable | disable | reset]\n"
    "    - watch enable/disable/reset\n"
);

U_BOOT_CMD(
    mfgcfg,    3,    0,     do_mfgcfg,
    "mfgcfg  - manufacturing config of EEPROMS\n",
    "\nmfgcfg system [read|update|clear]\n"
);

