#ifndef __INC_CACHE_LIB_TEMP_H
#define __INC_CACHE_LIB_TEMP_H


#define mvOsCacheLineFlushInv(handle,addr)								\
{                                                               \
  _WRS_ASM ("mcr p15, 0, %0, c7, c14, 1" : : "r" (addr));\
  _WRS_ASM ("mcr p15, 1, %0, c15, c10, 1" : : "r" (addr));\
  _WRS_ASM ("mcr p15, 0, %0, c7, c10, 4" : : "r" (addr));\
}

#define mvOsCacheLineInv(handle,addr)									\
{                                                               \
  _WRS_ASM ("mcr p15, 0, %0, c7, c6, 1" : : "r" (addr)); \
  _WRS_ASM ("mcr p15, 1, %0, c15, c11, 1" : : "r" (addr)); \
}

#endif /* __INC_CACHE_LIB_TEMP_H */
