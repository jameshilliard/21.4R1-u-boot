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
#ifndef __COMPONENTS_H__
#define __COMPONENTS_H__
#include "rmi/types.h"
/* 
   The Peripheral and I/O space in a CPU is specified by the architecture.
   It can vary from CPU to CPU. For e.g., the gmac0 adress space in Phoenix
   CPU starts at offset 0x0_C000. If it is defined as below:

   #define GMAC0_OFFSET 0x0c0000 

   then, in future, for another version of the chip, if the offset changes,
   the code will break.
   Instead, the offset is specified through a structure, each version of
   CPU can inherit the structure and initialize the offset at cpu initialization
   time.

   This structure has to be super set of all the components that present in all
   the supported CPUs.
*/
struct component_base
{
	uint32_t bridge;

	uint32_t ddr2_chn0;
	uint32_t ddr2_chn1;
	uint32_t ddr2_chn2;
	uint32_t ddr2_chn3;

	uint32_t rld2_chn0;
	uint32_t rld2_chn1;

	uint32_t sram;

	uint32_t pic;

	uint32_t pcix;

	uint32_t ht;

	uint32_t security;

	uint32_t gmac0;
	uint32_t gmac1;
	uint32_t gmac2;
	uint32_t gmac3;

	uint32_t gmac4;
	uint32_t gmac5;
	uint32_t gmac6;
	uint32_t gmac7;

	uint32_t pcie_0;
	uint32_t pcie_1;

	uint32_t usb;

	uint32_t cmp;


	uint32_t spi4_0;
	uint32_t xgmac_0;
	uint32_t spi4_1;
	uint32_t xgmac_1;

	uint32_t uart_0;
	uint32_t uart_1;

	uint32_t i2c_0;
	uint32_t i2c_1;

	uint32_t gpio;

	uint32_t piobus;

	uint32_t dma;
	
	uint32_t l2;

	uint32_t trace_buffer;
};

#endif
