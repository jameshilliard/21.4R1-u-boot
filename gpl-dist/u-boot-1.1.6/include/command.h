/*
 * (C) Copyright 2000
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
 *  Definitions for Command Processor
 */
#ifndef __COMMAND_H
#define __COMMAND_H

#ifndef NULL
#define NULL	0
#endif

#ifndef	__ASSEMBLY__
/*
 * Monitor Command Table
 */

struct cmd_tbl_s {
	char		*name;		/* Command Name			*/
	int		maxargs;	/* maximum number of arguments	*/
	int		repeatable;	/* autorepeat allowed?		*/
					/* Implementation function	*/
	int		(*cmd)(struct cmd_tbl_s *, int, int, char *[]);
	char		*usage;		/* Usage message	(short)	*/
#ifdef	CFG_LONGHELP
	char		*help;		/* Help  message	(long)	*/
#endif
#ifdef CONFIG_AUTO_COMPLETE
	/* do auto completion on the arguments */
	int		(*complete)(int argc, char *argv[], char last_char, int maxv, char *cmdv[]);
#endif
};

typedef struct cmd_tbl_s	cmd_tbl_t;

extern cmd_tbl_t  __u_boot_cmd_start;
extern cmd_tbl_t  __u_boot_cmd_end;


/* common/command.c */
cmd_tbl_t *find_cmd(const char *cmd);
cmd_tbl_t *find_cmd_tbl (const char *cmd, cmd_tbl_t *table, int table_len);
int make_argv(char *s, int argvsz, char *argv[]);

#ifdef CONFIG_AUTO_COMPLETE
extern void install_auto_complete(void);
extern int cmd_auto_complete(const char *const prompt, char *buf, int *np, int *colp);
#endif

/*
 * Monitor Command
 *
 * All commands use a common argument format:
 *
 * void function (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
 */

typedef	void 	command_t (cmd_tbl_t *, int, int, char *[]);

/*
 * Error codes that commands return to cmd_process(). We use the standard 0
 * and 1 for success and failure, but add one more case - failure with a
 * request to call cmd_usage(). But the cmd_process() function handles
 * CMD_RET_USAGE itself and after calling cmd_usage() it will return 1.
 * This is just a convenience for commands to avoid them having to call
 * cmd_usage() all over the place.
 */
enum command_ret_t {
    CMD_RET_USAGE = -1, /* Failure, please report 'usage' error */
    CMD_RET_SUCCESS,    /* 0 = Success */
    CMD_RET_FAILURE,	/* 1 = Failure */
};

#endif	/* __ASSEMBLY__ */

/*
 * Command Flags:
 */
#define CMD_FLAG_REPEAT		0x0001	/* repeat last command		*/
#define CMD_FLAG_BOOTD		0x0002	/* command is from bootd	*/

/*
 * Configurable monitor commands definitions have been moved
 * to include/cmd_confdefs.h
 */


#define Struct_Section  __attribute__ ((unused,section (".u_boot_cmd")))

#ifdef  CFG_LONGHELP

#define U_BOOT_CMD(name,maxargs,rep,cmd,usage,help) \
cmd_tbl_t __u_boot_cmd_##name Struct_Section = {#name, maxargs, rep, cmd, usage, help}

#define U_BOOT_CMD_MKENT(name,maxargs,rep,cmd,usage,help) \
{#name, maxargs, rep, cmd, usage, help}

#else	/* no long help info */

#define U_BOOT_CMD(name,maxargs,rep,cmd,usage,help) \
cmd_tbl_t __u_boot_cmd_##name Struct_Section = {#name, maxargs, rep, cmd, usage}

#define U_BOOT_CMD_MKENT(name,maxargs,rep,cmd,usage,help) \
{#name, maxargs, rep, cmd, usage}

#endif	/* CFG_LONGHELP */

#define U_BOOT_CMD_MKENT_COMPLETE(_name, _maxargs, _rep, _cmd,	    \
						_usage, _help, _comp)		\
	{ #_name, _maxargs, _rep, _cmd, _usage,		\
			_CMD_HELP(_help) _CMD_COMPLETE(_comp) }


#define U_BOOT_CMD_COMPLETE(_name, _maxargs, _rep, _cmd, _usage, _help, _comp) \
	ll_entry_declare(cmd_tbl_t, _name, cmd) =	    \
	U_BOOT_CMD_MKENT_COMPLETE(_name, _maxargs, _rep, _cmd,	\
							_usage, _help, _comp);

#endif	/* __COMMAND_H */
