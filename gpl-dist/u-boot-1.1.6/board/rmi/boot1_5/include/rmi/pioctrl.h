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
#ifndef __IOBUS_H__
#define __IOBUS_H__

#define FLASH_DEV_PARM        2
#define FLASH_TIME_PARAM_0    3
#define FLASH_TIME_PARAM_1    4

#define FLASH_PCMCIA_CFG_OFFSET      0x50
#define FLASH_PCMCIA_STATUS_OFFSET   0x60
#define FLASH_GEN_STATUS_OFFSET      0x70
#define FLASH_GEN_ERROR_OFFSET       0x80

/* [11:6] = register id
 * [5:2] = Chip Select
 */
#define FLASH_CS_0            0
#define FLASH_CS_1            1
#define FLASH_CS_2            2
#define FLASH_CS_3            3
#define FLASH_CS_4            4
#define FLASH_CS_5            5
#define FLASH_CS_6            6
#define FLASH_CS_7            7
#define FLASH_CS_8            8
#define FLASH_CS_9            9

#define FLASH_PCMCIA_BOOT_CS      FLASH_CS_6
#define FLASH_BOOT_FLASH_CS       FLASH_CS_0

#define PIO_BASE_PHYS		(448 << 20)
#define PIO_BASE_SIZE		(32 << 20)

#ifndef __ASSEMBLY__
#include "rmi/types.h"
#include "rmi/printk.h"
#include "rmi/addrspace.h"
#include "rmi/rmi_processor.h"
#include "rmi/board.h"
#include "rmi/device.h"
#include "rmi/on_chip.h"
#define reg_offset(cs, reg) ( ( (((reg)&0x3f) << 6) | (((cs)&0x0f) << 2) ) >> 2)

enum flash_cs
{
	CS_BOOT_FLASH = 0,
	CS_CPLD       = 1,
	CS_SPI4_0     = 2,
    CS_SCPLD      = CS_SPI4_0, /* in SOHO board there is a Slave CPLD in CS 2 */     
	CS_SPI4_1     = 3,
	CS_ACC_FPGA   = CS_SPI4_1, /* in PA board there is a acc fpga in CS 3 */     
	CS_CPLD_I2C   = 4,
	CS_SECONDARY_FLASH = 2,
	CS_PCMCIA     = 6
};
enum flash_regs
{
	FLASH_CSBASE_ADDR_REG           = 0,
	FLASH_CSADDR_MASK_REG           = 1,
	FLASH_CSDEV_PARAM_REG           = 2,
	FLASH_CSTIME_PARAMA_REG         = 3,
	FLASH_CSTIME_PARAMB_REG         = 4,
/* 
   For the following flash registers, though space is provided
   for 10 chip selects, only chip select 0 is available. So,
   use cs as 0 whenever the following registers are read.
*/
	FLASH_INT_MASK_REG              = 5,
	FLASH_INT_STATUS_REG            = 6,
	FLASH_ERROR_STATUS_REG          = 7,
	FLASH_ERROR_ADDR_REG            = 8
};

void rmi_piobus_setup(struct rmi_board_info * rmib);
void rmi_tweak_pio_cs(struct rmi_board_info * rmib);
void rmi_init_piobus(struct rmi_board_info * rmib);
void rmi_init_piobus_devices(struct rmi_board_info * rmib);




#endif
#endif




