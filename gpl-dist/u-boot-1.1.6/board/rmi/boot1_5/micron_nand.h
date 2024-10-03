/*******************************************************************************
*
*    File Name:  NAND_IO.h
*     Revision:  3.0
*         Date:  Jan 20, 2006
*        Email:  nandsupport@micron.com
*      Company:  Micron Technology, Inc.
*
*  Description:  Micron NAND I/O Driver
**
*   Disclaimer:  THIS DRIVER IS PROVIDED "AS IS" WITH NO WARRANTY
*                WHATSOEVER AND MICRON SPECIFICALLY DISCLAIMS ANY
*                IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR
*                A PARTICULAR PURPOSE, OR AGAINST INFRINGEMENT.
*
*                Copyright © 2006 Micron Technology, Inc.
*                All rights researved
*
* Rev  Author			Date		Changes
* ---  ---------------	----------	-------------------------------
* 1.0  WP				01/02/2006	Initial release
* 2.0  WP				01/20/2006	Updates to Disclaimer and header info.	  
* 3.0  dritz          06/12/2006  - Added Block Lock, OTP, and prog. Drive Strength

*/


#ifndef MICRON_NAND_H
#define MICRON_NAND_H

#define NAND_ID_MICRON 0x2C

/*------------------*/
/* NAND command set */
/*------------------*/
#define NAND_CMD_PAGE_READ_CYCLE1 0x00
#define	NAND_CMD_PAGE_READ_CYCLE2 0x30
#define NAND_CMD_READ_DATA_MOVE 0x35
#define NAND_CMD_RESET 0xFF
#define	NAND_CMD_PAGE_PROGRAM_CYCLE1 0x80
#define NAND_CMD_PAGE_PROGRAM_CYCLE2 0x10
#define NAND_CMD_CACHE_PRGM_CONFIRM 0x15
#define NAND_CMD_PRGM_DATA_MOVE	0x85
#define NAND_CMD_BLOCK_ERASE_CYCLE1 0x60
#define NAND_CMD_BLOCK_ERASE_CYCLE2 0xD0
#define NAND_CMD_RANDOM_DATA_INPUT 0x85
#define NAND_CMD_RANDOM_DATA_READ_CYCLE1	0x05
#define NAND_CMD_RANDOM_DATA_READ_CYCLE2	0xE0
#define NAND_CMD_READ_STATUS 0x70
#define NAND_CMD_READ_CACHE_START 0x31
#define NAND_CMD_READ_CACHE_LAST 0X3F
#define NAND_CMD_READ_UNIQUE_ID 0x65

#define NAND_CMD_DS 0xB8

#define	NAND_CMD_READ_ID 0x90
#define NAND_CMD_READ_UNIQUE_ID 0x65

#define	NAND_CMD_PROGRAM_OTP 0xA0
#define	NAND_CMD_PROTECT_OTP 0xA5
#define	NAND_CMD_READ_OTP 0xAF

#define	NAND_CMD_BLOCK_UNLOCK_CYCLE1 0x23
#define	NAND_CMD_BLOCK_UNLOCK_CYCLE2 0x24
#define	NAND_CMD_BLOCK_LOCK 0x2A
#define	NAND_CMD_BLOCK_LOCK_TIGHT 0x2C
#define	NAND_CMD_BLOCK_LOCK_STATUS 0x7A


/*---------------------------------*/
/* Maximum read status retry count */
/*---------------------------------*/
#define MAX_READ_STATUS_COUNT 100000

/*----------------------*/
/* NAND status bit mask */
/*----------------------*/
#define STATUS_BIT_0 0x01
#define STATUS_BIT_1 0x02
#define STATUS_BIT_5 0x20
#define STATUS_BIT_6 0x40

/*----------------------*/
/* NAND status response */
/*----------------------*/
#define NAND_IO_RC_PASS 0
#define NAND_IO_RC_FAIL 1
#define NAND_IO_RC_TIMEOUT 2


/*-------------------------*/
/* NAND device information */
/*-------------------------*/
#define NAND_PAGE_SIZE_BYTE 2112
#define NAND_PAGE_COUNT_PER_BLOCK 64
#define NAND_BLOCK_COUNT 1024
#define NAND_OTP_PAGES 0x03  //Higest Page ADDR starting from 0x02 = 10


#define NAND_SECTOR_BYTES 512
#define NAND_SECTOR_SPARE_BYTES 16

int nand_init(void);

int nand_reset(void);

int nand_readid(unsigned char *readid);

int nand_read_status(void);

int nand_read_uniqueid(unsigned short read_size, unsigned char *uniqueid);

int NAND_ReadPage(
	unsigned int a_uiPageNum,
	unsigned short a_usColNum,
	unsigned short a_usReadSizeByte,
	unsigned char *a_pucReadBuf);

int NAND_ReadPageRandomStart(
	unsigned int a_uiPageNum,
	unsigned short a_usColNum,
	unsigned short a_usReadSizeByte,
	unsigned char *a_pucReadBuf);

int NAND_ReadPageRandom(
	unsigned short a_usColNum,
	unsigned short a_usReadSizeByte, 
	unsigned char *a_pucReadBuf);

int NAND_ReadPageCacheStart(
	unsigned int a_uiPageNum,
	unsigned short a_usColNum);

int NAND_ReadPageCache(
	unsigned short a_usReadSizeByte,
	unsigned char *a_pucReadBuf);

int NAND_ReadPageCacheLast(
	unsigned short a_usReadSizeByte,
	unsigned char *a_pucReadBuf);

int nand_program_page( unsigned int page_num, unsigned short col_num, 
		unsigned short read_size_byte, unsigned char *read_buf);

int NAND_ProgramPageRandomStart(
	unsigned int a_uiPageNum,
	unsigned short a_usColNum,
	unsigned short a_usReadSizeByte,
	unsigned char *a_pucReadBuf);

int NAND_ProgramPageRandom(
	unsigned short a_usColNum,
	unsigned short a_usReadSizeByte,
	unsigned char *a_pucReadBuf);

int NAND_ProgramPageRandomLast(void);

int NAND_ProgramPageCache(
	unsigned int a_uiPageNum,
	unsigned short a_usColNum,
	unsigned short a_usReadSizeByte,
	unsigned char *a_pucReadBuf);

int NAND_ProgramPageCacheLast(
	unsigned int a_uiPageNum,
	unsigned short a_usColNum,
	unsigned short a_usReadSizeByte,
	unsigned char *a_pucReadBuf);

int NAND_ReadInternalDataMove(
	unsigned int a_uiPageNum,
	unsigned short a_usColNum);

int NAND_ProgramInternalDataMove(
	unsigned int a_uiPageNum,
	unsigned short a_usColNum);

int NAND_ProgramInternalDataMoveRandomStart(
	unsigned int a_uiPageNum,
	unsigned short a_usColNum,
	unsigned short a_usReadSizeByte,
	unsigned char *a_pucReadBuf);

int NAND_ProgramInternalDataMoveRandom(
	unsigned short a_usColNum,
	unsigned short a_usReadSizeByte,
	unsigned char *a_pucReadBuf);

int NAND_ProgramInternalDataMoveRandomLast(void);

int nand_erase_block(unsigned int blocknum);

int NAND_ReadDS(
	unsigned char *a_pucReadBuf);

int NAND_ProgDS(
	unsigned char *a_pucReadBuf);

int NAND_ProgramOTP(
	unsigned int a_uiPageNum,
	unsigned short a_usColNum,
	unsigned short a_usProgSizeByte,
	unsigned char *a_pucReadBuf);

int NAND_ProtectOTP(void);

int NAND_ReadOTP(
	unsigned int a_uiPageNum,
	unsigned short a_usColNum,
	unsigned short a_usReadSizeByte,
	unsigned char *a_pucReadBuf);

int NAND_ProgramInternalDataMoveRandomLast(void);

int nand_erase_block(unsigned int block_num);

int nand_block_lock(void);

int NAND_BlockLockTight(void);

int nand_block_unlock( unsigned int block_low, unsigned int block_high, 
		unsigned int invert);

int nand_block_lock_status( unsigned int block, unsigned char *read_status);

int nand_read_page( unsigned int page_num, unsigned short col_num,
		               unsigned short read_size_byte, unsigned char
			       *read_buf);

#endif /* MICRON_NAND_H */
