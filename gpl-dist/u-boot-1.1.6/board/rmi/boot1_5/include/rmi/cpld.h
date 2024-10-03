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
#ifndef __CPLD_H__
#define __CPLD_H__

enum cpld_regs
{
	CPLD_REV_GPR_REG            = 0,
	CPLD_FLASH_CPUID_REG        = 1,
	CPLD_RESET_REG              = 2,
	CPLD_RESET_REASON_REG       = 3,
	CPLD_CORE_VOLTAGE_CTRL_REG  = 4,
	CPLD_LED_REG                = 5,
	CPLD_INT_STATUS_REG         = 6,
	CPLD_MISC_STATUS_REG_XLS    = 6,
	CPLD_MISC_STATUS_REG        = 7,
	CPLD_MISC_CTRL_REG_XLS	    = 7,
	CPLD_MISC_CTRL_REG          = 8,
	CPLD_INT_STATUS_REG_XLS	    = 8
};

void rmi_cpld_init(void);
void cpld_enable_flash_write(unsigned char en);
void cpld_enable_nand_write(unsigned char en);
void cpld_set_pcmcia_mode(unsigned char io);


#endif
