/*
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 * Copyright 2004,2005 Cavium Networks
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

#include <watchdog_cpu.h>
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <devices.h>
#include <net.h>
#include <environment.h>
#include <image.h>
#include "octeon_boot.h"
#include <lib_octeon.h>
#include <watchdog.h>
#include <octeon_eeprom_types.h>
#include <lib_octeon_shared.h>
#include <jsrxnle_ddr_params.h>
#include "octeon-pci-console.h"
#include "cvmx-bootmem.h"
#include "octeon-model.h"

#ifdef CONFIG_POST
#include <post.h>
#endif
#if CONFIG_JSRXNLE
#include <platform_srxsme.h>
#endif

#if (!defined(ENV_IS_EMBEDDED) ||  defined(CFG_ENV_IS_IN_NVRAM))
#define	TOTAL_MALLOC_LEN	(CFG_MALLOC_LEN + CFG_ENV_SIZE)
#else
#define	TOTAL_MALLOC_LEN	CFG_MALLOC_LEN
#include <jsrxnle.h>
#endif


#define STACK_SIZE  (16*1024UL + CFG_BOOTPARAMS_LEN)
#define DRAM_LATE_ZERO_OFFSET  0x100000ull
#define OCTEON_MEMSIZE_2GB	  2048 /* Mb */
#define OCTEON_MEMSIZE_512MB   512 /* 512 Mb */

#define DEBUG_PRINTF 
 
int usb_scan_dev;

/* Global variable use to locate pci console descriptor */
uint64_t pci_cons_desc_addr;

extern int timer_init(void);
#if CONFIG_JSRXNLE
extern void pcie_init(void);
extern void ethsetup(void);
extern void boot_variables_setup(void);
extern void srxsme_bootseq_init(void);
extern uint8_t boot_sector_flag;
extern int recover_primary (void);
#endif
#ifdef CONFIG_MAG8600
extern int msc_board_phy_rst(void);
#endif

extern int incaip_set_cpuclk(void);

extern ulong uboot_end_data;
extern ulong uboot_end;

ulong monitor_flash_len;
#define SET_K0(val)                 asm volatile ("add $26, $0, %[rt]" : : [rt] "d" (val):"memory")

extern const char version_string[];
extern const char bootver_env_string[];
extern const char uboot_api_ver_string[];

static char *failed = "*** failed ***\n";

/*
 * Begin and End of memory area for malloc(), and current "brk"
 */
static ulong mem_malloc_start;
static ulong mem_malloc_end;
static ulong mem_malloc_brk;

/* Used for error checking on load commands */
ulong load_reserved_addr = 0; /* Base address of reserved memory for loading */
ulong load_reserved_size = 0; /* Size of reserved memory for loading */

/**
 * Get clock rate based on the clock type.
 *
 * @param clock - Enumeration of the clock type.
 * @return      - return the clock rate.
 */
uint64_t cvmx_clock_get_rate(cvmx_clock_t clock)
{
    const uint64_t REF_CLOCK = 50000000;
    uint64_t rate_eclk = 0;
    uint64_t rate_sclk = 0;
    uint64_t rate_dclk = 0;

    if (cvmx_unlikely(!rate_eclk))
    {
        if (OCTEON_IS_MODEL(OCTEON_CN56XX) || OCTEON_IS_MODEL(OCTEON_CN52XX))
        {
            cvmx_npei_dbg_data_t npei_dbg_data;
            npei_dbg_data.u64 = cvmx_read_csr(CVMX_PEXP_NPEI_DBG_DATA);
            rate_eclk =  REF_CLOCK * npei_dbg_data.s.c_mul;
            rate_sclk = rate_eclk;
        }
        else if (OCTEON_IS_MODEL(OCTEON_CN56XX) || OCTEON_IS_MODEL(OCTEON_CN52XX) || OCTEON_IS_MODEL(OCTEON_CN63XX)) 
        {
            cvmx_mio_rst_boot_t mio_rst_boot;
            mio_rst_boot.u64 = cvmx_read_csr(CVMX_MIO_RST_BOOT);
            rate_eclk =  REF_CLOCK * mio_rst_boot.s.c_mul;
            rate_sclk = REF_CLOCK * mio_rst_boot.s.pnr_mul;
        }
        else if (OCTEON_IS_MODEL(OCTEON_CN71XX)) {
            cvmx_rst_boot_t rst_boot;
            rst_boot.u64 = cvmx_read_csr(CVMX_RST_BOOT);
            rate_eclk = REF_CLOCK * rst_boot.s.c_mul;
            rate_sclk = REF_CLOCK * rst_boot.s.pnr_mul;
        }
        else
        {
            cvmx_dbg_data_t dbg_data;
            dbg_data.u64 = cvmx_read_csr(CVMX_DBG_DATA);
            rate_eclk =  REF_CLOCK * dbg_data.s.c_mul;
            rate_sclk = rate_eclk;
        }
    }

    switch (clock)
    {
        case CVMX_CLOCK_SCLK:
        case CVMX_CLOCK_TIM:
        case CVMX_CLOCK_IPD:
            return rate_sclk;

        case CVMX_CLOCK_RCLK:
        case CVMX_CLOCK_CORE:
            return rate_eclk;

        case CVMX_CLOCK_DDR:
            return rate_dclk;
    }

    cvmx_dprintf("cvmx_clock_get_rate: Unknown clock type\n");
    return 0;
}

/*
 * The Malloc area is immediately below the monitor copy in DRAM
 */
static void mem_malloc_init (void)
{
	DECLARE_GLOBAL_DATA_PTR;

	ulong dest_addr = CFG_MONITOR_BASE + gd->reloc_off;

	mem_malloc_end = dest_addr;
	mem_malloc_start = dest_addr - TOTAL_MALLOC_LEN;
	mem_malloc_brk = mem_malloc_start;

#if !CONFIG_OCTEON_SIM
	memset ((void *) mem_malloc_start,
		0,
		mem_malloc_end - mem_malloc_start);
#endif
}

void *sbrk (ptrdiff_t increment)
{
	ulong old = mem_malloc_brk;
	ulong new = old + increment;

	if ((new < mem_malloc_start) || (new > mem_malloc_end)) {
		return (NULL);
	}
	mem_malloc_brk = new;
	return ((void *) old);
}


static int init_func_ram (void)
{
    DECLARE_GLOBAL_DATA_PTR;

    puts ("DRAM:  ");

#if CONFIG_OCTEON_SIM
    if ((gd->ram_size = (uint64_t)(*((uint32_t *)0x9ffffff8) * 1024 * 1024)) > 0) {
        print_size (gd->ram_size, "\n");
        return (0);
    }
#else
    if ((gd->ram_size) > 0) {
        print_size (gd->ram_size, "\n");
        return (0);
    }
#endif

    puts (failed);
    return (1);
}

static int display_banner(void)
{
    DECLARE_GLOBAL_DATA_PTR;
    printf ("\n\n%s\n\n", version_string);
    if( gd->flags & GD_FLG_BOARD_EEPROM_READ_FAILED ){
        printf("ID EEPROM VALUES MISMATCH ... assuming default board id 0x%x\n",
                gd->board_desc.board_type );
    }    
    return (0);
}

static void display_flash_config(ulong size)
{
	puts ("Flash: ");
	print_size (size, "\n");
}

#if !CONFIG_OCTEON_SIM
static const dimm_odt_config_t dimm_odt_cn31xx_1rank_config_table[] = {
    CN31XX_DRAM_ODT_1RANK_CONFIGURATION
};
static const dimm_odt_config_t dimm_odt_cn31xx_2rank_config_table[] = {
    CN31XX_DRAM_ODT_2RANK_CONFIGURATION
};
static const dimm_odt_config_t dimm_odt_cn38xx_1rank_config_table[] = {
    CN38XX_DRAM_ODT_1RANK_CONFIGURATION
};
static const dimm_odt_config_t dimm_odt_cn38xx_2rank_config_table[] = {
    CN38XX_DRAM_ODT_2RANK_CONFIGURATION
};
static const dimm_odt_config_t dimm_odt_cn58xx_1rank_config_table[] = {
    CN58XX_DRAM_ODT_1RANK_CONFIGURATION
};
static const dimm_odt_config_t dimm_odt_cn58xx_2rank_config_table[] = {
    CN58XX_DRAM_ODT_2RANK_CONFIGURATION
};
static const dimm_odt_config_t dimm_odt_cn50xx_1rank_config_table[] = {
    CN50XX_DRAM_ODT_1RANK_CONFIGURATION
};
static const dimm_odt_config_t dimm_odt_cn50xx_2rank_config_table[] = {
    CN50XX_DRAM_ODT_2RANK_CONFIGURATION
};
static const dimm_odt_config_t dimm_odt_cn56xx_1rank_config_table[] = {
    CN56XX_DRAM_ODT_1RANK_CONFIGURATION
};
static const dimm_odt_config_t dimm_odt_cn56xx_2rank_config_table[] = {
    CN56XX_DRAM_ODT_2RANK_CONFIGURATION
};
static const dimm_odt_config_t dimm_odt_cn52xx_1rank_config_table[] = {
    CN52XX_DRAM_ODT_1RANK_CONFIGURATION
};
static const dimm_odt_config_t dimm_odt_cn52xx_2rank_config_table[] = {
    CN52XX_DRAM_ODT_2RANK_CONFIGURATION
};


static int init_dram(void)
{
    DECLARE_GLOBAL_DATA_PTR;
    int memsize;
    uint32_t measured_ddr_hertz;
    uint32_t ddr_config_valid_mask;
    uint32_t cpu_hertz;
    char *eptr;
    uint32_t ddr_hertz;
    uint32_t ddr_ref_hertz;
    int ram_resident = 0;
    ddr_configuration_t ddr_configuration_array[2];
    uint32_t cpu_id = cvmx_get_proc_id();

    if (gd->flags & GD_FLG_RAM_RESIDENT)
        ram_resident = 1;

    memset(ddr_configuration_array, 0, sizeof(ddr_configuration_array));
    if (ram_resident && getenv("dram_size_mbytes"))
    {
        /* DRAM size has been based by the remote entity that configured
        ** the DRAM controller, so we use that size rather than re-computing
        ** by reading the DIMM SPDs. */
        memsize = simple_strtoul(getenv("dram_size_mbytes"), NULL, 0);
        printf("Using DRAM size from environment: %d MBytes\n", memsize);
        printf("%s board (%d) revision major:%d, minor:%d, serial #: %s\n",
               cvmx_board_type_to_string(gd->board_desc.board_type), gd->board_desc.board_type,
               gd->board_desc.rev_major, gd->board_desc.rev_minor,
               gd->board_desc.serial_str);
         int ddr_int = 0;
        if (OCTEON_IS_MODEL(OCTEON_CN56XX))
        {
            /* Check to see if interface 0 is enabled */
            cvmx_l2c_cfg_t l2c_cfg;
            l2c_cfg.u64 = cvmx_read_csr(CVMX_L2C_CFG);
            if (!l2c_cfg.s.dpres0)
            {
                if (l2c_cfg.s.dpres1)
                    ddr_int = 1;
                else
                    printf("ERROR: no DRAM controllers initialized\n");
            }
        }
        gd->ddr_clock_mhz = measure_octeon_ddr_clock(0,ddr_configuration_array,gd->cpu_clock_mhz*1000*1000,0,0,ddr_int,0)/1000000;
    }
    else
    {
        /* Check for special case of mismarked 3005 samples, and adjust cpuid */
        if (cpu_id == OCTEON_CN3010_PASS1 && (cvmx_read_csr(CVMX_L2D_FUS3) & (1ull << 34)))
            cpu_id |= 0x10;
        
    uint32_t ddr_hertz = gd->ddr_clock_mhz * 1000000;
    uint32_t cpu_hertz = gd->cpu_clock_mhz * 1000000;
    uint32_t ddr_ref_hertz = gd->ddr_ref_hertz;

    uint32_t tmp_sku_i2cid;
    char *s;

    if ((s = getenv("ddr_clock_hertz")) != NULL) {
        ddr_hertz = simple_strtoul(s, NULL, 0);
        gd->ddr_clock_mhz = ddr_hertz / 1000000;
        printf("Parameter found in environment. ddr_clock_hertz = %d\n", ddr_hertz);
    }

    if ((s = getenv("ddr_ref_hertz")) != NULL) {
        ddr_ref_hertz = simple_strtoul(s, NULL, 0);
        gd->ddr_ref_hertz = ddr_ref_hertz;
        printf("Parameter found in environment. ddr_ref_hertz = %d\n", ddr_ref_hertz);
    }

    
        const ddr_configuration_t *ddr_config_ptr;

#ifdef CONFIG_JSRXNLE
        /*
         * Treat ddr configuration structure the same for all SRX110
         * platform variants. Temporarily assume a generic SRX110 type.
        */
        if (IS_PLATFORM_SRX110(gd->board_desc.board_type)) {
            tmp_sku_i2cid = gd->board_desc.board_type;
            gd->board_desc.board_type = I2C_ID_SRXSME_SRX110B_VA_CHASSIS;
        }
#endif
        ddr_config_ptr = lookup_ddr_config_structure(cpu_id, 
                                                     gd->board_desc.board_type,
                                                     gd->board_desc.rev_major,
                                                     gd->board_desc.rev_minor,
                                                     &ddr_config_valid_mask);
        if (!ddr_config_ptr)
        {
            printf("ERROR: unable to determine DDR configuration for board type: %s (%d)\n", cvmx_board_type_to_string(gd->board_desc.board_type), gd->board_desc.
board_type);
            return(-1);
        } 

#ifdef CONFIG_JSRXNLE
        /* reverting to non-generic SRX110 board type */
        if (IS_PLATFORM_SRX110(gd->board_desc.board_type)) {
            gd->board_desc.board_type = tmp_sku_i2cid;
        }
#endif

        memsize = octeon_ddr_initialize(cpu_id,
                                        gd->sys_clock_mhz*1000000,
                                        ddr_hertz,
                                        ddr_ref_hertz,
                                        ddr_config_valid_mask,
                                        ddr_config_ptr,
                                        &measured_ddr_hertz,
                                        gd->board_desc.board_type,
                                        gd->board_desc.rev_major,
                                        gd->board_desc.rev_minor);
        
        gd->ddr_clock_mhz = measured_ddr_hertz/1000000;

        printf("Measured DDR clock %d.%02d MHz\n", measured_ddr_hertz / 1000000, (measured_ddr_hertz - ((int)(measured_ddr_hertz / 1000000)) * 1000000) / CALC_SCALER);

        if (measured_ddr_hertz != 0)
        {
            if (!gd->ddr_clock_mhz)
            {
                /* If ddr_clock not set, use measured clock and don't warn */
                gd->ddr_clock_mhz = measured_ddr_hertz/1000000;
            }
            else if ((measured_ddr_hertz > ddr_hertz + 3000000) || (measured_ddr_hertz <  ddr_hertz - 3000000))
            {
                printf("\nWARNING:\n");
                printf("WARNING: Measured DDR clock mismatch! expected: %d MHz, measured: %d MHz, cpu clock: %d MHz\n",
                       ddr_hertz/1000000, measured_ddr_hertz/1000000, gd->cpu_clock_mhz);
                if (!(gd->flags & GD_FLG_RAM_RESIDENT))
                    printf("WARNING: Using measured clock for configuration.\n");
                printf("WARNING:\n\n");
                gd->ddr_clock_mhz = measured_ddr_hertz/1000000;
            }
        }

        if (gd->flags & GD_FLG_CLOCK_DESC_MISSING)
        {
            printf("Warning: Clock descriptor tuple not found in eeprom, using defaults\n");
        }
        if (gd->flags & GD_FLG_BOARD_DESC_MISSING)
        {
            printf("Warning: Board descriptor tuple not found in eeprom, using defaults\n");
        }


    }

    printf("%s board revision major:%d, minor:%d, serial #: %s\n",
           cvmx_board_type_to_string(gd->board_desc.board_type),
           gd->board_desc.rev_major, gd->board_desc.rev_minor,
           gd->board_desc.serial_str);

    {
        char buffer[32];
        memset(buffer, 0, sizeof(buffer));
        octeon_model_get_string_buffer(cvmx_get_proc_id(), buffer);
        printf("OCTEON %s, Core clock: %d MHz, DDR clock: %d MHz (%d Mhz data rate)\n",
               buffer,
               gd->cpu_clock_mhz,
               gd->ddr_clock_mhz, gd->ddr_clock_mhz *2);
    }

#ifdef CONFIG_JSRXNLE
    switch(gd->board_desc.board_type) {
    case I2C_ID_JSRXNLE_SRX100_LOWMEM_CHASSIS:
        /*
         * if its newer SRX100 protos, DRAM is initialized
         * by 1GB settings. But we need to limit the size
         * to 512 MB again in case i2c ID is programmed as
         * low mem.
         */
        if (is_proto_id_supported(gd->board_desc.board_type)) {
            if (memsize > OCTEON_MEMSIZE_512MB) {
                memsize = OCTEON_MEMSIZE_512MB;
            }
        }
        break;
    }
#endif

    if (memsize > 0)
    {
        gd->ram_size = (uint64_t)memsize * 1024 * 1024;

#if !defined(CONFIG_NO_RELOCATION) /* If already running in memory don't zero it. */
    /* Zero the low Megabyte of DRAM used by bootloader.  The rest is zeroed later.
    ** when running from DRAM*/


#if !CONFIG_NO_CLEAR_DDR
        octeon_bzero64_pfs(0, DRAM_LATE_ZERO_OFFSET);
#endif

#endif /* !CONFIG_NO_RELOCATION */
        return(0);
    }
    else
        return -1;
}
#endif

static int init_baudrate (void)
{
	DECLARE_GLOBAL_DATA_PTR;

	char tmp[64];	/* long enough for environment variables */
	int i = getenv_r ("baudrate", tmp, sizeof (tmp));

    	gd->baudrate = (i > 0)
			? (int) simple_strtoul (tmp, NULL, 10)
			: CONFIG_BAUDRATE;

	return (0);
}

#define DO_SIMPLE_DRAM_TEST_FROM_FLASH  0
#if DO_SIMPLE_DRAM_TEST_FROM_FLASH
/* Very simple DRAM test to run from flash for cases when bootloader boot
** can't complete.
*/
static int dram_test(void)
{
    uint32_t *addr;
    uint32_t *start = (void *)0x80000000;
    uint32_t *end = (void *)0x80200000;
    uint32_t val, incr, readback, pattern;
    int error_limit;

    pattern = 0;
    printf("Performing simple DDR test from flash.\n");

    #define DRAM_TEST_ERROR_LIMIT 200

    incr = 1;
    for (;;) {
        error_limit = DRAM_TEST_ERROR_LIMIT;

        printf ("\rPattern %08lX  Writing..."
                "%12s"
                "\b\b\b\b\b\b\b\b\b\b",
                pattern, "");

        for (addr=start,val=pattern; addr<end; addr++) {
                *addr = val;
                val  += incr;
        }

        puts ("Reading...");

        for (addr=start,val=pattern; addr<end; addr++) {
                readback = *addr;
                if (readback != val && error_limit-- > 0) {
                    if (error_limit + 1 == DRAM_TEST_ERROR_LIMIT)
                        puts("\n");
                    printf ("Mem error @ 0x%08X: "
                            "found %08lX, expected %08lX\n",
                            (uint)addr, readback, val);
                }
                val += incr;
        }

        /*
         * Flip the pattern each time to make lots of zeros and
         * then, the next time, lots of ones.  We decrement
         * the "negative" patterns and increment the "positive"
         * patterns to preserve this feature.
         */
        if(pattern & 0x80000000) {
                pattern = -pattern;	/* complement & increment */
        }
        else {
                pattern = ~pattern;
        }
        if (error_limit <= 0)
        {
            printf("Too many errors, printing stopped\n");
        }
        incr = -incr;
    }

}
#endif
/*
 * Breath some life into the board...
 *
 * The first part of initialization is running from Flash memory;
 * its main purpose is to initialize the RAM so that we
 * can relocate the monitor code to RAM.
 */

/*
 * All attempts to come up with a "common" initialization sequence
 * that works for all boards and architectures failed: some of the
 * requirements are just _too_ different. To get rid of the resulting
 * mess of board dependend #ifdef'ed code we now make the whole
 * initialization sequence configurable to the user.
 *
 * The requirements for any new initalization function is simple: it
 * receives a pointer to the "global data" structure as it's only
 * argument, and returns an integer return code, where 0 means
 * "continue" and != 0 means "fatal error, hang the system".
 */

int early_board_init(void);
int octeon_boot_bus_init_board(void);
#if CFG_LATE_BOARD_INIT
int late_board_init(void);
#endif

typedef int (init_fnc_t) (void);

init_fnc_t *init_sequence[] = {
#if CONFIG_OCTEON && (!CONFIG_OCTEON_SIM)
#if CONFIG_BOOT_BUS_BOARD
    octeon_boot_bus_init_board,  // board specific boot bus init
#else
    octeon_boot_bus_init,  // need to map PAL to read clocks, speed up flash
#endif
#endif
	timer_init,
	env_init,		/* initialize environment */
#if CONFIG_OCTEON
    early_board_init,
#endif
	init_baudrate,		/* initialze baudrate settings */
	serial_init,		/* serial communications setup */
	console_init_f,
	display_banner,		/* say that we are here */
#if !CONFIG_OCTEON_SIM
        init_dram,
#endif
#if DO_SIMPLE_DRAM_TEST_FROM_FLASH
        dram_test,
#endif
#ifdef CONFIG_POST
        post_init_f,
#endif
	checkboard,
	init_func_ram,
#ifdef CONFIG_MAG8600
        msc_board_phy_rst,
#endif
	NULL,
};

void board_init_f(ulong bootflag)
{
	DECLARE_GLOBAL_DATA_PTR;

	gd_t gd_data, *id;
	bd_t *bd;
	init_fnc_t **init_fnc_ptr;
	ulong len = (ulong)&uboot_end - CFG_MONITOR_BASE;
	ulong addr, addr_sp;
	uint32_t u_boot_mem_top; /* top of memory, as used by u-boot */

        int init_func_count = 0;

        /* Offsets from u-boot base for stack/heap.  These
        ** offsets will be the same for both physical and
        ** virtual maps. */
        SET_K0(&gd_data);
        /* Pointer is writable since we allocated a register for it.  */
	memset ((void *)gd, 0, sizeof (gd_t));
        gd->baudrate = CONFIG_BAUDRATE;
        /* Round u-boot length up to nice boundary */

        gd->cpu_clock_mhz = cvmx_clock_get_rate(CVMX_CLOCK_CORE)/1000000;
        gd->sys_clock_mhz = cvmx_clock_get_rate(CVMX_CLOCK_SCLK)/1000000;

        /*early init of uart */
        octeon_uart_setup(0);

        len = (len + 0xFFFF) & ~0xFFFF;
        /* Enable soft BIST so that BIST is run on soft resets. */
        if (!OCTEON_IS_MODEL(OCTEON_CN38XX_PASS2) && !OCTEON_IS_MODEL(OCTEON_CN31XX))
            cvmx_write_csr(CVMX_CIU_SOFT_BIST, 1);

        /* Change default setting for CN58XX pass 2 */
        if (OCTEON_IS_MODEL(OCTEON_CN58XX_PASS2))
            cvmx_write_csr(CVMX_MIO_FUS_EMA, 2);

#ifdef ENABLE_BOARD_DEBUG
        int gpio_status_data = 1;
#endif
	for (init_fnc_ptr = init_sequence; *init_fnc_ptr; ++init_fnc_ptr) {
            init_fnc_t* fnc = (init_fnc_t *)((uint32_t)(*init_fnc_ptr));
#ifdef ENABLE_BOARD_DEBUG
                gpio_status(gpio_status_data++);
#endif
                if ((*fnc)() != 0) {
                        printf("hanging, init func: %d\n", init_func_count);
                        hang ();
                }
                init_func_count++;
        }

    /* Disable Fetch under fill on CN63XXp1 due to errata Core-14317 */
    if (OCTEON_IS_MODEL(OCTEON_CN63XX_PASS1_X))
        {
            uint64_t val;
            val = get_cop0_cvmctl_reg();
            val |= (1ull << 19); /*CvmCtl[DEFET] */
            set_cop0_cvmctl_reg(val);
        }
    
#if CONFIG_JSRXNLE
    /*
     * Reset the CF. After reset, it takes about 600ms to become operational.
     * By resetting at an early stage here, and then performing ATA init
     * at a later stage, we get some useful work done without introducing
     * the 600ms delay.
     */
    switch( gd->board_desc.board_type ) {
    CASE_ALL_SRXLE_CF_MODELS
        /* don't need this for SRX550. we use a different controller */
        if(IS_PLATFORM_SRX550(gd->board_desc.board_type)) {
            break;
        }
	/* Turn it off */
	srxle_cf_disable();
	/* Wait 100 ms */
	udelay(100000);
	/* Turn it on */
	srxle_cf_enable();
        /* NOTE: 600ms delay needed before accessing */
        break;
    default:
        /* Nothing to do */
        break;
    }
#endif


#if CONFIG_POST && CONFIG_JSRXNLE
	if (post_run ("memory", (POST_ROM | POST_ALWAYS)) != 0) {
	    printf("Memory POST Failed!");
	}
        /* run other ROM-runnable post now */
        post_run("eeprom", (POST_ROM | POST_ALWAYS));
        
        /* Log results to flash */
        post_log_result_flash();
#endif

#if CONFIG_POST && CONFIG_ACX500_SVCS
	if (post_run ("memory", (POST_ROM | POST_ALWAYS)) != 0) {
	    printf("Memory POST Failed!");
	}
#endif

	/*
	 * Now that we have DRAM mapped and working, we can
	 * relocate the code and continue running from DRAM.
	 */
#if defined(CONFIG_NO_RELOCATION)
        /* If loaded into ram, we can skip relocation , and run from the spot we were loaded into */
	addr = CFG_MONITOR_BASE;
	u_boot_mem_top = CFG_SDRAM_BASE + MIN(gd->ram_size, (1*1024*1024));
#else
#if defined(CONFIG_JSRXNLE) && defined(RELOCATE_HIGH)
    /* Locate to almost top of 256M */
    addr = CFG_SDRAM_BASE + JSRXNLE_RELOCATE_TOP;
    memset((void *)(CFG_SDRAM_BASE + JSRXNLE_RELOCATE_BOT), 0,
            JSRXNLE_RELOCATE_RESERVE_LEN);
#else
	/* Locate at top of first Megabyte */
	addr = CFG_SDRAM_BASE + MIN(gd->ram_size, (1*1024*1024));
#endif
	u_boot_mem_top = addr;

		/* We can reserve some RAM "on top" here.
		 */
	
	DEBUG_PRINTF("U-boot link addr: 0x%x\n", CFG_MONITOR_BASE);
		/* round down to next 4 kB limit.
		 */
	addr &= ~(4096 - 1);

	DEBUG_PRINTF ("Top of RAM usable for U-Boot at: %08x\n", addr);

	/* Reserve memory for U-Boot code, data & bss
	 */
	addr -= MAX(len, (512*1024));
    /* round down to next 64k (allow us to create image at same addr for debugging)*/
	addr &= ~(64 * 1024 - 1);
	
	DEBUG_PRINTF ("Reserving %ldk for U-Boot at: %08x\n", MAX(len, (512*1024)) >> 10, addr);
	
#endif	// CONFIG_NO_RELOCATION
	
	
	/* Reserve memory for malloc() arena, at lower address than u-boot code/data
	 */
	
	addr_sp = addr - TOTAL_MALLOC_LEN;

	DEBUG_PRINTF ("Reserving %dk for malloc() at: %08x\n",
					TOTAL_MALLOC_LEN >> 10, addr_sp);

	/*
	 * (permanently) allocate a Board Info struct
	 * and a permanent copy of the "global" data
	 */
	addr_sp -= sizeof(bd_t);
	bd = (bd_t *)addr_sp;
	gd->bd = bd;

	DEBUG_PRINTF ("Reserving %d Bytes for Board Info at: %08x\n",
					sizeof(bd_t), addr_sp);
	addr_sp -= sizeof(gd_t);
	id = (gd_t *) addr_sp;
	DEBUG_PRINTF ("Reserving %d Bytes for Global Data at: %p\n",
					sizeof (gd_t), id);
	 /* Reserve memory for boot params.
			 */
	addr_sp -= CFG_BOOTPARAMS_LEN;
	bd->bi_boot_params = addr_sp;
	DEBUG_PRINTF ("Reserving %dk for boot params from stack at: %08x\n",
					CFG_BOOTPARAMS_LEN >> 10, addr_sp);

	/*
	 * Finally, we set up a new (bigger) stack.
	 *
	 * Leave some safety gap for SP, force alignment on 16 byte boundary
	 * Clear initial stack frame
	 */
	addr_sp -= 16;
	addr_sp &= ~0xF;

    /* Changed to avoid lvalue cast warnings */
    uint32_t *tmp_ptr = (void *)addr_sp;
    *tmp_ptr-- = 0;
    *tmp_ptr-- = 0;
    addr_sp = (uint32_t)addr_sp;

#define STACK_SIZE  (16*1024UL)
    bd->bi_uboot_ram_addr = (addr_sp - STACK_SIZE) & ~(STACK_SIZE - 1);
    bd->bi_uboot_ram_used_size = u_boot_mem_top - bd->bi_uboot_ram_addr;

	DEBUG_PRINTF("Stack Pointer at: %08x, stack size: 0x%08x\n", addr_sp, STACK_SIZE);
	DEBUG_PRINTF("Base DRAM address used by u-boot: 0x%08x, size: 0x%x\n", bd->bi_uboot_ram_addr, bd->bi_uboot_ram_used_size);

    /*
	 * Save local variables to board info struct
	 */
	bd->bi_memstart	= CFG_SDRAM_BASE;	/* start of  DRAM memory */
	bd->bi_memsize	= gd->ram_size;		/* size  of  DRAM memory in bytes */
	bd->bi_baudrate	= gd->baudrate;		/* Console Baudrate */
#if defined(CONFIG_JSRXNLE) || defined(CONFIG_ACX500_SVCS)
	bd->bi_i2c_id	= (uint32_t)gd->board_desc.board_type;	/* board i2c_id */
#endif /* CONFIG_JSRXNLE || CONFIG_ACX500_SVCS */

	memcpy (id, (void *)gd, sizeof (gd_t));

         DEBUG_PRINTF("relocating and jumping to code in DRAM at addr: 0x%x\n", addr);

		/* For non-tlb mode, we need to adjust some parameters */
		DEBUG_PRINTF("Stack top: 0x%x, new gd: %p, addr: 0x%x\n", 
					 (uint32_t)addr_sp, id, addr);

	relocate_code (addr_sp, id, addr);

		DEBUG_PRINTF("Relocate code done: %d %s\n", __LINE__, __FUNCTION__);

	/* NOTREACHED - relocate_code() does not return */
}

/************************************************************************
 *
 * This is the next part if the initialization sequence: we are now
 * running from RAM and have a "normal" C environment, i. e. global
 * data can be written, BSS has been cleared, the stack size in not
 * that critical any more, etc.
 *
 ************************************************************************
 */

void board_init_r (gd_t *id, ulong dest_addr)
{
    DECLARE_GLOBAL_DATA_PTR;
    uint32_t board_type;
    cmd_tbl_t *cmdtp;
    ulong size = 0;
    extern void malloc_bin_reloc (void);
#ifndef CFG_ENV_IS_NOWHERE
    extern char * env_name_spec;
#endif
    char *s, *e;
    bd_t *bd;
    int i;
#ifdef CONFIG_JSRXNLE
    uint8_t cpld_reg_val;
    uint8_t bootdev;
    int res;
    int val;
#endif

/* Convince GCC to really do what we want, otherwise the old value is still used......*/
    SET_K0(id);

    board_type = gd->board_desc.board_type;

    gd->flags |= GD_FLG_RELOC;  /* tell others: relocation done */

#ifdef DEBUG
    /* This is the address that u-boot has relocated itself to.  Use this 
    ** address for RELOCATED_TEXT_BASE in the makefile to enable the relocated
    ** build for debug info. */
    printf ("Now running in RAM - U-Boot at: 0x%x\n", dest_addr);
#endif

    gd->reloc_off = dest_addr - CFG_MONITOR_BASE;

    monitor_flash_len = (ulong)&uboot_end_data - dest_addr;

    /*
     * We have to relocate the command table manually
     */
    if (gd->reloc_off)
    {
        for (cmdtp = &__u_boot_cmd_start; cmdtp !=  &__u_boot_cmd_end; cmdtp++)
        {
            ulong addr;

            addr = (ulong) (cmdtp->cmd) + gd->reloc_off;
            cmdtp->cmd =
            (int (*)(struct cmd_tbl_s *, int, int, char *[]))addr;

            addr = (ulong)(cmdtp->name) + gd->reloc_off;
            cmdtp->name = (char *)addr;

            if (cmdtp->usage)
            {
                addr = (ulong)(cmdtp->usage) + gd->reloc_off;
                cmdtp->usage = (char *)addr;
            }
#ifdef	CFG_LONGHELP
            if (cmdtp->help)
            {
                addr = (ulong)(cmdtp->help) + gd->reloc_off;
                cmdtp->help = (char *)addr;
            }
#endif
        }
        /* there are some other pointer constants we must deal with */
#ifndef CFG_ENV_IS_NOWHERE
        env_name_spec += gd->reloc_off;
#endif

    }  /* if (gd-reloc_off) */

    bd = gd->bd;

    /* 
     * We have spent some time in relocation,
     * code copying etc. Pat watchdog once here
     * to tell we are still alive.
     */
    RELOAD_CPLD_WATCHDOG(PAT_WATCHDOG);

#if defined(CONFIG_POST) && defined(CONFIG_JSRXNLE)
    /* relocate our post test table too */
    post_reloc();
   
    printf("Running U-Boot CRC Test... ");

    post_run("u-boot-crc", (POST_RAM | POST_NORMAL));
    
    if (gd->post_result & UBOOT_CRC_POST_RESULT_MASK) {
	switch (board_type) {
	CASE_ALL_SRXLE_MODELS
	    /* If we are running from primary copy and its
	     * CRC is wrong, we will stop here an let
	     * backup u-boot trigger.
	     *
	     * If we are already in backup, its better
	     * to let go and pray for the best.
	     */
	    cpld_reg_val = cpld_read(SRX210_CPLD_MISC_REG);
	    if (!(cpld_reg_val & SRX210_CPLD_BOOT_SEC_SEL_MASK)) {
		printf("Waiting for backup u-boot to trigger.\n");
		for(;;);
	    }

	    break;

	default:
	    /* 
	     * For platforms that don't have backup (SRX650).
	     * we let go and pray for the best 
	     */
	    break;
	}
    } else {
	printf("OK.\n");
    }
#endif
#ifdef CONFIG_JSRXNLE
    switch( board_type ) {
    CASE_ALL_SRXSME_4MB_FLASH_MODELS

        /* Reset all the device on srx210 and srx240 platforms
         * using Board Reset_N n cpld as some device need this
         */
        cpld_reg_val = cpld_read(SRX210_CPLD_RESET_REG);
        cpld_write(SRX210_CPLD_RESET_REG, (cpld_reg_val &
                                        ~SRX210_CPLD_RESET_DEV_MASK));
        udelay(1000);
        cpld_write(SRX210_CPLD_RESET_REG, (cpld_reg_val));
        
        bd->bi_flashstart = CFG_FLASH_BASE;
        cpld_reg_val = cpld_read(SRXNLE_CPLD_MISC2_REG);
        cpld_write(SRXNLE_CPLD_MISC2_REG, (cpld_reg_val |
                                           SRXNLE_CPLD_ENABLE_FLASH_WRITE));
        break;

    /*
     * SRX220 deviates from other ASGARD family platforms as it
     * has 8MB boot flash.
     */
    CASE_ALL_SRXSME_8MB_FLASH_MODELS
        bd->bi_flashstart = CFG_8MB_FLASH_BASE;
    break;

    default:
        printf("error : invalid board\n");
    }

    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX110_MODELS
        /* Bring VIA PCI-to-USB chip out of reset */
        val = cpld_read(SRX110_DEVICE_RESET_REG);
        val |=  SRX110_PCI_USB_RESET_BIT;
        cpld_write(SRX110_DEVICE_RESET_REG, val);
        break;
    }
#endif

    size = flash_init();
    display_flash_config (size);

    bd->bi_flashsize = size;

#if CFG_MONITOR_BASE == CFG_FLASH_BASE
    bd->bi_flashoffset = monitor_flash_len; /* reserved area for U-Boot */
#else
    bd->bi_flashoffset = 0;
#endif

    /* initialize malloc() area */
    mem_malloc_init();
    malloc_bin_reloc();

    /* relocate environment function pointers etc. */
    env_relocate();

    /* 
     * Clear any previous request or status
     * if its set in bootflash
     */
    INIT_PLATFORM_BOOT_STATUS();

    SET_PLATFORM_BOOT_STATUS(BS_UBOOT_BOOTING);

#ifdef CONFIG_JSRXNLE
        /*
         * check did we boot from backup u-boot, if so make
         * enire bootflash accessiable in backup u-boot by
         * using cpld.By doing this way we can avoid checks
         * in other places when dealing with bootflash read
         * and writes
         */

    switch( board_type ) {
    CASE_ALL_SRXLE_MODELS
        cpld_reg_val = cpld_read(SRX210_CPLD_MISC_REG);
        if (cpld_reg_val & SRX210_CPLD_BOOT_SEC_SEL_MASK) {
            printf("WARNING: Running from backup u-boot\n");
            enableall_sectors();
	    setenv("boot.current", "secondary");
	    res = recover_primary();
	    if (res < 0) {
		printf("WARNING: Failed to auto-recover primary U-Boot.\n"
		       "Please recover it manually as soon as possible.\n");
	    } else if (res == 0) {
		printf("Recovered primary U-Boot from secondary, please it upgrade"
		       " to latest version.\n");
	    }
        } else {
	    setenv("boot.current", "primary");
	}
        break;
    CASE_ALL_SRX650_MODELS
	/* 
	 * SRX650 has only primary u-boot. So set it to
	 * primary in case bootflash has some other value.
	 */
	setenv("boot.current", "primary");
        break;

        default:
            printf("error : invalid board\n");
        }
#endif

#if !CONFIG_OCTEON_SIM
    if (octeon_is_model(OCTEON_CN38XX))
    {
        extern int octeon_ipd_bp_enable(void);
        extern int octeon_ipd_bp_verify(void);
        int loops = octeon_ipd_bp_enable();
        if (loops < 40)
        {
            if (octeon_ipd_bp_verify())
                printf("IPD backpressure workaround verified, took %d loops\n", loops);
            else
                printf("IPD backpressure workaround FAILED verfication. Adjust octeon_ipd_bp_enable() for this chip!\n");
        }
        else
            printf("IPD backpressure workaround FAILED. Adjust octeon_ipd_bp_enable() for this chip!\n");
    }


    /* Disable the watchdog timers, as these are still running after a soft reset as a PCI target */
    cvmx_write_csr(CVMX_CIU_WDOGX(0), 0);
    cvmx_write_csr(CVMX_CIU_WDOGX(1), 0);
    if (octeon_is_model(OCTEON_CN38XX) || octeon_is_model(OCTEON_CN58XX))
    {
        for (i = 2; i < 16;i++)
            cvmx_write_csr(CVMX_CIU_WDOGX(i), 0);
    }
#if !CONFIG_L2_ONLY
    DEBUG_PRINTF("About to flush L2 cache\n");
    /* Flush and unlock L2 cache (soft reset does not unlock locked lines) */
    cvmx_l2c_flush();
#endif
#endif
    /* initialize the USB */
    /* If POST is enabled, the usb post test
     * would expect the USB to be already
     * initialized
     */

#if !(CONFIG_RAM_RESIDENT)
#if defined (CFG_CMD_USB)
    if (usb_init() != -1)
        usb_stor_scan(1);
#endif
#endif
#if !CONFIG_OCTEON_SIM && !defined(CONFIG_NO_RELOCATION) && !defined(CONFIG_NO_CLEAR_DDR)
    /* Zero all of memory above DRAM_LATE_ZERO_OFFSET */
    printf("Clearing DRAM.....");
#if defined(CONFIG_JSRXNLE) && defined(RELOCATE_HIGH)
    memset((void *)0x80100000, 0, (JSRXNLE_RELOCATE_BOT - 0x100000));
#else
    memset((void *)0x80100000, 0, 255*1024*1024);
#endif
    puts(".");

    if (octeon_is_model(OCTEON_CN63XX))
    {
        if (gd->ram_size > 256*1024*1024)
        {
            octeon_bzero64_pfs(0x20000000ull, (uint64_t)(gd->ram_size - 256*1024*1024));
        }
    } else {

        if (gd->ram_size > 256*1024*1024)
        {
            octeon_bzero64_pfs(0x8000000410000000ull, (uint64_t)MIN((gd->ram_size - 256*1024*1024), 256*1024*1024));
            puts(".");
        }

        if (gd->ram_size > 512*1024*1024)
        {
            octeon_bzero64_pfs(0x8000000020000000ull, gd->ram_size - 512*1024*1024ull);
            puts(".");
        }
    }
    printf(" done\n");
#endif

    /* Clear DDR/L2C ECC error bits */
    if (octeon_is_model(OCTEON_CN63XX))
    {
        cvmx_write_csr(CVMX_L2C_INT_REG, cvmx_read_csr(CVMX_L2C_INT_REG));
        cvmx_write_csr(CVMX_L2C_ERR_TDTX(0), cvmx_read_csr(CVMX_L2C_ERR_TDTX(0)));
        cvmx_write_csr(CVMX_L2C_ERR_TTGX(0), cvmx_read_csr(CVMX_L2C_ERR_TTGX(0)));
    } else if (octeon_is_model(OCTEON_CN71XX)) {
        cvmx_write_csr(CVMX_L2C_TADX_INT(0), cvmx_read_csr(CVMX_L2C_TADX_INT(0)));
    } else {
        cvmx_write_csr(CVMX_L2D_ERR, cvmx_read_csr(CVMX_L2D_ERR));
        cvmx_write_csr(CVMX_L2T_ERR, cvmx_read_csr(CVMX_L2T_ERR));
    }
    if (octeon_is_model(OCTEON_CN56XX)) {
        cvmx_l2c_cfg_t l2c_cfg;
        l2c_cfg.u64 = cvmx_read_csr(CVMX_L2C_CFG);
        if (l2c_cfg.cn56xx.dpres0) {
            cvmx_write_csr(CVMX_LMCX_MEM_CFG0(0), cvmx_read_csr(CVMX_LMCX_MEM_CFG0(0)));
        }
        if (l2c_cfg.cn56xx.dpres1) {
            cvmx_write_csr(CVMX_LMCX_MEM_CFG0(1), cvmx_read_csr(CVMX_LMCX_MEM_CFG0(1)));
        }
    } else if (octeon_is_model(OCTEON_CN63XX) || octeon_is_model(OCTEON_CN71XX)) {
        cvmx_write_csr(CVMX_LMCX_INT(0), cvmx_read_csr(CVMX_LMCX_INT(0)));
    } else {
        cvmx_write_csr(CVMX_LMC_MEM_CFG0, cvmx_read_csr(CVMX_LMC_MEM_CFG0));
    }

    if (!cvmx_bootmem_phy_mem_list_init(gd->ram_size, OCTEON_RESERVED_LOW_MEM_SIZE, malloc(sizeof(cvmx_bootmem_desc_t))))
    {
        printf("FATAL: Error initializing free memory list\n");
        while (1);
    }

#if defined(CONFIG_JSRXNLE) && defined(RELOCATE_HIGH)
    if (0 > cvmx_bootmem_phy_alloc((JSRXNLE_RESERVE_TOP - JSRXNLE_RESERVE_BOT),
                JSRXNLE_RESERVE_BOT, JSRXNLE_RESERVE_TOP, 0, 0)) {
        printf("FATAL: Error reserve relocate memory\n");
        while (1);
    }
#endif

    /* setup cvmx_sysinfo structure so we can use simple executive functions */
    octeon_setup_cvmx_sysinfo();

#ifdef DEBUG
    cvmx_bootmem_phy_list_print();
#endif

#if !CONFIG_OCTEON_SIM || defined(CONFIG_NET_MULTI) || defined(OCTEON_RGMII_ENET) || defined(OCTEON_MGMT_ENET) 
    {
        char *eptr;
        uint32_t addr;
        uint32_t size;

        eptr = getenv("octeon_reserved_mem_load_size");

        if (!eptr || !strcmp("auto", eptr))
        {
            /* Pick a size that we think is appropriate.
            ** Please note that for small memory boards this guess will likely not be ideal.
            ** Please pick a specific size for boards/applications that require it. */
            size = MIN(96*1024*1024, (gd->ram_size/3) & ~0xFFFFF);
        }
        else
        {
            size = simple_strtol(eptr,NULL, 16);
        }
        if (size)
        {
            eptr = getenv("octeon_reserved_mem_load_base");
            if (!eptr || !strcmp("auto", eptr))
            {
                uint64_t mem_top;
                /* Leave some room for previous allocations that are made starting at the top of the low
                ** 256 Mbytes of DRAM */
                int adjust = 1024*1024;
                /* Put block at the top of DDR0, or bottom of DDR2 */
                if (gd->ram_size <= 512*1024*1024 || (size > gd->ram_size - 512*1024*1024))
                    mem_top = MIN(gd->ram_size - adjust, 256*1024*1024 - adjust);
                else
                {
                    /* We have enough room, so set mem_top so that block is at
                    ** base of  DDR2 segment */
                    mem_top = 512*1024*1024 + size;
                }
                addr = mem_top - size;
            }
            else
            {
                addr = simple_strtol(eptr,NULL, 16);
            }

            if (0 > cvmx_bootmem_phy_named_block_alloc(size, addr, addr + size, 0, OCTEON_BOOTLOADER_LOAD_MEM_NAME, 0))
            {
                printf("ERROR: Unable to allocate bootloader reserved memory (addr: 0x%x, size: 0x%x).\n", addr, size);
            }
            else
            {
                /* Set default load address to base of memory reserved for loading.
                ** The setting of the env. variable also sets the load_addr global variable.
                ** This environment variable is overridden each boot if a reserved block is created. */
                char str[20];
                sprintf(str, "0x%x", addr);
                setenv("loadaddr", str);
                load_reserved_addr = addr;
                load_reserved_size = size;
            }
        }
        else
            printf("WARNING: No reserved memory for image loading.\n");

    }
#endif

#if !CONFIG_OCTEON_SIM
    /* Reserve memory for Linux kernel.  The kernel requires specific physical addresses,
    ** so by reserving this here we can load simple exec applications and do other memory
    ** allocation without interfering with loading a kernel image.  This is freed and used
    ** when loading a Linux image.  If a Linux image is not loaded, then this is freed just
    ** before the applications start.
    */
    {
        char *eptr;
        uint32_t addr;
        uint32_t size;
        eptr = getenv("octeon_reserved_mem_linux_size");
        if (!eptr ||!strcmp("auto", eptr))
        {
#ifdef CONFIG_OCTEON
            /*
             * On SRX platform, we use this for tftp-ing the JUNOS kernel for
             * tftp-install and other executables, if we need anything bigger, 
             * lets set octeon_reserved_mem_load_size, adjust
             * octeon_reserved_mem_load_base accordingly.
             */
            size = (MIN(gd->ram_size, 256*1024*1024))/2;
#else
            size = (MIN(gd->ram_size, 256*1024*1024))/2;
#endif
        }
        else
        {
            size = simple_strtol(eptr,NULL, 16);
        }
        if (size)
        {
            eptr = getenv("octeon_reserved_mem_linux_base");
            if (!eptr ||!strcmp("auto", eptr))
            {
                /* Start right after the bootloader */
                addr = OCTEON_RESERVED_LOW_MEM_SIZE;
            }
            else
            {
                addr = simple_strtol(eptr,NULL, 16);
            }
#ifdef CONFIG_JSRXNLE            
        /*
         * On SRX platforms in u-boot totally we can use 256MB from DDR0
         * of DRAM. 
         * if octeon_reserved_mem_load_base and octeon_reserved_mem_load_size
         * is not defined, then we will allocate 96MB chunk out from 256MB
         * above and here we try to allocate 128MB, which will go through fine.
         * But,if octeon_reserved_mem_load_size and octeon_reserved_mem_load_base
         * is set to 0xc800000 and 0x100000,we will allocate the requested size i.e
         * 200MB above and we will be left with (256 - 200)MB and attempting 
         * to allocate 128MB here will fail. 
         * Added a check not to attempt to allocate memory here if 
         * octeon_reserved_mem_load_size is defined. 
         *
         * so with octeon_reserved_mem_load_size set to 0xc800000 we can tftpboot
         * an images bigger than 128MB and without this we can do upto 128MB. 
         */ 
        eptr = getenv("octeon_reserved_mem_load_size");
        if (!eptr) {
        /* Allocate the memory, and print message if we fail */
            if (0 > cvmx_bootmem_phy_named_block_alloc(size, addr, addr + size, 0, 
                    OCTEON_LINUX_RESERVED_MEM_NAME, 0))
                printf("ERROR: Unable to allocate linux reserved memory \
                        (addr: 0x%x, size: 0x%x).\n", addr, size);
        }
#else 
        /* Allocate the memory, and print message if we fail */
        if (0 > cvmx_bootmem_phy_named_block_alloc(size, addr, addr + size, 0, OCTEON_LINUX_RESERVED_MEM_NAME, 0))
            printf("ERROR: Unable to allocate linux reserved memory (addr: 0x%x, size: 0x%x).\n", addr, size);
#endif		    
        }

    }
#endif


    /* Put bootmem descriptor address in known location for host */
     {
         /* Make sure it is not in kseg0, as we want physical addresss */
         uint64_t *u64_ptr = (void *)(0x80000000 | BOOTLOADER_BOOTMEM_DESC_ADDR);
         *u64_ptr = ((uint32_t)__cvmx_bootmem_internal_get_desc_ptr()) & 0x7fffffffull;
     }

#ifdef CFG_PCI_CONSOLE
     /* Check to see if we need to set up a PCI console block. Set up structure if 
     ** either count is set (and greater than 0) or pci_console_active is set */
     char *ep = getenv("pci_console_count");
     if ((ep && (simple_strtoul(ep, NULL, 10) > 0)) || getenv("pci_console_active"))
     {
         /* Initialize PCI console data structures */
         int console_count;
         int console_size;
         ep = getenv("pci_console_count");
         if (ep)
             console_count = simple_strtoul(ep, NULL, 10);
         else
             console_count = 1;

         if (console_count < 1)
             console_count = 1;

         ep = getenv("pci_console_size");
         if (ep)
             console_size = simple_strtoul(ep, NULL, 10);
         else
             console_size = 1024;

         if (console_size < 128)
             console_size = 128;

         pci_cons_desc_addr = octeon_pci_console_init(console_count, console_size);
         if (!pci_cons_desc_addr)
         {
             printf("WARNING: PCI console init FAILED\n");
         }
         else
         {
             printf("PCI console init succeeded, %d consoles, %d bytes each\n", console_count, console_size);
         }
     }
#endif



#ifdef DEBUG
     cvmx_bootmem_phy_list_print();
#endif


#if (CONFIG_OCTEON_EEPROM_TUPLES)
    {
        /* Read coremask from eeprom, and set environment variable if present.
        ** Print warning if not present in EEPROM.
        ** Currently ignores voltage/freq fields, and just uses first capability tuple
        ** This is only valid for certain parts.
        */
        int addr;
        uint8_t ee_buf[OCTEON_EEPROM_MAX_TUPLE_LENGTH];
        octeon_eeprom_chip_capability_t *cc_ptr = (void *)ee_buf;
        addr = octeon_tlv_get_tuple_addr(CFG_DEF_EEPROM_ADDR, EEPROM_CHIP_CAPABILITY_TYPE, 0, ee_buf, OCTEON_EEPROM_MAX_TUPLE_LENGTH);
        if (addr >= 0 && 
            (((octeon_is_model(OCTEON_CN38XX_PASS1) || (octeon_is_model(OCTEON_CN38XX) && (cvmx_read_csr(CVMX_CIU_FUSE) & 0xffff) == 0xffff && !cvmx_octeon_fuse_locked())))
            || octeon_is_model(OCTEON_CN56XX) || octeon_is_model(OCTEON_CN52XX)
            ))
        {
            char tmp[10];
            sprintf(tmp,"0x%04x", cc_ptr->coremask);
            setenv("coremask_override", tmp);
            coremask_from_eeprom = cc_ptr->coremask;  /* Save copy for later verification */
        }
        else
        {
            /* No chip capability tuple found, so we need to check if we expect one.
            ** CN31XX chips will all have fuses blown appropriately.
            ** CN38XX chips may have fuses blown, but may not.  We will check to see if
            ** we need a chip capability tuple and only warn if we need it but don't have it.
            */

            if (octeon_is_model(OCTEON_CN38XX_PASS1))
            {
                /* always need tuple for pass 1 chips */
                printf("Warning: No chip capability tuple found in eeprom, coremask_override may be set incorrectly\n");
            }
            else if (octeon_is_model(OCTEON_CN38XX))
            {
                /* Here we only need it if no core fuses are blown and the lockdown fuse is not blown.
                ** In all other cases the cores fuses are definitive and we don't need a coremask override
                */
                if ((cvmx_read_csr(CVMX_CIU_FUSE) & 0xffff) == 0xffff && !cvmx_octeon_fuse_locked())
                {
                    printf("Warning: No chip capability tuple found in eeprom, coremask_override may be set incorrectly\n");
                }
                else
                {
                    /* Clear coremask_override as we have a properly fused part, and don't need it */
                    setenv("coremask_override", NULL);
                }
            }
            else
            {
                /* 31xx and 30xx will always have core fuses blown appropriately */

                setenv("coremask_override", NULL);
            }
        }
    }
#endif


    /* Set numcores env variable to indicate the number of cores available */
    char tmp[10];
    sprintf(tmp,"%d", octeon_get_available_core_count());
    setenv("numcores", tmp);

#if (!CONFIG_OCTEON_SIM)
    octeon_bist();
#endif

    /* board MAC address */
    s = getenv ("ethaddr");
#ifdef CONFIG_JSRXNLE
    if (s == NULL)
        board_get_enetaddr(bd->bi_enetaddr);
    else
#endif
    for (i = 0; i < 6; ++i)
    {
        bd->bi_enetaddr[i] = s ? simple_strtoul (s, &e, 16) : 0;
        if (s)
            s = (*e) ? e + 1 : e;
    }

    /* IP Address */
    bd->bi_ip_addr = getenv_IPaddr("ipaddr");

#if defined(CONFIG_PCI)
    /*
     * Do pci configuration
     */
    pci_init();
#endif
#if CONFIG_JSRXNLE
    pcie_init();

    boot_variables_setup();

#endif

#if defined(CONFIG_ACX500_SVCS)
    pcie_ep_init();
#endif

/** leave this here (after malloc(), environment and PCI are working) **/
    /* Initialize devices */
    devices_init ();

    /* Must change std* variables after devices_init()
    ** Force bootup settings based on pci_console_active, overriding the std* variables */
    if (getenv("pci_console_active"))
    {
        printf("Using PCI console, serial port disabled.\n");
        setenv("stdin", "pci");
        setenv("stdout", "pci");
        setenv("stderr", "pci");
#if !defined(CONFIG_JSRXNLE) && !defined(CONFIG_MAG8600) && !defined(CONFIG_ACX500_SVCS)
        octeon_led_str_write("PCI CONS");
#endif
    }
    else
    {
        setenv("stdin", "serial");
        setenv("stdout", "serial");
        setenv("stderr", "serial");
    }

#if defined(CONFIG_API)
    /* Initialize API */
    api_init();
#endif
    jumptable_init ();

    /* Initialize the console (after the relocation and devices init) */
    console_init_r ();
/** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **/

    if (getenv("pci_console_active"))
    {
        /* We want to print out the board/chip info again, as this is very useful for automated testing.
        ** and board identification. */
        int cpu_rev = cvmx_get_proc_id() & 0xff;
        printf("%s board revision major:%d, minor:%d, serial #: %s\n",
               cvmx_board_type_to_string(board_type),
               gd->board_desc.rev_major, gd->board_desc.rev_minor,
               gd->board_desc.serial_str);

        printf("OCTEON %s-%s revision: %d, Core clock: %d MHz, DDR clock: %d MHz (%d Mhz data rate)\n",
               octeon_get_model_name(), octeon_get_submodel_name(),
               cpu_rev & ~0x10,
               gd->cpu_clock_mhz,
               gd->ddr_clock_mhz, gd->ddr_clock_mhz *2);
    }

    /* Initialize from environment. Note that in most cases the environment
    ** variable will have been set from the reserved memory block allocated above.
    ** By also checking this here, this allows the load address to be set in cases
    ** where there is no reserved block configured. 
    ** If loadaddr is not set, set it to something.  This default should never really be used,
    ** but we need to handle this case. */
    if ((s = getenv ("loadaddr")) != NULL)
    {
        load_addr = simple_strtoul (s, NULL, 16);
    }
    else
    {
        uint32_t addr = 0x100000;
        char tmp[20];
#ifdef CFG_LOAD_ADDR
        addr = CFG_LOAD_ADDR;
#endif
        sprintf(tmp, "0x%x", addr);
#if !CONFIG_OCTEON_SIM
        printf("WARNING: loadaddr not set, defaulting to %s.  Please either define a load reserved block or set the loadaddr environment variable.\n", tmp);
#endif
        setenv("loadaddr", tmp);
    }


#if !CONFIG_OCTEON_SIM
    {
        char tmp[30];
        /* Set up a variety of environment variables for use by u-boot and
        ** applications (through environment named block). */

        /* Set env_addr and env_size variables to the address and size of the u-boot environment. */
        sprintf(tmp,"0x%x", CFG_ENV_ADDR);
        setenv("env_addr", tmp);
        sprintf(tmp,"0x%x", CFG_ENV_SIZE);
        setenv("env_size", tmp);

#ifdef CONFIG_JSRXNLE
        /* Describe physical flash */
        switch( board_type ) {
        CASE_ALL_SRXSME_4MB_FLASH_MODELS
            sprintf(tmp,"0x%x", CFG_FLASH_BASE);    
            setenv("flash_base_addr", tmp);
            sprintf(tmp,"0x%x", CFG_FLASH_SIZE);
            setenv("flash_size", tmp);
            break;

        CASE_ALL_SRXSME_8MB_FLASH_MODELS
            sprintf(tmp,"0x%x", CFG_8MB_FLASH_BASE);
            setenv("flash_base_addr", tmp);
            sprintf(tmp,"0x%x", CFG_8MB_FLASH_SIZE);
            setenv("flash_size", tmp);
            break;
        default:
            printf("error : %s invalid board\n");
        }
#endif
#ifdef CFG_NORMAL_BOOTLOADER_SIZE
        /* We always want to report the address/size of the normal bootloader,
        ** so that we can use these from the failsafe bootloader to update the normal
        ** bootloader.  Updating the failsafe bootloader is always a 'manual' process. */
        sprintf(tmp,"0x%x", CFG_FLASH_BASE + CFG_FAILSAFE_BOOTLOADER_SIZE);
        setenv("uboot_flash_addr", tmp);
        sprintf(tmp,"0x%x", CFG_NORMAL_BOOTLOADER_SIZE);
        setenv("uboot_flash_size", tmp);

        /* Set addr size for flash space that is available for application use */
        sprintf(tmp,"0x%x", CFG_FLASH_BASE + CFG_FAILSAFE_BOOTLOADER_SIZE + CFG_NORMAL_BOOTLOADER_SIZE);
        setenv("flash_unused_addr", tmp);
        sprintf(tmp,"0x%x", CFG_FLASH_SIZE - CFG_FAILSAFE_BOOTLOADER_SIZE - CFG_NORMAL_BOOTLOADER_SIZE - CFG_ENV_SIZE);
        setenv("flash_unused_size", tmp);
#endif
#ifdef CONFIG_MAG8600
    if (getenv("bootcmd") == NULL)
       setenv("bootcmd", "cp.b 0xbec80000 0x100000 0x100000; bootelf 0x100000");
    setenv("ubootupdate", "protect off all;erase 0xbec00000 0xbec7ffff;cp.b 0x20000000 0xbec00000 0x78000");
    setenv("ubootjload", "protect off all;erase 0xbec80000 0xbecfffff;cp.b 0x20000000 0xbec80000 0x70000");
    setenv("boot.ver", uboot_api_ver_string);
    if (getenv("ethact") == NULL)
        setenv("ethact", "octeth1");
    setenv("ethprime","octeth1");
    if ((getenv("loaddev")) == NULL)
        setenv("loaddev", "disk1");
    setenv("bootdelay", "5");
    mag8600_bootseq_init();
#endif

    }
#endif

#if (CONFIG_COMMANDS & CFG_CMD_NET)
    if ((s = getenv ("bootfile")) != NULL)
    {
        copy_filename (BootFile, s, sizeof (BootFile));
    }
#endif	/* CFG_CMD_NET */

#if defined(CONFIG_MISC_INIT_R)
    /* miscellaneous platform dependent initialisations */
    misc_init_r ();
#endif

#if CONFIG_OCTEON_LANBYPASS
    /* Set the LAN bypass defaults for Thunder */
    octeon_gpio_cfg_output(6); /* GPIO 6 controls the bypass enable */
    octeon_gpio_cfg_output(7); /* GPIO 7 controls the watchdog clear */
    octeon_gpio_cfg_input(5);  /* GPIO 5 tells you the bypass status */
    
    /* Startup with bypass disabled and watchdog cleared */
    octeon_gpio_clr(6);
    octeon_gpio_set(7);
#endif 

#ifdef CONFIG_JSRXNLE
    if  (IS_PLATFORM_SRX210(board_type)) {
        /*
         * Take NL055 and VIA out of reset by driving the gpio pins
         */
        octeon_gpio_ext_cfg_output(16); /* GPIO 16 controls NL055 enable */
        octeon_gpio_ext_cfg_output(17); /* GPIO 17 controls VIA enable */
         
        octeon_gpio_set(16);
        octeon_gpio_set(17);
        udelay(10000);
    }
#endif

#if (CONFIG_COMMANDS & CFG_CMD_NET) && defined(CONFIG_NET_MULTI)
    puts ("Net:   ");
    eth_initialize(gd->bd);
#endif


#if CONFIG_OCTEON && !CONFIG_OCTEON_SIM && (CONFIG_COMMANDS & CFG_CMD_IDE)
#ifndef CONFIG_PCI  
    if (1 || octeon_cf_present())
#endif
    {
        /* Scan for IDE controllers and register them in the ide_dev_desc array. 
        ** The order of scanning will determine what order they are listed to the user in. */

        /* Scan the DMA capable slot first, so that it is contoller 0 on the ebh5600 board. (and will likely
        ** be the only one populated on later board builds */
#if defined(OCTEON_CF_TRUE_IDE_CS0_ADDR)
        /* Init IDE structures for CF interface on boot bus */
        ide_register_device(IF_TYPE_IDE, BLOCK_QUIRK_IDE_CF | BLOCK_QUIRK_IDE_CF_TRUE_IDE, 0, MAKE_XKPHYS(OCTEON_CF_TRUE_IDE_CS0_ADDR), MAKE_XKPHYS(OCTEON_CF_TRUE_IDE_CS0_ADDR), 0, 0);
        if (CFG_IDE_MAXDEVICE > 1)
        {
            /* This device won't every be used, but we want to have 2 devices per ide bus regargless.  If we
            ** are only supporting 1 device, skip this */
            ide_register_device(IF_TYPE_IDE, BLOCK_QUIRK_IDE_CF | BLOCK_QUIRK_IDE_CF_TRUE_IDE, 1, MAKE_XKPHYS(OCTEON_CF_TRUE_IDE_CS0_ADDR), MAKE_XKPHYS(OCTEON_CF_TRUE_IDE_CS0_ADDR), 0, 0);
        }
#endif

#ifdef OCTEON_CF_COMMON_BASE_ADDR
        /* Init IDE structures for CF interface on boot bus */
        ide_register_device(IF_TYPE_IDE, BLOCK_QUIRK_IDE_CF, 0, MAKE_XKPHYS(OCTEON_CF_COMMON_BASE_ADDR + CFG_ATA_DATA_OFFSET), MAKE_XKPHYS(OCTEON_CF_COMMON_BASE_ADDR + CFG_ATA_REG_OFFSET), 0, 0);
        if (CFG_IDE_MAXDEVICE > 1)
        {
            /* This device won't every be used, but we want to have 2 devices per ide bus regargless.  If we
            ** are only supporting 1 device, skip this */
            ide_register_device(IF_TYPE_IDE, BLOCK_QUIRK_IDE_CF, 1, MAKE_XKPHYS(OCTEON_CF_COMMON_BASE_ADDR + CFG_ATA_DATA_OFFSET), MAKE_XKPHYS(OCTEON_CF_COMMON_BASE_ADDR + CFG_ATA_REG_OFFSET), 0, 0);
        }
#endif
        /* Always do ide_init if PCI enabled to allow for PCI IDE devices */

#ifdef CONFIG_JSRXNLE
        switch (board_type) {
        CASE_ALL_SRXLE_CF_MODELS
            /*
             * Set disk.install variable. This will be used
             * by the loader during image install from USB
             * disk0: USB0
             * disk3: USB1
             */
            if (num_usb_disks() > 1) {
                setenv("disk.install", "disk3");
            } else {
                setenv("disk.install", "disk0");
            }
            if (IS_PLATFORM_SRX550(board_type)) {
                sil3132_init(1, CF_SATA_PORT); 
                sil3132_init(1, SSD_SATA_PORT); 
            } else {
                sii_ata_pci_ide_init();
            }
            break;
        CASE_ALL_SRX650_MODELS
            /* 
             * Set disk.install variable. This will be used
             * by the loader during image install from USB
             * disk0: USB0
             * disk3: USB1
             */
            if (num_usb_disks() > 1) {
                setenv("disk.install", "disk3");
            } else {
                setenv("disk.install", "disk0");
            }
            octeon_gpio_cfg_input(SRX650_GPIO_CF_CD1_L);
            octeon_gpio_cfg_output(SRX650_GPIO_EXT_CF_WE); 
            octeon_gpio_cfg_output(SRX650_GPIO_IDE_CF_CS); 
            octeon_gpio_cfg_output(SRX650_GPIO_EXT_CF_PWR); 
            /*
             * If next bootmedia is external CF (DISK_SECOND) then check
             * if external CF is present, then enable it and do ide_init. 
             * For other cases disable external CF. This will make internal
             * CF visible, as out of internal and external only one can be
             * be visible (if external CF enabled, it overshadows internal)
             */
            bootdev = get_next_bootdev();
            switch (bootdev) {
            case DISK_SECOND:
		if (srxsme_is_device_enabled(DISK_SECOND)) {
		    printf("Enabling external compact flash...\n");
		    srxmr_select_and_enable_cf(DISK_SECOND);
		} else if (srxsme_is_device_enabled(DISK_FIRST)) {
		    printf("Enabling internal compact flash...\n");
		    srxmr_select_and_enable_cf(DISK_FIRST);
		} else {
		    printf("WARNING: None of the compact-flash is enabled in boot.devlist.\n");
		}
                break;
            case DISK_FIRST:
                /* First try to scan internal CF. For this first disable  
                 * external CF. If internal CF is not present, then try
                 * scanning external CF 
                 */
internal_cf_pref:
		if (srxsme_is_device_enabled(DISK_FIRST)) {
		    printf("Enabling internal compact flash...\n");
		    if (srxmr_select_and_enable_cf(DISK_FIRST)) {
			printf("Internal compact flash not present. Trying external...\n");
			/* if internal wasn't selected, try to select external */
			if (srxsme_is_device_enabled(DISK_SECOND)) {
			    srxmr_select_and_enable_cf(DISK_SECOND);
			}
		    }
		} else if (srxsme_is_device_enabled(DISK_SECOND)) {
		    printf("Enabling external compact flash...\n");
		    srxmr_select_and_enable_cf(DISK_SECOND);
		} else {
		    printf("WARNING: None of the compact-flash is enabled in boot.devlist.\n");
		}
                break;
            case DISK_ZERO:
            default:
                /* 
                 * If ext.cf.pref environment variable is set to 1, then 
                 * enable external CF if present, otherwise scan internal
                 */
                s = getenv ("ext.cf.pref");
                if (s == NULL || strcmp(s, "1")) {
                    goto internal_cf_pref;
                }
		if (srxsme_is_device_enabled(DISK_SECOND)) {
		    printf("Enabling external comapct flash...\n");
		    srxmr_select_and_enable_cf(DISK_SECOND);
		}
                break;
            }
            break;
        }
#else
        ide_init();
#endif
}
#endif

#if CONFIG_OCTEON
    /* verify that boot_init_vector type is the correct size */
    if (BOOT_VECTOR_NUM_WORDS*4 != sizeof(boot_init_vector_t))
    {
        printf("ERROR: boot_init_vector_t size mismatch: define: %d, type: %d\n",BOOT_VECTOR_NUM_WORDS*8,  sizeof(boot_init_vector_t));
        while (1)
            ;
    }
    /* DEBUGGER setup  */

    extern void cvmx_debugger_handler2(void);
    extern void cvmx_debugger_handler_pci(void);
    extern void cvmx_debugger_handler_pci_profiler(void);
    /* put relocated address of 2nd stage of debug interrupt
    ** at BOOTLOADER_DEBUG_TRAMPOLINE (word value)
    ** This only needs to be done once during initial boot
    */
    {
        /* Put address of cvmx_debugger_handler2() into fixed DRAM
        ** location so that minimal exception handler in flash
        ** can jump to this easily. Also put a list of the available handlers
        ** in a location accessible from PCI at a fixed address. Host PCI
        ** routines can then use this to replace the normal debug handler
        ** with one supporting other stuff.
        */
        uint32_t *ptr = (void *)BOOTLOADER_DEBUG_TRAMPOLINE;
        *ptr = (uint32_t)cvmx_debugger_handler2;
        ptr = (void*)BOOTLOADER_DEBUG_HANDLER_LIST;
        *ptr++ = (uint32_t)cvmx_debugger_handler2;
        *ptr++ = (uint32_t)cvmx_debugger_handler_pci;
        *ptr++ = (uint32_t)cvmx_debugger_handler_pci_profiler;
        *ptr = 0;

        /* Save the GP value currently used (after relocation to DRAM)
        ** so that debug stub can restore it before calls cvmx_debugger_handler2.
        */
        uint32_t tmp = 0x80000000 | BOOTLOADER_DEBUG_GP_REG;
        asm volatile ("sw    $28, 0(%[tmp])" : :[tmp] "r" (tmp) :"memory");
        /* Save K0 (global data pointer) so that debug stub can set up the correct
        ** context to use bootloader functions. */
        tmp = 0x80000000 | BOOTLOADER_DEBUG_KO_REG_SAVE;
        asm volatile ("sw    $26, 0(%[tmp])" : :[tmp] "r" (tmp) :"memory");
       
    }
#endif


#if CFG_LATE_BOARD_INIT
    late_board_init();
#endif
#if CONFIG_JSRXNLE
    ethsetup();
    
    /* turn off the mpim leds */
    turn_off_jsrxnle_mpim_leds();

    /*
     * starting ciu watchdog before disabling cpd watchdog
     */
    RELOAD_WATCHDOG(ENABLE_WATCHDOG);
#endif

#ifdef CONFIG_POST && CONFIG_JSRXNLE
    post_run("rtc", (POST_RAM | POST_NORMAL));

    /* 
     * If nand error occurs, this means ST nand controller
     * is in corrupted state.So,no usb POST should be run,
     * otherwise, the box will hang due to continuous read
     * errors.
     */
    if (!simple_strtoul(getenv("nand_error"), NULL, 16) ) {
        post_run("usb", (POST_RAM | POST_NORMAL));
    }

    post_run("sku-id", (POST_RAM | POST_NORMAL));    

    if (gd->post_result) {
        printf("POST Failed\n");
    } else {
        printf("POST Passed\n");
    }
    
    /* log results to environment variable */
    post_log_result_env();
#endif
    /*
     * disable cpld watchdog
     */

    RELOAD_CPLD_WATCHDOG(DISABLE_WATCHDOG); 
    /* main_loop() can return to retry autoboot, if so just run it again. */
    for (;;)
    {
        main_loop ();
    }

    /* NOTREACHED - no way out of command loop except booting */
}

void hang (void)
{
	puts ("### ERROR ### Please RESET the board ###\n");
	for (;;);
}

#ifdef ENABLE_BOARD_DEBUG
void gpio_status(int data)
{
    int i;
    for (i=0; i<7; ++i) {
        if ((data>>i)&1)
            octeon_gpio_set(8+i);
        else
            octeon_gpio_clr(8+i);
    }
}
#endif



#if CONFIG_JSRXNLE
void
boot_variables_setup() 
{
    DECLARE_GLOBAL_DATA_PTR;
    uint32_t board_type = gd->board_desc.board_type;
    unsigned char temp[64];
    uint8_t reboot_reason = 0;
    setenv("boot.ver", uboot_api_ver_string);

    switch (board_type) {
    CASE_ALL_SRXSME_4MB_FLASH_MODELS
        sprintf(temp, "0x%08x ", CFG_LOADER_START);
        setenv("boot.upgrade.loader", temp);

        sprintf(temp, "0x%08x ", CFG_PRIMARY_UBOOT_START);
        setenv("boot.upgrade.uboot", temp);

        /* U-Shell is required only on SRX240 */
        switch (board_type) {
        CASE_ALL_SRX240_MODELS
            sprintf(temp, "0x%08x ", CFG_4MB_USHELL_START);
            setenv("boot.upgrade.ushell", temp);
        }

        if (boot_sector_flag == FLASH_BOOT_SECTOR_BOTTOM) {
            sprintf(temp, "0x%08x ", SRXLE_BOOTSEQ_BOTBOOT_FLASH_SECTADDR
                                                         - CFG_FLASH_BASE);
        } else if (boot_sector_flag == FLASH_BOOT_SECTOR_TOP) {
            sprintf(temp, "0x%08x ", SRXLE_BOOTSEQ_TOPBOOT_FLASH_SECTADDR
                                                         - CFG_FLASH_BASE);
        }
        setenv("boot.btsq.start", temp);


        if (boot_sector_flag == FLASH_BOOT_SECTOR_BOTTOM) {
            sprintf(temp, "0x%08x ", SRXLE_BOTBOOT_BOOTSEQ_SECT_LEN);
        } else if (boot_sector_flag == FLASH_BOOT_SECTOR_TOP) {
            sprintf(temp, "0x%08x ", SRXLE_TOPBOOT_BOOTSEQ_SECT_LEN);
        }
        setenv("boot.btsq.len", temp);

        sprintf(temp, "0x%08x ", (CFG_ENV_ADDR - CFG_FLASH_BASE));
        setenv("boot.env.start", temp);
        break;
    /*
     * SRX220 deviates from other ASGARD family platforms as it
     * has 8MB boot flash. SRX650 doesn't have the secondary u-boot,
     * hence boot.upgrade.uboot.secondary is being set for SRX220 only.
     */
    CASE_ALL_SRXSME_8MB_FLASH_MODELS
        sprintf(temp, "0x%08x ", CFG_8MB_USHELL_START);
        setenv("boot.upgrade.ushell", temp);
        sprintf(temp, "0x%08x ", CFG_8MB_FLASH_LOADER_START);
        setenv("boot.upgrade.loader", temp);
        
        sprintf(temp, "0x%08x ", CFG_8MB_FLASH_PRIMARY_UBOOT_START);
        setenv("boot.upgrade.uboot", temp);

        sprintf(temp, "0x%08x ", CFG_8MB_FLASH_ENV_ADDR -
                                 CFG_8MB_FLASH_BASE);
        setenv("boot.env.start", temp);

        sprintf(temp, "0x%08x ", SRXMR_BOOTSEQ_FLASH_SECTADDR
                                 - CFG_8MB_FLASH_BASE);
        setenv("boot.btsq.start", temp);

        sprintf(temp, "0x%08x ", SRXMR_BOOTSEQ_SECT_LEN);
        setenv("boot.btsq.len", temp);
        switch (board_type) {
        CASE_ALL_SRX220_MODELS
        CASE_ALL_SRX550_MODELS
            reboot_reason = cpld_read(SRX220_CPLD_HW_RST_STATUS_REG);
            if (reboot_reason != 0x09 && reboot_reason != 0x03) {
                printf("Warning!!!Last reboot reason 0x%x abnormal\n", 
                       reboot_reason); 
            }
            cpld_write(SRX220_CPLD_HW_RST_STATUS_REG, 0xf);
            sprintf(temp, "0x%x", reboot_reason);
            setenv("reboot.reason",temp);

            sprintf(temp, "0x%08x ", CFG_8MB_FLASH_SECONDARY_UBOOT_START);
            setenv("boot.upgrade.uboot.secondary", temp);
            break;
        }
        break;
    default:
        printf("Unknown board type\n");
    }

    /* 
     * Add an environment variable to control a new is_polled_flag 
     * environment flag as a performance optimization to avoid hogging
     * 100% of the CPU during I/O.
     */
    if ((getenv("boot.is_polled_flag")) == NULL) {
        switch (gd->board_desc.board_type) {
        CASE_ALL_SRX650_MODELS
            setenv("boot.is_polled_flag", "0");
            break;
        }
    }

    if ((getenv("loaddev")) == NULL) {
        switch (board_type) {
        CASE_ALL_SRXLE_CF_MODELS
        CASE_ALL_SRX650_MODELS
            /* Default load device for CF based platforms is the CF (disk1)*/
            sprintf(temp, "%s ", "disk1:");
            break;
        default:
            sprintf(temp, "%s ", "disk0:");
            break;
        }
        setenv("loaddev", temp);
    }

    sprintf(temp, "0x%08x", CFG_ENV_SIZE);
    setenv("boot.env.size", temp);

    /*
     * Offsets of CRC header and the data on which
     * CRC is calculated.
     */
    sprintf(temp, "0x%08x ", IMG_HEADER_OFFSET);
    setenv("boot.upgrade.uboot.hdr", temp);

    sprintf(temp, "0x%08x ", IMG_DATA_OFFSET);
    setenv("boot.upgrade.uboot.data", temp);

    sprintf(temp, "0x%08x ", CFG_LOADER_HDR_OFFSET);
    setenv("boot.upgrade.loader.hdr", temp);

    sprintf(temp, "0x%08x ", CFG_LOADER_DATA_OFFSET);
    setenv("boot.upgrade.loader.data", temp);
#ifdef CONFIG_API
    sprintf(temp, "0x%08x", gd->board_desc.board_type);
    setenv("board_id", temp);

    setenv("rootpath", "/");
#endif

    if (IS_PLATFORM_SRX550(board_type)) {
        setenv("bootcmd", "cp.b 0xbf600000 0x100000 0xfffc0;\
 cp.b 0xbf800000 0x1fffc0 0x100000; bootelf 0x100000");
    } else {
        setenv("bootcmd", "cp.b 0xbfe00000 0x100000 0x100000; bootelf 0x100000");
    }

    /*
     * Initialize the bootsequencing code after
     * all variables are defined
     */
    srxsme_bootseq_init();
}

int
recover_primary (void)
{
    DECLARE_GLOBAL_DATA_PTR;     
    int ret_val = 0;

    switch (gd->board_desc.board_type) {
    CASE_ALL_SRXSME_4MB_FLASH_MODELS
        if (!is_valid_uboot_image(CFG_PRIMARY_UBOOT_START,
                                  CFG_UBOOT_SIZE, 0)) {
            printf("WARNING: Could not verify the sanity of Primary U-Boot.\n"
                   "System will try to recover it now.\n");
            /* primary copy is not good, snapshot backup to primary */
            if (!bootloader_upgrade(CFG_SECONDARY_UBOOT_START,
                                    CFG_PRIMARY_UBOOT_START,
                                    CFG_PRIMARY_UBOOT_END,
                                    CFG_UBOOT_SIZE)){
                if(!is_valid_uboot_image(CFG_PRIMARY_UBOOT_START,
                                         CFG_UBOOT_SIZE, 0)) {
                    ret_val = -1;
                }
            }
        } else {
            ret_val = 1;
        }
	break;
    /*
     * SRX220 deviates from other ASGARD family platforms as it
     * has 8MB boot flash.
     */
    CASE_ALL_SRXSME_8MB_FLASH_MODELS
        if (!is_valid_uboot_image(CFG_8MB_FLASH_PRIMARY_UBOOT_START,
                                  CFG_UBOOT_SIZE, 0)) {
            printf("WARNING: Could not verify the sanity of Primary U-Boot.\n"
                   "System will try to recover it now.\n");
            /* primary copy is not good, snapshot backup to primary */
            if (!bootloader_upgrade(CFG_8MB_FLASH_SECONDARY_UBOOT_START,
                                    CFG_8MB_FLASH_PRIMARY_UBOOT_START,
                                    CFG_8MB_FLASH_PRIMARY_UBOOT_END,
                                    CFG_UBOOT_SIZE)){
                if(!is_valid_uboot_image(CFG_8MB_FLASH_PRIMARY_UBOOT_START,
                                         CFG_UBOOT_SIZE, 0)) {
                    ret_val = -1;
                }
            }
        } else {
            ret_val = 1;
        }
	break;
    }
 
    return ret_val;
}

#endif
