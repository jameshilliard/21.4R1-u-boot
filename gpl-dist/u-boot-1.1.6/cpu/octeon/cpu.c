/*
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, <wd@denx.de>
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
#include <asm/mipsregs.h>
#include <watchdog_cpu.h>

#ifdef CONFIG_OCTEON
#include "octeon_boot.h"
#endif

#if defined(CONFIG_OCTEON)
void
octeon_cpu_reset (void)
{
    /*
     * Octeon3 chips have a different register
     * for initiating soft reset.
     */
#if defined(CONFIG_ACX500_SVCS)
    cvmx_write_csr(CVMX_RST_SOFT_RST, 1ull);
#endif

    uint8_t temp;

    cvmx_write_csr(CVMX_CIU_SOFT_BIST, 1ull);
    temp = cvmx_read_csr(CVMX_CIU_SOFT_RST);
    cvmx_write_csr(CVMX_CIU_SOFT_RST, 1ull);
}
#endif

int do_reset(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    /* 
     * Enable the CPLD watchdog again.
     * After reset, system will try to
     * come from primary, if its corrupted,
     * this watchdog will help in switching
     * to backup.
     */
#if defined(CONFIG_JSRXNLE)
    reset_jsrxnle_watchdog();
#endif
    RELOAD_CPLD_WATCHDOG(ENABLE_WATCHDOG);

#if defined(CONFIG_JSRXNLE)
    srxsme_board_reset();
#else
#if defined(CONFIG_OCTEON)
    octeon_cpu_reset(); 
#endif
#ifdef OCTEON_CF_RESET_GPIO
    octeon_gpio_cfg_output(OCTEON_CF_RESET_GPIO);
    octeon_gpio_clr(OCTEON_CF_RESET_GPIO);
    CVMX_SYNC;
    udelay(1);
    octeon_gpio_set(OCTEON_CF_RESET_GPIO);
    CVMX_SYNC;
    udelay(5000);
#endif
#endif

    fprintf(stderr, "*** reset failed ***\n");

    return 0;
}

void flush_cache (ulong start_addr, ulong size)
{

}

int dcache_status (void)
{
    return 0;       /* always off */
}

void dcache_disable (void)
{
    return;
}

void write_one_tlb( int index, u32 pagemask, u32 hi, u32 low0, u32 low1 ){
	write_32bit_cp0_register(CP0_ENTRYLO0, low0);
	write_32bit_cp0_register(CP0_PAGEMASK, pagemask);
	write_32bit_cp0_register(CP0_ENTRYLO1, low1);
	write_32bit_cp0_register(CP0_ENTRYHI, hi);
	write_32bit_cp0_register(CP0_INDEX, index);
	tlb_write_indexed();
}

#if MIPS64_CONTEXT_SAVE
#define TFx "%08x%08x"
#define REG_VAL(val)    (uint32_t)((uint64_t)val >> 32), (uint32_t)(val)
#else
#define TFx "%08x"
#define REG_VAL(val)    ((uint32_t)val)
#endif

void 
print_trap_frame (octeon_trap_frame_t *frame, int verbose)
{
    if (!verbose) {
        printf("ra: "TFx"\t epc: "TFx"\n", REG_VAL(frame->tf_ra), REG_VAL(frame->tf_pc));
        printf("bad_vaddr: "TFx"\n", REG_VAL(frame->tf_badvaddr));
        return;
    }

    printf("zero: "TFx"\tat: "TFx"\tv0: "TFx"\tv1: "TFx"\n",
    	   REG_VAL(frame->tf_zero), REG_VAL(frame->tf_ast), 
           REG_VAL(frame->tf_v0), REG_VAL(frame->tf_v1));

    printf("a0: "TFx"\ta1: "TFx"\ta2: "TFx"\ta3: "TFx"\n",
           REG_VAL(frame->tf_a0), REG_VAL(frame->tf_a1), 
           REG_VAL(frame->tf_a2), REG_VAL(frame->tf_a3));

    printf("t0: "TFx"\tt1: "TFx"\tt2: "TFx"\tt3: "TFx"\n",
           REG_VAL(frame->tf_t0), REG_VAL(frame->tf_t1), 
           REG_VAL(frame->tf_t2), REG_VAL(frame->tf_t3));

    printf("t4: "TFx"\tt5: "TFx"\tt6: "TFx"\tt7: "TFx"\n",
           REG_VAL(frame->tf_t4), REG_VAL(frame->tf_t5), 
           REG_VAL(frame->tf_t6), REG_VAL(frame->tf_t7));

    printf("t8: "TFx"\tt9: "TFx"\ts0: "TFx"\ts1: "TFx"\n",
           REG_VAL(frame->tf_t8), REG_VAL(frame->tf_t9), 
           REG_VAL(frame->tf_s0), REG_VAL(frame->tf_s1));

    printf("s2: "TFx"\ts3: "TFx"\ts4: "TFx"\ts5: "TFx"\n",
           REG_VAL(frame->tf_s2), REG_VAL(frame->tf_s3), 
           REG_VAL(frame->tf_s4), REG_VAL(frame->tf_s5));

    printf("s6: "TFx"\ts7: "TFx"\tk0: "TFx"\tk1: "TFx"\n",
           REG_VAL(frame->tf_s6), REG_VAL(frame->tf_s7), 
           REG_VAL(frame->tf_k0), REG_VAL(frame->tf_k1));

    printf("gp: "TFx"\tsp: "TFx"\ts8: "TFx"\tra: "TFx"\n",
           REG_VAL(frame->tf_gp), REG_VAL(frame->tf_sp), 
           REG_VAL(frame->tf_s8), REG_VAL(frame->tf_ra));

    printf("sr: "TFx"\tmullo: "TFx"\tmulhi: "TFx"\tbadvaddr: "TFx"\n",
           REG_VAL(frame->tf_sr), REG_VAL(frame->tf_mullo), 
           REG_VAL(frame->tf_mulhi), REG_VAL(frame->tf_badvaddr));

    printf("cause: "TFx"\tpc: "TFx"\n",
           REG_VAL(frame->tf_cause), REG_VAL(frame->tf_pc));

}

void 
octeon_exception_handler (octeon_trap_frame_t *trap_frame)
{
    char cause_buffer[64];
    char *message = NULL;
    int counter;
    uint32_t boot_status;
    int is_nmi = 0;
    DECLARE_GLOBAL_DATA_PTR;

    if (trap_frame->tf_sr & (0x01UL << 19)) {
        sprintf(cause_buffer, "NMI");
        is_nmi = 1;
    } else {
        sprintf(cause_buffer, "%d", (trap_frame->tf_cause >> 2 & 0x1FUL));
    }


    GET_PLATFORM_BOOT_STATUS(boot_status);

    for (counter = 0; counter < BOOT_STATUS_MAX; counter++) {
        switch(boot_status) {
        case BS_LOADER_LOADING_KERNEL:
            message = "Failed to load kernel.";
            break;

        case BS_UBOOT_BOOTING:
            message = "Problem encountered during u-boot stage.";
            break;

        case BS_LOADER_BOOTING:
            message = "Loader failed to boot.";
            break;

        case BS_LOADER_BOOTING_KERNEL:
            message = "Problem encountered while launching the kernel.";
            break;
        }
    }

    /* 
     * If its not NMI but some other exception, always print
     * the register dump. If its NMI, but were not expecting
     * it, as in case of bootsequencing where loader waits
     * for watchdog to fire, we should print register dump.
     */
    if (!is_nmi
        || (is_nmi && boot_status != BS_LOADER_WAITING_FOR_WATCHDOG)) {
        printf("\nU-Boot Exception, ");
        printf("Cause: %s\n", cause_buffer);
        printf("%s\n", message);
        print_trap_frame(trap_frame, 1);

        /* wait a sec to let printfs flush out */
        udelay(1000000);
    }

#if defined (CONFIG_JSRXNLE)
    switch (boot_status) {
    /* 
     * if exception came from within u-boot
     * and if we booted from primary, enable
     * cpld watchdog and wait for backup u-boot
     * to trigger.
     */
    case BS_UBOOT_BOOTING:
        switch(gd->board_desc.board_type) {
        CASE_ALL_SRXLE_MODELS
            printf("Waiting for the backup U-Boot to trigger.\n");

            /* 
             * Disable the CIU watchdog before enabling cpld watchdog
             * because depending on platform CIU watchdog will have
             * shorter timeout and it might trigger even before CPLD
             * watchdog timeout.
             */
            RELOAD_WATCHDOG(DISABLE_WATCHDOG);

            /* enable cpld watchdog */
            RELOAD_CPLD_WATCHDOG(ENABLE_WATCHDOG);

            /* and loop */
            for ( ; ; ) {
                udelay(1000); /* 1 msec */
            }
            break;

        default:
            srxsme_board_reset();
            break;
        }

    default:
        /* reset the box */
        srxsme_board_reset();
        break;
    }
#endif

    octeon_cpu_reset();

    /* we should never come here */
    for( ; ; )
        udelay(1000);   /* 1msec */
}
