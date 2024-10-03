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

/* #define DEBUG */

#include <common.h>

#if defined(CFG_ENV_IS_IN_FLASH) /* Environment is in Flash */

#include <command.h>
#include <environment.h>
#include <linux/stddef.h>
#include <malloc.h>

DECLARE_GLOBAL_DATA_PTR;

#if ((CONFIG_COMMANDS&(CFG_CMD_ENV|CFG_CMD_FLASH)) == (CFG_CMD_ENV|CFG_CMD_FLASH))
#define CMD_SAVEENV
#elif defined(CFG_ENV_ADDR_REDUND)
#error Cannot use CFG_ENV_ADDR_REDUND without CFG_CMD_ENV & CFG_CMD_FLASH
#endif

#if defined(CFG_ENV_SIZE_REDUND) && (CFG_ENV_SIZE_REDUND < CFG_ENV_SIZE)
#error CFG_ENV_SIZE_REDUND should not be less then CFG_ENV_SIZE
#endif

#ifdef CONFIG_INFERNO
# ifdef CFG_ENV_ADDR_REDUND
#error CFG_ENV_ADDR_REDUND is not implemented for CONFIG_INFERNO
# endif
#endif

char * env_name_spec = "Flash";

#ifdef ENV_IS_EMBEDDED

extern uchar environment[];
env_t *env_ptr = (env_t *)(&environment[0]);

#ifdef CMD_SAVEENV
/* static env_t *flash_addr = (env_t *)(&environment[0]);-broken on ARM-wd-*/
static env_t *flash_addr = (env_t *)CFG_ENV_ADDR;
#endif

#else /* ! ENV_IS_EMBEDDED */

env_t *env_ptr = (env_t *)CFG_ENV_ADDR;
#ifdef CMD_SAVEENV
static env_t *flash_addr = (env_t *)CFG_ENV_ADDR;
#endif

#endif /* ENV_IS_EMBEDDED */

#ifdef CFG_ENV_ADDR_REDUND
static env_t *flash_addr_new = (env_t *)CFG_ENV_ADDR_REDUND;

/* CFG_ENV_ADDR is supposed to be on sector boundary */
static ulong end_addr = CFG_ENV_ADDR + CFG_ENV_SECT_SIZE - 1;
static ulong end_addr_new = CFG_ENV_ADDR_REDUND + CFG_ENV_SECT_SIZE - 1;

#define ACTIVE_FLAG   1
#define OBSOLETE_FLAG 0
#endif /* CFG_ENV_ADDR_REDUND */

/*
 * Some environment variables like run_once
 * can be 150 or more. 1024 seems same limit
 * for per environment length.
 */
#define MAX_PER_ENV_LEN		1024

extern uchar default_environment[];
extern int default_environment_size;


uchar env_get_char_spec (int index)
{
	return ( *((uchar *)(gd->env_addr + index)) );
}

#ifdef CFG_ENV_ADDR_REDUND

int  env_init(void)
{
	int crc1_ok = 0, crc2_ok = 0;

	uchar flag1 = flash_addr->flags;
	uchar flag2 = flash_addr_new->flags;

	ulong addr_default = (ulong)&default_environment[0];
	ulong addr1 = (ulong)&(flash_addr->data);
	ulong addr2 = (ulong)&(flash_addr_new->data);

#ifdef CONFIG_OMAP2420H4
	int flash_probe(void);

	if(flash_probe() == 0)
		goto bad_flash;
#endif

	crc1_ok = (crc32(0, flash_addr->data, ENV_SIZE) == flash_addr->crc);
	crc2_ok = (crc32(0, flash_addr_new->data, ENV_SIZE) == flash_addr_new->crc);

	if (crc1_ok && ! crc2_ok) {
		gd->env_addr  = addr1;
		gd->env_valid = 1;
	} else if (! crc1_ok && crc2_ok) {
		gd->env_addr  = addr2;
		gd->env_valid = 1;
	} else if (! crc1_ok && ! crc2_ok) {
		gd->env_addr  = addr_default;
		gd->env_valid = 0;
	} else if (flag1 == ACTIVE_FLAG && flag2 == OBSOLETE_FLAG) {
		gd->env_addr  = addr1;
		gd->env_valid = 1;
	} else if (flag1 == OBSOLETE_FLAG && flag2 == ACTIVE_FLAG) {
		gd->env_addr  = addr2;
		gd->env_valid = 1;
	} else if (flag1 == flag2) {
		gd->env_addr  = addr1;
		gd->env_valid = 2;
	} else if (flag1 == 0xFF) {
		gd->env_addr  = addr1;
		gd->env_valid = 2;
	} else if (flag2 == 0xFF) {
		gd->env_addr  = addr2;
		gd->env_valid = 2;
	}

#ifdef CONFIG_OMAP2420H4
bad_flash:
#endif
	return (0);
}

#ifdef CMD_SAVEENV
int saveenv(void)
{
	char *saved_data = NULL;
	int rc = 1;
	char flag = OBSOLETE_FLAG, new_flag = ACTIVE_FLAG;
#if CFG_ENV_SECT_SIZE > CFG_ENV_SIZE
	ulong up_data = 0;
#endif

	debug ("Protect off %08lX ... %08lX\n",
		(ulong)flash_addr, end_addr);

	if (flash_sect_protect (0, (ulong)flash_addr, end_addr)) {
		goto Done;
	}

	debug ("Protect off %08lX ... %08lX\n",
		(ulong)flash_addr_new, end_addr_new);

	if (flash_sect_protect (0, (ulong)flash_addr_new, end_addr_new)) {
		goto Done;
	}

#if CFG_ENV_SECT_SIZE > CFG_ENV_SIZE
	up_data = (end_addr_new + 1 - ((long)flash_addr_new + CFG_ENV_SIZE));
	debug ("Data to save 0x%x\n", up_data);
	if (up_data) {
		if ((saved_data = malloc(up_data)) == NULL) {
			printf("Unable to save the rest of sector (%ld)\n",
				up_data);
			goto Done;
		}
		memcpy(saved_data,
			(void *)((long)flash_addr_new + CFG_ENV_SIZE), up_data);
		debug ("Data (start 0x%x, len 0x%x) saved at 0x%x\n",
			   (long)flash_addr_new + CFG_ENV_SIZE,
				up_data, saved_data);
	}
#endif
	puts ("Erasing Flash...");
	debug (" %08lX ... %08lX ...",
		(ulong)flash_addr_new, end_addr_new);

	if (flash_sect_erase ((ulong)flash_addr_new, end_addr_new)) {
		goto Done;
	}

	puts ("Writing to Flash... ");
	debug (" %08lX ... %08lX ...",
		(ulong)&(flash_addr_new->data),
		sizeof(env_ptr->data)+(ulong)&(flash_addr_new->data));
	if ((rc = flash_write((char *)env_ptr->data,
			(ulong)&(flash_addr_new->data),
			sizeof(env_ptr->data))) ||
	    (rc = flash_write((char *)&(env_ptr->crc),
			(ulong)&(flash_addr_new->crc),
			sizeof(env_ptr->crc))) ||
	    (rc = flash_write(&flag,
			(ulong)&(flash_addr->flags),
			sizeof(flash_addr->flags))) ||
	    (rc = flash_write(&new_flag,
			(ulong)&(flash_addr_new->flags),
			sizeof(flash_addr_new->flags))))
	{
		flash_perror (rc);
		goto Done;
	}
	puts ("done\n");

#if CFG_ENV_SECT_SIZE > CFG_ENV_SIZE
	if (up_data) { /* restore the rest of sector */
		debug ("Restoring the rest of data to 0x%x len 0x%x\n",
			   (long)flash_addr_new + CFG_ENV_SIZE, up_data);
		if (flash_write(saved_data,
				(long)flash_addr_new + CFG_ENV_SIZE,
				up_data)) {
			flash_perror(rc);
			goto Done;
		}
	}
#endif
	{
		env_t * etmp = flash_addr;
		ulong ltmp = end_addr;

		flash_addr = flash_addr_new;
		flash_addr_new = etmp;

		end_addr = end_addr_new;
		end_addr_new = ltmp;
	}

	rc = 0;
Done:

	if (saved_data)
		free (saved_data);
	/* try to re-protect */
	(void) flash_sect_protect (1, (ulong)flash_addr, end_addr);
	(void) flash_sect_protect (1, (ulong)flash_addr_new, end_addr_new);

	return rc;
}
#endif /* CMD_SAVEENV */

#else /* ! CFG_ENV_ADDR_REDUND */

int  env_init(void)
{
#ifdef CONFIG_JSRXNLE
	env_t *tmp_env_ptr;
#endif

#ifdef CONFIG_OMAP2420H4
	int flash_probe(void);

	if(flash_probe() == 0)
		goto bad_flash;
#endif

#ifdef RMI_EVAL_BOARD
    /* env_relocate_spec will take care of reading from flash */
    gd->env_addr  = (ulong)&default_environment[0];
    gd->env_valid = 1;
    return 0;
#endif
#ifdef CONFIG_FX
        /* The Flash Controller is not re-based to the final CFG_BASE_ADDR.
	 * So, forcing the env_ptr to point the MIPS based address (0xBFC00000 + ENV_OFFSET).
	 */  
env_ptr = 0xbfff0000;
#endif

#ifdef CONFIG_JSRXNLE
        switch (gd->board_desc.board_type) {
        CASE_ALL_SRXSME_4MB_FLASH_MODELS
            tmp_env_ptr = (ulong)CFG_ENV_ADDR;
            break;
        CASE_ALL_SRXSME_8MB_FLASH_MODELS
            tmp_env_ptr = (ulong)CFG_8MB_FLASH_ENV_ADDR;
            break;
        }
        if (crc32(0, tmp_env_ptr->data, ENV_SIZE) == tmp_env_ptr->crc) {
                gd->env_addr  = (ulong)&(tmp_env_ptr->data);
                gd->env_valid = 1;
                return(0);
        }
#else
	if (crc32(0, env_ptr->data, ENV_SIZE) == env_ptr->crc) {
		gd->env_addr  = (ulong)&(env_ptr->data);
		gd->env_valid = 1;
		return(0);
	}
#endif 

#ifdef CONFIG_OMAP2420H4
bad_flash:
#endif
	gd->env_addr  = (ulong)&default_environment[0];
	gd->env_valid = 0;
	return (0);
}

#ifdef CMD_SAVEENV

#ifdef CONFIG_JSRXNLE

/* 
 * List of environment variables which are
 * transient and should not be saved to
 * boot flash.
 */
static const char *srxsme_volatile_env_vars[] =  {
	"boot.status",
	"oct.net.init",
	"boot.ver",
	"post.",
	"boot_status",
	"numcores",
	"ethact",
	"boot.install_target_dev",
	NULL
};

#endif

/**
 * @brief:
 * Filter out the uboot environment variables which are volatile
 * and should not go to boot flash.
 *
 * @params:
 * env_buf: Pointer to the environment buffer of type env_t where
 * the first four bytes of the buffer contain CRC.
 *
 * @params:
 * vol_env_list: Null terminated list of environment variables
 * which are volatile.
 */
void filter_out_volatile_env_variables (env_t *env_buf, const uchar *vol_env_list[])
{
	unsigned char lbuf[MAX_PER_ENV_LEN];

	/*
	 * Temporary buffer of size total environment
	 * buffer minus size of crc i.e. 4 Bytes.
	 */
	unsigned char *tbuf;
	int counter, ro_cntr;
	unsigned char *tbufp, *ebuf, match;
	unsigned int ebuf_pos = 0;
	unsigned char etrunc = 0;

	/* latch env start pointer */
	ebuf = env_buf->data;

	/* Check if env buffer has some env variables. */
	if (*ebuf == 0) {
		return;
	}

	/* allocate temporary buffer of env data size */
	tbuf = malloc(ENV_SIZE);
	if (!tbuf) {
		return;
	}

	/* clear out temp buffer to copy filterd envs */
	memset(tbuf, 0, ENV_SIZE);
	tbufp = tbuf;

	/*
	 * Iterate through environment buffer to parse
	 * environment variables line by line. We will
	 * loop for the complete buffer. Inside this
	 * loop if we find that we have reached end of
	 * environment variables, we will break this.
	 */
	for ( ; ebuf_pos < ENV_SIZE; ebuf++, ebuf_pos++) {
		/* take out a env line from buffer */
		for (counter = 0; *ebuf != 0; counter++, ebuf++) {
			if (etrunc) {
			    /* 
			     * etrunc is set so we have reached
			     * end of lbuf so we should drop all 
			     * characters now.
			     */
			    continue;
			}

			/*
			 * If we have exhausted lbuf set truncate flag now,
			 * to avoid writing futher and causing overruns.
			 */
			if (counter >= MAX_PER_ENV_LEN) {
			    /* 
			     * Truncate the futher env value and
			     * drop all characters until we find null.
			     */
			    etrunc = 1;
			    printf("Error: environment variable too long. Truncating.\n");
			    continue;
			}

			lbuf[counter] = *ebuf;
		}

		/* null terminate the env line */
		if (etrunc) {
		    lbuf[MAX_PER_ENV_LEN] = 0;
		    counter = MAX_PER_ENV_LEN;
		} else {
		    lbuf[counter] = 0;
		}

		match = 0;
		etrunc = 0;

		/* find if env in lbuf is one of the volatile vars */
		for (ro_cntr = 0; vol_env_list[ro_cntr] != NULL; ro_cntr++) {
			if (strstr((const char*)lbuf, (const char*)vol_env_list[ro_cntr])) {
				match = 1;
				break;
			}
		}

		/* 
		 * If no match was found in ro list, this variables is
		 * non-volatile, so copy to tmp buffer 
		 */
		if (!match) {
			strncpy((char*)tbufp, (const char*)lbuf, counter);
			tbufp += (counter + 1);
		}

		/* 
		 * If reached end of environments, break the loop
		 * since outermost loop, loops around complete buffer.
		 */
		if (*(ebuf + 1) == 0){
			*tbufp = 0; /* terminate with second null */
			break;
		}
	}

	/* 
	 * Copy temp buffer back to the given buffer
	 * and recalculate the CRC of filtered list.
	 */
	memcpy(env_buf->data, tbuf, ENV_SIZE);
	env_buf->crc = crc32(0, tbuf, ENV_SIZE);
	free(tbuf);
}

int saveenv(void)
{ 
	int	len, rc;
	ulong	end_addr;
	ulong	flash_sect_addr;

#if defined(CONFIG_JSRXNLE)
	ulong	flash_offset;
	uchar	buffer[64 *1024];
	uchar	*env_buffer;
	ulong	env_sect_size = 0;
	ulong	flash_addr;
#else	
#if defined(CFG_ENV_SECT_SIZE) && (CFG_ENV_SECT_SIZE > CFG_ENV_SIZE)
#if defined(CONFIG_EX2200) || \
    defined(CONFIG_EX3300)
	uchar   *env_buffer = NULL;
	ulong	flash_addr;
#else
	uchar	env_buffer[CFG_ENV_SECT_SIZE];
#endif
	ulong	flash_offset;
#else
	uchar *env_buffer = (uchar *)env_ptr;
#endif	/* CFG_ENV_SECT_SIZE */
#endif	
	int rcode = 0;

#if defined(CFG_ENV_SECT_SIZE) && (CFG_ENV_SECT_SIZE > CFG_ENV_SIZE)

#if defined(CONFIG_EX2200) || \
    defined(CONFIG_EX3300)
	if ((env_buffer = malloc(CFG_ENV_SECT_SIZE)) == NULL) {
		printf("Unable to save env\n");
		goto Done;
	}
	flash_addr = (ulong)CFG_ENV_ADDR;
#endif

	flash_offset    = ((ulong)flash_addr) & (CFG_ENV_SECT_SIZE-1);
	flash_sect_addr = ((ulong)flash_addr) & ~(CFG_ENV_SECT_SIZE-1);

	debug ( "copy old content: "
		"sect_addr: %08lX  env_addr: %08lX  offset: %08lX\n",
		flash_sect_addr, (ulong)flash_addr, flash_offset);

	/* copy old contents to temporary buffer */
	memcpy (env_buffer, (void *)flash_sect_addr, CFG_ENV_SECT_SIZE);

	/* copy current environment to temporary buffer */
	memcpy ((uchar *)((unsigned long)env_buffer + flash_offset),
		env_ptr,
		CFG_ENV_SIZE);

	len	 = CFG_ENV_SECT_SIZE;
#else
#ifdef CONFIG_JSRXNLE
       env_sect_size = find_env_sect_size();

       if (env_sect_size > CFG_ENV_SIZE) {
           switch (gd->board_desc.board_type) {
           CASE_ALL_SRXSME_4MB_FLASH_MODELS
               flash_addr = (ulong)CFG_ENV_ADDR;
               break;
           CASE_ALL_SRXSME_8MB_FLASH_MODELS
               flash_addr = (ulong)CFG_8MB_FLASH_ENV_ADDR;
               break;
           default:
               break; 
           }
           env_buffer      = (uchar *)buffer;
           flash_offset    = ((ulong)flash_addr) & (env_sect_size-1);
           flash_sect_addr = ((ulong)flash_addr) & ~(env_sect_size-1);

           debug ("copy old content: "
                  "sect_addr: %08lX  env_addr: %08lX  offset: %08lX\n",
                   flash_sect_addr, (ulong)flash_addr, flash_offset);

           /* copy old contents to temporary buffer */
           memcpy(env_buffer, (void *)flash_sect_addr, env_sect_size);

          /* copy current environment to temporary buffer */
           memcpy(env_buffer + flash_offset, env_ptr, CFG_ENV_SIZE);

           /* remove the volatile variables from the given buffer. */
           filter_out_volatile_env_variables((env_t *)(env_buffer + flash_offset),
                                             srxsme_volatile_env_vars);

           len	 = env_sect_size;

       } else {
           env_buffer = &buffer;

           /* 
            * We will work on copy of env buffer to filter out volatile variables
            * and then write them to boot flash.
            */
           memcpy(env_buffer, env_ptr, CFG_ENV_SIZE);

           /* remove the volatile variables from env_buffer before writing to flash */
           filter_out_volatile_env_variables((env_t *)env_buffer, srxsme_volatile_env_vars);

           switch (gd->board_desc.board_type) {
           CASE_ALL_SRXSME_4MB_FLASH_MODELS
                flash_sect_addr = (ulong)CFG_ENV_ADDR;
                break;
           CASE_ALL_SRXSME_8MB_FLASH_MODELS
                flash_sect_addr = (ulong)CFG_8MB_FLASH_ENV_ADDR;
                break;
           default:
           break;
       }
       len  = CFG_ENV_SIZE;
}
#else
	flash_sect_addr = (ulong)flash_addr;
	len	 = CFG_ENV_SIZE;
#endif

#endif	/* CFG_ENV_SECT_SIZE */

#ifndef CONFIG_INFERNO
	end_addr = flash_sect_addr + len - 1;
#else
	/* this is the last sector, and the size is hardcoded here */
	/* otherwise we will get stack problems on loading 128 KB environment */
	end_addr = flash_sect_addr + 0x20000 - 1;
#endif

	debug ("Protect off %08lX ... %08lX\n",
		(ulong)flash_sect_addr, end_addr);

	if (flash_sect_protect (0, flash_sect_addr, end_addr))
#if !defined(CONFIG_EX2200) && \
    !defined(CONFIG_EX3300)
		return 1;
#else
	{
		rcode = 1;
		goto Done;
	}
#endif

	puts ("Erasing Flash...");
	if (flash_sect_erase (flash_sect_addr, end_addr))
#if !defined(CONFIG_EX2200) && \
    !defined(CONFIG_EX3300)
		return 1;
#else
	{
		rcode = 1;
		goto Done;
	}
#endif

	puts ("Writing to Flash... ");
	rc = flash_write((char *)env_buffer, flash_sect_addr, len);
	if (rc != 0) {
		flash_perror (rc);
		rcode = 1;
	} else {
		puts ("done\n");
	}

#if !defined(CONFIG_EX2200) && \
    !defined(CONFIG_EX3300)
	/* try to re-protect */
	(void) flash_sect_protect (1, flash_sect_addr, end_addr);
	return rcode;
#else
Done:

	if (env_buffer)
		free (env_buffer);
	/* try to re-protect */
	(void) flash_sect_protect (1, flash_sect_addr, end_addr);

	return rc;
#endif
}

#endif /* CMD_SAVEENV */

#endif /* CFG_ENV_ADDR_REDUND */

void env_relocate_spec (void)
{
#if !defined(ENV_IS_EMBEDDED) || defined(CFG_ENV_ADDR_REDUND)
#ifdef RMI_EVAL_BOARD
    int flash_probe(env_t **env, env_t **flash_addr);
    if(flash_probe(&env_ptr, &flash_addr) == 0) {
        gd->env_valid = 1;
    }else {
	gd->env_valid = 0;
    }
#endif
#ifdef CFG_ENV_ADDR_REDUND
	if (gd->env_addr != (ulong)&(flash_addr->data)) {
		env_t * etmp = flash_addr;
		ulong ltmp = end_addr;

		flash_addr = flash_addr_new;
		flash_addr_new = etmp;

		end_addr = end_addr_new;
		end_addr_new = ltmp;
	}

	if (flash_addr_new->flags != OBSOLETE_FLAG &&
	    crc32(0, flash_addr_new->data, ENV_SIZE) ==
	    flash_addr_new->crc) {
		char flag = OBSOLETE_FLAG;

		gd->env_valid = 2;
		flash_sect_protect (0, (ulong)flash_addr_new, end_addr_new);
		flash_write(&flag,
			    (ulong)&(flash_addr_new->flags),
			    sizeof(flash_addr_new->flags));
		flash_sect_protect (1, (ulong)flash_addr_new, end_addr_new);
	}

	if (flash_addr->flags != ACTIVE_FLAG &&
	    (flash_addr->flags & ACTIVE_FLAG) == ACTIVE_FLAG) {
		char flag = ACTIVE_FLAG;

		gd->env_valid = 2;
		flash_sect_protect (0, (ulong)flash_addr, end_addr);
		flash_write(&flag,
			    (ulong)&(flash_addr->flags),
			    sizeof(flash_addr->flags));
		flash_sect_protect (1, (ulong)flash_addr, end_addr);
	}

	if (gd->env_valid == 2)
		puts ("*** Warning - some problems detected "
		      "reading environment; recovered successfully\n\n");
#endif /* CFG_ENV_ADDR_REDUND */
#if CONFIG_JSRXNLE
	switch (gd->board_desc.board_type) {
    CASE_ALL_SRXSME_4MB_FLASH_MODELS
        flash_addr = (env_t *)CFG_ENV_ADDR;
    break;

    CASE_ALL_SRXSME_8MB_FLASH_MODELS
        flash_addr = (env_t *)CFG_8MB_FLASH_ENV_ADDR;
    break;

	default:
		break;
	}
#endif
#ifndef RMI_EVAL_BOARD
        /* Flash could be NAND also: So skip this as flash anyway is read in
	 * flash_probe above */
	memcpy (env_ptr, (void*)flash_addr, CFG_ENV_SIZE);

#if CONFIG_JSRXNLE
	/* 
	 * If because of some old u-boot version, volatile variables were
	 * saved to bootflash, skip them.
	 */
	filter_out_volatile_env_variables(env_ptr, srxsme_volatile_env_vars);
#endif

#endif	
#endif /* ! ENV_IS_EMBEDDED || CFG_ENV_ADDR_REDUND */
}

#endif /* CFG_ENV_IS_IN_FLASH */
