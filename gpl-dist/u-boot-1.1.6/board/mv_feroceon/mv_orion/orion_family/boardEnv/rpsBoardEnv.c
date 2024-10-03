/*
 * Copyright (c) 2006-2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
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

#define RPS_BOARD_PCI_IF_NUM		0x0
#define RPS_BOARD_TWSI_DEF_NUM	0x0
#define RPS_BOARD_MAC_INFO_NUM	0x1
#define RPS_BOARD_GPP_INFO_NUM	0x1
#define RPS_BOARD_DEBUG_LED_NUM	0x0
#define RPS_BOARD_MPP_CONFIG_NUM	0x1
#define RPS_BOARD_DEVICE_CONFIG_NUM	0x1

MV_BOARD_MAC_INFO rpsInfoBoardMacInfo[RPS_BOARD_MAC_INFO_NUM] = 
	/* {{MV_BOARD_MAC_SPEED	boardMacSpeed, MV_U8 boardEthSmiAddr}} */
	{{BOARD_MAC_SPEED_AUTO, 0x8},
	}; 

MV_BOARD_GPP_INFO	rpsInfoBoardGppInfo[RPS_BOARD_GPP_INFO_NUM] = 
	/* {{MV_BOARD_DEV_CLASS	devClass, MV_U8	gppPinNum}} */
	{{BOARD_DEV_RESET, 7},
	};

MV_BOARD_MPP_INFO	rpsInfoBoardMppConfigValue[RPS_BOARD_MPP_CONFIG_NUM] = 
	{{{0x11110300,			/* mpp0_7 */
	0x11111111,				/* mpp8_15 */
	N_A,						/* mpp16_23 */
	N_A}},						/* mppDev */						
	};

MV_DEV_CS_INFO rpsInfoBoardDeCsInfo[RPS_BOARD_DEVICE_CONFIG_NUM] =
		/*{params, devType, devWidth}*/			   
		{{ 0, 0x8fcfffff, BOARD_DEV_NAND_FLASH, 8}}; /* NAND DEV */

MV_BOARD_INFO rpsInfo = {
	"RPS-200",				/* boardName[MAX_BOARD_NAME_LEN] */
	RPS_BOARD_MPP_CONFIG_NUM,		/* numBoardMppConfig */
	rpsInfoBoardMppConfigValue,
	0,						/* intsGppMask */
	RPS_BOARD_DEVICE_CONFIG_NUM,		/* numBoardDevIf */
	rpsInfoBoardDeCsInfo,
	RPS_BOARD_PCI_IF_NUM,			/* numBoardPciIf */
	NULL,
	RPS_BOARD_TWSI_DEF_NUM,			/* numBoardTwsiDev */
	NULL,					
	RPS_BOARD_MAC_INFO_NUM,			/* numBoardMacInfo */
	rpsInfoBoardMacInfo,
	RPS_BOARD_GPP_INFO_NUM,			/* numBoardGppInfo */
	rpsInfoBoardGppInfo,
	RPS_BOARD_DEBUG_LED_NUM,			/* activeLedsNumber */              
	NULL,
	N_A,						/* ledsPolarity */		
	RPS_GPP_OE,				/* gppOutEnVal */
	RPS_GPP_VAL,				/* gppOutVal */
	0x1						/* gppPolarityVal */
};


MV_BOARD_INFO*	boardInfoTbl[]	=	{&rpsInfo};

#define	BOARD_ID_BASE			DB_RPS_ID
#define MV_MAX_BOARD_ID			BOARD_ID_CUSTOMER_MAX
