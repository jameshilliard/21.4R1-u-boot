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

VPATH	= $(TOPDIR)

include $(TOPDIR)/config.mk

LIB	= $(obj)lib$(BOARD).a

COBJS	:= 	$(BOARD).o \
		spd_sdram.o fsl_i2c.o tsec.o \
		pci_express.o pcie_indirect.o pcie_auto.o pcie.o \
		cmd_mx80.o tbbcpld.o

SOBJS	:= init.o

SRCS	:= $(SOBJS:.o=.S) $(COBJS:.o=.c)
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

