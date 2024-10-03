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

#ifndef __PCI_REG_H__
#define __PCI_REG_H__

#define PCI_REG_VENDOR_DEVICE        0x00
#define PCI_REG_COMMAND              0x04
#define PCI_VAL_COMMAND_IOEN                       0x01
#define PCI_VAL_COMMAND_MEMEN                      0x02
#define PCI_VAL_COMMAND_MASTEN                     0x04
#define PCI_VAL_COMMAND_MWINVEN                    0x10

#define PCI_REG_STATUS               0x06
#define PCI_VAL_STATUS_CAP_LIST	     0x10

#define PCI_REG_CLASS_REV             0x08
#define PCI_VAL_CLASS_BRIDGE_PCI                   0x0604
#define PCI_VAL_CLASS_BRIDGE_PCMCIA                0x0605

#define PCI_REG_CACHE_LINE_SIZE       0x0c
#define PCI_REG_LATENCY_TIMER         0x0d

#define PCI_REG_HDRTYPE               0x0e
#define PCI_VAL_HDRTYPE_NORMAL                     0x00
#define PCI_VAL_HDRTYPE_BRIDGE                     0x01
#define PCI_VAL_HDRTYPE_MULTIFN                    0x80

#define PCI_REG_FIRST_BAR             0x10
#define PCI_REG_BAR(x)                (PCI_REG_FIRST_BAR + (x) * 4)
#define PCI_VAL_BAR_MEM                            0x00
#define PCI_VAL_BAR_MEM_64                         0x06   
#define PCI_VAL_BAR_MEM_MASK                       (~0x0fUL)
#define PCI_VAL_BAR_IO                             0x01
#define PCI_VAL_BAR_IO_MASK                        (~0x03UL)
#define PCI_VAL_ROM_ADDR_MASK                      (~0x7ffUL)

#define PCI_REG_PRIMARY_BUS		0x18
#define PCI_REG_SECONDARY_BUS	        0x19
#define PCI_REG_SUBORDINATE_BUS	        0x1a
#define PCI_REG_IO_BASE		        0x1c
#define PCI_REG_IO_LIMIT		0x1d

#define PCI_REG_MEMORY_BASE		0x20
#define PCI_REG_MEMORY_LIMIT	        0x22

#define PCI_REG_PREF_MEM_BASE	        0x24
#define PCI_REG_PREF_MEM_LIMIT	        0x26

#define PCI_REG_IO_BASE_UPPER16	        0x30
#define PCI_REG_IO_LIMIT_UPPER16	0x32

#define PCI_REG_CAP_PTR	                0x34

/* Capability Register Offsets*/

#define PCI_VAL_CAP_ID       0x0
#define PCI_VAL_CAP_NEXTPTR  0x1

/* Capability Identification Numbers */

#define PCI_VAL_CAP_PMG        0x01    /* PCI Power Management */
#define PCI_VAL_CAP_MSI        0x05    /* Message Signaled Interrupts */
#define PCI_VAL_CAP_PCIX       0x07    /* PCI-X */
#define PCI_VAL_CAP_HT         0x08    /* HyperTransport */
#define PCI_VAL_CAP_EXPRESS    0x10    /* PCI Express */
#define PCI_VAL_CAP_MSIX       0x11    /* MSI-X */

#define PCI_REG_PM_CTRL		            4

#define PCI_REG_BRIDGE_CTRL              0x3e
#define PCI_VAL_BRIDGE_CTRL_MA                     0x20


#define DEVICE_MEM_BAR_64              (1<<4)
#define DEVICE_IO_PC_COMPATIBLE        (1<<5)
#define DEVICE_RESOURCE_IMPLEMENTED    (1<<6)

#define PCI_BAR_ALIGN(base,size)       ((( (base) - 1) | (size -1) ) + 1)



#endif
