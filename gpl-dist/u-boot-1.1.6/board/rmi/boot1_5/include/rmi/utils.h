#include "rmi/types.h"
static __inline__ int num_ones(uint32_t mask)
{
	int i = 0;
	int count = 0;

	for (i = 0; i < 32; i++)
		if (mask & (1 << i))
			count++;
	return count;
}

