#include "mvOsPrestera.h"

/*******************************************************************************
* osCacheDmaMalloc - Allocated memory for RX and TX descriptors.
*
* DESCRIPTION:
*       This function allocates memory for RX and TX descriptors.

* INPUT:
*       int size - size of memory should be allocated.
*
* RETURN: None
*
*******************************************************************************/
void* osCacheDmaMalloc(void* osHandle, MV_U32 descSize,
                       MV_U32* pPhysAddr, MV_U32 *memHandle)
{
	#ifdef PRESTERA_CACHED
	return mvOsIoCachedMalloc(osHandle, descSize, pPhysAddr,memHandle);
	#else
    	return mvOsIoUncachedMalloc(osHandle, descSize, pPhysAddr,memHandle);
	#endif
}

/*******************************************************************************
* osCacheDmaFree - Free memory for RX and TX descriptors & buffers.
*
* DESCRIPTION:
*       This function frees memory for RX and TX descriptors & buffers.
*
* INPUT:
*       
*
* RETURN: None
*
*******************************************************************************/
void osCacheDmaFree(void* osHandle, MV_U32 size,
                         MV_U32* phyAddr, void* pVirtAddr, MV_U32 *memHandle)
{
	#ifdef PRESTERA_CACHED
	mvOsIoCachedFree(osHandle,size,(MV_ULONG)phyAddr,pVirtAddr,memHandle);
	#else
    	mvOsIoUncachedFree(osHandle,size,(MV_ULONG)phyAddr,pVirtAddr,memHandle);
	#endif 
}

