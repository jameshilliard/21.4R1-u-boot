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

#include "mvPoe.h"
#include "hwIf/mvHwIf.h"
#include "common/macros/mvCommonDefs.h"    
#include "mvSysHwConfig.h"

/*
 * Internal defines; should not be exposed.
 */

/*
 * POE CPU Control Register:
 * PoeEn = 1
 * PoeCpuEn = 0
 * GPP0Func = 0 (GPP0 is GPP)
 * CpuInitRam = 1 (TCM is enabled at reset)
 * CpuClk1to1 = 1
 * BridgeLittle_big = 1 (Little)
 */
#define POE_CPU_CTRL_REG_INIT_VAL 0x38080869
#define POE_CPU_EN_BIT (1<<2)

#define ITCM_START (INTER_REGS_BASE + 0x80000)
#define ITCM_SIZE (_64KB)
static MV_VOID * const itcmP = (MV_VOID * const)ITCM_START;
static const MV_U32 itcmSize = ITCM_SIZE;

#define DTCM_START (ITCM_START + ITCM_SIZE)
#define DTCM_SIZE (_32KB)
static MV_VOID * const dtcmP = (MV_VOID * const)DTCM_START;
static const MV_U32 dtcmSize = DTCM_SIZE;

#define DTCM_STACK_START  DTCM_START
#define DTCM_STACK_SIZE   (_16KB)
static MV_VOID * const dtcmStackP = (MV_VOID * const)DTCM_STACK_START;
static const MV_U32 dtcmStackSize = DTCM_STACK_SIZE;

#define DTCM_SHARED_START (DTCM_STACK_START + DTCM_STACK_SIZE)
#define DTCM_SHARED_SIZE  (_8KB)
static MV_VOID * const dtcmSharedP = (MV_VOID * const)DTCM_SHARED_START;
static const MV_U32 dtcmSharedSize = DTCM_SHARED_SIZE;

#define DTCM_OTHER_START  (DTCM_SHARED_START + DTCM_SHARED_SIZE)
#define DTCM_OTHER_SIZE   (_8KB)
static MV_VOID * const dtcmOtherP = (MV_VOID * const)DTCM_OTHER_START;
static const MV_U32 dtcmOtherSize = DTCM_OTHER_SIZE;

/******************************************************************************
ToDo:

0.  Should I use in real C code SendVerilogLine()-like function?
1.  IRQ_polarity.

******************************************************************************/

MV_STATUS mvPoeInit(MV_VOID)
{
    mvSwitchWriteReg(PRESTERA_DEFAULT_DEV, 0x88, POE_CPU_CTRL_REG_INIT_VAL);
    if (memset(dtcmP, 0xFF, dtcmSize) != dtcmP)
        return MV_FAIL;
    return MV_OK;
}

MV_STATUS mvPoeSWDownload(const MV_VOID *src, MV_U32 size)
{
    if (size == 0 || size > itcmSize)
        return MV_FAIL;

    memcpy(src, itcmP, size);
    return MV_OK;
}

MV_STATUS mvPoeEnableSet(MV_BOOL enable)
{
    if (enable)
        mvSwitchWriteReg(PRESTERA_DEFAULT_DEV, 0x88,
                         POE_CPU_CTRL_REG_INIT_VAL |  POE_CPU_EN_BIT);
    else
        mvSwitchWriteReg(PRESTERA_DEFAULT_DEV, 0x88,
                         POE_CPU_CTRL_REG_INIT_VAL & ~POE_CPU_EN_BIT);
    return MV_OK;
}

MV_STATUS mvPoeSharedMemWrite(MV_U32 offset, const MV_VOID *buffer, MV_U32 length)
{
    if (length > dtcmSharedSize - offset)
        return MV_BAD_PARAM;

    memcpy(buffer, dtcmSharedP + offset, length);
    return MV_OK;
}

MV_STATUS mvPoeSharedMemRead(MV_U32 offset, MV_U32 length, MV_VOID *buffer)
{
    if (length > dtcmSharedSize - offset)
        return MV_BAD_PARAM;

    memcpy(dtcmSharedP + offset, buffer, length);
    return MV_OK;
}

MV_STATUS mvPoeSharedMemBaseAddrGet(MV_U32* sharedMemP)
{
    *sharedMemP = (MV_U32)dtcmSharedP;
    return MV_OK;
}


