/*
 * Copyright (c) 2008-2010, Juniper Networks, Inc.
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
	#include <pcie.h>
	#include "lc_cpld.h"
	#include "fsl_i2c.h"
	#include <net.h>
	#include <usb.h>
	#include <config.h>
	#include <common.h>
	#include <part.h>
	#include <fat.h>
	#include "ex82xx_devices.h"
	#include "ccb_iic.h"
	#include "ex82xx_i2c.h"
	#include "../common/rtc1672.h"
	#include "ex82xx_common.h"
	#include "post.h"

extern block_dev_desc_t usb_dev_desc[USB_MAX_STOR_DEV];
extern char console_buffer[];
extern int cmd_get_data_size(char* arg, int default_size);
extern int pcie_init(void);
extern void show_mastership_status(void);
extern void release_mastership_status(void);
extern void request_mastership_status(void);
extern int tsec_initialize_debug(unsigned char enable, int index, char* devname);
/* Prototypes */
void pcieinfo(int, int);
void pciedump(int);
static char* pcie_classes_str(unsigned char);
void pcie_header_show_brief(pcie_dev_t);
void pcie_header_show(pcie_dev_t);
int do_pcie_probe(cmd_tbl_t*, int, int, char*[]);

DECLARE_GLOBAL_DATA_PTR;


#define swap_ulong(x)						\
	({ unsigned long x_ = (unsigned long)x;	\
	   (unsigned long)(						\
		   ((x_ & 0x000000FFUL) << 24) |	\
		   ((x_ & 0x0000FF00UL) <<  8) |	\
		   ((x_ & 0x00FF0000UL) >>  8) |	\
		   ((x_ & 0xFF000000UL) >> 24) );	\
	 })

#define swap_short(x)						  \
	({ unsigned short x_ = (unsigned short)x; \
	   (unsigned short)(					  \
		   ((x_ & 0x00FF) << 8) |			  \
		   ((x_ & 0xFF00) >> 8) );			  \
	 })


static int atoh(char* string)
{
	int res = 0;

	while (*string != 0) {
		res *= 16;
		if (*string >= '0' && *string <= '9') {
			res += *string - '0';
		} else if (*string >= 'A' && *string <= 'F') {
			res += *string - 'A' + 10;
		} else if (*string >= 'a' && *string <= 'f') {
			res += *string - 'a' + 10;
		} else {
			break;
		}
		string++;
	}
	return res;
}


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
			if (function && !(header_type & 0x80)) {
				break;
			}

			dev = PCIE_BDF(busnum, device, function);

			pcie_read_config_word(dev, PCIE_VENDOR_ID, &vendor_id);
			if ((vendor_id == 0xFFFF) || (vendor_id == 0x0000)) {
				continue;
			}

			pcie_read_config_byte(dev, PCIE_HEADER_TYPE, &header_type);

			if (short_listing) {
				printf("%02x.%02x.%02x   ", busnum, device, function);
				pcie_header_show_brief(dev);
			} else {
				printf("\nFound PCIE device %02x.%02x.%02x:\n",
					   busnum,
					   device,
					   function);
				pcie_header_show(dev);
			}
		}
	}
}


void pciedump(int busnum)
{
	pcie_dev_t dev;
	unsigned char buff[256], temp[20];
	int addr, offset = 0;
	int linebytes;
	int nbytes = 256;
	int i, j = 0;

	printf("\nScanning PCIE devices on bus %d\n", busnum);

	dev = PCIE_BDF(busnum, 0, 0);
	memset(buff, 0xff, 256);

	for (addr = 0 ; addr < 265 ; addr++) {
		pcie_read_config_byte(dev, addr, (buff + addr));
	}

	do {
		printf("0x%02x :", offset);
		linebytes = (nbytes < 16) ? nbytes : 16;

		for (i = 0 ; i < linebytes ; i++) {
			if ((i == 4) || (i == 8) || (i == 12)) {
				printf(" ");
			}
			temp[i] = buff[j];
			printf(" %02x", buff[j]);
			j++;
		}
		printf("  ");
		for (i = 0 ; i < linebytes ; i++) {
			if ((i == 4) || (i == 8) || (i == 12)) {
				printf(" ");
			}
			if ((temp[i] < 0x20) || (temp[i] > 0x7e)) {
				putc('.');
			} else {
				putc(temp[i]);
			}
		}
		printf("\n");

		offset = offset + 16;
		nbytes = nbytes - linebytes;
	} while (nbytes);

	printf("\n");
}


static char* pcie_classes_str(u8 class)
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
		return "???";
		break;
	}
	;
}


/* Reads and prints the header of the specified PCIe device in short form
 * Inputs: dev  bus+device+function number
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

#define PRINT(msg, type, reg)						 \
	pcie_read_config_ ## type(dev, reg, &_ ## type); \
	printf(msg, _ ## type)

	pcie_read_config_byte(dev, PCIE_HEADER_TYPE, &header_type);
	PRINT("vendor ID = 0x%.4x\n", word, PCIE_VENDOR_ID);
	PRINT("device ID = 0x%.4x\n", word, PCIE_DEVICE_ID);
	PRINT("command register = 0x%.4x\n", word, PCIE_COMMAND);
	PRINT("status register = 0x%.4x\n", word, PCIE_STATUS);
	PRINT("revision ID = 0x%.2x\n", byte, PCIE_REVISION_ID);
	PRINT("sub class code = 0x%.2x\n", byte, PCIE_CLASS_SUB_CODE);
	PRINT("programming interface = 0x%.2x\n", byte, PCIE_CLASS_PROG);
	PRINT("cache line = 0x%.2x\n", byte, PCIE_CACHE_LINE_SIZE);
	PRINT("header type = 0x%.2x\n", byte, PCIE_HEADER_TYPE);
	PRINT("base address 0 = 0x%.8x\n", dword, PCIE_BASE_ADDRESS_0);

	switch (header_type & 0x03) {
	case PCIE_HEADER_TYPE_NORMAL:   /* "normal" PCIE device */
		PRINT("base address 1 = 0x%.8x\n", dword, PCIE_BASE_ADDRESS_1);
		PRINT("base address 2 = 0x%.8x\n", dword, PCIE_BASE_ADDRESS_2);
		PRINT("base address 3 = 0x%.8x\n", dword, PCIE_BASE_ADDRESS_3);
		PRINT("base address 4 = 0x%.8x\n", dword, PCIE_BASE_ADDRESS_4);
		PRINT("base address 5 = 0x%.8x\n", dword, PCIE_BASE_ADDRESS_5);
		PRINT("sub system vendor ID = 0x%.4x\n", word, PCIE_SUBSYSTEM_VENDOR_ID);
		PRINT("sub system ID = 0x%.4x\n", word, PCIE_SUBSYSTEM_ID);
		PRINT("interrupt line = 0x%.2x\n", byte, PCIE_INTERRUPT_LINE);
		PRINT("interrupt pin = 0x%.2x\n", byte, PCIE_INTERRUPT_PIN);
		break;

	case PCIE_HEADER_TYPE_BRIDGE:   /* PCIE-to-PCIE bridge */
		PRINT("base address 1 = 0x%.8x\n", dword, PCIE_BASE_ADDRESS_1);
		PRINT("primary bus number = 0x%.2x\n", byte, PCIE_PRIMARY_BUS);
		PRINT("secondary bus number = 0x%.2x\n", byte, PCIE_SECONDARY_BUS);
		PRINT("subordinate bus number = 0x%.2x\n", byte, PCIE_SUBORDINATE_BUS);
		PRINT("IO base = 0x%.2x\n", byte, PCIE_IO_BASE);
		PRINT("IO limit = 0x%.2x\n", byte, PCIE_IO_LIMIT);
		PRINT("memory base = 0x%.4x\n", word, PCIE_MEMORY_BASE);
		PRINT("memory limit = 0x%.4x\n", word, PCIE_MEMORY_LIMIT);
		PRINT("prefetch memory base = 0x%.4x\n", word, PCIE_PREF_MEMORY_BASE);
		PRINT("prefetch memory limit = 0x%.4x\n", word, PCIE_PREF_MEMORY_LIMIT);
		PRINT("interrupt line = 0x%.2x\n", byte, PCIE_INTERRUPT_LINE);
		PRINT("interrupt pin =  0x%.2x\n", byte, PCIE_INTERRUPT_PIN);
		PRINT("bridge control = 0x%.4x\n", word, PCIE_BRIDGE_CONTROL);
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
int do_pcie_probe(cmd_tbl_t* cmd, int flag, int argc, char* argv[])
{
	int verbose = 0, dump = 0, bus, max_bus;

	if (argc == 2) {
		if (strcmp(argv[1], "-v") == 0) {
			verbose = 1;
			dump = 0;
		} else if (strcmp(argv[1], "-d") == 0) {
			verbose = 0;
			dump = 1;
		} else if (strcmp(argv[1], "init") == 0) {
			pcie_init();
			return 0;
		}
	} else {
		verbose = 0;
		dump = 0;
	}
	max_bus = pcie_hose_get_max_bus(0);
	if (!dump) {
		for (bus = 0; bus <= max_bus; bus++) {
			pcieinfo(bus, !verbose);
		}
	} else {
		for (bus = 0; bus <= max_bus; bus++) {
			pciedump(bus);
		}
	}

	return 0;
}


static pcie_dev_t get_pcie_dev(char* name)
{
	char cnum[12];
	int len, i, iold, n;
	int bdfs[3] = { 0, 0, 0 };

	len = strlen(name);
	if (len > 8) {
		return -1;
	}
	for (i = 0, iold = 0, n = 0; i < len; i++) {
		if (name[i] == '.') {
			memcpy(cnum, &name[iold], i - iold);
			cnum[i - iold] = '\0';
			bdfs[n++] = simple_strtoul(cnum, NULL, 16);
			iold = i + 1;
		}
	}
	strcpy(cnum, &name[iold]);
	if (n == 0) {
		n = 1;
	}
	bdfs[n] = simple_strtoul(cnum, NULL, 16);
	return PCIE_BDF(bdfs[0], bdfs[1], bdfs[2]);
}


static int pcie_cfg_display(pcie_dev_t bdf, ulong addr,
							ulong size, ulong length)
{
#define DISP_LINE_LEN    16
	ulong i, nbytes, linebytes;
	int rc = 0;

	if (length == 0) {
		length = 0x40 / size; /* Standard PCI configuration space */
	}
	/* Print the lines.
	 * once, and all accesses are with the specified bus width.
	 */
	nbytes = length * size;
	do {
		uint val4;
		ushort val2;
		u_char val1;

		printf("%08lx:", addr);
		linebytes = (nbytes > DISP_LINE_LEN) ? DISP_LINE_LEN : nbytes;
		for (i = 0; i < linebytes; i += size) {
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

	return rc;
}


static int pcie_cfg_write(pcie_dev_t bdf, ulong addr, ulong size, ulong value)
{
	if (size == 4) {
		pcie_write_config_dword(bdf, addr, value);
	} else if (size == 2) {
		ushort val = value & 0xffff;
		pcie_write_config_word(bdf, addr, val);
	} else {
		u_char val = value & 0xff;
		pcie_write_config_byte(bdf, addr, val);
	}
	return 0;
}


static int pcie_cfg_modify(pcie_dev_t bdf,
						   ulong addr,
						   ulong size,
						   ulong value,
						   int incrflag)
{
	ulong i;
	int nbytes;
	extern char console_buffer[];
	uint val4;
	ushort val2;
	u_char val1;

	/* Print the address, followed by value.  Then accept input for
	 * the next value.  A non-converted value exits.
	 */
	do {
		printf("%08lx:", addr);
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

		nbytes = readline(" ? ");
		if (nbytes == 0 || (nbytes == 1 && console_buffer[0] == '-')) {
			/* <CR> pressed as only input, don't modify current
			 * location and move to next. "-" pressed will go back.
			 */
			if (incrflag) {
				addr += nbytes ? -size : size;
			}
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
			char* endp;
			i = simple_strtoul(console_buffer, &endp, 16);
			nbytes = endp - console_buffer;
			if (nbytes) {
#ifdef CONFIG_BOOT_RETRY_TIME
				/* good enough to not time out
				 */
				reset_cmd_timeout();
#endif
				pcie_cfg_write(bdf, addr, size, i);
				if (incrflag) {
					addr += size;
				}
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
int do_pcie(cmd_tbl_t* cmdtp, int flag, int argc, char* argv[])
{
	uint32_t addr = 0, value = 0, size = 0;
	uint32_t data_32, pci_addr;
	uint8_t data;
	pcie_dev_t bdf = 0;
	char cmd = 's';
	uint32_t pci_base = 0, i = 0;
	volatile uint32_t* longp;
	volatile uint16_t* shortp;
	volatile uint8_t* cp;


	if (argc > 1) {
		cmd = argv[1][0];
	}

	switch (cmd) {
	case 'r':        /* reset */
		lc_cpld_reg_read(LC_CPLD_RESET_CTRL, &data);
		lc_cpld_reg_write(LC_CPLD_RESET_CTRL, data | 0x08);
		udelay(1000);
		lc_cpld_reg_write(LC_CPLD_RESET_CTRL, data & ~0x08);
		break;
	
	case 'i':       /* display */

		/* Check for a size specification. */
		size = cmd_get_data_size(argv[1], 4);
		if (argc > 3) {
			addr = simple_strtoul(argv[3], NULL, 16);
		}
		if (argc > 4) {
			value = simple_strtoul(argv[4], NULL, 16);
		}	
		if ((bdf = get_pcie_dev(argv[2])) == -1) {
			return 1;
		} 

		pcie_read_config_dword(bdf, PCIE_BASE_ADDRESS_0, &pci_base);

		if (argv[1][1] == 'r') {
			for (i = 0; i < value; i++) {
				pci_addr = addr + pci_base + (i * 4);
				if (size == 4) {
					longp = (uint32_t*)pci_addr;
					data_32 = swap_ulong(*longp);
					printf("Address = 0x%08X Data = 0x%08X\n",
					    pci_addr, data_32);
				}
				if (size == 2) {
					shortp = (uint16_t*)pci_addr;
					data_32 = swap_short(*shortp);
					printf("Address = 0x%08X Data = 0x%04X\n",
					    pci_addr, data_32);
				}
				if (size == 1) {
					cp = (uint8_t*)pci_addr;
					data_32 = *cp;
					printf("Address = 0x%08X Data = 0x%02X\n",
					    pci_addr, data_32);
				}    
			}
		} else {
			pci_addr = addr + pci_base;
			/* data_32 to write */
			data_32 = value;
			printf("Address = 0x%08X Data = 0x%08X \n", pci_addr, data_32);
			if (size == 4) {
				longp = (uint32_t*)pci_addr;
				*longp = swap_ulong(data_32);
			}
			if (size == 2) {
				shortp = (uint16_t*)pci_addr;
				*shortp = swap_short(data_32 & 0xFFFF);
			}
			if (size == 1) {
				cp = (uint8_t*)pci_addr;
				*cp = data_32;
			}    
		}
		break;

	case 'd':           /* display */
	case 'n':           /* next */
	case 'm':           /* modify */
	case 'w':           /* write */
		/* Check for a size specification. */
		size = cmd_get_data_size(argv[1], 4);
		if (argc > 3) {
			addr = simple_strtoul(argv[3], NULL, 16);
		}
		if (argc > 4) {
			value = simple_strtoul(argv[4], NULL, 16);
		}
	case 'h':        /* header */
		if (argc < 3) {
			goto usage;
		}
		if ((bdf = get_pcie_dev(argv[2])) == -1) {
			return 1;
		}
		break;
	default:        /* scan bus */
		value = 1;  /* short listing */
		bdf = 0;    /* bus number  */
		if (argc > 1) {
			if (argv[argc - 1][0] == 'l') {
				value = 0;
				argc--;
			}
			if (argc > 1) {
				bdf = simple_strtoul(argv[1], NULL, 16);
			}
		}
		pcieinfo(bdf, value);
		return 0;
	}

	switch (argv[1][0]) {
	case 'h':        /* header */
		pcie_header_show(bdf);
		return 0;
	case 'd':           /* display */
		return pcie_cfg_display(bdf, addr, size, value);
	case 'n':           /* next */
		if (argc < 4) {
			goto usage;
		}
		return pcie_cfg_modify(bdf, addr, size, value, 0);
	case 'm':        /* modify */
		if (argc < 4) {
			goto usage;
		}
		return pcie_cfg_modify(bdf, addr, size, value, 1);
	case 'w':        /* write */
		if (argc < 5) {
			goto usage;
		}
		return pcie_cfg_write(bdf, addr, size, value);
	}

	return 1;
usage:
	printf("Usage:\n%s\n", cmdtp->usage);
	return 1;
}


int do_i2c(cmd_tbl_t* cmdtp, int flag, int argc, char* argv[])
{
	char cmd1, cmd2;
	ulong controller = 0, device = 0;
	int i, ret, temp, len = 0, offset = 0, source_addr = 0;
	uint8_t fdr;
	char tbyte[3];
	uint8_t tdata[128];
	char* data;

	if (argc < 2) {
		goto usage;
	}

	cmd1 = argv[1][0];
	cmd2 = argv[2][0];

	switch (cmd1) {
	case 'c':    /* ctrl */
	case 'C':
		if (argc < 3) {
			goto usage;
		}
		controller = simple_strtoul(argv[2], NULL, 16);
		if (controller < 1 || controller > 2) {
			goto usage;
		}
		i2c_controller(controller);
		break;

	case 's':    /* switch ctrl */
	case 'S':

		if (argc < 4) {
			goto usage;
		}

		switch (cmd2) {
		case 's':    /* set switch ctrl */
		case 'S':

			device = simple_strtoul(argv[3], NULL, 10);
			tdata[0] = (1 << (uint8_t)(simple_strtoul(argv[4], NULL, 10) & 0xFF));
			printf("Selecting i2c switch 0x%02X - Channel %d \n", device,
				   ((uint8_t)(simple_strtoul(argv[4], NULL, 10) & 0xFF)));

			/* There is no offset for switch device. first data after the
			 * device address selects the channel*/
			if ((ret =
					 i2c_write(device, tdata[0], 1, &tdata[1] /*dummy*/,
							   0)) != 0) {
				printf("I2C write to device 0x%x failed.\n", device);
			}

			break;

		case 'g':    /* get switch ctrl */
		case 'G':

			device = simple_strtoul(argv[3], NULL, 10);

			/* There is no offset for switch device. first data after the
			 * device address selects the channel*/
			if ((ret = i2c_raw_read(device, tdata, 1)) != 0) {
				printf("I2C read from device 0x%x failed.\n", device);
			} else {
				if (tdata[0]) {
					printf(
						"Channels [bit map] 0x%X Selected on i2c switch 0x%02X \n",
						tdata[0],
						device);
				} else {
					printf("All channels on i2c switch 0x%02X are disabled  \n",
						   device);
				}
			}


			break;
		}

		break;

	case 'e':    /* EEPROM */
	case 'E':
		if (argc <= 4) {
			goto usage;
		}

		device = simple_strtoul(argv[2], NULL, 10);
		offset = simple_strtoul(argv[3], NULL, 10);


		if ((argv[1][1] == 'r') || (argv[1][1] == 'R')) {  /* read */
			len = simple_strtoul(argv[4], NULL, 10);
			if (len) {
				if ((ret = eeprom_read(device, offset, tdata, len)) != 0) {
					printf("I2C read from EEPROM device 0x%x failed.\n", device);
				} else {
					printf("The data read from offset - \n", offset);

					for (i = 0; i < len; i++) {
						printf("%02x ", tdata[i]);
					}

					printf("\n");
				}
			}
		} else if ((argv[1][1] == 'w') || (argv[1][1] == 'W')) {    /* write */
			data = argv[4];

			if ((argv[5][0] == 'h') || (argv[5][0] == 'H')) {
				/* to handle if the data is provided as 0xAABBCCD*/
				if ((argv[4][0] == '0') &&
					((argv[4][1] == 'x') || (argv[4][1] == 'X'))) {
					data += 2;
				}

				len = strlen(data) / 2;
				tbyte[2] = 0;
				for (i = 0; i < len; i++) {
					tbyte[0] = data[2 * i];
					tbyte[1] = data[2 * i + 1];
					temp = atoh(tbyte);
					tdata[i] = temp;
				}
			} else {
				len = strlen(data);
				for (i = 0; i < len; i++) {
					tdata[i] = data[i];
				}
			}

			if ((ret = eeprom_write(device, offset, tdata, len)) != 0) {
				printf("I2C write to EEROM device 0x%x failed.\n", device);
			}
		} else {
			/* program from bin data */
			device = simple_strtoul(argv[2], NULL, 10);
			source_addr = simple_strtoul(argv[3], NULL, 10);
			offset = simple_strtoul(argv[4], NULL, 10);
			len = simple_strtoul(argv[5], NULL, 10);
			printf(
				"Programming eeprom device 0x%02X - Source memory : 0x%08X \
					Destination Offset : 0x%08x Length : %d \n"                                                              ,
				device,
				source_addr,
				offset,
				len);
			if (len) {
				unsigned char* file_bin_addr = (unsigned char*)source_addr;

				for (i = 0; i < len; i++) {
					tdata[0] = file_bin_addr[i];

					if ((ret = eeprom_write(device, i, tdata, 1)) != 0) {
						printf("I2C write to EEROM device 0x%x failed.\n",
							   device);
						return 1;
					}

					udelay(100000);

					if (!(i % 10)) {
						printf(".");
					}
				}
			}
			printf("\n");
		}
		break;

	case 'w':           /* raw write */
	case 'W':

		device = simple_strtoul(argv[2], NULL, 10);
		data = argv[3];

		if ((argv[4][0] == 'h') || (argv[4][0] == 'H')) {
			/* to handle if the data is provided as 0xAABBCCD*/
			if ((argv[3][0] == '0') &&
				((argv[3][1] == 'x') || (argv[3][1] == 'X'))) {
				data += 2;
			}

			len = strlen(data) / 2;
			tbyte[2] = 0;
			for (i = 0; i < len; i++) {
				tbyte[0] = data[2 * i];
				tbyte[1] = data[2 * i + 1];
				temp = atoh(tbyte);
				tdata[i] = temp;
			}
		} else {
			len = strlen(data);
			for (i = 0; i < len; i++) {
				tdata[i] = data[i];
			}
		}
		if ((ret = i2c_raw_write(device, tdata, len)) != 0) {
			printf("I2C write to device 0x%x failed.\n", device);
		}

		break;

	case 'r':           /* read/reset */
	case 'R':
		if (((argv[1][1] == 'e') || (argv[1][1] == 'E')) &&
			((argv[1][2] == 's') || (argv[1][2] == 'S'))) {
			i2c_reset();
		} else {
			/*raw read*/
			device = simple_strtoul(argv[2], NULL, 10);
			len = simple_strtoul(argv[3], NULL, 10);
			if (len) {
				if ((ret = i2c_raw_read(device, tdata, len)) != 0) {
					printf("I2C read from device 0x%x failed.\n", device);
				} else {
					printf("The data read from offset - \n", offset);
					for (i = 0; i < len; i++) {
						printf("%02x ", tdata[i]);
					}
					printf("\n");
				}
			}
		}

		break;

	case 'f':           /* I2C FDR */
	case 'F':
		if (argc <= 3) {
			goto usage;
		}

		controller = simple_strtoul(argv[2], NULL, 16);
		fdr = simple_strtoul(argv[3], NULL, 16);
		i2c_controller(controller);
		i2c_fdr(fdr);
		printf("i2c[%d] fdr = 0x%x.\n", controller, fdr);
		break;

	case 'p':           /* I2C probe */
	case 'P':
		if (argc < 3) {
			goto usage;
		}

		device = simple_strtoul(argv[2], NULL, 16);
		if (i2c_probe(device) == 0) {
			printf("Device %02x is present \n", device);
		} else {
			printf("Device %02x is absent \n", device);
		}
		break;

	default:
		printf("???");
		break;
	}

	return 1;
usage:
	printf("Usage:\n%s\n", cmdtp->usage);
	return 1;
}


int do_i2ce_probe(u_int8_t group)
{
	int j;

	puts("Valid chip addresses:");
	for (j = 0; j < 128; j++) {
		if (ccb_iic_probe(group, j) == 0) {
			printf(" %02X", j);
		}
	}
	putc('\n');
	return 0;
}


int do_i2ce(cmd_tbl_t* cmdtp, int flag, int argc, char* argv[])
{
	char cmd1, cmd2;
	ulong group = 0, device = 0;
	int i, ret, temp, len = 0, offset = 0, source_addr = 0;
	char tbyte[3];
	uint8_t tdata[256];
	char* data;
	u8*a = (u8*)&offset;
	int alen = 1;

	if (!EX82XX_RECPU) {
		printf("This command is not supported on this platform");
		return 1;
	}

	if (argc < 2) {
		goto usage;
	}

	cmd1 = argv[1][0];
	cmd2 = argv[2][0];

	switch (cmd1) {
	case 'e':    /* EEPROM */
	case 'E':
		if (argc < 6) {
			goto usage;
		}

		group  = simple_strtoul(argv[2], NULL, 10) & 0xFF;
		device = simple_strtoul(argv[3], NULL, 10) & 0x7F;
		offset = simple_strtoul(argv[4], NULL, 10);


		if (offset > 0xFF) {
			alen = 2;
		}

		if (offset > 0xFFFF) {
			alen = 3;
		}

		if (offset > 0xFFFFFF) {
			alen = 4;
		}
		printf(" Grp = %x, device = %x offset = %x alen= %x \n",
			   group,
			   device,
			   offset,
			   alen) ;

		if ((argv[1][1] == 'r') || (argv[1][1] == 'R')) {  /* read */
			len = simple_strtoul(argv[5], NULL, 10);
			if (len) {
				if (device == SCB_SLAVE_CPLD_ADDR) {
					/* Need parity generation*/
					offset = ccb_i2cs_set_odd_parity((u_int8_t)(offset & 0xFF));
				}

				if ((ret = ccb_iic_write((u_int8_t)group, (u_int8_t)device, alen,
									   &a[4 - alen])) != 0) {
					printf("I2C read from device 0x%x failed.\n", device);
				} else {
					if ((ret = ccb_iic_read((u_int8_t)group, (u_int8_t)device,
										  len,
										  tdata)) != 0) {
						printf("I2C read from device 0x%x failed.\n", device);
					} else {
						printf("The data read from offset - \n", offset);
						for (i = 0; i < len; i++) {
							printf("%02x ", tdata[i]);
						}

						printf("\n");
					}
				}
			}
		} else if ((argv[1][1] == 'p') || (argv[1][1] == 'P')) {
			/* program from bin data */
			device = simple_strtoul(argv[3], NULL, 10);
			source_addr = simple_strtoul(argv[4], NULL, 10);
			offset = simple_strtoul(argv[5], NULL, 10);
			len = simple_strtoul(argv[6], NULL, 10);

			printf("Programming eeprom group = 0x%02X device 0x%02X -  \
				Source memory : 0x%08X Destination Offset :0x%08x Length :%d\n",
				group,
				device,
				source_addr,
				offset,
				len);
			if (len) {
				unsigned char* file_bin_addr = (unsigned char*)source_addr;
				for (i = 0; i < len; i++) {
					tdata[0] = i;
					tdata[1] = file_bin_addr[i];

					if ((ret =
							 ccb_iic_write((u_int8_t)group, (u_int8_t)device, 2,
										   tdata))) {
						printf(
							"I2C write to EEROM group 0x%X device 0x%x failed.\n",
							group,
							device);
					}
					udelay(100000);
					if (!(i % 10)) {
						printf(".");
					}
				}
			}
			printf("\n");
		} else {  /* write */
			for (i = 0; i < alen; i++) {
				tdata[i] = a[4 - alen + i];
			}
			data = argv[5];
			if ((argv[6][0] == 'h') || (argv[6][0] == 'H')) {
				/* to handle if the data is provided as 0xAABBCCD*/
				if ((argv[5][0] == '0') &&
					((argv[5][1] == 'x') || (argv[5][1] == 'X'))) {
					data += 2;
				}

				len = strlen(data) / 2;

				tbyte[2] = 0;
				for (i = 0; i < len; i++) {
					tbyte[0] = data[2 * i];
					tbyte[1] = data[2 * i + 1];
					temp = atoh(tbyte);
					tdata[i + alen] = temp;
				}
			} else {
				len = strlen(data);
				for (i = 0; i < len ; i++) {
					tdata[i + alen] = data[i];
				}
			}

			if (device == SCB_SLAVE_CPLD_ADDR) {
				/* Need parity generation*/
				tdata[0] = ccb_i2cs_set_odd_parity(tdata[0]);   /* add parity to offset */
				tdata[1] = ccb_i2cs_set_odd_parity(tdata[1]);   /* add parity to data   */
				/* assume len=1 and alen =1 for slave cpld access*/
				if (len != 1 || alen != 1) {
					goto usage;
				}
			}
			if ((ret =
					 ccb_iic_write((u_int8_t)group, (u_int8_t)device, len +
								   alen,
								   tdata))) {
				printf("I2C write to EEROM device 0x%x failed.\n", device);
			}
		}
		break;
	case 'r':           /* read/reset */
	case 'R':

		if (((argv[1][1] == 'e') || (argv[1][1] == 'E')) &&
			((argv[1][2] == 's') || (argv[1][2] == 'S'))) {
			ccb_iic_init();
			break;
		}
		break;

	case 'p':           /* probe */
	case 'P':
		if (argc < 3) {
			goto usage;
		}
		group  = simple_strtoul(argv[2], NULL, 10) & 0xFF;
		do_i2ce_probe(group);
		break;

	case 's':           /* select group */
	case 'S':
		if (argc < 3) {
			goto usage;
		}
		group  = simple_strtoul(argv[2], NULL, 10) & 0xFF;
		ccb_iic_ctlr_group_select(group);
		break;

	default:
		printf("???");
		break;
	}

	return 1;
usage:
	printf("Usage:\n%s\n", cmdtp->usage);
	return 1;
}


#ifdef USB_WRITE_READ
extern unsigned long usb_stor_write(unsigned long device, unsigned long blknr,
									unsigned long blkcnt, unsigned long* buffer);
#endif

/* storage device commands
 *
 * Syntax:
 */
int do_storage(cmd_tbl_t* cmdtp, int flag, int argc, char* argv[])
{
	char cmd1, cmd2;
	int device, block, count;
	ulong mem;
	volatile ulong* buffer;
	block_dev_desc_t* stor_dev = NULL;

	if (argc <= 6) {
		goto usage;
	}
	cmd1 = argv[1][0];
	cmd2 = argv[2][0];

	device = simple_strtoul(argv[3], NULL, 10);
	block = simple_strtoul(argv[4], NULL, 10);
	mem = simple_strtoul(argv[5], NULL, 16);
	buffer = (ulong*)mem;

	if ((cmd2 == 'n') || (cmd2 == 'N')) {
#if (CONFIG_COMMANDS & CFG_CMD_FAT)
		stor_dev = &nor_dev;
#else
		printf(" Nor device not supported.\n");
		goto usage;
#endif
	} else {
		if ((device >= 0) && (device < 5)) {
			stor_dev = &usb_dev_desc[device];
		}
		;
	}

	if (!stor_dev) {
		printf("No matched %s for device number %d\n", argv[2], device);
	}

	switch (cmd1) {
	case 'r':
	case 'R':
		count = simple_strtoul(argv[6], NULL, 10);
		stor_dev->block_read(device, block, count, (ulong*)buffer);

		break;

#if defined (USB_WRITE_READ)
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
		if (size & 511) {
			count++;
		}
		usb_stor_write(device, block, count, (ulong*)buffer);
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


static int do_rtc_1672(cmd_tbl_t* cmdtp, int flag, int argc, char* argv[])
{
	char cmd1, cmd2, cmd3_0;
	unsigned long curr_word;
	uint yrs, mon, day, hrs, min, sec;
	int i;

	if (!EX82XX_RECPU) {
		printf("Invalid command..RTC not present\n");
		return 1;
	}

	if (argc < 2) {
		goto usage;
	}

	cmd1 = argv[1][0];
	cmd2 = argv[2][0];

	switch (cmd1) {
	case 'e':       /* enable rtc*/
	case 'E':
		en_rtc_1672();
		printf("\n RTC Enabled \n");
		return 0;


	case 's':       /* show */
	case 'S':
		disp_clk_regs();
		return 0;

	case 'd':       /* date or disable rtc */
	case 'D':

		if (argv[1][1] == 'i' || argv[1][1] == 'I') {
			/* Disable RTC */
			dis_rtc_1672();
			printf("\n RTC Disabled \n");
			return 0;
		}

		/* date operation */
		curr_word = get_rtc1672_count_as_word(RTC_I2C_ADDR);
		day2date(curr_word, &yrs, &mon, &day, &hrs, &min, &sec);
		if (argc == 2) {
			printf("\n DATE : %04d-%02d-%02d \n", yrs, mon, day);
			return 0;
		}

		if ((argc != 4) && (argc != 6) && (argc != 8)) {
			goto usage;
		}

		for (i = 2; i < argc; i += 2) {
			cmd3_0 = argv[i][0];
			switch (cmd3_0) {
			case 'm':       /* show */
			case 'M':
				/* Need to modify month */
				mon = simple_strtoul(argv[i + 1], NULL, 10);
				if ((mon < 1) && (mon > 12)) {
					printf("\nEnter the month (1-12): ");
					return 1;
				}
				break;

			case 'd':       /* show */
			case 'D':
				/* Need to modify date */
				day = simple_strtoul(argv[i + 1], NULL, 10);
				if ((day < 1) && (day > 31)) {
					printf("\nEnter the date (1-31): ");
					return 1;
				}
				break;

			case 'y':       /* show */
			case 'Y':
				/* Need to modify year */
				yrs = simple_strtoul(argv[i + 1], NULL, 10);

				if ((yrs < 1970) && (yrs > 2099)) {
					printf("\nEnter the year (1970-2099): ");
					return 1;
				}
				break;
			}
		}

		curr_word = date2day(yrs, mon, day, hrs, min, sec);
		set_rtc1672_word_as_count(RTC_I2C_ADDR, curr_word);
		curr_word = get_rtc1672_count_as_word(RTC_I2C_ADDR);
		day2date(curr_word, &yrs, &mon, &day, &hrs, &min, &sec);
		printf("\n Modified DATE : %04d-%02d-%02d \n", yrs, mon, day);

		return 0;

	case 't':       /* time */
	case 'T':

		curr_word = get_rtc1672_count_as_word(RTC_I2C_ADDR);
		day2date(curr_word, &yrs, &mon, &day, &hrs, &min, &sec);
		if (argc == 2) {
			printf("\n TIME : %02d:%02d:%02d \n", hrs, min, sec);
			return 0;
		}

		if ((argc != 4) && (argc != 6) && (argc != 8)) {
			goto usage;
		}

		for (i = 2; i < argc; i += 2) {
			cmd3_0 = argv[i][0];
			switch (cmd3_0) {
			case 'h':       /* hours */
			case 'H':
				/* Need to modify hours */
				hrs = simple_strtoul(argv[i + 1], NULL, 10);
				if ((hrs < 1) && (hrs > 24)) {
					printf("\nEnter the hour (1-24): ");
					return 1;
				}
				break;

			case 'm':       /* minute */
			case 'M':
				/* Need to modify minute */
				min = simple_strtoul(argv[i + 1], NULL, 10);
				if ((min < 0) && (min > 59)) {
					printf("\nEnter the minute (0-59): ");
					return 1;
				}
				break;

			case 's':       /* sec */
			case 'S':
				/* Need to modify v */
				sec = simple_strtoul(argv[i + 1], NULL, 10);

				if ((sec < 0) && (sec > 59)) {
					printf("\nEnter the seconds (0-59): ");
					return 1;
				}
				break;
			}
		}

		curr_word = date2day(yrs, mon, day, hrs, min, sec);
		set_rtc1672_word_as_count(RTC_I2C_ADDR, curr_word);
		curr_word = get_rtc1672_count_as_word(RTC_I2C_ADDR);
		day2date(curr_word, &yrs, &mon, &day, &hrs, &min, &sec);
		printf("\n Modified TIME : %02d:%02d:%02d \n", hrs, min, sec);

		return 0;
	}
usage:
	printf("Usage:\n%s\n", cmdtp->usage);
	return 1;
}


static int do_scb_master(cmd_tbl_t* cmdtp, int flag, int argc, char* argv[])
{
	char cmd1;

	if (!EX82XX_RECPU) {
		printf("Invalid command..Operation not supported on LCs\n");
		return 1;
	}
	if (argc < 2) {
		goto usage;
	}

	cmd1 = argv[1][0];

	switch (cmd1) {
	case 's':
	case 'S':
		show_mastership_status();
		break;

	case 'g':
	case 'G':
		request_mastership_status();
		break;

	case 'r':
	case 'R':
		release_mastership_status();
		break;
	}

	return 0;

usage:
	printf("Usage:\n%s\n", cmdtp->usage);
	return 1;
}


static int do_debugport(cmd_tbl_t* cmdtp, int flag, int argc, char* argv[])
{
	char cmd1;

	if (EX82XX_RECPU) {
		printf("Invalid command..Operation not supported on RE\n");
		return 1;
	}
	if (argc < 2) {
		goto usage;
	}

	cmd1 = argv[1][0];

	switch (cmd1) {
	case 'e':
	case 'E':
		tsec_initialize_debug(1, 0, CONFIG_MPC85XX_TSEC1_NAME);
		break;

	case 'd':
	case 'D':
		tsec_initialize_debug(0, 0, CONFIG_MPC85XX_TSEC1_NAME);
		break;
	}
	return 0;
usage:
	printf("Usage:\n%s\n", cmdtp->usage);
	return 1;
}


int do_scb_read(cmd_tbl_t* cmdtp, int flag, int argc, char* argv[])
{
	ulong addr, data, offset, base;
	int size;
	volatile uint* longp;
	volatile ushort* shortp;
	volatile u_char* cp;

	if (argc < 2) {
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	/* Check for a size spefication.
	 * Defaults to long if no or incorrect specification.
	 */
	if ((size = cmd_get_data_size(argv[0], 4)) < 0) {
		return 1;
	}

	/* Address is always specified.
	 */

	offset = simple_strtoul(argv[1], NULL, 16);
	base   = ccb_get_regp();
	addr = offset + base;


	/* We want to optimize the loops to run as fast as possible.
	 * If we have only one object, just run infinite loops.
	 */

	if (size == 4) {
		longp = (uint*)addr;
		data = swap_ulong(*longp);
		printf("Address = 0x%08X Data = 0x%08X\n", addr, (data));
	}
	if (size == 2) {
		shortp = (ushort*)addr;
		data = swap_short(*shortp);
		printf("Address = 0x%08X Data = 0x%04X\n", addr, (data));
	}
	if (size == 1) {
		cp = (u_char*)addr;
		data = *cp;
		printf("Address = 0x%08X Data = 0x%02X\n", addr, data);
	}
	return 0;
}


int do_scb_write(cmd_tbl_t* cmdtp, int flag, int argc, char* argv[])
{
	ulong addr, data, offset, base;
	int size;
	volatile uint* longp;
	volatile ushort* shortp;
	volatile u_char* cp;

	if (argc < 3) {
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	/* Check for a size spefication.
	 * Defaults to long if no or incorrect specification.
	 */
	if ((size = cmd_get_data_size(argv[0], 4)) < 0) {
		return 1;
	}

	/* Address is always specified.
	 */
	offset = simple_strtoul(argv[1], NULL, 16);
	base   = ccb_get_regp();
	addr = offset + base;

	/* data to write */
	data = simple_strtoul(argv[2], NULL, 16);

	printf("Address = 0x%08X Data = 0x%08X \n", addr, data);

	if (size == 4) {
		longp = (uint*)addr;
		*longp = swap_ulong(data);
	}
	if (size == 2) {
		shortp = (ushort*)addr;
		*shortp = swap_short(data & 0xFFFF);
	}
	if (size == 1) {
		cp = (u_char*)addr;
		*cp = data;
	}
	return 0;
}


int do_scb_poll(cmd_tbl_t* cmdtp, int flag, int argc, char* argv[])
{
	ulong addr, mask, data, offset, base, delay, exp_data;
	int size;
	volatile uint* longp;
	volatile ushort* shortp;
	volatile u_char* cp;

	if (argc < 4) {
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	/* Check for a size specification.
	 * Defaults to long if no or incorrect specification.
	 */
	if ((size = cmd_get_data_size(argv[0], 4)) < 0) {
		return 1;
	}

	/* Address is always specified.
	 */
	offset = simple_strtoul(argv[1], NULL, 16);
	base   = ccb_get_regp();
	addr = offset + base;
	mask = simple_strtoul(argv[2], NULL, 16);
	exp_data = simple_strtoul(argv[3], NULL, 16);
	if (argc < 5) {
		delay = 10;
	} else {
		delay = simple_strtoul(argv[4], NULL, 16);
	}

	/* We want to optimize the loops to run as fast as possible.
	 * If we have only one object, just run infinite loops.
	 */
	if (size == 4) {
		longp = (uint*)addr;
		while (1) {
			data = swap_ulong(*longp);

			if (data & mask == exp_data & mask) {
				printf("Address = 0x%08X Data = 0x%08X\n", addr, (data));
				return 0;
			}
			udelay(delay);
			if (ctrlc()) {
				putc('\n');
				printf("Polling aborted ...\n");
				break;
			}
		}
	}

	if (size == 2) {
		shortp = (ushort*)addr;
		while (1) {
			data = swap_short(*shortp);

			if (data & mask == exp_data & mask) {
				printf("Address = 0x%08X Data = 0x%08X\n", addr, (data));
				return 0;
			}
			udelay(delay);
			if (ctrlc()) {
				putc('\n');
				printf("Polling aborted ...\n");
				break;
			}
		}
	}

	if (size == 1) {
		cp = (u_char*)addr;
		while (1) {
			data = *cp;

			if (data & mask == exp_data & mask) {
				printf("Address = 0x%08X Data = 0x%08X\n", addr, (data));
				return 0;
			}
			udelay(delay);
			if (ctrlc()) {
				putc('\n');
				printf("Polling aborted ...\n");
				break;
			}
		}
	}
	return 0;
}


#define CFG_MEMTEST_PATTERN    0x55AACC33
#define MEMTEST_DMA            0x01
#define MEMTEST_SW             0x02
int cmd_get_verbose(char* arg)
{
	/* Check for a .v
	 */
	int len = strlen(arg);

	if (len > 2 && arg[len - 2] == '.') {
		switch (arg[len - 1]) {
		case 'v':
			return 1;
		default:
			return 0;
		}
	}
	return 0;
}


int is_mem_block_valid(ulong addr, ulong size)
{
	return 1; 
}


int do_memtest(cmd_tbl_t* cmdtp, int flag, int argc, char* argv[])
{
	static ulong test_pattern = CFG_MEMTEST_PATTERN;
	static ulong memtest_stat_cline = 0;
	ulong start, dest, do_test, size, mem_valid, end, cline, dots_size, verbose;
	ulong* p, dma_manual_start;
	int i;

	do_test = 0;
	mem_valid = 0;
	verbose = 0;

	verbose = cmd_get_verbose(argv[0]);
	if (strcmp(argv[1], "pattern") == 0) {
		test_pattern =  simple_strtoul(argv[2], NULL, 16);
		printf("\nMem test data pattern changed --> 0x%08X\n", test_pattern);
	} else if (strcmp(argv[1], "dma") == 0) {
		start = simple_strtoul(argv[2], NULL, 16);
		dest = simple_strtoul(argv[3], NULL, 16);
		size = simple_strtoul(argv[4], NULL, 16);
		do_test = MEMTEST_DMA;
	} else if (strcmp(argv[1], "sw") == 0) {
		start = simple_strtoul(argv[2], NULL, 16);
		size = simple_strtoul(argv[3], NULL, 16);
		do_test = MEMTEST_SW;
	} else {
		printf("\nUsage\n");
	}

	if (do_test == MEMTEST_SW) {
		mem_valid = is_mem_block_valid(start, size);
		if (mem_valid) {
			dots_size = (size / 100);
			p = (ulong*)start;
			end = (start + size) & ~(0x03);
			for (cline = start; cline < end; cline += 4, p++) {
				if (((unsigned int)p & 0x1f) == 0) {
					ppcDcbz((unsigned long)p);
				}
				*p = (unsigned int)test_pattern;
				if (((unsigned int)p & 0x1c) == 0x1c) {
					ppcDcbf((unsigned long)p);
				}
				/* '.' every 'dots_size'*/
				if (verbose) {
					if (!((cline - start) % dots_size)) {
						printf(".");
					}
				}
				if (ctrlc()) {
					printf("\nMem SW test aborted\n");
					break;
				}
			}
			printf("\n");
			memtest_stat_cline += (cline / 4);
		}
	} else if (do_test == MEMTEST_DMA) {
		dma_manual_start = 0x2000;
		p = (ulong*)start;
		for (cline = start; cline < (start + size); cline += 4, p++) {
			if (((unsigned int)p & 0x1f) == 0) {
				ppcDcbz((unsigned long)p);
			}
			*p = (unsigned int)test_pattern;
			if (((unsigned int)p & 0x1c) == 0x1c) {
				ppcDcbf((unsigned long)p);
			}
		}
		dots_size = (size / 0x20000) / 10;
		dma_init();
		for (i = 0; i < (size / 0x100000); i++) {
			dma_xfer((uint*)(dest + (i * 0x100000)), 0x100000,
					 (uint*)(start + (i * 0x100000)));                                 /* 8K */
			if (verbose) {
				if (!(i % dots_size)) {
					printf(".");
				}
			}
			if (ctrlc()) {
				printf("\nMem DMA test aborted\n");
				break;
			}
		}
		printf("\n");
	}
	return 0;
}


#define FLASH_CMD_PROTECT_SET      0x01
#define FLASH_CMD_PROTECT_CLEAR    0xD0
#define FLASH_CMD_CLEAR_STATUS     0x50
#define FLASH_CMD_PROTECT          0x60

#define AMD_CMD_RESET              0xF0
#define AMD_CMD_WRITE              0xA0
#define AMD_CMD_ERASE_START        0x80
#define AMD_CMD_ERASE_SECTOR       0x30
#define AMD_CMD_UNLOCK_START       0xAA
#define AMD_CMD_UNLOCK_ACK         0x55

#define AMD_STATUS_TOGGLE          0x40

/* simple flash 16 bits write, do not accross sector */
int flash_write_direct(ulong addr, char* src, ulong cnt)
{
	volatile uint8_t* faddr = (uint8_t*)addr;
	volatile uint8_t* faddrs = (uint8_t*)addr;
	volatile uint8_t* faddr1 = (uint8_t*)((addr & 0xFFFFF000) | 0xAAA);
	volatile uint8_t* faddr2 = (uint8_t*)((addr & 0xFFFFF000) | 0x554);
	uint8_t* data = (uint8_t*)src;
	ulong start, saddr;
	int i;

	if ((addr >= 0xff800000) && (addr < 0xff80ffff)) {          /* 8k sector */
		saddr = addr & 0xffffe000;
	} else if ((addr >= 0xff810000) && (addr < 0xffefffff)) {   /* 64k sector */
		saddr = addr & 0xffff0000;
	} else if ((addr >= 0xfff00000) && (addr < 0xffffffff)) {   /* 8k sector */
		saddr = addr & 0xffffe000;
	} else {
		printf("flash address 0x%x out of range 0xFF800000 - 0xFFFFFFFF\n",
			   addr);
		return 1;
	}
	faddrs = (uint8_t*)saddr;
	faddr1 = (uint8_t*)((saddr & 0xFFFFF000) | 0xAAA);
	faddr2 = (uint8_t*)((saddr & 0xFFFFF000) | 0x554);

	/* un-protect sector */
	*faddrs = FLASH_CMD_CLEAR_STATUS;
	*faddrs = FLASH_CMD_PROTECT;
	*faddrs = FLASH_CMD_PROTECT_CLEAR;
	start = get_timer(0);
	while ((*faddrs & AMD_STATUS_TOGGLE) != (*faddrs & AMD_STATUS_TOGGLE)) {
		if (get_timer(start) > 0x2000) {
			*faddrs = AMD_CMD_RESET;
			return 1;
		}
		udelay(1);
	}

	/* erase sector */
	*faddr1 = AMD_CMD_UNLOCK_START;
	*faddr2 = AMD_CMD_UNLOCK_ACK;
	*faddr1 = AMD_CMD_ERASE_START;
	*faddr1 = AMD_CMD_UNLOCK_START;
	*faddr2 = AMD_CMD_UNLOCK_ACK;
	*faddrs = AMD_CMD_ERASE_SECTOR;
	start = get_timer(0);
	while ((*faddrs & AMD_STATUS_TOGGLE) != (*faddrs & AMD_STATUS_TOGGLE)) {
		if (get_timer(start) > 0x2000) {
			*faddrs = AMD_CMD_RESET;
			return 1;
		}
		udelay(1);
	}

	for (i = 0; i < cnt; i++) {
		/* write data byte */
		*faddr1 = AMD_CMD_UNLOCK_START;
		*faddr2 = AMD_CMD_UNLOCK_ACK;
		*faddr1 = AMD_CMD_WRITE;

		*(faddr + i) = data[i];
		start = get_timer(0);
		while ((*faddrs & AMD_STATUS_TOGGLE) !=
			   (*faddrs & AMD_STATUS_TOGGLE)) {
			if (get_timer(start) > 0x100) {
				*faddrs = AMD_CMD_RESET;
				return 1;
			}
			udelay(1);
		}
	}

	/* protect sector */
	*faddrs = FLASH_CMD_CLEAR_STATUS;
	*faddrs = FLASH_CMD_PROTECT;
	*faddrs = FLASH_CMD_PROTECT_SET;
	start = get_timer(0);
	while ((*faddrs & AMD_STATUS_TOGGLE) != (*faddrs & AMD_STATUS_TOGGLE)) {
		if (get_timer(start) > 0x2000) {
			*faddrs = AMD_CMD_RESET;
			return 1;
		}
		udelay(1);
	}

	return 0;
}


/* upgrade commands
 *
 * Syntax:
 *    upgrade get
 *    upgrade set <state>
 */
int do_upgrade(cmd_tbl_t* cmdtp, int flag, int argc, char* argv[])
{
	char cmd1;
	int state;
	volatile int* ustate = (int*)(CFG_FLASH_BASE + CFG_UPGRADE_BOOT_STATE_OFFSET);

	if (argc < 2) {
		goto usage;
	}

	cmd1 = argv[1][0];

	switch (cmd1) {
	case 'g':          /* get state */
	case 'G':
		printf("upgrade state = 0x%x\n", *ustate);
		break;

	case 's':                    /* set state */
	case 'S':
		state = simple_strtoul(argv[2], NULL, 16);
		if (state == *ustate) {
			return 1;
		}

		if (flash_write_direct(CFG_FLASH_BASE + CFG_UPGRADE_BOOT_STATE_OFFSET,
							   (char*)&state, sizeof(int)) != 0) {
			printf("upgrade set state 0x%x failed.\n", state);
		}
		return 0;
		break;

	default:
		printf("???");
		break;
	}

	return 1;
usage:
	printf("Usage:\n%s\n", cmdtp->usage);
	return 1;
}

int num_test;
int mac_set;
int phy_set;
int ext_set;
uint bd_id;

int 
do_diag (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	unsigned int i;
	unsigned int j;
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
		if (argc == 2) {
			puts(" Please specify the test name \n");
		}
		if (argc == 5 || argc == 7 || argc == 8 ) {
			if(!strcmp(argv[argc-4], "TSEC0")) {
				if(!strcmp(argv[argc-3], "MAC_LB")){
					num_test = simple_strtoul(argv[6], NULL, 10);
					mac_set = 1;
				}
				if(!strcmp(argv[argc-3], "PHY_LB")){
					num_test = simple_strtoul(argv[6], NULL, 10);
					phy_set = 1;
				}
				if(!strcmp(argv[argc-3], "10Mbs")){
					num_test = simple_strtoul(argv[6], NULL, 10);
					ext_set = 1;
				}
				if(!strcmp(argv[argc-3], "100Mbs")){
					num_test = simple_strtoul(argv[6], NULL, 10);
					ext_set = 2;
				}
				if(!strcmp(argv[argc-3], "PORT_LB")){
					num_test = simple_strtoul(argv[6], NULL, 10);
					ext_set = 3;
				}
				printf("The total number of test %d\n", num_test);
				for( j=0; j <= num_test; j++) {
					printf("The current test number : %d\n", j);
					for (i = 2; i < argc - 4; i++) {
						if (post_run (argv[i], POST_RAM | POST_MANUAL) != 0)
							printf ("%s - unable to execute the test\n", \
									argv[i]);
					}
				}
			}

			if(!strcmp(argv[argc-4], "TSEC2")) {
				if(!strcmp(argv[argc-3], "MAC_LB")) {
					mac_set = 2;
					num_test = simple_strtoul(argv[6], NULL, 10);
					printf("The total number of test %d\n", num_test);
					for( j=0; j <= num_test; j++) {
						printf("The current test number : %d\n",j);
						for (i = 2; i < argc - 4; i++) {
							if (post_run(argv[i], POST_RAM | POST_MANUAL) != 0)
								printf ("%s - unable to execute \
										the test\n", argv[i]);
						}
					}
				}
			}
			bd_id = simple_strtoul(argv[4], NULL, 16);
			num_test = simple_strtoul(argv[6], NULL, 10);
			printf("The total number of test %d\n", num_test);
			for( j=0; j <=num_test; j++) {
				printf("The current test number : %d\n", j);
				for (i = 2; i < argc - 5; i++) {
					if(!strcmp(argv[7], "-v")) {
						if (post_run (argv[i], POST_RAM | POST_MANUAL 
									| POST_DEBUG) != 0)
							printf ("%s - unable to execute \
									the test\n", argv[i]);
					}
					else {
						if (post_run(argv[i], POST_RAM | POST_MANUAL) != 0)
							printf ("%s - unable to execute \
									the test\n", argv[i]);
					}
				}
			}

			if(argc == 5) {
				num_test = simple_strtoul(argv[4], NULL, 10);
				if(!strcmp(argv[3], "test")) {
					printf("The total number of test %d\n", num_test);
					for( j=0; j <= num_test; j++) {
						printf("The current test number : %d\n", j);
						for (i = 2; i < argc - 2; i++) {
							if (post_run(argv[i], POST_RAM | POST_MANUAL) != 0)
								printf ("%s - unable to execute \
										the test\n", argv[i]);
						}
					}
				}
				puts(" Please specify the proper test command \n");
			}
		}
		else {
			if(!strcmp(argv[argc-1], "-v")) {
				if(argc == 3){
					puts(" Please specify the test name \n");
				}
				else {
					for (i = 2; i < argc - 1; i++) {
						if (post_run(argv[i], POST_RAM | POST_MANUAL 
									 | POST_DEBUG) != 0)
							printf ("%s - unable to execute \
									the test\n", argv[i]);
					}
				}
			}
			else {
				for (i = 2; i < argc; i++) {
					if (post_run(argv[i], POST_RAM | POST_MANUAL) != 0)
						printf ("%s - unable to execute \
								the test\n", argv[i]);
				}
			}
		}
	}

	return 0;
}

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
	memtest, 5, 1, do_memtest,
	"memtest - perform memory test for the specified blocks\n",
	"memtest sw <addr1> <size>\n"
	"memtest dma <addr1> <addr2> <size>\n"
	"memtest pattern <value>\n"
	"\n"
	);

U_BOOT_CMD(
	scbr,    2,    1,    do_scb_read,
	"scbr    - read scb registers\n",
	"[.b, .w, .l] address \n"
	"\n"
	);

U_BOOT_CMD(
	scbw,    3,    1,    do_scb_write,
	"scbw   - write to scb regsiters\n",
	"[.b, .w, .l] address  data\n"
	"\n"
	);

U_BOOT_CMD(
	scbp,    4,    1,    do_scb_poll,
	"scbp   - poll scb regsiters\n",
	"[.b, .w, .l] address poll_mask data [delay]\n"
	"default delay is 10us\n"
	);

U_BOOT_CMD(
	debugport,     2,      1, do_debugport,
	"debugport- utility for enabling mgmt ethernet port on standalone LCs\n",
	"\n"
	"debugport enable  \n"
	"debugport disable   \n"
	);


U_BOOT_CMD(
	mastership,     8,      1, do_scb_master,
	"mastership- utility for controlling the mastership\n",
	"\n"
	"mastership show  \n"
	"mastership get   \n"
	"mastership release   \n"
	);

U_BOOT_CMD(
	rtc,     8,      1,       do_rtc_1672,
	"rtc     - utility for tetsing  DS1672 rtc functionality\n",
	"\n"
	"rtc   show \n"
	"    - Display the current date and time every second\n"
	"rtc   enable \n"
	"    - Enables or starts the RTC\n"
	"rtc   disable \n"
	"    - Disables or stops the RTC\n"
	"rtc   date [month <value>][day <value>] [year <value>]\n"
	"    - Display the current date or set with modified date\n"
	"rtc   time [hour <value>] [minute <value>] [second <value>]\n"
	"    - Display the current time or set with modified time\n"
	);

U_BOOT_CMD(
	storage,    7,    1,     do_storage,
	"storage - storage device block utility\n",
	"\n"
	"storage read [usb | nor] <device> <block> <dest> <count>\n"
	"    - read # blocks from device block offset to dest memory location\n"
#if defined (USB_WRITE_READ)
	"storage write [usb | nor] <device> <block> <src> <size>\n"
	"    - write source memory to device block offset location\n"
#endif
	);


U_BOOT_CMD(
	i2cs,   8,  1,  do_i2ce,
	"i2cs    - I2C commands using external controller\n",
	"\n"
	"i2cs reset \n"
	"    - reset selected i2c controller \n"
	"i2cs sel <group>\n"
	"    - select the i2cs group \n"
	"i2cs probe <group>\n"
	"    -probe the i2ce devices connected to external i2c interface \n"
	"i2cs ew <group> <device> <address> <data> [hex | char]\n"
	"    - write data to device at i2c address\n"
	"i2cs er <group> <device> <address> <len>\n"
	"    - read data from device at i2c address\n"
	"i2cs ep <group> <device> <src_address> <dest_offset> <len>\n"
	"    - program eeprom from a bin file.\n"
	);


U_BOOT_CMD(
	i2c,    8,  1,  do_i2c,
	"i2c     - I2C commands using onchip controller\n",
	"\n"
	"i2c ctrl <ctrlr_id>\n"
	"    - select i2c controller. (valid ctrlr_id values 1..2, default 1)\n"
	"i2c reset \n"
	"    - reset selected i2c controller \n"
	"i2c probe <device> \n"
	"    - probe the specified device \n"
	"i2c sw set <device> <channel> \n"
	"    - Select the channel on i2c switch \n"
	"i2c sw get <device> \n"
	"    - Get the selected channel on i2c switch \n"
	"i2c ew <device> <address> <data> [hex | char]\n"
	"    - write data to device at i2c address\n"
	"i2c er <device> <address> <len>\n"
	"    - read data from device at i2c address\n"
	"i2c ep <device> <src_address> <dest_offset> <len>\n"
	"    - program eeprom from a bin file.\n"
	"i2c w <device> <data> [hex | char]\n"
	"    - write data to device \n"
	"i2c r <device> <len>\n"
	"    - read data from device \n"
	);
U_BOOT_CMD(
	pcie,   5,  1,  do_pcie,
	"pcie    - list and access PCIE Configuration Space\n",
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
	"pcie reset bridge\n"
	"    - Reset the pcie bridge\n"
	"pcie iread[.b, .w, .l] b.d.f [address] [# of objects]\n"
	"    - display PCIE memory space \n"
	"pcie iwrite[.b, .w, .l] b.d.f address value\n"
	"    - write to PCIE memory space \n"
	);

U_BOOT_CMD(
	pcieprobe,     2,      1,       do_pcie_probe,
	"pcieprobe  - List the PCI-E devices found on the bus\n",
	"pcieprobe [-v]/[-d](v - verbose d - dump)\n"
	"pcieprobe init - rescan and initialize pcie devices\n"
	);

U_BOOT_CMD(
	diag,   CFG_MAXARGS,    0,  do_diag,
	"diag    - perform board diagnostics\n",
	"    - print list of available tests\n"
	"diag [test1 [test2]]\n"
	"    - print information about specified tests\n"
	"diag run - run all available tests\n"
	"diag run [test1 [test2]]\n"
	"    - run specified tests\n"
	);
#endif /* (CONFIG_COMMANDS & CFG_CMD_BSP) */
