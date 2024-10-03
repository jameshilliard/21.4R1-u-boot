/*
 * Copyright (c) 2008-2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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

/*
 *  Command Processor Table
 */

#include "rps.h"
#include <exports.h>
#include "rps_command.h"

#define COMMAND(name) \
extern cmd_tbl_t  __u_boot_cmd_##name;
#include "rps_command.def"
#undef COMMAND

enum command_index {
#define COMMAND(name) COMMAND_##name ,
#include "rps_command.def"
#undef COMMAND
    COMMAND_COUNT
};

p_cmd_tbl_t command_handlers[] = {
#define COMMAND(name) &__u_boot_cmd_##name ,
#include "rps_command.def"
#undef COMMAND
    NULL
};

extern int ctrlc(void);

#if 0
int
do_version (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	extern char rps_version_string;
	printf ("\n%s\n", rps_version_string);
	return (0);
}

U_BOOT_CMD(
	version,	1,		1,	do_version,
 	"version - print monitor version\n",
	NULL
);
#endif

extern int rps_loop;
int
do_exit (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int r;
	extern int32_t clear_mboxes(void);

	r = 0;
	if (argc > 1)
		r = simple_strtoul(argv[1], NULL, 10);
    
	printf("Warning: "
           "exit will stop RPS functionality.  Please enter <yes> to confirm.\n");

	rps_loop = 0;
	watchdog_disable();  /* disable watchdog before going to U-boot */
	if (getc() == 'y') {
	} 
	else 
	{
		rps_loop = 1;
		watchdog_enable();
		printf("exit aborted\n");
		return (-1);
	}
    
	clear_mboxes();  /* RPS down */
    
	return (-r - 2);
}

U_BOOT_CMD(
	exit,	2,	1,	do_exit,
 	"exit    - exit script\n",
	"    - exit functionality\n"
);

/*
 * Use puts() instead of printf() to avoid printf buffer overflow
 * for long help messages
 */
int 
do_help (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	int i;
	int rcode = 0;

	if (argc == 1) {	/*show list of commands */

		int cmd_items = COMMAND_COUNT;
		cmd_tbl_t *cmd_array[cmd_items];
		int i, j, swaps;

		/* Make array of commands from .uboot_cmd section */
		cmdtp = command_handlers[0];
		for (i = 0; i < cmd_items; i++) {
			cmd_array[i] = command_handlers[i];
		}

		/* Sort command list (trivial bubble sort) */
		for (i = cmd_items - 1; i > 0; --i) {
			swaps = 0;
			for (j = 0; j < i; ++j) {
				if (strcmp (cmd_array[j]->name,
					    cmd_array[j + 1]->name) > 0) {
					cmd_tbl_t *tmp;
					tmp = cmd_array[j];
					cmd_array[j] = cmd_array[j + 1];
					cmd_array[j + 1] = tmp;
					++swaps;
				}
			}
			if (!swaps)
				break;
		}

		/* print short help (usage) */
		for (i = 0; i < cmd_items; i++) {
			const char *usage = cmd_array[i]->usage;

			/* allow user abort */
			if (ctrlc ())
				return (1);
			if (usage == NULL)
				continue;
			puts (usage);
		}
		return (0);
	}
	/*
	 * command help (long version)
	 */
	for (i = 1; i < argc; ++i) {
		if ((cmdtp = find_cmd (argv[i])) != NULL) {
#ifdef	CFG_LONGHELP
			/* found - print (long) help info */
			puts (cmdtp->name);
			putc (' ');
			if (cmdtp->help) {
				puts (cmdtp->help);
			} else {
				puts ("- No help available.\n");
				rcode = 1;
			}
			putc ('\n');
#else	/* no long help available */
			if (cmdtp->usage)
				puts (cmdtp->usage);
#endif	/* CFG_LONGHELP */
		} else {
			printf ("Unknown command '%s' - try 'help'"
				" without arguments for list of all"
				" known commands\n\n", argv[i]
					);
			rcode = 1;
		}
	}
	return (rcode);
}


U_BOOT_CMD(
	help,	CFG_MAXARGS,	1,	do_help,
 	"help    - print online help\n",
 	"[command ...]\n"
 	"    - show help information (for 'command')\n"
 	"'help' prints online help for the monitor commands.\n\n"
 	"Without arguments, it prints a short usage message for all commands.\n\n"
 	"To get detailed help information for specific commands you can type\n"
  "'help' with one or more command names as arguments.\n"
);

int 
do_sysreset (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    do_reset();
    return (0);
}

U_BOOT_CMD(
	reset, CFG_MAXARGS, 1,	do_sysreset,
	"reset   - Perform RESET of the system\n",
	NULL
);

/***************************************************************************
 * find command table entry for a command
 */
cmd_tbl_t *
find_cmd (const char *cmd)
{
	cmd_tbl_t *cmdtp;
	cmd_tbl_t *cmdtp_temp = NULL;	/*Init value */
	const char *p;
	int len;
	int n_found = 0;
	int i;

	/*
	 * Some commands allow length modifiers (like "cp.b");
	 * compare command name only until first dot.
	 */
	len = ((p = strchr(cmd, '.')) == NULL) ? strlen (cmd) : (p - cmd);

	for (i = 0, cmdtp = command_handlers[i];
	     cmdtp != NULL;
	     i++, cmdtp = command_handlers[i]) {
		if (strncmp (cmd, cmdtp->name, len) == 0) {
			if (len == strlen (cmdtp->name))
				return (cmdtp);	/* full match */

			cmdtp_temp = cmdtp;	/* abbreviated command ? */
			n_found++;
		}
	}
	if (n_found == 1) {			/* exactly one match */
		return (cmdtp_temp);
	}

	return (NULL);	/* not found or ambiguous command */
}

