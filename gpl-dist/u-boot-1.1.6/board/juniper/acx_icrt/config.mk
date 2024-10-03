#
# Copyright (c) 2011-2014, Juniper Networks, Inc. 
# All rights reserved.
#
# Copyright (C) 2007-2008 Freescale Semiconductor, Inc. All rights reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see
# <http://www.gnu.org/licenses/gpl-2.0.html>.


#
# iCRT for acx board
#

sinclude $(OBJTREE)/board/$(BOARDDIR)/config.tmp

TEXT_BASE = 0xfff80000

PLATFORM_CPPFLAGS += -DCONFIG_E500=1
PLATFORM_CPPFLAGS += -DCONFIG_MPC85xx=1
PLATFORM_CPPFLAGS += -Werror
