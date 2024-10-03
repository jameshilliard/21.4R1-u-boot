/*
 * libfdt - Flat Device Tree manipulation (build/run environment adaptation)
 * Copyright (C) 2007 Gerald Van Baren, Custom IDEAS, vanbaren@cideas.com
 * Original version written by David Gibson, IBM Corporation.
 *
 * SPDX-License-Identifier:	LGPL-2.1+
 */

#ifndef _LIBFDT_ENV_H
#define _LIBFDT_ENV_H

#include "compiler.h"

/* Junos
 * Since this is included in building tools/ directory with USE_HOSTCC
 * defined, it gives problem to find asm/types.h
 * So this has been excluded
 */
/* #include "linux/types.h" */

extern struct fdt_header *working_fdt;  /* Pointer to the working fdt */

/* Junos
 * These are changed, since linux/types.h is not included
 * also older version of code, directly uses uint16_t, uint32_t, uint64_t
 *
typedef __be16 fdt16_t;
typedef __be32 fdt32_t;
typedef __be64 fdt64_t;
*/

/* Junos */
typedef uint16_t fdt16_t;
typedef uint32_t fdt32_t;
typedef uint64_t fdt64_t;
/* Junos */

#define fdt32_to_cpu(x)		be32_to_cpu(x)
#define cpu_to_fdt32(x)		cpu_to_be32(x)
#define fdt64_to_cpu(x)		be64_to_cpu(x)
#define cpu_to_fdt64(x)		cpu_to_be64(x)

/* adding a ramdisk needs 0x44 bytes in version 2008.10 */
#define FDT_RAMDISK_OVERHEAD	0x80

#endif /* _LIBFDT_ENV_H */
