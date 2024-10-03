#
# Copyright (c) 2010, Juniper Networks, Inc.
# All rights reserved.
#
# Copyright 2004 Freescale Semiconductor.
# (C) Copyright 2001
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# See file CREDITS for list of people who contributed to this
# project.
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
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

VPATH	= $(TOPDIR)

include $(TOPDIR)/config.mk

LIB	= $(obj)lib$(BOARD).a

COBJS	:= $(BOARD).o \
                  cmd_ex45xx.o \
                  cmd_m1m2.o \
                  epld.o \
                  epld_watchdog.o \
                  led.o \
                  lcd.o \
                  pci_express.o \
                  pcie.o \
                  spd_sdram.o \
                  tsec.o

SOBJS	:= init.o

LOBJS	:= cmd_jmem.o \
                  fsl_i2c.o 

SRCS	:= $(SOBJS:.o=.S) $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))
SOBJS	:= $(addprefix $(obj),$(SOBJS))
LOBJS	:= $(addprefix $(obj),$(LOBJS))

CFLAGS += -I$(TOPDIR) -I$(TOPDIR)/drivers -I$(TOPDIR)/post/ex45xx
CPPFLAGS += $(CFLAGS)

$(LIB):	$(obj).depend $(OBJS) $(SOBJS) $(LOBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS) $(LOBJS)

$(obj)cmd_jmem.o:	$(TOPDIR)/board/juniper/ex3242/cmd_jmem.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)fsl_i2c.o:	$(TOPDIR)/board/juniper/ex3242/fsl_i2c.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(SOBJS)

distclean:	clean
	rm -f $(LIB) core *.bak .depend

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
