#
# Copyright (c) 2009-2010, Juniper Networks, Inc.
# All rights reserved.
#
# Copyright (C) 2007-2008 Freescale Semiconductor, Inc. All rights reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#

#
# mx80 board
#

sinclude $(OBJTREE)/board/$(BOARDDIR)/config.tmp

TEXT_BASE = 0xfff80000

PLATFORM_CPPFLAGS += -DCONFIG_E500=1
PLATFORM_CPPFLAGS += -DCONFIG_MPC85xx=1
PLATFORM_CPPFLAGS += -DCONFIG_MPC8572=1
