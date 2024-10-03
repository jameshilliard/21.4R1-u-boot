/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

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

*******************************************************************************/


#include <common.h>
#include <command.h>
#include <net.h>
#include <malloc.h>

#if defined (MV_PRESTERA_SWITCH)
#include "mvOs.h"
#include "hwIf/mvHwIf.h"
#include "util/mvUtils.h"
#include "util/mvNetTransmit.h"
#include "util/mvNetReceive.h"
#include "vlan/mvVlan.h"
#include "sdma/mvBridge.h"
#include "common/macros/mvCommonDefs.h"
#include "common/mvSysConf.h"
#include "mvPresteraRegs.h"

#if 0 
#define MV_DEBUG 
#endif
#ifdef MV_DEBUG
#define DB(x) x
#else
#define DB(x)
#endif

/****************************************************** 
 * driver internal definitions --                     *
 ******************************************************/ 

#define SWITCH_MTU         1530

/* rings length */
#define PRESTERA_TXQ_LEN   20
#define PRESTERA_RXQ_LEN   20


#define PRESTERA_Q_NUM     8

typedef struct _switchPriv
{
	int port;
	MV_VOID *halPriv;
	MV_U32 rxqCount;
	MV_U32 txqCount;
	MV_BOOL devInit;
} switchPriv;

MV_U8*	txPacketData;     /* TX buffer virtual  addr ptr */
MV_U8	rxPacketData[SWITCH_MTU];

/****************************************************** 
 * functions prototype --                             *
 ******************************************************/
static int mvSwitchLoad( int port, char *name, char *enet_addr );

static int mvSwitchInit( struct eth_device *dev, bd_t *p );
static int mvSwitchHalt( struct eth_device *dev );
static int mvSwitchTx( struct eth_device *dev, volatile MV_VOID *packet, int len );
static int mvSwitchRx( struct eth_device *dev );

MV_BOOL mvSwitchCheckPortStatus(int devNum, int portNum);

MV_STATUS mvPresteraHalInit(void)
{


}


/*********************************************************** 
 * mv_prestera_initialize --                               *
 *   main driver initialization. loading the interfaces.   *
 ***********************************************************/
int mv_prestera_initialize( bd_t *bis ) 
{

	int dev,port;
	MV_8 name[NAMESIZE+1];
	MV_8 *enet_addr;
	
	#if defined(MV_INC_DOUBLE_RESET_WA) && defined(DB_98DX2122)
	for(dev=0; dev < mvSwitchGetDevicesNum(); dev++)
	{
		/* Reset and then set bit 29 of (switch) FTDLL configuration register */
		mvSwitchReadModWriteReg(dev, 0x0c, BIT29,0x0);
		mvSwitchReadModWriteReg(dev, 0x0c, BIT29,BIT29);
	}

	mvOsDelay(100 /* m-sec */);
	#endif	

	/* SDMA - disable retransmit on resource error */
	mvSwitchReadModWriteReg(0, 0x2800, 0xff00, 0xff00);

	/* Open Window to xBar */
	mvSwitchWriteReg(0, 0x30C, 0xE00);          /* Define window to DRAM
						       Target ID 0, attribute 0xE */
	mvSwitchWriteReg(0, 0x310, 0xFFFF0000);     /* Set window size */
	mvSwitchReadModWriteReg(0, 0x34C, 0x1,0x0); /* Enable window 0 */

	if(mvSwitchGetDevicesNum() > 1)
	{
		/* Open PEX Window for second device */	
		mvSwitchWriteReg(1, 0x30C, 0x80F);  	    /* Define window to PEX
						       	       Target ID 0xF, attribute 0x80 */
		mvSwitchWriteReg(1, 0x310, 0xFFFF0000);     /* Set window size */
		mvSwitchReadModWriteReg(1, 0x34C, 0x1,0x0); /* Enable window 0 */
	}

	for(dev=0; dev < mvSwitchGetDevicesNum(); dev++)
	{
		if(setCpuAsVLANMember(dev, 1 /*VLAN Num*/)!=MV_OK)
		{
			printf( "\nError: (Prestera) Unable to set CPU as VLAN member (for device %d)\n",dev);
			return -1;
		}
		
		#if defined(PRESTERA_CASCADE_STACK_PORTS)
		MV_U32 mask = dev * (BIT_24|BIT_25) | (1 - dev) * (BIT_26|BIT_27);
		mvSwitchReadModWriteReg(dev, CASCADE_AND_HEADER_CONFIG_REG, mask,mask);

		/* Enable the device if not enabled already */
		mvSwitchReadModWriteReg(dev, 0x58, BIT_0, BIT_0);	
		#endif
	}

	/* load interface(s) */
  	for( port=0; port < mvSwitchGetPortsNum(); port++ )
	{
		/* interface name */
		sprintf( name, "port%d", port );
		/* interface MAC addr extract */
		enet_addr = getenv( "ethaddr" );

		if(setCPUAddressInMACTAble(PORT_TO_DEV(port)/* devNum */,(MV_U8*)enet_addr/* macAddr */,1/* vid */)!=MV_OK)
		{
			printf( "\nError: (Prestera) Unable to teach CPU MAC address\n");
			return -1;
		}
		
		mvSwitchLoad( port, name, enet_addr );
	} 
	
	txPacketData = malloc(SWITCH_MTU);
	if( !txPacketData ) {
		DB( printf( "%s: %s falied to alloc TX buffer (error)\n", __FUNCTION__, name ) );
		return 1;
	}

	return 0;
}


/*********************************************************** 
 * mvSwitchLoad --                                          *
 *   load a network interface into uboot network core.     *
 *   initialize sw structures e.g. private, rings, etc.    *
 ***********************************************************/
static int mvSwitchLoad( int port, char *name, char *enet_addr ) 
{
	struct eth_device *dev = NULL;
	switchPriv *priv = NULL;

	DB( printf( "%s: %s load - ", __FUNCTION__, name ) );

	dev = malloc( sizeof(struct eth_device) );
	priv = malloc( sizeof(switchPriv) );

	if( !dev ) {
		DB( printf( "%s: %s falied to alloc eth_device (error)\n", __FUNCTION__, name ) );
		goto error;
	}

	if( !priv ) {
		DB( printf( "%s: %s falied to alloc egiga_priv (error)\n", __FUNCTION__, name ) );
		goto error;
	}

	memset( priv, 0, sizeof(switchPriv) );

	/* init device methods */
	memcpy( dev->name, name, NAMESIZE );
	/* Copy MAC address */
	createMacAddr((MV_8*)dev->enetaddr, enet_addr);

	dev->init = (void *)mvSwitchInit;
	dev->halt = (void *)mvSwitchHalt;
	dev->send = (void *)mvSwitchTx;
	dev->recv = (void *)mvSwitchRx;
	dev->priv = priv;
	dev->iobase = 0;
	priv->port = port;

	/* register the interface */
	eth_register(dev);


	DB( printf( "%s: %s load ok\n", __FUNCTION__, name ) );
	return 0;

	error:
	printf( "%s: %s load failed\n", __FUNCTION__, name );
	if( priv ) free( dev->priv );
	if( dev ) free( dev );
	return -1;
}


unsigned int switch_init[SYS_CONF_MAX_DEV]={0};
unsigned int entryId=0;

static int mvSwitchInit( struct eth_device *dev, bd_t *p )
{
	switchPriv *priv = dev->priv;
	MV_U32 portNum = PORT_TO_DEV_PORT(priv->port);
	MV_U8  devNum =  PORT_TO_DEV(priv->port);

	DB( printf( "%s: %s init - ", __FUNCTION__, dev->name ) );

	/* Check Link */
	if( mvSwitchCheckPortStatus(devNum, portNum) == MV_FALSE ) {
		printf( "%s no link\n", dev->name );
		return 0;
	}
	else DB( printf( "link up\n" ) );

	if (switch_init[devNum]==0)
	{
		/* init the hal -- create internal port control structure and descriptor rings */
		if(mvInitSdmaNetIfDev(devNum,20,
				      PRESTERA_Q_NUM*PRESTERA_RXQ_LEN,
				      PRESTERA_Q_NUM*PRESTERA_TXQ_LEN,
				      SWITCH_MTU)!=MV_OK)
		{
			printf( "Error: (Prestera) Unable to initialize SDMA\n");
			goto error;
		}	

		priv->devInit = MV_TRUE;
		switch_init[devNum] = 1;
	}

	DB( printf( "%s: %s complete ok\n", __FUNCTION__, dev->name ) );
	return 1;

	error:
	if( priv->devInit )
		mvSwitchHalt( dev );
	printf( "%s: %s failed\n", __FUNCTION__, dev->name );
	return 0;
}

static int mvSwitchHalt( struct eth_device *dev )
{

	switchPriv *priv = dev->priv;

	DB( printf( "%s: %s halt - ", __FUNCTION__, dev->name ) );

	if( priv->devInit == MV_TRUE ) {
		priv->devInit = MV_FALSE;
	}

	/* switch_init[PORT_TO_DEV(priv->port)] = 0; */

	DB( printf( "%s: %s complete\n", __FUNCTION__, dev->name ) );
	return 0;
}

int mvSwitchTx( struct eth_device *dev, volatile void *buf, int len )
{
	switchPriv 	*priv = dev->priv;
	MV_PKT_DESC 	packetDesc;
	MV_STATUS 	status;
	MV_U8  devNum =  PORT_TO_DEV(priv->port);
	MV_U32 portNum = PORT_TO_DEV_PORT(priv->port);

	/* if no link exist */
	if(!switch_init[devNum]) return 0;

	/* prepare buffer */
	memset( txPacketData, 0, SWITCH_MTU );
	memset( &packetDesc, 0, sizeof(MV_PKT_DESC) );
	packetDesc.pcktData[0] = txPacketData;	    /* Packet's buffer virtual address */
        packetDesc.pcktPhyData[0] = txPacketData;   /* Packet's buffer physical address (Virtual==Physical)*/
	
	/* build the packet */
	status = mvSwitchBuildPacket(devNum,	/* device num */
				     portNum,   /* port num */
				     entryId,	/* entryId */
		  		     1, 	/* appendCrc */
                                     1, 	/* pcktsNum */
                                     0, 	/* gap */
			            (MV_U8*)buf,/* pcktData */
				    len, 	/* pcktSize*/
				    &packetDesc);

	if( status != MV_OK ) {
		if( status == MV_NO_RESOURCE ) 
			 DB( printf( "can't build packet. out of memory (error)\n" ) ) ;
		else 
			 DB( printf( "unrecognize status (error) mvSwitchBuildPacket\n" ) );
		goto error;
	} 
	else {
		DB( printf( "packet build ok\n" ) );
	}

	/* send the packet */
	if(mvSwitchTxStart(&packetDesc)!=MV_OK)
	{
		DB( printf( "Unable to Start Tx\n"));
		return MV_FAIL;

	}

	priv->txqCount++;

	DB( printf( "%s: %s complete ok\n", __FUNCTION__, dev->name ) );

	return 0;

	error:
	DB( printf( "%s: %s failed\n", __FUNCTION__, dev->name ) );
	return 1;
}


static int mvSwitchRx( struct eth_device *dev )
{
	switchPriv*  	priv = dev->priv;
	MV_STATUS   	status = MV_PRESTERA_REDO;
	MV_PKT_INFO 	pktInfo;
	MV_BUF_INFO 	bufInfo;
	MV_U8		queueIdx = 0;
	MV_U8		devNum = PORT_TO_DEV(priv->port);


	/* if no link exist */
	if(!switch_init[devNum]) return 0;

	//DB( printf( "%s: %s \n", __FUNCTION__, dev->name ) );

	memset( &pktInfo, 0, sizeof(MV_PKT_INFO) );

	pktInfo.pFrags = &bufInfo; 

	while(status == MV_PRESTERA_REDO)
	{
		memset( &bufInfo, 0, sizeof(MV_BUF_INFO) );

		status = mvSwitchRxStart(devNum, queueIdx, &pktInfo);
		//DB( printf( "good rx\n" ) );
		priv->rxqCount--;
		
		if( status==MV_OK){
  			/* no more rx packets ready */
			//DB( printf( "no more work\n" ) );
			return MV_OK;
		}		
		else if(status!=MV_PRESTERA_REDO) {
			DB( printf( "Rx error\n") );
			goto error;
		}
		else {
			/* good rx - push the packet up (skip on two first empty bytes) */
			
			DB( printf( "%s: %s calling NetRecieve ", __FUNCTION__, dev->name) );
			DB( printf( "%s: calling NetRecieve pkInfo = 0x%x\n", __FUNCTION__, pktInfo.pFrags->bufPhysAddr));
			DB( printf( "%s: calling NetRecieve pktSize = 0x%x\n", __FUNCTION__, pktInfo.pFrags->bufSize) );

			/* Now we remove 8 bytes of DSA Tag */
			/* first we copy MAC SA & DA */
			memcpy( rxPacketData, (void*)pktInfo.pFrags->bufVirtPtr , 12 );
			/* Set rest of packet (without 8 bytes of DSA tag) */
			memcpy( rxPacketData+12, (void*)pktInfo.pFrags->bufVirtPtr + 20 , pktInfo.pFrags->bufSize-16 );

			NetReceive(rxPacketData, 
				   pktInfo.pFrags->bufSize-8);

			mvFreeRxBuf(&pktInfo.pFrags->bufVirtPtr,1,devNum,queueIdx);
		}

	}

	DB( printf( "%s: %s complete ok\n", __FUNCTION__, dev->name ) );
	return 0;

	error:
	DB( printf( "%s: %s failed\n", __FUNCTION__, dev->name ) );
	return 1;
}

MV_BOOL mvSwitchCheckPortStatus(int devNum, int portNum)
{
	unsigned long regVal = 0;
	if( mvSwitchReadReg(devNum, PRESTERA_PORT_STATUS_REG + (portNum * 0x400), 
		(MV_U32*)&regVal) != MV_OK)
	{
		return MV_FALSE;
	}
	return (regVal & 0x1) ? MV_TRUE : MV_FALSE;
}


#endif /* #if defined (MV_PRESTERA_SWITCH) */
