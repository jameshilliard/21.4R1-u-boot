/*
 * Copyright (c) 2007-2010, Juniper Networks, Inc.
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
#include <post.h>

#if (CONFIG_COMMANDS & CFG_CMD_BSP)
#include <asm/processor.h>
#include <asm/immap_85xx.h>
#include <i2c.h>
#include <asm/io.h>
#include "pcie.h"
#include "fsl_i2c.h"
#include <net.h>
#include <usb.h>
#include <config.h>
#include <common.h>
#include <part.h>
#include <fat.h>
#include "recb.h"
#include "recb_iic.h"
#include <ns16550.h>
#include <serial.h>
#include <asm/types.h>
#include <asm/fsl_i2c.h>
#define I2C ((fsl_i2c_t *)mpc_i2c)
#define LOCAL_RE 0x18
#define BYTE_LENGTH 4

extern block_dev_desc_t usb_dev_desc[USB_MAX_STOR_DEV];
extern block_dev_desc_t nor_dev;

/* Prototypes */
void lc_cpld_dump(void);
int do_lc_cpld(cmd_tbl_t *, int , int , char *[]);
void pcieinfo(int , int );
void pciedump(int );
static char *pcie_classes_str(unsigned char );
void pcie_header_show_brief(pcie_dev_t );
void pcie_header_show(pcie_dev_t );
int do_pcie_probe(cmd_tbl_t *, int , int , char *[]);

DECLARE_GLOBAL_DATA_PTR;

static int rosebud_access_max_count;

extern int cmd_get_data_size(char* arg, int default_size);
extern block_dev_desc_t * sata_get_dev(int dev);

static int atoh(char* string)
{
	int res = 0;

	while (*string != 0) {
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


void i2c_reset(void)
{
	volatile immap_t *immap = (immap_t *) CFG_CCSRBAR;
	volatile ccsr_gur_t *gur = &immap->im_gur;
	  
	gur->gpoutdr &= ~0x1000000;
	udelay(1000);
	gur->gpoutdr |= 0x1000000;
	udelay(1000);
}

void i2c_fdr (uint8_t clock, int slaveadd)
{
	/* set clock */
	writeb(clock, &I2C->fdr);
	udelay(100000);
}

/* Show information about devices on PCIe bus. */

void pcieinfo (int busnum, int short_listing)
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
	unsigned char buff[256],temp[20];
	int addr,offset = 0;
	int linebytes;
	int nbytes = 256;
	int i,j=0;

	printf("\nScanning PCIE devices on bus %d\n", busnum);

	dev = PCIE_BDF(busnum, 0, 0);
	memset(buff,0xff,256);

	for (addr = 0 ; addr<265 ; addr++) 
	pcie_read_config_byte(dev, addr,(buff+addr));

	do {
		printf("0x%02x :",offset);
		linebytes = (nbytes < 16)?nbytes:16;

		for(i=0 ;i<linebytes ;i++) {
			if((i==4) || (i==8)|| (i == 12))
				printf(" ");

			temp[i] = buff[j];
			printf(" %02x",buff[j]);
			j++;
		}
		printf("  ");

		for(i=0 ;i<linebytes ;i++) {
			if((i==4) || (i==8)|| (i == 12))
				printf(" ");
		
			if((temp[i] < 0x20) || (temp[i] > 0x7e)) 
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
	return  "???";
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

	switch (header_type & 0x03) {
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

/* This routine services the "pcieprobe" command.
 * Syntax - pcieprobe [-v] [-d]
 *            -v for verbose, -d for dump.
 */

int do_pcie_probe(cmd_tbl_t *cmd, int flag, int argc, char *argv[])
{
	if(argc == 2) {
		if(strcmp(argv[1],"-v") == 0) {
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

int do_i2c(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char cmd1, cmd2;
	ulong controller = 0, mux = 0, device = 0, channel = 0, value = 0, size = 0;
	unsigned char ch, bvalue;
	uint regnum = 0;
	int i, ret, reg = 0, temp, len = 0, offset = 0;
	uint8_t fdr;
	char tbyte[3];
	uint8_t tdata[128];
	char *data;
	uint16_t cksum = 0;

	if (argc < 2)
		goto usage;
	
	cmd1 = argv[1][0];
	cmd2 = argv[2][0];
	
	switch (cmd1) {
	case 'b':	/* byte read/write */
	case 'B':
		device = simple_strtoul(argv[2], NULL, 16);
		if ((argv[1][1] == 'r') ||(argv[1][1] == 'R')) {
			 bvalue = 0xff;
			if ((ret = i2c_read(device, 0, 0, &bvalue, 1)) != 0) {
				printf("i2c failed to read from device 0x%x.\n", device);
				return 0;
			}
			printf("i2c read addr=0x%x value=0x%x\n", device, bvalue);
		} else {
			if (argc <= 3)
				goto usage;
				
			bvalue = simple_strtoul(argv[3], NULL, 16);
			if ((ret = i2c_write(device, 0, 0, &bvalue, 1)) != 0) {
				printf("i2c failed to write to address 0x%x value 0x%x.\n", device, bvalue);
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
				if ((ret = i2c_write(mux, 0, 0, &ch, 1)) != 0) {
					printf("i2c failed to enable mux 0x%x channel %d.\n", mux, channel);
				} else
					printf("i2c enabled mux 0x%x channel %d.\n", mux, channel);
			} else {
				ch = 0;
				if ((ret = i2c_write(mux, 0, 0, &ch, 1)) != 0) {
					printf("i2c failed to disable mux 0x%x channel %d.\n", mux, channel);
				} else
					printf("i2c disabled mux 0x%x channel %d.\n", mux, channel);
			}
		}
		break;
			
	case 'd':	/* Data */
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
				if ((ret = i2c_read(device, regnum, reg, tdata, len)) != 0) {
					printf ("I2C read from device 0x%x failed.\n", device);
				} else {
					printf ("The data read - \n");
					for (i=0; i< len; i++)
						printf("%02x ", tdata[i]);
					printf("\n");
				}
			}
		} else {  /* write */
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
		 	if ((ret = i2c_write(device, regnum, reg, tdata, len)) != 0)
				printf ("I2C write to device 0x%x failed.\n", device);
		}
						
		break;
			
	case 'e':	/* EEPROM */
	case 'E':
		if (argc <= 4)
			goto usage;
			
		device = simple_strtoul(argv[2], NULL, 16);
		offset = simple_strtoul(argv[3], NULL, 10);
		if ((argv[1][1] == 'r') ||(argv[1][1] == 'R')) {  /* read */
			len = simple_strtoul(argv[4], NULL, 10);
			if (len) {   
				if ((ret = eeprom_read(device, offset, tdata, len)) != 0) {
					printf ("I2C read from EEPROM device 0x%x failed.\n", device);
				} else {
					printf ("The data read from offset - \n", offset);
					for (i=0; i< len; i++)
						printf("%02x ", tdata[i]);
					printf("\n");
				}
			}
		} else {  /* write */
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
			} else {
				len = strlen(data);
				for (i=0; i<len; i++) {
					tdata[i] = data[i];
			  	}
			}
			if ((ret = eeprom_write(device, offset, tdata, len)) != 0)
				printf ("I2C write to EEROM device 0x%x failed.\n", device);
		}
					
		break;
			
	case 'w':	/* write */
	case 'W':
		if (argc <= 6)
			goto usage;
	
		reg = 0;
		regnum = 0;		
		value = simple_strtoul(argv[6], NULL, 16);
		if (argc > 6) {
			reg = 1; 
			regnum = simple_strtoul(argv[7], NULL, 16);
		}	
	
	case 'r':	       /* read/reset */
	case 'R':
		if (((argv[1][1] == 'e') ||(argv[1][1] == 'E')) &&
			((argv[1][2] == 's') ||(argv[1][2] == 'S'))) {
			i2c_reset();
			break;
		}
			
		if (argc <= 5)
			goto usage;
		if (((cmd1 == 'r') || (cmd1 == 'R')) && (argc > 5)) {
			reg = 1;
			regnum = simple_strtoul(argv[6], NULL, 16);
		}
			
		controller = simple_strtoul(argv[2], NULL, 16);
		mux = simple_strtoul(argv[3], NULL, 16);
		device = simple_strtoul(argv[4], NULL, 16);
		channel = simple_strtoul(argv[5], NULL, 16);
		if (channel >= 8) {
			printf("channel %d too big.\n", channel);
			break;
		}
		ch = 1 << channel;
			
		size = cmd_get_data_size(argv[1], 4);

		i2c_controller(controller);
		if ((ret = i2c_write(mux, 0, 0, &ch, 1)) != 0) {
			printf("i2c failed to enable mux 0x%x channel 0x%x.\n", mux, channel);
			goto exit_rw;
		}

		if ((cmd1 == 'w') || (cmd1 == 'W')) {
			value = value << ((4-size) * 8);
			if ((ret = i2c_write(device, regnum, reg, (uint8_t *)&value, size)) != 0) {
				printf("i2c failed to write to device 0x%x.\n", device);
				goto exit_rw;
			} 
		} else {
			if ((ret = i2c_read(device, regnum, reg, (uint8_t *)&value, size)) != 0) {
				printf("i2c failed to read from device 0x%x channel 0x%x.\n", device, ch);
				goto exit_rw;
			}
			printf("i2c read = 0x%x.\n", value);
		}
			
		ch = 0;
		if ((ret = i2c_write(mux, 0, 0, &ch, 1)) != 0) {
			printf("i2c failed to disable mux 0x%x channel 0x%x.\n", mux, channel);
			goto exit_rw;
		}
exit_rw:
		i2c_controller(1);
			break;

	case 'f':	       /* I2C FDR */
	case 'F':
		if (argc <= 3)
			goto usage;
		
		controller = simple_strtoul(argv[2], NULL, 16);
		fdr = simple_strtoul(argv[3], NULL, 16);
	 	i2c_controller(controller);
		i2c_fdr(fdr, CFG_I2C_SLAVE);
		printf("i2c[%d] fdr = 0x%x.\n", controller, fdr);
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
		for (i=0; i<linebytes; i+= size) {
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

static int pcie_cfg_write (pcie_dev_t bdf, ulong addr, ulong size, ulong value)
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

static int
pcie_cfg_modify (pcie_dev_t bdf, ulong addr, ulong size, ulong value, int incrflag)
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
int do_pcie (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
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
	printf ("Usage:\n%s\n", cmdtp->usage);
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

U_BOOT_CMD(
	i2c,	8,	1,	do_i2c,
	"i2c     - I2C test commands\n",
	"\n"
	"i2c reset\n"
	"    - reset i2c mux and I/O expander\n"
	"i2c read[.b, .w, .l] <controller> <mux> <device> <channel> [<reg>]\n"
	"    - read from i2c address\n"
	"i2c write[.b, .w, .l] <controller> <mux> <device> <channel> <value> [<reg>]\n"
	"    - write to i2c address\n"
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
	"i2c fdr <controller> <hex value>\n"
	"    - set fdr for i2c controller\n"
	"i2c mux [enable|disable] <mux addr> <channel>\n"
	"    - enable/disable mux channel\n"
	"I2C LED set <led> <color> <state>\n"
	"    - I2C set [fan | uplink | poe | power | rps | stack | system] [off |amber | green] [steady | slow | fast]\n"
	"I2C LED update\n"
	"    - I2C update LED\n"
	"I2C network <mode>\n"
	"    - I2C network [1G | 10G]\n"
);


#ifdef USB_WRITE_READ
extern unsigned long usb_stor_write(unsigned long device,unsigned long blknr, 
					   unsigned long blkcnt, unsigned long *buffer);

#endif


#if (CONFIG_COMMANDS & CFG_CMD_FAT)
extern int fat_register_device(block_dev_desc_t *dev_desc, int part_no);

block_dev_desc_t nor_dev;

block_dev_desc_t * nor_get_dev(int dev)
{
	return ((block_dev_desc_t *)&nor_dev);
}

ulong nor_bread(int dev_num, ulong blknr, ulong blkcnt, ulong *dst)
{
	int nor_block_size = FS_BLOCK_SIZE;
	ulong src = blknr * nor_block_size + CFG_FLASH_BASE;

	memcpy((void *)dst, (void *)src, blkcnt*nor_block_size);
	return blkcnt;
}

int nor_init(int verbose)
{

	/* fill in device description */
	nor_dev.if_type = IF_TYPE_NOR;
	nor_dev.part_type = PART_TYPE_DOS;
	nor_dev.dev = 0;
	nor_dev.lun = 0;
	nor_dev.type = 0;
	nor_dev.blksz = 512;
	nor_dev.lba = 12288;  /* 12288 * 512 = 6MByte */
	sprintf(nor_dev.vendor,"Man %s", "Spansion");
	sprintf(nor_dev.product,"%s","NOR Flash");
	sprintf(nor_dev.revision,"%x %x",0, 0);
	nor_dev.removable = 0;
	nor_dev.block_read = nor_bread;

	if (fat_register_device(&nor_dev,1)) { /* partitions start counting with 1 */
		if (verbose)
		printf ("** Failed to register NOR FAT device %d **\n", nor_dev.dev);
	}

	return 1;
}
#endif

/* storage device commands
 *
 * Syntax:
 */
int do_storage (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
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

	if ((cmd2 == 's') || (cmd2 == 'S'))
	{
		stor_dev = sata_get_dev(device);
		if (stor_dev->type == DEV_TYPE_UNKNOWN) {
			stor_dev = NULL;
		}
	} else {
		if ((cmd2 == 'n') || (cmd2 == 'N'))
			stor_dev = &nor_dev;
		else {
			if ((device >= 0) && (device < 5))
				stor_dev = &usb_dev_desc[device];;
		}
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


U_BOOT_CMD(
	storage,    7,    1,     do_storage,
	"storage - storage device block utility\n",
	"\n"
	"storage read [usb | nor | sata] <device> <block> <dest> <count>\n"
	"    - read # blocks from device block offset to dest memory location\n"
#if defined(USB_WRITE_READ)
	"storage write [usb | nor] <device> <block> <src> <size>\n"
	"    - write source memory to device block offset location\n"
#endif
);



/* 
 * add commands
 * 1. sfr <addr> 
 * 2. sfd <addr>  dispaly 64 values 
 * 3. sfw <addr> <value>
 * 4. reset_rb  reset rosebud 
 * 5. reset_sf  reset sf
 */
static uint32_t rb_base_p = 0xe8300000;

#define RB_BASE         (rb_base_p)
#define RB_REG_RESET    (RB_BASE + 0x4)
#define RB_REG_TEST55   (RB_BASE + 0x158)
#define RB_REG_SF_RESET (RB_BASE + 0x178)
#define RB_REG_CMD      (RB_BASE + 0x1000)
#define RB_REG_ADDR     (RB_BASE + 0x1004)
#define RB_REG_DIN      (RB_BASE + 0x1008)
#define RB_REG_DOUT     (RB_BASE + 0x100c)
#define RB_REG_STAT     (RB_BASE + 0x1014)

#define ENDIAN_SWAP_4_BYTE(_i) le32_to_cpu(_i)

#define   ROSEBUD_SF_JCMD_RDWR              (0x1)
#define   ROSEBUD_SF_JSTATUS_OPERDONE       (0x1)
#define   ROSEBUD_SF_JSTATUS_READ_SUCCESS   (0x31)
#define   ROSEBUD_SF_JSTATUS_WRITE_SUCCESS  (0x29)

#define msleep(a) udelay(a * 1000)
#define ssleep(a) msleep(a * 1000)

/*
 * rosebud_read_version
 * read FPGA version.
 */
static int
rosebud_reg_access_test (u_int32_t address, u_int32_t value, u_int32_t *read_val, int op)
{
	if((op == 0) || (op > 1)) {
		writel(value, address);
	}

	if((op == 1) || (op > 1)) {
		*read_val = readl(address);
	}

	if(*read_val == value)
		return(0);
	else
		return(1);
}

/*
 * jbus_wait_for_done
 * Poll for the "transaction in progress" bit to be clear.
 * Return 1 on success.
 */

static int
jbus_wait_for_done (int write, uint32_t *status)
{
	int i;

	for(i = 0; i < 4096; i++) { /* software timeout 4096 */
		*status = readl(RB_REG_STAT);
		if(rosebud_access_max_count < i)
			rosebud_access_max_count = i;
		if (*status & ROSEBUD_SF_JSTATUS_OPERDONE)
			break;
	}

	if(i >= 4096) {
		printf("Error: Software time out\n");
		return 0;
	}

	if(write) {
		if(*status != ROSEBUD_SF_JSTATUS_WRITE_SUCCESS) {
			return 0;
		}
	} else {
		if(*status != ROSEBUD_SF_JSTATUS_READ_SUCCESS) {
		return 0;
		}
	}
   return(1);
}


/*
 * rosebud_jbus_write_word
 * write SF registers through Jbus.
 * return 0 success
 */
static int
rosebud_jbus_write_word (u_int32_t offset, 
						 u_int32_t data,
						 u_int32_t *status)
{
	int rc;

	writel(ROSEBUD_SF_JCMD_RDWR, RB_REG_CMD);
	writel(offset, RB_REG_ADDR);
	writel(data, RB_REG_DIN);

	if(jbus_wait_for_done(1, status))
		rc = 0;
	else 
		rc = 1;

	return(rc);
}

/*
 * rosebud_jbus_read_word
 * read SF registers through Jbus.
 * return 0 success
 */
static int
rosebud_jbus_read_word ( u_int32_t offset, 
						 u_int32_t *data_p,
						 u_int32_t *status )
{
	int rc;

	writel(0, RB_REG_CMD);
	writel(offset, RB_REG_ADDR);

	if (jbus_wait_for_done(0, status)) {
		/* read the actual data */
		*data_p = readl(RB_REG_DOUT);
		rc = 0;
	} else {
		*data_p = readl(RB_REG_DOUT);
		rc = 1;
	}

	return(rc);
}
/* storage device commands
 *
 * Syntax:
 */
int do_switchfabric (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	uint32_t addr;
	uint32_t value;
	uint32_t value1;
	uint32_t count, delay;
	uint32_t count1;
	int i;
	uint32_t status;
 
	switch (argc) {
	case 0:
	case 1:
	case 2:
			printf ("Usage:\n%s\n", cmdtp->usage);
			return (1);
	case 3:
		if (strcmp(argv[1],"reset") == 0) {
			printf("reset ");
			addr = (uint32_t) (simple_strtoul(argv[2], NULL, 16) & ~3);
			printf("base address : 0x%08x \n", addr);
			rb_base_p = addr;
			writel(0xffffffff, RB_REG_RESET);
			msleep(5);
			writel(0x0, RB_REG_RESET);
			msleep(5);
			writel(0xffffffff, RB_REG_SF_RESET);
			msleep(5);
			writel(0x0, RB_REG_SF_RESET);
			rosebud_access_max_count = 0;
			return(0);
		}

		if (strcmp(argv[1],"read") == 0) {
			printf("read ");
			addr = (uint32_t) (simple_strtoul(argv[2], NULL, 16) & ~3);
			printf("0x%08x ", addr);
			if (rosebud_jbus_read_word(addr, &value, &status)) {
				printf("= 0x%08x \n", value);
				printf("error status = 0x%08x\n", status);
			} else {
				printf("= 0x%08x \n", value);
			}
			return (0);
		}
			
		if (strcmp(argv[1],"show") == 0) {
			if (strcmp(argv[2],"count") == 0) {
				printf("Max jbus status polling count = %d\n", rosebud_access_max_count);
				rosebud_access_max_count = 0;
			}
			return(0);
		}
		
		printf ("Usage:\n%s\n", cmdtp->usage);
		return (1);
	default:
		/* at least 4 args */
		if (strcmp(argv[1],"write") == 0) {
			printf("write ");
			addr = (uint32_t) (simple_strtoul(argv[2], NULL, 16)  & ~3);
			value = (uint32_t) simple_strtoul(argv[3], NULL, 16);
			printf("0x%08x = 0x%08x ", addr, value);
			if (rosebud_jbus_write_word(addr, value, &status)) {
				printf("error status = 0x%08x\n", status);
			} else {
				printf("okay \n");
			}
			return (0);
		} else if (strcmp(argv[1],"dump") == 0){
			printf("dump \n");
			addr = (uint32_t) (simple_strtoul(argv[2], NULL, 16) & ~3);
			count = (uint32_t) simple_strtoul(argv[3], NULL, 16);
			for (i = 0; i < count; i++) {
				if (rosebud_jbus_read_word(addr, &value, &status)) {
					printf("0x%08x = 0x%08x \n", addr, value);
					printf("0x%08x read error status = 0x%08x\n", addr, status);
				} else {
					printf("0x%08x = 0x%08x \n", addr, value);
				}
				addr += 4;
			}
			return (0);
		} else if (strcmp(argv[1],"tread") == 0) {
			printf("test read ");
			addr = (uint32_t) (simple_strtoul(argv[2], NULL, 16) & ~3);
			count = (uint32_t) simple_strtoul(argv[3], NULL, 16);
			value = readl(addr);
			count1 = 0;
			for(i=0; i<count; i++) {
				value1 = readl(addr);
				if(value1 != value) {
					printf("new value = 0x%08x\n", value1);
					count1++;
				}
			}
			printf("0x%08x = 0x%08x %d times, %d times mis-match\n", addr, value, count+1, count1);
			return (0);
		} else if (strcmp(argv[1],"twrite") == 0) {
			printf("test write ");
			addr = (uint32_t) (simple_strtoul(argv[2], NULL, 16)  & ~3);
			value = (uint32_t) simple_strtoul(argv[3], NULL, 16);
			count = (uint32_t) simple_strtoul(argv[4], NULL, 16);
			for(i=0; i<count; i++) {
				 if(rosebud_reg_access_test(addr, value, &value1, 0));
			}
			printf("0x%08x = 0x%08x %d times\n", addr, value, count);
			return (0);
		} else if (strcmp(argv[1],"test") == 0) {
			printf("test access ");
			addr = (uint32_t) (simple_strtoul(argv[2], NULL, 16)  & ~3);
			value = (uint32_t) simple_strtoul(argv[3], NULL, 16);
			count = (uint32_t) simple_strtoul(argv[4], NULL, 16);
			delay = (count & 0xffff0000) >> 16;
			count = count & 0xffff;
			printf("write/read 0x%08x = 0x%08x %d times, delay = %d us\n", addr, value, count, delay);
			count1 = 0;
			for(i=0; i<count; i++) {
				writel(value, addr);
				udelay(delay);
				value1 = readl(addr);
				if(value1 != value)
					count1++;
			}
			printf("last read 0x%08x = 0x%08x\n", addr, value1);
			printf("%d of %d read/write value mis-match\n", count1, count);
			return (0);
		}			
	}

	printf ("Usage:\n%s\n", cmdtp->usage);
	return (1);
}

U_BOOT_CMD(
	fabric,    5,    1,     do_switchfabric,
	"fabric - switch fabric device block utility\n",
	"\n"
	"fabric read | dump | write \n"
	"   - read addr\n"
	"   - dump addr count\n"
	"   - write addr value\n"
	"   - reset base_addr\n"
);

int i2c_cksum = 0;

int do_i2c_controller ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int controller = 1;  /* default I2C controller 1 */
	   
	if (argc > 1) {
		/* Set new base address.
		*/
		controller = simple_strtoul(argv[1], NULL, 10);
	} else {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
	i2c_controller(controller);

	return (0);
}

int
do_i2c_diag (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int  controller  = 0;
	int  rw          = 0;
	int  len         = 0;
	int  i           = 0;
	int  temp        = 0;
	int  slave_addr  = 0;
	int  reg         = 0;
	int  ret         = 0;
	int  noreg       = 1;
	char *data;
	char tbyte[3];
	uint8_t tdata[128];

	memset(tdata, 0, sizeof(tdata));
	/* cmd arguments */
	rw = simple_strtoul(argv[1], NULL, 10);
	controller = simple_strtoul(argv[2], NULL, 10);
	slave_addr = simple_strtoul(argv[3], NULL, 16);

	/* check for valied i2c controller only 1 and 2 are allowed */
	if (controller == 1 || controller == 2 ) {
		/* set the ctrl */
		i2c_controller(controller);
		if (rw == 1) {
			len = simple_strtoul(argv[5], NULL, 10);
		} else {
			data = argv[5];
			len = simple_strtoul(argv[6], NULL, 10);
		}
		reg = simple_strtoul(argv[4], NULL, 16);
		if (reg == 0xFF) {
			noreg = 0;
		}
		if (rw == 1) {
			if (controller == 2) {
				for ( i = 0; i < len;  i++) {
					ret = i2c_read(slave_addr, reg, noreg,
							&tdata[i], 1);
					reg++;
				}
			} else {
				ret = i2c_read(slave_addr, reg, noreg,
						tdata, len);
			}
			if (ret == 0) {
				printf ("The data read - \n");
				for (i = 0; i < len; i++)
					printf("%02x ", tdata[i]);
				printf("\n");
			} else {
				printf ("Read to the i2c device failed !\n");
			}
		} else {
			tbyte[2] = 0;
			for (i = 0; i < len; i++) {
				tbyte[0] = data[2 * i];
				tbyte[1] = data[2 * i + 1];
				temp = atoh(tbyte);
				tdata[i] = temp;
			}
			if (controller == 1) {
				ret = i2c_write(slave_addr, reg, noreg,
						tdata, len);
			} else {
				for ( i = 0; i < len;  i++) {
					ret = i2c_write(slave_addr, (uchar) (reg & 0x3f),
							noreg, &tdata[i], 1);
					reg++;
				}
			}
			if (ret == 0) {
				printf("Write to the i2c device sucess !\n");
			} else {
				printf("Write to the i2c device failed !\n");
			}
		}
	} else {
		printf(" Input controler 1 or 2\n");
	}
	controller = 1; /* set back to default */
	i2c_controller(controller);
	return 1;
}


int do_i2c_diag_switch ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
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

	i2c_controller(controller);

	/* enable the channel */
	ch = 1 << channel;
	i2c_write(sw_addr, 0, 0, &ch, 1); 

	if (rw == 1) {
		ret = i2c_read(slave_addr, reg, noreg, tdata, len);
		if (ret == 0)
			printf ("The data read - \n");
		for (i=0; i< len; i++)
			printf("%02x ", tdata[i]);
		printf("\n");
	} else {
		tbyte[2] = 0;
		for (i=0; i<len; i++) {
			tbyte[0] = data[2*i];
			tbyte[1] = data[2*i+1];
			temp = atoh(tbyte);
			tdata[i] = temp;
		}
		if (i2c_cksum) {
			for (i=0; i<len; i++) {
				cksum += tdata[i];
			}
			tdata[len] = (cksum & 0xff00) >> 8;
			tdata[len+1] = cksum & 0xff;
			len = len + 2;
		}
		ret = i2c_write(slave_addr, reg, noreg, tdata, len);
	}

	/* disable the channel */
	ch=0x0;
	i2c_write(sw_addr, 0, 0, &ch, 1);

	controller = 1; /* set back to default */
	i2c_controller(controller);
	if (ret != 0) {
		printf ("i2c r/w failed\n");
		return 1;
	}
	return (0);
}

int do_i2c_cksum ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
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


int do_i2ce(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]) {
    char cmd1, cmd2;
    ulong group = 0, device = 0;
    int i, ret, temp, len = 0, offset = 0;
    char tbyte[3];
    uint8_t tdata[128];
    char *data;
    u8 *a = (u8*)&offset;
    int alen = 1;

    if (argc < 2)
        goto usage;

    cmd1 = argv[1][0];
    cmd2 = argv[2][0];

    switch (cmd1) {
        case 'e':   /* EEPROM */
        case 'E':
            if (argc < 6)
                goto usage;

            group  = simple_strtoul(argv[2], NULL, 10) & 0xFF;
            device = simple_strtoul(argv[3], NULL, 10) & 0x7F;
            offset = simple_strtoul(argv[4], NULL, 10);

            if (offset > 0xFF) {
                alen =2;
            }
            if (offset > 0xFFFF) {
                alen =3;
            }
            if (offset > 0xFFFFFF) {
                alen =4;
            }

            printf(" Grp = %x, device = %x offset = %x alen= %x \n",group,device,offset,alen) ;

            if ((argv[1][1] == 'r') ||(argv[1][1] == 'R')) {  /* read */

                len = simple_strtoul(argv[5], NULL, 10);
                if (len) {
                    if (device == RECB_SLAVE_CPLD_ADDR) {
                        /* Need parity generation*/
                        offset = recb_i2cs_set_odd_parity((u_int8_t)(offset&0xFF)); /* add parity to offset */
                    }

                    if ((ret = recb_iic_write ((u_int8_t)group, (u_int8_t) device, alen, &a[4 - alen])) != 0) {
                        printf ("I2C read from device 0x%x failed.\n", device);
                    } else {
                        if ((ret = recb_iic_read ((u_int8_t)group, (u_int8_t) device, len, tdata)) != 0) {
                            printf ("I2C read from device 0x%x failed.\n", device);
                        } else {
                            printf ("The data read from offset - \n", offset);
                            for (i=0; i< len; i++)
                                printf("%02x ", tdata[i]);

                            printf("\n");
                        }
                    }
                }
            } else {  /* write */
                for (i=0; i<alen; i++) {
                    tdata[i] = a[4 - alen + i];
                }

                data = argv [5];

                if ((argv[6][0] == 'h') || (argv[6][0] == 'H')) {

                    /* to handle if the data is provided as 0xAABBCCD*/
                    if ((argv[5][0] == '0') && ((argv[5][1] == 'x') || (argv[5][1] == 'X'))) {
                        data+=2;
                    }

                    len = strlen(data)/2;

                    tbyte[2] = 0;
                    for (i=0; i<len; i++) {
                        tbyte[0] = data[2*i];
                        tbyte[1] = data[2*i+1];
                        temp = atoh(tbyte);
                        tdata[i+alen] = temp;
                    }

                } else {
                    len = strlen(data);
                    for (i=0; i<len ; i++) {
                        tdata[i+alen] = data[i];
                    }
                }

                if (device == RECB_SLAVE_CPLD_ADDR) {
                    /* Need parity generation*/
                    tdata[0] = recb_i2cs_set_odd_parity(tdata[0]); /* add parity to offset */
                    tdata[1] = recb_i2cs_set_odd_parity(tdata[1]); /* add parity to data   */
                    /* assume len=1 and alen =1 for slave cpld access*/
                    if (len !=1 || alen != 1) {
                        goto usage;
                    }
                }

                if ((ret = recb_iic_write ((u_int8_t)group, (u_int8_t) device, len+alen, tdata))) {
                    printf ("I2C write to EEROM device 0x%x failed.\n", device);
                }
            }
            break;

        case 'r':          /* read/reset */
        case 'R':
            if (((argv[1][1] == 'e') ||(argv[1][1] == 'E')) &&
                ((argv[1][2] == 's') ||(argv[1][2] == 'S'))) {
                recb_iic_init();
                break;
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

int
do_i2c_reg (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	if (argc < 2)
		goto usage;
	if (strcmp(argv[1], "show") == 0) {
		printf("I2C slave address register                  0x%x\n",
				I2C->adr);
		printf("I2C frequency divider register              0x%x\n",
				I2C->fdr);
		printf("I2C control redister                        0x%x\n",
				I2C->cr);
		printf("I2C status register                         0x%x\n",
				I2C->sr);
		printf("I2C data register                           0x%x\n",
				I2C->dr);
		printf("I2C digital filter sampling rate register   0x%x\n",
				I2C->dfsrr);
	} else {
		puts ("Not a valied command !\n");
	}
	return 1;
usage:
	printf ("Usage:\n%s\n", cmdtp->usage);
	return 1;
}

#define   fpga_offsetof(type, field)	((size_t)(&((type *)0)->field))
#define FPGA_BASE   CFG_PARKES_BASE

struct fpga_reg_test_name_item {
	u_int32_t offset;
	char *description;
};

static struct fpga_reg_test_name_item fpga_reg_test_name_tbl[] = {
	{0x0001, "FPGA I2C CORE"},
	{0x0002, "FPGA UART CORE - 0"},
	{0x0004, "FPGA UART CORE - 1"},
	{0x0008, "Can be verified by running the test - diag run pcie"},
	{0x0010, "verfiy: SATA TO PATA CHIP"},
	{0x0020, "Can be verified by running the test - diag run ethernet"},
	{0x0040, "Can be verified by running the test - pci"},
	{0x0080, "Can be verified by running the test - pci"},
	{0x0100, "Can be verified by running the test - i2cs command"},
	{0x0200, "Can be verified by running the test - diag run pcie"},
	{0x0400, "HRESET - Reset the board"},
	{0x0800, "SRESET - Reset the board"},
	{0x1000, "Fabio device"}
};

int num_fpga_reg_tbl = 0;
int reg_indx = 0;
struct fpga_reg_name_item {
	u_int32_t offset;
	char *description;
};
static struct fpga_reg_name_item fpga_reg_name_tbl[] = {
	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			recb_version), "Fpga version" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			recb_scratch_pad), "Fpga scratch pad" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			recb_board_status), "Fpga board status" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			recb_re_reset_enable), "Fpga re reset enable" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			recb_re_reset_history), "Fpga re reset history" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			recb_re_reset_controller), "Fpga re reset controller" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			recb_control_register), "Fpga control register" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			recb_timer_cnt), "Fpga timer cnt" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			recb_wdtimer), "Fpga wdtimer" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			recb_cpp_bootram_write_enable), "Fpga cpp bootram write enable" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			recb_card_presence), "Fpga card presence" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			recb_irq4_status), "Fpga irq4 status" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			recb_irq4_en), "Fpga irq4 en" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			recb_irq7_status), "Fpga irq7 status" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			recb_irq7_en), "Fpga irq7 en" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			recb_irq8_status), "Fpga irq8 status" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			recb_irq8_en), "Fpga irq8 en" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			recb_irq10_status), "Fpga irq10 status" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			recb_irq10_en), "Fpga irq10 en" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			recb_keep_alive_fail_timer), "Fpga keep alive fail timer" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			recb_fast_alive_fail_timer), "Fpga fast alive fail timer" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			recb_fast_keep_alive_status), "Fpga fast keep alive status" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			recb_misc_led_cntrl), "Fpga misc led cntrl" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			ms_arb.recb_mstr_config), "Fpga mstr config" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			ms_arb.recb_mstr_alive), "Fpga mstr alive" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			ms_arb.recb_mstr_alive_cnt), "Fpga mstr alive cnt" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			ms_arb.recb_mas_slv_sm_sw_disable), "Fpga mas slv sm sw disable" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			slow_bus.recb_slow_bus_addr), "Fpga slow bus addr" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			slow_bus.recb_slow_bus_data), "Fpga slow bus data" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			slow_bus.recb_slow_bus_ctrl), "Fpga slow bus ctrl" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			slow_bus.recb_slow_bus_i2c_sel), "Fpga slow bus i2c sel" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			uart_mux), "Fpga UART mux" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			ha_led), "Fpga HA led controller" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			interupt_source_reg), "Fpga interrupt source register " },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			interupt_source_en), "Fpga interrupt source enable " },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			re_to_cpp_mbox[0]), "Fpga re to cpp mbox byte 0" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			re_to_cpp_mbox[1]), "Fpga re to cpp mbox byte 1" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			re_to_cpp_mbox[2]), "Fpga re to cpp mbox byte 2" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			re_to_cpp_mbox[3]), "Fpga re to cpp mbox byte 3" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			re_to_cpp_mbox[4]), "Fpga re to cpp mbox byte 4" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			re_to_cpp_mbox[5]), "Fpga re to cpp mbox byte 5" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			re_to_cpp_mbox[6]), "Fpga re to cpp mbox byte 6" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			re_to_cpp_mbox[7]), "Fpga re to cpp mbox byte 7" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			cpp_to_re_mbox[0]), "Fpga cpp to re mbox byte 0" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			cpp_to_re_mbox[1]), "Fpga cpp to re mbox byte 1" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			cpp_to_re_mbox[2]), "Fpga cpp to re mbox byte 2" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			cpp_to_re_mbox[3]), "Fpga cpp to re mbox byte 3" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			cpp_to_re_mbox[4]), "Fpga cpp to re mbox byte 4" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			cpp_to_re_mbox[5]), "Fpga cpp to re mbox byte 5" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			cpp_to_re_mbox[6]), "Fpga cpp to re mbox byte 6" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			cpp_to_re_mbox[7]), "Fpga cpp to re mbox byte 7" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			re_to_cpp_mbox_event), "Fpga re to cpp event" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			cpp_to_re_mbox_event), "Fpga cpp to re event" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			cpp_to_re_mbox_event), "Fpga cpp to re event" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			master_re_i2c_status), "Fpga master re i2c status" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			i2c_scratch_pad), "Fpga i2c scratch pad" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			nextboot_reg0), "Fpga nextboot register 0" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			nextboot_reg1), "Fpga nextboot register 1" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			recb_card_type), "Fpga recb card type" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			watchdog_readback), "Fpga watchdog readback" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			watchdog_counter), "Fpga watchdog counter" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			sreset_history), "Fpga sreset history" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			int_spi_flash_cmd_reg), "Fpga int spi flash cmd reg" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			int_spi_flash_wr_data_reg), "Fpga int spi flash wr data reg" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			int_spi_flash_rd_data_reg), "Fpga int spi flash rd data reg" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			int_spi_flash_intr_data_reg), "Fpga int spi flash intr data reg" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			spi_program), "Fpga spi program" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			uart[0].data_buf), "Fpga UART 0 data buffer" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			uart[0].int_en), "Fpga UART 0 interrupt enable" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			uart[0].int_id_fifo_cntl), "Fpga UART 0 fifo cntl" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			uart[0].line_cntl), "Fpga UART 0 line cntl" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			uart[0].modem_cntl), "Fpga UART 0 modem cntl" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			uart[0].line_status), "Fpga UART 0 line status" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			uart[0].modem_status), "Fpga UART 0 modem status" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			uart[0].scratch), "Fpga UART 0 scratch" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			uart[1].data_buf), "Fpga UART 1 data buffer" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			uart[1].int_en), "Fpga UART 1 interrupt enable" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			uart[1].int_id_fifo_cntl), "Fpga UART 1 fifo cntl" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			uart[1].line_cntl), "Fpga UART 1 line cntl" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			uart[1].modem_cntl), "Fpga UART 1 modem cntl" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			uart[1].line_status), "Fpga UART 1 line status" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			uart[1].modem_status), "Fpga UART 1 modem status" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			uart[1].scratch), "Fpga UART 1 scratch" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			internalSPI.spi_ctl), "Fpga Inernal SPI Ctl" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			internalSPI.command), "Fpga Inernal SPI command" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			internalSPI.address_high), "Fpga Inernal SPI address high" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			internalSPI.address_mid), "Fpga Inernal SPI address mid" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			internalSPI.address_low), "Fpga Inernal SPI address_low" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			internalSPI.write_data), "Fpga Inernal SPI write data" },

	{ FPGA_BASE + fpga_offsetof(recb_fpga_regs_t,
			internalSPI.read_data), "Fpga Inernal SPI read_data" }
};

void 
fpga_testmode (u_int32_t pattern) {
	volatile u_int32_t reg_addr   = 0;
	*((u_int32_t *)fpga_reg_name_tbl[1].offset) = pattern;
	reg_addr = *((u_int32_t *)fpga_reg_name_tbl[1].offset);
	if (reg_addr == pattern) {
		printf(" Test Pass: wrote and read 0x%08x\n",
				reg_addr);
	} else {
		printf(" Test Fail: wrote and read 0x%08x\n",
				reg_addr);
	}
}

static int
check_offset_range (u_int32_t reg_offset) {
	int ret = 0, i = 0;
	for (i = 0; i < num_fpga_reg_tbl; i++) {
		if (fpga_reg_name_tbl[i].offset == reg_offset) {
			reg_indx = i;
			ret = 1;
		}
	}
	return ret;
}
int
do_fpga (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i = 0;
	unsigned int ret      = 0;
	unsigned int temp     = 0;
	int          first    = 1;
	ulong        addr     = 0;
	ulong        writeval = 0;
	recb_fpga_regs_t *recb_p = (recb_fpga_regs_t *)CFG_PARKES_BASE;
	num_fpga_reg_tbl =  sizeof(fpga_reg_name_tbl) /
		sizeof(struct fpga_reg_name_item);
	volatile u_int32_t reg_offset = 0;
	volatile u_int32_t val        = 0;
	volatile u_int32_t reg_addr   = 0;
	int len;
	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return (1);
	}
	if (strcmp(argv[1], "show") == 0) {
		printf("FPGA-Register                           Offset               Value\n");
		printf("____________________________________________________________"
				"________\n\n");
		for (i = 0; i < num_fpga_reg_tbl; i++) {
			printf("%-34s      0x%08x       0x%08x\n",
					fpga_reg_name_tbl[i].description,
					fpga_reg_name_tbl[i].offset,
					*(u_int32_t *)(fpga_reg_name_tbl[i].offset));
			if (ctrlc()) {
				putc('\n');
				goto end;
			}
		}
	}
end:
	if (strcmp(argv[1], "write") == 0) {
		printf("Write fpga\n");
		reg_offset = simple_strtoul(argv[2], NULL, 16);
		val        = simple_strtoul(argv[3], NULL, 16);
		reg_addr   = reg_offset;
		ret = check_offset_range(reg_offset);
		if (ret) {
			*((u_int32_t *)fpga_reg_name_tbl[reg_indx].offset) = (u_int32_t )val;
			printf("FPGA: write done !\n");
		} else {
			printf(" Fpga offset 0x%08x not valied\n", reg_offset);
		}
	}
	if (strcmp(argv[1], "read") == 0) {
		reg_offset = simple_strtoul(argv[2], NULL, 16);
		len        = simple_strtoul(argv[3], NULL, 10);
		ret = check_offset_range(reg_offset);
		if (ret) {
			if (len == 0) {
				printf(" Fpga read offset 0x%08x value = 0x%08x\n",
						fpga_reg_name_tbl[reg_indx].offset,
						*((u_int32_t *)fpga_reg_name_tbl[reg_indx].offset));
			}
			if (len < num_fpga_reg_tbl && len > 0) {
				for ( i = 0; i < len; i++) {
					printf(" Fpga read offset 0x%08x value = 0x%08x\n",
							fpga_reg_name_tbl[reg_indx].offset,
							*((u_int32_t *)fpga_reg_name_tbl[reg_indx].offset));
					reg_indx++;
				}
			}
			if (len > num_fpga_reg_tbl) {
				for (i = 0; i < num_fpga_reg_tbl; i++) {
					printf(" Fpga read offset 0x%08x value = 0x%08x\n",
							fpga_reg_name_tbl[reg_indx].offset,
							*((u_int32_t *)fpga_reg_name_tbl[reg_indx].offset));
					reg_indx++;
					if (reg_indx == num_fpga_reg_tbl) {
						break;
					}
				}
			}
		} else {
			printf(" Fpga offset 0x%08x not valied\n", reg_offset);
		}
	}
	if (strcmp(argv[1], "testmode") == 0) {
		fpga_testmode(CHECKERBOARD_PATTERN_55);
		fpga_testmode(CHECKERBOARD_PATTERN_AA);
	}
	return 0;
}

int
do_read_flash (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong flash_size;
	recb_fpga_regs_t *recb_p = (recb_fpga_regs_t *)CFG_PARKES_BASE;
	num_fpga_reg_tbl =  sizeof(fpga_reg_name_tbl) /
		sizeof(struct fpga_reg_name_item);
	volatile u_int32_t rs_val = 0;
	int reg_indx = 5;
	extern char version_string[];
	if (argc < 2)
		goto usage;
	if (strcmp(argv[1], "show") == 0) {
		printf ("%s\n", version_string);
		if ((flash_size = flash_init ()) > 0) {
			rs_val  = *((u_int32_t *)fpga_reg_name_tbl[reg_indx].offset);
			/* Check for partition bit set or not i.e 0 or 1 */
			printf("Active partition %d\n", (rs_val & 0x10)?1:0);
			puts ("Flash size: ");
			print_size (flash_size, "\n");
		} else {
			puts ("Failed to read flash Manufacture-ID, size, partition\n");
		}
	}
	return 1;
usage:
	printf ("Usage:\n%s\n", cmdtp->usage);
	return 1;
}

static void
recb_i2cs_set_odd_parity_func (uint8_t *tdata)
{
	/*
	 * Need parity generation for the data & offset
	 * Add parity to offset
	 */
	tdata[0] = recb_i2cs_set_odd_parity(tdata[0]);
	/* Add parity to data */
	tdata[1] = recb_i2cs_set_odd_parity(tdata[1]);
}

int
do_read_fled  (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong group    = 0x0f;
	ulong device   = 0x54;
	int alen       = 1;
	int pass       = 0;
	int ret        = 0;
	int len        = 1;
	int offset     = 0;
	uint8_t tdata[128];
	memset(tdata,0,sizeof(tdata));

	if (argc < 2)
		goto usage;

	if (strcmp(argv[1], "green") == 0) {
		if (strcmp(argv[2], "on") == 0) {
			/* offset addr of fabio led reg 0 */
			tdata[0] = 0x7c;
			/* data to write reg 0 */
			tdata[1] = 0x55;
			recb_i2cs_set_odd_parity_func(tdata);
			if ((ret = recb_iic_write((u_int8_t)group, (u_int8_t)device,
							len+alen, tdata))) {
				printf ("I2C write from device 0x%02x failed of offset 0x%02x.\n",
						device, tdata[0]);
			} else {
				pass = 1;
			}
			/* offset addr of fabio led reg 1 */
			tdata[0] = 0x7d;
			/* data to write reg 1 */
			tdata[1] = 0x2a;
			recb_i2cs_set_odd_parity_func(tdata);
			if ((ret = recb_iic_write((u_int8_t)group, (u_int8_t)device,
							len+alen, tdata))) {
				printf ("I2C write from device 0x%02x failed of offset 0x%02x.\n",
						device, tdata[0]);
			} else {
				pass |= 2;
			}
			/* offset addr of fabio led reg 2 */
			tdata[0] = 0x7e;
			/* data to write reg 2 */
			tdata[1] = 0x15;
			recb_i2cs_set_odd_parity_func(tdata);
			if ((ret = recb_iic_write((u_int8_t)group, (u_int8_t)device,
							len+alen, tdata))) {
				printf ("I2C write from device 0x%02x failed of offset 0x%02x.\n",
						device, tdata[0]);
			} else {
				pass |= 4;
			}
			/* If all the green led on fabio glows value 7 will be set */
			if (pass == 7) {
				printf("I2C write success!\n");
			}
		}
	}
	pass = 0;
	if (strcmp(argv[1], "red") == 0) {
		if (strcmp(argv[2], "on") == 0) {
			/* offset addr of fabio led reg 0 */
			tdata[0] = 0x7c;
			/* data to write reg 0 */
			tdata[1] = 0x2a;
			recb_i2cs_set_odd_parity_func(tdata);
			if ((ret = recb_iic_write((u_int8_t)group, (u_int8_t)device,
							len+alen, tdata))) {
				printf ("I2C write from device 0x%02x failed of offset 0x%02x.\n",
						device, tdata[0]);
			} else {
				pass = 1;
			}
			/* offset addr of fabio led reg 1 */
			tdata[0] = 0x7d;
			/* data to write reg 1 */
			tdata[1] = 0x55;
			recb_i2cs_set_odd_parity_func(tdata);
			if ((ret = recb_iic_write((u_int8_t)group, (u_int8_t)device,
							len+alen, tdata))) {
				printf ("I2C write from device 0x%02x failed of offset 0x%02x.\n",
						device, tdata[0]);
			} else {
				pass |= 2;
			}
			/* offset addr of fabio led reg 2 */
			tdata[0] = 0x7e;
			/* data to write reg 2 */
			tdata[1] = 0x2a;
			recb_i2cs_set_odd_parity_func(tdata);
			if ((ret = recb_iic_write((u_int8_t)group, (u_int8_t)device,
							len+alen, tdata))) {
				printf ("I2C write from device 0x%02x failed of offset 0x%02x.\n",
						device, tdata[0]);
			} else {
				pass |= 4;
			}
			/* If all the red led on fabio glows value 7 will be set */
			if (pass == 7) {
				printf("I2C write success!\n");
			}
		} 
	}
	pass = 0;
	if (strcmp(argv[1],"off") == 0) {
		/* offset addr of fabio led reg 0 */
		tdata[0] = 0x7c;
		/* data to write reg 0 */
		tdata[1] = 0x00;
		recb_i2cs_set_odd_parity_func(tdata);
		if ((ret = recb_iic_write((u_int8_t)group, (u_int8_t)device,
						len+alen, tdata))) {
			printf ("I2C write from device 0x%02x failed of offset 0x%02x.\n",
					device, tdata[0]);
		} else {
			pass = 1;
		}
		/* offset addr of fabio led reg 1 */
		tdata[0] = 0x7d;
		/* data to write reg 1 */
		tdata[1] = 0x00;
		recb_i2cs_set_odd_parity_func(tdata);
		if ((ret = recb_iic_write((u_int8_t)group, (u_int8_t)device,
						len+alen, tdata))) {
			printf ("I2C write from device 0x%02x failed of offset 0x%02x.\n",
					device, tdata[0]);
		} else {
			pass |= 2;
		}
		/* offset addr of fabio led reg 2 */
		tdata[0] = 0x7e;
		/* data to write reg 2 */
		tdata[1] = 0x00;
		recb_i2cs_set_odd_parity_func(tdata);
		if ((ret = recb_iic_write((u_int8_t)group, (u_int8_t)device,
						len+alen, tdata))) {
			printf ("I2C write from device 0x%02x failed of offset 0x%02x.\n",
					device, tdata[0]);
		} else {
			pass |= 4;
		}
		/* If all the led on fabio is off value 7 will be set */
		if (pass == 7) {
			printf("I2C write success!\n");
		}
	}
	return 1;
usage:
	printf ("Usage:\n%s\n", cmdtp->usage);
	return 1;
}

#define PSU_SLOT_0 0x11
#define PSU_SLOT_1 0x16
#define PSU_SLOT_2 0x12
#define PSU_SLOT_3 0x14
int
do_read_psu (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong group        = 0x18;
	ulong device       = 0x3f;
	ulong device_cpld  = 0x54;
	ulong offset       = 0;
	int ret            = 0;
	int alen           = 1;
	int len            = 1;
	int psu_slot       = 0;
	u8 *a              = (u8*)&offset;
	uint8_t tdata[128];
	memset(tdata, 0, sizeof(tdata));

	if (argc < 2) {
		goto usage;
	}
	if (strcmp(argv[1], "status") == 0) {
		if (strcmp(argv[2], "800") == 0) {
			/* Check for slot0 */
			printf("\nPSU 0 Slot\n");
			if ((ret = recb_iic_read(PSU_SLOT_0, (u_int8_t) device,
							len, tdata)) != 0) {
				printf ("I2C read from device 0x%x failed.\n", device);
			} else {
				/* Read value is compared to 12V ON/OFF & NOAC value */
				if (tdata[0] == 0x1F) {
					printf("The Normal 12V ON - eeprom read value 0x%02x\n",
							tdata[0]);
				} else if (tdata[0] ==  0x1E) {
					printf("The Normal 12V 0FF - eeprom read value 0x%02x\n",
							tdata[0]);
				} else if (tdata[0] == 0x14) {
					printf("No AC - eeprom read value 0x%02x\n",
							tdata[0]);
				} else {
					printf("The eeprom read value 0x%02x\n", tdata[0]);
				}
			}
			tdata[0] = 0x00;
			printf("\nPSU 1 Slot\n");
			/* Check for slot1 */
			if ((ret = recb_iic_read(PSU_SLOT_1, (u_int8_t) device,
							len, tdata)) != 0) {
				printf ("I2C read from device 0x%x failed.\n", device);
			} else {
				/* Read value is compared to 12V ON/OFF & NOAC value */
				if (tdata[0] == 0x1F) {
					printf("The Normal 12V ON - eeprom read value 0x%02x\n",
							tdata[0]);
				} else if (tdata[0] ==  0x1E) {
					printf("The Normal 12V 0FF - eeprom read value 0x%02x\n",
							tdata[0]);
				} else if (tdata[0] == 0x14) {
					printf("No AC - eeprom read value 0x%02x\n",
							tdata[0]);
				} else {
					printf("The eeprom read value 0x%02x\n",
							tdata[0]);
				}
			}
			tdata[0] = 0x00;
			printf("\nPSU 2 Slot\n");
			/* Check for slot2 */
			if ((ret = recb_iic_read(PSU_SLOT_2, (u_int8_t) device,
							len, tdata)) != 0) {
				printf ("I2C read from device 0x%x failed.\n", device);
			} else {
				/* Read value is compared to 12V ON/OFF & NOAC value */
				if (tdata[0] == 0x1F) {
					printf("The Normal 12V ON - eeprom read value 0x%02x\n",
							tdata[0]);
				} else if (tdata[0] ==  0x1E) {
					printf("The Normal 12V 0FF - eeprom read value 0x%02x\n",
							tdata[0]);
				} else if (tdata[0] == 0x14) {
					printf("No AC - eeprom read value 0x%02x\n", tdata[0]);
				} else {
					printf("The eeprom read value 0x%02x\n", tdata[0]);
				}
			}
			tdata[0] = 0x00;
			/* Check for slot3 */
			printf("\nPSU 3 Slot\n");
			if ((ret = recb_iic_read(PSU_SLOT_3, (u_int8_t) device,
							len, tdata)) != 0) {
				printf ("I2C read from device 0x%x failed.\n", device);
			} else {
				/* Read value is compared to 12V ON/OFF & NOAC value */
				if (tdata[0] == 0x1F) {
					printf("The Normal 12V ON - eeprom read value 0x%02x\n",
							tdata[0]);
				} else if (tdata[0] ==  0x1E) {
					printf("The Normal 12V 0FF - eeprom read value 0x%02x\n",
							tdata[0]);
				} else if (tdata[0] == 0x14) {
					printf("No AC - eeprom read value 0x%02x\n", tdata[0]);
				} else {
					printf("The eeprom read value 0x%02x\n", tdata[0]);
				}
			}
		}
		if (strcmp(argv[2], "1200") == 0) {
			/* Check for slot0 */
			printf("\nPSU 0 Slot\n");
			offset = 0xef;
			if ((ret = recb_iic_write(PSU_SLOT_0, (u_int8_t) device,
							alen, &a[BYTE_LENGTH - alen])) != 0) {
				printf ("I2C write from device 0x%x failed.\n", device);
			} else {
				if ((ret = recb_iic_read(PSU_SLOT_0, (u_int8_t) device,
								len, tdata)) != 0) {
					printf ("I2C read from device 0x%x failed.\n", device);
				} else {
				/* Read value is compared to 12V ON/OFF & NOAC value */
					if (tdata[0] == 0x1F) {
						printf("The Normal 12V ON - eeprom read value 0x%02x\n",
								tdata[0]);
					} else if (tdata[0] ==  0x1E) {
						printf("The Normal 12V 0FF - eeprom read value 0x%02x\n",
								tdata[0]);
					} else if (tdata[0] == 0x14) {
						printf("No AC - eeprom read value 0x%02x\n", tdata[0]);
					} else {
						printf("The eeprom read value 0x%02x\n", tdata[0]);
					}
				}
			}
			tdata[0] = 0x00;
			/* Check for slot1 */
			printf("\nPSU 1 Slot\n");
			offset = 0xef;
			if ((ret = recb_iic_write(PSU_SLOT_1, (u_int8_t) device,
							alen, &a[BYTE_LENGTH - alen])) != 0) {
				printf ("I2C write from device 0x%x failed.\n", device);
			} else {
				if ((ret = recb_iic_read(PSU_SLOT_1, (u_int8_t) device,
								len, tdata)) != 0) {
					printf ("I2C read from device 0x%x failed.\n", device);
				} else {
				/* Read value is compared to 12V ON/OFF & NOAC value */
					if (tdata[0] == 0x1F) {
						printf("The Normal 12V ON - eeprom read value 0x%02x\n",
								tdata[0]);
					} else if (tdata[0] ==  0x1E) {
						printf("The Normal 12V 0FF - eeprom read value 0x%02x\n",
								tdata[0]);
					} else if (tdata[0] == 0x14) {
						printf("No AC - eeprom read value 0x%02x\n", tdata[0]);
					} else {
						printf("The eeprom read value 0x%02x\n", tdata[0]);
					}
				}
			}
			tdata[0] = 0x00;
			/* Check for slot2 */
			printf("\nPSU 2 Slot\n");
			offset = 0xef;
			if ((ret = recb_iic_write(PSU_SLOT_2, (u_int8_t) device,
							alen, &a[BYTE_LENGTH - alen])) != 0) {
				printf ("I2C write from device 0x%x failed.\n", device);
			} else {
				if ((ret = recb_iic_read(PSU_SLOT_2, (u_int8_t) device,
								len, tdata)) != 0) {
					printf ("I2C read from device 0x%x failed.\n", device);
				} else {
				/* Read value is compared to 12V ON/OFF & NOAC value */
					if (tdata[0] == 0x1F) {
						printf("The Normal 12V ON - eeprom read value 0x%02x\n",
								tdata[0]);
					} else if (tdata[0] ==  0x1E) {
						printf("The Normal 12V 0FF - eeprom read value 0x%02x\n",
								tdata[0]);
					} else if (tdata[0] == 0x14) {
						printf("No AC - eeprom read value 0x%02x\n", tdata[0]);
					} else {
						printf("The eeprom read value 0x%02x\n", tdata[0]);
					}
				}
			}
			tdata[0] = 0x00;
			/* Check for slot3 */
			printf("\nPSU 3 Slot\n");
			offset = 0xef;
			if ((ret = recb_iic_write(PSU_SLOT_3, (u_int8_t) device,
							alen, &a[BYTE_LENGTH - alen])) != 0) {
				printf ("I2C write from device 0x%x failed.\n", device);
			} else {
				if ((ret = recb_iic_read(PSU_SLOT_3, (u_int8_t) device,
								len, tdata)) != 0) {
					printf ("I2C read from device 0x%x failed.\n", device);
				} else {
				/* Read value is compared to 12V ON/OFF & NOAC value */
					if (tdata[0] == 0x1F) {
						printf("The Normal 12V ON - eeprom read value 0x%02x\n",
								tdata[0]);
					} else if (tdata[0] ==  0x1E) {
						printf("The Normal 12V 0FF - eeprom read value 0x%02x\n",
								tdata[0]);
					} else if (tdata[0] == 0x14) {
						printf("No AC - eeprom read value 0x%02x\n", tdata[0]);
					} else {
						printf("The eeprom read value 0x%02x\n", tdata[0]);
					}
				}
			}
		}
	}
	if (strcmp(argv[1], "slot") == 0) {
		psu_slot = simple_strtoul(argv[2], NULL, 10);
		/* psu slot range 0, 1, 2, 3 */
		if (psu_slot >= 0 && psu_slot <= 3) {
			if (strcmp(argv[3], "on") == 0) {
				if (psu_slot == 0) {
					/* I2cs CPLD offset for slot 0 */
					offset = 0x74;
				}
				if (psu_slot == 1) {
					/* I2cs CPLD offset for slot 1 */
					offset = 0x75;
				}
				if (psu_slot == 2) {
					/* I2cs CPLD offset for slot 2 */
					offset = 0x76;
				}
				if (psu_slot == 3) {
					/* I2cs CPLD offset for slot 3 */
					offset = 0x77;
				}
				tdata[0] = offset;
				tdata[1] = 0x01;
				/* Need parity generation*/
				tdata[0] = recb_i2cs_set_odd_parity(tdata[0]); /* add parity to offset */
				tdata[1] = recb_i2cs_set_odd_parity(tdata[1]); /* add parity to data   */
				recb_i2cs_set_odd_parity_func(tdata);
				if ((ret = recb_iic_write((u_int8_t) group,
								(u_int8_t) device_cpld,
								len+alen, tdata))) {
					printf ("I2C write from device 0x%x failed.\n",
							device_cpld);
				} else {
					printf("The Normal 12V ON\n");
				}
			}
			if (strcmp(argv[3], "off") == 0) {
				if (psu_slot == 0) {
					offset = 0x74;
				}
				if (psu_slot == 1) {
					offset = 0x75;
				}
				if (psu_slot == 2) {
					offset = 0x76;
				}
				if (psu_slot == 3) {
					offset = 0x77;
				}
				tdata[0] = offset;
				tdata[1] = 0x00;
				/* Need parity generation*/
				tdata[0] = recb_i2cs_set_odd_parity(tdata[0]); /* add parity to offset */
				tdata[1] = recb_i2cs_set_odd_parity(tdata[1]); /* add parity to data   */
				recb_i2cs_set_odd_parity_func(tdata);
				if ((ret = recb_iic_write((u_int8_t) group,
								(u_int8_t) device_cpld,
								len+alen, tdata))) {
					printf ("I2C write from device 0x%x failed.\n",
							device_cpld);
				} else {
					printf("The Normal 12V OFF\n");
				}
			}
		} else {
			printf("Enter valid psu slot 0, 1, 2, 3\n");
		}
	}
	return 1;
usage:
	printf ("Usage:\n%s\n", cmdtp->usage);
	return 1;
}

int
do_read_pfpgamode (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int offset    = 0x70;
	int i         = 0;
	int ret       = 0;
	int len       = 1;
	int alen      = 1;
	ulong device  = 0x54;
	u8 *a = (u8*)&offset;
	uint8_t tdata[128];
	memset(tdata, 0, sizeof(tdata));
	if (argc < 2) {
		goto usage;
	}
	if (strcmp(argv[1], "status") == 0) {
		recb_i2cs_set_odd_parity((u_int8_t) (offset & 0xFF)); /* add parity to offset */
		if ((ret = recb_iic_write(LOCAL_RE, (u_int8_t) device, alen,
						&a[BYTE_LENGTH - alen])) != 0) {
			printf ("I2C read from device 0x%x failed.\n", device);
		} else {
			if ((ret = recb_iic_read(LOCAL_RE, (u_int8_t) device,
							len, tdata)) != 0) {
				printf ("I2C read from device 0x%x failed.\n", device);
			} else {
				if (tdata[0] & 0x20) {
					printf("Parkes FPGA got programmed in SPI mode using "
							"embedded flash inside the fpga\n");
				} else {
					printf("Parkes FPGA got programmed in BPI mode "
							"from external flash\n");
				}
			}
		}
	}
	return 1;
usage:
	printf ("Usage:\n%s\n", cmdtp->usage);
	return 1;
}

int
do_read_board (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	recb_fpga_regs_t *recb_p = (recb_fpga_regs_t *)CFG_PARKES_BASE;
	num_fpga_reg_tbl =  sizeof(fpga_reg_name_tbl) /
		sizeof(struct fpga_reg_name_item);
	int reg_indx = 2;
	volatile u_int32_t rs_val = 0;
	if (argc < 2) {
		goto usage;
	}
	if (strcmp(argv[1], "status") == 0) {
		rs_val = *((u_int32_t *) fpga_reg_name_tbl[reg_indx].offset);
		/* check for slot id bit */
		rs_val = (rs_val >> 8);
		if (rs_val & 0x02) {
			printf("RE SLOT ID = 1\n");
		} else {
			printf("RE SLOT ID = 0\n");
		}
	}
	return 1;
usage:
	printf ("Usage:\n%s\n", cmdtp->usage);
	return 1;
}


#define MAX_CFM_ID   12
static u_int32_t cfm_id[MAX_CFM_ID] = { 0x0001, 0x0002, 0x0004, 0x0008,
	0x0010, 0x0020, 0x0040, 0x0080,
	0x0100, 0x0200, 0x0400, 0x0800 };

int
do_read_cfm (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	recb_fpga_regs_t *recb_p = (recb_fpga_regs_t *)CFG_PARKES_BASE;
	num_fpga_reg_tbl =  sizeof(fpga_reg_name_tbl) /
		sizeof(struct fpga_reg_name_item);
	int reg_indx              = 10;
	int i                     = 0;
	volatile u_int32_t rs_val = 0;
	int card_pst              = 0;

	if (argc < 2) {
		goto usage;
	}
	if (strcmp(argv[1], "status") == 0) {
		rs_val = *((u_int32_t *) fpga_reg_name_tbl[reg_indx].offset);
		for (i = 0; i < MAX_CFM_ID; i++) {
			if (rs_val & cfm_id[i]) {
				card_pst = 1;
				printf("CFM card present at slot number: %d\n", (i + 1));
			}
		}
		if (!card_pst) {
			printf("CFM card not present !\n");
		}
	}
	return 1;
usage:
	printf ("Usage:\n%s\n", cmdtp->usage);
	return 1;
}

int
do_read_uart (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	gd->serial_reg_flag = 1;
	if (argc < 2) {
		goto usage;
	}
	if (strcmp(argv[1], "show") == 0) {
		serial_init();
	} else {
		puts ("Not a valied command !\n");
	}
	return 1;
usage:
	printf ("Usage:\n%s\n", cmdtp->usage);
	return 1;
}

#define RST_MIN 0
#define RST_MAX 14
int
do_reset_dev (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	recb_fpga_regs_t *recb_p = (recb_fpga_regs_t *)CFG_PARKES_BASE;
	num_fpga_reg_tbl =  sizeof(fpga_reg_name_tbl) /
		sizeof(struct fpga_reg_name_item);
	volatile u_int32_t val      = 0;
	u_int32_t test_bits         = 0x0000;
	u_int32_t read_fpga_bits    = 0;
	int reg_indx                = 5;
	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return (1);
	}
	if (strcmp(argv[1], "show") == 0) {
		printf("1.  FPGA I2C CORE RESET\n");
		printf("2.  FPGA UART CORE #0 RESET\n");
		printf("3.  FPGA UART CORE #1 RESET\n");
		printf("4.  PCIE TO SATA CHIP RESET\n");
		printf("5.  SATA TO PATA CHIP RESET\n");
		printf("6.  PHY RESET - BCM5482\n");
		printf("7.  USB CHIP RESET\n");
		printf("8.  ETHERNET SWITCH RESET\n");
		printf("9.  I2C MUX Device RESET\n");
		printf("10. CTRL RESET TO PEX8508\n");
		printf("11. HRESET\n");
		printf("12. SRESET\n");
		printf("13. FABIO RESET\n");
		return 1;
	}
	if (strcmp(argv[1], "reset") == 0) {
		val = simple_strtoul(argv[2], NULL, 10);
		if (val > RST_MIN && val < RST_MAX) {
			val = val - 1;
			test_bits = (u_int32_t)
				fpga_reg_test_name_tbl[val].offset;
			read_fpga_bits = *((u_int32_t *) fpga_reg_name_tbl[reg_indx].offset);
			read_fpga_bits |= test_bits;
			*((u_int32_t *)fpga_reg_name_tbl[reg_indx].offset) = (u_int32_t)
				read_fpga_bits;
			printf("Reset done : %s\n", fpga_reg_test_name_tbl[val].description);
		} else {
			printf("Input test range from 1 to 13\n");
		}
	}
	if (strcmp(argv[1], "clear") == 0) {
		if (strcmp(argv[2], "all") == 0) {
			*((u_int32_t *)fpga_reg_name_tbl[reg_indx].offset) = (u_int32_t)
				test_bits;
			printf("Clear done !\n");
		} else {
			val  = simple_strtoul(argv[2], NULL, 10);
			if (val > RST_MIN && val < RST_MAX) {
				val = val - 1;
				test_bits = (u_int32_t) fpga_reg_test_name_tbl[val].offset;
				read_fpga_bits = *((u_int32_t *)
						fpga_reg_name_tbl[reg_indx].offset);
				read_fpga_bits &= (~test_bits);
				*((u_int32_t *)fpga_reg_name_tbl[reg_indx].offset) = (u_int32_t)
					read_fpga_bits;
				printf("Clear done !\n");
			} else {
				printf("Input test range from 1 to 13\n");
			}
		}
	}
	return 1;
}

static int
do_rtc_1672 (cmd_tbl_t* cmdtp, int flag, int argc, char* argv[])
{
	char cmd1 = 0;
	char cmd2 = 0;
	char cmd3_0 = 0;
	unsigned long curr_word = 0;
	uint yrs = 0;
	uint mon = 0;
	uint day = 0;
	uint hrs = 0;
	uint min = 0;
	uint sec = 0;
	int i;

	if (argc < 2) {
		goto usage;
	}

	cmd1 = argv[1][0];
	cmd2 = argv[2][0];

	switch (cmd1) {
	case 'e':       /* enable rtc*/
	case 'E':
		en_rtc_1672();
		printf("\n RTC Enabled\n");
		return 0;

	case 'r':       /* show */
	case 'R':
		rtc_test();
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
			printf("\n RTC Disabled\n");
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

int
do_diag (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	int i;
	int j;
	if (argc == 1 || strcmp (argv[1], "run") != 0) {
		/* List test info */
		if (argc == 1) {
			puts ("Available hardware tests:\n");
			post_info (NULL);
			puts ("Use 'diag [<test1> [<test2> ...]]'"
					" to get more info.\n");
			puts ("Use 'diag run [<test1> [<test2> ...]]'"
					" to run tests.\n");
		} else {
			for (i = 1; i < argc; i++) {
				if (post_info (argv[i]) != 0)
					printf ("%s - no such test\n", argv[i]);
			}
		}
	} else {
		/* Check for minimum cmd arg to validate the cmd */
		if (argc == 2) {
			puts(" Please specify the test name\n");
		} else {
			if (!strcmp(argv[argc - 1], "-v")) {
				if(argc == 3){
					puts(" Please specify the test name\n");
				} else {
					for (i = 2; i < (argc - 1); i++) {
						if (post_run(argv[i], POST_RAM | POST_MANUAL | POST_DEBUG) != 0)
							printf ("%s - unable to execute the test\n", argv[i]);
					}
				}
			} else {
				for (i = 2; i < argc; i++) {
					if (post_run(argv[i], POST_RAM | POST_MANUAL) != 0)
						printf ("%s - unable to execute the test\n", argv[i]);
				}
			}
		}
	}
	return 0;
}

int do_ha_led (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong pci_base = 0;

	if (argc > 1) {
		pci_base = simple_strtoul(argv[1], NULL, 16);
	}
	init_ha_led(pci_base);
	return 0;
}

U_BOOT_CMD(
	i2c_dev_reg, 4, 1, do_i2c_reg,
	"i2c_dev_reg - Display i2c registers\n",
	"\n"
	"i2c_dev_reg show\n"
	"    - Display i2c registers\n"
	);


U_BOOT_CMD(
	i2c_ctrl,	2,	1,	do_i2c_controller,
	"i2c_ctrl     - select I2C controller\n",
	"ctrl\n    - I2C controller number\n"
	"      (valid controller values 1..2, default 1)\n"
);

U_BOOT_CMD(
	i2c_dev, 10, 1, do_i2c_diag,
	"i2c_dev   - diag on slave device directly on i2c\n",
	"r/w read (1) or write (0) operation\n"
	"ctrl specifies the i2c ctrl 1 or 2\n"
	"slave_addr  address (in hex) of the device on i2c\n"
	"reg - register (in hex) in the device to which data to be r/w\n"
	"data  the data to be r/w\n"
	"length_data length (decimal) of the data to r/w\n"
);

U_BOOT_CMD(
	i2c_switch, 9, 1,	do_i2c_diag_switch,
	"i2c_switch   - i2c on a specific slave device with switch\n",
	"	r/w	 read (1) or write (0) operation\n"
	"	ctrl specifies the i2c ctrl 1 or 2\n"
	"	switch	the switch address (hex) the dev sits on\n"
	"	channel	the channel number on the switch\n"
	"	slave_addr address (hex) of the device on i2c\n"
	"	reg - register (hex) in the device to which data to be r/w, 0xff - reg not applicable\n"
	"	data	the data to be r/w\n"
	"	length_data	length (decimal) of the data to r/w\n"
);

U_BOOT_CMD(
	i2c_cksum, 2, 1,	do_i2c_cksum,
	"i2c_cksum   - i2c Tx checksum on/off\n",
	"	on/off	 Tx checksum on (1) or off (0)\n"
);

U_BOOT_CMD(
	i2cs,   8,  1,  do_i2ce,
	"i2cs    - I2C commands using external controller\n",
	"\n"
	"i2cs reset \n"
	"    - reset selected i2c controller \n"
	"i2cs ew <group> <device> <address> <data> [hex | char]\n"
	"    - write data to device at i2c address\n"
	"i2cs er <group> <device> <address> <len>\n"
	"    - read data from device at i2c address\n"
	);
U_BOOT_CMD(
	fpga, 10, 1, do_fpga,
	"fpga  - fpga functionalites\n",
	"\n"
	"fpga show\n"
	"    - Display fpga registers\n"
	"fpga write <offset> <value> [hex]\n"
	"    - write data to fpga register [hex]\n"
	"fpga read <offset> <value>\n"
	"    - read data from fpga register\n"
	"fpga testmode\n"
	"    - worte and read to fpga scratch pad register pattern 0x5555 and 0xAAAA\n"
	);
U_BOOT_CMD(
	pfpgamode, 10, 1, do_read_pfpgamode,
	"pfpgamode - checks fpga is programmed in BPI or SPI mode\n",
	"\n"
	"pfpgamode status\n"
	"    - Displays in which mode fpga is programmed BPI or SPI mode\n"
	);
U_BOOT_CMD(
	reboard, 10, 1, do_read_board,
	"reboard - Board status register\n",
	"\n"
	"reboard status\n"
	"    - Display RE slot Id.\n"
	);
U_BOOT_CMD(
	cfm, 10, 1, do_read_cfm,
	"cfm - Display card presence register\n",
	"\n"
	"cfm status\n"
	"    - Display card presence register\n"
	);
U_BOOT_CMD(
	fled, 10, 1, do_read_fled,
	"fled - Displays menu to switch off/on fabio led\n",
	"\n"
	"fled  green <on>\n"
	"    - Display green led on\n"
	"fled  red  <on>\n"
	"    - Display red led on\n"
	"fled  off\n"
	"    - switch off led glow\n"
	);
U_BOOT_CMD(
	psu, 10, 1, do_read_psu,
	"psu - Display psu slot and on/off psu\n",
	"\n"
	"psu  status <800/1200> pwatts\n"
	"    - Display psu slot status based on selection of pwatts\n"
	"psu  slot  <number> <on/off>\n"
	"    - Based on valid psu slot 0, 1, 2, 3 switch ON/OFF psu\n"
	);
U_BOOT_CMD(
	flash, 10, 1, do_read_flash,
	"flash - flash command to access flash device\n",
	"\n"
	"flash show\n"
	"    - Display flash Verson, Manufacture-Id, Partition and size\n"
	);

U_BOOT_CMD(
	uart, 10, 1, do_read_uart,
	"uart - Display uart registers\n",
	"\n"
	"uart show\n"
	"    - Display uart registers\n"
	);
U_BOOT_CMD(
	reset_dev, 8, 1, do_reset_dev,
	"reset - Display reset option of the devices\n",
	"\n"
	"reset_dev  show\n"
	"    - Display reset commands\n"
	"reset_dev  reset <number>\n"
	"    - reset devices @number\n"
	"reset_dev  clear <number>\n"
	"    - clear reset devices @number\n"
	"reset_dev  clear all \n"
	"    - clear all reset of the dev\n"
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
	"rtc   rtctest \n"
	"    - Test RTC chip\n"
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
	ha_led, 2, 0, do_ha_led,
	"ha_led   - Init HA port LED controller\n",
	"\n"
	"ha_led [<pci base address>]\n"
	);
#endif /* (CONFIG_COMMANDS & CFG_CMD_BSP) */
