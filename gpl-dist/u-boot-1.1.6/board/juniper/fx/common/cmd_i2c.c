/*
 * Copyright (c) 2009-2010, Juniper Networks, Inc.
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

#include <common.h>
#include <command.h>
#include <asm/byteorder.h>

#include "fx_common.h"
#include "i2c.h"

#define MAX_I2C_BUS    1

/* Display values from last command.
 * Memory modify remembered values are different from display memory.
 */
static uchar i2c_dp_last_chip;
static uint i2c_dp_last_addr;
static uint i2c_dp_last_alen;
static uint i2c_dp_last_length = 0x10;
static uint i2c_dp_last_bus = 0;

static uchar i2c_mm_last_chip;
static uint i2c_mm_last_addr;
static uint i2c_mm_last_alen;

#if defined (CFG_I2C_NOPROBES)
static uchar i2c_no_probes[] = CFG_I2C_NOPROBES;
#endif

static int
mod_i2c_mem(cmd_tbl_t *cmdtp, int incrflag, int flag, int argc, char *argv[]);
extern int cmd_get_data_size(char*arg, int default_size);

/*
 * Syntax:
 *	imd {bus} {i2c_chip} {addr}{.0, .1, .2} {len}
 */
#define DISP_LINE_LEN    16

static int
do_i2c_md (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    u_char chip,tot_buffer[256]={0};
    uint bus, addr, alen = 1, length, arg_cnt = 1;
    int i=0,j=0, nbytes, linebytes;

    if (argc < 3 && argc > 6) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return 1;
    }

    /* We use the last specified parameters, unless new ones are
     * entered.
     */
    bus    = i2c_dp_last_bus;
    chip   = i2c_dp_last_chip;
    addr   = i2c_dp_last_addr;
    alen   = i2c_dp_last_alen;
    length = i2c_dp_last_length;


    if (argc == 5) {
        bus = simple_strtoul(argv[1], NULL, 16);

        if (bus > MAX_I2C_BUS) {
            printf("\nOnly %d i2c buses defined, defaulting to bus 0");
            bus = 0;
        }

        arg_cnt = 2;
    }

    if ((flag & CMD_FLAG_REPEAT) == 0) {
        /*
         * New command specified.
         */
        alen = 1;

        /*
         * I2C chip address
         */
        chip = simple_strtoul(argv[arg_cnt], NULL, 16);

        /*
         * I2C data address within the chip.  This can be 1 or
         * 2 bytes long.  Some day it might be 3 bytes long :-).
         */
        addr = simple_strtoul(argv[arg_cnt + 1], NULL, 16);
        alen = 1;
        for (j = 0; j < 8; j++) {
            if (argv[arg_cnt + 1][j] == '.') {
                alen = argv[arg_cnt + 1][j + 1] - '0';
                if (alen > 4) {
                    printf("Usage:\n%s\n", cmdtp->usage);
                    return 1;
                }
                break;
            } else if (argv[arg_cnt + 1][j] == '\0') {
                break;
            }
        }

        /*
         * If another parameter, it is the length to display.
         * Length is the number of objects, not number of bytes.
         */
        if (argc > 3) {
            length = simple_strtoul(argv[arg_cnt + 2], NULL, 16);
        }
    }


    /*
     * Print the lines.
     *
     * We buffer all read data, so we can make sure data is read only
     * once.
     */
    nbytes = length;

    i2c_dp_last_addr = addr;

    if (alen > 1){
        if (i2c_seq_rd(bus, chip, addr, alen, tot_buffer, nbytes) != 0) {
            printf("Error in sequentially reading the chip 0x%x on i2c_bus 0x%x\n", chip, bus); 
            return -1;
        }else{
            i = 0;
            do {    
                unsigned char *cp;

                linebytes = (nbytes > DISP_LINE_LEN) ? DISP_LINE_LEN : nbytes;
                cp = &tot_buffer[i];

                for (j = 0; j < linebytes; j++) {
                    printf(" %02x", *cp++);
                    addr++;
                }
                puts("    ");
                cp = &tot_buffer[i];
                for (j = 0; j < linebytes; j++) {
                    if ((*cp < 0x20) || (*cp > 0x7e)) {
                        puts(".");
                    } else {
                        printf("%c", *cp);
                    }
                    cp++;
                }
                putc('\n');

                nbytes -= linebytes;
                i += linebytes;
            } while (nbytes > 0);

            return 0; 
        }
    } else {

        do {
            unsigned char linebuf[DISP_LINE_LEN];
            unsigned char *cp;

            linebytes = (nbytes > DISP_LINE_LEN) ? DISP_LINE_LEN : nbytes;

            if (i2c_read(bus, chip, addr,linebuf, linebytes) != 0) {
                printf("Error reading the chip 0x%x on i2c_bus 0x%x.\n", chip, bus);
                return -1;
            }

            cp = linebuf;

            for (j = 0; j < linebytes; j++) {
                printf(" %02x", *cp++);
                addr++;
            }
            puts("    ");
            cp = linebuf;
            for (j = 0; j < linebytes; j++) {
                if ((*cp < 0x20) || (*cp > 0x7e)) {
                    puts(".");
                } else {
                    printf("%c", *cp);
                }
                cp++;
            }
            putc('\n');

            nbytes -= linebytes;
        } while (nbytes > 0);
    }

    i2c_dp_last_bus    = bus;
    i2c_dp_last_chip   = chip;
    i2c_dp_last_addr   = addr;
    i2c_dp_last_alen   = alen;
    i2c_dp_last_length = length;

    return 0;
}


/* Write (fill) memory
 *
 * Syntax:
 *	imw {bus} {i2c_chip} {addr}{.0, .1, .2} {data} [{count}]
 */
static int
do_i2c_mw (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uchar chip;
    ulong addr;
    uint alen = 1, bus = 0, arg_cnt = 1;
    uchar byte;
    int count;
    int i=0,j=0;

    if ((argc < 4) || (argc > 6)) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return 1;
    }

    if (argc == 6) {
        bus = simple_strtoul(argv[1], NULL, 16);

        if (bus > MAX_I2C_BUS) {
            bus = 0;
            printf("\nOnly %d i2c buses defined, defaulting to bus 0");
        }

        arg_cnt = 2;
    }


    /*
     * Chip is always specified.
     */
    chip = simple_strtoul(argv[arg_cnt], NULL, 16);

    /*
     * Address is always specified.
     */
    addr = simple_strtoul(argv[arg_cnt + 1], NULL, 16);
    alen = 1;
    for (j = 0; j < 8; j++) {
        if (argv[arg_cnt + 1][j] == '.') {
            alen = argv[arg_cnt + 1][j + 1] - '0';
            if (alen > 4) {
                printf("Usage:\n%s\n", cmdtp->usage);
                return 1;
            }
            break;
        } else if (argv[arg_cnt + 1][j] == '\0') {
            break;
        }
    }

    /*
     * Value to write is always specified.
     */
    byte = simple_strtoul(argv[arg_cnt + 2], NULL, 16);

    /*
     * Optional count
     */
    if ((arg_cnt == 2) && (argc == 6)) {
        count = simple_strtoul(argv[arg_cnt + 3], NULL, 16);
    } else {
        count = 1;
    }

#ifdef I2C_SEQ_WR
    /*
       printf("[0x%x 0x%x 0x%x 0x%x]", chip, addr,count,alen);
       printf("Byte being written is 0x%x - %d times\n", byte,count);
     */

    if (alen > 1) {
        if (i2c_seq_wr(bus, chip, addr, alen, tot_buffer, count) == -1) {
            printf("Error writing sequentially to chip 0x%x on i2c_bus 0x%x.\n", chip, bus); 
            return -1;
        } else {
            printf("Sequential write successful\n");
        }
        return 0;
    }else
#endif
    {
        if (i2c_write_fill(bus, chip, addr, &byte, count) != 0) {
            printf("Error writing the chip 0x%x at offset 0x%x on i2c_bus 0x%x.\n",
                    chip,
                    addr,
                    bus);
        }
    }

    /*
     * Wait for the write to complete.  The write can take
     * up to 10mSec (we allow a little more time).
     *
     * On some chips, while the write is in progress, the
     * chip doesn't respond.  This apparently isn't a
     * universal feature so we don't take advantage of it.
     */
    /*
     * No write delay with FRAM devices.
     */
#if !defined (CFG_I2C_FRAM)
    udelay(11000);
#endif

    return 0;
}


/* Calculate a CRC on memory
 *
 * Syntax:
 *	icrc32 {i2c_chip} {addr}{.0, .1, .2} {count}
 */
static int
do_i2c_crc (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uchar chip;
    ulong addr;
    uint alen, bus = 0;
    int count;
    uchar byte;
    ulong crc;
    ulong err;
    int j;

    if (argc < 4) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return 1;
    }

    /*
     * Chip is always specified.
     */
    chip = simple_strtoul(argv[1], NULL, 16);

    /*
     * Address is always specified.
     */
    addr = simple_strtoul(argv[2], NULL, 16);
    alen = 1;
    for (j = 0; j < 8; j++) {
        if (argv[2][j] == '.') {
            alen = argv[2][j + 1] - '0';
            if (alen > 4) {
                printf("Usage:\n%s\n", cmdtp->usage);
                return 1;
            }
            break;
        } else if (argv[2][j] == '\0') {
            break;
        }
    }

    /*
     * Count is always specified
     */
    count = simple_strtoul(argv[3], NULL, 16);

    printf("CRC32 for %08lx ... %08lx ==> ", addr, addr + count - 1);
    /*
     * CRC a byte at a time.  This is going to be slooow, but hey, the
     * memories are small and slow too so hopefully nobody notices.
     */
    crc = 0;
    err = 0;
    while (count-- > 0) {
        if (i2c_read(bus, chip, addr, &byte, 1) != 0) {
            err++;
        }
        crc = crc32(crc, &byte, 1);
        addr++;
    }
    if (err > 0) {
        puts("Error reading the chip,\n");
    } else {
        printf("%08lx\n", crc);
    }

    return 0;
}


/* Modify memory.
 *
 * Syntax:
 *	imm{.b, .w, .l} {bus} {i2c_chip} {addr}{.0, .1, .2}
 *	inm{.b, .w, .l} {bus} {i2c_chip} {addr}{.0, .1, .2}
 */

static int
mod_i2c_mem (cmd_tbl_t *cmdtp, int incrflag, int flag, int argc, char *argv[])
{
    uchar chip;
    ulong addr;
    uint alen, bus = 0, arg_cnt = 1;
    ulong data;
    int size = 1;
    int nbytes;
    int j;
    extern char console_buffer[];

    if (argc != 3) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return 1;
    }


    if (argc == 4) {
        bus = simple_strtoul(argv[1], NULL, 16);

        if (bus > 1) {
            bus = 0;
        }

        arg_cnt = 1;
    }


#ifdef CONFIG_BOOT_RETRY_TIME
    reset_cmd_timeout();    /* got a good command to get here */
#endif
    /*
     * We use the last specified parameters, unless new ones are
     * entered.
     */
    chip = i2c_mm_last_chip;
    addr = i2c_mm_last_addr;
    alen = i2c_mm_last_alen;

    if ((flag & CMD_FLAG_REPEAT) == 0) {
        /*
         * New command specified.  Check for a size specification.
         * Defaults to byte if no or incorrect specification.
         */
        size = cmd_get_data_size(argv[0], 1);

        /*
         * Chip is always specified.
         */
        chip = simple_strtoul(argv[arg_cnt + 1], NULL, 16);

        /*
         * Address is always specified.
         */
        addr = simple_strtoul(argv[arg_cnt + 2], NULL, 16);
        alen = 1;
        for (j = 0; j < 8; j++) {
            if (argv[arg_cnt + 2][j] == '.') {
                alen = argv[arg_cnt + 2][j + 1] - '0';
                if (alen > 4) {
                    printf("Usage:\n%s\n", cmdtp->usage);
                    return 1;
                }
                break;
            } else if (argv[arg_cnt + 2][j] == '\0') {
                break;
            }
        }
    }

    /*
     * Print the address, followed by value.  Then accept input for
     * the next value.  A non-converted value exits.
     */
    do {
        printf("%08lx:", addr);
        if (i2c_read(bus, chip, addr, (uchar *)&data, size) != 0) {
            puts("\nError reading the chip,\n");
        } else {
            data = cpu_to_be32(data);
            if (size == 1) {
                printf(" %02lx", (data >> 24) & 0x000000FF);
            } else if (size == 2) {
                printf(" %04lx", (data >> 16) & 0x0000FFFF);
            } else {
                printf(" %08lx", data);
            }
        }

        nbytes = readline(" ? ");
        if (nbytes == 0) {
            /*
             * <CR> pressed as only input, don't modify current
             * location and move to next.
             */
            if (incrflag) {
                addr += size;
            }
            nbytes = size;
#ifdef CONFIG_BOOT_RETRY_TIME
            reset_cmd_timeout(); /* good enough to not time out */
#endif
        }
#ifdef CONFIG_BOOT_RETRY_TIME
        else if (nbytes == -2) {
            break;  /* timed out, exit the command	*/
        }
#endif
        else {
            char *endp;

            data = simple_strtoul(console_buffer, &endp, 16);
            if (size == 1) {
                data = data << 24;
            } else if (size == 2) {
                data = data << 16;
            }
            data = be32_to_cpu(data);
            nbytes = endp - console_buffer;
            if (nbytes) {
#ifdef CONFIG_BOOT_RETRY_TIME
                /*
                 * good enough to not time out
                 */
                reset_cmd_timeout();
#endif
                if (i2c_write(bus, chip, addr, (uchar *)&data, size) != 0) {
                    puts("Error writing the chip.\n");
                }
#ifdef CFG_EEPROM_PAGE_WRITE_DELAY_MS
                udelay(CFG_EEPROM_PAGE_WRITE_DELAY_MS * 1000);
#endif
                if (incrflag) {
                    addr += size;
                }
            }
        }
    } while (nbytes);

    chip = i2c_mm_last_chip;
    addr = i2c_mm_last_addr;
    alen = i2c_mm_last_alen;

    return 0;
}


/*
 * Syntax:
 *	iprobe [bus#] {addr}{.0, .1, .2}
 */
static int
do_i2c_probe (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    unsigned int j;

    uchar bus = 0;

    if (argc == 2) {
        bus = simple_strtoul(argv[1], NULL, 16);

        /* Assuming there will no more than 2 i2c buses [bus0, bus1]*/
        if (bus >  MAX_I2C_BUS) {
            bus = 0;
        }
    }

#if defined (CFG_I2C_NOPROBES)
    int k, skip;
#endif

    puts("Valid chip addresses:\n");

    /* Address 0 on an i2c bus is a broadcast bus and the
     * probe will be always be successful */
    for (j = 1; j < 128; j++) {
#if defined (CFG_I2C_NOPROBES)
        skip = 0;
        for (k = 0; k < sizeof(i2c_no_probes); k++) {
            if (j == i2c_no_probes[k]) {
                skip = 1;
                break;
            }
        }
        if (skip) {
            continue;
        }
#endif
        if (i2c_probe(bus, j) == 0) {
            printf("Sucessful probe on slave_read_addr 0x%02X on i2c bus %d\n",
                    j,
                    bus);
        }
    }
    putc('\n');

#if defined (CFG_I2C_NOPROBES)
    puts("Excluded chip addresses:");
    for (k = 0; k < sizeof(i2c_no_probes); k++) {
        printf(" %02X", i2c_no_probes[k]);
    }
    putc('\n');
#endif

    return 0;
}

static int 
do_i2c_dbg (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    unsigned short bus = 0;

    if (argc == 2) {
        bus = simple_strtoul(argv[1], NULL, 16);

        /* Assuming there will no more than 2 i2c buses [bus0, bus1]*/
        if (bus >  MAX_I2C_BUS) {
            bus = 0;
        }
    }

    return(i2c_debug(bus));
}

static int
do_i2c_mm (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    return mod_i2c_mem(cmdtp, 1, flag, argc, argv);
}


static int
do_i2c_nm (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    return mod_i2c_mem(cmdtp, 0, flag, argc, argv);
}

/* 
 * Sequential read & write 
 *   16-bit data burst reads & writes for 8-bit i2c slave devices 
 * 
 * i2c_read_seq :   burst read
 * i2c_write_seq:   burst write
 */

static int 
do_i2c_seq_rd (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uchar bus, dev, offset, buff[2]={0};

    if ( argc != 4 ) {
        printf("Usage: iseqr <bus> <slave_addr> <offset> \n");
        return  0;
    }

    bus = (uchar) simple_strtoul(argv[1], NULL, 16);
    dev = (uchar) simple_strtoul(argv[2], NULL, 16);
    offset = (uchar) simple_strtoul(argv[3], NULL, 16);

    printf("Reading from offset 0x%x on device at 0x%x on bus 0x%x\n",
            offset, dev, bus);

    i2c_read_seq(bus,dev,offset,buff,2); 

    printf("MSB:0x%x LSB:0x%x\n", buff[0],buff[1]);

    return 0;
}

static int
do_i2c_seq_wr (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    unsigned char bus, dev, offset, buff[2]={0};
    uint16_t val;

    if ( argc != 5 ) {
        printf("Usage: iseqw <bus> <slave_addr> <offset> <val>\n");
        return 0;
    }

    bus = simple_strtoul(argv[1], NULL, 16);
    dev = simple_strtoul(argv[2], NULL, 16);
    offset = simple_strtoul(argv[3], NULL, 16);
    val = (uint16_t) simple_strtoul(argv[4], NULL, 16);

    buff[0] = (val & 0xff);
    buff[1] = (val & 0xff00) >> 8;

    printf("Writing 0x%x (%x,%x) to dev 0x%x at offset 0x%x\n", val, 
            buff[0], buff[1], dev,offset);

    i2c_write_seq(bus, dev,offset,&buff,2);

    return 0;
}

/*Debug utilities & commands*/

#define BUFF_LEN 1024
extern int _i2c_tx_seq (unsigned short slave_addr, unsigned short bus,
        unsigned short addr, unsigned char *buffer, unsigned  int len) ;

static int 
do_i2c_sw (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    ulong addr;
    uint i, chip, alen = 2, bus = 0, arg_cnt = 1;
    uchar buff[BUFF_LEN],aa_buff[BUFF_LEN]; 
    int len,index,type;

    for(i=0;i<BUFF_LEN;i++){ buff[i] = i; }
    for(i=0;i<BUFF_LEN;i++){ aa_buff[i] = 0x00; }

    if (argc != 7) {
        printf("Usage: isw <bus> <chip> <addr> <index> <len> <type>\n");
        return 0;
    }

    bus = simple_strtoul(argv[1], NULL, 16);
    chip = simple_strtoul(argv[2], NULL, 16);
    addr = simple_strtoul(argv[3], NULL, 16);
    index =  simple_strtoul(argv[4], NULL, 16);
    len = simple_strtoul(argv[5], NULL, 16);
    type = simple_strtoul(argv[6], NULL, 16);

    if (type == 1)
        _i2c_tx_seq(chip, bus, addr, &buff[index],len);
    else if (type == 2)
        i2c_seq_wr(bus,chip,addr,alen,&aa_buff[index],len);
    else
        i2c_seq_wr(bus,chip,addr,alen,&buff[index],len);

    return 0;
}


static int 
do_i2c_set (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uint reg, bus = 0;
    int val;

    if ( argc != 4 ) {
        printf("Usage: iset <bus> <reg#> <val>\n");
        return 0;
    }
    bus = simple_strtoul(argv[1], NULL, 16);
    reg = simple_strtoul(argv[2], NULL, 16);
    val = simple_strtoul(argv[3], NULL, 16);

    i2c_set(bus,reg,val);

    return 0;
}


/* Programs the mfg board id prom with mfg info */
static int 
do_i2c_burn_idprom (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]){

    char *buff, *no;
    /* PA */
    char pa_buff[ ] = { 0x7f, 0xb0, 0x01, 0xff, 0x09, 0xa5, 0x00, 0x02, 
        0x52, 0x45, 0x56, 0x20, 0x30, 0x32, 0x00, 0x00 };

    /* SOHO */
    char soho_buff[ ] = { 0x7f, 0xb0, 0x01, 0xff, 0x09, 0xa2, 0x00, 0x02, 
        0x52, 0x45, 0x56, 0x20, 0x30, 0x32, 0x00, 0x00 };

    /*board revision */
    char rev[ ] = { 0x30, 0x32};

    /* SOHO */
    char soho_no[ ] = { 0x00, 0x00, 0x00, 0x00, 0x37, 0x31, 0x31, 0x2d, 
        0x30, 0x32, 0x39, 0x36, 0x35, 0x33, 0x00, 0x00 };

    /* PA */
    char pa_no[ ]   = { 0x00, 0x00, 0x00, 0x00, 0x37, 0x31, 0x31, 0x2d, 
        0x30, 0x33, 0x32, 0x32, 0x33, 0x34, 0x00, 0x00 };

    /* Serial number */
    char ser[ ]  = { 0x53, 0x2f, 0x4e, 0x20, 0x45, 0x42, 0x32, 0x38, 0x38,0x35};

    char tail[ ] = { 0x00, 0x00, 0x00, 0x0a, 0x0c, 0x07};

    int16_t rev_no,s_no;

    char *rev_ch, *s_ch;

    if (argc == 3) {
        strncpy(&rev[1], argv[1], 1);
        strncpy(&ser[4], argv[2], strlen(argv[2]));
    } else {
        printf("Usage: iprom <rev#> <serial#>\n");
        return 0;
    }

    if (fx_is_soho()) {
        buff = soho_buff;
        no   = soho_no;
    } else if (fx_is_pa()) {
        buff = pa_buff;
        no   = pa_no;
    } else {
        printf("Unsupported board\n");
        return -1;
    }

    i2c_write(I2C_BUS0, FX_BOARD_EEPROM_ADDR, MFG_EEPROM_HEAD_OFFSET, buff, 16);

    i2c_write(I2C_BUS0, FX_BOARD_EEPROM_ADDR, MFG_EEPROM_REV_OFFSET, rev, 2);

    i2c_write(I2C_BUS0, FX_BOARD_EEPROM_ADDR, MFG_EEPROM_NUM_OFFSET, no,  16);

    i2c_write(I2C_BUS0, FX_BOARD_EEPROM_ADDR, MFG_EEPROM_SER_OFFSET, ser, 10);

    i2c_write(I2C_BUS0, FX_BOARD_EEPROM_ADDR, MFG_EEPROM_TAIL_OFSET, tail, 6);

    return 0;
}


/****************** U-BOOT i2c commands ************************/
U_BOOT_CMD(
        idbg,
        2,
        1,
        do_i2c_dbg,
        "idbg  - dump the i2c controlelr registers \n",
        "idbg [bus]\n"
        );

U_BOOT_CMD(
        imd,
        5,
        1,
        do_i2c_md,                                                                   
        "imd     - i2c memory display\n",                                            
        "[bus#] chip address[.0, .1, .2] [# of objects]\n    - i2c memory display\n" 
        );

U_BOOT_CMD(
        imm,    3,  1,  do_i2c_mm,
        "imm     - i2c memory modify (auto-incrementing)\n",
        "[bus#] chip address[.0, .1, .2]\n"
        "    - memory modify, auto increment address\n"
        );
U_BOOT_CMD(
        inm,
        4,
        1,
        do_i2c_nm,
        "inm     - memory modify (constant address)\n",
        "[bus#] chip address[.0, .1, .2]\n    - memory modify, read and keep address\n"
        );

U_BOOT_CMD(
        imw,
        6,
        1,
        do_i2c_mw,
        "imw     - memory write (fill)\n",
        "[bus#] chip address[.0, .1, .2] value [count]\n    - memory write (fill)\n"
        );

U_BOOT_CMD(
        icrc32, 5,  1,  do_i2c_crc,
        "icrc32  - checksum calculation\n",
        "[bus#] chip address[.0, .1, .2] count\n    - compute CRC32 checksum\n"
        );

U_BOOT_CMD(
        iprobe, 2,  1,  do_i2c_probe,
        "iprobe  - probe to discover valid I2C chip addresses on a\n"
        "particular i2c bus \n",
        " -discover valid I2C chip addresses\n"
        );

U_BOOT_CMD(
        iseqr,
        4,
        1,
        do_i2c_seq_rd,
        "iseqr  - iseqr <bus> <dev_addr> <offset>\n",
        " \n"
        );

U_BOOT_CMD(
        iseqw,
        5,
        1,
        do_i2c_seq_wr,
        "iseqw  - iseqw <bus> <dev_addr> <offset> <val>\n",
        " \n"
        );


/* test cmd to test multi byte seq i2c read/write */
U_BOOT_CMD(
        isw,
        7,
        1,
        do_i2c_sw,
        "isw     - memory write seq\n",
        "[bus#] chip address[.0, .1, .2] buff_index [count]\n    - memory write seq\n"
        );

U_BOOT_CMD(
        iset,
        4,
        1,
        do_i2c_set,
        "iset     - set i2c reg\n",
        "[bus#] reg# val  \n"
        );

U_BOOT_CMD(
        iprom,
        3,
        1,
        do_i2c_burn_idprom,
        "iprom  - burn rev# , ser # to  eeprom\n",
        " \n"
        );


