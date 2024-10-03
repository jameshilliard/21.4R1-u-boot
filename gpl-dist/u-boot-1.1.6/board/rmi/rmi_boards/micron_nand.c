#include "rmi/nand_flash.h"
#include "micron_nand.h"

int nand_reset(void)
{
	WRITE_NAND_CLE(NAND_CMD_RESET);
	return 0;
} 

/*
Description: The ReadID function is used to read 4 bytes of identifier code 
             which is programmed into the NAND device at the factory.  
*/
int nand_readid(unsigned char *readid)
{
	WRITE_NAND_CLE(NAND_CMD_READ_ID);		
	WRITE_NAND_ALE(0x00);	
	READ_NAND_ARRAY(readid, 4);
	return 0;
}

/*
Description: This function Reads the status register of the NAND device by 
             issuing a 0x70 command.	
Return code:
		NAND_IO_RC_PASS		=0 : The function completes operation successfully.
		NAND_IO_RC_FAIL		=1 : The function does not complete operation successfully.
		NAND_IO_RC_TIMEOUT	=2 : The function times out before operation completes.
*/
int nand_read_status(void)
{
	int status_count;
	unsigned char status;

	WRITE_NAND_CLE(NAND_CMD_READ_STATUS);		
	status_count = 0;

	while(status_count < MAX_READ_STATUS_COUNT)
	{
		/* Read status byte */
		READ_NAND_BYTE(status);
		/* Check status */
		if((status& STATUS_BIT_6) == STATUS_BIT_6)  /* if 1, device is ready */
		{
			if((status& STATUS_BIT_0) == 0)	/* if 0, the last operation was succesful */
				return NAND_IO_RC_PASS;
			else
				return NAND_IO_RC_FAIL;
		}			
		status_count++;
	}

	return NAND_IO_RC_TIMEOUT;
}


/* 
Description: This function reads the Unique ID information from the NAND device.
             The 16 byte Unique ID information starts at byte 0 of the page and
	     is followed by 16 bytes which are the complement of the first 16 
	     bytes. These 32 bytes of data are then repeated 16 times to form 
	     the 512 byte Unique ID> 	

Return code:
		NAND_IO_RC_PASS =		0 : The function completes operation successfully.
		NAND_IO_RC_FAIL =		1 : The function does not complete operation successfully.
		NAND_IO_RC_TIMEOUT =	2 : The function times out before operation completes.

*/

int nand_read_uniqueid(unsigned short read_size, unsigned char *uniqueid)
{
	int rc;

	WRITE_NAND_CLE(NAND_CMD_PAGE_READ_CYCLE2);
	WRITE_NAND_CLE(NAND_CMD_READ_UNIQUE_ID);	
	WRITE_NAND_CLE(NAND_CMD_PAGE_READ_CYCLE1);

	WRITE_NAND_ALE(0x00);
	WRITE_NAND_ALE(0x00);
	WRITE_NAND_ALE(0x02);
	WRITE_NAND_ALE(0x00);
	WRITE_NAND_ALE(0x00);
	
/* Issue Unique ID commands */
	WRITE_NAND_CLE(NAND_CMD_PAGE_READ_CYCLE2);

	rc = nand_read_status();						/* Wait for status */                                        
	if(rc != NAND_IO_RC_PASS)
		return rc;

	/* Issue Command - Set device to read from data register. */
	WRITE_NAND_CLE(NAND_CMD_PAGE_READ_CYCLE1);					

	READ_NAND_ARRAY(uniqueid, read_size);

	return rc;
} 

/*
Description: This function erases (returns all bytes in the block to 0xFF) a 
             block of data in the NAND device	

Return code:
		NAND_IO_RC_PASS = 0 : This function completes its operation successfully
		NAND_IO_RC_FAIL = 1 : This function does not complete its operation successfully
		NAND_IO_RC_TIMEOUT
*/

int nand_erase_block(unsigned int blocknum)
{
	int rc;
	unsigned int shifted_blocknum;
	shifted_blocknum = blocknum << 6;

	/* Issue command */
	WRITE_NAND_CLE(NAND_CMD_BLOCK_ERASE_CYCLE1);

	/* Issue address */
	WRITE_NAND_ALE((unsigned char)(shifted_blocknum & 0xFF)); /* Set page address byte 0 */ 
	WRITE_NAND_ALE((unsigned char)((shifted_blocknum >> 8) & 0xFF)); /* Set page address byte 1 */ 
	WRITE_NAND_ALE((unsigned char)((shifted_blocknum >> 16) & 0x0F)); /* Set page address byte 2 */

	/* Issue command */
	WRITE_NAND_CLE(NAND_CMD_BLOCK_ERASE_CYCLE2);

//	mdelay(20);
	rc = nand_read_status(); /* Wait for ready status */                                        

	return rc;
}

/* 
Description: The NAND_BlockLock function is used to lock the blocks.
Note:
		This feature will not work on all Micron NAND devices.
		MT29F1GxxABB, and some future products support these features.
*/
int nand_block_lock(void)
{
	WRITE_NAND_CLE(NAND_CMD_BLOCK_LOCK);
	return 0;
}

/*
Description: The nand_block_unlock function is used to lock the blocks.
Note:
		This feature will not work on all Micron NAND devices.
		MT29F1GxxABB, and some future products support these features.
*/
int nand_block_unlock( unsigned int block_low, unsigned int block_high, 
		unsigned int invert)
{
	/* Issue command */
	WRITE_NAND_CLE(NAND_CMD_BLOCK_UNLOCK_CYCLE1);
	/* Issue Address Cycles */
	WRITE_NAND_ALE((unsigned char)((block_low >> 8) & 0xFF)); /* Set page address byte 1 */ 
	WRITE_NAND_ALE((unsigned char) (block_low & 0xFF)); /* Set page address byte 2 */ 
	
	/* Issue command */
	WRITE_NAND_CLE(NAND_CMD_BLOCK_UNLOCK_CYCLE2);
	/* Issue Address Cycles */
	WRITE_NAND_ALE(((unsigned char)((block_high >> 8) & 0xFE)) | (invert & 0x0001)); /* Set page address byte 1 */ 
	WRITE_NAND_ALE ((unsigned char) (block_high & 0xFF)); /* Set page address byte 2 */ 

	return 0;
	
} 

/* 
Description: This function Reads the lock status of blocks
	Note:
		This feature will not work on all Micron NAND devices.
		MT29F1GxxABB, and some future products support these features.
	Return code:
		NAND_IO_RC_PASS		=0 : The function completes operation successfully.
		NAND_IO_RC_FAIL		=1 : The function does not complete operation successfully.
		NAND_IO_RC_TIMEOUT	=2 : The function times out before operation completes.
*/
int nand_block_lock_status( unsigned int block, unsigned char *read_status)
{
	/* Issue command */
	WRITE_NAND_CLE(NAND_CMD_BLOCK_LOCK_STATUS);		
	
	/* Issue Address Cycles */
	WRITE_NAND_ALE((unsigned char)((block >> 8) & 0xFF));	/* Set page address byte 1 */ 
	WRITE_NAND_ALE((unsigned char) (block & 0xFF));			/* Set page address byte 0 */ 

	/* Read Block Lock Status byte */
	READ_NAND_BYTE(*read_status);

	return 0;

}


/* 
Description:
        The nand_program_page function is used program a page of data into the NAND device.

	Return code:
		NAND_IO_RC_PASS =		0 : The function completes operation successfully.
		NAND_IO_RC_FAIL =		1 : The function does not complete operation successfully.
		NAND_IO_RC_TIMEOUT =	2 : The function times out before operation completes.

*/

int nand_program_page( unsigned int page_num, unsigned short col_num, 
		unsigned short read_size_byte, unsigned char *read_buf)
{
	int rc;

	/* Issue command */
	WRITE_NAND_CLE(NAND_CMD_PAGE_PROGRAM_CYCLE1); /* Issue 0x80 command */

	/* Issue Address */
	WRITE_NAND_ALE((unsigned char)(col_num & 0xFF)); /* Set column address byte 0 */
	WRITE_NAND_ALE((unsigned char)((col_num >> 8) & 0x0F)); /* Set column address byte 1 */
	WRITE_NAND_ALE((unsigned char)(page_num & 0xFF)); /* Set page address byte 0 */ 
	WRITE_NAND_ALE((unsigned char)((page_num >> 8) & 0xFF)); /* Set page address byte 1 */ 
	WRITE_NAND_ALE((unsigned char)((page_num >> 16) & 0x0F)); /* Set page address byte 2 */

	WRITE_NAND_ARRAY(read_buf ,read_size_byte); /* Input data to NAND device */	

	WRITE_NAND_CLE(NAND_CMD_PAGE_PROGRAM_CYCLE2); /* Issue 0x10 command */
//	mdelay(20);

	rc = nand_read_status(); /* Wait for status */                                        

	return rc;
}


int nand_read_page( unsigned int page_num, unsigned short col_num,
		               unsigned short read_size_byte, unsigned char
			       *read_buf)
{
	int rc;

	/* Issue command */
	WRITE_NAND_CLE(NAND_CMD_PAGE_READ_CYCLE1); /* Issue page read command cycle 1 */

	/* Issue Address */
	WRITE_NAND_ALE((unsigned char)(col_num & 0xFF));
	WRITE_NAND_ALE((unsigned char)((col_num >> 8) & 0x0F)); /* Set column address byte 1 */
	WRITE_NAND_ALE((unsigned char)(page_num & 0xFF)); /* Set page address byte 0 */
	WRITE_NAND_ALE((unsigned char)((page_num >> 8) & 0xFF)); /* Set page address byte 1 */
	WRITE_NAND_ALE((unsigned char)((page_num >> 16) & 0x0F)); /* Set page address byte 2 */

	/* Issue command */
	WRITE_NAND_CLE(NAND_CMD_PAGE_READ_CYCLE2);

	rc = nand_read_status();
	if(rc != NAND_IO_RC_PASS)
		return rc;

	/* Issue Command - Set device to read from data register. */
	WRITE_NAND_CLE(NAND_CMD_PAGE_READ_CYCLE1);

	READ_NAND_ARRAY(read_buf, read_size_byte);

	return rc;
}


