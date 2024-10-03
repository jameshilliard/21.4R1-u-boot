/***********************************************************************
Copyright 2003-2006 Raza Microelectronics, Inc.(RMI). All rights
reserved.
Use of this software shall be governed in all respects by the terms and
conditions of the RMI software license agreement ("SLA") that was
accepted by the user as a condition to opening the attached files.
Without limiting the foregoing, use of this software in source and
binary code forms, with or without modification, and subject to all
other SLA terms and conditions, is permitted.
Any transfer or redistribution of the source code, with or without
modification, IS PROHIBITED,unless specifically allowed by the SLA.
Any transfer or redistribution of the binary code, with or without
modification, is permitted, provided that the following condition is
met:
Redistributions in binary form must reproduce the above copyright
notice, the SLA, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution:
THIS SOFTWARE IS PROVIDED BY Raza Microelectronics, Inc. `AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL RMI OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*****************************#RMI_3#***********************************/
#include "rmi/debug.h"
#include "rmi/types.h"
#include "asm/string.h"
#include "rmi/boardconfig.h"

extern void printf (const char *fmt, ...);

/* VT102 emulation */
#define DEL    0x7f
#define BEL    0x07
#define BS     0x08
#define ETX    0x03
#define CTRL_C ETX
#define NAK    0x15
#define CTRL_U NAK
#define ETB    0x17
#define CTRL_W ETB
#define SOH    1
#define CTRL_A SOH
#define VT     0x0b
#define CTRL_K VT
#define TAB    0x09
#define ESC    0x1B
#define SOT    0x02  /*Start of Text*/
#define CTRL_B SOT

char * strtok(char * s,const char * ct);
extern char inbyte_nowait(void);
extern char inbyte(void);
extern int putch(int);
extern void enable_dram_eye_finder (int argc, char **argv);
extern void prog_info(int argc, char *argv[]);
extern void mem_read(int argc, char **argv);
extern void mem_write0(int argc, char **argv);
extern void mem_write1(int argc, char **argv);
extern void fmtest(int argc, char **argv);
extern void i2c_read_cmd(int argc, char *argv[ ]);
extern void i2c_write_fill_cmd(int argc, char *argv[ ]);
extern void dog_reset_disable(int argc, char *argv[ ]);

#define MAX_ARGV_ARGS 10
static char shell_cmd_buf[64];
static int shell_cmd_index;
static char *argv[MAX_ARGV_ARGS];
int  shell_loop = 1;
#define MAX_CMDS 16
#define CMD_NAME_SIZE 16
#define CMD_SHHELP_SIZE 48

struct cmd {
	char name[CMD_NAME_SIZE];
	char shorthelp[CMD_SHHELP_SIZE];
	void (*func) (int, char *[]);
};

static struct cmd cmd_list[MAX_CMDS];
static const char *prompt;

static char shell_prompt[20];

void execute_cmd(char *buf)
{
	const char *delim = " \t";
	int argc = 0;
	int i = 0, arg_changed = 0;

	/* Parse command line and setup args */
	argv[argc++] = strtok(buf, delim);
	if (!argv[argc - 1]) {
		printf_err("Bad command token??\n");
		return;
	}
	while (1) {
		/* Can't handle more than 10 arguments */
		if (argc >= MAX_ARGV_ARGS)
			break;

		argv[argc++] = strtok(0, delim);
		if (!argv[argc - 1]) {
			argc--;
			break;
		}
	}

	/* Search for the command */

        if ((strncmp(argv[0], "md", 2) == 0) || (strncmp(argv[0], "mw", 2) == 0))
        {
            if ((*(*(argv) + 2)) == '.')
            {
               *(*(argv) + 2) = '\0';    
                arg_changed = 1;
            }    
        }    
	for (i = 0; i < MAX_CMDS; i++) {
		if (strcmp(cmd_list[i].name, argv[0]) == 0)
			break;
	}
        if (arg_changed)
        {
           *(*(argv) + 2) = '.';
        }       

	if (i >= MAX_CMDS) {
		printf_err("\n%s: command not found\n", buf);
		return;
	}

	if (!cmd_list[i].func) {
		printf_err("Bad function pointer!\n");
		return;
	}

	/* call the command handler */
	(cmd_list[i].func) (argc, argv);

}

void print_prompt(void)
{
	printf("\n\r%s",prompt);
}

void shell_run(void)
{
	int ch;
        int i = 0;
	do {
        ch = inbyte_nowait(); 
		switch (ch) {
			/* null char -> no data */
		case 0:
			break;
		case '\r':
		case '\n':
			{
				shell_cmd_buf[shell_cmd_index] = 0;
				shell_cmd_index = 0;
				if (shell_cmd_buf[0] != 0)
					execute_cmd(shell_cmd_buf);
				print_prompt();
			}
			break;
		case TAB:
			{
				putch(BEL);
			}
			break;
		case CTRL_C:
			{
				shell_cmd_buf[shell_cmd_index] = 0;
				shell_cmd_index = 0;
				print_prompt();
			}
			break;
		case BS:
		case DEL:
			{
				if (shell_cmd_index == 0) {
					putch(BEL);
				} else {
					shell_cmd_index--;
					shell_cmd_buf[shell_cmd_index] = 0;
					printf("\b \b");
				}
			}
			break;
		default:
			{
				shell_cmd_buf[shell_cmd_index++] = ch;
				printf("%c",ch);
			}
		}
	}
	while (0);
}

int shell_app_init(void)
{
	print_prompt();
	return 0;
}

int register_shell_cmd(const char *name,
        void (*func) (int argc, char *argv[]), const char *shorthelp)
{
    int i = 0;
	int ret = -1;

	if (!name || !name[0])
		goto out;

	for (i = 0; i < MAX_CMDS && cmd_list[i].name[0]; i++) ;
	if (i >= MAX_CMDS)
		goto out;

	strncpy(cmd_list[i].name, name, CMD_NAME_SIZE);
    strncpy(cmd_list[i].shorthelp, shorthelp, CMD_SHHELP_SIZE);
	cmd_list[i].func = func;

	ret = 0;
out:
	if (ret < 0) {
		printf("Failed to register [%s]\n", name);
	}
	return ret;
}


void shell_help(int argc, char *argv[])
{
	int i = 0;

	printf("\n\r---------------------------\n\r");
	printf("    Setup Command List:\n\r");
	printf("---------------------------\n\r");
	for (i = 0; i < MAX_CMDS; i++) {
		if (cmd_list[i].name[0]) {
			printf("%s\t\t%s\n\r", cmd_list[i].name, cmd_list[i].shorthelp);
		} else
			continue;
	}
    print_prompt();
}


void shell_init(const char *prmpt)
{
	int i = 0;

	for (i = 0; i < MAX_CMDS; i++)
		cmd_list[i].name[0] = 0;

	prompt = prmpt;
	register_shell_cmd("help", shell_help, "print this help message");

}

void shell_quit(int argc, char *argv[])
{
    shell_loop = 0;
}



void execute_shell_temp (void)
{
    int setup_delay = 5;
    int esc_delay = 2;
    char data;
    int esc_key_pressed = 0;

    strcpy(shell_prompt,"Boot1.5> ");
    shell_init(shell_prompt);
    register_shell_cmd("dram_ss", enable_dram_eye_finder, "Enable DRAM Strobe Sweep");
    register_shell_cmd("md", mem_read, "display memory");
    register_shell_cmd("mw0", mem_write0, "write to memory");
    register_shell_cmd("mw1", mem_write0, "write to memory");
    register_shell_cmd("pinfo", prog_info, "Print Program Info");
    register_shell_cmd("fmtest",fmtest,"Full memory test");
    register_shell_cmd("imd",i2c_read_cmd,"Read i2c bus");
    register_shell_cmd("imw",i2c_write_fill_cmd,"Write over i2c bus");
    register_shell_cmd("wd", dog_reset_disable,"Disable boot cpld watchdog reset");
    register_shell_cmd("quit", shell_quit, "Quit Current Setup");

#ifndef CFG_CONSOLE_INFO_QUIET
    printf("Hit ESC-CTRL-b to enter Boot1.5 Shell...");
#endif

    while(esc_delay)
    {
        esc_delay--;
        wwait(500000000); /* Wait for a 0.5 second */
        if ((data = inbyte_nowait()))
        {
            if (data == ESC) {
                esc_key_pressed = 1;
                break;
            }
        }
    }

    if (esc_key_pressed == 0) {
        return;
    }
    
    while(setup_delay)
    {
        setup_delay--;
        printf("%d", setup_delay);
        wwait(500000000); /* Wait for a 0.5 second */
        if ((data = inbyte_nowait()))
        {
            if (data == ESC) {
                esc_key_pressed = 1;
            } else {
                if (esc_key_pressed && (data == CTRL_B)) {
                    shell_help(0, NULL);
                    while(shell_loop)
                    {
                        shell_run();
                    }
                    break;
                } else {
                    esc_key_pressed = 0;
                }
            }
        }
        printf("\b");
    }
    printf("\n");
}

