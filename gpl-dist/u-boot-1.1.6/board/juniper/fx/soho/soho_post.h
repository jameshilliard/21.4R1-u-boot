/*
 * Copyright (c) 2010-2011, Juniper Networks, Inc.
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


#ifndef __SOHO_POST_H__
#define __SOHO_POST_H__


/* Netlogic Phys */
#define PER_SEG_SOHO_NLOGICS 4
#define NUM_SOHO_NLOGIC_SEGS 2

#ifdef SOHO_EMI    
#define SOHO_POST_TESTS_SIZE 7
#else
#define SOHO_POST_TESTS_SIZE 6
#endif    

#define PA_POST_TESTS_SIZE 6

void probe_i2c_tree();

int soho_diag_post_i2c_init();
int soho_diag_post_i2c_run();
int soho_diag_post_i2c_clean();

int soho_diag_post_nlogic_init();
int soho_diag_post_nlogic_run();
int soho_diag_post_nlogic_clean();

/* SOHO specific post test defines*/
#define SOHO_POST_I2C_PROBE_TEST   \
{                                  \
    "I2C Probe Test",              \
    "i2c_probe",                   \
    "I2C Probe Test",              \
    POST_MANUAL,                   \
    soho_diag_post_i2c_init,       \
    soho_diag_post_i2c_run,        \
    soho_diag_post_i2c_clean,      \
}


#define SOHO_POST_NETLOGIC_TEST    \
{                                  \
    "Netlogic access Test",        \
    "nl_access",                    \
    "Netlogic access Test",        \
    soho_diag_post_nlogic_init,    \
    soho_diag_post_nlogic_run,     \
    soho_diag_post_nlogic_clean,   \
}


diag_t soho_post_tests[];
diag_t pa_post_tests[ ];

#endif /* __SOHO_POST_H__ */
