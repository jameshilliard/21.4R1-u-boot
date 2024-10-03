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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/gpl-2.0.html>.
 */

#include <common.h>
#include <command.h>

#if (CONFIG_COMMANDS & CFG_CMD_BSP)
#include <asm/processor.h>
#include <asm/immap_85xx.h>
#include <i2c.h>
#include <asm/io.h>
#include "pcie.h"
#include <net.h>
#include <usb.h>
#include <config.h>
#include <common.h>
#include <part.h>
#include <fat.h>

extern block_dev_desc_t usb_dev_desc[USB_MAX_STOR_DEV];
extern block_dev_desc_t nor_dev;

/* Prototypes */
void pcieinfo(int, int);
void pciedump(int);
static char *pcie_classes_str(unsigned char);
void pcie_header_show_brief(pcie_dev_t);
void pcie_header_show(pcie_dev_t);
int do_pcie_probe(cmd_tbl_t *, int, int, char *[]);

DECLARE_GLOBAL_DATA_PTR;


extern int cmd_get_data_size(char* arg, int default_size);
extern block_dev_desc_t * sata_get_dev(int dev);


/* Show information about devices on PCIe bus. */

void pcieinfo(int busnum, int short_listing)
{
	unsigned int device = 0, function = 0;
	unsigned char header_type;
	unsigned short vendor_id;
	pcie_dev_t dev;

	printf("\nScanning PCIE devices on bus %d\n", busnum);
	
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
	
			pcie_read_config_word(dev, PCIE_VENDOR_ID, &vendor_id);
			if ((vendor_id == 0xFFFF) || (vendor_id == 0x0000))
				continue;
	
			pcie_read_config_byte(dev, PCIE_HEADER_TYPE, &header_type);
		
			if (short_listing) {
				printf("%02x.%02x.%02x   ",busnum, device, function);
				pcie_header_show_brief(dev);
			} else {
				printf("\nFound PCIE device %02x.%02x.%02x:\n",busnum, device,function); 	pcie_header_show(dev);
			}
		}
	}
}

void pciedump(int busnum)
{
	pcie_dev_t dev;
	unsigned char buff[256], temp[20];
	int addr,offset = 0;
	int linebytes;
	int nbytes = 256;
	int i,j=0;

	printf("\nScanning PCIE devices on bus %d\n", busnum);

	dev = PCIE_BDF(busnum, 0, 0);
	memset(buff,0xff,256);

	for (addr = 0; addr < 265; addr++) 
	pcie_read_config_byte(dev, addr,(buff+addr));

	do {
		printf("0x%02x :",offset);
		linebytes = (nbytes < 16)?nbytes:16;

		for (i=0; i < linebytes; i++) {
			if ((i==4) || (i==8)|| (i == 12))
				printf(" ");

			temp[i] = buff[j];
			printf(" %02x",buff[j]);
			j++;
		}
		printf("  ");

		for (i=0; i < linebytes; i++) {
			if ((i==4) || (i==8)|| (i == 12))
				printf(" ");
		
			if ((temp[i] < 0x20) || (temp[i] > 0x7e)) 
				putc('.');
			else
				putc(temp[i]);
		}
		printf("\n");

		offset = offset + 16;
		nbytes = nbytes - linebytes;
	} while (nbytes);

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
	return  "Unknown class";
		break;
	};
}


/* Reads and prints the header of the specified PCIe device in short form 
 * Inputs: dev	bus+device+function number
 */
	
void pcie_header_show_brief(pcie_dev_t dev)
{
	unsigned short vendor, device;
	unsigned char class, subclass;

	pcie_read_config_word(dev, PCIE_VENDOR_ID, &vendor);
	pcie_read_config_word(dev, PCIE_DEVICE_ID, &device);
	pcie_read_config_byte(dev, PCIE_CLASS_CODE, &class);
	pcie_read_config_byte(dev, PCIE_CLASS_SUB_CODE, &subclass);
				
	printf("0x%.4x     0x%.4x     %-23s 0x%.2x\n", 
			vendor, device, pcie_classes_str(class), subclass);
}

/* Reads the header of the specified PCIe device
 * Inputs: dev  bus+device+function number
 */

void pcie_header_show(pcie_dev_t dev)
{
	unsigned char _byte, header_type;
	unsigned short _word;
	unsigned int _dword;
		
#define PRINT(msg, type, reg) \
		pcie_read_config_##type(dev, reg, &_##type); \
		printf(msg, _##type)

	pcie_read_config_byte(dev, PCIE_HEADER_TYPE, &header_type);
	PRINT ("  vendor ID=                    0x%.4x\n", word, PCIE_VENDOR_ID);
	PRINT ("  device ID=                    0x%.4x\n", word, PCIE_DEVICE_ID);
	PRINT ("  command register=             0x%.4x\n", word, PCIE_COMMAND);
	PRINT ("  status register=              0x%.4x\n", word, PCIE_STATUS);
	PRINT ("  revision ID=                  0x%.2x\n", byte, PCIE_REVISION_ID);
	PRINT ("  sub class code=               0x%.2x\n", byte, PCIE_CLASS_SUB_CODE);
	PRINT ("  programming interface=        0x%.2x\n", byte, PCIE_CLASS_PROG);
	PRINT ("  cache line=                   0x%.2x\n", byte, PCIE_CACHE_LINE_SIZE);
	PRINT ("  header type=                  0x%.2x\n", byte, PCIE_HEADER_TYPE);
	PRINT ("  base address 0=               0x%.8x\n", dword, PCIE_BASE_ADDRESS_0);

	switch (header_type & 0x03) {
	case PCIE_HEADER_TYPE_NORMAL:   /* "normal" PCIE device */
		PRINT ("  base address 1=               0x%.8x\n", dword, PCIE_BASE_ADDRESS_1);
		PRINT ("  base address 2=               0x%.8x\n", dword, PCIE_BASE_ADDRESS_2);
		PRINT ("  base address 3=               0x%.8x\n", dword, PCIE_BASE_ADDRESS_3);
		PRINT ("  base address 4=               0x%.8x\n", dword, PCIE_BASE_ADDRESS_4);
		PRINT ("  base address 5=               0x%.8x\n", dword, PCIE_BASE_ADDRESS_5);
		PRINT ("  sub system vendor ID=         0x%.4x\n", word, PCIE_SUBSYSTEM_VENDOR_ID);
		PRINT ("  sub system ID=                0x%.4x\n", word, PCIE_SUBSYSTEM_ID);
		PRINT ("  interrupt line=               0x%.2x\n", byte, PCIE_INTERRUPT_LINE);
		PRINT ("  interrupt pin=                0x%.2x\n", byte, PCIE_INTERRUPT_PIN);
		break;

	case PCIE_HEADER_TYPE_BRIDGE:   /* PCIE-to-PCIE bridge */
		PRINT ("  base address 1=               0x%.8x\n", dword, PCIE_BASE_ADDRESS_1);
		PRINT ("  primary bus number=           0x%.2x\n", byte, PCIE_PRIMARY_BUS);
		PRINT ("  secondary bus number=         0x%.2x\n", byte,PCIE_SECONDARY_BUS);
		PRINT ("  subordinate bus number=       0x%.2x\n", byte, PCIE_SUBORDINATE_BUS);
		PRINT ("  IO base=                      0x%.2x\n", byte, PCIE_IO_BASE);
		PRINT ("  IO limit=                     0x%.2x\n", byte, PCIE_IO_LIMIT);
		PRINT ("  memory base=                  0x%.4x\n", word, PCIE_MEMORY_BASE);
		PRINT ("  memory limit=                 0x%.4x\n", word, PCIE_MEMORY_LIMIT);
		PRINT ("  prefetch memory base=         0x%.4x\n", word, PCIE_PREF_MEMORY_BASE);
		PRINT ("  prefetch memory limit=        0x%.4x\n", word, PCIE_PREF_MEMORY_LIMIT);
		PRINT ("  interrupt line=               0x%.2x\n", byte, PCIE_INTERRUPT_LINE);
		PRINT ("  interrupt pin=                0x%.2x\n", byte, PCIE_INTERRUPT_PIN);
		PRINT ("  bridge control=               0x%.4x\n", word, PCIE_BRIDGE_CONTROL);
		break;
	
	default:
		printf("unknown header\n");
		break;
	}

#undef PRINT
}

/* This routine services the "pcieprobe" command.
 * Syntax - pcieprobe [-v] [-d]
 *            -v for verbose, -d for dump.
 */

int do_pcie_probe(cmd_tbl_t *cmd, int flag, int argc, char *argv[])
{
	if (argc == 2) {
		if (strcmp(argv[1],"-v") == 0) {
			pcieinfo(0,0); /* 0 - bus0 0 - verbose list */
			pcieinfo(1,0); /* 1 - bus1 0 - verbose list */
			pcieinfo(2,0); /* 1 - bus1 0 - verbose list */
		} else if (strcmp(argv[1],"-d") == 0) {
			pciedump(0);  /* 0 - bus0 */
			pciedump(1);  /* 1 - bus1 */
			pciedump(2);  /* 1 - bus1 */
		}
	} else {
		pcieinfo(0,1); /* 0 - bus0 1 - short list */
		pcieinfo(1,1); /* 1 - bus1 1 - short list */
		pcieinfo(2,1); /* 1 - bus1 0 - short list */
	}
	
	return 0;
}

U_BOOT_CMD(
	pcieprobe,     2,      1,       do_pcie_probe,
	"pcieprobe  - List the PCI-E devices found on the bus\n",
	"pcieprobe [-v]/[-d](v - verbose d - dump)\n"
);


/* Convert the "bus.device.function" identifier into a number.
 */
static pcie_dev_t get_pcie_dev(char* name)
{
	char cnum[12];
	int len, i, iold, n;
	int bdfs[3] = {0, 0, 0};

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
	strncpy(cnum, &name[iold], sizeof(cnum));
	cnum[sizeof(cnum) -1] = '\0';

	if (n == 0)
		n = 1;
	bdfs[n] = simple_strtoul(cnum, NULL, 16);
	return PCIE_BDF(bdfs[0], bdfs[1], bdfs[2]);
}

static int pcie_cfg_display(pcie_dev_t bdf, ulong addr, ulong size, ulong length)
{
#define DISP_LINE_LEN	16
	ulong i, nbytes, linebytes;
	int rc = 0;

	if (length == 0)
		length = 0x40 / size; /* Standard PCI configuration space */

	/* Print the lines.
	 * once, and all accesses are with the specified bus width.
	 */
	nbytes = length * size;
	do {
		uint	val4;
		ushort  val2;
		u_char	val1;

		printf("%08lx:", addr);
		linebytes = (nbytes>DISP_LINE_LEN)?DISP_LINE_LEN:nbytes;
		for (i=0; i < linebytes; i+= size) {
			if (size == 4) {
				pcie_read_config_dword(bdf, addr, &val4);
				printf(" %08x", val4);
			} else if (size == 2) {
				pcie_read_config_word(bdf, addr, &val2);
				printf(" %04x", val2);
			} else {
				pcie_read_config_byte(bdf, addr, &val1);
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

static int pcie_cfg_write(pcie_dev_t bdf, ulong addr, ulong size, ulong value)
{
	printf("b.d.f=%d.%d.%d reg=%02x size=%d val=%02x\n",
			PCIE_BUS(bdf),PCIE_DEV(bdf),PCIE_FUNC(bdf), addr, size, value);
	if (size == 4) {
		pcie_write_config_dword(bdf, addr, value);
	}
	else if (size == 2) {
		ushort val = value & 0xffff;
		pcie_write_config_word(bdf, addr, val);
	} else {
		u_char val = value & 0xff;
		pcie_write_config_byte(bdf, addr, val);
	}
	return 0;
}

static int pcie_cfg_modify(pcie_dev_t bdf, ulong addr, ulong size, ulong value, int incrflag)
{
	ulong	i;
	int	nbytes;
	extern char console_buffer[];
	uint	val4;
	ushort  val2;
	u_char	val1;

	/* Print the address, followed by value.  Then accept input for
	 * the next value.  A non-converted value exits.
	 */
	do {
		printf("%08lx:", addr);
		if (size == 4) {
			pcie_read_config_dword(bdf, addr, &val4);
			printf(" %08x", val4);
		}
		else if (size == 2) {
			pcie_read_config_word(bdf, addr, &val2);
			printf(" %04x", val2);
		} else {
			pcie_read_config_byte(bdf, addr, &val1);
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
			break;	/* timed out, exit the command	*/
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
				pcie_cfg_write (bdf, addr, size, i);
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
 *	pci display[.b, .w, .l] bus.device.function} [addr] [len]
 *	pci next[.b, .w, .l] bus.device.function [addr]
 *      pci modify[.b, .w, .l] bus.device.function [addr]
 *      pci write[.b, .w, .l] bus.device.function addr value
 */
int do_pcie(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong addr = 0, value = 0, size = 0;
	pcie_dev_t bdf = 0;
	char cmd = 's';

	if (argc > 1)
		cmd = argv[1][0];

	switch (cmd) {
	case 'd':		/* display */
	case 'n':		/* next */
	case 'm':		/* modify */
	case 'w':		/* write */
		/* Check for a size specification. */
		size = cmd_get_data_size(argv[1], 4);
		if (argc > 3)
			addr = simple_strtoul(argv[3], NULL, 16);
		if (argc > 4)
			value = simple_strtoul(argv[4], NULL, 16);
	case 'h':		/* header */
		if (argc < 3)
			goto usage;
		if ((bdf = get_pcie_dev(argv[2])) == -1)
			return 1;
		break;
	default:		/* scan bus */
		value = 1; /* short listing */
		bdf = 0;   /* bus number  */
		if (argc > 1) {
			if (argv[argc-1][0] == 'l') {
				value = 0;
				argc--;
			}
			if (argc > 1)
				bdf = simple_strtoul(argv[1], NULL, 16);
		}
		pcieinfo(bdf, value);
		return 0;
	}

	switch (argv[1][0]) {
	case 'h':		/* header */
		pcie_header_show(bdf);
		return 0;
	case 'd':		/* display */
		return pcie_cfg_display(bdf, addr, size, value);
	case 'n':		/* next */
		if (argc < 4)
			goto usage;
		return pcie_cfg_modify(bdf, addr, size, value, 0);
	case 'm':		/* modify */
		if (argc < 4)
			goto usage;
		return pcie_cfg_modify(bdf, addr, size, value, 1);
	case 'w':		/* write */
		if (argc < 5)
			goto usage;
		return pcie_cfg_write(bdf, addr, size, value);
	}

	return 1;
 usage:
	printf("Usage:\n%s\n", cmdtp->usage);
	return 1;
}


U_BOOT_CMD(
	pcie,	5,	1,	do_pcie,
	"pcie     - list and access PCIE Configuration Space\n",
	"[bus] [long]\n"
	"    - short or long list of PCIE devices on bus 'bus'\n"
	"pcie header b.d.f\n"
	"    - show header of PCIE device 'bus.device.function'\n"
	"pcie display[.b, .w, .l] b.d.f [address] [# of objects]\n"
	"    - display PCIE configuration space (CFG)\n"
	"pcie next[.b, .w, .l] b.d.f address\n"
	"    - modify, read and keep CFG address\n"
	"pcie modify[.b, .w, .l] b.d.f address\n"
	"    -  modify, auto increment CFG address\n"
	"pcie write[.b, .w, .l] b.d.f address value\n"
	"    - write to CFG address\n"
);


/* storage device commands
 *
 * Syntax:
 */
extern block_dev_desc_t usb_dev_desc[USB_MAX_STOR_DEV];
#if defined(USB_WRITE_READ)
extern unsigned long usb_stor_write(unsigned long device,unsigned long blknr, 
                       unsigned long blkcnt, unsigned long *buffer);
#endif

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
    printf("Usage:\n%s\n", cmdtp->usage);
    return 1;
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


#endif /* (CONFIG_COMMANDS & CFG_CMD_BSP) */
