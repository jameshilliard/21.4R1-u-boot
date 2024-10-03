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

/* Global Board-level configuration file */

#ifndef __BOARDCONFIG_H_
#define __BOARDCONFIG_H_

#include <config.h>
   /*
    * If this is not an RMI Eval board, we treat it as a template board
    * which gets rid of a lot of RMI board dependent code
    */  
#ifndef RMI_EVAL_BOARD
#define CFG_BOARD_TEMPLATE
#define CFG_TEMPLATE_USER
#endif
#ifdef CFG_BOARD_TEMPLATE
    /* Select userdefined Vs. minimal
     * config for Template Board
     */
    #ifndef CFG_TEMPLATE_USER
    #define CFG_TEMPLATE_MINIMAL          
    #endif
#else
    #define CFG_BOARD_RMIREF
#endif

#ifdef CFG_BOARD_TEMPLATE           
    /* Defines for stages prior to boot2 */
        /* I2C bus [0 or 1] to use for SPD.
         * I2C slave DIMM addresses assumed to
         * be 0x54/0x55/0x56/0x57 currently
         */
    #define CFG_TEMPLATE_I2CBUS_SPD         0
    /*
     * No Temp Sensor on i2c 1 for FX boards
     */ 
#ifndef CONFIG_FX    
    #define CFG_TEMPLATE_I2C_TEMPSENSOR
#endif    
    #ifdef  CFG_TEMPLATE_I2C_TEMPSENSOR
        #define CFG_TEMPLATE_I2CADDR_TEMPSENSOR 0x4C
        #define CFG_TEMPLATE_I2CBUS_TEMPSENSOR  1
        #define CFG_TEMPLATE_EXTTEMP_OFFSET     1
    #endif
    #define CFG_TEMPLATE_I2C_EEPROM                  
    #ifdef  CFG_TEMPLATE_I2C_EEPROM
        #define CFG_TEMPLATE_SIZE_EEPROM    32
        #define CFG_TEMPLATE_I2CADDR_EEPROM 0x51
        #define CFG_TEMPLATE_I2CBUS_EEPROM  0
        #define CFG_TEMPLATE_MACADDR_OFFSET 0x20
    #endif
    /* User defined template-board features
     * for reference in porting
     */
    #ifdef CFG_TEMPLATE_USER
        /* Defines for the boot2 stage */
        #define CFG_TEMPLATE_GMAC0
        #define CFG_TEMPLATE_PCIX
        #define CFG_TEMPLATE_PIO
    #endif
    #ifdef CFG_TEMPLATE_GMAC0
        /* GMAC0 Port Number */
        #define CFG_TEMPLATE_GMAC0_PORTID   0x00
        /* GMAC0 Board PHY ID */
        #define CFG_TEMPLATE_GMAC0_PHYID    0x00
    #endif

#endif  /* CFG_BOARD_TEMPLATE */

#endif  /* __BOARDCONFIG_H_   */
