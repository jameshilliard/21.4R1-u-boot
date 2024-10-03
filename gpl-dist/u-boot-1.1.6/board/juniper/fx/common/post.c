/*
 * Copyright (c) 2009-2013, Juniper Networks, Inc.
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
#include <net.h>
#include <malloc.h>
#include "post.h"

#include <cb/cb_post.h>
#include <soho/soho_post.h>

#define MAX_POST_TIME_LIMIT 10  /* value of 1 = 6 secs */

char post_status[MAX_POST_TESTS + PLT_POST_TESTS];

char post_test_err_str[MAX_POST_TESTS][3][30] = {
        {
           POST_GMAC_RX_ERR_STR,
           POST_GMAC_TX_ERR_STR,
           0, 0, 0
        },
        {
           POST_USB_PROBE_ERR_STR,
           POST_USB_ND_ERR_STR,
           POST_USB_AD_ERR_STR,
           0, 0   
        },
        {
           POST_DMA_NRM_STR,
           POST_DMA_ERR_STR,
           0, 0, 0    
        },
        {
           POST_MULTI_CORE_ERR_STR,
           0, 0, 0, 0
        },
        {
           POST_I2C_PROBE_ND_ERR_STR,
           POST_I2C_PROBE_AD_ERR_STR,
           POST_I2C_PROBE_MUX_ERR_STR,
           0, 0
        }
}; 

int post_verbose = POST_VERB_STD;
int post_enable  = 1;

static diag_t *post_test_g;
static int post_test_size;

int post_printf(int verbosity, const char *fmt, ...) {

    va_list args;
    unsigned int i;
    char printbuffer[1024];

    va_start (args, fmt);

    i = vsprintf (printbuffer, fmt, args);

    va_end (args);


    if ( verbosity & POST_VERB_LOG ) {
         printbuffer[i] = '\0';       
         return (print_2_eeprom(printbuffer, i));
    } 

    if (post_verbose < verbosity) return 0; 

    puts(printbuffer);  

    return i;
}
	    
	   
static int 
post_init (void) 
{
    if (!post_enable) {
        post_printf(POST_VERB_DBG,"POST tests disabled\n");
        return 0;
    }

    post_verbose = simple_strtoul(getenv("post_verbose"), NULL, 16);

    post_printf(POST_VERB_DBG, "Post Verbosity Level : %d\n", post_verbose); 

    /* Run platform specific post if any */
    /* Disable the cpld boot key and set watchdog to MAX_POST_TIME_LIMIT */ 
    if (post_verbose > 1) {
        fx_cpld_set_bootkey();
        fx_cpld_enable_watchdog(1, MAX_POST_TIME_LIMIT);
    }
      
    switch(g_board_type) {

    case QFX3500_BOARD_ID:
    CASE_SE_WESTERNAVENUE_SERIES:   
    case QFX3500_48S4Q_AFO_BOARD_ID:
    CASE_UFABRIC_QFX3600_SERIES:
        post_printf(POST_VERB_DBG,"Running TOR POST tests\n");
        post_test_g = pa_post_tests;
        post_test_size = PA_POST_TESTS_SIZE;
        break;

    case QFX_CONTROL_BOARD_ID:	
        post_printf(POST_VERB_DBG,"Running CB POST tests\n");
        post_test_g = cb_post_tests;
        post_test_size = CB_POST_TESTS_SIZE;
        break;

    case QFX5500_BOARD_ID:
        post_printf(POST_VERB_DBG,"Running SOHO POST tests\n");
        post_test_g = soho_post_tests;
        post_test_size = SOHO_POST_TESTS_SIZE;
        break; 

    default:
        post_printf(POST_VERB_DBG,"Unknown board ID. "
                    "No board specific post being run\n");
        post_test_g = NULL;
        post_test_size = 0;

        return -1;
    }

    return 0;
}


static void 
post_info_single (diag_t *test, int full)
{
	if (full) {
		printf("%s - %s\n"
			   "  %s\n", test->test_cmd, test->test_name, test->test_desc);
	} else {
		printf("  %-15s - %s\n", test->test_cmd, test->test_name);
	}
}


int 
post_info (char *cmd)
{
    if (post_init() == -1) {
        return (-1);
    }

	if (cmd == NULL) {
               while(post_test_g->init != NULL){
                       post_info_single(post_test_g, 0);
                       post_test_g++;
               }
		return (0);
	} else {
               while(post_test_g->init != NULL){
                       if (strcmp(post_test_g->test_cmd, cmd) == 0) {
                           post_info_single(post_test_g, 1);
                           return (0);
                       }
                       post_test_g++;
		}

        return (-1);
	}
}

void 
post_run_test_single (diag_t *post_test)
{
    if (post_test->test_name) {
        printf("\n%s\n", post_test->test_name);
    }
    if (post_test->init) {
        post_test->init();
    }
    if (post_test->run) {
        post_test->run();
    }
    if (post_test->clean) {
        post_test->clean();
    }
}


int 
post_run_test (char *cmd) 
{
    if (post_init() == -1) {
        return (-1);
    }

    if (post_test_g == NULL) {
        printf("post_test is NULL\n");
        return -1; 
    }

	if (cmd == NULL) {
               while(post_test_g->init != NULL){
                       post_run_test_single(post_test_g);
                       post_test_g++;
               }
		return (0);
	} else {
               while(post_test_g->init != NULL){
                       if (strcmp(post_test_g->test_cmd, cmd) == 0) {
                           post_run_test_single(post_test_g);
                           return (0);
                       }
                       post_test_g++;
                }
		return (-1);
	}
}


int 
post_run (char *name, int flags) {
    int status = 1;

    /* Run platform specific post if any*/
    
    if (post_init() == -1) {
        return (-1);
    }

    if (post_test_g == NULL) {
        printf("post_test is NULL\n");
        return -1; 
    }

    /*
     * There are two types supported currently - common tests and platform
     * specific tests. We are running only common tests.
     */
    post_test_nums_set(0, MAX_POST_TESTS);
    post_test_nums_set(1, 0);

    /* Run POST */
    while(post_test_g->init != NULL){
        if ((post_test_g->flags & POST_POWERON) == 0) {
            post_test_g++;
            continue;
        }
        
        if (post_test_g->test_name) {
            post_printf(POST_VERB_DBG,"   %s", post_test_g->test_name);
        }

        if (post_test_g->init) {
            post_test_g->init();
        }

        if (post_test_g->run) {
           if (post_test_g->run() == 0) 
               post_printf(POST_VERB_DBG,"   %s%-12s\n", 
                           post_test_g->test_name, "...PASS");
           else    
               printf("   %s%-12s\n", post_test_g->test_name, "...FAIL");
        }

        if (post_test_g->clean) {
            post_test_g->clean();
        }

        post_test_g++;
    }

    post_printf(POST_VERB_DBG, "Post Done.\n"); 
    return status;
}


