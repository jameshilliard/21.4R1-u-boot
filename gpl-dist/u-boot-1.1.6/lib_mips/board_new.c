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

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <devices.h>
#include <net.h>
#include <environment.h>

DECLARE_GLOBAL_DATA_PTR;

#if (!defined(ENV_IS_EMBEDDED) ||  defined(CFG_ENV_IS_IN_NVRAM))
#define	TOTAL_MALLOC_LEN	(CFG_MALLOC_LEN + CFG_ENV_SIZE)
#else
#define	TOTAL_MALLOC_LEN	CFG_MALLOC_LEN
#endif

extern int timer_init(void);
extern int incaip_set_cpuclk(void);

extern ulong uboot_end_data;
extern ulong uboot_end;

ulong monitor_flash_len;

extern const char version_string[];

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

/*
 * The Malloc area is immediately below the monitor copy in DRAM
 */
static void mem_malloc_init (void)
{
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
#ifdef  CONFIG_BOARD_TYPES
        int board_type = gd->board_type;
#else
        int board_type = 0; /* use dummy arg */
#endif
    puts ("DRAM:  ");

    if ((gd->ram_size = initdram (board_type)) > 0) {

         print_size (gd->ram_size, "\n");
         return (0);
    }
    puts (failed);
    return (1);
}

static int display_banner(void)
{
	printf ("\n\n%s\n\n", version_string);
	return (0);
}

static void display_flash_config(ulong size)
{
	puts ("Flash: ");
	print_size (size, "\n");
}

static int init_baudrate (void)
{
	char tmp[64];	/* long enough for environment variables */
	int i = getenv_r ("baudrate", tmp, sizeof (tmp));

    	gd->baudrate = (i > 0)
			? (int) simple_strtoul (tmp, NULL, 10)
			: CONFIG_BAUDRATE;

	return (0);
}

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

typedef int (init_fnc_t) (void);

init_fnc_t *init_sequence[] = {
	timer_init,
	env_init,		/* initialize environment */
	init_baudrate,		/* initialze baudrate settings */
	serial_init,		/* serial communications setup */
        console_init_f,
	display_banner,		/* say that we are here */
	checkboard,
	init_func_ram,
	NULL,
};

void board_init_f(ulong bootflag)
{
	gd_t gd_data, *id;
	bd_t *bd;
	init_fnc_t **init_fnc_ptr;
	ulong addr, addr_sp, len = (ulong)&uboot_end - CFG_MONITOR_BASE;
        
#ifdef CONFIG_PURPLE
	void copy_code (ulong);
#endif
        gd = &gd_data;
         /* compiler optimization barrier needed for GCC >= 3.4 */
        __asm__ __volatile__("": : :"memory");
        /* Pointer is writable since we allocated a register for it.  */
	memset ((void *)gd, 0, sizeof (gd_t));
#ifdef CONFIG_FX
    /* 
     * Here the Flash base is still at 0xbfc00000. The env_init 
     * will access the flash to read the environment variables.
     * To access the environment variables sector 0xbc3f0000, which
     * is the last sector of partition 0, when booting in partition 1,
     * the Full Flash Mode in the boot CPLD has to be enabled now.
     * The CPLD is now inited with temperory address at 0xbfb00000.
     * CPLD init will be done again later after relocation of the 
     * base address to initialize it at correct address and also 
     * to init all the software structures.
     */
	fx_cpld_early_init();
	fx_cpld_enable_full_flash_mode();
#endif    
	for (init_fnc_ptr = init_sequence; *init_fnc_ptr; ++init_fnc_ptr) {
		if ((*init_fnc_ptr)() != 0) {
			printf("hanging...\n");
			hang ();
		}
	}
	/*
	 * Now that we have DRAM mapped and working, we can
	 * relocate the code and continue running from DRAM.
	 */

    addr = CFG_SDRAM_BASE + gd->ram_size;
    addr &= ~(4096 - 1);
    debug ("Top of RAM usable for U-Boot at: %08lx\n", addr);
    addr -= len;
    addr &= ~(16 * 1024 - 1);

	 	/* Reserve memory for malloc() arena.
		 */
	addr_sp = addr - TOTAL_MALLOC_LEN;
#ifdef DEBUG
	printf ("Reserving %dk for malloc() at: %08lx\n",
			TOTAL_MALLOC_LEN >> 10, addr_sp);
#endif

	/*
	 * (permanently) allocate a Board Info struct
	 * and a permanent copy of the "global" data
	 */
	addr_sp -= sizeof(bd_t);
	bd = (bd_t *)addr_sp;
	gd->bd = bd;
#ifdef DEBUG
	printf ("Reserving %d Bytes for Board Info at: %08lx\n",
			sizeof(bd_t), addr_sp);
#endif
	addr_sp -= sizeof(gd_t);
	id = (gd_t *)addr_sp;
#ifdef DEBUG
	printf ("Reserving %d Bytes for Global Data at: %08lx\n",
			sizeof (gd_t), addr_sp);
#endif

	 	/* Reserve memory for boot params.
		 */
	addr_sp -= CFG_BOOTPARAMS_LEN;
	bd->bi_boot_params = addr_sp;
#ifdef DEBUG
	printf ("Reserving %dk for boot params at: %08lx\n",
			CFG_BOOTPARAMS_LEN >> 10, addr_sp);
#endif

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
	/*
	 * Save local variables to board info struct
	 */
	bd->bi_memstart	= CFG_SDRAM_BASE;	/* start of  DRAM memory */
	bd->bi_memsize	= gd->ram_size;		/* size  of  DRAM memory in bytes */
	bd->bi_baudrate	= gd->baudrate;		/* Console Baudrate */

	memcpy (id, (void *)gd, sizeof (gd_t));

	/* On the purple board we copy the code in a special way
	 * in order to solve flash problems
	 */
#ifdef CONFIG_PURPLE
	copy_code(addr);
#endif
#ifdef DEBUG
        printf("relocating and jumping to code in DRAM\n");
#endif
#ifdef CONFIG_FX
    /*
     * Disable the FULL Flash mode since when the partition1 is
     * executing, the relocate code needs access to boot1_5 address
     * space. Re-enable it once the code relocation is over.
     */
    fx_cpld_disable_full_flash_mode();
#endif
	relocate_code (addr_sp, id, addr);

	/* NOTREACHED - relocate_code() does not return */
}


typedef volatile u_int32_t xlr_reg_t;

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
    cmd_tbl_t *cmdtp;
    ulong size = 0;
    extern void malloc_bin_reloc (void);
#ifndef CFG_ENV_IS_NOWHERE
    extern char * env_name_spec;
#endif
    char *s, *e;
    bd_t *bd;
    int i;

/* Convince GCC to really do what we want, otherwise the old value is still used......*/
    gd = id;
    gd->flags |= GD_FLG_RELOC;  /* tell others: relocation done */
#ifdef DEBUG
    /* This is the address that u-boot has relocated itself to.  Use this 
    ** address for RELOCATED_TEXT_BASE in the makefile to enable the relocated
    ** build for debug info. */
    printf ("Now running in RAM - U-Boot at: 0x%x\n", dest_addr);
#endif
#ifdef CONFIG_FX
    /*
     * Relocation is now over. The Full Flash Mode
     * canb be safely enabled now.
     */
    fx_cpld_enable_full_flash_mode();
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
    bd->bi_flashstart = CFG_FLASH_BASE;
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

    /* IP Address */
    bd->bi_ip_addr = getenv_IPaddr("ipaddr");
#ifdef CONFIG_FX
    /* This needs to be called after the env is all set up */
    set_board_type();
    fx_reset_fpga();
#endif    
#if defined(CONFIG_PCI)
    /*
     * Do pci configuration
     */
    pci_init();
#endif
#if CONFIG_JSRXNLE
    pcie_init();
#endif

/** leave this here (after malloc(), environment and PCI are working) **/
    /* Initialize devices */
    devices_init ();
    jumptable_init ();

    /* Initialize the console (after the relocation and devices init) */
    console_init_r ();
/** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **/
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

#if (CONFIG_COMMANDS & CFG_CMD_NET)
    if ((s = getenv ("bootfile")) != NULL)
    {
        copy_filename (BootFile, s, sizeof (BootFile));
    }
#endif	/* CFG_CMD_NET */

#ifdef CONFIG_FX
    /* save default environment. */
    if (gd->env_valid == 3) {
        if (saveenv() == 0) {
            gd->env_valid = 1;
        }
    }
#endif    

#if defined(CONFIG_MISC_INIT_R)
    /* miscellaneous platform dependent initialisations */
    debug("%s[%d]\n", __func__, __LINE__);
    misc_init_r ();
#endif

#if (CONFIG_COMMANDS & CFG_CMD_NET) && defined(CONFIG_NET_MULTI)
    puts ("Net:   ");
    eth_initialize(gd->bd);
#endif

#ifdef CONFIG_FX 
    extern int post_run(char *name, int flags);

    SET_PLATFORM_BOOT_STATUS(BS_UBOOT_BOOTING);
    post_run(NULL, 0);
    eth_init(gd->bd); // Init Ethernet interface
    /* Reaching here, it is safe to say that u-boot is good. 
     * Setting the boot_key to avoid a CPU reset from CPLD
     */
    fx_cpld_set_bootkey();
    fx_cpld_enable_watchdog(0, 0);
#endif 

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
