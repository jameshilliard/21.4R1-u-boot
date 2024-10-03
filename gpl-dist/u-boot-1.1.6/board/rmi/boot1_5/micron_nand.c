#include "micron_nand_flash.h"
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

