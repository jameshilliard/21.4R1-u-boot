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


/**
 * @file octeon_boot.h
 *
 * $Id$
 * 
 */
 
#ifndef __OCTEON_BOOT_H__
#define __OCTEON_BOOT_H__
#include <octeon-app-init.h> 
#include "octeon_mem_map.h"


/* Defines for GPIO switch usage */
/* Switch 0 is failsafe/normal bootloader */
#define OCTEON_GPIO_SHOW_FREQ       (0x1 << 1)   /* switch 1 set shows freq display */

#define OCTEON_BZERO_PFS_STRIDE (8*128ULL)
#define OCTEON_BZERO_PFS_STRIDE_MASK (OCTEON_BZERO_PFS_STRIDE - 1)


#define OCTEON_BOOT_DESC_IMAGE_LINUX    (1 << 0)


uint64_t octeon_get_cycles(void);
extern uint32_t cur_exception_base;

extern octeon_boot_descriptor_t boot_desc[16];

extern uint32_t coremask_iter_core;
extern uint32_t coremask_iter_mask;
extern int coremask_iter_first_core;
extern uint32_t coremask_to_run;
extern uint64_t boot_cycle_adjustment;
extern uint32_t coremask_from_eeprom;

int octeon_bist(void);
uint32_t get_except_base_reg(void);
uint32_t get_core_num(void);
uint32_t get_except_base_addr(void);
void set_except_base_addr(uint32_t addr);
void copy_default_except_handlers(uint32_t addr);
void cvmx_set_cycle(uint64_t cycle);
uint32_t get_coremask_override(void);
uint32_t octeon_coremask_num_cores(int num_cores);
int coremask_iter_get_first_core(void);
int coremask_iter_next(void);
int coremask_iter_init(uint32_t mask);
uint64_t get_cop0_cvmctl_reg(void);
void set_cop0_cvmctl_reg(uint64_t val);
uint64_t get_cop0_cvmmemctl_reg(void);
void set_cop0_cvmmemctl_reg(uint64_t val);
uint64_t octeon_phy_mem_list_init(uint64_t mem_size, uint32_t low_reserved_bytes);
uint32_t octeon_get_cop0_status(void);
uint64_t octeon_read_gpio(void);
void octeon_gpio_cfg_output(int bit);
void octeon_gpio_cfg_input(int bit);
void octeon_gpio_ext_cfg_output(int bit);
void octeon_gpio_ext_cfg_input(int bit);
void octeon_gpio_set(int bit);
void octeon_gpio_clr(int bit);
int octeon_gpio_value(int bit);
void octeon_display_mem_config(void);
void octeon_led_str_write(const char *str);
int octeon_mcu_read_ddr_clock(void);
int octeon_mcu_read_cpu_ref(void);
int octeon_ebt3000_rev1(void);
void octeon_delay_cycles(uint64_t cycles);
int octeon_boot_bus_init(void);
void octeon_write64_byte(uint64_t csr_addr, uint8_t val);
uint8_t octeon_read64_byte(uint64_t csr_addr);
void octeon_bzero64_pfs(uint64_t start_addr_arg, uint64_t count_arg);
int octeon_cf_present(void);
int32_t validate_coremask(uint32_t core_mask);
int octeon_setup_boot_desc_block(uint32_t core_mask, int argc, char **argv, uint64_t entry_point, uint32_t stack_size, uint32_t heap_size, uint32_t boot_flags, uint64_t stack_heep_base_addr, uint32_t image_flags, uint32_t config_flags);
int octeon_setup_boot_vector(uint32_t func_addr, uint32_t core_mask);
void start_cores(uint32_t coremask_to_start);
int cvmx_spi4000_initialize(int interface);
int cvmx_spi4000_detect(int interface);
void octeon_flush_l2_cache(void);
uint32_t octeon_get_available_coremask(void);
uint32_t octeon_get_available_core_count(void);
void octeon_check_mem_load_range(void);
void octeon_setup_cvmx_sysinfo(void);
int octeon_check_mem_errors(void);
uint8_t cpld_read (uint8_t addr);
void cpld_write(uint8_t addr, uint8_t val);
uchar jsrxnle_read_eeprom(uchar offset);

#define OCTEON_BREAK {asm volatile ("break");}
#define OCTEON_SYNC asm volatile ("sync" : : :"memory")
/* JSRXNLE_FIXME - Hardcoded instruction for syncw */
/*#define OCTEON_SYNCW asm volatile ("syncw" : : :"memory")*/
#define OCTEON_SYNCW asm volatile (".long 0x10F" : : :"memory")


#define CAST64(v) ((long long)(long)(v))
#define CASTPTR(type, v) ((type *)(long)(v))

#include "executive-config.h"
#include "cvmx-config.h"
#include "octeon_hal.h"
#include "cvmx-bootmem.h"



#define CVMX_CACHE_LINE_SIZE    (128)   // In bytes
#define CVMX_CACHE_LINE_MASK    (CVMX_CACHE_LINE_SIZE - 1)   // In bytes


/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/

/* Move these out of octeon-app-init with new names, change to orig names once
** removed from octeon-app-init.h
*/
#define NUM_OCTEON_TLB_ENTRIES  32
/** Description of TLB entry, used by bootloader */
typedef struct
{
    uint64_t    ri:     1;
    uint64_t    xi:     1;
    uint64_t    cca:    3;
} tlb_flags_t;
typedef struct 
{
    uint64_t phy_page0;
    uint64_t phy_page1;
    uint64_t virt_base;
    uint32_t page_size;
    tlb_flags_t flags;
} octeon_tlb_entry_t_bl;


uint32_t mem_copy_tlb(octeon_tlb_entry_t_bl *tlb_array, uint64_t dest_virt, uint64_t src_phys, uint32_t len);

void memset64(uint64_t start_addr, uint8_t value, uint64_t len);
uint64_t memcpy64(uint64_t dest_addr, uint64_t src_addr, uint64_t count);
void octeon_free_tmp_named_blocks(void);

/* These macros simplify the process of creating common IO addresses */
#define OCTEON_IO_SEG 2LL
#define OCTEON_ADD_SEG(segment, add)          ((((uint64_t)segment) << 62) | (add))
#define OCTEON_ADD_IO_SEG(add)                OCTEON_ADD_SEG(OCTEON_IO_SEG, (add))



#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#if MIPS64_CONTEXT_SAVE
typedef uint64_t    trap_reg_t;
#else
typedef uint32_t    trap_reg_t;
#endif

/* trap frame generated by u-boot exception handler */
typedef struct octeon_trap_frame_s {
    trap_reg_t      tf_zero;     /* 0    */
    trap_reg_t      tf_ast;      /* 4    */
    trap_reg_t      tf_v0;       /* 8    */
    trap_reg_t      tf_v1;       /* 12   */
    trap_reg_t      tf_a0;       /* 16   */
    trap_reg_t      tf_a1;       /* 20   */
    trap_reg_t      tf_a2;       /* 24   */
    trap_reg_t      tf_a3;       /* 28   */
    trap_reg_t      tf_t0;       /* 32   */
    trap_reg_t      tf_t1;       /* 36   */
    trap_reg_t      tf_t2;       /* 40   */
    trap_reg_t      tf_t3;       /* 44   */
    trap_reg_t      tf_t4;       /* 48   */
    trap_reg_t      tf_t5;       /* 52   */
    trap_reg_t      tf_t6;       /* 56   */
    trap_reg_t      tf_t7;       /* 60   */
    trap_reg_t      tf_s0;       /* 64   */
    trap_reg_t      tf_s1;       /* 68   */
    trap_reg_t      tf_s2;       /* 72   */
    trap_reg_t      tf_s3;       /* 76   */
    trap_reg_t      tf_s4;       /* 80   */
    trap_reg_t      tf_s5;       /* 84   */
    trap_reg_t      tf_s6;       /* 88   */
    trap_reg_t      tf_s7;       /* 92   */
    trap_reg_t      tf_t8;       /* 96   */
    trap_reg_t      tf_t9;       /* 100  */
    trap_reg_t      tf_k0;       /* 104  */
    trap_reg_t      tf_k1;       /* 108  */
    trap_reg_t      tf_gp;       /* 112  */
    trap_reg_t      tf_sp;       /* 116  */
    trap_reg_t      tf_s8;       /* 120  */
    trap_reg_t      tf_ra;       /* 124  */
    trap_reg_t      tf_sr;       /* 128  */
    trap_reg_t      tf_mullo;    /* 132  */
    trap_reg_t      tf_mulhi;    /* 136  */
    trap_reg_t      tf_badvaddr; /* 140  */
    trap_reg_t      tf_cause;    /* 144  */
    trap_reg_t      tf_pc;       /* 148  */
} octeon_trap_frame_t;

/*
 * Vector for exception handling in u-boot. Just like boot 
 * vector described below, it is present at a fixed address
 * in DRAM. This is first in initialized by u-boot during
 * boot up. So if exception happens until when JUNOS hasn't
 * come up default exception handler of u-boot will run which
 * does very minimalistic stuff. When JUNOS comes up, it
 * reinitializes the vector to its own handlers. Since, NMI
 * is also a kind of exception, this handler can handle that
 * too.
 *
 * Only core 0 initializes this structure for u-boot and
 * it does this after relocation has happened.
 */
typedef struct uboot_exception_handler_info_s {
    uint32_t    exception_setup_addr;       /* address to code that sets up 
                                               stage for handler */
    uint32_t    exception_handler_addr;     /* actual handler which might do 
                                               fancy stuff */
    uint32_t    gd_addr;                    /* value of gd */
    uint32_t    gp_val;                     /* value of relocated gp */
    uint32_t    init_sp;                    /* initial stack pointer */
    uint32_t    reserved;                   /* for packing. */
} uboot_exception_handler_info_t;

/* This structure is access by assembly code in start.S, so any changes
** to content or size must be reflected there as well
** This is placed at a fixed address in DRAM, so that cores can access it
** when they come out of reset.  It is used to setup the minimal bootloader
** runtime environment (stack, but no heap, global data ptr) that is needed
** by the non-boot cores to setup the environment for the applications.
** The boot_info_addr is the address of a boot_info_block_t structure
** which contains more core-specific information.
**
*/
typedef struct
{
    uint32_t stack_addr;  /* Temp stack during bootstrap */
    uint32_t code_addr;   /* First stage address - in ram instead of flash */
    uint32_t app_start_func_addr;  /* setup code for application, NOT application entry point */
    uint32_t k0_val;        /* k0 is used for global data - needs to be passed to other cores */
    uint32_t flags;         /* flags */
    uint32_t boot_info_addr;    /* address of boot info block structure */
    uint32_t pad;
    uint32_t pad2; 
} boot_init_vector_t;


/* Bootloader internal structure for per-core boot information 
** This is used in addition the boot_init_vector_t structure to store
** information that the cores need when starting an application.
** This structure is only used by the bootloader.
** The boot_desc_addr is the address of the boot descriptor
** block that is used by the application startup code.  This descriptor
** used to contain all the information that was used by the application - now
** it just used for argc/argv, and heap.  The remaining fields are deprecated
** and have been moved to the cvmx_descriptor, as they are all simple exec related.
**
** The boot_desc_addr and cvmx_desc_addr addresses are physical for linux, and virtual
** for simple exec applications.
*/
typedef struct
{
    octeon_tlb_entry_t_bl tlb_entries[NUM_OCTEON_TLB_ENTRIES];
    uint64_t entry_point;
    uint64_t boot_desc_addr;  /* address in core's final memory map */
    uint64_t stack_top;
    uint32_t exception_base;
    uint64_t cvmx_desc_addr;  /* address in core's final memory map */
    uint64_t flags;
} boot_info_block_t;

#define BOOT_INFO_FLAG_DISABLE_ICACHE_PREFETCH  1

/* In addition to these two structures, there are also two more:
** 1) the boot_descriptor, which is used by the toolchain's crt0
**    code to setup the stack, head, and command line arguments
**    for the application.
** 2) the cvmx_boot_descriptor, which is used to pass additional
**    information to the simple exec.
*/



/************************************************************************/
#define  OCTEON_SPINLOCK_UNLOCKED_VAL  0
typedef struct {
    volatile unsigned int value;
} cvmx_spinlock_t;
static inline void cvmx_spinlock_unlock(cvmx_spinlock_t *lock)
{
    OCTEON_SYNC;
    lock->value = 0;
    OCTEON_SYNCW;
}
static inline void cvmx_spinlock_lock(cvmx_spinlock_t *lock)
{
    unsigned int tmp;

    __asm__ __volatile__(
    ".set push         \n"
    ".set noreorder         \n"
    "1: ll   %[tmp], %[val]  \n"
    "   bnez %[tmp], 1b     \n"
    "   li   %[tmp], 1      \n"  
    "   sc   %[tmp], %[val] \n"
    "   beqz %[tmp], 1b     \n"
    "   sync                \n"
    ".set pop           \n"
    : [val] "+m" (lock->value), [tmp] "=&r" (tmp)
    :
    : "memory");

}
/************************************************************************/

/* Default stack and heap sizes, in bytes */
#define DEFAULT_STACK_SIZE              (1*1024*1024)
#define DEFAULT_HEAP_SIZE               (3*1024*1024)


#define ALIGN_MASK(x)   ((~0ULL) << (x))
#define TLB_PAGE_SIZE_FROM_EXP(x)   ((1UL) << (x))
#define ALIGN_ADDR_DOWN(addr, align)   ((addr) & (align))
#define MAKE_XKPHYS(x)       ((1ULL << 63) | (x))
#define MAKE_KSEG0(x)       ((1ULL << 31) | (x))


/* Returns the Octeon submodel name of the chip. */
static inline char *octeon_get_submodel_name(void)
{
    if (!cvmx_octeon_dfa_present() && !cvmx_octeon_zip_present() && !cvmx_octeon_crypto_present())
        return("CP");
    else if (!cvmx_octeon_dfa_present() && !cvmx_octeon_zip_present())
        return("SCP");
    else if (!cvmx_octeon_crypto_present())
        return("EXP");
    else
        return("NSP");
}
static inline char *octeon_get_model_name(void)
{
    if (octeon_is_model(OCTEON_CN3010))
        return("CN3010");
    else if (octeon_is_model(OCTEON_CN3005))
        return("CN3005");
    else if (octeon_is_model(OCTEON_CN3020))
        return("CN3020");
    else if (octeon_is_model(OCTEON_CN31XX))
        return("CN31XX");
    else if (octeon_is_model(OCTEON_CN58XX))
        return("CN58XX");
    else if (octeon_is_model(OCTEON_CN56XX))
        return("CN56XX");
    else if (octeon_is_model(OCTEON_CN52XX))
        return("CN52XX");
    else if (octeon_is_model(OCTEON_CN50XX))
        return("CN50XX");
    else if (octeon_is_model(OCTEON_CN38XX))
    {
        if (cvmx_octeon_model_CN36XX())
            return("CN36XX");
        else
            return("CN38XX");
    }
    else
        return("UNKNOWN");
}




#endif /* __OCTEON_BOOT_H__ */
