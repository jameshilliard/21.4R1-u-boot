#
# Copyright (c) 2010, Juniper Networks, Inc.
# All rights reserved.
#
# (C) Copyright 2002-2006
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

include $(TOPDIR)/config.mk

SRCS 	:= $(AOBJS:.o=.S) $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(AOBJS) $(COBJS))
LIB	:= $(obj)$(LIB)

CFLAGS += -I$(TOPDIR) -I$(TOPDIR)/drivers
CFLAGS += -I$(TOPDIR)/board/juniper/ex45xx -I$(TOPDIR)/board/juniper/ex3242
CPPFLAGS += $(CFLAGS)

all:	$(LIB)

$(LIB):	$(obj).depend $(OBJS) $(LOBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS) $(LOBJS)

$(obj)cpu.o:	$(TOPDIR)/post/ex3242/cpu.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)memory.o:	$(TOPDIR)/post/ex3242/memory.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)post.o:	$(TOPDIR)/post/ex3242/post.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)tests.o:	$(TOPDIR)/post/ex3242/tests.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)uart.o:	$(TOPDIR)/post/ex3242/uart.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)usb.o:	$(TOPDIR)/post/ex3242/usb.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)watchdog.o:	$(TOPDIR)/post/ex3242/watchdog.c
	$(CC) $(CPPFLAGS) -c -o $@ $<


#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
