/*
 * Copyright (c) 2011-2013, Juniper Networks, Inc.
 * All rights reserved.
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
#include "cmd_jmem.h"

#undef DEBUG
#ifdef DEBUG
#define dprintf(fmt,args...)	printf(fmt, ##args)
#else
#define dprintf(fmt,args...)
#endif

DECLARE_GLOBAL_DATA_PTR;
typedef int (*p2f)(UT_SIZE, UT_SIZE, UT_SIZE);

static char algorithm[MAX_ALGORITHM][10] = 
{
"MATS\0",
"MATS+\0",
"MATS++\0",
"MARCH_X\0",
"MARCH_C--\0",
"MARCH_A\0",
"MARCH_Y\0",
"MARCH_B\0",
"IFA-9\0",
"IFA-13\0"
};

static int algorithm_index = 0;
static int algorithm_stage = 0;
static UT_SIZE errAddr, errPattern;

/*-----------------------------------------------------*/
int p_null (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    return 1;
}

int p_delay (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    dprintf ("%s\n", __FUNCTION__);
    udelay(100000);  /* 100ms */
    return 1;
}

int u_w0 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    dprintf ("%s\n", __FUNCTION__);
    return w0 (LOW_TO_HIGH, saddr, eaddr, pattern);
}

int u_r0 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    dprintf ("%s\n", __FUNCTION__);
    return r0 (LOW_TO_HIGH, saddr, eaddr, pattern);
}

int u_r1 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    dprintf ("%s\n", __FUNCTION__);
    return r1 (LOW_TO_HIGH, saddr, eaddr, pattern);
}

int u_r0_w1 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    dprintf ("%s\n", __FUNCTION__);
    return r0_w1 (LOW_TO_HIGH, saddr, eaddr, pattern);
}

int d_r0_w1 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    dprintf ("%s\n", __FUNCTION__);
    return r0_w1 (HIGH_TO_LOW, saddr, eaddr, pattern);
}

int u_r1_w0 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    dprintf ("%s\n", __FUNCTION__);
    return r1_w0 (LOW_TO_HIGH, saddr, eaddr, pattern);
}

int d_r1_w0 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    dprintf ("%s\n", __FUNCTION__);
    return r1_w0 (HIGH_TO_LOW, saddr, eaddr, pattern);
}

int d_r0_w1_w0 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    dprintf ("%s\n", __FUNCTION__);
    return r0_w1_w0 (HIGH_TO_LOW, saddr, eaddr, pattern);
}

int u_r0_w1_r1 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    dprintf ("%s\n", __FUNCTION__);
    return r0_w1_r1 (LOW_TO_HIGH, saddr, eaddr, pattern);
}

int d_r0_w1_r1 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    dprintf ("%s\n", __FUNCTION__);
    return r0_w1_r1 (HIGH_TO_LOW, saddr, eaddr, pattern);
}

int u_r1_w0_w1 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    dprintf ("%s\n", __FUNCTION__);
    return r1_w0_w1 (LOW_TO_HIGH, saddr, eaddr, pattern);
}

int u_r1_w0_r0 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    dprintf ("%s\n", __FUNCTION__);
    return r1_w0_r0 (LOW_TO_HIGH, saddr, eaddr, pattern);
}

int d_r1_w0_r0 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    dprintf ("%s\n", __FUNCTION__);
    return r1_w0_r0 (HIGH_TO_LOW, saddr, eaddr, pattern);
}

int u_r0_w1_w0_w1 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    dprintf ("%s\n", __FUNCTION__);
    return r0_w1_w0_w1 (LOW_TO_HIGH, saddr, eaddr, pattern);
}

int d_r1_w0_w1_w0 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    dprintf ("%s\n", __FUNCTION__);
    return r1_w0_w1_w0 (HIGH_TO_LOW, saddr, eaddr, pattern);
}

int u_r0_w1_r1_w0_r0_w1 (UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    dprintf ("%s\n", __FUNCTION__);
    return r0_w1_r1_w0_r0_w1 (LOW_TO_HIGH, saddr, eaddr, pattern);
}


void inject_error (void)
{
    UT_SIZE * addr = (UT_SIZE *)errAddr;
    
    if (errAddr)
        *addr = errPattern;
}


void print_error(int step, UT_SIZE addr, UT_SIZE read, UT_SIZE expected)
{
    int bit;
    UT_SIZE mismatch;

    printf("%s stage %d step %d read/verify at address 0x%x "
        "expect 0x%08X got 0x%08X\n",
        algorithm[algorithm_index], algorithm_stage, 
        step, addr, expected, read);

    mismatch = read ^ expected;
    printf ("Mismatch pattern: %08X -> ",mismatch);
    for (bit=0; bit<32; bit++)
        if (mismatch & (0x80000000UL >> bit))
            printf ("DDR-DQ%d ", bit + ((addr&4) ? 32 : 0));
    printf ("\n");
}


/* (w0) */
int w0 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    int i, size;
    volatile UT_SIZE *mem;
    volatile UT_SIZE anti_pattern = (volatile UT_SIZE)(~pattern);

    mem = (volatile UT_SIZE *) saddr;
    size = (eaddr - saddr + 1)  / sizeof (UT_SIZE);

    if (direction) { /* low to high */
        for (i=0; i<size; i++) {
            mem[i] = anti_pattern;
        }
    }
    else { /* high to low */
        for (i=size-1; i>=0; i--) {
            mem[i] = anti_pattern;
        }
    }
    return 1;
}


/* (r0) */
int r0 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    int result = 1;
    int i, size;
    volatile UT_SIZE *mem;
    volatile UT_SIZE anti_pattern = (volatile UT_SIZE)(~pattern);

    mem = (volatile UT_SIZE *) saddr;
    size = (eaddr - saddr + 1)  / sizeof (UT_SIZE);

    inject_error();
    if (direction) { /* low to high */
        for (i=0; i<size; i++) {
            if (mem[i] != anti_pattern) {
                print_error(1, (UT_SIZE)&mem[i], mem[i], anti_pattern);
                result = 0;
            }
        }
    }
    else { /* high to low */
        for (i=size-1; i>=0; i--) {
            if (mem[i] != anti_pattern) {
                print_error(1, (UT_SIZE)&mem[i], mem[i], anti_pattern);
                result = 0;
            }
        }
    }
    return result;
}


/* (r1) */
int r1 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    int result = 1;
    int i, size;
    volatile UT_SIZE *mem;

    mem = (volatile UT_SIZE *) saddr;
    size = (eaddr - saddr + 1)  / sizeof (UT_SIZE);

    inject_error();
    if (direction) { /* low to high */
        for (i=0; i<size; i++) {
            if (mem[i] != pattern) {
                print_error(1, (UT_SIZE)&mem[i], mem[i], pattern);
                result = 0;
            }
        }
    }
    else { /* high to low */
        for (i=size-1; i>=0; i--) {
            if (mem[i] != pattern) {
                print_error(1, (UT_SIZE)&mem[i], mem[i], pattern);
                result = 0;
            }
        }
    }
    return result;
}


/* (r0, w1) */
int r0_w1 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    int result = 1;
    int i, size;
    volatile UT_SIZE *mem;
    volatile UT_SIZE anti_pattern = (volatile UT_SIZE)(~pattern);

    mem = (volatile UT_SIZE *) saddr;
    size = (eaddr - saddr + 1)  / sizeof (UT_SIZE);

    inject_error();
    if (direction) { /* low to high */
        for (i=0; i<size; i++) {
            if (mem[i] != anti_pattern) {
                print_error(1, (UT_SIZE)&mem[i], mem[i], anti_pattern);
                result = 0;
            }
            mem[i] = pattern;
        }
    }
    else { /* high to low */
        for (i=size-1; i>=0; i--) {
            if (mem[i] != anti_pattern) {
                print_error(1, (UT_SIZE)&mem[i], mem[i], anti_pattern);
                result = 0;
            }
            mem[i] = pattern;
        }
    }
    return result;
}


/* (r1, w0) */
int r1_w0 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    int result = 1;
    int i, size;
    volatile UT_SIZE *mem;
    volatile UT_SIZE anti_pattern = (volatile UT_SIZE)(~pattern);

    mem = (volatile UT_SIZE *) saddr;
    size = (eaddr - saddr + 1)  / sizeof (UT_SIZE);

    inject_error();
    if (direction) { /* low to high */
        for (i=0; i<size; i++) {
            if (mem[i] != pattern) {
                print_error(1, (UT_SIZE)&mem[i], mem[i], pattern);
                result = 0;
            }
            mem[i] = anti_pattern;
        }
    }
    else { /* high to low */
        for (i=size-1; i>=0; i--) {
            if (mem[i] != pattern) {
                print_error(1, (UT_SIZE)&mem[i], mem[i], pattern);
                result = 0;
            }
            mem[i] = anti_pattern;
        }
    }
    return result;
}


/* (r0, w1, w0) */
int r0_w1_w0 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    int result = 1;
    int i, size;
    volatile UT_SIZE *mem;
    volatile UT_SIZE anti_pattern = (volatile UT_SIZE)(~pattern);

    mem = (volatile UT_SIZE *) saddr;
    size = (eaddr - saddr + 1)  / sizeof (UT_SIZE);

    inject_error();
    if (direction) { /* low to high */
        for (i=0; i<size; i++) {
            if (mem[i] != anti_pattern) {
                print_error(1, (UT_SIZE)&mem[i], mem[i], anti_pattern);
                result = 0;
            }
            mem[i] = pattern;
            mem[i] = anti_pattern;
        }
    }
    else { /* high to low */
        for (i=size-1; i>=0; i--) {
            if (mem[i] != anti_pattern) {
                print_error(1, (UT_SIZE)&mem[i], mem[i], anti_pattern);
                result = 0;
            }
            mem[i] = pattern;
            mem[i] = anti_pattern;
        }
    }
    return result;
}


/* (r0, w1, r1) */
int r0_w1_r1 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    int result = 1;
    int i, size;
    volatile UT_SIZE *mem;
    volatile UT_SIZE anti_pattern = (volatile UT_SIZE)(~pattern);

    mem = (volatile UT_SIZE *) saddr;
    size = (eaddr - saddr + 1)  / sizeof (UT_SIZE);

    inject_error();
    if (direction) { /* low to high */
        for (i=0; i<size; i++) {
            if (mem[i] != anti_pattern) {
                print_error(1, (UT_SIZE)&mem[i], mem[i], anti_pattern);
                result = 0;
            }
            mem[i] = pattern;
            if (errAddr  && (errAddr == (UT_SIZE)&mem[i]))
                mem[i] = errPattern;
            if (mem[i] != pattern) {
                print_error(3, (UT_SIZE)&mem[i], mem[i], pattern);
                result = 0;
            }
        }
    }
    else { /* high to low */
        for (i=size-1; i>=0; i--) {
            if (mem[i] != anti_pattern) {
                print_error(1, (UT_SIZE)&mem[i], mem[i], anti_pattern);
                result = 0;
            }
            mem[i] = pattern;
            if (errAddr  && (errAddr == (UT_SIZE)&mem[i]))
                mem[i] = errPattern;
            if (mem[i] != pattern) {
                print_error(3, (UT_SIZE)&mem[i], mem[i], pattern);
                result = 0;
            }
        }
    }
    return result;
}


/* (r1, w0, w1) */
int r1_w0_w1 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    int result = 1;
    int i, size;
    volatile UT_SIZE *mem;
    volatile UT_SIZE anti_pattern = (volatile UT_SIZE)(~pattern);

    mem = (volatile UT_SIZE *) saddr;
    size = (eaddr - saddr + 1)  / sizeof (UT_SIZE);

    inject_error();
    if (direction) { /* low to high */
        for (i=0; i<size; i++) {
            if (mem[i] != pattern) {
                print_error(1, (UT_SIZE)&mem[i], mem[i], pattern);
                result = 0;
            }
            mem[i] = anti_pattern;
            mem[i] = pattern;
        }
    }
    else { /* high to low */
        for (i=size-1; i>=0; i--) {
            if (mem[i] != pattern) {
                print_error(1, (UT_SIZE)&mem[i], mem[i], pattern);
                result = 0;
            }
            mem[i] = anti_pattern;
            mem[i] = pattern;
        }
    }
    return result;
}


/* (r1, w0, r0) */
int r1_w0_r0 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    int result = 1;
    int i, size;
    volatile UT_SIZE *mem;
    volatile UT_SIZE anti_pattern = (volatile UT_SIZE)(~pattern);

    mem = (volatile UT_SIZE *) saddr;
    size = (eaddr - saddr + 1)  / sizeof (UT_SIZE);

    inject_error();
    if (direction) { /* low to high */
        for (i=0; i<size; i++) {
            if (mem[i] != pattern) {
                print_error(1, (UT_SIZE)&mem[i], mem[i], pattern);
                result = 0;
            }
            mem[i] = anti_pattern;
            if (errAddr  && (errAddr == (UT_SIZE)&mem[i]))
                mem[i] = errPattern;
            if (mem[i] != anti_pattern) {
                print_error(3, (UT_SIZE)&mem[i], mem[i], anti_pattern);
                result = 0;
            }
        }
    }
    else { /* high to low */
        for (i=size-1; i>=0; i--) {
            if (mem[i] != pattern) {
                print_error(1, (UT_SIZE)&mem[i], mem[i], pattern);
                result = 0;
            }
            mem[i] = anti_pattern;
            if (errAddr  && (errAddr == (UT_SIZE)&mem[i]))
                mem[i] = errPattern;
            if (mem[i] != anti_pattern) {
                print_error(3, (UT_SIZE)&mem[i], mem[i], anti_pattern);
                result = 0;
            }
        }
    }
    return result;
}


/* (r0, w1, w0, w1) */
int r0_w1_w0_w1 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    int result = 1;
    int i, size;
    volatile UT_SIZE *mem;
    volatile UT_SIZE anti_pattern = (volatile UT_SIZE)(~pattern);

    mem = (volatile UT_SIZE *) saddr;
    size = (eaddr - saddr + 1)  / sizeof (UT_SIZE);

    inject_error();
    if (direction) { /* low to high */
        for (i=0; i<size; i++) {
            if (mem[i] != anti_pattern) {
                print_error(1, (UT_SIZE)&mem[i], mem[i], anti_pattern);
                result = 0;
            }
            mem[i] = pattern;
            mem[i] = anti_pattern;
            mem[i] = pattern;
        }
    }
    else { /* high to low */
        for (i=size-1; i>=0; i--) {
            if (mem[i] != anti_pattern) {
                print_error(1, (UT_SIZE)&mem[i], mem[i], anti_pattern);
                result = 0;
            }
            mem[i] = pattern;
            mem[i] = anti_pattern;
            mem[i] = pattern;
        }
    }
    return result;
}


/* (r1, w0, w1, w0) */
int r1_w0_w1_w0 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    int result = 1;
    int i, size;
    volatile UT_SIZE *mem;
    volatile UT_SIZE anti_pattern = (volatile UT_SIZE)(~pattern);

    mem = (volatile UT_SIZE *) saddr;
    size = (eaddr - saddr + 1)  / sizeof (UT_SIZE);

    inject_error();
    if (direction) { /* low to high */
        for (i=0; i<size; i++) {
            if (mem[i] != pattern) {
                print_error(1, (UT_SIZE)&mem[i], mem[i], pattern);
                result = 0;
            }
            mem[i] = anti_pattern;
            mem[i] = pattern;
            mem[i] = anti_pattern;
        }
    }
    else { /* high to low */
        for (i=size-1; i>=0; i--) {
            if (mem[i] != pattern) {
                print_error(1, (UT_SIZE)&mem[i], mem[i], pattern);
                result = 0;
            }
            mem[i] = anti_pattern;
            mem[i] = pattern;
            mem[i] = anti_pattern;
        }
    }
    return result;
}


/* (r0, w1, r1, w0, r0, w1) */
int r0_w1_r1_w0_r0_w1 (int direction, UT_SIZE saddr, UT_SIZE eaddr, UT_SIZE pattern)
{
    int result = 1;
    int i, size;
    volatile UT_SIZE *mem;
    volatile UT_SIZE anti_pattern = (volatile UT_SIZE)(~pattern);

    mem = (volatile UT_SIZE *) saddr;
    size = (eaddr - saddr + 1)  / sizeof (UT_SIZE);

    inject_error();
    if (direction) { /* low to high */
        for (i=0; i<size; i++) {
            if (mem[i] != anti_pattern) {
                print_error(1, (UT_SIZE)&mem[i], mem[i], anti_pattern);
                result = 0;
            }
            mem[i] = pattern;
            if (errAddr  && (errAddr == (UT_SIZE)&mem[i]))
                mem[i] = errPattern;
            if (mem[i] != pattern) {
                print_error(3, (UT_SIZE)&mem[i], mem[i], pattern);
                result = 0;
            }
            mem[i] = anti_pattern;
            if (errAddr  && (errAddr == (UT_SIZE)&mem[i]))
                mem[i] = errPattern;
            if (mem[i] != anti_pattern) {
                print_error(5, (UT_SIZE)&mem[i], mem[i], anti_pattern);
                result = 0;
            }
            mem[i] = pattern;
        }
    }
    else { /* high to low */
        for (i=size-1; i>=0; i--) {
            if (mem[i] != anti_pattern) {
                print_error(1, (UT_SIZE)&mem[i], mem[i], anti_pattern);
                result = 0;
            }
            mem[i] = pattern;
            if (errAddr  && (errAddr == (UT_SIZE)&mem[i]))
                mem[i] = errPattern;
            if (mem[i] != pattern) {
                print_error(3, (UT_SIZE)&mem[i], mem[i], pattern);
                result = 0;
            }
            mem[i] = anti_pattern;
            if (errAddr  && (errAddr == (UT_SIZE)&mem[i]))
                mem[i] = errPattern;
            if (mem[i] != anti_pattern) {
                print_error(5, (UT_SIZE)&mem[i], mem[i], anti_pattern);
                result = 0;
            }
            mem[i] = pattern;
        }
    }
    return result;
}



/* memory algorithm commands
 *
 * Syntax:
 */
int do_malgo(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    ulong mstart, mend, size, id;
    UT_SIZE pattern;
    int i, j, mloop = 0, k = 0, ret, testpassed = 1;
    static p2f algorithm_sequence[MAX_ALGORITHM][MAX_STAGE];
    static int malgo_init = 0;

    if (malgo_init == 0) {
        printf("memory algorithm init.\n");
        for (i=0; i<MAX_ALGORITHM; i++) {
            for (j=0; j<MAX_STAGE; j++) {
                algorithm_sequence[i][j] = p_null;
            }
        }

        /* init individual test array */
        /* MATS {^(w0); v(r0, w1); ^(r1)} */
        algorithm_sequence[0][0] = u_w0;
        algorithm_sequence[0][1] = d_r0_w1;
        algorithm_sequence[0][2] = u_r1;
        /* MATS+ {^(w0); ^(r0, w1); v(r1, w0)} */
        algorithm_sequence[1][0] = u_w0;
        algorithm_sequence[1][1] = u_r0_w1;
        algorithm_sequence[1][2] = d_r1_w0;
        /* MATS++ {^(w0); ^(r0, w1); v(r1, w0, r0)} */
        algorithm_sequence[2][0] = u_w0;
        algorithm_sequence[2][1] = u_r0_w1;
        algorithm_sequence[2][2] = d_r1_w0_r0;
        /* MARCH X {^(w0); ^(r0, w1); v(r1, w0); ^(r0)} */
        algorithm_sequence[3][0] = u_w0;
        algorithm_sequence[3][1] = u_r0_w1;
        algorithm_sequence[3][2] = d_r1_w0;
        algorithm_sequence[3][3] = u_r0;
        /* MARCH C-- {^(w0); ^(r0, w1); ^(r1, w0); v(r0, w1); v(r1, w0); ^(r0)} */
        algorithm_sequence[4][0] = u_w0;
        algorithm_sequence[4][1] = u_r0_w1;
        algorithm_sequence[4][2] = u_r1_w0;
        algorithm_sequence[4][3] = d_r0_w1;
        algorithm_sequence[4][4] = d_r1_w0;
        algorithm_sequence[4][5] = u_r0;
        /* MARCH A {^(w0); ^(r0, w1, w0, w1); ^(r1, w0, w1); v(r1, w0, w1, w0); v(r0, w1, w0)} */
        algorithm_sequence[5][0] = u_w0;
        algorithm_sequence[5][1] = u_r0_w1_w0_w1;
        algorithm_sequence[5][2] = u_r1_w0_w1;
        algorithm_sequence[5][3] = d_r1_w0_w1_w0;
        algorithm_sequence[5][4] = d_r0_w1_w0;
        /* MARCH Y {^(w0); ^(r0, w1, r1); v(r1, w0, r0); ^(r0)} */
        algorithm_sequence[6][0] = u_w0;
        algorithm_sequence[6][1] = u_r0_w1_r1;
        algorithm_sequence[6][2] = d_r1_w0_r0;
        algorithm_sequence[6][3] = u_r0;
        /* MARCH B {^(w0); ^(r0, w1, r1, w0, r0, w1); ^(r1, w0, w1); v(r1, w0, w1, w0); v(r0, w1, w0)} */
        algorithm_sequence[7][0] = u_w0;
        algorithm_sequence[7][1] = u_r0_w1_r1_w0_r0_w1;
        algorithm_sequence[7][2] = u_r1_w0_w1;
        algorithm_sequence[7][3] = d_r1_w0_w1_w0;
        algorithm_sequence[7][4] = d_r0_w1_w0;
        /* IFA-9 {^(w0); ^(r0, w1); ^(r1, w0); v(r0, w1); v(r1, w0); Delay; ^(r0, w1); Delay; ^(r1)} */
        algorithm_sequence[8][0] = u_w0;
        algorithm_sequence[8][1] = u_r0_w1;
        algorithm_sequence[8][2] = u_r1_w0;
        algorithm_sequence[8][3] = d_r0_w1;
        algorithm_sequence[8][4] = d_r1_w0;
        algorithm_sequence[8][5] = p_delay;
        algorithm_sequence[8][6] = u_r0_w1;
        algorithm_sequence[8][7] = p_delay;
        algorithm_sequence[8][8] = u_r1;
        /* IFA-13 {^(w0); ^(r0, w1, r1); ^(r1, w0, r0); v(r0, w1, r1); v(r1, w0, r0); Delay; ^(r0, w1); Delay; ^(r1)} */
        algorithm_sequence[9][0] = u_w0;
        algorithm_sequence[9][1] = u_r0_w1_r1;
        algorithm_sequence[9][2] = u_r1_w0_r0;
        algorithm_sequence[9][3] = d_r0_w1_r1;
        algorithm_sequence[9][4] = d_r1_w0_r0;
        algorithm_sequence[9][5] = p_delay;
        algorithm_sequence[9][6] = u_r0_w1;
        algorithm_sequence[9][7] = p_delay;
        algorithm_sequence[9][8] = u_r1;

        malgo_init = 1;
    }

    if (argc <= 4)
        goto usage;
    
    id = simple_strtoul(argv[1], NULL, 10);
    mstart = simple_strtoul(argv[2], NULL, 16);
    if (mstart % sizeof(UT_SIZE)) {
        size = mstart / sizeof(UT_SIZE);
        mstart = (size + 1) * size;
    }
    mend = simple_strtoul(argv[3], NULL, 16);
#if defined(CONFIG_EX3242) ||defined(CONFIG_EX45XX)
    if (mstart < CONFIG_MEM_RELOCATE_TOP - (1 << 20)) {
        if (mend > CONFIG_MEM_RELOCATE_TOP - (1 << 20)) {
            printf("ERROR: start and end address range can not include"
                   "0x%08lx to 0x%081x",
                   CONFIG_MEM_RELOCATE_TOP - (1 << 20),
                   CONFIG_MEM_RELOCATE_TOP + 0x2000);
            return 1;
        }
    } else if (mstart >= CONFIG_MEM_RELOCATE_TOP - (1 << 20) &&
        mstart < CONFIG_MEM_RELOCATE_TOP + 0x2000) {
        printf("ERROR: start address 0x%08lx can not be between"
               "0x%08lx and 0x%08lx\n", mstart,
               CONFIG_MEM_RELOCATE_TOP - (1 << 20),
               CONFIG_MEM_RELOCATE_TOP + 0x2000);
        return 1;
    } else if (mend > (gd->ram_size - 0x2000)) {
        printf("ERROR: end address 0x%08x greater than maximum address"
	        "0x%08lx", mend, gd->ram_size - 0x2000);
        return 1;
    }
#endif
#if defined(CONFIG_EX2200) || \
    defined(CONFIG_RPS200) || \
    defined(CONFIG_EX3300)
    if (mstart < 0x400000) {
        printf("ERROR: start address 0x%08x less than minimum address"
               "0x400000\n",
               mstart);
        return 1;
    }
    if (mend > (gd->ram_size)) {
        printf("ERROR: end address 0x%08x greater than maximum address"
               "0x%08x\n",
               mend, gd->ram_size);
        return 1;
    }
#endif
    size = (mend - mstart) / sizeof(UT_SIZE);
    mend = mstart + size * sizeof(UT_SIZE) - 1;

#ifdef RAMTEST_64BIT
    pattern = simple_strtoull(argv[4], NULL, 16);
#ifdef LITTLEENDIAN
    pattern = swap_ull(pattern);
#endif
#else
    pattern = simple_strtoul(argv[4], NULL, 16);
#endif
    if (argc >= 6)
        mloop = simple_strtol(argv[5], NULL, 10);
    errAddr = 0;
    errPattern = ERR_PATTERN;
    if (argc == 8) {
        errAddr = simple_strtoul(argv[6], NULL, 16);
#ifdef RAMTEST_64BIT
        errPattern = simple_strtoull(argv[7], NULL, 16);
#ifdef LITTLEENDIAN
        errPattern = swap_ull(errPattern);
#endif
#else
        errPattern = simple_strtoul(argv[7], NULL, 16);
#endif
    }

    if (errAddr)
#ifdef RAMTEST_64BIT
        printf("Memory algorithm test: start 0x%x end 0x%x pattern %s "
               "loop %d error address 0x%x error pattern %s.\n", 
               mstart, mend, argv[4], 
               mloop, 
               errAddr, argv[7]);
#else
        printf("Memory algorithm test: start 0x%lx end 0x%lx pattern 0x%08x "
               "loop %d error address 0x%x error pattern 0x%08x.\n", 
               mstart, mend, pattern, mloop, errAddr, errPattern);
#endif
    else 
#ifdef RAMTEST_64BIT
        printf("Memory algorithm test: start 0x%x end 0x%x pattern %s "
               "loop %s.\n", 
               mstart, mend, 
               argv[4], argv[5]);
#else
        printf("Memory algorithm test: start 0x%lx end 0x%lx pattern 0x%08x "
               "loop %d.\n", mstart, mend, pattern, mloop);
#endif

    if ((id >=1) && (id <=MAX_ALGORITHM)) {
        algorithm_index = id-1;
        while ((mloop == 0) ||(k < mloop)) {
            printf("\nLoop: %d\n", k+1);
            for (j=0; j<MAX_STAGE; j++) {
                if (algorithm_sequence[algorithm_index][j] != p_null) {
                    algorithm_stage = j;
                    if ((ret = algorithm_sequence[algorithm_index][j](mstart, mend, pattern)) == 0) {
                        printf("-12%s: [%5d, %5d] Failed at test %d "
                               "stage %d.\n", 
                               algorithm[algorithm_index],
                               single_bit_ecc_errors(),
                               double_bits_ecc_errors(),
                               k, j);
                        testpassed = 0;
                    }
                }
            }
            k++;
            if (ctrlc()) {
                putc ('\n');
                return 1;
            }
        }
        if (testpassed)
            printf("%-12s: [%5d, %5d] Passed\n", 
                   algorithm[algorithm_index],
                   single_bit_ecc_errors(),
                   double_bits_ecc_errors());
    }
    else if (id == 11) {
        while ((mloop == 0) ||(k < mloop)) {
            printf("\nLoop: %d\n", k+1);
            for (i=0; i<MAX_ALGORITHM; i++) {
                algorithm_index = i;
                testpassed = 1;
                for (j=0; j<MAX_STAGE; j++) {
                    if (algorithm_sequence[i][j] != p_null) {
                        algorithm_stage = j;
                        if ((ret = algorithm_sequence[i][j](mstart, mend, pattern)) == 0) {
                            printf("-12%s: [%5d, %5d] Failed at test %d "
                                   "stage %d.\n", 
                                   algorithm[i],
                                   single_bit_ecc_errors(),
                                   double_bits_ecc_errors(),
                                   k, j);
                            testpassed = 0;
                        }
                    }
                }
                if (testpassed)
                    printf("%-12s: [%5d, %5d] Passed\n", 
                           algorithm[i],
                           single_bit_ecc_errors(),
                           double_bits_ecc_errors());
                
                if (ctrlc()) {
                    putc ('\n');
                    return 1;
                }
            }
            k++;
        }
    }
    else
        goto usage;

    printf("Memory algorithm test done.\n");
    
    return 1;
 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return 1;
}


/*
 * Perform a memory test. 
 */
int memtest (ulong saddr, ulong eaddr, int mloop, int display)
{
       vu_long *start, *end, *addr;
	ulong	val;
	ulong	readback;

	vu_long	addr_mask;
	vu_long	offset;
	vu_long	test_offset;
	vu_long	pattern;
	vu_long	temp;
	vu_long	anti_pattern;
	vu_long	num_words;
	vu_long *dummy = 0;	/* yes, this is address 0x0, not NULL */
	int	i, j;
	int iterations = 1;

	static const ulong bitpattern[] = {
		0x00000001,	/* single bit */
		0x00000003,	/* two adjacent bits */
		0x00000007,	/* three adjacent bits */
		0x0000000F,	/* four adjacent bits */
		0x00000005,	/* two non-adjacent bits */
		0x00000015,	/* three non-adjacent bits */
		0x00000055,	/* four non-adjacent bits */
		0xaaaaaaaa,	/* alternating 1/0 */
	};

       start = (vu_long *)saddr;
       end = (vu_long *)eaddr;
       
       if (display)
           printf("%s: start = 0x%.8lx, end = 0x%.8lx, loop = %d\n",
			__FUNCTION__, saddr, eaddr, mloop);
       
	for (i=0; ((i<mloop) || (mloop == 0)); i++) {
		if (ctrlc()) {
			putc ('\n');
			return 0;
		}

              if (display) {
		    printf("Iteration: %6d\n", iterations);
              }
		iterations++;

		/*
		 * Data line test: write a pattern to the first
		 * location, write the 1's complement to a 'parking'
		 * address (changes the state of the data bus so a
		 * floating bus doen't give a false OK), and then
		 * read the value back. Note that we read it back
		 * into a variable because the next time we read it,
		 * it might be right (been there, tough to explain to
		 * the quality guys why it prints a failure when the
		 * "is" and "should be" are obviously the same in the
		 * error message).
		 *
		 * Rather than exhaustively testing, we test some
		 * patterns by shifting '1' bits through a field of
		 * '0's and '0' bits through a field of '1's (i.e.
		 * pattern and ~pattern).
		 */
		addr = start;
		for (j = 0; j < sizeof(bitpattern)/sizeof(bitpattern[0]); j++) {
		    val = bitpattern[j];
		    for(; val != 0; val <<= 1) {
			*addr  = val;
			*dummy  = ~val; /* clear the test data off of the bus */
			readback = *addr;
			if(readback != val) {
			     printf ("FAILURE (data line): "
				"expected %08lx, actual %08lx\n",
					  val, readback);
			}
			*addr  = ~val;
			*dummy  = val;
			readback = *addr;
			if(readback != ~val) {
			    printf ("FAILURE (data line): "
				"Is %08lx, should be %08lx\n",
					readback, ~val);
			}
		    }
		}

		/*
		 * Based on code whose Original Author and Copyright
		 * information follows: Copyright (c) 1998 by Michael
		 * Barr. This software is placed into the public
		 * domain and may be used for any purpose. However,
		 * this notice must not be changed or removed and no
		 * warranty is either expressed or implied by its
		 * publication or distribution.
		 */

		/*
		 * Address line test
		 *
		 * Description: Test the address bus wiring in a
		 *              memory region by performing a walking
		 *              1's test on the relevant bits of the
		 *              address and checking for aliasing.
		 *              This test will find single-bit
		 *              address failures such as stuck -high,
		 *              stuck-low, and shorted pins. The base
		 *              address and size of the region are
		 *              selected by the caller.
		 *
		 * Notes:	For best results, the selected base
		 *              address should have enough LSB 0's to
		 *              guarantee single address bit changes.
		 *              For example, to test a 64-Kbyte
		 *              region, select a base address on a
		 *              64-Kbyte boundary. Also, select the
		 *              region size as a power-of-two if at
		 *              all possible.
		 *
		 * Returns:     0 if the test succeeds, 1 if the test fails.
		 *
		 * ## NOTE ##	Be sure to specify start and end
		 *              addresses such that addr_mask has
		 *              lots of bits set. For example an
		 *              address range of 01000000 02000000 is
		 *              bad while a range of 01000000
		 *              01ffffff is perfect.
		 */
		addr_mask = ((ulong)end - (ulong)start)/sizeof(vu_long);
		pattern = (vu_long) 0xaaaaaaaa;
		anti_pattern = (vu_long) 0x55555555;

		/*
		 * Write the default pattern at each of the
		 * power-of-two offsets.
		 */
		for (offset = 1; (offset & addr_mask) != 0; offset <<= 1) {
			start[offset] = pattern;
		}

		/*
		 * Check for address bits stuck high.
		 */
		test_offset = 0;
		start[test_offset] = anti_pattern;

		for (offset = 1; (offset & addr_mask) != 0; offset <<= 1) {
		    temp = start[offset];
		    if (temp != pattern) {
                     if (display)
			    printf ("\nFAILURE: Address bit stuck high @ 0x%.8lx:"
				" expected 0x%.8lx, actual 0x%.8lx\n",
				(ulong)&start[offset], pattern, temp);
			return 1;
		    }
		}
		start[test_offset] = pattern;

		/*
		 * Check for addr bits stuck low or shorted.
		 */
		for (test_offset = 1; (test_offset & addr_mask) != 0; test_offset <<= 1) {
		    start[test_offset] = anti_pattern;

		    for (offset = 1; (offset & addr_mask) != 0; offset <<= 1) {
			temp = start[offset];
			if ((temp != pattern) && (offset != test_offset)) {
                         if (display)
			        printf ("\nFAILURE: Address bit stuck low or shorted @"
				    " 0x%.8lx: expected 0x%.8lx, actual 0x%.8lx\n",
				    (ulong)&start[offset], pattern, temp);
			    return 1;
			}
		    }
		    start[test_offset] = pattern;
		}

		/*
		 * Description: Test the integrity of a physical
		 *		memory device by performing an
		 *		increment/decrement test over the
		 *		entire region. In the process every
		 *		storage bit in the device is tested
		 *		as a zero and a one. The base address
		 *		and the size of the region are
		 *		selected by the caller.
		 *
		 * Returns:     0 if the test succeeds, 1 if the test fails.
		 */
		num_words = ((ulong)end - (ulong)start)/sizeof(vu_long) + 1;

		/*
		 * Fill memory with a known pattern.
		 */
		for (pattern = 1, offset = 0; offset < num_words; pattern++, offset++) {
			start[offset] = pattern;
		}

		/*
		 * Check each location and invert it for the second pass.
		 */
		for (pattern = 1, offset = 0; offset < num_words; pattern++, offset++) {
		    temp = start[offset];
		    if (temp != pattern) {
                     if (display)
			    printf ("\nFAILURE (read/write) @ 0x%.8lx:"
				" expected 0x%.8lx, actual 0x%.8lx)\n",
				(ulong)&start[offset], pattern, temp);
			return 1;
		    }

		    anti_pattern = ~pattern;
		    start[offset] = anti_pattern;
		}

		/*
		 * Check each location for the inverted pattern and zero it.
		 */
		for (pattern = 1, offset = 0; offset < num_words; pattern++, offset++) {
		    anti_pattern = ~pattern;
		    temp = start[offset];
		    if (temp != anti_pattern) {
                     if (display)
			    printf ("\nFAILURE (read/write): @ 0x%.8lx:"
				" expected 0x%.8lx, actual 0x%.8lx)\n",
				(ulong)&start[offset], anti_pattern, temp);
			return 1;
		    }
		    start[offset] = 0;
		}
	}
	return 0;
}

/*
 * Perform a memory test. 
 */
int do_memtest (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    ulong start, end;
    int     mloop = 1;

    if (argc <= 2)
        goto usage;
    
    start = simple_strtoul(argv[1], NULL, 16);

    end = simple_strtoul(argv[2], NULL, 16);

    if (argc > 3) {
        mloop = (ulong)simple_strtoul(argv[3], NULL, 16);
    }
    
    printf ("Testing %08x ... %08x:\n", (uint)start, (uint)end);

    return memtest(start, end, mloop, 1);
       
 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return 1;
}

/***************************************************/

U_BOOT_CMD(
    malgo,	 8, 1, do_malgo,
    "malgo   - Memory test algorithm commands\n",
    "\n"
    "malgo <algorithm> <start address> <end address> <pattern> <loop> "
    "[<error address> <error pattern>]\n"
    "    - memory test for selected algorithm.\n"
    "       1. MATS\n"
    "       2. MATS+\n"
    "       3. MATS++\n"
    "       4. MARCH X\n"
    "       5. MARCH C--\n"
    "       6. MARCH A\n"
    "       7. MARCH Y\n"
    "       8. MARCH B\n"
    "       9. IFA-9\n"
    "      10. IFA-13\n"
    "      11. all\n"
#if 0
    "          ^(up), v(down), w(write), r(read/verify), 0(anti-pattern), 1(pattern), Delay(100ms)\n"
    "       1. MATS      {^(w0);v(r0,w1);^(r1)}\n"
    "       2. MATS+     {^(w0);^(r0,w1);v(r1,w0)}\n"
    "       3. MATS++    {^(w0);^(r0,w1);v(r1,w0,r0)}\n"
    "       4. MARCH X   {^(w0);^(r0,w1);v(r1,w0);^(r0)}\n"
    "       5. MARCH C-- {^(w0);^(r0,w1);^(r1,w0);v(r0,w1);v(r1,w0);^(r0)}\n"
    "       6. MARCH A   {^(w0);^(r0,w1,w0,w1);^(r1,w0,w1);v(r1,w0,w1,w0);v(r0,w1,w0)}\n"
    "       7. MARCH Y   {^(w0);^(r0,w1,r1);v(r1,w0,r0);^(r0)}\n"
    "       8. MARCH B   {^(w0);^(r0,w1,r1,w0,r0,w1);^(r1,w0,w1);v(r1,w0,w1,w0);v(r0,w1,w0)}\n"
    "       9. IFA-9     {^(w0);^(r0,w1);^(r1,w0);v(r0,w1);v(r1,w0);Delay;^(r0,w1);Delay;^(r1)}\n"
    "      10. IFA-13    {^(w0);^(r0,w1,r1);^(r1,w0,r0);v(r0,w1,r1);v(r1,w0,r0); Delay;^(r0,w1);Delay;^(r1)}\n"
    "      11. all\n"
#endif
);

U_BOOT_CMD(
    memtest,    5,    1,     do_memtest,
    "memtest - simple RAM memory test\n",
    "\n"
    "memtest <start><end>[<loop>]\n"
    "    - simple RAM memory read/write test\n"
);

