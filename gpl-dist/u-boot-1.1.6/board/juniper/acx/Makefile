#
# Rajat Jain, Feb 2011
#
# Copyright (c) 2011-2012, Juniper Networks, Inc. 
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

VPATH	= $(TOPDIR)

include $(TOPDIR)/config.mk

LIB	= $(obj)lib$(BOARD).a

COBJS	:= 	$(BOARD).o \
		fixed_sdram.o fsl_i2c.o tsec.o i2c_cdev.o \
		cmd_acx.o syspld.o acx500_secureboot.o

SOBJS	:= init.o

SRCS	:= $(SOBJS:.o=.S) $(COBJS:.o=.c)

COBJS	+= secure_measured_boot.o
SRCS	+= $(obj)secure_measured_boot.c

OBJS	:= $(addprefix $(obj),$(COBJS))
SOBJS	:= $(addprefix $(obj),$(SOBJS))

$(LIB):	$(obj).depend $(OBJS) $(SOBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

clean:
	rm -f $(OBJS) $(SOBJS)

distclean:	clean
	rm -f $(LIB) core *.bak .depend

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

$(obj)secure_measured_boot.c:
	                @rm -f $(obj)secure_measured_boot.c
	                ln -s $(src)../common/secure_measured_boot.c $(obj)secure_measured_boot.c
