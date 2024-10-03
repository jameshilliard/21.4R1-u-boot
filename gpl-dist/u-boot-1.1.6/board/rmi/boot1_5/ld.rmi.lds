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
ENTRY(reset)
OUTPUT_FORMAT("elf32-bigmips")
/*OUTPUT_FORMAT("elf32-tradbigmips")*/

SECTIONS
{	
  .reset 0xbfc00000 : AT (0x1fc00000) {
	reset.o(.exception.reset)
  }

  .exception.200 0xbfc00200 : AT (0x1fc00200) {
	traps.o(.exception.200)
  }

  .exception.280 0xbfc00280 : AT (0x1fc00280) {
	traps.o(.exception.280)
  }

  .exception.300 0xbfc00300 : AT (0x1fc00300) {
	traps.o(.exception.300)
  }

  .exception.380 0xbfc00380 : AT (0x1fc00380) {
	traps.o(.exception.380)
  }


  .nand 0x9fc01000 : AT (0x1fc01000)	{
	  _ntext = .;
	  nand_main.o(.text)
	  nand_ecc.o(.text)
          micron_nand.o(.text)
	  nand_uart.o(.text)
	  _netext = .;
  } = 0x0
	  
  .text : AT (0x1fc02000)	{
  	_ftext = . ;
	*(.text)
  	. = ALIGN(32) ;
  } = 0x0
  _etext = .;


  .rodata : AT (0x1fc02000 + SIZEOF(.text)) {
    *(.rdata)
    *(.rodata)
    *(.rodata.*)
    *(.gnu.linkonce.r*)
    . = ALIGN(32768);
  } = 0x0
  _erodata = .;



  .data : AT (0x1fc02000 + SIZEOF(.text) + SIZEOF(.rodata)) 
  {
    _fdata = .;
    *(.data)
  } = 0x0

  _edata  =  .;

  .bss : AT (0x1fc02000 + SIZEOF(.text) + SIZEOF(.rodata) + SIZEOF(.data))
  {
	  . = ALIGN(8);
  _fbss = .;
  __bss_start = .;
   *(.sbss)
   *(.bss)
   *(.scommon)
  } = 0x0
  _bss_end = . ;

  .reginfo : AT (0x1fc02000 + SIZEOF(.text) + SIZEOF(.rodata) + SIZEOF(.data) + SIZEOF(.bss))
  {
  }

  . = ALIGN(8);
  _aligned_stack = .        ;
  . = . + 8192 ;
  __stack = . - 64;

  _end = .			; 

}
