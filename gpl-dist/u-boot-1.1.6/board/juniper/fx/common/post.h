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

#ifndef POST_H
#define POST_H

#include <common.h>
#include <rmi/types.h>
#include <exports.h>
#include "platform_defs.h"

enum psot_verb {
  POST_VERB_STD = 1,
  POST_VERB_DBG = 2,
  POST_VERB_EXT = 4,
  POST_VERB_LOG = 8
};

extern u_int16_t g_board_type;

extern int diag_post_gmac_init(void);
extern int diag_post_gmac_run(void);
extern int diag_post_gmac_clean(void);

extern int diag_post_usb_init(void);
extern int diag_post_usb_run(void);
extern int diag_post_usb_clean(void);

extern int diag_post_multi_core_init(void);
extern int diag_post_multi_core_run(void);
extern int diag_post_multi_core_clean(void);

extern int diag_post_dma_init(void);
extern int diag_post_dma_run(void);
extern int diag_post_dma_clean(void);

extern int diag_post_i2c_init(void);
extern int diag_post_i2c_run(void);
extern int diag_post_i2c_clean(void);

extern int diag_post_usb_storage_init(void);
extern int diag_post_usb_storage_run(void);
extern int diag_post_usb_storage_clean(void);

extern int diag_mac_post_main(void);

/* flags - not all used */
#define POST_POWERON    0x01	/* test runs on power-on booting */
#define POST_NORMAL     0x02	/* test runs on normal booting */
#define POST_SLOWTEST   0x04	/* test is slow, enabled by key press */
#define POST_POWERTEST  0x08	/* test runs after watchdog reset */

#define POST_COLDBOOT   0x80	/* first boot after power-on */

#define POST_ROM        0x0100	/* test runs in ROM */
#define POST_RAM        0x0200	/* test runs in RAM */
#define POST_MANUAL     0x0400	/* test runs on diag command */
#define POST_REBOOT     0x0800	/* test may cause rebooting */
#define POST_PREREL     0x1000  /* test runs before relocation */
#define POST_DEBUG      0x2000 
#define BOOT_POST_FLAG  0x0020 

#define POST_MEM        (POST_RAM | POST_ROM)
#define POST_ALWAYS     (POST_NORMAL    | \
                        POST_SLOWTEST   | \
                        POST_MANUAL     | \
                        POST_POWERON    )
				 
typedef struct DIAG_T {
    char *test_name;
    char *test_cmd;
    char *test_desc;
    int flags;
    int (*init)(void);
    int (*run)(void);
    int (*clean)(void);
} diag_t;



typedef struct post_i2c_st {
    uchar dev_flag;
    uchar dev_addr;
} post_i2c_dev_t;

#define POST_GMAC_LPBK_TEST   \
{  			      \
    POST_GMAC_LPBK_TEST_STR,  \
    "mac_lb",                 \
    "Mac Loopback Test",      \
    POST_POWERON | POST_MANUAL,    \
    diag_post_gmac_init,      \
    diag_post_gmac_run,       \
    diag_post_gmac_clean      \
}

#define POST_USB_PROBE_TEST  \
{  			     \
    POST_USB_PROBE_TEST_STR, \
    "usb_probe",             \
    "USB  Probe Test",       \
    POST_POWERON | POST_MANUAL,    \
    diag_post_usb_init,      \
    diag_post_usb_run,       \
    diag_post_usb_clean      \
}

#define POST_DMA_TXR_TEST          \
{                                  \
    POST_DMA_TXR_TEST_STR,         \
    "dma_engine",                  \
    "DMA Engine Transfer test",    \
    POST_POWERON | POST_MANUAL,    \
    diag_post_dma_init,            \
    diag_post_dma_run,             \
    diag_post_dma_clean            \
}

#define POST_MULT_CORE_MEM_TEST    \
{                                  \
    POST_MULT_CORE_MEM_TEST_STR,   \
    "mcore_mtest",                 \
    "Multi-Core Memory Test",      \
    POST_POWERON | POST_MANUAL,    \
    diag_post_multi_core_init,     \
    diag_post_multi_core_run,      \
    diag_post_multi_core_clean     \
}

#define POST_I2C_PROBE_TEST    \
{                              \
    POST_I2C_PROBE_TEST_STR,   \
    "i2c_probe",               \
    "Generic I2C Probe Test",  \
    POST_POWERON | POST_MANUAL,    \
    diag_post_i2c_init,        \
    diag_post_i2c_run,         \
    diag_post_i2c_clean        \
}

#define POST_USB_STORAGE_TEST   \
{                               \
    POST_USB_STORAGE_TEST_STR,  \
    "usb_storage",              \
    "USB Storage Test",         \
    POST_MANUAL,                \
    diag_post_usb_storage_init, \
    diag_post_usb_storage_run,  \
    diag_post_usb_storage_clean \
}

#define MAX_POST_TESTS              5
#define PLT_POST_TESTS              5

#define POST_TEST_LIST_TERMINATOR \
{                                 \
    0,                            \
    0,                            \
    0,                            \
    0,                            \
    0,                            \
    0                             \
}

#endif
