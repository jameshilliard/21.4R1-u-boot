/*
 * Copyright (c) 2009-2012, Juniper Networks, Inc.
 * All rights reserved.
 *
 * (C) Copyright 2000-2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2001 Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Andreas Heppel <aheppel@sysgo.de>

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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/**************************************************************************
 *
 * Support for persistent environment data
 *
 * The "environment" is stored as a list of '\0' terminated
 * "name=value" strings. The end of the list is marked by a double
 * '\0'. New entries are always added at the end. Deleting an entry
 * shifts the remaining entries to the front. Replacing an entry is a
 * combination of deleting the old value and adding the new one.
 *
 * The environment is preceeded by a 32 bit CRC over the data part.
 *
 **************************************************************************
 */

#include <common.h>
#include <command.h>
#include <environment.h>
#include <watchdog.h>
#include <serial.h>
#include <linux/stddef.h>
#include <asm/byteorder.h>
#include <vsprintf.h>
#if (CONFIG_COMMANDS & CFG_CMD_NET)
#include <net.h>
#endif
#ifdef CONFIG_JSRXNLE
#include <configs/jsrxnle.h>
#endif
#ifdef CONFIG_MAG8600
#include <configs/mag8600.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

#if !defined(CFG_ENV_IS_IN_NVRAM)	&& \
    !defined(CFG_ENV_IS_IN_EEPROM)	&& \
    !defined(CFG_ENV_IS_IN_FLASH)	&& \
    !defined(CFG_ENV_IS_IN_DATAFLASH)	&& \
    !defined(CFG_ENV_IS_IN_NAND)	&& \
    !defined(CFG_ENV_IS_NOWHERE)
# error Define one of CFG_ENV_IS_IN_{NVRAM|EEPROM|FLASH|DATAFLASH|NOWHERE}
#endif

#define XMK_STR(x)	#x
#define MK_STR(x)	XMK_STR(x)

/************************************************************************
************************************************************************/

/* Function that returns a character from the environment */
extern uchar (*env_get_char)(int);

/* Function that returns a pointer to a value from the environment */
/* (Only memory version supported / needed). */
extern uchar *env_get_addr(int);

/* Function that updates CRC of the enironment */
extern void env_crc_update (void);

/************************************************************************
************************************************************************/

/* removed static for api.c */
int envmatch (uchar *, int);

/*
 * Table with supported baudrates (defined in config_xyz.h)
 */
static const unsigned long baudrate_table[] = CFG_BAUDRATE_TABLE;
#define	N_BAUDRATES (sizeof(baudrate_table) / sizeof(baudrate_table[0]))


#ifdef CONFIG_JSRXNLE
/* 
 * List of environment variables which are
 * modifiable by the U-Boot only and should 
 * not be modified by user.
 */
static const char *srxsme_ro_env_vars[] = {
	"nand_error",
	NULL
};
#endif


/************************************************************************
 * Command interface: print one or all environment variables
 */

int do_printenv (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i, j, k, nxt;
	int rcode = 0;

	if (argc == 1) {		/* Print all env variables	*/
		for (i=0; env_get_char(i) != '\0'; i=nxt+1) {
			for (nxt=i; env_get_char(nxt) != '\0'; ++nxt)
				;
			for (k=i; k<nxt; ++k)
				putc(env_get_char(k));
			putc  ('\n');

			if (ctrlc()) {
				puts ("\n ** Abort\n");
				return 1;
			}
		}

		printf("\nEnvironment size: %d/%d bytes\n", i, ENV_SIZE);

		return 0;
	}

	for (i=1; i<argc; ++i) {	/* print single env variables	*/
		char *name = argv[i];

		k = -1;

		for (j=0; env_get_char(j) != '\0'; j=nxt+1) {

			for (nxt=j; env_get_char(nxt) != '\0'; ++nxt)
				;
			k = envmatch((uchar *)name, j);
			if (k < 0) {
				continue;
			}
			puts (name);
			putc ('=');
			while (k < nxt)
				putc(env_get_char(k++));
			putc ('\n');
			break;
		}
		if (k < 0) {
			printf ("## Error: \"%s\" not defined\n", name);
			rcode ++;
		}
	}
	return rcode;
}

/************************************************************************
 * Set a new environment variable,
 * or replace or delete an existing one.
 *
 * This function will ONLY work with a in-RAM copy of the environment
 */

int _do_setenv (int flag, int argc, char *argv[])
{
	int   i, len, oldval;
	int   console = -1;
	uchar *env, *nxt = NULL;
	char *name;
	const char *env_delay_name = "bootdelay";
#ifdef CONFIG_JSRXNLE
	char devlist[256];
#endif
	bd_t *bd = gd->bd;

	uchar *env_data = env_get_addr(0);

	if (!env_data)	/* need copy in RAM */
		return 1;

	name = argv[1];

	if (strchr(name, '=')) {
		printf ("## Error: illegal character '=' in variable name \"%s\"\n", name);
		return 1;
	}

	/*
	 * search if variable with this name already exists
	 */
	oldval = -1;
	for (env=env_data; *env; env=nxt+1) {
		for (nxt=env; *nxt; ++nxt)
			;
		if ((oldval = envmatch((uchar *)name, env-env_data)) >= 0)
			break;
	}

#ifdef CONFIG_JSRXNLE
	if (!strncmp(name, SRXSME_BOOTDEVLIST_ENV, 
                 strlen(SRXSME_BOOTDEVLIST_ENV))) {
	    if (!srxsme_is_valid_devlist(argv[2])) {
            get_bootmedia_list(devlist);
            printf("Error: Bootlist \"%s\" has one or more invalid media names.\n"
                   "Supported media are %s\n", argv[2], devlist);
            return 1;
	    }

	    /* 
	     * If boot.devlist is changed, reset the boot sequencing
	     * to start afresh.
	     */
	    srxsme_reset_bootseq(argv[2]);
	}
#endif

        /* 
         * Don't allow bootdelay be set to zero. This is checked
         * before old value is deleted.
         */
        if (!strncmp(name, env_delay_name, strlen(env_delay_name)) 
            && (argc >= 3)) {
           
            int32_t delay = simple_strtoul(argv[2], NULL, 10);

            if (delay <= 0) {
                printf("WARNING: Boot delay cannot be set to zero.\n");
                return 1;
            }
        }
        
	/*
	 * Delete any existing definition
	 */
	if (oldval >= 0) {
#ifndef CONFIG_ENV_OVERWRITE

		/*
		 * Ethernet Address and serial# can be set only once,
		 * ver is readonly.
		 */
		if ( (strcmp (name, "serial#") == 0) ||
		    ((strcmp (name, "ethaddr") == 0)
#if defined(CONFIG_OVERWRITE_ETHADDR_ONCE) && defined(CONFIG_ETHADDR)
		     && (strcmp ((char *)env_get_addr(oldval),MK_STR(CONFIG_ETHADDR)) != 0)
#endif	/* CONFIG_OVERWRITE_ETHADDR_ONCE && CONFIG_ETHADDR */
		    ) ) {
			printf ("Can't overwrite \"%s\"\n", name);
			return 1;
		}
#endif

		/* Check for console redirection */
		if (strcmp(name,"stdin") == 0) {
			console = stdin;
		} else if (strcmp(name,"stdout") == 0) {
			console = stdout;
		} else if (strcmp(name,"stderr") == 0) {
			console = stderr;
		}

		if (console != -1) {
			if (argc < 3) {		/* Cannot delete it! */
				printf("Can't delete \"%s\"\n", name);
				return 1;
			}

			/* Try assigning specified device */
			if (console_assign (console, argv[2]) < 0)
				return 1;

#ifdef CONFIG_SERIAL_MULTI
			if (serial_assign (argv[2]) < 0)
				return 1;
#endif
		}

		/*
		 * Switch to new baudrate if new baudrate is supported
		 */
		if (strcmp(argv[1],"baudrate") == 0) {
			int baudrate = simple_strtoul(argv[2], NULL, 10);
			int i;
			for (i=0; i<N_BAUDRATES; ++i) {
				if (baudrate == baudrate_table[i])
					break;
			}
			if (i == N_BAUDRATES) {
				printf ("## Baudrate %d bps not supported\n",
					baudrate);
				return 1;
			}
			printf ("## Switch baudrate to %d bps and press ENTER ...\n",
				baudrate);
			udelay(50000);
			gd->baudrate = baudrate;
#ifdef CONFIG_PPC
			gd->bd->bi_baudrate = baudrate;
#endif

			serial_setbrg ();
			udelay(50000);
			for (;;) {
				if (getc() == '\r')
				      break;
			}
		}

		if (*++nxt == '\0') {
			if (env > env_data) {
				env--;
			} else {
				*env = '\0';
			}
		} else {
			for (;;) {
				*env = *nxt++;
				if ((*env == '\0') && (*nxt == '\0'))
					break;
				++env;
			}
		}
		*++env = '\0';
	}

#ifdef CONFIG_NET_MULTI
	if (strncmp(name, "eth", 3) == 0) {
		char *end;
		int   num = simple_strtoul(name+3, &end, 10);

		if (strcmp(end, "addr") == 0) {
			eth_set_enetaddr(num, argv[2]);
		}
	}
#endif


	/* Delete only ? */
	if ((argc < 3) || argv[2] == NULL) {
		env_crc_update ();
		return 0;
	}

	/*
	 * Append new definition at the end
	 */
	for (env=env_data; *env || *(env+1); ++env)
		;
	if (env > env_data)
		++env;
	/*
	 * Overflow when:
	 * "name" + "=" + "val" +"\0\0"  > ENV_SIZE - (env-env_data)
	 */
	len = strlen(name) + 2;
	/* add '=' for first arg, ' ' for all others */
	for (i=2; i<argc; ++i) {
		len += strlen(argv[i]) + 1;
	}
	if (len > (&env_data[ENV_SIZE]-env)) {
		printf ("## Error: environment overflow, \"%s\" deleted\n", name);
		return 1;
	}
	while ((*env = *name++) != '\0')
		env++;
	for (i=2; i<argc; ++i) {
		char *val = argv[i];

		*env = (i==2) ? '=' : ' ';
		while ((*++env = *val++) != '\0')
			;
	}

	/* end is marked with double '\0' */
	*++env = '\0';

	/* Update CRC */
	env_crc_update ();

	/*
	 * Some variables should be updated when the corresponding
	 * entry in the enviornment is changed
	 */
#if !defined(CONFIG_EX45XX)
	if (strcmp(argv[1],"ethaddr") == 0) {
		char *s = argv[2];	/* always use only one arg */
		char *e;
		for (i=0; i<6; ++i) {
			bd->bi_enetaddr[i] = s ? simple_strtoul(s, &e, 16) : 0;
			if (s) s = (*e) ? e+1 : e;
		}
#ifdef CONFIG_NET_MULTI
		eth_set_enetaddr(0, argv[2]);
#endif
		return 0;
	}
#endif

#ifdef CONFIG_JSRXNLE
	if (strcmp(argv[1],"ethact") == 0) {
		/* Enable the active port and disable other ports */
		switch (gd->board_desc.board_type) {
		CASE_ALL_SRX240_MODELS
		CASE_ALL_SRX550_MODELS
		CASE_ALL_SRX650_MODELS
			do_link_update();
			break;
		default:
			break;
		}
	}
#endif
	if (strcmp(argv[1],"ipaddr") == 0) {
		char *s = argv[2];	/* always use only one arg */
		char *e;
		unsigned long addr;
		bd->bi_ip_addr = 0;
		for (addr=0, i=0; i<4; ++i) {
			ulong val = s ? simple_strtoul(s, &e, 10) : 0;
			addr <<= 8;
			addr  |= (val & 0xFF);
			if (s) s = (*e) ? e+1 : e;
		}
		bd->bi_ip_addr = htonl(addr);
		return 0;
	}
	if (strcmp(argv[1],"loadaddr") == 0) {
		load_addr = simple_strtoul(argv[2], NULL, 16);
		return 0;
	}
#if (CONFIG_COMMANDS & CFG_CMD_NET)
	if (strcmp(argv[1],"bootfile") == 0) {
		copy_filename (BootFile, argv[2], sizeof(BootFile));
		return 0;
	}
#endif	/* CFG_CMD_NET */

#ifdef CONFIG_AMIGAONEG3SE
	if (strcmp(argv[1], "vga_fg_color") == 0 ||
	    strcmp(argv[1], "vga_bg_color") == 0 ) {
		extern void video_set_color(unsigned char attr);
		extern unsigned char video_get_attr(void);

		video_set_color(video_get_attr());
		return 0;
	}
#endif	/* CONFIG_AMIGAONEG3SE */

	return 0;
}

#ifdef CONFIG_ACX
/**
 * Set an environment variable to an integer value
 *
 * @param varname	Environment variable to set
 * @param value		Value to set it to
 * @return 0 if ok, 1 on error
 */
int setenv_ulong(const char *varname, ulong value)
{
	/* TODO: this should be unsigned */
	char *str = simple_itoa(value);

	setenv((char *)varname, str);
	return 0;
}
#endif

void setenv (char *varname, char *varvalue)
{
	char *argv[4] = { "setenv", varname, varvalue, NULL };

#ifdef CONFIG_FX
        if (strncmp(varname, "boot.status", 11) == 0) {
            eeprom_do_setenv(varvalue);
            return;
        }
#endif
	_do_setenv (0, 3, argv);
}

#ifdef CONFIG_JSRXNLE
/**
 * @brief:
 * Check if the uboot environment variables is a
 * read-only variable and should not be allowed
 * to be set by the user.
 *
 * @arg:
 * arg: pointer to variable's name string that 
 * needs to be updated by user.
 *
 * @ro_env_list:
 * ro_env_list: Null terminated list of environment variables
 * which are read-only.
 */
int is_srxsme_ro_env_vars (char *arg, const uchar *ro_env_list[])
{
	int idx = 0;

	while (ro_env_list[idx]) {
		if (!strcmp(arg, ro_env_list[idx]))
			return 1;
		idx++;
	}
	return 0;
}
#endif

int do_setenv ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
#ifdef CONFIG_JSRXNLE
        if (is_srxsme_ro_env_vars(argv[1], srxsme_ro_env_vars)) {
		printf("Read only variable, can not be modified\n");
		return;
        }
#endif

	return _do_setenv (flag, argc, argv);
}

/************************************************************************
 * Prompt for environment variable
 */

#if (CONFIG_COMMANDS & CFG_CMD_ASKENV)
int do_askenv ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	extern char console_buffer[CFG_CBSIZE];
	char message[CFG_CBSIZE];
	int size = CFG_CBSIZE - 1;
	int len;
	char *local_args[4];

	local_args[0] = argv[0];
	local_args[1] = argv[1];
	local_args[2] = NULL;
	local_args[3] = NULL;

	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
	/* Check the syntax */
	switch (argc) {
	case 1:
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;

	case 2:		/* askenv envname */
		sprintf (message, "Please enter '%s':", argv[1]);
		break;

	case 3:		/* askenv envname size */
		sprintf (message, "Please enter '%s':", argv[1]);
		size = simple_strtoul (argv[2], NULL, 10);
		break;

	default:	/* askenv envname message1 ... messagen size */
	    {
		int i;
		int pos = 0;

		for (i = 2; i < argc - 1; i++) {
			if (pos) {
				message[pos++] = ' ';
			}
			strcpy (message+pos, argv[i]);
			pos += strlen(argv[i]);
		}
		message[pos] = '\0';
		size = simple_strtoul (argv[argc - 1], NULL, 10);
	    }
		break;
	}

	if (size >= CFG_CBSIZE)
		size = CFG_CBSIZE - 1;

	if (size <= 0)
		return 1;

	/* prompt for input */
	len = readline (message);

	if (size < len)
		console_buffer[size] = '\0';

	len = 2;
	if (console_buffer[0] != '\0') {
		local_args[2] = console_buffer;
		len = 3;
	}

	/* Continue calling setenv code */
	return _do_setenv (flag, len, local_args);
}
#endif	/* CFG_CMD_ASKENV */

/************************************************************************
 * Look up variable from environment,
 * return address of storage for that variable,
 * or NULL if not found
 */

char *getenv (char *name)
{
	int i, nxt;

#ifdef CONFIG_FX
        /* For the FX Platforms, the booting status is stored in the EEPROM 
         * as we need access to this in the early stages of booting before 
         * the flash can be accessed
         */
        if (strncmp(name, "boot.status", 11) == 0) {
            return((char *)(eeprom_do_getenv()));
        }    
#endif
	WATCHDOG_RESET();

	for (i=0; env_get_char(i) != '\0'; i=nxt+1) {
		int val;

		for (nxt=i; env_get_char(nxt) != '\0'; ++nxt) {
			if (nxt >= CFG_ENV_SIZE) {
				return (NULL);
			}
		}
		if ((val=envmatch((uchar *)name, i)) < 0)
			continue;
		return ((char *)env_get_addr(val));
	}

	return (NULL);
}

int getenv_r (char *name, char *buf, unsigned len)
{
	int i, nxt;

	for (i=0; env_get_char(i) != '\0'; i=nxt+1) {
		int val, n;

		for (nxt=i; env_get_char(nxt) != '\0'; ++nxt) {
			if (nxt >= CFG_ENV_SIZE) {
				return (-1);
			}
		}
		if ((val=envmatch((uchar *)name, i)) < 0)
			continue;
		/* found; copy out */
		n = 0;
		while ((len > n++) && (*buf++ = env_get_char(val++)) != '\0')
			;
		if (len == n)
			*buf = '\0';
		return (n);
	}
	return (-1);
}

#if defined(CFG_ENV_IS_IN_NVRAM) || defined(CFG_ENV_IS_IN_EEPROM) || \
    ((CONFIG_COMMANDS & (CFG_CMD_ENV|CFG_CMD_FLASH)) == \
      (CFG_CMD_ENV|CFG_CMD_FLASH)) || \
    ((CONFIG_COMMANDS & (CFG_CMD_ENV|CFG_CMD_NAND)) == \
      (CFG_CMD_ENV|CFG_CMD_NAND))
int do_saveenv (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	extern char * env_name_spec;

	printf ("Saving Environment to %s...\n", env_name_spec);

	return (saveenv() ? 1 : 0);
}


#endif


/************************************************************************
 * Match a name / name=value pair
 *
 * s1 is either a simple 'name', or a 'name=value' pair.
 * i2 is the environment index for a 'name2=value2' pair.
 * If the names match, return the index for the value2, else NULL.
 */

int /* JUNOS, removed static for API use */
envmatch (uchar *s1, int i2)
{

	while (*s1 == env_get_char(i2++))
		if (*s1++ == '=')
			return(i2);
	if (*s1 == '\0' && env_get_char(i2-1) == '=')
		return(i2);
	return(-1);
}


/**
 * Set an environment variable to an value in hex
 *
 * @param varname       Environment variable to set
 * @param value         Value to set it to
 * @return 0 if ok, 1 on error
 */
int setenv_hex(const char *varname, ulong value)
{
        char str[17];

        sprintf(str, "%lx", value);
        setenv((char *) varname, str);

	return 0;
}

ulong getenv_hex(const char *varname, ulong default_val)
{
        const char *s;
        ulong value;
        char *endp;

        s = getenv((char *) varname);
        if (s)
                value = simple_strtoul(s, &endp, 16);
        if (!s || endp == s)
                return default_val;

        return value;
}


/**************************************************/

U_BOOT_CMD(
	printenv, CFG_MAXARGS, 1,	do_printenv,
	"printenv- print environment variables\n",
	"\n    - print values of all environment variables\n"
	"printenv name ...\n"
	"    - print value of environment variable 'name'\n"
);

U_BOOT_CMD(
	setenv, CFG_MAXARGS, 0,	do_setenv,
	"setenv  - set environment variables\n",
	"name value ...\n"
	"    - set environment variable 'name' to 'value ...'\n"
	"setenv name\n"
	"    - delete environment variable 'name'\n"
);

#if defined(CFG_ENV_IS_IN_NVRAM) || defined(CFG_ENV_IS_IN_EEPROM) || \
    ((CONFIG_COMMANDS & (CFG_CMD_ENV|CFG_CMD_FLASH)) == \
      (CFG_CMD_ENV|CFG_CMD_FLASH)) || \
    ((CONFIG_COMMANDS & (CFG_CMD_ENV|CFG_CMD_NAND)) == \
      (CFG_CMD_ENV|CFG_CMD_NAND))
U_BOOT_CMD(
	saveenv, 1, 0,	do_saveenv,
	"saveenv - save environment variables to persistent storage\n",
	NULL
);

#endif	/* CFG_CMD_ENV */

#if (CONFIG_COMMANDS & CFG_CMD_ASKENV)

U_BOOT_CMD(
	askenv,	CFG_MAXARGS,	1,	do_askenv,
	"askenv  - get environment variables from stdin\n",
	"name [message] [size]\n"
	"    - get environment variable 'name' from stdin (max 'size' chars)\n"
	"askenv name\n"
	"    - get environment variable 'name' from stdin\n"
	"askenv name size\n"
	"    - get environment variable 'name' from stdin (max 'size' chars)\n"
	"askenv name [message] size\n"
	"    - display 'message' string and get environment variable 'name'"
	"from stdin (max 'size' chars)\n"
);
#endif	/* CFG_CMD_ASKENV */

#if (CONFIG_COMMANDS & CFG_CMD_RUN)
int do_run (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
U_BOOT_CMD(
	run,	CFG_MAXARGS,	1,	do_run,
	"run     - run commands in an environment variable\n",
	"var [...]\n"
	"    - run the commands in the environment variable(s) 'var'\n"
);
#endif  /* CFG_CMD_RUN */
