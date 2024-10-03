# $Id: Makefile.inc 824648 2016-12-29 07:00:11Z sjg $
# Copyright (c) 2011-2016, Juniper Networks, Inc.
# All rights reserved.
#
# UBOOT build rules

UBOOT_SRCTOP = ${SRCTOP}/gpl-dist/u-boot
UBOOT_OBJTOP = ${OBJTOP_BSD}/gnu/sys/u-boot/${.TARGET:E}

UBOOT_MAKEFLAGS = \
	"CROSS_COMPILE=${FWTOOLS_PREFIX}/${TOOLCHAIN_${MACHINE_ARCH}}/bin/${CROSS_TARGET_PREFIX}" \
	"HOSTCC=${_HOST_CC}" \
	"BISON=${BISON}"

# ensure we only pass on MAKEFLAGS that mean the same to gmake as bmake
UBOOT_MAKE = MAKEFLAGS="${MAKEFLAGS:M-[kenirqst]}" \
	${GMAKE} -j1 ${UBOOT_MAKEFLAGS}

_BUILD_UBOOT_USE: .USE .META
	if [ -f ${UBOOT_SRCTOP}/Makefile ] ; then \
	    ${UBOOT_MAKE} -C ${UBOOT_SRCTOP} O=${UBOOT_OBJTOP} ${.TARGET:E}_config; \
	    ${UBOOT_MAKE} -C ${UBOOT_SRCTOP} O=${UBOOT_OBJTOP} INSTNAME=${UBOOT_OBJTOP}/${.TARGET:E}-uboot.bin all; \
	fi


_BUILD_UBOOT_45XX_USE: .USE .META
	if [ -f ${UBOOT_SRCTOP}/Makefile ] ; then \
	    ${UBOOT_MAKE} -C ${UBOOT_SRCTOP} O=${UBOOT_OBJTOP}/${.TARGET:E}_readonly ${.TARGET:E}_readonly_config; \
	    ${UBOOT_MAKE} -C ${UBOOT_SRCTOP} O=${UBOOT_OBJTOP}/${.TARGET:E}_readonly INSTNAME=${UBOOT_OBJTOP}/${.TARGET:E}-ro-uboot.bin all; \
	    ${UBOOT_MAKE} -C ${UBOOT_SRCTOP} O=${UBOOT_OBJTOP}/${.TARGET:E}_upgrade ${.TARGET:E}_upgrade_config; \
	    ${UBOOT_MAKE} -C ${UBOOT_SRCTOP} O=${UBOOT_OBJTOP}/${.TARGET:E}_upgrade INSTNAME=${UBOOT_OBJTOP}/${.TARGET:E}-upg-uboot.bin all; \
	fi
