/*
 * Copyright (c) 2010, Juniper Networks, Inc.
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
#include <pci.h>
#include <pcie.h>
#include <miiphy.h>
#include <tsec.h>
#include <post.h>
#include <i2c.h>
#include <ether.h>
#include "ex45xx_local.h"
#include "cmd_ex45xx.h"
#if !defined(CONFIG_PRODUCTION)
#include "hush.h"
#include "../ex3242/fsl_i2c.h"
#include "epld.h"
#include "epld_watchdog.h"
#include "led.h"
#include "lcd.h"
#include <ns16550.h>
#endif /* CONFIG_PRODUCTION */
#endif  /* CFG_CMD_BSP */

DECLARE_GLOBAL_DATA_PTR;

int i2c_manual = 0;
int i2c_mfgcfg = 0;

/* for M1-SFP+ and M2-LBK */
int card_init[CFG_EX4500_LAST_CARD] = { 1, 0, 0, 0 };

int flash_write_direct (ulong addr, char *src, ulong cnt);

#if (CONFIG_COMMANDS & CFG_CMD_BSP)
extern block_dev_desc_t usb_dev_desc[USB_MAX_STOR_DEV];

extern char console_buffer[];

extern struct pcie_controller pcie_hose[];

extern struct tsec_private *privlist1[];

static void pcie_header_show_brief (int, pcie_dev_t );
static void pcie_header_show (int, pcie_dev_t );

#if !defined(CONFIG_PRODUCTION)

int i2c_cksum = 0;

#endif /* CONFIG_PRODUCTION */

ulong 
ex_pci_error_get (void)
{
    volatile immap_t    *immap = (immap_t *)CFG_CCSRBAR;
    volatile ccsr_pcix_t *pcix = &immap->im_pcix;
    volatile uint32_t perr = pcix->pedr;

    printf("PCI error register: %08x\n", perr);
    if (perr & 0x7ff) {
        if (perr & 0x400) printf("-- address parity error\n");
        if (perr & 0x200) printf("-- received SERR error\n");
        if (perr & 0x100) printf("-- master PERR error\n");
        if (perr & 0x80) printf("-- target PERR error\n");
        if (perr & 0x40) printf("-- master abort error\n");
        if (perr & 0x20) printf("-- target abort error\n");
        if (perr & 0x10) 
            printf("-- outbound write memory space violation error\n");
        if (perr & 0x8) 
            printf("-- outbound read memory space violation error\n");
        if (perr & 0x4) 
            printf("-- inbound read memory space violation error\n");
        if (perr & 0x2) printf("-- split completion message error\n");
        if (perr & 0x1) printf("-- time out error\n");
    }
    
    return (perr);
}

void 
ex_pci_error_clear (void)
{
    volatile immap_t    *immap = (immap_t *)CFG_CCSRBAR;
    volatile ccsr_pcix_t *pcix = &immap->im_pcix;

    pcix->pedr = 0x800007ff;
}

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

#if !defined(CONFIG_PRODUCTION)
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
    return ((int8_t)temp);
}

static int 
twos_complement_16 (uint16_t temp)
{
    return ((int16_t)temp);
}

/* i2c command structures */
typedef struct _hw_cmd_ {
    unsigned char  i2c_cmd;
    unsigned char  flags;
    unsigned short len;
} hw_cmd_t;

typedef struct _eeprom_cmd_ {
   hw_cmd_t         cmd;
   unsigned short   eeprom_addr;
   unsigned short   eeprom_len;
   unsigned char    buf[MAX_BUF_LEN];
} eeprom_cmd_t;

static int
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
        eeprom_cmd.eeprom_len = length;
        eeprom_cmd.eeprom_addr = addr;

        eeprom_cmd.cmd.i2c_cmd = WRITE_EEPROM_DATA;
        eeprom_cmd.cmd.flags = 0;
        eeprom_cmd.cmd.len = sizeof(hw_cmd_t) + 4 + length;

        memcpy((void *)eeprom_cmd.buf, in_buf, length);

        status = i2c_write (i2c_addr, 0, 0,
            (uint8_t *) &eeprom_cmd, eeprom_cmd.cmd.len);
        if (status) {
            printf("secure eeprom write failed status = %d.\n", status);
            return (status);
        }

        udelay (50000);  /* ~500 bytes */

    return (status);
}
#endif /* CONFIG_PRODUCTION */


/*---------------------------------------------*/

/* Show information about devices on PCIe bus. */

struct pcie_list
{
    uint32_t    lane;  /* bitmap */
    uint32_t    bus;
    uint32_t    dev;
    uint32_t    fun;
    uint16_t    ven_id;
    uint16_t    dev_id;
    uint32_t    up_bus;
    uint32_t    up_dev;
    uint32_t    up_fun;
    uint32_t    link_reg;
    uint32_t    link_status;
    uint32_t    link_op;  /* 0 - equal, 1 - bit and */
    char        dev_name[30];
};

static struct pcie_list pcie_dev_list[] =  /* by lane number */
{
    {
        /* PEX8618 */
        0xF,  /* lane 0-3 */
        1,  /* bus */
        0,  /* device */
        0,  /* function */
        PCIE_PEX8618_VENDOR_ID,  /* vendor ID */
        PCIE_PEX8618_DEVICE_ID,  /* device ID */
        0,
        0,
        0,
        0x404,
        0x16,
        0,
        "PEX8618",
    },
    {
        /* Lion-1 PCIe-0 */
        0x40,  /* lane 6 */
        0xb,  /* bus */
        0,  /* device */
        0,  /* function */
        PCIE_LION_VENDOR_ID,  /* vendor ID */
        PCIE_LION_DEVICE_ID,  /* device ID */
        2,
        0xc,
        0,
        0x78,
        0x20000000,
        1,
        "Lion-1 PCIe-0 (lane 6)",
    },
    {
        /* Lion-1 PCIe-1 */
        0x80,  /* lane 7 */
        0xd,  /* bus */
        0,  /* device */
        0,  /* function */
        PCIE_LION_VENDOR_ID,  /* vendor ID */
        PCIE_LION_DEVICE_ID,  /* device ID */
        2,
        0xe,
        0,
        0x78,
        0x20000000,
        1,
        "Lion-1 PCIe-1 (lane 7)",
    },
    {
        /* Lion-1 PCIe-2 */
        0x80,  /* lane 8 */
        0x3,  /* bus */
        0,  /* device */
        0,  /* function */
        PCIE_LION_VENDOR_ID,  /* vendor ID */
        PCIE_LION_DEVICE_ID,  /* device ID */
        2,
        1,
        0,
        0x78,
        0x20000000,
        1,
        "Lion-1 PCIe-2 (lane 8)",
    },
    {
        /* Lion-1 PCIe-3 */
        0x100,  /* lane 9 */
        0x3,  /* bus */
        0,  /* device */
        0,  /* function */
        PCIE_LION_VENDOR_ID,  /* vendor ID */
        PCIE_LION_DEVICE_ID,  /* device ID */
        2,
        5,
        0,
        0x78,
        0x20000000,
        1,
        "Lion-1 PCIe-3 (lane 9)",
    },
    {
        /* Lion-2 PCIe-0 */
        0x400,  /* lane 10 */
        0x7,  /* bus */
        0,  /* device */
        0,  /* function */
        PCIE_LION_VENDOR_ID,  /* vendor ID */
        PCIE_LION_DEVICE_ID,  /* device ID */
        2,
        7,
        0,
        0x78,
        0x20000000,
        1,
        "Lion-2 PCIe-0 (lane 10)",
    },
    {
        /* Lion-2 PCIe-1 */
        0x800,  /* lane 11 */
        0x8,  /* bus */
        0,  /* device */
        0,  /* function */
        PCIE_LION_VENDOR_ID,  /* vendor ID */
        PCIE_LION_DEVICE_ID,  /* device ID */
        2,
        9,
        0,
        0x78,
        0x20000000,
        1,
        "Lion-2 PCIe-1 (lane 11)",
    },
    {
        /* Lion-2 PCIe-2 */
        0x1000,  /* lane 12 */
        0x5,  /* bus */
        0,  /* device */
        0,  /* function */
        PCIE_LION_VENDOR_ID,  /* vendor ID */
        PCIE_LION_DEVICE_ID,  /* device ID */
        2,
        3,
        0,
        0x78,
        0x20000000,
        1,
        "Lion-2 PCIe-2 (lane 12)",
    },
    {
        /* Lion-2 PCIe-3 */
        0x2000,  /* lane 13 */
        0xa,  /* bus */
        0,  /* device */
        0,  /* function */
        PCIE_LION_VENDOR_ID,  /* vendor ID */
        PCIE_LION_DEVICE_ID,  /* device ID */
        2,
        0xb,
        0,
        0x78,
        0x20000000,
        1,
        "Lion-2 PCIe-3 (lane 13)",
    },
};

static void 
pcieinfo (int controller, int busnum, int short_listing)
{
    unsigned int device = 0, function = 0;
    unsigned char header_type;
    unsigned short vendor_id;
    pcie_dev_t dev;
    
    printf("\nScanning PCIE[%d] devices on bus %d\n", controller, busnum);
    
    if (short_listing) {
        printf("BusDevFun  VendorId   DeviceId   Device Class     Sub-Class");
        printf("\n");
        printf("-----------------------------------------------------------");
        printf("\n");
    }

    for (device = 0; device < PCIE_MAX_PCIE_DEVICES; device++) {
        header_type = 0;
        vendor_id = 0;

        for (function = 0; function < PCIE_MAX_PCIE_FUNCTIONS; function++) {
            
            /* If this is not a multi-function device, we skip the rest. */
            if (function && !(header_type & 0x80))
                break;

            dev = PCIE_BDF(busnum, device, function);
    
            pcie_hose_read_config_word(&pcie_hose[controller-1], 
                                       dev, PCIE_VENDOR_ID, &vendor_id);
            if ((vendor_id == 0xFFFF) || (vendor_id == 0x0000))
                continue;
    
            pcie_hose_read_config_byte(&pcie_hose[controller-1], 
                                       dev, PCIE_HEADER_TYPE, &header_type);
        
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

static void 
pciedump (int controller, int busnum)
{
    pcie_dev_t dev;
    unsigned char buff[256], temp[20];
    int addr, offset = 0;
    int linebytes;
    int nbytes = 256;
    int i, j = 0;

    printf("\nScanning PCIE[%d] devices on bus %d\n", controller, busnum);

    dev = PCIE_BDF(busnum, 0, 0);
    memset(buff,0xff,256);

    for (addr = 0 ; addr<265 ; addr++) 
    pcie_hose_read_config_byte(&pcie_hose[controller-1], 
                               dev, addr,(buff+addr));

    do {
    printf("0x%02x :",offset);
    linebytes = (nbytes < 16)?nbytes:16;

    for (i = 0 ;i < linebytes ;i++) {
        if ((i == 4) || (i == 8)|| (i == 12))
            printf(" ");

        temp[i] = buff[j];
        printf(" %02x",buff[j]);
        j++;
    }
    printf("  ");

    for (i = 0 ;i < linebytes ;i++) {
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
    
static void 
pcie_header_show_brief (int controller, pcie_dev_t dev)
{
    unsigned short vendor, device;
    unsigned char class, subclass;

    pcie_hose_read_config_word(&pcie_hose[controller-1], 
                               dev, PCIE_VENDOR_ID, &vendor);
    pcie_hose_read_config_word(&pcie_hose[controller-1], 
                               dev, PCIE_VENDOR_ID, &vendor);
    pcie_hose_read_config_word(&pcie_hose[controller-1], 
                               dev, PCIE_DEVICE_ID, &device);
    pcie_hose_read_config_byte(&pcie_hose[controller-1], 
                               dev, PCIE_CLASS_CODE, &class);
    pcie_hose_read_config_byte(&pcie_hose[controller-1], 
                               dev, PCIE_CLASS_SUB_CODE, &subclass);
                
    printf("0x%.4x     0x%.4x     %-23s 0x%.2x\n", 
           vendor, device, pcie_classes_str(class), subclass);
}

/* Reads the header of the specified PCIe device
 * Inputs: dev  bus+device+function number
 */

static void 
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
        PRINT ("sub system vendor ID = 0x%.4x\n", 
               word, PCIE_SUBSYSTEM_VENDOR_ID);
        PRINT ("sub system ID = 0x%.4x\n", word, PCIE_SUBSYSTEM_ID);
        PRINT ("interrupt line = 0x%.2x\n", byte, PCIE_INTERRUPT_LINE);
        PRINT ("interrupt pin = 0x%.2x\n", byte, PCIE_INTERRUPT_PIN);
        break;

    case PCIE_HEADER_TYPE_BRIDGE:   /* PCIE-to-PCIE bridge */
        PRINT ("base address 1 = 0x%.8x\n", dword, PCIE_BASE_ADDRESS_1);
        PRINT ("primary bus number = 0x%.2x\n", byte, PCIE_PRIMARY_BUS);
        PRINT ("secondary bus number = 0x%.2x\n", byte,PCIE_SECONDARY_BUS);
        PRINT ("subordinate bus number = 0x%.2x\n", 
               byte, PCIE_SUBORDINATE_BUS);
        PRINT ("IO base = 0x%.2x\n", byte, PCIE_IO_BASE);
        PRINT ("IO limit = 0x%.2x\n", byte, PCIE_IO_LIMIT);
        PRINT ("memory base = 0x%.4x\n", word, PCIE_MEMORY_BASE);
        PRINT ("memory limit = 0x%.4x\n", word, PCIE_MEMORY_LIMIT);
        PRINT ("prefetch memory base = 0x%.4x\n", 
               word, PCIE_PREF_MEMORY_BASE);
        PRINT ("prefetch memory limit = 0x%.4x\n", 
               word, PCIE_PREF_MEMORY_LIMIT);
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
    pcie_hose_read_config_word(&pcie_hose[controller-1],
                               dev, PCIE_VENDOR_ID, &vendor_id);
    pcie_hose_read_config_byte(&pcie_hose[controller-1], 
                               dev, PCIE_HEADER_TYPE, &header_type);
    pcie_hose_read_config_word(&pcie_hose[controller-1], 
                               dev, PCIE_DEVICE_ID, &device_id);
    pcie_hose_read_config_byte(&pcie_hose[controller-1], 
                               dev, PCIE_CLASS_CODE, &class_code);
    pcie_hose_read_config_byte(&pcie_hose[controller-1], 
                               dev, PCIE_CLASS_SUB_CODE, &subclass);
    pcie_hose_read_config_byte(&pcie_hose[controller-1], 
                               0, 0x404, &linkstat);
        
    printf("%d\t" "%d\t" "0\t" "0\t" "0x%.4x\t" "0x%.4x\t", 
           controller, busnum, vendor_id, device_id);
    printf("%-23s\t0x%.2x\t", pcie_classes_str(class_code), subclass);
    printf("0x%2X", linkstat);
    printf("\n");
    return (linkstat);
}

static void 
do_pcie_status (void)
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

static int 
stress_pcie_up (int controller, int retries, uint32_t lanemap)
{   
    int i, j = 0, lane;
    unsigned char ls;
    unsigned short vendor_id;
    unsigned short device_id;

    if (lanemap == 0xFFFFFFFF)
        printf("PCIE stresstest ctrl %d - all lanes up   (%d retries) ... ", 
               controller, retries);
    else {
        for (lane = 0; lane < 15; lane++) {
            if (lane & lanemap)
                break;
        }
        for (j = 0; j < sizeof(pcie_dev_list) / sizeof(pcie_dev_list[0]); 
             lane++) {
            if (lane & pcie_dev_list[j].lane)
                break;
        }
        if (j < sizeof(pcie_dev_list) / sizeof(pcie_dev_list[0]))
            printf("PCIE stresstest ctrl %d - %s up   (%d retries) ... ", 
                   controller, pcie_dev_list[j].dev_name, retries);
        else {
            printf("PCIE stresstest ctrl %d - lane %d not mapped   "
                   "(%d retries) ... ", 
                   controller, lane, retries);
            return (1);
        }
    }
    pcie_hose_read_config_word(&pcie_hose[controller-1], 
                               PCIE_BDF(pcie_dev_list[j].bus, 
                               pcie_dev_list[j].dev, pcie_dev_list[j].fun), 
                               PCIE_VENDOR_ID,
                               &vendor_id);
    pcie_hose_read_config_word(&pcie_hose[controller-1], 
                               PCIE_BDF(pcie_dev_list[j].bus, 
                               pcie_dev_list[j].dev, pcie_dev_list[j].fun), 
                               PCIE_DEVICE_ID,
                               &device_id);
    if (vendor_id != pcie_dev_list[j].ven_id)
    {
        printf("Invalid Vendor ID detected : %04X  expected : %04x\n", 
               vendor_id, pcie_dev_list[j].ven_id);
        return (1);
    }
    if (device_id != pcie_dev_list[j].dev_id)
    {
        printf("Invalid Device ID detected : %04X expected : %04X\n",
               device_id, pcie_dev_list[j].dev_id);
        return (1);
    }
    for (i = 0; i < retries; i++) 
    {
        pcie_hose_read_config_byte(&pcie_hose[controller-1], 
                                   PCIE_BDF(pcie_dev_list[j].up_bus, 
                                   pcie_dev_list[j].up_dev, 
                                   pcie_dev_list[j].up_fun), 
                                   pcie_dev_list[j].link_reg, 
                                   &ls);
        if (pcie_dev_list[j].link_op == 0) {
            if (ls != pcie_dev_list[j].link_status) {
                printf("LinkStatus: 0x%08X after %5d retries, "
                       "expected 0x%08x\n",
                       ls, i, pcie_dev_list[j].link_status);
                return (1);
            }
        }
        else if (pcie_dev_list[j].link_op == 1) {
            if ((ls & pcie_dev_list[j].link_status) == 0) {
                printf("LinkStatus: 0x%08X after %5d retries, "
                       "expected 0x%08x\n",
                       ls, i, pcie_dev_list[j].link_status);
                return (1);
            }
        }
    }
    printf("OK\n");
    return (0);
}

static int 
stress_pcie_down (int controller, int retries)
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
            return (1);
        }
    }
    printf("OK\n");
    return (0);
}

static void 
do_pcie_stress (uint32_t lanemap)
{   
    int err     = 0;
    int retries = 5000000;

    err += stress_pcie_up(1, retries, lanemap);
    if (err)
        printf("FAILED - PCIE stresstest with %d PCI errors\n", err);
    else
        printf("PASSED - PCIE stresstest\n");
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
    int i;
    uint32_t lanemap;

    if (argc <= 1) {   
        do_pcie_status();
        return (0);
    }
    
    if (argv[1][0] == '1')
        ;
    else if (argv[1][0] == 's') { 
        if (argc == 3) {
            lanemap = simple_strtoul(argv[2], NULL, 16);
            do_pcie_stress(lanemap);
        }
        else
            do_pcie_stress(0xFFFFFFFF);
        return (0);
    }
    else {   
        do_pcie_status();
        return (0);
    }
       
    controller = simple_strtoul(argv[1], NULL, 10);
    if (controller != 1)
        printf("PCIE controller %d out of range 1-1.\n",controller);

    pcie_hose_read_config_byte(&pcie_hose[controller-1], 0, 0x404, &link);
    if (argc == 3) {
        if (strcmp(argv[2], "-v") == 0) {
            pcieinfo(controller, 0,0); /* 0 - bus i - verbose list */
            if (link != 0x16)
                return (0);

            for (i = 1; i < 15; i++)
                pcieinfo(controller, i, 0); /* 1 - bus i - verbose list */
        }
        else if (strcmp(argv[2], "-d") == 0) {
            pciedump(controller, 0);  /* 0 - bus0 */
            if (link != 0x16)
                return (0);
            for (i = 1; i < 15; i++)
                pciedump(controller, i);  /* 1 - bus i */
        }
    }
    else {
        pcieinfo(controller, 0, 1); /* 0 - bus0 1 - short list */
        if (link != 0x16)
            return (0);
        for (i = 1; i < 15; i++)
            pcieinfo(controller, i, 1); /* 1 - bus1 i - short list */
    }
    
    return (0);
 usage:
    printf("Usage:\n%s\n", cmd->usage);
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
    return (PCIE_BDF(bdfs[0], bdfs[1], bdfs[2]));
}

static int 
pcie_cfg_display (int controller, pcie_dev_t bdf, ulong addr, ulong size, 
                  ulong length)
{
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
        for (i = 0; i < linebytes; i += size) {
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

    if (argc < 2)
            goto usage;
    
    if (argc == 2) {
        if (!strcmp(argv[1], "init"))
            pcie_init();
        return (1);
    }

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
            if ((bdf >= 1) && (link != 0x16))
                return (0);
            pcieinfo(controller, bdf, value);
            return (0);
    }

    if ((bdf >= 1) && (link != 0x16))
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


/* isp1568 commands
 *
 */
int 
do_isp (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{

    char cmd1, cmd2;
    int busdevfunc;
    uint32_t addr, reg, value;
    static uint32_t ohci_base_addr = 0, ehci_base_addr = 0;
    
    if (argc <= 2)
        goto usage;
    
    cmd1 = argv[1][0];
    cmd2 = argv[2][0];
    
    switch (cmd2) {
        case 'a':       /* address */
        case 'A':
            if ((cmd1 == 'o') || (cmd1 == 'O')) {
                busdevfunc = pci_find_device(USB_OHCI_VEND_ID,
                                             USB_OHCI_DEV_ID, 0);
                if (busdevfunc == -1) {
                    printf("Error USB OHCI (%04X,%04X) not found\n",
                           USB_OHCI_VEND_ID, USB_OHCI_DEV_ID);
                    return (-1);
                }

                pci_read_config_dword(busdevfunc, PCI_BASE_ADDRESS_0,
                                      &ohci_base_addr);
                ohci_base_addr &= 0xFFFFF000;
                printf("OHCI Base Address = 0x%lx\n", ohci_base_addr);
            }
            else if ((cmd1 == 'e') || (cmd1 == 'E')) {
                busdevfunc = pci_find_device(USB_OHCI_VEND_ID,
                                             USB_EHCI_DEV_ID, 0);
                if (busdevfunc == -1) {
                    printf("Error USB EHCI (%04X,%04X) not found\n", 
                           USB_OHCI_VEND_ID,USB_EHCI_DEV_ID);
                    return (-1);
                }

                pci_read_config_dword(busdevfunc, PCI_BASE_ADDRESS_0,
                                      &ehci_base_addr);
                ehci_base_addr &= 0xFFFFFF00;
                printf("EHCI Base Address = 0x%lx\n", ehci_base_addr);
            }
            else
                goto usage;
            break;
                        
        case 'd':       /* dump */
        case 'D':
            if ((cmd1 == 'o') || (cmd1 == 'O')) {
                if (ohci_base_addr == 0) {
                    printf("OHCI Base Address = 0x%lx\n",ohci_base_addr);
                    return (-1);
                }

                for (addr = 0; addr < 0x5c; addr += 4) {
                    value = readl(ohci_base_addr | addr);
                    printf("OHCI reg 0x%02x = 0x%lx\n", addr, value);
                }
            }
            else if ((cmd1 == 'e') || (cmd1 == 'E')) {
                if (ehci_base_addr == 0) {
                    printf("EHCI Base Address = 0x%lx\n",ehci_base_addr);
                    return (-1);
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
            if ((cmd1 == 'o') || (cmd1 == 'O')) {
                if (ohci_base_addr == 0) {
                    printf("OHCI Base Address = 0x%lx\n", ohci_base_addr);
                    return (-1);
                }
                value = readl(ohci_base_addr | reg);
                    printf("OHCI reg 0x%x (0x%lx) = 0x%lx\n",
                           reg , ohci_base_addr|reg, value);
            }
            else if ((cmd1 == 'e') || (cmd1 == 'E')) {
                if (ehci_base_addr == 0) {
                    printf("EHCI Base Address = 0x%lx\n", ehci_base_addr);
                    return (-1);
                }
                value = readl(ehci_base_addr | reg);
                printf("EHCI reg 0x%x (0x%lx) = 0x%lx\n",
                       reg , ehci_base_addr|reg, value);
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
            if ((cmd1 == 'o') || (cmd1 == 'O')) {
                if (ohci_base_addr == 0) {
                    printf("OHCI Base Address = 0x%lx\n", ohci_base_addr);
                    return (-1);
                }
                writel(value, ohci_base_addr | reg);
            }
            else if ((cmd1 == 'e') || (cmd1 == 'E')) {
                if (ehci_base_addr == 0) {
                    printf("EHCI Base Address = 0x%lx\n", ehci_base_addr);
                    return (-1);
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

    return (1);
 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return (1);
}

extern struct tsec_private *privlist1[];

static char mac_name [4][6] = 
 { "TSEC0",
    "TSEC1",
    "TSEC2",
    "TSEC3"
};

typedef struct _tsec_mii_t {
    uint8_t reg;
    uint8_t pmap;
    char    *name;
} tsec_mii_t;

tsec_mii_t tsec_mii_1112_table[] = {
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

tsec_mii_t tsec_mii_1145_table[] = {
    { 0x00, 0x03, "control" },
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
    { 0x0f, 0x3f, "ext status" },
    { 0x10, 0x3f, "phy spec control" },
    { 0x11, 0x03, "phy spec status" },
    { 0x12, 0x03, "int enable" },
    { 0x13, 0x03, "int status" },
    { 0x14, 0x3f, "ext phy control" },
    { 0x15, 0x3f, "error" },
    { 0x16, 0x3f, "page" },
    { 0x17, 0x3f, "global status" },
    { 0x18, 0x3f, "led control" },
    { 0x19, 0x3f, "led overwrite" },
    { 0x1a, 0x3f, "ext spec control 2" },
    { 0x1b, 0x3f, "ext spec status" },
    { 0x1c, 0x3f, "VCT skew/swap" },
    { 0x1e, 0x3f, "ext address" }
};

typedef struct _reg_desc_t {
    ushort  regno;
    char    *name;
} reg_desc_t;


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

void 
tsec_phy_dump (char *dev)
{
    int i, j;
    uint16_t temp, temp1;
    struct eth_device *eth_dev;
    struct tsec_private *priv1145 = NULL;
    struct tsec_private *priv1112 = NULL;
    struct tsec_private *priv0;
    struct tsec_private *priv1;

    eth_dev = eth_get_dev_by_name(dev);
    priv0 = (struct tsec_private *)eth_dev->priv;
    priv1 = privlist1[priv0->index];
    if (priv0->index <= 2)
        priv1145 = priv0;
    else if (priv0->index == 3) {
        priv1145 = priv1;
        priv1112 = priv0;
    }

    /* PHY1112 for mgmt port only */
    if ((!strcmp(dev, "TSEC3")) && (priv1112 && priv1112->phyinfo)) {
        mdio_mode_set(2, 1);
        printf("%s phy 1112 dump:\n",dev);
        printf("page %30s-0--  -1--  -2--  -3--  -4--  -5--  -6--\n", " ");
        for (i = 0; 
             i < sizeof(tsec_mii_1112_table)/sizeof(tsec_mii_1112_table[0]);
             i++) {
            printf("Register %2d  %-20s", 
                   tsec_mii_1112_table[i].reg, tsec_mii_1112_table[i].name);
            for (j = 0; j < 7; j++) {
                if (tsec_mii_1112_table[i].pmap & (1<<j)) {
                    temp = (uint16_t)read_phy_reg(priv1112, 0x16);
                    temp = ((temp & 0xff00) | j);
                    write_phy_reg(priv1112, 0x16, temp);
                    udelay(10);
                    temp1 = (uint16_t)read_phy_reg(priv1112, 
                                                   tsec_mii_1112_table[i].reg);
                    printf("  %04x", temp1);
                }
                else {
                    printf("      ");
                }
            }
            printf("\n");
        }
        temp = (uint16_t)read_phy_reg(priv1112, 0x16);
        temp = (temp & 0xff00);
        write_phy_reg(priv1112, 0x16, temp);
        printf("\n");
    }
    
    if (priv1145 && priv1145->phyinfo){
        mdio_mode_set(0, 1);
        printf("%s phy 1145 dump:\n",dev);
        printf("page %30s-0--  -1--  -2--  -3--  -4--  -5--\n", " ");
        for (i = 0; 
             i < sizeof(tsec_mii_1145_table)/sizeof(tsec_mii_1145_table[0]);
             i++) {
            printf("Register %2d  %-20s", 
                tsec_mii_1145_table[i].reg, tsec_mii_1145_table[i].name);
            for (j = 0; j < 6; j++) {
                if (tsec_mii_1145_table[i].pmap & (1<<j)) {
                    temp = (uint16_t)read_phy_reg(priv1145, 0x16);
                    temp = ((temp & 0xff00) | j);
                    write_phy_reg(priv1145, 0x16, temp);
                    udelay(10);
                    temp1 = (uint16_t)read_phy_reg(priv1145, 
                                                   tsec_mii_1145_table[i].reg);
                    printf("  %04x", temp1);
                }
                else {
                    printf("      ");
                }
            }
            printf("\n");
        }
        temp = (uint16_t)read_phy_reg(priv1145, 0x16);
        temp = (temp & 0xff00);
        write_phy_reg(priv1145, 0x16, temp);
    }
    mdio_mode_set(0, 0);
}

void 
tsec_reg_dump (char *dev)
{
   int i;
   struct eth_device *eth_dev = eth_get_dev_by_name(dev);
   struct tsec_private *priv = (struct tsec_private *)eth_dev->priv;
   volatile uint32_t *regs;
   
   regs = (volatile uint32_t *)(TSEC_BASE_ADDR + (priv->index) * TSEC_SIZE);
   if (regs != NULL) {
       printf("%s general control/status register dump:\n",dev);
       for (i = 0; i < sizeof(tsec_desc_tbl) / sizeof(tsec_desc_tbl[0]); i++)
           printf("0x%02x %-10s %08x\n", 
                  tsec_desc_tbl[i].regno, tsec_desc_tbl[i].name,
                  regs[tsec_desc_tbl[i].regno]);
   }
}

void 
mac_reg_dump (char *dev)
{
   int i;
   struct eth_device *eth_dev = eth_get_dev_by_name(dev);
   struct tsec_private *priv = (struct tsec_private *)eth_dev->priv;
   volatile uint32_t *regs;
   
   regs = (volatile uint32_t *)(TSEC_BASE_ADDR + 
                                (priv->index) * TSEC_SIZE + 0x500);
   if (regs != NULL) {
       printf("%s MAC register dump:\n",dev);
       for (i = 0; i < sizeof(mac_desc_tbl) / sizeof(mac_desc_tbl[0]); i++)
           printf("0x%02x %-10s %08x\n", 
                  mac_desc_tbl[i].regno, mac_desc_tbl[i].name,
                  regs[mac_desc_tbl[i].regno]);
   }
}

static void 
ether_tsec_dump (char *dev)
{
   struct eth_device *eth_dev = eth_get_dev_by_name(dev);

   if (eth_dev) {
       printf("%s :\n", eth_dev->name);
       printf("Ethernet: %02x:%02x:%02x:%02x:%02x:%02x\n", 
              eth_dev->enetaddr[0],
              eth_dev->enetaddr[1],
              eth_dev->enetaddr[2],
              eth_dev->enetaddr[3],
              eth_dev->enetaddr[4],
              eth_dev->enetaddr[5]
              );
       printf("IO base : %08x\n", eth_dev->iobase);
   }
}

/* tsec commands
 *
 */
int 
do_tsec (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int i = 0;
    int loopback, debug_flag, count, speed, mac;
    char cmd1, cmd2, cmd3;
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
                    for (i = 0; 
                         i < sizeof(tsec_desc_tbl) / sizeof(tsec_desc_tbl[0]);
                         i++) {
                        if (regno == tsec_desc_tbl[i].regno)
                            regindex = i;
                    }

                    if (regindex < 0) {
                        printf("register number out of range %d - %d\n", 
                               tsec_desc_tbl[0].regno, tsec_desc_tbl[i].regno);
                        return (1);
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
                    for (i = 0;
                         i < sizeof(tsec_desc_tbl) /sizeof(tsec_desc_tbl[0]);
                         i++) {
                        if (regno == tsec_desc_tbl[i].regno)
                            regindex = i;
                    }

                    if (regindex < 0) {
                        printf("register number out of range %d - %d\n", 
                               tsec_desc_tbl[0].regno, tsec_desc_tbl[i].regno);
                        return (1);
                    }
                    regs[regno] = temp32;
                    break;
            }
            break;

        case 'm':
        case 'M':
            if (argc <= 2)
                goto usage;
            
            if ((argv[1][1] == 'o') || (argv[1][1] == 'O')) { /* mode */
                if ((cmd2 == 's') || (cmd2 == 'S')) { /* set */
                    count = simple_strtoul(argv[3], NULL, 10);
                    mdio_mode_set(count, 0);
                    return (1);
                }
                else { /* get */
                    printf("MDIO mode %d, in use = %d\n", 
                           mdio_mode_get(), mdio_get_inuse());
                    return (1);
                }
            }
            
            if (dev[5] == '2')
                regs = (volatile uint32_t *)(TSEC_BASE_ADDR + 
                                             2 * TSEC_SIZE + 0x500);
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
                    for (i = 0; 
                         i < sizeof(mac_desc_tbl) / sizeof(mac_desc_tbl[0]);
                         i++) {
                        if (regno == mac_desc_tbl[i].regno)
                            regindex = i;
                    }

                    if (regindex < 0) {
                        printf("register number out of range %d - %d\n", 
                               mac_desc_tbl[0].regno, mac_desc_tbl[i].regno);
                        return (1);
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
                    for (i = 0; 
                         i < sizeof(mac_desc_tbl) / sizeof(mac_desc_tbl[0]);
                         i++) {
                        if (regno == mac_desc_tbl[i].regno)
                            regindex = i;
                    }

                    if (regindex < 0) {
                        printf("register number out of range %d - %d\n", 
                               mac_desc_tbl[0].regno, mac_desc_tbl[i].regno);
                        return (1);
                    }
                    regs[regno] =  temp32;
                    break;
            }
            break;

        case 'i':
        case 'I':    /* re-init */
            if ((argv[1][2] == 'i') || (argv[1][2] == 'i')) { /* init */
                eth_dev = eth_get_dev_by_name(dev);
                ether_tsec_reinit(eth_dev);
            }
            else {
                mac = simple_strtoul(argv[2], NULL, 10);
                ether_tsec_dump(mac_name[mac]);
            }
            break;
            
        case 'l':
        case 'L':   /* loopback */
            if (argc < 4)
                goto usage;
            
            cmd3 = argv[3][0];
            debug_flag = 0;
            mac = simple_strtoul(argv[2], NULL, 10);
            if ((mac <0) || (mac > 3))
                goto usage;
            
            if ((cmd3 == 'm') || (cmd3 == 'M')) { /* MAC */
                loopback = 0;
                count = simple_strtoul(argv[4], NULL, 10);
                if ((argc == 6) && ((argv[5][0]=='d') || (argv[5][0]=='D')))
                    debug_flag = 1;
            } else if ((cmd3 == 'p') || (cmd3 == 'P')) { /* PHY */
                if (!strcmp(argv[4], "1112"))
                    loopback = 2;
                else
                    loopback = 1;
                count = simple_strtoul(argv[5], NULL, 10);
                if ((argc == 7) && ((argv[6][0]=='d') || (argv[6][0]=='D')))
                    debug_flag = 1;
            } else if ((cmd3 == 'e') || (cmd3 == 'E')) { /* EXT */
                mac = 3;
                speed = simple_strtoul(argv[4], NULL, 10);
                if (speed == 10)
                    loopback = 3;
                else if (speed == 100)
                    loopback = 4;
                else if (speed == 1000)
                    loopback = 5;
                else
                    goto usage;
                count = simple_strtoul(argv[5], NULL, 10);
                if ((argc == 7) && ((argv[6][0]=='d') || (argv[6][0]=='D')))
                    debug_flag = 1;
            }
            else
                goto usage;
                
            for (i = 0; ((i < count) || (count == 0)); i++) {
                
                if (ctrlc()) {
                    putc ('\n');
                    return (0);
                }

                if ((loopback == 1) || (loopback == 2))
                    printf("TSEC%d PHY %s loopback: %d   ", mac, argv[3], i+1);
                else if ((loopback == 3) || (loopback == 4) || (loopback == 5))
                    printf("TSEC%d Ext %s loopback: %d   ", 
                           mac, argv[3], i+1);
                else
                    printf("TSEC%d MAC loopback: %d   ", mac, i+1);

                eth_dev = eth_get_dev_by_name(mac_name[mac]);
                if (ether_loopback(eth_dev, loopback, 1518, debug_flag, 1) 
                                   == 0) {
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

    return (1);
 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return (1);
}
#endif /* (CONFIG_COMMANDS & CFG_CMD_BSP) */


/* simple flash 16 bits write, do not accross sector */
int 
flash_write_direct (ulong addr, char *src, ulong cnt)
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
        printf("flash address 0x%x out of range 0xFF800000 - 0xFFFFFFFF\n", 
               addr);
        return (1);
    }
    faddrs = (uint16_t *)saddr;
    faddr1 = (uint16_t *)((saddr & 0xFFFFF000) | 0xAAA);
    faddr2 = (uint16_t *)((saddr & 0xFFFFF000) | 0x554);

    /* un-protect sector */
    *faddrs = FLASH_CMD_CLEAR_STATUS;
    *faddrs = FLASH_CMD_PROTECT;
    *faddrs = FLASH_CMD_PROTECT_CLEAR;
    start = get_timer (0);
    while ((*faddrs & AMD_STATUS_TOGGLE) != 
        (*faddrs & AMD_STATUS_TOGGLE)) {
        if (get_timer (start) > 0x2000) {
            *faddrs = AMD_CMD_RESET;
            return (1);
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
    while ((*faddrs & AMD_STATUS_TOGGLE) != 
        (*faddrs & AMD_STATUS_TOGGLE)) {
        if (get_timer (start) > 0x2000) {
            *faddrs = AMD_CMD_RESET;
            return (1);
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
        while ((*faddrs & AMD_STATUS_TOGGLE) != 
            (*faddrs & AMD_STATUS_TOGGLE)) {
            if (get_timer (start) > 0x100) {
                *faddrs = AMD_CMD_RESET;
                return (1);
            }
            udelay (1);
        }
    }

    /* protect sector */
    *faddrs = FLASH_CMD_CLEAR_STATUS;
    *faddrs = FLASH_CMD_PROTECT;
    *faddrs = FLASH_CMD_PROTECT_SET;
    start = get_timer (0);
    while ((*faddrs & AMD_STATUS_TOGGLE) != 
        (*faddrs & AMD_STATUS_TOGGLE)) {
        if (get_timer (start) > 0x2000) {
            *faddrs = AMD_CMD_RESET;
            return (1);
        }
        udelay (1);
    }

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

#define EX45XX_DIAG_NUM 8
static char *ex45xx_diag[EX45XX_DIAG_NUM]=
    {"cpu", "ram", "i2c", "pci", "usb", "ethernet", "uart", "watchdog"};

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
        if (!strcmp(argv[1], "run")) {
            for (i = 0; i < EX45XX_DIAG_NUM-2; i++) {
                if (post_run(ex45xx_diag[i], cmd_flag) != 0)
                    res = -1;
            }
            if (res)
                printf("\n- EX4500 test                           FAILED @\n");
            else
                printf("\n- EX4500 test                           PASSED\n");
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

extern flash_info_t flash_info[];       /* info for FLASH chips */
int resetenv_cmd(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    printf("Un-Protect ENV Sector\n");

    flash_protect(FLAG_PROTECT_CLEAR,
                  CFG_ENV_ADDR,CFG_ENV_ADDR + CFG_ENV_SECT_SIZE - 1,
                  &flash_info[0]);

	
    printf("Erase sector %d ... ",CFG_ENV_SECTOR);
    flash_erase(&flash_info[0], CFG_ENV_SECTOR, CFG_ENV_SECTOR);
    printf("done\nProtect ENV Sector\n");
	
    flash_protect(FLAG_PROTECT_SET,
                  CFG_ENV_ADDR,CFG_ENV_ADDR + CFG_ENV_SECT_SIZE - 1,
        &flash_info[0]);

    printf("\nWarning: Default Environment Variables will take effect Only "
           "after RESET \n\n");
    return 1;
}

static int
id_unknown (uint16_t id)
{
    switch (id) {
        case I2C_ID_JUNIPER_EX4500_40F:
        case I2C_ID_JUNIPER_EX4500_20F:
        case I2C_ID_TSUNAMI_UPLINK_SFPPLUS_4PORT_PIC:
        case I2C_ID_TSUNAMI_M2_LOOPBACK_PIC:
        case I2C_ID_EX4500_UPLINK_XFP_4PORT_PIC:
        case I2C_ID_EX4500_UPLINK_M2_OPTICAL_PIC:
        case I2C_ID_EX4500_UPLINK_M2_LEGACY_2PORT_PIC:
            return (0);
            break;
        default:
            break;
    }
    return (1);
}
#if defined(CONFIG_PRODUCTION)
char *
id_to_name (uint16_t id)
{
    switch (id) {
        case I2C_ID_JUNIPER_EX4500_40F:
            return ("T40");
            break;
        case I2C_ID_JUNIPER_EX4500_20F:
            return ("T20");
            break;
        default:
            break;
    }
    return ("Unknown");
}

char *
slot_to_name (uint16_t id, int slot)
{
    switch (slot) {
        case CFG_EX4500_T40_INDEX:
            if (id == I2C_ID_JUNIPER_EX4500_20F)
                return ("T20");
            else
                return ("T40");
            break;
        default:
            break;
    }
    return ("Unknown");
}
#else
char *
id_to_name (uint16_t id)
{
    switch (id) {
        case I2C_ID_JUNIPER_EX4500_40F:
            return ("T40");
            break;
        case I2C_ID_JUNIPER_EX4500_20F:
            return ("T20");
            break;
        case I2C_ID_TSUNAMI_UPLINK_SFPPLUS_4PORT_PIC:
            return ("M1-SFP+");
            break;
        case I2C_ID_TSUNAMI_M2_LOOPBACK_PIC:
            return ("M2-LBK");
            break;
        case I2C_ID_EX4500_UPLINK_XFP_4PORT_PIC:
            return ("M1-XFP");
            break;
        case I2C_ID_EX4500_UPLINK_M2_OPTICAL_PIC:
            return ("M2-OPTIC");
            break;
        case I2C_ID_EX4500_UPLINK_M2_LEGACY_2PORT_PIC:
            return ("M2-LEGACY");
            break;
        default:
            break;
    }
    return ("Unknown");
}

char *
slot_to_name (uint16_t id, int slot)
{
    switch (slot) {
        case CFG_EX4500_T40_INDEX:
            if (id == I2C_ID_JUNIPER_EX4500_20F)
                return ("T20");
            else
                return ("T40");
            break;
        case CFG_EX4500_M1_40_INDEX:
            return ("M1-40");
            break;
        case CFG_EX4500_M1_80_INDEX:
            return ("M1-80");
            break;
        case CFG_EX4500_M2_INDEX:
            return ("M2");
            break;
        default:
            break;
    }
    return ("Unknown");
}
#endif /* CONFIG_PRODUCTION */

int 
i2c_mux (int ctlr, uint8_t mux, uint8_t chan, int ena)
{
    uint8_t ch;
    int ret = 0;

    i2c_controller(ctlr);
    if (ena) {
        ch = 1 << chan;
        ret = i2c_write(mux, 0, 0, &ch, 1);
    }
    else {
        ch = 0;
        ret = i2c_write(mux, 0, 0, &ch, 1);
    }

    return ret;
}

void 
rtc_init (void)
{
    uint8_t temp[16];
    uint8_t i2cAddr = CFG_I2C_C1_9548SW1_P5_RTC;

    if (i2c_mux(0, CFG_I2C_C1_9548SW1, 5, 1)) {
        printf("i2c failed to enable mux 0x%x channel 5.\n", 
            CFG_I2C_C1_9548SW1);
    }
    else {
        if (i2c_read(i2cAddr, 0, 1, temp, 16)) {
            printf ("I2C read from device 0x%x failed.\n", i2cAddr);
            return;
        }

        if (temp[2] & 0x80) {
            temp[2] &= (~0x80);
                i2c_write(i2cAddr, 2, 1, &temp[2], 1);
        }
        temp[0] = 0x20;
        temp[1] = 0;
        temp[9] = temp[10] = temp[11] = temp[12] = 0x80;
        temp[13] = 0;
        temp[14] = 0;
        i2c_write(i2cAddr, 0, 1, temp, 16);
        temp[0] = 0;
        i2c_write(i2cAddr, 0, 1, temp, 1);
    }
    i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 0);  /* disable mux */
}

void 
rtc_start (void)
{
    uint8_t temp[2];
    uint8_t i2cAddr = CFG_I2C_C1_9548SW1_P5_RTC;

    if (i2c_mux(0, CFG_I2C_C1_9548SW1, 5, 1)) {
        printf("i2c failed to enable mux 0x%x channel 5.\n", 
               CFG_I2C_C1_9548SW1);
    }
    else {
        temp[0] = 0;
        i2c_write(i2cAddr, 0, 1, temp, 1);
    }
    i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 0);  /* disable mux */
}

void 
rtc_stop (void)
{
    uint8_t temp[2];
    uint8_t i2cAddr = CFG_I2C_C1_9548SW1_P5_RTC;

    if (i2c_mux(0, CFG_I2C_C1_9548SW1, 5, 1)) {
        printf("i2c failed to enable mux 0x%x channel 5.\n", 
               CFG_I2C_C1_9548SW1);
    }
    else {
        temp[0] = 0x20;
        i2c_write(i2cAddr, 0, 1, temp, 1);
    }
    i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 0);  /* disable mux */
}

void 
rtc_set_time (int yy, int mm, int dd, int hh, int mi, int ss)
{
    uint8_t temp[9];
    uint8_t i2cAddr = CFG_I2C_C1_9548SW1_P5_RTC;

    if (i2c_mux(0, CFG_I2C_C1_9548SW1, 5, 1)) {
        printf("i2c failed to enable mux 0x%x channel 5.\n", 
               CFG_I2C_C1_9548SW1);
    }
    else {
        i2c_read(i2cAddr, 0, 1, temp, 9);
        temp[8] = ((yy/10) << 4) + (yy%10);
        temp[7] &= 0x80;
        temp[7] = ((mm/10) << 4) + (mm%10);
        temp[5] = ((dd/10) << 4) + (dd%10);
        temp[4] = ((hh/10) << 4) + (hh%10);
        temp[3] = ((mi/10) << 4) + (mi%10);
        temp[2] = ((ss/10) << 4) + (ss%10);
        temp[6] = 0; /* weekday? */
        i2c_write(i2cAddr, 0, 1, temp, 9);
    }
    i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 0);  /* disable mux */
}

#if !defined(CONFIG_PRODUCTION)
static void 
rtc_reg_dump (void)
{
    uint8_t temp[20];
    uint8_t i2cAddr = CFG_I2C_C1_9548SW1_P5_RTC;
    int i;

    if (i2c_mux(0, CFG_I2C_C1_9548SW1, 5, 1)) {
        printf("i2c failed to enable mux 0x%x channel 5.\n", 
               CFG_I2C_C1_9548SW1);
    }
    else {
            if (i2c_read(i2cAddr, 0, 1, temp, 16)) {
                printf ("I2C read from device 0x%x failed.\n", i2cAddr);
            }
            else {
                printf("RTC register dump:\n");

                for (i = 0; i < 16; i++)
                    printf("%02x ", temp[i]);
                printf("\n");
        }
    }
    i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 0);  /* disable mux */
}

static void 
rtc_get_status (void)
{
    uint8_t temp[20];
    uint8_t i2cAddr = CFG_I2C_C1_9548SW1_P5_RTC;

    if (i2c_mux(0, CFG_I2C_C1_9548SW1, 5, 1)) {
        printf("i2c failed to enable mux 0x%x channel 5.\n", 
               CFG_I2C_C1_9548SW1);
    }
    else {
            if (i2c_read(i2cAddr, 0, 1, temp, 16)) {
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
    i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 0);  /* disable mux */
}

static void 
rtc_get_time (void)
{
    uint8_t temp[9];
    uint8_t i2cAddr = CFG_I2C_C1_9548SW1_P5_RTC;

    if (i2c_mux(0, CFG_I2C_C1_9548SW1, 5, 1)) {
        printf("i2c failed to enable mux 0x%x channel 5.\n", 
               CFG_I2C_C1_9548SW1);
    }
    else {
        if (i2c_read(i2cAddr, 0, 1, temp, 9)) {
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
    i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 0);  /* disable mux */
}


int 
do_rtc (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char *data;
    char tbyte[3];
    int i, len, temp, time1[3], time2[3];
    char *cmd = "status";  /* default command */

    if (argc == 1) {
        if (!strncmp(argv[0], "rtc", 3)) {
            rtc_get_time();
        }
        else
            goto usage;
    }
    else if (argc == 2) {
        cmd = argv[1];
        if (!strncmp(cmd, "reset", 3)) {
            rtc_set_time(0, 0, 0, 0, 0, 0);
        }
        else if (!strncmp(cmd, "init", 4)) {
            rtc_init();
        }
        else if (!strncmp(cmd, "start", 4)) {
            rtc_start();
        }
        else if (!strncmp(cmd, "stop", 3)) {
            rtc_stop();
        }
        else if (!strncmp(cmd, "status", 4)) {
            rtc_get_status();
        }
        else if (!strncmp(cmd, "dump", 4)) {
            rtc_reg_dump();
        }
        else
            goto usage;
    }
    else if (argc == 4) {
        cmd = argv[1];
        if (!strncmp(cmd, "set", 3)) {
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


int 
do_i2c_controller ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int controller = 1;  /* default I2C controller 1 */
       
    if (argc == 1) {
        printf("current i2c controller = %d\n", current_i2c_controller() + 1);
    }
    else if (argc == 2) {
        /* Set new base address.
        */
        controller = simple_strtoul(argv[1], NULL, 10);
        i2c_controller(controller-1);
    }
    else {
        printf ("Usage:\n%s\n", cmdtp->usage);
        return (1);
    }
       
    return (0);
}

int 
do_i2c_diag (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int controller, rw, len,i,temp; 
    unsigned int  slave_addr, reg, ret, noreg = 1;
    char *data;
    char tbyte[3];
    uint8_t tdata[128];
    
    /* check if all the arguments are passed */
    if (argc != 7) {
        printf ("Usage:\n%s\n", cmdtp->usage);
        return (1);
    }
    /* cmd arguments */
    rw = simple_strtoul(argv[1], NULL, 10);
    controller = simple_strtoul(argv[2], NULL, 10);
    slave_addr = simple_strtoul(argv[3], NULL, 16);
    reg = simple_strtoul(argv[4], NULL, 16);
    data = argv [5];
    len = simple_strtoul(argv[6], NULL, 10);
    if (reg == 0xFF) 
        noreg = 0;
    /* set the ctrl */
    i2c_controller(controller-1);

    if (rw == 1)
    {
        ret = i2c_read(slave_addr, reg, noreg, tdata, len);
        if (ret == 0)
            printf ("The data read - \n");
        for (i = 0; i < len; i++)
            printf("%02x ", tdata[i]);
        printf("\n");
    }
    else 
    {
        tbyte[2] = 0;
        for (i = 0; i < len; i++)
        {
            tbyte[0] = data[2*i];
            tbyte[1] = data[2*i+1];
            temp = atoh(tbyte);
            tdata[i] = temp;
        }
        ret = i2c_write(slave_addr, reg, noreg, tdata, len);
    }
    
    controller = 1; /* set back to default */
    i2c_controller(controller-1);

    if (ret != 0) 
    {
        printf ("i2c r/w failed\n");
        return (1);
    }
    return (0);
}

int 
do_i2c_diag_switch (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int controller, rw, len, temp, i; 
    unsigned int sw_addr, channel, slave_addr, reg, ret, noreg = 1;
       uint16_t cksum = 0;
    char ch;
    char *data;
    char tbyte[3];
    uint8_t tdata[128];

    /* check if all the arguments are passed */
    if (argc != 9) {
        printf ("Usage:\n%s\n", cmdtp->usage);
        return (1);
    }

    /* cmd arguments */
    rw = simple_strtoul(argv[1], NULL, 10);
    controller = simple_strtoul(argv[2], NULL, 10);
    sw_addr = simple_strtoul(argv[3], NULL, 16);
    channel = simple_strtoul(argv[4], NULL, 10);
    slave_addr = simple_strtoul(argv[5], NULL, 16);
    reg = simple_strtoul(argv[6], NULL, 16);
    data = argv [7];
    len = simple_strtoul(argv[8], NULL, 10);
    if (reg == 0xFF) 
        noreg = 0;

    i2c_controller(controller-1);

    /* enable the channel */
    ch = 1 << channel;
    i2c_write(sw_addr, 0, 0, &ch, 1); 

    if (rw == 1)
    {
        ret = i2c_read(slave_addr, reg, noreg, tdata, len);
        if (ret == 0)
            printf ("The data read - \n");
        for (i = 0; i < len; i++)
            printf("%02x ", tdata[i]);
        printf("\n");
    }
    else 
    {
        tbyte[2] = 0;
        for (i = 0; i < len; i++)
        {
            tbyte[0] = data[2*i];
            tbyte[1] = data[2*i+1];
            temp = atoh(tbyte);
            tdata[i] = temp;
        }
              if (i2c_cksum) 
              {
            for (i = 0; i < len; i++)
            {
                     cksum += tdata[i];
            }
                  tdata[len] = (cksum & 0xff00) >> 8;
                  tdata[len+1] = cksum & 0xff;
                  len = len + 2;
              }
        ret = i2c_write(slave_addr, reg, noreg, tdata, len);
    }

    /* disable the channel */
    ch = 0x0;
    i2c_write(sw_addr, 0, 0, &ch, 1);

    controller = 1; /* set back to default */
    i2c_controller(controller-1);
    if (ret != 0) 
    {
        printf ("i2c r/w failed\n");
        return (1);
    }
    return (0);
}

int 
do_i2c_cksum (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int cksum; 
    
    if (argc <= 1) {
        printf ("Usage:\n%s\n", cmdtp->usage);
        return (1);
    }
    
    cksum = simple_strtoul(argv[1], NULL, 10);
       if (cksum == 1)
           i2c_cksum = 1;
       else
           i2c_cksum = 0;
        
    return (0);
}

int
m1_40_present (void)
{
    uint8_t present;
    int controller = current_i2c_controller();
    uint8_t tdata;
    volatile immap_t *immap = (immap_t *) CFG_CCSRBAR;
    volatile ccsr_gur_t *gur = &immap->im_gur;
    
    i2c_controller(CFG_I2C_CTRL_2);
    i2c_read(CFG_I2C_C2_9506EXP1, 0x1, 1, &present, 1);
    i2c_read(CFG_I2C_C2_9506EXP1, 0xc, 1, &tdata, 1);
    if ((present & 0x4) == 0) {  /* M1-40 present */
        if ((tdata & 0x12) != 0x12) {
            tdata |= 0x12;  /* M1-40 power enable, i2c out of reset */
            i2c_write(CFG_I2C_C2_9506EXP1, 0xc, 1, &tdata, 1);
        }
        if ((gur->gpoutdr & 0x80) != 0x80) {
            gur->gpoutdr |= 0x80;  /* M1-40 out of reset */
            udelay(50000);
        }
        if (card_init[CFG_EX4500_M1_40_INDEX] == 0) {
            i2c_mux(1, CFG_I2C_C2_9546SW2, 3, 1);
            i2c_mux(1, CFG_I2C_M1_SFPP_9548SW1, 4, 1);
            /*  input 1, output 0 */ 
            i2c_reg_write(CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1, 0x18, 0x3F);
            i2c_reg_write(CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1, 0x19, 0x0);
            i2c_reg_write(CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1, 0x1A, 0xFF);
            i2c_reg_write(CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1, 0x1B, 0x0F);
            i2c_reg_write(CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1, 0x1C, 0x0);
            /*  Renesis out of reset - rev 2*/ 
            i2c_reg_write(CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1, 0x8, 0x40);
            udelay(50000);
            i2c_mux(1, CFG_I2C_M1_SFPP_9548SW1, 0, 0);
            i2c_mux(1, CFG_I2C_C2_9546SW2, 0, 0);
        }
        card_init[CFG_EX4500_M1_40_INDEX] = 1;
    }
    else {
        if (tdata & 0x12) {
            tdata &= ~0x12;  /* M1-40 power disable, i2c reset */
            i2c_write(CFG_I2C_C2_9506EXP1, 0xc, 1, &tdata, 1);
        }
        if (gur->gpoutdr & 0x80)
            gur->gpoutdr &= ~0x80;  /* M1-40 reset */
        card_init[CFG_EX4500_M1_40_INDEX] = 0;
    }
    i2c_controller(controller);

    return ((present & 0x4) == 0);
}

int
m1_80_present (void)
{
    uint8_t present;
    int controller = current_i2c_controller();
    uint8_t tdata;
    volatile immap_t *immap = (immap_t *) CFG_CCSRBAR;
    volatile ccsr_gur_t *gur = &immap->im_gur;
    
    i2c_controller(CFG_I2C_CTRL_2);
    i2c_read(CFG_I2C_C2_9506EXP1, 0x1, 1, &present, 1);
    i2c_read(CFG_I2C_C2_9506EXP1, 0xc, 1, &tdata, 1);
    if ((present & 0x8) == 0) {  /* M1-80 present */
        if ((tdata & 0x24) != 0x24) {
            tdata |= 0x24;  /* M1-80 power enable, i2c out of reset */
            i2c_write(CFG_I2C_C2_9506EXP1, 0xc, 1, &tdata, 1);
        }
        if ((gur->gpoutdr & 0x40) != 0x40) {
            gur->gpoutdr |= 0x40;  /* M1-80 out of reset */
            udelay(50000);
        }
        if (card_init[CFG_EX4500_M1_80_INDEX] == 0) {
            i2c_mux(1, CFG_I2C_C2_9546SW2, 2, 1);
            i2c_mux(1, CFG_I2C_M1_SFPP_9548SW1, 4, 1);
            /*  input 1, output 0 */ 
            i2c_reg_write(CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1, 0x18, 0x3F);
            i2c_reg_write(CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1, 0x19, 0x0);
            i2c_reg_write(CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1, 0x1A, 0xFF);
            i2c_reg_write(CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1, 0x1B, 0x0F);
            i2c_reg_write(CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1, 0x1C, 0x0);
            /*  Renesis out of reset - rev 2*/ 
            i2c_reg_write(CFG_I2C_M1_SFPP_9548SW1_P4_9506EXP1, 0x8, 0x40);
            udelay(50000);
            i2c_mux(1, CFG_I2C_M1_SFPP_9548SW1, 0, 0);
            i2c_mux(1, CFG_I2C_C2_9546SW2, 0, 0);
        }
        card_init[CFG_EX4500_M1_80_INDEX] = 1;
    }
    else {
        if (tdata & 0x24) {
            tdata &= ~0x24;  /* M1-80 power disable, i2c reset */
            i2c_write(CFG_I2C_C2_9506EXP1, 0xc, 1, &tdata, 1);
        }
        if (gur->gpoutdr & 0x40)
            gur->gpoutdr &= ~0x40;  /* M1-80 reset */
        card_init[CFG_EX4500_M1_80_INDEX] = 0;
    }
    i2c_controller(controller);

    return ((present & 0x8) == 0);
}

int
m2_present (void)
{
    uint8_t present;
    int controller = current_i2c_controller();
    uint8_t tdata;
    volatile immap_t *immap = (immap_t *) CFG_CCSRBAR;
    volatile ccsr_gur_t *gur = &immap->im_gur;
    
    i2c_controller(CFG_I2C_CTRL_2);
    i2c_read(CFG_I2C_C2_9506EXP1, 0x1, 1, &present, 1);
    i2c_read(CFG_I2C_C2_9506EXP1, 0xc, 1, &tdata, 1);
    if ((present & 0x10) == 0) {  /* M2 present */
        if ((tdata & 0x48) != 0x48) {
            tdata |= 0x48;  /* M2 power enable, i2c out of reset */
            i2c_write(CFG_I2C_C2_9506EXP1, 0xc, 1, &tdata, 1);
        }
        if ((gur->gpoutdr & 0x20) != 0x20) {
            gur->gpoutdr |= 0x20;  /* M2 out of reset */
            udelay(50000);
        }
        if (card_init[CFG_EX4500_M2_INDEX] == 0) {
            /* ? */
        }
        card_init[CFG_EX4500_M2_INDEX] = 1;
    }
    else {
        if (tdata & 0x48) {
            tdata &= ~0x48;  /* M2 power disable, i2c reset */
            i2c_write(CFG_I2C_C2_9506EXP1, 0xc, 1, &tdata, 1);
        }
        if (gur->gpoutdr & 0x20)
            gur->gpoutdr &= ~0x20;  /* M2 reset */
        card_init[CFG_EX4500_M2_INDEX] = 0;
    }
    i2c_controller(controller);

    return ((present & 0x10) == 0);
}

int
m1_40_id (int *rev, uint16_t *id, uint8_t *addr)
{
    uint16_t mid = 0;
    uint8_t ch = 1 << 6;
    int result = 0;
    int controller = current_i2c_controller();
    
    if (m1_40_present()) {
        if (i2c_mux(1, CFG_I2C_C2_9546SW2, 3, 1))
            return (-1);
        if (i2c_read_norsta(0x52, 4, 1, (uint8_t *)&mid, 2) == 0) {
            *rev = 1;
            *id = mid;
            *addr = 0x52;
        }
        else {
            i2c_write(0x70, 0, 0, &ch, 1);
            if (i2c_read_norsta(0x72, 4, 1, (uint8_t *)&mid, 2) == 0) {
                *rev = 2;
                *id = mid;
                *addr = 0x72;
            }
            else {
                *rev = 0;
                result = -1;
            }
            ch = 0;
            i2c_write(0x70, 0, 0, &ch, 1);
        }
        i2c_mux(1, CFG_I2C_C2_9546SW2, 0, 0);
        i2c_controller(controller);
    }
    else
        result = -1;
    return result;
}

int
m1_80_id (int *rev, uint16_t *id, uint8_t *addr)
{
    uint16_t mid = 0;
    uint8_t ch = 1 << 6;
    int result = 0;
    int controller = current_i2c_controller();
    
    if (m1_80_present()) {
        if (i2c_mux(1, CFG_I2C_C2_9546SW2, 2, 1))
            return (-1);
        if (i2c_read_norsta(0x52, 4, 1, (uint8_t *)&mid, 2) == 0) {
            *rev = 1;
            *id = mid;
            *addr = 0x52;
        }
        else {
            i2c_write(0x70, 0, 0, &ch, 1);
            if (i2c_read_norsta(0x72, 4, 1, (uint8_t *)&mid, 2) == 0) {
                *rev = 2;
                *id = mid;
                *addr = 0x72;
            }
            else {
                *rev = 0;
                result = -1;
            }
            ch = 0;
            i2c_write(0x70, 0, 0, &ch, 1);
        }
        i2c_mux(1, CFG_I2C_C2_9546SW2, 0, 0);
        i2c_controller(controller);
    }
    else
        result = -1;
    return result;
}

int
m2_id (int *rev, uint16_t *id, uint8_t *addr)
{
    uint16_t mid = 0;
    uint8_t ch = 0x22;  /* ch 1 + 5 */
    int result = 0;
    int controller = current_i2c_controller();
    
    if (m2_present()) {
        if (i2c_mux(1, CFG_I2C_C2_9546SW2, 1, 1))
            return (-1);
        if (i2c_read_norsta(0x52, 4, 1, (uint8_t *)&mid, 2) == 0) {
            *rev = 1;
            *id = mid;
            *addr = 0x52;
        }
        else if (i2c_read_norsta(0x62, 4, 1, (uint8_t *)&mid, 2) == 0) {
            *rev = 1;
            *id = mid;
            *addr = 0x62;
        }
        else {
            i2c_write(0x70, 0, 0, &ch, 1);
            if (i2c_read_norsta(0x72, 4, 1, (uint8_t *)&mid, 2) == 0) {
                *rev = 2;
                *id = mid;
                *addr = 0x72;
            }
            else {
                *rev = 0;
                *id = 0;
                result = -1;
            }
            ch = 0;
            i2c_write(0x70, 0, 0, &ch, 1);
        }
        i2c_mux(1, CFG_I2C_C2_9546SW2, 1, 0);
        i2c_controller(controller);
    }
    else
        result = -1;
    return result;
}

static void 
i2c_set_fan (int fan, int duty)
{
    uint8_t tdata[2];

    tdata[0] = duty * 0xFF / 100;
    i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 1);  /* enable mux chan 0 */

    switch (fan) {
        case 1:
            i2c_write(CFG_I2C_C1_9548SW1_ENV_0, 0x5b, 1, tdata, 1); 
            break;
                        
        case 2:
            i2c_write(CFG_I2C_C1_9548SW1_ENV_0, 0x5e, 1, tdata, 1);
            break;
                        
        case 3:
            i2c_write(CFG_I2C_C1_9548SW1_ENV_0, 0x5f, 1, tdata, 1);
            break;
                        
        case 4:
            i2c_write(CFG_I2C_C1_9548SW1_ENV_0, 0x5b, 1, tdata, 1);
            break;
                        
        case 5:
            i2c_write(CFG_I2C_C1_9548SW1_ENV_0, 0x5e, 1, tdata, 1);
            break;
    }
    i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 0);  /* disable mux */
}

int
i2c_read_temp_m1_40(uint16_t *id)
{
    int rev;
    uint16_t mid = 0;
    uint16_t data16;
    uint8_t data8[2], addr;
    int temp = 0;

    *id = 0;
    if (m1_40_present()) {
        if (m1_40_id(&rev, &mid, &addr) == 0) {
            if (rev == 1) {
                if (mid == I2C_ID_TSUNAMI_UPLINK_SFPPLUS_4PORT_PIC) {
                    /* ADT7410 */
                    i2c_mux(1, CFG_I2C_C2_9546SW2, 3, 1); 
                    i2c_mux(1, CFG_I2C_M1_SFPP_9548SW1, 5, 1); 
                    i2c_read(CFG_I2C_M1_SFPP_9548SW1_P5_ADT7410_REV1,
                             0x0, 1, (uint8_t *)&data16, 2);
                    i2c_mux(1, CFG_I2C_M1_SFPP_9548SW1, 5, 0); 
                    i2c_mux(1, CFG_I2C_C2_9546SW2, 3, 0); 

                    data16 &= 0xFFF8;
                    temp = twos_complement_16(data16) / 128;
                }
            }
            else {
                    /* LM75 */
                    i2c_mux(1, CFG_I2C_C2_9546SW2, 3, 1);
                    i2c_mux(1, CFG_I2C_M1_SFPP_9548SW1, 5, 1);
                    i2c_read(CFG_I2C_M1_SFPP_9548SW1_P5_LM75,
                             0x0, 1, data8, 2);
                    i2c_mux(1, CFG_I2C_M1_SFPP_9548SW1, 5, 0);
                    i2c_mux(1, CFG_I2C_C2_9546SW2, 3, 0);

                    temp = twos_complement(data8[0]);
            }
        }
        *id = mid;
    }
    
    return (temp);
}

int
i2c_read_temp_m1_80(uint16_t *id)
{
    int rev;
    uint16_t mid = 0;
    uint16_t data16;
    uint8_t data8[2], addr;
    int temp = 0;

    *id = 0;
    if (m1_80_present()) {
        if (m1_80_id(&rev, &mid, &addr) == 0) {
            if (rev == 1) {
                if (mid == I2C_ID_TSUNAMI_UPLINK_SFPPLUS_4PORT_PIC) {
                    /* ADT7410 */
                    i2c_mux(1, CFG_I2C_C2_9546SW2, 2, 1); 
                    i2c_mux(1, CFG_I2C_M1_SFPP_9548SW1, 5, 1);
                    i2c_read(CFG_I2C_M1_SFPP_9548SW1_P5_ADT7410_REV1,
                             0x0, 1, (uint8_t *)&data16, 2);
                    i2c_mux(1, CFG_I2C_M1_SFPP_9548SW1, 5, 0);
                    i2c_mux(1, CFG_I2C_C2_9546SW2, 2, 0);

                    data16 &= 0xFFF8;
                    temp = twos_complement_16(data16) / 128;
                }
            }
            else {
                    /* LM75 */
                    i2c_mux(1, CFG_I2C_C2_9546SW2, 2, 1); 
                    i2c_mux(1, CFG_I2C_M1_SFPP_9548SW1, 5, 1);
                    i2c_read(CFG_I2C_M1_SFPP_9548SW1_P5_LM75,
                             0x0, 1, data8, 2);
                    i2c_mux(1, CFG_I2C_M1_SFPP_9548SW1, 5, 0);
                    i2c_mux(1, CFG_I2C_C2_9546SW2, 2, 0);

                    temp = twos_complement(data8[0]);
            }
        }
        *id = mid;
    }
    
    return (temp);
}

int
i2c_read_temp_m2(uint16_t *id)
{
    int rev;
    uint16_t mid = 0;
    uint16_t data16;
    uint8_t data8[2], addr;
    int temp = 0;

    *id = 0;
    if (m2_present()) {
        if (m2_id(&rev, &mid, &addr) == 0) {
            if (rev == 1) {
                if (mid == I2C_ID_TSUNAMI_M2_LOOPBACK_PIC) {
                    /* ADT7410 */
                    i2c_mux(1, CFG_I2C_C2_9546SW2, 1, 1);
                    i2c_mux(1, CFG_I2C_M2_LBK_9543SW1_REV1, 0, 1); 
                    i2c_read(CFG_I2C_M2_LBK_9548SW1_P0_ADT7410_REV1,
                             0x0, 1, (uint8_t *)&data16, 2);
                    i2c_mux(1, CFG_I2C_M2_LBK_9543SW1_REV1, 0, 0);
                    i2c_mux(1, CFG_I2C_C2_9546SW2, 1, 0);

                    data16 &= 0xFFF8;
                    temp = twos_complement_16(data16) / 128;
                }
            }
            else {
                if (mid == I2C_ID_TSUNAMI_M2_LOOPBACK_PIC) {
                    /* LM75 */
                    i2c_mux(1, CFG_I2C_C2_9546SW2, 1, 1);
                    i2c_mux(1, CFG_I2C_M2_LBK_9548SW1, 2, 1);
                    i2c_read(CFG_I2C_M2_LBK_9548SW1_P2_LM75,
                             0x0, 1, data8, 2);
                    i2c_mux(1, CFG_I2C_M2_LBK_9548SW1, 2, 0);
                    i2c_mux(1, CFG_I2C_C2_9546SW2, 1, 0);

                    temp = twos_complement(data8[0]);
                }
            }
        }
        *id = mid;
    }
    
    return (temp);
}

int
i2c_read_temp_m2_array(uint16_t *id, int *data)
{
    int rev;
    uint16_t mid = 0;
    uint16_t data16;
    uint8_t data8[2], addr;
    int temp = 0;

    if (m2_present()) {
        if (m2_id(&rev, &mid, &addr) == 0) {
            if (rev == 1) {
                if (mid == I2C_ID_TSUNAMI_M2_LOOPBACK_PIC) {
                    /* ADT7410 */
                    i2c_mux(1, CFG_I2C_C2_9546SW2, 1, 1);
                    i2c_mux(1, CFG_I2C_M2_LBK_9543SW1_REV1, 0, 1); 
                    i2c_read(CFG_I2C_M2_LBK_9548SW1_P0_ADT7410_REV1,
                             0x0, 1, (uint8_t *)&data16, 2);
                    i2c_mux(1, CFG_I2C_M2_LBK_9543SW1_REV1, 0, 0);
                    i2c_mux(1, CFG_I2C_C2_9546SW2, 1, 0);

                    data16 &= 0xFFF8;
                    data[0] = twos_complement_16(data16) / 128;
                    temp = 1;
                }
            }
            else {
                if (mid == I2C_ID_TSUNAMI_M2_LOOPBACK_PIC) {
                    /* LM75 */
                    i2c_mux(1, CFG_I2C_C2_9546SW2, 1, 1);
                    i2c_mux(1, CFG_I2C_M2_LBK_9548SW1, 2, 1);
                    i2c_read(CFG_I2C_M2_LBK_9548SW1_P2_LM75,
                             0x0, 1, data8, 2);
                    i2c_mux(1, CFG_I2C_M2_LBK_9548SW1, 2, 0);
                    i2c_mux(1, CFG_I2C_C2_9546SW2, 1, 0);

                    data[0] = twos_complement(data8[0]);
                    temp = 1;
                }
                else if (mid == I2C_ID_EX4500_UPLINK_M2_LEGACY_2PORT_PIC) {
                    /* LM75 */
                    i2c_mux(1, CFG_I2C_C2_9546SW2, 1, 1);
                    i2c_mux(1, CFG_I2C_M2_LBK_9548SW1, 6, 1);
                    i2c_read(CFG_I2C_M2_LBK_9548SW1_P2_LM75,
                             0x0, 1, data8, 2);
                    i2c_mux(1, CFG_I2C_M2_LBK_9548SW1, 2, 0);
                    i2c_mux(1, CFG_I2C_C2_9546SW2, 1, 0);

                    data[0] = twos_complement(data8[0]);
                    temp = 1;
                }
                else if (mid == I2C_ID_EX4500_UPLINK_M2_OPTICAL_PIC) {
                    /* LM75 */
                    i2c_mux(1, CFG_I2C_C2_9546SW2, 1, 1);
                    i2c_mux(1, CFG_I2C_M2_LBK_9548SW1, 2, 1);
                    i2c_read(CFG_I2C_M2_LBK_9548SW1_P2_LM75,
                             0x0, 1, data8, 2);
                    data[0] = twos_complement(data8[0]);
                    i2c_read(CFG_I2C_M2_LBK_9548SW1_P2_LM75+1,
                             0x0, 1, data8, 2);
                    data[1] = twos_complement(data8[0]);
                    i2c_read(CFG_I2C_M2_LBK_9548SW1_P2_LM75+2,
                             0x0, 1, data8, 2);
                    data[2] = twos_complement(data8[0]);
                    i2c_mux(1, CFG_I2C_M2_LBK_9548SW1, 2, 0);
                    i2c_mux(1, CFG_I2C_C2_9546SW2, 1, 0);

                    temp = 3;
                }
                else {
                    /* LM75 */
                    i2c_mux(1, CFG_I2C_C2_9546SW2, 1, 1);
                    data8[0] = 0xff;
                    i2c_write(0x70, 0, 0, data8, 1);  /* enable all channels */
                    if (i2c_read(CFG_I2C_M2_LBK_9548SW1_P2_LM75,
                                 0x0, 1, data8, 2) == 0) {
                        data[temp] = twos_complement(data8[0]);
                        temp++;
                    }
                    if (i2c_read(CFG_I2C_M2_LBK_9548SW1_P2_LM75+1,
                                 0x0, 1, data8, 2) == 0) {
                        data[temp] = twos_complement(data8[0]);
                        temp++;
                    }
                    if (i2c_read(CFG_I2C_M2_LBK_9548SW1_P2_LM75+2,
                                 0x0, 1, data8, 2) == 0) {
                        data[temp] = twos_complement(data8[0]);
                        temp++;
                    }
                    i2c_mux(1, CFG_I2C_M2_LBK_9548SW1, 2, 0);
                    i2c_mux(1, CFG_I2C_C2_9546SW2, 1, 0);
                }
            }
        }
        *id = mid;
    }
    
    return (temp);
}
void 
i2c_read_temp_m1m2(void)
{
    uint16_t mid;
    int i, temp = 0;
    int data[3];

    if (m1_40_present()) {
        temp = i2c_read_temp_m1_40(&mid);
        if (!id_unknown(mid))
            printf(" TEMP: M1-40 slot (%s)\n", id_to_name(mid));
        else
            printf(" TEMP: M1-40 slot (%s - %04x)\n", id_to_name(mid), mid);
        printf("%4d\n\n", temp);
    }

    if (m1_80_present()) {
        temp = i2c_read_temp_m1_80(&mid);
        if (!id_unknown(mid))
            printf(" TEMP: M1-80 slot (%s)\n", id_to_name(mid));
        else
            printf(" TEMP: M1-80 slot (%s - %04x)\n", id_to_name(mid), mid);
        printf("%4d\n\n", temp);
    }

    if (m2_present()) {
        temp = i2c_read_temp_m2_array(&mid, data);
        if (!id_unknown(mid))
            printf(" TEMP: M2 slot (%s)\n", id_to_name(mid));
        else
            printf(" TEMP: M2 slot (%s - %04x)\n", id_to_name(mid), mid);
        for (i = 0; i < temp; i++) {
            printf("%4d  ", data[i]);
        }
        printf("\n\n");
    }

}

static uint8_t 
tempRead (int32_t sensor)
{
    uint8_t tdata[2] = {0, 0};

    if (sensor > 7)
        return (0);
    
    switch(sensor) {
        case 0:  /* CPU thermal pin, it does not work. */
            i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 1);  /* enable mux chan 0 */
            tdata[0] = 0x80;  /* bank 0 */
            i2c_write(CFG_I2C_C1_9548SW1_ENV_0, 0x4e, 1, tdata, 1);
            i2c_read(CFG_I2C_C1_9548SW1_ENV_0, 0x27, 1, tdata, 1);
            break;
            
        case 1:
            i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 1);  /* enable mux chan 0 */
            tdata[0] = 0x80;  /* bank 0 */
            i2c_write(CFG_I2C_C1_9548SW1_ENV_1, 0x4e, 1, tdata, 1);
            i2c_read(CFG_I2C_C1_9548SW1_ENV_1, 0x27, 1, tdata, 1);
            break;
            
        case 2:
            i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 1);  /* enable mux chan 0 */
            tdata[0] = 0x81;  /* bank 1 */
            i2c_write(CFG_I2C_C1_9548SW1_ENV_1, 0x4e, 1, tdata, 1);
            i2c_read(CFG_I2C_C1_9548SW1_ENV_1, 0x50, 1, tdata, 2);
            break;
            
        case 3:
            i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 1);  /* enable mux chan 0 */
            tdata[0] = 0x82;  /* bank 2 */
            i2c_write(CFG_I2C_C1_9548SW1_ENV_1, 0x4e, 1, tdata, 1);
            i2c_read(CFG_I2C_C1_9548SW1_ENV_1, 0x50, 1, tdata, 2);
            break;
            
        case 4:
            i2c_mux(0, CFG_I2C_C1_9548SW1, 6, 1);  /* enable mux chan 6 */
            i2c_read(CFG_I2C_C1_9548SW1_P6_TEMP_0, 0, 1, tdata, 1);
            break;
            
        case 5:
            i2c_mux(0, CFG_I2C_C1_9548SW1, 6, 1);  /* enable mux chan 6 */
            i2c_read(CFG_I2C_C1_9548SW1_P6_TEMP_0, 1, 1, tdata, 1);
            break;
            
        case 6:
            i2c_mux(0, CFG_I2C_C1_9548SW1, 6, 1);  /* enable mux chan 6 */
            i2c_read(CFG_I2C_C1_9548SW1_P6_TEMP_1, 0, 1, tdata, 1);
            break;
            
        case 7:
            i2c_mux(0, CFG_I2C_C1_9548SW1, 6, 1);  /* enable mux chan 6 */
            i2c_read(CFG_I2C_C1_9548SW1_P6_TEMP_1, 1, 1, tdata, 1);
            break;
            
        default:
            break;       
    }

    i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 0);  /* disable mux */

    if ((sensor <= 3) && (tdata[0] == 0xff))  /* read failure */
        tdata[0] = 0;
    
    return tdata[0];
}

static int 
i2c_read_temp (uint32_t threshold)
{
    uint8_t temp1[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    int16_t temp2[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    int i;
    int res = 0;

    /* skip 0, MPC8548 thermal pin alway read 0x7F */
    for (i = 1; i < 8; i++) {
        temp1[i] = tempRead(i);
        temp2[i] = twos_complement(temp1[i]);
    }

    /* adjustment */

    printf("Temperature (deg C): Winbond (TEMPn), NE1617A (Ln/Rn)\n");
    printf(" TEMP1    TEMP2    TEMP3    L1/R1   L2/R2\n");
    printf("%3d(%02x)  %3d(%02x)  %3d(%02x)  %2d/%2d   %2d/%2d\n",
           temp2[1], temp1[1], 
           temp2[2], temp1[2], 
           temp2[3], temp1[3], 
           temp2[4], temp2[5],
           temp2[6], temp2[7]);
    printf("\n");

    i2c_read_temp_m1m2();
    
    for (i = 1; i < 7; i++) {
        if (temp2[i] > threshold) {
            res = 1;
            break;
        }
    }
    
    return (res);
}

static uint16_t 
voltRead (uint8_t addr, int32_t sensor)
{
    uint8_t tdata;
    uint16_t temp = 0;

    if (sensor >= 5)
        return (0);
    
    i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 1);  /* enable mux chan 0 */
    switch(sensor) {
        case 0:
            i2c_read(addr, 0x20, 1, &tdata, 1);
            temp = tdata * 16;
            break;
            
        case 1:
            i2c_read(addr, 0x21, 1, &tdata, 1);
            temp = tdata * 16;
            break;
            
        case 2:
            i2c_read(addr, 0x22, 1, &tdata, 1);
            temp = tdata * 16;
            break;
            
        case 3:
            i2c_read(addr, 0x23, 1, &tdata, 1);
            temp = tdata * 2688 / 100;
            break;
            
        case 4:
            i2c_read(addr, 0x24, 1, &tdata, 1);
            temp = tdata * 608 / 40;
            break;
                       
        default:
            break;
            
    }
    i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 0);  /* disable mux */
    
    return (temp);
}

static void 
i2c_read_volt (void)
{
    int i;
    uint16_t volt0[5], volt1[5];
    
    for (i = 0; i < 5; i++) {
        volt0[i] = voltRead(CFG_I2C_C1_9548SW1_ENV_0, i);
        volt1[i] = voltRead(CFG_I2C_C1_9548SW1_ENV_1, i);
    }    
    printf("\n");
    printf("W83782G#0 voltage:\n");
    printf("%-10s = %2d.%03d\n", "1V L1", volt0[0]/1000, volt0[0]%1000);
    printf("%-10s = %2d.%03d\n", "1.2V PHY1", volt0[1]/1000, volt0[1]%1000);
    printf("%-10s = %2d.%03d\n", "1.1V", volt0[2]/1000, volt0[2]%1000);
    printf("%-10s = %2d.%03d\n", "5V", volt0[3]/1000, volt0[3]%1000);
    printf("%-10s = %2d.%03d\n", "2.5V", volt0[4]/1000, volt0[4]%1000);
    printf("W83782G#1 voltage:\n");
    printf("%-10s = %2d.%03d\n", "1V L2", volt1[0]/1000, volt1[0]%1000);
    printf("%-10s = %2d.%03d\n", "1.2V PHY2", volt1[1]/1000, volt1[1]%1000);
    printf("%-10s = %2d.%03d\n", "1.8V DDR", volt1[2]/1000, volt1[2]%1000);
    printf("%-10s = %2d.%03d\n", "5V", volt1[3]/1000, volt1[3]%1000);
    printf("%-10s = %2d.%03d\n", "3.3V", volt1[4]/1000, volt1[4]%1000);
}

static uint16_t 
voltPWR1220Read (uint8_t addr, int32_t sensor)
{
    uint8_t tdata, tdata1;
    uint16_t temp = 0;

    if (sensor >= 11)
        return (0);
    
    i2c_mux(0, CFG_I2C_C1_9548SW1, 6, 1);  /* enable mux chan 6 */

    tdata = 0x10 + sensor;
    i2c_write(addr, 0x9, 1, &tdata, 1);

    i2c_read(addr, 0x07, 1, &tdata, 1);
    while ((tdata & 0x1) == 0) {
        udelay(100);
        i2c_read(addr, 0x07, 1, &tdata, 1);
    }
    i2c_read(addr, 0x08, 1, &tdata1, 1);
    temp = ((tdata1 * 16) + (tdata  / 16)) * 2;

    i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 0);  /* disable mux */
    
    return (temp);
}

static void 
i2c_read_PWR1220_volt (void)
{
    int i;
    uint16_t volt[11];
    
    for (i = 0; i < 11; i++) {
        volt[i] = voltPWR1220Read(CFG_I2C_C1_9548SW1_P6_PWR1220, i);
    }    
    printf("\n");
    printf("PWR1220 voltage:\n");
    printf("%-10s = %2d.%03d\n", "3.3V", volt[0]/1000, volt[0]%1000);
    printf("%-10s = %2d.%03d\n", "3.3V SFP+", volt[1]/1000, volt[1]%1000);
    printf("%-10s = %2d.%03d\n", "1.1V", volt[2]/1000, volt[2]%1000);
    printf("%-10s = %2d.%03d\n", "2.5V", volt[3]/1000, volt[3]%1000);
    printf("%-10s = %2d.%03d\n", "1.8V L1", volt[4]/1000, volt[4]%1000);
    printf("%-10s = %2d.%03d\n", "1.8V L2", volt[5]/1000, volt[5]%1000);
    printf("%-10s = %2d.%03d\n", "1.0V", volt[6]/1000, volt[6]%1000);
    printf("%-10s = %2d.%03d\n", "1.2V PHY1", volt[7]/1000, volt[7]%1000);
    printf("%-10s = %2d.%03d\n", "1.2V PHY2", volt[8]/1000, volt[8]%1000);
    printf("%-10s = %2d.%03d\n", "1.0V L1", volt[9]/1000, volt[9]%1000);
    printf("%-10s = %2d.%03d\n", "1.0V L2", volt[10]/1000, volt[10]%1000);
}

/* I2C commands
 *
 * Syntax:
 */
int 
do_i2c (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char cmd1, cmd2, cmd3, cmd4_0, cmd4_1, cmd4_2, cmd5, cmd6_0, cmd6_1;
    int led, fan = 0;
    LedColor color;
    LedState state;
    ulong mux = 0, device = 0, channel = 0;
    unsigned char ch, bvalue;
    uint regnum = 0;
    int i = 0, ret, reg = 0, temp, len = 0, offset = 0, secure = 0, nbytes;
    char tbyte[3];
    uint8_t tdata[2048];
    char *data;
    uint16_t cksum = 0;
    int unit, rsta = 0;
    uint8_t fdr = 0x2F, addr8 = 0, reg8 = 0;
    ulong value, mloop = 1, threshold = 255;
    uint32_t duty = 0;
    uint8_t count1, count2, count3, count4, count5;
    int32_t rpm1, rpm2, rpm3, rpm4, rpm5;

    if (argc < 2)
        goto usage;
    
    cmd1 = argv[1][0];
    cmd2 = argv[2][0];
    
    switch (cmd1) {
        case 'l':       /* LED */
        case 'L':
            unit = simple_strtoul(argv[2], NULL, 10);
            cmd3 = argv[3][0];
            switch (cmd3) {
                case 'u':    /* LED update */
                case 'U':
                    if (unit)
                        update_led_back();
                    else
                        update_led();
                    break;
                    
                case 's':
                case 'S':    /* LED set */
                    if (argc <= 6)
                        goto usage;

                    cmd4_0 = argv[4][0];
                    cmd4_1 = argv[4][1];
                    cmd4_2 = argv[4][2];
                    cmd5 = argv[5][0];
                    cmd6_0 = argv[6][0];
                    cmd6_1 = argv[6][1];
                    switch (cmd4_0) {
                        case 't':
                        case 'T':
                            if (unit)
                                led = 2;
                            else
                                led = 4;
                            break;
                    
                        case 'm':
                        case 'M':
                            if (unit)
                                led = 1;
                            else
                                led = 3;
                            break;
                    
                        case 'b':
                        case 'B':
                            if (unit)
                                led = 0;
                            else
                                led = 1;
                            break;
                                        
                        default:
                            goto usage;
                            break;
                    }

                    if ((cmd5 == 'a') || (cmd5 == 'A')) 
                        color = LED_AMBER;
                    else if ((cmd5 == 'g') || (cmd5 == 'G'))
                        color = LED_GREEN;
                    else if ((cmd5 == 'r') || (cmd5 == 'R'))
                        color = LED_RED;
                    else if ((cmd5 == 'o') || (cmd5 == 'O'))
                        color = LED_OFF;
                    else
                        goto usage;

                    if ((cmd6_0 == 's') || (cmd6_0 == 'S')) {
                        if ((cmd6_1 == 't') || (cmd6_1 == 'T'))
                            state = LED_STEADY;
                        else if ((cmd6_1 == 'l') || (cmd6_1 == 'L'))
                            state = LED_SLOW_BLINKING;
                        else
                            goto usage;
                    }
                    else if ((cmd6_0 == 'f') || (cmd6_0 == 'F'))
                        state = LED_FAST_BLINKING;
                    else
                        goto usage;

                    if (unit)
                        set_led_back(led, color, state);
                    else
                        set_led(led, color, state);
                    break;
                    
                default:
                    printf("???");
                    break;
            }
            
            break;
            
        case 'b':   /* byte read/write */
        case 'B':
            device = simple_strtoul(argv[2], NULL, 16);
            if ((argv[1][1] == 'r') ||(argv[1][1] == 'R')) {
                 bvalue = 0xff;
             if ((ret = i2c_read_noaddr(device, &bvalue, 1)) != 0) {
                    printf("i2c failed to read from device 0x%x.\n", device);
                    return (0);
                }
                printf("i2c read addr=0x%x value=0x%x\n", device, bvalue);
            }
            else {
                if (argc <= 3)
                    goto usage;
                
                bvalue = simple_strtoul(argv[3], NULL, 16);
            if ((ret = i2c_write_noaddr(device, &bvalue, 1)) != 0) {
                    printf("i2c failed to write to address 0x%x value 0x%x.\n",
                           device, bvalue);
                    return (0);
                }
            }
            
            break;
            
        case 'm':   
        case 'M':
            if ((argv[1][1] == 'a') ||(argv[1][1] == 'A')) {  /* manual */
                if (argc < 3)
                    goto usage;
                if (!strncmp(argv[2], "on", 2))
                    i2c_manual = 1;
                else
                    i2c_manual = 0;
                break;
            }

            if ((argv[1][1] == 'u') ||(argv[1][1] == 'U')) {  /* mux channel */
                if (argc <= 3)
                    goto usage;
            
                mux = simple_strtoul(argv[3], NULL, 16);
                channel = simple_strtoul(argv[4], NULL, 10);
            
                if ((argv[2][0] == 'e') ||(argv[2][0] == 'E')) {
                    ch = 1<<channel;
                 if ((ret = i2c_write(mux, 0, 0, &ch, 1)) != 0) {
                        printf("i2c failed to enable mux 0x%x channel %d.\n", 
                               mux, channel);
                    }
                    else
                        printf("i2c enabled mux 0x%x channel %d.\n", 
                               mux, channel);
                }
                else {
                    ch = 0;
                 if ((ret = i2c_write(mux, 0, 0, &ch, 1)) != 0) {
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
            if (regnum == 0xff) {
                reg = 0;
                regnum = 0;
            }
            if ((argv[1][1] == 'r') ||(argv[1][1] == 'R')) {  /* read */
                len = simple_strtoul(argv[4], NULL, 10);
                if (len)
                {
                    if (reg == 0) {
                        if ((ret = i2c_read_noaddr(device, tdata, len)) != 0) {
                            printf ("I2C read from device 0x%x failed.\n", 
                                    device);
                        }
                    }
                    else {
                        if ((ret = i2c_read(device, regnum, reg, 
                            tdata, len)) != 0) {
                            printf ("I2C read from device 0x%x failed.\n", 
                                    device);
                        }
                    }
                    if (ret == 0) {
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
                if ((argv[5][0] == 'c') ||(argv[5][0] == 'C')) 
                {
                    for (i = 0; i < len; i++) {
                        cksum += tdata[i];
                    }
                    tdata[len] = (cksum & 0xff00) >> 8;
                    tdata[len+1] = cksum & 0xff;
                    len = len + 2;
                }
                if (reg == 0) {
                    if ((ret = i2c_write_noaddr(device, tdata, len)) != 0)
                        printf ("I2C write to device 0x%x failed.\n", device);
                }
                else {
                    if ((ret = i2c_write(device, regnum, reg, tdata, len))!= 0)
                        printf ("I2C write to device 0x%x failed.\n", device);
                }
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
                        if ((ret = secure_eeprom_read(device, offset, tdata, 
                                                      len)) != 0) {
                            printf ("I2C read from secure EEPROM device 0x%x "
                                    "failed.\n", 
                                    device);
                            return (-1);
                        }
                    }
                    else {
                        if ((ret = eeprom_read(device, offset, tdata, 
                                               len)) != 0) {
                            printf ("I2C read from EEPROM device 0x%x "
                                    "failed.\n", 
                                    device);
                            return (-1);
                        }
                    }
                    printf ("The data read from offset - \n", offset);
                    for (i = 0; i < len; i++)
                        printf("%02x ", tdata[i]);
                    printf("\n");
                }
            }
            else {  /* write -- offset auto increment */
                 if ((argv[1][2] == 'o') ||(argv[1][2] == 'O')) { 
                    if ((argv[4][0] == 's') ||(argv[4][0] == 'S'))
                        secure = 1;

                    /* Print the address, followed by value.  
                    * Then accept input for the next value.
                    * A non-converted value exits.
                    */
                    do {
                        if (secure) {
                            if ((ret = secure_eeprom_read(device, offset, 
                                                          tdata, 1)) != 0) {
                                printf ("I2C read from EEPROM device "
                                        "0x%x at 0x%x failed.\n", 
                                        device, offset);
                                return (1);
                            }
                        }
                        else {
                            if ((ret = eeprom_read(device, offset, 
                                                   tdata, 1)) != 0) {
                                printf ("I2C read from EEPROM device "
                                        "0x%x at 0x%x failed.\n", 
                                        device, offset);
                                return (1);
                            }
                        }
                        printf("%4d: %02x", offset, tdata[0]);

                        nbytes = readline (" ? ");
                        if (nbytes == 0 || 
                            (nbytes == 1 && console_buffer[0] == '-')) {
                            /* <CR> pressed as only input, don't modify current
                            * location and move to next. 
                            * "-" pressed will go back.
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
                                                                   offset, 
                                                                   tdata, 1))
                                                                   != 0) {
                                        printf ("I2C write to EEPROM device "
                                                "0x%x at 0x%x failed.\n", 
                                                device, offset);
                                        return (1);
                                    }
                                }
                                else {
                                    if ((ret = eeprom_write(device, offset, 
                                                            tdata, 1)) != 0) {
                                        printf ("I2C write to EEPROM device "
                                                "0x%x at 0x%x failed.\n", 
                                                device, offset);
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
                    if ((ret = secure_eeprom_write(device, offset, tdata, 
                                                   len)) != 0) {
                        printf ("I2C write to EEPROM device 0x%x failed.\n", 
                                device);
                    }
                }
                else {
                    if ((ret = eeprom_write(device, offset, tdata, len)) 
                            != 0) {
                        printf ("I2C write to EEPROM device 0x%x failed.\n", 
                                device);
                    }
                }
            }
                        
            break;


        case 'f':          /* fan/fdr */
        case 'F':
            if (argc < 2)
                goto usage;

            if ((argc == 3) && ((argv[1][1] == 'd') ||(argv[1][1] == 'D'))) {
                fdr = simple_strtoul(argv[2], NULL, 16);
                i2c_fdr(fdr, CFG_I2C_SLAVE);
                return (1);
            }
            
            value = 1000000;
            if (argc == 3)
                value = simple_strtoul(argv[2], NULL, 10) * 1000;
            if ((argc == 4) && ((argv[3][0] == 'r') ||(argv[3][0] == 'R')))
                fan = simple_strtoul(argv[2], NULL, 10);
            if (argc == 5) {
                if ((argv[3][0] == 'd') ||(argv[3][0] == 'D')) {
                    fan = simple_strtoul(argv[2], NULL, 10);
                    if ((fan <= 0) || (fan >5)) {
                        printf("Fan number #d out of range 1 - 5.\n", fan);
                    }
                    duty = simple_strtoul(argv[4], NULL, 10);
                    if ((duty < 0) || (duty >100)) {
                        printf("Fan duty #d out of range 0 - 100.\n", duty);
                    }
                }
                else 
                    goto usage;
            }

            if ((argc == 4) || (argc == 5)){
                if (argc == 5) {
                    i2c_set_fan(fan, duty);
                    udelay(3000000);
                }
                count1 = 0xff;
                i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 1);  /* enable mux chan 0 */
                switch (fan) {
                    case 1:
                        addr8 = CFG_I2C_C1_9548SW1_ENV_0;
                        reg8 = 0x28;
                        break;
                        
                    case 2:
                        addr8 = CFG_I2C_C1_9548SW1_ENV_0;
                        reg8 = 0x29;
                        break;
                        
                    case 3:
                        addr8 = CFG_I2C_C1_9548SW1_ENV_0;
                        reg8 = 0x2a;
                        break;
                        
                    case 4:
                        addr8 = CFG_I2C_C1_9548SW1_ENV_1;
                        reg8 = 0x28;
                        break;
                        
                    case 5:
                        addr8 = CFG_I2C_C1_9548SW1_ENV_1;
                        reg8 = 0x29;
                        break;
                }
                
                i2c_read(addr8, reg8, 1, &count1, 1); /* fan count */

                i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 0);  /* disable mux */
                
                rpm1 = 0;
                if (count1 != 0xff)
                    rpm1 = 1350000  / (16 * count1);
                if (argc == 4) {
                    printf("Fan %d (addr %02x, reg %02x = %02x) RPM = %d.\n", 
                           fan, addr8, reg8, count1, rpm1);
                }
                else {
                    printf("Fan %d duty %d percent RPM = %d.\n", 
                           fan, duty, rpm1);
                }
                return (1);
            }

            i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 1);  /* enable mux chan 0 */
            tdata[0] = 0x20;
            i2c_write(CFG_I2C_C1_9548SW1_ENV_0, 0x5b, 1, 
                      tdata, 1); /* fan 1 TAC */
            i2c_write(CFG_I2C_C1_9548SW1_ENV_0, 0x5e, 1, 
                      tdata, 1); /* fan 2 TAC */
            i2c_write(CFG_I2C_C1_9548SW1_ENV_0, 0x5f, 1, 
                      tdata, 1); /* fan 3 TAC */
            i2c_write(CFG_I2C_C1_9548SW1_ENV_1, 0x5b, 1, 
                      tdata, 1); /* fan 1 TAC */
            i2c_write(CFG_I2C_C1_9548SW1_ENV_1, 0x5e, 1, 
                      tdata, 1); /* fan 2 TAC */
            udelay(3000000);

            printf("              Fan 1      Fan 2      Fan 3      Fan 4      "
                   "Fan 5\n");
            for (i = 0; i <= 0xf; i++) {
                count1 = count2 = count3 = count4 = count5 = 0xff;
                tdata[0] = i * 0x10 + 0xf;
                i2c_write(CFG_I2C_C1_9548SW1_ENV_0, 0x5b, 1, 
                          tdata, 1); /* fan 1 TAC */
                i2c_write(CFG_I2C_C1_9548SW1_ENV_0, 0x5e, 1, 
                          tdata, 1); /* fan 2 TAC */
                i2c_write(CFG_I2C_C1_9548SW1_ENV_0, 0x5f, 1, 
                          tdata, 1); /* fan 3 TAC */
                i2c_write(CFG_I2C_C1_9548SW1_ENV_1, 0x5b, 1, 
                          tdata, 1); /* fan 4 TAC */
                i2c_write(CFG_I2C_C1_9548SW1_ENV_1, 0x5e, 1, 
                          tdata, 1); /* fan 5 TAC */
                udelay(value);
                i2c_read(CFG_I2C_C1_9548SW1_ENV_0, 0x28, 1, 
                         &count1, 1); /* fan 1 count */
                i2c_read(CFG_I2C_C1_9548SW1_ENV_0, 0x29, 1, 
                         &count2, 1); /* fan 2 count */
                i2c_read(CFG_I2C_C1_9548SW1_ENV_0, 0x2a, 1, 
                         &count3, 1); /* fan 3 count */
                i2c_read(CFG_I2C_C1_9548SW1_ENV_1, 0x28, 1, 
                         &count4, 1); /* fan 4 count */
                i2c_read(CFG_I2C_C1_9548SW1_ENV_1, 0x29, 1, 
                         &count5, 1); /* fan 5 count */
                duty = tdata[0] * 100 / 0xff;
                rpm1 = rpm2 = rpm3 = rpm4 = rpm5 = 0;
                if ((count1 != 0xff) && (count1 != 0x0))
                    rpm1 = 1350000  / (16 * count1);
                if ((count2 != 0xff) && (count2 != 0x0))
                    rpm2 = 1350000  / (16 * count2);
                if ((count3 != 0xff) && (count3 != 0x0))
                    rpm3 = 1350000  / (16 * count3);
                if ((count4 != 0xff) && (count4 != 0x0))
                    rpm4 = 1350000  / (16 * count4);
                if ((count5 != 0xff) && (count5 != 0x0))
                    rpm5 = 1350000  / (16 * count5);
                
                printf("Duty %3d\%", duty);
                printf("    %4dRPM    %4dRPM    %4dRPM    %4dRPM    %4dRPM\n",
                       rpm1, rpm2, rpm3, rpm4, rpm5);
            }
            i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 0);  /* disable mux */
            break;
            
        case 't':          /* I2C temperature */
        case 'T':
            value = 1000000;
            if (argc > 2) {
                if ((argv[2][0] == 's') ||(argv[3][0] == 'S')) {/* SFP+ temp */
                    i2c_sfp_temp();
                    return 1;
                }
                value = simple_strtoul(argv[2], NULL, 10);
            }
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
            
        case 'v':          /* voltage */
        case 'V':
            i2c_read_volt();
            i2c_read_PWR1220_volt();
            break;

        case 's':          /* scan */
        case 'S':
            if ((argc == 3) && ((argv[2][0] == 'r') ||(argv[2][0] == 'R')))
                rsta = 1;
            
            for (i = 0; i < 0x7F; i++) {
                if (rsta) 
                    ret = i2c_read(i, 0, 1, tdata, 1);
                else
                    ret = i2c_read_norsta(i, 0, 1, tdata, 1);
                if (ret == 0)
                    printf ("Found I2C device at 0x%02x.\n", i);
                udelay(100000);
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

static void 
epld_dump (void)
{
    int i;
    uint16_t val;

    for (i = EPLD_RESET_BLOCK_UNLOCK; i <= EPLD_LAST_REG; i++) {
        if (epld_reg_read(i, &val))
            printf("EPLD addr 0x%02x = 0x%04x.\n", i*2, val);
        else
            printf("EPLD addr 0x%02x read failed.\n", i*2);
    }
    printf("\n", i);
}

/* EPLD commands
 *
 * Syntax:
 *    epld register read [addr]
 *    epld register write addr value
 */
int 
do_epld (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int addr = 0;
    uint16_t value = 0, mask, temp, lcmd;
    char cmd1, cmd2, cmd3;
    int port;
    PortLink link;
    PortDuplex duplex;
    PortSpeed speed;

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
                    epld_dump();
                    break;
                    
                case 'r':   /* read */
                case 'R':
                    if (argc <= 3)
                        goto usage;
                    
                    addr = simple_strtoul(argv[3], NULL, 16);
                    if (epld_reg_read(addr/2, &value))
                        printf("EPLD addr 0x%02x = 0x%04x.\n", addr, value);
                    else
                        printf("EPLD addr 0x%02x read failed.\n", addr);
                    break;
                    
                case 'w':   /* write */
                case 'W':
                    if (argc <= 4)
                        goto usage;
                    
                    addr = simple_strtoul(argv[3], NULL, 16);
                    value = simple_strtoul(argv[4], NULL, 16);
                    if (argc >= 6) {
                        mask = simple_strtoul(argv[5], NULL, 16);
                        epld_reg_read(addr/2, &temp);
                        value = (temp & mask) | value;
                    }
                    if (!epld_reg_write(addr/2, value))
                        printf("EPLD write addr 0x%02x value 0x%04x failed.\n",
                               addr, value);
                    else {
                        if (epld_reg_read(addr/2, &value))
                            printf("EPLD read back addr 0x%02x = 0x%04x.\n", 
                                   addr, value);
                        else
                            printf("EPLD addr 0x%02x read back failed.\n", 
                                   addr);
                    }
                    break;
                    
                default:
                    printf("???");
                    break;
            }
            
            break;
            
        case 'w':   /* Watchdog */
        case 'W':
            switch (cmd2) {
                case 'e':
                case 'E':
                    epld_watchdog_enable();
                    break;
                    
                case 'd':
                case 'D':
                    if ((argv[2][1] == 'u') ||(argv[2][1] == 'U'))
                        epld_watchdog_dump();
                    else
                        epld_watchdog_disable();
                    break;
                    
                case 'r':
                case 'R':
                    epld_watchdog_reset();
                    break;
                    
                default:
                    printf("???");
                    break;
            }
            break;
            
        case 'c':       /* CPU */
        case 'C':
            if (argc <= 3)
                goto usage;

            if ((cmd2 == 'r') ||(cmd2 == 'R')) {
                cmd3 = argv[3][0];
                if ((cmd3 == 's') ||(cmd3 == 'S'))
                    epld_cpu_soft_reset();
                else if ((cmd3 == 'h') ||(cmd3 == 'H'))
                    epld_cpu_hard_reset();
                else
                    printf("???");
            }
            else
                printf("???");
            
            break;
            
        case 's':       /* System */
        case 'S':
            if ((cmd2 == 'r') ||(cmd2 == 'R')) {
                if (argc == 3)
                    epld_system_reset();
                else if ((argc == 4) && ((argv[3][0] == 'h') || 
                            (argv[3][0] == 'H')))
                    epld_system_reset_hold();
                else
                    printf("???");
            }
            else
                printf("???");
            
            break;
            
        case 'l':        /* LED/LCD */
        case 'L':
            if (((cmd2 == 'p') ||(cmd2 == 'P')) && ((argv[2][1] =='o') || 
                (argv[2][1] == 'O'))) {       /* port LED */
                if (argc <= 6)
                    goto usage;

                port = simple_strtol(argv[3], NULL, 10);
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
                    if ((argv[6][2] == 'g') ||(argv[6][2] == 'G'))
                        speed = SPEED_10G;
                    else if ((argv[6][2] == 'm') ||(argv[6][2] == 'M'))
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
            else {      /* LCD */
                switch (cmd2) {
                    case 'o':
                    case 'O':
                        if ((argv[2][1] == 'n') ||(argv[2][1] == 'N'))
                            lcd_init(0);
                        else if ((argv[2][1] == 'f') ||(argv[2][1] == 'F'))
                            lcd_off(0);
                        else
                            goto usage;
                        break;
                    
                    case 'b':
                    case 'B':
                        if ((argv[3][0] == 'o') ||(argv[3][0] == 'O')) {
                            if ((argv[3][1] == 'n') ||(argv[3][1] == 'N'))
                                lcd_backlight_control(0, LCD_BACKLIGHT_ON);
                            else if ((argv[3][1] == 'f') ||(argv[3][1] == 'F'))
                                lcd_backlight_control(0, LCD_BACKLIGHT_OFF);
                            else
                                goto usage;
                        }
                        else
                            goto usage;
                        break;
                    
                    case 'h':
                    case 'H':
                        lcd_heartbeat(0);
                        break;
                    
                    case 'i':
                    case 'I':
                        lcd_init(0);
                        break;
                    
                    case 'p':
                    case 'P':
                        lcd_printf(0, "%s\n",argv[3]);
                        break;
                    
                    case 'C':
                    case 'c':
                        lcmd = simple_strtoul(argv[3], NULL, 16);
                        lcd_cmd(0, lcmd);
                        break;
                    
                    case 'D':
                    case 'd':
                        lcd_dump(0);
                        break;
                    
                    default:
                        printf("???");
                        break;
                }
            }
            break;
            
        case 'i':       /* i2c */
        case 'I':
            if (argc <= 2)
                goto usage;
            
            temp = simple_strtoul(argv[2], NULL, 10);
            if ((temp = 0) || (temp > 5))
                printf("reset number out of range 1-5\n");
            if ((cmd2 == 'r') ||(cmd2 == 'R')) {
                if ((temp >= 1) && (temp <= 3)) {
                    epld_reg_read(EPLD_WATCHDOG_LEVEL_DEBUG, &value);
                    value &= ~(1 << (temp + 4));
                    epld_reg_write(EPLD_WATCHDOG_LEVEL_DEBUG, value);
                }
                else {
                    epld_reg_read(EPLD_MISC_CONTROL_0, &value);
                    value &= ~(1 << (temp + 10));
                    epld_reg_write(EPLD_MISC_CONTROL_0, value);
                }
            }
            else if ((cmd2 == 'c') ||(cmd2 == 'C')) {
                if ((temp >= 1) && (temp <= 3)) {
                    epld_reg_read(EPLD_WATCHDOG_LEVEL_DEBUG, &value);
                    value |= (1 << (temp + 4));
                    epld_reg_write(EPLD_WATCHDOG_LEVEL_DEBUG, value);
                }
                else {
                    epld_reg_read(EPLD_MISC_CONTROL_0, &value);
                    value |= (1 << (temp + 10));
                    epld_reg_write(EPLD_MISC_CONTROL_0, value);
                }
            }
            else
                printf("???");
            
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
            return 0;
        }
        printf("\n\nLoop:  %d\n", j);
        j++;
        for (i = 1; i <argc; ++i) {
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

int 
do_usleep (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    ulong delay;

    if (argc != 2) {
        printf ("Usage:\n%s\n", cmdtp->usage);
        return (1);
    }

    delay = simple_strtoul(argv[1], NULL, 10);

    udelay(delay);

    return (0);
}

typedef struct 
    { char *name; ushort id; char *clei; ushort macblk; } 
    assm_id_type;

assm_id_type assm_id_list_system [] = {
 { "EX4500-40F"     , I2C_ID_JUNIPER_EX4500_40F, "N/A       " ,0x40},
 { "EX4500-20F"     , I2C_ID_JUNIPER_EX4500_20F, "N/A       " ,0x40},
 {  NULL             , 0                          }
};

assm_id_type assm_id_list_m1_40 [] = {
 { "M1-SFP+"     , I2C_ID_TSUNAMI_UPLINK_SFPPLUS_4PORT_PIC, "N/A       " ,0},
 {  NULL             , 0                          }
};

assm_id_type assm_id_list_m1_80 [] = {
 { "M1-SFP+"     , I2C_ID_TSUNAMI_UPLINK_SFPPLUS_4PORT_PIC, "N/A       " ,0},
 {  NULL             , 0                          }
};

assm_id_type assm_id_list_m2 [] = {
 { "M2-LBK"     , I2C_ID_TSUNAMI_M2_LOOPBACK_PIC, "N/A       " ,0},
 { "M2-OPTIC"     , 0x0A03, "N/A       " ,0},
 {  NULL             , 0                          }
};

static int 
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
    int access = 0, present, rev = 0;
    uint16_t id;
    uint8_t device, mux[2], chan[2], nmux = 0, cmux = 0;
    unsigned int start_address = 0;
    int controller = CFG_I2C_CTRL_1;
    int ret, assm_id = 0;
    int i, j, nbytes;
    uint8_t tdata[0x80];
    char tbyte[3];
    
    if (argc <= 2)
    {   printf ("Usage:\n%s\n", cmdtp->usage);
        return (1);
    }
    
    if (strcmp(argv[1], "system") == 0) {
        access = 0;
    }
    else if (strcmp(argv[1], "m1-40") == 0) {
        if ((present = m1_40_present()) == 0) {
            printf("No card present at M1-40 slot\n");
            return (1);
        }
        m1_40_id(&rev, &id, &device);
        access = 1;
    }
    else if (strcmp(argv[1], "m1-80") == 0) {
        if ((present = m1_80_present()) == 0) {
            printf("No card present at M1-80 slot\n");
            return (1);
        }
        m1_80_id(&rev, &id, &device);
        access = 2;
    }
    else if (strcmp(argv[1], "m2") == 0) {
        if ((present = m2_present()) == 0) {
            printf("No card present at M2 slot\n");
            return (1);
        }
        m2_id(&rev, &id, &device);
        access = 3;
    }
    else {
        printf ("Usage:\n%s\n", cmdtp->usage);
        return (1);
    }

    switch(access)
    {
        case 0: /* T40 */
            controller   = CFG_I2C_CTRL_1;
            mux[0] = CFG_I2C_C1_9548SW1;
            chan[0] = 4;
            nmux = 1;
            device = CFG_I2C_C1_9548SW1_P4_SEEPROM;
            assm_id_list = assm_id_list_system;
            break;
        case 1:  /* M1-40 */
            controller   = CFG_I2C_CTRL_2;
            mux[0] = CFG_I2C_C2_9546SW2;
            chan[0] = 3;
            assm_id_list = assm_id_list_m1_40;
            if (rev == 1) {
                nmux = 1;
            }
            else if (rev == 2) {
                mux[1] = CFG_I2C_M1_SFPP_9548SW1;
                chan[1] = 6;
                nmux = 2;
            }
            break;
        case 2:  /* M1-80 */
            controller   = CFG_I2C_CTRL_2;
            mux[0] = CFG_I2C_C2_9546SW2;
            chan[0] = 2;
            assm_id_list = assm_id_list_m1_80;
            if (rev == 1) {
                nmux = 1;
            }
            else if (rev == 2) {
                mux[1] = CFG_I2C_M1_SFPP_9548SW1;
                chan[1] = 6;
                nmux = 2;
            }
            break;
        case 3:  /* M2 */
            controller   = CFG_I2C_CTRL_2;
            mux[0] = CFG_I2C_C2_9546SW2;
            chan[0] = 1;
            assm_id_list = assm_id_list_m2;
            if (rev == 1) {
                nmux = 1;
            }
            else if (rev == 2) {
                mux[1] = CFG_I2C_M2_LBK_9548SW1;
                chan[1] = 1;
                nmux = 2;
            }
            break;
    }

    cmux = 0;
    i2c_mfgcfg = 1;
    for (i = 0; i < nmux; i++) {
        if ((ret = i2c_mux(controller, mux[i], chan[i], 1)) != 0) {
            printf("i2c failed to enable mux 0x%x channel 0x%x.\n", 
                   mux[i], chan[i]);
            cmux = i;
            break;
        }
    }

    if (cmux) {  /* setup failed, reversal to clear setup */
        for (i = cmux; i > 0; i--)
            i2c_mux(controller, mux[i], chan[i], 0);
        i2c_mfgcfg = 0;
        return (1);
    }

    if (secure_eeprom_read (device, start_address, tdata, 0x80)) {
        printf ("ERROR reading eeprom at device 0x%x\n", device);
        for (i = nmux; i > 0; i--)
            i2c_mux(controller, mux[i], chan[i], 0);
        i2c_mfgcfg = 0;
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
        if (!strcmp(argv[2], "update"))
        {
            /* constants for all eeproms*/    
            memcpy (tdata      , "\x7F\xB0\x02\xFF", 4);
            memcpy (tdata+0x08 , "REV "            , 4);         
            memcpy (tdata+0x31 , "\xFF\xFF\xFF"    , 3);
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
              
                if (nbytes >= 16 && 
                    (!strncmp(console_buffer+10, " REV ", 5) || 
                    !strncmp(console_buffer+10, " Rev ", 5) ))
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
            if (assm_id_list[i].name && 
                strncmp(assm_id_list[i].clei, "N/A", 10))
            {
                printf("\n"
                       "Assembly Model Name   : '%.23s'\n " ,tdata+0x4f); 
                printf("Assembly Model Rev    : '%.3s'\n"   ,tdata+0x66); 
                nbytes = readline ("Enter Assembly Model Name: ");
                if (nbytes >= 13) 
                {
                    strncpy (tdata+0x4f, console_buffer,13);
                    tdata [0x4f+13] = 0;
                  
                    if (nbytes >= 19 && 
                        (!strncmp(console_buffer+13, " REV ", 5) || 
                        !strncmp(console_buffer+13, " Rev ", 5) ))
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
            

            if (!strcmp(argv[1], "system")) 
            {  
                /* constants for system eeprom */   
                memcpy (tdata+0x34, "\xAD\x01\x00",3);

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

            if (secure_eeprom_write (device, start_address, tdata, 0x80)) {
                printf ("ERROR writing eeprom at device 0x%x\n", device);
            }
        }
        else if (!strcmp(argv[2], "clear"))
        {
            memset (tdata, 0xFF, 0x80);
            if (secure_eeprom_write (device, start_address, tdata, 0x80)) {
                printf ("ERROR clearing eeprom at device 0x%x\n", device);
            }
        }       

        /* always reread */
        if (secure_eeprom_read (device, start_address, tdata, 0x80)) {
            printf ("ERROR reading eeprom at device 0x%x\n", device);
        }
        else {
            for (i = 0; i < 0x80; i+=8) {
                printf("%02X : ",i + start_address);
                for (j = 0; j < 8; j++)
                    printf("%02X ",tdata[i+j]);
                printf("\n");
            }
        }
    } 
    
    for (i = nmux; i > 0; i--)
        i2c_mux(controller, mux[i], chan[i], 0);
    i2c_mfgcfg = 0;

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
        if (!strcmp(argv[1], "system")) 
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

/* signal integrate commands
 *
 * Syntax:
 */
int 
do_si(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char cmd1;
    volatile immap_t *immap = (immap_t *) CFG_CCSRBAR;
    volatile ccsr_gur_t *gur = &immap->im_gur;
    uint32_t temp;
    int fm_ratio = 1000, ret;
    NS16550_t s_port = (NS16550_t)CFG_NS16550_COM1;
    unsigned char ch;
    uint8_t tdata[20], fm;
    char *arg = "0";
    int reg = 0, shift = 0, bit = 0;
    uint16_t data = 0;

    cmd1 = argv[1][0];
    switch (cmd1) {
        case 'g':
        case 'G':
            if (argc <= 1)
                goto usage;
            if (((argv[1][1] == 'p') ||(argv[1][1] == 'P')) &&
                 ((argv[1][2] == 'i') ||(argv[1][2] == 'I'))) { /* gpin */
                 printf("gpin = 0x%02x\n", gur->gpindr);
            }
            else {  /* gpout */
                if (argc <= 3)
                    goto usage;
                temp = simple_strtoul(argv[3], NULL, 10);
                if (temp > 6)
                    printf("device number out of range 0-6\n");
                temp = 1 << (7-temp);
                if (((argv[2][0] == 'o') ||(argv[2][0] == 'O')) &&
                     ((argv[2][1] == 'n') ||(argv[2][1] == 'N'))) { /* on */
                    gur->gpoutdr |= temp;
                }
                else {
                    gur->gpoutdr &= ~temp;
                }
                printf("gpout = 0x%02x\n", gur->gpoutdr);
            }
            
            break;
            
        case 'f':
        case 'F':
            if (argc < 3)
                goto usage;

            arg = argv[2];
            if (!strcmp(arg, "-5")) { 
                fm = 0x1b; 
                fm_ratio = 950;
            }
            else if (!strcmp(arg, "-3")) { 
                fm = 0x1a; 
                fm_ratio = 970;
            }
            else if (!strcmp(arg, "0")) { 
                fm = 0x0; 
                fm_ratio = 1000;
            }
            else if (!strcmp(arg, "1")) { 
                fm = 0x1; 
                fm_ratio = 1010;
            }
            else if (!strcmp(arg, "2")) { 
                fm = 0x2; 
                fm_ratio = 1020;
            }
            else if (!strcmp(arg, "3")) { 
                fm = 0x3; 
                fm_ratio = 1030;
            }
            else if (!strcmp(arg, "4")) { 
                fm = 0x4; 
                fm_ratio = 1040;
            }
            else if (!strcmp(arg, "5")) { 
                fm = 0x5; 
                fm_ratio = 1050;
            }
            else {
                goto usage;
            }
            i2c_mux(0, CFG_I2C_C1_9548SW1, 6, 1);  /* enable mux chan 6 */
            i2c_controller(CFG_I2C_CTRL_1);
            ch = 1<<7;
            if ((ret = i2c_mux(0, CFG_I2C_C1_9548SW1, 7, 1)) == 0) {
                if ((ret = i2c_read(CFG_I2C_C1_9548SW1_P7_MK1493, 0x80, 1, 
                                    tdata, 1)) == 0) {
                    tdata[0] = (tdata[0] & 0xe0) | fm;
                    if ((ret = i2c_write(CFG_I2C_C1_9548SW1_P7_MK1493, 0x80, 1,
                                         tdata, 1)) == 0) {
                        /* adjust CPU and CCB clock */
                        gd->cpu_clk = (gd->bd->bi_intfreq / 1000) * fm_ratio;
                        gd->bus_clk = (gd->bd->bi_busfreq / 1000) * fm_ratio;
                            
                        /* adjust divisor for baudrate */
                        NS16550_init(s_port, gd->bus_clk / 16 / gd->baudrate);
                        printf("\nfrequency margin command -- 0x%x\n", 
                               tdata[0]);
                    }
                    else {
                        printf("\nfailed to write to i2c address 0x%x", 
                               CFG_I2C_C1_9548SW1_P7_MK1493);
                        i2c_mux(0, CFG_I2C_C1_9548SW1, 7, 0);
                        return (1);
                    }
                }
                else {
                    printf("\failed to read from i2c address 0x%x", 
                           CFG_I2C_C1_9548SW1_P7_MK1493);
                    i2c_mux(0, CFG_I2C_C1_9548SW1, 7, 0);
                    return (1);
                }
            }
            i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 0);
                        
            break;
            
        case 'v':
        case 'V':
            if (argc <= 2)
                goto usage;

            arg = argv[2];
            if (!strncmp(argv[1], "vm_pwr1220", 3)) {
                if ((!strcmp(arg, "SFP+")) || (!strcmp(arg, "sfp+"))) {
                    reg = 0x13;  /* TRIM1 */
                }
                else if ((!strcmp(arg, "PHY1")) || (!strcmp(arg, "phy1"))){ 
                    reg = 0x14;  /* TRIM2 */
                }
                else if ((!strcmp(arg, "PHY2")) || (!strcmp(arg, "phy2"))) { 
                    reg = 0x15;  /* TRIM3 */
                }
                else if ((!strcmp(arg, "L1")) || (!strcmp(arg, "l1"))) { 
                    reg = 0x16;  /* TRIM4 */
                }
                else if ((!strcmp(arg, "L2")) || (!strcmp(arg, "l2"))) { 
                    reg = 0x17;  /* TRIM5 */
                }
                else
                    goto usage;

                arg = argv[3];
                if (!strcmp(arg, "-5")) { 
                    tdata[0] = 0xd0;
                }
                else if (!strcmp(arg, "+5")) { 
                    tdata[0] = 0x50;
                }
                else if (!strcmp(arg, "0")) { 
                    tdata[0] = 0x80;
                }
                else
                    goto usage;
                
                i2c_mux(0, CFG_I2C_C1_9548SW1, 6, 1);  /* enable mux chan 6 */
                i2c_write(CFG_I2C_C1_9548SW1_P6_PWR1220, reg, 1, tdata, 1);
                i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 0);
                
                return (1);
            }

            /* vm */
            if (!strcmp(arg, "3.3")) { 
                reg = EPLD_MISC_CONTROL; 
                shift = 4;
            }
            else if (!strcmp(arg, "2.5")) { 
                reg = EPLD_PUSH_BUTTON; 
                shift = 14;
            }
            else if (!strcmp(arg, "1.0")) { 
                reg = EPLD_MISC_CONTROL; 
                shift = 2;
            }
            else if (!strcmp(arg, "1.1")) { 
                reg = EPLD_PUSH_BUTTON; 
                shift = 12;
            }
            else
                goto usage;
            
            arg = argv[3];
            if (!strcmp(arg, "-5")) { 
                bit = 0; 
            }
            else if (!strcmp(arg, "+5")) { 
                bit = 3; 
            }
            else if (!strcmp(arg, "0")) { 
                bit = 2; 
            }
            else
                goto usage;

            epld_reg_read(reg, &data);
            data &= ~(0x3 << shift);
            data |= (bit << shift);
            epld_reg_write(reg, data);
                                    
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

/***************************************************/

U_BOOT_CMD(
    isp,    5,  1,  do_isp,
    "isp     - isp1568 commands\n",
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
    "pcie init\n"
    "    - pcie init\n"
);

U_BOOT_CMD(
    pcieprobe,     4,      1,       do_pcie_probe,
    "pcieprobe  - List the PCI-E devices found on the bus\n",
    "<controller>  [-v]/[-d](v - verbose d - dump)\n"
);

U_BOOT_CMD(
    tsec,   7,  1,  do_tsec,
    "tsec    - tsec utility commands\n",
    "\n"
    "tsec mode [set | get] <mode>\n"
    "    - MDIO mode 0-phy1145, 1-M2/M40, 2-phy1112, 3-M80\n"
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
    "tsec info [0|1|2|3]\n"
    "    - tsec info\n"
    "tsec init\n"
    "    - init tsec\n"
    "tsec loopback [0|1|2|3] [mac | phy [1145|1112]  | ext [10|100|1000]] "
    "<count> [debug]\n"
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

U_BOOT_CMD(
    resetenv,      1,     1,      resetenv_cmd,
    "resetenv	- Return all environment variable to default.\n",
    " \n"
    "\t Erase the environemnt variable sector.\n"
);

#if !defined(CONFIG_PRODUCTION)
U_BOOT_CMD(
    i2c_ctrl,   2,  1,  do_i2c_controller,
    "i2c_ctrl     - select I2C controller\n",
    "ctrl\n    - I2C controller number\n"
    "      (valid controller values 1..2, default 1)\n"
);

U_BOOT_CMD(
    i2c_expander, 7, 1, do_i2c_diag,
    "i2c_expander   - diag on slave device directly on i2c + IOExpander\n",
    "   r/w read (1) or write (0) operation\n"
    "   ctrl specifies the i2c ctrl 1 or 2\n"
    "   slave_addr  address (in hex) of the device on i2c\n"
    "   reg - register (in hex) in the device to which data to be r/w\n"
    "   data     the data to be r/w\n"
    "   length_data length (decimal) of the data to r/w; for IOexp, this is"
    "1\n"
);

U_BOOT_CMD(
    i2c_switch, 9, 1,   do_i2c_diag_switch,
    "i2c_switch   - i2c on a specific slave device with switch\n",
    "   r/w  read (1) or write (0) operation\n"
    "   ctrl specifies the i2c ctrl 1 or 2\n"
    "   switch  the switch address (hex) the dev sits on\n"
    "   channel the channel number on the switch\n"
    "   slave_addr address (hex) of the device on i2c\n"
    "   reg - register (hex) in the device to which data to be r/w, "
    "0xff - reg not applicable\n"
    "   data    the data to be r/w\n"
    "   length_data length (decimal) of the data to r/w\n"
);

U_BOOT_CMD(
    i2c_cksum, 2, 1,    do_i2c_cksum,
    "i2c_cksum   - i2c Tx checksum on/off\n",
    "   on/off   Tx checksum on (1) or off (0)\n"
);

U_BOOT_CMD(
    runloop,	CFG_MAXARGS,	1,	do_runloop,
    "runloop - run commands in an environment variable\n",
    "var [...]\n"
    "    - run the commands in the environment variable(s) 'var' "
    "infinite loop\n"
);

U_BOOT_CMD(
    usleep ,    2,    2,     do_usleep,
    "usleep  - delay execution for usec\n",
    "\n"
    "    - delay execution for N micro seconds (N is _decimal_ !!!)\n"
);

U_BOOT_CMD(
    i2c,    8,  1,  do_i2c,
    "i2c     - I2C test commands\n",
    "\n"
    "i2c bw <address> <value>\n"
    "    - write byte to i2c address\n"
    "i2c br <address>\n"
    "    - read byte from i2c address\n"
    "i2c dw <address> <reg> <data> [checksum]\n"
    "    - write data to i2c address\n"
    "i2c dr <address> <reg> <len>\n"
    "    - read data from i2c address\n"
    "i2c ew <address> <offset> <data> [hex|char]\n"
    "    - write data to EEPROM at i2c address\n"
    "i2c er <address> <offset> <len>\n"
    "    - read data from EEPROM at i2c address\n"
    "i2c ews <address> <offset> <data> [hex|char]\n"
    "    - write data to secure EEPROM at i2c address\n"
    "i2c ers <address> <offset> <len>\n"
    "    - read data from secure EEPROM at i2c address\n"
    "i2c ewo <address> <offset> [secure]\n"
    "    - write byte to offset (auto-incrementing) of the device EEPROM\n"
    "i2c mux [enable|disable] <mux addr> <channel>\n"
    "    - enable/disable mux channel\n"
    "i2c LED <unit> set <led> <color> <state>\n"
    "    - I2C LED [0|1] set [top|middle|bottom] [off|red|green|amber] "
    "[steady|slow|fast]\n"
    "i2c LED <unit> update\n"
    "    - I2C LED [0|1] update LED\n"
    "i2c fan [[<delay> |<fan> duty <percent>]|[<fan> rpm]]\n"
    "    - i2c fan test interval delay #ms (1000ms) or individual fan duty\n"
    "i2c temperature [SFP+ |<delay>] [<loop>] [<threshold>]\n"
    "    - i2c temperature reading delay #us (100000) for Winbond read "
    "temperature\n"
    "i2c voltage\n"
    "    - i2c voltage reading for Winbond\n"
    "i2c fdr <fdr>\n"
    "    - I2C set fdr\n"
    "i2c scan [rstart]\n"
    "    - I2C scan 0-0x7F [repeat start]\n"
    "i2c manual [off|on]\n"
    "    - I2C manual operation off (default) or on\n"
);

U_BOOT_CMD(
    epld,   7,  1,  do_epld,
    "epld    - EPLD test commands\n",
    "\n"
    "epld register read <address>\n"
    "    - read EPLD register\n"
    "epld register write <address> <value> [<mask>]\n"
    "    - write EPLD register with option mask\n"
    "epld register dump\n"
    "    - dump EPLD registers\n"
    "epld watchdog [enable | disable | reset | dump]\n"
    "    - EPLD watchdog operation\n"
    "epld CPU reset [soft | hard]\n"
    "    - EPLD CPU reset\n"
    "epld system reset [hold]\n"
    "    - EPLD system reset\n"
    "epld i2c [reset |clear] <1..5>\n"
    "    i2c mux reset/clear I2C_RST_L<n>\n"
    "    1-C0 70(U17) / C1 71(U19) / C1 73(U20)\n"
    "    2-C1 71P0 74(U51) / C1 71P1 74(U53) / C1 71P2 74(U52)\n"
    "    3-C1 21(U16) / C1 M71P0 24(U55) / C1 M71P2 24(U208)/\n"
    "      C1 M71P2 20(U207) / C1 M71P2 22(U210)\n"
    "    4-C1 M71P3 74(U67) / C1 M72P0 74(U68)\n"
    "    5-C1 M71P1 20(U65) / C1 M71P1 22(U66) / C1 M71P1 24(U64)\n"
    "epld LCD backlight [on | off]\n"
    "    - EPLD LCD backlight\n"
    "epld LCD init\n"
    "    - EPLD init\n"
    "epld LCD [on | off]\n"
    "    - EPLD LCD on/off\n"
    "epld LCD heartbeat <unit>\n"
    "    - EPLD LCD heartbeat\n"
    "epld LCD print <string>\n"
    "    - EPLD LCD print\n"
    "epld LCD dump\n"
    "    - EPLD LCD DDRAM\n"
    "epld LCD command <cmd>\n"
    "    - EPLD LCD control command\n"
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
    mfgcfg,    3,    0,     do_mfgcfg,
    "mfgcfg  - manufacturing config of EEPROMS\n",
    "\nmfgcfg [system|m1-40|m1-80|m2] [read|update|clear]\n"
);

U_BOOT_CMD(
    si,    4,    1,     do_si,
    "si      - signal integrate\n",
    "\n"
    "si gpin \n"
    "    - gpin status bit0-7\n"
    "si gpout [on | off] <device>\n"
    "    - gpout [on|off]  0 -M1-40, 1 - M1-80k, 2 - M2, 3 - ISP1568, "
    "4 - ST72681, 5 - W83782G, 6 - ISP1520\n"
    "si fm [-5|-3|0|1|2|3|4|5]\n"
    "    - Frequency margin MK1493 -5/-3/0/1/2/3/4/5 %\n"
    "si vm [3.3|2.5|1.1|1.0] [-5|+5|0]\n"
    "    - Voltage margin 3.3/2.5/1.1/1.0 V -5/+5/0 %\n"
    "si vm_pwr1220 [SFP+|PHY1|PHY2|L1|L2] [-5|+5|0]\n"
    "    - Voltage margin SFP+(3.3V)/PHY(1.2V)/Lion(1.8V) -5/+5/0 %\n"
);

#endif
