

#ifndef __OCTEON_HAL_H__
#define __OCTEON_HAL_H__

#include "octeon_csr.h"

/* Provide alias for __octeon_is_model_runtime__ */
#define octeon_is_model(x)     __octeon_is_model_runtime__(x)

#define OCTEON_PCI_IOSPACE_BASE     0x80011a0400000000ull

static inline int cvmx_octeon_fuse_locked(void)
{
    if (!octeon_is_model(OCTEON_CN38XX_PASS1))
        return(cvmx_fuse_read(123));
    else
        return(0);
}

#endif
