/*
 * Copyright (c) 2009-2010, Juniper Networks, Inc.
 * All rights reserved.
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
 * You should have received a copy of the GNU General Public
 * License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <watchdog.h>
#include <command.h>
#include <rmi/pci-phoenix.h>
#include <rmi/on_chip.h>
#include <rmi/byteorder.h>
#include <rmi/gpio.h>
#include <rmi/bridge.h>
#include <malloc.h>

#include <common/post.h>

#undef  DEBUG


#define DIAG_CPU      8
#define DIAG_THEAD    4


static unsigned int cpu_state[DIAG_CPU][DIAG_THEAD];
static unsigned int cpu_data[DIAG_CPU][DIAG_THEAD];
static unsigned int cpu_malloc[DIAG_CPU][DIAG_THEAD];
static unsigned int cpu_valid[DIAG_CPU][DIAG_THEAD];


#define DIAG_MULTI_MEM_64_BYTE    64



static __inline__ int
phnx_thr (void)
{
    unsigned int cpu = 0;

    __asm__ volatile (".set push\n"
                      ".set mips64\n"
                      "mfc0 %0, $15, 1\n" ".set pop\n" : "=r" (cpu)
                      );
    return cpu & 0x1F;
}


int diag_post_multi_core_init(void);

int
diag_post_multi_core_init ()
{
    int i, j;

    debug("%s:\n", __FUNCTION__);

    for (i = 0; i < DIAG_CPU; i++) {
        for (j = 0; j < DIAG_THEAD; j++) {
            cpu_state[i][j]  = 0xAAAA5555;
            cpu_data[i][j]   = 0xDEADBEEF;
            cpu_malloc[i][j] = (unsigned int)malloc(64);
            cpu_valid[i][j]  = 0;
        }
    }

    return 1;
}


int diag_post_multi_core_run(void);



int
diag_post_multi_code_test (int *count)

{
    int cpu_id = 0;
    char *mem_test_ptr;
    int i;

    cpu_id = phnx_thr();
    mem_test_ptr = (char *)cpu_malloc[cpu_id >> 0x2][cpu_id & 0x3];
    cpu_valid[cpu_id >> 0x2][cpu_id & 0x3] = 1;
    for (i = 0; i < 64; i++) {
        *mem_test_ptr++ = i;
    }

    return 1;
}


typedef void (*smp_call)(void *);

extern void rmi_wakeup_secondary_cpus(smp_call fn,
                                      void *args,
                                      uint32_t cpu_mask);

int
diag_post_multi_core_run (void)
{
    int mask = 0xFFE;

    int count = 0;

    debug("%s:\n", __FUNCTION__);

    rmi_wakeup_secondary_cpus((smp_call)diag_post_multi_code_test, &count, mask);

    return 0;
}


int diag_post_multi_core_clean(void);

int
diag_post_multi_core_clean (void)
{
    char *mem_test_ptr;
    int i, j, k;
    int status = 0;

    debug("%s:\n", __FUNCTION__);

    udelay(100); 
    for (i = 0; i < DIAG_CPU; i++) {
        for (j = 0; j < DIAG_THEAD; j++) {
            if (cpu_valid[i][j] == 0x1) {
                mem_test_ptr    = (char *)cpu_malloc[i][j];
                for (k = 0; k < 64; k++) {
                    if (mem_test_ptr[k] != k) {
                        post_printf(POST_VERB_STD,"Error: Multi-core memory test failed \n");
                        post_printf(POST_VERB_STD,
                            "Core %d -Thread %d Address=0x%08x Expected=0x%08x Actual=0x%08x \n",
                            i,
                            j,
                            &mem_test_ptr[k],
                            k,
                            mem_test_ptr[k]);
                        status = -1;
                    }
                }
            }
            if ((status == 0) && (cpu_valid[i][j] == 0x1)) {
                post_printf(POST_VERB_DBG,"Pass Memory test for cpu=%d thread=%d\n", i, j);
            }
            free((void*)cpu_malloc[i][j]);
        }
    }

    if (status == -1) {
        post_test_status_set(POST_MULT_CORE_MEM_ID, POST_MULTI_CORE_ERROR); 
    } 

    return status;
}


typedef struct DIAG_MULTI_CORE_T {
    char *test_name;
    int (*init)(void);
    int (*run)(void);
    int (*clean)(void);
} diag_multi_core_t;


diag_multi_core_t diag_post_multi_core = {
    "Multi-Core Memory Test\n",
    diag_post_multi_core_init,
    diag_post_multi_core_run,
    diag_post_multi_core_clean
};


int
diag_multi_core_post_main (void)
{
    int status = 0;


    debug("%s: \n", __FUNCTION__);

    post_printf(POST_VERB_STD,"Test Start: %s \n", diag_post_multi_core.test_name);

    status = diag_post_multi_core.init();

    if (status == 0) {
        goto diag_multi_core_post_main_exit;
    }

    status = diag_post_multi_core.run();

    if (status == 0) {
        goto diag_multi_core_post_main_exit;
    }




diag_multi_core_post_main_exit:

    diag_post_multi_core.clean();
    post_printf(POST_VERB_STD,"%s %s \n",
           diag_post_multi_core.test_name,
           status == 1 ? "PASS" : "FAILURE");
    post_printf(POST_VERB_STD,"Test End: %s \n", diag_post_multi_core.test_name);
    return status;
}


int
diag_post_multi_core_main (void)
{
    int status = 1;

    status = diag_multi_core_post_main();
    return status;
}


