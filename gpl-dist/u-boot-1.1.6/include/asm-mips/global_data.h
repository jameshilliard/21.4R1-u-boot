/*
 * (C) Copyright 2002-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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

#ifndef	__ASM_GBL_DATA_H
#define __ASM_GBL_DATA_H

#include <asm/regdef.h>

#ifdef CONFIG_OCTEON
#include <octeon_eeprom_types.h>
#endif
/*
 * The following data structure is placed in some memory wich is
 * available very early after boot (like DPRAM on MPC8xx/MPC82xx, or
 * some locked parts of the data cache) to allow for a minimum set of
 * global variables during system initialization (until we have set
 * up the memory controller so that we can use RAM).
 *
 * Keep it *SMALL* and remember to set CFG_GBL_DATA_SIZE > sizeof(gd_t)
 */
#ifdef CONFIG_OCTEON
#define SERIAL_LEN  20
#define GD_TMP_STR_SIZE  30
#endif
typedef	struct	global_data {
	bd_t		*bd;
	void		**jt;		/* jump table */
	unsigned long	flags;
	unsigned long	baudrate;
	unsigned long	have_console;	/* serial_init() was called */
#if defined (CONFIG_OCTEON) || defined (CONFIG_RMI_XLS) 
	uint64_t    	ram_size;	/* RAM size */
#else
	unsigned long       /* RAM size */
#endif
	unsigned long	reloc_off;	/* Relocation Offset */
	unsigned long	env_addr;	/* Address  of Environment struct */
	unsigned long	env_valid;	/* Checksum of Environment valid? */
#ifdef CONFIG_OCTEON
 	unsigned long       cpu_clock_mhz;  /* CPU clock speed in Mhz */
	unsigned long       ddr_clock_mhz;  /* DDR clock (not data rate!) in Mhz */
	unsigned long       sys_clock_mhz;  /* co-processor clock in Mhz */
	unsigned long       ddr_ref_hertz;
	int                 mcu_rev_maj;
	int                 mcu_rev_min;
	int                 console_uart;
	/* EEPROM data structures as read from EEPROM or populated by other
	** means on boards without an EEPROM */
	octeon_eeprom_board_desc_t board_desc;
	octeon_eeprom_clock_desc_t clock_desc;
	octeon_eeprom_mac_addr_t   mac_desc;

    char                *err_msg;   /* pointer to error message to save until console is up */
    char                tmp_str[GD_TMP_STR_SIZE];  /* Temporary string used in ddr init code before DRAM is up */
    unsigned long       uboot_flash_address;    /* Address of normal bootloader in flash */
    unsigned long       uboot_flash_size;       /* Size of normal bootloader */
    uint64_t            dfm_ram_size;   /* DFM RAM size */

#endif
       
#if (defined(CONFIG_POST) && defined(CONFIG_OCTEON))
	uint32_t        post_result;
#endif
#ifdef CONFIG_API
    void            *api_sig_addr;      /** Pointer to API signature */
#endif

} gd_t;


/*
 * Global Data Flags
 */
#define	GD_FLG_RELOC	0x00001		/* Code was relocated to RAM     */
#define	GD_FLG_DEVINIT	0x00002		/* Devices have been initialized */
#define	GD_FLG_SILENT	0x00004		/* Silent mode			 */
#ifdef CONFIG_OCTEON
#define GD_FLG_CLOCK_DESC_MISSING   0x0008
#define GD_FLG_BOARD_DESC_MISSING   0x0010
#define GD_FLG_DDR_VERBOSE          0x0020
#define GD_FLG_BOARD_EEPROM_READ_FAILED 0x0800
#define GD_FLG_RAM_RESIDENT          0x0200
#define GD_FLG_DDR0_CLK_INITIALIZED  0x0040
#define GD_FLG_DDR1_CLK_INITIALIZED  0x0080
#define GD_FLG_DDR2_CLK_INITIALIZED  0x0100
#define GD_FLG_MEMORY_PRESERVED      0x8000

#endif


#define DECLARE_GLOBAL_DATA_PTR     register volatile gd_t *gd asm ("k0")

#endif /* __ASM_GBL_DATA_H */
