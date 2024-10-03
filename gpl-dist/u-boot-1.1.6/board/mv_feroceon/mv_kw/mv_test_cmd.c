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

//#include <config.h>
#include <common.h>
#include <command.h>

#if (CONFIG_COMMANDS)
#if !defined(CONFIG_PRODUCTION)
#if defined(MV_PRESTERA_SWITCH)

#if defined(MV_NAND_BOOT)
#include <nand.h>
#endif

#include "sflash/mvSFlash.h"
#include "mvCtrlEnvRegs.h"
#include "mvCommon.h"
#include "mvOs.h"
#include "eth-phy/mvEthPhy.h"
#include "mvBoardEnvLib.h"
#include "prestera/hwIf/mvHwIf.h"
#include "prestera/common/mvSysConf.h"


extern MV_U32   mvCpuIfTargetWinBaseLowGet(MV_TARGET target);
extern void mvMD5(unsigned char const *buf, unsigned len, unsigned char* digest);
extern void sflashInfoCopy(flash_info_t *flash_info, MV_SFLASH_INFO *pFlash);
extern int do_flerase (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
extern int do_protect (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
extern int do_mem_cmp (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
extern int do_mem_cp ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
extern int do_dhcp (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
extern int do_ping (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
extern int do_tftpb (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);

extern flash_info_t flash_info[];	/* info for FLASH chips */
extern ulong NetBootFileXferSize;	/* The actual transferred size of the bootfile (in bytes) */
 

static MV_SFLASH_INFO sflash;

#if 0
#define MV_DEBUG 
#endif
#ifdef MV_DEBUG
#define DB(x) x
#else
#define DB(x)
#endif
 

#define FLASH_BUG

#define TFTP_ADDR	"0x800000"
#define TFTP_ADDR_CMP	"0x1000000"

/* Look up command in command table */
#define FIND_CMD(cmdtp, name) \
{ \
    	if ((cmdtp = find_cmd(name)) == NULL) { \
		printf ("Unknown command '%s' - try 'help'\n", name); \
		return 1; \
	} \
} \


#define CHECK_CMD_STATUS(origFunc) \
{ \
    	int mvStatus = origFunc; \
    	if (0 != mvStatus) \
    	{ \
		printf("Test Failed !!!\n"); \
        	return mvStatus; \
    	} \
}

#define CHECK_CTRL_C(op) \
{ \
	if (ctrlc()) { \
		op; \
		putc ('\n'); \
		printf("Interrupted by user\n"); \
		return 1; \
	} \
}



#define DRAM_SIZE		_64M
#define DRAM_PROTECT		_16M
#define DRAM_LIMIT		DRAM_BASE + DRAM_SIZE
#define FLASH_BANK		1
#define U_BOOT_PROTECT		_1M

#define SECTOR_ID(addr)		(addr - sflash.baseAddr)/sflash.sectorSize

MV_U32 random(void)
{
	int i,rand;
	MV_U32 random[16];
	unsigned char digest[16];

	MV_REG_BIT_SET(0x1478, BIT7);
	for(i=0; i < 16;i++)
		random[i] = MV_REG_READ(0x1470);
	
	/* Run MD5 over the ftdll buffer */
	mvMD5((unsigned char*)random, 64, (MV_U8*)&digest);
	memcpy(&rand,digest,4);	
	return   rand;	
}


MV_STATUS eraseBySector
(
	unsigned long startAddr,
	unsigned long endAddr
)
{
	cmd_tbl_t *tmpcmdtp;
	MV_8 strErase[15];
	
	printf("erase %d:%d-%d\n",FLASH_BANK,
					SECTOR_ID(startAddr),
					SECTOR_ID(endAddr));
	
	sprintf((char*)strErase,"%d:%d-%d",FLASH_BANK,SECTOR_ID(startAddr),SECTOR_ID(endAddr));
	char* argv[] = {"erase", strErase};
	FIND_CMD(tmpcmdtp,"erase");
	CHECK_CMD_STATUS( do_flerase (tmpcmdtp, 0, 4, argv) );

	return MV_OK;
}

MV_STATUS eraseByAddress
(
	unsigned long startAddr,
	unsigned long size
)
{
	cmd_tbl_t *tmpcmdtp;
	MV_U32 alignedAddr,alignedSize;
	MV_8 strStartAddr[11],strSize[11];
	
	alignedAddr = startAddr -   (startAddr % sflash.sectorSize);
	alignedSize = size 	+   (startAddr % sflash.sectorSize);
	DB( printf("Start Address: 0x%08X Size: 0x%04X\n",
					startAddr,size) );
	DB( printf("Aligned Address: 0x%08X Aligned Size: 0x%04X\n",
					alignedAddr,alignedSize) );
	printf("erase 0x%08X +0x%04X",alignedAddr,alignedSize);

	sprintf((char*)strStartAddr,"0x%x",alignedAddr);
	sprintf((char*)strSize,"+0x%x",alignedSize);
	
	DB( printf("\nDebug: Start Address: %s\tSize %s\n",strStartAddr,strSize) );
	char* argv[] = {"erase", strStartAddr,strSize};

	FIND_CMD(tmpcmdtp,"erase");
	CHECK_CMD_STATUS( do_flerase (tmpcmdtp, 0, 3, argv) );
	
	return MV_OK;
}

void initFlashInfo(void)
{
    	sflash.baseAddr         = mvCpuIfTargetWinBaseLowGet(SPI_CS);
    	sflash.sectorNumber     = flash_info[FLASH_BANK-1].sector_count;                        
    	sflash.sectorSize       = flash_info[FLASH_BANK-1].size/flash_info[FLASH_BANK-1].sector_count;                        
        
	DB( printf("Base: 0x%08X\nSectors: 0x%08X\nSector Size: 0x%08X\nFlashSize: 0x%08X\n",
		sflash.baseAddr,sflash.sectorNumber,sflash.sectorSize,sflash.sectorNumber*sflash.sectorSize) );
}

int protectFlash(void)
{
	cmd_tbl_t *tmpcmdtp;
	char* argv[] = {"protect", "on","all"};

	printf("protect on all\n");

	FIND_CMD(tmpcmdtp,"protect");
	CHECK_CMD_STATUS( do_protect (tmpcmdtp, 0, 3, argv) );
}

int do_flash_test(void)
{
	unsigned long size,flashAddr,dramAddr;
	cmd_tbl_t *tmpcmdtp;

	initFlashInfo();
	MV_U32 FLASH_SIZE =  sflash.sectorNumber*sflash.sectorSize;
	MV_U32 FLASH_LIMIT=  sflash.baseAddr + FLASH_SIZE;

	printf("protect off 0x%08X +0x%08X\n",sflash.baseAddr+U_BOOT_PROTECT,
				              FLASH_SIZE-U_BOOT_PROTECT);
	{
	MV_8 startAddr[11],strSize[11];
	sprintf((char*)startAddr,"0x%x",sflash.baseAddr+U_BOOT_PROTECT);
	sprintf((char*)strSize,"+0x%x",FLASH_SIZE-U_BOOT_PROTECT);
	char* argv[] = {"protect", "off",startAddr,strSize};
	FIND_CMD(tmpcmdtp,"protect");
	CHECK_CMD_STATUS( do_protect (tmpcmdtp, 0, 4, argv) );
	}

	for (;;) 
	{
		CHECK_CTRL_C(protectFlash())

		do 
		{
			int x = random();
			flashAddr = sflash.baseAddr + U_BOOT_PROTECT +
						(x % (FLASH_SIZE-U_BOOT_PROTECT));
		} while(flashAddr >= FLASH_LIMIT
			#if defined(FLASH_BUG)
			/* Bug in flash driver: 
 			   odd addresses cannot be copies */
				|| (flashAddr % 2)
			#endif
			);

		do 
		{
			size   = random() % (FLASH_SIZE-U_BOOT_PROTECT);

		} while((flashAddr + size) > FLASH_LIMIT);

		do 
		{
			dramAddr   = DRAM_BASE + DRAM_PROTECT + (random() % DRAM_SIZE);
		} while((dramAddr + size) > DRAM_LIMIT
			#if defined(FLASH_BUG)
			/* Bug in flash driver: 
 			   odd addresses cannot be copies */
				|| (dramAddr % 2)
			#endif
			);

		if (random() % 2)
		{
			CHECK_CMD_STATUS( eraseBySector(flashAddr,flashAddr+size) );
		}
		else
		{
			CHECK_CMD_STATUS( eraseByAddress(flashAddr,size) );
		}

		CHECK_CTRL_C(protectFlash())

		MV_8 srcAddr[11],destAddr[11],strSize[11];
		sprintf((char*)srcAddr,"0x%x",dramAddr);
		sprintf((char*)destAddr,"0x%x",flashAddr);
		sprintf((char*)strSize,"0x%x",size);

		printf("cp.b  0x%08X 0x%08X 0x%04X\n",dramAddr,flashAddr,size);
		{
		DB( printf("cp.b params: Src: %s\tDest: %s\tSize: %s\n", srcAddr, destAddr, strSize) );
		char* argv[] = {"cp.b", srcAddr, destAddr, strSize};
		FIND_CMD(tmpcmdtp,"cp");
		CHECK_CMD_STATUS( do_mem_cp (tmpcmdtp, 0, 4, argv) );
		}

		CHECK_CTRL_C(protectFlash())

		printf("cmp.b 0x%08X 0x%08X 0x%04X\n",dramAddr,flashAddr,size);
		{
		DB( printf("cmp.b params: Src: %s\tDest: %s\tSize: %s\n", srcAddr, destAddr, strSize) );
		char* argv[] = {"cmp.b", srcAddr, destAddr, strSize};
		FIND_CMD(tmpcmdtp,"cmp");
		CHECK_CMD_STATUS( do_mem_cmp (tmpcmdtp, 0, 4, argv) );
		}
		
		printf("\n");
	}
 
	return 1;
}

U_BOOT_CMD(
	test_flash,      1,     1,      do_flash_test,
	"test_flash	- test flash interfaces.\n",
    	"\n"
);

#if defined(MV_NAND_BOOT)
int do_nand_test(void)
{
	nand_info_t *nand = &nand_info[0];
	nand_erase_options_t er_opts;
	nand_write_options_t wr_opts;
	nand_read_options_t rd_opts;
	u_char readBuff[0x40000];
	u_char* dataBuff;
	unsigned long testOffset, testMaxSize;
	unsigned long size, startAddr;
	int i, ret;
	
	testOffset = 0xC0000;
	testMaxSize = 0x40000;
	size = testMaxSize;
	startAddr = 0;	

//	dataBuff = (u_char*)malloc(size);
//	if (dataBuff == NULL){ printf("\n Malloc Fail 1"); return 0;}
	dataBuff = (u_char*)malloc(0x40000);
	if (readBuff == NULL){ printf("\n Malloc Fail 2"); return 0;}

	for(i = 0; i < size; i++)
	{
		dataBuff[i] = (i*23) % 256;
	}

	printf("Erase 0x%X - 0x%X ... ",testOffset, testMaxSize);
	memset(&er_opts, 0, sizeof(er_opts));
	er_opts.offset = testOffset;
	er_opts.length = testMaxSize;
	er_opts.quiet  = 1;
	ret = nand_erase_opts(nand, &er_opts);
	printf("status 0x%X", ret);
	
	printf("\nCopy to Nand Flash 0x%X - 0x%X... ", testOffset+startAddr, size);
	memset(&wr_opts, 0, sizeof(wr_opts));
	wr_opts.buffer	= (u_char*) dataBuff;
	wr_opts.offset	= testOffset+startAddr;
	wr_opts.length	= size;
	/* opts.forcejffs2 = 1; */
	wr_opts.pad	= 1;
	wr_opts.blockalign = 1;
	wr_opts.quiet      = 1;
	ret = nand_write_opts(nand, &wr_opts);
	printf("status 0x%X", ret);

	size = 0x40000;
	printf("\nRead Nand Flash... ");
for( startAddr = 0; startAddr+size <= testMaxSize; startAddr+=size)
{
	rd_opts.buffer	= (u_char*) readBuff;
	rd_opts.offset	= testOffset+startAddr;
	rd_opts.length	= size;
	rd_opts.readoob	= 0;
	rd_opts.quiet   = 1;
	ret = nand_read_opts(nand, &rd_opts);

	ret = 0;
	for(i = 0; i < size; i++)
	{
		if( dataBuff [i] != readBuff[i] )
		{
			printf("\nFailed at 0x%X, expected 0x%X, read 0x%X",	
				testOffset+startAddr+i, dataBuff[i], readBuff[i]);
			ret++;
		}

//		printf("\Ok at 0x%X, expected 0x%X, read 0x%X",	
//				testOffset+i, &((u_char*)dataBuff)[i], &((u_char*)readBuff)[i]);
		if( ret > 10 ) 
		{
			printf("\nFailed\n");
			break;
		}
	}
	printf("~");
}

free((void*)dataBuff);
//free((void*)readBuff);
	printf("\nPassed !!\n");
	return 1;
}

U_BOOT_CMD(
	test_nand,      1,     1,      do_nand_test,
	"test_nand	- test NAND flash interfaces.\n",
    	"\n"
);

#endif /* defined(MV_NAND_BOOT) */

#define CRYPT_SRAM_BASE		CRYPT_ENG_BASE	
#define CRYPT_SRAM_SIZE		_2K
#define CRYPT_SRAM_LIMIT	CRYPT_SRAM_BASE + CRYPT_SRAM_SIZE

int do_sram_test(void)
{
	unsigned long size,sramAddr,dramAddr;
	cmd_tbl_t *tmpcmdtp;

	for (;;) 
	{
		CHECK_CTRL_C()

		do 
		{
			sramAddr = CRYPT_SRAM_BASE + (random() % CRYPT_SRAM_SIZE);
		} while(sramAddr >= CRYPT_SRAM_LIMIT);

		do 
		{
			size   = random() % CRYPT_SRAM_SIZE;
		} while(((sramAddr + size) > CRYPT_SRAM_LIMIT) || (size==0));

		do 
		{
			dramAddr   = DRAM_BASE + (random() % DRAM_SIZE);
		} while((dramAddr + size) > DRAM_LIMIT);

		MV_8 srcAddr[11],destAddr[11],strSize[11];
		sprintf((char*)srcAddr,"0x%x",dramAddr);
		sprintf((char*)destAddr,"0x%x",sramAddr);
		sprintf((char*)strSize,"0x%x",size);
	
		printf("cp.b  0x%08X 0x%08X 0x%04X\n",dramAddr,sramAddr,size);
		{
		DB( printf("cp.b params: Src: %s\tDest: %s\tSize: %s\n", srcAddr, destAddr, strSize) );
		char* argv[] = {"cp.b", srcAddr, destAddr, strSize};
		FIND_CMD(tmpcmdtp,"cp");
		CHECK_CMD_STATUS( do_mem_cp (tmpcmdtp, 0, 4, argv) );
		}

		printf("cmp.b 0x%08X 0x%08X 0x%04X\n",dramAddr,sramAddr,size);
		{
		DB( printf("cmp.b params: Src: %s\tDest: %s\tSize: %s\n", srcAddr, destAddr, strSize) );
		char* argv[] = {"cmp.b", srcAddr, destAddr, strSize};
		FIND_CMD(tmpcmdtp,"cmp");
		CHECK_CMD_STATUS( do_mem_cmp (tmpcmdtp, 0, 4, argv) );
		}
		
		printf("\n");
	}
 
	return 1;
}

U_BOOT_CMD(
	test_sram,      1,     1,      do_sram_test,
	"test_sram	- test sram interfaces.\n",
    	"\n"
);

int do_cpu_pp_conn_test(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	cmd_tbl_t *tmpcmdtp;
	char *gatewayIp,*serverIp,*imageName;
	MV_U32	vendirId,checkTftp;
	MV_U32 devNum = PRESTERA_DEFAULT_DEV;
	MV_U32 rand;
	MV_8 strSize[11];

	switch (argc) {
	case 1:
		checkTftp = 0;	
		break;
	case 2:
		checkTftp = 0;
		devNum = argv[1];	
		break;
	case 3:
		checkTftp = 1;
		serverIp = argv[1];
		imageName = argv[2];
		printf("Setting server ip to %s\n", serverIp);
		setenv("serverip", serverIp);	
		break;
	case 4:
		checkTftp = 1;
		devNum = argv[1];
		serverIp = argv[2];
		imageName = argv[3];
		printf("Setting server ip to %s\n", serverIp);
		setenv("serverip", serverIp);
		break;	
	default: printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	printf("Checking device %d\n", devNum);
	mvSwitchReadReg(devNum, 0x50, &vendirId);
	if(vendirId != MV_VENDOR_ID)
	{
		printf("Unrecognized vendor ID\n");
		goto error;
	}

	printf("dhcp\n");
	{
	char* argv[] = {"dhcp"};
	FIND_CMD(tmpcmdtp,"dhcp");
	CHECK_CMD_STATUS( do_dhcp (tmpcmdtp, 0, 1, argv) );
	}

	if(checkTftp)
	{
		printf("Setting serverip to %s\n", serverIp);
		setenv("serverip", serverIp);
	}

	for (;;) 
	{
		CHECK_CTRL_C();

		rand = random() * checkTftp;

		if (rand % 3 == 0)
		{
			if ((gatewayIp = getenv("gatewayip")) == NULL) {
				printf("Unable tp read gatewayip environment variable\n");
				goto error;
			}

			printf("ping %s\n",gatewayIp);
			char* argv[] = {"ping",gatewayIp};
			FIND_CMD(tmpcmdtp,"ping");
			CHECK_CMD_STATUS( do_ping (tmpcmdtp, 0, 2, argv) );
		}
		else if (rand % 3 == 1)
		{
			printf("ping %s\n",serverIp);
			char* argv[] = {"ping",serverIp};
			FIND_CMD(tmpcmdtp,"ping");
			CHECK_CMD_STATUS( do_ping (tmpcmdtp, 0, 2, argv) );
		}
		else
		{
			{
			printf("tftp %s %s\n",TFTP_ADDR, imageName);
			char* argv[] = {"tftp",TFTP_ADDR, imageName};
			FIND_CMD(tmpcmdtp,"tftp");
			CHECK_CMD_STATUS( do_tftpb (tmpcmdtp, 0, 3, argv) );
			}

			{
			printf("tftp %s %s\n",TFTP_ADDR_CMP, imageName);
			char* argv[] = {"tftp",TFTP_ADDR_CMP, imageName};
			FIND_CMD(tmpcmdtp,"tftp");
			CHECK_CMD_STATUS( do_tftpb (tmpcmdtp, 0, 3, argv) );
			}
	
			
			sprintf((char*)strSize,"0x%x",NetBootFileXferSize);
	
			printf("cmp.b %s %s 0x%04X\n",TFTP_ADDR, TFTP_ADDR_CMP, NetBootFileXferSize);
			{
			DB( printf("cmp.b params: Src: %s\tDest: %s\tSize: %s\n", TFTP_ADDR, TFTP_ADDR_CMP, strSize) );
			char* argv[] = {"cmp.b", TFTP_ADDR, TFTP_ADDR_CMP, strSize};
			FIND_CMD(tmpcmdtp,"cmp");
			CHECK_CMD_STATUS( do_mem_cmp (tmpcmdtp, 0, 4, argv) );
			}
		}

		printf("\n");
	}
 
	return 1;
error:
	printf("Test Failed !!!\n");
	return 0;	
}

U_BOOT_CMD(
	test_cpu_pp_conn,      4,     1,      do_cpu_pp_conn_test,
	"test_cpu_pp	- test CPU and PP connectivity.\n",
    	"\n"
);

#define PHY_VENDOR_ID 0x0141

int do_smi_test(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	MV_U32	vendirId=0, ethPortNum;

	switch (argc) {
	case 1:
		ethPortNum = 0;	
		break;
	case 2:
		ethPortNum = simple_strtoul( argv[1], NULL, 10 );
		break;
	default: printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	printf("Checking for port %d\n", ethPortNum);
	mvEthPhyRegRead(mvBoardPhyAddrGet(ethPortNum),2,&vendirId);
	if(vendirId != PHY_VENDOR_ID)
	{
		printf("PHY Vendor ID: 0x%04x\n",vendirId);
		printf("Expected PHY Vendor ID: 0x%04x\n",PHY_VENDOR_ID);
		goto error;
	}

	printf("Test Passed\n");
	return 1;
 error:
	printf("Test Failed !!!\n");
	return 0;
}

U_BOOT_CMD(
	test_smi,      2,     1,      do_smi_test,
	"test_smi	- test smi interfaces.\n",
    	"\n"
);


#endif  /* #if defined(MV_PRESTERA_SWITCH) */
#endif  /* #if !defined(CONFIG_PRODUCTION) */

#endif	/* #if (CONFIG_COMMANDS) */

