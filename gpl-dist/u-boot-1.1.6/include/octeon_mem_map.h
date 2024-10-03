/*
 * (C) Copyright 2004,2005
 * Cavium Networks
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
#define OCTEON_NUM_CORES    16

/* Debugger related defines */
#define DEBUG_STACK_SIZE        1024    /* Size of the debugger stack */
#define DEBUG_NUMREGS           512    /* Total number of (64 bit)registers stored. Must match debug_handler.S */


#define EXCEPTION_BASE_BASE     0          /* must be 4k aligned */
#define EXCEPTION_BASE_INCR     (4*1024)   /* Increment size for exception base addresses (4k minimum) */
#define OCTEON_EXCEPTION_VECTOR_BLOCK_SIZE  (OCTEON_NUM_CORES*EXCEPTION_BASE_INCR) /* 16 4k blocks */

#define BOOTLOADER_DEBUG_REG_SAVE_SIZE  (OCTEON_NUM_CORES * DEBUG_NUMREGS * 8)
#define BOOTLOADER_DEBUG_STACK_SIZE  (OCTEON_NUM_CORES * DEBUG_STACK_SIZE)
#define BOOTLOADER_PCI_READ_BUFFER_SIZE     (256)
#define BOOTLOADER_PCI_WRITE_BUFFER_SIZE     (256)

/* Bootloader debugger stub register save area follows exception vector space */
#define BOOTLOADER_DEBUG_REG_SAVE_BASE  (EXCEPTION_BASE_BASE + OCTEON_EXCEPTION_VECTOR_BLOCK_SIZE)
/* debugger stack follows reg save area */
#define BOOTLOADER_DEBUG_STACK_BASE  (BOOTLOADER_DEBUG_REG_SAVE_BASE + BOOTLOADER_DEBUG_REG_SAVE_SIZE)
/* pci communication blocks.  Read/write are from bootloader perspective*/
#define BOOTLOADER_PCI_READ_BUFFER_BASE   (BOOTLOADER_DEBUG_STACK_BASE + BOOTLOADER_DEBUG_STACK_SIZE)   /* Original bootloader command block */
#define BOOTLOADER_BOOTMEM_DESC_ADDR      (BOOTLOADER_PCI_READ_BUFFER_BASE + BOOTLOADER_PCI_READ_BUFFER_SIZE)
#define BOOTLOADER_END_RESERVED_SPACE   (BOOTLOADER_PCI_WRITE_BUFFER_BASE + 0x8 /* BOOTLOADER_BOOTMEM_DESC_ADDR size */)




/* Use the range EXCEPTION_BASE_BASE + 0x800 - 0x1000 (2k-4k) for bootloader private data
** structures that need fixed addresses
*/                                        

#define BOOTLOADER_PRIV_DATA_BASE       (EXCEPTION_BASE_BASE + 0x800)
#define BOOTLOADER_BOOT_VECTOR          (BOOTLOADER_PRIV_DATA_BASE)
#define BOOTLOADER_DEBUG_TRAMPOLINE     (BOOTLOADER_BOOT_VECTOR + BOOT_VECTOR_SIZE)   /* WORD */
#define BOOTLOADER_DEBUG_GP_REG         (BOOTLOADER_DEBUG_TRAMPOLINE + 4)   /* WORD */
#define BOOTLOADER_DEBUG_HANDLER_LIST   (BOOTLOADER_DEBUG_GP_REG + 4)   /* WORD */
#define BOOTLOADER_DEBUG_FLAGS_BASE     (BOOTLOADER_DEBUG_HANDLER_LIST + 4*8)  /* WORD * NUM_CORES */
#define BOOTLOADER_DEBUG_KO_REG_SAVE    (BOOTLOADER_DEBUG_FLAGS_BASE + 4*OCTEON_NUM_CORES)
#define BOOTLOADER_NEXT_AVAIL_ADDR      (BOOTLOADER_DEBUG_KO_REG_SAVE + 4)

/* Debug flag bit definitions in cvmx-bootloader.h */

/* Address used for boot vectors for non-zero cores */
#define BOOT_VECTOR_BASE    (0x80000000 | BOOTLOADER_BOOT_VECTOR)
#define BOOT_VECTOR_NUM_WORDS           (8)
#define NMI_HANDLER_NUM_WORDS           (6)
#define BOOT_VECTOR_SIZE                (((OCTEON_NUM_CORES*4) *        \
                                         BOOT_VECTOR_NUM_WORDS) +       \
                                         (NMI_HANDLER_NUM_WORDS * 4))


/* Real physical addresses of memory regions */
#define OCTEON_DDR0_BASE    (0x0ULL)
#define OCTEON_DDR0_SIZE    (0x010000000ULL)
#define OCTEON_DDR1_BASE    (0x410000000ULL)
#define OCTEON_DDR1_SIZE    (0x010000000ULL)
#define OCTEON_DDR2_BASE    (0x020000000ULL)
#define OCTEON_DDR2_SIZE    (0x3e0000000ULL)
#define OCTEON_MAX_PHY_MEM_SIZE (16*1024*1024*1024ULL)



/* Reserved DRAM for exception vectors, bootinfo structures, bootloader, etc */
#define OCTEON_RESERVED_LOW_MEM_SIZE    (1*1024*1024)


#define OCTEON_BOOTLOADER_NAMED_BLOCK_TMP_PREFIX "__tmp"



/* The environment variables used for controlling the reserved block allocations are:
** octeon_reserved_mem_bl_base
** octeon_reserved_mem_bl_size
** octeon_reserved_mem_linux_base
** octeon_reserved_mem_linux_size
**
** If the size is defined to be 0, then the reserved block is not created.  See bootloader HTML
** documentation for more information.
*/
/* DRAM reserved for loading images.  This is freed just
** before the application starts, but is used by the bootloader for
** loading ELF images, etc */
#define OCTEON_BOOTLOADER_LOAD_MEM_NAME    "__tmp_load"
/* DRAM section reserved for the Linux kernel (we want this to be contiguous with
** the bootloader reserved area to that they can be merged on free.
*/
#define OCTEON_LINUX_RESERVED_MEM_NAME    "__tmp_reserved_linux"


/* Named block name for block containing u-boot environment that is created
** for use by application */
#define OCTEON_ENV_MEM_NAME    "__bootloader_env"

/* Wired TLB entry used for IO space corresponding to USB */
#define NUM_WIRED_TLB_ENTRIES 1
#define WIRED_TLB_VIRT_BASE 0xC0000000

/* 
 * lo0 entry is PFN ((0x00016F0010000000ull >> 12) << 6) | C (2) | D (1) | V (1)
 * | G (1) ==> 0x5BC00400017 
 */
#define WIRED_TLB_LO0_U          0x000005BC
#define WIRED_TLB_LO0_L          0x00400017

/* 
 * lo1 entry is PFN ((0x00017F0010000000ull >> 12) << 6) | C (2) | D (1) | V (1)
 * | G (1) ==> 0x5FC00400017 
 */
#define WIRED_TLB_LO1_U          0x000005FC
#define WIRED_TLB_LO1_L          0x00400017

#define WIRED_TLB_PAGE_MASK      0x0007E000            /* Size = 256k */

#define OCTEON_USB_UNIT0_VIRT_BASE WIRED_TLB_VIRT_BASE
#define OCTEON_USB_UNIT1_VIRT_BASE (WIRED_TLB_VIRT_BASE + (256 * 1024))
