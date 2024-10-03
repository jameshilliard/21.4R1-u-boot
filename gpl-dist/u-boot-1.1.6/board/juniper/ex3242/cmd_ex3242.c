/*
 * Copyright (c) 2007-2011, Juniper Networks, Inc.
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

#if (CONFIG_COMMANDS & CFG_CMD_BSP)
#include <asm/processor.h>
#include <asm/immap_85xx.h>
#include <asm/io.h>
#include <part.h>
#include <usb.h>
#include "pci.h"
#include <pcie.h>
#include <miiphy.h>
#include "tsec.h"
#include "post.h"
#endif  /* CFG_CMD_BSP */

DECLARE_GLOBAL_DATA_PTR;

extern int board_id(void);

int flash_write_direct (ulong addr, char *src, ulong cnt);

#if (CONFIG_COMMANDS & CFG_CMD_BSP)
extern block_dev_desc_t usb_dev_desc[USB_MAX_STOR_DEV];

#if defined(CONFIG_NOR)
extern block_dev_desc_t nor_dev;
#endif

extern int cmd_get_data_size(char* arg, int default_size);

#if defined(USB_WRITE_READ)
extern unsigned long usb_stor_write(unsigned long device,unsigned long blknr, 
                       unsigned long blkcnt, unsigned long *buffer);
#endif

extern struct pcie_controller pcie_hose[];
extern int tsec_tbimii_read(char *devname, unsigned char reg, unsigned short *value);
extern int tsec_tbimii_write(char *devname, unsigned char reg, unsigned short value);

void pcieinfo(int, int , int );
void pciedump(int, int );
static char *pcie_classes_str(unsigned char );
void pcie_header_show_brief(int, pcie_dev_t );
void pcie_header_show(int, pcie_dev_t );
int do_pcie_probe(cmd_tbl_t *, int , int , char *[]);

/*--------------- USB OHCI/EHCI ----------------*/
#define swap_ulong(x) \
    ({ unsigned long x_ = (unsigned long)x; \
     (unsigned long)( \
        ((x_ & 0x000000FFUL) << 24) | \
        ((x_ & 0x0000FF00UL) <<  8) | \
        ((x_ & 0x00FF0000UL) >>  8) | \
        ((x_ & 0xFF000000UL) >> 24) ); \
    })
    
#undef readl
#define readl(a) swap_ulong(*((vu_long *)(a)))
#undef writel
#define writel(a, b) (*((vu_long *)(b)) = swap_ulong((vu_long)a))
/*---------------------------------------------*/

/* Show information about devices on PCIe bus. */

void pcieinfo(int controller, int busnum, int short_listing)
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
    
               pcie_hose_read_config_word(&pcie_hose[controller-1], dev, PCIE_VENDOR_ID, &vendor_id);
        if ((vendor_id == 0xFFFF) || (vendor_id == 0x0000))
        continue;
    
        pcie_hose_read_config_byte(&pcie_hose[controller-1], dev, PCIE_HEADER_TYPE, &header_type);
        
        if (short_listing) {
            printf("%02x.%02x.%02x   ",busnum, device, function);
            pcie_header_show_brief(controller, dev);
        }
        else {
            printf("\nFound PCIE[%d] device %02x.%02x.%02x:\n",controller, busnum, device,function); 
                  pcie_header_show(controller, dev);
        }
       }
    }
}

void pciedump(int controller, int busnum)
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
    pcie_hose_read_config_byte(&pcie_hose[controller-1], dev, addr,(buff+addr));

    do {
    printf("0x%02x :",offset);
    linebytes = (nbytes < 16)?nbytes:16;

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




static char *pcie_classes_str(u8 class)
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
    
void pcie_header_show_brief(int controller, pcie_dev_t dev)
{
    unsigned short vendor, device;
    unsigned char class, subclass;

    pcie_hose_read_config_word(&pcie_hose[controller-1], dev, PCIE_VENDOR_ID, &vendor);
    pcie_hose_read_config_word(&pcie_hose[controller-1], dev, PCIE_VENDOR_ID, &vendor);
    pcie_hose_read_config_word(&pcie_hose[controller-1], dev, PCIE_DEVICE_ID, &device);
    pcie_hose_read_config_byte(&pcie_hose[controller-1], dev, PCIE_CLASS_CODE, &class);
    pcie_hose_read_config_byte(&pcie_hose[controller-1], dev, PCIE_CLASS_SUB_CODE, &subclass);
                
    printf("0x%.4x     0x%.4x     %-23s 0x%.2x\n", 
       vendor, device, pcie_classes_str(class), subclass);
}

/* Reads the header of the specified PCIe device
 * Inputs: dev  bus+device+function number
 */

void pcie_header_show(int controller, pcie_dev_t dev)
{
    unsigned char _byte, header_type;
    unsigned short _word;
    unsigned int _dword;
        
#define PRINT(msg, type, reg) \
        pcie_hose_read_config_##type(&pcie_hose[controller-1], dev, reg, &_##type); \
        printf(msg, _##type)

    pcie_hose_read_config_byte(&pcie_hose[controller-1], dev, PCIE_HEADER_TYPE, &header_type);
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
        PRINT ("sub system vendor ID = 0x%.4x\n", word, PCIE_SUBSYSTEM_VENDOR_ID);
        PRINT ("sub system ID = 0x%.4x\n", word, PCIE_SUBSYSTEM_ID);
        PRINT ("interrupt line = 0x%.2x\n", byte, PCIE_INTERRUPT_LINE);
        PRINT ("interrupt pin = 0x%.2x\n", byte, PCIE_INTERRUPT_PIN);
        break;

    case PCIE_HEADER_TYPE_BRIDGE:   /* PCIE-to-PCIE bridge */
        PRINT ("base address 1 = 0x%.8x\n", dword, PCIE_BASE_ADDRESS_1);
        PRINT ("primary bus number = 0x%.2x\n", byte, PCIE_PRIMARY_BUS);
        PRINT ("secondary bus number = 0x%.2x\n", byte,PCIE_SECONDARY_BUS);
        PRINT ("subordinate bus number = 0x%.2x\n", byte, PCIE_SUBORDINATE_BUS);
        PRINT ("IO base = 0x%.2x\n", byte, PCIE_IO_BASE);
        PRINT ("IO limit = 0x%.2x\n", byte, PCIE_IO_LIMIT);
        PRINT ("memory base = 0x%.4x\n", word, PCIE_MEMORY_BASE);
        PRINT ("memory limit = 0x%.4x\n", word, PCIE_MEMORY_LIMIT);
        PRINT ("prefetch memory base = 0x%.4x\n", word, PCIE_PREF_MEMORY_BASE);
        PRINT ("prefetch memory limit = 0x%.4x\n", word, PCIE_PREF_MEMORY_LIMIT);
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

static unsigned char pcieshortinfo (int controller, int busnum)
{
    pcie_dev_t     dev;
    unsigned short vendor_id;
    unsigned short device_id;
    unsigned char  header_type;
    unsigned char  class_code, subclass;
    unsigned char  linkstat;
    
    dev = PCIE_BDF(busnum, 0, 0);
    pcie_hose_read_config_word
        (&pcie_hose[controller-1], dev, PCIE_VENDOR_ID, &vendor_id);
    pcie_hose_read_config_byte
        (&pcie_hose[controller-1], dev, PCIE_HEADER_TYPE, &header_type);
    pcie_hose_read_config_word
        (&pcie_hose[controller-1], dev, PCIE_DEVICE_ID, &device_id);
    pcie_hose_read_config_byte
        (&pcie_hose[controller-1], dev, PCIE_CLASS_CODE, &class_code);
    pcie_hose_read_config_byte
        (&pcie_hose[controller-1], dev, PCIE_CLASS_SUB_CODE, &subclass);
    pcie_hose_read_config_byte
        (&pcie_hose[controller-1], 0, 0x404, &linkstat);
        
    printf("%d\t" "%d\t" "0\t" "0\t" "0x%.4x\t" "0x%.4x\t", 
           controller, busnum, vendor_id, device_id);
    printf("%-23s\t0x%.2x\t", pcie_classes_str(class_code), subclass);
    printf("0x%2X", linkstat);
    printf("\n");
    return linkstat;
}

static void do_pcie_status (void)
{   
    unsigned char ls1, ls2, ls3;
    
    printf("Ctrl\t""Bus\t""Dev\t""Func\t""Vendor\t""Device\t""%-23s\t" 
           "SubCls\t""Link\n", "Class");
    ls1 = pcieshortinfo(1, 0);
    if (ls1 == 0x16) pcieshortinfo(1, 1);
    printf("-\n");
    ls2 = pcieshortinfo(2, 0);
    if (ls2 == 0x16) pcieshortinfo(2, 1);
    printf("-\n");
    ls3 = pcieshortinfo(3, 0);
    if (ls3 == 0x16) pcieshortinfo(3, 1);
}

static int stress_pcie_up (int controller, int retries, int CheetahNo)
{   
    int i;
    unsigned char ls;
    unsigned short vendor_id;
    unsigned short device_id;
    
    printf("PCIE stresstest ctrl %d - Cheetah %d up   (%d retries) ... ", 
           controller, CheetahNo, retries);
    pcie_hose_read_config_word
        (&pcie_hose[controller-1], PCIE_BDF(1, 0, 0), PCIE_VENDOR_ID, 
         &vendor_id);
    pcie_hose_read_config_word
        (&pcie_hose[controller-1], PCIE_BDF(1, 0, 0), PCIE_DEVICE_ID, 
         &device_id);
    if (vendor_id != 0x11AB )
    {
        printf("Invalid Vendor ID detected : %04X\n", vendor_id);
        return 1;
    }
    if (device_id != 0xDB00 && device_id != 0xDB01)
    {
        printf("No CheetahJ detected - device id=%04X "
               "(expected 0xDB00/0xDB01)\n", device_id);
        return 1;
    }
    for (i = 0; i < retries; i++) 
    {
        pcie_hose_read_config_byte(&pcie_hose[controller-1], 0, 0x404, &ls);
        if (ls != 0x16)
        {
            printf("LinkStatus: 0x%02X after %5d retries, expected 0x16\n",
                   ls, i);
            return 1;
        }
    }
    printf("OK\n");
    return 0;
}

static int stress_pcie_down (int controller, int retries)
{   
    int i;
    unsigned char ls;

    printf("PCIE stresstest ctrl %d expect Link down (%d retries) ... ", 
           controller, retries);
    for (i = 0; i < retries; i++) 
    {
        pcie_hose_read_config_byte(&pcie_hose[controller-1], 0, 0x404, &ls);
        if (ls != 0x00 && ls != 0x01)
        {
            printf("LinkStatus: 0x%02X after %5d retries "
                   "(expected 0x00 or 0x01)\n", ls, i);
            return 1;
        }
    }
    printf("OK\n");
    return 0;
}

static void do_pcie_stress (void)
{   
    int err     = 0;
    int retries = 5000000;

    switch (board_id())
    {
        default: 
            printf("Unsupported board type - "
                   "can not stress test PCI-E bus!");
            err = 1;
            break;

        case I2C_ID_JUNIPER_EX3200_24_T: /* Espresso 24 T */
        case I2C_ID_JUNIPER_EX3200_24_P: /* Espresso 24 P */
            err += stress_pcie_up(1, retries, 1);
            err += stress_pcie_down(2, retries);
            err += stress_pcie_down(3, retries);
            break;

        case I2C_ID_JUNIPER_EX4200_24_F: /* Latte 24 SFP */
            err += stress_pcie_up(3, retries, 1);
            err += stress_pcie_up(1, retries, 2);
            err += stress_pcie_down(2, retries);
            break;

        case I2C_ID_JUNIPER_EX4200_24_T: /* Latte 24 T */
        case I2C_ID_JUNIPER_EX4200_24_P: /* Latte 24 P */
            err += stress_pcie_up(3, retries, 1);
            err += stress_pcie_up(1, retries, 2);
            err += stress_pcie_down(2, retries);
            break;

        case I2C_ID_JUNIPER_EX3200_48_T: /* Espresso 48 T */
        case I2C_ID_JUNIPER_EX3200_48_P: /* Espresso 48 P */
            err += stress_pcie_up(1, retries, 1);
            err += stress_pcie_up(3, retries, 2);
            err += stress_pcie_down(2, retries);
            break;

        case I2C_ID_JUNIPER_EX4200_48_T: /* Latte 48 T */
        case I2C_ID_JUNIPER_EX4200_48_P: /* Latte 48 P */
            err += stress_pcie_up(2, retries, 1);
            err += stress_pcie_up(3, retries, 2);
            err += stress_pcie_up(1, retries, 3);
            break;

    }
    if (err)
        printf("FAILED - PCIE stresstest with %d PCI errors\n", err);
    else
        printf("PASSED - PCIE stresstest\n");
}

/* This routine services the "pcieprobe" command.
 * Syntax - pcieprobe [-v] [-d]
 *            -v for verbose, -d for dump.
 */

int do_pcie_probe(cmd_tbl_t *cmd, int flag, int argc, char *argv[])
{
    int controller = 1;  /* default PCIe 1 */
    unsigned char link;

    if (argc <= 1) {   
        do_pcie_status();
        return 0;
    }
    
    if ((argv[1][0] == '1') ||
        (argv[1][0] == '2') ||
        (argv[1][0] == '3'))
        ;
    else if (argv[1][0] == 's') {   
        do_pcie_stress();
        return 0;
    }
    else {   
        do_pcie_status();
        return 0;
    }
       
    controller = simple_strtoul(argv[1], NULL, 10);
    if ((controller > 3) || (controller < 1))
        printf("PCIE controller %d out of range 1-3.\n",controller);

    pcie_hose_read_config_byte(&pcie_hose[controller-1], 0, 0x404, &link);
    if (argc == 3) {
    if (strcmp(argv[2], "-v") == 0) {
        pcieinfo(controller, 0,0); /* 0 - bus0 0 - verbose list */
           if (link != 0x16)
               return 0;
        pcieinfo(controller, 1,0); /* 1 - bus1 0 - verbose list */
        pcieinfo(controller, 2,0); /* 1 - bus1 0 - verbose list */
    }
    else if (strcmp(argv[2],"-d") == 0) {
        pciedump(controller, 0);  /* 0 - bus0 */
           if (link != 0x16)
               return 0;
        pciedump(controller, 1);  /* 1 - bus1 */
        pciedump(controller, 2);  /* 1 - bus1 */
    }
    }else {
    pcieinfo(controller, 0,1); /* 0 - bus0 1 - short list */
       if (link != 0x16)
           return 0;
    pcieinfo(controller, 1,1); /* 1 - bus1 1 - short list */
    pcieinfo(controller, 2,1); /* 1 - bus1 0 - short list */
    }
    
    return 0;
 usage:
    printf("Usage:\n%s\n", cmd->usage);
    return 1;
}


/* Convert the "bus.device.function" identifier into a number.
 */
static pcie_dev_t get_pcie_dev(char* name)
{
    char cnum[12];
    int len, i, iold, n;
    int bdfs[3] = {0,0,0};

    len = strlen(name);
    if (len > 8)
        return -1;
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

static int pcie_cfg_display(int controller, pcie_dev_t bdf, ulong addr, ulong size, ulong length)
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
                pcie_hose_read_config_dword(&pcie_hose[controller-1], bdf, addr, &val4);
                printf(" %08x", val4);
            } else if (size == 2) {
                pcie_hose_read_config_word(&pcie_hose[controller-1], bdf, addr, &val2);
                printf(" %04x", val2);
            } else {
                pcie_hose_read_config_byte(&pcie_hose[controller-1], bdf, addr, &val1);
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

static int pcie_cfg_write (int controller, pcie_dev_t bdf, ulong addr, ulong size, ulong value)
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
    return 0;
}

static int
pcie_cfg_modify (int controller, pcie_dev_t bdf, ulong addr, ulong size, ulong value, int incrflag)
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
            pcie_hose_read_config_dword(&pcie_hose[controller-1], bdf, addr, &val4);
            printf(" %08x", val4);
        }
        else if (size == 2) {
            pcie_hose_read_config_word(&pcie_hose[controller-1], bdf, addr, &val2);
            printf(" %04x", val2);
        }
        else {
            pcie_hose_read_config_byte(&pcie_hose[controller-1], bdf, addr, &val1);
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

    return 0;
}


/* PCI Configuration Space access commands
 *
 * Syntax:
 *  pci display[.b, .w, .l] bus.device.function} [addr] [len]
 *  pci next[.b, .w, .l] bus.device.function [addr]
 *      pci modify[.b, .w, .l] bus.device.function [addr]
 *      pci write[.b, .w, .l] bus.device.function addr value
 */
int do_pcie (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    ulong addr = 0, value = 0, size = 0;
    pcie_dev_t bdf = 0;
    char cmd = 's';
       int controller = 1;  /* default PCIe 1 */
       unsigned char link;

    if (argc < 2)
            goto usage;
    
    if (argc > 2)
        cmd = argv[2][0];
    
       if ((argv[1][0] == '1') ||
           (argv[1][0] == '2') ||
           (argv[1][0] == '3'))
           ;
       else
            goto usage;
       
       controller = simple_strtoul(argv[1], NULL, 10);
       if ((controller > 3) || (controller < 1))
           printf("PCIe controller %d out of range 1-3.\n",controller);

       pcie_hose_read_config_byte(&pcie_hose[controller-1], 0, 0x404, &link);

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
            return 1;
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
              if ((bdf >= 1) && (link != 0x16))
                  return 0;
        pcieinfo(controller, bdf, value);
        return 0;
    }

       if ((bdf >= 1) && (link != 0x16))
           return 0;
       
    switch (argv[2][0]) {
    case 'h':       /* header */
        pcie_header_show(controller, bdf);
        return 0;
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

    return 1;
 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return 1;
}


/* storage device commands
 *
 * Syntax:
 */
int do_storage(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char cmd1, cmd2;
    int device, block, count;
    ulong mem;
    volatile ulong *buffer;
    block_dev_desc_t *stor_dev = NULL;
      
    if (argc <= 6)
        goto usage;
    cmd1 = argv[1][0];
    cmd2 = argv[2][0];
    
    device = simple_strtoul(argv[3], NULL, 10);
    block = simple_strtoul(argv[4], NULL, 10);
    mem = simple_strtoul(argv[5], NULL, 16);
    buffer = (ulong *)mem;
    
    if ((cmd2 == 'n') || (cmd2 == 'N'))
#if defined(CONFIG_NOR)
        stor_dev = &nor_dev;
#else
        stor_dev = NULL;
#endif
    else {
        if ((device >= 0) && (device < 5))
            stor_dev = &usb_dev_desc[device];;
    }

    if (!stor_dev)
        printf("No matched %s for device number %d\n",argv[2], device);
        
    switch (cmd1) {
        case 'r':
        case 'R':
            count = simple_strtoul(argv[6], NULL, 10);
            stor_dev->block_read(device, block, count, (ulong *)buffer);

            break;
            
#if defined(USB_WRITE_READ)
        case 'w':
        case 'W':
        {
            ulong size;

            if ((cmd2 == 'n') || (cmd2 == 'N')) {
                printf("Block write is not implemented for NOR flash\n");
                break;
            }
            size = simple_strtoul(argv[6], NULL, 16);
            count = size / 512;
            if (size & 511) 
                count++;
            usb_stor_write(device, block, count, (ulong *)buffer);
        }
            break;
#endif

        default:
            printf("???");
            break;
    }

    return 1;
 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return 1;
}


/* isp1564 commands
 *
 */
int do_isp(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{

    char cmd1, cmd2;
    int busdevfunc;
    uint32_t addr, reg, value;
    static uint32_t ohci_base_addr = 0, ehci_base_addr = 0;

    /* To take care of non isp devices, get PCI Device ID */
    busdevfunc = pci_find_device(USB_OHCI_VEND_ID, USB_OHCI_DEV_ID, 0); 
    if (busdevfunc == -1) {
        printf("Error USB OHCI (%04X,%04X) not found\n",
            USB_OHCI_VEND_ID, USB_OHCI_DEV_ID);
        return -1;
    }              
    if (argc <= 2)
        goto usage;
    
    cmd1 = argv[1][0];
    cmd2 = argv[2][0];
    
    switch (cmd2) {
        case 'a':       /* address */
        case 'A':
            if ((cmd1 == 'o') || (cmd1 == 'O')) {
             busdevfunc=pci_find_device(USB_OHCI_VEND_ID,USB_OHCI_DEV_ID,0); /* get PCI Device ID */
             if (busdevfunc == -1) {
              printf("Error USB OHCI (%04X,%04X) not found\n",USB_OHCI_VEND_ID,USB_OHCI_DEV_ID);
              return -1;
             }

             pci_read_config_dword(busdevfunc,PCI_BASE_ADDRESS_0,&ohci_base_addr);
                ohci_base_addr &= 0xFFFFF000;
             printf("OHCI Base Address = 0x%lx\n",ohci_base_addr);
            }
            else if ((cmd1 == 'e') || (cmd1 == 'E')) {
             busdevfunc=pci_find_device(USB_OHCI_VEND_ID,USB_EHCI_DEV_ID,0); /* get PCI Device ID */
             if (busdevfunc == -1) {
              printf("Error USB EHCI (%04X,%04X) not found\n",USB_OHCI_VEND_ID,USB_EHCI_DEV_ID);
              return -1;
             }

             pci_read_config_dword(busdevfunc,PCI_BASE_ADDRESS_0,&ehci_base_addr);
                ehci_base_addr &= 0xFFFFFF00;
             printf("EHCI Base Address = 0x%lx\n",ehci_base_addr);
            }
            else
                goto usage;
            break;
                        
        case 'd':       /* dump */
        case 'D':
            if ((cmd1=='o') || (cmd1=='O')) {
                if (ohci_base_addr == 0) {
                 printf("OHCI Base Address = 0x%lx\n",ohci_base_addr);
                    return -1;
                }

                for (addr = 0; addr < 0x5c; addr += 4) {
                    value = readl(ohci_base_addr | addr);
                    printf("OHCI reg 0x%02x = 0x%lx\n", addr, value);
                }
            }
            else if ((cmd1=='e') || (cmd1=='E')) {
                if (ehci_base_addr == 0) {
                 printf("EHCI Base Address = 0x%lx\n",ehci_base_addr);
                    return -1;
                }
                for (addr = 0; addr < 0x70; addr += 4) {
                    if ((addr == 0x14) ||
                        (addr == 0x18) ||
                        (addr == 0x1c) ||
                        (addr == 0x30) ||
                        ((addr >= 0x3c) && (addr <= 0x5c)))
                        continue;
                        
                    value = readl(ehci_base_addr | addr);
                    printf("EHCI reg 0x%02x = 0x%lx\n", addr, value);
                }
            }
            else
                goto usage;
            break;
                        
        case 'r':       /* read */
        case 'R':
            if (argc <= 3)
                goto usage;
            
            reg = simple_strtoul(argv[3], NULL, 16);
            if ((cmd1=='o') || (cmd1=='O')) {
                if (ohci_base_addr == 0) {
                 printf("OHCI Base Address = 0x%lx\n",ohci_base_addr);
                    return -1;
                }
                value = readl(ohci_base_addr | reg);
             printf("OHCI reg 0x%x (0x%lx) = 0x%lx\n",reg , ohci_base_addr|reg, value);
            }
            else if ((cmd1=='e') || (cmd1=='E')) {
                if (ehci_base_addr == 0) {
                 printf("EHCI Base Address = 0x%lx\n",ehci_base_addr);
                    return -1;
                }
                value = readl(ehci_base_addr | reg);
             printf("EHCI reg 0x%x (0x%lx) = 0x%lx\n",reg , ehci_base_addr|reg, value);
            }
            else
                goto usage;
            break;
                        
        case 'w':       /* write */
        case 'W':
            if (argc <= 4)
                goto usage;
            
            reg = simple_strtoul(argv[3], NULL, 16);
            value = simple_strtoul(argv[4], NULL, 16);
            if ((cmd1=='o') || (cmd1=='O')) {
                if (ohci_base_addr == 0) {
                 printf("OHCI Base Address = 0x%lx\n",ohci_base_addr);
                    return -1;
                }
                writel(value, ohci_base_addr | reg);
            }
            else if ((cmd1=='e') || (cmd1=='E')) {
                if (ehci_base_addr == 0) {
                 printf("EHCI Base Address = 0x%lx\n",ehci_base_addr);
                    return -1;
                }
                writel(value, ehci_base_addr | reg);
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

extern int ether_tsec_reinit(struct eth_device *dev);
extern int ether_loopback(struct eth_device *dev, int loopback, 
                              int max_pkt_len, int debug_flag, int reg_dump); 

typedef struct _tsec_mii_t {
    uint8_t reg;
    uint8_t pmap;
    char * name;
} tsec_mii_t;

tsec_mii_t tsec_mii_table[] = {
    { 0x00, 0x07, "control" },
    { 0x01, 0x03, "status" },
    { 0x02, 0x3f, "PHY ID 1" },
    { 0x03, 0x3f, "PHY ID 2" },
    { 0x04, 0x03, "auto adv base" },
    { 0x05, 0x03, "auto partner ab" },
    { 0x06, 0x03, "auto adv ex" },
    { 0x07, 0x03, "auto next" },
    { 0x08, 0x03, "auto partner next" },
    { 0x09, 0x01, "1000-T control" },
    { 0x0a, 0x01, "1000-T status" },
    { 0x0f, 0x03, "ext status" },
    { 0x10, 0x7f, "control reg 1" },
    { 0x11, 0x7f, "status reg 1" },
    { 0x12, 0x3f, "misc control" },
    { 0x13, 0x37, "status reg 2" },
    { 0x14, 0x30, "misc" },
    { 0x15, 0x23, "error" },
    { 0x16, 0x7f, "page" },
    { 0x1a, 0x27, "control reg 2" }
};

typedef struct _reg_desc_t {
    ushort regno;
    char * name;
} reg_desc_t;

reg_desc_t tbimii_desc_tbl[] = {
    {  0x0,   "CR"           },
    {  0x1,   "SR"           },
    {  0x4,   "ANA"          },
    {  0x5,   "ANLPBPA"      },
    {  0x6,   "ANEX"         },
    {  0x7,   "ANNPT"        },
    {  0x8,   "ANLPANP"      },
    {  0xF,   "EXST"         },
    { 0x10,   "JD"           },
    { 0x11,   "TBICON"       },
};

reg_desc_t mac_desc_tbl[] = {
    {  0x0,   "MACCFG1"      },
    {  0x1,   "MACCFG2"      },
    {  0x2,   "IPGIFG"       },
    {  0x3,   "HAFDUP"       },
    {  0x4,   "MAXFRM"       },
    {  0x8,   "MIIMCFG"      },
    {  0x9,   "MIIMCOM"      },
    {  0xA,   "MIIMADD"      },
    {  0xC,   "MIIMSTAT"     },
    {  0xD,   "MIIMIND"      },
    {  0xF,   "IFSTAT"       },
};

reg_desc_t tsec_desc_tbl[] = {
    {  0x0,   "TSEC_ID"      },
    {  0x1,   "TSEC_ID2"     },
    {  0x4,   "IEVENT"       },
    {  0x5,   "IMASK"        },
    {  0x6,   "EDIS"         },
    {  0x8,   "ECNTRL"       },
    {  0x9,   "PTV"          },
    {  0xA,   "DMACTRL"      },
    {  0xB,   "MIIMSTAT"     },
    {  0xC,   "TBIPA"        },
};

void tsec_phy_dump(char *dev)
{
   int i, j;
    uint16_t temp, temp1;
   
   printf("%s phy dump:\n",dev);
   printf("page %30s-0--  -1--  -2--  -3--  -4--  -5--  -6--\n"," ");
   for (i = 0; i < sizeof(tsec_mii_table)/sizeof(tsec_mii_table[0]); i++) {
       printf("Register %2d  %-20s", tsec_mii_table[i].reg, tsec_mii_table[i].name);
       for (j = 0; j < 7; j++) {
           if (tsec_mii_table[i].pmap & (1<<j)) {
               miiphy_read(dev, 1, 0x16, &temp);
               temp = ((temp & 0xff00) | j);
               miiphy_write(dev, 1, 0x16, temp);
               udelay(10);
               if (miiphy_read(dev, 1, tsec_mii_table[i].reg, &temp1) == 0)
                   printf("  %04x", temp1);
               else
                   printf("  ????");
           }
           else {
               printf("      ");
           }
       }
       printf("\n");
    }
    miiphy_read(dev, 1, 0x16, &temp);
    temp = (temp & 0xff00);
    miiphy_write(dev, 1, 0x16, temp);  /* back to page 0 */
    
}

void tsec_reg_dump(char *dev)
{
   int i;
   volatile uint32_t *regs = NULL;
   
   if (dev[5] == '0')
       regs = (volatile uint32_t *)(TSEC_BASE_ADDR);
   else if (dev[5] == '2')
       regs = (volatile uint32_t *)(TSEC_BASE_ADDR + 2 * TSEC_SIZE);
   
   if (regs != NULL) {
       printf("%s general control/status register dump:\n",dev);
       for (i = 0; i < sizeof(tsec_desc_tbl) / sizeof(tsec_desc_tbl[0]); i++)
           printf("0x%02x %-10s %08x\n", 
                   tsec_desc_tbl[i].regno, tsec_desc_tbl[i].name,
                   regs[tsec_desc_tbl[i].regno]);
   }
}

void mac_reg_dump(char *dev)
{
   int i;
   volatile uint32_t *regs = NULL;
   
   if (dev[5] == '0')
                regs = (volatile uint32_t *)(TSEC_BASE_ADDR + 0x500);
   else if (dev[5] == '2')
                regs = (volatile uint32_t *)(TSEC_BASE_ADDR + 2 * TSEC_SIZE + 0x500);
   if (regs != NULL) {
       printf("%s MAC register dump:\n",dev);
       for (i = 0; i < sizeof(mac_desc_tbl) / sizeof(mac_desc_tbl[0]); i++)
           printf("0x%02x %-10s %08x\n", 
                      mac_desc_tbl[i].regno, mac_desc_tbl[i].name,
                      regs[mac_desc_tbl[i].regno]);
   }
}

void tbi_reg_dump(char *dev)
{
   int i;
   uint16_t temp16;
   
   printf("%s TBI MII register dump:\n",dev);
   for (i = 0; i < sizeof(tbimii_desc_tbl) / sizeof(tbimii_desc_tbl[0]); i++) {
       if (tsec_tbimii_read(dev, tbimii_desc_tbl[i].regno, &temp16) == 0)
           printf("0x%02x %-10s %08x\n", tbimii_desc_tbl[i].regno, tbimii_desc_tbl[i].name, temp16);
       else
           printf("0x%02x %-10s N/A\n", tbimii_desc_tbl[i].regno, tbimii_desc_tbl[i].name);
   }
}
/* tsec commands
 *
 */
int do_tsec(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int i = 0;
    int loopback, debug_flag, count, speed;
    char cmd1, cmd2;
    uint16_t temp16;
    uint32_t temp32;
    int regindex = -1;
    char *dev;
    unsigned char regno;
    volatile uint32_t *regs = NULL;
    struct eth_device *eth_dev = NULL;
    
    if (argc <= 2)
        goto usage;
    
    cmd1 = argv[1][0];
    cmd2 = argv[2][0];
    dev = miiphy_get_current_dev();
    
    switch (cmd1) {                        
        case 'p':       /* mgmt phy */
        case 'P':
            if ((cmd2 == 'd') || (cmd2 == 'D')) { /* dump */
                tsec_phy_dump(dev);
            }
            else
                goto usage;
            
            break;
                        
        case 'c':
        case 'C':
            if (argc <= 2)
                goto usage;
            
            if (dev[5] == '2')
                regs = (volatile uint32_t *)(TSEC_BASE_ADDR + 2 * TSEC_SIZE);
            else
                regs = (volatile uint32_t *)(TSEC_BASE_ADDR);
            
            switch (cmd2) {
                case 'd':
                case 'D':
                    tsec_reg_dump(dev);
                    break;
                    
                case 'r':
                case 'R':
                    if (argc <= 3)
                        goto usage;
                    regno = simple_strtoul(argv[3], NULL, 16);
                    for (i = 0; i < sizeof(tsec_desc_tbl) /sizeof(tsec_desc_tbl[0]); i++) {
                        if (regno == tsec_desc_tbl[i].regno)
                            regindex = i;
                    }

                    if (regindex < 0) {
                        printf("register number out of range %d - %d\n", 
                            tsec_desc_tbl[0].regno, tsec_desc_tbl[i].regno);
                        return 1;
                    }
                    printf("    %-10s 0x%x\n", tsec_desc_tbl[regindex].name,
                               regs[regno]);
                    break;
                    
                case 'w':
                case 'W':
                    if (argc <= 4)
                        goto usage;
                    regno = simple_strtoul(argv[3], NULL, 16);
                    temp32 = simple_strtoul(argv[4], NULL, 16);
                    for (i = 0; i < sizeof(tsec_desc_tbl) /sizeof(tsec_desc_tbl[0]); i++) {
                        if (regno == tsec_desc_tbl[i].regno)
                            regindex = i;
                    }

                    if (regindex < 0) {
                        printf("register number out of range %d - %d\n", 
                            tsec_desc_tbl[0].regno, tsec_desc_tbl[i].regno);
                        return 1;
                    }
                    regs[regno] = temp32;
                    break;
            }
            break;

        case 'm':
        case 'M':
            if (argc <= 2)
                goto usage;
            
            if (dev[5] == '2')
                regs = (volatile uint32_t *)(TSEC_BASE_ADDR + 2 * TSEC_SIZE + 0x500);
            else
                regs = (volatile uint32_t *)(TSEC_BASE_ADDR + 0x500);
                
            switch (cmd2) {
                case 'd':
                case 'D':
                    mac_reg_dump(dev);
                    break;
                    
                case 'r':
                case 'R':
                    if (argc <= 3)
                        goto usage;
                    regno = simple_strtoul(argv[3], NULL, 16);
                    for (i = 0; i < sizeof(mac_desc_tbl) /sizeof(mac_desc_tbl[0]); i++) {
                        if (regno == mac_desc_tbl[i].regno)
                            regindex = i;
                    }

                    if (regindex < 0) {
                        printf("register number out of range %d - %d\n", 
                            mac_desc_tbl[0].regno, mac_desc_tbl[i].regno);
                        return 1;
                    }
                    printf("    %-10s 0x%x\n", mac_desc_tbl[regindex].name,
                               regs[regno]);
                    break;
                    
                case 'w':
                case 'W':
                    if (argc <= 4)
                        goto usage;
                    regno = simple_strtoul(argv[3], NULL, 16);
                    temp32 = simple_strtoul(argv[4], NULL, 16);
                    for (i = 0; i < sizeof(mac_desc_tbl) /sizeof(mac_desc_tbl[0]); i++) {
                        if (regno == mac_desc_tbl[i].regno)
                            regindex = i;
                    }

                    if (regindex < 0) {
                        printf("register number out of range %d - %d\n", 
                            mac_desc_tbl[0].regno, mac_desc_tbl[i].regno);
                        return 1;
                    }
                    regs[regno] =  temp32;
                    break;
            }
            break;
                        
        case 't':
        case 'T':
            switch (cmd2) {
                case 'd':
                case 'D':
                    if (argc <= 2)
                        goto usage;
                    tbi_reg_dump(dev);
                    break;
                    
                case 'r':
                case 'R':
                    if (argc <= 3)
                        goto usage;
                    regno = simple_strtoul(argv[3], NULL, 16);
                    for (i = 0; i < sizeof(tbimii_desc_tbl) /sizeof(tbimii_desc_tbl[0]); i++) {
                        if (regno == tbimii_desc_tbl[i].regno)
                            regindex = i;
                    }

                    if (regindex < 0) {
                        printf("register number out of range %d - %d\n", 
                            tbimii_desc_tbl[0].regno, tbimii_desc_tbl[i].regno);
                        return 1;
                    }

                    if (tsec_tbimii_read(dev, regno, &temp16) == 0)
                        printf("    %-10s 0x%x\n", tbimii_desc_tbl[regindex].name, temp16);
                    else
                        printf("    %-10s N/A\n", tbimii_desc_tbl[regindex].name);
                    break;
                    
                case 'w':
                case 'W':
                    if (argc <= 4)
                        goto usage;
                    regno = simple_strtoul(argv[3], NULL, 16);
                    temp16 = simple_strtoul(argv[4], NULL, 16);
                    for (i = 0; i < sizeof(tbimii_desc_tbl) /sizeof(tbimii_desc_tbl[0]); i++) {
                        if (regno == tbimii_desc_tbl[i].regno)
                            regindex = i;
                    }

                    if (regindex < 0) {
                        printf("register number out of range %d - %d\n", 
                            tbimii_desc_tbl[0].regno, tbimii_desc_tbl[i].regno);
                        return 1;
                    }
                    tsec_tbimii_write(dev, regno, temp16);
                    break;
            }
            break;
            
        case 'i':
        case 'I':    /* re-init */
            eth_dev = eth_get_dev_by_name(dev);
            ether_tsec_reinit(eth_dev);
            break;
            
        case 'l':
        case 'L':   /* loopback */
            eth_dev = eth_get_dev_by_name(dev);
            debug_flag = 0;
            if ((cmd2 == 'm') || (cmd2 == 'M')) { /* MAC */
                loopback = 0;
                count = simple_strtoul(argv[3], NULL, 10);
                if ((argc == 5) && ((argv[4][0]=='d') || (argv[5][0]=='D')))
                    debug_flag = 1;
            } else if ((cmd2 == 'p') || (cmd2 == 'P')) { /* PHY */
                loopback = 1;
                count = simple_strtoul(argv[3], NULL, 10);
                if ((argc == 5) && ((argv[4][0]=='d') || (argv[5][0]=='D')))
                    debug_flag = 1;
            } else if ((cmd2 == 'e') || (cmd2 == 'E')) { /* EXT */
                speed = simple_strtoul(argv[3], NULL, 10);
                if (speed == 10)
                    loopback = 2;
                else if (speed == 100)
                    loopback = 3;
                else if (speed == 1000)
                    loopback = 4;
                else
                    goto usage;
                count = simple_strtoul(argv[4], NULL, 10);
                if ((argc == 6) && ((argv[5][0]=='d') || (argv[5][0]=='D')))
                    debug_flag = 1;
            }
            else
                goto usage;
                
	     for (i = 0; ((i < count) || (count == 0)); i++) {
                if (ctrlc()) {
			putc ('\n');
			return 0;
		  }

                if ((loopback == 2) || (loopback == 3) || (loopback == 4))
		      printf("%s %s %s loopback: %d   ", dev, argv[2], argv[3], i+1);
                else
		      printf("%s %s loopback: %d   ", dev, argv[2], i+1);
                    
                if (ether_loopback(eth_dev, loopback, 1518, debug_flag, 1) == 0) {
		      printf(" passed.");
                }
                else {
		      printf(" failed.");
                }
		  printf("\n");
	     }
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
#endif /* (CONFIG_COMMANDS & CFG_CMD_BSP) */


#define FLASH_CMD_PROTECT_SET       0x01
#define FLASH_CMD_PROTECT_CLEAR 0xD0
#define FLASH_CMD_CLEAR_STATUS  0x50
#define FLASH_CMD_PROTECT              0x60

#define AMD_CMD_RESET                  0xF0
#define AMD_CMD_WRITE                  0xA0
#define AMD_CMD_ERASE_START     0x80
#define AMD_CMD_ERASE_SECTOR        0x30
#define AMD_CMD_UNLOCK_START        0xAA
#define AMD_CMD_UNLOCK_ACK      0x55

#define AMD_STATUS_TOGGLE              0x40

/* simple flash 16 bits write, do not accross sector */
int flash_write_direct (ulong addr, char *src, ulong cnt)
{
    volatile uint16_t *faddr = (uint16_t *) addr;
    volatile uint16_t *faddrs = (uint16_t *) addr;
    volatile uint16_t *faddr1 = (uint16_t *)((addr & 0xFFFFF000) | 0xAAA);
    volatile uint16_t *faddr2 = (uint16_t *)((addr & 0xFFFFF000) | 0x554);
    uint16_t *data = (uint16_t *) src;
    ulong start, saddr;
    int i;

    if ((addr >= 0xff800000) && (addr < 0xff80ffff)) {  /* 8k sector */
        saddr = addr & 0xffffe000;
    }
    else if ((addr >= 0xff810000) && (addr < 0xffefffff)) { /* 64k sector */
        saddr = addr & 0xffff0000;
    }
    else if ((addr >= 0xfff00000) && (addr < 0xffffffff)) {  /* 8k sector */
        saddr = addr & 0xffffe000;
    }
    else {
        printf("flash address 0x%x out of range 0xFF800000 - 0xFFFFFFFF\n", addr);
        return 1;
    }
    faddrs = (uint16_t *)saddr;
    faddr1 = (uint16_t *)((saddr & 0xFFFFF000) | 0xAAA);
    faddr2 = (uint16_t *)((saddr & 0xFFFFF000) | 0x554);

    /* un-protect sector */
    *faddrs = FLASH_CMD_CLEAR_STATUS;
    *faddrs = FLASH_CMD_PROTECT;
    *faddrs = FLASH_CMD_PROTECT_CLEAR;
    start = get_timer (0);
    while ((*faddrs & AMD_STATUS_TOGGLE) != (*faddrs & AMD_STATUS_TOGGLE)) {
        if (get_timer (start) > 0x2000) {
            *faddrs = AMD_CMD_RESET;
            return 1;
        }
        udelay (1);
    }
    
    /* erase sector */
    *faddr1 = AMD_CMD_UNLOCK_START;
    *faddr2 = AMD_CMD_UNLOCK_ACK;
    *faddr1 = AMD_CMD_ERASE_START;
    *faddr1 = AMD_CMD_UNLOCK_START;
    *faddr2 = AMD_CMD_UNLOCK_ACK;
    *faddrs = AMD_CMD_ERASE_SECTOR;
    start = get_timer (0);
    while ((*faddrs & AMD_STATUS_TOGGLE) != (*faddrs & AMD_STATUS_TOGGLE)) {
        if (get_timer (start) > 0x2000) {
            *faddrs = AMD_CMD_RESET;
            return 1;
        }
        udelay (1);
    }

    for (i = 0; i < cnt/2; i++) {
        /* write data byte */
        *faddr1 = AMD_CMD_UNLOCK_START;
        *faddr2 = AMD_CMD_UNLOCK_ACK;
        *faddr1 = AMD_CMD_WRITE;

        *(faddr+i) = data[i];
        start = get_timer (0);
        while ((*faddrs & AMD_STATUS_TOGGLE) != (*faddrs & AMD_STATUS_TOGGLE)) {
            if (get_timer (start) > 0x100) {
                *faddrs = AMD_CMD_RESET;
                return 1;
            }
            udelay (1);
        }
    }

    /* protect sector */
    *faddrs = FLASH_CMD_CLEAR_STATUS;
    *faddrs = FLASH_CMD_PROTECT;
    *faddrs = FLASH_CMD_PROTECT_SET;
    start = get_timer (0);
    while ((*faddrs & AMD_STATUS_TOGGLE) != (*faddrs & AMD_STATUS_TOGGLE)) {
        if (get_timer (start) > 0x2000) {
            *faddrs = AMD_CMD_RESET;
            return 1;
        }
        udelay (1);
    }

   return 0;
}

/* upgrade commands
 *
 * Syntax:
 *    upgrade get
 *    upgrade set <state>
 */
int do_upgrade(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char cmd1;
    int state;
    volatile int *ustate = (int *) (CFG_FLASH_BASE + CFG_UPGRADE_BOOT_STATE_OFFSET);

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
                return 1;
            
            if (flash_write_direct(CFG_FLASH_BASE + CFG_UPGRADE_BOOT_STATE_OFFSET,
                (char *)&state, sizeof(int)) != 0)
                printf("upgrade set state 0x%x failed.\n", state);
                return 0;
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

int num_test;
int mac_set;
int phy_set;
int ext_set;
uint bd_id;
#define EX3242_DIAG_NUM 9
static char *ex3242_diag[EX3242_DIAG_NUM] =
    {"cpu", "ram", "i2c", "pci", "usb", "ethernet", 
     "uart", "watchdog"};

int do_diag (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
    unsigned int i;
    unsigned int j;
    int cmd_flag = POST_RAM | POST_MANUAL;
    int res = 0;
    
    mac_set = 0;
    phy_set = 0;
    ext_set = 0;
    bd_id = 0;

    if (argc == 1 || strcmp (argv[1], "run") != 0) {
        /* List test info */
        if (argc == 1) {
            puts ("Available hardware tests:\n");
            post_info (NULL);
            puts ("Use 'diag [<test1> [<test2> ...]]'"
                  " to get more info.\n");
            puts ("Use 'diag run [<test1> [<test2> ...]]'"
                  " to run tests.\n");
        } 
        else {
            for (i = 1; i < argc; i++) {
                if (post_info (argv[i]) != 0)
                    printf ("%s - no such test\n", argv[i]);
                }
        }
    } 
    else {
        /* Run tests */
        if ((argc == 2) ||
            ((argc == 3) && (!strcmp(argv[argc-1], "-v")))) {
            if ((argc == 3) && (!strcmp(argv[argc-1], "-v")))
                cmd_flag = POST_RAM | POST_MANUAL | POST_DEBUG;
            
            if (!strcmp(argv[1], "run")) {
                for (i = 0; i < EX3242_DIAG_NUM -3; i++) {
                    if (post_run(ex3242_diag[i], cmd_flag) != 0)
                        res = -1;
                }
                if (res)
                    printf("\n- EX3242 test                           FAILED"
                           "@\n");
                else
                    printf("\n- EX3242 test                           PASSED"
                           "\n");
            }
            else
                puts(" Please specify the test name \n");
        }
        if (argc == 5 || argc == 7 || argc == 8 ) {
            if (!strcmp(argv[argc-4], "TSEC0")) {
                if (!strcmp(argv[argc-3], "MAC_LB")){
                    num_test = simple_strtoul(argv[6], NULL, 10);
                    mac_set = 1;
                }
                if (!strcmp(argv[argc-3], "PHY_LB")){
                    num_test = simple_strtoul(argv[6], NULL, 10);
                    phy_set = 1;
                }
                if (!strcmp(argv[argc-3], "10Mbs")){
                    num_test = simple_strtoul(argv[6], NULL, 10);
                    ext_set = 1;
                }
                if (!strcmp(argv[argc-3], "100Mbs")){
                    num_test = simple_strtoul(argv[6], NULL, 10);
                    ext_set = 2;
                }
                if (!strcmp(argv[argc-3], "PORT_LB")){
                    num_test = simple_strtoul(argv[6], NULL, 10);
                    ext_set = 3;
                }
                printf("The total number of test %d\n", num_test);
                for (j = 0; j <=  num_test; j++) {
                    printf("The current test number : %d\n", j);
                    for (i = 2; i < argc - 4; i++) {
                        if (post_run (argv[i], POST_RAM | POST_MANUAL) != 0)
                            res = -1;
                    }
                }
            }
            if (!strcmp(argv[argc-4], "TSEC2")) {
                if (!strcmp(argv[argc-3], "MAC_LB")) {
                    mac_set = 2;
                    num_test = simple_strtoul(argv[6], NULL, 10);
                    printf("The total number of test %d\n", num_test);
                    for (j = 0; j <= num_test; j++) {
                        printf("The current test number : %d\n",j);
                        for (i = 2; i < argc - 4; i++) {
                            if (post_run(argv[i], POST_RAM | POST_MANUAL) != 0)
                                res = -1;
                        }
                    }
                }
            }
            if (!strcmp(argv[argc-5], "JAVA_BD_ID")) {
                bd_id = simple_strtoul(argv[4], NULL, 16);
                num_test = simple_strtoul(argv[6], NULL, 10);
                printf("The total number of test %d\n", num_test);
                for (j = 0; j <= num_test; j++) {
                    printf("The current test number : %d\n", j);
                    for (i = 2; i < argc - 5; i++) {
                        if (!strcmp(argv[7], "-v")) {
                            if (post_run (argv[i], POST_RAM | POST_MANUAL | POST_DEBUG) != 0)
                                res = -1;
                        }
                        else {
                            if (post_run(argv[i], POST_RAM | POST_MANUAL) != 0)
                                res = -1;
                        }
                    }
                }
            }
            
            if (argc == 5) {
                num_test = simple_strtoul(argv[4], NULL, 10);
                if (!strcmp(argv[3], "test")) {
                    printf("The total number of test %d\n", num_test);
                    for (j = 0; j <= num_test; j++) {
                        printf("The current test number : %d\n", j);
                        for (i = 2; i < argc - 2; i++) {
                            if (post_run(argv[i], POST_RAM | POST_MANUAL) != 0)
                                res = -1;
                        }
                    }
                }
                puts(" Please specify the proper test command \n");
            }
        }
        else {
            if (!strcmp(argv[argc-1], "-v")) {
                if (argc == 3){
                    puts(" Please specify the test name \n");
                }
                else {
                    for (i = 2; i < argc - 1; i++) {
                        if (post_run(argv[i], POST_RAM | POST_MANUAL | POST_DEBUG) != 0)
                            res = -1;
                    }
                }
            }
            else {
                for (i = 2; i < argc; i++) {
                    if (post_run(argv[i], POST_RAM | POST_MANUAL) != 0)
                        res = -1;
                }
            }
        }
    }

    return 0;
}

/***************************************************/

U_BOOT_CMD(
    storage,    7,    1,     do_storage,
    "storage - storage device block utility\n",
    "\n"
    "storage read [usb | nor] <device> <block> <dest> <count>\n"
    "    - read # blocks from device block offset to dest memory location\n"
#if defined(USB_WRITE_READ)
    "storage write [usb | nor] <device> <block> <src> <size>\n"
    "    - write source memory to device block offset location\n"
#endif
);

U_BOOT_CMD(
    isp,    5,  1,  do_isp,
    "isp     - isp1564 commands\n",
    "\n"
    "isp [ohci |ehci] address\n"
    "    - ohci/ehci PCI base address/off\n"
    "isp [ohci |ehci] dump\n"
    "    - ohci/ehci register dump\n"
    "isp [ohci |ehci] read <reg>\n"
    "    - ohci/ehci register read\n"
    "isp [ohci |ehci] write <reg> <value>\n"
    "    - ohci/ehci register write\n"
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
);

U_BOOT_CMD(
    pcieprobe,     3,      1,       do_pcie_probe,
    "pcieprobe  - List the PCI-E devices found on the bus\n",
    "<controller> [-v]/[-d](v - verbose d - dump)\n"
);

U_BOOT_CMD(
    tsec,   6,  1,  do_tsec,
    "tsec    - tsec utility commands\n",
    "\n"
    "tsec phy dump\n"
    "    - dump phy registers\n"
    "tsec cs read  <reg>\n"
    "    - read control/status register <reg>\n"
    "tsec cs write  <reg> <data>\n"
    "    - write control/status register <reg>\n"
    "tsec cs dump\n"
    "    - general control/status register dump\n"
    "tsec mac read <reg>\n"
    "    - read mac register <reg>\n"
    "tsec mac write <reg> <data>\n"
    "    - write MAC register <reg>\n"
    "tsec mac dump\n"
    "    - MAC register dump\n"
    "tsec tbi read  <reg>\n"
    "    - read TBI MII register <reg>\n"
    "tsec tbi write  <reg> <value>\n"
    "    - write TBI MII register <reg>\n"
    "tsec tbi dump\n"
    "    - TBI MII register dump\n"
    "tsec init\n"
    "    - init tsec\n"
    "tsec loopback [mac | phy | ext [10 | 100 | 1000]] <count> [debug]\n"
    "    - tsec loopback\n"
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

