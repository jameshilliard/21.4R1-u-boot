/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell 
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or 
modify this File in accordance with the terms and conditions of the General 
Public License Version 2, June 1991 (the "GPL License"), a copy of which is 
available along with the File in the license.txt file or by writing to the Free 
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or 
on the worldwide web at http://www.gnu.org/licenses/gpl.txt. 

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED 
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY 
DISCLAIMED.  The GPL License provides additional details about this warranty 
disclaimer.
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or 
modify this File under the following licensing terms. 
Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer. 

    *   Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution. 

    *   Neither the name of Marvell nor the names of its contributors may be 
        used to endorse or promote products derived from this software without 
        specific prior written permission. 
    
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#ifndef __mvGenTypesh
#define __mvGenTypesh

/*#include "common/mvEnvDep.h"*/
#include "mvTypes.h"

#ifdef _VXWORKS
#define MV_INLINE __inline__
#else
#define  MV_INLINE
#endif


#ifndef NULL
#define NULL ((void*)0)
#endif

#undef IN
#define IN
#undef OUT
#define OUT
#undef INOUT
#define INOUT

#if 0
typedef void          (*MV_VOIDFUNCPTR) (void); /* ptr to function returning void */
typedef unsigned int  (*MV_INTFUNCPTR)  (void); /* ptr to function returning int  */
#endif

typedef void* MV_PTR;

/*
 * Typedef: enum MV_COMP_RES
 *
 * Description: Values to be returned by compare function
 *
 */
typedef enum
{
    MV_EQUAL   = 0,
    MV_GREATER = 1,
    MV_SMALLER = 2
}MV_COMP_RES;

#if 0
typedef unsigned int MV_STATUS;

/***** generic return codes **********************************/
#define MV_ERROR           (0xFFFFFFFF)
#endif
#define MV_OK              (0x00)   /* Operation succeeded */
#define MV_FAIL            (0x01)   /* Operation failed    */

#define MV_BAD_VALUE       (0x02)   /* Illegal value        */
#define MV_OUT_OF_RANGE    (0x03)   /* Value is out of range*/
#define MV_BAD_PARAM       (0x04)   /* Illegal parameter in function called  */
#define MV_BAD_PTR         (0x05)   /* Illegal pointer value                 */
#define MV_BAD_SIZE        (0x06)   /* Illegal size                          */
#define MV_BAD_STATE       (0x07)   /* Illegal state of state machine        */
#define MV_SET_ERROR       (0x08)   /* Set operation failed                  */
#define MV_GET_ERROR       (0x09)   /* Get operation failed                  */
#define MV_CREATE_ERROR    (0x0A)   /* Fail while creating an item           */
#define MV_NOT_FOUND       (0x0B)   /* Item not found                        */
#define MV_NO_MORE         (0x0C)   /* No more items found                   */
#define MV_NO_SUCH         (0x0D)   /* No such item                          */
#define MV_TIMEOUT         (0x0E)   /* Time Out                              */
#define MV_NO_CHANGE       (0x0F)   /* The parameter is already in this value*/
#define MV_NOT_SUPPORTED   (0x10)   /* This request is not support           */
#define MV_NOT_IMPLEMENTED (0x11)   /* This request is not implemented       */
#define MV_NOT_INITIALIZED (0x12)   /* The item is not initialized           */
#define MV_NO_RESOURCE     (0x13)   /* Resource not available (memory ...)   */
#define MV_FULL            (0x14)   /* Item is full (Queue or table etc...)  */
#define MV_EMPTY           (0x15)   /* Item is empty (Queue or table etc...) */
#define MV_INIT_ERROR      (0x16)   /* Error occured while INIT process      */
#define MV_NOT_READY       (0x1A)   /* The other side is not ready yet       */
#define MV_ALREADY_EXIST   (0x1B)   /* Tried to create existing item         */
#define MV_OUT_OF_CPU_MEM  (0x1C)   /* Cpu memory allocation failed.         */
#define MV_ABORTED         (0x1D)   /* Operation has been aborted.           */
#define MV_MAX_STATUS      (0x1E)   /* Maximum number of error including 
                                       MV_ERROR                              */

#endif   /* __mvGenTypesh */


