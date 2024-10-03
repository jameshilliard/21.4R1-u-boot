/*
 * Copyright (c) 2008-2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * (C) Copyright 2002
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
/*
 * The following data structure is placed in some memory wich is
 * available very early after boot (like DPRAM on MPC8xx/MPC82xx, or
 * some locked parts of the data cache) to allow for a minimum set of
 * global variables during system initialization (until we have set
 * up the memory controller so that we can use RAM).
 *
 * Keep it *SMALL* and remember to set CFG_GBL_DATA_SIZE > sizeof(gd_t)
 */

typedef	struct	global_data {
	bd_t		*bd;
	/*
	 * It's infinitely better to have the jump table at the
	 * beginning of the global data structure with no new
	 * fields being added in front of it than when it's at
	 * the rear of the structure. The JUNOS loader uses the
	 * jump table to call into U-Boot and having it at a
	 * fixed offset within the structure is crucial for
	 * portability and compatibility.
	 */
	void		**jt;		/* jump table */
	unsigned long	flags;
	unsigned long	baudrate;
	unsigned long	have_console;	/* serial_init() was called */
	unsigned long	reloc_off;	/* Relocation Offset */
	unsigned long	env_addr;	/* Address  of Environment struct */
	unsigned long	env_valid;	/* Checksum of Environment valid? */
	unsigned long	fb_base;	/* base address of frame buffer */
#ifdef CONFIG_VFD
	unsigned char	vfd_type;	/* display type */
#endif
#if defined(CONFIG_RPS200) || \
    defined(CONFIG_EX2200) || \
    defined(CONFIG_EX3300)
	unsigned long	cpu_clk;	/* CPU clock in Hz!		*/
	unsigned long	ram_size;	/* RAM size */
	unsigned long	flash_size;	/* Flash size */
	unsigned long	board_type;
#endif
#if defined(CONFIG_RPS200)
	unsigned long	post_ram;
	unsigned long	post_cpld;
	unsigned long	post_nand;
#endif
#if defined(CONFIG_EX2200) || \
    defined(CONFIG_EX3300)
	unsigned char mem_cfg;
	unsigned char valid_bid;
	unsigned short last_reset;
#if defined(CONFIG_POST) || defined(CONFIG_LOGBUFFER)
	unsigned long	post_log_word;  /* Record POST activities */
	unsigned long	post_init_f_time;  /* When post_init_f started */
#endif
#endif
#ifdef CONFIG_MARVELL
	unsigned long  bus_clk;
	unsigned long  sw_clk;
       unsigned int tclk;
#endif
#if 0
	unsigned long	cpu_clk;	/* CPU clock in Hz!		*/
	unsigned long	bus_clk;
	unsigned long	ram_size;	/* RAM size */
	unsigned long	reset_status;	/* reset status register at boot */
#endif
} gd_t;

/*
 * Global Data Flags
 */
#define	GD_FLG_RELOC	0x00001		/* Code was relocated to RAM		*/
#define	GD_FLG_DEVINIT	0x00002		/* Devices have been initialized	*/
#define	GD_FLG_SILENT	0x00004		/* Silent mode				*/

#define DECLARE_GLOBAL_DATA_PTR     register volatile gd_t *gd asm ("r8")

#endif /* __ASM_GBL_DATA_H */
