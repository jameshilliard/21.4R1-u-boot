/*
 * This file contains the configuration parameters for the RMI boards
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <config.h>
/* FIXME - this is reqd to make cmd_tbl_t struct aligned for now 
 * without this 4 extra bytes are getting into the structure
 Need to remove this later
 */
#define CFG_LONGHELP 1

#define CFG_MAX_FLASH_SECT 128

/* This must be the start of uboot (boot2) i.e load address of uboot */
#define CFG_MONITOR_BASE TEXT_BASE
/* Default baud rate */
#define CONFIG_BAUDRATE 9600


#define CFG_SDRAM_BASE	0x80000000
#define CFG_MALLOC_LEN	(8 << 20) /* 8 MB for now */
#define CFG_BOOTPARAMS_LEN	128*1024
#define CONFIG_USB_SHOW_NO_PROGRESS

//#define CFG_ENV_IS_NOWHERE 1

#define CFG_ENV_IS_IN_FLASH 1

#ifdef RMI_EVAL_BOARD
#define CFG_ENV_OFFSET ((4 << 20) - (128 << 10))
#define CFG_ENV_ADDR 	(0xbfc00000 + CFG_ENV_OFFSET)
#define CFG_ENV_SECT_SIZE (128 << 10)
#define CFG_ENV_SIZE (128 << 10)
#define CFG_PROMPT              "RMI_BOARD # "
#define CFG_MAX_FLASH_BANKS     2       /* max number of memory banks */
#define CONFIG_BOOTCOMMAND \
               "bootelf 0xbc200000"
#endif

#define CFG_FLASH_BASE		0xBc000000
#define CFG_HZ			rmi_cpu_frequency

#define CFG_CBSIZE              256
#define CFG_MAXARGS             16              /* max number of command args*/
#define CFG_PBSIZE		512

#define CFG_LOAD_ADDR           0x81000000     /* default load address  */

/* valid baudrates */
#define CFG_BAUDRATE_TABLE      { 9600, 19200, 38400, 57600, 115200 }

#define CFG_MEMTEST_START	0x80100000
#define CFG_MEMTEST_END		0x80800000

/* supported commands, add any other supported cmds here */
#define CONFIG_COMMANDS (CFG_CMD_USB  | CFG_CMD_EXT2 | CFG_CMD_FAT  | \
			CFG_CMD_DHCP | CFG_CMD_PING | CFG_CMD_NET  | \
			CFG_CMD_FLASH | CFG_CMD_MEMORY |  \
		        CFG_CMD_ENV | CFG_CMD_PCI | CFG_CMD_ELF | CFG_CMD_RUN)

/* This must be included AFTER the definition of CONFIG_COMMANDS (if any) */
#include <cmd_confdefs.h>

#define CONFIG_MISC_INIT_R
/* For USB, at least one among CONFIG_MAC_PARTITION / CONFIG_DOS_PARTITION / 
   CONFIG_ISO_PARTITION needs to be configured */
#define CONFIG_DOS_PARTITION

#define CONFIG_USB_OHCI       /* USB */
#define CONFIG_USB_STORAGE    /* USB */

#define CONFIG_GMAC_RMIBOARD  /* gmac */
#define CONFIG_NET_MULTI      /* Multi ethernet cards support */
#define CFG_RX_ETH_BUFFER 16  /* gmac */

#define CONFIG_PCI            /* pci */
#define CONFIG_PCI_PNP
#define CONFIG_RMI_NATSEMI
#define CONFIG_RMI_PCI_SKY2

#define CONFIG_BOOTDELAY        5

#endif
