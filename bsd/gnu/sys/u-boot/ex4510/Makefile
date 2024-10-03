# $Id: Makefile 600834 2013-08-30 16:39:06Z ib-builder $
#
# Copyright (c) 2012-2013, Juniper Networks, Inc.
# All rights reserved.
#

UPDATE_DEPENDFILE = NO

all: build_uboot.ex4510

build_uboot.ex4510: _BUILD_UBOOT_45XX_USE

_BUILD_UBOOT_45XX_USE: .META
XMAKE_META_FILE = build_uboot.ex4510.meta

.include "../Makefile.inc"

