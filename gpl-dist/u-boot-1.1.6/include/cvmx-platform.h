/***********************license start***************
 * Copyright (c) 2003-2008  Cavium Networks (support@cavium.com). All rights
 * reserved.
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Cavium Networks nor the names of
 *       its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 *
 * This Software, including technical data, may be subject to U.S.  export
 * control laws, including the U.S.  Export Administration Act and its
 * associated regulations, and may be subject to export or import regulations
 * in other countries.  You warrant that You will comply strictly in all
 * respects with all such regulations and acknowledge that you have the
 * responsibility to obtain licenses to export, re-export or import the
 * Software.
 *
 * TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
 * AND WITH ALL FAULTS AND CAVIUM NETWORKS MAKES NO PROMISES, REPRESENTATIONS
 * OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 * RESPECT TO THE SOFTWARE, INCLUDING ITS CONDITION, ITS CONFORMITY TO ANY
 * REPRESENTATION OR DESCRIPTION, OR THE EXISTENCE OF ANY LATENT OR PATENT
 * DEFECTS, AND CAVIUM SPECIFICALLY DISCLAIMS ALL IMPLIED (IF ANY) WARRANTIES
 * OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A PARTICULAR
 * PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET
 * POSSESSION OR CORRESPONDENCE TO DESCRIPTION.  THE ENTIRE RISK ARISING OUT
 * OF USE OR PERFORMANCE OF THE SOFTWARE LIES WITH YOU.
 *
 *
 * For any questions regarding licensing please contact marketing@caviumnetworks.com
 *
 ***********************license end**************************************/






/**
 * @file
 *
 * This file is resposible for including all system dependent
 * headers for the cvmx-* files.
 *
 * <hr>$Revision: 1.2 $<hr>
*/

#ifndef __CVMX_PLATFORM_H__
#define __CVMX_PLATFORM_H__

#include "cvmx-abi.h"

/* This file defines macros for use in determining the current
    building environment. It defines a single CVMX_BUILD_FOR_*
    macro representing the target of the build. The current
    possibilities are:
        CVMX_BUILD_FOR_UBOOT
        CVMX_BUILD_FOR_LINUX_KERNEL
        CVMX_BUILD_FOR_LINUX_USER
        CVMX_BUILD_FOR_LINUX_HOST
        CVMX_BUILD_FOR_VXWORKS
        CVMX_BUILD_FOR_STANDALONE */
#if defined(__U_BOOT__)
    /* We are being used inside of Uboot */
    #define CVMX_BUILD_FOR_UBOOT
#elif defined(__linux__)
    #if !defined(__mips__)
        /* We are being used under Linux but not on Octeon. Assume
            we are on a Linux host with an Octeon target over PCI/PCIe */
        #define CVMX_BUILD_FOR_LINUX_HOST
    #elif defined(__KERNEL__)
        /* We are in the Linux kernel on Octeon */
        #define CVMX_BUILD_FOR_LINUX_KERNEL
    #else
        #ifdef CVMX_BUILD_FOR_LINUX_HOST
            /* This is a manual special case. The host PCI utilities can
                be configured to run on Octeon. In this case it is impossible
                to tell the difference between the normal userspace setup
                and using cvmx_read/write_csr over the PCI bus. The host
                utilites force this define to fix this */
        #else
            /* We are in the Linux userspace on Octeon */
            #define CVMX_BUILD_FOR_LINUX_USER
        #endif
    #endif
#elif defined(_WRS_KERNEL) || defined(VXWORKS_USER_MAPPINGS)
    /* We are in VxWorks on Octeon */
    #define CVMX_BUILD_FOR_VXWORKS
#elif defined(_OCTEON_TOOLCHAIN_RUNTIME)
    /* To build the simple exec toolchain runtime (newlib) library. We
       should only use features available on all Octeon models.  */
    #define CVMX_BUILD_FOR_TOOLCHAIN
#else
    /* We are building a simple exec standalone image for Octeon */
    #define CVMX_BUILD_FOR_STANDALONE
#endif




#if defined(CVMX_BUILD_FOR_UBOOT)

    #include <common.h>
    #include "cvmx-sysinfo.h"

#elif defined(CVMX_BUILD_FOR_LINUX_KERNEL)

    #include <linux/kernel.h>
    #include <linux/string.h>
    #include <linux/types.h>
    #include <stdarg.h>

#elif defined(CVMX_BUILD_FOR_LINUX_USER)

    #include <stddef.h>
    #include <stdint.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdarg.h>
    #include <string.h>
    #include <assert.h>
    #include <fcntl.h>
    #include <sys/mman.h>
    #include <unistd.h>

#elif defined(CVMX_BUILD_FOR_LINUX_HOST)

    #include <stddef.h>
    #include <stdint.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdarg.h>
    #include <string.h>
    #include <assert.h>
    #include <fcntl.h>
    #include <sys/mman.h>
    #include <unistd.h>

#elif defined(CVMX_BUILD_FOR_VXWORKS)

    #include <stdint.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdarg.h>
    #include <string.h>
    #include <assert.h>

#elif defined(CVMX_BUILD_FOR_STANDALONE)

    #include <stddef.h>
    #include <stdint.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdarg.h>
    #include <string.h>
    #include <assert.h>

#elif defined(CVMX_BUILD_FOR_TOOLCHAIN)

    #include <stddef.h>
    #include <stdint.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdarg.h>
    #include <string.h>
    #include <assert.h>

#else

    #error Unexpected CVMX_BUILD_FOR_* macro

#endif

#endif /* __CVMX_PLATFORM_H__ */
