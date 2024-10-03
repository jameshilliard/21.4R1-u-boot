/*
 * Copyright (c) 2011, Juniper Networks, Inc.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. if not ,see
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0,html>  
 */

#include <common.h>
#include <command.h>

#if (CONFIG_COMMANDS & CFG_CMD_MII)
#include <miiphy.h>

void MII_dump_0_to_5(ushort regvals[6],
                     uchar reglo,
                     uchar reghi);

#ifdef CONFIG_MAG8600
/* 
 * this include defines the functions used to read PHY reg information
 * via Master CPLD interface. This code in only valid in the MSC context
 */
#include "msc_fsc_phy.h"
#include "msc_i2c.h"
#include "msc_slv_smi.c"
#include "msc_marvel_98DX5128.h"

#define FSC_PHY_DEBUG
#define FSC_PHY_TRACE
#undef FSC_PHY_TRACE
#undef FSC_PHY_DEBUG

#ifdef FSC_PHY_DEBUG
#define dprintf printf
#ifdef FSC_PHY_TRACE 
#define dtrace dprintf
#else
#define dtrace(...)
#endif
#else
#define dprintf(...)
#define dtrace(...)
#endif

int cmd_get_data_size(char* arg, int default_size);

#endif  /* CONFIG_MAG8600 */

#ifdef CONFIG_MII_CLAUSE_45
/*
 * Display values from last command.
 */
static uint last_op;
static uint last_addr;
static uint last_data;
static uint last_reg;
static uint last_dev;

int miiphy_read45(unsigned char addr, unsigned char dev, unsigned short reg, unsigned short *value);

int miiphy_write45(unsigned char addr, unsigned char dev, unsigned short reg, unsigned short value);

int 
do_mii45 (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char            op;
    unsigned char   addr, dev;
    unsigned short  data, reg;
    int             rcode = 0;

    /*
     * We use the last specified parameters, unless new ones are
     * entered.
     */
    op   = last_op;
    addr = last_addr;
    data = last_data;
    reg  = last_reg;
    dev  = last_dev;

    if ((flag & CMD_FLAG_REPEAT) == 0) {
        op = argv[1][0];
        if (argc >= 3) {
            addr = simple_strtoul(argv[2], NULL, 16);
        }
        if (argc >= 4) {
            dev  = simple_strtoul(argv[3], NULL, 16);
        }
        if (argc >= 5) {
            reg  = simple_strtoul(argv[4], NULL, 16);
        }
        if (argc >= 6) {
            data = simple_strtoul(argv[5], NULL, 16);
        }
    }

    /*
     * check info/read/write.
     */
    if (op == 'i') {
        printf("info not supported for clause 45 yet\n");
        return 0;

    } else if (op == 's') {
        /* Scan for valid addresses */
        for (addr = 0; addr < 32; addr++) {
            for (dev = 0; dev < 32; dev++) {
                /* Read register 5, as it should be present in all MMDs per IEEE spec */
                if (miiphy_read45(addr, dev, 5, &data) == 0 && data != 0xffff) {
                    printf("Address 0x%02x, dev 0x%02x appears to a valid MMD\n", addr, dev);
                }
            }
        }
        for (addr = 0x80; addr < (0x80 + 32); addr++) {
            for (dev = 0; dev < 32; dev++) {
                /* Read register 5, as it should be present in all MMDs per IEEE spec */
                if (miiphy_read45(addr, dev, 5, &data) == 0 && data != 0xffff) {
                    printf("Address 0x%02x, dev 0x%02x appears to a valid MMD\n", addr, dev);
                }
            }
        }

    } else if (op == 'r') {
        if (miiphy_read45(addr, dev, reg, &data) != 0) {
            puts ("Error reading from the PHY\n");
            rcode = 1;
        }
        printf ("%04X\n", data & 0x0000FFFF);

    } else if (op == 'd') {
        int cur_reg;
        int max_reg = data;  /* In this case, data is really max_reg */

        for (cur_reg = reg; cur_reg <= max_reg; cur_reg++) {
            if (miiphy_read45(addr, dev, cur_reg, &data) != 0) {
                puts ("Error reading from the PHY\n");
                rcode = 1;
            }
            printf ("0x%04x:%04X\n", cur_reg, data & 0x0000FFFF);
        }

    } else if (op == 'w') {
        if (miiphy_write45(addr, dev, reg, data) != 0) {
            puts ("Error writing to the PHY\n");
            rcode = 1;
        }

    } else {
        printf ("Usage:\n%s\n", cmdtp->usage);
        return 1;
    }

    /*
     * Save the parameters for repeats.
     */
    last_op   = op;
    last_addr = addr;
    last_data = data;
    last_reg  = reg;
    last_dev  = dev;

    return rcode;
}

/***************************************************/

U_BOOT_CMD(
           mii45,	6,	1,	do_mii45,
           "mii45                                          - MII clause 45 utility commands\n",
           "mii45 info  <addr>                             - display MII PHY info\n"
           "mii45 read  <addr> <dev> <reg>                 - read  MII PHY register\n"
           "mii45 dump  <addr> <dev> <start_reg> <end_reg> - dump range of registers \n"
           "mii45 write <addr> <dev> <data>                - write MII PHY register>\n"
           "mii45 scan                                     - scan MDIO bus for clause 45 devices.\n"
           "Note: Bit 7 of <addr> selects which MDIO bus to use\n"
          );

#endif


#ifdef CONFIG_OCTEON_MSC
/* This code implements the mii command in the 
 * case the PHY is accessed indirectly thru some other device
 * in the case of the MSC it is done through the Master CPLD   
 * This code in only valid in the MSC context
 */
/*
 * Display values from last command.
 */

int 
do_fsc_phy (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{

    char            op;
    unsigned char   bus;
    unsigned char   dev;
    unsigned short  reg;
    unsigned short  data; 

    int             rcode = 0;
    unsigned char   clk = msc_get_clk();

    dtrace(">> %s: \n", __FUNCTION__); 

    /*
     * We use the last specified parameters, unless new ones are
     * entered.
     */
    op   = last_op;
    bus  = last_bus;
    dev  = last_dev;
    reg  = last_reg;
    data = last_data;



    if ((flag & CMD_FLAG_REPEAT) == 0) {
        op = argv[1][0];
        if (argc >= 3) {
            bus = simple_strtoul(argv[2], NULL, 16);
        }
        if (argc >= 4) {
            dev  = simple_strtoul(argv[3], NULL, 16);
        }
        if (argc >= 5) {
            reg  = simple_strtoul(argv[4], NULL, 16);
        }
        if (argc >= 6) {
            data = simple_strtoul(argv[5], NULL, 16);
        }
    }


    if (op == 'i') {
        /* 
         *  retreive MARVEL PHY 
         *  88E1111
         *  See Doc. No. MV-S100649-00, Rev. H October 11, 2006 
         */
        unsigned int  oui;
        unsigned char model;
        unsigned char rev;

        if (fsc_phy_info (bus, dev , &oui, &model, &rev) == 0) {
            printf ("PHY @0x%02X,0x%02X: "
                    "OUI = 0x%04X, "
                    "Model = 0x%02X, "
                    "Rev = 0x%02X, "
                    "%3dbaseT, %s\n",
                    bus,dev, oui, model, rev,
                    fsc_phy_speed (bus, dev),
                    fsc_phy_duplex (bus, dev) == FULL ? "FDX" : "HDX");
        }

    } else if (op == 'r') {	
        data = 0xffff;
        if (msc_i2c_read_16(bus, dev, reg, &data, clk) != 0) {
            printf("Error reading from the PHY bus=%02x device=%02x reg=%02x\n",
                   bus, dev, reg);
            rcode = 1;
        } else {
            printf("bus=%02x, device=%02x reg=%02x data=%04X\n",
                   bus, dev, reg, data & 0x0000FFFF);
        }

    } else if (op == 'w') {
        if (msc_i2c_write_16 (bus, dev, reg, data, clk) != 0) {
            printf("Error writing to the PHY bus=%02x device=%02x reg=%02x\n",
                   bus, dev, reg);
            rcode = 1;
        }	

    } else if (op == 'd') {

        ushort  reg_data[6];
        int     ok = 1;
        int     ireg;

        reg = 0; /* we take all 6 of them each time for dumping */

        for (ireg = 0; ireg < 6; ireg++) {
            if (msc_i2c_read_16(bus, dev, reg, &reg_data[ireg], clk) != 0) {
                ok = 0;
                printf("Error reading from the PHY bus=%02x addr=%02x reg=%02x\n",
                       bus, dev, reg);
                rcode = 1;
            }
            printf("reg[%d]=0x%X\n", ireg, reg_data[ireg]);
            reg++;
        }
        if (ok) {
            MII_dump_0_to_5(reg_data, 0, 5);
        }
        printf("\n");

    } else {
        printf("Usage:\n%s\n", cmdtp->usage);
        return 1;
    }

    /*
     * Save the parameters for repeats.
     */
    last_op   = op;
    last_bus  = bus;
    last_dev  = dev;
    last_reg  = reg;
    last_data = data;

    return rcode;
}

static uint last_ch_op;
static uint last_ch_bus;
static uint last_ch_dev;
static uint last_ch_address;
static uint last_ch_port; 
static uint last_ch_data;
static uint last_ch_size;

int 
do_ch_ln (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char                     op;
    unsigned char            bus;
    unsigned char            dev;
    unsigned int             address;
    unsigned char            port;
    unsigned int             data; 
    unsigned short           size;
    int                      rcode = 0;
    msc_slv_smi_status_t     status;
    msc_marvel_pci_status_t  dstatus;
    uint32_t                 read_data;

    dtrace(">> %s: \n", __FUNCTION__); 

    /*
     * We use the last specified parameters, unless new ones are
     * entered.
     */
    op      = last_ch_op;
    bus     = last_ch_bus;
    dev     = last_ch_dev;
    address = last_ch_address;
    port    = last_ch_address;
    data    = last_ch_data;

    if ((size = cmd_get_data_size(argv[0], 4)) < 0) {
        return 1;
    }
    if ((flag & CMD_FLAG_REPEAT) == 0) {
        op = argv[1][0];
        if (argc >= 3) {
            bus = simple_strtoul(argv[2], NULL, 16);
        }
        if (argc >= 4) {
            dev  = simple_strtoul(argv[3], NULL, 16);
        }
        if (argc >= 5) {
            address  = simple_strtoul(argv[4], NULL, 16);
        }
        if (argc >= 6) {
            port  = simple_strtoul(argv[5], NULL, 16);
        }
        if (argc >= 7) {
            data = simple_strtoul(argv[6], NULL, 16);
        }
    }


    if (op == 'r') {
        status = msc_slv_smi_read(bus, dev, (address + (port * 0x400)) , &read_data);

        if ( status ==  SLV_SMI_OK) {
            printf("register for port(%x): %x = %x \n", port, (address + (port * 0x400)), read_data);
        } else { 
            printf("Error reading register for port(%x) : %x\n", port,  address);
            printf("Status : %x \n", status);
        }

    } else if (op == 'w') {
        status = msc_slv_smi_write(bus, dev, (address + (port * 0x400)), data);

        if ( status ==  SLV_SMI_OK) {
            printf("register for port(%x) : %x = %x \n", port, (address + (port * 0x400) ), data);
        } else {
            printf("Error writing %x value in  register for port(%x): %x \n", data, port, (address + (port * 0x400)));
            printf("Status : %x \n", status);
        }

    } else if (op == 'd') {
        dstatus = msc_marvel_pci_display_reg(bus, dev, address, port);

    } else {
        printf("Usage:\n%s\n", cmdtp->usage);
        return 1;
    }

    /*
     * Save the parameters for repeats.
     */
    last_ch_op      = op;
    last_ch_bus     = bus;
    last_ch_dev     = dev;
    last_ch_address = address;
    last_ch_port    = port;
    last_ch_data    = data;
    last_ch_size    = size;

    return rcode;

}



/***************************************************/

U_BOOT_CMD(
           fsc_phy,	6,	1,	do_fsc_phy,
           "fsc_phy                                           - MII  MSC utility commands\n",
           "info  <bus> <dev_addr>                            - display FSC PHY info\n"
           "fsc_phy read  <bus> <dev_addr> <reg_addr>         - read  FSC PHY register \n"
           "fsc_phy write <bus> <dev_addr> <reg_addr> <value> - write FSC PHY register\n"
           "fsc_phy dump  <bus> <dev_addr>                    - dump FSC PHY registers 0-5\n"
          );


U_BOOT_CMD(
           ch_ln,	7,	1,	do_ch_ln,
           "ch_ln                                          - ch_ln  MSC utility commands\n",
           "ch_ln read  <bus> <dev_addr> <reg_addr> <port> - read  ch_ln  register \n"
           "ch_ln write <bus> <dev_addr> <reg_addr> <port> <value> - write ch_ln  register\n"
           "ch_ln dump  <bus> <dev_addr> <reg_addr> <port> - dump content of reg & fields\n"
          );

#endif /* CONFIG_OCTEON_MSC */


#ifdef CONFIG_TERSE_MII
/*
 * MII read/write
 *
 * Syntax:
 *  mii read {addr} {reg}
 *  mii write {addr} {reg} {data}
 */

int 
do_mii (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char            op;
    unsigned char   addr, reg;
    unsigned short  data;
    int             rcode = 0;

#if defined(CONFIG_8xx) || defined(CONFIG_MCF52x2)
    mii_init ();
#endif

    /*
     * We use the last specified parameters, unless new ones are
     * entered.
     */
    op   = last_op;
    addr = last_addr;
    data = last_data;
    reg  = last_reg;

    if ((flag & CMD_FLAG_REPEAT) == 0) {
        op = argv[1][0];
        if (argc >= 3) {
            addr = simple_strtoul (argv[2], NULL, 16);
        }
        if (argc >= 4) {
            reg  = simple_strtoul (argv[3], NULL, 16);
        }
        if (argc >= 5) {
            data = simple_strtoul (argv[4], NULL, 16);
        }
    }

    /*
     * check info/read/write.
     */
    if (op == 'i') {
        unsigned char  j, start, end;
        unsigned int   oui;
        unsigned char  model;
        unsigned char  rev;

        /*
         * Look for any and all PHYs.  Valid addresses are 0..31.
         */
        if (argc >= 3) {
            start = addr; end = addr + 1;
        } else {
            start = 0; end = 32;
        }

        for (j = start; j < end; j++) {
            if (miiphy_info (j, &oui, &model, &rev) == 0) {
                printf ("PHY 0x%02X: "
                        "OUI = 0x%04X, "
                        "Model = 0x%02X, "
                        "Rev = 0x%02X, "
                        "%3dbaseT, %s\n",
                        j, oui, model, rev,
                        miiphy_speed (j),
                        miiphy_duplex (j) == FULL ? "FDX" : "HDX");
            }
        }

    } else if (op == 'r') {
        if (miiphy_read (addr, reg, &data) != 0) {
            puts ("Error reading from the PHY\n");
            rcode = 1;
        }
        printf ("%04X\n", data & 0x0000FFFF);

    } else if (op == 'w') {
        if (miiphy_write (addr, reg, data) != 0) {
            puts ("Error writing to the PHY\n");
            rcode = 1;
        }

    } else {
        printf ("Usage:\n%s\n", cmdtp->usage);
        return 1;
    }

    /*
     * Save the parameters for repeats.
     */
    last_op   = op;
    last_addr = addr;
    last_data = data;
    last_reg  = reg;

    return rcode;
}

/***************************************************/

U_BOOT_CMD(
           mii,	5,	1,	do_mii,
           "mii                           - MII utility commands\n",
           "info  <addr>                  - display MII PHY info\n"
           "mii read  <addr> <reg>        - read  MII PHY <addr> register <reg>\n"
           "mii write <addr> <reg> <data> - write MII PHY <addr> register <reg>\n"
          );

#else /* ! CONFIG_TERSE_MII ================================================= */

typedef struct _MII_reg_desc_t {
    ushort regno;
    char * name;
} MII_reg_desc_t;

MII_reg_desc_t reg_0_5_desc_tbl[] = {
    { 0,   "PHY control register"                },
    { 1,   "PHY status register"                 },
    { 2,   "PHY ID 1 register"                   },
    { 3,   "PHY ID 2 register"                   },
    { 4,   "Autonegotiation advertisement register" },
    { 5,   "Autonegotiation partner abilities register" },
};

typedef struct _MII_field_desc_t {
    ushort hi;
    ushort lo;
    ushort mask;
    char * name;
} MII_field_desc_t;

MII_field_desc_t reg_0_desc_tbl[] = {
    { 15, 15, 0x01, "reset"                        },
    { 14, 14, 0x01, "loopback"                     },
    { 13,  6, 0x81, "speed selection"              }, /* special */
    { 12, 12, 0x01, "A/N enable"                   },
    { 11, 11, 0x01, "power-down"                   },
    { 10, 10, 0x01, "isolate"                      },
    {  9,  9, 0x01, "restart A/N"                  },
    {  8,  8, 0x01, "duplex"                       }, /* special */
    {  7,  7, 0x01, "collision test enable"        },
    {  5,  0, 0x3f, "(reserved)"                   }
};

MII_field_desc_t reg_1_desc_tbl[] = {
    { 15, 15, 0x01, "100BASE-T4 able"              },
    { 14, 14, 0x01, "100BASE-X  full duplex able"  },
    { 13, 13, 0x01, "100BASE-X  half duplex able"  },
    { 12, 12, 0x01, "10 Mbps    full duplex able"  },
    { 11, 11, 0x01, "10 Mbps    half duplex able"  },
    { 10, 10, 0x01, "100BASE-T2 full duplex able"  },
    {  9,  9, 0x01, "100BASE-T2 half duplex able"  },
    {  8,  8, 0x01, "extended status"              },
    {  7,  7, 0x01, "(reserved)"                   },
    {  6,  6, 0x01, "MF preamble suppression"      },
    {  5,  5, 0x01, "A/N complete"                 },
    {  4,  4, 0x01, "remote fault"                 },
    {  3,  3, 0x01, "A/N able"                     },
    {  2,  2, 0x01, "link status"                  },
    {  1,  1, 0x01, "jabber detect"                },
    {  0,  0, 0x01, "extended capabilities"        },
};

MII_field_desc_t reg_2_desc_tbl[] = {
    { 15,  0, 0xffff, "OUI portion"                },
};

MII_field_desc_t reg_3_desc_tbl[] = {
    { 15, 10, 0x3f, "OUI portion"                },
    {  9,  4, 0x3f, "manufacturer part number"   },
    {  3,  0, 0x0f, "manufacturer rev. number"   },
};

MII_field_desc_t reg_4_desc_tbl[] = {
    { 15, 15, 0x01, "next page able"               },
    { 14, 14, 0x01, "reserved"                     },
    { 13, 13, 0x01, "remote fault"                 },
    { 12, 12, 0x01, "reserved"                     },
    { 11, 11, 0x01, "asymmetric pause"             },
    { 10, 10, 0x01, "pause enable"                 },
    {  9,  9, 0x01, "100BASE-T4 able"              },
    {  8,  8, 0x01, "100BASE-TX full duplex able"  },
    {  7,  7, 0x01, "100BASE-TX able"              },
    {  6,  6, 0x01, "10BASE-T   full duplex able"  },
    {  5,  5, 0x01, "10BASE-T   able"              },
    {  4,  0, 0x1f, "xxx to do"                    },
};

MII_field_desc_t reg_5_desc_tbl[] = {
    { 15, 15, 0x01, "next page able"               },
    { 14, 14, 0x01, "acknowledge"                  },
    { 13, 13, 0x01, "remote fault"                 },
    { 12, 12, 0x01, "(reserved)"                   },
    { 11, 11, 0x01, "asymmetric pause able"        },
    { 10, 10, 0x01, "pause able"                   },
    {  9,  9, 0x01, "100BASE-T4 able"              },
    {  8,  8, 0x01, "100BASE-X full duplex able"   },
    {  7,  7, 0x01, "100BASE-TX able"              },
    {  6,  6, 0x01, "10BASE-T full duplex able"    },
    {  5,  5, 0x01, "10BASE-T able"                },
    {  4,  0, 0x1f, "xxx to do"                    },
};

#define DESC0LEN (sizeof(reg_0_desc_tbl)/sizeof(reg_0_desc_tbl[0]))
#define DESC1LEN (sizeof(reg_1_desc_tbl)/sizeof(reg_1_desc_tbl[0]))
#define DESC2LEN (sizeof(reg_2_desc_tbl)/sizeof(reg_2_desc_tbl[0]))
#define DESC3LEN (sizeof(reg_3_desc_tbl)/sizeof(reg_3_desc_tbl[0]))
#define DESC4LEN (sizeof(reg_4_desc_tbl)/sizeof(reg_4_desc_tbl[0]))
#define DESC5LEN (sizeof(reg_5_desc_tbl)/sizeof(reg_5_desc_tbl[0]))

typedef struct _MII_field_desc_and_len_t {
    MII_field_desc_t  *pdesc;
    ushort             len;
} MII_field_desc_and_len_t;

MII_field_desc_and_len_t desc_and_len_tbl[] = {
    { reg_0_desc_tbl, DESC0LEN },
    { reg_1_desc_tbl, DESC1LEN },
    { reg_2_desc_tbl, DESC2LEN },
    { reg_3_desc_tbl, DESC3LEN },
    { reg_4_desc_tbl, DESC4LEN },
    { reg_5_desc_tbl, DESC5LEN },
};

static void dump_reg(ushort regval, MII_reg_desc_t *prd, MII_field_desc_and_len_t *pdl);

static int special_field(ushort regno, MII_field_desc_t *pdesc, ushort regval);

void 
MII_dump_0_to_5 (ushort regvals[6], uchar reglo, uchar reghi)
{
    ulong i;

    for (i = 0; i < 6; i++) {
        if ((reglo <= i) && (i <= reghi)) {
            dump_reg(regvals[i], &reg_0_5_desc_tbl[i],
                     &desc_and_len_tbl[i]);
        }
    }
}

static void 
dump_reg (ushort regval, MII_reg_desc_t *prd, MII_field_desc_and_len_t *pdl)
{
    ulong              i;
    ushort             mask_in_place;
    MII_field_desc_t  *pdesc;

    printf("%u.     (%04hx)                 -- %s --\n",
           prd->regno, regval, prd->name);

    for (i = 0; i < pdl->len; i++) {
        pdesc = &pdl->pdesc[i];

        mask_in_place = pdesc->mask << pdesc->lo;

        printf("  (%04hx:%04hx) %u.",
               mask_in_place,
               regval & mask_in_place,
               prd->regno);

        if (!special_field(prd->regno, pdesc, regval)) {
            if (pdesc->hi == pdesc->lo) {
                printf("%2u   ", pdesc->lo);
            } else {
                printf("%2u-%2u", pdesc->hi, pdesc->lo);
            }
            printf(" = %5u    %s",
                   (regval & mask_in_place) >> pdesc->lo,
                   pdesc->name);
        }
        printf("\n");

    }
    printf("\n");
}

/* Special fields:
 ** 0.6,13
 ** 0.8
 ** 2.15-0
 ** 3.15-0
 ** 4.4-0
 ** 5.4-0
 */

static int 
special_field (ushort regno, MII_field_desc_t *pdesc, ushort regval)
{
    if ((regno == 0) && (pdesc->lo == 6)) {
        ushort speed_bits = regval & PHY_BMCR_SPEED_MASK;
        printf("%2u,%2u =   b%u%u    speed selection = %s Mbps",
               6, 13,
               (regval >>  6) & 1,
               (regval >> 13) & 1,
               speed_bits == PHY_BMCR_1000_MBPS ? "1000" :
               speed_bits == PHY_BMCR_100_MBPS  ? "100" :
               speed_bits == PHY_BMCR_10_MBPS   ? "10" :
               "???");
        return 1;

    } else if ((regno == 0) && (pdesc->lo == 8)) {
        printf("%2u    = %5u    duplex = %s",
               pdesc->lo,
               (regval >>  pdesc->lo) & 1,
               ((regval >> pdesc->lo) & 1) ? "full" : "half");
        return 1;

    } else if ((regno == 4) && (pdesc->lo == 0)) {
        ushort sel_bits = (regval >> pdesc->lo) & pdesc->mask;
        printf("%2u-%2u = %5u    selector = %s",
               pdesc->hi, pdesc->lo, sel_bits,
               sel_bits == PHY_ANLPAR_PSB_802_3 ?
               "IEEE 802.3" :
               sel_bits == PHY_ANLPAR_PSB_802_9 ?
               "IEEE 802.9 ISLAN-16T" :
               "???");
        return 1;

    } else if ((regno == 5) && (pdesc->lo == 0)) {
        ushort sel_bits = (regval >> pdesc->lo) & pdesc->mask;
        printf("%2u-%2u =     %u    selector = %s",
               pdesc->hi, pdesc->lo, sel_bits,
               sel_bits == PHY_ANLPAR_PSB_802_3 ?
               "IEEE 802.3" :
               sel_bits == PHY_ANLPAR_PSB_802_9 ?
               "IEEE 802.9 ISLAN-16T" :
               "???");
        return 1;
    }

    return 0;
}

static uint last_op=0;
static uint last_bus=0;
static uint last_data=0;
static uint last_addr_lo=0;
static uint last_addr_hi=0;
static uint last_reg_lo=0;
static uint last_reg_hi=0;
static uint last_size=0;

static void
extract_range (char *input, unsigned char *plo, unsigned char *phi)
{
    char * end;
    *plo = simple_strtoul(input, &end, 16);
    if (*end == '-') {
        end++;
        *phi = simple_strtoul(end, NULL, 16);
    } else {
        *phi = *plo;
    }
}

/* ---------------------------------------------------------------- */
int
do_mii (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char            op;
    unsigned char   bus = 0;
    unsigned char   addrlo = 0; 
    unsigned char   addrhi = 0;
    unsigned char   reglo = 0;
    unsigned char   reghi = 0;
    unsigned char   addr = 0;
    unsigned char   reg = 0;
    unsigned short  data = 0;
    int             rcode = 0;
    unsigned char   size = 0;
    unsigned char   bus_selector=0;

#ifdef CONFIG_8xx
    mii_init ();
#endif

    /*
     * We use the last specified parameters, unless new ones are
     * entered.
     */
    op     = last_op;
    bus    = last_bus;
    addrlo = last_addr_lo;
    addrhi = last_addr_hi;
    reglo  = last_reg_lo;
    reghi  = last_reg_hi;
    data   = last_data;
    size   = last_size;

    if ((size = cmd_get_data_size(argv[0], 1)) < 0) {
        return 1;
    }

    if ((flag & CMD_FLAG_REPEAT) == 0) {
        op = argv[1][0];
        if (argc >= 3) {
            bus = simple_strtoul(argv[2], NULL, 16);
        }
        if (argc >= 4) {
            extract_range(argv[3], &addrlo, &addrhi);
        }
        if (argc >= 5) {
            extract_range(argv[4], &reglo, &reghi);
        }
        if (argc >= 6) {
            data = simple_strtoul (argv[5], NULL, 16);
        }
    }

    printf(" op=%x bus=%x addrlo=%x addrhi=%x reglo=%x reghi=%x data=%x\n", 
           op, bus, addrlo, addrhi, reglo, reghi, data);

    if (bus == 0) {
        bus_selector = 0;
    } else if (bus == 1) {
        bus_selector = 0x80;
    } else {
        printf("Only bus 0 1 are supported at this time\n");
        return 1;
    } 

    /*
     * check info/read/write/dump.
     */
    if (op == 'i') {
        unsigned char  j, start, end;
        unsigned int   oui;
        unsigned char  model;
        unsigned char  rev;

        /*
         * Look for any and all PHYs.  Valid addresses are 0..31.
         */
        if (argc >= 4) {
            start = addr; end = addr + 1;
        } else {
            start = 0; end = 32;
        }

        for (j = start; j < end; j++) {
            if (miiphy_info ((j | bus_selector), &oui, &model, &rev) == 0) {
                printf("PHY 0x%02X,0x%02X: "
                       "OUI = 0x%04X, "
                       "Model = 0x%02X, "
                       "Rev = 0x%02X, "
                       "%3dbaseT, %s\n",
                       bus, j, oui, model, rev,
                       miiphy_speed ((j | bus_selector)),
                       miiphy_duplex ((j | bus_selector)) == FULL ? "FDX" : "HDX");
            }
        }

    } else if (op == 'r') {
        for (addr = addrlo; addr <= addrhi; addr++) {
            for (reg = reglo; reg <= reghi; reg++) {
                data = 0xffff;
                if (miiphy_read ((addr | bus_selector), reg, &data) != 0) {
                    printf("Error reading from the PHY bus=%02X addr=%02x reg=%02x\n",
                           bus,addr, reg);
                    rcode = 1;
                } else {
                    if ((addrlo != addrhi) || (reglo != reghi)) {
                        printf("bus=%02X addr=%02x reg=%02x data=",
                               (uint)bus, (uint)addr, (uint)reg);
                    }
                    printf("%04X\n", data & 0x0000FFFF);
                }
            }
            if ((addrlo != addrhi) && (reglo != reghi)) {
                printf("\n");
            }
        }

    } else if (op == 'w') {
        for (addr = addrlo; addr <= addrhi; addr++) {
            for (reg = reglo; reg <= reghi; reg++) {
                if (miiphy_write ((addr | bus_selector), reg, data) != 0) {
                    printf("Error writing to the PHY bus=%02X addr=%02x reg=%02x\n",
                           bus, addr, reg);
                    rcode = 1;
                }
            }
        }

    } else if (op == 'd') {
        ushort  regs[6];
        int     ok = 1;
        if ((reglo > 5) || (reghi > 5)) {
            printf("The MII dump command only formats the "
                   "standard MII registers, 0-5.\n");
            return 1;
        }
        for (addr = addrlo; addr <= addrhi; addr++) {
            for (reg = 0; reg < 6; reg++) {
                if (miiphy_read((addr | bus_selector), reg, &regs[reg]) != 0) {
                    ok = 0;
                    printf("Error reading from the PHY bus=%02X addr=%02x reg=%02x\n",
                           bus, addr, reg);
                    rcode = 1;
                }
            }
            if (ok) {
                MII_dump_0_to_5(regs, reglo, reghi);
            }
            printf("\n");
        }

    } else {
        printf("Usage:\n%s\n", cmdtp->usage);
        return 1;
    }

    /*
     * Save the parameters for repeats.
     */
    last_op      = op;
    last_bus     = bus;
    last_addr_lo = addrlo;
    last_addr_hi = addrhi;
    last_reg_lo  = reglo;
    last_reg_hi  = reghi;
    last_data    = data;

    return rcode;
}

/***************************************************/

U_BOOT_CMD(
           mii,	6,	1,	do_mii,
           "mii                                 - MII utility commands\n",
           "info  <bus> <addr>                  - display MII PHY info\n"
           "mii read  <bus> <addr> <reg>        - read  MII PHY on bus <bus> <addr> register <reg>\n"
           "mii write <bus> <addr> <reg> <data> - write MII PHY on bus <bus> <addr> register <reg>\n"
           "mii dump  <bus> <addr> <reg>        - pretty-print <bus> <addr> <reg> (0-5 only)\n"
           "Addr and/or reg may be ranges, e.g. 2-7.\n"
           "Note: Bit 7 of <addr> selects which MDIO bus to use\n"
          );

#endif /* CONFIG_TERSE_MII */

#endif /* CFG_CMD_MII */
