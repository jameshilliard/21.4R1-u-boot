/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or 
modify this File in accordance with the terms and conditions of the General 
Public License Version 2, June 1991 (the "GPL License"), a copy of which is 
available along with the File in the license.txt file or by writing to the Free 
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or 
on the worldwide web at http://www.gnu.org/licenses/gpl.txt. 

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED 
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY 
DISCLAIMED.  The GPL License provides additional details about this warranty 
disclaimer.

*******************************************************************************/

#include <common.h>
#include <command.h>

#if (CONFIG_COMMANDS)
#if !defined(CONFIG_PRODUCTION)
#if defined(MV_PRESTERA_SWITCH)

#include "prestera/hwIf/mvHwIf.h"
#include "prestera/util/mvUtils.h"


int sir_cmd( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[] )
{
	MV_U32 regNum = 0x0, regVal, regValTmp, res;
	unsigned int 	devNum = PRESTERA_DEFAULT_DEV;
	MV_8 regValBin[40];
	MV_8 cmd[30];
	int i,j = 0, flagm = 0;
	extern MV_8 console_buffer[];

	if( argc == 2 ) {
		regNum = simple_strtoul( argv[1], NULL, 16 );
	}
	else if( argc == 3 ) {
		regNum = simple_strtoul( argv[1], NULL, 16 );
		devNum = simple_strtoul( argv[2], NULL, 16 );
	}

	else { 
		printf( "Usage:\n%s\n", cmdtp->usage );
		return 0;
	}  

	if (devNum >= mvSwitchGetDevicesNum())
	{
		printf( "Error: Invalid device number: %d\n", devNum);
		return 0;
	}

	if (mvSwitchReadReg(devNum, regNum, &regVal)!=MV_OK) {
		printf( "Error: Problem reading register 0x%08x\n", regNum);
		return 0;
	}
	regValTmp = regVal;
	printf( "Switch Internal register 0x%x value : 0x%x\n ",regNum, regVal );
	printf( "\n    31      24        16         8         0" );
	printf( "\n     |       |         |         |         |\nOLD: " );

	for( i = 31 ; i >= 0 ; i-- ) {
		if( regValTmp > 0 ) {
			res = regValTmp % 2;
			regValTmp = (regValTmp - res) / 2;
			if( res == 0 )
				regValBin[i] = '0';
			else
				regValBin[i] = '1';
		}
		else
			regValBin[i] = '0';
	}

	for( i = 0 ; i < 32 ; i++ ) {
		printf( "%c", regValBin[i] );
		if( (((i+1) % 4) == 0) && (i > 1) && (i < 31) )
			printf( "-" );
	}

	readline( "\nNEW: " );
	strcpy(cmd, console_buffer);
	if( (cmd[0] == '0') && (cmd[1] == 'x') ) {
		regVal = simple_strtoul( cmd, NULL, 16 );
		flagm=1;
	}
	else {
		for( i = 0 ; i < 40 ; i++ ) {
			if(cmd[i] == '\0')
				break;
			if( i == 4 || i == 9 || i == 14 || i == 19 || i == 24 || i == 29 || i == 34 )
				continue;
			if( cmd[i] == '1' ) {
				regVal = regVal | (0x80000000 >> j);
				flagm = 1;
			}
			else if( cmd[i] == '0' ) {
				regVal = regVal & (~(0x80000000 >> j));
				flagm = 1;
			}
			j++;
		}
	}

	if( flagm == 1 ) {
		if (mvSwitchWriteReg(devNum, regNum, regVal)!=MV_OK) {
			printf( "Error: Problem writing register 0x%08x\n", regNum);
			return 0;
		}

		if (mvSwitchReadReg(devNum, regNum, &regVal)!=MV_OK) {
			printf( "Error: Problem reading register 0x%08x\n", regNum);
			return 0;
		}
		printf( "\nNew value = 0x%x\n\n", regVal);
	}
	return 1;
}

U_BOOT_CMD(
	sir,      3,     1,      sir_cmd,
	"sir	- reading and changing MV Switch internal register values.\n",
	" [device] address\n"
	"\tDisplays the contents of the switch internal register in 2 forms, hex and binary.\n"
	"\tIt's possible to change the value by writing a hex value beginning with \n"
	"\t0x or by writing 0 or 1 in the required place. \n"
    	"\tPressing enter without any value keeps the value unchanged.\n"
);

/* Display values from last command.
 * modify remembered values are different from display.
 */
uint	int_reg_disp_last_addr;
uint	int_reg_disp_last_length = 0x40;
uint    int_reg_disp_last_dev = PRESTERA_DEFAULT_DEV;

uint	int_reg_mod_last_addr;
uint	int_reg_mod_last_dev = PRESTERA_DEFAULT_DEV;

static int mod_mem(cmd_tbl_t *, int, int, int, char *[]);

/* Switch Internal Registers Display
 *
 * Syntax:
 *	smd{.b, .w, .l} {offset} {number}
 */
#define DISP_LINE_LEN	4	/* registers per line*/
int do_switch_md ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	unsigned int	addr, length, origLength, regVal, devNum;
	unsigned int	i, lineRegs;
	int rc = 0;

	/* We use the last specified parameters, unless new ones are
	 * entered.
	 */
	addr = int_reg_disp_last_addr;
	length = int_reg_disp_last_length;
	devNum = int_reg_disp_last_dev;

	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 0;
	}

	if ((flag & CMD_FLAG_REPEAT) == 0) {
		/* Address is specified since argc > 1
		*/
		addr = simple_strtoul(argv[1], NULL, 16);

		/* If another parameter, it is the length to display.
		 * Length is the number of objects, not number of bytes.
		 */
		if (argc > 2 )
			length = simple_strtoul(argv[2], NULL, 16);
		if (argv > 3)
			devNum = simple_strtoul(argv[3], NULL, 16);
	}

	origLength = length;

	if (devNum >= mvSwitchGetDevicesNum())
	{
		printf( "Error: Invalid device number: %d\n", devNum);
		return 0;
	}

	/* Print the lines.
	 *
	 * We buffer all read data, so we can make sure data is read only
	 * once, and all accesses are with the specified bus width.
	 */
	do {
		printf("%08lx:", addr);
		lineRegs = (length>DISP_LINE_LEN)?DISP_LINE_LEN:length;

		for (i=0; i<lineRegs; i++) {
			if (mvSwitchReadReg(devNum, addr, &regVal)!=MV_OK) {
				printf( "Error: Problem reading register 0x%08x\n", addr);
				return 0;
			}
			printf(" %08x", regVal);
			addr += 4;
		}
		puts ("    ");
		for (i=0; i<lineRegs; i++) {
			printf("%s", "....");
		}
		putc ('\n');
		length -= lineRegs;
		if (ctrlc()) {
			rc = 1;
			break;
		}
	} while (length > 0);

	int_reg_disp_last_addr = addr;
	int_reg_disp_last_length = origLength;
	int_reg_disp_last_dev = devNum;
	return (rc);
}

U_BOOT_CMD(
	smd,     4,     1,      do_switch_md,
	"smd      - switch internal register display\n",
	"[.b, .w, .l] address [# of objects] [device]\n    - switch internal register display\n"
);

/* Switch internal register memory.
 *
 * Syntax:
 *	mm{.b, .w, .l} {addr}
 *	nm{.b, .w, .l} {addr}
 */

static int
switch_mod_reg(cmd_tbl_t *cmdtp, int incrflag, int flag, int argc, char *argv[])
{
	unsigned int	addr, regVal;
	unsigned int 	devNum = PRESTERA_DEFAULT_DEV;
	int	nbytes, regSize;
	extern char console_buffer[];

	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 0;
	}

	/* We use the last specified parameters, unless new ones are
	 * entered.
	 */
	addr = int_reg_mod_last_addr;
	devNum = int_reg_mod_last_dev;
	regSize = 4; /* address increamets in 4 bytes*/

	if ((flag & CMD_FLAG_REPEAT) == 0) {
		if (argc > 1)
		{
			/* Address is specified since argc > 1
			*/
			addr = simple_strtoul(argv[1], NULL, 16);
		}
		if (argc > 2)
		{
			/* Device is specified since argc > 2
			*/
			devNum = simple_strtoul(argv[2], NULL, 16);
		}
	}

	if (devNum >= mvSwitchGetDevicesNum())
	{
		printf( "Error: Invalid device number: %d\n", devNum);
		return 0;
	}

	/* Print the address, followed by value.  Then accept input for
	 * the next value.  A non-converted value exits.
	 */
	do {
		printf("%08lx:", addr);
		if (mvSwitchReadReg(devNum, addr, &regVal)!=MV_OK) {
			printf( "Error: Problem reading register 0x%08x\n", addr);
			return 0;
		}
		printf(" %08x", regVal);

		nbytes = readline (" ? ");
		if (nbytes == 0 || (nbytes == 1 && console_buffer[0] == '-')) {
			/* <CR> pressed as only input, don't modify current
			 * location and move to next. "-" pressed will go back.
			 */
			if (incrflag)
				addr += nbytes ? -regSize : regSize;
			nbytes = 1;
		}
		else {
			char *endp;
			regVal = simple_strtoul(console_buffer, &endp, 16);
			nbytes = endp - console_buffer;
			if (nbytes) {
			if (mvSwitchWriteReg(devNum, addr, regVal)!=MV_OK) {
				printf( "Error: Problem writing register 0x%08x\n", addr);
				return 0;
			}

			if (incrflag)
				addr += regSize;
			}
		}
	} while (nbytes);

	int_reg_mod_last_addr = addr;
	int_reg_mod_last_dev = devNum;

	return 1;
}

int do_switch_mm ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return switch_mod_reg (cmdtp, 1, flag, argc, argv);
}


U_BOOT_CMD(
	smm,     3,      1,       do_switch_mm,
	"smm      - switch internal register modify (auto-incrementing)\n",
	"[.b, .w, .l] address [device]\n" "    - switch internal registery modify, auto increment address\n"
);

int do_switch_mw ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong	addr, writeval, count, devNum = PRESTERA_DEFAULT_DEV;
	int	size = 4; /* register size is 4 bytes*/

	if (argc < 3) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 0;
	}

	/* Check for size specification.
	*/
	if ((size = cmd_get_data_size(argv[0], 4)) < 1)
		return 0;

	addr = simple_strtoul(argv[1], NULL, 16);

	/*addr += base_address;*/

	/* Get the value to write.
	*/
	writeval = simple_strtoul(argv[2], NULL, 16);

	/* Count ? */
	count = (argc > 3) ? count = simple_strtoul(argv[3], NULL, 16) : 1;
	
	if (argc > 4)
		devNum = simple_strtoul(argv[4], NULL, 16);

	if (devNum >= mvSwitchGetDevicesNum())
	{
		printf( "Error: Invalid device number: %d\n", devNum);
		return 0;
	}
 
	while (count-- > 0) {
		if (mvSwitchWriteReg(devNum, addr, writeval)!=MV_OK) {
			printf( "Error: Problem writing register 0x%08x\n", addr);
			return 0;
		}

		addr += size;
	}
	return 1;
}

U_BOOT_CMD(
	smw,     5,      1,       do_switch_mw,
	"smw      - switch internal register modify - register fill\n",
	"[.b, .w, .l] address value [count] [device]\n" "    - switch internal registery modify - register fill\n"
);

int do_switch_mib_counter_read ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	MV_U8 port;
	if( argc != 2 ) {
		printf( "Usage:\n%s\n", cmdtp->usage );
		return 0;
	}
	
	port = simple_strtoul( argv[1], NULL, 10 );
	if (port >= mvSwitchGetPortsNum())
	{
		printf( "Error: Invalid port num\n");
		return 0;
	}

	mvPresteraReadPortMibCounters(port);

	return 1;
}

U_BOOT_CMD(
	switchMibRead,     2,      1,       do_switch_mib_counter_read,
	"switchMibRead      - read switch MIB counters for specified port\n",
	"port\n" "    - read switch MIB counter for specified port\n"
);

#if !defined(EX2200)
int do_poe_init ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong retVal;

	if (argc != 1) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	printf("Initializing POE subsystem ...");
	retVal = mvPoeInit();
	printf("%s\n",(retVal) ? "Failed" : "Done");

	return !retVal;
}

U_BOOT_CMD(
	poeInit,     1,      1,       do_poe_init,
	"poeInit - Initialize POE subsystem\n",
	"\n" "	- Initialize POE subsystem\n"
);

int do_poe_enable ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	MV_BOOL val, status;

	if (argc != 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 0;
	}

	/* Get status */
	val = simple_strtoul(argv[1], NULL, 10);
	if(val==0)
		status = MV_FALSE;
	else if(val==1)
		status = MV_TRUE;
	else
	{
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 0;
	}

	printf("%s POE subsystem\n",
		(val) ? "Enabling" : "Disabling");
	return !mvPoeEnableSet(status);
}

U_BOOT_CMD(
	poeEnable,     2,      1,       do_poe_enable,
	"poeEnable - Enable/Disable POE subsystem\n",
	"0/1 (0 - Disable, 1 - Enable)\n" 
	"	- Enable/Disable POE subsystem\n"
);

int do_poe_download ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong	srcAddr, size, retVal;

	if (argc != 3) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 0;
	}

	/* Get source address*/
	srcAddr = simple_strtoul(argv[1], NULL, 16);

	/* Get image size */
	size = simple_strtoul(argv[2], NULL, 16);
	
	printf("Loading POE subsystem from address 0x%x ...",srcAddr);
	retVal = mvPoeSWDownload(srcAddr,size);
	printf("%s\n",(retVal) ? "Failed" : "Done");
	return !retVal;
}

U_BOOT_CMD(
	poeDownload,     3,      1,       do_poe_download,
	"poeDownload - Loads POE subsystem code into Dragonite RAM\n",
	"<image src address> <image size>\n" 
	"	- Loads POE subsystem code into Dragonite RAM\n"
);

#endif
#endif  /* #if defined(MV_PRESTERA_SWITCH) */
#endif  /* #if !defined(CONFIG_PRODUCTION) */

#endif	/* #if (CONFIG_COMMANDS) */

