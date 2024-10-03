/***********************************************************************
Copyright 2003-2006 Raza Microelectronics, Inc.(RMI). All rights
reserved.
Use of this software shall be governed in all respects by the terms and
conditions of the RMI software license agreement ("SLA") that was
accepted by the user as a condition to opening the attached files.
Without limiting the foregoing, use of this software in source and
binary code forms, with or without modification, and subject to all
other SLA terms and conditions, is permitted.
Any transfer or redistribution of the source code, with or without
modification, IS PROHIBITED,unless specifically allowed by the SLA.
Any transfer or redistribution of the binary code, with or without
modification, is permitted, provided that the following condition is
met:
Redistributions in binary form must reproduce the above copyright
notice, the SLA, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution:
THIS SOFTWARE IS PROVIDED BY Raza Microelectronics, Inc. `AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL RMI OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*****************************#RMI_3#***********************************/
#ifndef __PHYSADDRSPACE_H__
#define __PHYSADDRSPACE_H__

#include "rmi/rmi_processor.h"
#include "rmi/board.h"

#define KB(x) ((x) << 10)
#define MB(x) ((x) << 20)
#define GB(x) ((x) << 30)
#define TB(x) ((x) << 40)

#define PSB_MEM_MAP_MAX 32
#define PSB_IO_MAP_MAX 32

/* The Processor address space (1TB) is divided into: */
enum {
	PHYS_MEMORY = 1,
	PHYS_IO,
	PHYS_AVAILABLE
};

/* The PHYS_MEMORY, some areas are RAM and some areas are ROM */
enum {
	BOOT_MEM_RAM      =  1,
	BOOT_MEM_ROM_DATA =  2,
	BOOT_MEM_RESERVED =  3,

	/* The PHYS_IO is divided into: */
        PROCESSOR_PIO_SPACE = 0x10, /* The Processor's Peripheral 
				       and IO Configuration Space */
        PCIX_IO_SPACE,
        PCIX_CFG_SPACE,
        PCIX_MEMORY_SPACE,
        HT_IO_SPACE,
        HT_CFG_SPACE,
        HT_MEMORY_SPACE,
        SRAM_SPACE,
        FLASH_CONTROLLER_SPACE,
        PCIE_IO_SPACE,
        PCIE_CFG_SPACE,
        PCIE_MEMORY_SPACE
};

/* There could be a number of them */
struct psb_mem_map {
	int nr_map;
	struct psb_mem_map_entry {
		uint64_t addr;	/* start of memory segment */
		uint64_t size;	/* size of memory segment */
		uint32_t type;	/* type of memory segment */
	} map[PSB_MEM_MAP_MAX];
};

/*
  Each of the above is a separate region. For e.g.
  PROCESSOR_PIO_SPACE starts typically at 0x1ef00000
  and is of size 1MB 
*/
struct psb_io_map {
	int nr_map;
	struct psb_io_map_entry {
		uint64_t addr;	/* start of IO segment */
		uint64_t size;	/* size of IO segment */
		long type;	/* type of IO segment */
	} map[PSB_IO_MAP_MAX];
};

/* 
   Use bits 16-32 to indicate whether an area is shared,
   can be used for storing env variables, etc.
   Bits 15-0 could be used to indicate the resource type
   PHYS_MEMORY,PHYS_IO etc.
*/
enum region_attrib{
	SHARED    = 0x00010000,
	BOOTCODE  = 0x00020000,
	BKUPCODE  = 0x00040000,
	ENVSTORE  = 0x00080000,
};

/********************************************************************
 PCIe Address Map
********************************************************************/
enum pcie_map {
	 PCIE_MEM_BAR_BASE = MB(256), /* 0x10000000 */
	 PCIE_MEM_BAR_SIZE = MB(128),

	 PCIE_IO_BAR_BASE  = 0xd0000000,
	 PCIE_IO_BAR_SIZE  = MB(64),

	 PCIE_CONFIG_BAR_BASE = MB(384),
	 PCIE_CONFIG_BAR_SIZE = MB(32),
};
struct board;
extern void physaddr_init(struct board *this_board);
extern uint64_t physaddr_reserve_region(const char *name, int type,
				 uint64_t base, uint64_t size);

void physaddr_release_region(const char *name, int type,uint64_t base, uint64_t size);
#endif
