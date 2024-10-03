/*
 * Copyright (c) 2010, Juniper Networks, Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 *
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>.
 */


#include <common.h>
#include <command.h>
#include <asm/byteorder.h>

#include "fx_common.h"


static int 
do_diag (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    unsigned int i;

    if (argc == 1 || strcmp(argv[1], "run") != 0) {
        /* List test info */
        if (argc == 1) {
            printf("Available hardware tests:\n");
            post_info(NULL);
            printf("Use 'diag [<test1>]' to get more info.\n");
            printf("Use 'diag run [<test1>]' to run tests.\n");
            printf("Use 'diag run' runs all tests.\n");
        } else {
            for (i = 1; i < argc; i++) {
                if (post_info(argv[i]) != 0)
                    printf("%s - no such test\n", argv[i]);
            }
        }
    } else {
        /* Run tests */
        switch (argc) {
            case 2:
                printf(" Running all tests\n");
                if (post_run_test(NULL) != 0) {
                    printf("unable to execute all test\n");
                }
                break;
            case 3:
                if (post_run_test (argv[2]) != 0) {
                    printf("%s - unable to execute the test\n", argv[2]);
                }
                break;
            default:
                printf("Use 'diag run [<test1>]' to run tests.\n");
                printf("Use 'diag run' runs all tests.\n");
                break;
        }
    }

    return 0;
}


U_BOOT_CMD(
    diag,   CFG_MAXARGS,    0,  do_diag,
    "diag    - perform board diagnostics\n",
    "    - print list of available tests\n"
    "diag [test1 [test2]]\n"
    "    - print information about specified tests\n"
    "diag run - run all available tests\n"
    "diag run [test1 [test2]]\n"
    "    - run specified tests\n"
    );   


