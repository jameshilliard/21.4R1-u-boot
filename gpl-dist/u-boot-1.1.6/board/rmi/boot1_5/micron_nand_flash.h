/***********************************************************************
Copyright 2003-2008 Raza Microelectronics, Inc.(RMI). All rights
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
#ifndef __NAND_FLASH_H__
#define __NAND_FLASH_H__

#include "rmi/system.h"
#include "rmi/on_chip.h"
#include "rmi/rmi_processor.h"
#include "rmi/board.h"
#include "rmi/device.h"
#include "rmi/flashdev.h"

static void nand_write_cmd(unsigned long offset, unsigned char cmd)
{
	/*phoenix_write_reg((volatile uint32_t *)offset, 0, (uint32_t)cmd);*/
	*(volatile uint32_t *)offset = (uint32_t)cmd;
}

static void nand_write_addr(unsigned long offset, unsigned char addr)
{
	/*phoenix_write_reg((volatile uint32_t *)offset, 0, (uint32_t)addr);*/
	*(volatile uint32_t *)offset = (uint32_t)addr;
}

static void nand_read_multi(unsigned long offset, void *buf, unsigned short len)
{
	int i;
	volatile unsigned char *tbuf = (volatile unsigned char *)buf;

	for (i = 0; i < len; i++) {
		/**tbuf = phoenix_inb(offset);*/
		*tbuf = *(volatile unsigned char *)offset;
		tbuf++;
	}
}

static void nand_read_byte(unsigned long offset, void *buf)
{
	/**(volatile unsigned char *) buf = phoenix_inb(offset);*/
	*(volatile unsigned char *) buf = *(volatile unsigned char *)offset;
}

#define CLE_REG 0xbef19240 
#define ALE_REG 0xbef19280
//#define RW_OFFSET  0xbc000000 
#define RW_OFFSET  0xbfc00000 

#define WRITE_NAND_CLE(command)(nand_write_cmd(CLE_REG, command))
#define WRITE_NAND_ALE(address)(nand_write_addr(ALE_REG, address))
#define READ_NAND_BYTE(data)(nand_read_byte(RW_OFFSET, &(data)))
#define READ_NAND_ARRAY(data,n) (nand_read_multi(RW_OFFSET, data, n))

#endif /* __NAND_FLASH_H__ */
