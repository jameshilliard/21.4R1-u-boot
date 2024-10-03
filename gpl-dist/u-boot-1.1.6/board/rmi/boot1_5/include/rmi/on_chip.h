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
#ifndef _ON_CHIP_H
#define _ON_CHIP_H

#include "rmi/types.h"

#define DEFAULT_PHOENIX_IO_BASE 0xffffffffbef00000ULL

#define PHOENIX_IO_BRIDGE_OFFSET          0x00000

#define PHOENIX_IO_DDR2_CHN0_OFFSET       0x01000
#define PHOENIX_IO_DDR2_CHN1_OFFSET       0x02000
#define PHOENIX_IO_DDR2_CHN2_OFFSET       0x03000
#define PHOENIX_IO_DDR2_CHN3_OFFSET       0x04000

#define PHOENIX_IO_RLD2_CHN0_OFFSET       0x05000
#define PHOENIX_IO_RLD2_CHN1_OFFSET       0x06000

#define PHOENIX_IO_SRAM_OFFSET            0x07000

#define PHOENIX_IO_PIC_OFFSET             0x08000
#define PHOENIX_IO_PCIX_OFFSET            0x09000
#define PHOENIX_IO_HT_OFFSET              0x0A000

#define PHOENIX_IO_SECURITY_OFFSET        0x0B000

#define PHOENIX_IO_GMAC_0_OFFSET          0x0C000
#define PHOENIX_IO_GMAC_1_OFFSET          0x0D000
#define PHOENIX_IO_GMAC_2_OFFSET          0x0E000
#define PHOENIX_IO_GMAC_3_OFFSET          0x0F000

#define PHOENIX_IO_SPI4_0_OFFSET          0x10000
#define PHOENIX_IO_XGMAC_0_OFFSET         0x11000
#define PHOENIX_IO_SPI4_1_OFFSET          0x12000
#define PHOENIX_IO_XGMAC_1_OFFSET         0x13000

#define PHOENIX_IO_UART_0_OFFSET          0x14000
#define PHOENIX_IO_UART_1_OFFSET          0x15000

#define PHOENIX_IO_I2C_0_OFFSET           0x16000
#define PHOENIX_IO_I2C_1_OFFSET           0x17000

#define PHOENIX_IO_GPIO_OFFSET            0x18000

#define PHOENIX_IO_FLASH_OFFSET           0x19000

#define PHOENIX_IO_L2_OFFSET              0x1b000

#define PHOENIX_IO_PCIE_0_OFFSET          0x1E000
#define PHOENIX_IO_PCIE_1_OFFSET          0x1F000

#define PHOENIX_IO_GMAC_4_OFFSET          0x20000
#define PHOENIX_IO_GMAC_5_OFFSET          0x21000
#define PHOENIX_IO_GMAC_6_OFFSET          0x22000
#define PHOENIX_IO_GMAC_7_OFFSET          0x23000

#ifndef __ASSEMBLY__


#include "rmi/rmi_processor.h"

extern unsigned long phoenix_io_base;
struct processor;

void on_chip_init(struct processor *this_chip);

#define phoenix_io_mmio(offset) ((phoenix_reg_t *)(phoenix_io_base+(offset)))
#define phoenix_read_reg(base, offset) (be32_to_cpu((base)[(offset)]))
#define phoenix_write_reg(base, offset, value) ((base)[(offset)] = cpu_to_be32((value)))

#endif

#endif
